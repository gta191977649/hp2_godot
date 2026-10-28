#include "game_core/port/frontend/selection_render.hpp"
#include "game_core/port/frontend/composition.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/memory.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <random>
#include <stdexcept>
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
int main(){
    hp2::debug::FrontEndRenderingCar car;
    require(port::selection::frontend_render_flags(car)==0,"main car unexpectedly disables rendering features");
    car.Reflection=1;const auto reflected=port::selection::frontend_render_flags(car);
    require(reflected==0x11 && !port::selection::frontend_strip_enabled(reflected,true)
        && port::selection::frontend_strip_enabled(reflected,false),"reflection must omit sorted-alpha strips, preserving opaque ones");
    require(port::selection::frontend_strip_enabled(1,true),"disable-envmap alone must retain alpha strips");
    car.Reflection=0;require(port::selection::frontend_strip_enabled(port::selection::frontend_render_flags(car),true),"returning to main car must restore alpha eligibility");
    using port::selection::AlphaKey;using port::selection::alpha_order_001053B8;
    port::driving::Memory table;table.map(0x2c1530,29*4);
    for(int i=0;i<29;++i)table.scalar(0x2c1530+i*4,float(100+i));
    for(float max:{65535.f,16777215.f})for(int layer=0;layer<29;++layer){
        const auto result=port::selection::flailer_depth_0010FE48(table,layer,max);
        const float expected=layer>=20 && layer<=24?float(100+layer):max-float(100+layer);
        require(result.gs_depth==expected && result.alpha_sort_depth==float(100+layer)-max
            && result.packed_depth==(std::uint32_t(expected)<<4),"original background/foreground GS-depth and alpha-key domains were conflated");
    }
    require(!hp2::guard([&]{port::selection::flailer_depth_0010FE48(table,-1,65535);}).empty(),"untagged Flailer layer silently used a depth");
    require(alpha_order_001053B8({}).empty(),"empty shared alpha domain failed");
    require(alpha_order_001053B8({{0,3},{0,3},{0,-2},{0,-2},{0,9},{1,999},{-1,999}})
        ==std::vector<std::size_t>{4,1,0,2,3,5,6},"signed-depth ties/rendering buckets differ from original radix/reverse passes");
    // Independent comparison oracle for the original bitwise order, including
    // signed zero and nonfinite bit patterns; no numeric float comparison.
    std::mt19937 random(42);
    for(int count:{1,2,256,1024,65535}){
        std::vector<AlphaKey> keys;keys.reserve(count);
        for(int i=0;i<count;++i){const std::uint32_t raw=i%5?random():0x80000000u;
            float depth;std::memcpy(&depth,&raw,4);keys.push_back({std::int8_t(int(random()%12)-2),depth});}
        std::vector<std::size_t> expected(keys.size());std::iota(expected.begin(),expected.end(),0);
        const auto bits=[](float f){std::uint32_t raw;std::memcpy(&raw,&f,4);return raw;};
        const auto bucket=[](int order){return order<0 || order>7?7:order;};
        std::sort(expected.begin(),expected.end(),[&](auto a,auto b){
            const auto aa=bucket(keys[a].rendering_order),bb=bucket(keys[b].rendering_order);
            if(aa!=bb)return aa<bb;
            const auto ua=bits(keys[a].depth),ub=bits(keys[b].depth);
            const auto ka=ua&0x80000000u?~ua:ua^0x80000000u;
            const auto kb=ub&0x80000000u?~ub:ub^0x80000000u;
            if(ka!=kb)return ka>kb;
            return ua&0x80000000u?a<b:a>b;
        });
        require(alpha_order_001053B8(keys)==expected,"native Debug alpha radix differs from independent bit-order oracle");
    }
    require(!hp2::guard([&]{alpha_order_001053B8(std::vector<AlphaKey>(65536));}).empty(),"original16-bit counter overflow domain silently accepted");
    std::puts("PASS Debug shared alpha radix/buckets/signed ties,65535-key bound; no GPU/original runtime equivalence claim");
}
