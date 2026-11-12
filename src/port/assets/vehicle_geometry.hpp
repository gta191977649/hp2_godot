#pragma once
#include <set>
#include "port/render/vehicle.hpp"
#include <cstdint>
#include <vector>
namespace port::assets {
// Port-side, bounds-checked reader for the shipped 0x34000 container.
// Animated solids retain original two-matrix/weight carriers (E-0112-SKIN01);
// static geometry and full packed XYZ operands remain unchanged.
render::VehicleGeometry decode_vehicle_geometry(const std::vector<std::uint8_t>& data, const std::set<std::uint32_t>& hashes = {});
std::vector<render::VehiclePositionMarker> decode_vehicle_position_markers(const std::vector<std::uint8_t>& data);
}
