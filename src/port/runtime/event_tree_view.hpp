#pragma once
#include "game_core/indep/src/eventtree.hpp"
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
    //
    // `profile` is the player profile the statuses and the start node come from. A
    // default-constructed one is all zero, which is what the original hands its own driver
    // before a load, so passing one is always safe. The view keeps a copy: the original reads
    // the live global, and the port has no global.
    EventTreeLoadResult load(assets::Source& source, const std::string& relative_path, int mode_id,
                             const hp2::memorycard::Profile_002FC050& profile = {});

    bool loaded() const { return tree() != nullptr; }
    std::vector<EventTreeNode> nodes() const;
    int cursor() const { return cursor_; }

    // FUN_001FAF60's state word, view+0x74. FUN_001F96B8 initialises it from profile+0x170.
    int state() const { return state_; }

    // FUN_001F96B8 sets the EVENT_TREE widget's localised string id from the selected mode.
    std::uint32_t title_string_id() const;

    // FUN_001FA9C8: -1 and +1 walk the leaves at the same depth, nearest in X and without
    // wrapping (FUN_001FA880); +2 and -2 walk the two adjacency lists, nearest in X
    // (FUN_001FA978). Returns true when the cursor actually moved, which is what the original
    // turns into a sound or into silence.
    bool move_cursor(int direction);

    // FUN_001FAF60's answer for one input, as a menu sound id from kMenuSoundTable_002FB0A8:
    // 0x14 for a left/right move, 0x13 for up/down, 0x15 for the zoom in and out, 0x18 for
    // starting an event, 8 (MenuWrong) for an event whose record+0x3E is below 1, and 0 for
    // "nothing happened, play nothing". `direction` is the FUN_001FA9C8 ordinal.
    int move_cursor_sound(int direction);

    // State 0 confirm and state 2 back, the two transitions FUN_001FAF60 drives through
    // FUN_001FAC78 and FUN_001FAD20. Both return the sound id the original answers with.
    int confirm();
    int back();

    // FUN_001FAB50(view, delta): clamp the zoom level to 0..1 and take that level's scale out of
    // the recovered two-float table. True when the level changed.
    bool step_zoom(int delta);
    int zoom_level() const { return zoom_level_; }
    float zoom() const { return hp2::frontend::kZoomLevels_gp_0x4178[zoom_level_]; }

    // The node under the cursor, or a default-constructed node when there is none.
    EventTreeNode cursor_info() const;

    void clear();

private:
    const hp2::frontend::EventTree_001FFC40* tree() const;
    EventTreeNode node_at(std::size_t index) const;
    int status_of(std::size_t index) const;

    std::vector<hp2::frontend::EventTree_001FFC40> trees_;
    hp2::memorycard::Profile_002FC050 profile_;
    int selected_tree_ = -1;
    int cursor_ = -1;
    int mode_id_ = 0;
    int state_ = hp2::frontend::kViewStateBrowsing_001FAF60;
    int zoom_level_ = hp2::frontend::kZoomLevelMin_001FAB50;
};

// ------------------------------------------------------------------ camera and presentation
//
// RECOVERED by TASK-0021, replacing TASK-0018's screenshot calibration. The camera is a
// 0x70-byte `tStillCameraMover` positioned by `FUN_001FAB00(320.0f, 240.0f, view, leaf)` as
// `centre - leaf_position * zoom`, so the selected node is pinned to the centre of the 640x480
// authored space -- which is what the reference screenshot shows, and what this file already
// did. The zoom is one of two discrete levels from the table at gp-0x4178,
// `hp2::frontend::kZoomLevels_gp_0x4178` == {0.4f, 1.0f}; the port's measured 0.6f is gone.
// What is still **not** recovered is the mover's easing between the two levels, so the port
// snaps. `EventTreeView::step_zoom` is the recovered level clamp; the interpolation is not.
//
// The panel's size is now mostly recovered. The original never scales a texture here:
// FUN_001F6FA0 looks screen 39's own EVENT_PANEL widget up by hash (0xA3B23510) and
// FUN_00144A28 binds FUN_001F75F0's texture name onto it, so the drawn size is that widget's
// **authored** scale times the node's 1.0/2.0 times the camera zoom. Screen 39 authors
// EVENT_PANEL at scale (1.0, 0.8) over SPEED_EVENT_PANEL_LOCKED -- that 0.8 is where the
// panel's vertical squash comes from, and the port now applies it. (E-0021-21)
//
// STILL NOT RECOVERED, and deliberately reduced to this one factor: the authored panel is
// 256 px wide, so at the recovered zoom 0.4 a node would be ~102 px across a lattice whose
// columns are 130 * 0.4 == 52 px apart, i.e. every node would overlap its neighbours. Something
// between the authored widget space and the camera is still missing -- the `tStillCameraMover`
// may carry a base scale of its own, or the tree may be mapped into the BOTTOM_MASK rectangle
// FUN_001F96B8 stores at view+0x3F8..+0x404 rather than into the full 640x480. Until that is
// read out of the binary, this single port-side factor reconciles the two, chosen so the result
// matches the original-hardware reference screenshot. It multiplies nothing that is recovered:
// the authored scale, the node scale and the zoom all apply on their own.
inline constexpr float kPortPanelFit = 0.65f;

// STILL NOT RECOVERED: the connector colour rule. The original colours each `tLineSegment` per
// branch state and that mapping was not read out of FUN_001F96B8, so a won source is green and
// anything else red, matching what the reference screenshot shows for a passed and a pending
// branch.
//
// Appends the tree over whatever the frame already holds. TASK-0021 identified the two widgets
// the original does toggle at runtime -- HELP_TEXT and HELP_BUTTON, revealed when the state-7
// zoom transition ends -- but the authored placeholder text widgets on screen 39 are left
// visible, as the earlier host scene did, because nothing was found that hides them.
//
// `missing_panels` reports nodes whose panel texture is in no readable bundle; those draw as an
// explicit outline placeholder rather than with invented art.
struct EventTreeOverlayStats {
    int panels = 0, connectors = 0, numbers = 0, missing_panels = 0;
};
// `panel_template` is screen 39's authored EVENT_PANEL widget, whose scale every node panel
// inherits. Pass nullptr only when the screen has no such widget; the nodes then fall back to
// the texture's own size, which is a port-side fallback and is marked as one in the code.
EventTreeOverlayStats append_event_tree(const EventTreeView& view, const WidgetSizes& sizes,
                                        render::Resources& resources, render::Frame& frame,
                                        const hp2::frontend::FeWidget* panel_template = nullptr);

} // namespace port::runtime
