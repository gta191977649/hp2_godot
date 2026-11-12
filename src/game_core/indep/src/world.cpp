#include "game_core/port/race/car_reset_services.hpp"
// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\world.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): World::DoTimestep() [0x002AFD18]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFD30]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFD58]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFD78]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFDA0]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFDC8]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFDE8]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFE10]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFE30]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFE58]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFE80]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFEB0]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFED8]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFF00]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFF28]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFF50]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFF78]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFFA0]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFFC8]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFFF0]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002B0020]
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/inc/Smokeable.hpp"
#include "game_core/Chopper.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/src/bList.hpp"
#include "game_core/indep/src/trackgraph.hpp"
#include "game_core/port/race/munitions.hpp"
#include "game_core/port/race/smokeables.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/race/world_construction.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/timer.hpp"
#include "game_core/src/bMath.hpp"

namespace hp2::recovered {
// ELF 0x0014D338; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014d338.md.
void World::BuildCarTable() {
    hp2::not_implemented("0x0014D338", "World::BuildCarTable (signature and full body unresolved)");
}

// ELF 0x0014D508; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014d508.md.
void World::GetCarByNumber() {
    hp2::not_implemented("0x0014D508", "World::GetCarByNumber (signature and full body unresolved)");
}

// ELF 0x0014D638; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014d638.md.
void World::ResetTimeScale() {
    hp2::not_implemented("0x0014D638", "World::ResetTimeScale (signature and full body unresolved)");
}

// ELF 0x0014E3E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014e3e8.md.
void World::Sub_0014E3E8() {
    hp2::not_implemented("0x0014E3E8", "World::Sub_0014E3E8 (signature and full body unresolved)");
}

// ELF 0x0014E440; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014e440.md.
void World::Sub_0014E440() {
    hp2::not_implemented("0x0014E440", "World::Sub_0014E440 (signature and full body unresolved)");
}
} // namespace hp2::recovered
// TASK-0048 E-0048-T06. Debug0014D630, single active dynamic car against immutable track bodies.
#include "game_core/port/driving/static_collisions.hpp"
namespace hp2::recovered {
void World::DoTimestepCollisions(port::driving::Memory& m,port::driving::StaticCollisions& collisions,std::uint32_t body,std::uint32_t car,const port::track::Topology& topology,port::driving::CollisionMotion& motion,const std::function<void(float)>& move,const std::function<void()>& recompute){
    for(unsigned iteration=0;iteration<20;++iteration){const auto contacts=collisions.contacts(m,body,topology,motion);if(contacts.empty())return;collisions.resolve(m,body,car,contacts.front(),motion,move,recompute);}
}
}

