// TASK-0018: the event tree parsed out of the player's own FRONTEND/FRONTB.BUN.
//
// Needs the configured ZZDATA copy; skips cleanly when it is not configured, exactly like
// hp2_tpk_texture_tests. Port-side only: nothing here establishes equivalence with the original.
#include "game_core/frontend/eventtree.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cstdio>
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
        check(tree.FindLeafByNumber_001FA840(1) == 0, "FUN_001FA840 finds event 1");
        check(tree.FindLeafByNumber_001FA840(99) == -1, "an unknown number is not found");

        // Event 1 is the root: no incoming links, depth 0, and the only event that can report a
        // status above Locked without a profile.
        check(tree.leaves[0].incoming.empty(), "event 1 is a root");
        check(tree.leaves[0].depth_0x1C == 0 && tree.leaves[0].y == 0.0f, "root sits on row 0");
        check(tree.Status_002010B8(0) == kEventStatusUnlocked, "the root is unlocked");

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
            const int leaf = tree.FindLeafByNumber_001FA840(static_cast<std::uint8_t>(number));
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
