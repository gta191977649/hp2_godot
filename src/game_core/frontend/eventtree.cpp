#include "game_core/frontend/eventtree.hpp"
#include "game_core/not_implemented.hpp"
#include <cstring>

namespace hp2::frontend {
namespace {

std::uint32_t read_u32(const std::uint8_t* d, std::size_t off) {
    return static_cast<std::uint32_t>(d[off]) | (static_cast<std::uint32_t>(d[off + 1]) << 8) |
           (static_cast<std::uint32_t>(d[off + 2]) << 16) |
           (static_cast<std::uint32_t>(d[off + 3]) << 24);
}

std::uint16_t read_u16(const std::uint8_t* d, std::size_t off) {
    return static_cast<std::uint16_t>(d[off] | (d[off + 1] << 8));
}

std::string read_fixed_string(const std::uint8_t* d, std::size_t off, std::size_t max_len) {
    std::size_t len = 0;
    while (len < max_len && d[off + len] != 0) ++len;
    return std::string(reinterpret_cast<const char*>(d + off), len);
}

void collect(const std::vector<hp2::assets::Chunk>& chunks, std::uint32_t tag,
             std::vector<const hp2::assets::Chunk*>& out) {
    for (const auto& chunk : chunks) {
        if (chunk.tag == tag) out.push_back(&chunk);
        if (chunk.is_container()) collect(chunk.children, tag, out);
    }
}

const hp2::assets::Chunk* child(const hp2::assets::Chunk& parent, std::uint32_t tag) {
    for (const auto& c : parent.children) {
        if (c.tag == tag) return &c;
    }
    return nullptr;
}

} // namespace

const char* EventPanelTexture_001F75F0(int tree_type, int status) {
    // FUN_001F75F0: the switch has no arm for any other status, and leaves the texture unset.
    if (tree_type == 0) {
        switch (status) {
            case kEventStatusLocked: return kSpeedPanelLocked_002BA768;
            case kEventStatusUnlocked:
            case kEventStatusUnlocked2:
            case kEventStatusUnlocked3: return kSpeedPanelUnlocked_002BA788;
            case kEventStatusWon: return kSpeedPanelWon_002BA7A8;
            default: return nullptr;
        }
    }
    if (tree_type == 1) {
        switch (status) {
            case kEventStatusLocked: return kPursuitPanelLocked_002BA7C0;
            case kEventStatusUnlocked:
            case kEventStatusUnlocked2:
            case kEventStatusUnlocked3: return kPursuitPanelUnlocked_002BA7E0;
            case kEventStatusWon: return kPursuitPanelWon_002BA800;
            default: return nullptr;
        }
    }
    return nullptr;
}

int EventSaveSize_00200DE8(std::uint8_t goal_type) {
    switch (goal_type) {
        case 0:
        case 2: return 1;
        case 1:
        case 5:
        case 6: return 4;
        case 3:
        case 4: return 0x10;
        default: return 1; // the original's own default arm
    }
}

const char* EventTree_001FFC40::layout_map() const {
    if (type_0x08 == 0) return kLayoutChampionship_0031B3C0;
    if (type_0x08 == 1) return kLayoutHotPursuit_0031B490;
    return nullptr;
}

int EventTree_001FFC40::FindLeafByNumber_001FA840(std::uint8_t number) const {
    for (std::size_t i = 0; i < records.size(); ++i) {
        if (records[i].number_0x28 == number) return static_cast<int>(i);
    }
    return -1;
}

int EventTree_001FFC40::Status_002010B8(int record_index) const {
    if (record_index < 0 || record_index >= static_cast<int>(records.size())) {
        return kEventStatusLocked;
    }
    // FUN_002010B8's structure, with the profile slice unavailable:
    //   * record+0x28 == 0x1F and == 0x20 are special-cased against completion state that only a
    //     profile can supply, so both stay locked here;
    //   * an event with no incoming links (record+0x3D < 1) skips the prerequisite test and falls
    //     straight through to the profile read;
    //   * otherwise at least one incoming link must be satisfied, which again needs the profile.
    // With an all-zero block the profile read yields 0, so the only events that can report
    // anything above Locked are the link-less roots. Reporting more than that would be inventing
    // save data.
    const auto& record = records[static_cast<std::size_t>(record_index)];
    if (record.number_0x28 == 0x1f || record.number_0x28 == 0x20) return kEventStatusLocked;
    if (record.link_count_0x3D < 1) return kEventStatusUnlocked;
    return kEventStatusLocked;
}

std::vector<EventTree_001FFC40> parse_event_trees(const std::vector<std::uint8_t>& buffer) {
    std::vector<EventTree_001FFC40> trees;
    auto chunks = hp2::assets::parse_chunks_top(buffer);
    std::vector<const hp2::assets::Chunk*> containers;
    collect(chunks, kEventTreeChunkId_80034A00, containers);

    for (const auto* container : containers) {
        EventTree_001FFC40 tree;

        if (const auto* name = child(*container, kEventTreeNameChunkId_00000001)) {
            tree.name = read_fixed_string(buffer.data(), name->payload_offset(), name->size);
        }
        // FUN_001FDEF8 compares the parsed name against the four known tree names and stores the
        // type; anything else leaves the tree untyped, which is what -1 means here.
        if (tree.name == "Championship" || tree.name == "Championship E3") tree.type_0x08 = 0;
        if (tree.name == "Hot Pursuit" || tree.name == "Hot Pursuit E3") tree.type_0x08 = 1;

        if (const auto* links = child(*container, kEventTreeChunk_00034A03)) {
            const std::uint8_t* d = buffer.data() + links->payload_offset();
            const std::size_t count = links->size / kEventLinkSize_002008B8;
            tree.links.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                const std::size_t off = i * kEventLinkSize_002008B8;
                EventLink_002008B8 link;
                link.field_0x00 = read_u16(d, off + 0);
                link.target_number_0x02 = read_u16(d, off + 2);
                link.field_0x04 = read_u32(d, off + 4);
                tree.links.push_back(link);
            }
        }

        const auto* blob = child(*container, kEventTreeChunk_00034A05);
        if (blob == nullptr) continue;
        const std::uint8_t* d = buffer.data() + blob->payload_offset();
        const std::size_t count = blob->size / kEventRecordSize_001FFF60;
        tree.records.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            const std::size_t off = i * kEventRecordSize_001FFF60;
            EventRecord_001FFF60 record;
            record.name_0x08 = read_fixed_string(d, off + 0x08, 0x20);
            record.number_0x28 = d[off + 0x28];
            record.goal_type_0x29 = d[off + 0x29];
            record.link_count_0x3D = d[off + 0x3d];
            record.index_0x42 = read_u16(d, off + 0x42);
            record.string_id_0xB0 = read_u32(d, off + 0xb0);
            for (int k = 0; k < static_cast<int>(record.link_count_0x3D); ++k) {
                // FUN_002008B8 indexes a s16 array at record+0x7C and multiplies by the 8-byte
                // link row size.
                const std::size_t slot = off + 0x7c + static_cast<std::size_t>(k) * 2;
                record.link_index_0x7C.push_back(
                    static_cast<std::int16_t>(read_u16(d, slot)));
            }
            tree.records.push_back(std::move(record));
        }

