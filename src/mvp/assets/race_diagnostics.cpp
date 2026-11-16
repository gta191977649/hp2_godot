#include "race_diagnostics.hpp"
#include "systems/effects/race_effect_options.hpp"

#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/EventTree.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/FMenus.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/Language.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/TrackedController.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/src/speech/actions.hpp"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <algorithm>
#include <stdexcept>

#ifdef HP2_MVP_IMGUI
#include "tests/pursuit_test_driver.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/Chopper.hpp"
#include "indep/inc/Munitions.hpp"
#include "indep/inc/spikestrip.hpp"
#include <cstdio>
#endif

namespace hp2 {

bool profile_grid_effect_disabled(const char* mode) {
    static const auto selected = [] {
        const auto* os = godot::OS::get_singleton();
        if (os->get_environment("HP2_MVP_TEST_PROFILE") != "1" ||
            os->get_environment("HP2_MVP_PROFILE_GRID") != "1") return godot::String{};
        return os->get_environment("HP2_MVP_PROFILE_GRID_EFFECT");
    }();
    return selected == mode;
}

bool profile_legacy_sun_bounds() {
    return profile_grid_effect_disabled("legacy-sun-bounds");
}

bool profile_baseline_particle_draw() {
    const auto* os = godot::OS::get_singleton();
    return os->get_environment("HP2_MVP_TEST_PROFILE") == "1" &&
        os->get_environment("HP2_MVP_PROFILE_PARTICLES") == "baseline";
}

void profile_fog_state(const RacePostState& post, RaceEffectOptions& options, godot::Dictionary& sample) {
    static const auto mode = [] {
        const auto* os = godot::OS::get_singleton();
        if (os->get_environment("HP2_MVP_TEST_PROFILE") != "1") return godot::String{};
        return os->get_environment("HP2_MVP_PROFILE_FOG");
    }();
    if (mode.is_empty()) return;
    options.fog &= mode != "no-fog";
    options.depthOfField &= mode != "no-focus";
    options.motionBlur &= mode != "no-history";
    sample["fog_strength"] = post.fogEnabled ? post.fog[3] : 0.0f;
    sample["fog_distance"] = post.fogRange[0];
    sample["fog_mode"] = post.fogMode;
    sample["focus_taps"] = options.depthOfField && post.focusEnabled ? post.focusPasses : 0;
    sample["radial"] = options.motionBlur && post.radialActive;
    sample["history"] = options.motionBlur && post.radialMask != nullptr;
}

bool profile_legacy_primary_scenery() {
    const auto* os = godot::OS::get_singleton();
    return os->get_environment("HP2_MVP_PROFILE_GRID") == "1" &&
        os->get_environment("HP2_MVP_TEST_PROFILE") == "1" &&
        os->get_environment("HP2_MVP_PROFILE_LEGACY_SCENERY") == "1";
}

bool profile_baseline_scenery_lod() {
    const auto* os = godot::OS::get_singleton();
    return os->get_environment("HP2_MVP_TEST_PROFILE") == "1" &&
        os->get_environment("HP2_MVP_PROFILE_BASELINE_SCENERY_LOD") == "1";
}

bool test_scenery_camera_enabled() {
    const auto* os = godot::OS::get_singleton();
    return os->get_environment("HP2_MVP_TEST_PROFILE") == "1" &&
        os->get_environment("HP2_MVP_TEST_SCENERY_CAMERA") == "1";
}

bool profile_shared_world_reflections() {
    const auto* os = godot::OS::get_singleton();
    return os->get_environment("HP2_MVP_PROFILE_REFLECTIONS") == "1" &&
        os->get_environment("HP2_MVP_TEST_PROFILE") == "1" &&
        os->get_environment("HP2_MVP_PROFILE_SHARED_REFLECTION_WORLD") == "1";
}

FrontendReady reflection_profile_frontend_ready() {
    const auto* os = godot::OS::get_singleton();
    if (os->get_environment("HP2_MVP_PROFILE_REFLECTIONS") != "1" ||
        os->get_environment("HP2_MVP_TEST_PROFILE") != "1") return nullptr;
    return [](MainMenuScreenServices& menus, const void*) {
        constexpr std::uint32_t seed = 0x0146u;
        menus.raceSetup->parameters->eventRandomSeed = seed;
        (void)vectorRandomSeed(seed, &vectorDefaultRandomState);
        const auto track = godot::OS::get_singleton()->get_environment("HP2_MVP_PROFILE_DEMO_TRACK");
        menus.demoMode->RequestDemoMode(track == "44" ? 44 : -1);
    };
}

FrontendReady race_profile_frontend_ready() {
    const auto* os = godot::OS::get_singleton();
    if (os->get_environment("HP2_MVP_PROFILE_GRID") != "1" ||
        os->get_environment("HP2_MVP_TEST_PROFILE") != "1") return reflection_profile_frontend_ready();
    return [](MainMenuScreenServices& menus, const void*) {
        (void)vectorRandomSeed(0x0146u, &vectorDefaultRandomState);
        menus.frontend->copsEnabled = false;
        StartQuickRace(menus, 0, 13);
    };
}

godot::PackedVector3Array race_photo_route(const RaceStateView& race) {
    godot::PackedVector3Array points;
    const auto* simulation = race.simulation;
    if (!simulation || !simulation->race || !simulation->race->route || !simulation->players) return points;
    const auto* player = simulation->players->playersByNumber[0];
    if (!player || !player->car) return points;
    const auto* route = simulation->race->route;
    const auto* movement = player->car;
    auto* terrain = movement && movement->bindings.environment ? movement->bindings.environment->contact.terrain : nullptr;
    if (!terrain || !terrain->topology) return points;
    TerrainSurface query{};
    terrainSurfaceInitialize(&query, terrain->topology);
    constexpr int count = 96;
    points.resize(count);
    const auto& section = route->sections[0];
    for (int i = 0; i < count; ++i) {
        const auto& node = route->nodes[section.firstNode + (section.nodeCount - 1) * i / count];
        const float point[4]{node.point[0], node.point[1], 0, 0};
        bool found = false;
        const auto height = terrainSurfaceElevation(&query, point, nullptr, nullptr, &found);
        if (found) points.set(i, godot::Vector3(point[0], height, -point[1]));
    }
    terrainSurfaceDispose(&query);
    return points;
}

void configure_race_diagnostics(bool nativeDebug, bool unlockRaces, bool isolatedProfile) {
    applicationConfigureTesting(nativeDebug, unlockRaces && isolatedProfile, isolatedProfile);
}

void collect_police_diagnostics(godot::Dictionary& result, const RaceStateView& race,
    const FrontendStateView& frontend) {
    if (!race.beTheCop || !race.simulation || !race.simulation->pursuit) return;
    const auto& pursuit = *race.simulation->pursuit;
    const auto* selected = pursuitManagerSelectedEntry(&pursuit);
    const auto* group = selected ? selected->group : nullptr;
    const auto* players = race.simulation->players;
    const auto* player = players ? players->playersByNumber[0] : nullptr;
    const auto clock = hashString("BETHECOP_TIME");
    result["police_hud"] = frontend.layout && std::any_of(frontend.layout->layers.begin(),
        frontend.layout->layers.end(), [clock](const auto& layer) {
            return std::any_of(layer.begin(), layer.end(),
                [clock](const FlailerStrip* strip) { return strip->sourceObject == clock; });
        });
    result["player_is_pursuing_officer"] = player && player->car && selected && selected->movement == player->car;
    result["police_seconds"] = pursuit.remainingSeconds;
    result["police_roadblocks"] = pursuit.roadBlocksRemaining;
    result["police_helicopters"] = pursuit.chopperRequestsRemaining;
    result["police_target_active"] = group && group->target;
    result["police_support_flags"] = group ? group->requestFlags : 0u;
    result["police_busts"] = pursuit.completedPullovers;
}

void collect_menu_alignment_diagnostics(godot::Dictionary& result, const FrontendStateView& frontend) {
    static const bool enabled = godot::OS::get_singleton()->get_environment("HP2_MVP_TEST_MENU_ALIGNMENT") == "1";
    if (!enabled || !frontend.layout) return;
    const auto page = std::find_if(frontend.pointerTargets.begin(), frontend.pointerTargets.end(),
        [](const auto& target) { return target.object == 0; });
    if (page == frontend.pointerTargets.end()) return;
    godot::Dictionary bounds;
    for (const auto& layer : frontend.layout->layers) for (const auto* strip : layer) {
        if (strip->sourceScreen != page->screen || !strip->sourceObject) continue;
        const auto key = static_cast<int64_t>(strip->sourceObject);
        for (auto* batch = strip; batch; batch = batch->nextTexture.get()) for (const auto& poly : batch->polygons) {
            if (std::none_of(poly.vertices.begin(), poly.vertices.end(),
                    [](const auto& vertex) { return (vertex.color >> 24) != 0; })) continue;
            godot::Rect2 rect({poly.vertices[0].position[0], poly.vertices[0].position[1]}, {});
            for (const auto& vertex : poly.vertices)
                rect = rect.expand({vertex.position[0], vertex.position[1]});
            if (bounds.has(key)) rect = rect.merge(static_cast<godot::Rect2>(bounds[key]));
            bounds[key] = rect;
        }
    }
    result["menu_alignment_bounds"] = bounds;
}

void collect_controls_diagnostics(godot::Dictionary& result, const RaceStateView& race,
    const FrontendStateView& frontend, const godot::String& device) {
    static const bool enabled = godot::OS::get_singleton()->get_environment("HP2_MVP_TEST_CONTROLS") == "1";
    if (!enabled) return;
    result["controls_device"] = device;
    result["controls_help_open"] = frontend.pointerHelp;
    const auto* players = race.simulation ? race.simulation->players : nullptr;
    const auto* player = players ? players->playersByNumber[0] : nullptr;
    if (player) {
        result["controls_hud_profile"] = player->settings.inputProfile;
        result["controls_camera_kind"] = carFramePlayerCameraKind(player);
        result["controls_camera_state"] = player->movement.steeringFeedbackBusy;
        result["controls_camera_view"] = player->settings.cameraView;
        result["controls_automatic"] = player->settings.gearInputMode == 0;
        result["controls_look_back"] = player->lookBack != 0;
    }
    const auto* driver = player ? player->driver : nullptr;
    if (driver && driver->kind == CAR_DRIVER_REAL && driver->owner.real) {
        const auto& input = driver->owner.real->input;
        result["controls_throttle"] = input.throttleInput;
        result["controls_brake"] = input.brakeInput;
        result["controls_steering"] = input.steeringInput;
        result["controls_handbrake"] = input.handbrakeInput;
        if (input.frame)
            result["controls_horn"] = carFrameState(input.frame)->blinkVisible != 0;
        if (input.movement) {
            const auto& bindings = input.movement->bindings;
            if (bindings.controls) result["controls_gear_request"] = bindings.controls->gearRequest;
            if (bindings.ownerMotion) result["controls_recovery_started"] = bindings.ownerMotion->windowStartTicks;
        }
        result["controls_nitro_active"] = realDriverNitroActive(&input) != 0;
    }
}

void collect_replay_diagnostics(godot::Dictionary& result, const RaceStateView& race,
    const FrontendStateView& frontend) {
    static const bool enabled = godot::OS::get_singleton()->get_environment("HP2_MVP_MOUSE_REPLAY") == "1";
    if (!enabled || !frontend.pointerReplay || !race.simulation || !race.simulation->replay) return;
    const auto* replay = race.simulation->replay->manager;
    if (!replay) return;
    result["replay_paused"] = replay->paused != 0;
    result["replay_slow"] = replay->slowMotion != 0;
    result["replay_seeking"] = frontend.replayPointer.seeking;
    result["replay_hud"] = frontend.replayPointer.screen == 0x60befaedu;
    result["replay_help"] = frontend.pointerHelp;
    result["replay_ticks"] = race.simulation->clock->worldTimer;
    result["replay_begin_ticks"] = replay->beginCapturingTime;
    result["replay_end_ticks"] = replay->endCapturingTime;

    for (const auto& layer : frontend.layout->layers) for (const auto* strip : layer) {
        if (strip->sourceScreen != 0x60befaedu || strip->sourceObject != 0xbd2bc882u ||
            strip->polygons.empty()) continue;
        const auto& quad = strip->polygons.front().vertices;
        const godot::Vector2 start(quad[0].position[0], quad[0].position[1]);
        result["replay_bar"] = godot::Rect2(start, godot::Vector2(quad[2].position[0], quad[2].position[1]) - start);
        return;
    }
}

void start_quick_race_diagnostic(MainMenuScreenServices& menus, const void* context) {
    const auto track = *static_cast<const int*>(context);
    if (!menus.raceSetup->tracks->Find(track))
        throw std::invalid_argument("Track is absent from the authored scenario catalogue");
    StartQuickRace(menus, 0, track);
    const auto* os = godot::OS::get_singleton();
    if (os->get_environment("HP2_MVP_TEST_PROFILE") == "1" &&
        os->get_environment("HP2_MVP_TEST_TRACK_REVERSE") == "1")
        menus.raceSetup->parameters->reverse = 1;
}

void start_campaign_race_diagnostic(MainMenuScreenServices& menus, const void* context) {
    const auto& request = *static_cast<const CampaignRaceDiagnostic*>(context);
    const auto& setup = *menus.raceSetup;
    auto* tree = setup.events->PrimaryTree(static_cast<std::size_t>(request.tree));
    auto* selected = tree ? tree->GetEventNumber(request.event) : nullptr;
    if (!selected) throw std::invalid_argument("Event is absent from the authored campaign tree");

    auto& frontend = *menus.frontend;
    auto& parameters = *setup.parameters;
    raceParametersInitializeDefaults(&parameters, setup.applicationOptions, &setup.language->currentLanguage);
    frontend.gameType = request.tree == 0 ? 8 : 9;
    frontend.playerCount = 1;
    frontend.copsEnabled = request.tree == 1;
    frontend.challengeSetup.raceOption254 = 1;
    frontend.eventSelection.treeType = request.tree;
    frontend.eventSelection.SetSelectedEvent(request.tree, request.event);
    parameters.authoredRaceOption254 = 1;
    tree->SetRace(parameters, *selected, {&frontend, setup.currentEvent, setup.cars, setup.tracks, setup.language});
    frontend.MakeEventCarsAvailable(*tree, *selected);

    const auto* authoredCar = selected->GetCar(0);
    Require(authoredCar, "Diagnostic event has no authored player car");
    const auto model = static_cast<int>((static_cast<std::uint32_t>(*authoredCar) >> 16) & 255u);
    auto& profile = *frontend.players[0];
    const auto& available = profile.availability.at(static_cast<std::size_t>(model));
    Require(available.skinCount > 0, "Diagnostic event has no authored player skin");
    profile.AssignCarOfType(frontend.gameType, model, available.skinNumbers[0], true);
    menus.screens->stack.Push("View Event Tree");
    frontend.StartRace(setup);
}

#ifdef HP2_MVP_IMGUI
bool RaceDiagnosticChecks::finish_for_menu_test(const RaceStateView& race) {
    if (!finishForMenuTest || race.sessionState != RC_STATE_RACE_IN_PROGRESS ||
        !race.simulation || race.simulation->world->frameCounter < 120)
        return false;
    auto* frame = race.simulation->race;
    Require(frame && frame->services && frame->services->session,
        "Results menu test requires the original race session");
    finishForMenuTest = false;
    Require(raceCoordinatorTheRaceHasFinished(*frame->services->session),
        "Results menu test could not complete the original race");
    return true;
}

void RaceDiagnosticChecks::advance(const WorldSimulation& simulation, int track, int raceState,
    std::uint64_t generation, std::uint32_t step) {
    if (raceState != RC_STATE_RACE_IN_PROGRESS) return;
    if (subtitleRadioFixture && step >= subtitleRadioFixture) {
        const auto index = subtitleRadioFixture / 240;
        constexpr std::uint32_t events[]{96, 100, 101};
        auto* speech = worldSpeechGetInstance();
        if (speech && speech->messages)
            Require(worldSpeechEnqueueAction(*speech->messages, events[index], {}) == WORLD_SPEECH_OK,
                "Cannot queue the original subtitle radio test event");
        subtitleRadioFixture = index + 1 < std::size(events) ? subtitleRadioFixture + 240 : 0;
    }
    if (fireTruckCamera && (track == 12 || track == 13)) {
        // Track B's FireTruck_WaterA..D instances span the roadside.
        auto& view = simulation.particles->cameras->screens[1].view->current;
        const float eye[] = {-577.0f, 121.0f, -50.0f};
        std::copy_n(eye, 3, view.eye);
        view.forward[0] = 0.0f;
        view.forward[1] = 0.957826f;
        view.forward[2] = -0.287348f;
        view.matrix[1] = 0.0f;
        view.matrix[5] = 0.0f;
        view.matrix[9] = -1.0f;
    } else if (trackEffectsCamera && (track == 12 || track == 21 || track == 61)) {
        // Views beside source 35020 emitters. The normal emitter
        // scheduler still decides when to emit using the actual view eye.
        const float forest[3] = {-654.92f, 190.07f, -53.85f};
        const float desert[3] = {1517.87f, 206.27f, 15.37f};
        const float tropical[3] = {-80.7261f, -200.3983f, 98.6204f};
        const float* target = track == 12 ? forest : track == 21 ? desert : tropical;
        auto& view = simulation.particles->cameras->screens[1].view->current;
        std::copy_n(target, 3, view.eye);
        view.eye[0] -= 35.0f;
        view.eye[1] -= 18.0f;
        view.eye[2] += 195.0f;
        view.forward[0] = 0.176f;
        view.forward[1] = 0.0905f;
        view.forward[2] = -0.9803f;
        view.matrix[1] = 0.0f;
        view.matrix[5] = 0.0f;
        view.matrix[9] = -1.0f;
    }
    if (supportVisuals && step >= 35) {
        const auto* manager = simulation.pursuit;
        auto* chopper = simulation.chopper;
        if (!manager || !chopper || manager->groupCount == 0) return;
        const auto* selected = pursuitManagerSelectedEntry(manager);
        if (!selected || !selected->group) return;
        auto* barricades = selected->group->owner->barricades;
        Require(barricades, "Police support test requires the selected pursuit's barricade owner");
        const auto* player = simulation.players->playersByNumber[0];
        const auto& car = *player->car->bindings.state;
        const float point[2] = {
            car.centerPosition[0] + car.orientation[0] * 18.0f,
            car.centerPosition[1] + car.orientation[1] * 18.0f
        };
        float ground[4];
        pursuitGroundPoint(barricades->topology, ground, point);
        std::int16_t barrel = -1;
        Require(pursuitBarricadesCreateAt(barricades, ground, &barrel) == WORLD_SERVICES_OK && barrel != -1,
            "Police support test could not create the authored roadblock barrel");
        auto* registry = chopper->environment.munitions;
        auto* bomb = chopperSmokeBombCreate(registry);
        Require(bomb, "Police support test could not create the authored smoke bomb");
        float pose[16];
        rotationMatrixIdentity(pose);
        std::copy_n(ground, 3, pose + 12);
        pose[14] += 2.0f;
        const float velocity[4]{};
        chopperMunitionSetTransform(bomb, pose, velocity);
        chopperMunitionDetonate(bomb);
        Require(registry->result == WORLD_SERVICES_OK, "Police support test smoke detonation failed");
        chopperMunitionDestroy(bomb);
        // A second original drop stays live long enough to inspect its mesh
        // regardless of how the AI approaches the selected target.
        auto* drop = chopperSmokeBombCreate(registry);
        Require(drop, "Police support test could not create the live smoke bomb");
        pose[14] += 8.0f;
        chopperMunitionSetTransform(drop, pose, velocity);
        supportVisuals = false;
        std::fprintf(stderr, "HP2_MVP_TRACE stage=support_visual_fixture barrel=%d\n", barrel);
    }
    if (track != 13) return;
    if (arrestCinematic && step >= 60) {
        start_arrest_cinematic_test(simulation);
        arrestCinematic = false;
    }
    if (pursuitDriver && pursuitGeneration != generation) {
        attach_pursuit_test_driver(*simulation.world, *simulation.players->playersByNumber[0]);
        pursuitGeneration = generation;
        std::fprintf(stderr, "HP2_MVP_TRACE stage=pursuit_test_driver track=13 generation=%llu\n",
            static_cast<unsigned long long>(generation));
    }
}
#endif

}
