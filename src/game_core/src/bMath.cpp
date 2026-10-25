
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <cstring>
#include <cmath>

namespace port::frontend {
namespace {
float retail_math_bits(std::uint32_t bits){float result;std::memcpy(&result,&bits,4);return result;}
}
}
namespace hp2::frontend {
// Retail0025A5E0 ->0025A540, E-0094-D04. Constants0032F1B4..0032F1DB,
// independently read from Retail; no Debug addresses/behavior are transplanted.
// CVT.W.S uses the existing finite truncation projection; original FCR31/runtime
// equivalence remains unverified. Unsupported exceptional operands throw here.
float bSin(float radians) {
    using ::port::frontend::retail_math_bits;
    const float scaled=radians*retail_math_bits(0x4622f983);
    if(!std::isfinite(scaled) || static_cast<double>(scaled)<-2147483648.0 || static_cast<double>(scaled)>=2147483648.0)
        hp2::not_implemented("0x0025A5F0","Retail frontend sine CVT.W.S exceptional operand");
    const auto angle=static_cast<std::uint16_t>(static_cast<std::int32_t>(scaled));
    float f8=static_cast<float>(angle)*retail_math_bits(0x38c90fdb),sign=1;
    if(retail_math_bits(0x4096cbe4)<=f8)f8=f8-retail_math_bits(0x40c90fdb);
    else if(retail_math_bits(0x3fc90fdb)<=f8){sign=-sign;f8=f8-retail_math_bits(0x40490fdb);}
    float f1=f8*f8,f0=f8*f1,f2=f0*f1;
    f0=f0*retail_math_bits(0x3e2aaaab);
    float f3=f2*f1;f0=f8-f0;f2=f2*retail_math_bits(0x3c088889);
    f1=f3*f1;f3=f3*retail_math_bits(0x39500d01);f0=f0+f2;
    f1=f1*retail_math_bits(0x3638ef1d);f0=f0-f3;f0=f0+f1;
    return f0*sign;
}
}
#include "game_core/src/bMath.hpp"
namespace hp2::recovered {
using namespace port::driving;
using namespace port::driving::native;
using namespace port::driving::init;
namespace { float debug_math_scalar(Memory& m,std::uint32_t a){return m.scalar(a);} }
// Debug002539E0, E-0098-C01. Original unsigned16 quarter-turn addition.
float bCos(Memory& m,std::uint16_t angle){return hp2::recovered::bSin(m,static_cast<std::uint16_t>(angle+0x4000u));}
}

// Retail 0x0025A3D0; E-0041-M01; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x002537A0; E-0044-M01; bRandom(int, unsigned int *)
namespace hp2::recovered {
std::uint32_t bRandom(Memory& m,std::uint32_t state,std::uint32_t divisor) {
    using namespace port::driving::native::detail_group_00137d90;
    if(!divisor)hp2::not_implemented("0x002537AC","Original divide-by-zero BREAK7");
    const auto old=m.word(state);const auto v3=old^0x1d872b41u;
    const auto a0=v3^(v3>>5);m.word(state,a0^v3^(a0<<27));return old%divisor;
}
}

// Retail 0x0025A410; E-0042-C169; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x002537E0; E-0044-M01; bRandom(float, unsigned int *)
namespace hp2::recovered {
float bRandom(Memory& m,std::uint32_t state,float value) {
    using namespace port::driving::native::detail_group_00137d90;
    const auto result=hp2::recovered::bRandom(m,state,0x7fffffffu);
    const float f0=value*0x1p-31f;
    float f1;
    if(sb(result)<0){f1=static_cast<float>((result>>1)|(result&1));f1=f1+f1;}
    else f1=static_cast<float>(sb(result));
    return f0*f1;
}
}

// Retail 0x0025A4A8; E-0042-U190; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x00253878; E-0044-M01; bRandom(float)
namespace hp2::recovered {
float bRandom(Memory& m,float value) {
    using namespace port::driving::native::detail_group_00137d90;return hp2::recovered::bRandom(m,0x00329060u,value);}
}

