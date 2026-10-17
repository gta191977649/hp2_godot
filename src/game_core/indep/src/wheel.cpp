
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001A3F10; E-0042-C113; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving::init {
void Sub_001A3F10(ByteView out,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t t0){
    out.require(0,0x184);
    out.word(8,0x002b5c40);out.word(0x18,t0);out.word(0xc,a1);out.word(0x10,a2);out.word(0x14,a3);
    out.word(0x110,1);out.scalar(0x17c,1);out.word(0x180,0);out.scalar(0x178,1);
    out.data[0x156]=0;out.data[0x157]=0;out.word(0x164,0);out.word(0x15c,0);
}
}

// Retail 0x001A4348; E-0042-C116; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving::init {
void Calibrate(ByteView out,std::uint32_t global){out.word(0x24,global);}
}

// Retail 0x001A4158; E-0042-C114; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving::init {
void SetState(ByteView out,float f12,float f13){out.require(0x1c,8);out.scalar(0x20,f13);out.scalar(0x1c,f12);}
}

// Retail 0x001A4358; E-0041-V01; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving::native {
void Wheel::ComputeAuxiliaries(Memory& m,std::uint32_t object){
    using namespace detail_group_00137100_runtime;
    m.matrix(object+0x30,init::bIdentity());
    const auto angle=m.scalar(object+0x1c);
    const float cosine=port::driving::bSin(m,angle+m.scalar(0x0032e628));
    const float sine=port::driving::bSin(m,angle);
    m.scalar(object+0x58,cosine);m.scalar(object+0x38,-sine);m.scalar(object+0x30,cosine);m.scalar(object+0x50,sine);
    for(auto offset:{0x114,0x118,0x108,0x160})m.word(object+offset,0);
    const auto wrap=m.scalar(0x0032e62c);
    if(wrap<angle){float next=angle-wrap;if(m.scalar(0x0032e630)<next)next=0;m.scalar(object+0x1c,next);}
    else if(angle<m.scalar(0x0032e634)){float next=angle+wrap;if(next<m.scalar(0x0032e638))next=0;m.scalar(object+0x1c,next);}
    const auto mode=m.word(object+0x110);float speed=m.scalar(object+0x20);
    if((mode==0&&speed<0)||(mode==2&&0<speed)){speed=0;m.word(object+0x20,0);}
    m.word(object+0x110,m.scalar(0x0032e63c)<speed?0:(m.scalar(0x0032e640)<=speed?1:2));
}
}

// Retail 0x001A5428; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
void GetHorizontalGroundForceInCarFrame(const SolverWheelProjection& wheel, SolverVector& out) {
    using namespace detail_group_00138f38;
    const float f7 = wheel.Field_0xc8 * wheel.Field_0x118;
    const float f4 = wheel.Field_0xb0 * wheel.Field_0x114;
    const float f3 = wheel.Field_0xc0 * wheel.Field_0x118;
    const float f2 = wheel.Field_0xb4 * wheel.Field_0x114;
    const float f1 = wheel.Field_0xc4 * wheel.Field_0x118;
    const float f0 = wheel.Field_0xb8 * wheel.Field_0x114;
    out[0] = f4 + f3; out[1] = f2 + f1; out[2] = f0 + f7;
}
}

// Retail 0x001A4778; E-0042-C120; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
void CheckForBrakeLock(float f12, SolverWheelProjection& wheel, const SolverWheelGlobals& globals) {
    using namespace detail_group_00138f38;
    float f1 = globals.Field_00330CC0 * wheel.Field_0x170;
    float f2 = globals.Field_00330CC4 * wheel.Field_0x174;
    f2 = f2 * wheel.config_0x14_Field_0x8;
    f1 = f1 * wheel.config_0x14_Field_0x4;
    f1 = f1 + f2;
    f2 = f1 * globals.Field_00330CC8;
    float f0 = std::abs(wheel.Field_0x20);
    f12 = f12 * wheel.config_Field_0xc;
    f0 = f0 * globals.Field_00330CCC;
    f12 = f12 + f0;
    if (f12 < f2) {
        wheel.Field_0x10c = 1.0f < f2 ? 1u : 0u;
        wheel.Field_0x20 = 0.0f;
    } else {
        wheel.Field_0x10c = 0;
    }
}
}

