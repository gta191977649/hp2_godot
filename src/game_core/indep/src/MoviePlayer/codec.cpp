#include "MV3Codec.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace hp2 {

// Source static initializer232128 only derives the second pointer from the
// first. Native data initialization has the same one-storage pointer relation;
// this adaptation is not an additional original callable body.
CodecSharedState movieCodecShared;

namespace {
std::uint8_t clampPixel(std::int32_t value) {
    const auto& shared = movieCodecShared;
    const auto base = reinterpret_cast<std::uintptr_t>(shared.clamp.data());
    const auto zero = reinterpret_cast<std::uintptr_t>(shared.clampZero);
    const auto address = static_cast<std::int64_t>(zero) + value;
    if (address < static_cast<std::int64_t>(base) ||
        address >= static_cast<std::int64_t>(base + shared.clamp.size()))
        throw std::out_of_range("MV3 pixel is outside retained clamp storage");
    return *reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(address));
}
} // namespace

// PAL231D70..231DA0, selected by the unique RingBuffer::StartupBuffer
// consumer. The identical signed-reader clone remains separately unlicensed.
std::uint32_t GetU32(const void* input) {
    if (input == nullptr)
        throw std::invalid_argument("MV3 missing four-byte input");
    const auto* bytes = static_cast<const std::uint8_t*>(input);
    return static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8) |
        (static_cast<std::uint32_t>(bytes[2]) << 16) |
        (static_cast<std::uint32_t>(bytes[3]) << 24);
}

// PAL231DD0..231DF0, explicitly little endian and safe for unaligned input.
std::uint16_t GetU16(const void* input) {
    if (input == nullptr)
        throw std::invalid_argument("MV3 missing two-byte input");
    const auto* bytes = static_cast<const std::uint8_t*>(input);
    return static_cast<std::uint16_t>(bytes[0] |
        (static_cast<std::uint16_t>(bytes[1]) << 8));
}

// PAL231DF0..231E10.
std::int16_t GetI16(const void* input) {
    const std::uint16_t word = GetU16(input);
    return word <= 0x7fffu ? static_cast<std::int16_t>(word) :
        static_cast<std::int16_t>(-1 - static_cast<std::int32_t>(word ^ 0xffffu));
}

// PAL231E10..231EEC. Only the first live codec rebuilds the actual clipping
// and five-bit expansion tables. Releasing every codec permits reinitialization.
Codec::Codec() : temporary_() {
    auto& shared = movieCodecShared;
    if (shared.instanceCount == 0) {
        std::fill_n(shared.clamp.data(), 256, static_cast<std::uint8_t>(0));
        std::fill_n(shared.clamp.data() + 512, 256, static_cast<std::uint8_t>(255));
        for (std::int32_t value = 0; value < 256; ++value)
            shared.clampZero[value] = static_cast<std::uint8_t>(value);
        std::int32_t numerator = 16;
        for (auto& value : shared.fiveBitExpansion) {
            value = static_cast<std::uint8_t>(numerator / 31);
            numerator += 255;
        }
    }
    ++shared.instanceCount;
}

// PAL231EF0..231F60. Deleting-destructor flags become actual C++ ownership.
Codec::~Codec() {
    --movieCodecShared.instanceCount;
    temporary_.reset();
}

// PAL231F60..232058. Source byte order is B,G,R -> Y,U,V. Keep individual
// binary32 operations and source grouping; build uses /fp:strict, no FMA.
void Codec::rgb2yuv(std::uint8_t* pixel) const {
    if (pixel == nullptr)
        throw std::invalid_argument("MV3 missing BGR pixel");
    const auto& shared = movieCodecShared;
    const float green = pixel[1];
    const float red = pixel[2];
    const float blue = pixel[0];
    const float yGreen = green * shared.yGreen;
    const float yRed = red * shared.yRed;
    const float yBlue = blue * shared.yBlue;
    const float uRed = red * shared.uRed;
    const float uGreen = green * shared.uGreen;
    const float uBlue = blue * 0.5f;
    const float vGreen = green * shared.vGreen;
    const float vRed = red * 0.5f;
    const float vBlue = blue * shared.vBlue;
    float y = yRed + yGreen;
    y = y + yBlue;
    y = y + 0.5f;
    float u = uRed - uGreen;
    u = u + uBlue;
    u = u + 128.0f;
    u = u + 0.5f;
    float v = vRed - vGreen;
    v = v - vBlue;
    v = v + 128.0f;
    v = v + 0.5f;
    pixel[0] = clampPixel(static_cast<std::int32_t>(y));
    pixel[1] = clampPixel(static_cast<std::int32_t>(u));
    pixel[2] = clampPixel(static_cast<std::int32_t>(v));
}

// PAL232058..232128. All three input lanes are read before the first store.
void Codec::yuv2rgb(std::uint8_t* pixel) const {
    if (pixel == nullptr)
        throw std::invalid_argument("MV3 missing YUV pixel");
    const auto& shared = movieCodecShared;
    const float v = static_cast<float>(pixel[2]) - 128.0f;
    const float u = static_cast<float>(pixel[1]) - 128.0f;
    const float y = pixel[0];
    const float redV = v * shared.redV;
    const float greenU = u * shared.greenU;
    const float greenV = v * shared.greenV;
    const float blueU = u * shared.blueU;
    float red = y + redV;
    red = red + 0.5f;
    float green = y - greenU;
    green = green - greenV;
    green = green + 0.5f;
    float blue = y + blueU;
    blue = blue + 0.5f;
    pixel[2] = clampPixel(static_cast<std::int32_t>(red));
    pixel[1] = clampPixel(static_cast<std::int32_t>(green));
    pixel[0] = clampPixel(static_cast<std::int32_t>(blue));
}

} // namespace hp2
