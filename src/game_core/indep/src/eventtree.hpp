// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\eventtree.cpp`
// (string at 0x002BA9F8, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, and the `.hpp` extension is port-side -- only
// the `.cpp` path was recovered.
#pragma once
#include "game_core/memorycard/include/memorycard.h"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <cstdint>
#include <array>
#include <functional>
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
// TASK-0043 E-0043-F01: original EventInfo record identity from SetEvents and
// independently mapped getters. Host storage is not the native 188-byte layout.
// Recovered fields: nCars +0x3B, nTracks +0x3C, nGoals +0x3D,
// nRewards +0x3E, aCars +0x44, aTracks +0x6C, aGoals +0x7C, aRewards +0x8C.
struct EventInfo {
    std::string name_0x08;          // 32-byte field; the authored English event name
    std::uint8_t number_0x28 = 0;   // 1-based event number; FUN_001FA840 looks nodes up by it
    std::uint8_t goal_type_0x29 = 0;// FUN_00200DE8's switch
    std::uint16_t index_0x42 = 0;   // row index into the 0x34A03 link table for this event
    std::uint8_t nGoals = 0;      // number of incoming links (FUN_002008B8's bound)
    std::vector<std::int16_t> aGoals; // s16 rows into the 0x34A03 table, link_count long
    std::uint32_t string_id_0xB0 = 0;      // localised description, fed to the scrolling text
    // E-0022-D01: 001F6770 / 002008F8 / 00200938 and text-token consumers.
    std::uint8_t nTracks = 0, nRewards = 0, Field_0x3F = 0;
    std::vector<std::int16_t> aTracks, aRewards;
    std::uint32_t string_id_0xB4 = 0;
    std::uint8_t nCars = 0; // E-0022-D08, 00200878 car list bound
    std::vector<std::int16_t> aCars;
};

// One 8-byte row of chunk 0x34A03. TASK-0021 corrected what this table is: it is a
// **requirement** table, not a plain link table. `FUN_002008B8(leaf, i)` returns
// `tree+0x24 + record[0x7C + 2i] * 8` for every `i` below `record+0x3D`, and
// `FUN_001FFFB0(row, tree)` asks whether the event numbered at `row+0x02` reaches status 4
// **when judged against this row's own `row+0x04` threshold** -- so the row carries the goal an
// event has to meet, and an event unlocks when any one of its rows is satisfied.
// (E-0021-12; supersedes E-0018-06 in part)
//
// Retired, kept per AGENT.md R6: TASK-0018 recorded +0x02 as "the event number at the other end
// of the link" and the remaining six bytes as unidentified. The +0x02 reading stands -- it names
// the prerequisite event -- but calling the table a link table understated it, and four of those
// six bytes are now identified. `field_0x00` is still unidentified.
struct EventLink_002008B8 {
    std::uint16_t field_0x00 = 0;
    std::uint16_t prerequisite_number_0x02 = 0;
    std::uint32_t goal_threshold_0x04 = 0;
};

// `tEventLeaf`: named allocation at 001F96B8 followed by constructor 001F6FA0,
// label 002BA878; E-0028-C03. Allocation extent 0x60 is not native sizeof.
// The port keeps the fields it needs plus the graph
// edges the view builds in FUN_001F96B8.
struct tEventLeaf {
    int record_index = 0;            // position in the record array, leaf+0x08
    int depth_0x1C = 0;              // FUN_001F7FA8
    float x = 0.0f, y = 0.0f;        // FUN_001F7710 / FUN_001F7730
    std::vector<int> incoming;       // record indices; built from link_index_0x7C
    std::vector<int> outgoing;       // the reverse edges the view also threads (leaf+0x2C list)
    int group = -1;                  // `tEventGroup` index; connected component

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001F6FA0; E-0018-02; E-0028-C03; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001f6fa0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001F6FA0();

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001F7FA8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001f7fa8.md.
    // Original Debug reference: tEventLeaf::CalculateGeneration(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void CalculateGeneration();
};

