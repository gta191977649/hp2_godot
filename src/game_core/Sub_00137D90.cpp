// TASK-0041 E-0041-R01; retail 0x00137D90: PhysicsCar::Move(float).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#include "Sub_00137D90.hpp"
#include "Sub_00137100_runtime.hpp"
#include "Sub_0018BCE0_memory.hpp"
#include "Sub_0018F808.hpp"
#include "Sub_001A4930.hpp"
#include "Sub_00137C88.hpp"
#include "port/not_implemented.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>
namespace port::driving::native {
namespace {
std::uint32_t ga(std::uint16_t off){return 0x3358f0u+static_cast<std::int16_t>(off);}
float g(Memory& m,std::uint16_t off){return m.scalar(ga(off));}
std::int32_t sb(std::uint32_t v){std::int32_t out;std::memcpy(&out,&v,4);return out;}
template<class F,class... A> auto invoke(F& f,const char* address,A&&... a) {
    if(!f)hp2::not_implemented(address,"Required mover external service is absent");
    return f(std::forward<A>(a)...);
}
}
// ELF001A5D60; E-0038-E07. Decoded raw listing, unsigned-half input is intentional.
void Sub_001A5D60(Memory& m,std::uint32_t s,std::uint16_t value) {
    float f12=static_cast<float>(value)*g(m,0x8dbc);
    const auto config=m.word(s);
    float f2=m.scalar(config+0x120)-m.scalar(config+0x180);
    float f1=m.scalar(config+0x124)-m.scalar(config+0x144);
    if(g(m,0x8dc0)<f12) f12=f12-g(m,0x8dc4);
    bool positive=true;
    if(f12<0){f12=-f12;positive=false;}
    float f0=f1*f12;f1=f2*f12;f0=f2+f0;f0=f1/f0;
    float f22,f23;
    if(positive){f22=f12;f23=f0;}else{f23=-f12;f22=-f0;}
    const float f21=g(m,0x8dc8);
    auto matrix=init::Sub_0025AC68();
    float cosine=port::driving::Sub_0025A5E0(m,f22+f21);
    float sine=port::driving::Sub_0025A5E0(m,f22);
    matrix[0]=cosine;matrix[1]=sine;matrix[4]=-sine;matrix[5]=cosine;m.matrix(s+0x10,matrix);
    matrix=init::Sub_0025AC68();
    cosine=port::driving::Sub_0025A5E0(m,f23+f21);sine=port::driving::Sub_0025A5E0(m,f23);
    matrix[0]=cosine;matrix[1]=sine;matrix[4]=-sine;matrix[5]=cosine;m.matrix(s+0x50,matrix);
}
// ELF00139490; E-0038-E08.
void Sub_00139490(Memory& m,std::uint32_t s,float value) {
    const auto car=m.word(s+0x54);float f6=value;
    if(m.signed_byte(car+0x84c)!=0 && m.signed_byte(m.word(car+0x34)+0x12)!=0) f6=value*g(m,0x8454);
    m.scalar(m.word(s+0x40)+0x58,f6);m.scalar(m.word(s+0x18)+0x178,value);
    const auto helper=m.word(s+0x48);m.scalar(helper+8,value);m.scalar(helper+0xc,1.0f-m.scalar(car+0x458));
    for(unsigned i=0;i<4;++i)m.scalar(m.word(s+0x30+4*i)+0x17c,value);
}
// ELF001C08D0/001C0888; E-0038-E09.
float Sub_001C08D0(Memory& m,std::uint32_t s) {
    const auto angle=port::driving::Sub_0025A890(m,m.scalar(s+0x30),m.scalar(s+0x34));
    const auto delta=static_cast<std::uint16_t>(m.half(s+0x364)-static_cast<std::uint16_t>(angle));
    float result=static_cast<float>(delta)*g(m,0x8f58);
    if(g(m,0x8f5c)<result)result=result-g(m,0x8f60);
    return result;
}
float Sub_001C0888(Memory& m,std::uint32_t s){return m.scalar(s+0x380)<5.0f?0.0f:Sub_001C08D0(m,s);}
// ELF00135EB8; E-0038-E09.
float Sub_00135EB8(Memory& m,std::uint32_t car) {
    const auto start=m.word(m.word(car+0x34)+0x10c);
    if(start==0 || start==0x7fffffffu)return 0;
    const float elapsed=static_cast<float>(sb(m.word(ga(0xb388))-start))*g(m,0x83bc);
    if(!(elapsed<4.0f))return 0;
    return port::driving::Sub_0018F9F0(m,ga(0xa6c8),elapsed);
}
// ELF00135E18 includes exact00133260 car+870 accessor; E-0038-E09.
bool Sub_00135E18(Memory& m,std::uint32_t car) {
    if(m.word(m.word(car+0x14)+8)==3)return false;
    return m.word(car+0x870)==0 && m.signed_byte(car+0x4a8)!=0;
}
// ELF0025A3D0; E-0038-E11. DIVU contradicts decompiler signed remainder.
std::uint32_t Sub_0025A3D0(Memory& m,std::uint32_t state,std::uint32_t divisor) {
    if(!divisor)hp2::not_implemented("0x0025A3E0","Original divide-by-zero BREAK7");
    const auto old=m.word(state);const auto v3=old^0x1d872b41u;
    const auto a0=v3^(v3>>5);m.word(state,a0^v3^(a0<<27));return old%divisor;
}
// ELF0025A410/0025A4A8; E-0038-E11. Input multiplication precedes integer scaling.
float Sub_0025A410(Memory& m,std::uint32_t state,float value) {
    const auto result=Sub_0025A3D0(m,state,0x7fffffffu);
    const float f0=value*0x1p-31f;
    float f1;
    if(sb(result)<0){f1=static_cast<float>((result>>1)|(result&1));f1=f1+f1;}
    else f1=static_cast<float>(sb(result));
    return f0*f1;
}
float Sub_0025A4A8(Memory& m,float value){return Sub_0025A410(m,ga(0xc778),value);}
// ELF00137D90; E-0038-E10. Normal branch only; instrumentation via explicit services.
float Move(Memory& m,std::uint32_t s,float dt,DrivingServices& hooks) {
    const auto car=m.word(s+0x54);
    if(m.word(car+0x20)==1)hp2::not_implemented("0x00137FE4","Excluded original car+20==1 mover branch");
    const auto mode_address=m.word(ga(0xa900));
    if(m.signed_word(mode_address)==4 && m.word(ga(0xa758))!=0)hp2::not_implemented("0x001389BC","Excluded original Race4 diagnostic position-restore branch");
    const auto start=invoke(hooks.Sub_00250F38,"0x00250F38");
    Sub_00139490(m,s,m.scalar(car+0x3e0));Sub_001375F0(m,s);
    const auto controls=m.word(s),body=m.word(s+0x18),state=m.word(s+0x50),config=m.word(s+0x4c);
    const auto engine=m.word(s+0x40),drive=m.word(s+0x44);
    if(m.scalar(controls+0x218)<m.scalar(controls+0x214) && m.scalar(controls+0x21c)<m.scalar(controls+0x214)) {
        if(m.scalar(controls+0x218)<0.5f)m.word(controls+0x218,0);
        if(m.scalar(controls+0x21c)<0.5f)m.word(controls+0x21c,0);
    }else if(m.scalar(controls+0x214)<0.5f)m.word(controls+0x214,0);
    const auto steering=m.word(s+0x1c),table=m.word(steering+8);
    if(m.word(table+0x14)!=0x001a5d60)hp2::not_implemented("0x00137E9C","Unrecovered steering virtual target");
    Sub_001A5D60(m,steering+static_cast<std::int32_t>(m.signed_half(table+0x10)),m.half(controls+0x210));
    m.word(engine+0x44,m.word(controls+0x214));
    Sub_0018BCA0(m,drive,m.signed_word(controls+0x224));
    if(m.word(controls+0x228)!=0){m.word(engine+0x50,1);m.scalar(engine+0x54,static_cast<float>(m.signed_word(ga(0xb388)))*g(m,0x83f4));}
    Sub_0018ABF0(m,engine);m.word(controls+0x228,0);
    m.word(controls+0x220,static_cast<std::uint32_t>(SetDesiredGear(m,drive,m.signed_word(controls+0x220))));
    m.word(controls+0x224,m.word(drive+0x48));
    const bool active=g(m,0x83f8)<m.scalar(controls+0x214) ||
        (m.word(state+0x37c)==4 && g(m,0x83fc)<std::fabs(m.scalar(state+0x58)));
    m.word(body+0x110,active?0:1);
    float f24=m.scalar(s+0x70);if(m.word(state+0x39c)==0)f24=f24*g(m,0xa724);
    for(unsigned i=0;i<4;++i)m.word(state+0x2fc-4*i,0);
    const float f25=m.scalar(state+0x380)*g(m,0x8400);
    float f23=m.scalar(s+0x74);
    if(0.5f<=m.scalar(controls+0x21c)){f23=f23-dt*20.0f;if(f23<0.5f)f23=0.5f;}
    else {f23=f23+dt;if(1.0f<f23)f23=1.0f;}
    m.scalar(s+0x74,f23);
    for(unsigned i=0;i<4;++i) {
        const auto wheel=m.word(s+0x30+4*i);
        if(m.signed_word(mode_address)==4 || (m.word(s+0x60)!=0 && i<2)) {
            float f0;
            if(g(m,0x8404)<m.scalar(controls+0x214) && 0.5f<m.scalar(controls+0x218) && f25<10.0f && i>=2) f0=f25*g(m,0x8408);
            else f0=m.scalar(controls+0x218)*m.scalar(0x2f0500+4*i);
            m.scalar(wheel+0x170,f0);
        }else m.word(wheel+0x170,0);
        float f20=m.scalar(controls+0x21c)*m.scalar(0x2f0510+4*i);
        bool add;
        if(m.word(state+0x39c)==0)add=g(m,0x840c)<f20;
        else add=g(m,0x8410)<f20 && g(m,0x8414)<std::fabs(Sub_001C0888(m,state)) && f25<80.0f;
        if(add)f20=f20+0.5f;
        m.scalar(wheel+0x174,f20);
        if(i==2) {
            float a1=m.scalar(body+0x28);
            const float factor=port::driving::Sub_00137C88(Sub_001C0888(m,state),f20,m.scalar(state+0x384),
                m.scalar(config+0x310),m.scalar(config+0x314),Sub_00135EB8(m,car),m.signed_word(state+0x39c),a1);
            float f0=factor-1.0f;f0=f23*f0;f0=f0+1.0f;f24=f24*f0;
        }
        m.scalar(wheel+0x178,f24);
        const auto selector=m.word(state+0x3a4);
        if(((i==0 || i==3)&&selector==1) || ((i==1 || i==2)&&selector==2))m.scalar(wheel+0x178,f24*m.scalar(state+0x3ac));
        m.scalar(wheel+0x16c,((m.signed_byte(car+0x84c)>>i)&1)?1.0f:0.0f);
    }
    if(m.scalar(controls+0x460)!=0 && m.signed_byte(car+0x84c)==0)Sub_00139490(m,s,m.scalar(controls+0x460));
    const auto helper=m.word(s+0x48);m.word(helper+0x10,m.word(controls+0x214));
    m.scalar(helper+0x14,static_cast<float>(m.signed_word(state+0x37c))*0.25f);
    if(m.word(state+0x39c)==0)m.word(body+0x170,m.word(config+0x318));
    else {
        float f20=m.scalar(config+0x31c);
        if(f20==0){const float angle=Sub_001C0888(m,state);if(angle*m.scalar(body+0x78)<0 && g(m,0x8418)<std::fabs(angle))f20=20;}
        m.scalar(body+0x170,f20);
    }
    float scale=1;
    if(m.word(car+0x848)!=0)scale=1.0f-invoke(hooks.Sub_001B50C8,"0x001B50C8",m,m.word(car+0x848)+8)*g(m,0x841c);
    if(m.word(engine+0x50)!=0)m.word(body+0x174,0);
    else if(m.signed_byte(car+0x84c)!=0)m.scalar(body+0x174,60);
    else m.scalar(body+0x174,m.scalar(config+0x320)*scale);
    const auto steps=sb(static_cast<std::uint32_t>(port::driving::cvt_w_s(dt/g(m,0xa728)))+1);
    const float subdt=dt/static_cast<float>(steps);
    for(std::int32_t i=0;i<steps;++i) {
        if(m.word(ga(0xa758))!=0 || m.signed_word(mode_address)!=4)m.copy(0x2f0520,body+0x10,0x50);
        m.scalar(ga(0xb408),subdt);
        const auto begin=invoke(hooks.Sub_00250F38,"0x00250F38");
        invoke(hooks.Sub_0019C290,"0x0019C290",m,s+8,subdt);
        invoke(hooks.Sub_0011BD90,"0x0011BD90",m,body+0x20);
        const auto end=invoke(hooks.Sub_00250F38,"0x00250F38");invoke(hooks.Sub_00250F48,"0x00250F48",begin,end);
        if(m.word(ga(0xa6fc))!=0)invoke(hooks.Sub_0019D980,"0x0019D980",m,body);
        if(m.word(ga(0xa700))!=0)invoke(hooks.Sub_0018C030,"0x0018C030",m,drive);
        const auto mode=m.signed_word(mode_address);
        if(mode==4) {
            // ELF00138B9C..00138BD4; E-0038-E13. Race state4 is the ordinary
            // running branch when the separate A758 diagnostic flag is clear.
            if(m.word(ga(0xa758))!=0)hp2::not_implemented("0x001389BC","Excluded original Race4 diagnostic position-restore branch");
            m.scalar(ga(0xa784),static_cast<float>(m.signed_word(ga(0xb388)))*g(m,0x8448));
            m.scalar(ga(0xa79c),1);m.word(ga(0xa788),0);m.scalar(ga(0xa794),1);
            m.scalar(ga(0xa798),1);m.word(ga(0xa790),0);m.word(ga(0xa78c),0);
        } else if(mode>=2) {
            const float tickscale=g(m,0x8420);
            if(static_cast<float>(m.signed_word(s+0x58))*tickscale<static_cast<float>(sb(m.word(ga(0xb388))-m.word(s+0x5c)))*tickscale)m.word(s+0x60,1);
            if(0.5f<m.scalar(controls+0x214)) {
                if(m.word(s+0x64)!=0 && m.word(s+0x68)!=0) {
                    float f0=Sub_0025A4A8(m,g(m,0x8424));f0=f0+g(m,0x8428);f0=f0*4000;f0=f0+0.5f;
                    m.word(s+0x60,0);m.word(s+0x64,0);m.word(s+0x58,static_cast<std::uint32_t>(port::driving::cvt_w_s(f0)));
                    m.word(s+0x68,m.word(s+0x68)-1);
                    f0=static_cast<float>(m.signed_word(ga(0xb388)))*tickscale;f0=f0*4000;f0=f0+0.5f;
                    m.word(s+0x5c,static_cast<std::uint32_t>(port::driving::cvt_w_s(f0)));
                }
            }else{m.word(s+0x60,1);m.word(s+0x64,1);}
        }
        if(mode!=4 && m.word(s+0x60)!=0) {
            if(!hooks.Field_001389A4_stack_w)hp2::not_implemented("0x001389A4","Explicit original temporary W projection is absent");
            for(unsigned j=0;j<3;++j)m.word(body+0x60+4*j,0);
            m.word(body+0x6c,*hooks.Field_001389A4_stack_w);
        }
    }
    if(m.word(ga(0xa758))!=0)for(auto off:{0xa788,0xa78c,0xa790})if(0<g(m,off))m.scalar(ga(off),g(m,off)-g(m,0xb398));
    invoke(hooks.Sub_00139188,"0x00139188",m,s,dt);
    if(Sub_00135E18(m,car))for(unsigned i=0;i<4;++i)invoke(hooks.Sub_001A5520,"0x001A5520",m,m.word(s+0x30+4*i),car,dt);
    const auto final=invoke(hooks.Sub_00250F38,"0x00250F38");invoke(hooks.Sub_00250F48,"0x00250F48",start,final);
    return dt;
}
}
