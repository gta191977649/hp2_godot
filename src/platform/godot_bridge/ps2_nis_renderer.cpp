#include "ps2_nis_renderer.hpp"
#include "ps2_texture_loader.hpp"
#include "boundary.hpp"
#include "platform/ps2_asset_root.hpp"
#include "platform/shaders/car_base_shader.hpp"
#include "port/assets/vehicle_geometry.hpp"
#include "port/assets/catalog.hpp"
#include "port/vehicle/texture_state.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/CarRenderInfo.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <map>
#include <cmath>
namespace port::godot_bridge {
namespace {godot::Vector3 host(const std::array<float,3>& v){return {v[1],v[2],v[0]};}}
struct Ps2NisRenderer::Impl {
 godot::Ref<Ps2TextureLoader> textures;
 std::unique_ptr<assets::DirectorySource> source;
 std::map<std::string,std::vector<std::uint8_t>> banks;
 std::map<std::uint32_t,render::VehicleSolid> solids;
 std::map<std::pair<std::uint32_t,unsigned>,godot::Ref<godot::ShaderMaterial>> materials;
 std::vector<godot::MeshInstance3D*> nodes;
 const render::VehicleSolid& solid(std::uint32_t hash){
  if(const auto found=solids.find(hash);found!=solids.end())return found->second;
  if(!source){const auto start=godot::ProjectSettings::get_singleton()->globalize_path("res://");const auto root=hp2::platform::resolve_zzdata_root(start.utf8().get_data());if(!root)throw std::runtime_error("Original NIS asset root missing");source=std::make_unique<assets::DirectorySource>(*root);}
  for(const auto path:{"GLOBAL/CHARACTERSA.BIN","GLOBAL/INGAMEB.BUN","GLOBAL/GLOBALB.BUN"}){
   auto at=banks.find(path);if(at==banks.end())at=banks.emplace(path,source->read(path)).first;
   render::VehicleGeometry decoded;
   try{decoded=assets::decode_vehicle_geometry(at->second,{hash});}
   catch(const std::runtime_error& error){if(std::string(error.what())=="Vehicle geometry container not found")continue;throw;}
   if(decoded.solids.empty())continue;
   if(decoded.solids.size()!=1)throw std::runtime_error("Ambiguous original NIS solid hash");
   return solids.emplace(hash,std::move(decoded.solids.front())).first->second;
  }
  hp2::not_implemented("0x0010E048","original NIS draw solid missing from actual bound common banks");
 }
 godot::Ref<godot::ShaderMaterial> material(const render::VehicleSurface& surface){
  // E-0112-N01: actual Render(...,NULL,0) sets disable_mask1C0 in
  // RenderStrips0010E120: lighting/environment flags are disabled by source.
  const unsigned flags=surface.render_flags&~0x1c0u;
  const auto key=std::make_pair(surface.texture_hash,flags);
  if(const auto found=materials.find(key);found!=materials.end())return found->second;
  hp2::assets::TpkTextureHeader header;auto texture=textures->load_texture_for_3d(surface.texture_hash,header);
  if(texture.is_null())throw std::runtime_error("Original NIS texture missing: "+std::to_string(surface.texture_hash));
  const auto state=vehicle::texture_state_00108CC0(header);const auto test=vehicle::test_state_00105AC0(header);
  const bool blend=state.blend==vehicle::BlendMode::SourceOver;
  std::string modes="fog_disabled, shadows_disabled, ";modes+=test.write_depth?"depth_draw_always":"depth_draw_never";if(blend||state.deferred)modes+=", blend_mix";
  godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code(godot::String(car_base_shader_source(modes,car_fragment_source(false,test.alpha_test,blend,state.deferred),"filter_linear, repeat_disable").c_str()));
  godot::Ref<godot::ShaderMaterial> value;value.instantiate();value->set_shader(shader);value->set_shader_parameter("base_texture",texture);value->set_shader_parameter("base_lit",false);value->set_shader_parameter("flag_0x4",bool(flags&4)||header.Field_0x74!=0);value->set_render_priority(state.deferred?2:0);
  materials.emplace(key,value);return value;
 }
};
void Ps2NisRenderer::_bind_methods(){godot::ClassDB::bind_method(godot::D_METHOD("apply_packets","packets"),&Ps2NisRenderer::apply_packets);}
Ps2NisRenderer::Ps2NisRenderer():impl_(std::make_unique<Impl>()){
 impl_->textures.instantiate();godot::PackedStringArray banks;for(const char* path:{"GLOBAL/DRIVTEX.BIN","GLOBAL/GLOBALB.BUN","GLOBAL/INGAMEB.BUN","GLOBAL/CHARACTERSA.BIN"})banks.append(path);impl_->textures->set_search_bundles(banks);
}
Ps2NisRenderer::~Ps2NisRenderer()=default;
godot::Dictionary Ps2NisRenderer::apply_packets(const godot::Array& packets){godot::Dictionary result;const auto error=catch_all([&]{
 std::vector<std::pair<godot::Ref<godot::ArrayMesh>,godot::Transform3D>> prepared;
 for(int p=0;p<packets.size();++p){const godot::Dictionary packet=packets[p];if(!packet.has("model_hash")||!packet.has("world")||!packet.has("bones"))throw std::runtime_error("Incomplete immutable NIS draw");
  const auto hash=int64_t(packet["model_hash"]);if(hash<=0||hash>0xffffffffLL)throw std::runtime_error("Invalid NIS solid identity");const auto& solid=impl_->solid(std::uint32_t(hash));const godot::PackedFloat32Array bones=packet["bones"];if(bones.size()%16)throw std::runtime_error("NIS bone packet is not a matrix array");if(solid.animation_node_count&&bones.size()!=solid.animation_node_count*16)throw std::runtime_error("Original NIS solid and bone count differ");
  godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();
  for(const auto& surface:solid.surfaces){if(surface.indices.empty()||(surface.render_flags&0x400))continue;godot::PackedVector3Array positions;godot::PackedVector2Array uv;godot::PackedColorArray colors;godot::PackedInt32Array indices;
   hp2::assets::TpkTextureHeader header;auto texture=impl_->textures->load_texture_for_3d(surface.texture_hash,header);if(texture.is_null())throw std::runtime_error("NIS mesh texture not bound");const auto& entry=surface.strip_entry;const auto factors=port::FixStripSTs(header.width,header.height,unsigned(entry[4])|(unsigned(entry[5])<<8),unsigned(entry[6])|(unsigned(entry[7])<<8));
   for(const auto& vertex:surface.vertices){auto position=vertex.position;
    if(vertex.skin){const auto skin=*vertex.skin;if(skin.matrix0>=bones.size()/16||skin.matrix1>=bones.size()/16)throw std::runtime_error("Original NIS vertex references absent bone");const auto transform=[&](unsigned matrix){std::array<float,3> out{};for(unsigned axis=0;axis<3;++axis)out[axis]=position[0]*bones[matrix*16+axis]+position[1]*bones[matrix*16+4+axis]+position[2]*bones[matrix*16+8+axis]+bones[matrix*16+12+axis];return out;};const auto first=transform(skin.matrix0),second=transform(skin.matrix1);const float weight=skin.matrix1_weight();for(unsigned axis=0;axis<3;++axis)position[axis]=skin.raw_weight==0?first[axis]:skin.raw_weight==1?second[axis]:first[axis]*(1-weight)+second[axis]*weight;
    }else if(!bones.is_empty())hp2::not_implemented("0x0010DB40","animated NIS solid lost original vertex skin bindings");
    for(const auto value:position)if(!std::isfinite(value))throw std::runtime_error("Nonfinite committed NIS vertex");positions.push_back(host(position));uv.push_back({vertex.uv[0]*factors.cropped_image[0],vertex.uv[1]*factors.cropped_image[1]});const auto c=vertex.raw_color;colors.push_back({float((c&31)*8)/255,float(((c>>5)&31)*8)/255,float(((c>>10)&31)*8)/255,float((c&0x8000)?128:0)/255});
   }
   for(const auto index:surface.indices)indices.push_back(index);godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);arrays[godot::Mesh::ARRAY_VERTEX]=positions;arrays[godot::Mesh::ARRAY_TEX_UV]=uv;arrays[godot::Mesh::ARRAY_COLOR]=colors;arrays[godot::Mesh::ARRAY_INDEX]=indices;const auto count=mesh->get_surface_count();mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays);if(mesh->get_surface_count()!=count+1)throw std::runtime_error("GPU rejected original NIS draw strip");mesh->surface_set_material(count,impl_->material(surface));
  }
  prepared.push_back({mesh,godot::Transform3D(packet["world"])});
 }
 while(impl_->nodes.size()<prepared.size()){auto* node=memnew(godot::MeshInstance3D);add_child(node);node->set_layer_mask(1);impl_->nodes.push_back(node);}
 for(unsigned i=0;i<impl_->nodes.size();++i){auto* node=impl_->nodes[i];node->set_visible(i<prepared.size());if(i<prepared.size()){node->set_mesh(prepared[i].first);node->set_transform(prepared[i].second);}else node->set_mesh({});}
 result["draws"]=int(prepared.size());
 });result["ok"]=error.empty();if(!error.empty())result["error"]=godot::String(error.c_str());return result;}
}
