#pragma once
#include "game_core/frontend/eventtree.hpp"
#include "port/assets/catalog.hpp"
#include "port/runtime/frame_builder.hpp"
#include <string>
#include <vector>

// TASK-0018: the View Event Tree screen's view state and its port-side presentation, with no
// Godot and no backend. Parsing, geometry and navigation live in game_core and carry their ELF
// provenance there (hp2::frontend, `..\indep\src\eventtree.cpp`); this holds only which tree is
// open, where the cursor is, and how the port draws them.
namespace port::runtime {

// One node as a host or renderer reads it.
struct EventTreeNode {
    int index = 0;
    int number = 0;
    std::string name;
    float x = 0, y = 0;
    int depth = 0, group = -1, status = 0;
    std::uint32_t string_id = 0;
    std::string panel_texture; // empty when FUN_001F75F0's switch does not cover the status
    bool selected = false;
    float scale = 1.0f; // FUN_001FA9C8 writes 2.0f into the node it moves to, 1.0f into the other
    std::vector<int> links; // record indices this node points at
};

struct EventTreeLoadResult {
    bool ok = false;
    std::string tree_name, error;
    int tree_type = -1, node_count = 0;
};

class EventTreeView {
public:
    // Loads both trees from a bundle and selects one by the mode id the mode-select screens write
    // into DAT_0030C77C: 8 picks "Championship", 9 picks "Hot Pursuit" (FUN_00185568).
    EventTreeLoadResult load(assets::Source& source, const std::string& relative_path, int mode_id);

    bool loaded() const { return tree() != nullptr; }
    std::vector<EventTreeNode> nodes() const;
    int cursor() const { return cursor_; }

    // FUN_001FA9C8: -1 and +1 walk the node's +0x1C link list, +2 the +0x20 list and -2 the +0x28
    // list. Returns true when the cursor actually moved, which is what the original turns into a
    // sound or into silence.
    bool move_cursor(int direction);

    // The node under the cursor, or a default-constructed node when there is none.
    EventTreeNode cursor_info() const;

    void clear();

private:
    const hp2::frontend::EventTree_001FFC40* tree() const;
    EventTreeNode node_at(std::size_t index) const;

    std::vector<hp2::frontend::EventTree_001FFC40> trees_;
    int selected_tree_ = -1;
    int cursor_ = -1;
};

// ------------------------------------------------------------------ port-side presentation
//
// NOT RECOVERED. FUN_001F96B8 ends by attaching a "StillCameraMover" to the selected node; its
// zoom, easing and bounds were never reversed, and the authored node coordinates span far more
// than 640x480 (130 per column, 350 per row). The two constants below were measured off an
// original-hardware screenshot purely so the recovered geometry is legible, and the selected node
// is centred because that is where the reference screenshot shows it. Replace both once the
// camera is reversed. The connector colours are in the same position: the original colours them
// per branch state and that mapping was not recovered, so a won source is green and anything else
// red, matching what the reference screenshot shows for a passed and a pending branch.
inline constexpr float kPortZoom = 0.6f;
inline constexpr float kPortPanelScale = 0.26f;

// Appends the tree over whatever the frame already holds. The original's runtime visibility
// changes for screen 39's authored placeholder widgets were not recovered, so the tree is painted
// over them, exactly as the earlier host scene did.
//
// `missing_panels` reports nodes whose panel texture is in no readable bundle; those draw as an
// explicit outline placeholder rather than with invented art.
struct EventTreeOverlayStats {
    int panels = 0, connectors = 0, numbers = 0, missing_panels = 0;
};
EventTreeOverlayStats append_event_tree(const EventTreeView& view, const WidgetSizes& sizes,
                                        render::Resources& resources, render::Frame& frame);

} // namespace port::runtime
