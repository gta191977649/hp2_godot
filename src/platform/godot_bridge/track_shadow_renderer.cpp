#include "track_shadow_renderer.hpp"
#include "game_core/CarRenderInfo.hpp"
#include "platform/shaders/track_shader.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace port::godot_bridge {
namespace {
std::array<float,3> native(const godot::Variant& value){const godot::Vector3 p=value;return {{float(p.x),float(p.y),float(p.z)}};}
godot::Vector3 host(const std::array<float,3>& p){return {p[1],p[2],p[0]};}
struct Texture {hp2::assets::TpkTextureHeader header;godot::Ref<godot::Texture2D> image;unsigned maximum_alpha=0;};
Texture texture(const godot::Dictionary& d){Texture t;auto& h=t.header;t.image=d["texture"];h.tex_hash=std::uint32_t(std::int64_t(d["hash"]));h.width=int(d["width"]);h.height=int(d["height"]);h.alpha_bits=int(d["alpha_bits"]);h.alpha_fix=int(d["alpha_fix"]);h.texture_fx=int(d["texture_fx"]);h.Field_0x4c=int(d["repeat"]);h.Field_0x4d=int(d["sorting"]);h.Field_0x50=int(d["write_depth"]);h.Field_0x6e=int(d["mxl"]);h.Field_0x6f=int(d["mmag"]);h.Field_0x70=int(d["mmin"]);h.Field_0x71=int(d["lod_l"]);h.Field_0x72=int(d["lod_k"]);h.Field_0x74=int(d["gloss"]);h.is_any_semitransparency=int(d["semi"]);t.maximum_alpha=int(d["maximum_alpha"]);if(t.image.is_null()||h.alpha_bits!=0x44||h.alpha_fix||h.Field_0x6e)throw std::runtime_error("Shadow texture is outside original bounded ALPHA44/MXL0 state");return t;}

godot::Color strip_colour(const port::track::ShadowStrip& strip) {
 const auto& colour=strip.colour;
 return {colour[0]/255.f,colour[1]/255.f,colour[2]/255.f,colour[3]/255.f};
}

godot::Ref<godot::ArrayMesh> create_strip_mesh(const port::track::ShadowStrip& strip,bool dynamic) {
 godot::PackedVector3Array positions;
 godot::PackedVector2Array uv;
 godot::PackedColorArray colours;
 godot::PackedInt32Array indices;
 const auto colour=strip_colour(strip);
 for(const auto& vertex:strip.vertices){
  positions.push_back(host(vertex.position));
  uv.push_back({vertex.uv[0],vertex.uv[1]});
  colours.push_back(colour);
 }
 for(unsigned j=0;j+2<strip.vertices.size();++j){
  indices.push_back(j+(j&1));
  indices.push_back(j+1-(j&1));
  indices.push_back(j+2);
 }
 godot::Array arrays;
 arrays.resize(godot::Mesh::ARRAY_MAX);
 arrays[godot::Mesh::ARRAY_VERTEX]=positions;
 arrays[godot::Mesh::ARRAY_TEX_UV]=uv;
 arrays[godot::Mesh::ARRAY_COLOR]=colours;
 arrays[godot::Mesh::ARRAY_INDEX]=indices;
 godot::Ref<godot::ArrayMesh> mesh;
 mesh.instantiate();
 const auto flags=dynamic?std::uint64_t(godot::Mesh::ARRAY_FLAG_USE_DYNAMIC_UPDATE):0;
 mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays,{}, {},flags);
 return mesh;
}

// Port-side transport. Winding and strip ordering still come from the original
// packet. Stable vertex count means the existing index buffer is immutable.
struct ShadowMeshStream {
 godot::Ref<godot::ArrayMesh> mesh;
 std::size_t count=0;
 bool layout_supported=false;
 godot::PackedByteArray positions,attributes;
 godot::AABB bounds;

 static bool supports_layout(const godot::ArrayMesh& candidate,int32_t count) {
  if(candidate.get_surface_count()!=1)return false;
  auto* rs=godot::RenderingServer::get_singleton();
  const godot::BitField<godot::RenderingServer::ArrayFormat> format(candidate.surface_get_format(0));
  return rs->mesh_surface_get_format_vertex_stride(format,count)==12
      && rs->mesh_surface_get_format_attribute_stride(format,count)==12
      && rs->mesh_surface_get_format_offset(format,count,godot::RenderingServer::ARRAY_VERTEX)==0
      && rs->mesh_surface_get_format_offset(format,count,godot::RenderingServer::ARRAY_COLOR)==0
      && rs->mesh_surface_get_format_offset(format,count,godot::RenderingServer::ARRAY_TEX_UV)==4
      && !(std::uint64_t(format)&godot::Mesh::ARRAY_FLAG_COMPRESS_ATTRIBUTES);
 }

 template<std::size_t size>
 static bool replace_bytes(std::uint8_t* destination,const void* source) {
  if(std::memcmp(destination,source,size)==0)return false;
  std::memcpy(destination,source,size);
  return true;
 }

