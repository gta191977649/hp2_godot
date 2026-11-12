#include "game_core/port/race/cop_driver_targets.hpp"
#include "game_core/port/race/ai_drive_target.hpp"
#include "game_core/indep/inc/CopChase.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/indep/src/pursuit.hpp"
// TASK-0099 E-0099-W91. Debug NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Exact original definition source ../indep/src/copdriver.cpp; revision unknown.
#include "game_core/port/race/control_construction.hpp"
#include "game_core/indep/inc/CarDriver.hpp"
#include "game_core/indep/src/coputil.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/inc/CarMover.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/port/driving/ee_rounding.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/indep/inc/Replay.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>
namespace hp2::debug {
// Debug0019E650, E-0110-CM01; full constructor stores, including the original
// unwritten local vector's independently supplied PC research padding lane.
void CopMover::Sub_0019E650(std::uint32_t car,std::optional<std::uint32_t> normal_w){
 auto& m=m_;const auto a=address_;m.word(a+4,0x2ad7c8);m.word(a+8,car);
 m.vector(a+0x10,m.vector(car+0x70));m.vector(a+0x50,m.vector(a+0x10));
 m.word(a+0x74,0);m.word(a+0x30,m.word(car+0x3c0));m.half(a+0x2a,m.half(car+0x3a4));
 m.word(a+0x34,m.word(0x325680));m.word(a+0x2c,m.word(car+0x3c0));m.half(a+0x28,m.half(car+0x3a4));
 ResetMaxThrottleAccel();m.scalar(a+0x38,40);m.word(a+0x20,0);m.word(a+0x24,0);
 if(!normal_w)hp2::not_implemented("0x0019E6E4","CopMover constructor ground-normal stack padding lane requires an independent explicit original or PC research operand");
 m.word(a+0x60,0);m.word(a+0x64,0);m.word(a+0x68,0x3f800000);m.word(a+0x6c,*normal_w);
 m.word(a+0x70,0);m.word(a+0x40,0);
}
// Debug0019E700, E-0110-CM01; the original tail calls the base with live flags.
void CopMover::Sub_0019E700(int flags){m_.word(address_+4,0x2ad7c8);CarMover(m_,address_).Sub_00136B28(flags);}
// Debug0019E718, E-0110-CM01; exact31-row jump table002AD520; default preserves.
void CopMover::ResetMaxThrottleAccel(){auto& m=m_;const auto type=m.word(m.word(m.word(address_+8)+0xc)+0x534);
 switch(type){case 4:m.word(address_+0x3c,0x41340000);break;case 31:m.word(address_+0x3c,0x414c0000);break;case 32:case 33:m.word(address_+0x3c,0x41700000);break;case 34:m.word(address_+0x3c,0x41640000);break;default:break;}}
// Debug0019E798,E-0110-CM01. Enum snapshot consumes one byte in-place;
// bool snapshot consumes one byte but restores the original full bool word.
void CopMover::DoSnapshot(std::uint32_t snapshot){const auto a=address_;ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2ad5a0);CarMover(m_,a).DoSnapshot(snapshot);s.MoveData(a+0x10,12);s.MoveData(a+0x28,2);s.MoveData(a+0x2a,2);for(unsigned off:{0x2cu,0x34u,0x30u})s.MoveData(a+off,4);s.MoveData(a+0x50,12);s.MoveData(a+0x74,1);s.MoveData(a+0x38,4);s.MoveData(a+0x3c,4);s.MoveData(a+0x60,12);s.MoveData(a+0x70,1,4,0);s.MoveData(a+0x20,8);s.MoveData(a+0x40,4);s.EndSection();}

