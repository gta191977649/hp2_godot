#pragma once
// TASK-0045. Port-owned test geometry/publication, never a second tire model.
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace port::driving {
class Memory;
enum class SurfaceMode { Uniform=0, Zones=1 };
struct SurfaceSettings { SurfaceMode mode=SurfaceMode::Uniform; int terrain=1; };
inline bool operator==(const SurfaceSettings& a,const SurfaceSettings& b){return a.mode==b.mode&&a.terrain==b.terrain;}
inline bool operator!=(const SurfaceSettings& a,const SurfaceSettings& b){return !(a==b);}
// Native +X is the confirmed vehicle forward axis, mapped to host +Z.
inline constexpr std::array<float,4> surface_boundaries{{25,50,75,100}};
inline constexpr std::array<int,5> surface_terrain_ids{{1,2,6,7,8}};
// Explicit tool colors, not recovered terrain textures or PS2 packed colors.
inline constexpr std::array<std::array<float,3>,5> surface_colors{{
    {{0.25f,0.28f,0.32f}},{{0.47f,0.29f,0.14f}},{{0.20f,0.40f,0.18f}},
    {{0.72f,0.61f,0.36f}},{{0.82f,0.86f,0.91f}}
}};
void validate_surface(const SurfaceSettings&);
int surface_material_at(const SurfaceSettings&,float native_x);
struct SurfaceMaterial {
    int id=0;
    std::string name;
    float static_friction=0,dynamic_friction=0,rolling_friction=0;
    float noise_spacing_inverse=0,noise_scale=0;
};
std::array<SurfaceMaterial,5> read_surface_catalog(const Memory&);
std::vector<SurfaceMaterial> read_track_material_catalog(const Memory&);
// Stored original Wheel/TireInfo values; no inferred combined grip coefficient.
struct GripSnapshot {
    std::array<float,3> ground_point{};std::array<float,4> ground_plane{};
    int terrain=0;
    bool slipping=false;
    float z_force=0,current_slip=0,traction_usage=0,traction_boost=0,catchup_friction=0,road_noise=0;
    std::array<float,2> slip{},ground_force{};
    float static_coefficient=0,dynamic_coefficient=0;
};
GripSnapshot observe_grip(const Memory&,std::uint32_t wheel);
}
