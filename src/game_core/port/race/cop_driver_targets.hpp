#pragma once
// E-0112-COP01, original copdriver.cpp functions; port-native vector carriers.
#include "game_core/port/driving/memory.hpp"
namespace hp2::debug {
bool FindDriveTargetPointByDistance(::port::driving::Memory&,std::array<float,2>&,float&,std::uint32_t,const std::array<float,2>&,float,const std::array<float,2>&);
void GetDriveTargetPointByDLOffsetFromStart(::port::driving::Memory&,std::array<float,2>&,std::uint32_t,const std::array<float,2>&);
}