// Debug0019E8A8,E-0110-CM01. Original steering/translating branches and common
// five-point ground/body/wheel publication. Scoped EE truncation is the existing
// PC numerical carrier; no original-runtime bit-equivalence claim is made.
float CopMover::Move(float t,::port::race::PhysicsConstructionServices& services){
 ::port::driving::EeTruncationScope rounding("0x0019E8A8");
 auto& m=m_;const auto a=address_,car=m.word(a+8),s=car+0x40;
 const auto divide=[](float n,float d,const char* at){if(!std::isfinite(n)||!std::isfinite(d))hp2::not_implemented(at,"CopMover nonfinite EE scalar operand is outside the finite PC numerical carrier");const float value=d==0?::port::driving::ee_div_s_zero(n,d,at):n/d;if(!std::isfinite(value))hp2::not_implemented(at,"CopMover exceptional EE scalar encoding is outside the finite PC numerical carrier");return value;};
 const auto square=[](const std::array<float,4>& v){float value=v[0]*v[0];value=value+v[1]*v[1];return value+v[2]*v[2];};
 auto position=m.vector(car+0x60),normal=m.vector(a+0x60);int terrain;bool valid;
 TopologyCoordinate topology(m,car+0x520);topology.GetElevation({position[0],position[1],position[2]},&terrain,&normal,&valid);
 if(!valid){if(!services.cop_mover_invalid_ground_normal_w)hp2::not_implemented("0x0019E930","CopMover invalid-topology ground normal requires independent source stack padding");normal[0]=0;normal[1]=0;normal[2]=1;m.word(a+0x6c,*services.cop_mover_invalid_ground_normal_w);normal[3]=m.scalar(a+0x6c);}
 m.vector(a+0x60,normal);m.half(a+0x28,m.half(car+0x3a4));m.word(a+0x2c,m.word(car+0x3c0));m.vector(a+0x10,m.vector(car+0x70));
 const auto manager=m.word(0x328c1c);if(!manager)hp2::not_implemented("0x0019E97C","CopMover requires the original active CopManager");
 const auto difficulty=m.signed_word(manager+0x28ec);if(difficulty<0||difficulty>2)hp2::not_implemented("0x0019E998","CopMover difficulty reads outside the source three-element local speed table");
 m.word(a+0x70,0);const float maximum=m.scalar(a+0x34)+m.scalar(0x2ad5c8+4u*unsigned(difficulty));
 const float acceleration=divide(-m.scalar(a+0x3c),maximum,"0x0019E9A8")*std::fabs(m.scalar(a+0x2c))+m.scalar(a+0x3c);
 if(maximum<m.scalar(a+0x30))m.scalar(a+0x30,maximum);m.word(car+0x810,0);
 const auto mode=m.word(a+0x74);auto velocity=m.vector(a+0x10);
 if(mode==0){
  CalculateFacingAngle(maximum,t);m.half(car+0x3a4,m.half(a+0x28));
  float change=divide(m.scalar(a+0x30)-m.scalar(a+0x2c),t,"0x0019EA10");const float magnitude=std::fabs(change);
  if(change<0){float timer=m.scalar(a+0x40)+(t+t);if(1<timer)timer=1;m.scalar(a+0x40,timer);const float brake=m.scalar(a+0x38),limit=brake*timer;
   if(limit<magnitude)change=change*divide(limit,magnitude,"0x0019EA74");
   if(brake*m.scalar(0x325684)<magnitude||timer==1)m.scalar(m.word(car+0x2c)+0x218,divide(magnitude,brake,"0x0019EAA4"));
  }else{m.word(a+0x40,0);if(acceleration<magnitude){m.word(car+0x810,1);change=change*divide(acceleration,magnitude,"0x0019EAC8");}m.word(m.word(car+0x2c)+0x218,0);}
  float speed=m.scalar(a+0x2c)+change*t;m.scalar(a+0x2c,speed);if(maximum<std::fabs(speed)){speed=speed*divide(maximum,std::fabs(speed),"0x0019EB14");m.scalar(a+0x2c,speed);}m.scalar(car+0x3c0,speed);
  const auto angle=m.half(a+0x28);velocity[0]=speed*hp2::recovered::bCos(m,angle);velocity[1]=speed*hp2::recovered::bSin(m,angle);
  float dot=velocity[0]*normal[0];dot=dot+velocity[1]*normal[1];dot=dot+velocity[2]*normal[2];const float projected=dot<4?dot:4;
  for(unsigned i=0;i<3;++i)velocity[i]=velocity[i]-normal[i]*projected;
  if(4<dot)m.word(a+0x70,1);m.vector(a+0x10,velocity);m.vector(car+0x70,velocity);m.scalar(car+0x3c4,std::sqrt(square(velocity)));m.word(car+0x440,0);
 }else if(mode==1){
  const float length=std::sqrt(square(velocity)),vx=divide(velocity[0],length,"0x0019EC54"),vy=divide(velocity[1],length,"0x0019EC4C");
  const float ax=divide(m.scalar(a+0x50)-velocity[0],t,"0x0019EC34"),ay=divide(m.scalar(a+0x54)-velocity[1],t,"0x0019EC3C");
  const float longitudinal=ax*vx+ay*vy;float lx=longitudinal*vx,ly=longitudinal*vy;
  const float previous_x=m.scalar(a+0x20),previous_y=m.scalar(a+0x24);float tx=ax-lx,ty=ay-ly;m.scalar(a+0x20,tx);m.scalar(a+0x24,ty);
  const float transverse=std::sqrt(tx*tx+ty*ty);if(35<transverse){const float factor=divide(35,transverse,"0x0019ECDC");tx=tx*factor;ty=ty*factor;m.scalar(a+0x20,tx);m.scalar(a+0x24,ty);}
  const float jx=divide(tx-previous_x,t,"0x0019ED08"),jy=divide(ty-previous_y,t,"0x0019ED0C"),jerk=std::sqrt(jx*jx+jy*jy);
  if(150<jerk){const float factor=divide(150,jerk,"0x0019ED44");ty=previous_y+(jy*factor)*t;tx=previous_x+(jx*factor)*t;m.scalar(a+0x24,ty);m.scalar(a+0x20,tx);}
  float magnitude=std::sqrt(lx*lx+ly*ly);
  if(longitudinal<0){float timer=m.scalar(a+0x40)+(t+t);if(1<timer)timer=1;m.scalar(a+0x40,timer);const float brake=m.scalar(a+0x38),limit=brake*timer;
   if(limit<magnitude){const float factor=divide(limit,magnitude,"0x0019EDDC");lx=lx*factor;ly=ly*factor;magnitude=limit;}
   if(brake*m.scalar(0x325688)<magnitude||timer==1)m.scalar(m.word(car+0x2c)+0x218,divide(magnitude,brake,"0x0019EE1C"));
  }else{m.word(a+0x40,0);m.word(m.word(car+0x2c)+0x218,0);if(acceleration<magnitude){m.word(car+0x810,1);const float factor=divide(acceleration,magnitude,"0x0019EE40");lx=lx*factor;ly=ly*factor;}}
  velocity[0]=velocity[0]+(lx+m.scalar(a+0x20))*t;velocity[1]=velocity[1]+(ly+m.scalar(a+0x24))*t;
  float dot=velocity[0]*normal[0];dot=dot+velocity[1]*normal[1];dot=dot+velocity[2]*normal[2];const float projected=dot<4?dot:4;
  for(unsigned i=0;i<3;++i)velocity[i]=velocity[i]-normal[i]*projected;if(4<dot)m.word(a+0x70,1);
  float speed=std::sqrt(square(velocity));if(maximum<std::fabs(speed)){const float factor=divide(maximum,std::fabs(speed),"0x0019EF40");for(auto& lane:velocity)lane=lane*factor;speed=speed*factor;}
  m.vector(a+0x10,velocity);m.vector(car+0x70,velocity);m.scalar(car+0x3c0,speed);m.scalar(car+0x3c4,std::sqrt(square(velocity)));m.word(car+0x440,0);
  const float squared=square(velocity),inverse=squared==0?::port::driving::ee_vrsqrt_one_zero(squared,"0x0019EFA4"):1.f/std::sqrt(squared);
  const auto angle=hp2::recovered::bFixATan(m,::port::driving::cvt_w_s((velocity[0]*inverse)*65536.f),::port::driving::cvt_w_s((velocity[1]*inverse)*65536.f));
  m.half(a+0x2a,std::uint16_t(angle));CalculateFacingAngle(maximum,t);m.half(car+0x3a4,m.half(a+0x28));
 }
 position=m.vector(car+0x60);velocity=m.vector(a+0x10);for(unsigned i=0;i<4;++i)position[i]=position[i]+velocity[i]*t;m.vector(car+0x60,position);
 bool grounded=m.word(a+0x70)==0;
 if(grounded){
  // Listing0019F0B8..0019F0D0 is a five-iteration NOP loop, followed by
  // literal world-axis offsets; no unobserved wheel/body rotation is invented.
  const std::array<std::array<float,2>,5> offsets{{{{0,0}},{{1,0}},{{-1,0}},{{0,1}},{{0,-1}}}};
  std::array<float,5> heights;float minimum=m.scalar(0x32568c),maximum_height=m.scalar(0x325690);
  for(unsigned i=0;i<5;++i){int sample_terrain;heights[i]=topology.GetElevation({position[0]+offsets[i][0],position[1]+offsets[i][1],position[2]},&sample_terrain);minimum=std::min(minimum,heights[i]);maximum_height=std::max(maximum_height,heights[i]);}
  if(std::fabs(maximum_height-minimum)<=2){const auto config=m.word(car+0xc);const float pitch=((heights[1]-heights[0])-(heights[2]-heights[0]))*.5f*m.scalar(0x325694),roll=((heights[3]-heights[0])-(heights[4]-heights[0]))*.5f*m.scalar(0x325694);
   position[2]=((heights[0]+m.scalar(config+0x18c))-m.scalar(config+0x188))+m.scalar(config+0x118);m.scalar(car+0x68,position[2]);
   auto matrix=hp2::recovered::bIdentity();hp2::recovered::eRotateZ(m,matrix,matrix,m.half(car+0x3a4));hp2::recovered::eRotateX(m,matrix,matrix,std::uint16_t(::port::driving::cvt_w_s(roll)));hp2::recovered::eRotateY(m,matrix,matrix,std::uint16_t(-std::int16_t(::port::driving::cvt_w_s(pitch))));m.matrix(car+0x90,matrix);
   const auto cog=m.vector(config+0x110);auto geometry=position;for(unsigned i=0;i<4;++i)geometry[i]=geometry[i]-cog[i];m.vector(car+0x50,geometry);
  }else{m.word(a+0x70,1);}
 }
 hp2::recovered::CarState state(m,s);state.CalculateGeometryPosition();
 // The grounded path writes stack3C=0 in its five-offset array setup. If the
 // initial airborne branch skips that setup, the original fourth lane is a
 // distinct unwritten stack operand, not the constructor's normal padding.
 std::uint32_t angular_w=0;if(!grounded){if(!services.cop_mover_airborne_angular_w)hp2::not_implemented("0x0019F2F8","CopMover airborne angular-velocity padding requires an independent explicit source stack operand");angular_w=*services.cop_mover_airborne_angular_w;}
 m.word(car+0xd0,0);m.word(car+0xd4,0);m.word(car+0xd8,0);m.word(car+0xdc,angular_w);m.half(car+0x3a6,m.half(m.word(car+0x2c)+0x210));
 const auto config=m.word(car+0xc);for(unsigned i=0;i<4;++i){const auto off=i*4;const float angular=divide(m.scalar(a+0x2c),m.scalar(config+0x12c+0x20*i),"0x0019F338");m.scalar(s+0x2b0+off,angular);m.scalar(s+0x2c0+off,m.scalar(s+0x2c0+off)+angular*t);m.word(s+0x300+off,0);state.SetWheelTouchingGround(int(i),1);state.SetWheelTerrain(int(i),1);m.word(s+0x334+8*i,0);m.word(s+0x338+8*i,0);m.word(s+0x354+off,0);m.word(s+0x2d0+off,0);for(unsigned j=0;j<4;++j)m.word(s+0x2f0+4*j,0);}
 state.CalculateWheelMatrices();hp2::recovered::Car(m,car).ComputeFakeRPM();return t;
}
// Debug0019F560; fresh body/listing + mdebug ownership, E-0099-W91.
void CopDriver::Sub_0019F560(std::uint32_t car,const ::port::track::RouteData& routes){
 TrackedController(m_,address_).Sub_00233D68(car,routes);m_.word(address_+8,0x2ad758);
 const auto driver=m_.allocate(0x230);CarDriver(m_,driver).Sub_00136D10();m_.word(driver+0x22c,0x2a67f0);m_.word(driver+0x228,0);m_.word(driver+0x220,0xffffffff);
 m_.word(address_+0x30,0);m_.word(address_+0x3c,0xffffffff);m_.scalar(address_+0x40,0);m_.scalar(address_+0x44,0);m_.scalar(address_+0x38,m_.scalar(0x3256a8));m_.word(address_+0x28,1);m_.word(address_+4,driver);m_.word(address_+0x34,0);m_.word(address_+0x2c,1);m_.word(address_+0x24,1);
 // Original switch table002AD5F0; default deliberately preserves nCarClass.
 switch(m_.word(m_.word(car+0xc)+0x534)){case 4:m_.word(address_+0x48,4);break;case 31:m_.word(address_+0x48,3);break;case 32:m_.word(address_+0x48,0);break;case 33:m_.word(address_+0x48,1);break;case 34:m_.word(address_+0x48,2);break;default:break;}
}
// Debug0019F6A8; known constructor's real NullDriver virtual destructor only.
void CopDriver::Sub_0019F6A8(int flags){m_.word(address_+8,0x2ad758);if(const auto driver=m_.word(address_+4)){const auto vt=m_.word(driver+0x22c);if(m_.word(vt+0xc)!=0x136b98||m_.signed_half(vt+8)!=0)hp2::not_implemented("0x0019F6A8","actual replacement CarDriver destructor is unrecovered");m_.word(driver+0x22c,0x2a67f0);CarDriver(m_,driver).Sub_00136CB0(3);}TrackedController(m_,address_).Sub_00233DB8(flags);}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug001A03B8, E-0099-W101; exact original copdriver.cpp definition owner.
// Enum bytes preserve their upper storage; the signed int index widens to4.
void CopDriver::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(m_,snapshot);s.BeginSection(address_,0x2ad748);TrackedController(m_,address_).DoSnapshot(snapshot);for(unsigned off:{0x24u,0x28u,0x2cu})s.MoveData(address_+off,1);for(unsigned off:{0x30u,0x34u,0x38u})s.MoveData(address_+off,4);s.MoveData(address_+0x40,8);s.MoveData(address_+0x3c,1,4,1);s.EndSection();}
}

