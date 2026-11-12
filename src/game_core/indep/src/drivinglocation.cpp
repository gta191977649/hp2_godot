// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/drivinglocation.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00239C90; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00239c90.md.
void DrivingLocation::DoesThisRoutePointMatchThisRoute() {
    hp2::not_implemented("0x00239C90", "DrivingLocation::DoesThisRoutePointMatchThisRoute (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug002338F8; TASK-0099 E-0099-W34. Original pointer publication before line.
void DrivingLocation::SetRoute(std::uint32_t route,std::uint32_t point,int line,bool relaxed){m_.word(address_,route);m_.word(address_+4,point);SetDrivingLine(line,relaxed);}
// Debug00233918; E-0099-W34.
void DrivingLocation::SetDrivingLine(int line,bool relaxed){m_.word(address_+8,static_cast<std::uint32_t>(line));SetRelaxedSwitch(relaxed);}
// Debug00233928; E-0099-W34. Actual WorldTimer, not host wall time.
void DrivingLocation::SetRelaxedSwitch(bool relaxed){m_.word(address_+12,relaxed?m_.word(0x327cf8):0);}
// Debug002339C8; E-0099-W34. Point/line validity is not part of this predicate.
bool DrivingLocation::IsClear()const{return m_.word(address_)==0;}
}

namespace hp2::debug {
// Debug00233940; E-0099-W37. A zero timestamp is not specially excluded.
float DrivingLocation::GetNumSecondsLeftInRelaxedSwitch()const{float remaining=3-static_cast<float>(static_cast<std::int32_t>(m_.word(0x327cf8)-m_.word(address_+12)))*m_.scalar(0x3260fc);if(remaining<0)remaining=0;return remaining;}
// Debug00233990; E-0099-W37.
bool DrivingLocation::AreWeInARelaxedSwitch()const{return GetNumSecondsLeftInRelaxedSwitch()>0;}
}

namespace hp2::debug {
// Debug00233910; E-0099-W47. Only route-point pointer changes.
void DrivingLocation::SetRoutePoint(std::uint32_t point){m_.word(address_+4,point);}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/src/trackroute.hpp"
namespace hp2::debug {
// Debug00233A38, E-0099-W74. Source widths from full MIPS, Timer raw4.
void DrivingLocation::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2b5978);TrackRouteManager routes(m_,0x30bb30);routes.DoSnapshot(address_,snapshot);routes.DoSnapshot(address_+4,m_.word(address_),snapshot);s.MoveData(address_+8,1,4,1);s.MoveData(address_+12,4);s.EndSection();}
}
