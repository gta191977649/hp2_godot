// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trackmap.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/TrackMap.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001BCA08; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bca08.md.
void TrackMap::EmitPoint() {
    hp2::not_implemented("0x001BCA08", "TrackMap::EmitPoint (signature and full body unresolved)");
}
} // namespace hp2::recovered


#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/port/track/texture_bindings.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/src/ecstasy/emathe.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/inc/Race.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
namespace port::race::map_detail {
using V2=std::array<float,2>;using V3=std::array<float,4>;using Quad=std::array<V2,4>;
static std::uint32_t mode(const driving::Memory& m,std::uint32_t a){const auto i=m.signed_word(a+0xa4);if(i<0||i>=11)hp2::not_implemented("0x001B8200","TrackMap original displayed mode outside11-entry table");return 0x30b7b0+0x44*unsigned(i);}
static std::string text(const driving::Memory& m,std::uint32_t a){std::string s;for(unsigned i=0;i<4096;++i){const auto c=m.byte(a+i);if(!c)return s;s+=char(c);}hp2::not_implemented("0x001B7FD0","TrackMap source resource name lacks terminator");}
// Source ePoly0010FD48 default values; explicit carrier rather than PS2 ABI.
static HudPolygon polygon(){HudPolygon p;const std::array<std::array<float,3>,4> vertices{{{{0,0,0}},{{64,0,0}},{{64,64,0}},{{0,64,0}}}};const std::array<std::array<float,3>,4> uv{{{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}}};for(unsigned i=0;i<4;++i){p.vertices[i].position=vertices[i];p.vertices[i].uvq=uv[i];p.vertices[i].colour=0x80808080;}return p;}
// Debug001BAC38; E-0112-M01. Source cubic grouping retained.
static void cubic(float& y,float& speed,float target,float t){const float old=speed,delta=target-y,a=(old-delta)-delta,b=((delta+delta+delta)-old)-old;y=((a*t+b)*t+old)*t+y;speed=((a+a+a)*t+b+b)*t+old;}
// Debug001C8150/001C8208; E-0112-M01.
static int side(const V2& p0,const V2& p1,const V2& t){const float value=(t[0]-p0[0])*(p1[1]-p0[1])+(t[1]-p0[1])*-(p1[0]-p0[0]);return value>0?1:value<0?-1:0;}
static bool inside(const V2& point,const Quad& points){bool value=false;unsigned previous=3;for(unsigned i=0;i<4;++i){if(((points[i][1]<=point[1]&&point[1]<points[previous][1])||(points[previous][1]<=point[1]&&point[1]<points[i][1]))&&point[0]<((points[previous][0]-points[i][0])*(point[1]-points[i][1]))/(points[previous][1]-points[i][1])+points[i][0])value=!value;previous=i;}return value;}
static V3 transform(driving::Memory& m,const std::array<float,16>& matrix,const V3& point){V3 out;hp2::debug::eMulVector(m,out,matrix,point);return out;}
static std::uint32_t texture_hash(const driving::Memory& m,std::uint32_t texture){if(!texture)hp2::not_implemented("0x001B9558","original TrackMap ePoly texture dependency absent");return m.word(texture+0x20);}
}
namespace hp2::debug {
// Debug001B7FD0; E-0112-M01. Native original resource lookup and signed track ID.
void TrackMap::Sub_001B7FD0(std::uint32_t track,std::uint32_t car,std::uint32_t player){auto& m=m_;const auto a=address_;m.word(a+0x9c,1);m.word(a+16,0);if(!track||!car||!player){m.word(a+0x9c,0);return;}InitTrackMapModeInfo();m.word(a+0x84,car);m.word(a+0x88,player);m.word(a+0xa4,6);m.word(a+16,m.word(0x327cf8));m.word(a+0x8c,track);m.word(a+0x90,0);m.word(a+0xa0,6);for(unsigned i=0;i<4;++i)m.scalar(a+4*i,0);char name[16];const auto fmt=::port::race::map_detail::text(m,0x2ae878);const int count=std::snprintf(name,sizeof name,fmt.c_str(),int(m.signed_byte(track+0x41)));if(count<0||count>=16)hp2::not_implemented("0x001B7FD0","source TrackMap texture name exceeds16-byte source extent");m.word(a+0x60,GetTextureInfo(m,hp2::frontend::bStringHash(name),0,0));if(!m.word(a+0x60))m.word(a+0x9c,0);
 for(const auto item:std::array<std::pair<unsigned,std::uint32_t>,6>{{{0x64,0x2ae888},{0x68,0x2ae898},{0x6c,0x2ae8a8},{0x70,0x328108},{0x7c,0x2ae8b8},{0x80,0x328110}}}){const auto text=::port::race::map_detail::text(m,item.second);m.word(a+item.first,GetTextureInfo(m,hp2::frontend::bStringHash(text.c_str()),1,0));}
 if(!m.word(a+0x64)||!m.word(a+0x68)||!m.word(a+0x6c)||!m.word(a+0x70))m.word(a+0x9c,0);if(!m.word(0x311f78+0x28)&&!m.word(a+0x7c))m.word(a+0x9c,0);eIdentity(m,a+0x20);auto matrix=m.matrix(a+0x20);hp2::recovered::eRotateZ(m,matrix,matrix,m.half(track+0x68));m.matrix(a+0x20,matrix);
}
// Debug001B81E0; E-0112-M01.
void TrackMap::Sub_001B81E0(int flags){if(flags&1)m_.release(address_);}
// Debug001BA610; E-0112-M01. Exact original assignment order and source bits.
void TrackMap::InitTrackMapModeInfo(){auto& m=m_;
 m.half(0x30b7b0,std::uint16_t(-0x2a26)); //ModeInfo[0].AngleX
 m.half(0x30b7f4,std::uint16_t(-0x2a26)); //ModeInfo[1].AngleX
 m.scalar(0x30b7e0,m.scalar(0x3258ac)); //ModeInfo[0].StartFinishLineWidth
 m.scalar(0x30b7e4,m.scalar(0x3258b0)); //ModeInfo[0].StartFinishLineHeight
 m.scalar(0x30b804,190.0f); //ModeInfo[1].Height
 m.scalar(0x30b814,m.scalar(0x3258a0)); //ModeInfo[1].ViewDistanceForward
 m.scalar(0x30b818,m.scalar(0x3258a4)); //ModeInfo[1].ViewDistanceBackward
 m.scalar(0x30b820,m.scalar(0x3258a8)); //ModeInfo[1].CarDotSize
 m.scalar(0x30b824,m.scalar(0x3258b4)); //ModeInfo[1].StartFinishLineWidth
 m.half(0x30b7fe,std::uint16_t(0xe4)); //ModeInfo[1].Y
 m.byte(0x30bab6,64); //MarkerInfo[7].Green
 m.scalar(0x30b7bc,190.0f); //ModeInfo[0].Width
 m.scalar(0x30b7c0,190.0f); //ModeInfo[0].Height
 m.scalar(0x30b7d0,m.scalar(0x3258a0)); //ModeInfo[0].ViewDistanceForward
 m.scalar(0x30b7d4,m.scalar(0x3258a4)); //ModeInfo[0].ViewDistanceBackward
 m.scalar(0x30b7dc,m.scalar(0x3258a8)); //ModeInfo[0].CarDotSize
 m.scalar(0x30b7b4,200.0f); //ModeInfo[0].H
 m.half(0x30b7b8,std::uint16_t(0x5c)); //ModeInfo[0].X
 m.half(0x30b7ba,std::uint16_t(0xe4)); //ModeInfo[0].Y
 m.byte(0x30b7d8,0); //ModeInfo[0].ZoomToKeepPursuedCarInView
 m.byte(0x30b7e8,1); //ModeInfo[0].HasRotation
 m.byte(0x30b7e9,1); //ModeInfo[0].HasScrolling
 m.byte(0x30b7ea,1); //ModeInfo[0].HasRotatingCarDots
 m.scalar(0x30b7ec,1.0f); //ModeInfo[0].Alpha
 m.byte(0x30b7f0,1); //ModeInfo[0].IsDrawn
 m.scalar(0x30b7c8,139.0f); //ModeInfo[0].ClippingWidth
 m.scalar(0x30b7cc,m.scalar(0x3258b8)); //ModeInfo[0].ClippingHeight
 m.scalar(0x30b800,190.0f); //ModeInfo[1].Width
 m.byte(0x30b7f1,1); //ModeInfo[0].HasBorder
 m.half(0x30b7c4,std::uint16_t(0x5c)); //ModeInfo[0].ClippingX
 m.half(0x30b7c6,std::uint16_t(0xf0)); //ModeInfo[0].ClippingY
 m.scalar(0x30b7f8,200.0f); //ModeInfo[1].H
 m.half(0x30b7fc,std::uint16_t(0x5c)); //ModeInfo[1].X
 m.byte(0x30b81c,0); //ModeInfo[1].ZoomToKeepPursuedCarInView
 m.scalar(0x30b828,m.scalar(0x3258bc)); //ModeInfo[1].StartFinishLineHeight
 m.scalar(0x30b80c,139.0f); //ModeInfo[1].ClippingWidth
 m.scalar(0x30b810,m.scalar(0x3258b8)); //ModeInfo[1].ClippingHeight
 m.half(0x30b808,std::uint16_t(0x5c)); //ModeInfo[1].ClippingX
 m.half(0x30b80a,std::uint16_t(0xf0)); //ModeInfo[1].ClippingY
 m.byte(0x30b82c,0); //ModeInfo[1].HasRotation
 m.byte(0x30b82d,1); //ModeInfo[1].HasScrolling
 m.byte(0x30b82e,1); //ModeInfo[1].HasRotatingCarDots
 m.scalar(0x30b830,1.0f); //ModeInfo[1].Alpha
 m.byte(0x30b834,1); //ModeInfo[1].IsDrawn
 m.byte(0x30b835,1); //ModeInfo[1].HasBorder
 m.half(0x30b838,std::uint16_t(-1)); //ModeInfo[2].AngleX
 m.scalar(0x30b83c,200.0f); //ModeInfo[2].H
 m.half(0x30b840,std::uint16_t(0x48)); //ModeInfo[2].X
 m.half(0x30b842,std::uint16_t(0xf3)); //ModeInfo[2].Y
 m.scalar(0x30b844,130.0f); //ModeInfo[2].Width
 m.scalar(0x30b848,130.0f); //ModeInfo[2].Height
 m.scalar(0x30b858,m.scalar(0x3258c0)); //ModeInfo[2].ViewDistanceForward
 m.scalar(0x30b85c,m.scalar(0x3258c0)); //ModeInfo[2].ViewDistanceBackward
 m.byte(0x30b860,0); //ModeInfo[2].ZoomToKeepPursuedCarInView
 m.scalar(0x30b864,m.scalar(0x3258c4)); //ModeInfo[2].CarDotSize
 m.scalar(0x30b868,m.scalar(0x3258c8)); //ModeInfo[2].StartFinishLineWidth
 m.scalar(0x30b86c,m.scalar(0x3258cc)); //ModeInfo[2].StartFinishLineHeight
 m.byte(0x30b870,1); //ModeInfo[2].HasRotation
 m.byte(0x30b871,1); //ModeInfo[2].HasScrolling
 m.byte(0x30b872,1); //ModeInfo[2].HasRotatingCarDots
 m.scalar(0x30b874,1.0f); //ModeInfo[2].Alpha
 m.byte(0x30b878,1); //ModeInfo[2].IsDrawn
 m.byte(0x30b879,1); //ModeInfo[2].HasBorder
 m.scalar(0x30b850,100.0f); //ModeInfo[2].ClippingWidth
 m.scalar(0x30b854,100.0f); //ModeInfo[2].ClippingHeight
 m.half(0x30b84c,std::uint16_t(0x48)); //ModeInfo[2].ClippingX
 m.half(0x30b84e,std::uint16_t(0xf3)); //ModeInfo[2].ClippingY
 m.half(0x30b87c,std::uint16_t(-1)); //ModeInfo[3].AngleX
 m.scalar(0x30b880,200.0f); //ModeInfo[3].H
 m.scalar(0x30b8ac,m.scalar(0x3258d0)); //ModeInfo[3].StartFinishLineWidth
 m.scalar(0x30b8b0,m.scalar(0x3258d4)); //ModeInfo[3].StartFinishLineHeight
 m.half(0x30b884,std::uint16_t(0x48)); //ModeInfo[3].X
 m.half(0x30b886,std::uint16_t(0xf3)); //ModeInfo[3].Y
 m.scalar(0x30b888,130.0f); //ModeInfo[3].Width
 m.scalar(0x30b88c,130.0f); //ModeInfo[3].Height
 m.scalar(0x30b89c,m.scalar(0x3258c0)); //ModeInfo[3].ViewDistanceForward
 m.scalar(0x30b8a0,m.scalar(0x3258c0)); //ModeInfo[3].ViewDistanceBackward
 m.byte(0x30b8a4,0); //ModeInfo[3].ZoomToKeepPursuedCarInView
 m.scalar(0x30b8a8,m.scalar(0x3258c4)); //ModeInfo[3].CarDotSize
 m.byte(0x30b8b4,0); //ModeInfo[3].HasRotation
 m.byte(0x30b8b5,1); //ModeInfo[3].HasScrolling
 m.byte(0x30b8b6,1); //ModeInfo[3].HasRotatingCarDots
 m.scalar(0x30b8b8,1.0f); //ModeInfo[3].Alpha
 m.byte(0x30b8bc,1); //ModeInfo[3].IsDrawn
 m.byte(0x30b8bd,1); //ModeInfo[3].HasBorder
 m.scalar(0x30b894,100.0f); //ModeInfo[3].ClippingWidth
 m.scalar(0x30b898,100.0f); //ModeInfo[3].ClippingHeight
 m.half(0x30b890,std::uint16_t(0x48)); //ModeInfo[3].ClippingX
 m.half(0x30b892,std::uint16_t(0xf3)); //ModeInfo[3].ClippingY
 m.half(0x30b8c0,std::uint16_t(-1)); //ModeInfo[4].AngleX
 m.scalar(0x30b8c4,200.0f); //ModeInfo[4].H
 m.half(0x30b8c8,std::uint16_t(0x4f)); //ModeInfo[4].X
 m.half(0x30b8ca,std::uint16_t(0x100)); //ModeInfo[4].Y
 m.scalar(0x30b8cc,130.0f); //ModeInfo[4].Width
 m.scalar(0x30b8d0,130.0f); //ModeInfo[4].Height
 m.scalar(0x30b8e0,1.0f); //ModeInfo[4].ViewDistanceForward
 m.scalar(0x30b8e4,1.0f); //ModeInfo[4].ViewDistanceBackward
 m.byte(0x30b8e8,0); //ModeInfo[4].ZoomToKeepPursuedCarInView
 m.scalar(0x30b8ec,m.scalar(0x3258d8)); //ModeInfo[4].CarDotSize
 m.scalar(0x30b8f0,m.scalar(0x3258dc)); //ModeInfo[4].StartFinishLineWidth
 m.scalar(0x30b8f4,m.scalar(0x3258e0)); //ModeInfo[4].StartFinishLineHeight
 m.byte(0x30b8f8,0); //ModeInfo[4].HasRotation
 m.byte(0x30b8f9,0); //ModeInfo[4].HasScrolling
 m.half(0x30b8d6,std::uint16_t(0xda)); //ModeInfo[4].ClippingY
 m.byte(0x30b8fa,0); //ModeInfo[4].HasRotatingCarDots
 m.scalar(0x30b8fc,1.0f); //ModeInfo[4].Alpha
 m.byte(0x30b900,1); //ModeInfo[4].IsDrawn
 m.byte(0x30b901,0); //ModeInfo[4].HasBorder
 m.scalar(0x30b8d8,1000.0f); //ModeInfo[4].ClippingWidth
 m.scalar(0x30b8dc,1000.0f); //ModeInfo[4].ClippingHeight
 m.half(0x30b8d4,std::uint16_t(0x4f)); //ModeInfo[4].ClippingX
 m.half(0x30b904,std::uint16_t(-1)); //ModeInfo[5].AngleX
 m.scalar(0x30b908,200.0f); //ModeInfo[5].H
 m.half(0x30b90c,std::uint16_t(0x140)); //ModeInfo[5].X
 m.half(0x30b90e,std::uint16_t(0xf3)); //ModeInfo[5].Y
 m.scalar(0x30b910,262.0f); //ModeInfo[5].Width
 m.scalar(0x30b914,262.0f); //ModeInfo[5].Height
 m.scalar(0x30b924,1.0f); //ModeInfo[5].ViewDistanceForward
 m.scalar(0x30b928,1.0f); //ModeInfo[5].ViewDistanceBackward
 m.byte(0x30b92c,0); //ModeInfo[5].ZoomToKeepPursuedCarInView
 m.scalar(0x30b930,m.scalar(0x3258e4)); //ModeInfo[5].CarDotSize
 m.scalar(0x30b934,m.scalar(0x3258e8)); //ModeInfo[5].StartFinishLineWidth
 m.scalar(0x30b938,m.scalar(0x3258ec)); //ModeInfo[5].StartFinishLineHeight
 m.byte(0x30b93c,0); //ModeInfo[5].HasRotation
 m.byte(0x30b93d,0); //ModeInfo[5].HasScrolling
 m.byte(0x30b93e,1); //ModeInfo[5].HasRotatingCarDots
 m.scalar(0x30b940,m.scalar(0x3258f0)); //ModeInfo[5].Alpha
 m.byte(0x30b944,1); //ModeInfo[5].IsDrawn
 m.byte(0x30b945,0); //ModeInfo[5].HasBorder
 m.scalar(0x30b91c,1000.0f); //ModeInfo[5].ClippingWidth
 m.scalar(0x30b920,1000.0f); //ModeInfo[5].ClippingHeight
 m.half(0x30b918,std::uint16_t(0x140)); //ModeInfo[5].ClippingX
 m.half(0x30b91a,std::uint16_t(0xf3)); //ModeInfo[5].ClippingY
 m.half(0x30b98c,std::uint16_t(-1)); //ModeInfo[7].AngleX
 m.scalar(0x30b990,200.0f); //ModeInfo[7].H
 m.half(0x30b994,std::uint16_t(0x53)); //ModeInfo[7].X
 m.half(0x30b996,std::uint16_t(0xf5)); //ModeInfo[7].Y
 m.half(0x30b9da,std::uint16_t(0x100)); //ModeInfo[8].Y
 m.scalar(0x30b9a8,140.0f); //ModeInfo[7].ClippingHeight
 m.half(0x30b9a0,std::uint16_t(0x53)); //ModeInfo[7].ClippingX
 m.half(0x30b9a2,std::uint16_t(0xf5)); //ModeInfo[7].ClippingY
 m.scalar(0x30b9fc,m.scalar(0x3258d8)); //ModeInfo[8].CarDotSize
 m.scalar(0x30ba00,m.scalar(0x3258dc)); //ModeInfo[8].StartFinishLineWidth
 m.scalar(0x30ba04,m.scalar(0x3258e0)); //ModeInfo[8].StartFinishLineHeight
 m.scalar(0x30b998,130.0f); //ModeInfo[7].Width
 m.scalar(0x30b99c,130.0f); //ModeInfo[7].Height
 m.scalar(0x30b9ac,1.0f); //ModeInfo[7].ViewDistanceForward
 m.scalar(0x30b9b0,1.0f); //ModeInfo[7].ViewDistanceBackward
 m.byte(0x30b9b4,0); //ModeInfo[7].ZoomToKeepPursuedCarInView
 m.scalar(0x30b9b8,m.scalar(0x3258d8)); //ModeInfo[7].CarDotSize
 m.scalar(0x30b9bc,m.scalar(0x3258dc)); //ModeInfo[7].StartFinishLineWidth
 m.scalar(0x30b9c0,m.scalar(0x3258e0)); //ModeInfo[7].StartFinishLineHeight
 m.byte(0x30b9c4,0); //ModeInfo[7].HasRotation
 m.byte(0x30b9c5,0); //ModeInfo[7].HasScrolling
 m.byte(0x30b9c6,0); //ModeInfo[7].HasRotatingCarDots
 m.scalar(0x30b9c8,1.0f); //ModeInfo[7].Alpha
 m.byte(0x30b9cc,1); //ModeInfo[7].IsDrawn
 m.byte(0x30b9cd,0); //ModeInfo[7].HasBorder
 m.scalar(0x30b9a4,140.0f); //ModeInfo[7].ClippingWidth
 m.half(0x30b9d0,std::uint16_t(-1)); //ModeInfo[8].AngleX
 m.scalar(0x30b9d4,200.0f); //ModeInfo[8].H
 m.half(0x30b9d8,std::uint16_t(0x4f)); //ModeInfo[8].X
 m.scalar(0x30b9dc,130.0f); //ModeInfo[8].Width
 m.scalar(0x30b9e0,130.0f); //ModeInfo[8].Height
 m.scalar(0x30b9f0,1.0f); //ModeInfo[8].ViewDistanceForward
 m.scalar(0x30b9f4,1.0f); //ModeInfo[8].ViewDistanceBackward
 m.byte(0x30b9f8,0); //ModeInfo[8].ZoomToKeepPursuedCarInView
 m.byte(0x30ba08,0); //ModeInfo[8].HasRotation
 m.byte(0x30ba09,0); //ModeInfo[8].HasScrolling
 m.byte(0x30ba0a,0); //ModeInfo[8].HasRotatingCarDots
 m.scalar(0x30ba0c,1.0f); //ModeInfo[8].Alpha
 m.half(0x30b9e6,std::uint16_t(0xda)); //ModeInfo[8].ClippingY
 m.half(0x30b9e4,std::uint16_t(0x4f)); //ModeInfo[8].ClippingX
 m.scalar(0x30ba24,262.0f); //ModeInfo[9].Height
 m.scalar(0x30ba40,m.scalar(0x3258e4)); //ModeInfo[9].CarDotSize
 m.scalar(0x30ba44,m.scalar(0x3258e8)); //ModeInfo[9].StartFinishLineWidth
 m.scalar(0x30ba48,m.scalar(0x3258ec)); //ModeInfo[9].StartFinishLineHeight
 m.scalar(0x30ba50,m.scalar(0x3258f0)); //ModeInfo[9].Alpha
 m.scalar(0x30ba30,1000.0f); //ModeInfo[9].ClippingHeight
 m.half(0x30ba28,std::uint16_t(0x140)); //ModeInfo[9].ClippingX
 m.half(0x30ba58,std::uint16_t(-1)); //ModeInfo[10].AngleX
 m.scalar(0x30ba5c,200.0f); //ModeInfo[10].H
 m.byte(0x30ba10,1); //ModeInfo[8].IsDrawn
 m.byte(0x30ba11,0); //ModeInfo[8].HasBorder
 m.scalar(0x30b9e8,1000.0f); //ModeInfo[8].ClippingWidth
 m.scalar(0x30b9ec,1000.0f); //ModeInfo[8].ClippingHeight
 m.half(0x30ba14,std::uint16_t(-1)); //ModeInfo[9].AngleX
 m.scalar(0x30ba18,200.0f); //ModeInfo[9].H
 m.half(0x30ba1c,std::uint16_t(0x140)); //ModeInfo[9].X
 m.half(0x30ba1e,std::uint16_t(0xf3)); //ModeInfo[9].Y
 m.scalar(0x30ba20,262.0f); //ModeInfo[9].Width
 m.scalar(0x30ba34,1.0f); //ModeInfo[9].ViewDistanceForward
 m.scalar(0x30ba38,1.0f); //ModeInfo[9].ViewDistanceBackward
 m.byte(0x30ba3c,0); //ModeInfo[9].ZoomToKeepPursuedCarInView
 m.byte(0x30ba4c,0); //ModeInfo[9].HasRotation
 m.byte(0x30ba4d,0); //ModeInfo[9].HasScrolling
 m.byte(0x30ba4e,1); //ModeInfo[9].HasRotatingCarDots
 m.byte(0x30ba54,1); //ModeInfo[9].IsDrawn
 m.byte(0x30ba55,0); //ModeInfo[9].HasBorder
 m.scalar(0x30ba2c,1000.0f); //ModeInfo[9].ClippingWidth
 m.half(0x30ba2a,std::uint16_t(0xf3)); //ModeInfo[9].ClippingY
 m.half(0x30ba60,std::uint16_t(0x48)); //ModeInfo[10].X
 m.half(0x30ba62,std::uint16_t(0xf3)); //ModeInfo[10].Y
 m.scalar(0x30ba64,130.0f); //ModeInfo[10].Width
 m.scalar(0x30ba68,130.0f); //ModeInfo[10].Height
 m.scalar(0x30ba7c,m.scalar(0x3258c0)); //ModeInfo[10].ViewDistanceBackward
 m.scalar(0x30ba84,m.scalar(0x3258c4)); //ModeInfo[10].CarDotSize
 m.scalar(0x30ba88,m.scalar(0x3258c8)); //ModeInfo[10].StartFinishLineWidth
 m.scalar(0x30ba8c,m.scalar(0x3258cc)); //ModeInfo[10].StartFinishLineHeight
 m.scalar(0x30ba94,1.0f); //ModeInfo[10].Alpha
 m.byte(0x30ba99,1); //ModeInfo[10].HasBorder
 m.scalar(0x30ba74,100.0f); //ModeInfo[10].ClippingHeight
 m.half(0x30ba6c,std::uint16_t(0x48)); //ModeInfo[10].ClippingX
 m.half(0x30ba6e,std::uint16_t(0xf3)); //ModeInfo[10].ClippingY
 m.scalar(0x30ba78,m.scalar(0x3258c0)); //ModeInfo[10].ViewDistanceForward
 m.byte(0x30ba80,1); //ModeInfo[10].ZoomToKeepPursuedCarInView
 m.byte(0x30ba90,1); //ModeInfo[10].HasRotation
 m.byte(0x30ba91,1); //ModeInfo[10].HasScrolling
 m.byte(0x30ba92,1); //ModeInfo[10].HasRotatingCarDots
 m.byte(0x30ba98,1); //ModeInfo[10].IsDrawn
 m.scalar(0x30ba70,100.0f); //ModeInfo[10].ClippingWidth
 m.half(0x30b948,std::uint16_t(0)); //ModeInfo[6].AngleX
 m.scalar(0x30b94c,0.0f); //ModeInfo[6].H
 m.half(0x30b950,std::uint16_t(0)); //ModeInfo[6].X
 m.half(0x30b952,std::uint16_t(0)); //ModeInfo[6].Y
 m.scalar(0x30b954,0.0f); //ModeInfo[6].Width
 m.scalar(0x30b958,0.0f); //ModeInfo[6].Height
 m.scalar(0x30b968,0.0f); //ModeInfo[6].ViewDistanceForward
 m.scalar(0x30b96c,0.0f); //ModeInfo[6].ViewDistanceBackward
 m.byte(0x30b970,0); //ModeInfo[6].ZoomToKeepPursuedCarInView
 m.scalar(0x30b974,0.0f); //ModeInfo[6].CarDotSize
 m.scalar(0x30b978,0.0f); //ModeInfo[6].StartFinishLineWidth
 m.scalar(0x30b97c,0.0f); //ModeInfo[6].StartFinishLineHeight
 m.byte(0x30b980,0); //ModeInfo[6].HasRotation
 m.byte(0x30b981,0); //ModeInfo[6].HasScrolling
 m.byte(0x30b982,0); //ModeInfo[6].HasRotatingCarDots
 m.scalar(0x30b984,0.0f); //ModeInfo[6].Alpha
 m.byte(0x30b988,0); //ModeInfo[6].IsDrawn
 m.byte(0x30b989,0); //ModeInfo[6].HasBorder
 m.byte(0x30bab7,122); //MarkerInfo[7].Blue
 m.half(0x30b95e,std::uint16_t(0)); //ModeInfo[6].ClippingY
 m.byte(0x30baa0,0xac); //MarkerInfo[0].Red
 m.byte(0x30baa3,75); //MarkerInfo[1].Red
 m.byte(0x30baa8,0xff); //MarkerInfo[2].Blue
 m.byte(0x30baba,0xc0); //MarkerInfo[8].Blue
 m.byte(0x30bab2,0xac); //MarkerInfo[6].Red
 m.byte(0x30bab3,53); //MarkerInfo[6].Green
 m.byte(0x30baa4,85); //MarkerInfo[1].Green
 m.byte(0x30baaa,100); //MarkerInfo[3].Green
 m.byte(0x30bab0,0x80); //MarkerInfo[5].Green
 m.scalar(0x30b960,0.0f); //ModeInfo[6].ClippingWidth
 m.scalar(0x30b964,0.0f); //ModeInfo[6].ClippingHeight
 m.half(0x30b95c,std::uint16_t(0)); //ModeInfo[6].ClippingX
 m.byte(0x30baa1,53); //MarkerInfo[0].Green
 m.byte(0x30baa5,0xff); //MarkerInfo[1].Blue
 m.byte(0x30baa2,0); //MarkerInfo[0].Blue
 m.byte(0x30baa6,0); //MarkerInfo[2].Red
 m.byte(0x30baa7,0); //MarkerInfo[2].Green
 m.byte(0x30baa9,0); //MarkerInfo[3].Red
 m.byte(0x30baab,0); //MarkerInfo[3].Blue
 m.byte(0x30baac,0); //MarkerInfo[4].Red
 m.byte(0x30baad,0); //MarkerInfo[4].Green
 m.byte(0x30baae,0x80); //MarkerInfo[4].Blue
 m.byte(0x30bab8,0); //MarkerInfo[8].Red
 m.byte(0x30bab9,0); //MarkerInfo[8].Green
 m.byte(0x30baaf,0x80); //MarkerInfo[5].Red
 m.byte(0x30bab1,0); //MarkerInfo[5].Blue
 m.byte(0x30bab4,0); //MarkerInfo[6].Blue
 m.byte(0x30bab5,0); //MarkerInfo[7].Red
}

// Debug001B9E68; E-0112-M01. Source track calibration width and matrix.
std::array<float,2> TrackMap::GetCarUVPosition(const std::array<float,4>& point){const auto track=m_.word(address_+0x8c);const float width=m_.scalar(track+0x64);if(!std::isfinite(width)||width==0)hp2::not_implemented("0x001B9E68","original track-map calibration width requires nonzero finite source data");const std::array<float,4> uv{{(point[0]-m_.scalar(track+0x5c))/width,(-point[1]+m_.scalar(track+0x60))/width,1,0}};const auto mapped=::port::race::map_detail::transform(m_,m_.matrix(address_+0x20),uv);return {{mapped[0],mapped[1]}};}
// Debug001B9F00; E-0112-M01. Source rotation*translation, exact corner order.
std::array<std::array<float,2>,4> TrackMap::GetBoxOnMap(const std::array<float,2>& centre,float forward,float backward,float left,float right,std::uint16_t angle){auto rotation=hp2::recovered::bIdentity(),translation=rotation;hp2::recovered::eRotateZ(m_,rotation,rotation,angle);hp2::recovered::eTranslate(translation,translation,{{centre[0],centre[1],0}});hp2::recovered::eMulMatrix(translation,rotation,translation);const std::array<std::array<float,4>,4> corners{{{{right,forward,0,0}},{{-left,forward,0,0}},{{-left,-backward,0,0}},{{right,-backward,0,0}}}};std::array<std::array<float,2>,4> output;for(unsigned i=0;i<4;++i){const auto value=::port::race::map_detail::transform(m_,translation,corners[i]);output[i]={{value[0],value[1]}};}return output;}
// Debug001BA070; E-0112-M01. Source perspective and per-corner depth alpha.
void TrackMap::MapToScreen(::port::race::HudPolygon& polygon,std::array<std::array<float,4>,4>& points){const auto info=::port::race::map_detail::mode(m_,address_);auto matrix=hp2::recovered::bIdentity();hp2::recovered::eRotateX(m_,matrix,matrix,m_.half(info));for(unsigned i=0;i<4;++i)points[i]=::port::race::map_detail::transform(m_,matrix,points[i]);const float h=m_.scalar(info+4),height=m_.scalar(info+16),x=float(m_.half(info+8)),y=float(m_.half(info+10));for(unsigned i=0;i<4;++i){const auto& point=points[i];const float divisor=point[2]+h;if(!std::isfinite(divisor)||divisor==0)hp2::not_implemented("0x001BA070","source TrackMap perspective division requires finite nonzero denominator");auto& vertex=polygon.vertices[i];vertex.position[0]=(point[0]*h)/divisor+x;vertex.position[1]=(point[1]*h)/divisor+y;const int alpha=point[2]>0?::port::driving::cvt_w_s((1-point[2]/(height*m_.scalar(0x32588c+4*i)))*128):128;vertex.colour=(vertex.colour&0xffffff)|(std::uint32_t(std::uint8_t(alpha))<<24);}}
// Debug001BA4C8; E-0112-M01. Actual view's front-most source camera mover.
int TrackMap::GetCameraDirection(std::array<float,4>& direction){auto& m=m_;const auto player=m.word(address_+0x88),view=::port::race::eGetView(m,m.signed_word(player+0x14)),head=view+0x3c;const auto first=m.word(head),mover=first==head?0:first;if(!mover||m.word(mover+8)!=1){const auto race=m.word(0x327258);if(!race)hp2::not_implemented("0x001BA4C8","original TrackMap requires Race owner");if(m.word(race)==4){direction[0]=m.scalar(address_);direction[1]=m.scalar(address_+4);}else{const auto car=m.word(address_+0x84);direction[0]=m.scalar(car+0x90);direction[1]=m.scalar(car+0x94);}}else{const auto camera=m.word(mover+0x18);for(unsigned i=0;i<3;++i)direction[i]=m.scalar(camera+0x50+4*i);auto rotation=hp2::recovered::bIdentity();hp2::recovered::eRotateZ(m,rotation,rotation,m.half(mover+0x8c));direction=::port::race::map_detail::transform(m,rotation,direction);if(std::fabs(m.scalar(address_))<=m.scalar(0x32589c)&&std::fabs(m.scalar(address_+4))<=m.scalar(0x32589c)){m.scalar(address_,direction[0]);m.scalar(address_+4,direction[1]);m.word(address_+16,m.word(0x327cf8));}}return 1;}
// Debug001B8A90; E-0112-M01. Source cubic source WorldTimer smoothing.
int TrackMap::GetAngle(std::uint16_t& angle){const auto info=::port::race::map_detail::mode(m_,address_);if(!m_.byte(info+0x38)){angle=0x8000;return 1;}std::array<float,4> direction{};if(!GetCameraDirection(direction))return 0;float x=m_.scalar(address_),y=m_.scalar(address_+4),sx=m_.scalar(address_+8),sy=m_.scalar(address_+12);const float t=(float(std::int32_t(m_.word(0x327cf8)-m_.word(address_+16)))*m_.scalar(0x325878))*m_.scalar(0x32587c);::port::race::map_detail::cubic(x,sx,direction[0],t);::port::race::map_detail::cubic(y,sy,direction[1],t);m_.scalar(address_,x);m_.scalar(address_+4,y);m_.scalar(address_+8,sx);m_.scalar(address_+12,sy);m_.word(address_+16,m_.word(0x327cf8));angle=std::uint16_t(std::uint16_t(hp2::recovered::bATan(m_,y,x))-0x8000);return 1;}
// Debug001B9558/001B9E20; E-0112-M01. Original Sutherland-style edge order,
// source integer-alpha interpolation, projective UV.q and quad fan batching.
void TrackMap::ClipPolygon(const ::port::race::HudPolygon& polygon,std::uint32_t texture,int layer,std::vector<::port::race::HudPolygon>& output){using namespace ::port::race;using namespace ::port::race::map_detail;const auto info=mode(m_,address_);const float x=float(m_.half(info+20)),y=float(m_.half(info+22)),w=m_.scalar(info+24)*.5f,h=m_.scalar(info+28)*.5f;const Quad clip{{{{x-w,y-h}},{{x+w,y-h}},{{x+w,y+h}},{{x-w,y+h}}}};struct Point {V2 xy;std::array<float,3> uv;std::uint8_t alpha;};std::vector<Point> input;for(const auto& v:polygon.vertices)input.push_back({{{v.position[0],v.position[1]}},v.uvq,std::uint8_t(v.colour>>24)});std::vector<Point> result;
 unsigned previous_edge=3;for(unsigned edge=0;edge<4;++edge){result.clear();if(!input.empty()){unsigned previous=unsigned(input.size()-1);for(unsigned current=0;current<input.size();++current){const auto& a=input[current];const auto& b=input[previous];const auto& p0=clip[edge];const auto& p1=clip[previous_edge];const bool a_inside=side(p0,p1,a.xy)!=-1,b_inside=side(p0,p1,b.xy)!=-1;if(b_inside)result.push_back(b);if(a_inside!=b_inside){const float dx=p1[0]-p0[0],dy=p1[1]-p0[1],line_x=b.xy[0]-a.xy[0];const float denominator=line_x*dy-(b.xy[1]-a.xy[1])*dx;if(!std::isfinite(denominator)||denominator==0)hp2::not_implemented("0x001B9558","source map clip intersection division outside finite nonzero domain");const float t=((a.xy[1]-p0[1])*dx-(a.xy[0]-p0[0])*dy)/denominator;Point point;point.xy={{a.xy[0]+t*line_x,a.xy[1]+t*(b.xy[1]-a.xy[1])}};for(unsigned j=0;j<3;++j)point.uv[j]=a.uv[j]+t*(b.uv[j]-a.uv[j]);point.alpha=std::uint8_t(::port::driving::cvt_w_s(float(a.alpha)+t*float(int(b.alpha)-int(a.alpha))));result.push_back(point);}if(result.size()>8)hp2::not_implemented("0x001B9E20","source TrackMap clipping output exceeds original eight-point arrays");previous=current;}}input=result;previous_edge=edge;}
 unsigned top=0;for(unsigned i=1;i<result.size();++i){if(result[top].xy[1]<=result[i].xy[1]){if(std::fabs(result[i].xy[1]-result[top].xy[1])<=m_.scalar(0x325888)&&result[i].xy[0]<result[top].xy[0])top=i;}else top=i;}
 unsigned current=1;while(result.size()>=3&&current<result.size()-1){auto fan=::port::race::map_detail::polygon();fan.flailer=true;fan.layer=layer;fan.texture_hash=texture_hash(m_,texture);const auto set=[&](unsigned slot,unsigned index){const auto& point=result[index];fan.vertices[slot].position={{point.xy[0],point.xy[1],float(layer)}};fan.vertices[slot].uvq=point.uv;fan.vertices[slot].colour=(polygon.vertices[slot].colour&0xffffff)|(std::uint32_t(point.alpha)<<24);};set(0,top);unsigned last=current;for(unsigned i=1;i<4;++i){last=std::min(current+i-1,unsigned(result.size()-1));const unsigned index=(last+top)%unsigned(result.size());set(i,index);}output.push_back(fan);current=last;}
}

// Debug001B8BC0; E-0112-M01. Original scrolling/non-scrolling texture geometry.
void TrackMap::DrawMap(std::array<float,2>& centre,std::array<std::array<float,2>,4>& visible,std::uint16_t angle,std::vector<::port::race::HudPolygon>& output){using namespace ::port::race::map_detail;const auto info=mode(m_,address_);if(!m_.byte(info+0x40))return;auto map=polygon();const auto player_uv=GetCarUVPosition(m_.vector(m_.word(address_+0x84)+0x60));Quad uvs;if(!m_.byte(info+0x39)){uvs=Quad{{{{0,0}},{{1,0}},{{1,1}},{{0,1}}}};visible=uvs;}else{float forward,backward,side_distance;const auto pursued=m_.word(address_+0x90);if(!m_.byte(info+0x28)||!pursued){backward=m_.scalar(info+0x24);forward=m_.scalar(info+0x20);side_distance=(forward+backward)*.5f;}else{const auto own=m_.word(address_+0x84),track=m_.word(address_+0x8c);const float y=m_.scalar(own+0x64)-m_.scalar(pursued+0x64),x=m_.scalar(own+0x60)-m_.scalar(pursued+0x60);const float width=m_.scalar(track+0x64),clip_width=m_.scalar(info+0x18);if(width==0||clip_width==0)hp2::not_implemented("0x001B8BC0","source pursued-map zoom requires nonzero calibration/clipping width");backward=(std::sqrt(x*x+y*y)/width)*(m_.scalar(info+0xc)/clip_width)+m_.scalar(0x325880);forward=side_distance=backward;}m_.scalar(address_+0x94,forward);m_.scalar(address_+0x98,backward);uvs=GetBoxOnMap(player_uv,forward,backward,side_distance,side_distance,angle);const float expand=m_.scalar(0x325884);visible=GetBoxOnMap(player_uv,forward*expand,backward*expand,side_distance*expand,side_distance*expand,angle);}
 for(unsigned i=0;i<4;++i){map.vertices[i].uvq[0]=uvs[i][0];map.vertices[i].uvq[1]=uvs[i][1];}const float third=m_.scalar(0x3258f4);centre[0]=(((uvs[0][0]+uvs[1][0]+uvs[2][0])*third)+((uvs[0][0]+uvs[2][0]+uvs[3][0])*third))*.5f;centre[1]=(((uvs[0][1]+uvs[1][1]+uvs[2][1])*third)+((uvs[0][1]+uvs[2][1]+uvs[3][1])*third))*.5f;const float w=m_.scalar(info+12)*.5f,h=m_.scalar(info+16)*.5f;std::array<V3,4> points{{{{-w,-h,0,0}},{{w,-h,0,0}},{{w,h,0,0}},{{-w,h,0,0}}}};MapToScreen(map,points);const float alpha=m_.scalar(info+0x3c),projection=m_.scalar(info+4);for(unsigned i=0;i<4;++i){auto& vertex=map.vertices[i];const auto scaled=std::uint8_t(::port::driving::cvt_w_s(float(std::uint8_t(vertex.colour>>24))*alpha));vertex.colour=(vertex.colour&0xffffff)|(std::uint32_t(scaled)<<24);const float divisor=points[i][2]+projection;if(divisor==0)hp2::not_implemented("0x001B8BC0","source map projective UV division by zero");const float inverse=1/divisor;for(auto& component:vertex.uvq)component*=inverse;}ClipPolygon(map,m_.word(address_+0x60),14,output);}
// Debug001B9068; E-0112-M01. Source marker point test and RGB-only replacement.
void TrackMap::DrawMarker(const std::array<float,4>& world,const std::array<float,2>& centre,std::uint16_t angle,const std::array<std::array<float,2>,4>& visible,std::uint32_t colour,int layer,std::uint32_t texture,std::uint16_t rotation,float width,float height,std::vector<::port::race::HudPolygon>& output){using namespace ::port::race::map_detail;const auto info=mode(m_,address_);if(!m_.byte(info+0x40))return;const auto uv=GetCarUVPosition(world);if(!inside(uv,visible))return;auto marker=polygon();auto matrix=hp2::recovered::bIdentity();hp2::recovered::eRotateZ(m_,matrix,matrix,std::uint16_t(0-angle));const V3 offset{{uv[0]-centre[0],uv[1]-centre[1],0,0}};const auto temp=transform(m_,matrix,offset);V2 position{{temp[0],temp[1]}};if(m_.byte(info+0x39)){const float distance=m_.scalar(address_+0x94)+m_.scalar(address_+0x98);if(distance==0)hp2::not_implemented("0x001B9068","source marker scroll distance is zero");position[0]=temp[0]/distance;position[1]=temp[1]/distance;}const auto box=GetBoxOnMap(position,height*.5f,height*.5f,width*.5f,width*.5f,rotation);std::array<V3,4> points;for(unsigned i=0;i<4;++i)points[i]={{-box[i][0]*m_.scalar(info+12),-box[i][1]*m_.scalar(info+16),0,0}};MapToScreen(marker,points);const std::uint32_t rgb=std::uint32_t(m_.byte(colour))|(std::uint32_t(m_.byte(colour+1))<<8)|(std::uint32_t(m_.byte(colour+2))<<16);for(auto& vertex:marker.vertices)vertex.colour=(vertex.colour&0xff000000)|rgb;ClipPolygon(marker,texture,layer,output);}
// Debug001B93A0; E-0112-M01. Border is an authored un-clipped original ePoly.
void TrackMap::DrawOverOrUnderLay(std::uint32_t texture,int layer,std::vector<::port::race::HudPolygon>& output){using namespace ::port::race::map_detail;const auto info=mode(m_,address_);if(!m_.byte(info+0x41))return;auto border=polygon();const float x=float(m_.half(info+20)),y=float(m_.half(info+22)),w=m_.scalar(info+24)*.5f,h=m_.scalar(info+28)*.5f;border.vertices[0].position={{x-w,y-h,float(layer)}};border.vertices[1].position={{x+w,y-h,float(layer)}};border.vertices[2].position={{x+w,y+h,float(layer)}};border.vertices[3].position={{x-w,y+h,float(layer)}};if(m_.byte(info+0x40)){border.flailer=true;border.layer=layer;border.texture_hash=texture_hash(m_,texture);output.push_back(border);}}

// Debug001B8200; E-0112-M01. Actual source World actors, pursuits, roadblocks,
// ghost and player ordering. Carrier contains original geometry, not host icons.
void TrackMap::Render(std::vector<::port::race::HudPolygon>& output){auto& m=m_;const auto a=address_,world=m.word(0x3272f0),race=m.word(0x327258);if(!m.word(a+0x9c)||!world||!race)return;m.word(a+0x90,0);m.word(a+0xa4,m.word(a+0xa0));const auto manager=m.word(0x328c1c);if(m.word(a+0xa0)!=6&&manager){const int index=m.signed_word(manager+12);std::uint32_t cop=0;if(index!=-1){if(index<0||index>=16)hp2::not_implemented("0x001B8200","source player cop index outside original16 slots");cop=m.word(manager+16+4*unsigned(index));}if(cop)if(const auto pursuit=m.word(cop)){const auto chased=m.word(pursuit+16);m.word(a+0x90,chased);if(chased)m.word(a+0xa4,10);}}
 std::array<float,2> centre{{0,0}};std::array<std::array<float,2>,4> visible;std::uint16_t angle;if(!GetAngle(angle))return;DrawOverOrUnderLay(m.word(a+0x70),15,output);DrawMap(centre,visible,angle,output);const auto track=m.word(a+0x8c),info=::port::race::map_detail::mode(m,a);const auto line_angle=std::uint16_t(m.half(track+0x68)-angle);const float line_width=m.scalar(info+0x30),line_height=m.scalar(info+0x34);DrawMarker({{m.scalar(race+0x668),m.scalar(race+0x66c),0,0}},centre,angle,visible,0x30baa0+3*6,13,m.word(a+0x7c),std::uint16_t(m.half(track+0x6a)+line_angle),line_width,line_height,output);if(m.word(0x311f78+0x28))DrawMarker({{m.scalar(race+0x670),m.scalar(race+0x674),0,0}},centre,angle,visible,0x30baa0+3*6,13,m.word(a+0x7c),std::uint16_t(m.half(track+0x6c)+line_angle),line_width,line_height,output);
 const bool rotate=m.byte(info+0x3a)!=0;const auto marker_texture=m.word(a+(rotate?0x68:0x64));const float size=m.scalar(info+0x2c);const auto car_angle=[&](std::uint32_t car){return rotate?std::uint16_t((m.half(track+0x68)-m.half(car+0x3a4)-angle)+0x4000):std::uint16_t(0);};
 const auto draw_car=[&](std::uint32_t car,int color,int layer){if(!car||!m.byte(car+0x4a8)||!m.word(car+0x86c))return;if(m.word(a+0x90)==car){color=7;layer=3;}DrawMarker(m.vector(car+0x60),centre,angle,visible,0x30baa0+3*unsigned(color),layer,marker_texture,car_angle(car),size,size,output);};
 for(int i=m.signed_word(world+0x20);i<m.signed_word(world+0x24);++i){if(i<0||i>=14)hp2::not_implemented("0x001B8200","source AI map car-table index exceeds14 actors");draw_car(m.word(world+0x2c+4*unsigned(i)),3,5);}
 for(int i=m.signed_word(world+0x24);i<m.signed_word(world+0x28);++i){if(i<0||i>=14)hp2::not_implemented("0x001B8200","source cop map car-table index exceeds14 actors");draw_car(m.word(world+0x2c+4*unsigned(i)),4,6);}
 if(manager){tCopManager cops(m,manager);for(int i=0;i<cops.GetNumActiveRoadBlocks();++i){const auto roadblock=cops.GetActiveRoadBlock(i);if(!roadblock)hp2::not_implemented("0x001B8200","source active roadblock list lookup returned no owner");DrawMarker({{m.scalar(roadblock+0x44),m.scalar(roadblock+0x48),0,0}},centre,angle,visible,0x30baa0+3*8,7,m.word(a+0x80),std::uint16_t((m.half(track+0x68)-m.half(roadblock+0x1c)-angle)+0x4000),line_width,line_height,output);}}
 if(const auto ghost=m.word(race+0x67c)){const auto rotation=rotate?std::uint16_t((m.half(track+0x68)-hp2::recovered::bATan(m,m.scalar(ghost+0x20),m.scalar(ghost+0x24))-angle)+0x4000):std::uint16_t(0);DrawMarker(m.vector(ghost+0x50),centre,angle,visible,0x30baa0+3*5,4,marker_texture,rotation,size,size,output);}
 const auto players=m.signed_word(0x327780);if(players==1){if(const auto player=m.word(0x327778))if(const auto car=m.word(player+4)){const int color=m.word(m.word(car+12)+0x538)==1?2:0;draw_car(car,color,1);}}
 else if(players==2){const auto first=m.word(0x327770),second=m.word(0x327774);if(!first||!second)hp2::not_implemented("0x001B8200","source split map requires both players");const auto car0=m.word(first+4),car1=m.word(second+4),racing0=Race(m,race).GetRacingCar(car0),racing1=Race(m,race).GetRacingCar(car1);if(!racing0||!racing1)hp2::not_implemented("0x001B8200","source split map requires both racing cars");const bool second_ahead=m.signed_byte(racing1+0xa)<m.signed_byte(racing0+0xa);draw_car(car0,m.word(m.word(car0+12)+0x538)==1?2:0,second_ahead?2:1);draw_car(car1,m.word(m.word(car1+12)+0x538)==1?2:1,second_ahead?1:2);}
 DrawOverOrUnderLay(m.word(a+0x6c),0,output);
}
}
