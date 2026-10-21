#include "vehicle_flares.hpp"
#include "projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>
namespace port::driving {
namespace {
// Original Create/Render name hashes; order is explicitly port-owned.
constexpr std::array<std::uint32_t,12> names{{0xacec665c,0x4ae7f96f,0x7efcf06f,
 0x34970013,0x5f09c5e2,0xbea9aee6,0x944e5339,0xa7e6ea53,0xa532fc46,
 0x1e4150b4,0xe662c161,0xb4348dba}};
std::array<float,4> transform(const std::array<float,16>& m,const std::array<float,3>& p,float w){
 std::array<float,4> out{};for(unsigned r=0;r<4;++r)out[r]=((m[r]*p[0]+m[4+r]*p[1])+m[8+r]*p[2])+m[12+r]*w;return out;
}
std::array<float,3> xyz(const std::array<float,4>& p){return {{p[0],p[1],p[2]}};}
}
// Debug ELF815b60c7f473...; E-0086-F01. Original data, not embedded defaults.
VehicleFlareResources read_vehicle_flare_resources(const Memory& m){
 VehicleFlareResources out;
 for(unsigned i=0;i<10;++i){auto& p=out.parameters[i];const auto a=0x002cd740+i*60;
  p.min_size=m.scalar(a);p.max_size=m.scalar(a+4);
  for(unsigned c=0;c<4;++c){p.min_colour[c]=m.scalar(a+8+c*4);p.max_colour[c]=m.scalar(a+24+c*4);}
  p.power=m.scalar(a+40);p.z_bias=m.scalar(a+44);p.min_scale=m.scalar(a+48);p.max_scale=m.scalar(a+52);p.texture_id=m.word(a+56);
  if(p.texture_id>=3)hp2::not_implemented("0x00116510","Original flare texture ID outside recovered texture table");
  for(unsigned c=0;c<2;++c){const auto ptr=m.word(0x002cd998+i*8+c*4);int index=-1;
   if(ptr){if(ptr<0x002cd740||ptr>=0x002cd998||(ptr-0x002cd740)%60)hp2::not_implemented("0x001168E8","Original flare parameter pointer outside recovered table");index=int((ptr-0x002cd740)/60);}
   out.parameter_indices[i][c]=index;
  }
 }
 for(unsigned i=0;i<3;++i)out.texture_hashes[i]=m.word(0x002cd730+i*4);
 for(unsigned i=0;i<6;++i)for(unsigned c=0;c<3;++c)out.normals[i][c]=m.scalar(0x002d7af8+i*16+c*4);
 out.minimum_angle=m.scalar(0x00324a4c);return out;
}
// CreateCarLightFlares Debug00120558 slice. Allocation/list ownership is port-side.
std::optional<VehicleFlareMarker> create_vehicle_flare_marker(const VehicleFlareResources& resources,
 std::uint32_t name,const std::array<float,3>& position,std::uint32_t param,const std::array<float,3>& pivot){
 const auto found=std::find(names.begin(),names.end(),name);if(found==names.end())return std::nullopt;
 const auto index=std::size_t(found-names.begin());VehicleFlareMarker out;out.name_hash=name;
 out.type=index<2?0:index<5?1:index<7?2:index==7?8:index==8?7:std::uint8_t(index-5);
 out.flags=index==9||index==10?1:0;
 const unsigned normal=(param>>8)&255;if(normal>=resources.normals.size())hp2::not_implemented("0x00120558","Original flare marker normal index outside recovered six-entry table");
 out.direction=resources.normals[normal];for(unsigned c=0;c<3;++c)out.position[c]=position[c]-pivot[c];return out;
}
// CarRenderInfo::Render Debug001217B8..00121C34 intensity slice only.
VehicleFlareState vehicle_flare_state(const Memory& m,std::uint32_t car,int time_of_day,
 std::uint32_t timebase,std::uint32_t broken,std::uint32_t force){
 VehicleFlareState out;const bool night=std::uint32_t(std::int16_t(time_of_day))-2u<2u;
 float head=night?.75f:0.f,brake=night?m.scalar(0x00324b84):0.f,reverse=0.f;
 for(unsigned i=0;i<4;++i)if(m.word(0x00326c80+i*4))force|=1u<<i;
 const auto phase=timebase+m.word(0x00327d04);const auto cop=car?m.word(car+0x808):0u;
 if((force&1)||(car&&m.word(car+0x830)))head+=1.f;
 if(force&2)brake+=1.f;else if(car){const auto driver=m.word(car+0x28);if(driver&&m.scalar(0x00324b88)<m.scalar(driver+0x218))brake+=m.scalar(driver+0x218)*16.f;}
 if((force&4)||(car&&m.signed_word(car+0x40+0x388)==0))reverse=1.f;
 const auto available=[&](float amount,unsigned mask){return broken&mask?0.f:amount;};
 out.intensity[0]=available(head,1);out.intensity[1]=available(head,2);
 out.intensity[2]=available(brake,4);out.intensity[3]=available(brake,16);out.intensity[4]=available(brake,8);
 out.intensity[5]=available(reverse,4);out.intensity[6]=available(reverse,8);
 if((force&8)||cop){const bool left=m.byte(0x002d7ae8+(phase&15))!=0;
  out.intensity[7]=left&&!(broken&1)?1.f:0.f;out.intensity[8]=!left&&!(broken&2)?1.f:0.f;
  const auto offset=cop==2?(phase&7):((phase>>1)&15);
  for(unsigned c=0;c<3;++c)out.intensity[9+c]=m.byte((cop==2?0x00326c98+c*8:0x002d7ab8+c*16)+offset)?1.f:0.f;
 }
 for(auto& value:out.intensity)value=std::clamp(value,0.f,1.f);
 return out;
}
float vehicle_flare_intensity(const VehicleFlareState& state,std::uint32_t name){
 const auto i=std::find(names.begin(),names.end(),name);return i==names.end()?0.f:state.intensity[std::size_t(i-names.begin())];
}
float vehicle_flare_angle_power(float angle,float power,float minimum){
 const float clamped=std::max(angle,minimum);
 std::int32_t bits;std::memcpy(&bits,&clamped,4);
 const float integer=static_cast<float>(bits);
 const float correction=1065353216.f-power*1065353216.f;
 const auto result_bits=cvt_w_s(integer*power+correction);float result;
 std::memcpy(&result,&result_bits,4);return result;
}
// eRenderLightFlare001168E8 / RenderSingleFlare00116510 projections.
std::vector<VehicleFlareBillboard> vehicle_flare_billboards(const VehicleFlareResources& resources,
 const std::vector<VehicleFlareMarker>& markers,const VehicleFlareState& state,
 const std::array<float,16>& local_world,const VehicleFlareFrame& frame){
 if(!(frame.h>0)||!std::isfinite(frame.h))throw std::invalid_argument("Original flare eView::H must be positive and finite");
 std::vector<VehicleFlareBillboard> out;
 for(const auto& marker:markers){const float intensity=vehicle_flare_intensity(state,marker.name_hash);if(!marker.enabled||!(intensity>0))continue;
  if(marker.type>=resources.parameter_indices.size())hp2::not_implemented("0x001168E8","Original flare type outside recovered parameter table");
  const auto position=xyz(transform(local_world,marker.position,1)),normal=xyz(transform(local_world,marker.direction,0));
  const auto clip=transform(frame.world_clip,position,1);if(clip[0]>clip[3]||clip[0]<-clip[3]||clip[1]>clip[3]||clip[1]<-clip[3]||clip[2]>clip[3]||clip[2]<-clip[3])continue;
  std::array<float,3> toward{};float squared=0;for(unsigned c=0;c<3;++c){toward[c]=frame.camera_position[c]-position[c];squared+=toward[c]*toward[c];}
  if(!(squared>0))hp2::not_implemented("0x00116A1C","Original flare camera-position reciprocal square root exceptional operand");
  const float inverse=1.f/std::sqrt(squared);float angle=0;for(unsigned c=0;c<3;++c){toward[c]*=inverse;angle+=toward[c]*normal[c];}if(marker.flags&1)angle=std::abs(angle);
  const float scale=transform(frame.camera_matrix,position,1)[2]/frame.h;
  for(const int index:resources.parameter_indices[marker.type]){if(index<0)continue;if(unsigned(index)>=resources.parameters.size())throw std::out_of_range("Flare parameter index");const auto& p=resources.parameters[index];
   const float amount=vehicle_flare_angle_power(angle,p.power,resources.minimum_angle)*intensity;
   float size=p.min_size+(p.max_size-p.min_size)*amount;if(!(size>0))continue;size*=std::clamp(scale,p.min_scale,p.max_scale);
   VehicleFlareBillboard b;b.name_hash=marker.name_hash;b.parameter_index=unsigned(index);b.texture_hash=resources.texture_hashes.at(p.texture_id);b.intensity=amount;b.half_size=size;
   for(unsigned c=0;c<3;++c)b.centre[c]=position[c]+toward[c]*p.z_bias;
   for(unsigned c=0;c<4;++c)b.colour[c]=std::uint8_t(cvt_w_s(p.min_colour[c]+(p.max_colour[c]-p.min_colour[c])*amount));
   constexpr float sx[]{-1,1,1,-1},sy[]{1,1,-1,-1};
   for(unsigned v=0;v<4;++v)for(unsigned c=0;c<3;++c)b.corners[v][c]=(b.centre[c]+sx[v]*frame.world_view[c*4]*size)+sy[v]*frame.world_view[c*4+1]*size;
   b.depth=transform(frame.world_view,b.corners[0],1)[2];out.push_back(b);
  }
 }
 return out;
}
}
