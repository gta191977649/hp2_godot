#pragma once
// TASK-0038, E-0038-E01..E06. Port-side logical-address projection, not original ABI.
// SLUS_203.62 executable revision unknown; ELF SHA-256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
#include "game_core/port/driving/memory.hpp"
#include <cstdint>
namespace port::driving {
// Port-side finite CVT.W.S projection (truncate). Exceptional operands throw.
std::int32_t cvt_w_s(float);
// ELF00189168/00189120/0018F808; E-0038-E01. Original table+0 pointer/+4 count.
void Sub_00189168(Memory&,std::uint32_t);
void Sub_00189120(Memory&,std::uint32_t,float,float);
void Sub_0018F808(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,float,float);
// ELF0018F838; E-0038-E01, bounded ordinary finite scalar interpolation.
float Sub_0018F838(Memory&,std::uint32_t,float);
// ELF0018F9F0; E-0038-E01, irregular x/y pairs.
float Sub_0018F9F0(Memory&,std::uint32_t,float);
// ELF0018AC38; E-0038-E02, direct logical addresses replace stack table carrier.
void Sub_0018AC38(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,float);
// ELF0018A7E8/0018AE70; E-0038-E03, preserve fields not written by constructors.
void Sub_0018A7E8(Memory&,std::uint32_t,std::uint32_t,std::uint32_t);
void Sub_0018AE70(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
// ELF0018AA28/0018AAB0/0018A988/0018A9D8; E-0038-E04.
float Sub_0018AA28(Memory&,std::uint32_t);
float Sub_0018AAB0(Memory&,std::uint32_t);
void Sub_0018A988(Memory&,std::uint32_t);
std::uint32_t Sub_0018A9D8(Memory&,std::uint32_t,std::uint32_t);
// ELF0018B008/0018B0D8/0018B178/0018BC18/0018BC68/0018B4A0; E-0038-E05.
float Sub_0018B008(Memory&,std::uint32_t);
float Sub_0018B0D8(Memory&,std::uint32_t);
void Sub_0018B178(Memory&,std::uint32_t,float);
std::uint32_t Sub_0018BC18(Memory&,std::uint32_t);
std::uint32_t Sub_0018BC68(Memory&,std::uint32_t);
void Sub_0018B4A0(Memory&,std::uint32_t);
// ELF0018B698; E-0038-E05, state-indexed derivative scalar.
std::uint32_t Sub_0018B698(Memory&,std::uint32_t,std::uint32_t);
// ELF0018C368/0018C1C8; E-0038-E06, exact constant-return leaves.
std::uint32_t Sub_0018C368();
std::uint32_t Sub_0018C1C8();
}
