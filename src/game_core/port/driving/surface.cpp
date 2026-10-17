#include "surface.hpp"
#include "memory.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace port::driving {
void validate_surface(const SurfaceSettings& value){
    if(value.mode!=SurfaceMode::Uniform&&value.mode!=SurfaceMode::Zones)
        throw std::invalid_argument("Unsupported test surface mode");
    if(std::find(surface_terrain_ids.begin(),surface_terrain_ids.end(),value.terrain)==surface_terrain_ids.end())
        throw std::invalid_argument("Test surface must be ROAD, DIRT, GRASS, SAND or SNOW");
}
int surface_material_at(const SurfaceSettings& value,float x){
    validate_surface(value);
    if(!std::isfinite(x))throw std::invalid_argument("Nonfinite test surface coordinate");
    if(value.mode==SurfaceMode::Uniform)return value.terrain;
    return surface_terrain_ids[std::upper_bound(surface_boundaries.begin(),surface_boundaries.end(),x)-surface_boundaries.begin()];
}
std::array<SurfaceMaterial,5> read_surface_catalog(const Memory& m){
    // E-0045-T01: Debug TerrainTypeInfoTable003102E8, sizeof TerrainTypeInfo=0x90.
    // Validate every row's declared type; catalogue only the supported test surfaces.
    for(unsigned i=0;i<33;++i)if(m.word(0x003102e8+i*0x90+8)!=i)
        throw std::runtime_error("Debug terrain table identity mismatch");
    std::array<SurfaceMaterial,5> out;
    for(unsigned i=0;i<out.size();++i){
        auto& row=out[i];row.id=surface_terrain_ids[i];const auto a=0x003102e8+row.id*0x90;
        const auto name=m.word(a);
        for(unsigned j=0;j<128;++j){const auto ch=m.byte(name+j);if(!ch)break;row.name.push_back(static_cast<char>(ch));}
        if(row.name.empty()||row.name.size()==128)throw std::runtime_error("Invalid Debug terrain name");
        row.noise_spacing_inverse=m.scalar(a+0x18);row.noise_scale=m.scalar(a+0x1c);
        row.static_friction=m.scalar(a+0x20);row.dynamic_friction=m.scalar(a+0x24);row.rolling_friction=m.scalar(a+0x28);
        for(float f:{row.noise_spacing_inverse,row.noise_scale,row.static_friction,row.dynamic_friction,row.rolling_friction})
            if(!std::isfinite(f)||f<0)throw std::runtime_error("Invalid Debug terrain coefficient publication");
    }
    return out;
}
GripSnapshot observe_grip(const Memory& m,std::uint32_t a){
    // E-0045-W01: exact Debug Wheel/TireInfo fields, observed after Move/UpdateCar.
    // E-0048-H02: stored Wheel ground point+130 and terrain plane+140;
    // no second topology sample or inferred contact face is introduced.
    GripSnapshot out;const auto ground=m.vector(a+0x130);out.ground_point={{ground[0],ground[1],ground[2]}};out.ground_plane=m.vector(a+0x140);out.terrain=m.signed_word(a+0x150);out.slipping=m.byte(a+0x154)!=0;
    out.z_force=m.scalar(a+0x158);out.road_noise=m.scalar(a+0x15c);out.current_slip=m.scalar(a+0x160);
    out.traction_usage=m.scalar(a+0x168);out.traction_boost=m.scalar(a+0x178);out.catchup_friction=m.scalar(a+0x17c);
    out.slip={{m.scalar(a+0x100),m.scalar(a+0x104)}};out.ground_force={{m.scalar(a+0x114),m.scalar(a+0x118)}};
    const auto tire=m.word(a+0x10);out.static_coefficient=m.scalar(tire+0xc);out.dynamic_coefficient=m.scalar(tire+0x10);
    if(out.terrain<0||out.terrain>=33)throw std::runtime_error("Invalid Debug wheel terrain publication");
    for(float f:{out.z_force,out.road_noise,out.current_slip,out.traction_usage,out.traction_boost,out.catchup_friction,
                 out.slip[0],out.slip[1],out.ground_force[0],out.ground_force[1],out.static_coefficient,out.dynamic_coefficient})
        if(!std::isfinite(f))throw std::runtime_error("Nonfinite Debug wheel diagnostic publication");
    return out;
}
std::vector<SurfaceMaterial> read_track_material_catalog(const Memory& m){
    // E-0045-T01 and E-0048-D01: original Debug table, semantic Retail identity map.
    std::vector<SurfaceMaterial> result;
    for(int i=0;i<33;++i){const auto a=0x003102e8+i*0x90;if(m.word(a+8)!=std::uint32_t(i))throw std::runtime_error("Debug terrain table identity mismatch");
        SurfaceMaterial row;row.id=i;const auto name=m.word(a);
        for(unsigned n=0;n<128;++n){auto ch=m.byte(name+n);if(!ch)break;row.name.push_back(char(ch));}
        if(row.name.empty()||row.name.size()==128)throw std::runtime_error("Invalid Debug terrain name");
        row.noise_spacing_inverse=m.scalar(a+0x18);row.noise_scale=m.scalar(a+0x1c);row.static_friction=m.scalar(a+0x20);row.dynamic_friction=m.scalar(a+0x24);row.rolling_friction=m.scalar(a+0x28);
        for(float f:{row.noise_spacing_inverse,row.noise_scale,row.static_friction,row.dynamic_friction,row.rolling_friction})if(!std::isfinite(f))throw std::runtime_error("Nonfinite track terrain coefficient");
        result.push_back(std::move(row));
    }return result;
}
}
