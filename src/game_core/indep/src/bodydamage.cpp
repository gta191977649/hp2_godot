// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/bodydamage.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001B4D48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b4d48.md.
void BodyDamage::SetDamage() {
    hp2::not_implemented("0x001B4D48", "BodyDamage::SetDamage (signature and full body unresolved)");
}

// ELF 0x001B4E10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b4e10.md.
void BodyDamage::GetSectorNumber() {
    hp2::not_implemented("0x001B4E10", "BodyDamage::GetSectorNumber (signature and full body unresolved)");
}

// ELF 0x001B4F10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b4f10.md.
void BodyDamage::GetDamageAmount() {
    hp2::not_implemented("0x001B4F10", "BodyDamage::GetDamageAmount (signature and full body unresolved)");
}

// ELF 0x001B50C8; E-0042-C133; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b50c8.md.
void BodyDamage::GetVisibleTotalDamage() {
    hp2::not_implemented("0x001B50C8", "BodyDamage::GetVisibleTotalDamage (signature and full body unresolved)");
}
} // namespace hp2::recovered
