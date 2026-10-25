#include "frame.hpp"
#include <algorithm>
#include <stdexcept>
namespace port::render {
namespace {
bool valid_ranges(const Frame& frame) noexcept {
 std::size_t end=0;
 for(const auto& range:frame.frontend_ranges) {
  if(!range.count || range.first<end || range.first>frame.quads.size() ||
     range.count>frame.quads.size()-range.first)return false;
  end=range.first+range.count;
 }
 return true;
}
}
bool frontend_ranges_complete(const Frame& frame) noexcept {
 if(!valid_ranges(frame))return false;
 std::size_t end=0;
 for(const auto& range:frame.frontend_ranges) {
  if(range.first!=end || (range.stage!=FrontendStage::background &&
                         range.stage!=FrontendStage::foreground))return false;
  end=range.first+range.count;
 }
 return end==frame.quads.size();
}
void append_frontend_frame(Frame& destination,const Frame& source) {
 if(&destination==&source) {const auto copy=source;append_frontend_frame(destination,copy);return;}
 if(!valid_ranges(destination) || !valid_ranges(source))
  throw std::invalid_argument("Frontend quad ranges overlap or exceed packet bounds");
 const auto offset=destination.quads.size();
 destination.quads.insert(destination.quads.end(),source.quads.begin(),source.quads.end());
 for(auto range:source.frontend_ranges) {
  range.first+=offset;
  if(!destination.frontend_ranges.empty() &&
     destination.frontend_ranges.back().stage==range.stage &&
     destination.frontend_ranges.back().first+destination.frontend_ranges.back().count==range.first)
   destination.frontend_ranges.back().count+=range.count;
  else destination.frontend_ranges.push_back(range);
 }
}
bool crop(Quad& q,const Rect& clip) {
 if(q.bypass_clip)return true;
 if(!q.destination.has_area())return false;
 if(!clip.has_area())return true;
 const auto d=q.destination;
 const float x=std::max(d.x,clip.x),y=std::max(d.y,clip.y);
 const float right=std::min(d.x+d.width,clip.x+clip.width);
 const float bottom=std::min(d.y+d.height,clip.y+clip.height);
 if(right<=x||bottom<=y)return false;
 if(q.has_vertex_data) {
  // E-0039-Q06 / 00141808: clip U horizontally, V vertically; corner colours stay unchanged.
  auto& uv=q.source_points;
  if(x>d.x) {
   const float t=(x-d.x)/d.width;
   uv[0].x=uv[3].x=(1-t)*uv[0].x+t*uv[1].x;
  }
  if(clip.x+clip.width<=d.x+d.width) {
   const float t=(d.x+d.width-right)/(d.x+d.width-x);
   uv[1].x=uv[2].x=t*uv[0].x+(1-t)*uv[1].x;
  }
  if(y>d.y) {
   const float t=(y-d.y)/d.height;
   uv[0].y=uv[1].y=(1-t)*uv[0].y+t*uv[2].y;
  }
  if(clip.y+clip.height<=d.y+d.height) {
   const float t=(d.y+d.height-bottom)/(d.y+d.height-y);
   uv[2].y=uv[3].y=t*uv[0].y+(1-t)*uv[2].y;
  }
  q.positions={Vec2{x,y},Vec2{right,y},Vec2{right,bottom},Vec2{x,bottom}};
 }
 const float sx=q.source.width/d.width,sy=q.source.height/d.height;
 q.source={q.source.x+(x-d.x)*sx,q.source.y+(y-d.y)*sy,(right-x)*sx,(bottom-y)*sy};
 q.destination={x,y,right-x,bottom-y};return true;
}
}
