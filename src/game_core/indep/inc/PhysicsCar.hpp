#pragma once
// TASK-0044 E-0044-P01/P02, Debug NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Original class/methods, explicit port memory and external-service binding; no host ABI claim.
#include "game_core/port/driving/object_view.hpp"
namespace port::driving::native {struct InitServices;struct DrivingServices;}
namespace hp2::recovered {
float YawFrictionBoost(port::driving::Memory&,float,float,float,float,float,int,float&,float);
// Debug imported enum; E-0044-P01, original names and values.
enum DampingSuitability {DS_NO_DAMP=0,DS_DAMP=1,DS_DOUSE=2};
class PhysicsCar : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Initialize(std::uint32_t,std::uint32_t,std::uint32_t,port::driving::native::InitServices&);
 void Calibrate();void InitializeStateFromCar();DampingSuitability CheckDampingSuitability();
 void BottomOut(port::driving::native::InitServices&);void ComputeAuxiliaries(port::driving::native::InitServices&);
 float Move(float,port::driving::native::DrivingServices&);
 void SetBoost(float);void SetFrictionBoost(float);float GetBrakeSettings(std::int32_t);
 void JumpStabilizer();void IterateForces();void ResolveForces();void UpdateCar(float);
};
}
