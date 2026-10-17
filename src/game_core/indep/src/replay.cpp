// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\replay.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): ReplayManager::BeginFrame() [0x002AEF30]
// Diagnostic text (not a callable signature): ReplayManager::HandleSnapshot() [0x002AEFF8]
#include "game_core/ReplayManager.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/Replay.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0013CD68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0013cd68.md.
void ReplaySnapshot::GetChecksum() {
    hp2::not_implemented("0x0013CD68", "ReplaySnapshot::GetChecksum (signature and full body unresolved)");
}

// ELF 0x0013CE78; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0013ce78.md.
void ReplaySnapshot::ReadData() {
    hp2::not_implemented("0x0013CE78", "ReplaySnapshot::ReadData (signature and full body unresolved)");
}
} // namespace hp2::recovered
