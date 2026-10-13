#include "port/runtime/event_tree_view.hpp"
#include <algorithm>
#include <cmath>

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
    selected_tree_ = -1;
    cursor_ = -1;
}

const hp2::frontend::EventTree_001FFC40* EventTreeView::tree() const {
    if (selected_tree_ < 0 || selected_tree_ >= static_cast<int>(trees_.size())) return nullptr;
    return &trees_[static_cast<std::size_t>(selected_tree_)];
}

EventTreeLoadResult EventTreeView::load(assets::Source& source, const std::string& relative_path,
                                        int mode_id) {
    EventTreeLoadResult result;
    clear();
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
    // FUN_001F96B8 takes the starting node from the profile (FUN_001F5D00). Without a profile the
    // port starts on event 1, which is the only node that is unlocked in that state.
    cursor_ = selected->FindLeafByNumber_001FA840(1);
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
    node.status = selected->Status_002010B8(static_cast<int>(index));
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

bool EventTreeView::move_cursor(int direction) {
    const auto* selected = tree();
    if (selected == nullptr || cursor_ < 0) return false;
    const auto& leaf = selected->leaves[static_cast<std::size_t>(cursor_)];
    // FUN_001FA9C8: -1/+1 take the node's own link list, +2 the one the view threads forward and
    // -2 the one it threads back. The port models those three lists as the node's outgoing and
    // incoming edges; which concrete neighbour each ordinal picks within a list is the order the
    // view built them in.
    const std::vector<int>* list = nullptr;
    int step = 0;
    if (direction == -1 || direction == 1) {
        list = &leaf.outgoing;
        step = direction;
    } else if (direction == 2) {
        list = &leaf.outgoing;
        step = 1;
    } else if (direction == -2) {
        list = &leaf.incoming;
        step = 1;
    }
    if (list == nullptr || list->empty()) return false;

    int target = -1;
    if (direction == -1 || direction == 1) {
        // Walk the siblings that share this node's depth, in the order the list holds them.
        const int at = cursor_;
        std::vector<int> row;
        for (std::size_t i = 0; i < selected->leaves.size(); ++i) {
            if (selected->leaves[i].depth_0x1C == leaf.depth_0x1C) row.push_back(static_cast<int>(i));
        }
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (row[i] != at) continue;
            const int next = static_cast<int>(i) + step;
            if (next >= 0 && next < static_cast<int>(row.size()))
                target = row[static_cast<std::size_t>(next)];
            break;
        }
    } else {
        target = (*list)[0];
    }
    if (target < 0 || target == cursor_) return false;
    cursor_ = target;
    return true;
}

EventTreeOverlayStats append_event_tree(const EventTreeView& view, const WidgetSizes& sizes,
                                        render::Resources& resources, render::Frame& frame) {
    EventTreeOverlayStats stats;
    const auto nodes = view.nodes();
    if (nodes.empty()) return stats;

    const render::Vec2 centre{frame.logical_size.x * 0.5f, frame.logical_size.y * 0.5f};
    render::Vec2 focus{0, 0};
    for (const auto& node : nodes) {
        if (node.selected) focus = {node.x, node.y};
    }
    const auto place = [&](float x, float y) {
        return render::Vec2{centre.x + (x - focus.x) * kPortZoom,
                            centre.y + (y - focus.y) * kPortZoom};
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
            const float width = static_cast<float>(bitmap->width) * kPortPanelScale * node.scale;
            const float height = static_cast<float>(bitmap->height) * kPortPanelScale * node.scale;
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
