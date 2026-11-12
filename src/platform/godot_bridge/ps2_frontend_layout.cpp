#include "event_reward_transport.hpp"
#include "event_settings_transport.hpp"
#include "event_score_transport.hpp"
#include "event_profile_transport.hpp"
#include "ps2_frontend_layout.hpp"
#include "boundary.hpp"
#include <cstring>
#include <algorithm>
#include "port/runtime/frame_builder.hpp"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
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
    godot::Dictionary selection;selection["active"]=state.selection.active;selection["loaded"]=state.selection.loaded;selection["car"]=state.selection.car;selection["skin"]=state.selection.skin;selection["overlay"]=state.selection.overlay;selection["visible_cars"]=state.selection.visible_cars;selection["generation"]=static_cast<int64_t>(state.selection.generation);selection["load_ms"]=state.selection.load_ms;selection["cache_hits"]=int64_t(state.selection.cache_hits);result["selection"]=selection;
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
    godot::ClassDB::bind_method(godot::D_METHOD("begin_loading_screen","tree_type","transmission","language"),&Ps2FrontEndLayout::begin_loading_screen);
    godot::ClassDB::bind_method(godot::D_METHOD("tick_loading_screen","elapsed"),&Ps2FrontEndLayout::tick_loading_screen);
    godot::ClassDB::bind_method(godot::D_METHOD("end_loading_screen"),&Ps2FrontEndLayout::end_loading_screen);
    godot::ClassDB::bind_method(godot::D_METHOD("take_event_launch"),&Ps2FrontEndLayout::take_event_launch);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_event","result"),&Ps2FrontEndLayout::complete_event);
    godot::ClassDB::bind_method(godot::D_METHOD("continue_event_attempt","previous_generation","require_saved_outcome"),&Ps2FrontEndLayout::continue_event_attempt);
    godot::ClassDB::bind_method(godot::D_METHOD("return_from_event","random_state"),&Ps2FrontEndLayout::return_from_event,DEFVAL(-1));
    godot::ClassDB::bind_method(godot::D_METHOD("ui_number_reference","number"),&Ps2FrontEndLayout::ui_number_reference);
    godot::ClassDB::bind_method(godot::D_METHOD("ui_snapshot"),&Ps2FrontEndLayout::ui_snapshot);
    godot::ClassDB::bind_method(godot::D_METHOD("get_background_movie"),&Ps2FrontEndLayout::get_background_movie);
    godot::ClassDB::bind_method(godot::D_METHOD("ui_action","action"),&Ps2FrontEndLayout::ui_action);
    godot::ClassDB::bind_method(godot::D_METHOD("ui_texture","id"),&Ps2FrontEndLayout::ui_texture);
    godot::ClassDB::bind_method(godot::D_METHOD("ui_fonts"),&Ps2FrontEndLayout::ui_fonts);
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