// Retail 0x001A44A8; E-0042-C117; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
float ExchangeZForces(float f12, SolverWheelProjection& wheel, std::int32_t a1) {
    using namespace detail_group_00138f38;
    wheel.Field_0x155 = static_cast<std::uint8_t>(a1);
    if (a1 != 0) {
        const float factor = -wheel.config_Field_0xc;
        for (unsigned i = 0; i < 4; ++i) {
            const float product = wheel.Field_0x140[i] * factor;
            wheel.Field_0x130[i] = product + wheel.Field_0xa0[i];
        }
    }
    if (0.0f < f12) {
        const auto first = cross_xyz(wheel.Field_0x80, wheel.Field_0x140, wheel.Field_0x80[3]);
        const auto second = cross_xyz(first, wheel.Field_0x80, first[3]);
        const float x = second[0] * wheel.Field_0x140[0];
        const float y = second[1] * wheel.Field_0x140[1];
        const float z = second[2] * wheel.Field_0x140[2];
        const float xy = x + y;
        float f0 = (xy + z) * 4.0f;
        f0 = f0 - 3.0f;
        if (f0 < 0.300000011920928955078125f) f0 = 0.300000011920928955078125f;
        wheel.Field_0x158 = f12 * f0;
    } else {
        wheel.Field_0x158 = 0.0f;
    }
    return 0.0f;
}
}

// Retail 0x001A4930; E-0042-C122; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
void SolverDependencies::GuessGroundForce(SolverWheelProjection&, int, std::int8_t) {
    using namespace detail_group_00138f38;
    hp2::not_implemented("0x001A4930", "wheel solver initialization");
}
}

// Retail 0x001A4EE0; E-0042-C123; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
void SolverDependencies::FinalizeGroundForce(SolverWheelProjection&, int) {
    using namespace detail_group_00138f38;
    hp2::not_implemented("0x001A4EE0", "wheel solver finalization");
}
}

// Retail 0x001A4818; E-0042-C121; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
float GetPilotFactor(Memory&m,std::uint32_t a){
    using namespace detail_group_001a4930;
    if(m.word(a+0x10c)!=0 || m.scalar(a+0x20)<0 || m.signed_word(m.word(a+0xc)+0x14)<2)return 1;
    float f=(m.scalar(m.word(a+0x18)+0x380)-g(m,-0x7298))*g(m,-0x7294);
    if(1<f)f=1;else if(f<0)f=0;
    const double first=static_cast<double>(g(m,-0x4c1c));
    const double second=1.0-static_cast<double>(g(m,-0x4c1c));
    const double product=second*static_cast<double>(f);
    return static_cast<float>(first+product);
}
}

// Retail 0x001A4D28; E-0042-U136; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
float GetLateralForce(Memory&m,float request,float argument,std::uint32_t a){
    using namespace detail_group_001a4930;
    if(gw(m,-0x4c10)==0){
        const float lookup=Table::GetValue(m,0x00310f48,argument);
        float f1=g(m,-0x4c14)*lookup;float f2=m.scalar(a+0x158)*g(m,-0x4c20);
        return f1*f2;
    }
    const auto c=m.word(a+0x10);float f2=12.0f/m.scalar(c);
    float f0=argument*g(m,-0x7274);request=request*g(m,-0x4c08);
    f0=f0*f2;request=request/m.scalar(c+4);f0=f0*0.5f;
    const auto index=cvt_w_s(f0);const float fraction=f0-static_cast<float>(index);
    float value;
    if(index<6){
        const auto p=0x003110b0u+static_cast<std::uint32_t>(index)*4;
        float first=Table::GetValue(m,m.word(p),request);
        float second=Table::GetValue(m,m.word(p+4),request);
        second=second-first;second=fraction*second;value=first+second;
    }else value=Table::GetValue(m,m.word(0x003110c8),request);
    const float factor=m.scalar(c+8)*g(m,-0x4c0c);return factor*value;
}
}

