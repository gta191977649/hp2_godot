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
#include "platform/ps2_profile_store.hpp"
#include <fstream>
#include <iterator>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <chrono>
#include <thread>
#include <filesystem>
#include <cstdlib>

namespace {

using namespace port::runtime;
using namespace hp2::input;
struct PrivateSaveFixture {
    bool existed=false;std::string prior;
    explicit PrivateSaveFixture(const std::filesystem::path& path){if(const auto* old=std::getenv("HP2_PORT_SAVE_ROOT")){existed=true;prior=old;}
#ifdef _WIN32
        _putenv_s("HP2_PORT_SAVE_ROOT",path.string().c_str());
#else
        setenv("HP2_PORT_SAVE_ROOT",path.string().c_str(),1);
#endif
    }
    ~PrivateSaveFixture(){
#ifdef _WIN32
        _putenv_s("HP2_PORT_SAVE_ROOT",existed?prior.c_str():"");
#else
        if(existed)setenv("HP2_PORT_SAVE_ROOT",prior.c_str(),1);else unsetenv("HP2_PORT_SAVE_ROOT");
#endif
    }
};

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
    require(runtime.font(hp2::frontend::bStringHash("BUTTONSOUT")) != nullptr,
            "original zero-pair button font must remain available");
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
    port::render::Frame staged;
    runtime.build_frame(*runtime.catalog(),staged);
    require(!staged.quads.empty() && port::render::frontend_ranges_complete(staged),
            "original main-menu packet lost complete frontend stage coverage");

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
        require(runtime.background_movie()=="MOVIES/mainmenu_movie.AVI","Main menu movie request missing");
        for (int step = 0; step < choice; ++step) state = press(runtime, JOY_EVENT_SCROLL_DOWN);
        state = press(runtime, JOY_EVENT_MENU_SELECT);
        const int target = (int[]){1, 2, 24}[choice];
        require(state.screen == target, "main-menu activation expected " + std::to_string(target) +
                                            " got " + std::to_string(state.screen));
        if (choice < 2) {
            require(state.dat_002FC070 == 1 - choice, "main-menu mode flag mismatch");
            require(runtime.background_movie()==(choice==0?"MOVIES/hotpursuit_movie.AVI":"MOVIES/worldracing_ movie.AVI"),
                    "Mode selector movie request differs");
            const auto missing = press(runtime, JOY_EVENT_MENU_SELECT);
            require(!missing.not_implemented.empty(), "untranslated mode handler must raise");
            const auto expected=runtime.background_movie();
            const auto document=choice==0?0xe55f1297u:0x2d62cfe2u;
            int challenge=-1;
            for(int index=0;index<runtime.screen_count();++index)
                if(runtime.screen(index)->name_hash_0x08==document){challenge=index;break;}
            require(challenge>=0 && runtime.menu_open(challenge).ok, "Challenge document unavailable");
            require(runtime.background_movie()==expected,"Challenge movie must follow the initialized race mode");
        } else require(runtime.background_movie()=="MOVIES/options_movie_new.AVI","Options constructor must use the shared movie");
    }
    auto state = press(runtime, JOY_EVENT_MENU_BACK);
    require(state.screen == 0, "Options Back did not return to main");
    require(runtime.background_movie()=="MOVIES/mainmenu_movie.AVI","Options Back did not restore main menu movie");
    state = press(runtime, JOY_EVENT_MENU_BACK);
    require(state.screen == 0, "main menu must reject Back");

    for (int choice = 0; choice < 7; ++choice) {
        state = runtime.menu_open(24);
        require(state.ok, "Options failed to open");
        for (int step = 0; step < choice; ++step) state = press(runtime, JOY_EVENT_SCROLL_DOWN);
        state = press(runtime, JOY_EVENT_MENU_SELECT);
        const int target = (int[]){28, 31, 26, 37, 29, 16, 30}[choice];
        if (choice == 0 || choice == 1 || choice == 2 || choice == 4) {
            require(state.screen == 24 && state.exiting, "Options must animate before switching");
            runtime.menu_tick(0.5);
            state = runtime.menu_state();
        }
        require(state.screen == target, "Options choice " + std::to_string(choice) + " expected " +
                                            std::to_string(target) + " got " + std::to_string(state.screen));
        if (choice == 4)
            require(state.active_class == "NameEntry", "shared document must preserve NameEntry");
        const std::string expected_movie=state.active_class=="Phils Load Save"?"":"MOVIES/options_movie_new.AVI";
        require(runtime.background_movie()==expected_movie,
                "Options child movie lifecycle differs for "+state.active_class);
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
        {0, 28}, {1, 31}, {2, 26}, {3, 37}, {4, 29}, {5, 16}, {6, 30}};
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
    const auto first = runtime.frontend_music(false,true);
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
        require(runtime.background_movie().empty(),"Event Tree did not stop the preceding mode movie");
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
    // E-0039-F01/N01: original asset decode and event digit geometry, not a pixel oracle.
    const auto number_hash = hp2::frontend::bStringHash("FONT_EVENTNUMBERS");
    const auto* number_font = runtime.font(number_hash);
    require(number_font && number_font->line_height == 98, "event font height is98, not offset28");
    const auto* number_template = widget_named(runtime,39,"EVENT_NUMBER");
    const auto* panel_template = widget_named(runtime,39,"EVENT_PANEL");
    require(number_template && panel_template, "original event templates available");
    WidgetSizes number_sizes(*runtime.catalog(),runtime.fonts());
    port::render::Frame number_frame;
    append_event_tree(runtime.event_tree(),number_sizes,*runtime.catalog(),number_frame,
                      panel_template,number_template,runtime.screen(39));
    const auto camera = runtime.event_tree().camera_offset();
    const float expected_y = camera.y + start.y*runtime.event_tree().zoom() -
        98.0f*number_template->scale_y*runtime.event_tree().zoom()*0.5f;
    bool centred_number = false;
    for (const auto& quad : number_frame.quads) if (quad.texture == number_hash &&
        std::abs(quad.destination.y-expected_y)<0.001f) centred_number = true;
    require(centred_number,"001F7BF0 vertically centres the event digit using scaled maximum glyph height");
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
    // This fixture retains Championship mode8 from the preceding route. Both
    // reviewed first-event adapters have dedicated readiness/transport groups;
    // remain on the detail here to test the independent model-preview boundary.
    runtime.menu_open(39);
    for (int step = 0; runtime.event_tree().cursor_info().number != 1 && step < 10; ++step)
        runtime.event_tree_joy_event(JOY_EVENT_SCROLL_UP);
    require(runtime.event_tree().cursor_info().number == 1, "return to unlocked root through recovered navigation");
    runtime.menu_joy_event(0, JOY_EVENT_MENU_SELECT, true);
    runtime.menu_tick(0.5);
    const auto details=runtime.ui_snapshot();
    const auto paragraph=std::find_if(details.widgets.begin(),details.widgets.end(),[](const auto& w){return w.widget.name=="TRACK_LINE_1" && w.wrap;});
    require(paragraph!=details.widgets.end() && paragraph->bounds.y==126.f && paragraph->clip.y==64.f,
            "paragraph text origin must stay below the track labels inside the larger clip box");
    require(!runtime.take_event_launch(),"Opening Championship details fabricated a ready World request");
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

