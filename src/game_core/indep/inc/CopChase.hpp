#pragma once
// TASK-0099 E-0099-W38. Original ../indep/inc/CopChase.hpp inline owners,
// confirmed by raw .mdebug FDR/SYMR/marker/path. These are logical Memory views,
// not native original ABI or complete police-class implementations.
#include "game_core/port/driving/object_view.hpp"
#include <optional>
namespace port::race {struct CarConstructionServices;struct PhysicsConstructionServices;struct CarResetServices;struct AiDriveTargetServices;}
namespace hp2::debug {
// Debug /eLogicSubState datatype, E-0100-PU04; exact source names/values.
enum eLogicSubState {eLSS_NONE=0,eLSS_PURSUIT_FOLLOW=1,eLSS_PURSUIT_FOLLOW_OFFSET=2,
 eLSS_PURSUIT_LOW_SPEED_CLOSE_FOLLOW=3,eLSS_PURSUIT_FOLLOW_BACKWARDS=4,
 eLSS_PURSUIT_FLANK=5,eLSS_PURSUIT_BUMP=6,eLSS_PURSUIT_BLOCK=7,
 eLSS_PURSUIT_GET_AHEAD=8,eLSS_PURSUIT_ACQUIRE=9,eLSS_PURSUIT_STOP=10,
 eLSS_LIMBO_DISABLED=11,eLSS_LIMBO_READY=12,eLSS_CAUGHTSPEEDER_TICKETING=13,
 eLSS_CAUGHTSPEEDER_WAITING=14};
void Sub_0022B320(::port::driving::Memory&); //Debug0022B320,E-0100-CM01; source static initializer.

class tCop:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void Sub_00227A80(int);void Sub_00227AA0(int);void SetDesiredLogicState(int); /*00227BC0,W121, original cop.cpp*/ void SetPursuit(std::uint32_t); /*00227BB8,E-0100-PA01*/ void BaseDoSnapshot(std::uint32_t); /*00227AC8, W107*/};
int GetZoneFromSubState(int,float); //00229E98,E-0112-COP01.
int RequestZonePermission(::port::driving::Memory&,int,int); //00229E58.
float GetForwardTrackDistance(::port::driving::Memory&,float,float); //0022B408,E-0110-F07.
bool IsTeleportLocationClear(::port::driving::Memory&,const std::array<float,2>&); //0022B438,E-0110-F07.
void InitZonePermissionTable(::port::driving::Memory&); //00229DB8 E-0099-W91.
class tAICop:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 void Sub_002281A8(std::uint32_t,int,const ::port::race::CarConstructionServices&,::port::race::PhysicsConstructionServices&); //W91.
 void Sub_00228338(int); //W91.
 void DoSnapshot(std::uint32_t); //00228398, E-0099-W107.
 // E-0099-W96; actual cop.cpp methods, bounded reached state/action domains.
 void Think(::port::race::PhysicsConstructionServices* =nullptr); //002285F8.
 void MovementModeTransitions(::port::race::PhysicsConstructionServices*); //00228BC8.
 void HandleStateTransitions(int,int,::port::race::PhysicsConstructionServices*); //00228840.
 void Teleport(std::uint32_t,std::uint16_t,::port::race::PhysicsConstructionServices*,const ::port::race::CarResetServices*); //002284E0,E-0110-F07.
 void SetSpeedTrap(std::uint32_t,::port::race::PhysicsConstructionServices* =nullptr,const ::port::race::CarResetServices* =nullptr); //00228548,E-0110-F07.
 void InitSpeedTrapLogic(::port::race::PhysicsConstructionServices*); //00228DE0,E-0110-F07.
 void InitRoadBlockLogic(::port::race::PhysicsConstructionServices* =nullptr); //00229208,E-0112-COP01.
 void DoRoadBlockLogic(::port::race::PhysicsConstructionServices* =nullptr); //00229280.
 bool CanPatrollerSeeThisCar(std::uint32_t); //002293B0.
 void DoDrivingOnPatrolLogic(::port::race::PhysicsConstructionServices* =nullptr); //00229498.
 void DoCaughtSpeederLogic(::port::race::PhysicsConstructionServices* =nullptr); //0022AA38.
 void DoDisabledLogic(); //0022AC38.
 void DoGuardLocationLogic(); //0022B260.
 void DoSpeedTrapLogic(::port::race::PhysicsConstructionServices* =nullptr); //00228E80,E-0100-PA01; displaced reteleport remains guarded.
 void DoHotPursuitLogic(::port::race::PhysicsConstructionServices* =nullptr); //0022A5A0,E-0100-PU04; source scheduler/current dispatch.
 void DoPursuitFollowLogic(int previous_substate,std::uint32_t chased,const ::port::race::AiDriveTargetServices* =nullptr); //00229F20,E-0100-PU04; far and reviewed close paths, positive flank child guarded.
 bool IsValidPursuitTarget(std::uint32_t,bool); //002290D0,E-0100-PA01.
 float GetBestFlankSide(std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //0022A8C0,E-0112-COP01.
 void DoPursuitFollowBackwardsLogic(int,std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //00229648.
 void DoPursuitFollowOffsetLogic(int,std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //00229820.
 void DoPursuitFlankLogic(int,std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //002299E0.
 void DoPursuitBumpLogic(int,std::uint32_t,::port::race::PhysicsConstructionServices* =nullptr); //00229BE0.
 void DoLowSpeedClosePursuitFollowLogic(int,std::uint32_t); //0022A1B0.
 void DoPursuitBlockLogic(int,std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //0022A268.
 void DoPursuitAcquireLogic(int,std::uint32_t); //0022A3F8.
 void DoPursuitStopLogic(int,std::uint32_t); //0022A470.
 void DoPursuitGetAheadLogic(int,std::uint32_t,const ::port::race::AiDriveTargetServices* =nullptr); //0022A528.
 void ClearSpeedTrap(); //00228BA8,E-0110-F07.
 void ClearPursuit(); //00228B60,E-0100-PA01; actual RemoveCop/null chain.
 void DoneWritingTicket(); //0022B2C8,E-0099-W121; cop.cpp; original ticket state/timer.
 void InitializeDriveTarget(const ::port::race::AiDriveTargetServices&); //0022ACA8,E-0112-COP01.
 void UpdateDriveTarget(const std::array<float,2>&,bool,const ::port::race::AiDriveTargetServices&,std::optional<std::uint32_t> point_w={}); //0022AE40.
 bool IsNearPursuitTarget(); //0022B268.
 void BuildStationaryDriveTarget(); //0022B0D0.
 void DoLimboLogic(); //0022ABE0.
 void DisableMyCar(bool,::port::race::PhysicsConstructionServices* =nullptr); //0022B138; existing LIMBO/DISABLED early returns.

 // Debug0022B380, delay-slot return constant. No original fields consumed.
 bool IsAIControlled()const{return true;}
};
class tPlayerCop:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;
 void Sub_00227BD0(std::uint32_t,int); //W91.
 void Sub_00227C38(int); //W91.
 // Debug0022B3B8, delay-slot return zero. No original fields consumed.
 bool IsAIControlled()const{return false;}
};
}
