#pragma once
#include "game_core/port/track/assets.hpp"
#include "game_core/port/driving/memory.hpp"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <functional>
#include <memory>
namespace port::godot_bridge {
struct EnvironmentResourceLookup {
 std::function<godot::Ref<godot::ArrayMesh>(std::size_t,std::size_t)> mesh;
 std::function<godot::Ref<godot::Texture2D>(std::uint32_t)> texture;
 std::function<godot::Ref<godot::Texture2D>(std::uint32_t,unsigned)> mip;
};
struct EnvironmentFrame {port::track::WeatherResult weather;std::size_t sky_draws=0;std::vector<std::string> gaps;};
class TrackEnvironmentRenderer {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 TrackEnvironmentRenderer(godot::Node3D&,EnvironmentResourceLookup);
 ~TrackEnvironmentRenderer();
 void clear();
 EnvironmentFrame update(godot::Camera3D*,std::shared_ptr<const port::track::Scene>,port::driving::Memory&,std::uint32_t frame_counter,float timestep,bool sky_enabled=true,unsigned original_view_id=1,unsigned host_layer=1);
};
}
