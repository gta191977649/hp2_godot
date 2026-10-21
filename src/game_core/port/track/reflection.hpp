#pragma once
// Port-owned projection of Retail0010AC50's original context2 packet defaults.
#include "effects.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
namespace port::track {
struct EnvironmentPacket {
    std::uint64_t tex0=0x620213000ULL;
    std::uint64_t alpha=0x1000000048ULL;
};
inline EnvironmentPacket environment_packet_0010AC50(bool flag4,int display_mode=0){
    if(display_mode==3)return {0x620012800ULL,0x4000000048ULL};
    return {0x620213000ULL,flag4?0x1000000058ULL:0x1000000048ULL};
}
// The current separate-pass host shader consumes source alpha. Packet31C instead
// consumes the framebuffer alpha left by the first pass (or a previous draw).
// Never silently substitute its own RGBAQ alpha for that destination value.
inline void require_environment_source_alpha(bool effective_flag4,int display_mode){
    if(effective_flag4&&display_mode!=3)
        hp2::not_implemented("0x0010AC50","Environment packet31C destination-alpha composition is not implemented by the source-alpha vehicle shader");
}
// Original context2 M1.w + M2.w * reflectedDirection.z; unit direction bound.
// Null means the low-byte RGBAQ extraction needs additional range analysis.
inline std::optional<std::array<std::uint8_t,2>> environment_alpha_bounds(const std::array<float,32>& material){
    const float middle=material[23],extent=std::fabs(material[27]);
    const float lower=middle-extent,upper=middle+extent;
    if(!std::isfinite(lower)||!std::isfinite(upper)||lower<0||upper>255)return std::nullopt;
    return std::array<std::uint8_t,2>{{std::uint8_t(std::trunc(lower)),std::uint8_t(std::trunc(upper))}};
}
inline bool environment_318_additive_supported(const EffectState& state){
    return state.environment_pass&&(state.flags&4)==0&&!state.rewrite_alpha;
}
}
