#include "port/runtime/event_tree_view.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace port::runtime {
namespace {

// Colours and widths the earlier host scene used; see the header for what is and is not recovered.
constexpr render::Color kWonBranch{0.25f, 0.85f, 0.25f, 0.9f};
constexpr render::Color kPendingBranch{0.75f, 0.15f, 0.15f, 0.9f};
constexpr render::Color kMissingPanel{0.55f, 0.6f, 0.7f, 0.8f};
constexpr float kConnectorWidth = 2.0f;
constexpr float kPlaceholderRadius = 26.0f;
constexpr float kPlaceholderWidth = 1.5f;
constexpr float kNumberScale = 0.6f;
constexpr float kNumberBaseline = 14.0f;

void solid(render::Frame& frame, float x, float y, float width, float height,
           const render::Color& colour) {
    if (width <= 0 || height <= 0) return;
    render::Quad quad;
    quad.texture = 0; // solid-colour rect
    quad.destination = {x, y, width, height};
    quad.color = colour;
    frame.quads.push_back(quad);
}

// An axis-aligned segment of `width`, centred on the line, as one solid rect.
void segment(render::Frame& frame, render::Vec2 from, render::Vec2 to, float width,
             const render::Color& colour) {
    if (from.x == to.x) {
        solid(frame, from.x - width * 0.5f, std::min(from.y, to.y), width,
              std::fabs(to.y - from.y), colour);
    } else if (from.y == to.y) {
        solid(frame, std::min(from.x, to.x), from.y - width * 0.5f, std::fabs(to.x - from.x),
              width, colour);
    }
}

} // namespace

void EventTreeView::clear() {
    trees_.clear();
    profile_ = {};
    selected_tree_ = -1;
    cursor_ = -1;
    mode_id_ = 0;
    state_ = hp2::frontend::kViewStateBrowsing_001FAF60;
    zoom_level_ = hp2::frontend::kZoomLevelMin_001FAB50;
}

const hp2::frontend::EventTree_001FFC40* EventTreeView::tree() const {
    if (selected_tree_ < 0 || selected_tree_ >= static_cast<int>(trees_.size())) return nullptr;
    return &trees_[static_cast<std::size_t>(selected_tree_)];
}

EventTreeLoadResult EventTreeView::load(assets::Source& source, const std::string& relative_path,
                                        int mode_id,
                                        const hp2::memorycard::Profile_002FC050& profile) {
    EventTreeLoadResult result;
    clear();
    profile_ = profile;
    mode_id_ = mode_id;
    std::vector<std::uint8_t> buffer;
    try {
        buffer = source.read(relative_path);
    } catch (const std::exception& error) {
        result.error = error.what();
        return result;
    }
    trees_ = hp2::frontend::parse_event_trees(buffer);
    // FUN_00185568: profile+0x1072C == 8 selects "Championship", == 9 selects "Hot Pursuit".
    const int wanted_type = mode_id == 9 ? 1 : (mode_id == 8 ? 0 : -1);
    for (std::size_t i = 0; i < trees_.size(); ++i) {
        if (trees_[i].type_0x08 == wanted_type) selected_tree_ = static_cast<int>(i);
    }
    const auto* selected = tree();
    if (selected == nullptr || selected->records.empty()) {
        result.error = "no event tree for mode id " + std::to_string(mode_id);
        clear();
        return result;
    }
    // FUN_001F96B8: `FUN_001FA840(view, FUN_001F5D00(0x2FC1B4, tree->type_0x08))`. The profile
    // supplies the last node the player was on; FUN_001F5D00's own fallback is event 1, and an
    // all-zero profile stores 0, which FUN_001FA840 does not match -- so the port falls back to
    // event 1 the same way it did before, but now for a recovered reason. (E-0021-09)
    const int stored_start = profile_.StartNode_001F5D00(selected->type_0x08);
    cursor_ = selected->FindLeafByNumber_001FA840(static_cast<std::uint8_t>(stored_start));
    if (cursor_ < 0) {
        cursor_ = selected->FindLeafByNumber_001FA840(
            static_cast<std::uint8_t>(hp2::memorycard::kDefaultStartNode_001F5D00));
    }
    // FUN_001F96B8: `piVar16[0x1d] = DAT_002FC1C0`, i.e. the view state comes from the profile,
    // and a stored 2 zooms straight into the selected event. FUN_001F9F10's intro path, which
    // forces state 7, is not translated.
    state_ = profile_.EventTreeViewState_001FAF60();
    if (state_ != hp2::frontend::kViewStateEventSelected_001FAF60) {
        state_ = hp2::frontend::kViewStateBrowsing_001FAF60;
    }
    // FUN_001FAB50(view, 0) at the end of the build pass: level 0.
    zoom_level_ = hp2::frontend::kZoomLevelMin_001FAB50;
    result.ok = true;
    result.tree_name = selected->name;
    result.tree_type = selected->type_0x08;
    result.node_count = static_cast<int>(selected->records.size());
    return result;
}

