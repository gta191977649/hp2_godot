#include "game_core/port/race/physics_construction.hpp"
#include "game_core/port/race/car_reset_services.hpp"
#include <cstdint>
namespace port::driving {class Memory;}
namespace port::race::cop_logic_detail {static float distance(const driving::Memory&,std::uint32_t,std::uint32_t);}
#include "game_core/port/race/damage_resources.hpp"
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/cop.hpp"
#include <cmath>
#include <utility>

#include "game_core/port/not_implemented.hpp"

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/CopChase.hpp"
#include "game_core/indep/src/coputil.hpp"
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/src/coputil.hpp"
#include "game_core/indep/src/pursuit.hpp"
#include "game_core/indep/inc/TrackRoute.hpp"
namespace hp2::debug {
// Debug00227BB8,E-0100-PA01. Exact source pointer setter.
void tCop::SetPursuit(std::uint32_t pursuit){m_.word(address_,pursuit);}
// Debug002290D0,E-0100-PA01. Listing uses strict80<speed (NaN does not admit).
bool tAICop::IsValidPursuitTarget(std::uint32_t car,bool in_view){auto& m=m_;const auto manager=m.word(0x328c1c);if(!manager)hp2::not_implemented("0x002290D0","actual CopManager required by pursuit target admission");const int cop_index=m.signed_word(manager+0xc);std::uint32_t player_cop=0;if(cop_index!=-1){if(cop_index<0||cop_index>=7)hp2::not_implemented("0x002290D0","player cop index outside original seven-pointer table");player_cop=m.word(manager+0x10+4*unsigned(cop_index));}const auto player_car=player_cop?m.word(player_cop+4):0;if(car==player_car)return false;const int index=m.signed_half(car+8);if(index<0||index>=8)hp2::not_implemented("0x002290D0","pursuit target index outside original eight pullover/timer rows");if(m.word(manager+0x64+0x1c*unsigned(index))||0.f<m.scalar(manager+0x2874+4*unsigned(index)))return false;if(CarCausedRecentCollision(m,car))return true;if(!in_view)return false;const float speed=m.scalar(car+0x3c0)*m.scalar(0x326088);if(!std::isfinite(speed))hp2::not_implemented("0x002291B0","original pursuit speed conversion exceeds reviewed finite PC domain");return 80.f<std::fabs(speed)||tCopManager(m,manager).IsBeingPursued(car)!=0;}
// Debug00227AC8, E-0099-W107; exact original cop.cpp definition owner.
void tCop::BaseDoSnapshot(std::uint32_t snapshot){auto& m=m_;ReplaySnapshot s(m,snapshot);s.BeginSection(0x328c78);World(m,m.word(0x3272f0)).DoPointerSnapshot(snapshot,address_+4);tCopManager(m,m.word(0x328c1c)).DoPointerSnapshot(snapshot,::port::race::SnapshotPursuitPointer{address_});if(const auto pursuit=m.word(address_)){if(!tPursuit(m,pursuit).HasThisCop(address_)||m.signed_byte(pursuit+0x5c8)<0)m.word(address_,0);}for(unsigned off:{0xcu,0x14u,0x10u,0x18u})s.MoveData(address_+off,1);s.EndSection();}
// Debug00228398, E-0099-W107; parent and pointer codecs retain source widths.
void tAICop::DoSnapshot(std::uint32_t snapshot){auto& m=m_;ReplaySnapshot s(m,snapshot);s.BeginSection(address_,0x328c78);tCop(m,address_).BaseDoSnapshot(snapshot);tCopManager(m,m.word(0x328c1c)).DoPointerSnapshot(snapshot,::port::race::SnapshotSpeedTrapPointer{address_+0x20});s.MoveData(address_+0x24,1,4,1);s.MoveData(address_+0x50,4);s.MoveData(address_+0x54,4);s.MoveData(address_+0x58,4,4,1);for(unsigned off:{0x28u,0x2cu,0x30u,0x34u,0x38u,0x3cu,0x40u,0x44u})s.MoveData(address_+off,4);TrackRouteManager manager(m,0x30bb30);manager.DoPointerSnapshot(snapshot,::port::race::SnapshotRoutePointer{address_+0x48});manager.DoPointerSnapshot(snapshot,::port::race::SnapshotRoutePointPointer{address_+0x4c},m.word(address_+0x48));if(m.byte(snapshot+0x3d))if(const auto trap=m.word(address_+0x20))m.word(trap,address_);s.EndSection();}
}

