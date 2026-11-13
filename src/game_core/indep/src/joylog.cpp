#include "Joylog.hpp"
#include <vector>

static int streamFull(const BitStream* stream)
{
    return stream->writePosition >= stream->capacity * 8u - 68u;
}

uint32_t replayIntegerLogRead(ReplayIntegerLog* log, uint32_t bits, uint32_t channel)
{
    if (!log->stream) return 0;
    BitStream* stream = log->stream;
    const uint32_t value = bitStreamRead(stream, bits, channel);
    if (stream->readError) {
        ++log->readFailures;
        log->stream = NULL;
        log->replaying = 0;
        if (stream->transport)stream->transport->FinishInput();
    } else if (stream->readCount >= stream->writeCount) {
        log->stream = NULL;
        log->replaying = 0;
        if (stream->transport)stream->transport->FinishInput();
    }
    return value;
}

struct JoylogFile::Storage {
    explicit Storage(const GameTime& clock) { log.clock=&clock; }
    ReplayIntegerLog log{};
    BitStream stream{};
    std::vector<uint8_t> recording;
    RepeatFileTransport transport;
    bool initialized=false;
    bool outputOpen=false;
};
JoylogFile::JoylogFile(const GameTime& clock):storage_(std::make_unique<Storage>(clock)) {}
JoylogFile::~JoylogFile() { (void)Finish(); }
ReplayIntegerLog& JoylogFile::Log() noexcept { return storage_->log; }

JoylogResult JoylogFile::Initialize(int32_t applicationMode,int32_t enabled,
    const std::string& input,const std::string& output,JoylogConfigItems* config)
{
    auto& owner=*storage_;
    if (owner.initialized)return JoylogResult::InvalidBinding;
    // These two independent actual launch gates precede every allocation.
    if (applicationMode!=1 || !enabled)return JoylogResult::Ok;
    if (!config || !joylogConfigBindingsValid(*config))return JoylogResult::InvalidBinding;
    if (!input.empty()) {
        if (!owner.transport.ReadInput(input))return JoylogResult::FileFailure;
        if (bitStreamRestore(&owner.stream,owner.transport.InputData(),owner.transport.InputSize())!=BIT_STREAM_OK)
            return JoylogResult::InvalidInput;
        bitStreamBeginRead(&owner.stream);
        owner.log.replaying=1;
    } else {
        owner.recording.resize(16384);
        if (bitStreamInitialize(&owner.stream,owner.recording.data(),16384,8)!=BIT_STREAM_OK)
            return JoylogResult::StreamFailure;
        owner.stream.flushEnabled=1;
        const uint32_t repeat[8]={0,8,4,0,0,16,16,8};
        for (uint32_t i=0;i<8;++i)
            if (bitStreamConfigureChannel(&owner.stream,i,repeat[i],i==2 ? 1u : 0u)!=BIT_STREAM_OK)
                return JoylogResult::StreamFailure;
        if (!owner.transport.BeginOutput(output,owner.stream))return JoylogResult::FileFailure;
        owner.outputOpen=true;owner.log.recording=1;
    }
    owner.stream.transport=&owner.transport;owner.log.stream=&owner.stream;
    owner.initialized=true;
    if (owner.log.replaying && !joylogLoadConfigItems(owner.log,*config))return JoylogResult::StreamFailure;
    // Source Init tails SaveConfigItems even during playback; AddData's own
    // recording gate suppresses those writes, not a fake snapshot callback.
    if (!joylogSaveConfigItems(owner.log,*config))return JoylogResult::StreamFailure;
    return JoylogResult::Ok;
}
JoylogResult JoylogFile::Finish()
{
    auto& owner=*storage_;
    if (owner.outputOpen) {
        if (!owner.transport.FinishOutput(owner.stream))return JoylogResult::FileFailure;
        owner.outputOpen=false;
    }
    owner.log.recording=0;owner.log.replaying=0;owner.log.stream=nullptr;
    owner.transport.FinishInput();
    return JoylogResult::Ok;
}

uint32_t replayIntegerLogReadUnsigned(ReplayIntegerLog* log, uint32_t bits, uint32_t channel)
{
    const uint32_t value = replayIntegerLogRead(log, bits, channel);
    const uint32_t shift = (32u - bits) & 31u;
    return (value << shift) >> shift;
}

void replayIntegerLogReadBytes(ReplayIntegerLog* log, uint8_t* bytes, int32_t count, uint32_t channel)
{
    for (int32_t index = 0; index < count; ++index)
        bytes[index] = (uint8_t)replayIntegerLogRead(log, 8, channel);
}

void replayIntegerLogWrite(ReplayIntegerLog* log, uint32_t value, uint32_t bits, uint32_t channel)
{
    if (!log->stream || !log->recording) return;
    BitStream* stream = log->stream;
    (void)bitStreamWrite(stream, value, bits, channel);
    if (streamFull(stream)) log->recording = 0;
    stream->lastFrame = (uint32_t)log->clock->realTimeFrames;
    (void)bitStreamStore(stream);
}

void replayIntegerLogWriteBytes(ReplayIntegerLog* log, const uint8_t* bytes, int32_t count, uint32_t channel)
{
    for (int32_t index = 0; index < count; ++index)
        replayIntegerLogWrite(log, (uint32_t)(int32_t)(int8_t)bytes[index], 8, channel);
}
