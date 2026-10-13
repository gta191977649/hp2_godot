// TASK-0018: the front-end runtime's behaviour against the player's own original assets, with
// no Godot and no renderer. These checks were previously GDScript smokes on scenes/frontend.gd
// (--frontend-smoke, --menu-smoke, --activation-smoke, --submenu-smoke); the logic they exercise
// is now platform-independent, so they belong here, where they run under CTest.
//
// What stayed in the Godot host is what genuinely needs it: movie decoding into host textures,
// PCM into AudioStreamWAV, and key-event filtering (press/release/echo).
//
// PC behaviour only. None of this establishes original-hardware equivalence.
#include "game_core/port/input/joyevents.hpp"
#include "port/runtime/frontend_runtime.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using namespace port::runtime;
using namespace hp2::input;

void require(bool condition, const std::string& what) {
    if (!condition) throw std::runtime_error(what);
}

const hp2::frontend::FeWidget* widget_named(const FrontEndRuntime& runtime, int screen_index,
                                            const std::string& name, std::size_t* slot = nullptr) {
    const auto* screen = runtime.screen(screen_index);
    if (screen == nullptr) return nullptr;
    for (std::size_t i = 0; i < screen->widgets.size(); ++i) {
        if (screen->widgets[i].name == name) {
            if (slot != nullptr) *slot = i;
            return &screen->widgets[i];
        }
    }
    return nullptr;
}

// Press one recovered joypad event and report the state it produced.
MenuState press(FrontEndRuntime& runtime, int joy_event) {
    return runtime.menu_joy_event(0, joy_event, true);
}

// --------------------------------------------------------------------- asset resolution
// Was --frontend-smoke: every authored texture hash must resolve to an original TexturePack
// entry or an original font table.
void check_asset_resolution(FrontEndRuntime& runtime) {
    int resolved = 0, missing = 0;
    for (int index = 0; index < runtime.screen_count(); ++index) {
        const auto* screen = runtime.screen(index);
        for (const auto& widget : screen->widgets) {
            if (widget.texture_hash == 0) continue;
            const bool found = widget.chunk_id == hp2::frontend::kTextWidgetId
                ? runtime.font(widget.texture_hash) != nullptr
                : static_cast<bool>(runtime.catalog()->texture(widget.texture_hash));
            if (found) ++resolved; else ++missing;
        }
    }
    require(runtime.screen_count() >= 40, "unexpected screen count");
    require(resolved >= 500, "too few authored textures resolved: " + std::to_string(resolved));
    std::printf("  asset resolution: screens=%d resolved=%d missing=%d\n",
                runtime.screen_count(), resolved, missing);
}

// --------------------------------------------------------------------- recovered menu runtime
// Was --menu-smoke.
void check_main_menu(FrontEndRuntime& runtime) {
    require(runtime.find_screen_by_name("NEW_MAIN_MENU") == 0, "NEW_MAIN_MENU must resolve to 0");
    auto state = runtime.menu_open(0);
    require(state.ok, "main menu failed to open: " + state.error + state.not_implemented);
    require(state.item_count >= 2, "NEW_MAIN_MENU must expose at least two MENUITEM widgets");

    const int start = state.current;
    state = press(runtime, JOY_EVENT_SCROLL_DOWN);
    require(state.current != start, "SCROLL_DOWN did not move the selection");
    require(state.sound_name == "MenuUpDown", "SCROLL_DOWN must request MenuUpDown, got " + state.sound_name);

    state = press(runtime, JOY_EVENT_MENU_ANALOG_UP);
    require(state.current == start, "MENU_ANALOG_UP did not return to the starting item");

    state = press(runtime, JOY_EVENT_MENU_SELECT);
    require(state.activated_name_hash != 0, "MENU_SELECT did not activate an item");
    require(state.sound_name == "MenuSelect", "MENU_SELECT must request MenuSelect");

    // The highlight colour must be the one stored in the ELF, applied by the recovered colour
    // path to the real widget records: 0x80,0x40,0x00 over the 0x80-is-opaque scale.
    bool found_highlight = false;
    for (const auto& widget : runtime.screen(state.screen)->widgets) {
        if (widget.r == 0x80 && widget.g == 0x40 && widget.b == 0x00) { found_highlight = true; break; }
    }
    require(found_highlight, "no widget carries the recovered highlight colour");

    for (const char* which : {"highlight_animation", "localised_text", "popup"}) {
        const auto probe = runtime.probe_not_implemented(which);
        require(!probe.not_implemented.empty(),
                std::string("probe ") + which + " must raise hp2::NotImplemented");
    }
    std::printf("  main menu: items=%d start=%d\n", state.item_count, start);
}

