#pragma once
// TASK-0044 E-0044-S01/S02; Debug SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Exact source indep/src/componentphysics.cpp and indep/inc/ComponentPhysics.hpp.
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/driving/projections.hpp"
namespace hp2::recovered {
class PhysicsSystem : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 // Debug00199a98 constructor body; allocation is a separate port boundary.
 void port_initialize();
 static void port_initialize(port::driving::init::ByteView,std::uint32_t);
 // Debug00199b60/00199bb8/00199f98/0019a130/0019a220.
 void AddComponent(std::uint32_t component);
 void IntegrateEuler(float dt,const port::driving::native::IntegrationCalls&);
 void GetDerivative(std::uint32_t output,const port::driving::native::IntegrationCalls&);
 void ComputeAuxiliaries(port::driving::native::InitServices&);
 void ResolveForces();
};
class PhysicsComponent : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 // Debug0019a230/0019a238.
 void ComputeAuxiliaries();void ResolveForces();
};
}
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring class with adopted retail correspondence; evidence: E-0042-S001.
// Placement: TASK-0042 E-0042-S001; exact Debug ComponentPhysics.hpp definition marker for PhysicsVehicle destructor emitted in physicscar.cpp; no native inheritance or ABI assertion.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class PhysicsVehicle {
public:
    // ELF 0x00139678; E-0042-U030; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00139678.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00139678();
};
} // namespace hp2::recovered
