#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Port-side presentation data. These are not PS2 ABI structures.
namespace port::render {
struct VehicleVertex {
    std::array<float, 3> position{};
    std::array<float, 2> uv{};
};
struct VehicleSurface {
    std::uint32_t texture_hash{};
    std::uint32_t material_hash{};
    std::vector<VehicleVertex> vertices;
    std::vector<std::uint32_t> indices;
};
struct VehicleSolid {
    std::string name;
    std::uint32_t hash{};
    std::array<float, 3> bounds_min{};
    std::array<float, 3> bounds_max{};
    std::array<float, 16> transform{};
    std::vector<VehicleSurface> surfaces;
};
struct VehicleGeometry {
    std::vector<VehicleSolid> solids;
    std::vector<std::string> gaps;
};
} // namespace port::render