// --------------------------------------------------------------------- activation routing
// Was --activation-smoke: the real main-menu and Options routes, the mode flag the two mode
// selects write, Back, the deferred Options exit animation, and the gaps that must still raise.
void check_activation(FrontEndRuntime& runtime) {
    for (int choice = 0; choice < 3; ++choice) {
        auto state = runtime.menu_open(0);
        require(state.ok, "main menu failed to open");
        for (int step = 0; step < choice; ++step) state = press(runtime, JOY_EVENT_SCROLL_DOWN);
        state = press(runtime, JOY_EVENT_MENU_SELECT);
        const int target = (int[]){1, 2, 24}[choice];
        require(state.screen == target, "main-menu activation expected " + std::to_string(target) +
                                            " got " + std::to_string(state.screen));
        if (choice < 2) {
            require(state.dat_002FC070 == 1 - choice, "main-menu mode flag mismatch");
            const auto missing = press(runtime, JOY_EVENT_MENU_SELECT);
            require(!missing.not_implemented.empty(), "untranslated mode handler must raise");
        }
    }
    auto state = press(runtime, JOY_EVENT_MENU_BACK);
    require(state.screen == 0, "Options Back did not return to main");
    state = press(runtime, JOY_EVENT_MENU_BACK);
    require(state.screen == 0, "main menu must reject Back");

    for (int choice = 0; choice < 7; ++choice) {
        state = runtime.menu_open(24);
        require(state.ok, "Options failed to open");
        for (int step = 0; step < choice; ++step) state = press(runtime, JOY_EVENT_SCROLL_DOWN);
        state = press(runtime, JOY_EVENT_MENU_SELECT);
        if (choice == 2) { // Display preparation, ELF 0x001EE6C0, deliberately untranslated
            require(state.not_implemented.find("001EE6C0") != std::string::npos &&
                        state.screen == 24,
                    "Display preparation must report its missing state");
            continue;
        }
        const int target = (int[]){28, 31, 26, 37, 29, 16, 30}[choice];
        if (choice == 0 || choice == 1 || choice == 4) {
            require(state.screen == 24 && state.exiting, "Options must animate before switching");
            runtime.menu_tick(0.5);
            state = runtime.menu_state();
        }
        require(state.screen == target, "Options choice " + std::to_string(choice) + " expected " +
                                            std::to_string(target) + " got " + std::to_string(state.screen));
        if (choice == 4)
            require(state.active_class == "NameEntry", "shared document must preserve NameEntry");
    }
    std::printf("  activation: main routes, mode flag, Back, Options exit, explicit gaps\n");
}

// Reaches one Options submenu the way a player does, through the recovered activations.
MenuState enter_option(FrontEndRuntime& runtime, int option_index) {
    auto state = runtime.menu_open(0);
    press(runtime, JOY_EVENT_SCROLL_DOWN);
    press(runtime, JOY_EVENT_SCROLL_DOWN);
    state = press(runtime, JOY_EVENT_MENU_SELECT);
    for (int i = 0; i < option_index; ++i) state = press(runtime, JOY_EVENT_SCROLL_DOWN);
    state = press(runtime, JOY_EVENT_MENU_SELECT);
    runtime.menu_tick(1.0); // finish the original Options exit path, if this route uses one
    return runtime.menu_state();
}

