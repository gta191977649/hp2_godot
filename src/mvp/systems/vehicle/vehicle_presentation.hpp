#pragma once

#include <array>
#include <cstdint>
#include <vector>

struct Car;
namespace godot { class Node3D; class MeshInstance3D; }

namespace hp2 {

struct VehicleEntityState;
struct RaceStateView;
struct CarPaintAssets;
class LightSceneRegistry;
struct VehiclePresentation {
    static constexpr std::uint32_t primaryLayer = 1u << 1;
    static constexpr std::uint32_t rearLayer = 1u << 2;
    godot::Node3D* root{};
    std::vector<godot::MeshInstance3D*> meshes;
    std::array<godot::MeshInstance3D*, 4> wheels{};
    std::array<godot::MeshInstance3D*, 4> wheelBlur{}, rearWheelBlur{};
    int model{}, skin{};
    std::uint32_t layers{};

    void attach(godot::Node3D& parent, Car& car, const VehicleEntityState& state, int worldFrames,
        const CarPaintAssets& paint);
    void update(const VehicleEntityState& state, const LightSceneRegistry& lights,
        const float* environment, const RaceStateView& race);
};

}
