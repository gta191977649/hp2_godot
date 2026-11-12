// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/Car.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/race/snapshot_services.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): Car::SetMovementMode() [0x002AE518]
// Diagnostic text (not a callable signature): Car::Move() [0x002AE5A0]
// Diagnostic text (not a callable signature): Car::TriggerCarTerrainEffects() [0x002AE690]
#include "game_core/port/driving/object_view.hpp"
#include <string>
#include <optional>
namespace port::driving::native {struct InitServices;}
namespace port::driving {struct ContactProvider;struct PlayerServices;}
namespace port::driving {struct TerrainEffectServices;}
namespace port::driving {struct CarEffectServices;}
namespace port::race {struct CarConstructionServices;}
namespace port::race {struct PhysicsConstructionServices;}
namespace port::race {struct CarResetServices;}
namespace port::race {struct AiDriveTargetServices;}
namespace port::track {struct RouteData;}
namespace hp2::recovered {
class Car : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 // Debug00130B90/00130FF8, TASK-0099 E-0099-W32. Explicit original lifetime;
 // this host memory view constructor never substitutes for these operations.
 void Sub_00130B90(std::uint32_t driver,std::uint32_t start_coordinate,std::uint16_t start_angle,const ::port::race::CarConstructionServices&);
 void Sub_00130FF8(int in_chrg);
 // Debug001314F0/001316F8, E-0099-W36. Original mode replacement;
 // unresolved movement families retain their original-address exception.
 void SetControlMode(int,const ::port::track::RouteData&);
 void SetMovementMode(int,::port::race::PhysicsConstructionServices&);
 void SetToPhysicsMode(std::uint32_t collision_car,::port::race::PhysicsConstructionServices&); //Debug001318E0
 std::array<float,2> WorldToCar(const std::array<float,2>&) const; //Debug00191F28, E-0099-W47, inline emission aicarcontroller.cpp.
 void Think(float,const ::port::race::AiDriveTargetServices&,::port::race::PhysicsConstructionServices&); //Debug00131B10, E-0099-W48.
 void Drive(float,::port::driving::PlayerServices* =nullptr); //Debug00131B60, E-0099-W48.
 float GetForwardAcceleration() const; //Debug00135AF8, E-0099-W47.
 float CalculateDraftFactor() const; //Debug00131A80, E-0099-W48.
 float CalculateDraftFactor(std::uint32_t other) const; //Debug00131958.
 float Move(float,::port::race::PhysicsConstructionServices&); //Debug00131B90, E-0099-W43.
 void StartForceImpact(std::uint32_t impulse,float magnitude); //Debug00134E90, E-0099-W46.
 void MaybeCameraShake(std::optional<std::uint32_t> port_stack_shake_w=std::nullopt); //Debug00134708, E-0099-W58.
 void TriggerEvents(std::optional<std::uint32_t> port_stack_vector_lane=std::nullopt); //Debug00133628, E-0099-W64.
 void EventHandler(std::uint32_t event); //Debug00133980, E-0099-W64; instruction-proven empty.
 void ComputeFakeRPM(); //Debug00132BB0,E-0099-W106.
 void ChangeGear(int); //Debug00132B28.
 void UpdatePosition(std::uint32_t); //Debug00131D58.
 void CalculateNewCatchupMultipliers(); //Debug001351F8.
 std::uint32_t GetLastPenetrationTime() const; //Debug001351E8.
 // Debug001332C8, E-0099-W91. MIPS LW return (not decompiler low-byte bool).
 std::uint32_t IsPoofed()const;
 void FixBlownTires(); //00135178,E-0099-W121; actual car.cpp owner.
 void StartBlinking(float); //Debug00133180, E-0099-W70.
 void BlinkBlinkPoof(); //Debug00133148, E-0099-W70.
 void DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& ={}); //Debug00131108, E-0099-W74; capture/known descriptor tree.
 void FakeEngineTask(); //Debug00132B20, E-0099-W67; JR RA/NOP.
 void StopBlinking(); // Debug00133190, E-0099-W32.
 // Debug001334F0/00133558; TASK-0099 E-0099-W28. Actual character artwork owner.
 void AssignAnimatedDriver(const std::string& car_base_name);
 void UnAssignAnimatedDriver();
 // Debug00131D78, E-0099-W39: actual constructed shared topology/cache.
 // Legacy FinishMovement without RouteData retains its explicit resource gap.
 void BeginMovement(float);
 void FinishMovement(float);
 //Debug00131E58, E-0099-W39: actual shared path/cache and headlight/horn;
 // W59 binds ordinary Player/periodic replay consumers; reached jump/zone
 // callees retain their original-address boundaries.
 void FinishMovement(float,const ::port::track::RouteData&,const ::port::race::PhysicsConstructionServices* =nullptr);
 float AverageAir(float& seconds,float& highest,const ::port::race::PhysicsConstructionServices* =nullptr); //00135B88,E-0100-AA01.
 void HonkAndOrBlinkHeadlights(float on,float off,bool blink,bool honk,bool twice); //00134198,E-0099-W93.
 void UpdateHeadlightsAndHorn(); //Debug001341F8.
 float GetAmountOutOfBounds()const; //Debug00133370, E-0099-W37; Null/AI targets supported.
 void GetCarSpaceBoundingBox(std::uint32_t min,std::uint32_t max)const; //Debug001351C8.
 std::uint32_t GetRoutePoint(const ::port::race::AiDriveTargetServices&)const; //Debug00133340.
 std::uint32_t GetRoute(const ::port::race::AiDriveTargetServices&)const; //Debug00133310,E-0110-F04; actual AI location services.
 std::uint32_t GetRoute()const; //Debug00133310, W69; Null/Tracked targets.
 std::uint32_t GetRoutePoint()const; //Debug00133340, W69; Null/Tracked targets.
 // Debug 00133988; E-0075-T01. Wheel loop recovered; full remainder throws.
 void TriggerCarTerrainEffects(float,port::driving::TerrainEffectServices&);
 //Debug00133988, E-0099-W68. Actual shared terrain/cache/emitter/resource path.
 void TriggerCarTerrainEffects(float distance,std::optional<std::uint32_t> port_stack_vector_lane=std::nullopt);
 void TurnWipersOnForDuration(int start_time,int end_time); //00133590, W68.
 // Debug 0011E4E0; E-0075-C01. Host service binding; wheel effects only.
 void TriggerEffect(int,float,int,const std::array<float,4>*,port::driving::CarEffectServices&);
 // Debug0011E4E0, E-0099-W38. Actual shared emitter/position lists and AcidFX call.
 void TriggerEffect(int,float,int,std::uint32_t override_velocity=0);
 void TriggerEffect(int,float,int,std::uint32_t override_velocity,std::optional<std::uint32_t> port_stack_vector_lane); //same original0011E4E0, explicit port rawFX policy.
 // Debug00132788; E-0048-D03. Host bindings; original five topology probes,
 // wheel3 ride height and quantized rotations, no whole Car construction claim.
 std::uint16_t GetSteering() const; //Debug00132BA0, E-0099-W111; actual car.cpp definition.
 void FindFlatOnTrackPosition(std::array<float,16>&,std::array<float,4>&,std::uint16_t,port::driving::ContactProvider&);
 // Debug001329C0; TASK-0099 E-0099-W26. Actual shared state placement,
 // original five topology probes plus GeometryPosition = Position - CoG.
 void PutFlatOnTrack(port::driving::ContactProvider&);
 void ResetCore(std::uint32_t newpos,float speed,std::uint16_t angle,const ::port::race::CarResetServices&); //00132DF0,W122.
 void ResetToPosition(std::uint32_t,float,std::uint16_t,const ::port::race::CarResetServices&); //00132F48,E-0110-F07/T01; exact Tracked/AI virtual domains.
 void ResetToRoutePoint(std::uint32_t route,std::uint32_t point,float speed,bool blinking,float direction,const ::port::race::CarResetServices&); //00132FB0,W122.
 void SwitchSkin(unsigned); //0011E7B0,E-0110-T01.
 bool IsCarInFront(std::uint32_t,std::uint16_t); //00133900,E-0100-PA01.
 // Debug0011DEA8/0011E4A8, E-0099-W25. Concrete shared resource path.
 void InitRenderInfo(std::uint32_t ride_info);void CloseRenderInfo();
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00133070; E-0042-U013; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00133070.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void PleaseResetMeLater(port::driving::native::InitServices&);
    void PleaseResetMeLater(); //Debug001330D8, E-0099-W47; actual shared pursuit lookup.
    // ELF 0x00133130; E-0042-U014; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00133130.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    bool IsBlinking();
    // ELF 0x00135E18; E-0042-C008; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00135e18.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    bool ShouldVisualEffectsBeGenerated();
    // ELF 0x00135EB8; E-0042-C009; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00135eb8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    float GetPostCollisionYawControlBoost();

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // Debug001316D8 E-0099-W47 /00132A10 E-0099-W48; actual signatures
    // replace historical Retail void/no-argument gap declarations.
    void SetFrictionBoost(float);
    std::uint32_t CalculateDebugChecksum() const;
    // Replaces historical Retail00134DB8 void gap; fresh Debug float signature.
    float GetWheelTraction(); //Debug00134E10 E-0099-W66.
    // Replaces historical Retail00134E00 void gap; fresh Debug float signature.
    float GetWheelsOnGround(); //Debug00134E58 E-0099-W66.
    void MaybeRumble(); //Debug001343D0.
    void DoWheelForces(); //Debug00134730.
    void CalcForceParameters(int port); //Debug00134750.
};
} // namespace hp2::recovered

namespace hp2::debug {void Sub_00136628(::port::driving::Memory&); /* E-0099-W42, original static initializer. */}
