// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): ReplayManager::BeginFrame() [0x002AEF30]
// Diagnostic text (not a callable signature): ReplayManager::HandleSnapshot() [0x002AEFF8]
namespace hp2::recovered {
class ReplayManager {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered
