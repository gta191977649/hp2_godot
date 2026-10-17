// TASK-0018: the event tree parsed out of the player's own FRONTEND/FRONTB.BUN.
//
// Needs the configured ZZDATA copy; skips cleanly when it is not configured, exactly like
// hp2_tpk_texture_tests. Port-side only: nothing here establishes equivalence with the original.
#include "game_core/indep/src/eventtree.hpp"
#include "platform/ps2_asset_root.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace hp2::frontend;

namespace {
void check(bool value, const char* reason) {
    if (!value) throw std::runtime_error(reason);
}

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot open " + path);
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
}

// FUN_00200DE8's switch, verbatim.
void save_sizes_match_the_goal_type_switch() {
    check(EventSaveSize_00200DE8(0) == 1 && EventSaveSize_00200DE8(2) == 1, "goal types 0 and 2");
    check(EventSaveSize_00200DE8(1) == 4 && EventSaveSize_00200DE8(5) == 4 &&
              EventSaveSize_00200DE8(6) == 4, "goal types 1, 5 and 6");
    check(EventSaveSize_00200DE8(3) == 0x10 && EventSaveSize_00200DE8(4) == 0x10, "types 3 and 4");
    check(EventSaveSize_00200DE8(99) == 1, "the original's default arm returns 1");
}

// FUN_001F75F0's two switches.
void panel_textures_match_the_status_switch() {
    check(std::string(EventPanelTexture_001F75F0(0, kEventStatusLocked)) ==
              "SPEED_EVENT_PANEL_LOCKED", "speed locked");
    check(std::string(EventPanelTexture_001F75F0(0, kEventStatusUnlocked2)) ==
              "SPEED_EVENT_PANEL_UNLOCKED", "statuses 1..3 all map to UNLOCKED");
    check(std::string(EventPanelTexture_001F75F0(0, kEventStatusWon)) == "SPEED_EVENT_PANEL_WON",
          "speed won");
    check(std::string(EventPanelTexture_001F75F0(1, kEventStatusLocked)) ==
              "PURSUIT_EVENT_PANEL_LOCKED", "pursuit locked");
    check(EventPanelTexture_001F75F0(0, 7) == nullptr, "an uncovered status has no texture");
}

// Each layout map is 200 characters and carries exactly one mark per event.
void layout_maps_are_well_formed() {
    for (const char* map : {kLayoutChampionship_0031B3C0, kLayoutHotPursuit_0031B490}) {
        std::size_t length = 0, marks = 0;
        for (const char* p = map; *p; ++p) {
            ++length;
            if (*p == 'x') ++marks;
            check(*p == 'x' || *p == ' ', "the map holds only marks and spaces");
        }
        check(length == 200, "the map is 10 rows of 20 columns");
        check(marks == 32, "one mark per event");
    }
}

