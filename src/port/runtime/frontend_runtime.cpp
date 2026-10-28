#include "port/runtime/frontend_runtime.hpp"
#include "game_core/port/frontend/frontendscreens/registry_0016adb0.hpp"
#include "game_core/port/input/joyevents.hpp"
#include "game_core/port/not_implemented.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cmath>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace port::runtime {
namespace {

std::string hex(std::uint32_t value) {
    std::ostringstream text;
    text << "0x" << std::hex << value;
    return text.str();
}
float audio_value(const std::array<std::uint8_t,0xb0>& options,std::size_t index) {
    std::uint32_t bits=0;
    for(unsigned b=0;b<4;++b)bits|=std::uint32_t(options.at(index*4+b))<<(b*8);
    float value;std::memcpy(&value,&bits,sizeof(value));
    if(!std::isfinite(value) || value<0)throw std::runtime_error("invalid saved audio category volume");
    return value;
}

std::string describe_menu_message(int message) {
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
        default: return hex(static_cast<std::uint32_t>(message));
    }
}

} // namespace

FrontEndRuntime::~FrontEndRuntime() {
    selection_.reset();
    hp2::frontend::install_menu_sound_sink({}); // Global host callback must not outlive this owner.
}

void FrontEndRuntime::reset_state() {
    selection_.reset();pending_sound_controls_.clear();
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
    music_scheduler_.reset();
    jukebox_.reset();
    jukebox_screen_.reset();
    autosave_.reset();save_popup_.reset();
    audio_screen_.reset();pending_volume_feedback_.clear();
    display_screen_.reset();hud_screen_.reset();calibration_.reset();name_entry_.reset();
    display_baseline_valid_=false;hud_arrow_target_=0;hud_arrow_phase_=0;
    selection_animation_={};
    audio_options_={};
    pending_music_index_=-1;pending_music_stop_=false;music_auto_mode_=true;music_playing_=false;
    pending_sound_names_.clear();
    last_sound_name_.clear();
    sound_clock_seconds_ = 0;
    last_sound_seconds_ = -1;
    background_animation_.Construct_002495F0();
    highlight_animation_ = {};
    pending_class_name_.clear();
    active_class_name_.clear();
    exit_animation_.reset();
    authored_screens_.clear();
    catalog_.reset();
    asset_source_.reset();
    startup_movie_visible_ = startup_letterbox_ = startup_active_ = false;
    event_tree_.clear();
    event_text_cursor_ = -1;
    event_tree_error_.clear();
    event_tree_bundle_.clear();
    profile_ = {};
    profile_load_ = {};
}

LoadResult FrontEndRuntime::load(const std::string& asset_search_start_dir,
                                 const std::string& relative_path) {
    LoadResult result;
    reset_state();
    profile_search_start_=asset_search_start_dir;

    auto root = hp2::platform::resolve_zzdata_root(asset_search_start_dir);
    if (!root) {
        last_error_ = "PS2 asset root not configured (set HP2_PS2_ASSET_ROOT or create "
                      "config/ps2_assets.local.json).";
        result.error = last_error_;
        return result;
    }
    // One Godot-free source feeds both the renderer's textures and widget measurement, so both
    // read the original bundles from the same place for every backend.
    asset_source_ = std::make_unique<assets::DirectorySource>(*root);
    catalog_ = std::make_unique<assets::Catalog>(*asset_source_);

    event_tree_bundle_ = relative_path;
    // TASK-0021: the optional player profile. Nothing configured leaves it all zero, which is
    // what the original hands its own driver before a load, so the front end behaves exactly as
    // it did before the save system existed. hp2::platform owns every path decision.
    profile_ = {};
    profile_load_ = hp2::platform::load_profile(asset_search_start_dir,
                                                hp2::memorycard::kRegionNtsc_001A3390, profile_);
    std::vector<std::uint8_t> buffer;
    try {
        buffer = asset_source_->read(relative_path);
    } catch (const std::exception& error) {
        last_error_ = error.what();
        result.error = last_error_;
        return result;
    }

    screens_ = hp2::frontend::parse_screens(buffer);
    try {
        // English is the port's explicit initial language choice, not guessed text.
        language_ = std::make_unique<hp2::port::assets::FeLanguage>(
            asset_source_->read("LANGUAGES/FE_ENGLISH.BIN"));
        event_names_.load(asset_source_->read("GLOBAL/GLOBALB.BUN"));
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
        initialize_audio_options();
        if(!profile_load_.loaded) {
            const std::array<std::uint8_t,8> defaults{1,0,0,7,8,2,1,1}; //001EE0A0,English,non-debug
            std::copy(defaults.begin(),defaults.end(),profile_.bytes()+0x14c);
            for(int player=0;player<2;++player) {
                const auto name=event_localize(0x27165+player);
                std::copy_n(name.data(),std::min<std::size_t>(11,name.size()),profile_.bytes()+0x10d90+player*0x6d0);
            }
        }
    } catch (const std::exception& error) {
        last_error_ = error.what();
        result.error = last_error_;
        return result;
    }
    authored_screens_ = screens_;
    if (screens_.empty()) {
        last_error_ = relative_path + " contains no frontend screen list chunk.";
        result.error = last_error_;
        return result;
    }
    last_error_.clear();
    result.ok = true;
    result.screen_count = screen_count();
    return result;
}

