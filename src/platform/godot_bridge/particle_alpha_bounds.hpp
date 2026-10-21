#pragma once
// Port-side sampler-footprint bound, not recovered PS2 behavior. Q1 particle
// packets select LOD0. Include both bilinear taps and original wrap/clamp modes.
#include <array>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace port::godot_bridge {
inline unsigned particle_uv_alpha_max(const std::vector<std::uint8_t>& rgba,int width,int height,
    const std::array<std::array<float,2>,4>& uv,bool repeat_u,bool repeat_v){
 if(width<=0||height<=0||rgba.size()!=std::size_t(width)*std::size_t(height)*4)throw std::invalid_argument("Particle alpha bound image dimensions invalid");
 float lo_u=uv[0][0],hi_u=lo_u,lo_v=uv[0][1],hi_v=lo_v;
 for(const auto& p:uv){if(!std::isfinite(p[0])||!std::isfinite(p[1])||std::abs(p[0])>2||std::abs(p[1])>2)throw std::invalid_argument("Particle atlas UV outside bounded packet range");lo_u=std::min(lo_u,p[0]);hi_u=std::max(hi_u,p[0]);lo_v=std::min(lo_v,p[1]);hi_v=std::max(hi_v,p[1]);}
 unsigned maximum=0;
 for(int y=int(std::floor(lo_v*height-.5f));y<=int(std::floor(hi_v*height-.5f))+1;++y)
  for(int x=int(std::floor(lo_u*width-.5f));x<=int(std::floor(hi_u*width-.5f))+1;++x){
   const int sx=repeat_u?(x%width+width)%width:std::clamp(x,0,width-1);
   const int sy=repeat_v?(y%height+height)%height:std::clamp(y,0,height-1);
   maximum=std::max(maximum,unsigned(rgba[(std::size_t(sy)*width+sx)*4+3]));
  }
 return maximum;
}
}
