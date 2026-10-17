
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001A5FB0; explicit host partition; original TU unconfirmed
namespace port::driving::init {
void Sub_001A5FB0(ByteView out){out.require(0x14,8);out.word(0x18,0);out.word(0x14,0);}
}

// Retail 0x001A5FC0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A5FC0(Memory& m,std::uint32_t object,ContactProvider& contact){
    using namespace detail_group_00137100_runtime;
    Memory::Scratch scratch(m);
    const auto body=m.word(object+0x94),wheel=m.word(object+0x90),config=m.word(object+0xc);
    auto world=transform(m,m.matrix(body+0x20),m.vector(object+0x80));
    const auto position=m.vector(body+0x10);for(unsigned i=0;i<4;++i)world[i]=world[i]+position[i];
    const auto temporary=m.allocate(16);m.vector(temporary,world);
    const float hit=port::driving::Sub_001A4580(m,wheel,temporary,body+0x20,contact);
    const float value=hit+m.scalar(m.word(wheel+0xc)+0xc),old=m.scalar(object+0x14);
    float extra=0;
    if(m.scalar(config+0x14)<value){extra=value-m.scalar(config+0x14);m.scalar(object+0x14,m.scalar(config+0x14));m.word(object+0x70,1);if(global(m,0xb414)<extra)extra=0;}
    else if(value<m.scalar(config+0x18)){m.scalar(object+0x14,m.scalar(config+0x18));m.word(object+0x70,0);}
    else {m.scalar(object+0x14,value);m.word(object+0x70,1);}
    const float dt=global(m,0xb408);
    if(global(m,0x8dcc)<dt)m.scalar(object+0x18,(m.scalar(object+0x14)-old)/dt);
    m.copy(object+0x20,object+0x80,16);m.scalar(object+0x28,m.scalar(object+0x28)+m.scalar(object+0x14));
    auto matrix=init::bIdentity();const float steer=m.scalar(wheel+0x16c);
    if(steer!=0){auto rotation=init::bIdentity();const float a=(global(m,0xb41c)*steer)*port::driving::bSin(m,m.scalar(wheel+0x1c));
        const float c=port::driving::bSin(m,a+global(m,0x8dd0)),s=port::driving::bSin(m,a);
        rotation[0]=c;rotation[1]=s;rotation[4]=-s;rotation[5]=c;matrix=bMulMatrix(matrix,rotation);}
    const auto local=m.vector(object+0x20);for(unsigned i=0;i<3;++i)matrix[12+i]=local[i];matrix[15]=1;
    m.matrix(object+0x30,matrix);
    if(m.word(object+0x98)){const auto other=m.word(object+0x98);matrix=bMulMatrix(matrix,m.matrix(m.word(other+4)+m.word(object+0x9c)*0x40));}
    m.matrix(wheel+0xb0,matrix);
    const auto relative=transform(m,m.matrix(body+0x20),m.vector(object+0x20));
    const auto velocity=GetPointVelocity(m.vector(body+0x90),m.vector(body+0xa0),relative);m.vector(wheel+0xf0,velocity);
    m.matrix(wheel+0x70,bMulMatrix(m.matrix(body+0xb0),matrix));
    if(m.word(object+0x70)){
        const float displacement=m.scalar(object+0x14),rate=m.scalar(object+0x18);
        float response=std::max(displacement,0.0f)*m.scalar(config);response=response+1;response=m.scalar(config+4)*response;
        response=displacement*response;response=m.scalar(object+0x78)+response;
        response=response+rate*m.scalar(config+8+(rate<=0?4:0));m.scalar(object+0x1c,response);
        response=response+m.scalar(config+0x10)*(displacement-m.scalar(m.word(object+0x10)+0x14));
        response=response+extra*global(m,0xb410);m.scalar(object+0x1c,response);
        if(!(response<0))return;
    }
    m.word(object+0x1c,0);
}
}

