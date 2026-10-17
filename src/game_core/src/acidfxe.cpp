// TASK-0042 E-0042-S001: exact Debug source marker(s): src\acidfxe.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AcidEffectSystem::DoTimeStep() [0x002AD370]
// Diagnostic text (not a callable signature): AcidEffectSystem::RenderEffects [0x002AD390]
#include "game_core/indep/inc/AcidFX.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00127418; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00127418.md.
void AcidEffectSystem::Sub_00127418() {
    hp2::not_implemented("0x00127418", "AcidEffectSystem::Sub_00127418 (signature and full body unresolved)");
}

// ELF 0x00127688; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00127688.md.
void AcidEffectSystem::RemoveAutoEmitter() {
    hp2::not_implemented("0x00127688", "AcidEffectSystem::RemoveAutoEmitter (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/src/acidfxe.hpp"

namespace hp2::recovered {
// ELF 0x00129880; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00129880.md.
void RainSplash::Recalc() {
    hp2::not_implemented("0x00129880", "RainSplash::Recalc (signature and full body unresolved)");
}
} // namespace hp2::recovered
