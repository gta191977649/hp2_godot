#include "DCTImage.h"
#include "MV3Codec.h"

#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace hp2 {
DCTBlockState movieDctBlockState{2, 2, 0, 0};
namespace {

// R5900 integer butterflies wrap32-bit additions/products and perform signed
// shifts. Express those operations directly without host signed-overflow UB.
std::int32_t signedWord(std::uint32_t word) noexcept {
    return word <= 0x7fffffffu ? static_cast<std::int32_t>(word) :
        -1 - static_cast<std::int32_t>(~word);
}
std::int32_t sum(std::int32_t left, std::int32_t right) noexcept {
    return signedWord(static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right));
}
std::int32_t difference(std::int32_t left, std::int32_t right) noexcept {
    return signedWord(static_cast<std::uint32_t>(left) - static_cast<std::uint32_t>(right));
}
std::int32_t product(std::int32_t left, std::int32_t right) noexcept {
    return signedWord(static_cast<std::uint32_t>(left) * static_cast<std::uint32_t>(right));
}
std::int32_t shifted(std::int32_t value, unsigned amount) noexcept {
    return value >= 0 ? static_cast<std::int32_t>(static_cast<std::uint32_t>(value) >> amount) :
        -1 - static_cast<std::int32_t>((~static_cast<std::uint32_t>(value)) >> amount);
}

// Exactly the two-frequency butterfly used in both passes of IDCT4/Add4.
// Input coefficients are compact2x2; quantizers remain the full8x8 table.
std::array<std::int32_t, 8> twoFrequencies(std::int32_t dc, std::int32_t ac) noexcept {
    const auto cosine473 = shifted(product(ac, 473), 8);
    const auto cosine362 = shifted(product(ac, 362), 8);
    const auto cosine277 = shifted(product(ac, 277), 8);
    const auto first = difference(cosine473, ac);
    const auto second = difference(cosine362, first);
    const auto third = sum(difference(cosine277, cosine473), second);
    return {{sum(dc, ac), sum(dc, first), sum(dc, second), difference(dc, third),
              sum(dc, third), difference(dc, second), difference(dc, first), difference(dc, ac)}};
}

std::uint8_t clipped(std::int32_t value, bool centered) {
    // Both pointers borrow the SAME actual768-byte Codec-owned clamp array.
    // This is an inlined source table lookup, not a second clipping owner.
    const auto& state = movieCodecShared;
    const auto base = reinterpret_cast<std::uintptr_t>(state.clamp.data());
    const auto zero = reinterpret_cast<std::uintptr_t>(
        centered ? state.dctClampZero : state.clampZero);
    const auto address = static_cast<std::int64_t>(zero) + value;
    if (address < static_cast<std::int64_t>(base) ||
        address >= static_cast<std::int64_t>(base + state.clamp.size()))
        throw std::out_of_range("MV3 IDCT sample exceeds authored clamp storage");
    return *reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(address));
}

void requireBlock(const std::int16_t* coefficients, const std::int16_t* quantizer,
                   std::uint8_t* output, std::int32_t pitch) {
    if (coefficients == nullptr || quantizer == nullptr || output == nullptr || pitch < 8)
        throw std::invalid_argument("MV3 IDCT requires coefficients, quantizer and8-byte output rows");
}

std::array<std::array<std::int32_t, 2>, 8> firstPass(
    const std::int16_t* coefficients, const std::int16_t* quantizer) noexcept {
    std::array<std::array<std::int32_t, 2>, 8> intermediate;
    for (std::size_t column = 0; column < 2; ++column) {
        const auto dc = product(coefficients[column], quantizer[column]);
        const auto ac = product(coefficients[column + 2], quantizer[column + 8]);
        const auto transformed = twoFrequencies(dc, ac);
        for (std::size_t row = 0; row < 8; ++row)
            intermediate[row][column] = transformed[row];
    }
    return intermediate;
}

