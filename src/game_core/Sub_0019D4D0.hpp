// TASK-0041 E-0041-R03; retail 0x0019D4D0: RigidBody::GetDerivative(float *).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#pragma once
// TASK-0038, E-0038-B01..B07. Port-side logical-address projection; original path unknown.
// SLUS_203.62 revision unknown; SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
#include "port/driving/memory.hpp"
#include <functional>
namespace port::driving::native {
using Vector=std::array<float,4>;
using Matrix=std::array<float,16>;
// ELF0025AE18, E-0038-B01. Column-major product, preserves instruction grouping.
Matrix Sub_0025AE18(const Matrix&,const Matrix&);
Matrix transpose(const Matrix&);
Vector transform(Memory&,const Matrix&,const Vector&);
// ELF0019CD90/0019CE80, E-0038-S03/S04 logical-address adapters.
void Sub_0019CD90(Memory&,std::uint32_t,const Vector&,const Vector&);
void Sub_0019CE80(Memory&,std::uint32_t,const Vector&,const Vector&);
// ELF0019D370, E-0038-B02; caller supplies original angular vector and matrix.
Matrix Sub_0019D370(const Vector&,const Matrix&);
// ELF0019CC88/0019D848, E-0038-B03/B04. Unconsumed stack W is explicit tool input.
void Sub_0019CC88(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
void Sub_0019D848(Memory&,std::uint32_t,float unused_stack_w=0);
// ELF0019D4D0, E-0038-B05. Writes the original derivative buffer, including affine lane15.
void GetDerivative(Memory&,std::uint32_t,std::uint32_t);
// Port dispatch seam, original adjusted virtual targets resolved by its implementation.
struct IntegrationCalls {
    std::function<void(Memory&,std::uint32_t,std::uint32_t)> derivative;
    std::function<void(Memory&,std::uint32_t)> after;
};
// ELF0019C670/0019C290, E-0038-B06/B07.
void Sub_0019C670(Memory&,std::uint32_t,std::uint32_t,const IntegrationCalls&);
void Sub_0019C290(Memory&,float,std::uint32_t,const IntegrationCalls&);
}
