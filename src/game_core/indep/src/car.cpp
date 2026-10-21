#include "game_core/indep/inc/Car.hpp"
// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\car.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/NullController.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/driving/terrain_effects.hpp"
#include "game_core/port/driving/frame_lifecycle.hpp"
#include "game_core/indep/src/carstate.hpp"
#include <algorithm>
#include <cmath>

namespace port::driving {
// Debug00133988 body DriveOver slice; E0089W01/W05. Caller enforces >2
// grounded wheels and performs original body-position elevation query.
void trigger_drive_over_effects(Memory& m,std::uint32_t car,unsigned terrain,TerrainEffectServices& services){
 if(terrain>=33)hp2::not_implemented("0x00133988","Body terrain outside original table");
 const auto info=0x3102e8+terrain*0x90;
 const float speed=m.scalar(car+0x3c4)*m.scalar(0x324d2c);
 float amount[2]{};constexpr unsigned fields[]{0x3c,0x48};
 for(unsigned i=0;i<2;++i){const auto p=m.word(info+fields[i]);
  if(p&&m.byte(p))amount[i]=std::min(1.f,std::max(0.f,(speed-m.scalar(info+fields[i]+4))/(m.scalar(info+fields[i]+8)-m.scalar(info+fields[i]+4))));
 }
 const float excess=(amount[0]+amount[1])-1.f;
 if(excess>0){const unsigned i=m.scalar(info+0x40)<m.scalar(info+0x4c)?0:1;amount[i]=std::max(0.f,amount[i]-(excess+excess));}
 for(unsigned i=0;i<2;++i)if(amount[i]>0){
  if(!services.trigger)hp2::not_implemented("0x0011E4E0","DriveOver emitter service missing");
  services.trigger(car,int(i+1),amount[i],int(terrain),nullptr);
 }
}
// Debug00131D78 state slice; E-0081-L01, reverse/task0081/lifecycle.md.
void begin_movement_state(Memory& m,std::uint32_t car,float t){
    if(0.0f<t){
        m.copy(car+0x470,car+0x60,16);
        m.copy(car+0x480,car+0x70,16);
        m.copy(car+0x490,car+0xd0,16);
    }
    hp2::recovered::CarState(m,car+0x40).BeginTimestep();
}
// Debug00131E58..00131F80 state slice; E-0081-L01.
void finish_movement_state(Memory& m,std::uint32_t car,float t){
    if(0.0f<t){
        auto linear=m.vector(car+0x70),angular=m.vector(car+0xd0);
        const auto old_linear=m.vector(car+0x480),old_angular=m.vector(car+0x490);
        const float inverse=1.0f/t;
        for(unsigned i=0;i<4;++i){
            linear[i]=(linear[i]-old_linear[i])*inverse;
            angular[i]=(angular[i]-old_angular[i])*inverse;
        }
        m.vector(car+0x80,linear);m.vector(car+0xe0,angular);
    }
    const auto state=car+0x40;
    const float speed=m.scalar(state+0x384);
    // Preserve the listing's rejection comparisons, including unordered inputs.
    if(!(speed<0.0f)&&!(1000.0f<speed))
        m.scalar(state+0x368,m.scalar(state+0x368)+speed*t);
    const auto old=m.vector(car+0x470),position=m.vector(car+0x60);
    const float x=old[0]-position[0],y=old[1]-position[1],z=old[2]-position[2];
    float squared=x*x;squared=squared+y*y;squared=squared+z*z;
    m.scalar(state+0x36c,m.scalar(state+0x36c)+std::sqrt(squared));
    m.copy(car+0x470,car+0x60,16);
}
// Debug 00133988, wheel loop 00133A20..00133CB4; E-0075-T01,
// reverse/task0075/terrain-producer.md. Deliberately bounded host projection.
void trigger_wheel_terrain_effects(Memory& m,std::uint32_t car,float distance,TerrainEffectServices& services) {
    const auto emit=[&](std::int32_t effect,float intensity,std::int32_t parameter,const std::array<float,4>* velocity){
        if(!services.trigger)hp2::not_implemented("0x0011E4E0","Car::TriggerEffect service missing");
        services.trigger(car,effect,intensity,parameter,velocity);
    };
    const auto enabled=[&](std::uint32_t field){const auto p=m.word(field);return p!=0&&m.byte(p)!=0;};
    const auto clamp=[](float v){return std::min(1.0f,std::max(v,0.0f));};
    for(unsigned wheel=0;wheel<4;++wheel){
        if(!m.byte(car+0x350+wheel))continue; // Original airborne path preserves counter.
        const auto terrain=m.word(car+0x354+4*wheel);
        const auto info=0x003102e8+terrain*0x90;
        const auto counter=car+0x870+wheel;
        const float x=m.scalar(car+0x374+8*wheel),y=m.scalar(car+0x378+8*wheel);
        const float slip=std::sqrt(x*x+y*y);
        const auto parameter=static_cast<std::int32_t>(((wheel+5)<<16)+terrain);
        if(distance<150.0f&&m.scalar(info+0x58)<slip){
            const auto count=std::min<unsigned>(m.byte(counter)+1u,255u);m.byte(counter,static_cast<std::uint8_t>(count));
            if(enabled(info+0x54)){
                float intensity=(slip-m.scalar(info+0x58))/(m.scalar(info+0x5c)-m.scalar(info+0x58));
                if(count<2)intensity=0;
                else if(count<20)intensity*=0.25f;
                else if(count<60)intensity*=0.5f;
                else if(count<120)intensity*=0.75f;
                intensity=clamp(intensity);
                if(0<intensity)emit(3,intensity,parameter,nullptr);
            }
        }else m.byte(counter,0);
        if(distance<40.0f&&wheel>1&&m.scalar(info+0x64)<x&&enabled(info+0x60)){
            const float intensity=clamp((x-m.scalar(info+0x64))/(m.scalar(info+0x68)-m.scalar(info+0x64)));
            if(0<intensity){
                std::array<float,4> velocity{};
                hp2::recovered::bMulMatrix(velocity,m.matrix(car+0xf0+wheel*0x40),{{x,y,0,0}},SolverMathContext{m.vector(0x00317b20)});
                const auto body_velocity=m.vector(car+0x70);
                for(unsigned i=0;i<4;++i)velocity[i]+=body_velocity[i];
                emit(4,intensity,parameter,&velocity);
            }
        }
    }
}
}

