#pragma once
#include "game_core/port/driving/vehicle_flares.hpp"
#include "port/vehicle/textures.hpp"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <functional>
#include <memory>
namespace port::godot_bridge {
// Port particle presentation layer for persistent attached sprites. No simulation.
class ParticleBillboardRenderer {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using TextureLookup=std::function<godot::Ref<godot::Texture2D>(std::uint32_t)>;
 ParticleBillboardRenderer(godot::Node3D&,TextureLookup);
 ~ParticleBillboardRenderer();
 void clear();
 void update(const std::vector<port::driving::VehicleFlareBillboard>&,
             const std::vector<port::vehicle::Texture>&,
             const godot::Vector3& camera,const godot::Vector3& forward);
 godot::Dictionary stats()const;
};
}
