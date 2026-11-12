// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trafficteleporter.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrafficTeleporter::DoTimestep() [0x002BCDC0]
#include "game_core/TrafficTeleporter.hpp"

#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug0021CFF0, E-0099-W02. Actual route totals and original constructor-only minimum20.
TrafficTeleporter::TrafficTeleporter(::port::driving::Memory& memory,std::uint32_t address,
    const ::port::race::RoutePointCount& routes):memory_(memory),address_(address){
    if(!routes)hp2::not_implemented("0x0021CFF0","original loaded-route provider missing");
    FillInTotals(address_+8,50,routes);FillInTotals(address_+0x30,60,routes);SetThinkTime();
    if(memory_.scalar(address_)<20)memory_.scalar(address_,20);
    memory_.word(address_+4,0);
}
// Debug0021D778, E-0099-W02. Unsigned cumulative totals; missing routes preserve prior total.
void TrafficTeleporter::FillInTotals(std::uint32_t array,std::uint32_t start,const ::port::race::RoutePointCount& routes){
    if(!routes)hp2::not_implemented("0x0021D778","original loaded-route provider missing");
    std::uint32_t total=0;
    for(unsigned i=0;i<10;++i){const auto route=start+i;
        if(static_cast<std::int32_t>(route)>=0){const auto count=routes(route);if(count)total+=*count;}
        memory_.word(array+4*i,total);
    }
}
// Debug0021D700, E-0099-W02. Subsequent setter calls do not apply constructor's minimum.
void TrafficTeleporter::SetThinkTime(){memory_.scalar(address_,(1.f-memory_.scalar(0x311f78+0x10))*15.f+1.f);}
}

#include "game_core/indep/inc/World.hpp"
#include <cmath>
namespace hp2::debug {
// Debug0021D678, E-0099-W67. Original cached XY visibility threshold is strict.
bool TrafficTeleporter::CheckIsVisible(const std::array<float,2>& point){const auto world=memory_.word(0x3272f0);if(!world)hp2::not_implemented("0x0021D678","actual World is required by traffic visibility");return World(memory_,world).GetDistanceToClosestPlayer(point)<250;}
// Debug0021D6B8, E-0099-W67. Keep signed wrap selection before absolute compare.
bool TrafficTeleporter::CheckIsVisible(float distance){const auto world=memory_.word(0x3272f0);if(!world)hp2::not_implemented("0x0021D6B8","actual World is required by traffic path visibility");return std::abs(World(memory_,world).GetDATToClosestPlayer(distance))<250;}
// Debug0021D0D8, E-0099-W67. Exact source density/roster/stage/player gates,
// timer and round-robin visibility. A required teleport child cannot be skipped.
void TrafficTeleporter::DoTimestep(float timestep,const ::port::race::CarResetServices* reset){auto& m=memory_;if(!(m.scalar(0x325f9c)<=m.scalar(0x311f88)))return;const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0021D0D8","original traffic scheduling requires World owner");const int total=m.signed_word(world+0x1c),start=m.signed_word(world+0x28);if(total==start)return;const auto race=m.word(0x327258);if(!race)hp2::not_implemented("0x0021D0D8","original traffic scheduling requires Race owner");if(m.word(race)!=4||!m.word(0x327780))return;const float remaining=m.scalar(address_)-timestep;m.scalar(address_,remaining);if(!(remaining<=0))return;SetThinkTime();const auto last=m.word(address_+4);if(start<0||total<start||total>14||last>=std::uint32_t(total-start))hp2::not_implemented("0x0021D0D8","original round-robin/table state outside valid traffic domain");auto index=last;do{++index;if(std::uint32_t(total-start)<=index)index=0;const auto car=m.word(world+0x2c+4*(std::uint32_t(start)+index));if(!CheckIsVisible(m.scalar(car+0x7f4))){if(!reset)hp2::not_implemented("0x0021D380","actual traffic reset services are required");TryToTeleportCar(car,*reset);}}while(index!=last);m.word(address_+4,index);}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug0021D078, E-0099-W76. Source records only timer and round-robin index.
void TrafficTeleporter::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(memory_,snapshot);s.BeginSection(0x2b4908);s.MoveData(address_,4);s.MoveData(address_+4,4,4,0);s.EndSection();}
}

