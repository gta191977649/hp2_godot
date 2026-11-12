#include "game_core/indep/inc/ComponentPhysics.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include <cmath>
#include <cstring>
#include "game_core/port/race/traffic_construction.hpp"
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x0019C170; E-0042-C089; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace hp2::recovered {
void PhysicsSystem::port_initialize(port::driving::init::ByteView out,std::uint32_t address){using namespace port::driving;using namespace port::driving::init;
    out.require(0,0x10);out.word(0,address);out.word(0xc,0x002ad160);out.word(4,address);out.word(8,0);
}
}
namespace port::driving::init {
void Sub_0019C170(ByteView out,std::uint32_t address){hp2::recovered::PhysicsSystem::port_initialize(out,address);}

}

// Retail 0x0019C238; E-0042-C090; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace hp2::recovered {
void PhysicsSystem::AddComponent(std::uint32_t object){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto head=address_;
    using namespace detail_group_00137100_runtime;
    const auto last=m.word(head+4),table=m.word(object+8);
    m.word(last,object);m.word(head+4,object);m.word(object+4,last);m.word(object,head);
    const auto adjusted=object+static_cast<std::int32_t>(m.signed_half(table+0x10));
    (void)adjusted; // These exact constant-return implementations do not read this.
    const auto count=port::driving::native::detail_group_0019d4d0::count(m,object);
    m.word(head+8,m.word(head+8)+count);
}
}
namespace port::driving::native {
void AddComponent(Memory& m,std::uint32_t head,std::uint32_t object){hp2::recovered::PhysicsSystem(m,head).AddComponent(object);}

}

// Retail 0x0019C808; E-0041-V01; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace hp2::recovered {
void PhysicsSystem::ComputeAuxiliaries(port::driving::native::InitServices& services){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto head=address_;
    using namespace detail_group_00137100_runtime;
    if(!services.Sub_00250F38)hp2::not_implemented("0x0024A340","Original performance-counter service is absent");
    const auto start=services.Sub_00250F38();auto object=m.word(head);
    while(object!=head){
        const auto table=m.word(object+8);
        const auto adjusted=object+static_cast<std::int32_t>(m.signed_half(table+0x40));
        if(m.word(table+0x44)==0x2154d8)hp2::debug::Mellovator(m,adjusted).ComputeAuxiliaries();
        else reset_dispatch(m,m.word(table+0x44),adjusted,services);
        object=m.word(object);
    }
    const auto end=services.Sub_00250F38();
    if(!services.Sub_00250F48)hp2::not_implemented("0x0024A350","Original performance-counter delta service is absent");
    m.word(global_address(0xa780),services.Sub_00250F48(start,end));
}
}
namespace port::driving::native {
void PhysicsSystem::ComputeAuxiliaries(Memory& m,std::uint32_t head,InitServices& services){hp2::recovered::PhysicsSystem(m,head).ComputeAuxiliaries(services);}

}

// Retail 0x0019C670; E-0042-C092; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace hp2::recovered {
void PhysicsSystem::GetDerivative(std::uint32_t output,const port::driving::native::IntegrationCalls& calls){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto head=address_;
    using namespace detail_group_0019d4d0;
    if(!calls.derivative)hp2::not_implemented("0x00199F98","component derivative dispatch");
    std::uint32_t i=0;
    for(auto node=m.word(head);node!=head;node=m.word(node)) {
        calls.derivative(m,node,output+i*4);i+=count(m,node);
    }
}
}
namespace port::driving::native {
void PhysicsSystem::GetDerivative(Memory& m,std::uint32_t head,std::uint32_t output,const IntegrationCalls& calls){hp2::recovered::PhysicsSystem(m,head).GetDerivative(output,calls);}

}

