#include "Huffman2.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace hp2 {

// Actual final-PAL 2BDC48, twelve {code, length} pairs. SHF_WRITE is retained;
// BuildDecodeTab consumes the current entries, not a hidden immutable copy.
std::array<HuffmanCodeEntry, 12> movieHuffmanCodes{{
    {0, 2}, {1, 2}, {4, 3}, {5, 3}, {6, 3}, {14, 4},
    {30, 5}, {62, 6}, {126, 7}, {254, 8}, {510, 9}, {511, 9}
}};
std::array<HuffmanDecodeEntry, 512> movieHuffmanDecodeTable{};

// PAL 236EE8..236F14.
std::uint32_t Huffman::NumBits(std::uint32_t value) const noexcept {
    std::uint32_t count = 0;
    while (value != 0) {
        value >>= 1;
        ++count;
    }
    return count;
}

// PAL 236F18..236F48. R5900 variable shifts use the low five count bits.
void Huffman::STORE(std::uint32_t value, std::uint32_t count) {
    if (count > 32 || bitCount_ < 0 ||
        count + static_cast<std::uint32_t>(bitCount_) > 32)
        throw std::invalid_argument("MV3 Huffman invalid bit-store extent");
    bitCount_ += static_cast<std::int32_t>(count);
    totalBits_ += count;
    bits_ |= value << ((0u - static_cast<std::uint32_t>(bitCount_)) & 31u);
}

// Native bounded output adaptation; the original byte-emission loop is shared
// by Store, Compress and EndCompress without changing its state-update order.
void Huffman::WriteByte() {
    if (outputAt_ >= outputCapacity_ || output_ == nullptr)
        throw std::out_of_range("MV3 Huffman compressed output is full");
    output_[outputAt_] = static_cast<std::uint8_t>(bits_ >> 24);
    ++outputAt_;
    bits_ <<= 8;
    bitCount_ -= 8;
}

void Huffman::FlushBytes() {
    while (bitCount_ >= 8)
        WriteByte();
}

// PAL 236F48..236FB4.
void Huffman::Store(std::uint32_t value, std::uint32_t count) {
    STORE(value, count);
    FlushBytes();
}

// PAL 236FB8..236FCC; capacity is an explicit native span boundary.
void Huffman::InitCompress(std::uint8_t* output, std::size_t capacity) {
    if (capacity != 0 && output == nullptr)
        throw std::invalid_argument("MV3 Huffman missing output storage");
    output_ = output;
    outputCapacity_ = capacity;
    outputAt_ = 0;
    bitCount_ = 0;
    totalBits_ = 0;
    bits_ = 0;
}

// PAL 236FD0..2371DC: signed coefficients, actual prefix codes and runs of
// 1..72 zeros. This is the game's algorithm, not a JPEG/library adapter.
void Huffman::Compress(const std::int16_t* input, std::uint32_t count) {
    if (count != 0 && input == nullptr)
        throw std::invalid_argument("MV3 Huffman missing coefficients");
    remaining_ = count;
    if (count == 0)
        return;
    remaining_ = count - 1;
    do {
        std::int32_t coefficient = *input++;
        if (coefficient == 0) {
            STORE(movieHuffmanCodes[0].code, movieHuffmanCodes[0].length);
            std::uint32_t extraZeros = 0;
            while (extraZeros < remaining_ && extraZeros < 71 &&
                   input[extraZeros] == 0)
                ++extraZeros;
            if (extraZeros >= 8) {
                STORE(1, 1);
                STORE(extraZeros - 8, 6);
            } else {
                STORE(0, 1);
                STORE(extraZeros, 3);
            }
            input += extraZeros;
            remaining_ -= extraZeros;
        } else {
            if (coefficient == -2048)
                coefficient = -2047;
            const auto magnitude = static_cast<std::uint32_t>(
                coefficient < 0 ? -coefficient : coefficient);
            const std::uint32_t category = NumBits(magnitude);
            if (category >= movieHuffmanCodes.size())
                throw std::out_of_range("MV3 coefficient exceeds the authored codebook");
            const auto& prefix = movieHuffmanCodes[category];
            const std::uint32_t sign = coefficient < 0 ? 1u : 0u;
            const std::uint32_t value =
                (static_cast<std::uint32_t>(prefix.code) << category) |
                (sign << (category - 1)) |
                (magnitude - (1u << (category - 1)));
            STORE(value, category + prefix.length);
        }
        FlushBytes();
    } while (remaining_-- != 0);
}

// PAL 2371E0..237228. The <= boundary is intentional: even an aligned or
// empty stream emits one final byte; the result is the actual bit count.
std::uint32_t Huffman::EndCompress() {
    while (bitCount_ >= 0)
        WriteByte();
    return totalBits_;
}

