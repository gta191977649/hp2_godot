#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <optional>
#include <vector>

// Port-side presentation data. These are not PS2 ABI structures.
namespace port::render {
struct VehicleVertex {
    std::array<float, 3> position{};
    std::array<float, 2> uv{};
    std::uint16_t raw_color{};
    bool has_color{};
    std::array<std::int8_t,3> raw_normal{};
    std::array<float,3> normal{};
    bool has_normal{};
};
struct VehicleSurface {
    std::uint32_t texture_hash{};
    std::uint32_t material_hash{};
    std::uint16_t render_flags{};
    std::array<std::uint32_t, 4> packet_control{};
    std::uint8_t packet_mode{};
    std::array<std::uint8_t, 64> strip_entry{};
    std::vector<VehicleVertex> vertices;
    std::vector<std::uint32_t> indices;
};
// E0089W04: port projection of original eRigidAnimation, not a PS2 ABI object.
struct VehicleRigidAnimation {
    std::int16_t num_frames{};
    std::int8_t triggerable_flag{},frame_rate_scale{};
    std::uint32_t name_hash{};
    std::string name;
    std::vector<std::array<float,16>> matrices;
};
struct VehicleSolid {
    std::string name;
    std::uint32_t hash{};
    std::uint16_t strip_flags{}; // Original eSolid aggregate StripFlags+0x32.
    std::array<float, 3> bounds_min{};
    std::array<float, 3> bounds_max{};
    std::array<float, 16> transform{};
    std::vector<VehicleSurface> surfaces;
    std::uint32_t external_rigid_animation_hash{};
    std::optional<VehicleRigidAnimation> rigid_animation;
    bool rigid_animation_unresolved=false;
};
// E-0086-M01, Debug LoaderPositionMarkers001A5670/ePositionMarker.
// Port projection excludes the original intrusive runtime link words.
struct VehiclePositionMarker {
    std::uint32_t name_hash{},type_hash{};
    std::array<float,3> position{};
    std::int32_t param{};
};
struct VehicleGeometry {
    std::vector<VehicleSolid> solids;
    std::vector<VehiclePositionMarker> position_markers;
    std::vector<std::string> gaps;
};
} // namespace port::render
