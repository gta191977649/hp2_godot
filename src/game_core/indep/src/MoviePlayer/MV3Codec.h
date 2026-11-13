#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <exception>
#include "DCTImage.h"

struct GameTime;
struct WorldAudioRequestQueue;
namespace hp2 { class GameResources; class FileIoRuntime; class ResourceRegistry; }

namespace hp2 {

std::uint32_t GetU32(const void* input);
std::uint16_t GetU16(const void* input);
std::int16_t GetI16(const void* input);

// Native arrangement of the actually shared codec data. Both clamp pointers
// address this same storage, and all codecs use the same reference count.
struct CodecSharedState final {
    CodecSharedState(const CodecSharedState&) = delete;
    CodecSharedState& operator=(const CodecSharedState&) = delete;
    CodecSharedState() = default;
    std::array<std::uint8_t, 768> clamp{};
    std::array<std::uint8_t, 32> fiveBitExpansion{};
    std::uint8_t* clampZero = clamp.data() + 256;
    std::uint8_t* dctClampZero = clamp.data() + 384;
    std::int32_t instanceCount = 0;
    float yRed = 0.299f;
    float yGreen = 0.587f;
    float yBlue = 0.114f;
    float uRed = -0.1687f;
    float uGreen = 0.3313f;
    float vGreen = 0.4187f;
    float vBlue = 0.0813f;
    float redV = 1.402f;
    float greenU = 0.34414f;
    float greenV = 0.71414f;
    float blueU = 1.772f;
};
extern CodecSharedState movieCodecShared;

class Codec {
public:
    Codec();
    virtual ~Codec();
    Codec(const Codec&) = delete;
    Codec& operator=(const Codec&) = delete;
    Codec(Codec&&) = delete;
    Codec& operator=(Codec&&) = delete;
    void rgb2yuv(std::uint8_t* pixel) const;
    void yuv2rgb(std::uint8_t* pixel) const;

private:
    std::unique_ptr<std::uint8_t[]> temporary_;
};

// The final file format uses a 52-byte header, including three colour masks.
// These are fixed-width file fields, independent of Windows SDK structures.
struct BitmapInfoHeader {
    std::uint32_t biSize;
    std::int32_t biWidth;
    std::int32_t biHeight;
    std::uint16_t biPlanes;
    std::uint16_t biBitCount;
    std::uint32_t biCompression;
    std::uint32_t biSizeImage;
    std::int32_t biXPelsPerMeter;
    std::int32_t biYPelsPerMeter;
    std::uint32_t biClrUsed;
    std::uint32_t biClrImportant;
    std::uint32_t biRedMask;
    std::uint32_t biGreenMask;
    std::uint32_t biBlueMask;
};
struct BitmapInfo {
    BitmapInfoHeader bmiHeader;
    std::uint32_t bmiColors[1];
};
static_assert(sizeof(BitmapInfoHeader) == 52, "MV3 bitmap header extent");
static_assert(sizeof(BitmapInfo) == 56, "MV3 bitmap format extent");

// Native pointers replace the PAL descriptor's 32-bit pointer fields. This
// descriptor is an in-memory call argument; it is never read from an AVI.
struct ICDecompressEx {
    std::uint32_t dwFlags;
    const BitmapInfoHeader* lpbiSrc;
    const std::uint8_t* lpSrc;
    const BitmapInfoHeader* lpbiDst;
    std::uint8_t* lpDst;
    std::int32_t xDst, yDst, dxDst, dyDst;
    std::int32_t xSrc, ySrc, dxSrc, dySrc;
};

// The individually proved MV3 owner embeds one DCTImage and shares Codec's
// clamp tables. FrameEx is individually anchored by MoviePlayer.Update48;
// the other identical Frame wrapper and named plane getters remain held.
class MV3Codec final : public Codec {
public:
    MV3Codec();
    ~MV3Codec() override;
    std::int32_t GetDecompressFormat(const BitmapInfo* input, BitmapInfo* output) const;
    std::int32_t QueryDecompress(const BitmapInfo* input, const BitmapInfo* output) const;
    std::int32_t QueryDecompressEx(const ICDecompressEx* request, std::int32_t bytes) const;
    virtual std::int32_t BeginDecompress(const BitmapInfoHeader* input,
                                         const BitmapInfoHeader* output);
    std::int32_t BeginDecompressEx(const ICDecompressEx* request, std::int32_t bytes);
    std::int32_t DecompressFrameShared(std::uint32_t flags,
        const BitmapInfoHeader* input, const std::uint8_t* encoded,
        const BitmapInfoHeader* output, std::uint8_t* pixels,
        std::size_t readableBytes, std::size_t outputBytes);
    std::int32_t DecompressFrameEx(const ICDecompressEx* request,
        std::size_t readableBytes, std::size_t outputBytes);
    std::int32_t EndDecompress() noexcept;

