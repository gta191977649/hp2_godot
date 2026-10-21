#include "track_postprocess.hpp"
#include "vehicle_depth_staging.hpp"
#include "platform/render/depth_sync.hpp"
#include "game_core/src/ecstasy/eenveffectse.hpp"
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <stdexcept>
#include <map>
#include <set>
#include <functional>
#include <exception>
#include "game_core/port/not_implemented.hpp"
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/os.hpp>
namespace port::godot_bridge {
namespace {
constexpr unsigned depth_layer=1u<<19;
// Godot scene/resource mutation occurs on its main thread. One compositor owns
// a camera mask at a time; this is host ownership, not original game behavior.
std::map<std::uint64_t,const void*>& camera_owners(){static std::map<std::uint64_t,const void*> owners;return owners;}
const char* composite_code=
#include "platform/shaders/postprocess_composite.glsl.inc"
;
godot::Ref<godot::ShaderMaterial> material(const char* code){
 godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code(code);
 godot::Ref<godot::ShaderMaterial> result;result.instantiate();result->set_shader(shader);return result;
}
godot::Ref<godot::ImageTexture> palette(const port::track::postprocess::Palette& p){
 godot::PackedByteArray bytes;bytes.resize(1024);auto* out=bytes.ptrw();
 for(unsigned i=0;i<256;i++)for(unsigned k=0;k<4;k++)out[4*i+k]=std::uint8_t(p[i]>>(8*k));
 return godot::ImageTexture::create_from_image(godot::Image::create_from_data(256,1,false,godot::Image::FORMAT_RGBA8,bytes));
}
void copy_camera(godot::Camera3D* target,godot::Camera3D* source,unsigned mask,bool reuse){
 if(!reuse||target->get_projection()!=source->get_projection())target->set_projection(source->get_projection());if(!reuse||target->get_fov()!=source->get_fov())target->set_fov(source->get_fov());
 if(!reuse||target->get_size()!=source->get_size())target->set_size(source->get_size());if(!reuse||target->get_frustum_offset()!=source->get_frustum_offset())target->set_frustum_offset(source->get_frustum_offset());
 if(!reuse||target->get_near()!=source->get_near())target->set_near(source->get_near());if(!reuse||target->get_far()!=source->get_far())target->set_far(source->get_far());
 if(!reuse||target->get_h_offset()!=source->get_h_offset())target->set_h_offset(source->get_h_offset());if(!reuse||target->get_v_offset()!=source->get_v_offset())target->set_v_offset(source->get_v_offset());
 if(!reuse||target->get_keep_aspect_mode()!=source->get_keep_aspect_mode())target->set_keep_aspect_mode(source->get_keep_aspect_mode());if(!reuse||target->get_environment()!=source->get_environment())target->set_environment(source->get_environment());
 if(!reuse||target->get_global_transform()!=source->get_global_transform())target->set_global_transform(source->get_global_transform());if(!reuse||target->get_cull_mask()!=mask)target->set_cull_mask(mask);if(!reuse||!target->is_current())target->set_current(true);
}
}
struct TrackPostprocess::Impl {
 bool reuse=false,occlusion=false,consumer_cache=true,parameter_notifications=true;
 bool vehicle_staging=false;std::uint64_t staged_reads=0,staged_misses=0;
 bool structure_observer=false;port::render::DepthDirtyState dirty;
 godot::Callable order_observer;std::set<std::uint64_t> order_parents;
 godot::Transform3D owner_transform;bool owner_transform_valid=false;
 bool discovery_cache=false;std::uint64_t discovery_scans=0,discovery_visits=0,proxy_creations=0;
 bool skip_source_3d=false,source_3d_acquired=false,source_3d_was_disabled=false;std::uint64_t source_viewport_id=0;std::uint64_t parameter_sets=0,parameter_skips=0,uniform_reads=0,direct_parameter_sets=0,direct_parameter_skips=0;
 std::map<std::uint64_t,std::map<godot::StringName,godot::Variant>> parameter_values;
 std::set<std::uint64_t> instance_sources;std::map<std::uint64_t,godot::MultiMeshInstance3D*> instance_proxies;
 godot::Ref<godot::ShaderMaterial> depth_material(const godot::Ref<godot::ShaderMaterial>&,float,float,std::set<std::uint64_t>&);
 void parameter(const godot::Ref<godot::ShaderMaterial>& material,const godot::StringName& name,const godot::Variant& value,bool consumer=false){
  if((!consumer&&reuse)||(consumer&&consumer_cache)){auto& values=parameter_values[material->get_instance_id()];const auto old=values.find(name);if(old!=values.end()&&old->second==value){++parameter_skips;return;}values[name]=value;}
  material->set_shader_parameter(name,value);++parameter_sets;
 }

