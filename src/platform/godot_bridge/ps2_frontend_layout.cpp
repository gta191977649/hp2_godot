#include "ps2_frontend_layout.hpp"
#include "boundary.hpp"
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "game_core/port/not_implemented.hpp"

namespace port::godot_bridge {
namespace {

// Where the ZZDATA root search starts. This is the one thing the runtime cannot do itself:
// it needs the project/executable location, which only the host knows.
std::string search_start_dir() {
    godot::String project_path = godot::ProjectSettings::get_singleton()->globalize_path("res://");
    if (!project_path.is_empty()) return std::string(project_path.utf8().get_data());
    return std::string(godot::OS::get_singleton()->get_executable_path().get_base_dir().utf8().get_data());
}

godot::String to_godot(const std::string& text) { return godot::String::utf8(text.c_str()); }

godot::Ref<godot::AudioStreamWAV> pcm_stream(const hp2::port::audio::PcmSound& pcm) {
    godot::PackedByteArray data;
    data.resize(static_cast<int64_t>(pcm.samples.size() * 2));
    for (std::size_t i = 0; i < pcm.samples.size(); ++i) {
        const auto sample = static_cast<std::uint16_t>(pcm.samples[i]);
        data.set(i * 2, sample & 0xff);
        data.set(i * 2 + 1, sample >> 8);
    }
    godot::Ref<godot::AudioStreamWAV> stream;
    stream.instantiate();
    stream->set_format(godot::AudioStreamWAV::FORMAT_16_BITS);
    stream->set_mix_rate(static_cast<int>(pcm.sample_rate));
    stream->set_stereo(pcm.channels == 2);
    stream->set_data(data);
    return stream;
}

// Keeps the shape the older host scene reads: a failed call carries only its own outcome, and
// a state with no open menu list carries no item, sound or message fields.
godot::Dictionary to_dictionary(const port::runtime::MenuState& state) {
    godot::Dictionary result;
    result["ok"] = state.ok;
    if (!state.error.empty()) result["error"] = to_godot(state.error);
    if (!state.not_implemented.empty()) result["not_implemented"] = to_godot(state.not_implemented);
    if (!state.ok && !state.has_list) return result;

    result["screen"] = state.screen;
    result["dat_002FC070"] = state.dat_002FC070;
    result["active_class"] = to_godot(state.active_class);
    godot::Dictionary selection;selection["active"]=state.selection.active;selection["loaded"]=state.selection.loaded;selection["car"]=state.selection.car;selection["skin"]=state.selection.skin;selection["overlay"]=state.selection.overlay;selection["visible_cars"]=state.selection.visible_cars;selection["generation"]=static_cast<int64_t>(state.selection.generation);result["selection"]=selection;
    result["history_depth"] = state.history_depth;
    result["exiting"] = state.exiting;
    result["item_count"] = state.item_count;
    result["current"] = state.current;
    if (!state.has_list) return result;

    godot::Array items;
    for (const auto& item : state.items) {
        godot::Dictionary entry;
        entry["name_hash"] = static_cast<std::int64_t>(item.name_hash);
        entry["menuitem_index"] = item.menuitem_index;
        entry["state"] = static_cast<std::int64_t>(item.state);
        entry["enabled"] = item.enabled;
        entry["highlighted"] = item.highlighted;
        entry["current"] = item.current;
        items.append(entry);
    }
    result["items"] = items;
    result["current_name_hash"] = static_cast<std::int64_t>(state.current_name_hash);
    result["current_name"] = to_godot(state.current_name);
    result["activated_name_hash"] = static_cast<std::int64_t>(state.activated_name_hash);
    result["sound_id"] = state.sound_id;
    result["sound_name"] = to_godot(state.sound_name);
    godot::PackedStringArray messages;
    for (const auto& line : state.messages) messages.push_back(to_godot(line));
    result["messages"] = messages;
    return result;
}

// What a bound method returns when something unexpected escaped the runtime. The port keeps
// running and the host sees the message instead of Godot dying inside the extension.
godot::Dictionary boundary_failure(const std::string& error) {
    godot::Dictionary result;
    result["ok"] = false;
    result["error"] = to_godot("port error crossing the GDExtension boundary: " + error);
    return result;
}

} // namespace

void Ps2FrontEndLayout::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("load", "relative_path"), &Ps2FrontEndLayout::load);
    godot::ClassDB::bind_method(godot::D_METHOD("load_fonts", "bundles"), &Ps2FrontEndLayout::load_fonts);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_open", "screen_index"), &Ps2FrontEndLayout::menu_open);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_joy_event", "pad_index", "joy_event", "pressed"), &Ps2FrontEndLayout::menu_joy_event);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_state"), &Ps2FrontEndLayout::menu_state);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_tick", "delta"), &Ps2FrontEndLayout::menu_tick);
    godot::ClassDB::bind_method(godot::D_METHOD("drain_menu_audio"), &Ps2FrontEndLayout::drain_menu_audio);
    godot::ClassDB::bind_method(godot::D_METHOD("event_tree_joy_event", "joy_event"), &Ps2FrontEndLayout::event_tree_joy_event);
    godot::ClassDB::bind_method(godot::D_METHOD("title_visual_state", "movie_state", "can_start"), &Ps2FrontEndLayout::title_visual_state);
    godot::ClassDB::bind_method(godot::D_METHOD("title_pulse_restart"), &Ps2FrontEndLayout::title_pulse_restart);
    godot::ClassDB::bind_method(godot::D_METHOD("set_startup_presentation", "movie_visible", "letterbox", "startup_active"), &Ps2FrontEndLayout::set_startup_presentation);
    godot::ClassDB::bind_method(godot::D_METHOD("frontend_music", "previous_finished"), &Ps2FrontEndLayout::frontend_music);
    godot::ClassDB::bind_method(godot::D_METHOD("start_frontend_music"), &Ps2FrontEndLayout::start_frontend_music);
    godot::ClassDB::bind_method(godot::D_METHOD("find_screen_by_name", "screen_name"), &Ps2FrontEndLayout::find_screen_by_name);
}

