
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001A5EA0; E-0042-C125; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace port::driving::init {
void Sub_001A5EA0(ByteView out,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t t0,std::uint32_t t1){
    out.require(0,0xa0);
    out.word(0x9c,a1);out.word(0x94,a2);out.word(8,0x002b5cd0);out.word(0x90,a3);
    out.word(0x98,t0);out.word(0xc,t1);out.word(0x10,0);
}
}

// Retail 0x001A5F88; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace port::driving::init {
void Sub_001A5F88(ByteView out,float f12,const std::array<float,4>& a,const std::array<float,4>& b){
    out.require(0x78,0x18);out.scalar(0x78,f12);
    for(std::size_t i=0;i<4;++i)out.scalar(0x80+4*i,a[i]-b[i]);
}
}

// Retail 0x001A63D0; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace port::driving::native {
std::array<float,4> GetPointVelocity(const std::array<float,4>& linear,const std::array<float,4>& angular,const std::array<float,4>& point){
    using namespace detail_group_00137100_runtime;
    auto result=cross(angular,point);for(unsigned i=0;i<4;++i)result[i]=linear[i]+result[i];return result;
}
}

// Retail 0x001A6408; E-0042-C130; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace port::driving::native {
float ApplyGroundPenaltyImpulse(Memory& m,std::uint32_t object){
    using namespace detail_group_00137100_runtime;
    const auto body=m.word(object+0x94),wheel=m.word(object+0x90);
    const auto point=transform(m,m.matrix(body+0x20),m.vector(object+0x20));
    const auto velocity=GetPointVelocity(m.vector(body+0x90),m.vector(body+0xa0),point),normal=m.vector(wheel+0x140);
    const float dot=(velocity[0]*normal[0]+velocity[1]*normal[1])+velocity[2]*normal[2];float result=0;
    if(0<=-dot){result=(-dot*global(m,0xb40c))*m.scalar(body+0x118);auto input=normal;for(auto& lane:input)lane=lane*result;
        if(m.word(global_address(0xb420))==0)ApplyImpulse(m,body,input,point);}
    return result;
}
}

// Retail 0x001A6330; E-0042-C128; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace port::driving {
void Sub_001A6330(float f12, SolverSuspensionProjection& suspension) {
    using namespace detail_group_00138f38;
    float f0 = suspension.Field_0x1c;
    if (suspension.Field_0x70 != 0) f0 = f0 + f12;
    if (!suspension.Field_0x90) hp2::not_implemented("0x001A6330", "missing +0x90 wheel projection");
    suspension.Field_0xa0 = ExchangeZForces(f0, *suspension.Field_0x90, suspension.Field_0x70);
}
}

// Retail 0x001A6370; E-0042-C129; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace port::driving {
void ResolveMoreForces(SolverSuspensionProjection& suspension, std::int32_t Field_00330D10) {
    using namespace detail_group_00138f38;
    if (suspension.Field_0x70 == 0) return;
    if (!suspension.Field_0x90 || !suspension.Field_0x94)
        hp2::not_implemented("0x001A6370", "missing wheel/body projections");
    SolverVector delta{};
    for (unsigned i = 0; i < 4; ++i)
        delta[i] = suspension.Field_0x90->Field_0x130[i] - suspension.Field_0x94->Field_0x10[i];
    if (Field_00330D10 == 0)
        ApplyForce(*suspension.Field_0x94, suspension.Field_0x90->Field_0x120, delta);
}
}
