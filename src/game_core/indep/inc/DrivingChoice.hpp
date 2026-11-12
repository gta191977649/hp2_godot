// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/DrivingChoice.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/race/ai_drive_target.hpp"
namespace hp2::debug {
class FuturePoints : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //0023B740, W74.
 // E-0099-W37; actual404-byte storage with twenty20-byte records.
 int GetClosestPointAhead(const std::array<float,2>&)const;
 void DoSmoothOffsetChange(std::uint32_t target,std::uint32_t nearby,float start,float end,int end_index,bool conservative,const ::port::race::AiDriveTargetServices&);
 void Clear();
 void AddPoint(int route,int point,int line,float offset,bool crossed); //0023E130
 bool AdvancePoint(std::uint32_t& route,std::uint32_t& point,int& index,float& offset,bool& crossed,int& line,std::uint32_t controller,const ::port::race::AiDriveTargetServices&); //0023DF50
 void BuildArray(std::uint32_t target,std::uint32_t location,std::uint32_t nearby,float& initial_l,const ::port::race::AiDriveTargetServices&); //0023E188
};
class DrivingChoice : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //0023B740, W74.
 // E-0099-W37, original DrivingChoice.hpp/drivingchoice.cpp definitions.
 void Clear(); //0023EDA0
 void SetChoice(int type,std::uint32_t car,bool extra); //0023B8B0
 void SetChoice(std::uint32_t location); //0023ECF8
 static bool DrivingChoiceTypeIsDrivingLine(int); //0023ECE8
 static bool DrivingChoiceTypeIsSideOfCar(int); //0023ECD8
 static int GetChoiceType(int line); //0023B7B8
 static int GetDrivingLine(const ::port::driving::Memory&,int type,std::uint32_t car); //0023B7D8
 bool IsValid(std::uint32_t location,std::uint32_t nearby); //0023B8F8
 // E-0099-W95; exact drivingchoice.cpp source definitions.
 bool IsEqualTo(std::uint32_t other)const; //0023B9E8.
 bool GetPreferredSideOfCar(std::uint32_t row,std::uint32_t nearby,std::uint32_t location); //0023CE18.
 bool TryToDriveAroundCar(std::uint32_t target,bool& preferred,std::uint32_t around,std::uint32_t location,std::uint32_t nearby,float projected,float maximum,bool try_current,const ::port::race::AiDriveTargetServices&); //0023C750,E-0110-F10.
 bool MakeNewChoice(std::uint32_t target,std::uint32_t location,std::uint32_t nearby,float maximum,bool& preferred,bool try_current,const ::port::race::AiDriveTargetServices&); //0023BD28.
 static bool TryChoice(::port::driving::Memory&,std::uint32_t choice,bool preferred,std::uint32_t location,std::uint32_t nearby,float maximum,bool& found,std::uint32_t this_target,std::uint32_t best_choice,std::uint32_t best_target,bool& best_preferred,float& best_score,std::uint32_t alternate_choice,std::uint32_t alternate_target,float& alternate_impact,std::uint32_t& hit,float& impact,std::uint32_t critical,const ::port::race::AiDriveTargetServices&); //0023C488.
 bool BuildDriveTarget(std::uint32_t target,std::uint32_t location,std::uint32_t nearby,float projected,int movement,const ::port::race::AiDriveTargetServices&); //0023BA58
 bool IsDriveable(std::uint32_t target,bool preferred,float& score,std::uint32_t& car_to_hit,float& impact,std::uint32_t location,std::uint32_t nearby,float projected,float maximum,int movement,const ::port::race::AiDriveTargetServices&); //0023BBB0
 bool BuildDriveTargetAroundCar(std::uint32_t target,std::uint32_t nearby,std::uint32_t location,const ::port::race::AiDriveTargetServices&);
 float GetLateralDistance(std::uint32_t nearby,std::uint32_t around,std::uint32_t future,int index)const;
 void MakeDriveTargetParallelToCar(std::uint32_t target,std::uint32_t nearby,float lateral,const ::port::race::AiDriveTargetServices&);
 bool IsEnoughRoom(std::uint32_t around,std::uint32_t nearby,const ::port::race::AiDriveTargetServices&)const;
 void FixupCarToExcludeFromCollisionPrediction(std::uint32_t car,bool preferred,std::uint32_t nearby,const ::port::race::AiDriveTargetServices&);
 static void FillUpDriveTarget(::port::driving::Memory&,std::uint32_t target,std::uint32_t location,std::uint32_t nearby,int movement,bool conservative,const ::port::race::AiDriveTargetServices&); //0023D098
 static float GetSpeedBackToLine(::port::driving::Memory&,std::uint32_t nearby,std::uint32_t location,int movement); //0023D3E0
 static void ReduceCurrentL(::port::driving::Memory&,float& lateral,float amount,int movement,std::uint32_t point,int line,float half_width,bool conservative); //0023D4B8
 static void AddToDriveTarget(::port::driving::Memory&,float lateral,std::uint32_t location,std::uint32_t future_point,std::uint32_t target,const ::port::race::AiDriveTargetServices&); //0023D318
};
}

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
