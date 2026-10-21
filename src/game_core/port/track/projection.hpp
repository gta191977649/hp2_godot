#pragma once
// Port seam: keep original screen-relative LOD independent of host output resolution.
#include "game_core/src/ecstasy/ecstasye.hpp"
namespace port::track {
inline float original_focal_h(port::driving::Memory& math,std::uint16_t original_horizontal_fov){
    return hp2::recovered::CalculateH(math,original_horizontal_fov);
}
}
