
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x00152070; E-0042-C021; E-0042-S001; exact Debug TU ..\indep\src\track.cpp
namespace port::driving {
float PlaneContact::GetElevation(Memory& m,std::uint32_t,std::uint32_t point,
    std::uint32_t material,std::uint32_t plane,std::uint32_t found) {
    const auto p=m.vector(point);
    for(float v:p)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite synthetic plane query");
    if(material)m.word(material,1);
    if(plane)m.vector(plane,{{0,0,1,height}});
    if(found)m.word(found,1);
    return height;
}
}
