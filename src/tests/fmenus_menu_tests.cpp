// Tests for the recovered front-end menu runtime (TASK-0014).
//
// These are port-side tests: they prove the translation reproduces what the decompiled ELF code
// says, using the literal constants and hashes read out of the binary as oracles. They do NOT
// establish equivalence with the original hardware (AGENTS.md rule 11 / AGENT.md R1), and nothing
// here may be used to promote a status to Verified.
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/input/joyevents.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cstdio>
#include <stdexcept>
#include <utility>
#include <string>
#include <vector>

using namespace hp2::frontend;
namespace {
void require(bool value, const char* reason) {
    if (!value) throw std::runtime_error(reason);
}

// Builds a screen holding `count` type-1 MENUITEM_n text widgets (n starting at 1, as every
// FRONTB.BUN screen does) plus one unrelated widget, each with a distinct authored colour.
FeScreen make_menu_screen(int count) {
    FeScreen screen;
    screen.index = 0;
    for (int i = 1; i <= count; ++i) {
        FeWidget widget;
        widget.name = "MENUITEM_" + std::to_string(i);
        widget.name_hash = StringHash_002534A8(widget.name.c_str());
        widget.type = 1;
        widget.r = static_cast<std::uint8_t>(0x10 + i);
        widget.g = 0x7f;
        widget.b = 0x7f;
        widget.a = 0x80;
        screen.widgets.push_back(widget);
    }
    FeWidget other;
    other.name = "GRID";
    other.name_hash = StringHash_002534A8("GRID");
    other.type = 2;
    screen.widgets.push_back(other);
    return screen;
}

MenuGroup_0016AA00 make_group(FeScreen& screen, MenuCallback callback = {}) {
    MenuGroup_0016AA00 group;
    group.screen_0x14 = &screen;
    group.pad_index_0x10 = -1;
    // FUN_0016A0F0 takes a LIST POSITION, not a MENUITEM_%d suffix: items are appended in
    // scan order, so position 0 is MENUITEM_1 on a screen that starts its items at 1.
    group.initial_index_0x0C = 0;
    group.callback_0x18 = std::move(callback);
    return group;
}

// FUN_0016B478 passes these four hashes as literal immediates; FUN_00169F78 passes 0x27C5F4AB.
void string_hash_matches_elf_immediates() {
    require(StringHash_002534A8("POPUP_1") == 0xe0105663u, "POPUP_1 hash");
    require(StringHash_002534A8("POPUP_2") == 0xe0105664u, "POPUP_2 hash");
    require(StringHash_002534A8("POPUP_3") == 0xe0105665u, "POPUP_3 hash");
    require(StringHash_002534A8("ANIMATING_MENUITEM") == 0xab671c9au, "ANIMATING_MENUITEM hash");
    require(StringHash_002534A8(kHighlightBarName) == kHighlightBarNameHash_00169F78,
            "HIGHLIGHT_BAR must hash to the constant FUN_00169F78 looks up");
}

void menu_item_index_reverse_lookup() {
    for (int i = 0; i < kMenuItemIndexLimit_0016A330; ++i) {
        const std::string name = "MENUITEM_" + std::to_string(i);
        require(MenuItemIndexFromNameHash_0016A330(StringHash_002534A8(name.c_str())) == i,
                "FUN_0016A330 must round-trip every index it scans");
    }
    // The original's own fallback for an unknown hash is 0, not an error.
    require(MenuItemIndexFromNameHash_0016A330(StringHash_002534A8("GRID")) == 0,
            "unknown hash must fall back to 0");
}

// FUN_00169F78 only appends items for names the screen actually declares, scanning 0..15.
void menu_list_only_takes_declared_items() {
    FeScreen screen = make_menu_screen(3);
    MenuGroup_0016AA00 group = make_group(screen);
    group.EnsureMenu_0016A858();
    require(group.child_0x08 != nullptr, "EnsureMenu must create the list");
    require(group.child_0x08->count() == 3, "only the three declared MENUITEM widgets may appear");
    const auto items = group.child_0x08->items();
    require(items[0]->index_0x10 == 1 && items[2]->index_0x10 == 3,
            "item indices must be the MENUITEM_%d suffixes, not 0-based positions");
}

std::uint32_t widget_rgb(const FeScreen& screen, const char* name) {
    for (const FeWidget& widget : screen.widgets) {
        if (widget.name == name) {
            return static_cast<std::uint32_t>(widget.r) | (static_cast<std::uint32_t>(widget.g) << 8) |
                   (static_cast<std::uint32_t>(widget.b) << 16);
        }
    }
    throw std::runtime_error("widget not found");
}

// (state & 3): 1 -> authored colour, 3 -> highlight colour, 0/2 -> disabled colour.
void state_bits_select_the_recovered_colours() {
    FeScreen screen = make_menu_screen(2);
    MenuGroup_0016AA00 group = make_group(screen);
    group.EnsureMenu_0016A858();
    auto items = group.child_0x08->items();

    const std::uint32_t highlight = 0x80u | (0x40u << 8) | (0x00u << 16);
    const std::uint32_t disabled = 0x18u | (0x18u << 8) | (0x18u << 16);

    require(widget_rgb(screen, "MENUITEM_1") == highlight,
            "RefreshAll must leave the first enabled item on the highlight colour");
    require(items[0]->highlighted(), "first item must be highlighted after refresh");

    items[1]->Highlight_00169968({});
    require(widget_rgb(screen, "MENUITEM_2") == highlight, "highlighted item takes 0x80,0x40,0x00");
    items[1]->Unhighlight_001699F8({});
    require(widget_rgb(screen, "MENUITEM_2") == (0x12u | (0x7fu << 8) | (0x7fu << 16)),
            "unhighlighted enabled item returns to its authored colour");

    items[1]->ClearStateBits_00169CB0(kMenuThingStateEnabled_bit0);
    require(widget_rgb(screen, "MENUITEM_2") == disabled,
            "disabled item takes the 0x18,0x18,0x18 colour");
}

// FUN_00140CA0 keeps the alpha byte and only replaces the low 24 bits.
void colour_write_preserves_alpha_and_stamps_dirty() {
    FeWidget widget;
    widget.r = 1;
    widget.g = 2;
    widget.b = 3;
    widget.a = 0x44;
    SetWidgetColour_00140CA0(widget, kHighlightColour_gp_0x5330);
    require(widget.a == 0x44, "FUN_00140CA0 must not touch the alpha byte");
    require(widget.r == 0x80 && widget.g == 0x40 && widget.b == 0x00, "rgb must be replaced");
    require(widget.dirty_0x2b == 7, "a real change must stamp +0x2b with 7");
    widget.dirty_0x2b = 0;
    SetWidgetColour_00140CA0(widget, kHighlightColour_gp_0x5330);
    require(widget.dirty_0x2b == 0, "an unchanged write must not stamp +0x2b");
}

// gp-0x533C is 1 in the shipped image, so FUN_0016A5E0/FUN_0016A6C8 wrap around.
void navigation_wraps_in_both_directions() {
    FeScreen screen = make_menu_screen(3);
    MenuGroup_0016AA00 group = make_group(screen);
    group.EnsureMenu_0016A858();
    MenuList_00169F78& list = *group.child_0x08;

    require(list.CurrentIndex_0016A130() == 0, "refresh must land on the first item");
    list.Next_0016A5E0();
    require(list.CurrentIndex_0016A130() == 1, "next moves forward");
    list.Next_0016A5E0();
    require(list.CurrentIndex_0016A130() == 2, "next moves forward");
    list.Next_0016A5E0();
    require(list.CurrentIndex_0016A130() == 0, "next past the tail wraps to the head");
    list.Previous_0016A6C8();
    require(list.CurrentIndex_0016A130() == 2, "previous past the head wraps to the tail");
}

// The same traversal must step over items whose enabled bit is clear.
void navigation_skips_disabled_items() {
    FeScreen screen = make_menu_screen(3);
    MenuGroup_0016AA00 group = make_group(screen);
    group.EnsureMenu_0016A858();
    MenuList_00169F78& list = *group.child_0x08;
    auto items = list.items();
    items[1]->ClearStateBits_00169CB0(kMenuThingStateEnabled_bit0);

    require(list.CurrentIndex_0016A130() == 0, "start on the first item");
    list.Next_0016A5E0();
    require(list.CurrentIndex_0016A130() == 2, "the disabled middle item must be skipped");
    list.Previous_0016A6C8();
    require(list.CurrentIndex_0016A130() == 0, "previous must skip it as well");
}

// A single enabled item must not move, and must not fire a highlight change.
void single_item_list_does_not_move() {
    FeScreen screen = make_menu_screen(1);
    MenuGroup_0016AA00 group = make_group(screen);
    group.EnsureMenu_0016A858();
    MenuList_00169F78& list = *group.child_0x08;
    list.Next_0016A5E0();
    list.Previous_0016A6C8();
    require(list.CurrentIndex_0016A130() == 0, "a one-item list stays put");
}

// FUN_0016A458 maps SCROLL_UP/DOWN and the analog equivalents onto the list moves, with the
// recovered sound ids, and the callback can veto the move through result_0x08.
void joy_events_drive_the_list_with_recovered_sounds() {
    FeScreen screen = make_menu_screen(3);
    std::vector<int> messages;
    MenuGroup_0016AA00 group = make_group(screen, [&](int message, MenuEvent_0016A458&) {
        messages.push_back(message);
    });
    group.EnsureMenu_0016A858();
    MenuList_00169F78& list = *group.child_0x08;

    group.HandleJoyEvent_0016AA00(0, hp2::input::JOY_EVENT_SCROLL_DOWN, true);
    require(list.CurrentIndex_0016A130() == 1, "SCROLL_DOWN moves to the next item");
    require(last_menu_sound_request().sound_id == kMenuSoundMenuUpDown_4,
            "SCROLL_DOWN must request sound id 4");
    require(std::string(last_menu_sound_request().name) == "MenuUpDown",
            "sound id 4 is the original MenuUpDown event");

    group.HandleJoyEvent_0016AA00(0, hp2::input::JOY_EVENT_MENU_ANALOG_UP, true);
    require(list.CurrentIndex_0016A130() == 0, "MENU_ANALOG_UP is grouped with SCROLL_UP");
    require(last_menu_sound_request().sound_id == kMenuSoundMenuUpDown_3,
            "SCROLL_UP must request sound id 3");

    // A released event is dropped by the original's `param_4 == 0` gate.
    group.HandleJoyEvent_0016AA00(0, hp2::input::JOY_EVENT_SCROLL_DOWN, false);
    require(list.CurrentIndex_0016A130() == 0, "a released event must not move the selection");

    require(!messages.empty(), "the callback must see messages");
}

void callback_can_veto_a_move() {
    FeScreen screen = make_menu_screen(3);
    MenuGroup_0016AA00 group = make_group(screen, [](int message, MenuEvent_0016A458& event) {
        if (message == kMenuMessage0x09) event.result_0x08 = 1; // non-zero vetoes
    });
    group.EnsureMenu_0016A858();
    group.HandleJoyEvent_0016AA00(0, hp2::input::JOY_EVENT_SCROLL_DOWN, true);
    require(group.child_0x08->CurrentIndex_0016A130() == 0,
            "a non-zero result_0x08 must suppress the move");
}

// FUN_00169D98: START and MENU_SELECT raise message 1 with sound 5 on the current item.
void select_activates_the_current_item() {
    FeScreen screen = make_menu_screen(3);
    std::vector<std::pair<int, std::uint32_t>> activations;
    MenuGroup_0016AA00 group = make_group(screen, [&](int message, MenuEvent_0016A458& event) {
        if (message == kMenuMessage0x01) activations.emplace_back(message, event.field_0x00);
    });
    group.EnsureMenu_0016A858();
    group.HandleJoyEvent_0016AA00(0, hp2::input::JOY_EVENT_SCROLL_DOWN, true);
    group.HandleJoyEvent_0016AA00(0, hp2::input::JOY_EVENT_MENU_SELECT, true);

    require(activations.size() == 1, "MENU_SELECT must activate exactly once");
    require(activations[0].second == StringHash_002534A8("MENUITEM_2"),
            "the activated item must be the highlighted one");
    require(last_menu_sound_request().sound_id == kMenuSoundMenuSelect_5,
            "activation must request sound id 5");
    require(std::string(last_menu_sound_request().name) == "MenuSelect", "sound id 5 is MenuSelect");
}

// The group consumes JOY_EVENT_START when its message 0x0D callback answers 1.
void start_can_be_consumed_by_the_group() {
    FeScreen screen = make_menu_screen(3);
    int activations = 0;
    MenuGroup_0016AA00 group = make_group(screen, [&](int message, MenuEvent_0016A458& event) {
        if (message == kMenuMessage0x0D) event.result_0x08 = 1;
        if (message == kMenuMessage0x01) ++activations;
    });
    group.EnsureMenu_0016A858();
    group.HandleJoyEvent_0016AA00(0, hp2::input::JOY_EVENT_START, true);
    require(activations == 0, "a consumed START must not reach the item handler");
}

// Message 0x0B may override the initial index, as result_0x08 - 1 (FUN_0016A858).
void creation_message_can_override_the_initial_index() {
    FeScreen screen = make_menu_screen(3);
    MenuGroup_0016AA00 group = make_group(screen, [](int message, MenuEvent_0016A458& event) {
        if (message == kMenuMessage0x0B) event.result_0x08 = 3; // selects index 2
    });
    group.EnsureMenu_0016A858();
    require(group.child_0x08->CurrentIndex_0016A130() == 2,
            "result 3 must select list position 3 - 1 = 2, i.e. MENUITEM_3");
}

void pad_filter_is_honoured() {
    FeScreen screen = make_menu_screen(3);
    MenuGroup_0016AA00 group = make_group(screen);
    group.pad_index_0x10 = 1;
    group.EnsureMenu_0016A858();
    group.HandleJoyEvent_0016AA00(0, hp2::input::JOY_EVENT_SCROLL_DOWN, true);
    require(group.child_0x08->CurrentIndex_0016A130() == 0, "pad 0 must not drive a pad-1 group");
    group.HandleJoyEvent_0016AA00(1, hp2::input::JOY_EVENT_SCROLL_DOWN, true);
    require(group.child_0x08->CurrentIndex_0016A130() == 1, "pad 1 must drive it");
}

void recovered_names_are_verbatim() {
    require(std::string(kMenuItemNameFormat_002B2760) == "MENUITEM_%d", "format string verbatim");
    require(std::string(menu_sound_name_002FB0A8(1)) == "MenuLeftRight", "sound 1");
    require(std::string(menu_sound_name_002FB0A8(6)) == "MenuCancel", "sound 6");
    require(std::string(menu_sound_name_002FB0A8(8)) == "MenuWrong", "sound 8");
    require(menu_sound_name_002FB0A8(kMenuSoundNone) == nullptr, "id 0 has no record");
    require(std::string(hp2::input::joy_event_name(hp2::input::JOY_EVENT_MENU_SELECT)) ==
                "JOY_EVENT_MENU_SELECT",
            "joy event names verbatim");
    require(std::string(hp2::input::joy_event_name(hp2::input::JOY_EVENT_MENU_ANALOG_DOWN)) ==
                "JOY_EVENT_MENU_ANALOG_DOWN",
            "analog down name verbatim");
}

// AGENT.md R3: unimplemented recovered paths must throw rather than degrade.
// Screen name hashes recovered from the 0x10203 header: hashing the recovered name must land on
// the screen that stores that hash, and an unknown name must report -1 rather than a guess.
void screen_names_resolve_by_hash() {
    std::size_t count = 0;
    const RecoveredScreenName* names = recovered_screen_names(count);
    require(count == 18, "18 of the 41 screen names are recovered");

    std::vector<FeScreen> screens;
    for (int i = 0; i < 41; ++i) {
        FeScreen screen;
        screen.index = i;
        screens.push_back(screen);
    }
    for (std::size_t i = 0; i < count; ++i) {
        screens[static_cast<std::size_t>(names[i].screen_index)].name_hash_0x08 =
            StringHash_002534A8(names[i].name);
    }
    require(screen_index_for_screen_name(screens, "NEW_MAIN_MENU") == 0,
            "NEW_MAIN_MENU must resolve to screen 0");
    require(screen_index_for_screen_name(screens, "TITLE_SCREEN") == 22,
            "TITLE_SCREEN must resolve to screen 22");
    require(screen_index_for_screen_name(screens, "JOYSTICK_CONFIG") == 13,
            "JOYSTICK_CONFIG must resolve to screen 13");
    require(screen_index_for_screen_name(screens, "NOT_A_SCREEN") == -1,
            "an unknown name must report -1");
}

void unimplemented_paths_throw() {
    bool threw = false;
    try {
        Sub_0016FBB0();
    } catch (const hp2::NotImplemented&) {
        threw = true;
    }
    require(threw, "the highlight animation must report itself unimplemented");

    threw = false;
    try {
        Sub_0016BAC0(0, hp2::input::JOY_EVENT_MENU_SELECT, true);
    } catch (const hp2::NotImplemented&) {
        threw = true;
    }
    require(threw, "pop-up handling must report itself unimplemented");

    threw = false;
    try {
        localised_text(0);
    } catch (const hp2::NotImplemented&) {
        threw = true;
    }
    require(threw, "localised text must report itself unimplemented");
}

} // namespace

int main() {
    try {
        string_hash_matches_elf_immediates();
        menu_item_index_reverse_lookup();
        menu_list_only_takes_declared_items();
        state_bits_select_the_recovered_colours();
        colour_write_preserves_alpha_and_stamps_dirty();
        navigation_wraps_in_both_directions();
        navigation_skips_disabled_items();
        single_item_list_does_not_move();
        joy_events_drive_the_list_with_recovered_sounds();
        callback_can_veto_a_move();
        select_activates_the_current_item();
        start_can_be_consumed_by_the_group();
        creation_message_can_override_the_initial_index();
        pad_filter_is_honoured();
        recovered_names_are_verbatim();
        screen_names_resolve_by_hash();
        unimplemented_paths_throw();
    } catch (const std::exception& error) {
        std::fputs("fmenus_menu_tests failed: ", stderr);
        std::fputs(error.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }
    std::puts("FMENUS_MENU_TESTS_OK");
    return 0;
}
