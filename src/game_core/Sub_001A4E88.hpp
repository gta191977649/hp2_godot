// TASK-0041 E-0041-M01; retail 0x001A4E88: Wheel::ModifyGroundForce(bVector2 *).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#pragma once
#include <array>
namespace port::driving {
// Port-side field projection, not an original type/layout. Original source path unknown.
struct WheelIterationProjection {
    float config_Field_0xc=0,Field_0x24=0,Field_0xb0=0,Field_0xc0=0,Field_0x108=0,Field_0x114=0;
};
// ELF 001A4E88; E-0038-005; SLUS_203.62 revision unknown; SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Finite ordinary-float slice. No physical field names or PS2 exceptional FPU equivalence.
void ModifyGroundForce(WheelIterationProjection&,const std::array<float,2>&);
}
