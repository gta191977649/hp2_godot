#include "environment.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/src/skye.hpp"
#include "game_core/src/sune.hpp"
#include "game_core/indep/src/weatherman.hpp"
#include "game_core/src/ecstasy/eenvmape.hpp"
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <algorithm>
namespace port::track {
namespace {
struct Bytes {const std::vector<std::uint8_t>& b;
void check(std::size_t p,std::size_t n)const{if(p>b.size()||n>b.size()-p)throw std::runtime_error("Truncated environment resource");}
std::uint32_t u32(std::size_t p)const{check(p,4);return b[p]|std::uint32_t(b[p+1])<<8|std::uint32_t(b[p+2])<<16|std::uint32_t(b[p+3])<<24;}
std::int16_t i16(std::size_t p)const{check(p,2);return static_cast<std::int16_t>(b[p]|std::uint16_t(b[p+1])<<8);}
float f32(std::size_t p)const{auto bits=u32(p);float v;std::memcpy(&v,&bits,4);if(!std::isfinite(v))throw std::runtime_error("Nonfinite environment resource");return v;}
std::string text(std::size_t p)const{check(p,32);auto e=std::find(b.begin()+p,b.begin()+p+32,0);return {b.begin()+p,e};}
};
}
// Retail CreateViewMatricies0010E638 / Debug0010EA18: bounded depth adapter.
ViewDepth original_view_depth(port::driving::Memory& math,unsigned id,int mover,std::uint16_t fov,float near_z,float far_z,float force_near,float force_far){
 if((id==1||id==2)&&mover!=1&&mover!=5){const auto angle=std::uint16_t(fov>>1),baseline=std::uint16_t(0x238e>>1);
 const float tan_base=hp2::recovered::bSin(math,baseline)/hp2::recovered::bSin(math,std::uint16_t(baseline+0x4000));
 const float tan_current=hp2::recovered::bSin(math,angle)/hp2::recovered::bSin(math,std::uint16_t(angle+0x4000));
 const float span=far_z-near_z;near_z=near_z*(tan_base/tan_current);far_z=span+near_z;}
 if(force_near!=0)near_z=force_near;if(force_far!=0)far_z=force_far;
 if(!std::isfinite(near_z)||!std::isfinite(far_z)||near_z<=0||far_z<=near_z)throw std::runtime_error("Invalid original view depth range");return {near_z,far_z};
}
EnvironmentResources decode_environment(const std::vector<std::uint8_t>& bundle){
 EnvironmentResources out;Bytes bytes{bundle};
 using Candidate=hp2::recovered::SkySceneryCandidate;using Candidates=hp2::recovered::SkyScenerySection;std::vector<Candidates> candidates;std::size_t instance_index=0;
 for(std::size_t p=0;p<bundle.size();){const auto id=bytes.u32(p),length=bytes.u32(p+4);bytes.check(p+8,length);const auto end=p+8+length;
 if(id==0x80034100){std::size_t info=0,info_count=0,instances=0,instance_count=0;int number=0;
 for(auto c=p+8;c<end;){const auto child=bytes.u32(c),size=bytes.u32(c+4);if(size>end-c-8)throw std::runtime_error("Environment scenery child exceeds section");
 if(child==0x34101){auto a=(c+23)&~std::size_t(15);number=int(bytes.u32(a+8));}
 if(child==0x34102){info=c+8;info_count=size/40;}
 if(child==0x34103){instances=(c+23)&~std::size_t(15);if(instances>c+8+size||(c+8+size-instances)%48)throw std::runtime_error("Environment scenery stride mismatch");instance_count=(c+8+size-instances)/48;}c+=8+size;}
 Candidates section;section.section=number;
 for(std::size_t i=0;i<instance_count;++i){const auto a=instances+i*48;const auto index=static_cast<unsigned>(static_cast<std::uint16_t>(bytes.i16(a+12)));if(index>=info_count)throw std::runtime_error("Sky scenery index exceeds info");Candidate candidate;candidate.instance=instance_index++;candidate.hash=bytes.u32(info+index*40);candidate.matrix[15]=1;
 for(unsigned col=0;col<3;++col)for(unsigned row=0;row<3;++row)candidate.matrix[col*4+row]=float(bytes.i16(a+28+(col*3+row)*2))/16384.f;
 for(unsigned row=0;row<3;++row)candidate.matrix[12+row]=bytes.f32(a+16+row*4);section.values.push_back(candidate);}
 if(number>=90)candidates.insert(candidates.begin(),std::move(section));else candidates.push_back(std::move(section));
 }
 if(id==0x34202){const auto a=(p+23)&~std::size_t(15);if(a>end||end-a<244)throw std::runtime_error("Truncated sun data");
 if(bytes.u32(a)==2){out.sun.available=true;for(unsigned k=0;k<3;++k){out.sun.position[k]=bytes.f32(a+4+k*4);out.sun.shadow_position[k]=bytes.f32(a+16+k*4);}
 for(unsigned k=0;k<6;++k){auto& layer=out.sun.layers[k];const auto l=a+28+k*36;layer.texture=bytes.u32(l);layer.alpha_type=bytes.u32(l+4);if(layer.texture>=5||layer.alpha_type>1)throw std::runtime_error("Invalid original sun enum");layer.intensity=bytes.f32(l+8);layer.size=bytes.f32(l+12);layer.offset_x=bytes.f32(l+16);layer.offset_y=bytes.f32(l+20);for(unsigned j=0;j<4;++j)layer.colour[j]=bundle[l+24+j];layer.angle=static_cast<std::int32_t>(bytes.u32(l+28));layer.sweep=bytes.f32(l+32);}}}
 if(id==0x34250){const auto a=(p+23)&~std::size_t(15);if(a>end||end-a<16)throw std::runtime_error("Truncated weather header");
 if(bytes.u32(a+8)!=2)throw std::runtime_error("Unsupported weather version");const auto count=bytes.u32(a+12);
 if(count>(end-a-16)/128)throw std::runtime_error("Weather region count exceeds chunk");
 for(unsigned n=0;n<count;++n){const auto r=a+16+n*128;WeatherRegion w;w.name_hash=bytes.u32(r+8);w.group_hash=bytes.u32(r+12);w.name=bytes.text(r+16);w.group=bytes.text(r+48);w.type=static_cast<int>(bytes.u32(r+80));w.colour=bytes.u32(r+84);w.fog_start=bytes.f32(r+88);w.parameter1=bytes.f32(r+92);for(unsigned k=0;k<3;++k)w.position[k]=bytes.f32(r+96+k*4);w.radius=bytes.f32(r+108);w.intensity=bytes.f32(r+112);w.falloff_start=bytes.f32(r+116);if(unsigned(w.type)<4)out.weather.push_back(std::move(w));}
 }
 p=end;}
 hp2::recovered::SkyLoader(out,candidates);
 return out;
}
unsigned SunOcclusion::uncovered()const {unsigned count=0;for(int y=rectangle.start_y;y<rectangle.end_y;++y)for(int x=rectangle.start_x;x<rectangle.end_x;++x){if(x<0||x>=32||y<0||y>=32)throw std::runtime_error("Sun readback clipped rectangle exceeds32x32");if((pixels[std::size_t(y)*32+x]&0xffffffu)<2)++count;}return count;}
SunReadbackRect sun_readback_rect(const SunResources& sun,unsigned id,float x,float y,float z,unsigned width,unsigned height){SunReadbackRect r;if(!sun.available||id!=1||z<0)return r;float largest=0;for(const auto& layer:sun.layers)if(layer.texture==0&&largest<layer.size)largest=layer.size;if(x < -largest||x>float(width)+largest||y < -largest||y>float(height)+largest)return r;r.requested=true;r.x=int(x-16.f);r.y=int(y-16.f);r.end_x=r.end_y=32;
 if(r.x<0){r.end_x=r.x+32;r.x=0;}if(int(width)-33<r.x){r.start_x=(r.x+1)-(int(width)-32);r.x=int(width)-33;}
 if(r.y<0){r.end_y=r.y+32;r.y=0;}if(int(height)-33<r.y){r.start_y=(r.y+1)-(int(height)-32);r.y=int(height)-33;}
 return r;}
std::vector<SunQuad> sun_quads(port::driving::Memory& m,const EnvironmentResources& r,unsigned id,float x,float y,float z,unsigned w,unsigned h,float zmax,unsigned samples){return hp2::recovered::StuffSun(m,r,id,x,y,z,w,h,zmax,samples);}
WeatherResult query_weather(const EnvironmentResources& r,const EnvironmentVector& p){return hp2::recovered::WeatherManQuery(r.weather,p);}
EnvironmentVector environment_eye_offset(){return {{0,0,0.949999988079071044921875f}};}
ReflectionTables reflection_tables(){return hp2::recovered::CreateRefSampleTable();}
std::vector<EnvironmentWarp> environment_warps(const ReflectionTables& t,const std::array<bool,6>& a,unsigned w,unsigned h){return hp2::recovered::eCreateEnvMapPrims(t,a,w,h);}
EnvironmentCameras environment_cameras(const EnvironmentVector& v,const EnvironmentVector& p,std::uint32_t frame,int mode,bool game){return hp2::recovered::eEnvMap::UpdateCameras(v,p,frame,mode,game);}
}
