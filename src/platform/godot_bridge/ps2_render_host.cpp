#include "ps2_render_host.hpp"
#include "boundary.hpp"
#include "game_core/port/not_implemented.hpp"
#include "port/runtime/frame_builder.hpp"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <cstring>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

namespace port::godot_bridge {
namespace {

godot::Rect2 to_rect(const port::render::Rect& rect) {
    return godot::Rect2(rect.x, rect.y, rect.width, rect.height);
}

godot::Color to_color(const port::render::Color& color) {
    return godot::Color(color.r, color.g, color.b, color.a);
}

godot::Ref<godot::Image> to_image(const port::render::Bitmap& bitmap) {
    godot::PackedByteArray pixels;
    pixels.resize(static_cast<int64_t>(bitmap.rgba.size()));
    if (!bitmap.rgba.empty())
        std::memcpy(pixels.ptrw(), bitmap.rgba.data(), bitmap.rgba.size());
    return godot::Image::create_from_data(bitmap.width, bitmap.height, false,
                                          godot::Image::FORMAT_RGBA8, pixels);
}

} // namespace

std::shared_ptr<const port::render::Bitmap> LiveResources::texture(port::render::TextureId id) {
    if (id == port::runtime::kIntroMovieTexture) return intro_;
    if (id == port::runtime::kBackgroundMovieTexture) return background_;
    return catalog_ == nullptr ? nullptr : catalog_->texture(id);
}

void Ps2RenderHost::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("console_visible"), &Ps2RenderHost::console_visible);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_ui_snapshot","snapshot"),&Ps2RenderHost::viewer_ui_snapshot);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_ui_next"),&Ps2RenderHost::viewer_ui_next);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_ui_capture"),&Ps2RenderHost::viewer_ui_capture);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_request", "action", "source"), &Ps2RenderHost::viewer_request);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_next"), &Ps2RenderHost::viewer_next);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_activate", "kind"), &Ps2RenderHost::viewer_activate);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_apply", "action", "source"), &Ps2RenderHost::viewer_apply);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_state"), &Ps2RenderHost::viewer_state);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_camera", "description"), &Ps2RenderHost::viewer_camera);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_loaded", "success", "diagnostic", "rendering"), &Ps2RenderHost::viewer_loaded);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_rendering", "description"), &Ps2RenderHost::viewer_rendering);
    godot::ClassDB::bind_method(godot::D_METHOD("is_debug_build"), &Ps2RenderHost::is_debug_build);
    godot::ClassDB::bind_method(godot::D_METHOD("console_available"), &Ps2RenderHost::console_available);
    godot::ClassDB::bind_method(godot::D_METHOD("console_init", "settings", "info"), &Ps2RenderHost::console_init);
    godot::ClassDB::bind_method(godot::D_METHOD("console_input", "event"), &Ps2RenderHost::console_input);
    godot::ClassDB::bind_method(godot::D_METHOD("console_frame", "width", "height", "delta", "status"), &Ps2RenderHost::console_frame);
    godot::ClassDB::bind_method(godot::D_METHOD("console_log", "text", "error"), &Ps2RenderHost::console_log);
    godot::ClassDB::bind_method(godot::D_METHOD("console_execute", "command"), &Ps2RenderHost::console_execute);
    godot::ClassDB::bind_method(godot::D_METHOD("console_focus_lost"), &Ps2RenderHost::console_focus_lost);
    godot::ClassDB::bind_method(godot::D_METHOD("attach", "parent_canvas_item"),
                                &Ps2RenderHost::attach);
    godot::ClassDB::bind_method(godot::D_METHOD("set_output_scale", "scale"),
                                &Ps2RenderHost::set_output_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_output_scale"),
                                &Ps2RenderHost::get_output_scale);
    godot::ClassDB::bind_method(
        godot::D_METHOD("draw", "runtime", "intro_movie", "background_movie"),
        &Ps2RenderHost::draw);
    godot::ClassDB::bind_method(godot::D_METHOD("release"), &Ps2RenderHost::release);
    godot::ClassDB::bind_method(godot::D_METHOD("get_texture_count"),
                                &Ps2RenderHost::get_texture_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_error"), &Ps2RenderHost::get_last_error);
}

Ps2RenderHost::~Ps2RenderHost() { release(); }

