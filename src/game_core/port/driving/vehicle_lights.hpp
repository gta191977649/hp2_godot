#pragma once
// Port-owned publication of Debug00120350 texture-state selection, E-0082-L01.
// This is not the original CarRenderInfo allocation/layout or flare renderer.
#include "memory.hpp"
namespace port::driving {
struct VehicleLightState {
    bool headlight_left=false,headlight_right=false,brakelights=false;
};
VehicleLightState vehicle_light_state(const Memory&,std::uint32_t car,
    int time_of_day=-1,bool frontend=false,std::uint32_t car_timebase_start=0);
}