namespace hp2::debug {
// Debug0019F980, E-0099-W108. Exact copdriver.cpp owner and source branch delay slot.
// Tracked Think and CopSlot read precede the non-COP_MOVEMENT return.
// E-0112-COP01 restores original patrol/pursuit/backing-up continuations.
// Debug0019F708,E-0112-COP01. Source near/far history and exact DLOffset.
void CopDriver::GetPursuitTargetPoint(std::array<float,2>& result,const ::port::race::AiDriveTargetServices& services,std::optional<std::uint32_t> point_w){auto& m=m_;const auto a=address_,cop=m.word(m.word(0x328c1c)+16+4*m.word(a+0x3c)),pursuit=m.word(cop);if(!pursuit)return;const auto chased=m.word(pursuit+16),car=m.word(a),driver=m.word(a+4);const auto position=[&](std::uint32_t at){return std::array<float,2>{{m.scalar(at+0x60),m.scalar(at+0x64)}};};float lateral;switch(m.word(cop+0x14)){
 case 1:case 2:case 3:case 4:case 5:case 6:case 10:{const float x=m.scalar(car+0x60)-m.scalar(chased+0x60),y=m.scalar(car+0x64)-m.scalar(chased+0x64);m.word(a+0x2c,1);const std::array<float,2> offset{{m.scalar(cop+0x2c),m.scalar(cop+0x30)}};if(std::sqrt(x*x+y*y)<20){DriveTarget(m,driver).Sub_002165B8(pursuit+0x240);GetDriveTargetPointByDLOffsetFromStart(m,result,pursuit+0x240,offset);}else{DriveTarget(m,driver).Sub_002165B8(pursuit+0x30);FindDriveTargetPointByDistance(m,result,lateral,pursuit+0x30,position(car),20,offset);}break;}
 case 7:case 8:{m.word(a+0x2c,0);tAICop owner(m,cop);owner.InitializeDriveTarget(services);owner.UpdateDriveTarget(position(chased),false,services,point_w);std::array<float,2> offset{{0,0}};if(m.word(cop+0x14)==7){const auto route=hp2::recovered::Car(m,chased).GetRoutePoint(services);offset[1]=(m.scalar(chased+0x60)-m.scalar(route))*m.scalar(route+0x10)-(m.scalar(chased+0x64)-m.scalar(route+4))*m.scalar(route+0xc);}FindDriveTargetPointByDistance(m,result,lateral,m.word(a+4),position(chased),20,offset);break;}
 case 9:{m.word(a+0x2c,0);tAICop owner(m,cop);owner.InitializeDriveTarget(services);owner.UpdateDriveTarget(position(car),false,services,point_w);FindDriveTargetPointByDistance(m,result,lateral,m.word(a+4),position(car),20,{{0,0}});break;}
 default:return;}}
void CopDriver::Think(float elapsed,const ::port::track::RouteData& routes,::port::race::PhysicsConstructionServices* physics,const ::port::race::AiDriveTargetServices* supplied){
 auto& m=m_;TrackedController(m,address_).Think(elapsed,routes);const auto a=address_,index=m.word(a+0x3c),car=m.word(a);const auto manager=m.word(0x328c1c);if(!manager)hp2::not_implemented("0x0019F980","actual CopManager owner required before source slot lookup");const auto mode=m.word(car+0x24),cop=m.word(manager+0x10+4*index);if(mode!=5)return;if(index>=7||!cop)hp2::not_implemented("0x0019F9E8","actual registered AI cop required before original pursuit dereference");
 const auto pursuit=m.word(cop),chased=pursuit?m.word(pursuit+16):0,mover=m.word(car+0x30),driver=m.word(a+4);::port::race::AiDriveTargetServices fallback;fallback.routes=&routes;const auto& services=supplied?*supplied:fallback;const auto point_w=physics?physics->cop_target_point_w:std::optional<std::uint32_t>{};
 if(AreWeStuck(elapsed)&&!tAICop(m,cop).IsNearPursuitTarget()){m.word(a+0x28,m.word(a+0x24));m.word(a+0x24,4);m.scalar(a+0x34,2);}
 m.scalar(mover+0x38,40);CopMover(m,mover).ResetMaxThrottleAccel();
 const auto position=[&](std::uint32_t c){return std::array<float,2>{{m.scalar(c+0x60),m.scalar(c+0x64)}};};
 const auto target=[&]{std::array<float,2> result{{m.scalar(a+0x40),m.scalar(a+0x44)}};GetPursuitTargetPoint(result,services,point_w);m.scalar(a+0x40,result[0]);m.scalar(a+0x44,result[1]);};
 const auto polar=[&](const std::array<float,2>& v){::port::driving::Memory::Temporary out(m,8);hp2::recovered::bToPolar(m,out.address(),v);return std::pair<float,std::uint16_t>{m.scalar(out.address()),m.half(out.address()+4)};};
 const auto steer_delta=[&]{return std::array<float,2>{{m.scalar(a+0x40)-m.scalar(car+0x60),m.scalar(a+0x44)-m.scalar(car+0x64)}};};
 const auto find_patrol=[&]{std::array<float,2> result;float lateral;FindDriveTargetPointByDistance(m,result,lateral,m.word(a+4),position(car),20,{{m.scalar(cop+0x2c),m.scalar(cop+0x30)}});m.scalar(a+0x40,result[0]);m.scalar(a+0x44,result[1]);};
 const auto route_lateral=[&](std::uint32_t c){const auto p=hp2::recovered::Car(m,c).GetRoutePoint(services);if(!p)hp2::not_implemented("0x0019F980","source cop lateral comparison requires actual localized route point");return (m.scalar(c+0x60)-m.scalar(p))*m.scalar(p+0x10)-(m.scalar(c+0x64)-m.scalar(p+4))*m.scalar(p+0xc);};
 switch(m.word(a+0x24)){
 case 0:{m.scalar(mover+0x38,2);m.word(mover+0x30,0);m.word(driver+0x214,0);m.scalar(driver+0x21c,1);m.word(driver+0x224,0xffffffff);m.scalar(driver+0x218,1);m.half(driver+0x210,0);m.word(driver+0x220,0xffffffff);m.word(mover+0x74,0);if(m.word(m.word(driver+0x22c)+0x3c)!=0x136b88)hp2::not_implemented("0x0019FAD8","Original disabled-cop desired-speed virtual target is not recovered");break;}
 case 1:m.word(mover+0x30,0);m.word(mover+0x74,0);break;
 case 2:find_patrol();m.word(mover+0x74,0);m.half(mover+0x2a,polar(steer_delta()).second);m.scalar(mover+0x3c,4);m.word(mover+0x30,m.word(a+0x38));break;
 case 3:{if(!pursuit)return;float chased_speed=m.scalar(chased+0x3c4);target();const auto delta=steer_delta();const float x=m.scalar(car+0x60)-m.scalar(chased+0x60),y=m.scalar(car+0x64)-m.scalar(chased+0x64),distance=std::sqrt(x*x+y*y);std::array<float,2> chased_velocity{{m.scalar(chased+0x70),m.scalar(chased+0x74)}};const auto sub=m.word(cop+0x14);
 if(distance<40&&0<chased_velocity[0]*m.scalar(car+0x70)+chased_velocity[1]*m.scalar(car+0x74)&&m.scalar(0x3256ac)<=m.scalar(car+0x3c4)){
  m.word(mover+0x74,1);float parallel_factor=1,perpendicular_factor=1.5f,extra=0;if(m.word(chased+0x18)){chased_velocity=tNearFollowTarget(m,pursuit+0x450).GetVelocity(.25f);const float length=std::sqrt(chased_velocity[0]*chased_velocity[0]+chased_velocity[1]*chased_velocity[1]);if(0<length){const auto late=tNearFollowTarget(m,pursuit+0x450).GetVelocity(m.scalar(0x328c34));const float factor=std::sqrt(late[0]*late[0]+late[1]*late[1])/length;chased_velocity[0]=chased_velocity[0]*factor;chased_velocity[1]=chased_velocity[1]*factor;}}
  switch(sub){case 5:case 8:parallel_factor=1.5f;extra=3;break;case 6:extra=3;perpendicular_factor=10;break;case 7:{float factor=m.scalar(0x3256b4);if(std::fabs(route_lateral(chased)-route_lateral(car))<=2.5f){if(m.scalar(chased+0x3c0)*m.scalar(0x3256b0)<60){chased_velocity={{0,0}};break;}factor=.5f;}chased_velocity[0]=chased_velocity[0]*factor;chased_velocity[1]=chased_velocity[1]*factor;break;}case 10:perpendicular_factor=3;m.scalar(mover+0x38,14);break;default:break;}
  const auto direction=hp2::recovered::bNormalize(std::array<float,2>{{m.scalar(car+0x70),m.scalar(car+0x74)}});const float projection=delta[0]*direction[0]+delta[1]*direction[1],px=direction[0]*projection,py=direction[1]*projection;float vx=0,vy=0;if(sub!=10){vy=py*parallel_factor+(delta[1]-py)*perpendicular_factor+chased_velocity[1];vx=px*parallel_factor+(delta[0]-px)*perpendicular_factor+chased_velocity[0];}const float speed=std::sqrt((vx*vx+vy*vy)+0.f);float factor=1;if(0<speed)factor=(speed+extra)/speed;if(!physics||!physics->cop_desired_velocity_w)hp2::not_implemented("0x0019FF20","source local vel.pad requires independent explicit port operand");std::uint32_t lane=*physics->cop_desired_velocity_w;float pad;std::memcpy(&pad,&lane,4);if(0<speed){vx=vx*factor;vy=vy*factor;pad=pad*factor;}m.scalar(mover+0x50,vx);m.scalar(mover+0x54,vy);m.word(mover+0x58,0);m.scalar(mover+0x5c,pad);return;
 }
 m.word(mover+0x74,0);m.half(mover+0x2a,polar(delta).second);float desired=0;switch(sub){case 1:case 2:case 4:case 5:case 6:desired=distance+chased_speed;break;case 3:desired=distance*5+chased_speed;break;case 7:if(std::fabs(route_lateral(chased)-route_lateral(car))<=2.5f){if(!(m.scalar(chased+0x3c0)*m.scalar(0x3256b8)<60))desired=chased_speed*.5f;break;}[[fallthrough]];case 8:desired=chased_speed*m.scalar(0x3256bc);break;case 9:desired=m.scalar(mover+0x34);break;case 10:desired=0;break;default:break;}
 const int car_class=std::min(int(m.signed_byte(m.word(chased+0xc)+0xe6)),m.signed_word(a+0x48));const auto route=hp2::recovered::Car(m,car).GetRoutePoint(services);const int difficulty=m.word(chased+0x18)?m.signed_word(manager+0x28ec):2;if(car_class<0||car_class>=21||difficulty<0||difficulty>2||!route)hp2::not_implemented("0x0019F980","source cop line speed needs actual route/class/difficulty table domain");const float line=(float(std::uint32_t(m.byte(route+0x5a+unsigned(car_class)))<<16)*1.52587890625e-5f)*m.scalar(0x308608+4*unsigned(difficulty));float speed=std::min(line,desired);if(m.scalar(0x3256c0)<speed){const auto own=position(car),ch=position(chased);const auto angle=polar({{ch[0]-own[0],ch[1]-own[1]}});const auto diff=std::int16_t(angle.second-m.half(car+0x3a4));if(16384<std::fabs(float(diff))&&1<angle.first)speed=m.scalar(0x3256c0);}m.scalar(mover+0x30,speed);break;}
 case 4:{const float time=m.scalar(a+0x34)-elapsed;m.scalar(a+0x34,time);if(time<0){m.word(a+0x24,m.word(a+0x28));return;}if(m.word(a+0x28)==2)find_patrol();else if(m.word(a+0x28)==3)target();m.word(mover+0x74,0);m.half(mover+0x2a,polar(steer_delta()).second);m.scalar(mover+0x3c,4);m.word(mover+0x30,m.word(0x3256c4));break;}
 default:break;
 }
}
}