int FrontEndRuntime::load_fonts(const std::vector<std::string>& bundles) {
    fonts_.clear();
    if (!asset_source_) {
        last_error_ = "PS2 asset root not configured.";
        return 0;
    }
    for (const auto& bundle : bundles) {
        std::vector<std::uint8_t> buffer;
        try {
            buffer = asset_source_->read(bundle);
        } catch (const std::exception&) {
            continue;
        }
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

const hp2::frontend::FeScreen* FrontEndRuntime::screen(int index) const {
    if (index < 0 || index >= screen_count()) return nullptr;
    return &screens_[static_cast<std::size_t>(index)];
}

const hp2::frontend::FeFont* FrontEndRuntime::font(std::uint32_t texture_hash) const {
    for (const auto& entry : fonts_) {
        if (entry.texture_hash == texture_hash) return &entry;
    }
    return nullptr;
}

ScreenInfo FrontEndRuntime::screen_info(int index) const {
    ScreenInfo info;
    const auto* found = screen(index);
    if (found == nullptr) return info;
    info.index = found->index;
    info.width = found->width;
    info.height = found->height;
    info.widget_count = static_cast<int>(found->widgets.size());
    info.name_hash = found->name_hash_0x08;
    // The screen's own recovered name, where the hash at 0x10203 +0x08 has been matched.
    std::size_t recovered_count = 0;
    const auto* recovered = hp2::frontend::recovered_screen_names(recovered_count);
    for (std::size_t i = 0; i < recovered_count; ++i) {
        if (recovered[i].screen_index == found->index) { info.screen_name = recovered[i].name; break; }
    }
    for (const auto& widget : found->widgets) {
        if (!widget.name.empty()) info.widget_names.push_back(widget.name);
    }
    return info;
}


int FrontEndRuntime::find_screen_by_name(const std::string& screen_name) const {
    return hp2::frontend::screen_index_for_screen_name(screens_, screen_name.c_str());
}

render::Rect FrontEndRuntime::widget_clip(int screen_index, std::size_t slot) const {
    if (screen_index < 0 || screen_index >= screen_count()) return {};
    auto& screen = const_cast<hp2::frontend::FeScreen&>(screens_[static_cast<std::size_t>(screen_index)]);
    if (slot >= screen.widgets.size()) return {};
    const auto clip = hp2::frontend::GetWidgetClip_00145F30(
        screen, screen.widgets[slot], [this](const auto& widget) { return measure_widget(widget); });
    if (!clip.valid) return {};
    return {clip.x0, clip.y0, clip.x1 - clip.x0, clip.y1 - clip.y0};
}

// ---------------------------------------------------------------------- recovered menu runtime

MenuState FrontEndRuntime::menu_open(int screen_index) {
    MenuState result;
    commit_hud(); //001F4178 destruction commit, including Back/manager replacement.
    hud_screen_.reset();display_screen_.reset();calibration_.reset();name_entry_.reset();
    menu_group_.reset();
    menu_screen_index_ = -1;
    message_log_.clear();
    activated_name_hash_ = 0;
    scrolling_text_.reset();
    menu_viewport_.reset();
    menu_selector_.reset();
    jukebox_screen_.reset();
    autosave_.reset();save_popup_.reset();
    exit_animation_.reset();
    exit_class_name_.clear();
    audio_screen_.reset();

    if (screen_index < 0 || screen_index >= screen_count()) {
        result.error = "screen index out of range";
        return result;
    }

    const auto slot = static_cast<std::size_t>(screen_index);
    screens_[slot] = authored_screens_[slot];
    menu_screen_index_ = screen_index;
    const auto* registration =
        port::frontend::find_registration_for_document_hash(screens_[slot].name_hash_0x08);
    active_class_name_ = requested_class_for_open_.empty()
        ? (registration == nullptr ? "" : std::string(registration->class_name))
        : requested_class_for_open_;
    requested_class_for_open_.clear();
    menu_group_ = std::make_unique<hp2::frontend::MenuGroup_0016AA00>();
    menu_group_->screen_0x14 = &screens_[slot];
    menu_group_->pad_index_0x10 = -1;    // FUN_0016AA00's "any pad" value
    menu_group_->initial_index_0x0C = 0; // list position, not a MENUITEM_%d suffix
    menu_group_->callback_0x18 = [this](int message, hp2::frontend::MenuEvent_0016A458& event) {
        if(name_entry_) {dispatch_name_entry(message,event);return;}
        if(jukebox_screen_) {
            dispatch_jukebox(message,event.sound_id_0x0C);
            if(message_log_.size()>=64)message_log_.erase(message_log_.begin());
            message_log_.push_back(describe_menu_message(message));
            return;
        }
        if(hud_screen_) {
            dispatch_hud(message,event);
            if(message_log_.size()>=64)message_log_.erase(message_log_.begin());
            message_log_.push_back(describe_menu_message(message));return;
        }
        if(calibration_) {
            if(hp2::frontend::Sub_001F4B20(*calibration_,message,event.sound_id_0x0C))request_history_back();
            profile_.write_u32(0x144,static_cast<std::uint32_t>(calibration_->x));
            profile_.write_u32(0x148,static_cast<std::uint32_t>(calibration_->y));
            return;
        }
        if (menu_viewport_ && menu_selector_) {
            const hp2::frontend::WidgetMeasure measure = [this](const auto& w) { return measure_widget(w); };
            if (active_class_name_ == "Audio Options")
                hp2::frontend::Sub_001EFB80(message, event, *menu_viewport_, *menu_selector_,
                                            *menu_group_->screen_0x14, measure);
            else
                hp2::frontend::Sub_001F3A28(message, event, *menu_viewport_, *menu_selector_,
                                            *menu_group_->screen_0x14, measure);
        }
        if(audio_screen_ && message!=1)dispatch_audio(message,event);
        if(display_screen_ && message!=1)dispatch_display(message,event);
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

    result.not_implemented = hp2::guard([&] {
        if(active_class_name_=="NameEntry") {
            name_entry_=std::make_unique<port::frontend::NameEntryState>();
            for(int p=0;p<2;++p) {
                std::copy_n(profile_.bytes()+0x10d90+p*0x6d0,12,name_entry_->names[p].begin());
                name_entry_->names[p][11]=0;
                name_entry_->flags[p]=profile_.read_u8(0x10d83+p*0x6d0);
            }
            name_entry_->initial_names=name_entry_->names;name_entry_->initial_flags=name_entry_->flags;
            const auto* panel=hp2::frontend::FindWidget_00143D50(screens_[slot],0x35b2f256,0);
            if(!panel)hp2::not_implemented("0x0024C458","NameEntry keyboard panel missing");
            name_entry_->panel_authored_x=panel->x;name_entry_->panel_authored_y=panel->y;
            hp2::frontend::Sub_0024D408(*name_entry_,screens_[slot],[this](auto id){return event_localize(id);});
            menu_selector_=std::make_unique<hp2::frontend::tLeftRightArrowScroll>();
            menu_selector_->Construct_0024A690(screens_[slot],0xd1cda594,0x9345e4a7);
        }
        if(active_class_name_=="JukeBox Music")open_jukebox();
        if(active_class_name_=="Audio Options") {
            audio_screen_=std::make_unique<port::frontend::AudioOptionsState>();
            hp2::frontend::Sub_001EF3C8(*audio_screen_,audio_options_);
        }
        if(active_class_name_=="Display Options") {
            if(!display_baseline_valid_)capture_display_baseline();
            display_screen_=std::make_unique<port::frontend::DisplayOptionsState>();
            std::copy_n(profile_.bytes()+0x14c,8,display_screen_->snapshot.begin());
            hp2::frontend::Sub_001F3A28(*display_screen_,screens_[slot],[this](auto id){return event_localize(id);});
        }
        if(active_class_name_=="Customize P1 HUD" || active_class_name_=="Customize P2 HUD") {
            hud_screen_=std::make_unique<port::frontend::HudOptionsState>();
            hud_screen_->player=active_class_name_=="Customize P2 HUD" ? 1 : 0;
            std::copy_n(profile_.bytes()+0x10d7c+hud_screen_->player*0x6d0,0x30,hud_screen_->snapshot.begin());
            hp2::frontend::Sub_001F46D8(*hud_screen_,screens_[slot],[this](auto id){return event_localize(id);});
            scrolling_text_=std::make_unique<hp2::frontend::tAnimatedBlurb>();
            scrolling_text_->Construct_002491B0(screens_[slot]);
        }
        if(active_class_name_=="Calibrate display offset") {
            calibration_.emplace();
            calibration_->x=profile_.read_i32(0x144);calibration_->y=profile_.read_i32(0x148);
            //001F4AD8 caches current regional GS offsets on entry.
            calibration_->cached_x=calibration_->x;calibration_->cached_y=calibration_->y;
        }
        if(active_class_name_=="Autosave") {
            autosave_.emplace();
            const int document=hp2::frontend::screen_index_for_name_hash(screens_,0x0ce3f7ce);
            if(document<0)hp2::not_implemented("0x00245E30","original memory-card popup document missing");
            save_popup_=authored_screens_[static_cast<std::size_t>(document)];
            apply_save_popup();
        }
        open_menu_animation(screen_index);
        background_animation_.Bind_00249A40(&screens_[slot],
            [this](int bound) { return hp2::frontend::bRandom(bound, random_state_); },
            [this](const auto& widget) { return measure_widget(widget); });
        const auto hint = port::frontend::hint_binding(active_class_name_, 0);
        if (hint.attachment_known && hint.attached) {
            scrolling_text_ = std::make_unique<hp2::frontend::tAnimatedBlurb>();
            scrolling_text_->Construct_002491B0(screens_[slot]);
        }
        if (active_class_name_ == "Audio Options" || active_class_name_ == "Display Options") {
            menu_viewport_ = std::make_unique<hp2::frontend::tScrollingMenuList>();
            if (active_class_name_ == "Audio Options")
                menu_viewport_->Sub_001EF3C8(*menu_group_->screen_0x14);
            else
                menu_viewport_->Sub_001F33B8(*menu_group_->screen_0x14);
            menu_selector_ = std::make_unique<hp2::frontend::tLeftRightArrowScroll>();
            menu_selector_->Construct_0024A690(*menu_group_->screen_0x14, 0xD1CDA594, 0x9345E4A7);
        }
        menu_group_->EnsureMenu_0016A858();
        apply_jukebox_widgets();
        if(audio_screen_)hp2::frontend::Sub_0016DE30(*audio_screen_,screens_[slot],0,
            [this](const auto& w){return measure_widget(w);},[this](auto id){return event_localize(id);});
        // 001F1DD8 message 0x0B, after the menu list exists: 0016D2A8 looks the item up in it.
        // Both class names share this document and this constructor; only the +0x08 flag
        // differs, and that is what decides whether Back/activation runs the 00183830 reset.
        if (active_class_name_ == "Language Options" ||
            active_class_name_ == "Language Options Pre FE") {
            hp2::frontend::Construct_001F1A78(
                language_screen_, *menu_group_->screen_0x14, menu_group_->child_0x08,
                port_language_, active_class_name_ == "Language Options" ? 1 : 0);
        }
    });
    if (!result.not_implemented.empty()) return result;

    menu_screen_index_ = screen_index;
    // The tree is view state belonging to one screen, so it follows that screen in and out.
    event_tree_error_.clear();
    event_text_cursor_ = -1;
    if (screen_index == kEventTreeScreen && asset_source_) {
        // FUN_001FB218 message 0x0B: `FUN_00185568(0x2FC050)`, i.e. the tree is chosen by
        // profile+0x1072C, which the mode-select screen just wrote. The port used to pass a
        // hardcoded 9 here, so Championship opened the Hot Pursuit tree.
        const int mode = profile_.SelectedMode_00185568();
        const auto loaded =
            event_tree_.load(*asset_source_, event_tree_bundle_, mode, profile_);
        if (!loaded.ok) event_tree_error_ = loaded.error;
        for (int sibling : {17, 18, 19}) screens_[sibling] = authored_screens_[sibling];
        result.not_implemented = hp2::guard([&] {
            scrolling_text_ = std::make_unique<hp2::frontend::tAnimatedBlurb>();
            scrolling_text_->Construct_002491B0(screens_[slot], 0xfecebef6, 0xfecebef6, 0x70a404ec);
            event_car_logos_.Bind_001F8C10(screens_[slot], 0);
            event_track_logos_.Bind_001F90F0(screens_[slot], 0);
            apply_event_tree_widgets();
            bind_event_tree_text();
        });
        if (!result.not_implemented.empty()) { result.ok = true; return result; }
    } else {
        event_tree_.clear();
    }
    result = menu_state();
    result.ok = true;
    return result;
}

bool FrontEndRuntime::event_tree_joy_event(int joy_event) {
    if (menu_screen_index_ != kEventTreeScreen || !event_tree_.loaded()) return false;
    // Transition states consume input (001FAF60); dispatch must never leak to DummyMenuItem.
    if (event_tree_.transitioning()) return true;
    if (event_tree_.state() == 7 && joy_event != hp2::input::JOY_EVENT_MENU_SELECT &&
        joy_event != hp2::input::JOY_EVENT_START)
        return joy_event != hp2::input::JOY_EVENT_MENU_BACK;
    // FUN_001FAF60. State 0: left/right move the cursor by -1/+1 and up/down by +2/-2 through
    // FUN_001FA9C8, answering TreeLeftRight (0x14) or TreeUpDown (0x13), or 0 for a move that
    // does not land on a new node. Confirm zooms into the event (0x15) and Back zooms out again.
    // State 2 dispatches selection through FUN_001F6B70's status gate; state 5 returns
    // from reward preview. Missing downstream race/model dependencies are explicit throws.
    int sound = hp2::frontend::kSoundSilent_001FAF60;
    switch (joy_event) {
        case hp2::input::JOY_EVENT_SCROLL_LEFT:
        case hp2::input::JOY_EVENT_MENU_ANALOG_LEFT:
            sound = event_tree_.move_cursor_sound(-1);
            break;
        case hp2::input::JOY_EVENT_SCROLL_RIGHT:
        case hp2::input::JOY_EVENT_MENU_ANALOG_RIGHT:
            sound = event_tree_.move_cursor_sound(1);
            break;
        case hp2::input::JOY_EVENT_SCROLL_UP:
        case hp2::input::JOY_EVENT_MENU_ANALOG_UP:
            sound = event_tree_.move_cursor_sound(2);
            break;
        case hp2::input::JOY_EVENT_SCROLL_DOWN:
        case hp2::input::JOY_EVENT_MENU_ANALOG_DOWN:
            sound = event_tree_.move_cursor_sound(-2);
            break;
        case hp2::input::JOY_EVENT_MENU_SELECT:
        case hp2::input::JOY_EVENT_START:
            if (event_tree_.state() == 2) {
                const int status = event_tree_.selected_tree()->Status_002010B8(
                    event_tree_.profile(), event_tree_.cursor());
                // Debug001F2D20: only SELECT starts; Start has no branch.
                if(joy_event!=hp2::input::JOY_EVENT_MENU_SELECT)return true;
                if(status>0)selection_=std::make_unique<SelectionRuntime>(*this);
                else hp2::frontend::PlayMenuSound_0016E038(0x17);
                return true;
            }
            if (event_tree_.state() == 5) return true; // 001F65C8 delegates preview input.
            {
                const bool intro = event_tree_.state() == 7;
                sound = event_tree_.confirm();
                if (intro) sound = 5; // FUN_001F9598's returned sound survives the restore.
            }
            if (event_tree_.state() == 1) {
                profile_.SetSelectedEvent(event_tree_.tree_type(), event_tree_.cursor_info().number);
            }
            break;
        case hp2::input::JOY_EVENT_MENU_SQUARE:
            if (event_tree_.state() == 2) {
                const auto& record = event_tree_.selected_tree()->records.at(event_tree_.cursor());
                if (record.nRewards < 1) sound = 8;
                else {
                    event_tree_.show_preview();
                    event_car_logos_.Sub_001F8D08();
                    event_track_logos_.Sub_001F91E0();
                    hp2::frontend::PlayMenuSound_0016E038(0x18);
                    hp2::not_implemented("0x001F6440", "reward preview car/track model renderer not translated");
                }
            } else if (event_tree_.state() == 5) {
                event_tree_.close_preview(); sound = 0x18;
                event_car_logos_.Sub_001F8CF0(); event_track_logos_.Show();
            }
            break;
        case hp2::input::JOY_EVENT_MENU_BACK:
            // Only state 2 consumes Back; in state 0 the tree is a normal screen and Back pops
            // the navigation history, so the tree must let it through.
            if (event_tree_.state() != 2 && event_tree_.state() != 5) {
                return false;
            }
            sound = event_tree_.back();
            if (event_tree_.state() == 6) {
                event_car_logos_.Sub_001F8CF0(); event_track_logos_.Show();
            }
            break;
        default: return false;
    }
    // The recovered sound id goes through the recovered FUN_0016E038 and the same sink every
    // other screen uses, so the bSound event names and the 34 ms repeat suppression stay in one
    // place. Id 0 is the original's "nothing happened"; PlayMenuSound_0016E038 resolves it to a
    // null name, so nothing plays.
    hp2::frontend::PlayMenuSound_0016E038(sound);
    // This is called through menu_joy_event's guard so text dependencies are reported.
    bind_event_tree_text();
    apply_event_tree_widgets();
    return true;
}

// TASK-0092 E-0092-J05:001EF1C8/001EEF60/001AFA28. Host representation of
// original global AudioOptions, including exact default volume bits and song pairs.
void FrontEndRuntime::initialize_audio_options() {
    if(profile_load_.loaded)std::copy_n(profile_.bytes()+0x94,audio_options_.size(),audio_options_.begin());
    else {
        constexpr std::uint32_t defaults[]{0x3ee66666,0x3f59999a,0x3ec28f5c,0x3f0ccccd,
            0x3ea8f5c3,0x3e9eb852,0x3f400000};
        for(std::size_t i=0;i<7;++i)for(unsigned b=0;b<4;++b)
            audio_options_[i*4+b]=static_cast<std::uint8_t>(defaults[i]>>(b*8));
        const auto& tunes=jukebox_->tunes();
        if(tunes.size()>32)hp2::not_implemented("0x001EEF60","Jukebox exceeds original AudioOptions song capacity");
        audio_options_[0x9c]=static_cast<std::uint8_t>(tunes.size());
        audio_options_[0xa4]=5;audio_options_[0xac]=1;audio_options_[0xaf]=0xff;
        for(std::size_t i=0;i<tunes.size();++i) {
            audio_options_[0x1c+i*4]=tunes[i].cartridge_category;
            audio_options_[0x1d+i*4]=tunes[i].cartridge_song;
            audio_options_[0x1e + i*4]=static_cast<std::uint8_t>(tunes[i].type);
        }
    }
    //001EF240 publishes genre indices into the same snapshot. This also validates
    // the saved song pairs before using them in audio scheduling.
    open_jukebox();
    audio_options_=jukebox_screen_->snapshot;
    jukebox_screen_.reset();
    std::copy(audio_options_.begin(),audio_options_.end(),profile_.bytes()+0x94);
    filter_frontend_music();
}

void FrontEndRuntime::open_jukebox() {
    std::uint32_t count=0;
    for(unsigned b=0;b<4;++b)count|=std::uint32_t(audio_options_[0x9c+b])<<(b*8);
    if(count>32)hp2::not_implemented("0x001EEF60","saved AudioOptions song count exceeds original capacity");
    std::vector<port::frontend::JukeboxSongMetadata> metadata;
    for(std::size_t i=0;i<count;++i) {
        const auto& tunes=jukebox_->tunes();
        auto tune=std::find_if(tunes.begin(),tunes.end(),[&](const auto& t){
            return t.cartridge_category==audio_options_[0x1c+i*4] && t.cartridge_song==audio_options_[0x1d+i*4];});
        if(tune==tunes.end() || tune->metadata_fields.size()<5)
            hp2::not_implemented("0x001AED90","saved Jukebox pair or original metadata unavailable");
        metadata.push_back({tune->metadata_fields[0],tune->metadata_fields[1],tune->metadata_fields[3]});
    }
    auto state=std::make_unique<port::frontend::JukeboxScreenState>();
    hp2::frontend::Sub_001F02D8(*state,audio_options_,std::move(metadata));
    jukebox_screen_=std::move(state);
}

void FrontEndRuntime::filter_frontend_music() {
    const auto& tunes=jukebox_->tunes();
    std::vector<bool> enabled(tunes.size(),false);
    const auto count=audio_options_[0x9c]; // validated full word in open_jukebox
    for(std::size_t i=0;i<count;++i) {
        const auto category=audio_options_[0x1c+i*4],song=audio_options_[0x1d+i*4];
        const auto genre=audio_options_[0x1f+i*4];
        if(genre>=8)hp2::not_implemented("0x001AF050","saved Jukebox genre exceeds original bit mask");
        for(std::size_t j=0;j<tunes.size();++j)if(tunes[j].cartridge_category==category && tunes[j].cartridge_song==song)
            enabled[j]=hp2::port::audio::Sub_001AF050(audio_options_[0x1e + i*4],(audio_options_[0xaf]&(1u<<genre))!=0);
    }
    music_scheduler_->set_enabled(std::move(enabled));
}

void FrontEndRuntime::queue_jukebox_track(std::uint8_t category,std::uint8_t song,bool automatic) {
    const auto& tunes=jukebox_->tunes();
    auto tune=std::find_if(tunes.begin(),tunes.end(),[&](const auto& t){return t.cartridge_category==category && t.cartridge_song==song;});
    if(tune==tunes.end())hp2::not_implemented("0x0027E530","Jukebox cartridge/song pair unavailable");
    pending_music_index_=static_cast<int>(tune-tunes.begin());
    music_scheduler_->select(static_cast<std::size_t>(pending_music_index_));
    pending_music_stop_=true;music_auto_mode_=automatic;music_playing_=true;
}

void FrontEndRuntime::commit_jukebox(const port::frontend::JukeboxScreenState& state) {
    //001F0D5C..001F0ED8: Autosave precedes full copy/filter/resume. Preferred
    // index+A4 is independent of the highlighted preview cursor.
    if(state.snapshot!=audio_options_ && profile_.read_u32(4)!=0) {
        if(navigation_history_.size()>=12)hp2::not_implemented("0x0016B418","Jukebox Autosave history overflow");
        navigation_history_.emplace_back("Autosave");
    }
    audio_options_=state.snapshot;
    std::copy(audio_options_.begin(),audio_options_.end(),profile_.bytes()+0x94);
    filter_frontend_music();
    std::uint32_t preferred=0;
    for(unsigned b=0;b<4;++b)preferred|=std::uint32_t(audio_options_[0xa4+b])<<(b*8);
    if(preferred>=audio_options_[0x9c])hp2::not_implemented("0x001F0D10","Jukebox preferred song outside snapshot");
    const auto genre=audio_options_[0x1f+preferred*4];
    if((audio_options_[0xaf]&(1u<<genre)) && audio_options_[0x1e + preferred*4]!=0)
        queue_jukebox_track(audio_options_[0x1c+preferred*4],audio_options_[0x1d+preferred*4],true);
    else {
        music_auto_mode_=true;pending_music_stop_=true;pending_music_index_=-1;music_playing_=false;
        const auto next=music_scheduler_->finished(0); //0027D640(1) always selects NEXT.
        if(next)queue_jukebox_track(jukebox_->tunes()[*next].cartridge_category,jukebox_->tunes()[*next].cartridge_song,true);
    }
}

void FrontEndRuntime::dispatch_jukebox(int message,int& sound) {
    const auto ticks=static_cast<std::uint32_t>(std::fmod(menu_clock_seconds_*4000,4294967296.0));
    const auto elapsed=static_cast<float>(static_cast<std::int32_t>(ticks-jukebox_screen_->selection_tick))*0.00025f; //0032EC04
    port::frontend::JukeboxScreenServices services{
        [this](auto category,auto song,bool automatic){queue_jukebox_track(category,song,automatic);},
        [this]{pending_music_stop_=true;pending_music_index_=-1;music_auto_mode_=false;music_playing_=false;music_scheduler_->stop();},
        [this](const auto& state){commit_jukebox(state);}};
    hp2::frontend::Sub_001F0D10(*jukebox_screen_,message,sound,ticks,elapsed,music_playing_,services);
    apply_jukebox_widgets();
}

void FrontEndRuntime::apply_jukebox_widgets() {
    if(!jukebox_screen_ || active_class_name_!="JukeBox Music" || menu_screen_index_<0)return;
    hp2::frontend::Sub_001F0478(*jukebox_screen_,screens_[static_cast<std::size_t>(menu_screen_index_)],
        [this](auto id){return event_localize(id);});
}

void FrontEndRuntime::apply_save_popup() {
    if(!autosave_ || !save_popup_)return;
    auto& document=*save_popup_;
    //00245E30 config:0 buttons,flags9,title249F4,authored document0CE3F7CE.
    const auto format=event_localize(autosave_->text_id);
    char text[256]; //00245E30 formats slot+1 (twice for the saving warning).
    const int length=std::snprintf(text,sizeof(text),format.c_str(),autosave_->logical_slot+1,autosave_->logical_slot+1);
    if(length<0 || length>=static_cast<int>(sizeof(text)))
        hp2::not_implemented("0x00245E30","formatted save popup exceeds original temporary buffer");
    hp2::frontend::Sub_00144BA0(document,0x5d6dd123,text);
    hp2::frontend::SetWidgetStringId_001449A8(document,0x85eda094,0x249f4);
    if(auto* title=hp2::frontend::FindWidget_00143D50(document,0x85eda094,1))title->text=event_localize(0x249f4);
    for(auto hash:{0xe0105663u,0xe0105664u,0xe0105665u})
        if(auto* button=hp2::frontend::FindWidget_00143D50(document,hash,1)) {
            button->string_id_0x60=0;button->text.clear();button->dirty_0x2b=5;
        }
    hp2::frontend::SetWidgetVisible_00144968(document,0xab671c9a,false);
}

void FrontEndRuntime::capture_display_baseline() {
    //001EE6C0: two eight-byte records followed by both complete0x30 player records.
    std::size_t destination=0;
    for(const auto& block:std::array<std::pair<std::size_t,std::size_t>,4>{{{0x144,8},{0x14c,8},{0x10d7c,0x30},{0x1144c,0x30}}}) {
        std::copy_n(profile_.bytes()+block.first,block.second,display_baseline_.begin()+destination);
        destination+=block.second;
    }
    display_baseline_valid_=true;
}
bool FrontEndRuntime::display_settings_changed() const {
    if(!display_baseline_valid_)hp2::not_implemented("0x001EE878","Display baseline not captured");
    std::size_t destination=0;
    for(const auto& block:std::array<std::pair<std::size_t,std::size_t>,4>{{{0x144,8},{0x14c,8},{0x10d7c,0x30},{0x1144c,0x30}}}) {
        if(!std::equal(profile_.bytes()+block.first,profile_.bytes()+block.first+block.second,display_baseline_.begin()+destination))return true;
        destination+=block.second;
    }
    return false;
}
void FrontEndRuntime::commit_hud() {
    if(hud_screen_)std::copy(hud_screen_->snapshot.begin(),hud_screen_->snapshot.end(),
        profile_.bytes()+0x10d7c+hud_screen_->player*0x6d0); //001F4178 destruction
}
void FrontEndRuntime::dispatch_display(int message,hp2::frontend::MenuEvent_0016A458& event) {
    const auto* target=hp2::frontend::Sub_001F3A28(*display_screen_,message,event,{});
    hp2::frontend::Sub_001F3A28(*display_screen_,screens_[static_cast<std::size_t>(menu_screen_index_)],
        [this](auto id){return event_localize(id);});
    if(message==4) {
        constexpr std::uint32_t targets[]{0x55cb09f2,0x2e931f1b,0x03d90172,0x5610f2f4,0x422313cb,0x8fe4086b};
        const auto item=event.field_0x00-0x52e039f3u;
        selection_animation_.Bind_002488D0(item<6 ? targets[item] : 0,static_cast<float>(menu_clock_seconds_));
    }
    if(target) {
        std::copy(display_screen_->snapshot.begin(),display_screen_->snapshot.end(),profile_.bytes()+0x14c);
        exit_animation_=std::make_unique<hp2::frontend::tExitScreenAnimation>();
        exit_animation_->Construct_002490D8(screens_[static_cast<std::size_t>(menu_screen_index_)]);exit_class_name_=target;
    }
}
void FrontEndRuntime::dispatch_hud(int message,hp2::frontend::MenuEvent_0016A458& event) {
    hp2::frontend::Sub_001F46D8(*hud_screen_,message,event);
    auto& screen=screens_[static_cast<std::size_t>(menu_screen_index_)];
    hp2::frontend::Sub_001F46D8(*hud_screen_,screen,[this](auto id){return event_localize(id);});
    if(message==4) {
        constexpr std::uint32_t values[]{0x2e018745,0xffc3dcd2,0x4ceb3ed2,0x1054482b,0xc7e2117e,0x6caf6d10,0x6bd89c11};
        constexpr std::uint32_t tips[]{0x24d74,0x24d75,0x24d76,0x24d77,0x24d78,0x24d79,0x24cb0};
        const auto item=event.field_0x00-0x52e039f3u;
        hud_arrow_target_=item<2 ? event.field_0x00 : item<7 ? values[item] : 0;
        selection_animation_.Bind_002488D0(item<7 ? values[item] : 0,static_cast<float>(menu_clock_seconds_));
        if(scrolling_text_)scrolling_text_->Bind_00249518(item<7 ? event_localize(tips[item]) : "");
        bind_menu_animation(event.field_0x00);
        update_hud_arrows(0);
    }
}
void FrontEndRuntime::update_hud_arrows(float delta) {
    if(!hud_screen_)return;
    auto& screen=screens_[static_cast<std::size_t>(menu_screen_index_)];
    hud_arrow_phase_+=600.0f*delta; //003319C4,00215EE8
    if(hud_arrow_phase_>360.0f)hud_arrow_phase_-=360.0f;
    std::uint32_t bits=0x3c8efa36;float radians;std::memcpy(&radians,&bits,4);
    const int alpha=static_cast<int>((hp2::frontend::bSin(hud_arrow_phase_*radians)+1.0f)*64.0f);
    const auto rect=hp2::frontend::GetWidgetRect_00144280(screen,hud_arrow_target_,[this](const auto& w){return measure_widget(w);});
    for(auto hash:{0xd1cda594u,0x9345e4a7u})if(auto* arrow=hp2::frontend::FindWidget_00143D50(screen,hash,2)) {
        arrow->visible=hud_arrow_target_ ? 1 : 0;
        if(rect.valid) {
            const int top=static_cast<int>(rect.y0),bottom=static_cast<int>(rect.y1);
            const int height=static_cast<int>(measure_widget(*arrow).height);
            arrow->y=static_cast<float>(top+(bottom-top)/2)-static_cast<float>(height)*0.5f;
            arrow->dirty_0x2b=4;
        }
        hp2::frontend::SetAlpha(*arrow,alpha);
    }
}

// Retail0024D870/0024D5C0/0024D088, E-0095-N01..N03.
void FrontEndRuntime::commit_name_entry() {
    if(!name_entry_)return;
    for(int p=0;p<2;++p) {
        std::copy_n(name_entry_->names[p].begin(),11,profile_.bytes()+0x10d90+p*0x6d0);
        profile_.bytes()[0x10d83+p*0x6d0]=name_entry_->flags[p];
    }
}
void FrontEndRuntime::dispatch_name_entry(int message,hp2::frontend::MenuEvent_0016A458& event) {
    auto& state=*name_entry_;auto& screen=screens_[static_cast<std::size_t>(menu_screen_index_)];
    const auto measure=[this](const auto& w){return measure_widget(w);};
    const auto localize=[this](auto id){return event_localize(id);};
    if(state.active) {
        const int old_row=state.row,old_column=state.column;
        hp2::frontend::Sub_0024D088(state,message,event);
        if(state.active && (old_row!=state.row || old_column!=state.column)) {
            const bool wrap=std::abs(old_row-state.row)>1 || std::abs(old_column-state.column)>1;
            hp2::frontend::Sub_0024C948(state,screen,measure,wrap);
        }
        if(!state.active)commit_name_entry();
    } else {
        if(message==1) {
            const int player=event.field_0x00==0x52e039f5?1:0;
            if((event.field_0x00==0x52e039f3 || event.field_0x00==0x52e039f5) && event.pad_index_0x04==player) {
                hp2::frontend::Sub_0024C708(state,player);
                hp2::frontend::Sub_0024C948(state,screen,measure,true);
                state.panel_x={};state.panel_x.current=650;state.panel_x.target=state.panel_authored_x;
                state.panel_x.duration=0.5f;state.panel_x.phase=2;
                //0024C708 performs its immediate render/update before the first tick.
                hp2::frontend::Sub_0024CA60(state,screen,0,measure,localize);
            } else event.sound_id_0x0C=0;
        } else if((message==6 || message==7) && (event.field_0x00==0x52e039f4 || event.field_0x00==0x52e039f6)) {
            hp2::frontend::Sub_0024D660(state,message,event);commit_name_entry();
        } else if(message==4) {
            bind_menu_animation(event.field_0x00);
            const std::uint32_t value=event.field_0x00==0x52e039f4?0xd6f9d954u:event.field_0x00==0x52e039f6?0xc33bae35u:0u;
            selection_animation_.Bind_002488D0(value,static_cast<float>(menu_clock_seconds_));
            if(menu_selector_)menu_selector_->Bind_0024A860(value);
            bind_scrolling_text(event.field_0x00);
        }
    }
    hp2::frontend::Sub_0024D408(state,screen,localize);
}

void FrontEndRuntime::commit_audio(bool autosave) {
    if(!audio_screen_ || audio_screen_->snapshot==audio_options_)return;
    audio_options_=audio_screen_->snapshot;
    std::copy(audio_options_.begin(),audio_options_.end(),profile_.bytes()+0x94);
    filter_frontend_music(); //001EF120 reapplies current frontend eligibility.
    if(autosave && profile_.read_u32(4)!=0) {
        if(navigation_history_.size()>=12)hp2::not_implemented("0x0016B418","Audio Autosave history overflow");
        navigation_history_.emplace_back("Autosave");
    }
}

// E-0093-A02,001EFB80/001EFAA0/001AFAB8/001AF1E0 host service binding.
void FrontEndRuntime::dispatch_audio(int message,hp2::frontend::MenuEvent_0016A458& event) {
    auto& state=*audio_screen_;
    const auto hash=event.field_0x00;
    if(message==6 || message==7) {
        if(hash>=0x52e039f3 && hash<=0x52e039f9) {
            const auto feedback=hp2::frontend::Sub_0016DD38(state,hash-0x52e039f3,message==6 ? -0.05f : 0.05f);
            if(feedback)pending_volume_feedback_.emplace_back("MenuAdjustVolume",*feedback); //001AF760/002B2B48
        } else if(hash==0x52e039fa) {
            const int old=state.snapshot[0xac]|(int(state.snapshot[0xad])<<8);
            state.snapshot[0xac]=static_cast<std::uint8_t>((old+(message==6 ? 2 : 1))%3);
            state.snapshot[0xad]=0;event.sound_id_0x0C=message==6 ? 1 : 2;
        }
    } else if(message==1) {
        if(hash==0x52e039fb) {
            commit_audio(false);
            exit_animation_=std::make_unique<hp2::frontend::tExitScreenAnimation>();
            exit_animation_->Construct_002490D8(screens_[static_cast<std::size_t>(menu_screen_index_)]);
            exit_class_name_="JukeBox Music";
        } else if(hash==0xaee77883) {
            constexpr std::uint32_t defaults[]{0x3ee66666,0x3f59999a,0x3ec28f5c,0x3f0ccccd,0x3ea8f5c3,0x3e9eb852,0x3f400000};
            for(std::size_t i=0;i<7;++i) {
                for(unsigned b=0;b<4;++b)state.snapshot[i*4+b]=static_cast<std::uint8_t>(defaults[i]>>(b*8));
                const auto feedback=hp2::frontend::Sub_0016DD38(state,i,0);
                if(feedback)pending_volume_feedback_.emplace_back("MenuAdjustVolume",*feedback);
            }
        } else if(hash==0xaee77884) {
            for(std::size_t i=0;i<state.snapshot[0x9c];++i) {
                const auto category=state.snapshot[0x1c+i*4],song=state.snapshot[0x1d+i*4];
                for(const auto& tune:jukebox_->tunes())if(tune.cartridge_category==category && tune.cartridge_song==song)
                    state.snapshot[0x1e + i*4]=static_cast<std::uint8_t>(tune.type);
            }
            for(std::size_t i=0xa0;i<0xac;++i)state.snapshot[i]=0;
            state.snapshot[0xae]=0;state.snapshot[0xaf]=0xff;
        } else event.sound_id_0x0C=0;
    } else if(message==13)event.sound_id_0x0C=0;
    hp2::frontend::Sub_0016DE30(state,screens_[static_cast<std::size_t>(menu_screen_index_)],0,
        [this](const auto& w){return measure_widget(w);},[this](auto id){return event_localize(id);});
}

void FrontEndRuntime::dispatch_activation(hp2::frontend::MenuEvent_0016A458& event) {
    if(audio_screen_){dispatch_audio(1,event);return;}
    if(display_screen_){dispatch_display(1,event);return;}
    const std::uint32_t name_hash = event.field_0x00;
    port::frontend::ScreenActionServices services{
        [this](const char* name, bool push) {
            pending_class_name_ = name;
            pending_push_history_ = push;
        },
        [this](const char* name) {
            exit_animation_ = std::make_unique<hp2::frontend::tExitScreenAnimation>();
            exit_animation_->Construct_002490D8(screens_[static_cast<std::size_t>(menu_screen_index_)]);
            exit_class_name_ = name;
        },
        [this] { capture_display_baseline(); },
        [this](std::uint32_t address, std::uint32_t argument) {
            // The same three seams the Back path uses, in the same order the handler calls them.
            if (address == 0x0021E0D0) apply_language(static_cast<int>(argument));
            else if (address == 0x00170320 && argument == 1) {
                request_history_back();
                apply_pending_screen(); // 001F1B08 tail-calls the pop itself
            } else {
                hp2::not_implemented(hex(address).c_str(),
                                     "activation side-effect dependency not translated");
            }
        }};
    if (active_class_name_ == "Language Options" ||
        active_class_name_ == "Language Options Pre FE") {
        // 001F1DD8 forwards every message but 0x0B/0x0C to 001F1B08.
        hp2::frontend::Sub_001F1B08(1, name_hash, language_screen_, port_language_,
                                    dat_00331A60_, services);
    } else if (menu_screen_index_ == 0) {
        hp2::frontend::Sub_00216638(1, name_hash, dat_002FC070_, services);
    } else if (menu_screen_index_ == 24) {
        hp2::frontend::Sub_001EECA8(1, name_hash, services);
    } else if(active_class_name_=="Audio Options" && name_hash==0x52e039fb) {
        //001EFB80 message1, E-0092-J06: copy/apply AudioOptions before the
        // original exit animation to JukeBox Music. No edited audio snapshot yet.
        services.exit_animation("JukeBox Music");
    } else if (menu_screen_index_ == 1) {
        // "Cops On Mode Select" (FUN_002174A0). The event is passed through because the original
        // gates Quick Race and Tournament on its +0x04 pad field.
        hp2::frontend::Sub_002174A0(1, name_hash, event, mode_select_globals_, services);
        commit_mode_select_globals();
    } else if (menu_screen_index_ == 2) {
        hp2::frontend::Sub_00217AF8(1, name_hash, event, mode_select_globals_, services); // 0x00217AF8
        commit_mode_select_globals();
    } else {
        const auto* registration = port::frontend::find_registration(active_class_name_);
        const std::string address = registration == nullptr ? "unknown screen handler"
                                                            : hex(registration->handler_address);
        hp2::not_implemented(address.c_str(), "screen item activation handler not translated");
    }
}

void FrontEndRuntime::apply_event_tree_widgets() {
    // What FUN_001F96B8 and FUN_001FA698 do to screen 39's authored widgets at runtime.
    // (E-0021-16, E-0021-27, E-0021-28)
    if (menu_screen_index_ != kEventTreeScreen) return;
    if (static_cast<std::size_t>(menu_screen_index_) >= screens_.size()) return;
    auto& screen = screens_[static_cast<std::size_t>(menu_screen_index_)];

    // FUN_001F96B8: FUN_001449A8(screen, EVENT_TREE, id), where the id is 0x272A for
    // DAT_0030C77C == 8 and 0x272D for 9. Until this runs the widget keeps its authored
    // placeholder, which is the literal string "This Will Change" -- the title the port was
    // showing.
    const std::uint32_t title_hash =
        hp2::frontend::bStringHash(hp2::frontend::kWidgetEventTree_A01F24F0);
    hp2::frontend::SetWidgetStringId_001449A8(screen, title_hash, event_tree_.title_string_id());
    // FUN_00140EA8 clears the authored placeholder and leaves the text system to resolve the
    // new id at draw time (FUN_0021E560). The port resolves string ids once at load, so an id
    // assigned at runtime has to be resolved here or the widget draws empty.
    if (language_) {
        if (auto* title = hp2::frontend::FindWidget_00143D50(screen, title_hash, 1)) {
            if (title->string_id_0x60 != 0 &&
                !language_->lookup(title->string_id_0x60, title->text)) {
                title->text = "NOT TRANSLATED PROPERLY"; // ELF 0x002BC958, E-0016-S01
            }
        }
    }

    // FUN_001FA698, every frame: FUN_001454E8(screen, 0xA2A16EA1, state == 0). The overlay
    // class covers BG1, DIVERBAR, SMALL_EVENTNUMBER and the OVERLAY_* text widgets, so the
    // whole status/reward overlay is visible while browsing and hidden in every other state.
    hp2::frontend::SetClassVisible_001454E8(
        screen, hp2::frontend::kOverlayClassHash_A2A16EA1,
        event_tree_.state() == hp2::frontend::kViewStateBrowsing_001FAF60);

    // FUN_001F9F50 / E-0022-T: only state 7 hides help.
    const bool browsing = event_tree_.state() != 7;
    hp2::frontend::SetWidgetVisible_00144968(
        screen, hp2::frontend::bStringHash(hp2::frontend::kWidgetHelpText_0880AC4C),
        browsing);
    hp2::frontend::SetWidgetVisible_00144968(
        screen, hp2::frontend::bStringHash(hp2::frontend::kWidgetHelpButton_027F7503),
        browsing);
    // FUN_001F96B8 hides the authored leaf templates; leaf draw emits instances.
    hp2::frontend::SetClassVisible_001454E8(screen, 0xa01bcdc6, false);
}

std::string FrontEndRuntime::event_localize(std::uint32_t id) const {
    std::string text;
    if (!language_ || !language_->lookup(id, text))
        hp2::not_implemented("0x0021E560", "event text localization entry unavailable");
    return text;
}

std::string FrontEndRuntime::event_track_name(int index, int direction) const {
    return event_localize(event_names_.track_name_id(index, direction != 0));
}
std::string FrontEndRuntime::event_car_name(int index) const {
    auto text = event_localize(assets::EventNameCatalog::car_name_id(index));
    // FUN_001F6220 passes the resolved car name through FUN_0021E508. Absent
    // hash translation returns the original text, as that function explicitly does.
    std::string translated;
    if (language_->lookup_hash(hp2::frontend::bStringHash(text.c_str()), translated))
        return translated;
    return text;
}
std::string FrontEndRuntime::event_timer(std::int32_t ticks) const {
    return hp2::frontend::Sub_001A3B98(ticks, port_language_);
}

void FrontEndRuntime::bind_event_tree_text() {
    if (!event_tree_.loaded() || event_text_cursor_ == event_tree_.cursor()) return;
    const auto& tree = *event_tree_.selected_tree();
    const int index = event_tree_.cursor();
    const auto& record = tree.records.at(static_cast<std::size_t>(index));
    const auto& profile = event_tree_.profile();
    const auto timer = [this](std::int32_t ticks) { return event_timer(ticks); };
    const auto localize = [this](std::uint32_t id) { return event_localize(id); };
    const auto ticks = static_cast<std::uint32_t>(std::fmod(menu_clock_seconds_ * 4000, 4294967296.0));
    std::vector<std::uint32_t> car_logos, track_logos;
    for (int i = 0; i < record.nCars && i < 8; ++i)
        if (const auto* row = tree.GetCar(index, i))
            car_logos.push_back(event_names_.car_logo_hash((*row >> 16) & 0xff));
    for (int i = 0; i < record.nTracks && i < 12; ++i)
        if (const auto* row = tree.GetTrack(index, i))
            track_logos.push_back(event_names_.track_logo_hash(*row & 0xff, (*row >> 9) & 1, port_language_));
    event_car_logos_.Sub_001F8E18(car_logos, ticks);
    event_track_logos_.Sub_001F91F0(track_logos, ticks);
    auto set_text = [](hp2::frontend::FeScreen& screen, std::uint32_t hash, const std::string& text) {
        if (auto* widget = hp2::frontend::FindWidget_00143D50(screen, hash, 1)) {
            widget->string_id_0x60 = 0;
            // 001451E0 draws the separate 512-byte paragraph buffer directly; the
            // authored TRACK_LINE_1 capacity is not the paragraph output bound.
            widget->text = hash == 0x7a31036b ? text :
                (widget->text_capacity ? text.substr(0, widget->text_capacity - 1) : text);
            widget->dirty_0x2b = 3;
        }
    };
    // FUN_001F8890 / E-0022-R03: real browsing overlay producers.
    const int status = tree.Status_002010B8(profile, index);
    const auto result = status >= 3
        ? hp2::frontend::Sub_00200E78(tree, profile, index, status, localize, timer) : std::string{};
    event_pending_status_ = hp2::frontend::Sub_001F60E8(
        status, event_localize(hp2::frontend::Sub_001F60E8(status)),
        tree.Sub_002012D8(profile, index), hp2::frontend::GetRaceCount(record), result);
    const auto reward = hp2::frontend::Sub_001F6220(tree.GetReward(index, 0),
        event_localize(0x101d8), [this](int car) { return event_car_name(car); },
        [this](int track, int direction) { return event_track_name(track, direction); });
    event_pending_reward_ = reward;
    set_text(screens_[19], 0x7e428084, reward);
    const auto number_format = event_localize(0x111af);
    char number[256];
    std::snprintf(number, sizeof(number), number_format.c_str(), record.number_0x28);
    event_pending_number_ = number;
    event_overlay_pending_ = true;
    if (scrolling_text_) scrolling_text_->Bind_00249518(event_localize(record.string_id_0xB0));
    // FUN_001F6770: choose completed-track ordinal, then fall back to row zero.
    const auto* track = tree.GetTrack(index, tree.Sub_002012D8(profile, index));
    if (!track) track = tree.GetTrack(index, 0);
    if (track) set_text(screens_[17], 0xf601f2d4,
                        event_track_name(*track & 0xff, (*track >> 9) & 1));
    set_text(screens_[17], 0x7a31036b, hp2::frontend::Sub_001F5F88(
        tree, index, event_localize(record.string_id_0xB4), timer));
    // FUN_001F9480 / E-0022-T09: document 18's original introduction text.
    set_text(screens_[18], 0x7a31036b, event_localize(0xe32c9));
    const bool initial = event_text_cursor_ < 0;
    if (initial) {
        set_text(screens_[39], 0x87cfd6c6, event_pending_status_);
        set_text(screens_[39], 0x35b77247, event_pending_reward_);
        set_text(screens_[39], 0xcea3b7c2, event_pending_number_);
        event_overlay_pending_ = false;
    }
    event_text_cursor_ = index;
    event_overlay_seconds_ = initial ? -0.15 : 0; // 001F8A98 defers initial overlay.
    const auto* small = hp2::frontend::FindWidget_00143D50(screens_[39], 0xcea3b7c2, 1);
    event_overlay_alpha_ = initial || !small ? 0 : small->a;
}

void FrontEndRuntime::update_event_tree_overlay(float delta) {
    if (!event_tree_.loaded()) return;
    // FUN_001F8938 / E-0022-R03. gp-4194/4190/418C/4188: .15,.15,.35,382.
    event_overlay_seconds_ += delta;
    const float t = static_cast<float>(event_overlay_seconds_);
    float x = 382, alpha = 0;
    if (t > 0.15f) {
        if (event_overlay_pending_) {
            for (const auto& entry : {std::pair<std::uint32_t, const std::string*>{0x87cfd6c6, &event_pending_status_},
                                     {0x35b77247, &event_pending_reward_}, {0xcea3b7c2, &event_pending_number_}})
                if (auto* widget = hp2::frontend::FindWidget_00143D50(screens_[39], entry.first, 1)) {
                    widget->string_id_0x60 = 0;
                    widget->text = *entry.second;
                    widget->dirty_0x2b = 3;
                }
            event_overlay_pending_ = false;
        }
        x = t < 0.35f ? 382 + (640 - 382) * (0.35f - t) / (0.35f - 0.15f) : 382;
        alpha = 128;
    } else if (t < 0.15f) alpha = event_overlay_alpha_ * (1 - t / 0.15f);
    for (auto& widget : screens_[39].widgets) {
        if (widget.class_hash != 0xa2a16ea1) continue;
        const auto* authored = hp2::frontend::FindWidget_00143D50(authored_screens_[39], widget.name_hash, 0);
        if (authored) widget.x = authored->x + x - 382;
        hp2::frontend::SetAlpha(widget, static_cast<int>(alpha));
    }
}

void FrontEndRuntime::commit_mode_select_globals() {
    // E-0021-23: five of the six mode-select globals live inside the profile, so in the original
    // the handler's writes ARE profile writes. ModeSelectGlobals is a port-side mirror, and this
    // is the one place that reconciles it, right after a handler returns. Without this the
    // event tree read a hardcoded mode and ignored which entry the player chose.
    using hp2::memorycard::Profile_002FC050;
    profile_.write_u32(0x001c, static_cast<std::uint32_t>(mode_select_globals_.DAT_002FC06C));
    profile_.write_u32(hp2::memorycard::kEventTreeViewState_001FAF60,
                       static_cast<std::uint32_t>(mode_select_globals_.DAT_002FC1C0));
    profile_.write_u32(hp2::memorycard::kSelectedModeField_00185568,
                       static_cast<std::uint32_t>(mode_select_globals_.DAT_0030C77C));
    profile_.write_u32(0x10734, static_cast<std::uint32_t>(mode_select_globals_.DAT_0030C784));
    // FUN_001F5D50(0x2FC1B4, n), only when the handler actually called it.
    if (mode_select_globals_.DAT_002FC1B4_tree_type >= 0) {
        profile_.SetEventTreeType_001F5D50(mode_select_globals_.DAT_002FC1B4_tree_type);
    }
}

void FrontEndRuntime::apply_pending_screen() {
    if (pending_class_name_.empty()) return;
    const std::string name = pending_class_name_;
    pending_class_name_.clear();
    const auto* registration = port::frontend::find_registration(name);
    if (registration == nullptr) hp2::not_implemented("0x0016C8C8", "requested class not recovered");
    const int target = hp2::frontend::screen_index_for_name_hash(screens_, registration->document_hash);
    if (target < 0) hp2::not_implemented("0x0016ADB0", "registered document not present in loaded bundle");
    if (pending_push_history_) {
        if (navigation_history_.size() >= 12)
            hp2::not_implemented("0x0016B418", "history overflow behavior not recovered");
        navigation_history_.push_back(active_class_name_);
    }
    if(selection_ && selection_->observation().active && name!="Select Car"){selection_->close();selection_.reset();}
    const auto activated = activated_name_hash_;
    requested_class_for_open_ = name;
    const auto opened = menu_open(target);
    active_class_name_ = name; // preserve many-to-one class identity (e.g. NameEntry)
    activated_name_hash_ = activated;
    if (!opened.ok || !opened.not_implemented.empty()) hp2::not_implemented("0x0016C4C0", ("target menu construction failed: "+opened.not_implemented).c_str());
    if(name=="Select Car"){if(!selection_)throw std::logic_error("Selection owner absent");selection_->activate();}
}

void FrontEndRuntime::request_history_back() {
    // 0016C670/0016B438: empty history leaves the current screen alone.
    if (navigation_history_.empty()) return;
    pending_class_name_ = navigation_history_.back();
    navigation_history_.pop_back();
    pending_push_history_ = false;
}

void FrontEndRuntime::apply_language(int language) {
    if (language == port_language_) return; // 0021E0D0 returns immediately for unchanged locale.
    // Port asset I/O seam for 0021E0D0; enum order matches original locale table.
    static constexpr const char* names[]{"ENGLISH", "FRENCH", "GERMAN", "ITALIAN", "SPANISH", "SWEDISH"};
    if (language < 0 || language >= 6) hp2::not_implemented("0x0021E0D0", "unknown locale");
    if (!asset_source_) hp2::not_implemented("0x0021E0D0", "asset root unavailable");
    std::vector<std::uint8_t> bytes;
    try {
        bytes = asset_source_->read(std::string("LANGUAGES/FE_") + names[language] + ".BIN");
    } catch (const std::exception&) {
        hp2::not_implemented("0x0021E0D0", "locale asset unavailable");
    }
    language_ = std::make_unique<hp2::port::assets::FeLanguage>(bytes);
    for (auto* collection : {&screens_, &authored_screens_}) for (auto& screen : *collection)
        for (auto& widget : screen.widgets) if (widget.string_id_0x60 &&
            !language_->lookup(widget.string_id_0x60, widget.text)) widget.text = "NOT TRANSLATED PROPERLY";
    port_language_ = language;
}

MenuState FrontEndRuntime::menu_joy_event(int pad_index, int joy_event, bool pressed) {
    MenuState result;
    if (!menu_group_) {
        result.error = "no menu open";
        return result;
    }
    const std::string not_implemented = hp2::guard([&] {
        if(autosave_)return; //0016BAC0 flags9 consumes all input while status popup is active.
        if(name_entry_ && name_entry_->active) {
            if(!pressed)return;
            int message=0;
            switch(joy_event) {
            case hp2::input::JOY_EVENT_MENU_SELECT:message=1;break;
            case hp2::input::JOY_EVENT_SCROLL_LEFT:case hp2::input::JOY_EVENT_MENU_ANALOG_LEFT:message=6;break;
            case hp2::input::JOY_EVENT_SCROLL_RIGHT:case hp2::input::JOY_EVENT_MENU_ANALOG_RIGHT:message=7;break;
            case hp2::input::JOY_EVENT_SCROLL_UP:case hp2::input::JOY_EVENT_MENU_ANALOG_UP:message=8;break;
            case hp2::input::JOY_EVENT_SCROLL_DOWN:case hp2::input::JOY_EVENT_MENU_ANALOG_DOWN:message=9;break;
            case hp2::input::JOY_EVENT_MENU_SQUARE:message=10;break;
            case hp2::input::JOY_EVENT_MENU_L1:message=15;break;
            case hp2::input::JOY_EVENT_MENU_R1:message=16;break;
            case hp2::input::JOY_EVENT_MENU_BACK:message=17;break;
            default:return;
            }
            hp2::frontend::MenuEvent_0016A458 event;event.pad_index_0x04=pad_index;
            dispatch_name_entry(message,event);
            hp2::frontend::PlayMenuSound_0016E038(event.sound_id_0x0C);
            return;
        }
        if(selection_ && selection_->observation().active){selection_->input(pad_index,joy_event,pressed);apply_pending_screen();return;}
        if (pressed && event_tree_joy_event(joy_event)) {apply_pending_screen();return;}
        // FUN_001EEF00 forwards to 00249128 during exit, which consumes further menu input.
        if (!exit_animation_ && joy_event == hp2::input::JOY_EVENT_MENU_BACK && pressed) {
            if(jukebox_screen_) {
                int sound=6;dispatch_jukebox(17,sound);
                hp2::frontend::PlayMenuSound_0016E038(sound);
                request_history_back();
            } else if(name_entry_) {
                commit_name_entry();
                if(hp2::frontend::Sub_0024D370(*name_entry_) && profile_.read_u32(4)!=0) {
                    if(navigation_history_.size()>=12)hp2::not_implemented("0x0016B418","NameEntry Autosave history overflow");
                    navigation_history_.emplace_back("Autosave");
                }
                hp2::frontend::PlayMenuSound_0016E038(6);request_history_back();
            } else if(audio_screen_) {
                commit_audio(true);hp2::frontend::PlayMenuSound_0016E038(6);request_history_back();
            } else if(display_screen_) {
                std::copy(display_screen_->snapshot.begin(),display_screen_->snapshot.end(),profile_.bytes()+0x14c);
                if(display_settings_changed() && profile_.read_u32(4)!=0) {
                    if(navigation_history_.size()>=12)hp2::not_implemented("0x0016B418","Display Autosave history overflow");
                    navigation_history_.emplace_back("Autosave");
                }
                display_baseline_valid_=false;hp2::frontend::PlayMenuSound_0016E038(6);request_history_back();
            } else if(hud_screen_ || calibration_ || active_class_name_=="Calibrate Display") {
                hp2::frontend::PlayMenuSound_0016E038(6);request_history_back();
            } else if (active_class_name_ == "Options") {
                hp2::frontend::PlayMenuSound_0016E038(6);
                request_history_back();
            } else if (active_class_name_ != "New Main Menu") {
                const auto* registration = port::frontend::find_registration(active_class_name_);
                if (!registration) hp2::not_implemented("0x0016C670", "Back class registration missing");
                port::frontend::BackContext context;
                // No settings/editor mutation handlers are enabled in this runtime. The
                // entry snapshots therefore remain identical, not an unknown comparison.
                context.settings_changed = false;
                context.keyboard_active = false; // 0024D408 initializes editor state to zero.
                context.language = port_language_;
                context.language_instance_0x08 = active_class_name_ == "Language Options" ? 1 : 0;
                if (auto* item = menu_group_->child_0x08 ? menu_group_->child_0x08->current_0x08 : nullptr)
                    context.selected_hash = item->name_hash_0x0C;
                port::frontend::BackServices services{
                    [this](std::uint32_t address, std::uint32_t arg) {
                        if (address == 0x0021E0D0) apply_language(static_cast<int>(arg));
                        else if (address == 0x00170320 && arg == 1) {
                            request_history_back();
                            apply_pending_screen(); // Language handler pops before outer sound playback.
                        }
                        else hp2::not_implemented("0x0016C670", "Back side-effect dependency not translated");
                    },
                    [this](const char* name) {
                        if (navigation_history_.size() >= 12) hp2::not_implemented("0x0016B418", "history overflow");
                        navigation_history_.emplace_back(name);
                    }};
                // FUN_001FAF60 state 0, message 0x11: FUN_001F5D28 stores the node the cursor
                // is on into the profile before the manager pops. Done here because the profile
                // lives here, not in the recovered Back table. (E-0021-26)
                if (menu_screen_index_ == kEventTreeScreen && event_tree_.loaded() &&
                    event_tree_.state() == hp2::frontend::kViewStateBrowsing_001FAF60) {
                    const auto node = event_tree_.cursor_info();
                    profile_.SetSelectedEvent(event_tree_.tree_type(), node.number);
                }
                const auto event = port::frontend::query_back(registration->handler_address, context, services);
                hp2::frontend::PlayMenuSound_0016E038(event.sound);
                if (event.permission == 1) request_history_back();
            } // 00216664 blocks main Back and clears sound.
        } else if (!exit_animation_) {
            menu_group_->HandleJoyEvent_0016AA00(pad_index, joy_event, pressed);
        }
        apply_pending_screen();
    });
    result = menu_state();
    result.ok = true;
    result.not_implemented = not_implemented;
    return result;
}

MenuState FrontEndRuntime::menu_state() const {
    MenuState state;
    if(selection_)state.selection=selection_->observation();
    state.screen = menu_screen_index_;
    state.dat_002FC070 = dat_002FC070_;
    state.active_class = active_class_name_;
    state.history_depth = static_cast<int>(navigation_history_.size());
    state.exiting = exit_animation_ != nullptr;
    if (!menu_group_ || menu_group_->child_0x08 == nullptr) return state;
    state.has_list = true;

    auto* list = menu_group_->child_0x08;
    state.item_count = static_cast<int>(list->count());
    state.current = static_cast<int>(list->GetCurrentItemNumber());

    const hp2::frontend::MenuThing_00169740* current = list->current_0x08;
    for (hp2::frontend::MenuThing_00169740* item : list->items()) {
        MenuItem entry;
        entry.name_hash = item->name_hash_0x0C;
        entry.menuitem_index = static_cast<int>(item->index_0x10);
        entry.state = item->state_0x08;
        entry.enabled = item->enabled();
        entry.highlighted = item->highlighted();
        entry.current = item == current;
        state.items.push_back(entry);
    }
    state.current_name_hash = current == nullptr ? 0 : current->name_hash_0x0C;
    if (current != nullptr)
        state.current_name = "MENUITEM_" + std::to_string(current->index_0x10);
    state.activated_name_hash = activated_name_hash_;

    const auto& sound = hp2::frontend::last_menu_sound_request();
    state.sound_id = sound.sound_id;
    if (sound.name != nullptr) state.sound_name = sound.name;
    state.messages = message_log_;
    return state;
}


WidgetSizes::Redirect FrontEndRuntime::ghost_redirect() const {
    // The highlight ghost acquires its target's font, string and transform on every tick
    // (FUN_001410A0), so it must be measured as its target, not as the authored template.
    return [this](const hp2::frontend::FeWidget& widget) -> const hp2::frontend::FeWidget* {
        if(selection_animation_.screen_0x00 && widget.name_hash==selection_animation_.template_hash_0x0C)
            return hp2::frontend::FindWidget_00143D50(*selection_animation_.screen_0x00,selection_animation_.target_hash_0x04,1);
        if (highlight_animation_.screen_0x00 != nullptr &&
            widget.name_hash == highlight_animation_.template_hash_0x0C) {
            return hp2::frontend::FindWidget_00143D50(*highlight_animation_.screen_0x00,
                                                      highlight_animation_.target_hash_0x04, 1);
        }
        return &widget;
    };
}

hp2::frontend::WidgetMetrics FrontEndRuntime::measure_widget(const hp2::frontend::FeWidget& widget) const {
    // Measured from the original font tables and TexturePack entries, with the same ghost
    // redirect the renderer uses, so the recovered arithmetic and the renderer never disagree.
    // There is no host measurement round trip any more.
    const auto* screen = menu_group_ ? menu_group_->screen_0x14 : highlight_animation_.screen_0x00;
    if (screen == nullptr || !catalog_) return {};
    return WidgetSizes(*catalog_, fonts_, ghost_redirect()).scaled(widget);
}

void FrontEndRuntime::open_menu_animation(int screen_index) {
    highlight_animation_ = {};
    selection_animation_ = {};
    menu_clock_seconds_ = 0;
    if(audio_screen_ || display_screen_ || hud_screen_ || name_entry_) {
        auto& screen=screens_[static_cast<std::size_t>(screen_index)];
        highlight_animation_.Construct_00248870(&screen,0xab671c9a,hp2::frontend::kAnimGhostColour_gp_0x3F08,0);
        selection_animation_.Construct_00248870(&screen,0x1042eac3,0xffffffff,0);
        return;
    }
    // E-0016-V01: Title also binds ANIMATING_MENUITEM/PRESS_START_1, retaining default 4s period.
    // Screens 1 and 2 bind it the same way: FUN_00216E80 and FUN_00217538 each call
    // FUN_00248870(instance+0x23C, document, ANIMATING_MENUITEM, gp-0x3F08). (E-0016-08)
    if (screen_index != 0 && screen_index != 1 && screen_index != 2 && screen_index != 22) return;
    highlight_animation_.Construct_00248870(&screens_[static_cast<std::size_t>(screen_index)],
        hp2::frontend::bStringHash("ANIMATING_MENUITEM"),
        hp2::frontend::kAnimGhostColour_gp_0x3F08, 0);
    // Correction (E-0016-08): the 2 s period belongs to the manager-level object FUN_0016FBB0
    // builds at manager+0x1B8, which binds GLOBAL_ANIMATING_MENUITEM against the LOADING_TRACK
    // document. Every screen-owned instance -- FUN_00216460 at +0x1A8 for New Main Menu and
    // FUN_00216E80/FUN_00217538 at +0x23C for the two mode selects -- goes through FUN_00248870
    // without overriding +0x1C, so it keeps the default 4 s.
    if (screen_index == 22) highlight_animation_.Bind_002488D0(0xB070DBC9, 0);
}

void FrontEndRuntime::bind_menu_animation(std::uint32_t name_hash) {
    if (highlight_animation_.screen_0x00 == nullptr) return;
    highlight_animation_.Bind_002488D0(name_hash, static_cast<float>(menu_clock_seconds_));
    if(menu_viewport_)return; //001F4E68 bars were already placed using viewport slot.
    char mask[32];
    std::snprintf(mask, sizeof(mask), hp2::frontend::kMaskNameFormat_002BEFA0,
                  hp2::frontend::MenuItemIndexFromNameHash_0016A330(name_hash));
    if(hud_screen_ || name_entry_) {
        hp2::frontend::HandleHighlight(*menu_group_->screen_0x14,hp2::frontend::bStringHash(mask),
            [this](const auto& w){return measure_widget(w);});return;
    }
    hp2::frontend::PlaceSelectionBars_00216880(*highlight_animation_.screen_0x00,
        hp2::frontend::bStringHash(mask), [this](const auto& w) { return measure_widget(w); });
}

void FrontEndRuntime::bind_scrolling_text(std::uint32_t name_hash) {
    if (!scrolling_text_ || !language_) return;
    const auto binding = port::frontend::hint_binding(active_class_name_, name_hash);
    if (!binding.attachment_known)
        hp2::not_implemented("0x002491B0", "screen scrolling-text attachment not recovered");
    std::string text;
    if (!language_->lookup(binding.string_id, text)) text = "NOT TRANSLATED PROPERLY";
    scrolling_text_->Bind_00249518(text);
}

TickResult FrontEndRuntime::menu_tick(double delta) {
    TickResult result;
    if (!std::isfinite(delta) || delta < 0) {
        result.error = "menu delta must be finite and nonnegative";
        return result;
    }
    menu_clock_seconds_ += delta;
    sound_clock_seconds_ += delta;
    if(selection_ && selection_->observation().active){result.not_implemented=hp2::guard([&]{selection_->tick(static_cast<float>(delta));});result.ok=true;result.screen=menu_screen_index_;return result;}
    if(autosave_) {
        result.not_implemented=hp2::guard([&] {
            const bool complete=hp2::frontend::Sub_00246138(*autosave_,delta,[this]{
                return hp2::platform::store_profile(profile_search_start_,hp2::memorycard::kRegionNtsc_001A3390,profile_);});
            if(complete) {
                request_history_back();apply_pending_screen(); //00247C68 ->00170320(1)
            } else apply_save_popup();
        });
        result.ok=true;result.screen=menu_screen_index_;
        return result;
    }
    if(jukebox_screen_) {
        result.not_implemented=hp2::guard([&]{int sound=0;dispatch_jukebox(18,sound);});
        if(!result.not_implemented.empty()){result.ok=true;result.screen=menu_screen_index_;return result;}
    }
    if(audio_screen_)hp2::frontend::Sub_0016DE30(*audio_screen_,screens_[static_cast<std::size_t>(menu_screen_index_)],static_cast<float>(delta),
        [this](const auto& w){return measure_widget(w);},[this](auto id){return event_localize(id);});
    update_hud_arrows(static_cast<float>(delta));
    if(name_entry_)hp2::frontend::Sub_0024CA60(*name_entry_,screens_[static_cast<std::size_t>(menu_screen_index_)],
        static_cast<float>(delta),[this](const auto& w){return measure_widget(w);},[this](auto id){return event_localize(id);});
    if (menu_screen_index_ == kEventTreeScreen) {
        event_tree_.tick(static_cast<float>(delta));
        // The original mutates live profile globals during transitions.
        profile_.write_u32(hp2::memorycard::kEventTreeViewState_001FAF60,
            event_tree_.profile().EventTreeViewState_001FAF60());
        if(event_tree_.state()==0 || event_tree_.state()==2)profile_.write_u32(hp2::memorycard::kEventTreeViewState_001FAF60,event_tree_.state());
        update_event_tree_overlay(static_cast<float>(delta));
        const auto ticks = static_cast<std::uint32_t>(std::fmod(menu_clock_seconds_ * 4000, 4294967296.0));
        event_car_logos_.Sub_001F8FB0(ticks, random_state_);
        event_track_logos_.Sub_001F9318(ticks);
    }
    // FUN_001FA698 runs the overlay visibility test every frame, not once on open, because the
    // view state changes under it. (E-0021-28)
    apply_event_tree_widgets();
    if (scrolling_text_) scrolling_text_->Update_00249348(static_cast<float>(delta));
    if (menu_viewport_) menu_viewport_->Update_0024A1A8(static_cast<float>(delta));
    if (menu_selector_) menu_selector_->Update_0024A730(static_cast<float>(delta),
        [this](const auto& w) { return measure_widget(w); });
    // E-0091-G02: table starts002FAEA0 (name,enter,exit,tick). State3 is
    // GAMEFLOW_STATE_IN_FRONTEND; the earlier entry-field-based naming was shifted.
    background_animation_.Update_00249AE8(static_cast<float>(delta),
        [this](int bound){return hp2::frontend::bRandom(bound,random_state_);});
    if (exit_animation_ && exit_animation_->Update_00249128(static_cast<float>(delta))) {
        exit_animation_.reset();
        pending_class_name_ = exit_class_name_;
        pending_push_history_ = true;
        exit_class_name_.clear();
        result.not_implemented = hp2::guard([&] { apply_pending_screen(); });
    }
    if (highlight_animation_.active() && (menu_screen_index_ != 22 || !title_movie_state_)) {
        highlight_animation_.Update_00248970(static_cast<float>(menu_clock_seconds_),
                                             [this](const auto& w) { return measure_widget(w); });
    }
    if(selection_animation_.active())selection_animation_.Update_00248970(static_cast<float>(menu_clock_seconds_),
        [this](const auto& w){return measure_widget(w);});
    result.ok = true;
    result.screen = menu_screen_index_;
    return result;
}

std::vector<SoundRequest> FrontEndRuntime::drain_menu_audio() {
    auto result=std::move(pending_sound_controls_);pending_sound_controls_.clear();
    auto requests = std::move(pending_volume_feedback_);
    pending_volume_feedback_.clear();
    for(auto& name:pending_sound_names_)requests.emplace_back(std::move(name),1.0f);
    pending_sound_names_.clear();
    for (const auto& entry : requests) {
        const auto& name=entry.first;
        SoundRequest request;
        request.event = name;
        try {
            if (!sound_bank_) throw std::runtime_error("original sound bank not loaded");
            request.pcm = sound_bank_->decode(name);
            request.pitch_scale = hp2::port::audio::neutral_playback_pitch_scale(request.pcm.sample_rate);
            request.gain = request.pcm.nominal_gain * audio_value(audio_screen_ ? audio_screen_->snapshot : audio_options_,5)*entry.second;
        } catch (const std::exception& error) {
            request.not_implemented = error.what();
        }
        result.push_back(std::move(request));
    }
    return result;
}

MusicRequest FrontEndRuntime::frontend_music(bool previous_finished,bool start_requested) {
    MusicRequest result;
    try {
        if (!jukebox_ || !music_scheduler_) throw std::runtime_error("original jukebox not loaded");
        const auto ticks = static_cast<std::uint32_t>(std::fmod(sound_clock_seconds_ * 4000, 4294967296.0));
        std::optional<std::size_t> index;
        // E-0092-J08: completion belongs only to the currently playing device
        // request; stopped/queued replacement callbacks cannot select another tune.
        if(previous_finished && music_playing_ && pending_music_index_<0 && !pending_music_stop_) {
            music_playing_=false;
            if(music_auto_mode_)index=music_scheduler_->finished(ticks);
            else music_scheduler_->stop();
        }
        if(pending_music_index_>=0)index=static_cast<std::size_t>(pending_music_index_);
        else if(start_requested && !music_playing_ && !pending_music_stop_) {
            index=music_scheduler_->enter(ticks); // explicit001AEAB0 call, never a device poll
            if(index)music_auto_mode_=true;
        }
        result.stop=pending_music_stop_;
        result.gain=audio_value(audio_screen_ ? audio_screen_->snapshot : audio_options_,0); //category5,001EEF60
        const auto& output=audio_screen_ ? audio_screen_->snapshot : audio_options_;
        result.output_mode=output[0xac]|(int(output[0xad])<<8);
        if (index) {
            result.pcm = jukebox_->decode(*index);
            result.pitch_scale = hp2::port::audio::neutral_playback_pitch_scale(result.pcm.sample_rate);
            result.index = static_cast<int>(*index);
            result.name = jukebox_->tunes()[*index].name;
            result.has_track = true;
            music_playing_=true;
        }
        pending_music_stop_=false;pending_music_index_=-1;
        result.ok = true;
    } catch (const std::exception& error) {
        result.not_implemented = error.what();
    }
    return result;
}

void FrontEndRuntime::title_visual_state(bool movie_state, bool can_start) {
    title_movie_state_ = movie_state;
    if (screens_.size() <= 22) return;
    // E-0016-V01: 0016F1F0 -> 001454E8 toggles TITLE_IMAGE class members; 00144968
    // controls PRESS_START_1 with the strict 0016F4F8 time gate. Movie widget slots are stopped.
    for (auto& widget : screens_[22].widgets) {
        if (widget.class_hash == 0xB73C8D23) widget.visible = movie_state ? 0 : 1;
        if (widget.name_hash == hp2::frontend::bStringHash("PRESS_START_1"))
            widget.visible = !movie_state && can_start ? 1 : 0;
        if (!movie_state && (widget.name_hash == 0x3ACF2D73 || widget.name_hash == 0xBFAEBF79))
            widget.visible = 0;
    }
}

void FrontEndRuntime::title_pulse_restart() {
    // 00248AD0 on skipping the second intro: resets pulse timestamp, preserves target.
    highlight_animation_.start_time_0x08 = static_cast<float>(menu_clock_seconds_);
}

void FrontEndRuntime::set_startup_presentation(bool movie_visible, bool letterbox,
                                               bool startup_active) {
    startup_movie_visible_ = movie_visible;
    startup_letterbox_ = letterbox;
    startup_active_ = startup_active;
}

FrameStats FrontEndRuntime::build_frame(render::Resources& resources, render::Frame& frame) {
    if (menu_screen_index_ < 0 || static_cast<std::size_t>(menu_screen_index_) >= screens_.size()) {
        frame.quads.clear();
        frame.frontend_ranges.clear();
        return {};
    }
    FrameOptions options;
    if (startup_movie_visible_) {
        // E-0016-V01: the intro owns the screen through one of the two authored title slots.
        options.exclusive_widget = startup_letterbox_ ? "TITLE_LETTERBOX" : "TITLE_FULLSCREEN";
        options.exclusive_texture = kIntroMovieTexture;
        const auto bitmap = resources.texture(kIntroMovieTexture);
        options.exclusive_width = bitmap ? static_cast<float>(bitmap->width) : 0.0f;
        options.exclusive_height = bitmap ? static_cast<float>(bitmap->height) : 0.0f;
    } else if (startup_active_) {
        options.suppressed_widget = "TITLE_LETTERBOX";
    }
    const WidgetSizes sizes(resources, fonts_, ghost_redirect());
    auto stats = port::runtime::build_frame(screens_[static_cast<std::size_t>(menu_screen_index_)],
                                            sizes, options, frame);
    if(autosave_ && save_popup_) {
        render::Frame addition;
        const auto drawn=port::runtime::build_frame(*save_popup_,sizes,{},addition);
        render::append_frontend_frame(frame,addition);
        stats.quads+=drawn.quads;stats.glyphs+=drawn.glyphs;stats.texts+=drawn.texts;
        stats.missing+=drawn.missing;stats.clipped_out+=drawn.clipped_out;
    }
    if (menu_screen_index_ == kEventTreeScreen && options.exclusive_widget.empty()) {
        // FUN_001F6FA0 takes every node's panel geometry from screen 39's own EVENT_PANEL
        // widget, so hand the authored record over instead of inventing a size.
        const hp2::frontend::FeWidget* panel = hp2::frontend::FindWidget_00143D50(
            screens_[static_cast<std::size_t>(menu_screen_index_)],
            hp2::frontend::bStringHash(hp2::frontend::kWidgetEventPanel_A3B23510), 0);
        const auto* number = hp2::frontend::FindWidget_00143D50(
            screens_[39], hp2::frontend::bStringHash("EVENT_NUMBER"), 1);
        const auto overlay = append_event_tree(event_tree_, sizes, resources, frame, panel, number,
                                               &screens_[39]);
        stats.quads += overlay.panels + overlay.connectors * 3 + overlay.missing_panels * 4;
        stats.glyphs += overlay.numbers;
        stats.missing += overlay.missing_panels;
        const int state = event_tree_.state();
        auto sibling = [&](int index, float alpha, bool wrapped) {
            auto document = screens_[static_cast<std::size_t>(index)];
            for (auto& widget : document.widgets) {
                widget.a = static_cast<std::uint8_t>(widget.a * alpha);
                if (widget.type == 2) for (auto& corner : widget.field_0x5c)
                    corner = (corner & 0xffffffu) |
                        (static_cast<std::uint32_t>(static_cast<std::uint8_t>((corner >> 24) * alpha)) << 24);
            }
            FrameOptions sibling_options;
            if (wrapped) {
                // 001451E0 explicitly draws this template even when authored visibility is 0.
                if (auto* paragraph = hp2::frontend::FindWidget_00143D50(document, 0x7a31036b, 1))
                    paragraph->visible = 1;
                sibling_options.wrapped_text_hash = 0x7a31036b;
                const auto box = hp2::frontend::GetWidgetRect_00144280(document, 0x7ad2801e,
                    sizes.measure());
                sibling_options.wrapped_text_bounds = {box.x0, box.y0, box.x1 - box.x0, box.y1 - box.y0};
            }
            render::Frame addition;
            const auto drawn = port::runtime::build_frame(document, sizes, sibling_options, addition);
            render::append_frontend_frame(frame,addition);
            stats.quads += drawn.quads; stats.glyphs += drawn.glyphs; stats.texts += drawn.texts;
            stats.missing += drawn.missing; stats.clipped_out += drawn.clipped_out;
        };
        // FUN_001FA458 / E-0022-T02: document draw predicates and exact fade operands.
        if ((state >= 1 && state <= 4) || state == 6) sibling(17, 1 - event_tree_.alpha(), true);
        if (state >= 4 && state <= 6) sibling(19, event_tree_.alpha(), false);
        if (state == 7) sibling(18, event_tree_.alpha(), true);
    }
    // TASK-0094 port display-device policy: four horizontal GS clock units per
    // host logical pixel, one vertical unit per logical pixel. Original raw
    // offsets stay in the profile; this is not a GS scanout equivalence claim.
    frame.display_offset={static_cast<float>(profile_.read_i32(0x144))*0.25f,
                          static_cast<float>(profile_.read_i32(0x148))};
    return stats;
}

FrontEndRuntime::ProbeResult FrontEndRuntime::probe_not_implemented(const std::string& which) {
    ProbeResult result;
    if (which != "popup" && which != "highlight_animation" && which != "localised_text") {
        result.error = "unknown probe: " + which;
        return result;
    }
    result.not_implemented = hp2::guard([&] {
        if (which == "popup") hp2::frontend::Sub_0016BAC0(0, hp2::input::JOY_EVENT_MENU_SELECT, true);
        else if (which == "highlight_animation") hp2::frontend::Sub_0016FBB0();
        else hp2::frontend::localised_text(0);
    });
    if (result.not_implemented.empty()) result.error = "probe did not raise hp2::NotImplemented";
    return result;
}

} // namespace port::runtime
