// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\eventtree.cpp`
// (string at 0x002BA9F8, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, not the original translation unit.
#include "game_core/indep/src/eventtree.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <algorithm>

namespace hp2::frontend {
namespace {

// Port-side bounds checks around recovered slice readers; not original failure behavior.
namespace port {
const std::uint8_t* profile_slice(const EventTree_001FFC40& tree,
                                const hp2::memorycard::Profile_002FC050& profile, int index) {
    const auto layout = build_profile_layout_001FE988(tree);
    const auto* block = profile.EventBlock_001FE988(tree.type_0x08);
    if (!block || index < 0 || static_cast<std::size_t>(index) >= layout.offsets.size())
        throw std::out_of_range("invalid event profile slice");
    const auto offset = layout.offsets[static_cast<std::size_t>(index)];
    if (offset < 0 || offset + EventSaveSize_00200DE8(tree.records[index].goal_type_0x29) > 256)
        throw std::out_of_range("event profile slice exceeds block");
    return block + offset;
}
} // namespace port

int Sub_002001A8(std::uint32_t value) { // E-0022-D02, ELF 002001A8
    int count = 0;
    for (int i = 0; i < 8; ++i) count += ((value >> (i * 4)) & 15) != 0;
    return count;
}
int Sub_002006A0(const std::uint8_t* bytes, int count) { // E-0022-D02, ELF 002006A0
    constexpr std::uint8_t points_003317E8[8] = {10, 6, 4, 3, 2, 1, 0, 0};
    if (count < 0 || count >= 8)
        hp2::not_implemented("0x002006A0", "point-count table index outside recovered eight bytes");
    int divisor = 0, sum = 0;
    for (int i = 0; i <= count; ++i) divisor += points_003317E8[i];
    for (int i = 0; i < 8; ++i) sum += bytes[i];
    return sum / divisor;
}
int Sub_00200710(const std::uint8_t* bytes, int index) { // E-0022-D02, ELF 00200710
    int rank = 1;
    for (int i = 0; i < 8; ++i)
        if (i != index && (bytes[i] > bytes[index] || (bytes[i] == bytes[index] && i < index)))
            ++rank;
    return rank;
}

// Port-side safe representation of the recovered sprintf calls. Formats in the
// language data are parsed rather than forwarded as a variadic C format string.
namespace port {
std::string format_operands(const std::string& format, const std::vector<std::string>& operands) {
    std::string out;
    std::size_t operand = 0;
    for (std::size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%') { out += format[i]; continue; }
        if (++i == format.size()) throw std::runtime_error("truncated event text format");
        if (format[i] == '%') { out += '%'; continue; }
        if ((format[i] != 'd' && format[i] != 'i' && format[i] != 's') || operand >= operands.size())
            hp2::not_implemented("0x00251398", "event localized format conversion not implemented");
        out += operands[operand++];
    }
    return out;
}
} // namespace port

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

int EventTree_001FFC40::StatusWithoutProfile_002010B8(int record_index) const {
    if (record_index < 0 || record_index >= static_cast<int>(records.size())) {
        return kEventStatusLocked;
    }
    // TASK-0018's reduction, retained for callers that hold no profile. An event with no
    // requirement rows reports Unlocked, everything else Locked.
    const auto& record = records[static_cast<std::size_t>(record_index)];
    if (record.number_0x28 == 0x1f || record.number_0x28 == 0x20) return kEventStatusLocked;
    if (record.link_count_0x3D < 1) return kEventStatusUnlocked;
    return kEventStatusLocked;
}

const EventLink_002008B8* EventTree_001FFC40::Requirement_002008B8(int record_index, int i) const {
    // FUN_002008B8, verbatim: the bound is record+0x3D and the row index comes from the s16
    // array at record+0x7C. The original returns 0 for an out-of-range `i`.
    if (record_index < 0 || record_index >= static_cast<int>(records.size())) return nullptr;
    const auto& record = records[static_cast<std::size_t>(record_index)];
    if (i < 0 || i >= static_cast<int>(record.link_count_0x3D)) return nullptr;
    if (static_cast<std::size_t>(i) >= record.link_index_0x7C.size()) return nullptr;
    const std::int16_t row = record.link_index_0x7C[static_cast<std::size_t>(i)];
    if (row < 0 || static_cast<std::size_t>(row) >= links.size()) return nullptr;
    return &links[static_cast<std::size_t>(row)];
}

int EventTree_001FFC40::Status_002010B8(const hp2::memorycard::Profile_002FC050& profile,
                                        int record_index,
                                        const EventLink_002008B8* row) const {
    if (record_index < 0 || record_index >= static_cast<int>(records.size())) {
        return kEventStatusLocked;
    }
    const auto& record = records[static_cast<std::size_t>(record_index)];

    // FUN_002010B8 opens with `if (iGpffffbeb0 == 0)` and answers 4 for every event otherwise.
    // That global is an unlock-all debug switch whose producer is not traced, so the port holds
    // it at its released value, zero, and the branch is not reachable here. Recorded as
    // TASK-0021 unresolved question 2 rather than exposed as a port-side cheat.

    // The two special cases. `bVar3` in the decompile is event 0x1F's gate; the original ORs it
    // into the final answer, so a locked 0x1F still reports 1 when either completion global is
    // set. Neither global is recovered, so reaching this at all is an untranslated path.
    if (record.number_0x28 == 0x1f) {
        hp2::not_implemented("0x002010B8",
                             "event 31's gate reads DAT_0031D7F4+0x180 / +0x1B8, which are not "
                             "recovered");
    }

    if (row == nullptr) {
        // The original's `param_2 == 0` entry: run the prerequisite test, then pick this
        // record's own requirement row from record+0x42 and judge the slice against it.
        if (record.number_0x28 == 0x20) {
            hp2::not_implemented("0x002010B8",
                                 "event 32 requires events 1..31 of BOTH trees to be Won, which "
                                 "needs the other tree's view; not translated");
        }
        if (record.link_count_0x3D >= 1) {
            bool satisfied = false;
            for (int i = 0; i < static_cast<int>(record.link_count_0x3D); ++i) {
                const EventLink_002008B8* requirement = Requirement_002008B8(record_index, i);
                if (requirement == nullptr) continue;
                // FUN_001FFFB0(row, tree): look the prerequisite up by number and ask for its
                // status judged against THIS row's threshold.
                const int prerequisite = FindLeafByNumber_001FA840(
                    static_cast<std::uint8_t>(requirement->prerequisite_number_0x02));
                if (prerequisite < 0) continue;
                if (Status_002010B8(profile, prerequisite, requirement) == kEventStatusWon) {
                    satisfied = true;
                }
            }
            if (!satisfied) return kEventStatusLocked;
        }
        if (static_cast<std::size_t>(record.index_0x42) >= links.size()) return kEventStatusLocked;
        row = &links[static_cast<std::size_t>(record.index_0x42)];
    }

    // The GoalType switch on record+0x29. (E-0021-11)
    const std::uint8_t* block = profile.EventBlock_001FE988(type_0x08);
    if (block == nullptr) return kEventStatusLocked;
    const EventProfileLayout_001FE988 layout = build_profile_layout_001FE988(*this);
    if (static_cast<std::size_t>(record_index) >= layout.offsets.size()) {
        return kEventStatusLocked;
    }
    const int slice_offset = layout.offsets[static_cast<std::size_t>(record_index)];
    if (slice_offset < 0 ||
        slice_offset + EventSaveSize_00200DE8(record.goal_type_0x29) >
            static_cast<int>(hp2::memorycard::kEventBlockSize_001FE988)) {
        return kEventStatusLocked;
    }
    const std::uint8_t* slice = block + slice_offset;

    switch (record.goal_type_0x29) {
    case 0:
    case 2:
        return hp2::memorycard::SliceStatusByte_00200018(slice[0], row->goal_threshold_0x04);
    case 1:
    case 5:
    case 6:
        return hp2::memorycard::SliceStatusWord_00200068(
            static_cast<std::int32_t>(read_u32(slice, 0)),
            static_cast<std::int32_t>(row->goal_threshold_0x04));
    case 3:
        // E-0022-D02, 002003F8: any nonzero nibble means progress; low nibble is result.
        if (Sub_002001A8(read_u32(slice, 0)) == 0) return 1;
        if ((slice[0] & 15) == 0) return 2;
        return (slice[0] & 15) <= row->goal_threshold_0x04 ? 4 : 3;
    case 4:
        // E-0022-D02, 002007D0: completed rounds from sum/points, result from ranking.
        {
            const int progress = Sub_002006A0(slice, record.Field_0x3F);
            if (progress == 0) return 1;
            if (progress < record.Field_0x3C) return 2;
            return static_cast<std::uint32_t>(Sub_00200710(slice, 0)) <= row->goal_threshold_0x04 ? 4 : 3;
        }
    default:
        // The original's switch has no default arm, so `uVar7` keeps its initialiser, 1.
        return kEventStatusUnlocked;
    }
}

int EventTree_001FFC40::WalkSameDepth_001FA880(int record_index, int direction) const {
    // FUN_001FA880, verbatim. It walks the view's whole leaf list, skips the current leaf, keeps
    // only leaves whose +0x1C (depth) equals the current leaf's, and tracks the nearest X on the
    // requested side. The initial "best" values are the literals at gp-0x6CBC and gp-0x6CB8, and
    // the comparisons are strict, so an exact X tie is not a candidate and there is no wrap.
    if (record_index < 0 || record_index >= static_cast<int>(leaves.size())) return -1;
    if (direction != -1 && direction != 1) return -1;
    const auto& current = leaves[static_cast<std::size_t>(record_index)];

    float best = direction == -1 ? kWalkWorstLeft_gp_0x6CBC : kWalkWorstRight_gp_0x6CB8;
    int found = -1;
    for (std::size_t i = 0; i < leaves.size(); ++i) {
        if (static_cast<int>(i) == record_index) continue;
        const auto& candidate = leaves[i];
        if (candidate.depth_0x1C != current.depth_0x1C) continue;
        if (direction == -1) {
            if (candidate.x < current.x && best < candidate.x) {
                best = candidate.x;
                found = static_cast<int>(i);
            }
        } else {
            if (current.x < candidate.x && candidate.x < best) {
                best = candidate.x;
                found = static_cast<int>(i);
            }
        }
    }
    return found;
}

int EventTree_001FFC40::WalkAdjacent_001FA978(int record_index, int direction) const {
    // FUN_001FA978, verbatim: the candidate with the smallest |x - current.x| on the chosen
    // adjacency list, starting from gp-0x6CB4 == 999999.0f, strict comparison so the first of
    // equal distances wins. FUN_001FA9C8 passes leaf+0x20 for +2 and leaf+0x28 for -2;
    // FUN_001F96B8 threads +0x20 with this leaf's prerequisites and +0x28 with the leaves that
    // require it, so +2 walks towards the root.
    if (record_index < 0 || record_index >= static_cast<int>(leaves.size())) return -1;
    if (direction != 2 && direction != -2) return -1;
    const auto& current = leaves[static_cast<std::size_t>(record_index)];
    const std::vector<int>& list = direction == 2 ? current.incoming : current.outgoing;

    float best = kWalkWorstDistance_gp_0x6CB4;
    int found = -1;
    for (int candidate : list) {
        if (candidate < 0 || candidate >= static_cast<int>(leaves.size())) continue;
        const float distance =
            std::fabs(leaves[static_cast<std::size_t>(candidate)].x - current.x);
        if (distance < best) {
            best = distance;
            found = candidate;
        }
    }
    return found;
}

int EventTree_001FFC40::MoveCursor_001FA9C8(int record_index, int direction) const {
    // FUN_001FA9C8's own dispatch, including its quirk: the switch tests -1 first, then <0 for
    // -2, then 1, then 2, and the -1 arm is also what an unrecognised positive value falls into.
    // Only the four values the handler ever passes are modelled.
    if (direction == -1 || direction == 1) return WalkSameDepth_001FA880(record_index, direction);
    if (direction == 2 || direction == -2) return WalkAdjacent_001FA978(record_index, direction);
    return -1;
}

std::uint32_t TitleStringId_001F96B8(int selected_mode) {
    // FUN_001F96B8's three-way on DAT_0030C77C. (E-0021-16)
    if (selected_mode == 8) return kTitleStringChampionship_001F96B8;
    if (selected_mode == 9) return kTitleStringHotPursuit_001F96B8;
    return 0;
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

        if (const auto* rows = child(*container, kEventTreeChunk_00034A01)) {
            const auto* d = buffer.data() + rows->payload_offset();
            for (std::size_t off = 0; off + 4 <= rows->size; off += 4)
                tree.Field_0x14.push_back(read_u32(d, off));
        }

        if (const auto* rows = child(*container, kEventTreeChunk_00034A02)) {
            const auto* d = buffer.data() + rows->payload_offset();
            for (std::size_t off = 0; off + 4 <= rows->size; off += 4)
                tree.Field_0x1C.push_back(read_u32(d, off));
        }
        if (const auto* rows = child(*container, kEventTreeChunk_00034A04)) {
            const auto* d = buffer.data() + rows->payload_offset();
            for (std::size_t off = 0; off + 8 <= rows->size; off += 8) {
                std::array<std::uint8_t, 8> row{};
                std::memcpy(row.data(), d + off, 8);
                tree.Field_0x2C.push_back(row);
            }
        }

        if (const auto* links = child(*container, kEventTreeChunk_00034A03)) {
            const std::uint8_t* d = buffer.data() + links->payload_offset();
            const std::size_t count = links->size / kEventLinkSize_002008B8;
            tree.links.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                const std::size_t off = i * kEventLinkSize_002008B8;
                EventLink_002008B8 link;
                link.field_0x00 = read_u16(d, off + 0);
                link.prerequisite_number_0x02 = read_u16(d, off + 2);
                link.goal_threshold_0x04 = read_u32(d, off + 4);
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
            record.string_id_0xB4 = read_u32(d, off + 0xb4);
            record.Field_0x3B = d[off + 0x3b];
            for (int k = 0; k < record.Field_0x3B; ++k) {
                if (0x44 + k * 2 + 2 > static_cast<int>(kEventRecordSize_001FFF60))
                    throw std::runtime_error("event car indices exceed record bounds");
                record.Field_0x44.push_back(static_cast<std::int16_t>(read_u16(d, off + 0x44 + k * 2)));
            }
            record.Field_0x3C = d[off + 0x3c];
            record.Field_0x3E = d[off + 0x3e];
            record.Field_0x3F = d[off + 0x3f];
            for (int k = 0; k < record.Field_0x3C; ++k) {
                if (k >= 8) throw std::runtime_error("event track indices overlap requirement indices");
                record.Field_0x6C.push_back(static_cast<std::int16_t>(read_u16(d, off + 0x6c + k * 2)));
            }
            for (int k = 0; k < record.Field_0x3E; ++k) {
                if (0x8c + k * 2 + 2 > static_cast<int>(kEventRecordSize_001FFF60))
                    throw std::runtime_error("event reward indices exceed record bounds");
                record.Field_0x8C.push_back(static_cast<std::int16_t>(read_u16(d, off + 0x8c + k * 2)));
            }
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
                                                  .prerequisite_number_0x02));
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
        hp2::not_implemented("0x001FE988",
                            "event profile slices overflow the 0x100-byte block, which the "
                            "original's two passes are sized to avoid");
    }
    return layout;
}

