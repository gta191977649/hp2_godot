#include "game_core/indep/src/wheel.hpp"

// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001A3F10; E-0042-C113; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::port_initialize(std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t t0){
    using namespace port::driving;using namespace port::driving::native;
    auto out=m_.view(address_,0x184);
    out.require(0,0x184);
    out.word(8,0x002ada00);out.word(0x18,t0);out.word(0xc,a1);out.word(0x10,a2);out.word(0x14,a3);
    out.word(0x110,1);out.scalar(0x17c,1);out.word(0x180,0);out.scalar(0x178,1);
    out.data[0x156]=0;out.data[0x157]=0;out.word(0x164,0);out.word(0x15c,0);
}
}
namespace port::driving::init {
void Sub_001A3F10(ByteView out,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t t0){port::driving::Memory m;m.map(0x1000,std::vector<std::uint8_t>(out.data,out.data+out.size));hp2::recovered::Wheel(m,0x1000).port_initialize(a1,a2,a3,t0);std::memcpy(out.data,m.view(0x1000,out.size).data,out.size);}

}

// Retail 0x001A4348; E-0042-C116; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::Calibrate(port::driving::init::ByteView out,std::uint32_t global){using namespace port::driving;using namespace port::driving::init;out.word(0x24,global);}
}
namespace port::driving::init {
void Calibrate(ByteView out,std::uint32_t global){hp2::recovered::Wheel::Calibrate(out,global);}

}

// Retail 0x001A4158; E-0042-C114; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::SetState(port::driving::init::ByteView out,float f12,float f13){using namespace port::driving;using namespace port::driving::init;out.require(0x1c,8);out.scalar(0x20,f13);out.scalar(0x1c,f12);}
}
namespace port::driving::init {
void SetState(ByteView out,float f12,float f13){hp2::recovered::Wheel::SetState(out,f12,f13);}

}

// Retail 0x001A4358; E-0041-V01; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::ComputeAuxiliaries(){
    using namespace port::driving;using namespace port::driving::native;
    auto& m=m_;const auto object=address_;
    using namespace detail_group_00137100_runtime;
    m.matrix(object+0x30,init::bIdentity());
    const auto angle=m.scalar(object+0x1c);
    const float cosine=port::driving::bSin(m,angle+m.scalar(0x003256fc));
    const float sine=port::driving::bSin(m,angle);
    m.scalar(object+0x58,cosine);m.scalar(object+0x38,-sine);m.scalar(object+0x30,cosine);m.scalar(object+0x50,sine);
    for(auto offset:{0x114,0x118,0x108,0x160})m.word(object+offset,0);
    const auto wrap=m.scalar(0x00325700);
    if(wrap<angle){float next=angle-wrap;if(m.scalar(0x00325704)<next)next=0;m.scalar(object+0x1c,next);}
    else if(angle<m.scalar(0x00325708)){float next=angle+wrap;if(next<m.scalar(0x0032570c))next=0;m.scalar(object+0x1c,next);}
    const auto mode=m.word(object+0x110);float speed=m.scalar(object+0x20);
    if((mode==0&&speed<0)||(mode==2&&0<speed)){speed=0;m.word(object+0x20,0);}
    m.word(object+0x110,m.scalar(0x00325710)<speed?0:(m.scalar(0x00325714)<=speed?1:2));
}
}
namespace port::driving::native {
void Wheel::ComputeAuxiliaries(Memory& m,std::uint32_t object){hp2::recovered::Wheel(m,object).ComputeAuxiliaries();}

}

// Retail 0x001A5428; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::GetHorizontalGroundForceInCarFrame(const port::driving::SolverWheelProjection& wheel, port::driving::SolverVector& out){
using namespace port::driving;

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
namespace port::driving {
void GetHorizontalGroundForceInCarFrame(const SolverWheelProjection& wheel, SolverVector& out){hp2::recovered::Wheel::GetHorizontalGroundForceInCarFrame(wheel,out);}

}

