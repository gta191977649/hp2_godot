#pragma once
// TASK-0110 E-0110-RO01. Port-only read-only observation of original road
// TrackRoute points. This is test/telemetry data, never an original autopilot,
// controller localization call, generated spline or game-state mutation.
#include "game_core/indep/src/aiutil.hpp"
#include <cmath>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
namespace port::race {
struct RoadRoutePoint {
 int route=-1,point=-1;
 std::uint32_t address=0;
 float x=0,y=0,z=0,dx=0,dy=0,length=0,width_left=0,width_right=0,distance=0;
};
struct RoadRouteObservation {
 std::vector<RoadRoutePoint> points;
 bool ended=false,looped=false;
 // Port course metadata only; ahead-window behavior remains unchanged.
 std::optional<std::size_t> loop_entry_index;
};
// GetNextRoutePointAhead001C8A88/GetNextRoutePoint001C8978 read source memory
// and modify only the local NearestRoutePointInfo carrier. Raw row fields and
// route bounds follow E-0099-W08/W14/W17/W60. horizon is a port export limit
// measured along observed XY chords, not an original race distance calculation.
inline RoadRouteObservation observe_route_ahead(driving::Memory& m,
 const track::RouteData& routes,std::uint32_t route,std::uint32_t point,
 std::size_t max_points=64,float horizon=150){
 if(!max_points||max_points>256||!std::isfinite(horizon)||horizon<=0)
  throw std::invalid_argument("Road observation requires bounded positive export limits");
 RoadRouteObservation out;
 if(!route||!point){out.ended=true;return out;}
 hp2::debug::NearestRoutePointInfo cursor{route,point};
 std::set<std::pair<std::uint32_t,std::uint32_t>> seen;
 float traversed=0;
 for(std::size_t i=0;i<max_points;++i){
  if(!cursor.Route||!cursor.Point){out.ended=true;break;}
  if(!seen.emplace(cursor.Route,cursor.Point).second){out.looped=true;break;}
  const int number=m.signed_half(cursor.Route+0xa),count=m.signed_word(cursor.Route+0x10);
  const auto delta=std::int64_t(cursor.Point)-cursor.Route-0x428;
  if(number<0||number>=int(routes.routes.size())||routes.routes[std::size_t(number)]!=cursor.Route||count<0||count>1024||delta<0||delta%0x70||delta/0x70>=count)
   throw std::out_of_range("Road observation left the actual loaded TrackRoute point tables");
  const auto p=cursor.Point;RoadRoutePoint row;
  row.route=number;row.point=int(delta/0x70);row.address=p;
  row.x=m.scalar(p);row.y=m.scalar(p+4);row.z=m.scalar(p+8);
  row.dx=m.scalar(p+0xc);row.dy=m.scalar(p+0x10);row.length=m.scalar(p+0x14);
  row.width_left=m.scalar(p+0x18);row.width_right=m.scalar(p+0x1c);row.distance=m.scalar(p+0x28);
  for(const auto value:{row.x,row.y,row.z,row.dx,row.dy,row.length,row.width_left,row.width_right,row.distance})
   if(!std::isfinite(value))throw std::out_of_range("Road observation contains nonfinite source fields");
  if(!out.points.empty()){const auto& previous=out.points.back();traversed+=std::hypot(row.x-previous.x,row.y-previous.y);}
  out.points.push_back(row);
  if(traversed>=horizon)break;
  hp2::debug::GetNextRoutePointAhead(m,cursor);
 }
 return out;
}
inline RoadRouteObservation observe_route_ahead(driving::Memory& m,
 const track::RouteData& routes,const hp2::debug::NearestRoutePointInfo& current,
 std::size_t max_points=64,float horizon=150){
 return observe_route_ahead(m,routes,current.Route,current.Point,max_points,horizon);
}
// TASK-0100 E-0100-RC02. One-time external-test planning geometry from the
// initialized source road. This does not localize a controller or implement an
// original autopilot. Unlike the bounded ahead window, its structural budget
// is the sum of actual loaded route point counts, never a guessed lap length.
inline RoadRouteObservation observe_route_course(driving::Memory& m,
 const track::RouteData& routes,std::uint32_t route,std::uint32_t point){
 RoadRouteObservation out;
 if(!route||!point){out.ended=true;return out;}
 std::size_t budget=0;
 std::set<std::uint32_t> loaded;
 for(std::size_t number=0;number<routes.routes.size();++number){
  const auto address=routes.routes[number];if(!address)continue;
  const int count=m.signed_word(address+0x10);
  if(m.signed_half(address+0xa)!=int(number)||count<0||count>1024||!loaded.insert(address).second)
   throw std::out_of_range("Road course has invalid loaded route identity/count");
  budget+=std::size_t(count);
 }
 hp2::debug::NearestRoutePointInfo cursor{route,point};
 std::map<std::pair<std::uint32_t,std::uint32_t>,std::size_t> seen;
 for(;;){
  if(!cursor.Route||!cursor.Point){out.ended=true;return out;}
  const auto identity=std::make_pair(cursor.Route,cursor.Point);
  if(const auto previous=seen.find(identity);previous!=seen.end()){out.looped=true;out.loop_entry_index=previous->second;return out;}
  if(out.points.size()>=budget||!loaded.count(cursor.Route))
   throw std::out_of_range("Road course left the loaded route structural bound");
  const int number=m.signed_half(cursor.Route+0xa),count=m.signed_word(cursor.Route+0x10);
  const auto delta=std::int64_t(cursor.Point)-cursor.Route-0x428;
  if(number<0||number>=int(routes.routes.size())||routes.routes[std::size_t(number)]!=cursor.Route||count<0||count>1024||delta<0||delta%0x70||delta/0x70>=count)
   throw std::out_of_range("Road course left the actual loaded TrackRoute point tables");
  RoadRoutePoint row;const auto p=cursor.Point;
  row.route=number;row.point=int(delta/0x70);row.address=p;
  row.x=m.scalar(p);row.y=m.scalar(p+4);row.z=m.scalar(p+8);
  row.dx=m.scalar(p+0xc);row.dy=m.scalar(p+0x10);row.length=m.scalar(p+0x14);
  row.width_left=m.scalar(p+0x18);row.width_right=m.scalar(p+0x1c);row.distance=m.scalar(p+0x28);
  for(const auto value:{row.x,row.y,row.z,row.dx,row.dy,row.length,row.width_left,row.width_right,row.distance})
   if(!std::isfinite(value))throw std::out_of_range("Road course contains nonfinite source fields");
  seen.emplace(identity,out.points.size());out.points.push_back(row);
  // This mutates only cursor. Preserve source sign/skip/end behavior; a repeat
  // can be a previously observed merge point rather than the supplied origin.
  hp2::debug::GetNextRoutePointAhead(m,cursor);
 }
}
inline RoadRouteObservation observe_route_course(driving::Memory& m,
 const track::RouteData& routes,const hp2::debug::NearestRoutePointInfo& current){
 return observe_route_course(m,routes,current.Route,current.Point);
}
}