class EventTree_001FFC40 {
public:
    std::string name;            // from the 0x00000001 chunk
    int type_0x08 = -1;          // 0 = "Championship", 1 = "Hot Pursuit" (FUN_001FDEF8)
    std::vector<EventInfo> records;
    std::vector<EventLink_002008B8> links;
    // E-0043-F01: original EventTree table names at +0x1C/+0x14/+0x2C.
    // These vectors project original pointers; no native pointer layout is implied.
    std::vector<std::uint32_t> pTracks; // chunk 34A02, 002008F8
    std::vector<std::uint32_t> pCars; // chunk 34A01, 00200878
    std::vector<std::array<std::uint8_t, 8>> pRewards; // chunk 34A04, 00200938
    std::vector<tEventLeaf> leaves;

    const char* layout_map() const;
    // FUN_001FA840(view, number): the leaf whose record+0x28 matches, or -1.
    int FindLeafByEventNumber(std::uint8_t number) const;

    // FUN_002008B8(leaf, i): the i-th requirement row of a record, or nullptr when `i` is
    // outside `record+0x3D`. (E-0021-12)
    const EventLink_002008B8* GetGoal(int record_index, int i) const;

    // FUN_002010B8, translated against a real profile. `row` is the requirement row whose
    // threshold the slice is judged against; pass nullptr for the original's `param_2 == 0`
    // entry, in which case the function performs the prerequisite test first and then picks the
    // row itself from `record+0x42`, exactly as the original does. (E-0021-10, E-0021-11)
    //
    // TASK-0022 recovered GoalTypes 3 and 4 (E-0022-D02). Raises hp2::NotImplemented
    // for events 0x1F and 0x20 whose gate globals
    // (DAT_0031D7F4+0x180 / +0x1B8) are not recovered -- but only when a non-empty profile
    // could actually reach those branches. With an all-zero profile the recovered arms answer
    // first, so nothing throws.
    int Status_002010B8(const hp2::memorycard::Profile_002FC050& profile, int record_index,
                        const EventLink_002008B8* row = nullptr) const;

    // Retired, kept per AGENT.md R6 and still used where no profile is available: the
    // profile-less reduction TASK-0018 shipped. An event with no requirement rows is Unlocked
    // and every other event is Locked, which is what Status_002010B8 answers for a zero profile
    // anyway -- so this is now a shortcut, not a different answer.
    int StatusWithoutProfile_002010B8(int record_index) const;

    // FUN_001FA880(view, leaf, leaf->depth_0x1C, direction): among the leaves at the same depth,
    // the one nearest in X strictly to the left (direction -1) or strictly to the right
    // (direction +1) of the current leaf. Returns -1 when there is none -- the original does not
    // wrap. (E-0021-19)
    int WalkSameDepth_001FA880(int record_index, int direction) const;

    // FUN_001FA978(view, leaf, list): among the leaves on one of the current leaf's two
    // adjacency lists, the one whose X is closest to the current leaf's. direction +2 walks
    // leaf+0x20 (the prerequisites, towards the root) and -2 walks leaf+0x28 (the events that
    // require this one). First match wins a tie. Returns -1 for an empty list. (E-0021-20)
    int WalkAdjacent_001FA978(int record_index, int direction) const;

    // FUN_001FA9C8's dispatch: -1/+1 go to WalkSameDepth_001FA880, +2/-2 to
    // WalkAdjacent_001FA978, anything else answers -1.
    int MoveCursor_001FA9C8(int record_index, int direction) const;
    // ELF 002008F8 / 00200938, E-0022-D01. Null for original index-bound failure.
    const std::uint32_t* GetTrack(int record_index, int i) const;
    // ELF 00200878, E-0022-D08: car list row, bound +3B and signed indices +44.
    const std::uint32_t* GetCar(int record_index, int i) const;
    const std::array<std::uint8_t, 8>* GetReward(int record_index, int i) const;
    // ELF 002012D8, E-0022-D02. Number of completed subevents from profile slice.
    int Sub_002012D8(const hp2::memorycard::Profile_002FC050& profile, int record_index) const;
};