// Retail 0x001A4930; E-0042-C122; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
void GuessGroundForce(Memory&m,std::uint32_t a,std::int32_t,std::int32_t a2){
    using namespace detail_group_001a4930;
    const auto config=m.word(a+0x14);float f3=g(m,-0x4c30)*m.scalar(a+0x170);
    float f0=g(m,-0x4c2c)*m.scalar(a+0x174);const float rate=m.scalar(a+0x20);
    const float f22=f0*m.scalar(config+8);f3=f3*m.scalar(config+4);
    if(0<rate)f3=-f3;
    if(m.word(a+0x10c)==0)m.scalar(a+0x108,m.scalar(a+0x108)+f3);
    float second=f22;if(0<rate)second=-second;
    if(m.word(a+0x10c)==0)m.scalar(a+0x108,m.scalar(a+0x108)+second);
    const auto v=local(m,a);float f20=angle(m,v[0],v[1],-0x7290);
    f0=m.scalar(a+0x20)*m.scalar(m.word(a+0xc)+0xc);f0=f0-v[0];
    m.scalar(a+0x160,f0);m.scalar(a+0x164,f0);
    if(f0 < -g(m,-0x4c34) && m.scalar(a+0x108)<0 && f22==0 && a2==0)
        m.scalar(a+0x108,m.scalar(a+0x108)*g(m,-0x7280));
    const float f23=m.scalar(a+0x160);float f1=v[1]*v[1];f0=f23*f23;
    f0=f0+f1;const float f24=std::sqrt(f0);bool branch=false;float scale=1,f21=0;
    if(f24!=0 && (v[1]!=0 || v[0]!=0)){
        auto c=m.word(a+0x10);f0=m.scalar(a+0x178)*m.scalar(0x00318c24u+m.word(a+0x150)*0x90);
        float f2=m.scalar(c+0x10)*m.scalar(a+0x17c);scale=f2*f0;
        if(m.word(m.word(a+0x18)+0x39c)==1)scale=scale*GetPilotFactor(m,a);
        float f12=g(m,-0x4c20)*m.scalar(a+0x158);f12=f12*scale;f21=f12/f24;
        f0=v[0]*v[0];f2=v[1]*v[1];f0=f0+f2;f0=std::sqrt(f0);
        f12=f12/f0;f12=std::abs(v[0])*f12;threshold(m,f12,a);
        branch=g(m,-0x4c34)<std::abs(f23) || m.scalar(m.word(a+0x10))<std::abs(f20) || m.word(a+0x10c)!=0;
    }
    if(branch){
        float f12=f21*f23;float f3out=(-f21)*v[1];m.byte(a+0x154,1);
        m.scalar(a+0x114,f12);m.scalar(a+0x118,f3out);
        if(m.scalar(m.word(a+0x18)+0x380)*g(m,-0x727c)<1 && g(m,-0x7278)<scale){
            m.scalar(a+0x118,f3out/scale);m.scalar(a+0x114,f12/scale);
        }
        m.scalar(a+0x104,v[1]);m.scalar(a+0x100,f23);
    }else{
        m.byte(a+0x154,0);m.scalar(a+0x114,m.scalar(a+0x108)/m.scalar(m.word(a+0xc)+0xc));
        const float sign=0<v[0]?1.0f:-1.0f;
        float result=GetLateralForce(m,g(m,-0x4c20)*m.scalar(a+0x158),std::abs(f20),a);
        if(0<f20)result=-result;
        m.scalar(a+0x118,sign*result);
        m.word(a+0x104,0);m.word(a+0x100,0);
    }
}
}

