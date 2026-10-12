#pragma once
#include "game_core/engine/engine_shell.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
namespace port::godot_bridge {
class MainLoopObserver : public godot::RefCounted {
    GDCLASS(MainLoopObserver, godot::RefCounted)
protected:
    static void _bind_methods();
public:
    godot::Dictionary initialize(std::int64_t scenario=0,bool strict=false,bool alternate_startup=false);
    godot::Dictionary step_frame(double host_delta_seconds=0,bool boundary=true);
    godot::Dictionary shutdown();
    godot::Dictionary snapshot() const;
    godot::Array get_trace(bool startup=false) const;
    godot::Array get_world_connections() const;
private:
    hp2::engine::EngineShell shell_;
    bool strict_=false;
    std::int64_t scenario_=0;
};
}
