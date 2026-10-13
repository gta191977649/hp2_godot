#include "ps2_frontend_layout.hpp"
#include "platform/ps2_asset_root.hpp"
#include "game_core/frontend/frontendscreens/options.hpp"
#include "game_core/frontend/frontendscreens/registry_0016adb0.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <cstring>
#include <memory>
#include <cmath>
#include <cstdio>
#include <fstream>

namespace port::godot_bridge {
namespace {

std::string search_start_dir() {
    godot::String project_path = godot::ProjectSettings::get_singleton()->globalize_path("res://");
    if (!project_path.is_empty()) return std::string(project_path.utf8().get_data());
    return std::string(godot::OS::get_singleton()->get_executable_path().get_base_dir().utf8().get_data());
}

godot::Ref<godot::AudioStreamWAV> pcm_stream(const hp2::port::audio::PcmSound& pcm) {
    godot::PackedByteArray data;
    data.resize(static_cast<int64_t>(pcm.samples.size() * 2));
    for (std::size_t i = 0; i < pcm.samples.size(); ++i) {
        const auto sample = static_cast<std::uint16_t>(pcm.samples[i]);
        data.set(i*2, sample & 0xff); data.set(i*2+1, sample >> 8);
    }
    godot::Ref<godot::AudioStreamWAV> stream;
    stream.instantiate();
    stream->set_format(godot::AudioStreamWAV::FORMAT_16_BITS);
    stream->set_mix_rate(static_cast<int>(pcm.sample_rate));
    stream->set_stereo(pcm.channels == 2); stream->set_data(data);
    return stream;
}

} // namespace

Ps2FrontEndLayout::~Ps2FrontEndLayout() {
    hp2::frontend::install_menu_sound_sink({}); // Global host callback must not outlive this owner.
}

void Ps2FrontEndLayout::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("load", "relative_path"), &Ps2FrontEndLayout::load);
    godot::ClassDB::bind_method(godot::D_METHOD("get_screen_count"), &Ps2FrontEndLayout::get_screen_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_screen_info", "index"), &Ps2FrontEndLayout::get_screen_info);
    godot::ClassDB::bind_method(godot::D_METHOD("get_widgets", "index"), &Ps2FrontEndLayout::get_widgets);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_error"), &Ps2FrontEndLayout::get_last_error);
    godot::ClassDB::bind_method(godot::D_METHOD("load_fonts", "bundles"), &Ps2FrontEndLayout::load_fonts);
    godot::ClassDB::bind_method(godot::D_METHOD("get_font", "texture_hash"), &Ps2FrontEndLayout::get_font);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_open", "screen_index"), &Ps2FrontEndLayout::menu_open);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_joy_event", "pad_index", "joy_event", "pressed"), &Ps2FrontEndLayout::menu_joy_event);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_state"), &Ps2FrontEndLayout::menu_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_widget_colours", "index"), &Ps2FrontEndLayout::get_widget_colours);
    godot::ClassDB::bind_method(godot::D_METHOD("set_widget_measurements", "index", "dimensions"), &Ps2FrontEndLayout::set_widget_measurements);
    godot::ClassDB::bind_method(godot::D_METHOD("get_widget_dynamic", "index"), &Ps2FrontEndLayout::get_widget_dynamic);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_tick", "delta"), &Ps2FrontEndLayout::menu_tick);
    godot::ClassDB::bind_method(godot::D_METHOD("drain_menu_audio"), &Ps2FrontEndLayout::drain_menu_audio);
    godot::ClassDB::bind_method(godot::D_METHOD("title_visual_state", "movie_state", "can_start"), &Ps2FrontEndLayout::title_visual_state);
    godot::ClassDB::bind_method(godot::D_METHOD("title_pulse_restart"), &Ps2FrontEndLayout::title_pulse_restart);
    godot::ClassDB::bind_method(godot::D_METHOD("frontend_music", "previous_finished"), &Ps2FrontEndLayout::frontend_music);
    godot::ClassDB::bind_method(godot::D_METHOD("joy_event_name", "joy_event"), &Ps2FrontEndLayout::joy_event_name);
    godot::ClassDB::bind_method(godot::D_METHOD("probe_not_implemented", "which"), &Ps2FrontEndLayout::probe_not_implemented);
    godot::ClassDB::bind_method(godot::D_METHOD("find_screen_by_name", "screen_name"), &Ps2FrontEndLayout::find_screen_by_name);
    godot::ClassDB::bind_method(godot::D_METHOD("name_hash", "name"), &Ps2FrontEndLayout::name_hash);
}

