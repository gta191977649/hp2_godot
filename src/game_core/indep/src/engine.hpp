#pragma once
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/driving/projections.hpp"
// TASK-0044 E-0044-E01. Behavior: NFSHP2.ELF Debug SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// ObjectView is a port binding, not original host ABI/layout.
namespace hp2::recovered {
using port::driving::Memory;
using port::driving::DrivetrainProjection;
using port::driving::DrivetrainGlobals;
using port::driving::EngineEvaluation;
using port::driving::EngineTimerProjection;
void CalculateShiftPoints(Memory&,std::uint32_t config,std::uint32_t engineConfig,std::uint32_t up,std::uint32_t down,float detune); // 00188DE8
class Engine : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_00188998(std::uint32_t config,std::uint32_t car);
 void ComputeAuxiliaries(); //00188B38
 std::uint32_t GetDerivative(std::uint32_t out); //00188B88
 float GetTorque(); //00188BD8
 float GetEngineTorque(float rpm); //00188D20
 float GetTotalEngineTorque(); //00188C60
 void DecrementNitro(); //00188DA0
 // Debug0018A348..0018A4A0; E-0044-E01 full entry listings.
 void SetThrottleOverride(float value);void ApplyTorque(float value);
 float GetNitroBoostTimer();void SetNitroBoostTimer(float value);
 void SetNitroFlag(std::int32_t value);std::int32_t GetNitroFlag();std::uint32_t GetEI();
 float GetMaxRPM();float GetRedLineRPM();float GetIdleRPM();float GetEffectiveThrottle();float GetThrottle();
 float GetTorqueResponse();float GetAngularAcceleration();float GetAngularVelocity();
 void SetIsHP(std::int32_t value);void SetState(float value);void SetState(std::uint32_t input);
 void GetState(std::uint32_t out);float GetState();std::int32_t GetSize();
 void SetCatchupTorqueMultiplier(float value);void SetThrottle(float value);
 static void DecrementNitro(EngineTimerProjection&,std::int32_t ticks,float scale);
};
class DriveTrain : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_00189020(std::uint32_t config,std::uint32_t engine,std::uint32_t wheels,std::uint32_t car);
 float GetDifferentialAngularVelocity(); //001891B0
 float GetDriveWheelSlippage(); //00189280
 void SetDifferentialAngularVelocity(float); //00189320
 float GetDifferentialAngularAcceleration(); //00189508
 void ComputeAuxiliaries(); //00189648
 std::uint32_t GetDerivative(std::uint32_t out); //00189840
 void ApplyClutchTorque(float); //00189870
 void GuessClutchTorque(); //00189AF0
 void ModifyClutchTorque(); //00189BD0
 void FinalizeClutchTorque(); //00189D40
 std::uint32_t Idling(); //00189D48
 std::uint32_t ClutchMatched(); //00189D98
 std::int32_t SetSportGear(std::int32_t); //00189DD0
 std::int32_t SetDesiredGear(std::int32_t); //00189E10
 void PrintCurrentState(); //0018A160, original empty entry
 // Debug0018A1B0..0018A2F0; E-0044-E01.
 std::int32_t RearWheelDrive();std::int32_t FrontWheelDrive();std::int32_t GetTopGear();
 std::int32_t GetDesiredGear();std::int32_t GetPreviousGear();std::int32_t GetSportGear();std::int32_t GetCurrentGear();
 std::int32_t IsClutchEngaged();std::uint32_t GetEngine();std::uint32_t GetDriveTrainInfo();
 float GetShiftDownRPM(std::int32_t gear);float GetShiftUpRPM(std::int32_t gear);std::int32_t GetCurrentClutchState();
 void SetState(std::int32_t gear);void SetState(std::uint32_t input);void GetState(std::uint32_t out);float GetState();std::int32_t GetSize();
 static std::int32_t SetSportGear(DrivetrainProjection&,std::int32_t);
 static std::int32_t SetDesiredGear(DrivetrainProjection&,std::int32_t,const DrivetrainGlobals&);
 static float GetDifferentialAngularAcceleration(const DrivetrainProjection&,const DrivetrainGlobals&);
 static void ApplyClutchTorque(float,DrivetrainProjection&,const DrivetrainGlobals&);
 static void GuessClutchTorque(DrivetrainProjection&,const DrivetrainGlobals&,const EngineEvaluation&);
 static void ModifyClutchTorque(DrivetrainProjection&,const DrivetrainGlobals&,const EngineEvaluation&);
};
}
