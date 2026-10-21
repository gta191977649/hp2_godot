#include "game_core/indep/src/carstate.hpp"
#include "game_core/src/bMath.hpp"

// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001C08D0; E-0042-C136; E-0042-S001; exact Debug TU ..\indep\src\carstate.cpp
namespace port::driving::native {
float GetYawAlways(Memory& m,std::uint32_t s) {return hp2::recovered::CarState(m,s).GetYawAlways();}
}

// Retail 0x001C0888; E-0042-C135; E-0042-S001; exact Debug TU ..\indep\src\carstate.cpp
namespace port::driving::native {
float GetYaw(Memory& m,std::uint32_t s){return hp2::recovered::CarState(m,s).GetYaw();}
}

// Retail 0x001C0D78; E-0042-U144; E-0042-S001; exact Debug TU ..\indep\src\carstate.cpp
namespace port::driving {
void CalculateWheelMatrices(Memory&m,std::uint32_t car,std::int32_t index){hp2::recovered::CarState(m,car).CalculateWheelMatrices(index);}
}

#include "game_core/indep/src/carstate.hpp"
#include "game_core/src/bMath.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001C0780; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c0780.md.
void CarState::ResetCarForSound() {
    hp2::not_implemented("0x001C0780", "CarState::ResetCarForSound (signature and full body unresolved)");
}

// Debug001BD938; E-0079-S01; reverse/task0079/driving-chain.md.
// NFSHP2.ELF SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
void CarState::BeginTimestep() {
    // Original descending stores: CrossedWheelTerrain[3..0] = ROAD (1).
    for (std::int32_t i=3;i>=0;--i)
        m_.word(address_+0x324+static_cast<std::uint32_t>(i)*4,1);
}

// ELF 0x001C0888; E-0042-C135; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c0888.md.


// ELF 0x001C08D0; E-0042-C136; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c08d0.md.


// ELF 0x001C0CF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c0cf8.md.


// ELF 0x001C0D48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c0d48.md.


// ELF 0x001C0D78; E-0042-U144; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/001c0d78.md.

} // namespace hp2::recovered

// TASK-0044 E-0044-P02, canonical Debug CarState members; source ../indep/src/carstate.cpp.
namespace hp2::recovered {
// Debug001BD6C0; E-0081-L01; reverse/task0081/lifecycle.md. Full reset leaf.
void CarState::ResetCar(){
    const auto s=address_;auto& m=m_;
    for(auto offset:{0x30u,0x40u})
        for(auto lane:{0u,2u,1u})m.word(s+offset+4*lane,0);
    m.matrix(s+0x50,hp2::recovered::bIdentity());
    for(auto lane:{0u,2u,1u})m.word(s+0x90+4*lane,0);
    for(unsigned lane=0;lane<3;++lane)m.word(s+0xa0+4*lane,0);
    m.half(s+0x364,0);m.half(s+0x366,0);
    for(unsigned i=0;i<4;++i){
        for(auto offset:{0x2b0u,0x2c0u,0x2d0u,0x2e0u,0x2f0u})
            m.word(s+offset+4*i,0);
        m.byte(s+0x310+i,1);
        m.word(s+0x334+8*i,0);m.word(s+0x338+8*i,0);
        m.word(s+0x314+4*i,1);m.word(s+0x354+4*i,0);
        m.word(s+0x324+4*i,1);m.word(s+0x300+4*i,0);
    }
    m.word(s+0x37c,4);
    CalculateWheelMatrices();
    for(auto offset:{0x370u,0x374u,0x378u,0x380u,0x384u,0x38cu,0x3a4u,0x3a8u})
        m.word(s+offset,0);
    m.scalar(s+0x390,1000);m.scalar(s+0x394,-1);m.word(s+0x398,1);
    m.scalar(s+0x3ac,1);m.word(s+0x388,1);m.scalar(s+0x3a0,1);
    for(unsigned offset=0x3c0;offset<0x400;++offset)m.byte(s+offset,0);
    m.word(s+0x400,0);
}
// Debug001BD970.
float CarState::GetYaw(){return m_.scalar(address_+0x380)<5.0f?0.0f:GetYawAlways();}
// Debug001BD9B8.
float CarState::GetYawAlways(){const auto angle=port::driving::bATan(m_,m_.scalar(address_+0x30),m_.scalar(address_+0x34));const auto delta=static_cast<std::uint16_t>(m_.half(address_+0x364)-static_cast<std::uint16_t>(angle));float value=static_cast<float>(delta)*m_.scalar(0x00325914);if(m_.scalar(0x00325918)<value)value=value-m_.scalar(0x0032591c);return value;}
// Debug001BDE60. Brake translation deliberately retains original wheel position.
void CarState::CalculateWheelMatrices(std::int32_t index){auto& m=m_;const auto car=address_;using namespace port::driving;

    using namespace detail_group_001a4930;
    const auto offset=static_cast<std::uint32_t>(index)*4;
    SolverMatrix first{};first[0]=first[5]=first[10]=first[15]=1;auto second=first;
    const auto phase=static_cast<std::uint16_t>(cvt_w_s(m.scalar(car+offset+0x2c0)*m.scalar(0x00325920)));
    hp2::recovered::eRotateY(m,first,first,phase);
    if(index<2){hp2::recovered::eRotateZ(m,first,first,m.half(car+0x366));hp2::recovered::eRotateZ(m,second,second,m.half(car+0x366));}
    const auto location=m.word(car)+static_cast<std::uint32_t>(index)*0x20+0x120;
    auto translation=m.vector(location);translation[3]=1;
    translation[2]=translation[2]+m.scalar(car+offset+0x300);
    translation[1]=translation[1]+m.scalar(location+0x18);
    for(unsigned i=0;i<4;++i){first[12+i]=translation[i];second[12+i]=m.scalar(location+4*i);}
    second[15]=1;
    m.matrix(car+static_cast<std::uint32_t>(index)*0x40+0xb0,first);
    m.matrix(car+static_cast<std::uint32_t>(index)*0x40+0x1b0,second);

}
// Debug001BDFB8.
void CarState::CalculateWheelMatrices(){for(std::int32_t i=0;i<4;++i)CalculateWheelMatrices(i);}
// Debug001BDDE0; E-0044-P02, original signed byte transition contract.
void CarState::SetWheelTouchingGround(std::int32_t i,char value){const auto old=m_.byte(address_+0x310+i);m_.byte(address_+0x310+i,static_cast<std::uint8_t>(value));if(!old&&value)m_.word(address_+0x37c,m_.word(address_+0x37c)+1u);else if(old&&!value)m_.word(address_+0x37c,m_.word(address_+0x37c)-1u);}
// Debug001BDE30; E-0044-P02.
void CarState::SetWheelTerrain(std::int32_t i,std::int32_t value){m_.word(address_+0x314+4*i,static_cast<std::uint32_t>(value));if(m_.signed_word(address_+0x324+4*i)<value)m_.word(address_+0x324+4*i,static_cast<std::uint32_t>(value));}
}