// Full8-frequency inverse butterfly from PAL233C68..233D9C and its second
// pass. Each rounding site is kept before the following sum/difference.
std::array<std::int32_t, 8> eightFrequencies(const std::array<std::int32_t, 8>& frequency) noexcept {
    const auto evenSum = sum(frequency[2], frequency[6]);
    const auto evenRotation = shifted(product(difference(frequency[2], frequency[6]), 362), 8);
    const auto evenDifference = difference(evenRotation, evenSum);
    const auto oddDifferenceA = difference(frequency[5], frequency[3]);
    const auto oddDifferenceB = difference(frequency[1], frequency[7]);
    const auto oddSumA = sum(frequency[5], frequency[3]);
    const auto oddSumB = sum(frequency[1], frequency[7]);
    const auto coupled = shifted(product(sum(oddDifferenceA, oddDifferenceB), 473), 8);
    const auto oddRotatedA = sum(shifted(product(oddDifferenceA, -669), 8), coupled);
    const auto oddRotatedB = difference(shifted(product(oddDifferenceB, 277), 8), coupled);
    const auto oddRotation = shifted(product(difference(oddSumB, oddSumA), 362), 8);
    const auto oddTotal = sum(oddSumA, oddSumB);
    const auto oddMiddleA = difference(oddRotatedA, oddTotal);
    const auto oddMiddleB = difference(oddRotation, oddMiddleA);
    const auto oddMiddleC = sum(oddRotatedB, oddMiddleB);
    const auto baseSum = sum(frequency[0], frequency[4]);
    const auto baseDifference = difference(frequency[0], frequency[4]);
    const auto even0 = sum(baseSum, evenSum);
    const auto even1 = sum(baseDifference, evenDifference);
    const auto even2 = difference(baseDifference, evenDifference);
    const auto even3 = difference(baseSum, evenSum);
    return {{sum(even0, oddTotal), sum(even1, oddMiddleA), sum(even2, oddMiddleB),
        difference(even3, oddMiddleC), sum(even3, oddMiddleC), difference(even2, oddMiddleB),
        difference(even1, oddMiddleA), difference(even0, oddTotal)}};
}

std::array<std::array<std::int32_t, 8>, 8> fullFirstPass(
    const std::int16_t* coefficients, const std::int16_t* quantizer) noexcept {
    std::array<std::array<std::int32_t, 8>, 8> intermediate;
    for (std::size_t column = 0; column < 8; ++column) {
        std::array<std::int32_t, 8> frequency;
        for (std::size_t row = 0; row < 8; ++row)
            frequency[row] = product(coefficients[row * 8 + column], quantizer[row * 8 + column]);
        const auto transformed = eightFrequencies(frequency);
        for (std::size_t row = 0; row < 8; ++row)
            intermediate[row][column] = transformed[row];
    }
    return intermediate;
}

std::int16_t signedHalf(std::uint32_t word) noexcept {
    const auto half = word & 0xffffu;
    return static_cast<std::int16_t>(half <= 0x7fffu ? static_cast<std::int32_t>(half) :
        static_cast<std::int32_t>(half) - 0x10000);
}

// The reference passes extract and sign-extend bits16..31 of EACH64-bit
// product, before the following sums. This is different from the AAN path's
// wrapped32-bit products followed by an8-bit arithmetic shift.
std::int32_t referenceProduct(std::int32_t value, std::int32_t cosine) noexcept {
    const auto wide = static_cast<std::int64_t>(value) * cosine;
    return signedHalf(static_cast<std::uint32_t>(static_cast<std::uint64_t>(wide) >> 16));
}

std::array<std::int32_t, 8> referenceFrequencies(const std::array<std::int32_t, 8>& x) noexcept {
    const auto h0 = referenceProduct(x[0], 23170);
    const auto h4 = referenceProduct(x[4], 23170);
    const auto evenPlus = sum(h0, h4);
    const auto evenMinus = difference(h0, h4);
    const auto a = sum(referenceProduct(x[2], 30274), referenceProduct(x[6], 12540));
    const auto b = difference(referenceProduct(x[6], 30274), referenceProduct(x[2], 12540));
    const auto p = sum(referenceProduct(x[1], 32138), referenceProduct(x[7], 6393));
    const auto q = sum(referenceProduct(x[3], 27246), referenceProduct(x[5], 18205));
    const auto r = difference(referenceProduct(x[5], 27246), referenceProduct(x[3], 18205));
    const auto s = difference(referenceProduct(x[7], 32138), referenceProduct(x[1], 6393));
    const auto rotatedP = referenceProduct(product(difference(p, q), 2), 23170);
    const auto rotatedR = referenceProduct(product(sum(r, s), 2), 23170);
    const auto oddSum = sum(p, q);
    const auto oddDifference = difference(s, r);
    const auto b0 = sum(evenPlus, a);
    const auto b1 = sum(evenMinus, rotatedP);
    const auto b2 = difference(evenMinus, rotatedP);
    const auto b3 = difference(evenPlus, a);
    const auto c0 = sum(rotatedR, b);
    const auto c1 = difference(b, rotatedR);
    return {{sum(b0, oddSum), difference(b1, c0), sum(b1, c0), difference(b3, oddDifference),
        sum(b3, oddDifference), sum(b2, c1), difference(b2, c1), difference(b0, oddSum)}};
}

