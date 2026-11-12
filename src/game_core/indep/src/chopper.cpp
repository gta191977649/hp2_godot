// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\chopper.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): Chopper::ShiftTarget() [0x002B95E0]
// Diagnostic text (not a callable signature): Chopper::FindUnobstructedSpot() [0x002B95F8]
// Diagnostic text (not a callable signature): Chopper::PlaceChopperInWorld() [0x002B9618]
// Diagnostic text (not a callable signature): Chopper::StartChaseLogic() [0x002B9638]
// Diagnostic text (not a callable signature): Chopper::EndChaseLogic() [0x002B9658]
// Diagnostic text (not a callable signature): Chopper::FindChopperPointFromScratch() [0x002B9678]
// Diagnostic text (not a callable signature): Chopper::FindChopperRoutePoint() [0x002B96A0]
// Diagnostic text (not a callable signature): Chopper::FindPreviousFork() [0x002B96C8]
// Diagnostic text (not a callable signature): Chopper::SwitchToNearbyFork() [0x002B96E8]
// Diagnostic text (not a callable signature): Chopper::FollowRoute() [0x002B9708]
// Diagnostic text (not a callable signature): Chopper::BeginStakeout() [0x002B9720]
// Diagnostic text (not a callable signature): Chopper::HangoutNearHere() [0x002B9740]
// Diagnostic text (not a callable signature): Chopper::AdjustTargetInBounds() [0x002B9760]
// Diagnostic text (not a callable signature): Chopper::CleverPursuitPlan() [0x002B9780]
// Diagnostic text (not a callable signature): Chopper::BombRunLogic() [0x002B97A0]
// Diagnostic text (not a callable signature): Chopper::HighLevelPlanning() [0x002B9818]
// Diagnostic text (not a callable signature): Chopper::Think() [0x002B9838]
// Diagnostic text (not a callable signature): Chopper::MoveChopper() [0x002B9850]
// Diagnostic text (not a callable signature): Chopper::DrawChopper() [0x002B9868]
// Diagnostic text (not a callable signature): Chopper::TriggerDustEmitter() [0x002B9880]
// Diagnostic text (not a callable signature): Chopper::Fly() [0x002B98A0]
// Diagnostic text (not a callable signature): Chopper::FireSalvo() [0x002B98B0]
// Diagnostic text (not a callable signature): Chopper::FireWeapon() [0x002B98C8]
#include "game_core/Chopper.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001E8560; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e8560.md.
void Chopper::GetMaxDeceleration() {
    hp2::not_implemented("0x001E8560", "Chopper::GetMaxDeceleration (signature and full body unresolved)");
}

// ELF 0x001E9AE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e9ae8.md.
void Chopper::SetRotateTarget() {
    hp2::not_implemented("0x001E9AE8", "Chopper::SetRotateTarget (signature and full body unresolved)");
}

// ELF 0x001E9E40; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e9e40.md.
void Chopper::CalcChopperRotation() {
    hp2::not_implemented("0x001E9E40", "Chopper::CalcChopperRotation (signature and full body unresolved)");
}

// ELF 0x001E9E80; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e9e80.md.
void Chopper::SubtractFromYaw() {
    hp2::not_implemented("0x001E9E80", "Chopper::SubtractFromYaw (signature and full body unresolved)");
}

