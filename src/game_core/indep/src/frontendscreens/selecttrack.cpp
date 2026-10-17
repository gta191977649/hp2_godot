// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\frontendscreens\selecttrack.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/SelectTrackScreen.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/SelectableTrack.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/TimeOfDayTrack.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/tTotalTimeRecord.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0019E048; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019e048.md.
void SelectableTrack::GetTimeOfDay() {
    hp2::not_implemented("0x0019E048", "SelectableTrack::GetTimeOfDay (signature and full body unresolved)");
}

// ELF 0x0019E0A8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019e0a8.md.
void SelectableTrack::ScrollTimeOfDay() {
    hp2::not_implemented("0x0019E0A8", "SelectableTrack::ScrollTimeOfDay (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/frontendscreens/selecttrack.hpp"

namespace hp2::recovered {
// ELF 0x0019E7B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019e7b0.md.
void SelectableTrackManager::GetNextTrack() {
    hp2::not_implemented("0x0019E7B0", "SelectableTrackManager::GetNextTrack (signature and full body unresolved)");
}

// ELF 0x0019E7D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019e7d0.md.
void SelectableTrackManager::GetPrevTrack() {
    hp2::not_implemented("0x0019E7D0", "SelectableTrackManager::GetPrevTrack (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0019EF00; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019ef00.md.
void TrackRecordDisplayer::Sub_0019EF00() {
    hp2::not_implemented("0x0019EF00", "TrackRecordDisplayer::Sub_0019EF00 (signature and full body unresolved)");
}
} // namespace hp2::recovered