// Retail 0x001A4EE0; E-0042-C123; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
void FinalizeGroundForce(Memory&m,std::uint32_t a,std::int32_t){
    using namespace detail_group_001a4930;
    bool scaled=false;
    if(0<m.scalar(a+0x108) && m.word(a+0x10c)==0){scaled=true;m.scalar(a+0x114,m.scalar(a+0x114)*g(m,-0x4c00));}
    float x=m.scalar(a+0x114),y=m.scalar(a+0x118);float f1=x*x,f0=y*y;f1=f1+f0;
    const float length=std::sqrt(f1);const auto c=m.word(a+0x10);
    f1=g(m,-0x4c20)*m.scalar(a+0x17c);
    f0=m.scalar(0x00318c20u+m.word(a+0x150)*0x90)*m.scalar(c+0xc);
    float f2=m.scalar(a+0x158)*m.scalar(a+0x178);f1=f1*f0;
    float limit=f1*f2;if(m.word(m.word(a+0x18)+0x39c)==1)limit=limit*GetPilotFactor(m,a);
    if(limit<length && g(m,-0x7270)<length){
        const float factor=limit/length;m.scalar(a+0x114,m.scalar(a+0x114)*factor);m.scalar(a+0x118,m.scalar(a+0x118)*factor);
    }else if(scaled)m.scalar(a+0x114,m.scalar(a+0x114)/g(m,-0x4c00));
    float ratio=1;if(g(m,-0x726c)<limit){ratio=length/limit;if(1<ratio)ratio=1;}
    if(m.signed_byte(a+0x154)!=0 || m.word(a+0x10c)!=0)ratio=1;
    if(m.signed_byte(a+0x155)==0)ratio=0;
    m.scalar(a+0x168,ratio);
    const auto v=local(m,a);float theta=angle(m,v[0],v[1],-0x7268);
    float f20=bSin(m,theta)*m.scalar(a+0x118);
    // The sine-derived temporary is computed but only f20 affects stores.
    const float lateral=bSin(m,theta+g(m,-0x7258))*m.scalar(a+0x118);(void)lateral;
    if(1<std::abs(v[0])){f0=f20*0.5f;m.scalar(a+0x114,m.scalar(a+0x114)-f0);}
    //001A3F00 is confirmed empty; the debug comparison001A51B4 has no branch consumer.
    SolverMatrix matrix{};matrix[0]=matrix[5]=matrix[10]=matrix[15]=1;
    const auto mode=gw(m,-0x4c04);const auto first=m.vector(a+0x70),second=m.vector(a+0x80),normal=m.vector(a+0x140);
    SolverVector row0{},row1{},row2{};
    if(mode==1){row0=first;row1=second;row2=m.vector(a+0x90);}
    else if(mode==2){
        const float p0=-dot(normal,first),p1=-dot(normal,second);
        for(unsigned i=0;i<4;++i){f0=normal[i]*p0;row0[i]=f0+first[i];f0=normal[i]*p1;row1[i]=f0+second[i];}
        row0[3]=row1[3]=0;row2=normal;row2[3]=0;
    }else if(mode==3){
        const float p=-dot(normal,first);for(unsigned i=0;i<4;++i){f0=normal[i]*p;row0[i]=f0+first[i];}
        const float length2=dot(row0,row0);
        if(!(0<length2) || !std::isfinite(length2))
            hp2::not_implemented("0x001A52EC","VU reciprocal-square-root exceptional operand");
        const float q=1.0f/std::sqrt(length2);
        for(float& lane:row0)lane=lane*q;
        row0[3]=0;row2=normal;row2[3]=0;
        row1=cross(row2,row0,row2[3]);row1[3]=0;
    }else if(mode==4){row0=first;row1=second;row2=normal;row2[3]=0;}
    if(mode>=1 && mode<=4){for(unsigned i=0;i<4;++i){matrix[i]=row0[i];matrix[4+i]=row1[i];matrix[8+i]=row2[i];}}
    m.vector(a+0x120,transform(m,matrix,{{m.scalar(a+0x114),m.scalar(a+0x118),m.scalar(a+0x158),0}}));
}
}

