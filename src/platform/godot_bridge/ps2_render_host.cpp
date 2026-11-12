#include <godot_cpp/classes/performance.hpp>
#include "ps2_render_host.hpp"
#include "ps2_driving_runtime.hpp"
#include "boundary.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include "port/runtime/frame_builder.hpp"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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

godot::Dictionary Ps2RenderHost::diagnostic_qpc_clock() const {
    godot::Dictionary clock;
#ifdef _WIN32
    LARGE_INTEGER before, after, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&before);
    const auto godot_usec = godot::Time::get_singleton()->get_ticks_usec();
    QueryPerformanceCounter(&after);
    clock["godot_usec"] = godot_usec;
    clock["qpc_before"] = before.QuadPart;
    clock["qpc_after"] = after.QuadPart;
    clock["qpc_frequency"] = frequency.QuadPart;
#endif
    return clock;
}

std::shared_ptr<const port::render::Bitmap> LiveResources::texture(port::render::TextureId id) {
    if (id == port::runtime::kIntroMovieTexture) return intro_;
    if (id == port::runtime::kBackgroundMovieTexture) return background_;
    return catalog_ == nullptr ? nullptr : catalog_->texture(id);
}

void Ps2RenderHost::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("diagnostic_qpc_clock"), &Ps2RenderHost::diagnostic_qpc_clock);
#if HP2_DEBUG_BUILD
    godot::ClassDB::bind_method(godot::D_METHOD("debug_draw_authored_screen","resources","hash","width","height"),&Ps2RenderHost::debug_draw_authored_screen);
#endif
    godot::ClassDB::bind_method(godot::D_METHOD("draw_mirror_overlay","world","resources","width","height"),&Ps2RenderHost::draw_mirror_overlay);
    godot::ClassDB::bind_method(godot::D_METHOD("set_world_mirror","texture"),&Ps2RenderHost::set_world_mirror);
    godot::ClassDB::bind_method(godot::D_METHOD("prepare_world_ui","world","resources"),&Ps2RenderHost::prepare_world_ui);
    godot::ClassDB::bind_method(godot::D_METHOD("draw_loading","resources","width","height"),&Ps2RenderHost::draw_loading);
    godot::ClassDB::bind_method(godot::D_METHOD("draw_world","world","resources","width","height"),&Ps2RenderHost::draw_world);
    godot::ClassDB::bind_method(godot::D_METHOD("draw_controls","runtime","layers","width","height"),&Ps2RenderHost::draw_controls);
    godot::ClassDB::bind_method(godot::D_METHOD("console_visible"), &Ps2RenderHost::console_visible);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_keyboard_capture"),&Ps2RenderHost::viewer_keyboard_capture);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_ui_snapshot","snapshot"),&Ps2RenderHost::viewer_ui_snapshot);
    godot::ClassDB::bind_method(godot::D_METHOD("viewer_ui_targets"),&Ps2RenderHost::viewer_ui_targets);
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
    godot::ClassDB::bind_method(godot::D_METHOD("get_ui_scale"),&Ps2RenderHost::get_ui_scale);
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
    world_mirror_.unref();world_ui_.reset();world_ui_generation_=0;world_ui_revision_=0;world_ui_screens_=0;
    frontend_image_.unref();frontend_gpu_.reset();
#if HP2_DEBUG_BUILD
    console_.reset();
#endif
    auto* server = godot::RenderingServer::get_singleton();
    if (server != nullptr) {
        if(mirror_mask_item_.is_valid())server->free_rid(mirror_mask_item_);
        for(auto rid:projective_items_)if(rid.is_valid())server->free_rid(rid);
        for (auto rid : console_draw_items_) if(rid.is_valid()) server->free_rid(rid);
        for (auto rid : console_items_) if(rid.is_valid()) server->free_rid(rid);
        if(console_font_.is_valid()) server->free_rid(console_font_);
        if(console_parent_.is_valid()) server->free_rid(console_parent_);
    }
    mirror_mask_item_={};mirror_mask_material_.unref();mirror_mask_shader_.unref();
    projective_items_.clear();projective_materials_.clear();projective_shader_.unref();
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
    if(id==0x100000003ull)return world_mirror_.is_valid()?world_mirror_->get_rid():godot::RID();
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

// TASK-0112. Prepare candidate UI before the shared presentation barrier.
void Ps2RenderHost::set_world_mirror(const godot::Ref<godot::Texture2D>& texture){world_mirror_=texture;}