// Retail 0x001A5D60; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A5D60(Memory& m,std::uint32_t s,std::uint16_t value) {
    using namespace detail_group_00137d90;
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
    auto matrix=init::bIdentity();
    float cosine=port::driving::bSin(m,f22+f21);
    float sine=port::driving::bSin(m,f22);
    matrix[0]=cosine;matrix[1]=sine;matrix[4]=-sine;matrix[5]=cosine;m.matrix(s+0x10,matrix);
    matrix=init::bIdentity();
    cosine=port::driving::bSin(m,f23+f21);sine=port::driving::bSin(m,f23);
    matrix[0]=cosine;matrix[1]=sine;matrix[4]=-sine;matrix[5]=cosine;m.matrix(s+0x50,matrix);
}
}

// Retail 0x00153458; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_00153458(Memory& m,std::int32_t x,std::int32_t y) {
    const auto a=static_cast<std::uint32_t>(x),b=static_cast<std::uint32_t>(y);
    const auto index=(a*b*11u+(a+1u)*(b+1u)*13u)&15u;
    return m.scalar(0x002f94c8+index*4);
}
}

// Retail 0x00153498; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_00153498(Memory& m,std::uint32_t point,std::int32_t material) {
    if(material<0||material>=32)throw std::out_of_range("Material outside bounded road projection");
    const auto row=0x00318c00u+static_cast<std::uint32_t>(material)*0x90u;
    float x=m.scalar(point)*m.scalar(row+0x18),y=m.scalar(point+4)*m.scalar(row+0x18);
    float fx=static_cast<float>(cvt_w_s(x)),fy=static_cast<float>(cvt_w_s(y));
    if(x<fx)fx=fx-1.0f;
    if(y<fy)fy=fy-1.0f;
    const auto ix=cvt_w_s(fx),iy=cvt_w_s(fy);
    y=y-fy;x=x-fx;const float inverse=1.0f-x;
    float value=inverse*(1.0f-y);value=value*Sub_00153458(m,ix,iy);
    float term=x*(1.0f-y);term=term*Sub_00153458(m,ix+1,iy);value=value+term;
    term=inverse*y;term=term*Sub_00153458(m,ix,iy+1);value=value+term;
    term=x*y;term=term*Sub_00153458(m,ix+1,iy+1);value=value+term;
    return value*m.scalar(row+0x1c);
}
}

// Retail 0x0018AC38; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0018AC38(Memory& m,std::uint32_t config,std::uint32_t engineconfig,
                  std::uint32_t out50,std::uint32_t out74,float f26) {
    using namespace detail_group_0018f808;
    const float lower=m.scalar(engineconfig),upper=m.scalar(engineconfig+8);
    const float step=8.0f/(upper-lower);
    const auto evaluate=[&](float value) {
        return interpolate(value,lower,upper,step,9,[&](std::int32_t i) {
            if(i<0 || i>=9) throw std::out_of_range("Engine table generation index outside9 samples");
            return m.scalar(engineconfig+0x10+static_cast<std::uint32_t>(i)*4);
        });
    };
    const auto limit=signed_bits(m.word(config+0x18)+1);
    std::int32_t current=2;
    // Memory bounds reject corrupt indexes; no fabricated default or changed cap.
    while(current<limit) {
        const auto off=static_cast<std::uint32_t>(current)*4;
        const float f24=m.scalar(config+0x20+off), f23=m.scalar(config+0x24+off);
        float f0;
        if(m.word(0x3308b4)==0) {
            float f1=m.scalar(engineconfig+4)-lower;
            f0=m.scalar(config)*f1;f0=f0+lower;
        } else {
            const float f22=upper-200.0f;
            float f21=(lower+upper)*0.5f;
            bool pending=true;
            while(f21<f22 && pending) {
                const float f20=evaluate(f21);
                float value=f21*f23;value=value/f24;
                f0=evaluate(value);f0=f0*f23;f0=f0/f24;
                if(f20<f0) { m.scalar(out50+off,f21);pending=false; }
                const float next=f21+50.0f;
                if(next==f21) hp2::not_implemented("0x0018AD74","non-progressing exceptional table scan");
                f21=next;
            }
            if(pending) m.scalar(out50+off,upper-200.0f);
            f0=m.scalar(out50+off)*f26;
        }
        m.scalar(out50+off,f0);
        f0=m.scalar(0x3308b0)*m.scalar(out50+off);f0=f0*f23;f0=f0/f24;
        m.scalar(out74+off+4,f0);
        ++current;
    }
    m.scalar(out50+static_cast<std::uint32_t>(current)*4,upper);
}
}

