#ifndef NFSHP2_BREPEAT_HPP
#define NFSHP2_BREPEAT_HPP

#include <stddef.h>
#include <stdint.h>
struct RepeatFileTransport;

#ifdef __cplusplus
extern "C" {
#endif

#define BIT_STREAM_MAX_CHANNELS 16

typedef enum BitStreamResult {
    BIT_STREAM_OK,
    BIT_STREAM_INVALID_STORAGE,
    BIT_STREAM_INVALID_CHANNEL,
    BIT_STREAM_INVALID_WIDTH,
    BIT_STREAM_OUTPUT_CAPACITY,
    BIT_STREAM_INVALID_HEADER,
    BIT_STREAM_TRANSPORT_FAILURE
} BitStreamResult;

typedef struct BitStreamChannel {
    uint32_t repeatBits;
    uint32_t defaultValue;
    uint32_t previousWidth;
    uint32_t writeRepeats;
    uint32_t writeRepeatPosition;
    uint32_t previousValue;
    uint32_t readRepeats;
    uint32_t readRepeatPosition;
    uint32_t currentValue;
} BitStreamChannel;

/* The transport consumes complete bytes. A native byte buffer replaces the
 * console host-transfer channel; partial trailing bits stay in the stream. */
typedef struct BitStreamOutput {
    uint8_t* bytes;
    size_t capacity;
    size_t size;
} BitStreamOutput;

/* Storage and output are borrowed. Channel state is decoded into named native
 * fields; the serialized header is written only at the file boundary. */
typedef struct BitStream {
    uint8_t* bytes;
    uint32_t capacity;
    uint32_t dataStart;
    uint32_t writePosition;
    uint32_t readPosition;
    uint32_t writeCount;
    uint32_t readCount;
    uint32_t readError;
    uint32_t flushCount;
    uint32_t flushEnabled;
    uint32_t lastFrame;
    uint32_t channelCount;
    BitStreamChannel channels[BIT_STREAM_MAX_CHANNELS];
    BitStreamOutput* output;
    BitStreamResult result;
    /* Same byte transport that owns a Joylog file. Existing borrowed output
     * buffers remain valid for embedded replay streams. */
    struct RepeatFileTransport* transport;
    /* Actual borrowed byte extent. The original capacity word is the
     * recording allocation, which can exceed the saved demo chunk size. */
    uint32_t storageBytes;
} BitStream;

BitStreamResult bitStreamInitialize(BitStream* stream, uint8_t* storage,
                                    uint32_t capacity, uint32_t channelCount);
BitStreamResult bitStreamRestore(BitStream* stream, uint8_t* storage, uint32_t capacity);
BitStreamResult bitStreamStore(BitStream* stream);
BitStreamResult bitStreamConfigureChannel(BitStream* stream, uint32_t channel,
                                          uint32_t repeatBits, uint32_t defaultValue);
void bitStreamWriteBitsAt(uint8_t* bytes, uint32_t position, uint32_t value, uint32_t bits);
uint32_t bitStreamReadBitsAt(const uint8_t* bytes, uint32_t position, uint32_t bits);
BitStreamResult bitStreamFlush(BitStream* stream);
int32_t bitStreamWrite(BitStream* stream, uint32_t value, uint32_t bits, uint32_t channel);
uint32_t bitStreamRead(BitStream* stream, uint32_t bits, uint32_t channel);
void bitStreamBeginRead(BitStream* stream);

#ifdef __cplusplus
}
#include <memory>
#include <string>
/* Windows replacement for the three Codeine buffer/payload messages. The
 * file contains the real bRepeat header and concatenated source payloads. */
struct RepeatFileTransport {
    RepeatFileTransport();
    ~RepeatFileTransport();
    RepeatFileTransport(const RepeatFileTransport&)=delete;
    RepeatFileTransport& operator=(const RepeatFileTransport&)=delete;
    bool ReadInput(const std::string& path);
    uint8_t* InputData() noexcept;
    uint32_t InputSize() const noexcept;
    bool InputAvailable() const noexcept;
    void FinishInput() noexcept;
    bool BeginOutput(const std::string& path,const BitStream& stream);
    bool AppendPayload(const uint8_t* bytes,uint32_t count);
    bool FinishOutput(BitStream& stream);
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
#endif
#endif
