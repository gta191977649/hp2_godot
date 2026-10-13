// TASK-0018: the front-end runtime's behaviour against the player's own original assets, with
// no Godot and no renderer. These checks were previously GDScript smokes on scenes/frontend.gd
// (--frontend-smoke, --menu-smoke, --activation-smoke, --submenu-smoke); the logic they exercise
// is now platform-independent, so they belong here, where they run under CTest.
//
// What stayed in the Godot host is what genuinely needs it: movie decoding into host textures,
// PCM into AudioStreamWAV, and key-event filtering (press/release/echo).
//
// PC behaviour only. None of this establishes original-hardware equivalence.
#include "game_core/input/joyevents.hpp"
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
    std::printf("  submenus: 8 Back routes, locale direct-pop, Audio/Display clipping/scroll\n");
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
// The View Event Tree screen loads with its screen and navigates without a host.
void check_event_tree(FrontEndRuntime& runtime) {
    const auto state = runtime.menu_open(39);
    require(state.ok || !state.not_implemented.empty(), "event tree screen failed to open");
    require(runtime.event_tree().loaded(), "event tree did not load with its screen: " +
                                               runtime.event_tree_error());
    const auto nodes = runtime.event_tree().nodes();
    require(nodes.size() == 32, "both shipped trees hold 32 events, got " + std::to_string(nodes.size()));
    const auto start = runtime.event_tree().cursor_info();
    require(start.number == 1, "the profile-less port must start on event 1");
    require(runtime.event_tree_joy_event(JOY_EVENT_SCROLL_UP), "the tree must consume its own events");
    require(runtime.event_tree().cursor_info().number != start.number, "cursor did not move");
    require(!runtime.event_tree_joy_event(JOY_EVENT_MENU_SELECT),
            "the tree must leave events it does not handle to the menu");
    // Leaving the screen drops the view.
    runtime.menu_open(0);
    require(!runtime.event_tree().loaded(), "event tree outlived its screen");
    std::printf("  event tree: %zu events, cursor navigation, screen-scoped lifetime\n", nodes.size());
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