godot::Dictionary Ps2FrontEndLayout::load(const godot::String& relative_path) {
    godot::Dictionary result;
    menu_group_.reset();          // holds a pointer into screens_, which is about to reallocate
    menu_screen_index_ = -1;
    navigation_history_.clear();
    port_language_ = 0;
    screens_.clear();
    scrolling_text_.reset();
    menu_viewport_.reset();
    menu_selector_.reset();
    language_.reset();
    sound_bank_.reset();
    music_scheduler_.reset(); jukebox_.reset();
    pending_sound_names_.clear();
    last_sound_name_.clear();
    sound_clock_seconds_ = 0; last_sound_seconds_ = -1;
    background_animation_.Construct_002495F0();
    highlight_animation_ = {};
    widget_base_metrics_.clear();
    pending_class_name_.clear();
    active_class_name_.clear();
    navigation_history_.clear();
    exit_animation_.reset();
    authored_screens_.clear();
    highlight_animation_ = {};
    widget_base_metrics_.clear();

    auto root = hp2::platform::resolve_zzdata_root(search_start_dir());
    if (!root) {
        last_error_ = "PS2 asset root not configured (set HP2_PS2_ASSET_ROOT or create config/ps2_assets.local.json).";
        result["ok"] = false; result["screen_count"] = 0; result["error"] = last_error_;
        return result;
    }
    godot::String full_path = godot::String::utf8(root->c_str()).path_join(relative_path);
    godot::Ref<godot::FileAccess> file = godot::FileAccess::open(full_path, godot::FileAccess::READ);
    if (file.is_null()) {
        last_error_ = godot::String("Could not open ") + full_path;
        result["ok"] = false; result["screen_count"] = 0; result["error"] = last_error_;
        return result;
    }
    godot::PackedByteArray bytes = file->get_buffer(file->get_length());
    file->close();

    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(bytes.size()));
    if (!buffer.empty()) std::memcpy(buffer.data(), bytes.ptr(), buffer.size());

    screens_ = hp2::frontend::parse_screens(buffer);
    try {
        // English is the port's explicit initial language choice, not guessed text.
        std::ifstream language_file(*root + "/LANGUAGES/FE_ENGLISH.BIN", std::ios::binary);
        if (!language_file) throw std::runtime_error("could not open LANGUAGES/FE_ENGLISH.BIN");
        std::vector<std::uint8_t> language_bytes((std::istreambuf_iterator<char>(language_file)), {});
        language_ = std::make_unique<hp2::port::assets::FeLanguage>(language_bytes);
        for (auto& screen : screens_) for (auto& widget : screen.widgets) {
            if (widget.string_id_0x60 != 0 &&
                !language_->lookup(widget.string_id_0x60, widget.text))
                widget.text = "NOT TRANSLATED PROPERLY"; // ELF 0x002BC958, E-0016-S01.
        }
        sound_bank_ = std::make_unique<hp2::port::audio::GuestSampleBank>(
            hp2::port::audio::GuestSampleBank::load_from_zzdata(*root));
        jukebox_ = std::make_unique<hp2::port::audio::JukeboxCartridge>(
            hp2::port::audio::JukeboxCartridge::load_from_zzdata(*root));
        music_scheduler_ = std::make_unique<hp2::port::audio::FrontendMusic>(jukebox_->tunes());
    } catch (const std::exception& error) {
        result["ok"] = false;
        last_error_ = godot::String::utf8(error.what());
        result["error"] = last_error_;
        return result;
    }
    authored_screens_ = screens_;
    if (screens_.empty()) {
        last_error_ = full_path + " contains no frontend screen list chunk.";
        result["ok"] = false; result["screen_count"] = 0; result["error"] = last_error_;
        return result;
    }
    last_error_ = "";
    result["ok"] = true;
    result["screen_count"] = get_screen_count();
    result["error"] = "";
    return result;
}

godot::Dictionary Ps2FrontEndLayout::get_screen_info(int index) const {
    godot::Dictionary info;
    if (index < 0 || index >= get_screen_count()) return info;
    const auto& screen = screens_[static_cast<std::size_t>(index)];
    info["index"] = screen.index;
    info["width"] = screen.width;
    info["height"] = screen.height;
    info["widget_count"] = static_cast<int>(screen.widgets.size());
    info["name_hash"] = static_cast<std::int64_t>(screen.name_hash_0x08);
    // The screen's own recovered name, where the hash at 0x10203 +0x08 has been matched.
    info["screen_name"] = godot::String();
    std::size_t recovered_count = 0;
    const auto* recovered = hp2::frontend::recovered_screen_names(recovered_count);
    for (std::size_t i = 0; i < recovered_count; ++i) {
        if (recovered[i].screen_index == screen.index) {
            info["screen_name"] = godot::String::utf8(recovered[i].name);
            break;
        }
    }
    godot::PackedStringArray names;
    for (const auto& w : screen.widgets) {
        if (!w.name.empty()) names.push_back(godot::String::utf8(w.name.c_str()));
    }
    info["names"] = names;
    return info;
}

