#pragma once

#include "indep/inc/AnimatedDriver.hpp"
#include "systems/cinematic/pose_interpolation.hpp"

#include <array>
#include <cstdint>
#include <vector>
#include <godot_cpp/variant/aabb.hpp>

struct Car;
struct CarCinematicViewFrame;
namespace godot { class Node3D; class MeshInstance3D; class Skeleton3D; }

namespace hp2 {

struct VehicleEntityState;
struct RaceStateView;
struct CarPaintAssets;
class LightSceneRegistry;
struct VehiclePresentation {
    static constexpr std::uint32_t primaryLayer = 1u << 1;
    static constexpr std::uint32_t rearLayer = 1u << 2;
    godot::Node3D* root{};
    godot::AABB bodyBounds;
    std::vector<godot::MeshInstance3D*> meshes;
    std::array<godot::MeshInstance3D*, 4> wheels{};
    std::array<godot::MeshInstance3D*, 4> wheelBlur{};
    std::array<godot::MeshInstance3D*, 3> driverMeshes{};
    std::array<godot::Skeleton3D*, 3> driverSkeletons{};
    std::array<PoseInterpolation, 3> driverPoses;
    std::array<std::uint32_t, 3> driverModels{};
    std::array<std::uint32_t, 2> driverTextures{};
    godot::Vector3 driverSeat;
    float driverTranslation[3]{};
    float driverRotations[CAR_DRIVER_VISUAL_BONE_COUNT][4]{};
    std::uint32_t driverStep{};
    std::uint64_t driverReplayEpoch{};
    bool driverPoseValid{};
    int model{}, skin{};
    std::uint32_t layers{};

    void attach(godot::Node3D& parent, Car& car, const VehicleEntityState& state, int worldFrames,
        const CarPaintAssets& paint);
    void update(const VehicleEntityState& state, const LightSceneRegistry& lights,
        const float* environment, const RaceStateView& race, bool showExterior);
    void update_driver(Car& car, const VehicleEntityState& state, const CarCinematicViewFrame& camera,
        std::uint32_t step, std::uint64_t replayEpoch, float fraction);
};

}
