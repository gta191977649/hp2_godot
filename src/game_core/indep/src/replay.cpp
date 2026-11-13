#include "indep/inc/Replay.hpp"
#include "indep/inc/Timer.hpp"
#include <cstring>
#include "indep/inc/DemoModeReplay.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/CarDriver.hpp"
#include <algorithm>
#include <cstdio>

void ReplayStreamIndex::Build(const BitStream& source)
{
    hp2::Require(source.bytes && source.channelCount==4 && source.dataStart==192 &&
        source.storageBytes<=INT32_MAX/8 && source.storageBytes>=source.dataStart &&
        source.writePosition>=source.dataStart*8u && source.writePosition<=source.storageBytes*8ull,
        "Demo index has no bounded four-channel recorded stream");
    constexpr std::uint32_t repeats[4]={8,4,0,8};
    for(std::size_t i=0;i<4;++i)
        hp2::Require(source.channels[i].repeatBits==repeats[i] &&
            source.channels[i].defaultValue==(i==1?1u:0u),
            "Demo stream channel configuration differs from the released replay writer");
    // bRepeat publishes reader cursors into its header. Validate an owned
    // copy so preflight cannot mutate the recording or its live reader.
    std::vector<std::uint8_t> bytes(source.bytes,source.bytes+source.storageBytes);
    BitStream reader=source;reader.bytes=bytes.data();reader.output=nullptr;reader.transport=nullptr;
    bitStreamBeginRead(&reader);
    std::vector<Boundary> boundaries;
    const auto save=[&]() {
        Boundary boundary;boundary.position=reader.readPosition;boundary.count=reader.readCount;
        for(std::size_t i=0;i<4;++i) {
            const auto& channel=reader.channels[i];
            boundary.channels[i]={channel.readRepeats,channel.readRepeatPosition,
                channel.readRepeatPosition?channel.currentValue:0u};
        }
        boundaries.push_back(boundary);
    };
    const auto read=[&](std::uint32_t bits,std::uint32_t channel) {
        hp2::Require(reader.readCount<reader.writeCount,"Demo frame is truncated before its required value");
        const auto value=bitStreamRead(&reader,bits,channel);
        hp2::Require(!reader.readError && reader.result==BIT_STREAM_OK,
            "Demo frame does not follow the recorded timestep/input/checksum channel order");
        return value;
    };
    save();
    // Channel2 has repeatBits0: each completed frame consumes at least its
    // four-bit channel tag and eight-bit checksum, even with repeated input.
    const auto maximumFrames=(source.writePosition-source.dataStart*8u)/12u;
    while(reader.readCount<reader.writeCount) {
        hp2::Require(boundaries.size()<=maximumFrames,"Demo frame count exceeds its actual encoded bit extent");
        (void)read(8,1);
        const auto events=read(8,0);
        for(std::uint32_t i=0;i<events;++i)
            hp2::Require((read(19,0)>>16)<2,"Demo input contains an unsupported joystick port");
        (void)read(8,2);
        save();
    }
    hp2::Require(reader.readCount==reader.writeCount && reader.readPosition==reader.writePosition,
        "Demo does not end on the exact complete-frame boundary");
    boundaries_.swap(boundaries);writtenBits_=source.writePosition;writtenValues_=source.writeCount;
}

std::size_t ReplayStreamIndex::FindFrame(const BitStream& reader) const noexcept
{
    if(reader.channelCount!=4 || reader.dataStart!=192 || reader.result!=BIT_STREAM_OK ||
        reader.readError || reader.writePosition!=writtenBits_ ||
        reader.writeCount!=writtenValues_)return NoFrame;
    const auto found=std::lower_bound(boundaries_.begin(),boundaries_.end(),reader.readCount,
        [](const Boundary& boundary,std::uint32_t count){return boundary.count<count;});
    if(found==boundaries_.end() || found->count!=reader.readCount || found->position!=reader.readPosition)return NoFrame;
    for(std::size_t i=0;i<4;++i) {
        const auto& channel=reader.channels[i];const auto& expected=found->channels[i];
        if(channel.readRepeats!=expected.repeats || channel.readRepeatPosition!=expected.position ||
            (channel.readRepeatPosition && channel.currentValue!=expected.value))return NoFrame;
    }
    return static_cast<std::size_t>(found-boundaries_.begin());
}

// Full PAL13E008..13E074, individual source proof53. Recording takes
// precedence if both flags are set, and must not clear restoring/paused.
void replayManagerEndFrame(ReplayManager* replay,const GameTime* clock)
{
    if(replay->recording) {
        const uint32_t loop=static_cast<uint32_t>(clock->worldLoopCounter)+1u;
        std::memcpy(&replay->endCapturingLoopCount,&loop,sizeof(loop));
        // The actual Timer(float)136DD0 performs the two separate binary32
        // operations before truncation. Preserve modular MIPS ADDU below.
        const float scaled=clock->worldTimeElapsed*4000.0f;
        const int32_t elapsed=convertFloatToInt32(scaled+0.5f);
        const uint32_t end=static_cast<uint32_t>(clock->worldTimer)+static_cast<uint32_t>(elapsed);
        replay->recording=0;
        std::memcpy(&replay->endCapturingTime,&end,sizeof(end));
    } else if(replay->restoring) {
        replay->replayPausedThisFrame=0;
        replay->restoring=0;
    }
}

namespace {
bool TransferBinding(ReplaySnapshot* snapshot,const void* values,int32_t bytes,int32_t maximum)
{
    if (!snapshot || snapshot->result!=REPLAY_SNAPSHOT_OK)return false;
    if (!snapshot->bytes || (!values && bytes>0)) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return false;
    }
    // The released PAL caller uses maximumBytes=-1 for a raw, unextended
    // transfer. Most World/Race/AI snapshot fields, including floats and
    // matrices, use that exact convention. Rejecting -1 makes every authentic
    // Demo replay fail even though its source bytes are intact.
    if (bytes<0 || (maximum!=-1 && maximum<bytes) || snapshot->cursor<0) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;return false;
    }
    return true;
}
}

void replaySnapshotInitialize(ReplaySnapshot* snapshot,const char* name,const GameTime* clock)
{
    if (!snapshot)return;
    if (!name || !clock || std::strlen(name)>=sizeof(snapshot->name)) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
    }
    std::memcpy(snapshot->name,name,std::strlen(name)+1);
    snapshot->worldTimer=clock->worldTimer;
    snapshot->worldLoopCounter=clock->worldLoopCounter;
    snapshot->sectionDepth=-1;snapshot->debugDepth=-1;
    snapshot->dataSize=0;
    snapshot->recording=0;snapshot->restoring=0;
    snapshot->recordingChecksums=0;snapshot->verifyingChecksums=0;
    snapshot->overflowReported=0;snapshot->nextSectionOrdinal=0;
    snapshot->debugLoopCounter=0;snapshot->cursor=0;
    snapshot->clock=clock;snapshot->result=REPLAY_SNAPSHOT_OK;
    snapshot->endLoopCounter=0;snapshot->memoryImageLoaded=0;snapshot->totalInterestingValue=0.0f;
    snapshot->wireFormat=REPLAY_SNAPSHOT_NATIVE;
    snapshot->clockPolicy=clock->worldTimerPolicy;
    snapshot->sourcePlayerTokens[0]=0;snapshot->sourcePlayerTokens[1]=0;
}

