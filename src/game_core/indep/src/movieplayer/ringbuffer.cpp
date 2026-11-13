#include "RingBuffer.h"
#include "MV3Codec.h"
#include "bware/inc/bFile.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace hp2 {

struct RingBuffer::Storage {
    explicit Storage(::hp2::FileIoRuntime& runtime) : io(runtime) {}
    ::hp2::FileIoRuntime& io;
    const ::hp2::FileRange* file = nullptr;
    std::shared_ptr<::hp2::FileReadOperation> physicalRead;
    std::unique_ptr<std::uint8_t[]> buffer;
    std::unique_ptr<std::uint8_t[]> staging;
    std::unique_ptr<MovieFrame[]> frames;
    MovieFrame* head = nullptr;
    MovieFrame* free = nullptr;
    CyclicBuffer audio;
    std::uint32_t capacity = 0;
    std::uint32_t written = 0;
    std::uint32_t consumed = 0;
    std::uint32_t pendingBytes = 0;
    std::uint32_t fileOffset = 0;
    std::uint32_t fileBytes = 0;
    std::uint32_t frameLimit = 0;
    std::uint32_t framesFound = 0;
    std::uint32_t audioOffset = 0;
    bool frameLimitBound = false;
    bool consumeAudio = true;
    bool readComplete = true;
    bool audioConsumerActive = false;
    bool audioRecordPending = false;
};

std::size_t RingBuffer::ReadableBytes(const std::uint8_t* data) const {
    const auto& s=*storage_;
    const auto start=reinterpret_cast<std::uintptr_t>(s.buffer.get());
    const auto at=reinterpret_cast<std::uintptr_t>(data);
    if (!s.buffer || at<start || at-start>s.capacity)
        throw std::invalid_argument("Movie payload does not borrow its streaming allocation");
    // ReadData publishes only complete records with the following real
    // eight-byte header retained. Do not expose the uninitialized allocation
    // tail as decoder lookahead, even when a malformed bitstream asks for it.
    const auto offset=static_cast<std::uint32_t>(at-start);
    for (auto* frame=s.head;frame;frame=frame->next) {
        if (frame->videoData!=data)continue;
        const auto end=std::uint64_t{frame->dataOffset}+frame->chunkBytes;
        const auto videoEnd=std::uint64_t{offset}+frame->videoBytes;
        if (videoEnd>end || end+8>s.capacity ||
            (offset<s.written && videoEnd+8>s.written))
            throw std::out_of_range("Movie payload exceeds its published complete record");
        return static_cast<std::size_t>(frame->videoBytes)+8;
    }
    throw std::invalid_argument("Movie payload has no live published frame");
}

bool RingBuffer::Drained() const noexcept {
    const auto& s=*storage_;
    return s.head && !s.head->next && s.written==s.consumed;
}

// PAL239220..23927C. File service is the actual shared Windows transport,
// not another request queue or game-level completion/loaded-state owner.
RingBuffer::RingBuffer(::hp2::FileIoRuntime& io) : storage_(new Storage(io)) {}

// PAL239280..2392DC, including the real embedded CyclicBuffer lifetime.
RingBuffer::~RingBuffer() {
    ReleaseAll();
}

// PAL2392E0..23939C. The physical transport must finish writing before native
// destination storage is released (the source bFile close/wait contract).
void RingBuffer::ReleaseAll(const ::hp2::FileRange* waitForFile) {
    auto& state = *storage_;
    if (waitForFile != nullptr && state.file != nullptr && waitForFile != state.file)
        throw std::invalid_argument("Movie wait refers to another physical file");
    if (state.physicalRead) {
        state.io.Wait(state.physicalRead);
        if (state.io.State(state.physicalRead) == ::hp2::FileReadState::Complete)
            ASyncDoneCallback();
        state.physicalRead.reset();
    }
    state.buffer.reset();
    state.staging.reset();
    state.frames.reset();
    state.head = nullptr;
    state.free = nullptr;
    state.capacity = 0;
    state.written = 0;
    state.consumed = 0;
    state.file = nullptr;
    state.audio.Close();
    state.audioConsumerActive = false;
    state.audioRecordPending = false;
}

// PAL2393A0..2393D8.
MovieFrame* RingBuffer::GetLastFrame() const noexcept {
    MovieFrame* frame = storage_->head;
    if (frame != nullptr)
        while (frame->next != nullptr)
            frame = frame->next;
    return frame;
}

