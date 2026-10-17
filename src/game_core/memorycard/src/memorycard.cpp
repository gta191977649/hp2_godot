// Original source path, recovered verbatim from the authorized ELF: `src\memorycard.cpp`
// (string at 0x002AD488, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a partial
// port-side reconstruction, not the original translation unit.
#include "game_core/memorycard/include/memorycard.h"

#include "game_core/port/not_implemented.hpp"

#include <algorithm>
#include <cstring>

namespace hp2::memorycard {
namespace {

// The payload is little-endian on the PS2 and stays little-endian in the file, so every
// accessor below reads bytes rather than casting, and host endianness does not matter.
std::uint32_t load_u32(const std::uint8_t* data) {
    return static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8) |
           (static_cast<std::uint32_t>(data[2]) << 16) |
           (static_cast<std::uint32_t>(data[3]) << 24);
}

void store_u32(std::uint8_t* data, std::uint32_t value) {
    data[0] = static_cast<std::uint8_t>(value & 0xffu);
    data[1] = static_cast<std::uint8_t>((value >> 8) & 0xffu);
    data[2] = static_cast<std::uint8_t>((value >> 16) & 0xffu);
    data[3] = static_cast<std::uint8_t>((value >> 24) & 0xffu);
}

} // namespace

std::string Sub_0012DEE0(int region) {
    // FUN_0012DEE0: FUN_001A3390() == 0 takes the PAL triple, anything else the NTSC one, and
    // the three pieces are joined with "%s%s%s". (E-0021-02)
    const bool pal = region == kRegionPal_001A3390;
    std::string name = pal ? kRegionPrefixPal_0032FDB0 : kRegionPrefixNtsc_0032FDC0;
    name += pal ? kTitleIdPal_002AD6C8 : kTitleIdNtsc_002AD6D8;
    name += kDirectorySuffix_0032FDB8;
    return name;
}

std::int32_t Checksum_001878B0(const std::uint8_t* payload) {
    // FUN_001878B0, verbatim: a signed 32-bit accumulator folding 0x4537 little-endian words
    // from the base. Signed wrap-around is the original's behaviour, so the accumulation runs
    // through an unsigned type and is reinterpreted once at the end rather than relying on
    // signed overflow. (E-0021-06)
    std::uint32_t sum = 0;
    for (std::size_t word = 0; word < kChecksumWordCount_001878B0; ++word) {
        sum += load_u32(payload + word * 4);
    }
    return static_cast<std::int32_t>(sum);
}

Profile_002FC050::Profile_002FC050() : bytes_(kProfileSize_00245B00, 0) {}

std::uint32_t Profile_002FC050::read_u32(std::size_t offset) const {
    if (offset + 4 > bytes_.size()) return 0;
    return load_u32(bytes_.data() + offset);
}

std::int32_t Profile_002FC050::read_i32(std::size_t offset) const {
    return static_cast<std::int32_t>(read_u32(offset));
}

std::uint8_t Profile_002FC050::read_u8(std::size_t offset) const {
    return offset < bytes_.size() ? bytes_[offset] : std::uint8_t{0};
}

void Profile_002FC050::write_u32(std::size_t offset, std::uint32_t value) {
    if (offset + 4 > bytes_.size()) return;
    store_u32(bytes_.data() + offset, value);
}

int Profile_002FC050::SelectedMode_00185568() const {
    return read_i32(kSelectedModeField_00185568);
}

void Profile_002FC050::SetEventTreeType_001F5D50(int tree_type) {
    write_u32(kEventTreeTypeField_001F5D50, static_cast<std::uint32_t>(tree_type));
}

int Profile_002FC050::EventTreeType_001F5D50() const {
    return read_i32(kEventTreeTypeField_001F5D50);
}

int Profile_002FC050::GetSelectedEvent(int tree_type) const {
    // FUN_001F5D00, from the listing at 0x001F5D00: `bne a1,zero` then `lw v0,0x8(a0)`;
    // `bne a1,v0(==1)` then `lw v0,0x4(a0)`; otherwise `li v0,0x1`. (E-0021-09)
    if (tree_type == 0) return read_i32(kEventTreeStartNodeType0_001F5D00);
    if (tree_type == 1) return read_i32(kEventTreeStartNodeType1_001F5D00);
    return kDefaultStartNode_001F5D00;
}

