#pragma once
// Debug ../indep/inc/point.hpp, E-0098-U01. Typed host fields, not PS2 ABI.
#include <array>
#include <cstdint>
namespace port::driving {class Memory;}
namespace hp2::debug {
struct tCubic1D {
    float Val=0,dVal=0,ValDesired=0,dValDesired=0;
    std::array<float,4> Coeff{};
    float time=0,duration=1,arrivalThreshold=1;
    std::int16_t state=0,flags=0;
    void MakeCoeffs();
    float GetVal(float) const;
    float GetdVal(float) const;
    float GetddVal(float) const;
    static float GetddVal(::port::driving::Memory&,std::uint32_t,float); //W106 field binding, same001D7468.
    float GetDerivative(float) const;
    float GetSecondDerivative(float) const;
    void ClampDerivative(float);
    void ClampSecondDerivative(float);
};
struct cPoint {static void SplineSeek(tCubic1D&,float time,float fDClamp,float fDDClamp);
 // W106 explicit memory binding to existing scalar source, no host ABI memcpy.
 static void SplineSeek(::port::driving::Memory&,std::uint32_t,float,float,float);
 static void SplineSeek(::port::driving::Memory&,std::uint32_t,float); //001D7C38.
};
}
