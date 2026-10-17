// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trackroute.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrackRoute::FindClosestPoint() [0x002B7328]
#include "game_core/indep/inc/TrackRoute.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001C1910; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1910.md.
void TrackRoute::GetRelativePoint() {
    hp2::not_implemented("0x001C1910", "TrackRoute::GetRelativePoint (signature and full body unresolved)");
}

// ELF 0x001C1D30; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1d30.md.
void TrackRoute::FindClosestPointAhead() {
    hp2::not_implemented("0x001C1D30", "TrackRoute::FindClosestPointAhead (signature and full body unresolved)");
}

// ELF 0x001C2780; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c2780.md.
void TrackRoute::DistanceIsWithinRoute() {
    hp2::not_implemented("0x001C2780", "TrackRoute::DistanceIsWithinRoute (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/trackroute.hpp"

namespace hp2::recovered {
// ELF 0x001C28A0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c28a0.md.
void TrackRouteManager::AddRouteToRightList() {
    hp2::not_implemented("0x001C28A0", "TrackRouteManager::AddRouteToRightList (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001C1340; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1340.md.
void TrackRoutePoint::Sub_001C1340() {
    hp2::not_implemented("0x001C1340", "TrackRoutePoint::Sub_001C1340 (signature and full body unresolved)");
}

// ELF 0x001C1408; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1408.md.
void TrackRoutePoint::GetPosition() {
    hp2::not_implemented("0x001C1408", "TrackRoutePoint::GetPosition (signature and full body unresolved)");
}

// ELF 0x001C1490; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1490.md.
void TrackRoutePoint::SetWidth() {
    hp2::not_implemented("0x001C1490", "TrackRoutePoint::SetWidth (signature and full body unresolved)");
}
} // namespace hp2::recovered