// Retail 0x0025A540; E-0042-C170; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x00253910; E-0044-M01; bSin(unsigned short)
namespace hp2::recovered {
float bSin(Memory&m,std::uint16_t a) {
    using namespace port::driving::detail_group_001a4930;
    float f8=static_cast<float>(a)*debug_math_scalar(m,0x0032625c),sign=1;
    if(debug_math_scalar(m,0x00326260)<=f8)f8=f8-debug_math_scalar(m,0x00326264);
    else if(debug_math_scalar(m,0x00326268)<=f8){sign=-sign;f8=f8-debug_math_scalar(m,0x0032626c);}
    float f1=f8*f8;float f0=f8*f1;float f2=f0*f1;
    f0=f0*debug_math_scalar(m,0x00326270);float f3=f2*f1;f0=f8-f0;
    f2=f2*debug_math_scalar(m,0x00326274);f1=f3*f1;f3=f3*debug_math_scalar(m,0x00326278);
    f0=f0+f2;f1=f1*debug_math_scalar(m,0x0032627c);f0=f0-f3;f0=f0+f1;return f0*sign;
}
}

// Retail 0x0025A5E0; E-0042-C171; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x002539B0; E-0044-M01; bSin(float)
namespace hp2::recovered {
float bSin(Memory&m,float f) {
    using namespace port::driving::detail_group_001a4930;return hp2::recovered::bSin(m,static_cast<std::uint16_t>(cvt_w_s(f*debug_math_scalar(m,0x00326280))));}
}

// Retail 0x0025A748; E-0042-U191; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x00253B18; E-0044-M01; bASin(float)
namespace hp2::recovered {
std::uint16_t bASin(Memory&m,float f) {
    using namespace port::driving::detail_group_001a4930;
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
    const auto entry=0x00316e80u+static_cast<std::uint32_t>((a6*16+v3)*8);
    float f0=static_cast<float>(reconstructed)*0.0000152587890625f;
    f0=f-f0;f0=f0*m.scalar(entry+4);f0=f0*65536.0f;
    const auto value=static_cast<std::uint16_t>(m.half(entry)+cvt_w_s(f0));
    return negative?static_cast<std::uint16_t>(0u-value):value;
}
}

// Retail 0x0025A890; E-0042-U192; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x00253C60; E-0044-M01; bATan(float, float)
namespace hp2::recovered {
std::int16_t bATan(Memory&m,float x,float y) {
    using namespace port::driving::detail_group_001a4930;
    unsigned quadrant=x<0?1u:0u;if(y<0)quadrant^=3;
    const float yy=y*y,xx=x*x,sum=yy+xx;float ratio=0;std::uint16_t value;
    if(std::abs(y)<std::abs(x)){
        if(y!=0)ratio=std::abs(y)/std::sqrt(sum);
        value=hp2::recovered::bASin(m,ratio);
    }else{
        if(x!=0)ratio=std::abs(x)/std::sqrt(sum);
        value=static_cast<std::uint16_t>(0x4000u-hp2::recovered::bASin(m,ratio));
    }
    if(quadrant==3)value=static_cast<std::uint16_t>(0u-value);
    else if(quadrant==1)value=static_cast<std::uint16_t>(0x8000u-value);
    else if(quadrant==2)value=static_cast<std::uint16_t>(value+0x8000u);
    std::int16_t result;std::memcpy(&result,&value,2);return result;
}
}

// Retail 0x0025A300; E-0041-M01; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x002536D0; E-0044-M01; bDiv(int, int)
namespace hp2::recovered {
std::int32_t bDiv(std::int32_t numerator,std::int32_t denominator) {
    using namespace port::driving::detail_group_001a4930;
    if(denominator==0){if(numerator==0)return 0;return numerator>0?0x7fffffff:signed_word_bits(0x80000000);}
    const auto quotient=0x7fffffff/denominator;
    const auto product=static_cast<std::int64_t>(numerator)*quotient;
    const auto shifted=static_cast<std::uint32_t>(static_cast<std::uint64_t>(product)>>16);
    return signed_word_bits(shifted<<1);
}
}

