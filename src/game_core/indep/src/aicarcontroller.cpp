#include "game_core/indep/src/BeltMover.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/carstate.hpp"
#include <cstring>
#include "game_core/indep/inc/AIPlan.hpp"
#include "game_core/indep/src/timer.hpp"
#include "game_core/indep/inc/Player.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/indep/inc/Car.hpp"
// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\aicarcontroller.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AICarController::GetCurrentTrackRoutePoint() [0x002B4CC8]
#include "game_core/indep/inc/AICarController.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): NearbyCarList::BuildArray [0x002B4D70]
#include "game_core/indep/inc/AICarController.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// Debug00191F28; E-0099-W47. Inline Car.hpp implementation emitted by this
// original TU. eMulVector reads the actual initialized homogeneous global;
// source temporary vector padding is not consumed by its XYZ load sequence.
std::array<float,2> Car::WorldToCar(const std::array<float,2>& world)const{auto& m=m_;port::driving::SolverMatrix transposed{};bTransposeMatrix(transposed,m.matrix(CarState(m,address_+0x40).GetTransformMatrix()));const port::driving::SolverVector relative{{world[0]-m.scalar(address_+0x60),world[1]-m.scalar(address_+0x64),0,0}};port::driving::SolverVector transformed{};bMulMatrix(transformed,transposed,relative,{m.vector(0x2d39d0)});return {{transformed[0],transformed[1]}};}
// ELF 0x00193758; E-0042-C084; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00193758.md.
void AICarController::ForceBoost() {
    hp2::not_implemented("0x00193758", "AICarController::ForceBoost (signature and full body unresolved)");
}

