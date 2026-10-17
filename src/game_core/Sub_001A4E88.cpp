// TASK-0041 E-0041-M01; retail 0x001A4E88: Wheel::ModifyGroundForce(bVector2 *).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#include "Sub_001A4E88.hpp"
namespace port::driving {
// ELF 001A4E88..001A4EDF; E-0038-005. Keep the operation order; do not cancel factors.
void ModifyGroundForce(WheelIterationProjection& a0,const std::array<float,2>& a1){
    float f3=a0.Field_0x114;
    const float f4=a0.config_Field_0xc;
    float f0=a0.Field_0x108;
    float f1=f3*f4;
    const float f7=a0.Field_0x24;
    const float f6=a1[1],f5=a1[0];
    f0=f0-f1;
    float f2=a0.Field_0xb0;f1=a0.Field_0xc0;
    f2=f2*f5;f0=f0/f7;f1=f1*f6;f2=f2+f1;
    f0=f0*f4;f0=f0-f2;f0=f0*f7;f0=f0/f4;f3=f3+f0;
    a0.Field_0x114=f3;
}
}