// Retail 0x001A4778; E-0042-C120; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::CheckForBrakeLock(float f12, port::driving::SolverWheelProjection& wheel, const port::driving::SolverWheelGlobals& globals){
using namespace port::driving;

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
namespace port::driving {
void CheckForBrakeLock(float f12, SolverWheelProjection& wheel, const SolverWheelGlobals& globals){hp2::recovered::Wheel::CheckForBrakeLock(f12,wheel,globals);}

}

// Retail 0x001A44A8; E-0042-C117; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
float Wheel::ExchangeZForces(float f12, port::driving::SolverWheelProjection& wheel, std::int32_t a1){
using namespace port::driving;

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
namespace port::driving {
float ExchangeZForces(float f12, SolverWheelProjection& wheel, std::int32_t a1){return hp2::recovered::Wheel::ExchangeZForces(f12,wheel,a1);}

}

// Retail 0x001A4930; E-0042-C122; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {

}

// Retail 0x001A4EE0; E-0042-C123; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace port::driving {

}

// Retail 0x001A4818; E-0042-C121; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
float Wheel::GetPilotFactor(){
    using namespace port::driving;using namespace port::driving::native;
    auto& m=m_;const auto a=address_;
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
namespace port::driving {
float GetPilotFactor(Memory&m,std::uint32_t a){return hp2::recovered::Wheel(m,a).GetPilotFactor();}

}

// Retail 0x001A4D28; E-0042-U136; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
float Wheel::GetLateralForce(float request,float argument){
    using namespace port::driving;using namespace port::driving::native;
    auto& m=m_;const auto a=address_;
    using namespace detail_group_001a4930;
    if(gw(m,-0x4c10)==0){
        const float lookup=Table::GetValue(m,0x00308658,argument);
        float f1=g(m,-0x4c14)*lookup;float f2=m.scalar(a+0x158)*g(m,-0x4c20);
        return f1*f2;
    }
    const auto c=m.word(a+0x10);float f2=12.0f/m.scalar(c);
    float f0=argument*g(m,-0x7274);request=request*g(m,-0x4c08);
    f0=f0*f2;request=request/m.scalar(c+4);f0=f0*0.5f;
    const auto index=cvt_w_s(f0);const float fraction=f0-static_cast<float>(index);
    float value;
    if(index<6){
        const auto p=0x003087c0u+static_cast<std::uint32_t>(index)*4;
        float first=Table::GetValue(m,m.word(p),request);
        float second=Table::GetValue(m,m.word(p+4),request);
        second=second-first;second=fraction*second;value=first+second;
    }else value=Table::GetValue(m,m.word(0x003087d8),request);
    const float factor=m.scalar(c+8)*g(m,-0x4c0c);return factor*value;
}
}
namespace port::driving {
float GetLateralForce(Memory&m,float request,float argument,std::uint32_t a){return hp2::recovered::Wheel(m,a).GetLateralForce(request,argument);}

}

// Retail 0x001A4930; E-0042-C122; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::GuessGroundForce(std::uint32_t, std::int32_t a2){
    using namespace port::driving;using namespace port::driving::native;
    auto& m=m_;const auto a=address_;
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
        auto c=m.word(a+0x10);f0=m.scalar(a+0x178)*m.scalar(0x0031030cu+m.word(a+0x150)*0x90);
        float f2=m.scalar(c+0x10)*m.scalar(a+0x17c);scale=f2*f0;
        if(m.word(m.word(a+0x18)+0x39c)==1)scale=scale*hp2::recovered::Wheel(m,a).GetPilotFactor();
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
        float result=hp2::recovered::Wheel(m,a).GetLateralForce(g(m,-0x4c20)*m.scalar(a+0x158),std::abs(f20));
        if(0<f20)result=-result;
        m.scalar(a+0x118,sign*result);
        m.word(a+0x104,0);m.word(a+0x100,0);
    }
}
}
namespace port::driving {
void GuessGroundForce(Memory&m,std::uint32_t a,std::int32_t gf,std::int32_t collision){hp2::recovered::Wheel(m,a).GuessGroundForce(static_cast<std::uint32_t>(gf),collision);}

}

