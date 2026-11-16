#include "race_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "addon/subtitles/police_subtitles.hpp"
#include "assets/port_credit.hpp"
#include "assets/race_diagnostics.hpp"
#include "bridge/entity_state_bridge.hpp"
#ifdef HP2_MVP_IMGUI
#include "assets/frontend_debug_menu.hpp"
#include "assets/imgui_overlay.hpp"
#include "assets/trackviewer/track_viewer.hpp"
#endif
#include "systems/audio/godot_speech_output.hpp"
#include "systems/audio/godot_audio_device.hpp"
#include "systems/camera/camera_presentation.hpp"
#include "systems/camera/race_post_presentation.hpp"
#include "systems/cinematic/nis_presentation.hpp"
#include "systems/effects/race_effects_presentation.hpp"
#include "systems/frontend/frontend_presentation.hpp"
#include "systems/frontend/graphic_menu.hpp"
#include "systems/track/track_presentation.hpp"
#include "systems/vehicle/vehicle_presentation.hpp"
#include "systems/vehicle/vehicle_reflection.hpp"

#include "bsound/include/bSound.h"
#include "bware/inc/bFile.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/FMenus.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/NIS.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/World.hpp"
#include "indep/src/speech/SEDWedge.hpp"
#include "port/not_implemented.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/compositor.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <array>
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
constexpr std::array<const char*,6> menuActions{
    "hp2_menu_confirm", "hp2_menu_back", "hp2_menu_up", "hp2_menu_down", "hp2_menu_left", "hp2_menu_right"};

class RaceHost final : public NativeApplicationHost {
public:
    explicit RaceHost(Node3D& root) : root_(root) {}

    bool inRace{};
    bool menuInput{};
    bool inRaceMenuBack{};
    bool demoInput{};
    bool beTheCop{};
    bool nativeDebug{};
    bool scrollingCredits{};
    std::array<bool, menuActions.size()> consumedMenuKeys{};
#ifdef HP2_MVP_IMGUI
    ImGuiOverlay* debug{};
#endif
    bool PollEvents() override {
        if (nativeDebug) {
            const auto pads = Input::get_singleton()->get_connected_joypads();
            if (debugPad_ < 0 || !pads.has(debugPad_)) {
                const int next = pads.is_empty() ? -1 : static_cast<int>(pads[0]);
                if (debugPad_ != next) joystickNotifyNativeInputReset(0);
                debugPad_ = next;
            }
        }
        return true;
    }
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
        const auto backend = OS::get_singleton()->get_environment("HP2_MVP_AUDIO_BACKEND");
#ifdef _WIN32
        if (backend == "godot")
            speechOutput_ = std::make_unique<GodotSpeechOutput>(root_);
        if (backend != "godot-device") return sound.OpenWindowsDevice(speechOutput_.get());
#endif
        auto device = std::make_unique<GodotAudioDevice>(root_);
        const auto result = device->Open(sound);
        if (result == WORLD_AUDIO_OK) audioDevice_ = std::move(device);
        return result;
    }
    WorldAudioResult PumpAudio(bSound& sound) override {
        if (audioDevice_) return audioDevice_->Pump();
#ifdef _WIN32
        return sound.PumpWindowsDevice();
#else
        return WORLD_AUDIO_INVALID_BINDING;
#endif
    }
    WorldAudioResult CloseAudio(bSound& sound) override {
        if (audioDevice_) {
            const auto result = audioDevice_->Close();
            audioDevice_.reset();
            return result;
        }
#ifdef _WIN32
        const auto result = sound.CloseWindowsDevice();
        if (result == WORLD_AUDIO_OK) speechOutput_.reset();
        return result;
#else
        return WORLD_AUDIO_INVALID_BINDING;
#endif
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
            const bool pause = inRace && pressed("hp2_pause");
            if (!pause) submenuPauseHeld_ = false;
            else if (inRaceMenuBack) submenuPauseHeld_ = true;
            // Keep a Back press from becoming a fresh Start edge on the parent screen.
            SetButton(pad, Ps2Button::Start, pause && !submenuPauseHeld_);
            if (!inRace || menuInput || demoInput) {
                policeSupportHeld_ = false;
                nitroModifierHeld_ = false;
                constexpr std::array<Ps2Button, menuActions.size()> buttons{
                    Ps2Button::Cross, Ps2Button::Triangle, Ps2Button::Up, Ps2Button::Down,
                    Ps2Button::Left, Ps2Button::Right};
                for (std::size_t i = 0; i < buttons.size(); ++i) {
                    const bool back = i == 1;
                    const bool down = pressed(menuActions[i]) || (back && inRaceMenuBack && pause);
                    if (!down) consumedMenuKeys[i] = false;
                    SetButton(pad, buttons[i], down && !consumedMenuKeys[i] &&
                        (!back || !inRace || demoInput || inRaceMenuBack));
                }
                if (scrollingCredits) SetButton(pad, Ps2Button::Square, pressed("hp2_credits_pause"));
                destination = NativeScannerControls(pad);
                return 1;
            }
            const bool accelerate = pressed("hp2_accelerate");
            const bool brake = pressed("hp2_brake");
            const bool left = pressed("hp2_steer_left");
            const bool right = pressed("hp2_steer_right");
            const bool handbrake = pressed("hp2_handbrake");
            const bool shiftDown = pressed("hp2_l2");
            const bool shiftUp = pressed("hp2_r2");
            const bool roadblock = beTheCop && (pressed("hp2_roadblock") || (handbrake && shiftUp));
            const bool helicopter = beTheCop && (pressed("hp2_helicopter") || (handbrake && shiftDown));
            const bool nitro = pressed("hp2_nitro");
            const bool policeSupport = roadblock || helicopter;
            // PAL scanner requires the Triangle modifier to be held across
            // two samples before the L2/R2 primary edge.
            const bool supportReady = policeSupport && policeSupportHeld_;
            policeSupportHeld_ = policeSupport;
            pad.sticks[2] = left == right ? 128 : left ? 0 : 255;
            SetButton(pad, Ps2Button::Cross, accelerate);
            SetButton(pad, Ps2Button::Square, brake);
            // The default PS2 scanner uses Triangle for camera and R1 for handbrake.
            SetButton(pad, Ps2Button::Triangle, pressed("hp2_camera") || policeSupport || nitro);
            SetButton(pad, Ps2Button::Circle, pressed("hp2_horn"));
            SetButton(pad, Ps2Button::L2, (shiftDown && !helicopter) || (supportReady && helicopter));
            SetButton(pad, Ps2Button::R2, (shiftUp && !roadblock) || (supportReady && roadblock));
            SetButton(pad, Ps2Button::L1, pressed("hp2_l1"));
            SetButton(pad, Ps2Button::R1, handbrake || (nitro && nitroModifierHeld_));
            SetButton(pad, Ps2Button::Select, pressed("hp2_select"));
            SetButton(pad, Ps2Button::R3, nitro);
            if (nativeDebug) {
                if (joystickNativeDebugCaption()) {
                    SetButton(pad, Ps2Button::Up, pressed("hp2_menu_up"));
                    SetButton(pad, Ps2Button::Down, pressed("hp2_menu_down"));
                    SetButton(pad, Ps2Button::Left, pressed("hp2_menu_left"));
                    SetButton(pad, Ps2Button::Right, pressed("hp2_menu_right"));
                }
                read_debug_pad(input, pad);
            }
            nitroModifierHeld_ = nitro;
        }
        destination = NativeScannerControls(pad);
        return pad.connected ? 1 : 0;
    }