void Profile_002FC050::SetSelectedEvent(int tree_type, int event_number) {
    // FUN_001F5D28, from the listing at 0x001F5D28. The first instruction is
    // `lw v1,0x0(a0)` -- the selector is the block's own stored tree type at +0x164, and the
    // function's `a1` argument is never read, even though its only caller passes the tree's
    // type_0x08 there. Keeping the quirk is AGENT.md R4. (E-0021-09)
    (void)tree_type;
    const int stored = EventTreeType_001F5D50();
    if (stored == 0) {
        write_u32(kEventTreeStartNodeType0_001F5D00, static_cast<std::uint32_t>(event_number));
    } else if (stored == 1) {
        write_u32(kEventTreeStartNodeType1_001F5D00, static_cast<std::uint32_t>(event_number));
    }
}

int Profile_002FC050::EventTreeViewState_001FAF60() const {
    return read_i32(kEventTreeViewState_001FAF60);
}

const std::uint8_t* Profile_002FC050::EventBlock_001FE988(int tree_type) const {
    if (tree_type == 0) return bytes_.data() + kEventBlockChampionship_002FC1D8;
    if (tree_type == 1) return bytes_.data() + kEventBlockHotPursuit_002FC2D8;
    return nullptr;
}

std::uint8_t* Profile_002FC050::EventBlock_001FE988(int tree_type) {
    if (tree_type == 0) return bytes_.data() + kEventBlockChampionship_002FC1D8;
    if (tree_type == 1) return bytes_.data() + kEventBlockHotPursuit_002FC2D8;
    return nullptr;
}

LoadResult_00245780 Load_00245D30(const std::vector<std::uint8_t>& payload,
                                  Profile_002FC050& profile) {
    // The original never sees a short payload: it asks the driver for exactly 0x114E0 bytes
    // and the driver either delivers them or reports an error the front-end screen handles.
    // A host file can be any length, so the length check is the port's own guard and is
    // reported rather than thrown -- a corrupt save is a runtime condition, not an
    // untranslated code path.
    if (payload.size() != kProfileSize_00245B00) return LoadResult_00245780::kWrongLength;

    // FUN_00245780: FUN_001878B0(buffer) against *(int *)(buffer + 0x114DC). (E-0021-06)
    const std::int32_t expected =
        static_cast<std::int32_t>(load_u32(payload.data() + kProfileChecksumField_00245738));
    if (Checksum_001878B0(payload.data()) != expected) {
        return LoadResult_00245780::kBadChecksum;
    }

    // FUN_00245D30: HP2_CopyBytesForward(0x2FC050, **(screen+0x10), 0x114E0). (E-0021-07)
    std::memcpy(profile.bytes(), payload.data(), kProfileSize_00245B00);
    return LoadResult_00245780::kOk;
}

std::vector<std::uint8_t> Store_00245738(Profile_002FC050& profile) {
    // FUN_00245738, verbatim: stamp the size word, then the checksum over everything before
    // it, then hand back the profile address itself. (E-0021-05)
    profile.write_u32(kProfileSizeField_00245738,
                      static_cast<std::uint32_t>(kProfileSize_00245B00));
    const std::int32_t checksum = Checksum_001878B0(profile.bytes());
    profile.write_u32(kProfileChecksumField_00245738, static_cast<std::uint32_t>(checksum));
    return std::vector<std::uint8_t>(profile.bytes(), profile.bytes() + profile.size());
}

int EventRankState::GetStatus(std::uint8_t slice_value, std::uint32_t goal_threshold) {
    // FUN_00200018, verbatim. The comparison is unsigned in the original.
    if (slice_value == 0) return kSliceStatusNotAttempted;
    return static_cast<std::uint32_t>(slice_value) <= goal_threshold ? kSliceStatusWon
                                                                     : kSliceStatusGoalMissed;
}

int EventTimeState::GetStatus(std::int32_t slice_value, std::int32_t goal_threshold) {
    // FUN_00200068, verbatim, including both "not set" sentinels and the signed comparison.
    if (slice_value == 0 || slice_value == 0x7fffffff) return kSliceStatusNotAttempted;
    return slice_value <= goal_threshold ? kSliceStatusWon : kSliceStatusGoalMissed;
}

} // namespace hp2::memorycard