// E-0022-D08, ELF 001F8C10/8E18/8D20/8FB0. Original type name unknown.
// Widget pointer is a port-side representation of original document/hash lookups.
// TASK-0041 E-0041-R05; original class spelling; retail 0x001F8C10, debug 0x001F47F0. Native interface/layout remain port projections.
class tCarLogoAnimation {
    FeWidget* Field_0x00 = nullptr;
    std::uint32_t Field_0x04 = 0;
    std::vector<std::uint32_t> Field_0x08;
    int Field_0x2C = 0, Field_0x30 = 0, Field_0x38 = 0, Field_0x3C = 0;
    float Field_0x34 = 0;
    void Sub_001F8D20(bool randomize, std::uint32_t& rng);
public:
    void Bind_001F8C10(FeScreen& screen, std::uint32_t now_ticks);
    void Sub_001F8E18(const std::vector<std::uint32_t>& logos, std::uint32_t now_ticks);
    void Sub_001F8FB0(std::uint32_t now_ticks, std::uint32_t& rng);
    // E-0022-D08, ELF 001F8CF0 / 001F8D08: show/hide the bound car-logo widget.
    void Sub_001F8CF0();
    void Sub_001F8D08();
};
// E-0022-D09, ELF 001F90F0/91F0/9318. Original type name unknown.
// TASK-0041 E-0041-R06; original class spelling; retail 0x001F90F0, debug 0x001F4CD0. Native interface/layout remain port projections.
class tTrackLogoAnimation {
    FeWidget* Field_0x00 = nullptr;
    std::vector<std::uint32_t> Field_0x04;
    int Field_0x38 = 0;
    float Field_0x40 = 0, Field_0x48 = 0;
    std::uint32_t Field_0x44 = 0;
    bool Field_0x4C = false;
public:
    void Bind_001F90F0(FeScreen& screen, std::uint32_t now_ticks);
    void Sub_001F91F0(const std::vector<std::uint32_t>& logos, std::uint32_t now_ticks);
    void Sub_001F9318(std::uint32_t now_ticks);
    // E-0022-D09, ELF 001F91D0 / 001F91E0: show/hide the bound track-logo widget.
    void Show();
    void Sub_001F91E0();
};
// Both animators reuse fmenus.hpp's Sub_0025A3D0 and the caller's shared RNG seed.

// ELF 00201350, E-0022-D02: goal 3 -> +3F, goal 4 -> +3C, otherwise 1.
int GetRaceCount(const EventInfo& record);
// ELF 001A3B98/001A3B28/001A3838/001A37B8, E-0022-D07. Timer formatter,
// colocated here as a port-side dependency aggregation; source path not recovered.
// locale is DAT_0031C1C8, flags are original 001A3838 bits (0 in event callers).
std::string Sub_001A3B98(std::int32_t value, int locale, unsigned flags = 0);
// ELF 001F60E8, E-0022-D03. The caller resolves ID F230 + status. Its operands
// are progress/total for status 2, a result string for statuses 3/4, none for 0/1.
std::uint32_t Sub_001F60E8(int status);
std::string Sub_001F60E8(int status, const std::string& format, int progress, int total,
                       const std::string& result);
// ELF 001F6220, E-0022-D04. Callback seams are port-side dependencies on the
// original car/track databases; absent callbacks fail loudly on the reached arm.
std::string Sub_001F6220(const std::array<std::uint8_t, 8>* reward,
                       const std::string& localized_101D8,
                       const std::function<std::string(int)>& car_name,
                       const std::function<std::string(int, int)>& track_name);
// ELF 001F5F88 and 001F5DD0, E-0022-D05. 512 output bytes, original token order
// and original placeholder literals. Timer callback is the 001A3B98 dependency.
std::string Sub_001F5F88(const EventTree_001FFC40& tree, int record_index,
                       const std::string& source,
                       const std::function<std::string(std::int32_t)>& timer);
