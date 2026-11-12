#include "game_core/indep/src/BeltMover.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/rumble.hpp"
#include "game_core/indep/src/collision.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/port/race/car_reset_services.hpp"
#include "game_core/indep/inc/RealDriver.hpp"
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
namespace hp2::recovered {
// Debug00133900,E-0100-PA01. Original bToPolar and signed16 angle difference.
bool Car::IsCarInFront(std::uint32_t other,std::uint16_t tolerance){auto& m=m_;::port::driving::Memory::Temporary polar(m,8);bToPolar(m,polar.address(),{{m.scalar(other+0x60)-m.scalar(address_+0x60),m.scalar(other+0x64)-m.scalar(address_+0x64)}});const int difference=static_cast<std::int16_t>(static_cast<std::uint16_t>(m.half(polar.address()+4)-m.half(address_+0x3a4)));return std::abs(difference)<int(tolerance);}
}
#include "game_core/port/driving/terrain_effects.hpp"
#include "game_core/port/driving/frame_lifecycle.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/indep/src/track.hpp"
#include <algorithm>
#include <cmath>
#include "game_core/indep/inc/AnimatedDriver.hpp"

namespace hp2::recovered {
// Debug001334F0, E-0099-W28. CAR_USAGE_TYPE_TRAFFIC is recovered enum2.
void Car::AssignAnimatedDriver(const std::string& base_name){
 if(m_.signed_word(m_.word(address_+0xc)+0x538)!=2){const auto driver=m_.allocate(0x90);
  hp2::debug::AnimatedDriver(m_,driver).Sub_001A6CF0(address_,base_name);m_.word(address_+0x4b4,driver);}
}
// Debug00133558, E-0099-W28. Actual original destruction, then pointer reset.
void Car::UnAssignAnimatedDriver(){if(auto driver=m_.word(address_+0x4b4)){hp2::debug::AnimatedDriver(m_,driver).Sub_001A6DE8(3);m_.word(address_+0x4b4,0);}}
}

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
// Debug00131D78; E-0099-W39. Full shared-cache entry; original ordering retained.
void Car::BeginMovement(float t){
    auto& m=m_;const auto car=address_;
    if(m.word(car+0x44)!=car+0x520)hp2::not_implemented("0x00131D78","actual constructed Car topology coordinate is required");
    if(0<t){m.copy(car+0x470,car+0x60,16);m.copy(car+0x480,car+0x70,16);m.copy(car+0x490,car+0xd0,16);}
    const std::array<float,2> position{{m.scalar(car+0x60),m.scalar(car+0x64)}},velocity{{m.scalar(car+0x70),m.scalar(car+0x74)}};
    const float x=m.scalar(car+0x70),y=m.scalar(car+0x74),z=m.scalar(car+0x78);
    const float radius=std::sqrt((x*x+y*y)+z*z)*m.scalar(0x324ce8)+3;
    hp2::debug::TopologyCoordinate topology(m,car+0x520);topology.SetInterestBBox(position,radius,&velocity);
    if(topology.HasTopology(position)&&1<m.signed_word(car+0x3bc)){m.word(car+0x860,m.word(car+0x60));m.word(car+0x864,m.word(car+0x64));}
    CarState(m,car+0x40).BeginTimestep();
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
    // Port lifetime: contact may expand a persistent shared topology slot pool.
    // Release this one stack operand without rolling back callee allocations.
    port::driving::Memory::Temporary storage(m,0x10);
    std::array<float,5> heights{};const auto temporary=storage.address();
    for(unsigned i=0;i<5;++i){auto probe=position;probe[0]+=relative[i][0];probe[1]+=relative[i][1];m.vector(temporary,probe);heights[i]=contact.GetElevation(m,0,temporary,0,0,0);}
    const auto cfg=m.word(address()+0xc);const auto wheel=cfg+0x120+3*0x20;
    const float x=((heights[1]-heights[0])-(heights[2]-heights[0]))*0.5f*m.scalar(0x00324d14);
    const float y=((heights[3]-heights[0])-(heights[4]-heights[0]))*0.5f*m.scalar(0x00324d14);
    position[2]=((heights[0]+m.scalar(wheel+0xc))-m.scalar(wheel+8))+m.scalar(cfg+0x118);
    rotation=hp2::recovered::bIdentity();hp2::recovered::eRotateZ(m,rotation,rotation,angle);
    hp2::recovered::eRotateX(m,rotation,rotation,static_cast<std::uint16_t>(static_cast<int>(y)));
    hp2::recovered::eRotateY(m,rotation,rotation,static_cast<std::uint16_t>(-static_cast<std::int16_t>(static_cast<int>(x))));
}
// Debug00132BA0, E-0099-W111; original car.cpp LW driver then LHU Steering.
std::uint16_t Car::GetSteering()const{return m_.half(m_.word(address_+0x2c)+0x210);}
// Debug001329C0, E-0099-W26. Original PutFlatOnTrack stores the full
// 128-bit Position-CoG result, including the original vector padding lane.
void Car::PutFlatOnTrack(port::driving::ContactProvider& contact){
    auto rotation=m_.matrix(address_+0x90);auto position=m_.vector(address_+0x60);
    FindFlatOnTrackPosition(rotation,position,m_.half(address_+0x3a4),contact);
    m_.matrix(address_+0x90,rotation);m_.vector(address_+0x60,position);
    const auto cog=m_.vector(m_.word(address_+0xc)+0x110);
    for(unsigned i=0;i<4;++i)position[i]=position[i]-cog[i];
    m_.vector(address_+0x50,position);
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
// Debug001316D8, E-0099-W47. Actual shared state; original Physics setter is an inline store.
void Car::SetFrictionBoost(float boost) {m_.scalar(address_+0x454,boost);if(m_.word(address_+0x24)==1)m_.scalar(m_.word(address_+0x30)+0x70,boost);}

// Debug00132A10, E-0099-W48. Actual source scalar grouping and EE integer conversion.
std::uint32_t Car::CalculateDebugChecksum() const {return std::uint32_t(::port::driving::cvt_w_s((m_.scalar(address_+0x60)+m_.scalar(address_+0x64))*65536.f));}

// Debug00134E10 E-0099-W66; MIPS C.LT selection preserves source axle grouping.
float Car::GetWheelTraction() {const float a=(m_.scalar(address_+0x394)+m_.scalar(address_+0x3a0))*.5f,b=(m_.scalar(address_+0x398)+m_.scalar(address_+0x39c))*.5f;return a<b?b:a;}

// Debug00134E58 E-0099-W66; four independent nonzero bytes, not cached count.
float Car::GetWheelsOnGround() {float result=0;for(unsigned i=0;i<4;++i)if(m_.byte(address_+0x350+i))result+=.25f;return result;}
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

#include "game_core/indep/inc/CarDriver.hpp"
#include "game_core/indep/src/drivetarget.hpp"
namespace hp2::debug {
// Debug00136D10, E-0099-W21. Original definition ../indep/inc/CarDriver.hpp,
// instantiated in ../indep/src/car.cpp. Preserve SportGear and alignment bytes.
void CarDriver::Sub_00136D10(){
 m_.word(address_+0x22c,0x2a6898);DriveTarget target(m_,address_);target.Sub_00216528();
 m_.scalar(address_+0x214,0);m_.half(address_+0x210,0);
 m_.scalar(address_+0x218,0);m_.scalar(address_+0x21c,0);
 m_.word(address_+0x228,0);m_.word(address_+0x220,1);target.Clear(0,0);
}
// Debug00136CB0, E-0099-W21. DriveTarget destructor00216598 receives2,
// proven to perform no stores/free on this branch.
void CarDriver::Sub_00136CB0(int in_chrg){m_.word(address_+0x22c,0x2a6898);if(in_chrg&1)m_.release(address_);}
}

#include "game_core/indep/inc/CarController.hpp"
#include "game_core/indep/inc/CarMover.hpp"
#include "game_core/inc/bSlotPool.hpp"
namespace hp2::debug {
// Debug00136908, E-0099-W21. The known constructor owns a NullDriver;
// any later replaced dynamic driver requires its actual original destructor.
void NullController::Sub_00136908(int flags){
 m_.word(address_+8,0x2a6740);
 if(const auto driver=m_.word(address_+4)){
  if(m_.word(driver+0x22c)!=0x2a67f0)hp2::not_implemented("0x00136908","NullController driver virtual destructor is not reconstructed for this class");
  // NullDriver00136B98 writes its vtable then invokes base destructor.
  m_.word(driver+0x22c,0x2a67f0);CarDriver(m_,driver).Sub_00136CB0(3);
 }
 Sub_001BD1C0(flags);
}
// Debug00136B50 /00136B20, E-0099-W21;00136B28 body is in original CarMover.hpp (W105).
void CarMover::Sub_00136B50(){m_.word(address_+4,0x2a67d0);}
void CarMover::SetDriver(std::uint32_t driver){m_.word(address_,driver);}
// Debug00136B08, E-0099-W21. Original does not call base ctor or set Driver.
void NullMover::Sub_00136B08(std::uint32_t state){m_.word(address_+8,state);m_.word(address_+4,0x2a67b0);}
// Debug00136A88 /00136AA8 ->00136A78, E-0099-W21.
std::uint32_t NullMover::Sub_00136A88(::port::driving::Memory& m){return bOMalloc(m,m.word(0x327044));}
void NullMover::Sub_00136AA8(int flags){
 m_.word(address_+4,0x2a67b0);Sub_00136B28(0);
 if(flags&1)SlotPool(m_,m_.word(0x327044)).Free(address_);
}
// Debug00136A70, E-0099-W21; instruction-confirmed empty Null override.
void NullMover::SetDriver(std::uint32_t){}
// Debug00130B40, E-0099-W21. Wheel matrices still update with no movement.
float NullMover::Move(float t){hp2::recovered::CarState(m_,m_.word(address_+8)).CalculateWheelMatrices();return t;}
}

#include "game_core/port/race/control_construction.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/port/race/traffic_construction.hpp"
namespace port::race {
// Port dispatch for the original virtual destructors, E-0099-W36. Each table
// belongs to the primary controller/mover base; no host class ABI is implied.
void destroy_car_controller(::port::driving::Memory& m,std::uint32_t object){
 using namespace hp2::debug;
 switch(m.word(object+8)){
 case 0x2a6740:NullController(m,object).Sub_00136908(3);break;
 case 0x2b5c40:RealController(m,object).Sub_00234220(3);break;
 case 0x2a6358:AIDriver(m,object).Sub_001301E0(3);break;
 case 0x2ac510:TrafficDriver(m,object).Sub_0018A540(3);break;
 case 0x2ad758:CopDriver(m,object).Sub_0019F6A8(3);break;
 default:hp2::not_implemented("0x001314F0","Car controller virtual destruction for this class is not reconstructed");
 }
}
void destroy_car_mover(::port::driving::Memory& m,std::uint32_t object){
 using namespace hp2::debug;
 switch(m.word(object+4)){
 case 0x2a67b0:NullMover(m,object).Sub_00136AA8(3);break;
 case 0x2a6af8:PhysicsCar(m,object).Sub_001376C8(3);break;
 case 0x2aa000:TrafficCar(m,object).Sub_00167550(3);break;
 case 0x2b42f0:MellowMover(m,object).Sub_00214410(3);break;
 case 0x2b2c90:BeltMover(m,object).Sub_001FF410(3);break;
 case 0x2ad7c8:CopMover(m,object).Sub_0019E700(3);break;
 default:hp2::not_implemented("0x001316F8","Car mover virtual destruction for this class is not reconstructed");
 }
}
}

#include "game_core/port/race/car_construction.hpp"
#include "game_core/port/race/car_sound_resources.hpp"
#include "game_core/port/race/damage_resources.hpp"
#include "game_core/port/race/events.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
namespace hp2::recovered {
// Debug00130B90, E-0099-W32. NFSHP2.ELF815b...; every required game
// initializer uses its recovered implementation and the same logical Memory.
// Original local vector W is indeterminate; services label that one PC operand.
void Car::Sub_00130B90(std::uint32_t driver,std::uint32_t start,std::uint16_t angle,const ::port::race::CarConstructionServices& services){
 auto& m=m_;const auto car=address_;
 if(!services.routes||!services.routes->initialized)hp2::not_implemented("0x002036E0","actual initialized route/path resources are required for Car");
 if(!services.topology||!services.topology->loaded||services.topology->owner!=&m)hp2::not_implemented("0x00151CC0","actual shared topology resource owner is required for Car");
 if(!services.collisions||!services.collisions->source_data_loaded)hp2::not_implemented("0x00213C98","complete vehicle collision source owner is required for Car");
 if(!services.port_stack_vector_lane)hp2::not_implemented("0x00130B90","original uninitialized local vector W requires an explicit port operand");
 const auto stack_lane=*services.port_stack_vector_lane;
 const auto type=m.signed_word(driver+0xc);if(type<0||type>=64)throw std::out_of_range("Original Car type outside loaded64-entry table");
 const auto info_table=m.word(0x3278e0);if(!info_table)hp2::not_implemented("0x00186148","actual original CarTypeInfoArray is not loaded");
 const auto info=info_table+static_cast<unsigned>(type)*0x560;
 CarState(m,car+0x40).Sub_001BD2C0(car+0x520,info);
 m.word(car+0x518,0);m.word(car+0x514,0xffffffffu);m.word(car+0x510,0);
 hp2::debug::TopologyCoordinate(m,car+0x520).Sub_00151878();
 hp2::debug::TrackPathCoordinate::Sub_002035A0(m,car+0x7e0);
 for(auto offset:{0x820u,0x84cu,0x858u,0x884u})m.word(car+offset,0);
 for(auto offset:{0x470u,0x480u,0x490u}){for(unsigned j=0;j<3;++j)m.word(car+offset+j*4,0);m.word(car+offset+12,stack_lane);}
 std::array<float,4> position{{0,0,0,0}};
 std::uint32_t position_w=stack_lane;
 if(start){position=m.vector(start);position_w=m.word(start+12);m.copy(car+0x470,start,16);}
 hp2::debug::TrackPathCoordinate::UpdatePosition(m,car+0x7e0,services.routes->paths,{position[0],position[1]});
 for(unsigned i=0;i<3;++i)m.scalar(car+0x60+4*i,position[i]);
 m.word(car+0x6c,position_w);m.half(car+0x3a4,angle);
 m.byte(car+0x848,0);m.byte(car+0x849,0);m.scalar(car+0x454,1);
 m.scalar(car+0x458,0);m.word(car+0x14,driver);m.scalar(car+0x45c,0);m.scalar(car+0x460,0);
 const auto sound_enabled=m.word(0x326f40);const auto unique=m.signed_byte(driver+1);
 m.word(car+0x10,driver+0xc);m.half(car+8,0);m.half(car+0xa,static_cast<std::uint16_t>(unique));
 m.word(car+0x18,0);m.word(car+0x1c,0);m.word(car+0xc,info);m.word(car+0x850,0);
 const auto usage=m.word(info+0x538);
 if(sound_enabled){
  if(!services.audio||!services.audio->source_ready()){
   if(unique==0||unique==1)hp2::not_implemented("0x0018CB30","actual loaded player-car sound source owner is required");
   if(usage==1)hp2::not_implemented("0x0018CBC0","actual loaded cop-car sound source owner is required");
   if(usage==0||usage==2)hp2::not_implemented("0x0018CB78","actual loaded AI-car sound source owner is required");
  }
  if(unique==0||unique==1)m.word(car+0x850,hp2::debug::sound::CreatePlayerCarSound(m,car,*services.audio));
  else if(usage==1)m.word(car+0x850,hp2::debug::sound::CreateAICopCarSound(m,car,*services.audio));
  else if(usage==0||usage==2)m.word(car+0x850,hp2::debug::sound::CreateAICarSound(m,car,*services.audio));
 }
 const auto controller=m.allocate(0xc);hp2::debug::CarController(m,controller).Sub_001BD1A8(car);m.word(controller+8,0x2a6740);
 const auto car_driver=m.allocate(0x230);hp2::debug::CarDriver(m,car_driver).Sub_00136D10();m.word(controller+4,car_driver);
 m.word(car+0x28,controller);m.word(car+0x2c,car_driver);m.word(car_driver+0x22c,0x2a67f0);
 const auto mover=hp2::debug::NullMover::Sub_00136A88(m);hp2::debug::NullMover(m,mover).Sub_00136B08(car+0x40);
 m.word(car+0x30,mover);m.word(car+0x4b4,0);m.word(car+0x844,0);
 if(usage!=2){const auto damage=m.allocate(0xe4);hp2::debug::CarDamage(m,damage).Sub_001B2940(car);m.word(car+0x844,damage);}
 m.word(car+0x20,0);m.word(car+0x24,0);
 const auto body=hp2::debug::bOMalloc(m,m.word(0x32842c));hp2::debug::CarCollisionBody(m,body).Sub_001D4A80(car,*services.collisions);
 m.word(car+0x87c,0);m.word(car+0x878,0);m.word(car+0x880,0);m.word(car+0x34,body);
 hp2::debug::espSetAttributeInt(m,0,0x2a64d8,m.signed_half(car+0xa),services.monitor);
 ::port::race::SharedTopologyContact contact(m,car+0x520);PutFlatOnTrack(contact);
 m.word(car+0x3c8,2);m.word(car+0x450,1);m.scalar(car+0x4a0,0);m.word(car+0x4b0,0);
 if(m.word(driver+0x14)&&usage!=2&&(!services.characters||!services.characters->source_data_loaded))hp2::not_implemented("0x001D6030","actual character/body-type sources are not loaded");
 InitRenderInfo(m.word(car+0x10));
 if(const auto damage=m.word(car+0x844))hp2::debug::CarDamage(m,damage).InitSectorPositions();
 m.byte(car+0x4a8,1);m.word(car+0x4ac,1);m.word(car+0x854,0);m.scalar(car+0x860,m.scalar(car+0x60));m.scalar(car+0x864,m.scalar(car+0x64));
 hp2::debug::emAddHandler(m,0x130b70,0x10000);
 for(auto offset:{0x838u,0x83cu,0x840u,0x834u,0x808u,0x830u,0x82cu,0x814u,0x818u,0x81cu,0x80cu})m.word(car+offset,0);
 m.word(car+0x820,m.word(0x327cf8));m.scalar(car+0x828,-1);m.scalar(car+0x824,-1);StopBlinking();
 const float time=static_cast<float>(m.signed_word(0x327cf8))*m.scalar(0x324cd0);
 m.word(car+0x86c,1);m.scalar(car+0x4a4,0);m.word(car+0x868,0);m.scalar(car+0x85c,2);
 m.word(car+0x884,static_cast<std::uint32_t>(::port::driving::cvt_w_s(time*4000.f+0.5f)));
 for(unsigned i=0;i<4;++i)m.byte(car+0x870+i,0);
 m.byte(car+0x874,0);m.word(car+0x810,0);m.word(car+0x88c,0);
}
// Debug00133190, E-0099-W32. Does not change duration/visibility bits.
void Car::StopBlinking(){m_.word(address_+0x858,0);}
// Debug00130FF8, E-0099-W32. Actual construction-state virtual targets.
// Later movement/controller/audio classes must bind their real destruction.
void Car::Sub_00130FF8(int flags){
 auto& m=m_;const auto car=address_;
 if(const auto controller=m.word(car+0x28)){
  ::port::race::destroy_car_controller(m,controller);
 }
 if(const auto mover=m.word(car+0x30)){
  ::port::race::destroy_car_mover(m,mover);
 }
 if(const auto body=m.word(car+0x34)){
  if(m.word(body+0x50)!=0x2b04f0)hp2::not_implemented("0x00130FF8","Car collision-body virtual destruction is not bound for this class");
  hp2::debug::CarCollisionBody(m,body).Sub_001D4BD8(3);
 }
 if(const auto damage=m.word(car+0x844)){hp2::debug::CarDamage(m,damage).Sub_001B29C8();m.release(damage);m.word(car+0x844,0);}
 CloseRenderInfo();
 if(m.word(car+0x878))hp2::not_implemented("0x00130FF8","Car development object destruction is not yet translated");
 if(const auto sound=m.word(car+0x850))hp2::debug::sound::DeleteCarSound(m,sound);
 hp2::debug::emRemoveHandler(m,0x130b70);hp2::debug::TrackPathCoordinate::Sub_00203630(m,car+0x7e0,2);
 hp2::debug::TopologyCoordinate(m,car+0x520).Sub_001518C0(2);
 // Original001368C0 bTNode<Car> destructor only releases on bit1.
 if(flags&1)m.release(car);
}
}

namespace hp2::recovered {
// Debug001314F0, E-0099-W36. Replacement is unconditional even for equal modes.
void Car::SetControlMode(int mode,const ::port::track::RouteData& routes){
 auto& m=m_;const auto car=address_;
 if(mode<0||mode>4)throw std::out_of_range("Original ControlMode outside recovered enum");
 if(const auto old=m.word(car+0x28))::port::race::destroy_car_controller(m,old);
 m.word(car+0x20,static_cast<std::uint32_t>(mode));std::uint32_t controller=0;
 switch(mode){
 case 0:{
  controller=m.allocate(0xc);hp2::debug::CarController(m,controller).Sub_001BD1A8(car);
  m.word(controller+8,0x2a6740);const auto driver=m.allocate(0x230);
  hp2::debug::CarDriver(m,driver).Sub_00136D10();m.word(controller+4,driver);
  m.word(car+0x28,controller);m.word(driver+0x22c,0x2a67f0);break;
 }
 case 1:controller=m.allocate(0x54);hp2::debug::RealController(m,controller).Sub_00234120(car,routes);break;
 case 2:controller=m.allocate(0x2d0);hp2::debug::AIDriver(m,controller).Sub_00130118(car,routes);break;
 case 3:controller=m.allocate(0x2d0);hp2::debug::TrafficDriver(m,controller).Sub_0018A4A8(car,routes);break;
 case 4:controller=m.allocate(0x4c);hp2::debug::CopDriver(m,controller).Sub_0019F560(car,routes);break;
 }
 m.word(car+0x28,controller);const auto driver=m.word(controller+4);
 m.word(car+0x2c,driver);m.word(m.word(car+0x30),driver);
}
// Debug001316F8, E-0099-W36. Direct Driver assignment is the original inlined
// base setter, not the proven-empty NullMover virtual override.
void Car::SetMovementMode(int mode,::port::race::PhysicsConstructionServices& services){
 auto& m=m_;const auto car=address_;::port::race::SharedTopologyContact own_contact(m,car+0x520);auto actual=services;actual.contact=&own_contact;
 if(mode<0||mode>6)throw std::out_of_range("Original MovementMode outside recovered enum");
 if(const auto old=m.word(car+0x30))::port::race::destroy_car_mover(m,old);
 m.word(car+0x24,static_cast<std::uint32_t>(mode));std::uint32_t mover=0;
 const auto body=m.word(car+0x34);
 switch(mode){
 case 0:mover=hp2::debug::NullMover::Sub_00136A88(m);hp2::debug::NullMover(m,mover).Sub_00136B08(car+0x40);m.word(car+0x30,mover);m.word(body+0x54,0);break;
 case 1:
  mover=m.allocate(0x78);hp2::debug::PhysicsCar(m,mover).Sub_001370C0(m.word(car+0xc),m.word(car+0x2c),car,actual);
  m.word(mover+0x70,m.word(car+0x454));m.word(body+0x54,1);m.word(car+0x30,mover);break;
 case 2:mover=m.allocate(0x24);hp2::debug::TrafficCar(m,mover).Sub_00166ED0(car);m.word(car+0x30,mover);m.word(body+0x54,0);break;
 case 3:hp2::not_implemented("0x001FDFD0","DeliveryMover construction is not reconstructed");
 case 4:mover=hp2::debug::MellowMover::Sub_00216500(m);hp2::debug::MellowMover(m,mover).Sub_00214248(car,m.word(car+0xc),m.word(car+0x2c),car+0x40,actual);m.word(car+0x30,mover);m.word(body+0x54,1);break;
 case 5:mover=m.allocate(0x80);hp2::debug::CopMover(m,mover).Sub_0019E650(car,actual.cop_mover_ground_normal_w);m.word(car+0x30,mover);m.word(body+0x54,0);break;
 case 6:mover=m.allocate(0x48);hp2::debug::BeltMover(m,mover).Sub_001FF108(car);m.word(car+0x30,mover);m.word(body+0x54,0);break;
 }
 hp2::debug::CarCollisionBody(m,body).SetupPhysicsPointers();m.word(mover,m.word(car+0x2c));
}
}

namespace hp2::recovered {
// Debug001318E0, E-0099-W36. Decode the original descriptor and this adjustment.
void Car::SetToPhysicsMode(std::uint32_t collision,::port::race::PhysicsConstructionServices& services){
 auto& m=m_;const auto controller=m.word(address_+0x28);
 if(!controller){const auto mode=m.word(address_+0x24);if(mode!=1&&mode!=4)SetMovementMode(m.word(address_+0x18)?1:4,services);return;}
 const auto table=m.word(controller+8),target=m.word(table+0x5c),adjusted=controller+static_cast<std::int32_t>(m.signed_half(table+0x58));
 if(target==0x1bd218)hp2::debug::CarController(m,adjusted).SetToPhysicsMode(collision,services);
 else if(target==0x191328)hp2::debug::AICarController(m,adjusted).SetToPhysicsMode(collision,services);
 else hp2::not_implemented("0x001318E0","Car controller SetToPhysicsMode virtual target is not reconstructed");
}
}

#include "game_core/indep/inc/RealDriver.hpp"
namespace hp2::recovered {
// Debug001341F8, E-0099-W39. Original phase comparisons and32-bit bool stores.
void Car::UpdateHeadlightsAndHorn(){
 auto& m=m_;const auto car=address_;if(m.word(car+0x18))return;
 const float on=m.scalar(car+0x824);float elapsed=static_cast<float>(static_cast<std::int32_t>(m.word(0x327cf8)-m.word(car+0x820)))*m.scalar(0x324d48);
 bool phase=false;
 if(on<=elapsed){elapsed=elapsed-on;if(elapsed<m.scalar(car+0x828))phase=m.word(car+0x81c)==0;else if(!(on<=elapsed-m.scalar(car+0x828)))phase=true;}else phase=true;
 m.word(car+0x830,((phase&&m.word(car+0x814))||m.word(car+0x82c))?1u:0u);
 const bool requested=phase&&m.word(car+0x818);const auto driver=m.word(car+0x2c),table=m.word(driver+0x22c);
 const auto query=m.word(table+0x6c),query_this=driver+static_cast<std::int32_t>(m.signed_half(table+0x68));bool honking;
 if(query==0x136c00)honking=hp2::debug::CarDriver(m,query_this).IsHonking();
 else if(query==0x13a488)honking=hp2::debug::PsychoDriver(m,query_this).IsHonking();
 else hp2::not_implemented("0x001341F8","unrecovered driver IsHonking virtual target");
 if(requested==honking)return;
 const auto offset=requested?0x58u:0x60u,target=m.word(table+offset+4),adjusted=driver+static_cast<std::int32_t>(m.signed_half(table+offset));
 if(target==0x136c10)hp2::debug::CarDriver(m,adjusted).StartHonkHorn();
 else if(target==0x136c08)hp2::debug::CarDriver(m,adjusted).StopHonkHorn();
 else if(target==0x13a448)hp2::debug::PsychoDriver(m,adjusted).StartHonkHorn();
 else if(target==0x13a468)hp2::debug::PsychoDriver(m,adjusted).StopHonkHorn();
 else if(target==0x13c220)RealDriver(m,adjusted).StartHonkHorn();
 else if(target==0x13c260)RealDriver(m,adjusted).StopHonkHorn();
 else hp2::not_implemented("0x001341F8","unrecovered driver horn virtual target");
}
// Debug00131E58, E-0099-W39/W59. Actual shared path and Player periodic
// replay consumers; reached zone/air prediction callees remain explicit.
void Car::FinishMovement(float t,const ::port::track::RouteData& routes,const ::port::race::PhysicsConstructionServices* physics){
 auto& m=m_;const auto car=address_;::port::driving::finish_movement_state(m,car,t);
 const std::array<float,2> position{{m.scalar(car+0x60),m.scalar(car+0x64)}};
 hp2::debug::TrackPathCoordinate::UpdatePosition(m,car+0x7e0,routes.paths,position);
 const auto loop=m.word(0x327d10);
 if(((loop+7)&31)==0&&m.word(car+0x24)!=0){hp2::debug::TopologyCoordinate topology(m,car+0x520);if(!topology.HasTopology(position)&&!topology.HasTopology({{position[0]+1,position[1]}}))m.word(car+0x854,1);}
 const auto distance=[&]{const auto point=m.word(car+0x7e8);if(!point)return 0.f;const float x=m.scalar(car+0x7e0)-m.scalar(point),y=m.scalar(car+0x7e4)-m.scalar(point+4);return std::sqrt(x*x+y*y);};
 const float d=distance();if(m.scalar(0x324cec)<d){if(!m.word(0x32711c))m.word(car+0x854,1);}
 else if(300<distance()&&loop%60==0&&m.word(car+0x24)!=0){m.word(car+0x7e8,0);hp2::debug::TrackPathCoordinate::UpdatePosition(m,car+0x7e0,routes.paths,position);}
 UpdateHeadlightsAndHorn();const auto player=m.word(car+0x18);if(!player)return;
 const auto wheels=m.signed_word(car+0x3bc);
 auto zone=m.signed_word(player+0x54);
 if((zone==6||zone==7)&&wheels>0)hp2::not_implemented("0x00176B70","original Player.ClearZone(true) dependency reached after landing");
 if(((loop+3)&7)!=0)return;
 const float speed=m.scalar(car+0x3c4);
 const auto interesting=[&](float value,const char* name){const auto replay=m.word(0x3271a8);if(!replay)hp2::not_implemented("0x0013E070","original Player frame requires actual ReplayManager owner");ReplayManager(m,replay).RegisterInterestingEvent(value,name);};
 if(speed>40&&wheels<4&&std::abs(CarState(m,car+0x40).GetYaw())<m.scalar(0x324cf0))interesting(2,"CarInAir");
 if(!m.word(0x326fd8)&&zone!=6&&zone!=7&&wheels==0&&!m.word(player+0x2f4)&&speed>30){
  float seconds=3,highest;AverageAir(seconds,highest,physics);
  if(seconds<m.scalar(0x324cf4))m.scalar(car+0x4a4,float(m.signed_word(0x327cf8))*m.scalar(0x324cf8)+5);
  else m.scalar(car+0x4a4,seconds*.5f+float(m.signed_word(0x327cf8))*m.scalar(0x324cfc));
  if(1.5f<highest&&.5f<seconds){
   if(!physics||!physics->disable_jump_camera_presentation)hp2::not_implemented("0x00175238","original IsJumpCamAvailable/RequestZone camera dependencies required");
   ::port::diagnostics::warn("0x00175238","original jump-camera presentation chain is not implemented","explicit PC policy omits camera request before entry; source air prediction/timing retained, no zone or time-scale acceptance fabricated");
  }
 }
 // The source zone6 pose publication occurs independently of auto-replay enable.
 if(zone==6){if(m.scalar(car+0x78)<0&&m.scalar(player+0x698)<m.scalar(car+0x68))m.scalar(player+0x698,m.scalar(car+0x68));
  if(m.scalar(car+0x4a4)<float(m.signed_word(0x327cf8))*m.scalar(0x324d00))hp2::not_implemented("0x00176B70","original expired jump zone ClearZone(false)/RequestZone7 dependencies are not reconstructed");}
 if(speed<5&&!m.word(player+0x4c))interesting(-10,"GoingSlow");
 if(speed>40){const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x00131E58","original Player proximity replay events require actual World");
  const auto count_near=[&](unsigned begin,unsigned end){int count=0;for(auto i=m.signed_word(world+begin);i<m.signed_word(world+end);++i){const auto other=m.word(world+0x2c+4*std::uint32_t(i));const float x=m.scalar(other+0x60)-m.scalar(car+0x60),y=m.scalar(other+0x64)-m.scalar(car+0x64),z=m.scalar(other+0x68)-m.scalar(car+0x68);if(std::sqrt((x*x+y*y)+z*z)<40)++count;}return count;};
  const auto cars=count_near(0x20,0x24);if(cars>0)interesting(float(cars)*.5f,"CloseCars");const auto cops=count_near(0x24,0x28);if(cops>0)interesting(float(cops)*m.scalar(0x324d04),"CloseCops");}
 interesting(speed*m.scalar(0x324d08),"Speed");
 if(speed>30){if(m.scalar(0x324d0c)<std::abs(CarState(m,car+0x40).GetYaw()))interesting(1,"CarSlidingSideways");const auto collision=hp2::debug::CollisionBody(m,m.word(car+0x34)).GetCollision(0);
  if(collision){hp2::debug::Collision contact(m,collision);if(contact.IsPlayerInvolved()&&float(std::int32_t(m.word(0x327cf8)-m.word(collision+0xc)))*m.scalar(0x324d10)<.5f){
   if(contact.IsTwoCarCollision()&&speed>40){interesting(3,"CarCollision");std::uint32_t first,second;contact.GetCollidingCars(first,second);}else if(contact.IsSmokeableInvolved())interesting(2.5f,"SmokeableCollision");}}}
}
}

#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/table.hpp"
namespace hp2::debug {
// Debug00136628, E-0099-W42. Complete original Car TU static initializer.
// ObjectPoints XYZ stores intentionally preserve each fourth padding lane.
void Sub_00136628(::port::driving::Memory& m){
 const float draft[][2]={{0,0},{1,0},{5,10},{150,60}};
 for(unsigned i=0;i<4;++i)for(unsigned j=0;j<2;++j)m.scalar(0x2e7d00+i*8+j*4,draft[i][j]);
 hp2::recovered::Table(m,0x2e7d20).Sub_0018D810(0x2e7ce8,6,0,1);hp2::recovered::Graph(m,0x326f98).Sub_0018D9E8(0x2e7d00,4);
 m.word(0x32aef0,m.word(0x324da4));m.scalar(0x32aef4,0);m.word(0x32aef8,m.word(0x324da8));m.scalar(0x32aefc,1);hp2::recovered::Graph(m,0x327010).Sub_0018D9E8(0x32aef0,2);
 const float points[][3]={{0,0,0},{3,0,3},{0,-3,3},{-3,0,3},{0,3,3},{1,0,4},{0,-1,4},{-1,0,4},{0,1,4},{3,0,9},{0,-3,9},{-3,0,9},{0,3,9}};
 for(unsigned i=0;i<13;++i)for(unsigned j=0;j<3;++j)m.scalar(0x32ade0+i*16+j*4,points[i][j]);
 m.matrix(0x32aeb0,{{.5f,0,0,0,0,.5f,0,0,0,0,.5f,0,0,0,1,1}});
 m.scalar(0x2e7d68,0);m.scalar(0x2e7d6c,0);m.word(0x2e7d70,m.word(0x324dac));m.scalar(0x2e7d74,1);m.scalar(0x2e7d78,1);m.scalar(0x2e7d7c,1);m.scalar(0x2e7d80,2);m.scalar(0x2e7d84,0);hp2::recovered::Graph(m,0x327018).Sub_0018D9E8(0x2e7d68,4);
}
}

namespace hp2::recovered {
// Debug00133370, E-0099-W37. Original descriptor adjustment and precise target.
float Car::GetAmountOutOfBounds()const{
 const auto controller=m_.word(address_+0x28),table=m_.word(controller+8),target=m_.word(table+0x44),adjusted=controller+static_cast<std::int32_t>(m_.signed_half(table+0x40));
 if(target==0x1368d8)return 0.f; // original NullController constant getter.
 if(target==0x18e798)return hp2::debug::AICarController(m_,adjusted).GetAmountOutOfBounds();
 if(target==0x234048)return hp2::debug::TrackedController(m_,adjusted).GetAmountOutOfBounds();
 hp2::not_implemented("0x00133370","Car bounds virtual target is not reconstructed");
}
// Debug001351C8, E-0099-W37. Preserve full16-byte lanes and original copy order.
void Car::GetCarSpaceBoundingBox(std::uint32_t min,std::uint32_t max)const{const auto body=m_.word(address_+0x34);m_.copy(min,body+0xe0,16);m_.copy(max,body+0xf0,16);}
}

#include "game_core/port/race/ai_drive_target.hpp"
namespace hp2::recovered {
// Debug00133340, E-0099-W37. Fresh MIPS uses38/3C, not FindWhereIAm48/4C.
// Debug00133310, E-0110-F04. Bind actual AI localization when a finished
// player's source control changes to AI; never return a guessed route.
std::uint32_t Car::GetRoute(const ::port::race::AiDriveTargetServices& services)const{
 const auto controller=m_.word(address_+0x28),table=m_.word(controller+8),target=m_.word(table+0x34),adjusted=controller+static_cast<std::int32_t>(m_.signed_half(table+0x30));
 if(target==0x1368f0)return 0;
 if(target==0x233ee0)return hp2::debug::TrackedController(m_,adjusted).GetCurrentRoute();
 if(target==0x18e6e8)return hp2::debug::AICarController(m_,adjusted).GetCurrentRoute(services);
 hp2::not_implemented("0x00133310","Car route virtual target is not reconstructed");
}
std::uint32_t Car::GetRoutePoint(const ::port::race::AiDriveTargetServices& services)const{
 const auto controller=m_.word(address_+0x28),table=m_.word(controller+8),target=m_.word(table+0x3c),adjusted=controller+static_cast<std::int32_t>(m_.signed_half(table+0x38));
 if(target==0x1368e8)return 0; // proven original NullController return.
 if(target==0x234118)return hp2::debug::TrackedController(m_,adjusted).GetCurrentRoutePoint();
 if(target==0x18e740)return hp2::debug::AICarController(m_,adjusted).GetCurrentRoutePoint(services);
 hp2::not_implemented("0x00133340","Car route-point virtual target is not reconstructed");
}
}

namespace hp2::recovered {
// E-0099-W69. Original signed-delta route descriptors. Player's real tracked
// controller needs no host service mirror; unsupported AI localization stays
// explicit in this no-service projection rather than returning a fake route.
std::uint32_t Car::GetRoute()const{const auto controller=m_.word(address_+0x28),table=m_.word(controller+8),target=m_.word(table+0x34),self=controller+std::int32_t(m_.signed_half(table+0x30));if(target==0x1368f0)return 0;if(target==0x233ee0)return hp2::debug::TrackedController(m_,self).GetCurrentRoute();hp2::not_implemented("0x00133310","original controller GetCurrentRoute target requires unrecovered/no-service dependencies");}
std::uint32_t Car::GetRoutePoint()const{const auto controller=m_.word(address_+0x28),table=m_.word(controller+8),target=m_.word(table+0x3c),self=controller+std::int32_t(m_.signed_half(table+0x38));if(target==0x1368e8)return 0;if(target==0x234118)return hp2::debug::TrackedController(m_,self).GetCurrentRoutePoint();hp2::not_implemented("0x00133340","original controller GetCurrentRoutePoint requires typed AI localization dependencies");}
// TASK-0099 E-0099-W43. Original single-player catchup gate, without forcing
// boost or player-count globals to bypass the unresolved two-player consumer.
void Car::CalculateNewCatchupMultipliers(){if(m_.word(address_+0x18)&&m_.word(0x311f78+0x2f8)&&m_.word(0x327780)==2)hp2::not_implemented("0x001351F8","two-player RacingCar/difficulty catchup graph binding is not reconstructed");}
// Debug00131D58, E-0099-W43.
void Car::UpdatePosition(std::uint32_t){if(m_.word(address_+0x878))hp2::not_implemented("0x00131D58","espSetObjectPosition development packet binding is not reconstructed");}
// Debug001351E8, E-0099-W43.
std::uint32_t Car::GetLastPenetrationTime()const{return m_.word(m_.word(address_+0x34)+0x108);}
// Debug00132B28, E-0099-W43.
void Car::ChangeGear(int gear){const auto old=m_.signed_word(address_+0x450);if(old==gear)return;const auto driver=m_.word(address_+0x4b4);if(old!=1&&driver){hp2::debug::AnimatedDriver animation(m_,driver);if(old==0)animation.EndDrivingBackwards();animation.BeginGearChange();if(gear==0)animation.BeginDrivingBackwards();}m_.word(address_+0x450,std::uint32_t(gear));}
// Debug00131B90, E-0099-W43.
float Car::Move(float t,::port::race::PhysicsConstructionServices& services){
 CalculateNewCatchupMultipliers();::port::race::SharedTopologyContact own_contact(m_,address_+0x520);auto actual=services;actual.contact=&own_contact;const auto mover=m_.word(address_+0x30),table=m_.word(mover+4),target=m_.word(table+0x14),adjusted=mover+std::int32_t(m_.signed_half(table+0x10));float advanced;
 if(target==0x130b40)advanced=hp2::debug::NullMover(m_,adjusted).Move(t);
 else if(target==0x137d18)advanced=hp2::debug::PhysicsCar(m_,adjusted).Move(t,actual);
 else if(target==0x214958)advanced=hp2::debug::MellowMover(m_,adjusted).Move(t,actual);
 else if(target==0x1ffde0)advanced=hp2::debug::BeltMover(m_,adjusted).Move(t,actual); //E-0099-W106.
 else if(target==0x166fc0)advanced=hp2::debug::TrafficCar(m_,adjusted).Move(t,actual); //E-0099-W111.
 else if(target==0x19e8a8)advanced=hp2::debug::CopMover(m_,adjusted).Move(t,actual); //E-0110-CM01.
 else hp2::not_implemented("0x00131B90","unreconstructed CarMover Move virtual target");
 UpdatePosition(address_+0x60);ChangeGear(m_.signed_word(address_+0x3c8));
 // Debug00131BFC..00131C70,E-0110-F15. Source-requested deferred reset only.
 if(m_.word(address_+0x854)){
  if(!services.car_reset||!services.car_reset->ai)hp2::not_implemented("0x00131C38","Original rolling reset requires frame-owned routes/AI/physics reset context");
  const float forward=m_.scalar(address_+0x3c0),alignment=m_.scalar(address_+0x90)*m_.scalar(address_+0x7f8)+m_.scalar(address_+0x94)*m_.scalar(address_+0x7fc),maximum=m_.scalar(0x326fc8);
  if(!std::isfinite(forward)||!std::isfinite(alignment)||!std::isfinite(maximum))hp2::not_implemented("0x00131C28","Exceptional original rolling-reset MAX/MIN operands are not reconstructed");
  const float projected=(forward>0?forward:0.f)*(alignment>0?alignment:0.f);if(!std::isfinite(projected))hp2::not_implemented("0x00131C34","Exceptional original rolling-reset MUL is not reconstructed");const float speed=maximum<projected?maximum:projected;
  const auto route=GetRoute(*services.car_reset->ai),point=GetRoutePoint(*services.car_reset->ai);ResetToRoutePoint(route,point,speed,true,1,*services.car_reset);m_.word(address_+0x854,0);return advanced;
 }
 const auto penetration=GetLastPenetrationTime();if(IsBlinking()&&penetration&&penetration!=0x7fffffff){const auto now=m_.word(0x327cf8);const auto delay=m_.scalar(0x326fcc),scale=m_.scalar(0x324ce4);const float elapsed=float(std::int32_t(now-penetration))*scale;
  if(m_.scalar(address_+0x85c)-delay<float(std::int32_t(now-m_.word(address_+0x858)))*scale&&elapsed<delay&&elapsed>=0){const auto packed=delay*4000.f+.5f;if(!std::isfinite(packed)||packed>=2147483648.f||packed< -2147483648.f)hp2::not_implemented("0x00136D80","EE timer conversion outside supported finite signed range");m_.word(address_+0x858,m_.word(address_+0x858)+std::uint32_t(std::int32_t(packed)));}}
 return advanced;
}
}

#include "game_core/src/logitechwheel.hpp"
namespace hp2::recovered {
// TASK-0099 E-0099-W46. Actual state gates and XYZW impulse normalization.
// Inline IsReplayDemoMode/IsGameFlowPaused predicates are exact source state
// tests; wheel output is the actual pending force record, not device delivery.
void Car::StartForceImpact(std::uint32_t impulse,float magnitude){
 const auto coordinator=m_.word(0x327290);if(!coordinator)hp2::not_implemented("0x00134E90","original RaceCoordinator is required before force impact");const auto state=m_.word(coordinator),player=m_.word(address_+0x18);
 if(state-8<2||!player)return;if(state==4||state==7||state==5||state==9)return;
 if(m_.word(0x3275ac)==3||state==6||!m_.byte(player+0x1d)||m_.word(player+0x54))return;
 const float vx=m_.scalar(address_+0x70),vy=m_.scalar(address_+0x74),vz=m_.scalar(address_+0x78);const float speed_square=(vx*vx+vy*vy)+vz*vz;
 if(!std::isfinite(speed_square))hp2::not_implemented("0x00134E90","exceptional EE velocity sqrt operand");const auto speed=std::sqrt(speed_square);float factor=speed<5?-5.f:speed-5.f;
 if(factor<0)factor=0;else{if(factor>30)factor=30;factor*=m_.scalar(0x324d7c);}if(magnitude>10)magnitude=10;
 const float x=m_.scalar(impulse),y=m_.scalar(impulse+4),z=m_.scalar(impulse+8);const float square=(x*x+y*y)+z*z;
 if(!(square>0)||!std::isfinite(square))hp2::not_implemented("0x00134E90","degenerate or exceptional EE impact rsqrt operand");const auto reciprocal=1.f/std::sqrt(square);const auto strength=(magnitude*m_.scalar(0x324d80))*factor;
 for(unsigned i=0;i<4;++i)m_.scalar(impulse+i*4,m_.scalar(impulse+i*4)*reciprocal);
 hp2::debug::PortPlayCrash(m_,m_.signed_word(player+8),::port::driving::cvt_w_s(strength*100.f)+100,strength,m_.scalar(impulse+4));
}
}

namespace hp2::recovered {
// TASK-0099 E-0099-W48. Original source table/graph, candidate ordering and
// finite XYZ reductions. Temporary normalized W is not consumed by the dot.
float Car::CalculateDraftFactor(std::uint32_t other)const{
 const auto speed=m_.scalar(other+0x3c4);if(!(m_.scalar(0x2e7d08)<=speed))return 0;
 const float x=m_.scalar(other+0x60)-m_.scalar(address_+0x60),y=m_.scalar(other+0x64)-m_.scalar(address_+0x64),z=m_.scalar(other+0x68)-m_.scalar(address_+0x68);const float square=(x*x+y*y)+z*z;
 if(!std::isfinite(square))hp2::not_implemented("0x00131958","exceptional EE draft distance sqrt operand");const auto distance=std::sqrt(square);if(!(distance<=m_.scalar(0x2e7d1c)))return 0;
 const float vx=m_.scalar(other+0x70),vy=m_.scalar(other+0x74),vz=m_.scalar(other+0x78);const float velocity_square=(vx*vx+vy*vy)+vz*vz;
 const bool zero_domain=velocity_square==0||distance==0;
 if(velocity_square<0||!std::isfinite(velocity_square)||(zero_domain&&!::port::driving::pc_sqrt_carrier_enabled()))hp2::not_implemented("0x00131958",("degenerate EE draft rsqrt/distance operand: distance="+std::to_string(distance)+" velocity_squared="+std::to_string(velocity_square)+" subject="+std::to_string(address_)+" other="+std::to_string(other)).c_str());
 // E-0110-D01: original VRSQRT001319E8 and DIV.S00131A24 do not trap
 // on zero. Keep signed zero/direction and the full cone/table path, rather
 // than skipping coincident cars or inventing a normalized direction.
 const float reciprocal=velocity_square==0?::port::driving::ee_vrsqrt_one_zero(velocity_square,"0x001319E8"):1.f/std::sqrt(velocity_square);
 const float dot=(x*(vx*reciprocal)+y*(vy*reciprocal))+z*(vz*reciprocal);
 const float cosine=distance==0?::port::driving::ee_div_s_zero(dot,distance,"0x00131A24"):dot/distance;
 if(distance==0)::port::diagnostics::warn("0x00131A24","original coincident-car DIV.S uses extended EE maximum encoding","using the documented finite PC carrier; signed result and subsequent source cone/table decisions are preserved");
 const float angular=cosine-m_.scalar(0x324cd4);
 const float factor=zero_domain?::port::driving::ee_mul_s_finite(angular,m_.scalar(0x324cd8),"0x00131A2C"):angular*m_.scalar(0x324cd8);
 if(factor>=0){const auto range=hp2::recovered::Graph(m_,0x326f98).GetValue(speed);const float denominator=zero_domain?::port::driving::ee_mul_s_finite(factor,range,"0x00131A58"):factor*range;const float parameter=denominator==0?::port::driving::ee_div_s_zero(distance,denominator,"0x00131A68"):distance/denominator;return hp2::recovered::Table(m_,0x2e7d20).GetValue(parameter);}return 0;
}
float Car::CalculateDraftFactor()const{
 const auto world=m_.word(0x3272f0);if(!world)hp2::not_implemented("0x00131A80","actual World is required for draft candidates");float total=0;int i=0;do{if(m_.signed_word(world+0x1c)<=i)break;if(i>=14)hp2::not_implemented("0x00131A80","original World candidate table exceeds14Cars");const auto other=m_.word(world+0x2c+std::uint32_t(i)*4);if(other!=address_)total+=CalculateDraftFactor(other);++i;}while(total<m_.scalar(0x324cdc));const auto cap=m_.scalar(0x324ce0);if(!std::isfinite(total)||!std::isfinite(cap))hp2::not_implemented("0x00131A80","exceptional EE draft MIN.S operand");return total<cap?total:cap;
}
}

namespace hp2::recovered {
// TASK-0099 E-0099-W48. Fresh MIPS preserves f12 through the driver call,
// despite decompiler omission. Decode actual descriptor and signed adjustment.
void Car::Drive(float t,::port::driving::PlayerServices* player_services){const auto driver=m_.word(address_+0x2c),table=m_.word(driver+0x22c),target=m_.word(table+0x14),adjusted=driver+std::int32_t(m_.signed_half(table+0x10));
 if(target==0x136b90)return; // original NullDriver JR RA / NOP.
 if(target==0x139b58){hp2::debug::PsychoDriver(m_,adjusted).Drive(t);return;}
 if(target==0x13b200){if(!player_services)hp2::not_implemented("0x0013B200","actual RealDriver primitive input/replay services are required");RealDriver(m_,adjusted).Drive(t,*player_services);return;}
 hp2::not_implemented("0x00131B60","Car driver Drive virtual target is not reconstructed");
}
}

namespace hp2::recovered {
// Debug00131B10, E-0099-W48. Original draft publication precedes virtual Think.
void Car::Think(float t,const ::port::race::AiDriveTargetServices& ai,::port::race::PhysicsConstructionServices& physics){m_.scalar(address_+0x458,CalculateDraftFactor());const auto controller=m_.word(address_+0x28),table=m_.word(controller+8),target=m_.word(table+0x24),adjusted=controller+std::int32_t(m_.signed_half(table+0x20));
 if(target==0x136900)return; // original NullController JR RA / NOP.
 if(target==0x1302d8){hp2::debug::AIDriver(m_,adjusted).Think(t,ai,physics);return;}
 if(target==0x18f0e0){hp2::debug::AICarController(m_,adjusted).Think(t,ai,physics);return;}
 if(target==0x234798){hp2::debug::RealController(m_,adjusted).Think(t,ai);return;}
 if(target==0x18a600){hp2::debug::TrafficDriver(m_,adjusted).Think(t,ai,physics);return;}
 if(target==0x19f980){if(!ai.routes)hp2::not_implemented("0x00233DD0","actual CopDriver tracked Think requires shared route resources");hp2::debug::CopDriver(m_,adjusted).Think(t,*ai.routes,&physics,&ai);return;}
 hp2::not_implemented("0x00131B10","Car controller Think virtual target is not reconstructed");
}
}

namespace hp2::recovered {
// Debug00135AF8, E-0099-W47. The original low-speed branch returns the full
// acceleration magnitude; only the moving branch projects onto velocity XYZ.
float Car::GetForwardAcceleration()const{const float ax=m_.scalar(address_+0x80),ay=m_.scalar(address_+0x84),az=m_.scalar(address_+0x88);if(m_.scalar(address_+0x3c4)<m_.scalar(0x324d90)){const float square=(ax*ax+ay*ay)+az*az;if(!std::isfinite(square))hp2::not_implemented("0x00135AF8","exceptional EE acceleration sqrt operand");return std::sqrt(square);}const float x=m_.scalar(address_+0x70),y=m_.scalar(address_+0x74),z=m_.scalar(address_+0x78);const float square=(x*x+y*y)+z*z;if(!(square>0)||!std::isfinite(square))hp2::not_implemented("0x00135AF8","degenerate or exceptional EE velocity rsqrt operand");const float reciprocal=1.f/std::sqrt(square);return ((x*reciprocal)*ax+(y*reciprocal)*ay)+(z*reciprocal)*az;}
}

#include "game_core/indep/src/copmgr.hpp"
namespace hp2::recovered {
// Debug001330D8, E-0099-W47. Actual source pursuit object, no success callback.
void Car::PleaseResetMeLater(){if(!m_.word(0x3121ec)||m_.signed_word(0x3121d4)<1||m_.word(m_.word(address_+0x14)+8)!=2||!hp2::debug::tCopManager(m_,m_.word(0x328c1c)).IsBeingPursued(address_))m_.word(address_+0x854,1);}
}

namespace hp2::recovered {
// Debug00134708, E-0099-W58. Original nullable Player gate, PlayerNumber and
// direct State.Acceleration pointer; no host camera callback or per-car mirror.
void Car::MaybeCameraShake(std::optional<std::uint32_t> port_stack_shake_w){const auto player=m_.word(address_+0x18);if(player)hp2::debug::MaybeCameraShake(m_,m_.signed_word(player+0x10),address_+0x80,port_stack_shake_w);}
}

#include "game_core/indep/src/fernflattener.hpp"
#include "game_core/indep/inc/Player.hpp"
namespace hp2::recovered {
// Debug00133628, E-0099-W64. Original future geometry position and actual
// event tree/queue. No externally supplied successful gameplay callback.
void Car::TriggerEvents(std::optional<std::uint32_t> port_stack_vector_lane){auto& m=m_;const auto car=address_;std::array<float,3> future{};for(unsigned i=0;i<3;++i)future[i]=m.scalar(car+0x50+4*i)+m.scalar(car+0x70+4*i)*m.scalar(0x324d28);const auto results=hp2::debug::emTriggerEvents(m,future);if(!results)return;
 for(unsigned i=0;;++i){const auto event=m.word(results+4*i);if(!event)break;const auto id=m.word(event+0xc);
  if(id==0x10003){const auto code=m.word(m.word(event+0x10)+8);hp2::debug::FlattenFern(m,code);hp2::debug::ShakeFern(m,code,car+0x70,port_stack_vector_lane);if(m.word(car+0x18)&&((m.word(0x327d10)+3)&3)==0&&m.scalar(car+0x3c4)>30){const auto replay=m.word(0x3271a8);if(!replay)hp2::not_implemented("0x0013E070","original fern event requires ReplayManager owner");ReplayManager(m,replay).RegisterInterestingEvent(2,"FlattenFern");}}
  else if(id==0x10005){const auto player=m.word(car+0x18);if(player){const auto parameter=m.word(m.word(event+0x10)+8),direction=m.word(0x311f84);if(((direction==0&&!(parameter&255))||(direction==1&&(parameter&255)))&&m.scalar(car+0x3c4)>40&&!m.word(player+0x2f4))hp2::debug::Player(m,player).TriggerVisibleDrivingLine(int(parameter>>16),float((parameter>>8)&255));}}
  else hp2::debug::emRemoveEvent(m,event);
 }
}
// Debug00133980, E-0099-W64. Original JR RA/NOP; not an unrecovered handler.
void Car::EventHandler(std::uint32_t){}
}
namespace hp2::debug {
// Debug00130B70, E-0099-W64. Original raw CarPtr gate and direct child call.
void CarReflectEventHandler(::port::driving::Memory& m,std::uint32_t event){if(const auto car=m.word(event+0x14))hp2::recovered::Car(m,car).EventHandler(event);}
}

namespace hp2::recovered {
// TASK-0099 E-0099-W66, Debug001343D0. Actual source-owned feedback queues.
void Car::MaybeRumble(){
 const auto player=m_.word(address_+0x18);if(!player)return;
 const auto racing=m_.word(address_+0x1c);if(racing&&m_.signed_half(racing+0x12))return;
 const int port=m_.signed_word(player+8);
 const auto sum=[&](unsigned offset){float value=0;for(unsigned i=0;i<4;++i)value+=m_.scalar(address_+offset+4*i);return value;};
 hp2::debug::PortRumbleShockSway(m_,port,(sum(0x310)*.25f)/m_.scalar(m_.word(address_+0xc)+0x1e0));
 hp2::debug::PortRumbleRoadNoise(m_,port,sum(0x320)*hp2::recovered::Graph(m_,0x327010).GetValue(m_.scalar(address_+0x3c4)));
 hp2::debug::PortRumbleShockBottom(m_,port,sum(0x330)*.25f);
 const int blown=std::int8_t(m_.byte(address_+0x849));
 if(blown>0&&!hp2::debug::Player(m_,player).IsNisPlaying()){
  bool grounded=false;for(unsigned i=0;i<4;++i)if(m_.byte(address_+0x350+i)&&(m_.byte(address_+0x848)&(1u<<i)))grounded=true;
  const auto timer=m_.word(address_+0x84c);
  if(grounded&&timer&&timer!=0x7fffffffu){const float decay=1.f-float(std::int32_t(m_.word(0x327cf8)-timer))*m_.scalar(0x324d4c)*m_.scalar(0x324d50);
   if(decay>0){const auto v=m_.vector(address_+0x70);const float square=(v[0]*v[0]+v[1]*v[1])+v[2]*v[2];if(!std::isfinite(square))hp2::not_implemented("0x001345C0","exceptional EE blown-tire velocity sqrt");hp2::debug::PortRumbleBlownTires(m_,port,(std::sqrt(square)/float(blown))*decay);}}
 }
 const auto previous=m_.byte(address_+0x874);unsigned bumps=0;
 for(unsigned i=0;i<4;++i)if(m_.word(address_+0x364+4*i)-30u<2u)bumps|=1u<<i;
 if((bumps|previous)!=previous){const float speed=m_.scalar(address_+0x3c4);if(13.f<speed){if(((bumps|previous)&3u)!=(previous&3u))hp2::debug::PortRumbleDoubleRoadBump(m_,port,1);}else if(5.f<speed)hp2::debug::PortRumbleSingleRoadBump(m_,port,1);}
 m_.byte(address_+0x874,std::uint8_t(bumps));
}
// Debug00134730: no Player means the original genuine empty path.
void Car::DoWheelForces(){const auto player=m_.word(address_+0x18);if(player)CalcForceParameters(m_.signed_word(player+8));}
// Debug00134750. Dead bReset operands are erased by the leaf projection;
// caller stack fFadeLevel is preserved. No ForceFeedbackTask/device claim.
void Car::CalcForceParameters(int port){
 const auto player=m_.word(address_+0x18),coordinator=m_.word(0x327290);const auto state=coordinator?m_.word(coordinator):0;
 const auto condition=[&](int type,float offset,float dead,float ns,float ps,float nc,float pc){hp2::debug::PortSetForceConditionData(m_,type,port,offset,dead,ns,ps,nc,pc);};
 const auto periodic=[&](int type,float magnitude,float direction,float period){hp2::debug::PortSetForcePeriodicData(m_,type,port,magnitude,direction,period,0,0,0,0,0,0);};
 if(!player||m_.word(0x3275ac)==3||(coordinator&&(state==4||state==7||state==5||state==9||state==6))||!m_.byte(player+0x1d)){
  condition(0,0,0,1.5f,1.5f,1,1);condition(1,0,0,0,0,1,1);periodic(4,0,0,0);periodic(3,0,0,0);hp2::debug::PortSetOverallForceGain(m_,port,1);return;
 }
 const auto driver=m_.word(address_+0x2c);if(!driver)hp2::not_implemented("0x00132BA0","original Car.GetSteering requires CarDriver");
 float steering=float(m_.half(driver+0x210))*.0054931640625f;steering=steering<180?-steering:360-steering;steering*=m_.scalar(0x324d54);
 const auto v=m_.vector(address_+0x70);const float square=(v[0]*v[0]+v[1]*v[1])+v[2]*v[2];if(!std::isfinite(square))hp2::not_implemented("0x00134990","exceptional EE wheel velocity sqrt");
 float speed=std::sqrt(square);if(speed<5)speed=0;speed-=5;if(speed<0)speed=0;if(30<speed)speed=30;speed*=m_.scalar(0x324d58);
 const float dead=m_.scalar(0x324d5c)-speed*m_.scalar(0x324d5c),traction=GetWheelTraction();float positive=0,negative=0;
 if(m_.scalar(0x324d60)<=traction){positive=speed;negative=speed;}
 if(m_.scalar(0x324d64)<traction&&m_.scalar(0x324d68)<speed){float loss=(traction-m_.scalar(0x324d64))*m_.scalar(0x324d6c);if(1<loss)loss=1;positive-=loss*m_.scalar(0x324d70);negative-=loss*m_.scalar(0x324d70);}
 float bottom=0;for(unsigned i=0;i<4;++i)bottom+=m_.scalar(address_+0x330+i*4);
 if(0<bottom){negative=1;positive=1;hp2::debug::PortPlayShake(m_,port,bottom*.25f);const float packed=(float(std::int32_t(m_.word(0x327cf8)))*m_.scalar(0x324d74)+1.f)*4000.f+.5f;m_.word(address_+0x884,std::uint32_t(::port::driving::cvt_w_s(packed)));}
 condition(0,0,dead,negative*speed,positive*speed,1,1);condition(1,0,speed,1-speed,1-speed,1,1);hp2::debug::PortSetOverallForceGain(m_,port,GetWheelsOnGround());
 const auto zone=m_.signed_word(player+0x54);
 if(zone==0||zone==6||zone==7){float excess=GetWheelTraction()-m_.scalar(0x324d78);if(excess<0)excess=0;float gain=excess*50;if(1<gain)gain=1;periodic(4,speed*gain,-steering,100);
  int wet=0;float frequency=0,magnitude=0;for(unsigned i=0;i<4;++i){const auto terrain=m_.word(address_+0x354+i*4);if(terrain-4u<2u||terrain==20)++wet;if(terrain>=33)hp2::not_implemented("0x00134D18","wheel terrain outside original TerrainTypeInfoTable");const auto row=0x3102e8+terrain*0x90;frequency+=m_.scalar(row+0x34);magnitude+=m_.scalar(row+0x30);}
  periodic(3,((speed*magnitude)*.25f)*.5f,90,frequency*.25f);if(wet>1){condition(0,0,0,1.5f,1.5f,1,1);condition(1,0,0,0,0,1,1);}
 }else{periodic(4,0,0,0);periodic(3,0,0,0);}
}
}

#include "game_core/port/race/car_sound_construction.hpp"
// TASK-0099 E-0099-W68. Fresh complete Debug00133988/00133590 MIPS and types;
// original ../indep/src/car.cpp, SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
namespace hp2::recovered {
void Car::TurnWipersOnForDuration(int start,int end){auto&m=m_;const auto a=address_;if(m.word(a+0x840))return;int on=m.signed_word(a+0x838),off=m.signed_word(a+0x83c);const int now=m.signed_word(0x327d04);if(on<now){if(off<=now){on=start;m.word(a+0x838,std::uint32_t(start));}}else if(start<on){on=start;m.word(a+0x838,std::uint32_t(start));}if(off<end){off=end;m.word(a+0x83c,std::uint32_t(end));}const auto numerator=std::int32_t(std::uint32_t(off)-std::uint32_t(on)+0x51u);const int periods=numerator/0x52;m.word(a+0x83c,std::uint32_t(periods)*0x52u+std::uint32_t(on));}
void Car::TriggerCarTerrainEffects(float distance,std::optional<std::uint32_t> policy){auto&m=m_;const auto car=address_;const float speed=m.scalar(car+0x3c4)*m.scalar(0x324d2c);unsigned water=0;bool deep=false;unsigned grounded=0;
 const auto row=[&](unsigned terrain){if(terrain>=33)hp2::not_implemented("0x00133988","Original terrain enum exceeds33-row shared table");return 0x3102e8+terrain*0x90;};
 const auto clamp=[](float value,const char* site){if(!std::isfinite(value))hp2::not_implemented(site,"Original exceptional EE terrain-effect arithmetic is outside the recovered finite native projection");return std::fmin(1.f,std::fmax(value,0.f));};
 const auto ratio=[&](float numerator,float denominator,const char* site){if(denominator==0)hp2::not_implemented(site,"Original EE terrain-effect zero divisor requires its reached instruction semantics");return numerator/denominator;};
 const auto enabled=[&](unsigned at){const auto name=m.word(at);return name&&m.signed_byte(name)!=0;};
 const auto emit=[&](int effect,float intensity,int parameter,unsigned velocity=0){TriggerEffect(effect,intensity,parameter,velocity,policy);};
 for(unsigned wheel=0;wheel<4;++wheel){if(!m.signed_byte(car+0x350+wheel))continue;++grounded;const auto terrain=m.word(car+0x354+4*wheel),info=row(terrain);if(terrain-4u<2u||terrain==20){water|=1u<<wheel;deep|=terrain==5;}
  const float x=m.scalar(car+0x374+8*wheel),y=m.scalar(car+0x378+8*wheel);const float xx=x*x,yy=y*y;const float norm=xx+yy;if(!std::isfinite(norm))hp2::not_implemented("0x00133AB0","Original exceptional EE wheel-slip SQRT is outside recovered finite projection");const float slip=std::sqrt(norm);const int parameter=int(((wheel+5)<<16)+terrain);const auto count_at=car+0x870+wheel;
  if(distance<150&&m.scalar(info+0x58)<slip){const unsigned count=std::min(unsigned(m.byte(count_at))+1,255u);m.byte(count_at,std::uint8_t(count));if(enabled(info+0x54)){float intensity=ratio(slip-m.scalar(info+0x58),m.scalar(info+0x5c)-m.scalar(info+0x58),"0x00133B20");if(count<2)intensity=0;else if(count<20)intensity*=.25f;else if(count<60)intensity*=.5f;else if(count<120)intensity*=.75f;intensity=clamp(intensity,"0x00133B80");if(0<intensity)emit(3,intensity,parameter);}}
  else m.byte(count_at,0);
  if(distance<40&&wheel>1&&m.scalar(info+0x64)<x&&enabled(info+0x60)){const float intensity=clamp(ratio(x-m.scalar(info+0x64),m.scalar(info+0x68)-m.scalar(info+0x64),"0x00133C24"),"0x00133C28");if(0<intensity){::port::driving::Memory::Temporary storage(m,16);::port::driving::SolverVector velocity{};hp2::recovered::bMulMatrix(velocity,m.matrix(car+0xf0+wheel*0x40),{{x,y,0,0}},::port::driving::SolverMathContext{m.vector(0x317b20)});const auto body=m.vector(car+0x70);for(unsigned lane=0;lane<4;++lane)velocity[lane]+=body[lane];m.vector(storage.address(),velocity);emit(4,intensity,parameter,storage.address());}}
 }
 if(grounded>2){int terrain;hp2::debug::TopologyCoordinate(m,car+0x520).GetElevation({{m.scalar(car+0x60),m.scalar(car+0x64),m.scalar(car+0x68)}},&terrain,nullptr,nullptr);const auto info=row(unsigned(terrain));float first=0,second=0;if(enabled(info+0x3c))first=clamp(ratio(speed-m.scalar(info+0x40),m.scalar(info+0x44)-m.scalar(info+0x40),"0x00133D20"),"0x00133D24");if(enabled(info+0x48))second=clamp(ratio(speed-m.scalar(info+0x4c),m.scalar(info+0x50)-m.scalar(info+0x4c),"0x00133D58"),"0x00133D60");if(0<first+second){const float excess=(first+second)-1;if(0<excess){if(m.scalar(info+0x40)<m.scalar(info+0x4c))first=std::fmax(first-(excess+excess),0.f);else second=std::fmax(second-(excess+excess),0.f);}if(0<first)emit(1,first,terrain);if(0<second)emit(2,second,terrain);}
  float bottom=0;for(unsigned wheel=0;wheel<4;++wheel)bottom=bottom+m.scalar(car+0x330+4*wheel);const float intensity=clamp(ratio(bottom-m.scalar(info+0x70),m.scalar(info+0x74)-m.scalar(info+0x70),"0x00133E70"),"0x00133E74");if(0<intensity)emit(5,intensity,terrain);
 }
 if(distance<60&&water&&10<m.scalar(car+0x3c4)){const auto now=m.word(0x327d04);TurnWipersOnForDuration(int(now+0x3c),int(now+0xb4));if(const auto sound=m.word(car+0x850))hp2::debug::BaseCarSound(m,sound).TriggerSplash((water&6)!=0,deep?3:1);if(m.word(car+0x18)&&((m.word(0x327d10)+3)&3)==0&&30<m.scalar(car+0x3c4)){const auto replay=m.word(0x3271a8);if(!replay)hp2::not_implemented("0x0013E070","Original water effect requires actual ReplayManager owner");if(!deep)ReplayManager(m,replay).RegisterInterestingEvent(1.25f,"InShallowWater");else if(!m.word(car+0x88c)){ReplayManager(m,replay).RegisterInterestingEvent(10,"InDeepWater");m.word(car+0x88c,1);}}}
 if(!deep)m.word(car+0x88c,0);
 if(const auto damage=m.word(car+0x844)){const auto body=damage+8;if(m.scalar(0x324d30)<hp2::debug::BodyDamage(m,body).GetVisibleTotalDamage())emit(7,1,0);else if(hp2::debug::CarDamage(m,damage).GetDamageAmount(m.word(damage+0xcc),14)==3&&m.scalar(0x324d34)<hp2::debug::BodyDamage(m,body).GetVisibleTotalDamage())emit(6,1,0);}
 bool nitro=false;if(m.word(car+0x18)&&m.word(car+0x24)==1){const auto engine=m.word(m.word(car+0x30)+0x40);nitro=m.word(engine+0x50)!=0;}if(m.word(car+0x810)||nitro)emit(10,1,0);
 if(m.signed_byte(car+0x849))for(unsigned wheel=0;wheel<4;++wheel)if(m.signed_byte(car+0x350+wheel)&&((m.signed_byte(car+0x848)>>wheel)&1)){const float intensity=clamp(((m.scalar(car+0x3c4)*m.scalar(0x324d38))-m.scalar(0x324d3c))*m.scalar(0x324d40),"0x00134130");emit(9,intensity,int((wheel+5)<<16));}
}
}

namespace hp2::recovered {
// Debug00133180/00133148, E-0099-W70; actual WorldTimer, duration and fullword flags.
void Car::StartBlinking(float duration){m_.scalar(address_+0x85c,duration);m_.word(address_+0x858,m_.word(0x327cf8));}
void Car::BlinkBlinkPoof(){StartBlinking(2.f);m_.word(address_+0x86c,0);m_.word(address_+0x868,1);}
// Debug00132B20, E-0099-W67; instruction-confirmed original empty function.
void Car::FakeEngineTask(){}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/CarDriver.hpp"
#include "game_core/indep/src/drivetarget.hpp"
namespace hp2::debug {
// Debug00130AA0; E-0099-W74/W104; exact original car.cpp definition marker/TU.
void CarDriver::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2a6498);s.MoveData(address_+0x210,2);
 for(unsigned off:{0x214u,0x218u,0x21cu})s.MoveData(address_+off,4);
 s.MoveData(address_+0x220,1,4,1);DriveTarget(m_,address_).DoSnapshot(snapshot);s.EndSection();
}
// Debug00130B38, E-0099-W105; exact car.cpp definition marker, JR RA/NOP.
void CarMover::DoSnapshot(std::uint32_t){}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/port/race/damage_resources.hpp"
#include "game_core/port/race/control_construction.hpp"
#include "game_core/port/race/traffic_construction.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/inc/AnimatedDriver.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include <cstdio>
namespace hp2::recovered {
// Debug00131108; E-0099-W74; replay-frame/00131108.json/type-Car.json/car-tail-exact-listing.json.
void Car::DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& services){
 auto& m=m_;const auto a=address_;hp2::debug::ReplaySnapshot s(m,snapshot);
 ::port::driving::Memory::Temporary name(m,16);char text[16];std::snprintf(text,sizeof(text),"Car%d",int(m.signed_half(a+8)));
 for(unsigned i=0;;++i){m.byte(name.address()+i,static_cast<unsigned char>(text[i]));if(!text[i])break;}s.BeginSection(name.address());
 s.MoveData(a+0x808,1);s.MoveData(a+0x80c,1,4,0);
 for(unsigned off:{0x820u,0x824u,0x828u})s.MoveData(a+off,4);
 for(unsigned off:{0x830u,0x82cu,0x814u,0x818u,0x81cu})s.MoveData(a+off,1,4,0);
 s.MoveData(a+0x834,1,4,0);s.MoveData(a+0x838,4,4,1);s.MoveData(a+0x83c,4,4,1);s.MoveData(a+0x840,1,4,0);
 s.MoveData(a+0x848,1);s.MoveData(a+0x849,1);s.MoveData(a+0x84c,4);
 if(const auto damage=m.word(a+0x844))hp2::debug::CarDamage(m,damage).DoSnapshot(snapshot);
 s.MoveData(a+0x20,1,4,0);s.MoveData(a+0x24,1,4,0);const auto ride=m.word(a+0x10);const auto old_skin=m.byte(ride+5);s.MoveData(ride+5,1);
 if(m.byte(snapshot+0x3d)){
  // E-0099-W110 port unknown-domain boundary: original reconstructed spline
  // has unwritten heap W; XYZ-only codec cannot establish consumed fourth lanes.
  if(m.signed_word(a+0x24)==6)hp2::not_implemented("0x001FF4D8","fresh Belt QuickSpline XYZ-only restoration leaves original heap W unresolved; no explicit heap-lane policy");
  if(!services.routes||!services.physics)hp2::not_implemented("0x00131108","Snapshot mode reconstruction requires concrete routes and physics primitives");
  const auto control=m.signed_word(a+0x20),movement=m.signed_word(a+0x24);
  ::port::race::SharedTopologyContact contact(m,a+0x520);auto physics=*services.physics;physics.contact=&contact;
  SetControlMode(0,*services.routes);SetMovementMode(0,physics);SetControlMode(control,*services.routes);SetMovementMode(movement,physics);
  if(old_skin!=m.byte(ride+5))hp2::not_implemented("0x00131108","Original restored SwitchSkin child is not reconstructed");
 }
 const auto controller=m.word(a+0x28),ce=m.word(controller+8)+0x18,ct=m.word(ce+4),ca=controller+static_cast<std::uint32_t>(m.signed_half(ce));
 if(ct==0x1bd1e8)hp2::debug::CarController(m,ca).DoSnapshot(snapshot);
 else if(ct==0x2342a8)hp2::debug::RealController(m,ca).DoSnapshot(snapshot);
 else if(ct==0x233e38)hp2::debug::TrackedController(m,ca).DoSnapshot(snapshot);
 else if(ct==0x130258)hp2::debug::AIDriver(m,ca).DoSnapshot(snapshot);
 else if(ct==0x18e430)hp2::debug::AICarController(m,ca).DoSnapshot(snapshot);
 else if(ct==0x1a03b8)hp2::debug::CopDriver(m,ca).DoSnapshot(snapshot);
 else if(ct==0x18a5a0)hp2::debug::TrafficDriver(m,ca).DoSnapshot(snapshot);
 else hp2::not_implemented("0x00131108","Controller snapshot virtual target");
 const auto mover=m.word(a+0x30),me=m.word(mover+4)+0x18,mt=m.word(me+4),ma=mover+static_cast<std::uint32_t>(m.signed_half(me));
 if(mt==0x137410)hp2::debug::PhysicsCar(m,ma).DoSnapshot(snapshot,services);
 else if(mt==0x2144e8)hp2::debug::MellowMover(m,ma).DoSnapshot(snapshot,services);
 else if(mt==0x1ff4d8)hp2::debug::BeltMover(m,ma).DoSnapshot(snapshot); //E-0099-W110.
 else if(mt==0x166f28)hp2::debug::TrafficCar(m,ma).DoSnapshot(snapshot);
 else if(mt==0x19e798)hp2::debug::CopMover(m,ma).DoSnapshot(snapshot); //E-0110-CM01.
 else if(mt!=0x130b38)hp2::not_implemented("0x00131108","Mover snapshot virtual target"); //00130B38 is confirmed empty.
 CarState(m,a+0x40).DoSnapshot(snapshot);
 const auto body=m.word(a+0x34),be=m.word(body+0x50)+0x10;
 if(m.word(be+4)!=0x1d4c90)hp2::not_implemented("0x00131108","Car collision-body snapshot virtual target");
 hp2::debug::CarCollisionBody(m,body+static_cast<std::uint32_t>(m.signed_half(be))).DoSnapshot(snapshot);
 if(const auto driver=m.word(a+0x4b4))hp2::debug::AnimatedDriver(m,driver).DoSnapshot(snapshot);
 s.MoveData(a+0x4a0,4);s.MoveData(a+0x450,1);s.MoveData(a+0x454,4);s.MoveData(a+0x4a8,1);s.MoveData(a+0x4ac,1,4,0);s.MoveData(a+0x4a4,4);
 if(!services.routes)hp2::not_implemented("0x00203650","Snapshot requires actual loaded path point table");
 hp2::debug::TrackPathCoordinate::DoSnapshot(m,a+0x7e0,snapshot,services.routes->paths);
 s.MoveData(a+0x854,1,4,1);s.MoveData(a+0x868,1,4,0);s.MoveData(a+0x86c,1,4,0);
 s.MoveData(a+0x858,4);s.MoveData(a+0x85c,4);s.MoveData(a+0x860,8);s.MoveData(a+0x870,4,4,0);
 if(m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d)){m.word(a+0x458,0);m.word(a+0x60,m.word(a+0x60)&0xfffffff0u);}
 s.MoveData(a+0x810,1,4,0);s.MoveData(a+0x45c,4);s.MoveData(a+0x460,4);s.EndSection();
}
}

