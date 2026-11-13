#include "race_presentation.hpp"

#include "assets/race_diagnostics.hpp"
#include "bridge/entity_state_bridge.hpp"
#ifdef HP2_MVP_IMGUI
#include "assets/imgui_overlay.hpp"
#endif
#include "systems/audio/godot_speech_output.hpp"
#include "systems/camera/camera_presentation.hpp"
#include "systems/cinematic/nis_presentation.hpp"
#include "systems/effects/race_effects_presentation.hpp"
#include "systems/frontend/frontend_presentation.hpp"
#include "systems/track/track_presentation.hpp"
#include "systems/vehicle/vehicle_presentation.hpp"

#include "bsound/include/bSound.h"
#include "bware/inc/bFile.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/World.hpp"
#include "port/not_implemented.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
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
    bool menuInput{};
    bool beTheCop{};
#ifdef HP2_MVP_IMGUI
    ImGuiOverlay* debug{};
#endif
    bool PollEvents() override { return true; }
    int Width() const override { return root_.get_viewport()->get_visible_rect().size.x; }
    int Height() const override { return root_.get_viewport()->get_visible_rect().size.y; }
    std::size_t AvailableMemory() const override { return 64u * 1024u * 1024u; }
    void ApplyDisplaySettings(int32_t, int32_t, bool, bool) override {}
    void SubmitFrame(EcstasyFrame&) override {
        throw std::logic_error("Native frame submission reached the state-only host");
    }
    void RecordSessionEvent(const char* event, const char* detail) noexcept override {
        const std::string_view name(event);
        if (name == "application.state" || name == "shutdown.failure" || name == "resource.missing") {
            std::fprintf(stderr, "HP2_MVP_TRACE %s %s\n", event, detail);
#ifdef HP2_MVP_IMGUI
            try { if (debug) debug->log(event, detail); }
            catch (...) { /* Preserve the native host's noexcept log boundary. */ }
#endif
        }
    }
    WorldAudioResult OpenAudio(bSound& sound) override {
        // Voice allocation traces synchronously write to stderr during World updates.
        if (OS::get_singleton()->get_environment("HP2_MVP_AUDIO_TRACE") != "1")
            sound.DisablePrintf();
        if (OS::get_singleton()->get_environment("HP2_MVP_AUDIO_BACKEND") == "godot")
            speechOutput_ = std::make_unique<GodotSpeechOutput>(root_);
        return sound.OpenWindowsDevice(speechOutput_.get());
    }
    WorldAudioResult PumpAudio(bSound& sound) override { return sound.PumpWindowsDevice(); }
    WorldAudioResult CloseAudio(bSound& sound) override {
        const auto result = sound.CloseWindowsDevice();
        if (result == WORLD_AUDIO_OK) speechOutput_.reset();
        return result;
    }
    void SetRumble(int32_t, uint8_t, uint8_t) override {}
    int32_t ReadInput(int32_t port, JoystickControls& destination) override {
        ControllerState pad;
        if (port == 0) {
            pad.connected = true;
#ifdef HP2_MVP_IMGUI
            if (debug && debug->captures_game_input()) {
                destination = NativeScannerControls(pad);
                return 1;
            }
#endif
            auto& input = *Input::get_singleton();
            const auto pressed = [&](const StringName& action) { return input.is_action_pressed(action); };
            SetButton(pad, Ps2Button::Start, inRace && pressed("hp2_pause"));
            if (!inRace || menuInput) {
                policeSupportHeld_ = false;
                nitroModifierHeld_ = false;
                SetButton(pad, Ps2Button::Cross, pressed("hp2_menu_confirm"));
                SetButton(pad, Ps2Button::Triangle, !inRace && pressed("hp2_menu_back"));
                SetButton(pad, Ps2Button::Up, pressed("hp2_menu_up"));
                SetButton(pad, Ps2Button::Down, pressed("hp2_menu_down"));
                SetButton(pad, Ps2Button::Left, pressed("hp2_menu_left"));
                SetButton(pad, Ps2Button::Right, pressed("hp2_menu_right"));
                destination = NativeScannerControls(pad);
                return 1;
            }
            const bool accelerate = pressed("hp2_accelerate");
            const bool brake = pressed("hp2_brake");
            const bool left = pressed("hp2_steer_left");
            const bool right = pressed("hp2_steer_right");
            const bool roadblock = beTheCop && pressed("hp2_roadblock");
            const bool helicopter = beTheCop && pressed("hp2_helicopter");
            const bool nitro = pressed("hp2_nitro");
            const bool policeSupport = roadblock || helicopter;
            // PAL scanner requires the Triangle modifier to be held across
            // two samples before the L2/R2 primary edge.
            const bool supportReady = policeSupport && policeSupportHeld_;
            policeSupportHeld_ = policeSupport;
            pad.sticks[2] = left == right ? 128 : left ? 0 : 255;
            SetButton(pad, Ps2Button::Cross, accelerate);
            SetButton(pad, Ps2Button::Square, brake);
            SetButton(pad, Ps2Button::Triangle, pressed("hp2_handbrake") || policeSupport || nitro);
            SetButton(pad, Ps2Button::Circle, pressed("hp2_horn"));
            SetButton(pad, Ps2Button::L2, pressed("hp2_l2") || (supportReady && helicopter));
            SetButton(pad, Ps2Button::R2, pressed("hp2_r2") || (supportReady && roadblock));
            SetButton(pad, Ps2Button::L1, pressed("hp2_l1"));
            SetButton(pad, Ps2Button::R1, pressed("hp2_camera") || (nitro && nitroModifierHeld_));
            SetButton(pad, Ps2Button::Select, pressed("hp2_select"));
            SetButton(pad, Ps2Button::R3, nitro);
            nitroModifierHeld_ = nitro;
        }
        destination = NativeScannerControls(pad);
        return pad.connected ? 1 : 0;
    }

