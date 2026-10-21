#include "track_sun_renderer.hpp"
#include "camera_visibility_bounds.hpp"
#include "platform/shaders/track_shader.hpp"
#include "platform/shaders/screen_projection.hpp"
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <stdexcept>
#include <tuple>
#include <chrono>
#include <atomic>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
namespace port::godot_bridge {
namespace {std::atomic<std::uint64_t> sun_telemetry_generation{0};}
struct TrackSunRenderer::Impl {
 godot::Node3D& owner;TextureLookup lookup;MipLookup mip_lookup;std::map<std::uint32_t,unsigned> maximum_alpha;port::track::SunOcclusion occlusion;port::track::SunReadbackRect request;bool pending=false;
 godot::SubViewport* readback=nullptr;godot::TextureRect* copy=nullptr;godot::Ref<godot::ShaderMaterial> copy_material;
 bool gpu_requested=false,gpu=false,gpu_compare=false,eligibility_checked=false;std::string gpu_fallback;
 godot::SubViewport* counters[2]={nullptr,nullptr};godot::TextureRect* counter_copies[2]={nullptr,nullptr};godot::Ref<godot::ShaderMaterial> counter_materials[2];
 godot::Ref<godot::ImageTexture> zero_counter;int ready=-1,queued=-1;std::uint64_t pending_source_frame=0,counter_source_frame=0,telemetry_generation=++sun_telemetry_generation;
 bool reduction=false,compare=false;unsigned reduced_uncovered=0,reference_uncovered=0;
 std::uint64_t count_mismatches=0,completed_readbacks=0,readback_bytes=0;double readback_ms=0;
 godot::SubViewport* comparison=nullptr;godot::TextureRect* comparison_copy=nullptr;godot::Ref<godot::ShaderMaterial> comparison_material;
 std::vector<godot::MeshInstance3D*> nodes;std::map<std::tuple<std::uint32_t,bool,float,unsigned>,godot::Ref<godot::ShaderMaterial>> materials;
 Impl(godot::Node3D& n,TextureLookup l,MipLookup m):owner(n),lookup(std::move(l)),mip_lookup(std::move(m)){}
};
TrackSunRenderer::TrackSunRenderer(godot::Node3D& n,TextureLookup l,MipLookup m):impl_(std::make_unique<Impl>(n,std::move(l),std::move(m))){
 impl_->gpu_requested=bool(godot::ProjectSettings::get_singleton()->get_setting("hp2/rendering/sun_gpu_resident",false));
 const auto args=godot::OS::get_singleton()->get_cmdline_user_args();
 for(int i=0;i<args.size();++i)for(const auto* prefix:{"--track-sun-reduction=","--track-sun-reduction-compare=","--track-sun-gpu-resident=","--track-sun-gpu-compare="})if(args[i].begins_with(prefix)){
  const auto value=args[i].substr(godot::String(prefix).length());if(value!="true"&&value!="false")throw std::invalid_argument("Sun reduction switches require true or false");
  if(godot::String(prefix)=="--track-sun-reduction=")impl_->reduction=value=="true";else if(godot::String(prefix)=="--track-sun-reduction-compare=")impl_->compare=value=="true";else if(godot::String(prefix)=="--track-sun-gpu-resident=")impl_->gpu_requested=value=="true";else impl_->gpu_compare=value=="true";
 }
 if(impl_->gpu_compare&&!impl_->gpu_requested)throw std::invalid_argument("Sun GPU comparison requires GPU resident counter");
 if(impl_->gpu_requested&&impl_->compare)throw std::invalid_argument("Select GPU comparison or CPU reduction comparison separately");
 if(impl_->compare&&!impl_->reduction)throw std::invalid_argument("Sun reduction comparison requires reduction enabled");
}
TrackSunRenderer::~TrackSunRenderer(){clear();}
namespace {
// Both targets sample the same nearest source texel at the native pixel centre.
const char* sun_copy_shader="shader_type canvas_item;render_mode unshaded,blend_disabled;uniform sampler2D raw_depth : filter_nearest,repeat_disable;uniform ivec2 source_origin;void fragment(){ivec2 native_p=source_origin+ivec2(FRAGCOORD.xy);vec2 uv=(vec2(native_p)+vec2(0.5))/vec2(512.0,448.0);ivec2 s=textureSize(raw_depth,0);ivec2 p=ivec2(floor(uv*vec2(s)));COLOR=(native_p.x>=0&&native_p.y>=0&&native_p.x<512&&native_p.y<448)?texelFetch(raw_depth,p,0):vec4(0.0);}";
const char* sun_reduce_shader=R"(shader_type canvas_item;render_mode unshaded,blend_disabled;
uniform sampler2D raw_depth : filter_nearest,repeat_disable;
uniform ivec2 source_origin;uniform ivec2 clip_start;uniform ivec2 clip_end;uniform int producer_frame=0;
void fragment(){
 int count=0;ivec2 size=textureSize(raw_depth,0);
 for(int y=0;y<32;y++){for(int x=0;x<32;x++){
  if(x>=clip_start.x&&x<clip_end.x&&y>=clip_start.y&&y<clip_end.y){
   ivec2 native_p=source_origin+ivec2(x,y);int raw=0;
   if(native_p.x>=0&&native_p.y>=0&&native_p.x<512&&native_p.y<448){
    vec2 uv=(vec2(native_p)+vec2(0.5))/vec2(512.0,448.0);
    ivec3 bytes=ivec3(floor(texelFetch(raw_depth,ivec2(floor(uv*vec2(size))),0).rgb*255.0+vec3(0.5)));
    raw=bytes.r+bytes.g*256+bytes.b*65536;
   }
   if(raw<2){count++;}
  }
 }}
 // RG is the original count. BA preserves the CPU producer frame across an
 // asynchronous copy; each row stores the same low/high16-bit halves.
 uint frame_half=(uint(producer_frame)>>uint((int(FRAGCOORD.x)&1)*16))&65535u;
 COLOR=vec4(float(count%256)/255.0,float(count/256)/255.0,float(frame_half&255u)/255.0,float(frame_half>>8u)/255.0);
})";
void create_sun_target(godot::Node3D& owner,bool reduction,godot::SubViewport*& target,godot::TextureRect*& copy,godot::Ref<godot::ShaderMaterial>& material){
 target=memnew(godot::SubViewport);target->set_name(reduction?"TrackSunReduction":"TrackSunReadback");// Godot 4.4 SubViewport requires at least 2x2; all four pixels carry the same RG count; BA carries producer metadata.
 const int extent=reduction?2:32;target->set_size({extent,extent});target->set_disable_3d(true);target->set_transparent_background(true);target->set_update_mode(godot::SubViewport::UPDATE_DISABLED);owner.add_child(target);
 copy=memnew(godot::TextureRect);copy->set_size({float(extent),float(extent)});target->add_child(copy);godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code(reduction?sun_reduce_shader:sun_copy_shader);material.instantiate();material->set_shader(shader);copy->set_material(material);
}
godot::PackedByteArray sun_target_bytes(godot::SubViewport* viewport,int extent){
 const auto image=viewport->get_texture()->get_image();if(image.is_null()||image->get_width()!=extent||image->get_height()!=extent)throw std::runtime_error("Sun depth transfer dimensions do not match scheduled target");image->convert(godot::Image::FORMAT_RGBA8);const auto bytes=image->get_data();if(bytes.size()!=extent*extent*4)throw std::runtime_error("Sun depth transfer is not RGBA8");return bytes;
}
unsigned sun_reference_count(const godot::PackedByteArray& bytes,const port::track::SunReadbackRect& request,port::track::SunOcclusion& occlusion){
 for(unsigned i=0;i<1024;++i)occlusion.pixels[i]=std::uint32_t(bytes[i*4])|(std::uint32_t(bytes[i*4+1])<<8)|(std::uint32_t(bytes[i*4+2])<<16);occlusion.rectangle=request;return occlusion.uncovered();
}
}
void TrackSunRenderer::clear(){for(auto* n:impl_->nodes){if(n->get_parent())n->get_parent()->remove_child(n);memdelete(n);}impl_->nodes.clear();impl_->materials.clear();impl_->maximum_alpha.clear();
 for(auto* viewport:{impl_->readback,impl_->comparison,impl_->counters[0],impl_->counters[1]})if(viewport){if(viewport->get_parent())viewport->get_parent()->remove_child(viewport);memdelete(viewport);}
 impl_->ready=impl_->queued=-1;impl_->counter_source_frame=0;impl_->telemetry_generation=++sun_telemetry_generation;impl_->gpu=false;impl_->eligibility_checked=false;impl_->gpu_fallback.clear();impl_->zero_counter.unref();
 for(int i=0;i<2;++i){impl_->counters[i]=nullptr;impl_->counter_copies[i]=nullptr;impl_->counter_materials[i].unref();}
 impl_->readback=nullptr;impl_->copy=nullptr;impl_->copy_material.unref();impl_->comparison=nullptr;impl_->comparison_copy=nullptr;impl_->comparison_material.unref();impl_->pending=false;impl_->occlusion={};impl_->reduced_uncovered=impl_->reference_uncovered=0;impl_->count_mismatches=impl_->completed_readbacks=impl_->readback_bytes=0;impl_->readback_ms=0;
}
void TrackSunRenderer::complete_readback(){
 if(!impl_->pending)return;
 // Publish only an actually scheduled transfer. Normal GPU transport never maps
 // the texture to the CPU; the next update samples this completed buffer.
 if(impl_->gpu){
  if(impl_->queued<0)throw std::runtime_error("Sun GPU transfer missing producer buffer");
  if(impl_->gpu_compare){
   const auto bytes=sun_target_bytes(impl_->counters[impl_->queued],2);
   impl_->reduced_uncovered=unsigned(bytes[0])|(unsigned(bytes[1])<<8);
   const auto reference=sun_target_bytes(impl_->comparison,32);
   impl_->reference_uncovered=sun_reference_count(reference,impl_->request,impl_->occlusion);
   if(impl_->reduced_uncovered>1024||impl_->reference_uncovered!=impl_->reduced_uncovered)throw std::runtime_error("Sun resident GPU counter differs from reference");
   impl_->readback_bytes=bytes.size()+reference.size();
  }else{impl_->readback_bytes=0;impl_->readback_ms=0;}
  impl_->ready=impl_->queued;impl_->queued=-1;impl_->counter_source_frame=impl_->pending_source_frame;++impl_->completed_readbacks;impl_->pending=false;return;
 }
 if(!impl_->readback)return;const auto begin=std::chrono::steady_clock::now();
 const auto bytes=sun_target_bytes(impl_->readback,impl_->reduction?2:32);impl_->readback_bytes=bytes.size();
 if(impl_->reduction){impl_->reduced_uncovered=unsigned(bytes[0])|(unsigned(bytes[1])<<8);if(impl_->reduced_uncovered>1024)throw std::runtime_error("Sun reduction count exceeds original transfer extent");}
 else impl_->reference_uncovered=sun_reference_count(bytes,impl_->request,impl_->occlusion);
 if(impl_->compare){const auto reference=sun_target_bytes(impl_->comparison,32);impl_->readback_bytes+=reference.size();impl_->reference_uncovered=sun_reference_count(reference,impl_->request,impl_->occlusion);if(impl_->reference_uncovered!=impl_->reduced_uncovered)++impl_->count_mismatches;}
 impl_->readback_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();++impl_->completed_readbacks;impl_->pending=false;
 if(impl_->count_mismatches)throw std::runtime_error("Sun GPU reduction differs from original 32x32 reference count");
}
godot::Array TrackSunRenderer::scheduled_viewports()const{
 godot::Array result;if(impl_->pending){auto* target=impl_->gpu?(impl_->queued>=0?impl_->counters[impl_->queued]:nullptr):impl_->readback;if(target)result.push_back(target->get_viewport_rid());if(impl_->comparison)result.push_back(impl_->comparison->get_viewport_rid());}return result;
}