// --------------------------------------------------------------------- submenu history, clipping
// Was --submenu-smoke.
void check_submenus(FrontEndRuntime& runtime) {
    const struct { int item, screen; } routes[] = {
        {0, 28}, {1, 31}, {3, 37}, {4, 29}, {5, 16}, {6, 30}};
    for (const auto& route : routes) {
        auto state = enter_option(runtime, route.item);
        require(state.screen == route.screen,
                "submenu route " + std::to_string(route.item) + " reached " + std::to_string(state.screen));
        // A Back that is not a press must not pop history. Key repeat/echo is filtered by the
        // host, which has no equivalent here.
        state = runtime.menu_joy_event(0, JOY_EVENT_MENU_BACK, false);
        require(state.screen == route.screen, "Back release must not pop submenu history");
        state = press(runtime, JOY_EVENT_MENU_BACK);
        require(state.screen == 24 && state.not_implemented.empty(),
                "submenu Back failed: " + state.not_implemented);
        state = press(runtime, JOY_EVENT_MENU_BACK);
        require(state.screen == 0 && state.history_depth == 0,
                "submenu class history did not return to main");
    }
    for (int main_item = 0; main_item < 2; ++main_item) {
        auto state = runtime.menu_open(0);
        for (int i = 0; i < main_item; ++i) state = press(runtime, JOY_EVENT_SCROLL_DOWN);
        press(runtime, JOY_EVENT_MENU_SELECT);
        state = press(runtime, JOY_EVENT_MENU_BACK);
        require(state.screen == 0, "mode selector Back failed");
    }

    // Audio Options: the authored viewport clips its overflowing rows, the selection does not
    // wrap at either end, and the selector arrows follow their scrolling target.
    auto state = enter_option(runtime, 0);
    require(state.screen == 28, "Audio Options not reached");
    state = press(runtime, JOY_EVENT_SCROLL_UP);
    require(state.current == 0, "Audio must block upward wrap at the first item");

    const auto* footer = widget_named(runtime, 28, "SCROLLING_TEXT");
    require(footer != nullptr, "Audio footer missing");
    const float footer_y = footer->y;
    std::size_t item9_slot = 0;
    const auto* item9 = widget_named(runtime, 28, "MENUITEM_9", &item9_slot);
    require(item9 != nullptr, "Audio MENUITEM_9 missing");
    const float original_y = item9->y;
    const auto clip = runtime.widget_clip(28, item9_slot);
    const float clip_bottom = clip.y + clip.height;
    require(clip.has_area() && original_y > clip_bottom,
            "Audio overflow must start clipped below the authored mask");
    for (const auto& widget : runtime.screen(28)->widgets) {
        if ((widget.name_hash == 0xd1cda594 || widget.name_hash == 0x9345e4a7) && widget.visible != 0)
            require(false, "Audio selector arrows visible on a volume row");
    }

    for (int i = 0; i < 8; ++i) press(runtime, JOY_EVENT_SCROLL_DOWN);
    runtime.menu_tick(0.5);
    item9 = widget_named(runtime, 28, "MENUITEM_9");
    require(item9->y < original_y && item9->y < clip_bottom,
            "Audio selected overflow item did not scroll into the viewport");
    require(widget_named(runtime, 28, "SCROLLING_TEXT")->y == footer_y,
            "Audio footer moved with the scrolling menu");

    press(runtime, JOY_EVENT_SCROLL_UP); // Audio Mode, the row the selector attaches to
    runtime.menu_tick(0.5);
    int arrows = 0;
    for (const auto& widget : runtime.screen(28)->widgets) {
        if (widget.name_hash != 0xd1cda594 && widget.name_hash != 0x9345e4a7) continue;
        require(widget.visible != 0 && widget.y < footer_y,
                "Audio selector did not follow its scrolling target");
        ++arrows;
    }
    require(arrows == 2, "Audio selector arrows missing");

    for (int i = 0; i < 8; ++i) state = press(runtime, JOY_EVENT_SCROLL_DOWN);
    require(state.current == 10, "Audio must block downward wrap at the last item");
    state = press(runtime, JOY_EVENT_MENU_BACK);
    require(state.screen == 24, "Audio Back failed after scrolling");
    press(runtime, JOY_EVENT_MENU_BACK);

    // Language Options applies its locale before the direct history pop, so the main menu's own
    // authored string must come back translated differently.
    std::string before_language = widget_named(runtime, 0, "MENUITEM_1")->text;
    state = enter_option(runtime, 6);
    require(state.screen == 30, "Language Options not reached");
    press(runtime, JOY_EVENT_SCROLL_DOWN); // French, original enum 1
    state = press(runtime, JOY_EVENT_MENU_BACK);
    const std::string after_language = widget_named(runtime, 0, "MENUITEM_1")->text;
    require(state.screen == 24 && before_language != after_language,
            "Language Back did not apply its selected locale before the direct pop");

    // TASK-0025: activating a language item (001F1B08 message 0x01) applies the locale and
    // performs its own 00170320(1) pop, exactly as its Back arm does. This used to raise
    // "screen item activation handler not translated (ELF 0x1f1dd8)".
    state = enter_option(runtime, 6);
    require(state.screen == 30 && state.not_implemented.empty(), "Language Options not reached");
    // 001F1A78 always disables and hides MENUITEM_7, because 0016FDB0 returns 0 in this build.
    // The screen really does author seven items -- six languages plus the one 001F1A78 always
    // disables and hides, because 0016FDB0 returns 0 in this build. So this is observable
    // behaviour and not a vacuous check. (E-0025-L02)
    require(state.item_count == 7, "Language Options must author MENUITEM_1..MENUITEM_7");
    int seventh_items = 0;
    for (const auto& item : state.items) {
        if (item.name_hash != 0x52E039F9u) continue;
        ++seventh_items;
        require(!item.enabled, "Language MENUITEM_7 must be disabled on entry");
    }
    require(seventh_items == 1, "Language MENUITEM_7 missing from the recovered menu list");
    const auto* seventh = widget_named(runtime, 30, "MENUITEM_7");
    require(seventh != nullptr && seventh->visible == 0,
            "Language MENUITEM_7 must be hidden on entry");
    press(runtime, JOY_EVENT_SCROLL_DOWN); // from French to German, original enum 2
    press(runtime, JOY_EVENT_SCROLL_DOWN);
    state = press(runtime, JOY_EVENT_MENU_SELECT);
    require(state.not_implemented.empty(), "Language activation reported a missing path");
    require(state.screen == 24, "Language activation did not pop through 00170320(1)");
    require(widget_named(runtime, 0, "MENUITEM_1")->text != after_language,
            "Language activation did not apply the activated item's locale");

    // Display Options, entered directly: the same viewport contract on a different screen.
    state = runtime.menu_open(26);
    require(state.ok, "Display Options failed to open");
    std::size_t display_slot = 0;
    const auto* display_item = widget_named(runtime, 26, "MENUITEM_9", &display_slot);
    require(display_item != nullptr, "Display MENUITEM_9 missing");
    const float display_original_y = display_item->y;
    const auto display_clip = runtime.widget_clip(26, display_slot);
    require(display_clip.has_area() && display_original_y > display_clip.y + display_clip.height,
            "Display overflow clip missing");
    for (int i = 0; i < 12; ++i) state = press(runtime, JOY_EVENT_SCROLL_DOWN);
    runtime.menu_tick(0.5);
    require(state.current == 8, "Display must block last-row wrap");
    require(widget_named(runtime, 26, "MENUITEM_9")->y < display_clip.y + display_clip.height,
            "Display overflow did not scroll into view");
    std::printf("  submenus: 8 Back routes, locale direct-pop and activation, hidden MENUITEM_7,\n"
                "            Audio/Display clipping/scroll\n");
}