// TASK-0104 PC semantic contracts against real original assets, not PS2 equivalence.
void check_ui_contract(FrontEndRuntime& runtime){
    auto opened=runtime.menu_open(0);require(opened.ok,"UI contract main menu");
    const auto original=runtime.ui_snapshot();require(original.generation>0,"UI generation missing");
    // FRONTB document2920254113: bit2 left-aligned text starts at its authored X.
    // The previous snapshot subtracted half its width and clipped the description.
    const auto main_label=std::find_if(original.widgets.begin(),original.widgets.end(),[](const auto& w){return w.widget.name=="MENUITEM_1";});
    const auto description=std::find_if(original.widgets.begin(),original.widgets.end(),[](const auto& w){return w.widget.name=="SCROLLING_TEXT";});
    require(main_label!=original.widgets.end() && main_label->bounds.x==152.f,"left-aligned menu origin differs from original document");
    require(description!=original.widgets.end() && description->bounds.x==description->widget.x,"scroll text origin differs from original document");
    const auto target=std::find_if(original.widgets.begin(),original.widgets.end(),[](const auto& w){return w.interactive && w.menu_index==3;});
    require(target!=original.widgets.end() && target->bounds.has_area(),"Options has no semantic hit target");
    FrontendUiAction action;action.id=target->id;action.generation=original.generation;action.kind=FrontendUiAction::Kind::Focus;
    auto focused=runtime.ui_action(action);require(focused.ok && focused.current_name=="MENUITEM_3","PC focus did not use original menu selection");
    require(runtime.ui_snapshot().generation==original.generation,"Focus reconstructed the page");
    action.kind=FrontendUiAction::Kind::Activate;require(runtime.ui_action(action).not_implemented.empty(),"PC activation hit untranslated path");
    for(int i=0;i<90;++i)runtime.menu_tick(1.0/60);
    require(runtime.menu_state().active_class=="Options","PC activation did not reach Options");
    const auto stale=runtime.ui_action(action);require(!stale.ok && stale.error.find("stale")!=std::string::npos,"old page click was accepted");
    auto options=runtime.ui_snapshot();auto audio=std::find_if(options.widgets.begin(),options.widgets.end(),[](const auto& w){return w.interactive && w.menu_index==1;});
    require(audio!=options.widgets.end(),"Audio action missing");action.id=audio->id;action.generation=options.generation;
    require(runtime.ui_action(action).not_implemented.empty(),"Audio entry failed");for(int i=0;i<90;++i)runtime.menu_tick(1.0/60);
    auto snapshot=runtime.ui_snapshot();require(snapshot.page=="Audio Options","Audio UI page missing");
    require(std::count_if(snapshot.widgets.begin(),snapshot.widgets.end(),[](const auto& w){return w.kind=="slider";})==7,"Audio scalar controls missing");
    action.id="audio-slider:0";action.generation=snapshot.generation;action.kind=FrontendUiAction::Kind::SetValue;action.value=.25f;
    require(runtime.ui_action(action).ok,"PC scalar request rejected");
    snapshot=runtime.ui_snapshot();const auto scalar=std::find_if(snapshot.widgets.begin(),snapshot.widgets.end(),[](const auto& w){return w.id=="audio-slider:0";});
    require(scalar!=snapshot.widgets.end() && std::fabs(scalar->value-.25f)<1e-5f,"native audio scalar did not consume PC value");
    action.value=2;require(!runtime.ui_action(action).ok,"invalid PC scalar accepted");
    action.kind=FrontendUiAction::Kind::Focus;action.port=2;require(!runtime.ui_action(action).ok,"unknown PC port accepted");
    std::printf("  typed UI: real menu focus/activation, stale clicks, scalar conversion/range/port checks\n");
}

