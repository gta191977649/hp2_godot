// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/AIPlan.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/race/ai_drive_target.hpp"
#include "game_core/inc/bSlotPool.hpp"
namespace hp2::debug {
// Debug enum spellings/values recovered with E-0110-F06.
enum HeadingClassification {HEADING_OKAY=0,HEADING_OUT_OF_ROUTE_FORWARDS=1,HEADING_OUT_OF_ROUTE_BACKWARDS=2,HEADING_NONE=3};
enum CollisionClassification {COLLISION_IN_FRONT=0,COLLISION_BEHIND=1,COLLISION_LEFT_SIDE=2,COLLISION_RIGHT_SIDE=3,COLLISION_NONE=4};
// Debug001C30B8/001C30E8, E-0099-W89; original aiplan.cpp process lifetime.
void InitAIPlans(::port::driving::Memory&);
void CloseAIPlans(::port::driving::Memory&);
class AIPlan:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void DoSnapshot(std::uint32_t snapshot);void Sub_001C7C88(float timeout);void SetTimeout(float);void Sub_001C7C60(int flags);};
class AIAction:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void DoSnapshot(std::uint32_t snapshot);static void DoSnapshot(::port::driving::Memory&,std::uint32_t pp_action,std::uint32_t snapshot);void Sub_001C31A8(int score,float timeout);void Sub_001C3BA8(int flags);void UpdateStatus(std::uint32_t information,const ::port::race::AiDriveTargetServices* =nullptr);void SetDesiredSpeedSlowerThanThisCar(std::uint32_t row,std::uint32_t nearby,float amount,float seconds);void SetDesiredSpeedToMaxAcceleration(std::uint32_t,float);void SetDesiredSpeedToMaxDeceleration(std::uint32_t,float); //001C35A8/001C3600,E-0110-F11.
 static HeadingClassification GetHeadingClassification(::port::driving::Memory&,std::uint32_t location,std::uint32_t car,bool check_steer_to_point); //001C3658,E-0110-F06.
 static CollisionClassification GetCollisionClassification(::port::driving::Memory&,std::uint32_t collision,std::uint32_t car); //001C3A50,E-0110-F06.
};
class AIActionJustDrive:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void DoSnapshot(std::uint32_t snapshot);void Sub_001C4430(int score,float timeout);void Sub_001C7818(int flags);bool HasBeenAccomplished(std::uint32_t)const;bool HasFailed(std::uint32_t)const;bool HasExpired(std::uint32_t)const;static int CalculateScore(std::uint32_t information);void CalculatePointsToDrive(std::uint32_t driving_information,const ::port::race::AiDriveTargetServices&);};
// Debug001C3D70/001C4A40/001C5C88, E-0099-W90. Exact negative prefixes;
// positive choice/recovery children retain address-specific guards.
class AIActionAvoidCars:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 void Sub_001C3C18(int score,float timeout,std::uint32_t car,float impact,bool okay,bool preferred);
 // Debug001C7A20, E-0099-W93: original inline definition owner AIPlan.hpp.
 void Sub_001C7A20(int flags){m_.word(address_+0x10,0x2af9c0);AIAction(m_,address_).Sub_001C3BA8(0);if(flags&1)SlotPool(m_,m_.word(0x3281e8)).Free(address_);}
 bool HasBeenAccomplished(std::uint32_t info);bool HasExpired(std::uint32_t)const;bool HasFailed(std::uint32_t)const;
 void CalculatePointsToDrive(std::uint32_t info,const ::port::race::AiDriveTargetServices&);
 void DoSnapshot(std::uint32_t snapshot);
 static int CalculateScore(::port::driving::Memory&,std::uint32_t information,std::uint32_t& car,std::uint32_t proposed,float& impact,bool& okay,bool& preferred,const ::port::race::AiDriveTargetServices&);};
