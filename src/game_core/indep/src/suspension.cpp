#include "game_core/indep/src/suspension.hpp"
#include "game_core/indep/src/wheel.hpp"
#include "game_core/indep/src/rigidbody.hpp"

// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001A5EA0; E-0042-C125; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace hp2::recovered {
void Suspension::port_initialize(port::driving::init::ByteView out,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t t0,std::uint32_t t1){using namespace port::driving;using namespace port::driving::init;
    out.require(0,0xa0);
    out.word(0x9c,a1);out.word(0x94,a2);out.word(8,0x002ada90);out.word(0x90,a3);
    out.word(0x98,t0);out.word(0xc,t1);out.word(0x10,0);
}
}
namespace port::driving::init {
void Sub_001A5EA0(ByteView out,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t t0,std::uint32_t t1){hp2::recovered::Suspension::port_initialize(out,a1,a2,a3,t0,t1);}

}

// Retail 0x001A5F88; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace hp2::recovered {
void Suspension::Calibrate(port::driving::init::ByteView out,float f12,const std::array<float,4>& a,const std::array<float,4>& b){using namespace port::driving;using namespace port::driving::init;
    out.require(0x78,0x18);out.scalar(0x78,f12);
    for(std::size_t i=0;i<4;++i)out.scalar(0x80+4*i,a[i]-b[i]);
}
}
namespace port::driving::init {
void Calibrate(ByteView out,float f12,const std::array<float,4>& a,const std::array<float,4>& b){hp2::recovered::Suspension::Calibrate(out,f12,a,b);}

}

// Retail 0x001A63D0; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace hp2::recovered {
std::array<float,4> GetPointVelocity(const std::array<float,4>& linear,const std::array<float,4>& angular,const std::array<float,4>& point){using namespace port::driving;using namespace port::driving::native;
    using namespace detail_group_00137100_runtime;
    auto result=cross(angular,point);for(unsigned i=0;i<4;++i)result[i]=linear[i]+result[i];return result;
}
}
namespace port::driving::native {
std::array<float,4> GetPointVelocity(const std::array<float,4>& linear,const std::array<float,4>& angular,const std::array<float,4>& point){return hp2::recovered::GetPointVelocity(linear,angular,point);}

}

// Retail 0x001A6408; E-0042-C130; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace hp2::recovered {
float Suspension::ApplyGroundPenaltyImpulse(){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto object=address_;
    using namespace detail_group_00137100_runtime;
    const auto body=m.word(object+0x94),wheel=m.word(object+0x90);
    const auto point=transform(m,m.matrix(body+0x20),m.vector(object+0x20));
    const auto velocity=hp2::recovered::GetPointVelocity(m.vector(body+0x90),m.vector(body+0xa0),point),normal=m.vector(wheel+0x140);
    const float dot=(velocity[0]*normal[0]+velocity[1]*normal[1])+velocity[2]*normal[2];float result=0;
    if(0<=-dot){result=(-dot*global(m,0xb40c))*m.scalar(body+0x118);auto input=normal;for(auto& lane:input)lane=lane*result;
        if(m.word(global_address(0xb420))==0)ApplyImpulse(m,body,input,point);}
    return result;
}
}
namespace port::driving::native {
float ApplyGroundPenaltyImpulse(Memory& m,std::uint32_t object){return hp2::recovered::Suspension(m,object).ApplyGroundPenaltyImpulse();}

}

// Retail 0x001A6330; E-0042-C128; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace hp2::recovered {
void Suspension::ResolveForces(float f12, port::driving::SolverSuspensionProjection& suspension){using namespace port::driving;using namespace port::driving::native;
    using namespace detail_group_00138f38;
    float f0 = suspension.Field_0x1c;
    if (suspension.Field_0x70 != 0) f0 = f0 + f12;
    if (!suspension.Field_0x90) hp2::not_implemented("0x001A6330", "missing +0x90 wheel projection");
    suspension.Field_0xa0 = ExchangeZForces(f0, *suspension.Field_0x90, suspension.Field_0x70);
}
}
namespace port::driving {
void Suspension::ResolveForces(float f12, SolverSuspensionProjection& suspension){hp2::recovered::Suspension::ResolveForces(f12,suspension);}

}