struct ButterflyRange {
    std::int32_t minimum = std::numeric_limits<std::int32_t>::max();
    std::int32_t maximum = std::numeric_limits<std::int32_t>::min();
    void Include(std::int32_t value) noexcept {
        // The source's maximum test is in the ELSE path of the minimum test.
        // Keep that order, including the first/new-minimum observations.
        if (value < minimum) minimum = value;
        else if (maximum < value) maximum = value;
    }
};

// Mode0 observes the actual first-pass butterfly intermediates, in source
// order. These are persistent CPU diagnostics, not PS2 cache/transport work.
void recordButterflyRange(const std::array<std::int32_t, 8>& x,
                          const std::array<std::int32_t, 8>& transformed,
                          ButterflyRange& range) noexcept {
    for (auto i : {0u, 2u, 4u, 6u}) range.Include(x[i]);
    const auto baseSum = sum(x[0], x[4]);
    const auto baseDifference = difference(x[0], x[4]);
    range.Include(baseSum); range.Include(baseDifference);
    const auto evenSum = sum(x[2], x[6]);
    const auto evenDifference = difference(shifted(product(difference(x[2], x[6]), 362), 8), evenSum);
    range.Include(evenSum); range.Include(evenDifference);
    range.Include(sum(baseSum, evenSum)); range.Include(sum(baseDifference, evenDifference));
    range.Include(difference(baseDifference, evenDifference)); range.Include(difference(baseSum, evenSum));
    for (auto i : {1u, 3u, 5u, 7u}) range.Include(x[i]);
    const auto differenceA = difference(x[5], x[3]);
    const auto differenceB = difference(x[1], x[7]);
    const auto sumA = sum(x[5], x[3]);
    const auto sumB = sum(x[1], x[7]);
    range.Include(differenceA); range.Include(sumB); range.Include(differenceB); range.Include(sumA);
    const auto total = sum(sumA, sumB);
    const auto rotation = shifted(product(difference(sumB, sumA), 362), 8);
    range.Include(total); range.Include(rotation);
    const auto coupled = shifted(product(sum(differenceA, differenceB), 473), 8);
    const auto rotatedB = difference(shifted(product(differenceB, 277), 8), coupled);
    const auto rotatedA = sum(shifted(product(differenceA, -669), 8), coupled);
    range.Include(coupled); range.Include(rotatedB); range.Include(rotatedA);
    const auto middleA = difference(rotatedA, total);
    const auto middleB = difference(rotation, middleA);
    const auto middleC = sum(rotatedB, middleB);
    range.Include(middleC); range.Include(middleB); range.Include(middleA);
    for (auto value : transformed) range.Include(value);
}
} // namespace

// PAL232498..2328B0. Every input and quantizer is read before the first
// in-place coefficient store, including the supported same-buffer case.
void DCTImage::IDCTRowReference(std::int16_t* coefficients, const std::int16_t* quantizer) const {
    if (coefficients == nullptr || quantizer == nullptr)
        throw std::invalid_argument("MV3 reference row requires8 coefficients and quantizers");
    std::array<std::int32_t, 8> frequency;
    for (std::size_t i = 0; i < 8; ++i) frequency[i] = product(coefficients[i], quantizer[i]);
    const auto transformed = referenceFrequencies(frequency);
    for (auto i : {7u, 0u, 1u, 2u, 3u, 4u, 5u, 6u})
        coefficients[i] = signedHalf(static_cast<std::uint32_t>(transformed[i]));
}

// PAL2328B0..232D5C. Source SRAV masks the current mutable shift to5bits;
// each pixel uses the same current centered clamp pointer.
void DCTImage::IDCTColReference(const std::int16_t* coefficients, std::uint8_t* output,
                               std::int32_t pitch) const {
    if (coefficients == nullptr || output == nullptr || pitch < 1)
        throw std::invalid_argument("MV3 reference column requires8 pitched output bytes");
    std::array<std::int32_t, 8> frequency;
    for (std::size_t i = 0; i < 8; ++i) frequency[i] = coefficients[i * 8];
    const auto transformed = referenceFrequencies(frequency);
    for (std::size_t i = 0; i < 8; ++i)
        output[i * static_cast<std::size_t>(pitch)] = clipped(shifted(transformed[i],
            static_cast<std::uint32_t>(movieDctBlockState.referenceShift) & 31u), true);
}