namespace hp2::recovered {
// Debug00131D78; E-0081-L01; reverse/task0081/lifecycle.md. Partial full entry.
void Car::BeginMovement(float t){
    port::driving::begin_movement_state(m_,address_,t);
    hp2::not_implemented("0x00131D78","Car::BeginMovement topology interest and LastInTroughPosition services");
}
// Debug00131E58; E-0081-L01; reverse/task0081/lifecycle.md. Partial full entry.
void Car::FinishMovement(float t){
    port::driving::finish_movement_state(m_,address_,t);
    hp2::not_implemented("0x00131E58","Car::FinishMovement path/recovery, headlights/horn and Player consumers");
}
// Debug 00133988; E-0075-T01. Full entry cannot silently omit the remainder.
void Car::TriggerCarTerrainEffects(float distance,port::driving::TerrainEffectServices& services){
    port::driving::trigger_wheel_terrain_effects(m_,address_,distance,services);
    hp2::not_implemented("0x00133988","Car::TriggerCarTerrainEffects remaining drive-over/water/damage/nitro/flat-tire branches");
}
// Debug00132788; E-0048-D03. Exact five-point flat placement and wheel3 config selection.
void Car::FindFlatOnTrackPosition(std::array<float,16>& rotation,std::array<float,4>& position,std::uint16_t angle,port::driving::ContactProvider& contact){
    auto& m=m_;
    const std::array<std::array<float,2>,5> relative{{{{0,0}},{{1,0}},{{-1,0}},{{0,1}},{{0,-1}}}};
    port::driving::Memory::Scratch scratch(m);
    std::array<float,5> heights{};const auto temporary=m.allocate(0x10);
    for(unsigned i=0;i<5;++i){auto probe=position;probe[0]+=relative[i][0];probe[1]+=relative[i][1];m.vector(temporary,probe);heights[i]=contact.GetElevation(m,0,temporary,0,0,0);}
    const auto cfg=m.word(address()+0xc);const auto wheel=cfg+0x120+3*0x20;
    const float x=((heights[1]-heights[0])-(heights[2]-heights[0]))*0.5f*m.scalar(0x00324d14);
    const float y=((heights[3]-heights[0])-(heights[4]-heights[0]))*0.5f*m.scalar(0x00324d14);
    position[2]=((heights[0]+m.scalar(wheel+0xc))-m.scalar(wheel+8))+m.scalar(cfg+0x118);
    rotation=hp2::recovered::bIdentity();hp2::recovered::eRotateZ(m,rotation,rotation,angle);
    hp2::recovered::eRotateX(m,rotation,rotation,static_cast<std::uint16_t>(static_cast<int>(y)));
    hp2::recovered::eRotateY(m,rotation,rotation,static_cast<std::uint16_t>(-static_cast<std::int16_t>(static_cast<int>(x))));
}
}