// Retail 0x001A4170; E-0042-C115; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
void SetState(Memory&m,float f12,float f13,float f14,std::uint32_t a,
                 std::uint32_t car,std::uint32_t,std::uint16_t steering,ContactProvider&provider){
    using namespace detail_group_001a4930;
    const auto index=m.signed_word(m.word(a+0xc)+0x14);
    m.scalar(a+0x20,f13);m.scalar(a+0x1c,f12);
    m.half(car+0x366,steering);const auto offset=static_cast<std::uint32_t>(index)*4;
    m.scalar(car+offset+0x2b0,f13);m.scalar(car+offset+0x2c0,f12);m.scalar(car+offset+0x300,f14);
    CalculateWheelMatrices(m,car,index);
    auto body=m.matrix(car+0x50);auto position=m.vector(car+0x20);position[3]=1;
    for(unsigned i=0;i<4;++i)body[12+i]=position[i];
    const auto local_matrix=m.matrix(car+static_cast<std::uint32_t>(index)*0x40+0xb0);
    SolverMatrix world{};eMulMatrix(world,local_matrix,body);
    m.matrix(a+0xb0,local_matrix);m.matrix(a+0x70,world);
    SolverVector delta{},velocity{};for(unsigned i=0;i<4;++i)delta[i]=world[12+i]-m.scalar(car+0x20+i*4);
    // VOPMSUB.XYZ preserves the old delta W.
    velocity=cross(delta,m.vector(car+0x90),delta[3]);const auto linear=m.vector(car+0x30);
    for(unsigned i=0;i<4;++i)velocity[i]=velocity[i]+linear[i];
    m.vector(a+0xf0,velocity);
    Memory::Scratch scratch(m);const auto temporary=m.allocate(0x40);
    m.vector(temporary,{{world[12],world[13],world[14],world[15]}});
    const float height=provider.GetElevation(m,m.word(car+4),temporary,temporary+0x20,temporary+0x10,0);
    const auto slot=car+0x310+static_cast<std::uint32_t>(index);const auto old=m.signed_byte(slot);m.byte(slot,1);
    if(old==0)m.word(car+0x37c,m.word(car+0x37c)+1u);
    const auto material=m.signed_word(temporary+0x20);m.word(car+offset+0x314,static_cast<std::uint32_t>(material));
    if(m.signed_word(car+offset+0x324)<material)m.word(car+offset+0x324,static_cast<std::uint32_t>(material));
    m.scalar(a+0x130,world[12]);m.scalar(a+0x134,world[13]);m.scalar(a+0x138,height);
    m.vector(a+0x140,m.vector(temporary+0x10));m.word(a+0x150,static_cast<std::uint32_t>(material));
    m.byte(a+0x155,1);
}
}

// Retail 0x001A46B0; E-0041-V01; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
std::uint32_t Wheel::GetDerivative(Memory&m,std::uint32_t a,std::uint32_t out){
    using namespace detail_group_001a4930;
    m.scalar(out,m.scalar(a+0x20));
    const auto c=m.word(a+0xc);float f4=m.scalar(a+0x20);
    float f2=f4*m.scalar(0x00318c28u+m.word(a+0x150)*0x90);
    float f3=m.scalar(a+0x114)*m.scalar(c+0xc);
    float f0=m.scalar(a+0x108);float f1=m.scalar(a+0x16c);
    f2=f2*g(m,-0x72a4);f0=f0-f3;f1=f1*g(m,-0x72a0);
    f0=f0-f2;f4=f4*f1;f0=f0-f4;f0=f0/m.scalar(a+0x24);
    m.scalar(out+4,f0);if(m.word(a+0x10c)!=0)m.word(out+4,0);
    if(m.signed_byte(a+0x154)==0 && g(m,-0x729c)<g(m,-0x4bf8)){
        f0=m.scalar(c+0xc)*g(m,-0x4bf8);f2=m.scalar(a+0x160)/f0;
        m.scalar(out+4,m.scalar(out+4)-f2);
    }
    return 0;
}
}

// Retail 0x001A4E88; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {
void ModifyGroundForce(WheelIterationProjection& a0,const std::array<float,2>& a1){
    float f3=a0.Field_0x114;
    const float f4=a0.config_Field_0xc;
    float f0=a0.Field_0x108;
    float f1=f3*f4;
    const float f7=a0.Field_0x24;
    const float f6=a1[1],f5=a1[0];
    f0=f0-f1;
    float f2=a0.Field_0xb0;f1=a0.Field_0xc0;
    f2=f2*f5;f0=f0/f7;f1=f1*f6;f2=f2+f1;
    f0=f0*f4;f0=f0-f2;f0=f0*f7;f0=f0/f4;f3=f3+f0;
    a0.Field_0x114=f3;
}
}

#include "game_core/indep/src/wheel.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001A3F10; E-0042-C113; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a3f10.md.
void Wheel::Sub_001A3F10() {
    hp2::not_implemented("0x001A3F10", "Wheel::Sub_001A3F10 (signature and full body unresolved)");
}

// ELF 0x001A4158; E-0042-C114; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4158.md.
void Wheel::Sub_001A4158() {
    hp2::not_implemented("0x001A4158", "Wheel::Sub_001A4158 (signature and full body unresolved)");
}

