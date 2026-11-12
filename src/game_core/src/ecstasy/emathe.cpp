
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/src/ecstasy/emathe.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include <cmath>
namespace hp2::debug {
// Debug0011C2E8, E-0099-W50. Original _GLOBAL_$I$eMathIdentityMatrix writes
// only eUnitVector4. CRT001000D0 traverses the captured ctor list forward, so
// this occurs AFTER original Camera/eView construction001148A0.
void Sub_0011C2E8(::port::driving::Memory& m){m.scalar(0x2d39d0,0);m.scalar(0x2d39dc,1);m.scalar(0x2d39d8,0);m.scalar(0x2d39d4,0);}
// Debug0011B558, E-0099-W50. Actual original matrix stores, not a producer-local
// replacement for missing global math startup.
void eIdentity(::port::driving::Memory& m,std::uint32_t matrix){m.scalar(matrix+4,0);m.scalar(matrix+60,1);m.scalar(matrix,1);for(unsigned offset:{8u,12u,16u})m.scalar(matrix+offset,0);m.scalar(matrix+20,1);for(unsigned offset:{24u,28u,32u,36u})m.scalar(matrix+offset,0);m.scalar(matrix+40,1);for(unsigned offset:{44u,48u,52u,56u})m.scalar(matrix+offset,0);}
// Debug0011B4E0, E-0099-W50. Exact separate original engine math initialization.
void eMathInit(::port::driving::Memory& m){eIdentity(m,0x2d3950);for(unsigned offset=0;offset<64;++offset)m.byte(0x2d3990+offset,0);}
// Debug0011B668, E-0099-W50. Preloaded operands permit destination aliasing;
// retain original ordered four multiply/add terms and all output lanes.
void eMulVector(std::array<float,4>& destination,const std::array<float,16>& matrix,const std::array<float,4>& vector){const auto input=vector;std::array<float,4> result{};for(unsigned lane=0;lane<4;++lane){float value=matrix[lane]*input[0];value+=matrix[4+lane]*input[1];value+=matrix[8+lane]*input[2];value+=matrix[12+lane]*input[3];result[lane]=value;}destination=result;}
// Debug0011B6A8, E-0099-W50. Actual static input W, not caller vectorW or an
// invented always-zero/always-one homogeneous default.
void eMulVector(::port::driving::Memory& m,std::array<float,4>& destination,const std::array<float,16>& matrix,const std::array<float,4>& vector){auto operand=m.vector(0x2d39d0);for(unsigned lane=0;lane<3;++lane)operand[lane]+=vector[lane];eMulVector(destination,matrix,operand);}
// Debug0011BEA0, E-0099-W50. Exact transpose, including homogeneous lanes.
void eTransposeMatrix(std::array<float,16>& destination,const std::array<float,16>& matrix){const auto input=matrix;for(unsigned row=0;row<4;++row)for(unsigned column=0;column<4;++column)destination[row*4+column]=input[column*4+row];}
// Debug0011BF28, E-0099-W50. Original rigid-transformation inverse: affine
// rotation transpose followed by the actual negative-translation matrix multiply.
void eInvertTransformationMatrix(std::array<float,16>& destination,const std::array<float,16>& matrix){const auto input=matrix;auto rotation=hp2::recovered::bIdentity(),translation=hp2::recovered::bIdentity();for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column)rotation[row*4+column]=input[column*4+row];for(unsigned axis=0;axis<3;++axis)translation[12+axis]=-input[12+axis];hp2::recovered::eMulMatrix(destination,translation,rotation);}
// Debug0011B8C8, E-0099-W50. Original right/down/forward view, including cross
// order before separate right/vertical normalization and final translation.
void eCreateLookAtMatrix(std::array<float,16>& destination,const std::array<float,4>& eye,const std::array<float,4>& center,const std::array<float,4>& up){
 const auto normalize=[](std::array<float,3> value){const float squared=(value[0]*value[0]+value[1]*value[1])+value[2]*value[2];if(!(squared>0)||!std::isfinite(squared))hp2::not_implemented("0x0011B8C8","exceptional original look-at VU reciprocal square root");const float inverse=1/std::sqrt(squared);for(auto& lane:value)lane*=inverse;return value;};
 const auto cross=[](const std::array<float,3>& first,const std::array<float,3>& second){return std::array<float,3>{{first[1]*second[2]-first[2]*second[1],first[2]*second[0]-first[0]*second[2],first[0]*second[1]-first[1]*second[0]}};};
 const auto forward=normalize({{center[0]-eye[0],center[1]-eye[1],center[2]-eye[2]}});auto right=cross({{-up[0],-up[1],-up[2]}},forward);auto vertical=cross(forward,right);right=normalize(right);vertical=normalize(vertical);auto basis=hp2::recovered::bIdentity(),translation=hp2::recovered::bIdentity();for(unsigned axis=0;axis<3;++axis){basis[axis*4]=right[axis];basis[axis*4+1]=vertical[axis];basis[axis*4+2]=forward[axis];translation[12+axis]=-eye[axis];}hp2::recovered::eMulMatrix(destination,translation,basis);
}
}
namespace hp2::recovered {
using namespace port::driving;
using namespace port::driving::native;
using namespace port::driving::init;
// Debug0011BDD8, E-0098-C01. Row-major post-translation; retain full multiply order.
void eTranslate(SolverMatrix& destination,const SolverMatrix& input,const std::array<float,3>& v){
    const SolverMatrix translation{{1,0,0,0,0,1,0,0,0,0,1,0,v[0],v[1],v[2],1}};
    hp2::recovered::eMulMatrix(destination,input,translation);
}
}