// PAL2393D8..2394D4. The exact256-frame pool and0x60000-byte audio FIFO
// coexist with the sector-sized streaming buffer and0x18000-byte staging area.
std::int32_t RingBuffer::Init(std::int32_t capacity, bool withAudio) {
    if (capacity < 0x4000)
        throw std::invalid_argument("Movie streaming capacity is below its startup window");
    ReleaseAll();
    auto& state = *storage_;
    if (withAudio)
        state.audio.Init(0x60000);
    state.capacity = static_cast<std::uint32_t>(capacity) & ~2047u;
    state.buffer.reset(new std::uint8_t[state.capacity]);
    state.staging.reset(new std::uint8_t[0x18000]);
    state.frames.reset(new MovieFrame[256]);
    state.free = state.frames.get();
    for (std::size_t index = 0; index < 255; ++index)
        state.frames[index].next = &state.frames[index + 1];
    state.frames[255].next = nullptr;
    state.readComplete = true;
    return 1;
}

// PAL2394D8..23959C. FileRange supplies the same retained bytes through a
// checked physical read; no independent file-position mirror is introduced.
void RingBuffer::StartupBuffer(const ::hp2::FileRange& file, std::uint32_t startOffset) {
    auto& state = *storage_;
    if (!file.Valid() || !state.buffer || state.free == nullptr)
        throw std::invalid_argument("Movie startup requires initialized buffers and a live file");
    if (file.Size() > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("Movie file exceeds the source signed scheduling domain");
    state.fileBytes = static_cast<std::uint32_t>(file.Size());
    state.fileOffset = startOffset;
    state.pendingBytes = (state.capacity - 0x2000u) & ~2047u;
    file.Read(startOffset, state.buffer.get(), state.pendingBytes);
    state.file = &file;
    state.head = state.free;
    state.free = state.free->next;
    state.head->next = nullptr;
    state.head->dataOffset = 0;
    const std::uint32_t chunk = GetU32(state.buffer.get() + 4) + 8u;
    state.head->chunkBytes = chunk + (chunk & 1u);
    state.head->junkBytes = 0;
    state.head->videoData = nullptr;
    state.head->audioData = nullptr;
    state.consumeAudio = true;
    state.framesFound = 0;
}

// PAL2395A0..2395D8. Source uses unsigned cursors and reserves a sector at
// the wrap boundary. Keep its arithmetic, including a closed/unbound result.
std::uint32_t RingBuffer::ContinuousAvail() const noexcept {
    const auto& state = *storage_;
    if (state.written < state.consumed)
        return state.consumed - state.written - 2048u;
    if (state.consumed != 0)
        return state.capacity - state.written;
    return state.capacity - state.written - 2048u;
}

// PAL2395D8..239A44, the complete final release parser/scheduler. In
// particular its early frame-limit gate is newer than the beta body.
void RingBuffer::ReadData(const ::hp2::FileRange& file, std::int32_t requestedBytes) {
    auto& state = *storage_;
    if (state.file != &file || !state.buffer || !state.staging || !state.frameLimitBound)
        throw std::invalid_argument("Movie parser requires its actual initialized file/frame-limit owner");
    if (requestedBytes < 0 || requestedBytes > std::numeric_limits<std::int32_t>::max() - 2047)
        throw std::invalid_argument("Movie read request exceeds source scheduling domain");
    ServicePhysicalRead();
    if (!state.readComplete)
        return;

    const auto within = [&](std::uint32_t offset, std::uint32_t bytes) {
        if (offset > state.capacity || bytes > state.capacity - offset)
            throw std::out_of_range("Movie chunk exceeds retained streaming allocation");
    };
    const auto wordAt = [&](std::uint32_t offset) {
        within(offset, 4);
        return GetU32(state.buffer.get() + offset);
    };
    const auto recognized = [](std::uint32_t kind) {
        return kind == 0x5453494cu || kind == 0x62643030u ||
            kind == 0x63643030u || kind == 0x62773130u || kind == 0x4b4e554au;
    };

    if (state.pendingBytes != 0 || state.written > state.capacity - 2048u ||
        state.audioRecordPending) {
        if (state.framesFound >= state.frameLimit)
            return;
        if (state.pendingBytes <= 0x18000u) {
            within(state.written, state.pendingBytes);
            if (state.pendingBytes != 0)
                std::memcpy(state.buffer.get() + state.written,
                            state.staging.get(), state.pendingBytes);
        }
        state.fileOffset += state.pendingBytes;
        state.written += state.pendingBytes;
        state.pendingBytes = 0;

        for (;;) {
            MovieFrame* frame = GetLastFrame();
            if (frame == nullptr)
                throw std::logic_error("Movie parser has no startup descriptor");
            const std::uint32_t start = frame->dataOffset;
            const std::uint32_t end = start + frame->chunkBytes;
            if (end < start)
                throw std::out_of_range("Movie chunk length wraps its allocation");

            if (end >= state.capacity - 2048u) {
                const std::uint32_t tail = state.written - start;
                if (tail != 0) {
                    if (state.written < state.consumed || tail >= state.consumed)
                        return;
                    within(start, tail);
                    within(0, tail);
                    std::memcpy(state.buffer.get(), state.buffer.get() + start, tail);
                    state.written = tail;
                    frame->dataOffset = 0;
                    break; // The source schedules another physical fill here.
                }
            }

            const std::uint32_t lastHeader = state.written - 8u;
            if (state.consumed < state.written) {
                if (end >= lastHeader)
                    break;
            } else if (start < lastHeader && end >= lastHeader) {
                break;
            }

            if (state.audioConsumerActive) {
                // The PAL producer ignores CyclicBuffer::AddData(false). That
                // loses arbitrary-sized 01wb chunks when a native device or
                // presentation schedule lags. These bytes are a continuous
                // ADPCM stream, so even one lost byte destroys block framing.
                // Preflight the complete retained record without publishing
                // payloads/counters. A retry therefore cannot duplicate audio
                // or make video release storage still needed by the consumer.
                std::uint32_t needed = 0;
                for (std::uint32_t cursor = start; cursor < end;) {
                    if (end - cursor < 8u)
                        throw std::out_of_range("Movie record has a truncated subchunk header");
                    const auto kind = wordAt(cursor);
                    const auto bytes = wordAt(cursor + 4u);
                    if (kind == 0x5453494cu) {
                        if (end - cursor < 12u)
                            throw std::out_of_range("Movie record has a truncated LIST header");
                        cursor += 12u;
                        continue;
                    }
                    const std::uint64_t next = std::uint64_t{cursor} + 8u + bytes + (bytes & 1u);
                    if (next > end)
                        throw std::out_of_range("Movie subchunk exceeds its complete retained record");
                    if (kind == 0x62773130u) {
                        constexpr std::uint32_t maximumAudio = 0x60000u - 1u;
                        if (bytes > maximumAudio - needed)
                            throw std::length_error("Movie record audio exceeds the original FIFO capacity");
                        needed += bytes;
                    }
                    cursor = static_cast<std::uint32_t>(next);
                }
                if (needed != 0 && (state.audio.SpaceAvail() < 0 ||
                    needed > static_cast<std::uint32_t>(state.audio.SpaceAvail()))) {
                    state.audioRecordPending = true;
                    return;
                }
            }
            state.audioRecordPending = false;
            frame->junkBytes = 0;
            frame->videoBytes = 0;
            frame->audioBytes = 0;
            std::uint32_t parsed = 0;
            do {
                const std::uint32_t at = frame->dataOffset + parsed;
                const std::uint32_t kind = wordAt(at);
                const std::uint32_t bytes = wordAt(at + 4u);
                if (kind == 0x5453494cu) { // LIST: step over its record header.
                    parsed += 12u;
                    continue;
                }
                if (bytes > std::numeric_limits<std::uint32_t>::max() - parsed - 9u)
                    throw std::out_of_range("Movie subchunk length overflows its record");
                parsed += bytes + 8u;
                if (kind == 0x62773130u) { // 01wb, original audio-byte stream.
                    within(at + 8u, bytes);
                    frame->audioBytes = bytes;
                    frame->audioData = state.buffer.get() + at + 8u;
                    frame->audioOffset = state.audioOffset;
                    state.audioOffset += bytes;
                    const auto accepted = state.audio.AddData(frame->audioData,
                        static_cast<std::int32_t>(bytes));
                    if (!accepted && state.audioConsumerActive)
                        throw std::logic_error("Movie audio preflight lost its owner-thread FIFO reservation");
                    // With no native consumer, retain the original silent
                    // producer path rather than waiting for absent playback.
                } else if (kind == 0x62643030u || kind == 0x63643030u) { // 00db/dc.
                    within(at + 8u, bytes);
                    frame->videoBytes = bytes;
                    frame->videoData = state.buffer.get() + at + 8u;
                    frame->frameIndex = state.framesFound;
                    ++state.framesFound;
                } else if (kind == 0x4b4e554au) {
                    frame->junkBytes = bytes;
                }
                parsed = (parsed + 1u) & ~1u;
            } while (parsed < frame->chunkBytes);

            if (state.fileOffset == state.fileBytes || state.framesFound == state.frameLimit)
                break;
            if (state.written < end + 8u)
                break;
            const std::uint32_t nextKind = wordAt(end < state.capacity ? end : 0u);
            if (!recognized(nextKind)) {
                state.framesFound = state.frameLimit;
                return;
            }
            const std::uint32_t lengthAt = end + 4u;
            const std::uint32_t bytes = wordAt(lengthAt < state.capacity ? lengthAt : 0u);
            if (bytes > std::numeric_limits<std::uint32_t>::max() - 9u)
                throw std::out_of_range("Movie next record length overflows");
            if (state.free == nullptr)
                throw std::out_of_range("Movie exceeds the actual256-descriptor pool");
            frame->next = state.free;
            MovieFrame* next = state.free;
            state.free = next->next;
            next->dataOffset = end;
            next->chunkBytes = bytes + (bytes & 1u) + 8u;
            next->audioBytes = 0;
            next->next = nullptr;
            next->videoData = nullptr;
            next->audioData = nullptr;
            next->junkBytes = 0;
            next->videoBytes = 0;
        }
    }

    // Actual signed minima, clamp-to-zero, then sector-down alignment. The
    // next OS read only fills staging; parsing waits for its real completion.
    const auto signedWord = [](std::uint32_t word) {
        return word <= 0x7fffffffu ? static_cast<std::int32_t>(word) :
            -1 - static_cast<std::int32_t>(~word);
    };
    std::int32_t request = (requestedBytes + 2047) & ~2047;
    request = std::min(request, signedWord(ContinuousAvail()));
    request = std::min(request, signedWord(state.fileBytes - state.fileOffset));
    if (request < 0)
        request = 0;
    request &= ~2047;
    if (request == 0 || state.framesFound >= state.frameLimit)
        return;
    if (state.physicalRead)
        throw std::logic_error("Movie physical read overlaps its pending transfer");
    if (request > 0x18000)
        throw std::out_of_range("Movie read exceeds its actual staging allocation");
    state.readComplete = false;
    state.physicalRead = state.io.Read(file, state.fileOffset,
        state.staging.get(), static_cast<std::size_t>(request));
    state.pendingBytes = static_cast<std::uint32_t>(request);
}

// PAL239A48..239A54. Delivery happens only after a real physical transfer.
void RingBuffer::ASyncDoneCallback() noexcept {
    storage_->readComplete = true;
}

// PAL239A58..239AB4. The final descriptor is withheld only when it has no
// successor and all physical file bytes have already been consumed.
MovieFrame* RingBuffer::GetFrame() const noexcept {
    const auto& state = *storage_;
    MovieFrame* frame = state.head;
    if (frame == nullptr || (frame->videoData == nullptr &&
        frame->audioData == nullptr && frame->junkBytes == 0))
        return nullptr;
    if (frame->next != nullptr || state.fileOffset != state.fileBytes)
        return frame;
    return nullptr;
}

// PAL239AB8..239B48. The source genuinely ignores the second int parameter.
// A short read zeros exactly the requested output extent, leaving the FIFO.
std::int32_t RingBuffer::GetAudio(void* output, std::int32_t unused, std::int32_t bytes) {
    static_cast<void>(unused);
    if (bytes < 0 || (bytes != 0 && output == nullptr))
        throw std::invalid_argument("Movie audio requires a valid output span");
    auto& state = *storage_;
    if (state.audio.DataAvail() < bytes) {
        if (bytes != 0)
            std::memset(output, 0, static_cast<std::size_t>(bytes));
        return 0;
    }
    const std::int32_t result = state.audio.GetData(output, bytes, state.consumeAudio ? 1 : 0);
    state.consumeAudio = true;
    return result;
}

// PAL239B48..239B9C. Return this very descriptor to the same free list and
// advance/rebase the consumed cursor before any future physical fill.
void RingBuffer::ReleaseFrame() {
    auto& state = *storage_;
    MovieFrame* frame = state.head;
    if (frame == nullptr)
        throw std::logic_error("Movie has no borrowed frame to release");
    state.head = frame->next;
    frame->next = state.free;
    state.free = frame;
    state.consumed = frame->dataOffset + frame->chunkBytes;
    if (state.head != nullptr && state.consumed + state.head->chunkBytes >= state.capacity)
        state.consumed = 0;
}

void RingBuffer::ServicePhysicalRead() {
    auto& state = *storage_;
    if (!state.physicalRead)
        return;
    state.io.Service();
    const auto completion = state.io.State(state.physicalRead);
    if (completion == ::hp2::FileReadState::Pending)
        return;
    if (completion == ::hp2::FileReadState::Failed)
        throw std::runtime_error("Movie physical read failed");
    ASyncDoneCallback();
    state.physicalRead.reset();
}

void RingBuffer::SetFrameLimit(std::uint32_t limit) noexcept {
    storage_->frameLimit = limit;
    storage_->frameLimitBound = true;
}

void RingBuffer::SetAudioConsumption(bool consume) noexcept {
    storage_->consumeAudio = consume;
}

void RingBuffer::SetAudioConsumerActive(bool active) noexcept {
    storage_->audioConsumerActive = active;
}

} // namespace hp2
