// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trackedcontroller.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring class with adopted retail correspondence; evidence: E-0042-S001.
// Placement: TASK-0042 E-0042-S001; exact Debug definition source/header markers where recorded; unmatched companion remains port-side; retail ABI unconfirmed.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/TrackedController.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0023A178; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a178.md.
void TrackedController::SetCurrentRoute() {
    hp2::not_implemented("0x0023A178", "TrackedController::SetCurrentRoute (signature and full body unresolved)");
}

// ELF 0x0023A198; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a198.md.
void TrackedController::GetCurrentRoute() {
    hp2::not_implemented("0x0023A198", "TrackedController::GetCurrentRoute (signature and full body unresolved)");
}

// ELF 0x0023A1C8; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a1c8.md.
void TrackedController::FindWhereIAm() {
    hp2::not_implemented("0x0023A1C8", "TrackedController::FindWhereIAm (signature and full body unresolved)");
}

// ELF 0x0023A2F8; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a2f8.md.
void TrackedController::JustReset() {
    hp2::not_implemented("0x0023A2F8", "TrackedController::JustReset (signature and full body unresolved)");
}

// ELF 0x0023A300; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a300.md.
void TrackedController::GetAmountOutOfBounds() {
    hp2::not_implemented("0x0023A300", "TrackedController::GetAmountOutOfBounds (signature and full body unresolved)");
}

