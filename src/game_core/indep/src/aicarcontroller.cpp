// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\aicarcontroller.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AICarController::GetCurrentTrackRoutePoint() [0x002B4CC8]
#include "game_core/indep/inc/AICarController.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): NearbyCarList::BuildArray [0x002B4D70]
#include "game_core/indep/inc/AICarController.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00193758; E-0042-C084; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00193758.md.
void AICarController::ForceBoost() {
    hp2::not_implemented("0x00193758", "AICarController::ForceBoost (signature and full body unresolved)");
}

// ELF 0x00193780; E-0042-C085; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00193780.md.
void AICarController::ReleaseBoost() {
    hp2::not_implemented("0x00193780", "AICarController::ReleaseBoost (signature and full body unresolved)");
}
} // namespace hp2::recovered
