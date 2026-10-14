#pragma once
#include "game_core/port/assets/hp2_tpk_texture.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cstdint>

namespace port::vehicle {
// Port-side projection of the static texture state emitted by 00108CC0.
// E-0033-T06. Not a complete GS state machine or original C++ ABI.
enum class BlendMode { Copy, SourceOver };
struct TextureState {
    BlendMode blend = BlendMode::Copy;
    bool deferred = false;
    bool texture_alpha = true; // TEX0.TCC is forced to 1 by 00108CC0.
    std::uint64_t alpha_register = 0;
    std::uint64_t tex1_register = 0xe0;
    bool magnification_linear = true;
    bool minification_linear = false;
    bool repeat_u = false;
    bool repeat_v = false;
};

struct TestState {
    std::uint32_t test_register = 0;
    bool write_depth = false;
    std::uint8_t alpha_test = 0;
    std::uint8_t alpha_reference = 0;
    std::uint8_t depth_test = 2; // GS GEQUAL, with original reversed depth convention.
    bool accepts_alpha(std::uint8_t alpha) const {
        return alpha_test == 6 ? alpha > alpha_reference : alpha >= alpha_reference;
    }
};

// Projection for the normal target defaults set by 001064C8: ZMSK=0,
// ZTST=2, ATST=6. Does not claim all framebuffer/environment-map targets.
// E-0033-T07 corrects the initial packet-index interpretation: +0x4F/+0x74
// select TEST (bit0), while +0x50 selects depth writes (bit1) independently.
// The global 001033C0 gate can clear bit1; this viewer selects the depth-enabled
// normal route by default. +0x4D only determines the sorting partition.
inline TestState test_state_00105AC0(const hp2::assets::TpkTextureHeader& header,
                                   bool second_context = false, bool depth_route_enabled = true) {
    const bool bit_zero = header.is_any_semitransparency != 0 || header.Field_0x74 != 0;
    const bool write_depth = header.Field_0x50 != 0 && depth_route_enabled && !second_context;
    if (bit_zero) return {0x5000b, write_depth, 5, 0, 2};
    return {0x5030d, write_depth, 6, 0x30, 2};
}
inline TextureState texture_state_00108CC0(const hp2::assets::TpkTextureHeader& header) {
    if (header.texture_fx != 0)
        hp2::not_implemented("0x00108CC0", "Vehicle texture TFX other than MODULATE");
    if (header.Field_0x6e != 0 || header.Field_0x6f != 1 || header.Field_0x70 != 3 ||
        header.Field_0x71 != 0 || header.Field_0x72 != 0)
        hp2::not_implemented("0x00108CC0", "Vehicle TEX1 outside MXL0/MMAG1/MMIN3/L0/K0");
    if (header.alpha_fix != 0 || (header.alpha_bits != 0x0a && header.alpha_bits != 0x44))
        hp2::not_implemented("0x00108CC0", "Vehicle static ALPHA equation other than copy/source-over");
    return {header.alpha_bits == 0x44 ? BlendMode::SourceOver : BlendMode::Copy,
            header.Field_0x4d != 0, true,
            (static_cast<std::uint64_t>(header.alpha_fix) << 32) | header.alpha_bits,
            0xe0, true, false, (header.Field_0x4c & 1) != 0, (header.Field_0x4c & 2) != 0};
}

// Scalar reference for the supported RGB blend equations in GS integer units.
// ALPHA0A: Cs. ALPHA44: ((Cs-Cd)*As >> 7)+Cd. Destination alpha is not used.
// Returns the unclamped result; framebuffer color-clamp state is not inferred here.
inline int blend_channel_00108CC0(BlendMode mode, std::uint8_t source,
                                 std::uint8_t destination, std::uint8_t source_alpha) {
    if (mode == BlendMode::Copy) return source;
    const int product = (static_cast<int>(source) - destination) * source_alpha;
    const int scaled = product >= 0 ? product / 128 : -((-product + 127) / 128);
    return scaled + destination;
}
} // namespace port::vehicle
