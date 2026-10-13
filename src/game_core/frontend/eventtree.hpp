#pragma once
#include "game_core/assets/elab_chunks.hpp"
#include <cstdint>
#include <string>
#include <vector>

// TASK-0018: the event tree behind the Ultimate Racer / Championship entries.
//
// Original source file: `..\indep\src\eventtree.cpp` (ELF string 0x002BA9F8), so this file keeps
// that name. The class names in it are the original pool-name strings: `tEventTreeView`
// (0x002BA8F8), `tEventLeaf` (0x002BA878), `tEventGroup` (0x002BA888) and `tLineSegment`
// (0x002BA858). Everything whose original identifier was not recovered carries its ELF address,
// per AGENT.md R2.
//
// SLUS_203.62, SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Evidence E-0018-01..12; records under reverse/functions/<binary-id>/frontend/.
namespace hp2::frontend {

// ------------------------------------------------------------------- the data source
// FUN_001FFC40 parses one `0x80034A00` container per tree. FRONTEND/FRONTB.BUN carries exactly
// two of them, named "Championship" and "Hot Pursuit" by their `0x00000001` child. The tree the
// player sees is picked by FUN_00185568 from profile+0x1072C: 8 -> "Championship" (the World
// Racing branch), 9 -> "Hot Pursuit" (the branch whose menu entry is labelled Ultimate Racer).
// `iGpffffA548` switches both to their "... E3" variants, which this bundle does not carry.
// (E-0018-01)
inline constexpr std::uint32_t kEventTreeChunkId_80034A00 = 0x80034a00;
inline constexpr std::uint32_t kEventTreeNameChunkId_00000001 = 0x00000001;
inline constexpr std::uint32_t kEventTreeChunk_00034A01 = 0x00034a01; // -> tree+0x14, 4-byte rows
inline constexpr std::uint32_t kEventTreeChunk_00034A02 = 0x00034a02; // -> tree+0x1C, 4-byte rows
inline constexpr std::uint32_t kEventTreeChunk_00034A03 = 0x00034a03; // -> tree+0x24, the 8-byte
                                                                      //    link table
inline constexpr std::uint32_t kEventTreeChunk_00034A04 = 0x00034a04; // -> tree+0x2C, 8-byte rows
inline constexpr std::uint32_t kEventTreeChunk_00034A05 = 0x00034a05; // -> the event records
inline constexpr std::uint32_t kEventTreeChunk_00034A06 = 0x00034a06; // -> tree+0x34, 4-byte rows

// FUN_001FFF60 divides the 0x34A05 payload by 0xBC, so an event record is 188 bytes and both
// shipped trees hold exactly 32 of them. (E-0018-02)
inline constexpr std::size_t kEventRecordSize_001FFF60 = 0xbc;
inline constexpr std::size_t kEventLinkSize_002008B8 = 8;

// ------------------------------------------------------------------- node placement
// FUN_001F96B8 lays the nodes out from a 200-character ASCII map held in the ELF: it walks the
// map counting non-space characters until it has counted `record+0x28` (the event number) of
// them, and stores `130.0f * (position % 20)` as that event's X. So the map is 10 rows of 20
// columns and the n-th 'x' in reading order is event n. Y is not taken from the map: FUN_001F7730
// returns `depth * 350.0f`, where FUN_001F7FA8 computes depth by following the first incoming
// link back to the root. (E-0018-03)
inline constexpr int kLayoutColumns_001F96B8 = 20;
inline constexpr float kColumnSpacing_gp_0x41B0 = 130.0f;
inline constexpr float kRowSpacing_gp_0x41B4 = 350.0f;

inline constexpr const char* kLayoutChampionship_0031B3C0 =
    "        x           "
    "    x       x       "
    "  x   x  x  x  x    "
    " x x   x x   x   x  "
    "x  x  x x x    x   x"
    "x  x   x x x     x  "
    "     x       x      "
    "        x           "
    "        x           "
    "        x           ";

inline constexpr const char* kLayoutHotPursuit_0031B490 =
    "        x           "
    "    x       x       "
    "  x   x x x   x     "
    " x x    x  x  x  x  "
    " x    x   x x x x  x"
    "x x      x   x x x  "
    "     x          x   "
    "        x           "
    "        x           "
    "        x           ";

// ------------------------------------------------------------------- per-node state
// FUN_001F75F0 picks a node's panel texture from the tree type and the status FUN_002010B8
// returns: 0 -> LOCKED, 1/2/3 -> UNLOCKED, 4 -> WON. The texture names are the ELF's own strings.
// (E-0018-04)
enum EventStatus_002010B8 : int {
    kEventStatusLocked = 0,
    kEventStatusUnlocked = 1,
    kEventStatusUnlocked2 = 2,
    kEventStatusUnlocked3 = 3,
    kEventStatusWon = 4,
};
inline constexpr const char* kSpeedPanelLocked_002BA768 = "SPEED_EVENT_PANEL_LOCKED";
inline constexpr const char* kSpeedPanelUnlocked_002BA788 = "SPEED_EVENT_PANEL_UNLOCKED";
inline constexpr const char* kSpeedPanelWon_002BA7A8 = "SPEED_EVENT_PANEL_WON";
inline constexpr const char* kPursuitPanelLocked_002BA7C0 = "PURSUIT_EVENT_PANEL_LOCKED";
inline constexpr const char* kPursuitPanelUnlocked_002BA7E0 = "PURSUIT_EVENT_PANEL_UNLOCKED";
inline constexpr const char* kPursuitPanelWon_002BA800 = "PURSUIT_EVENT_PANEL_WON";
// Returns the panel texture name for a tree type (0 = Championship/speed, 1 = Hot Pursuit) and a
// status, or nullptr for a status FUN_001F75F0's switch does not cover.
const char* EventPanelTexture_001F75F0(int tree_type, int status);

// FUN_00200DE8: how many bytes of the profile block one event owns, selected by its goal type at
// record+0x29. (E-0018-05)
int EventSaveSize_00200DE8(std::uint8_t goal_type);

// ------------------------------------------------------------------- records and links
// Only the fields this port reads are modelled; the record is 188 bytes and most of it was not
// identified. Every offset below was read from the code that uses it.
struct EventRecord_001FFF60 {
    std::string name_0x08;          // 32-byte field; the authored English event name
    std::uint8_t number_0x28 = 0;   // 1-based event number; FUN_001FA840 looks nodes up by it
    std::uint8_t goal_type_0x29 = 0;// FUN_00200DE8's switch
    std::uint16_t index_0x42 = 0;   // row index into the 0x34A03 link table for this event
    std::uint8_t link_count_0x3D = 0;      // number of incoming links (FUN_002008B8's bound)
    std::vector<std::int16_t> link_index_0x7C; // s16 rows into the 0x34A03 table, link_count long
    std::uint32_t string_id_0xB0 = 0;      // localised description, fed to the scrolling text
};

// One 8-byte row of chunk 0x34A03. FUN_002008B8 returns a pointer to it and its only caller reads
// +0x02 as the event number at the other end of the link; the remaining six bytes are not
// identified. (E-0018-06)
struct EventLink_002008B8 {
    std::uint16_t field_0x00 = 0;
    std::uint16_t target_number_0x02 = 0;
    std::uint32_t field_0x04 = 0;
};

// `tEventLeaf` (0x60 bytes in the original). The port keeps the fields it needs plus the graph
// edges the view builds in FUN_001F96B8.
struct EventLeaf_001F6FA0 {
    int record_index = 0;            // position in the record array, leaf+0x08
    int depth_0x1C = 0;              // FUN_001F7FA8
    float x = 0.0f, y = 0.0f;        // FUN_001F7710 / FUN_001F7730
    std::vector<int> incoming;       // record indices; built from link_index_0x7C
    std::vector<int> outgoing;       // the reverse edges the view also threads (leaf+0x2C list)
    int group = -1;                  // `tEventGroup` index; connected component
};

class EventTree_001FFC40 {
public:
    std::string name;            // from the 0x00000001 chunk
    int type_0x08 = -1;          // 0 = "Championship", 1 = "Hot Pursuit" (FUN_001FDEF8)
    std::vector<EventRecord_001FFF60> records;
    std::vector<EventLink_002008B8> links;
    std::vector<EventLeaf_001F6FA0> leaves;