// Retail 0x0019C290; E-0042-C091; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace hp2::recovered {
void PhysicsSystem::IntegrateEuler(float dt,const port::driving::native::IntegrationCalls& calls){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto head=address_;
    using namespace detail_group_0019d4d0;
    // Original bounded stack is140 floats. Allocate a reusable port scratch region.
    std::uint32_t total=0;
    for(auto node=m.word(head);node!=head;node=m.word(node)) {
        const auto n=count(m,node);
        if(n>140-total)throw std::out_of_range("Original derivative stack capacity exceeded");
        total+=n;
    }
    Memory::Scratch frame(m);const auto scratch=m.allocate(140*4);hp2::recovered::PhysicsSystem(m,head).GetDerivative(scratch,calls);std::uint32_t i=0;
    for(auto node=m.word(head);node!=head;node=m.word(node)) {
        const auto n=count(m,node);if(n>140-i)throw std::out_of_range("Original derivative stack capacity exceeded");
        const auto table=m.word(node+8),target=m.word(table+0x1c);
        const auto op=m.word(target),delay=m.word(target+4);
        if(op!=0x03e00008u || (delay&0xffff0000u)!=0x24820000u)
            hp2::not_implemented("0x00199BB8","component state pointer virtual target");
        const auto state=node+static_cast<std::int16_t>(m.half(table+0x18))+static_cast<std::int16_t>(delay&0xffffu);
        for(std::uint32_t j=0;j<n;++j){const float derivative=m.scalar(scratch+(i+j)*4),old=m.scalar(state+j*4);if(!std::isfinite(dt)||!std::isfinite(old)||!std::isfinite(derivative)){const auto bits=[](float x){std::uint32_t word;std::memcpy(&word,&x,4);return std::to_string(word);};const std::string why="Euler input producer component="+std::to_string(node)+" state="+std::to_string(state)+" index="+std::to_string(j)+" old_bits="+bits(old)+" derivative_bits="+bits(derivative)+" timestep_bits="+bits(dt);hp2::not_implemented("0x00199C50",why.c_str());}const float product=::port::driving::ee_mul_s_finite(dt,derivative,"0x00199C50"),result=::port::driving::ee_add_s_finite(old,product,"0x00199C58");if(!std::isfinite(old)||!std::isfinite(derivative)||!std::isfinite(product)||!std::isfinite(result)){const auto bits=[](float x){std::uint32_t word;std::memcpy(&word,&x,4);return std::to_string(word);};const std::string why="Euler producer component="+std::to_string(node)+" state="+std::to_string(state)+" index="+std::to_string(j)+" old_bits="+bits(old)+" derivative_bits="+bits(derivative)+" timestep_bits="+bits(dt)+" product_bits="+bits(product)+" result_bits="+bits(result);hp2::not_implemented("0x00199BB8",why.c_str());}m.scalar(state+j*4,result);}i+=n;
    }
    if(!calls.after)hp2::not_implemented("0x00199BB8","post integration dispatch");
    calls.after(m,head);
}
}
namespace port::driving::native {
void IntegrateEuler(Memory& m,float dt,std::uint32_t head,const IntegrationCalls& calls){hp2::recovered::PhysicsSystem(m,head).IntegrateEuler(dt,calls);}

}

namespace hp2::recovered {
// Debug00199a98 E-0044-S01; constructor body through explicit host binding.
void PhysicsSystem::port_initialize(){PhysicsSystem::port_initialize(m_.view(address_,0x10),address_);}
// Debug0019a220 /0019a238 E-0044-S01; both JR RA/NOP.
void PhysicsSystem::ResolveForces(){}
void PhysicsComponent::ResolveForces(){}
// Debug0019a230 E-0044-S01; JR RA/NOP.
void PhysicsComponent::ComputeAuxiliaries(){}
}

// TASK-0099 E-0099-W33. Debug00199A70/00199AB8; original componentphysics.cpp.
#include "game_core/indep/src/rigidbody.hpp"
#include "game_core/indep/src/wheel.hpp"
#include "game_core/indep/src/suspension.hpp"
#include "game_core/indep/src/engine.hpp"
namespace hp2::recovered {
void PhysicsComponent::Sub_00199A70(int flags){m_.word(address_+8,0x2ad180);if(flags&1)m_.release(address_);}
void PhysicsSystem::Sub_00199AB8(int flags){
 auto& m=m_;const auto head=address_;m.word(head+0xc,0x2ad160);
 while(m.word(head)!=head){
  const auto node=m.word(head),next=m.word(node),prev=m.word(node+4),table=m.word(node+8);
  m.word(prev,next);m.word(next+4,prev);
  const auto adjusted=node+static_cast<std::int32_t>(m.signed_half(table+8));
  switch(m.word(table+0xc)){
   case 0x19b5f8:RigidBody(m,adjusted).Sub_0019B5F8(3);break;
   case 0x1a3410:Wheel(m,adjusted).Sub_001A3410(3);break;
   case 0x1a3e48:Suspension(m,adjusted).Sub_001A3E48(3);break;
   case 0x188a38:Engine(m,adjusted).Sub_00188A38(3);break;
   case 0x18a2f8:DriveTrain(m,adjusted).Sub_0018A2F8(3);break;
   case 0x216400:hp2::debug::Mellovator(m,adjusted).Sub_00216400(3);break;
   default:hp2::not_implemented("0x00199AB8","Unrecovered physics component destructor target");
  }
 }
 if(flags&1)m.release(head);
}
}