uint32_t replaySnapshotChecksum(const ReplaySnapshot* snapshot,const void* values,int32_t bytes,int32_t ignored)
{
    (void)snapshot;(void)ignored;
    const auto* input=static_cast<const uint8_t*>(values);
    uint32_t hash=UINT32_MAX;
    for (int32_t index=0;index<bytes;++index)hash=hash*33u+input[index];
    return hash;
}

void replaySnapshotWriteData(ReplaySnapshot* snapshot,const void* values,int32_t bytes,
    int32_t maximumBytes,int32_t signedExtension)
{
    if (!TransferBinding(snapshot,values,bytes,maximumBytes))return;
    const auto end=static_cast<uint64_t>(snapshot->cursor)+static_cast<uint32_t>(bytes);
    if (end>REPLAY_SNAPSHOT_WRITE_LIMIT || end>snapshot->capacity) {
        snapshot->overflowReported=1;snapshot->result=REPLAY_SNAPSHOT_CAPACITY;return;
    }
    const auto* source=static_cast<const uint8_t*>(values);
    int32_t index=0,cursor=snapshot->cursor;uint8_t last=0;
    // Preserve the source's bytewise forward order, including aliases into
    // this same stream. memmove would change its overlapping-write behavior.
    for (;index<bytes;++index) {
        last=source[index];snapshot->bytes[cursor++]=last;
    }
    const uint8_t extension=signedExtension && (last&0x80u) ? 0xffu : 0;
    for (;index<maximumBytes;++index) {
        if (source[index]!=extension) {
            // Source250AB0 is the noncanonical-width diagnostic. Keep a
            // readable native result; all already-written bytes remain.
            snapshot->result=REPLAY_SNAPSHOT_NONCANONICAL_EXTENSION;snapshot->cursor=cursor;return;
        }
    }
    snapshot->cursor=cursor;
}

void replaySnapshotReadData(ReplaySnapshot* snapshot,void* values,int32_t bytes,
    int32_t maximumBytes,int32_t signedExtension)
{
    if (!TransferBinding(snapshot,values,bytes,maximumBytes))return;
    const auto end=static_cast<uint64_t>(snapshot->cursor)+static_cast<uint32_t>(bytes);
    // A retained resource may carry a corrupt signed payload length. Casting
    // -1 to unsigned would otherwise allow unpublished bytes to be read as a
    // valid snapshot. Keep the source transfer order on every healthy stream.
    if (snapshot->dataSize<0 || static_cast<uint64_t>(snapshot->dataSize)>snapshot->capacity ||
        end>static_cast<uint32_t>(snapshot->dataSize) || end>snapshot->capacity) {
        snapshot->result=REPLAY_SNAPSHOT_READ_BOUNDS;return;
    }
    auto* destination=static_cast<uint8_t*>(values);
    int32_t index=0,cursor=snapshot->cursor;uint8_t last=0;
    for (;index<bytes;++index) {
        last=snapshot->bytes[cursor++];destination[index]=last;
    }
    const uint8_t extension=signedExtension && (last&0x80u) ? 0xffu : 0;
    for (;index<maximumBytes;++index)destination[index]=extension;
    snapshot->cursor=cursor;
}

void replaySnapshotMoveData(ReplaySnapshot* snapshot,void* values,int32_t bytes,
    int32_t maximumBytes,int32_t signedExtension)
{
    if (!snapshot || snapshot->result!=REPLAY_SNAPSHOT_OK)return;
    if (snapshot->sectionDepth<0 || snapshot->sectionDepth>=REPLAY_SNAPSHOT_MAX_SECTIONS) {
        snapshot->result=REPLAY_SNAPSHOT_SECTION_BOUNDS;return;
    }
    auto& section=snapshot->sections[snapshot->sectionDepth];
    ++section.transferCount;
    const auto maximum=maximumBytes==-1 ? bytes : maximumBytes;
    if (snapshot->recording)replaySnapshotWriteData(snapshot,values,bytes,maximum,signedExtension);
    else if (snapshot->restoring)replaySnapshotReadData(snapshot,values,bytes,maximum,signedExtension);
    else if (snapshot->recordingChecksums) {
        const auto checksum=static_cast<uint16_t>(replaySnapshotChecksum(snapshot,values,bytes,signedExtension));
        replaySnapshotWriteData(snapshot,&checksum,2,2,0);
    } else if (snapshot->verifyingChecksums) {
        const auto checksum=static_cast<uint16_t>(replaySnapshotChecksum(snapshot,values,bytes,signedExtension));
        uint16_t recorded=0;
        replaySnapshotReadData(snapshot,&recorded,2,2,0);
        if (snapshot->result==REPLAY_SNAPSHOT_OK && checksum!=recorded) {
            section.mismatchReported=1;
            // PAL13CFF4->13D0A4->250AB0 traps, rather than merely reporting.
            // Native verification returns a sticky typed failure; callers must
            // reject it without installing a hardware exception interpreter.
            snapshot->result=REPLAY_SNAPSHOT_CHECKSUM_MISMATCH;
        }
    }
}

// Complete PAL13D0D0..13D0F4; beta VerifyData(Pviii)13CED0..13CEF4.
// The high halfword at source+3C holds the two checksum modes. Ordinary
// record/restore must not advance this verification-only transfer stream.
void replaySnapshotVerifyData(ReplaySnapshot* snapshot,void* values,int32_t bytes,
    int32_t maximumBytes,int32_t signedExtension)
{
    if(snapshot && (snapshot->recordingChecksums || snapshot->verifyingChecksums))
        replaySnapshotMoveData(snapshot,values,bytes,maximumBytes,signedExtension);
}

// bRepeat::DoSnapshot is defined in the original replay.cpp, PAL13CB90.
// A capture at the live end writes the writer cursor/channel histories;
// restoring consumes those bytes into the reader side of the same stream.
void replaySnapshotBitStream(BitStream* stream,ReplaySnapshot* snapshot,int32_t writingEnd)
{
    hp2::Require(stream && snapshot && stream->channelCount<=BIT_STREAM_MAX_CHANNELS,
        "Replay bRepeat snapshot has no live stream or valid channel count");
    // Preserve the reader's entire live channel/cursor state if a retained
    // replay snapshot is truncated after some of its fields. Publishing the
    // partially read header changes subsequent Demo input/checksum decoding.
    BitStream staged=*stream;
    replaySnapshotBeginSection(snapshot,nullptr,"bRepeat");
    if(writingEnd) {
        replaySnapshotMoveUnsigned(snapshot,&staged.writePosition,4);
        replaySnapshotMoveUnsigned(snapshot,&staged.writeCount,4);
        for(uint32_t index=0;index<stream->channelCount;++index) {
            auto& channel=staged.channels[index];
            replaySnapshotMoveUnsigned(snapshot,&channel.writeRepeats,4);
            replaySnapshotMoveUnsigned(snapshot,&channel.writeRepeatPosition,4);
            replaySnapshotMoveUnsigned(snapshot,&channel.previousValue,4);
        }
    } else {
        staged.readError=0;
        replaySnapshotMoveUnsigned(snapshot,&staged.readPosition,4);
        replaySnapshotMoveUnsigned(snapshot,&staged.readCount,4);
        for(uint32_t index=0;index<stream->channelCount;++index) {
            auto& channel=staged.channels[index];
            replaySnapshotMoveUnsigned(snapshot,&channel.readRepeats,4);
            replaySnapshotMoveUnsigned(snapshot,&channel.readRepeatPosition,4);
            replaySnapshotMoveUnsigned(snapshot,&channel.currentValue,4);
        }
    }
    replaySnapshotEndSection(snapshot);
    if (snapshot->result!=REPLAY_SNAPSHOT_OK)return;
    if(snapshot->restoring) {
        bool valid=staged.readCount<=staged.writeCount && staged.readPosition>=staged.dataStart*8u &&
            staged.readPosition<=staged.writePosition;
        for(std::uint32_t index=0;index<staged.channelCount && valid;++index) {
            const auto& channel=staged.channels[index];
            const auto position=channel.readRepeatPosition;
            if(!position)valid=channel.readRepeats==0;
            else {
                valid=channel.repeatBits<=31 && position>=staged.dataStart*8u+4u &&
                    position<=staged.writePosition && channel.repeatBits<=staged.writePosition-position;
                if(valid)valid=bitStreamReadBitsAt(staged.bytes,position-4u,4)==index &&
                    channel.readRepeats<=bitStreamReadBitsAt(staged.bytes,position,channel.repeatBits);
            }
        }
        if(!valid) {snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;return;}
    }
    hp2::Require(bitStreamStore(&staged)==BIT_STREAM_OK,
        "Replay bRepeat snapshot cannot publish the real stream header");
    *stream=staged;
}

