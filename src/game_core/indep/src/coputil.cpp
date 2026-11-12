// TASK-0099 E-0099-W119, Debug0022B958, original ..\indep\src\coputil.cpp.
// NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/indep/src/coputil.hpp"
#include "game_core/indep/src/carinfo.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/src/collision.hpp"
namespace hp2::debug {
// Debug0022B738,E-0100-PA01. Original iterator order and speed/front tests.
std::uint32_t CarCausedRecentCollision(::port::driving::Memory& m,std::uint32_t car){const auto body=m.word(car+0x34);int index=0;while(index<m.signed_byte(body+0x12)){Collision collision(m,CollisionBody(m,body).GetCollision(index++));if(!collision.IsTwoCarCollision())continue;std::uint32_t first,second;collision.GetCollidingCars(first,second);const auto other=first==car?second:first;if(!other)return 0;const float speed=m.scalar(other+0x3c0);if(speed!=0){if(!hp2::recovered::Car(m,car).IsCarInFront(other,0x2000))continue;if(speed<0||m.scalar(car+0x3c0)<=speed)continue;}return other;}return 0;}
// Debug0022B888,E-0100-PU03. The source iterator, not a timer/attribution
// heuristic: skip non-two-car records and match the actual opposite owner.
bool DidTheseCarsCollide(::port::driving::Memory& m,std::uint32_t first,std::uint32_t second){
 const auto body=m.word(first+0x34);int index=0;
 while(index<m.signed_byte(body+0x12)){
  Collision collision(m,CollisionBody(m,body).GetCollision(index++));
  if(!collision.IsTwoCarCollision())continue;
  std::uint32_t left,right;collision.GetCollidingCars(left,right);
  const auto other=left==first?right:left;
  if(!other)return false;
  if(other==second)return true;
 }
 return false;
}
// Debug0022B878,E-0110-P01; original coputil.cpp. The original signed byte
// load is converted to bool; any nonzero collision-count encoding is true.
bool CarInRecentCollision(::port::driving::Memory& m,std::uint32_t car){return m.byte(m.word(car+0x34)+0x12)!=0;}
float GetTopSpeed(::port::driving::Memory& m,std::uint32_t car){const auto info=m.word(car+0xc);const float mass=m.scalar(info+0x1e0),drag=m.scalar(info+0x2f0),limit=m.scalar(0x3260b8),step=m.scalar(0x3260bc);if(mass==0||!std::isfinite(mass)||!std::isfinite(drag)||!(step>0)||!std::isfinite(step)||!std::isfinite(limit))hp2::not_implemented("0x0022B958","original top-speed finite scan/division operands are unresolved");float speed=m.scalar(0x3260b4);for(;speed<limit;speed=speed+step){const auto table=GetAccelerationTable(m.signed_word(m.word(car+0x10)));const float acceleration=hp2::recovered::Table(m,table).GetValue(speed);if(acceleration<((speed*speed)*drag)/mass)break;}return speed-m.scalar(0x3260c0);}
}

#include "game_core/indep/inc/World.hpp"
namespace hp2::debug {
// Debug0022B408/0022B438,E-0110-F07; original ..\indep\src\coputil.cpp; mdebug FDR153/SYMR508911/508937.
float GetForwardTrackDistance(::port::driving::Memory& m,float p1,float p0){float distance=p1-p0;if(distance<0)distance+=m.scalar(0x313308);return distance;}
bool IsTeleportLocationClear(::port::driving::Memory& m,const std::array<float,2>& position){const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0022B438","actual World required by cop teleport clearance");if(!(250.f<=World(m,world).GetDistanceToClosestPlayer(position)))return false;for(int i=0;i<m.signed_word(world+0x1c);++i){const auto car=m.word(world+0x2c+4*i);const float dx=position[0]-m.scalar(car+0x60),dy=position[1]-m.scalar(car+0x64);if(std::sqrt(dx*dx+dy*dy)<15.f)return false;}return true;}
// Debug0022B610; E-0100-CT01; original coputil.cpp. The nearby-car test
// intentionally uses the CURRENT location and excludes this exact car.
bool CanTeleport(::port::driving::Memory& m,std::uint32_t car,const std::array<float,2>& destination){
 const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0022B610","actual World required by original cop teleport predicate");
 const std::array<float,2> current{{m.scalar(car+0x60),m.scalar(car+0x64)}};
 if(!(250.f<=World(m,world).GetDistanceToClosestPlayer(current)))return false;
 if(!(250.f<=World(m,world).GetDistanceToClosestPlayer(destination)))return false;
 for(int i=0;i<m.signed_word(world+0x1c);++i){const auto other=m.word(world+0x2c+4*unsigned(i));if(other==car)continue;const float dx=current[0]-m.scalar(other+0x60),dy=current[1]-m.scalar(other+0x64);if(std::sqrt(dx*dx+dy*dy)<15.f)return false;}
 return true;
}
}

