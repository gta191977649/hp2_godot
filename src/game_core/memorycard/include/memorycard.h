// Original source path, recovered verbatim from the authorized ELF: `src\memorycard.cpp`
// (string at 0x002AD488, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// Only the `.cpp` path is recovered. This header's name and the `include/` + `src/`
// split are port-side, mirroring the `bSound/` pair already in this tree; the
// original header name is not recovered.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// TASK-0021: the PS2 memory-card save container and the global player profile.
//
// SLUS_203.62, SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Read-only GhidraMCP, Ghidra 12.0.4, 2026-10-13. Evidence E-0021-01..12; see
// docs/SAVE_SYSTEM.md and tasks/active/TASK-0021-memorycard-profile.md.
//
// Nothing here touches a file, a memory card or Godot. The caller supplies the bytes and
// stores the bytes; `src/platform` owns that. The PS2 driver sequencing inside
// `src\memorycard.cpp` (FUN_0012C938 and the `sceMc*` wrappers below it) is deliberately
// not translated -- a host file is not a memory card -- and H-0021-01 records what was
// observed about its operation numbers without asserting a mapping.
namespace hp2::memorycard {

// ------------------------------------------------------------------ the card-side names
// FUN_0012DEE0 builds the save directory name as "%s%s%s" (0x0032FDC8) from a region
// prefix, the title id and the suffix "HP2". FUN_001A3390 returns `uGpffffB364`; zero
// selects the PAL triple. (E-0021-02)
inline constexpr const char* kDirectoryFormat_0032FDC8 = "%s%s%s";
inline constexpr const char* kRegionPrefixPal_0032FDB0 = "BE";
inline constexpr const char* kRegionPrefixNtsc_0032FDC0 = "BA";
inline constexpr const char* kTitleIdPal_002AD6C8 = "SLES-50731";
inline constexpr const char* kTitleIdNtsc_002AD6D8 = "SLUS-20362";
inline constexpr const char* kDirectorySuffix_0032FDB8 = "HP2";

// FUN_0012CDE8 / FUN_0012CF00 / FUN_0012D118 / FUN_0012D858 join with "/%s/%s"; FUN_0012C938
// enumerates with "/%s/*". (E-0021-03)
inline constexpr const char* kPathFormat_0032FD98 = "/%s/%s";
inline constexpr const char* kEnumerateFormat_0032FDA0 = "/%s/*";

// The three files the original creates in that directory. (E-0021-03, E-0021-04)
inline constexpr const char* kProfileFileName_002BEBE0 = "settings.db";
inline constexpr const char* kIconDescriptorName_002AD5E8 = "icon.sys";
inline constexpr const char* kIconFileName_002AD588 = "nfs6.ico";
inline constexpr const char* kIconDescriptorMagic_0032FDA8 = "PS2D";
// FUN_0012D858 writes the icon descriptor as 0x3C4 bytes from manager+0x184. The contents
// of that block were not recovered.
inline constexpr std::size_t kIconDescriptorSize_0012D858 = 0x3c4;

// The two region values FUN_0012DEE0 can produce. (E-0021-02)
enum Region_001A3390 : int {
    kRegionPal_001A3390 = 0,   // uGpffffB364 == 0
    kRegionNtsc_001A3390 = 1,  // anything else
};

// "BASLUS-20362HP2" or "BESLES-50731HP2".
std::string Sub_0012DEE0(int region);

// ------------------------------------------------------------------ the payload layout
// FUN_00245B00 and FUN_00245BA0 both pass `settings.db` with length 0x114E0 through
// vtable+0x2C (FUN_0012E270). (E-0021-04)
inline constexpr std::size_t kProfileSize_00245B00 = 0x114e0;
// FUN_00245738: `DAT_002FC050 = 0x114E0`, i.e. the first word of the payload is stamped with
// the payload's own size before every store. (E-0021-05)
inline constexpr std::size_t kProfileSizeField_00245738 = 0x0000;
// FUN_00245738: `DAT_0030D52C = FUN_001878B0(0x2FC050)`, and 0x0030D52C - 0x002FC050 ==
// 0x114DC, so the checksum is the payload's final dword. (E-0021-05)
inline constexpr std::size_t kProfileChecksumField_00245738 = 0x114dc;
// FUN_001878B0's counter starts at 0x4536 and the loop exits once the decrement reaches -1,
// so it folds 0x4537 words == 0x114DC bytes: everything except the checksum itself.
// (E-0021-06)
inline constexpr std::size_t kChecksumWordCount_001878B0 = 0x4537;

// FUN_001878B0: sum of signed 32-bit little-endian words from the buffer base.
std::int32_t Checksum_001878B0(const std::uint8_t* payload);

// ------------------------------------------------------------------ the profile block
// The global player profile is one 0x114E0-byte block at ELF 0x002FC050. FUN_00245D30 copies
// a loaded payload onto it verbatim and FUN_00245738 hands the same address back as the
// store source, so there is no staging copy in either direction. (E-0021-07)
//
// Only the offsets some function was observed to read are named below. **The great majority
// of the block is unidentified** and is deliberately not modelled: this type is an opaque
// byte array with accessors, not a struct. Adding a field here requires the instruction that
// reads it.
inline constexpr std::uint32_t kProfileBase_002FC050 = 0x002fc050;

// FUN_001EF120(0x2FC0E4) in FUN_00245D30's post-load pass. Meaning not recovered.
inline constexpr std::size_t kField_0x0094_001EF120 = 0x0094;
// FUN_001F4B10(0x2FC194) in the same pass. Meaning not recovered.
inline constexpr std::size_t kField_0x0144_001F4B10 = 0x0144;
// The event-tree start-node block at 0x002FC1B4, read by FUN_001F5D00, written by
// FUN_001F5D28 and FUN_001F5D50. (E-0021-09)
inline constexpr std::size_t kEventTreeTypeField_001F5D50 = 0x0164;
inline constexpr std::size_t kEventTreeStartNodeType1_001F5D00 = 0x0168; // Hot Pursuit
inline constexpr std::size_t kEventTreeStartNodeType0_001F5D00 = 0x016c; // Championship
// DAT_002FC1C0, the view state FUN_001FAF60 restores when its zoom transition ends.
inline constexpr std::size_t kEventTreeViewState_001FAF60 = 0x0170;
// DAT_002FC1C4 <- DAT_0031C1C8 in FUN_00245D30. Meaning not recovered.
inline constexpr std::size_t kField_0x0174_00245D30 = 0x0174;
// DAT_002FC1C8, two words fed to FUN_00154308 / FUN_00155000. Meaning not recovered.
inline constexpr std::size_t kField_0x0178_00245D30 = 0x0178;
// The two 0x100-byte event blocks FUN_001FE988 slices up: DAT_002FC1D8 and DAT_002FC2D8.
inline constexpr std::size_t kEventBlockChampionship_002FC1D8 = 0x0188;
inline constexpr std::size_t kEventBlockHotPursuit_002FC2D8 = 0x0288;
inline constexpr std::size_t kEventBlockSize_001FE988 = 0x0100;
// DAT_0030C77C, the mode-select tree selector: 8 Championship, 9 Hot Pursuit (FUN_00185568).
inline constexpr std::size_t kSelectedModeField_00185568 = 0x1072c;

// FUN_001F5D00's `else` arm. An empty profile therefore starts on event 1. (E-0021-09)
inline constexpr int kDefaultStartNode_001F5D00 = 1;

// The 0x114E0 block. Default-constructed all zero, which is what the original's own
// FUN_00245700 produces before a load and what this port uses when no save is configured.
class Profile_002FC050 {
public:
    Profile_002FC050();