// Retail 0x0011BD90; E-0041-M01; E-0042-S001; exact Debug TU src\ecstasy\emathe.cpp
// Debug 0x0011C108; E-0044-M01; eOrthonormalizeMatrix(bMatrix4 *, bMatrix4 *)
namespace hp2::recovered {
void eOrthonormalizeMatrix(Memory& m,std::uint32_t a) {
    // E-0110-F19. Exact original in-place caller binding (a0==a1 at21B730).
    // Preserve VMUL/ADD/VRSQRT/VOPMULA/VOPMSUB order; do not use a stable
    // host hypot or move normalization ahead of original ComputeAuxiliaries.
    using namespace ::port::driving;
    const auto normalize=[](SolverVector v,const char* square_pc,const char* add_y_pc,const char* add_z_pc,const char* rsqrt_pc,const char* scale_pc){
        const float x=ee_mul_s_finite(v[0],v[0],square_pc),y=ee_mul_s_finite(v[1],v[1],square_pc),z=ee_mul_s_finite(v[2],v[2],square_pc);
        const float square=ee_vadd_finite(ee_vadd_finite(x,y,add_y_pc),z,add_z_pc);float factor;
        if(square==0){if(!pc_sqrt_carrier_enabled())hp2::not_implemented(rsqrt_pc,"Original zero eOrthonormalize VRSQRT requires explicit finite PC carrier for Sony divide-by-zero MAX result");
            ::port::diagnostics::warn(rsqrt_pc,"Original VRSQRT denominator is zero; Sony returns signed finite MAX with divide status","using explicit finite PC maximum7F7FFFFF instead of EE/VU maximum7FFFFFFF; no epsilon or replacement basis");factor=ee_vrsqrt_one_zero(square,rsqrt_pc);
        }else factor=1.f/std::sqrt(square);
        for(unsigned lane=0;lane<3;++lane)v[lane]=ee_mul_s_finite(v[lane],factor,scale_pc);
        // Original VMUL.xyzw computes W but no later XYZ operation reads it;
        // source stores clear every final affine W after both cross products.
        v[3]=v[3]*factor;return v;
    };
    const auto cross=[](const SolverVector& left,const SolverVector& right,const char* mula_pc,const char* msub_pc){SolverVector out;for(unsigned lane=0;lane<3;++lane){const unsigned j=(lane+1)%3,k=(lane+2)%3;auto acc=ee_vmula_finite(left[j],right[k],mula_pc);acc=ee_vmsuba_finite(acc,right[j],left[k],msub_pc);out[lane]=acc.value;}out[3]=left[3];return out;};
    auto x=m.vector(a),y=m.vector(a+0x10);
    x=normalize(x,"0x0011C110","0x0011C118","0x0011C124","0x0011C12C","0x0011C140");
    auto z=cross(x,y,"0x0011C14C","0x0011C150");
    z=normalize(z,"0x0011C15C","0x0011C160","0x0011C164","0x0011C168","0x0011C17C");
    y=cross(z,x,"0x0011C18C","0x0011C190");
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
