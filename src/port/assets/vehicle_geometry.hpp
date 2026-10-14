#pragma once
#include <set>
#include "port/render/vehicle.hpp"
#include <cstdint>
#include <vector>
namespace port::assets {
// Port-side, bounds-checked reader for the shipped 0x34000 container.
render::VehicleGeometry decode_vehicle_geometry(const std::vector<std::uint8_t>& data, const std::set<std::uint32_t>& hashes = {});
}
