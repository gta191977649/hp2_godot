// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\drivingchoice.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): DrivingChoice::BuildDriveTarget() [0x002BE870]
// Diagnostic text (not a callable signature): DrivingChoice::IsDriveable() [0x002BE898]
// Diagnostic text (not a callable signature): DrivingChoice::MakeNewChoice() [0x002BE8B8]
// Diagnostic text (not a callable signature): DrivingChoice::TryChoice() [0x002BE8D8]
// Diagnostic text (not a callable signature): DrivingChoice::FixupCarToExcludeFromCollisionPrediction() [0x002BE8F8]
// Diagnostic text (not a callable signature): DrivingChoice::FillUpDriveTarget [0x002BE938]
// Diagnostic text (not a callable signature): DrivingChoice::GetSpeedBackToLine() [0x002BE960]
// Diagnostic text (not a callable signature): DrivingChoice::BuildDriveTargetAroundCar() [0x002BE988]
// Diagnostic text (not a callable signature): DrivingChoice::MakeDriveTargetParallelToCar() [0x002BE9B8]
#include "game_core/indep/inc/DrivingChoice.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): FuturePoints::BuildArray() [0x002BE9E8]
// Diagnostic text (not a callable signature): FuturePoints::DoSmoothOffsetChange() [0x002BEA08]
#include "game_core/indep/inc/DrivingChoice.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00241CF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00241cf8.md.
void DrivingChoice::SetChoice() {
    hp2::not_implemented("0x00241CF8", "DrivingChoice::SetChoice (signature and full body unresolved)");
}

// ELF 0x00243988; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00243988.md.
void DrivingChoice::ReduceCurrentL() {
    hp2::not_implemented("0x00243988", "DrivingChoice::ReduceCurrentL (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00244768; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00244768.md.
void FuturePoints::AddPoint() {
    hp2::not_implemented("0x00244768", "FuturePoints::AddPoint (signature and full body unresolved)");
}

// ELF 0x002452B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002452b0.md.
void FuturePoints::Dump() {
    hp2::not_implemented("0x002452B0", "FuturePoints::Dump (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/trackroute.hpp"

namespace hp2::recovered {
// ELF 0x002455B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002455b0.md.
void TrackRoutePoint::GetAmountOutOfBounds() {
    hp2::not_implemented("0x002455B0", "TrackRoutePoint::GetAmountOutOfBounds (signature and full body unresolved)");
}
} // namespace hp2::recovered