void Ps2RenderHost::attach(const godot::RID& parent_canvas_item) {
    auto* server = godot::RenderingServer::get_singleton();
    if (server == nullptr) {
        last_error_ = "RenderingServer unavailable";
        return;
    }
    if (!item_.is_valid()) item_ = server->canvas_item_create();
    server->canvas_item_set_parent(item_, parent_canvas_item);
    if (!console_parent_.is_valid()) console_parent_ = server->canvas_item_create();
    server->canvas_item_set_parent(console_parent_, parent_canvas_item);
    server->canvas_item_set_z_index(console_parent_, 100);
    last_error_ = "";
}

void Ps2RenderHost::set_output_scale(double scale) {
    output_scale_ = scale > 0 ? scale : 1.0;
}

void Ps2RenderHost::release() {
#if HP2_DEBUG_BUILD
    console_.reset();
#endif
    auto* server = godot::RenderingServer::get_singleton();
    if (server != nullptr) {
        for (auto rid : console_draw_items_) if(rid.is_valid()) server->free_rid(rid);
        for (auto rid : console_items_) if(rid.is_valid()) server->free_rid(rid);
        if(console_font_.is_valid()) server->free_rid(console_font_);
        if(console_parent_.is_valid()) server->free_rid(console_parent_);
    }
    console_items_.clear(); console_draw_items_.clear(); console_font_={};console_parent_={};
    if (server == nullptr) { textures_.clear(); item_ = godot::RID(); return; }
    for (auto& entry : textures_)
        if (entry.second.rid.is_valid()) server->free_rid(entry.second.rid);
    textures_.clear();
    if (item_.is_valid()) server->free_rid(item_);
    item_ = godot::RID();
}

godot::RID Ps2RenderHost::texture_for(port::render::TextureId id,
                                      port::render::Resources& resources) {
    const auto bitmap = resources.texture(id);
    if (!bitmap || bitmap->width <= 0 || bitmap->height <= 0) return godot::RID();
    auto* server = godot::RenderingServer::get_singleton();
    auto& entry = textures_[id];
    if (entry.rid.is_valid() && entry.width == bitmap->width && entry.height == bitmap->height) {
        // Original entries never change; the live movie surfaces bump their revision per frame.
        if (entry.revision != bitmap->revision) {
            server->texture_2d_update(entry.rid, to_image(*bitmap), 0);
            entry.revision = bitmap->revision;
        }
        return entry.rid;
    }
    if (entry.rid.is_valid()) server->free_rid(entry.rid);
    entry.rid = server->texture_2d_create(to_image(*bitmap));
    entry.width = bitmap->width;
    entry.height = bitmap->height;
    entry.revision = bitmap->revision;
    return entry.rid;
}

int Ps2RenderHost::submit(const port::render::Frame& frame, port::render::Resources& resources) {
    int movie_quads = 0;
    auto* server = godot::RenderingServer::get_singleton();
    server->canvas_item_clear(item_);
    server->canvas_item_set_transform(
        item_, godot::Transform2D().scaled(godot::Vector2(static_cast<real_t>(output_scale_),
                                                          static_cast<real_t>(output_scale_))));
    if (frame.clear.a > 0) {
        server->canvas_item_add_rect(
            item_, godot::Rect2(0, 0, frame.logical_size.x, frame.logical_size.y),
            to_color(frame.clear));
    }
    for (const auto& quad : frame.quads) {
        if (quad.texture == 0) { // solid-colour rect; see render::Quad
            server->canvas_item_add_rect(item_, to_rect(quad.destination), to_color(quad.color));
            continue;
        }
        if (quad.texture == port::runtime::kIntroMovieTexture ||
            quad.texture == port::runtime::kBackgroundMovieTexture) ++movie_quads;
        const auto rid = texture_for(quad.texture, resources);
        if (!rid.is_valid()) continue;
        // clip_uv true: sampling stays inside the frame's source rectangle, so a glyph never
        // bleeds in its neighbour from the shared font atlas and a cropped quad keeps the exact
        // partial region port::render::crop produced for the original clip.
        server->canvas_item_add_texture_rect_region(item_, to_rect(quad.destination), rid,
                                                    to_rect(quad.source), to_color(quad.color),
                                                    false, true);
    }
    return movie_quads;
}