namespace port::race {
// Port-only malformed-list guard. Floyd traversal requires no per-call allocations.
void validate_world_ring(::port::driving::Memory& memory,std::uint32_t head){
    const auto next=[&](std::uint32_t pointer){
        if(!pointer)hp2::not_implemented("0x0014D028","original World list contains null link");
        return memory.word(pointer);
    };
    auto slow=head,fast=head;
    do{
        slow=next(slow);fast=next(fast);if(fast!=head)fast=next(fast);
        if(fast==head)return;
        if(slow==fast)hp2::not_implemented("0x0014D028","original World list cycle does not reach sentinel");
    }while(true);
}
}
namespace hp2::debug {
// Debug0014CA28, E-0099-W06. Debug NFSHP2.ELF SHA256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// The original publishes pCurrentWorld before resources are initialized. Host
// sessions must keep construction private until every required callee succeeds.
void World::Sub_0014CA28(const ::port::race::WorldConstructionServices& services){
    auto& memory=memory_;const auto world=address_;
    for(unsigned offset:{0u,8u,16u}){
        memory.word(world+offset,world+offset);memory.word(world+offset+4,world+offset);
    }
    // Original trivial array constructors leave tables/positions unchanged.
    for(unsigned i=0;i<20;++i){OBB object(memory,world+0x1a0+i*0x50);}
    for(unsigned i=0;i<20;++i){OBB object(memory,world+0x7e0+i*0x50);}
    DriveTarget(memory,world+0x10f0).Sub_00216528();
    DriveTarget(memory,world+0x1300).Sub_00216528();
    DriveTarget(memory,world+0x1510).Sub_00216528();
    memory.word(0x003272f0,world);
    hp2::recovered::ResetWorldTime(memory);
    const auto seed=hp2::recovered::bRandom(memory,0x7fffffff);
    for(unsigned offset:{0x1cu,0x20u,0x24u,0x28u})memory.word(world+offset,0);
    memory.word(world+0x178,seed);memory.word(world+0x19c,1);memory.word(world+0x198,1);
    ResetTimeScale();memory.word(world+0x180,0);
    for(unsigned i=0;i<0x20;++i)memory.byte(world+0x150+i,0);
    for(unsigned i=0;i<8;++i)memory.byte(world+0x170+i,0);
    memory.word(world+0x18,0);
    const auto teleporter=memory.allocate(0x58);
    if(!services.routes)hp2::not_implemented("0x0021CFF0","actual loaded World route data is not bound");
    TrafficTeleporter traffic(memory,teleporter,[&](unsigned number)->std::optional<std::uint32_t>{
        const auto route=services.routes->routes.at(number);
        if(!route)return std::nullopt;return memory.word(route+0x10);
    });
    memory.word(world+0x194,teleporter);
    InitCarEffects(memory);
    InitClans(memory);
    InitSkids(memory,services.texture_info);
    InitMunitions(memory,services.emitter_group);
    InitFernFlatteners();
    InitFernShakers(memory,services.emitter_group);
    if(!services.smokeables)hp2::not_implemented("0x0021A4E8","actual loaded smokeable/scenery source data is not bound");
    SpawnSmokeables(memory,*services.smokeables,services.emitter_group);
    InitTerrainEffectEmitters(memory,services.emitter_group);
    InitCops(memory,services.cops);
    InitTrackGraph(memory,*services.routes);
}
// Debug0014D028, E-0099-W01. Original stable four traversals: player, AI racer,
// COP/SUPER_COP, traffic. Car.MyDriver+8 supplies type; unknown types are omitted.
void World::BuildCarTable(){
    auto& memory=memory_;const auto world=address_;
    ::port::race::validate_world_ring(memory,world);::port::race::validate_world_ring(memory,world+8);
    memory.word(world+0x1c,0);memory.word(world+0x20,0);memory.word(world+0x24,0);memory.word(world+0x28,0);
    unsigned count=0;
    const auto append_group=[&](int type,int alternative){
        for(auto car=memory.word(world);car!=world;car=memory.word(car)){
            const auto driver=memory.word(car+0x14);
            if(!driver)hp2::not_implemented("0x0014D028","original Car.MyDriver is not bound");
            const int actual=memory.signed_word(driver+8);
            if(actual!=type && actual!=alternative)continue;
            if(count>=14)hp2::not_implemented("0x0014D028","original World.CarTable exceeds fourteen entries");
            memory.word(world+0x2c+4*count,car);++count;memory.word(world+0x1c,count);
        }
    };
    append_group(1,1);memory.word(world+0x20,count);
    append_group(2,2);memory.word(world+0x24,count);
    append_group(4,5);memory.word(world+0x28,count);
    append_group(3,3);
    for(unsigned index=0;index<count;++index)memory.half(memory.word(world+0x2c+4*index)+8,static_cast<std::uint16_t>(index));
    // Original final AICarList traversal has no body; ring validity was checked above.
}
// Debug0014D1F8, E-0099-W01. Search signed Car.DriverNumber, not CarIndex or a table ordinal.
std::uint32_t World::GetCarByNumber(int number) const{
    const int count=memory_.signed_word(address_+0x1c);
    if(count<0 || count>14)hp2::not_implemented("0x0014D1F8","original World.CarTable count outside host bounds");
    for(int i=0;i<count;++i){const auto car=memory_.word(address_+0x2c+4*i);
        if(memory_.signed_half(car+0xa)==number)return car;}
    return 0;
}
// Debug0014D328, E-0099-W01. Preserve previous waste and all unrelated World fields.
void World::ResetTimeScale(){
    memory_.word(address_+0x190,0);memory_.scalar(address_+0x184,1);
    memory_.scalar(address_+0x188,0);memory_.scalar(address_+0x17c,0);memory_.word(address_+0x18c,0);
}
}