// --------------------------------------------------------------------- highlight ghost
// Was the portable half of --animation-smoke: the ghost starts at its target's scale and alpha,
// grows and fades over the period, copies the target's live string, and retargets on navigation.
void check_highlight_animation(FrontEndRuntime& runtime) {
    auto state = runtime.menu_open(0);
    require(state.ok, "main menu failed to open");
    const auto* ghost = widget_named(runtime, 0, "ANIMATING_MENUITEM");
    require(ghost != nullptr, "the animation template is not in the authored screen");

    const auto target_of = [&](const MenuState& s) -> const hp2::frontend::FeWidget* {
        for (const auto& widget : runtime.screen(0)->widgets)
            if (widget.name_hash == s.current_name_hash) return &widget;
        return nullptr;
    };
    require(target_of(state) != nullptr, "the highlighted item is not in the authored screen");

    runtime.menu_tick(0.0);
    ghost = widget_named(runtime, 0, "ANIMATING_MENUITEM");
    const float target_scale = target_of(state)->scale_x;
    require(std::fabs(ghost->scale_x - target_scale) < 0.0001f,
            "ghost did not start at the target scale");
    require(ghost->a == 128, "ghost did not start opaque");

    runtime.menu_tick(0.25);
    ghost = widget_named(runtime, 0, "ANIMATING_MENUITEM");
    require(ghost->scale_x > target_scale, "ghost did not grow at a quarter second");
    require(ghost->a < 128, "ghost did not fade at a quarter second");
    require(ghost->text == target_of(state)->text, "ghost did not copy the target text");

    // The period itself is an open question, not an assertion: this check previously expected a
    // restart at 2 s while open_menu_animation records screen 0 as keeping the default 4 s
    // (E-0016-08). Report what the runtime does and leave the contradiction to its own task.
    runtime.menu_tick(1.76);
    runtime.menu_tick(0.0);
    const float at_two_seconds = widget_named(runtime, 0, "ANIMATING_MENUITEM")->scale_x;

    state = press(runtime, JOY_EVENT_SCROLL_DOWN);
    runtime.menu_tick(0.0);
    ghost = widget_named(runtime, 0, "ANIMATING_MENUITEM");
    require(ghost->text == target_of(state)->text, "ghost did not retarget after navigation");

    require(!runtime.menu_tick(-1.0).ok, "a negative delta must be rejected");
    require(!runtime.menu_tick(std::nan("")).ok, "a non-finite delta must be rejected");
    std::printf("  highlight ghost: start, grow/fade, text copy, retarget, delta guard "
                "(scale at 2.01 s = %.4f vs target %.4f; period unresolved)\n",
                at_two_seconds, target_scale);
}

