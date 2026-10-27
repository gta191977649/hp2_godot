#include "composition.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/driving/projections.hpp"
#include <cmath>
#include <algorithm>
#include <array>
#include <cstring>
#include <numeric>

namespace port::selection {
// Debug0010FE48, E-0098-F04: UI background layers20..24 use the table
// directly; every Flailer alpha key uses table-ZMax regardless of that branch.
FlailerDepth flailer_depth_0010FE48(const port::driving::Memory& memory,int layer,float z_max){
    if(layer<0 || layer>28)hp2::not_implemented("0x0010FE48","Flailer layer outside recovered frontend submission scope");
    if(z_max!=65535.f && z_max!=16777215.f)
        hp2::not_implemented("0x0010FE48","resolved original target ZMax is required");
    const float table=memory.scalar(0x2c1530u+unsigned(layer)*4);
    const float depth=layer>=20 && layer<=24?table:z_max-table;
    if(!std::isfinite(depth) || depth<double(-2147483648.0) || depth>=double(2147483648.0))
        hp2::not_implemented("0x0010FE48","Flailer depth outside finite signed conversion domain");
    return {depth,table-z_max,std::uint32_t(port::driving::cvt_w_s(depth))<<4};
}
// Debug00104FE0/001053B8, E-0098-F04. Keep original byte-radix passes,
// negative high-byte reversal, rendering buckets and final reverse-list rebuild.
// Host indices substitute PS2 sort-key pointers/DMA; allocation failure remains
// a host exception rather than emulating the original FrameMalloc fallback.
std::vector<std::size_t> alpha_order_001053B8(const std::vector<AlphaKey>& keys){
    if(keys.size()>65535)hp2::not_implemented("0x00104FE0","alpha count exceeds original16-bit radix counter domain");
    std::vector<std::size_t> source(keys.size()),destination(keys.size());
    std::iota(source.begin(),source.end(),std::size_t(0));
    std::vector<std::uint32_t> bits;bits.reserve(keys.size());
    std::array<std::array<std::uint16_t,256>,4> counters{};
    std::array<std::uint16_t,8> renderorders{};
    std::uint32_t negative=0;
    const auto bucket=[](std::int8_t order){return order<0 || order>7?7:order;};
    for(const auto& key:keys){
        std::uint32_t raw;std::memcpy(&raw,&key.depth,4);bits.push_back(raw);
        for(unsigned pass=0;pass<4;++pass)++counters[pass][(raw>>(pass*8))&255];
        negative+=raw>>31;++renderorders[7-bucket(key.rendering_order)];
    }
    const unsigned stable_passes=negative?3:4;
    for(unsigned pass=0;pass<stable_passes;++pass){
        std::uint32_t sum=0;
        for(auto& count:counters[pass]){const auto frequency=count;count=std::uint16_t(sum);sum+=frequency;}
    }
    if(negative){
        std::uint32_t sum=negative;
        for(unsigned byte=0;byte<128;++byte){const auto frequency=counters[3][byte];counters[3][byte]=std::uint16_t(sum);sum+=frequency;}
        sum=0;
        for(int byte=255;byte>=128;--byte){sum+=counters[3][byte];counters[3][byte]=std::uint16_t(sum);}
    }
    std::uint32_t sum=0;
    for(auto& count:renderorders){const auto frequency=count;count=std::uint16_t(sum);sum+=frequency;}
    for(unsigned pass=0;pass<stable_passes;++pass){
        for(auto index:source)destination[counters[pass][(bits[index]>>(pass*8))&255]++]=index;
        source.swap(destination);
    }
    if(negative){
        for(auto index:source){auto& offset=counters[3][bits[index]>>24];
            if(bits[index]&0x80000000u)destination[--offset]=index;
            else destination[offset++]=index;
        }
        source.swap(destination);
    }
    for(auto index:source)destination[renderorders[7-bucket(keys[index].rendering_order)]++]=index;
    std::reverse(destination.begin(),destination.end());return destination;
}
}