 struct Variant {godot::Ref<godot::ShaderMaterial> source,depth;godot::Array uniforms,values;std::vector<godot::StringName> names,dynamic_names;std::map<godot::StringName,int> uniform_slots;std::uint64_t shader_id=0;std::uint64_t staged_generation=0,staged_version=0;bool dynamic_contract=false,first_sync=true;float near_z=0,far_z=0;};
 std::map<std::uint64_t,Variant> variants;std::map<std::uint64_t,std::set<std::uint64_t>> material_meshes,mesh_materials;
 std::map<std::uint64_t,godot::Ref<godot::Shader>> shaders;
 std::map<std::uint64_t,bool> shader_writes;std::set<std::uint64_t> nonwriting_nodes;
 std::set<std::uint64_t> dirty_materials,tracked_materials,tracked_meshes,dirty_meshes,visible_sources;
 std::vector<std::uint64_t> source_meshes;int scene_node_count=-1;bool discovery_dirty=true;
 std::map<std::uint64_t,godot::MeshInstance3D*> proxies;
 std::map<std::uint64_t,std::pair<float,bool>> sorting_cache;
 port::track::postprocess::Palette cached_fog{},cached_focus{};
 bool cache_valid=false,fog_active=false,focus_active=false,history_ready=false;bool history_alpha_known[2]={false,false};
 bool depth_alpha_active=false,bw_active=false,filter_active=false,radial_active=false,previous_active=false;
 godot::SubViewport* history[2]={nullptr,nullptr};godot::TextureRect* history_rect[2]={nullptr,nullptr};unsigned history_index=0;
 godot::Ref<godot::Texture2D> filter_texture,radial_texture;
 std::uint64_t owner_id=0,source_id=0,output_id=0;std::array<std::uint64_t,5> owned_ids{};unsigned source_mask=0;
 godot::SubViewport* colour=nullptr;godot::SubViewport* depth=nullptr;
 godot::Camera3D* colour_camera=nullptr;godot::Camera3D* depth_camera=nullptr;
 godot::CanvasLayer* canvas=nullptr;godot::TextureRect* output=nullptr;
 godot::Ref<godot::ShaderMaterial> composite_material;
 godot::Ref<godot::ImageTexture> fog,focus,black_white,initial_display;
 godot::Ref<godot::Environment> depth_environment;
};
godot::Ref<godot::ShaderMaterial> TrackPostprocess::Impl::depth_material(const godot::Ref<godot::ShaderMaterial>& original,float n,float f,std::set<std::uint64_t>& synced_materials){
 auto& p=*this;
     auto& variant=p.variants[original->get_instance_id()];
     const auto shader_id=original->get_shader()->get_instance_id();if(variant.depth.is_valid()&&variant.shader_id!=shader_id)variant=Variant{};
     if(variant.depth.is_null()){
      auto& cached_shader=p.shaders[original->get_shader()->get_instance_id()];
      if(cached_shader.is_null()){
      std::string depth=original->get_shader()->get_code().utf8().get_data();
      const auto start=depth.find("void fragment()");if(start==std::string::npos)throw std::runtime_error("Track depth proxy fragment entry unsupported");
      const auto open=depth.find('{',start);std::size_t end=open+1;int nesting=1;for(;end<depth.size()&&nesting;end++){if(depth[end]=='{')nesting++;if(depth[end]=='}')nesting--;}
      if(nesting)throw std::runtime_error("Track depth proxy fragment braces invalid");
      const bool writing=depth.find("depth_draw_never")==std::string::npos;
      const bool bias=depth.find("hp2_track_depth_coeff")!=std::string::npos;
      const bool particle=depth.find("particle_original_depth")!=std::string::npos;
      if(writing&&depth.find("hp2_framebuffer_rgb")==std::string::npos)throw std::runtime_error("Track depth proxy unsupported shader framebuffer conversion");
      std::string output=writing?std::string("\nfloat pp_lo=uintBitsToFloat(")+(bias?"0x40000040u":"0x40000010u")+");float pp_hi=uintBitsToFloat("+(bias?"0x40802000u":"0x40800000u")+");float pp_a=(-pp_hi*pp_near+pp_lo*pp_far)/(pp_far-pp_near);float pp_b=pp_far*pp_near*(pp_hi-pp_lo)/(pp_far-pp_near);uint pp_z=(floatBitsToUint(pp_a+pp_b/max(-VERTEX.z,0.000001))>>4u)&0xffffffu;ALBEDO=hp2_framebuffer_rgb(vec3(float(pp_z&255u),float((pp_z>>8u)&255u),float((pp_z>>16u)&255u))/255.0);ALPHA=1.0;\n":"\ndiscard;\n";
      if(particle&&writing)output="\nuint pp_z=uint(particle_z)&0xffffffu;ALBEDO=hp2_framebuffer_rgb(vec3(float(pp_z&255u),float((pp_z>>8u)&255u),float((pp_z>>16u)&255u))/255.0);ALPHA=1.0;\n";
      depth.insert(end-1,output);depth.insert(depth.find("void vertex()"),"uniform float pp_near=0.5;uniform float pp_far=500.0;\n");
      for(const char* blend:{"blend_add","blend_sub","blend_mul","blend_premul_alpha"}){const auto pos=depth.find(blend);if(pos!=std::string::npos)depth.replace(pos,std::char_traits<char>::length(blend),"blend_mix");}
      cached_shader.instantiate();cached_shader->set_code(depth.c_str());
      }
      variant.source=original;variant.depth.instantiate();variant.depth->set_shader(cached_shader);variant.depth->set_render_priority(original->get_render_priority());variant.uniforms=original->get_shader()->get_shader_uniform_list();variant.values.resize(variant.uniforms.size());variant.shader_id=shader_id;
      for(int u=0;u<variant.uniforms.size();++u){const godot::Dictionary uniform=variant.uniforms[u];const godot::StringName name=uniform["name"];variant.names.push_back(name);variant.uniform_slots.emplace(name,u);}
      if(original->has_meta("hp2_depth_uniform_contract")&&int64_t(original->get_meta("hp2_depth_uniform_contract"))==1&&original->has_meta("hp2_depth_dynamic_uniforms")){
       const auto names=original->get_meta("hp2_depth_dynamic_uniforms");if(names.get_type()==godot::Variant::PACKED_STRING_ARRAY){variant.dynamic_contract=true;const godot::PackedStringArray list=names;for(int i=0;i<list.size();++i){const godot::StringName name=list[i];if(!variant.uniform_slots.count(name))throw std::runtime_error("Declared dynamic depth uniform absent from actual shader");variant.dynamic_names.push_back(name);}}
      }
      p.dirty_materials.insert(original->get_instance_id());
     }
     if(synced_materials.insert(original->get_instance_id()).second){
      if(p.dirty_materials.count(original->get_instance_id())||!p.tracked_materials.count(original->get_instance_id())){
       const bool full=variant.first_sync||p.dirty_materials.count(original->get_instance_id())||!p.reuse||!variant.dynamic_contract;
       const auto& names=full?variant.names:variant.dynamic_names;
       vehicle_depth_staging::Registry::Snapshot staged;
       const bool staged_valid=p.vehicle_staging&&!full&&vehicle_depth_staging::read_snapshot(original->get_instance_id(),shader_id,staged);
       const bool unchanged=staged_valid&&variant.staged_generation==staged.generation&&variant.staged_version==staged.version;
       for(const auto& name:names){const int u=variant.uniform_slots.at(name);godot::Variant value;bool published=false;
        if(staged_valid)for(std::size_t i=0;i<staged.names.size();++i)if(staged.names[i]==name&&staged.valid[i]){published=true;++p.staged_reads;if(!unchanged)value=staged.values[i];break;}
        if(published&&unchanged)continue;
        if(!published){value=original->get_shader_parameter(name);++p.uniform_reads;if(p.vehicle_staging&&!full)++p.staged_misses;}
        if(variant.values[u]!=value){variant.depth->set_shader_parameter(name,value);variant.values[u]=value;}
       }
       if(staged_valid){variant.staged_generation=staged.generation;variant.staged_version=staged.version;}else{variant.staged_generation=variant.staged_version=0;}
       variant.first_sync=false;
      }
      if(variant.near_z!=n){variant.depth->set_shader_parameter("pp_near",n);variant.near_z=n;}if(variant.far_z!=f){variant.depth->set_shader_parameter("pp_far",f);variant.far_z=f;}
     }
 return variant.depth;
}
void TrackPostprocess::set_resource_reuse(bool enabled){impl_->reuse=enabled;}
void TrackPostprocess::set_occlusion_enabled(bool enabled){impl_->occlusion=enabled;}
void TrackPostprocess::mark_instance_batch(godot::MultiMeshInstance3D* node){if(node)impl_->instance_sources.insert(node->get_instance_id());}
godot::Array TrackPostprocess::scheduled_viewports()const{godot::Array result;if(active()){result.push_back(impl_->colour->get_viewport_rid());result.push_back(impl_->depth->get_viewport_rid());result.push_back(impl_->history[impl_->history_index]->get_viewport_rid());}return result;}
godot::Dictionary TrackPostprocess::stats()const{
 const auto& p=*impl_;godot::Dictionary d;d["vehicle_depth_staging"]=p.vehicle_staging;d["staged_parameter_reads"]=int64_t(p.staged_reads);d["staged_snapshot_reads"]=int64_t(vehicle_depth_staging::stats().snapshot_reads);d["staged_parameter_misses"]=int64_t(p.staged_misses);d["discovery_cache"]=p.discovery_cache;d["structure_observer"]=p.structure_observer;d["source_structure_epoch"]=int64_t(p.dirty.structure_epoch);d["discovery_scans"]=int64_t(p.discovery_scans);d["discovery_visits"]=int64_t(p.discovery_visits);d["proxy_creations"]=int64_t(p.proxy_creations);d["parameter_notifications"]=p.parameter_notifications;d["direct_parameter_sets"]=int64_t(p.direct_parameter_sets);d["direct_parameter_skips"]=int64_t(p.direct_parameter_skips);d["uniform_reads"]=int64_t(p.uniform_reads);d["parameter_sets"]=int64_t(p.parameter_sets);d["parameter_skips"]=int64_t(p.parameter_skips);d["depth_proxies"]=int64_t(p.proxies.size());d["instance_depth_proxies"]=int64_t(p.instance_proxies.size());d["depth_materials"]=int64_t(p.variants.size());d["depth_shaders"]=int64_t(p.shaders.size());
 const bool enabled=active();d["consumer_cache"]=p.consumer_cache;d["active"]=enabled;d["fog_active"]=enabled&&p.fog_active;d["focus_active"]=enabled&&p.focus_active;d["depth_alpha_active"]=enabled&&p.depth_alpha_active;d["black_white_active"]=enabled&&p.bw_active;d["filter_active"]=enabled&&p.filter_active;d["radial_active"]=enabled&&p.radial_active;d["previous_active"]=enabled&&p.previous_active;d["history_ready"]=p.history_ready;d["history_index"]=int64_t(p.history_index);
 godot::Array targets;const char* roles[]{"colour","native_depth","history_0","history_1"};
 for(unsigned i=0;i<4;++i){godot::Dictionary target;target["role"]=roles[i];auto* viewport=godot::Object::cast_to<godot::SubViewport>(godot::ObjectDB::get_instance(p.owned_ids[i+1]));
  const bool scheduled=enabled&&viewport&&(i<2||i-2==p.history_index);target["scheduled"]=scheduled;
  if(viewport){target["size"]=viewport->get_size();target["update_mode"]=int64_t(viewport->get_update_mode());target["rid"]=viewport->get_viewport_rid();d[godot::String(roles[i])+"_size"]=viewport->get_size();}
  targets.push_back(target);
 }
 d["targets"]=targets;d["scheduled_viewport_count"]=enabled?3:0;d["depth_route"]="encoded geometry proxies; transparent writers retained";d["history_route"]="full-size ping-pong; availability semantics unchanged";d["staged_focus"]=false;d["source_3d_skip_enabled"]=p.skip_source_3d;d["source_3d_suppressed"]=p.source_3d_acquired;return d;
}
TrackPostprocess::TrackPostprocess():impl_(std::make_unique<Impl>()){
 const auto args=godot::OS::get_singleton()->get_cmdline_user_args();
 for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-source-3d-skip=")){const auto value=args[i].trim_prefix("--track-source-3d-skip=");if(value!="true"&&value!="false")throw std::invalid_argument("Source3D suppression requires true or false");impl_->skip_source_3d=value=="true";}
for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-vehicle-depth-staging=")){const auto value=args[i].trim_prefix("--track-vehicle-depth-staging=");if(value!="true"&&value!="false")throw std::invalid_argument("Vehicle depth staging requires true or false");impl_->vehicle_staging=value=="true";}for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-depth-discovery-cache=")){const auto value=args[i].trim_prefix("--track-depth-discovery-cache=");if(value!="true"&&value!="false")throw std::invalid_argument("Depth discovery cache requires true or false");impl_->discovery_cache=value=="true";}for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-depth-param-cache=")){const auto value=args[i].trim_prefix("--track-depth-param-cache=");if(value!="true"&&value!="false")throw std::invalid_argument("Depth parameter cache requires true or false");impl_->parameter_notifications=value=="true";}for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-post-consumer-cache=")){
  const auto value=args[i].substr(godot::String("--track-post-consumer-cache=").length());
  if(value!="true"&&value!="false")throw std::invalid_argument("Post consumer cache requires true or false");impl_->consumer_cache=value=="true";
 }
}
TrackPostprocess::~TrackPostprocess(){release();}
void TrackPostprocess::mark_tracked_mesh(godot::MeshInstance3D* mesh){if(!mesh)return;const auto id=mesh->get_instance_id();if(impl_->tracked_meshes.insert(id).second){impl_->dirty_meshes.insert(id);impl_->discovery_dirty=true;}}
void TrackPostprocess::forget_mesh(godot::MeshInstance3D* mesh){
 if(!mesh)return;auto& p=*impl_;const auto id=mesh->get_instance_id();
 const auto found=p.proxies.find(id);if(found!=p.proxies.end()){memdelete(found->second);p.proxies.erase(found);}
 const auto bindings=p.mesh_materials.find(id);if(bindings!=p.mesh_materials.end()){for(auto material:bindings->second){auto sources=p.material_meshes.find(material);if(sources!=p.material_meshes.end()){sources->second.erase(id);if(sources->second.empty())p.material_meshes.erase(sources);}}p.mesh_materials.erase(bindings);}p.tracked_meshes.erase(id);p.dirty_meshes.erase(id);p.dirty.forget(id);p.nonwriting_nodes.erase(id);p.visible_sources.erase(id);p.sorting_cache.erase(id);
 p.source_meshes.erase(std::remove(p.source_meshes.begin(),p.source_meshes.end(),id),p.source_meshes.end());p.discovery_dirty=true;
}
void TrackPostprocess::forget_material(const godot::Ref<godot::ShaderMaterial>& material){
 if(material.is_null())return;auto& p=*impl_;const auto id=material->get_instance_id();
 p.variants.erase(id);const auto sources=p.material_meshes.find(id);if(sources!=p.material_meshes.end()){for(auto mesh:sources->second){auto bindings=p.mesh_materials.find(mesh);if(bindings!=p.mesh_materials.end()){bindings->second.erase(id);if(bindings->second.empty())p.mesh_materials.erase(bindings);}}p.material_meshes.erase(sources);}p.parameter_values.erase(id);p.tracked_materials.erase(id);p.dirty_materials.erase(id);
}
void TrackPostprocess::mark_mesh_changed(godot::MeshInstance3D* mesh,unsigned changes){if(mesh){const auto id=mesh->get_instance_id();impl_->dirty_meshes.insert(id);impl_->dirty.mark(id,changes);if(changes&(Geometry|Material))impl_->nonwriting_nodes.erase(id);}}
bool TrackPostprocess::needs_structure_observer()const{return impl_->discovery_cache;}
void TrackPostprocess::notify_source_order(){impl_->discovery_dirty=true;}
void TrackPostprocess::set_structure_observer(bool enabled,const godot::Callable& observer){impl_->order_observer=observer;if(impl_->structure_observer!=enabled){impl_->structure_observer=enabled;impl_->discovery_dirty=true;}}
void TrackPostprocess::notify_scene_node(godot::Node* node){
 auto& p=*impl_;if(!p.discovery_cache||!node||!p.source_id)return;
 const auto id=node->get_instance_id();
 // Own proxy/history trees cannot introduce source geometry. On removal an old
 // source may no longer have a viewport, so consult the previous ordered list first.
 if(std::find(p.source_meshes.begin(),p.source_meshes.end(),id)!=p.source_meshes.end()){p.dirty.observe_structure(true,false,true,false);return;}
 for(auto* parent=node;parent;parent=parent->get_parent())for(auto owned:p.owned_ids)if(owned&&parent->get_instance_id()==owned)return;
 if(!godot::Object::cast_to<godot::MeshInstance3D>(node)&&!godot::Object::cast_to<godot::MultiMeshInstance3D>(node))return;
 auto* source=godot::Object::cast_to<godot::Camera3D>(godot::ObjectDB::get_instance(p.source_id));
 p.dirty.observe_structure(false,false,true,source&&node->get_viewport()==source->get_viewport());
}
void TrackPostprocess::notify_mesh_sorting(godot::MeshInstance3D* mesh,float offset,bool use_aabb){
 if(!mesh)return;const auto id=mesh->get_instance_id();const auto value=std::make_pair(offset,use_aabb);
 auto old=impl_->sorting_cache.find(id);if(old!=impl_->sorting_cache.end()&&old->second==value)return;
 impl_->sorting_cache[id]=value;auto proxy=impl_->proxies.find(id);if(proxy!=impl_->proxies.end()){proxy->second->set("sorting_offset",offset);proxy->second->set("sorting_use_aabb_center",use_aabb);}
}
void TrackPostprocess::mark_tracked_material(const godot::Ref<godot::ShaderMaterial>& material){if(material.is_valid())impl_->tracked_materials.insert(material->get_instance_id());}
void TrackPostprocess::mark_material_changed(const godot::Ref<godot::ShaderMaterial>& material){mark_material_dirty(material);}
void TrackPostprocess::notify_material_parameter(const godot::Ref<godot::ShaderMaterial>& material,const godot::StringName& name,const godot::Variant& value){
 if(material.is_null())return;auto found=impl_->variants.find(material->get_instance_id());
 if(found!=impl_->variants.end()){
  auto& v=found->second;const auto slot=v.uniform_slots.find(name);
  if(impl_->parameter_notifications&&slot!=v.uniform_slots.end()){if(!v.first_sync&&v.values[slot->second]==value){++impl_->direct_parameter_skips;return;}v.values[slot->second]=value;}
  v.depth->set_shader_parameter(name,value);++impl_->direct_parameter_sets;
 }
}
void TrackPostprocess::mark_material_dirty(const godot::Ref<godot::ShaderMaterial>& material){if(material.is_valid()){const auto id=material->get_instance_id();if(impl_->vehicle_staging)vehicle_depth_staging::invalidate(id);impl_->dirty_materials.insert(id);const auto meshes=impl_->material_meshes.find(id);if(meshes!=impl_->material_meshes.end())for(auto mesh:meshes->second){impl_->dirty_meshes.insert(mesh);impl_->dirty.mark(mesh,Material);impl_->nonwriting_nodes.erase(mesh);}}}
void TrackPostprocess::set_screen_resources(const godot::Ref<godot::Texture2D>& filter,const godot::Ref<godot::Texture2D>& radial){impl_->filter_texture=filter;impl_->radial_texture=radial;}
godot::Ref<godot::ViewportTexture> TrackPostprocess::get_colour_texture()const{return available()?impl_->colour->get_texture():godot::Ref<godot::ViewportTexture>();}
godot::Ref<godot::ViewportTexture> TrackPostprocess::get_depth_texture()const{return available()?impl_->depth->get_texture():godot::Ref<godot::ViewportTexture>();}
bool TrackPostprocess::available()const{return impl_&&godot::ObjectDB::get_instance(impl_->owner_id)&&godot::ObjectDB::get_instance(impl_->source_id)&&godot::ObjectDB::get_instance(impl_->owned_ids[1])&&godot::ObjectDB::get_instance(impl_->owned_ids[2])&&godot::ObjectDB::get_instance(impl_->output_id);}
bool TrackPostprocess::active()const{if(!available())return false;auto* owner=godot::Object::cast_to<godot::Node3D>(godot::ObjectDB::get_instance(impl_->owner_id));return owner&&owner->is_visible_in_tree()&&impl_->output->is_visible();}
bool TrackPostprocess::fog_active()const{return active()&&impl_->fog_active;}
bool TrackPostprocess::focus_active()const{return active()&&impl_->focus_active;}
void TrackPostprocess::release(){
 auto& p=*impl_;
 for(auto id:p.order_parents)if(auto* node=godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(id)))if(node->is_connected("child_order_changed",p.order_observer))node->disconnect("child_order_changed",p.order_observer);
 const auto order_observer=p.order_observer;
 auto& owners=camera_owners();const auto owned=owners.find(p.source_id);
 if(owned!=owners.end()&&owned->second==impl_.get()){
  if(auto* object=godot::ObjectDB::get_instance(p.source_id))
   if(auto* camera=godot::Object::cast_to<godot::Camera3D>(object))camera->set_cull_mask(p.source_mask);
  if(p.source_3d_acquired)if(auto* viewport=godot::Object::cast_to<godot::Viewport>(godot::ObjectDB::get_instance(p.source_viewport_id)))viewport->set_disable_3d(p.source_3d_was_disabled);
  owners.erase(owned);
 }
 // Parent deletion may already have freed some children before this C++ owner.
 // Resolve each ID independently; a live parent is not proof of live children.
 for(auto id:p.owned_ids)if(auto* object=godot::ObjectDB::get_instance(id)){
  if(auto* node=godot::Object::cast_to<godot::Node>(object)){
   if(node->get_parent())node->get_parent()->remove_child(node);
   if(!node->is_queued_for_deletion())node->queue_free();
  }
 }
 const bool reuse=p.reuse,occlusion=p.occlusion,consumer_cache=p.consumer_cache,parameter_notifications=p.parameter_notifications,skip_source_3d=p.skip_source_3d,structure_observer=p.structure_observer,discovery_cache=p.discovery_cache,vehicle_staging=p.vehicle_staging;p=Impl{};p.reuse=reuse;p.occlusion=occlusion;p.consumer_cache=consumer_cache;p.parameter_notifications=parameter_notifications;p.skip_source_3d=skip_source_3d;p.structure_observer=structure_observer;p.discovery_cache=discovery_cache;p.vehicle_staging=vehicle_staging;p.order_observer=order_observer;
}
void TrackPostprocess::update(godot::Node3D* owner,godot::Camera3D* source,
 const port::track::postprocess::Weather& weather,float n,float f,
 const port::track::postprocess::Controls& controls,bool allowed,float focal,float dof,const port::track::postprocess::Frame* prepared,const port::track::postprocess::ScreenControls* screen){
 if(!allowed||!owner||!source||!owner->is_inside_tree()||!owner->is_visible_in_tree()){release();return;}
 if(!(n>0&&f>n))throw std::invalid_argument("Track postprocess original near/far invalid");
 auto& p=*impl_;
 if(p.owner_id&&!available())release();
 if(p.owner_id&&(p.owner_id!=owner->get_instance_id()||p.source_id!=source->get_instance_id()))release();
 struct FailureCleanup {TrackPostprocess& self;int count=std::uncaught_exceptions();~FailureCleanup(){if(std::uncaught_exceptions()>count)self.release();}} cleanup{*this};
 if(!p.colour){
  auto& owners=camera_owners();const auto occupied=owners.find(source->get_instance_id());
  if(occupied!=owners.end()&&occupied->second!=impl_.get())throw std::runtime_error("Track postprocess camera already owned by another visible renderer");
  if(source->get_cull_mask()==0)throw std::runtime_error("Track postprocess cannot acquire a camera with an empty source mask");
  owners[source->get_instance_id()]=impl_.get();
  p.owner_id=owner->get_instance_id();p.source_id=source->get_instance_id();p.source_mask=source->get_cull_mask();

  p.colour=memnew(godot::SubViewport);p.depth=memnew(godot::SubViewport);p.owned_ids[1]=p.colour->get_instance_id();p.owned_ids[2]=p.depth->get_instance_id();
  for(auto* viewport:{p.colour,p.depth}){
   viewport->set_use_hdr_2d(godot::RenderingServer::get_singleton()->get_current_rendering_method()!="gl_compatibility");
   viewport->set_world_3d(source->get_world_3d());viewport->set_canvas_cull_mask(0);
   viewport->set_update_mode(godot::SubViewport::UPDATE_ALWAYS);owner->add_child(viewport);
  }
  p.colour_camera=memnew(godot::Camera3D);p.depth_camera=memnew(godot::Camera3D);
  p.colour->add_child(p.colour_camera);p.depth->add_child(p.depth_camera);
  p.depth_environment.instantiate();p.depth_environment->set_background(godot::Environment::BG_COLOR);p.depth_environment->set_bg_color(godot::Color(0,0,0,1));
  p.canvas=memnew(godot::CanvasLayer);p.owned_ids[0]=p.canvas->get_instance_id();p.canvas->set_layer(1);owner->add_child(p.canvas);
  p.output=memnew(godot::TextureRect);p.output_id=p.output->get_instance_id();p.output->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
  p.output->set_texture(p.colour->get_texture());p.output->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
  p.composite_material=material(composite_code);p.canvas->add_child(p.output);
  port::track::postprocess::Palette initial;initial.fill(0x80000000u);p.initial_display=palette(initial);
  p.output->set_material(material("shader_type canvas_item;render_mode unshaded,blend_disabled;void fragment(){COLOR=vec4(texture(TEXTURE,UV).rgb,1.0);}"));
  for(unsigned i=0;i<2;i++){
   p.history[i]=memnew(godot::SubViewport);p.owned_ids[3+i]=p.history[i]->get_instance_id();p.history[i]->set_disable_3d(true);p.history[i]->set_transparent_background(true);owner->add_child(p.history[i]);
   p.history_rect[i]=memnew(godot::TextureRect);p.history_rect[i]->set_texture(p.colour->get_texture());p.history_rect[i]->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);p.history_rect[i]->set_material(p.composite_material);p.history[i]->add_child(p.history_rect[i]);
  }
  p.parameter(p.composite_material,"colour_texture",p.colour->get_texture(),true);
  p.parameter(p.composite_material,"encoded_depth",p.depth->get_texture(),true);
 }
 const godot::Vector2 size=source->get_viewport()->get_visible_rect().size;
 if(!p.reuse||p.colour->get_size()!=godot::Vector2i(size))p.colour->set_size(godot::Vector2i(size));if(!p.reuse||p.depth->get_size()!=godot::Vector2i(size))p.depth->set_size(godot::Vector2i(size));if(!p.reuse||p.output->get_size()!=size)p.output->set_size(size);
 for(unsigned i=0;i<2;i++){if(p.history[i]->get_size()!=godot::Vector2i(size)){p.history_ready=false;p.history[i]->set_size(godot::Vector2i(size));}p.history_rect[i]->set_size(size);p.history[i]->set_update_mode(godot::SubViewport::UPDATE_DISABLED);}
 p.history_index^=1u;p.history[p.history_index]->set_update_mode(godot::SubViewport::UPDATE_ALWAYS);
 p.output->set_texture(p.history[p.history_index]->get_texture());
 if(p.history_ready)p.parameter(p.composite_material,"previous_texture",p.history[p.history_index^1u]->get_texture(),true);else p.parameter(p.composite_material,"previous_texture",p.initial_display,true);
 p.colour->set_use_occlusion_culling(p.occlusion);p.depth->set_use_occlusion_culling(p.occlusion);
 copy_camera(p.colour_camera,source,p.source_mask&~depth_layer,p.reuse);copy_camera(p.depth_camera,source,depth_layer,p.reuse);p.depth_camera->set_environment(p.depth_environment);
 source->set_cull_mask(0);
 // E0071H01: the source camera now has no3D layers. Suppress only that
 // empty viewport pass; owned colour/depth/reflection viewports still render.
 if(p.skip_source_3d&&!p.source_3d_acquired){auto* viewport=source->get_viewport();p.source_viewport_id=viewport->get_instance_id();p.source_3d_was_disabled=viewport->is_3d_disabled();viewport->set_disable_3d(true);p.source_3d_acquired=true;}
 port::track::postprocess::View view;view.near_z=n;view.far_z=f;view.weather=weather;
 view.camera_focal_distance=focal;view.camera_depth_of_field=dof;
 const auto frame=prepared?*prepared:port::track::postprocess::prepare(view,controls,512,448);
 if(!p.cache_valid||p.cached_fog!=frame.fog.linear){p.fog=palette(frame.fog.linear);p.cached_fog=frame.fog.linear;}
 if(!p.cache_valid||p.cached_focus!=frame.focus.linear){p.focus=palette(frame.focus.linear);p.cached_focus=frame.focus.linear;}
 p.cache_valid=true;p.fog_active=frame.fog.enabled;p.focus_active=frame.focus.enabled;
 bool bw=false;for(const auto& pass:frame.passes)bw|=pass.stage==port::track::postprocess::Stage::BlackAndWhite;
 p.depth_alpha_active=controls.EnableDepthIntoAlpha;p.bw_active=bw;
 if(bw&&!controls.EnableDepthIntoAlpha&&p.history_ready&&!p.history_alpha_known[p.history_index^1u])hp2::not_implemented("0x0011A9F0","MainDisp alpha unavailable after a depth-alpha-disabled frame without an alpha-writing post pass");
 if(bw&&p.black_white.is_null())p.black_white=palette(hp2::recovered::SetupBlackAndWhite().linear);
 auto& shader=p.composite_material;p.parameter(shader,"black_white_enabled",bw);if(bw)p.parameter(shader,"black_white_palette",p.black_white,p.consumer_cache);
 p.parameter(shader,"history_available",p.history_ready);p.parameter(shader,"fog_palette",p.fog,p.consumer_cache);p.parameter(shader,"focus_palette",p.focus,p.consumer_cache);
 p.parameter(shader,"fog_enabled",frame.fog.enabled);p.parameter(shader,"focus_enabled",frame.focus.enabled);
 for(const auto& pass:frame.passes)if(pass.stage==port::track::postprocess::Stage::Fog){
  const godot::Vector4i abcd(pass.blend.a,pass.blend.b,pass.blend.c,pass.blend.d);
  p.parameter(shader,"fog_abcd",abcd);p.parameter(shader,"fog_fix",pass.blend.fix);
 }
 p.parameter(shader,"depth_into_alpha_enabled",controls.EnableDepthIntoAlpha);
 const auto screen_controls=screen?*screen:port::track::postprocess::ScreenControls{};
 const auto screen_frame=port::track::postprocess::prepare_screen(view,screen_controls);
 p.filter_active=bool(screen_frame.filter);p.radial_active=!screen_frame.radial_mask.empty();p.previous_active=bool(screen_frame.previous);
 if(screen_frame.filter&&p.filter_texture.is_null())throw std::runtime_error("Original active screen filter texture02C8711C unavailable");
 if(!screen_frame.radial_mask.empty()&&p.radial_texture.is_null())throw std::runtime_error("Original active radial maskAB9A6E21 unavailable");
 p.parameter(shader,"filter_enabled",bool(screen_frame.filter));
 p.parameter(shader,"radial_enabled",!screen_frame.radial_mask.empty());
 p.parameter(shader,"previous_enabled",bool(screen_frame.previous));
 if(screen_frame.filter){const auto& q=*screen_frame.filter;p.parameter(shader,"filter_texture",p.filter_texture,p.consumer_cache);p.parameter(shader,"filter_abcd",godot::Vector4i(q.blend.a,q.blend.b,q.blend.c,q.blend.d));p.parameter(shader,"filter_fix",q.blend.fix);p.parameter(shader,"filter_tfx",q.texture_fx);const auto& c=q.colours[0];p.parameter(shader,"filter_colour",godot::Vector4(c[0],c[1],c[2],c[3]));}
 if(!screen_frame.radial_mask.empty()){p.parameter(shader,"radial_texture",p.radial_texture,p.consumer_cache);p.parameter(shader,"radial_alpha",godot::Vector2(screen_controls.nRadialAlphaCenter,screen_controls.nRadialAlphaEdge));}
 bool known_alpha=controls.EnableDepthIntoAlpha||frame.fog.enabled||frame.focus.enabled||bw||bool(screen_frame.filter)||!screen_frame.radial_mask.empty();
 if(screen_frame.previous)known_alpha=controls.EnableDepthIntoAlpha||bw||!p.history_ready||p.history_alpha_known[p.history_index^1u];
 p.history_alpha_known[p.history_index]=known_alpha;p.history_ready=true;
 p.parameter(shader,"jitter_blend",controls.JitterBlend);p.parameter(shader,"jitter_count",controls.dofNumPasses);
 // A depth proxy retains the original vertex transformation, discard and Z-write
 // rules. Reading hint_depth_texture here would omit transparent road geometry.
 std::set<std::uint64_t> seen,synced_materials;
 const auto source_world=source->get_world_3d();
 if(p.discovery_cache){const auto transform=owner->get_global_transform();if(!p.owner_transform_valid||p.owner_transform!=transform){for(auto id:p.tracked_meshes){p.dirty_meshes.insert(id);p.dirty.mark(id,Transform|Visibility);}p.owner_transform=transform;p.owner_transform_valid=true;}}
 const int node_count=source->get_tree()->get_node_count();
 for(auto id:p.source_meshes)if(!godot::ObjectDB::get_instance(id)){p.discovery_dirty=true;break;}
 if(p.discovery_dirty||(p.discovery_cache&&p.structure_observer?p.dirty.structure_changed():p.scene_node_count!=node_count)){
  ++p.discovery_scans;p.source_meshes.clear();
  std::set<std::uint64_t> next_parents;
  std::function<void(godot::Node*)> discover=[&](godot::Node* node){
   ++p.discovery_visits;
   if(node!=source->get_viewport()&&godot::Object::cast_to<godot::Viewport>(node))return;
   if(auto* mesh=godot::Object::cast_to<godot::MeshInstance3D>(node))
    if((mesh->get_layer_mask()&p.source_mask)&&mesh->get_world_3d()==source_world){p.source_meshes.push_back(mesh->get_instance_id());if(p.discovery_cache&&p.structure_observer)for(auto* parent=mesh->get_parent();parent;parent=parent->get_parent()){next_parents.insert(parent->get_instance_id());if(parent==source->get_viewport())break;}}
   for(int i=0;i<node->get_child_count();i++)discover(node->get_child(i));
  };
  discover(source->get_viewport());
  for(auto id:p.order_parents)if(!next_parents.count(id))if(auto* node=godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(id)))if(node->is_connected("child_order_changed",p.order_observer))node->disconnect("child_order_changed",p.order_observer);
  for(auto id:next_parents)if(!p.order_parents.count(id))if(auto* node=godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(id)))if(!node->is_connected("child_order_changed",p.order_observer))node->connect("child_order_changed",p.order_observer);
  p.order_parents=std::move(next_parents);p.discovery_dirty=false;p.scene_node_count=node_count;p.dirty.discovered();
 }
 for(auto source_id:p.source_meshes){
  auto* mesh=godot::Object::cast_to<godot::MeshInstance3D>(godot::ObjectDB::get_instance(source_id));if(!mesh)continue;
  if(p.tracked_meshes.count(source_id)&&!p.dirty_meshes.count(source_id)&&(p.proxies.count(source_id)||p.nonwriting_nodes.count(source_id))){
   if(p.visible_sources.count(source_id))seen.insert(source_id);continue;
  }
  const unsigned changes=p.discovery_cache?p.dirty.changes(source_id,p.tracked_meshes.count(source_id)!=0):All;
  p.visible_sources.erase(source_id);
  {
   if((mesh->get_layer_mask()&p.source_mask)&&mesh->is_visible_in_tree()&&mesh->get_mesh().is_valid()&&mesh->get_world_3d()==source_world){
    const auto id=mesh->get_instance_id();
    bool writing=p.proxies.count(id)&&!(changes&(Geometry|Material));
    if(!writing&&!p.nonwriting_nodes.count(id))for(int surface=0;surface<mesh->get_mesh()->get_surface_count();surface++){
     godot::Ref<godot::ShaderMaterial> m=mesh->get_active_material(surface);if(m.is_null()||m->get_shader().is_null())throw std::runtime_error("Track depth proxy requires known shader material");
     const auto shader_id=m->get_shader()->get_instance_id();auto found=p.shader_writes.find(shader_id);
     if(found==p.shader_writes.end())found=p.shader_writes.emplace(shader_id,std::string(m->get_shader()->get_code().utf8().get_data()).find("depth_draw_never")==std::string::npos).first;
     writing|=found->second;
    }
    if(!writing){p.nonwriting_nodes.insert(id);}else{
    seen.insert(id);p.visible_sources.insert(id);auto& proxy=p.proxies[id];
    const bool created=!proxy;
    if(!proxy){++p.proxy_creations;proxy=memnew(godot::MeshInstance3D);proxy->set_layer_mask(depth_layer);p.depth->add_child(proxy);}
    if(created||(changes&Geometry))if(proxy->get_mesh()!=mesh->get_mesh())proxy->set_mesh(mesh->get_mesh());if(created||(changes&Transform))if(proxy->get_global_transform()!=mesh->get_global_transform())proxy->set_global_transform(mesh->get_global_transform());
    if(created||(changes&Bounds)){if(proxy->get_extra_cull_margin()!=mesh->get_extra_cull_margin())proxy->set_extra_cull_margin(mesh->get_extra_cull_margin());
    if(proxy->get_custom_aabb()!=mesh->get_custom_aabb())proxy->set_custom_aabb(mesh->get_custom_aabb());
    if(proxy->get_visibility_range_begin()!=mesh->get_visibility_range_begin())proxy->set_visibility_range_begin(mesh->get_visibility_range_begin());
    if(proxy->get_visibility_range_end()!=mesh->get_visibility_range_end())proxy->set_visibility_range_end(mesh->get_visibility_range_end());
    proxy->set_visibility_range_begin_margin(0);proxy->set_visibility_range_end_margin(0);proxy->set_visibility_range_fade_mode(godot::GeometryInstance3D::VISIBILITY_RANGE_FADE_DISABLED);
    }
    if((created||(changes&Bounds))&&mesh->has_meta("hp2_hlod_instance")){
     proxy->set_meta("hp2_hlod_instance",mesh->get_meta("hp2_hlod_instance"));proxy->set_meta("hp2_hlod_slot",mesh->get_meta("hp2_hlod_slot"));proxy->set_meta("hp2_hlod_view",mesh->get_meta("hp2_hlod_view"));proxy->set_meta("hp2_hlod_depth_proxy",true);
    }
    if(created||(changes&Sorting)){if(proxy->get("sorting_offset")!=mesh->get("sorting_offset"))proxy->set("sorting_offset",mesh->get("sorting_offset"));if(proxy->get("sorting_use_aabb_center")!=mesh->get("sorting_use_aabb_center"))proxy->set("sorting_use_aabb_center",mesh->get("sorting_use_aabb_center"));}if(!proxy->is_visible())proxy->show();
    if(created||(changes&(Geometry|Material)))for(int surface=0;surface<mesh->get_mesh()->get_surface_count();surface++){
     godot::Ref<godot::ShaderMaterial> original=mesh->get_active_material(surface);
     if(original.is_null()||original->get_shader().is_null())throw std::runtime_error("Track depth proxy requires known shader material");
     if(p.discovery_cache){p.material_meshes[original->get_instance_id()].insert(id);p.mesh_materials[id].insert(original->get_instance_id());}
     const auto material=p.depth_material(original,n,f,synced_materials);
     if(proxy->get_surface_override_material(surface)!=material)proxy->set_surface_override_material(surface,material);
    }
    }
   }
  }
 }
 // Instance batches share the exact MultiMesh resource; only material output changes.
 for(auto it=p.instance_sources.begin();it!=p.instance_sources.end();){
  const auto id=*it;auto* node=godot::Object::cast_to<godot::MultiMeshInstance3D>(godot::ObjectDB::get_instance(id));
  if(!node){auto proxy=p.instance_proxies.find(id);if(proxy!=p.instance_proxies.end()){memdelete(proxy->second);p.instance_proxies.erase(proxy);}it=p.instance_sources.erase(it);continue;}++it;
  auto found=p.instance_proxies.find(id);
  if(!node->is_visible_in_tree()||!(node->get_layer_mask()&p.source_mask)||node->get_world_3d()!=source_world){if(found!=p.instance_proxies.end())found->second->hide();continue;}
  godot::Ref<godot::ShaderMaterial> material=node->get_material_override();if(material.is_null()||material->get_shader().is_null())throw std::runtime_error("Instance depth proxy requires its known source shader");
  if(std::string(material->get_shader()->get_code().utf8().get_data()).find("depth_draw_never")!=std::string::npos){if(found!=p.instance_proxies.end())found->second->hide();continue;}
  auto& proxy=p.instance_proxies[id];if(!proxy){++p.proxy_creations;proxy=memnew(godot::MultiMeshInstance3D);proxy->set_layer_mask(depth_layer);proxy->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);p.depth->add_child(proxy);}
  if(proxy->get_multimesh()!=node->get_multimesh())proxy->set_multimesh(node->get_multimesh());
  if(proxy->get_global_transform()!=node->get_global_transform())proxy->set_global_transform(node->get_global_transform());
  if(proxy->get_custom_aabb()!=node->get_custom_aabb())proxy->set_custom_aabb(node->get_custom_aabb());
  const auto depth_material=p.depth_material(material,n,f,synced_materials);if(proxy->get_material_override()!=depth_material)proxy->set_material_override(depth_material);
  if(proxy->get_sorting_offset()!=node->get_sorting_offset())proxy->set_sorting_offset(node->get_sorting_offset());proxy->set_sorting_use_aabb_center(false);if(!proxy->is_visible())proxy->show();
 }
 // Material dirtiness is independent of geometry dirtiness: a camera/light
 // update must reach a sealed mesh even when its transform remains unchanged.
 for(auto& item:p.variants){auto& v=item.second;
  if(p.dirty_materials.count(item.first)&&(!p.parameter_notifications||!synced_materials.count(item.first)))for(const auto& name:v.names){const int u=v.uniform_slots.at(name);const auto value=v.source->get_shader_parameter(name);++p.uniform_reads;if(v.values[u]!=value){v.depth->set_shader_parameter(name,value);v.values[u]=value;}}
  if(v.near_z!=n){v.depth->set_shader_parameter("pp_near",n);v.near_z=n;}if(v.far_z!=f){v.depth->set_shader_parameter("pp_far",f);v.far_z=f;}
 }
 p.dirty_meshes.clear();p.dirty.consumed();p.dirty_materials.clear();
 for(auto& entry:p.proxies)if(!seen.count(entry.first))entry.second->hide();
 // Nodes created above belong to this compositor, so they cannot introduce
 // new source geometry. Keep the discovery watermark after our own mutations;
 // external node changes and explicit notifications still invalidate next frame.
 if(p.discovery_cache)p.scene_node_count=source->get_tree()->get_node_count();
 p.output->show();
}
}