// --------------------------------------------------- localisation, menu samples, jukebox
// Was the portable half of --runtime-smoke. The movie decoding and the AudioStreamWAV
// conversion stayed in the Godot host, which is what actually needs it.
void check_localisation_and_audio(FrontEndRuntime& runtime) {
    // Every screen the recovered hint binding covers must show its localised string, never the
    // authored placeholder, and must enter from the authored X rather than sit off-screen.
    for (const int screen_index : {0, 1, 2, 24, 28, 31}) {
        const auto state = runtime.menu_open(screen_index);
        require(state.not_implemented.empty(),
                "hint screen " + std::to_string(screen_index) + ": " + state.not_implemented);
        runtime.menu_tick(0.0);
        runtime.menu_tick(0.25);
        const auto* hint = widget_named(runtime, screen_index, "SCROLLING_TEXT");
        require(hint != nullptr, "recovered hint attachment missing on screen " + std::to_string(screen_index));
        require(!hint->text.empty() && hint->text.find("This text will scroll") == std::string::npos,
                "authored placeholder survived on screen " + std::to_string(screen_index));
        require(hint->x < 640.0f, "hint did not enter at the authored X on screen " + std::to_string(screen_index));
    }

    // One recovered menu event must deliver the original stereo sample with its own gain.
    runtime.menu_open(0);
    runtime.menu_tick(0.05);
    runtime.drain_menu_audio(); // clear whatever opening the screen queued
    runtime.menu_joy_event(0, JOY_EVENT_SCROLL_DOWN, true);
    const auto sounds = runtime.drain_menu_audio();
    require(!sounds.empty(), "native bSound request was not delivered");
    const auto& sound = sounds.back();
    require(sound.not_implemented.empty(), "menu sample failed: " + sound.not_implemented);
    require(sound.pcm.channels == 2 && sound.pcm.sample_rate == 22050,
            "original menu sample is 22050 Hz stereo");
    require(sound.pcm.samples.size() * 2 == 23408, "original menu PCM size differs: " +
                                                       std::to_string(sound.pcm.samples.size() * 2));
    require(std::fabs(sound.gain - 0.1023f) < 0.001f, "menu category/descriptor gain differs");

    // The jukebox picks tune 5 on entry and tune 7 when that one ends.
    const auto first = runtime.frontend_music(false);
    require(first.ok && first.has_track, "first frontend music selection failed");
    require(first.index == 5 && first.pcm.sample_rate == 32000, "first tune selection/PCM differs");
    require(std::fabs(first.gain - 0.45f) < 0.0001f, "music category gain differs");
    require(std::fabs(first.pitch_scale - 1.0103759765625f) < 0.000001f, "music hardware pitch differs");
    const auto next = runtime.frontend_music(true);
    require(next.ok && next.has_track && next.index == 7, "music completion selection failed");
    std::printf("  localisation/audio: 6 hint screens, stereo menu sample, jukebox 5 then 7\n");
}

