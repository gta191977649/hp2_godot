// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): SlotPoolManager::NewSlotPool [0x002C04B0]
// Diagnostic text (not a callable signature): SlotPoolManager::SlotPoolManager() [0x002C04B0]
// Diagnostic text (not a callable signature): SlotPoolManager::DeleteSlotPool [0x002C0510]
#include "game_core/SlotPoolManager.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x002584B0; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/memory/002584b0.md.
void SlotPoolManager::NewSlotPool() {
    hp2::not_implemented("0x002584B0", "SlotPoolManager::NewSlotPool (signature and full body unresolved)");
}

// ELF 0x00258548; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/memory/00258548.md.
void SlotPoolManager::DeleteSlotPool() {
    hp2::not_implemented("0x00258548", "SlotPoolManager::DeleteSlotPool (signature and full body unresolved)");
}
} // namespace hp2::recovered
