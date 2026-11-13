#include "bRepeat.hpp"

#include <limits.h>
#include <string.h>
#include <fstream>
#include <vector>
#include <limits>

static inline uint32_t bitStreamLoadWord(const uint8_t* bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static inline void bitStreamStoreWord(uint8_t* bytes, uint32_t value)
{
    for (uint32_t byte = 0; byte < 4; ++byte)
        bytes[byte] = (uint8_t)(value >> (byte * 8));
}

enum { STREAM_HEADER_BYTES = 48, STREAM_CHANNEL_BYTES = 36, STREAM_MARGIN_BITS = 68 };


BitStreamResult bitStreamInitialize(BitStream* stream, uint8_t* storage,
                                    uint32_t capacity, uint32_t channelCount)
{
    if (!stream || !storage || channelCount == 0 || channelCount > BIT_STREAM_MAX_CHANNELS ||
        capacity > INT32_MAX / 8 || capacity < STREAM_HEADER_BYTES + channelCount * STREAM_CHANNEL_BYTES + 9)
        return BIT_STREAM_INVALID_STORAGE;
    memset(stream, 0, sizeof(*stream));
    stream->bytes = storage;
    stream->capacity = capacity;
    stream->storageBytes = capacity;
    stream->channelCount = channelCount;
    stream->dataStart = STREAM_HEADER_BYTES + channelCount * STREAM_CHANNEL_BYTES;
    stream->writePosition = stream->dataStart * 8;
    memset(storage, 0, capacity);
    return bitStreamStore(stream);
}

void bitStreamWriteBitsAt(uint8_t* bytes, uint32_t position, uint32_t value, uint32_t bits)
{
    for (uint32_t index = 0; index < bits; ++index) {
        const uint32_t bit = position + index;
        const uint8_t mask = (uint8_t)(1u << (bit & 7u));
        uint8_t* byte = &bytes[bit >> 3];
        *byte = (uint8_t)((*byte & (uint8_t)~mask) | ((value >> (index & 31u) & 1u) << (bit & 7u)));
    }
}

uint32_t bitStreamReadBitsAt(const uint8_t* bytes, uint32_t position, uint32_t bits)
{
    uint32_t value = 0;
    for (uint32_t index = 0; index < bits; ++index) {
        const uint32_t bit = position + index;
        value |= ((uint32_t)(bytes[bit >> 3] >> (bit & 7u)) & 1u) << (index & 31u);
    }
    return value;
}

void bitStreamBeginRead(BitStream* stream)
{
    stream->readPosition = stream->dataStart * 8;
    stream->readCount = 0;
    stream->readError = 0;
    for (uint32_t index = 0; index < stream->channelCount; ++index) {
        stream->channels[index].readRepeats = 0;
        stream->channels[index].readRepeatPosition = 0;
    }
    (void)bitStreamStore(stream);
}

static bool bitStreamReadableSpan(const BitStream* stream, uint32_t position, uint32_t bits)
{
    return stream->bytes && stream->storageBytes <= INT32_MAX / 8 &&
        stream->writePosition <= stream->storageBytes * 8 &&
        position <= stream->writePosition && bits <= stream->writePosition - position;
}

uint32_t bitStreamRead(BitStream* stream, uint32_t bits, uint32_t channelIndex)
{
    if (!stream || channelIndex >= stream->channelCount) {
        if (stream) stream->result = BIT_STREAM_INVALID_CHANNEL;
        return 0;
    }
    if (bits > 32) { stream->result = BIT_STREAM_INVALID_WIDTH; return 0; }
    BitStreamChannel* channel = &stream->channels[channelIndex];
    uint32_t value = channel->defaultValue;
    if (channel->repeatBits > 31 ||
        (channel->readRepeatPosition && !bitStreamReadableSpan(stream, channel->readRepeatPosition, channel->repeatBits))) {
        stream->readError = 1;
    } else if (channel->readRepeatPosition && channel->readRepeats <
        bitStreamReadBitsAt(stream->bytes, channel->readRepeatPosition, channel->repeatBits)) {
        ++channel->readRepeats;
        value = channel->currentValue;
    } else if (stream->readCount >= stream->writeCount || !bitStreamReadableSpan(stream, stream->readPosition, 4)) {
        stream->readError = 1;
    } else {
        const uint32_t storedChannel = bitStreamReadBitsAt(stream->bytes, stream->readPosition, 4);
        stream->readPosition += 4;
        if (storedChannel != channelIndex || !bitStreamReadableSpan(stream, stream->readPosition, channel->repeatBits + bits)) {
            stream->readError = 1;
        } else {
            channel->readRepeatPosition = stream->readPosition;
            channel->readRepeats = 0;
            stream->readPosition += channel->repeatBits;
            value = bitStreamReadBitsAt(stream->bytes, stream->readPosition, bits);
            stream->readPosition += bits;
            channel->currentValue = value;
        }
    }
    ++stream->readCount;
    (void)bitStreamStore(stream);
    return value;
}

BitStreamResult bitStreamStore(BitStream* stream)
{
    if (!stream || !stream->bytes || !stream->channelCount || stream->channelCount > BIT_STREAM_MAX_CHANNELS ||
        stream->capacity < stream->dataStart || stream->storageBytes < stream->dataStart ||
        stream->dataStart != STREAM_HEADER_BYTES + stream->channelCount * STREAM_CHANNEL_BYTES)
        return BIT_STREAM_INVALID_STORAGE;
    const uint32_t words[12] = {UINT32_C(0x12345678), stream->flushCount, stream->writePosition,
        stream->dataStart, stream->readPosition, stream->capacity, stream->channelCount, stream->flushEnabled,
        stream->writeCount, stream->readCount, stream->readError, stream->lastFrame};
    for (uint32_t index = 0; index < 12; ++index)
        bitStreamStoreWord(stream->bytes + index * 4, words[index]);
    for (uint32_t index = 0; index < stream->channelCount; ++index) {
        const BitStreamChannel* channel = &stream->channels[index];
        const uint32_t values[9] = {channel->repeatBits, channel->defaultValue, channel->previousWidth,
            channel->writeRepeats, channel->writeRepeatPosition, channel->previousValue,
            channel->readRepeats, channel->readRepeatPosition, channel->currentValue};
        for (uint32_t field = 0; field < 9; ++field)
            bitStreamStoreWord(stream->bytes + STREAM_HEADER_BYTES + index * STREAM_CHANNEL_BYTES + field * 4, values[field]);
    }
    return stream->result = BIT_STREAM_OK;
}

BitStreamResult bitStreamRestore(BitStream* stream, uint8_t* storage, uint32_t capacity)
{
    if (!stream || !storage || capacity < STREAM_HEADER_BYTES || capacity > INT32_MAX / 8)
        return BIT_STREAM_INVALID_STORAGE;
    const uint32_t count = bitStreamLoadWord(storage + 24);
    if (bitStreamLoadWord(storage) != UINT32_C(0x12345678) || count == 0 || count > BIT_STREAM_MAX_CHANNELS)
        return BIT_STREAM_INVALID_HEADER;
    const uint32_t start = STREAM_HEADER_BYTES + count * STREAM_CHANNEL_BYTES;
    const uint32_t recordedCapacity = bitStreamLoadWord(storage + 20);
    const uint32_t written = bitStreamLoadWord(storage + 8);
    // PAL DemoModeReplay::Save13F7E8 writes ((writePosition/8+4)/4)*4
    // bytes of the same live bRepeat image. Its +20 allocation capacity is
    // not rewritten to that smaller extent. Retain that source word, while
    // bounding every native payload access by the actual borrowed chunk.
    if (start > capacity || bitStreamLoadWord(storage + 12) != start ||
        recordedCapacity < start || recordedCapacity > INT32_MAX / 8 ||
        written < start * 8 || written > recordedCapacity * 8 || written > capacity * 8)
        return BIT_STREAM_INVALID_HEADER;
    BitStream restored{};
    restored.bytes = storage;
    restored.capacity = recordedCapacity;
    restored.storageBytes = capacity;
    restored.dataStart = start;
    restored.channelCount = count;
    restored.flushCount = bitStreamLoadWord(storage + 4);
    restored.writePosition = written;
    restored.readPosition = bitStreamLoadWord(storage + 16);
    restored.flushEnabled = bitStreamLoadWord(storage + 28);
    restored.writeCount = bitStreamLoadWord(storage + 32);
    restored.readCount = bitStreamLoadWord(storage + 36);
    restored.readError = bitStreamLoadWord(storage + 40);
    restored.lastFrame = bitStreamLoadWord(storage + 44);
    if (restored.readPosition > written ||
        (restored.readPosition && restored.readPosition < start * 8))
        return BIT_STREAM_INVALID_HEADER;
    for (uint32_t index = 0; index < count; ++index) {
        const uint8_t* bytes = storage + STREAM_HEADER_BYTES + index * STREAM_CHANNEL_BYTES;
        BitStreamChannel* channel = &restored.channels[index];
        channel->repeatBits = bitStreamLoadWord(bytes);
        channel->defaultValue = bitStreamLoadWord(bytes + 4);
        channel->previousWidth = bitStreamLoadWord(bytes + 8);
        channel->writeRepeats = bitStreamLoadWord(bytes + 12);
        channel->writeRepeatPosition = bitStreamLoadWord(bytes + 16);
        channel->previousValue = bitStreamLoadWord(bytes + 20);
        channel->readRepeats = bitStreamLoadWord(bytes + 24);
        channel->readRepeatPosition = bitStreamLoadWord(bytes + 28);
        channel->currentValue = bitStreamLoadWord(bytes + 32);
        const auto validPosition = [&](uint32_t position) {
            return !position || (position >= start * 8 + 4 && position <= written &&
                channel->repeatBits <= written - position);
        };
        if (channel->repeatBits > 31 || channel->previousWidth > 32 ||
            !validPosition(channel->writeRepeatPosition) || !validPosition(channel->readRepeatPosition))
            return BIT_STREAM_INVALID_HEADER;
    }
    *stream = restored;
    return BIT_STREAM_OK;
}

BitStreamResult bitStreamConfigureChannel(BitStream* stream, uint32_t channel,
                                          uint32_t repeatBits, uint32_t defaultValue)
{
    if (!stream || channel >= stream->channelCount) return BIT_STREAM_INVALID_CHANNEL;
    if (repeatBits > 31) return stream->result = BIT_STREAM_INVALID_WIDTH;
    stream->channels[channel].repeatBits = repeatBits;
    stream->channels[channel].defaultValue = defaultValue;
    return bitStreamStore(stream);
}

BitStreamResult bitStreamFlush(BitStream* stream)
{
    const uint32_t first = stream->dataStart;
    const uint32_t end = stream->writePosition >> 3;
    const uint32_t count = end > first ? end - first : 0;
    BitStreamOutput* output = stream->output;
    if (count && stream->transport) {
        if (!stream->transport->AppendPayload(stream->bytes+first,count))
            return stream->result=BIT_STREAM_TRANSPORT_FAILURE;
    } else if (count && (!output || !output->bytes || output->size > output->capacity || count > output->capacity - output->size))
        return stream->result = BIT_STREAM_OUTPUT_CAPACITY;
    else if (count) {
        memcpy(output->bytes + output->size, stream->bytes + first, count);
        output->size += count;
    }
    for (uint32_t index = 0; index < stream->channelCount; ++index)
        stream->channels[index].writeRepeatPosition = 0;
    const uint32_t remaining = stream->writePosition & 7u;
    const uint32_t value = bitStreamReadBitsAt(stream->bytes, end * 8, remaining);
    bitStreamWriteBitsAt(stream->bytes, first * 8, value, remaining);
    stream->writePosition = first * 8 + remaining;
    return bitStreamStore(stream);
}

struct RepeatFileTransport::Storage {
    std::vector<uint8_t> input;
    bool available=false;
    std::fstream output;
    uint32_t payloadBytes=0;
};

RepeatFileTransport::RepeatFileTransport():storage_(std::make_unique<Storage>()) {}
RepeatFileTransport::~RepeatFileTransport()=default;

bool RepeatFileTransport::ReadInput(const std::string& path)
{
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if (!file)return false;
    const auto end=file.tellg();
    if (end<STREAM_HEADER_BYTES || end>INT32_MAX/8)return false;
    storage_->input.resize(static_cast<size_t>(end));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(storage_->input.data()),end)) {
        storage_->input.clear();return false;
    }
    storage_->available=true;
    return true;
}
uint8_t* RepeatFileTransport::InputData() noexcept { return storage_->input.data(); }
uint32_t RepeatFileTransport::InputSize() const noexcept { return static_cast<uint32_t>(storage_->input.size()); }
bool RepeatFileTransport::InputAvailable() const noexcept { return storage_->available; }
void RepeatFileTransport::FinishInput() noexcept { storage_->available=false; }

