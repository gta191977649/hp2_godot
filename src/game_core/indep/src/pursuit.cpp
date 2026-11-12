// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/pursuit.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/speech/conversions.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/src/coputil.hpp"
#include "game_core/indep/inc/CopChase.hpp"
#include <cmath>

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/runtime_warnings.hpp"

namespace hp2::debug {
// Debug00226DC8,E-0100-PA01. Source active-slot compaction and roadblock
// pointer unlink; never substitutes an empty pursuit or alters the other cop.
bool tPursuit::RemoveCopFromPursuit(std::uint32_t cop){if(!cop)return false;auto& m=m_;const auto block=m.word(address_+0x14);bool removed=false;int count=m.signed_word(address_+0x5c4);if(count>2)hp2::not_implemented("0x00226DC8","original pursuit count exceeds two-pointer cop array");for(int i=0;i<count&&!removed;++i)if(m.word(address_+0x1c+4*unsigned(i))==cop){m.word(address_+0x1c+4*unsigned(i),0);m.word(address_+0x5c4,std::uint32_t(--count));if(!m.word(address_+0x1c)){m.word(address_+0x1c,m.word(address_+0x20));m.word(address_+0x20,0);}removed=true;}if(block&&!removed)for(unsigned i=0;i<2&&!removed;++i)if(m.word(block+4*i)==cop){m.word(block+4*i,0);removed=true;}if(removed)tCop(m,cop).SetPursuit(0);return removed;}
// Debug00226358/00225260/00225230/00225318/00224A60,E-0100-PA01.
bool tPursuit::IsPlayerInvolved()const{return m_.word(m_.word(address_+0x10)+0x18)||m_.word(0x3121ec);}
std::uint32_t tPursuit::GetExactVehicleDescription(){auto& m=m_;const auto car=m.word(address_+0x10),ride=m.word(car+0x10);const int type=m.signed_word(ride);if(type<0||type>=67||!m.word(0x3278e0))hp2::not_implemented("0x00225260","actual original CarTypeInfo row required by pursuit speech description");if(m.word(m.word(0x3278e0)+unsigned(type)*0x560+0x538)==1)return 0x10000000u;if((type>=15&&type<=16)||type==22)return 0xffffffffu;return std::uint32_t(Speech::ConvertCarTypeToDescription(m,type));}
std::uint32_t tPursuit::GetChasedCarColour(){auto& m=m_;const auto ride=m.word(m.word(address_+0x10)+0x10);const auto skin=::port::race::GetSkinInfo(m,m.signed_word(ride),m.signed_byte(ride+5),1);if(!skin)hp2::not_implemented("0x00225230","actual authored skin row required by pursuit speech colour");return m.word(skin+0x28);}
std::uint32_t tPursuit::GetCopSpeaker(){auto& m=m_;const auto primary=m.word(address_+0x5d8);if(primary!=5)return primary;const auto block=m.word(address_+0x14);return block?m.word(block+0x74):tCopManager(m,m.word(0x328c1c)).GetCurrentCopSpeaker();}
float tPursuit::GetSpeedMPHOrKPH(){auto& m=m_;const auto car=m.word(address_+0x10);return m.scalar(car+0x3c0)*m.scalar(m.word(car+0x18)&&m.byte(0x312265)?0x326004:0x326000);}
// Debug002256E0,E-0100-PA01. Speech-disabled mode retains state escalation.
void tPursuit::BeginPursuitCollisionSpeech(std::uint32_t other,bool disabled){auto& m=m_;const bool cop=m.word(m.word(other+0x14)+8)==4;(void)GetCopSpeaker();(void)m.word(address_+0x5e0);if(!disabled)hp2::not_implemented(cop?"0x00205300":"0x00205460","original collision pursuit speech event submission required");::port::diagnostics::warn(cop?"0x00205300":"0x00205460","collision pursuit speech emission disabled by explicit PC presentation policy","retaining source collision classification and pursuit escalation before continuing");if(cop)SwitchPursuitState(2,disabled);else if(m.word(address_+0x5cc)==0)SwitchPursuitState(1,disabled);}
// Debug00225778,E-0100-PA01. Optional event emission is isolated only after
// source argument/selection/RNG/state work; no fabricated speaking predicate.
void tPursuit::StartPursuitSpeech(bool disabled){
 auto& m=m_;const auto exact=GetExactVehicleDescription();const auto manager=m.word(0x328c1c);auto emit=[&](const char* address){if(!disabled)hp2::not_implemented(address,"original pursuit speech event submission required");::port::diagnostics::warn(address,"pursuit speech emission disabled by explicit PC presentation policy","retaining source selectors, arguments, shared RNG, speaker rotation and pursuit state; not submitting speech event");};
 if(!IsPlayerInvolved()){const auto coordinator=m.word(0x327290);if(!coordinator)hp2::not_implemented("0x00225778","actual source RaceCoordinator required for nonplayer pursuit speech");const auto state=m.word(coordinator);if(state==8||state==9)return;if(exact==0xffffffffu){(void)GetCopSpeaker();(void)GetChasedCarColour();emit("0x002054B0");}else{(void)tCopManager(m,manager).GetCurrentCopSpeaker();(void)GetChasedCarColour();emit("0x00205510");}tCopManager(m,manager).NextCopSpeaker();return;}
 if(m.word(0x3121ec)){(void)GetCopSpeaker();(void)GetChasedCarColour();emit(exact==0xffffffffu?"0x002054B0":"0x00205510");return;}
 if(const auto other=CarCausedRecentCollision(m,m.word(address_+0x10))){BeginPursuitCollisionSpeech(other,disabled);m.word(address_+0x5d0,1);m.word(address_+0x5e8,1);return;}
 if(tCopManager(m,manager).GetNumTickets(m.word(address_+0x10))>0){SwitchPursuitState(1,disabled);if(hp2::recovered::bRandom(m,10)<8&&exact!=0xffffffffu){(void)GetCopSpeaker();(void)GetChasedCarColour();emit("0x00205580");return;}}
 if(100.f<GetSpeedMPHOrKPH()&&hp2::recovered::bRandom(m,10)<7){(void)GetCopSpeaker();(void)GetSpeedMPHOrKPH();emit("0x002056C0");return;}
 const auto selection=hp2::recovered::bRandom(m,2);(void)GetCopSpeaker();emit(selection==0?"0x00205390":"0x00205350");
}
// Debug00225A30,E-0100-PA01. Source speaker reservations/rotation and fullword
// bAlreadyJoined publication remain active under optional speech suppression.
void tPursuit::JoinPursuitSpeech(bool disabled){auto& m=m_;if(!m.word(m.word(address_+0x10)+0x18))return;const auto manager=m.word(0x328c1c);if(m.word(address_+0x5d8)==5){m.word(address_+0x5d8,tCopManager(m,manager).ReserveCopSpeaker());const bool active=std::fabs(m.scalar(manager+0x286c))<=m.scalar(0x326008);(void)active;if(!disabled)hp2::not_implemented("0x00205B10","original suspect-found speech event submission required");::port::diagnostics::warn("0x00205B10","suspect-found speech emission disabled by explicit PC presentation policy","retaining original speaker reservation, primary speaker store and supervisor predicate");}
 else if(!m.word(address_+0x5e4)&&m.signed_word(address_+0x5cc)>0){m.word(address_+0x5e4,1);if(m.word(address_+0x5dc)==5&&m.word(0x311fa8)==1)m.word(address_+0x5dc,tCopManager(m,manager).ReserveCopSpeaker());else{(void)tCopManager(m,manager).GetCurrentCopSpeaker();tCopManager(m,manager).NextCopSpeaker();}(void)GetCopSpeaker();if(!disabled)hp2::not_implemented("0x00205760","original backup-confirm speech event submission required");::port::diagnostics::warn("0x00205760","backup-confirm speech emission disabled by explicit PC presentation policy","retaining original AlreadyJoined flag and speaker reservations/rotation");}}
// Debug00226FA8, E-0099-W107; exact original pursuit.cpp definition owner.
bool tPursuit::HasThisCop(std::uint32_t cop)const{for(int i=0;i<m_.signed_word(address_+0x5c4);++i)if(m_.word(address_+0x1c+4*std::uint32_t(i))==cop)return true;if(const auto block=m_.word(address_+0x14))for(unsigned i=0;i<2;++i)if(m_.word(block+4*i)==cop)return true;return false;}
}

