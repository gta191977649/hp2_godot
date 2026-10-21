#include "sune.hpp"
#include "game_core/src/bMath.hpp"
#include <stdexcept>
namespace hp2::recovered {
// Retail001252B0 / Debug00125430. Caller provides original projected sun,
// view-space Z and count of 24-bit depth samples<2 in the original32x32 readback.
// StuffSunGlare is a distinct upstream call, not approximated here.
std::vector<port::track::SunQuad> StuffSun(port::driving::Memory& m,const port::track::EnvironmentResources& r,unsigned id,
 float x,float y,float camera_z,unsigned width,unsigned height,float zmax,unsigned uncovered){
 std::vector<port::track::SunQuad> out;if(!r.sun.available||id!=1||camera_z<0)return out;
 if(!width||!height||uncovered>1024)throw std::invalid_argument("Invalid original sun viewport/readback");
 float largest=0;for(const auto& l:r.sun.layers)if(l.texture==0&&largest<l.size)largest=l.size;
 if(x < -largest||x>float(width)+largest||y < -largest||y>float(height)+largest)return out;
 const float coverage=float(uncovered)*(1.f/1024.f);
 for(unsigned i=0;i<6;++i){const auto& l=r.sun.layers[i];if(l.intensity==0)continue;
 const auto angle=std::uint16_t(l.angle+int(l.sweep*((x+largest)/(float(width)+largest+largest))*65536.f));
 const float sine=(l.size*.5f)*hp2::recovered::bSin(m,angle),cosine=(l.size*.5f)*hp2::recovered::bSin(m,std::uint16_t(angle+0x4000));
 const float a=cosine-sine,b=sine+cosine,cx=x+l.offset_x,cy=y+l.offset_y;
 port::track::SunQuad q;q.texture_hash=r.sun_textures[l.texture];q.vertices={{{{cx-b,cy-a}},{{cx+a,cy-b}},{{cx+b,cy+a}},{{cx-a,cy+b}}}};
 q.colour=l.colour;const float alpha=l.intensity<0?-l.intensity*128.f:coverage*l.intensity*128.f;
 q.colour[3]=static_cast<std::uint8_t>(int(alpha)&255);q.additive=l.alpha_type!=0;q.native_depth=i==0?1.f:zmax;out.push_back(q);
 }
 return out;
}
}

