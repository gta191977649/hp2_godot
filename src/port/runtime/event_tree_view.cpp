#include "port/runtime/event_tree_view.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace port::runtime {
namespace {

// FUN_001FB790 RGBA float tables, E-0022-G03. Neutral PS2 modulation is128.
constexpr render::Color kWonBranch{0.0f, 80.0f / 128.0f, 0.0f, 80.0f / 128.0f};
constexpr render::Color kPendingBranch{60.0f / 128.0f, 0.0f, 0.0f, 80.0f / 128.0f};
constexpr render::Color kMissingPanel{0.55f, 0.6f, 0.7f, 0.8f};
constexpr float kPlaceholderRadius = 26.0f;
constexpr float kPlaceholderWidth = 1.5f;

void solid(render::Frame& frame, float x, float y, float width, float height,
           const render::Color& colour) {
    if (width <= 0 || height <= 0) return;
    render::Quad quad;
    quad.texture = 0; // solid-colour rect
    quad.destination = {x, y, width, height};
    quad.color = colour;
    frame.quads.push_back(quad);
}

} // namespace

void EventTreeView::clear() {
    trees_.clear();
    status_gaps_.clear();
    border_phase_ = 0;
    profile_ = {};
    selected_tree_ = -1;
    cursor_ = -1;
    mode_id_ = 0;
    state_ = hp2::frontend::kViewStateBrowsing_001FAF60;
    zoom_level_ = hp2::frontend::kZoomLevelMin_001FAB50;
    for (auto* path : {&zoom_path_, &alpha_path_, &camera_x_, &camera_y_}) {
        *path = {};
        path->field_0x40 = 0.5f;
        path->field_0x24 = path->field_0x1c = 0;
        path->field_0x48 = 0;
    }
    zoom_path_.field_0x24 = zoom_path_.field_0x1c = 0.4f;
    alpha_path_.field_0x24 = alpha_path_.field_0x1c = 1.0f;
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
    target_camera(zoom());
    camera_x_.field_0x1c = camera_x_.field_0x24; camera_x_.field_0x48 = 0;
    camera_y_.field_0x1c = camera_y_.field_0x24; camera_y_.field_0x48 = 0;
    if (state_ == 2) { state_ = 0; confirm(); }
    // FUN_001F9F10: event 32 positive but not Won starts intro state 7.
    const int intro = selected->FindLeafByNumber_001FA840(32);
    if (intro >= 0) {
        // FUN_002010B8 event32 first requires events1..31 in BOTH trees Won.
        // Resolve that dependency here, where both parsed trees are available.
        bool completion = true;
        for (const auto& candidate : trees_) {
            for (int number = 1; number <= 31; ++number) {
                const int index = candidate.FindLeafByNumber_001FA840(number);
                if (index < 0) hp2::not_implemented("0x002010B8", "event32 completion event unavailable");
                if (candidate.Status_002010B8(profile_, index) != 4) { completion = false; break; }
            }
            if (!completion) break;
        }
        if (completion) {
            const int status = selected->Status_002010B8(profile_, intro);
            if (status > 0 && status != 4) state_ = 7;
        }
    }
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
    if (node.number == 31) node.visibility = node.status < 0 ? -1 : node.status != 0;
    if (node.number == 32 && node.status == 0) {
        int visibility = 0;
        const auto gap = hp2::guard([&] {
            for (const auto& candidate : trees_) {
                const int row = candidate.FindLeafByNumber_001FA840(31);
                if (row < 0) hp2::not_implemented("0x001F77F0", "event31 tree gate unavailable");
                if (candidate.Status_002010B8(profile_, row) == 4) { visibility = 1; return; }
            }
        });
        if (!gap.empty()) {
            visibility = -1;
            if (std::find(status_gaps_.begin(), status_gaps_.end(), gap) == status_gaps_.end()) status_gaps_.push_back(gap);
        }
        node.visibility = visibility;
    } else if (node.number == 32 && node.status < 0) node.visibility = -1;
    node.string_id = record.string_id_0xB0;
    const char* panel = hp2::frontend::EventPanelTexture_001F75F0(selected->type_0x08, node.status);
    if (panel != nullptr) node.panel_texture = panel;
    node.selected = static_cast<int>(index) == cursor_;
    // FUN_001FA9C8 writes this field; FUN_001F7978 does not apply it to panel size.
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
    int status = -1; // Port-side unknown, never represented as a recovered status.
    const std::string gap = hp2::guard([&] {
        if (selected->records[index].number_0x28 == 32) {
            // Resolve event32's other-tree dependency where both parsed trees exist.
            for (const auto& candidate : trees_) for (int number = 1; number <= 31; ++number) {
                const int record = candidate.FindLeafByNumber_001FA840(number);
                if (record < 0) hp2::not_implemented("0x002010B8", "event32 completion event unavailable");
                if (candidate.Status_002010B8(profile_, record) != 4) { status = 0; return; }
            }
        }
        status = selected->Status_002010B8(profile_, static_cast<int>(index));
    });
    if (!gap.empty() && std::find(status_gaps_.begin(), status_gaps_.end(), gap) == status_gaps_.end())
        status_gaps_.push_back(gap);
    return status;
}

