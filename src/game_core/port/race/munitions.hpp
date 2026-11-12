#pragma once
// Port bindings for original Debug munition initialization; no native PS2 ABI.
#include "game_core/port/race/world_construction.hpp"
#include "game_core/indep/inc/Munitions.hpp"

namespace hp2::debug {
// Debug00219890, E-0099-W09. Stores original creator identity in shared memory.
// Never converted to a host executable pointer. Concrete instance creation
// still needs its recovered factory/constructor implementation.
void RegisterSmokeableCreator(::port::driving::Memory&,SmokeableType,std::uint32_t);
// Debug001BCDC0, E-0099-W09. Original process-static emitter-list construction.
// Call during fresh runtime static initialization, not on every race restart.
void Sub_001BCDC0(::port::driving::Memory&);
// Debug001BB7A0, E-0099-W09; real handler and ordered resource initialization.
void InitMunitions(::port::driving::Memory&,const ::port::race::EmitterGroupLookup&);
// Debug001BB7E8, E-0099-W12. Supports actual idle-handler teardown. A nonempty
// dynamic effect list still throws at001BCA98 rather than dropping live effects.
void CloseMunitions(::port::driving::Memory&);
}

namespace hp2::debug {
// TASK-0099 W79. Actual original creator signatures over shared logical Memory.
std::uint32_t CreateSmokeable(::port::driving::Memory&,SmokeableType,int id,std::uint32_t info,std::uint32_t model_hash); //002198A8.
std::uint32_t BasicSmokeableCreator(::port::driving::Memory&,int,std::uint32_t,std::uint32_t); //002198E0.
std::uint32_t RocketSmokeableCreator(::port::driving::Memory&,int,std::uint32_t,std::uint32_t); //001BB620.
std::uint32_t SmokeBombSmokeableCreator(::port::driving::Memory&,int,std::uint32_t,std::uint32_t); //001BB6A0.
std::uint32_t DroppedSpikesSmokeableCreator(::port::driving::Memory&,int,std::uint32_t,std::uint32_t); //001BB720.

}
