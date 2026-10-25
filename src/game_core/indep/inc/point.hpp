#pragma once
// Debug ../indep/inc/point.hpp, E-0098-U01. Typed host fields, not PS2 ABI.
#include <array>
#include <cstdint>
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
    float GetDerivative(float) const;
    float GetSecondDerivative(float) const;
    void ClampDerivative(float);
    void ClampSecondDerivative(float);
};
struct cPoint {static void SplineSeek(tCubic1D&,float time,float fDClamp,float fDDClamp);};
}