private:
    void read_debug_pad(const Input& input, ControllerState& pad) const {
        if (debugPad_ < 0) return;
        constexpr std::pair<JoyButton, Ps2Button> buttons[]{
            {JOY_BUTTON_BACK, Ps2Button::Select}, {JOY_BUTTON_START, Ps2Button::Start},
            {JOY_BUTTON_DPAD_UP, Ps2Button::Up}, {JOY_BUTTON_DPAD_RIGHT, Ps2Button::Right},
            {JOY_BUTTON_DPAD_DOWN, Ps2Button::Down}, {JOY_BUTTON_DPAD_LEFT, Ps2Button::Left},
            {JOY_BUTTON_A, Ps2Button::Cross}, {JOY_BUTTON_B, Ps2Button::Circle},
            {JOY_BUTTON_X, Ps2Button::Square}, {JOY_BUTTON_Y, Ps2Button::Triangle},
            {JOY_BUTTON_LEFT_SHOULDER, Ps2Button::L1}, {JOY_BUTTON_RIGHT_SHOULDER, Ps2Button::R1},
            {JOY_BUTTON_LEFT_STICK, Ps2Button::L3}, {JOY_BUTTON_RIGHT_STICK, Ps2Button::R3},
        };
        for (const auto& [source, target] : buttons)
            if (input.is_joy_button_pressed(debugPad_, source)) SetButton(pad, target, true);
        constexpr std::pair<JoyAxis, Ps2Button> triggers[]{
            {JOY_AXIS_TRIGGER_LEFT, Ps2Button::L2}, {JOY_AXIS_TRIGGER_RIGHT, Ps2Button::R2}};
        for (const auto& [source, target] : triggers) {
            const auto strength = std::clamp(input.get_joy_axis(debugPad_, source), 0.0f, 1.0f);
            if (strength > 0.08f) SetButton(pad, target, true, static_cast<std::uint8_t>(strength * 255));
        }
        constexpr JoyAxis axes[]{JOY_AXIS_RIGHT_X, JOY_AXIS_RIGHT_Y, JOY_AXIS_LEFT_X, JOY_AXIS_LEFT_Y};
        for (std::size_t i = 0; i < pad.sticks.size(); ++i) {
            if (pad.sticks[i] != 128) continue;
            const auto value = std::clamp(input.get_joy_axis(debugPad_, axes[i]), -1.0f, 1.0f);
            if (std::abs(value) > 0.08f)
                pad.sticks[i] = NormalizeAxis(static_cast<std::int16_t>(value * (value < 0 ? 32768 : 32767)));
        }
    }
    int debugPad_{-1};
    bool submenuPauseHeld_{};
    Node3D& root_;
    bool policeSupportHeld_{};
    bool nitroModifierHeld_{};
    std::unique_ptr<GodotSpeechOutput> speechOutput_;
    std::unique_ptr<GodotAudioDevice> audioDevice_;
};
}

struct RacePresentation::Session {
    Session(RacePresentation& owner, const String& assets, const String& profile)
        : host(owner), files(std::filesystem::path(assets.wide_string().get_data())),
          runtime(files, host, std::filesystem::path(profile.wide_string().get_data())) {}