// ELF 0x001A4170; E-0042-C115; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4170.md.
void Wheel::Sub_001A4170() {
    hp2::not_implemented("0x001A4170", "Wheel::Sub_001A4170 (signature and full body unresolved)");
}

// ELF 0x001A4348; E-0042-C116; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4348.md.
void Wheel::Calibrate() {
    hp2::not_implemented("0x001A4348", "Wheel::Calibrate (signature and full body unresolved)");
}

// ELF 0x001A4358; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4358.md.
void Wheel::ComputeAuxiliaries() {
    hp2::not_implemented("0x001A4358", "Wheel::ComputeAuxiliaries (signature and full body unresolved)");
}

// ELF 0x001A44A8; E-0042-C117; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a44a8.md.
void Wheel::ExchangeZForces() {
    hp2::not_implemented("0x001A44A8", "Wheel::ExchangeZForces (signature and full body unresolved)");
}

// ELF 0x001A46B0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a46b0.md.
void Wheel::GetDerivative() {
    hp2::not_implemented("0x001A46B0", "Wheel::GetDerivative (signature and full body unresolved)");
}

// ELF 0x001A4778; E-0042-C120; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4778.md.
void Wheel::CheckForBrakeLock() {
    hp2::not_implemented("0x001A4778", "Wheel::CheckForBrakeLock (signature and full body unresolved)");
}

// ELF 0x001A4818; E-0042-C121; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4818.md.
void Wheel::GetPilotFactor() {
    hp2::not_implemented("0x001A4818", "Wheel::GetPilotFactor (signature and full body unresolved)");
}

// ELF 0x001A4930; E-0042-C122; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4930.md.
void Wheel::GuessGroundForce() {
    hp2::not_implemented("0x001A4930", "Wheel::GuessGroundForce (signature and full body unresolved)");
}

// ELF 0x001A4D28; E-0042-U136; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4d28.md.
void Wheel::GetLateralForce() {
    hp2::not_implemented("0x001A4D28", "Wheel::GetLateralForce (signature and full body unresolved)");
}

// ELF 0x001A4E88; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4e88.md.
void Wheel::ModifyGroundForce() {
    hp2::not_implemented("0x001A4E88", "Wheel::ModifyGroundForce (signature and full body unresolved)");
}

// ELF 0x001A4EE0; E-0042-C123; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a4ee0.md.
void Wheel::FinalizeGroundForce() {
    hp2::not_implemented("0x001A4EE0", "Wheel::FinalizeGroundForce (signature and full body unresolved)");
}

// ELF 0x001A5428; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a5428.md.
void Wheel::GetHorizontalGroundForceInCarFrame() {
    hp2::not_implemented("0x001A5428", "Wheel::GetHorizontalGroundForceInCarFrame (signature and full body unresolved)");
}

// ELF 0x001A5520; E-0042-U137; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001a5520.md.
void Wheel::MaybeMakeSkids() {
    hp2::not_implemented("0x001A5520", "Wheel::MaybeMakeSkids (signature and full body unresolved)");
}

// ELF 0x001A5BF0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001a5bf0.md.
void Wheel::Sub_001A5BF0() {
    hp2::not_implemented("0x001A5BF0", "Wheel::Sub_001A5BF0 (signature and full body unresolved)");
}

// ELF 0x001A5C08; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001a5c08.md.
void Wheel::Sub_001A5C08() {
    hp2::not_implemented("0x001A5C08", "Wheel::Sub_001A5C08 (signature and full body unresolved)");
}

// ELF 0x001A5C20; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a5c20.md.
void Wheel::Sub_001A5C20() {
    hp2::not_implemented("0x001A5C20", "Wheel::Sub_001A5C20 (signature and full body unresolved)");
}

// ELF 0x001A5C28; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001a5c28.md.
void Wheel::GetSize() {
    hp2::not_implemented("0x001A5C28", "Wheel::GetSize (signature and full body unresolved)");
}

// ELF 0x001A5C30; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001a5c30.md.
void Wheel::Sub_001A5C30() {
    hp2::not_implemented("0x001A5C30", "Wheel::Sub_001A5C30 (signature and full body unresolved)");
}
} // namespace hp2::recovered
