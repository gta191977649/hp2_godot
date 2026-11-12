// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/race/world_construction.hpp"
namespace hp2::debug {
class TerrainEffectEmitters {
public:
    // Debug0021CB40, E-0099-W06. Exact seven-by-thirty-three original lists;
    // emitter lookup binds real loaded resources and is required when named.
    TerrainEffectEmitters(::port::driving::Memory&,std::uint32_t,const ::port::race::EmitterGroupLookup&);
    // Debug0021CD30, E-0099-W38. Shared original emitter list/state binding.
    static void TriggerEffect(::port::driving::Memory&,std::uint32_t object,int effect,int terrain,float intensity,std::uint32_t position,std::uint32_t velocity);
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class TerrainEffectEmitters {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered
