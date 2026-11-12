#pragma once
// Port-owned binding of loaded resources to Debug World construction. These
// callbacks are whole required callees, not optional effects or default no-ops.
#include "game_core/TrafficTeleporter.hpp"
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/track/route_data.hpp"
#include <functional>

namespace port::race {
struct SmokeableResources;
struct CopResources;
using TextureInfoLookup=std::function<std::uint32_t(std::uint32_t,int,int)>;
using EmitterGroupLookup=std::function<void(::port::driving::Memory&,std::uint32_t,std::uint32_t)>;
// Required original loaded-data and speech callees. No fallback constructs
// empty roadblocks/radar traps or silently suppresses original speech RNG.
struct CopConstructionServices {
    const CopResources* resources=nullptr;
    std::function<void(::port::driving::Memory&,std::uint32_t)> allocate_voices;
    std::function<void(::port::driving::Memory&,std::uint32_t)> flush_speech_queue;
};
struct WorldConstructionServices {
    // Actual loaded route/path/signpost ownership, never synthetic route counts.
    const port::track::RouteData* routes=nullptr;
    TextureInfoLookup texture_info;
    EmitterGroupLookup emitter_group;
    const SmokeableResources* smokeables=nullptr;
    CopConstructionServices cops;
};
}

namespace hp2::debug {
// Debug0011DDB8/001A8C98/001A8D20/001A8798/001D6698/0021CE28;
// E-0099-W06. Implementations retain their original source-file ownership.
void CloseCarEffects(::port::driving::Memory&); // Debug0011DE78, E-0099-W18
void CloseClans(::port::driving::Memory&); // Debug001A8D20, constructor-empty branch
void CloseSkids(::port::driving::Memory&); // Debug001A8850
void Sub_001D70D0(::port::driving::Memory&); // Actual fern process-static initializer
void CloseFernShakers(::port::driving::Memory&); // Debug001D6E38
void CloseFernFlatteners(::port::driving::Memory&); // Debug001D66A0, constructor-empty branch
void CloseTerrainEffectEmitters(::port::driving::Memory&); // Debug0021CE68
void InitCarEffects(::port::driving::Memory&);
void InitClans(::port::driving::Memory&);
// Debug001A90F0, E-0099-W06; not implicit in InitClans or World.
void Sub_001A90F0(::port::driving::Memory&);
void InitSkids(::port::driving::Memory&,const ::port::race::TextureInfoLookup&);
void InitFernFlatteners();
// Debug001D6D30; E-0099-W06. Actual original two-info initialization.
void InitFernShakers(::port::driving::Memory&,const ::port::race::EmitterGroupLookup&);
void InitTerrainEffectEmitters(::port::driving::Memory&,const ::port::race::EmitterGroupLookup&);
// Debug00222490, E-0099-W11. Original nMaxCops guard and manager ownership.
void InitCops(::port::driving::Memory&,const ::port::race::CopConstructionServices&);
// Debug002224D8, E-0099-W17. Original null guard and virtual-dtor ownership.
void CleanupCops(::port::driving::Memory&);
}
