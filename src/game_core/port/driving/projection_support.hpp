#pragma once
// TASK-0042: shared host utilities; partitions prevent helper name collisions.
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>
#include "game_core/port/driving/projections.hpp"

namespace port::driving::native::detail_group_00137100_runtime {

constexpr std::uint32_t gp=0x003358f0;
inline float global(Memory& m,std::uint16_t offset){return m.scalar(gp+static_cast<std::int16_t>(offset));}
inline std::uint32_t global_address(std::uint16_t offset){return gp+static_cast<std::int16_t>(offset);}
inline float length(const std::array<float,4>& v){return std::sqrt((v[0]*v[0]+v[1]*v[1])+v[2]*v[2]);}
inline std::array<float,4> cross(const std::array<float,4>& a,const std::array<float,4>& b){return {{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0],a[3]}};}
inline std::int32_t signed_bits(std::uint32_t bits){std::int32_t result;std::memcpy(&result,&bits,4);return result;}

inline void reset_dispatch(Memory& m,std::uint32_t code,std::uint32_t object,InitServices& services){
    switch(code){
    case 0x0019d848:Sub_0019D848(m,object);return;
    case 0x001a4358:Wheel::ComputeAuxiliaries(m,object);return;
    case 0x001a5fc0:
        if(!services.contact)hp2::not_implemented("0x001A5FC0","Contact provider is absent");
        Sub_001A5FC0(m,object,*services.contact);return;
    case 0x0018a988:port::driving::Sub_0018A988(m,object);return;
    case 0x0018b4a0:port::driving::Sub_0018B4A0(m,object);return;
    default:hp2::not_implemented("0x0019C808","Unknown component virtual target at slot +0x44");
    }
}
inline void refresh(Memory& m,std::uint32_t object){
    const auto body=m.word(object+0x94),wheel=m.word(object+0x90);
    init::Calibrate(m.view(object,0xa0),m.scalar(object+0x78),m.vector(m.word(wheel+0xc)),m.vector(body+0x160));
}

}

namespace port::driving::native::detail_group_00137d90 {

inline std::uint32_t ga(std::uint16_t off){return 0x3358f0u+static_cast<std::int16_t>(off);}
inline float g(Memory& m,std::uint16_t off){return m.scalar(ga(off));}
inline std::int32_t sb(std::uint32_t v){std::int32_t out;std::memcpy(&out,&v,4);return out;}
template<class F,class... A> auto invoke(F& f,const char* address,A&&... a) {
    if(!f)hp2::not_implemented(address,"Required mover external service is absent");
    return f(std::forward<A>(a)...);
}

}

namespace port::driving::native::detail_group_00138e08 {

inline float g(Memory& m,std::uint16_t off){return m.scalar(0x003358f0u+static_cast<std::int16_t>(off));}
inline float len(const Vector& a){const float x=a[0]*a[0],y=a[1]*a[1],z=a[2]*a[2];return std::sqrt((x+y)+z);}
inline float dot(const Vector& a,const Vector& b){const float x=a[0]*b[0],y=a[1]*b[1],z=a[2]*b[2];return (x+y)+z;}
inline Vector mul(Vector a,float f){for(auto& v:a)v=v*f;return a;}
inline Vector cross(const Vector& a,const Vector& b){return {{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0],a[3]}};}
inline SolverWheelProjection wheel(Memory& m,std::uint32_t a){
    SolverWheelProjection w;w.config_Field_0xc=m.scalar(m.word(a+0xc)+0xc);w.Field_0x24=m.scalar(a+0x24);
    w.Field_0xb0=m.scalar(a+0xb0);w.Field_0xb4=m.scalar(a+0xb4);w.Field_0xb8=m.scalar(a+0xb8);
    w.Field_0xc0=m.scalar(a+0xc0);w.Field_0xc4=m.scalar(a+0xc4);w.Field_0xc8=m.scalar(a+0xc8);
    w.Field_0x108=m.scalar(a+0x108);w.Field_0x114=m.scalar(a+0x114);w.Field_0x118=m.scalar(a+0x118);
    w.Field_0x80=m.vector(a+0x80);w.Field_0x140=m.vector(a+0x140);w.Field_0xa0=m.vector(a+0xa0);
    w.Field_0x130=m.vector(a+0x130);return w;
}
inline void suspension_request(Memory& m,std::uint32_t a,float request){
    const auto target=m.word(a+0x90);auto w=wheel(m,target);const auto flag=m.signed_word(a+0x70);
    float f=m.scalar(a+0x1c);if(flag!=0)f=f+request;
    const auto result=port::driving::ExchangeZForces(f,w,flag);
    m.byte(target+0x155,w.Field_0x155);m.vector(target+0x130,w.Field_0x130);m.scalar(target+0x158,w.Field_0x158);m.scalar(a+0xa0,result);
}

}