// ELF 00200E78, E-0022-D06. Result text uses EE48 + placing for byte/nibble/points
// goals; timed goals use the timer dependency. Requires status >= 3 as original.
std::string Sub_00200E78(const EventTree_001FFC40& tree,
                       const hp2::memorycard::Profile_002FC050& profile, int record_index,
                       int status, const std::function<std::string(std::uint32_t)>& localize,
                       const std::function<std::string(std::int32_t)>& timer);

// ------------------------------------------------------------------- the camera (TASK-0021)
// Recovered, replacing the port-side calibration TASK-0018 had to use. FUN_001F96B8 allocates a
// 0x70-byte `tStillCameraMover` (pool name string 0x002BA898) with FUN_002577C0 and constructs
// it through FUN_0015E6A0(object, 1), then positions it with
// `FUN_001FAB00(320.0f, 240.0f, view, view->current_leaf)`:
//
//     target.x = 320.0f - leaf->x(+0x30) * view->zoom(+0x80)
//     target.y = 240.0f - leaf->y(+0x34) * view->zoom(+0x80)
//
// and hands `target` to FUN_001DAAA0(view+0xA8). So the selected node is pinned to the centre of
// the 640x480 authored space, and the scale is one of two discrete levels rather than a
// continuum: FUN_001FAB50(view, delta) clamps `view+0x138` to 0..1 and reads the level's scale
// out of a two-float table at gp-0x4178. FUN_001F96B8 calls it with delta 0, so a freshly opened
// tree sits at level 0. (E-0021-13)
//
// gp itself is 0x003358F0, established by locating the already-known pair
// gp-0x41B4 == 350.0f / gp-0x41B0 == 130.0f at 0x0033173C.
inline constexpr float kCameraCentreX_001FAB00 = 320.0f;
inline constexpr float kCameraCentreY_001FAB00 = 240.0f;
inline constexpr float kZoomLevels_gp_0x4178[2] = {0.4f, 1.0f};
inline constexpr int kZoomLevelMin_001FAB50 = 0;
inline constexpr int kZoomLevelMax_001FAB50 = 1;

// FUN_001FA880's initial "best" values, read out of gp-0x6CBC and gp-0x6CB8, and FUN_001FA978's
// out of gp-0x6CB4. They are these literals, not FLT_MAX. (E-0021-14)
inline constexpr float kWalkWorstLeft_gp_0x6CBC = -999999.0f;
inline constexpr float kWalkWorstRight_gp_0x6CB8 = 999999.0f;
inline constexpr float kWalkWorstDistance_gp_0x6CB4 = 999999.0f;

// ------------------------------------------------------------------- screen 39 (TASK-0021)
// The event tree's own screen is index 39 of FRONTEND/FRONTB.BUN. Every name below was confirmed
// by matching an ELF immediate against that screen's stored widget name or class hash under
// StringHash_002534A8, which is why the hash is written next to it. (E-0021-15)
//
// This answers docs/EVENT_TREE.md open question 3: the widgets FUN_001FAF60 touches at runtime
// are HELP_TEXT and HELP_BUTTON, both revealed by `FUN_00144968(screen, hash, 1)` once the
// state-7 zoom transition finishes.
inline constexpr const char* kWidgetEventTree_A01F24F0 = "EVENT_TREE";
inline constexpr const char* kWidgetEventPanel_A3B23510 = "EVENT_PANEL";
inline constexpr const char* kWidgetEventNumber_16B7D629 = "EVENT_NUMBER";
inline constexpr const char* kWidgetEventTitleBar_FECEBEF6 = "EVENT_TITLE_BAR";
inline constexpr const char* kWidgetWhiteoutBar_70A404EC = "WHITEOUT_BAR";
inline constexpr const char* kWidgetHelpText_0880AC4C = "HELP_TEXT";
inline constexpr const char* kWidgetHelpButton_027F7503 = "HELP_BUTTON";
inline constexpr const char* kWidgetBottomMask_AA5D7D3F = "BOTTOM_MASK";
// EVENT_NUMBER and EVENT_PANEL share this class hash; FUN_001F6FA0 walks every widget of the
// class with FUN_00143DD8 so each `tEventLeaf` gets its own instance pair.
inline constexpr std::uint32_t kEventNodeClassHash_A01BCDC6 = 0xa01bcdc6u;
// The class shared by BG1, DIVERBAR, SMALL_EVENTNUMBER and the OVERLAY_* text widgets, bound by
// FUN_001F8820.
inline constexpr std::uint32_t kOverlayClassHash_A2A16EA1 = 0xa2a16ea1u;

