// TASK-0046: Debug0013E070 is canonical; E-0046-C01. Historical template
// provenance below does not describe this typed Debug member or its PS2 ABI.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): ReplayManager::BeginFrame() [0x002AEF30]
// Diagnostic text (not a callable signature): ReplayManager::HandleSnapshot() [0x002AEFF8]
namespace hp2::recovered {
class ReplayManager : public port::driving::ObjectView {
public:
    using ObjectView::ObjectView;
    // Debug0013E070; E-0046-C01; replay.cpp. No host ABI claim.
    void RegisterInterestingEvent(float interesting_value,const char* name);
};
} // namespace hp2::recovered
