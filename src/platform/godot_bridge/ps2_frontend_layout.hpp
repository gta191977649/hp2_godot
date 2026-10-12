#pragma once
#include "game_core/frontend/fmenus.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <vector>

// TASK-0013: exposes the original FrontEnd screen/widget layout records (hp2::frontend) to
// GDScript so Godot can draw the real authored screens -- real widget names, positions, scales,
// colours and TexturePack references -- instead of a hand-built debug UI. Reads the layout
// straight out of the user's configured ZZDATA copy at runtime; nothing is pre-extracted.
namespace port::godot_bridge {

class Ps2FrontEndLayout : public godot::RefCounted {
    GDCLASS(Ps2FrontEndLayout, godot::RefCounted)
protected:
    static void _bind_methods();

public:
    // Loads the screen list from a ZZDATA-relative bundle, e.g. "FRONTEND/FRONTB.BUN".
    // Returns {"ok": bool, "screen_count": int, "error": String}.
    godot::Dictionary load(const godot::String& relative_path);

    int get_screen_count() const { return static_cast<int>(screens_.size()); }

    // {"index": int, "width": int, "height": int, "widget_count": int,
    //  "names": PackedStringArray}  -- cheap summary used to pick a screen.
    godot::Dictionary get_screen_info(int index) const;

    // Ordered widget list for one screen; each entry is a Dictionary carrying every decoded
    // field (see src/game_core/frontend/fmenus.hpp for the record layout and its evidence).
    godot::Array get_widgets(int index) const;

    // Loads bitmap font tables from one or more ZZDATA-relative bundles (fonts are spread across
    // FRONTEND/FRONTB.BUN, GLOBAL/GLOBALB.BUN and GLOBAL/INGAMEB.BUN). Returns the font count.
    int load_fonts(const godot::PackedStringArray& bundles);

    // {"line_height": int, "glyphs": {code: PackedFloat32Array[u, v, w, h, advance]}}
    // Empty Dictionary when the font texture hash is unknown.
    godot::Dictionary get_font(std::int64_t texture_hash) const;

    godot::String get_last_error() const { return last_error_; }

private:
    std::vector<hp2::frontend::FeScreen> screens_;
    std::vector<hp2::frontend::FeFont> fonts_;
    godot::String last_error_;
};

} // namespace port::godot_bridge