godot::Dictionary Ps2FrontEndLayout::take_event_launch(){
    godot::Dictionary out;const auto failure=catch_all([&]{const auto request=runtime_.take_event_launch();out["ok"]=true;out["pending"]=bool(request);if(!request)return;
        const auto& r=*request;out["generation"]=int64_t(r.generation);out["tree_type"]=r.tree_type;out["event_index"]=r.event_index;out["event_number"]=r.event_number;
        out["car_id"]=r.car_id;out["skin_id"]=r.skin_id;out["language"]=r.language;out["track"]=r.track;out["reverse"]=r.reverse;
        out["driving_style"]=r.driving_style;out["transmission"]=r.transmission;out["preferred_gender"]=r.preferred_gender;
        out["has_dash"]=r.has_dash;out["vertex_damage"]=r.vertex_damage;out["player_points"]=int64_t(r.player_points);out["random_state"]=int64_t(r.random_state);
        godot::PackedByteArray progress;progress.resize(r.event_progress.size());if(!r.event_progress.empty())std::memcpy(progress.ptrw(),r.event_progress.data(),r.event_progress.size());out["event_progress"]=progress;
        godot::PackedInt32Array offsets;for(const auto at:r.event_progress_offsets)offsets.push_back(at);out["event_progress_offsets"]=offsets;if(r.event_profile)out["event_profile"]=profile_transfer(*r.event_profile);if(r.prepared_event)out["prepared_event"]=prepared_transfer(*r.prepared_event);if(r.event_scores)out["event_scores"]=score_transfer(*r.event_scores);if(r.event_settings)out["event_settings"]=settings_transfer(*r.event_settings);if(r.event_rewards)out["event_rewards"]=reward_transfer(*r.event_rewards);
    });if(!failure.empty())return boundary_failure(failure);return out;
}
godot::Dictionary Ps2FrontEndLayout::complete_event(const godot::Dictionary& result){
    const auto failure=catch_all([&]{port::race::EventSettlement r;r.generation=int64_t(result.get("generation",0));r.tree_type=result.get("tree_type",-1);r.event_index=result.get("event_index",-1);r.event_number=result.get("event_number",-1);
        if(!result.has("random_state"))throw std::invalid_argument("Event settlement lacks its original source RNG state");
        r.final_reward_awarded=result.get("final_reward_awarded",false);r.aborted=result.get("aborted",false);r.player_points=int64_t(result.get("player_points",0));r.random_state=int64_t(result["random_state"]);const godot::PackedByteArray progress=result.get("event_progress",godot::PackedByteArray());if(progress.size())r.event_progress.assign(progress.ptr(),progress.ptr()+progress.size());if(result.has("event_profile"))r.event_profile=profile_blocks(godot::PackedByteArray(result["event_profile"]));if(result.has("event_scores"))r.event_scores=score_profile(godot::Dictionary(result["event_scores"]));if(result.has("event_settings"))r.event_settings=settings_transfer(godot::Dictionary(result["event_settings"]));if(result.has("event_rewards"))r.event_rewards=reward_transfer(godot::PackedByteArray(result["event_rewards"]));runtime_.complete_event(r);
    });if(!failure.empty())return boundary_failure(failure);godot::Dictionary out;out["ok"]=true;return out;
}
godot::Dictionary Ps2FrontEndLayout::continue_event_attempt(int64_t previous_generation,bool require_saved_outcome){
 godot::Dictionary out;const auto failure=catch_all([&]{if(previous_generation<=0)throw std::invalid_argument("Authored continuation requires a positive previous generation");const auto r=runtime_.continue_event_attempt(std::uint64_t(previous_generation),require_saved_outcome);out["ok"]=true;out["storage_baseline_only"]=true;out["previous_generation"]=int64_t(r.previous_generation);out["generation"]=int64_t(r.generation);out["tree_type"]=r.tree_type;out["event_index"]=r.event_index;out["event_number"]=r.event_number;out["player_points"]=int64_t(r.player_points);out["random_state"]=int64_t(r.random_state);godot::PackedByteArray progress;progress.resize(r.event_progress.size());if(!r.event_progress.empty())std::memcpy(progress.ptrw(),r.event_progress.data(),r.event_progress.size());out["event_progress"]=progress;if(r.event_profile)out["event_profile"]=profile_transfer(*r.event_profile);if(r.event_scores)out["event_scores"]=score_transfer(*r.event_scores);if(r.event_settings)out["event_settings"]=settings_transfer(*r.event_settings);if(r.event_rewards)out["event_rewards"]=reward_transfer(*r.event_rewards);});if(!failure.empty())return boundary_failure(failure);return out;
}

