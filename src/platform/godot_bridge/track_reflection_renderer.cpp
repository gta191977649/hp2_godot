#include "track_reflection_renderer.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/mesh_instance2d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <cmath>
#include <stdexcept>
namespace port::godot_bridge {
namespace {
godot::Vector3 host(const port::track::EnvironmentVector& v){return {v[1],v[2],v[0]};}
godot::Transform3D camera_transform(const port::track::EnvironmentFace& face){
    // Original look-at rows are right,up,forward in native world coordinates.
    const auto&m=face.camera_matrix;
    port::track::EnvironmentVector position{};
    for(unsigned k=0;k<3;++k)position[k]=-(m[k*4]*m[12]+m[k*4+1]*m[13]+m[k*4+2]*m[14]);
    const auto right=host({{m[0],m[4],m[8]}}),up=host({{m[1],m[5],m[9]}}),forward=host({{m[2],m[6],m[10]}});
    godot::Basis basis;basis.set_column(0,right);basis.set_column(1,up);basis.set_column(2,-forward);
    return {basis,host(position)};
}
}
struct TrackReflectionRenderer::Impl {
    godot::Node* container=nullptr;
    std::array<CaptureView,6> faces{};
    unsigned scheduled=0;godot::SubViewport* atlas=nullptr;
    std::vector<godot::MeshInstance2D*> patches;
    std::vector<port::track::EnvironmentWarp> warps;
    SubmitView submit;
    port::track::ReflectionTables tables=port::track::reflection_tables();
    port::track::EnvironmentMatrix hack{};
    std::string diagnostic="Original vehicle environment origin unavailable";
    unsigned captured=0;
    bool available=false;
    explicit Impl(godot::Node*owner,SubmitView callback):submit(std::move(callback)){
        auto* owner3d=godot::Object::cast_to<godot::Node3D>(owner);
        if(!owner3d||!submit)throw std::invalid_argument("Reflection capture requires a Node3D owner and shared submission callback");
        container=memnew(godot::Node);container->set_name("OriginalEnvironmentCapture");owner->add_child(container);
        for(unsigned i=0;i<faces.size();++i){auto& f=faces[i];f.face_index=i;f.view_id=4+i;f.host_layer=1u<<(i+1);
            f.viewport=memnew(godot::SubViewport);f.viewport->set_name(godot::String("TrackReflectionFace")+godot::String::num_int64(i));f.viewport->set_size({64,64});f.viewport->set_use_own_world_3d(false);f.viewport->set_world_3d(owner3d->get_world_3d());f.viewport->set_disable_3d(false);f.viewport->set_use_hdr_2d(godot::RenderingServer::get_singleton()->get_current_rendering_method()!="gl_compatibility");f.viewport->set_update_mode(godot::SubViewport::UPDATE_DISABLED);container->add_child(f.viewport);
            f.scene_root=memnew(godot::Node3D);f.viewport->add_child(f.scene_root);
            f.camera=memnew(godot::Camera3D);f.camera->set_cull_mask(f.host_layer);f.camera->set_keep_aspect_mode(godot::Camera3D::KEEP_WIDTH);f.camera->set_near(.5);f.camera->set_far(10000);f.scene_root->add_child(f.camera);f.camera->set_current(true);
        }
        atlas=memnew(godot::SubViewport);atlas->set_name("TrackReflectionAtlas");atlas->set_size({256,256});atlas->set_disable_3d(true);atlas->set_transparent_background(true);atlas->set_clear_mode(godot::SubViewport::CLEAR_MODE_ONCE);atlas->set_update_mode(godot::SubViewport::UPDATE_DISABLED);container->add_child(atlas);
        const std::array<bool,6> all{{true,true,true,true,true,true}};
        warps=port::track::environment_warps(tables,all,256,256);
        godot::Ref<godot::Shader> shader;shader.instantiate();shader->set_code("shader_type canvas_item;render_mode unshaded,blend_disabled;void fragment(){vec3 raw=clamp(texture(TEXTURE,UV).rgb*255.0,vec3(0),vec3(255));COLOR=vec4(floor(raw/8.0)*8.0/255.0,128.0/255.0);}");
        for(const auto& warp:warps){godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);godot::PackedVector3Array positions;godot::PackedVector2Array uv;godot::PackedInt32Array indices;
            for(const auto&v:warp.vertices){positions.push_back({v.destination[0]*256,v.destination[1]*256,0});uv.push_back({v.source_uv[0],v.source_uv[1]});}for(auto index:warp.indices)indices.push_back(index);
            arrays[godot::Mesh::ARRAY_VERTEX]=positions;arrays[godot::Mesh::ARRAY_TEX_UV]=uv;arrays[godot::Mesh::ARRAY_INDEX]=indices;
            godot::Ref<godot::ArrayMesh> mesh;mesh.instantiate();mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays);
            auto* patch=memnew(godot::MeshInstance2D);patch->set_mesh(mesh);patch->set_texture(faces[warp.source_face].viewport->get_texture());patch->set_texture_filter(godot::CanvasItem::TEXTURE_FILTER_LINEAR);patch->set_texture_repeat(godot::CanvasItem::TEXTURE_REPEAT_DISABLED);
            godot::Ref<godot::ShaderMaterial> material;material.instantiate();material->set_shader(shader);patch->set_material(material);patch->set_visible(false);atlas->add_child(patch);patches.push_back(patch);
        }
    }
};
TrackReflectionRenderer::TrackReflectionRenderer(godot::Node*owner,SubmitView submit):impl_(std::make_unique<Impl>(owner,std::move(submit))){}
TrackReflectionRenderer::~TrackReflectionRenderer(){if(impl_->container){if(auto*parent=impl_->container->get_parent())parent->remove_child(impl_->container);memdelete(impl_->container);}}
void TrackReflectionRenderer::clear(){impl_->scheduled=0;impl_->available=false;impl_->captured=0;impl_->diagnostic="Original vehicle environment origin unavailable";for(auto& f:impl_->faces)f.viewport->set_update_mode(godot::SubViewport::UPDATE_DISABLED);for(auto*p:impl_->patches)p->set_visible(false);impl_->atlas->set_clear_mode(godot::SubViewport::CLEAR_MODE_ONCE);impl_->atlas->set_update_mode(godot::SubViewport::UPDATE_DISABLED);}
bool TrackReflectionRenderer::update(const port::track::EnvironmentVector& viewer,const std::optional<port::track::EnvironmentVector>& origin,std::uint32_t frame,float original_capture_focal_h,int mode,bool in_game,bool backwards){
    impl_->scheduled=0;    if(!origin){clear();return false;}
    try{if(!std::isfinite(original_capture_focal_h)||original_capture_focal_h<=0)throw std::invalid_argument("Original environment focal H must be positive and finite");const double capture_fov=2.0*std::atan(32.0/original_capture_focal_h)*180.0/3.14159265358979323846;const auto cameras=port::track::environment_cameras(viewer,*origin,frame,mode,in_game);unsigned pending=0;
        for(unsigned i=0;i<6;++i){auto& face=impl_->faces[i];face.viewport->set_update_mode(godot::SubViewport::UPDATE_DISABLED);face.native=cameras.faces[i];if(!face.native.active)continue;
            face.exclude_flags=(mode==3||mode==4?0x11u:0x10u)|0x102u|(backwards?4u:8u);face.camera->set_transform(camera_transform(face.native));face.camera->set_fov(capture_fov);face.camera->set_near(face.native.near_z);face.camera->set_far(face.native.far_z);
            std::string reason;if(!impl_->submit(face,reason))throw std::runtime_error(reason.empty()?"Shared environment submission failed":reason);pending|=1u<<i;
        }
        // Publish only after every requested native submission succeeds.
        const auto& warps=impl_->warps;
        for(unsigned i=0;i<6;++i)if(pending&(1u<<i))impl_->faces[i].viewport->set_update_mode(godot::SubViewport::UPDATE_ONCE);
        for(std::size_t i=0;i<warps.size();++i)impl_->patches[i]->set_visible((pending&(1u<<warps[i].source_face))!=0);
        impl_->scheduled=pending;impl_->atlas->set_update_mode(godot::SubViewport::UPDATE_ONCE);impl_->captured|=pending;impl_->hack=cameras.hack_man_matrix;impl_->available=impl_->captured==0x3f;impl_->diagnostic=impl_->available?"Host capture uses RGBA8 accumulation; original per-draw PSMCT16 RGB/Z precision remains unverified":"Original environment capture warming up";return true;
    }catch(const std::exception&e){clear();impl_->diagnostic=e.what();return false;}
}
bool TrackReflectionRenderer::ready()const{return impl_->available;}
const std::string& TrackReflectionRenderer::diagnostic()const{return impl_->diagnostic;}
const port::track::EnvironmentMatrix& TrackReflectionRenderer::hack_matrix()const{return impl_->hack;}
godot::Array TrackReflectionRenderer::scheduled_viewports()const{godot::Array result;for(unsigned i=0;i<6;++i)if(impl_->scheduled&(1u<<i))result.push_back(impl_->faces[i].viewport->get_viewport_rid());if(impl_->scheduled)result.push_back(impl_->atlas->get_viewport_rid());return result;}
godot::Ref<godot::Texture2D> TrackReflectionRenderer::texture()const{return impl_->available?godot::Ref<godot::Texture2D>(impl_->atlas->get_texture()):godot::Ref<godot::Texture2D>();}
godot::Ref<godot::Texture2D> TrackReflectionRenderer::face_texture(unsigned i)const{return impl_->faces.at(i).viewport->get_texture();}
}