void replaySnapshotBeginSection(ReplaySnapshot* snapshot,const void* object,const char* name)
{
    if (!snapshot)return;
    if (snapshot->sectionDepth>=REPLAY_SNAPSHOT_MAX_SECTIONS-1) {
        snapshot->result=REPLAY_SNAPSHOT_SECTION_BOUNDS;return;
    }
    if (snapshot->sectionDepth==-1)snapshot->nextSectionOrdinal=0;
    ++snapshot->sectionDepth;
    auto& section=snapshot->sections[snapshot->sectionDepth];
    section.transferCount=0;section.mismatchReported=0;
    section.ordinal=snapshot->nextSectionOrdinal++;
    section.byteCount=snapshot->cursor;section.object=object;section.name=name;
    // Source13D0F8 is exactly JR RA/NOP. Source13D3D0/13CB28 here only
    // formats the debug trace when a section breakpoint is selected.
}

void replaySnapshotEndSection(ReplaySnapshot* snapshot)
{
    if (!snapshot)return;
    if (snapshot->sectionDepth<0 || snapshot->sectionDepth>=REPLAY_SNAPSHOT_MAX_SECTIONS) {
        if(snapshot->result==REPLAY_SNAPSHOT_OK)snapshot->result=REPLAY_SNAPSHOT_SECTION_BOUNDS;
        return;
    }
    auto& section=snapshot->sections[snapshot->sectionDepth];
    section.byteCount=snapshot->cursor-section.byteCount;
    --snapshot->sectionDepth;
    if (snapshot->sectionDepth<snapshot->debugDepth)snapshot->debugDepth=-1;
}

void replaySnapshotMoveInteger(ReplaySnapshot* snapshot,int32_t* value,int32_t bytes)
{
    replaySnapshotMoveData(snapshot,value,bytes,4,1);
}

void replaySnapshotMoveUnsigned(ReplaySnapshot* snapshot,uint32_t* value,int32_t bytes)
{
    replaySnapshotMoveData(snapshot,value,bytes,4,0);
}

void replaySnapshotMoveFloat(ReplaySnapshot* snapshot,float* value)
{
    replaySnapshotMoveData(snapshot,value,4,-1,0);
}

// PAL13D100..13D288. The source overload ignores its nominal byte argument.
// A matrix keeps its original stream encoding and is restored in place.
void replaySnapshotMoveMatrix(ReplaySnapshot* snapshot,float matrix[16],int32_t bytes)
{
    (void)bytes;
    int8_t encoding=0;
    if (matrix[3]==0.0f && matrix[7]==0.0f && matrix[11]==0.0f && matrix[15]==1.0f)
        encoding=matrix[12]==0.0f && matrix[13]==0.0f && matrix[14]==0.0f ? 1 : 2;
    replaySnapshotMoveData(snapshot,&encoding,1,1,1);
    if (snapshot->result!=REPLAY_SNAPSHOT_OK)return;
    // Final PAL uses only the three stream layouts 0, 1 and 2. Reject a
    // damaged selector before touching the live transform, rather than
    // interpreting arbitrary bytes as a rotation-only matrix.
    if (encoding<0 || encoding>2) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;return;
    }
    if (snapshot->restoring) {
        // All lanes are one logical world transform. In a damaged Demo or
        // replay file the original sequential reader could copy the first
        // rotation row and then fail on the next row, leaving an invalid
        // physics/camera matrix. Preflight the exact PAL encoded extent; a
        // complete healthy stream still follows its original transfer order.
        const std::uint64_t transferBytes=encoding==0 ? 64u : (encoding==1 ? 36u : 48u);
        if (snapshot->cursor<0 || snapshot->dataSize<0 ||
            static_cast<std::uint64_t>(snapshot->dataSize)>snapshot->capacity ||
            static_cast<std::uint64_t>(snapshot->cursor)+transferBytes>
                static_cast<std::uint64_t>(snapshot->dataSize) ||
            static_cast<std::uint64_t>(snapshot->cursor)+transferBytes>snapshot->capacity) {
            snapshot->result=REPLAY_SNAPSHOT_READ_BOUNDS;
            return;
        }
    }
    if (encoding==0) {
        replaySnapshotMoveData(snapshot,matrix,64,-1,0);
    } else {
        replaySnapshotMoveData(snapshot,matrix,12,-1,0);
        replaySnapshotMoveData(snapshot,matrix+4,12,-1,0);
        replaySnapshotMoveData(snapshot,matrix+8,12,-1,0);
        matrix[15]=1.0f;
        matrix[3]=0.0f;matrix[7]=0.0f;matrix[11]=0.0f;
        if (encoding==2)replaySnapshotMoveData(snapshot,matrix+12,12,-1,0);
        else {matrix[14]=0.0f;matrix[12]=0.0f;matrix[13]=0.0f;}
    }
}

static int streamFull(const BitStream* stream)
{
    return stream->writePosition >= stream->capacity * 8u - 68u;
}

uint32_t replayManagerRead(ReplayManager* replay, uint32_t bits, uint32_t channel)
{
    if (!replay->restoring || replay->exhausted) return 0;
    const uint32_t value = bitStreamRead(replay->stream, bits, channel);
    if (replay->stream->readError) replay->exhausted = 1;
    return value;
}

uint32_t replayManagerReadUnsigned(ReplayManager* replay, uint32_t bits, uint32_t channel)
{
    const uint32_t value = replayManagerRead(replay, bits, channel);
    const uint32_t shift = (32u - bits) & 31u;
    return (value << shift) >> shift;
}

void replayManagerRequestStop(ReplayManager* replay)
{
    if (!replay->stopRequested) replay->stopRequested = 1;
}

void replayManagerWrite(ReplayManager* replay, uint32_t value, uint32_t bits, uint32_t channel)
{
    if (!replay->recording) return;
    (void)bitStreamWrite(replay->stream, value, bits, channel);
    if (streamFull(replay->stream)) {
        replay->recording = 0;
        replayManagerRequestStop(replay);
    }
}

