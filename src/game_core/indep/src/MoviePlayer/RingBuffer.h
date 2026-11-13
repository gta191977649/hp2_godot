#pragma once

#include <cstdint>
#include <memory>

namespace hp2 { class FileIoRuntime; class FileRange; }

// Observed original header; these are native owning declarations, not the
// serialized/PS2 object layout. RingBuffer embeds this one byte FIFO.
namespace hp2 {

class CyclicBuffer final {
public:
    CyclicBuffer() noexcept;
    ~CyclicBuffer();
    CyclicBuffer(const CyclicBuffer&) = delete;
    CyclicBuffer& operator=(const CyclicBuffer&) = delete;
    CyclicBuffer(CyclicBuffer&&) = delete;
    CyclicBuffer& operator=(CyclicBuffer&&) = delete;

    void Init(std::int32_t capacity);
    void Close() noexcept;
    std::int32_t DataAvail() const noexcept;
    std::int32_t SpaceAvail() const noexcept;
    std::int32_t AddData(const void* source, std::int32_t count);
    std::int32_t GetData(void* destination, std::int32_t count,
                         std::int32_t consume);

private:
    std::unique_ptr<std::uint8_t[]> buffer_;
    std::int32_t capacity_;
    std::int32_t write_;
    std::int32_t read_;
};

// Native typed view of one actually pooled source frame descriptor. Payloads
// borrow the streaming allocation; returning/reusing a frame never copies it.
struct MovieFrame final {
    MovieFrame* next = nullptr;
    std::uint32_t chunkBytes = 0;
    std::uint32_t dataOffset = 0;
    std::uint32_t frameIndex = 0;
    std::uint32_t videoBytes = 0;
    std::uint32_t audioBytes = 0;
    std::uint32_t junkBytes = 0;
    std::uint32_t audioOffset = 0;
    const std::uint8_t* videoData = nullptr;
    const std::uint8_t* audioData = nullptr;
};

class RingBuffer final {
public:
    explicit RingBuffer(::hp2::FileIoRuntime& io);
    ~RingBuffer();
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;
    void ReleaseAll(const ::hp2::FileRange* waitForFile = nullptr);
    MovieFrame* GetLastFrame() const noexcept;
    std::int32_t Init(std::int32_t capacity, bool withAudio);
    void StartupBuffer(const ::hp2::FileRange& file, std::uint32_t startOffset);
    std::uint32_t ContinuousAvail() const noexcept;
    void ReadData(const ::hp2::FileRange& file, std::int32_t requestedBytes);
    void ASyncDoneCallback() noexcept;
    MovieFrame* GetFrame() const noexcept;
    std::int32_t GetAudio(void* output, std::int32_t unused, std::int32_t bytes);
    void ReleaseFrame();

    // Native transport delivery and actual MoviePlayer direct-member stores,
    // not additional original methods/counts. The borrowed io/file outlive us.
    void ServicePhysicalRead();
    void SetFrameLimit(std::uint32_t limit) noexcept;
    void SetAudioConsumption(bool consume) noexcept;
    // Native physical playback can lag the movie producer. Retain an entire
    // pending record until its real audio FIFO has room; never drop 01wb data.
    // Off means no active consumer, retaining the original muted parser path.
    void SetAudioConsumerActive(bool active) noexcept;
    std::size_t ReadableBytes(const std::uint8_t* data) const;
    bool Drained() const noexcept;

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

} // namespace hp2
