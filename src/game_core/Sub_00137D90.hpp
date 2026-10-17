// TASK-0041 E-0041-R01; retail 0x00137D90: PhysicsCar::Move(float).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#pragma once
// TASK-0038 logical-memory projections. Original source path unknown.
#include "game_core/port/driving/memory.hpp"
#include <functional>
#include <optional>
namespace port::driving::native {
// Explicit port-side services for external calls; missing required callbacks throw.
struct DrivingServices {
    std::function<std::uint64_t()> Sub_00250F38;
    std::function<std::uint32_t(std::uint64_t,std::uint64_t)> Sub_00250F48;
    std::function<void(Memory&,std::uint32_t,float)> Sub_0019C290;
    std::function<void(Memory&,std::uint32_t)> Sub_0011BD90;
    std::function<void(Memory&,std::uint32_t)> Sub_0019D980;
    std::function<void(Memory&,std::uint32_t)> Sub_0018C030;
    std::function<void(Memory&,std::uint32_t,float)> Sub_00139188;
    std::function<void(Memory&,std::uint32_t,std::uint32_t,float)> Sub_001A5520;
    std::function<float(Memory&,std::uint32_t)> Sub_001B50C8;
    std::optional<std::uint32_t> Field_001389A4_stack_w;
};
// SLUS_203.62 revision unknown; ELF SHA256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// ELF001A5D60; E-0038-E07, original16-bit control and two matrix stores.
void Sub_001A5D60(Memory&,std::uint32_t,std::uint16_t);
// ELF00139490; E-0038-E08, component scale publication.
void Sub_00139490(Memory&,std::uint32_t,float);
// ELF001C0888/001C08D0/00135EB8/00135E18; E-0038-E09.
float Sub_001C08D0(Memory&,std::uint32_t);
float Sub_001C0888(Memory&,std::uint32_t);
float Sub_00135EB8(Memory&,std::uint32_t);
bool Sub_00135E18(Memory&,std::uint32_t);
// ELF0025A3D0/A410/A4A8; E-0038-E11, original unsigned-modulo RNG chain.
std::uint32_t Sub_0025A3D0(Memory&,std::uint32_t,std::uint32_t);
float Sub_0025A410(Memory&,std::uint32_t,float);
float Sub_0025A4A8(Memory&,float);
// ELF00137D90; E-0038-E10/E13. car+20!=1; Race state4 requires A758==0.
// Unsupported original mode branches and absent services throw at their address.
float Move(Memory&,std::uint32_t,float,DrivingServices&);
}
