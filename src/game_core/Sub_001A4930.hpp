#pragma once
#include "port/driving/memory.hpp"
#include "Sub_00138F38.hpp"
#include "Sub_00152070.hpp"
#include <cstdint>
namespace port::driving {
// TASK-0038; address-anchored ordinary finite PS2 arithmetic projections.
// SLUS_203.62 revision unknown, SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Original field meanings/source paths unknown. Globals/tables reside in Memory;
// no host sine/asin approximation or invented stock tire parameters are used.
// ELF0025A540/A5E0/A748/A890; E-0038-T01..T04.
float Sub_0025A540(Memory&,std::uint16_t);
float Sub_0025A5E0(Memory&,float);
std::uint16_t Sub_0025A748(Memory&,float);
std::int16_t Sub_0025A890(Memory&,float,float);
// ELF0025AB50/AA78/A300; E-0038-T14. Original fixed-point heading route.
std::int32_t Sub_0025A300(std::int32_t,std::int32_t);
std::uint16_t Sub_0025AA78(Memory&,std::int32_t);
std::int16_t Sub_0025AB50(Memory&,std::int32_t,std::int32_t);
// ELF001A46B0/001A63C0; E-0038-T15. Derivative buffer callbacks.
std::uint32_t Sub_001A46B0(Memory&,std::uint32_t,std::uint32_t);
std::uint32_t Sub_001A63C0(Memory&,std::uint32_t,std::uint32_t);
// ELF0025AFA8; E-0038-T17. Original mutable math-global initializer.
void Sub_0025AFA8(Memory&);
// ELF001A57E0; E-0038-T16. Required original static tire table initialization.
void Sub_001A57E0(Memory&);
// ELF001A4818/4D28/4930/4EE0; E-0038-T05..T08.
float Sub_001A4818(Memory&,std::uint32_t);
float Sub_001A4D28(Memory&,float,float,std::uint32_t);
void Sub_001A4930(Memory&,std::uint32_t,std::int32_t,std::int32_t);
void Sub_001A4EE0(Memory&,std::uint32_t,std::int32_t);
// ELF0011B248,0011B6E0,001C0D78; E-0038-T09..T11.
void Sub_0011B248(SolverMatrix&,const SolverMatrix&,const SolverMatrix&);
float Sub_0011B6E0(Memory&,float);
void Sub_001C0D78(Memory&,std::uint32_t,std::int32_t);
// ELF001A4580/45A8; E-0038-T13. Original memory-pointer signatures.
float Sub_001A4580(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,ContactProvider&);
float Sub_001A45A8(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,ContactProvider&);
// ELF001A4170; E-0038-T12. f12/f13/f14, A0/A1/A2/A3 actual ABI.
void Sub_001A4170(Memory&,float,float,float,std::uint32_t,std::uint32_t,
                 std::uint32_t,std::uint16_t,ContactProvider&);
}
