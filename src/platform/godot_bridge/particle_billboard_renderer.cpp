#include "particle_billboard_renderer.hpp"
#include "platform/shaders/track_shader.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/core/object.hpp>
#include <map>
#include <algorithm>
namespace port::godot_bridge {
namespace {
godot::Vector3 billboard_host(const std::array<float,3>& v){return {v[1],v[2],v[0]};}
}
struct ParticleBillboardRenderer::Impl {
 godot::Node3D& owner;TextureLookup texture;
 godot::Ref<godot::ArrayMesh> mesh;
 std::map<std::uint32_t,godot::Ref<godot::ShaderMaterial>> materials;
 std::vector<godot::MeshInstance3D*> nodes;std::vector<std::uint64_t> node_ids;
 std::size_t active=0;std::uint64_t creations=0;
 Impl(godot::Node3D& o,TextureLookup t):owner(o),texture(std::move(t)){}
};
ParticleBillboardRenderer::ParticleBillboardRenderer(godot::Node3D& o,TextureLookup t):impl_(std::make_unique<Impl>(o,std::move(t))){}
ParticleBillboardRenderer::~ParticleBillboardRenderer(){clear();}
void ParticleBillboardRenderer::clear(){for(auto id:impl_->node_ids)if(auto* n=godot::Object::cast_to<godot::MeshInstance3D>(godot::ObjectDB::get_instance(id))){if(n->get_parent())n->get_parent()->remove_child(n);memdelete(n);}impl_->nodes.clear();impl_->node_ids.clear();impl_->materials.clear();impl_->mesh.unref();impl_->active=0;}
void ParticleBillboardRenderer::update(const std::vector<port::driving::VehicleFlareBillboard>& sprites,const std::vector<port::vehicle::Texture>& textures,const godot::Vector3& camera,const godot::Vector3& forward){
 for(auto* n:impl_->nodes)n->hide();impl_->active=0;
 if(sprites.empty())return;
 if(impl_->mesh.is_null()){
  godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);
  godot::PackedVector3Array vertices;for(const auto& v:std::array<godot::Vector3,4>{{{-1,1,0},{1,1,0},{1,-1,0},{-1,-1,0}}})vertices.push_back(v);
  godot::PackedVector2Array uv;for(const auto& v:std::array<godot::Vector2,4>{{{0,0},{1,0},{1,1},{0,1}}})uv.push_back(v);
  godot::PackedInt32Array indices;for(int i:{0,1,3,1,2,3})indices.push_back(i);
  arrays[godot::Mesh::ARRAY_VERTEX]=vertices;arrays[godot::Mesh::ARRAY_TEX_UV]=uv;arrays[godot::Mesh::ARRAY_INDEX]=indices;
  impl_->mesh.instantiate();impl_->mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays);
 }
 std::vector<port::track::AlphaKey> keys;
 for(const auto& sprite:sprites){
  const auto texture=std::find_if(textures.begin(),textures.end(),[&](const auto& t){return t.header.tex_hash==sprite.texture_hash;});
  if(texture==textures.end())hp2::not_implemented("0x00116510","Original vehicle flare texture is not loaded");
  const auto effect=port::track::effect_state(texture->header,0x200);
  if(effect.test.write_depth)hp2::not_implemented("0x00110368","Depth-writing flare material requires explicit instance colour depth-proxy binding");
  if(effect.matrix_pack||effect.environment_pass||effect.rewrite_alpha)hp2::not_implemented("0x00110368","Vehicle flare material requires an unsupported ePoly pass");
  if(texture->header.Field_0x6e)hp2::not_implemented("0x00110368","Vehicle flare mipmapped ePoly material requires explicit mip bindings");
  if(!effect.texture.additive&&effect.texture.blend==port::vehicle::BlendMode::SourceOver){unsigned maximum=0;for(std::size_t p=3;p<texture->bitmap.rgba.size();p+=4)maximum=std::max(maximum,unsigned(texture->bitmap.rgba[p]));if(maximum*unsigned(sprite.colour[3])>=129u*128u)hp2::not_implemented("0x00110368","Original flare source-over alpha exceeds normalized host blend range");}
  keys.push_back({effect.rendering_order,sprite.depth});
  auto& material=impl_->materials[sprite.texture_hash];
  if(material.is_null()){
   auto code=track_shader_source(texture->header,effect,port::track::Fusion40::None);
   const auto vertex=code.find("void vertex(){");code.insert(vertex,"instance uniform vec4 flare_colour;\n");
   const std::string packed="vec4 packed = round(COLOR * 255.0);";const auto at=code.find(packed);if(at==std::string::npos)throw std::logic_error("Flare shader colour seam changed");code.replace(at,packed.size(),"vec4 packed = flare_colour;");
   godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code(godot::String::utf8(code.c_str()));material.instantiate();material->set_shader(shader);
   material->set_shader_parameter("base_lit",false);const auto image=impl_->texture(sprite.texture_hash);if(image.is_null())throw std::runtime_error("Original vehicle flare GPU texture is missing");material->set_shader_parameter("base_texture",image);
  }
  const auto index=impl_->active++;
  if(index==impl_->nodes.size()){auto* n=memnew(godot::MeshInstance3D);n->set_mesh(impl_->mesh);n->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);impl_->owner.add_child(n);n->set_as_top_level(true);n->set_sorting_use_aabb_center(false);impl_->nodes.push_back(n);impl_->node_ids.push_back(n->get_instance_id());++impl_->creations;}
  auto* node=impl_->nodes[index];node->set_material_override(material);
  const auto a=billboard_host(sprite.corners[0]),b=billboard_host(sprite.corners[1]),c=billboard_host(sprite.corners[2]),d=billboard_host(sprite.corners[3]);
  const auto x=(b-a)*.5f,y=(a-d)*.5f;node->set_global_transform({godot::Basis(x,y,x.cross(y).normalized()),(a+c)*.5f});
  node->set_instance_shader_parameter("flare_colour",godot::Vector4(sprite.colour[0],sprite.colour[1],sprite.colour[2],sprite.colour[3]));node->show();
 }
 const auto order=port::track::alpha_order_001055B0(keys);
 for(std::size_t rank=0;rank<order.size();++rank){auto* node=impl_->nodes[order[rank]];const float origin=float((node->get_global_position()-camera).dot(forward));node->set_sorting_offset(origin-float(order.size()-rank)*4096.f);}
}
godot::Dictionary ParticleBillboardRenderer::stats()const{godot::Dictionary d;d["active"]=int64_t(impl_->active);d["nodes"]=int64_t(impl_->nodes.size());d["materials"]=int64_t(impl_->materials.size());d["node_creations"]=int64_t(impl_->creations);d["mesh_uploads"]=impl_->mesh.is_valid()?1:0;d["simulation"]=false;return d;}
}