void check_event_transport(FrontEndRuntime& runtime,int tree_type=1){
    if(!std::getenv("HP2_PS2_DEBUG_ELF") && !std::filesystem::exists("config/ps2_debug_elf.local.json")){
        std::puts("  authored event transport skipped: authorized Debug ELF not configured");return;
    }
    require(!runtime.take_event_launch(),"Fresh frontend unexpectedly contains a World launch");
    runtime.menu_open(0);if(tree_type==0)press(runtime,JOY_EVENT_SCROLL_DOWN);press(runtime,JOY_EVENT_MENU_SELECT);
    for(int i=0;i<90;++i)runtime.menu_tick(1.0/60);
    require(runtime.menu_state().active_class==(tree_type==1?"Cops On Mode Select":"Cops Off Mode Select"),"Actual event transport route did not enter mode selection");
    for(int i=0;i<(tree_type==1?3:2);++i)press(runtime,JOY_EVENT_SCROLL_DOWN);
    press(runtime,JOY_EVENT_MENU_SELECT);for(int i=0;i<90;++i)runtime.menu_tick(1.0/60);
    require(runtime.menu_state().active_class=="View Event Tree","Actual event transport route did not enter tree");
    const auto selected=runtime.event_tree().cursor_info();
    const auto selected_tree=*runtime.event_tree().selected_tree();
    const auto progress_layout=hp2::frontend::build_profile_layout_001FE988(selected_tree);
    const auto* source_progress=runtime.profile().EventBlock_001FE988(tree_type);
    const std::vector<std::uint8_t> expected_progress(source_progress,source_progress+256);
    EventProfilePayload expected_both;for(int tree=0;tree<2;++tree)std::copy_n(runtime.profile().EventBlock_001FE988(tree),256,expected_both[tree].begin());
    press(runtime,JOY_EVENT_MENU_SELECT);runtime.menu_tick(1);
    auto entered=press(runtime,JOY_EVENT_MENU_SELECT);require(entered.not_implemented.empty(),"Authored tree entry failed: "+entered.not_implemented);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
    bool loaded=false;
    while(std::chrono::steady_clock::now()<deadline){
        const auto tick=runtime.menu_tick(1.0/60);require(tick.not_implemented.empty(),"Selection tick failed: "+tick.not_implemented);
        const auto selection=runtime.menu_state().selection;
        if(selection.active&&selection.loaded&&selection.overlay==0){loaded=true;break;}
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    require(loaded,"Original selection assets did not become ready for launch");
    require(!runtime.take_event_launch(),"Selection loading fabricated an unconfirmed launch");
    const auto selected_ride=runtime.menu_state().selection;
    press(runtime,JOY_EVENT_MENU_SELECT); // Original transmission popup.
    press(runtime,JOY_EVENT_SCROLL_RIGHT);press(runtime,JOY_EVENT_MENU_SELECT); // Manual, then handling popup.
    const auto chosen=press(runtime,JOY_EVENT_MENU_SELECT);require(chosen.not_implemented.empty(),"Selection continuation did not stage launch: "+chosen.not_implemented);
    auto request=runtime.take_event_launch();require(bool(request),"Ready original selectors did not stage authored event");
    require(request->event_number==selected.number && request->event_index==selected.index && request->tree_type==tree_type,"Launch lost actual selected event identity");
    require(request->prepared_event.has_value(),"Production menu lost its sole original SetRace checkpoint");
    const auto& prepared=*request->prepared_event;
    require(prepared.tree_type==tree_type&&prepared.event_number==selected.number,"Prepared menu identity differs from selected authored event");
    require(prepared.parameters[unsigned(port::runtime::PreparedParameter::NFSRaceMode)]==1,"Menu NFSRaceMode was replaced by language");
    require(prepared.parameters[unsigned(port::runtime::PreparedParameter::RaceType)]==1&&!prepared.objective.empty(),"Original menu SetRace scalar/objective preparation is missing");
    require(prepared.old_state.size()==unsigned(tree_type==0?1:4),"Prepared menu old-state extent differs from source type");
    require(request->event_progress.size()==256&&request->event_progress_offsets.size()==32,"Launch omitted versioned profile layout");
    require(request->event_profile&&request->event_profile->blocks==expected_both,"Launch dropped opposite-tree source profile state");
    const auto& full_layout=request->event_profile->layout;
    require(full_layout.version==EventProfileVersion::Retail&&full_layout.trees[0].size()==32&&full_layout.trees[1].size()==32,"Launch lacks complete explicit Retail layout");
    const auto shared0=std::find_if(full_layout.trees[0].begin(),full_layout.trees[0].end(),[](const auto& r){return r.event.number==32;});
    const auto shared1=std::find_if(full_layout.trees[1].begin(),full_layout.trees[1].end(),[](const auto& r){return r.event.number==32;});
    require(shared0->location==shared1->location,"Launch split original globally shared event32 slice");
    require(request->event_progress==expected_progress&&std::equal(request->event_progress_offsets.begin(),request->event_progress_offsets.end(),progress_layout.offsets.begin()),"Launch changed original progress or independent Retail record mapping");
    require(request->car_id>=0&&request->skin_id>=0&&request->track>0&&request->transmission==1,"Launch lost original selected ride/settings/track");
    require(request->car_id==selected_ride.car&&request->skin_id==selected_ride.skin,"Launch substituted the displayed source-selected ride");
    require(request->random_state==runtime.random_state(),"Event handoff reset the live original frontend RNG state");
    require(!runtime.take_event_launch(),"One original confirmation produced duplicate World requests");
    auto stale=port::race::EventSettlement{};bool rejected=false;
    try{runtime.complete_event(stale);}catch(const std::logic_error&){rejected=true;}
    require(rejected,"Stale outcome was accepted by active authored event");
    require(runtime.random_state()==request->random_state,"Rejected settlement changed original RNG state");
    // E-0101-EP01 explicit transport fixtures, not generated gameplay outcomes.
    port::race::EventSettlement outcome;outcome.generation=request->generation;outcome.tree_type=request->tree_type;outcome.event_index=request->event_index;outcome.event_number=request->event_number;outcome.player_points=request->player_points+7;outcome.random_state=request->random_state+1;outcome.event_progress=request->event_progress;
    rejected=false;try{runtime.complete_event(outcome);}catch(const std::logic_error&){rejected=true;}require(rejected,"Full request silently downgraded to selected-block-only settlement");
    outcome.event_profile=expected_both;
    require(request->event_scores.has_value(),"Production request omitted source score/name baseline");
    outcome.event_scores=request->event_scores->profile; // Explicit transport fixture; no source score outcome is fabricated.
    require(request->event_settings.has_value(),"Production request omitted full settings");outcome.event_settings=request->event_settings;outcome.event_rewards=request->event_rewards;
    require(outcome.event_rewards.has_value(),"Production request omitted physical reward authority");
    // Explicit semantic transport fixture: preserve the Debug spare/car22 words
    // through the private pair without pretending these are gameplay awards.
    outcome.event_rewards->point_words[5]=0x13579bdf;outcome.event_rewards->point_words[7]=1;
    auto missing_rewards=outcome;missing_rewards.event_rewards.reset();rejected=false;try{runtime.complete_event(missing_rewards);}catch(const std::logic_error&){rejected=true;}require(rejected,"Settlement silently dropped physical reward authority");
    const int other=1-tree_type;const auto rank=std::find_if(full_layout.trees[other].begin(),full_layout.trees[other].end(),[](const auto& r){return r.event.type==0&&r.event.number!=32;});require(rank!=full_layout.trees[other].end(),"Original opposite tree lacks rank fixture");
    (*outcome.event_profile)[rank->location.tree_type][rank->location.offset]=1;
    (*outcome.event_profile)[shared0->location.tree_type][shared0->location.offset]=1;
    outcome.event_progress.assign((*outcome.event_profile)[tree_type].begin(),(*outcome.event_profile)[tree_type].end());
    auto conflict=outcome;conflict.event_progress[0]^=1;rejected=false;try{runtime.complete_event(conflict);}catch(const std::logic_error&){rejected=true;}require(rejected,"Conflicting legacy/full settlement state was accepted");
    auto opaque=outcome;opaque.event_profile->at(0).at(255)^=1;opaque.event_progress.assign(opaque.event_profile->at(tree_type).begin(),opaque.event_profile->at(tree_type).end());rejected=false;try{runtime.complete_event(opaque);}catch(const std::logic_error&){rejected=true;}require(rejected,"Unallocated Retail bytes were silently overwritten by settlement");
    const auto fixture=std::filesystem::path("local/task0101")/("frontend-profile-codec-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));PrivateSaveFixture isolated(fixture);
    rejected=false;try{runtime.continue_event_attempt(request->generation,true);}catch(const std::logic_error&){rejected=true;}require(rejected,"Unsaved result authorized another source attempt");
    runtime.complete_event(outcome);
    for(int tree=0;tree<2;++tree)require(std::equal(outcome.event_profile->at(tree).begin(),outcome.event_profile->at(tree).end(),runtime.profile().EventBlock_001FE988(tree)),"Frontend commit lost both-tree semantic state/alias");
    require(runtime.profile().read_u32(0x10e04)==outcome.player_points&&runtime.random_state()==outcome.random_state,"Full commit lost source points/RNG");
    const auto save_path=hp2::platform::profile_store_path(".",hp2::memorycard::kRegionNtsc_001A3390);std::ifstream save(save_path,std::ios::binary);const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(save),{}};hp2::memorycard::Profile_002FC050 saved_profile;
    require(hp2::memorycard::Load_00245D30(bytes,saved_profile)==hp2::memorycard::LoadResult_00245780::kOk,"Both-tree private fixture save failed original size/checksum validation");
    for(int tree=0;tree<2;++tree)require(std::equal(outcome.event_profile->at(tree).begin(),outcome.event_profile->at(tree).end(),saved_profile.EventBlock_001FE988(tree)),"Private save did not retain both original tree blocks");
    rejected=false;try{runtime.complete_event(outcome);}catch(const std::logic_error&){rejected=true;}require(rejected&&runtime.profile().read_u32(0x10e04)==outcome.player_points,"Repeated full settlement was awarded twice");
    // TASK-0112: storage acknowledgement is separate from source retry/next.
    // It neither launches another race nor awards points or consumes RNG.
    const std::vector<std::uint8_t> sealed_bytes(runtime.profile().bytes(),runtime.profile().bytes()+runtime.profile().size());
    const auto prior_generation=request->generation;
    auto baseline=runtime.continue_event_attempt(prior_generation,true);
    require(baseline.previous_generation==prior_generation&&baseline.generation>prior_generation&&baseline.player_points==outcome.player_points&&baseline.random_state==outcome.random_state,"Continuation did not seal the saved career/RNG baseline");
    require(baseline.event_profile&&baseline.event_profile->blocks==*outcome.event_profile,"Continuation lost opposite-tree/shared progress");
    require(baseline.event_rewards&&encode_debug_reward_payload(*baseline.event_rewards)==encode_debug_reward_payload(*outcome.event_rewards),"Saved continuation discarded Debug-only spare/car22 physical rewards");
    require(std::filesystem::exists(save_path+".hp2-debug-rewards.v1"),"Private outcome omitted its metadata/hash-bound reward supplement");
    require(std::equal(sealed_bytes.begin(),sealed_bytes.end(),runtime.profile().bytes())&&runtime.random_state()==outcome.random_state&&!runtime.take_event_launch(),"Storage seal changed profile/RNG or staged another initial launch");
    apply_event_attempt_baseline(*request,baseline);
    require(request->prepared_event.has_value()&&request->generation==baseline.generation,"Storage acknowledgement discarded original launch identity");
    rejected=false;try{runtime.continue_event_attempt(prior_generation,true);}catch(const std::logic_error&){rejected=true;}require(rejected,"Duplicate old generation resealed an attempt");
    rejected=false;try{apply_event_attempt_baseline(*request,baseline);}catch(const std::logic_error&){rejected=true;}require(rejected,"Native duplicate continuation acknowledgement was accepted");
    rejected=false;try{runtime.complete_event(outcome);}catch(const std::logic_error&){rejected=true;}require(rejected,"Old result settled into the new source attempt");
    rejected=false;try{runtime.continue_event_attempt(request->generation,true);}catch(const std::logic_error&){rejected=true;}require(rejected,"Unsettled next attempt inherited previous save acknowledgement");
    // A pause retry needs no result save; the old persisted baseline survives.
    const auto retry=runtime.continue_event_attempt(request->generation,false);apply_event_attempt_baseline(*request,retry);
    outcome.generation=request->generation;runtime.complete_event(outcome);
    require(runtime.profile().read_u32(0x10e04)==outcome.player_points&&runtime.random_state()==outcome.random_state,"Retry settlement re-awarded prior saved points or drew RNG");
    rejected=false;try{runtime.complete_event(outcome);}catch(const std::logic_error&){rejected=true;}require(rejected,"Retry result settlement was not once-only");
    auto expected_return_state=outcome.random_state;
    const auto expected_return_color=std::uint32_t(hp2::frontend::bRandom(-1,expected_return_state));
    runtime.return_from_event();require(runtime.menu_state().active_class=="View Event Tree","Event return did not restore retained frontend tree");
    require(runtime.event_default_color_seed()==expected_return_color,"Normal return omitted the original frontend-loading default-color checkpoint");
    auto returned_screen=*runtime.screen(39);hp2::frontend::tGlobalBackgroundAnimation expected_background;expected_background.Construct_002495F0();
    expected_background.Bind_00249A40(&returned_screen,[&](int bound){return hp2::frontend::bRandom(bound,expected_return_state);},[](const auto&){return hp2::frontend::WidgetMetrics{};});
    require(runtime.random_state()==expected_return_state,"Return default checkpoint did not precede exactly the original page/background draws");
    const auto after_return=runtime.random_state();rejected=false;try{runtime.return_from_event(outcome.random_state);}catch(const std::logic_error&){rejected=true;}
    require(rejected&&runtime.random_state()==after_return&&runtime.event_default_color_seed()==expected_return_color,"Duplicate return consumed another source default draw");
    for(int tree=0;tree<2;++tree)require(std::equal(outcome.event_profile->at(tree).begin(),outcome.event_profile->at(tree).end(),runtime.profile().EventBlock_001FE988(tree)),"Frontend default reset erased settled progress");
    require(!runtime.take_event_launch(),"Returning to tree replayed an old launch");
    std::puts("  authored event transport: real tree/selection/readiness, exact ride/settings, versioned state, one-shot request, stale outcome rejection, retained frontend return");
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
            {"event tree", check_event_tree},{"Control UI contracts",check_ui_contract},
            {"HP authored event transport",[](FrontEndRuntime& runtime){check_event_transport(runtime,1);}},
            {"Championship authored event transport",[](FrontEndRuntime& runtime){check_event_transport(runtime,0);}}};
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
