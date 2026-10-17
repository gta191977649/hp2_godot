
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x0025A3D0; E-0041-M01; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving::native {
std::uint32_t bRandom(Memory& m,std::uint32_t state,std::uint32_t divisor) {
    using namespace detail_group_00137d90;
    if(!divisor)hp2::not_implemented("0x0025A3E0","Original divide-by-zero BREAK7");
    const auto old=m.word(state);const auto v3=old^0x1d872b41u;
    const auto a0=v3^(v3>>5);m.word(state,a0^v3^(a0<<27));return old%divisor;
}
}

// Retail 0x0025A410; E-0042-C169; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving::native {
float bRandom(Memory& m,std::uint32_t state,float value) {
    using namespace detail_group_00137d90;
    const auto result=bRandom(m,state,0x7fffffffu);
    const float f0=value*0x1p-31f;
    float f1;
    if(sb(result)<0){f1=static_cast<float>((result>>1)|(result&1));f1=f1+f1;}
    else f1=static_cast<float>(sb(result));
    return f0*f1;
}
}

// Retail 0x0025A4A8; E-0042-U190; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving::native {
float bRandom(Memory& m,float value){
    using namespace detail_group_00137d90;return bRandom(m,ga(0xc778),value);}
}

// Retail 0x0025A540; E-0042-C170; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving {
float bSin(Memory&m,std::uint16_t a){
    using namespace detail_group_001a4930;
    float f8=static_cast<float>(a)*g(m,-0x673c),sign=1;
    if(g(m,-0x6738)<=f8)f8=f8-g(m,-0x6734);
    else if(g(m,-0x6730)<=f8){sign=-sign;f8=f8-g(m,-0x672c);}
    float f1=f8*f8;float f0=f8*f1;float f2=f0*f1;
    f0=f0*g(m,-0x6728);float f3=f2*f1;f0=f8-f0;
    f2=f2*g(m,-0x6724);f1=f3*f1;f3=f3*g(m,-0x6720);
    f0=f0+f2;f1=f1*g(m,-0x671c);f0=f0-f3;f0=f0+f1;return f0*sign;
}
}

// Retail 0x0025A5E0; E-0042-C171; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving {
float bSin(Memory&m,float f){
    using namespace detail_group_001a4930;return bSin(m,static_cast<std::uint16_t>(cvt_w_s(f*g(m,-0x6718))));}
}

// Retail 0x0025A748; E-0042-U191; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving {
std::uint16_t bASin(Memory&m,float f){
    using namespace detail_group_001a4930;
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
}

// Retail 0x0025A890; E-0042-U192; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving {
std::int16_t bATan(Memory&m,float x,float y){
    using namespace detail_group_001a4930;
    unsigned quadrant=x<0?1u:0u;if(y<0)quadrant^=3;
    const float yy=y*y,xx=x*x,sum=yy+xx;float ratio=0;std::uint16_t value;
    if(std::abs(y)<std::abs(x)){
        if(g(m,-0x66e0)<sum && y!=0)ratio=std::abs(y)/std::sqrt(sum);
        value=bASin(m,ratio);
    }else{
        if(g(m,-0x66dc)<sum && x!=0)ratio=std::abs(x)/std::sqrt(sum);
        value=static_cast<std::uint16_t>(0x4000u-bASin(m,ratio));
    }
    if(quadrant==3)value=static_cast<std::uint16_t>(0u-value);
    else if(quadrant==1)value=static_cast<std::uint16_t>(0x8000u-value);
    else if(quadrant==2)value=static_cast<std::uint16_t>(value+0x8000u);
    std::int16_t result;std::memcpy(&result,&value,2);return result;
}
}

// Retail 0x0025A300; E-0041-M01; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving {
std::int32_t bDiv(std::int32_t numerator,std::int32_t denominator){
    using namespace detail_group_001a4930;
    if(denominator==0){if(numerator==0)return 0;return numerator>0?0x7fffffff:signed_word_bits(0x80000000);}
    const auto quotient=0x7fffffff/denominator;
    const auto product=static_cast<std::int64_t>(numerator)*quotient;
    const auto shifted=static_cast<std::uint32_t>(static_cast<std::uint64_t>(product)>>16);
    return signed_word_bits(shifted<<1);
}
}

// Retail 0x0025AA78; E-0042-C172; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving {
std::uint16_t bFixATan(Memory&m,std::int32_t value){
    using namespace detail_group_001a4930;
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
}

// Retail 0x0025AB50; E-0042-C173; E-0042-S001; exact Debug TU src\bMath.cpp
namespace port::driving {
std::int16_t bFixATan(Memory&m,std::int32_t x,std::int32_t y){
    using namespace detail_group_001a4930;
    unsigned quadrant=0;
    if(x<0){quadrant=1;x=signed_word_bits(0u-static_cast<std::uint32_t>(x));}
    if(y<0){quadrant^=3;y=signed_word_bits(0u-static_cast<std::uint32_t>(y));}
    std::uint16_t value=0x4000;
    if((y>>14)<x)value=bFixATan(m,bDiv(y,x));
    if(quadrant==1)value=static_cast<std::uint16_t>(0x8000u-value);
    else if(quadrant==2)value=static_cast<std::uint16_t>(value-0x8000u);
    else if(quadrant==3)value=static_cast<std::uint16_t>(0u-value);
    return signed_half_bits(value);
}
}