private:
    Node3D& root_;
    bool policeSupportHeld_{};
    bool nitroModifierHeld_{};
    std::unique_ptr<GodotSpeechOutput> speechOutput_;
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
    std::unique_ptr<NisPresentation> cinematic;
    std::unique_ptr<RaceEffectsPresentation> effects;
    RaceEffectsFrame effectsFrame;
    std::unique_ptr<WorldObjectPresentation> worldObjects;
    std::vector<TrackPlacementVisibility> movablePlacements;
    String frontendScreen;
    std::uint32_t selectedItem{};
    bool modal{};
    int raceState{};
    bool frontendMode{};
    int requestedTrack{-1};
    CampaignRaceDiagnostic campaignDiagnostic;
#ifdef HP2_MVP_IMGUI
    RaceDiagnosticChecks diagnosticChecks;
#endif
    Node3D* scene{};
    Camera3D* camera{};
    SubViewport* rearViewport{};
    Camera3D* rearCamera{};
    std::unordered_map<std::uint64_t, VehiclePresentation> vehicles;
    std::size_t placements{};
    std::chrono::steady_clock::time_point loadingSince{std::chrono::steady_clock::now()};
};

RacePresentation::RacePresentation() = default;
RacePresentation::~RacePresentation() {
    if (!session_) return;
    try {
        session_->runtime.Shutdown();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "HP2_MVP_ASSERT stage=shutdown track=%d reason=%s\n",
            session_->runtime.SelectedTrack(), error.what());
    }
}

void RacePresentation::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_race", "assets", "profile", "track"), &RacePresentation::start_race);
    ClassDB::bind_method(D_METHOD("start_event_race", "assets", "profile", "tree", "event"), &RacePresentation::start_event_race);
    ClassDB::bind_method(D_METHOD("start_frontend", "assets", "profile"), &RacePresentation::start_frontend);
    ClassDB::bind_method(D_METHOD("telemetry"), &RacePresentation::telemetry);
    ClassDB::bind_method(D_METHOD("report_launch_error", "stage", "track", "reason"),
        &RacePresentation::report_launch_error);
}

