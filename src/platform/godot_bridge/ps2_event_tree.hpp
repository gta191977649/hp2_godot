#pragma once
#include "game_core/frontend/eventtree.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <vector>

// TASK-0018: exposes the recovered event tree (hp2::frontend, `..\indep\src\eventtree.cpp`) to
// GDScript so the View Event Tree screen can be drawn from the player's own FRONTB.BUN instead of
// its authored placeholders. Port-side glue only; the parsing, geometry and navigation live in
// game_core and carry their ELF provenance there.
namespace port::godot_bridge {

class Ps2EventTree : public godot::RefCounted {
    GDCLASS(Ps2EventTree, godot::RefCounted)
protected:
    static void _bind_methods();

public:
    // Loads both trees from a ZZDATA-relative bundle (FRONTEND/FRONTB.BUN) and selects one by the
    // mode id the mode-select screens write into DAT_0030C77C: 8 picks "Championship" (the World
    // Racing branch) and 9 picks "Hot Pursuit" (the Ultimate Racer branch), matching
    // FUN_00185568. Returns {"ok", "tree_name", "node_count", "error"}.
    godot::Dictionary load(const godot::String& relative_path, int mode_id);

    // One Dictionary per node: number, name, x, y, depth, status, panel_texture, selected,
    // scale, group, and "links" as the record indices this node points at.
    godot::Array get_nodes() const;

    int get_cursor() const { return cursor_; }
    // FUN_001FA9C8: -1 and +1 walk the node's +0x1C link list, +2 and -2 the +0x20 and +0x28
    // lists. Returns true when the cursor actually moved, which is what the original turns into a
    // sound or into silence.
    bool move_cursor(int direction);

    // {"number", "name", "status", "panel_texture", "string_id"} for the node under the cursor.
    godot::Dictionary get_cursor_info() const;

    godot::String get_last_error() const { return last_error_; }

private:
    std::vector<hp2::frontend::EventTree_001FFC40> trees_;
    int selected_tree_ = -1;
    int cursor_ = -1;
    godot::String last_error_;

    const hp2::frontend::EventTree_001FFC40* tree() const;
};

} // namespace port::godot_bridge
