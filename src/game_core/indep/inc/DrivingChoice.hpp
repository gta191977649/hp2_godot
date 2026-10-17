// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/DrivingChoice.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

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
namespace hp2::recovered {
class DrivingChoice {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00241CF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00241cf8.md.
    // Original Debug reference: DrivingChoice::SetChoice(DrivingChoiceType, Car *, bool); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetChoice();
    // ELF 0x00243988; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00243988.md.
    // Original Debug reference: DrivingChoice::ReduceCurrentL(float *, float, MovementWithRespectToLine, TrackRoutePoint *, int, float, bool); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ReduceCurrentL();
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): FuturePoints::BuildArray() [0x002BE9E8]
// Diagnostic text (not a callable signature): FuturePoints::DoSmoothOffsetChange() [0x002BEA08]
namespace hp2::recovered {
class FuturePoints {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00244768; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00244768.md.
    // Original Debug reference: FuturePoints::AddPoint(int, int, int, float, bool); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void AddPoint();
    // ELF 0x002452B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002452b0.md.
    // Original Debug reference: FuturePoints::Dump(Car *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Dump();
};
} // namespace hp2::recovered
