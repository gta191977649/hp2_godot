#include "game_core/indep/src/drivinglocation.hpp"
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/drivetarget.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/ee_rounding.hpp"

namespace hp2::recovered {
// ELF 0x0021BDC8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021bdc8.md.
void DriveTarget::GetLastIndexToCheckForDrivability() {
    hp2::not_implemented("0x0021BDC8", "DriveTarget::GetLastIndexToCheckForDrivability (signature and full body unresolved)");
}

// ELF 0x0021DAC8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021dac8.md.
void DriveTarget::RecalculateLength() {
    hp2::not_implemented("0x0021DAC8", "DriveTarget::RecalculateLength (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include <cmath>
namespace hp2::debug {
// Debug00218850, E-0099-W06. Exact source marker ../indep/src/drivetarget.cpp.
OBB::OBB(::port::driving::Memory& memory,std::uint32_t address){
    memory.word(address+0x4c,0);
}
// Debug00216528, E-0099-W02. Original zero-fill is followed by Clear, yielding last-index -1.
void DriveTarget::Sub_00216528(){
    for(unsigned i=0;i<0x210;++i)memory_.byte(address_+i,0);
    Clear(0,0);
}
// Debug002167C0; E-0099-W02/W37. Original forward discard, interpolation,
// truncation and cache reset. Positive distance is not a replacement path.
void DriveTarget::Clear(std::uint32_t car,float distance){auto& m=memory_;const auto a=address_;bool preserve=false;const int count=m.signed_byte(a);if(count!=0){if(m.scalar(0x325f40)<std::fabs(distance)&&distance>=0&&count>0){int first=0;for(;first<m.signed_byte(a);++first){const float x=m.scalar(a+0x10+16*first)-m.scalar(car+0x60),y=m.scalar(a+0x14+16*first)-m.scalar(car+0x64);if(0<m.scalar(car+0x70)*x+m.scalar(car+0x74)*y)break;}if(first<m.signed_byte(a)){if(first>0)ShiftLeft(first,false);::port::driving::Memory::Temporary point(m,16);const int index=InterpolatePoint(car,distance,point.address());if(index<m.signed_byte(a)-1){if(index>=0){m.byte(a,static_cast<std::uint8_t>(index+1));AddPointToTarget(point.address(),m.signed_byte(a+0x1a0+index),m.signed_half(a+0x1b4+2*index),m.signed_byte(a+0x1dc+index),false,0);RecalculateLength();preserve=true;}}else if(index>=0)preserve=true;if(!preserve){const int picked=GetFirstPoint(car+0x40);const auto line=m.signed_byte(a+0x1dc+picked),route=m.signed_byte(a+0x1a0+picked);const auto number=m.signed_half(a+0x1b4+2*picked);m.byte(a,0);AddPointToTarget(point.address(),route,number,line,false,0);RecalculateLength();preserve=true;}}}if(!preserve)m.byte(a,0);}if(!preserve)m.scalar(a+0x200,0);m.word(a+0x1fc,0);m.byte(a+0x204,255);m.word(a+0x1f8,0);}

}

#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include <array>
namespace hp2::debug {
// Debug00218AD8; E-0099-W37. Original required global initialization.
void Sub_00218AD8(::port::driving::Memory& m){const float d=m.scalar(0x328a80);m.scalar(0x329454,d*d);}
// Debug002165B8; E-0099-W37. Original whole0x210-byte assignment.
void DriveTarget::Sub_002165B8(std::uint32_t source){memory_.copy(address_,source,0x210);}
// Debug002169D8; E-0099-W37. Signed char arithmetic and final signed result.
int DriveTarget::GetLastIndexToCheckForDrivability()const{const int last=memory_.signed_byte(address_+0x204);return static_cast<std::int8_t>((last==-1?memory_.signed_byte(address_)-2:last-1));}
// Debug00217070; E-0099-W37. Original WorldTimer cache and strict dot/distance.
int DriveTarget::GetFirstPoint(std::uint32_t state){auto& m=memory_;const auto a=address_;if(m.word(0x327cf8)!=m.word(a+0x1f8)){const int n=m.signed_byte(a);if(n<1){m.byte(a+0x1f4,0);m.word(a+0x1f8,0);}else{int chosen=n-1;const float threshold=m.scalar(state+0x380)>30?0:m.scalar(0x325f44);const auto velocity=hp2::recovered::bNormalize(std::array<float,2>{{m.scalar(state+0x30),m.scalar(state+0x34)}});for(int i=0;i<n;++i){const std::array<float,2> delta{{m.scalar(a+0x10+16*i)-m.scalar(state+0x20),m.scalar(a+0x14+16*i)-m.scalar(state+0x24)}};const auto direction=hp2::recovered::bNormalize(delta);if(threshold<direction[0]*velocity[0]+direction[1]*velocity[1]&&m.scalar(0x329454)<delta[0]*delta[0]+delta[1]*delta[1]){chosen=i;break;}}m.byte(a+0x1f4,static_cast<std::uint8_t>(chosen));m.word(a+0x1f8,m.word(0x327cf8));}}return m.signed_byte(a+0x1f4);}
// Debug00217ED0; E-0099-W37. Signed speeds, no absolute denominator rewrite.
float DriveTarget::CalculateTime(float distance,float initial,float final)const{if(std::abs(initial)<=memory_.scalar(0x325f5c)&&std::abs(final)<=memory_.scalar(0x325f5c))return memory_.scalar(0x325f60);return distance/((initial+final)*0.5f);}
// Debug00217EC0; E-0099-W37. The original is exactly constant30, not a tire model.
float DriveTarget::GetMaxLateralAcceleration(std::uint32_t)const{return 30;}
// Debug00217D40; E-0099-W37. Original acceleration projection and30-unit cap.
bool DriveTarget::CalculateMaxSpeedCore(std::uint32_t car,const std::array<float,2>& initial,const std::array<float,2>& desired,float distance,float speed,float& final){const float threshold=memory_.scalar(0x325f58);const float time=CalculateTime(distance,speed,final);if(threshold<std::abs(time)){const float ay=(desired[1]-initial[1])*(1.0f/time),ax=(desired[0]-initial[0])*(1.0f/time);const float factor=(initial[0]*ax+initial[1]*ay)/(initial[0]*initial[0]+initial[1]*initial[1]);const float lateral_y=ay-initial[1]*factor,lateral_x=ax-initial[0]*factor;const float acceleration=std::sqrt(lateral_x*lateral_x+lateral_y*lateral_y),maximum=GetMaxLateralAcceleration(car);return !(maximum<=acceleration&&threshold<std::abs(acceleration-maximum));}final=speed;return true;}
// Debug00217B48; E-0099-W37. Original scaled normalization order is preserved.
bool DriveTarget::CalculateMaxSpeed(std::uint32_t car,const std::array<float,2>& point,float time,float& final){const auto state=car+0x40;const auto velocity=hp2::recovered::bNormalize(std::array<float,2>{{point[0]-memory_.scalar(state+0x20),point[1]-memory_.scalar(state+0x24)}},final);return CalculateMaxSpeedCore(car,{{memory_.scalar(state+0x30),memory_.scalar(state+0x34)}},velocity,time*std::abs(memory_.scalar(state+0x380)),memory_.scalar(state+0x384),final);}
// Debug00217BF8; E-0099-W37. Zero initial speed returns without replacing final.
bool DriveTarget::CalculateMaxSpeed(std::uint32_t car,const std::array<float,2>& p1,const std::array<float,2>& p2,const std::array<float,2>& p3,float distance,float initial,float& final){const float threshold=memory_.scalar(0x325f54);if(threshold<std::abs(initial)){if(threshold<std::abs(distance)){const auto a=hp2::recovered::bNormalize(std::array<float,2>{{p2[0]-p1[0],p2[1]-p1[1]}},initial);const auto b=hp2::recovered::bNormalize(std::array<float,2>{{p3[0]-p2[0],p3[1]-p2[1]}},final);return CalculateMaxSpeedCore(car,a,b,distance,initial,final);}final=initial;}return true;}
// Debug00217530; TASK-0099 E-0099-W37. Original ordered point skipping,
// drivability checks and distance-weighted speed score, with no generated path.
float DriveTarget::EstimateScore(std::uint32_t car){auto& m=memory_;const auto a=address_,state=car+0x40;const int n=m.signed_byte(a);if(n==0)return -1;if(n<0||n>20)hp2::not_implemented("0x00217530","original DriveTarget point count outside20-point carrier");const bool single=n==1;std::array<float,20> distances{};const float speed=m.scalar(state+0x384);const auto point=[&](int i){return std::array<float,2>{{m.scalar(a+0x10+16*i),m.scalar(a+0x14+16*i)}};};const auto calc=[&](int i){return m.scalar(a+0x150+4*i);};const auto store=[&](int i,float v){m.scalar(a+0x150+4*i,v);};const auto separation=[](const std::array<float,2>&p,const std::array<float,2>&q){const float x=p[0]-q[0],y=p[1]-q[1];return std::sqrt(x*x+y*y);};const std::array<float,2> position{{m.scalar(state+0x20),m.scalar(state+0x24)}};if(single){store(0,speed);distances[0]=separation(position,point(0));}const int first=GetFirstPoint(state);if(first<0||first>=n)hp2::not_implemented("0x00217530","cached original first-point index outside authored target");if(!single)for(int i=first;i<n;++i){store(i,speed);distances[i]=separation(point(i),i>first?point(i-1):position);}bool can_drive=true;
 if(!single){bool possible=true;float time=CalculateTime(distances[first],speed,calc(first));int current=first;if(time<m.scalar(0x325f48)){int candidate=first;while(candidate<n){current=candidate+1;time=CalculateTime(separation(position,point(candidate+1)),speed,calc(candidate+1));if(m.scalar(0x325f4c)<=time)break;candidate=current;}}
  int next=current+1;if(next<n){float final=calc(current);possible=CalculateMaxSpeed(car,point(current),0.5f,final);store(current,final);}bool finished=!possible;can_drive=possible;const int last=GetLastIndexToCheckForDrivability();if(possible&&next<=last){do{float distance=separation(point(current),point(next));time=CalculateTime(distance,calc(next-1),calc(next));if(time<m.scalar(0x325f50)&&next<n){int speed_index=next,point_index=next+1;const float threshold=m.scalar(0x325f50);while(true){++speed_index;++next;distance=separation(point(current),point(point_index));const float duration=CalculateTime(distance,calc(current),calc(speed_index));if(threshold<=duration||n<=next)break;++point_index;}}
    if(last<next)finished=true;else{float final=calc(next);possible=CalculateMaxSpeed(car,point(current),point(next),point(next+1),distance,calc(current),final);store(next,final);++next;}if(!possible){can_drive=false;finished=true;}if(finished||next>last)break;current=next-1;
   }while(true);}
 }
 if(!can_drive)return -1;float total=distances[first];float score=total*0.5f*(calc(first)+speed);for(int i=first+1;i<n;++i){total=total+distances[i];score=score+distances[i]*0.5f*(calc(i)+calc(i-1));}return total>0?score/total:-1;
}
}

namespace hp2::debug {
// Debug00216EC8; E-0099-W37. Original reject-double-back XYZ dot and16-byte
// point publication. Active my_car predicate is a separate original callee.
void DriveTarget::AddPointToTarget(std::uint32_t position,std::int8_t route,std::int16_t point,std::int8_t line,bool reject,std::uint32_t car){auto& m=memory_;const auto a=address_;const int n=m.signed_byte(a);if(n>=20)return;if(n<0)hp2::not_implemented("0x00216EC8","original negative DriveTarget point index");bool accepted=true;if(reject){if(n<2){if(car)accepted=IsPointAheadOfCar(m,{{m.scalar(position),m.scalar(position+4)}},car,0);}else{const auto previous=a+0x10+16*(n-1),before=previous-16;std::array<float,3> v0{},v1{};for(unsigned i=0;i<3;++i){v0[i]=m.scalar(previous+4*i)-m.scalar(before+4*i);v1[i]=m.scalar(position+4*i)-m.scalar(previous+4*i);}const float dot=(v0[0]*v1[0]+v0[1]*v1[1])+v0[2]*v1[2];if(dot<0)accepted=false;}}if(!accepted)return;m.copy(a+0x10+16*n,position,16);m.byte(a+0x1a0+n,static_cast<std::uint8_t>(route));m.half(a+0x1b4+2*n,static_cast<std::uint16_t>(point));m.byte(a+0x1dc+n,static_cast<std::uint8_t>(line));m.word(a+0x1f8,0);m.word(a+0x1fc,0);m.scalar(a+0x150+4*n,0);if(n>0){const auto prev=a+0x10+16*(n-1);const float x=m.scalar(position)-m.scalar(prev),y=m.scalar(position+4)-m.scalar(prev+4),z=m.scalar(position+8)-m.scalar(prev+8);m.scalar(a+0x200,m.scalar(a+0x200)+std::sqrt((x*x+y*y)+z*z));}m.byte(a,static_cast<std::uint8_t>(n+1));}
}

namespace hp2::debug {
// Debug00218738; E-0099-W37. Preserve original-1 metadata checks; do not add
// route-point bounds/alignment semantics absent from the original predicate.
bool DriveTarget::GetRouteAndRoutePoint(int index,std::uint32_t& route,std::uint32_t& point)const{route=0;point=0;if(index<0||index>=memory_.signed_byte(address_))return false;const int number=memory_.signed_byte(address_+0x1a0+index),n=memory_.signed_half(address_+0x1b4+2*index);if(number==-1||n==-1)return false;const auto r=number<0?0:memory_.word(0x30bb40+4*number);if(!r)return false;const auto p=r+0x428+static_cast<std::uint32_t>(112*n);if(!p)return false;route=r;point=p;return true;}
}

namespace hp2::debug {
// Debug002186D8; E-0099-W37. Ordered XYZ segment sum.
void DriveTarget::RecalculateLength(){auto& m=memory_;const auto a=address_;m.scalar(a+0x200,0);for(int i=1;i<m.signed_byte(a);++i){const auto point=a+0x10+16*i,prior=point-16;const float x=m.scalar(prior)-m.scalar(point),y=m.scalar(prior+4)-m.scalar(point+4),z=m.scalar(prior+8)-m.scalar(point+8);m.scalar(a+0x200,m.scalar(a+0x200)+std::sqrt((x*x+y*y)+z*z));}}
// Debug00218660; E-0099-W37. Retains full supplied vector including W lane;
// unlike AddPoint, only WhichPoint cache is invalidated by the original.
void DriveTarget::SetPoint(int index,std::uint32_t position,std::int8_t route,std::int16_t point,std::int8_t line,float speed){auto& m=memory_;const auto a=address_;if(index<0||index>=m.signed_byte(a))return;m.copy(a+0x10+16*index,position,16);m.byte(a+0x1a0+index,static_cast<std::uint8_t>(route));m.half(a+0x1b4+2*index,static_cast<std::uint16_t>(point));m.byte(a+0x1dc+index,static_cast<std::uint8_t>(line));m.scalar(a+0x150+4*index,speed);m.word(a+0x1f8,0);RecalculateLength();}
// Debug00218168; E-0099-W37. Explicit port W operand for the original
// uninitialized temporary vector padding; no generated steering alternative.
void DriveTarget::BlendWithCurrentAcceleration(std::uint32_t car,std::uint32_t lane){auto& m=memory_;const auto a=address_;if(m.word(a+0x1fc)==m.word(0x327cf8)||m.word(car+0x18))return;m.word(a+0x1fc,m.word(0x327cf8));std::array<float,2> position{{m.scalar(car+0x60),m.scalar(car+0x64)}},velocity{{m.scalar(car+0x70),m.scalar(car+0x74)}},acceleration{{m.scalar(car+0x80),m.scalar(car+0x84)}};const float speed=m.scalar(car+0x3c4);float time=0;const int first=GetFirstPoint(car+0x40);if(!(1.f<=speed)||first>=m.signed_byte(a))return;::port::driving::Memory::Temporary point(m,16);for(int i=first;i<m.signed_byte(a);++i){const auto previous=i==m.signed_byte(a+0x1f4)?car+0x60:a+0x10+16*(i-1),next=a+0x10+16*i;const float x=m.scalar(previous)-m.scalar(next),y=m.scalar(previous+4)-m.scalar(next+4),duration=std::sqrt(x*x+y*y)/speed;time=time+duration;std::array<float,2> next_velocity{},next_position{};for(unsigned k=0;k<2;++k){next_velocity[k]=velocity[k]+acceleration[k]*duration;next_position[k]=position[k]+next_velocity[k]*duration;}const float raw=time*m.scalar(0x325f68),weight=raw<1.f?raw:1.f;m.scalar(point.address(),m.scalar(next)*weight+next_position[0]*(1.f-weight));m.scalar(point.address()+4,m.scalar(next+4)*weight+next_position[1]*(1.f-weight));m.scalar(point.address()+8,0);m.word(point.address()+12,lane);position=next_position;velocity=next_velocity;SetPoint(i,point.address(),m.signed_byte(a+0x1a0+i),m.signed_half(a+0x1b4+2*i),m.signed_byte(a+0x1dc+i),0);if(time>=.75f)break;}}
}

namespace hp2::debug {
// Debug00217F40; E-0099-W37. Source semi-implicit acceleration integration,
// original reversal rejection and20-point bound, not a host motion substitute.
void DriveTarget::FillWithCurrentAcceleration(std::uint32_t car,std::uint32_t lane){auto& m=memory_;const auto a=address_;Clear(0,0);const float maximum=CalculateLookAheadDistance(m,car);std::array<float,2> position{{m.scalar(car+0x60),m.scalar(car+0x64)}},velocity{{m.scalar(car+0x70),m.scalar(car+0x74)}},acceleration{{m.scalar(car+0x80),m.scalar(car+0x84)}};if(std::sqrt(velocity[0]*velocity[0]+velocity[1]*velocity[1])*.75f<std::sqrt(acceleration[0]*acceleration[0]+acceleration[1]*acceleration[1])){StraightLinePredict(m,car,a,lane);return;}float length=0;if(!(length<maximum&&m.signed_byte(a)<20))return;::port::driving::Memory::Temporary point(m,16);do{const float duration=m.scalar(0x325f64);std::array<float,2> next_velocity{},next_position{};for(unsigned k=0;k<2;++k){next_velocity[k]=velocity[k]+acceleration[k]*duration;next_position[k]=position[k]+next_velocity[k]*duration;m.scalar(point.address()+4*k,next_position[k]);}m.scalar(point.address()+8,0);m.word(point.address()+12,lane);AddPointToTarget(point.address(),-1,-1,-1,true,0);const float x=position[0]-next_position[0],y=position[1]-next_position[1];length=length+std::sqrt(x*x+y*y);position=next_position;velocity=next_velocity;if(next_velocity[0]*m.scalar(car+0x70)+next_velocity[1]*m.scalar(car+0x74)<0||maximum<=length)return;}while(m.signed_byte(a)<20);}
}

namespace hp2::debug {
// Debug00216A10; E-0099-W37. Confirmed actual empty function, not a service gap.
void DummyFunction(){}
// Debug00218538; E-0099-W37. num_positions>=NumPoints is genuinely unchanged.
void DriveTarget::ShiftLeft(int positions,bool recalculate){auto& m=memory_;const auto a=address_;if(positions>=m.signed_byte(a))return;for(int i=0;i<m.signed_byte(a)-positions;++i){const int source=i+positions;m.copy(a+0x10+16*i,a+0x10+16*source,16);m.byte(a+0x1a0+i,m.byte(a+0x1a0+source));m.half(a+0x1b4+2*i,m.half(a+0x1b4+2*source));m.byte(a+0x1dc+i,m.byte(a+0x1dc+source));m.word(a+0x150+4*i,m.word(a+0x150+4*source));}m.word(a+0x1f8,0);m.byte(a,static_cast<std::uint8_t>(m.signed_byte(a)-positions));m.word(a+0x1fc,0);if(recalculate)RecalculateLength();}
// Debug00216A18; E-0099-W37. XYZ distances, full XYZW interpolation. Original
// 3D scaled normalizer writes XYZ only, preserving temporary W on extrapolation.
int DriveTarget::InterpolatePoint(std::uint32_t car,float distance,std::uint32_t output){auto& m=memory_;const auto a=address_;const int count=m.signed_byte(a);if(count==0)return -1;if(count<0||count>20)hp2::not_implemented("0x00216A18","original point count outside twenty-point carrier");const auto norm_add=[&](std::uint32_t base,std::uint32_t end,float length){std::array<float,4> delta{};for(unsigned k=0;k<4;++k)delta[k]=m.scalar(end+4*k)-m.scalar(base+4*k);const auto xyz=hp2::recovered::bNormalize(std::array<float,3>{{delta[0],delta[1],delta[2]}},length);for(unsigned k=0;k<3;++k)delta[k]=xyz[k];for(unsigned k=0;k<4;++k)m.scalar(output+4*k,m.scalar(end+4*k)+delta[k]);};
 if(count==1){std::array<float,4> delta{};for(unsigned k=0;k<4;++k)delta[k]=m.scalar(a+0x10+4*k)-m.scalar(car+0x60+4*k);const auto xyz=hp2::recovered::bNormalize(std::array<float,3>{{delta[0],delta[1],delta[2]}},distance);for(unsigned k=0;k<3;++k)delta[k]=xyz[k];for(unsigned k=0;k<4;++k)m.scalar(output+4*k,m.scalar(car+0x60+4*k)+delta[k]);return 0;}
 const int first=GetFirstPoint(car+0x40);if(first<0||first>=count)hp2::not_implemented("0x00216A18","cached original first point outside target carrier");std::array<float,20> distances{};const auto point=[&](int i){return a+0x10+16*i;};const auto separation=[&](std::uint32_t p,std::uint32_t q){const float x=m.scalar(p)-m.scalar(q),y=m.scalar(p+4)-m.scalar(q+4),z=m.scalar(p+8)-m.scalar(q+8);return std::sqrt((x*x+y*y)+z*z);};distances[first]=separation(point(first),car+0x60);int next=first;if(distances[first]<distance){next=first+1;for(;next<count;++next){distances[next]=separation(point(next),point(next-1))+distances[next-1];DummyFunction();if(distance<=distances[next])break;}}
 if(count-1<next){norm_add(point(count-2),point(count-1),distance-distances[count-1]);return count-1;}const int prior=first<next?next-1:-1;const float before=prior>=0?distances[prior]:0;const auto base=prior>=0?point(prior):car+0x60;const float fraction=(distance-before)/(distances[next]-before);for(unsigned k=0;k<4;++k)m.scalar(output+4*k,(m.scalar(point(next)+4*k)-m.scalar(base+4*k))*fraction+m.scalar(base+4*k));return prior;
}
}

namespace hp2::debug {
// Debug002171D8; E-0099-W47. Full XYZW segment interpolation and source
// single-precision stepping. E-0110-A01 adds the source truncating arithmetic
// carrier; six-point original-runtime comparison is bounded, not general
// bit equivalence. Metadata arrays and unknown scratch-tail guard are unchanged.
void DriveTarget::Resample(std::uint32_t car){(void)car;auto& m=memory_;const auto a=address_;const int count=m.signed_byte(a);if(count<=2)return;if(count>20)hp2::not_implemented("0x002171D8","original point count exceeds twenty-point carrier");const ::port::driving::EeTruncationScope rounding("0x002171D8");std::array<std::array<float,4>,20> segments{},output{};std::array<float,20> lengths{};m.scalar(a+0x200,0);for(int i=0;i<count-1;++i){for(unsigned k=0;k<4;++k)segments[i][k]=m.scalar(a+0x10+16*(i+1)+4*k)-m.scalar(a+0x10+16*i+4*k);const auto& d=segments[i];lengths[i]=std::sqrt((d[0]*d[0]+d[1]*d[1])+d[2]*d[2]);m.scalar(a+0x200,m.scalar(a+0x200)+lengths[i]);}segments[count-1]=segments[count-2];lengths[count-1]=lengths[count-2];const float total=m.scalar(a+0x200),spacing=total/static_cast<float>(count-1);float distance=0,within=0,last=0;int segment=0,produced=0;if(total>=0){while(true){if(segment>=count||produced>=20)hp2::not_implemented("0x002171D8","source stepping exceeds bounded original scratch carrier");last=distance;distance=last+spacing;if(!(lengths[segment]>0.f)||!std::isfinite(lengths[segment]))hp2::not_implemented("0x0021737C","exceptional EE segment division needs exact operand semantics");const float fraction=within/lengths[segment];within=within+spacing;for(unsigned k=0;k<4;++k)output[produced][k]=segments[segment][k]*fraction+m.scalar(a+0x10+16*segment+4*k);++produced;if(distance<=total){if(count<=segment)break;if(lengths[segment]<=within){do{within=within-lengths[segment];++segment;if(total<distance)break;if(count<=segment)goto resample_done;}while(lengths[segment]<=within);}}if(count<=segment||total<distance)break;}}
resample_done:m.scalar(a+0x200,last);if(produced<count)hp2::not_implemented("0x00217454","original scratch tail was not initialized by floating-point stepping; explicit prior stack operands required");for(int i=0;i<count;++i)for(unsigned k=0;k<4;++k)m.scalar(a+0x10+16*i+4*k,output[i][k]);m.word(a+0x1fc,0);m.word(a+0x1f8,0);}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug00216660; E-0099-W74; replay-frame/00216660.json/type-DriveTarget.json.
void DriveTarget::DoSnapshot(std::uint32_t snapshot){
 auto& m=memory_;const auto a=address_;ReplaySnapshot s(m,snapshot);s.BeginSection(0x2b43c0);
 s.MoveData(a+0x1f0,1);s.MoveData(a,1);s.MoveData(a+0x1f4,1);
 s.MoveData(a+0x1f8,4);s.MoveData(a+0x1fc,4);s.MoveData(a+0x200,4);s.MoveData(a+0x204,1);
 if(m.signed_byte(a)>20)hp2::not_implemented("0x00216660","DriveTarget point count outside original20-point storage");
 for(int i=0;i<m.signed_byte(a);++i){s.MoveData(a+0x10+16*i,12);s.MoveData(a+0x1b4+2*i,2);
  s.MoveData(a+0x150+4*i,4);s.MoveData(a+0x1a0+i,1);s.MoveData(a+0x1dc+i,1);}
 s.EndSection();
}
}

namespace hp2::debug {
// Debug00216598,E-0099-W119. Source trivial destructor: embedded flags2 do nothing.
void DriveTarget::Sub_00216598(int flags){if(flags&1)memory_.release(address_);}
}

namespace hp2::debug {
// Debug00216CE0,E-0110-F12. Original clear-location return and current location metadata.
void DriveTarget::AddPointToTarget(std::uint32_t position,std::uint32_t location,bool reject,std::uint32_t car){auto& m=memory_;if(DrivingLocation(m,location).IsClear())return;const auto route=m.word(location),point=m.word(location+4);const auto difference=point-route-0x428u;const auto index=static_cast<std::int32_t>(difference*0xb6db6db7u)>>4;AddPointToTarget(position,static_cast<std::int8_t>(m.signed_half(route+0xa)),static_cast<std::int16_t>(index),static_cast<std::int8_t>(m.signed_word(location+8)),reject,car);}
}

// TASK-0100 E-0100-PU02. Original drivetarget.cpp bVector2 wrappers. XYZ
// construction is source-defined; each local W is a distinct unknown operand.
#include "game_core/port/runtime_warnings.hpp"
namespace hp2::debug {
void DriveTarget::SetPoint(int index,const std::array<float,2>& pos,std::int8_t route,std::int16_t point,std::int8_t line,float speed,std::optional<std::uint32_t> w){
 if(!w)hp2::not_implemented("0x00218618","original bVector2 SetPoint localW requires independent operand");
 ::port::diagnostics::warn("0x00218618","original pursuit SetPoint localW is unknown","using independently supplied PC research operand; original stack parity unproved");
 ::port::driving::Memory::Temporary local(memory_,16);memory_.scalar(local.address(),pos[0]);memory_.scalar(local.address()+4,pos[1]);memory_.scalar(local.address()+8,0);memory_.word(local.address()+12,*w);SetPoint(index,local.address(),route,point,line,speed);
}
void DriveTarget::AddPointToTarget(const std::array<float,2>& pos,std::int8_t route,std::int16_t point,std::int8_t line,bool reject,std::uint32_t car,std::optional<std::uint32_t> w){
 if(!w)hp2::not_implemented("0x00216DD8","original bVector2 AddPointToTarget localW requires independent operand");
 ::port::diagnostics::warn("0x00216DD8","original pursuit AddPointToTarget localW is unknown","using independently supplied PC research operand; original stack parity unproved");
 ::port::driving::Memory::Temporary local(memory_,16);memory_.scalar(local.address(),pos[0]);memory_.scalar(local.address()+4,pos[1]);memory_.scalar(local.address()+8,0);memory_.word(local.address()+12,*w);AddPointToTarget(local.address(),route,point,line,reject,car);
}
}