godot::Array Ps2FrontEndLayout::get_widgets(int index) const {
    godot::Array widgets;
    if (index < 0 || index >= get_screen_count()) return widgets;
    for (const auto& w : screens_[static_cast<std::size_t>(index)].widgets) {
        godot::Dictionary d;
        d["chunk_id"] = static_cast<std::int64_t>(w.chunk_id);
        d["is_text"] = w.chunk_id == hp2::frontend::kTextWidgetId;
        d["name"] = godot::String::utf8(w.name.c_str());
        d["name_hash"] = static_cast<std::int64_t>(w.name_hash);
        d["class_hash"] = static_cast<std::int64_t>(w.class_hash);
        d["type"] = w.type;
        d["visible"] = w.visible;
        d["layer"] = w.layer;
        d["x"] = w.x; d["y"] = w.y;
        d["scale_x"] = w.scale_x; d["scale_y"] = w.scale_y;
        d["texture_hash"] = static_cast<std::int64_t>(w.texture_hash);
        d["clip_hash"] = static_cast<std::int64_t>(w.clip_hash_0x4c);
        // Initial get_widgets builds measurement inputs; dynamic reads follow that host phase.
        if (index == menu_screen_index_ && static_cast<std::size_t>(index) < widget_base_metrics_.size() &&
            widget_base_metrics_[index].size() == screens_[index].widgets.size()) {
            auto& screen = const_cast<hp2::frontend::FeScreen&>(screens_[index]);
            const auto clip = hp2::frontend::GetWidgetClip_00145F30(screen,w,
                [this](const auto& widget) { return measure_widget(widget); });
            if (clip.valid) d["clip_rect"] = godot::Rect2(clip.x0,clip.y0,clip.x1-clip.x0,clip.y1-clip.y0);
        }
        // PS2 colour bytes: 0x80 is the neutral/opaque value, so scale by 128 and clamp.
        d["color_r"] = static_cast<double>(w.r) / 128.0;
        d["color_g"] = static_cast<double>(w.g) / 128.0;
        d["color_b"] = static_cast<double>(w.b) / 128.0;
        d["color_a"] = static_cast<double>(w.a) / 128.0;
        d["text"] = godot::String(w.text.c_str());
        d["string_id"] = static_cast<std::int64_t>(w.string_id_0x60);
        d["text_align"] = static_cast<std::int64_t>(w.text_align);
        d["text_capacity"] = static_cast<std::int64_t>(w.text_capacity);
        widgets.append(d);
    }
    return widgets;
}

int Ps2FrontEndLayout::load_fonts(const godot::PackedStringArray& bundles) {
    fonts_.clear();
    auto root = hp2::platform::resolve_zzdata_root(search_start_dir());
    if (!root) {
        last_error_ = "PS2 asset root not configured.";
        return 0;
    }
    for (int i = 0; i < bundles.size(); ++i) {
        godot::String full_path = godot::String::utf8(root->c_str()).path_join(bundles[i]);
        godot::Ref<godot::FileAccess> file = godot::FileAccess::open(full_path, godot::FileAccess::READ);
        if (file.is_null()) continue;
        godot::PackedByteArray bytes = file->get_buffer(file->get_length());
        file->close();
        std::vector<std::uint8_t> buffer(static_cast<std::size_t>(bytes.size()));
        if (!buffer.empty()) std::memcpy(buffer.data(), bytes.ptr(), buffer.size());
        for (auto& font : hp2::frontend::parse_fonts(buffer)) {
            bool known = false;
            for (const auto& existing : fonts_) {
                if (existing.texture_hash == font.texture_hash) { known = true; break; }
            }
            if (!known) fonts_.push_back(std::move(font));
        }
    }
    return static_cast<int>(fonts_.size());
}

godot::Dictionary Ps2FrontEndLayout::get_font(std::int64_t texture_hash) const {
    godot::Dictionary result;
    const std::uint32_t hash = static_cast<std::uint32_t>(texture_hash);
    for (const auto& font : fonts_) {
        if (font.texture_hash != hash) continue;
        godot::Dictionary glyphs;
        for (const auto& g : font.glyphs) {
            godot::PackedFloat32Array metrics;
            metrics.push_back(g.u);
            metrics.push_back(g.v);
            metrics.push_back(g.w);
            metrics.push_back(g.h);
            metrics.push_back(g.advance);
            glyphs[static_cast<std::int64_t>(g.code)] = metrics;
        }
        result["line_height"] = static_cast<std::int64_t>(font.line_height);
        result["glyphs"] = glyphs;
        return result;
    }
    return result;
}

// ---------------------------------------------------------------------- recovered menu runtime

namespace {

godot::String describe_menu_message(int message) {
    // Only the numbers are recovered; the words are this port's reading of the call site.
    switch (message) {
        case hp2::frontend::kMenuMessage0x01: return "0x01 item activated";
        case hp2::frontend::kMenuMessage0x02: return "0x02 item refresh";
        case hp2::frontend::kMenuMessage0x04: return "0x04 highlight gained";
        case hp2::frontend::kMenuMessage0x05: return "0x05 highlight lost";
        case hp2::frontend::kMenuMessage0x06: return "0x06 adjust left";
        case hp2::frontend::kMenuMessage0x07: return "0x07 adjust right";
        case hp2::frontend::kMenuMessage0x08: return "0x08 list previous";
        case hp2::frontend::kMenuMessage0x09: return "0x09 list next";
        case hp2::frontend::kMenuMessage0x0A: return "0x0a MENU_SQUARE";
        case hp2::frontend::kMenuMessage0x0B: return "0x0b menu created";
        case hp2::frontend::kMenuMessage0x0D: return "0x0d START";
        case hp2::frontend::kMenuMessage0x0E: return "0x0e SELECT";
        case hp2::frontend::kMenuMessage0x0F: return "0x0f MENU_L1";
        case hp2::frontend::kMenuMessage0x10: return "0x10 MENU_R1";
        default: return godot::String("0x") + godot::String::num_int64(message, 16);
    }
}

} // namespace

