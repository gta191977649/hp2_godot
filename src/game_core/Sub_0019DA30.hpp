#pragma once
// TASK-0038, E-0038-I30..I32, original source paths unknown.
// SLUS_203.62 revision unknown; SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
#include "port/driving/memory.hpp"
#include <functional>
namespace port::driving::native {
// ELF0019DA30 initializes body damping headers, not their ELF-resident sample arrays.
void Sub_0019DA30(Memory&);
// ELF001395B0 initializes irregular table330060, including mutable points2F0570.
void Sub_001395B0(Memory&);
// Port-side observation/replay seam; each actual original call must be supplied.
struct ScheduleServices {
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> Sub_00183358;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> Sub_00183498;
};
// ELF001A33A8/33C0 and00136DD0, exact quantization constants/instruction grouping.
float Sub_001A33A8(Memory&,std::uint32_t);
std::uint32_t Sub_001A33C0(float);
std::uint32_t Sub_00136DD0(float);
// ELF001A33F0 chooses bounded quantized frame delta from actual elapsed input.
void Sub_001A33F0(Memory&,float,ScheduleServices&);
// ELF001A3500,001A3598,001A35E0,001A35F0, frame/game clock stores.
void Sub_001A3500(Memory&);
void Sub_001A3598(Memory&);
void Sub_001A35E0(Memory&,float);
void Sub_001A35F0(Memory&);
}
