// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\componentphysics.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): PhysicsSystem::IntegrateEuler() [0x002B5328]
// Diagnostic text (not a callable signature): PhysicsSystem::IntegrateEuler [0x002B5348]
// Diagnostic text (not a callable signature): PhysicsSystem::GetDerivative() [0x002B5380]
#include "game_core/indep/inc/ComponentPhysics.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0019C808; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0019c808.md.
void PhysicsSystem::ComputeAuxiliaries() {
    hp2::not_implemented("0x0019C808", "PhysicsSystem::ComputeAuxiliaries (signature and full body unresolved)");
}

// ELF 0x0019C8F8; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0019c8f8.md.
void PhysicsSystem::ResolveForces() {
    hp2::not_implemented("0x0019C8F8", "PhysicsSystem::ResolveForces (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x0019C170; E-0042-C089; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace port::driving::init {
void Sub_0019C170(ByteView out,std::uint32_t address){
    out.require(0,0x10);out.word(0,address);out.word(0xc,0x002b53a0);out.word(4,address);out.word(8,0);
}
}

// Retail 0x0019C238; E-0042-C090; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace port::driving::native {
void AddComponent(Memory& m,std::uint32_t head,std::uint32_t object){
    using namespace detail_group_00137100_runtime;
    const auto last=m.word(head+4),table=m.word(object+8);
    m.word(last,object);m.word(head+4,object);m.word(object+4,last);m.word(object,head);
    const auto adjusted=object+static_cast<std::int32_t>(m.signed_half(table+0x10));
    (void)adjusted; // These exact constant-return implementations do not read this.
    std::uint32_t count;
    switch(m.word(table+0x14)){
    case 0x0019dcc8:count=32;break;
    case 0x001a5c28:case 0x001a6660:count=2;break;
    case 0x0018c368:case 0x0018c1c8:count=1;break;
    default:hp2::not_implemented("0x0019C238","Unknown component virtual count target at slot +0x14");
    }
    m.word(head+8,m.word(head+8)+count);
}
}

// Retail 0x0019C808; E-0041-V01; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace port::driving::native {
void PhysicsSystem::ComputeAuxiliaries(Memory& m,std::uint32_t head,InitServices& services){
    using namespace detail_group_00137100_runtime;
    if(!services.Sub_00250F38)hp2::not_implemented("0x00250F38","Original performance-counter service is absent");
    const auto start=services.Sub_00250F38();auto object=m.word(head);
    while(object!=head){
        const auto table=m.word(object+8);
        reset_dispatch(m,m.word(table+0x44),object+static_cast<std::int32_t>(m.signed_half(table+0x40)),services);
        object=m.word(object);
    }
    const auto end=services.Sub_00250F38();
    if(!services.Sub_00250F48)hp2::not_implemented("0x00250F48","Original performance-counter delta service is absent");
    m.word(global_address(0xa780),services.Sub_00250F48(start,end));
}
}

// Retail 0x0019C670; E-0042-C092; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace port::driving::native {
void PhysicsSystem::GetDerivative(Memory& m,std::uint32_t head,std::uint32_t output,const IntegrationCalls& calls) {
    using namespace detail_group_0019d4d0;
    if(!calls.derivative)hp2::not_implemented("0x0019C670","component derivative dispatch");
    std::uint32_t i=0;
    for(auto node=m.word(head);node!=head;node=m.word(node)) {
        calls.derivative(m,node,output+i*4);i+=count(m,node);
    }
}
}

// Retail 0x0019C290; E-0042-C091; E-0042-S001; exact Debug TU ..\indep\src\componentphysics.cpp
namespace port::driving::native {
void IntegrateEuler(Memory& m,float dt,std::uint32_t head,const IntegrationCalls& calls) {
    using namespace detail_group_0019d4d0;
    // Original bounded stack is140 floats. Allocate a reusable port scratch region.
    std::uint32_t total=0;
    for(auto node=m.word(head);node!=head;node=m.word(node)) {
        const auto n=count(m,node);
        if(n>140-total)throw std::out_of_range("Original derivative stack capacity exceeded");
        total+=n;
    }
    Memory::Scratch frame(m);const auto scratch=m.allocate(140*4);PhysicsSystem::GetDerivative(m,head,scratch,calls);std::uint32_t i=0;
    for(auto node=m.word(head);node!=head;node=m.word(node)) {
        const auto n=count(m,node);if(n>140-i)throw std::out_of_range("Original derivative stack capacity exceeded");
        const auto table=m.word(node+8),target=m.word(table+0x1c);
        const auto op=m.word(target),delay=m.word(target+4);
        if(op!=0x03e00008u || (delay&0xffff0000u)!=0x24820000u)
            hp2::not_implemented("0x0019C290","component state pointer virtual target");
        const auto state=node+static_cast<std::int16_t>(m.half(table+0x18))+static_cast<std::int16_t>(delay&0xffffu);
        for(std::uint32_t j=0;j<n;++j){const float product=dt*m.scalar(scratch+(i+j)*4);m.scalar(state+j*4,m.scalar(state+j*4)+product);}i+=n;
    }
    if(!calls.after)hp2::not_implemented("0x0019C290","post integration dispatch");
    calls.after(m,head);
}
}

namespace hp2::recovered {
// ELF 0x0019C170; E-0042-C089; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019c170.md.
void PhysicsSystem::Sub_0019C170() {
    hp2::not_implemented("0x0019C170", "PhysicsSystem::Sub_0019C170 (signature and full body unresolved)");
}

// ELF 0x0019C238; E-0042-C090; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019c238.md.
void PhysicsSystem::AddComponent() {
    hp2::not_implemented("0x0019C238", "PhysicsSystem::AddComponent (signature and full body unresolved)");
}

// ELF 0x0019C290; E-0042-C091; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019c290.md.
void PhysicsSystem::IntegrateEuler() {
    hp2::not_implemented("0x0019C290", "PhysicsSystem::IntegrateEuler (signature and full body unresolved)");
}

// ELF 0x0019C670; E-0042-C092; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019c670.md.
void PhysicsSystem::GetDerivative() {
    hp2::not_implemented("0x0019C670", "PhysicsSystem::GetDerivative (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0019C910; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019c910.md.
void PhysicsComponent::ResolveForces() {
    hp2::not_implemented("0x0019C910", "PhysicsComponent::ResolveForces (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0019C190; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019c190.md.
void PhysicsSystem::Sub_0019C190() {
    hp2::not_implemented("0x0019C190", "PhysicsSystem::Sub_0019C190 (signature and full body unresolved)");
}
} // namespace hp2::recovered
