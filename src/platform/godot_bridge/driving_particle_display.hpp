#pragma once
// Port-owned flat Test drive display. Track consumers use their shared renderer.
#include "track_particle_renderer.hpp"
#include "game_core/port/driving/original_image.hpp"
#include <map>
namespace port::godot_bridge {
class Ps2DrivingRuntime;
class DrivingParticleDisplay {
    std::shared_ptr<port::track::Scene> scene_;
    port::driving::Memory math_;
    std::map<std::uint32_t,std::vector<godot::Ref<godot::Texture2D>>> textures_;
    std::unique_ptr<TrackParticleRenderer> renderer_;
    godot::Node3D* owner_=nullptr;
public:
    godot::Dictionary present(Ps2DrivingRuntime&,godot::Node3D&,godot::Camera3D&);
};
}