#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/inc/CarInfo.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/port/race/car_construction.hpp"
#include "game_core/port/race/ai_drive_target.hpp"
#include "game_core/port/race/control_construction.hpp"
#include "game_core/src/bMath.hpp"
namespace hp2::debug {
// Debug0021D730, E-0099-W91. Source consumes the shared World random stream.
bool TrafficTeleporter::ForwardsOrBackwards(){const auto world=memory_.word(0x3272f0);if(!world)hp2::not_implemented("0x0021D730","actual World owner absent");return World(memory_,world).Random(1.f)<memory_.scalar(0x311f8c);}
// Debug0021D9C0, E-0099-W91. Cumulative totals select a point, not a uniform route.
void TrafficTeleporter::GenerateRandomPoint(NearestRoutePointInfo& info,bool forwards){auto& m=memory_;const auto totals=address_+(forwards?8:0x30);const auto world=m.word(0x3272f0);const auto count=m.word(totals+36);if(!world||!count||count>0x7fffffff)hp2::not_implemented("0x0021D9C0","original nonempty route-total/World domain required");auto draw=World(m,world).Random(int(count));unsigned bucket=0;while(bucket<10&&m.word(totals+4*bucket)<=draw)++bucket;if(bucket==10)hp2::not_implemented("0x0021D9C0","invalid original cumulative totals");if(bucket)draw-=m.word(totals+4*(bucket-1));info.Route=m.word(0x30bb40+4*((forwards?50:60)+bucket));if(!info.Route)hp2::not_implemented("0x0021D9C0","selected original route absent");info.Point=info.Route+0x428+0x70*draw;}
// Debug0021DAE0, E-0099-W91. Zero available skins consumes no integer draw.
int TrafficTeleporter::PickRandomSkin(std::uint32_t type){const auto count=CarTypeInfo::GetNumAvailableSkins(memory_,type);if(!count)return 0;const auto world=memory_.word(0x3272f0);if(!world)hp2::not_implemented("0x0021DAE0","actual World owner absent");return CarTypeInfo::GetAvailableSkin(memory_,type,int(World(memory_,world).Random(count)));}
// Debug0021D8B0, E-0099-W91. Source scans the intrusive list before table rebuild.
bool TrafficTeleporter::TooCloseToAnotherCar(float distance,std::uint32_t except,bool invisible){auto& m=memory_;const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0021D8B0","actual World owner absent");for(auto car=m.word(world);car!=world;car=m.word(car)){if(car==except||hp2::recovered::Car(m,car).IsPoofed())continue;if(!invisible&&!m.byte(car+0x4a8))continue;const float delta=TrackPathManager::CompareDistances(m.scalar(0x313308),distance,m.scalar(car+0x7f4));if(m.signed_word(m.word(car+0x14)+8)==3){if(std::abs(delta)<100)return true;}else if(delta<100&&0<delta)return true;}return false;}
// Debug0021D530, E-0099-W91. Preserve strict timer/start-line/visibility limits.
bool TrafficTeleporter::CanPlaceCarHere(const NearestRoutePointInfo& info,std::uint32_t except,bool invisible){auto& m=memory_;const auto race=m.word(0x327258);const float dat=m.scalar(info.Point+0x28);if(!race)hp2::not_implemented("0x0021D530","source null-Race branch dereferences Race boundary state");const auto elapsed=static_cast<std::int32_t>(m.word(0x327cf8)-m.word(race+0xc));if(float(elapsed)*m.scalar(0x325fa4)<20){const auto boundary=m.signed_byte(race+0x652);const float start=m.scalar(0x313308)*m.scalar(0x325fa8)*m.scalar(race+0x654+4*boundary);if(std::abs(TrackPathManager::CompareDistances(m.scalar(0x313308),dat,start))<2000)return false;}if(CheckIsVisible(dat))return false;return !TooCloseToAnotherCar(dat,except,invisible);}
// Debug0021D1D8, E-0099-W91. Exact factory/placement/control sequence; no synthetic Car.
std::uint32_t TrafficTeleporter::PlaceNewCar(std::uint32_t driver,const ::port::race::CarConstructionServices& construction,::port::race::PhysicsConstructionServices& physics,const ::port::race::AiDriveTargetServices& ai){auto& m=memory_;::port::driving::Memory::Temporary tpc(m,0x40);TrackPathCoordinate::Sub_002035A0(m,tpc.address());const bool forwards=ForwardsOrBackwards();if(m.word(address_+0x2c)&&m.word(address_+0x54)){for(unsigned attempt=0;attempt<100;++attempt){NearestRoutePointInfo info;GenerateRandomPoint(info,forwards);if(!CanPlaceCarHere(info,0,true))continue;if(!construction.routes)hp2::not_implemented("0x00136DB0","actual loaded track paths required by ResetPosition");m.word(tpc.address()+8,0);TrackPathCoordinate::UpdatePosition(m,tpc.address(),construction.routes->paths,{m.scalar(info.Point),m.scalar(info.Point+4)});const auto angle=hp2::recovered::bATan(m,m.scalar(info.Point+0xc),m.scalar(info.Point+0x10));::port::driving::Memory::Temporary pos(m,16);m.scalar(pos.address(),m.scalar(info.Point));m.scalar(pos.address()+4,m.scalar(info.Point+4));m.scalar(pos.address()+8,m.scalar(info.Point+8));const int skin=PickRandomSkin(m.word(0x3278e0)+0x560*m.word(driver+0xc));m.byte(driver+0x11,static_cast<std::uint8_t>(skin));if(!construction.port_stack_vector_lane)hp2::not_implemented("0x0021D1D8","original pos3d fourth stack lane requires explicit PC operand policy");m.word(pos.address()+12,*construction.port_stack_vector_lane);const auto car=m.allocate(0x890);hp2::recovered::Car object(m,car);object.Sub_00130B90(driver,pos.address(),static_cast<std::uint16_t>(angle),construction);object.SetMovementMode(0,physics);object.SetControlMode(3,*construction.routes);const auto control=m.word(car+0x28),table=m.word(control+8);if(m.word(table+0x2c)!=0x18e650)hp2::not_implemented("0x0021D1D8","unrecovered SetCurrentRoute virtual target");const auto adjusted=control+static_cast<std::uint32_t>(static_cast<std::int32_t>(m.signed_half(table+0x28)));AICarController(m,adjusted).SetCurrentRoute(info.Route,info.Point,ai);m.byte(car+0x4a8,0);TrackPathCoordinate::Sub_00203630(m,tpc.address(),2);return car;}}TrackPathCoordinate::Sub_00203630(m,tpc.address(),2);return 0;}
}

#include "game_core/port/race/car_reset_services.hpp"
namespace hp2::debug {
// Debug0021DAA8, E-0110-T01. A single player still consumes the original RNG draw.
std::uint32_t TrafficTeleporter::RandomPlayerCar(){auto& m=memory_;const auto world=m.word(0x3272f0);const auto draw=World(m,world).Random(m.signed_word(0x327780));return m.word(m.word(0x327778+4*draw)+4);}
// Debug0021DB30. Source comparison is against a signed RideInfo skin byte.
void TrafficTeleporter::SwitchSkin(std::uint32_t car){auto& m=memory_;const int skin=PickRandomSkin(m.word(car+0xc));if(skin!=m.signed_byte(m.word(car+0x10)+5))hp2::recovered::Car(m,car).SwitchSkin(static_cast<unsigned>(skin));}
// Debug0021D380. Original unbounded route walk, visibility and signed forward
// comparison; no roster, RNG, placement or source reset operation is bypassed.
void TrafficTeleporter::TryToTeleportCar(std::uint32_t car,const ::port::race::CarResetServices& reset){
 auto& m=memory_;if(!reset.routes||!reset.physics)hp2::not_implemented("0x0021D380","actual traffic routes/physics required");const bool forwards=ForwardsOrBackwards();const auto player=RandomPlayerCar();const float old_delta=TrackPathManager::CompareDistances(m.scalar(0x313308),m.scalar(car+0x7f4),m.scalar(player+0x7f4));NearestRoutePointInfo info;
 if(!GetNearestRoutePoint(m,*reset.routes,info,{{m.scalar(player+0x60),m.scalar(player+0x64)}},forwards?FORWARD_TRAFFIC_ROUTES:ONCOMING_TRAFFIC_ROUTES,0,0))hp2::not_implemented("0x0021D400","original unchecked nearest-route output stack words are unresolved when no route is selected");
 unsigned count=GuessTargetDestinations(m,&info,1,info.Route,info.Point,250,forwards);float new_delta=0;
 while(count==1){new_delta=TrackPathManager::CompareDistances(m.scalar(0x313308),m.scalar(info.Point+0x28),m.scalar(player+0x7f4));if(CanPlaceCarHere(info,car,false))break;count=GuessTargetDestinations(m,&info,1,info.Route,info.Point,50,forwards);}
 if(count!=1||(old_delta>0&&old_delta<new_delta)||new_delta<0)return;
 hp2::recovered::Car object(m,car);object.ResetToRoutePoint(info.Route,info.Point,m.scalar(0x325fa0),false,1,reset);object.SetMovementMode(2,*reset.physics);SwitchSkin(car);
}
}
