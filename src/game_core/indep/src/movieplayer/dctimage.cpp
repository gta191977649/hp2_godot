#include "DCTImage.h"
#include "MV3Codec.h"
#include "Huffman2.h"

#include <algorithm>
#include <limits>
#include <cstring>
#include <new>
#include <stdexcept>
#include <utility>

namespace hp2 {
// Final PAL2BDB08. The same source table controls prediction and pointer
// advancement for every coded block, separate from the two-bit kind value.
std::array<std::uint32_t, 4> movieBlockCoefficientCounts{{0, 1, 4, 64}};

// Final PAL31C9A0. Each natural-order coefficient maps to its coded zigzag
// position. Every actual image constructor rebuilds the shared inverse map.
std::array<std::uint32_t, 64> movieZigzagOrder{{
    0,1,5,6,14,15,27,28, 2,3,7,13,16,26,29,42,
    4,8,12,17,25,30,41,43, 9,11,18,24,31,40,44,53,
    10,19,23,32,39,45,52,54, 20,22,33,38,46,51,55,60,
    21,34,37,47,50,56,59,61, 35,36,48,49,57,58,62,63
}};
std::array<std::uint32_t, 64> movieInverseZigzagOrder{};

// Final PAL2BDA88, the actual AAN quantizer scaling cells. Mutable once,
// shared by all four image tables; no imported beta/JPEG scaling defaults.
std::array<std::int16_t, 64> movieIdctQuantizerScale{{
    16384,22725,21407,19266,16384,12873,8867,4520,
    22725,31521,29692,26722,22725,17855,12299,6270,
    21407,29692,27969,25172,21407,16819,11585,5906,
    19266,26722,25172,22654,19266,15137,10426,5315,
    16384,22725,21407,19266,16384,12873,8867,4520,
    12873,17855,16819,15137,12873,10114,6967,3552,
    8867,12299,11585,10426,8867,6967,4799,2446,
    4520,6270,5906,5315,4520,3552,2446,1247
}};

namespace {
struct FrameDeleter {
    void operator()(std::uint8_t* bytes) const noexcept {
        ::operator delete[](bytes, std::align_val_t(64));
    }
};
using FrameAllocation = std::unique_ptr<std::uint8_t[], FrameDeleter>;

std::int16_t scaledQuantizer(std::int16_t value, std::int16_t scale) {
    const std::int32_t product = static_cast<std::int32_t>(value) * scale + 2048;
    const std::int32_t shifted = product >= 0 ? product / 4096 :
        -1 - static_cast<std::int32_t>(static_cast<std::uint32_t>(~product) / 4096);
    const auto low = static_cast<std::uint16_t>(static_cast<std::uint32_t>(shifted));
    return low <= 32767 ? static_cast<std::int16_t>(low) :
        static_cast<std::int16_t>(-1 - static_cast<std::int32_t>(low ^ 65535u));
}

std::uint8_t moviePixel(std::int32_t value) {
    const auto& state = movieCodecShared;
    const auto base = reinterpret_cast<std::uintptr_t>(state.clamp.data());
    const auto address = static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(state.clampZero)) + value;
    if (address < static_cast<std::int64_t>(base) || address >= static_cast<std::int64_t>(base + state.clamp.size()))
        throw std::out_of_range("MV3 converted pixel exceeds shared clamp storage");
    return *reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(address));
}

// Actual source dependency2361D0: pack8x4 tiles in scan order. This is an
// internal native factor of the individually proved5P converter, count0.
void packPlaneTiles(std::uint8_t* output, const std::uint8_t* input,
                    std::int32_t width, std::int32_t height) {
    for (std::int32_t y = 0; y < height; y += 4) {
        for (std::int32_t x = 0; x < width; x += 8) {
            for (std::int32_t row = 0; row < 4; ++row)
                std::memcpy(output + row * 8, input + static_cast<std::ptrdiff_t>(y + row) * width + x, 8);
            output += 32;
        }
    }
}
} // namespace