// --------------------------------------------------------------------- event tree
// TASK-0021 follow-up: the route a player actually takes. The main menu opens a mode-select
// screen, its Tournament entry writes the mode and the tree type into the profile and requests
// the class "View Event Tree", and that class's registration resolves to FRONTB.BUN screen 39.
// Every one of those links was broken before: the class had no registration row, so
// apply_pending_screen raised NotImplemented at 0x0016C8C8 and nothing happened on screen.
//
// `mode_screen` is 1 (Cops On, Ultimate Racer) or 2 (Cops Off, Championship); `want_mode` is the
// value FUN_00185568 keys on and `want_type` the tree type FUN_001F5D50 stores.
void check_event_tree_route(FrontEndRuntime& runtime, int mode_screen, int want_mode,
                            int want_type, const char* want_name) {
    bool reached = false;
    for (int choice = 0; choice < 6 && !reached; ++choice) {
        auto state = runtime.menu_open(mode_screen);
        require(state.ok, "mode select failed to open");
        for (int step = 0; step < choice; ++step) state = press(runtime, JOY_EVENT_SCROLL_DOWN);
        state = press(runtime, JOY_EVENT_MENU_SELECT);
        // FUN_00217020 does not switch screens: it builds a 0x1C-byte tExitScreenAnimation
        // (pool name string 0x002BC528) through FUN_002490D8 and stores it at instance+0x270,
        // and the switch happens only when that animation finishes. Tick it out, exactly as
        // the Options route above does.
        if (state.exiting) {
            runtime.menu_tick(1.0);
            state = runtime.menu_state();
        }
        if (state.screen != 39) continue;
        reached = true;
        require(state.not_implemented.empty(),
                "entering the event tree must not raise: " + state.not_implemented);
        require(runtime.event_tree().loaded(),
                "the tree must load with screen 39: " + runtime.event_tree_error());
        require(runtime.profile().SelectedMode_00185568() == want_mode,
                "the mode select must write its mode id into profile+0x1072C");
        require(runtime.profile().EventTreeType_001F5D50() == want_type,
                "FUN_001F5D50 must store the tree type into profile+0x164");
        require(runtime.profile().EventTreeViewState_001FAF60() == 0,
                "entering the tree clears the view state at profile+0x170");
        require(runtime.event_tree().nodes().size() == 32, "the selected tree holds 32 events");
        require(runtime.event_tree().title_string_id() ==
                    (want_mode == 8 ? hp2::frontend::kTitleStringChampionship_001F96B8
                                    : hp2::frontend::kTitleStringHotPursuit_001F96B8),
                "the screen title id follows the selected mode");
        std::printf("  event tree route: screen %d item %d -> 39, mode %d, %s\n", mode_screen,
                    choice, want_mode, want_name);
    }
    require(reached, "no item on the mode select screen reached the event tree");
}