struct ReplayManagerOwner::Storage {
    ReplayManager state{};
    hp2::DemoModeReplay* demo{};
    bool borrowedDemoPayload{};
    GameSession* coordinator{};
    RaceParameters* parameters{};
    const GameTime* clock{};
    const CarDriverAppearanceCatalogue* characters{};
    GameInputRouter* input{};
    std::vector<std::unique_ptr<ReplaySnapshotStorage>> freeSnapshots;
    std::vector<std::unique_ptr<ReplaySnapshotStorage>> snapshots;
    std::unique_ptr<BitStream> repeat;
    std::vector<uint8_t> recordingBytes;
};

void ReplayManagerOwner::BindRace(GameSession& coordinator,RaceParameters& parameters,
    const GameTime& clock,const CarDriverAppearanceCatalogue& characters,GameInputRouter& input)
{
    auto& owner=*storage_;
    hp2::Require(!owner.coordinator && coordinator.bindings && coordinator.bindings->world &&
        coordinator.bindings->replayOwner==this && coordinator.bindings->clock==&clock,
        "Replay snapshot bindings differ from the canonical race owners");
    owner.coordinator=&coordinator;owner.parameters=&parameters;
    owner.clock=&clock;owner.characters=&characters;owner.input=&input;
}

namespace {
std::unique_ptr<ReplaySnapshotStorage> AllocateReplaySnapshot()
{
    auto node=std::make_unique<ReplaySnapshotStorage>();
    // PAL allocates 0x9558 total bytes, of which the actual data area is
    // 0x9400 bytes. Source ctor initializes only the two Timer members;
    // native value initialization makes inactive metadata safe to inspect.
    node->allocation.resize(0x9400);
    node->snapshot.bytes=node->allocation.data();
    node->snapshot.capacity=node->allocation.size();
    node->snapshot.worldTimer=0;node->snapshot.endTime=0;
    return node;
}
}

ReplayManagerOwner::ReplayManagerOwner(hp2::DemoModeReplay& demo)
    : storage_(std::make_unique<Storage>())
{
    auto& owner=*storage_;
    auto& state=owner.state;
    state.owner=this;
    state.firstSourceReferenceMismatchLoop=-1;
    state.lapSnapshotSpacing=15.0f;
    state.nextFancyDebugTime=9999999;
    state.beginCapturingLoopCount=-1;state.endCapturingLoopCount=-1;
    state.lastSnapshotTime=-1;
    hp2::Require(!demo.replayBorrower_,"Demo replay already has a live ReplayManager");
    if (demo.active) {
        hp2::Require(demo.repeat_ && demo.repeat_->bytes && demo.repeat_->channelCount>0 &&
            !demo.snapshots_.empty(),
            "Active Demo cannot borrow a missing recorded stream or empty snapshot list");
        // Allocate before publishing the move, so native allocation failure
        // leaves the original live recording and its byte references intact.
        owner.freeSnapshots.push_back(AllocateReplaySnapshot());
        owner.snapshots=std::move(demo.snapshots_);
        owner.repeat=std::move(demo.repeat_);
        state.stream=owner.repeat.get();
        state.maximumSnapshots=1;
        owner.borrowedDemoPayload=true;
        state.synchronizationPolicy=hp2::ReadU32(demo.metadata_)==8u?
            REPLAY_SYNC_PAL_REFERENCE:REPLAY_SYNC_NATIVE_STRICT;
    }
    owner.demo=&demo;demo.replayBorrower_=this;
}

ReplayManagerOwner::~ReplayManagerOwner()
{
    if (!storage_)return;
    auto& owner=*storage_;
    if (owner.demo && owner.borrowedDemoPayload) {
        // Source returns every memory-image node to the same demo list.
        // Native snapshots are deleted here, and free snapshots below.
        owner.snapshots.erase(std::remove_if(owner.snapshots.begin(),owner.snapshots.end(),
            [](const auto& node){return !node->snapshot.memoryImageLoaded;}),owner.snapshots.end());
        auto& demo=*owner.demo;
        demo.snapshots_=std::move(owner.snapshots);
        // bRepeat belongs to the authored recording independently of how
        // many snapshot nodes survived playback. The old conditional lost
        // the stream whenever all Demo snapshots had been consumed/deleted.
        if(owner.repeat)demo.repeat_=std::move(owner.repeat);
    }
    if(owner.demo)owner.demo->replayBorrower_=nullptr;
    owner.state.stream=nullptr;
    owner.snapshots.clear();owner.freeSnapshots.clear();owner.repeat.reset();
}

ReplayManager& ReplayManagerOwner::State() noexcept {return storage_->state;}
const ReplayManager& ReplayManagerOwner::State() const noexcept {return storage_->state;}
std::size_t ReplayManagerOwner::FreeSnapshotCount() const noexcept {return storage_->freeSnapshots.size();}
std::size_t ReplayManagerOwner::ActiveSnapshotCount() const noexcept
{return storage_->snapshots.size();}
ReplaySnapshot* ReplayManagerOwner::ActiveSnapshot(std::size_t index) noexcept
{
    return index<storage_->snapshots.size()?&storage_->snapshots[index]->snapshot:nullptr;
}
const ReplaySnapshotStorage* ReplayManagerOwner::ActiveSnapshotStorage(std::size_t index) const noexcept
{
    return index<storage_->snapshots.size()?storage_->snapshots[index].get():nullptr;
}

void ReplayManagerOwner::MarkTheBeginningOfTime(const GameTime& clock,std::size_t availableBytes)
{
    auto& owner=*storage_;auto& state=owner.state;
    hp2::Require(!owner.clock || owner.clock==&clock,"Replay begin time uses a different GameTime owner");
    owner.clock=&clock;
    if (state.beginCapturingLoopCount!=-1)return;
    state.endCapturingLoopCount=clock.worldLoopCounter;
    state.beginCapturingLoopCount=clock.worldLoopCounter;
    state.beginCapturingTime=clock.worldTimer;state.endCapturingTime=clock.worldTimer;
    if (availableBytes<0x8000u || availableBytes-0x8000u<0xd0ad8u)return;
    hp2::Require(!owner.repeat && !state.stream,
        "Replay recording cannot replace the live borrowed demo BitStream");
    std::vector<uint8_t> bytes(0x32000);
    auto repeat=std::make_unique<BitStream>();
    hp2::Require(bitStreamInitialize(repeat.get(),bytes.data(),static_cast<uint32_t>(bytes.size()),4)==BIT_STREAM_OK,
        "Replay allocation cannot initialize its actual BitStream");
    const uint32_t repeatBits[4]={8,4,0,8};
    for (uint32_t index=0;index<4;++index)
        hp2::Require(bitStreamConfigureChannel(repeat.get(),index,repeatBits[index],index==1 ? 1u : 0u)==BIT_STREAM_OK,
            "Replay BitStream channel configuration failed");
    std::vector<std::unique_ptr<ReplaySnapshotStorage>> snapshots;
    snapshots.reserve(17);
    for (int32_t index=0;index<17;++index)snapshots.push_back(AllocateReplaySnapshot());
    owner.recordingBytes=std::move(bytes);owner.repeat=std::move(repeat);
    owner.freeSnapshots=std::move(snapshots);
    state.stream=owner.repeat.get();state.maximumSnapshots=17;
    state.maximumInstantReplaySnapshots=2;state.maximumLapSnapshots=8;
    state.maximumHighlightSnapshots=5;state.stopRequested=0;state.maximumAutoReplaySnapshots=0;
}