godot::Dictionary Ps2RenderHost::draw(godot::Object* runtime, godot::Object* intro_movie,
                                      godot::Object* background_movie) {
    godot::Dictionary result;
    result["ok"] = false;
    auto* layout = godot::Object::cast_to<Ps2FrontEndLayout>(runtime);
    if (layout == nullptr) {
        last_error_ = "draw() requires a Ps2FrontEndLayout";
        result["error"] = last_error_;
        return result;
    }
    if (!item_.is_valid()) {
        last_error_ = "attach() has not been called";
        result["error"] = last_error_;
        return result;
    }
    auto* intro = godot::Object::cast_to<Ps2MoviePlayer>(intro_movie);
    auto* background = godot::Object::cast_to<Ps2MoviePlayer>(background_movie);
    LiveResources resources(layout->runtime().catalog(),
                            intro == nullptr ? nullptr : intro->frame_bitmap(),
                            background == nullptr ? nullptr : background->frame_bitmap());

    port::render::Frame frame;
    port::runtime::FrameStats stats;
    // Two different outcomes: a recovered path we have not translated (expected, reported as
    // not_implemented) and a port bug (unexpected, reported as an error). Neither may cross into
    // Godot, which is compiled -fno-exceptions.
    std::string gap;
    const auto failure =
        catch_all([&] { gap = hp2::guard([&] { stats = layout->runtime().build_frame(resources, frame); }); });
    if (!failure.empty()) {
        last_error_ = godot::String::utf8(("port error building the frame: " + failure).c_str());
        result["error"] = last_error_;
        return result;
    }
    if (!gap.empty()) {
        last_error_ = godot::String::utf8(gap.c_str());
        result["error"] = last_error_;
        result["not_implemented"] = last_error_;
        return result;
    }
    const int movie_quads = submit(frame, resources);

    last_error_ = "";
    result["ok"] = true;
    result["quads"] = stats.quads;
    result["glyphs"] = stats.glyphs;
    result["texts"] = stats.texts;
    result["missing"] = stats.missing;
    result["clipped_out"] = stats.clipped_out;
    result["sequence"] = static_cast<std::int64_t>(frame.sequence);
    result["textures"] = get_texture_count();
    result["movie_quads"] = movie_quads;
    result["error"] = "";
    return result;
}


