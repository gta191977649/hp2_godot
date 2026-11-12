// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
#include <array>
namespace hp2::debug {
class eAnimation:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001A3FD8();void Sub_001A4030(int);
 void Init(std::uint32_t);void UnInit();
 void SetAnimation(std::uint32_t,float); //001A4148, source hash binding/frame.
 void GetBoundingBox(std::array<float,4>&,std::array<float,4>&)const; //001A4730, XYZ source bounds; preserve W.
};
// Debug001A4990, E-0099-W28. Explicit original animation-list/bone-scale startup.
void Sub_001A4990(::port::driving::Memory&);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class eAnimation {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered
