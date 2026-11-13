#include "race_presentation.hpp"

#include "bridge/entity_state_bridge.hpp"
#include "systems/camera/camera_presentation.hpp"
#include "systems/frontend/frontend_presentation.hpp"
#include "systems/track/track_presentation.hpp"
#include "systems/vehicle/vehicle_presentation.hpp"

#include "bsound/include/bSound.h"
#include "bware/inc/bFile.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/World.hpp"
#include "port/not_implemented.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace hp2 {

using namespace godot;

namespace {
class RaceHost final : public NativeApplicationHost {
public:
    explicit RaceHost(Node3D& root) : root_(root) {}

    bool inRace{};
    bool PollEvents() override { return true; }
    int Width() const override { return root_.get_viewport()->get_visible_rect().size.x; }
    int Height() const override { return root_.get_viewport()->get_visible_rect().size.y; }
    std::size_t AvailableMemory() const override { return 64u * 1024u * 1024u; }
    void ApplyDisplaySettings(int32_t, int32_t, bool, bool) override {}
    void SubmitFrame(EcstasyFrame&) override {
        throw std::logic_error("Native frame submission reached the state-only host");
    }
    void RecordSessionEvent(const char* event, const char* detail) noexcept override {
        if (std::string_view(event) == "application.state" || std::string_view(event) == "shutdown.failure")
            std::fprintf(stderr, "HP2_MVP_TRACE %s %s\n", event, detail);
    }
    WorldAudioResult OpenAudio(bSound& sound) override { return sound.OpenWindowsDevice(); }
    WorldAudioResult PumpAudio(bSound& sound) override { return sound.PumpWindowsDevice(); }
    WorldAudioResult CloseAudio(bSound& sound) override { return sound.CloseWindowsDevice(); }
    void SetRumble(int32_t, uint8_t, uint8_t) override {}
    int32_t ReadInput(int32_t port, JoystickControls& destination) override {
        ControllerState pad;
        if (port == 0) {
            auto& input = *Input::get_singleton();
            const auto pressed = [&](Key key) { return input.is_physical_key_pressed(key); };
            pad.connected = true;
            if (!inRace) {
                SetButton(pad, Ps2Button::Cross, pressed(KEY_ENTER) || pressed(KEY_KP_ENTER));
                SetButton(pad, Ps2Button::Triangle, pressed(KEY_BACKSPACE));
                SetButton(pad, Ps2Button::Up, pressed(KEY_UP) || pressed(KEY_W));
                SetButton(pad, Ps2Button::Down, pressed(KEY_DOWN) || pressed(KEY_S));
                SetButton(pad, Ps2Button::Left, pressed(KEY_LEFT) || pressed(KEY_A));
                SetButton(pad, Ps2Button::Right, pressed(KEY_RIGHT) || pressed(KEY_D));
                destination = NativeScannerControls(pad);
                return 1;
            }
            const bool accelerate = pressed(KEY_W) || pressed(KEY_UP);
            const bool brake = pressed(KEY_S) || pressed(KEY_DOWN);
            const bool left = pressed(KEY_A) || pressed(KEY_LEFT);
            const bool right = pressed(KEY_D) || pressed(KEY_RIGHT);
            pad.sticks[2] = left == right ? 128 : left ? 0 : 255;
            SetButton(pad, Ps2Button::Cross, accelerate);
            SetButton(pad, Ps2Button::Square, brake);
            SetButton(pad, Ps2Button::Triangle, pressed(KEY_SPACE));
        }
        destination = NativeScannerControls(pad);
        return pad.connected ? 1 : 0;
    }

private:
    Node3D& root_;
};
}

struct RacePresentation::Session {
    Session(RacePresentation& owner, const String& assets, const String& profile)
        : host(owner), files(std::filesystem::path(assets.wide_string().get_data())),
          runtime(files, host, std::filesystem::path(profile.wide_string().get_data())) {}

    RaceHost host;
    GameResources files;
    ApplicationRuntime runtime;
    EntityStateBridge bridge;
    Node2D* frontendCanvas{};
    std::unique_ptr<FrontendPresentation> frontend;
    String frontendScreen;
    std::uint32_t selectedItem{};
    bool frontendMode{};
    Node3D* scene{};
    Camera3D* camera{};
    Label* hud{};
    std::unordered_map<std::uint64_t, VehiclePresentation> vehicles;
    std::size_t placements{};
    bool failed{};
    std::chrono::steady_clock::time_point loadingSince{std::chrono::steady_clock::now()};
};

