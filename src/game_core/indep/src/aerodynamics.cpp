// TASK-0044 E-0044-E01, full Debug001AB3F0/001AB638 listings.
#include "game_core/indep/src/aerodynamics.hpp"
#include "game_core/port/driving/projection_support.hpp"
namespace hp2::recovered {
void Aerodynamics::ResolveForces(std::uint32_t output) {
    using namespace port::driving::native;
    using namespace port::driving::native::detail_group_00138e08;
    auto& m=m_;const auto a=address_;
    const auto body=m.word(a+4),config=m.word(a);const auto velocity=m.vector(body+0x90);
    const float speed=len(velocity)/m.scalar(a+8);float value=speed*m.scalar(a+0xc);value=value*m.scalar(config);
    float extra=value*(m.scalar(0x327ed0)-1.0f);extra=extra*(1.0f-m.scalar(a+0x10));value=-(value+extra);
    Vector point{{0,0,0,0}};if(0.5f<=m.scalar(a+0x14))point[2]=m.scalar(0x327ed4)*(1.0f-m.scalar(a+0x10));
    ApplyForce(m,body,mul(velocity,value),point);
    float f=m.scalar(body+0x48);if(f<0)f=0;if(0.5f<=m.scalar(a+0x14))f=1;
    Vector direction{{1,0,0,0}};if(m.scalar(0x3257e0)<speed)direction=mul(m.vector(body+0x90),1.0f/speed);
    float projection=dot(direction,m.vector(body+0x20));if(projection<0)projection=0;
    projection=projection*projection;projection=projection*projection;
    float front,rear;
    if(m.scalar(a+0x14)==0){projection=projection*f;const float sum=m.scalar(config+4)+m.scalar(config+8);front=speed*m.scalar(0x3257e4);front=front*projection;front=front*sum;rear=speed*m.scalar(0x3257e8);rear=rear*projection;rear=rear*sum;}
    else{projection=projection*f;front=speed*m.scalar(config+4);front=front*projection;rear=speed*m.scalar(config+8);rear=rear*projection;}
    ApplyForceInFrame(m,body,{{0,0,-front,0}},m.vector(0x00308cc0));ApplyForceInFrame(m,body,{{0,0,-rear,0}},m.vector(0x00308cd0));
    if(output)m.scalar(output,front+rear);
}
float Aerodynamics::GetDownforceAtSpeed(float speed) {
 const auto config=m_.word(address_);return std::max(0.0f,speed)*(m_.scalar(config+4)+m_.scalar(config+8));
}
void Aerodynamics::DoSnapshot(){hp2::not_implemented("0x001AB3C8","replay snapshot outside driving scope");}
}
namespace port::driving::native {
float GetDownforceAtSpeed(Memory&m,std::uint32_t a,float speed){return hp2::recovered::Aerodynamics(m,a).GetDownforceAtSpeed(speed);}
void Aerodynamics::ResolveForces(Memory&m,std::uint32_t a,std::uint32_t out){hp2::recovered::Aerodynamics(m,a).ResolveForces(out);}
}