    const char* layout_map() const;
    // FUN_001FA840(view, number): the leaf whose record+0x28 matches, or -1.
    int FindLeafByNumber_001FA840(std::uint8_t number) const;
    // FUN_002010B8 reduced to what a profile-less host can answer: an event with no incoming
    // links is unlocked, every other event is locked until the profile says otherwise. The real
    // function also decodes the per-event profile slice and has two special cases, for event 31
    // and for event 32 -- see docs.
    int Status_002010B8(int record_index) const;
};

// Parses every `0x80034A00` container in a bundle. Returns the trees in file order, which is
// "Championship" then "Hot Pursuit" in the shipped FRONTEND/FRONTB.BUN.
std::vector<EventTree_001FFC40> parse_event_trees(const std::vector<std::uint8_t>& buffer);

// ------------------------------------------------------------------- the profile block
// FUN_001FE988(tree, block, 0x100, 4) hands every event a slice of a 256-byte block -- one block
// per tree, DAT_002FC1D8 for the Championship tree and DAT_002FC2D8 for Hot Pursuit -- sized by
// the event's goal type and aligned up to 4. Events whose record+0x28 is 0x20 (event 32) reuse
// the previous slice rather than taking one of their own. This reproduces the offsets; the
// contents come from the player's memory-card profile, which this port does not read, so every
// slice is zero and no event reports a status above Locked. (E-0018-07)
struct EventProfileLayout_001FE988 {
    static constexpr std::size_t kBlockSize = 0x100;
    static constexpr int kAlignment = 4;
    std::vector<int> offsets;   // per record index; -1 when the record shares the previous slice
    int used = 0;               // bytes consumed, tree+0x48 at the end of the pass
};
EventProfileLayout_001FE988 build_profile_layout_001FE988(const EventTree_001FFC40& tree);

} // namespace hp2::frontend