bool RepeatFileTransport::BeginOutput(const std::string& path,const BitStream& stream)
{
    if (path.empty() || !stream.bytes)return false;
    storage_->output.open(path,std::ios::binary|std::ios::in|std::ios::out|std::ios::trunc);
    if (!storage_->output)return false;
    storage_->payloadBytes=0;
    storage_->output.write(reinterpret_cast<const char*>(stream.bytes),stream.dataStart);
    return static_cast<bool>(storage_->output);
}
bool RepeatFileTransport::AppendPayload(const uint8_t* bytes,uint32_t count)
{
    if (!storage_->output || !bytes || count>static_cast<uint32_t>(INT32_MAX/8)-storage_->payloadBytes)return false;
    // The original 0x41 Codeine messages carry at most 2016 payload bytes.
    for (uint32_t offset=0;offset<count;) {
        const uint32_t size=(count-offset)>2016u ? 2016u : count-offset;
        storage_->output.write(reinterpret_cast<const char*>(bytes+offset),size);
        if (!storage_->output)return false;
        offset+=size;
    }
    storage_->payloadBytes+=count;
    return true;
}
bool RepeatFileTransport::FinishOutput(BitStream& stream)
{
    if (!storage_->output || !stream.bytes)return false;
    const uint32_t pendingBits=stream.writePosition-stream.dataStart*8;
    const uint32_t pendingBytes=(pendingBits+7)/8;
    const uint64_t total=static_cast<uint64_t>(stream.dataStart)+storage_->payloadBytes+pendingBytes+9;
    if (total>INT32_MAX/8)return false;
    const uint64_t position=static_cast<uint64_t>(stream.dataStart+storage_->payloadBytes)*8+pendingBits;
    if (!AppendPayload(stream.bytes+stream.dataStart,pendingBytes))return false;
    const uint8_t padding[9]{};
    storage_->output.write(reinterpret_cast<const char*>(padding),sizeof(padding));
    // Serialize one file view of the same stream. No live channel/timer state
    // is changed: packet-local repeat positions have no meaning in this file.
    std::vector<uint8_t> header(stream.dataStart);
    BitStream serialized=stream;
    serialized.bytes=header.data();serialized.capacity=static_cast<uint32_t>(total);
    serialized.storageBytes=static_cast<uint32_t>(header.size());
    serialized.writePosition=static_cast<uint32_t>(position);
    serialized.readPosition=serialized.dataStart*8;serialized.readCount=0;serialized.readError=0;
    for (uint32_t i=0;i<serialized.channelCount;++i) {
        serialized.channels[i].writeRepeatPosition=0;
        serialized.channels[i].readRepeats=0;serialized.channels[i].readRepeatPosition=0;
    }
    if (bitStreamStore(&serialized)!=BIT_STREAM_OK)return false;
    storage_->output.seekp(0);
    storage_->output.write(reinterpret_cast<const char*>(header.data()),header.size());
    storage_->output.flush();
    const bool result=static_cast<bool>(storage_->output);
    storage_->output.close();
    return result;
}