    RaceHost host;
    GameResources files;
    std::unique_ptr<GraphicMenu> graphicMenu;
#ifdef HP2_MVP_IMGUI
    std::unique_ptr<FrontendDebugMenu> debugMenu;
#endif
    ApplicationRuntime runtime;
    EntityStateBridge bridge;
    Node2D* frontendCanvas{};
    Node2D* subtitleCanvas{};
    Label* nativeDebugCaption{};
    Node* frontendRoot{};
    std::unique_ptr<FrontendPresentation> frontend;
    std::unique_ptr<PoliceSubtitles> subtitles;
    std::unique_ptr<NisPresentation> cinematic;
    std::unique_ptr<RaceEffectsPresentation> effects;
    RaceEffectsFrame effectsFrame;
    Vector4 animationClock;
    std::unique_ptr<WorldObjectPresentation> worldObjects;
    std::vector<TrackPlacementVisibility> movablePlacements;
    String frontendScreen;
    std::uint32_t selectedItem{};
    bool modal{};
    std::uint64_t frontendEpoch{}, nameEditSession{};
    bool nameEditInput{}, creatingProfile{};
    int raceState{};
    bool radialBlurActive{};
    bool frontendMode{};
    int requestedTrack{-1};
    CampaignRaceDiagnostic campaignDiagnostic;
#ifdef HP2_MVP_IMGUI
    RaceDiagnosticChecks diagnosticChecks;
#endif
    Node3D* scene{};
    SubViewport* sceneViewport{};
    Dictionary fogProfile;
    Camera3D* camera{};
    RaceOrbitCamera orbit;
    int orbitStockView = -1;
    Ref<RacePostPresentation> post;
    SubViewport* rearViewport{};
    Camera3D* rearCamera{};
    std::unordered_map<std::uint64_t, VehiclePresentation> vehicles;
    std::unique_ptr<VehicleReflection> reflections;
    std::unique_ptr<TrackSceneryLod> primaryScenery;
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
    ClassDB::bind_static_method("RacePresentation", D_METHOD("validate_game_iso", "path"), &RacePresentation::validate_game_iso);
    ClassDB::bind_method(D_METHOD("set_graphics_menu_settings", "settings"), &RacePresentation::set_graphics_menu_settings);
    ClassDB::bind_method(D_METHOD("set_subtitle_setting", "setting"), &RacePresentation::set_subtitle_setting);
    ClassDB::bind_method(D_METHOD("start_game", "assets", "profile", "track", "tree", "event", "native_debug", "isolated_profile"),
        &RacePresentation::start_game);
    ClassDB::bind_method(D_METHOD("telemetry"), &RacePresentation::telemetry);
    ClassDB::bind_method(D_METHOD("race_scene"), &RacePresentation::race_scene);
    ClassDB::bind_method(D_METHOD("photo_route"), &RacePresentation::photo_route);
    ClassDB::bind_method(D_METHOD("test_update_scenery_camera", "enabled"), &RacePresentation::test_update_scenery_camera);
    ClassDB::bind_method(D_METHOD("report_launch_error", "stage", "track", "reason"),
        &RacePresentation::report_launch_error);
}

void RacePresentation::_notification(int what) {
    if (what == NOTIFICATION_WM_CLOSE_REQUEST && failed_) get_tree()->quit(1);
    if (session_ && (what == NOTIFICATION_APPLICATION_FOCUS_OUT ||
        what == NOTIFICATION_WM_WINDOW_FOCUS_OUT || what == NOTIFICATION_EXIT_TREE))
        session_->orbit.capture(false);
    if (what == NOTIFICATION_APPLICATION_FOCUS_OUT && session_ && nativeDebug_)
        joystickNotifyNativeInputReset(0);
    if (what == NOTIFICATION_EXIT_TREE && session_ && session_->post.is_valid())
        session_->post->stop();
}

