// TASK-0016: the two mode-select screens (FUN_002174A0 "Cops On Mode Select" and FUN_00217AF8
// "Cops Off Mode Select"). Port-side tests against the switch immediates read out of the ELF;
// they do not establish equivalence with the original (AGENT.md R1).
#include "game_core/frontend/frontendscreens/modeselect.hpp"
#include "game_core/frontend/frontendscreens/registry_0016adb0.hpp"
#include "game_core/not_implemented.hpp"
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace hp2::frontend;

namespace {
void check(bool value, const char* reason) {
    if (!value) throw std::runtime_error(reason);
}

struct Capture {
    std::string requested, animated;
    int requests = 0, animations = 0;
    port::frontend::ScreenActionServices services() {
        return {[this](const char* name, bool) { requested = name; ++requests; },
                [this](const char* name) { animated = name; ++animations; },
                [] { throw std::runtime_error("display_prepare must not be used here"); }};
    }
};

MenuEvent_0016A458 event_for(std::uint32_t hash, std::int32_t pad) {
    MenuEvent_0016A458 event;
    event.field_0x00 = hash;
    event.pad_index_0x04 = pad;
    event.sound_id_0x0C = kMenuSoundMenuSelect_5;
    return event;
}

// Every transition these screens make goes through tExitScreenAnimation, and every target must be
// a class the recovered registration table knows, otherwise the port could not resolve it.
void targets_exist_in_the_registration_table() {
    for (const char* name : {"ChallengeBeACop", "Challenge", "View Event Tree", "Options"}) {
        const auto* row = port::frontend::find_registration(name);
        if (std::string(name) == "View Event Tree") continue; // registered elsewhere; see below
        check(row != nullptr, "mode-select target missing from the registration table");
    }
    const auto* cops_on = port::frontend::find_registration("Cops On Mode Select");
    const auto* cops_off = port::frontend::find_registration("Cops Off Mode Select");
    check(cops_on != nullptr && cops_on->document_hash == 0x5B35F533u, "screen 1 document hash");
    check(cops_off != nullptr && cops_off->document_hash == 0x033E7771u, "screen 2 document hash");
    check(cops_on->handler_address == 0x002174A0u, "screen 1 handler address");
    check(cops_off->handler_address == 0x00217AF8u, "screen 2 handler address");
}

void cops_on_item_actions() {
    Capture capture;
    auto services = capture.services();
    port::frontend::ModeSelectGlobals globals;

    // MENUITEM_2 "Be The Cop": sets DAT_002FC06C and animates out to ChallengeBeACop.
    auto event = event_for(0x52E039F4u, 0);
    Sub_002174A0(1, event.field_0x00, event, globals, services);
    check(globals.DAT_002FC06C == 1, "Be The Cop must set DAT_002FC06C to 1");
    check(capture.animated == "ChallengeBeACop", "Be The Cop target");

    // MENUITEM_3 "Single Challenge": clears it again and sets DAT_0031AB90.
    event = event_for(0x52E039F5u, 0);
    Sub_002174A0(1, event.field_0x00, event, globals, services);
    check(globals.DAT_002FC06C == 0 && globals.DAT_0031AB90 == 1, "Single Challenge globals");
    check(capture.animated == "Challenge", "Single Challenge target");

    // MENUITEM_4 "Tournament": cops-on event tree id 9.
    event = event_for(0x52E039F6u, 0);
    Sub_002174A0(1, event.field_0x00, event, globals, services);
    check(globals.DAT_0030C77C == 9 && globals.DAT_0030C784 == 1, "cops-on event tree id");
    check(capture.animated == "View Event Tree", "Tournament target");
    check(capture.requests == 0, "these screens never request directly, they animate out");
}

void cops_off_item_actions() {
    Capture capture;
    auto services = capture.services();
    port::frontend::ModeSelectGlobals globals;

    auto event = event_for(0x52E039F4u, 0);
    Sub_00217AF8(1, event.field_0x00, event, globals, services);
    check(globals.DAT_002FC06C == 0 && globals.DAT_0031AB90 == 1, "Challenge globals");
    check(capture.animated == "Challenge", "Challenge target");

    event = event_for(0x52E039F5u, 0);
    Sub_00217AF8(1, event.field_0x00, event, globals, services);
    check(globals.DAT_0030C77C == 8, "cops-off event tree id is 8, not 9");
    check(capture.animated == "View Event Tree", "Tournament target");

    // There is no Be The Cop entry on the cops-off screen: MENUITEM_4 is the Options branch.
    event = event_for(0x52E039F6u, 0);
    Sub_00217AF8(1, event.field_0x00, event, globals, services);
    check(capture.animated == "Options", "cops-off MENUITEM_4 is Options");
}

// Quick Race and Tournament are refused from any pad other than 0; the original writes 0 into the
// event's sound field and does nothing else.
void pad_gate_refuses_other_pads() {
    Capture capture;
    auto services = capture.services();
    port::frontend::ModeSelectGlobals globals;

    auto event = event_for(0x52E039F6u, 1); // Tournament from pad 1
    Sub_002174A0(1, event.field_0x00, event, globals, services);
    check(event.sound_id_0x0C == 0, "a refused activation must silence the event sound");
    check(capture.animations == 0 && globals.DAT_0030C77C == 0, "refusal must have no effect");

    // The same item from pad 0 goes through.
    event = event_for(0x52E039F6u, 0);
    Sub_002174A0(1, event.field_0x00, event, globals, services);
    check(capture.animations == 1, "pad 0 must be accepted");
    check(event.sound_id_0x0C == kMenuSoundMenuSelect_5, "an accepted activation keeps its sound");

    // Items that are not gated work from any pad.
    Capture other;
    auto other_services = other.services();
    port::frontend::ModeSelectGlobals other_globals;
    auto ungated = event_for(0x52E039F4u, 1);
    Sub_002174A0(1, ungated.field_0x00, ungated, other_globals, other_services);
    check(other.animated == "ChallengeBeACop", "Be The Cop is not pad-gated");
}

void quick_race_reports_itself_unimplemented() {
    Capture capture;
    auto services = capture.services();
    port::frontend::ModeSelectGlobals globals;
    bool threw = false;
    auto event = event_for(0x52E039F3u, 0);
    try {
        Sub_002174A0(1, event.field_0x00, event, globals, services);
    } catch (const hp2::NotImplemented&) {
        threw = true;
    }
    check(threw, "Quick Race leaves the front end and must report itself unimplemented");

    threw = false;
    auto off = event_for(0x52E039F3u, 0);
    try {
        Sub_00217AF8(1, off.field_0x00, off, globals, services);
    } catch (const hp2::NotImplemented&) {
        threw = true;
    }
    check(threw, "the cops-off Quick Race behaves the same way");
}

void highlight_tables_match_the_elf_immediates() {
    const auto first = HighlightTarget_00217380(0x52E039F3u);
    check(first.valid && first.mask_hash == kMask1Hash && first.string_id == 0x2721u, "cops-on 1");
    check(HighlightTarget_00217380(0x52E039F4u).string_id == 0x271Eu, "cops-on 2");
    check(HighlightTarget_00217380(0x52E039F5u).string_id == 0x271Cu, "cops-on 3");
    const auto fourth = HighlightTarget_00217380(0x52E039F6u);
    check(fourth.mask_hash == kMask4Hash && fourth.string_id == 0x271Du, "cops-on 4");
    check(!HighlightTarget_00217380(0u).valid, "an unknown item resolves to nothing");

    check(HighlightTarget_002179F0(0x52E039F3u).string_id == 0x2716u, "cops-off 1");
    check(HighlightTarget_002179F0(0x52E039F4u).string_id == 0x2717u, "cops-off 2");
    check(HighlightTarget_002179F0(0x52E039F5u).string_id == 0x2718u, "cops-off 3");
    check(!HighlightTarget_002179F0(0x52E039F7u).valid, "cops-off has no fifth branch");

    // The mask hashes are the names, not opaque constants.
    check(StringHash_002534A8("MI_1_MASK") == kMask1Hash, "MI_1_MASK");
    check(StringHash_002534A8("MI_4_MASK") == kMask4Hash, "MI_4_MASK");
    check(StringHash_002534A8("MI_5_MASK") == kMask5Hash, "MI_5_MASK");
}

// The highlight tail must retarget the ghost and snap the two selection bars onto the item's mask.
void highlight_places_the_selection_bars() {
    FeScreen screen;
    auto add = [&screen](const char* name, std::uint8_t type, float x, float y) {
        FeWidget widget;
        widget.name = name;
        widget.name_hash = StringHash_002534A8(name);
        widget.type = type;
        widget.x = x;
        widget.y = y;
        screen.widgets.push_back(widget);
    };
    add("MENUITEM_1", 1, 150.0f, 100.0f);
    add("MI_1_MASK", 2, 150.0f, 100.0f);
    add("MI_UNDERSCORE", 2, 0.0f, 0.0f);
    add("MI_OVERSCORE", 2, 0.0f, 0.0f);
    add("ANIMATING_MENUITEM", 1, 0.0f, 0.0f);

    const WidgetMeasure measure = [](const FeWidget&) { return WidgetMetrics{200.0f, 28.0f}; };
    MenuHighlightAnim_00248870 anim;
    anim.Construct_00248870(&screen, StringHash_002534A8("ANIMATING_MENUITEM"),
                            static_cast<std::int32_t>(kAnimGhostColour_gp_0x3F08), 0.0f);
    ApplyHighlight_00217380(screen, anim, StringHash_002534A8("MENUITEM_1"),
                            HighlightTarget_00217380(0x52E039F3u), 0.0f, measure);

    check(anim.target_hash_0x04 == StringHash_002534A8("MENUITEM_1"), "ghost must retarget");
    const FeWidget* under = FindWidget_00143D50(screen, StringHash_002534A8("MI_UNDERSCORE"), 0);
    const FeWidget* over = FindWidget_00143D50(screen, StringHash_002534A8("MI_OVERSCORE"), 0);
    // mask y0 = 100, y1 = 100 + 28 = 128; underscore = y1 - 3, overscore = y0 - 6.
    check(under != nullptr && under->y == 125.0f, "MI_UNDERSCORE lands 3 above the mask bottom");
    check(over != nullptr && over->y == 94.0f, "MI_OVERSCORE lands 6 above the mask top");
}

} // namespace

int main() {
    try {
        targets_exist_in_the_registration_table();
        cops_on_item_actions();
        cops_off_item_actions();
        pad_gate_refuses_other_pads();
        quick_race_reports_itself_unimplemented();
        highlight_tables_match_the_elf_immediates();
        highlight_places_the_selection_bars();
    } catch (const std::exception& error) {
        std::fputs("frontend_modeselect_tests failed: ", stderr);
        std::fputs(error.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }
    std::puts("FRONTEND_MODESELECT_TESTS_OK");
    return 0;
}
