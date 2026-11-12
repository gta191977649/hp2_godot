#pragma once
// TASK-0099 E-0099-W35. Port bindings, not a native PS2 ABI.
// Debug NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/race/control_construction.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/indep/inc/TrafficCar.hpp"
namespace port::race {
// Port-side W111/W114 final-consumer projection boundary, not an EE register/FCSR model.
void traffic_normalize_selected_xyz(std::array<float,4>&,float);
}
namespace hp2::debug {
void Sub_00216250(::port::driving::Memory&); // original static graph initialization
void Sub_001674A8(::port::driving::Memory&); //E-0099-W111 original TrafficAccelerationTable CRT.

class TrafficDriver : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_0018A4A8(std::uint32_t car,const ::port::track::RouteData&);
 void Sub_0018A540(int);
 void DoSnapshot(std::uint32_t); //Debug0018A5A0, E-0099-W104.
 void Think(float,const ::port::race::AiDriveTargetServices&,::port::race::PhysicsConstructionServices&); //0018A600 W103.
 void ExecuteCurrentAction(const ::port::race::AiDriveTargetServices&); //0018ACC0 W103.
 void CalculateDesiredSpeed(float&,float&); //0018AD90 W103.
 void JustReset(const ::port::race::AiDriveTargetServices&); //0018AC38,E-0110-T01.

 void GetRouteNumbersToUse(int& lower_bound,int& upper_bound); //Debug0018AC10, E-0099-W35
};
class Mellovator : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //Debug00214FE8, E-0099-W74.
 void Sub_00214E78(std::uint32_t config,std::uint32_t body,std::uint32_t coordinate);
 void Sub_00216400(int);
 void ComputeAuxiliaries(); //002154D8
 void ComputeSuspensionForces(); //00215720
 void GetGroundHeightAndWheelPos(std::array<std::array<float,4>,4>&,std::array<float,4>&); //002159E8
 float BottomOut(std::uint32_t state); //002160A8
 void ResolveForces(); //00215AE8
 void Calibrate(float); //002151D0
 void SetState(float,float,float,float); //00215248
 void GetDerivative(std::uint32_t output); //00215290, E-0099-W45.
};
class MellowMover : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& ={}); //Debug002144E8, E-0099-W74.
 static std::uint32_t Sub_00216500(::port::driving::Memory&); // original operator new
 void Sub_00214248(std::uint32_t car,std::uint32_t config,std::uint32_t driver,std::uint32_t state,::port::race::PhysicsConstructionServices&);
 void Sub_00214410(int);
 void ResolveForces(const ::port::race::PhysicsConstructionServices* =nullptr); //00214B60
 void JumpStabilizer(std::optional<std::uint32_t> up_w=std::nullopt); //00214C50,E-0110-M01.
 void Calibrate(); //00214598
 void InitializeStateFromCar(::port::race::PhysicsConstructionServices&); //002145E8
 void ComputeAuxiliaries(::port::race::PhysicsConstructionServices&); //00214AE8
 float Move(float,::port::race::PhysicsConstructionServices&); //00214958, E-0099-W45.
 void SetBoost(float); //00214BA8, E-0099-W45.
 void UpdateCar(float,::port::race::PhysicsConstructionServices&); //00214680, E-0099-W45.
};
}
