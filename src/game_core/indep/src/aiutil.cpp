#include "game_core/indep/src/BeltMover.hpp"
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/aiutil.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include <cmath>

namespace hp2::debug {
// Debug001C7D98; E-0100-CM01; exact four original WheelTouchingGround bytes.
int GetNumWheelsOnGround(const ::port::driving::Memory& m,std::uint32_t car){int count=0;for(unsigned i=0;i<4;++i)if(m.byte(car+0x350+i))++count;return count;}

// Debug001C87B8, E-0099-W17. Traversal uses the real original intrusive lists,
// not numeric route order. DontTake and traffic-direction filters are original.
bool GetNearestRoutePoint(::port::driving::Memory& memory,const ::port::track::RouteData& data,NearestRoutePointInfo& output,
    const std::array<float,2>& position,RoutesToSearch type,int flags,float dat){
    const bool filter=type!=ALL_RACING_ROUTES&&type!=ALL_TRAFFIC_ROUTES;
    bool forward=memory.signed_word(0x00311f78+0xc)==0;
    if(type==ONCOMING_TRAFFIC_ROUTES)forward=!forward;
    const auto head=std::uint32_t(0x0030bb30+(type==ALL_RACING_ROUTES?0:8));
    std::uint32_t chosen_route=0,chosen_point=0;float maximum=memory.scalar(0x00325a50);
    for(auto route=memory.word(head);route!=head;route=memory.word(route)){
        if(filter&&((memory.signed_half(route+0xa)<60)!=forward))continue;
        if(memory.word(route+0x18)&4)continue;
        const auto point=TrackRoute(memory,route).FindClosestPoint(position,maximum,flags,dat,data.paths.track_length);
        if(!point)continue;
        float distance;
        if(flags&0x20)distance=TrackRoutePoint(memory,point).GetDistanceAway(position,dat,data.paths.track_length);
        else{const auto x=memory.scalar(point)-position[0],y=memory.scalar(point+4)-position[1];distance=std::sqrt(x*x+y*y);}
        if(maximum<=distance)continue;
        chosen_route=route;chosen_point=point;maximum=distance;
    }
    if(!chosen_route||!chosen_point)return false;
    output.Route=chosen_route;output.Point=chosen_point;return true;
}
}

