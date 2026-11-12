// TASK-0042 E-0042-S001: exact Debug source marker(s): src\acidfxe.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): AcidEffectSystem::DoTimeStep() [0x002AD370]
// Diagnostic text (not a callable signature): AcidEffectSystem::RenderEffects [0x002AD390]
#include "game_core/indep/inc/AcidFX.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/track/emitter_bindings.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <cstring>
#include <stdexcept>

namespace hp2::recovered {
// ELF 0x00127418; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00127418.md.
void AcidEffectSystem::Sub_00127418() {
    hp2::not_implemented("0x00127418", "AcidEffectSystem::Sub_00127418 (signature and full body unresolved)");
}

// ELF 0x00127688; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00127688.md.
void AcidEffectSystem::RemoveAutoEmitter() {
    hp2::not_implemented("0x00127688", "AcidEffectSystem::RemoveAutoEmitter (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/src/acidfxe.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/src/ecstasy/emathe.hpp"
#include <cmath>

namespace hp2::debug {
// Debug001261E0, E-0098-P01. Keep the original24-term accumulation order.
float fDeterminant(const port::driving::SolverMatrix& m){
    for(const float lane:m)if(!std::isfinite(lane))hp2::not_implemented("0x001261E0","nonfinite determinant input outside frontend projection");
    float result=m[3]*m[12]*m[9]*m[6]-m[2]*m[12]*m[9]*m[7];
    result=result-m[3]*m[12]*m[10]*m[5];
    result=result+m[1]*m[12]*m[10]*m[7];
    result=result+m[2]*m[12]*m[11]*m[5];
    result=result-m[1]*m[12]*m[11]*m[6];
    result=result-m[3]*m[13]*m[8]*m[6];
    result=result+m[2]*m[13]*m[8]*m[7];
    result=result+m[3]*m[13]*m[10]*m[4];
    result=result-m[0]*m[13]*m[10]*m[7];
    result=result-m[2]*m[13]*m[11]*m[4];
    result=result+m[0]*m[13]*m[11]*m[6];
    result=result+m[3]*m[14]*m[8]*m[5];
    result=result-m[1]*m[14]*m[8]*m[7];
    result=result-m[3]*m[14]*m[9]*m[4];
    result=result+m[0]*m[14]*m[9]*m[7];
    result=result+m[1]*m[14]*m[11]*m[4];
    result=result-m[0]*m[14]*m[11]*m[5];
    result=result-m[2]*m[15]*m[8]*m[5];
    result=result+m[1]*m[15]*m[8]*m[6];
    result=result+m[2]*m[15]*m[9]*m[4];
    result=result-m[0]*m[15]*m[9]*m[6];
    result=result-m[1]*m[15]*m[10]*m[4];
    result=result+m[0]*m[15]*m[10]*m[5];
    if(!std::isfinite(result))hp2::not_implemented("0x001261E0","exceptional EE determinant result");
    return result;
}
// Debug00126378, E-0098-P01. The sixteen original cofactor expressions have
// the same two instruction-order forms; index projection avoids reordering sums.
void fInvertMatrix(port::driving::SolverMatrix& out,const port::driving::SolverMatrix& in){
    if(&out==&in)hp2::not_implemented("0x00126378","inverse source/destination alias is outside frontend projection");
    for(const float lane:in)if(!std::isfinite(lane))hp2::not_implemented("0x00126378","nonfinite general matrix inverse input");
    const float determinant=fDeterminant(in);
    if(determinant==0 || !std::isfinite(determinant))
        hp2::not_implemented("0x00126378","singular or exceptional EE general matrix inverse");
    const float reciprocal=1.f/determinant;
    port::driving::SolverMatrix candidate{};
    for(int row=0;row<4;++row)for(int column=0;column<4;++column){
        std::array<int,3> rows{},columns{};int ri=0,ci=0;
        for(int i=0;i<4;++i){if(i!=column)rows[ri++]=i;if(i!=row)columns[ci++]=i;}
        const float a=in[rows[0]*4+columns[0]],b=in[rows[0]*4+columns[1]],c=in[rows[0]*4+columns[2]];
        const float d=in[rows[1]*4+columns[0]],e=in[rows[1]*4+columns[1]],f=in[rows[1]*4+columns[2]];
        const float g=in[rows[2]*4+columns[0]],h=in[rows[2]*4+columns[1]],i=in[rows[2]*4+columns[2]];
        const float cofactor=(row+column)%2
            ? ((((c*e*g-b*f*g)-c*d*h)+a*f*h+b*d*i)-a*e*i)
            : (((((b*f*g-c*e*g)+c*d*h)-a*f*h)-b*d*i)+a*e*i);
        candidate[row*4+column]=reciprocal*cofactor;
        if(!std::isfinite(candidate[row*4+column]))hp2::not_implemented("0x00126378","exceptional EE inverse result");
    }
    out=candidate;
}
// Debug00126AB8, E-0098-P01. Keep both original basis products, despite their
// algebraic cancellation. eMulMatrix is the recovered VU operation, not a library.
void hermite_basis(port::driving::SolverMatrix& out,const port::driving::SolverMatrix& p,
    float u1,float u2,float u3,float u4){
    for(const float lane:p)if(!std::isfinite(lane))hp2::not_implemented("0x00126AB8","nonfinite Hermite controls outside frontend projection");
    port::driving::SolverMatrix u{},inverse{},k{},nf{};
    const std::array<float,4> parameters{{u1,u2,u3,u4}};
    for(int i=0;i<4;++i){const float square=parameters[i]*parameters[i];u[i*4]=square*parameters[i];
        u[i*4+1]=square;u[i*4+2]=parameters[i];u[i*4+3]=1;}
    const port::driving::SolverMatrix mf{{2,-2,1,1,-3,3,-2,-1,0,0,1,0,1,0,0,0}};
    const port::driving::SolverMatrix imf{{0,0,0,1,1,1,1,1,0,0,1,0,3,2,1,0}};
    fInvertMatrix(inverse,u);
    hp2::recovered::eMulMatrix(k,imf,inverse);
    hp2::recovered::eMulMatrix(nf,mf,k);
    hp2::recovered::eMulMatrix(out,nf,p);
}
}

namespace hp2::recovered {
// ELF 0x00129880; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00129880.md.
void RainSplash::Recalc() {
    hp2::not_implemented("0x00129880", "RainSplash::Recalc (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK0052 E-0052-E07: Retail00128718 / Debug00128870 active particle loop.
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include <cmath>
#include <algorithm>
namespace hp2::recovered {
void AcidEffectSystem::DoTimeStep(std::vector<port::track::Particle>& particles,const port::track::ParticleDefinition& data,float dt,const std::array<float,3>& inherited){
 if(!std::isfinite(dt)||dt<0)throw std::invalid_argument("Invalid original particle timestep");
 constexpr float inverse127=0.0078740157186985016f,inverse255=0.0039215688593685627f;
 const auto ticks=port::driving::cvt_w_s(dt*1024.f);const auto fps=static_cast<std::int8_t>(data.flags[3]);const int interval=fps?port::driving::cvt_w_s((1.f/float(fps))*1024.f):0;
 auto next=particles;next.erase(std::remove_if(next.begin(),next.end(),[&](auto& p){const int life=int(p.life)-ticks;if(life<1)return true;
 if(interval){const auto mask=((1u<<(data.flags[1]&31))<<(data.flags[2]&31))-1;p.frame=std::uint8_t((p.frame+int(p.life)/interval-life/interval)&mask);}
 const float scale=float(p.speed)*.0078125f*inverse127;std::array<float,3> velocity{};
 for(unsigned k=0;k<3;++k){const float v=float(p.direction[k])*scale;const float sign=v<0?1.f:-1.f;const float gravity=k==2?-data.gravity:0.f;velocity[k]=k==2?v+(gravity+sign*v*v*data.drag)*dt:v+sign*data.drag*v*v*dt;p.position[k]+=velocity[k]*dt;if(data.flags[6])p.position[k]+=(dt*inherited[k])*(float(p.inherit)*inverse255);}
 const float magnitude=std::sqrt(velocity[0]*velocity[0]+velocity[1]*velocity[1]+velocity[2]*velocity[2]);const float inverse=magnitude==0.f?port::driving::ee_div_s_zero(1.f,magnitude,"0x00128C64"):1.f/magnitude;p.life=std::int16_t(life);p.speed=std::uint16_t(port::driving::cvt_w_s(magnitude*128.f));for(unsigned k=0;k<3;++k)p.direction[k]=std::int8_t(port::driving::cvt_w_s(velocity[k]*inverse*127.f));return false;
 }),next.end());particles=std::move(next);
}
}

// TASK0052 E-0052-E07: original auto-emitter loop projection. Host owns list nodes.
namespace hp2::recovered {
void AcidEffectSystem::DoTimeStep(port::track::ParticleEmitter& emitter,const port::track::ParticleDefinition& data,float dt,const std::function<void(float)>& trigger){
 if(!std::isfinite(dt)||dt<0)throw std::invalid_argument("Invalid original emitter timestep");
 if(!trigger)throw std::invalid_argument("Original TriggerEmitter callback missing");
 const auto state=emitter.state;
 if(state==1)trigger(dt);
 if(emitter.cycle_time!=0.f&&state<2){
  float remaining=emitter.cycle_time-dt;
  if(remaining<=0.f){
   const auto index=state?2u:0u;
   const float duration=data.controls[index],variance=data.controls[index+1];
   float next=duration;
   if(duration!=0.f){
    auto seed=emitter.random_seed;seed^=seed<<17;seed^=seed>>13;seed^=seed<<5;emitter.random_seed=seed;
    next=(duration-variance*duration)+float(seed&0x7fffffffu)*4.656612873077392578125e-10f*variance*duration+remaining;
   }
   emitter.state=state==0;emitter.cycle_time=next;emitter.countdown=0;
  }else emitter.cycle_time=remaining;
 }
}
}

#include "game_core/src/bMath.hpp"
namespace hp2::recovered {
void AcidEffectSystem::TriggerEmitter(port::driving::Memory& math,port::track::ParticleEmitter& e,const port::track::ParticleDefinition& d,float dt,const std::array<std::array<float,3>,3>& interests,std::uint32_t& global_seed,std::vector<port::track::Particle>& particles,std::size_t available,bool group_available){
 if(!std::isfinite(dt))throw std::invalid_argument("Invalid original TriggerEmitter timestep");
 auto random=[](std::uint32_t& seed){seed^=seed<<17;seed^=seed>>13;seed^=seed<<5;return float(seed&0x7fffffffu)*4.656612873077392578125e-10f;};
 bool nearby=false;for(const auto& p:interests){float length=0;for(unsigned k=0;k<3;++k){const float v=e.local_world[12+k]-p[k];length+=v*v;}if(std::sqrt(length)<d.far_clip){nearby=true;break;}}
 if(!e.random_seed)e.random_seed=std::uint32_t(port::driving::cvt_w_s(random(global_seed)*2147483648.f));
 if(!nearby||(e.state!=1&&dt>=0))return;
 float remaining=dt<0?-dt:((e.cycle_time!=0&&e.cycle_time<dt)?e.cycle_time:dt),countdown=e.countdown;
 if(countdown<remaining&&group_available){
  auto seed=e.random_seed;
  auto transform=[](const std::array<float,16>& m,const std::array<float,3>& v,bool point){std::array<float,3> out{};for(unsigned row=0;row<3;++row){out[row]=m[row]*v[0]+m[4+row]*v[1]+m[8+row]*v[2];if(point)out[row]+=m[12+row];}return out;};
  auto angle=[](float degrees){return std::uint16_t(port::driving::cvt_w_s(degrees*65536.f)/360);};
  const auto& c=d.controls;const float number_variance=c[5]*c[4],life_variance=c[7]*c[6],speed_variance=c[9]*c[8],motion_variance=c[13]*c[12],motion_base=c[12]-motion_variance;
  const float angle_range=c[10]*182.0444488525390625f;
  const float frames=d.flags[4]?float((1u<<(d.flags[1]&31))<<(d.flags[2]&31)):1.f;
  std::size_t port_iterations=0;
  do {
   if(++port_iterations>port::track::emission_iteration_budget)hp2::not_implemented("0x00127700","Port execution budget exhausted while preserving original emitter RNG and zero-interval retries");
   remaining-=countdown;
   std::array<float,3> direction{};auto rotation=bIdentity();
   if(!d.flags[0]){
    const float rx=random(seed)*c[11]-c[11]*.5f,ry=random(seed)*c[11]-c[11]*.5f,rz=random(seed)*c[11]-c[11]*.5f;
    eRotateX(math,rotation,rotation,angle(rx));eRotateY(math,rotation,rotation,angle(ry));eRotateZ(math,rotation,rotation,angle(rz));direction=transform(rotation,{{e.local_world[8],e.local_world[9],e.local_world[10]}},false);
   }else{
    const float tilt=random(seed)*c[11]-c[11]*.5f;const auto azimuth=angle(random(seed)*360.f-180.f);
    std::array<float,3> radial{{hp2::recovered::bSin(math,std::uint16_t(azimuth+0x4000)),hp2::recovered::bSin(math,azimuth),0}};
    eRotateY(math,rotation,rotation,angle(tilt));direction=transform(e.local_world,transform(rotation,radial,false),false);
   }
   std::array<float,3> local{};for(unsigned k=0;k<3;++k)local[k]=d.volume_center[k]+(random(seed)*d.volume_extent[k]-d.volume_extent[k]*.5f);
   const auto position=transform(e.local_world,local,true);
   const float frame_random=random(seed),life=std::min((c[6]-life_variance)+random(seed)*life_variance,60.f),angle_random=random(seed);
   float speed=(c[8]-speed_variance)+random(seed)*speed_variance,inherit=0;
   if(motion_base!=0.f){inherit=std::clamp(motion_base+random(seed)*motion_variance,0.f,1.f);if(!d.flags[6]){
    float magnitude=0;for(unsigned k=0;k<3;++k){direction[k]=speed*direction[k]+inherit*e.inherit_velocity[k];magnitude+=direction[k]*direction[k];}
    speed=std::sqrt(magnitude);
    // Debug00128510 VRSQRT ->00128524 VMUL, E-0098-P03. The original
    // zero-speed selector nebula packs a zero direction, not host IEEE0/0 NaN.
    const float reciprocal=magnitude==0.f?port::driving::ee_vrsqrt_one_zero(magnitude,"0x00128510"):1.f/speed;
    for(auto& v:direction)v=v*reciprocal;
   }}
   auto initial_angle=std::uint16_t(port::driving::cvt_w_s(-angle_range*.5f+angle_random*angle_range)&0xfffe);if(d.flags[5])initial_angle|=seed&1;
   if(!e.dont_emit&&available){port::track::Particle p;p.position=position;p.frame=std::uint8_t(port::driving::cvt_w_s(frame_random*frames));p.life=std::int16_t(port::driving::cvt_w_s(life*1024.f));p.angle=initial_angle;p.speed=std::uint16_t(port::driving::cvt_w_s(std::min(speed*128.f,65535.f)));p.inherit=std::uint8_t(port::driving::cvt_w_s(inherit*255.f));for(unsigned k=0;k<3;++k)p.direction[k]=std::int8_t(port::driving::cvt_w_s(direction[k]*127.f));particles.push_back(p);--available;}
   countdown=1.f/((c[4]-number_variance)+random(seed)*number_variance);countdown=std::clamp(countdown,0.f,240.f);
   // E-0087-P01 Debug00127858/Retail00127700: a clamped zero interval
   // retries the original loop and consumes its RNG; it is not an immediate gap.
  }while(countdown<=remaining);
  e.random_seed=seed;
 }
 e.countdown=dt>=0?countdown-remaining:0.f;
}
}


namespace hp2::recovered {
void AcidEffectSystem::AddAutoEmitter(port::track::ParticleEmitter& e,const port::track::ParticleDefinition* d,std::uint32_t& seed,const std::function<void(bool)>& insert){
 if(!insert)throw std::invalid_argument("Original auto-emitter insertion service missing");
 auto random=[](std::uint32_t& s){s^=s<<17;s^=s>>13;s^=s<<5;return float(s&0x7fffffffu)*4.656612873077392578125e-10f;};
 if(!e.random_seed)e.random_seed=std::uint32_t(port::driving::cvt_w_s(random(seed)*2147483648.f));
 if(!d||e.state==2){e.state=2;insert(false);}
 else if(e.state<2){const auto index=e.state?0u:2u;float duration=d->controls[index];const float variance=d->controls[index+1];if(duration!=0.f)duration=(duration-variance*duration)+random(e.random_seed)*variance*duration;e.cycle_time=duration;e.countdown=0;insert(true);}
 else {if(e.state!=3)e.state=2;insert(false);}
}
}

#include <cstring>
namespace hp2::recovered {
port::driving::SolverVector hermite_parameter(const port::driving::SolverMatrix& basis,float t){
 const float square=t*t;const std::array<float,4> u{{square*t,square,t,1}};std::array<float,4> out{};
 for(unsigned r=0;r<4;++r)out[r]=((basis[r]*u[0]+basis[4+r]*u[1])+basis[8+r]*u[2])+basis[12+r];return out;
}
std::vector<port::track::ParticleDraw> AcidEffectSystem::RenderEffects(port::driving::Memory& math,const port::track::ParticleSimulation& simulation,const port::track::ParticleView& view){
 if(!view.outside_clip||!view.project||!view.texture_available||!std::isfinite(view.focal_h)||view.focal_h<=0)throw std::invalid_argument("Original particle projection/texture services missing");
 std::vector<port::track::ParticleDraw> draws;std::size_t group_index=0;
 for(const auto& group:simulation.groups()){
  const auto& data=simulation.resources().definitions.at(group.definition);port::track::ParticleDraw draw;draw.texture=data.texture_hash;draw.group=group_index++;
  if(!view.texture_available(draw.texture))continue;
  const unsigned shift=data.flags[1]&31,mask_x=(1u<<shift)-1,mask_y=(1u<<(data.flags[2]&31))-1;const float du=1.f/float(1u<<shift),dv=1.f/float(1u<<(data.flags[2]&31));
  const float maximum=data.max_pixel_size==0?512.f:data.max_pixel_size;float sum_z=0;
  for(const auto& particle:group.particles){
   if(view.outside_clip(particle.position))continue;
   const float t=1.f-(float(particle.life)*.0009765625f)*(1.f/data.controls[6]);const auto projection=view.project(particle.position);const auto extra=hermite_parameter(data.extra_basis,t);
   float radius=extra[0]>0?(extra[0]*view.focal_h)/projection.camera_z:0.f;if(radius>maximum)radius=maximum;
   if(!(radius>1.f&&projection.camera_z<data.far_clip))continue;
   auto relative=std::uint16_t(port::driving::cvt_w_s(extra[1]));if(particle.angle&1)relative=std::uint16_t(-relative);const auto angle=std::uint16_t(particle.angle+relative);
   port::track::ParticleQuad quad;const int center_x=port::driving::cvt_w_s(projection.screen_x),center_y=port::driving::cvt_w_s(projection.screen_y);
   if(!angle){const int rx=port::driving::cvt_w_s(radius*8.f),ry=port::driving::cvt_w_s(radius*7.f);quad.xy={{{{center_x*16-rx,center_y*16-ry}},{{center_x*16+rx,center_y*16-ry}},{{center_x*16-rx,center_y*16+ry}},{{center_x*16+rx,center_y*16+ry}}}};}
   else {const std::uint32_t radians_bits=0x38c90fdbu,quarter_bits=0x3fc90fdcu;float radians_scale,quarter;std::memcpy(&radians_scale,&radians_bits,4);std::memcpy(&quarter,&quarter_bits,4);const float radians=float(angle)*radians_scale;const float cosine=eSin(math,radians+quarter),sine=eSin(math,radians);const float size=float(port::driving::cvt_w_s(radius));for(unsigned i=0;i<4;++i){const float x=(i&1)?size:-size,y=(i&2)?size:-size;quad.xy[i]={{port::driving::cvt_w_s((cosine*x-sine*y)*8.f+float(center_x*16)),port::driving::cvt_w_s((sine*x+cosine*y)*7.f+float(center_y*16))}};}}
   quad.native_depth=(projection.screen_depth_bits&0x0fffffffu)>>(view.z_shift&31);const auto colour=hermite_parameter(data.colour_basis,t);for(unsigned k=0;k<4;++k)quad.colour[k]=std::uint8_t(port::driving::cvt_w_s(std::clamp(colour[k],0.f,255.f)));
   const float u=float(particle.frame&mask_x)*du,v=float((unsigned(particle.frame)>>shift)&mask_y)*dv;quad.uv={{{{u,v}},{{u+du,v}},{{u,v+dv}},{{u+du,v+dv}}}};
   draw.quads.push_back(quad);sum_z+=float(port::driving::cvt_w_s(projection.camera_z));
  }
  if(!draw.quads.empty()){draw.depth=sum_z/float(draw.quads.size());draws.push_back(std::move(draw));}
 }
 return draws;
}
}

namespace port::track {
namespace {
void emitter_detach(port::driving::Memory& m,std::uint32_t node){
 const auto next=m.word(node),previous=m.word(node+4);m.word(previous,next);m.word(next+4,previous);
}
void emitter_append(port::driving::Memory& m,std::uint32_t head,std::uint32_t node){
 const auto previous=m.word(head+4);m.word(previous,node);m.word(head+4,node);m.word(node,head);m.word(node+4,previous);
}
std::uint32_t emitter_word(const std::vector<std::uint8_t>& b,std::size_t at){
 if(at>b.size()||b.size()-at<4)throw std::runtime_error("Truncated original AcidFX resource");
 return b[at]|std::uint32_t(b[at+1])<<8|std::uint32_t(b[at+2])<<16|std::uint32_t(b[at+3])<<24;
}
std::string emitter_name(const port::driving::Memory& m,std::uint32_t at){
 std::string name;for(unsigned i=0;i<4096;++i){const auto c=m.byte(at+i);if(!c)return name;name.push_back(char(c));}
 throw std::runtime_error("Unterminated original AcidFX emitter name");
}
// Debug00127570; E-0099-W10. Interest points and padding are not initialized
// by this original constructor and are deliberately not assigned here.
void emitter_system_construct(port::driving::Memory& m,std::uint32_t at){
 m.word(at,at);m.word(at+4,at);m.word(at+8,at+8);m.word(at+12,at+8);
 m.word(at+0x40,0);m.word(at+0x44,0);for(int i=0;i<6;++i)m.byte(at+0x48+i,0);
 m.word(at+0x50,0);m.word(at+0x54,0);m.scalar(at+0x58,0);m.scalar(at+0x5c,0);
}
// Memory projection of Debug001275F8 using its existing recovered arithmetic.
void emitter_register(port::driving::Memory& m,std::uint32_t at){
 ParticleEmitter emitter;emitter.state=m.word(at+0x10);emitter.random_seed=m.word(at+0x24);
 emitter.cycle_time=m.scalar(at+0x14);emitter.countdown=m.scalar(at+0x18);
 ParticleDefinition definition;const auto data=m.word(at+0x1c);
 if(data)for(int i=0;i<4;++i)definition.controls[i]=m.scalar(data+0x20+4*i);
 auto seed=m.word(0x326d78);
 hp2::recovered::AcidEffectSystem::AddAutoEmitter(emitter,data?&definition:nullptr,seed,[&](bool first){
  constexpr std::uint32_t head=0x2e6f48;
  if(first){const auto next=m.word(head);m.word(head,at);m.word(next+4,at);m.word(at,next);m.word(at+4,head);}
  else emitter_append(m,head,at);
 });
 m.word(0x326d78,seed);m.word(at+0x10,emitter.state);m.word(at+0x24,emitter.random_seed);
 m.scalar(at+0x14,emitter.cycle_time);m.scalar(at+0x18,emitter.countdown);
 m.word(0x2e6f90,m.word(0x2e6f90)+1);
}
}
}
namespace hp2::debug {
using port::driving::Memory;
// Debug00126EC8; E-0099-W10.
void afxInit(Memory& m){
 m.word(0x326d60,bNewSlotPool(m,0x20,1000,0x2a5358,0x10));
 m.word(0x326d64,bNewSlotPool(m,0x1c,400,0x2a5370,0x10));
 m.word(0x326d68,bNewSlotPool(m,0xc0,600,0x2a5388,0x10));
 for(auto global:{0x326d60u,0x326d64u}){const auto pool=m.word(global);
  m.word(pool+0x14,m.word(pool+0x14)&~std::uint32_t(SLOTPOOL_FLAG_ZERO_ALLOCATED_MEMORY|SLOTPOOL_FLAG_OVERFLOW_IF_FULL));}
}
// Debug00127278; E-0099-W10. First-match lookup, including duplicate hashes.
std::uint32_t afxGetEmitterData(Memory& m,std::uint32_t name,std::uint32_t group,std::uint32_t previous){
 if(!name&&!group)return 0;
 constexpr std::uint32_t head=0x326d70;
 for(auto p=m.word(previous?previous:head);p!=head;p=m.word(p))
  if((!name||m.word(p+8)==name)&&(!group||m.word(p+0x10)==group))return p;
 return 0;
}
// Debug001272F0; E-0099-W10. Name-only compares the DEFINITION name;
// name+group compares the EMITTER name before its definition group.
std::uint32_t afxGetEmitter(Memory& m,std::uint32_t name,std::uint32_t group,std::uint32_t previous){
 if(!name&&!group)return 0;
 constexpr std::uint32_t head=0x2e6f48;
 for(auto p=m.word(previous?previous:head);p!=head;p=m.word(p)){
  const auto data=m.word(p+0x1c);
  if(!name){if(data&&m.word(data+0x10)==group)return p;}
  else if(!group){if(!data)hp2::not_implemented("0x001272F0","name-only lookup encountered null original emitter data");if(m.word(data+8)==name)return p;}
  else if(m.word(p+8)==name&&data&&m.word(data+0x10)==group)return p;
 }
 return 0;
}
// Debug00127380/00127518; E-0099-W10. Pool allocation and zeroed copy
// retain the original Name/definition/matrix-only initialization.
std::uint32_t afxCopyEmitter(Memory& m,std::uint32_t source){
 const auto pool=m.word(0x326d68);if(!pool)hp2::not_implemented("0x00127380","original AcidEmitterSlotPool has not been initialized");
 const auto at=bOMalloc(m,pool);if(!at)hp2::not_implemented("0x00127380","original emitter pool allocation returned null before its memset");
 std::memset(m.view(at+8,0xb8).data,0,0xb8);m.word(0x326d6c,m.word(0x326d6c)+1);
 const auto name=port::track::emitter_name(m,source+0x80);m.word(at+8,hp2::frontend::bStringHash(name.c_str()));
 for(std::size_t i=0;i<std::min<std::size_t>(63,name.size());++i)m.byte(at+0x80+std::uint32_t(i),std::uint8_t(name[i]));
 m.byte(at+0xbf,0);if(const auto data=m.word(source+0x1c))m.word(at+0x1c,data);
 m.copy(at+0x30,source+0x30,0x40);return at;
}
// Debug00127438; E-0099-W10. Lookup order and tail insertion are original.
void afxGetEmitters(Memory& m,std::uint32_t head,std::uint32_t group){
 std::uint32_t previous=0;
 while((previous=afxGetEmitter(m,0,group,previous))!=0)
  if(const auto copy=afxCopyEmitter(m,previous))port::track::emitter_append(m,head,copy);
}
// Debug001274C0/00124FA0/001250A0; E-0099-W10.
void afxDeleteEmitters(Memory& m,std::uint32_t head){
 while(m.word(head)!=head){const auto node=m.word(head);port::track::emitter_detach(m,node);
  m.word(0x326d6c,m.word(0x326d6c)-1);SlotPool(m,m.word(0x326d68)).Free(node);}
}
// Debug00127800/0012A3B0/0012A418/0012A728/0012A490; E-0099-W10.
void KillAllActiveEffects(Memory& m,std::uint32_t head){
 while(m.word(head)!=head){const auto group=m.word(head);port::track::emitter_detach(m,group);
  m.word(group+0xc,0);m.word(group+8,0);const auto particles=group+0x14;
  while(m.word(particles)!=particles){const auto p=m.word(particles);port::track::emitter_detach(m,p);SlotPool(m,m.word(0x326d60)).Free(p);}
  SlotPool(m,m.word(0x326d64)).Free(group);
 }
}
// Debug00126F60; E-0099-W10. Emitter pool is intentionally retained.
void afxClose(Memory& m){
 KillAllActiveEffects(m,0x2e6ee0);KillAllActiveEffects(m,0x2e6f40);
 bDeleteSlotPool(m,m.word(0x326d60));bDeleteSlotPool(m,m.word(0x326d64));
 m.word(0x326d60,0);m.word(0x326d64,0);
}
}
namespace port::track {
// Port owner: explicit original static construction precedes afxInit00126EC8.
void EmitterBindings::init(){
 if(initialized_)throw std::logic_error("AcidFX binding already initialized");
 if(memory_.word(0x326d60)||memory_.word(0x326d64)||memory_.word(0x326d68))throw std::logic_error("AcidFX pools already owned");
 if(memory_.word(0x326d6c))throw std::logic_error("AcidFX copied emitter count is not empty");
 emitter_system_construct(memory_,0x2e6ee0);emitter_system_construct(memory_,0x2e6f40);
 memory_.word(0x326d70,0x326d70);memory_.word(0x326d74,0x326d70);
 hp2::debug::afxInit(memory_);initialized_=true;
}
// Debug00126FA8/00127240/00127530/001275F8; E-0099-W10.
void EmitterBindings::load_bundle(const std::vector<std::uint8_t>& bytes){
 if(!initialized_)throw std::logic_error("AcidFX binding is not initialized");
 const auto chunks=hp2::assets::parse_chunks_top(bytes);
 const auto walk=[&](const auto& self,const std::vector<hp2::assets::Chunk>& list)->void{
  for(const auto& chunk:list){if(chunk.tag==0x35021||chunk.tag==0x35020){
    const auto start=(chunk.header_offset+23)&~std::size_t(15),end=chunk.payload_offset()+chunk.size;
    if(start>end||end-start<16)throw std::runtime_error("Truncated original AcidFX resource header");
    const auto stride=chunk.tag==0x35021?544u:192u,version=chunk.tag==0x35021?4u:5u;
    // The original loader recognizes but ignores unsupported record versions.
    if(emitter_word(bytes,start+8)!=version)continue;
    const auto count=emitter_word(bytes,start+12);
    if(count>(end-start-16)/stride)throw std::runtime_error("Original AcidFX record count exceeds chunk");
    if(!count)continue;
    const auto size=count*stride,address=memory_.allocate(size);
    std::memcpy(memory_.view(address,size).data,bytes.data()+start+16,size);
    records_.push_back({address,count,stride});
    for(std::uint32_t i=0;i<count;++i){const auto at=address+i*stride;
      if(stride==544)emitter_append(memory_,0x326d70,at);
      else{if(const auto data=hp2::debug::afxGetEmitterData(memory_,memory_.word(at+0xc),0))memory_.word(at+0x1c,data);
        emitter_register(memory_,at);}
    }
   }else self(self,chunk.children);
  }
 };walk(walk,chunks);
}
void EmitterBindings::populate(std::uint32_t list,std::uint32_t group){
 if(!initialized_)throw std::logic_error("AcidFX binding is not initialized");
 hp2::debug::afxGetEmitters(memory_,list,group);
}
void EmitterBindings::erase(std::uint32_t list){hp2::debug::afxDeleteEmitters(memory_,list);}
// Debug001270B8/001277E0/00127260; E-0099-W10. Port guard prevents
// references escaping their owning original resource lifetime.
void EmitterBindings::unload_all(){
 if(memory_.word(0x326d6c))throw std::logic_error("Delete copied AcidFX emitter lists before unloading resources");
 if(memory_.word(0x2e6f40)!=0x2e6f40||memory_.word(0x2e6ee0)!=0x2e6ee0)throw std::logic_error("Kill active AcidFX groups before unloading resources");
 for(const auto& record:records_){for(std::uint32_t i=0;i<record.count;++i){const auto at=record.address+i*record.stride;
   emitter_detach(memory_,at);if(record.stride==192)memory_.word(0x2e6f90,memory_.word(0x2e6f90)-1);}
  memory_.release(record.address);
 }
 records_.clear();
}
void EmitterBindings::shutdown(){
 if(!initialized_)return;
 if(memory_.word(0x326d6c))throw std::logic_error("Delete copied AcidFX emitter lists before shutdown");
 hp2::debug::afxClose(memory_);unload_all();
 // Port ownership policy: the original close retains this pool for process life.
 hp2::debug::bDeleteSlotPool(memory_,memory_.word(0x326d68));memory_.word(0x326d68,0);initialized_=false;
}
std::size_t EmitterBindings::definition_count()const{std::size_t n=0;for(const auto& r:records_)if(r.stride==544)n+=r.count;return n;}
std::size_t EmitterBindings::source_count()const{std::size_t n=0;for(const auto& r:records_)if(r.stride==192)n+=r.count;return n;}
}

namespace hp2::debug {
// Debug00127858, E-0099-W38/W53. Actual original raw gates, allocation/cache,
// RNG and packed particle writes; original-unwritten disc W requires port policy.
void AcidEffectSystem::TriggerEmitter(std::uint32_t emitter,float timestep,std::uint32_t car,std::optional<std::uint32_t> port_stack_vector_lane){
 const auto data=m_.word(emitter+0x1c);if(!data)return;
 bool near=false;for(unsigned i=0;i<3&&!near;++i){float square[3];for(unsigned axis=0;axis<3;++axis){const float delta=m_.scalar(emitter+0x60+axis*4)-m_.scalar(address_+0x10+i*16+axis*4);square[axis]=delta*delta;}near=std::sqrt((square[0]+square[1])+square[2])<m_.scalar(data+0x80);}
 // Source seeds even an emitter rejected by FarClip, before returning.
 if(!m_.word(emitter+0x24)){auto seed=m_.word(0x326d78);seed^=seed<<17;seed^=seed>>13;seed^=seed<<5;m_.word(0x326d78,seed);m_.word(emitter+0x24,static_cast<std::uint32_t>(::port::driving::cvt_w_s((static_cast<float>(seed&0x7fffffff)*4.656612873077392578125e-10f)*2147483648.f)));}
 if(!near)return;if(m_.word(emitter+0x10)!=1&&!(timestep<0))return;
 float remaining=timestep;if(timestep<0)remaining=-timestep;else{const float cycle=m_.scalar(emitter+0x14);if(cycle!=0&&cycle<timestep)remaining=cycle;}
 float countdown=m_.scalar(emitter+0x18);if(countdown<remaining){
  auto group=m_.word(emitter+0x20);const auto groups=m_.word(0x326d64),particles=m_.word(0x326d60);
  if(!groups||!particles)hp2::not_implemented("0x00127A74","Original AcidFX pools must be initialized before emission");
  const auto full=[&](std::uint32_t pool){return m_.word(pool+0x18)==m_.word(pool+0x2c);};
  if((!group||m_.word(group+12)!=emitter||m_.word(group+16)!=car)&&!full(groups)){
   group=SlotPool(m_,groups).Malloc();const auto head=group+20;m_.word(head,head);m_.word(head+4,head);const auto definition=m_.word(emitter+0x1c),tail=m_.word(address_+4);m_.word(group+16,car);m_.word(tail,group);m_.word(address_+4,group);m_.word(group+8,definition);m_.word(group+4,tail);m_.word(group,address_);m_.word(group+12,emitter);m_.word(emitter+0x20,group);
  }
  // Source retains a nonnull mismatching cached group when the group pool is
  // full. Its Emitter/Data/Car fields are NOT repaired by this branch.
  if(group){
   auto seed=m_.word(emitter+0x24);auto random=[&](){seed^=seed<<17;seed^=seed>>13;seed^=seed<<5;return static_cast<float>(seed&0x7fffffff)*4.656612873077392578125e-10f;};
   const auto angle=[](float degrees){return static_cast<std::uint16_t>(::port::driving::cvt_w_s(degrees*65536.f)/360);};
   const auto world=m_.matrix(emitter+0x30);auto orientation=world;orientation[12]=orientation[13]=orientation[14]=0;orientation[15]=1;
   const float number=m_.scalar(data+0x30),number_variance=m_.scalar(data+0x34)*number,life_base=m_.scalar(data+0x38),life_variance=m_.scalar(data+0x3c)*life_base,speed_base=m_.scalar(data+0x40),speed_variance=m_.scalar(data+0x44)*speed_base,motion_variance=m_.scalar(data+0x54)*m_.scalar(data+0x50),motion_base=m_.scalar(data+0x50)-motion_variance,spread=m_.scalar(data+0x4c),half_spread=spread*0.5f,angle_range=m_.scalar(data+0x48)*m_.scalar(0x324c7c);
   const auto frames_bits=(1u<<(static_cast<unsigned>(m_.signed_byte(data+0x19))&31))<<(static_cast<unsigned>(m_.signed_byte(data+0x1a))&31);const float frames=m_.byte(data+0x1c)?static_cast<float>(static_cast<std::int32_t>(frames_bits)):1;
   std::size_t iterations=0;do{
    if(++iterations>::port::track::emission_iteration_budget)hp2::not_implemented("0x00128738","Port execution budget exhausted while preserving original zero-interval emission retries");remaining-=countdown;
    auto rotation=hp2::recovered::bIdentity();std::array<float,4> direction;
    if(!m_.byte(data+0x18)){
     const float x=random()*spread-half_spread,y=random()*spread-half_spread,z=random()*spread-half_spread;
     hp2::recovered::eRotateX(m_,rotation,m_.matrix(0x2d3950),angle(x));hp2::recovered::eRotateY(m_,rotation,rotation,angle(y));hp2::recovered::eRotateZ(m_,rotation,rotation,angle(z));
     hp2::debug::eMulVector(direction,rotation,{{world[8],world[9],world[10],world[11]}});
    }else{
     const float tilt=random()*spread-half_spread;const auto azimuth=angle(random()*360.f-180.f);
     if(!port_stack_vector_lane)hp2::not_implemented("0x00127E38","Original disc pdir.W is unwritten; explicit port stack-lane policy required");float lane;const auto bits=*port_stack_vector_lane;std::memcpy(&lane,&bits,4);if(!std::isfinite(lane))hp2::not_implemented("0x00127E38","Exceptional chosen original disc stack-W arithmetic remains unmodeled");
     direction={{hp2::recovered::bCos(m_,azimuth),hp2::recovered::bSin(m_,azimuth),0,lane}};
     hp2::recovered::eRotateY(m_,rotation,m_.matrix(0x2d3950),angle(tilt));hp2::debug::eMulVector(direction,rotation,direction);hp2::debug::eMulVector(direction,orientation,direction);
    }
    std::array<float,4> position;for(unsigned axis=0;axis<3;++axis){const float extent=m_.scalar(data+0x70+axis*4);position[axis]=m_.scalar(data+0x60+axis*4)+(random()*extent-extent*0.5f);}position[3]=1;hp2::debug::eMulVector(position,world,position);
    const float frame_random=random();float life=(life_base-life_variance)+random()*life_variance;const float angle_random=random();float speed=(speed_base-speed_variance)+random()*speed_variance,inherit=0;
    if(motion_base!=0){inherit=motion_base+random()*motion_variance;if(inherit<0)inherit=0;if(inherit>1)inherit=1;if(!m_.byte(data+0x1e)){float square[3];for(unsigned axis=0;axis<3;++axis){direction[axis]=speed*direction[axis]+inherit*m_.scalar(emitter+0x70+axis*4);square[axis]=direction[axis]*direction[axis];}const float magnitude=(square[0]+square[1])+square[2];speed=std::sqrt(magnitude);const float inverse=magnitude==0?::port::driving::ee_vrsqrt_one_zero(magnitude,"0x00128510"):1.f/speed;for(unsigned axis=0;axis<3;++axis)direction[axis]*=inverse;}}
    auto initial_angle=static_cast<std::uint16_t>(::port::driving::cvt_w_s((-angle_range*0.5f)+angle_random*angle_range)&0xfffe);if(m_.byte(data+0x1d))initial_angle=static_cast<std::uint16_t>(initial_angle|(seed&1));if(life>60)life=60;
    if(!m_.word(emitter+0x28)&&!full(particles)){
     const auto particle=SlotPool(m_,particles).Malloc();m_.byte(particle+31,static_cast<std::uint8_t>(::port::driving::cvt_w_s(frame_random*frames)));m_.byte(particle+30,static_cast<std::uint8_t>(::port::driving::cvt_w_s(direction[2]*127.f)));m_.byte(particle+14,static_cast<std::uint8_t>(::port::driving::cvt_w_s(inherit*255.f)));m_.byte(particle+28,static_cast<std::uint8_t>(::port::driving::cvt_w_s(direction[0]*127.f)));m_.byte(particle+29,static_cast<std::uint8_t>(::port::driving::cvt_w_s(direction[1]*127.f)));
     const float packed_speed=std::min(m_.scalar(0x324c80),speed*128.f);const auto head=group+20,tail=m_.word(head+4);m_.word(tail,particle);m_.word(head+4,particle);m_.half(particle+8,static_cast<std::uint16_t>(::port::driving::cvt_w_s(life*1024.f)));m_.half(particle+10,initial_angle);m_.half(particle+12,static_cast<std::uint16_t>(::port::driving::cvt_w_s(packed_speed)));m_.scalar(particle+20,position[1]);m_.scalar(particle+16,position[0]);m_.scalar(particle+24,position[2]);m_.word(particle+4,tail);m_.word(particle,head);
     // Source never initializes AcidParticle.Pad+0F.
    }
    const float denominator=(number-number_variance)+random()*number_variance;countdown=denominator==0?::port::driving::ee_div_s_zero(1.f,denominator,"0x00128704"):1.f/denominator;if(countdown>240)countdown=240;if(countdown<0)countdown=0;
   }while(countdown<=remaining);
   m_.word(emitter+0x24,seed);if(m_.word(group+20)==group+20){::port::track::emitter_detach(m_,group);AcidActiveGroup(m_,group).Sub_0012A3B0(3);}
  }
 }
 m_.scalar(emitter+0x18,timestep>=0?countdown-remaining:0.f);
}
}

namespace hp2::debug {
// Debug0012A4A0/0012A728, E-0099-W53. bTNode destructor is empty; only
// original deletion flag frees the actual shared particle slot, no field reset.
void AcidParticle::Sub_0012A4A0(int flags){if(flags&1)SlotPool(m_,m_.word(0x326d60)).Free(address_);}
// Debug0012A3B0/0012A418/0012A490, E-0099-W53. Caller unlinks the group;
// source deliberately retains emitter.LastActiveGroup's stale cache pointer.
void AcidActiveGroup::Sub_0012A3B0(int flags){m_.word(address_+12,0);m_.word(address_+8,0);const auto head=address_+20;while(m_.word(head)!=head){const auto p=m_.word(head);::port::track::emitter_detach(m_,p);AcidParticle(m_,p).Sub_0012A4A0(3);}if(flags&1)SlotPool(m_,m_.word(0x326d64)).Free(address_);}
}

#include "game_core/port/track/acidfx_services.hpp"
#include "game_core/port/race/car_render_resources.hpp"
namespace hp2::debug {
// Debug001287F8, E-0099-W53. Exact1/2/0 view order, full16-byte camera
// CurrentKey.Position copies. Null camera preserves prior interest point.
void AcidEffectSystem::UpdateInterestPoints(){constexpr int views[3]{1,2,0};for(unsigned i=0;i<3;++i){const auto view=::port::race::eGetView(m_,views[i]),camera=m_.word(view+0x38);if(camera)m_.copy(address_+0x10+i*16,camera+0x40,16);}}
// Debug00128870, E-0099-W53. Actual shared auto-emitter/group/particle lists;
// no host ParticleSimulation ledger or synthetic pool-availability parameter.
void AcidEffectSystem::DoTimeStep(float dt,const ::port::track::AcidEffectServices& services){
 if(!services.ticker)hp2::not_implemented("0x0024A340","AcidFX update requires an explicit original Count-unit ticker");const auto start=services.ticker();const auto ticks=::port::driving::cvt_w_s(dt*1024.f);UpdateInterestPoints();
 const auto auto_head=address_+8;for(auto emitter=m_.word(auto_head);emitter!=auto_head;){const auto next=m_.word(emitter),state=m_.word(emitter+0x10);if(state==1)TriggerEmitter(emitter,dt,0,services.port_stack_vector_lane);const bool stop=state>1;float remaining=m_.scalar(emitter+0x14);
  if(remaining!=0&&state<2){remaining-=dt;if(remaining<=0){const auto data=m_.word(emitter+0x1c);if(!data)hp2::not_implemented("0x0012892C","Original cycling emitter requires loaded data");const auto off=state?0x28u:0x20u;float duration=m_.scalar(data+off);const float variance=m_.scalar(data+off+4);if(duration!=0){auto seed=m_.word(emitter+0x24);seed^=seed<<17;seed^=seed>>13;seed^=seed<<5;m_.word(emitter+0x24,seed);duration=(duration-variance*duration)+((static_cast<float>(seed&0x7fffffff)*4.656612873077392578125e-10f)*variance)*duration+remaining;}m_.word(emitter+0x10,state==0);m_.scalar(emitter+0x14,duration);m_.word(emitter+0x18,0);}else m_.scalar(emitter+0x14,remaining);}
  if(stop)break;emitter=next;
 }
 if(m_.word(0x3275ac)==3){for(auto data=m_.word(0x326d70);data!=0x326d70;data=m_.word(data)){
   std::array<float,4> keys;::port::driving::SolverMatrix extra,colour,basis;for(unsigned row=0;row<4;++row){keys[row]=m_.scalar(data+0x90+row*4)*m_.scalar(0x324c84);extra[row*4]=m_.scalar(data+0xe0+row*4);extra[row*4+1]=m_.scalar(data+0xf0+row*4)*m_.scalar(0x324c88);}
   hermite_basis(colour,m_.matrix(data+0xa0),keys[0],keys[1],keys[2],keys[3]);m_.matrix(data+0x100,colour);if(!services.port_extra_control_lanes)hp2::not_implemented("0x00128B0C","Original frontend extra-control Z/W stack lanes require explicit port policy");for(unsigned row=0;row<4;++row)for(unsigned lane=0;lane<2;++lane){const auto bits=(*services.port_extra_control_lanes)[row*2+lane];std::memcpy(&extra[row*4+2+lane],&bits,4);}hermite_basis(basis,extra,keys[0],keys[1],keys[2],keys[3]);m_.matrix(data+0x140,basis);
  }}
 for(auto group=m_.word(address_);group!=address_;){const auto next_group=m_.word(group),data=m_.word(group+8),head=group+20;const int fps=m_.signed_byte(data+0x1b);const int interval=fps?::port::driving::cvt_w_s((1.f/static_cast<float>(fps))*1024.f):0;const bool live=m_.byte(data+0x1e)!=0;std::array<float,3> inherited{{0,0,0}};const auto car=m_.word(group+16);if(live&&car&&0<m_.scalar(data+0x50))for(unsigned axis=0;axis<3;++axis)inherited[axis]=dt*m_.scalar(car+0x70+axis*4);
  for(auto particle=m_.word(head);particle!=head;){const auto next=m_.word(particle);const int old_life=m_.signed_half(particle+8),life=static_cast<std::int32_t>(static_cast<std::uint32_t>(old_life)-static_cast<std::uint32_t>(ticks));if(life<1){::port::track::emitter_detach(m_,particle);AcidParticle(m_,particle).Sub_0012A4A0(3);}else{
    auto frame=m_.byte(particle+31);if(interval){const auto mask=((1u<<(static_cast<unsigned>(m_.signed_byte(data+0x19))&31))<<(static_cast<unsigned>(m_.signed_byte(data+0x1a))&31))-1;frame=static_cast<std::uint8_t>((frame+old_life/interval-life/interval)&mask);}
    const float scale=(static_cast<float>(m_.half(particle+12))*0.0078125f)*m_.scalar(0x324c8c),drag=m_.scalar(data+0x88),inherit=static_cast<float>(m_.byte(particle+14))*m_.scalar(0x324c90);std::array<float,3> velocity,position;float squares[3];for(unsigned axis=0;axis<3;++axis){const float v=static_cast<float>(m_.signed_byte(particle+28+axis))*scale,sign=v<0?1.f:-1.f;if(axis==2){const float v_drag=v*drag;const float drag_force=(sign*v)*v_drag;velocity[axis]=v+(-m_.scalar(data+0x84)+drag_force)*dt;}else{const float square=v*v;const float drag_force=(sign*drag)*square;velocity[axis]=v+drag_force*dt;}position[axis]=m_.scalar(particle+16+axis*4)+velocity[axis]*dt;if(live)position[axis]+=inherited[axis]*inherit;squares[axis]=velocity[axis]*velocity[axis];}
    const float magnitude=std::sqrt((squares[0]+squares[1])+squares[2]);const float inverse=magnitude==0?::port::driving::ee_div_s_zero(1.f,magnitude,"0x00128DBC"):1.f/magnitude;m_.byte(particle+31,frame);m_.half(particle+8,static_cast<std::uint16_t>(life));for(unsigned axis=0;axis<3;++axis)m_.scalar(particle+16+axis*4,position[axis]);m_.half(particle+12,static_cast<std::uint16_t>(::port::driving::cvt_w_s(magnitude*128.f)));for(unsigned axis=0;axis<3;++axis)m_.byte(particle+28+axis,static_cast<std::uint8_t>(::port::driving::cvt_w_s((velocity[axis]*inverse)*127.f)));
   }particle=next;
  }
  if(m_.word(head)==head){::port::track::emitter_detach(m_,group);AcidActiveGroup(m_,group).Sub_0012A3B0(3);}group=next_group;
 }
 const auto end=services.ticker();const float rate=m_.scalar(0x326254);if(rate==0||!std::isfinite(rate))hp2::not_implemented("0x0024A350","Original ticker rate must be initialized for AcidFX update profile");m_.scalar(address_+0x5c,(static_cast<float>(end-start)/rate)*1000.f);
}
}

namespace hp2::debug {
// Debug001277E0, E-0099-W57. Original unlink and NumAutoEmitters decrement;
// no detached-state repair or implicit ownership transfer.
void AcidEffectSystem::RemoveAutoEmitter(std::uint32_t emitter){const auto prev=m_.word(emitter+4),next=m_.word(emitter);const auto count=m_.word(address_+0x50);m_.word(prev,next);m_.word(next+4,prev);m_.word(address_+0x50,count-1);}
}

namespace hp2::debug {
// Debug001275F8, E-0099-W57. Exact raw registration, cycle/RNG initialization,
// active-at-head and inactive-at-tail ordering; caller supplies a detached emitter.
void AcidEffectSystem::AddAutoEmitter(std::uint32_t emitter){
 if(!m_.word(emitter+0x24)){auto seed=m_.word(0x326d78);seed^=seed<<17;seed^=seed>>13;seed^=seed<<5;m_.word(0x326d78,seed);m_.word(emitter+0x24,static_cast<std::uint32_t>(::port::driving::cvt_w_s((static_cast<float>(seed&0x7fffffff)*4.656612873077392578125e-10f)*2147483648.f)));}
 const auto data=m_.word(emitter+0x1c),state=m_.word(emitter+0x10),head=address_+8;
 if(data&&state<2){const auto field=state?0x20u:0x28u;float duration=m_.scalar(data+field);const float variance=m_.scalar(data+field+4);if(duration!=0){auto seed=m_.word(emitter+0x24);seed^=seed<<17;seed^=seed>>13;seed^=seed<<5;m_.word(emitter+0x24,seed);duration=(duration-variance*duration)+((static_cast<float>(seed&0x7fffffff)*4.656612873077392578125e-10f)*variance)*duration;}
  const auto first=m_.word(head);m_.word(head,emitter);m_.word(first+4,emitter);m_.word(emitter,first);m_.scalar(emitter+0x14,duration);m_.word(emitter+4,head);m_.word(emitter+0x18,0);
 }else{if(!data||state!=3)m_.word(emitter+0x10,2);const auto tail=m_.word(head+4);m_.word(tail,emitter);m_.word(head+4,emitter);m_.word(emitter,head);m_.word(emitter+4,tail);}
 m_.word(address_+0x50,m_.word(address_+0x50)+1);
}
}

namespace hp2::debug {
// Debug001275E8/0012A2B0, E-0099-W67. Both bodies are original JR RA/NOP.
void AcidEffectSystem::TweakerCheck(){}
void rain_TimeStep(float){}
}