void RacePresentation::report_assert(const String& message) {
    failed_ = true;
    ERR_PRINT(message);
#ifdef HP2_MVP_IMGUI
    if (!debug_) debug_ = std::make_unique<ImGuiOverlay>(*this);
    debug_->log("assert", message.utf8().get_data(), true);
    debug_->show_console();
    set_process_input(true);
    set_process(true);
#else
    set_process(false);
    get_tree()->quit(1);
#endif
}

void RacePresentation::report_launch_error(const String& stage, const String& track, const String& reason) {
    report_assert("HP2_MVP_ASSERT stage=" + stage + " track=" + track + " generation=0 last_step=0 reason=" + reason);
}

void RacePresentation::quit_game() {
    try {
        retire_scene();
        session_->runtime.Shutdown();
        session_.reset();
        get_tree()->quit();
    } catch (const std::exception& error) {
        fail("shutdown", error.what());
    }
}

void RacePresentation::fail(const char* stage, const char* reason) {
    const auto generation = session_ ? session_->bridge.current().generation : 0;
    const auto step = session_ ? session_->bridge.current().step : 0;
    const auto track = session_ ? (session_->requestedTrack >= 0 ? session_->requestedTrack :
        session_->runtime.SelectedTrack()) : -1;
    const auto event = session_ && session_->campaignDiagnostic.tree >= 0 ?
        String(" tree=") + String::num_int64(session_->campaignDiagnostic.tree) +
        " event=" + String::num_int64(session_->campaignDiagnostic.event) : String();
    report_assert(String("HP2_MVP_ASSERT stage=") + stage + " track=" + String::num_int64(track) + " generation=" +
        String::num_uint64(generation) + " last_step=" + String::num_uint64(step) + event + " reason=" + reason);
}

bool RacePresentation::start_frontend(const String& assets, const String& profile) {
    return start(assets, profile, true, -1);
}

bool RacePresentation::start_race(const String& assets, const String& profile, int64_t track) {
    if (track < 0 || track > std::numeric_limits<int>::max()) {
        report_launch_error("launch", String::num_int64(track), "expected a nonnegative 32-bit track ID");
        return false;
    }
    return start(assets, profile, false, static_cast<int>(track));
}

bool RacePresentation::start_event_race(const String& assets, const String& profile, int tree, int event) {
    return start(assets, profile, false, -1, tree, event);
}

bool RacePresentation::start(const String& assets, const String& profile, bool frontend, int track, int tree, int event) {
    const char* stage = "asset_source";
    try {
        if (session_) throw std::logic_error("Game session is already running");
        session_ = std::make_unique<Session>(*this, assets, profile);
        // Reuse the donor's explicit racer Nitro option in the Godot host.
        applicationNativeGameplay = loadNativePcSettings(
            std::filesystem::path(profile.wide_string().get_data()) / "NFSHP2_pc.ini").gameplay;
        session_->frontendMode = frontend;
        session_->requestedTrack = track;
        session_->campaignDiagnostic = {tree, event};
        if (session_->files.IsLooseArchive()) {
            for (const char* bank : {"SOUND/common.msh", "SOUND/common.msb"}) {
                if (!session_->files.TryOpenPhysical(bank))
                    throw std::runtime_error(std::string("Incomplete extracted game source, missing ") + bank +
                        " in " + session_->files.SourcePath().u8string() +
                        ". Set mvp_root in config/ps2_assets.local.json to a complete PS2 disc image or archive.");
            }
        }
#ifdef HP2_MVP_IMGUI
        debug_ = std::make_unique<ImGuiOverlay>(*this);
        session_->host.debug = debug_.get();
        session_->diagnosticChecks.pursuitDriver = OS::get_singleton()->get_environment("HP2_MVP_PURSUIT_AI") == "1";
        session_->diagnosticChecks.supportVisuals = tree >= 0 &&
            OS::get_singleton()->get_environment("HP2_MVP_TEST_SUPPORT_VISUALS") == "1";
        session_->diagnosticChecks.trackEffectsCamera = !frontend &&
            OS::get_singleton()->get_environment("HP2_MVP_TEST_TRACK_EFFECTS") == "1";
        session_->diagnosticChecks.arrestCinematic = OS::get_singleton()->get_environment("HP2_MVP_TEST_ARREST_NIS") == "1";
        configure_race_diagnostics(
            OS::get_singleton()->get_environment("HP2_MVP_TEST_UNLOCK_RACES") == "1");
#endif
        set_process_input(true);
        stage = frontend ? "frontend_initialize" : "race_initialize";
        if (frontend) session_->runtime.InitializeFrontend();
        else if (tree >= 0) session_->runtime.InitializeRace(start_campaign_race_diagnostic, &session_->campaignDiagnostic);
        else session_->runtime.InitializeRace(start_quick_race_diagnostic, &session_->requestedTrack);
        session_->frontendCanvas = memnew(Node2D);
        session_->frontendCanvas->set_name("FrontendCanvas");
        add_child(session_->frontendCanvas);
        session_->frontend = std::make_unique<FrontendPresentation>(*session_->frontendCanvas, session_->files,
            session_->runtime, *this);
        session_->cinematic = std::make_unique<NisPresentation>(*this);
        set_process(true);
        return true;
    } catch (const std::exception& error) {
        if (session_) fail(stage, error.what());
        else report_launch_error(stage, frontend ? String("unknown") : String::num_int64(track), error.what());
        return false;
    }
}