namespace hp2::debug {
// Debug0014CC80, E-0099-W18. Original ordered destructor; its empty member
// destructors were inspected, never inferred from zero-initialized port memory.
void World::Sub_0014CC80(int in_chrg){
 auto& m=memory_;const auto world=address_;
 CloseTerrainEffectEmitters(m);CleanupCops(m);CleanupTrackGraph(m);
 EliminateAllWitnesses(m);
 ClearSmokeables(m);CloseFernShakers(m);CloseFernFlatteners(m);
 CloseMunitions(m);CloseClans(m);CloseSkids(m);CloseCarEffects(m);
 // TrafficTeleporter0021D058 and DriveTarget00216598 only free if in_chrg&1.
 if(const auto teleporter=m.word(world+0x194))m.release(teleporter);
 hp2::recovered::ResetWorldTime(m);m.word(0x3272f0,0);m.word(0x327d10,100000);
 // DriveTarget members receive2: no stores. OBB00218860 calls WorldObject
 // destruction iff WorldObj is nonnull; construction sets all40 to null.
 for(auto base:{0x7e0u,0x1a0u})for(int i=19;i>=0;--i)
  if(m.word(world+base+i*0x50+0x4c))hp2::not_implemented("0x00218860","active OBB WorldObject destruction is not reconstructed");
 if(m.word(world+16)!=world+16)hp2::not_implemented("0x0014E628","active World CollisionList destruction is not reconstructed");
 // E-0099-W35 extension: original bPList<Car>0014E768 jumps0014E840.
 // It deletes pointer nodes only, before the owning bTList<Car>0014E698.
 ::port::race::validate_world_ring(m,world+8);
 while(m.word(world+8)!=world+8){
  const auto node=m.word(world+8),next=m.word(node),prev=m.word(node+4);
  m.word(prev,next);m.word(next+4,prev);bPNode::Free(m,node);
 }
 //0014E6D4 directly calls Car00130FF8(flags3), not a virtual destructor.
 // Constructor-only Car subclasses share this base destruction; active audio,
 // collision/controller and other unknown callees retain their existing guards.
 ::port::race::validate_world_ring(m,world);
 while(m.word(world)!=world){
  const auto car=m.word(world),next=m.word(car),prev=m.word(car+4);
  m.word(prev,next);m.word(next+4,prev);hp2::recovered::Car(m,car).Sub_00130FF8(3);
 }
 if(in_chrg&1)m.release(world);
}
}

namespace hp2::debug {
// Debug0014E230, E-0099-W37. Independent World seed, original float RNG.
float World::Random(float range){return hp2::recovered::bRandom(memory_,address_+0x178,range);}
}

namespace hp2::debug {
// Debug0014E250, E-0099-W37. Original unsigned-wrap prediction sequence.
void World::IncrementCollisionPredictionID(){memory_.word(address_+0x198,memory_.word(address_+0x198)+1);}
}

#include <cmath>
namespace hp2::debug {
// Debug0014E0D0, E-0099-W44. MIN.S returns a float, not the decompiler's
// apparent integer conversion/bitwise expression; fresh raw MIPS retained.
float World::GetDistanceToClosestPlayer(const std::array<float,2>& point)const{
 float distances[2];for(unsigned i=0;i<2;++i){const auto a=address_+0x150+16*i;const float x=point[0]-memory_.scalar(a),y=point[1]-memory_.scalar(a+4);const float square=x*x+y*y;if(!std::isfinite(square))hp2::not_implemented("0x0014E0D0","exceptional EE sqrt/min operand is not mapped");distances[i]=std::sqrt(square);}return distances[0]<distances[1]?distances[0]:distances[1];
}
// Debug0014E128, E-0099-W44. VU sums XYZ in order; the loaded fourth lane is not consumed.
float World::GetDistanceToClosestPlayer(const std::array<float,3>& point)const{
 float distances[2];for(unsigned i=0;i<2;++i){const auto a=address_+0x150+16*i;const float x=point[0]-memory_.scalar(a),y=point[1]-memory_.scalar(a+4),z=point[2]-memory_.scalar(a+8);const float square=(x*x+y*y)+z*z;if(!std::isfinite(square))hp2::not_implemented("0x0014E128","exceptional EE sqrt/min operand is not mapped");distances[i]=std::sqrt(square);}return distances[0]<distances[1]?distances[0]:distances[1];
}
}

#include "game_core/port/race/physics_construction.hpp"
namespace hp2::debug {
// Debug0014D588, E-0099-W38. Original snapshots the dynamic CollisionBodyList
// BEFORE any Move can reorder it; this is not a CarTable traversal. Original
// stack array holds174 pointers; exceeding it is an explicit port boundary.
void World::DoTimestepMove(float timestep,::port::race::PhysicsConstructionServices& services){
 auto& m=memory_;std::array<std::uint32_t,174> snapshot{};std::size_t count=0;
 for(auto body=m.word(0x3283d8);body!=0x3283d8;body=m.word(body))if(m.byte(body+0xd)){
  if(count==snapshot.size())hp2::not_implemented("0x0014D5C8","Original dynamic collision snapshot exceeds174-entry stack storage");snapshot[count++]=body;
 }
 for(std::size_t i=0;i<count;++i){const auto body=snapshot[i],table=m.word(body+0x50),target=m.word(table+0x44),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x40));
  switch(target){case 0x1d4e28:CarCollisionBody(m,adjusted).Move(timestep,services);break;
   case 0x21b688:Smokeable(m,adjusted).Move(timestep,services);break; //E-0099-W117 actual dynamic list target.
   default:hp2::not_implemented("0x0014D588","Unrecovered dynamic CollisionBody.Move virtual target");}
 }
}
}

