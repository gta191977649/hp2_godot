// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class tNearFollowTarget {
public:
    // ELF 0x0022D080; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0022d080.md.
    // Original Debug reference: tNearFollowTarget::GetVelocity(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetVelocity();
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
#include <array>
#include <optional>
namespace port::race {struct PhysicsConstructionServices;}
namespace hp2::debug {
// Port companion view; original method definition belongs to pursuit.cpp.
class tNearFollowTarget:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void Sub_00227568();void Sub_002275F0(int);void DoSnapshot(std::uint32_t);std::array<float,2> Update(std::uint32_t position,std::uint32_t velocity,float);std::array<float,2> GetVelocity(float)const; /*00227708,E-0112-COP01.*/ /*00227618,PU02 explicit host return*/}; //W119.
class tRoadBlockStateMachine:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void Sub_00227288(std::uint32_t pursuit);void Sub_002272A0(int);void RequestRoadBlock(bool=false);void Update(bool=false,::port::race::PhysicsConstructionServices* =nullptr);void DisbandRoadBlock();void DoSnapshot(std::uint32_t snapshot,std::uint32_t pursuit);}; //W119.
class tPursuit:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 bool Update(bool disable_speech=false,std::optional<std::uint32_t> add_point_w={},std::optional<std::uint32_t> set_point_w={},::port::race::PhysicsConstructionServices* physics=nullptr); //00223B08,E-0100-PU02; initial ordinary chase path.
 void CopBackOffSpeech(bool=false);void AITicketSpeech(bool=false);
 void UpdateChopper(bool=false);void UpdateRoadBlocks(bool=false,::port::race::PhysicsConstructionServices* =nullptr);void CreateRoadBlock(bool=false,::port::race::PhysicsConstructionServices* =nullptr);
 void UpdateDriveTargets(float,std::optional<std::uint32_t> add_point_w={},std::optional<std::uint32_t> set_point_w={}); //00224B90.
 void SelectPrimaryCop(); //00224730.
 bool ShouldPursuitBeCalledOff(); //002269A0.
 bool DoStateTransition(bool disable_speech=false); //002249D0.
 bool IsChasedCarStoppable(); //00226618; ordinary chase, positive capture handled by caller.
 void Level0SubStates(bool disable_speech=false); //00223E38.
 void DoEarlyChaseCollisionCheck(bool disable_speech=false); //00225388.
 bool HasThisCop(std::uint32_t)const; /*00226FA8,W107*/
 bool RemoveCopFromPursuit(std::uint32_t); //00226DC8,E-0100-PA01.
 bool IsPlayerInvolved()const; //00226358,E-0100-PA01.
 std::uint32_t GetExactVehicleDescription(); //00225260,E-0100-PA01.
 std::uint32_t GetCopSpeaker(); //00225318,E-0100-PA01.
 std::uint32_t GetChasedCarColour(); //00225230,E-0100-PA01.
 float GetSpeedMPHOrKPH(); //00224A60,E-0100-PA01.
 void BeginPursuitCollisionSpeech(std::uint32_t,bool disable_speech=false); //002256E0,E-0100-PA01.
 void StartPursuitSpeech(bool disable_speech=false); //00225778,E-0100-PA01.
 bool RoadBlockEncounteredRecently()const; //00224638,E-0112-COP02.
 void RemovePrimarySpeaker(bool caused_by_crash,bool disable_speech=false); //00227018,E-0112-COP02.
 void JoinPursuitSpeech(bool disable_speech=false); //00225A30,E-0100-PA01.
 void Sub_00223850(std::int8_t id,std::uint32_t chased);void Sub_00223A18(int);void Sub_002279C8();
 float ScaleTimeByRaceLength(float)const;void SwitchPursuitState(int,bool disable_speech=false);void ReleaseAllCops();void DisbandRoadBlock(bool);void ReleaseChopper();void DoSnapshot(std::uint32_t);
}; //W119 source-owned lifetime and scoped snapshots.

}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class tPursuit {
public:
    // ELF 0x00229ED0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00229ed0.md.
    // Original Debug reference: tPursuit::SelectPrimaryCop(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SelectPrimaryCop();
    // ELF 0x0022BB28; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0022bb28.md.
    // Original Debug reference: tPursuit::IsCopNearChasedCar(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsCopNearChasedCar();
    // ELF 0x0022BC08; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0022bc08.md.
    // Original Debug reference: tPursuit::IsRacingWithMe(Car *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsRacingWithMe();
    // ELF 0x0022C920; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0022c920.md.
    // Original Debug reference: tPursuit::HasThisCop(tCop *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void HasThisCop();
};
} // namespace hp2::recovered

#include "game_core/indep/inc/Pursuit.hpp"