// PAL232D60..233950. Mode1 is the actual in-place reference implementation;
// all other modes use the integer8-frequency transform. Mode0's packed
// halfword products are the same64 signed products, with the original
// intermediate-range observations and persistent stores retained below.
void DCTImage::IDCT(std::int16_t* coefficients, const std::int16_t* quantizer,
                    std::uint8_t* output, std::int32_t pitch) const {
    requireBlock(coefficients, quantizer, output, pitch);
    const auto mode = movieDctBlockState.mode;
    if (mode == 1) {
        for (std::size_t row = 0; row < 8; ++row)
            IDCTRowReference(coefficients + row * 8, quantizer + row * 8);
        for (std::size_t column = 0; column < 8; ++column)
            IDCTColReference(coefficients + column, output + column, pitch);
        return;
    }
    std::array<std::array<std::int32_t, 8>, 8> intermediate;
    ButterflyRange range;
    for (std::size_t column = 0; column < 8; ++column) {
        std::array<std::int32_t, 8> frequency;
        for (std::size_t row = 0; row < 8; ++row)
            frequency[row] = product(coefficients[row * 8 + column], quantizer[row * 8 + column]);
        const auto transformed = eightFrequencies(frequency);
        if (mode == 0) recordButterflyRange(frequency, transformed, range);
        for (std::size_t row = 0; row < 8; ++row) intermediate[row][column] = transformed[row];
    }
    if (mode == 0) {
        if (range.minimum < movieDctBlockState.observedMinimum) movieDctBlockState.observedMinimum = range.minimum;
        if (movieDctBlockState.observedMaximum < range.maximum) movieDctBlockState.observedMaximum = range.maximum;
    }
    for (const auto& row : intermediate) {
        const auto transformed = eightFrequencies(row);
        for (std::size_t column = 0; column < 8; ++column)
            output[column] = clipped(shifted(transformed[column], 5), true);
        output += pitch;
    }
}

// PAL233950..2339A4, one coded coefficient -> the actual centered constant8x8.
void DCTImage::IDCT1(const std::int16_t* coefficients, const std::int16_t* quantizer,
                     std::uint8_t* output, std::int32_t pitch) const {
    requireBlock(coefficients, quantizer, output, pitch);
    const auto dc = shifted(product(coefficients[0], quantizer[0]), 5);
    const std::uint8_t value = clipped(dc, true);
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t column = 0; column < 8; ++column)
            output[column] = value;
        output += pitch;
    }
}

// PAL2339A8..233C1C. Actual compact four coefficients, both integer passes;
// the current output plane is replaced through the centered clamp lookup.
void DCTImage::IDCT4(const std::int16_t* coefficients, const std::int16_t* quantizer,
                     std::uint8_t* output, std::int32_t pitch) const {
    requireBlock(coefficients, quantizer, output, pitch);
    const auto intermediate = firstPass(coefficients, quantizer);
    for (const auto& row : intermediate) {
        const auto pixels = twoFrequencies(row[0], row[1]);
        for (std::size_t column = 0; column < 8; ++column)
            output[column] = clipped(shifted(pixels[column], 5), true);
        output += pitch;
    }
}

// PAL233FA0..23408C. Add the signed DC residual to actual existing pixels;
// use the non-centered clamp pointer, not the neutral128 image initializer.
void DCTImage::IDCTAdd1(const std::int16_t* coefficients, const std::int16_t* quantizer,
                        std::uint8_t* output, std::int32_t pitch) const {
    requireBlock(coefficients, quantizer, output, pitch);
    const auto dc = shifted(product(coefficients[0], quantizer[0]), 5);
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t column = 0; column < 8; ++column)
            output[column] = clipped(sum(dc, output[column]), false);
        output += pitch;
    }
}

// PAL234090..2343D8. Same two-frequency transform, source residual rounding,
// and the existing previous/motion-compensated plane as the sole pixel input.
void DCTImage::IDCTAdd4(const std::int16_t* coefficients, const std::int16_t* quantizer,
                        std::uint8_t* output, std::int32_t pitch) const {
    requireBlock(coefficients, quantizer, output, pitch);
    const auto intermediate = firstPass(coefficients, quantizer);
    for (const auto& row : intermediate) {
        const auto residual = twoFrequencies(row[0], row[1]);
        for (std::size_t column = 0; column < 8; ++column) {
            const auto delta = shifted(residual[column], 5);
            output[column] = clipped(sum(delta, output[column]), false);
        }
        output += pitch;
    }
}

// PAL233C20..233F9C. Full64 coefficients, complete quantized two-pass inverse,
// then signed residuals added to the one existing decoded/motion plane.
void DCTImage::IDCTAdd(const std::int16_t* coefficients, const std::int16_t* quantizer,
                       std::uint8_t* output, std::int32_t pitch) const {
    requireBlock(coefficients, quantizer, output, pitch);
    const auto intermediate = fullFirstPass(coefficients, quantizer);
    for (const auto& row : intermediate) {
        const auto residual = eightFrequencies(row);
        for (std::size_t column = 0; column < 8; ++column) {
            const auto delta = shifted(residual[column], 5);
            output[column] = clipped(sum(delta, output[column]), false);
        }
        output += pitch;
    }
}

} // namespace hp2
