#pragma once
#include "port/vehicle/texture_state.hpp"

namespace port::track {
// Port-side track inspection state. Retail 00108CC0, E-0048-A03.
// MXL is retained without passing through the car viewer's MXL0 restriction.
// Original mip source pixels are supplied separately by Scene::mipmaps. The
// host shader projects LOD/filtering from these raw TEX1 fields; that derivative
// projection is not a claim of PS2 rasterizer equivalence.
struct TextureState : vehicle::TextureState {
    std::uint8_t maximum_mip_level{};
    bool requires_lod_projection{};
    bool additive{}; // GS ALPHA48: (Cs-0)*As/128+Cd.
};
inline TextureState texture_state_00108CC0(const hp2::assets::TpkTextureHeader& header) {
    if(header.Field_0x6e>6||header.Field_0x6f!=1||header.Field_0x70>5||header.Field_0x71>3)
        hp2::not_implemented("0x00108CC0","Track TEX1 outside bounded original register fields");
    auto base=header;base.Field_0x6e=0;
    base.Field_0x70=3;base.Field_0x71=0;base.Field_0x72=0;
    const bool additive=header.alpha_bits==0x48&&header.alpha_fix==0;
    if(additive)base.alpha_bits=0x44;
    TextureState result;
    static_cast<vehicle::TextureState&>(result)=vehicle::texture_state_00108CC0(base);
    result.maximum_mip_level=header.Field_0x6e;
    result.additive=additive;
    result.alpha_register=(std::uint64_t(header.alpha_fix)<<32)|header.alpha_bits;
    result.requires_lod_projection=header.Field_0x6e!=0||header.Field_0x70!=3||header.Field_0x71!=0||header.Field_0x72!=0;
    result.tex1_register=(std::uint64_t(static_cast<std::int64_t>(header.Field_0x72))<<32)|
        (std::uint64_t(header.Field_0x70)<<6)|(std::uint64_t(header.Field_0x6f)<<5)|
        (std::uint64_t(header.Field_0x6e)<<2)|(std::uint64_t(header.Field_0x71)<<19);
    return result;
}
}
