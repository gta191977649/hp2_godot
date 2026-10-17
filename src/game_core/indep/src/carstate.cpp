
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001C08D0; E-0042-C136; E-0042-S001; exact Debug TU ..\indep\src\carstate.cpp
namespace port::driving::native {
float GetYawAlways(Memory& m,std::uint32_t s) {
    using namespace detail_group_00137d90;
    const auto angle=port::driving::bATan(m,m.scalar(s+0x30),m.scalar(s+0x34));
    const auto delta=static_cast<std::uint16_t>(m.half(s+0x364)-static_cast<std::uint16_t>(angle));
    float result=static_cast<float>(delta)*g(m,0x8f58);
    if(g(m,0x8f5c)<result)result=result-g(m,0x8f60);
    return result;
}
}

// Retail 0x001C0888; E-0042-C135; E-0042-S001; exact Debug TU ..\indep\src\carstate.cpp
namespace port::driving::native {
float GetYaw(Memory& m,std::uint32_t s){
    using namespace detail_group_00137d90;return m.scalar(s+0x380)<5.0f?0.0f:GetYawAlways(m,s);}
}

// Retail 0x001C0D78; E-0042-U144; E-0042-S001; exact Debug TU ..\indep\src\carstate.cpp
namespace port::driving {
void CalculateWheelMatrices(Memory&m,std::uint32_t car,std::int32_t index){
    using namespace detail_group_001a4930;
    const auto offset=static_cast<std::uint32_t>(index)*4;
    SolverMatrix first{};first[0]=first[5]=first[10]=first[15]=1;auto second=first;
    const auto phase=static_cast<std::uint16_t>(cvt_w_s(m.scalar(car+offset+0x2c0)*g(m,-0x709c)));
    const auto ry=Sub_0011B950(m,phase);eMulMatrix(first,first,ry);
    if(index<2){const auto rz=Sub_0011B7D8(m,m.half(car+0x366));eMulMatrix(first,first,rz);eMulMatrix(second,second,rz);}
    const auto location=m.word(car)+static_cast<std::uint32_t>(index)*0x20+0x120;
    auto translation=m.vector(location);translation[3]=1;
    translation[2]=translation[2]+m.scalar(car+offset+0x300);
    translation[1]=translation[1]+m.scalar(location+0x18);
    for(unsigned i=0;i<4;++i){first[12+i]=translation[i];second[12+i]=translation[i];}
    m.matrix(car+static_cast<std::uint32_t>(index)*0x40+0xb0,first);
    m.matrix(car+static_cast<std::uint32_t>(index)*0x40+0x1b0,second);
}
}

#include "game_core/indep/src/carstate.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001C0780; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c0780.md.
void CarState::ResetCarForSound() {
    hp2::not_implemented("0x001C0780", "CarState::ResetCarForSound (signature and full body unresolved)");
}

// ELF 0x001C0850; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c0850.md.
void CarState::BeginTimestep() {
    hp2::not_implemented("0x001C0850", "CarState::BeginTimestep (signature and full body unresolved)");
}

// ELF 0x001C0888; E-0042-C135; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c0888.md.
void CarState::GetYaw() {
    hp2::not_implemented("0x001C0888", "CarState::GetYaw (signature and full body unresolved)");
}

// ELF 0x001C08D0; E-0042-C136; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c08d0.md.
void CarState::GetYawAlways() {
    hp2::not_implemented("0x001C08D0", "CarState::GetYawAlways (signature and full body unresolved)");
}

// ELF 0x001C0CF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c0cf8.md.
void CarState::SetWheelTouchingGround() {
    hp2::not_implemented("0x001C0CF8", "CarState::SetWheelTouchingGround (signature and full body unresolved)");
}

// ELF 0x001C0D48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c0d48.md.
void CarState::SetWheelTerrain() {
    hp2::not_implemented("0x001C0D48", "CarState::SetWheelTerrain (signature and full body unresolved)");
}

// ELF 0x001C0D78; E-0042-U144; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/001c0d78.md.
void CarState::CalculateWheelMatrices() {
    hp2::not_implemented("0x001C0D78", "CarState::CalculateWheelMatrices (signature and full body unresolved)");
}
} // namespace hp2::recovered
