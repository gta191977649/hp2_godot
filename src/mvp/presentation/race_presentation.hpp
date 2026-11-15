#pragma once

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <memory>

namespace hp2 {

class ImGuiOverlay;
class TrackViewer;
class GameResources;

class RacePresentation : public godot::Node3D {
    GDCLASS(RacePresentation, godot::Node3D)

public:
    RacePresentation();
    ~RacePresentation() override;
    static godot::String validate_game_iso(const godot::String& path);
    void set_graphics_menu_settings(const godot::Callable& settings);
    godot::String start_game(const godot::String& assets, const godot::String& profile, int track,
        int tree, int event, bool nativeDebug);
    void _process(double delta) override;
    void _input(const godot::Ref<godot::InputEvent>& event) override;
    void report_launch_error(const godot::String& stage, const godot::String& track, const godot::String& reason);
    godot::Dictionary telemetry() const;
    godot::Node3D* race_scene() const;
    godot::PackedVector3Array photo_route() const;
    void test_update_scenery_camera(bool enabled);
    void frontend_pointer_input(const godot::Ref<godot::InputEvent>& event);

protected:
    static void _bind_methods();
    void _notification(int what);

private:
    struct Session;
#ifdef HP2_MVP_IMGUI
    godot::String assets_, profile_;
    std::unique_ptr<GameResources> viewerFiles_;
    std::unique_ptr<TrackViewer> viewer_;
    std::unique_ptr<ImGuiOverlay> debug_;
    void open_track_viewer();
    void service_track_viewer(double delta);
#endif
    std::unique_ptr<Session> session_;
    godot::Callable graphicsMenuSettings_;
    bool failed_{};
    bool nativeDebug_{};
    void report_assert(const godot::String& message);
    void quit_game();
    bool start(const godot::String& assets, const godot::String& profile, bool frontend, int track,
        int tree = -1, int event = 0, bool returnToMenu = false, godot::String* launchError = nullptr);
    void retire_scene();
    void retire_session();
    void fail(const char* stage, const char* reason);
};

}