namespace hp2::debug {
// Debug001A02F0,E-0110-P01; original copdriver.cpp. No recent collision
// preserves the timer; only patrolling/pursuit accumulate below strict2m/s.
bool CopDriver::AreWeStuck(float elapsed){if(!CarInRecentCollision(m_,m_.word(address_)))return false;switch(m_.word(address_+0x24)){case 0:case 1:case 4:m_.word(address_+0x30,0);return false;case 2:case 3:if(m_.scalar(m_.word(address_)+0x3c4)<2){const float timer=m_.scalar(address_+0x30)+elapsed;m_.scalar(address_+0x30,timer);return 1<timer;}m_.word(address_+0x30,0);return false;default:return false;}}
}

#include "game_core/indep/inc/CopDriver.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/port/driving/projections.hpp"
#include <cmath>
namespace hp2::debug {
// Debug0019F448,E-0110-F08. Instruction listing fixes MUL.S grouping and signed16 wrap.
float CopMover::CalculateFacingAngle(float maximum_speed,float elapsed){auto& m=m_;const float speed=m.scalar(address_+0x2c);float limit=m.scalar(0x3256a0);if(0<=speed){const float numerator=m.scalar(0x325698);const float ratio=maximum_speed==0?::port::driving::ee_div_s_zero(numerator,maximum_speed,"0x0019F468"):numerator/maximum_speed;limit=ratio*std::fabs(speed)+m.scalar(0x32569c);}const auto difference=static_cast<std::int16_t>(m.half(address_+0x2a)-m.half(address_+0x28));float delta=static_cast<float>(difference);if(limit<std::fabs(delta))delta=delta*(limit/std::fabs(delta));const float factor=std::fabs(speed)*m.scalar(0x3256a4);float angular=delta*factor;const float cap=static_cast<float>(0x780000/360);if(cap<std::fabs(angular))angular=0<=angular?cap:-cap;float change=angular*elapsed;if(std::fabs(delta)<std::fabs(change))change=delta;const auto increment=::port::driving::cvt_w_s(change);m.half(address_+0x28,static_cast<std::uint16_t>(m.half(address_+0x28)+static_cast<std::uint32_t>(increment)));return angular;}
}

