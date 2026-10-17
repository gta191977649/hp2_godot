#pragma once
// Port-side EE instruction projection, not an original game helper or parameter.
// TASK-0038 E-0038-E12; only finite operands / a signed-zero denominator supported.
// See reverse/driving/task0038-ee-zero-divide.md for instruction sites and ISA reference.
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
namespace port::driving {
inline float ee_div_s_zero(float numerator,float denominator,const char* instruction_address) {
    if(!std::isfinite(numerator) || !std::isfinite(denominator) || denominator!=0.0f)
        hp2::not_implemented(instruction_address,"Unsupported operand outside port-side zero-denominator EE DIV.S projection");
    std::uint32_t a,b;std::memcpy(&a,&numerator,4);std::memcpy(&b,&denominator,4);
    const std::uint32_t bits=((a^b)&0x80000000u)|0x7f7fffffu;
    float value;std::memcpy(&value,&bits,4);return value;
}
}