void RacePresentation::_input(const Ref<InputEvent>& event) {
    if (failed_ && event->is_action_pressed("hp2_quit")) {
        get_viewport()->set_input_as_handled();
        get_tree()->quit(1);
        return;
    }
#ifdef HP2_MVP_IMGUI
    if (debug_ && debug_->input(event)) {
        get_viewport()->set_input_as_handled();
        return;
    }
#endif
    if (session_) {
        const auto action = session_->host.inRace ? "hp2_pause" : "hp2_quit";
        if (!event->is_action(action)) return;
        get_viewport()->set_input_as_handled();
        if (!session_->host.inRace && event->is_action_pressed(action)) quit_game();
    }
}

void RacePresentation::frontend_pointer_input(const Ref<InputEvent>& event) {
    if (session_ && session_->frontend && !failed_) session_->frontend->pointer_input(event);
}

void RacePresentation::retire_scene() {
    auto& s = *session_;
    if (!s.scene) return;
    s.frontend->update_rear_view({}, {}, false);
    s.cinematic->clear();
    s.effects.reset();
    s.worldObjects.reset();
    s.movablePlacements.clear();
    s.vehicles.clear();
    remove_child(s.scene);
    s.scene->queue_free();
    s.scene = nullptr;
    s.camera = nullptr;
    s.rearViewport = nullptr;
    s.rearCamera = nullptr;
    s.placements = 0;
}