EventTreeNode EventTreeView::node_at(std::size_t index) const {
    EventTreeNode node;
    const auto* selected = tree();
    if (selected == nullptr || index >= selected->leaves.size()) return node;
    const auto& leaf = selected->leaves[index];
    const auto& record = selected->records[index];
    node.index = static_cast<int>(index);
    node.number = record.number_0x28;
    node.name = record.name_0x08;
    node.x = leaf.x;
    node.y = leaf.y;
    node.depth = leaf.depth_0x1C;
    node.group = leaf.group;
    node.status = status_of(index);
    node.string_id = record.string_id_0xB0;
    const char* panel = hp2::frontend::EventPanelTexture_001F75F0(selected->type_0x08, node.status);
    if (panel != nullptr) node.panel_texture = panel;
    node.selected = static_cast<int>(index) == cursor_;
    // FUN_001FA9C8 writes 2.0f into the node it moves to and 1.0f into the one it leaves.
    node.scale = node.selected ? 2.0f : 1.0f;
    node.links = leaf.outgoing;
    return node;
}

std::vector<EventTreeNode> EventTreeView::nodes() const {
    std::vector<EventTreeNode> result;
    const auto* selected = tree();
    if (selected == nullptr) return result;
    for (std::size_t i = 0; i < selected->leaves.size(); ++i) result.push_back(node_at(i));
    return result;
}

EventTreeNode EventTreeView::cursor_info() const {
    if (cursor_ < 0) return {};
    return node_at(static_cast<std::size_t>(cursor_));
}

int EventTreeView::status_of(std::size_t index) const {
    const auto* selected = tree();
    if (selected == nullptr) return hp2::frontend::kEventStatusLocked;
    // Status_002010B8 raises NotImplemented on the branches whose evidence is missing (GoalTypes
    // 3 and 4, and events 31 and 32). A single unrecoverable node must not take the whole screen
    // down, so this is the boundary that turns it into the status the original shows for an
    // untouched save: Locked. hp2::guard is the project's one catch point (AGENT.md R3).
    int status = hp2::frontend::kEventStatusLocked;
    const std::string gap = hp2::guard([&] {
        status = selected->Status_002010B8(profile_, static_cast<int>(index));
    });
    if (!gap.empty()) return hp2::frontend::kEventStatusLocked;
    return status;
}

std::uint32_t EventTreeView::title_string_id() const {
    return hp2::frontend::TitleStringId_001F96B8(mode_id_);
}

bool EventTreeView::move_cursor(int direction) {
    const auto* selected = tree();
    if (selected == nullptr || cursor_ < 0) return false;
    // FUN_001FA9C8, verbatim through the recovered walks: -1 and +1 take the nearest leaf in X at
    // the same depth without wrapping (FUN_001FA880), +2 and -2 the nearest in X on one of the
    // two adjacency lists (FUN_001FA978). The port's previous rule -- step one place through a
    // list of same-depth leaves in parse order -- was an invention and is gone. (E-0021-19/20)
    const int target = selected->MoveCursor_001FA9C8(cursor_, direction);
    if (target < 0 || target == cursor_) return false;
    // FUN_001FA9C8 also refuses the move when FUN_001F77F0 rejects the destination: events 31 and
    // 32 are only enterable once their completion gates are met. Those gates are not recovered,
    // so the port applies the part that is -- a node reporting Locked cannot be entered -- and
    // the rest stays an open question.
    if (status_of(static_cast<std::size_t>(target)) == hp2::frontend::kEventStatusLocked) {
        const auto& record = selected->records[static_cast<std::size_t>(target)];
        if (record.number_0x28 == 0x1f || record.number_0x28 == 0x20) return false;
    }
    cursor_ = target;
    return true;
}