// FUN_001F96B8 sets the EVENT_TREE widget's localised string id from DAT_0030C77C
// (profile+0x1072C) through FUN_001449A8: 8 selects 0x272A and 9 selects 0x272D, anything else
// 0. FE_ENGLISH.BIN resolves these as "Championship" and "Ultimate Racer". (E-0022-R03)
inline constexpr std::uint32_t kTitleStringChampionship_001F96B8 = 0x272a;
inline constexpr std::uint32_t kTitleStringHotPursuit_001F96B8 = 0x272d;
std::uint32_t TitleStringId_001F96B8(int selected_mode);

// The three sibling documents FUN_001F96B8 constructs alongside screen 39, identified by
// matching each ELF immediate against the 41 stored screen name hashes. Their own widget sets
// are not translated yet. (E-0021-17)
inline constexpr int kEventDetailScreen_A744BA7B = 17;    // FUN_001F6678(view+0x13C), state 2
inline constexpr int kZoomTransitionScreen_FD297FD4 = 18; // FUN_001F9480(view+0x654), state 7
inline constexpr int kState5Screen_F1EED4CD = 19;         // FUN_001F6328(view+0x5AC), state 5

// ------------------------------------------------------------------- the view state machine
// FUN_001FAF60's state word, view+0x74. FUN_001F96B8 initialises it from profile+0x170
// (DAT_002FC1C0) unless FUN_001F9F10 asks for the intro, which forces 7. (E-0021-18)
enum ViewState_001FAF60 : int {
    kViewStateBrowsing_001FAF60 = 0,
    kViewStateEventSelected_001FAF60 = 2,
    kViewStateState5_001FAF60 = 5,
    kViewStateState6_001FAF60 = 6,
    kViewStateZoomTransition_001FAF60 = 7,
};

