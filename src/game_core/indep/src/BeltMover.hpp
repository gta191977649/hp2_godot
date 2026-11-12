// Original translation unit: `..\indep\src\beltmover.cpp` (ELF 0x002BB048; E-0029-P01).
// Directory binding through 0x00204098; see reverse/classes/source-path-bindings.json.
// This per-class file is a port-side partition, not the original translation unit or header.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: confirmed original directory (E-0029-P01); per-class basename/header extension port-side.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): BeltMover::Move() [0x002BB078]
namespace hp2::recovered {
class BeltMover {
public:
    // ELF 0x00204098; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00204098.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00204098();
    // ELF 0x002043A0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/002043a0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_002043A0();
    // ELF 0x00204468; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00204468.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: BeltMover::DoSnapshot(ReplaySnapshot *); E-0041-V01; retail ABI unconfirmed.
    void DoSnapshot();
    // ELF 0x00204D78; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00204d78.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: BeltMover::Move(float); E-0041-V01; retail ABI unconfirmed.
    void Move();
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
namespace port::race {struct PhysicsConstructionServices;}
namespace hp2::debug {
void Sub_002005D8(::port::driving::Memory&); //E-0099-W106 original process static initializer.

// Debug source exact lifetime/SetBelt/Move; E-0099-W100/W102/W106.
// Exceptional numerical domains retain their original-address guards.
class BeltMover:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void Sub_001FF108(std::uint32_t car);void Sub_001FF410(int flags);void ClearCalculatedVariables();
 void DoSnapshot(std::uint32_t snapshot); //001FF4D8,E-0099-W110.
 float Move(float,::port::race::PhysicsConstructionServices&);
 void GetPoint(std::array<float,4>&,float);void GetDerivative(std::array<float,4>&,float);void GetSecondDerivative(std::array<float,4>&,float);float AverageAir(float);float SlideCoef(float);
 void SetBelt();void FindCoordinates();float Dot(const std::array<float,4>& point,std::array<float,4>& to,std::array<float,4>& slope,float t);float GetMaximumAcceleration();float GetAccelerationDueToGravity(); //E-0099-W102.
};
}