void recovered_detail_text_and_multirace_slices() {
    EventTree_001FFC40 tree;
    tree.type_0x08 = 0;
    tree.records.resize(1);
    auto& record = tree.records[0];
    record.number_0x28 = 1;
    record.goal_type_0x29 = 3;
    record.Field_0x3F = 3;
    record.nTracks = 3;
    record.aTracks = {0, 0, 0};
    tree.pTracks = {5u << 14};
    record.nRewards = 1;
    record.aRewards = {0};
    tree.pRewards.push_back({1, 7, 1});
    tree.links.push_back({1, 0, 2});
    hp2::memorycard::Profile_002FC050 profile;
    const auto layout = build_profile_layout_001FE988(tree);
    auto* slice = profile.EventBlock_001FE988(0) + layout.offsets[0];
    check(tree.GetTrack(0, -1) == nullptr && tree.GetTrack(0, 3) == nullptr,
          "original track accessor rejects indices outside record count");
    check(tree.GetTrack(0, 0) && *tree.GetTrack(0, 0) == 5u << 14,
          "track accessor selects record signed index into chunk34A02");
    check(tree.GetReward(0, -1) == nullptr && tree.GetReward(0, 1) == nullptr,
          "original reward accessor rejects indices outside record count");
    check(tree.GetReward(0, 0) && (*tree.GetReward(0, 0))[1] == 7,
          "reward accessor selects eight-byte row chunk34A04");
    check(tree.Status_002010B8(profile, 0) == 1, "empty multirace event is unlocked");
    slice[0] = 0x20;
    check(tree.Status_002010B8(profile, 0) == 2, "nonzero upper nibble with zero result means progress");
    check(tree.Sub_002012D8(profile, 0) == 1, "progress counts nonzero nibbles");
    slice[0] = 0x22;
    check(tree.Status_002010B8(profile, 0) == 4, "multirace threshold includes equality");
    slice[0] = 0x23;
    check(tree.Status_002010B8(profile, 0) == 3, "multirace placing above threshold is completed");
    check(Sub_001F60E8(0) == 0xf230 && Sub_001F60E8(4) == 0xf234, "original status string IDs");
    check(Sub_001F60E8(2, "%d / %d", 2, 3, "") == "2 / 3", "status progress operands");
    check(Sub_001F60E8(4, "Won: %s", 0, 0, "second") == "Won: second", "result string operand");
    check(Sub_001F5F88(tree, 0, "$NUM_LAPS,$NUM_OPPONENTS,$NUM_RACES,$TRACK_NAME,$UNKNOWN", {}) ==
          "5,3,3,TRACK_NAME_HERE,$UNKNOWN", "original tokens and unmatched dollar preserved");
    check(Sub_001F5F88(tree, 0, std::string(520, 'a'), {}).size() == 512, "original detail buffer bound");
    check(Sub_001F5F88(tree, 0, "$TARGET_TIME", {}) == "zz:zz", "original non-time placeholder");
    tree.links[0].field_0x00 = 2;
    tree.links[0].goal_threshold_0x04 = 4 * 1000 * 65;
    check(Sub_001F5F88(tree, 0, "$TARGET_TIME", [](std::int32_t time) {
        return Sub_001A3B98(time, 0);
    }) == "1:05.00", "target time strips original padding");
    check(Sub_001A3B98(0, 0) == "    0.00", "zero timer formatting");
    check(Sub_001A3B98(4000 * 65, 1) == " 1'05\"00", "quoted locale timer formatting");
    check(Sub_001A3B98(4000 * 65, 2) == " 1:05:00", "colon locale timer formatting");
    check(Sub_001A3B98(-4000 * 65, 0) == "-1:05.00", "negative timer sign");
    check(Sub_001A3B98(INT32_MAX, 0) == "    0.00", "INT_MAX timer sentinel");
    check(Sub_001A3B98(4000 * 3661, 0) == " 1h:01:01", "hours format omits fractional field");
    check(Sub_001A3B98(4000 * 600, 0, 2) == "+10:00.0", "two-digit signed timer shifts and truncates original field");
    check(Sub_00200E78(tree, profile, 0, 3, [](std::uint32_t id) {
        return std::to_string(id);
    }, {}) == std::to_string(0xee4b), "multirace result uses original placing table");
    std::array<std::uint8_t, 8> reward{1, 7, 1};
    check(Sub_001F6220(&reward, "none", {}, [](int track, int direction) {
        return std::to_string(track) + ":" + std::to_string(direction);
    }) == "7:1", "track reward preserves ID and direction");
    reward[0] = 0;
    check(Sub_001F6220(&reward, "none", [](int car) { return std::to_string(car); }, {}) == "7",
          "car reward preserves index");
    check(Sub_001F6220(nullptr, "none", {}, {}) == "none", "no reward uses localized 101D8");
    bool threw = false;
    try { (void)Sub_001F6220(&reward, "none", {}, {}); }
    catch (const hp2::NotImplemented&) { threw = true; }
    check(threw, "missing car dependency fails loudly");

    record.goal_type_0x29 = 4;
    tree.links[0].goal_threshold_0x04 = 2;
    record.Field_0x3F = 1; // original points divisor 10+6
    std::memset(slice, 0, 16);
    slice[0] = 10; slice[1] = 6;
    check(tree.Sub_002012D8(profile, 0) == 1, "points sum divides by original table total");
    check(tree.Status_002010B8(profile, 0) == 2, "incomplete points rounds status");
    slice[0] = 30; slice[1] = 18;
    check(tree.Sub_002012D8(profile, 0) == 3, "completed points rounds");
    check(tree.Status_002010B8(profile, 0) == 4, "completed points rounds rank player first");
    slice[0] = 6; slice[1] = 18; slice[2] = 24;
    check(tree.Status_002010B8(profile, 0) == 3, "completed points rounds retain losing placing");
}

