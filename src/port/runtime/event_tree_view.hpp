#pragma once
#include "game_core/indep/src/eventtree.hpp"
#include "game_core/indep/src/frontendscreens/options.hpp"
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
    int visibility = 1; // FUN_001F77F0; -1 denotes an explicitly reported unknown.
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
    const hp2::frontend::EventTree_001FFC40* selected_tree() const { return tree(); }
    const hp2::memorycard::Profile_002FC050& profile() const { return profile_; }
    const std::vector<std::string>& status_gaps() const { return status_gaps_; }
    std::vector<EventTreeNode> nodes() const;
    int cursor() const { return cursor_; }

    // FUN_001FAF60's state word, view+0x74. FUN_001F96B8 initialises it from profile+0x170.
    int state() const { return state_; }

    // The selected tree's type_0x08: 0 Championship, 1 Hot Pursuit, -1 when nothing is loaded.
    int tree_type() const;

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
    float zoom() const { return zoom_path_.field_0x1c; }
    float alpha() const { return alpha_path_.field_0x1c; }
    float advance_border_phase() const { const float phase = border_phase_; border_phase_ += 0.2f; return phase; }
    render::Vec2 camera_offset() const { return {camera_x_.field_0x1c, camera_y_.field_0x1c}; }
    // FUN_001FA698 advances the same zero-velocity cubic used by the menu exit animation.
    void tick(float delta_seconds);
    bool transitioning() const { return state_ == 1 || state_ == 3 || state_ == 4 || state_ == 6; }
    // FUN_001FAE40 and state-5 return in FUN_001FAF60.
    bool show_preview();
    bool close_preview();

    // The node under the cursor, or a default-constructed node when there is none.
    EventTreeNode cursor_info() const;

    void clear();

private:
    const hp2::frontend::EventTree_001FFC40* tree() const;
    EventTreeNode node_at(std::size_t index) const;
    int status_of(std::size_t index) const;
    void target_camera(float zoom);
    static void target(hp2::frontend::AnimationGroup& path, float value);

    std::vector<hp2::frontend::EventTree_001FFC40> trees_;
    hp2::memorycard::Profile_002FC050 profile_;
    int selected_tree_ = -1;
    int cursor_ = -1;
    int mode_id_ = 0;
    int state_ = hp2::frontend::kViewStateBrowsing_001FAF60;
    int zoom_level_ = hp2::frontend::kZoomLevelMin_001FAB50;
    hp2::frontend::AnimationGroup zoom_path_, alpha_path_, camera_x_, camera_y_;
    mutable std::vector<std::string> status_gaps_;
    mutable float border_phase_ = 0;
};

// TASK-0022, E-0022-G01/G02: FUN_001F7978 uses the authored widget scales and the
// zoom matrix without a fitted size or selected-node multiplier. FUN_001FA458 uses
// the view+0x78 zoom and view+0xA8 translation interpolators, not the car-preview
// tStillCameraMover allocated at view+0x5BC. Colour tables are initialized by
// FUN_001FB790; branch geometry comes from FUN_001F81B8/001F8300/001F8450.
struct EventTreeOverlayStats {
    int panels = 0, connectors = 0, numbers = 0, missing_panels = 0;
};
// `panel_template` is screen 39's authored EVENT_PANEL widget, whose scale every node panel
// inherits. Pass nullptr only when the screen has no such widget; the nodes then fall back to
// the texture's own size, which is a port-side fallback and is marked as one in the code.
EventTreeOverlayStats append_event_tree(const EventTreeView& view, const WidgetSizes& sizes,
                                        render::Resources& resources, render::Frame& frame,
                                        const hp2::frontend::FeWidget* panel_template = nullptr,
                                        const hp2::frontend::FeWidget* number_template = nullptr,
                                        const hp2::frontend::FeScreen* screen_template = nullptr);

} // namespace port::runtime