const std::uint32_t* EventTree_001FFC40::Sub_002008F8(int record_index, int i) const {
    const auto& record = records.at(static_cast<std::size_t>(record_index));
    if (i < 0 || i >= record.Field_0x3C) return nullptr;
    const auto row = record.Field_0x6C.at(static_cast<std::size_t>(i));
    if (row < 0 || static_cast<std::size_t>(row) >= Field_0x1C.size())
        throw std::out_of_range("event track row outside table");
    return &Field_0x1C[static_cast<std::size_t>(row)];
}
const std::uint32_t* EventTree_001FFC40::Sub_00200878(int record_index, int i) const {
    const auto& record = records.at(static_cast<std::size_t>(record_index));
    if (i < 0 || i >= record.Field_0x3B) return nullptr;
    const auto row = record.Field_0x44.at(static_cast<std::size_t>(i));
    if (row < 0 || static_cast<std::size_t>(row) >= Field_0x14.size())
        throw std::out_of_range("event car row outside table");
    return &Field_0x14[static_cast<std::size_t>(row)];
}
const std::array<std::uint8_t, 8>* EventTree_001FFC40::Sub_00200938(int record_index, int i) const {
    const auto& record = records.at(static_cast<std::size_t>(record_index));
    if (i < 0 || i >= record.Field_0x3E) return nullptr;
    const auto row = record.Field_0x8C.at(static_cast<std::size_t>(i));
    if (row < 0 || static_cast<std::size_t>(row) >= Field_0x2C.size())
        throw std::out_of_range("event reward row outside table");
    return &Field_0x2C[static_cast<std::size_t>(row)];
}
int EventTree_001FFC40::Sub_002012D8(const hp2::memorycard::Profile_002FC050& profile,
                                  int record_index) const {
    const auto& record = records.at(static_cast<std::size_t>(record_index));
    if (record.goal_type_0x29 == 3)
        return Sub_002001A8(read_u32(port::profile_slice(*this, profile, record_index), 0));
    if (record.goal_type_0x29 == 4)
        return Sub_002006A0(port::profile_slice(*this, profile, record_index), record.Field_0x3F);
    return 0;
}
int Sub_00201350(const EventRecord_001FFF60& record) {
    if (record.goal_type_0x29 == 3) return record.Field_0x3F;
    if (record.goal_type_0x29 == 4) return record.Field_0x3C;
    return 1;
}
std::string Sub_001A3B98(std::int32_t value, int locale, unsigned flags) {
    // E-0022-D07. Integer ticks are 1/4000 second; INT_MAX and zero format as zero.
    if (value == INT32_MIN)
        hp2::not_implemented("0x001A3B98", "INT_MIN timer negation behavior not modeled");
    int hours = 0, minutes = 0, seconds = 0, hundredths = 0;
    if (value != 0 && value != INT32_MAX) {
        if (value < 0) { value = -value; flags |= 1; }
        const int whole_seconds = value / 4000;
        hours = whole_seconds / 3600;
        minutes = (whole_seconds % 3600) / 60;
        seconds = whole_seconds % 60;
        hundredths = (value % 4000) / 40;
    }
    const bool quotes = locale == 1 || locale == 3;
    const bool colon = locale == 2 || locale == 4 || locale == 5;
    std::string out;
    auto digits = [&](std::size_t offset, int number, bool zeros) {
        for (int pos = 1; pos >= 0; --pos) {
            out[offset + static_cast<std::size_t>(pos)] =
                (!zeros && number % 10 == 0 && number <= 9) ? ' ' : char('0' + number % 10);
            number /= 10;
        }
    };
    int leading = 0;
    if (hours) {
        out = quotes ? "--h:--'--\"" : "--h:--:--";
        digits(0, hours, false); digits(4, minutes, true); digits(7, seconds, true);
        leading = hours;
    } else if (minutes) {
        out = quotes ? "--'--\"--" : colon ? "--:--:--" : "--:--.--";
        digits(0, minutes, false); digits(3, seconds, true); digits(6, hundredths, true);
        leading = minutes;
    } else {
        out = seconds ? (quotes ? "   --\"--" : colon ? "   --:--" : "   --.--")
                      : (quotes ? "    0\"--" : colon ? "    0:--" : "    0.--");
        if (seconds) digits(3, seconds, false);
        digits(6, hundredths, true);
    }
    if ((flags & 3) && leading > 9) out = " " + out.substr(0, out.size() - 1);
    if (flags & 1) out[0] = '-'; else if (flags & 2) out[0] = '+';
    return out;
}
std::uint32_t Sub_001F60E8(int status) {
    if (status < 0 || status > 4) throw std::out_of_range("event status outside original text table");
    return 0xf230u + static_cast<std::uint32_t>(status);
}
std::string Sub_001F60E8(int status, const std::string& format, int progress, int total,
                       const std::string& result) {
    (void)Sub_001F60E8(status);
    if (status <= 1) return format;
    if (status == 2) return port::format_operands(format, {std::to_string(progress), std::to_string(total)});
    return port::format_operands(format, {result});
}
std::string Sub_001F6220(const std::array<std::uint8_t, 8>* reward,
                       const std::string& localized_101D8,
                       const std::function<std::string(int)>& car_name,
                       const std::function<std::string(int, int)>& track_name) {
    if (!reward) return localized_101D8;
    if ((*reward)[0] == 0) {
        if (!car_name) hp2::not_implemented("0x00187EB8", "original car database name lookup missing");
        return car_name((*reward)[1]);
    }
    if ((*reward)[0] == 1) {
        if (!track_name) hp2::not_implemented("0x001DC5A8", "original track database name lookup missing");
        return track_name((*reward)[1], (*reward)[2]);
    }
    hp2::not_implemented("0x001F6220", "reward kind has no text-producing arm in original");
}
std::string Sub_001F5F88(const EventTree_001FFC40& tree, int record_index,
                       const std::string& source,
                       const std::function<std::string(std::int32_t)>& timer) {
    constexpr const char* tokens_0031B200[8] = {"$TARGET_TIME", "$NUM_LAPS", "$NUM_OPPONENTS",
        "$NUM_RACES", "$EVENTS_UNLOCKED", "$COMPLETION_TIME", "$EVENT_UNLOCKED", "$TRACK_NAME"};
    const auto& record = tree.records.at(static_cast<std::size_t>(record_index));
    std::string out;
    for (std::size_t i = 0; i < source.size() && out.size() < 512;) {
        int token = -1;
        if (source[i] == '$')
            for (int j = 0; j < 8; ++j)
                if (source.compare(i, std::strlen(tokens_0031B200[j]), tokens_0031B200[j]) == 0) {
                    token = j; break;
                }
        if (token < 0) { out += source[i++]; continue; }
        i += std::strlen(tokens_0031B200[token]);
        std::string substitution;
        switch (token) {
        case 0: {
            const auto& row = tree.links.at(record.index_0x42);
            if (row.field_0x00 == 2) {
                if (!timer) hp2::not_implemented("0x001A3B98", "original timer formatter missing");
                substitution = timer(static_cast<std::int32_t>(row.goal_threshold_0x04));
                const auto first = substitution.find_first_not_of(' ');
                substitution = first == std::string::npos ? "" : substitution.substr(first);
            } else substitution = "zz:zz";
            break;
        }
        case 1: {
            const auto* row = tree.Sub_002008F8(record_index, 0);
            if (!row) hp2::not_implemented("0x001F5DD0", "NUM_LAPS requires first track row");
            substitution = std::to_string((*row >> 14) & 15); break;
        }
        case 2: substitution = std::to_string(record.Field_0x3F); break;
        case 3: substitution = std::to_string(Sub_00201350(record)); break;
        case 4: substitution = "events x,y,z are Unlocked!"; break;
        case 5: substitution = "ff:ggg"; break;
        case 6: substitution = "event x is unlocked"; break;
        case 7: substitution = "TRACK_NAME_HERE"; break;
        }
        out.append(substitution, 0, 512 - out.size());
    }
    return out;
}
std::string Sub_00200E78(const EventTree_001FFC40& tree,
                       const hp2::memorycard::Profile_002FC050& profile, int record_index,
                       int status, const std::function<std::string(std::uint32_t)>& localize,
                       const std::function<std::string(std::int32_t)>& timer) {
    if (status < 3) return "";
    const auto& record = tree.records.at(static_cast<std::size_t>(record_index));
    const auto* slice = port::profile_slice(tree, profile, record_index);
    int result = 0;
    switch (record.goal_type_0x29) {
    case 0: case 2: result = slice[0]; break;
    case 1: case 5: case 6:
        if (!timer) hp2::not_implemented("0x001A3B98", "original timer formatter missing");
        return timer(static_cast<std::int32_t>(read_u32(slice, 0)));
    case 3: result = slice[0] & 15; break; // 00200220 case 0, listing 00200248..250
    case 4: result = Sub_00200710(slice, 0); break; // 00200768 delegates with index 0
    default: hp2::not_implemented("0x00200E78", "goal type has no result-text arm in original");
    }
    // The table is nine words in the original: placing 0..8.
    if (result > 8) hp2::not_implemented("0x00200E78", "result index beyond nine recovered placing text IDs");
    if (!localize) hp2::not_implemented("0x0021E560", "result localization lookup missing");
    return localize(0xee48u + static_cast<std::uint32_t>(result));
}

