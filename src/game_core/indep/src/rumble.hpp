// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class tEnvelope {
public:
    // ELF 0x00155948; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00155948.md.
    // Original Debug reference: tEnvelope::GetIndex(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetIndex();
    // ELF 0x001559A8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001559a8.md.
    // Original Debug reference: tEnvelope::OutOfRange(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void OutOfRange();
    // ELF 0x001559F0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001559f0.md.
    // Original Debug reference: tEnvelope::GetSlope(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetSlope();
};
} // namespace hp2::recovered
#include "game_core/port/driving/object_view.hpp"
#include <optional>
namespace hp2::debug {
void PortRumbleShockSway(::port::driving::Memory&,int,float); //00155C60 E-0099-W66.
void PortRumbleRoadNoise(::port::driving::Memory&,int,float); //00155CB8.
void PortRumbleShockBottom(::port::driving::Memory&,int,float); //00155D10.
void PortRumbleBlownTires(::port::driving::Memory&,int,float); //00155D30.
void PortRumbleSingleRoadBump(::port::driving::Memory&,int,float); //00155D50.
void PortRumbleDoubleRoadBump(::port::driving::Memory&,int,float); //00155D70.
class tEnvelope:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 int GetIndex(float); //00155500 E-0099-W50.
 bool OutOfRange(float); //00155560.
 float GetSlope(int); //001555A8.
 float GetValue(float); //00155658.
};
class tShaker:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void StartShaking(const std::array<float,4>&,float duration,float frequency); //00155E58.
 void Update(float); //00155E90.
 void GetValue(std::array<float,4>&); //00155EE0.
 float GetAmplitude(); //00155E20.
};
void Sub_00156478(::port::driving::Memory&); // E-0099-W50, actual complete rumble/shaker static initializer.
void UpdateCameraShakers(::port::driving::Memory&); //00155F70.
void GetShake(::port::driving::Memory&,int player,std::array<float,4>&); //00156048.
void ApplyCameraShake(::port::driving::Memory&,int view,std::array<float,16>&); //00156080.
void MaybeCameraShake(::port::driving::Memory&,int player,std::uint32_t acceleration,std::optional<std::uint32_t> port_stack_shake_w=std::nullopt); //001561E0 E-0099-W58; original unwritten W is an explicit port operand.
// TASK-0099 E-0099-W38; original indep/src/rumble.cpp. Memory views do not
// claim native ABI or external controller delivery from envelope queue state.
class tRumbler:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void StartRumble(std::uint32_t envelope,float amplitude); //00155820
};
void PortRumble(::port::driving::Memory&,int port,float intensity,std::uint32_t coefficients,std::uint32_t envelope,bool overflow); //001559F8
void PortRumbleImpact(::port::driving::Memory&,int port,float intensity); //00155C40
}