namespace port::driving::detail_group_00138f38 {

inline float xyz_length(const SolverVector& v) {
    const float x = v[0] * v[0], y = v[1] * v[1], z = v[2] * v[2];
    const float xy = x + y;
    return std::sqrt(xy + z);
}
// VOPMSUB writes XYZ only. The caller supplies the surviving destination W.
inline SolverVector cross_xyz(const SolverVector& a, const SolverVector& b, float w) {
    const float x0 = a[1] * b[2], x1 = a[2] * b[1];
    const float y0 = a[2] * b[0], y1 = a[0] * b[2];
    const float z0 = a[0] * b[1], z1 = a[1] * b[0];
    return {{x0 - x1, y0 - y1, z0 - z1, w}};
}

}

namespace port::driving::detail_group_0018bce0 {

inline std::int32_t add_one(std::int32_t value) {
    const std::uint32_t bits=static_cast<std::uint32_t>(value)+std::uint32_t{1};
    std::int32_t result; std::memcpy(&result,&bits,sizeof(result)); return result;
}
// Port bounds checks guard projections; these are not original game branches.
inline std::size_t index(std::int32_t value) {
    if(value<0 || value>=9) throw std::out_of_range("Drivetrain projection index outside captured table");
    return static_cast<std::size_t>(value);
}
inline float evaluate(const EngineEvaluation& engine) {
    if(!engine) hp2::not_implemented("0x0018AAB0","engine curve evaluation dependency");
    return engine();
}

}

