#pragma once

#include <chrono>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

struct TerrainTopology;
namespace godot { class AABB; class Camera3D; }

namespace hp2 {

struct RaceCameraState;
struct VehicleEntityState;
void apply_race_camera(godot::Camera3D& camera, const RaceCameraState& state);

class RaceOrbitCamera {
public:
    void capture(bool enabled);
    void reset();
    bool mouse_motion(const godot::Vector2& relative);
    bool apply(godot::Camera3D& camera, const VehicleEntityState& player,
        const godot::AABB& bodyBounds, TerrainTopology* topology, double delta);
    bool active() const { return active_; }
    bool player_exterior() const { return playerExterior_; }
    int terrain_queries() const { return terrainQueries_; }

private:
    godot::Vector2 motion_;
    godot::Vector3 lastOffset_;
    godot::Transform3D entryPose_, lastPose_;
    godot::Basis framing_;
    float entryNear_{}, lastNear_{};
    float yaw_{}, pitch_{}, distance_{}, clearDistance_{}, returnYaw_{};
    std::chrono::steady_clock::time_point lastMotion_{};
    std::chrono::steady_clock::time_point entryStarted_{};
    int terrainQueries_{};
    bool captured_{}, active_{}, returning_{}, hasSafeOffset_{}, playerExterior_{}, entryFromStock_{};
};

}