// Retail 0x001A4EE0; E-0042-C123; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::FinalizeGroundForce(std::int32_t wheel){
    using namespace port::driving;using namespace port::driving::native;
    auto& m=m_;const auto a=address_;
    using namespace detail_group_001a4930;
    bool scaled=false;
    if(0<m.scalar(a+0x108) && m.word(a+0x10c)==0){scaled=true;m.scalar(a+0x114,m.scalar(a+0x114)*g(m,-0x4c00));}
    float x=m.scalar(a+0x114),y=m.scalar(a+0x118);float f1=x*x,f0=y*y;f1=f1+f0;
    const float length=std::sqrt(f1);const auto c=m.word(a+0x10);
    f1=g(m,-0x4c20)*m.scalar(a+0x17c);
    f0=m.scalar(0x00310308u+m.word(a+0x150)*0x90)*m.scalar(c+0xc);
    float f2=m.scalar(a+0x158)*m.scalar(a+0x178);f1=f1*f0;
    float limit=f1*f2;if(m.word(m.word(a+0x18)+0x39c)==1)limit=limit*hp2::recovered::Wheel(m,a).GetPilotFactor();
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
    SetSliderValue(wheel,0.0f,1.0f,m.scalar(a+0x168));
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
            hp2::not_implemented("0x001A2AC4","VU reciprocal-square-root exceptional operand");
        const float q=1.0f/std::sqrt(length2);
        for(float& lane:row0)lane=lane*q;
        row0[3]=0;row2=normal;row2[3]=0;
        row1=cross(row2,row0,row2[3]);row1[3]=0;
    }else if(mode==4){row0=first;row1=second;row2=normal;row2[3]=0;}
    if(mode>=1 && mode<=4){for(unsigned i=0;i<4;++i){matrix[i]=row0[i];matrix[4+i]=row1[i];matrix[8+i]=row2[i];}}
    m.vector(a+0x120,port::driving::detail_group_001a4930::transform(m,matrix,{{m.scalar(a+0x114),m.scalar(a+0x118),m.scalar(a+0x158),0}}));
}
}
namespace port::driving {
void FinalizeGroundForce(Memory&m,std::uint32_t a,std::int32_t wheel){hp2::recovered::Wheel(m,a).FinalizeGroundForce(wheel);}

}

// Retail 0x001A4170; E-0042-C115; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::SetState(std::uint32_t car,std::uint32_t,float f12,float f13,std::uint16_t steering,float f14,port::driving::ContactProvider&provider){
    using namespace port::driving;using namespace port::driving::native;
    auto& m=m_;const auto a=address_;

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
namespace port::driving {
void SetState(Memory&m,float f12,float f13,float f14,std::uint32_t a,std::uint32_t car,std::uint32_t info,std::uint16_t steering,ContactProvider&provider){hp2::recovered::Wheel(m,a).SetState(car,info,f12,f13,steering,f14,provider);}

}

// Retail 0x001A46B0; E-0041-V01; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
std::uint32_t Wheel::GetDerivative(std::uint32_t out){
    using namespace port::driving;using namespace port::driving::native;
    auto& m=m_;const auto a=address_;
    using namespace detail_group_001a4930;
    m.scalar(out,m.scalar(a+0x20));
    const auto c=m.word(a+0xc);float f4=m.scalar(a+0x20);
    float f2=f4*m.scalar(0x00310310u+m.word(a+0x150)*0x90);
    float f3=m.scalar(a+0x114)*m.scalar(c+0xc);
    float f0=m.scalar(a+0x108);float f1=m.scalar(a+0x16c);
    f2=f2*m.scalar(0x00325720);f0=f0-f3;f1=f1*m.scalar(0x00325724);
    f0=f0-f2;f4=f4*f1;f0=f0-f4;f0=f0/m.scalar(a+0x24);
    m.scalar(out+4,f0);if(m.word(a+0x10c)!=0)m.word(out+4,0);
    if(m.signed_byte(a+0x154)==0 && m.scalar(0x00325728)<m.scalar(0x00327d78)){
        f0=m.scalar(c+0xc)*m.scalar(0x00327d78);f2=m.scalar(a+0x160)/f0;
        m.scalar(out+4,m.scalar(out+4)-f2);
    }
    return 0;
}
}
namespace port::driving {
std::uint32_t Wheel::GetDerivative(Memory&m,std::uint32_t a,std::uint32_t out){return hp2::recovered::Wheel(m,a).GetDerivative(out);}

}