godot::Dictionary Ps2FrontEndLayout::menu_open(int screen_index) {
    godot::Dictionary result;
    menu_group_.reset();
    menu_screen_index_ = -1;
    message_log_.clear();
    activated_name_hash_ = 0;
    scrolling_text_.reset();
    menu_viewport_.reset();
    menu_selector_.reset();
    exit_animation_.reset();
    exit_class_name_.clear();

    if (screen_index < 0 || screen_index >= get_screen_count()) {
        result["ok"] = false;
        result["error"] = "screen index out of range";
        return result;
    }

    screens_[static_cast<std::size_t>(screen_index)] = authored_screens_[static_cast<std::size_t>(screen_index)];
    menu_screen_index_ = screen_index;
    const auto* registration = port::frontend::find_registration_for_document_hash(
        screens_[static_cast<std::size_t>(screen_index)].name_hash_0x08);
    active_class_name_ = requested_class_for_open_.empty()
        ? (registration == nullptr ? "" : std::string(registration->class_name)) : requested_class_for_open_;
    requested_class_for_open_.clear();
    menu_group_ = std::make_unique<hp2::frontend::MenuGroup_0016AA00>();
    menu_group_->screen_0x14 = &screens_[static_cast<std::size_t>(screen_index)];
    menu_group_->pad_index_0x10 = -1;   // FUN_0016AA00's "any pad" value
    menu_group_->initial_index_0x0C = 0; // list position, not a MENUITEM_%d suffix
    menu_group_->callback_0x18 = [this](int message, hp2::frontend::MenuEvent_0016A458& event) {
        if (menu_viewport_ && menu_selector_) {
            const hp2::frontend::WidgetMeasure measure = [this](const auto& w) { return measure_widget(w); };
            if (active_class_name_ == "Audio Options")
                hp2::frontend::Sub_001EFB80(message,event,*menu_viewport_,*menu_selector_,*menu_group_->screen_0x14,measure);
            else hp2::frontend::Sub_001F3A28(message,event,*menu_viewport_,*menu_selector_,*menu_group_->screen_0x14,measure);
        }
        if (message == hp2::frontend::kMenuMessage0x04) {
            bind_menu_animation(event.field_0x00);
            bind_scrolling_text(event.field_0x00);
        }
        if (message == hp2::frontend::kMenuMessage0x01) {
            activated_name_hash_ = event.field_0x00;
            dispatch_activation(event);
        }
        if (message_log_.size() >= 64) message_log_.erase(message_log_.begin());
        message_log_.push_back(describe_menu_message(message));
    };

    hp2::frontend::install_menu_sound_sink([this](const hp2::frontend::MenuSoundRequest& request) {
        if (request.name == nullptr) return;
        // E-0016-A01: suppress repeats of the same object within 34 milliseconds.
        if (last_sound_name_ == request.name && sound_clock_seconds_ - last_sound_seconds_ < 0.034)
            return;
        last_sound_name_ = request.name;
        last_sound_seconds_ = sound_clock_seconds_;
        pending_sound_names_.push_back(request.name);
    });

    try {
        open_menu_animation(screen_index);
        background_animation_.Bind_00249A40(&screens_[static_cast<std::size_t>(screen_index)],
            [this](int bound) { return hp2::frontend::Sub_0025A3D0(bound, random_state_); },
            [this](const auto& widget) { return measure_widget(widget); });
        const auto hint = port::frontend::hint_binding(active_class_name_, 0);
        if (hint.attachment_known && hint.attached) {
            scrolling_text_ = std::make_unique<hp2::frontend::Type_002491B0>();
            scrolling_text_->Construct_002491B0(screens_[static_cast<std::size_t>(screen_index)]);
        }
        if (active_class_name_ == "Audio Options" || active_class_name_ == "Display Options") {
            menu_viewport_ = std::make_unique<hp2::frontend::Type_00249DE0>();
            if (active_class_name_ == "Audio Options") menu_viewport_->ConstructAudio_001EF3C8(*menu_group_->screen_0x14);
            else menu_viewport_->ConstructDisplay_001F33B8(*menu_group_->screen_0x14);
            menu_selector_ = std::make_unique<hp2::frontend::Type_0024A690>();
            menu_selector_->Construct_0024A690(*menu_group_->screen_0x14,0xD1CDA594,0x9345E4A7);
        }
        menu_group_->EnsureMenu_0016A858();
    } catch (const hp2::NotImplemented& error) {
        result["ok"] = false;
        result["not_implemented"] = godot::String::utf8(error.what());
        return result;
    }

    menu_screen_index_ = screen_index;
    result = menu_state();
    result["ok"] = true;
    result["error"] = "";
    return result;
}

