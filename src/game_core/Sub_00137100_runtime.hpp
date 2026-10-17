#pragma once
#include "game_core/port/driving/memory.hpp"
#include "game_core/Sub_00152070.hpp"
#include <functional>
#include <optional>

namespace port::driving::native {
// Port services for two performance-counter calls and the external 00227438
// predicate. No absent callback silently succeeds. Original source path unknown.
struct InitServices {
    std::function<std::uint64_t()> Sub_00250F38;
    std::function<std::uint32_t(std::uint64_t,std::uint64_t)> Sub_00250F48;
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> Sub_00227438;
    std::function<void(Memory&,std::uint32_t)> ResolveForces;
    // Explicit projection of the original uninitialized temporary's fourth lane.
    std::optional<std::uint32_t> Field_00137938_stack_w;
    ContactProvider* contact=nullptr;
};
// SLUS_203.62 revision unknown; SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// E-0038-I14: full list append, signed virtual adjustor, known component counts.
void Sub_0019C238(Memory&,std::uint32_t,std::uint32_t);
// E-0038-I15: list reset dispatch and exact performance-counter call order.
void Sub_0019C808(Memory&,std::uint32_t,InitServices&);
// E-0038-I16: max.S f12,0 (decompiler int-cast is incorrect).
float Sub_001ADFB8(Memory&,std::uint32_t,float);
// E-0038-I17/I18: complete late constructor field/config synchronization.
void Sub_001375F0(Memory&,std::uint32_t);
void Sub_00138CF8(Memory&,std::uint32_t);
// E-0038-I19/I20: complete wheel/controller reset paths.
void Sub_001A4358(Memory&,std::uint32_t);
void Sub_001A5FC0(Memory&,std::uint32_t,ContactProvider&);
// E-0038-I21/I22: vector velocity helper and contact response.
std::array<float,4> Sub_001A63D0(const std::array<float,4>&,const std::array<float,4>&,const std::array<float,4>&);
float Sub_001A6408(Memory&,std::uint32_t);
// E-0038-I23/I24: predicate and three-way state classification.
bool Sub_00133130(Memory&,std::uint32_t);
// E-0038-I29: conditional external predicate and +0x858 store.
void Sub_00133070(Memory&,std::uint32_t,InitServices&);
std::uint32_t Sub_001377D8(Memory&,std::uint32_t);
// E-0038-I25/I26: post-reset state routes and final adjusted virtual dispatch.
void Sub_00137938(Memory&,std::uint32_t,InitServices&);
void Sub_00137788(Memory&,std::uint32_t,InitServices&);
// E-0038-I27: complete initialization call/store graph. Memory allocation is a
// port-owned logical arena; original pool allocator implementation is not claimed.
// All object/config/global/table dependencies must be mapped before entry.
std::uint32_t Sub_00137100(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,InitServices&);
}