int EventTreeView::move_cursor_sound(int direction) {
    // FUN_001FAF60 state 0: messages 6 and 7 answer 0x14 (TreeLeftRight), messages 8 and 9
    // answer 0x13 (TreeUpDown), and every move that does not land on a new node answers 0 so
    // nothing plays. (E-0017-02)
    if (state_ != hp2::frontend::kViewStateBrowsing_001FAF60) {
        return hp2::frontend::kSoundSilent_001FAF60;
    }
    if (!move_cursor(direction)) return hp2::frontend::kSoundSilent_001FAF60;
    if (direction == -1 || direction == 1) return hp2::frontend::kSoundTreeLeftRight_001FAF60;
    return hp2::frontend::kSoundTreeUpDown_001FAF60;
}

int EventTreeView::confirm() {
    // FUN_001FAF60 state 0, message 1: store the node into the profile through FUN_001F5D28,
    // zoom in through FUN_001FAC78 and answer 0x15. The zoom-in animation itself
    // (FUN_001F9598 over document 18) is not translated, so the port goes straight to state 2.
    if (state_ != hp2::frontend::kViewStateBrowsing_001FAF60 || cursor_ < 0) {
        return hp2::frontend::kSoundSilent_001FAF60;
    }
    const auto* selected = tree();
    if (selected == nullptr) return hp2::frontend::kSoundSilent_001FAF60;
    const auto& record = selected->records[static_cast<std::size_t>(cursor_)];
    profile_.SetStartNode_001F5D28(selected->type_0x08, record.number_0x28);
    state_ = hp2::frontend::kViewStateEventSelected_001FAF60;
    step_zoom(1);
    return hp2::frontend::kSoundTreeZoomInOut_001FAF60;
}

int EventTreeView::back() {
    // FUN_001FAF60 state 2, message 0x11: FUN_001FAD20 zooms back out and answers 0x15.
    if (state_ != hp2::frontend::kViewStateEventSelected_001FAF60) {
        return hp2::frontend::kSoundSilent_001FAF60;
    }
    state_ = hp2::frontend::kViewStateBrowsing_001FAF60;
    step_zoom(-1);
    return hp2::frontend::kSoundTreeZoomInOut_001FAF60;
}

bool EventTreeView::step_zoom(int delta) {
    // FUN_001FAB50, verbatim apart from the animation it starts: clamp view+0x138 into 0..1 and
    // take that level's scale out of the gp-0x4178 table. The original flags the mover to
    // interpolate; the port snaps, because the mover's easing is not recovered.
    int level = zoom_level_ + delta;
    if (level > hp2::frontend::kZoomLevelMax_001FAB50) level = hp2::frontend::kZoomLevelMax_001FAB50;
    if (level < hp2::frontend::kZoomLevelMin_001FAB50) level = hp2::frontend::kZoomLevelMin_001FAB50;
    if (level == zoom_level_ && delta != 0) return false;
    zoom_level_ = level;
    return true;
}