void RacePresentation::_process(double delta) {
#ifdef HP2_MVP_IMGUI
    if (debug_) debug_->frame(delta);
#else
    (void)delta;
#endif
    if (!session_ || failed_) return;
    auto& s = *session_;
    const char* stage = s.host.inRace ? "world_step" : "frontend_flow";
    try {
        if (!s.runtime.AdvanceRace()) {
            quit_game();
            return;
        }
        const auto race = s.runtime.RaceState();
        stage = "race_effects_state";
        s.runtime.TakeRaceEffects(s.effectsFrame);
        const auto state = s.runtime.FrontendState();
        s.raceState = race.sessionState;
        s.host.inRace = race.simulation != nullptr;
        s.host.beTheCop = race.beTheCop;
        s.host.menuInput = state.screen && race.sessionState != RC_STATE_RACE_IN_PROGRESS;
        s.frontendScreen = state.screen ? String(state.screen) : String();
        s.selectedItem = state.selectedItem;
        s.modal = state.pointerModal;
        stage = "frontend_scene";
        s.frontend->update(state, get_viewport()->get_visible_rect().size,s.effectsFrame);
        if (!race.simulation) {
            retire_scene();
            if (!s.frontendMode && !s.bridge.current().generation &&
                std::chrono::steady_clock::now() - s.loadingSince > std::chrono::seconds(45))
                throw std::runtime_error(std::string("World did not start; GameFlow=") +
                    std::to_string(s.runtime.Flow().currentState));
            return;
        }
        stage = "race_state";
        if (race.simulation->world->presentationGeneration != s.bridge.current().generation)
            retire_scene();
        if (!race.camera) return;

#ifdef HP2_MVP_IMGUI
        stage = "race_diagnostics";
        s.diagnosticChecks.advance(*race.simulation, race.track, race.sessionState,
            race.simulation->world->presentationGeneration, race.simulation->world->frameCounter);
#endif
        stage = "entity_state_bridge";
        const auto& frame = s.bridge.stage(race);
        const auto& previous = s.bridge.current();
        const bool replayRestored = frame.replayEpoch != previous.replayEpoch;
        if (frame.generation == previous.generation && frame.step < previous.step && !replayRestored)
            throw NotImplemented("Godot race presentation", "World rewind within a presentation generation");
        const auto& world = *race.simulation->world;
        const bool newScene = !s.scene;
        if (newScene) {
            stage = "track_assets";
            if (!race.scenery) throw std::runtime_error("Committed track has no loaded scenery");
            s.scene = memnew(Node3D);
            add_child(s.scene);
            s.scene->set_name("RaceScene");
            if (!race.simulation->collisions || !race.simulation->collisions->objects)
                throw std::runtime_error("Committed track has no collision placement registry");
            s.placements = attach_track(*s.scene, *race.scenery, race.sky, frame.reverse,
                *race.simulation->collisions->objects, s.movablePlacements);
            s.worldObjects = std::make_unique<WorldObjectPresentation>(*s.scene);
            s.effects = std::make_unique<RaceEffectsPresentation>(*s.scene);
            s.effects->prepare_particles(s.runtime.RaceParticleAssets());
            s.camera = memnew(Camera3D);
            s.scene->add_child(s.camera);
            s.camera->set_cull_mask_value(3, false);
            s.camera->make_current();
        }
        stage = "vehicle_assets";
        for (std::size_t slot = 0; slot < frame.vehicles.size(); ++slot) {
            const auto& state = frame.vehicles[slot];
            auto [it, added] = s.vehicles.try_emplace(state.handle);
            auto& vehicle = it->second;
            if (added || vehicle.model != state.model || vehicle.skin != state.skin) {
                // Traffic can change skin on a live Car. Swap only the Godot
                // presentation and keep the World-issued handle.
                auto* previous = vehicle.root;
                vehicle.attach(*s.scene, *world.carTable[slot], state, race.worldFrames);
                if (previous) {
                    s.scene->remove_child(previous);
                    previous->queue_free();
                }
            }
            vehicle.update(state);
        }
        for (auto it = s.vehicles.begin(); it != s.vehicles.end();) {
            if (std::none_of(frame.vehicles.begin(), frame.vehicles.end(),
                    [&](const VehicleEntityState& state) { return state.handle == it->first; })) {
                s.scene->remove_child(it->second.root);
                it->second.root->queue_free();
                it = s.vehicles.erase(it);
            } else ++it;
        }

        stage = "world_object_assets";
        s.worldObjects->update(frame.worldObjects, race.worldFrames);
        for (const auto& placement : s.movablePlacements)
            placement.node->set_visible((*placement.flags & 0x10u) == 0);

        stage = "nis_presentation";
        if (race.nis) s.cinematic->update(*s.scene, *race.nis, race.blackAndWhite);
        stage = "camera";
        apply_race_camera(*s.camera, frame.camera);
        stage = "race_effects";
        if (!newScene && replayRestored) s.effects->reset();
        stage = "rear_view";
        if (frame.rearCamera) {
            if (!s.rearViewport) {
                s.rearViewport = memnew(SubViewport);
                s.rearViewport->set_name("RearViewTarget");
                s.rearViewport->set_world_3d(get_viewport()->find_world_3d());
                s.scene->add_child(s.rearViewport);
                RenderingServer::get_singleton()->viewport_set_disable_2d(
                    s.rearViewport->get_viewport_rid(), true);
                s.rearCamera = memnew(Camera3D);
                s.rearViewport->add_child(s.rearCamera);
                s.rearCamera->set_cull_mask_value(2, false);
                s.rearCamera->make_current();
            }
            // Target 7 has 256x64 logical pixels and focal scales 1.5x/1.08y.
            // Render square pixels at the matching frustum aspect.
            const auto window = get_viewport()->get_visible_rect().size;
            const float scale = std::min(window.x / 640.0f, window.y / 480.0f);
            const int height = std::max(64, static_cast<int>(std::ceil(64 * scale)));
            const Vector2i size(static_cast<int>(std::round(height * 2.88f)), height);
            if (s.rearViewport->get_size() != size) s.rearViewport->set_size(size);
            s.rearViewport->set_update_mode(SubViewport::UPDATE_WHEN_VISIBLE);
            apply_race_camera(*s.rearCamera, *frame.rearCamera);
            s.frontend->update_rear_view(s.rearViewport->get_texture(), race.rearTextures, race.ntsc);
        } else {
            if (s.rearViewport) s.rearViewport->set_update_mode(SubViewport::UPDATE_DISABLED);
            s.frontend->update_rear_view({}, {}, false);
        }
        stage = "race_effects";
        s.effects->update(s.effectsFrame,*s.camera,frame.rearCamera ? s.rearCamera:nullptr,frame.generation,race.ntsc,race.worldFrames);
        if (newScene)
            std::fprintf(stderr, "HP2_MVP_TRACE stage=scene_publish track=%d generation=%llu step=%u placements=%zu vehicles=%zu reverse=%d game_type=%d event=%d\n",
                frame.track, static_cast<unsigned long long>(frame.generation), frame.step, s.placements,
                s.vehicles.size(), static_cast<int>(frame.reverse), s.runtime.Frontend().gameType,
                s.runtime.Frontend().selectedEventNumber);
        s.bridge.publish();
    } catch (const std::exception& error) {
        fail(stage, error.what());
    }
}

