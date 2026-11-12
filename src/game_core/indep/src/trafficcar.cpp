// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trafficcar.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrafficCar::Move() [0x002B2178]
#include "game_core/indep/inc/TrafficCar.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00167A08; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00167a08.md.
void TrafficCar::Sub_00167A08() {
    hp2::not_implemented("0x00167A08", "TrafficCar::Sub_00167A08 (signature and full body unresolved)");
}

// ELF 0x00167A60; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00167a60.md.
void TrafficCar::Sub_00167A60() {
    hp2::not_implemented("0x00167A60", "TrafficCar::Sub_00167A60 (signature and full body unresolved)");
}

// ELF 0x00167AF8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00167af8.md.
void TrafficCar::Sub_00167AF8() {
    hp2::not_implemented("0x00167AF8", "TrafficCar::Sub_00167AF8 (signature and full body unresolved)");
}

// ELF 0x00168088; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00168088.md.
void TrafficCar::Sub_00168088() {
    hp2::not_implemented("0x00168088", "TrafficCar::Sub_00168088 (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/race/traffic_construction.hpp"
#include "game_core/indep/inc/CarMover.hpp"
namespace hp2::debug {
// Debug00166ED0, E-0099-W35; original trafficcar.cpp. Driver remains untouched.
void TrafficCar::Sub_00166ED0(std::uint32_t car){
 m_.word(address_+0xc,m_.word(car+0x60));m_.word(address_+0x10,m_.word(car+0x64));
 m_.word(address_+4,0x2aa000);m_.word(address_+8,car);
 m_.scalar(address_+0x14,static_cast<float>(m_.half(car+0x3a4))*m_.scalar(0x325298));
 m_.word(address_+0x1c,0);m_.word(address_+0x18,m_.word(car+0x3c4));m_.word(address_+0x20,0);
}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug00166F28, E-0099-W105; exact original trafficcar.cpp definition owner.
void TrafficCar::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2a9fd8);s.MoveData(address_+0xc,8);for(unsigned off:{0x14u,0x18u,0x1cu,0x20u})s.MoveData(address_+off,4);CarMover(m_,address_).DoSnapshot(snapshot);s.EndSection();}
}