void Ps2RenderHost::console_init(const godot::String& settings, bool info) {
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD
        console_=std::make_unique<extensions::DebugConsole>(settings.utf8().get_data());
        console_->enable_console(HP2_ENABLE_DEBUG_CONSOLE!=0);
        extensions::register_viewer_commands(console_->commands(),viewers_);
        console_->set_info(info);
        console_->clipboard([](void* user)->const char* {
            auto* self=static_cast<Ps2RenderHost*>(user);
            self->clipboard_text_=godot::DisplayServer::get_singleton()->clipboard_get().utf8().get_data();
            return self->clipboard_text_.c_str();
        },[](void*,const char* text) {
            godot::DisplayServer::get_singleton()->clipboard_set(godot::String::utf8(text));
        },this);
        auto* server=godot::RenderingServer::get_singleton();
        if(console_font_.is_valid())server->free_rid(console_font_);
        console_font_=server->texture_2d_create(to_image(console_->font()));
#else
        (void)settings;(void)info;
#endif
    });
    if(!failure.empty())last_error_=godot::String::utf8(failure.c_str());
}
void Ps2RenderHost::console_log(const godot::String& text,bool error) {
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD
        if(console_)console_->log(text.utf8().get_data(),error);
#else
        (void)text;(void)error;
#endif
    });
    if(!failure.empty())last_error_=godot::String::utf8(failure.c_str());
}
void Ps2RenderHost::console_execute(const godot::String& command) {
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD && HP2_ENABLE_DEBUG_CONSOLE
        if(console_)console_->execute(command.utf8().get_data());
#else
        (void)command;
#endif
    });
    if(!failure.empty())last_error_=godot::String::utf8(failure.c_str());
}
void Ps2RenderHost::console_focus_lost() {
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD
        if(console_)console_->input({extensions::Input::Kind::FocusLost});
#endif
    });
    if(!failure.empty())last_error_=godot::String::utf8(failure.c_str());
}
bool Ps2RenderHost::console_input(const godot::Ref<godot::InputEvent>& event) {
    bool consumed=false;
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD
        if(!console_||event.is_null())return;
        using extensions::Input;using extensions::Key;
        godot::Ref<godot::InputEventKey> key=event;
        if(key.is_valid()) {
            Input e{Input::Kind::Key};e.pressed=key->is_pressed();e.repeat=key->is_echo();
            e.ctrl=key->is_ctrl_pressed();e.shift=key->is_shift_pressed();
            e.alt=key->is_alt_pressed();e.super=key->is_meta_pressed();
            auto code=key->get_keycode();
            if(code==godot::KEY_F8)e.kind=Input::Kind::Toggle;
            const std::pair<godot::Key,Key> keys[]={
                {godot::KEY_TAB,Key::Tab},{godot::KEY_LEFT,Key::Left},{godot::KEY_RIGHT,Key::Right},
                {godot::KEY_UP,Key::Up},{godot::KEY_DOWN,Key::Down},{godot::KEY_PAGEUP,Key::PageUp},
                {godot::KEY_PAGEDOWN,Key::PageDown},{godot::KEY_HOME,Key::Home},{godot::KEY_END,Key::End},
                {godot::KEY_INSERT,Key::Insert},{godot::KEY_DELETE,Key::Delete},{godot::KEY_BACKSPACE,Key::Backspace},
                {godot::KEY_SPACE,Key::Space},{godot::KEY_ENTER,Key::Enter},{godot::KEY_ESCAPE,Key::Escape},
                {godot::KEY_KP_ENTER,Key::KeyPadEnter},{godot::KEY_A,Key::A},{godot::KEY_C,Key::C},
                {godot::KEY_V,Key::V},{godot::KEY_X,Key::X},{godot::KEY_Y,Key::Y},{godot::KEY_Z,Key::Z}};
            e.code=-1;for(auto pair:keys)if(pair.first==code)e.code=static_cast<int>(pair.second);
            consumed=console_->input(e);
            if(consumed&&e.kind!=Input::Kind::Toggle&&e.pressed&&key->get_unicode()>=32&&!e.ctrl&&!e.super) {
                e.kind=Input::Kind::Text;e.unicode=key->get_unicode();console_->input(e);
            }
        } else {
            godot::Ref<godot::InputEventMouseMotion> motion=event;
            godot::Ref<godot::InputEventMouseButton> button=event;
            if(motion.is_valid()) {
                Input e{Input::Kind::Mouse};e.x=motion->get_position().x;e.y=motion->get_position().y;
                consumed=console_->input(e);
            } else if(button.is_valid()) {
                Input e{Input::Kind::Mouse};e.x=button->get_position().x;e.y=button->get_position().y;
                consumed=console_->input(e);e.kind=Input::Kind::Button;e.pressed=button->is_pressed();
                const auto b=button->get_button_index();
                e.code=b==godot::MOUSE_BUTTON_LEFT?0:b==godot::MOUSE_BUTTON_RIGHT?1:b==godot::MOUSE_BUTTON_MIDDLE?2:-1;
                if(b>=godot::MOUSE_BUTTON_WHEEL_UP&&b<=godot::MOUSE_BUTTON_WHEEL_RIGHT) {
                    e.kind=Input::Kind::Wheel;
                    e.x=0;e.y=0;
                    if(e.pressed) {
                        if(b==godot::MOUSE_BUTTON_WHEEL_UP)e.y=button->get_factor();
                        if(b==godot::MOUSE_BUTTON_WHEEL_DOWN)e.y=-button->get_factor();
                        if(b==godot::MOUSE_BUTTON_WHEEL_LEFT)e.x=-button->get_factor();
                        if(b==godot::MOUSE_BUTTON_WHEEL_RIGHT)e.x=button->get_factor();
                    }
                }
                consumed=console_->input(e)||consumed;
            } else consumed=console_->visible();
        }
#else
        (void)event;
#endif
    });
    if(!failure.empty()) {last_error_=godot::String::utf8(failure.c_str());return true;}
    return consumed;
}
godot::Dictionary Ps2RenderHost::console_frame(double width,double height,double delta,const godot::Dictionary& status) {
    godot::Dictionary result;result["available"]=console_available();result["visible"]=false;result["batches"]=0;result["fps_visible"]=false;
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD
        if(!console_)return;
        extensions::Snapshot state;
        state.screen=godot::String(status.get("screen",godot::String("unknown"))).utf8().get_data();
        state.fps=status.get("fps",0.0);state.milliseconds=status.get("ms",0.0);
        state.quads=status.get("quads",0);state.glyphs=status.get("glyphs",0);
        state.textures=status.get("textures",0);state.missing=status.get("missing",0);state.clipped=status.get("clipped_out",0);
        console_->snapshot(std::move(state));
        const auto batches=console_->frame(width,height,delta,[&] {
            extensions::ViewerUi::fps(float(width),double(status.get("fps",0.0)),double(status.get("ms",0.0)));
            viewer_ui_.draw(float(width),float(height));
        });
        result["fps_visible"]=true;result["fps_right"]=width-8;
        result["capture_mouse"]=console_->capture_mouse();result["capture_keyboard"]=console_->capture_keyboard();
        result["visible"]=console_->visible();result["info"]=console_->info();result["batches"]=static_cast<int>(batches.size());
        result["logs"]=static_cast<int>(console_->log_count());
        auto* server=godot::RenderingServer::get_singleton();
        for(auto rid:console_draw_items_)server->canvas_item_clear(rid);
        if(!console_parent_.is_valid())return;
        while(console_items_.size()<batches.size()) {
            auto rid=server->canvas_item_create();server->canvas_item_set_parent(rid,console_parent_);console_items_.push_back(rid);
            auto child=server->canvas_item_create();server->canvas_item_set_parent(child,rid);console_draw_items_.push_back(child);
        }
        for(std::size_t i=0;i<batches.size();++i) {
            const auto& b=batches[i];auto rid=console_items_[i];
            server->canvas_item_set_custom_rect(rid,true,to_rect(b.clip));server->canvas_item_set_clip(rid,true);
            godot::PackedVector2Array points,uvs;godot::PackedColorArray colors;godot::PackedInt32Array indices;
            for(auto& v:b.vertices) {points.append({v.position.x,v.position.y});uvs.append({v.uv.x,v.uv.y});colors.append(to_color(v.color));}
            for(auto index:b.indices)indices.append(index);
            // RenderingServer clipping applies to children, so geometry belongs to a child.
            server->canvas_item_add_triangle_array(console_draw_items_[i],indices,points,colors,uvs,{}, {},console_font_);
        }
#else
        (void)width;(void)height;(void)delta;(void)status;
#endif
    });
    result["ok"]=failure.empty();
    if(!failure.empty()) {last_error_=godot::String::utf8(failure.c_str());result["error"]=last_error_;}
    return result;
}