#include "game_core/indep/inc/Contact.hpp"
#include "game_core/indep/src/collision.hpp"
namespace hp2::debug {
// Debug0014D630, E-0099-W38. Actual shared witness/contact/persistent Collision
// scheduler. Original20-resolution cap and duplicate witness rechecks retained;
// live Smokeable.Break/other body-family gaps are explicit actual callees.
void World::DoTimestepCollisions(float timestep,::port::race::PhysicsConstructionServices& service){
 auto& m=memory_;
 // MaybePrintAllCollisionBodies001D3AA0 is confirmed empty original code.
 for(auto w=m.word(0x328400);w!=0x328400;){const auto next=m.word(w);if(CollisionBodyWitness(m,w).HasExpired())CollisionBodyWitness(m,w).Sub_001D2BF0(3);w=next;}
 for(auto body=m.word(0x3283d8);body!=0x3283d8;body=m.word(body))if(m.byte(body+0xd)&&m.byte(body+0x13))DynamicCollisionBody(m,body).MaybeCreateWitnesses(&service);
 ::port::driving::Memory::Temporary list(m,8);const auto contacts=list.address();m.word(contacts,contacts);m.word(contacts+4,contacts);
 for(auto w=m.word(0x328400);w!=0x328400;w=m.word(w))CollisionBodyWitness(m,w).TestForContact(contacts,service.port_stack_vector_lane);
 if(!service.ticker)hp2::not_implemented("0x0024A340","Original collision scheduler Count-register read service is absent");(void)service.ticker();
 unsigned resolutions=0;
 while(m.word(contacts)!=contacts&&resolutions<20){const auto contact=m.word(contacts);Contact(m,contact).Resolve(service);
  // Read both body pointers before RemoveContacts can free this Contact.
  const auto first=m.word(contact+0xc),second=m.word(contact+0x10);if(m.byte(first+0xd))Contact::RemoveContacts(m,contacts,first);if(m.byte(second+0xd))Contact::RemoveContacts(m,contacts,second);
  std::array<std::uint32_t,8> recheck{};std::size_t count=0;
  for(auto body:{first,second}){
   if(m.byte(body+0xc)==SMOKEABLE_COLLISION_BODY&&m.scalar(body+0x118)==-1)hp2::not_implemented("0x0021C050","Original Smokeable.Break and returned dynamic fragment bodies are required");
   if(m.byte(body+0xd)&&m.byte(body+0x13)){if(count==recheck.size())hp2::not_implemented("0x0014D630","Original recheck stack exceeds8-body storage");recheck[count++]=body;}
  }
  for(std::size_t i=0;i<count;++i)DynamicCollisionBody(m,recheck[i]).MaybeCreateWitnesses(&service);++resolutions;
  for(auto w=m.word(0x328400);w!=0x328400;w=m.word(w))for(std::size_t i=0;i<count;++i)if(m.word(w+0x60)==recheck[i]||m.word(w+0x64)==recheck[i])CollisionBodyWitness(m,w).TestForContact(contacts,service.port_stack_vector_lane);
 }
 // Source20-cap flush and final local bTList<Contact> destructor free remaining
 // nodes. Source MIPS14D904 preserves the old node through unlink before delete.
 while(m.word(contacts)!=contacts){const auto p=m.word(contacts),next=m.word(p),previous=m.word(p+4);m.word(previous,next);m.word(next+4,previous);Contact(m,p).Sub_001D26C8(3);}
 const auto collision_list=address_+0x10;for(auto collision=m.word(collision_list);collision!=collision_list;){const auto next=m.word(collision);if(!Collision(m,collision).Update(timestep))Collision(m,collision).End();collision=next;}
}
}

