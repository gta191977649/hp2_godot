
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001ADFB8; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\aerodynamics.cpp
namespace port::driving::native {
float GetDownforceAtSpeed(Memory& m,std::uint32_t object,float f12){
    using namespace detail_group_00137100_runtime;
    const auto config=m.word(object);return std::max(0.0f,f12)*(m.scalar(config+4)+m.scalar(config+8));
}
}

// Retail 0x001ADD70; E-0042-C131; E-0042-S001; exact Debug TU ..\indep\src\aerodynamics.cpp
namespace port::driving::native {
void Sub_001ADD70(Memory& m,std::uint32_t a,std::uint32_t output){
    using namespace detail_group_00138e08;
    const auto body=m.word(a+4),config=m.word(a);const auto velocity=m.vector(body+0x90);
    const float speed=len(velocity)/m.scalar(a+8);float value=speed*m.scalar(a+0xc);value=value*m.scalar(config);
    float extra=value*(g(m,0xb568)-1.0f);extra=extra*(1.0f-m.scalar(a+0x10));value=-(value+extra);
    Vector point{{0,0,0,0}};if(0.5f<=m.scalar(a+0x14))point[2]=g(m,0xb56c)*(1.0f-m.scalar(a+0x10));
    ApplyForce(m,body,mul(velocity,value),point);
    float f=m.scalar(body+0x48);if(f<0)f=0;if(0.5f<=m.scalar(a+0x14))f=1;
    Vector direction{{1,0,0,0}};if(g(m,0x8e2c)<speed)direction=mul(m.vector(body+0x90),1.0f/speed);
    float projection=dot(direction,m.vector(body+0x20));if(projection<0)projection=0;
    projection=projection*projection;projection=projection*projection;
    float front,rear;
    if(m.scalar(a+0x14)==0){projection=projection*f;const float sum=m.scalar(config+4)+m.scalar(config+8);front=speed*g(m,0x8e30);front=front*projection;front=front*sum;rear=speed*g(m,0x8e34);rear=rear*projection;rear=rear*sum;}
    else{projection=projection*f;front=speed*m.scalar(config+4);front=front*projection;rear=speed*m.scalar(config+8);rear=rear*projection;}
    ApplyForceInFrame(m,body,{{0,0,-front,0}},m.vector(0x003115b0));ApplyForceInFrame(m,body,{{0,0,-rear,0}},m.vector(0x003115c0));
    if(output)m.scalar(output,front+rear);
}
}
