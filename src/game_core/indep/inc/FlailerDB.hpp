// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/FlailerDB.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): FlailerRenderManager::BeginFrame() [0x002AF230]
// Diagnostic text (not a callable signature): FlailerRenderManager::RenderLayer() [0x002AF258]
// Diagnostic text (not a callable signature): FlailerRenderManager::RenderScreenPrintf() [0x002AF298]
namespace hp2::recovered {
class FlailerRenderManager {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00145AF0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00145af0.md.
    // Original Debug reference: FlailerRenderManager::FlailerRenderManager(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00145AF0();
    // ELF 0x00145CF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00145cf8.md.
    // Original Debug reference: FlailerRenderManager::IsScreenVisible(unsigned int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsScreenVisible();
};
} // namespace hp2::recovered
