#include "frame.hpp"
#include <algorithm>
namespace port::render {
bool crop(Quad& q,const Rect& clip) {
 if(!q.destination.has_area())return false;
 if(!clip.has_area())return true;
 const auto d=q.destination;
 const float x=std::max(d.x,clip.x),y=std::max(d.y,clip.y);
 const float right=std::min(d.x+d.width,clip.x+clip.width);
 const float bottom=std::min(d.y+d.height,clip.y+clip.height);
 if(right<=x||bottom<=y)return false;
 const float sx=q.source.width/d.width,sy=q.source.height/d.height;
 q.source={q.source.x+(x-d.x)*sx,q.source.y+(y-d.y)*sy,(right-x)*sx,(bottom-y)*sy};
 q.destination={x,y,right-x,bottom-y};return true;
}
}