void RacePresentation::report_assert(const String& message) {
    if (session_) session_->orbit.capture(false);
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
        retire_session();
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

String RacePresentation::validate_game_iso(const String& path) {
    try {
        const std::filesystem::path image(path.wide_string().get_data());
        if (!std::filesystem::is_regular_file(image) || path.get_extension().to_lower() != "iso")
            return "Choose a PS2 ISO file.";
        const GameFileSystem disc(image);
        if (disc.DiscBootExecutable() != "SLUS_203.62" || !disc.Contains("SLUS_203.62"))
            return "This disc does not boot SLUS_203.62. Choose the US PS2 Hot Pursuit 2 ISO.";
        const auto& executable = disc.Get("SLUS_203.62").range;
        if (executable.Size() < 52 || executable.Read(0, 4) != std::vector<std::uint8_t>{0x7f, 'E', 'L', 'F'})
            return "The disc's SLUS_203.62 executable is invalid.";
        const ZDirArchive archive(disc);
        return {};
    } catch (const std::exception& error) {
        return String("Cannot read the game ISO. ") + String::utf8(error.what());
    }
}

void RacePresentation::set_graphics_menu_settings(const Callable& settings) {
    if (!session_) graphicsMenuSettings_ = settings;
}

void RacePresentation::set_subtitle_setting(const Callable& setting) {
    if (!session_) subtitleSetting_ = setting;
}

String RacePresentation::start_game(const String& assets, const String& profile, int track, int tree,
    int event, bool nativeDebug, bool isolatedProfile) {
    if (!session_) {
        nativeDebug_ = nativeDebug;
        isolatedProfile_ = isolatedProfile;
    }
    String error;
    start(assets, profile, track < 0 && tree < 0, track, tree, event, false, &error);
    return error;
}

bool RacePresentation::start(const String& assets, const String& profile, bool frontend, int track, int tree,
    int event, bool returnToMenu, String* launchError) {
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
        session_->host.nativeDebug = nativeDebug_;
        bool unlockRaces = false;
        if (session_->files.IsLooseArchive()) {
            for (const char* bank : {"SOUND/common.msh", "SOUND/common.msb"}) {
                if (!session_->files.TryOpenPhysical(bank))
                    throw std::runtime_error(std::string("Incomplete extracted game source, missing ") + bank +
                        " in " + session_->files.SourcePath().u8string() +
                        ". Set mvp_root in config/ps2_assets.local.json to a complete PS2 disc image or archive.");
            }
        }
#ifdef HP2_MVP_IMGUI
        if (!debug_) debug_ = std::make_unique<ImGuiOverlay>(*this);
        if (!viewer_) {
            assets_ = assets;
            profile_ = profile;
            viewer_ = std::make_unique<TrackViewer>(*this);
            debug_->attach_track_viewer(*viewer_, [this] { open_track_viewer(); });
        }
        session_->host.debug = debug_.get();
        session_->diagnosticChecks.finishForMenuTest = !frontend && track >= 0 &&
            OS::get_singleton()->get_environment("HP2_MVP_TEST_RESULTS_MENU") == "1" &&
            OS::get_singleton()->get_environment("HP2_MVP_TEST_PROFILE") == "1";
        session_->diagnosticChecks.pursuitDriver = OS::get_singleton()->get_environment("HP2_MVP_PURSUIT_AI") == "1";
        session_->diagnosticChecks.supportVisuals = tree >= 0 &&
            OS::get_singleton()->get_environment("HP2_MVP_TEST_SUPPORT_VISUALS") == "1";
        if (OS::get_singleton()->get_environment("HP2_MVP_TEST_SUBTITLE_RADIO") == "1")
            session_->diagnosticChecks.subtitleRadioFixture = 120;
        session_->diagnosticChecks.trackEffectsCamera = !frontend &&
            OS::get_singleton()->get_environment("HP2_MVP_TEST_TRACK_EFFECTS") == "1";
        session_->diagnosticChecks.fireTruckCamera = !frontend &&
            OS::get_singleton()->get_environment("HP2_MVP_TEST_TRACK_EFFECTS") == "firetruck";
        session_->diagnosticChecks.arrestCinematic = OS::get_singleton()->get_environment("HP2_MVP_TEST_ARREST_NIS") == "1";
        unlockRaces = OS::get_singleton()->get_environment("HP2_MVP_TEST_UNLOCK_RACES") == "1";
#else
        (void)returnToMenu;
#endif
        configure_race_diagnostics(nativeDebug_, unlockRaces, isolatedProfile_);
        set_process_input(true);
        stage = frontend ? "frontend_initialize" : "race_initialize";
        if (subtitleSetting_.is_valid()) session_->runtime.BindSubtitleSetting([](void* context, int action) {
            return bool(static_cast<const Callable*>(context)->call(action));
        }, &subtitleSetting_);
        if (frontend) {
            const bool requireNamedProfile = OS::get_singleton()->get_environment("HP2_MVP_TEST_PROFILE") != "1";
            if (graphicsMenuSettings_.is_valid())
                session_->graphicMenu = std::make_unique<GraphicMenu>(graphicsMenuSettings_);
            const auto* graphics = session_->graphicMenu ? &session_->graphicMenu->extension() : nullptr;
#ifdef HP2_MVP_IMGUI
            const String mode = OS::get_singleton()->get_environment("HP2_MVP_DEBUG_MODE");
            if (nativeDebug_ || mode == "1" || (mode.is_empty() &&
                    bool(ProjectSettings::get_singleton()->get_setting("hp2/debug_mode", false)))) {
                session_->debugMenu = std::make_unique<FrontendDebugMenu>([this] { open_track_viewer(); });
            }
            session_->runtime.InitializeFrontend(returnToMenu ? TrackViewer::return_to_main_menu : race_profile_frontend_ready(),
                nullptr, session_->debugMenu ? &session_->debugMenu->extension() : nullptr, &portCredit, graphics,
                requireNamedProfile);
#else
            session_->runtime.InitializeFrontend(race_profile_frontend_ready(), nullptr, nullptr, &portCredit, graphics,
                requireNamedProfile);
#endif
        }
        else if (tree >= 0) session_->runtime.InitializeRace(start_campaign_race_diagnostic, &session_->campaignDiagnostic);
        else session_->runtime.InitializeRace(start_quick_race_diagnostic, &session_->requestedTrack);
        session_->frontendCanvas = memnew(Node2D);
        session_->frontendCanvas->set_name("FrontendCanvas");
        add_child(session_->frontendCanvas);
        if (nativeDebug_) {
            auto* caption = memnew(Label);
            caption->set_name("NativeDebugCameraCaption");
            caption->set_position({16, 34});
            caption->set_z_index(35);
            caption->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
            caption->add_theme_constant_override("outline_size", 4);
            caption->hide();
            session_->frontendCanvas->add_child(caption);
            session_->nativeDebugCaption = caption;
        }
        session_->frontend = std::make_unique<FrontendPresentation>(*session_->frontendCanvas, session_->files,
            session_->runtime, *this);
        auto* subtitleCanvas = memnew(Node2D);
        session_->subtitleCanvas = subtitleCanvas;
        subtitleCanvas->set_name("PoliceSubtitles");
        subtitleCanvas->set_z_index(40);
        add_child(subtitleCanvas);
        session_->subtitles = std::make_unique<PoliceSubtitles>(*subtitleCanvas, session_->files);
        session_->frontendRoot = memnew(Node);
        add_child(session_->frontendRoot);
        session_->cinematic = std::make_unique<NisPresentation>(*session_->frontendRoot);
        set_process(true);
        return true;
    } catch (const std::exception& error) {
        if (launchError) *launchError = String::utf8(error.what());
        else if (session_) fail(stage, error.what());
        else report_launch_error(stage, frontend ? String("unknown") : String::num_int64(track), error.what());
        return false;
    }
}

