#pragma once
// Port-owned logical-address bindings; no native PS2 class ABI is asserted.
// TASK-0099 E-0099-W24. Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/track/route_data.hpp"
#include "game_core/indep/inc/CopDriver.hpp"
namespace port::race {struct AiDriveTargetServices;}
namespace port::race {struct PhysicsConstructionServices;}

namespace hp2::debug {
class Average : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //Debug0018DE40, E-0099-W74.
 // Debug0018DBF8 constructor and0018DD58 reset; original table.cpp.
 void Sub_0018DBF8(int n);
 void Sub_0018DC70(int in_chrg); // Debug0018DC70 destructor.
 void Reset(float);
 void Record(float); //Debug0018DCC0, E-0099-W47.
};
class PidError : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //Debug0018DFA0, E-0099-W74.
 // Debug0018E010; original table.cpp.
 void Reset(float);
 void Record(float error,float seconds); //Debug0018DF38, E-0099-W47.
 void Sub_0013AB30(int in_chrg); // Debug0013AB30, Table.hpp emitted in psychodriver.cpp.
};
class QuickSpline : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot,int num_fields); //00236F48,E-0099-W110.
 // Debug00236D88/00236EE0/00237010; original quickspline.cpp.
 void Sub_00236D88(int endpoint_type,int basis_type,int max_control_points);
 void Sub_00236E78(int in_chrg); // Debug00236E78 destructor.
 void Init(int endpoint_type,int basis_type);
 void SetNumControlPoints(int);
 void FixupEndpoints(); //002370A8, E-0099-W60.
 float ClampParam(float); //002372E8; loop FMod remains explicit.
 void GenerateExtrapolatedControlPoints(std::array<std::array<float,4>,4>&,int); //00237340.
 void EvaluateSpline(std::array<float,4>&,float,int,const std::array<std::array<float,4>,4>&); //00237268.
 void GetPoint(std::array<float,4>&,float,int derivative); //00237448.
 float FindClosestParam(const std::array<float,4>&,float initial,float tolerance); //00237748.
 int FindClosestControlPoint(const std::array<float,4>& point,int start); //002375A8,W102.
};
// Debug00236B10; original quickspline.cpp. Lazy shared original matrix stores.
void InitBasisMatricies(::port::driving::Memory&);
class TrackedController : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //Debug00233E38, E-0099-W74.
 void SetCurrentRoute(std::uint32_t route,std::uint32_t point); //00233EC0,W122.
 void JustReset(); //00234040,W122; exact empty original body.
 // Debug00233D68/00233F10; original trackedcontroller.cpp.
 void Sub_00233D68(std::uint32_t car,const ::port::track::RouteData&);
 void Sub_00233DB8(int in_chrg); // Debug00233DB8 destructor.
 void FindWhereIAm(const ::port::track::RouteData&);
 void Think(float,const ::port::track::RouteData&); //00233DD0, E-0099-W60.
 std::uint32_t GetCurrentRoute()const; //00233EE0, actual manager table.
 std::uint32_t GetCurrentRoutePoint()const; //Debug00234118, E-0099-W37.
 float GetAmountOutOfBounds()const; //Debug00234048, original virtual query.
};
class RealDriver : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //Debug0013ACC0, E-0099-W74.
 // Debug0013AB98/0013B368; original realdriver.cpp.
 void Sub_0013AB98(std::uint32_t car);
 void Sub_0013AC60(int in_chrg); // Debug0013AC60; live player/view and joy-handler gaps stay explicit.
 void AddJoyHandlers(); //Debug0013AE88, E-0099-W77; actual raw callback registration.
 void RemoveJoyHandlers(); // Debug0013B058; empty handler branch reconstructed.
 void ResetJoyValues();
};
class RealController : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //Debug002342A8, E-0099-W74.
 // Debug00234120; original realcontroller.cpp.
 void Sub_00234120(std::uint32_t car,const ::port::track::RouteData&);
 void Sub_00234220(int in_chrg); // Debug00234220 destructor.
 unsigned PlayerDriveLineNumber()const; //00234380, E-0099-W60.
 void Think(float,const ::port::race::AiDriveTargetServices&); //00234798.
 bool CalculateLateralPostionAndVelocityWRTDrivingLine(float&,float&,float,std::uint32_t route,std::uint32_t point); //002349D8.
 void RouteDeltaPredict(std::uint32_t target,float lateral_velocity,float lateral_position,std::uint32_t route,std::uint32_t point,std::uint32_t port_stack_vector_lane); //00234398.
};
class AICarController : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //Debug0018E430, E-0099-W74.
 // Debug0018E148/0018FC58; original aicarcontroller.cpp.
 void Sub_0018E148(std::uint32_t car,const ::port::track::RouteData&);
 void Sub_0018E398(int in_chrg); // Debug0018E398; exact JustDrive/base action dtor dispatch.
 void ReleaseBoost(); //00191458,E-0099-W121; aicarcontroller.cpp.
 void ClearStickyDecisions();
 int GetStickyIndex()const; //Debug0018FCD0, E-0099-W37.
 float GetAmountOutOfBounds()const; //Debug0018E798, E-0099-W37.
 void SteerStraightAhead(); //Debug001906B8, E-0099-W47.
 int GetMyDriverNumber()const; //0018E8B8, W47.
 void DoStickyDecisions(const ::port::race::AiDriveTargetServices&); //0018FAA8.
 void GetCurrentTrackRoutePoint(std::uint32_t car_to_follow,const ::port::race::AiDriveTargetServices&); //00190090.
 void GeneratePointsToFulfillAction(std::uint32_t action,const ::port::race::AiDriveTargetServices&); //001904D8.
 void ChooseNewAction(std::uint32_t pp_action,int forced_type); //001905F0, instruction-proven empty base, W90.
 std::uint32_t InterruptAction(const ::port::race::AiDriveTargetServices&); //001905F8, W90.
 void CalculateDistanceAndPositionBoost(std::uint32_t mine,std::uint32_t player,float& distance,float& position,const ::port::race::AiDriveTargetServices&); //00190AB0, W92.
 void MaybeHonkAndOrBlinkHeadlights(std::uint32_t car,std::uint32_t nearby); //00190DE0,W93.
 void DecideBoostFactor(const ::port::race::AiDriveTargetServices&); //00190760; E-0099-W92 full source boost/catchup.
 void ExecuteCurrentAction(const ::port::race::AiDriveTargetServices&); //001910B0.
 bool WeNeedTheBelt()const; //0018EAD8, W47.
 bool WeCanHaveTheBelt()const; //0018EAE8, W47.
 // Debug0018F0E0, W47. Prestart/countdown and first-action active chain;
 // existing-action selection/interrupt and active boost/Belt retain explicit gaps.
 void Think(float,const ::port::race::AiDriveTargetServices&,::port::race::PhysicsConstructionServices&);
 bool SignPostDecision(std::uint32_t sign,int& line,std::uint32_t location,const ::port::race::AiDriveTargetServices&); //0018FD20
 bool GetNewDrivingLine(std::uint32_t route,std::uint32_t point,bool relaxed,int& line,std::uint32_t location,const ::port::race::AiDriveTargetServices&); //001902F8
 void SetCurrentRoute(std::uint32_t route,std::uint32_t point,const ::port::race::AiDriveTargetServices&); //0018E650
 std::uint32_t GetCurrentRoute(const ::port::race::AiDriveTargetServices&); //0018E6E8
 std::uint32_t GetCurrentRoutePoint(const ::port::race::AiDriveTargetServices&); //0018E740
 void FindWhereIAm(const ::port::race::AiDriveTargetServices&); //00190D48
 void JustReset(const ::port::race::AiDriveTargetServices&); //0018E830,E-0110-T01.
 void FindClosestTrackRoute(int first,int last,const ::port::race::AiDriveTargetServices&); //0018E8C8
 void SetToPhysicsMode(std::uint32_t,::port::race::PhysicsConstructionServices&); //Debug00191328
};
class PsychoDriver : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //Debug001398B8, E-0099-W74.
 // Debug001396C0/00139A38/00139AC0; original psychodriver.cpp.
 void Sub_001396C0(std::uint32_t car);
 void Sub_00139828(int in_chrg); // Debug00139828 destructor.
 void StopHonkHorn(); // Debug0013A468.
 void StartHonkHorn(); //Debug0013A448, E-0099-W39.
 bool IsHonking()const; //Debug0013A488, E-0099-W39.
 float GetDesiredSpeed()const; // Debug0013AB00, E-0099-W34.
 std::uint32_t GetSteerToPoint()const{return address_+0x234;} //Debug0013AB10,E-0110-F06, exact original inline pointer.
 void ZeroSteeringMemory();
 void ZeroThrottleMemory();
 void SetDesiredSpeed(float speed,float seconds,bool allowed); //Debug001398A8, E-0099-W47.
 void SetLowGear(); //0013AAE8,E-0110-P01.
 std::uint16_t GetError(float& body,float& heading); //00139978
 int GetMovementIndex()const; //00139AF0
 void CalcSteerToPoint(); //0013A288
 void CalcThrottleAndBrake(float& throttle,float& brake); //0013A398
 void ClearSteeringMemory(); //00139A78
 void ClearThrottleMemory(); //00139AD0
 void Drive(float seconds); //00139B58
 void SetSteerToPoint(const std::array<float,2>&); //0013AB18, W47.
   void SetTarget(std::uint32_t target); //0013A230, W47.
 int GetDriverNumber()const; //0013AA90,E-0099-W95.
 float GetDesiredTimeToSpeed()const; //0013AAF8,W102.
};
void Sub_0013A978(::port::driving::Memory&); //original PID/steering table static initializer, W47.
class AIDriver : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //Debug00130258, E-0099-W74.
 // Debug00130118; original aidriver.cpp.
 void Sub_00130118(std::uint32_t car,const ::port::track::RouteData&);
 void Sub_001301E0(int in_chrg); // Debug001301E0 destructor.
 void GetRouteNumbersToUse(int& first,int& last)const; //Debug001309D8, E-0099-W37.
 void Think(float,const ::port::race::AiDriveTargetServices&,::port::race::PhysicsConstructionServices&); //001302D8, W47.
 bool ShouldWeAvoidThisCar(std::uint32_t car,float impact,const ::port::race::AiDriveTargetServices&); //001308F8,W93.
 void ChooseNewAction(std::uint32_t pp_action,int forced_type,const ::port::race::AiDriveTargetServices&); //00130388, W90.
};
}

namespace hp2::debug {
// Debug0019F560/0019F6A8, E-0099-W91. Exact copdriver.cpp definition owners.
class CopDriver:public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_0019F560(std::uint32_t,const ::port::track::RouteData&);
 void Sub_0019F6A8(int);
 void GetPursuitTargetPoint(std::array<float,2>&,const ::port::race::AiDriveTargetServices&,std::optional<std::uint32_t> target_point_w={}); //0019F708,E-0112-COP01.
 void Think(float,const ::port::track::RouteData&,::port::race::PhysicsConstructionServices* =nullptr,const ::port::race::AiDriveTargetServices* =nullptr); //Debug0019F980, E-0099-W108.
 bool AreWeStuck(float); //001A02F0,E-0110-P01.
 void DoSnapshot(std::uint32_t); //Debug001A03B8, E-0099-W101.
};
}

// Original inline method bodies follow their exact recovered header owner.
#include "game_core/indep/inc/PsychoDriver.hpp"
