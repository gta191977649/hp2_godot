#include "track_environment_renderer.hpp"
#include "camera_visibility_bounds.hpp"
#include "game_core/src/skye.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include "game_core/port/track/runtime.hpp"
#include "platform/shaders/track_shader.hpp"
#include "platform/shaders/sky_depth.hpp"
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <algorithm>
#include <godot_cpp/variant/vector4.hpp>
#include <stdexcept>
namespace port::godot_bridge {
namespace {
godot::Vector3 host(const std::array<float,3>& p){return {p[1],p[2],p[0]};}
godot::Transform3D matrix(const std::array<float,16>& m){godot::Basis b;b.set_column(0,host({{m[4],m[5],m[6]}}));b.set_column(1,host({{m[8],m[9],m[10]}}));b.set_column(2,host({{m[0],m[1],m[2]}}));return {b,host({{m[12],m[13],m[14]}})};}
}
struct TrackEnvironmentRenderer::Impl {
 godot::Node3D& owner;EnvironmentResourceLookup lookup;std::shared_ptr<const port::track::Scene> scene;
 std::vector<godot::MeshInstance3D*> sky;std::vector<std::string> gaps;
 Impl(godot::Node3D& n,EnvironmentResourceLookup r):owner(n),lookup(std::move(r)){}
};
TrackEnvironmentRenderer::TrackEnvironmentRenderer(godot::Node3D& n,EnvironmentResourceLookup r):impl_(std::make_unique<Impl>(n,std::move(r))){}
TrackEnvironmentRenderer::~TrackEnvironmentRenderer(){clear();}
void TrackEnvironmentRenderer::clear(){for(auto* n:impl_->sky){if(n->get_parent())n->get_parent()->remove_child(n);memdelete(n);}impl_->sky.clear();impl_->scene.reset();impl_->gaps.clear();}
EnvironmentFrame TrackEnvironmentRenderer::update(godot::Camera3D* camera,std::shared_ptr<const port::track::Scene> scene,port::driving::Memory&,std::uint32_t,float,bool enabled,unsigned original_view_id,unsigned host_layer){
 EnvironmentFrame result;if(!scene||!camera){clear();return result;}
 const auto p=camera->get_global_position();result.weather=port::track::query_weather(scene->environment,{{float(p.z),float(p.x),float(p.y)}});
 if(impl_->scene!=scene){clear();impl_->scene=scene;const auto* sky=hp2::recovered::StuffSky(scene->environment,original_view_id,true);
 if(!sky)impl_->gaps.push_back("Original sky model is absent");else {
 std::size_t solid=0;while(solid<scene->geometry.solids.size()&&scene->geometry.solids[solid].hash!=sky->hash)++solid;
 if(solid==scene->geometry.solids.size())throw std::runtime_error("Original sky geometry dependency is absent");
 port::track::DrawCommand model;model.solid=solid;model.instance=sky->instance;model.matrix=sky->local_world;model.has_light_context=false;model.identity=false;
 port::track::FrameView view;port::track::Frame frame;hp2::recovered::eModel::Render(*scene,view,model,frame);
 for(auto command:frame.commands){const auto& surface=scene->geometry.solids[solid].surfaces.at(command.surface);
 const auto texture=std::find_if(scene->textures.begin(),scene->textures.end(),[&](const auto& t){return t.header.tex_hash==surface.texture_hash;});if(texture==scene->textures.end())throw std::runtime_error("Original sky texture dependency is absent");
 command.effect.test.write_depth=false;command.effect.matrix_pack=0;
 auto code=track_shader_source(texture->header,command.effect,port::track::Fusion40::None);
 // Host background-depth adapter: preserve native fixed model coordinates and
 // projected XY while allowing the original100..50000 sky pass behind world.
 const auto vertex=code.find("void vertex(){");if(vertex==std::string::npos)throw std::runtime_error("Sky shader vertex entry absent");
 const auto end=code.find("void fragment",vertex);const auto brace=code.rfind('}',end);if(brace==std::string::npos)throw std::runtime_error("Sky shader vertex boundary absent");
 code.insert(brace,"\n    sky_view_depth=-(MODELVIEW_MATRIX*vec4(VERTEX,1.0)).z;\n"+std::string(sky_far_depth_vertex));
 code.insert(vertex,"varying float sky_view_depth;\n");const auto fragment_start=code.find("void fragment");const auto fragment_brace=code.find('{',fragment_start);code.insert(fragment_brace+1,"\n    if(sky_view_depth<100.0||sky_view_depth>50000.0)discard;\n");
 godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code(godot::String::utf8(code.c_str()));godot::Ref<godot::ShaderMaterial> material;material.instantiate();material->set_shader(shader);material->set_render_priority(-128);material->set_shader_parameter("base_texture",impl_->lookup.texture(surface.texture_hash));material->set_shader_parameter("base_lit",false);material->set_shader_parameter("flag_0x4",bool(command.effect.flags&4));
 for(unsigned level=1;level<=texture->header.Field_0x6e;++level)material->set_shader_parameter(godot::String::utf8(("mip_"+std::to_string(level)).c_str()),impl_->lookup.mip(surface.texture_hash,level));
 auto* node=memnew(godot::MeshInstance3D);node->set_mesh(impl_->lookup.mesh(solid,command.surface));node->set_material_override(material);node->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);impl_->owner.add_child(node);node->set_transform(matrix(sky->local_world));node->set_meta("original_sky",true);node->set_layer_mask(host_layer);node->set_ignore_occlusion_culling(true);impl_->sky.push_back(node);
 }
 }
 }
 const auto bounds=camera_visibility_corners(camera);
 for(auto* n:impl_->sky){apply_camera_visibility_bounds(n,bounds);n->set_visible(enabled);}result.sky_draws=enabled?impl_->sky.size():0;result.gaps=impl_->gaps;
 return result;
}
}