namespace hp2::recovered {
// ELF 0x0022ED68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0022ed68.md.
void tAICop::CanPatrollerSeeThisCar() {
    hp2::not_implemented("0x0022ED68", "tAICop::CanPatrollerSeeThisCar (signature and full body unresolved)");
}

// ELF 0x00230BF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00230bf8.md.
void tAICop::IsNearPursuitTarget() {
    hp2::not_implemented("0x00230BF8", "tAICop::IsNearPursuitTarget (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/CopChase.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/port/race/car_construction.hpp"
#include "game_core/port/race/control_construction.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include <cmath>
namespace hp2::debug {
// Debug00227A80/00227AA0, E-0099-W91, exact cop.cpp definitions.
void tCop::Sub_00227A80(int number){m_.word(address_+8,std::uint32_t(number));m_.word(address_,0);m_.word(address_+0x1c,0x2b51b8);m_.word(address_+0xc,0);}
void tCop::Sub_00227AA0(int flags){m_.word(address_+0x1c,0x2b51b8);if(flags&1)m_.release(address_);}
// Debug00227BD0/00227C38, E-0099-W91. Full-word previous-light reset.
void tPlayerCop::Sub_00227BD0(std::uint32_t car,int number){tCop(m_,address_).Sub_00227A80(number);m_.word(address_+0x1c,0x2b5190);m_.word(address_+4,car);m_.word(address_+0x20,0xffffffff);m_.word(address_+0x24,0xffffffff);m_.scalar(address_+0x28,0);m_.scalar(address_+0x2c,0);m_.word(address_+0x30,0);m_.word(address_+0x10,0);}
void tPlayerCop::Sub_00227C38(int flags){m_.word(address_+0x1c,0x2b5190);tCop(m_,address_).Sub_00227AA0(flags);}
// Debug00229DB8, E-0099-W91. Literal original36-entry store sequence.
void InitZonePermissionTable(::port::driving::Memory& m){const unsigned values[]{0,0,0,0,1,0,1,0,1,0,1,1,1,1,0,1,1,0,1,0,1,0,1,1,1,1,1,1,0,1,1,1,0,1,1,0};for(unsigned i=0;i<36;++i)m.word(0x313c10+4*i,values[i]);}
// Debug002281A8, E-0099-W91. Source mutation of shared type mass/inertia is intentional.
// Native arithmetic is bounded to normal/signed-zero encodings; no VU bit-equivalence claim.
void tAICop::Sub_002281A8(std::uint32_t driver,int number,const ::port::race::CarConstructionServices& construction,::port::race::PhysicsConstructionServices& physics){
 auto& m=m_;tCop(m,address_).Sub_00227A80(number);m.word(address_+0x24,0);m.word(address_+0x48,0);m.word(address_+0x4c,0);m.word(address_+0x28,0);m.word(address_+0x1c,0x2b5168);
 ::port::driving::Memory::Temporary pos(m,16);m.word(pos.address(),0);m.word(pos.address()+4,0);m.word(pos.address()+8,0);if(!construction.port_stack_vector_lane)hp2::not_implemented("0x002281A8","original initial Car position fourth lane requires explicit PC policy");m.word(pos.address()+12,*construction.port_stack_vector_lane);
 const auto car=m.allocate(0x890);hp2::recovered::Car object(m,car);object.Sub_00130B90(driver,pos.address(),0,construction);m.word(address_+4,car);
 const auto world=m.word(0x3272f0),manager=m.word(0x328c1c);if(!world||!manager)hp2::not_implemented("0x002281A8","actual World and CopManager owners required");const auto type=m.word(car+0xc);const int difficulty=m.signed_word(manager+0x28ec);if(difficulty<0||difficulty>2)hp2::not_implemented("0x00228250","original cop mass table index outside original three-difficulty domain");
 const auto prev=m.word(world+4);m.word(prev,car);m.word(world+4,car);m.word(car+4,prev);m.word(car,world);
 const auto normal=[](float value){return std::isfinite(value)&&(value==0||std::isnormal(value));};const float mass=m.scalar(type+0x1e0),selected=m.scalar(0x313c00+4*std::uint32_t(difficulty));if(!normal(mass)||!normal(selected)||mass==0)hp2::not_implemented("0x00228258","cop mass DIV.S encoding outside finite normal nonzero-denominator scope");const float ratio=selected/mass,newmass=mass*ratio;if(!normal(ratio)||!normal(newmass))hp2::not_implemented("0x00228274","cop mass result encoding outside finite normal scope");m.scalar(type+0x1e0,newmass);
 for(unsigned column=0;column<3;++column)for(unsigned lane=0;lane<4;++lane){const auto at=type+0x1f0+16*column+4*lane;const float operand=m.scalar(at),product=operand*ratio;if(!normal(operand)||!normal(product))hp2::not_implemented("0x00228280","original VMULx inertia encoding requires unimplemented VU domain");m.scalar(at,product);}
 object.SetControlMode(4,*construction.routes);const auto control=m.word(car+0x28);m.word(control+0x24,1);m.word(control+0x3c,m.word(address_+8));object.SetMovementMode(0,physics);m.byte(car+0x4a8,0);m.word(address_+0x50,0);m.word(address_+0x10,1);m.word(address_+0x54,0);m.word(address_+0x58,0);m.scalar(address_+0x3c,m.scalar(0x328c70));m.scalar(address_+0x40,m.scalar(0x328c74));for(unsigned off:{0x44u,0x30u,0x2cu,0x38u,0x34u})m.word(address_+off,0);m.word(car+0x86c,0);m.word(car+0x834,0);m.word(car+0x808,0);InitZonePermissionTable(m);
}
// Debug00228338 ->0022B388, E-0099-W91. Car belongs to World, not this pool object.
void tAICop::Sub_00228338(int flags){m_.word(address_+0x1c,0x2b5168);tCop(m_,address_).Sub_00227AA0(0);if(flags&1)SlotPool(m_,m_.word(0x328c08)).Free(address_);}
}

#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/inc/PsychoDriver.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/src/bMath.hpp"
#include <cstring>
namespace port::race {
// Port-only consumer projection, E-0099-W96. Sony original corev6 pp162-165,
// DIV.S357/MUL.S372, together with source228798..228828. No raw Fmax/FCSR model.
// With raw+0 delta, normal rate>=1 and positive-normal dt, the actual extended
// product is strictly positive; original ABS/branch selects the exact +0 delta.
static float cop_dl_selected_delta(float delta,float rate,float elapsed,const char* instruction){
 std::uint32_t bits;std::memcpy(&bits,&delta,4);
 if(!bits){if(std::isnormal(rate)&&std::isfinite(rate)&&rate>=1&&std::isnormal(elapsed)&&std::isfinite(elapsed)&&elapsed>0)return delta;hp2::not_implemented(instruction,"zero cop DL sign division outside proved positive-normal rate/time consumer scope");}
 if(!std::isnormal(delta)||!std::isfinite(delta)||!std::isfinite(rate)||!std::isfinite(elapsed)||(rate!=0&&!std::isnormal(rate))||(elapsed!=0&&!std::isnormal(elapsed)))hp2::not_implemented(instruction,"cop DL arithmetic encoding outside finite normal scope");
 const float sign=delta/std::abs(delta),step=(sign*rate)*elapsed;
 if(!std::isfinite(step)||(step!=0&&!std::isnormal(step)))hp2::not_implemented(instruction,"cop DL product encoding outside finite normal scope");
 return std::abs(delta)<std::abs(step)?delta:step;
}
}
namespace hp2::debug {
// Debug00228B60, E-0099-W96. Source null-pursuit branch is truly empty.
void tAICop::ClearPursuit(){if(const auto pursuit=m_.word(address_)){tPursuit(m_,pursuit).RemoveCopFromPursuit(address_);tCop(m_,address_).SetPursuit(0);}}
// Debug0022B320; E-0100-CM01; original _GLOBAL_$I$fNearFollowDistances store.
void Sub_0022B320(::port::driving::Memory& m){m.half(0x329458,0x71c);}
// Debug00228BC8; E-0100-CM01. Original wheel/angle/angular-speed delay and
// COP<->MELLOW transitions, with actual mover/driver resources.
void tAICop::MovementModeTransitions(::port::race::PhysicsConstructionServices* physics){
 auto& m=m_;const auto car=m.word(address_+4),mode=m.word(car+0x24);
 if(!mode||m.word(address_+0x10)==2)return;
 const auto length=[&](const std::array<float,4>& v,const char* mul,const char* add_y,const char* add_z){const float x=::port::driving::ee_vmul_finite(v[0],v[0],mul),y=::port::driving::ee_vmul_finite(v[1],v[1],mul),z=::port::driving::ee_vmul_finite(v[2],v[2],mul);return std::sqrt(::port::driving::ee_vadd_finite(::port::driving::ee_vadd_finite(x,y,add_y),z,add_z));};
 if(mode==5){
  m.word(address_+0x50,0);if(!m.word(m.word(car+0x30)+0x70))return;
  auto velocity=m.vector(car+0x70);if(velocity[2]>2)velocity[2]=2;m.vector(car+0x70,velocity);
  m.scalar(car+0x3c4,length(velocity,"0x00228D4C","0x00228D50","0x00228D54"));m.word(car+0x440,0);m.scalar(address_+0x54,m.scalar(0x326080));
  if(!physics)hp2::not_implemented("0x001316F8","cop collision transition requires actual shared Mellow construction resources");
  hp2::recovered::Car(m,car).SetMovementMode(4,*physics);
  const auto driver=m.word(car+0x2c);m.word(driver+0x220,0xffffffff);m.scalar(driver+0x214,1);m.word(driver+0x218,0);m.word(driver+0x21c,0);m.half(driver+0x210,0);
  const auto table=m.word(driver+0x22c),target=m.word(table+0x3c),self=driver+static_cast<std::int32_t>(m.signed_half(table+0x38));
  if(target==0x1398a8)PsychoDriver(m,self).SetDesiredSpeed(m.scalar(car+0x3c0),.5f,true);
  else if(target!=0x136b88)hp2::not_implemented("0x00228DC8","unrecovered original desired-speed virtual target after cop Mellow transition");
  return;
 }
 const float seconds=m.scalar(address_+0x50)+m.scalar(0x327d08);m.scalar(address_+0x50,seconds);if(!(seconds>=0))return;
 bool stable=false;
 if(GetNumWheelsOnGround(m,car)>3){
  ::port::driving::Memory::Temporary polar(m,8);hp2::recovered::bToPolar(m,polar.address(),{{m.scalar(car+0x70),m.scalar(car+0x74)}});
  const int difference=static_cast<std::int16_t>(static_cast<std::uint16_t>(m.half(polar.address()+4)-m.half(car+0x3a4)));
  if(std::abs(difference)<=int(m.half(0x329458))||m.scalar(car+0x3c4)<=m.scalar(0x326078))stable=length(m.vector(car+0xd0),"0x00228C8C","0x00228C90","0x00228C94")<=1;
 }
 if(!stable)m.scalar(address_+0x54,m.scalar(0x32607c));
 const float delay=m.scalar(address_+0x54)-m.scalar(0x327d08);m.scalar(address_+0x54,delay);
 if(delay<=0){m.word(address_+0x54,0);if(!physics)hp2::not_implemented("0x001316F8","stable cop transition requires actual shared CopMover construction resources");hp2::recovered::Car(m,car).SetMovementMode(5,*physics);}
}
// Debug00228840, E-0099-W96. Exact reached LIMBO entry; active state children explicit.
void tAICop::HandleStateTransitions(int current,int next,::port::race::PhysicsConstructionServices* physics){auto& m=m_;m.word(address_+0x28,0);const auto car=m.word(address_+4),controller=m.word(car+0x28);if(current==2)tCopManager(m,m.word(0x328c1c)).ResetAllDamage(car);if(current==1){CollisionBody(m,m.word(car+0x34)).EnableCollisions();m.byte(car+0x4a8,1);if(!physics)hp2::not_implemented("0x001316F8","actual cop state transition requires shared physics resources");hp2::recovered::Car(m,car).SetMovementMode(5,*physics);tCopManager(m,m.word(0x328c1c)).ResetAllDamage(car);}if(current==3)ClearSpeedTrap();switch(next){case 1:ClearPursuit();m.word(car+0x86c,0);m.word(car+0x834,0);m.word(car+0x808,0);if(!physics)hp2::not_implemented("0x001316F8","actual LIMBO entry requires shared physics resources");hp2::recovered::Car(m,car).SetMovementMode(0,*physics);m.word(controller+0x24,1);CollisionBody(m,m.word(car+0x34)).DisableCollisions();m.byte(car+0x4a8,0);if(current==2){m.word(address_+0x14,11);m.scalar(address_+0x28,1);}else m.word(address_+0x14,12);break;
 case 3:tCopManager(m,m.word(0x328c1c)).ResetAllDamage(car);InitSpeedTrapLogic(physics);break;
 case 4:tCopManager(m,m.word(0x328c1c)).ResetAllDamage(car);InitRoadBlockLogic(physics);break;
 case 5:{ClearPursuit();m.word(address_+0x14,0);m.word(car+0x86c,0);const auto ai=physics&&physics->car_reset?physics->car_reset->ai:nullptr;if(!ai)hp2::not_implemented("0x0022897C","source patrol entry requires actual route/local stack services");InitializeDriveTarget(*ai);UpdateDriveTarget({{m.scalar(car+0x60),m.scalar(car+0x64)}},false,*ai,physics->cop_target_point_w);m.word(controller+0x24,2);m.word(car+0x808,0);m.word(car+0x834,0);break;}
 case 7:tCop(m,address_).SetPursuit(0);m.word(controller+0x24,1);m.word(address_+0x14,13);m.word(car+0x808,1);m.word(car+0x86c,1);m.word(car+0x834,0);break;
 case 8:ClearSpeedTrap();ClearPursuit();tCopManager(m,m.word(0x328c1c)).ResetAllDamage(car);m.word(controller+0x24,1);m.word(car+0x808,0);m.word(address_+0x14,0);m.word(car+0x86c,0);m.word(car+0x834,0);break;
 case 2:{ClearSpeedTrap();ClearPursuit();m.word(controller+0x24,0);m.word(address_+0x14,0);m.word(car+0x834,4);m.word(car+0x808,1);m.word(car+0x86c,0);if(!physics)hp2::not_implemented("0x00228AA8","source disabled Mellow mode requires actual physics");hp2::recovered::Car(m,car).SetMovementMode(4,*physics);const auto driver=m.word(car+0x2c);m.word(driver+0x214,0);m.word(driver+0x218,m.word(0x326074));m.scalar(driver+0x21c,1);m.word(driver+0x224,0xffffffff);m.half(driver+0x210,0);m.word(driver+0x220,0xffffffff);const auto vt=m.word(driver+0x22c),target=m.word(vt+0x3c),self=driver+std::int32_t(m.signed_half(vt+0x38));if(target==0x1398a8)PsychoDriver(m,self).SetDesiredSpeed(0,10,true);else if(target!=0x136b88)hp2::not_implemented("0x00228AFC","source disabled driver desired-speed target unavailable");break;}
 case 6:m.word(car+0x808,2);m.word(controller+0x24,3);m.word(address_+0x18,1);m.word(car+0x834,1);m.word(car+0x86c,1);m.word(address_+0x14,0);break;
 default:if(next>=1&&next<=8)hp2::not_implemented("0x00228840","actual requested active cop state-entry child chain required");break;}}
// Debug0022B0D0, E-0099-W96. Real CarDriver-owned DriveTarget and full position copy.
void tAICop::BuildStationaryDriveTarget(){const auto car=m_.word(address_+4),driver=m_.word(car+0x2c);if(!driver)hp2::not_implemented("0x0022B0D0","source stationary target requires actual CarDriver before original null dereference");DriveTarget target(m_,driver);target.Clear(0,0);target.AddPointToTarget(car+0x60,-1,-1,-1,false,0);}
// Debug0022ABE0, E-0099-W96. Source disabled-to-ready strict zero timer.
void tAICop::DoLimboLogic(){BuildStationaryDriveTarget();if(m_.word(address_+0x14)==11&&m_.scalar(address_+0x28)==0)m_.word(address_+0x14,12);}
// Debug0022B138, E-0099-W96. Source existing LIMBO/DISABLED early returns only.
void tAICop::DisableMyCar(bool caused,::port::race::PhysicsConstructionServices* physics){auto& m=m_;const auto a=address_;if(m.word(a+0xc)-1u<2)return;const auto car=m.word(a+4);if(const auto damage=m.word(car+0x844))CarDamage(m,damage).SetDamageToFull();if(const auto pursuit=m.word(a)){auto chased=m.word(pursuit+0x10);if(m.word(chased+0x18)&&::port::race::cop_logic_detail::distance(m,car,chased)<75)tPursuit(m,pursuit).RemovePrimarySpeaker(caused,physics&&physics->disable_pursuit_speech_emission);chased=m.word(m.word(a)+0x10);if(chased&&m.word(chased+0x18)){const int index=m.signed_half(chased+8);if(index<0||index>=8)hp2::not_implemented("0x0022B138","source speed-trap respawn timer index outside eight cars");const auto at=m.word(0x328c1c)+0x2894+4*unsigned(index);m.scalar(at,m.scalar(at)+10);}ClearPursuit();}tCop(m,a).SetDesiredLogicState(2);}
// Debug002285F8, E-0099-W96. Original order; unknown states retain precise callees.
void tAICop::Think(::port::race::PhysicsConstructionServices* physics){auto& m=m_;auto car=m.word(address_+4);if(m.signed_word(0x327540)==m.signed_half(car+0xa))DriveTargetRenderer(m,0x30bd18).SetTargetToRender(m.word(car+0x2c));MovementModeTransitions(physics);car=m.word(address_+4);if(m.scalar(car+0xb8)<=m.scalar(0x32606c)&&m.scalar(car+0x3c4)*m.scalar(0x326070)<10){DisableMyCar(true,physics);car=m.word(address_+4);}if(m.byte(car+0x848))DisableMyCar(false,physics);const float remaining=m.scalar(address_+0x28)-m.scalar(0x327d08);m.scalar(address_+0x28,remaining<0?0:remaining);auto state=m.word(address_+0xc);if(state!=m.word(address_+0x10)){HandleStateTransitions(static_cast<int>(state),m.signed_word(address_+0x10),physics);state=m.word(address_+0x10);m.word(address_+0xc,state);}switch(state){case 1:DoLimboLogic();break;case 2:DoDisabledLogic();break;case 3:DoSpeedTrapLogic(physics);break;case 4:DoRoadBlockLogic(physics);break;case 5:DoDrivingOnPatrolLogic(physics);break;case 6:DoHotPursuitLogic(physics);break;case 7:DoCaughtSpeederLogic(physics);break;case 8:DoGuardLocationLogic();break;default:break;}
 const float current_d=m.scalar(address_+0x2c),current_l=m.scalar(address_+0x30),delta_d=m.scalar(address_+0x34)-current_d,delta_l=m.scalar(address_+0x38)-current_l,elapsed=m.scalar(0x327d08);const float selected_d=::port::race::cop_dl_selected_delta(delta_d,m.scalar(address_+0x3c),elapsed,"0x0022879C");const float selected_l=::port::race::cop_dl_selected_delta(delta_l,m.scalar(address_+0x40),elapsed,"0x002287A8");m.scalar(address_+0x2c,current_d+selected_d);m.scalar(address_+0x30,current_l+selected_l);m.scalar(address_+0x40,m.scalar(0x328c74));m.scalar(address_+0x3c,m.scalar(0x328c70));}
}

namespace hp2::debug {
// Debug0022B2C8,E-0099-W121; actual cop.cpp. The disabled branch is a real no-op.
void tAICop::DoneWritingTicket(){const auto state=m_.word(address_+0x10);if(state==2)return;if(state!=7)tCop(m_,address_).SetDesiredLogicState(7);m_.word(address_+0x14,14);m_.scalar(address_+0x28,10);}
}

namespace hp2::debug {
// Debug00227BC0,E-0099-W121; fresh mdebug proves original cop.cpp owner.
void tCop::SetDesiredLogicState(int state){m_.word(address_+0x10,static_cast<std::uint32_t>(state));}
}


#include "game_core/port/race/car_reset_services.hpp"
#include "game_core/port/race/physics_construction.hpp"
namespace hp2::debug {
// Debug002284E0/00228548,E-0110-F07. XYZ is source-written; W is an independent port operand.
void tAICop::Teleport(std::uint32_t position,std::uint16_t angle,::port::race::PhysicsConstructionServices* physics,const ::port::race::CarResetServices* reset){const auto car=m_.word(address_+4);if(!car)return;::port::driving::Memory::Temporary local(m_,16);m_.word(local.address(),m_.word(position));m_.word(local.address()+4,m_.word(position+4));m_.scalar(local.address()+8,0);if(!physics)hp2::not_implemented("0x001316F8","actual shared physics required by cop Teleport");hp2::recovered::Car vehicle(m_,car);vehicle.SetMovementMode(5,*physics);if(!reset||!reset->cop_teleport_position_w)hp2::not_implemented("0x002284E0","original cop teleport local position W requires independent explicit port operand");m_.word(local.address()+12,*reset->cop_teleport_position_w);vehicle.ResetToPosition(local.address(),0,angle,*reset);}
void tAICop::SetSpeedTrap(std::uint32_t trap,::port::race::PhysicsConstructionServices* physics,const ::port::race::CarResetServices* reset){m_.word(address_+0x20,trap);m_.word(trap,address_);Teleport(trap+4,m_.half(trap+0xc),physics,reset);tCop(m_,address_).SetDesiredLogicState(3);}
}

namespace hp2::debug {
// Debug00228BA8/00228DE0/00228E80,E-0110-F07.
void tAICop::ClearSpeedTrap(){if(const auto trap=m_.word(address_+0x20)){m_.word(trap,0);m_.word(address_+0x20,0);}}
void tAICop::InitSpeedTrapLogic(::port::race::PhysicsConstructionServices* physics){ClearPursuit();auto& m=m_;const auto car=m.word(address_+4),controller=m.word(car+0x28);m.word(address_+0x14,0);m.word(car+0x86c,0);m.scalar(car+0x3c0,0);m.word(controller+0x24,1);if(!physics||!physics->cop_speedtrap_velocity_w)hp2::not_implemented("0x00228DE0","original speed-trap zero-velocity local W requires independent explicit port operand");m.word(car+0x70,0);m.word(car+0x74,0);m.word(car+0x78,0);m.word(car+0x7c,*physics->cop_speedtrap_velocity_w);m.scalar(car+0x3c4,0);m.scalar(address_+0x28,15);m.word(car+0x440,0);m.word(car+0x808,0);m.word(car+0x834,0);}
void tAICop::DoSpeedTrapLogic(::port::race::PhysicsConstructionServices* physics){BuildStationaryDriveTarget();auto& m=m_;const auto world=m.word(0x3272f0),car=m.word(address_+4);if(!world)hp2::not_implemented("0x00228E80","actual World required by speed-trap logic");if(m.scalar(address_+0x28)<=0&&!(World(m,world).GetDistanceToClosestPlayer(std::array<float,2>{{m.scalar(car+0x60),m.scalar(car+0x64)}})<250.f)){tCop(m,address_).SetDesiredLogicState(1);return;}const auto trap=m.word(address_+0x20);const float dx=m.scalar(trap+4)-m.scalar(car+0x60),dy=m.scalar(trap+8)-m.scalar(car+0x64);const auto angle=static_cast<std::int16_t>(m.half(trap+0xc)-m.half(car+0x3a4));if((m.scalar(0x326084)<std::sqrt(dx*dx+dy*dy)||100.f<std::fabs(static_cast<float>(angle)))&&CanTeleport(m,car,{{m.scalar(trap+4),m.scalar(trap+8)}}))Teleport(trap+4,m.half(trap+0xc),physics,physics?physics->car_reset:nullptr);for(int i=0;i<m.signed_word(world+0x24);++i){const auto candidate=m.word(world+0x2c+4*i);const float x=m.scalar(candidate+0x60)-m.scalar(car+0x60),y=m.scalar(candidate+0x64)-m.scalar(car+0x64);if(std::sqrt(x*x+y*y)<45.f&&std::fabs(m.scalar(candidate+0x68)-m.scalar(car+0x68))<2.5f&&IsValidPursuitTarget(candidate,true)&&tCopManager(m,m.word(0x328c1c)).RequestPursuit(address_,candidate,physics))tCop(m,address_).SetDesiredLogicState(6);}}
}

// TASK-0100 E-0100-PU04, original cop.cpp. Preserve CM01 movement/state
// transition code above. This scheduler never treats desired as current twice.
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/src/coputil.hpp"
namespace hp2::debug {
void tAICop::DoPursuitFollowLogic(int previous,std::uint32_t chased,const ::port::race::AiDriveTargetServices* ai){
 auto& m=m_;const auto a=address_,p=m.word(a),car=m.word(a+4);
 if(!p||!car||!chased)hp2::not_implemented("0x00229F20","actual source pursuit/cop/chased owners required");
 const auto follow_distance=[&]{return m.scalar(m.word(p+0x1c)==a?0x328c68:0x328c6c);};
 if(previous!=m.signed_word(a+0x14)){m.scalar(a+0x28,3);m.scalar(a+0x34,follow_distance());m.scalar(a+0x38,0);}
 const float x=m.scalar(chased+0x60)-m.scalar(car+0x60),y=m.scalar(chased+0x64)-m.scalar(car+0x64);const float distance=std::sqrt(x*x+y*y);
 if(!std::isfinite(distance))hp2::not_implemented("0x00229F20","original pursuit follow distance exceptional EE operand");
 if(21.f<=distance){m.scalar(a+0x28,3);m.scalar(a+0x34,follow_distance());m.scalar(a+0x38,m.word(p+0x1c)==a?0.f:2.5f);return;}
 if(m.scalar(a+0x34)==0)m.scalar(a+0x34,follow_distance());
 const auto manager=m.word(0x328c1c),world=m.word(0x3272f0);if(!manager||!world)hp2::not_implemented("0x00229F20","actual CopManager/World required for source close-follow selection");
 const int aggression=m.word(chased+0x18)?m.signed_word(manager+0x28ec):2;
 if(aggression<0||aggression>2)hp2::not_implemented("0x00229F20","source aggression index outside original three entries");
 const float choice=World(m,world).Random(m.scalar(0x2b50f8+4*unsigned(aggression)));
 if(choice<1.f&&m.scalar(a+0x28)==0&&!DidTheseCarsCollide(m,car,chased)){const float side=GetBestFlankSide(chased,ai);if(side!=0){const auto first=m.word(p+0x1c),other=first==a?m.word(p+0x20):first;const auto other_car=other?m.word(other+4):0;bool allowed=true;if(other_car)allowed=RequestZonePermission(m,GetZoneFromSubState(eLSS_PURSUIT_FOLLOW_OFFSET,side),GetZoneFromSubState(m.signed_word(other+0x14),m.scalar(other+0x44)))!=0;if(allowed){m.scalar(a+0x44,side);m.word(a+0x18,eLSS_PURSUIT_FOLLOW_OFFSET);}}}
 if(std::fabs(m.scalar(chased+0x3c0)*m.scalar(0x32609c))<20.f)m.word(a+0x18,eLSS_PURSUIT_LOW_SPEED_CLOSE_FOLLOW);
}
void tAICop::DoHotPursuitLogic(::port::race::PhysicsConstructionServices* physics){
 auto& m=m_;const auto a=address_,p=m.word(a),car=m.word(a+4);
 const int previous=m.signed_word(a+0x14);
 if(!p||!car)hp2::not_implemented("0x0022A5A0","actual source pursuit and cop car required before original dereference");
 const auto chased=m.word(p+0x10);if(!chased)hp2::not_implemented("0x0022A5A0","actual chased car required before original dereference");
 if(previous!=m.signed_word(a+0x18))m.word(a+0x14,m.word(a+0x18));
 const float distance=TrackPathManager::CompareDistances(m.scalar(0x313308),m.scalar(car+0x7f4),m.scalar(chased+0x7f4));
 if(!std::isfinite(distance))hp2::not_implemented("0x0022A5A0","original HOT wrapped track-distance exceptional operand");
 if(450.f<=std::fabs(distance)){tCop(m,a).SetDesiredLogicState(5);ClearPursuit();return;}
 if(distance<=-180.f)m.word(a+0x18,eLSS_PURSUIT_ACQUIRE);
 else if(6.f<=distance&&20.f<=m.scalar(chased+0x3c0)*m.scalar(0x3260b0)){
  hp2::recovered::Car target(m,chased);const auto ai=physics&&physics->car_reset?physics->car_reset->ai:nullptr;
  const auto point=ai?target.GetRoutePoint(*ai):target.GetRoutePoint();if(!point)hp2::not_implemented("0x0022A5A0","actual source chased routepoint required by HOT direction test");
  const float direction=m.scalar(chased+0x70)*m.scalar(point+0xc)+m.scalar(chased+0x74)*m.scalar(point+0x10);
  m.word(a+0x18,0.f<=direction?eLSS_PURSUIT_BLOCK:eLSS_PURSUIT_FOLLOW_BACKWARDS);
 }
 // Dispatch the current substate captured after the one source synchronization;
 // desired updates above take effect at the next original HOT tick.
 const auto ai=physics&&physics->car_reset?physics->car_reset->ai:nullptr;
 switch(m.signed_word(a+0x14)){
 case eLSS_PURSUIT_FOLLOW:DoPursuitFollowLogic(previous,chased,ai);return;
 case eLSS_PURSUIT_FOLLOW_OFFSET:DoPursuitFollowOffsetLogic(previous,chased,ai);return;
 case eLSS_PURSUIT_LOW_SPEED_CLOSE_FOLLOW:DoLowSpeedClosePursuitFollowLogic(previous,chased);return;
 case eLSS_PURSUIT_FOLLOW_BACKWARDS:DoPursuitFollowBackwardsLogic(previous,chased,ai);return;
 case eLSS_PURSUIT_FLANK:DoPursuitFlankLogic(previous,chased,ai);return;
 case eLSS_PURSUIT_BUMP:DoPursuitBumpLogic(previous,chased,physics);return;
 case eLSS_PURSUIT_BLOCK:DoPursuitBlockLogic(previous,chased,ai);return;
 case eLSS_PURSUIT_GET_AHEAD:DoPursuitGetAheadLogic(previous,chased,ai);return;
 case eLSS_PURSUIT_ACQUIRE:DoPursuitAcquireLogic(previous,chased);return;
 case eLSS_PURSUIT_STOP:DoPursuitStopLogic(previous,chased);return;
 default:return;
 }
}
}

#include "game_core/port/race/ai_drive_target.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/trackroute.hpp"
namespace hp2::debug {
// Debug0022B268,E-0112-COP01. Strict20m XY proximity, no pursuit meansfalse.
bool tAICop::IsNearPursuitTarget(){const auto pursuit=m_.word(address_);if(!pursuit)return false;const auto chased=m_.word(pursuit+16),car=m_.word(address_+4);const float x=m_.scalar(chased+0x60)-m_.scalar(car+0x60),y=m_.scalar(chased+0x64)-m_.scalar(car+0x64);return std::sqrt(x*x+y*y)<20;}
// Debug0022ACA8,E-0112-COP01. Actual route virtual getters and source nearest
// traffic/racing route fallback; no host route interpolation or invented target.
void tAICop::InitializeDriveTarget(const ::port::race::AiDriveTargetServices& services){auto& m=m_;const auto a=address_,car=m.word(a+4);m.word(a+0x4c,0);m.word(a+0x48,0);const auto desired=m.word(a+0x10);if(desired!=5){if(desired==6){const int sub=m.signed_word(a+0x14);if(sub>6){std::uint32_t chosen;if(sub<9)chosen=m.word(m.word(a)+0x10);else if(sub==9)chosen=car;else return;m.word(a+0x48,hp2::recovered::Car(m,chosen).GetRoute(services));m.word(a+0x4c,hp2::recovered::Car(m,chosen).GetRoutePoint(services));}}return;}if(!services.routes)hp2::not_implemented("0x0022ACA8","source patrol nearest-route producer requires actual loaded routes");NearestRoutePointInfo nearest;const std::array<float,2> position{{m.scalar(car+0x60),m.scalar(car+0x64)}};bool found=GetNearestRoutePoint(m,*services.routes,nearest,position,ONCOMING_TRAFFIC_ROUTES,2,0);m.word(a+0x48,nearest.Route);m.word(a+0x4c,nearest.Point);if(found){const auto own=hp2::recovered::Car(m,car).GetRoutePoint(services);float lateral=(m.scalar(nearest.Point)-m.scalar(own))*m.scalar(own+0x10)-(m.scalar(nearest.Point+4)-m.scalar(own+4))*m.scalar(own+0xc);const float width=lateral<0?m.scalar(own+0x18):m.scalar(own+0x1c);if(lateral<0)lateral=-lateral;if(!(width<lateral))return;}GetNearestRoutePoint(m,*services.routes,nearest,position,ALL_RACING_ROUTES,2,0);m.word(a+0x48,nearest.Route);m.word(a+0x4c,nearest.Point);}
// Debug0022AE40,E-0112-COP01. Original20-point route walk/metadata and local W.
void tAICop::UpdateDriveTarget(const std::array<float,2>& position,bool first_ahead,const ::port::race::AiDriveTargetServices& services,std::optional<std::uint32_t> point_w){auto& m=m_;const auto a=address_,driver=m.word(m.word(a+4)+0x2c);if(!first_ahead){bool initialized=false;int count=0;do{if(count==10){initialized=true;InitializeDriveTarget(services);count=0;}auto point=m.word(a+0x4c);if(!point)hp2::not_implemented("0x0022AE40","source cop route target has no actual localized point");float forward=(position[0]-m.scalar(point))*m.scalar(point+0xc)+(position[1]-m.scalar(point+4))*m.scalar(point+0x10);while(0<forward){const bool more=count<10;++count;if(!more)break;NearestRoutePointInfo next{m.word(a+0x48),m.word(a+0x4c)};GetNextRoutePoint(m,next);if(next.Point){m.word(a+0x48,next.Route);m.word(a+0x4c,next.Point);}point=m.word(a+0x4c);forward=(position[0]-m.scalar(point))*m.scalar(point+0xc)+(position[1]-m.scalar(point+4))*m.scalar(point+0x10);} }while(count==10&&!initialized);}
 DriveTarget target(m,driver);target.Clear(0,0);NearestRoutePointInfo next{m.word(a+0x48),m.word(a+0x4c)};::port::driving::Memory::Temporary local(m,16);for(int i=0;;++i){if(!next.Route||!next.Point)hp2::not_implemented("0x0022AE40","source cop route walk dereferences absent route/point");float lateral=m.scalar(a+0x38);const int line=m.signed_word(a+0x58);if(line!=-1){if(line<0||line>=21)hp2::not_implemented("0x0022AE40","source cop driving line outside21 offsets");lateral=lateral+float(std::int32_t(m.signed_half(next.Point+0x30+2*unsigned(line)))*256)*1.52587890625e-5f;}if(!point_w)hp2::not_implemented("0x0022AE40","source local position_to_drive.pad requires independent explicit port operand");TrackRoutePoint(m,next.Point).GetPosition(local.address(),m.scalar(a+0x34),lateral,*point_w);const auto index=std::int32_t(next.Point-next.Route-0x428u)/0x70;target.AddPointToTarget(local.address(),std::int8_t(m.half(next.Route+0xa)),std::int16_t(index),0,false,0);GetNextRoutePoint(m,next);if(!next.Point||i+1>19)break;}if(!first_ahead){m.scalar(driver+16,position[0]);m.scalar(driver+20,position[1]);}}
}

#include "game_core/port/driving/ee_scalar.hpp"
namespace port::race::detail {void contact_promote(driving::Memory&,std::uint32_t,std::uint32_t,PhysicsConstructionServices&);void contact_apply(driving::Memory&,std::uint32_t,std::uint32_t,std::uint32_t);}
namespace port::race::cop_logic_detail {
static float distance(const driving::Memory& m,std::uint32_t a,std::uint32_t b){const float x=m.scalar(b+0x60)-m.scalar(a+0x60),y=m.scalar(b+0x64)-m.scalar(a+0x64);return std::sqrt(x*x+y*y);}
static std::uint32_t point(driving::Memory& m,std::uint32_t car,const AiDriveTargetServices* ai){const auto p=ai?hp2::recovered::Car(m,car).GetRoutePoint(*ai):hp2::recovered::Car(m,car).GetRoutePoint();if(!p)hp2::not_implemented("0x0022A5A0","source tactical pursuit requires actual localized route point");return p;}
static std::pair<float,unsigned> relative_polar(driving::Memory& m,std::uint32_t car,std::uint32_t chased){driving::Memory::Temporary p(m,8);hp2::recovered::bToPolar(m,p.address(),{{m.scalar(car+0x60)-m.scalar(chased+0x60),m.scalar(car+0x64)-m.scalar(chased+0x64)}});const int delta=std::int16_t(m.half(p.address()+4)-m.half(chased+0x3a4));return {m.scalar(p.address()),unsigned(std::abs(delta))&65535};}
static bool reached(const driving::Memory& m,std::uint32_t a,float tolerance){return std::fabs(m.scalar(a+0x2c)-m.scalar(a+0x34))<=tolerance&&std::fabs(m.scalar(a+0x30)-m.scalar(a+0x38))<=tolerance;}
}
namespace hp2::debug {
// Debug00229E98/00229E58,E-0112-COP01. Raw6x6 zone permission table.
int GetZoneFromSubState(int state,float side){switch(state){case 1:return 4;case 2:return side<0?3:5;case 5:case 6:case 8:return side<0?1:2;case 7:return 0;default:return 6;}}
int RequestZonePermission(::port::driving::Memory& m,int requested,int other){if(requested==6||other==6)return 1;if(requested<0||requested>=6||other<0||other>=6)hp2::not_implemented("0x00229E58","source zone permission outside6x6 table");return m.signed_word(0x313c10+4*unsigned(other*6+requested));}
// Debug0022A8C0,E-0112-COP01. Original route-width and relative-car-space tests.
float tAICop::GetBestFlankSide(std::uint32_t car,const ::port::race::AiDriveTargetServices* ai){auto& m=m_;if(ai)(void)hp2::recovered::Car(m,car).GetRoute(*ai);else(void)hp2::recovered::Car(m,car).GetRoute();const auto p=::port::race::cop_logic_detail::point(m,car,ai);const float lateral=(m.scalar(car+0x60)-m.scalar(p))*m.scalar(p+0x10)-(m.scalar(car+0x64)-m.scalar(p+4))*m.scalar(p+0xc);const bool left=-3<-m.scalar(p+0x18)-lateral,right=m.scalar(p+0x1c)-lateral<3;const auto own=m.word(address_+4);const auto local=hp2::recovered::Car(m,car).WorldToCar({{m.scalar(own+0x60),m.scalar(own+0x64)}});if(local[0]>=0){if(!right)return 1;}else if(!left)return -1;if(std::fabs(local[1])<2.5f)return 0;if(!right)return 1;if(left)return 0;return -1;}
// Debug00229648,E-0112-COP01. Actual World RNG draw only for primary timer0.
void tAICop::DoPursuitFollowBackwardsLogic(int,std::uint32_t chased,const ::port::race::AiDriveTargetServices* ai){auto& m=m_;const auto a=address_,p=m.word(a),car=m.word(a+4);const float length=::port::race::cop_logic_detail::distance(m,car,chased);if(m.word(p+0x1c)==a){if(length>=20){m.word(a+0x34,0);m.word(a+0x38,0);}else{m.word(a+0x34,m.word(0x328c68));if(m.scalar(a+0x28)==0){const float side=GetBestFlankSide(chased,ai);const auto world=m.word(0x3272f0);const auto choice=World(m,world).Random(3);m.scalar(a+0x28,1.25f);m.scalar(a+0x38,m.scalar(0x2b5070+4*choice)*side);}}}else{m.scalar(a+0x34,length<20?m.scalar(0x328c6c):0);m.scalar(a+0x38,2.5f);}if(std::fabs(m.scalar(chased+0x3c0)*m.scalar(0x326090))<5&&length<m.scalar(0x328c38))m.word(a+0x18,eLSS_PURSUIT_STOP);const auto route=::port::race::cop_logic_detail::point(m,chased,ai);if(1<m.scalar(chased+0x70)*m.scalar(route+0xc)+m.scalar(chased+0x74)*m.scalar(route+0x10))m.word(a+0x18,eLSS_PURSUIT_FOLLOW);}
// Debug00229820,E-0112-COP01. Strict timer/offset/collision/angle gates.
void tAICop::DoPursuitFollowOffsetLogic(int previous,std::uint32_t chased,const ::port::race::AiDriveTargetServices* ai){auto& m=m_;const auto a=address_;if(previous!=m.signed_word(a+0x14))m.scalar(a+0x28,2);m.word(a+0x34,m.word(0x328c68));m.scalar(a+0x38,m.scalar(a+0x44)*3);if(GetBestFlankSide(chased,ai)==m.scalar(a+0x44)){if(m.scalar(a+0x28)!=0||!::port::race::cop_logic_detail::reached(m,a,m.scalar(0x326094)))return;if(DidTheseCarsCollide(m,m.word(a+4),chased)){m.scalar(a+0x28,2);return;}const auto relative=::port::race::cop_logic_detail::relative_polar(m,m.word(a+4),chased);if(relative.second<0x6000||relative.first<=15){m.word(a+0x18,eLSS_PURSUIT_FLANK);return;}}m.word(a+0x18,eLSS_PURSUIT_FOLLOW);}
// Debug002299E0,E-0112-COP01. Preserve NPC/player/primary differences and RNG.
void tAICop::DoPursuitFlankLogic(int previous,std::uint32_t chased,const ::port::race::AiDriveTargetServices* ai){auto& m=m_;const auto a=address_,p=m.word(a);if(previous!=m.signed_word(a+0x14)){m.word(a+0x34,0);m.scalar(a+0x28,2.25f);m.scalar(a+0x38,m.scalar(a+0x44)*3);}if(GetBestFlankSide(chased,ai)!=m.scalar(a+0x44)){m.word(a+0x18,eLSS_PURSUIT_FOLLOW_OFFSET);return;}const auto relative=::port::race::cop_logic_detail::relative_polar(m,m.word(a+4),chased);if(relative.second<0x4000){if(!m.word(chased+0x18)){if(m.word(p+0x630)&4){const auto n=World(m,m.word(0x3272f0)).Random(15);m.word(a+0x18,n<=1?eLSS_PURSUIT_GET_AHEAD:eLSS_PURSUIT_BUMP);}}else{int desired=eLSS_PURSUIT_GET_AHEAD;if(m.word(p+0x630)&4){const auto n=World(m,m.word(0x3272f0)).Random(15);if(n>1||m.word(p+0x1c)!=a)desired=eLSS_PURSUIT_BUMP;}m.word(a+0x18,desired);}}if(m.scalar(a+0x28)!=0||!::port::race::cop_logic_detail::reached(m,a,m.scalar(0x326098)))return;m.scalar(a+0x28,2.25f);if(relative.second>=0x6000)m.word(a+0x18,eLSS_PURSUIT_FOLLOW_OFFSET);}
// Debug00229BE0,E-0112-COP01. Source collision promotion and real impulse;
// only the unwritten arm W is an explicit PC research operand.
void tAICop::DoPursuitBumpLogic(int previous,std::uint32_t chased,::port::race::PhysicsConstructionServices* physics){auto& m=m_;const auto a=address_,car=m.word(a+4);const int difficulty=m.word(chased+0x18)?m.signed_word(m.word(0x328c1c)+0x28ec):2;if(difficulty<0||difficulty>2)hp2::not_implemented("0x00229BE0","source bump difficulty outside three-entry table");if(previous!=m.signed_word(a+0x14))m.scalar(a+0x28,1.75f);const float timer=m.scalar(a+0x28),distance=std::fabs(timer-.875f);m.word(a+0x34,0);m.scalar(a+0x40,100);m.scalar(a+0x3c,100);m.scalar(a+0x38,((distance+distance)/1.75f)*m.scalar(a+0x44)*3);if(timer!=0){if(m.word(0x311f78+0x288)==2)return;if(!DidTheseCarsCollide(m,car,chased))return;if(timer<.4375f){auto impulse=m.vector(chased+0x60),own=m.vector(car+0x60);for(unsigned i=0;i<4;++i)impulse[i]=impulse[i]-own[i];const float squared=(impulse[0]*impulse[0]+impulse[1]*impulse[1])+impulse[2]*impulse[2];const float inverse=squared==0?::port::driving::ee_vrsqrt_one_zero(squared,"0x00229D00"):1/std::sqrt(squared);for(auto& lane:impulse){lane=lane*inverse;lane=lane*m.scalar(0x2b50c0+4*unsigned(difficulty));}if(m.word(chased+0x24)!=3){const auto body=m.word(chased+0x34);::port::driving::Memory::Temporary local(m,32);m.vector(local.address(),impulse);for(unsigned i=0;i<3;++i)m.word(local.address()+16+4*i,0);if(!physics||!physics->cop_bump_arm_w)hp2::not_implemented("0x00229D40","source bump arm.pad requires independent explicit port operand");m.word(local.address()+28,*physics->cop_bump_arm_w);::port::race::detail::contact_promote(m,body,0,*physics);::port::race::detail::contact_apply(m,body,local.address(),local.address()+16);}}}m.word(a+0x18,eLSS_PURSUIT_FLANK);}
// Debug0022A1B0,E-0112-COP01. Source low-speed and capture thresholds.
void tAICop::DoLowSpeedClosePursuitFollowLogic(int previous,std::uint32_t chased){auto& m=m_;const auto a=address_;if(previous!=m.signed_word(a+0x14)){m.word(a+0x38,0);m.scalar(a+0x34,6);}const float speed=m.scalar(chased+0x3c0)*m.scalar(0x3260a0);if(20<std::fabs(speed))m.word(a+0x18,eLSS_PURSUIT_FOLLOW);if(std::fabs(speed)<5&&::port::race::cop_logic_detail::distance(m,m.word(a+4),chased)<m.scalar(0x328c38))m.word(a+0x18,eLSS_PURSUIT_STOP);}
// Debug0022A268,E-0112-COP01. Original path direction/slow capture precedence.
void tAICop::DoPursuitBlockLogic(int previous,std::uint32_t chased,const ::port::race::AiDriveTargetServices* ai){auto& m=m_;const auto a=address_,car=m.word(a+4);if(previous!=m.signed_word(a+0x14)){m.word(a+0x34,0);m.word(a+0x58,0xffffffff);}m.word(a+0x34,0);m.scalar(a+0x40,100);m.scalar(a+0x3c,100);m.word(a+0x38,0);const float length=::port::race::cop_logic_detail::distance(m,car,chased),distance=hp2::debug::TrackPathManager::CompareDistances(m.scalar(0x313308),m.scalar(car+0x7f4),m.scalar(chased+0x7f4));float conversion=m.scalar(0x3260a8);if(distance<0)m.word(a+0x18,eLSS_PURSUIT_FOLLOW);else{const auto p=::port::race::cop_logic_detail::point(m,chased,ai);if(std::fabs(m.scalar(chased+0x3c4)*m.scalar(0x3260a4))<=20)conversion=m.scalar(0x3260a4);else if(m.scalar(chased+0x70)*m.scalar(p+0xc)+m.scalar(chased+0x74)*m.scalar(p+0x10)<0&&20<length)m.word(a+0x18,eLSS_PURSUIT_FOLLOW_BACKWARDS);}if(std::fabs(m.scalar(chased+0x3c0)*conversion)<20&&m.scalar(0x328c38)<length)m.word(a+0x18,eLSS_PURSUIT_LOW_SPEED_CLOSE_FOLLOW);}
// Debug0022A3F8/0022A470/0022A528,E-0112-COP01.
void tAICop::DoPursuitAcquireLogic(int previous,std::uint32_t chased){auto& m=m_;const auto a=address_;if(previous!=m.signed_word(a+0x14)){m.word(a+0x58,0);m.word(a+0x34,0);m.word(a+0x38,0);}if(-180<hp2::debug::TrackPathManager::CompareDistances(m.scalar(0x313308),m.scalar(m.word(a+4)+0x7f4),m.scalar(chased+0x7f4)))m.word(a+0x18,eLSS_PURSUIT_FOLLOW);}
void tAICop::DoPursuitStopLogic(int previous,std::uint32_t chased){auto& m=m_;const auto a=address_;if(previous!=m.signed_word(a+0x14)){m.word(a+0x34,0);m.scalar(a+0x28,.5f);m.word(a+0x38,0);}if(m.scalar(a+0x28)<=0&&(5<std::fabs(m.scalar(chased+0x3c0)*m.scalar(0x3260ac))||m.scalar(0x328c38)<::port::race::cop_logic_detail::distance(m,m.word(a+4),chased)))m.word(a+0x18,eLSS_PURSUIT_LOW_SPEED_CLOSE_FOLLOW);}
void tAICop::DoPursuitGetAheadLogic(int previous,std::uint32_t,const ::port::race::AiDriveTargetServices* ai){auto& m=m_;const auto a=address_;if(previous==m.signed_word(a+0x14))return;m.word(a+0x34,0);const auto car=m.word(a+4),p=::port::race::cop_logic_detail::point(m,car,ai);m.word(a+0x58,0xffffffff);m.scalar(a+0x38,(m.scalar(car+0x60)-m.scalar(p))*m.scalar(p+0x10)-(m.scalar(car+0x64)-m.scalar(p+4))*m.scalar(p+0xc));}
}

namespace hp2::debug {
// Debug00229208,E-0112-COP01. Distinct original local velocity padding.
void tAICop::InitRoadBlockLogic(::port::race::PhysicsConstructionServices* physics){auto& m=m_;const auto a=address_,car=m.word(a+4);m.word(a+0x14,0);m.word(car+0x86c,0);m.word(car+0x3c0,0);m.word(m.word(car+0x28)+0x24,1);if(!physics||!physics->cop_roadblock_velocity_w)hp2::not_implemented("0x00229208","source roadblock zero-velocity.pad requires independent explicit port operand");for(unsigned i=0;i<3;++i)m.word(car+0x70+4*i,0);m.word(car+0x7c,*physics->cop_roadblock_velocity_w);m.word(car+0x3c4,0);m.word(car+0x834,1);m.word(car+0x440,0);m.word(car+0x808,0);}
// Debug00229280,E-0112-COP01. Actual per-cop roadblock position/angle.
void tAICop::DoRoadBlockLogic(::port::race::PhysicsConstructionServices* physics){BuildStationaryDriveTarget();auto& m=m_;const auto a=address_,p=m.word(a);if(!p)return;const auto block=m.word(p+0x14);if(!block)return;const int index=m.signed_word(a+0x24);if(index<0||index>=2)hp2::not_implemented("0x00229280","source roadblock cop index outside two positions");const auto pos=block+8+8*unsigned(index),car=m.word(a+4);const float x=m.scalar(pos)-m.scalar(car+0x60),y=m.scalar(pos+4)-m.scalar(car+0x64);const auto angle=m.half(block+0x18+2*unsigned(index));const int difference=std::int16_t(angle-m.half(car+0x3a4));if((m.scalar(0x32608c)<std::sqrt(x*x+y*y)||100<std::fabs(float(difference)))&&CanTeleport(m,car,{{m.scalar(pos),m.scalar(pos+4)}}))Teleport(pos,angle,physics,physics?physics->car_reset:nullptr);}
// Debug002293B0,E-0112-COP01. Source candidate velocity-direction predicate.
bool tAICop::CanPatrollerSeeThisCar(std::uint32_t car){const auto own=m_.word(address_+4);const float x=m_.scalar(car+0x60)-m_.scalar(own+0x60),y=m_.scalar(car+0x64)-m_.scalar(own+0x64);if(!(std::sqrt(x*x+y*y)<50))return false;const float vx=m_.scalar(car+0x70),vy=m_.scalar(car+0x74),length=std::sqrt(vx*vx+vy*vy);if(!(0<length))return false;const float inverse=1/length;return 20<=(vx*inverse)*x+(vy*inverse)*y;}
// Debug00229498,E-0112-COP01. Original target rebuild/250m/45m admission.
void tAICop::DoDrivingOnPatrolLogic(::port::race::PhysicsConstructionServices* physics){auto& m=m_;const auto a=address_,car=m.word(a+4),world=m.word(0x3272f0);m.word(a+0x34,0);m.word(a+0x30,0);m.word(a+0x2c,0);m.word(a+0x38,0);const auto ai=physics&&physics->car_reset?physics->car_reset->ai:nullptr;if(!ai)hp2::not_implemented("0x00229498","source patrol target requires actual route/local stack services");UpdateDriveTarget({{m.scalar(car+0x60),m.scalar(car+0x64)}},false,*ai,physics->cop_target_point_w);if(250<=World(m,world).GetDistanceToClosestPlayer(std::array<float,2>{{m.scalar(car+0x60),m.scalar(car+0x64)}})){tCop(m,a).SetDesiredLogicState(1);return;}const int count=m.signed_word(world+0x24);for(int i=0;i<count;++i){const auto candidate=m.word(world+0x2c+4*unsigned(i));if(::port::race::cop_logic_detail::distance(m,car,candidate)<45&&std::fabs(m.scalar(candidate+0x68)-m.scalar(car+0x68))<2.5f){const bool visible=CanPatrollerSeeThisCar(candidate);if(IsValidPursuitTarget(candidate,visible)&&tCopManager(m,m.word(0x328c1c)).RequestPursuit(a,candidate,physics))tCop(m,a).SetDesiredLogicState(6);}}}
// Debug0022AA38,E-0112-COP01. Waiting timer and fresh pursuit admission continue
// even after a limbo/patrol desired-state request within this source tick.
void tAICop::DoCaughtSpeederLogic(::port::race::PhysicsConstructionServices* physics){BuildStationaryDriveTarget();auto& m=m_;const auto a=address_,car=m.word(a+4),world=m.word(0x3272f0);if(m.word(a+0x14)!=14)return;if(250<=World(m,world).GetDistanceToClosestPlayer(std::array<float,2>{{m.scalar(car+0x60),m.scalar(car+0x64)}}))tCop(m,a).SetDesiredLogicState(1);else if(m.scalar(a+0x28)==0)tCop(m,a).SetDesiredLogicState(5);for(int i=0;i<m.signed_word(world+0x24);++i){const auto candidate=m.word(world+0x2c+4*unsigned(i));if(::port::race::cop_logic_detail::distance(m,car,candidate)<45&&std::fabs(m.scalar(candidate+0x68)-m.scalar(car+0x68))<2.5f&&IsValidPursuitTarget(candidate,true)&&tCopManager(m,m.word(0x328c1c)).RequestPursuit(a,candidate,physics))tCop(m,a).SetDesiredLogicState(6);}}
// Debug0022AC38/0022B260,E-0112-COP01.
void tAICop::DoDisabledLogic(){BuildStationaryDriveTarget();const auto car=m_.word(address_+4);if(250<=World(m_,m_.word(0x3272f0)).GetDistanceToClosestPlayer(std::array<float,2>{{m_.scalar(car+0x60),m_.scalar(car+0x64)}}))tCop(m_,address_).SetDesiredLogicState(1);}
void tAICop::DoGuardLocationLogic(){BuildStationaryDriveTarget();}
}
