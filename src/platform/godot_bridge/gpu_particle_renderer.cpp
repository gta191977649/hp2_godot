#include "gpu_particle_renderer.hpp"
#include "camera_visibility_bounds.hpp"
#include "particle_alpha_bounds.hpp"
#include "platform/shaders/track_shader.hpp"
#include "platform/shaders/screen_projection.hpp"
#include "game_core/indep/inc/AcidFX.hpp"
#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <map>
#include <sstream>
#include <set>
#include <algorithm>
#include <cmath>
namespace port::godot_bridge {
namespace {
// E-0085: host shader translation of AcidEffectSystem::DoTimeStep 00128870.
const char* process_source=R"glsl(shader_type particles;
render_mode disable_force, disable_velocity;
uniform int epoch=0;
uniform float timestep=0.0;
uniform vec3 inherited;
uniform float gravity;
uniform float drag;
uniform bool inherit_enabled;
uniform int interval;
uniform int frame_mask;
void start(){}
void process(){
 if(int(CUSTOM.w)!=epoch){
  int previous=int(CUSTOM.x);int life=previous-int(timestep*1024.0);
  if(life<1){ACTIVE=false;}else{
   int packed=int(CUSTOM.z);int frame=packed&255;
   if(interval!=0){frame=(frame+previous/interval-life/interval)&frame_mask;}
   vec3 sign_v=mix(vec3(-1.0),vec3(1.0),lessThan(VELOCITY,vec3(0.0)));
   vec3 velocity=VELOCITY+(vec3(0.0,0.0,-gravity)+sign_v*VELOCITY*VELOCITY*drag)*timestep;
   TRANSFORM[3].xyz+=velocity*timestep;
   if(inherit_enabled){TRANSFORM[3].xyz+=(timestep*inherited)*(float(packed>>8)*(1.0/255.0));}
   float magnitude=length(velocity);
   int speed=int(magnitude*128.0)&65535;
   ivec3 direction=ivec3(0);
   if(magnitude!=0.0){direction=ivec3(velocity/magnitude*127.0);}
   VELOCITY=vec3(direction)*(float(speed)*0.0078125*(1.0/127.0));
   CUSTOM.x=float(life);CUSTOM.z=float((packed&65280)|frame);
  }
  CUSTOM.w=float(epoch);
 }
}
)glsl";
// Pinned 4.7.2 Vulkan probe: request_particles_process adds a second dispatch
// that loses single emissions on this runtime. speed_scale=0 still dispatches
// the custom shader at DELTA=0; epoch is the sole simulation clock instead.
godot::Ref<godot::ShaderMaterial> shader_material(const std::string& code){godot::Ref<godot::Shader> s;s.instantiate();s->set_code(godot::String::utf8(code.c_str()));godot::Ref<godot::ShaderMaterial> m;m.instantiate();m->set_shader(s);return m;}
godot::Projection matrix(const std::array<float,16>& m){return godot::Projection({m[0],m[1],m[2],m[3]},{m[4],m[5],m[6],m[7]},{m[8],m[9],m[10],m[11]},{m[12],m[13],m[14],m[15]});}
float alpha_peak(const std::array<float,16>& m){
 const float a=m[3],b=m[7],c=m[11],d=m[15];
 const auto curve=[&](float t){return ((a*t+b)*t+c)*t+d;};float peak=std::max(curve(0),curve(1));
 const auto sample=[&](float t){if(t>0&&t<1)peak=std::max(peak,curve(t));};
 if(a==0){if(b!=0)sample(-c/(2*b));}else{const float q=4*b*b-12*a*c;if(q>=0){sample((-2*b+std::sqrt(q))/(6*a));sample((-2*b-std::sqrt(q))/(6*a));}}
 return std::clamp(peak,0.f,255.f);
}
std::string draw_source(const hp2::assets::TpkTextureHeader& h,const port::track::EffectState& effect,const std::vector<unsigned>& alpha,bool raw){
 auto code=track_shader_source(h,effect,port::track::Fusion40::None);
 std::ostringstream declarations;declarations<<screen_position_shader<<R"glsl(
uniform mat4 particle_clip;
uniform mat4 particle_colour;
uniform mat4 particle_extra;
uniform vec3 particle_camera;
uniform vec3 particle_direction;
uniform float particle_focal;
uniform float particle_lifetime;
uniform float particle_far;
uniform float particle_max_size;
uniform vec2 particle_planes;
uniform vec2 particle_original_depth;
uniform ivec2 particle_atlas;
varying flat float particle_z;
varying flat vec3 particle_host_depth;
varying flat float particle_valid;
)glsl";
 if(raw)declarations<<"uniform sampler2D particle_raw_depth :filter_nearest,repeat_disable;\n";
 declarations<<"float tile_alpha(int frame){\n";for(unsigned i=0;i<alpha.size();++i)declarations<<"if(frame=="<<i<<")return "<<alpha[i]<<".0;\n";declarations<<"return 255.0;}\n";
 const auto begin=code.find("void vertex(){"),end=code.find("void fragment",begin);
 const std::string vertex=R"glsl(void vertex(){
 vec3 p=MODEL_MATRIX[3].xyz;
 float z=dot(p-particle_camera,particle_direction);
 vec4 clip=particle_clip*vec4(p,1.0);
 float t=1.0-(INSTANCE_CUSTOM.x*0.0009765625)*(1.0/particle_lifetime);
 vec4 powers=vec4(t*t*t,t*t,t,1.0);
 vec4 extra=particle_extra*powers;
 float radius=min(extra.x>0.0?extra.x*particle_focal/z:0.0,particle_max_size==0.0?512.0:particle_max_size);
 particle_valid=(z>=particle_planes.x&&z<=particle_planes.y&&z<particle_far&&radius>1.0&&abs(clip.x)<=clip.w&&abs(clip.y)<=clip.w)?1.0:0.0;
 int relative=int(extra.y)&65535;
 int angle=int(INSTANCE_CUSTOM.y);if((angle&1)!=0){relative=(-relative)&65535;}angle=(angle+relative)&65535;
 vec2 center=vec2(ivec2(vec2(2048.0+clip.x/clip.w*256.0,2048.0-clip.y/clip.w*224.0)))*16.0;
 vec2 corner=UV*2.0-1.0;
 vec2 xy;
 if(angle==0){xy=center+corner*vec2(ivec2(radius*vec2(8.0,7.0)));}
 else{float a=float(angle)*uintBitsToFloat(0x38c90fdbu);float s=sin(a);float c=sin(a+uintBitsToFloat(0x3fc90fdcu));vec2 q=corner*float(int(radius));xy=vec2(ivec2(center+vec2(c*q.x-s*q.y,s*q.x+c*q.y)*vec2(8.0,7.0)));}
 vec4 rgba=vec4(ivec4(clamp(particle_colour*powers,vec4(0.0),vec4(255.0))));
 int frame=int(INSTANCE_CUSTOM.z)&255;
 int nx=1<<particle_atlas.x;int ny=1<<particle_atlas.y;
 UV=(vec2(float(frame&(nx-1)),float((frame>>particle_atlas.x)&(ny-1)))+UV)/vec2(float(nx),float(ny));
)glsl"+std::string(!effect.texture.additive&&effect.texture.blend==port::vehicle::BlendMode::SourceOver?"if(rgba.a*tile_alpha(frame)>=16512.0){particle_valid=0.0;}\n":"")+R"glsl(
 gs_color_w=1.0;gs_color_numerator=rgba;
 float native_screen=particle_original_depth.x+particle_original_depth.y/z;
 particle_z=float((floatBitsToUint(native_screen)&0x0fffffffu)>>4u);
 particle_host_depth=vec3(-PROJECTION_MATRIX[2][2],PROJECTION_MATRIX[3][2],CLIP_SPACE_FAR);
 POSITION=hp2_host_screen_position((xy/16.0-vec2(1792.0,1824.0))/vec2(512.0,448.0),PROJECTION_MATRIX[1][1]);
}
)glsl";
 code.replace(begin,end-begin,declarations.str()+vertex);
 const auto f=code.find('{',code.find("void fragment"));code.insert(f+1,std::string("\nif(particle_valid<0.5)discard;\n")+(raw?"vec3 raw_z=floor(textureLod(particle_raw_depth,SCREEN_UV,0.0).rgb*255.0+0.5);if(particle_z<dot(raw_z,vec3(1.0,256.0,65536.0)))discard;\n":"")+"float native_screen=uintBitsToFloat((uint(particle_z)<<4u)|0x40000000u);float inverse_z=(native_screen-particle_original_depth.x)/particle_original_depth.y;float ndc_z=particle_host_depth.x+particle_host_depth.y*inverse_z;DEPTH=particle_host_depth.z<0.0?ndc_z*0.5+0.5:ndc_z;\n");
 const std::string derivative="float lod=0.5*log2(max(rho,0.000001));";if(auto where=code.find(derivative);where!=std::string::npos)code.replace(where,derivative.size(),"float lod=0.0;");return code;
}
}
struct GpuParticleRenderer::Impl{
 godot::Node3D& owner;TextureLookup texture;MipLookup mip;std::shared_ptr<const port::track::Scene> scene;
 struct Group{godot::GPUParticles3D* node=nullptr;godot::Ref<godot::ShaderMaterial> process,draw;std::size_t definition=0;unsigned epoch=0;std::array<float,3> centroid{};std::int8_t order=0;bool advanced=false,alpha_gap=false;};
 std::map<std::uint64_t,Group> backup_groups;std::vector<std::uint64_t> backup_order;std::uint64_t backup_emissions=0,backup_steps=0;bool prepared=false;std::map<std::size_t,godot::Ref<godot::ShaderMaterial>> draw_materials;std::map<std::uint64_t,Group> groups;std::vector<std::uint64_t> order,visible;
 std::function<void(const godot::Ref<godot::ShaderMaterial>&)> tracked;
 std::function<void(const godot::Ref<godot::ShaderMaterial>&,const godot::StringName&,const godot::Variant&)> parameter;
 std::uint64_t emissions=0,steps=0,creations=0;bool enabled=false;std::vector<std::function<void()>> pending;
 Impl(godot::Node3D& o,TextureLookup t,MipLookup m):owner(o),texture(std::move(t)),mip(std::move(m)){}
 void remove(std::uint64_t id){auto it=groups.find(id);if(it==groups.end())return;auto* n=it->second.node;if(n->get_parent())n->get_parent()->remove_child(n);memdelete(n);groups.erase(it);order.erase(std::remove(order.begin(),order.end(),id),order.end());}
};
GpuParticleRenderer::GpuParticleRenderer(godot::Node3D& o,TextureLookup t,MipLookup m):impl_(std::make_unique<Impl>(o,std::move(t),std::move(m))){}
GpuParticleRenderer::~GpuParticleRenderer(){clear();}
void GpuParticleRenderer::clear(){while(!impl_->groups.empty())impl_->remove(impl_->groups.begin()->first);impl_->visible.clear();impl_->pending.clear();impl_->backup_groups.clear();impl_->backup_order.clear();impl_->prepared=false;impl_->draw_materials.clear();impl_->scene.reset();}
void GpuParticleRenderer::set_material_callbacks(std::function<void(const godot::Ref<godot::ShaderMaterial>&)> a,std::function<void(const godot::Ref<godot::ShaderMaterial>&,const godot::StringName&,const godot::Variant&)> b){impl_->tracked=std::move(a);impl_->parameter=std::move(b);}
ParticleFrame GpuParticleRenderer::update(std::shared_ptr<const port::track::Scene> scene,const std::vector<port::track::ParticleGpuCommand>& commands,const port::track::FrameView& frame,godot::Ref<godot::Texture2D> depth,bool enabled,unsigned layer,godot::Camera3D* camera,bool require_depth){
 ParticleFrame out;abort_frame();impl_->pending.clear();if(!scene){clear();return out;}
 // Validate dependencies and command shape before submitting any GPU mutation.
 if(enabled&&(!require_depth||depth.is_valid())){if(!camera)throw std::invalid_argument("GPU particles require source camera visibility bounds");camera_visibility_corners(camera);port::track::screen_depth_coefficients(frame.near_z,frame.far_z,0,true);}
 std::map<std::uint64_t,unsigned> advances;std::map<std::uint64_t,bool> known;
 if(scene==impl_->scene)for(const auto& pair:impl_->groups)known[pair.first]=true;
 for(const auto& c:commands){
  if(c.kind==port::track::ParticleGpuCommandKind::RemoveGroup){known.erase(c.group);continue;}
  if(c.kind==port::track::ParticleGpuCommandKind::Advance){if(!known.count(c.group))throw std::logic_error("GPU Advance precedes Spawn");if(++advances[c.group]>1)throw std::logic_error("Multiple GPU advances in one batch");continue;}
  known[c.group]=true;const auto& d=scene->particles.definitions.at(c.definition);
  const auto texture=std::find_if(scene->textures.begin(),scene->textures.end(),[&](const auto& t){return t.header.tex_hash==d.texture_hash;});if(texture==scene->textures.end())throw std::runtime_error("GPU particle original texture missing");
  const auto e=port::track::effect_state(texture->header,0);if(!e.alpha_pass||e.matrix_pack)hp2::not_implemented("0x00128DA0","GPU particles require supported alpha chain and zero matrix pack");
  if((d.flags[1]&31)+(d.flags[2]&31)>8)hp2::not_implemented("0x00128DA0","GPU particle atlas exceeds packed frame range");
  if(impl_->texture(d.texture_hash).is_null())throw std::runtime_error("GPU particle texture upload missing");
  for(unsigned m=1;m<=texture->header.Field_0x6e;++m)if(!impl_->mip||impl_->mip(d.texture_hash,m).is_null())throw std::runtime_error("GPU particle mip upload missing");
 }
 if(scene!=impl_->scene){clear();impl_->scene=scene;}impl_->backup_groups=impl_->groups;impl_->backup_order=impl_->order;impl_->backup_emissions=impl_->emissions;impl_->backup_steps=impl_->steps;impl_->prepared=true;impl_->enabled=enabled;impl_->visible.clear();for(auto& pair:impl_->groups)pair.second.advanced=false;
 std::set<std::uint64_t> removed,changed;
 for(const auto& command:commands){
  if(command.kind==port::track::ParticleGpuCommandKind::RemoveGroup){const auto id=command.group;removed.insert(id);impl_->pending.push_back([this,id](){impl_->remove(id);});continue;}
  if(!impl_->groups.count(command.group)){
   if(command.kind!=port::track::ParticleGpuCommandKind::Spawn)throw std::logic_error("GPU Advance precedes Spawn");
   const auto& d=scene->particles.definitions.at(command.definition);auto found=std::find_if(scene->textures.begin(),scene->textures.end(),[&](const auto& t){return t.header.tex_hash==d.texture_hash;});if(found==scene->textures.end())throw std::runtime_error("GPU particle original texture missing");
   const auto effect=port::track::effect_state(found->header,0);if(!effect.alpha_pass||effect.matrix_pack)hp2::not_implemented("0x00128DA0","GPU particles require supported alpha chain and zero matrix pack");
   Impl::Group group;group.definition=command.definition;group.order=found->header.RenderingOrder;group.process=shader_material(process_source);
   const unsigned sx=d.flags[1]&31,sy=d.flags[2]&31;if(sx+sy>8)hp2::not_implemented("0x00128DA0","GPU particle atlas exceeds packed frame range");
   std::vector<unsigned> bounds;for(unsigned f=0;f<(1u<<(sx+sy));++f){const float u=float(f&((1u<<sx)-1))/float(1u<<sx),v=float(f>>sx)/float(1u<<sy),du=1.f/float(1u<<sx),dv=1.f/float(1u<<sy);bounds.push_back(particle_uv_alpha_max(found->bitmap.rgba,found->header.width,found->header.height,{{{u,v},{u+du,v},{u,v+dv},{u+du,v+dv}}},effect.texture.repeat_u,effect.texture.repeat_v));}
   group.alpha_gap=!effect.texture.additive&&effect.texture.blend==port::vehicle::BlendMode::SourceOver&&alpha_peak(d.colour_basis)*float(*std::max_element(bounds.begin(),bounds.end()))>=16512.f;
   auto cached_draw=impl_->draw_materials.find(command.definition);if(cached_draw!=impl_->draw_materials.end())group.draw=cached_draw->second;else{group.draw=shader_material(draw_source(found->header,effect,bounds,!effect.test.write_depth&&depth.is_valid()));group.draw->set_shader_parameter("base_lit",false);group.draw->set_render_priority(64);group.draw->set_shader_parameter("base_texture",impl_->texture(d.texture_hash));for(unsigned m=1;m<=found->header.Field_0x6e;++m){if(!impl_->mip)throw std::runtime_error("GPU particle mip callback missing");group.draw->set_shader_parameter(godot::String::utf8(("mip_"+std::to_string(m)).c_str()),impl_->mip(d.texture_hash,m));}
   group.draw->set_shader_parameter("particle_colour",matrix(d.colour_basis));group.draw->set_shader_parameter("particle_extra",matrix(d.extra_basis));group.draw->set_shader_parameter("particle_lifetime",d.controls[6]);group.draw->set_shader_parameter("particle_far",d.far_clip);group.draw->set_shader_parameter("particle_max_size",d.max_pixel_size);group.draw->set_shader_parameter("particle_atlas",godot::Vector2i(sx,sy));
   impl_->draw_materials.emplace(command.definition,group.draw);}
   auto process=group.process;process->set_shader_parameter("gravity",d.gravity);process->set_shader_parameter("drag",d.drag);process->set_shader_parameter("inherit_enabled",bool(d.flags[6]));const auto fps=std::int8_t(d.flags[3]);process->set_shader_parameter("interval",fps?int((1.f/float(fps))*1024.f):0);process->set_shader_parameter("frame_mask",int((1u<<(sx+sy))-1));
   auto* node=memnew(godot::GPUParticles3D);group.node=node;node->set_emitting(false);node->set_amount(1000);node->set_lifetime(60);node->set_speed_scale(0);node->set_fixed_fps(0);node->set_fractional_delta(false);node->set_interpolate(false);node->set_use_local_coordinates(true);node->set_process_material(process);node->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);godot::Ref<godot::QuadMesh> quad;quad.instantiate();quad->set_size({2,2});node->set_draw_pass_mesh(0,quad);node->set_material_override(group.draw);impl_->owner.add_child(node);node->set_as_top_level(true);node->set_global_transform(godot::Transform3D());node->hide();if(impl_->tracked)impl_->tracked(group.draw);impl_->groups.emplace(command.group,std::move(group));impl_->order.push_back(command.group);++impl_->creations;
  }
  auto& g=impl_->groups.at(command.group);
  changed.insert(command.group);
  if(command.kind==port::track::ParticleGpuCommandKind::Advance){if(g.advanced)throw std::logic_error("GPU command batch contains multiple advances for one group");g.advanced=true;g.epoch=(g.epoch+1)&0xfffffu;const auto material=g.process;const auto epoch=g.epoch;const auto timestep=command.timestep;const auto inherited=command.inherited;impl_->pending.push_back([material,epoch,timestep,inherited](){material->set_shader_parameter("epoch",int(epoch));material->set_shader_parameter("timestep",timestep);material->set_shader_parameter("inherited",godot::Vector3(inherited[0],inherited[1],inherited[2]));});++impl_->steps;}
  else for(const auto& p:command.particles){godot::Transform3D transform;transform.origin={p.position[0],p.position[1],p.position[2]};const float s=float(p.speed)*.0078125f*(1.f/127);auto* node=g.node;const auto epoch=g.epoch;impl_->pending.push_back([node,transform,p,s,epoch](){node->emit_particle(transform,{p.direction[0]*s,p.direction[1]*s,p.direction[2]*s},{1,1,1,1},{float(p.life),float(p.angle),float(unsigned(p.frame)+unsigned(p.inherit)*256),float(epoch)},31);});g.centroid=p.position;++impl_->emissions;}
 }
 for(auto id:impl_->order){auto& g=impl_->groups.at(id);if(removed.count(id)){g.node->hide();continue;}if(changed.count(id)){const auto base=g.node->get_base();impl_->pending.push_back([base](){godot::RenderingServer::get_singleton()->particles_request_process(base);});}g.node->hide();if(!enabled||(require_depth&&depth.is_null()))continue;
  const auto set=[&](const godot::StringName& n,const godot::Variant& v){g.draw->set_shader_parameter(n,v);if(impl_->parameter)impl_->parameter(g.draw,n,v);};set("particle_clip",matrix(frame.clip_matrix));set("particle_camera",godot::Vector3(frame.position[0],frame.position[1],frame.position[2]));set("particle_direction",godot::Vector3(frame.direction[0],frame.direction[1],frame.direction[2]));set("particle_focal",frame.focal_h);set("particle_planes",godot::Vector2(frame.near_z,frame.far_z));const auto coefficients=port::track::screen_depth_coefficients(frame.near_z,frame.far_z,0,true);set("particle_original_depth",godot::Vector2(coefficients[0],coefficients[1]));if(depth.is_valid())set("particle_raw_depth",depth);
  if(!camera)throw std::invalid_argument("GPU particles require source camera visibility bounds");const auto points=camera_visibility_corners(camera);godot::AABB bounds(points[0],{});for(unsigned i=1;i<8;++i)bounds.expand_to(points[i]);g.node->set_visibility_aabb(bounds);g.node->set_custom_aabb(bounds);g.node->set_layer_mask(layer);g.node->set_sorting_use_aabb_center(false);float z=0;for(unsigned i=0;i<3;++i)z+=(g.centroid[i]-frame.position[i])*frame.direction[i];if(g.alpha_gap)out.gaps.push_back("GPU particle definition "+std::to_string(g.definition)+" potentially exceeds source-over alpha bound; affected whole quads discarded on GPU, exact count unavailable without readback");out.alpha_keys.push_back({g.order,z});impl_->visible.push_back(id);++out.groups;out.capacity_quads+=1000;
 }
 if(enabled&&require_depth&&depth.is_null())out.gaps.push_back("GPU AcidFX requires original GS depth source");return out;
}
void GpuParticleRenderer::commit_frame(){auto pending=std::move(impl_->pending);impl_->pending.clear();for(auto& action:pending)action();impl_->backup_groups.clear();impl_->backup_order.clear();impl_->prepared=false;}
void GpuParticleRenderer::abort_frame(){impl_->pending.clear();if(!impl_->prepared)return;std::vector<std::uint64_t> added;for(const auto& pair:impl_->groups)if(!impl_->backup_groups.count(pair.first))added.push_back(pair.first);for(auto id:added)impl_->remove(id);impl_->groups=std::move(impl_->backup_groups);impl_->order=std::move(impl_->backup_order);impl_->emissions=impl_->backup_emissions;impl_->steps=impl_->backup_steps;impl_->visible.clear();impl_->prepared=false;}
void GpuParticleRenderer::set_alpha_ranks(const std::vector<std::size_t>& ranks,std::size_t total,float origin){if(ranks.size()!=impl_->visible.size())throw std::invalid_argument("GPU particle rank count mismatch");for(std::size_t i=0;i<ranks.size();++i){if(ranks[i]>=total)throw std::invalid_argument("GPU particle alpha rank exceeds common submission");auto* n=impl_->groups.at(impl_->visible[i]).node;n->set_sorting_offset(origin-float(total-ranks[i])*4096.f);n->show();}}
godot::Dictionary GpuParticleRenderer::stats()const{godot::Dictionary d;d["gpu_draw_materials"]=int64_t(impl_->draw_materials.size());d["gpu_nodes"]=int64_t(impl_->groups.size());d["capacity_quads"]=int64_t(impl_->groups.size()*1000);d["gpu_emitted_particles"]=int64_t(impl_->emissions);d["gpu_manual_steps"]=int64_t(impl_->steps);d["gpu_node_creations"]=int64_t(impl_->creations);d["mesh_updates"]=int64_t(0);d["uploaded_bytes"]=int64_t(0);d["gpu_emission_payload_bytes"]=int64_t(impl_->emissions*128);d["gpu_visibility_exact"]=false;d["gpu_readbacks"]=int64_t(0);return d;}
}







