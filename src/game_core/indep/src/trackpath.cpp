// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trackpath.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrackPathCoordinate::UpdatePosition() [0x002BB238]
#include "game_core/indep/inc/TrackPath.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x002086B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002086b8.md.
void TrackPathCoordinate::Sub_002086B8() {
    hp2::not_implemented("0x002086B8", "TrackPathCoordinate::Sub_002086B8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/trackpath.hpp"

namespace hp2::recovered {
// ELF 0x00207C68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00207c68.md.
void TrackPathManager::FindClosestPoint() {
    hp2::not_implemented("0x00207C68", "TrackPathManager::FindClosestPoint (signature and full body unresolved)");
}

// ELF 0x00207ED0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00207ed0.md.
void TrackPathManager::FindClosestAdjacentPointOnSamePath() {
    hp2::not_implemented("0x00207ED0", "TrackPathManager::FindClosestAdjacentPointOnSamePath (signature and full body unresolved)");
}

// ELF 0x00208598; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00208598.md.
void TrackPathManager::CompareDistances() {
    hp2::not_implemented("0x00208598", "TrackPathManager::CompareDistances (signature and full body unresolved)");
}

// ELF 0x002085E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002085e8.md.
void TrackPathManager::ComparePercentDistances() {
    hp2::not_implemented("0x002085E8", "TrackPathManager::ComparePercentDistances (signature and full body unresolved)");
}
} // namespace hp2::recovered
