#include "DCTImage.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace hp2 {

// Final PAL 2BDB30 and 2BDBB0, not the beta's file data or library defaults.
std::array<std::int16_t, 64> movieLumaQuantizer{{
    12,12,15,19,25,34,40,48, 12,12,18,22,27,44,47,46,
    17,18,21,26,35,46,52,47, 18,20,24,28,40,61,59,51,
    20,24,32,43,50,72,72,63, 25,31,42,48,58,72,81,75,
    38,46,54,61,71,84,88,85, 50,61,65,68,79,78,86,91
}};
std::array<std::int16_t, 64> movieChromaQuantizer{{
    12,16,24,47,99,99,99,99, 16,21,26,66,99,99,99,99,
    24,26,56,99,99,99,99,99, 47,66,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99, 99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99, 99,99,99,99,99,99,99,99
}};

// PAL 236D90..236DF4. MULT's low word and ADDIU wrap before signed DIV;
// explicit unsigned arithmetic avoids turning that behavior into C++ overflow.
void DCTTable::ScaleTable(std::int32_t scale, std::int16_t* table) {
    if (table == nullptr)
        throw std::invalid_argument("MV3 quantizer requires 64 output cells");
    for (std::uint32_t i = 0; i < 64; ++i) {
        const std::uint32_t word =
            static_cast<std::uint32_t>(static_cast<std::int32_t>(table[i])) *
            static_cast<std::uint32_t>(scale) + 50u;
        const std::int32_t signedWord = word <= 0x7fffffffu ?
            static_cast<std::int32_t>(word) :
            -1 - static_cast<std::int32_t>(~word);
        const std::int32_t value = signedWord / 100;
        table[i] = static_cast<std::int16_t>(std::clamp(value, 1, 32767));
    }
}

// PAL 236DF8..236E48.
std::int32_t DCTTable::ConvertQualityToScale(std::int32_t quality) noexcept {
    quality = std::clamp(quality, 1, 100);
    if (quality < 50)
        return 5000 / quality;
    return 200 - quality * 2;
}

// PAL 236E48..236E98. Authored image tables are separate non-overlapping
// slices, matching the original 128-byte memcpy contract.
void DCTTable::GenerateLuma(std::int32_t quality, std::int16_t* table) {
    if (table == nullptr)
        throw std::invalid_argument("MV3 missing luma quantizer output");
    std::memcpy(table, movieLumaQuantizer.data(), 128);
    ScaleTable(ConvertQualityToScale(quality), table);
}

// PAL 236E98..236EE8.
void DCTTable::GenerateChroma(std::int32_t quality, std::int16_t* table) {
    if (table == nullptr)
        throw std::invalid_argument("MV3 missing chroma quantizer output");
    std::memcpy(table, movieChromaQuantizer.data(), 128);
    ScaleTable(ConvertQualityToScale(quality), table);
}

} // namespace hp2