// Retail 0x001A4E88; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\wheel.cpp
namespace hp2::recovered {
void Wheel::ModifyGroundForce(port::driving::WheelIterationProjection& a0,const std::array<float,2>& a1){
using namespace port::driving;

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
namespace port::driving {
void ModifyGroundForce(WheelIterationProjection& a0,const std::array<float,2>& a1){hp2::recovered::Wheel::ModifyGroundForce(a0,a1);}

}


namespace hp2::recovered {
// Debug001a16d8, E-0044-S01; stack reserve/release only.
void SetSliderValue(int,float,float,float){}
void Wheel::Calibrate(){Wheel::Calibrate(m_.view(address_,0x184),m_.word(0x00327d20));}
void Wheel::SetState(float angle,float velocity){Wheel::SetState(m_.view(address_,0x184),angle,velocity);}
void Wheel::SetState(float velocity){m_.scalar(address_+0x20,velocity);}
void Wheel::SetState(std::uint32_t state){m_.copy(address_+0x1c,state,8);}
void Wheel::GetState(std::uint32_t state){m_.copy(state,address_+0x1c,8);}
std::uint32_t Wheel::GetState(){return address_+0x1c;}
int Wheel::GetSize(){return 2;}
void Wheel::MaybeMakeSkids(float,std::uint32_t){hp2::not_implemented("0x001A2CF8","Wheel::MaybeMakeSkids requires explicit skid/topology services");}
}

namespace hp2::recovered {
float Wheel::ExchangeZForces(int touching,float force){
 auto p=port::driving::native::detail_group_00138e08::wheel(m_,address_);
 const float result=ExchangeZForces(force,p,touching);
 m_.byte(address_+0x155,p.Field_0x155);m_.vector(address_+0x130,p.Field_0x130);m_.scalar(address_+0x158,p.Field_0x158);return result;
}
void Wheel::CheckForBrakeLock(float force){port::driving::detail_group_001a4930::threshold(m_,force,address_);}
void Wheel::ModifyGroundForce(std::uint32_t acceleration){
 auto p=port::driving::native::detail_group_00138e08::wheel(m_,address_);
 port::driving::WheelIterationProjection q;q.Field_0x114=p.Field_0x114;q.config_Field_0xc=p.config_Field_0xc;q.Field_0x108=p.Field_0x108;q.Field_0x24=p.Field_0x24;q.Field_0xb0=p.Field_0xb0;q.Field_0xc0=p.Field_0xc0;
 ModifyGroundForce(q,{{m_.scalar(acceleration),m_.scalar(acceleration+4)}});m_.scalar(address_+0x114,q.Field_0x114);
}
void Wheel::GetHorizontalGroundForceInCarFrame(std::uint32_t output){
 auto p=port::driving::native::detail_group_00138e08::wheel(m_,address_);port::driving::SolverVector result{};
 GetHorizontalGroundForceInCarFrame(p,result);for(unsigned i=0;i<3;++i)m_.scalar(output+4*i,result[i]);
}
}


namespace hp2::recovered {
float Wheel::CheckTerrain(std::uint32_t normal_out,std::uint32_t material_out,std::uint32_t point,std::uint32_t body,port::driving::ContactProvider&provider){
using namespace port::driving;auto& m=m_;const auto wheel=address_;
    using namespace detail_group_001a4930;
    const float height=provider.GetElevation(m,m.word(m.word(wheel+0x18)+4),point,material_out,normal_out,0);
    const float roughness=Sub_00153498(m,point,m.signed_word(material_out));m.scalar(wheel+0x15c,roughness);
    const auto normal=m.vector(normal_out);const float denominator=dot(normal,m.vector(body+0x20));
    if(denominator<g(m,-0x72a8)){m.word(material_out,0);return -100.0f;}
    // Query XY are copied unchanged into the temporary, so XYZ difference is(0,0,Z).
    SolverVector delta{{0,0,(height+roughness)-m.scalar(point+8),0}};
    return dot(normal,delta)/denominator;
}
float Wheel::FindDrivingSurface(std::uint32_t point,std::uint32_t body,port::driving::ContactProvider&provider){
using namespace port::driving;auto& m=m_;const auto wheel=address_;
    using namespace detail_group_001a4930;
    return Sub_001A45A8(m,wheel,wheel+0x140,wheel+0x150,point,body,provider);
}
}

#include "game_core/src/bMath.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/table.hpp"

// Debug 0x001A2FC0; E-0044-M02; source ownership E-0042-S001.
namespace hp2::recovered {
void Sub_001A2FC0(Memory&m) {
    const float maximum=m.scalar(0x0032577c);m.scalar(0x00327d38,maximum);
    hp2::recovered::Table(m,0x00308658).Sub_0018D810(0x00308638,7,0,maximum);
    for(unsigned i=0;i<7;++i)hp2::recovered::Table(m,0x00308718+24*i).Sub_0018D810(0x00308670+24*i,6,0,10);
}
}

#include "game_core/indep/src/skids.hpp"
// Debug001A2CF8; E-0046-S03; original source indep/src/wheel.cpp.
namespace hp2::recovered {
void Wheel::MaybeMakeSkids(float,std::uint32_t car,port::driving::SkidServices&s){
    using namespace port::driving;
    auto&m=m_;const auto w=address_;SkidMaker maker(m,w+0x180);
    const auto wi=m.word(w+0xc);const bool slipping=m.byte(w+0x154)!=0;
    const auto blown=static_cast<std::uint32_t>(static_cast<std::int32_t>(m.signed_byte(car+0x848)));
    const float vx=m.scalar(w+0xf0),vy=m.scalar(w+0xf4);
    if(!(slipping||8.0f<m.scalar(w+0x158)*m.scalar(w+0x168))||!m.byte(w+0x155)||
       ((blown>>(m.word(wi+0x14)&31))&1)||!(1.0f<std::sqrt(vx*vx+vy*vy))){maker.MakeNoSkid();return;}
    validate_skid_services(s);
    if(!s.contact)hp2::not_implemented("0x00151CC0","Wheel skid endpoint topology elevation service");
    SkidVector delta{{-vy,vx,0}};
    const float inv=1.0f/std::sqrt((delta[0]*delta[0]+delta[1]*delta[1])+delta[2]*delta[2]);
    for(auto&v:delta)v=v*inv;
    // Debug001A2C60 bRotateVector(...,WorldMatrix,(0,1,0)) reads row1.
    float dot=m.scalar(w+0x80)*delta[0];dot=dot+m.scalar(w+0x84)*delta[1];dot=dot+m.scalar(w+0x88)*delta[2];
    float intensity=std::abs(dot);const auto terrain=m.signed_word(w+0x150);
    if(terrain<0||terrain>=33)throw std::runtime_error("Invalid skid terrain index");
    float width=m.scalar(wi+0x10)*m.scalar(0x003102e8+terrain*0x90+0x14);
    width=width*(intensity<0.5f?0.5f:intensity);width=width*0.5f;
    delta=hp2::recovered::bNormalize(delta,width);
    const SkidVector center{{m.scalar(w+0xa0),m.scalar(w+0xa4),m.scalar(w+0xa8)}};
    Memory::Scratch scratch(m);const auto right=m.allocate(16),left=m.allocate(16);
    for(unsigned i=0;i<3;++i){m.scalar(right+4*i,center[i]+delta[i]);m.scalar(left+4*i,center[i]-delta[i]);}
    const auto cs=m.word(w+0x18),owner=m.word(cs+4);
    const float rz=s.contact->GetElevation(m,owner,right,0,0,0);
    const float lz=s.contact->GetElevation(m,owner,left,0,0,0);
    SkidVector position{{center[0],center[1],(rz+lz)*0.5f}};
    delta[2]=rz*0.5f-lz*0.5f;delta=hp2::recovered::bNormalize(delta,width);
    if(!slipping)intensity=intensity*m.scalar(0x00325774);
    if(terrain==4)intensity=0;else if(intensity<m.scalar(0x00325778))intensity=m.scalar(0x00325778);
    maker.MakeSkid(m.word(car+0x514)==0xffffffffu?0:car,position,delta,terrain,intensity,s);
}
}