// PAL13E270 RegisterInterestingEvent: the name argument is unused in the
// released body. The active snapshot list is its sole intensity owner.
void replayManagerAddHighlight(ReplayManager* manager,float intensity)
{
    hp2::Require(manager,"Replay interesting event has no manager");
    if(!manager->recording)return;
    hp2::Require(manager->owner && &manager->owner->State()==manager,
        "Replay interesting event has no canonical active-snapshot owner");
    for(std::size_t index=0;index<manager->owner->ActiveSnapshotCount();++index) {
        auto* snapshot=manager->owner->ActiveSnapshot(index);
        if(!std::strcmp(snapshot->name,"INSTANT_REPLAY"))
            snapshot->totalInterestingValue+=intensity;
    }
}


#include "indep/inc/PauseMenu.hpp"
#include "bware/inc/bWare.hpp"
int replayManagerHandleSpecialEvent(ReplayManager& replay,hp2::RaceScreenServices& services,
    const ReplaySpecialEventInfo& event)
{
    (void)replay;
    return hp2::HandlePauseMenuReplaySpecialEvents(services,event);
}
int replayManagerAddSpecialEvent(ReplayManager& replay,hp2::RaceScreenServices& services,
    int32_t kind,int32_t argument1,int32_t argument2)
{
    hp2::Require(services.frontend && services.frontend->ui && services.frontend->ui->clock,
        "Replay special event has no actual world clock");
    const ReplaySpecialEventInfo event{kind,services.frontend->ui->clock->worldLoopCounter,argument1,argument2};
    if (replay.beginCapturingLoopCount!=-1 && !replay.stopRequested && replay.stream) {
        hp2::Require(replay.specialEventCount>=0,"Replay special event count is corrupt");
        if (replay.specialEventCount<256)replay.specialEvents[replay.specialEventCount++]=event;
        else replayManagerRequestStop(&replay);
    }
    return replayManagerHandleSpecialEvent(replay,services,event);
}

int32_t replayChecksumSnapshotTimes[4]={-1,-1,-1,-1};
namespace {
int32_t ReplayAdd(int32_t a,int32_t b)
{
    const auto bits=static_cast<uint32_t>(a)+static_cast<uint32_t>(b);
    int32_t value;std::memcpy(&value,&bits,sizeof(value));return value;
}
int32_t ReplaySubtract(int32_t a,int32_t b)
{
    const auto bits=static_cast<uint32_t>(a)-static_cast<uint32_t>(b);
    int32_t value;std::memcpy(&value,&bits,sizeof(value));return value;
}
}
ReplaySnapshot* ReplayManagerOwner::FindSnapshot(const char* name,int32_t ordinal) noexcept
{
    if(!name)return nullptr;
    for(auto& owner:storage_->snapshots)
        if(!std::strcmp(owner->snapshot.name,name)) {
            if(ordinal==0)return &owner->snapshot;
            --ordinal;
        }
    return nullptr;
}
void ReplayManagerOwner::DeleteSnapshot(ReplaySnapshot& snapshot)
{
    auto& owner=*storage_;
    const auto found=std::find_if(owner.snapshots.begin(),owner.snapshots.end(),[&](const auto& node) {
        return &node->snapshot==&snapshot;
    });
    hp2::Require(found!=owner.snapshots.end(),"Replay cannot retire a snapshot from a different owner");
    owner.freeSnapshots.push_back(std::move(*found));owner.snapshots.erase(found);
}
bool ReplayManagerOwner::DeleteSnapshot(const char* name)
{
    auto* snapshot=FindSnapshot(name);
    if(!snapshot)return false;
    DeleteSnapshot(*snapshot);return true;
}

void ReplayManagerOwner::SortSnapshots(const char* name)
{
    hp2::Require(name,"Replay snapshot sorting has no name");
    auto& snapshots=storage_->snapshots;
    std::vector<std::unique_ptr<ReplaySnapshotStorage>> matching;
    for(auto item=snapshots.begin();item!=snapshots.end();) {
        if(std::strcmp((*item)->snapshot.name,name)) {++item;continue;}
        matching.push_back(std::move(*item));item=snapshots.erase(item);
    }
    std::stable_sort(matching.begin(),matching.end(),[](const auto& a,const auto& b) {
        return a->snapshot.worldLoopCounter<b->snapshot.worldLoopCounter;
    });
    for(auto& item:matching)snapshots.push_back(std::move(item));
}

namespace {
void TransferManagerState(ReplayManager& state,ReplaySnapshot& snapshot)
{
    replaySnapshotBeginSection(&snapshot,nullptr,"ReplayManager");
    replaySnapshotMoveInteger(&snapshot,&state.beginCapturingLoopCount,4);
    replaySnapshotMoveInteger(&snapshot,&state.endCapturingLoopCount,4);
    replaySnapshotMoveInteger(&snapshot,&state.snapshotsTaken,4);
    hp2::Require(snapshot.result==REPLAY_SNAPSHOT_OK,"Replay manager snapshot header is truncated");
    hp2::Require(state.snapshotsTaken>=0 && state.snapshotsTaken<=4096,
        "Replay manager snapshot exceeds the original snapshot-time allocation");
    for(int32_t index=0;index<state.snapshotsTaken;++index)
        replaySnapshotMoveInteger(&snapshot,&state.snapshotTimes[index],4);
    replaySnapshotMoveInteger(&snapshot,&state.specialEventCount,4);
    hp2::Require(snapshot.result==REPLAY_SNAPSHOT_OK,"Replay manager snapshot times are truncated");
    hp2::Require(state.specialEventCount>=0 && state.specialEventCount<=256,
        "Replay manager snapshot exceeds the original special-event allocation");
    static_assert(sizeof(ReplaySpecialEventInfo)==16);
    for(int32_t index=0;index<state.specialEventCount;++index)
        replaySnapshotMoveData(&snapshot,&state.specialEvents[index],16,-1,0);
    replaySnapshotEndSection(&snapshot);
    hp2::Require(snapshot.result==REPLAY_SNAPSHOT_OK,"Replay manager special events are truncated");
}
}

void ReplayManagerOwner::DoEndSnapshot(ReplaySnapshot& snapshot)
{
    auto& state=storage_->state;
    if(!snapshot.restoring) {TransferManagerState(state,snapshot);return;}
    // Counts are read before their arrays. Do not publish a prefix from a
    // truncated retained manager node into the live replay owner.
    auto staged=std::make_unique<ReplayManager>(state);
    try {TransferManagerState(*staged,snapshot);}
    catch(...) {snapshot.restoring=0;throw;}
    state.beginCapturingLoopCount=staged->beginCapturingLoopCount;
    state.endCapturingLoopCount=staged->endCapturingLoopCount;
    state.snapshotsTaken=staged->snapshotsTaken;
    std::copy_n(staged->snapshotTimes,state.snapshotsTaken,state.snapshotTimes);
    state.specialEventCount=staged->specialEventCount;
    std::copy_n(staged->specialEvents,state.specialEventCount,state.specialEvents);
}

ReplayCaptureRange ReplayManagerOwner::ValidateStateSnapshot(const ReplaySnapshot& source)
{
    auto state=std::make_unique<ReplayManager>();
    ReplaySnapshot snapshot=source;
    snapshot.cursor=0;snapshot.recording=0;snapshot.restoring=1;snapshot.result=REPLAY_SNAPSHOT_OK;
    snapshot.sectionDepth=-1;
    TransferManagerState(*state,snapshot);
    hp2::Require(snapshot.cursor==snapshot.dataSize,"Replay manager snapshot has unconsumed trailing data");
    return {state->beginCapturingLoopCount,state->endCapturingLoopCount};
}

