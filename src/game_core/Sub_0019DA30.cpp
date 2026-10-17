#include "Sub_0019DA30.hpp"
#include "Sub_0018F808.hpp"
#include "port/not_implemented.hpp"
#include <algorithm>
#include <cstring>
namespace port::driving::native {
namespace {
constexpr std::uint32_t gp=0x003358f0;
std::uint32_t address(std::uint16_t offset){return gp+static_cast<std::int16_t>(offset);}
float g(Memory& m,std::uint16_t offset){return m.scalar(address(offset));}
std::int32_t signed_bits(std::uint32_t bits){std::int32_t value;std::memcpy(&value,&bits,4);return value;}
}
void Sub_0019DA30(Memory& m){
    for(auto a:{0x00310dc0u,0x00310dc8u,0x00310dd0u,0x00310dd8u,0x00310dc4u,0x00310dd4u})m.word(a,0);
    port::driving::Sub_0018F808(m,0x00310df8,0x00310de0,6,0,10);
    port::driving::Sub_0018F808(m,0x00310e30,0x00310e10,8,0,1);
}
void Sub_001395B0(Memory& m){
    m.word(0x002f0570,0);m.word(0x002f0574,0);m.word(0x002f0578,m.word(address(0x8458)));
    m.word(0x002f057c,0);m.scalar(0x002f0584,20);m.scalar(0x002f0580,2);
    m.word(address(0xa730),m.word(address(0x845c)));
    // ELF0018F9E0: sw a1,0(a0); jr ra; sw a2,4(a0).
    m.word(address(0xa770),0x002f0570);m.word(address(0xa770)+4,3);
}
float Sub_001A33A8(Memory& m,std::uint32_t value){return static_cast<float>(signed_bits(value))*g(m,0x8d04);}
std::uint32_t Sub_001A33C0(float f12){f12=f12*3600.0f;f12=f12+0.5f;return static_cast<std::uint32_t>(port::driving::cvt_w_s(f12));}
std::uint32_t Sub_00136DD0(float f12){f12=f12*4000.0f;f12=f12+0.5f;return static_cast<std::uint32_t>(port::driving::cvt_w_s(f12));}
void Sub_001A33F0(Memory& m,float f12,ScheduleServices& services){
    const auto index=m.word(address(0xb384))&15u;std::uint32_t value;
    if(m.word(address(0xaf44))){
        if(!services.Sub_00183358)hp2::not_implemented("0x00183358","Original frame replay read service is absent");
        value=services.Sub_00183358(8,2);services.Sub_00183358(4,4);
    }else{
        float accumulator=f12*g(m,0x8d08);accumulator=accumulator+g(m,0xb380);
        if(m.word(address(0xb364))==0)accumulator=std::min(g(m,0x8d10),std::max(accumulator,g(m,0x8d0c)));
        else accumulator=std::min(g(m,0x8d18),std::max(accumulator,g(m,0x8d14)));
        value=Sub_001A33C0(accumulator);
        const auto integer=Sub_00136DD0(Sub_001A33A8(m,value));
        m.scalar(address(0xb380),accumulator-static_cast<float>(signed_bits(integer))*g(m,0x8d1c));
        if(!services.Sub_00183498)hp2::not_implemented("0x00183498","Original frame observation service is absent");
        services.Sub_00183498(value,8,2);services.Sub_00183498(index,4,4);
    }
    m.word(address(0xb378),value);m.scalar(address(0xb374),Sub_001A33A8(m,value));
}
void Sub_001A3500(Memory& m){
    const auto dt=g(m,0xb374);const auto sum=g(m,0xb37c)+dt;
    const auto integer=port::driving::cvt_w_s(sum*60.0f);
    const auto next=m.word(address(0xb370))+static_cast<std::uint32_t>(integer?integer:1);
    m.word(address(0xb36c),next);m.word(address(0xb370),next);
    m.scalar(address(0xb37c),sum-static_cast<float>(integer)*g(m,0x8d20));
    m.word(address(0xb384),m.word(address(0xb384))+1);
    m.word(address(0xb368),m.word(address(0xb368))+Sub_00136DD0(dt));
}
void Sub_001A3598(Memory& m){
    const auto integer=port::driving::cvt_w_s(g(m,0x8d24));
    m.word(address(0xb394),1);m.word(address(0xb388),static_cast<std::uint32_t>(integer));m.word(address(0xb390),1);
    m.word(address(0xb398),0);m.word(address(0xb39c),0);m.word(address(0xb3a0),0);
    m.scalar(address(0xb38c),static_cast<float>(integer)*g(m,0x8d28));
}
void Sub_001A35E0(Memory& m,float f12){m.scalar(address(0xb398),f12);m.word(address(0xb3a4),0);}
void Sub_001A35F0(Memory& m){
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
