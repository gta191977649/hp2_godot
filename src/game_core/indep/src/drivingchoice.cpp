// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\drivingchoice.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): DrivingChoice::BuildDriveTarget() [0x002BE870]
// Diagnostic text (not a callable signature): DrivingChoice::IsDriveable() [0x002BE898]
// Diagnostic text (not a callable signature): DrivingChoice::MakeNewChoice() [0x002BE8B8]
// Diagnostic text (not a callable signature): DrivingChoice::TryChoice() [0x002BE8D8]
// Diagnostic text (not a callable signature): DrivingChoice::FixupCarToExcludeFromCollisionPrediction() [0x002BE8F8]
// Diagnostic text (not a callable signature): DrivingChoice::FillUpDriveTarget [0x002BE938]
// Diagnostic text (not a callable signature): DrivingChoice::GetSpeedBackToLine() [0x002BE960]
// Diagnostic text (not a callable signature): DrivingChoice::BuildDriveTargetAroundCar() [0x002BE988]
// Diagnostic text (not a callable signature): DrivingChoice::MakeDriveTargetParallelToCar() [0x002BE9B8]
#include "game_core/indep/inc/DrivingChoice.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): FuturePoints::BuildArray() [0x002BE9E8]
// Diagnostic text (not a callable signature): FuturePoints::DoSmoothOffsetChange() [0x002BEA08]
#include "game_core/indep/inc/DrivingChoice.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00241CF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00241cf8.md.
void DrivingChoice::SetChoice() {
    hp2::not_implemented("0x00241CF8", "DrivingChoice::SetChoice (signature and full body unresolved)");
}

