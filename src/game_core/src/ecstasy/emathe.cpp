
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x0011BD90; E-0041-M01; E-0042-S001; exact Debug TU src\ecstasy\emathe.cpp
namespace port::driving::native {
void eOrthonormalizeMatrix(Memory& m,std::uint32_t a){
    using namespace detail_group_00138e08;
    auto x=m.vector(a),y=m.vector(a+0x10);x=mul(x,1.0f/len(x));auto z=cross(x,y);z=mul(z,1.0f/len(z));y=cross(z,x);
    x[3]=y[3]=z[3]=0;m.vector(a,x);m.vector(a+0x10,y);m.vector(a+0x20,z);m.vector(a+0x30,{{0,0,0,1}});
}
}

// Retail 0x0011B248; E-0041-M01; E-0042-S001; exact Debug TU src\ecstasy\emathe.cpp
namespace port::driving {
void eMulMatrix(SolverMatrix&out,const SolverMatrix&a,const SolverMatrix&b){
    using namespace detail_group_001a4930;
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