// The menu sound ids FUN_001FAF60 answers with, from kMenuSoundTable_002FB0A8. Writing 0 means
// silence, which is what a move that does not land on a new node answers. (E-0017-02)
inline constexpr int kSoundSilent_001FAF60 = 0;
inline constexpr int kSoundMenuWrong_001FAF60 = 8;
inline constexpr int kSoundTreeUpDown_001FAF60 = 0x13;
inline constexpr int kSoundTreeLeftRight_001FAF60 = 0x14;
inline constexpr int kSoundTreeZoomInOut_001FAF60 = 0x15;
inline constexpr int kSoundStartEvent_001FAF60 = 0x18;

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

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: preserved_existing_module; class-to-original-file binding not asserted.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::frontend {
class tEventGroup {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001F7FE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001f7fe8.md.
    // Original Debug reference: tEventGroup::IsMember(tEventLeaf *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsMember();
};
} // namespace hp2::frontend

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: preserved_existing_module; class-to-original-file binding not asserted.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::frontend {
class tEventTreeView {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001FA840; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fa840.md.
    // Original Debug reference: tEventTreeView::FindLeafByEventNumber(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FindLeafByEventNumber();
};
} // namespace hp2::frontend

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class EventInfo {
public:
    // ELF 0x00200878; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00200878.md.
    // Original Debug reference: EventInfo::GetCar(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetCar();
    // ELF 0x002008B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002008b8.md.
    // Original Debug reference: EventInfo::GetGoal(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetGoal();
    // ELF 0x002008F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002008f8.md.
    // Original Debug reference: EventInfo::GetTrack(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTrack();
    // ELF 0x00200938; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00200938.md.
    // Original Debug reference: EventInfo::GetReward(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetReward();
    // ELF 0x00200978; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200978.md.
    // Original Debug reference: EventInfo::GetOpponent(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetOpponent();
    // ELF 0x00200D48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200d48.md.
    // Original Debug reference: EventInfo::GetRankState(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetRankState();
    // ELF 0x00200D70; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200d70.md.
    // Original Debug reference: EventInfo::GetTournamentState(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTournamentState();
    // ELF 0x00200D90; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200d90.md.
    // Original Debug reference: EventInfo::GetKoState(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetKoState();
    // ELF 0x00200DB0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200db0.md.
    // Original Debug reference: EventInfo::GetTimeState(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTimeState();
    // ELF 0x00201350; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00201350.md.
    // Original Debug reference: EventInfo::GetRaceCount(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetRaceCount();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class EventKoState {
public:
    // ELF 0x002001A8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002001a8.md.
    // Original Debug reference: EventKoState::GetRaceIndex(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetRaceIndex();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class EventLeaf {
public:
    // ELF 0x001FC580; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fc580.md.
    // Original Debug reference: EventLeaf::SetScrollNotch(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetScrollNotch();
    // ELF 0x001FCC88; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fcc88.md.
    // Original Debug reference: EventLeaf::FillVerts(bVector3 *, bVector4 *, int, int, int, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FillVerts();
    // ELF 0x001FD050; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fd050.md.
    // Original Debug reference: EventLeaf::GetNeighbour(unsigned short); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetNeighbour();
    // ELF 0x001FD0B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fd0b0.md.
    // Original Debug reference: EventLeaf::GetReward(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetReward();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class EventRankState {
public:
    // ELF 0x00200018; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200018.md.
    // Original Debug reference: EventRankState::GetStatus(EventGoal *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetStatus();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class EventTimeState {
public:
    // ELF 0x00200068; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200068.md.
    // Original Debug reference: EventTimeState::GetStatus(EventGoal *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetStatus();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class EventTournamentState {
public:
    // ELF 0x00200638; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00200638.md.
    // Original Debug reference: EventTournamentState::Reset(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Reset();
    // ELF 0x00200710; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00200710.md.
    // Original Debug reference: EventTournamentState::RacerRank(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void RacerRank();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class EventTree {
public:
    // ELF 0x001FDFE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fdfe8.md.
    // Original Debug reference: EventTree::GetEventNumber(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetEventNumber();
    // ELF 0x001FFF60; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001fff60.md.
    // Original Debug reference: EventTree::SetEvents(void *, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetEvents();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class RaceEvent {
public:
    // ELF 0x00201558; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00201558.md.
    // Original Debug reference: RaceEvent::RacerIndexFromDriverNumber(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void RacerIndexFromDriverNumber();
    // ELF 0x00201578; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00201578.md.
    // Original Debug reference: RaceEvent::DriverNumberFromRacerIndex(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void DriverNumberFromRacerIndex();
    // ELF 0x00201858; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00201858.md.
    // Original Debug reference: RaceEvent::GetRankState(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetRankState();
    // ELF 0x00201880; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00201880.md.
    // Original Debug reference: RaceEvent::GetTournamentState(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTournamentState();
    // ELF 0x002018A0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002018a0.md.
    // Original Debug reference: RaceEvent::GetKoState(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetKoState();
    // ELF 0x002018C0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002018c0.md.
    // Original Debug reference: RaceEvent::GetTimeState(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTimeState();
    // ELF 0x00202088; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00202088.md.
    // Original Debug reference: RaceEvent::GetRaceCount(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetRaceCount();
};
} // namespace hp2::recovered