void Ps2FrontEndLayout::dispatch_activation(hp2::frontend::MenuEvent_0016A458& event) {
    const std::uint32_t name_hash = event.field_0x00;
    port::frontend::ScreenActionServices services{
        [this](const char* name, bool push) {
            pending_class_name_ = name;
            pending_push_history_ = push;
        },
        [this](const char* name) {
            exit_animation_ = std::make_unique<hp2::frontend::Type_002490D8>();
            exit_animation_->Construct_002490D8(screens_[static_cast<std::size_t>(menu_screen_index_)]);
            exit_class_name_ = name;
        },
        [] { HP2_NOT_IMPLEMENTED("0x001EE6C0", "display preparation not translated"); }};
    if (menu_screen_index_ == 0) {
        hp2::frontend::Sub_00216638(1, name_hash, dat_002FC070_, services);
    } else if (menu_screen_index_ == 24) {
        hp2::frontend::Sub_001EECA8(1, name_hash, services);
    } else if (menu_screen_index_ == 1) {
        // "Cops On Mode Select" (FUN_002174A0). The event is passed through because the original
        // gates Quick Race and Tournament on its +0x04 pad field.
        hp2::frontend::Sub_002174A0(1, name_hash, event, mode_select_globals_, services);
    } else if (menu_screen_index_ == 2) {
        hp2::frontend::Sub_00217AF8(1, name_hash, event, mode_select_globals_, services); // 0x00217AF8
    } else {
        const auto* registration = port::frontend::find_registration(active_class_name_);
        const std::string address = registration == nullptr ? "unknown screen handler" :
            "0x" + std::string(godot::String::num_int64(registration->handler_address, 16).utf8().get_data());
        HP2_NOT_IMPLEMENTED(address.c_str(), "screen item activation handler not translated");
    }
}

void Ps2FrontEndLayout::apply_pending_screen() {
    if (pending_class_name_.empty()) return;
    const std::string name = pending_class_name_;
    pending_class_name_.clear();
    const auto* registration = port::frontend::find_registration(name);
    if (registration == nullptr) HP2_NOT_IMPLEMENTED("0x0016C8C8", "requested class not recovered");
    const int target = hp2::frontend::screen_index_for_name_hash(screens_, registration->document_hash);
    if (target < 0) HP2_NOT_IMPLEMENTED("0x0016ADB0", "registered document not present in loaded bundle");
    if (pending_push_history_) {
        if (navigation_history_.size() >= 12)
            HP2_NOT_IMPLEMENTED("0x0016B418", "history overflow behavior not recovered");
        navigation_history_.push_back(active_class_name_);
    }
    const auto activated = activated_name_hash_;
    requested_class_for_open_ = name;
    const auto opened = menu_open(target);
    active_class_name_ = name; // preserve many-to-one class identity (e.g. NameEntry)
    activated_name_hash_ = activated;
    if (!static_cast<bool>(opened.get("ok", false)))
        HP2_NOT_IMPLEMENTED("0x0016C4C0", "target menu construction failed");
}

void Ps2FrontEndLayout::request_history_back() {
    // 0016C670/0016B438: empty history leaves the current screen alone.
    if (navigation_history_.empty()) return;
    pending_class_name_ = navigation_history_.back();
    navigation_history_.pop_back();
    pending_push_history_ = false;
}

void Ps2FrontEndLayout::apply_language(int language) {
    if (language == port_language_) return; // 0021E0D0 returns immediately for unchanged locale.
    // Port asset I/O seam for 0021E0D0; enum order matches original locale table.
    static constexpr const char* names[]{"ENGLISH","FRENCH","GERMAN","ITALIAN","SPANISH","SWEDISH"};
    if (language < 0 || language >= 6) HP2_NOT_IMPLEMENTED("0x0021E0D0", "unknown locale");
    const auto root = hp2::platform::resolve_zzdata_root(search_start_dir());
    if (!root) HP2_NOT_IMPLEMENTED("0x0021E0D0", "asset root unavailable");
    std::ifstream file(*root + "/LANGUAGES/FE_" + names[language] + ".BIN",std::ios::binary);
    if (!file) HP2_NOT_IMPLEMENTED("0x0021E0D0", "locale asset unavailable");
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
    language_ = std::make_unique<hp2::port::assets::FeLanguage>(bytes);
    for (auto* collection : {&screens_,&authored_screens_}) for (auto& screen : *collection)
        for (auto& widget : screen.widgets) if (widget.string_id_0x60 &&
            !language_->lookup(widget.string_id_0x60,widget.text)) widget.text="NOT TRANSLATED PROPERLY";
    port_language_ = language;
}

