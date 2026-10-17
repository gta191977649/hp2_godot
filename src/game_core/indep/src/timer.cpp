
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001A33C0; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\timer.cpp
namespace port::driving::native {
std::uint32_t GetQuantizedRealTimeElapsed(float f12){
    using namespace detail_group_0019da30;f12=f12*3600.0f;f12=f12+0.5f;return static_cast<std::uint32_t>(port::driving::cvt_w_s(f12));}
}
