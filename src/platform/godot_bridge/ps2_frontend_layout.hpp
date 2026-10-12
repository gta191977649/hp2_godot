#pragma once
#include "game_core/frontend/fmenus.hpp"
#include "game_core/frontend/frontendscreens/options.hpp"
#include "game_core/assets/fe_language.hpp"
#include "game_core/audio/sound.hpp"
#include "game_core/audio/bJukebox.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include "game_core/input/joyevents.hpp"
#include "game_core/not_implemented.hpp"
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <memory>
#include <vector>

// TASK-0013: exposes the original FrontEnd screen/widget layout records (hp2::frontend) to
// GDScript so Godot can draw the real authored screens -- real widget names, positions, scales,
// colours and TexturePack references -- instead of a hand-built debug UI. Reads the layout
// straight out of the user's configured ZZDATA copy at runtime; nothing is pre-extracted.
namespace port::godot_bridge {

class Ps2FrontEndLayout : public godot::RefCounted {
    GDCLASS(Ps2FrontEndLayout, godot::RefCounted)
protected:
    static void _bind_methods();

public:
    ~Ps2FrontEndLayout() override;
    // Loads the screen list from a ZZDATA-relative bundle, e.g. "FRONTEND/FRONTB.BUN".
    // Returns {"ok": bool, "screen_count": int, "error": String}.
    godot::Dictionary load(const godot::String& relative_path);

    int get_screen_count() const { return static_cast<int>(screens_.size()); }

    // {"index": int, "width": int, "height": int, "widget_count": int,
    //  "names": PackedStringArray}  -- cheap summary used to pick a screen.
    godot::Dictionary get_screen_info(int index) const;

    // Ordered widget list for one screen; each entry is a Dictionary carrying every decoded
    // field (see src/game_core/frontend/fmenus.hpp for the record layout and its evidence).
    godot::Array get_widgets(int index) const;

    // Loads bitmap font tables from one or more ZZDATA-relative bundles (fonts are spread across
    // FRONTEND/FRONTB.BUN, GLOBAL/GLOBALB.BUN and GLOBAL/INGAMEB.BUN). Returns the font count.
    int load_fonts(const godot::PackedStringArray& bundles);

    // {"line_height": int, "glyphs": {code: PackedFloat32Array[u, v, w, h, advance]}}
    // Empty Dictionary when the font texture hash is unknown.
    godot::Dictionary get_font(std::int64_t texture_hash) const;

    // ---------------------------------------------------------------- recovered menu runtime
    // Port-side binding for hp2::frontend::MenuGroup_0016AA00 / MenuList_00169F78: builds the
    // menu for one already-loaded screen exactly the way FUN_0016A858 does (scan the screen for
    // "MENUITEM_%d" type-1 widgets, 0..15) and drives it with recovered JOY_EVENT_* ids.
    //
    // The menu mutates the loaded screen's widget colours in place, through the recovered
    // FUN_00169BD0/FUN_00140CA0 path, so after an event the renderer just re-reads
    // get_widget_colours() for the same screen.
    //
    // {"ok": bool, "item_count": int, "current": int, "items": Array[Dictionary], "error": String}
    godot::Dictionary menu_open(int screen_index);

    // Feeds one recovered joypad event into the menu. Returns menu_state(), plus
    // "not_implemented": String when the recovered path that would have handled the event raises
    // hp2::NotImplemented (AGENT.md R3) -- the message is surfaced, not swallowed.
    godot::Dictionary menu_joy_event(int pad_index, int joy_event, bool pressed);

    // {"screen": int, "item_count": int, "current": int, "current_name": String,
    //  "current_name_hash": int, "items": Array, "sound_id": int, "sound_name": String,
    //  "messages": PackedStringArray}
    godot::Dictionary menu_state() const;

    // Live widget colours for a screen, in the same order as get_widgets(). Separate from
    // get_widgets() so the renderer can refresh colours every frame without rebuilding
    // dictionaries for 1500 widgets.
    godot::PackedColorArray get_widget_colours(int index) const;

    // Port clock/measurement seams for the recovered main-menu animation. Measurements are
    // authored font/texture dimensions at the widget's current scale, not Godot UI geometry.
    void set_widget_measurements(int index, const godot::PackedVector2Array& dimensions);
    godot::Dictionary menu_tick(double delta);
    godot::Array get_widget_dynamic(int index) const;
    godot::Array drain_menu_audio();
    void title_visual_state(bool movie_state, bool can_start);
    void title_pulse_restart();
    godot::Dictionary frontend_music(bool previous_finished);

    // Verbatim original name of a JOY_EVENT_* id (ELF table 0x002FA2D0); empty when unlisted.
    godot::String joy_event_name(int joy_event) const;

    // Index of the screen whose recovered name hash matches, or -1 (hp2::frontend's
    // screen_index_for_screen_name; the original uses the same hash as a document handle).
    int find_screen_by_name(const godot::String& screen_name) const;

    // Deliberately reaches a recovered-but-unimplemented path so the UI can show what is still
    // missing. `which` is one of "popup", "highlight_animation", "localised_text".
    // Returns {"not_implemented": String} or {"error": String} for an unknown key.
    godot::Dictionary probe_not_implemented(const godot::String& which);

    godot::String get_last_error() const { return last_error_; }

private:
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
    void apply_language(int language);
    void request_history_back();
    hp2::frontend::Type_002495F0 background_animation_;
    std::uint32_t random_state_ = 0x12345678; // Original initialized seed; prior boot draws unresolved.
    void bind_scrolling_text(std::uint32_t name_hash);
    void open_menu_animation(int screen_index);
    void bind_menu_animation(std::uint32_t name_hash);
    hp2::frontend::WidgetMetrics measure_widget(const hp2::frontend::FeWidget& widget) const;
    hp2::frontend::MenuHighlightAnim_00248870 highlight_animation_;
    bool title_movie_state_ = false;
    double menu_clock_seconds_ = 0.0; // Host clock, not a reconstructed PS2 frame counter.
    std::vector<std::vector<hp2::frontend::WidgetMetrics>> widget_base_metrics_;
    std::vector<hp2::frontend::FeScreen> screens_;
    std::vector<hp2::frontend::FeScreen> authored_screens_; // Reload projection on each screen entry.
    std::vector<hp2::frontend::FeFont> fonts_;
    godot::String last_error_;
    std::unique_ptr<hp2::frontend::MenuGroup_0016AA00> menu_group_;
    int menu_screen_index_ = -1;
    std::uint32_t activated_name_hash_ = 0;
    std::int32_t dat_002FC070_ = -1; // Unknown initial value; written only by recovered actions.
    std::string active_class_name_;
    std::string requested_class_for_open_;
    std::string pending_class_name_;
    bool pending_push_history_ = false;
    std::unique_ptr<hp2::frontend::Type_002490D8> exit_animation_;
    std::string exit_class_name_;
    std::vector<std::string> navigation_history_; // 0016B418 stores class-name strings, not layout indices. // Port projection of recovered screen history.
    void dispatch_activation(std::uint32_t name_hash);
    void apply_pending_screen(); // Deferred until callback returns to preserve list lifetime.
    std::vector<godot::String> message_log_;
};

} // namespace port::godot_bridge
