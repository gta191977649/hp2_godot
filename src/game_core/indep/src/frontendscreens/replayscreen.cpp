// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\frontendscreens\replayscreen.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): ReplayScreen::Update() [0x002AE150]
#include "game_core/indep/inc/ReplayScreen.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001EC510; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ec510.md.
void ReplayScreen::HasCameraMover() {
    hp2::not_implemented("0x001EC510", "ReplayScreen::HasCameraMover (signature and full body unresolved)");
}

// ELF 0x001ED410; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ed410.md.
void ReplayScreen::GetTimelinePositionX() {
    hp2::not_implemented("0x001ED410", "ReplayScreen::GetTimelinePositionX (signature and full body unresolved)");
}
} // namespace hp2::recovered
