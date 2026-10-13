// TASK-0021: the recovered save container and the 0x114E0 player profile.
//
// Needs no original asset and no save file: every fixture here is a synthetic block, so the
// test always runs. Port-side only -- nothing here establishes equivalence with the original,
// and a passing run says only that the port reproduces what was read out of the ELF.
#include "game_core/memorycard/include/memorycard.h"
#include "platform/ps2_profile_store.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace hp2::memorycard;

namespace {
void check(bool value, const char* reason) {
    if (!value) throw std::runtime_error(reason);
}

// FUN_0012DEE0's "%s%s%s", both regions. (E-0021-02)
void directory_name_matches_both_regions() {
    check(Sub_0012DEE0(kRegionNtsc_001A3390) == "BASLUS-20362HP2",
          "NTSC save directory name");
    check(Sub_0012DEE0(kRegionPal_001A3390) == "BESLES-50731HP2",
          "PAL save directory name");
}

// FUN_001878B0 folds exactly 0x4537 words, i.e. everything up to but excluding the checksum
// dword at 0x114DC. Proven by construction: flipping the last covered word changes the sum,
// and writing into the checksum word itself does not. (E-0021-06)
void checksum_covers_everything_but_itself() {
    Profile_002FC050 profile;
    check(profile.size() == kProfileSize_00245B00, "profile is 0x114E0 bytes");
    check(kChecksumWordCount_001878B0 * 4 == kProfileChecksumField_00245738,
          "0x4537 words is exactly 0x114DC bytes");

    const std::int32_t zero_sum = Checksum_001878B0(profile.bytes());
    check(zero_sum == 0, "an all-zero profile sums to zero");

    profile.write_u32(kProfileChecksumField_00245738 - 4, 0x01020304u);
    check(Checksum_001878B0(profile.bytes()) == 0x01020304,
          "the last covered word is inside the fold");

    profile.write_u32(kProfileChecksumField_00245738 - 4, 0);
    profile.write_u32(kProfileChecksumField_00245738, 0xdeadbeefu);
    check(Checksum_001878B0(profile.bytes()) == 0, "the checksum word is outside the fold");

    // The accumulator is a signed 32-bit int in the original and wraps rather than saturating.
    profile.write_u32(kProfileChecksumField_00245738, 0);
    profile.write_u32(0x100, 0x7fffffffu);
    profile.write_u32(0x104, 0x00000002u);
    check(Checksum_001878B0(profile.bytes()) == static_cast<std::int32_t>(0x80000001u),
          "the fold wraps like the original's signed accumulator");
}

// FUN_00245738 stamps the size word and the checksum; FUN_00245780 then FUN_00245D30 verify
// and copy. (E-0021-05, E-0021-06, E-0021-07)
void store_stamps_and_load_verifies() {
    Profile_002FC050 written;
    written.write_u32(kSelectedModeField_00185568, 9);
    written.SetEventTreeType_001F5D50(1);
    written.SetStartNode_001F5D28(1, 15);

    const std::vector<std::uint8_t> payload = Store_00245738(written);
    check(payload.size() == kProfileSize_00245B00, "stored payload is 0x114E0 bytes");
    check(written.read_u32(kProfileSizeField_00245738) == kProfileSize_00245B00,
          "the first word is stamped with the payload size");
    check(written.read_i32(kProfileChecksumField_00245738) ==
              Checksum_001878B0(payload.data()),
          "the last word is stamped with the checksum");

    Profile_002FC050 restored;
    check(Load_00245D30(payload, restored) == LoadResult_00245780::kOk, "a stored payload loads");
    check(restored.SelectedMode_00185568() == 9, "the mode survives the round trip");
    check(restored.EventTreeType_001F5D50() == 1, "the tree type survives");
    check(restored.StartNode_001F5D00(1) == 15, "the start node survives");

    // Storing the restored profile must reproduce the same bytes, which also proves the stamp
    // is idempotent: the size word and the checksum are already correct.
    const std::vector<std::uint8_t> again = Store_00245738(restored);
    check(again == payload, "store then load then store is byte-identical");
}

void load_rejects_a_damaged_payload() {
    Profile_002FC050 source;
    source.write_u32(0x200, 0x11223344u);
    std::vector<std::uint8_t> payload = Store_00245738(source);

    Profile_002FC050 target;
    std::vector<std::uint8_t> truncated(payload.begin(), payload.end() - 1);
    check(Load_00245D30(truncated, target) == LoadResult_00245780::kWrongLength,
          "a short payload is rejected");

    payload[0x200] ^= 0x01u;
    check(Load_00245D30(payload, target) == LoadResult_00245780::kBadChecksum,
          "a flipped bit is caught by the checksum");

    // The rejected loads must not have touched the target.
    check(target.read_u32(0x200) == 0, "a rejected load leaves the profile untouched");
}

// FUN_001F5D00 / FUN_001F5D28 / FUN_001F5D50, including the quirk that the writer selects on
// the block's stored type rather than on its own argument. (E-0021-09)
void start_node_accessors_match_the_listing() {
    Profile_002FC050 profile;
    check(profile.StartNode_001F5D00(0) == 0 && profile.StartNode_001F5D00(1) == 0,
          "an empty profile reports node 0 for both known tree types");
    check(profile.StartNode_001F5D00(2) == kDefaultStartNode_001F5D00 &&
              kDefaultStartNode_001F5D00 == 1,
          "an unknown tree type takes FUN_001F5D00's `li v0,0x1` arm");

    profile.SetEventTreeType_001F5D50(0);
    profile.SetStartNode_001F5D28(1, 7); // the `1` argument is ignored by the original
    check(profile.StartNode_001F5D00(0) == 7,
          "the writer stored against the block's type, not its argument");
    check(profile.StartNode_001F5D00(1) == 0, "the other tree's slot is untouched");

    profile.SetEventTreeType_001F5D50(1);
    profile.SetStartNode_001F5D28(0, 23);
    check(profile.StartNode_001F5D00(1) == 23, "and again for tree type 1");
    check(profile.StartNode_001F5D00(0) == 7, "tree type 0 kept its value");

    profile.SetEventTreeType_001F5D50(5);
    profile.SetStartNode_001F5D28(0, 31);
    check(profile.StartNode_001F5D00(0) == 7 && profile.StartNode_001F5D00(1) == 23,
          "a stored type the writer's two arms do not cover writes nothing");
}

// The two 0x100-byte blocks FUN_001FE988 slices. (E-0021-07)
void event_blocks_sit_where_the_loader_puts_them() {
    Profile_002FC050 profile;
    check(kEventBlockChampionship_002FC1D8 == 0x0188 && kEventBlockHotPursuit_002FC2D8 == 0x0288,
          "0x002FC1D8 and 0x002FC2D8 relative to the 0x002FC050 base");
    check(kEventBlockChampionship_002FC1D8 + kEventBlockSize_001FE988 ==
              kEventBlockHotPursuit_002FC2D8,
          "the two blocks are adjacent and 0x100 apart");
    check(profile.EventBlock_001FE988(0) == profile.bytes() + 0x0188, "Championship block");
    check(profile.EventBlock_001FE988(1) == profile.bytes() + 0x0288, "Hot Pursuit block");
    check(profile.EventBlock_001FE988(2) == nullptr, "no block for an unknown tree type");
    check(kSelectedModeField_00185568 == 0x1072c &&
              kProfileChecksumField_00245738 + 4 == kProfileSize_00245B00,
          "the mode selector and the checksum sit at the recovered offsets");
}

// FUN_00200018 and FUN_00200068, both switch arms, against the decompiled truth table.
// (E-0021-11)
void slice_decoders_match_the_decompiled_switches() {
    check(SliceStatusByte_00200018(0, 3) == kSliceStatusNotAttempted, "byte 0 is not attempted");
    check(SliceStatusByte_00200018(1, 3) == kSliceStatusWon, "byte at or under the goal wins");
    check(SliceStatusByte_00200018(3, 3) == kSliceStatusWon, "byte exactly at the goal wins");
    check(SliceStatusByte_00200018(4, 3) == kSliceStatusGoalMissed, "byte over the goal misses");
    // The original compares unsigned, so a 0xFF byte is 255, never -1.
    check(SliceStatusByte_00200018(0xff, 0x80) == kSliceStatusGoalMissed,
          "the byte comparison is unsigned");

    check(SliceStatusWord_00200068(0, 100) == kSliceStatusNotAttempted, "word 0 is not attempted");
    check(SliceStatusWord_00200068(0x7fffffff, 100) == kSliceStatusNotAttempted,
          "0x7FFFFFFF is the second not-attempted sentinel");
    check(SliceStatusWord_00200068(100, 100) == kSliceStatusWon, "word exactly at the goal wins");
    check(SliceStatusWord_00200068(101, 100) == kSliceStatusGoalMissed, "word over the goal misses");
    // The original compares signed, so a negative stored value beats any positive goal.
    check(SliceStatusWord_00200068(-5, 100) == kSliceStatusWon,
          "the word comparison is signed");
}

// The host half: hp2::platform::store_profile writes through the recovered Store_00245738 and
// hp2::platform::load_profile reads back through Load_00245D30. Port-side plumbing, not
// recovered behaviour -- the original writes to a memory card, which has no host equivalent.
void host_store_and_load_round_trip() {
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / "hp2_memorycard_tests";
    std::error_code ec;
    fs::remove_all(root, ec);
#ifdef _WIN32
    _putenv_s(hp2::platform::kSaveRootEnvVar, root.string().c_str());
    _putenv_s(hp2::platform::kProfilePathEnvVar, "");
#else
    setenv(hp2::platform::kSaveRootEnvVar, root.string().c_str(), 1);
    unsetenv(hp2::platform::kProfilePathEnvVar);
#endif

    const int region = kRegionNtsc_001A3390;
    const std::string expected =
        (root / "BASLUS-20362HP2" / "settings.db").generic_string();
    check(hp2::platform::profile_store_path(".", region) == expected,
          "the store path is <save root>/BASLUS-20362HP2/settings.db");
    check(!hp2::platform::resolve_profile_path(".", region).has_value(),
          "nothing resolves before anything is written");

    Profile_002FC050 written;
    written.write_u32(kSelectedModeField_00185568, 8);
    written.SetEventTreeType_001F5D50(0);
    written.SetStartNode_001F5D28(0, 12);
    const std::string store_error = hp2::platform::store_profile(".", region, written);
    check(store_error.empty(), store_error.empty() ? "stored" : store_error.c_str());
    check(fs::file_size(fs::path(expected), ec) == kProfileSize_00245B00,
          "the written file is 0x114E0 bytes");

    // A profile the port wrote must be picked up on the next run with no configuration.
    auto resolved = hp2::platform::resolve_profile_path(".", region);
    check(resolved.has_value() && *resolved == expected, "the written profile resolves itself");

    Profile_002FC050 restored;
    const hp2::platform::ProfileLoad load = hp2::platform::load_profile(".", region, restored);
    check(load.loaded, load.error.empty() ? "loaded" : load.error.c_str());
    check(restored.SelectedMode_00185568() == 8, "the mode survives the host round trip");
    check(restored.StartNode_001F5D00(0) == 12, "the start node survives");

    // A corrupted file is reported, not thrown, and leaves the profile alone.
    {
        std::fstream file(fs::path(expected), std::ios::binary | std::ios::in | std::ios::out);
        check(static_cast<bool>(file), "reopen the written profile");
        file.seekp(0x300);
        const char flipped = 0x5a;
        file.write(&flipped, 1);
    }
    Profile_002FC050 untouched;
    const hp2::platform::ProfileLoad bad = hp2::platform::load_profile(".", region, untouched);
    check(!bad.loaded && bad.error == "bad checksum", "a corrupted file reports bad checksum");
    check(untouched.read_u8(0x300) == 0, "a rejected host load leaves the profile untouched");

    fs::remove_all(root, ec);
}

} // namespace

int main() {
    try {
        directory_name_matches_both_regions();
        checksum_covers_everything_but_itself();
        store_stamps_and_load_verifies();
        load_rejects_a_damaged_payload();
        start_node_accessors_match_the_listing();
        event_blocks_sit_where_the_loader_puts_them();
        slice_decoders_match_the_decompiled_switches();
        host_store_and_load_round_trip();
    } catch (const std::exception& error) {
        std::fputs("memorycard_tests failed: ", stderr);
        std::fputs(error.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }
    std::puts("MEMORYCARD_TESTS_OK");
    return 0;
}
