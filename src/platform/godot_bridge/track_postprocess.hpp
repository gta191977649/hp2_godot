#pragma once
#include "game_core/port/track/postprocess.hpp"
#include <memory>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
namespace godot { class Node; class Node3D; class Camera3D; class Texture2D; class ShaderMaterial; class MeshInstance3D; class MultiMeshInstance3D; }
namespace port::godot_bridge {
// Host resources only. Original palette/stage numerical behavior lives in core.
class TrackPostprocess {
    struct Impl; std::unique_ptr<Impl> impl_;
public:
    TrackPostprocess(); ~TrackPostprocess();
    TrackPostprocess(const TrackPostprocess&)=delete;
    TrackPostprocess& operator=(const TrackPostprocess&)=delete;
    void update(godot::Node3D* owner,godot::Camera3D* source,
        const port::track::postprocess::Weather&,float original_near,float original_far,
        const port::track::postprocess::Controls&,bool allowed=true,
        float camera_focal_distance=0,float camera_depth_of_field=0,
        const port::track::postprocess::Frame* prepared=nullptr,
        const port::track::postprocess::ScreenControls* screen=nullptr);
    // Sealed meshes require mark_mesh_changed after visibility, mesh, material,
    // transform or sorting changes. Material parameters use their separate API.
    void set_resource_reuse(bool);
    void set_occlusion_enabled(bool);
    godot::Array scheduled_viewports()const;
    godot::Dictionary stats()const;
    void mark_instance_batch(godot::MultiMeshInstance3D*);
    void mark_tracked_mesh(godot::MeshInstance3D*);
    // Host residency eviction: detach proxies before releasing source resources.
    void forget_mesh(godot::MeshInstance3D*);
    void forget_material(const godot::Ref<godot::ShaderMaterial>&);
    enum Change : unsigned { Geometry=1,Transform=2,Visibility=4,Sorting=8,Material=16,Bounds=32,All=63 };
    void mark_mesh_changed(godot::MeshInstance3D*,unsigned changes=All);
    void notify_scene_node(godot::Node*);
    void set_structure_observer(bool,const godot::Callable&);
    void notify_source_order();
    bool needs_structure_observer()const;
    void notify_mesh_sorting(godot::MeshInstance3D*,float offset,bool use_aabb=false);
    void mark_tracked_material(const godot::Ref<godot::ShaderMaterial>&);
    void mark_material_changed(const godot::Ref<godot::ShaderMaterial>&);
    void notify_material_parameter(const godot::Ref<godot::ShaderMaterial>&,const godot::StringName&,const godot::Variant&);
    void mark_material_dirty(const godot::Ref<godot::ShaderMaterial>&);
    void set_screen_resources(const godot::Ref<godot::Texture2D>& filter,const godot::Ref<godot::Texture2D>& radial_mask);
    void release(); bool available()const; bool active()const;
    bool fog_active()const; bool focus_active()const;
    godot::Ref<godot::ViewportTexture> get_depth_texture()const;
    godot::Ref<godot::ViewportTexture> get_colour_texture()const;
};
}