void recovered_event_logo_animation_phases() {
    FeScreen screen;
    screen.widgets.resize(2);
    auto& car = screen.widgets[0];
    auto& track = screen.widgets[1];
    car.name_hash = 0xc6398c48u; car.a = 128; car.texture_hash = 99;
    track.name_hash = 0xf60114e4u; track.a = 128; track.x = 470; track.texture_hash = 98;
    tCarLogoAnimation cars;
    cars.Bind_001F8C10(screen, 0);
    cars.Sub_001F8E18({11, 22}, 0);
    check(car.visible && car.texture_hash == 11 && car.x == 70 && car.y == 140,
          "car record binds first logo and original initial placement");
    std::uint32_t rng = 0x12345678u;
    cars.Sub_001F8FB0(0, rng);
    check(car.a == 0 && rng == 0x12345678u, "initial car record enters fade-in without randomization");
    cars.Sub_001F8FB0(1000, rng);
    check(car.a >= 63 && car.a <= 64, "initial car fade-in midpoint");
    cars.Sub_001F8FB0(1996, rng);
    check(car.a == 127, "car fade-in integer alpha truncation before endpoint");
    cars.Sub_001F8FB0(2000, rng);
    check(car.a == 127 && car.texture_hash == 11, "car clock reset does not restore endpoint alpha");
    cars.Sub_001F8FB0(22000, rng);
    check(car.a == 127, "car holds through exact five-second boundary");
    cars.Sub_001F8FB0(23000, rng);
    check(car.a >= 63 && car.a <= 64, "car fade-out midpoint after five-second hold");
    cars.Sub_001F8FB0(24000, rng);
    check(car.texture_hash == 22 && car.a == 0 && rng != 0x12345678u,
          "car swaps once after fadeout and uses original random position updates");
    const auto swapped_rng = rng;
    cars.Sub_001F8FB0(24500, rng);
    check(car.texture_hash == 22 && rng == swapped_rng, "car cycle does not randomize twice in fade-in");
    check((car.x == 70 || car.x == 150 || car.x == 210 || car.x == 260) &&
          (car.y == 140 || car.y == 280), "car positions are the original finite coordinate tables");

    tTrackLogoAnimation tracks;
    tracks.Bind_001F90F0(screen, 0);
    tracks.Sub_001F91F0({33, 44}, 0);
    check(track.x == 640 && track.visible, "rapid track change moves logo offscreen");
    tracks.Sub_001F9318(700);
    check(track.texture_hash == 98 && track.a >= 63 && track.a <= 64,
          "track fades previous texture before switch");
    tracks.Sub_001F9318(1396);
    check(track.texture_hash == 98, "track retains texture through final pre-boundary tick");
    tracks.Sub_001F9318(1410);
    check(track.texture_hash == 33 && track.a == 128 && track.x > 470 && track.x < 640,
          "track replaces texture and slides first track logo from authored right edge");
    tracks.Sub_001F9318(2484);
    check(track.x == 470 && track.texture_hash == 33, "track completes at authored position");
    tracks.Sub_001F9318(40000);
    check(track.texture_hash == 33, "track updater does not cycle subsequent logo list entries");
    tracks.Sub_001F91F0({55}, 40000);
    tracks.Sub_001F9318(40100);
    tracks.Sub_001F91F0({66}, 40100);
    check(track.x == 640, "track navigation before fade completion resets right-edge placement");

    std::uint32_t unsigned_seed = 0xffffffffu;
    check(bRandom(3, unsigned_seed) == 0,
          "original RNG uses unsigned divu even for high-bit state");
}