namespace hp2::recovered {
// ELF 0x001CD408; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001cd408.md.
void NearestRoutePointInfo::IsValid() {
    hp2::not_implemented("0x001CD408", "NearestRoutePointInfo::IsValid (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/src/bMath.hpp"
namespace hp2::debug {
// Debug001C7D78; TASK-0099 E-0099-W37. Original runtime tuning operands.
float CalculateLookAheadDistance(const ::port::driving::Memory& m,std::uint32_t car){const float value=m.scalar(car+0x3c4)*m.scalar(0x327a00),minimum=m.scalar(0x327a04);return minimum<value?value:minimum;}
// Debug001C7F88; E-0099-W37. Velocity-based normalized strict dot test.
bool IsPointAheadOfCar(const ::port::driving::Memory& m,const std::array<float,2>& point,std::uint32_t car,float cosine){const auto v=hp2::recovered::bNormalize(std::array<float,2>{{m.scalar(car+0x70),m.scalar(car+0x74)}});const auto p=hp2::recovered::bNormalize(std::array<float,2>{{point[0]-m.scalar(car+0x60),point[1]-m.scalar(car+0x64)}});return cosine<v[0]*p[0]+v[1]*p[1];}
}

#include "game_core/indep/src/drivinglocation.hpp"
namespace hp2::debug {
// Debug001C8468; E-0099-W37. Original conditional debug-global publication.
void HighlightDrivingLine(::port::driving::Memory& m,std::uint32_t location,int driver){if(m.signed_word(0x327540)==driver){m.word(0x3281cc,0xffffffff);if(!DrivingLocation(m,location).IsClear())m.word(0x3281cc,static_cast<std::uint32_t>(m.signed_half(m.word(location)+0xa)));m.word(0x3281d0,m.word(location+8));}}
// Debug001C9DB8; E-0099-W37. Strict rear overlap test; player margin is -0.5.
bool DoWeCareAboutThisCar(const ::port::driving::Memory& m,std::uint32_t car,std::uint32_t list){if(!car||m.word(car+0x18)==m.word(list))return false;const float margin=m.word(m.word(car+0x18)+0x18)?-.5f:0.f;return -(m.scalar(car+0x14)+m.scalar(list+0x1cc)+margin)<m.scalar(car+4);}
}

#include "game_core/indep/src/drivetarget.hpp"
#include <cstring>
namespace hp2::debug {
// Debug001CA4A8; E-0099-W37. Listing confirms normalization sum XYZ and
// scaled publication XYZW. Slow-branch norm_vel.W is an explicit port operand.
void StraightLinePredict(::port::driving::Memory& m,std::uint32_t car,std::uint32_t target,std::uint32_t lane){const float distance=CalculateLookAheadDistance(m,car);std::array<float,4> direction{};if(m.scalar(car+0x3c4)<1.f){hp2::recovered::bSinCos(m,direction[1],direction[0],m.half(car+0x3a4));direction[2]=0;std::memcpy(&direction[3],&lane,4);}else{const float x=m.scalar(car+0x70),y=m.scalar(car+0x74),z=m.scalar(car+0x78),inverse=1.f/std::sqrt((x*x+y*y)+z*z);for(unsigned k=0;k<4;++k)direction[k]=m.scalar(car+0x70+4*k)*inverse;}::port::driving::Memory::Temporary point(m,16);for(unsigned k=0;k<4;++k)m.scalar(point.address()+4*k,direction[k]*distance+m.scalar(car+0x60+4*k));DriveTarget view(m,target);view.Clear(0,0);view.AddPointToTarget(point.address(),-1,-1,-1,false,0);}
}

namespace hp2::debug {
// Debug001CA118; E-0099-W47. Original simultaneous trig and minimum5 distance.
// Unknown temporary vector pad is intentionally absent from this XYZ projection.
std::array<float,3> SteerStraightVector3D(::port::driving::Memory& m,std::uint32_t car,float distance){float sine=0,cosine=0;hp2::recovered::bSinCos(m,sine,cosine,m.half(car+0x3a4));const float length=distance<5.f?5.f:distance;return {{cosine*length+m.scalar(car+0x60),sine*length+m.scalar(car+0x64),m.scalar(car+0x68)}};}
// Debug001CA1A8; E-0099-W47. Reuses actual XYZ scaled-normalize00251D28;
// original destination pad is unknown and is not consumed by the caller.
std::array<float,3> SteerStraightVelocity3D(::port::driving::Memory& m,std::uint32_t car,float distance){const auto v=hp2::recovered::bNormalize(std::array<float,3>{{m.scalar(car+0x70),m.scalar(car+0x74),m.scalar(car+0x78)}},distance<5.f?5.f:distance);return {{m.scalar(car+0x60)+v[0],m.scalar(car+0x64)+v[1],m.scalar(car+0x68)+v[2]}};}
}

#include "game_core/indep/src/carinfo.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/table.hpp"
#include "game_core/indep/inc/Car.hpp"
namespace hp2::debug {
// Debug001C9E20; E-0099-W47. Actual authored acceleration table and reviewed
// CarTypeInfo Body.Mass/Aero.Drag/Control.SpeedLimiter; no alternate force model.
float CalculateMaxAcceleration(::port::driving::Memory& m,std::uint32_t car,float speed){const auto cfg=m.word(car+0xc);const float scaled=speed/m.scalar(car+0x3e0);float acceleration=hp2::recovered::Table(m,GetAccelerationTable(m.signed_word(m.word(car+0x10)))).GetValue(scaled);const float mass=m.scalar(cfg+0x1e0),drag=((1.f-m.scalar(car+0x458))*m.scalar(cfg+0x2f0)*scaled)*scaled;if(m.scalar(cfg+0x320)*m.scalar(0x325a54)<=scaled&&drag<acceleration)acceleration=drag;float gravity=0;if(m.word(car+0x24)==6)gravity=BeltMover(m,m.word(car+0x30)).GetAccelerationDueToGravity();else{const float x=m.scalar(car+0x90),y=m.scalar(car+0x94),z=m.scalar(car+0x98);const float square=(x*x+y*y)+z*z;if(!(square>0.f)||!std::isfinite(square))hp2::not_implemented("0x001C9E20","exceptional EE reciprocal-square-root input needs exact operand semantics");gravity=-m.scalar(0x327ba0)*(z*(1.f/std::sqrt(square)));}acceleration=acceleration*m.scalar(0x328860);return (acceleration-drag/mass)+gravity;}
// Debug001C9FA8; E-0099-W47. Source calls but discards negative-max fallback.
bool CanICatchThisCar(::port::driving::Memory& m,std::uint32_t nearby,std::uint32_t car,float maximum,bool debug_print){(void)debug_print;float acceleration=hp2::recovered::Car(m,car).GetForwardAcceleration();if(maximum<0)(void)CalculateMaxAcceleration(m,car,m.scalar(car+0x3c4));const auto other=m.word(nearby+0x18);const float other_acceleration=hp2::recovered::Car(m,other).GetForwardAcceleration();if(0.f<m.scalar(car+0x90)*m.scalar(other+0x90)+m.scalar(car+0x94)*m.scalar(other+0x94))acceleration=acceleration-other_acceleration;else acceleration=acceleration+other_acceleration;const float x=m.scalar(nearby),y=m.scalar(nearby+4),vx=m.scalar(nearby+8),vy=m.scalar(nearby+12),time=m.scalar(0x328264);return std::sqrt(x*x+y*y)<std::sqrt(vx*vx+vy*vy)*time+((acceleration*time)*time)*.5f;}
}

namespace hp2::debug {
// Debug001C8618; E-0099-W47. This exact source branch is genuinely empty.
// Marker creation/destruction/transforms remain explicit active dependencies.
void DriveTargetRenderer::SetTargetToRender(std::uint32_t target){(void)target;if(!m_.word(0x328288)){if(!m_.word(address_))return;hp2::not_implemented("0x001C8618","original DestoryTargetMarkers dependency is not reconstructed");}hp2::not_implemented("0x001C8618","original live target marker construction/transform dependencies are not reconstructed");}
}

namespace hp2::debug {
// Debug001CA3D8; E-0099-W56. Preserve the original inverse-multiply and
// arithmetic-shift point-index conversion even for an unaligned explicit operand.
bool NearestRoutePointInfo::IsValid(const ::port::driving::Memory& m)const{if(!Route||!Point||static_cast<std::uint32_t>(static_cast<std::int32_t>(m.signed_half(Route+0xa)))>=70u)return false;const std::uint32_t bits=(Point-Route-0x428u)*0xb6db6db7u;std::int32_t product=0;std::memcpy(&product,&bits,4);const int index=product>=0?product/16:~((~product)/16);return index>=0&&index<m.signed_word(Route+0x10);}
// Debug001C8978/listing, E-0099-W60. SignNumber is the source byte2C; its
// sentinel is FF, not a four-byte comparison over adjacent route metadata.
void GetNextRoutePoint(::port::driving::Memory& m,NearestRoutePointInfo& output){auto route=output.Route,point=output.Point;if(m.byte(point+0x2c)!=255){const auto sign=TrackRoutePoint(m,point).GetSignpost(),flags=m.word(sign);if(!(flags&0x800)&&((flags&0x700)==0x200||!(flags&0x700))){output.Route=Signpost::GetNewRoute(m,sign);output.Point=Signpost::GetNewRoutePoint(m,sign);return;}point=output.Point;}const int next=std::int32_t(point-route-0x428u)/0x70+1;output.Point=next<m.signed_word(route+0x10)?route+0x428+std::uint32_t(next)*0x70:0;}
// Debug001C8A88, E-0099-W60. Genuine sign-relative backwards/near-point
// skips operate on the same output route, preserving source endpoint behavior.
void GetNextRoutePointAhead(::port::driving::Memory& m,NearestRoutePointInfo& output){const auto previous=output.Point;if(!previous||!output.Route)return;GetNextRoutePoint(m,output);if(m.byte(previous+0x2c)==255||!output.Point)return;const auto next=output.Point;if((m.scalar(next)-m.scalar(previous))*m.scalar(previous+0xc)+(m.scalar(next+4)-m.scalar(previous+4))*m.scalar(previous+0x10)<0){GetNextRoutePoint(m,output);const float x=m.scalar(previous)-m.scalar(output.Point),y=m.scalar(previous+4)-m.scalar(output.Point+4);if(std::sqrt(x*x+y*y)<8)GetNextRoutePoint(m,output);}}
}

namespace hp2::debug {
// Debug001C8208, E-0099-W61. Original half-open Y intervals and strict X
// crossing parity. Zero/negative counts genuinely return0; no geometry epsilon.
std::int64_t IsPointInPoly(::port::driving::Memory& m,const std::array<float,2>& point,std::uint32_t points,std::int64_t count){
 if(count>0x7fffffff)hp2::not_implemented("0x001C8208","original polygon index conversion outside supported addressable extent");
 if(count<=0)return 0;
 std::int64_t previous=count-1;std::uint32_t inside=0;
 for(std::int64_t i=0;i<count;++i){const auto current=points+8*std::uint32_t(i),prior=points+8*std::uint32_t(previous);const float y=m.scalar(current+4),py=m.scalar(prior+4);
  if((y<=point[1]&&point[1]<py)||(py<=point[1]&&point[1]<y)){const float x=m.scalar(current),crossing=((m.scalar(prior)-x)*(point[1]-y))/(py-y)+x;if(point[0]<crossing)inside^=1;}
  previous=i;
 }
 return inside;
}
}

namespace hp2::debug {
// Debug001CA210, E-0099-W62. Actual World category bounds and RacingCar ranks;
// player styles are raw signed bytes, not normalized enum substitutes.
void SetAIDriverModesFromPlayers(::port::driving::Memory& m){
 const auto second=m.word(0x327774),first=m.word(0x327770),world=m.word(0x3272f0),race=m.word(0x327258);
 if(!world||!race)return;
 const auto find=[&](std::uint32_t car){return m.word(car+0x1c);}; //GetRacingCar00148F30.
 int front=0,rear=0,split=0;
 if(m.signed_word(0x311f78+0x30)==2){
  const int rank0=m.signed_byte(find(m.word(first+4))+0xa),rank1=m.signed_byte(find(m.word(second+4))+0xa);
  front=m.signed_byte(first+0x1e);rear=m.signed_byte(second+0x1e);
  if(rank1<rank0)std::swap(front,rear);
  split=m.signed_word(0x311f78+0x34)/2;if(rank0<=split)++split;if(rank1<=split)++split;
 }else front=m.signed_byte(m.word(0x327778)+0x1e);
 const int start=m.signed_word(world+0x20),count=m.signed_word(world+0x24)-start;
 for(int i=0;i<count;++i){const auto car=m.word(world+0x2c+4*(start+i));int style=front;if(m.signed_word(0x311f78+0x30)==2&&split<m.signed_byte(find(car)+0xa))style=rear;m.word(car+0x3dc,static_cast<std::uint32_t>(style));}
}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/src/trackroute.hpp"
namespace hp2::debug {
// Debug001CA430; E-0099-W74; actual inline wrappers delegate original pointer codecs.
void NearestRoutePointInfo::DoSnapshot(::port::driving::Memory& m,std::uint32_t a,std::uint32_t snapshot){
 ReplaySnapshot s(m,snapshot);s.BeginSection(0x2afb28);TrackRouteManager manager(m,0x30bb30);
 manager.DoPointerSnapshot(snapshot,::port::race::SnapshotRoutePointer{a});
 manager.DoPointerSnapshot(snapshot,::port::race::SnapshotRoutePointPointer{a+4},m.word(a));s.EndSection();
}
}

#include "game_core/indep/inc/WorldObject.hpp"
namespace hp2::debug {
// Debug001C85B0, E-0099-W89. MIPS is a fixed20-pointer array, not the
// decompiler's erroneous pointer chain. Inactive flag leaves all slots intact.
void DriveTargetRenderer::DestoryTargetMarkers(){
 if(!m_.word(address_))return;
 for(unsigned i=0;i<20;++i){const auto slot=address_+4+4*i;if(const auto object=m_.word(slot))WorldObject(m_,object).Sub_001CA7F8(3);m_.word(slot,0);}
 m_.word(address_,0);
}
}

#include "game_core/indep/src/drivinglocation.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include "game_core/src/bMath.hpp"
namespace hp2::debug {
// Debug001C9B90, E-0099-W95. Source uses route points0/6/12 and normalized
// planar vectors, then VU cross magnitude with a sign opposite cross.z. W is
// unconsumed by the source XYZ dot; exceptional finite-domain checks stay loud.
float GetCurrentCurvatureOfDrivingLine(::port::driving::Memory& m,std::uint32_t location){if(DrivingLocation(m,location).IsClear())return 0;const auto route=m.word(location),point=m.word(location+4);const int line=m.signed_word(location+8),index=static_cast<std::int32_t>(point-route-0x428)/112;if(m.signed_word(route+0x10)-index<=12)return 0;auto position=[&](int offset){const auto p=point+112*offset;return TrackRoutePoint(m,p).GetPosition(0,static_cast<float>(m.signed_half(p+0x30+2*line)*256)*1.52587890625e-5f);};const auto a=position(0),b=position(6),c=position(12);const auto first=hp2::recovered::bNormalize(std::array<float,2>{{a[0]-b[0],a[1]-b[1]}}),second=hp2::recovered::bNormalize(std::array<float,2>{{b[0]-c[0],b[1]-c[1]}});const float z=first[0]*second[1]-first[1]*second[0],square=(0.f+0.f)+z*z;if(!std::isfinite(square))hp2::not_implemented("0x001C9B90","exceptional source VU normalized curvature arithmetic is not reconstructed");const float magnitude=std::sqrt(square);return z<0?magnitude:-magnitude;}
}

#include "game_core/port/driving/ee_scalar.hpp"
namespace hp2::debug {
// Debug001C9F48,E-0110-F11. Instruction grouping halves aero/friction before products.
float CalculateMaxDeceleration(::port::driving::Memory& m,std::uint32_t car,float speed){const auto config=m.word(car+0xc);const float mass=m.scalar(config+0x1e0),aero=(m.scalar(config+0x2f4)+m.scalar(config+0x2f8))*.5f,friction=(m.scalar(config+0x1cc)+m.scalar(config+0x1ac))*.5f;const float force=(std::fabs(speed)*aero+m.scalar(0x327ba0)*mass)*friction;return -(mass==0?::port::driving::ee_div_s_zero(force,mass,"0x001C9F9C"):force/mass);}
}

namespace hp2::debug {
// Debug001C8B88, E-0110-T01. Logical host storage preserves the original
// twenty-entry deque, branch ordering, dropped pushes when full, and point flags.
unsigned GuessTargetDestinations(::port::driving::Memory& m,NearestRoutePointInfo* out,unsigned requested,std::uint32_t route,std::uint32_t point,float distance,bool forwards){
 if(!route||!point)return 0;
 struct RouteWithDist {std::uint32_t Route,Point;float Distance;};
 std::array<RouteWithDist,20> pending{};unsigned head=0,tail=0;bool full=false;
 const auto push=[&](RouteWithDist value,bool front){if(full)return;if(front){head=head?head-1:19;pending[head]=value;}else{pending[tail]=value;tail=(tail+1)%20;}full=head==tail;};
 const auto next=[&](std::uint32_t r,std::uint32_t p){const auto index=(p-r-0x428)/0x70;if(forwards)return index+1<m.word(r+0x10)?p+0x70:0u;return index?p-0x70:0u;};
 RouteWithDist current{route,point,distance};unsigned produced=0;
 if(!forwards)current.Distance+=m.scalar(point+0x14);
 current.Distance-=m.scalar(point+0x14);
 for(;;){
  if(!(0<=current.Distance)){out[produced++]={current.Route,current.Point};if(produced==requested)return produced;current.Point=0;}
  else {
   bool advance=true;const auto old=current.Point;
   if(m.byte(old+0x2c)!=255){const auto sign=TrackRoutePoint(m,old).GetSignpost(),bits=m.word(sign);if(forwards?!(bits&0x800):bool(bits&0x800)){
    advance=false;const auto type=bits&0x700;
    if(type==0x300)current.Distance=-1;
    else {const auto new_route=Signpost::GetNewRoute(m,sign),new_point=Signpost::GetNewRoutePoint(m,sign);const float d=(current.Distance+m.scalar(old+0x14))-((m.scalar(new_point)-m.scalar(old))*m.scalar(old+0xc)+(m.scalar(new_point+4)-m.scalar(old+4))*m.scalar(old+0x10));
     if(type==0x200)current={new_route,new_point,d};
     else {if(const auto p=next(current.Route,old))push({current.Route,p,current.Distance},true);push({new_route,new_point,d},type==0);current.Point=0;}
    }
   }}
   if(advance)current.Point=next(current.Route,current.Point);
  }
  if(current.Point&&!(m.word(current.Point+0x2c)&0x8000))current.Point=0;
  if(!current.Point){if(!full&&head==tail)return produced;current=pending[head];head=(head+1)%20;full=false;}
  current.Distance-=m.scalar(current.Point+0x14);
 }
}
}