bool Ps2RenderHost::console_visible() const {
#if HP2_DEBUG_BUILD
    return console_ && console_->visible();
#else
    return false;
#endif
}
void Ps2RenderHost::viewer_ui_snapshot(const godot::Dictionary& snapshot) {
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD
        extensions::ViewerUiState state;
        const auto text=[&](const char* key){return std::string(godot::String(snapshot.get(key,"")).utf8().get_data());};
        state.kind=text("kind");if(state.kind.empty())state.kind="frontend";
        state.source=text("source");state.status=text("status");state.diagnostic=text("diagnostic");
        state.rendering=text("rendering");state.camera=text("camera");
        state.wheel_speed=snapshot.get("wheel_speed",80.0);
        state.car_id=snapshot.get("car_id",-1);state.skin_id=snapshot.get("skin_id",-1);state.lod=snapshot.get("lod",-1);
        state.wireframe=snapshot.get("wireframe",false);state.bounds=snapshot.get("bounds",false);state.inspection=snapshot.get("inspection",false);state.show_lods=snapshot.get("show_lods",false);state.preview=snapshot.get("preview",false);state.wheel_blur=snapshot.get("wheel_blur",false);
        const godot::Array cars=snapshot.get("cars",godot::Array());
        for(int i=0;i<cars.size();++i){
            const godot::Dictionary car=cars[i];extensions::UiCar c;
            c.id=car.get("id",-1);c.code=godot::String(car.get("code","")).utf8().get_data();
            c.path=godot::String(car.get("path","")).utf8().get_data();c.error=godot::String(car.get("error","")).utf8().get_data();
            c.available=car.get("available",false);const godot::Array skins=car.get("skins",godot::Array());
            for(int j=0;j<skins.size();++j){const godot::Dictionary skin=skins[j];extensions::UiSkin color;color.id=skin.get("id",-1);color.name=godot::String(skin.get("name","")).utf8().get_data();color.preview_ready=skin.has("preview");color.preview_error=godot::String(skin.get("preview_error","")).utf8().get_data();if(color.preview_ready){const godot::Color rgb=skin["preview"];color.preview={{rgb.r,rgb.g,rgb.b}};}c.skins.push_back(std::move(color));}
            state.cars.push_back(std::move(c));
        }
        const godot::Array parts=snapshot.get("parts",godot::Array());
        for(int i=0;i<parts.size();++i){const godot::Dictionary part=parts[i];state.parts.push_back({godot::String(part.get("name","")).utf8().get_data(),bool(part.get("enabled",true))});}
        viewer_ui_.update(std::move(state));if(console_)console_->enable_viewer_input(viewer_ui_.state().kind!="frontend");
#else
        (void)snapshot;
#endif
    });
    if(!failure.empty())last_error_=godot::String::utf8(failure.c_str());
}
godot::Dictionary Ps2RenderHost::viewer_ui_next() {
    godot::Dictionary result;
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD
        const auto action=viewer_ui_.next();if(!action)return;
        result["action"]=godot::String::utf8(action->action.c_str());result["source"]=godot::String::utf8(action->source.c_str());
        result["name"]=godot::String::utf8(action->name.c_str());result["car_id"]=action->car_id;result["skin_id"]=action->skin_id;
        result["value"]=action->value;result["lod"]=action->lod;result["enabled"]=action->enabled;
#endif
    });
    result["ok"]=failure.empty();if(!failure.empty())result["error"]=godot::String::utf8(failure.c_str());return result;
}
bool Ps2RenderHost::viewer_ui_capture()const {
#if HP2_DEBUG_BUILD
    return console_&&(console_->capture_mouse()||console_->capture_keyboard());
#else
    return false;
#endif
}
godot::Dictionary Ps2RenderHost::viewer_request(const godot::String& action,const godot::String& source) {
    godot::Dictionary result;
    const auto failure=catch_all([&] {
        viewers_.request(extensions::parse_viewer_action(action.utf8().get_data()),source.utf8().get_data());
    });
    result["ok"]=failure.empty();result["error"]=godot::String::utf8(failure.c_str());return result;
}
godot::Dictionary Ps2RenderHost::viewer_next() {
    godot::Dictionary result;
    const auto failure=catch_all([&] {
        const auto request=viewers_.next();
        if(request) {result["action"]=extensions::action_name(request->action);result["source"]=godot::String::utf8(request->source.c_str());}
    });
    result["ok"]=failure.empty();result["error"]=godot::String::utf8(failure.c_str());return result;
}
godot::Dictionary Ps2RenderHost::viewer_activate(const godot::String& kind) {
    godot::Dictionary result;
    const auto failure=catch_all([&] {viewers_.activate(extensions::parse_viewer_kind(kind.utf8().get_data()));});
    result["ok"]=failure.empty();result["error"]=godot::String::utf8(failure.c_str());return result;
}
godot::Dictionary Ps2RenderHost::viewer_apply(const godot::String& action,const godot::String& source) {
    godot::Dictionary result;
    const auto failure=catch_all([&] {
        viewers_.apply({extensions::parse_viewer_action(action.utf8().get_data()),source.utf8().get_data()});
    });
    result["ok"]=failure.empty();result["error"]=godot::String::utf8(failure.c_str());return result;
}
godot::Dictionary Ps2RenderHost::viewer_state() const {
    godot::Dictionary result;
    const auto failure=catch_all([&] {
        const auto& state=viewers_.state();result["enabled"]=viewers_.enabled();
        result["kind"]=extensions::viewer_name(state.kind);result["source"]=godot::String::utf8(state.source.c_str());
        result["status"]=godot::String::utf8(state.status.c_str());result["diagnostic"]=godot::String::utf8(state.diagnostic.c_str());
        result["fixture"]=state.fixture;result["camera"]=godot::String::utf8(state.camera.c_str());
        result["rendering"]=godot::String::utf8(state.rendering.c_str());
    });
    result["ok"]=failure.empty();result["error"]=godot::String::utf8(failure.c_str());return result;
}
void Ps2RenderHost::viewer_camera(const godot::String& description) {
    const auto failure=catch_all([&] {viewers_.camera(description.utf8().get_data());});
    if(!failure.empty())last_error_=godot::String::utf8(failure.c_str());
}
godot::Dictionary Ps2RenderHost::viewer_loaded(bool success,const godot::String& diagnostic,const godot::String& rendering) {
    godot::Dictionary result;
    const auto failure=catch_all([&] { viewers_.loaded(success,diagnostic.utf8().get_data(),rendering.utf8().get_data()); });
    result["ok"]=failure.empty();result["error"]=godot::String::utf8(failure.c_str());return result;
}
void Ps2RenderHost::viewer_rendering(const godot::String& description) {
    const auto failure=catch_all([&] { viewers_.rendering(description.utf8().get_data()); });
    if(!failure.empty())last_error_=godot::String::utf8(failure.c_str());
}

} // namespace port::godot_bridge