void RacePresentation::_input(const Ref<InputEvent>& event) {
    const Ref<InputEventKey> key = event;
    if (session_ && key.is_valid() && !key->is_pressed()) {
        for (std::size_t i = 0; i < menuActions.size(); ++i)
            if (key->is_action(menuActions[i])) session_->host.consumedMenuKeys[i] = false;
    }
#ifdef HP2_MVP_IMGUI
    if (debug_ && debug_->input(event)) {
        if (session_ && debug_->needs_mouse_cursor()) session_->orbit.capture(false);
        get_viewport()->set_input_as_handled();
        return;
    }
    if (viewer_ && viewer_->input(event)) {
        get_viewport()->set_input_as_handled();
        return;
    }
#endif
    if (session_) {
        auto& s = *session_;
        if (event->is_action_pressed("hp2_camera")) s.orbit.reset();
        if (event->is_action_pressed("hp2_pause")) s.orbit.capture(false);
        if (const Ref<InputEventMouseMotion> motion = event; motion.is_valid() &&
            s.orbit.mouse_motion(motion->get_screen_relative())) {
            get_viewport()->set_input_as_handled();
            return;
        }
        if (!failed_ && key.is_valid() && s.nameEditSession) {
            using Action = FrontendNameEditAction;
            constexpr std::array<std::pair<const char*, Action>,5> editActions{{
                {"hp2_name_edit_left", Action::Left}, {"hp2_name_edit_right", Action::Right},
                {"hp2_name_edit_backspace", Action::Backspace}, {"hp2_name_edit_delete", Action::Delete},
                {"hp2_name_edit_commit", Action::Commit}}};
            const auto binding = std::find_if(editActions.begin(), editActions.end(),
                [&](const auto& entry) { return key->is_action(entry.first); });
            const auto character = key->get_unicode();
            const bool typed = character >= 32 && !key->is_ctrl_pressed() &&
                !key->is_alt_pressed() && !key->is_meta_pressed();
            if (s.nameEditInput && (binding != editActions.end() || typed)) {
                if (key->is_pressed()) {
                    for (std::size_t i = 0; i < menuActions.size(); ++i)
                        if (key->is_action(menuActions[i])) s.host.consumedMenuKeys[i] = true;
                    auto action = binding != editActions.end() ? binding->second : Action::Insert;
                    if (action == Action::Commit && s.creatingProfile &&
                        (key->get_physical_keycode() == KEY_ESCAPE || key->get_keycode() == KEY_ESCAPE))
                        action = Action::Cancel;
                    if ((action != Action::Commit && action != Action::Cancel) || !key->is_echo())
                        s.runtime.QueueFrontendNameEdit(s.frontendEpoch, s.nameEditSession, action, character);
                }
                get_viewport()->set_input_as_handled();
                return;
            }
        }
        if (s.host.inRace && event->is_action("hp2_pause"))
            get_viewport()->set_input_as_handled();
    }
}

void RacePresentation::frontend_pointer_input(const Ref<InputEvent>& event) {
    if (session_ && session_->frontend && !failed_) session_->frontend->pointer_input(event);
}

void RacePresentation::retire_scene() {
    auto& s = *session_;
    s.orbit.capture(false);
    s.radialBlurActive = false;
    if (!s.scene) return;
    s.frontend->update_rear_view({}, {}, false);
    s.cinematic->clear();
    s.effects.reset();
    s.worldObjects.reset();
    s.movablePlacements.clear();
    s.vehicles.clear();
    s.reflections.reset();
    s.primaryScenery.reset();
    if (s.sceneViewport) {
        if (s.post.is_valid()) {
            s.post->detach();
            s.post.unref();
        }
        auto* parent = s.sceneViewport->get_parent();
        parent->remove_child(s.sceneViewport);
        s.sceneViewport->queue_free();
    } else {
        remove_child(s.scene);
        s.scene->queue_free();
    }
    s.sceneViewport = nullptr;
    s.scene = nullptr;
    s.camera = nullptr;
    s.rearViewport = nullptr;
    s.rearCamera = nullptr;
    s.placements = 0;
}

void RacePresentation::retire_session() {
    if (!session_) return;
    retire_scene();
    auto old = std::move(session_);
    old->frontend.reset();
    old->subtitles.reset();
    old->cinematic.reset();
    for (auto* node : {static_cast<Node*>(old->frontendCanvas), static_cast<Node*>(old->subtitleCanvas), old->frontendRoot}) {
        if (!node) continue;
        remove_child(node);
        node->queue_free();
    }
    old->runtime.Shutdown();
}

#ifdef HP2_MVP_IMGUI
void RacePresentation::open_track_viewer() {
    if (failed_ || !session_) return;
    const auto race = session_->runtime.RaceState();
    viewer_->open(!session_->frontendMode || race.simulation || session_->runtime.Flow().requestedState == 2,
        race.track, session_->runtime.Frontend().currentLanguage);
}

void RacePresentation::service_track_viewer(double delta) {
    if (!viewer_ || failed_) return;
    const auto request = viewer_->take_request();
    try {
        switch (request.action) {
        case TrackViewer::Action::Open:
            viewer_->show();
            retire_session();
            [[fallthrough]];
        case TrackViewer::Action::Load:
            viewer_->detach();
            if (!viewerFiles_)
                viewerFiles_ = std::make_unique<GameResources>(std::filesystem::path(assets_.wide_string().get_data()));
            viewer_->begin_load(std::max(0, request.track), *viewerFiles_);
            break;
        case TrackViewer::Action::Close:
            viewer_->detach();
            viewer_->close();
            viewerFiles_.reset();
            frontendLaunchType = 1;
            start(assets_, profile_, true, -1, -1, 0, true);
            break;
        case TrackViewer::Action::None:
            break;
        }
        viewer_->advance(delta, debug_->captures_game_input());
        if (viewer_->active()) viewer_->update_scene(delta, debug_->effect_options());
    } catch (const std::exception& error) {
        viewer_->detach();
        viewer_->error(error.what());
        debug_->log("trackviewer", error.what(), true);
    }
}
#endif