int32_t bitStreamWrite(BitStream* stream, uint32_t value, uint32_t bits, uint32_t channelIndex)
{
    if (!stream) return 0;
    if (channelIndex >= stream->channelCount) { stream->result = BIT_STREAM_INVALID_CHANNEL; return 0; }
    if (bits > 32) { stream->result = BIT_STREAM_INVALID_WIDTH; return 0; }
    const uint32_t writableBytes = stream->capacity < stream->storageBytes ? stream->capacity : stream->storageBytes;
    if (!stream->bytes || writableBytes > INT32_MAX / 8 || writableBytes < 9 ||
        stream->writePosition >= writableBytes * 8 - STREAM_MARGIN_BITS) return 0;
    BitStreamChannel* channel = &stream->channels[channelIndex];
    ++stream->writeCount;
    const uint32_t repeatLimit = (UINT32_C(1) << channel->repeatBits) - 1;
    if (channel->writeRepeatPosition && channel->previousWidth == bits &&
        channel->writeRepeats != repeatLimit && channel->previousValue == value) {
        ++channel->writeRepeats;
        bitStreamWriteBitsAt(stream->bytes, channel->writeRepeatPosition, channel->writeRepeats, channel->repeatBits);
    } else {
        bitStreamWriteBitsAt(stream->bytes, stream->writePosition, channelIndex, 4);
        stream->writePosition += 4;
        channel->writeRepeatPosition = stream->writePosition;
        bitStreamWriteBitsAt(stream->bytes, stream->writePosition, 0, channel->repeatBits);
        channel->writeRepeats = 0;
        stream->writePosition += channel->repeatBits;
        bitStreamWriteBitsAt(stream->bytes, stream->writePosition, value, bits);
        stream->writePosition += bits;
        channel->previousWidth = bits;
        channel->previousValue = value;
    }
    if (stream->flushEnabled) {
        ++stream->flushCount;
        if (stream->writePosition >= stream->capacity * 8 - STREAM_MARGIN_BITS && bitStreamFlush(stream) != BIT_STREAM_OK)
            return 0;
    }
    (void)bitStreamStore(stream);
    return 1;
}