godot::Dictionary Ps2FrontEndLayout::menu_joy_event(int pad_index, int joy_event, bool pressed) {
    godot::Dictionary result;
    if (!menu_group_) {
        result["ok"] = false;
        result["error"] = "no menu open";
        return result;
    }
    godot::String not_implemented;
    try {
        // FUN_001EEF00 forwards to 00249128 during exit, which consumes further menu input.
        if (!exit_animation_ && joy_event == hp2::input::JOY_EVENT_MENU_BACK && pressed) {
            if (active_class_name_ == "Options") {
                hp2::frontend::PlayMenuSound_0016E038(6);
                request_history_back();
            } else if (active_class_name_ != "New Main Menu") {
                const auto* registration = port::frontend::find_registration(active_class_name_);
                if (!registration) HP2_NOT_IMPLEMENTED("0x0016C670", "Back class registration missing");
                port::frontend::BackContext context;
                // No settings/editor mutation handlers are enabled in this bridge. The
                // entry snapshots therefore remain identical, not an unknown comparison.
                context.settings_changed = false;
                context.keyboard_active = false; // 0024D408 initializes editor state to zero.
                context.language = port_language_;
                context.language_instance_0x08 = active_class_name_ == "Language Options" ? 1 : 0;
                if (auto* item = menu_group_->child_0x08 ? menu_group_->child_0x08->current_0x08 : nullptr) context.selected_hash = item->name_hash_0x0C;
                port::frontend::BackServices services{
                    [this](std::uint32_t address,std::uint32_t arg) {
                        if (address == 0x0021E0D0) apply_language(static_cast<int>(arg));
                        else if (address == 0x00170320 && arg == 1) {
                            request_history_back();
                            apply_pending_screen(); // Language handler pops before outer sound playback.
                        }
                        else HP2_NOT_IMPLEMENTED("0x0016C670", "Back side-effect dependency not translated");
                    },
                    [this](const char* name) {
                        if (navigation_history_.size() >= 12) HP2_NOT_IMPLEMENTED("0x0016B418", "history overflow");
                        navigation_history_.emplace_back(name);
                    }};
                const auto event = port::frontend::query_back(registration->handler_address,context,services);
                hp2::frontend::PlayMenuSound_0016E038(event.sound);
                if (event.permission == 1) request_history_back();
            } // 00216664 blocks main Back and clears sound.
        } else if (!exit_animation_) {
            menu_group_->HandleJoyEvent_0016AA00(pad_index, joy_event, pressed);
        }
        apply_pending_screen();
    } catch (const hp2::NotImplemented& error) {
        not_implemented = godot::String::utf8(error.what());
    }
    result = menu_state();
    result["ok"] = true;
    if (!not_implemented.is_empty()) result["not_implemented"] = not_implemented;
    return result;
}

godot::Dictionary Ps2FrontEndLayout::menu_state() const {
    godot::Dictionary state;
    state["screen"] = menu_screen_index_;
    state["dat_002FC070"] = dat_002FC070_;
    state["active_class"] = godot::String::utf8(active_class_name_.c_str());
    state["history_depth"] = static_cast<int>(navigation_history_.size());
    state["exiting"] = exit_animation_ != nullptr;
    if (!menu_group_ || menu_group_->child_0x08 == nullptr) {
        state["item_count"] = 0;
        state["current"] = -1;
        return state;
    }
    auto* list = menu_group_->child_0x08;
    state["item_count"] = static_cast<int>(list->count());
    state["current"] = static_cast<int>(list->CurrentIndex_0016A130());

    godot::Array items;
    const hp2::frontend::MenuThing_00169740* current = list->current_0x08;
    for (hp2::frontend::MenuThing_00169740* item : list->items()) {
        godot::Dictionary entry;
        entry["name_hash"] = static_cast<std::int64_t>(item->name_hash_0x0C);
        entry["menuitem_index"] = static_cast<int>(item->index_0x10);
        entry["state"] = static_cast<std::int64_t>(item->state_0x08);
        entry["enabled"] = item->enabled();
        entry["highlighted"] = item->highlighted();
        entry["current"] = item == current;
        items.append(entry);
    }
    state["items"] = items;
    state["current_name_hash"] = current == nullptr
                                     ? 0
                                     : static_cast<std::int64_t>(current->name_hash_0x0C);
    state["current_name"] = current == nullptr
                                ? godot::String()
                                : godot::String("MENUITEM_") +
                                      godot::String::num_int64(current->index_0x10);
    state["activated_name_hash"] = static_cast<std::int64_t>(activated_name_hash_);

    const auto& sound = hp2::frontend::last_menu_sound_request();
    state["sound_id"] = sound.sound_id;
    state["sound_name"] = sound.name == nullptr ? godot::String()
                                                : godot::String::utf8(sound.name);
    godot::PackedStringArray messages;
    for (const auto& line : message_log_) messages.push_back(line);
    state["messages"] = messages;
    return state;
}

godot::PackedColorArray Ps2FrontEndLayout::get_widget_colours(int index) const {
    godot::PackedColorArray colours;
    if (index < 0 || index >= get_screen_count()) return colours;
    for (const auto& w : screens_[static_cast<std::size_t>(index)].widgets) {
        colours.push_back(godot::Color(static_cast<float>(w.r) / 128.0f,
                                       static_cast<float>(w.g) / 128.0f,
                                       static_cast<float>(w.b) / 128.0f,
                                       static_cast<float>(w.a) / 128.0f));
    }
    return colours;
}

void Ps2FrontEndLayout::set_widget_measurements(int index, const godot::PackedVector2Array& dimensions) {
    if (index < 0 || index >= get_screen_count()) return;
    widget_base_metrics_.resize(screens_.size());
    auto& metrics = widget_base_metrics_[static_cast<std::size_t>(index)];
    const auto& widgets = screens_[static_cast<std::size_t>(index)].widgets;
    metrics.resize(widgets.size());
    for (std::size_t i = 0; i < widgets.size() && i < static_cast<std::size_t>(dimensions.size()); ++i) {
        metrics[i] = {widgets[i].scale_x == 0 ? 0 : dimensions[i].x / widgets[i].scale_x,
                      widgets[i].scale_y == 0 ? 0 : dimensions[i].y / widgets[i].scale_y};
    }
    if (index == menu_screen_index_ && menu_viewport_)
        hp2::frontend::PlaceSelectionBars_001F4E68(screens_[index],menu_viewport_->Sub_0024A368(),
            [this](const auto& widget) { return measure_widget(widget); });
}

