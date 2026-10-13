#pragma once
#include "port/runtime/frontend_runtime.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

// TASK-0018 / D008: Godot translation shim over port::runtime::FrontEndRuntime.
//
// The front end itself -- screens, menus, navigation, localisation, audio scheduling,
// animation and frame production -- lives in the runtime and has no Godot dependency. This
// class exists only to turn native results into Godot values for the older Control-based host
// scene, and to resolve the ZZDATA search root, which needs ProjectSettings/OS.
//
// New host code should not grow this Dictionary surface. The native path goes through
// Ps2RenderHost, which reads the runtime directly.
namespace port::godot_bridge {

class Ps2FrontEndLayout : public godot::RefCounted {
    GDCLASS(Ps2FrontEndLayout, godot::RefCounted)
protected:
    static void _bind_methods();

public:
    // Loads the screen list from a ZZDATA-relative bundle, e.g. "FRONTEND/FRONTB.BUN".
    // {"ok": bool, "screen_count": int, "error": String}.
    godot::Dictionary load(const godot::String& relative_path);

    // Loads bitmap font tables from ZZDATA-relative bundles (fonts are spread across
    // FRONTEND/FRONTB.BUN, GLOBAL/GLOBALB.BUN and GLOBAL/INGAMEB.BUN). Returns the font count.
    int load_fonts(const godot::PackedStringArray& bundles);

    // Recovered menu runtime, projected to Dictionaries. See port::runtime::MenuState.
    godot::Dictionary menu_open(int screen_index);
    godot::Dictionary menu_joy_event(int pad_index, int joy_event, bool pressed);
    godot::Dictionary menu_state() const;
    godot::Dictionary menu_tick(double delta);
    godot::Array drain_menu_audio();
    godot::Dictionary frontend_music(bool previous_finished);

    // View Event Tree screen: true when the recovered tree consumed the event, so the host does
    // not also feed it to the menu.
    bool event_tree_joy_event(int joy_event);

    void title_visual_state(bool movie_state, bool can_start);
    void title_pulse_restart();
    void set_startup_presentation(bool movie_visible, bool letterbox, bool startup_active);

    // Index of the screen whose recovered name hash matches, or -1.
    int find_screen_by_name(const godot::String& screen_name) const;

    // Native, unbound: the renderer reads the runtime through these, with no Godot value in
    // between. See Ps2RenderHost.
    port::runtime::FrontEndRuntime& runtime() { return runtime_; }

private:
    port::runtime::FrontEndRuntime runtime_;
};

} // namespace port::godot_bridge
