// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/AcidFX.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/track/particles.hpp"
#include <functional>
#include "game_core/port/driving/projections.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AcidEffectSystem::DoTimeStep() [0x002AD370]
// Diagnostic text (not a callable signature): AcidEffectSystem::RenderEffects [0x002AD390]
namespace hp2::recovered {
class AcidEffectSystem {
public:
    // TASK0052 Retail00128718/Debug00128870 active-particle integration island.
    // Emitter triggering/list scheduling remains a separate original branch.
    static void DoTimeStep(std::vector<port::track::Particle>&,const port::track::ParticleDefinition&,float,const std::array<float,3>& live_inherited_velocity);

    // TASK0052 Retail00128718 / Debug00128870 auto-emitter scheduling branch.
    // Trigger callback is the original TriggerEmitter boundary, invoked before cycle transition.
    static void DoTimeStep(port::track::ParticleEmitter&,const port::track::ParticleDefinition&,float,const std::function<void(float)>& trigger);

    // Retail00127700 / Debug00127858. Pool/list allocation is supplied by the host;
    // capacity is the remaining shared original1000-slot particle pool, never per emitter.
    static void TriggerEmitter(port::driving::Memory&,port::track::ParticleEmitter&,const port::track::ParticleDefinition&,float,const std::array<std::array<float,3>,3>& interest_points,std::uint32_t& global_seed,std::vector<port::track::Particle>& group,std::size_t available_slots,bool group_available);

    // Retail001274A0 / Debug001275F8. Host callback receives true for head,
    // false for tail; native intrusive storage is not reinterpreted.
    static void AddAutoEmitter(port::track::ParticleEmitter&,const port::track::ParticleDefinition*,std::uint32_t& global_seed,const std::function<void(bool)>& insert);

    // Retail00128DA0 / Debug00128EF8, static WorldEffects groups only.
    static std::vector<port::track::ParticleDraw> RenderEffects(port::driving::Memory&,const port::track::ParticleSimulation&,const port::track::ParticleView&);

    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00127418; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00127418.md.
    // Original Debug reference: AcidEffectSystem::AcidEffectSystem(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00127418();
    // ELF 0x00127688; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00127688.md.
    // Original Debug reference: AcidEffectSystem::RemoveAutoEmitter(AcidEmitter *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void RemoveAutoEmitter();
};
} // namespace hp2::recovered
