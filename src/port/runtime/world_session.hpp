#pragma once
#include "race_rules_view.hpp"
// TASK-0107. Port-owned source World lifetime/publication. No Godot or PS2 ABI.
#include "game_core/port/driving/session.hpp"
#include "game_core/port/driving/camera.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "event_launch.hpp"
#include "event_attempt.hpp"
#include "game_core/port/race/settlement.hpp"
#include "game_core/port/race/player_hud_gameplay.hpp"
#include "game_core/port/race/hud_geometry.hpp"
#include "game_core/port/race/nis_render_packet.hpp"
#include "game_core/port/race/player_hud_render.hpp"
#include <filesystem>
#include <memory>

namespace port::track { class ParticleSimulation; }
namespace port::race { struct RoadRoutePoint; struct RoadRouteObservation; }
namespace port::race {
struct EventRuntimeContinuation;
struct WorldResearchOperands {
    std::optional<std::uint32_t> construction_w, physics_w, ai_w;
    std::array<std::optional<std::uint32_t>,8> racer_start_w;
    std::optional<std::uint32_t> event_disc_w, terrain_disc_w, camera_shake_w, acid_w;
    std::optional<std::uint32_t> negative_override_route, negative_override_point;
    std::optional<std::uint32_t> cop_teleport_position_w;
    std::optional<std::uint32_t> car_reset_route_newpos_w,car_reset_velocity_w;
    std::optional<std::uint32_t> cop_speedtrap_velocity_w;
    std::optional<std::uint32_t> cop_mover_ground_normal_w;
    std::optional<std::uint32_t> cop_mover_invalid_ground_normal_w,cop_mover_airborne_angular_w;
    std::optional<std::uint32_t> mellow_jump_up_w;
    std::optional<std::uint32_t> pursuit_add_point_w, pursuit_set_point_w;
    std::optional<std::uint32_t> average_air_position_w,average_air_acceleration_w;
    std::optional<std::uint32_t> cop_desired_velocity_w,cop_target_point_w;
    std::optional<std::uint32_t> cop_bump_arm_w;
    std::optional<std::uint32_t> cop_roadblock_velocity_w;
    std::optional<std::uint32_t> nis_placement_route_position_w;
};
struct WorldLaunchRequest {
    std::filesystem::path project_root = ".";
    int track = 11, car_id = -1, skin_id = -1;
    std::string car_code;
    bool reverse = false;
    // Explicit PC presentation/numerical policy with retained console warnings.
    // FreeRun uses the TASK-0111 one-player preset. Authored events retain their
    // original participant producers and rules; only presentation is disabled.
    bool minimal_driving = false; // Native diagnostics retain strict default.
    // Port UI integration selector; native fixtures retain their explicit scope.
    bool original_ui = false;
    // Production authored event; absent retains the independent FreeRun route.
    std::optional<EventLaunchRequest> event;
    std::shared_ptr<const EventRuntimeContinuation> continuation;
    std::optional<PlayerHudCameraContext> hud_camera;
    std::optional<WorldResearchOperands> research_operands;
    // Explicit host test fault, not an original game path or ordinary request.
    std::optional<std::uint64_t> publication_test_fault_after;
};
struct WorldActorFrame {
    bool visible = true;
    std::uint32_t address = 0;
    int driver_number = 0, car_index = 0, car_id = -1, skin_id = 0;
    std::string car_code;
    driving::Snapshot state;
};
struct WorldObjectFrame {
    std::uint32_t address = 0, model_hash = 0, source_instance = 0;
    driving::Matrix transform{};
    bool visible = false;
    int source_section = -1, source_record = -1;
};
struct WorldSessionStats {
    std::size_t regions=0, payload_bytes=0, owned_allocations=0;
    std::uint64_t audio_commands=0;
    bool source_cleanup_ok=false, source_cleanup_attempted=false;
};
struct WorldSceneryFrame {
    int source_section = -1, source_record = -1;
    std::uint16_t flags = 0;
    std::optional<driving::Matrix> transform;
};
struct WorldFrame {
    std::optional<RaceRulesView> rule_view;
    std::uint64_t generation = 0, step = 0;
    // Port publication identity: UI may change while the simulation is frozen.
    std::uint64_t ui_revision = 0;
    std::uint32_t menu_screen = 0;
    std::uint32_t menu_item = 0;
    bool menu_popup = false;
    bool menu_paused = false;
    bool return_requested = false;
    bool autosave_requested = false;
    bool awaiting_attempt=false,attempt_requires_saved=false,continuation_reload=false;
    std::vector<std::pair<std::uint32_t,int>> result_menu_actions;
    int track = 0;
    bool reverse = false;
    std::uint32_t player_car = 0;
    int source_driver_info_count=0; // Original RP.NumDriverInfo (+308), after SetupCars/AddDriverInfo.
    std::optional<driving::DrivingCameraFrame> camera;
    std::optional<driving::DrivingCameraFrame> mirror_camera;
    std::optional<HudMirrorView> mirror_view;
    float mirror_far_z=0;
    std::vector<hp2::frontend::FeScreen> ui_screens;
    std::vector<HudPolygon> hud_geometry;
    // Original Flailer global clip L/T/R/B, copied before frame publication.
    std::array<float,4> ui_clip_bounds{};
    std::vector<std::string> ui_texture_bundles; //Actual selected track texture owners, immutable host resource references.
    float dt = 0;
    std::vector<WorldActorFrame> actors;
    std::vector<WorldObjectFrame> objects;
    std::vector<NisDrawPacket> nis_draws;
    std::vector<float> pcm; // Committed stereo float PCM, explicit PC rate48000.
    std::shared_ptr<const ::port::track::ParticleSimulation> particles;
    std::vector<WorldSceneryFrame> scenery_flags;
    // Port display of committed original race/player state; no host rules.
    int race_stage=0,rank=0,lap=0,lap_limit=0,elapsed_ticks=0,finish_reason=0;
    int player_score=0;
    bool player_finished=false;
    float track_percent=0,race_progress=0;
    std::string elapsed_text;
    std::string event_name;
};
// Optional PC wall-clock observations for one advance call.
struct WorldAdvanceTiming {
    double replay_ms=0;
    double timestep_ms=0;
    double publish_ms=0;
};
class WorldSession {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    WorldSession();
    ~WorldSession();
    WorldSession(const WorldSession&) = delete;
    WorldSession& operator=(const WorldSession&) = delete;
    // Synchronous native construction; the bridge owns asynchronous dispatch.
    void open(const WorldLaunchRequest&);
    void close();
    void set_input(const driving::Input&);
    // Queued host requests; source camera only advances at a World step.
    void camera_input(bool look_back, bool cycle = false);
    void cycle_camera_pov();
    bool advance(float elapsed, WorldAdvanceTiming* timing = nullptr);
    bool single_step(float elapsed = 1.f / 60.f);
    void pause();
    void resume();
    void restart();
    void open_pause(int port=0);
    void menu_input(int event,bool pressed,int port=0);
    bool advance_ui(float elapsed);
    void acknowledge_autosave();
    void acknowledge_event_attempt(const ::port::runtime::EventAttemptBaseline&);
    std::optional<WorldLaunchRequest> take_continuation_launch();
    const WorldFrame& frame() const;
    const WorldLaunchRequest& request() const;
    const std::string& phase() const;
    const std::string& diagnostic() const;
    bool configured() const;
    bool paused() const;
    const driving::Input& controls() const;
    const WorldActorFrame* player() const;
    std::vector<driving::SkidQuad> render_skids(const driving::SkidView&) const;
    WorldSessionStats stats() const;
    std::optional<EventSettlement> event_settlement() const;
    std::optional<SettlementSnapshot> event_results() const;
    // Score-only read-only native observations; no finalizer/finish mutation.
    std::optional<::port::runtime::EventScoreProfile> current_event_scores() const;
    std::optional<std::array<std::int32_t,2>> event_record_times_to_beat() const;
    std::optional<::port::runtime::EventSettingsTransfer> current_event_settings() const;
    std::optional<::port::runtime::EventPlayerSettings> current_player_settings() const;
    std::vector<std::array<float,3>> route_points() const;
    std::vector<RoadRoutePoint> road_points() const;
    RoadRouteObservation road_course() const; // Port-only read-only initial road navigation. // Source XY and distance, read-only test observation.
    std::optional<std::uint32_t> current_random_state() const;
};
}