void RacePresentation::_process(double delta) {
#ifdef HP2_MVP_IMGUI
    if (debug_) debug_->frame(delta, !session_ || !session_->debugMenu || !session_->debugMenu->demoSceneOpen());
    service_track_viewer(delta);
#else
    (void)delta;
#endif
    if (!session_ || failed_) return;
    auto& s = *session_;
    const char* stage = s.host.inRace ? "world_step" : "frontend_flow";
    try {
#ifdef HP2_MVP_IMGUI
        if (s.diagnosticChecks.finish_for_menu_test(s.runtime.RaceState())) stage = "results_menu_fixture";
#endif
        if (!s.runtime.AdvanceRace()) {
            quit_game();
            return;
        }
        const auto race = s.runtime.RaceState();
        const auto& clock = s.runtime.Clock();
        // One shared clock upload feeds the GPU across the race, rear view and preview.
        const float fraction = s.runtime.PresentationStepFraction();
        const float poseFraction = clock.worldTimeElapsed > 0.0f ? fraction : 1.0f;
        const float worldOffset = clock.worldTimeElapsed * fraction;
        s.animationClock = Vector4(clock.worldTimeFrames + worldOffset * 60.0f,
            clock.worldTimer * 0.0002500000118743628f + worldOffset,
            clock.realTimeFrames, clock.worldTimeFrames);
        RenderingServer::get_singleton()->global_shader_parameter_set("hp2_animation_clock", s.animationClock);
        stage = "race_effects_state";
        s.runtime.TakeRaceEffects(s.effectsFrame);
        const auto state = s.runtime.FrontendState();
        s.frontendEpoch = state.pointerEpoch;
        s.nameEditSession = state.nameEditSession;
        s.nameEditInput = state.nameEditInput;
        s.creatingProfile = state.creatingProfile;
        s.raceState = race.sessionState;
        s.host.inRace = race.simulation != nullptr;
        s.host.demoInput = race.sessionState == RC_STATE_AI_DEMO || race.sessionState == RC_STATE_REPLAY_DEMO;
        s.host.beTheCop = race.beTheCop;
        s.host.menuInput = state.screen && race.sessionState != RC_STATE_RACE_IN_PROGRESS;
        // Race information pages and pause submenus use the source LeaveScreen input.
        // Open popups use it to cancel. The root Pause Menu and replay use Start.
        s.host.inRaceMenuBack = s.host.menuInput && !state.pointerReplay &&
            (race.sessionState == RC_STATE_CAMERA_FLYBY || race.sessionState == RC_STATE_PRE_RACE ||
             race.sessionState == RC_STATE_POST_RACE ||
             (race.sessionState == RC_STATE_PAUSE &&
              (state.pointerModal || std::string_view(state.screen) != "Pause Menu")));
        s.frontendScreen = state.screen ? String(state.screen) : String();
        s.host.scrollingCredits = s.frontendScreen == "EACredits" || s.frontendScreen == "BBGCredits" ||
            s.frontendScreen == "Port Credits";
        s.selectedItem = state.selectedItem;
        s.modal = state.pointerModal;
        stage = "frontend_scene";
        s.frontend->update(state, get_viewport()->get_visible_rect().size,s.effectsFrame);
        const auto* subtitleWorld = race.simulation ? race.simulation->world : nullptr;
        bool cinematicActive = false;
        for (auto* manager = race.nis ? race.nis->first : nullptr; manager; manager = manager->Next())
            cinematicActive |= manager->Playback().active != 0;
        s.subtitles->update(subtitleSetting_.is_valid() && bool(subtitleSetting_.call(-1)),
            race.sessionState == RC_STATE_RACE_IN_PROGRESS && subtitleWorld && !s.host.demoInput && !cinematicActive,
            race.sessionState == RC_STATE_PAUSE,
            subtitleWorld ? subtitleWorld->presentationGeneration : 0,
            subtitleWorld ? subtitleWorld->presentationReplayEpoch : 0,
            s.runtime.SpeechCaption(), s.runtime.PresentationResources().Flailer(),
            *state.layout, get_viewport()->get_visible_rect().size);
        if (auto* caption = s.nativeDebugCaption) {
            const auto* text = race.simulation && !s.host.menuInput && !s.host.demoInput
                ? joystickNativeDebugCaption() : nullptr;
            if (text) caption->set_text(text);
            caption->set_visible(text != nullptr);
        }
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
        if (!race.camera) {
            s.orbit.capture(false);
            return;
        }

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
        if (newScene) RenderingServer::get_singleton()->global_shader_parameter_set(
            "hp2_track_focal_scale", race.ntsc ? 224.0f / 1.0714286566f : 256.0f / 1.285f);
        if (newScene) {
            stage = "track_assets";
            if (!race.scenery) throw std::runtime_error("Committed track has no loaded scenery");
            s.scene = memnew(Node3D);
            if (RenderingServer::get_singleton()->get_rendering_device()) {
                s.sceneViewport = RacePostPresentation::create_scene(*get_viewport());
                s.sceneViewport->add_child(s.scene);
            } else {
                add_child(s.scene);
            }
            s.scene->set_name("RaceScene");
            if (!race.simulation->collisions || !race.simulation->collisions->objects)
                throw std::runtime_error("Committed track has no collision placement registry");
            s.worldObjects = std::make_unique<WorldObjectPresentation>(*s.scene);
            const bool sharedWorldReflection = profile_shared_world_reflections();
            s.reflections = std::make_unique<VehicleReflection>(*s.scene, race, sharedWorldReflection);
            if (!profile_legacy_primary_scenery())
                s.primaryScenery = std::make_unique<TrackSceneryLod>(*s.scene, TrackSceneryLod::View::Race,
                    !profile_baseline_scenery_lod());
            s.placements = attach_track(*s.scene, *race.scenery, race.sky, frame.reverse,
                *race.simulation->collisions->objects, s.movablePlacements, *s.worldObjects,
                s.reflections->track(), sharedWorldReflection, s.primaryScenery.get());
            s.effects = std::make_unique<RaceEffectsPresentation>(*s.scene);
            s.effects->prepare_track_coronas(stage_track_coronas(*race.renderResources), race.ntsc, true);
            s.effects->prepare_particles(s.runtime.RaceParticleAssets());
            s.camera = memnew(Camera3D);
            s.scene->add_child(s.camera);
            s.camera->set_cull_mask_value(3, false);
            s.camera->set_cull_mask(s.camera->get_cull_mask() & ~(trackReflectionLayer | trackMirrorSceneryLayer));
            s.camera->make_current();
            s.camera->set_name("RaceCamera");
            if (s.sceneViewport) {
                s.post.instantiate();
                s.post->attach(*s.sceneViewport, *get_viewport(), *s.camera);
            }
        }
        stage = "camera";
        apply_race_camera(*s.camera, frame.camera);
        const auto playerState = std::find_if(frame.vehicles.begin(), frame.vehicles.end(),
            [](const auto& vehicle) { return vehicle.player; });
        const auto playerVisual = playerState != frame.vehicles.end() ?
            s.vehicles.find(playerState->handle) : s.vehicles.end();
        const auto* players = race.simulation->players;
        const auto* player = players ? players->playersByNumber[0] : nullptr;
        const auto* movement = player ? player->car : nullptr;
        const auto* terrain = movement && movement->bindings.environment ?
            movement->bindings.environment->contact.terrain : nullptr;
        const int stockView = player ? player->settings.cameraView : -1;
        bool canOrbit = race.sessionState == RC_STATE_RACE_IN_PROGRESS && !s.host.menuInput && !cinematicActive &&
            !state.pointerModal && playerVisual != s.vehicles.end() && playerState->exteriorVisible &&
            playerVisual->second.model == playerState->model && playerVisual->second.skin == playerState->skin &&
            terrain && terrain->topology && !player->lookBack && get_window()->has_focus() &&
            !Input::get_singleton()->is_action_pressed("hp2_pause") &&
            !Input::get_singleton()->is_action_pressed("hp2_l1");
#ifdef HP2_MVP_IMGUI
        canOrbit = canOrbit && (!debug_ || !debug_->needs_mouse_cursor());
#endif
        if (replayRestored || stockView != s.orbitStockView) s.orbit.reset();
        s.orbitStockView = stockView;
        s.orbit.capture(canOrbit);
        const bool orbitView = canOrbit && s.orbit.apply(*s.camera, *playerState,
            playerVisual->second.bodyBounds, terrain->topology, delta);
        const auto cameraEye = s.camera->get_global_position();
        const std::array<float, 3> renderEye{cameraEye.x, -cameraEye.z, cameraEye.y};
        auto driverCamera = *race.camera;
        if (orbitView) {
            const auto forward = -s.camera->get_global_basis().get_column(2);
            std::copy(renderEye.begin(), renderEye.end(), driverCamera.eye);
            driverCamera.forward[0] = forward.x;
            driverCamera.forward[1] = -forward.z;
            driverCamera.forward[2] = forward.y;
        }
        stage = "vehicle_assets";
        s.reflections->update(race, delta, newScene || replayRestored);
        for (std::size_t slot = 0; slot < frame.vehicles.size(); ++slot) {
            const auto& state = frame.vehicles[slot];
            auto [it, added] = s.vehicles.try_emplace(state.handle);
            auto& vehicle = it->second;
            if (added || vehicle.model != state.model || vehicle.skin != state.skin) {
                // Traffic can change skin on a live Car. Swap only the Godot
                // presentation and keep the World-issued handle.
                auto* previous = vehicle.root;
                vehicle.attach(*s.scene, *world.carTable[slot], state, race.worldFrames, s.reflections->assets());
                if (previous) {
                    s.scene->remove_child(previous);
                    previous->queue_free();
                }
            }
            const bool showExterior = state.visible || (orbitView && s.orbit.player_exterior() && state.player);
            vehicle.update(state, race.renderResources->Lights(), s.reflections->matrix(), race, showExterior);
            // World::frameCounter also advances during AI collision searches.
            // Driver poses follow the source clock's actual simulation ticks.
            vehicle.update_driver(*world.carTable[slot], state, driverCamera,
                static_cast<std::uint32_t>(clock.worldLoopCounter), frame.replayEpoch, poseFraction);
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
        s.worldObjects->update(frame.worldObjects);
        for (const auto& placement : s.movablePlacements)
            placement.node->set_visible((*placement.flags & 0x10u) == 0);

        stage = "nis_presentation";
        RaceEffectOptions effectOptions;
#ifdef HP2_MVP_IMGUI
        if (debug_) effectOptions = debug_->effect_options();
#endif
        effectOptions.particles &= !profile_grid_effect_disabled("no-particles");
        effectOptions.sunGlare &= !profile_grid_effect_disabled("no-sun");
        if (race.nis) s.cinematic->update(*s.scene, *race.nis,
            s.post.is_null() && effectOptions.monochrome && race.blackAndWhite, frame.step, frame.replayEpoch,
            worldOffset * 60.0f);
        stage = "camera";
        if (s.primaryScenery) {
            const float focal = 256.0f / std::tan(race.camera->fieldOfView * (3.141592654f / 65536.0f));
            s.primaryScenery->update_camera(*s.camera, orbitView ? renderEye.data() : frame.camera.eye.data(), focal,
                effectOptions.sceneryVisibility, newScene || replayRestored);
        }
        if (s.post.is_valid()) {
            auto post = s.runtime.RacePost();
            // Port presentation for the attract demo. Keep any authored
            // replay or camera trigger authoritative when it supplies a mask.
            if (s.host.demoInput && post.radialMask && !post.radialActive) {
                post.radialActive = true;
                post.radialAmounts = {0, 160}; // RadialBlurOn's default envelope.
            }
            profile_fog_state(post, effectOptions, s.fogProfile);
            s.radialBlurActive = effectOptions.motionBlur && post.radialActive;
            s.post->update(post, newScene || replayRestored, effectOptions, race.ntsc);
        }
        stage = "race_effects";
        if (!newScene && replayRestored) s.effects->reset();
        stage = "rear_view";
        if (frame.rearCamera && !s.host.demoInput) {
            if (!s.rearViewport) {
                s.rearViewport = memnew(SubViewport);
                s.rearViewport->set_name("RearViewTarget");
                s.rearViewport->set_world_3d(s.camera->get_world_3d());
                s.scene->add_child(s.rearViewport);
                RenderingServer::get_singleton()->viewport_set_disable_2d(
                    s.rearViewport->get_viewport_rid(), true);
                s.rearCamera = memnew(Camera3D);
                s.rearViewport->add_child(s.rearCamera);
                s.rearCamera->set_cull_mask_value(2, false);
                s.rearCamera->set_cull_mask(s.rearCamera->get_cull_mask() & ~(trackReflectionLayer | trackPrimarySceneryLayer));
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
        s.effects->update(s.effectsFrame, *s.camera, frame.rearCamera && !s.host.demoInput ? s.rearCamera : nullptr,
            frame.generation, race.ntsc, race.worldFrames, effectOptions.particles, effectOptions.sunGlare);
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

Node3D* RacePresentation::race_scene() const {
    return session_ ? session_->scene : nullptr;
}

PackedVector3Array RacePresentation::photo_route() const {
    return session_ ? race_photo_route(session_->runtime.RaceState()) : PackedVector3Array();
}

void RacePresentation::test_update_scenery_camera(bool enabled) {
    if (!test_scenery_camera_enabled() || !session_ || !session_->primaryScenery || !session_->camera) return;
    auto& camera = *session_->camera;
    const auto position = camera.get_global_position();
    const float eye[]{position.x, -position.z, position.y};
    const auto heightFocal = 256.0f / std::tan(camera.get_fov() * (3.141592654f / 360.0f));
    const float ratio = session_->runtime.RaceState().ntsc ? 448.0f / (512.0f * 1.0714286566f) : 1.0f / 1.285f;
    session_->primaryScenery->update_camera(camera, eye, heightFocal * ratio, enabled, false);
    if (session_->effects) session_->effects->update_water_sky(camera);
}

Dictionary RacePresentation::telemetry() const {
    Dictionary result;
#ifdef HP2_MVP_IMGUI
    result["debug_console_open"] = debug_ && debug_->captures_game_input();
    result["roadshine_enabled"] = !debug_ || debug_->effect_options().roadshine;
    if (viewer_ && viewer_->modal()) result["viewer_confirm"] = true;
    if (viewer_ && viewer_->active()) {
        result["track_viewer"] = true;
        result["viewer_track"] = viewer_->track();
        result["viewer_catalogue_size"] = viewer_->catalogue_size();
        result["viewer_entities"] = viewer_->entity_count();
        result["track"] = viewer_->track();
        result["generation"] = viewer_->generation();
        result["step"] = viewer_->step();
        result["placements"] = viewer_->placement_count();
        result["scenery_sections"] = static_cast<int64_t>(viewer_->scenery_section_count());
        result["scenery_zones_enabled"] = debug_ && debug_->effect_options().sceneryVisibility;
        result["vehicles"] = 0;
        result["reverse"] = false;
        return result;
    }
#endif
    if (!session_) return result;
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
        result["frontend_loop"] = session_->runtime.Clock().realLoopCounter;
        result["selected_item"] = session_->selectedItem;
        result["name_editing"] = session_->nameEditSession != 0;
        result["creating_profile"] = session_->creatingProfile;
        result["modal"] = session_->modal;
        result["video_playing"] = session_->frontend->video_playing();
        result["video_request"] = session_->frontend->video_request();
        result["loading"] = session_->frontend->showing_loading();
    }
    result["race_state"] = session_->raceState;
    if (session_->subtitles) {
        std::u32string text;
        for (unsigned char code : session_->subtitles->visible_text()) text += code;
        result["police_subtitle"] = String(text.c_str());
#ifdef HP2_MVP_IMGUI
        const auto& speech = session_->runtime.SpeechCaption();
        result["subtitle_source_verified"] = session_->subtitles->verified_source();
        result["speech_caption_sequence"] = speech.sequence;
        result["speech_caption_finished"] = speech.finished != 0;
        if (speech.count) {
            result["speech_caption_event"] = speech.events[0];
            result["speech_caption_offset"] = speech.offsets[0];
            Array fragments;
            for (unsigned i = 0; i < speech.count; ++i)
                fragments.push_back(Vector2i(speech.events[i], speech.offsets[i]));
            result["speech_caption_parts"] = fragments;
        }
#endif
    }
    result["radial_blur_active"] = session_->radialBlurActive;
    result["orbit_active"] = session_->orbit.active();
    result["orbit_terrain_queries"] = session_->orbit.terrain_queries();
    result["animation_clock"] = session_->animationClock;
    if (!session_->fogProfile.is_empty()) result["fog_profile"] = session_->fogProfile;
    const auto frontend = session_->runtime.FrontendState();
    collect_police_diagnostics(result, session_->runtime.RaceState(), frontend);
    collect_menu_alignment_diagnostics(result, frontend);
    if (!session_->scene) return result;
    result["stock_camera_eye"] = race_vector(frame.camera.eye.data());
    result["stock_camera_forward"] = race_vector(frame.camera.forward.data());
    result["nis_parts"] = static_cast<int64_t>(session_->cinematic->part_count());
    result["generation"] = frame.generation;
    result["track"] = frame.track;
    result["reverse"] = frame.reverse;
    result["step"] = frame.step;
    result["replay_epoch"] = frame.replayEpoch;
    result["placements"] = static_cast<int64_t>(session_->placements);
    if (session_->primaryScenery) {
        const auto& stats = session_->primaryScenery->statistics();
        result["scenery_sections"] = static_cast<int64_t>(stats.sections);
        result["scenery_tree_nodes"] = static_cast<int64_t>(stats.treeNodes);
        result["scenery_candidates"] = static_cast<int64_t>(stats.candidates);
        result["scenery_selected"] = static_cast<int64_t>(stats.placements);
        result["scenery_batch_updates"] = static_cast<int64_t>(stats.batchUpdates);
        result["scenery_fallback"] = stats.fallback;
    }
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