void tCarLogoAnimation::Bind_001F8C10(FeScreen& screen, std::uint32_t now_ticks) {
    Field_0x00 = nullptr;
    for (auto& widget : screen.widgets)
        if (widget.name_hash == 0xc6398c48u) { Field_0x00 = &widget; break; }
    if (!Field_0x00) hp2::not_implemented("0x001F8C10", "car logo widget C6398C48 missing");
    Field_0x04 = now_ticks;
    Field_0x08.clear();
    Field_0x2C = Field_0x30 = Field_0x38 = Field_0x3C = 0;
    Field_0x34 = Field_0x00->a;
}
void tCarLogoAnimation::Sub_001F8D20(bool randomize, std::uint32_t& rng) {
    if (!Field_0x00) hp2::not_implemented("0x001F8D20", "car logo animator not bound");
    if (Field_0x2C < static_cast<int>(Field_0x08.size()))
        Field_0x00->texture_hash = Field_0x08[static_cast<std::size_t>(Field_0x2C)];
    if (randomize) {
        Field_0x38 = (Field_0x38 + static_cast<int>(bRandom(3, rng)) + 1) % 4;
        Field_0x3C = (Field_0x3C + static_cast<int>(bRandom(2, rng))) & 1;
    }
    constexpr float x_0031B5B0[4] = {70, 150, 210, 260};
    constexpr float y_00331760[2] = {140, 280};
    Field_0x00->x = x_0031B5B0[Field_0x38];
    Field_0x00->y = y_00331760[Field_0x3C];
    Field_0x00->dirty_0x2b = 1;
}
void tCarLogoAnimation::Sub_001F8E18(const std::vector<std::uint32_t>& logos,
                               std::uint32_t now_ticks) {
    Field_0x08.assign(logos.begin(), logos.begin() + static_cast<std::ptrdiff_t>(std::min<std::size_t>(8, logos.size())));
    Field_0x2C = Field_0x38 = Field_0x3C = 0;
    Field_0x30 = 1;
    std::uint32_t unused = 0;
    Sub_001F8D20(false, unused);
    Field_0x04 = now_ticks - 22000u; // 00136DD0(5.5) at 4000 ticks/second
    Field_0x00->visible = 1; // 001F8CF0
}
void tCarLogoAnimation::Sub_001F8FB0(std::uint32_t now_ticks, std::uint32_t& rng) {
    if (!Field_0x00) hp2::not_implemented("0x001F8FB0", "car logo animator not bound");
    const float elapsed = static_cast<float>(static_cast<std::int32_t>(now_ticks - Field_0x04)) * 0.00025f;
    if (elapsed > 5.0f) {
        if (elapsed < 5.5f) {
            const float fraction = (elapsed - 5.0f) / 0.5f; // 001F8F68
            Field_0x00->a = static_cast<std::uint8_t>(static_cast<int>(Field_0x34 * (1.0f - fraction)));
            Field_0x30 = 0;
        } else if (elapsed < 6.0f) {
            if (Field_0x30 == 0) {
                if (++Field_0x2C >= static_cast<int>(Field_0x08.size())) Field_0x2C = 0;
                Sub_001F8D20(true, rng);
                Field_0x30 = 1;
            }
            const float fraction = (elapsed - 5.5f) / 0.5f;
            Field_0x00->a = static_cast<std::uint8_t>(static_cast<int>(Field_0x34 * fraction));
        } else Field_0x04 = now_ticks;
        Field_0x00->dirty_0x2b = 1;
    }
}
void tCarLogoAnimation::Sub_001F8CF0() { if (Field_0x00) Field_0x00->visible = 1; }
void tCarLogoAnimation::Sub_001F8D08() { if (Field_0x00) Field_0x00->visible = 0; }
void tTrackLogoAnimation::Bind_001F90F0(FeScreen& screen, std::uint32_t now_ticks) {
    Field_0x00 = nullptr;
    for (auto& widget : screen.widgets)
        if (widget.name_hash == 0xf60114e4u) { Field_0x00 = &widget; break; }
    if (!Field_0x00) hp2::not_implemented("0x001F90F0", "track logo widget F60114E4 missing");
    Field_0x40 = Field_0x00->x;
    Field_0x48 = Field_0x00->a;
    Field_0x44 = 0; // constructor's original clock field is zero, not current time
    Field_0x4C = false;
    Field_0x38 = 0;
    Field_0x04.clear();
    Field_0x00->visible = 1; // 001F91D0
    (void)now_ticks;
}
void tTrackLogoAnimation::Sub_001F91F0(const std::vector<std::uint32_t>& logos,
                               std::uint32_t now_ticks) {
    if (!Field_0x00) hp2::not_implemented("0x001F91F0", "track logo animator not bound");
    Field_0x04.assign(logos.begin(), logos.begin() + static_cast<std::ptrdiff_t>(std::min<std::size_t>(12, logos.size())));
    const float elapsed = static_cast<float>(static_cast<std::int32_t>(now_ticks - Field_0x44)) * 0.00025f;
    if (elapsed < 0.35f) Field_0x00->x = 640.0f;
    Field_0x38 = 0;
    Field_0x44 = now_ticks;
    Field_0x00->visible = 1;
    Field_0x00->dirty_0x2b = 1;
}
void tTrackLogoAnimation::Sub_001F9318(std::uint32_t now_ticks) {
    if (!Field_0x00) hp2::not_implemented("0x001F9318", "track logo animator not bound");
    const float elapsed = static_cast<float>(static_cast<std::int32_t>(now_ticks - Field_0x44)) * 0.00025f;
    constexpr float fade_end = 0.35f; // 0032EC30
    constexpr float slide_end = 0.35f + 0.27f; // gp9330 + gpBE78
    if (elapsed < fade_end) {
        const float fraction = elapsed < 0.0f ? 0.0f : elapsed / fade_end;
        Field_0x00->a = static_cast<std::uint8_t>(static_cast<int>(Field_0x48 * (1.0f - fraction)));
        Field_0x4C = false;
    } else if (elapsed <= fade_end || elapsed > slide_end) {
        if (elapsed > slide_end) Field_0x00->x = Field_0x40;
    } else {
        if (!Field_0x4C) {
            // Original zeroed twelve-word list yields texture zero for an empty list.
            Field_0x00->texture_hash = Field_0x04.empty() ? 0 : Field_0x04[static_cast<std::size_t>(Field_0x38)];
            Field_0x4C = true;
        }
        const float fraction = (elapsed - fade_end) / (slide_end - fade_end);
        Field_0x00->x = static_cast<float>(static_cast<int>(fraction * (Field_0x40 - 640.0f) + 640.0f));
        Field_0x00->a = static_cast<std::uint8_t>(static_cast<int>(Field_0x48));
    }
    Field_0x00->dirty_0x2b = 1;
}
void tTrackLogoAnimation::Sub_001F91D0() { if (Field_0x00) Field_0x00->visible = 1; }
void tTrackLogoAnimation::Sub_001F91E0() { if (Field_0x00) Field_0x00->visible = 0; }

} // namespace hp2::frontend

namespace hp2::frontend {
// ELF 0x001F6FA0; E-0018-02; E-0028-C03; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001f6fa0.md.
void tEventLeaf::Sub_001F6FA0() {
    hp2::not_implemented("0x001F6FA0", "tEventLeaf::Sub_001F6FA0 (signature and full body unresolved)");
}
} // namespace hp2::frontend