namespace port::driving::native::detail_group_0018bce0_memory {

inline DrivetrainGlobals globals(Memory& m) {
    DrivetrainGlobals g;
    g.Field_0032e358=m.scalar(0x0032e358);g.Field_0032e35c=m.scalar(0x0032e35c);g.Field_0032e370=m.scalar(0x0032e370);
    g.Field_0032e374=m.scalar(0x0032e374);g.Field_0032e378=m.scalar(0x0032e378);g.Field_0032e388=m.scalar(0x0032e388);
    g.Field_0032e38c=m.scalar(0x0032e38c);g.Field_0032e390=m.scalar(0x0032e390);g.Field_0032e394=m.scalar(0x0032e394);
    g.Field_003308b8=m.scalar(0x003308b8);g.Field_003308bc=m.scalar(0x003308bc);g.Field_003308c4=m.scalar(0x003308c4);
    g.Field_00330c88=m.scalar(0x00330c88);return g;
}
inline DrivetrainProjection load(Memory& m,std::uint32_t a) {
    DrivetrainProjection s;
    s.Field_0xc=m.scalar(a+0xc);s.Field_0x2c=m.signed_word(a+0x2c);s.Field_0x30=m.signed_word(a+0x30);
    s.Field_0x34=m.signed_word(a+0x34);s.Field_0x38=m.signed_word(a+0x38);s.Field_0x40=m.scalar(a+0x40);
    s.Field_0x44=m.scalar(a+0x44);s.Field_0x48=m.signed_word(a+0x48);s.Field_0x4c=m.scalar(a+0x4c);
    const auto config=m.word(a+0x10),engine=m.word(a+0x14),ec=m.word(engine+0x10),car=m.word(a+0x28);
    s.config_Field_0x8=m.scalar(config+8);s.config_Field_0xc=m.scalar(config+0xc);s.config_Field_0x10=m.scalar(config+0x10);s.config_Field_0x14=m.scalar(config+0x14);s.config_Field_0x18=m.signed_word(config+0x18);
    for(unsigned i=0;i<9;++i){s.config_Field_0x20[i]=m.scalar(config+0x20+i*4);s.Field_0x50[i]=m.scalar(a+0x50+i*4);s.Field_0x74[i]=m.scalar(a+0x74+i*4);}
    s.engine_Field_0xc=m.scalar(engine+0xc);s.engine_Field_0x44=m.scalar(engine+0x44);s.engine_Field_0x48=m.scalar(engine+0x48);
    s.engine_config_Field_0x0=m.scalar(ec);s.engine_config_Field_0x8=m.scalar(ec+8);s.engine_config_Field_0xc=m.scalar(ec+0xc);s.car_Field_0x3a4=m.signed_word(car+0x3a4);
    for(unsigned i=0;i<4;++i){const auto w=m.word(a+0x18+i*4);auto& p=s.wheels[i];p.Field_0x20=m.scalar(w+0x20);p.Field_0x24=m.scalar(w+0x24);p.Field_0x108=m.scalar(w+0x108);p.Field_0x114=m.scalar(w+0x114);p.Field_0x10c=m.signed_word(w+0x10c);p.Field_0x155=m.signed_byte(w+0x155);p.Field_0x160=m.scalar(w+0x160);p.config_Field_0xc=m.scalar(m.word(w+0xc)+0xc);}
    return s;
}
inline void store(Memory& m,std::uint32_t a,const DrivetrainProjection& s) {
    m.word(a+0x2c,static_cast<std::uint32_t>(s.Field_0x2c));m.word(a+0x34,static_cast<std::uint32_t>(s.Field_0x34));
    m.scalar(a+0x44,s.Field_0x44);m.word(a+0x48,static_cast<std::uint32_t>(s.Field_0x48));m.scalar(a+0x4c,s.Field_0x4c);
    m.scalar(m.word(a+0x14)+0x48,s.engine_Field_0x48);
    for(unsigned i=0;i<4;++i)m.scalar(m.word(a+0x18+i*4)+0x108,s.wheels[i].Field_0x108);
}

}

namespace port::driving::detail_group_0018f808 {

inline std::int32_t signed_bits(std::uint32_t v) { std::int32_t s; std::memcpy(&s,&v,4); return s; }
// Shared port-side implementation carrier for the instruction sequence0018F838.
template<class Read> float interpolate(float value,float lower,float upper,float step,
                                      std::uint32_t count,Read read) {
    if(value<lower) value=lower;
    else if(upper<value) value=upper;
    float f12=value-lower; f12=f12*step;
    const auto i=cvt_w_s(f12);
    if(i==signed_bits(count-1)) return read(i);
    const float f2=read(i);
    float f0=read(i+1)-f2;
    const float f1=f12-static_cast<float>(i);
    f0=f0*f1; return f2+f0;
}
inline float ratio(Memory& m,std::uint32_t s) {
    return m.scalar(m.word(s+0x10)+0x20+m.word(s+0x30)*4);
}

}

namespace port::driving::native::detail_group_0019d4d0 {

inline float g(Memory& m,std::uint16_t offset) { return m.scalar(0x003358f0u+static_cast<std::int16_t>(offset)); }
inline float length(const Vector& a) { const float x=a[0]*a[0],y=a[1]*a[1],z=a[2]*a[2];return std::sqrt((x+y)+z); }
inline float dot(const Vector& a,const Vector& b) {const float x=a[0]*b[0],y=a[1]*b[1],z=a[2]*b[2];return (x+y)+z;}
inline Vector scale(Vector a,float f) {for(auto& v:a)v=v*f;return a;}
inline void add(Memory& m,std::uint32_t address,const Vector& a) {auto b=m.vector(address);for(unsigned i=0;i<4;++i)b[i]=b[i]+a[i];m.vector(address,b);}
inline std::uint32_t count(Memory& m,std::uint32_t object) {
    const auto table=m.word(object+8),target=m.word(table+0x14);
    switch(target) {
    case 0x0019dcc8:return 32;
    default:break;
    }
    // Slot+14 leaves all return LI v0,<count>. Read only known audited addresses.
    const auto first=m.word(target),second=m.word(target+4);
    if(first==0x03e00008u && (second&0xffff0000u)==0x24020000u) return second&0xffffu;
    hp2::not_implemented("0x0019C290","untranslated component count virtual target");
}

}