    // PAL MV3+288: one source-owned conversion control, written by the actual
    // MoviePlayer.Update producer at 23845C. No second frame/image owner.
    std::int32_t ConvertImageToRGBA = 0;

    // Native borrowed view/state diagnostics, not recovered getter identities.
    const DCTImage& Image() const noexcept { return image_; }
    std::array<std::uint32_t, 4> DecodeState() const noexcept {
        return {{static_cast<std::uint32_t>(decodeWidth_), static_cast<std::uint32_t>(decodeHeight_),
            static_cast<std::uint32_t>(decodeBits_), currentFrame_}};
    }

private:
    DCTImage image_;
    std::int32_t decodeWidth_ = 0;
    std::int32_t decodeHeight_ = 0;
    std::int32_t decodeBits_ = 0;
    std::uint32_t currentFrame_ = 0;
};

// The named owner is defined in the proved movieplayer.cpp. This is an
// observed original header arrangement, not an invented MoviePlayer.h or a
// beta object overlay. All resources, clocks and audio borrow live owners.
class MoviePlayer final {
public:
    MoviePlayer(const ::hp2::GameResources&, ::hp2::FileIoRuntime&,
        ::hp2::ResourceRegistry&, const ::GameTime&, const std::int32_t& soundEnabled,
        const std::int32_t& enableFivePass, ::WorldAudioRequestQueue* audio);
    ~MoviePlayer();
    MoviePlayer(const MoviePlayer&) = delete;
    MoviePlayer& operator=(const MoviePlayer&) = delete;
    int InitFile(const char* filename);
    // Movie stream metadata remains retained through a full device queue;
    // its real STOP is submitted when acknowledgement frees a command slot.
    void Close();
    // Terminal CPU cleanup does not throw through a destructor. The first
    // failure remains available while a failed audio/IO retirement is retried.
    std::exception_ptr Shutdown() noexcept;
    int Update(float elapsed, void* output, std::size_t outputBytes);
    void SetFPS(float fps);
    int GetAudio(void* output, std::uint32_t unused, std::uint32_t bytes);
    int ExtractVideoHeader(const void* header, std::size_t bytes);
    static void BuildYUV2ABGRPalettes(std::uint32_t* y, std::uint32_t* ua,
        std::uint32_t* us, std::uint32_t* va, std::uint32_t* vs);
    // Borrowed native observations, zero recovered getter/body claims.
    int Width() const noexcept;
    int Height() const noexcept;
    bool ValidFrame() const noexcept;
    void UseExternalPlayback() noexcept;
    const char* RequestedMovie() const noexcept;
    std::uint64_t MovieRequestId() const noexcept;
    void CompleteExternalMovie(std::uint64_t request) noexcept;
    const ::GameTime& Clock() const noexcept;
    std::array<std::uint32_t, 16> InspectState() const noexcept;
    std::array<std::uint32_t, 39> InspectHeaders() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

} // namespace hp2