void ReplayManagerOwner::HandleSnapshot(ReplaySnapshot& snapshot)
{
    if(!std::strcmp(snapshot.name,"REPLAY_MANAGER_STATE")) {DoEndSnapshot(snapshot);return;}
    auto& owner=*storage_;auto& state=owner.state;
    hp2::Require(owner.coordinator && owner.parameters && owner.characters && owner.input && owner.clock && state.stream,
        "Replay world snapshot has incomplete actual race/input/stream bindings");
    raceParametersSnapshot(owner.parameters,&snapshot,owner.characters);
    const bool end=(snapshot.recording || snapshot.recordingChecksums) &&
        owner.clock->worldLoopCounter==state.endCapturingLoopCount;
    replaySnapshotBitStream(state.stream,&snapshot,end);
    joystickDoSnapshot(owner.input,&snapshot);
    raceCoordinatorDoSnapshot(*owner.coordinator,snapshot);
}

ReplaySnapshot* ReplayManagerOwner::TakeSnapshot(const char* name)
{
    auto& owner=*storage_;auto& state=owner.state;
    if(state.beginCapturingLoopCount<0)return nullptr;
    hp2::Require(owner.clock,"Replay capture has no actual race clock");
    if(owner.freeSnapshots.empty())owner.freeSnapshots.push_back(AllocateReplaySnapshot());
    auto node=std::move(owner.freeSnapshots.front());owner.freeSnapshots.erase(owner.freeSnapshots.begin());
    auto* snapshot=&node->snapshot;owner.snapshots.push_back(std::move(node));
    const auto oldSnapshotCount=state.snapshotsTaken;
    const auto oldLastSnapshotTime=state.lastSnapshotTime;
    try {
        replaySnapshotInitialize(snapshot,name,owner.clock);
        hp2::Require(snapshot->result==REPLAY_SNAPSHOT_OK,"Replay snapshot initialization failed");
        if(owner.clock->worldLoopCounter==state.endCapturingLoopCount) {
            hp2::Require(state.snapshotsTaken>=0,"Replay snapshot-time count is invalid");
            if(state.snapshotsTaken<4096)
                state.snapshotTimes[state.snapshotsTaken++]=owner.clock->worldLoopCounter;
        }
        state.lastSnapshotTime=owner.clock->worldLoopCounter;
        snapshot->cursor=0;snapshot->recording=1;
        HandleSnapshot(*snapshot);
        snapshot->dataSize=snapshot->cursor;snapshot->recording=0;
        hp2::Require(snapshot->overflowReported || snapshot->result==REPLAY_SNAPSHOT_OK,
            "Replay snapshot capture failed in an actual participant");
    } catch (...) {
        // The real PAL list node cannot remain published after a failed
        // transfer. Return this allocation to its owner and restore the
        // prior snapshot-time cursor, without accepting the failed capture.
        snapshot->recording=0;
        state.snapshotsTaken=oldSnapshotCount;
        state.lastSnapshotTime=oldLastSnapshotTime;
        owner.freeSnapshots.push_back(std::move(owner.snapshots.back()));
        owner.snapshots.pop_back();
        throw;
    }
    if(snapshot->overflowReported) {
        // Capacity rejection is not exceptional, but it is still a failed
        // capture: an end-of-replay phantom time must not be published.
        state.snapshotsTaken=oldSnapshotCount;
        state.lastSnapshotTime=oldLastSnapshotTime;
        DeleteSnapshot(*snapshot);
        return nullptr;
    }
    return snapshot;
}

void ReplayManagerOwner::RestoreSnapshot(ReplaySnapshot& snapshot)
{
    auto& owner=*storage_;
    snapshot.restoring=0;owner.state.restoring=0;
    hp2::Require(owner.clock,"Replay restoration has no actual race clock");
    if(!std::strcmp(snapshot.name,"REPLAY_MANAGER_STATE"))ValidateStateSnapshot(snapshot);
    else if(owner.borrowedDemoPayload && owner.demo && owner.demo->active) {
        hp2::Require(owner.parameters && owner.characters,"Demo restore has no actual parameter/character owners");
        owner.demo->ValidateSnapshotReader(snapshot,*owner.parameters,*owner.characters);
    }
    const BitStream oldStream=owner.state.stream?*owner.state.stream:BitStream{};
    std::vector<std::uint8_t> oldHeader;
    if(oldStream.bytes)oldHeader.assign(oldStream.bytes,oldStream.bytes+oldStream.dataStart);
    RaceParameters oldParameters{};RaceRuntimeSettings oldSettings{};
    std::array<RaceDriverInfo,RACE_DRIVER_LIMIT> oldDrivers{};
    if(owner.parameters) {
        oldParameters=*owner.parameters;
        hp2::Require(oldParameters.drivers && oldParameters.runtimeSettings,
            "Replay restore has no live parameter allocation");
        oldSettings=*oldParameters.runtimeSettings;
        std::copy_n(oldParameters.drivers,RACE_DRIVER_LIMIT,oldDrivers.begin());
    }
    std::array<std::uint8_t,2> oldTypes{};
    if(owner.input)std::copy_n(owner.input->monitoredTypes,2,oldTypes.begin());
    const auto restorePrefix=[&]() noexcept {
        if(oldStream.bytes) {
            std::copy(oldHeader.begin(),oldHeader.end(),oldStream.bytes);
            *owner.state.stream=oldStream;
        }
        if(owner.parameters) {
            *owner.parameters=oldParameters;
            *oldParameters.runtimeSettings=oldSettings;
            std::copy(oldDrivers.begin(),oldDrivers.end(),oldParameters.drivers);
        }
        if(owner.input)std::copy(oldTypes.begin(),oldTypes.end(),owner.input->monitoredTypes);
    };
    snapshot.cursor=0;snapshot.restoring=1;
    // Native transfer status belongs to this attempt; the original snapshot
    // bytes and source record flags are retained across retries.
    snapshot.result=REPLAY_SNAPSHOT_OK;
    try {
        HandleSnapshot(snapshot);
    }
    catch (const std::exception& error) {
        snapshot.restoring=0;restorePrefix();
        if(owner.demo)owner.demo->RecordSessionEvent("demo.restore_error",&snapshot,error.what());
        std::string section;
        for(int32_t depth=0;depth<=snapshot.sectionDepth && depth<REPLAY_SNAPSHOT_MAX_SECTIONS;++depth) {
            if(!section.empty())section+='/';
            if(snapshot.sections[depth].name)section+=snapshot.sections[depth].name;
        }
        throw std::runtime_error("Replay participant restore failed: "+std::string(error.what())+
            " cursor="+std::to_string(snapshot.cursor)+" section="+section);
    }
    catch (...) {
        // The restored data is still invalid and the error must propagate;
        // never leave this retained node permanently in restoring mode.
        snapshot.restoring=0;
        restorePrefix();
        throw;
    }
    snapshot.restoring=0;
    if (snapshot.result!=REPLAY_SNAPSHOT_OK || snapshot.cursor!=snapshot.dataSize) {
        restorePrefix();
        if(owner.demo)owner.demo->RecordSessionEvent("demo.restore_error",&snapshot,"participant status or complete byte consumption failed");
        // Diagnose the first real snapshot transfer failure, rather than
        // losing its section/cursor behind a generic GameFlow exception.
        const auto result=static_cast<int>(snapshot.result);
        throw std::runtime_error("Replay snapshot restoration failed in an actual participant: name="+
            std::string(snapshot.name)+" result="+std::to_string(result)+
            " cursor="+std::to_string(snapshot.cursor)+
            " data="+std::to_string(snapshot.dataSize)+
            " capacity="+std::to_string(snapshot.capacity));
    }
    if(std::strcmp(snapshot.name,"REPLAY_MANAGER_STATE")) {
        auto* world=owner.coordinator->bindings->world;
        hp2::Require(worldValidateReplayState(world,owner.clock)!=0,
            "Replay snapshot restored nonfinite native World/Car state");
        ++world->presentationReplayEpoch;
    }
    owner.state.exhausted=0;owner.state.lastSnapshotTime=owner.clock->worldLoopCounter;
}
bool ReplayManagerOwner::RestoreSnapshot(const char* name)
{
    auto* snapshot=FindSnapshot(name);
    if(!snapshot)return false;
    RestoreSnapshot(*snapshot);return true;
}