hp2::frontend::WidgetMetrics Ps2FrontEndLayout::measure_widget(const hp2::frontend::FeWidget& widget) const {
    const auto* screen = menu_group_ ? menu_group_->screen_0x14 : highlight_animation_.screen_0x00;
    if (screen == nullptr) return {};
    const auto& widgets = screen->widgets;
    // The ghost acquires its target's font, string and transform on every tick; use the target's
    // measured base dimensions rather than the template's authored placeholder text.
    const auto* measured = &widget;
    if (highlight_animation_.screen_0x00 != nullptr && widget.name_hash == highlight_animation_.template_hash_0x0C) {
        measured = hp2::frontend::FindWidget_00143D50(*highlight_animation_.screen_0x00,
                                                    highlight_animation_.target_hash_0x04, 1);
        if (measured == nullptr) return {};
    }
    const auto screen_index = static_cast<std::size_t>(screen - screens_.data());
    const auto slot = static_cast<std::size_t>(measured - widgets.data());
    if (screen_index >= widget_base_metrics_.size() || slot >= widget_base_metrics_[screen_index].size()) return {};
    const auto base = widget_base_metrics_[screen_index][slot];
    return {base.width * widget.scale_x, base.height * widget.scale_y};
}

void Ps2FrontEndLayout::open_menu_animation(int screen_index) {
    highlight_animation_ = {};
    menu_clock_seconds_ = 0;
    // E-0016-V01: Title also binds ANIMATING_MENUITEM/PRESS_START_1, retaining default 4s period.
    // Screens 1 and 2 bind it the same way: FUN_00216E80 and FUN_00217538 each call
    // FUN_00248870(instance+0x23C, document, ANIMATING_MENUITEM, gp-0x3F08). (E-0016-08)
    if (screen_index != 0 && screen_index != 1 && screen_index != 2 && screen_index != 22) return;
    highlight_animation_.Construct_00248870(&screens_[static_cast<std::size_t>(screen_index)],
        hp2::frontend::StringHash_002534A8("ANIMATING_MENUITEM"),
        hp2::frontend::kAnimGhostColour_gp_0x3F08, 0);
    // Correction (E-0016-08): the 2 s period belongs to the manager-level object FUN_0016FBB0
    // builds at manager+0x1B8, which binds GLOBAL_ANIMATING_MENUITEM against the LOADING_TRACK
    // document. Every screen-owned instance -- FUN_00216460 at +0x1A8 for New Main Menu and
    // FUN_00216E80/FUN_00217538 at +0x23C for the two mode selects -- goes through FUN_00248870
    // without overriding +0x1C, so it keeps the default 4 s.
    if (screen_index == 22) highlight_animation_.Bind_002488D0(0xB070DBC9, 0);
}

void Ps2FrontEndLayout::bind_menu_animation(std::uint32_t name_hash) {
    if (highlight_animation_.screen_0x00 == nullptr) return;
    highlight_animation_.Bind_002488D0(name_hash, static_cast<float>(menu_clock_seconds_));
    char mask[32];
    std::snprintf(mask, sizeof(mask), hp2::frontend::kMaskNameFormat_002BEFA0,
                  hp2::frontend::MenuItemIndexFromNameHash_0016A330(name_hash));
    hp2::frontend::PlaceSelectionBars_00216880(*highlight_animation_.screen_0x00,
        hp2::frontend::StringHash_002534A8(mask), [this](const auto& w) { return measure_widget(w); });
}

godot::Dictionary Ps2FrontEndLayout::menu_tick(double delta) {
    godot::Dictionary result;
    if (!std::isfinite(delta) || delta < 0) {
        result["ok"] = false;
        result["error"] = "menu delta must be finite and nonnegative";
        return result;
    }
    menu_clock_seconds_ += delta;
    sound_clock_seconds_ += delta;
    if (scrolling_text_) scrolling_text_->Update_00249348(static_cast<float>(delta));
    if (menu_viewport_) menu_viewport_->Update_0024A1A8(static_cast<float>(delta));
    if (menu_selector_) menu_selector_->Update_0024A730(static_cast<float>(delta),
        [this](const auto& w) { return measure_widget(w); });
    // 0016CCA8 gates this update on GAMEFLOW_STATE_LOADING_TRACK_COMMON (index 3).
    // This standalone host is in front-end state (index 2), so blueprint state is frozen.
    if (exit_animation_ && exit_animation_->Update_00249128(static_cast<float>(delta))) {
        exit_animation_.reset();
        pending_class_name_ = exit_class_name_;
        pending_push_history_ = true;
        exit_class_name_.clear();
        try { apply_pending_screen(); }
        catch (const hp2::NotImplemented& error) {
            result["not_implemented"] = godot::String::utf8(error.what());
        }
    }
    if (highlight_animation_.active() && (menu_screen_index_ != 22 || !title_movie_state_)) {
        highlight_animation_.Update_00248970(static_cast<float>(menu_clock_seconds_),
                                            [this](const auto& w) { return measure_widget(w); });
    }
    result["ok"] = true;
    result["screen"] = menu_screen_index_;
    return result;
}