// ELF 0x00193780; E-0042-C085; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00193780.md.
void AICarController::ReleaseBoost() {
    hp2::not_implemented("0x00193780", "AICarController::ReleaseBoost (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00191D10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00191d10.md.
void AICarController::ClearStickyDecisions() {
    hp2::not_implemented("0x00191D10", "AICarController::ClearStickyDecisions (signature and full body unresolved)");
}

// ELF 0x00191D90; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00191d90.md.
void AICarController::GetStickyIndex() {
    hp2::not_implemented("0x00191D90", "AICarController::GetStickyIndex (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/aicarcontroller.hpp"

namespace hp2::recovered {
// ELF 0x00193E68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00193e68.md.
void DrivingInformation::Clear() {
    hp2::not_implemented("0x00193E68", "DrivingInformation::Clear (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00193D90; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00193d90.md.
void NearbyCarList::GetCarsIndexInArray() {
    hp2::not_implemented("0x00193D90", "NearbyCarList::GetCarsIndexInArray (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/race/control_construction.hpp"
#include "game_core/indep/inc/CarController.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include "game_core/port/driving/projections.hpp"
#include <stdexcept>
namespace hp2::debug {
// Debug0018FC58; E-0099-W24. Only bit0 is cleared; retain the other sticky bits.
void AICarController::ClearStickyDecisions(){for(unsigned i=0;i<8;++i)m_.word(address_+0x28c+i*4,m_.word(address_+0x28c+i*4)&0xfffffffeu);}
// Debug0018E148; E-0099-W24; original aicarcontroller.cpp.
void AICarController::Sub_0018E148(std::uint32_t car,const ::port::track::RouteData& routes){
 if(!routes.initialized)throw std::invalid_argument("AICarController requires initialized route resources");
 CarController(m_,address_).Sub_001BD1A8(car);auto& m=m_;const auto a=address_;const auto info=a+0xc;
 m.word(a+8,0x002acb70);for(unsigned i=0;i<16;++i)m.word(info+8+i*0x1c+0x18,0);
 m.word(info,car);
 // NearbyCarList::ClearArray00191DB0, then original inline member constructors.
 m.word(info+0x1cc,0);m.word(info+4,0);m.word(info+0x1c8,0);
 m.word(info+0x1dc,0);m.word(info+0x1d8,0xffffffffu);m.word(info+0x1d0,0);m.word(info+0x1d4,0);
 m.word(info+0x1e8,0);m.word(info+0x1e0,0);m.word(info+0x1e4,0);
 // DrivingInformation::Clear00191B40, called twice in the original constructor.
 m.word(info+4,0);m.word(info+0x1d8,0xffffffffu);m.word(info+0x1e0,0);m.word(info+0x1e8,0);
 m.word(info+0x1c8,0);m.word(info+0x1cc,0);m.word(info+0x1d0,0);m.word(info+0x1d4,0);m.word(info+0x1dc,0);m.word(info+0x1e4,0);
 for(auto off:{0x218u,0x220u,0x228u,0x22cu,0x234u,0x238u,0x23cu,0x240u,0x250u,0x254u})m.word(a+off,0);
 ClearStickyDecisions();m.word(a+0x214,0);m.word(a+0x210,11);m.byte(a+0x208,0xff);
 m.scalar(a+0x20c,static_cast<float>(m.signed_half(car+8))*m.scalar(0x003254ec));
 m.word(info+4,0);m.word(info+0x1d8,0xffffffffu);m.word(info+0x1e0,0);m.word(info+0x1e8,0);
 m.word(info+0x1c8,0);m.word(info+0x1cc,0);m.word(info+0x1d0,0);m.word(info+0x1d4,0);m.word(info+0x1dc,0);m.word(info+0x1e4,0);
 m.word(a+0x1f8,info+0x1d0);m.word(a+0x1fc,info+0x1e0);m.word(a+0x200,info);
 m.word(a+0x218,0);m.word(a+0x21c,0);m.word(a+0x204,0);m.word(a+0x234,0);m.word(a+0x238,0);m.word(a+0x23c,0);
 m.word(a+0x24c,0);m.word(a+0x250,0);m.word(a+0x254,0);m.word(a+0x258,0);
 m.word(a+0x240,static_cast<std::uint32_t>(::port::driving::cvt_w_s(m.scalar(0x003254f0))));
 const auto progress=m.allocate(16);Average(m,progress).Sub_0018DBF8(10);m.word(a+0x244,progress);
 const auto delta=m.allocate(16);Average(m,delta).Sub_0018DBF8(5);m.word(a+0x2ac,delta);
 m.vector(a+0x2b0,{{0,0,0,0}});m.word(a+0x280,1);m.word(a+0x284,0);m.word(a+0x224,0);m.word(a+0x220,0);
 ClearStickyDecisions();m.word(a+0x288,static_cast<std::uint32_t>(Signpost::GetIndex(m,routes,m.scalar(m.word(a)+0x7f4))));m.word(a+0x230,1);
}
}


namespace hp2::debug {
// Debug0018E398; E-0099-W24. Empty embedded member destructors confirmed separately.
void AICarController::Sub_0018E398(int flags){
 auto& m=m_;const auto a=address_;m.word(a+8,0x002acb70);
 if(const auto action=m.word(a+0x214)){const auto table=m.word(action+0x10),target=m.word(table+0xc),self=action+static_cast<std::int32_t>(m.signed_half(table+8));if(target==0x1c7818)AIActionJustDrive(m,self).Sub_001C7818(3);else if(target==0x1c7a20)AIActionAvoidCars(m,self).Sub_001C7A20(3);else if(target==0x1c79a8)AIActionPass(m,self).Sub_001C79A8(3);else if(target==0x1c7658)AIActionIJustGotHit(m,self).Sub_001C7658(3);else if(target==0x1c75e8)AIActionAccidentRecovery(m,self).Sub_001C75E8(3);else if(target==0x1c77a8)AIActionPullOver(m,self).Sub_001C77A8(3);else if(target==0x1c76c8)AIActionGetUnstuck(m,self).Sub_001C76C8(3);else if(target==0x1c3ba8)AIAction(m,self).Sub_001C3BA8(3);else hp2::not_implemented("0x0018E398","original active AIAction destructor virtual target is not reconstructed");}
 if(const auto average=m.word(a+0x244))Average(m,average).Sub_0018DC70(3);
 if(const auto average=m.word(a+0x2ac))Average(m,average).Sub_0018DC70(3);
 // DrivingInformation00191D40 ->DrivingChoice00191EA8, DrivingLocation00191EE0,
 // NearbyCarList00191DC0 ->NearbyCar00191E58: embedded flags2/0 do no work.
 CarController(m,a).Sub_001BD1C0(flags);
}
}

#include "game_core/indep/inc/Car.hpp"
namespace hp2::debug {
// Debug00191328, E-0099-W36. Timer constants are floats converted to packed int.
void AICarController::SetToPhysicsMode(std::uint32_t collision,::port::race::PhysicsConstructionServices& services){
 auto& m=m_;const auto car=m.word(address_),previous=m.word(car+0x24);
 if(previous==6)m.word(address_+0x238,static_cast<std::uint32_t>(::port::driving::cvt_w_s(m.scalar(0x32558c))));
 auto mode=m.word(0x326fa0);if(collision&&m.word(collision+0x18))mode=1;
 if(mode!=previous)hp2::recovered::Car(m,car).SetMovementMode(static_cast<int>(mode),services);
 if(mode==1)m.word(address_+0x23c,static_cast<std::uint32_t>(::port::driving::cvt_w_s(m.scalar(0x325590))));
}
}

namespace hp2::debug {
// Debug00191A68; E-0099-W37. Exact original pointer search and first match.
int NearbyCarList::GetCarsIndexInArray(std::uint32_t car)const{const int n=m_.signed_word(address_+4);if(n>16)hp2::not_implemented("0x00191A68","original nearby count exceeds16 authored carrier entries");for(int i=0;i<n;++i)if(m_.word(address_+8+28*i+0x18)==car)return i;return -1;}
// Debug00191AB0; E-0099-W37.
std::uint32_t NearbyCarList::GetNearbyCar(std::uint32_t car)const{const int i=GetCarsIndexInArray(car);return i==-1?0:address_+8+28*i;}
}

#include "game_core/port/race/ai_drive_target.hpp"
#include "game_core/indep/inc/DrivingChoice.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/drivinglocation.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/indep/inc/World.hpp"
namespace hp2::debug {
// Debug0018FCD0; E-0099-W37. Original exhaustion returns-1, with no invented LRU.
int AICarController::GetStickyIndex()const{for(int i=0;i<8;++i)if(!(m_.word(address_+0x28c+4*i)&1))return i;return -1;}
// Debug001902F8; E-0099-W37. Full real target construction/scoring/prediction
// chain, ordered line choices, actual tuning operands and original highlight.
bool AICarController::GetNewDrivingLine(std::uint32_t route,std::uint32_t point,bool relaxed,int& line,std::uint32_t location,const ::port::race::AiDriveTargetServices& services){auto& m=m_;const auto a=address_;bool okay=false;::port::driving::Memory::Temporary target(m,0x210),choice(m,12),temp(m,16);DriveTarget(m,target.address()).Sub_00216528();DrivingChoice pick(m,choice.address());pick.Clear();m.word(temp.address()+12,0);m.word(temp.address()+12,0);m.word(temp.address()+8,0xffffffff);m.word(temp.address(),0);m.word(temp.address()+4,0);float score=-1,impact=m.scalar(0x32554c);std::uint32_t to_hit=0;line=DrivingChoice::GetDrivingLine(m,1,m.word(a));DrivingLocation(m,temp.address()).SetRoute(route,point,line,relaxed);if(route&&point){const int count=m.signed_word(0x327ac0);if(count>0){int type=1;while(true){pick.SetChoice(type,0,false);const int current=DrivingChoice::GetDrivingLine(m,type,m.word(a));DrivingLocation(m,temp.address()).SetDrivingLine(current,relaxed);if(pick.IsDriveable(target.address(),false,score,to_hit,impact,temp.address(),m.word(a+0x200),0,m.scalar(0x328260),0,services)){okay=true;line=DrivingChoice::GetDrivingLine(m,type,m.word(a));break;}const bool again=type<count;++type;if(!again)break;}}if(location)HighlightDrivingLine(m,location,m.signed_half(m.word(a)+0xa));}return okay;}
// Debug0018FD20; E-0099-W37. Original eight sticky decisions, Debug flag rules,
// grace branch, exact World RNG/skill chances and recursive real line validation.
bool AICarController::SignPostDecision(std::uint32_t sign,int& line,std::uint32_t location,const ::port::race::AiDriveTargetServices& services){auto& m=m_;const auto a=address_;const auto packed=m.word(sign);if((packed&0x800)||(packed&0x4000)){line=DrivingLocation(m,location).IsClear()?DrivingChoice::GetDrivingLine(m,1,m.word(a)):m.signed_word(location+8);return false;}if(!services.routes||!services.routes->signposts)hp2::not_implemented("0x0018FD20","actual shared signpost-array owner is not loaded");const auto sign_index=(sign-services.routes->signposts)>>3;for(unsigned i=0;i<8;++i){const auto decision=m.word(a+0x28c+4*i);if((decision&1)&&m.byte(a+0x28d+4*i)==(sign_index&255)){line=(decision>>2)&63;return ((decision>>1)&1)!=0;}}
 const int sticky=GetStickyIndex();bool take=false;const auto mode=packed&0x700;if(mode==0x200)take=true;else{const auto race=m.word(0x327258);if(!race)hp2::not_implemented("0x00149088","original Race grace owner is not constructed");if(!Race(m,race).GracePeriodIsActive(m.word(a))){const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0014E230","original World RNG owner is not constructed");if(mode==0x400)take=World(m,world).Random(1)<0.5f;else if(mode!=0x500){if(!services.planning)hp2::not_implemented("0x0018FD20","reviewed authored AI skill sign-chance resource is not bound");int skill=m.signed_byte(m.word(m.word(a)+0x14)+3);if(skill<0)skill=0;if(skill>9)skill=9;const float random=World(m,world).Random(1);int decision=0;if(random<=services.planning->take_sign_chance(m,skill,0))decision=2;else if(random<=services.planning->take_sign_chance(m,skill,1))decision=1;else if(random<=services.planning->take_sign_chance(m,skill,2))decision=3;if(decision==0||decision==1)take=mode==0;else if(decision==2)take=mode==0x100;}}
  if(!take)line=m.signed_word(location+8);else{const auto route=m.word(0x30bb40+4*m.byte(sign)),point=route+0x428+112*m.half(sign+2);if(!GetNewDrivingLine(route,point,true,line,location,services))take=false;}
 }
 if(take&&line<0){const auto route=m.word(0x30bb40+4*m.byte(sign)),point=route+0x428+112*m.half(sign+2);GetNewDrivingLine(route,point,true,line,location,services);}if(static_cast<std::uint32_t>(line)>1)line=DrivingLocation(m,location).IsClear()?DrivingChoice::GetDrivingLine(m,1,m.word(a)):m.signed_word(location+8);const auto address=a+0x28c+static_cast<std::uint32_t>(4*sticky),old=m.word(address);m.word(address,(old&0xfffffffd)|1|(take?2:0));m.word(address,(old&0xffffff01)|1|(take?2:0)|((static_cast<std::uint32_t>(line)&63)<<2));m.byte(address+1,static_cast<std::uint8_t>(sign_index));return take;
}
}

#include "game_core/port/race/traffic_construction.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include <cmath>
namespace hp2::debug {
// Debug0018E650; E-0099-W37. GetNewDrivingLine return is intentionally ignored.
void AICarController::SetCurrentRoute(std::uint32_t route,std::uint32_t point,const ::port::race::AiDriveTargetServices& services){if(route&&point){const auto location=m_.word(address_+0x1f8),old=m_.word(location);m_.byte(address_+0x208,old?static_cast<std::uint8_t>(m_.signed_half(old+0xa)):0xff);int line=0;GetNewDrivingLine(route,point,true,line,0,services);DrivingLocation(m_,location).SetRoute(route,point,line,true);}}
// Debug0018E6E8/0018E740; E-0099-W37. Full original virtual location recovery.
std::uint32_t AICarController::GetCurrentRoute(const ::port::race::AiDriveTargetServices& services){if(DrivingLocation(m_,m_.word(address_+0x1f8)).IsClear()){const auto table=m_.word(address_+8),target=m_.word(table+0x4c),self=address_+static_cast<std::int32_t>(m_.signed_half(table+0x48));if(target!=0x190d48)hp2::not_implemented("0x0018E6E8","original FindWhereIAm virtual target is not reconstructed");AICarController(m_,self).FindWhereIAm(services);(void)DrivingLocation(m_,m_.word(address_+0x1f8)).IsClear();}return m_.word(m_.word(address_+0x1f8));}
std::uint32_t AICarController::GetCurrentRoutePoint(const ::port::race::AiDriveTargetServices& services){if(DrivingLocation(m_,m_.word(address_+0x1f8)).IsClear()){const auto table=m_.word(address_+8),target=m_.word(table+0x4c),self=address_+static_cast<std::int32_t>(m_.signed_half(table+0x48));if(target!=0x190d48)hp2::not_implemented("0x0018E740","original FindWhereIAm virtual target is not reconstructed");AICarController(m_,self).FindWhereIAm(services);(void)DrivingLocation(m_,m_.word(address_+0x1f8)).IsClear();}return m_.word(m_.word(address_+0x1f8)+4);}
// Debug00190D48; E-0099-W37. Exact fresh-clear sequence and original route-range
// virtual dispatch. AIDriver/TrafficDriver are concrete; other targets stay loud.
void AICarController::FindWhereIAm(const ::port::race::AiDriveTargetServices& services){const auto location=m_.word(address_+0x1f8);m_.word(location+8,0xffffffff);m_.word(location+12,0);m_.word(location,0);m_.word(location+4,0);const auto table=m_.word(address_+8),target=m_.word(table+0x74),self=address_+static_cast<std::int32_t>(m_.signed_half(table+0x70));int first=0,last=0;if(target==0x1309d8)AIDriver(m_,self).GetRouteNumbersToUse(first,last);else if(target==0x18ac10)TrafficDriver(m_,self).GetRouteNumbersToUse(first,last);else hp2::not_implemented("0x00190D48","original GetRouteNumbersToUse virtual target is not reconstructed");FindClosestTrackRoute(first,last,services);}
// Debug0018E8C8; E-0099-W37. Original two-pass authored-route scan, XYZ distance
// ranking and exclusion of the prior route, then real SetCurrentRoute virtual.
void AICarController::FindClosestTrackRoute(int first,int last,const ::port::race::AiDriveTargetServices& services){if(!services.routes)hp2::not_implemented("0x0018E8C8","actual route resource owner is not bound");auto& m=m_;const auto a=address_,location=m.word(a+0x1f8),old_route=m.word(location),car=m.word(a);const float around=m.scalar(car+0x7f4);std::uint32_t best_point=0;float best=m.scalar(0x3254f8);for(int pass=0;pass<2;++pass){std::uint32_t best_route=0;for(int number=first;number<=last;++number){if(number>=100)hp2::not_implemented("0x0018E8C8","original route range exceeds shared100-entry table");const auto route=number<0?0:m.word(0x30bb40+4*number);if(!route)continue;if(pass==0&&!TrackRoute(m,route).DistanceIsWithinRoute(around))continue;const auto point=TrackRoute(m,route).FindClosestPoint({{m.scalar(car+0x60),m.scalar(car+0x64)}},m.scalar(0x3254fc),3,0,m.scalar(0x313308));if(!point)continue;if(!services.port_stack_vector_lane)hp2::not_implemented("0x001BE4B8","original temporary XYZ vector padding requires explicit port operand");::port::driving::Memory::Temporary position(m,16);TrackRoutePoint(m,point).GetPosition(position.address(),0,0,*services.port_stack_vector_lane);const float x=m.scalar(car+0x60)-m.scalar(position.address()),y=m.scalar(car+0x64)-m.scalar(position.address()+4),z=m.scalar(car+0x68)-m.scalar(position.address()+8);const float distance=std::sqrt((x*x+y*y)+z*z);if(distance<best&&route!=old_route){best=distance;best_route=route;best_point=point;}}
  if(best_route){if(best_point){const auto table=m.word(a+8),target=m.word(table+0x2c),self=a+static_cast<std::int32_t>(m.signed_half(table+0x28));if(target!=0x18e650)hp2::not_implemented("0x0018E8C8","original SetCurrentRoute virtual target is not reconstructed");AICarController(m,self).SetCurrentRoute(best_route,best_point,services);return;}break;}}
 const auto current=m.word(a+0x1f8);m.word(current+8,0xffffffff);m.word(current+12,0);m.word(current,0);m.word(current+4,0);
}
}

namespace hp2::debug {
// Debug0018E798; E-0099-W37. Source clear-location returns0, otherwise signed
// road-width excess, preserving negative values inside the road.
float AICarController::GetAmountOutOfBounds()const{const auto location=m_.word(address_+0x1f8);if(DrivingLocation(m_,location).IsClear())return 0;const auto car=m_.word(address_),point=m_.word(location+4);const float lateral=(m_.scalar(car+0x60)-m_.scalar(point))*m_.scalar(point+0x10)-(m_.scalar(car+0x64)-m_.scalar(point+4))*m_.scalar(point+0xc);return lateral>0?lateral-m_.scalar(point+0x1c):-(lateral+m_.scalar(point+0x18));}
}

namespace hp2::debug {
// Debug001906B8; E-0099-W47. Source separate cosine/sine calls and actual
// driver SetSteerToPoint descriptor slot, with no generated steering control.
void AICarController::SteerStraightAhead(){auto& m=m_;const auto car=m.word(address_),driver=m.word(address_+4);const auto angle=m.half(car+0x3a4);const float cosine=hp2::recovered::bCos(m,angle),sine=hp2::recovered::bSin(m,angle);const std::array<float,2> point{{m.scalar(car+0x60)+cosine,m.scalar(car+0x64)+sine}};const auto table=m.word(driver+0x22c),target=m.word(table+0x4c),adjusted=driver+static_cast<std::int32_t>(m.signed_half(table+0x48));if(target!=0x13ab18)hp2::not_implemented("0x001906B8","original driver SetSteerToPoint virtual target is not reconstructed");PsychoDriver(m,adjusted).SetSteerToPoint(point);}
// Debug0018F0E0; E-0099-W47. Prestart/countdown plus original first-action
// active continuation. Existing-action interrupt/selection, active boost/Belt
// and debug resource branches remain explicit where the source requires them.
void AICarController::Think(float seconds,const ::port::race::AiDriveTargetServices& services,::port::race::PhysicsConstructionServices& physics){(void)seconds;(void)services;(void)physics;auto& m=m_;const auto a=address_,race=m.word(0x327258);if(!race)hp2::not_implemented("0x00148E50","actual current Race is required for controller Think");if(Race(m,race).CanAICarsStartDriving()){
 const auto car=m.word(a),nearby=m.word(a+0x200),location=m.word(a+0x1f8);
 const auto active=[&](unsigned offset){const auto value=m.word(a+offset);return value!=0&&value!=0x7fffffff;};
 const auto countdown=[&](unsigned offset){Timer timer{m.signed_word(a+offset)};const int value=timer.CountDown(seconds);m.word(a+offset,static_cast<std::uint32_t>(timer.PackedTime));return value;};
 if(const auto action=m.word(a+0x214)){if(m.word(action+0x248)!=6&&active(0x240)&&!countdown(0x240)){m.word(a+0x240,static_cast<std::uint32_t>(::port::driving::cvt_w_s(m.scalar(0x325534))));Average(m,m.word(a+0x244)).Record(m.scalar(0x313308)*std::fabs(m.scalar(m.word(car+0x1c)+0x18)));const float distance=World(m,m.word(0x3272f0)).GetDistanceToClosestPlayer(std::array<float,3>{{m.scalar(car+0x60),m.scalar(car+0x64),m.scalar(car+0x68)}});if(!hp2::recovered::Car(m,car).IsBlinking()){const auto average=m.word(a+0x244);if(m.byte(average+2)==m.byte(average)&&m.scalar(average+8)<m.scalar(0x325538)&&distance>=50.f)hp2::recovered::Car(m,car).PleaseResetMeLater();}}}
 if(active(0x23c)&&!countdown(0x23c)){if(m.signed_word(car+0x3bc)<4)m.word(a+0x23c,static_cast<std::uint32_t>(::port::driving::cvt_w_s(m.scalar(0x32553c))));else hp2::recovered::Car(m,car).SetMovementMode(m.signed_word(0x326fa0),physics);}
 if(active(0x238)){if(m.word(car+0x24)==6)hp2::recovered::Car(m,car).SetMovementMode(m.signed_word(0x326fa0),physics);countdown(0x238);}
 countdown(0x250);countdown(0x254);
 if(m.word(0x326f38)&&!m.word(0x32753c)&&m.signed_word(0x327540)==GetMyDriverNumber()&&m.word(car+0x24)==6)hp2::not_implemented("0x0018F0E0","source BeltMover DebugScreenPrint is not reconstructed");
 if(m.word(0x327a68)&&!DrivingLocation(m,location).IsClear())hp2::not_implemented("0x0018F0E0","source above-car speed-difference printing is not reconstructed");
 if(m.signed_word(0x327540)==GetMyDriverNumber()&&m.word(0x327a08))hp2::not_implemented("0x0018F0E0","source tweaker/debug-reset dependencies are not reconstructed");
 m.word(nearby+4,0);m.scalar(a+0x20c,m.scalar(a+0x20c)-seconds);m.scalar(nearby+0x1c8,0);m.scalar(nearby+0x1cc,0);
 const auto world=m.word(0x3272f0);const int type=m.signed_word(m.word(car+0x14)+8),loop=m.signed_word(0x327d10);bool evaluate;
 if(type==2){const int slot=loop%8;evaluate=slot<m.signed_word(world+0x24)-m.signed_word(world+0x20)&&m.word(world+0x2c+4*(m.signed_word(world+0x20)+slot))==car;}else if(type==3){const int slot=loop%6;evaluate=slot<m.signed_word(world+0x1c)-m.signed_word(world+0x28)&&m.word(world+0x2c+4*(m.signed_word(world+0x28)+slot))==car;}else evaluate=!(0.f<m.scalar(a+0x20c));
 const float pass=m.scalar(0x328264),impact=m.scalar(0x328260),look=m.scalar(0x327a00);const float minimum=pass<impact?impact:pass;const float horizon=minimum<look?look:minimum;
 if(!evaluate&&m.word(a+0x214)){const auto choice=m.word(a+0x1fc);if(!DrivingChoice::DrivingChoiceTypeIsSideOfCar(m.signed_word(choice))||!m.word(m.word(choice+4)+0x18))return;::port::driving::Memory::Temporary interim(m,0x210);DriveTarget(m,interim.address()).Sub_00216528();NearbyCarList(m,nearby).BuildArray(location,horizon);if(NearbyCarList(m,nearby).GetNearbyCar(m.word(choice+4))){DrivingChoice(m,choice).BuildDriveTarget(interim.address(),location,nearby,0,0,services);const auto driver=m.word(a+4),table=m.word(driver+0x22c),target=m.word(table+0x44),self=driver+static_cast<std::int32_t>(m.signed_half(table+0x40));if(target!=0x13a230)hp2::not_implemented("0x0018F0E0","source interim driver SetTarget virtual target is not reconstructed");PsychoDriver(m,self).SetTarget(interim.address());if(m.signed_word(0x327540)==GetMyDriverNumber())DriveTargetRenderer(m,0x30bd18).SetTargetToRender(driver);}return;}
 m.scalar(a+0x20c,m.scalar(a+0x20c)+m.scalar(0x325544));DoStickyDecisions(services);NearbyCarList(m,nearby).BuildArray(location,horizon);GetCurrentTrackRoutePoint(0,services);
 if(DrivingLocation(m,location).IsClear()){SteerStraightAhead();const auto driver=m.word(a+4),table=m.word(driver+0x22c),target=m.word(table+0x3c),self=driver+static_cast<std::int32_t>(m.signed_half(table+0x38));if(target!=0x1398a8)hp2::not_implemented("0x0018F0E0","source clear-location desired-speed virtual target is not reconstructed");PsychoDriver(m,self).SetDesiredSpeed(0,0,true);return;}
 if(m.word(0x32711c))hp2::not_implemented("0x0018F0E0","source camera-tuning action continuation is not reconstructed");
 if(!m.word(a+0x214)){const auto action=bOMalloc(m,m.word(0x3281e8));AIActionJustDrive(m,action).Sub_001C4430(m.signed_word(0x3281f8),-1);m.word(a+0x214,action);}else{
  AIAction(m,m.word(a+0x214)).UpdateStatus(a+0xc,&services);
  const auto current=m.word(a+0x214);
  const auto status=current?m.word(current+0xc):0;
  const bool replace=!current||status==1||status==2||(status==3&&m.word(current)!=0);
  const auto port_destroy=[&](std::uint32_t action){if(!action)return;const auto table=m.word(action+0x10),target=m.word(table+0xc),self=action+std::int32_t(m.signed_half(table+8));if(target==0x1c7818)AIActionJustDrive(m,self).Sub_001C7818(3);else if(target==0x1c7a20)AIActionAvoidCars(m,self).Sub_001C7A20(3);else if(target==0x1c79a8)AIActionPass(m,self).Sub_001C79A8(3);else if(target==0x1c7658)AIActionIJustGotHit(m,self).Sub_001C7658(3);else if(target==0x1c75e8)AIActionAccidentRecovery(m,self).Sub_001C75E8(3);else if(target==0x1c77a8)AIActionPullOver(m,self).Sub_001C77A8(3);else if(target==0x1c76c8)AIActionGetUnstuck(m,self).Sub_001C76C8(3);else if(target==0x1c3ba8)AIAction(m,self).Sub_001C3BA8(3);else hp2::not_implemented("0x0018F89C","original selected AIAction destructor is not reconstructed");};
  if(replace){
   port_destroy(current);m.word(a+0x214,0);
   ::port::driving::Memory::Temporary next(m,4);m.word(next.address(),0);
   const auto table=m.word(a+8),target=m.word(table+0x7c),self=a+std::int32_t(m.signed_half(table+0x78));
   if(target==0x130388)AIDriver(m,self).ChooseNewAction(next.address(),11,services);
   else if(target==0x1905f0)AICarController(m,self).ChooseNewAction(next.address(),11);
   else hp2::not_implemented("0x0018F8BC","original ChooseNewAction virtual target is not reconstructed");
   m.word(a+0x214,m.word(next.address()));
  }else if(const auto next=InterruptAction(services)){port_destroy(m.word(a+0x214));m.word(a+0x214,next);}
 }
 const auto table=m.word(a+8),target=m.word(table+0x84),self=a+static_cast<std::int32_t>(m.signed_half(table+0x80));if(target!=0x1904d8)hp2::not_implemented("0x0018F0E0","source GeneratePointsToFulfillAction virtual target is not reconstructed");AICarController(m,self).GeneratePointsToFulfillAction(m.word(a+0x214),services);ExecuteCurrentAction(services);
 if(WeNeedTheBelt()&&WeCanHaveTheBelt()){if(m.word(car+0x24)!=6)hp2::recovered::Car(m,car).SetMovementMode(6,physics);BeltMover(m,m.word(car+0x30)).SetBelt();}else if(m.word(car+0x24)==6)hp2::recovered::Car(m,car).SetMovementMode(m.signed_word(0x326fa0),physics);
 if(m.signed_word(0x327540)==GetMyDriverNumber())DriveTargetRenderer(m,0x30bd18).SetTargetToRender(m.word(a+4));m.scalar(nearby+0x1cc,0);m.word(nearby+4,0);m.scalar(nearby+0x1c8,0);return;
}float desired=0;if(m.signed_word(race)>=2){const auto packed=m.signed_word(a+0x228);bool generate=false;float random=0,base=0;if(packed==0||packed==0x7fffffff){const bool brake=hp2::recovered::bRandom(m,2)!=0;m.word(a+0x230,brake?1:0);random=hp2::recovered::bRandom(m,brake?.25f:1.f);base=brake?m.scalar(0x325524):1.5f;generate=true;}else if(static_cast<float>(packed)*m.scalar(0x325528)<static_cast<float>(static_cast<std::int32_t>(m.word(0x327cf8)-m.word(a+0x22c)))*m.scalar(0x325528)){m.word(a+0x230,m.word(a+0x230)^1u);const bool brake=m.word(a+0x230)!=0;random=hp2::recovered::bRandom(m,brake?.25f:1.f);base=brake?m.scalar(0x32552c):1.5f;generate=true;}if(generate){m.word(a+0x228,static_cast<std::uint32_t>(::port::driving::cvt_w_s((random+base)*4000.f+.5f)));m.word(a+0x22c,static_cast<std::uint32_t>(::port::driving::cvt_w_s((static_cast<float>(m.signed_word(0x327cf8))*m.scalar(0x325530))*4000.f+.5f)));}if(m.word(a+0x230))desired=100;}
 const auto driver=m.word(a+4),table=m.word(driver+0x22c),target=m.word(table+0x3c),adjusted=driver+static_cast<std::int32_t>(m.signed_half(table+0x38));if(target!=0x1398a8)hp2::not_implemented("0x0018F0E0","original driver SetDesiredSpeed virtual target is not reconstructed");PsychoDriver(m,adjusted).SetDesiredSpeed(desired,0,true);SteerStraightAhead();}
}

#include "game_core/indep/src/trackpath.hpp"
namespace hp2::debug {
// Debug00191570; E-0099-W47. Source intrusive list scan, candidate writes before
// rejection, sixteen accepted rows and stable descending longitudinal ordering.
// min_time is not consumed by this original body.
void NearbyCarList::BuildArray(std::uint32_t location,float min_time){(void)min_time;auto& m=m_;const auto a=address_,car=m.word(a);const float around=m.scalar(car+0x7f4);if(!car||!location)return;for(unsigned i=0;i<0x1c0;++i)m.byte(a+8+i,0);float lookahead=CalculateLookAheadDistance(m,car);const float acceleration=CalculateMaxAcceleration(m,car,m.scalar(car+0x3c4));if(m.word(car+0x20)==3)lookahead=lookahead*m.scalar(0x327958);unsigned count=0;const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x00191570","actual current World intrusive CarList is required");::port::driving::Memory::Temporary bounds(m,32);for(auto other=m.word(world);other!=world;other=m.word(other)){if(count>=16)continue;const bool poofed=m.byte(other+0x868)!=0;if(((poofed?0u:1u)&m.byte(other+0x4a8))==0)continue;const auto position=hp2::recovered::Car(m,car).WorldToCar({{m.scalar(other+0x60),m.scalar(other+0x64)}});::port::driving::SolverVector relative{};for(unsigned k=0;k<4;++k)relative[k]=m.scalar(other+0x70+4*k)-m.scalar(car+0x70+4*k);const auto row=a+8+0x1c*count;const float track_distance=std::fabs(TrackPathManager::CompareDistances(m.scalar(0x313308),m.scalar(other+0x7f4),around));m.scalar(row,position[0]);m.scalar(row+4,position[1]);m.scalar(row+8,relative[0]);m.scalar(row+12,relative[1]);m.word(row+0x18,other);if(!((std::sqrt(position[0]*position[0]+position[1]*position[1])<=lookahead||CanICatchThisCar(m,row,car,acceleration,false))&&track_distance<=lookahead*3.f))continue;++count;const auto angle=hp2::recovered::bATan(m,m.scalar(car+0x70),m.scalar(car+0x74));::port::driving::SolverMatrix rotation{};hp2::recovered::eRotateZ(m,rotation,m.matrix(0x2d3950),static_cast<std::uint16_t>(0x4000-angle));::port::driving::SolverVector rotated{};hp2::recovered::bMulMatrix(rotated,rotation,relative,{m.vector(0x2d39d0)});m.scalar(row+8,rotated[0]);m.scalar(row+12,rotated[1]);hp2::recovered::Car(m,other).GetCarSpaceBoundingBox(bounds.address(),bounds.address()+16);const float half_x=(m.scalar(bounds.address()+16)-m.scalar(bounds.address()))*.5f,half_y=(m.scalar(bounds.address()+20)-m.scalar(bounds.address()+4))*.5f;float cosine=hp2::recovered::bCos(m,static_cast<std::uint16_t>(m.half(other+0x3a4)-m.half(car+0x3a4)));cosine=cosine*cosine;m.scalar(row+0x10,cosine*half_y+(1.f-cosine)*half_x);m.scalar(row+0x14,cosine*half_x+(1.f-cosine)*half_y);}
 m.word(a+4,count);std::array<std::array<std::uint8_t,0x1c>,16> scratch{};std::array<unsigned,16> order{};for(unsigned i=0;i<16;++i){order[i]=i;for(unsigned j=0;j<0x1c;++j)scratch[i][j]=m.byte(a+8+0x1c*i+j);}auto longitudinal=[&](unsigned i){float value;std::memcpy(&value,scratch[i].data()+4,4);return value;};for(unsigned pass=0;pass<count;++pass)for(unsigned i=0;i+1<count;++i)if(longitudinal(order[i])<longitudinal(order[i+1]))std::swap(order[i],order[i+1]);for(unsigned i=0;i<count;++i)for(unsigned j=0;j<0x1c;++j)m.byte(a+8+0x1c*i+j,scratch[order[i]][j]);hp2::recovered::Car(m,car).GetCarSpaceBoundingBox(bounds.address(),bounds.address()+16);m.word(a,car);m.scalar(a+0x1c8,m.scalar(bounds.address()+20));m.scalar(a+0x1cc,m.scalar(bounds.address()+16));}
}

namespace hp2::debug {
// Debug00191AF8/0018E8B8; E-0099-W47.
std::uint32_t NearbyCarList::GetCarAhead(std::uint32_t car)const{const int index=GetCarsIndexInArray(car);return index<1?0:address_+8+0x1c*(index-1);}
int AICarController::GetMyDriverNumber()const{return m_.signed_half(m_.word(address_)+0xa);}
// Debug0018FAA8; E-0099-W90 corrects cursor to288 (204 is a Car pointer). Original sticky expiry and forward sign scan, using loaded
// signpost globals and actual wrapping distance comparison.
void AICarController::DoStickyDecisions(const ::port::race::AiDriveTargetServices& services){auto& m=m_;const auto a=address_,car=m.word(a),signs=m.word(0x328190);const float around=m.scalar(car+0x7f4);for(unsigned i=0;i<8;++i){const auto sticky=a+0x28c+4*i;if(m.word(sticky)&1){const float distance=TrackPathManager::CompareDistances(m.scalar(0x313308),around,m.scalar(signs+8*m.byte(sticky+1)+4));if(10.f<distance)m.word(sticky,m.word(sticky)&0xfffffffeu);}}float lookahead=(m.scalar(0x327a00)*m.scalar(car+0x3c4))*m.scalar(0x325548);lookahead=100.f<lookahead?lookahead:100.f;lookahead=250.f<lookahead?250.f:lookahead;while(GetStickyIndex()!=-1){const int index=m.signed_word(a+0x288);const auto sign=signs+8*index;const float distance=TrackPathManager::CompareDistances(m.scalar(0x313308),m.scalar(sign+4),around);if(0.f<=distance){if(lookahead<=distance)return;int line=-1;SignPostDecision(sign,line,m.word(a+0x1f8),services);}int next=m.signed_word(a+0x288)+1;if(next>=m.signed_word(0x328194))next=0;m.word(a+0x288,static_cast<std::uint32_t>(next));}}
// Debug00190090. car_to_follow is unused by this source body.
void AICarController::GetCurrentTrackRoutePoint(std::uint32_t car_to_follow,const ::port::race::AiDriveTargetServices& services){(void)car_to_follow;auto& m=m_;const auto a=address_,location=m.word(a+0x1f8),car=m.word(a);if(!DrivingLocation(m,location).IsClear()){const auto initial=m.word(location+4);const float half=m.scalar(a+0xc+0x1c8),lateral=(m.scalar(car+0x60)-m.scalar(initial))*m.scalar(initial+0x10)-(m.scalar(car+0x64)-m.scalar(initial+4))*m.scalar(initial+0xc);if(!(m.scalar(initial+0x1c)<lateral-half)&&-m.scalar(initial+0x18)<=lateral+half){const auto route=m.word(location);auto point=TrackRoute(m,route).FindClosestPointAhead({{m.scalar(car+0x60),m.scalar(car+0x64)}},initial);const bool beyond=point==0;if(beyond)point=route+0x428+0x70*(m.signed_word(route+0x10)-1);DrivingLocation(m,location).SetRoutePoint(point);bool switched=false;const int end=static_cast<std::int32_t>(point-(route+0x428))/0x70;for(int index=static_cast<std::int32_t>(initial-(route+0x428))/0x70;index<=end;++index){const auto current=route+0x428+0x70*index;if(m.signed_byte(current+0x2c)==-1)continue;const auto sign=TrackRoutePoint(m,current).GetSignpost();int line=-1;if(SignPostDecision(sign,line,location,services)){m.byte(a+0x208,DrivingLocation(m,location).IsClear()?0xff:m.byte(m.word(location)+0xa));const auto next_route=Signpost::GetNewRoute(m,sign),next_point=Signpost::GetNewRoutePoint(m,sign);DrivingLocation(m,location).SetRoute(next_route,next_point,line,true);switched=true;break;}(void)Signpost::GetNewRoute(m,sign);}if(!beyond||switched)return;}}
 {const auto table=m.word(a+8),target=m.word(table+0x4c),self=a+static_cast<std::int32_t>(m.signed_half(table+0x48));if(target!=0x190d48)hp2::not_implemented("0x00190090","original FindWhereIAm virtual target is not reconstructed");AICarController(m,self).FindWhereIAm(services);}}
// Debug001904D8. Original action dispatch and driving-line publication. Missing
// alternate action selection remains explicit only when the source requests it.
void AICarController::GeneratePointsToFulfillAction(std::uint32_t action,const ::port::race::AiDriveTargetServices& services){auto& m=m_;const auto a=address_;(void)NearbyCarList(m,m.word(a+0x200)).GetCarAhead(m.word(a));const auto table=m.word(action+0x10),target=m.word(table+0x1c),self=action+static_cast<std::int32_t>(m.signed_half(table+0x18));if(target==0x1c4528)AIActionJustDrive(m,self).CalculatePointsToDrive(a+0xc,services);else if(target==0x1c3f70)AIActionAvoidCars(m,self).CalculatePointsToDrive(a+0xc,services);else if(target==0x1c4f10)AIActionPass(m,self).CalculatePointsToDrive(a+0xc,services);else if(target==0x1c6f90)AIActionIJustGotHit(m,self).CalculatePointsToDrive(a+0xc,services);else if(target==0x1c7460)AIActionAccidentRecovery(m,self).CalculatePointsToDrive(a+0xc,services);else if(target==0x1c5710)AIActionPullOver(m,self).CalculatePointsToDrive(a+0xc,services);else if(target==0x1c65a8)AIActionGetUnstuck(m,self).CalculatePointsToDrive(a+0xc,services);else hp2::not_implemented("0x001904D8","original action CalculatePointsToDrive virtual target is not reconstructed");if(!m.word(action+0x230)){if(const auto current=m.word(a+0x214)){const auto vt=m.word(current+0x10),dtor=m.word(vt+0xc),object=current+static_cast<std::int32_t>(m.signed_half(vt+8));if(dtor==0x1c7818)AIActionJustDrive(m,object).Sub_001C7818(3);else if(dtor==0x1c7a20)AIActionAvoidCars(m,object).Sub_001C7A20(3);else if(dtor==0x1c79a8)AIActionPass(m,object).Sub_001C79A8(3);else if(dtor==0x1c7658)AIActionIJustGotHit(m,object).Sub_001C7658(3);else if(dtor==0x1c75e8)AIActionAccidentRecovery(m,object).Sub_001C75E8(3);else if(dtor==0x1c77a8)AIActionPullOver(m,object).Sub_001C77A8(3);else if(dtor==0x1c76c8)AIActionGetUnstuck(m,object).Sub_001C76C8(3);else hp2::not_implemented("0x001904D8","original alternate action destructor is not reconstructed");}m.word(a+0x214,0);const auto chooser_table=m.word(a+8),chooser=m.word(chooser_table+0x7c),chooser_self=a+static_cast<std::int32_t>(m.signed_half(chooser_table+0x78));if(chooser==0x130388)AIDriver(m,chooser_self).ChooseNewAction(a+0x214,2,services);else if(chooser==0x1905f0)AICarController(m,chooser_self).ChooseNewAction(a+0x214,2);else hp2::not_implemented("0x001904D8","original fallback ChooseNewAction virtual target required");hp2::not_implemented("0x001904D8","original forced-type2 action point generation continuation required");}const auto choice=m.word(a+0x1fc),location=m.word(a+0x1f8);if(DrivingChoice::DrivingChoiceTypeIsDrivingLine(m.signed_word(choice))){const int line=DrivingChoice::GetDrivingLine(m,m.signed_word(choice),m.word(a));if(line!=m.signed_word(location+8)){DrivingLocation(m,location).SetDrivingLine(line,false);HighlightDrivingLine(m,location,GetMyDriverNumber());}}}
}

namespace hp2::debug {
// Debug00190760; E-0099-W47/W92. Full source catchup/boost selection,
// authored frequency, source Player average ratio and signed position operands.
void AICarController::DecideBoostFactor(const ::port::race::AiDriveTargetServices& services){
 auto& m=m_;const auto a=address_,car=m.word(a),racing=m.word(car+0x1c);
 if(!racing||!m.word(a+0x280)||m.word(car+0x18))return;
 const auto action=m.word(a+0x214);const bool allowed=action&&m.word(action+0x244)!=0;
 if(!services.planning)hp2::not_implemented("0x00190760","actual reviewed difficulty resource owner is not bound");
 const float elapsed=static_cast<float>(static_cast<std::int32_t>(m.word(0x327cf8)-m.word(a+0x234)))*m.scalar(0x325550);
 if(elapsed<=services.planning->boost_calculation_frequency(m))return;
 m.word(a+0x234,m.word(0x327cf8));m.word(a+0x284,0);float boost=1.f;
 if(!m.word(0x327ac4)&&m.word(m.word(0x327290))!=8&&m.signed_word(0x311f78+0x30)>0){
  if(!allowed){m.scalar(car+0x3e0,1);return;}
  float distance=0,position=0,scale=1;std::uint32_t player=0,player_racing=0;
  const int players=m.signed_word(0x327780);
  if(players==1){player=m.word(0x327778);player_racing=m.word(m.word(player+4)+0x1c);}
  else if(players==2){
   auto first=m.word(0x327770),second=m.word(0x327774);
   auto front=m.word(m.word(first+4)+0x1c),back=m.word(m.word(second+4)+0x1c);
   if(m.signed_byte(back+10)<m.signed_byte(front+10)){std::swap(first,second);std::swap(front,back);}
   const int half=m.signed_word(0x311f78+0x34)/2;
   int boundary=half<m.signed_byte(front+10)?half:half+1;
   if(m.signed_byte(back+10)<=boundary)++boundary;
   const bool use_back=boundary<m.signed_byte(racing+10);
   player=use_back?second:first;player_racing=use_back?back:front;
  }
  if(player){
   if(players==1){m.word(a+0x284,player);scale=m.scalar(0x311f78+0x2fc+4*m.signed_word(player+0x10));CalculateDistanceAndPositionBoost(racing,player_racing,distance,position,services);}
   else{CalculateDistanceAndPositionBoost(racing,player_racing,distance,position,services);m.word(a+0x284,player);scale=m.scalar(0x311f78+0x2fc+4*m.signed_word(player+0x10));}
  }
  boost=(distance+1.f)+position*scale;
 }
 if(allowed){if(const auto player=m.word(a+0x284)){
  services.planning->require_boost_fields(m);
  const int skill=std::max(0,int(m.signed_byte(m.word(car+0x14)+3)));
  const float slow=m.scalar(0x2e7980+0x184+28*skill);
  if(Player(m,player).CatchupAllowed(slow)){
   const int position=m.signed_byte(m.word(m.word(player+4)+0x1c)+10);const float ratio=Player(m,player).GetSpeedRatio();
   ::port::driving::Memory::Temporary table(m,24);
   hp2::recovered::Table curve(m,table.address());curve.Sub_0018D810((ratio<1.f?0x2e7aa8u:0x2e7a68u)+8*position,2,0,9);
   const float factor=(ratio-1.f)*curve.GetValue(static_cast<float>(skill))+1.f;
   boost=boost*(slow<factor?factor:slow);
  }
 }}
 m.scalar(car+0x3e0,boost);
}
// Debug001910B0; E-0099-W47. Source driver descriptor dispatch, line-speed
// selection and actual World RNG. Boost delegates its explicitly scoped body.
void AICarController::ExecuteCurrentAction(const ::port::race::AiDriveTargetServices& services){auto& m=m_;const auto a=address_,driver=m.word(a+4);auto table=m.word(driver+0x22c);const auto target=m.word(table+0x44),self=driver+static_cast<std::int32_t>(m.signed_half(table+0x40));if(target!=0x13a230)hp2::not_implemented("0x001910B0","original driver SetTarget virtual target is not reconstructed");PsychoDriver(m,self).SetTarget(m.word(a+0x214)+0x20);DecideBoostFactor(services);const auto location=m.word(a+0x1f8);float speed=0,seconds=0;bool allowed=true;if(!DrivingLocation(m,location).IsClear()){const auto action=m.word(a+0x214);if(m.word(0x32711c)){speed=static_cast<float>(std::uint32_t(m.byte(m.word(location+4)+0x5a+m.signed_word(location+8)))<<16)*0x1p-16f;}else{if(!m.word(action+0x23c)){speed=m.scalar(action+0x234);seconds=m.scalar(action+0x238);}else{if(!services.planning)hp2::not_implemented("0x001910B0","actual reviewed difficulty resource owner is not bound");const auto car=m.word(a);const auto skill=m.byte(m.word(car+0x14)+3);const float elapsed=static_cast<float>(static_cast<std::int32_t>(m.word(0x327cf8)-m.word(a+0x218)))*m.scalar(0x325584);if(skill<10&&services.planning->route_point_change_frequency(m)<elapsed){const int variance=services.planning->route_point_variance(m,skill)+1;const auto world=m.word(0x3272f0);m.word(a+0x21c,World(m,world).Random(variance*2)-static_cast<std::uint32_t>(variance));m.word(a+0x218,m.word(0x327cf8));}const auto route=m.word(location);int point=static_cast<std::int32_t>(m.word(location+4)-(route+0x428))/0x70+m.signed_word(a+0x21c);point=point<0?0:point;point=point<m.signed_word(route+0x10)?point:m.signed_word(route+0x10)-1;speed=static_cast<float>(std::uint32_t(m.byte(route+0x428+0x70*point+0x5a+m.signed_word(location+8)))<<16)*0x1p-16f;if(std::fabs(speed)<=m.scalar(0x325588))speed=30;speed=speed*services.planning->speed_factor(m,static_cast<std::int8_t>(skill));if(m.word(0x3121ec)||(m.word(0x326e9c)&&m.word(0x326ec8)))speed=speed*.75f;speed=speed*m.scalar(car+0x3e0);}allowed=m.word(action+0x240)!=0;}}
 table=m.word(driver+0x22c);const auto desired=m.word(table+0x3c),object=driver+static_cast<std::int32_t>(m.signed_half(table+0x38));if(desired!=0x1398a8)hp2::not_implemented("0x001910B0","original driver SetDesiredSpeed virtual target is not reconstructed");PsychoDriver(m,object).SetDesiredSpeed(speed,seconds,allowed);DriveTarget(m,m.word(a+0x214)+0x20).Clear(0,0);}
}

namespace hp2::debug {
// Debug0018EAD8/0018EAE8; E-0099-W47. Timer0/MAX are genuinely inactive;
// ordinary BEQ/BNE unconditional delay slots verified independently in MIPS.
bool AICarController::WeNeedTheBelt()const{return m_.word(0x3279f0)!=0;}
bool AICarController::WeCanHaveTheBelt()const{auto& m=m_;const auto a=address_,driver=m.word(a+4);if(m.signed_byte(driver)<3)return false;const auto coordinator=m.word(0x327290);if(!coordinator)hp2::not_implemented("0x0018EAE8","actual RaceCoordinator is required");if(m.word(coordinator)==8)return false;const auto off=m.word(a+0x238),real=m.word(a+0x23c);if(off!=0&&off!=0x7fffffff)return false;if(real!=0&&real!=0x7fffffff)return false;const auto car=m.word(a);if(m.scalar(car+0x3c0)<5.f||m.signed_word(car+0x3bc)<=0)return false;const auto action=m.word(a+0x214);if(!action)return false;const auto type=m.word(action+0x248);if(type-6u<2u||type==10||type==8||type==9||m.signed_byte(m.word(car+0x14)+3)==-2)return false;const auto world=m.word(0x3272f0);return 120.f<=World(m,world).GetDistanceToClosestPlayer(std::array<float,3>{{m.scalar(car+0x60),m.scalar(car+0x64),m.scalar(car+0x68)}});}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/World.hpp"
namespace hp2::debug {
// Debug0018E430; E-0099-W74; replay-frame/0018e430.json/type-AICarController.json.
void AICarController::DoSnapshot(std::uint32_t snapshot){
 auto& m=m_;const auto a=address_;ReplaySnapshot s(m,snapshot);s.BeginSection(0x2ac968);CarController(m,a).DoSnapshot(snapshot);
 DrivingInformation(m,a+12).DoSnapshot(snapshot);s.MoveData(a+0x210,1);s.MoveData(a+0x208,1);s.MoveData(a+0x20c,4);
 AIAction::DoSnapshot(m,a+0x214,snapshot);s.MoveData(a+0x218,4);s.MoveData(a+0x21c,4,4,1);
 World world(m,m.word(0x3272f0));world.DoPointerSnapshot(snapshot,a+0x204);
 for(unsigned off:{0x234u,0x238u,0x23cu,0x240u})s.MoveData(a+off,4);
 Average(m,m.word(a+0x244)).DoSnapshot(snapshot);Average(m,m.word(a+0x2ac)).DoSnapshot(snapshot);s.MoveData(a+0x2b0,16);
 world.DoPointerSnapshot(snapshot,a+0x24c);for(unsigned off:{0x250u,0x254u,0x258u,0x25cu,0x224u})s.MoveData(a+off,4);
 ::port::driving::Memory::Temporary local(m,1);std::int8_t player=-1;if(const auto p=m.word(a+0x284))player=static_cast<std::int8_t>(m.word(p+0x10));
 m.byte(local.address(),static_cast<std::uint8_t>(player));s.MoveData(local.address(),1);
 if(m.byte(snapshot+0x3d)){player=m.signed_byte(local.address());if(player>=2)hp2::not_implemented("0x0018E430","Restored catchup player exceeds source table");m.word(a+0x284,player<0?0:m.word(0x327770+4*player));}
 s.MoveData(a+0x288,4,4,1);s.MoveData(a+0x280,1,4,0);s.MoveData(a+0x28c,32);s.MoveData(a+0x220,4);s.EndSection();
}
}

// Debug00191B78, E-0099-W74. Original transient nearby cache reset occurs on
// capture OR restore, before both child snapshots; checksum-only does not clear.
namespace hp2::debug {
void DrivingInformation::DoSnapshot(std::uint32_t snapshot){if(m_.byte(snapshot+0x3c)||m_.byte(snapshot+0x3d)){m_.word(address_+4,0);m_.word(address_+0x1c8,0);m_.word(address_+0x1cc,0);}DrivingLocation(m_,address_+0x1d0).DoSnapshot(snapshot);DrivingChoice(m_,address_+0x1e0).DoSnapshot(snapshot);}
}

namespace hp2::debug {
// Debug001905F0; E-0099-W90. Actual JR RA/NOP, no output mutation.
void AICarController::ChooseNewAction(std::uint32_t pp_action,int forced_type){(void)pp_action;(void)forced_type;}
// Debug001905F8; E-0099-W90. Forced type is controller+210, not an omitted
// default argument. Normal interruptions retain only a strictly higher score;
// discarded candidates are destroyed through the original signed descriptor.
std::uint32_t AICarController::InterruptAction(const ::port::race::AiDriveTargetServices& services){
 auto& m=m_;const auto a=address_;const int forced=m.signed_word(a+0x210);
 ::port::driving::Memory::Temporary next(m,4);m.word(next.address(),0);
 const auto table=m.word(a+8),target=m.word(table+0x7c),self=a+std::int32_t(m.signed_half(table+0x78));
 if(target==0x130388)AIDriver(m,self).ChooseNewAction(next.address(),forced,services);
 else if(target==0x1905f0)AICarController(m,self).ChooseNewAction(next.address(),forced);
 else hp2::not_implemented("0x00190630","original ChooseNewAction virtual target is not reconstructed");
 const auto result=m.word(next.address());
 if(forced!=11){m.word(a+0x210,11);return result;}
 if(result){
  const auto current=m.word(a+0x214);
  if(current&&m.signed_word(current+0x24c)<m.signed_word(result+0x24c))return result;
  const auto vtable=m.word(result+0x10),destructor=m.word(vtable+0xc),object=result+std::int32_t(m.signed_half(vtable+8));
  if(destructor==0x1c7818)AIActionJustDrive(m,object).Sub_001C7818(3);
  else if(destructor==0x1c7a20)AIActionAvoidCars(m,object).Sub_001C7A20(3);
  else if(destructor==0x1c79a8)AIActionPass(m,object).Sub_001C79A8(3);else if(destructor==0x1c7658)AIActionIJustGotHit(m,object).Sub_001C7658(3);else if(destructor==0x1c75e8)AIActionAccidentRecovery(m,object).Sub_001C75E8(3);else if(destructor==0x1c77a8)AIActionPullOver(m,object).Sub_001C77A8(3);else if(destructor==0x1c76c8)AIActionGetUnstuck(m,object).Sub_001C76C8(3);
  else if(destructor==0x1c3ba8)AIAction(m,object).Sub_001C3BA8(3);
  else hp2::not_implemented("0x00190678","original discarded AIAction destructor is not reconstructed");
 }
 return 0;
}
}

#include "game_core/indep/src/table.hpp"
namespace hp2::debug {
// Debug00190AB0, E-0099-W92. Individually reviewed original difficulty fields;
// native temporary storage is port-side and source Graph/Linear math is reused.
void AICarController::CalculateDistanceAndPositionBoost(std::uint32_t mine,std::uint32_t player,float& distance,float& position,const ::port::race::AiDriveTargetServices& services){
 auto& m=m_;distance=0;position=0;if(!m.word(0x311f78+0x2f8))return;
 float delta=m.scalar(mine+0x14)-m.scalar(player+0x14);
 if(m.word(0x311f78+0x274)||(m.word(0x326e9c)&&m.word(0x326ec8))){delta=delta+.5f;float integral=static_cast<float>(::port::driving::cvt_w_s(delta));if(delta<integral)integral=integral-1.f;delta=(delta-integral)-.5f;}
 delta=m.scalar(0x313308)*delta;
 if(!services.planning)hp2::not_implemented("0x00190AB0","actual reviewed difficulty resource owner is not bound");services.planning->require_boost_fields(m);
 if(!(delta<=m.scalar(0x2e7ce4)))return;
 ::port::driving::Memory::Temporary graph_lo(m,8),graph_hi(m,8),line_distance(m,8),line_position(m,8);
 hp2::recovered::Graph low(m,graph_lo.address()),high(m,graph_hi.address());low.Sub_0018D9E8(0x2e7980,7);high.Sub_0018D9E8(0x2e79b8,7);
 Linear distance_line(m,line_distance.address());distance_line.Init(0,low.GetValue(delta),9,high.GetValue(delta));
 const int skill=std::max(0,int(m.signed_byte(m.word(m.word(address_)+0x14)+3)));distance=distance_line.GetValue(static_cast<float>(skill));
 const float denominator=((delta*m.scalar(0x325554))*delta)+1.f;
 float correction=((m.scalar(m.word(mine)+0x3c4)-m.scalar(m.word(player)+0x3c4))*m.scalar(0x325558))/denominator;
 correction=correction<m.scalar(0x32555c)?m.scalar(0x32555c):correction;correction=m.scalar(0x325560)<correction?m.scalar(0x325560):correction;distance=distance+correction;
 const int gap=int(m.signed_byte(player+10))-int(m.signed_byte(mine+10)),index=gap+(gap<0?8:7);
 Linear position_line(m,line_position.address());position_line.Init(0,m.scalar(0x2e79f0+4*index),9,m.scalar(0x2e7a30+4*index));position=position_line.GetValue(static_cast<float>(skill));
}
}

namespace hp2::debug {
// Debug00190DE0, E-0099-W93. Actual source conditions and World RNG order.
void AICarController::MaybeHonkAndOrBlinkHeadlights(std::uint32_t other,std::uint32_t nearby){auto& m=m_;const auto a=address_;if(m.word(m.word(other+0xc)+0x538)-1u<2u)return;if(m.word(m.word(0x327290))!=8&&m.word(other+0x20)!=1)return;const auto row=NearbyCarList(m,m.word(a+0x200)).GetNearbyCar(other);const int control=m.signed_word(m.word(a)+0x20);bool blink_allowed=false,horn_allowed=false;if(control==2){if(DoWeCareAboutThisCar(m,row,nearby))blink_allowed=horn_allowed=true;}else if(control==3)blink_allowed=horn_allowed=true;if(!blink_allowed&&!horn_allowed)return;
 World world(m,m.word(0x3272f0));bool honk=false,blink=false;const float velocity=m.scalar(row+0xc);
 if(horn_allowed){float chance=1.f;if(velocity<=50.f){if(m.word(m.word(other+0x14)+8)==3&&velocity< -3.f)chance=1;else if(m.word(other+0x18))chance=.75f;else chance=m.scalar(m.signed_word(nearby+4)>5?0x325568:0x325564);}honk=!(chance<=world.Random(1.f));}
 if(honk&&blink_allowed){const float chance=velocity<=50.f?.5f:m.scalar(0x32556c);blink=!(chance<=world.Random(1.f));}
 const float on=(world.Random(m.scalar(0x325570))+m.scalar(0x325574))-m.scalar(0x325578);
 const float off=(world.Random(m.scalar(0x325578))+m.scalar(0x32557c))-m.scalar(0x325580);
 const bool twice=world.Random(1.f)<.5f;hp2::recovered::Car(m,m.word(a)).HonkAndOrBlinkHeadlights(on,off,blink,honk,twice);
}
}

#include "game_core/indep/inc/CarController.hpp"
namespace hp2::debug {
// Debug00191458,E-0099-W121; actual aicarcontroller.cpp.
void AICarController::ReleaseBoost(){CarController(m_,address_).ReleaseBoost();m_.word(address_+0x280,1);}
}

namespace hp2::debug {
// Debug0018E830,E-0110-T01. Source zero vector, driver virtual dispatch,
// sticky decisions and original loaded signpost index are all preserved.
void AICarController::JustReset(const ::port::race::AiDriveTargetServices& services){auto& m=m_;const auto car=m.word(address_);m.scalar(address_+0x20c,float(m.signed_half(car+8))*m.scalar(0x3254f4));m.vector(address_+0x2b0,{{0,0,0,0}});const auto driver=m.word(address_+4),table=m.word(driver+0x22c);if(m.word(table+0x7c)!=0x139a38)hp2::not_implemented("0x0018E888","original ZeroSteeringMemory virtual target is not reconstructed");PsychoDriver(m,driver+std::int32_t(m.signed_half(table+0x78))).ZeroSteeringMemory();ClearStickyDecisions();if(!services.routes)hp2::not_implemented("0x0018E89C","original signpost routes are required");m.word(address_+0x288,std::uint32_t(Signpost::GetIndex(m,*services.routes,m.scalar(car+0x7f4))));}
}