 godot::Ref<godot::ArrayMesh> update(const port::track::ShadowStrip& strip,ShadowFrame& result) {
  const bool rebuild=mesh.is_null()||count!=strip.vertices.size()||!layout_supported;
  if(rebuild){
   mesh=create_strip_mesh(strip,true);
   ++result.mesh_creations;
   count=strip.vertices.size();
   layout_supported=count>=3&&supports_layout(*mesh.ptr(),count);
   if(!layout_supported)return mesh;
   positions.resize(count*12);
   attributes.resize(count*12);
   positions.fill(0);
   attributes.fill(0);
   bounds=mesh->get_aabb();
  }
  auto* xyz_bytes=positions.ptrw();
  auto* attr_bytes=attributes.ptrw();
  const auto colour=strip_colour(strip);
  std::uint8_t rgba[4];
  for(int channel=0;channel<4;++channel)
   rgba[channel]=std::uint8_t(std::clamp(double(colour[channel])*255.0,0.0,255.0));
  bool positions_dirty=false,attributes_dirty=false;
  auto minimum=host(strip.vertices.front().position);
  auto maximum=minimum;
  for(const auto& vertex:strip.vertices){
   const auto point=host(vertex.position);
   const float xyz[]{float(point.x),float(point.y),float(point.z)};
   positions_dirty|=replace_bytes<sizeof(xyz)>(xyz_bytes,xyz);
   attributes_dirty|=replace_bytes<sizeof(rgba)>(attr_bytes,rgba);
   attributes_dirty|=replace_bytes<sizeof(vertex.uv)>(attr_bytes+4,vertex.uv.data());
   xyz_bytes+=12;
   attr_bytes+=12;
   minimum=minimum.min(point);
   maximum=maximum.max(point);
  }
  if(!rebuild){
   if(positions_dirty){
    mesh->surface_update_vertex_region(0,0,positions);
    ++result.region_updates;
    // Match RenderingServer::_surface_set_data's minimum extent, including
    // flat strips, so spatial culling and transparent sort centers agree.
    constexpr godot::real_t epsilon=0.00001;
    const godot::AABB updated(minimum,(maximum-minimum).max({epsilon,epsilon,epsilon}));
    if(updated!=bounds){
     mesh->set_custom_aabb(updated);
     bounds=updated;
    }
   }
   if(attributes_dirty){
    mesh->surface_update_attribute_region(0,0,attributes);
    ++result.region_updates;
   }
  }
  return mesh;
 }
};
}
struct TrackShadowRenderer::Impl {
 godot::Node3D& owner;
 bool stream_meshes;
 std::vector<godot::MeshInstance3D*> nodes;
 std::vector<ShadowMeshStream> streams;
 std::map<std::uint32_t,godot::Ref<godot::ShaderMaterial>> materials;
 explicit Impl(godot::Node3D& n,bool stream):owner(n),stream_meshes(stream){}
};
TrackShadowRenderer::TrackShadowRenderer(godot::Node3D& n,bool stream):impl_(std::make_unique<Impl>(n,stream)){}
TrackShadowRenderer::~TrackShadowRenderer(){clear();}
void TrackShadowRenderer::clear(){for(auto* n:impl_->nodes){if(n->get_parent())n->get_parent()->remove_child(n);memdelete(n);}impl_->nodes.clear();impl_->streams.clear();impl_->materials.clear();}

godot::Dictionary TrackShadowRenderer::test_mesh_stream(){
 godot::Dictionary fixture;
 port::track::ShadowStrip strip;
 strip.vertices={{{0,-1,-1},{0,0}},{{0,1,-1},{1,0}},{{0,-1,1},{0,1}},{{0,1,1},{1,1}}};
 strip.colour={37,61,129,93};
 ShadowMeshStream stream;
 ShadowFrame counts;
 const auto initial=stream.update(strip,counts);
 stream.update(strip,counts);
 const bool stationary=counts.mesh_creations==1&&counts.region_updates==0;
 for(auto& vertex:strip.vertices)vertex.position[1]+=0.125f;
 const auto moved=stream.update(strip,counts);
 const bool movement=initial==moved&&counts.mesh_creations==1&&counts.region_updates==1;
 strip.vertices[1].uv={0.875f,0.125f};
 strip.colour={97,67,127,109};
 const auto updated=stream.update(strip,counts);
 const auto baseline=create_strip_mesh(strip,false);
 const bool transport=updated==moved&&counts.mesh_creations==1&&counts.region_updates==2
     && updated->get_custom_aabb()==baseline->get_aabb();
 const auto baseline_arrays=baseline->surface_get_arrays(0);
 const auto updated_arrays=updated->surface_get_arrays(0);
 const bool arrays_match=baseline_arrays==updated_arrays;
 strip.vertices.pop_back();
 const auto smaller=stream.update(strip,counts);
 const bool topology=smaller!=updated&&counts.mesh_creations==2
     && smaller->surface_get_arrays(0)==create_strip_mesh(strip,false)->surface_get_arrays(0);
 fixture["ok"]=stationary&&movement&&transport&&arrays_match&&topology;
 fixture["stationary"]=stationary;
 fixture["movement"]=movement;
 fixture["transport"]=transport;
 fixture["arrays_match"]=arrays_match;
 fixture["topology"]=topology;
 fixture["baseline"]=baseline;
 fixture["updated"]=updated;
 return fixture;
}