namespace hp2::recovered {
// Debug001332C8, E-0099-W91. JR RA / LW v0,0x868(a0); full word consumed by traffic caller.
std::uint32_t Car::IsPoofed()const{return m_.word(address_+0x868);}
}

namespace hp2::recovered {
// Debug00134198, E-0099-W93. Strict source cooldown and fullword booleans.
void Car::HonkAndOrBlinkHeadlights(float on,float off,bool blink,bool honk,bool twice){auto& m=m_;const auto a=address_;const float elapsed=static_cast<float>(static_cast<std::int32_t>(m.word(0x327cf8)-m.word(a+0x820)))*m.scalar(0x324d44);if((m.scalar(a+0x824)+m.scalar(a+0x828))+m.scalar(a+0x824)<elapsed){m.word(a+0x81c,twice);m.word(a+0x820,m.word(0x327cf8));m.scalar(a+0x824,on);m.scalar(a+0x828,off);m.word(a+0x814,blink);m.word(a+0x818,honk);}}
}

#include "game_core/indep/inc/Sound3d.hpp"
namespace hp2::recovered {
// Debug00132DF0, E-0099-W122; original car.cpp. Resource checks are host binding
// prerequisites; original mutation order follows. The normalize stack W is
// required at its full-vector consumer, after the real ResetCar/PutFlat prefix.
void Car::ResetCore(std::uint32_t newpos,float speed,std::uint16_t angle,const ::port::race::CarResetServices& services){
 if(!services.routes||!services.routes->initialized||!services.physics)hp2::not_implemented("0x00132DF0","actual route and physics owners are required for Car.ResetCore");
 auto& m=m_;const auto a=address_;const auto mode=m.signed_word(a+0x24);SetMovementMode(0,*services.physics);CarState state(m,a+0x40);state.ResetCar();
 hp2::debug::TrackPathCoordinate::UpdatePosition(m,a+0x7e0,services.routes->paths,{m.scalar(newpos),m.scalar(newpos+4)});m.half(a+0x3a4,angle);m.half(a+0x3a6,0);m.copy(a+0x60,newpos,16);m.scalar(a+0x3c0,speed);hp2::debug::TopologyCoordinate(m,a+0x520).ClearInterestBBox();::port::race::SharedTopologyContact contact(m,a+0x520);PutFlatOnTrack(contact);
 const auto ordinary=[](float x){return std::isfinite(x)&&(x==0||std::isnormal(x));};
 const std::array<float,3> forward{{m.scalar(a+0x90),m.scalar(a+0x94),m.scalar(a+0x98)}};const float length=speed+m.scalar(0x324d20);
 if(!ordinary(length)||length==0||!ordinary(forward[0])||!ordinary(forward[1])||!ordinary(forward[2]))hp2::not_implemented("0x00132E74","source reset normalization exceptional/zero-length EE operands require separate evidence");
 const auto velocity=bNormalize(forward,length);for(float v:velocity)if(!ordinary(v))hp2::not_implemented("0x00132E7C","source reset normalization result outside ordinary native scalar projection");
 if(!services.reset_velocity_w)hp2::not_implemented("0x00132E7C","original ResetCore normalized-velocity stack W requires its own explicit port operand");
 for(unsigned i=0;i<3;++i)m.scalar(a+0x70+4*i,velocity[i]);m.word(a+0x7c,*services.reset_velocity_w);float squared=velocity[0]*velocity[0];squared=squared+velocity[1]*velocity[1];squared=squared+velocity[2]*velocity[2];m.scalar(a+0x3c4,std::sqrt(squared));m.word(a+0x440,0);
 const auto info=m.word(a+0xc);for(unsigned i=0;i<4;++i){const float radius=m.scalar(info+0x12c+0x20*i);if(!ordinary(speed)||!ordinary(radius)||radius==0)hp2::not_implemented("0x00132ED0","source reset wheel DIV exceptional/zero radius EE operands require separate evidence");const float angular=speed/radius;if(!ordinary(angular)||(speed!=0&&angular==0))hp2::not_implemented("0x00132ED0","source reset wheel DIV overflow/underflow outside normal native scalar projection");m.scalar(a+0x2f0+4*i,angular);}state.CalculateWheelMatrices();
 const auto driver=m.word(a+0x2c),table=m.word(driver+0x22c),low_gear=m.word(table+0x34),driver_self=driver+std::int32_t(m.signed_half(table+0x30));
 if(low_gear==0x13b1d8)RealDriver(m,driver_self).SetLowGear();
 else if(low_gear==0x13aae8)hp2::debug::PsychoDriver(m,driver_self).SetLowGear();
 else if(low_gear!=0x136b68)hp2::not_implemented("0x00132EF8","Original ResetCore CarDriver.SetLowGear virtual target is not reconstructed");
 // NullDriver.SetLowGear00136B68 is a confirmed JR RA/NOP, without stores.
 SetMovementMode(mode,*services.physics);
 if(m.word(a+0x18)){const auto replay=m.word(0x3271a8);if(!replay)hp2::not_implemented("0x0013E0F0","actual player reset ReplayManager owner required");ReplayManager(m,replay).InvalidateInterestingEvents();}
}
// Debug00132FB0, E-0099-W122. The route-local W preflight is explicitly a port
// policy at the call boundary after ResetPosition/heading; it is not a claim
// that original ResetCore mutations were atomic or absent on unknown storage.
void Car::ResetToRoutePoint(std::uint32_t route,std::uint32_t point,float speed,bool blinking,float direction,const ::port::race::CarResetServices& services){
 if(!route||!point)return;if(!services.routes||!services.routes->initialized)hp2::not_implemented("0x00132FB0","actual route owner required for Car.ResetToRoutePoint");auto& m=m_;::port::driving::Memory::Temporary local(m,16);const auto position=local.address();for(unsigned i=0;i<3;++i)m.word(position+4*i,m.word(point+4*i));hp2::debug::TrackPathCoordinate::ResetPosition(m,address_+0x7e0,services.routes->paths,{m.scalar(point),m.scalar(point+4)},m.scalar(point+0x28));const auto angle=hp2::recovered::bATan(m,m.scalar(point+0xc)*direction,m.scalar(point+0x10)*direction);
 if(!services.route_newpos_w)hp2::not_implemented("0x00132FB0","original route-reset local newpos W requires its own explicit port operand (port call-boundary preflight)");m.word(position+12,*services.route_newpos_w);ResetCore(position,speed,angle,services);hp2::debug::TrackPathCoordinate::UpdatePosition(m,address_+0x7e0,services.routes->paths,{m.scalar(address_+0x60),m.scalar(address_+0x64)});
 const auto controller=m.word(address_+0x28);auto table=m.word(controller+8);const auto route_self=controller+std::int32_t(m.signed_half(table+0x28));
 if(m.word(table+0x2c)==0x233ec0)hp2::debug::TrackedController(m,route_self).SetCurrentRoute(route,point);
 else if(m.word(table+0x2c)==0x18e650){if(!services.ai)hp2::not_implemented("0x00133074","actual AI reset context is required");hp2::debug::AICarController(m,route_self).SetCurrentRoute(route,point,*services.ai);}
 else hp2::not_implemented("0x00133074","original reset controller SetCurrentRoute virtual target is not reconstructed");
 table=m.word(controller+8);const auto reset_self=controller+std::int32_t(m.signed_half(table+0x50));
 if(m.word(table+0x54)==0x234040)hp2::debug::TrackedController(m,reset_self).JustReset();
 else if(m.word(table+0x54)==0x18e830){if(!services.ai)hp2::not_implemented("0x0013308C","actual AI reset context is required");hp2::debug::AICarController(m,reset_self).JustReset(*services.ai);}
 else if(m.word(table+0x54)==0x18ac38){if(!services.ai)hp2::not_implemented("0x0013308C","actual traffic reset context is required");hp2::debug::TrafficDriver(m,reset_self).JustReset(*services.ai);}
 else hp2::not_implemented("0x0013308C","original reset controller JustReset virtual target is not reconstructed");if(blinking)StartBlinking(2.f);
}
}
namespace hp2::recovered {
// Debug00132BB0,E-0099-W106. Original table row i+1 and fifth-gear clamp.
void Car::ComputeFakeRPM(){auto& m=m_;const auto a=address_;const int old=m.signed_word(a+0x3c8);int gear=old;float speed=m.scalar(0x32700c);if(speed<=0)speed=m.scalar(a+0x3c4);if(m.scalar(0x327000)<=speed)speed=m.scalar(0x327000)-m.scalar(0x324d1c);const float smoothed=hp2::debug::smooth(m.scalar(a+0x4a0),speed,m.scalar(0x327008));m.scalar(a+0x4a0,smoothed);float rpm=0;const float first=m.scalar(0x326fec);auto ratio=[&](float numerator,float denominator){if(denominator==0)hp2::not_implemented("0x00132BB0","original fake-RPM ratio zero divisor requires exact EE semantics");return numerator/denominator;};if(smoothed<first){gear=m.scalar(0x326fe8)<=smoothed?2:0;float magnitude=std::fabs(smoothed);if(first<magnitude)magnitude=first;rpm=m.scalar(0x2e7d38)+ratio(magnitude,first)*(m.scalar(0x2e7d3c)-m.scalar(0x2e7d38));}else{std::array<float,6> thresholds;for(unsigned i=0;i<6;++i)thresholds[i]=m.scalar(0x326fec+4*i);for(int i=1;i<6;++i)if(smoothed<thresholds[i]){gear=i+2;if(gear>6)gear=6;const auto row=0x2e7d38+8*(i+1);rpm=m.scalar(row)+ratio(smoothed-thresholds[i-1],thresholds[i]-thresholds[i-1])*(m.scalar(row+4)-m.scalar(row));break;}}const float test=m.scalar(0x32818c);if(old==gear)rpm=hp2::debug::smooth(test==100?m.scalar(a+0x3d0):test,rpm,m.scalar(0x327004));m.word(a+0x3c8,static_cast<std::uint32_t>(gear));m.scalar(a+0x3d0,test==100?rpm:test);}
}