        // --- the view's node graph (FUN_001F96B8)
        tree.leaves.resize(tree.records.size());
        for (std::size_t i = 0; i < tree.records.size(); ++i) {
            tree.leaves[i].record_index = static_cast<int>(i);
        }
        for (std::size_t i = 0; i < tree.records.size(); ++i) {
            for (std::int16_t row : tree.records[i].link_index_0x7C) {
                if (row < 0 || static_cast<std::size_t>(row) >= tree.links.size()) continue;
                const int other = tree.FindLeafByNumber_001FA840(
                    static_cast<std::uint8_t>(tree.links[static_cast<std::size_t>(row)]
                                                  .target_number_0x02));
                if (other < 0) continue;
                tree.leaves[i].incoming.push_back(other);
                tree.leaves[static_cast<std::size_t>(other)].outgoing.push_back(
                    static_cast<int>(i));
            }
        }

        // Depth: FUN_001F7FA8 follows the first incoming link back to a root and counts the hops.
        for (auto& leaf : tree.leaves) {
            int depth = 0;
            int walk = leaf.record_index;
            while (!tree.leaves[static_cast<std::size_t>(walk)].incoming.empty() &&
                   depth <= static_cast<int>(tree.leaves.size())) {
                walk = tree.leaves[static_cast<std::size_t>(walk)].incoming.front();
                ++depth;
            }
            leaf.depth_0x1C = depth;
            leaf.y = static_cast<float>(depth) * kRowSpacing_gp_0x41B4;
        }

