// TASK-0041 E-0041-R03; retail 0x0019D4D0: RigidBody::GetDerivative(float *).
// Functions here use host projection signatures; original member scope/ABI is recorded separately.
#include "Sub_0019D4D0.hpp"
#include "Sub_00138F38.hpp"
#include "Sub_0018F808.hpp"
#include "port/not_implemented.hpp"
#include <cmath>
#include <stdexcept>
namespace port::driving::native {
namespace {
float g(Memory& m,std::uint16_t offset) { return m.scalar(0x003358f0u+static_cast<std::int16_t>(offset)); }
float length(const Vector& a) { const float x=a[0]*a[0],y=a[1]*a[1],z=a[2]*a[2];return std::sqrt((x+y)+z); }
float dot(const Vector& a,const Vector& b) {const float x=a[0]*b[0],y=a[1]*b[1],z=a[2]*b[2];return (x+y)+z;}
Vector scale(Vector a,float f) {for(auto& v:a)v=v*f;return a;}
void add(Memory& m,std::uint32_t address,const Vector& a) {auto b=m.vector(address);for(unsigned i=0;i<4;++i)b[i]=b[i]+a[i];m.vector(address,b);}
std::uint32_t count(Memory& m,std::uint32_t object) {
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
Matrix Sub_0025AE18(const Matrix& a,const Matrix& b) {
    Matrix result;
    for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r) {
        float v=a[r]*b[c*4];const float y=a[4+r]*b[c*4+1];v=v+y;
        const float z=a[8+r]*b[c*4+2];v=v+z;const float w=a[12+r]*b[c*4+3];result[c*4+r]=v+w;
    } return result;
}
Matrix transpose(const Matrix& a) {Matrix b;port::driving::Sub_0025AF20(b,a);return b;}
Vector transform(Memory& m,const Matrix& a,const Vector& b) {Vector c;port::driving::Sub_0025AED0(c,a,b,{m.vector(0x00320a60)});return c;}
void Sub_0019CD90(Memory& m,std::uint32_t body,const Vector& force,const Vector& point) {
    SolverBodyProjection p;p.Field_0xf0=m.vector(body+0xf0);p.Field_0x100=m.vector(body+0x100);
    port::driving::Sub_0019CD90(p,force,point);m.vector(body+0xf0,p.Field_0xf0);m.vector(body+0x100,p.Field_0x100);
}
void Sub_0019CE80(Memory& m,std::uint32_t body,const Vector& force,const Vector& point) {
    const auto a=m.matrix(body+0x20);Sub_0019CD90(m,body,transform(m,a,force),transform(m,a,point));
}
Matrix Sub_0019D370(const Vector& a,const Matrix& b) {
    Matrix r{};
    for(unsigned c=0;c<3;++c) {
        float f0=-a[2]*b[c*4+1],f1=a[1]*b[c*4+2];r[c*4]=f0+f1;
        f0=a[2]*b[c*4];f1=a[0]*b[c*4+2];r[c*4+1]=f0-f1;
        f0=-a[1]*b[c*4];f1=a[0]*b[c*4+1];r[c*4+2]=f0+f1;
    }r[15]=1;return r;
}
void Sub_0019CC88(Memory& m,std::uint32_t body,std::uint32_t position,std::uint32_t matrix,std::uint32_t velocity,std::uint32_t angular) {
    m.vector(body+0x10,m.vector(position));m.matrix(body+0x20,m.matrix(matrix));
    m.vector(body+0x60,scale(m.vector(velocity),m.scalar(body+0x118)));
    auto a=init::Sub_0025AC68();a[0]=1.0f/m.scalar(body+0x120);a[5]=1.0f/m.scalar(body+0x134);a[10]=1.0f/m.scalar(body+0x148);
    const auto b=m.matrix(body+0x20);const auto c=Sub_0025AE18(Sub_0025AE18(b,a),transpose(b));
    m.vector(body+0x70,transform(m,c,m.vector(angular)));
}
void Sub_0019D848(Memory& m,std::uint32_t body,float unused_stack_w) {
    m.vector(body+0x90,scale(m.vector(body+0x60),m.scalar(body+0x11c)));
    const auto b=m.matrix(body+0x20);const auto c=Sub_0025AE18(Sub_0025AE18(b,m.matrix(body+0x120)),transpose(b));
    m.vector(body+0xa0,transform(m,c,m.vector(body+0x70)));
    if(m.scalar(body+0x80)<0)m.scalar(body+0x80,0);
    m.matrix(body+0xb0,b);m.vector(body+0xe0,m.vector(body+0x10));m.scalar(body+0xec,1);
    m.vector(body+0xf0,{{0,0,0,unused_stack_w}});m.vector(body+0x100,{{0,0,0,unused_stack_w}});
    const Vector force{{0,0,-g(m,0xb238)*m.scalar(body+0x118),0}},point{{0,0,0,0}};
    Sub_0019CD90(m,body,force,point);
}
void GetDerivative(Memory& m,std::uint32_t body,std::uint32_t output) {
    if(m.scalar(body+0x174)!=0) {
        float f3=m.scalar(body+0x174)*m.scalar(body+0x178);f3=f3+g(m,0x8c68);
        if(m.word(body+0x114)!=0)f3=f3*g(m,0xb270);
        float f20=m.scalar(body+0x28),f1;
        if(0<f20) {if(g(m,0xb24c)<f20)f20=g(m,0xb24c);float f0=(1.0f-g(m,0xb248))*f20;f0=f0/g(m,0xb24c);f1=1.0f-f0;}
        else {f20=-f20;if(g(m,0xb244)<f20)f20=g(m,0xb244);float f0=(g(m,0xb240)-1.0f)*f20;f0=f0/g(m,0xb244);f1=f0+1.0f;}
        f3=f3*f1;
        const auto velocity=m.vector(body+0x90);const float x=velocity[0]*velocity[0],y=velocity[1]*velocity[1];
        if(f3<std::sqrt(x+y)*g(m,0x8c6c) && 0<dot(m.vector(body+0xf0),m.vector(body+0x20))) {
            auto local=transform(m,transpose(m.matrix(body+0x20)),m.vector(body+0xf0));
            f20=-f20*g(m,0xb254);if(g(m,0xb250)<f20)f20=g(m,0xb250);local[0]=f20;
            m.vector(body+0xf0,transform(m,m.matrix(body+0x20),local));
        }
    }
    m.vector(output,m.vector(body+0x90));m.matrix(output+0x10,Sub_0019D370(m.vector(body+0xa0),m.matrix(body+0x20)));
    m.vector(output+0x50,m.vector(body+0xf0));m.vector(output+0x60,m.vector(body+0x100));
    const float angular_length=length(m.vector(body+0xa0));
    const float f=port::driving::Sub_0018F838(m,0x00310df8,angular_length);
    add(m,output+0x60,scale(m.vector(body+0x70),-f));
    float f12=length(m.vector(body+0x90))/g(m,0xb258);const float f20=angular_length/g(m,0xb25c);if(f12<f20)f12=f20;
    const float limit=port::driving::Sub_0018F838(m,0x00310e30,f12);
    m.scalar(output+0x70,m.scalar(body+0x80)<limit ? g(m,0xb268) : -g(m,0xb26c));
    if(m.word(body+0x110)!=0) {
        add(m,output+0x60,scale(m.vector(body+0x70),-g(m,0xb264)*m.scalar(body+0x80)));
        add(m,output+0x50,scale(m.vector(body+0x60),-g(m,0xb260)*m.scalar(body+0x80)));
    }
    const float f1=m.scalar(body+0x170),f2=m.scalar(body+0x108);
    if(f1!=0) {if(f1<f2)m.scalar(output+0x68,f1);else if(f2< -f1)m.scalar(output+0x68,-f1);}
}
void Sub_0019C670(Memory& m,std::uint32_t head,std::uint32_t output,const IntegrationCalls& calls) {
    if(!calls.derivative)hp2::not_implemented("0x0019C670","component derivative dispatch");
    std::uint32_t i=0;
    for(auto node=m.word(head);node!=head;node=m.word(node)) {
        calls.derivative(m,node,output+i*4);i+=count(m,node);
    }
}
void Sub_0019C290(Memory& m,float dt,std::uint32_t head,const IntegrationCalls& calls) {
    // Original bounded stack is140 floats. Allocate a reusable port scratch region.
    std::uint32_t total=0;
    for(auto node=m.word(head);node!=head;node=m.word(node)) {
        const auto n=count(m,node);
        if(n>140-total)throw std::out_of_range("Original derivative stack capacity exceeded");
        total+=n;
    }
    Memory::Scratch frame(m);const auto scratch=m.allocate(140*4);Sub_0019C670(m,head,scratch,calls);std::uint32_t i=0;
    for(auto node=m.word(head);node!=head;node=m.word(node)) {
        const auto n=count(m,node);if(n>140-i)throw std::out_of_range("Original derivative stack capacity exceeded");
        const auto table=m.word(node+8),target=m.word(table+0x1c);
        const auto op=m.word(target),delay=m.word(target+4);
        if(op!=0x03e00008u || (delay&0xffff0000u)!=0x24820000u)
            hp2::not_implemented("0x0019C290","component state pointer virtual target");
        const auto state=node+static_cast<std::int16_t>(m.half(table+0x18))+static_cast<std::int16_t>(delay&0xffffu);
        for(std::uint32_t j=0;j<n;++j){const float product=dt*m.scalar(scratch+(i+j)*4);m.scalar(state+j*4,m.scalar(state+j*4)+product);}i+=n;
    }
    if(!calls.after)hp2::not_implemented("0x0019C290","post integration dispatch");
    calls.after(m,head);
}
}
