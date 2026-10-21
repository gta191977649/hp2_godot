#pragma once
// TASK-0086 E-0086-F01. Port-owned projections/transport; no original ABI claim.
#include "memory.hpp"
#include <array>
#include <optional>
#include <vector>
namespace port::driving {
struct VehicleFlareParams {
 float min_size=0,max_size=0;
 std::array<float,4> min_colour{},max_colour{};
 float power=0,z_bias=0,min_scale=0,max_scale=0;
 std::uint32_t texture_id=0;
};
struct VehicleFlareResources {
 std::array<VehicleFlareParams,10> parameters{};
 std::array<std::array<int,2>,10> parameter_indices{}; // -1 is original null
 std::array<std::uint32_t,3> texture_hashes{};
 std::array<std::array<float,3>,6> normals{};
 float minimum_angle=0;
};
struct VehicleFlareMarker {
 std::uint32_t name_hash=0;
 std::uint8_t type=0;
 std::uint16_t flags=0;
 bool enabled=true;
 std::array<float,3> position{},direction{};
};
struct VehicleFlareState {
 // Port order: HL left/right, brake left/centre/right, reverse left/right,
 // cop head left/right, cop red/blue/white. Values include original 0..1 clamp.
 std::array<float,12> intensity{};
};
struct VehicleFlareFrame {
 std::array<float,3> camera_position{};
 std::array<float,16> world_clip{},world_view{},camera_matrix{};
 float h=0; // Original eView::H, not viewport pixel count
};
struct VehicleFlareBillboard {
 std::uint32_t texture_hash=0,name_hash=0;
 unsigned parameter_index=0;
 std::array<float,3> centre{};
 std::array<std::array<float,3>,4> corners{}; // ePoly order: TL,TR,BR,BL
 std::array<std::uint8_t,4> colour{};
 float intensity=0,half_size=0,depth=0;
};
VehicleFlareResources read_vehicle_flare_resources(const Memory&);
// Unknown nonlamp markers return nullopt, as CreateCarLightFlares ignores them.
std::optional<VehicleFlareMarker> create_vehicle_flare_marker(const VehicleFlareResources&,
 std::uint32_t name_hash,const std::array<float,3>& position,std::uint32_t param,
 const std::array<float,3>& pivot);
VehicleFlareState vehicle_flare_state(const Memory&,std::uint32_t car,int time_of_day=-1,
 std::uint32_t car_timebase_start=0,std::uint32_t broken_states=0,std::uint32_t force_light_state=0);
float vehicle_flare_intensity(const VehicleFlareState&,std::uint32_t name_hash);
// Original bit-float approximation at Debug00116574..001165BC; never std::pow.
float vehicle_flare_angle_power(float angle,float power,float minimum_angle);
std::vector<VehicleFlareBillboard> vehicle_flare_billboards(const VehicleFlareResources&,
 const std::vector<VehicleFlareMarker>&,const VehicleFlareState&,
 const std::array<float,16>& local_world,const VehicleFlareFrame&);
}
