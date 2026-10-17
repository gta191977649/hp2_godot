// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trackrouteeditor.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/TrackRouteEditor.hpp"

#include "game_core/indep/src/trackrouteeditor.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/memory.hpp"

namespace hp2::recovered {
// Debug001C2ED0; E-0046-C01. Host car binding supplies original position/velocity.
void MaybeAddTrackRoutePoint(port::driving::Memory& m,std::uint32_t){
    if(m.word(0x3281d4)!=0)hp2::not_implemented("0x001C24A8","TrackRouteEditor recording is not recovered");
}
}

namespace hp2::recovered {
// ELF 0x001C3718; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c3718.md.
void FancyJoyEventHandler::IsButtonPressedRepeat() {
    hp2::not_implemented("0x001C3718", "FancyJoyEventHandler::IsButtonPressedRepeat (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001C4188; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c4188.md.
void TrackRouteEditor::GetCameraPosition() {
    hp2::not_implemented("0x001C4188", "TrackRouteEditor::GetCameraPosition (signature and full body unresolved)");
}
} // namespace hp2::recovered