namespace hp2::recovered {
// Debug00135178,E-0099-W121; exact car.cpp definition. SB preserves adjacent bytes.
void Car::FixBlownTires(){m_.word(address_+0x84c,0);m_.byte(address_+0x848,0);m_.byte(address_+0x849,0);}
}

namespace hp2::recovered {
// Debug00132F48,E-0110-F07. Required source virtual targets retain exact guards.
void Car::ResetToPosition(std::uint32_t position,float speed,std::uint16_t angle,const ::port::race::CarResetServices& services){
 ResetCore(position,speed,angle,services);if(!services.routes||!services.routes->initialized)hp2::not_implemented("0x002036E0","actual route paths required by ResetToPosition");auto& m=m_;hp2::debug::TrackPathCoordinate::UpdatePosition(m,address_+0x7e0,services.routes->paths,{m.scalar(address_+0x60),m.scalar(address_+0x64)});const auto controller=m.word(address_+0x28);auto table=m.word(controller+8);const auto find_self=controller+std::int32_t(m.signed_half(table+0x48));
 if(m.word(table+0x4c)==0x233f10)hp2::debug::TrackedController(m,find_self).FindWhereIAm(*services.routes);
 else if(m.word(table+0x4c)==0x190d48){if(!services.ai)hp2::not_implemented("0x00132F7C","actual AI reset context is required");hp2::debug::AICarController(m,find_self).FindWhereIAm(*services.ai);}
 else hp2::not_implemented("0x00132F7C","actual ResetToPosition FindWhereIAm virtual target is not reconstructed");
 table=m.word(controller+8);const auto reset_self=controller+std::int32_t(m.signed_half(table+0x50));
 if(m.word(table+0x54)==0x234040)hp2::debug::TrackedController(m,reset_self).JustReset();
 else if(m.word(table+0x54)==0x18e830){if(!services.ai)hp2::not_implemented("0x00132F94","actual AI reset context is required");hp2::debug::AICarController(m,reset_self).JustReset(*services.ai);}
 else if(m.word(table+0x54)==0x18ac38){if(!services.ai)hp2::not_implemented("0x00132F94","actual traffic reset context is required");hp2::debug::TrafficDriver(m,reset_self).JustReset(*services.ai);}
 else hp2::not_implemented("0x00132F94","actual ResetToPosition JustReset virtual target is not reconstructed");StopBlinking();
}
}