void trees_parse_from_the_shipped_bundle(const std::string& root) {
    const auto bundle = read_file(root + "/FRONTEND/FRONTB.BUN");
    const auto trees = parse_event_trees(bundle);
    check(trees.size() == 2, "FRONTB.BUN carries exactly two event trees");
    check(trees[0].name == "Championship", "the first tree is Championship");
    check(trees[1].name == "Hot Pursuit", "the second tree is Hot Pursuit");
    check(trees[0].type_0x08 == 0 && trees[1].type_0x08 == 1, "FUN_001FDEF8's type assignment");

    for (const auto& tree : trees) {
        check(tree.records.size() == 32, "32 records of 0xBC bytes per tree");
        check(tree.leaves.size() == 32, "one leaf per record");

        // The records are stored in event-number order, 1..32.
        for (std::size_t i = 0; i < tree.records.size(); ++i) {
            check(tree.records[i].number_0x28 == static_cast<std::uint8_t>(i + 1),
                  "records are in event-number order");
            check(!tree.records[i].name_0x08.empty(), "every event carries a name");
        }
        check(tree.FindLeafByEventNumber(1) == 0, "FUN_001FA840 finds event 1");
        check(tree.FindLeafByEventNumber(99) == -1, "an unknown number is not found");

        // Event 1 is the root: no incoming links, depth 0, and the only event that can report a
        // status above Locked without a profile.
        check(tree.leaves[0].incoming.empty(), "event 1 is a root");
        check(tree.leaves[0].depth_0x1C == 0 && tree.leaves[0].y == 0.0f, "root sits on row 0");
        check(tree.StatusWithoutProfile_002010B8(0) == kEventStatusUnlocked,
              "the root is unlocked by the profile-less reduction");
        // TASK-0021: and the recovered decoder agrees for an all-zero profile, which is what the
        // original hands its own driver before a load. Event 1 has no requirement rows, so
        // FUN_002010B8 skips the prerequisite test and decodes its slice; every zero slice means
        // "not attempted", i.e. status 1.
        const hp2::memorycard::Profile_002FC050 empty_profile;
        check(tree.Status_002010B8(empty_profile, 0) == kEventStatusUnlocked,
              "the recovered decoder agrees with the reduction for an empty profile");

        // Every other event hangs off the graph, and the whole tree is one connected component.
        int roots = 0;
        for (const auto& leaf : tree.leaves) {
            if (leaf.incoming.empty()) ++roots;
            check(leaf.group == 0, "the shipped trees are a single connected component");
            check(leaf.x >= 0.0f && leaf.x <= 19.0f * kColumnSpacing_gp_0x41B0, "x within 20 cols");
        }
        check(roots == 1, "exactly one root");

        // The map places event 1 in column 8 on both trees, and the last three events share the
        // same column as the root.
        check(tree.leaves[0].x == 8.0f * kColumnSpacing_gp_0x41B0, "event 1 is in column 8");
        for (int number : {30, 31, 32}) {
            const int leaf = tree.FindLeafByEventNumber(static_cast<std::uint8_t>(number));
            check(leaf >= 0, "late events exist");
            check(tree.leaves[static_cast<std::size_t>(leaf)].x ==
                      8.0f * kColumnSpacing_gp_0x41B0,
                  "events 30..32 converge back onto column 8");
        }

        // The profile layout must fit the 256-byte block the original hands it.
        const auto layout = build_profile_layout_001FE988(tree);
        check(layout.offsets.size() == 32, "one slice per record");
        check(layout.used > 0 && layout.used <= 0x100, "the slices fit the 0x100-byte block");
        // FUN_001FE988 runs two passes: the >= 4-byte events first, 4-byte aligned, then the
        // 1-byte events packed with no rounding. So a 1-byte event need not be aligned, and the
        // first record does not necessarily start the block.
        for (std::size_t i = 0; i < layout.offsets.size(); ++i) {
            check(layout.offsets[i] >= 0, "every record gets a slice");
            check(layout.offsets[i] < static_cast<int>(layout.kBlockSize), "slice inside block");
            const int size = EventSaveSize_00200DE8(tree.records[i].goal_type_0x29);
            if (size >= 4) check(layout.offsets[i] % 4 == 0, "pass-1 slices are 4-byte aligned");
        }
        // The two passes must not overlap: every pass-1 slice sits below every pass-2 slice.
        int first_pass_end = 0, second_pass_start = static_cast<int>(layout.kBlockSize);
        for (std::size_t i = 0; i < layout.offsets.size(); ++i) {
            const int size = EventSaveSize_00200DE8(tree.records[i].goal_type_0x29);
            if (size >= 4) {
                if (layout.offsets[i] + size > first_pass_end) first_pass_end = layout.offsets[i] + size;
            } else if (layout.offsets[i] < second_pass_start) {
                second_pass_start = layout.offsets[i];
            }
        }
        check(first_pass_end <= second_pass_start, "the two passes do not overlap");
    }

    // The two trees are genuinely different layouts, not the same data twice.
    bool differs = false;
    for (std::size_t i = 0; i < 32; ++i) {
        if (trees[0].leaves[i].x != trees[1].leaves[i].x) differs = true;
    }
    check(differs, "the Championship and Hot Pursuit layouts differ");
}

} // namespace

int main() {
    try {
        save_sizes_match_the_goal_type_switch();
        panel_textures_match_the_status_switch();
        layout_maps_are_well_formed();
        recovered_detail_text_and_multirace_slices();
        recovered_event_logo_animation_phases();

        auto root = hp2::platform::resolve_zzdata_root(".");
        if (!root) {
            std::puts("EVENTTREE_TESTS_SKIPPED: PS2 asset root not configured");
            return 0;
        }
        trees_parse_from_the_shipped_bundle(*root);
    } catch (const std::exception& error) {
        std::fputs("eventtree_tests failed: ", stderr);
        std::fputs(error.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }
    std::puts("EVENTTREE_TESTS_OK");
    return 0;
}