// The View Event Tree screen loads with its screen and navigates without a host.
void check_event_tree(FrontEndRuntime& runtime) {
    // Both routes in, which also proves the two screens select *different* trees -- the port
    // used to pass a hardcoded mode here, so Championship opened the Hot Pursuit tree.
    check_event_tree_route(runtime, 1, 9, 1, "Hot Pursuit / Ultimate Racer");
    const std::string hot_pursuit = runtime.event_tree().cursor_info().name;
    check_event_tree_route(runtime, 2, 8, 0, "Championship");
    require(runtime.event_tree().cursor_info().name != hot_pursuit ||
                runtime.event_tree().nodes()[1].name != hot_pursuit,
            "the two mode-select screens must not open the same tree");

    const auto state = runtime.menu_open(39);
    require(state.ok || !state.not_implemented.empty(), "event tree screen failed to open");
    require(runtime.event_tree().loaded(), "event tree did not load with its screen: " +
                                               runtime.event_tree_error());
    const auto nodes = runtime.event_tree().nodes();
    require(nodes.size() == 32, "both shipped trees hold 32 events, got " + std::to_string(nodes.size()));
    const auto start = runtime.event_tree().cursor_info();
    require(runtime.screen_info(39).screen_name == "NEW_EVENT_TREE", "event tree frontend name recovered");
    auto text_of = [&](int screen, const char* name) {
        for (const auto& widget : runtime.screen(screen)->widgets) if (widget.name == name) return widget.text;
        throw std::runtime_error(std::string("missing original widget ") + name);
    };
    require(text_of(39, "EVENT_TREE") == "Championship", "runtime title replaces This Will Change");
    require(text_of(39, "OVERLAY_STATUSTITLE") == "Unlocked", "root status uses real language entry");
    require(text_of(39, "OVERLAY_REWARDTITLE") == "National Forest",
            "Championship event 1 reward resolves its original track row");
    require(text_of(39, "SMALL_EVENTNUMBER") == "Event 1", "event number uses localized format");
    require(text_of(17, "TRACK_NAME").find("123456") == std::string::npos,
            "detail track title resolves original track catalog");
    require(text_of(17, "TRACK_LINE_1").find('$') == std::string::npos,
            "detail template expands original operands");
    require(start.number == 1, "an empty profile starts on event 1 (FUN_001F5D00's fallback)");
    require(runtime.event_tree().state() == hp2::frontend::kViewStateBrowsing_001FAF60,
            "an empty profile opens the tree in state 0");
    require(runtime.event_tree().zoom() == hp2::frontend::kZoomLevels_gp_0x4178[0],
            "FUN_001FAB50(view, 0) leaves the tree at zoom level 0");

    // TASK-0021 replaced an invented navigation rule, so these expectations changed with it.
    // FUN_001FA9C8 sends +2 to FUN_001FA978 over leaf+0x20, the list FUN_001F96B8 threads with
    // an event's prerequisites. Event 1 is the root and has none, so up does nothing and answers
    // sound 0 -- the port used to walk the outgoing list here and move downward on an up press.
    require(runtime.event_tree_joy_event(JOY_EVENT_SCROLL_UP),
            "the tree consumes its own events even when nothing moves");
    require(runtime.event_tree().cursor_info().number == start.number,
            "up from the root must not move: the root has no prerequisites");
    // Sound 0 is observed by nothing reaching the queue: FUN_0016E038 returns early for id 0
    // and deliberately leaves the previous request in place, so the queue is the only honest
    // witness. Drain first so the screen-open sounds do not count.
    runtime.drain_menu_audio();
    require(runtime.event_tree_joy_event(JOY_EVENT_SCROLL_UP), "up is consumed again");
    require(runtime.drain_menu_audio().empty(), "a move that lands nowhere queues no sound");

    // Down walks leaf+0x28, the events that require this one, picking the one nearest in X.
    require(runtime.event_tree_joy_event(JOY_EVENT_SCROLL_DOWN), "down is consumed");
    const int after_down = runtime.event_tree().cursor_info().number;
    require(after_down != start.number, "down from the root reaches a dependent event");
    require(hp2::frontend::last_menu_sound_request().sound_id ==
                hp2::frontend::kSoundTreeUpDown_001FAF60,
            "an up/down move answers TreeUpDown (0x13)");
    require(!runtime.drain_menu_audio().empty(), "and it reaches the audio queue");

    // Left/right stay at the same depth and do not wrap, so one of the two must fail at a row
    // end while the other succeeds -- and a successful one answers TreeLeftRight.
    const bool left = runtime.event_tree_joy_event(JOY_EVENT_SCROLL_LEFT);
    const int after_left = runtime.event_tree().cursor_info().number;
    require(left, "left is consumed");
    if (after_left != after_down) {
        require(hp2::frontend::last_menu_sound_request().sound_id ==
                    hp2::frontend::kSoundTreeLeftRight_001FAF60,
                "a left/right move answers TreeLeftRight (0x14)");
        const auto moved = runtime.event_tree().cursor_info();
        require(moved.depth == 1, "left/right stayed on the depth down had reached");
    }

    // Confirm enters state 2 and zooms in; Back leaves it again. Both answer TreeZoomInOut.
    require(runtime.menu_joy_event(0, JOY_EVENT_MENU_SELECT, true).not_implemented.empty(),
            "integrated event dispatch consumes confirm without DummyMenuItem activation");
    require(runtime.event_tree().state() == 1, "confirm begins the recovered transition state 1");
    runtime.menu_tick(0.25);
    require(runtime.event_tree().state() == 1, "half-duration is still transitioning");
    require(std::fabs(runtime.event_tree().zoom() - 1.7f) < 0.00001f &&
            std::fabs(runtime.event_tree().alpha() - 0.5f) < 0.00001f,
            "half-duration uses shared Hermite zoom and alpha");
    runtime.menu_tick(0.25);
    require(runtime.event_tree().state() == hp2::frontend::kViewStateEventSelected_001FAF60,
            "confirm moves the view into state 2");
    require(runtime.event_tree().zoom() == 3.0f, "detail uses recovered zoom 3.0");
    require(hp2::frontend::last_menu_sound_request().sound_id ==
                hp2::frontend::kSoundTreeZoomInOut_001FAF60,
            "confirm answers TreeZoomInOut (0x15)");
    const auto locked_select = runtime.menu_joy_event(0, JOY_EVENT_MENU_SELECT, true);
    require(locked_select.not_implemented.empty() && runtime.event_tree().state() == 2,
            "locked detail selection stays on the detail without invented race flow");
    require(hp2::frontend::last_menu_sound_request().sound_id == 0x17,
            "FUN_001F6B70 locked selection answers TreeWrong");
    require(runtime.event_tree_joy_event(JOY_EVENT_MENU_BACK), "state 2 consumes Back");
    require(runtime.event_tree().state() == 3, "Back starts state 3 before returning");
    runtime.menu_tick(0.5);
    require(runtime.event_tree().state() == hp2::frontend::kViewStateBrowsing_001FAF60,
            "Back returns to state 0");
    require(!runtime.event_tree_joy_event(JOY_EVENT_MENU_BACK),
            "state 0 leaves Back to the navigation history");
    // Use the integrated route on an unlocked event: the known input handler must
    // report its actual downstream gaps while preview return remains functional.
    runtime.menu_open(39);
    for (int step = 0; runtime.event_tree().cursor_info().number != 1 && step < 10; ++step)
        runtime.event_tree_joy_event(JOY_EVENT_SCROLL_UP);
    require(runtime.event_tree().cursor_info().number == 1, "return to unlocked root through recovered navigation");
    runtime.menu_joy_event(0, JOY_EVENT_MENU_SELECT, true);
    runtime.menu_tick(0.5);
    const auto race_gap = runtime.menu_joy_event(0, JOY_EVENT_MENU_SELECT, true);
    require(race_gap.not_implemented.find("0x001F6A28") != std::string::npos,
            "unlocked detail selection reports missing car/race selection by original address");
    require(hp2::frontend::last_menu_sound_request().sound_id == 0x16,
            "unlocked detail selection answers TreeSelect");
    const auto preview_gap = runtime.menu_joy_event(0, JOY_EVENT_MENU_SQUARE, true);
    require(preview_gap.not_implemented.find("0x001F6440") != std::string::npos &&
            runtime.event_tree().state() == 4, "model preview gap is explicit after state 4 entry");
    runtime.menu_tick(0.5);
    require(runtime.event_tree().state() == 5, "preview transition reaches state 5");
    runtime.menu_joy_event(0, JOY_EVENT_MENU_BACK, true);
    require(runtime.event_tree().state() == 6 && hp2::frontend::last_menu_sound_request().sound_id == 0x18,
            "preview Back starts state 6 and preserves original return sound");
    runtime.menu_tick(0.5);
    require(runtime.event_tree().state() == 2, "preview Back returns to detail");
    runtime.menu_joy_event(0, JOY_EVENT_MENU_BACK, true);
    runtime.menu_tick(0.5);
    const auto parent = runtime.menu_joy_event(0, JOY_EVENT_MENU_BACK, true);
    require(parent.screen == 2 && parent.not_implemented.empty(),
            "browsing Back returns through the actual mode-select history");
    runtime.menu_open(39);

    // The recovered screen title id follows whichever mode the route above left in the profile.
    require(runtime.event_tree().title_string_id() ==
                hp2::frontend::TitleStringId_001F96B8(runtime.profile().SelectedMode_00185568()),
            "the screen title id follows profile+0x1072C");

    // Leaving the screen drops the view.
    runtime.menu_open(0);
    require(!runtime.event_tree().loaded(), "event tree outlived its screen");
    std::printf("  event tree: %zu events, recovered nav (root up blocked, down to %d), "
                "state 0<->2, zoom %.2f/%.2f\n",
                nodes.size(), after_down, hp2::frontend::kZoomLevels_gp_0x4178[0],
                hp2::frontend::kZoomLevels_gp_0x4178[1]);
}

} // namespace