struct DCTImage::Storage {
    FrameAllocation current;
    FrameAllocation previous;
    std::unique_ptr<std::uint8_t[]> blockKinds;
    std::unique_ptr<std::int16_t[]> coefficients;
    // The source's shared64-cell dezigzag scratch is algorithmic storage;
    // give it the same native lifetime as the image using it. Unconsumed
    // per-block console profiling banks and timestamp reads are unnecessary.
    std::array<std::int16_t,64> dezigzag{};
    std::size_t planeBytes = 0;
    std::size_t allocationBytes = 0;
    std::size_t blockCount = 0;
    std::size_t coefficientCount = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t pixelCount = 0;
    std::int32_t initializationOption = 0;
    std::int32_t frameState = 0;
    const std::uint8_t* encoded = nullptr;
    std::size_t encodedBytes = 0;
    // Source does not initialize these until InitIDCTTables. The native
    // initial zeros are not a claim about source-unwritten object storage.
    std::array<std::array<std::int16_t, 64>, 4> quantizers{};

    DCTFrameView View(const FrameAllocation& frame) const noexcept {
        if (!frame)
            return {nullptr, nullptr, nullptr, nullptr, 0, 0};
        return {frame.get(), frame.get() + planeBytes,
            frame.get() + planeBytes * 2, frame.get() + planeBytes * 3,
            planeBytes, blockCount};
    }
};

// PAL2343D8..234468. Native owning object state replaces pointer-overlay ABI.
DCTImage::DCTImage() : storage_(new Storage) {
    for (std::uint32_t coefficient = 0; coefficient < 64; ++coefficient) {
        const auto coded = movieZigzagOrder[coefficient];
        if (coded >= movieInverseZigzagOrder.size())
            throw std::out_of_range("MV3 zigzag position exceeds64 entries");
        movieInverseZigzagOrder[coded] = coefficient;
    }
}

// PAL234468..2344B4, actual embedded destruction plus native object lifetime.
DCTImage::~DCTImage() {
    ReleaseAll();
}

// PAL2344B8..23455C. Dimensions/options are deliberately retained, as source
// clears allocations/work pointers but does not reset its44/48/4C/50/58 cells.
void DCTImage::ReleaseAll() noexcept {
    auto& state = *storage_;
    state.current.reset();
    state.previous.reset();
    state.blockKinds.reset();
    state.coefficients.reset();
    state.coefficientCount = 0;
    state.frameState = 0;
}