// Retail 0x0018A988; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0018A988(Memory& m,std::uint32_t s) {
    using namespace detail_group_0018f808;
    const auto config=m.word(s+0x10);const float scale=m.scalar(0x32e328);
    float value=m.scalar(s+0xc);float bound=m.scalar(config)*scale;
    if(value<bound) { m.scalar(s+0xc,bound);value=bound; }
    bound=m.scalar(config+8)*scale;
    if(bound<value) m.scalar(s+0xc,bound);
    m.word(s+0x48,0);
}
}

// Retail 0x0018B008; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_0018B008(Memory& m,std::uint32_t s) {
    using namespace detail_group_0018f808;
    const auto config=m.word(s+0x10);const float f4=m.scalar(config+0x10);
    float f3=0;
    if(m.scalar(0x32e33c)<=f4) {
        const auto a=m.word(s+0x20),b=m.word(s+0x24);
        float f0=m.scalar(a+0x20)+m.scalar(b+0x20);f0=f0*0.5f;f3=f0*m.scalar(config+0xc);
    }
    if(f4<=m.scalar(0x32e340)) {
        const auto a=m.word(s+0x18),b=m.word(s+0x1c);
        float f0=m.scalar(a+0x20)+m.scalar(b+0x20);f0=f0*0.5f;f0=f0*m.scalar(config+8);f3=f3+f0;
        if(m.scalar(0x32e33c)<=f4) f3=f3*0.5f;
    }
    return f3*ratio(m,s);
}
}

// Retail 0x0018B0D8; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_0018B0D8(Memory& m,std::uint32_t s) {
    using namespace detail_group_0018f808;
    const float f4=m.scalar(m.word(s+0x10)+0x10);float f3=0;
    if(m.scalar(0x32e344)<=f4) {
        f3=m.scalar(m.word(s+0x20)+0x164)+m.scalar(m.word(s+0x24)+0x164);f3=f3*0.5f;
    }
    if(f4<=m.scalar(0x32e348)) {
        float f0=m.scalar(m.word(s+0x18)+0x164)+m.scalar(m.word(s+0x1c)+0x164);
        f0=f0*0.5f;f0=f3+f0;f3=f0*0.5f;
    }
    return f3;
}
}

// Retail 0x0018B178; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0018B178(Memory& m,std::uint32_t s,float value) {
    using namespace detail_group_0018f808;
    const float f23=value-Sub_0018B008(m,s);
    const float f22=m.scalar(m.word(s+0x28)+0x384)*m.scalar(0x32e34c);
    auto config=m.word(s+0x10);
    const auto axle=[&](std::uint32_t off,float config_lane) {
        const auto a=m.word(s+off),b=m.word(s+off+4);
        bool average=f22<40.0f;
        if(m.signed_byte(a+0x155)==0 && m.signed_byte(b+0x155)==0) average=true;
        float a20=m.scalar(a+0x20),b20=m.scalar(b+0x20);
        const float denominator=ratio(m,s)*config_lane;
        const float f21=f23/denominator;
        if(average) {b20=(a20+b20)*0.5f;a20=b20;}
        m.scalar(a+0x20,a20+f21);m.scalar(b+0x20,b20+f21);
    };
    if(m.scalar(0x32e350)<=m.scalar(config+0x10)) {
        axle(0x20,m.scalar(config+0xc));config=m.word(s+0x10);
    }
    if(m.scalar(config+0x10)<=m.scalar(0x32e354)) axle(0x18,m.scalar(config+8));
}
}

