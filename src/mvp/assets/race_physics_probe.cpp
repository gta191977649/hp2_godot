#include "race_physics_probe.hpp"

#include "bware/inc/bMath.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/FMenus.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/Language.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/TrackedController.hpp"
#include "indep/inc/World.hpp"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/os.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <locale>
#include <stdexcept>

namespace hp2 {
namespace {
constexpr unsigned responseTicks = 120;
constexpr unsigned maxTicks = 1800;

const CarFramePlayerState* player(const WorldSimulation& world) {
    return world.players->playersByNumber[0];
}

void vector_csv(std::ostream& stream, const float* vector) {
    for (int axis = 0; axis < 3; ++axis) stream << ',' << vector[axis];
}

float nonnegative_number(const godot::String& value, const char* name) {
    const std::string input = value.utf8().get_data();
    std::size_t length{};
    const auto result = std::stof(input, &length);
    if (length != input.size() || !std::isfinite(result) || result < 0)
        throw std::invalid_argument(std::string("Invalid race physics probe ") + name);
    return result;
}
}

void RacePhysicsProbe::configure(bool isolatedProfile, int track) {
    const auto* os = godot::OS::get_singleton();
    const auto path = os->get_environment("HP2_MVP_PHYSICS_TRACE");
    if (path.is_empty()) return;
    if (!isolatedProfile || os->get_environment("HP2_MVP_TEST_PROFILE") != "1" || track < 0)
        throw std::invalid_argument("Race physics probe requires an isolated Quick Race");
    track_ = track;
    model_ = os->get_environment("HP2_MVP_PHYSICS_CAR").utf8().get_data();
    if (model_ != "BMWM5" && model_ != "MURCIELAGO" && model_ != "ELISE")
        throw std::invalid_argument("Race physics probe requires BMWM5, MURCIELAGO or ELISE");
    targetSpeed_ = nonnegative_number(os->get_environment("HP2_MVP_PHYSICS_SPEED"), "speed");
    triggerDistance_ = nonnegative_number(os->get_environment("HP2_MVP_PHYSICS_DISTANCE"), "distance");
    const auto maneuver = os->get_environment("HP2_MVP_PHYSICS_MANEUVER");
    if (maneuver == "right") maneuver_ = Maneuver::Right;
    else if (maneuver == "key-left") maneuver_ = Maneuver::KeyLeft;
    else if (maneuver == "graze") maneuver_ = Maneuver::Graze;
    else if (maneuver != "left") throw std::invalid_argument("Unknown race physics probe maneuver");
    const std::filesystem::path prefix(path.wide_string().get_data());
    auto stepPath = prefix;
    auto framePath = prefix;
    ticks_.open(stepPath += ".ticks.csv");
    frames_.open(framePath += ".frames.csv");
    if (!ticks_ || !frames_) throw std::runtime_error("Could not open race physics trace files");
    for (auto* stream : {&ticks_, &frames_}) {
        stream->imbue(std::locale::classic());
        *stream << std::setprecision(std::numeric_limits<float>::max_digits10);
    }
    ticks_ << "generation,epoch,step,case_tick,pulse_tick,world_timer,quanta,world_dt,time_scale,"
              "nominal_substeps,last_force_dt,"
              "stick,steering,steer_angle,throttle,gear,speed,route_distance,route_section,"
              "yaw_rate,lateral_speed,x,y,z,vx,vy,vz,forward_x,forward_y,forward_z,"
              "slip_fl,slip_fr,slip_rr,slip_rl,wheel_contacts,impact_count,impact_x,impact_y,impact_z,world_rng\n";
    frames_ << "step,render_us,car_x,car_y,car_z,car_fx,car_fy,car_fz,"
               "camera_x,camera_y,camera_z,camera_fx,camera_fy,camera_fz\n";
}

void RacePhysicsProbe::start_race(MainMenuScreenServices& menus, const void* context) {
    const auto& probe = *static_cast<const RacePhysicsProbe*>(context);
    const auto& setup = *menus.raceSetup;
    const auto* track = setup.tracks->Find(probe.track_);
    if (!track) throw std::invalid_argument("Race physics probe track is absent from the catalogue");
    const auto& model = setup.cars->Find(probe.model_);
    auto& frontend = *menus.frontend;
    auto& race = *setup.parameters;
    auto& profile = *frontend.players[0];
    raceParametersInitializeDefaults(&race, setup.applicationOptions, &setup.language->currentLanguage);
    menus.screens->StopMovie();
    frontend.gameType = 2;
    frontend.playerCount = 1;
    frontend.copsEnabled = false;
    race.selectedScenario = probe.track_;
    race.authoredTrackOptionSecond = track->timeOfDay;
    race.pointToPoint = track->routeKind;
    race.raceKind = 1;
    race.totalLaps = 2;
    race.mode = 1;
    race.opponentCount = 0;
    race.policeDriverCount = 0;
    race.trafficDensity = 0.0f;
    race.playerStartingPositions[0] = 1;
    race.eventRandomSeed = 0x118u;
    profile.SetCarAvailability(0, model.modelCode, -1, true);
    profile.AssignCarOfType(frontend.gameType, model.modelCode, model.defaultSkinNumber, false);
    profile.settings.gearInputMode = 0;
    frontend.StartRace(setup);
}

void RacePhysicsProbe::before_step(const WorldSimulation& world) {
    const auto* local = player(world);
    recording_ = !complete_ && world.race && world.race->state == 4 &&
        local && local->car && local->car->physics && local->driver &&
        local->driver->kind == CAR_DRIVER_REAL && local->driver->owner.real;
    steer_ = 128;
    if (!recording_) return;
    timeScale_ = world.world->timing.timeScale;
    const auto& car = *local->car;
    if (triggerStep_ < 0 && car.bindings.state->forwardSpeed >= targetSpeed_ &&
        car.routeLocation.distance >= triggerDistance_)
        triggerStep_ = static_cast<int>(ticksWritten_);
    if (triggerStep_ < 0) return;
    const int since = static_cast<int>(ticksWritten_) - triggerStep_;
    switch (maneuver_) {
    // 32 raw units clear the original scanner's dead zone and hysteresis.
    case Maneuver::Left: if (since < 3) steer_ = 96; break;
    case Maneuver::Right: if (since < 3) steer_ = 160; break;
    case Maneuver::KeyLeft: if (since == 0) steer_ = 0; break;
    case Maneuver::Graze: if (since < 18) steer_ = 168; break;
    }
}

bool RacePhysicsProbe::read_input(ControllerState& pad) const {
    if (!recording_) return false;
    pad.connected = true;
    pad.sticks[2] = static_cast<std::uint8_t>(steer_);
    SetButton(pad, Ps2Button::Cross, true);
    return true;
}

void RacePhysicsProbe::after_step(const WorldSimulation& world) {
    if (!recording_) return;
    // Only the scanner read inside this transaction borrows the injected input.
    recording_ = false;
    const auto& local = *player(world);
    const auto& car = *local.car;
    const auto& state = *car.bindings.state;
    const auto& clock = *world.clock;
    float impulse[3]{};
    unsigned count{};
    for (std::size_t i = 0; i < world.collisions->impactCapacity; ++i) {
        const auto& impact = world.collisions->impacts[i];
        if (!impact.occupied || impact.lastTicks != clock.worldTimer || !impact.pair) continue;
        const auto* participant = car.collisionParticipant;
        const auto sign = impact.pair->first == participant ? 1 : impact.pair->second == participant ? -1 : 0;
        if (!sign) continue;
        ++count;
        for (int axis = 0; axis < 3; ++axis) impulse[axis] += static_cast<float>(sign) * impact.impulse[axis];
    }
    const auto lateral = state.velocity[0] * state.orientation[4] +
        state.velocity[1] * state.orientation[5] + state.velocity[2] * state.orientation[6];
    ticks_ << world.world->presentationGeneration << ',' << world.world->presentationReplayEpoch << ','
           << clock.worldLoopCounter << ',' << ticksWritten_ << ','
           << (triggerStep_ < 0 ? -1 : static_cast<int>(ticksWritten_) - triggerStep_) << ','
           << clock.worldTimer << ',' << clock.realTimeQuantized << ',' << clock.worldTimeElapsed << ',' << timeScale_ << ','
           << convertFloatToInt32(clock.worldTimeElapsed / car.bindings.environment->tuning->maximumStep) + 1
           << ',' << car.physics->wheelSteps[0].elapsed << ',' << steer_ << ','
           << local.driver->owner.real->input.steeringInput << ',' << state.steeringAngle << ','
           << state.effectiveThrottle << ',' << state.gear << ',' << state.forwardSpeed << ','
           << car.routeLocation.distance << ',' << (car.routeLocation.node ? car.routeLocation.node->section : -1)
           << ',' << state.angularVelocity[2] << ',' << lateral;
    vector_csv(ticks_, state.position);
    vector_csv(ticks_, state.velocity);
    vector_csv(ticks_, state.orientation);
    for (const auto slip : state.wheelSlipRatio) ticks_ << ',' << slip;
    ticks_ << ',' << state.activeContactCount << ',' << count;
    vector_csv(ticks_, impulse);
    ticks_ << ',' << world.world->simulationRandomState << '\n';
    ++ticksWritten_;
    complete_ = triggerStep_ < 0 ? ticksWritten_ >= maxTicks :
        ticksWritten_ - static_cast<unsigned>(triggerStep_) >= responseTicks;
    if (complete_) {
        ticks_.flush();
        if (!ticks_) throw std::runtime_error("Could not write race physics tick trace");
    }
}

void RacePhysicsProbe::render_sample(unsigned step, const godot::Node3D& car, const godot::Node3D& camera) {
    if (!frames_.is_open()) return;
    const auto sample = [this](const godot::Node3D& node, int axis) {
        const auto transform = node.get_global_transform();
        const auto forward = transform.basis.get_column(axis);
        frames_ << ',' << transform.origin.x << ',' << transform.origin.y << ',' << transform.origin.z
                << ',' << forward.x << ',' << forward.y << ',' << forward.z;
    };
    const auto observed = std::chrono::steady_clock::now().time_since_epoch();
    frames_ << step << ',' << std::chrono::duration_cast<std::chrono::microseconds>(observed).count();
    sample(car, 0);
    sample(camera, 2);
    frames_ << '\n';
    if (complete_) {
        frames_.close();
        if (!frames_) throw std::runtime_error("Could not write race physics render trace");
    }
}

}
