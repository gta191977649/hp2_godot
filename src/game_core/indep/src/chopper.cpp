// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\chopper.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): Chopper::ShiftTarget() [0x002B95E0]
// Diagnostic text (not a callable signature): Chopper::FindUnobstructedSpot() [0x002B95F8]
// Diagnostic text (not a callable signature): Chopper::PlaceChopperInWorld() [0x002B9618]
// Diagnostic text (not a callable signature): Chopper::StartChaseLogic() [0x002B9638]
// Diagnostic text (not a callable signature): Chopper::EndChaseLogic() [0x002B9658]
// Diagnostic text (not a callable signature): Chopper::FindChopperPointFromScratch() [0x002B9678]
// Diagnostic text (not a callable signature): Chopper::FindChopperRoutePoint() [0x002B96A0]
// Diagnostic text (not a callable signature): Chopper::FindPreviousFork() [0x002B96C8]
// Diagnostic text (not a callable signature): Chopper::SwitchToNearbyFork() [0x002B96E8]
// Diagnostic text (not a callable signature): Chopper::FollowRoute() [0x002B9708]
// Diagnostic text (not a callable signature): Chopper::BeginStakeout() [0x002B9720]
// Diagnostic text (not a callable signature): Chopper::HangoutNearHere() [0x002B9740]
// Diagnostic text (not a callable signature): Chopper::AdjustTargetInBounds() [0x002B9760]
// Diagnostic text (not a callable signature): Chopper::CleverPursuitPlan() [0x002B9780]
// Diagnostic text (not a callable signature): Chopper::BombRunLogic() [0x002B97A0]
// Diagnostic text (not a callable signature): Chopper::HighLevelPlanning() [0x002B9818]
// Diagnostic text (not a callable signature): Chopper::Think() [0x002B9838]
// Diagnostic text (not a callable signature): Chopper::MoveChopper() [0x002B9850]
// Diagnostic text (not a callable signature): Chopper::DrawChopper() [0x002B9868]
// Diagnostic text (not a callable signature): Chopper::TriggerDustEmitter() [0x002B9880]
// Diagnostic text (not a callable signature): Chopper::Fly() [0x002B98A0]
// Diagnostic text (not a callable signature): Chopper::FireSalvo() [0x002B98B0]
// Diagnostic text (not a callable signature): Chopper::FireWeapon() [0x002B98C8]
#include "game_core/Chopper.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001E8560; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e8560.md.
void Chopper::GetMaxDeceleration() {
    hp2::not_implemented("0x001E8560", "Chopper::GetMaxDeceleration (signature and full body unresolved)");
}

// ELF 0x001E9AE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e9ae8.md.
void Chopper::SetRotateTarget() {
    hp2::not_implemented("0x001E9AE8", "Chopper::SetRotateTarget (signature and full body unresolved)");
}

// ELF 0x001E9E40; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e9e40.md.
void Chopper::CalcChopperRotation() {
    hp2::not_implemented("0x001E9E40", "Chopper::CalcChopperRotation (signature and full body unresolved)");
}

// ELF 0x001E9E80; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e9e80.md.
void Chopper::SubtractFromYaw() {
    hp2::not_implemented("0x001E9E80", "Chopper::SubtractFromYaw (signature and full body unresolved)");
}

// ELF 0x001EA578; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ea578.md.
void Chopper::RenderChopper() {
    hp2::not_implemented("0x001EA578", "Chopper::RenderChopper (signature and full body unresolved)");
}
} // namespace hp2::recovered
