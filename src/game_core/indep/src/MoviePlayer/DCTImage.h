#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace hp2 {

// Actual final-PAL quantizer templates; mutable native storage is shared by
// every image. Their contents deliberately differ from standard JPEG tables.
extern std::array<std::int16_t, 64> movieLumaQuantizer;
extern std::array<std::int16_t, 64> movieChromaQuantizer;

class DCTTable final {
public:
    static void ScaleTable(std::int32_t scale, std::int16_t* table);
    static std::int32_t ConvertQualityToScale(std::int32_t quality) noexcept;
    static void GenerateLuma(std::int32_t quality, std::int16_t* table);
    static void GenerateChroma(std::int32_t quality, std::int16_t* table);
};

extern std::array<std::uint32_t, 64> movieZigzagOrder;
extern std::array<std::uint32_t, 64> movieInverseZigzagOrder;
extern std::array<std::int16_t, 64> movieIdctQuantizerScale;

// One mutable projection of PAL331CB0..331CBF. Descriptive native names,
// not recovered variable symbols: the original reference shift/mode both
// start at2, while the packed path's persistent extrema start at0.
struct DCTBlockState {
    std::int32_t referenceShift;
    std::int32_t mode;
    std::int32_t observedMaximum;
    std::int32_t observedMinimum;
};
extern DCTBlockState movieDctBlockState;

extern std::array<std::uint32_t, 4> movieBlockCoefficientCounts;

struct DCTFrameView {
    std::uint8_t* y;
    std::uint8_t* u;
    std::uint8_t* v;
    std::uint8_t* blockData;
    std::size_t planeBytes;
    std::size_t blockCount;
};

// One image owns both frame allocations and all decode workspaces. Views
// borrow that storage; they never retain a second decoded plane catalogue.
class DCTImage final {
public:
    DCTImage();
    ~DCTImage();
    DCTImage(const DCTImage&) = delete;
    DCTImage& operator=(const DCTImage&) = delete;
    DCTImage(DCTImage&&) = delete;
    DCTImage& operator=(DCTImage&&) = delete;
    void ReleaseAll() noexcept;
    void Init(std::int32_t width, std::int32_t height, bool unused,
              std::int32_t initializationOption);
    void InitIDCTTables(std::int8_t quality, std::int8_t qualityDelta);
    void SwapPrevBuffer() noexcept;
    void IDCTZero(std::uint8_t* output, std::int32_t pitch) const;
    void IDCTRowReference(std::int16_t* coefficients, const std::int16_t* quantizer) const;
    void IDCTColReference(const std::int16_t* coefficients, std::uint8_t* output,
                          std::int32_t pitch) const;
    void IDCT(std::int16_t* coefficients, const std::int16_t* quantizer,
              std::uint8_t* output, std::int32_t pitch) const;
    void IDCT1(const std::int16_t* coefficients, const std::int16_t* quantizer,
               std::uint8_t* output, std::int32_t pitch) const;
    void IDCT4(const std::int16_t* coefficients, const std::int16_t* quantizer,
               std::uint8_t* output, std::int32_t pitch) const;
    void IDCTAdd1(const std::int16_t* coefficients, const std::int16_t* quantizer,
                  std::uint8_t* output, std::int32_t pitch) const;
    void IDCTAdd4(const std::int16_t* coefficients, const std::int16_t* quantizer,
                  std::uint8_t* output, std::int32_t pitch) const;
    void IDCTAdd(const std::int16_t* coefficients, const std::int16_t* quantizer,
                 std::uint8_t* output, std::int32_t pitch) const;
    void IDCTImageData(std::uint8_t* output, std::int32_t pitch,
                       const std::int16_t* coded, const std::int16_t* quantizer) const;
    void IDCTImageData1(std::uint8_t* output, std::int32_t pitch,
                        const std::int16_t* coded, const std::int16_t* quantizer) const;
    void IDCTImageData4(std::uint8_t* output, std::int32_t pitch,
                        const std::int16_t* coded, const std::int16_t* quantizer) const;
    void IDCTAddImageData(std::uint8_t* output, std::int32_t pitch,
                          const std::int16_t* coded, const std::int16_t* quantizer) const;
    void IDCTAddImageData1(std::uint8_t* output, std::int32_t pitch,
                           const std::int16_t* coded, const std::int16_t* quantizer) const;
    void IDCTAddImageData4(std::uint8_t* output, std::int32_t pitch,
                           const std::int16_t* coded, const std::int16_t* quantizer) const;
    void ToImage32(std::uint8_t* output, std::uint32_t flags) const;
    void ToImage32_5P(std::uint8_t* output, std::uint32_t unusedFlags) const;
    void CopyMotionBlocks(std::uint8_t* planarOutput) const;
    void CopyPreviousFrameBlock(std::int32_t x, std::int32_t y,
                                std::uint8_t* outputY, std::uint8_t* outputU,
                                std::uint8_t* outputV, std::int32_t motionIndex) const;
    void ToImage(std::uint8_t* output, std::size_t outputBytes,
                  std::int8_t quality, std::int8_t qualityDelta, std::uint32_t flags);

    // Native borrowed views/accessors: no original inline/getter identities.
    DCTFrameView Current() const noexcept;
    DCTFrameView Previous() const noexcept;
    std::int32_t Width() const noexcept;
    std::int32_t Height() const noexcept;
    const std::array<std::array<std::int16_t, 64>, 4>& Quantizers() const noexcept;
    // Borrow the same retained coded frame. readableBytes includes actual
    // supplied lookahead bytes; this adapter never manufactures zero padding.
    void BindEncodedData(const std::uint8_t* input, std::size_t readableBytes);

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

} // namespace hp2