// Retail 0x0018BC18; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018BC18(Memory& m,std::uint32_t s) {
    using namespace detail_group_0018f808;
    const auto engine=m.word(s+0x14);
    if(!(m.scalar(engine+0x44)<m.scalar(0x32e37c))) return 0;
    float f0=m.scalar(m.word(engine+0x10))*m.scalar(0x32e380);f0=f0+m.scalar(0x32e384);
    return m.scalar(s+0x40)<=f0 ? 1:0;
}
}

// Retail 0x0018BC68; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018BC68(Memory& m,std::uint32_t s) {
    using namespace detail_group_0018f808;
    float f0=m.scalar(m.word(s+0x14)+0xc)-m.scalar(s+0x40);f0=std::fabs(f0);
    return f0<=m.scalar(0x3308c0) ? 1:0;
}
}

// Retail 0x0018B4A0; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0018B4A0(Memory& m,std::uint32_t s) {
    using namespace detail_group_0018f808;
    m.scalar(s+0x40,Sub_0018B008(m,s));
    const auto state=m.signed_word(s+0x38);const auto engine=m.word(s+0x14);
    if(state==1) {
        const auto pending=m.word(s+0x3c);m.word(s+0x30,m.word(s+0x2c));m.word(engine+0x4c,0);
        if(pending!=0 || m.scalar(0x32e36c)<m.scalar(engine+0x44)) {
            m.word(s+0x3c,0);m.word(s+0x38,2);
        }
    } else if(state==0) {
        m.word(engine+0x4c,0);const float f1=m.scalar(engine+0x4c);
        if(m.scalar(s+0xc)<=f1) {m.word(s+0x38,1);m.scalar(s+0xc,f1);}
    } else if(state==2) {
        m.word(engine+0x4c,m.word(0x32e368));
        if(1.0f<=m.scalar(s+0xc)) {
            m.scalar(s+0xc,1);
            if(Sub_0018BC68(m,s)!=0 || m.word(s+0x30)==1) m.word(s+0x38,3);
        }
        if(Sub_0018BC18(m,s)!=0) m.word(s+0x38,0);
    } else if(state==3) {
        m.scalar(engine+0x4c,1);auto current=m.word(s+0x30);
        if(current!=1) {
            const float f20=m.scalar(s+0x40)-m.scalar(engine+0xc);
            const float f2=Sub_0018B0D8(m,s);
            if(m.scalar(0x32e360)<m.scalar(engine+0x44) && 0.0f<f2) {
                const float f0=m.scalar(engine+0xc);m.scalar(s+0x40,f0);Sub_0018B178(m,s,f0);
            } else {
                float f0=f20*m.scalar(0x32e364);f0=m.scalar(engine+0xc)+f0;m.scalar(engine+0xc,f0);
            }
            if(Sub_0018BC18(m,s)!=0) m.word(s+0x38,0);
            current=m.word(s+0x30);
        }
        if(current!=m.word(s+0x2c)) {m.word(s+0x38,0);m.word(s+0x3c,1);}
    }
}
}

// Retail 0x0018B698; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018B698(Memory& m,std::uint32_t s,std::uint32_t out) {
    using namespace detail_group_0018f808;
    const auto at=0x310050u+m.word(s+0x38)*4;
    const float value=m.scalar(m.word(s+0x10)+0x1c)*m.scalar(at);
    m.scalar(out,value);return 0;
}
}

// Retail 0x0018C368; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018C368(){
    using namespace detail_group_0018f808;return 1;}
}

// Retail 0x0018C1C8; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_0018C1C8(){
    using namespace detail_group_0018f808;return 1;}
}

