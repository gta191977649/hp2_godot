
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
#include "game_core/src/bMath.hpp"
namespace hp2::recovered {
using namespace port::driving;
using namespace port::driving::native;
using namespace port::driving::init;
}

// Retail 0x0011BD90; E-0041-M01; E-0042-S001; exact Debug TU src\ecstasy\emathe.cpp
// Debug 0x0011C108; E-0044-M01; eOrthonormalizeMatrix(bMatrix4 *, bMatrix4 *)
namespace hp2::recovered {
void eOrthonormalizeMatrix(Memory& m,std::uint32_t a) {
    using namespace port::driving::native::detail_group_00138e08;
    auto x=m.vector(a),y=m.vector(a+0x10);x=mul(x,1.0f/len(x));auto z=cross(x,y);z=mul(z,1.0f/len(z));y=cross(z,x);
    x[3]=y[3]=z[3]=0;m.vector(a,x);m.vector(a+0x10,y);m.vector(a+0x20,z);m.vector(a+0x30,{{0,0,0,1}});
}
}

// Retail 0x0011B248; E-0041-M01; E-0042-S001; exact Debug TU src\ecstasy\emathe.cpp
// Debug 0x0011B5F0; E-0044-M01; eMulMatrix(bMatrix4 *, bMatrix4 *, bMatrix4 *)
namespace hp2::recovered {
void eMulMatrix(SolverMatrix&out,const SolverMatrix&a,const SolverMatrix&b) {
    using namespace port::driving::detail_group_001a4930;
    const auto first=a,second=b;SolverMatrix result{};
    for(unsigned row=0;row<4;++row)for(unsigned lane=0;lane<4;++lane){
        float acc=second[lane]*first[row*4];
        float product=second[4+lane]*first[row*4+1];acc=acc+product;
        product=second[8+lane]*first[row*4+2];acc=acc+product;
        product=second[12+lane]*first[row*4+3];result[row*4+lane]=acc+product;
    }
    out=result;
}
}

// TASK-0044 historical projection compatibility: forwarding only.
namespace port::driving::native {
void eOrthonormalizeMatrix(Memory& m,std::uint32_t a) { return hp2::recovered::eOrthonormalizeMatrix(m,a); }
}
namespace port::driving {
void eMulMatrix(SolverMatrix&out,const SolverMatrix&a,const SolverMatrix&b) { return hp2::recovered::eMulMatrix(out,a,b); }
}

#include "game_core/src/bMath.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/table.hpp"

// Debug 0x0011BA88; E-0044-M02; source ownership E-0042-S001.
namespace hp2::recovered {
float eSin(Memory&m,float f12) {
    using namespace port::driving::detail_group_001a4930;
    const float upper=m.scalar(0x00324b1c),step=m.scalar(0x00324b20);
    if(!(0<upper) || !(0<step) || !std::isfinite(f12))
        hp2::not_implemented("0x0011BA88","unsupported polynomial range reduction");
    while(upper<f12)f12=f12-upper;
    while(f12<0)f12=f12+step;
    const float bound=m.scalar(0x00324b24);float sign=1;
    if(bound<=f12){f12=f12-bound;sign=-1;}
    if(m.scalar(0x00324b28)<=f12)f12=bound-f12;
    float f1=f12*f12;float f0=f12*f1;float f2=f0*f1;
    f0=f0*m.scalar(0x00324b2c);float f3=f2*f1;f0=f12+f0;
    f2=f2*m.scalar(0x00324b30);f1=f3*f1;f3=f3*m.scalar(0x00324b34);
    f0=f0+f2;f1=f1*m.scalar(0x00324b38);f0=f0+f3;f0=f0+f1;return f0*sign;
}
}

namespace hp2::recovered {
// Debug 0x0011BD10; E-0044-M02. Full original eRotateZ including matrix multiply.
// Debug0x0011BB80; E-0048-D03; original emathe.cpp, host matrix binding.
void eRotateX(Memory& m,SolverMatrix& destination,const SolverMatrix& input,std::uint16_t angle){
    const float radians=static_cast<float>(angle)*m.scalar(0x00324b3c);
    const float cosine=eSin(m,radians+m.scalar(0x00324b40)),sine=eSin(m,radians);
    const SolverMatrix rotation{{1,0,0,0,0,cosine,sine,0,0,-sine,cosine,0,0,0,0,1}};
    hp2::recovered::eMulMatrix(destination,input,rotation);
}
void eRotateZ(Memory& m,SolverMatrix& destination,const SolverMatrix& input,std::uint16_t angle) {
    const float radians=static_cast<float>(angle)*m.scalar(0x00324b4c);
    const float cosine=eSin(m,radians+m.scalar(0x00324b50)),sine=eSin(m,radians);
    const SolverMatrix rotation{{cosine,sine,0,0,-sine,cosine,0,0,0,0,1,0,0,0,0,1}};
    hp2::recovered::eMulMatrix(destination,input,rotation);
}
// Debug 0x0011BC48; E-0044-M02. Full original eRotateY including matrix multiply.
void eRotateY(Memory& m,SolverMatrix& destination,const SolverMatrix& input,std::uint16_t angle) {
    const float radians=static_cast<float>(angle)*m.scalar(0x00324b44);
    const float cosine=eSin(m,radians+m.scalar(0x00324b48)),sine=eSin(m,radians);
    const SolverMatrix rotation{{cosine,0,-sine,0,0,1,0,0,sine,0,cosine,0,0,0,0,1}};
    hp2::recovered::eMulMatrix(destination,input,rotation);
}
}
