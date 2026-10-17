// TASK-0041 E-0041-V01; retail 0x00138E08: PhysicsCar::ResolveForces(void).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#include "Sub_00138E08.hpp"
#include "Sub_0018BCE0_memory.hpp"
#include "Sub_0018F808.hpp"
#include "Sub_001A4930.hpp"
#include "Sub_00138F38.hpp"
#include "port/not_implemented.hpp"
#include <cmath>
namespace port::driving::native {
namespace {
float g(Memory& m,std::uint16_t off){return m.scalar(0x003358f0u+static_cast<std::int16_t>(off));}
float len(const Vector& a){const float x=a[0]*a[0],y=a[1]*a[1],z=a[2]*a[2];return std::sqrt((x+y)+z);}
float dot(const Vector& a,const Vector& b){const float x=a[0]*b[0],y=a[1]*b[1],z=a[2]*b[2];return (x+y)+z;}
Vector mul(Vector a,float f){for(auto& v:a)v=v*f;return a;}
Vector cross(const Vector& a,const Vector& b){return {{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0],a[3]}};}
SolverWheelProjection wheel(Memory& m,std::uint32_t a){
    SolverWheelProjection w;w.config_Field_0xc=m.scalar(m.word(a+0xc)+0xc);w.Field_0x24=m.scalar(a+0x24);
    w.Field_0xb0=m.scalar(a+0xb0);w.Field_0xb4=m.scalar(a+0xb4);w.Field_0xb8=m.scalar(a+0xb8);
    w.Field_0xc0=m.scalar(a+0xc0);w.Field_0xc4=m.scalar(a+0xc4);w.Field_0xc8=m.scalar(a+0xc8);
    w.Field_0x108=m.scalar(a+0x108);w.Field_0x114=m.scalar(a+0x114);w.Field_0x118=m.scalar(a+0x118);
    w.Field_0x80=m.vector(a+0x80);w.Field_0x140=m.vector(a+0x140);w.Field_0xa0=m.vector(a+0xa0);
    w.Field_0x130=m.vector(a+0x130);return w;
}
void suspension_request(Memory& m,std::uint32_t a,float request){
    const auto target=m.word(a+0x90);auto w=wheel(m,target);const auto flag=m.signed_word(a+0x70);
    float f=m.scalar(a+0x1c);if(flag!=0)f=f+request;
    const auto result=port::driving::Sub_001A44A8(f,w,flag);
    m.byte(target+0x155,w.Field_0x155);m.vector(target+0x130,w.Field_0x130);m.scalar(target+0x158,w.Field_0x158);m.scalar(a+0xa0,result);
}
}
// ELF0011BD90, E-0038-B09. Original Gram-Schmidt order and affine fourth lanes.
void Sub_0011BD90(Memory& m,std::uint32_t a){
    auto x=m.vector(a),y=m.vector(a+0x10);x=mul(x,1.0f/len(x));auto z=cross(x,y);z=mul(z,1.0f/len(z));y=cross(z,x);
    x[3]=y[3]=z[3]=0;m.vector(a,x);m.vector(a+0x10,y);m.vector(a+0x20,z);m.vector(a+0x30,{{0,0,0,1}});
}
// ELF001ADD70, E-0038-B10. Optional output reports two recovered components' sum.
void Sub_001ADD70(Memory& m,std::uint32_t a,std::uint32_t output){
    const auto body=m.word(a+4),config=m.word(a);const auto velocity=m.vector(body+0x90);
    const float speed=len(velocity)/m.scalar(a+8);float value=speed*m.scalar(a+0xc);value=value*m.scalar(config);
    float extra=value*(g(m,0xb568)-1.0f);extra=extra*(1.0f-m.scalar(a+0x10));value=-(value+extra);
    Vector point{{0,0,0,0}};if(0.5f<=m.scalar(a+0x14))point[2]=g(m,0xb56c)*(1.0f-m.scalar(a+0x10));
    Sub_0019CD90(m,body,mul(velocity,value),point);
    float f=m.scalar(body+0x48);if(f<0)f=0;if(0.5f<=m.scalar(a+0x14))f=1;
    Vector direction{{1,0,0,0}};if(g(m,0x8e2c)<speed)direction=mul(m.vector(body+0x90),1.0f/speed);
    float projection=dot(direction,m.vector(body+0x20));if(projection<0)projection=0;
    projection=projection*projection;projection=projection*projection;
    float front,rear;
    if(m.scalar(a+0x14)==0){projection=projection*f;const float sum=m.scalar(config+4)+m.scalar(config+8);front=speed*g(m,0x8e30);front=front*projection;front=front*sum;rear=speed*g(m,0x8e34);rear=rear*projection;rear=rear*sum;}
    else{projection=projection*f;front=speed*m.scalar(config+4);front=front*projection;rear=speed*m.scalar(config+8);rear=rear*projection;}
    Sub_0019CE80(m,body,{{0,0,-front,0}},m.vector(0x003115b0));Sub_0019CE80(m,body,{{0,0,-rear,0}},m.vector(0x003115c0));
    if(output)m.scalar(output,front+rear);
}
// ELF00139518 E-0038-B11. Original angular damping curve and car state gate.
void Sub_00139518(Memory& m,std::uint32_t mover){
    if(m.signed_word(m.word(mover+0x54)+0x3bc)>=2)return;
    const auto body=m.word(mover+0x18);const auto v=m.vector(body+0xa0);const float mass=m.scalar(body+0x118);
    const float f=port::driving::Sub_0018F9F0(m,0x00330060,len(v));
    const auto damping=mul(v,-(mass*f));auto old=m.vector(body+0x100);for(unsigned i=0;i<4;++i)old[i]=old[i]+damping[i];m.vector(body+0x100,old);
}
// ELF00138F38 E-0038-S10 native state binding, exact five iterations.
void Sub_00138F38(Memory& m,std::uint32_t mover){
    const auto drive=m.word(mover+0x44);Sub_0018B948(m,drive);
    const auto ai=m.signed_byte(m.word(m.word(mover+0x54)+0x34)+0x12);
    for(unsigned i=0;i<4;++i)port::driving::Sub_001A4930(m,m.word(mover+0x30+4*i),0,ai);
    const auto body=m.word(mover+0x18);const auto base=transform(m,transpose(m.matrix(body+0x20)),m.vector(body+0xf0));
    for(unsigned iteration=0;iteration<5;++iteration){
        auto sum=base;float moment=0;
        for(unsigned i=0;i<4;++i){const auto a=m.word(mover+0x30+4*i),s=m.word(mover+0x20+4*i);Vector value{};port::driving::Sub_001A5428(wheel(m,a),value);
            for(unsigned j=0;j<4;++j)sum[j]=sum[j]+value[j];
            const float y=m.scalar(s+0x80)*value[1],x=m.scalar(s+0x84)*value[0];moment=moment+y;moment=moment-x;
        }
        const float x=sum[0]/m.scalar(body+0x118),y=sum[1]/m.scalar(body+0x118);const float divisor=1.0f/m.scalar(body+0x148);moment=moment/divisor;
        for(unsigned i=0;i<4;++i){const auto a=m.word(mover+0x30+4*i),s=m.word(mover+0x20+4*i);if(m.byte(a+0x154)==0&&m.word(a+0x10c)==0){auto w=wheel(m,a);const float f1=moment*m.scalar(s+0x80),f2=moment*m.scalar(s+0x84);port::driving::ModifyGroundForce(w,{{x-f2,y+f1}});m.scalar(a+0x114,w.Field_0x114);}}
        Sub_0018BAA0(m,drive);
    }
    for(std::int32_t i=0;i<4;++i)port::driving::Sub_001A4EE0(m,m.word(mover+0x30+4*i),i);
}
// ELF00138E08 E-0038-B12. Profiling counter writes are port-side diagnostics only.
void ResolveForces(Memory& m,std::uint32_t mover){
    Memory::Scratch frame(m);const auto scratch=m.allocate(16);const auto body=m.word(mover+0x18);
    const auto table=m.word(body+8);if(m.word(table+0x34)!=0x0019d978)hp2::not_implemented("0x00138E08","unknown body pre-force virtual entry");
    m.word(0x00330058,m.word(0x00330058)+1u);Sub_001ADD70(m,m.word(mover+0x48),scratch);
    const float request=m.word(0x00330054)==0 ? 0 : m.scalar(scratch)*0.25f;
    for(unsigned i=0;i<4;++i)suspension_request(m,m.word(mover+0x20+4*i),request);
    Sub_00138F38(m,mover);
    for(unsigned i=0;i<4;++i){const auto a=m.word(mover+0x20+4*i);if(m.word(a+0x70)!=0){const auto w=m.word(a+0x90),b=m.word(a+0x94);const auto p=m.vector(w+0x130),q=m.vector(b+0x10);Vector delta;for(unsigned j=0;j<4;++j)delta[j]=p[j]-q[j];if(m.word(0x00330d10)==0)Sub_0019CD90(m,b,m.vector(w+0x120),delta);}}
    Sub_00139518(m,mover);
}
void integrate(Memory& m,std::uint32_t list,float dt,InitServices& services){
    IntegrationCalls calls;
    calls.derivative=[](Memory& state,std::uint32_t object,std::uint32_t out){
        const auto table=state.word(object+8),target=state.word(table+0x3c);const auto adjusted=object+static_cast<std::int32_t>(state.signed_half(table+0x38));
        switch(target){
        case 0x0019d4d0:GetDerivative(state,adjusted,out);return;
        case 0x001a46b0:port::driving::Sub_001A46B0(state,adjusted,out);return;
        case 0x001a63c0:port::driving::Sub_001A63C0(state,adjusted,out);return;
        case 0x0018a9d8:port::driving::Sub_0018A9D8(state,adjusted,out);return;
        case 0x0018b698:port::driving::Sub_0018B698(state,adjusted,out);return;
        default:hp2::not_implemented("0x0019C670","untranslated derivative target");}
    };
    calls.after=[&](Memory& state,std::uint32_t head){const auto table=state.word(head+0xc),target=state.word(table+0x14);if(target!=0x00137788)hp2::not_implemented("0x0019C290","unknown post-integrate target");Sub_00137788(state,head+static_cast<std::int32_t>(state.signed_half(table+0x10)),services);};
    Sub_0019C290(m,dt,list,calls);
}
// ELF00139188 and001C0F40/CF8/D48/EF0; E-0038-B13.
void Sub_00139188(Memory& m,std::uint32_t mover,float){
    const auto body=m.word(mover+0x18),car=m.word(mover+0x50),config=m.word(car);
    m.vector(car+0x20,m.vector(body+0x10));m.matrix(car+0x50,m.matrix(body+0x20));
    const auto center=transform(m,m.matrix(car+0x50),m.vector(config+0x110));auto position=m.vector(car+0x20);for(unsigned i=0;i<4;++i)position[i]=position[i]-center[i];m.vector(car+0x10,position);
    m.vector(car+0x30,m.vector(body+0x90));m.scalar(car+0x384,len(m.vector(body+0x90)));m.word(car+0x400,0);m.vector(car+0x90,m.vector(body+0xa0));
    m.half(car+0x364,static_cast<std::uint16_t>(port::driving::Sub_0025AB50(m,cvt_w_s(m.scalar(car+0x50)*65536.0f),cvt_w_s(m.scalar(car+0x54)*65536.0f))));
    m.half(car+0x366,m.half(m.word(mover)+0x210));m.scalar(car+0x380,dot(m.vector(car+0x30),m.vector(car+0x50)));
    for(unsigned i=0;i<4;++i){const auto w=m.word(mover+0x30+4*i),s=m.word(mover+0x20+4*i),cfg=m.word(s+0xc);
        m.word(car+0x2b0+i*4,m.word(w+0x20));m.word(car+0x2c0+i*4,m.word(w+0x1c));
        const float limit=m.scalar(cfg+0x1c),value=m.scalar(s+0x14);m.scalar(car+0x300+i*4,value+(limit<=value?limit-value:0.0f));
        const auto old=m.byte(car+0x310+i),now=m.byte(w+0x155);m.byte(car+0x310+i,now);if(old==0&&now!=0)m.word(car+0x37c,m.word(car+0x37c)+1u);else if(old!=0&&now==0)m.word(car+0x37c,m.word(car+0x37c)-1u);
        const auto material=m.word(w+0x150);m.word(car+0x314+i*4,material);if(m.signed_word(car+0x324+i*4)<static_cast<std::int32_t>(material))m.word(car+0x324+i*4,material);
        m.word(car+0x354+i*4,m.word(w+0x168));m.word(car+0x334+i*8,m.word(w+0x100));m.word(car+0x338+i*8,m.word(w+0x104));m.word(car+0x2d0+i*4,m.word(w+0x158));m.word(car+0x2e0+i*4,now?m.word(w+0x15c):0);
    }
    const auto engine=m.word(mover+0x40),drive=m.word(mover+0x44);m.scalar(car+0x38c,m.scalar(engine+0x44)*m.scalar(engine+0x4c));m.word(car+0x388,m.word(drive+0x30));m.word(car+0x398,m.word(drive+0x38)==3);m.scalar(car+0x390,m.scalar(engine+0xc)*g(m,0x8450));
    const auto w0=m.word(mover+0x30),w1=m.word(mover+0x34),w2=m.word(mover+0x38),w3=m.word(mover+0x3c);
    float a=std::abs(m.scalar(w0+0x160)),b=std::abs(m.scalar(w1+0x160));const auto c=std::abs(m.scalar(w2+0x160)),d=std::abs(m.scalar(w3+0x160));if(a<d)a=d;if(b<c)b=c;a=a/g(m,0xa76c);b=b/g(m,0xa76c);if(1<a)a=1;if(1<b)b=1;m.scalar(car+0x374,a);m.scalar(car+0x378,b);
    const float sum=((m.scalar(w0+0x168)+m.scalar(w1+0x168))+m.scalar(w2+0x168))+m.scalar(w3+0x168);m.scalar(car+0x370,sum*0.25f);
    for(std::int32_t i=0;i<4;++i)port::driving::Sub_001C0D78(m,car,i);
}
}