// Retail 0x0019CC88; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_0019CC88(Memory& m,std::uint32_t body,std::uint32_t position,std::uint32_t matrix,std::uint32_t velocity,std::uint32_t angular) {
    using namespace detail_group_0019d4d0;
    m.vector(body+0x10,m.vector(position));m.matrix(body+0x20,m.matrix(matrix));
    m.vector(body+0x60,scale(m.vector(velocity),m.scalar(body+0x118)));
    auto a=init::bIdentity();a[0]=1.0f/m.scalar(body+0x120);a[5]=1.0f/m.scalar(body+0x134);a[10]=1.0f/m.scalar(body+0x148);
    const auto b=m.matrix(body+0x20);const auto c=bMulMatrix(bMulMatrix(b,a),transpose(b));
    m.vector(body+0x70,transform(m,c,m.vector(angular)));
}
}

// Retail 0x0019D848; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_0019D848(Memory& m,std::uint32_t body,float unused_stack_w) {
    using namespace detail_group_0019d4d0;
    m.vector(body+0x90,scale(m.vector(body+0x60),m.scalar(body+0x11c)));
    const auto b=m.matrix(body+0x20);const auto c=bMulMatrix(bMulMatrix(b,m.matrix(body+0x120)),transpose(b));
    m.vector(body+0xa0,transform(m,c,m.vector(body+0x70)));
    if(m.scalar(body+0x80)<0)m.scalar(body+0x80,0);
    m.matrix(body+0xb0,b);m.vector(body+0xe0,m.vector(body+0x10));m.scalar(body+0xec,1);
    m.vector(body+0xf0,{{0,0,0,unused_stack_w}});m.vector(body+0x100,{{0,0,0,unused_stack_w}});
    const Vector force{{0,0,-g(m,0xb238)*m.scalar(body+0x118),0}},point{{0,0,0,0}};
    ApplyForce(m,body,force,point);
}
}

// Retail 0x0019DA30; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_0019DA30(Memory& m){
    using namespace detail_group_0019da30;
    for(auto a:{0x00310dc0u,0x00310dc8u,0x00310dd0u,0x00310dd8u,0x00310dc4u,0x00310dd4u})m.word(a,0);
    port::driving::Sub_0018F808(m,0x00310df8,0x00310de0,6,0,10);
    port::driving::Sub_0018F808(m,0x00310e30,0x00310e10,8,0,1);
}
}

// Retail 0x001395B0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001395B0(Memory& m){
    using namespace detail_group_0019da30;
    m.word(0x002f0570,0);m.word(0x002f0574,0);m.word(0x002f0578,m.word(address(0x8458)));
    m.word(0x002f057c,0);m.scalar(0x002f0584,20);m.scalar(0x002f0580,2);
    m.word(address(0xa730),m.word(address(0x845c)));
    // ELF0018F9E0: sw a1,0(a0); jr ra; sw a2,4(a0).
    m.word(address(0xa770),0x002f0570);m.word(address(0xa770)+4,3);
}
}

// Retail 0x001A33A8; explicit host partition; original TU unconfirmed
namespace port::driving::native {
float Sub_001A33A8(Memory& m,std::uint32_t value){
    using namespace detail_group_0019da30;return static_cast<float>(signed_bits(value))*g(m,0x8d04);}
}

// Retail 0x00136DD0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
std::uint32_t Sub_00136DD0(float f12){
    using namespace detail_group_0019da30;f12=f12*4000.0f;f12=f12+0.5f;return static_cast<std::uint32_t>(port::driving::cvt_w_s(f12));}
}