godot::Dictionary Ps2FrontEndLayout::return_from_event(int64_t random_state){const auto failure=catch_all([&]{if(random_state< -1||random_state>0xffffffffll)throw std::invalid_argument("World RNG state is outside the original uint32 transport domain");std::optional<std::uint32_t> seed;if(random_state>=0)seed=static_cast<std::uint32_t>(random_state);runtime_.return_from_event(seed);});if(!failure.empty())return boundary_failure(failure);auto state=runtime_.menu_state();state.ok=true;return to_dictionary(state);}

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
    godot::Dictionary result;
    const auto failure=catch_all([&]{
        auto state = runtime_.menu_state();state.ok=true;result=to_dictionary(state);result.erase("ok");
        const auto& view=runtime_.event_tree();
        if(state.active_class=="View Event Tree"&&view.loaded()){
            godot::Dictionary tree;tree["tree_type"]=view.tree_type();tree["cursor_index"]=view.cursor();tree["state"]=view.state();tree["transitioning"]=view.transitioning();
            godot::Array directions;for(int direction:{-1,1,2,-2})directions.append(direction);tree["navigation_directions"]=directions;
            godot::Array nodes;const auto* source=view.selected_tree();
            for(const auto& node:view.nodes()){
                godot::Dictionary row;row["index"]=node.index;row["number"]=node.number;row["name"]=to_godot(node.name);row["status"]=node.status;row["visibility"]=node.visibility;row["selected"]=node.selected;row["x"]=node.x;row["y"]=node.y;
                row["event_type"]=int(source->records.at(node.index).goal_type_0x29);
                godot::Array links,candidates;for(int link:node.links)links.append(link);
                // Pure source navigation candidates; visibility/admission still
                // belongs to the actual UI. This query never moves its cursor.
                for(int direction:{-1,1,2,-2})candidates.append(source->MoveCursor_001FA9C8(node.index,direction));
                row["links"]=links;row["raw_navigation_candidates"]=candidates;nodes.append(row);
            }
            tree["nodes"]=nodes;result["event_tree"]=tree;
        }
    });
    if(!failure.empty())return boundary_failure(failure);
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


godot::Array Ps2FrontEndLayout::ui_number_reference(int number){
    godot::Array out;const auto failure=catch_all([&]{
        if(number<1||number>32)throw std::logic_error("number reference outside1..32");
        const auto snap=runtime_.ui_snapshot();
        const auto item=std::find_if(snap.widgets.begin(),snap.widgets.end(),[](const auto& w){return w.id.rfind("event:",0)==0;});
        if(item==snap.widgets.end())throw std::logic_error("number reference requires actual event page");
        auto widget=item->widget;widget.text=number<10?" "+std::to_string(number):std::to_string(number);
        hp2::frontend::FeScreen screen;screen.widgets.push_back(widget);
        port::runtime::WidgetSizes sizes(*runtime_.catalog(),runtime_.fonts());port::render::Frame frame;
        port::runtime::build_frame(screen,sizes,{},frame);const auto* font=sizes.font(widget.texture_hash);
        for(const auto& q:frame.quads){
            const auto glyph=std::find_if(font->glyphs.begin(),font->glyphs.end(),[&](const auto& g){return q.source.x==g.u+.5f&&q.source.y==g.v+.5f&&q.source.width==g.w&&q.source.height==g.h;});
            if(glyph==font->glyphs.end())throw std::logic_error("reference glyph missing");
            godot::Dictionary g;g["code"]=glyph->code;g["bounds"]=godot::Rect2(q.destination.x,q.destination.y,q.destination.width,q.destination.height);
            g["source"]=godot::Rect2(q.source.x,q.source.y,q.source.width,q.source.height);out.append(g);
        }
    });if(!failure.empty())godot::UtilityFunctions::push_error(godot::String::utf8(failure.c_str()));return out;
}

godot::String Ps2FrontEndLayout::get_background_movie() const {
    return to_godot(runtime_.background_movie());
}

