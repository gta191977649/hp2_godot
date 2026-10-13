// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): DemoDiscManager::Init() [0x002ADA80]
// Diagnostic text (not a callable signature): DemoDiscManager::SetEndReason() [0x002ADDC8]
// Diagnostic text (not a callable signature): DemoDiscManager::EndDemo() [0x002ADE40]
#include "game_core/DemoDiscManager.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0012E6C8; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/startup/0012e6c8.md.
void DemoDiscManager::Init() {
    hp2::not_implemented("0x0012E6C8", "DemoDiscManager::Init (signature and full body unresolved)");
}

// ELF 0x0012EB00; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/startup/0012eb00.md.
void DemoDiscManager::SetEndReason() {
    hp2::not_implemented("0x0012EB00", "DemoDiscManager::SetEndReason (signature and full body unresolved)");
}
} // namespace hp2::recovered