// ELF 0x0023A3D0; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a3d0.md.
void TrackedController::GetCurrentRoutePoint() {
    hp2::not_implemented("0x0023A3D0", "TrackedController::GetCurrentRoutePoint (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0023A070; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023a070.md.
void TrackedController::Sub_0023A070() {
    hp2::not_implemented("0x0023A070", "TrackedController::Sub_0023A070 (signature and full body unresolved)");
}

// ELF 0x0023A088; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023a088.md.
void TrackedController::Think() {
    hp2::not_implemented("0x0023A088", "TrackedController::Think (signature and full body unresolved)");
}

// ELF 0x0023A0F0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023a0f0.md.
void TrackedController::DoSnapshot() {
    hp2::not_implemented("0x0023A0F0", "TrackedController::DoSnapshot (signature and full body unresolved)");
}
} // namespace hp2::recovered


#include "game_core/port/race/control_construction.hpp"
#include "game_core/indep/inc/CarController.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include <stdexcept>
namespace hp2::debug {
// Debug00233EC0/00234040, E-0099-W122; original trackedcontroller.cpp.
void TrackedController::SetCurrentRoute(std::uint32_t,std::uint32_t point){m_.scalar(address_+0x10,m_.scalar(0x326104));m_.word(address_+0xc,point);m_.scalar(address_+0x18,m_.scalar(m_.word(address_)+0x3c4));}
void TrackedController::JustReset(){}
// Debug00233D68; E-0099-W24.
void TrackedController::Sub_00233D68(std::uint32_t car,const ::port::track::RouteData& routes){
 CarController(m_,address_).Sub_001BD1A8(car);m_.word(address_+0xc,0);m_.word(address_+0x14,0);
 m_.word(address_+0x1c,0);m_.word(address_+0x20,0);m_.word(address_+8,0x002b5aa0);FindWhereIAm(routes);
}
// Debug00233F10; E-0099-W24. Uses original route-distance metric and filters.
void TrackedController::FindWhereIAm(const ::port::track::RouteData& routes){
 if(!routes.initialized)throw std::invalid_argument("TrackedController requires initialized route resources");
 auto& m=m_;const auto a=address_;auto car=m.word(a);const std::array<float,2> pos{{m.scalar(car+0x60),m.scalar(car+0x64)}};
 auto point=m.word(a+0xc);bool valid=false;
 if(point){const auto number=m.byte(point+0x6f);if(number>=routes.routes.size()||!routes.routes[number])throw std::out_of_range("TrackedController route missing");
  point=TrackRoute(m,routes.routes[number]).FindClosestPoint(pos,24,0x22,m.scalar(car+0x7f4),routes.paths.track_length);m.word(a+0xc,point);
  if(point){float outside=(m.scalar(car+0x60)-m.scalar(point))*m.scalar(point+0x10)-(m.scalar(car+0x64)-m.scalar(point+4))*m.scalar(point+0xc);
   outside=outside>0?outside-m.scalar(point+0x1c):-(outside+m.scalar(point+0x18));
   if(outside>5)m.word(a+0xc,0);valid=m.word(a+0xc)!=0;}
  car=m.word(a);
 }
 if(!valid){NearestRoutePointInfo info;if(GetNearestRoutePoint(m,routes,info,pos,ALL_RACING_ROUTES,0x22,m.scalar(car+0x7f4)))m.word(a+0xc,info.Point);}
 m.word(a+0x10,m.word(0x00326108));m.word(a+0x18,m.word(m.word(a)+0x3c4));
}
}


namespace hp2::debug {
// Debug00233DB8; E-0099-W24.
void TrackedController::Sub_00233DB8(int flags){m_.word(address_+8,0x002b5aa0);CarController(m_,address_).Sub_001BD1C0(flags);}
}

namespace hp2::debug {
// Debug00234118, E-0099-W37. Original raw TrackRoutePoint pointer field.
std::uint32_t TrackedController::GetCurrentRoutePoint()const{return m_.word(address_+0xc);}
// Debug00234048. Preserve nested original virtual dispatch; do not clamp signed
// road-width excess to zero or substitute the Car path coordinate's point.
float TrackedController::GetAmountOutOfBounds()const{
 const auto table=m_.word(address_+8),target=m_.word(table+0x3c),adjusted=address_+static_cast<std::int32_t>(m_.signed_half(table+0x38));std::uint32_t point;
 if(target==0x234118)point=TrackedController(m_,adjusted).GetCurrentRoutePoint();
 else if(target==0x1368e8)point=0;
 else hp2::not_implemented("0x00234048","Tracked bounds route-point virtual target is not reconstructed");
 if(!point)return 0;const auto car=m_.word(address_);const float x=m_.scalar(car+0x60)-m_.scalar(point),y=m_.scalar(car+0x64)-m_.scalar(point+4);const float lateral=x*m_.scalar(point+0x10)-y*m_.scalar(point+0xc);return 0<lateral?lateral-m_.scalar(point+0x1c):-(lateral+m_.scalar(point+0x18));
}
// Debug00233EE0/listing, E-0099-W60. The real shared manager table is
// authoritative; no generated route or Car path-coordinate substitution.
std::uint32_t TrackedController::GetCurrentRoute()const{const auto point=m_.word(address_+0xc);return point?m_.word(0x30bb40+std::uint32_t(m_.byte(point+0x6f))*4):0;}
// Debug00233DD0/listing, E-0099-W60. Preserve original timer and acceleration
// update before signed-delta virtual FindWhereIAm dispatch.
void TrackedController::Think(float elapsed,const ::port::track::RouteData& routes){auto& m=m_;const auto a=address_;const float remaining=m.scalar(a+0x10)-elapsed;m.scalar(a+0x10,remaining);if(!(0<remaining)){const auto table=m.word(a+8),target=m.word(table+0x4c);const float divisor=m.scalar(0x326100)-remaining;if(divisor==0)hp2::not_implemented("0x00233DD0","exceptional original tracked-acceleration zero denominator");m.scalar(a+0x14,(m.scalar(m.word(a)+0x3c4)-m.scalar(a+0x18))/divisor);if(target!=0x233f10)hp2::not_implemented("0x00233DD0","tracked FindWhereIAm virtual target is not reconstructed");TrackedController(m,a+std::int32_t(m.signed_half(table+0x48))).FindWhereIAm(routes);}}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/CarController.hpp"
#include "game_core/indep/src/trackroute.hpp"
namespace hp2::debug {
// Debug00233E38; E-0099-W74; replay-frame/00233e38.json/type-TrackedController.json.
void TrackedController::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2b5a88);CarController(m_,address_).DoSnapshot(snapshot);
 TrackRouteManager(m_,0x30bb30).DoPointerSnapshot(snapshot,::port::race::SnapshotRoutePointPointer{address_+12});
 for(unsigned off:{16u,20u,24u})s.MoveData(address_+off,4);
 s.EndSection();
}
}