class AIActionPass:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 void Sub_001C4960(int,float,std::uint32_t,float); //E-0110-F10.
 void Sub_001C79A8(int flags){m_.word(address_+0x10,0x2af980);AIAction(m_,address_).Sub_001C3BA8(0);if(flags&1)SlotPool(m_,m_.word(0x3281e8)).Free(address_);} //001C79A8, original AIPlan.hpp inline destructor.
 void DoSnapshot(std::uint32_t); //001C49C8.
 bool HasBeenAccomplished(std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //001C4BF8.
 bool HasExpired(std::uint32_t)const; //001C4D68.
 bool HasFailed(std::uint32_t); //001C4DB0.
 void CalculatePointsToDrive(std::uint32_t,const ::port::race::AiDriveTargetServices&); //001C4F10.
static bool SideOfCarIsOkay(::port::driving::Memory&,std::uint32_t,std::uint32_t,bool); //001C4E90,E-0110-F10.
static int CalculateScore(::port::driving::Memory&,std::uint32_t information,std::uint32_t& car,std::uint32_t proposed,float& projected,const ::port::race::AiDriveTargetServices* =nullptr);};
// E-0110-F14, original AIPlan.hpp/aiplan.cpp; actual source action type6.
class AIActionPullOver:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 static int CalculateScore(::port::driving::Memory&,std::uint32_t); //001C5468,E-0099-W103.
 void Sub_001C5300(int,float,std::uint32_t,std::uint32_t,float);
 void Sub_001C77A8(int flags){m_.word(address_+0x10,0x2af840);AIAction(m_,address_).Sub_001C3BA8(0);if(flags&1)SlotPool(m_,m_.word(0x3281e8)).Free(address_);}
 void DoSnapshot(std::uint32_t); //001C53D8.
 bool HasBeenAccomplished(std::uint32_t); //001C5678.
 bool HasExpired(std::uint32_t)const; //001C56B8.
 bool HasFailed(std::uint32_t)const; //001C56C0.
 float GetDistanceToRightEdgeOfRoute(std::uint32_t,std::uint32_t)const; //001C56C8.
 void CalculatePointsToDrive(std::uint32_t,const ::port::race::AiDriveTargetServices&); //001C5710.
};
class AIActionGetUnstuck:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 void Sub_001C5B88(int,float,float); //E-0110-F12, actual minimum lifetime operand.
 void Sub_001C76C8(int flags){m_.word(address_+0x10,0x2af7c0);AIAction(m_,address_).Sub_001C3BA8(0);if(flags&1)SlotPool(m_,m_.word(0x3281e8)).Free(address_);} //Original AIPlan.hpp inline.
 void DoSnapshot(std::uint32_t); //001C5C10.
 std::uint32_t GetGoalTrackRoutePoint(std::uint32_t)const; //001C6070; literal+3 source pointer, no invented endpoint clamp.
 std::array<float,2> GetGoalPoint(std::uint32_t)const; //001C60C8.
 void GetGoalPoint(std::uint32_t,std::uint32_t,std::uint32_t)const; //001C6130; explicit underlying route-position W operand.
 bool HasBeenAccomplished(std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //001C6198.
 bool HasExpired(std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //001C62F0.
 bool HasFailed(std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //001C6398.
 void CalculatePointsToDrive(std::uint32_t,const ::port::race::AiDriveTargetServices&); //001C65A8.

 static int CalculateScore(::port::driving::Memory&,std::uint32_t information,const ::port::race::AiDriveTargetServices* =nullptr); //001C5C88,E-0110-F06.
 static bool AreWeStuckBehindSomeone(::port::driving::Memory&,std::uint32_t location,std::uint32_t nearby); //001C5F40,E-0110-F06.
};
class AIActionAvoidRoadBlock {public:static int CalculateScore(::port::driving::Memory&,std::uint32_t information,std::uint32_t& roadblock);}; //001C6768, W47.
class AIActionIJustGotHit:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 void Sub_001C6C98(int,float,std::uint32_t); //E-0110-F11.
 void Sub_001C7658(int flags){m_.word(address_+0x10,0x2af780);AIAction(m_,address_).Sub_001C3BA8(0);if(flags&1)SlotPool(m_,m_.word(0x3281e8)).Free(address_);} //Original AIPlan.hpp inline.
 void DoSnapshot(std::uint32_t); //001C6CE8.
 bool HasBeenAccomplished(std::uint32_t); //001C6EC0.
 bool HasExpired(std::uint32_t)const; //001C6F40.
 bool HasFailed(std::uint32_t)const; //001C6F88, original aiplan.cpp JR/clear.
 void CalculatePointsToDrive(std::uint32_t,const ::port::race::AiDriveTargetServices&); //001C6F90.
static int CalculateScore(::port::driving::Memory&,std::uint32_t information,std::uint32_t& car);}; //001C6D50.
// E-0110-F15, source AIPlan.hpp/aiplan.cpp actual accident recovery action.
class AIActionAccidentRecovery:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 static int CalculateScore(::port::driving::Memory&,std::uint32_t); //001C72D0.
 static bool SortOfParallelToTrack(::port::driving::Memory&,std::uint32_t,float); //001C7318.
 void Sub_001C7218(int,float);
 void Sub_001C75E8(int flags){m_.word(address_+0x10,0x2af740);AIAction(m_,address_).Sub_001C3BA8(0);if(flags&1)SlotPool(m_,m_.word(0x3281e8)).Free(address_);}
 void DoSnapshot(std::uint32_t); //001C7268.
 bool HasBeenAccomplished(std::uint32_t); //001C73C0.
 bool HasExpired(std::uint32_t)const; //001C7410.
 bool HasFailed(std::uint32_t)const; //001C7458.
 void CalculatePointsToDrive(std::uint32_t,const ::port::race::AiDriveTargetServices&); //001C7460.
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionAccidentRecovery::CalculatePointsToDrive() [0x002B7990]
namespace hp2::recovered {
class AIActionAccidentRecovery {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionAvoidCars::CalculatePointsToDrive() [0x002B7678]
namespace hp2::recovered {
class AIActionAvoidCars {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionAvoidRoadBlock::CalculatePointsToDrive() [0x002B78F8]
namespace hp2::recovered {
class AIActionAvoidRoadBlock {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionBlock::CalculatePointsToDrive() [0x002B7800]
namespace hp2::recovered {
class AIActionBlock {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionDraft::CalculatePointsToDrive() [0x002B7720]
namespace hp2::recovered {
class AIActionDraft {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionGetUnstuck::CalculatePointsToDrive() [0x002B78A0]
namespace hp2::recovered {
class AIActionGetUnstuck {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001C9018; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c9018.md.
    // Original Debug reference: AIActionGetUnstuck::GetGoalTrackRoutePoint(DrivingLocation *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetGoalTrackRoutePoint();
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionIJustGotHit::CalculatePointsToDrive() [0x002B7950]
namespace hp2::recovered {
class AIActionIJustGotHit {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionJustDrive::CalculatePointsToDrive() [0x002B76D0]
namespace hp2::recovered {
class AIActionJustDrive {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionNudgeCar::CalculatePointsToDrive() [0x002B77B8]
namespace hp2::recovered {
class AIActionNudgeCar {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionPass::CalculatePointsToDrive() [0x002B7768]
namespace hp2::recovered {
class AIActionPass {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AIActionPullOver::CalculatePointsToDrive() [0x002B7850]
namespace hp2::recovered {
class AIActionPullOver {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001C8660; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c8660.md.
    // Original Debug reference: AIActionPullOver::GetDistanceToRightEdgeOfRoute(NearbyCarList *, TrackRoutePoint *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetDistanceToRightEdgeOfRoute();
};
} // namespace hp2::recovered