namespace port::driving::native::detail_group_0019da30 {

constexpr std::uint32_t gp=0x003358f0;
inline std::uint32_t address(std::uint16_t offset){return gp+static_cast<std::int16_t>(offset);}
inline float g(Memory& m,std::uint16_t offset){return m.scalar(address(offset));}
inline std::int32_t signed_bits(std::uint32_t bits){std::int32_t value;std::memcpy(&value,&bits,4);return value;}

}

namespace port::driving::detail_group_001a4930 {

constexpr std::uint32_t gp=0x003358f0;
inline float g(Memory& m,std::int32_t off){return m.scalar(gp+static_cast<std::uint32_t>(off));}
inline std::uint32_t gw(Memory& m,std::int32_t off){return m.word(gp+static_cast<std::uint32_t>(off));}
inline SolverMathContext context(Memory& m){return {m.vector(0x00320a60)};}
inline SolverVector transform(Memory& m,const SolverMatrix& mat,const SolverVector& v){
    SolverVector out{};bMulMatrix(out,mat,v,context(m));return out;
}
inline SolverVector local(Memory& m,std::uint32_t a){
    SolverMatrix mat{};bTransposeMatrix(mat,m.matrix(a+0x70));
    mat[3]=0;mat[7]=0;mat[11]=0;
    return transform(m,mat,m.vector(a+0xf0));
}
inline float dot(const SolverVector&a,const SolverVector&b){
    const float x=a[0]*b[0],y=a[1]*b[1],z=a[2]*b[2];
    const float xy=x+y;return xy+z;
}
inline SolverVector cross(const SolverVector&a,const SolverVector&b,float w){
    const float x=a[1]*b[2],xx=a[2]*b[1];
    const float y=a[2]*b[0],yy=a[0]*b[2];
    const float z=a[0]*b[1],zz=a[1]*b[0];return {{x-xx,y-yy,z-zz,w}};
}
inline void threshold(Memory&m,float f,std::uint32_t a){
    SolverWheelProjection w;w.Field_0x170=m.scalar(a+0x170);w.Field_0x174=m.scalar(a+0x174);
    w.Field_0x20=m.scalar(a+0x20);const auto c=m.word(a+0x14);
    w.config_0x14_Field_0x4=m.scalar(c+4);w.config_0x14_Field_0x8=m.scalar(c+8);
    w.config_Field_0xc=m.scalar(m.word(a+0xc)+0xc);
    CheckForBrakeLock(f,w,{g(m,-0x4c30),g(m,-0x4c2c),g(m,-0x4c28),g(m,-0x4c24)});
    m.word(a+0x10c,w.Field_0x10c);m.scalar(a+0x20,w.Field_0x20);
}
inline float angle(Memory&m,float x,float y,std::int32_t off){
    float result=static_cast<float>(bATan(m,x,y))*g(m,off);
    const float bound=g(m,off+4);
    if(bound<result)result=result-g(m,off+8);
    if(x<0){result=result+bound;if(bound<result)result=result-g(m,off+12);}
    return result;
}

}

namespace port::driving::detail_group_001a4930 {

// ELF0011B7D8..0011B883; E-0038-T11, port matrix value carrier.

// ELF0011B950..0011BA17; E-0038-T11, rotation matrix supplied to0011B248.


}

namespace port::driving::detail_group_001a4930 {

inline std::int32_t signed_word_bits(std::uint32_t bits){std::int32_t value;std::memcpy(&value,&bits,4);return value;}
inline std::int16_t signed_half_bits(std::uint16_t bits){std::int16_t value;std::memcpy(&value,&bits,2);return value;}

}
