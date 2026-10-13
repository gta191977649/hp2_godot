#pragma once
#include "game_core/port/frontend/screen_manager.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace port::godot_bridge {

// TASK-0011: thin RefCounted wrapper exposing hp2::frontend::ScreenManager to GDScript.
// See src/game_core/frontend/screen_manager.hpp and docs/FRONTEND_MENU.md for evidence
// and the structural-projection scope. No rendering, widgets, or original art are ported.
class FrontEndMenuObserver : public godot::RefCounted {
    GDCLASS(FrontEndMenuObserver, godot::RefCounted)
protected:
    static void _bind_methods();
public:
    godot::Dictionary initialize();
    godot::Dictionary request_screen(const godot::String& name, bool push_history);
    godot::Dictionary go_back();
    godot::Dictionary reset();
    godot::Dictionary snapshot() const;
    godot::Array get_registered_screens() const;
    godot::Array get_history() const;

private:
    hp2::frontend::ScreenManager manager_;
};

} // namespace port::godot_bridge