        // X: walk the ASCII map counting non-space characters until the event number is reached,
        // then take 130.0f * (position % 20).
        if (const char* map = tree.layout_map()) {
            const std::size_t length = std::strlen(map);
            for (auto& leaf : tree.leaves) {
                int remaining = tree.records[static_cast<std::size_t>(leaf.record_index)].number_0x28;
                for (std::size_t pos = 0; pos < length; ++pos) {
                    if (map[pos] != ' ') --remaining;
                    if (remaining == 0) {
                        leaf.x = kColumnSpacing_gp_0x41B0 *
                                 static_cast<float>(pos % kLayoutColumns_001F96B8);
                        break;
                    }
                }
            }
        }

        // Groups: FUN_001F96B8 puts every leaf reachable through the link lists into one
        // `tEventGroup`, i.e. the connected components of the undirected graph.
        int group = 0;
        for (auto& leaf : tree.leaves) {
            if (leaf.group != -1) continue;
            std::vector<int> stack{leaf.record_index};
            while (!stack.empty()) {
                const int at = stack.back();
                stack.pop_back();
                auto& node = tree.leaves[static_cast<std::size_t>(at)];
                if (node.group != -1) continue;
                node.group = group;
                for (int next : node.incoming) stack.push_back(next);
                for (int next : node.outgoing) stack.push_back(next);
            }
            ++group;
        }

        trees.push_back(std::move(tree));
    }
    return trees;
}

EventProfileLayout_001FE988 build_profile_layout_001FE988(const EventTree_001FFC40& tree) {
    // FUN_001FE988 makes TWO passes over the records, split on whether an event's slice is
    // smaller than the alignment:
    //   pass 1 takes every event whose size is >= 4 and rounds the cursor up to 4 after it;
    //   pass 2 takes the 1-byte events and packs them with no rounding at all.
    // An event numbered 0x20 reuses the slice cached in the global DAT_002FC3D8 instead of taking
    // one of its own. That global is shared between the two trees in the original; this function
    // models it per tree, which is the only scope a single call can speak for.
    EventProfileLayout_001FE988 layout;
    layout.offsets.assign(tree.records.size(), -1);
    int used = 0;
    int shared = -1;

    auto assign = [&](std::size_t i, bool rounded) {
        const auto& record = tree.records[i];
        const int size = EventSaveSize_00200DE8(record.goal_type_0x29);
        if (record.number_0x28 == 0x20 && shared >= 0) {
            layout.offsets[i] = shared;
            return;
        }
        layout.offsets[i] = used;
        used = rounded ? ((used + size - 1 + layout.kAlignment) / layout.kAlignment) *
                             layout.kAlignment
                       : used + size;
        if (record.number_0x28 == 0x20) shared = layout.offsets[i];
    };

    for (std::size_t i = 0; i < tree.records.size(); ++i) {
        if (EventSaveSize_00200DE8(tree.records[i].goal_type_0x29) >= layout.kAlignment) {
            assign(i, true);
        }
    }
    for (std::size_t i = 0; i < tree.records.size(); ++i) {
        if (EventSaveSize_00200DE8(tree.records[i].goal_type_0x29) < layout.kAlignment) {
            assign(i, false);
        }
    }

    layout.used = used;
    if (used > static_cast<int>(layout.kBlockSize)) {
        HP2_NOT_IMPLEMENTED("0x001FE988",
                            "event profile slices overflow the 0x100-byte block, which the "
                            "original's two passes are sized to avoid");
    }
    return layout;
}

} // namespace hp2::frontend
