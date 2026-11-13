#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace hp2 {

struct HuffmanCodeEntry {
    std::uint16_t code;
    std::uint16_t length;
};
struct HuffmanDecodeEntry {
    std::uint8_t consumedBits;
    std::int8_t value;
};

// One mutable codebook and one mutable, actually generated decode table.
// These native names do not claim original variable/debug identities.
extern std::array<HuffmanCodeEntry, 12> movieHuffmanCodes;
extern std::array<HuffmanDecodeEntry, 512> movieHuffmanDecodeTable;

class Huffman final {
public:
    std::uint32_t NumBits(std::uint32_t value) const noexcept;
    void STORE(std::uint32_t value, std::uint32_t count);
    void Store(std::uint32_t value, std::uint32_t count);
    void InitCompress(std::uint8_t* output, std::size_t capacity);
    void Compress(const std::int16_t* input, std::uint32_t count);
    std::uint32_t EndCompress();
    std::uint32_t GetBits(std::uint32_t count);
    void BuildDecodeTab();
    void InitDecompress(const std::uint8_t* input, std::size_t bytes);
    void Decompress(std::int16_t* output, std::uint32_t count);
    std::uint32_t GETBITS(std::uint32_t count);

    // Bounded native byte-view diagnostics, not additional original methods.
    std::size_t BytesWritten() const noexcept { return outputAt_; }
    std::size_t BytesRead() const noexcept { return inputAt_; }

private:
    void FlushBytes();
    void Refill();
    void WriteByte();
    std::uint32_t bits_ = 0;
    std::int32_t bitCount_ = 0;
    std::uint32_t totalBits_ = 0;
    std::uint32_t remaining_ = 0;
    std::uint8_t* output_ = nullptr;
    std::size_t outputCapacity_ = 0;
    std::size_t outputAt_ = 0;
    const std::uint8_t* input_ = nullptr;
    std::size_t inputBytes_ = 0;
    std::size_t inputAt_ = 0;
};

} // namespace hp2
