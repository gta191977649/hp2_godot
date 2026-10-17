#include "Sub_00137C88.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
namespace port::driving {
// ELF 00137C88..00137D37; E-0038-003. No physical field meanings asserted.
float Sub_00137C88(float f12,float f13,float f14,float f15,float f16,float f17,int a0,float& a1){
    const auto bits=[](std::uint32_t raw){float f;std::memcpy(&f,&raw,4);return f;};
    a1=std::fabs(a1);const float f6=a1+1.0f;
    const float f5=std::fabs(f12);
    f15=f15+(a0!=0?f17*f16:f16);
    if(0.5f<f13&&f5<bits(0x3eb2b8c4))return f6;
    // gp A710/A714/A718/A71C/A720: 0,1,0,30,0.7, read through Ghidra.
    const float f3=f14-0.0f;
    const float f2=30.0f-0.0f;
    const float f0=(1.0f-0.0f)*(f3/f2);
    const float f1=0.0f+f0;
    float result=(f5*f15)*f1;
    if(bits(0x3f333333)<result)result=bits(0x3f333333);
    return f6+result;
}
}