bool ReplayManagerOwner::WasSnapshotTakenAtThisTime(int32_t worldLoop)
{
    auto& state=storage_->state;
    hp2::Require(state.snapshotsTaken>=0 && state.snapshotsTaken<=4096 &&
        state.lastSnapshotIndex>=0 && state.lastSnapshotIndex<4096,
        "Replay snapshot-time search has invalid native bounds");
    int32_t first=0;
    if(state.lastSnapshotIndex && state.snapshotTimes[state.lastSnapshotIndex]<worldLoop)
        first=state.lastSnapshotIndex;
    for(int32_t index=first;index<state.snapshotsTaken;++index)
        if(state.snapshotTimes[index]==worldLoop) {state.lastSnapshotIndex=index;return true;}
    return false;
}

void ReplayManagerOwner::MaybeTakeDummySnapshot()
{
    auto& owner=*storage_;auto& state=owner.state;
    hp2::Require(owner.clock,"Replay dummy snapshot has no actual race clock");
    const auto loop=owner.clock->worldLoopCounter;
    bool capture=false;
    if(state.restoring && !state.replayPausedThisFrame && loop!=state.lastSnapshotTime)
        capture=WasSnapshotTakenAtThisTime(loop);
    if(state.recording && (loop%211==0 || ReplayAdd(state.lastSnapshotTime,30)<loop))capture=true;
    if(capture && TakeSnapshot("DUMMY_SNAPSHOT"))DeleteSnapshot("DUMMY_SNAPSHOT");
}

void ReplayManagerOwner::MaybeTakeInstantReplaySnapshot()
{
    auto& owner=*storage_;auto& state=owner.state;
    hp2::Require(owner.clock,"Instant replay snapshot has no actual race clock");
    const auto& clock=*owner.clock;
    if(clock.worldTimeFrames<state.nextInstantReplaySnapshotTime)return;
    state.nextInstantReplaySnapshotTime=ReplayAdd(clock.worldTimeFrames,300);
    if(state.instantReplaySnapshotCount && state.instantReplaySnapshotCount>=state.maximumInstantReplaySnapshots) {
        auto* oldest=FindSnapshot("INSTANT_REPLAY");
        hp2::Require(oldest,"Instant replay count has no matching active snapshot");
        ReplaySnapshot* replacement=nullptr;
        for(int32_t index=0;index<state.maximumHighlightSnapshots;++index) {
            if(index>=state.highlightSnapshotCount) {replacement=nullptr;break;}
            auto* current=FindSnapshot("HIGHLIGHT_SNAPSHOT",index);
            hp2::Require(current,"Replay highlight count has no matching active snapshot");
            if(oldest->worldLoopCounter<current->endLoopCounter) {replacement=current;break;}
            if(!replacement || current->totalInterestingValue<replacement->totalInterestingValue)replacement=current;
        }
        if(oldest->totalInterestingValue>0.0f &&
            (!replacement || replacement->totalInterestingValue<oldest->totalInterestingValue)) {
            if(state.highlightSnapshotCount==state.maximumHighlightSnapshots ||
                (replacement && oldest->worldLoopCounter<replacement->endLoopCounter)) {
                hp2::Require(replacement,"Replay highlight allocation is inconsistent");
                DeleteSnapshot(*replacement);--state.highlightSnapshotCount;
            }
            std::strcpy(oldest->name,"HIGHLIGHT_SNAPSHOT");
            oldest->endLoopCounter=clock.worldLoopCounter;oldest->endTime=clock.worldTimer;
            --state.instantReplaySnapshotCount;++state.highlightSnapshotCount;
        } else {
            DeleteSnapshot("INSTANT_REPLAY");--state.instantReplaySnapshotCount;
        }
    }
    if(TakeSnapshot("INSTANT_REPLAY"))++state.instantReplaySnapshotCount;
}

void ReplayManagerOwner::MaybeTakeLapSnapshot()
{
    auto& owner=*storage_;auto& state=owner.state;
    hp2::Require(owner.clock,"Lap replay snapshot has no actual race clock");
    for(;;) {
        const auto elapsed=ReplaySubtract(owner.clock->worldTimer,state.beginCapturingTime);
        const float due=static_cast<float>(state.lapSnapshotCount)*state.lapSnapshotSpacing;
        if(!(due<=static_cast<float>(elapsed)*0.0002500000118743628f))return;
        if(state.lapSnapshotCount!=state.maximumLapSnapshots) {
            if(TakeSnapshot("LAP_SNAPSHOT"))++state.lapSnapshotCount;
            return;
        }
        state.lapSnapshotSpacing+=state.lapSnapshotSpacing;
        // Source Find(index), increment, Find(index) deletes every other
        // remaining snapshot. Deleting all snapshots would lose its anchor.
        int32_t index=0;
        while(FindSnapshot("LAP_SNAPSHOT",index)) {
            ++index;
            if(auto* discard=FindSnapshot("LAP_SNAPSHOT",index)) {
                DeleteSnapshot(*discard);--state.lapSnapshotCount;
            }
        }
        if(state.lapSnapshotCount>=state.maximumLapSnapshots)return;
    }
}

void ReplayManagerOwner::DoChecksumSnapshot(const char* name)
{
    auto& owner=*storage_;auto& state=owner.state;
    hp2::Require(owner.clock,"Checksum replay snapshot has no actual race clock");
    if(state.recording) {
        hp2::Require(!FindSnapshot(name),"Replay checksum snapshot name is already active");
        if(owner.freeSnapshots.empty())owner.freeSnapshots.push_back(AllocateReplaySnapshot());
        auto node=std::move(owner.freeSnapshots.front());owner.freeSnapshots.erase(owner.freeSnapshots.begin());
        auto* snapshot=&node->snapshot;owner.snapshots.push_back(std::move(node));
        try {
            replaySnapshotInitialize(snapshot,name,owner.clock);
            hp2::Require(snapshot->result==REPLAY_SNAPSHOT_OK,
                "Replay checksum snapshot initialization failed");
            snapshot->cursor=0;snapshot->recordingChecksums=1;
            HandleSnapshot(*snapshot);
            snapshot->recordingChecksums=0;snapshot->dataSize=snapshot->cursor;
            hp2::Require(snapshot->result==REPLAY_SNAPSHOT_OK,"Replay checksum capture failed");
        } catch(...) {
            snapshot->recordingChecksums=0;
            owner.freeSnapshots.push_back(std::move(owner.snapshots.back()));
            owner.snapshots.pop_back();
            throw;
        }
    } else if(state.restoring && !state.replayPausedThisFrame) {
        if(auto* snapshot=FindSnapshot(name)) {
            snapshot->cursor=0;snapshot->verifyingChecksums=1;
            snapshot->result=REPLAY_SNAPSHOT_OK;
            try {HandleSnapshot(*snapshot);}
            catch(...) {snapshot->verifyingChecksums=0;throw;}
            snapshot->verifyingChecksums=0;
            hp2::Require(snapshot->result==REPLAY_SNAPSHOT_OK,"Replay checksum verification could not read the stream");
        }
    }
}