namespace hp2::recovered {
// ELF 0x0022D080; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0022d080.md.
void tNearFollowTarget::GetVelocity() {
    hp2::not_implemented("0x0022D080", "tNearFollowTarget::GetVelocity (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00229ED0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00229ed0.md.
void tPursuit::SelectPrimaryCop() {
    hp2::not_implemented("0x00229ED0", "tPursuit::SelectPrimaryCop (signature and full body unresolved)");
}

// ELF 0x0022BB28; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0022bb28.md.
void tPursuit::IsCopNearChasedCar() {
    hp2::not_implemented("0x0022BB28", "tPursuit::IsCopNearChasedCar (signature and full body unresolved)");
}

// ELF 0x0022BC08; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0022bc08.md.
void tPursuit::IsRacingWithMe() {
    hp2::not_implemented("0x0022BC08", "tPursuit::IsRacingWithMe (signature and full body unresolved)");
}

// ELF 0x0022C920; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0022c920.md.
void tPursuit::HasThisCop() {
    hp2::not_implemented("0x0022C920", "tPursuit::HasThisCop (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/barricade.hpp"
#include "game_core/indep/src/coputil.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/inc/Replay.hpp"
#include "game_core/Chopper.hpp"
#include <cmath>
namespace hp2::debug {
// E-0099-W119, Debug00227568/002275F0; original pursuit.cpp.
void tNearFollowTarget::Sub_00227568(){m_.word(address_+0x16c,0x2b4f38);for(unsigned i=0;i<30;++i){m_.word(address_+4+8*i,0);m_.word(address_+8+8*i,0);m_.word(address_+0xf4+4*i,0);}m_.word(address_,0);}
void tNearFollowTarget::Sub_002275F0(int flags){m_.word(address_+0x16c,0x2b4f38);if(flags&1)m_.release(address_);}
// Debug00227808,E-0099-W119. Count is signed data1/memory4, all30 XY/time rows.
void tNearFollowTarget::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(m_,snapshot);s.MoveData(address_,1,4,1);for(unsigned i=0;i<30;++i){s.MoveData(address_+4+8*i,8);s.MoveData(address_+0xf4+4*i,4);}}
// Debug00227288/002272A0,E-0099-W119. Timer is genuinely not initialized here.
void tRoadBlockStateMachine::Sub_00227288(std::uint32_t pursuit){m_.word(address_+0x10,pursuit);m_.word(address_,0);m_.word(address_+8,0);m_.word(address_+0xc,0);}
void tRoadBlockStateMachine::Sub_002272A0(int flags){if(flags&1)m_.release(address_);}
// Debug002274E8,E-0099-W119. Raw enum byte is not a fullword bool; owner is rebound.
void tRoadBlockStateMachine::DoSnapshot(std::uint32_t snapshot,std::uint32_t pursuit){ReplaySnapshot s(m_,snapshot);s.MoveData(address_,1);s.MoveData(address_+4,4);s.MoveData(address_+8,4,4,1);s.MoveData(address_+0xc,4,4,1);m_.word(address_+0x10,pursuit);}
// Debug002247D0,E-0099-W119. Real race laps/Point2Point and loaded TrackLength.
float tPursuit::ScaleTimeByRaceLength(float t)const{const float laps=m_.word(0x311fa0)?1.f:static_cast<float>(m_.signed_word(0x311fa4));float distance=laps*m_.scalar(0x313308);if(!std::isfinite(t)||!std::isfinite(distance))hp2::not_implemented("0x002247D0","original pursuit timer finite FPU operands are unresolved");if(m_.scalar(0x325ff0)<distance)distance=m_.scalar(0x325ff0);if(distance<m_.scalar(0x325ff4))distance=m_.scalar(0x325ff4);return t*(distance*m_.scalar(0x325ff8)+.25f);}
// Debug00224848,E-0099-W119. Reached supervisor/give-up positive children guarded.
void tPursuit::SwitchPursuitState(int state,bool disable_speech){auto& m=m_;const auto a=address_;if(state<0||state>5)hp2::not_implemented("0x00224848","original pursuit state table index outside recovered six states");m.word(a+0x5cc,static_cast<std::uint32_t>(state));m.scalar(a+0x5f4,ScaleTimeByRaceLength(m.scalar(0x313bb8+4*static_cast<unsigned>(state))));switch(state){case 0:m.word(a+0x5d4,1);m.word(a+0x5d0,0);m.word(a+0x630,m.word(a+0x630)|2u);break;case 1:m.word(a+0x5d0,3);m.word(a+0x5ec,1);m.word(a+0x5d4,2);m.word(a+0x630,m.word(a+0x630)|3u);break;case 2:m.word(a+0x5ec,1);m.word(a+0x5d4,3);m.word(a+0x630,m.word(a+0x630)|7u);m.word(a+0x5d0,4);tCopManager(m,m.word(0x328c1c)).PossibleCallSupervisor(a,disable_speech);break;case 3:m.word(a+0x5ec,1);m.word(a+0x5d4,4);m.word(a+0x630,m.word(a+0x630)|0x20fu);m.word(a+0x5d0,6);tCopManager(m,m.word(0x328c1c)).PossibleCallSupervisor(a,disable_speech);break;case 4:m.word(a+0x5d4,0);break;case 5:{m.word(a+0x5d4,0);if(IsPlayerInvolved())CopBackOffSpeech(disable_speech);const auto index=m.signed_half(m.word(a+0x10)+8);if(index<0||index>=8)hp2::not_implemented("0x00224848","source car timer index outside eight entries");const auto manager=m.word(0x328c1c);m.scalar(manager+0x2874+4*unsigned(index),16);m.word(manager+0x2894+4*unsigned(index),m.word(0x325ffc));ReleaseAllCops();break;}}}
// Debug00223850,E-0099-W119. Original embedded constructors and owned Barricade.
void tPursuit::Sub_00223850(std::int8_t id,std::uint32_t car){auto& m=m_;const auto a=address_;m.word(a+0x634,0x2b4f28);m.word(a+0x10,car);DriveTarget(m,a+0x30).Sub_00216528();DriveTarget(m,a+0x240).Sub_00216528();tNearFollowTarget(m,a+0x450).Sub_00227568();tRoadBlockStateMachine(m,a+0x614).Sub_00227288(a);m.byte(a+0x5c8,static_cast<std::uint8_t>(id));for(unsigned off:{0x5c4u,0x5d4u,0x62cu})m.word(a+off,0);const auto manager=m.word(0x328c1c);if(!manager)hp2::not_implemented("0x00223850","original pursuit requires actual CopManager owner");const int difficulty=m.signed_word(manager+0x28ec);if(difficulty<0||difficulty>2)hp2::not_implemented("0x00223850","original pursuit spawn table difficulty outside authored three rows");m.scalar(a+0x5f0,2);m.scalar(a+0x5f8,3.5f);m.word(0x328c30,m.word(0x2b4e90+4*static_cast<unsigned>(difficulty)));m.word(a+0x5fc,0);m.scalar(a+0x600,10);m.scalar(a+0x604,12);m.word(a+0x5dc,5);for(unsigned off:{0x20u,0x1cu,0x5e0u,0x5e4u,0x5e8u,0x5ecu,0x14u,0x630u,0x18u,0x628u})m.word(a+off,0);if(car){m.scalar(a+0x608,GetTopSpeed(m,car));if(m.word(0x3121ec))m.word(a+0x5d8,m.word(manager+0x2860));else if(m.word(car+0x18))m.word(a+0x5d8,tCopManager(m,manager).ReserveCopSpeaker());}const auto coordinator=m.word(0x327290);if(!coordinator)hp2::not_implemented("0x00223850","original pursuit requires actual RaceCoordinator owner");SwitchPursuitState(m.word(coordinator)==8?3:0);m.word(a+0x610,0);m.scalar(a+0x60c,1);const auto barricade=m.allocate(0x40);tBarricadeManager(m,barricade).Sub_00248418();m.word(a+0x5c0,barricade);}
// Debug00226C20,E-0099-W119. Actual null-deployment return; positive graph guarded.
void tPursuit::DisbandRoadBlock(bool){if(m_.word(address_+0x14))hp2::not_implemented("0x00226C20","actual active roadblock cop/spike/barricade disband dependencies are required");}
// Debug00224B50,E-0099-W119. Existing source Chopper.FollowThisCar is reused.
void tPursuit::ReleaseChopper(){if(const auto chopper=m_.word(address_+0x18)){m_.word(m_.word(0x328c1c)+0x138,0);Chopper(m_,chopper).FollowThisCar(0);m_.word(address_+0x628,0);m_.word(address_+0x18,0);}}
// Debug00224AA0,E-0099-W119/E-0100-PA01. Preserve source for-loop count
// reload after compaction, including its two-slot retained-cop counterexample.
// Actual IsAIControlled descriptor delta/target are bound; unknown targets guard.
void tPursuit::ReleaseAllCops(){auto& m=m_;const auto a=address_;int count=m.signed_word(a+0x5c4);for(int i=0;i<count;++i){if(i>=2)hp2::not_implemented("0x00224AA0","original pursuit cop count exceeds owned two-pointer array");if(const auto cop=m.word(a+0x1c+4*unsigned(i))){RemoveCopFromPursuit(cop);const auto table=m.word(cop+0x1c);if(!table)hp2::not_implemented("0x00224AE8","actual cop virtual table required after source pursuit unlink");const auto self=cop+std::int32_t(m.signed_half(table+0x10)),target=m.word(table+0x14);bool ai;if(target==0x22b380)ai=tAICop(m,self).IsAIControlled();else if(target==0x22b3b8)ai=tPlayerCop(m,self).IsAIControlled();else hp2::not_implemented("0x00224AF4","unrecovered actual cop IsAIControlled virtual target");if(ai)tCop(m,cop).SetDesiredLogicState(5);count=m.signed_word(a+0x5c4);}}DisbandRoadBlock(false);ReleaseChopper();}
// Debug00223A18,E-0099-W119. Source child order and deletion flag. bTNode227890
// tail-calls bNode115280(flags0), whose proven branch has no stores/calls.
void tPursuit::Sub_00223A18(int flags){auto& m=m_;const auto a=address_;m.word(a+0x634,0x2b4f28);ReleaseAllCops();const auto car=m.word(a+0x10);if(!car)hp2::not_implemented("0x00223A18","original pursuit destructor requires restored chased Car before player access");if(m.word(car+0x18)){tCopManager manager(m,m.word(0x328c1c));manager.ReleaseCopSpeaker(m.word(a+0x5d8));if(m.word(a+0x5dc)!=5)manager.ReleaseCopSpeaker(m.word(a+0x5dc));}const auto barricade=m.word(a+0x5c0);m.byte(a+0x5c8,0xfe);if(barricade){const auto table=m.word(barricade+0x3c),target=m.word(table+0xc),self=barricade+static_cast<std::int32_t>(m.signed_half(table+8));if(target!=0x248468)hp2::not_implemented("0x00223A18","actual barricade destructor virtual target is not reconstructed");tBarricadeManager(m,self).Sub_00248468(3);}tRoadBlockStateMachine(m,a+0x614).Sub_002272A0(2);tNearFollowTarget(m,a+0x450).Sub_002275F0(2);DriveTarget(m,a+0x240).Sub_00216598(2);DriveTarget(m,a+0x30).Sub_00216598(2);if(flags&1)Sub_002279C8();}
// Debug00224D78,E-0099-W119. Source widths/order, borrowed pointer codecs and
// actual embedded children. No substituted empty pursuit or guessed fresh lanes.
void tPursuit::DoSnapshot(std::uint32_t snapshot){auto& m=m_;const auto a=address_,manager=m.word(0x328c1c),world=m.word(0x3272f0);if(!manager||!world)hp2::not_implemented("0x00224D78","actual pursuit snapshot World/CopManager owners required");ReplaySnapshot s(m,snapshot);s.BeginSection(a,0x2b4f08);s.VerifyData(a+0x5c8,1);World(m,world).DoPointerSnapshot(snapshot,a+0x10);tCopManager cops(m,manager);cops.DoPointerSnapshot(snapshot,::port::race::SnapshotRoadBlockPointer{a+0x14});if(const auto block=m.word(a+0x14)){s.MoveData(block+0x74,4);for(unsigned i=0;i<2;++i)cops.DoPointerSnapshot(snapshot,::port::race::SnapshotCopPointer{block+4*i});}s.MoveData(a+0x5d8,4);s.MoveData(a+0x5dc,4);for(unsigned off:{8u,0xcu,0x62cu})s.MoveData(a+off,4,4,1);::port::driving::Memory::Temporary chopper_flag(m,4);m.word(chopper_flag.address(),m.word(a+0x18)!=0);s.MoveData(chopper_flag.address(),4,4,1);if(m.byte(snapshot+0x3d))m.word(a+0x18,m.word(chopper_flag.address())?m.word(world+0x18):0);s.MoveData(a+0x600,4);s.MoveData(a+0x604,4);s.MoveData(a+0x628,1);s.MoveData(a+0x5c4,4,4,1);for(int i=0;i<m.signed_word(a+0x5c4);++i){if(i>=2)hp2::not_implemented("0x00224D78","original pursuit cop count exceeds recovered two-pointer array");cops.DoPointerSnapshot(snapshot,::port::race::SnapshotCopPointer{a+0x1c+4*static_cast<unsigned>(i)});}for(unsigned off:{0x5ccu,0x5d0u,0x5e0u})s.MoveData(a+off,1);for(unsigned off:{0x5f4u,0x5fcu,0x5f8u,0x5f0u,0x60cu,0x610u})s.MoveData(a+off,4);for(unsigned off:{0x5e4u,0x5e8u,0x5ecu})s.MoveData(a+off,1,4,0);s.MoveData(a+0x608,4);s.MoveData(a+0x630,4);s.MoveData(a+0x5d4,1,4,1);DriveTarget(m,a+0x30).DoSnapshot(snapshot);DriveTarget(m,a+0x240).DoSnapshot(snapshot);const auto chased=m.word(a+0x10);if(!chased)hp2::not_implemented("0x00224D78","original pursuit near-follow condition requires a restored chased Car");if(m.word(chased+0x18))tNearFollowTarget(m,a+0x450).DoSnapshot(snapshot);tRoadBlockStateMachine(m,a+0x614).DoSnapshot(snapshot,a);tBarricadeManager(m,m.word(a+0x5c0)).DoSnapshot(snapshot);s.EndSection();}
}

// TASK-0100 E-0100-PU02. Original pursuit.cpp mandatory ordinary chase tick.
// Authority SHA815b60c7...332f1b; read-only listing/ABI audit in PU01.
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/raceparameters.hpp"
namespace hp2::debug {
namespace {
void port_pursuit_emission(bool disabled,const char* at){
 if(!disabled)hp2::not_implemented(at,"original pursuit speech event submission required");
 ::port::diagnostics::warn(at,"pursuit speech emission disabled by explicit PC presentation policy","retaining source selector, arguments, RNG and pursuit state before omitted event submission");
}
// Original227618 hidden-return ABI: a0 output, a1 self,a2 position,a3 velocity.
// The output is input position unchanged; caller discards it. Preserve integer
// history and latency-loop operations, not the decompiler's float-count fiction.
}
std::array<float,2> tNearFollowTarget::Update(std::uint32_t position,std::uint32_t velocity,float dt){
 auto& m=m_;const auto a=address_;
 int count=m.signed_word(a);
 if(count<0||count>30)hp2::not_implemented("0x00227618","original near-follow history count outside thirty slots");
 for(int i=28;i>=0;--i){m.copy(a+4+8*(i+1),a+4+8*i,8);m.word(a+0xf4+4*(i+1),m.word(a+0xf4+4*i));}
 m.copy(a+4,velocity,8);m.scalar(a+0xf4,dt);if(count!=30)m.word(a,unsigned(++count));
 if(count>0){float elapsed=m.scalar(a+0xf4);if(elapsed<m.scalar(0x328c34))for(int i=1;i<count;++i){elapsed=elapsed+m.scalar(a+0xf4+4*i);if(!(elapsed<m.scalar(0x328c34)))break;}}
 return {{m.scalar(position),m.scalar(position+4)}};
}
void tPursuit::UpdateDriveTargets(float,std::optional<std::uint32_t> add_w,std::optional<std::uint32_t> set_w){
 auto& m=m_;const auto a=address_,car=m.word(a+0x10);const std::array<float,2> pos{{m.scalar(car+0x60),m.scalar(car+0x64)}};
 auto append=[&](std::uint32_t target,float threshold,bool inclusive){
  const int count=m.signed_byte(target);if(count<0||count>20)hp2::not_implemented("0x00224B90","original pursuit drive-target count exceeds twenty");
  DriveTarget view(m,target);if(count>1){const auto last=target+16*unsigned(count),previous=last-16;const float x=m.scalar(previous)-m.scalar(last),y=m.scalar(previous+4)-m.scalar(last+4);const float distance=std::sqrt(x*x+y*y);
   if(!std::isfinite(distance))hp2::not_implemented("0x00224B90","original pursuit target-length exceptional operand requires EE semantics");
   if(inclusive?distance<=threshold:distance<threshold){view.SetPoint(count-1,pos,-1,-1,-1,0,set_w);return;}
   if(count==20)view.ShiftLeft(1,true);
  }view.AddPointToTarget(pos,-1,-1,-1,false,0,add_w);
 };
 append(a+0x30,20,false);(void)tNearFollowTarget(m,a+0x450).Update(car+0x60,car+0x70,m.scalar(0x327d08));append(a+0x240,1,true);
}
void tPursuit::SelectPrimaryCop(){auto& m=m_;const auto first=m.word(address_+0x1c),second=m.word(address_+0x20);if(!first||!second)return;const auto car=m.word(address_+0x10);auto distance=[&](std::uint32_t cop){const auto other=m.word(cop+4);const float x=m.scalar(car+0x60)-m.scalar(other+0x60),y=m.scalar(car+0x64)-m.scalar(other+0x64);return std::sqrt(x*x+y*y);};if(30.f<distance(first)-distance(second)){m.word(address_+0x1c,second);m.word(address_+0x20,first);}}
bool tPursuit::ShouldPursuitBeCalledOff(){auto& m=m_;const auto car=m.word(address_+0x10);if(m.byte(car+0x848)||m.word(0x311fa0)||RaceParameters(m,0x311f78).IsFreeRun())return false;const int laps=m.signed_word(0x311fa4);if(laps<=3)return false;const float remaining=((float(laps)-m.scalar(m.word(car+0x1c)+0x20))/float(laps))*m.scalar(0x313308);return remaining<=1000.f;}
bool tPursuit::DoStateTransition(bool disabled){const int state=m_.signed_word(address_+0x5cc);if(state==5)return true;int next=3;if(state==0)next=1;else if(state==1)next=2;else if(state==4)next=4;if(next!=state)SwitchPursuitState(next,disabled);return false;}
bool tPursuit::IsChasedCarStoppable(){
 auto& m=m_;const auto a=address_,car=m.word(a+0x10),manager=m.word(0x328c1c);
 if(!m.word(0x3121d8)||m.word(manager+0x64+0x1c*unsigned(m.signed_half(car+8)))||hp2::recovered::Car(m,car).IsPoofed()||m.signed_half(m.word(car+0x1c)+0x12))return false;
 if(m.word(0x3121ec))hp2::not_implemented("0x00226618","actual BeACop capture timer/player-cop path required");
 const bool nearby=m.word(a+0x1c)?tCopManager(m,manager).IsCopNearby(car,m.scalar(0x328c38)):m.word(m.word(car+0x14)+8)==2;
 bool stopped=false;
 if(10.f<=m.scalar(car+0x3c4)*m.scalar(0x32604c)||!nearby)m.scalar(a+0x5f0,2);
 else{const float timer=m.scalar(a+0x5f0)-m.scalar(0x327d08);m.scalar(a+0x5f0,timer);if(timer<=0)stopped=true;else if(timer<=1)stopped=tCopManager(m,manager).DidRecentlyCollideWithCop(car);}
 if(m.byte(car+0x848)&&m.scalar(car+0x3c4)<m.scalar(0x326050))return true;
 if(hp2::recovered::Car(m,car).IsBlinking())return true;
 if(m.scalar(car+0xb8)<=m.scalar(0x326054)&&m.scalar(car+0x384)*m.scalar(0x326058)<10)return true;
 return stopped;
}
void tPursuit::Level0SubStates(bool disabled){
 auto& m=m_;const auto a=address_;const int sub=m.signed_word(a+0x5d0);const float timer=m.scalar(a+0x5f4);
 if(sub==0&&timer<16){if(IsPlayerInvolved()){const auto exact=GetExactVehicleDescription();if(exact==0xffffffffu){(void)GetCopSpeaker();(void)GetChasedCarColour();port_pursuit_emission(disabled,"0x002054B0");}else{const auto choice=hp2::recovered::bRandom(m,40);(void)GetCopSpeaker();if(choice<26)(void)GetChasedCarColour();port_pursuit_emission(disabled,choice==0?"0x002055E0":choice<17?"0x00205510":choice<26?"0x002054B0":choice<36?"0x00205A10":"0x002053D0");}}m.word(a+0x5d0,1);}
 else if(sub==1&&timer<3){if(m.word(m.word(a+0x10)+0x18)){(void)GetCopSpeaker();port_pursuit_emission(disabled,m.word(a+0x5e0)==0?"0x00205720":"0x002057B0");}m.word(a+0x5d0,2);}
}
void tPursuit::DoEarlyChaseCollisionCheck(bool disabled){auto& m=m_;const auto a=address_,car=m.word(a+0x10);if(!m.word(car+0x18)||m.word(a+0x5e8))return;const auto other=CarCausedRecentCollision(m,car);if(!other)return;m.word(a+0x5e8,1);if(!m.word(a+0x5d0)){BeginPursuitCollisionSpeech(other,disabled);return;}const auto primary=m.word(a+0x1c);(void)GetCopSpeaker();if(primary&&other==m.word(primary+4)){port_pursuit_emission(disabled,"0x002052B0");SwitchPursuitState(2,disabled);return;}port_pursuit_emission(disabled,"0x00206618");if(m.word(m.word(other+0x14)+8)==4)SwitchPursuitState(2,disabled);else if(!m.word(a+0x5cc))SwitchPursuitState(1,disabled);}
bool tPursuit::Update(bool disabled,std::optional<std::uint32_t> add_w,std::optional<std::uint32_t> set_w,::port::race::PhysicsConstructionServices* physics){
 auto& m=m_;const auto a=address_,car=m.word(a+0x10);const float dt=m.scalar(0x327d08);bool finished=false;
 tBarricadeManager(m,m.word(a+0x5c0)).Update(dt);const auto landmark=FindLandmark(m,{{m.scalar(car+0x60),m.scalar(car+0x64)}});m.word(a+0x5e0,landmark?m.word(landmark+0x34):0);
 if(m.word(0x311fa8)!=1&&m.signed_half(m.word(car+0x1c)+0x12))return true;
 if(m.word(0x3121ec))hp2::not_implemented("0x00223B08","actual BeACop chase-direction/update path required");
 if(m.scalar(a+0x608)<m.scalar(car+0x3c0))m.scalar(a+0x608,m.scalar(car+0x3c0));
 const int count=m.signed_word(a+0x5c4);if(count<0||count>2)hp2::not_implemented("0x00223B08","original pursuit cop count exceeds two owned pointers");
 for(int i=0;i<count;++i){const auto other=m.word(m.word(a+0x1c+4*i)+4);if(m.word(other+0x24)==5)m.word(m.word(other+0x30)+0x34,m.word(a+0x608));}
 UpdateDriveTargets(dt,add_w,set_w);SelectPrimaryCop();const bool called_off=ShouldPursuitBeCalledOff();
 if(called_off){if(m.signed_word(a+0x5cc)>1){SwitchPursuitState(5,disabled);finished=true;}}
 else if(count||m.signed_word(a+0x5cc)>=3)m.scalar(a+0x5f4,m.scalar(a+0x5f4)-dt);
 if(m.scalar(a+0x5f4)<=0&&DoStateTransition(disabled))finished=true;
 const auto state=m.word(a+0x5cc);if(state!=4&&state!=5){if(finished)return true;if(IsChasedCarStoppable()){tCopManager(m,m.word(0x328c1c)).CreatePullover(a,car,true,physics);SwitchPursuitState(4,disabled);ReleaseAllCops();finished=true;}}
 if(finished)return true;
 tRoadBlockStateMachine(m,a+0x614).Update(disabled,physics);UpdateRoadBlocks(disabled,physics);UpdateChopper(disabled);
 switch(m.word(a+0x5cc)){case 0:Level0SubStates(disabled);[[fallthrough]];case 1:DoEarlyChaseCollisionCheck(disabled);break;case 5:return true;default:break;}return false;
}
}

#include "game_core/port/driving/ee_scalar.hpp"
namespace hp2::debug {
// Debug00227708,E-0112-COP01. Hidden RVO output is a native two-float return.
// Preserve source's subtraction of FIRST time slot (not requested latency).
std::array<float,2> tNearFollowTarget::GetVelocity(float latency)const{const auto a=address_;const int count=m_.signed_word(a);if(count<0||count>30)hp2::not_implemented("0x00227708","source near-follow count outside30 slots");std::array<float,2> value{{m_.scalar(a+4),m_.scalar(a+8)}};float elapsed=0;for(int i=0;i<count;++i){const float dt=m_.scalar(a+0xf4+4*unsigned(i));elapsed=elapsed+dt;if(latency<=elapsed){if(i<count-1){const float remainder=elapsed-m_.scalar(a+0xf4);for(unsigned j=0;j<2;++j){const auto at=a+4+8*unsigned(i)+4*j;const float current=m_.scalar(at),difference=current-m_.scalar(at+8);const float rate=dt==0?::port::driving::ee_div_s_zero(difference,dt,"0x002277A8"):difference/dt;value[j]=current+rate*remainder;}}break;}}return value;}
}

namespace hp2::debug {
// Debug00224638, E-0112-COP02. Original blown-tire fast path, requested-chopper
// doubling and strict spawn-timer comparison; no host clock or elapsed guess.
bool tPursuit::RoadBlockEncounteredRecently()const{const auto car=m_.word(address_+0x10);if(m_.signed_byte(car+0x848)!=0)return true;float time=m_.scalar(0x328c30);if(m_.word(address_+0x628)==1)time=time+time;return time-4.f<m_.scalar(address_+0x5fc);}
// Debug00227018, E-0112-COP02. Release precedes primary/secondary reassignment;
// crash takes exactly one original shared RNG draw even with speech deferred.
void tPursuit::RemovePrimarySpeaker(bool caused_by_crash,bool disable_speech){auto& m=m_;const auto a=address_;if(!m.word(m.word(a+0x10)+0x18)||RoadBlockEncounteredRecently())return;const auto which=m.word(a+0x5d8);tCopManager manager(m,m.word(0x328c1c));manager.ReleaseCopSpeaker(which);
 if(m.word(a+0x5dc)==5)m.word(a+0x5d8,m.signed_word(a+0x5c4)<2?5:manager.ReserveCopSpeaker());else{m.word(a+0x5d8,m.word(a+0x5dc));m.word(a+0x5dc,5);}
 const char* emission=caused_by_crash?(hp2::recovered::bRandom(m,10)<2?"0x00205960":"0x00205880"):"0x002058D0";
 port_pursuit_emission(disable_speech,emission);
}
}

namespace hp2::debug {
// Debug00225F10/00225FC8,E-0112-COP03. Selectors/RNG precede explicit speech boundary.
void tPursuit::CopBackOffSpeech(bool disabled){auto& m=m_;const auto manager=m.word(0x328c1c);if(m.word(address_+0x5c4)&&(std::fabs(m.scalar(manager+0x286c))>m.scalar(0x326034)||hp2::recovered::bRandom(m,4)==0)){(void)GetCopSpeaker();port_pursuit_emission(disabled,"0x00205D28");}else port_pursuit_emission(disabled,"0x00205D20");}
void tPursuit::AITicketSpeech(bool disabled){auto& m=m_;if(m.word(m.word(0x327290))-8u<2)return;const auto manager=m.word(0x328c1c);if(!m.word(0x3121ec)){(void)tCopManager(m,manager).GetCurrentCopSpeaker();tCopManager(m,manager).NextCopSpeaker();}else(void)m.word(manager+0x2860);const auto description=GetExactVehicleDescription();const auto choice=hp2::recovered::bRandom(m,3);port_pursuit_emission(disabled,choice&&description!=0xffffffffu?"0x00205D78":"0x00205DC8");}
}
#include "game_core/port/race/pursuit_escalation.inc"