// PAL234560..234714. Two actual64-byte-aligned contiguous Y/U/V allocations,
// each followed by one word per16x16 block, and the separate two-bit kind and
// one-row coefficient workspaces. `unused` is genuinely unused in this body.
void DCTImage::Init(std::int32_t width, std::int32_t height, bool unused,
                    std::int32_t initializationOption) {
    static_cast<void>(unused);
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("MV3 image requires positive dimensions");
    const std::uint64_t pixels = static_cast<std::uint64_t>(width) *
                                 static_cast<std::uint64_t>(height);
    const std::uint64_t plane = (pixels + 127u) & ~std::uint64_t(127u);
    const std::uint64_t blocks = static_cast<std::uint64_t>(width / 16) *
                                 static_cast<std::uint64_t>(height / 16);
    const std::uint64_t bytes = plane * 3 + blocks * 4;
    const std::uint64_t coefficients = static_cast<std::uint64_t>(width / 16) * 384;
    if (bytes > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ||
        pixels > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("MV3 image exceeds source valid allocation extent");

    ReleaseAll();
    auto& state = *storage_;
    state.width = width;
    state.height = height;
    state.pixelCount = static_cast<std::int32_t>(pixels);
    state.initializationOption = initializationOption;
    state.blockCount = static_cast<std::size_t>(blocks);
    state.planeBytes = static_cast<std::size_t>(plane);
    state.allocationBytes = static_cast<std::size_t>(bytes);
    state.current.reset(static_cast<std::uint8_t*>(
        ::operator new[](state.allocationBytes, std::align_val_t(64))));
    state.previous.reset(static_cast<std::uint8_t*>(
        ::operator new[](state.allocationBytes, std::align_val_t(64))));
    std::fill_n(state.current.get(), state.allocationBytes, static_cast<std::uint8_t>(0));
    std::fill_n(state.previous.get(), state.allocationBytes, static_cast<std::uint8_t>(0));
    state.blockKinds.reset(new std::uint8_t[(state.blockCount + 3) / 4]);
    state.coefficientCount = static_cast<std::size_t>(coefficients);
    state.coefficients.reset(new std::int16_t[state.coefficientCount]);
    state.frameState = 0;
}

// PAL234718..234834. Four authored64-cell tables: Y(q),Y(q+d),UV(q),UV(q+d),
// followed by the exact multiply/+2048/arithmetic-shift12/low16 operations.
void DCTImage::InitIDCTTables(std::int8_t quality, std::int8_t qualityDelta) {
    auto& tables = storage_->quantizers;
    const std::int32_t first = quality;
    const std::int32_t second = first + qualityDelta;
    DCTTable::GenerateLuma(first, tables[0].data());
    DCTTable::GenerateLuma(second, tables[1].data());
    DCTTable::GenerateChroma(first, tables[2].data());
    DCTTable::GenerateChroma(second, tables[3].data());
    for (std::size_t coefficient = 0; coefficient < 64; ++coefficient) {
        const std::int16_t scale = movieIdctQuantizerScale[coefficient];
        for (auto& table : tables)
            table[coefficient] = scaledQuantizer(table[coefficient], scale);
    }
}

// PAL236638..23668C. Swap allocation ownership, so every previously borrowed
// plane keeps its address and the current/previous roles change together.
void DCTImage::SwapPrevBuffer() noexcept {
    std::swap(storage_->current, storage_->previous);
}

// PAL234A68..234A9C, including the actual neutral128 fill and pitched rows.
void DCTImage::IDCTZero(std::uint8_t* output, std::int32_t pitch) const {
    if (output == nullptr || pitch < 8)
        throw std::invalid_argument("MV3 zero block requires8-byte output rows");
    for (std::int32_t row = 0; row < 8; ++row) {
        std::fill_n(output, 8, static_cast<std::uint8_t>(128));
        output += pitch;
    }
}

// PAL234838..234914. Retain the dezigzag algorithm and current inverse map;
// the native image owns its scratch, with no unused per-block profiling.
void DCTImage::IDCTImageData(std::uint8_t* output, std::int32_t pitch,
                            const std::int16_t* coded, const std::int16_t* quantizer) const {
    if (coded == nullptr) throw std::invalid_argument("MV3 full block requires64 coded coefficients");
    auto& workspace = storage_->dezigzag;
    for (std::size_t i = 0; i < 64; ++i) {
        const auto natural = movieInverseZigzagOrder[i];
        if (natural >= workspace.size()) throw std::out_of_range("MV3 inverse zigzag exceeds64 cells");
        workspace[natural] = coded[i];
    }
    IDCT(workspace.data(), quantizer, output, pitch);
}

// PAL234918..2349C0. Sparse encoded coefficients have their own actual
// table shape, so they do not use the full dezigzag loop.
void DCTImage::IDCTImageData1(std::uint8_t* output, std::int32_t pitch,
                             const std::int16_t* coded, const std::int16_t* quantizer) const {
    IDCT1(coded, quantizer, output, pitch);
}

// PAL2349C0..234A68, same actual sparse4 kernel.
void DCTImage::IDCTImageData4(std::uint8_t* output, std::int32_t pitch,
                             const std::int16_t* coded, const std::int16_t* quantizer) const {
    IDCT4(coded, quantizer, output, pitch);
}

// PAL234AA0..234AF4, same image-owned workspace and full residual tail.
void DCTImage::IDCTAddImageData(std::uint8_t* output, std::int32_t pitch,
                               const std::int16_t* coded, const std::int16_t* quantizer) const {
    if (coded == nullptr) throw std::invalid_argument("MV3 residual block requires64 coded coefficients");
    auto& workspace = storage_->dezigzag;
    for (std::size_t i = 0; i < 64; ++i) {
        const auto natural = movieInverseZigzagOrder[i];
        if (natural >= workspace.size()) throw std::out_of_range("MV3 inverse zigzag exceeds64 cells");
        workspace[natural] = coded[i];
    }
    IDCTAdd(workspace.data(), quantizer, output, pitch);
}

// PAL234AF8..234B14 and234B18..234B34: concrete one/four residual tails.
void DCTImage::IDCTAddImageData1(std::uint8_t* output, std::int32_t pitch,
                                const std::int16_t* coded, const std::int16_t* quantizer) const {
    IDCTAdd1(coded, quantizer, output, pitch);
}
void DCTImage::IDCTAddImageData4(std::uint8_t* output, std::int32_t pitch,
                                const std::int16_t* coded, const std::int16_t* quantizer) const {
    IDCTAdd4(coded, quantizer, output, pitch);
}

// PAL2363A8..236614. Original2x2 chroma reuse, integer products/truncation,
// channel/alpha store order and optional signed output pitch are retained.
void DCTImage::ToImage32(std::uint8_t* output, std::uint32_t flags) const {
    const auto& state = *storage_;
    if (output == nullptr || !state.current || (state.width & 1) || (state.height & 1))
        throw std::invalid_argument("MV3 pixel conversion requires an even initialized frame and output");
    const auto frame = state.View(state.current);
    const std::ptrdiff_t pitch = (flags & 1u) ? -static_cast<std::ptrdiff_t>(state.width) * 4 :
        static_cast<std::ptrdiff_t>(state.width) * 4;
    auto* firstRow = (flags & 1u) ? output + static_cast<std::ptrdiff_t>(state.height - 1) * state.width * 4 : output;
    std::size_t chroma = 0;
    for (std::int32_t y = 0; y < state.height; y += 2) {
        auto* top = firstRow;
        for (std::int32_t x = 0; x < state.width; x += 2, ++chroma) {
            const auto v = static_cast<std::int32_t>(frame.v[chroma]) - 128;
            const auto u = static_cast<std::int32_t>(frame.u[chroma]) - 128;
            const auto first = v * 359 / 256;
            const auto second = (u * -88 - v * 183) / 256;
            const auto third = u * 454 / 256;
            const auto store = [&](std::uint8_t* pixel, std::uint8_t luma) {
                pixel[3] = 255;
                pixel[0] = moviePixel(static_cast<std::int32_t>(luma) + first);
                pixel[1] = moviePixel(static_cast<std::int32_t>(luma) + second);
                pixel[2] = moviePixel(static_cast<std::int32_t>(luma) + third);
            };
            const auto offset = static_cast<std::size_t>(y) * state.width + x;
            store(top, frame.y[offset]); store(top + 4, frame.y[offset + 1]);
            store(top + pitch, frame.y[offset + state.width]); store(top + pitch + 4, frame.y[offset + state.width + 1]);
            top += 8;
        }
        firstRow += pitch * 2;
    }
}

// PAL2362F0..2363A8. The actual5P tiled planar output and offsets derive
// from pixelCount, not the128-byte allocation padding between source planes.
void DCTImage::ToImage32_5P(std::uint8_t* output, std::uint32_t unusedFlags) const {
    static_cast<void>(unusedFlags);
    const auto& state = *storage_;
    if (output == nullptr || !state.current || (state.width & 15) || (state.height & 7))
        throw std::invalid_argument("MV3 tiled output requires16x8-aligned initialized dimensions");
    const auto frame = state.View(state.current);
    packPlaneTiles(output, frame.y, state.width, state.height);
    packPlaneTiles(output + state.pixelCount, frame.u, state.width / 2, state.height / 2);
    packPlaneTiles(output + state.pixelCount + state.pixelCount / 4, frame.v, state.width / 2, state.height / 2);
}

// PAL236950..236AE8. Full-pixel signed motion vectors; chroma coordinates
// truncate the summed coordinates toward zero. The original packed Y load
// alignment is byte-equivalent to these16-byte rows, with no interpolation.
void DCTImage::CopyPreviousFrameBlock(std::int32_t x, std::int32_t y,
                                     std::uint8_t* outputY, std::uint8_t* outputU,
                                     std::uint8_t* outputV, std::int32_t motionIndex) const {
    const auto& state = *storage_;
    if (!state.previous || !state.current || outputY == nullptr || outputU == nullptr || outputV == nullptr ||
        motionIndex < 0 || static_cast<std::size_t>(motionIndex) >= state.blockCount)
        throw std::invalid_argument("MV3 motion copy requires the actual frame/vector owner");
    const auto current = state.View(state.current);
    std::int16_t vector[2]; std::memcpy(vector, current.blockData + static_cast<std::size_t>(motionIndex) * 4, 4);
    const auto sourceX = static_cast<std::int64_t>(x) + vector[0];
    const auto sourceY = static_cast<std::int64_t>(y) + vector[1];
    if (sourceX < 0 || sourceY < 0 || sourceX + 16 > state.width || sourceY + 16 > state.height)
        throw std::out_of_range("MV3 motion source exceeds the previous decoded image");
    const auto previous = state.View(state.previous);
    auto* source = previous.y + sourceY * state.width + sourceX;
    for (std::size_t row = 0; row < 16; ++row) {
        std::memcpy(outputY, source, 16); outputY += state.width; source += state.width;
    }
    const auto chromaPitch = state.width / 2;
    const auto chromaOffset = (sourceY / 2) * chromaPitch + sourceX / 2;
    const auto* sourceU = previous.u + chromaOffset;
    const auto* sourceV = previous.v + chromaOffset;
    for (std::size_t row = 0; row < 8; ++row) {
        std::memcpy(outputU, sourceU, 8); outputU += chromaPitch; sourceU += chromaPitch;
    }
    for (std::size_t row = 0; row < 8; ++row) {
        std::memcpy(outputV, sourceV, 8); outputV += chromaPitch; sourceV += chromaPitch;
    }
}

// PAL236690..236950. Separate original planar-copy method: one skip bit
// per macroblock, compact vector consumption, then a second chroma pass
// restarting that SAME vector stream. It does not reinterpret ToImage's
// four-macroblock/two-flag packet convention.
void DCTImage::CopyMotionBlocks(std::uint8_t* planarOutput) const {
    const auto& state = *storage_;
    if (planarOutput == nullptr || !state.current || !state.previous || !state.blockKinds ||
        (state.width & 15) || (state.height & 15))
        throw std::invalid_argument("MV3 planar motion copy requires initialized macroblock planes");
    const auto current = state.View(state.current);
    const auto previous = state.View(state.previous);
    const auto skipped = [&](std::size_t index) { return (state.blockKinds[index / 8] >> (index & 7u)) & 1u; };
    const auto vector = [&](std::size_t index) {
        if (index >= state.blockCount) throw std::out_of_range("MV3 compact motion-vector index");
        std::array<std::int16_t, 2> result;
        std::memcpy(result.data(), current.blockData + index * 4, 4); return result;
    };
    std::size_t block = 0, motion = 0;
    for (std::int32_t y = 0; y < state.height; y += 16) for (std::int32_t x = 0; x < state.width; x += 16, ++block) {
        if (skipped(block)) continue;
        const auto delta = vector(motion++);
        const auto sourceX = x + static_cast<std::int32_t>(delta[0]);
        const auto sourceY = y + static_cast<std::int32_t>(delta[1]);
        if (sourceX < 0 || sourceY < 0 || sourceX + 16 > state.width || sourceY + 16 > state.height)
            throw std::out_of_range("MV3 planar Y motion source");
        for (std::int32_t row = 0; row < 16; ++row)
            std::memcpy(planarOutput + static_cast<std::ptrdiff_t>(y + row) * state.width + x,
                previous.y + static_cast<std::ptrdiff_t>(sourceY + row) * state.width + sourceX, 16);
    }
    block = 0; motion = 0;
    const auto pitch = state.width / 2;
    auto* outputU = planarOutput + state.pixelCount;
    auto* outputV = outputU + state.pixelCount / 4;
    for (std::int32_t y = 0; y < state.height / 2; y += 8) for (std::int32_t x = 0; x < pitch; x += 8, ++block) {
        if (skipped(block)) continue;
        const auto delta = vector(motion++);
        const auto sourceX = x + delta[0] / 2;
        const auto sourceY = y + delta[1] / 2;
        if (sourceX < 0 || sourceY < 0 || sourceX + 8 > pitch || sourceY + 8 > state.height / 2)
            throw std::out_of_range("MV3 planar chroma motion source");
        for (std::int32_t row = 0; row < 8; ++row)
            std::memcpy(outputU + static_cast<std::ptrdiff_t>(y + row) * pitch + x,
                previous.u + static_cast<std::ptrdiff_t>(sourceY + row) * pitch + sourceX, 8);
        for (std::int32_t row = 0; row < 8; ++row)
            std::memcpy(outputV + static_cast<std::ptrdiff_t>(y + row) * pitch + x,
                previous.v + static_cast<std::ptrdiff_t>(sourceY + row) * pitch + sourceX, 8);
    }
}

// PAL234B38..2361CC. Whole release decoder: actual kind/motion packet,
// Huffman row stream, signed DC prediction, current/previous planar owners,
// six-block transform order, conversion and allocation-role swap.
void DCTImage::ToImage(std::uint8_t* output, std::size_t outputBytes,
                       std::int8_t quality, std::int8_t qualityDelta, std::uint32_t flags) {
    auto& state = *storage_;
    if (!state.current || !state.previous || state.encoded == nullptr ||
        (state.width & 15) || (state.height & 15))
        throw std::invalid_argument("MV3 decode requires retained bytes and initialized16x16 macroblocks");
    const auto requireBytes = [&](std::size_t at, std::size_t bytes) {
        if (at > state.encodedBytes || bytes > state.encodedBytes - at)
            throw std::out_of_range("MV3 coded packet exceeds its retained readable byte span");
    };
    InitIDCTTables(quality, qualityDelta);
    const bool intra = (flags & 2u) != 0;
    requireBytes(0, intra ? 2 : 4);
    const auto kindBytesSigned = GetI16(state.encoded);
    if (kindBytesSigned < 0) throw std::out_of_range("MV3 negative kind-stream extent");
    const auto kindBytes = static_cast<std::size_t>(kindBytesSigned);
    std::size_t motionCount = 0;
    std::size_t headerBytes = 2;
    if (!intra) {
        const auto count = GetI16(state.encoded + 2);
        if (count < 0 || static_cast<std::size_t>(count) > state.blockCount)
            throw std::out_of_range("MV3 motion vector count exceeds owned frame records");
        motionCount = static_cast<std::size_t>(count);
        const auto flagBytes = (state.blockCount + 3) / 4;
        requireBytes(4, flagBytes);
        std::memcpy(state.blockKinds.get(), state.encoded + 4, flagBytes);
        headerBytes = 4 + flagBytes;
    }
    requireBytes(headerBytes, kindBytes);
    const auto* kinds = state.encoded + headerBytes;
    const auto huffmanAt = headerBytes + kindBytes;
    Huffman huffman;
    huffman.InitDecompress(state.encoded + huffmanAt, state.encodedBytes - huffmanAt);
    const auto frame = state.View(state.current);
    if (!intra) huffman.Decompress(reinterpret_cast<std::int16_t*>(frame.blockData), static_cast<std::uint32_t>(motionCount * 2));
    std::size_t kindIndex = 0, frameBlock = 0, motionIndex = 0;
    const auto blockKind = [&](std::size_t index) {
        if (index / 4 >= kindBytes) throw std::out_of_range("MV3 block kind exceeds retained kind stream");
        return static_cast<unsigned>((kinds[index / 4] >> ((index & 3u) * 2)) & 3u);
    };
    const auto signedHalf = [](std::int32_t value) {
        const auto low = static_cast<std::uint32_t>(value) & 65535u;
        return static_cast<std::int16_t>(low <= 32767u ? static_cast<std::int32_t>(low) : static_cast<std::int32_t>(low) - 65536);
    };
    for (std::int32_t y = 0; y < state.height; y += 16) {
        const auto coefficientCount = huffman.GetBits(16);
        const auto codedBlocks = intra ? static_cast<std::uint32_t>(state.width / 16) : huffman.GetBits(8);
        if (coefficientCount > state.coefficientCount || codedBlocks > static_cast<std::uint32_t>(state.width / 16))
            throw std::out_of_range("MV3 Huffman row exceeds owned coefficient workspace");
        huffman.Decompress(state.coefficients.get(), coefficientCount);
        std::size_t coefficientAt = 0, predictionKind = kindIndex;
        std::array<std::int16_t, 3> predictor{};
        for (std::uint32_t block = 0; block < codedBlocks; ++block) for (unsigned planeBlock = 0; planeBlock < 6; ++planeBlock) {
            const auto kind = blockKind(predictionKind++);
            const auto count = movieBlockCoefficientCounts[kind];
            if (coefficientAt > coefficientCount || count > coefficientCount - coefficientAt)
                throw std::out_of_range("MV3 DC prediction exceeds decoded row coefficients");
            if (count != 0) {
                const auto plane = planeBlock < 4 ? 0u : planeBlock - 3;
                const auto dc = signedHalf(static_cast<std::int32_t>(state.coefficients[coefficientAt]) + predictor[plane]);
                state.coefficients[coefficientAt] = dc; predictor[plane] = dc;
                coefficientAt += count;
            }
        }
        coefficientAt = 0;
        for (std::int32_t x = 0; x < state.width; x += 16, ++frameBlock) {
            auto* outputY = frame.y + static_cast<std::size_t>(y) * state.width + x;
            auto* outputU = frame.u + static_cast<std::size_t>(y / 2) * (state.width / 2) + x / 2;
            auto* outputV = frame.v + static_cast<std::size_t>(y / 2) * (state.width / 2) + x / 2;
            const auto macroFlags = intra ? 0u : static_cast<unsigned>(state.blockKinds[frameBlock / 4] >> (frameBlock & 3u));
            const bool residual = (macroFlags & 1u) != 0;
            if (residual) {
                if (motionIndex >= motionCount) throw std::out_of_range("MV3 referenced motion vector was not decoded");
                CopyPreviousFrameBlock(x, y, outputY, outputU, outputV, static_cast<std::int32_t>(motionIndex++));
                if (macroFlags & 16u) continue;
            }
            for (unsigned planeBlock = 0; planeBlock < 6; ++planeBlock) {
                const auto kind = blockKind(kindIndex++);
                const auto count = movieBlockCoefficientCounts[kind];
                if (coefficientAt > coefficientCount || count > coefficientCount - coefficientAt)
                    throw std::out_of_range("MV3 inverse block exceeds decoded row coefficients");
                auto* destination = planeBlock < 4 ? outputY + (planeBlock & 1u) * 8 + (planeBlock / 2) * static_cast<std::size_t>(state.width) * 8 :
                    planeBlock == 4 ? outputU : outputV;
                const auto pitch = planeBlock < 4 ? state.width : state.width / 2;
                const auto& table = state.quantizers[(planeBlock < 4 ? 0u : 2u) + (residual ? 1u : 0u)];
                const auto* coefficients = state.coefficients.get() + coefficientAt;
                if (residual) {
                    if (kind == 1) IDCTAddImageData1(destination, pitch, coefficients, table.data());
                    else if (kind == 2) IDCTAddImageData4(destination, pitch, coefficients, table.data());
                    else if (kind == 3) IDCTAddImageData(destination, pitch, coefficients, table.data());
                } else {
                    if (kind == 0) IDCTZero(destination, pitch);
                    else if (kind == 1) IDCTImageData1(destination, pitch, coefficients, table.data());
                    else if (kind == 2) IDCTImageData4(destination, pitch, coefficients, table.data());
                    else IDCTImageData(destination, pitch, coefficients, table.data());
                }
                coefficientAt += count;
            }
        }
    }
    if ((flags & 4u) && (state.initializationOption == 0 || state.initializationOption == 1)) {
        const auto required = static_cast<std::size_t>(state.pixelCount) * 4;
        if (output == nullptr || outputBytes < required) throw std::out_of_range("MV332-bit output span");
        ToImage32(output, flags);
    }
    SwapPrevBuffer();
}

DCTFrameView DCTImage::Current() const noexcept { return storage_->View(storage_->current); }
DCTFrameView DCTImage::Previous() const noexcept { return storage_->View(storage_->previous); }
std::int32_t DCTImage::Width() const noexcept { return storage_->width; }
std::int32_t DCTImage::Height() const noexcept { return storage_->height; }
const std::array<std::array<std::int16_t, 64>, 4>& DCTImage::Quantizers() const noexcept {
    return storage_->quantizers;
}
void DCTImage::BindEncodedData(const std::uint8_t* input, std::size_t readableBytes) {
    if (input == nullptr && readableBytes != 0) throw std::invalid_argument("MV3 coded byte span");
    storage_->encoded = input; storage_->encodedBytes = readableBytes;
}

} // namespace hp2
