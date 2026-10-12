#include "game_core/frontend/frontendscreens/modeselect.hpp"
#include "game_core/not_implemented.hpp"

namespace hp2::frontend {
namespace {

// FUN_00217020 / FUN_002176D8 are the same function body on each screen: allocate a 0x1C-byte
// "tExitScreenAnimation" (pool-name string 0x002BC528), construct it with the target class name
// through FUN_002490D8, park it in the instance at +0x270, and start MI_UNDERSCORE moving at
// 650 px/s. While +0x270 is non-null both dispatchers route every message to FUN_00249128
// instead of their own handling, so the screen is locked until the animation finishes and the
// screen request fires. The port expresses that as the `exit_animation` service. (E-0016-07)
void ExitTo_00217020(const port::frontend::ScreenActionServices& services, const char* class_name) {
    if (!services.exit_animation) {
        HP2_NOT_IMPLEMENTED("0x00217020", "exit-screen-animation service missing");
    }
    services.exit_animation(class_name);
}

// Both screens gate two of their items on the event's +0x04 field being 0, writing 0 into the
// event's sound field (+0x0C) and returning otherwise. Reproduced verbatim: the original neither
// plays MenuWrong nor reports anything, it just drops the activation.
bool PadGate(MenuEvent_0016A458& event) {
    if (event.pad_index_0x04 == 0) return true;
    event.sound_id_0x0C = 0;
    return false;
}

} // namespace

HighlightTarget HighlightTarget_00217380(std::uint32_t item_hash) {
    switch (item_hash) {
        case 0x52e039f3u: return {kMask1Hash, 0x2721u, true}; // MENUITEM_1 Quick Race
        case 0x52e039f4u: return {kMask2Hash, 0x271eu, true}; // MENUITEM_2 Be The Cop
        case 0x52e039f5u: return {kMask3Hash, 0x271cu, true}; // MENUITEM_3 Single Challenge
        case 0x52e039f6u: return {kMask4Hash, 0x271du, true}; // MENUITEM_4 Tournament
        case 0x52e039f7u: return {kMask5Hash, 0x2719u, true}; // MENUITEM_5 (no such widget on
                                                              // screen 1; the branch exists)
        default: return {};                                   // the original returns immediately
    }
}

HighlightTarget HighlightTarget_002179F0(std::uint32_t item_hash) {
    switch (item_hash) {
        case 0x52e039f3u: return {kMask1Hash, 0x2716u, true}; // MENUITEM_1 Quick Race
        case 0x52e039f4u: return {kMask2Hash, 0x2717u, true}; // MENUITEM_2 Challenge
        case 0x52e039f5u: return {kMask3Hash, 0x2718u, true}; // MENUITEM_3 Tournament
        case 0x52e039f6u: return {kMask4Hash, 0x2719u, true}; // MENUITEM_4 (no such widget on
                                                              // screen 2; the branch exists)
        default: return {};
    }
}

void ApplyHighlight_00217380(FeScreen& screen, MenuHighlightAnim_00248870& anim,
                             std::uint32_t item_hash, const HighlightTarget& target,
                             float now_seconds, const WidgetMeasure& measure) {
    // FUN_002488D0 runs before the switch in both handlers, so the ghost retargets even for an
    // item the table does not know.
    anim.Bind_002488D0(item_hash, now_seconds);
    if (!target.valid) return;
    PlaceSelectionBars_00216880(screen, target.mask_hash, measure);
    // FUN_002494E8(scrolling text, string id) and FUN_0016E270(instance+4, mask hash) follow in
    // the original. The first needs the scrolling-text object (Sub_00249348, not translated) and
    // the second a visibility-group table that has not been recovered; neither is faked here.
}

void Sub_002174A0(int message, std::uint32_t item_hash, MenuEvent_0016A458& event,
                  port::frontend::ModeSelectGlobals& globals,
                  const port::frontend::ScreenActionServices& services) {
    if (message != 1) return;
    switch (item_hash) {
        case 0x52e039f3u: // Quick Race
            if (!PadGate(event)) return;
            Sub_00216C18(); // [[noreturn]]: control never reaches the next label
        case 0x52e039f4u: // Be The Cop
            globals.DAT_002FC06C = 1;
            ExitTo_00217020(services, "ChallengeBeACop");
            return;
        case 0x52e039f5u: // Single Challenge
            globals.DAT_0031AB90 = 1;
            globals.DAT_002FC06C = 0;
            ExitTo_00217020(services, "Challenge");
            return;
        case 0x52e039f6u: // Tournament
            if (!PadGate(event)) return;
            globals.DAT_0030C77C = 9; // cops-on event tree
            globals.DAT_0030C784 = 1;
            // FUN_001F5D50(0x2FC1B4, 1) selects the cops-on event tree data; not translated.
            globals.DAT_002FC1C0 = 0;
            ExitTo_00217020(services, "View Event Tree");
            return;
        case 0x52e039f7u: // MENUITEM_5; screen 1 declares no such widget
            ExitTo_00217020(services, "Options");
            return;
        default:
            return; // the original's switch has no default arm
    }
}

void Sub_00217AF8(int message, std::uint32_t item_hash, MenuEvent_0016A458& event,
                  port::frontend::ModeSelectGlobals& globals,
                  const port::frontend::ScreenActionServices& services) {
    if (message != 1) return;
    switch (item_hash) {
        case 0x52e039f3u: // Quick Race
            if (!PadGate(event)) return;
            Sub_00216C18(); // [[noreturn]]: control never reaches the next label
        case 0x52e039f4u: // Challenge
            globals.DAT_0031AB90 = 1;
            globals.DAT_002FC06C = 0;
            ExitTo_00217020(services, "Challenge");
            return;
        case 0x52e039f5u: // Tournament
            if (!PadGate(event)) return;
            globals.DAT_0030C784 = 1;
            globals.DAT_0030C77C = 8; // cops-off event tree
            // FUN_001F5D50(0x2FC1B4, 0) selects the cops-off event tree data; not translated.
            globals.DAT_002FC1C0 = 0;
            ExitTo_00217020(services, "View Event Tree");
            return;
        case 0x52e039f6u: // MENUITEM_4; screen 2 declares no such widget
            ExitTo_00217020(services, "Options");
            return;
        default:
            return;
    }
}

void Sub_00216C18() {
    HP2_NOT_IMPLEMENTED("0x00216C18",
                        "Quick Race leaves the front end: it writes the race configuration block "
                        "and hands off to the loader, which is not front-end work");
}

} // namespace hp2::frontend
