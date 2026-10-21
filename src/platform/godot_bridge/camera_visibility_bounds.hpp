#pragma once
// Host-only CPU bounds for shaders that project vertices independently of the
// mesh transform. Preserve camera layer filtering: instance_set_ignore_culling
// bypasses that filtering and must not be used for per-view geometry.
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <array>
#include <stdexcept>
namespace port::godot_bridge {
using CameraVisibilityCorners=std::array<godot::Vector3,8>;
inline CameraVisibilityCorners camera_visibility_corners(godot::Camera3D* camera){
 if(!camera||!camera->get_viewport())throw std::invalid_argument("Camera visibility bounds require a viewport");
 const auto size=camera->get_viewport()->get_visible_rect().size;
 const std::array<godot::Vector2,4> corners{{{0,0},{size.x,0},{size.x,size.y},{0,size.y}}};
 CameraVisibilityCorners points;
 for(unsigned plane=0;plane<2;++plane)for(unsigned corner=0;corner<4;++corner)
  points[plane*4+corner]=camera->project_position(corners[corner],plane?camera->get_far():camera->get_near());
 return points;
}
inline void apply_camera_visibility_bounds(godot::MeshInstance3D* mesh,const CameraVisibilityCorners& points){
 const auto inverse=mesh->get_global_transform().affine_inverse();
 godot::AABB bounds(inverse.xform(points[0]),{});
 for(std::size_t i=1;i<points.size();++i)bounds.expand_to(inverse.xform(points[i]));
 if(mesh->get_custom_aabb()!=bounds)mesh->set_custom_aabb(bounds);
}
}