// Retail 0x001A6370; E-0042-C129; E-0042-S001; exact Debug TU ..\indep\src\suspension.cpp
namespace hp2::recovered {
void Suspension::ResolveMoreForces(port::driving::SolverSuspensionProjection& suspension, std::int32_t Field_00330D10){using namespace port::driving;using namespace port::driving::native;
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
namespace port::driving {
void ResolveMoreForces(SolverSuspensionProjection& suspension, std::int32_t Field_00330D10){hp2::recovered::Suspension::ResolveMoreForces(suspension,Field_00330D10);}

}


namespace hp2::recovered {
void Suspension::port_initialize(int position,std::uint32_t body,std::uint32_t wheel,std::uint32_t steering,std::uint32_t info){
 Suspension::port_initialize(m_.view(address_,0xb0),static_cast<std::uint32_t>(position),body,wheel,steering,info);
}
void Suspension::Calibrate(float load){
 const auto a=m_.vector(m_.word(m_.word(address_+0x90)+0xc)),b=m_.vector(m_.word(address_+0x94)+0x160);
 Suspension::Calibrate(m_.view(address_,0xb0),load,a,b);
}
void Suspension::SetState(){SetState(m_.view(address_,0x1c));}
void Suspension::SetState(port::driving::init::ByteView state){state.require(0x14,8);state.word(0x18,0);state.word(0x14,0);}
void Suspension::SetState(std::uint32_t state){m_.copy(address_+0x14,state,8);}
void Suspension::GetState(std::uint32_t state){m_.copy(state,address_+0x14,8);}
std::uint32_t Suspension::GetState(){return address_+0x14;}
int Suspension::GetSize(){return 2;}
void Suspension::ComputeAuxiliaries(port::driving::ContactProvider& contact){
 using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto object=address_;

    using namespace detail_group_00137100_runtime;
    Memory::Scratch scratch(m);
    const auto body=m.word(object+0x94),wheel=m.word(object+0x90),config=m.word(object+0xc);
    auto world=transform(m,m.matrix(body+0x20),m.vector(object+0x80));
    const auto position=m.vector(body+0x10);for(unsigned i=0;i<4;++i)world[i]=world[i]+position[i];
    const auto temporary=m.allocate(16);m.vector(temporary,world);
    const float hit=hp2::recovered::Wheel(m,wheel).FindDrivingSurface(temporary,body+0x20,contact);
    const float value=hit+m.scalar(m.word(wheel+0xc)+0xc),old=m.scalar(object+0x14);
    float extra=0;
    if(m.scalar(config+0x14)<value){extra=value-m.scalar(config+0x14);m.scalar(object+0x14,m.scalar(config+0x14));m.word(object+0x70,1);if(global(m,0xb414)<extra)extra=0;}
    else if(value<m.scalar(config+0x18)){m.scalar(object+0x14,m.scalar(config+0x18));m.word(object+0x70,0);}
    else {m.scalar(object+0x14,value);m.word(object+0x70,1);}
    const float dt=global(m,0xb408);
    if(global(m,0x8dcc)<dt)m.scalar(object+0x18,(m.scalar(object+0x14)-old)/dt);
    m.copy(object+0x20,object+0x80,16);m.scalar(object+0x28,m.scalar(object+0x28)+m.scalar(object+0x14));
    auto matrix=init::bIdentity();const float steer=m.scalar(wheel+0x16c);
    if(steer!=0){auto rotation=init::bIdentity();const float a=(global(m,0xb41c)*steer)*port::driving::bSin(m,m.scalar(wheel+0x1c));
        const float c=port::driving::bSin(m,a+global(m,0x8dd0)),s=port::driving::bSin(m,a);
        rotation[0]=c;rotation[1]=s;rotation[4]=-s;rotation[5]=c;matrix=bMulMatrix(matrix,rotation);}
    const auto local=m.vector(object+0x20);for(unsigned i=0;i<3;++i)matrix[12+i]=local[i];matrix[15]=1;
    m.matrix(object+0x30,matrix);
    if(m.word(object+0x98)){const auto other=m.word(object+0x98);matrix=bMulMatrix(matrix,m.matrix(m.word(other+4)+m.word(object+0x9c)*0x40));}
    m.matrix(wheel+0xb0,matrix);
    const auto relative=transform(m,m.matrix(body+0x20),m.vector(object+0x20));
    const auto velocity=hp2::recovered::GetPointVelocity(m.vector(body+0x90),m.vector(body+0xa0),relative);m.vector(wheel+0xf0,velocity);
    m.matrix(wheel+0x70,bMulMatrix(m.matrix(body+0xb0),matrix));
    if(m.word(object+0x70)){
        const float displacement=m.scalar(object+0x14),rate=m.scalar(object+0x18);
        float response=std::max(displacement,0.0f)*m.scalar(config);response=response+1;response=m.scalar(config+4)*response;
        response=displacement*response;response=m.scalar(object+0x78)+response;
        response=response+rate*m.scalar(config+8+(rate<=0?4:0));m.scalar(object+0x1c,response);
        response=response+m.scalar(config+0x10)*(displacement-m.scalar(m.word(object+0x10)+0x14));
        response=response+extra*global(m,0xb410);m.scalar(object+0x1c,response);
        if(!(response<0))return;
    }
    m.word(object+0x1c,0);
}
std::uint32_t Suspension::GetDerivative(std::uint32_t out){auto& m=m_;
    using namespace port::driving::detail_group_001a4930;m.word(out,0);m.word(out+4,0);return out;}
void Suspension::ResolveForces(float request){
 const int touching=m_.signed_word(address_+0x70);float force=m_.scalar(address_+0x1c);if(touching)force=force+request;
 m_.scalar(address_+0xa0,Wheel(m_,m_.word(address_+0x90)).ExchangeZForces(touching,force));
}
void Suspension::ResolveMoreForces(){
 if(m_.signed_word(address_+0x70)==0)return;
 const auto wheel=m_.word(address_+0x90),body=m_.word(address_+0x94);auto delta=m_.vector(wheel+0x130);const auto pos=m_.vector(body+0x10);
 for(unsigned i=0;i<4;++i)delta[i]=delta[i]-pos[i];
 if(m_.word(0x00327d90)==0)RigidBody(m_,body).ApplyForce(m_.vector(wheel+0x120),delta);
}
}


#include "game_core/src/bMath.hpp"
namespace hp2::recovered {
// Debug001A3F30 full listing E-0044-E01. Constructor does not initialize matrices.
void TwoWheelAckermanSteering::Sub_001A3F30(std::uint32_t config) {
 m_.word(address_,config);m_.word(address_+8,0x2adad8);m_.word(address_+4,address_+0x10);
}
void TwoWheelAckermanSteering::ComputeAuxiliaries() {}
// Debug001A3540, exact float grouping and asymmetric inner/outer wheel assignment.
void TwoWheelAckermanSteering::SetSteering(std::uint16_t value) {
 auto& m=m_;const auto s=address_;
 float angle=static_cast<float>(value)*m.scalar(0x325780);
 const auto config=m.word(s);
 const float wheelbase=m.scalar(config+0x120)-m.scalar(config+0x180);
 const float track=m.scalar(config+0x124)-m.scalar(config+0x144);
 if(m.scalar(0x325784)<angle) angle=angle-m.scalar(0x325788);
 bool positive=true;if(angle<0){angle=-angle;positive=false;}
 float denominator=track*angle;float numerator=wheelbase*angle;
 denominator=wheelbase+denominator;const float inner=numerator/denominator;
 const float left=positive?angle:-inner,right=positive?inner:-angle;
 const float phase=m.scalar(0x32578c);
 auto matrix=bIdentity();float cosine=hp2::recovered::bSin(m,left+phase),sine=hp2::recovered::bSin(m,left);
 matrix[0]=cosine;matrix[1]=sine;matrix[4]=-sine;matrix[5]=cosine;m.matrix(s+0x10,matrix);
 matrix=bIdentity();cosine=hp2::recovered::bSin(m,right+phase);sine=hp2::recovered::bSin(m,right);
 matrix[0]=cosine;matrix[1]=sine;matrix[4]=-sine;matrix[5]=cosine;m.matrix(s+0x50,matrix);
}
}