godot::Array Ps2FrontEndLayout::get_widget_dynamic(int index) const {
    // Include text/font/layer/visibility: FUN_001410A0 changes all of these as well as x/y/scale.
    return get_widgets(index);
}

void Ps2FrontEndLayout::bind_scrolling_text(std::uint32_t name_hash) {
    if (!scrolling_text_ || !language_) return;
    const auto binding = port::frontend::hint_binding(active_class_name_, name_hash);
    if (!binding.attachment_known)
        HP2_NOT_IMPLEMENTED("0x002491B0", "screen scrolling-text attachment not recovered");
    std::string text;
    if (!language_->lookup(binding.string_id, text)) text = "NOT TRANSLATED PROPERLY";
    scrolling_text_->Bind_00249518(text);
}

godot::Array Ps2FrontEndLayout::drain_menu_audio() {
    godot::Array result;
    auto requests = std::move(pending_sound_names_);
    pending_sound_names_.clear();
    for (const auto& name : requests) {
        godot::Dictionary request;
        request["event"] = godot::String::utf8(name.c_str());
        try {
            if (!sound_bank_) throw std::runtime_error("original sound bank not loaded");
            const auto pcm = sound_bank_->decode(name);
            request["stream"] = pcm_stream(pcm);
            request["pitch_scale"] = hp2::port::audio::neutral_playback_pitch_scale(pcm.sample_rate);
            request["gain"] = pcm.nominal_gain * 0.31f; // E-0016-A04, category-3 init 001AFAB8.
        } catch (const std::exception& error) {
            request["not_implemented"] = godot::String::utf8(error.what());
        }
        result.append(request);
    }
    return result;
}

godot::Dictionary Ps2FrontEndLayout::frontend_music(bool previous_finished) {
    godot::Dictionary result;
    try {
        if (!jukebox_ || !music_scheduler_) throw std::runtime_error("original jukebox not loaded");
        const auto ticks = static_cast<std::uint32_t>(std::fmod(sound_clock_seconds_ * 4000, 4294967296.0));
        const auto index = previous_finished ? music_scheduler_->finished(ticks) : music_scheduler_->enter(ticks);
        if (index) {
            const auto pcm = jukebox_->decode(*index);
            result["stream"] = pcm_stream(pcm); result["gain"] = pcm.nominal_gain;
            result["pitch_scale"] = hp2::port::audio::neutral_playback_pitch_scale(pcm.sample_rate);
            result["index"] = static_cast<int>(*index);
            result["name"] = godot::String(jukebox_->tunes()[*index].name.c_str());
        }
        result["ok"] = true;
    } catch (const std::exception& error) {
        result["ok"] = false; result["not_implemented"] = godot::String::utf8(error.what());
    }
    return result;
}

void Ps2FrontEndLayout::title_visual_state(bool movie_state, bool can_start) {
    title_movie_state_ = movie_state;
    if (screens_.size() <= 22) return;
    // E-0016-V01: 0016F1F0 -> 001454E8 toggles TITLE_IMAGE class members; 00144968
    // controls PRESS_START_1 with the strict 0016F4F8 time gate. Movie widget slots are stopped.
    for (auto& widget : screens_[22].widgets) {
        if (widget.class_hash == 0xB73C8D23) widget.visible = movie_state ? 0 : 1;
        if (widget.name_hash == hp2::frontend::StringHash_002534A8("PRESS_START_1"))
            widget.visible = !movie_state && can_start ? 1 : 0;
        if (!movie_state && (widget.name_hash == 0x3ACF2D73 || widget.name_hash == 0xBFAEBF79))
            widget.visible = 0;
    }
}

void Ps2FrontEndLayout::title_pulse_restart() {
    // 00248AD0 on skipping the second intro: resets pulse timestamp, preserves target.
    highlight_animation_.start_time_0x08 = static_cast<float>(menu_clock_seconds_);
}

godot::String Ps2FrontEndLayout::joy_event_name(int joy_event) const {
    const char* name = hp2::input::joy_event_name(joy_event);
    return name == nullptr ? godot::String() : godot::String::utf8(name);
}

godot::Dictionary Ps2FrontEndLayout::probe_not_implemented(const godot::String& which) {
    godot::Dictionary result;
    try {
        if (which == "popup") {
            hp2::frontend::Sub_0016BAC0(0, hp2::input::JOY_EVENT_MENU_SELECT, true);
        } else if (which == "highlight_animation") {
            hp2::frontend::Sub_0016FBB0();
        } else if (which == "localised_text") {
            hp2::frontend::localised_text(0);
        } else {
            result["error"] = godot::String("unknown probe: ") + which;
            return result;
        }
    } catch (const hp2::NotImplemented& error) {
        result["not_implemented"] = godot::String::utf8(error.what());
        return result;
    }
    result["error"] = "probe did not raise hp2::NotImplemented";
    return result;
}

int Ps2FrontEndLayout::find_screen_by_name(const godot::String& screen_name) const {
    return hp2::frontend::screen_index_for_screen_name(screens_,
                                                       screen_name.utf8().get_data());
}

std::int64_t Ps2FrontEndLayout::name_hash(const godot::String& name) const {
    return static_cast<std::int64_t>(
        hp2::frontend::StringHash_002534A8(name.utf8().get_data()));
}

} // namespace port::godot_bridge
