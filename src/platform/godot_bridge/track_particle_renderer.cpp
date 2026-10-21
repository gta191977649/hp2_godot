#include "track_particle_renderer.hpp"
#include "gpu_particle_renderer.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include "ps2_driving_runtime.hpp"
#include "game_core/port/driving/car_effects.hpp"
#include "camera_visibility_bounds.hpp"
#include "particle_mesh_stream.hpp"
#include "particle_alpha_bounds.hpp"
#include <set>
#include <tuple>
#include "platform/shaders/track_shader.hpp"
#include "platform/shaders/screen_projection.hpp"
#include "game_core/indep/inc/AcidFX.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <map>
#include <cstring>
#include <cmath>
#include <chrono>
namespace port::godot_bridge {
struct TrackParticleRenderer::Impl {
 godot::Node3D& owner;TextureLookup texture;MipLookup mip;
 std::shared_ptr<const port::track::Scene> scene;std::unique_ptr<port::track::ParticleSimulation> simulation;
 Ps2DrivingRuntime* driving_source=nullptr; // Borrowed; owner retains the runtime.
 port::driving::WheelParticleEffects wheel_effects;std::uint64_t driving_step=0;
 std::uint32_t seed=0x23985727;std::array<std::array<float,3>,3> interests{};
 std::vector<godot::MeshInstance3D*> nodes;std::map<std::uint32_t,godot::Ref<godot::ShaderMaterial>> materials;
 std::map<std::uint32_t,unsigned> alpha_max;
 std::map<std::tuple<std::uint32_t,float,float,float,float>,unsigned> tile_alpha_max;
 bool gpu_backend=false;std::unique_ptr<GpuParticleRenderer> gpu;
 std::unique_ptr<port::track::ParticleSimulation> rollback;port::driving::WheelParticleEffects rollback_effects;std::uint64_t rollback_step=0;
 bool mesh_reuse=false;std::vector<ParticleMeshStream> streams;
 // Host-only experimental switch; immutable texture inputs live for this scene.
 bool transport_cache=true,state_cache=true,staging_cache=true; // Exact state/buffer parity validated in TASK0063.
 std::map<std::uint32_t,std::map<godot::StringName,godot::Variant>> parameter_cache;
 double simulation_ms=0,packet_ms=0,packing_ms=0,upload_ms=0;
 std::uint64_t parameter_sets=0,parameter_skips=0;
 std::function<void(godot::MeshInstance3D*)> mesh_changed;
 std::function<void(const godot::Ref<godot::ShaderMaterial>&)> track_material;
 std::function<void(const godot::Ref<godot::ShaderMaterial>&,const godot::StringName&,const godot::Variant&)> material_parameter;
 struct Staging {godot::PackedVector3Array points;godot::PackedVector2Array uvs;godot::PackedColorArray colours;godot::PackedInt32Array indices;};
 std::vector<Staging> staging;std::uint64_t staging_size_changes=0;

