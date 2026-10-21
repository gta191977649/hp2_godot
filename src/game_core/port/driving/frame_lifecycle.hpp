#pragma once
// Port-side slice bindings, not whole original Car entry points or topology services.
// E-0081-L01; reverse/task0081/lifecycle.md. Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "memory.hpp"
namespace port::driving {
// BeginMovement00131D78 motion capture plus BeginTimestep001BD938. Topology
// interest/LastInTroughPosition belong to the still-untranslated full entry.
void begin_movement_state(Memory&,std::uint32_t car,float timestep);
// FinishMovement00131E58 prefix through old-position publication, before path,
// recovery, headlights/horn and Player consumers. Called once after collisions.
void finish_movement_state(Memory&,std::uint32_t car,float timestep);
}
