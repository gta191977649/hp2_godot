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
// Debug001BD868; source wheel/audio reset only, not a vehicle position reset.
void CarState::ResetCarForSound(){auto& m=m_;const auto a=address_;m.scalar(a+0x390,1000);for(unsigned i=0;i<4;++i){for(auto offset:{0x2b0u,0x2c0u,0x2d0u,0x2f0u,0x300u,0x354u})m.word(a+offset+4*i,0);m.byte(a+0x310+i,1);m.word(a+0x334+8*i+4,0);m.word(a+0x334+8*i,0);m.word(a+0x314+4*i,1);m.word(a+0x324+4*i,1);}m.word(a+0x378,0);m.word(a+0x37c,4);m.word(a+0x384,0);m.word(a+0x370,0);m.word(a+0x374,0);}

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
    const auto phase=static_cast<std::uint16_t>(::port::driving::ee_cvt_w_s_finite(m.scalar(car+offset+0x2c0)*m.scalar(0x00325920),"0x001BDEB0"));
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

// Debug NFSHP2.ELF SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
namespace hp2::recovered {
// Debug001BD2C0 CarState constructor; TASK-0099 E-0099-W22.
// Original ../indep/src/carstate.cpp; Memory view lifetime is port-side.
void CarState::Sub_001BD2C0(std::uint32_t topology_coordinate,std::uint32_t pInfo){
 auto& m=m_;const auto s=address_;
 // Inlined default vector/matrix constructors execute empty loops: no padding initialization.
 m.word(s+0x368,0);m.word(s+0x36c,0);
 m.word(s+0x20,0);m.word(s+0x28,0);m.word(s+0x24,0);
 m.word(s+0x10,0);m.word(s+0x39c,0);
 m.word(s+4,topology_coordinate);m.word(s,pInfo);
 m.word(s+0x18,0);m.word(s+0x14,0);
 ResetCar();
}
}

namespace hp2::recovered {
// Debug001BE008; E-0099-W45. Fresh00254278 listing loads homogeneous base
// 00317B20; original complete XYZW position subtraction is preserved.
void CarState::CalculateGeometryPosition(){auto& m=m_;const auto s=address_;port::driving::SolverVector transformed{};bMulMatrix(transformed,m.matrix(s+0x50),m.vector(m.word(s)+0x110),{m.vector(0x317b20)});auto value=m.vector(s+0x20);for(unsigned i=0;i<4;++i)value[i]=value[i]-transformed[i];m.vector(s+0x10,value);}
// Debug001BDA20; E-0099-W47. Cached source relative-coordinate matrix;
// both source branches run the original orthonormalizer before publishing valid.
std::uint32_t CarState::GetTransformMatrix(){auto& m=m_;const auto s=address_,matrix=s+0x3c0;if(!m.word(s+0x400)){auto velocity=m.vector(s+0x30);if(m.scalar(s+0x380)<0)for(unsigned i=0;i<3;++i)velocity[i]=-velocity[i];auto value=bIdentity();const float square=(velocity[0]*velocity[0]+velocity[1]*velocity[1])+velocity[2]*velocity[2];if(square>1.f){value[0]=velocity[1];value[1]=-velocity[0];value[4]=velocity[0];value[5]=velocity[1];}else{const auto angle=m.half(s+0x364);const float cosine=hp2::recovered::bCos(m,angle),sine=hp2::recovered::bSin(m,angle);value[0]=sine;value[1]=-cosine;value[4]=cosine;value[5]=sine;}m.matrix(matrix,value);eOrthonormalizeMatrix(m,matrix);m.word(s+0x400,1);}return matrix;}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::recovered {
// Debug001BD388; TASK-0099 E-0099-W74. Raw capture 001bd388.json/type-CarState.json.
void CarState::DoSnapshot(std::uint32_t snapshot){
 auto& m=m_;const auto a=address_;hp2::debug::ReplaySnapshot s(m,snapshot);s.BeginSection(0x2af050);
 for(unsigned off:{0x10u,0x20u,0x30u,0x40u})s.MoveData(a+off,12);
 s.MoveData(::port::race::SnapshotMatrix{a+0x50},64);
 s.MoveData(a+0x90,12);s.MoveData(a+0xa0,12);
 for(unsigned i=0;i<4;++i){const auto p=a+0x2b0+4*i;
  s.MoveData(p,4);s.MoveData(p+16,4);s.MoveData(p+32,4);s.MoveData(p+64,4);
  s.MoveData(a+0x310+i,1);s.MoveData(p+100,1);s.MoveData(a+0x334+8*i,8);
  s.MoveData(p+164,4);s.MoveData(p+80,4);
  if(m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d))m.word(a+0x324+4*i,1);
 }
 s.MoveData(a+0x364,2);s.MoveData(a+0x366,2);s.MoveData(a+0x368,4);
 const bool taking=m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d);
 if(taking)for(unsigned off:{0x370u,0x374u,0x378u})m.word(a+off,0);
 s.MoveData(a+0x380,4);s.MoveData(a+0x384,4);s.MoveData(a+0x388,1);
 s.MoveData(a+0x38c,4);s.MoveData(a+0x390,4);s.MoveData(a+0x398,1,4,1);
 s.MoveData(a+0x39c,1,4,1);s.MoveData(a+0x3a0,4);s.MoveData(a+0x3a4,4,4,1);
 s.MoveData(a+0x3a8,4);s.MoveData(a+0x3ac,4);
 if(taking){
  m.matrix(a+0x3c0,hp2::recovered::bIdentity());m.word(a+0x400,0);m.word(a+0x37c,0);
  for(unsigned i=0;i<4;++i)m.word(a+0x37c,m.word(a+0x37c)+static_cast<std::uint32_t>(m.signed_byte(a+0x310+i)));
  CalculateWheelMatrices();
 }
 s.EndSection();
}
}
