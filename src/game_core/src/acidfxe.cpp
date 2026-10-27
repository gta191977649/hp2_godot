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