// ELF 0x001EA578; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ea578.md.
void Chopper::RenderChopper() {
    hp2::not_implemented("0x001EA578", "Chopper::RenderChopper (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/trackpath.hpp"
#include "game_core/src/bMath.hpp"
namespace hp2::debug {
// Debug001E4188, E-0099-W94; exact chopper.cpp definition owner.
// Source clears XYZ only; CurrentPos.W and all omitted fields remain untouched.
void Chopper::ClearVariables(const ::port::track::RouteData& routes){auto& m=m_;for(unsigned off:{0x2d0u,0x340u,0x2d4u,0x2dcu,0x2e8u,0x2ecu,0x2e0u,0x2e4u,0x31cu,0x320u,0x344u,0x348u})m.word(address_+off,0);m.matrix(address_+0x350,hp2::recovered::bIdentity());m.word(address_+0x2f8,0);TrackPathCoordinate::UpdatePosition(m,address_+0x2f0,routes.paths,{m.scalar(address_+0x340),m.scalar(address_+0x344)});m.scalar(address_+0x334,25);for(unsigned off:{0x338u,0x390u,0x394u,0x398u,0x39cu,0x3a0u,0x3a4u,0x3a8u,0x3acu,0x3b0u,0x3b4u,0x3b8u,0x3bcu,0x318u,0x324u,0x328u,0x330u,0x32cu})m.word(address_+off,0);}
}

#include "game_core/ChopperSound.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/indep/inc/WorldObject.hpp"
#include "game_core/port/track/emitter_bindings.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/indep/src/fmenus.hpp"
namespace hp2::debug {
// Debug001E4018, E-0099-W94. All original child owners use the same shared Memory.
void Chopper::Sub_001E4018(const ::port::track::RouteData& routes){auto& m=m_;TopologyCoordinate(m,address_+8).Sub_00151878();const auto emitters=address_+0x2c8;m.word(emitters,emitters);m.word(emitters+4,emitters);TrackPathCoordinate::Sub_002035A0(m,address_+0x2f0);const auto pool=m.word(0x328298);if(!pool)hp2::not_implemented("0x001E4018","original WorldObject pool startup required");const auto world_object=bOMalloc(m,pool);WorldObject(m,world_object).Sub_001CA730(hp2::frontend::bStringHash("TRACK_HELICOPTER"),0,true);m.word(address_+4,world_object);m.word(address_,sound::CreateChopperSound(m,address_));afxGetEmitters(m,emitters,hp2::frontend::bStringHash("XEM_HELI_DUST"));ClearVariables(routes);m.word(address_+0x2d8,6);}
// Debug001E40D0, E-0099-W94. WorldObject/eModel borrowers precede resource owners.
void Chopper::Sub_001E40D0(int flags){auto& m=m_;const auto emitters=address_+0x2c8;afxDeleteEmitters(m,emitters);if(const auto sample=m.word(address_)){const auto table=m.word(sample+0x1c);if(m.word(table+0xc)!=0x20f9e8)hp2::not_implemented("0x001E4110","unrecovered actual Chopper Sound3d virtual destructor target");const auto self=sample+static_cast<std::uint32_t>(static_cast<std::int32_t>(m.signed_half(table+8)));ChopperSound(m,self).Sub_0020F9E8(3);}if(const auto object=m.word(address_+4))WorldObject(m,object).Sub_001CA7F8(3);TrackPathCoordinate::Sub_00203630(m,address_+0x2f0,2);if(m.word(emitters)!=emitters)hp2::not_implemented("0x00124D70","source AcidEmitter list destructor remains nonempty after actual deletion");TopologyCoordinate(m,address_+8).Sub_001518C0(2);if(flags&1)m.release(address_);}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/inc/TrackRoute.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include "game_core/indep/inc/Sound3d.hpp"
#include "game_core/MunitionEffectsHandler.hpp"
#include "game_core/port/race/debug_transport.hpp"
#include <cmath>
namespace hp2::debug {
// Debug001E44B8, E-0099-W120; original chopper.cpp definition. This accepts a
// chase request only; no movement, coordinate or ownership substitute follows.
bool Chopper::FollowThisCar(std::uint32_t car){auto& m=m_;if(!m.word(0x328668)||!HasSignposts(m))return false;const auto plan=m.word(address_+0x2d8);if(!car){if(plan==3||plan==6){m.word(address_+0x2d4,0);m.word(address_+0x2d8,6);}else m.word(address_+0x2d8,5);}else{if(plan!=6&&plan!=3)return false;m.word(address_+0x2d4,car);m.word(address_+0x2d8,3);}m.word(address_+0x2d0,0);return true;}
// Debug001E6B98, W109/W120. The chase flag clears after the helper, before
// refresh, even if the helper rejects the request. Bomb remains a precise gap.
void Chopper::DoTweakerTriggers(){auto& m=m_;if(m.word(0x32863c)){std::uint32_t car=0;if(m.word(address_+0x2d8)==6){const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x001E6B98","actual current World required for source chase-trigger CarTable[0]");car=m.word(world+0x2c);}FollowThisCar(car);m.word(0x32863c,0);bRefreshTweaker(m);}if(m.word(0x328640))hp2::not_implemented("0x001BBCA8","actual SmokeBomb construction/launch/tweaker child required");}
// Debug001E6C70, E-0099-W109. Mandatory prelude precedes source Plan6 gate.
void Chopper::Fly(float timestep){DoTweakerTriggers();const auto effects=m_.word(0x328188);if(!effects)hp2::not_implemented("0x001BCB50","actual InitMunitions handler is required before Chopper.Fly");MunitionEffectsHandler(m_,effects).DoTimestep(timestep);if(m_.word(address_+0x2d8)!=6){const auto ordinary=[](float f){return std::isfinite(f)&&(f==0||std::isnormal(f));};const float before=m_.scalar(address_+0x2d0);if(!ordinary(before)||!ordinary(timestep))hp2::not_implemented("0x001E6CB4","EE Chopper think-timer subtraction operand outside finite normal/zero scope");const float left=before-timestep;if(!ordinary(left))hp2::not_implemented("0x001E6CB4","EE Chopper think-timer subtraction result outside finite normal/zero scope");m_.scalar(address_+0x2d0,left);if(left<0){const float interval=m_.scalar(0x325c6c),adjusted=left+interval;if(!ordinary(interval)||!ordinary(adjusted))hp2::not_implemented("0x001E6CD0","EE Chopper think-timer addition outside finite normal/zero scope");m_.scalar(address_+0x2d0,adjusted);hp2::not_implemented("0x001E6228","actual Chopper.Think planning chain required");}if(m_.word(address_+0x2d8)!=3)hp2::not_implemented("0x001E63C8","actual MoveChopper and following DrawChopper001E6880 required");}}
// Debug001E4998, E-0099-W98. Preserve disable -> virtual sound -> topology order.
void Chopper::RemoveChopperFromWorld(){auto& m=m_;const auto sound=m.word(address_),table=m.word(sound+0x1c);m.word(m.word(address_+4)+0xc,0);if(m.word(table+0x14)!=0x2039c0)hp2::not_implemented("0x001E4998","actual Chopper Sound3d.Activate virtual target is unrecovered");Sound3d(m,sound+std::uint32_t(std::int32_t(m.signed_half(table+0x10)))).Activate(false);TopologyCoordinate(m,address_+8).ClearInterestBBox();}
// Debug001E4250, E-0099-W98; exact chopper.cpp definition owner. Routes are
// explicit original resource operands, never an empty decoded fallback.
void Chopper::DoSnapshot(std::uint32_t snapshot,const ::port::track::RouteData& routes){
 auto& m=m_;ReplaySnapshot s(m,snapshot);s.BeginSection(0x328628);
 if(m.byte(snapshot+0x3d))RemoveChopperFromWorld();
 s.MoveData(address_+0x2d8,1);
 if(m.word(address_+0x2d8)==6){if(m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d))ClearVariables(routes);}
 else{
  s.MoveData(address_+0x2d0,4);World(m,m.word(0x3272f0)).DoCarPointerSnapshot(address_+0x2d4,snapshot);s.MoveData(address_+0x2dc,1);
  TrackRouteManager manager(m,0x30bb30);
  for(unsigned off:{0x2e0u,0x2e8u,0x31cu}){manager.DoSnapshot(address_+off,snapshot);manager.DoSnapshot(address_+off+4,m.word(address_+off),snapshot);}
  TrackPathCoordinate::DoSnapshot(m,address_+0x2f0,snapshot,routes.paths);
  s.MoveData(address_+0x340,12);s.MoveData(::port::race::SnapshotMatrix{address_+0x350},64);
  s.MoveData(address_+0x390,4);s.MoveData(address_+0x394,8);
  for(unsigned off:{0x39cu,0x3a0u,0x3a4u,0x3a8u,0x3acu,0x3b0u,0x3b4u,0x3b8u})s.MoveData(address_+off,4);
  s.MoveData(address_+0x3bc,1,4,0);s.MoveData(address_+0x318,4);s.MoveData(address_+0x324,1);s.MoveData(address_+0x328,4);s.MoveData(address_+0x32c,4);s.MoveData(address_+0x330,4);s.MoveData(address_+0x334,4);s.MoveData(address_+0x338,4);
 }
 s.EndSection();
}
}