godot::Dictionary Ps2FrontEndLayout::ui_snapshot(){
    godot::Dictionary out;const auto failure=catch_all([&]{
        const auto snap=runtime_.ui_snapshot();out["generation"]=int64_t(snap.generation);out["page"]=to_godot(snap.page);out["modal"]=snap.modal;out["selection"]=snap.selection;
        out["display_offset"]=godot::Vector2(snap.display_offset.x,snap.display_offset.y);
        out["event_clip"]=godot::Rect2(snap.event_clip.x,snap.event_clip.y,snap.event_clip.width,snap.event_clip.height);
        godot::Array widgets;
        for(const auto& item:snap.widgets){const auto& w=item.widget;godot::Dictionary d;
            d["id"]=to_godot(item.id);d["kind"]=to_godot(item.kind);d["port"]=item.input_port;d["value"]=item.value;d["handle_texture"]=int64_t(item.handle_texture);d["hash"]=int64_t(w.name_hash);d["name"]=to_godot(w.name);d["type"]=w.type;d["layer"]=w.layer;d["stack"]=item.stack;
            d["texture"]=int64_t(w.texture_hash);// FontFile glyph IDs are original byte codes, not UTF-8 text.
            d["text"]=godot::String(w.text.c_str());d["align"]=int64_t(w.text_align);
            d["bounds"]=godot::Rect2(item.bounds.x,item.bounds.y,item.bounds.width,item.bounds.height);
            d["clip"]=godot::Rect2(item.clip.x,item.clip.y,item.clip.width,item.clip.height);
            d["text_bounds"]=godot::Rect2(item.text_bounds.x,item.text_bounds.y,item.text_bounds.width,item.text_bounds.height);
            godot::Array glyphs;for(const auto& glyph:item.glyphs){godot::Dictionary g;
                g["code"]=glyph.code;g["bounds"]=godot::Rect2(glyph.bounds.x,glyph.bounds.y,glyph.bounds.width,glyph.bounds.height);
                g["source"]=godot::Rect2(glyph.source.x,glyph.source.y,glyph.source.width,glyph.source.height);glyphs.append(g);}d["glyphs"]=glyphs;
            d["wrap"]=item.wrap;d["scale"]=godot::Vector2(w.scale_x,w.scale_y);d["interactive"]=item.interactive;d["enabled"]=item.enabled;d["selected"]=item.selected;
            d["color"]=godot::Color(w.r/128.f,w.g/128.f,w.b/128.f,w.a/128.f);godot::PackedColorArray colours;colours.push_back(godot::Color(w.r/128.f,w.g/128.f,w.b/128.f,w.a/128.f));
            for(auto c:w.field_0x5c)colours.push_back(godot::Color((c&255)/128.f,((c>>8)&255)/128.f,((c>>16)&255)/128.f,(c>>24)/128.f));d["colors"]=colours;
            godot::PackedVector2Array uv;for(int i=0;i<4;++i)uv.push_back({w.field_0x70[i*2],w.field_0x70[i*2+1]});d["uv"]=uv;
            widgets.append(d);
        }out["widgets"]=widgets;out["ok"]=true;
    });if(!failure.empty())return boundary_failure(failure);return out;
}
godot::Dictionary Ps2FrontEndLayout::ui_action(const godot::Dictionary& action){
    port::runtime::MenuState result;const auto failure=catch_all([&]{port::runtime::FrontendUiAction a;
        a.generation=int64_t(action.get("generation",-1));a.id=godot::String(action.get("id","")).utf8().get_data();a.port=action.get("port",0);a.direction=action.get("direction",0);a.value=action.get("value",0.f);a.text=godot::String(action.get("text","")).utf8().get_data();
        const godot::String kind=action.get("kind","activate");
        if(kind=="edit_text")a.kind=decltype(a.kind)::EditText;else if(kind=="set_value")a.kind=decltype(a.kind)::SetValue;else if(kind=="focus")a.kind=decltype(a.kind)::Focus;else if(kind=="back")a.kind=decltype(a.kind)::Back;else if(kind=="adjust")a.kind=decltype(a.kind)::Adjust;
        else if(kind!="activate"){result.error="Unknown frontend UI action";return;}
        result=runtime_.ui_action(a);
    });if(!failure.empty())return boundary_failure(failure);return to_dictionary(result);
}
godot::Ref<godot::Texture2D> Ps2FrontEndLayout::ui_texture(int64_t id){
    auto found=ui_textures_.find(id);if(found!=ui_textures_.end())return found->second;
    godot::Ref<godot::ImageTexture> texture;const auto failure=catch_all([&]{auto* catalog=runtime_.catalog();if(!catalog)return;auto b=catalog->texture(id);if(!b)return;
        godot::PackedByteArray data;data.resize(b->rgba.size());std::memcpy(data.ptrw(),b->rgba.data(),b->rgba.size());
        auto image=godot::Image::create_from_data(b->width,b->height,false,godot::Image::FORMAT_RGBA8,data);texture=godot::ImageTexture::create_from_image(image);ui_textures_.emplace(id,texture);
    });if(!failure.empty())godot::UtilityFunctions::push_error(to_godot(failure));return texture;
}
godot::Array Ps2FrontEndLayout::ui_fonts()const{
    godot::Array out;for(const auto& font:runtime_.fonts()){godot::Dictionary f;f["texture"]=int64_t(font.texture_hash);f["height"]=int64_t(font.line_height);godot::Array glyphs,kern;
        for(const auto& g:font.glyphs){godot::Dictionary d;d["code"]=g.code;d["uv"]=godot::Rect2(g.u,g.v,g.w,g.h);d["advance"]=g.advance;glyphs.append(d);
            const auto count=static_cast<int16_t>(g.flags);for(int i=0;i<count && g.field_0x1c+std::size_t(i)<font.field_0x410.size();++i){const auto& pair=font.field_0x410[g.field_0x1c+i];godot::Dictionary k;k["first"]=pair.first;k["second"]=g.code;k["amount"]=pair.second;kern.append(k);}}
        f["glyphs"]=glyphs;f["kerning"]=kern;out.append(f);
    }return out;
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

namespace port::godot_bridge {
namespace {
void loading_status(godot::Dictionary& d,const port::runtime::LoadingPresentation& source){d["active"]=source.active();d["screen_hash"]=int64_t(source.screen_hash());d["revision"]=int64_t(source.revision());godot::Array clip;for(auto v:source.clip_bounds())clip.append(v);d["clip_bounds"]=clip;d["presentation_gameflow_stage"]=4;d["progress_available"]=false;}
}
godot::Dictionary Ps2FrontEndLayout::begin_loading_screen(int tree,int transmission,int language){godot::Dictionary d;const auto error=catch_all([&]{if(tree< -1||tree>1)throw std::invalid_argument("Loading tree must be a reviewed event tree or FreeRun");if(loading_)loading_->close();loading_=std::make_unique<port::runtime::LoadingPresentation>(search_start_dir(),language);loading_->begin(tree<0?6:tree==0?8:9,transmission);loading_status(d,*loading_);});d["ok"]=error.empty();if(!error.empty())d["error"]=to_godot(error);return d;}
godot::Dictionary Ps2FrontEndLayout::tick_loading_screen(double elapsed){godot::Dictionary d;const auto error=catch_all([&]{if(!loading_)throw std::logic_error("Original loading presentation not initialized");loading_->tick(elapsed);loading_status(d,*loading_);});d["ok"]=error.empty();if(!error.empty())d["error"]=to_godot(error);return d;}
godot::Dictionary Ps2FrontEndLayout::end_loading_screen(){godot::Dictionary d;const auto error=catch_all([&]{if(loading_){loading_->end();loading_status(d,*loading_);}});d["ok"]=error.empty();if(!error.empty())d["error"]=to_godot(error);return d;}
}