#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/port/driving/projection_support.hpp"
#include <cmath>
#include <cstring>
#include <limits>
namespace port::race {
// Port-only scalar domain boundary. This is not an EE FCSR/extended-float model.
static bool traffic_normal_or_zero(float x){return std::isfinite(x)&&(x==0||std::isnormal(x));}
static float traffic_div(float x,float y,const char* instruction){
 if(!traffic_normal_or_zero(x)||!std::isnormal(y))hp2::not_implemented(instruction,"TrafficCar EE DIV operands outside supported normal/nonzero denominator domain");
 const float result=x/y;
 if(!traffic_normal_or_zero(result))hp2::not_implemented(instruction,"TrafficCar EE DIV result outside supported normal/zero domain");
 return result;
}
// Exact final-consumer projection of original251D28 only in this caller.
// For +0 length and signed-zero XYZ: SQRT(+0)/+0 = Sony +Fmax;
// reciprocal underflows +0; final XYZ preserve signed-zero bits. W is not written.
// No claim about the intermediate raw Fmax, FCSR, or general abnormal domains.
void traffic_normalize_selected_xyz(std::array<float,4>& v,float length){
 std::uint32_t bits;std::memcpy(&bits,&length,4);
 if(bits==0&&v[0]==0&&v[1]==0&&v[2]==0)return;
 // E-0099-W114: separately proved positive-finite SQRT / +0 domain.
 // Component margins keep all nonzero squares and their sum far from EE
 // under/overflow despite unmodelled normal-arithmetic least-bit differences.
 if(bits==0){
  bool bounded=true;
  for(unsigned i=0;i<3;++i){const float magnitude=std::fabs(v[i]);if(v[i]!=0&&(!std::isnormal(v[i])||magnitude<0x1p-40f||0x1p40f<magnitude))bounded=false;}
  if(bounded){
   const float square=(v[0]*v[0]+v[1]*v[1])+v[2]*v[2];
   if(std::isnormal(square)&&0x1p-100f<square&&square<0x1p100f){
    // DIV gives positive Fmax; reciprocal underflows +0; finite XYZ * +0.
    // Only the exact final consumer bits are projected; W remains untouched.
    for(unsigned i=0;i<3;++i)v[i]=std::copysign(0.f,v[i]);
    return;
   }
  }
 }
 for(unsigned i=0;i<3;++i)if(!traffic_normal_or_zero(v[i]))hp2::not_implemented("0x00251D28","Traffic normalization extended/abnormal XYZ domain");
 const float x=v[0]*v[0],y=v[1]*v[1],z=v[2]*v[2];
 const float sum=(x+y)+z;
 if(!traffic_normal_or_zero(x)||!traffic_normal_or_zero(y)||!traffic_normal_or_zero(z)||!traffic_normal_or_zero(sum)||!std::isnormal(length))hp2::not_implemented("0x00251D28","Traffic normalization unresolved zero-length or extended intermediate domain");
 const float divisor=traffic_div(std::sqrt(sum),length,"0x00251D28");
 if(divisor==0){v[0]=length;v[2]=0;v[1]=0;return;}
 const float inverse=traffic_div(1,divisor,"0x00251D28");
 v[2]=v[2]*inverse;v[0]=v[0]*inverse;v[1]=v[1]*inverse;
}
}
namespace hp2::debug {
// Debug001674A8, E-0099-W111. Exact original trafficcar.cpp static initializer.
void Sub_001674A8(::port::driving::Memory& m){
 const float maximum=m.scalar(0x3252c0);
 hp2::recovered::Table(m,0x2f25d0).Sub_0018D810(0x2f25c0,4,0,maximum);
 hp2::recovered::Table(m,0x2f25f8).Sub_0018D810(0x2f25e8,4,0,maximum);
 hp2::recovered::Table(m,0x2f2708).Sub_0018D810(0x2f2610,0x3d,0,maximum);
}
// Debug00166FC0, E-0099-W111; fresh owner/body/MIPS in traffic-move.
// Native finite-normal arithmetic scope, explicit source-unwritten stack W policy.
float TrafficCar::Move(float t,::port::race::PhysicsConstructionServices& services){
 auto& m=m_;using hp2::recovered::Car;using port::race::traffic_div;
 const auto car=m.word(address_+8),info=m.word(car+0xc);Car vehicle(m,car);
 const float distance=m.scalar(address_+0x18)*t,rear=-m.scalar(info+0x180);
 const float wheelbase=m.scalar(info+0x120)+rear;
 const float fx=port::driving::bSin(m,m.scalar(address_+0x14)+m.scalar(0x32529c));
 const float fy=port::driving::bSin(m,m.scalar(address_+0x14));
 float turn=float(vehicle.GetSteering())*m.scalar(0x3252a0);
 if(m.scalar(0x3252a4)<turn)turn=turn-m.scalar(0x3252a8);
 const float limit=wheelbase*m.scalar(0x3252ac);
 if(limit<=turn||turn<=-limit){
  const float radius=traffic_div(wheelbase,turn,limit<=turn?"0x001670A4":"0x001670FC");
  const float rearx=m.scalar(address_+0xc)+fx*(-rear),reary=m.scalar(address_+0x10)+fy*(-rear);
  const float da=traffic_div(distance,radius,"0x00167124");
  const float cx=rearx+(-fy)*radius,cy=reary+fx*radius;
  const float co=port::driving::bSin(m,da+m.scalar(0x3252b0)),si=port::driving::bSin(m,da);
  const float x=m.scalar(address_+0xc)-cx,y=m.scalar(address_+0x10)-cy;
  m.scalar(address_+0xc,cx+(co*x-si*y));m.scalar(address_+0x10,cy+(si*x+co*y));
  m.scalar(address_+0x14,m.scalar(address_+0x14)+da);
 }else{
  m.scalar(address_+0xc,m.scalar(address_+0xc)+fx*distance);m.scalar(address_+0x10,m.scalar(address_+0x10)+fy*distance);
 }
 const auto driver=m.word(address_);float throttle=m.scalar(driver+0x214);
 const float acceleration=hp2::recovered::Table(m,0x2f25d0).GetValue(m.scalar(address_+0x18));throttle=t*throttle;
 const float brake=m.scalar(driver+0x218),braking=hp2::recovered::Table(m,0x2f25f8).GetValue(m.scalar(address_+0x18));
 const float next=m.scalar(address_+0x18)+(throttle*acceleration-(t*brake)*braking);
 m.scalar(address_+0x18,next);
 if(next<0)m.scalar(address_+0x18,0);else if(m.scalar(0x3252b4)<next)m.word(address_+0x18,m.word(0x3252b4));
 const auto old=m.vector(car+0x60);
 if(!services.port_stack_vector_lane)hp2::not_implemented("0x00167258","TrafficCar source position+3C and angular+8C stack W lanes require explicit port policy");
 float lane;const auto raw=*services.port_stack_vector_lane;std::memcpy(&lane,&raw,4);
 m.vector(car+0x60,{{m.scalar(address_+0xc),m.scalar(address_+0x10),m.scalar(car+0x68),lane}});
 if(!services.contact)hp2::not_implemented("0x001329C0","TrafficCar PutFlatOnTrack requires actual contact context");
 vehicle.PutFlatOnTrack(*services.contact);
 auto velocity=m.vector(car+0x60);for(unsigned i=0;i<4;++i)velocity[i]=velocity[i]-old[i];
 port::race::traffic_normalize_selected_xyz(velocity,m.scalar(address_+0x18));m.vector(car+0x70,velocity);
 const float xx=velocity[0]*velocity[0],yy=velocity[1]*velocity[1],zz=velocity[2]*velocity[2];
 m.scalar(car+0x3c4,std::sqrt((xx+yy)+zz));m.word(car+0x3c0,m.word(address_+0x18));m.word(car+0x440,0);
 const auto previous=m.half(car+0x3a4);const float phase=m.scalar(address_+0x14)*m.scalar(0x3252b8);
 if(!std::isfinite(phase)||phase>=2147483648.f||phase< -2147483648.f)hp2::not_implemented("0x00167300","TrafficCar EE angle conversion outside supported signed range");
 const auto angle=static_cast<std::uint16_t>(static_cast<std::int32_t>(phase));m.half(car+0x3a4,angle);
 const auto delta=static_cast<std::int16_t>(static_cast<std::uint16_t>(angle-previous));
 const float az=traffic_div(float(delta)*m.scalar(0x3252bc),t,"0x00167330");m.vector(car+0xd0,{{0,0,az,lane}});
 m.half(car+0x3a6,vehicle.GetSteering());hp2::recovered::CarState state(m,car+0x40);
 for(unsigned wheel=0;wheel<4;++wheel){
  const auto offset=wheel*4;const float av=traffic_div(m.scalar(address_+0x18),m.scalar(info+0x12c+wheel*0x20),"0x00167374");
  m.scalar(car+0x2f0+offset,av);m.scalar(car+0x300+offset,m.scalar(car+0x300+offset)+av*t);m.word(car+0x340+offset,0);
  state.SetWheelTouchingGround(wheel,1);state.SetWheelTerrain(wheel,1);
  m.word(car+0x374+wheel*8,0);m.word(car+0x378+wheel*8,0);m.word(car+0x394+offset,0);m.word(car+0x310+offset,0);
  for(int i=3;i>=0;--i)m.word(car+0x330+4*i,0);
 }
 state.CalculateWheelMatrices();state.CalculateGeometryPosition();vehicle.ComputeFakeRPM();return t;
}
}
