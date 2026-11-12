// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\realcontroller.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): RealController::Think() [0x002BE128]
// Diagnostic text (not a callable signature): RealController::CalculateLateralPos [0x002BE140]
#include "game_core/RealController.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include "game_core/port/race/ai_drive_target.hpp"
#include <cmath>

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {




















// ELF 0x0023A3D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a3d8.md.
void RealController::Sub_0023A3D8() {
    hp2::not_implemented("0x0023A3D8", "RealController::Sub_0023A3D8 (signature and full body unresolved)");
}

// ELF 0x0023A4D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a4d8.md.
void RealController::Sub_0023A4D8() {
    hp2::not_implemented("0x0023A4D8", "RealController::Sub_0023A4D8 (signature and full body unresolved)");
}

// ELF 0x0023A560; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a560.md.
// TASK-0041 E-0041-V01; debug 0x002342A8: RealController::DoSnapshot(ReplaySnapshot *); retail 0x0023A560.
void RealController::DoSnapshot() {
    hp2::not_implemented("0x0023A560", "RealController::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x0023AA50; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023aa50.md.
// TASK-0041 E-0041-V01; debug 0x00234798: RealController::Think(float); retail 0x0023AA50.
void RealController::Think() {
    hp2::not_implemented("0x0023AA50", "RealController::Think (signature and full body unresolved)");
}
} // namespace hp2::recovered


#include "game_core/port/race/control_construction.hpp"
namespace hp2::debug {
// Debug00234120; E-0099-W24. Only two of four PreviousRoutePoints are initialized.
void RealController::Sub_00234120(std::uint32_t car,const ::port::track::RouteData& routes){
 TrackedController(m_,address_).Sub_00233D68(car,routes);auto& m=m_;const auto a=address_;
 m.word(a+0x24,0);m.word(a+8,0x002b5c40);
 const auto driver=m.allocate(0x280);RealDriver(m,driver).Sub_0013AB98(car);m.word(a+4,driver);
 const auto average=m.allocate(16);Average(m,average).Sub_0018DBF8(8);m.word(a+0x28,average);
 const auto spline=m.allocate(0x28);QuickSpline(m,spline).Sub_00236D88(2,0,4);m.word(a+0x50,spline);QuickSpline(m,spline).SetNumControlPoints(0);
 m.word(a+0x2c,0);for(unsigned i=0;i<2;++i){m.word(a+0x30+i*8,0);m.word(a+0x34+i*8,0);}
}
}


namespace hp2::debug {
// Debug00234220; E-0099-W24. Dispatch only recovered original concrete driver types.
void RealController::Sub_00234220(int flags){
 auto& m=m_;const auto a=address_;m.word(a+8,0x002b5c40);
 if(const auto driver=m.word(a+4)){
  if(m.word(driver+0x22c)!=0x002a6e60)hp2::not_implemented("0x00234220","RealController driver virtual destructor for this dynamic class is not reconstructed");
  RealDriver(m,driver).Sub_0013AC60(3);
 }
 if(const auto average=m.word(a+0x28))Average(m,average).Sub_0018DC70(3);
 if(const auto spline=m.word(a+0x50))QuickSpline(m,spline).Sub_00236E78(3);
 TrackedController(m,a).Sub_00233DB8(flags);
}
// Debug00234380/listing, E-0099-W60. CarClass is the original signed byteE6.
unsigned RealController::PlayerDriveLineNumber()const{return unsigned(int(m_.signed_byte(m_.word(m_.word(address_)+0xc)+0xe6))*2);}
// Debug002349D8, E-0099-W60. Actual previous-route history, authored driving
// offsets, shared QuickSpline and Average. Scratch position W is overwritten
// with0 in the control point before any spline consumer; it is not a policy
// default for target vectors whose original pad remains observable.
bool RealController::CalculateLateralPostionAndVelocityWRTDrivingLine(float& lateral,float& velocity,float elapsed,std::uint32_t route,std::uint32_t point){auto& m=m_;const auto a=address_;const unsigned line=PlayerDriveLineNumber();if(line>=21)hp2::not_implemented("0x002349D8","original player driving-line index exceeds authored offset table");const int previous_count=m.signed_word(a+0x2c);if(previous_count<0||previous_count>4)hp2::not_implemented("0x002349D8","corrupt original four-entry route history");
 const auto previous_at=[&](int i){const auto address=a+0x30+std::uint32_t(i)*8;return NearestRoutePointInfo{m.word(address),m.word(address+4)};};std::array<std::array<float,4>,4> controls{};unsigned used=0;::port::driving::Memory::Temporary temporary(m,16);
 const auto add=[&](const NearestRoutePointInfo& info){const float offset=float(m.signed_half(info.Point+0x30+line*2))*.00390625f;TrackRoutePoint(m,info.Point).GetPosition(temporary.address(),0,offset,0);controls[used]=m.vector(temporary.address());controls[used++][3]=0;};
 for(int i=0;i<previous_count;++i){const auto info=previous_at(i);if(info.IsValid(m))add(info);}
 // With count0 the source deliberately tests bytes28/2C (Average pointer and
 // zero count) as the preceding record; its Point is genuinely null.
 auto next=previous_at(previous_count-1);if(next.IsValid(m))GetNextRoutePointAhead(m,next);else next={route,point};
 while(used<4&&next.IsValid(m)){add(next);GetNextRoutePointAhead(m,next);}
 const auto spline=m.word(a+0x50);if(int(used)<m.signed_byte(spline+0x16)){lateral=0;velocity=m.scalar(m.word(a+0x28)+8);return false;}
 QuickSpline curve(m,spline);curve.SetNumControlPoints(int(used));for(unsigned i=0;i<used;++i){m.vector(m.word(spline+0x20)+i*16,controls[i]);m.byte(spline+0x14,1);}
 const auto car=m.word(a);const float parameter=curve.FindClosestParam(m.vector(car+0x60),0,m.scalar(0x326118));std::array<float,4> position{};curve.GetPoint(position,parameter,0);const float x=position[0]-m.scalar(car+0x60),y=position[1]-m.scalar(car+0x64);lateral=std::sqrt(x*x+y*y);if(m.scalar(car+0xa0)*x+m.scalar(car+0xa4)*y<0)lateral=-lateral;
 if(elapsed==0)hp2::not_implemented("0x002349D8","original lateral-velocity zero time-step division");
 Average(m,m.word(a+0x28)).Record((lateral-m.scalar(a+0x24))/elapsed);velocity=m.scalar(m.word(a+0x28)+8);m.scalar(a+0x24,lateral);
 if(previous_count&&(point==previous_at(previous_count-1).Point||!route))return true;
 int index=previous_count;if(index==4){for(int i=0;i<3;++i){const auto source=previous_at(i+1);m.word(a+0x30+std::uint32_t(i)*8,source.Route);m.word(a+0x34+std::uint32_t(i)*8,source.Point);}index=3;}else m.word(a+0x2c,std::uint32_t(index+1));m.word(a+0x30+std::uint32_t(index)*8,route);m.word(a+0x34+std::uint32_t(index)*8,point);return true;
}
// Debug00234398, E-0099-W60. Exact route delta integration and vehicle-width
// clamping, appending actual point metadata. The target point pad is explicit.
void RealController::RouteDeltaPredict(std::uint32_t target,float velocity,float lateral,std::uint32_t route,std::uint32_t point,std::uint32_t lane){auto& m=m_;const auto car=m.word(address_),line=PlayerDriveLineNumber();if(line>=21)hp2::not_implemented("0x00234398","original player driving-line index exceeds authored offset table");const float lookahead=CalculateLookAheadDistance(m,car),speed=m.scalar(car+0x3c4);::port::driving::Memory::Temporary bounds(m,32);hp2::recovered::Car(m,car).GetCarSpaceBoundingBox(bounds.address(),bounds.address()+16);const float half_width=(m.scalar(bounds.address()+20)-m.scalar(bounds.address()+4))*.5f;float distance=0;if(!(0<lookahead))return;NearestRoutePointInfo next{route,point};::port::driving::Memory::Temporary position(m,16);m.vector(position.address(),m.vector(car+0x60));DriveTarget output(m,target);
 while(m.signed_byte(target)<20){const auto previous=next.Point;GetNextRoutePointAhead(m,next);if(!next.Route||!next.Point){output.Clear(0,0);StraightLinePredict(m,car,target,lane);return;}if(speed==0)hp2::not_implemented("0x00234398","original route-delta zero velocity-magnitude division");const float time=((m.scalar(next.Point)-m.scalar(previous))*m.scalar(previous+0xc)+(m.scalar(next.Point+4)-m.scalar(previous+4))*m.scalar(previous+0x10))/speed;lateral+=velocity*time;velocity=velocity<0?velocity+time:velocity-time;
  const float minimum=half_width-m.scalar(next.Point+0x18),maximum=m.scalar(next.Point+0x1c)-half_width;float offset=float(m.signed_half(next.Point+0x30+line*2))*.00390625f+lateral;if(!(minimum<offset))offset=minimum;if(maximum<offset)offset=maximum;TrackRoutePoint(m,next.Point).GetPosition(position.address(),0,offset,lane);const auto index=std::int32_t(next.Point-next.Route-0x428u)/0x70;output.AddPointToTarget(position.address(),std::int8_t(m.signed_half(next.Route+0xa)),std::int16_t(index),std::int8_t(line),false,0);distance+=m.scalar(previous+0x14);if(lookahead<=distance)return;
 }
}
// Debug00234798/listing, E-0099-W60. Original shared tracked state, genuine
// Player wrong-way gate, route prediction and signed-delta target publication.
void RealController::Think(float elapsed,const ::port::race::AiDriveTargetServices& services){auto& m=m_;const auto a=address_;if(!services.routes)hp2::not_implemented("0x00234798","actual original route resource owner is required");if(!std::isfinite(elapsed))hp2::not_implemented("0x00234798","exceptional original real-controller frame timing");::port::driving::Memory::Temporary target_memory(m,0x210);const auto target=target_memory.address();DriveTarget output(m,target);output.Sub_00216528();TrackedController(m,a).Think(elapsed,*services.routes);
 auto table=m.word(a+8);auto entry=m.word(table+0x34),self=a+std::int32_t(m.signed_half(table+0x30));std::uint32_t route;if(entry==0x233ee0)route=TrackedController(m,self).GetCurrentRoute();else if(entry==0x1368f0)route=0;else hp2::not_implemented("0x00234798","real controller route virtual target is not reconstructed");table=m.word(a+8);entry=m.word(table+0x3c);self=a+std::int32_t(m.signed_half(table+0x38));std::uint32_t point;if(entry==0x234118)point=TrackedController(m,self).GetCurrentRoutePoint();else if(entry==0x1368e8)point=0;else hp2::not_implemented("0x00234798","real controller route-point virtual target is not reconstructed");
 const float minimum=m.scalar(0x326114);if(elapsed<minimum)elapsed=minimum;const auto car=m.word(a),player=m.word(car+0x18);if(!player)hp2::not_implemented("0x00234798","actual Player wrong-way state is not bound");if(!services.port_stack_vector_lane)hp2::not_implemented("0x00234798","original target point uninitialized W requires explicit execution operand");const auto lane=*services.port_stack_vector_lane;bool predicted=false;
 if(!m.word(player+0x2f4)){NearestRoutePointInfo next{route,point};GetNextRoutePointAhead(m,next);float lateral=0,velocity=0;if(route&&point&&CalculateLateralPostionAndVelocityWRTDrivingLine(lateral,velocity,elapsed,route,point)){if(10<=m.scalar(car+0x3c0)){RouteDeltaPredict(target,velocity,lateral,route,point,lane);output.BlendWithCurrentAcceleration(car,lane);}else StraightLinePredict(m,car,target,lane);predicted=true;}}
 if(!predicted){if(10<=m.scalar(car+0x3c0))output.FillWithCurrentAcceleration(car,lane);else StraightLinePredict(m,car,target,lane);}
 const auto driver=m.word(a+4);table=m.word(driver+0x22c);entry=m.word(table+0x44);self=driver+std::int32_t(m.signed_half(table+0x40));if(entry!=0x136c20)hp2::not_implemented("0x00234798","original driver SetTarget virtual target is not reconstructed");DriveTarget(m,self).Sub_002165B8(target);if(m.signed_word(0x327540)==m.signed_half(car+0xa))DriveTargetRenderer(m,0x30bd18).SetTargetToRender(target);if(m.word(0x327a68)&&point)hp2::not_implemented("0x00234798","original speed-difference PrintAbove overlay is not reconstructed");
 // Source DriveTarget destructor00216598 flags2 is instruction-proven empty;
 // Memory::Temporary releases only this port-owned stack operand storage.
}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug002342A8; E-0099-W74; replay-frame/002342a8.json/type-RealController.json.
void RealController::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2b5be8);
 if(m_.byte(snapshot+0x3c)||m_.byte(snapshot+0x3d))QuickSpline(m_,m_.word(address_+0x50)).SetNumControlPoints(0);
 s.MoveData(address_+0x24,4);Average(m_,m_.word(address_+0x28)).DoSnapshot(snapshot);s.MoveData(address_+0x2c,4,4,1);
 if(m_.signed_word(address_+0x2c)>4)hp2::not_implemented("0x002342A8","Snapshot history exceeds original four NRI records");
 for(int i=0;i<m_.signed_word(address_+0x2c);++i)NearestRoutePointInfo::DoSnapshot(m_,address_+0x30+8*i,snapshot);
 TrackedController(m_,address_).DoSnapshot(snapshot);s.EndSection();
}
}
