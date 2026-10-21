#pragma once
// TASK-0075 E-0075-T01. Port-side callback and bounded wheel-only projection.
// This interface does not claim to run the remaining drive-over, water, damage,
// nitro or blown-tire branches of Debug Car::TriggerCarTerrainEffects.
#include "memory.hpp"
#include <functional>

namespace port::driving {
struct TerrainEffectServices {
    // Raw Debug CarEffect integer, intensity, packed (wheel+5)/terrain parameter,
    // and optional world-space velocity. The pointer is valid during the call.
    std::function<void(std::uint32_t, std::int32_t, float, std::int32_t,
                       const std::array<float,4>*)> trigger;
};
// Debug 00133988 wheel loop 00133A20..00133CB4 only; E-0075-T01.
void trigger_wheel_terrain_effects(Memory&,std::uint32_t,float,TerrainEffectServices&);
// Debug00133988 body DriveOver1/2 slice; E0089W01/W05.
void trigger_drive_over_effects(Memory&,std::uint32_t,unsigned,TerrainEffectServices&);
}
