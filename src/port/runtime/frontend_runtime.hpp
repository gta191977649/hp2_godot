#pragma once
#include "game_core/port/assets/fe_language.hpp"
#include "game_core/bSound/include/bJukebox.h"
#include "game_core/indep/src/sound.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/frontend/frontendscreens/modeselect.hpp"
#include "game_core/indep/src/frontendscreens/options.hpp"
#include "port/assets/catalog.hpp"
#include "port/runtime/event_tree_view.hpp"
#include "port/runtime/frame_builder.hpp"
#include "platform/ps2_profile_store.hpp"
#include <memory>
#include <string>
#include <vector>

// TASK-0018 / D008: the front end's runtime, with no Godot and no backend.
//
// This owns everything the front end is: the parsed screens and fonts, the recovered menu
// group, localisation, the jukebox and sample bank, the recovered animations, navigation
// history and the mode-select globals. It was previously held by the Godot adapter
// Ps2FrontEndLayout, which now only translates these native results into Godot values.
//
// Every result below is a plain C++ value. `not_implemented` carries the message of a
// recovered-but-untranslated path (AGENT.md R3): it is surfaced, never swallowed, and never
// replaced by a silent default.
namespace port::runtime {

struct LoadResult {
    bool ok = false;
    int screen_count = 0;
    std::string error;
};

struct MenuItem {
    std::uint32_t name_hash = 0;
    int menuitem_index = 0;
    std::uint32_t state = 0;
    bool enabled = false, highlighted = false, current = false;
};

struct MenuState {
    bool ok = false;
    // False when no menu list is open; a host projection then has no item/sound/message fields
    // to report, which is distinct from an open menu that happens to hold no items.
    bool has_list = false;
    std::string error, not_implemented;
    int screen = -1;
    std::int32_t dat_002FC070 = -1;
    std::string active_class;
    int history_depth = 0;
    bool exiting = false;
    int item_count = 0, current = -1;
    std::vector<MenuItem> items;
    std::uint32_t current_name_hash = 0, activated_name_hash = 0;
    std::string current_name;
    int sound_id = 0;
    std::string sound_name;
    std::vector<std::string> messages;
};

struct TickResult {
    bool ok = false;
    int screen = -1;
    std::string error, not_implemented;
};

struct ScreenInfo {
    int index = 0, width = 0, height = 0, widget_count = 0;
    std::uint32_t name_hash = 0;
    std::string screen_name; // empty when the hash has not been matched to a recovered name
    std::vector<std::string> widget_names;
};

// One decoded original sample, ready for whatever audio device the host owns.
struct SoundRequest {
    std::string event;
    hp2::port::audio::PcmSound pcm;
    float gain = 1, pitch_scale = 1;
    std::string not_implemented;
};

struct MusicRequest {
    bool ok = false;
    bool has_track = false;
    hp2::port::audio::PcmSound pcm;
    float gain = 1, pitch_scale = 1;
    int index = 0;
    std::string name;
    std::string not_implemented;
};

class FrontEndRuntime {
public:
    FrontEndRuntime() = default;
    ~FrontEndRuntime();
    // The recovered menu group holds a pointer into `screens_`, and the global menu sound sink
    // holds `this`, so this object must not move or be copied.
    FrontEndRuntime(const FrontEndRuntime&) = delete;
    FrontEndRuntime& operator=(const FrontEndRuntime&) = delete;

    // `asset_search_start_dir` is where the host wants the ZZDATA root search to begin;
    // resolution itself is hp2::platform::resolve_zzdata_root, which is already host-free.
    LoadResult load(const std::string& asset_search_start_dir, const std::string& relative_path);
    int load_fonts(const std::vector<std::string>& bundles);

    int screen_count() const { return static_cast<int>(screens_.size()); }
    ScreenInfo screen_info(int index) const;
    const hp2::frontend::FeScreen* screen(int index) const;
    const std::vector<hp2::frontend::FeFont>& fonts() const { return fonts_; }
    const hp2::frontend::FeFont* font(std::uint32_t texture_hash) const;
    int find_screen_by_name(const std::string& screen_name) const;

    // ----------------------------------------------------------------- recovered menu runtime
    MenuState menu_open(int screen_index);
    MenuState menu_joy_event(int pad_index, int joy_event, bool pressed);
    MenuState menu_state() const;
    TickResult menu_tick(double delta);
    std::vector<SoundRequest> drain_menu_audio();
    MusicRequest frontend_music(bool previous_finished);
    void title_visual_state(bool movie_state, bool can_start);
    void title_pulse_restart();

    // Presentation state the recovered startup sequence determines, so the frame builder, not
    // a host script, decides what the title screen shows.
    void set_startup_presentation(bool movie_visible, bool letterbox, bool startup_active);

    // Builds one frame for the open screen. `resources` must also resolve the live host
    // surfaces (kIntroMovieTexture, kBackgroundMovieTexture): the authored size of those quads
    // comes from the live movie.
    FrameStats build_frame(render::Resources& resources, render::Frame& frame);

    // The original-asset texture catalog, or nullptr before a successful load().
    assets::Catalog* catalog() { return catalog_.get(); }

    // The View Event Tree screen (FRONTB.BUN screen 39, class "View Event Tree", instance pool
    // "tEventTreeView"). The view loads when that screen opens and clears when another does; its
    // overlay is appended by build_frame. Per-event progress comes from the memory-card profile
    // (TASK-0021); with nothing configured that profile is all zero, so every event but the root
    // reports Locked -- which is the recovered answer for an untouched save, not a shortcut.
    const EventTreeView& event_tree() const { return event_tree_; }

    // The player profile this runtime loaded, and how that load went. `loaded()` false with an
    // error of "not configured" is the normal, expected state.
    const hp2::memorycard::Profile_002FC050& profile() const { return profile_; }
    const hp2::platform::ProfileLoad& profile_load() const { return profile_load_; }
    bool move_event_cursor(int direction) { return event_tree_.move_cursor(direction); }
    // FUN_001FAF60 state 0 drives the tree directly. Returns true when the event handled it.
    bool event_tree_joy_event(int joy_event);
    const std::string& event_tree_error() const { return event_tree_error_; }

    // The widget's original clip rectangle, or an empty rect when it has none.
    render::Rect widget_clip(int screen_index, std::size_t slot) const;

    // Deliberately reaches a recovered-but-unimplemented path so a host can show what is still
    // missing. `which` is "popup", "highlight_animation" or "localised_text". Returns the
    // NotImplemented message, or sets `error` for an unknown key or a path that did not raise.
    struct ProbeResult { std::string not_implemented, error; };
    ProbeResult probe_not_implemented(const std::string& which);

    const std::string& last_error() const { return last_error_; }

private:
    void apply_language(int language);
    void request_history_back();
    void bind_scrolling_text(std::uint32_t name_hash);
    void open_menu_animation(int screen_index);
    void bind_menu_animation(std::uint32_t name_hash);
    void dispatch_activation(hp2::frontend::MenuEvent_0016A458& event);
    void apply_pending_screen(); // Deferred until callback returns to preserve list lifetime.
    hp2::frontend::WidgetMetrics measure_widget(const hp2::frontend::FeWidget& widget) const;
    WidgetSizes::Redirect ghost_redirect() const;
    void reset_state();

    std::unique_ptr<assets::DirectorySource> asset_source_;
    std::unique_ptr<assets::Catalog> catalog_;
    std::unique_ptr<hp2::port::assets::FeLanguage> language_;
    std::unique_ptr<hp2::port::audio::GuestSampleBank> sound_bank_;
    std::unique_ptr<hp2::port::audio::JukeboxCartridge> jukebox_;
    std::unique_ptr<hp2::port::audio::FrontendMusic> music_scheduler_;
    std::vector<std::string> pending_sound_names_;
    std::string last_sound_name_;
    double sound_clock_seconds_ = 0, last_sound_seconds_ = -1;
    std::unique_ptr<hp2::frontend::Type_002491B0> scrolling_text_;
    std::unique_ptr<hp2::frontend::Type_00249DE0> menu_viewport_;
    std::unique_ptr<hp2::frontend::Type_0024A690> menu_selector_;
    int port_language_ = 0; // Explicit host initial locale, English.
    hp2::frontend::Type_002495F0 background_animation_;
    std::uint32_t random_state_ = 0x12345678; // Original initialized seed; prior boot draws unresolved.
    hp2::frontend::MenuHighlightAnim_00248870 highlight_animation_;
    bool title_movie_state_ = false;
    bool startup_movie_visible_ = false, startup_letterbox_ = false, startup_active_ = false;
    double menu_clock_seconds_ = 0.0; // Host clock, not a reconstructed PS2 frame counter.
    std::vector<hp2::frontend::FeScreen> screens_;
    std::vector<hp2::frontend::FeScreen> authored_screens_; // Reload projection on each screen entry.
    std::vector<hp2::frontend::FeFont> fonts_;
    std::string last_error_;
    std::unique_ptr<hp2::frontend::MenuGroup_0016AA00> menu_group_;
    int menu_screen_index_ = -1;
    std::uint32_t activated_name_hash_ = 0;
    std::int32_t dat_002FC070_ = -1; // Unknown initial value; written only by recovered actions.
    // Cops On / Cops Off Mode Select globals (FUN_002174A0 / FUN_00217AF8). (E-0016-03)
    port::frontend::ModeSelectGlobals mode_select_globals_;
    std::string active_class_name_;
    std::string requested_class_for_open_;
    std::string pending_class_name_;
    bool pending_push_history_ = false;
    std::unique_ptr<hp2::frontend::Type_002490D8> exit_animation_;
    std::string exit_class_name_;
    std::vector<std::string> navigation_history_; // 0016B418 stores class-name strings, not layout indices.
    std::vector<std::string> message_log_;

    // TASK-0021: the player profile, loaded once by load() and handed to every screen that
    // reads it. All zero when nothing is configured.
    hp2::memorycard::Profile_002FC050 profile_;
    hp2::platform::ProfileLoad profile_load_;

    // E-0021-23: copy the profile-resident mode-select globals into the profile.
    void commit_mode_select_globals();

    // FRONTB.BUN screen 39, document hash 0x147F8A59, class "View Event Tree". The mode is
    // profile+0x1072C and is no longer a field here: FUN_001FB218 reads it from the profile.
    static constexpr int kEventTreeScreen = 39;
    EventTreeView event_tree_;
    std::string event_tree_error_;
    std::string event_tree_bundle_;
};

} // namespace port::runtime
