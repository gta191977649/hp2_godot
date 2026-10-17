// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/DemoDisc.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): DemoDiscManager::Init() [0x002ADA80]
// Diagnostic text (not a callable signature): DemoDiscManager::SetEndReason() [0x002ADDC8]
// Diagnostic text (not a callable signature): DemoDiscManager::EndDemo() [0x002ADE40]
namespace hp2::recovered {
class DemoDiscManager {
public:
    // ELF 0x0012E6C8; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/startup/0012e6c8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Init();
    // ELF 0x0012EB00; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/startup/0012eb00.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetEndReason();
};
} // namespace hp2::recovered
