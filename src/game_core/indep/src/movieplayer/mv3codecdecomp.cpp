#include "MV3Codec.h"
#include "Huffman2.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace hp2 {
// PAL239118..239148, uniquely selected by the real Update descriptor call.
// The identical non-Ex wrapper has not been assigned by exclusion.
std::int32_t MV3Codec::DecompressFrameEx(const ICDecompressEx* request,
    std::size_t readableBytes, std::size_t outputBytes) {
    if (!request) throw std::invalid_argument("MV3 missing frame descriptor");
    return DecompressFrameShared(request->dwFlags,request->lpbiSrc,request->lpSrc,
        request->lpbiDst,request->lpDst,readableBytes,outputBytes);
}

namespace {
constexpr std::uint32_t movieCompression = 0x3033564du; // Authored 'MV30'.

std::int32_t negateWord(std::int32_t value) noexcept {
    const auto bits = 0u - static_cast<std::uint32_t>(value);
    return bits <= 0x7fffffffu ? static_cast<std::int32_t>(bits) :
        -1 - static_cast<std::int32_t>(~bits);
}

bool supportedInput(const BitmapInfoHeader& input) noexcept {
    return (static_cast<std::uint32_t>(input.biWidth) & 3u) == 0 &&
        (static_cast<std::uint32_t>(input.biHeight) & 3u) == 0 &&
        input.biCompression == movieCompression && input.biBitCount == 24;
}
} // namespace

// PAL238ED0..238F2C. Null output queries the full format size. Copy exactly
// the caller's header extent: unmentioned mask/palette bytes stay untouched.
std::int32_t MV3Codec::GetDecompressFormat(const BitmapInfo* input, BitmapInfo* output) const {
    if (output == nullptr) return 56;
    if (input == nullptr || input->bmiHeader.biSize > sizeof(BitmapInfo))
        throw std::invalid_argument("MV3 format header exceeds its supplied object");
    if (output != input) std::memcpy(output, input, input->bmiHeader.biSize);
    auto& header = output->bmiHeader;
    header.biBitCount = 32;
    header.biCompression = 0;
    header.biSizeImage = static_cast<std::uint32_t>(header.biWidth) *
        static_cast<std::uint32_t>(header.biHeight) * 4u;
    return 0;
}

// PAL238F30..238FB0. Preserve the source's four-pixel alignment check,
// including its distinction from DCTImage's actual macroblock allocation.
std::int32_t MV3Codec::QueryDecompress(const BitmapInfo* input, const BitmapInfo* output) const {
    if (input == nullptr) throw std::invalid_argument("MV3 missing input format");
    const auto& source = input->bmiHeader;
    if (!supportedInput(source)) return -2;
    if (output == nullptr) return 0;
    const auto& destination = output->bmiHeader;
    return destination.biWidth == source.biWidth && destination.biHeight == source.biHeight &&
        destination.biCompression == 0 && destination.biBitCount == 32 ? 0 : -2;
}

// PAL238FB0..239050. Extended queries accept either height sign. Compression
// three is accepted independently of bit depth; rectangles/byte count are not
// consulted by this original method and must not add new rejection branches.
std::int32_t MV3Codec::QueryDecompressEx(const ICDecompressEx* request, std::int32_t bytes) const {
    static_cast<void>(bytes);
    if (request == nullptr || request->lpbiSrc == nullptr)
        throw std::invalid_argument("MV3 missing extended input format");
    const auto& source = *request->lpbiSrc;
    if (!supportedInput(source)) return -2;
    if (request->lpbiDst == nullptr) return 0;
    const auto& destination = *request->lpbiDst;
    if (destination.biWidth != source.biWidth ||
        (destination.biHeight != source.biHeight && destination.biHeight != negateWord(source.biHeight)))
        return -2;
    if (destination.biCompression == 0) return destination.biBitCount == 32 ? 0 : -2;
    return destination.biCompression == 3 ? 0 : -2;
}

// PAL239050..2390B8. Uses the output format and ignores the input argument;
// release/reallocation and decode-table generation occur on every begin.
std::int32_t MV3Codec::BeginDecompress(const BitmapInfoHeader* input,
                                      const BitmapInfoHeader* output) {
    static_cast<void>(input);
    if (output == nullptr) throw std::invalid_argument("MV3 missing output format");
    decodeWidth_ = output->biWidth;
    decodeBits_ = output->biBitCount;
    decodeHeight_ = std::max(output->biHeight, negateWord(output->biHeight));
    image_.Init(decodeWidth_, decodeHeight_, false, 1);
    Huffman huffman;
    huffman.BuildDecodeTab();
    currentFrame_ = 0;
    return 0;
}

// PAL2390B8..2390E8. The original dispatches the same owned BeginDecompress
// through its virtual slot; no separate allocation or successful shortcut.
std::int32_t MV3Codec::BeginDecompressEx(const ICDecompressEx* request, std::int32_t bytes) {
    static_cast<void>(bytes);
    if (request == nullptr) throw std::invalid_argument("MV3 missing extended descriptor");
    return BeginDecompress(request->lpbiSrc, request->lpbiDst);
}

// PAL239148..239200. The compressed packet, not dwFlags, selects intra/delta.
// The two added byte counts describe borrowed native spans, including genuine
// container lookahead. No encoded bytes or padding are manufactured here.
std::int32_t MV3Codec::DecompressFrameShared(std::uint32_t flags,
    const BitmapInfoHeader* input, const std::uint8_t* encoded,
    const BitmapInfoHeader* output, std::uint8_t* pixels,
    std::size_t readableBytes, std::size_t outputBytes) {
    static_cast<void>(flags);
    static_cast<void>(input);
    if (encoded == nullptr || readableBytes < 4 || output == nullptr)
        throw std::invalid_argument("MV3 missing coded packet or output format");
    const auto signedByte = [](std::uint8_t value) {
        return static_cast<std::int8_t>(value < 128 ? value : static_cast<int>(value) - 256);
    };
    const auto quality = signedByte(encoded[0]);
    const auto delta = signedByte(encoded[1]);
    std::uint32_t imageFlags = ConvertImageToRGBA != 0 ? 4u : 0u;
    if (GetU16(encoded + 2) == 0) imageFlags |= 2u;
    if (output->biHeight < 0) imageFlags |= 1u;
    image_.BindEncodedData(encoded + 4, readableBytes - 4);
    image_.ToImage(pixels, outputBytes, quality, delta, imageFlags);
    return 0;
}

// PAL239200..239220. Release the real image allocations; source width,
// height, format and frame metadata remain as they were at BeginDecompress.
std::int32_t MV3Codec::EndDecompress() noexcept {
    image_.ReleaseAll();
    return 0;
}

} // namespace hp2