ShadowFrame TrackShadowRenderer::update(const godot::Dictionary& visual,const port::track::Scene& scene,port::driving::Memory& math,const port::track::Topology& topology,bool enabled,unsigned layer){
 ShadowFrame result;if(!enabled||!visual.has("available")||!bool(visual["available"])||!bool(visual.get("driving_pose",false))){clear();return result;}
 port::track::ShadowInput input;input.topology=&topology;input.car_position=native(visual["geometry_position_native"]);input.bounds_min=native(visual["bounds_min_native"]);input.bounds_max=native(visual["bounds_max_native"]);input.projected_size=double(visual["projected_size"]);input.car_lod=int(visual["lod"]);input.lod_swap1=double(visual["lod_swap1"]);input.lod_swap2=double(visual["lod_swap2"]);
 const godot::PackedFloat32Array matrix=visual["geometry_matrix_native"];if(matrix.size()!=16)throw std::runtime_error("Shadow geometry matrix must have16native lanes");for(int i=0;i<16;++i)input.local_world[i]=matrix[i];
 const godot::PackedVector3Array cv=visual["collision_vertices_native"],wheels=visual["wheel_centers_native"];if(wheels.size()!=4)throw std::runtime_error("Shadow requires four original wheel centers");for(int i=0;i<wheels.size();++i)input.wheels[i]=native(wheels[i]);for(int i=0;i<cv.size();++i)input.collision_vertices.push_back(native(cv[i]));
 std::map<std::uint32_t,Texture> textures;const godot::Array values=visual["shadow_textures"];for(int i=0;i<values.size();++i){auto t=texture(values[i]);textures.emplace(t.header.tex_hash,std::move(t));}
 const auto ramp=std::uint32_t(std::int64_t(visual["shadow_ramp_hash"])),fallback=std::uint32_t(std::int64_t(visual["shadow_ig_hash"]));input.fancy=textures.count(ramp)&&!input.collision_vertices.empty();
 input.fallback=textures.count(fallback)&&bool(visual.get("fallback_eligible",false));
 if(!input.fancy)result.gaps.push_back("Original fancy shadow requires the real car CV and SHADOWRAMP texture");
 if(!visual.has("fallback_eligible"))result.gaps.push_back("Fallback shadow PartCuller13 eligibility is not restored; branch is not guessed");
 const auto packet=hp2::recovered::CarRenderInfo::Render(math,scene,input);
 while(impl_->nodes.size()<packet.strips.size()){auto* n=memnew(godot::MeshInstance3D);n->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);impl_->owner.add_child(n);impl_->nodes.push_back(n);}
 if(impl_->stream_meshes&&impl_->streams.size()<packet.strips.size())impl_->streams.resize(packet.strips.size());
 for(std::size_t i=0;i<impl_->nodes.size();++i){auto* node=impl_->nodes[i];if(i>=packet.strips.size()){node->hide();continue;}const auto& strip=packet.strips[i];const auto& t=textures.at(strip.ramp?ramp:fallback);if(unsigned(strip.colour[3])*t.maximum_alpha>=129u*128u)throw std::runtime_error("Original shadow alpha exceeds bounded source-over range");
 result.triangles+=strip.vertices.size()>2?strip.vertices.size()-2:0;
 godot::Ref<godot::ArrayMesh> mesh;
 if(impl_->stream_meshes)mesh=impl_->streams[i].update(strip,result);
 else{mesh=create_strip_mesh(strip,false);++result.mesh_creations;}
 auto found=impl_->materials.find(t.header.tex_hash);if(found==impl_->materials.end()){
 port::track::EffectState effect;effect.texture=port::track::texture_state_00108CC0(t.header);effect.test=port::vehicle::test_state_00105AC0(t.header);effect.test.write_depth=false;
 godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code(godot::String::utf8(track_shader_source(t.header,effect,port::track::Fusion40::None).c_str()));godot::Ref<godot::ShaderMaterial> material;material.instantiate();material->set_shader(shader);material->set_render_priority(1);material->set_shader_parameter("base_lit",false);found=impl_->materials.emplace(t.header.tex_hash,material).first;
 }const auto material=found->second;material->set_shader_parameter("base_texture",t.image);
 if(node->get_mesh()!=mesh)node->set_mesh(mesh);
 node->set_material_override(material);node->set_layer_mask(layer);node->set_global_transform(godot::Transform3D());node->set_meta("original_car_shadow",true);node->show();++result.strips;
 }
 return result;
}
}
