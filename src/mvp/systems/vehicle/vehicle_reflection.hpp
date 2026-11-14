#pragma once

#include <memory>

namespace godot { class Node3D; }
namespace hp2 {
struct CarPaintAssets;
struct RaceStateView;
class VehicleReflection final {
public:
    VehicleReflection(godot::Node3D& scene, const RaceStateView& race);
    ~VehicleReflection();
    void update(const RaceStateView& race, double elapsed, bool reset);
    const CarPaintAssets& assets() const;
    const float* matrix() const;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