#include "game_core/port/race/cop_driver_targets.hpp"
namespace hp2::debug {
// Debug001A0490,E-0112-COP01. Original backward segment scan, second-point
// failure fallback and untouched lateral error on failure are intentional.
bool FindDriveTargetPointByDistance(::port::driving::Memory& m,std::array<float,2>& res,float& lateral,std::uint32_t target,const std::array<float,2>& position,float desired,const std::array<float,2>& offset){const int count=m.signed_byte(target);const auto point=[&](int index){const auto at=target+16+std::uint32_t(index)*16;return std::array<float,2>{{m.scalar(at),m.scalar(at+4)}};};if(count==1){const auto p=point(0);const float x=position[0]-p[0],y=position[1]-p[1];if(std::sqrt(x*x+y*y)<=desired){res=p;lateral=0;return true;}}else if(count>=2){for(int first=count-2;first>=0;--first){const auto a=point(first),b=point(first+1);float dx=b[0]-a[0],dy=b[1]-a[1];const float length=std::sqrt(dx*dx+dy*dy);if(length!=0){dx=dx*(1/length);dy=dy*(1/length);}const float projection=(position[0]-a[0])*dx+(position[1]-a[1])*dy;if(0<=projection&&projection<length){float distance=-projection;for(int i=first;i<count-1;++i){const auto p=point(i),q=point(i+1);dx=q[0]-p[0];dy=q[1]-p[1];const float segment=std::sqrt(dx*dx+dy*dy);distance=distance+segment;if(desired<=distance){const float inverse=segment==0?::port::driving::ee_div_s_zero(1,segment,"0x001A04FC"):1/segment;dx=dx*inverse;dy=dy*inverse;res[0]=(q[0]-dx*(distance-desired))+dx*offset[0]+dy*offset[1];res[1]=(q[1]-dy*(distance-desired))+dy*offset[0]+-dx*offset[1];lateral=std::fabs((position[0]-res[0])*dy+(position[1]-res[1])*-dx);return true;}}break;}}}res=point(count<2?0:1);return false;}
// Debug001A0750,E-0112-COP01. Offset.d measures BACK from final target point.
void GetDriveTargetPointByDLOffsetFromStart(::port::driving::Memory& m,std::array<float,2>& result,std::uint32_t target,const std::array<float,2>& offset){const int count=m.signed_byte(target);const auto point=[&](int index){const auto at=target+16+std::uint32_t(index)*16;return std::array<float,2>{{m.scalar(at),m.scalar(at+4)}};};auto older=point(count-1),newer=older;if(count<2){result=older;return;}float distance=0;if(offset[0]>=0){for(int i=count-1;i>=0;--i){newer=older;older=point(i);const float x=older[0]-newer[0],y=older[1]-newer[1];distance=distance+std::sqrt(x*x+y*y);if(i==0||offset[0]<distance)break;}}float x=newer[0]-older[0],y=newer[1]-older[1];const float length=std::sqrt(x*x+y*y);if(0<length){const float fraction=(distance-offset[0])/length;x=x*fraction;y=y*fraction;}result={{older[0]+x,older[1]+y}};float lx=newer[1]-older[1],ly=-(newer[0]-older[0]);const float size=std::sqrt(lx*lx+ly*ly);if(size!=0){lx=lx/size;ly=ly/size;}result[0]=result[0]+lx*offset[1];result[1]=result[1]+ly*offset[1];}
}