namespace hp2::recovered {
// ELF 0x001C0090; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c0090.md.
void NullController::Sub_001C0090() {
    hp2::not_implemented("0x001C0090", "NullController::Sub_001C0090 (signature and full body unresolved)");
}

// ELF 0x001C00A8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c00a8.md.


// ELF 0x001C00D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c00d0.md.
void NullController::Sub_001C00D0() {
    hp2::not_implemented("0x001C00D0", "NullController::Sub_001C00D0 (signature and full body unresolved)");
}









// ELF 0x002A0550; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/002a0550.md.
void NullController::Sub_002A0550() {
    hp2::not_implemented("0x002A0550", "NullController::Sub_002A0550 (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/CarDriver.hpp"

namespace hp2::recovered {
// ELF 0x00136C00; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c00.md.
void CarDriver::GetSteerToPoint() {
    hp2::not_implemented("0x00136C00", "CarDriver::GetSteerToPoint (signature and full body unresolved)");
}

// ELF 0x00136C08; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c08.md.
void CarDriver::SetPhysicsMode() {
    hp2::not_implemented("0x00136C08", "CarDriver::SetPhysicsMode (signature and full body unresolved)");
}

// ELF 0x00136C10; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c10.md.
void CarDriver::GetDesiredTimeToSpeed() {
    hp2::not_implemented("0x00136C10", "CarDriver::GetDesiredTimeToSpeed (signature and full body unresolved)");
}

// ELF 0x00136C20; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c20.md.
void CarDriver::GetDesiredSpeed() {
    hp2::not_implemented("0x00136C20", "CarDriver::GetDesiredSpeed (signature and full body unresolved)");
}

// ELF 0x00136C30; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c30.md.
void CarDriver::ClearSteeringMemory() {
    hp2::not_implemented("0x00136C30", "CarDriver::ClearSteeringMemory (signature and full body unresolved)");
}

// ELF 0x00136C38; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c38.md.
void CarDriver::ZeroSteeringMemory() {
    hp2::not_implemented("0x00136C38", "CarDriver::ZeroSteeringMemory (signature and full body unresolved)");
}

// ELF 0x00136C40; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c40.md.
void CarDriver::MaxSpeedToTarget() {
    hp2::not_implemented("0x00136C40", "CarDriver::MaxSpeedToTarget (signature and full body unresolved)");
}

// ELF 0x00136C50; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c50.md.
void CarDriver::IsHonking() {
    hp2::not_implemented("0x00136C50", "CarDriver::IsHonking (signature and full body unresolved)");
}

// ELF 0x00136C68; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c68.md.
void CarDriver::SetSteerToPoint() {
    hp2::not_implemented("0x00136C68", "CarDriver::SetSteerToPoint (signature and full body unresolved)");
}

// ELF 0x00136C70; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c70.md.
void CarDriver::SetTarget() {
    hp2::not_implemented("0x00136C70", "CarDriver::SetTarget (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/CarMover.hpp"

namespace hp2::recovered {
// ELF 0x001309F0; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001309f0.md.
void CarMover::DoSnapshot() {
    hp2::not_implemented("0x001309F0", "CarMover::DoSnapshot (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x00133130; E-0042-U014; E-0042-S001; exact Debug TU ..\indep\src\car.cpp
namespace port::driving::native {
bool IsBlinking(Memory& m,std::uint32_t object){return hp2::recovered::Car(m,object).IsBlinking();}
}

// Retail 0x00133070; E-0042-U013; E-0042-S001; exact Debug TU ..\indep\src\car.cpp
namespace port::driving::native {
void PleaseResetMeLater(Memory& m,std::uint32_t object,InitServices& services){hp2::recovered::Car(m,object).PleaseResetMeLater(services);}
}

// Retail 0x00135EB8; E-0042-C009; E-0042-S001; exact Debug TU ..\indep\src\car.cpp
namespace port::driving::native {
float GetPostCollisionYawControlBoost(Memory& m,std::uint32_t car){return hp2::recovered::Car(m,car).GetPostCollisionYawControlBoost();}
}

// Retail 0x00135E18; E-0042-C008; E-0042-S001; exact Debug TU ..\indep\src\car.cpp
namespace port::driving::native {
bool ShouldVisualEffectsBeGenerated(Memory& m,std::uint32_t car){return hp2::recovered::Car(m,car).ShouldVisualEffectsBeGenerated();}
}

namespace hp2::recovered {
// ELF 0x00136D00; E-0042-C010; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136d00.md.
void CarDriver::Sub_00136D00() {
    hp2::not_implemented("0x00136D00", "CarDriver::Sub_00136D00 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00136B78; E-0042-U015; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136b78.md.
void CarMover::Sub_00136B78() {
    hp2::not_implemented("0x00136B78", "CarMover::Sub_00136B78 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0042 E-0042-S001: Car whole-entry shells; all four adopted entries are defined in original car.cpp.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): Car::SetMovementMode() [0x002AE518]
// Diagnostic text (not a callable signature): Car::Move() [0x002AE5A0]
// Diagnostic text (not a callable signature): Car::TriggerCarTerrainEffects() [0x002AE690]
#include "game_core/indep/inc/Car.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00133070; E-0042-U013; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00133070.md.


// ELF 0x00133130; E-0042-U014; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00133130.md.


// ELF 0x00135E18; E-0042-C008; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00135e18.md.


// ELF 0x00135EB8; E-0042-C009; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00135eb8.md.

} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001315F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001315f8.md.
void Car::SetFrictionBoost() {
    hp2::not_implemented("0x001315F8", "Car::SetFrictionBoost (signature and full body unresolved)");
}

// ELF 0x00132960; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00132960.md.
void Car::CalculateDebugChecksum() {
    hp2::not_implemented("0x00132960", "Car::CalculateDebugChecksum (signature and full body unresolved)");
}

// ELF 0x00134DB8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00134db8.md.
void Car::GetWheelTraction() {
    hp2::not_implemented("0x00134DB8", "Car::GetWheelTraction (signature and full body unresolved)");
}

// ELF 0x00134E00; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00134e00.md.
void Car::GetWheelsOnGround() {
    hp2::not_implemented("0x00134E00", "Car::GetWheelsOnGround (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/car.hpp"

namespace hp2::recovered {
// ELF 0x001309F8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001309f8.md.
void NullMover::Move() {
    hp2::not_implemented("0x001309F8", "NullMover::Move (signature and full body unresolved)");
}

// ELF 0x00136AF8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00136af8.md.
void NullMover::Sub_00136AF8() {
    hp2::not_implemented("0x00136AF8", "NullMover::Sub_00136AF8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0044 E-0044-P02, Debug Car instance methods. Original source ../indep/src/car.cpp.
namespace hp2::recovered {
// Debug00133198; BlinkTimer858/BlinkDuration85c are version-specific.
bool Car::IsBlinking(){const auto start=m_.word(address_+0x858);if(!start||start==0x7fffffffu)return false;const auto dt=static_cast<std::int32_t>(m_.word(0x00327cf8)-start);return static_cast<float>(dt)*m_.scalar(0x00324d24)<m_.scalar(address_+0x85c);}
// Debug00135F10, packed-time collision yaw graph.
float Car::GetPostCollisionYawControlBoost(){const auto start=m_.word(m_.word(address_+0x34)+0x10c);if(!start||start==0x7fffffffu)return 0;const float elapsed=static_cast<float>(static_cast<std::int32_t>(m_.word(0x00327cf8)-start))*m_.scalar(0x00324d9c);if(!(elapsed<4.0f))return 0;return port::driving::Graph::GetValue(m_,0x00327018,elapsed);}
// Debug00135E70.
bool Car::ShouldVisualEffectsBeGenerated(){if(m_.word(m_.word(address_+0x14)+8)==3)return false;return m_.word(address_+0x868)==0&&m_.signed_byte(address_+0x4a8)!=0;}
// Debug001330D8, original external pursuit predicate remains an explicit service.
void Car::PleaseResetMeLater(port::driving::native::InitServices& services){bool set=true;if(m_.word(0x003121ec)&&m_.signed_word(0x003121d4)>=1&&m_.word(m_.word(address_+0x14)+8)==2){if(!services.Sub_00227438)hp2::not_implemented("0x00221C78","IsBeingPursued service absent");set=services.Sub_00227438(m_.word(0x00328c1c),address_)==0;}if(set)m_.word(address_+0x854,1);}
}