// Retail 0x0025AA78; E-0042-C172; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x00253E20; E-0044-M01; bFixATan(int)
namespace hp2::recovered {
std::uint16_t bFixATan(Memory&m,std::int32_t value) {
    using namespace port::driving::detail_group_001a4930;
    const bool negative=value<0;if(negative)value=signed_word_bits(0u-static_cast<std::uint32_t>(value));
    std::uint32_t lower,upper,weight;
    if(value<0x200000){
        std::uint32_t pointer;
        if(value<0x40000){pointer=0x00317910u+static_cast<std::uint32_t>(value>>11)*2;weight=(static_cast<std::uint32_t>(value)<<5)&0xffff;}
        else{pointer=0x00317a18u+static_cast<std::uint32_t>(value>>14)*2;weight=(static_cast<std::uint32_t>(value)<<2)&0xffff;}
        lower=m.half(pointer);upper=m.half(pointer+2);
    }else if(value<0x1000000){weight=static_cast<std::uint32_t>(value>>8);lower=0x3eba;upper=0x3fd7;}
    else{lower=upper=0x3fff;weight=0;}
    const auto product=signed_word_bits((upper-lower)*weight);
    const auto result=static_cast<std::uint16_t>(lower+static_cast<std::uint32_t>(product>>16));
    return negative?static_cast<std::uint16_t>(0u-result):result;
}
}

// Retail 0x0025AB50; E-0042-C173; E-0042-S001; exact Debug TU src\bMath.cpp
// Debug 0x00253EF8; E-0044-M01; bFixATan(int, int)
namespace hp2::recovered {
std::int16_t bFixATan(Memory&m,std::int32_t x,std::int32_t y) {
    using namespace port::driving::detail_group_001a4930;
    unsigned quadrant=0;
    if(x<0){quadrant=1;x=signed_word_bits(0u-static_cast<std::uint32_t>(x));}
    if(y<0){quadrant^=3;y=signed_word_bits(0u-static_cast<std::uint32_t>(y));}
    std::uint16_t value=0x4000;
    if((y>>14)<x)value=hp2::recovered::bFixATan(m,hp2::recovered::bDiv(y,x));
    if(quadrant==1)value=static_cast<std::uint16_t>(0x8000u-value);
    else if(quadrant==2)value=static_cast<std::uint16_t>(value-0x8000u);
    else if(quadrant==3)value=static_cast<std::uint16_t>(0u-value);
    return signed_half_bits(value);
}
}

// TASK-0044 historical projection compatibility: forwarding only.
namespace port::driving::native {
std::uint32_t bRandom(Memory& m,std::uint32_t state,std::uint32_t divisor) { return hp2::recovered::bRandom(m,state,divisor); }
}
namespace port::driving::native {
float bRandom(Memory& m,std::uint32_t state,float value) { return hp2::recovered::bRandom(m,state,value); }
}
namespace port::driving::native {
float bRandom(Memory& m,float value) { return hp2::recovered::bRandom(m,value); }
}
namespace port::driving {
float bSin(Memory&m,std::uint16_t a) { return hp2::recovered::bSin(m,a); }
}
namespace port::driving {
float bSin(Memory&m,float f) { return hp2::recovered::bSin(m,f); }
}
namespace port::driving {
std::uint16_t bASin(Memory&m,float f) { return hp2::recovered::bASin(m,f); }
}
namespace port::driving {
std::int16_t bATan(Memory&m,float x,float y) { return hp2::recovered::bATan(m,x,y); }
}
namespace port::driving {
std::int32_t bDiv(std::int32_t numerator,std::int32_t denominator) { return hp2::recovered::bDiv(numerator,denominator); }
}
namespace port::driving {
std::uint16_t bFixATan(Memory&m,std::int32_t value) { return hp2::recovered::bFixATan(m,value); }
}
namespace port::driving {
std::int16_t bFixATan(Memory&m,std::int32_t x,std::int32_t y) { return hp2::recovered::bFixATan(m,x,y); }
}

namespace hp2::recovered {
// Debug00251D28; E-0046-S03. Preserve sqrt/length then reciprocal grouping.
std::array<float,3> bNormalize(const std::array<float,3>&v,float length){
    float sum=v[0]*v[0];sum=sum+v[1]*v[1];sum=sum+v[2]*v[2];
    float divisor=std::sqrt(sum)/length;
    if(divisor==0)return {{length,0,0}};
    divisor=1.0f/divisor;return {{v[0]*divisor,v[1]*divisor,v[2]*divisor}};
}
}
