#include "Sub_001A4930.hpp"
#include "Sub_0018F808.hpp"
#include "port/not_implemented.hpp"
#include <cmath>
#include <cstring>

namespace port::driving {
namespace {
constexpr std::uint32_t gp=0x003358f0;
float g(Memory& m,std::int32_t off){return m.scalar(gp+static_cast<std::uint32_t>(off));}
std::uint32_t gw(Memory& m,std::int32_t off){return m.word(gp+static_cast<std::uint32_t>(off));}
SolverMathContext context(Memory& m){return {m.vector(0x00320a60)};}
SolverVector transform(Memory& m,const SolverMatrix& mat,const SolverVector& v){
    SolverVector out{};Sub_0025AED0(out,mat,v,context(m));return out;
}
SolverVector local(Memory& m,std::uint32_t a){
    SolverMatrix mat{};Sub_0025AF20(mat,m.matrix(a+0x70));
    mat[3]=0;mat[7]=0;mat[11]=0;
    return transform(m,mat,m.vector(a+0xf0));
}
float dot(const SolverVector&a,const SolverVector&b){
    const float x=a[0]*b[0],y=a[1]*b[1],z=a[2]*b[2];
    const float xy=x+y;return xy+z;
}
SolverVector cross(const SolverVector&a,const SolverVector&b,float w){
    const float x=a[1]*b[2],xx=a[2]*b[1];
    const float y=a[2]*b[0],yy=a[0]*b[2];
    const float z=a[0]*b[1],zz=a[1]*b[0];return {{x-xx,y-yy,z-zz,w}};
}
void threshold(Memory&m,float f,std::uint32_t a){
    SolverWheelProjection w;w.Field_0x170=m.scalar(a+0x170);w.Field_0x174=m.scalar(a+0x174);
    w.Field_0x20=m.scalar(a+0x20);const auto c=m.word(a+0x14);
    w.config_0x14_Field_0x4=m.scalar(c+4);w.config_0x14_Field_0x8=m.scalar(c+8);
    w.config_Field_0xc=m.scalar(m.word(a+0xc)+0xc);
    Sub_001A4778(f,w,{g(m,-0x4c30),g(m,-0x4c2c),g(m,-0x4c28),g(m,-0x4c24)});
    m.word(a+0x10c,w.Field_0x10c);m.scalar(a+0x20,w.Field_0x20);
}
float angle(Memory&m,float x,float y,std::int32_t off){
    float result=static_cast<float>(Sub_0025A890(m,x,y))*g(m,off);
    const float bound=g(m,off+4);
    if(bound<result)result=result-g(m,off+8);
    if(x<0){result=result+bound;if(bound<result)result=result-g(m,off+12);}
    return result;
}
}
// ELF0025A540..0025A5DF; E-0038-T01. Literal scalar instruction grouping.
float Sub_0025A540(Memory&m,std::uint16_t a){
    float f8=static_cast<float>(a)*g(m,-0x673c),sign=1;
    if(g(m,-0x6738)<=f8)f8=f8-g(m,-0x6734);
    else if(g(m,-0x6730)<=f8){sign=-sign;f8=f8-g(m,-0x672c);}
    float f1=f8*f8;float f0=f8*f1;float f2=f0*f1;
    f0=f0*g(m,-0x6728);float f3=f2*f1;f0=f8-f0;
    f2=f2*g(m,-0x6724);f1=f3*f1;f3=f3*g(m,-0x6720);
    f0=f0+f2;f1=f1*g(m,-0x671c);f0=f0-f3;f0=f0+f1;return f0*sign;
}
// ELF0025A5E0..0025A60B; E-0038-T02.
float Sub_0025A5E0(Memory&m,float f){return Sub_0025A540(m,static_cast<std::uint16_t>(cvt_w_s(f*g(m,-0x6718))));}
// ELF0025A748..0025A88B; E-0038-T03. Original piecewise table0031FDC0.
std::uint16_t Sub_0025A748(Memory&m,float f){
    const bool negative=f<0;if(negative)f=-f;
    if(1<=f)return negative?0xc000:0x4000;
    std::int32_t a4=0x8000,a5=0x8000,a6=0;
    std::int32_t v3=cvt_w_s(f*65536.0f);
    if(v3>=a4){a4>>=1;for(;;){a5+=a4;++a6;if(v3<a5)break;
        if(a6>=11){a5-=a4;goto selected;}a4>>=1;}}
    a5-=a4;
selected:
    v3=(v3-a5)>>((11-a6)&31);a4>>=4;
    const auto reconstructed=v3*a4+a5;
    const auto entry=0x0031fdc0u+static_cast<std::uint32_t>((a6*16+v3)*8);
    float f0=static_cast<float>(reconstructed)*0.0000152587890625f;
    f0=f-f0;f0=f0*m.scalar(entry+4);f0=f0*65536.0f;
    const auto value=static_cast<std::uint16_t>(m.half(entry)+cvt_w_s(f0));
    return negative?static_cast<std::uint16_t>(0u-value):value;
}
// ELF0025A890..0025A9D3; E-0038-T04.
std::int16_t Sub_0025A890(Memory&m,float x,float y){
    unsigned quadrant=x<0?1u:0u;if(y<0)quadrant^=3;
    const float yy=y*y,xx=x*x,sum=yy+xx;float ratio=0;std::uint16_t value;
    if(std::abs(y)<std::abs(x)){
        if(g(m,-0x66e0)<sum && y!=0)ratio=std::abs(y)/std::sqrt(sum);
        value=Sub_0025A748(m,ratio);
    }else{
        if(g(m,-0x66dc)<sum && x!=0)ratio=std::abs(x)/std::sqrt(sum);
        value=static_cast<std::uint16_t>(0x4000u-Sub_0025A748(m,ratio));
    }
    if(quadrant==3)value=static_cast<std::uint16_t>(0u-value);
    else if(quadrant==1)value=static_cast<std::uint16_t>(0x8000u-value);
    else if(quadrant==2)value=static_cast<std::uint16_t>(value+0x8000u);
    std::int16_t result;std::memcpy(&result,&value,2);return result;
}
// ELF001A4818..001A492F; E-0038-T05. Finite software-double arithmetic route.
float Sub_001A4818(Memory&m,std::uint32_t a){
    if(m.word(a+0x10c)!=0 || m.scalar(a+0x20)<0 || m.signed_word(m.word(a+0xc)+0x14)<2)return 1;
    float f=(m.scalar(m.word(a+0x18)+0x380)-g(m,-0x7298))*g(m,-0x7294);
    if(1<f)f=1;else if(f<0)f=0;
    const double first=static_cast<double>(g(m,-0x4c1c));
    const double second=1.0-static_cast<double>(g(m,-0x4c1c));
    const double product=second*static_cast<double>(f);
    return static_cast<float>(first+product);
}
// ELF001A4D28..001A4E87; E-0038-T06.
float Sub_001A4D28(Memory&m,float request,float argument,std::uint32_t a){
    if(gw(m,-0x4c10)==0){
        const float lookup=Sub_0018F838(m,0x00310f48,argument);
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
        float first=Sub_0018F838(m,m.word(p),request);
        float second=Sub_0018F838(m,m.word(p+4),request);
        second=second-first;second=fraction*second;value=first+second;
    }else value=Sub_0018F838(m,m.word(0x003110c8),request);
    const float factor=m.scalar(c+8)*g(m,-0x4c0c);return factor*value;
}
// ELF001A4930..001A4D27; E-0038-T07.
void Sub_001A4930(Memory&m,std::uint32_t a,std::int32_t,std::int32_t a2){
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
        if(m.word(m.word(a+0x18)+0x39c)==1)scale=scale*Sub_001A4818(m,a);
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
        float result=Sub_001A4D28(m,g(m,-0x4c20)*m.scalar(a+0x158),std::abs(f20),a);
        if(0<f20)result=-result;
        m.scalar(a+0x118,sign*result);
        m.word(a+0x104,0);m.word(a+0x100,0);
    }
}
// ELF001A4EE0..001A539F; E-0038-T08.
void Sub_001A4EE0(Memory&m,std::uint32_t a,std::int32_t){
    bool scaled=false;
    if(0<m.scalar(a+0x108) && m.word(a+0x10c)==0){scaled=true;m.scalar(a+0x114,m.scalar(a+0x114)*g(m,-0x4c00));}
    float x=m.scalar(a+0x114),y=m.scalar(a+0x118);float f1=x*x,f0=y*y;f1=f1+f0;
    const float length=std::sqrt(f1);const auto c=m.word(a+0x10);
    f1=g(m,-0x4c20)*m.scalar(a+0x17c);
    f0=m.scalar(0x00318c20u+m.word(a+0x150)*0x90)*m.scalar(c+0xc);
    float f2=m.scalar(a+0x158)*m.scalar(a+0x178);f1=f1*f0;
    float limit=f1*f2;if(m.word(m.word(a+0x18)+0x39c)==1)limit=limit*Sub_001A4818(m,a);
    if(limit<length && g(m,-0x7270)<length){
        const float factor=limit/length;m.scalar(a+0x114,m.scalar(a+0x114)*factor);m.scalar(a+0x118,m.scalar(a+0x118)*factor);
    }else if(scaled)m.scalar(a+0x114,m.scalar(a+0x114)/g(m,-0x4c00));
    float ratio=1;if(g(m,-0x726c)<limit){ratio=length/limit;if(1<ratio)ratio=1;}
    if(m.signed_byte(a+0x154)!=0 || m.word(a+0x10c)!=0)ratio=1;
    if(m.signed_byte(a+0x155)==0)ratio=0;
    m.scalar(a+0x168,ratio);
    const auto v=local(m,a);float theta=angle(m,v[0],v[1],-0x7268);
    float f20=Sub_0025A5E0(m,theta)*m.scalar(a+0x118);
    // The sine-derived temporary is computed but only f20 affects stores.
    const float lateral=Sub_0025A5E0(m,theta+g(m,-0x7258))*m.scalar(a+0x118);(void)lateral;
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

// ELF0011B248..0011B2BF; E-0038-T09. All inputs load before any output store.
void Sub_0011B248(SolverMatrix&out,const SolverMatrix&a,const SolverMatrix&b){
    const auto first=a,second=b;SolverMatrix result{};
    for(unsigned row=0;row<4;++row)for(unsigned lane=0;lane<4;++lane){
        float acc=second[lane]*first[row*4];
        float product=second[4+lane]*first[row*4+1];acc=acc+product;
        product=second[8+lane]*first[row*4+2];acc=acc+product;
        product=second[12+lane]*first[row*4+3];result[row*4+lane]=acc+product;
    }
    out=result;
}
// ELF0011B6E0..0011B7D3; E-0038-T10. Original polynomial coefficients.
float Sub_0011B6E0(Memory&m,float f12){
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
namespace {
// ELF0011B7D8..0011B883; E-0038-T11, port matrix value carrier.
SolverMatrix Sub_0011B7D8(Memory&m,std::uint16_t angle){
    const float radians=static_cast<float>(angle)*g(m,-0x7ea4);
    const float cosine=Sub_0011B6E0(m,radians+g(m,-0x7ea0));
    const float sine=Sub_0011B6E0(m,radians);
    return {{cosine,sine,0,0,-sine,cosine,0,0,0,0,1,0,0,0,0,1}};
}
// ELF0011B950..0011BA17; E-0038-T11, rotation matrix supplied to0011B248.
SolverMatrix Sub_0011B950(Memory&m,std::uint16_t angle){
    const float radians=static_cast<float>(angle)*g(m,-0x7e94);
    const float cosine=Sub_0011B6E0(m,radians+g(m,-0x7e90));
    const float sine=Sub_0011B6E0(m,radians);
    return {{cosine,0,-sine,0,0,1,0,0,sine,0,cosine,0,0,0,0,1}};
}
}
// ELF001C0D78..001C0EEF; E-0038-T11;0011B950/BA18/B7D8 are matrix dependencies.
void Sub_001C0D78(Memory&m,std::uint32_t car,std::int32_t index){
    const auto offset=static_cast<std::uint32_t>(index)*4;
    SolverMatrix first{};first[0]=first[5]=first[10]=first[15]=1;auto second=first;
    const auto phase=static_cast<std::uint16_t>(cvt_w_s(m.scalar(car+offset+0x2c0)*g(m,-0x709c)));
    const auto ry=Sub_0011B950(m,phase);Sub_0011B248(first,first,ry);
    if(index<2){const auto rz=Sub_0011B7D8(m,m.half(car+0x366));Sub_0011B248(first,first,rz);Sub_0011B248(second,second,rz);}
    const auto location=m.word(car)+static_cast<std::uint32_t>(index)*0x20+0x120;
    auto translation=m.vector(location);translation[3]=1;
    translation[2]=translation[2]+m.scalar(car+offset+0x300);
    translation[1]=translation[1]+m.scalar(location+0x18);
    for(unsigned i=0;i<4;++i){first[12+i]=translation[i];second[12+i]=translation[i];}
    m.matrix(car+static_cast<std::uint32_t>(index)*0x40+0xb0,first);
    m.matrix(car+static_cast<std::uint32_t>(index)*0x40+0x1b0,second);
}

// ELF001A4170..001A4347; E-0038-T12. Original discarded scratch W is not
// used by scalar physics; output+0x130 W remains deliberately unmodified.
void Sub_001A4170(Memory&m,float f12,float f13,float f14,std::uint32_t a,
                 std::uint32_t car,std::uint32_t,std::uint16_t steering,ContactProvider&provider){
    const auto index=m.signed_word(m.word(a+0xc)+0x14);
    m.scalar(a+0x20,f13);m.scalar(a+0x1c,f12);
    m.half(car+0x366,steering);const auto offset=static_cast<std::uint32_t>(index)*4;
    m.scalar(car+offset+0x2b0,f13);m.scalar(car+offset+0x2c0,f12);m.scalar(car+offset+0x300,f14);
    Sub_001C0D78(m,car,index);
    auto body=m.matrix(car+0x50);auto position=m.vector(car+0x20);position[3]=1;
    for(unsigned i=0;i<4;++i)body[12+i]=position[i];
    const auto local_matrix=m.matrix(car+static_cast<std::uint32_t>(index)*0x40+0xb0);
    SolverMatrix world{};Sub_0011B248(world,local_matrix,body);
    m.matrix(a+0xb0,local_matrix);m.matrix(a+0x70,world);
    SolverVector delta{},velocity{};for(unsigned i=0;i<4;++i)delta[i]=world[12+i]-m.scalar(car+0x20+i*4);
    // VOPMSUB.XYZ preserves the old delta W.
    velocity=cross(delta,m.vector(car+0x90),delta[3]);const auto linear=m.vector(car+0x30);
    for(unsigned i=0;i<4;++i)velocity[i]=velocity[i]+linear[i];
    m.vector(a+0xf0,velocity);
    Memory::Scratch scratch(m);const auto temporary=m.allocate(0x40);
    m.vector(temporary,{{world[12],world[13],world[14],world[15]}});
    const float height=provider.Sub_00152070(m,m.word(car+4),temporary,temporary+0x20,temporary+0x10,0);
    const auto slot=car+0x310+static_cast<std::uint32_t>(index);const auto old=m.signed_byte(slot);m.byte(slot,1);
    if(old==0)m.word(car+0x37c,m.word(car+0x37c)+1u);
    const auto material=m.signed_word(temporary+0x20);m.word(car+offset+0x314,static_cast<std::uint32_t>(material));
    if(m.signed_word(car+offset+0x324)<material)m.word(car+offset+0x324,static_cast<std::uint32_t>(material));
    m.scalar(a+0x130,world[12]);m.scalar(a+0x134,world[13]);m.scalar(a+0x138,height);
    m.vector(a+0x140,m.vector(temporary+0x10));m.word(a+0x150,static_cast<std::uint32_t>(material));
    m.byte(a+0x155,1);
}
// ELF001A45A8..001A46AF; E-0038-T13.00153498 reads only point XY,
// so the original stack copy with replaced Z is not needed by this projection.
float Sub_001A45A8(Memory&m,std::uint32_t wheel,std::uint32_t normal_out,
                  std::uint32_t material_out,std::uint32_t point,std::uint32_t body,ContactProvider&provider){
    const float height=provider.Sub_00152070(m,m.word(m.word(wheel+0x18)+4),point,material_out,normal_out,0);
    const float roughness=Sub_00153498(m,point,m.signed_word(material_out));m.scalar(wheel+0x15c,roughness);
    const auto normal=m.vector(normal_out);const float denominator=dot(normal,m.vector(body+0x20));
    if(denominator<g(m,-0x72a8)){m.word(material_out,0);return -100.0f;}
    // Query XY are copied unchanged into the temporary, so XYZ difference is(0,0,Z).
    SolverVector delta{{0,0,(height+roughness)-m.scalar(point+8),0}};
    return dot(normal,delta)/denominator;
}
// ELF001A4580..001A45A7; E-0038-T13.
float Sub_001A4580(Memory&m,std::uint32_t wheel,std::uint32_t point,std::uint32_t body,ContactProvider&provider){
    return Sub_001A45A8(m,wheel,wheel+0x140,wheel+0x150,point,body,provider);
}
namespace {
std::int32_t signed_word_bits(std::uint32_t bits){std::int32_t value;std::memcpy(&value,&bits,4);return value;}
std::int16_t signed_half_bits(std::uint16_t bits){std::int16_t value;std::memcpy(&value,&bits,2);return value;}
}
// ELF0025A300..0025A36B; E-0038-T14. Includes original zero-denominator literals.
std::int32_t Sub_0025A300(std::int32_t numerator,std::int32_t denominator){
    if(denominator==0){if(numerator==0)return 0;return numerator>0?0x7fffffff:signed_word_bits(0x80000000);}
    const auto quotient=0x7fffffff/denominator;
    const auto product=static_cast<std::int64_t>(numerator)*quotient;
    const auto shifted=static_cast<std::uint32_t>(static_cast<std::uint64_t>(product)>>16);
    return signed_word_bits(shifted<<1);
}
// ELF0025AA78..0025AB4B; E-0038-T14.
std::uint16_t Sub_0025AA78(Memory&m,std::int32_t value){
    const bool negative=value<0;if(negative)value=signed_word_bits(0u-static_cast<std::uint32_t>(value));
    std::uint32_t lower,upper,weight;
    if(value<0x200000){
        std::uint32_t pointer;
        if(value<0x40000){pointer=0x00320850u+static_cast<std::uint32_t>(value>>11)*2;weight=(static_cast<std::uint32_t>(value)<<5)&0xffff;}
        else{pointer=0x00320958u+static_cast<std::uint32_t>(value>>14)*2;weight=(static_cast<std::uint32_t>(value)<<2)&0xffff;}
        lower=m.half(pointer);upper=m.half(pointer+2);
    }else if(value<0x1000000){weight=static_cast<std::uint32_t>(value>>8);lower=0x3eba;upper=0x3fd7;}
    else{lower=upper=0x3fff;weight=0;}
    const auto product=signed_word_bits((upper-lower)*weight);
    const auto result=static_cast<std::uint16_t>(lower+static_cast<std::uint32_t>(product>>16));
    return negative?static_cast<std::uint16_t>(0u-result):result;
}
// ELF0025AB50..0025AC0F; E-0038-T14. Separate fixed-point quantization path.
std::int16_t Sub_0025AB50(Memory&m,std::int32_t x,std::int32_t y){
    unsigned quadrant=0;
    if(x<0){quadrant=1;x=signed_word_bits(0u-static_cast<std::uint32_t>(x));}
    if(y<0){quadrant^=3;y=signed_word_bits(0u-static_cast<std::uint32_t>(y));}
    std::uint16_t value=0x4000;
    if((y>>14)<x)value=Sub_0025AA78(m,Sub_0025A300(y,x));
    if(quadrant==1)value=static_cast<std::uint16_t>(0x8000u-value);
    else if(quadrant==2)value=static_cast<std::uint16_t>(value-0x8000u);
    else if(quadrant==3)value=static_cast<std::uint16_t>(0u-value);
    return signed_half_bits(value);
}
// ELF001A46B0..001A4773; E-0038-T15. Ghidra byte capture decoded
// read-only with Capstone5.0.7; R5900 three-register MULT independently audited.
std::uint32_t Sub_001A46B0(Memory&m,std::uint32_t a,std::uint32_t out){
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
// ELF001A63C0..001A63CF; E-0038-T15. Stores occur even when out aliases A0.
std::uint32_t Sub_001A63C0(Memory&m,std::uint32_t,std::uint32_t out){m.word(out,0);m.word(out+4,0);return out;}

// ELF001A57E0..001A5913; E-0038-T16. Initialize original static table headers.
void Sub_001A57E0(Memory&m){
    const float upper=g(m,-0x7248);m.scalar(gp-0x4c38,upper);
    Sub_0018F808(m,0x00310f48,0x00310f28,7,0,upper);
    for(unsigned i=0;i<7;++i)Sub_0018F808(m,0x00311008+i*0x18,0x00310f60+i*0x18,6,0,10);
}

// ELF0025AFA8..0025AFCF; E-0038-T17/S02. Exact four scalar stores.
void Sub_0025AFA8(Memory&m){
    m.scalar(0x00320a60,0);m.scalar(0x00320a6c,1);
    m.scalar(0x00320a68,0);m.scalar(0x00320a64,0);
}

}
