#pragma once

#include <array>

struct Car;
namespace godot { class Node3D; class MeshInstance3D; }

namespace hp2 {

struct VehicleEntityState;
struct VehiclePresentation {
    godot::Node3D* root{};
    std::array<godot::MeshInstance3D*, 4> wheels{};
    int model{}, skin{};

    void attach(godot::Node3D& parent, Car& car, const VehicleEntityState& state, int worldFrames);
    void update(const VehicleEntityState& state) const;
};

}
