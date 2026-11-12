// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/collision.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0021EB20; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021eb20.md.
void Collision::IsTwoCarCollision() {
    hp2::not_implemented("0x0021EB20", "Collision::IsTwoCarCollision (signature and full body unresolved)");
}

// ELF 0x0021EB48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021eb48.md.
void Collision::GetCollidingCars() {
    hp2::not_implemented("0x0021EB48", "Collision::GetCollidingCars (signature and full body unresolved)");
}

// ELF 0x0021EBD8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021ebd8.md.
void Collision::IsSmokeableInvolved() {
    hp2::not_implemented("0x0021EBD8", "Collision::IsSmokeableInvolved (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/inc/bSlotPool.hpp"
namespace hp2::debug {
// Debug002191D8/00219208, E-0099-W38. Actual original collision object pool.
void InitCollision(::port::driving::Memory& m){m.word(0x328b20,bNewSlotPool(m,0x70,32,0x2b4548,0x10));}
void CloseCollision(::port::driving::Memory& m){bDeleteSlotPool(m,m.word(0x328b20));m.word(0x328b20,0);}
}
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/src/soundcollision.hpp"
namespace hp2::debug {
// Debug00219230, E-0099-W38. Sample field is untouched unless the original
// distance branch assigns it. The source calls sound BEFORE final timer stamps.
void Collision::Sub_00219230(std::uint32_t witness,std::uint32_t point){
 auto& m=m_;const auto p=address_,world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x00219230","Original Collision requires the current World");
 const auto head=world+0x10,tail=m.word(head+4);m.scalar(p+0x40,0);m.word(tail,p);m.word(head+4,p);m.scalar(p+0x48,0);m.scalar(p+0x44,0);m.word(p+0x54,witness);m.word(p+4,tail);m.word(p,head);
 m.word(p+8,0);m.word(p+0xc,0);m.scalar(p+0x50,0);m.word(p+0x5c,0);
 if(point){m.copy(p+0x10,point,16);const std::array<float,3> xyz{{m.scalar(point),m.scalar(point+4),m.scalar(point+8)}};
  if(World(m,world).GetDistanceToClosestPlayer(xyz)<100)m.word(p+0x58,collision_sound::CreateSample(m,p));
 }else{m.scalar(p+0x10,0);m.scalar(p+0x18,0);m.scalar(p+0x14,0);}
 const auto time=m.word(0x327cf8);m.scalar(p+0x60,0);m.word(p+0xc,time);m.word(p+8,time);m.scalar(p+0x68,0);m.scalar(p+0x64,0);
}
// Debug00219308/00219858, E-0099-W38. Unlink before required sound release.
void Collision::Sub_00219308(int flags){auto& m=m_;const auto next=m.word(address_),previous=m.word(address_+4);m.word(previous,next);m.word(next+4,previous);collision_sound::ReleaseSample(m,address_);if(flags&1)SlotPool(m,m.word(0x328b20)).Free(address_);}
}

#include "game_core/indep/inc/CollisionBody.hpp"
namespace port::race::detail {
// Original virtual consumer00219698; exact signed this delta from shared table.
int shared_collision_terrain(::port::driving::Memory& m,std::uint32_t body){
 const auto table=m.word(body+0x50),target=m.word(table+0x94),adjusted=body+static_cast<std::int32_t>(m.signed_half(table+0x90));
 switch(target){case 0x1d5ab0:return 32; //TERRAIN_TYPE_CAR, actual immediate001D5AB4.
 case 0x1d5760:return hp2::debug::TrackPolygonCollisionBody(m,adjusted).GetTerrainType();
 case 0x21c818:return m.signed_word(m.word(adjusted+0x110)+0xc0);
 default:hp2::not_implemented("0x00219698","Unrecovered Collision GetTerrainType virtual target");}
}
}
namespace hp2::debug {
// Debug00219698; E-0099-W38. Greater priority wins; equal priorities select body1.
int Collision::GetTerrainType()const{const auto w=m_.word(address_+0x54);const int first=::port::race::detail::shared_collision_terrain(m_,m_.word(w+0x60)),second=::port::race::detail::shared_collision_terrain(m_,m_.word(w+0x64));
 const auto priority=[&](int type){return m_.scalar(0x3102e8+static_cast<std::uint32_t>(type)*0x90+0x2c);};return priority(second)<=priority(first)?first:second;}
// Debug00219730/00219758/002197E8, E-0099-W38. Read actual Type, never mover mode.
bool Collision::IsTwoCarCollision()const{const auto w=m_.word(address_+0x54);return m_.byte(m_.word(w+0x60)+0xc)==0&&m_.byte(m_.word(w+0x64)+0xc)==0;}
void Collision::GetCollidingCars(std::uint32_t& first,std::uint32_t& second)const{const auto w=m_.word(address_+0x54),a=m_.word(w+0x60),b=m_.word(w+0x64);first=0;second=0;if(!m_.byte(a+0xc))first=m_.word(a+0x100);if(!m_.byte(b+0xc))second=m_.word(b+0x100);}
bool Collision::IsSmokeableInvolved()const{const auto w=m_.word(address_+0x54);return m_.byte(m_.word(w+0x60)+0xc)==1||m_.byte(m_.word(w+0x64)+0xc)==1;}
}

namespace hp2::debug {
// Debug00219650/00219690, E-0099-W38. MIPS cvt.s.W consumes signed32 timer
// subtraction despite uint rendering in decompile; timestep is unconsumed.
int Collision::Update(float){const auto difference=static_cast<std::int32_t>(m_.word(0x327cf8)-m_.word(address_+0xc));return static_cast<float>(difference)*m_.scalar(0x325f6c)<m_.scalar(0x328b1c);}
void Collision::End(){CollisionBodyWitness(m_,m_.word(address_+0x54)).EndCollision();}
// Debug002197A0, E-0099-W38. Actual player ownership pointer, not driver flags.
bool Collision::IsPlayerInvolved()const{std::uint32_t first,second;GetCollidingCars(first,second);return (first&&m_.word(first+0x18))||(second&&m_.word(second+0x18));}
}

#include "game_core/TerrainEffectEmitters.hpp"
#include <cmath>
namespace port::race::detail {std::uint32_t shared_collision_pointer(::port::driving::Memory&,std::uint32_t,unsigned);}
namespace hp2::debug {
// Debug00219400, E-0099-W38. Actual accumulated four-lane impulse, timer,
// contact point and real terrain/audio producers; no deferred host FX event.
void Collision::AddImpulse(std::uint32_t point,std::uint32_t impulse,float scrape){
 auto& m=m_;const auto input=m.vector(impulse);auto total=m.vector(address_+0x40);for(unsigned i=0;i<4;++i)total[i]+=input[i];m.vector(address_+0x40,total);
 const float square=(input[0]*input[0]+input[1]*input[1])+input[2]*input[2],magnitude=std::sqrt(square);m.word(address_+0xc,m.word(0x327cf8));m.scalar(address_+0x50,m.scalar(address_+0x50)+magnitude);m.copy(address_+0x20,point,16);m.scalar(address_+0x30,scrape);
 const auto world=m.word(0x3272f0);if(World(m,world).GetDistanceToClosestPlayer(std::array<float,2>{{m.scalar(point),m.scalar(point+4)}})>=50)return;
 const int terrain=GetTerrainType();const auto info=0x3102e8+static_cast<std::uint32_t>(terrain)*0x90;
 const auto normalized=[&](float value,float minimum,float maximum){const float amount=(value-minimum)/(maximum-minimum);return amount<0?0:amount>1?1:amount;};
 if(m.scalar(info+0x88)<scrape){const float intensity=normalized(scrape,m.scalar(info+0x88),m.scalar(info+0x8c));const auto first=m.word(m.word(address_+0x54)+0x60),velocity=::port::race::detail::shared_collision_pointer(m,first,0x20);(void)::port::race::detail::shared_collision_pointer(m,first,0x20);
  TerrainEffectEmitters::TriggerEffect(m,m.word(0x328b58),6,terrain,intensity,point,velocity);collision_sound::PlayScrape(m,address_);
 }
 if(m.scalar(info+0x7c)<magnitude){const float intensity=normalized(magnitude,m.scalar(info+0x7c),m.scalar(info+0x80));::port::driving::Memory::Temporary velocity(m,16);for(unsigned i=0;i<3;++i)m.scalar(velocity.address()+i*4,0);
  // The original stack velocityW is unwritten; TriggerEffect consumes XYZ only
  // and explicitly writes InheritVelocityW0, so that unknown lane is not read.
  TerrainEffectEmitters::TriggerEffect(m,m.word(0x328b58),5,terrain,intensity,point,velocity.address());
 }
 if(!(square>0)||!std::isfinite(square))hp2::not_implemented("0x00219608","Exceptional original impulse-normal VU reciprocal square root is not modeled");auto normal=input;const float scale=1/std::sqrt(square);for(auto& lane:normal)lane*=scale;m.vector(address_+0x60,normal);
}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug00219370, E-0099-W76. Two packed timers, three XYZ vectors and magnitude.
void Collision::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(m_,snapshot);s.BeginSection(address_,0x2b4560);s.MoveData(address_+8,4);s.MoveData(address_+0xc,4);s.MoveData(address_+0x10,12);s.MoveData(address_+0x20,12);s.MoveData(address_+0x40,12);s.MoveData(address_+0x50,4);s.EndSection();}
}
