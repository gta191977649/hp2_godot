// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/TrafficCar.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrafficCar::Move() [0x002B2178]
namespace hp2::recovered {
class TrafficCar {
public:
    // ELF 0x00167A08; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00167a08.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00167A08();
    // ELF 0x00167A60; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00167a60.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00167A60();
    // ELF 0x00167AF8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00167af8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00167AF8();
    // ELF 0x00168088; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00168088.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00168088();
};
} // namespace hp2::recovered

#include "game_core/indep/inc/CarMover.hpp"
namespace port::race {struct PhysicsConstructionServices;}
namespace hp2::debug {
// Logical shared-Memory view, no host ABI. W105 freshly corroborates the
// original TrafficCar.hpp definition owner for destructor00167550 only.
class TrafficCar:public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_00166ED0(std::uint32_t); //Original trafficcar.cpp ctor, W35/W105.
 void DoSnapshot(std::uint32_t); //Original trafficcar.cpp00166F28, W105.
 float Move(float,::port::race::PhysicsConstructionServices&); //Debug00166FC0, E-0099-W111.
 void Sub_00167550(int flags){m_.word(address_+4,0x2aa000);CarMover(m_,address_).Sub_00136B28(flags);}
};
}