namespace hp2::debug {
// TASK-0099 E-0099-W48. Original unsigned wrap and full position publication.
std::uint16_t World::CalculateDebugChecksum(){auto& m=memory_;auto checksum=m.word(0x327cf8);for(int i=0;i<m.signed_word(address_+0x1c);++i){if(i>=14)hp2::not_implemented("0x0014DF10","original World checksum table exceeds14cars");const auto car=m.word(address_+0x2c+std::uint32_t(i)*4);const auto value=hp2::recovered::Car(m,car).CalculateDebugChecksum();m.copy(address_+0x70+std::uint32_t(i)*16,car+0x60,16);checksum=checksum*33+value;}const auto chopper=m.word(address_+0x18);if(chopper&&chopper!=0xfffffcb0)checksum+=std::uint32_t(::port::driving::cvt_w_s(m.scalar(chopper+0x380)));return std::uint16_t(checksum+(checksum>>16));}
}

namespace hp2::debug {
// Debug0014E260, E-0099-W38. Fresh original listing is JR RA / NOP only.
// Actual cleanup occurs through collision-owning bodies/witnesses, not this hook.
void World::FlushCollisions(){}
}

#include "game_core/port/race/joylog.hpp"
#include "game_core/ReplayManager.hpp"
namespace hp2::debug {
// Debug0014DFD0, E-0099-W49. Actual checksum channels; no silent dropped capture.
void World::VerifyDebugChecksums(const ::port::race::MonitorSyscall& monitor){auto& m=memory_;const auto checksum=CalculateDebugChecksum();if(!m.word(0x3278ac))Joylog::AddData(m,checksum,16,3,monitor);else (void)Joylog::GetData(m,16,3,monitor);const auto data=(std::uint32_t(m.byte(address_+0x178))+(checksum>>8)+checksum)&255;const auto replay=m.word(0x3271a8);if(!replay)hp2::not_implemented("0x0014DFD0","actual ReplayManager is required by World checksum scheduling");if(m.word(replay+4))hp2::recovered::ReplayManager(m,replay).AddData(std::int32_t(data),8,2);else if(m.word(replay)&&(hp2::recovered::ReplayManager(m,replay).GetData(8,2)&255)!=data)hp2::not_implemented("0x00108210","replay checksum failure requires original render wait/red screen and RC_SKIP_REPLAY message publication");}
}

namespace hp2::debug {
// Debug0014E208, E-0099-W47. Actual World seed, original integer RNG overload.
std::uint32_t World::Random(int range){return hp2::recovered::bRandom(memory_,address_+0x178,std::uint32_t(range));}
}

#include "game_core/indep/src/trackpath.hpp"
namespace hp2::debug {
// Debug0014E188, E-0099-W67. Both cached source DATs participate; equal
// absolute values select the second, preserving its original signed result.
float World::GetDATToClosestPlayer(float distance)const{const auto length=memory_.scalar(0x313308);const float first=TrackPathManager::CompareDistances(length,memory_.scalar(address_+0x170),distance),second=TrackPathManager::CompareDistances(length,memory_.scalar(address_+0x174),distance);return std::abs(first)<std::abs(second)?first:second;}
// Debug0014D2E8, E-0099-W67. Original <= on actual Position.X, no integer cast.
int CarListCheckFlip(::port::driving::Memory& m,std::uint32_t before,std::uint32_t after){return m.scalar(before+0x60)<=m.scalar(after+0x60);}
// Debug0014E5B0, E-0099-W67. Original intrusive adjacent swap/repeat loop.
// Only the actual World comparator is bound; no arbitrary successful callback.
void Sort(::port::driving::Memory& m,std::uint32_t head,std::uint32_t comparator){if(comparator!=0x14d2e8)hp2::not_implemented("0x0014E5B0","unrecovered original Car-list comparator target");bool changed;do{changed=false;auto before=m.word(head),after=m.word(before);while(before!=head&&after!=head){if(!std::isfinite(m.scalar(before+0x60))||!std::isfinite(m.scalar(after+0x60)))hp2::not_implemented("0x0014D2E8","exceptional EE position comparison/sort termination is unresolved");if(!CarListCheckFlip(m,before,after)){const auto next=m.word(after),previous=m.word(after+4);m.word(previous,next);m.word(next+4,previous);m.word(after,0xb);m.word(after+4,0xb);const auto prior=m.word(before+4);m.word(prior,after);m.word(before+4,after);m.word(after+4,prior);m.word(after,before);after=m.word(before);changed=true;}else{before=after;after=m.word(after);}}}while(changed);}
}