RacePresentation::RacePresentation() = default;
RacePresentation::~RacePresentation() {
    if (!session_) return;
    try {
        session_->runtime.Shutdown();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "HP2_MVP_ASSERT stage=shutdown track=12 reason=%s\n", error.what());
    }
}

void RacePresentation::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_race", "assets", "profile"), &RacePresentation::start_race);
    ClassDB::bind_method(D_METHOD("start_frontend", "assets", "profile", "ffmpeg"), &RacePresentation::start_frontend);
    ClassDB::bind_method(D_METHOD("telemetry"), &RacePresentation::telemetry);
}

void RacePresentation::fail(const char* stage, const char* reason) {
    if (session_) session_->failed = true;
    set_process(false);
    const auto generation = session_ ? session_->bridge.current().generation : 0;
    const auto step = session_ ? session_->bridge.current().step : 0;
    const auto track = session_ && session_->frontendMode ? session_->runtime.SelectedTrack() : 12;
    ERR_PRINT(String("HP2_MVP_ASSERT stage=") + stage + " track=" + String::num_int64(track) + " generation=" +
        String::num_uint64(generation) + " last_step=" + String::num_uint64(step) + " reason=" + reason);
    get_tree()->quit(1);
}

bool RacePresentation::start_frontend(const String& assets, const String& profile, const String& ffmpeg) {
    return start(assets, profile, ffmpeg, true);
}

bool RacePresentation::start_race(const String& assets, const String& profile) {
    return start(assets, profile, {}, false);
}

bool RacePresentation::start(const String& assets, const String& profile, const String& ffmpeg, bool frontend) {
    const char* stage = "asset_source";
    try {
        if (session_) throw std::logic_error("Track 12 is already running");
        session_ = std::make_unique<Session>(*this, assets, profile);
        session_->frontendMode = frontend;
        stage = frontend ? "frontend_initialize" : "race_initialize";
        if (frontend) {
            session_->runtime.InitializeFrontend(12);
            session_->frontendCanvas = memnew(Node2D);
            add_child(session_->frontendCanvas);
            session_->frontend = std::make_unique<FrontendPresentation>(*session_->frontendCanvas, session_->files, ffmpeg);
        } else {
            session_->host.inRace = true;
            session_->runtime.InitializeRace(12);
        }
        session_->hud = memnew(Label);
        add_child(session_->hud);
        session_->hud->set_position(Vector2(24, 24));
        session_->hud->add_theme_font_size_override("font_size", 25);
        session_->hud->set_text(frontend ? "Loading frontend" : "Loading Track 12");
        set_process(true);
        return true;
    } catch (const std::exception& error) {
        fail(stage, error.what());
        return false;
    }
}

