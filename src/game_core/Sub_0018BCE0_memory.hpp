// TASK-0041 E-0041-R02; retail 0x0018BCE0: DriveTrain::SetDesiredGear(Gear).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#pragma once
// Port-side adapters to the E-0038-D01..D07 scalar projections. No state is retained here.
#include "port/driving/memory.hpp"
namespace port::driving::native {
std::int32_t Sub_0018BCA0(Memory&,std::uint32_t,std::int32_t);
std::int32_t SetDesiredGear(Memory&,std::uint32_t,std::int32_t);
void Sub_0018ABF0(Memory&,std::uint32_t);
void Sub_0018B948(Memory&,std::uint32_t);
void Sub_0018BAA0(Memory&,std::uint32_t);
}