#include "game_core/port/race/world_frame.hpp"
#include "game_core/TrafficTeleporter.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/inc/Player.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/indep/inc/AcidFX.hpp"
#include "game_core/indep/src/fernflattener.hpp"
#include "game_core/indep/src/bodydamage.hpp"
#include "game_core/indep/src/trackrouteeditor.hpp"
#include "game_core/port/race/events.hpp"
namespace hp2::debug {
// Debug0014D9A8, E-0099-W67. Source order, fresh MIPS14D9A8/14DDD0.
// WorldTime preparation/advance belongs to the caller, not to this function.
void World::DoTimestep(float timestep,::port::race::WorldFrameServices& service){
 auto& m=memory_;VerifyDebugChecksums(service.monitor);m.scalar(0x327d78,0);
 for(unsigned i=0;i<2;++i){const auto player=m.word(0x327770+4*i),position=address_+0x150+16*i,dat=address_+0x170+4*i;if(player){const auto car=m.word(player+4);m.vector(position,m.vector(car+0x60));m.scalar(dat,m.scalar(car+0x7f4));}else{const float absent=m.scalar(0x324f58);for(unsigned j=0;j<3;++j)m.scalar(position+4*j,absent);m.scalar(dat,absent);}}
 ::port::race::CarResetServices reset;reset.routes=&service.routes;reset.physics=&service.physics;reset.ai=&service.ai;reset.cop_teleport_position_w=service.cop_teleport_position_w;reset.route_newpos_w=service.car_reset_route_newpos_w;reset.reset_velocity_w=service.car_reset_velocity_w;
 // Port lifetime binding for source-requested rolling resets, including
 // contact's nested Car.Move calls. Restore the caller service on every exit.
 struct ResetContextScope {
  ::port::race::PhysicsConstructionServices& physics;
  const ::port::race::CarResetServices* previous;
  ~ResetContextScope(){physics.car_reset=previous;}
 } reset_context{service.physics,service.physics.car_reset};
 service.physics.car_reset=&reset;
 if(const auto teleporter=m.word(address_+0x194))TrafficTeleporter(m,teleporter).DoTimestep(timestep,&reset);
 GlobalCopThink(m,timestep,&service.physics,&reset);
 // Bounds are re-read after each original callee, as in the source loops.
 for(int i=0;i<m.signed_word(address_+0x24)-m.signed_word(address_+0x20);++i)hp2::recovered::Car(m,m.word(address_+0x2c+4*(m.signed_word(address_+0x20)+i))).Think(timestep,service.ai,service.physics);
 for(int i=0;i<m.signed_word(address_+0x1c)-m.signed_word(address_+0x28);++i)hp2::recovered::Car(m,m.word(address_+0x2c+4*(m.signed_word(address_+0x28)+i))).Think(timestep,service.ai,service.physics);
 for(int i=0;i<m.signed_word(address_+0x20);++i)hp2::recovered::Car(m,m.word(address_+0x2c+4*i)).Think(timestep,service.ai,service.physics);
 for(int i=0;i<m.signed_word(address_+0x28)-m.signed_word(address_+0x24);++i)hp2::recovered::Car(m,m.word(address_+0x2c+4*(m.signed_word(address_+0x24)+i))).Think(timestep,service.ai,service.physics);
 for(int i=0;i<m.signed_word(address_+0x20);++i)hp2::recovered::Car(m,m.word(address_+0x2c+4*i)).DoWheelForces();
 for(int i=0;i<m.signed_word(address_+0x1c);++i)hp2::recovered::Car(m,m.word(address_+0x2c+4*i)).Drive(timestep,service.player);
 if(const auto chopper=m.word(address_+0x18))Chopper(m,chopper).Fly(timestep);
 SmokeableThink(m);
 for(int i=0;i<m.signed_word(address_+0x1c);++i)hp2::recovered::Car(m,m.word(address_+0x2c+4*i)).BeginMovement(timestep);
 DoTimestepMove(timestep,service.physics);DoTimestepCollisions(timestep,service.physics);
 for(int i=0;i<m.signed_word(address_+0x1c);++i)hp2::recovered::Car(m,m.word(address_+0x2c+4*i)).FinishMovement(timestep,service.routes,&service.physics);
 m.scalar(0x327d78,0);
 for(int i=0;i<m.signed_word(address_+0x1c);++i){const auto car=m.word(address_+0x2c+4*i);const auto p=m.vector(car+0x60);if(GetDistanceToClosestPlayer(std::array<float,3>{{p[0],p[1],p[2]}})<100)hp2::recovered::Car(m,car).TriggerEvents(service.event_disc_w);}
 const auto replay=[&]{const auto rc=m.word(0x327290);if(!rc)hp2::not_implemented("0x0014DD00","actual RaceCoordinator is required by World feedback");const auto state=m.word(rc);return state==4||state==7||state==5||state==9;};
 if(!replay())for(int i=0;i<m.signed_word(address_+0x20);++i){hp2::recovered::Car car(m,m.word(address_+0x2c+4*i));car.MaybeCameraShake(service.camera_shake_w);if(!replay())car.MaybeRumble();}
 AcidEffectSystem effects(m,0x2e6f40);effects.TweakerCheck();effects.DoTimeStep(timestep,service.acid);
 for(int i=0;i<m.signed_word(address_+0x1c);++i){const auto pointer=m.word(address_+0x2c+4*i);const auto p=m.vector(pointer+0x60);const float distance=GetDistanceToClosestPlayer(std::array<float,3>{{p[0],p[1],p[2]}});hp2::recovered::Car car(m,pointer);if(distance<150&&car.ShouldVisualEffectsBeGenerated())car.TriggerCarTerrainEffects(distance,service.terrain_disc_w);}
 for(int i=0;i<m.signed_word(address_+0x1c);++i)hp2::recovered::Car(m,m.word(address_+0x2c+4*i)).FakeEngineTask();
 ::port::race::EventDispatch events;events.ticker=service.physics.ticker;events.handlers.emplace(0x130b70,CarReflectEventHandler);emProcessAllEvents(m,events);
 Sort(m,address_,0x14d2e8);UpdatePlayersGameState(m,&service.ai);rain_TimeStep(timestep);ServiceFernFlatteners(m);ServiceTweakerDamage();ServiceTrackRouteEditor(m);
 if(const auto race=m.word(0x327258))Race(m,race).Update(service.physics,service.routes);
}
}