 std::uint64_t baseline_creations=0;std::size_t active_quads=0;
 Impl(godot::Node3D& o,TextureLookup t,MipLookup m):owner(o),texture(std::move(t)),mip(std::move(m)){}
};
TrackParticleRenderer::TrackParticleRenderer(godot::Node3D& owner,TextureLookup texture,MipLookup mip,std::uint32_t seed):impl_(std::make_unique<Impl>(owner,std::move(texture),std::move(mip))){impl_->seed=seed;
 const auto args=godot::OS::get_singleton()->get_cmdline_user_args();
 godot::String backend=godot::ProjectSettings::get_singleton()->get_setting("hp2/rendering/particle_backend","native");
 for(int i=0;i<args.size();++i)if(args[i].begins_with("--particle-backend="))backend=args[i].trim_prefix("--particle-backend=");
 if(backend!="native"&&backend!="godot_gpu")throw std::invalid_argument("Particle backend requires native or godot_gpu");
 impl_->gpu_backend=backend=="godot_gpu";
 if(impl_->gpu_backend)impl_->gpu=std::make_unique<GpuParticleRenderer>(owner,impl_->texture,impl_->mip);
 for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-particle-staging-cache=")){const auto value=args[i].trim_prefix("--track-particle-staging-cache=");if(value!="true"&&value!="false")throw std::invalid_argument("Particle staging cache requires true or false");impl_->staging_cache=value=="true";}
for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-particle-transport-cache=")){
  const auto value=args[i].substr(godot::String("--track-particle-transport-cache=").length());
  if(value!="true"&&value!="false")throw std::invalid_argument("Particle transport cache requires true or false");impl_->transport_cache=value=="true";
 }
 for(int i=0;i<args.size();++i)if(args[i].begins_with("--track-particle-state-cache=")){
  const auto value=args[i].substr(godot::String("--track-particle-state-cache=").length());
  if(value!="true"&&value!="false")throw std::invalid_argument("Particle state cache requires true or false");impl_->state_cache=value=="true";
 }
}
void TrackParticleRenderer::commit_frame(){if(!impl_->gpu)return;const auto begin=std::chrono::steady_clock::now();impl_->gpu->commit_frame();impl_->upload_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();impl_->simulation->take_gpu_commands();impl_->rollback.reset();}
void TrackParticleRenderer::abort_frame(){if(!impl_->gpu)return;impl_->gpu->abort_frame();if(impl_->rollback){impl_->simulation=std::move(impl_->rollback);impl_->wheel_effects=impl_->rollback_effects;impl_->driving_step=impl_->rollback_step;}}
std::uint32_t TrackParticleRenderer::global_seed()const{return impl_->simulation?impl_->simulation->global_seed():impl_->seed;}
void TrackParticleRenderer::set_material_callbacks(std::function<void(const godot::Ref<godot::ShaderMaterial>&)> tracked,std::function<void(const godot::Ref<godot::ShaderMaterial>&,const godot::StringName&,const godot::Variant&)> parameter){impl_->track_material=std::move(tracked);impl_->material_parameter=std::move(parameter);if(impl_->gpu)impl_->gpu->set_material_callbacks(impl_->track_material,impl_->material_parameter);}
void TrackParticleRenderer::set_mesh_reuse(bool enabled){if(impl_->scene&&enabled!=impl_->mesh_reuse)throw std::logic_error("Particle mesh reuse must be selected before track load");impl_->mesh_reuse=enabled;}
void TrackParticleRenderer::set_mesh_change_callback(std::function<void(godot::MeshInstance3D*)> callback){impl_->mesh_changed=std::move(callback);}
godot::Dictionary TrackParticleRenderer::stats()const{
 if(impl_->gpu){auto d=impl_->gpu->stats();d["backend"]="godot_gpu";d["simulation_ms"]=impl_->simulation_ms;d["packet_ms"]=impl_->packet_ms;d["packing_ms"]=0.0;d["upload_ms"]=impl_->upload_ms;
 if(impl_->simulation){std::size_t count=0,groups=0;const auto& auto_order=impl_->simulation->auto_emitter_order();for(const auto& g:impl_->simulation->groups())if(std::find(auto_order.begin(),auto_order.end(),g.emitter)==auto_order.end()){++groups;count+=g.particles.size();}d["native_active_particles"]=int64_t(impl_->simulation->particle_count());d["native_dynamic_particles"]=int64_t(count);d["native_dynamic_groups"]=int64_t(groups);}return d;}
 godot::Dictionary d;d["backend"]="native";std::uint64_t creations=impl_->baseline_creations,surfaces=impl_->baseline_creations,updates=0,bytes=0;std::size_t capacity=0,degenerate=0;
 for(const auto& s:impl_->streams){creations+=s.creations;surfaces+=s.surfaces;updates+=s.updates;bytes+=s.bytes;capacity+=s.capacity;if(s.active)degenerate+=s.capacity-s.active;}
 d["staging_cache"]=impl_->staging_cache;d["staging_size_changes"]=int64_t(impl_->staging_size_changes);d["transport_cache"]=impl_->transport_cache;d["state_cache"]=impl_->state_cache;d["simulation_ms"]=impl_->simulation_ms;d["packet_ms"]=impl_->packet_ms;d["packing_ms"]=impl_->packing_ms;d["upload_ms"]=impl_->upload_ms;d["parameter_sets"]=int64_t(impl_->parameter_sets);d["parameter_skips"]=int64_t(impl_->parameter_skips);
 std::uint64_t queries=0;for(const auto& s:impl_->streams)queries+=s.layout_queries;d["layout_queries"]=int64_t(queries);
 d["mesh_reuse"]=impl_->mesh_reuse;d["mesh_creations"]=int64_t(creations);d["surface_creations"]=int64_t(surfaces);d["mesh_updates"]=int64_t(updates);d["buffer_updates"]=int64_t(updates*2);d["uploaded_bytes"]=int64_t(bytes);d["capacity_quads"]=int64_t(capacity);d["active_quads"]=int64_t(impl_->active_quads);d["degenerate_quads"]=int64_t(degenerate);return d;
}
TrackParticleRenderer::~TrackParticleRenderer(){clear();}
void TrackParticleRenderer::set_driving_source(Ps2DrivingRuntime* source){
 if(source==impl_->driving_source)return;
 if(impl_->simulation)impl_->simulation->clear_dynamic_emitters();
 impl_->wheel_effects={};impl_->driving_step=0;impl_->driving_source=source;
}
void TrackParticleRenderer::clear(){impl_->rollback.reset();if(impl_->gpu)impl_->gpu->clear();if(impl_->simulation)impl_->seed=impl_->simulation->global_seed();impl_->simulation.reset();impl_->wheel_effects={};impl_->driving_step=0;impl_->scene.reset();for(auto* n:impl_->nodes){if(n->get_parent())n->get_parent()->remove_child(n);memdelete(n);}impl_->nodes.clear();impl_->materials.clear();impl_->parameter_cache.clear();impl_->parameter_sets=impl_->parameter_skips=0;impl_->simulation_ms=impl_->packet_ms=impl_->packing_ms=impl_->upload_ms=0;impl_->alpha_max.clear();impl_->tile_alpha_max.clear();impl_->streams.clear();impl_->staging.clear();impl_->staging_size_changes=0;impl_->baseline_creations=0;impl_->active_quads=0;}
ParticleFrame TrackParticleRenderer::update(std::shared_ptr<const port::track::Scene> scene,port::driving::Memory& math,const port::track::FrameView& frame,float dt,godot::Ref<godot::Texture2D> depth,bool enabled,unsigned layer,godot::Camera3D* source_camera,bool require_native_depth){
 ParticleFrame result;if(!scene){clear();return result;}if(scene!=impl_->scene){clear();impl_->scene=scene;impl_->simulation=std::make_unique<port::track::ParticleSimulation>(scene->particles,impl_->seed);impl_->simulation->set_state_cache(impl_->state_cache);impl_->simulation->set_gpu_emission(impl_->gpu_backend);}
 // Original UpdateInterestPoints reads views1,2,0 and leaves absent cameras unchanged.
 using Clock=std::chrono::steady_clock;const auto simulation_begin=Clock::now();impl_->simulation_ms=impl_->packet_ms=impl_->packing_ms=impl_->upload_ms=0;
 if(impl_->gpu){if(impl_->rollback)throw std::logic_error("GPU particle frame was not committed or aborted");impl_->rollback=std::make_unique<port::track::ParticleSimulation>(*impl_->simulation);impl_->rollback_effects=impl_->wheel_effects;impl_->rollback_step=impl_->driving_step;}
 impl_->interests[0]=frame.position;
 if(impl_->driving_source!=nullptr){
  const auto source_step=impl_->driving_source->particle_step();
  // Original World0014DDE0 updates existing groups before0014DE44 emission.
  // Candidate simulation/bindings and native visual counters commit together.
  auto next=*impl_->simulation;auto effects=impl_->wheel_effects;
  if(source_step<impl_->driving_step){next.clear_dynamic_emitters();effects={};}
  next.set_car_velocity(impl_->driving_source->particle_velocity());
  const float step=impl_->driving_source->particle_timestep(impl_->driving_step);
  if(!impl_->gpu||step>0)next.step(math,step,impl_->interests);
  if(step>0&&source_step!=impl_->driving_step)impl_->driving_source->emit_wheel_particles(next,effects,impl_->interests);
  *impl_->simulation=std::move(next);impl_->wheel_effects=std::move(effects);impl_->driving_step=source_step;
 }else if(!impl_->gpu||dt>0)impl_->simulation->step(math,dt,impl_->interests);
 result.active=impl_->simulation->particle_count();impl_->simulation_ms=std::chrono::duration<double,std::milli>(Clock::now()-simulation_begin).count();
 if(impl_->gpu){const auto begin=Clock::now();auto output=impl_->gpu->update(scene,impl_->simulation->gpu_commands(),frame,depth,enabled,layer,source_camera,require_native_depth);impl_->packet_ms=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();output.active=result.active;output.visible=enabled?result.active:0;return output;}
 impl_->active_quads=0;for(auto& stream:impl_->streams){stream.active=0;result.capacity_quads+=stream.capacity;}
 for(auto* n:impl_->nodes)n->hide();if(!enabled)return result;if(depth.is_null()&&require_native_depth){result.gaps.push_back("World AcidFX requires original GS depth source");return result;}
 port::track::ParticleView view;view.focal_h=frame.focal_h;
 const auto clip=[&](const std::array<float,3>& p){std::array<float,4> out{};for(unsigned r=0;r<4;++r)out[r]=((frame.clip_matrix[r]*p[0]+frame.clip_matrix[4+r]*p[1])+frame.clip_matrix[8+r]*p[2])+frame.clip_matrix[12+r];return out;};
 const auto camera_z=[&](const auto& p){float z=0;for(unsigned k=0;k<3;++k)z+=(p[k]-frame.position[k])*frame.direction[k];return z;};
 view.outside_clip=[&](const auto& p){const auto c=clip(p);const float z=camera_z(p);return c[0]>c[3]||c[0]<-c[3]||c[1]>c[3]||c[1]<-c[3]||z<frame.near_z||z>frame.far_z;};
 view.project=[&](const auto& p){const auto c=clip(p);const float z=camera_z(p),native_z=port::track::screen_depth(z,frame.near_z,frame.far_z,0,true);std::uint32_t bits;std::memcpy(&bits,&native_z,4);return port::track::ParticleProjection{z,2048.f+c[0]/c[3]*256.f,2048.f-c[1]/c[3]*224.f,bits};};
 view.texture_available=[&](auto hash){return std::any_of(scene->textures.begin(),scene->textures.end(),[&](const auto& t){return t.header.tex_hash==hash;});};
 if(!source_camera)throw std::invalid_argument("World particle host projection requires its actual Camera3D visibility bound");
 const auto visibility_bounds=camera_visibility_corners(source_camera);
 const auto packet_begin=Clock::now();auto draws=hp2::recovered::AcidEffectSystem::RenderEffects(math,*impl_->simulation,view);impl_->packet_ms=std::chrono::duration<double,std::milli>(Clock::now()-packet_begin).count();
 // Packet vector preserves original active-group insertion order for shared alpha ranking.
 std::set<std::uint32_t> reported_alpha_gaps;
 std::vector<std::size_t> order(draws.size());std::iota(order.begin(),order.end(),0);
 while(impl_->nodes.size()<draws.size()){auto* n=memnew(godot::MeshInstance3D);n->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);impl_->owner.add_child(n);impl_->nodes.push_back(n);}
 for(std::size_t rank=0;rank<order.size();++rank){const auto& draw=draws[order[rank]];const auto found=std::find_if(scene->textures.begin(),scene->textures.end(),[&](const auto& t){return t.header.tex_hash==draw.texture;});const auto& header=found->header;
  auto effect=port::track::effect_state(header,0);
  if(!effect.alpha_pass)hp2::not_implemented("0x00128DA0","World particle solid texture-chain submission is not yet connected");if(effect.matrix_pack)hp2::not_implemented("0x00128DA0","World particle nonzero matrix pack requires separate CPU packet audit");
  auto cached=impl_->materials.find(draw.texture);
  if(cached==impl_->materials.end()){
   unsigned maximum=0;for(std::size_t k=3;k<found->bitmap.rgba.size();k+=4)maximum=std::max(maximum,unsigned(found->bitmap.rgba[k]));if(auto m=scene->mipmaps.find(draw.texture);m!=scene->mipmaps.end())for(const auto& image:m->second)for(std::size_t k=3;k<image.rgba.size();k+=4)maximum=std::max(maximum,unsigned(image.rgba[k]));impl_->alpha_max[draw.texture]=maximum;
   auto code=track_shader_source(header,effect,port::track::Fusion40::None);
   
   const auto vertex=code.find("void vertex(){"),fragment=code.find("void fragment",vertex),brace=code.rfind('}',fragment);
   code.insert(brace,"\nparticle_z=VERTEX.z;particle_host_depth=vec3(-PROJECTION_MATRIX[2][2],PROJECTION_MATRIX[3][2],CLIP_SPACE_FAR);POSITION=hp2_host_screen_position((VERTEX.xy/16.0-vec2(1792.0,1824.0))/vec2(512.0,448.0),PROJECTION_MATRIX[1][1]);gs_color_w=1.0;gs_color_numerator=gs_rgba;\n");
   code.insert(vertex,std::string(screen_position_shader)+std::string((effect.test.write_depth||depth.is_null())?"":"uniform sampler2D particle_raw_depth : filter_nearest, repeat_disable;\n")+"varying flat float particle_z;\nvarying flat vec3 particle_host_depth;\nuniform vec2 particle_original_depth;\n");
   const auto fb=code.find('{',code.find("void fragment"));code.insert(fb+1,std::string((effect.test.write_depth||depth.is_null())?"\n":"\nvec3 raw_z=floor(textureLod(particle_raw_depth,SCREEN_UV,0.0).rgb*255.0+0.5);if(particle_z<dot(raw_z,vec3(1.0,256.0,65536.0)))discard;")+"float native_screen=uintBitsToFloat((uint(particle_z)<<4u)|0x40000000u);float inverse_z=(native_screen-particle_original_depth.x)/particle_original_depth.y;float ndc_z=particle_host_depth.x+particle_host_depth.y*inverse_z;DEPTH=particle_host_depth.z<0.0?ndc_z*0.5+0.5:ndc_z;\n");
   // Original particle packet uses ST/Q with Q1 for every corner.
   const std::string derivative="float lod=0.5*log2(max(rho,0.000001));";if(const auto where=code.find(derivative);where!=std::string::npos)code.replace(where,derivative.size(),"float lod=0.0;");
   godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code(godot::String::utf8(code.c_str()));godot::Ref<godot::ShaderMaterial> material;material.instantiate();material->set_shader(shader);material->set_shader_parameter("base_lit",false);material->set_render_priority(64);cached=impl_->materials.emplace(draw.texture,material).first;
  }
  const auto material=cached->second;if(impl_->track_material)impl_->track_material(material);
  const auto parameter=[&](const godot::StringName& name,const godot::Variant& value){
   if(impl_->transport_cache){auto& values=impl_->parameter_cache[draw.texture];const auto old=values.find(name);if(old!=values.end()&&old->second==value){++impl_->parameter_skips;return;}values[name]=value;}
   material->set_shader_parameter(name,value);if(impl_->material_parameter)impl_->material_parameter(material,name,value);++impl_->parameter_sets;
  };
  parameter("base_texture",impl_->texture(draw.texture));if(!effect.test.write_depth&&depth.is_valid())parameter("particle_raw_depth",depth);const auto coefficients=port::track::screen_depth_coefficients(frame.near_z,frame.far_z,0,true);parameter("particle_original_depth",godot::Vector2(coefficients[0],coefficients[1]));
  for(unsigned mip=1;mip<=header.Field_0x6e;++mip){if(!impl_->mip)throw std::runtime_error("World particle mip callback missing");parameter(godot::String::utf8(("mip_"+std::to_string(mip)).c_str()),impl_->mip(draw.texture,mip));}
  const auto packing_begin=Clock::now();
  Impl::Staging temporary;if(impl_->staging_cache&&impl_->staging.size()<=rank)impl_->staging.resize(rank+1);
  auto& stage=impl_->staging_cache?impl_->staging[rank]:temporary;auto& points=stage.points;auto& uvs=stage.uvs;auto& colours=stage.colours;auto& indices=stage.indices;
  const auto wanted=std::int64_t(draw.quads.size()*4);if(points.size()!=wanted)++impl_->staging_size_changes;
  points.resize(wanted);uvs.resize(wanted);colours.resize(wanted);const bool needs_indices=!impl_->transport_cache||!impl_->mesh_reuse;indices.resize(needs_indices?draw.quads.size()*6:0);std::size_t packed=0;

  for(const auto& quad:draw.quads){
   try{
    if(!effect.texture.additive&&effect.texture.blend==port::vehicle::BlendMode::SourceOver&&unsigned(quad.colour[3])*impl_->alpha_max.at(draw.texture)>=129u*128u){
     const auto key=std::make_tuple(draw.texture,quad.uv[0][0],quad.uv[0][1],quad.uv[3][0],quad.uv[3][1]);
     auto bound=impl_->tile_alpha_max.find(key);
     if(bound==impl_->tile_alpha_max.end())bound=impl_->tile_alpha_max.emplace(key,particle_uv_alpha_max(found->bitmap.rgba,header.width,header.height,quad.uv,effect.texture.repeat_u,effect.texture.repeat_v)).first;
     const auto maximum=bound->second;
     if(unsigned(quad.colour[3])*maximum>=129u*128u)
      hp2::not_implemented("0x00128DA0",("Original particle source-over alpha exceeds host normalized blend bound: "+header.name+" vertex_alpha="+std::to_string(quad.colour[3])+" tile_alpha_max="+std::to_string(maximum)).c_str());
    }
   }catch(const hp2::NotImplemented& gap){
    // Host failure boundary: report the untranslated packet, never invent its
    // blend or tear down unrelated scenery. Supported quads continue in order.
    ++result.unsupported_quads;
    if(reported_alpha_gaps.insert(draw.texture).second)result.gaps.push_back(gap.what());
    continue;
   }
   const int first=packed;for(unsigned j=0;j<4;++j){points.set(first+j,{float(quad.xy[j][0]),float(quad.xy[j][1]),float(quad.native_depth)});uvs.set(first+j,{quad.uv[j][0],quad.uv[j][1]});colours.set(first+j,godot::Color(quad.colour[0]/255.f,quad.colour[1]/255.f,quad.colour[2]/255.f,quad.colour[3]/255.f));}if(needs_indices){unsigned k=0;for(int index:{0,1,2,1,3,2})indices.set(packed/4*6+k++,first+index);}packed+=4;}
  points.resize(packed);uvs.resize(packed);colours.resize(packed);if(needs_indices)indices.resize(packed/4*6);
  impl_->packing_ms+=std::chrono::duration<double,std::milli>(Clock::now()-packing_begin).count();
  if(points.is_empty())continue;
  result.alpha_keys.push_back({header.RenderingOrder,draw.depth});
  const auto node_rank=result.groups;
  godot::Ref<godot::ArrayMesh> mesh;const auto old_surface_count=impl_->mesh_reuse&&impl_->streams.size()>node_rank?impl_->streams[node_rank].surfaces:0;
  if(impl_->mesh_reuse){if(impl_->streams.size()<=node_rank)impl_->streams.resize(node_rank+1);auto& stream=impl_->streams[node_rank];stream.transport_cache=impl_->transport_cache;const auto old_creations=stream.creations,old_capacity=stream.capacity;mesh=stream.update(points,uvs,colours);impl_->packing_ms+=stream.packing_ms;impl_->upload_ms+=stream.upload_ms;result.mesh_creations+=stream.creations-old_creations;result.surface_creations+=stream.surfaces-old_surface_count;result.capacity_quads+=stream.capacity-old_capacity;++result.mesh_updates;}
  else{const auto upload_begin=Clock::now();godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);arrays[godot::Mesh::ARRAY_VERTEX]=points;arrays[godot::Mesh::ARRAY_TEX_UV]=uvs;arrays[godot::Mesh::ARRAY_COLOR]=colours;arrays[godot::Mesh::ARRAY_INDEX]=indices;mesh.instantiate();mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays);++impl_->baseline_creations;++result.mesh_creations;++result.surface_creations;impl_->upload_ms+=std::chrono::duration<double,std::milli>(Clock::now()-upload_begin).count();}
  auto* node=impl_->nodes[node_rank];const bool geometry_changed=node->get_mesh()!=mesh||(impl_->mesh_reuse&&old_surface_count!=impl_->streams[node_rank].surfaces),material_changed=node->get_material_override()!=material;
  if(!impl_->mesh_reuse||node->get_mesh()!=mesh)node->set_mesh(mesh);apply_camera_visibility_bounds(node,visibility_bounds);node->set_material_override(material);node->set_layer_mask(layer);node->set_sorting_use_aabb_center(false);if(impl_->mesh_changed&&(geometry_changed||material_changed))impl_->mesh_changed(node);result.visible+=points.size()/4;impl_->active_quads+=points.size()/4;++result.groups;

 }
 return result;
}
void TrackParticleRenderer::set_alpha_ranks(const std::vector<std::size_t>& ranks,std::size_t total,float origin_depth){
 if(impl_->gpu){impl_->gpu->set_alpha_ranks(ranks,total,origin_depth);return;}
 if(ranks.size()>impl_->nodes.size())throw std::invalid_argument("Particle alpha rank count exceeds prepared nodes");
 for(std::size_t i=0;i<ranks.size();++i){if(ranks[i]>=total)throw std::invalid_argument("Particle alpha rank exceeds common submission");impl_->nodes[i]->set_sorting_offset(origin_depth-float(total-ranks[i])*4096.f);impl_->nodes[i]->show();}
}
godot::Dictionary TrackParticleRenderer::test_mesh_reuse(){
 godot::Dictionary out;
 try{
  const auto require=[](bool b,const char* message){if(!b)throw std::runtime_error(message);};
  const auto arrays=[](std::size_t quads,int phase){
   godot::PackedVector3Array p;godot::PackedVector2Array uv;godot::PackedColorArray c;godot::PackedInt32Array indices;
   for(std::size_t q=0;q<quads;++q){const int first=p.size();for(unsigned j=0;j<4;++j){const float x=1792.f*16+float(80+q*45+(j&1)*30+phase)*16,y=1824.f*16+float(100+(j>>1)*30)*16;p.push_back({x,y,float(10000+phase)});uv.push_back({float(j&1)+phase*.125f,float(j>>1)});const unsigned n=(q*4+j)%256;c.push_back({n/255.f,(255-n)/255.f,((n+phase)%256)/255.f,(quads==64?n:128)/255.f});}for(int j:{0,1,2,1,3,2})indices.push_back(first+j);}
   godot::Array a;a.resize(godot::Mesh::ARRAY_MAX);a[godot::Mesh::ARRAY_VERTEX]=p;a[godot::Mesh::ARRAY_TEX_UV]=uv;a[godot::Mesh::ARRAY_COLOR]=c;a[godot::Mesh::ARRAY_INDEX]=indices;return a;
  };
  const auto update=[](ParticleMeshStream& s,const godot::Array& a){return s.update(a[godot::Mesh::ARRAY_VERTEX],a[godot::Mesh::ARRAY_TEX_UV],a[godot::Mesh::ARRAY_COLOR]);};
  ParticleMeshStream all_colours;update(all_colours,arrays(64,0));const auto golden=arrays(64,1);update(all_colours,golden);
  godot::Ref<godot::ArrayMesh> reference;reference.instantiate();reference->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,golden);
  auto* rs=godot::RenderingServer::get_singleton();const auto actual=rs->mesh_get_surface(all_colours.mesh->get_rid(),0),expected=rs->mesh_get_surface(reference->get_rid(),0);
  for(const char* key:{"vertex_data","attribute_data","index_data"}){const godot::PackedByteArray a=actual[key],b=expected[key];require(a==b,"Particle raw updates differ from engine-packed original attributes/indices");}
  ParticleMeshStream cached_layout;cached_layout.transport_cache=true;update(cached_layout,arrays(64,0));update(cached_layout,golden);
  require(cached_layout.layout_queries==1,"Cached particle layout queried again without surface change");
  const auto cached_actual=rs->mesh_get_surface(cached_layout.mesh->get_rid(),0);
  for(const char* key:{"vertex_data","attribute_data","index_data"}){const godot::PackedByteArray a=cached_actual[key],b=expected[key];require(a==b,"Cached particle stream differs from reference raw buffers");}
  update(cached_layout,arrays(65,0));require(cached_layout.layout_queries==2,"Particle layout cache not invalidated by capacity growth");
  ParticleMeshStream grow;auto original=update(grow,arrays(1,0));update(grow,arrays(5,1));require(grow.mesh==original&&grow.capacity==8&&grow.creations==1&&grow.surfaces==2,"Particle growth replaced resource or failed capacity growth");
  ParticleMeshStream shrink;update(shrink,arrays(3,0));auto persistent=update(shrink,arrays(1,1));require(shrink.capacity==4&&shrink.creations==1&&shrink.surfaces==1&&shrink.updates==2,"Particle shrink recreated surface instead of updating");
  const auto unpacked=persistent->surface_get_arrays(0);const godot::PackedVector3Array positions=unpacked[godot::Mesh::ARRAY_VERTEX];for(int i=4;i<positions.size();++i)require(positions[i]==positions[0],"Inactive particle capacity is not degenerate");
  godot::Ref<godot::ArrayMesh> baseline;baseline.instantiate();baseline->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays(1,1));
  out["ok"]=true;out["baseline"]=baseline;out["reused"]=persistent;out["capacity_quads"]=int64_t(shrink.capacity);out["active_quads"]=int64_t(shrink.active);out["format"]=int64_t(std::uint64_t(persistent->surface_get_format(0)));out["screen_position_shader"]=godot::String::utf8(screen_position_shader.data());
 }catch(const std::exception& e){out["ok"]=false;out["error"]=e.what();}
 return out;
}
}
