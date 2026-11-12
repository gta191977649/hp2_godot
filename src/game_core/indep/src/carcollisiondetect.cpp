// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/carcollisiondetect.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001E6010; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e6010.md.
void CarCollisionPredict::IsThisCarStoppedDirectlyInFrontOfMe() {
    hp2::not_implemented("0x001E6010", "CarCollisionPredict::IsThisCarStoppedDirectlyInFrontOfMe (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/drivinglocation.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/inc/World.hpp"
#include <cmath>
namespace hp2::debug {
// Debug001E25C0; TASK-0099 E-0099-W37. Original strict lateral/forward bounds.
bool CarCollisionPredict::IsThisCarStoppedDirectlyInFrontOfMe(std::uint32_t row,std::uint32_t list)const{return std::fabs(m_.scalar(m_.word(row+0x18)+0x3c4))<=5.f&&std::fabs(m_.scalar(row))<m_.scalar(list+0x1c8)+m_.scalar(row+0x10)&&m_.scalar(row+4)<m_.scalar(list+0x1cc)+m_.scalar(row+0x14)+23.f;}
// Debug001E2410; E-0099-W37. Invalid target/location leaves impact unchanged.
// Original rear filtering, stopped/excluded/low-speed and moving best-hit
// branches; missing explicit unknown W operand fails only on the needed path.
std::uint32_t CarCollisionPredict::WhoWillThisDriveTargetHit(std::uint32_t target,std::uint32_t nearby,std::uint32_t location,float maximum,float& impact,std::optional<std::uint32_t> lane){
 if(!location||DrivingLocation(m_,location).IsClear())return 0;
 const int count=m_.signed_byte(target);if(count<=0||count>=21)return 0;
 float best=m_.scalar(0x325bd8);std::uint32_t hit_car=0;
 const auto my=m_.word(nearby),excluded=m_.word(m_.word(my+0x28)+0x204);
 for(int i=0;i<m_.signed_word(nearby+4);++i){const auto row=nearby+8+28*i;if(!DoWeCareAboutThisCar(m_,row,nearby))continue;float time=m_.scalar(0x325bdc);bool hit=IsThisCarStoppedDirectlyInFrontOfMe(row,nearby);if(hit)time=0.f;else if(m_.scalar(0x325be0)<std::fabs(m_.scalar(my+0x3c4))&&m_.word(row+0x18)!=excluded){if(!lane)hp2::not_implemented("0x001E2650","prediction requires explicit original unknown vector-stack W operand");hit=WillIHitThisCar(target,location,my,m_.word(row+0x18),maximum,time,*lane);}if(hit&&time<best){hit_car=m_.word(row+0x18);best=time;}World(m_,m_.word(0x3272f0)).IncrementCollisionPredictionID();}
 impact=best;return hit_car;
}
}

namespace hp2::debug {
// Debug001E3048; E-0099-W37. Inclusive overlap; failed outputs unchanged.
int IntervalOverlap(float a,float b,float c,float d,float& low,float& high){float ray[4]={a,b,c,d};for(int count=3;count>0;--count)for(int i=0;i<count;++i)if(ray[i+1]<ray[i]){const auto temp=ray[i];ray[i]=ray[i+1];ray[i+1]=temp;}if(a<=ray[1]&&ray[1]<=b&&c<=ray[1]&&ray[1]<=d){low=ray[1];high=ray[2];return 1;}return 0;}
// Debug001E37C0; E-0099-W37. Source epsilon and finite sentinels retained.
void TimeIntervalForSeparatingAxis(const ::port::driving::Memory& m,float v,float t,float sum,float& low,float& high){float result=m.scalar(0x325c14);if(std::fabs(v)<m.scalar(0x325c08)){if(sum<std::fabs(t)){low=m.scalar(0x325c0c);high=m.scalar(0x325c0c);return;}low=m.scalar(0x325c10);}else{high=(sum-t)/v;low=-(sum+t)/v;if(v>=0)return;result=low;low=high;}high=result;}
// Debug001E3880; E-0099-W37. Four original 2D OBB separating axes, source
// operation order and overlap handling. Inputs are logical original OBB views.
float WhenWillOverlapOccur2D(const ::port::driving::Memory& m,std::uint32_t a,std::uint32_t b,const std::array<float,2>& t,const std::array<float,2>& v){
 const float a0x=m.scalar(a+0x10),a0y=m.scalar(a+0x14),a1x=m.scalar(a+0x20),a1y=m.scalar(a+0x24),b0x=m.scalar(b+0x10),b0y=m.scalar(b+0x14),b1x=m.scalar(b+0x20),b1y=m.scalar(b+0x24);
 const float r10=a1x*b0x+a1y*b0y,r00=a0x*b0x+a0y*b0y,r11=a1x*b1x+a1y*b1y,r01=a0x*b1x+a0y*b1y,aa0=m.scalar(a+0x40),aa1=m.scalar(a+0x44),ba0=m.scalar(b+0x40),ba1=m.scalar(b+0x44);
 float l1,g1,l2,g2,l3,g3,l4,g4,low,high;
 TimeIntervalForSeparatingAxis(m,v[0]*a1x+v[1]*a1y,t[0]*a1x+t[1]*a1y,std::fabs(aa1)+std::fabs(ba0*r10)+std::fabs(ba1*r11),l1,g1);
 TimeIntervalForSeparatingAxis(m,v[0]*a0x+v[1]*a0y,t[0]*a0x+t[1]*a0y,std::fabs(aa0)+std::fabs(ba0*r00)+std::fabs(ba1*r01),l2,g2);
 TimeIntervalForSeparatingAxis(m,v[0]*b1x+v[1]*b1y,t[0]*b1x+t[1]*b1y,std::fabs(ba1)+std::fabs(aa0*r01)+std::fabs(aa1*r11),l3,g3);
 TimeIntervalForSeparatingAxis(m,v[0]*b0x+v[1]*b0y,t[0]*b0x+t[1]*b0y,std::fabs(ba0)+std::fabs(aa0*r00)+std::fabs(aa1*r10),l4,g4);
 if(!IntervalOverlap(l1,g1,l2,g2,low,high)||!IntervalOverlap(low,high,l3,g3,low,high)||!IntervalOverlap(low,high,l4,g4,low,high)){low=m.scalar(0x325c18);high=m.scalar(0x325c18);}if(low<=0){const bool negative=low<0;low=m.scalar(0x325c1c);if(negative&&high>0)low=0;}return low;
}
}

#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/src/bMath.hpp"
namespace hp2::debug {
// Debug001E2DE8; E-0099-W37. Original dynamic collision body's authored local
// bounds; first angle and subsequent point-to-previous axes are source behavior.
void CarCollisionPredict::MakeOBBsForDriveTarget(std::uint32_t target,std::uint32_t obbs,std::uint32_t car,float exaggeration){auto& m=m_;if(!target)return;const int count=m.signed_byte(target);if(count<=0||!obbs||!car)return;::port::driving::Memory::Temporary bounds(m,32);hp2::recovered::Car(m,car).GetCarSpaceBoundingBox(bounds.address(),bounds.address()+16);std::array<float,3> half{};for(unsigned k=0;k<3;++k)half[k]=std::fabs((m.scalar(bounds.address()+16+4*k)-m.scalar(bounds.address()+4*k))*.5f)*exaggeration;float sine=0,cosine=0;hp2::recovered::bSinCos(m,sine,cosine,m.half(car+0x3a4));for(int i=0;i<count;++i){const auto obb=obbs+80*i,point=target+0x10+16*i;std::array<float,2> direction{{cosine,sine}};if(i)direction=hp2::recovered::bNormalize(std::array<float,2>{{m.scalar(point-16)-m.scalar(point),m.scalar(point-12)-m.scalar(point+4)}});m.scalar(obb+0x10,direction[0]);m.scalar(obb+0x14,direction[1]);m.word(obb+0x18,0);m.word(obb+0x1c,0);m.scalar(obb+0x20,-direction[1]);m.scalar(obb+0x24,direction[0]);m.word(obb+0x28,0);m.word(obb+0x2c,0);m.word(obb+0x30,0);m.word(obb+0x34,0);m.scalar(obb+0x38,1);m.word(obb+0x3c,0);for(unsigned k=0;k<3;++k)m.scalar(obb+0x40+4*k,half[k]);m.copy(obb,point,16);}}
// Debug001E2650; E-0099-W37. Original per-segment swept OBB prediction.
// Scratch storage models original stack objects; their WorldObj stays0, so the
// captured OBB destructor has no WorldObject side effect on this original path.
bool CarCollisionPredict::WillIHitThisCar(std::uint32_t target,std::uint32_t location,std::uint32_t mine,std::uint32_t other,float maximum,float& impact,std::uint32_t lane){(void)location;auto& m=m_;::port::driving::Memory::Temporary storage(m,0x420+3200);const auto my_target=storage.address(),other_target=my_target+0x210,my_obbs=other_target+0x210,other_obbs=my_obbs+1600;DriveTarget a(m,my_target),b(m,other_target);a.Sub_001E3EB8(target);b.Sub_001E3EB8(m.word(other+0x2c));float result=m.scalar(0x325be4);const auto action=[&](std::uint32_t car){const auto plan=m.word(m.word(car+0x28)+0x214);return plan?m.signed_word(plan+0x248):11;};
 if(m.word(m.word(mine+0x14)+8)!=3||hp2::recovered::Car(m,mine).GetAmountOutOfBounds()<=-.25f)a.BlendWithCurrentAcceleration(mine,lane);else a.FillWithCurrentAcceleration(mine,lane);
 if(m.word(m.word(other+0x14)+8)==3){if(hp2::recovered::Car(m,other).GetAmountOutOfBounds()>-.25f)b.FillWithCurrentAcceleration(other,lane);else if(action(other)!=5)b.BlendWithCurrentAcceleration(other,lane);}
 float my_factor=m.word(mine+0x20)==3?m.scalar(0x325be8):1.f,other_factor=m.word(other+0x20)==3?m.scalar(0x325bec):1.f,my_speed=m.scalar(mine+0x3c4),other_speed=m.scalar(other+0x3c4);
 if(m.scalar(mine+0x3c0)<0){const int type=action(mine);if(type==7||type==11||type==10||m.signed_byte(my_target)==0){StraightLinePredict(m,mine,my_target,lane);my_speed=std::fabs(my_speed);my_factor=m.scalar(0x325bf0);}}
 if(m.scalar(other+0x3c0)<0&&(m.word(other+0x20)==2||m.word(other+0x20)==3)){const int type=action(other);if(type==7||type==11||type==10||m.signed_byte(other_target)==0){StraightLinePredict(m,other,other_target,lane);other_speed=std::fabs(other_speed);other_factor=m.scalar(0x325bf4);}}
 const float minimum=m.scalar(0x325bf8);my_speed=minimum<my_speed?my_speed:minimum;other_speed=minimum<other_speed?other_speed:minimum;if(my_speed<2)my_factor=1;if(other_speed<2)other_factor=1;
 for(unsigned i=0;i<20;++i){OBB(m,my_obbs+80*i);OBB(m,other_obbs+80*i);}MakeOBBsForDriveTarget(my_target,my_obbs,mine,my_factor);MakeOBBsForDriveTarget(other_target,other_obbs,other,other_factor);int ai=a.GetFirstPoint(mine+0x40),bi=b.GetFirstPoint(other+0x40);std::array<float,2> ap{{m.scalar(mine+0x60),m.scalar(mine+0x64)}},bp{{m.scalar(other+0x60),m.scalar(other+0x64)}};float elapsed=0;
 while(ai<m.signed_byte(my_target)&&bi<m.signed_byte(other_target)&&elapsed<=maximum){const std::array<float,2> aq{{m.scalar(my_obbs+80*ai),m.scalar(my_obbs+80*ai+4)}},bq{{m.scalar(other_obbs+80*bi),m.scalar(other_obbs+80*bi+4)}};const std::array<float,2> ad{{aq[0]-ap[0],aq[1]-ap[1]}},bd{{bq[0]-bp[0],bq[1]-bp[1]}};const float al=std::sqrt(ad[0]*ad[0]+ad[1]*ad[1]),bl=std::sqrt(bd[0]*bd[0]+bd[1]*bd[1]),at=al/my_speed,bt=bl/other_speed,duration=bt<=at?bt:at;const auto av=hp2::recovered::bNormalize(ad,my_speed),bv=hp2::recovered::bNormalize(bd,other_speed);const float entry=WhenWillOverlapOccur2D(m,my_obbs+80*ai,other_obbs+80*bi,{{bp[0]-ap[0],bp[1]-ap[1]}},{{bv[0]-av[0],bv[1]-av[1]}});if(entry<=duration){result=elapsed+entry;break;}const float ar=(duration*my_speed)/al,br=(duration*other_speed)/bl;for(unsigned k=0;k<2;++k){bp[k]=bp[k]+(bq[k]-bp[k])*br;ap[k]=ap[k]+(aq[k]-ap[k])*ar;}if(m.scalar(0x325bfc)<std::fabs(at-duration))++bi;else ++ai;elapsed=elapsed+duration;}
 impact=result;return !(m.scalar(0x325c00)<=result||std::fabs(result-m.scalar(0x325c00))<=m.scalar(0x325c04));}
}

namespace hp2::debug {
// Debug001E3EB8; E-0099-W37. Original inline copy constructor differs from
// assignment: named fields are copied; object padding remains untouched.
void DriveTarget::Sub_001E3EB8(std::uint32_t source){auto& m=memory_;const auto a=address_;m.byte(a,m.byte(source));for(unsigned i=0;i<20;++i)m.copy(a+0x10+16*i,source+0x10+16*i,16);for(unsigned i=0;i<20;++i)m.word(a+0x150+4*i,m.word(source+0x150+4*i));for(unsigned i=0;i<20;++i)m.byte(a+0x1a0+i,m.byte(source+0x1a0+i));for(unsigned i=0;i<20;++i)m.half(a+0x1b4+2*i,m.half(source+0x1b4+2*i));for(unsigned i=0;i<20;++i)m.byte(a+0x1dc+i,m.byte(source+0x1dc+i));m.word(a+0x1f0,m.word(source+0x1f0));m.byte(a+0x1f4,m.byte(source+0x1f4));m.word(a+0x1f8,m.word(source+0x1f8));m.word(a+0x1fc,m.word(source+0x1fc));m.word(a+0x200,m.word(source+0x200));m.byte(a+0x204,m.byte(source+0x204));}
}