    const std::uint8_t* bytes() const { return bytes_.data(); }
    std::uint8_t* bytes() { return bytes_.data(); }
    std::size_t size() const { return bytes_.size(); }

    std::uint32_t read_u32(std::size_t offset) const;
    std::int32_t read_i32(std::size_t offset) const;
    std::uint8_t read_u8(std::size_t offset) const;
    void write_u32(std::size_t offset, std::uint32_t value);

    // FUN_00185568 reads profile+0x1072C: 8 selects the Championship tree, 9 Hot Pursuit.
    int SelectedMode_00185568() const;
    // FUN_001F5D50(0x2FC1B4, type): `sw a1, 0x0(a0)`.
    void SetEventTreeType_001F5D50(int tree_type);
    int EventTreeType_001F5D50() const;
    // FUN_001F5D00(0x2FC1B4, type): type 0 -> +0x16C, type 1 -> +0x168, otherwise 1.
    int GetSelectedEvent(int tree_type) const;
    // FUN_001F5D28(0x2FC1B4, type, number). Selects on the stored type at +0x164, **not** on
    // its own `tree_type` argument, which the original ignores. Preserved deliberately.
    void SetSelectedEvent(int tree_type, int event_number);
    // DAT_002FC1C0.
    int EventTreeViewState_001FAF60() const;

