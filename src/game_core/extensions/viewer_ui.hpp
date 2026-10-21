#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <map>
#include <string>
#include <vector>
namespace port::extensions {
// Port-side ImGui model. UI consumes snapshots and emits deferred actions only.
struct UiSkin {int id=-1;std::string name;std::array<float,3> preview{{.35f,.35f,.35f}};bool preview_ready=false;std::string preview_error;};
struct UiCar {int id=-1;std::string code,path,error;bool available=false;std::vector<UiSkin> skins;};
struct UiTrack {int id=-1;std::string name,location,path,error;bool available=false;};
struct UiTrackMaterial {int id=0;std::string name;std::array<float,4> color{{1,1,1,1}};int polygon_count=0;};
struct UiEffect {std::string id,name,category,reason;bool enabled=false,available=false,active=false;};
struct UiPart {std::string name;bool enabled=true;};
struct UiDrivingMaterial {int id=1;std::string name;float static_friction=0,dynamic_friction=0,rolling_friction=0;};
struct UiDrivingGrip {
    bool known=false,slipping=false,contact_known=false,contact=false;int terrain=1;
    float z_force=0,current_slip=0,traction_usage=0,traction_boost=0,catchup_friction=0,road_noise=0,static_coefficient=0,dynamic_coefficient=0;
    std::array<float,2> slip{},ground_force{};
    std::array<float,3> ground_point{};std::array<float,4> ground_plane{};
};
struct ViewerUiState {
    std::string kind="frontend",source,status,diagnostic,rendering,camera;
    bool camera_known=false;
    bool render_statistics_enabled=false;
    std::vector<std::string> render_statistics;
    std::int64_t rendered_primitives=-1; // Godot triangle/line/point primitives, last rendered frame.
    std::int64_t draw_calls=-1; // Godot total for the last rendered frame.
    std::array<float,3> camera_host{}; // Port Godot XYZ; display also labels native XYZ.
    std::string driving_telemetry,track_name;
    bool real_track=false;int track_id=-1;std::vector<UiTrack> tracks;
    bool render_collision=false,render_material=false,show_lod=false,debug_ready=false;
    std::string debug_diagnostic;
    std::vector<UiEffect> effects;
    bool render_ready=false,distance_override=false,render_props=true;
    int loaded_instances=0,considered_instances=0,visible_instances=0,loaded_sections=0,selected_sections=0;
    float draw_distance=10000,effective_near=.5f,effective_far=10000,original_far=10000;
    double native_frame_ms=0,host_submit_ms=0;
    std::vector<UiTrackMaterial> material_legend;
    std::vector<std::string> driving_hints;
    bool driving_recording=false;
    bool driving_powertrain_known=false,driving_handbrake=false;
    int driving_gear=1,driving_transmission=0;
    float driving_speed=0,driving_rpm=0;
    bool driving_surface_known=false;int driving_surface_mode=0,driving_surface_terrain=1;
    std::vector<UiDrivingMaterial> driving_materials;
    std::array<UiDrivingGrip,4> driving_grip{};
    int car_id=-1,skin_id=-1,lod=-1;
    float wheel_speed=80;
    bool wireframe=false,bounds=false,inspection=false,show_lods=false,preview=false,wheel_blur=false;
    std::vector<UiCar> cars;
    std::vector<UiPart> parts;
};
struct UiAction {std::string action,source,name;int car_id=-1,skin_id=-1,lod=-1;bool enabled=false;double value=0;};
class ViewerUi {
    ViewerUiState state_;
    std::map<std::string,std::array<float,2>> targets_;
    bool advanced_open_=false;
    void draw_render_controls();
    bool was_preview_=false;
    bool driving_entered_=false;
    bool driving_left_=false;
    std::deque<UiAction> pending_;
    char source_[512]{},filter_[128]{};
public:
    void update(ViewerUiState state);
    void draw(float width,float height);
    void enqueue(UiAction action);
    std::optional<UiAction> next();
    const auto& targets()const{return targets_;}
    const ViewerUiState& state()const{return state_;}
    static void fps(float width,double fps,double ms);
};
}
