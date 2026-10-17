// TASK-0041 E-0041-V01; retail 0x00138E08: PhysicsCar::ResolveForces(void).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#pragma once
// TASK-0038 E-0038-B09..B13: native adapters over one authoritative logical state.
#include "Sub_00137100_runtime.hpp"
#include "Sub_0019D4D0.hpp"
#include "Sub_00137D90.hpp"
namespace port::driving::native {
void Sub_0011BD90(Memory&,std::uint32_t);
void Sub_001ADD70(Memory&,std::uint32_t,std::uint32_t);
void Sub_00139518(Memory&,std::uint32_t);
void Sub_00138F38(Memory&,std::uint32_t);
void ResolveForces(Memory&,std::uint32_t);
void Sub_00139188(Memory&,std::uint32_t,float);
void integrate(Memory&,std::uint32_t,float,InitServices&);
}
