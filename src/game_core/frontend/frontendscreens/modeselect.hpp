#pragma once
#include "game_core/frontend/fmenus.hpp"
#include "game_core/frontend/frontendscreens/options.hpp"
#include <cstdint>

// TASK-0016: the two mode-select screens the main menu leads to.
//
// Original source file: NOT recovered. The class names are: FUN_002174A0 allocates its 0x278-byte
// instance from a pool named "CopsOnModeSelect" (ELF 0x002BC5A8) and FUN_00217AF8 from
// "CopsOffModeSelect" (0x002BC5E0); both are registered through FUN_0016ADB0 by
// FUN_00218038 under the class names "Cops On Mode Select" and "Cops Off Mode Select", against
// layout document hashes 0x5B35F533 (FRONTB.BUN screen 1) and 0x033E7771 (screen 2).
// SLUS_203.62, SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Evidence: E-0016-01..08; reverse/functions/<binary-id>/frontend/002174a0.md and 00217af8.md.
//
// Screen 1 is the Hot Pursuit (cops on) mode list the main menu's first item opens, and screen 2
// is the cops-off list its second item opens. Their authored item text is
// Quick Race / Be The Cop / Single Challenge / Tournament and Quick Race / Challenge /
// Tournament respectively; the strings the player actually sees come from the localised table
// through the string ids below, not from those placeholders.
namespace port::frontend {

// The globals the two handlers write before leaving the screen. Named after their ELF addresses,
// because no original identifier was recovered for any of them. (E-0016-03)
struct ModeSelectGlobals {
    std::int32_t DAT_002FC06C = 0;  // 1 = entering Challenge as the cop, 0 = as the racer
    std::int32_t DAT_0031AB90 = 0;  // set to 1 on every Challenge entry
    std::int32_t DAT_0030C77C = 0;  // event-tree mode id: 9 cops-on, 8 cops-off, 2 Quick Race
    std::int32_t DAT_0030C784 = 0;  // set to 1 alongside it
    std::int32_t DAT_002FC1C0 = 0;  // cleared before entering the event tree
};

} // namespace port::frontend

namespace hp2::frontend {

// MI_<n>_MASK hashes, confirmed by StringHash_002534A8 against the immediates in FUN_00217380 and
// FUN_002179F0. (E-0016-04)
inline constexpr std::uint32_t kMask1Hash = 0x6e34e350u;
inline constexpr std::uint32_t kMask2Hash = 0x708a0bf1u;
inline constexpr std::uint32_t kMask3Hash = 0x72df3492u;
inline constexpr std::uint32_t kMask4Hash = 0x75345d33u;
inline constexpr std::uint32_t kMask5Hash = 0x778985d4u;

// What a screen's message-4 (highlight gained) handler resolves an item to: the MI_<n>_MASK quad
// the selection bars snap to, and the localised string id the scrolling text is set to.
struct HighlightTarget {
    std::uint32_t mask_hash = 0;
    std::uint32_t string_id = 0;
    bool valid = false;
};

// FUN_00217380 / FUN_002179F0: the item -> (mask, string id) tables, read from the switch
// immediates. (E-0016-04)
HighlightTarget HighlightTarget_00217380(std::uint32_t item_hash); // Cops On Mode Select
HighlightTarget HighlightTarget_002179F0(std::uint32_t item_hash); // Cops Off Mode Select

// The shared tail both highlight handlers run: bind the ghost to the highlighted item
// (FUN_002488D0), snap MI_UNDERSCORE/MI_OVERSCORE to the item's mask (FUN_00144280 +
// FUN_001B9AE8), and hand the string id to the scrolling text (FUN_002494E8). The scrolling
// text's motion is not recovered, so the id is returned rather than animated.
void ApplyHighlight_00217380(FeScreen& screen, MenuHighlightAnim_00248870& anim,
                             std::uint32_t item_hash, const HighlightTarget& target,
                             float now_seconds, const WidgetMeasure& measure);

// FUN_002174A0 message 1 (item activated). `pad_index` is the event's +0x04 field: the original
// refuses Quick Race and Tournament from any pad other than 0, and silences the event's sound
// field instead of acting. (E-0016-02)
void Sub_002174A0(int message, std::uint32_t item_hash, MenuEvent_0016A458& event,
                  ::port::frontend::ModeSelectGlobals& globals,
                  const ::port::frontend::ScreenActionServices& services);

// FUN_00217AF8 message 1. (E-0016-05)
void Sub_00217AF8(int message, std::uint32_t item_hash, MenuEvent_0016A458& event,
                  ::port::frontend::ModeSelectGlobals& globals,
                  const ::port::frontend::ScreenActionServices& services);

// FUN_00216C18: the Quick Race action. It does not open another front-end screen -- it writes the
// whole race configuration block (DAT_0031A8xx / DAT_0031ABxx), selects the event-tree mode id 2,
// branches on the cops flag the main menu set (DAT_002FC070) and hands off to the loader. None of
// that is front-end work and none of it is translated.
[[noreturn]] void Sub_00216C18();

// The front-end background movie each screen asks the manager for (FUN_0016CE30). Paths are the
// ELF's own strings. (E-0016-06)
inline constexpr const char* kMovieNewMainMenu_002BC430 = "MOVIES\\mainmenu_movie.AVI";
inline constexpr const char* kMovieCopsOnModeSelect_002BC508 = "MOVIES\\hotpursuit_movie.AVI";
inline constexpr const char* kMovieCopsOffModeSelect_002BC5C0 = "MOVIES\\worldracing_ movie.AVI";
inline constexpr const char* kMovieOptions_002B28D0 = "MOVIES\\options_movie.AVI";

} // namespace hp2::frontend