void ReplayManagerOwner::MaybeDoChecksumSnapshot()
{
    hp2::Require(storage_->clock,"Checksum replay selection has no actual race clock");
    constexpr const char* names[4]={"CHECKSUM0","CHECKSUM1","CHECKSUM2","CHECKSUM3"};
    for(int32_t index=0;index<4;++index)
        if(replayChecksumSnapshotTimes[index]==storage_->clock->worldLoopCounter) {
            DoChecksumSnapshot(names[index]);return;
        }
}
void ReplayManagerOwner::BeginRaceHighlights(const GameTime& clock)
{
    storage_->state.nextHighlightNumber=0;BeginNextRaceHighlight(clock);
}
void ReplayManagerOwner::BeginNextRaceHighlight(const GameTime& clock)
{
    auto& state=storage_->state;
    if(state.desynchronized) {
        state.paused=1;state.endCurrentHighlightTime=ReplayAdd(clock.worldLoopCounter,1);return;
    }
    auto* snapshot=FindSnapshot("HIGHLIGHT_SNAPSHOT",state.nextHighlightNumber);
    if(snapshot) {
        state.endCurrentHighlightTime=snapshot->endLoopCounter;
        state.nextHighlightNumber=ReplayAdd(state.nextHighlightNumber,1);
        if(!FindSnapshot("HIGHLIGHT_SNAPSHOT",state.nextHighlightNumber))state.nextHighlightNumber=0;
    } else {
        snapshot=FindSnapshot("LAP_SNAPSHOT",0);
        state.endCurrentHighlightTime=snapshot?state.endCapturingLoopCount:0;
        state.nextHighlightNumber=0;
    }
    if(snapshot)RestoreSnapshot(*snapshot);
}

const RaceParameters& ReplayManagerOwner::Parameters() const
{
    hp2::Require(storage_->parameters,"Replay recording has no live race parameters");
    return *storage_->parameters;
}

void ReplayManagerOwner::DoFancyJoylogDebugging(const GameTime& clock,ReplayIntegerLog& log)
{
    hp2::Require(log.clock==&clock,"Fancy replay uses a different Joylog clock owner");
    auto& state=storage_->state;
    const auto phase=state.fancyReplayInProgress;
    if(phase==0 && log.replaying && clock.worldLoopCounter==ReplayAdd(state.nextFancyDebugTime,-1)) {
        state.fancyReplayInProgress=1;std::fill(std::begin(state.fancyJoyEvents),std::end(state.fancyJoyEvents),uint8_t{});
        state.fancyJoyEvents[0]=0x83;state.fancyJoyEvents[6]=4;state.fancyJoyEvents[11]=4;
        state.fancyJoyEvents[16]=4;state.fancyJoyEvents[23]=9;
        state.fancyJoyEventCount=24;state.fancyJoyEventNumber=0;
    } else if(phase==1) {
        log.replaying=0;state.fancyReplayInProgress=2;
    } else if(phase==2) {
        if(clock.worldLoopCounter!=state.endCapturingLoopCount || !state.paused)return;
        state.nextFancyDebugTime=ReplayAdd(state.nextFancyDebugTime,211);
        DeleteSnapshot("CHECKSUM0");DeleteSnapshot("CHECKSUM1");DeleteSnapshot("CHECKSUM2");DeleteSnapshot("CHECKSUM3");
        replayChecksumSnapshotTimes[0]=ReplayAdd(clock.worldLoopCounter,1);
        replayChecksumSnapshotTimes[1]=ReplayAdd(clock.worldLoopCounter,52);
        replayChecksumSnapshotTimes[2]=ReplayAdd(clock.worldLoopCounter,105);
        replayChecksumSnapshotTimes[3]=ReplayAdd(clock.worldLoopCounter,158);
        state.fancyReplayInProgress=3;std::fill(std::begin(state.fancyJoyEvents),std::end(state.fancyJoyEvents),uint8_t{});
        state.fancyJoyEvents[0]=99;state.fancyJoyEvents[12]=9;
        state.fancyJoyEventCount=13;state.fancyJoyEventNumber=0;
    } else if(phase==3 && state.fancyJoyEventNumber>=state.fancyJoyEventCount) {
        hp2::Require(log.stream && !log.readFailures && !log.stream->readError,
            "Fancy replay cannot resume an exhausted or failed Joylog source");
        log.replaying=1;state.fancyReplayInProgress=0;
    }
}

bool ReplayManagerOwner::WillRestoreFrame(const GameTime& clock) const
{
    const auto& owner=*storage_;const auto& state=owner.state;
    hp2::Require(owner.demo && owner.clock==&clock,"Replay timeline has no actual Demo/race clock owners");
    return state.stream && (owner.demo->active ||
        (clock.worldLoopCounter==state.endCapturingLoopCount && state.paused) ||
        (state.beginCapturingLoopCount<=clock.worldLoopCounter && clock.worldLoopCounter<state.endCapturingLoopCount));
}

void ReplayManagerOwner::BeginFrame(const GameTime& clock,hp2::RaceScreenServices& services)
{
    auto& owner=*storage_;auto& state=owner.state;
    hp2::Require(owner.demo && owner.clock==&clock,"Replay BeginFrame has no actual DemoModeReplay/race clock owners");
    if(state.stream) {
        if(WillRestoreFrame(clock))state.restoring=1;
        else if(clock.worldLoopCounter==state.endCapturingLoopCount && !state.stopRequested)state.recording=1;
    }
    if(state.recording)MaybeTakeInstantReplaySnapshot();
    if(state.recording)MaybeTakeLapSnapshot();
    if(state.restoring) {
        if(state.paused)state.replayPausedThisFrame=1;
        else if(state.slowMotion)state.replayPausedThisFrame=clock.realLoopCounter%4!=0;
        hp2::Require(state.specialEventCount>=0 && state.specialEventCount<=256,
            "Replay special-event count exceeds its original allocation");
        if(state.specialEventCount) {
            auto index=state.lastSpecialEventNumber;
            hp2::Require(index<state.specialEventCount,"Replay special-event cursor is out of range");
            while(index>=0 && state.specialEvents[index].worldLoopCounter>=clock.worldLoopCounter)--index;
            if(index<0)index=0;
            while(index<state.specialEventCount && state.specialEvents[index].worldLoopCounter<=clock.worldLoopCounter) {
                if(state.specialEvents[index].worldLoopCounter==clock.worldLoopCounter)
                    (void)replayManagerHandleSpecialEvent(state,services,state.specialEvents[index]);
                ++index;
            }
            state.lastSpecialEventNumber=std::min(index,state.specialEventCount-1);
        }
    }
    MaybeTakeDummySnapshot();MaybeDoChecksumSnapshot();
    owner.demo->ServiceRecording(clock.worldLoopCounter==state.lastSnapshotTime,*this,clock);
}