// Original GetBits/Decompress refill until at least 25 bits are available.
// The caller must retain the real padded input extent, including lookahead.
void Huffman::Refill() {
    while (bitCount_ < 25) {
        if (bitCount_ < 0 || inputAt_ >= inputBytes_ || input_ == nullptr)
            throw std::out_of_range("MV3 Huffman truncated input/lookahead");
        bits_ |= static_cast<std::uint32_t>(input_[inputAt_]) <<
                 ((24u - static_cast<std::uint32_t>(bitCount_)) & 31u);
        ++inputAt_;
        bitCount_ += 8;
    }
}

// PAL 237228..2372A4.
std::uint32_t Huffman::GetBits(std::uint32_t count) {
    Refill();
    return GETBITS(count);
}

// PAL 2372A8..237414. This writes the same shared 512-entry table that is
// read by Decompress, including -128 escape entries for the long categories.
void Huffman::BuildDecodeTab() {
    for (std::uint32_t category = 0; category < 12; ++category) {
        const auto& prefix = movieHuffmanCodes[category];
        if (prefix.length > 9)
            throw std::out_of_range("MV3 Huffman invalid codebook prefix");
        const std::uint32_t alternatives = 1u << category;
        const std::uint32_t lowerMask = (alternatives - 1) >> 1;
        for (std::uint32_t suffix = 0; suffix < alternatives; ++suffix) {
            std::uint32_t pattern = static_cast<std::uint32_t>(prefix.code) << category;
            std::uint32_t length = category + prefix.length;
            std::int32_t value;
            if (length >= 10) {
                length = prefix.length;
                pattern >>= category;
                value = -128;
            } else {
                pattern |= suffix;
                if (category == 0)
                    value = 0;
                else {
                    const auto magnitude = static_cast<std::int32_t>(
                        (suffix & lowerMask) + (1u << (category - 1)));
                    value = suffix >= alternatives / 2 ? -magnitude : magnitude;
                }
            }
            for (std::uint32_t entry = 0; entry < 512; ++entry) {
                if ((entry >> (9 - length)) == pattern) {
                    movieHuffmanDecodeTable[entry].consumedBits =
                        static_cast<std::uint8_t>(value == -128 ? category - 1 : length);
                    movieHuffmanDecodeTable[entry].value = static_cast<std::int8_t>(value);
                }
            }
        }
    }
}

// PAL 237418..237428. No implicit decoder/table initialization is inserted.
void Huffman::InitDecompress(const std::uint8_t* input, std::size_t bytes) {
    if (bytes != 0 && input == nullptr)
        throw std::invalid_argument("MV3 Huffman missing input storage");
    input_ = input;
    inputBytes_ = bytes;
    inputAt_ = 0;
    bitCount_ = 0;
    bits_ = 0;
}

// PAL 237428..23765C. Nine-bit lookup, long signed magnitudes and both zero
// run forms are decoded directly, preserving the source prefetch policy.
void Huffman::Decompress(std::int16_t* output, std::uint32_t count) {
    if (count != 0 && output == nullptr)
        throw std::invalid_argument("MV3 Huffman missing output coefficients");
    std::uint32_t at = 0;
    while (at < count) {
        Refill();
        const auto entry = movieHuffmanDecodeTable[bits_ >> 23];
        if (entry.value == -128) {
            const std::uint32_t categoryMinusOne = entry.consumedBits;
            if (categoryMinusOne + 1 >= movieHuffmanCodes.size())
                throw std::out_of_range("MV3 Huffman invalid long category");
            const auto prefixLength = movieHuffmanCodes[categoryMinusOne + 1].length;
            bits_ <<= prefixLength & 31u;
            bitCount_ -= prefixLength;
            const std::uint32_t negative = GETBITS(1);
            const std::uint32_t magnitude =
                (bits_ >> ((0u - categoryMinusOne) & 31u)) + (1u << categoryMinusOne);
            bits_ <<= categoryMinusOne & 31u;
            bitCount_ -= static_cast<std::int32_t>(categoryMinusOne);
            output[at++] = static_cast<std::int16_t>(negative != 0 ?
                -static_cast<std::int32_t>(magnitude) : static_cast<std::int32_t>(magnitude));
        } else if (entry.value == 0) {
            bits_ <<= 2;
            bitCount_ -= 2;
            const bool longRun = GETBITS(1) != 0;
            const std::uint32_t run = longRun ? GETBITS(6) + 9 : GETBITS(3) + 1;
            if (run > count - at)
                throw std::out_of_range("MV3 Huffman zero run exceeds coefficient output");
            std::fill_n(output + at, run, static_cast<std::int16_t>(0));
            at += run;
        } else {
            output[at++] = entry.value;
            bits_ <<= entry.consumedBits & 31u;
            bitCount_ -= entry.consumedBits;
        }
    }
}

// PAL 237660..237684: consume from an already-filled word, without refill.
std::uint32_t Huffman::GETBITS(std::uint32_t count) {
    if (count > 31 || static_cast<std::int32_t>(count) > bitCount_)
        throw std::out_of_range("MV3 Huffman invalid buffered-bit request");
    const std::uint32_t result = bits_ >> ((0u - count) & 31u);
    bits_ <<= count & 31u;
    bitCount_ -= static_cast<std::int32_t>(count);
    return result;
}

} // namespace hp2