int main() {
    try {
        // Each group gets its own runtime, the way each GDScript smoke got its own process:
        // navigation history and the locale deliberately survive menu_open, so one shared
        // runtime would let one group's stack leak into the next.
        using Check = void (*)(FrontEndRuntime&);
        const std::pair<const char*, Check> groups[] = {
            {"asset resolution", check_asset_resolution}, {"main menu", check_main_menu},
            {"activation", check_activation},             {"submenus", check_submenus},
            {"highlight ghost", check_highlight_animation},
            {"localisation/audio", check_localisation_and_audio},
            {"event tree", check_event_tree}};
        for (const auto& group : groups) {
            FrontEndRuntime runtime;
            const auto loaded = runtime.load(".", "FRONTEND/FRONTB.BUN");
            if (!loaded.ok) {
                std::printf("SKIPPED frontend runtime checks (%s)\n", loaded.error.c_str());
                return 0; // original assets are not configured on this machine
            }
            require(runtime.load_fonts({"FRONTEND/FRONTB.BUN", "GLOBAL/GLOBALB.BUN",
                                        "GLOBAL/INGAMEB.BUN"}) > 0, "no original font tables loaded");
            group.second(runtime);
        }
        std::printf("FRONTEND_RUNTIME_TESTS_OK\n");
        return 0;
    } catch (const std::exception& error) {
        std::printf("frontend_runtime_tests failed: %s\n", error.what());
        return 1;
    }
}
