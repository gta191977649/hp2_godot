#include "indep/src/speech/mgr.hpp"

#include <cmath>
#include <cstdint>

// PAL223528..22358C. The result is an authored bit category, not a speed.
int32_t worldSpeechConvertSpeed(float speed)
{
    if (speed < 100.0f) return 0;
    if (340.0f < speed) return 4096;
    const float difference = speed - 100.0f;
    const float scaled = difference * 0.05000000074505806f;
    // The original compiler conversion returns zero for NaN. All finite
    // values reaching it are bounded to [0,12] by the two source branches.
    const std::uint32_t index = std::isnan(scaled) ? 0u : static_cast<std::uint32_t>(scaled);
    return static_cast<std::int32_t>(std::uint32_t{1} << (index & 31u));
}

#include "bware/inc/bWare.hpp"

namespace {
constexpr int32_t carDescriptionCategories[67] = {
    0x1, 0x1, 0x20, 0x20, 0x1, 0x8, 0x8, 0x2,
    0x2, 0x2000000, 0x2000000, 0x40000, 0x40000, 0x80000, 0x80000, 0x100000,
    0x100000, 0x100, 0x100, 0x2000, 0x400, 0x200, 0x20000000, 0x80,
    0x80, 0x4000000, 0x4000000, 0x1000, 0x1000, 0x400000, 0x400000, 0x2,
    0x10, 0x4, 0x20, 0x20000, 0x20000, 0x10000, 0x10000, 0x10,
    0x10, 0x8000, 0x8000, 0x800, 0x800, 0x40, 0x40, 0x4,
    0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0,
};
constexpr int32_t carUnlockCategories[67] = {
    0x1000, 0x1000, 0x200000, 0x200000, 0x1, 0x10, 0x10, 0x2000,
    0x2000, 0x4000, 0x4000, 0x8000, 0x8000, 0x800, 0x800, 0x1,
    0x1, 0x4, 0x4, 0x40000, 0x40, 0x80, 0x80, 0x200,
    0x200, 0x400000, 0x400000, 0x80000, 0x80000, 0x2, 0x2, 0x2,
    0x10, 0x4, 0x20, 0x10000, 0x10000, 0x20000, 0x20000, 0x100000,
    0x100000, 0x400, 0x400, 0x20, 0x20, 0x100, 0x100, 0x8,
    0x8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0,
};
constexpr const char* colourNames[] = {
    "black",
    "blue",
    "red",
    "yellow",
    "white",
    "silver",
    "gold",
    "purple",
    "green",
    "brown",
    "orange",
    "light blue",
    "light green",
    "any",
};
}

int32_t worldSpeechConvertCarTypeToDescription(int32_t carType)
{
    return carType >= 0 && carType < 67 ? carDescriptionCategories[carType] : 0;
}

int32_t worldSpeechConvertCarTypeToUnlockSpeech(int32_t carType)
{
    return carType >= 0 && carType < 67 ? carUnlockCategories[carType] : 0;
}

int32_t worldSpeechConvertTrackNumberToDescription(int32_t trackNumber)
{
    switch (trackNumber) {
    case 11: case 14:
        return 0x1000;
    case 12: case 15:
        return 0x2000;
    case 13: case 16:
        return 0x4000;
    case 21: case 24:
        return 0x8;
    case 22: case 25:
        return 0x10;
    case 23: case 26:
        return 0x20;
    case 31: case 34:
        return 0x200;
    case 32: case 35:
        return 0x400;
    case 33: case 36:
        return 0x800;
    case 41: case 44:
        return 0x4;
    case 42: case 45:
        return 0x2;
    case 43: case 46:
        return 0x1;
    case 61: case 64:
        return 0x40;
    case 62: case 65:
        return 0x80;
    case 63: case 66:
        return 0x100;
    default: return 0;
    }
}

int32_t worldSpeechConvertTrackNumberToUnlockSpeech(int32_t trackNumber)
{
    switch (trackNumber) {
    case 11: case 14:
        return 0x8000;
    case 12: case 15:
        return 0x20;
    case 13: case 16:
        return 0x10000;
    case 21: case 24:
        return 0x8;
    case 22: case 25:
        return 0x10;
    case 23: case 26:
        return 0x800;
    case 31: case 34:
        return 0x1000;
    case 32: case 35:
        return 0x2000;
    case 33: case 36:
        return 0x4000;
    case 41: case 44:
        return 0x100;
    case 42: case 45:
        return 0x2;
    case 43: case 46:
        return 0x20000;
    case 61: case 64:
        return 0x4;
    case 62: case 65:
        return 0x200;
    case 63: case 66:
        return 0x400;
    default: return 0;
    }
}

int32_t worldSpeechConvertColourName(const char* name)
{
    for (uint32_t index = 0; index < sizeof(colourNames) / sizeof(colourNames[0]); ++index)
        if (stringCompareInsensitive(colourNames[index], name) == 0)
            return static_cast<int32_t>(uint32_t{1} << index);
    return 0;
}
