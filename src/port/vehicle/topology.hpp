#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace port::vehicle {
// Port-side scalar projection of the VU lower-instruction ADC slice beginning
// ELF 002A76E0 (uploaded VU PC 1AF), not an original C++ function/signature.
// E-0033-T03: original RINIT/RNEXT schedule and VF16..19.w output provenance.
// Returns one packed-GIF ADC bit per input vertex; true suppresses drawing.
inline std::vector<bool> adc_002A76E0(
    std::uint8_t count, std::uint8_t mode, const std::array<std::uint32_t, 4>& control) {
    if (count == 0 || count > 32 || mode > 7 || (count - 1) / 4 != mode)
        throw std::runtime_error("Unsupported vehicle VU packet count/mode");
    // PC 1C5 jumps to 1DD-2*mode, delay slot RINIT VF23.x. The unrolled
    // dispatch reseeds at PC 1D0 (z) and 1D8 (w). RNEXT results already in
    // the output pipeline make the w-seeded tail 13 vertices, not 16.
    const unsigned first_component = mode == 3 ? 3 : (mode == 7 ? 2 : 0);
    const unsigned split = mode >= 4 ? 4u * mode - 9u : count;
    std::uint32_t state = control[first_component] & 0x007fffffu;
    std::vector<bool> result;
    result.reserve(count);
    for (unsigned vertex = 0; vertex < count; ++vertex) {
        if (vertex == split) state = control[3] & 0x007fffffu;
        // VU RNEXT advances the 23-bit LFSR; its output has exponent 0x3F800000.
        // Packed GIF XYZ2 uses word w bit 15 for ADC; the exponent is irrelevant.
        state = ((state << 1) ^ (((state >> 4) ^ (state >> 22)) & 1u)) & 0x007fffffu;
        result.push_back((state & 0x8000u) != 0);
    }
    return result;
}
} // namespace port::vehicle
