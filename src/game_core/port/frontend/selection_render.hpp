#pragma once
// TASK-0098 E-0098-R01. Typed host projection, not original Car.hpp/PS2 ABI.
#include "game_core/indep/inc/Fred.hpp"
#include <functional>
namespace hp2::debug {
struct eModel {std::uint32_t NameHash=0;};
struct FrontEndRenderingCar {
    std::array<float,3> Position{};
    std::array<float,16> BodyMatrix{};
    std::array<std::array<float,16>,4> TireMatrices{},BrakeMatrices{};
    int LightsOn=0,CopLightsOn=0,Reflection=0,Visible=1;
    const eModel* OverrideModel=nullptr;
};
}
namespace port::selection {
// Debug00123EA8 -> RenderStrips0010E120, E-0098-F03/RB01. Reflection
// suppresses environment mapping(bit1) and ApplyAlphaSorting strips(bit16).
// Port projection of the original flags, not an original function declaration.
inline std::uint32_t frontend_render_flags(const hp2::debug::FrontEndRenderingCar& car){
    return car.Reflection?0x11u:0u;
}
inline bool frontend_strip_enabled(std::uint32_t flags,bool apply_alpha_sorting){
    return !(flags&0x10u) || !apply_alpha_sorting;
}
struct RenderServices {
    std::function<void(int,int,int)> get_skin_info;
    std::function<void(hp2::debug::FrontEndRenderingCar&,const hp2::debug::RideInfo&)> reinit;
};
// Debug00123EA8/001210F0 normal path vs00123E98 override. Bounds are original Body0.
std::array<float,16> frontend_render_matrix(const hp2::debug::FrontEndRenderingCar&,
    const std::array<float,16>& car_scale,const std::array<float,3>& bounds_min,
    const std::array<float,3>& bounds_max);
}