godot::Dictionary Ps2RenderHost::draw_mirror_overlay(godot::Object* runtime,godot::Object* resources,double width,double height){
 godot::Dictionary result;const auto error=catch_all([&]{auto* source=godot::Object::cast_to<Ps2DrivingRuntime>(runtime);auto* layout=godot::Object::cast_to<Ps2FrontEndLayout>(resources);if(!source||!layout||!source->candidate_frame()||width<=0||height<=0)throw std::invalid_argument("Mirror overlay requires source frame and prepared render target");
  const auto& frame=*source->candidate_frame();if(!frame.mirror_view)return;auto* catalog=layout->runtime().catalog();if(!catalog)throw std::logic_error("Mirror original texture catalog is unavailable");const auto& view=*frame.mirror_view;
  port::render::Frame overlay;overlay.clear={0,0,0,0};overlay.logical_size={float(width),float(height)};const float sx=float(width)/float(view.texture_size[0]),sy=float(height)/float(view.texture_size[1]);const auto& rect=view.overlay_rect;
  // INGAMEB RVM uses ALPHA44 (source over); RVM_MASK uses ALPHA48:
  // (Cs-0)*As/128+Cd, with framebuffer alpha replaced by source alpha.
  // Generic Canvas source-over would paint its opaque black interior over
  // the rear view. Copy the prior raster RGB explicitly and replace alpha.
  const auto info=catalog->texture(view.overlay_hashes[0]),mask=catalog->texture(view.overlay_hashes[1]);if(!info||!mask)throw std::runtime_error("Original mirror overlay texture is missing");
  port::render::Quad quad;quad.has_vertex_data=true;quad.texture=view.overlay_hashes[0];quad.positions={port::render::Vec2{rect[0]*sx,rect[1]*sy},port::render::Vec2{(rect[0]+rect[2])*sx,rect[1]*sy},port::render::Vec2{(rect[0]+rect[2])*sx,(rect[1]+rect[3])*sy},port::render::Vec2{rect[0]*sx,(rect[1]+rect[3])*sy}};const float w=float(info->width),h=float(info->height);quad.source_points={port::render::Vec2{w,0},port::render::Vec2{0,0},port::render::Vec2{0,h},port::render::Vec2{w,h}};for(auto& c:quad.colors)c={1,1,1,1};overlay.quads.push_back(quad);
  output_scale_=1;submit(overlay,*catalog);
  auto* server=godot::RenderingServer::get_singleton();if(!mirror_mask_item_.is_valid()){mirror_mask_item_=server->canvas_item_create();server->canvas_item_set_parent(mirror_mask_item_,item_);server->canvas_item_set_draw_index(mirror_mask_item_,127);mirror_mask_shader_.instantiate();mirror_mask_shader_->set_code(R"shader(shader_type canvas_item;
render_mode unshaded, blend_disabled;
uniform sampler2D native_destination : hint_screen_texture, repeat_disable, filter_nearest;
void fragment(){vec4 src=texture(TEXTURE,UV);vec4 dst=texture(native_destination,SCREEN_UV);COLOR=vec4(dst.rgb+src.rgb*src.a,src.a);}
)shader");mirror_mask_material_.instantiate();mirror_mask_material_->set_shader(mirror_mask_shader_);server->canvas_item_set_material(mirror_mask_item_,mirror_mask_material_->get_rid());}
  server->canvas_item_clear(mirror_mask_item_);godot::PackedVector2Array points,uvs;godot::PackedColorArray colours;for(const auto& p:quad.positions){points.push_back({p.x,p.y});colours.push_back({1,1,1,1});}for(const godot::Vector2 uv:{godot::Vector2{1,0},godot::Vector2{0,0},godot::Vector2{0,1},godot::Vector2{1,1}})uvs.push_back(uv);godot::PackedInt32Array indices;for(int i:{0,1,3,1,2,3})indices.push_back(i);server->canvas_item_add_triangle_array(mirror_mask_item_,indices,points,colours,uvs,godot::PackedInt32Array(),godot::PackedFloat32Array(),texture_for(view.overlay_hashes[1],*catalog));
 });result["ok"]=error.empty();if(!error.empty())result["error"]=godot::String::utf8(error.c_str());return result;
}

godot::Dictionary Ps2RenderHost::prepare_world_ui(godot::Object* runtime,godot::Object* resources){
 godot::Dictionary result;const auto error=catch_all([&]{
  auto* source=godot::Object::cast_to<Ps2DrivingRuntime>(runtime);auto* layout=godot::Object::cast_to<Ps2FrontEndLayout>(resources);
  if(!source||!layout||!source->candidate_frame())throw std::invalid_argument("World UI requires a source frame and original resources");
  auto* catalog=layout->runtime().catalog();if(!catalog)throw std::logic_error("World UI original texture catalog is unavailable");
  const auto& candidate=*source->candidate_frame();catalog->set_presentation_bundles(candidate.ui_texture_bundles);port::render::Frame frame;frame.clear={0,0,0,0};frame.sequence=candidate.step;
  port::runtime::WidgetSizes sizes(*catalog,layout->runtime().fonts());int missing=0;
  for(auto document:candidate.ui_screens){port::render::Frame addition;const auto stats=port::runtime::build_frame(document,sizes,{},addition);missing+=stats.missing;port::render::append_frontend_frame(frame,addition);}
  if(candidate.mirror_view){
   if(world_mirror_.is_null())throw std::runtime_error("Original mirror requires its prepared render target");
   auto rect=candidate.mirror_view->display_rect;const auto& raster=candidate.mirror_view->raster_size;for(unsigned i=0;i<4;++i)rect[i]*=i%2?480.f/float(raster[1]):640.f/float(raster[0]);port::render::Quad mirror;mirror.texture=0x100000003ull;mirror.has_vertex_data=true;mirror.frontend_layer=127;mirror.positions={port::render::Vec2{rect[0],rect[1]},port::render::Vec2{rect[0]+rect[2],rect[1]},port::render::Vec2{rect[0]+rect[2],rect[1]+rect[3]},port::render::Vec2{rect[0],rect[1]+rect[3]}};
   const float w=float(world_mirror_->get_width()),h=float(world_mirror_->get_height());mirror.source_points={port::render::Vec2{w,0},port::render::Vec2{0,0},port::render::Vec2{0,h},port::render::Vec2{w,h}};for(auto& c:mirror.colors)c={1,1,1,1};frame.quads.insert(frame.quads.begin(),mirror);
  }
  for(const auto& polygon:candidate.hud_geometry){
   const auto bitmap=catalog->texture(polygon.texture_hash);if(!bitmap)throw std::runtime_error("Source HUD map texture is unavailable: hash="+std::to_string(polygon.texture_hash)+" layer="+std::to_string(polygon.layer));
   port::render::Quad quad;quad.texture=polygon.texture_hash;quad.has_vertex_data=true;quad.has_projective_uv=true;quad.bypass_clip=true;quad.frontend_layer=static_cast<std::int8_t>(polygon.layer);
   for(unsigned i=0;i<4;++i){const auto& v=polygon.vertices[i];quad.positions[i]={v.position[0],v.position[1]};quad.source_points[i]={v.uvq[0]*bitmap->width,v.uvq[1]*bitmap->height};quad.projective_q[i]=v.uvq[2];const auto c=v.colour;quad.colors[i]={float(c&255)/128.f,float((c>>8)&255)/128.f,float((c>>16)&255)/128.f,float(c>>24)/128.f};}
   frame.quads.push_back(std::move(quad));
  }
  // Original Flailer/ePoly world UI uses shared layer buckets, including map
  // geometry emitted outside the screen-object rings. Keep ties stable.
  if(!candidate.hud_geometry.empty()){std::stable_sort(frame.quads.begin(),frame.quads.end(),[](const auto& a,const auto& b){return a.frontend_layer>b.frontend_layer;});frame.frontend_ranges.clear();}
  if(missing)throw std::runtime_error("Candidate World UI references missing original font/texture resources");
  // Upload before committing any source pose/audio. Drawing only consumes RIDs.
  for(const auto& quad:frame.quads)if(quad.texture&&!texture_for(quad.texture,*catalog).is_valid())throw std::runtime_error("World UI texture upload failed");
  world_ui_=std::move(frame);world_ui_generation_=candidate.generation;world_ui_revision_=candidate.ui_revision;world_ui_screens_=int(candidate.ui_screens.size());
 });result["ok"]=error.empty();if(!error.empty())result["error"]=godot::String::utf8(error.c_str());return result;
}
// Native committed source documents only; no menu/simulation tick.
godot::Dictionary Ps2RenderHost::draw_world(godot::Object* runtime,godot::Object* resources,double width,double height){
 godot::Dictionary result;const auto error=catch_all([&]{
  auto* source=godot::Object::cast_to<Ps2DrivingRuntime>(runtime);auto* layout=godot::Object::cast_to<Ps2FrontEndLayout>(resources);
  if(!source||!layout||!item_.is_valid()||width<=0||height<=0)throw std::invalid_argument("World UI requires committed runtime, original resources, canvas and positive size");
  const auto* committed=source->presented_frame();if(!committed||!world_ui_||world_ui_->sequence!=committed->step||world_ui_generation_!=committed->generation||world_ui_revision_!=committed->ui_revision)throw std::logic_error("World UI presentation generation/step/revision mismatch");
  auto* catalog=layout->runtime().catalog();if(!catalog)throw std::logic_error("World UI original texture catalog is unavailable");
  // Central source screens use equal X/Y scale. Edge HUD anchoring requires its
  // recovered producer/role and is not inferred from arbitrary widget positions.
  // Caller places a clipped Control using the immutable original Flailer clip
  // (initializer001462B0:0,0,644,480), relative to the authored640x480 canvas.
  // Preserve that guard band; no widget positions/visibility or source state change.
  const auto& clip=committed->ui_clip_bounds;
  for(const auto value:clip)if(!std::isfinite(value))throw std::runtime_error("Invalid published source UI clip");
  if(!(clip[2]>clip[0]&&clip[3]>clip[1]))throw std::runtime_error("Source UI clip was not initialized");
  output_scale_=std::min(width/640.,height/480.);world_ui_->display_offset={-clip[0],-clip[1]};
  submit(*world_ui_,*catalog);result["screens"]=world_ui_screens_;result["quads"]=int64_t(world_ui_->quads.size());result["step"]=int64_t(world_ui_->sequence);
 });result["ok"]=error.empty();if(!error.empty())result["error"]=godot::String::utf8(error.c_str());return result;
}
#if HP2_DEBUG_BUILD
godot::Dictionary Ps2RenderHost::debug_draw_authored_screen(godot::Object* resources,int64_t hash,double width,double height){
 godot::Dictionary result;const auto error=catch_all([&]{
  auto* layout=godot::Object::cast_to<Ps2FrontEndLayout>(resources);if(!layout||!item_.is_valid()||width<=0||height<=0||hash<=0||hash>0xffffffffLL)throw std::invalid_argument("Authored-screen viewer arguments are invalid");
  auto& runtime=layout->runtime();auto* catalog=runtime.catalog();if(!catalog)throw std::runtime_error("Authored-screen original texture catalog missing");
  const hp2::frontend::FeScreen* document=nullptr;for(int i=0;;++i){const auto* candidate=runtime.screen(i);if(!candidate)break;if(candidate->name_hash_0x08==std::uint32_t(hash)){document=candidate;break;}}
  if(!document)throw std::runtime_error("Authored-screen hash absent from original bundle");
  auto copy=*document;port::runtime::WidgetSizes sizes(*catalog,runtime.fonts());port::render::Frame frame;const auto stats=port::runtime::build_frame(copy,sizes,{},frame);if(stats.missing)throw std::runtime_error("Authored-screen original font or texture missing");
  output_scale_=std::min(width/640.,height/480.);frame.display_offset={0,0};submit(frame,*catalog);result["quads"]=int64_t(frame.quads.size());
 });result["ok"]=error.empty();if(!error.empty())result["error"]=godot::String(error.c_str());return result;
}
#endif
int Ps2RenderHost::submit(const port::render::Frame& frame, port::render::Resources& resources) {
    int movie_quads = 0;
    auto* server = godot::RenderingServer::get_singleton();
    server->canvas_item_clear(item_);
    auto transform=godot::Transform2D().scaled(godot::Vector2(static_cast<real_t>(output_scale_),
                                                             static_cast<real_t>(output_scale_)));
    transform.set_origin(godot::Vector2(frame.display_offset.x*output_scale_,frame.display_offset.y*output_scale_));
    server->canvas_item_set_transform(item_,transform);
    if (frame.clear.a > 0) {
        server->canvas_item_add_rect(
            item_, godot::Rect2(0, 0, frame.logical_size.x, frame.logical_size.y),
            to_color(frame.clear));
    }
    const bool projective=std::any_of(frame.quads.begin(),frame.quads.end(),[](const auto& q){return q.has_projective_uv;});
    for(auto child:projective_items_)server->canvas_item_clear(child);
    if(projective){
     if(projective_shader_.is_null()){projective_shader_.instantiate();projective_shader_->set_code(R"shader(shader_type canvas_item;
render_mode unshaded;
uniform vec3 source_rgb=vec3(1.0);
varying float original_q;
varying float original_alpha;
void vertex(){original_q=COLOR.r;original_alpha=COLOR.g;COLOR=vec4(1.0);}
void fragment(){COLOR=texture(TEXTURE,UV/original_q)*vec4(source_rgb,original_alpha);}
)shader");}
     while(projective_items_.size()<frame.quads.size()){auto child=server->canvas_item_create();server->canvas_item_set_parent(child,item_);server->canvas_item_set_draw_index(child,int(projective_items_.size()));projective_items_.push_back(child);projective_materials_.emplace_back();}
    }
    std::size_t quad_index=0;
    for (const auto& quad : frame.quads) {
        const auto draw_item=projective?projective_items_[quad_index]:item_;
        if(projective){
         if(quad.has_projective_uv){
          auto& material=projective_materials_[quad_index];if(material.is_null()){material.instantiate();material->set_shader(projective_shader_);}
          for(unsigned i=1;i<4;++i)if(quad.colors[i].r!=quad.colors[0].r||quad.colors[i].g!=quad.colors[0].g||quad.colors[i].b!=quad.colors[0].b)throw std::logic_error("Source projective HUD polygon requires independent RGB transport");
          material->set_shader_parameter("source_rgb",godot::Vector3(quad.colors[0].r,quad.colors[0].g,quad.colors[0].b));server->canvas_item_set_material(draw_item,material->get_rid());
         }else server->canvas_item_set_material(draw_item,godot::RID());
        }
        ++quad_index;
        if (quad.has_vertex_data) {
            godot::RID rid;
            float width = 1, height = 1;
            if (quad.texture != 0) {
                rid = texture_for(quad.texture, resources);
                if (!rid.is_valid()) continue;
                if(quad.texture==0x100000003ull){width=float(world_mirror_->get_width());height=float(world_mirror_->get_height());}
                else{const auto& texture = textures_.at(quad.texture);width = static_cast<float>(texture.width);height = static_cast<float>(texture.height);}
                if (quad.texture == port::runtime::kIntroMovieTexture ||
                    quad.texture == port::runtime::kBackgroundMovieTexture) ++movie_quads;
            }
            godot::PackedVector2Array points, uvs;
            godot::PackedColorArray colors;
            for (std::size_t corner = 0; corner < 4; ++corner) {
                points.push_back(godot::Vector2(quad.positions[corner].x, quad.positions[corner].y));
                uvs.push_back(godot::Vector2(quad.source_points[corner].x / width,
                                            quad.source_points[corner].y / height));
                if(quad.has_projective_uv){if(!std::isfinite(quad.projective_q[corner])||quad.projective_q[corner]==0)throw std::logic_error("Invalid original HUD projective Q");colors.push_back(godot::Color(quad.projective_q[corner],quad.colors[corner].a,0,1));}
                else colors.push_back(to_color(quad.colors[corner]));
            }
            godot::PackedInt32Array indices;for(int index:{0,1,3,1,2,3})indices.push_back(index);
            server->canvas_item_add_triangle_array(draw_item,indices,points,colors,uvs,godot::PackedInt32Array(),godot::PackedFloat32Array(),rid);
            continue;
        }
        if (quad.texture == 0) { // solid-colour rect; see render::Quad
            server->canvas_item_add_rect(draw_item, to_rect(quad.destination), to_color(quad.color));
            continue;
        }
        if (quad.texture == port::runtime::kIntroMovieTexture ||
            quad.texture == port::runtime::kBackgroundMovieTexture) ++movie_quads;
        const auto rid = texture_for(quad.texture, resources);
        if (!rid.is_valid()) continue;
        // clip_uv true: sampling stays inside the frame's source rectangle, so a glyph never
        // bleeds in its neighbour from the shared font atlas and a cropped quad keeps the exact
        // partial region port::render::crop produced for the original clip.
        server->canvas_item_add_texture_rect_region(draw_item, to_rect(quad.destination), rid,
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
    int movie_quads=0;
    const auto render_error=catch_all([&]{gap=hp2::guard([&]{
        if(auto* selection=layout->runtime().selection();selection && selection->observation().active){
            if(!frontend_gpu_)frontend_gpu_=std::make_unique<FrontendGpuExecutor>();
            const auto packet=selection->packet(frame,resources);frontend_image_=frontend_gpu_->render(packet);
            result.merge(frontend_gpu_->statistics());
            const godot::String gpu_error=result.get("gpu_error",godot::String());if(!gpu_error.is_empty())throw std::runtime_error(gpu_error.utf8().get_data());
            auto* server=godot::RenderingServer::get_singleton();server->canvas_item_clear(item_);
            server->canvas_item_set_transform(item_,godot::Transform2D().scaled({real_t(output_scale_),real_t(output_scale_)}));
            server->canvas_item_add_texture_rect(item_,{0,0,frame.logical_size.x,frame.logical_size.y},frontend_image_->get_rid());
            selection->after_display();result["shared_frontend_target"]=true;result["shared_draws"]=int64_t(packet.draws.size());result["shared_vertices"]=int64_t(packet.vertices.size());
        }else{frontend_image_.unref();frontend_gpu_.reset();movie_quads=submit(frame,resources);}
    });});
    if(!render_error.empty()||!gap.empty()){last_error_=godot::String::utf8((render_error.empty()?gap:render_error).c_str());result["error"]=last_error_;return result;}

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


// TASK-0104: Control-generated layer images are device resources, not reconstructed widgets.
godot::Dictionary Ps2RenderHost::draw_controls(godot::Object* runtime,const godot::Array& layers,int width,int height){
    godot::Dictionary result;auto* layout=godot::Object::cast_to<Ps2FrontEndLayout>(runtime);
    if(!layout || width<=0 || height<=0){result["error"]="Invalid Control frontend target";return result;}
    std::string gap;const auto failure=catch_all([&]{gap=hp2::guard([&]{
        auto* server=godot::RenderingServer::get_singleton();server->canvas_item_clear(item_);
        auto* selection=layout->runtime().selection();
        if(!selection || !selection->observation().active){frontend_image_.unref();frontend_gpu_.reset();result["ok"]=true;return;}
        port::render::Frame ui;auto* resources=layout->runtime().catalog();if(!resources)throw std::logic_error("Control selection lacks original assets");
        const auto packet_begin=std::chrono::steady_clock::now();
        auto packet=selection->packet(ui,*resources);
        result["selection_packet_cpu_ms"]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-packet_begin).count();
        // Match the Control scene fit in both portrait and landscape without distorting the vehicle.
        const float fit=std::min(float(width)/640.f,float(height)/480.f);
        for(auto& vertex:packet.vertices){vertex.clip[0]*=640.f*fit/width;vertex.clip[1]*=480.f*fit/height;}
        packet.width=width;packet.height=height;
        std::vector<FrontendGpuExecutor::ExternalTexture> external;
        for(int i=0;i<layers.size();++i){godot::Dictionary layer=layers[i];const int number=layer["layer"];
            godot::Ref<godot::Texture2D> texture=layer["texture"];if(texture.is_null())throw std::logic_error("Control layer texture missing");
            if(texture->get_width()!=width || texture->get_height()!=height){result["ok"]=true;result["resize_pending"]=true;return;}
            const auto id=0x300000000ull|std::uint32_t(number);external.push_back({id,texture});
            const float depth=selection->layer_depth(number);
            port::render::FrontendDraw draw;draw.first=packet.vertices.size();draw.count=6;draw.texture=id;draw.stage=number>=20&&number<=24?1:7;draw.alpha_threshold=0;
            const int triangles[6]={0,1,2,0,2,3};const float xy[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};const float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
            for(int index:triangles)packet.vertices.push_back({{{xy[index][0],xy[index][1],depth,1}},{uv[index][0],uv[index][1]},{{1,1,1,1}}});
            packet.draws.push_back(draw);
        }
        std::stable_sort(packet.draws.begin(),packet.draws.end(),[](const auto& a,const auto& b){return a.stage<b.stage;});
        if(!frontend_gpu_)frontend_gpu_=std::make_unique<FrontendGpuExecutor>();
        const auto draws=packet.draws.size(),vertices=packet.vertices.size();
        result.merge(frontend_gpu_->statistics());
        const godot::String gpu_error=result.get("gpu_error",godot::String());if(!gpu_error.is_empty())throw std::runtime_error(gpu_error.utf8().get_data());
        frontend_image_=frontend_gpu_->render(std::move(packet),std::move(external));
        server->canvas_item_set_transform(item_,godot::Transform2D());server->canvas_item_add_texture_rect(item_,{0,0,float(width),float(height)},frontend_image_->get_rid());selection->after_display();
        result["ok"]=true;result["shared_frontend_target"]=true;result["shared_draws"]=int64_t(draws);result["shared_vertices"]=int64_t(vertices);
    });});
    if(!failure.empty()||!gap.empty()){result["ok"]=false;result["error"]=godot::String::utf8((failure.empty()?gap:failure).c_str());}return result;
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
        console_font_revision_=console_->font().revision;
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
double Ps2RenderHost::get_ui_scale()const{
    auto* display=godot::DisplayServer::get_singleton();
    if(display->get_name()=="headless")return 1;
#ifdef _WIN32
    // Effective per-window DPI follows the Windows scaling setting, not the
    // panel's physical pixel density. No process DPI-awareness state is changed.
    using QueryDpi=UINT(WINAPI*)(HWND);
    static const auto query=reinterpret_cast<QueryDpi>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow"));
    if(query){const auto hwnd=reinterpret_cast<HWND>(display->window_get_native_handle(godot::DisplayServer::WINDOW_HANDLE));
        if(const auto dpi=query(hwnd))return std::clamp(double(dpi)/96.0,.5,8.0);}
#endif
    return std::clamp(double(display->screen_get_scale(display->window_get_current_screen())),.5,8.0);
}

godot::Dictionary Ps2RenderHost::console_frame(double width,double height,double delta,const godot::Dictionary& status) {
    godot::Dictionary result;result["available"]=console_available();result["visible"]=false;result["batches"]=0;result["fps_visible"]=false;result["ui_bulk_upload"]=false;
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD
        if(!console_)return;
        const bool measure = bool(status.get("measure_debug_ui", false));
        const bool suspend = measure && bool(status.get("suspend_debug_ui", false));
        auto* server = godot::RenderingServer::get_singleton();
        if (suspend) {
            if (!console_drawing_suspended_) {
                for (auto rid : console_draw_items_) server->canvas_item_clear(rid);
            }
            console_drawing_suspended_ = true;
            result["suspended"] = true;
            result["info"] = console_->info();
            return;
        }
        console_drawing_suspended_ = false;
        using Clock = std::chrono::steady_clock;
        const auto stamp = [measure] { return measure ? Clock::now() : Clock::time_point{}; };
        const auto elapsed = [](Clock::time_point start, Clock::time_point end) {
            return std::chrono::duration<double, std::milli>(end - start).count();
        };
        // Main-thread publication includes warnings queued while hidden or on
        // source workers. Do not drain until the console actually exists.
        for(auto& warning:port::diagnostics::take_warning_messages())console_->warning(std::move(warning));
        const double ui_scale=status.has("ui_scale")?double(status["ui_scale"]):get_ui_scale();
        const godot::Dictionary profile_state=status.get("profile_debug",godot::Dictionary{});
        const bool bulk=bool(profile_state.get("ui_bulk_upload",false));result["ui_bulk_upload"]=bulk;
        console_->enable_viewer_input(viewer_ui_.state().kind!="frontend"||bool(profile_state.get("enabled",false))||console_->info());
        console_->set_ui_scale(float(ui_scale));
        extensions::Snapshot state;
        state.screen=godot::String(status.get("screen",godot::String("unknown"))).utf8().get_data();
        state.fps=status.get("fps",0.0);state.milliseconds=status.get("ms",0.0);
        state.quads=status.get("quads",0);state.glyphs=status.get("glyphs",0);
        state.textures=status.get("textures",0);state.missing=status.get("missing",0);state.clipped=status.get("clipped_out",0);
        console_->snapshot(std::move(state));
        const auto draw_begin = stamp();
        const auto batches=console_->frame(width,height,delta,[&] {
            const float scale=console_->ui_scale();
            std::optional<extensions::ViewerUi::FpsRegion> fps_region;
            const godot::Rect2 rect=status.get("fps_region",godot::Rect2{});
            if(rect.has_area())fps_region=extensions::ViewerUi::FpsRegion{rect.position.x/scale,rect.position.y/scale,rect.size.x/scale,rect.size.y/scale};
            if(!bool(status.get("fps_hidden",false)))extensions::ViewerUi::fps(float(width)/scale,double(status.get("fps",0.0)),double(status.get("ms",0.0)),fps_region);
            viewer_ui_.draw(float(width)/scale,float(height)/scale);
            result["profile_debug"]=profile_debug_ui_.draw(status.get("profile_debug",godot::Dictionary{}),float(width)/scale,float(height)/scale);
        });
        if (measure) result["draw_data_ms"] = elapsed(draw_begin, stamp());
        result["ui_scale"]=console_->ui_scale();result["font_revision"]=int64_t(console_->font().revision);
        result["font_width"]=console_->font().width;result["font_height"]=console_->font().height;
        const auto& ui=viewer_ui_.state();result["coordinates_visible"]=ui.camera_known&&(ui.kind=="track"||ui.real_track);
        if(ui.camera_known){const auto& p=ui.camera_host;result["camera_position_hp2"]=godot::Vector3(p[2],p[0],p[1]);result["camera_position_godot"]=godot::Vector3(p[0],p[1],p[2]);}
        result["fps_visible"]=!bool(status.get("fps_hidden",false));result["fps_region"]=status.get("fps_region",godot::Rect2{});result["fps_right"]=width-8*console_->ui_scale();
        result["capture_mouse"]=console_->capture_mouse();result["capture_keyboard"]=console_->capture_keyboard();
        result["visible"]=console_->visible();result["info"]=console_->info();result["batches"]=static_cast<int>(batches.size());
        result["logs"]=static_cast<int>(console_->log_count());
        const auto canvas_begin = stamp();
        for(auto rid:console_draw_items_)server->canvas_item_clear(rid);
        if(console_font_revision_!=console_->font().revision){
            if(console_font_.is_valid())server->free_rid(console_font_);
            console_font_=server->texture_2d_create(to_image(console_->font()));console_font_revision_=console_->font().revision;
        }
        if(!console_parent_.is_valid())return;
        while(console_items_.size()<batches.size()) {
            auto rid=server->canvas_item_create();server->canvas_item_set_parent(rid,console_parent_);console_items_.push_back(rid);
            auto child=server->canvas_item_create();server->canvas_item_set_parent(child,rid);console_draw_items_.push_back(child);
        }
        double packing_ms = 0;
        for(std::size_t i=0;i<batches.size();++i) {
            const auto& b=batches[i];auto rid=console_items_[i];
            server->canvas_item_set_custom_rect(rid,true,to_rect(b.clip));server->canvas_item_set_clip(rid,true);
            const auto packing_begin = stamp();
            godot::PackedVector2Array points,uvs;godot::PackedColorArray colors;godot::PackedInt32Array indices;
            if(bulk){
                const auto count=static_cast<int64_t>(b.vertices.size());
                points.resize(count);uvs.resize(count);colors.resize(count);indices.resize(static_cast<int64_t>(b.indices.size()));
                auto* point_data=points.ptrw();auto* uv_data=uvs.ptrw();auto* color_data=colors.ptrw();auto* index_data=indices.ptrw();
                for(std::size_t j=0;j<b.vertices.size();++j){const auto& v=b.vertices[j];point_data[j]={v.position.x,v.position.y};uv_data[j]={v.uv.x,v.uv.y};color_data[j]=to_color(v.color);}
                for(std::size_t j=0;j<b.indices.size();++j)index_data[j]=b.indices[j];
            }else{
                for(auto& v:b.vertices) {points.append({v.position.x,v.position.y});uvs.append({v.uv.x,v.uv.y});colors.append(to_color(v.color));}
                for(auto index:b.indices)indices.append(index);
            }
            if (measure) packing_ms += elapsed(packing_begin, stamp());
            // RenderingServer clipping applies to children, so geometry belongs to a child.
            server->canvas_item_add_triangle_array(console_draw_items_[i],indices,points,colors,uvs,{}, {},console_font_);
        }
        if (measure) {
            result["pack_ms"] = packing_ms;
            result["canvas_ms"] = elapsed(canvas_begin, stamp()) - packing_ms;
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
bool Ps2RenderHost::viewer_keyboard_capture()const {
#if HP2_DEBUG_BUILD
    return console_&&console_->capture_keyboard();
#else
    return false;
#endif
}
void Ps2RenderHost::viewer_ui_snapshot(const godot::Dictionary& snapshot) {
    const auto failure=catch_all([&] {
#if HP2_DEBUG_BUILD
        extensions::ViewerUiState state;
        if(auto* performance=godot::Performance::get_singleton()){state.draw_calls=std::int64_t(performance->get_monitor(godot::Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME));state.rendered_primitives=std::int64_t(performance->get_monitor(godot::Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME));}
        state.render_statistics_enabled=snapshot.get("render_statistics_enabled",false);
        const godot::Array statistics=snapshot.get("render_statistics",godot::Array());for(int i=0;i<statistics.size();++i)state.render_statistics.emplace_back(godot::String(statistics[i]).utf8().get_data());
        state.camera_known=snapshot.has("camera_position");if(state.camera_known){const godot::Vector3 p=snapshot["camera_position"];state.camera_host={{p.x,p.y,p.z}};}
        const auto text=[&](const char* key){return std::string(godot::String(snapshot.get(key,"")).utf8().get_data());};
        state.kind=text("kind");if(state.kind.empty())state.kind="frontend";
        state.source=text("source");state.status=text("status");state.diagnostic=text("diagnostic");
        state.rendering=text("rendering");state.camera=text("camera");
        state.real_track=snapshot.get("real_track",false);state.track_name=text("track_name");state.track_id=snapshot.get("track_id",-1);
        state.show_lod=snapshot.get("show_lod",false);state.render_collision=snapshot.get("render_collision",false);state.render_material=snapshot.get("render_material",false);
        state.loaded_instances=snapshot.get("loaded_instances",0);state.considered_instances=snapshot.get("considered_instances",0);state.visible_instances=snapshot.get("visible_instances",0);
        state.loaded_sections=snapshot.get("loaded_sections",0);state.selected_sections=snapshot.get("selected_sections",0);
        state.render_props=snapshot.get("render_props",true);state.render_ready=snapshot.get("render_ready",false);state.distance_override=snapshot.get("distance_override",false);
        state.draw_distance=snapshot.get("distance",10000.f);state.effective_near=snapshot.get("effective_near",.5f);state.effective_far=snapshot.get("effective_far",10000.f);state.original_far=snapshot.get("original_far",10000.f);state.native_frame_ms=snapshot.get("native_frame_ms",0.0);state.host_submit_ms=snapshot.get("host_submit_ms",0.0);
        const godot::Array effects=snapshot.get("effects",godot::Array());for(int i=0;i<effects.size();++i){const godot::Dictionary row=effects[i];extensions::UiEffect item;item.id=godot::String(row.get("id","")).utf8().get_data();item.name=godot::String(row.get("name","")).utf8().get_data();item.category=godot::String(row.get("category","")).utf8().get_data();item.reason=godot::String(row.get("reason","")).utf8().get_data();item.enabled=row.get("enabled",false);item.available=row.get("available",false);item.active=row.get("active",false);state.effects.push_back(std::move(item));}
        state.debug_ready=snapshot.get("debug_ready",false);state.debug_diagnostic=text("debug_diagnostic");
        const godot::Array legend=snapshot.get("material_legend",godot::Array());
        for(int i=0;i<legend.size();++i){const godot::Dictionary row=legend[i];extensions::UiTrackMaterial entry;
            entry.id=row.get("id",0);entry.name=godot::String(row.get("name","")).utf8().get_data();entry.polygon_count=row.get("polygon_count",0);
            const godot::Color color=row.get("color",godot::Color(1,1,1,1));entry.color={{color.r,color.g,color.b,color.a}};state.material_legend.push_back(std::move(entry));}
        const godot::Array tracks=snapshot.get("tracks",godot::Array());
        for(int i=0;i<tracks.size();++i){const godot::Dictionary t=tracks[i];extensions::UiTrack entry;
            entry.id=t.get("id",-1);entry.name=godot::String(t.get("name","")).utf8().get_data();entry.location=godot::String(t.get("location","")).utf8().get_data();
            entry.path=godot::String(t.get("path","")).utf8().get_data();entry.error=godot::String(t.get("error","")).utf8().get_data();entry.available=t.get("available",false);state.tracks.push_back(std::move(entry));}

        state.driving_telemetry=text("driving_telemetry");state.driving_recording=snapshot.get("driving_recording",false);
        const godot::Array hints=snapshot.get("driving_hints",godot::Array());for(int i=0;i<hints.size();++i){const godot::String hint=hints[i];state.driving_hints.emplace_back(hint.utf8().get_data());}
        state.driving_powertrain_known=snapshot.get("powertrain_known",false);
        if(state.driving_powertrain_known){const godot::Dictionary p=snapshot.get("powertrain",godot::Dictionary());
            state.driving_speed=p.get("speed",0.f);state.driving_rpm=p.get("rpm",0.f);state.driving_gear=p.get("gear",1);
            state.driving_transmission=p.get("manual_transmission",0);state.driving_handbrake=p.get("handbrake",false);
        }
        state.driving_surface_known=snapshot.get("surface_known",false);
        if(state.driving_surface_known){const godot::Dictionary surface=snapshot.get("surface",godot::Dictionary());state.driving_surface_mode=surface.get("mode",0);state.driving_surface_terrain=surface.get("terrain",1);}
        const godot::Array materials=snapshot.get("surface_catalog",godot::Array());
        for(int i=0;i<materials.size();++i){const godot::Dictionary entry=materials[i];state.driving_materials.push_back({int(entry.get("id",1)),godot::String(entry.get("name","")).utf8().get_data(),float(entry.get("static_friction",0.0)),float(entry.get("dynamic_friction",0.0)),float(entry.get("rolling_friction",0.0))});}
        const godot::Array driving_wheels=snapshot.get("wheels",godot::Array());
        for(int i=0;i<std::min(4,int(driving_wheels.size()));++i){const godot::Dictionary wheel=driving_wheels[i];auto& g=state.driving_grip[static_cast<std::size_t>(i)];g.contact_known=wheel.has("contact");g.contact=wheel.get("contact",false);if(!wheel.has("grip"))continue;
            const godot::Dictionary raw=wheel["grip"];g.known=true;g.terrain=raw.get("terrain",0);
            const godot::Vector3 point=raw.get("ground_point",godot::Vector3());const godot::Vector4 plane=raw.get("ground_plane",godot::Vector4());
            g.ground_point={{float(point.z),float(point.x),float(point.y)}};g.ground_plane={{float(plane.x),float(plane.y),float(plane.z),float(plane.w)}};g.slipping=raw.get("slipping",false);
            g.z_force=raw.get("z_force",0.0);g.current_slip=raw.get("current_slip",0.0);g.traction_usage=raw.get("traction_usage",0.0);g.traction_boost=raw.get("traction_boost",0.0);
            g.catchup_friction=raw.get("catchup_friction",0.0);g.road_noise=raw.get("road_noise",0.0);g.static_coefficient=raw.get("static_coefficient",0.0);g.dynamic_coefficient=raw.get("dynamic_coefficient",0.0);
            const godot::Vector2 slip=raw.get("slip",godot::Vector2()),force=raw.get("ground_force",godot::Vector2());g.slip={{slip.x,slip.y}};g.ground_force={{force.x,force.y}};
        }
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
godot::Dictionary Ps2RenderHost::viewer_ui_targets()const {
    godot::Dictionary result;
#if HP2_DEBUG_BUILD
    const float scale=console_?console_->ui_scale():1.f;
    for(const auto& [name,point]:viewer_ui_.targets())result[godot::String::utf8(name.c_str())]=godot::Vector2(point[0]*scale,point[1]*scale);
#endif
    return result;
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

namespace port::godot_bridge {
// TASK-0113: immutable source loading documents; draw never advances timers.
godot::Dictionary Ps2RenderHost::draw_loading(godot::Object* resources,double width,double height){godot::Dictionary result;const auto error=catch_all([&]{auto* layout=godot::Object::cast_to<Ps2FrontEndLayout>(resources);if(!layout||!item_.is_valid()||width<=0||height<=0)throw std::invalid_argument("Loading draw requires original resources and attached canvas");const auto* loading=layout->loading_presentation();if(!loading||!loading->active())throw std::logic_error("Original loading screen not active");auto* catalog=layout->runtime().catalog();if(!catalog)throw std::logic_error("Original loading texture catalog missing");catalog->set_presentation_bundles({"GLOBAL/DYNTEX.BIN","GLOBAL/GLOBALB.BUN"});port::runtime::WidgetSizes sizes(*catalog,layout->runtime().fonts());port::render::Frame frame;frame.sequence=loading->revision();int missing=0;for(auto document:loading->documents()){port::render::Frame addition;const auto stats=port::runtime::build_frame(document,sizes,{},addition);missing+=stats.missing;port::render::append_frontend_frame(frame,addition);}if(missing)throw std::runtime_error("Original loading font/texture unavailable");output_scale_=std::min(width/640.,height/480.);const auto clip=loading->clip_bounds();frame.display_offset={-clip[0],-clip[1]};submit(frame,*catalog);result["screen_hash"]=int64_t(loading->screen_hash());result["revision"]=int64_t(loading->revision());result["quads"]=int64_t(frame.quads.size());result["missing"]=missing;});result["ok"]=error.empty();if(!error.empty())result["error"]=godot::String::utf8(error.c_str());return result;}
}