// TASK-0100 E-0100-AA01. Original car.cpp AverageAir00135B88, Debug
// SHA815b60c7...332f1b. Source listing, not decompiler int/max artifacts.
#include "game_core/port/driving/ee_scalar.hpp"
namespace hp2::recovered {
float Car::AverageAir(float& seconds,float& highest,const ::port::race::PhysicsConstructionServices* physics){
 using ::port::driving::ee_mul_s_finite;using ::port::driving::ee_add_s_finite;
 auto& m=m_;const auto car=address_,info=m.word(car+0xc);highest=0;
 if(!physics||!physics->average_air_position_w)hp2::not_implemented("0x00135C4C","original AverageAir initial position-accumulator W requires independent operand");
 if(!physics->average_air_acceleration_w)hp2::not_implemented("0x00135CF8","original AverageAir acceleration W requires independent operand");
 ::port::diagnostics::warn("0x00135C4C","original AverageAir accumulator W is unknown","using independent supplied PC research operand, no original stack parity claim");
 ::port::diagnostics::warn("0x00135CF8","original AverageAir acceleration W is unknown","using independent supplied PC research operand, no original stack parity claim");
 const float speed=m.scalar(car+0x3c4),mass=m.scalar(info+0x1e0);
 if(!std::isfinite(seconds)||!std::isfinite(speed)||!std::isfinite(mass)||mass==0)hp2::not_implemented("0x00135C10","original AverageAir scalar conversion/division operands require finite reviewed domain");
 float acceleration=m.scalar(info+0x2f4)+m.scalar(info+0x2f8);
 acceleration=acceleration*m.scalar(0x324d94);acceleration=speed*acceleration;
 acceleration=acceleration/mass;acceleration=acceleration+m.scalar(0x327ba0);
 if(!std::isfinite(acceleration))hp2::not_implemented("0x00135C14","original AverageAir acceleration producer requires EE finite carrier");
 ::port::driving::SolverMatrix body{},wheel{};for(unsigned i=0;i<12;++i)body[i]=m.scalar(car+0x90+4*i);
 for(unsigned i=0;i<3;++i)body[12+i]=m.scalar(car+0x60+4*i);body[15]=1;
 for(unsigned i=0;i<16;++i)wheel[i]=m.scalar(car+0xf0+4*i);
 ::port::driving::SolverMathContext context{m.vector(0x317b20)};
 ::port::driving::SolverVector position{{0,0,0,0}},velocity=m.vector(car+0x70),a{{0,0,-acceleration,0}};
 std::memcpy(&position[3],&*physics->average_air_position_w,4);std::memcpy(&a[3],&*physics->average_air_acceleration_w,4);
 for(unsigned i=0;i<4;++i){
  //254288 VADD.xyz takes homogeneous W from bUnitVector4, ignoring the input
  // local's unwritten W. The host input W below is never an instruction input.
  ::port::driving::SolverVector point{{0,0,-m.scalar(info+0x12c+0x20*i),0}};
  hp2::recovered::bMulMatrix(point,wheel,point,context);hp2::recovered::bMulMatrix(point,body,point,context);
  for(unsigned lane=0;lane<4;++lane){const float quarter=ee_mul_s_finite(point[lane],.25f,"0x00135CC4");position[lane]=ee_add_s_finite(quarter,position[lane],"0x00135CC8");}
 }
 hp2::debug::TopologyCoordinate topology(m,car+0x520);
 const float elevation=topology.GetElevation({{position[0],position[1],position[2]}});
 const float clearance=position[2]-elevation;
 if(!std::isfinite(clearance))hp2::not_implemented("0x00135D24","original AverageAir MAX.S exceptional clearance operand");
 float total=std::max(0.f,clearance);
 float count_operand=speed*seconds;count_operand=count_operand*m.scalar(0x324d98);
 if(!std::isfinite(count_operand)||count_operand<-2147483648.f||count_operand>=2147483648.f)hp2::not_implemented("0x00135D28","original AverageAir sample-count CVT.W.S exceptional operand");
 const int count=::port::driving::cvt_w_s(count_operand);int sample=1;
 while(sample<count){
  const float time=(seconds*float(sample))/float(count); //f23 remains zero in source.
  ::port::driving::SolverVector predicted{};
  for(unsigned lane=0;lane<4;++lane){const float impulse=ee_mul_s_finite(a[lane],time,"0x00135D98");const float v=ee_add_s_finite(impulse,velocity[lane],"0x00135D9C");const float displacement=ee_mul_s_finite(v,time,"0x00135DAC");predicted[lane]=ee_add_s_finite(displacement,position[lane],"0x00135DB0");}
  std::array<float,4> normal;const float ground=topology.GetElevation({{predicted[0],predicted[1],predicted[2]}},nullptr,&normal);
  const float height=predicted[2]-ground;
  if(!std::isfinite(height))hp2::not_implemented("0x00135DC8","original AverageAir exceptional topology clearance operand");
  if(highest<height)highest=height;
  if(!(0.f<height)){seconds=time;++sample;break;}
  ++sample;total=total+height;
 }
 if(sample!=1)total=total/float(sample)-1.f;
 return total;
}
}
