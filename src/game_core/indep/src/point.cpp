// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\point.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/tCubic1D.hpp"

#include "game_core/indep/src/point.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/inc/point.hpp"
#include <cmath>

namespace hp2::debug {
// Debug001D73B8, E-0098-U01; exact source point.cpp E-0042-S001.
void tCubic1D::MakeCoeffs(){
    const float delta=ValDesired-Val;Coeff[3]=Val;Coeff[2]=dVal;
    Coeff[0]=(dVal+dValDesired)-(delta+delta);
    Coeff[1]=(delta*3.f-dValDesired)-(dVal+dVal);
}
// Debug001D7408, E-0098-U01.
float tCubic1D::GetVal(float t)const{return ((Coeff[0]*t+Coeff[1])*t+Coeff[2])*t+Coeff[3];}
// Debug001D7438, E-0098-U01.
float tCubic1D::GetdVal(float t)const{return ((Coeff[0]*t*3.f+Coeff[1])+Coeff[1])*t+Coeff[2];}
// Debug001D7468, E-0098-U01.
float tCubic1D::GetddVal(float t)const{return (Coeff[0]*t*6.f+Coeff[1])+Coeff[1];}
// Debug001D7490, E-0098-U01.
float tCubic1D::GetDerivative(float t)const{
    if(duration==0)hp2::not_implemented("0x001D7490","original zero-duration derivative exceptional domain");
    const float reciprocal=1.f/duration;return GetdVal(t*reciprocal)*reciprocal;
}
// Debug001D74C8, E-0098-U01.
float tCubic1D::GetSecondDerivative(float t)const{
    if(duration==0)hp2::not_implemented("0x001D74C8","original zero-duration second derivative exceptional domain");
    const float reciprocal=1.f/duration;return (GetddVal(t*reciprocal)*reciprocal)*reciprocal;
}
// Debug001D7508, E-0098-U01. Uses existing coefficients BEFORE MakeCoeffs.
void tCubic1D::ClampDerivative(float magnitude){
    const float derivative=GetDerivative(duration);
    if(magnitude<std::abs(derivative))dValDesired=((std::abs(derivative)/derivative)*magnitude)*duration;
}
// Debug001D7568, E-0098-U01. Original00325B38 bits3E2AAAAB (float32 one sixth).
void tCubic1D::ClampSecondDerivative(float magnitude){
    float first=GetSecondDerivative(0),last=GetSecondDerivative(duration);
    const float a=std::abs(first),b=std::abs(last);
    if(magnitude<a)first=(a/first)*magnitude;
    if(magnitude<b)last=(b/last)*magnitude;
    if(magnitude<b || magnitude<a){
        const float squared=duration*duration;first=first*squared;
        Coeff[1]=first*.5f;Coeff[0]=(last*squared-first)*0x1.555556p-3f;
    }
}
// Debug001D7B00, E-0098-U01. Preserve flag2 rebuild and threshold<1 behavior.
void cPoint::SplineSeek(tCubic1D& point,float elapsed,float d_clamp,float dd_clamp){
    if(point.state!=1){
        if(point.state!=2)return;
        point.time=0;if(point.flags==0)point.state=1;
        if(0<d_clamp)point.ClampDerivative(d_clamp);
        point.MakeCoeffs();
        // Original deliberately passes d_clamp rather than dd_clamp here.
        if(0<dd_clamp)point.ClampSecondDerivative(d_clamp);
    }
    if(point.duration==0)hp2::not_implemented("0x001D7B00","original zero-duration spline exceptional domain");
    point.time=point.time+elapsed/point.duration;
    if(point.arrivalThreshold<=point.time){
        if(1.f<=point.arrivalThreshold){point.Val=point.ValDesired;point.dVal=point.dValDesired;}
        point.state=0;
    }else{point.Val=point.GetVal(point.time);point.dVal=point.GetdVal(point.time);}
}
}

