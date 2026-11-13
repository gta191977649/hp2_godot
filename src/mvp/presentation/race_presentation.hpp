#pragma once

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <memory>

namespace hp2 {

class ImGuiOverlay;

class RacePresentation : public godot::Node3D {
    GDCLASS(RacePresentation, godot::Node3D)

public:
    RacePresentation();
    ~RacePresentation() override;
    bool start_frontend(const godot::String& assets, const godot::String& profile);
    bool start_race(const godot::String& assets, const godot::String& profile);
    void _process(double delta) override;
    void _input(const godot::Ref<godot::InputEvent>& event) override;
    void report_launch_error(const godot::String& stage, const godot::String& track, const godot::String& reason);
    godot::Dictionary telemetry() const;

protected:
    static void _bind_methods();

private:
    struct Session;
#ifdef HP2_MVP_IMGUI
    std::unique_ptr<ImGuiOverlay> debug_;
#endif
    std::unique_ptr<Session> session_;
    bool failed_{};
    void report_assert(const godot::String& message);
    void quit_game();
    bool start(const godot::String& assets, const godot::String& profile, bool frontend);
    void fail(const char* stage, const char* reason);
};

}