// ELF 0x00243988; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00243988.md.
void DrivingChoice::ReduceCurrentL() {
    hp2::not_implemented("0x00243988", "DrivingChoice::ReduceCurrentL (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00244768; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00244768.md.
void FuturePoints::AddPoint() {
    hp2::not_implemented("0x00244768", "FuturePoints::AddPoint (signature and full body unresolved)");
}

// ELF 0x002452B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002452b0.md.
void FuturePoints::Dump() {
    hp2::not_implemented("0x002452B0", "FuturePoints::Dump (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/trackroute.hpp"

namespace hp2::recovered {
// ELF 0x002455B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002455b0.md.
void TrackRoutePoint::GetAmountOutOfBounds() {
    hp2::not_implemented("0x002455B0", "TrackRoutePoint::GetAmountOutOfBounds (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug0023ECE8/0023ECD8, E-0099-W37. Unsigned subtraction is deliberate.
bool DrivingChoice::DrivingChoiceTypeIsDrivingLine(int type){return static_cast<unsigned>(type)-1u<4u;}
bool DrivingChoice::DrivingChoiceTypeIsSideOfCar(int type){return static_cast<unsigned>(type)-5u<2u;}
// Debug0023B7B8, E-0099-W37. Signed remainder retained.
int DrivingChoice::GetChoiceType(int line){return line%2+1;}
// Debug0023B7D8, E-0099-W37. Actual signedCarClass+E6, not body-type metadata.
int DrivingChoice::GetDrivingLine(const ::port::driving::Memory& m,int type,std::uint32_t car){return m.signed_byte(m.word(car+0xc)+0xe6)*2+(type-1);}
// Debug0023EDA0, E-0099-W37. Exact inline stores; no full object clear.
void DrivingChoice::Clear(){m_.word(address_+8,0);m_.word(address_,0);m_.word(address_+4,0);}
// Debug0023B8B0, E-0099-W37. Invalid type/car pairing leaves prior choice intact.
void DrivingChoice::SetChoice(int type,std::uint32_t car,bool extra){if((DrivingChoiceTypeIsDrivingLine(type)&&!car)||(DrivingChoiceTypeIsSideOfCar(type)&&car)){m_.word(address_+8,extra?1:0);m_.word(address_,static_cast<std::uint32_t>(type));m_.word(address_+4,car);}}
// Debug0023ECF8, E-0099-W37.
void DrivingChoice::SetChoice(std::uint32_t location){SetChoice(GetChoiceType(m_.signed_word(location+8)),0,false);}
}

#include "game_core/indep/src/trackroute.hpp"
#include "game_core/indep/src/drivinglocation.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/port/race/control_construction.hpp"
#include <cmath>
namespace hp2::debug {
// Debug0023EDD0; E-0099-W37. Original inline clear touches count only.
void FuturePoints::Clear(){m_.word(address_,0);}
// Debug0023E130; E-0099-W37. Original20-byte row and full-word boolean store.
void FuturePoints::AddPoint(int route,int point,int line,float offset,bool crossed){const int n=m_.signed_word(address_);if(n<0||n>=20)hp2::not_implemented("0x0023E130","original FuturePoints array exceeds20 entries");const auto p=address_+4+20*n;m_.word(p,route);m_.word(p+4,point);m_.word(p+8,line);m_.word(address_,n+1);m_.word(p+16,crossed?1:0);m_.scalar(p+12,offset);}
// Debug0023DF50; E-0099-W37. Actual signpost decision followed by exact route
// transition/offset. The FF byte sentinel is confirmed by0023DF9C..DFA8 listing.
bool FuturePoints::AdvancePoint(std::uint32_t& route,std::uint32_t& point,int& index,float& offset,bool& crossed,int& line,std::uint32_t controller,const ::port::race::AiDriveTargetServices& services){auto& m=m_;if(m.byte(point+0x2c)!=0xff){if(!services.routes||!services.routes->signposts)hp2::not_implemented("0x0023DF50","actual signpost resources are not loaded");const auto sign=services.routes->signposts+8*m.byte(point+0x2c);const int previous_line=m.signed_half(point+0x30+2*line);if(AICarController(m,controller).SignPostDecision(sign,line,m.word(controller+0x1f8),services)){if(!services.port_stack_vector_lane)hp2::not_implemented("0x001BE4B8","original temporary vector padding requires an explicit port operand");::port::driving::Memory::Temporary v(m,16);TrackRoutePoint(m,point).GetPosition(v.address(),0,static_cast<float>(previous_line*256)*1.52587890625e-5f,*services.port_stack_vector_lane);route=m.word(0x30bb40+4*m.byte(sign));point=route+0x428+112*m.half(sign+2);index=static_cast<std::int32_t>(point-route-0x428)/112;const float x=m.scalar(point),y=m.scalar(point+4),dx=m.scalar(point+0xc),dy=m.scalar(point+0x10);const int line_offset=m.signed_half(point+0x30+2*line);crossed=true;offset=((m.scalar(v.address())-x)*dy-(m.scalar(v.address()+4)-y)*dx)-static_cast<float>(line_offset*256)*1.52587890625e-5f;return true;}}
 const int old=index;index=old+1;const bool valid=index<m.signed_word(route+0x10);if(valid){offset=0;crossed=false;point=route+0x428+112*(old+1);}return valid;
}
// Debug0023E188; E-0099-W37. Original calibrated authored point traversal,
// lookahead and five-point minimum. It never invents or samples a host path.
void FuturePoints::BuildArray(std::uint32_t target,std::uint32_t location,std::uint32_t nearby,float& initial_l,const ::port::race::AiDriveTargetServices& services){auto& m=m_;Clear();const auto car=m.word(nearby),controller=m.word(car+0x28);auto route=m.word(location),point=m.word(location+4);int line=m.signed_word(location+8);float offset=0;bool crossed=false;int index=DrivingLocation(m,location).IsClear()?-1:static_cast<std::int32_t>(point-route-0x428)/112;const int count=m.signed_byte(target);
 if(count>0){const int number=m.signed_byte(target+0x1a0+count-1);route=number<0?0:m.word(0x30bb40+4*number);if(m.signed_word(m.word(car+0x14)+8)==2&&m.signed_word(route+0xc)==1)route=m.word(location);else{index=m.signed_half(target+0x1b4+2*(count-1));line=m.signed_byte(target+0x1dc+count-1);point=route+0x428+112*index;}do{if(!AdvancePoint(route,point,index,offset,crossed,line,controller,services))return;}while(-10.0f<(m.scalar(target+0x10+16*(count-1))-m.scalar(point))*m.scalar(point+0xc)+(m.scalar(target+0x14+16*(count-1))-m.scalar(point+4))*m.scalar(point+0x10));}
 if(!point)return;if(count==0)initial_l=(m.scalar(car+0x60)-m.scalar(point))*m.scalar(point+0x10)-(m.scalar(car+0x64)-m.scalar(point+4))*m.scalar(point+0xc);else{std::uint32_t previous_route=0,previous_point=0;if(DriveTarget(m,target).GetRouteAndRoutePoint(count-1,previous_route,previous_point))initial_l=(m.scalar(target+0x10+16*(count-1))-m.scalar(previous_point))*m.scalar(previous_point+0x10)-(m.scalar(target+0x14+16*(count-1))-m.scalar(previous_point+4))*m.scalar(previous_point+0xc);else initial_l=(m.scalar(target+0x10+16*(count-1))-m.scalar(point))*m.scalar(point+0x10)-(m.scalar(target+0x14+16*(count-1))-m.scalar(point+4))*m.scalar(point+0xc);}
 initial_l=initial_l-static_cast<float>(m.signed_half(point+0x30+2*m.signed_word(location+8))*256)*1.52587890625e-5f;AddPoint(m.signed_half(route+0xa),static_cast<std::int32_t>(point-route-0x428)/112,line,offset,false);if(m.signed_word(address_)==1){if(!AdvancePoint(route,point,index,offset,crossed,line,controller,services))return;AddPoint(m.signed_half(route+0xa),static_cast<std::int32_t>(point-route-0x428)/112,line,offset,false);}
 const float x=m.scalar(car+0x60)-m.scalar(point),y=m.scalar(car+0x64)-m.scalar(point+4);float distance=std::sqrt(x*x+y*y);const float ahead=CalculateLookAheadDistance(m,car);while((distance<=ahead||m.signed_word(address_)<5)){distance=distance+m.scalar(point+0x14);if(m.signed_word(address_)>=20||!AdvancePoint(route,point,index,offset,crossed,line,controller,services))break;AddPoint(m.signed_half(route+0xa),static_cast<std::int32_t>(point-route-0x428)/112,line,offset,crossed);}
}
// Debug0023D3E0; E-0099-W37. Original grace/traffic/relaxed-switch branches.
float DrivingChoice::GetSpeedBackToLine(::port::driving::Memory& m,std::uint32_t nearby,std::uint32_t location,int movement){if(movement==1)return 0;const auto car=m.word(nearby);if(static_cast<unsigned>(movement)-2u<2u||m.signed_word(car+0x20)==3)return 0.5f;const auto race=m.word(0x327258);if(!race)hp2::not_implemented("0x00149088","actual Race owner is not constructed");if(Race(m,race).GracePeriodIsActive(car))return 0;const float speed=m.scalar(car+0x3c4);if(m.scalar(0x3261bc)<speed){if(m.scalar(0x3261c0)<speed&&!DrivingLocation(m,location).AreWeInARelaxedSwitch())return 0.5f;return 0.25f;}return 0;}
// Debug0023D4B8; E-0099-W37. Original line-relative conservative width clips.
void DrivingChoice::ReduceCurrentL(::port::driving::Memory& m,float& current,float amount,int movement,std::uint32_t point,int line,float half,bool conservative){if(movement==3)current=current+amount;else if(movement==2)current=current-amount;else if(current<0)current=amount< -current?current+amount:0;else current=amount<current?current-amount:0;const float right=conservative?(m.scalar(point+0x1c)-1)-half:m.scalar(point+0x1c),left=conservative?(1-m.scalar(point+0x18))+half:-m.scalar(point+0x18);const float offset=static_cast<float>(m.signed_half(point+0x30+2*line)*256)*1.52587890625e-5f;const float upper=right-offset,lower=left-offset;const float clipped=lower<current?current:lower;current=upper<clipped?upper:clipped;}
// Debug0023D318; E-0099-W37. Real3D bank/elevation geometry, with explicit
// host padding operand in the separate raw TrackRoutePoint.GetPosition overload.
void DrivingChoice::AddToDriveTarget(::port::driving::Memory& m,float lateral,std::uint32_t location,std::uint32_t future,std::uint32_t target,const ::port::race::AiDriveTargetServices& services){(void)location;const int number=m.signed_word(future);const auto route=number<0?0:m.word(0x30bb40+4*number),point=route+0x428+112*m.signed_word(future+4);if(!services.port_stack_vector_lane)hp2::not_implemented("0x001BE4B8","original temporary vector padding requires explicit port operand");::port::driving::Memory::Temporary position(m,16);const float offset=lateral+static_cast<float>(m.signed_half(point+0x30+2*m.signed_word(future+8))*256)*1.52587890625e-5f;TrackRoutePoint(m,point).GetPosition(position.address(),0,offset,*services.port_stack_vector_lane);DriveTarget(m,target).AddPointToTarget(position.address(),static_cast<std::int8_t>(number),static_cast<std::int16_t>(m.signed_word(future+4)),static_cast<std::int8_t>(m.signed_word(future+8)),true,0);}
// Debug0023D098; E-0099-W37. The authored point sequence and original line
// correction/clip order are retained, including Williams tuning mode operand.
void DrivingChoice::FillUpDriveTarget(::port::driving::Memory& m,std::uint32_t target,std::uint32_t location,std::uint32_t nearby,int movement,bool conservative,const ::port::race::AiDriveTargetServices& services){if(!location||!target||!nearby||DrivingLocation(m,location).IsClear()||m.signed_byte(target)>=20)return;m.word(target+0x1f0,m.half(m.word(location+4)+0x2e)&63);::port::driving::Memory::Temporary future(m,404);FuturePoints points(m,future.address());points.Clear();float lateral=0;points.BuildArray(target,location,nearby,lateral,services);float amount=GetSpeedBackToLine(m,nearby,location,movement);const int count=m.signed_word(future.address());std::array<float,20> offsets{};for(int i=0;i<count;++i){const auto row=future.address()+4+20*i;const int number=m.signed_word(row);const auto route=number<0?0:m.word(0x30bb40+4*number),point=route+0x428+112*m.signed_word(row+4);if(movement==0&&m.word(row+16))amount=amount<0.25f?amount:0.25f;ReduceCurrentL(m,lateral,amount,movement,point,m.signed_word(location+8),m.scalar(nearby+0x1c8),conservative);lateral=lateral+m.scalar(row+12);offsets[i]=lateral;}
 for(int i=0;i<count;++i){if(m.word(0x328e08)&&movement==0)offsets[i]=0;AddToDriveTarget(m,offsets[i],location,future.address()+4+20*i,target,services);}::port::driving::Memory::Temporary choice(m,12);DrivingChoice hack(m,choice.address());hack.SetChoice(location);hack.FixupCarToExcludeFromCollisionPrediction(m.word(nearby),false,nearby,services);
}
}

#include "game_core/indep/inc/AICarController.hpp"
#include "game_core/indep/src/carcollisiondetect.hpp"
namespace hp2::debug {
// Debug0023B8F8; E-0099-W37. Original racing-line flags and traffic-route rule.
bool DrivingChoice::IsValid(std::uint32_t location,std::uint32_t nearby){const int type=m_.signed_word(address_);if(DrivingChoiceTypeIsDrivingLine(type)){const auto route=m_.word(location);const int number=DrivingLocation(m_,location).IsClear()?-1:m_.signed_half(route+0xa);if(static_cast<std::uint32_t>(number)<50){const int line=GetDrivingLine(m_,type,m_.word(nearby));return (m_.word(route+0x28+48*line)&3)!=0;}if(static_cast<std::uint32_t>(number-50)<20)return type==1;return false;}if(DrivingChoiceTypeIsSideOfCar(type))return NearbyCarList(m_,nearby).GetNearbyCar(m_.word(address_+4))!=0;return false;}
// Debug0023BA58; E-0099-W37. Original inherited target copy/clear, real authored
// line/around-car target construction and exact exclusion fixup.
bool DrivingChoice::BuildDriveTarget(std::uint32_t target,std::uint32_t location,std::uint32_t nearby,float projected,int movement,const ::port::race::AiDriveTargetServices& services){(void)projected;DriveTarget view(m_,target);view.Sub_002165B8(m_.word(m_.word(nearby)+0x2c));view.Clear(m_.word(nearby),0);bool valid=false;if(IsValid(location,nearby)){valid=true;if(DrivingChoiceTypeIsDrivingLine(m_.signed_word(address_))){::port::driving::Memory::Temporary copy(m_,16);m_.copy(copy.address(),location,16);const int line=GetDrivingLine(m_,m_.signed_word(address_),m_.word(nearby));DrivingLocation(m_,copy.address()).SetDrivingLine(line,false);FillUpDriveTarget(m_,target,copy.address(),nearby,movement,true,services);}else valid=BuildDriveTargetAroundCar(target,nearby,location,services);}else FillUpDriveTarget(m_,target,location,nearby,movement,true,services);FixupCarToExcludeFromCollisionPrediction(m_.word(nearby),false,nearby,services);return valid;}
// Debug0023BBB0; E-0099-W37. Original scoring/prediction and PotentialDriveTargetID
// always-increment rule. No caller-supplied success or zero-time prediction.
bool DrivingChoice::IsDriveable(std::uint32_t target,bool preferred,float& score,std::uint32_t& car_to_hit,float& impact,std::uint32_t location,std::uint32_t nearby,float projected,float maximum,int movement,const ::port::race::AiDriveTargetServices& services){score=-1;car_to_hit=0;impact=m_.scalar(0x326194);bool okay=false;if(BuildDriveTarget(target,location,nearby,projected,movement,services)){score=DriveTarget(m_,target).EstimateScore(m_.word(nearby));if(-1<score){const auto controller=m_.word(m_.word(nearby)+0x28);if(preferred){score=score*m_.scalar(0x326198);m_.word(controller+0x204,m_.word(address_+4));}else m_.word(controller+0x204,0);car_to_hit=CarCollisionPredict(m_,0x328610).WhoWillThisDriveTargetHit(target,nearby,location,maximum,impact,services.port_stack_vector_lane);if(maximum<impact)okay=true;}}const auto world=m_.word(0x3272f0);if(!world)hp2::not_implemented("0x0023BBB0","actual World target-ID owner is not constructed");m_.word(world+0x19c,m_.word(world+0x19c)+1);return okay;}
}

namespace hp2::debug {
// Debug0023EF70; E-0099-W37. Original inline TrackRoute.hpp owner, emitted
// in drivingchoice.cpp. This exact leaf treats zero as the left-width branch.
float TrackRoutePoint::GetAmountOutOfBounds(float lateral)const{return lateral<=0?-(lateral+memory_.scalar(address_+0x18)):lateral-memory_.scalar(address_+0x1c);}
}

#include "game_core/indep/inc/Car.hpp"
#include "game_core/src/bMath.hpp"
namespace hp2::debug {
// Debug0023E748; E-0099-W37. Source distance-based point skipping and strict ahead test.
int FuturePoints::GetClosestPointAhead(const std::array<float,2>& location)const{const auto& m=m_;int chosen=-1;float minimum=m.scalar(0x3261c8);for(int i=0;i<m.signed_word(address_);){const auto row=address_+4+20*i;const int number=m.signed_word(row);const auto route=number<0?0:m.word(0x30bb40+4*number),point=route+0x428+112*m.signed_word(row+4);const float x=m.scalar(point)-location[0],y=m.scalar(point+4)-location[1],distance=std::sqrt(x*x+y*y);if(0< -((location[0]-m.scalar(point))*m.scalar(point+0xc)+(location[1]-m.scalar(point+4))*m.scalar(point+0x10))&&distance<minimum){minimum=distance;chosen=i;}const int skip=static_cast<int>(distance*m.scalar(0x3261cc))-2;i=i+1+(skip>0?skip:0);}return chosen;}
// Debug0023E860; E-0099-W37. Authored-line distance interpolation with original
// end-index boundary and optional conservative road-width clipping.
void FuturePoints::DoSmoothOffsetChange(std::uint32_t target,std::uint32_t nearby,float start,float end,int index,bool conservative,const ::port::race::AiDriveTargetServices& services){auto& m=m_;const int last=index-2;if(index-1<=1)return;const auto point_for=[&](int i){const auto row=address_+4+20*i;const int n=m.signed_word(row);const auto route=n<0?0:m.word(0x30bb40+4*n);return route+0x428+112*m.signed_word(row+4);};const auto line_l=[&](int i,std::uint32_t p){return static_cast<float>(m.signed_half(p+0x30+2*m.signed_word(address_+4+20*i+8))*256)*1.52587890625e-5f;};const auto first=point_for(0),final=point_for(last);const auto from=TrackRoutePoint(m,first).GetPosition(0,line_l(0,first)),to=TrackRoutePoint(m,final).GetPosition(0,line_l(last,final));const float dx=from[0]-to[0],dy=from[1]-to[1],change=end-start;::port::driving::Memory::Temporary position(m,16);for(int i=0;i<=last&&m.signed_byte(target)<20;++i){const auto row=address_+4+20*i,point=point_for(i);const float line=line_l(i,point);const auto xy=TrackRoutePoint(m,point).GetPosition(0,line);const float x=from[0]-xy[0],y=from[1]-xy[1],raw=start+std::sqrt(x*x+y*y)*(change/std::sqrt(dx*dx+dy*dy));const float offset=end>start?(raw<=end?raw:end):(end<=raw?raw:end);float lateral=(offset+line)+m.scalar(row+12);const float low=conservative?(1.f-m.scalar(point+0x18))+m.scalar(nearby+0x1c8):-m.scalar(point+0x18),high=conservative?(m.scalar(point+0x1c)-1.f)-m.scalar(nearby+0x1c8):m.scalar(point+0x1c);lateral=low<lateral?lateral:low;lateral=high<lateral?high:lateral;if(!services.port_stack_vector_lane)hp2::not_implemented("0x001BE4B8","smooth target requires explicit original temporary W operand");TrackRoutePoint(m,point).GetPosition(position.address(),0,lateral,*services.port_stack_vector_lane);DriveTarget(m,target).AddPointToTarget(position.address(),static_cast<std::int8_t>(m.signed_word(row)),static_cast<std::int16_t>(m.signed_word(row+4)),static_cast<std::int8_t>(m.signed_word(row+8)),true,0);}}
// Debug0023DC90; E-0099-W37. Source line-relative gap and traffic/player minima.
float DrivingChoice::GetLateralDistance(std::uint32_t nearby,std::uint32_t around,std::uint32_t future,int index)const{auto& m=m_;if(!around)return 0;const auto mine=m.word(nearby),other=m.word(around+0x18);float gap=0;if(m.signed_word(future)!=0&&(index!=-1||(index=FuturePoints(m,future).GetClosestPointAhead({{m.scalar(other+0x60),m.scalar(other+0x64)}}))!=-1)){const auto first=future+4,row=first+20*index;const auto point_for=[&](std::uint32_t r){const int n=m.signed_word(r);const auto route=n<0?0:m.word(0x30bb40+4*n);return route+0x428+112*m.signed_word(r+4);};const auto p=point_for(first),q=point_for(row);const float a=(((m.scalar(mine+0x60)-m.scalar(p))*m.scalar(p+0x10)-(m.scalar(mine+0x64)-m.scalar(p+4))*m.scalar(p+0xc))-static_cast<float>(m.signed_half(p+0x30+2*m.signed_word(first+8))*256)*1.52587890625e-5f)+m.scalar(first+12),b=(((m.scalar(other+0x60)-m.scalar(q))*m.scalar(q+0x10)-(m.scalar(other+0x64)-m.scalar(q+4))*m.scalar(q+0xc))-static_cast<float>(m.signed_half(q+0x30+2*m.signed_word(row+8))*256)*1.52587890625e-5f)+m.scalar(row+12);gap=(std::fabs(a-b)-m.scalar(nearby+0x1c8))-m.scalar(around+0x10);}float minimum=2;if(m.word(m.word(mine+0x14)+8)!=3){const float stopped=m.scalar(m.word(m.word(other+0x14)+8)==3?0x328e14:0x328e10);if(stopped<std::fabs(m.scalar(other+0x3c4)))minimum=m.word(other+0x18)?1.5f:2.f;else minimum=6;}return minimum<gap?gap:minimum;}
// Debug0023DA68; E-0099-W37. Parallel extension from the actual other driver's target.
void DrivingChoice::MakeDriveTargetParallelToCar(std::uint32_t target,std::uint32_t nearby,float lateral,const ::port::race::AiDriveTargetServices& services){auto& m=m_;const auto other=m.word(address_+4),mine=m.word(nearby);if(!(m.scalar(other+0x3c4)>5&&m.scalar(mine+0x3c4)>5&&m.signed_word(other+0x3bc)>2))return;const auto a=hp2::recovered::bNormalize(std::array<float,2>{{m.scalar(mine+0x70),m.scalar(mine+0x74)}}),b=hp2::recovered::bNormalize(std::array<float,2>{{m.scalar(other+0x70),m.scalar(other+0x74)}});if(!(m.scalar(0x3261c4)<a[0]*b[0]+a[1]*b[1]))return;const auto driver=m.word(other+0x2c);DriveTarget path(m,driver);int index=path.GetFirstPoint(other+0x40);std::uint32_t route=0,point=0;::port::driving::Memory::Temporary position(m,16);while(m.signed_byte(target)<20&&path.GetRouteAndRoutePoint(index,route,point)){const auto original=driver+0x10+16*index;float offset=((m.scalar(original)-m.scalar(point))*m.scalar(point+0x10)-(m.scalar(original+4)-m.scalar(point+4))*m.scalar(point+0xc))+lateral;if(m.signed_word(route+0xc)!=1){const float low=(1.f-m.scalar(point+0x18))+m.scalar(nearby+0x1c8),high=(m.scalar(point+0x1c)-1.f)-m.scalar(nearby+0x1c8);offset=low<offset?offset:low;offset=high<offset?high:offset;}if(!services.port_stack_vector_lane)hp2::not_implemented("0x001BE4B8","parallel target requires explicit original temporary W operand");TrackRoutePoint(m,point).GetPosition(position.address(),0,offset,*services.port_stack_vector_lane);DriveTarget(m,target).AddPointToTarget(position.address(),m.signed_byte(driver+0x1a0+index),m.signed_half(driver+0x1b4+2*index),m.signed_byte(driver+0x1dc+index),true,mine);++index;}}
}
namespace hp2::debug {
// Debug0023D5D8; E-0099-W37. Real nearby-car pass point, authored future
// points, line-relative spacing and original parallel extension conditions.
bool DrivingChoice::BuildDriveTargetAroundCar(std::uint32_t target,std::uint32_t nearby,std::uint32_t location,const ::port::race::AiDriveTargetServices& services){auto& m=m_;::port::driving::Memory::Temporary future(m,404);FuturePoints points(m,future.address());points.Clear();const auto around=NearbyCarList(m,nearby).GetNearbyCar(m.word(address_+4));float initial=0;points.BuildArray(target,location,nearby,initial,services);bool okay=false;if(m.signed_word(future.address())!=0){const auto other=m.word(around+0x18);const int index=points.GetClosestPointAhead({{m.scalar(other+0x60),m.scalar(other+0x64)}});if(index!=-1){float spacing=GetLateralDistance(nearby,around,future.address(),index);if(m.word(address_+8))spacing=spacing+4.f;const auto row=future.address()+4+20*index;const int number=m.signed_word(row),line=m.signed_word(row+8);const auto route=number<0?0:m.word(0x30bb40+4*number),point=route+0x428+112*m.signed_word(row+4);const float x=m.scalar(other+0x60)-m.scalar(point),y=m.scalar(other+0x64)-m.scalar(point+4),dx=m.scalar(point+0xc),dy=m.scalar(point+0x10),other_l=x*dy-y*dx;if(TrackRoutePoint(m,point).GetAmountOutOfBounds(other_l)<5.f){if(m.word(0x328e0c))hp2::not_implemented("0x0023D5D8","active DebugBuildDriveTargetAroundCar FuturePoints Dump is not yet bound");const float half=m.scalar(nearby+0x1c8),separation=(m.signed_word(address_)==5?-1.f:1.f)*(half+m.scalar(around+0x10)+spacing),distance=x*dx+y*dy,low=half-(m.scalar(point+0x18)-1.f),high=(m.scalar(point+0x1c)-1.f)-half;float lateral=(other_l+separation)+m.scalar(row+12);lateral=low<lateral?lateral:low;lateral=high<lateral?high:lateral;const float final_offset=lateral-static_cast<float>(m.signed_half(point+0x30+2*line)*256)*1.52587890625e-5f;if(!services.port_stack_vector_lane)hp2::not_implemented("0x001BE4B8","around-car target requires explicit original temporary W operand");::port::driving::Memory::Temporary beside(m,16);TrackRoutePoint(m,point).GetPosition(beside.address(),distance,lateral,*services.port_stack_vector_lane);points.DoSmoothOffsetChange(target,nearby,initial,final_offset,index,true,services);DriveTarget view(m,target);view.AddPointToTarget(beside.address(),static_cast<std::int8_t>(m.signed_half(route+0xa)),static_cast<std::int16_t>((point-route-0x428)/112),static_cast<std::int8_t>(line),true,0);m.byte(target+0x204,static_cast<std::uint8_t>(m.signed_byte(target)-1));const auto selected=m.word(address_+4);const auto type=m.word(m.word(selected+0x14)+8);if((m.word(selected+0x18)||(type==3&&m.word(m.word(m.word(nearby)+0x14)+8)!=3))&&type!=4)MakeDriveTargetParallelToCar(target,nearby,separation,services);okay=true;}}}FillUpDriveTarget(m,target,location,nearby,1,true,services);return okay;}
}
namespace hp2::debug {
// Debug0023C638; E-0099-W37. Original signed road-width room predicate.
bool DrivingChoice::IsEnoughRoom(std::uint32_t around,std::uint32_t nearby,const ::port::race::AiDriveTargetServices& services)const{const int type=m_.signed_word(address_);if(DrivingChoiceTypeIsDrivingLine(type))return true;if(!DrivingChoiceTypeIsSideOfCar(type))return false;const auto car=m_.word(address_+4);hp2::recovered::Car view(m_,car);if(!(view.GetAmountOutOfBounds()<m_.scalar(0x3261b0)))return true;const auto point=view.GetRoutePoint(services);const float space=(m_.scalar(around+0x10)+m_.scalar(nearby+0x1c8))+5.f,lateral=(m_.scalar(car+0x60)-m_.scalar(point))*m_.scalar(point+0x10)-(m_.scalar(car+0x64)-m_.scalar(point+4))*m_.scalar(point+0xc);return type==5?-m_.scalar(point+0x18)<lateral-space:lateral+space<m_.scalar(point+0x1c);}
// Debug0023CC90; E-0099-W37. Source opposing traffic choice/action rules and
// room test. Rejected preferred exclusions retain the previous pointer.
void DrivingChoice::FixupCarToExcludeFromCollisionPrediction(std::uint32_t car,bool preferred,std::uint32_t nearby,const ::port::race::AiDriveTargetServices& services){auto& m=m_;const int type=m.signed_word(address_);if(!DrivingChoiceTypeIsDrivingLine(type)){if(!DrivingChoiceTypeIsSideOfCar(type))return;if(preferred){const auto other=m.word(address_+4);if(m.word(m.word(other+0x14)+8)==3&&m.scalar(other+0x90)*m.scalar(car+0x90)+m.scalar(other+0x94)*m.scalar(car+0x94)<0){const auto controller=m.word(other+0x28),plan=m.word(controller+0x214),choice=m.word(controller+0x1fc);const int action=plan?m.signed_word(plan+0x248):11,other_type=m.signed_word(choice);if(DrivingChoiceTypeIsSideOfCar(other_type)&&m.word(choice+4)==car&&((other_type==5&&type==6)||(other_type==6&&type==5)))return;if(action==6&&type==5)return;}const auto around=NearbyCarList(m,nearby).GetNearbyCar(other);if(!IsEnoughRoom(around,nearby,services))return;m.word(m.word(car+0x28)+0x204,other);return;}}m.word(m.word(car+0x28)+0x204,0);}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/World.hpp"
namespace hp2::debug {
// Debug0023B740, E-0099-W74. Type moves one byte only; bool memory-size4.
void DrivingChoice::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2b6330);s.MoveData(address_,1);World(m_,m_.word(0x3272f0)).DoCarPointerSnapshot(address_+4,snapshot);s.MoveData(address_+8,1,4,0);s.EndSection();}
}

#include "game_core/indep/src/aiutil.hpp"
namespace hp2::debug {
// Debug0023B9E8, E-0099-W95. Extra-distance flag and padding are source-ignored.
bool DrivingChoice::IsEqualTo(std::uint32_t other)const{if(DrivingChoiceTypeIsSideOfCar(m_.signed_word(address_)))return m_.word(address_+4)==m_.word(other+4)&&m_.word(address_)==m_.word(other);return m_.word(address_)==m_.word(other);}
// Debug0023C488, E-0099-W95. Distinguish successful collision-free target from
// return value (score differs from -1). Source best-preferred is always cleared.
bool DrivingChoice::TryChoice(::port::driving::Memory& m,std::uint32_t choice,bool preferred,std::uint32_t location,std::uint32_t nearby,float maximum,bool& found,std::uint32_t this_target,std::uint32_t best_choice,std::uint32_t best_target,bool& best_preferred,float& best_score,std::uint32_t alternate_choice,std::uint32_t alternate_target,float& alternate_impact,std::uint32_t& hit,float& impact,std::uint32_t critical,const ::port::race::AiDriveTargetServices& services){float score=-1;impact=m.scalar(0x3261a8);hit=0;const bool okay=DrivingChoice(m,choice).IsDriveable(this_target,preferred,score,hit,impact,location,nearby,0,maximum,0,services);if(okay&&best_score<score){found=true;m.copy(best_choice,choice,12);DriveTarget(m,best_target).Sub_002165B8(this_target);best_score=score;best_preferred=false;}const bool valid=m.scalar(0x3261ac)<std::fabs(score- -1.f);if(DrivingChoiceTypeIsSideOfCar(m.signed_word(choice))&&!m.word(choice+8)&&m.word(choice+4)==critical&&preferred){m.copy(alternate_choice,choice,12);DriveTarget(m,alternate_target).Sub_002165B8(this_target);alternate_impact=impact;}return valid;}
// Debug0023CE18, E-0099-W95. Original traffic/curvature/lateral choice and
// final route-edge flip; no invented lane or geometric fallback.
bool DrivingChoice::GetPreferredSideOfCar(std::uint32_t row,std::uint32_t nearby,std::uint32_t location){auto& m=m_;const float curvature=GetCurrentCurvatureOfDrivingLine(m,location);const auto initial=m.word(location+4),other=m.word(row+0x18),mine=m.word(nearby);const int line=m.signed_word(location+8);auto point=TrackRoute(m,m.word(location)).FindClosestPointAhead({{m.scalar(other+0x60),m.scalar(other+0x64)}},initial);if(!point)point=initial;const float lateral=(m.scalar(other+0x60)-m.scalar(point))*m.scalar(point+0x10)-(m.scalar(other+0x64)-m.scalar(point+4))*m.scalar(point+0xc);const float relative=lateral-static_cast<float>(m.signed_half(point+0x30+2*line)*256)*1.52587890625e-5f;const float my_relative=((m.scalar(mine+0x60)-m.scalar(initial))*m.scalar(initial+0x10)-(m.scalar(mine+0x64)-m.scalar(initial+4))*m.scalar(initial+0xc))-static_cast<float>(m.signed_half(initial+0x30+2*line)*256)*1.52587890625e-5f;bool left=true,decided=false;if(m.word(m.word(mine+0x14)+8)==3&&relative<5.f){decided=true;left=false;}if(!decided){if(std::fabs(my_relative)<=1.f&&std::fabs(relative)<=1.f&&50.f<m.scalar(row+4)&&m.scalar(0x3261b8)<=std::fabs(curvature)){left=curvature<=0;decided=true;}if(!decided)left=my_relative<=relative;}const float clearance=(m.scalar(row+0x10)+m.scalar(nearby+0x1c8))+5.f;const bool edge=left?lateral-clearance< -m.scalar(point+0x18):m.scalar(point+0x1c)<lateral+clearance;return edge?!left:left;}
// Debug0023BD28, E-0099-W95. Exact original current/line/car candidate order,
// preferred-side fallback and full target/choice publication; native scratch only.
bool DrivingChoice::MakeNewChoice(std::uint32_t target,std::uint32_t location,std::uint32_t nearby,float maximum,bool& preferred,bool try_current,const ::port::race::AiDriveTargetServices& services){auto& m=m_;::port::driving::Memory::Temporary alternate_target(m,0x210),best_target(m,0x210),this_target(m,0x210),best_choice(m,12),alternate_choice(m,12),this_choice(m,12),current_choice(m,12);for(auto p:{alternate_target.address(),best_target.address(),this_target.address()})DriveTarget(m,p).Sub_00216528();for(auto p:{best_choice.address(),alternate_choice.address(),this_choice.address()})DrivingChoice(m,p).Clear();DrivingChoice current(m,current_choice.address());current.SetChoice(location);if(IsValid(location,nearby))m.copy(current.address(),address_,12);bool found=false,best_preferred=false;float best_score=-1,alternate_impact=m.scalar(0x32619c),impact=m.scalar(0x32619c);std::uint32_t hit=0,critical=0;float initial_score=-1,initial_impact=m.scalar(0x32619c);
 auto attempt=[&](std::uint32_t choice,bool side,std::uint32_t critical_car){return TryChoice(m,choice,side,location,nearby,maximum,found,this_target.address(),best_choice.address(),best_target.address(),best_preferred,best_score,alternate_choice.address(),alternate_target.address(),alternate_impact,hit,impact,critical_car,services);};
 if(try_current){bool side=false;if(DrivingChoiceTypeIsSideOfCar(m.signed_word(current.address()))){if(const auto row=NearbyCarList(m,nearby).GetNearbyCar(m.word(current.address()+4))){const bool left=GetPreferredSideOfCar(row,nearby,location);side=left?m.word(current.address())==5:m.word(current.address())==6;}}(void)attempt(current.address(),side,0);initial_score=best_score;critical=hit;initial_impact=impact;if(side&&m.word(current.address()+4)==hit){m.word(alternate_choice.address()+8,m.word(current.address()+8));DriveTarget(m,alternate_target.address()).Sub_002165B8(this_target.address());}}
 if(!found){for(int line=1;line<=m.signed_word(0x327ac0)&&!found;++line){DrivingChoice trial(m,this_choice.address());trial.SetChoice(line,0,false);if(!try_current||!trial.IsEqualTo(current.address()))(void)attempt(trial.address(),false,critical);}
  if(!found){for(int index=0;index<m.signed_word(nearby+4);++index){const auto row=nearby+8+28*index;if(!DoWeCareAboutThisCar(m,row,nearby))continue;const bool left=GetPreferredSideOfCar(row,nearby,location);DrivingChoice trial(m,this_choice.address());bool valid=false;trial.SetChoice(5,m.word(row+0x18),false);if(!try_current||!trial.IsEqualTo(current.address()))valid=attempt(trial.address(),left,critical);trial.SetChoice(6,m.word(row+0x18),false);if(!try_current||!trial.IsEqualTo(current.address())){const bool right_valid=attempt(trial.address(),!left,critical);valid=valid||right_valid;}const auto other=m.word(row+0x18),mine=m.word(nearby);if(!valid&&m.scalar(other+0x70)*m.scalar(mine+0x70)+m.scalar(other+0x74)*m.scalar(mine+0x74)<0.f){trial.SetChoice(left?5:6,other,true);(void)attempt(trial.address(),true,critical);}}
   if(!found){if(!DrivingChoice(m,alternate_choice.address()).IsValid(location,nearby)||alternate_impact<initial_impact-m.scalar(0x3261a0)){if(m.scalar(0x3261a4)<std::fabs(initial_score- -1.f)){m.copy(address_,current.address(),12);current.BuildDriveTarget(target,location,nearby,0,1,services);preferred=false;}else{SetChoice(location);FillUpDriveTarget(m,target,location,nearby,1,true,services);preferred=false;found=false;}}else{m.copy(address_,alternate_choice.address(),12);DriveTarget(m,target).Sub_002165B8(alternate_target.address());preferred=true;}FixupCarToExcludeFromCollisionPrediction(m.word(nearby),preferred,nearby,services);return found;}
  }
 }
 m.copy(address_,best_choice.address(),12);DriveTarget(m,target).Sub_002165B8(best_target.address());preferred=best_preferred;FixupCarToExcludeFromCollisionPrediction(m.word(nearby),preferred,nearby,services);return found;
}
}

namespace hp2::debug {
// Debug0023C750,E-0110-F10. Original current/line/preferred/opposite candidate
// order; each attempt restores the source copy-constructed initial target.
bool DrivingChoice::TryToDriveAroundCar(std::uint32_t target,bool& preferred,std::uint32_t around,std::uint32_t location,std::uint32_t nearby,float projected,float maximum,bool try_current,const ::port::race::AiDriveTargetServices& services){
 auto& m=m_;const bool left=GetPreferredSideOfCar(around,nearby,location);bool success=false;float score=-1,impact=m.scalar(0x3261b4);std::uint32_t hit=0;
 ::port::driving::Memory::Temporary initial_target(m,0x210),initial_choice(m,12),this_choice(m,12),first_choice(m,12),second_choice(m,12);
 DriveTarget(m,initial_target.address()).Sub_001E3EB8(target);DrivingChoice initial(m,initial_choice.address());initial.Clear();if(m.word(address_))m.copy(initial.address(),address_,12);
 const bool fast=m.scalar(0x328e00)<m.scalar(m.word(around+0x18)+0x3c4)||m.scalar(0x328e00)<m.scalar(m.word(nearby)+0x3c4);
 auto attempt=[&](DrivingChoice& choice,bool side){DriveTarget(m,target).Sub_002165B8(initial_target.address());return choice.IsDriveable(target,side,score,hit,impact,location,nearby,projected,maximum,0,services);};
 if(try_current&&(!DrivingChoiceTypeIsDrivingLine(m.signed_word(initial.address()))||fast)&&initial.IsValid(location,nearby)){
  const int type=m.signed_word(initial.address());if(!DrivingChoiceTypeIsSideOfCar(type)||m.word(initial.address()+4)==m.word(around+0x18)){const bool side=(type==5&&left)||(type==6&&!left);if(attempt(initial,side)){success=true;preferred=side;}}
 }
 DrivingChoice trial(m,this_choice.address());trial.Clear();
 if(!success&&fast&&0<m.signed_word(0x327ac0))for(int line=1;line<=m.signed_word(0x327ac0)&&!success;++line){trial.SetChoice(line,0,false);if((!try_current||!trial.IsEqualTo(initial.address()))&&attempt(trial,false)){m.copy(address_,trial.address(),12);success=true;preferred=false;}}
 if(!success){DrivingChoice first(m,first_choice.address()),second(m,second_choice.address());first.Clear();second.Clear();first.SetChoice(left?5:6,m.word(around+0x18),false);second.SetChoice(left?6:5,m.word(around+0x18),false);
  if((!try_current||!first.IsEqualTo(initial.address()))&&(fast||first.IsEnoughRoom(around,nearby,services))&&attempt(first,true)){m.copy(address_,first.address(),12);success=true;preferred=true;}
  if(!success&&(!try_current||!second.IsEqualTo(initial.address()))&&(fast||second.IsEnoughRoom(around,nearby,services))&&attempt(second,false)){m.copy(address_,second.address(),12);success=true;preferred=false;}
  if(!success){first.Clear();if(initial.IsValid(location,nearby))m.copy(first.address(),initial.address(),12);else first.SetChoice(location);first.BuildDriveTarget(target,location,nearby,projected,1,services);m.copy(address_,first.address(),12);}
 }
 FixupCarToExcludeFromCollisionPrediction(m.word(nearby),preferred,nearby,services);return success;
}
}
