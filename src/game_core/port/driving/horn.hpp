#pragma once
// Port-side binding of the original horn activation to host-owned audio voices.
// E-0082-H01; Debug NFSHP2.ELF815b60c7...332f1b, reverse/task0082/horn.md.
#include "memory.hpp"
namespace port::driving {
// BaseCarSound0018B788/0018B7C8 gates -> Sound3d::Activate002039C0.
// Only activation state is bound here; PCM/voice transport is a host service.
void set_horn_activation(Memory&,std::uint32_t base_car_sound,bool active);
bool horn_activation(const Memory&,std::uint32_t base_car_sound);
}
