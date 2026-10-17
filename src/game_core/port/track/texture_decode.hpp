#pragma once
#include "game_core/port/assets/hp2_tpk_texture.hpp"

namespace port::track {
// Port-side orientation adapter, E-0049-T01. Original VIF ST coordinates address
// source row zero at T=0. The legacy indexed decoder first reverses its rows;
// its flip_vertical=true option reverses that internal conversion back. Direct
// PSMCT32 already retains source row order and must not receive another flip.
// Apply this same adapter independently to the base image and every mip slice.
inline std::optional<hp2::assets::DecodedTexture> decode_texture_level(
    const hp2::assets::Hp2TpkArchive& archive,const hp2::assets::TpkTextureHeader& header) {
    return header.bit_depth==32?archive.decode_linear32(header):archive.decode(header,true,true);
}
}