// Retail 0x001A33F0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A33F0(Memory& m,float f12,ScheduleServices& services){
    using namespace detail_group_0019da30;
    const auto index=m.word(address(0xb384))&15u;std::uint32_t value;
    if(m.word(address(0xaf44))){
        if(!services.Sub_00183358)hp2::not_implemented("0x00183358","Original frame replay read service is absent");
        value=services.Sub_00183358(8,2);services.Sub_00183358(4,4);
    }else{
        float accumulator=f12*g(m,0x8d08);accumulator=accumulator+g(m,0xb380);
        if(m.word(address(0xb364))==0)accumulator=std::min(g(m,0x8d10),std::max(accumulator,g(m,0x8d0c)));
        else accumulator=std::min(g(m,0x8d18),std::max(accumulator,g(m,0x8d14)));
        value=GetQuantizedRealTimeElapsed(accumulator);
        const auto integer=Sub_00136DD0(Sub_001A33A8(m,value));
        m.scalar(address(0xb380),accumulator-static_cast<float>(signed_bits(integer))*g(m,0x8d1c));
        if(!services.Sub_00183498)hp2::not_implemented("0x00183498","Original frame observation service is absent");
        services.Sub_00183498(value,8,2);services.Sub_00183498(index,4,4);
    }
    m.word(address(0xb378),value);m.scalar(address(0xb374),Sub_001A33A8(m,value));
}
}

// Retail 0x001A3500; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A3500(Memory& m){
    using namespace detail_group_0019da30;
    const auto dt=g(m,0xb374);const auto sum=g(m,0xb37c)+dt;
    const auto integer=port::driving::cvt_w_s(sum*60.0f);
    const auto next=m.word(address(0xb370))+static_cast<std::uint32_t>(integer?integer:1);
    m.word(address(0xb36c),next);m.word(address(0xb370),next);
    m.scalar(address(0xb37c),sum-static_cast<float>(integer)*g(m,0x8d20));
    m.word(address(0xb384),m.word(address(0xb384))+1);
    m.word(address(0xb368),m.word(address(0xb368))+Sub_00136DD0(dt));
}
}

// Retail 0x001A3598; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A3598(Memory& m){
    using namespace detail_group_0019da30;
    const auto integer=port::driving::cvt_w_s(g(m,0x8d24));
    m.word(address(0xb394),1);m.word(address(0xb388),static_cast<std::uint32_t>(integer));m.word(address(0xb390),1);
    m.word(address(0xb398),0);m.word(address(0xb39c),0);m.word(address(0xb3a0),0);
    m.scalar(address(0xb38c),static_cast<float>(integer)*g(m,0x8d28));
}
}

// Retail 0x001A35E0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A35E0(Memory& m,float f12){
    using namespace detail_group_0019da30;m.scalar(address(0xb398),f12);m.word(address(0xb3a4),0);}
}

// Retail 0x001A35F0; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void Sub_001A35F0(Memory& m){
    using namespace detail_group_0019da30;
    m.word(address(0xb3a4),1);const auto dt=g(m,0xb398);if(dt==0)return;
    const auto sum=g(m,0xb39c)+dt;const auto integer=port::driving::cvt_w_s(sum*60.0f);
    const auto next=m.word(address(0xb394))+static_cast<std::uint32_t>(integer);
    m.word(address(0xb390),next);m.word(address(0xb394),next);
    m.scalar(address(0xb39c),sum-static_cast<float>(integer)*g(m,0x8d2c));
    const auto ticks=m.word(address(0xb388))+Sub_00136DD0(dt);
    m.word(address(0xb398),0);m.word(address(0xb388),ticks);m.word(address(0xb3a0),m.word(address(0xb3a0))+1);
    m.scalar(address(0xb38c),static_cast<float>(signed_bits(ticks))*g(m,0x8d30));
}
}

// Retail 0x0011B6E0; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_0011B6E0(Memory&m,float f12){
    using namespace detail_group_001a4930;
    const float upper=g(m,-0x7ec4),step=g(m,-0x7ec0);
    if(!(0<upper) || !(0<step) || !std::isfinite(f12))
        hp2::not_implemented("0x0011B6E0","unsupported polynomial range reduction");
    while(upper<f12)f12=f12-upper;
    while(f12<0)f12=f12+step;
    const float bound=g(m,-0x7ebc);float sign=1;
    if(bound<=f12){f12=f12-bound;sign=-1;}
    if(g(m,-0x7eb8)<=f12)f12=bound-f12;
    float f1=f12*f12;float f0=f12*f1;float f2=f0*f1;
    f0=f0*g(m,-0x7eb4);float f3=f2*f1;f0=f12+f0;
    f2=f2*g(m,-0x7eb0);f1=f3*f1;f3=f3*g(m,-0x7eac);
    f0=f0+f2;f1=f1*g(m,-0x7ea8);f0=f0+f3;f0=f0+f1;return f0*sign;
}
}

