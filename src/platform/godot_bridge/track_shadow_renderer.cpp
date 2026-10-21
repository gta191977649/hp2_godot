#include "track_shadow_renderer.hpp"
#include "game_core/CarRenderInfo.hpp"
#include "platform/shaders/track_shader.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <stdexcept>
namespace port::godot_bridge {
namespace {
std::array<float,3> native(const godot::Variant& value){const godot::Vector3 p=value;return {{float(p.x),float(p.y),float(p.z)}};}
godot::Vector3 host(const std::array<float,3>& p){return {p[1],p[2],p[0]};}
struct Texture {hp2::assets::TpkTextureHeader header;godot::Ref<godot::Texture2D> image;unsigned maximum_alpha=0;};
Texture texture(const godot::Dictionary& d){Texture t;auto& h=t.header;t.image=d["texture"];h.tex_hash=std::uint32_t(std::int64_t(d["hash"]));h.width=int(d["width"]);h.height=int(d["height"]);h.alpha_bits=int(d["alpha_bits"]);h.alpha_fix=int(d["alpha_fix"]);h.texture_fx=int(d["texture_fx"]);h.Field_0x4c=int(d["repeat"]);h.Field_0x4d=int(d["sorting"]);h.Field_0x50=int(d["write_depth"]);h.Field_0x6e=int(d["mxl"]);h.Field_0x6f=int(d["mmag"]);h.Field_0x70=int(d["mmin"]);h.Field_0x71=int(d["lod_l"]);h.Field_0x72=int(d["lod_k"]);h.Field_0x74=int(d["gloss"]);h.is_any_semitransparency=int(d["semi"]);t.maximum_alpha=int(d["maximum_alpha"]);if(t.image.is_null()||h.alpha_bits!=0x44||h.alpha_fix||h.Field_0x6e)throw std::runtime_error("Shadow texture is outside original bounded ALPHA44/MXL0 state");return t;}
}
struct TrackShadowRenderer::Impl {godot::Node3D& owner;std::vector<godot::MeshInstance3D*> nodes;std::map<std::uint32_t,godot::Ref<godot::ShaderMaterial>> materials;explicit Impl(godot::Node3D& n):owner(n){}};
TrackShadowRenderer::TrackShadowRenderer(godot::Node3D& n):impl_(std::make_unique<Impl>(n)){}
TrackShadowRenderer::~TrackShadowRenderer(){clear();}
void TrackShadowRenderer::clear(){for(auto* n:impl_->nodes){if(n->get_parent())n->get_parent()->remove_child(n);memdelete(n);}impl_->nodes.clear();impl_->materials.clear();}
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
 for(std::size_t i=0;i<impl_->nodes.size();++i){auto* node=impl_->nodes[i];if(i>=packet.strips.size()){node->hide();continue;}const auto& strip=packet.strips[i];const auto& t=textures.at(strip.ramp?ramp:fallback);if(unsigned(strip.colour[3])*t.maximum_alpha>=129u*128u)throw std::runtime_error("Original shadow alpha exceeds bounded source-over range");
 godot::PackedVector3Array positions;godot::PackedVector2Array uv;godot::PackedColorArray colours;godot::PackedInt32Array indices;
 for(const auto& v:strip.vertices){positions.push_back(host(v.position));uv.push_back({v.uv[0],v.uv[1]});colours.push_back(godot::Color(strip.colour[0]/255.f,strip.colour[1]/255.f,strip.colour[2]/255.f,strip.colour[3]/255.f));}
 for(unsigned j=0;j+2<strip.vertices.size();++j){indices.push_back(j+(j&1));indices.push_back(j+1-(j&1));indices.push_back(j+2);++result.triangles;}
 godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);arrays[godot::Mesh::ARRAY_VERTEX]=positions;arrays[godot::Mesh::ARRAY_TEX_UV]=uv;arrays[godot::Mesh::ARRAY_COLOR]=colours;arrays[godot::Mesh::ARRAY_INDEX]=indices;godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays);
 auto found=impl_->materials.find(t.header.tex_hash);if(found==impl_->materials.end()){
 port::track::EffectState effect;effect.texture=port::track::texture_state_00108CC0(t.header);effect.test=port::vehicle::test_state_00105AC0(t.header);effect.test.write_depth=false;
 godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code(godot::String::utf8(track_shader_source(t.header,effect,port::track::Fusion40::None).c_str()));godot::Ref<godot::ShaderMaterial> material;material.instantiate();material->set_shader(shader);material->set_render_priority(1);material->set_shader_parameter("base_lit",false);found=impl_->materials.emplace(t.header.tex_hash,material).first;
 }const auto material=found->second;material->set_shader_parameter("base_texture",t.image);
 node->set_mesh(mesh);node->set_material_override(material);node->set_layer_mask(layer);node->set_global_transform(godot::Transform3D());node->set_meta("original_car_shadow",true);node->show();++result.strips;
 }
 return result;
}
}