int EventTreeView::tree_type() const {
    const auto* selected = tree();
    return selected == nullptr ? -1 : selected->type_0x08;
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
    target_camera(zoom_path_.field_0x24);
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

void EventTreeView::target(hp2::frontend::AnimationGroup& path, float value) {
    path.field_0x24 = value;
    if (value != path.field_0x1c) path.field_0x48 = 2;
}

void EventTreeView::target_camera(float scale) {
    target(zoom_path_, scale);
    const auto node = cursor_info();
    target(camera_x_, 320.0f - node.x * scale);
    target(camera_y_, 240.0f - node.y * scale);
}

void EventTreeView::tick(float delta_seconds) {
    // FUN_001FA698 / FUN_001DAEC8 / FUN_001DB000, E-0022-T03.
    for (auto* path : {&zoom_path_, &camera_x_, &camera_y_, &alpha_path_})
        ::port::frontend::advance_zero_velocity_path(*path, delta_seconds);
    if (alpha_path_.field_0x48 != 0) return;
    if (state_ == 1 || state_ == 6) {
        state_ = 2;
        profile_.write_u32(hp2::memorycard::kEventTreeViewState_001FAF60, 2);
    } else if (state_ == 3) {
        state_ = 0;
        profile_.write_u32(hp2::memorycard::kEventTreeViewState_001FAF60, 0);
    } else if (state_ == 4) state_ = 5;
}

int EventTreeView::confirm() {
    if (state_ == 7) {
        state_ = profile_.EventTreeViewState_001FAF60();
        if (state_ == 2) { state_ = 0; return confirm(); }
        return hp2::frontend::kSoundSilent_001FAF60;
    }
    if (state_ != 0 || cursor_ < 0) return hp2::frontend::kSoundSilent_001FAF60;
    const auto* selected = tree();
    if (selected == nullptr) return hp2::frontend::kSoundSilent_001FAF60;
    const auto& record = selected->records[static_cast<std::size_t>(cursor_)];
    profile_.SetStartNode_001F5D28(selected->type_0x08, record.number_0x28);
    state_ = 1; // FUN_001FAC78, E-0022-T01: detail zoom is 3.0, not level 1.
    target(alpha_path_, 0.0f);
    target_camera(3.0f);
    return hp2::frontend::kSoundTreeZoomInOut_001FAF60;
}

int EventTreeView::back() {
    if (state_ == 5) { close_preview(); return 0x18; }
    if (state_ != 2) return hp2::frontend::kSoundSilent_001FAF60;
    state_ = 3; // FUN_001FAD20, E-0022-T02.
    target(alpha_path_, 1.0f);
    target_camera(hp2::frontend::kZoomLevels_gp_0x4178[zoom_level_]);
    return hp2::frontend::kSoundTreeZoomInOut_001FAF60;
}

bool EventTreeView::show_preview() {
    if (state_ != 2) return false;
    state_ = 4; // FUN_001FAE40.
    target(alpha_path_, 1.0f);
    return true;
}

bool EventTreeView::close_preview() {
    if (state_ != 5) return false;
    state_ = 6; // FUN_001FAF60 state 5.
    target(alpha_path_, 0.0f);
    return true;
}

bool EventTreeView::step_zoom(int delta) {
    const int level = std::clamp(zoom_level_ + delta, 0, 1);
    if (level == zoom_level_ && delta != 0) return false;
    zoom_level_ = level;
    target_camera(hp2::frontend::kZoomLevels_gp_0x4178[level]);
    return true;
}

EventTreeOverlayStats append_event_tree(const EventTreeView& view, const WidgetSizes& sizes,
                                        render::Resources& resources, render::Frame& frame,
                                        const hp2::frontend::FeWidget* panel_template,
                                        const hp2::frontend::FeWidget* number_template,
                                        const hp2::frontend::FeScreen* screen_template) {
    EventTreeOverlayStats stats;
    const auto nodes = view.nodes();
    if (view.state() != 0 && view.state() != 1 && view.state() != 3) return stats;
    if (nodes.empty()) return stats;

    // FUN_001FA458: current zoom and current interpolated translation.
    const float zoom = view.zoom();
    const auto offset = view.camera_offset();
    const auto place = [&](float x, float y) {
        return render::Vec2{offset.x + x * zoom, offset.y + y * zoom};
    };

    const hp2::frontend::FeWidget *horizontal = nullptr, *vertical = nullptr, *mask = nullptr;
    if (screen_template) for (const auto& widget : screen_template->widgets) {
        if (widget.name_hash == 0x02ECA94Fu) horizontal = &widget;
        if (widget.name_hash == 0x03EA005Du) vertical = &widget;
        if (widget.name_hash == 0xAA5D7D3Fu) mask = &widget;
    }
    render::Rect clip;
    if (mask) { const auto size = sizes.scaled(*mask); clip = {mask->x, mask->y, size.width, size.height}; }
    const auto line = [&](render::Vec2 from, render::Vec2 to, const render::Color& colour) {
        const auto* authored = from.y == to.y ? horizontal : vertical;
        if (!authored) hp2::not_implemented("0x001F6CF0", "authored HLINE/VLINE template unavailable");
        const auto bitmap = resources.texture(authored->texture_hash);
        if (!bitmap || bitmap->width <= 0 || bitmap->height <= 0)
            hp2::not_implemented("0x001F6CF0", "authored line texture unavailable");
        render::Quad quad;
        quad.texture = authored->texture_hash;
        quad.source = {0,0,static_cast<float>(bitmap->width),static_cast<float>(bitmap->height)};
        const auto size = sizes.scaled(*authored);
        if (from.y == to.y) quad.destination = {std::min(from.x,to.x),from.y-size.height*0.5f,std::fabs(to.x-from.x),size.height};
        else quad.destination = {from.x-size.width*0.5f,std::min(from.y,to.y),size.width,std::fabs(to.y-from.y)};
        quad.color = colour;
        if (render::crop(quad, clip)) frame.quads.push_back(quad);
    };

    // FUN_001F81B8 creates shared horizontal runs between adjacent X values in
    // each parent/child group. FUN_001F8300 recolours only the runs whose midpoint
    // lies between a satisfied child and its prerequisite (status with that row >=2).
    const auto* tree = view.selected_tree();
    std::vector<std::vector<int>> groups;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].links.empty()) continue;
        std::vector<int> group{static_cast<int>(i)};
        group.insert(group.end(), nodes[i].links.begin(), nodes[i].links.end());
        for (auto it = groups.begin(); it != groups.end();) {
            bool shares_child = false;
            for (int child : nodes[i].links)
                if (std::find(it->begin(), it->end(), child) != it->end()) shares_child = true;
            if (shares_child) { group.insert(group.end(), it->begin(), it->end()); it = groups.erase(it); }
            else ++it;
        }
        std::sort(group.begin(), group.end());
        group.erase(std::unique(group.begin(), group.end()), group.end());
        groups.push_back(std::move(group));
    }
    for (auto& group : groups) {
        bool unknown = false;
        for (int i : group) if (nodes[i].status < 0) unknown = true;
        if (unknown) continue;
        std::stable_sort(group.begin(), group.end(), [&](int a, int b) { return nodes[a].x < nodes[b].x; });
        float low_y = nodes[group.front()].y, high_y = low_y;
        for (int i : group) { low_y = std::min(low_y, nodes[i].y); high_y = std::max(high_y, nodes[i].y); }
        const float middle_y = (low_y + high_y) * 0.5f;
        for (std::size_t i = 1; i < group.size(); ++i) {
            const float left = nodes[group[i - 1]].x, right = nodes[group[i]].x;
            const float midpoint = (left + right) * 0.5f;
            bool passed = false;
            for (int child : group) {
                if (nodes[child].y != high_y || nodes[child].status < 1) continue;
                const auto& record = tree->records[child];
                for (int row_index = 0; row_index < record.link_count_0x3D; ++row_index) {
                    const auto* row = tree->Requirement_002008B8(child, row_index);
                    if (!row) continue;
                    const int parent = tree->FindLeafByNumber_001FA840(row->prerequisite_number_0x02);
                    if (parent < 0) continue;
                    if (tree->Status_002010B8(view.profile(), parent, row) >= 2 &&
                        midpoint >= std::min(nodes[child].x, nodes[parent].x) &&
                        midpoint <= std::max(nodes[child].x, nodes[parent].x)) passed = true;
                }
            }
            line(place(left, middle_y), place(right, middle_y),
                    passed ? kWonBranch : kPendingBranch);
            ++stats.connectors;
        }
    }
    const float panel_half_height = panel_template ? sizes.scaled(*panel_template).height * 0.5f : 0.0f;
    // FUN_001F7978 draws vertical halves independently: incoming is green for
    // status >=1, outgoing only for status >=4. Neither depends on selected scale.
    for (const auto& node : nodes) {
        if (node.status < 0 || node.visibility != 1) continue;
        const auto& leaf = tree->leaves[node.index];
        if (!leaf.incoming.empty()) {
            const auto& parent = nodes[leaf.incoming.front()];
            line(place(node.x, node.y - panel_half_height + 5.0f),
                    place(node.x, (node.y + parent.y) * 0.5f),
                    node.status >= 1 ? kWonBranch : kPendingBranch);
        }
        if (!leaf.outgoing.empty() && nodes[leaf.outgoing.front()].visibility == 1) {
            const auto& child = nodes[leaf.outgoing.front()];
            const float edge_inset = view.tree_type() == 1 ? 5.0f : 16.0f;
            line(place(node.x, node.y + panel_half_height - edge_inset),
                    place(node.x, (node.y + child.y) * 0.5f),
                    node.status >= 4 ? kWonBranch : kPendingBranch);
        }
    }

    // FUN_001F7978: authored panel size through the current matrix.
    const auto* numbers = sizes.font(hp2::frontend::StringHash_002534A8("FONT_EVENTNUMBERS"));
    for (const auto& node : nodes) {
        const auto at = place(node.x, node.y);
        const auto panel_hash = node.panel_texture.empty()
            ? 0u : hp2::frontend::StringHash_002534A8(node.panel_texture.c_str());
        if (node.status < 0 || node.visibility != 1) continue; // Reported in status_gaps; no invented panel/state.
        const auto bitmap = panel_hash == 0 ? nullptr : resources.texture(panel_hash);
        if (bitmap && bitmap->width > 0 && bitmap->height > 0) {
            // Authored EVENT_PANEL scale times current zoom, without a fitted factor.
            const float authored_x = panel_template ? panel_template->scale_x : 1.0f;
            const float authored_y = panel_template ? panel_template->scale_y : 1.0f;
            const float width =
                static_cast<float>(bitmap->width) * authored_x * zoom;
            const float height =
                static_cast<float>(bitmap->height) * authored_y * zoom;
            render::Quad quad;
            quad.texture = panel_hash;
            quad.destination = {at.x - width * 0.5f, at.y - height * 0.5f, width, height};
            quad.source = {0, 0, static_cast<float>(bitmap->width), static_cast<float>(bitmap->height)};
            quad.color = {panel_template ? panel_template->r / 128.0f : 1.0f,
                          panel_template ? panel_template->g / 128.0f : 1.0f,
                          panel_template ? panel_template->b / 128.0f : 1.0f,
                          node.selected ? view.alpha() : 1.0f};
            if ((node.selected && view.state() != 0) || render::crop(quad, clip)) frame.quads.push_back(quad);
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

        if (numbers == nullptr || number_template == nullptr) continue;
        hp2::frontend::FeScreen number_screen;
        auto widget = *number_template;
        widget.visible = 1;
        widget.x = at.x; widget.y = at.y;
        widget.scale_x *= zoom; widget.scale_y *= zoom;
        // 001F7BF0 ->00140930 ->00140CD8: centre Y using scaled maximum glyph height.
        // E-0039-N01/F01; the original ignores the template's authored text position.
        widget.y -= static_cast<float>(numbers->line_height) * widget.scale_y * 0.5f;
        widget.string_id_0x60 = 0;
        widget.text = node.number < 10 ? " " + std::to_string(node.number) : std::to_string(node.number);
        // FUN_001F7748: dim when this node and all prerequisites report locked.
        bool dim = node.status == 0;
        for (int i : tree->leaves[node.index].incoming) if (nodes[i].status >= 1) dim = false;
        widget.a = static_cast<std::uint8_t>((dim ? 80.0f : 128.0f) *
                    (node.selected ? view.alpha() : 1.0f));
        widget.clip_hash_0x4c = 0;
        number_screen.widgets.push_back(widget);
        render::Frame number_frame;
        const auto result = build_frame(number_screen, sizes, {}, number_frame);
        for (auto quad : number_frame.quads)
            if ((node.selected && view.state() != 0) || render::crop(quad, clip)) frame.quads.push_back(quad);
        if (result.texts > 0) ++stats.numbers;
    }
    // FUN_001FA098: enlarged bounds (gp-0x4170=1.05), 15% corner runs,
    // pulse from the recovered polynomial sine; phase increments0.2 per render.
    const auto selected = view.cursor_info();
    if (selected.status >= 0 && selected.visibility == 1 && panel_template) {
        const auto dimensions = sizes.scaled(*panel_template);
        const float phase = view.advance_border_phase();
        const auto constant = [](std::uint32_t bits) { float value; std::memcpy(&value,&bits,4); return value; };
        // FUN_0025A5E0/0025A540, E-0022-G06. The wrapper CVT.W.S uses FCSR rounding.
        const auto angle = static_cast<std::uint16_t>(static_cast<std::int32_t>(std::nearbyint(phase * constant(0x4622f983))));
        float x = static_cast<float>(angle) * constant(0x38c90fdb), sign = 1.0f;
        if (x >= constant(0x4096cbe4)) x -= constant(0x40c90fdb);
        else if (x >= constant(0x3fc90fdb)) { x -= constant(0x40490fdb); sign = -1.0f; }
        const float square = x*x;
        const float third = x*square;
        const float fifth = third*square;
        const float seventh = fifth*square;
        const float sine = (((x-third*constant(0x3e2aaaab))+fifth*constant(0x3c088889))-seventh*constant(0x39500d01)+seventh*square*constant(0x3638ef1d))*sign;
        const render::Color white{1,1,1,(sine+1.0f)*0.5f};
        const auto tl = place(selected.x-dimensions.width*0.525f, selected.y-dimensions.height*0.525f);
        const auto br = place(selected.x+dimensions.width*0.525f, selected.y+dimensions.height*0.525f);
        const float dx=(br.x-tl.x)*0.15f, dy=(br.y-tl.y)*0.15f;
        line(tl,{tl.x+dx,tl.y},white); line({br.x-dx,tl.y},{br.x,tl.y},white);
        line({tl.x,br.y},{tl.x+dx,br.y},white); line({br.x-dx,br.y},{br.x+2.0f,br.y},white);
        line(tl,{tl.x,tl.y+dy},white); line({tl.x,br.y-dy},{tl.x,br.y},white);
        line({br.x,tl.y},{br.x,tl.y+dy},white); line({br.x,br.y-dy},br,white);
    }
    return stats;
}

} // namespace port::runtime