EventTreeOverlayStats append_event_tree(const EventTreeView& view, const WidgetSizes& sizes,
                                        render::Resources& resources, render::Frame& frame,
                                        const hp2::frontend::FeWidget* panel_template) {
    EventTreeOverlayStats stats;
    const auto nodes = view.nodes();
    if (nodes.empty()) return stats;

    // FUN_001FAB00(320.0f, 240.0f, view, leaf): the mover's target is
    // `centre - leaf_position * zoom`, so drawing a node at `centre + (pos - focus) * zoom`
    // puts the selected node on the centre, which is the same arithmetic. The centre is the
    // recovered 320/240 rather than half the frame, because the frame is the authored 640x480
    // space and the original hardcodes those two literals. (E-0021-13)
    const render::Vec2 centre{hp2::frontend::kCameraCentreX_001FAB00,
                              hp2::frontend::kCameraCentreY_001FAB00};
    const float zoom = view.zoom();
    render::Vec2 focus{0, 0};
    for (const auto& node : nodes) {
        if (node.selected) focus = {node.x, node.y};
    }
    const auto place = [&](float x, float y) {
        return render::Vec2{centre.x + (x - focus.x) * zoom, centre.y + (y - focus.y) * zoom};
    };

    // tLineSegment: the connectors, as three axis-aligned runs, under everything else.
    for (const auto& node : nodes) {
        const auto from = place(node.x, node.y);
        for (const int other_index : node.links) {
            if (other_index < 0 || other_index >= static_cast<int>(nodes.size())) continue;
            const auto& other = nodes[static_cast<std::size_t>(other_index)];
            const auto to = place(other.x, other.y);
            const auto colour = node.status == hp2::frontend::kEventStatusWon ? kWonBranch
                                                                             : kPendingBranch;
            const float mid = (from.y + to.y) * 0.5f;
            segment(frame, from, {from.x, mid}, kConnectorWidth, colour);
            segment(frame, {from.x, mid}, {to.x, mid}, kConnectorWidth, colour);
            segment(frame, {to.x, mid}, to, kConnectorWidth, colour);
            ++stats.connectors;
        }
    }

    // tEventLeaf: the panel, at twice the size when it is the node under the cursor, which is the
    // 1.0f/2.0f FUN_001FA9C8 writes into the node it leaves and the one it moves to.
    const auto* numbers = sizes.font(hp2::frontend::StringHash_002534A8("FONT_EVENTNUMBERS"));
    for (const auto& node : nodes) {
        const auto at = place(node.x, node.y);
        const auto panel_hash = node.panel_texture.empty()
            ? 0u : hp2::frontend::StringHash_002534A8(node.panel_texture.c_str());
        const auto bitmap = panel_hash == 0 ? nullptr : resources.texture(panel_hash);
        if (bitmap && bitmap->width > 0 && bitmap->height > 0) {
            // The authored EVENT_PANEL scale, then the node's own 1.0/2.0, then the camera zoom.
            // A camera scales the scene, so the panel scales with it exactly as the position
            // does -- drawing positions through `place` but sizes at a fixed factor, as this
            // file used to, had no original behaviour behind it.
            const float authored_x = panel_template ? panel_template->scale_x : 1.0f;
            const float authored_y = panel_template ? panel_template->scale_y : 1.0f;
            const float width =
                static_cast<float>(bitmap->width) * authored_x * node.scale * zoom * kPortPanelFit;
            const float height =
                static_cast<float>(bitmap->height) * authored_y * node.scale * zoom * kPortPanelFit;
            render::Quad quad;
            quad.texture = panel_hash;
            quad.destination = {at.x - width * 0.5f, at.y - height * 0.5f, width, height};
            quad.source = {0, 0, static_cast<float>(bitmap->width), static_cast<float>(bitmap->height)};
            frame.quads.push_back(quad);
            ++stats.panels;
        } else {
            // The LOCKED and UNLOCKED panels are not present in any bundle this port can read, so
            // the node is drawn as an explicit placeholder rather than with invented art. The
            // earlier host scene drew a circle; a frame carries no arc primitive, so this is the
            // same idea as a square outline. Both are port-side placeholders either way.
            const float radius = kPlaceholderRadius * node.scale;
            const float side = radius * 2;
            solid(frame, at.x - radius, at.y - radius, side, kPlaceholderWidth, kMissingPanel);
            solid(frame, at.x - radius, at.y + radius - kPlaceholderWidth, side, kPlaceholderWidth, kMissingPanel);
            solid(frame, at.x - radius, at.y - radius, kPlaceholderWidth, side, kMissingPanel);
            solid(frame, at.x + radius - kPlaceholderWidth, at.y - radius, kPlaceholderWidth, side, kMissingPanel);
            ++stats.missing_panels;
        }

        if (numbers == nullptr) continue;
        const auto text = std::to_string(node.number);
        const float scale = node.scale * kNumberScale;
        float pen = at.x - WidgetSizes::text_advance(*numbers, text) * scale * 0.5f;
        const float top = at.y - kNumberBaseline * scale;
        bool drew = false;
        for (const unsigned char code : text) {
            const hp2::frontend::FeGlyph* glyph = nullptr;
            for (const auto& candidate : numbers->glyphs) {
                if (candidate.code == code) { glyph = &candidate; break; }
            }
            if (glyph == nullptr) continue;
            render::Quad quad;
            quad.texture = numbers->texture_hash;
            quad.destination = {pen, top, glyph->w * scale, glyph->h * scale};
            quad.source = {glyph->u, glyph->v, glyph->w, glyph->h};
            frame.quads.push_back(quad);
            pen += glyph->advance * scale;
            drew = true;
        }
        if (drew) ++stats.numbers;
    }
    return stats;
}

} // namespace port::runtime
