#pragma once
#include "game_core/port/driving/session.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
namespace port::godot_bridge {
// Port-side Debug entry. Never implements game physics in the host.
class Ps2DrivingLab : public godot::RefCounted {
    GDCLASS(Ps2DrivingLab,godot::RefCounted)
    port::driving::Session session_;
protected:
    static void _bind_methods();
public:
    godot::Dictionary load_config(const godot::String& code);
    godot::Dictionary action(const godot::String& name);
    godot::Dictionary set_input(double throttle,double brake,double steering);
    godot::Dictionary tick(double elapsed=0);
    godot::Dictionary get_state()const;
};
}
