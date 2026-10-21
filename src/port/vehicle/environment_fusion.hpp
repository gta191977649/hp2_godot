#pragma once
// Port-side RGB-only candidate checks; these do not authorize a host draw.
// E0052D13: Retail0010AC50 packet31C ALPHA58; normal PSMCT32/FBA0.
// Retail SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Debug001058D8 corroborates common FRAME1/2 write mask; these helpers are not
// recovered original function names or a replacement for framebuffer alpha.
#include "port/vehicle/texture_state.hpp"
#include <array>
#include <algorithm>
namespace port::vehicle {
inline bool single_triangle_environment_fusion_rgb_candidate(
    const hp2::assets::TpkTextureHeader& header,std::size_t index_count,
    bool effective_flag4,bool lit,int display_mode){
    // One primitive avoids changing base-all/environment-all overlap semantics.
    // Full support still requires exact context2 alpha framebuffer writes.
    if(index_count!=3||!effective_flag4||!lit||display_mode==3)return false;
    const auto test=test_state_00105AC0(header);
    return header.texture_fx==0&&header.alpha_bits==0x0a&&header.alpha_fix==0&&
        test.alpha_test==5&&test.alpha_reference==0&&test.write_depth;
}
// Inputs are already GS MODULATE/truncated/clamped bytes, after Gouraud
// interpolation. PSMCT32/FBA0 first-pass copy writes base alpha verbatim.
// RGB only: framebuffer alpha write-mask behavior is a separate prerequisite.
inline std::array<std::uint8_t,3> fused_environment_copy_31c_rgb(
    const std::array<std::uint8_t,4>& base,
    const std::array<std::uint8_t,4>& environment){
    std::array<std::uint8_t,3> result{};
    for(unsigned i=0;i<3;++i)result[i]=std::uint8_t(std::min(255,
        int(base[i])+int(environment[i])*int(base[3])/128));
    return result;
}
}