godot::Dictionary TrackSunRenderer::stats()const{
 godot::Dictionary d;d["gpu_requested"]=impl_->gpu_requested;d["gpu_resident"]=impl_->gpu;d["gpu_fallback"]=godot::String(impl_->gpu_fallback.c_str());d["gpu_compare"]=impl_->gpu_compare;
 d["counter_frame_domain"]="engine_process_frame";d["counter_available"]=impl_->gpu&&impl_->ready>=0;d["counter_source_frame"]=int64_t(impl_->counter_source_frame);d["telemetry_generation"]=int64_t(impl_->telemetry_generation);
 if(impl_->gpu&&impl_->ready>=0)d["counter_texture"]=impl_->counters[impl_->ready]->get_texture()->get_rid();
 d["reduction_enabled"]=impl_->reduction;d["compare_enabled"]=impl_->compare;
 d["uncovered"]=impl_->gpu&&!impl_->gpu_compare?godot::Variant():godot::Variant(impl_->gpu||impl_->reduction?impl_->reduced_uncovered:impl_->reference_uncovered);
 d["reference_uncovered"]=impl_->gpu&&!impl_->gpu_compare?godot::Variant():godot::Variant(impl_->reference_uncovered);d["count_mismatches"]=int64_t(impl_->count_mismatches);d["completed_readbacks"]=int64_t(impl_->completed_readbacks);d["readback_bytes"]=int64_t(impl_->readback_bytes);d["readback_ms"]=impl_->gpu_compare?godot::Variant():godot::Variant(impl_->readback_ms);return d;
}
SunFrame TrackSunRenderer::update(godot::Camera3D* camera,const port::track::Scene& scene,port::driving::Memory& math,godot::Ref<godot::Texture2D> depth,bool enabled,unsigned layer){
 SunFrame result;result.uncovered_available=!impl_->gpu||impl_->gpu_compare;if(!enabled||!camera||!scene.environment.sun.available){for(auto* n:impl_->nodes)n->hide();return result;}
 // Eligibility is immutable until clear/reload. The host source-over path has
 // a proven alpha domain; check every original count, never clamp the result.
 if(!impl_->eligibility_checked){
  impl_->gpu=impl_->gpu_requested;
  for(const auto& l:scene.environment.sun.layers){if(l.intensity==0)continue;const auto hash=scene.environment.sun_textures.at(l.texture);
   const auto texture=std::find_if(scene.textures.begin(),scene.textures.end(),[&](const auto& t){return t.header.tex_hash==hash;});if(texture==scene.textures.end())throw std::runtime_error("Original sun texture dependency absent");
   unsigned maximum=0;for(std::size_t p=3;p<texture->bitmap.rgba.size();p+=4)maximum=std::max(maximum,unsigned(texture->bitmap.rgba[p]));
   if(const auto levels=scene.mipmaps.find(hash);levels!=scene.mipmaps.end())for(const auto& image:levels->second)for(std::size_t p=3;p<image.rgba.size();p+=4)maximum=std::max(maximum,unsigned(image.rgba[p]));
   impl_->maximum_alpha[hash]=maximum;
   if(l.alpha_type==0)for(unsigned count=0;count<=1024;++count){const float coverage=float(count)*(1.f/1024.f);const float a=l.intensity<0?-l.intensity*128.f:coverage*l.intensity*128.f;if(unsigned(int(a)&255)*maximum>=129u*128u){impl_->gpu=false;impl_->gpu_fallback="Original alpha exceeds proven host source-over range";break;}}
  }
  if(impl_->gpu){godot::PackedByteArray bytes;bytes.resize(16);bytes.fill(0);impl_->zero_counter=godot::ImageTexture::create_from_image(godot::Image::create_from_data(2,2,false,godot::Image::FORMAT_RGBA8,bytes));}
  impl_->eligibility_checked=true;
 }
 const auto size=camera->get_viewport()->get_visible_rect().size;const unsigned width=unsigned(size.x),height=unsigned(size.y);const auto& position=scene.environment.sun.position;const godot::Vector3 world(position[1],position[2],position[0]);const auto projected=camera->unproject_position(world);if(!width||!height)throw std::runtime_error("Sun host viewport is empty");const godot::Vector2 screen(projected.x*512.f/float(width),projected.y*448.f/float(height));const float z=-camera->get_global_transform().affine_inverse().xform(world).z;result.uncovered=impl_->gpu?(impl_->gpu_compare?impl_->reduced_uncovered:0):(impl_->reduction?impl_->reduced_uncovered:impl_->occlusion.uncovered());result.uncovered_available=!impl_->gpu||impl_->gpu_compare;
 if(depth.is_null()){for(auto* n:impl_->nodes)n->hide();result.gaps.push_back("Sun requires original GS depth comparison source");return result;}if(depth->get_width()!=int(width)||depth->get_height()!=int(height))throw std::runtime_error("Sun depth source must match host viewport");
 const auto visibility_bounds=camera_visibility_corners(camera);
 const auto quads=port::track::sun_quads(math,scene.environment,1,screen.x,screen.y,z,512,448,16777215.f,result.uncovered);
 std::vector<float> intensities;for(const auto& l:scene.environment.sun.layers)if(l.intensity!=0)intensities.push_back(l.intensity);
 if(!quads.empty()&&quads.size()!=intensities.size())throw std::runtime_error("Sun layer-to-quad correspondence changed");
 while(impl_->nodes.size()<quads.size()){auto* n=memnew(godot::MeshInstance3D);n->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);impl_->owner.add_child(n);impl_->nodes.push_back(n);}
 for(std::size_t i=0;i<impl_->nodes.size();++i){auto* node=impl_->nodes[i];if(i>=quads.size()){node->hide();continue;}const auto& q=quads[i];const auto found_texture=std::find_if(scene.textures.begin(),scene.textures.end(),[&](const auto& t){return t.header.tex_hash==q.texture_hash;});if(found_texture==scene.textures.end())throw std::runtime_error("Original sun texture dependency absent");if(!found_texture->header.Field_0x4d)throw std::runtime_error("Unsorted sun texture requires original solid attachment ordering");const auto key=std::make_tuple(q.texture_hash,q.additive,q.native_depth,unsigned(i));auto found=impl_->materials.find(key);
 if(found==impl_->materials.end()){unsigned maximum=0;for(std::size_t pixel=3;pixel<found_texture->bitmap.rgba.size();pixel+=4)maximum=std::max(maximum,unsigned(found_texture->bitmap.rgba[pixel]));if(const auto levels=scene.mipmaps.find(q.texture_hash);levels!=scene.mipmaps.end())for(const auto& image:levels->second)for(std::size_t pixel=3;pixel<image.rgba.size();pixel+=4)maximum=std::max(maximum,unsigned(image.rgba[pixel]));impl_->maximum_alpha[q.texture_hash]=maximum;auto header=found_texture->header;header.alpha_bits=q.additive?0x48:0x44;header.alpha_fix=0;port::track::EffectState effect;effect.texture=port::track::texture_state_00108CC0(header);effect.test={0x5000b,false,5,0,2};auto code=track_shader_source(header,effect,port::track::Fusion40::None);const auto modes=code.find("shadows_disabled,");code.insert(modes+std::string("shadows_disabled,").size()," depth_test_disabled,");const auto vertex=code.find("void vertex(){"),fragment=code.find("void fragment",vertex),brace=code.rfind('}',fragment);if(impl_->gpu)code.insert(brace,"\n gs_rgba.a=hp2_sun_alpha(hp2_sun_count(textureLod(sun_previous_count,vec2(0.5),0.0)),sun_intensity);\n");const auto updated_brace=code.rfind('}',code.find("void fragment",vertex));code.insert(updated_brace,"\n    POSITION=hp2_host_screen_position(VERTEX.xy/sun_viewport,PROJECTION_MATRIX[1][1]);\n    gs_color_w=1.0;gs_color_numerator=gs_rgba;\n");if(impl_->gpu)code.insert(vertex,"#include \"res://shaders/sun_counter.gdshaderinc\"\nuniform sampler2D sun_previous_count : filter_nearest,repeat_disable;\nuniform float sun_intensity;\n");code.insert(vertex,std::string(screen_position_shader)+"uniform vec2 sun_viewport;\nuniform sampler2D sun_raw_depth : filter_nearest, repeat_disable;\nuniform float sun_native_depth;\n");const auto fb=code.find('{',code.find("void fragment"));code.insert(fb+1,"\nvec3 sun_z_bytes=floor(textureLod(sun_raw_depth,SCREEN_UV,0.0).rgb*255.0+0.5);float sun_z=dot(sun_z_bytes,vec3(1.0,256.0,65536.0));if(sun_native_depth<sun_z)discard;\n");godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code(godot::String::utf8(code.c_str()));godot::Ref<godot::ShaderMaterial> material;material.instantiate();material->set_shader(shader);material->set_shader_parameter("base_lit",false);material->set_render_priority(127-int(i));found=impl_->materials.emplace(key,material).first;}
 if(!q.additive&&unsigned(q.colour[3])*impl_->maximum_alpha.at(q.texture_hash)>=129u*128u)throw std::runtime_error("Sun source-over alpha exceeds proven host blend range");
 const auto material=found->second;if(impl_->gpu){godot::Ref<godot::Texture2D> counter=impl_->zero_counter;if(impl_->ready>=0)counter=impl_->counters[impl_->ready]->get_texture();material->set_shader_parameter("sun_previous_count",counter);material->set_shader_parameter("sun_intensity",intensities.at(i));}for(unsigned level=1;level<=found_texture->header.Field_0x6e;++level){if(!impl_->mip_lookup)throw std::runtime_error("Original sun mip resource callback missing");material->set_shader_parameter(godot::String::utf8(("mip_"+std::to_string(level)).c_str()),impl_->mip_lookup(q.texture_hash,level));}material->set_shader_parameter("sun_viewport",godot::Vector2(512,448));material->set_shader_parameter("sun_raw_depth",depth);material->set_shader_parameter("sun_native_depth",q.native_depth);material->set_shader_parameter("base_texture",impl_->lookup(q.texture_hash));godot::PackedVector3Array points;godot::PackedVector2Array uv;godot::PackedColorArray colours;godot::PackedInt32Array indices;for(unsigned j=0;j<4;++j){points.push_back({q.vertices[j][0],q.vertices[j][1],0});const float u[4]={0,1,1,0},v[4]={0,0,1,1};uv.push_back({u[j],v[j]});colours.push_back(godot::Color(q.colour[0]/255.f,q.colour[1]/255.f,q.colour[2]/255.f,q.colour[3]/255.f));}for(int index:{0,1,3,1,2,3})indices.push_back(index);godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);arrays[godot::Mesh::ARRAY_VERTEX]=points;arrays[godot::Mesh::ARRAY_TEX_UV]=uv;arrays[godot::Mesh::ARRAY_COLOR]=colours;arrays[godot::Mesh::ARRAY_INDEX]=indices;godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays);node->set_mesh(mesh);apply_camera_visibility_bounds(node,visibility_bounds);node->set_material_override(material);node->set_layer_mask(layer);node->set_sorting_use_aabb_center(false);node->show();result.alpha_keys.push_back({7,q.native_depth});++result.quads;
 }
 const auto request=port::track::sun_readback_rect(scene.environment.sun,1,screen.x,screen.y,z,512,448);
 if(request.requested){if(depth.is_null()){result.gaps.push_back("Sun depth source unavailable; original occlusion transfer pending");return result;}if(depth->get_width()!=int(width)||depth->get_height()!=int(height))throw std::runtime_error("Sun depth source must match original projected viewport");
 if(request.start_x<0||request.start_y<0||request.end_x>32||request.end_y>32)throw std::runtime_error("Sun readback clipped rectangle exceeds32x32");
 if(impl_->gpu){impl_->queued=impl_->ready==0?1:0;if(!impl_->counters[impl_->queued])create_sun_target(impl_->owner,true,impl_->counters[impl_->queued],impl_->counter_copies[impl_->queued],impl_->counter_materials[impl_->queued]);}
 if(!impl_->gpu&&!impl_->readback)create_sun_target(impl_->owner,impl_->reduction,impl_->readback,impl_->copy,impl_->copy_material);
 if((impl_->compare||(impl_->gpu&&impl_->gpu_compare))&&!impl_->comparison)create_sun_target(impl_->owner,false,impl_->comparison,impl_->comparison_copy,impl_->comparison_material);
 impl_->request=request;impl_->pending_source_frame=godot::Engine::get_singleton()->get_process_frames();
 const auto schedule=[&](godot::SubViewport* target,godot::TextureRect* copy,const godot::Ref<godot::ShaderMaterial>& material,bool reduction){
  copy->set_texture(depth);material->set_shader_parameter("raw_depth",depth);material->set_shader_parameter("source_origin",godot::Vector2i(request.x,request.y));
  if(reduction){material->set_shader_parameter("producer_frame",int64_t(impl_->pending_source_frame));material->set_shader_parameter("clip_start",godot::Vector2i(request.start_x,request.start_y));material->set_shader_parameter("clip_end",godot::Vector2i(request.end_x,request.end_y));}
  target->set_update_mode(godot::SubViewport::UPDATE_ONCE);
 };
 if(impl_->gpu)schedule(impl_->counters[impl_->queued],impl_->counter_copies[impl_->queued],impl_->counter_materials[impl_->queued],true);else schedule(impl_->readback,impl_->copy,impl_->copy_material,impl_->reduction);if(impl_->compare||(impl_->gpu&&impl_->gpu_compare))schedule(impl_->comparison,impl_->comparison_copy,impl_->comparison_material,false);impl_->pending=true;result.readback_pending=true;

 }
 return result;
}
void TrackSunRenderer::set_alpha_ranks(const std::vector<std::size_t>& ranks,std::size_t total,float origin_depth){
 if(ranks.size()>impl_->nodes.size())throw std::invalid_argument("Sun alpha ranks exceed prepared nodes");
 for(std::size_t i=0;i<ranks.size();++i){if(ranks[i]>=total)throw std::invalid_argument("Sun alpha rank exceeds common submission");godot::Ref<godot::ShaderMaterial> material=impl_->nodes[i]->get_material_override();material->set_render_priority(64);impl_->nodes[i]->set_sorting_offset(origin_depth-float(total-ranks[i])*4096.f);}
}

}

