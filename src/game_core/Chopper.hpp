// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

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
namespace hp2::recovered {
class Chopper {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001E8560; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e8560.md.
    // Original Debug reference: Chopper::GetMaxDeceleration(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetMaxDeceleration();
    // ELF 0x001E9AE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e9ae8.md.
    // Original Debug reference: Chopper::SetRotateTarget(float, float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetRotateTarget();
    // ELF 0x001E9E40; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e9e40.md.
    // Original Debug reference: Chopper::CalcChopperRotation(float *, float *, float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void CalcChopperRotation();
    // ELF 0x001E9E80; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e9e80.md.
    // Original Debug reference: Chopper::SubtractFromYaw(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SubtractFromYaw();
    // ELF 0x001EA578; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ea578.md.
    // Original Debug reference: Chopper::RenderChopper(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void RenderChopper();
};
} // namespace hp2::recovered