    // The 0x100-byte event block for a tree type: 0 -> +0x188, 1 -> +0x288 (FUN_00245D30).
    // Returns nullptr for any other tree type, which the original never passes.
    const std::uint8_t* EventBlock_001FE988(int tree_type) const;
    std::uint8_t* EventBlock_001FE988(int tree_type);

private:
    std::vector<std::uint8_t> bytes_;
};

// ------------------------------------------------------------------ load and store
enum class LoadResult_00245780 {
    kOk,
    kWrongLength,     // not 0x114E0 bytes; the original asks the driver for exactly that many
    kBadChecksum,     // FUN_00245780 compared FUN_001878B0 against payload+0x114DC and differed
};

// FUN_00245780 then FUN_00245D30: verify the trailing checksum, then copy the payload onto
// the profile verbatim. The post-load fix-up calls FUN_00245D30 makes after the copy
// (FUN_001FE988 twice, FUN_001EF120, FUN_00154308/FUN_00155000, FUN_001F4B10) belong to their
// own subsystems and are **not** performed here; the event tree runs its own
// build_profile_layout_001FE988 already. (E-0021-07, E-0021-08)
LoadResult_00245780 Load_00245D30(const std::vector<std::uint8_t>& payload,
                                  Profile_002FC050& profile);

// FUN_00245738 then FUN_00245B00: stamp the size word and the checksum into the profile
// itself, then hand out 0x114E0 bytes. The original writes into the live profile, so this
// mutates `profile` exactly as it does. (E-0021-05)
std::vector<std::uint8_t> Store_00245738(Profile_002FC050& profile);

// ------------------------------------------------------------------ event slice decode
// FUN_002010B8's status values. 1, 2 and 3 all map to the same `_UNLOCKED` panel texture in
// FUN_001F75F0, but they are distinct statuses and the event detail panel uses them.
// (E-0021-10)
enum EventSliceStatus_002010B8 : int {
    kSliceStatusLocked = 0,
    kSliceStatusNotAttempted = 1,
    kSliceStatusBelowFloor = 2,
    kSliceStatusGoalMissed = 3,
    kSliceStatusWon = 4,
};

// FUN_00200018, GoalTypes 0 and 2: one byte against the requirement row's u32 threshold.
// E-0043-N01; typed host carrier, not a recovered native class layout.
struct EventRankState {
    static int GetStatus(std::uint8_t slice_value, std::uint32_t goal_threshold);
};
// FUN_00200068, GoalTypes 1, 5 and 6: a signed 32-bit value; 0 and 0x7FFFFFFF mean
// "not attempted".
// E-0043-N01; typed host carrier, not a recovered native class layout.
struct EventTimeState {
    static int GetStatus(std::int32_t slice_value, std::int32_t goal_threshold);
};

} // namespace hp2::memorycard