Dictionary RacePresentation::telemetry() const {
    Dictionary result;
    if (!session_) return result;
#ifdef HP2_MVP_IMGUI
    result["debug_console_open"] = debug_ && debug_->captures_game_input();
#endif
    const auto& frame = session_->bridge.current();
    if (session_->frontend) {
        result["game_type"] = session_->runtime.Frontend().gameType;
        result["selected_event"] = session_->runtime.Frontend().selectedEventNumber;
        if (const auto* picker = CurrentTrackSelectScreen(); picker && picker->tracks && picker->tracks->current) {
            result["picker_base_track"] = picker->data.baseTrackNumber;
            result["picker_direction"] = picker->data.direction;
            result["picker_availability"] = picker->tracks->current->availability;
        }
        result["screen"] = session_->frontendScreen;
        result["selected_item"] = session_->selectedItem;
        result["modal"] = session_->modal;
        result["video_playing"] = session_->frontend->video_playing();
        result["video_request"] = session_->frontend->video_request();
        result["loading"] = session_->frontend->showing_loading();
    }
    result["race_state"] = session_->raceState;
    collect_police_diagnostics(result, session_->runtime.RaceState(), session_->runtime.FrontendState());
    if (!session_->scene) return result;
    result["nis_parts"] = static_cast<int64_t>(session_->cinematic->part_count());
    result["generation"] = frame.generation;
    result["track"] = frame.track;
    result["reverse"] = frame.reverse;
    result["step"] = frame.step;
    result["replay_epoch"] = frame.replayEpoch;
    result["placements"] = static_cast<int64_t>(session_->placements);
    result["vehicles"] = static_cast<int64_t>(frame.vehicles.size());
    for (const auto& vehicle : frame.vehicles) {
        if (!vehicle.player) continue;
        result["speed"] = vehicle.speed;
        result["position"] = Vector3(vehicle.transform[12], vehicle.transform[13], vehicle.transform[14]);
        result["forward"] = Vector3(vehicle.transform[0], vehicle.transform[1], vehicle.transform[2]);
    }
    return result;
}

}