namespace hp2::recovered {
// ELF 0x001DB3D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001db3d0.md.
void cPoint::PointVelocity() {
    hp2::not_implemented("0x001DB3D0", "cPoint::PointVelocity (signature and full body unresolved)");
}

// ELF 0x001DB810; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001db810.md.
void cPoint::SetPosX() {
    hp2::not_implemented("0x001DB810", "cPoint::SetPosX (signature and full body unresolved)");
}

// ELF 0x001DB870; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001db870.md.
void cPoint::SetPosZ() {
    hp2::not_implemented("0x001DB870", "cPoint::SetPosZ (signature and full body unresolved)");
}

// ELF 0x001DB940; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001db940.md.
void cPoint::SetPosDesiredX() {
    hp2::not_implemented("0x001DB940", "cPoint::SetPosDesiredX (signature and full body unresolved)");
}

// ELF 0x001DB9A0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001db9a0.md.
void cPoint::SetPosDesiredZ() {
    hp2::not_implemented("0x001DB9A0", "cPoint::SetPosDesiredZ (signature and full body unresolved)");
}

// ELF 0x001DBAB8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbab8.md.
void cPoint::SetdPosZ() {
    hp2::not_implemented("0x001DBAB8", "cPoint::SetdPosZ (signature and full body unresolved)");
}

// ELF 0x001DBC28; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbc28.md.
void cPoint::SetRotX() {
    hp2::not_implemented("0x001DBC28", "cPoint::SetRotX (signature and full body unresolved)");
}

// ELF 0x001DBC50; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbc50.md.
void cPoint::SetRotY() {
    hp2::not_implemented("0x001DBC50", "cPoint::SetRotY (signature and full body unresolved)");
}

// ELF 0x001DBC78; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbc78.md.
void cPoint::SetRotZ() {
    hp2::not_implemented("0x001DBC78", "cPoint::SetRotZ (signature and full body unresolved)");
}

// ELF 0x001DBD38; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbd38.md.
void cPoint::SetRotDesiredX() {
    hp2::not_implemented("0x001DBD38", "cPoint::SetRotDesiredX (signature and full body unresolved)");
}

// ELF 0x001DBD60; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbd60.md.
void cPoint::SetRotDesiredY() {
    hp2::not_implemented("0x001DBD60", "cPoint::SetRotDesiredY (signature and full body unresolved)");
}

// ELF 0x001DBD88; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbd88.md.
void cPoint::SetRotDesiredZ() {
    hp2::not_implemented("0x001DBD88", "cPoint::SetRotDesiredZ (signature and full body unresolved)");
}

// ELF 0x001DBE48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbe48.md.
void cPoint::SetdRotX() {
    hp2::not_implemented("0x001DBE48", "cPoint::SetdRotX (signature and full body unresolved)");
}

// ELF 0x001DBE70; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbe70.md.
void cPoint::SetdRotY() {
    hp2::not_implemented("0x001DBE70", "cPoint::SetdRotY (signature and full body unresolved)");
}

// ELF 0x001DBE98; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbe98.md.
void cPoint::SetdRotZ() {
    hp2::not_implemented("0x001DBE98", "cPoint::SetdRotZ (signature and full body unresolved)");
}

// ELF 0x001DBF70; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbf70.md.
void cPoint::SetScale() {
    hp2::not_implemented("0x001DBF70", "cPoint::SetScale (signature and full body unresolved)");
}

// ELF 0x001DBF98; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbf98.md.
void cPoint::SetdScale() {
    hp2::not_implemented("0x001DBF98", "cPoint::SetdScale (signature and full body unresolved)");
}

// ELF 0x001DBFC0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbfc0.md.
void cPoint::SetYScale() {
    hp2::not_implemented("0x001DBFC0", "cPoint::SetYScale (signature and full body unresolved)");
}

// ELF 0x001DBFE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dbfe8.md.
void cPoint::SetdYScale() {
    hp2::not_implemented("0x001DBFE8", "cPoint::SetdYScale (signature and full body unresolved)");
}

// ELF 0x001DC010; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dc010.md.
void cPoint::SetScaleDesired() {
    hp2::not_implemented("0x001DC010", "cPoint::SetScaleDesired (signature and full body unresolved)");
}

// ELF 0x001DC040; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dc040.md.
void cPoint::SetYScaleDesired() {
    hp2::not_implemented("0x001DC040", "cPoint::SetYScaleDesired (signature and full body unresolved)");
}

// ELF 0x001DC070; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dc070.md.
void cPoint::SetDuration() {
    hp2::not_implemented("0x001DC070", "cPoint::SetDuration (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001DA780; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001da780.md.
void tCubic1D::MakeCoeffs() {
    hp2::not_implemented("0x001DA780", "tCubic1D::MakeCoeffs (signature and full body unresolved)");
}

// ELF 0x001DA7D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001da7d0.md.
void tCubic1D::GetVal() {
    hp2::not_implemented("0x001DA7D0", "tCubic1D::GetVal (signature and full body unresolved)");
}

// ELF 0x001DA800; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001da800.md.
void tCubic1D::GetdVal() {
    hp2::not_implemented("0x001DA800", "tCubic1D::GetdVal (signature and full body unresolved)");
}

// ELF 0x001DA830; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001da830.md.
void tCubic1D::GetddVal() {
    hp2::not_implemented("0x001DA830", "tCubic1D::GetddVal (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001DAA00; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001daa00.md.
void tCubic2D::SetVal() {
    hp2::not_implemented("0x001DAA00", "tCubic2D::SetVal (signature and full body unresolved)");
}

// ELF 0x001DAA50; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001daa50.md.
void tCubic2D::SetdVal() {
    hp2::not_implemented("0x001DAA50", "tCubic2D::SetdVal (signature and full body unresolved)");
}

// ELF 0x001DAAA0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001daaa0.md.
void tCubic2D::SetValDesired() {
    hp2::not_implemented("0x001DAAA0", "tCubic2D::SetValDesired (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001DABF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dabf8.md.
void tCubic3D::SetVal() {
    hp2::not_implemented("0x001DABF8", "tCubic3D::SetVal (signature and full body unresolved)");
}

// ELF 0x001DAC68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dac68.md.
void tCubic3D::SetdVal() {
    hp2::not_implemented("0x001DAC68", "tCubic3D::SetdVal (signature and full body unresolved)");
}

// ELF 0x001DACD8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001dacd8.md.
void tCubic3D::SetValDesired() {
    hp2::not_implemented("0x001DACD8", "tCubic3D::SetValDesired (signature and full body unresolved)");
}
} // namespace hp2::recovered