// Retail 0x0011B7D8; explicit host partition; original TU unconfirmed
namespace port::driving {
SolverMatrix Sub_0011B7D8(Memory&m,std::uint16_t angle){
    using namespace detail_group_001a4930;
    const float radians=static_cast<float>(angle)*g(m,-0x7ea4);
    const float cosine=Sub_0011B6E0(m,radians+g(m,-0x7ea0));
    const float sine=Sub_0011B6E0(m,radians);
    return {{cosine,sine,0,0,-sine,cosine,0,0,0,0,1,0,0,0,0,1}};
}
}

// Retail 0x0011B950; explicit host partition; original TU unconfirmed
namespace port::driving {
SolverMatrix Sub_0011B950(Memory&m,std::uint16_t angle){
    using namespace detail_group_001a4930;
    const float radians=static_cast<float>(angle)*g(m,-0x7e94);
    const float cosine=Sub_0011B6E0(m,radians+g(m,-0x7e90));
    const float sine=Sub_0011B6E0(m,radians);
    return {{cosine,0,-sine,0,0,1,0,0,sine,0,cosine,0,0,0,0,1}};
}
}

// Retail 0x001A45A8; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_001A45A8(Memory&m,std::uint32_t wheel,std::uint32_t normal_out,
                  std::uint32_t material_out,std::uint32_t point,std::uint32_t body,ContactProvider&provider){
    using namespace detail_group_001a4930;
    const float height=provider.GetElevation(m,m.word(m.word(wheel+0x18)+4),point,material_out,normal_out,0);
    const float roughness=Sub_00153498(m,point,m.signed_word(material_out));m.scalar(wheel+0x15c,roughness);
    const auto normal=m.vector(normal_out);const float denominator=dot(normal,m.vector(body+0x20));
    if(denominator<g(m,-0x72a8)){m.word(material_out,0);return -100.0f;}
    // Query XY are copied unchanged into the temporary, so XYZ difference is(0,0,Z).
    SolverVector delta{{0,0,(height+roughness)-m.scalar(point+8),0}};
    return dot(normal,delta)/denominator;
}
}

// Retail 0x001A4580; explicit host partition; original TU unconfirmed
namespace port::driving {
float Sub_001A4580(Memory&m,std::uint32_t wheel,std::uint32_t point,std::uint32_t body,ContactProvider&provider){
    using namespace detail_group_001a4930;
    return Sub_001A45A8(m,wheel,wheel+0x140,wheel+0x150,point,body,provider);
}
}

// Retail 0x001A63C0; explicit host partition; original TU unconfirmed
namespace port::driving {
std::uint32_t Sub_001A63C0(Memory&m,std::uint32_t,std::uint32_t out){
    using namespace detail_group_001a4930;m.word(out,0);m.word(out+4,0);return out;}
}

// Retail 0x001A57E0; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_001A57E0(Memory&m){
    using namespace detail_group_001a4930;
    const float upper=g(m,-0x7248);m.scalar(gp-0x4c38,upper);
    Sub_0018F808(m,0x00310f48,0x00310f28,7,0,upper);
    for(unsigned i=0;i<7;++i)Sub_0018F808(m,0x00311008+i*0x18,0x00310f60+i*0x18,6,0,10);
}
}

// Retail 0x0025AFA8; explicit host partition; original TU unconfirmed
namespace port::driving {
void Sub_0025AFA8(Memory&m){
    using namespace detail_group_001a4930;
    m.scalar(0x00320a60,0);m.scalar(0x00320a6c,1);
    m.scalar(0x00320a68,0);m.scalar(0x00320a64,0);
}
}
