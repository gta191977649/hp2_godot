#include "game_core/indep/inc/ComponentPhysics.hpp"
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
        reset_dispatch(m,m.word(table+0x44),object+static_cast<std::int32_t>(m.signed_half(table+0x40)),services);
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
        for(std::uint32_t j=0;j<n;++j){const float product=dt*m.scalar(scratch+(i+j)*4);m.scalar(state+j*4,m.scalar(state+j*4)+product);}i+=n;
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