void RacePresentation::_process(double) {
    if (!session_ || session_->failed) return;
    auto& s = *session_;
    const char* stage = s.frontend ? "frontend_flow" : "world_step";
    try {
        if (!s.runtime.AdvanceRace()) {
            get_tree()->quit();
            set_process(false);
            return;
        }
        const auto race = s.runtime.RaceState();
        if (!race.simulation) {
            if (s.frontend) {
                stage = "frontend_scene";
                const auto state = s.runtime.FrontendState();
                s.frontendScreen = state.screen ? String(state.screen) : String();
                s.selectedItem = state.selectedItem;
                s.frontend->update(state, get_viewport()->get_visible_rect().size);
                s.hud->set_visible(!state.screen);
                s.hud->set_text(s.runtime.Flow().currentState >= 4 ? "Loading Track 12" : "Loading frontend");
                return;
            }
            if (s.scene) throw NotImplemented("Track 12 Godot", "Transition out of the Track 12 race");
            if (std::chrono::steady_clock::now() - s.loadingSince > std::chrono::seconds(45))
                throw std::runtime_error(std::string("World did not start; GameFlow=") +
                    std::to_string(s.runtime.Flow().currentState));
            return;
        }
        if (s.frontend) {
            s.frontend.reset();
            remove_child(s.frontendCanvas);
            s.frontendCanvas->queue_free();
            s.frontendCanvas = nullptr;
            s.host.inRace = true;
            s.hud->show();
        }
        stage = "race_state";
        if (race.sessionState != RC_STATE_RACE_IN_PROGRESS) {
            const auto reason = "Godot presentation for RaceCoordinator state " + std::to_string(race.sessionState);
            throw NotImplemented("Track 12 Godot", reason.c_str());
        }
        if (s.bridge.current().generation == race.simulation->world->presentationGeneration &&
            s.bridge.current().step == race.simulation->world->frameCounter) return;

        stage = "entity_state_bridge";
        auto frame = s.bridge.stage(race);
        if (frame.track != 12) throw std::runtime_error("World committed track " + std::to_string(frame.track));
        if (frame.generation == s.bridge.current().generation && frame.step < s.bridge.current().step)
            throw NotImplemented("Track 12 Godot", "World rewind within a presentation generation");
        const bool newScene = !s.scene || s.bridge.current().generation != frame.generation;
        if (newScene) {
            stage = "track_assets";
            if (!race.scenery) throw std::runtime_error("Committed track has no loaded scenery");
            if (s.scene) {
                remove_child(s.scene);
                s.scene->queue_free();
            }
            s.vehicles.clear();
            s.scene = memnew(Node3D);
            add_child(s.scene);
            s.scene->set_name("Track12Scene");
            s.placements = attach_track(*s.scene, *race.scenery, race.sky);
            s.camera = memnew(Camera3D);
            s.scene->add_child(s.camera);
            s.camera->make_current();
        }
        stage = "vehicle_assets";
        const auto& world = *race.simulation->world;
        for (std::size_t slot = 0; slot < frame.vehicles.size(); ++slot) {
            const auto& state = frame.vehicles[slot];
            auto [it, added] = s.vehicles.try_emplace(state.handle);
            auto& vehicle = it->second;
            if (added) vehicle.attach(*s.scene, *world.carTable[slot], state, race.worldFrames);
            if (vehicle.model != state.model || vehicle.skin != state.skin)
                throw NotImplemented("Track 12 Godot", "Appearance replacement within a vehicle handle");
            vehicle.update(state);
            if (state.player) s.hud->set_text("Track 12   " + String::num(std::abs(state.speed) * 3.6, 0) +
                " km/h\nWASD / arrows    Space handbrake    Esc quit");
        }
        for (auto it = s.vehicles.begin(); it != s.vehicles.end();) {
            if (std::none_of(frame.vehicles.begin(), frame.vehicles.end(),
                    [&](const VehicleEntityState& state) { return state.handle == it->first; })) {
                s.scene->remove_child(it->second.root);
                it->second.root->queue_free();
                it = s.vehicles.erase(it);
            } else ++it;
        }

        stage = "camera";
        apply_race_camera(*s.camera, frame.camera);
        if (newScene)
            std::fprintf(stderr, "HP2_MVP_TRACE stage=scene_publish track=12 generation=%llu step=%u placements=%zu vehicles=%zu\n",
                static_cast<unsigned long long>(frame.generation), frame.step, s.placements, s.vehicles.size());
        s.bridge.publish(std::move(frame));
    } catch (const std::exception& error) {
        fail(stage, error.what());
    }
}

Dictionary RacePresentation::telemetry() const {
    Dictionary result;
    if (!session_) return result;
    const auto& frame = session_->bridge.current();
    if (session_->frontend) {
        result["screen"] = session_->frontendScreen;
        result["selected_item"] = session_->selectedItem;
        result["video_playing"] = session_->frontend->video_playing();
        result["video_request"] = session_->frontend->video_request();
    }
    result["generation"] = frame.generation;
    result["step"] = frame.step;
    result["placements"] = static_cast<int64_t>(session_->placements);
    result["vehicles"] = static_cast<int64_t>(frame.vehicles.size());
    for (const auto& vehicle : frame.vehicles) {
        if (!vehicle.player) continue;
        result["speed"] = vehicle.speed;
        result["position"] = Vector3(vehicle.transform[12], vehicle.transform[13], vehicle.transform[14]);
    }
    return result;
}

}