godot::Dictionary Ps2FrontEndLayout::load(const godot::String& relative_path) {
    const auto loaded = runtime_.load(search_start_dir(), std::string(relative_path.utf8().get_data()));
    godot::Dictionary result;
    result["ok"] = loaded.ok;
    result["screen_count"] = loaded.screen_count;
    result["error"] = to_godot(loaded.error);
    return result;
}





int Ps2FrontEndLayout::load_fonts(const godot::PackedStringArray& bundles) {
    std::vector<std::string> paths;
    for (int i = 0; i < bundles.size(); ++i) paths.emplace_back(bundles[i].utf8().get_data());
    return runtime_.load_fonts(paths);
}


godot::Dictionary Ps2FrontEndLayout::menu_open(int screen_index) {
    port::runtime::MenuState state;
    const auto failure = catch_all([&] { state = runtime_.menu_open(screen_index); });
    return failure.empty() ? to_dictionary(state) : boundary_failure(failure);
}

godot::Dictionary Ps2FrontEndLayout::menu_joy_event(int pad_index, int joy_event, bool pressed) {
    port::runtime::MenuState state;
    const auto failure =
        catch_all([&] { state = runtime_.menu_joy_event(pad_index, joy_event, pressed); });
    return failure.empty() ? to_dictionary(state) : boundary_failure(failure);
}

godot::Dictionary Ps2FrontEndLayout::menu_state() const {
    auto state = runtime_.menu_state();
    state.ok = true; // a plain query always succeeds; the shape is what the caller reads
    auto result = to_dictionary(state);
    result.erase("ok");
    return result;
}

godot::Dictionary Ps2FrontEndLayout::menu_tick(double delta) {
    port::runtime::TickResult tick;
    const auto failure = catch_all([&] { tick = runtime_.menu_tick(delta); });
    if (!failure.empty()) return boundary_failure(failure);
    godot::Dictionary result;
    result["ok"] = tick.ok;
    if (!tick.error.empty()) result["error"] = to_godot(tick.error);
    if (!tick.not_implemented.empty()) result["not_implemented"] = to_godot(tick.not_implemented);
    if (tick.ok) result["screen"] = tick.screen;
    return result;
}


godot::Array Ps2FrontEndLayout::drain_menu_audio() {
    godot::Array result;
    for (const auto& request : runtime_.drain_menu_audio()) {
        godot::Dictionary entry;
        entry["event"] = to_godot(request.event);
        if(request.operation==1){entry["stop"]=true;}
        else if (request.not_implemented.empty()) {
            entry["stream"] = pcm_stream(request.pcm);
            entry["pitch_scale"] = request.pitch_scale;
            entry["gain"] = request.gain;
        } else {
            entry["not_implemented"] = to_godot(request.not_implemented);
        }
        result.append(entry);
    }
    return result;
}

namespace {
godot::Dictionary music_dictionary(const port::runtime::MusicRequest& request) {
    godot::Dictionary result;
    result["ok"] = request.ok;
    result["stop"] = request.stop;
    result["gain"] = request.gain;
    result["output_mode"] = request.output_mode;
    if (!request.not_implemented.empty()) result["not_implemented"] = to_godot(request.not_implemented);
    if (request.has_track) {
        result["stream"] = pcm_stream(request.pcm);
        result["gain"] = request.gain;
        result["pitch_scale"] = request.pitch_scale;
        result["index"] = request.index;
        result["name"] = godot::String(request.name.c_str());
    }
    return result;
}

} // namespace

godot::Dictionary Ps2FrontEndLayout::frontend_music(bool previous_finished) {
    return music_dictionary(runtime_.frontend_music(previous_finished));
}
godot::Dictionary Ps2FrontEndLayout::start_frontend_music() {
    return music_dictionary(runtime_.frontend_music(false,true));
}

bool Ps2FrontEndLayout::event_tree_joy_event(int joy_event) {
    bool handled = false;
    std::string gap;
    const auto failure = catch_all([&] {
        gap = hp2::guard([&] { handled = runtime_.event_tree_joy_event(joy_event); });
    });
    if (!gap.empty() || !failure.empty()) {
        godot::UtilityFunctions::push_error(to_godot(gap.empty() ? failure : gap));
        return true;
    }
    return handled;
}


void Ps2FrontEndLayout::title_visual_state(bool movie_state, bool can_start) {
    runtime_.title_visual_state(movie_state, can_start);
}

void Ps2FrontEndLayout::title_pulse_restart() { runtime_.title_pulse_restart(); }

void Ps2FrontEndLayout::set_startup_presentation(bool movie_visible, bool letterbox,
                                                 bool startup_active) {
    runtime_.set_startup_presentation(movie_visible, letterbox, startup_active);
}


int Ps2FrontEndLayout::find_screen_by_name(const godot::String& screen_name) const {
    return runtime_.find_screen_by_name(screen_name.utf8().get_data());
}



} // namespace port::godot_bridge