#include "game_core/src/bDebug.hpp"
namespace hp2::debug {
// Debug0014D348, E-0099-W75. Original order and raw callback identity, no host game callback.
void World::SetWorldPaused(float duration,std::uint32_t callback,int param){memory_.word(address_+0x190,std::uint32_t(param));memory_.scalar(address_+0x17c,duration);memory_.word(address_+0x18c,callback);}
// Debug0014D368, E-0099-W75. Expiry publishes zero before the required callback.
void World::UpdateWorldPaused(){auto& m=memory_;if(m.scalar(address_+0x17c)>0){const float left=m.scalar(address_+0x17c)-m.scalar(0x327ce4);m.scalar(address_+0x17c,left);if(left<=0){m.scalar(address_+0x17c,0);if(m.word(address_+0x18c))hp2::not_implemented("0x0014D3B0","actual original World pause callback identity is not bound");}}}
// Debug0014D3D0, E-0099-W75. Tweaker flag is fullword and evaluated after duration.
int World::IsWorldPaused()const{return memory_.scalar(address_+0x17c)>0||memory_.word(0x3272f4)!=0;}
// Debug0014D408, E-0099-W75. Quantized replay channel1 and true floating
// MAX.S/MIN.S wait clamp (decompiler integer casts are wrong). Source callbacks
// remain original-address boundaries; no arbitrary successful game callbacks.
float World::GetTimestep(const std::function<std::uint32_t()>& ticker){
 auto& m=memory_;const auto replay=m.word(0x3271a8);if(!replay)hp2::not_implemented("0x0014D408","actual ReplayManager owner is required by World clock");std::uint32_t quantized=m.word(0x327ce8);
 if(m.word(replay))quantized=hp2::recovered::ReplayManager(m,replay).GetData(8,1);else if(m.word(replay+4))hp2::recovered::ReplayManager(m,replay).AddData(std::int32_t(quantized),8,1);
 const float elapsed=hp2::recovered::GetRealTimeElapsedFromQuantized(m,std::int32_t(quantized));
 if(m.word(replay)&&!m.word(replay+0x10)&&!m.word(replay+0xc)){
  const float value=(elapsed-m.scalar(0x327ce4))+m.scalar(address_+0x180),maximum=m.scalar(0x324f50);if(!std::isfinite(value)||!std::isfinite(maximum))hp2::not_implemented("0x0014D4B8","exceptional EE World replay wait MAX.S/MIN.S operands");const float nonnegative=value>=0?value:0.f,wait=nonnegative<maximum?nonnegative:maximum;m.scalar(address_+0x180,wait);
  if(wait>0){if(!ticker)hp2::not_implemented("0x0024A340","actual replay pacing requires primitive Count reader");const auto start=ticker();float duration;do{const auto end=ticker();duration=bGetTickerDifference(m,start,end)*m.scalar(0x324f54);}while(duration<wait);}
 }
 const float before=m.scalar(address_+0x188);if(before>0){const float left=before-elapsed;m.scalar(address_+0x188,left);if(left<=0){m.scalar(address_+0x188,0);m.scalar(address_+0x184,1);if(m.word(address_+0x18c))hp2::not_implemented("0x0014D550","actual original World time-scale callback identity is not bound");}}
 return elapsed*m.scalar(address_+0x184);
}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug0014D240, E-0099-W76. Source signed DriverNumber -> signed one-byte ID;
// never serialize the native/logical pointer as if it were a stable game ID.
void World::DoPointerSnapshot(std::uint32_t snapshot,std::uint32_t ppcar){auto& m=memory_;::port::driving::Memory::Temporary number(m,4);const auto p=number.address(),car=m.word(ppcar);m.word(p,car?std::uint32_t(std::int32_t(m.signed_half(car+0xa))):0xffffffffu);ReplaySnapshot(m,snapshot).MoveData(p,1,4,1);if(m.byte(snapshot+0x3d))m.word(ppcar,m.signed_word(p)<0?0:GetCarByNumber(m.signed_word(p)));}
}

#include "game_core/indep/inc/Clans.hpp"
#include "game_core/Chopper.hpp"
#include "game_core/port/track/emitter_bindings.hpp"
namespace hp2::debug {
// Debug0014CE40, E-0099-W76. Actual ordered source World snapshot, including
// restore-only resource flushes. Resource primitives propagate explicitly into
// Car/Smokeable RigidBody take-side calculations; no whole-memory substitute.
void World::DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& services){auto& m=memory_;ReplaySnapshot s(m,snapshot);s.BeginSection(0x3272f8);s.MoveData(address_+0x178,4,4,0);s.MoveData(address_+0x17c,4);s.MoveData(address_+0x184,4);s.MoveData(address_+0x188,4);s.MoveData(address_+0x18c,4);s.MoveData(address_+0x190,4,4,1);s.MoveData(address_+0x150,12);s.MoveData(address_+0x160,12);s.MoveData(address_+0x170,4);s.MoveData(address_+0x174,4);s.MoveData(address_+0x198,4,4,0);s.MoveData(address_+0x19c,4,4,0);
 if(m.byte(snapshot+0x3d)){FlushClans(m);KillAllActiveEffects(m,0x2e6f40);FlushFernShakers(m);m.word(0x326d3c,1);}
 WorldTimeDoSnapshot(m,snapshot);if(const auto tele=m.word(address_+0x194))TrafficTeleporter(m,tele).DoSnapshot(snapshot);if(const auto graph=m.word(0x328ef8))tTrackGraph(m,graph).DoSnapshot(snapshot);
 for(int i=0;i<m.signed_word(address_+0x1c);++i){
  hp2::recovered::Car(m,m.word(address_+0x2c+4*i)).DoSnapshot(snapshot,services);
 }
 SnapshotCops(m,snapshot);
 if(const auto chopper=m.word(address_+0x18)){if(!services.routes)hp2::not_implemented("0x001E4250","actual Chopper snapshot requires loaded route/path resources");Chopper(m,chopper).DoSnapshot(snapshot,*services.routes);}
 if(m.byte(snapshot+0x3d))EliminateAllWitnesses(m);
 SnapshotAllSmokeables(m,snapshot,services);
 TrackPolygonCollisionBody::SnapshotEverything(m,snapshot);
 CollisionBody::SnapshotEverything(m,snapshot);
 CollisionBodyWitness::SnapshotEverything(m,snapshot);
 s.EndSection();
}
}

#include "game_core/TrafficTeleporter.hpp"
namespace hp2::debug {
// Debug0014E268, E-0099-W91; exact world.cpp definition owner. No table rebuild here.
void World::AddTrafficCar(std::uint32_t driver,const ::port::race::CarConstructionServices& construction,::port::race::PhysicsConstructionServices& physics,const ::port::race::AiDriveTargetServices& ai){
 const auto teleporter=memory_.word(address_+0x194);
 if(!teleporter)hp2::not_implemented("0x0014E268","original TrafficTeleporter owner is absent");
 const auto car=TrafficTeleporter(memory_,teleporter).PlaceNewCar(driver,construction,physics,ai);
 if(car){const auto prev=memory_.word(address_+4);memory_.word(prev,car);memory_.word(address_+4,car);memory_.word(car,address_);memory_.word(car+4,prev);}
}
}
