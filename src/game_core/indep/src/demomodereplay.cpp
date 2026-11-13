#include "indep/inc/DemoModeReplay.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bFile.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/DemoDisc.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/FMenus.hpp"
#include "indep/inc/CarSounds.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Collision.hpp"
#include "bsound/include/bSound.h"
#include "bware/inc/bMath.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace hp2 {
std::array<std::string,6> demoModeReplayFilenames{{"REPLAY0.BIN",
    "JUL23\\T33GoldVanquish.bin","JUL23\\T12YellowSpider.bin","JUL23\\T62OrangeF1LM.bin",
    "JUL23\\T43GreenM5.bin","JUL23\\T11BlueElise.bin"}};

struct DemoModeReplay::ResourceFile {
    std::string name;
    std::uint32_t nameHash{};
    std::vector<std::uint8_t> bytes;
    std::vector<Chunk> chunks;
};

struct DemoModeReplay::SessionDiagnostics {
    NativeApplicationHost* host{};
};

DemoModeReplay::DemoModeReplay()=default;
DemoModeReplay::~DemoModeReplay()=default;

void DemoModeReplay::RecordSessionEvent(const char* event,const ReplaySnapshot* snapshot,
    const char* detail) const noexcept
{
    if(!diagnostics_ || !diagnostics_->host)return;
    if(!snapshot)snapshot=testSnapshot_;
    const auto* file=resources_.empty()?nullptr:resources_.front().get();
    const auto* stream=replayBorrower_?replayBorrower_->State().stream:repeat_.get();
    const auto* clock=snapshot?snapshot->clock:nullptr;
    char context[1024]{};
    std::snprintf(context,sizeof(context),
        "recording=%d source=%s source_id=%08x bytes=%zu clip=%d format=%u snapshot=%s loops=%d..%d cursor=%d/%d "
        "input_bits=%u/%u input_values=%u/%u loop=%d timer=%d real_q=%d %s",
        selectedRecording,file?file->name.c_str():"unloaded",file?file->nameHash:0,file?file->bytes.size():0,
        currentSnapshotIndex_,metadata_?ReadU32(metadata_):0u,snapshot?snapshot->name:"none",
        snapshot?snapshot->worldLoopCounter:0,snapshot?snapshot->endLoopCounter:0,
        snapshot?snapshot->cursor:0,snapshot?snapshot->dataSize:0,
        stream?stream->readPosition:0,stream?stream->writePosition:0,
        stream?stream->readCount:0,stream?stream->writeCount:0,
        clock?clock->worldLoopCounter:0,clock?clock->worldTimer:0,clock?clock->realTimeQuantized:0,detail?detail:"");
    diagnostics_->host->RecordSessionEvent(event,context);
}

bool DemoModeReplay::ConsiderLaunching(GameResources& resources,const GameTime& clock,
    const DemoDiscManager& demoDisc)
{
    // PAL13F120 adds random choice on the first ordinary launch as well
    // as for a configured demo disc. Beta only randomizes the latter.
    if (demoDisc.ActivationWord() || selectedRecording==0)
        selectedRecording=static_cast<std::int32_t>(vectorDefaultRandomBounded(5))+1;
    else {
        const auto next=static_cast<std::uint32_t>(selectedRecording)+1u;
        std::memcpy(&selectedRecording,&next,sizeof(next));
    }
    if (forcedRecording>=0)selectedRecording=forcedRecording;
    if (selectedRecording>=6)selectedRecording=1;
    const auto filename=GenerateFilename(selectedRecording);
    if (filename.empty() || !resources.TryOpenPhysical(filename))return false;
    // An optional installed recording must never crash the title-screen
    // idle path when its external native file is incomplete. The authored
    // PAL path, including random selection, remains unchanged on success.
    try { LoadResourceFile(resources,clock); }
    catch(const std::runtime_error& error) {
        RecordSessionEvent("demo.source_rejected",nullptr,error.what());return false;
    }
    return true;
}

void DemoModeReplay::Launch()
{
    // PAL13F1C0 requires version8 and the recorded-race flag. In the PC
    // resource boundary, the same recording must have an owned snapshot;
    // otherwise the first World/ReplayManager hand-off is impossible.
    (void)GenerateFilename(selectedRecording);
    // A valid metadata header and snapshot list without a BitStream must
    // NOT mark the replay active. The race borrows repeat_ in its native
    // ReplayManagerOwner constructor before its first World restore.
    if(metadata_ && metadataBytes_>=12u && !snapshots_.empty() &&
        repeat_ && repeat_->bytes && repeat_->channelCount>0 &&
        streamIndex_.FrameCount()>0 &&
        (ReadU32(metadata_)==8u || ReadU32(metadata_)==9u || ReadU32(metadata_)==10u) && ReadU32(metadata_+8)==1u)active=1;
    else UnloadResourceFile();
}

// Full PAL13F288..13F2C8. The recording's snapshot list has moved to the
// same ReplayManagerOwner; it is not loaded or copied a second time here.
void DemoModeReplay::BeginDemoMode(GameSession& coordinator)
{
    Require(replayBorrower_ && coordinator.bindings &&
        coordinator.bindings->replayOwner==replayBorrower_,
        "Demo playback has no canonical ReplayManager owner");
    currentSnapshotIndex_=-1;
    snapshotRestorePending_=false;
    RecordSessionEvent("demo.begin");
    (void)replayBorrower_->RestoreSnapshot("REPLAY_MANAGER_STATE");
    AdvanceSnapshot(true,coordinator);
}

// PAL13F430..13F4EC posts the actual selected node, not an ordinal. The
// source32 pointer word is represented by intptr_t in the native queue.
void DemoModeReplay::AdvanceSnapshot(bool forward,GameSession& coordinator)
{
    Require(replayBorrower_ && coordinator.bindings &&
        coordinator.bindings->replayOwner==replayBorrower_,
        "Demo snapshot selection has no canonical ReplayManager owner");
    const auto add=[](std::int32_t value,std::int32_t delta) {
        const auto bits=static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(delta);
        std::int32_t result;std::memcpy(&result,&bits,sizeof(result));return result;
    };
    ReplaySnapshot* selected;
    auto nextIndex=currentSnapshotIndex_;
    if(forward) {
        nextIndex=add(currentSnapshotIndex_,1);
        selected=replayBorrower_->FindSnapshot("DEMO_MODE_SNAPSHOT",nextIndex);
        if(!selected)return;
    } else if(currentSnapshotIndex_>0) {
        nextIndex=currentSnapshotIndex_-1;
        selected=replayBorrower_->FindSnapshot("DEMO_MODE_SNAPSHOT",nextIndex);
    } else selected=replayBorrower_->FindSnapshot("DEMO_MODE_SNAPSHOT",0);
    if(!selected)return;
    if(!raceActionsEnqueue(&coordinator.actions,14,reinterpret_cast<std::intptr_t>(selected))) {
        coordinator.result=RC_RESULT_QUEUE_CAPACITY;
        throw std::runtime_error("Demo snapshot cannot enter the actual coordinator queue");
    }
    testSnapshot_=selected;
    currentSnapshotIndex_=nextIndex;
    snapshotRestorePending_=true;
    RecordSessionEvent("demo.restore_queued",selected);
}

void DemoModeReplay::SnapshotRestoreCompleted(const ReplaySnapshot& snapshot)
{
    Require(replayBorrower_ && snapshotRestorePending_ && testSnapshot_==&snapshot,
        "Demo snapshot completion does not belong to the pending Replay action14");
    snapshotRestorePending_=false;
    RecordSessionEvent("demo.restore_complete",&snapshot);
}

// Full PAL13F310..13F430. +20 is also the existing ServiceRecording test
// snapshot, and +14 is its test/edit flag; playback introduces no mirror.
bool DemoModeReplay::ServiceDemoMode(GameSession& coordinator,const GameTime& clock)
{
    Require(replayBorrower_ && coordinator.bindings &&
        coordinator.bindings->replayOwner==replayBorrower_ && coordinator.bindings->clock==&clock,
        "Demo playback service uses different replay/clock owners");
    // Action14 is deferred by the native FIFO. Do not compare a snapshot's
    // recorded end loop with the prior (frontend/previous-race) clock until
    // its real ReplayManager restore has reset that clock.
    if(snapshotRestorePending_)return true;
    auto* snapshot=testSnapshot_;
    if(snapshot && clock.worldLoopCounter>=snapshot->endLoopCounter) {
        testSnapshot_=nullptr;
        AdvanceSnapshot(true,coordinator);
        snapshot=testSnapshot_;
        if(!snapshot)return false;
    }
    if(!recordingTest_ || !snapshot)return true;
    const auto difference=[](std::int32_t left,std::int32_t right) {
        const auto bits=static_cast<std::uint32_t>(left)-static_cast<std::uint32_t>(right);
        std::int32_t value;std::memcpy(&value,&bits,sizeof(value));return value;
    };
    const float seconds=static_cast<float>(difference(snapshot->endTime,snapshot->worldTimer))*
        0.0002500000118743628f;
    if(seconds<2.0f) {
        replayBorrower_->DeleteSnapshot(*testSnapshot_);
        testSnapshot_=nullptr;
        const auto bits=static_cast<std::uint32_t>(currentSnapshotIndex_)-1u;
        std::memcpy(&currentSnapshotIndex_,&bits,sizeof(bits));
    } else {
        // Timer::SetTime136DD0 rounds the separate binary32 product before
        // truncation; preserve source SUBU before the signed time comparison.
        const float halfSeconds=seconds*0.5f;
        const float scaled=halfSeconds*4000.0f;
        const auto halfTicks=convertFloatToInt32(scaled+0.5f);
        if(difference(snapshot->endTime,halfTicks)<clock.worldTimer) {
            testSnapshot_->endTime=clock.worldTimer;
            testSnapshot_->endLoopCounter=clock.worldLoopCounter;
            testSnapshot_=nullptr;
        }
    }
    if(testSnapshot_)return true;
    AdvanceSnapshot(true,coordinator);
    return false;
}

// Full PAL13F4F0..13F500. Borrowed nodes are returned by ReplayManagerOwner's
// destructor; this method only clears the original playback cells.
void DemoModeReplay::EndDemoMode() noexcept
{
    if(active)RecordSessionEvent("demo.end",testSnapshot_);
    testSnapshot_=nullptr;
    snapshotRestorePending_=false;
    active=0;
    currentSnapshotIndex_=0;
}

// PAL13F2D0..13F2FC, CanTestDemoMode. Inspect the canonical retained list.
bool DemoModeReplay::CanTestDemoMode() const
{
    Require(replayBorrower_,"Demo test has no canonical ReplayManager owner");
    return replayBorrower_->FindSnapshot("DEMO_MODE_SNAPSHOT",0)!=nullptr;
}

// PAL13F300..13F310. Unlike BeginDemoMode this does not restore manager state.
void DemoModeReplay::BeginDemoModeTest(GameSession& coordinator)
{
    currentSnapshotIndex_=-1;
    snapshotRestorePending_=false;
    AdvanceSnapshot(true,coordinator);
}

// PAL13F660..13F66C; allocation belongs to the existing ServiceRecording.
void DemoModeReplay::BeginRecordingSnapshot() noexcept
{
    recordingRequested_=1;
}

// PAL13F670..13F6C0. The released boolean argument is unused. Very short
// recordings are deleted, using the original signed result of 32-bit SUBU.
void DemoModeReplay::EndRecordingSnapshot(bool discard,const GameTime& clock)
{
    (void)discard;
    recordingRequested_=0;
    if (!recordingSnapshot_)return;
    Require(replayBorrower_,"Demo recording has no canonical ReplayManager owner");
    Require(recordingSnapshot_->clock==&clock,"Demo recording uses a different source clock");
    const auto bits=static_cast<std::uint32_t>(clock.worldLoopCounter)-
        static_cast<std::uint32_t>(recordingSnapshot_->worldLoopCounter);
    std::int32_t elapsed;std::memcpy(&elapsed,&bits,sizeof(elapsed));
    if (elapsed<31)replayBorrower_->DeleteSnapshot(*recordingSnapshot_);
    recordingSnapshot_=nullptr;
}

const std::string& DemoModeReplay::GetBaseFilename(std::int32_t index) const
{
    if (static_cast<std::uint32_t>(index)>=6u)index=0;
    return demoModeReplayFilenames[static_cast<std::size_t>(index)];
}

std::string DemoModeReplay::GenerateFilename(std::int32_t index) const
{
    if (GetBaseFilename(index).empty())return {};
    return "REPLAY\\"+GetBaseFilename(index);
}

bool DemoModeReplay::Loader(std::uint32_t id,std::uint8_t* payload,std::size_t bytes,const GameTime& clock)
{
    Require(!replayBorrower_,"Demo replay chunks are still borrowed by the live ReplayManager");
    if (id==0x39100u) {
        Require(!metadata_,"Demo recording has more than one metadata chunk");
        Require(payload && bytes>=0x50u && (ReadU32(payload)==8u || ReadU32(payload)==9u || ReadU32(payload)==10u) &&
            ReadU32(payload+8)==1u,"Demo replay metadata is truncated or has an unsupported format");
        metadata_=payload;metadataBytes_=bytes;return true;
    }
    if (id==0x39101u) {
        Require(payload && bytes>=0x158,"Demo replay snapshot header is truncated");
        auto owner=std::make_unique<ReplaySnapshotStorage>();
        owner->authoredPayload=payload;owner->authoredPayloadBytes=bytes;
        auto& snapshot=owner->snapshot;
        const auto* end=static_cast<const std::uint8_t*>(std::memchr(payload+8,0,32));
        Require(end,"Demo replay snapshot name has no bounded terminator");
        const std::string name(reinterpret_cast<const char*>(payload+8),static_cast<std::size_t>(end-(payload+8)));
        snapshot.bytes=payload+0x158;snapshot.capacity=bytes-0x158;
        replaySnapshotInitialize(&snapshot,name.c_str(),&clock);
        const auto signedWord=[](const std::uint8_t* input) {
            const auto bits=ReadU32(input);std::int32_t value;std::memcpy(&value,&bits,4);return value;
        };
        snapshot.worldTimer=signedWord(payload+0x28);snapshot.worldLoopCounter=signedWord(payload+0x30);
        snapshot.endTime=signedWord(payload+0x2c);snapshot.endLoopCounter=signedWord(payload+0x34);
        snapshot.memoryImageLoaded=1; // PAL13EE9C writes this tag, regardless of disk bytes.
        Require(metadata_,"Demo snapshot precedes its format metadata");
        snapshot.wireFormat=ReadU32(metadata_)==8u?static_cast<std::uint32_t>(REPLAY_SNAPSHOT_PAL_V8):ReadU32(payload+0x38u);
        Require((ReadU32(metadata_)==8u && snapshot.wireFormat==REPLAY_SNAPSHOT_PAL_V8) ||
                ((ReadU32(metadata_)==9u || ReadU32(metadata_)==10u) && snapshot.wireFormat==REPLAY_SNAPSHOT_NATIVE),
            "Native Demo snapshot has an unsupported reference encoding");
        snapshot.clockPolicy=ReadU32(metadata_)==10u ? GAME_TIME_TIMER_NATIVE:GAME_TIME_TIMER_LEGACY;
        std::copy(sourcePlayerTokens_.begin(),sourcePlayerTokens_.end(),snapshot.sourcePlayerTokens);
        const auto interesting=ReadU32(payload+0x14c);
        std::memcpy(&snapshot.totalInterestingValue,&interesting,4);
        snapshot.recording=payload[0x3c];snapshot.restoring=payload[0x3d];
        snapshot.recordingChecksums=payload[0x3e];snapshot.verifyingChecksums=payload[0x3f];
        std::memcpy(&snapshot.sectionDepth,payload+0x40,1);
        Require(snapshot.sectionDepth==-1 && !snapshot.recording && !snapshot.restoring &&
            !snapshot.recordingChecksums && !snapshot.verifyingChecksums,
            "Demo snapshot contains unfinished transfer state");
        snapshot.overflowReported=payload[0x41];snapshot.nextSectionOrdinal=ReadU16(payload+0x42);
        snapshot.debugDepth=signedWord(payload+0x44);snapshot.debugLoopCounter=signedWord(payload+0x48);
        snapshot.cursor=signedWord(payload+0x150);snapshot.dataSize=signedWord(payload+0x154);
        Require(snapshot.dataSize>=0 && static_cast<std::size_t>(snapshot.dataSize)<=snapshot.capacity &&
            snapshot.cursor>=0 && snapshot.cursor<=snapshot.dataSize,"Demo snapshot stream bounds are invalid");
        // Source's intrusive list adds this same payload at the tail. The
        // native list retains the actual stream, with no copy of its bytes.
        snapshots_.push_back(std::move(owner));return true;
    }
    if (id==0x39102u) {
        Require(!repeat_,"Demo recording has more than one input stream");
        Require(bytes<=UINT32_MAX,"Demo replay BitStream exceeds its source extent");
        auto repeat=std::make_unique<BitStream>();
        Require(bitStreamRestore(repeat.get(),payload,static_cast<std::uint32_t>(bytes))==BIT_STREAM_OK,
            "Demo replay contains an invalid actual BitStream header");
        streamIndex_.Build(*repeat);
        repeat_=std::move(repeat);return true;
    }
    return false;
}

bool DemoModeReplay::Unloader(std::uint32_t id,const std::uint8_t* payload)
{
    Require(!replayBorrower_,"Cannot remove demo replay storage while ReplayManager still uses it");
    if (id==0x39100u) {metadata_=nullptr;metadataBytes_=0;return true;}
    if (id==0x39101u) {
        const auto found=std::find_if(snapshots_.begin(),snapshots_.end(),[payload](const auto& entry) {
            return entry->authoredPayload==payload;
        });
        if (found!=snapshots_.end())snapshots_.erase(found);
        return true;
    }
    if (id==0x39102u) {repeat_.reset();streamIndex_={};return true;}
    return false;
}

void DemoModeReplay::LoadResourceFile(GameResources& resources,const GameTime& clock)
{
    Require(!replayBorrower_,"Cannot replace a demo recording used by ReplayManager");
    Require(!loadPending_,"Demo recording already has a pending load");
    if (loaded_) {
        // A second authored selection must replace the first recording, not
        // append more 39101 nodes ahead of the new ones. FirstSnapshot() and
        // RestoreRaceParametersSnapshot() always consume the *first* node.
        // Stage the replacement under a separate owner: a failed physical
        // read/parse must leave the previous recording fully usable.
        Require(!active && !recordingSnapshot_ && !testSnapshot_,
            "Cannot replace a Demo recording with active playback/recording state");
        DemoModeReplay staged;
        staged.selectedRecording=selectedRecording;
        staged.LoadResourceFile(resources,clock);
        resources_.swap(staged.resources_);
        snapshots_.swap(staged.snapshots_);
        repeat_.swap(staged.repeat_);
        std::swap(streamIndex_,staged.streamIndex_);
        std::swap(metadata_,staged.metadata_);
        std::swap(metadataBytes_,staged.metadataBytes_);
        std::swap(loaded_,staged.loaded_);
        std::swap(sourcePlayerTokens_,staged.sourcePlayerTokens_);
        currentSnapshotIndex_=0;
        RecordSessionEvent("demo.source_replaced");
        return;
    }
    loadPending_=true;
    const auto previousCount=snapshots_.size();
    auto* previousMetadata=metadata_;
    const auto previousMetadataBytes=metadataBytes_;
    const auto previousPlayerTokens=sourcePlayerTokens_;
    auto previousRepeat=std::move(repeat_);
    auto previousIndex=std::move(streamIndex_);
    try {
        auto file=std::make_unique<ResourceFile>();
        file->name=GenerateFilename(selectedRecording);
        file->nameHash=PathHash(file->name);
        file->bytes=resources.Load(file->name);
        file->chunks=ReadChunks(file->bytes);
        // Synchronous native file transfer replaces the platform's group7
        // completion. Reserve ownership before publishing borrowed pointers.
        resources_.reserve(resources_.size()+1);
        for (const auto& chunk:file->chunks)
            Loader(chunk.id,file->bytes.data()+chunk.payloadOffset,chunk.size,clock);
        Require(metadata_ && metadataBytes_>=0x50u && (ReadU32(metadata_)==8u || ReadU32(metadata_)==9u || ReadU32(metadata_)==10u) && ReadU32(metadata_+8)==1 &&
            repeat_ && streamIndex_.FrameCount()>0,
            "Demo recording lacks complete supported metadata or its framed input stream");
        std::size_t managerNodes=0,demoNodes=0;
        for(const auto& node:snapshots_) {
            if(!std::strcmp(node->snapshot.name,"REPLAY_MANAGER_STATE")) {
                const auto range=ReplayManagerOwner::ValidateStateSnapshot(node->snapshot);
                const auto frames=static_cast<std::uint32_t>(range.endLoop)-static_cast<std::uint32_t>(range.beginLoop);
                Require(frames==streamIndex_.FrameCount(),"Demo manager capture interval differs from its complete input stream");
                ++managerNodes;
            } else if(!std::strcmp(node->snapshot.name,"DEMO_MODE_SNAPSHOT"))++demoNodes;
        }
        Require(managerNodes==1 && demoNodes>0 &&
            !std::strcmp(snapshots_.front()->snapshot.name,"DEMO_MODE_SNAPSHOT"),
            "Demo recording has no unique manager state or initial World snapshot");
        resources_.push_back(std::move(file));
        loadPending_=false;loaded_=true;
        RecordSessionEvent("demo.source_loaded");
    } catch (...) {
        snapshots_.resize(previousCount);metadata_=previousMetadata;metadataBytes_=previousMetadataBytes;
        repeat_=std::move(previousRepeat);streamIndex_=std::move(previousIndex);
        SetSourcePlayerTokens(previousPlayerTokens);loadPending_=false;
        throw;
    }
}

void DemoModeReplay::UnloadResourceFile()
{
    Require(!replayBorrower_,"Cannot unload demo recording before ReplayManager destruction");
    if (!loaded_)return;
    RecordSessionEvent("demo.source_unload");
    // PAL13F0E8->199728->1992A8->199198 retires chunks backwards.
    // Retire files in reverse acquisition order, keeping every payload alive
    // until all of that file's borrowers have been unpublished.
    while (!resources_.empty()) {
        auto& file=*resources_.back();
        for (auto item=file.chunks.rbegin();item!=file.chunks.rend();++item)
            Unloader(item->id,file.bytes.data()+item->payloadOffset);
        resources_.pop_back();
    }
    sourcePlayerTokens_={};loaded_=false;
}

ReplaySnapshot* DemoModeReplay::FirstSnapshot() noexcept
{
    return snapshots_.empty() ? nullptr : &snapshots_.front()->snapshot;
}

void DemoModeReplay::ValidateSnapshotReader(const ReplaySnapshot& source,const RaceParameters& parameters,
    const CarDriverAppearanceCatalogue& characters) const
{
    Require(parameters.drivers && parameters.runtimeSettings,"Demo preflight has no actual driver/settings owners");
    const auto* repeat=replayBorrower_?replayBorrower_->State().stream:repeat_.get();
    Require(repeat && repeat->bytes,"Demo snapshot preflight has no retained input stream");
    RaceParameters staged=parameters;
    std::array<RaceDriverInfo,RACE_DRIVER_LIMIT> drivers;
    std::copy_n(parameters.drivers,RACE_DRIVER_LIMIT,drivers.begin());
    RaceRuntimeSettings settings=*parameters.runtimeSettings;
    staged.drivers=drivers.data();staged.runtimeSettings=&settings;
    ReplaySnapshot snapshot=source;
    snapshot.cursor=0;snapshot.recording=0;snapshot.restoring=1;
    snapshot.result=REPLAY_SNAPSHOT_OK;snapshot.sectionDepth=-1;
    raceParametersSnapshot(&staged,&snapshot,&characters);
    Require(snapshot.result==REPLAY_SNAPSHOT_OK,"Demo snapshot RaceParameters prefix is truncated");
    std::vector<std::uint8_t> bytes(repeat->bytes,repeat->bytes+repeat->storageBytes);
    BitStream reader=*repeat;reader.bytes=bytes.data();reader.output=nullptr;reader.transport=nullptr;
    replaySnapshotBitStream(&reader,&snapshot,0);
    Require(snapshot.result==REPLAY_SNAPSHOT_OK,"Demo snapshot bRepeat prefix is invalid or truncated");
    const auto frame=streamIndex_.FindFrame(reader);
    const ReplaySnapshot* manager=nullptr;
    if(replayBorrower_)manager=replayBorrower_->FindSnapshot("REPLAY_MANAGER_STATE");
    else for(const auto& node:snapshots_)
        if(!std::strcmp(node->snapshot.name,"REPLAY_MANAGER_STATE")){manager=&node->snapshot;break;}
    Require(manager,"Demo snapshot has no recorded manager interval");
    const auto range=ReplayManagerOwner::ValidateStateSnapshot(*manager);
    const auto offset=static_cast<std::uint32_t>(source.worldLoopCounter)-static_cast<std::uint32_t>(range.beginLoop);
    const auto duration=static_cast<std::uint32_t>(source.endLoopCounter)-
        static_cast<std::uint32_t>(source.worldLoopCounter);
    Require(frame!=ReplayStreamIndex::NoFrame && offset==frame && duration<=streamIndex_.FrameCount()-frame,
        "Demo snapshot does not begin at a complete frame or extends beyond its input stream");
}

void DemoModeReplay::RestoreRaceParametersSnapshot(RaceParameters& parameters,
    const ApplicationLaunchOptions& options,const std::int32_t& language,
    const CarDriverAppearanceCatalogue& characters)
{
    auto* snapshot=FirstSnapshot();
    Require(snapshot,"Demo replay restore requires its actual first retained snapshot");
    // Native input validation is transactional: an invalid or truncated demo
    // cannot corrupt the frontend race setup, its retained driver bank or the
    // two live profile-settings records before a second playback attempt.
    // Those are still the SAME owners after successful restoration.
    Require(parameters.drivers && parameters.runtimeSettings,
        "Demo race parameters lack their live driver/settings storage");
    // Every authored demo segment must have a valid actual reader state,
    // not just the first segment selected for the initial track load.
    for(const auto& node:snapshots_)
        if(!std::strcmp(node->snapshot.name,"DEMO_MODE_SNAPSHOT"))
            ValidateSnapshotReader(node->snapshot,parameters,characters);
    RaceParameters staged=parameters;
    std::array<RaceDriverInfo,RACE_DRIVER_LIMIT> drivers{};
    std::copy_n(parameters.drivers,RACE_DRIVER_LIMIT,drivers.begin());
    RaceRuntimeSettings settings=*parameters.runtimeSettings;
    staged.drivers=drivers.data();staged.runtimeSettings=&settings;
    // The same authored snapshot is consumed again by ReplayManager after
    // track creation. A preview must not truncate the World/Car suffix.
    const auto originalCursor=snapshot->cursor;
    const auto originalRecording=snapshot->recording;
    const auto originalRestoring=snapshot->restoring;
    const auto originalResult=snapshot->result;
    snapshot->cursor=0;snapshot->recording=0;snapshot->restoring=1;
    snapshot->result=REPLAY_SNAPSHOT_OK;
    try {
        raceParametersInitializeDefaults(&staged,&options,&language);
        raceParametersSnapshot(&staged,snapshot,&characters);
        Require(snapshot->result==REPLAY_SNAPSHOT_OK,
            "Demo RaceParameters cannot be read from the complete retained recording");
        staged.demoReplay=1;
    } catch(...) {
        snapshot->cursor=originalCursor;snapshot->recording=originalRecording;
        snapshot->restoring=originalRestoring;snapshot->result=originalResult;
        throw;
    }
    snapshot->cursor=originalCursor;snapshot->recording=originalRecording;
    snapshot->restoring=originalRestoring;snapshot->result=originalResult;
    // Commit only the valid PAL payload, keeping the original native
    // allocation addresses used by the remaining frontend and World owners.
    std::copy(drivers.begin(),drivers.end(),parameters.drivers);
    *parameters.runtimeSettings=settings;
    staged.drivers=parameters.drivers;staged.runtimeSettings=parameters.runtimeSettings;
    parameters=staged;
}
} // namespace hp2

namespace hp2 {
void DemoModeReplay::BindSessionHost(NativeApplicationHost* host)
{
    Require(!diagnostics_ && !replayBorrower_,"Demo diagnostics already has a live owner");
    diagnostics_=std::make_unique<SessionDiagnostics>();diagnostics_->host=host;
}

void DemoModeReplay::SetSourcePlayerTokens(const std::array<std::uint32_t,2>& tokens)
{
    Require(!replayBorrower_,"Cannot change the relocation of a borrowed Demo recording");
    Require(!tokens[0] || tokens[0]!=tokens[1],"Demo Player source tokens are ambiguous");
    sourcePlayerTokens_=tokens;
    for(auto& entry:snapshots_)
        std::copy(tokens.begin(),tokens.end(),entry->snapshot.sourcePlayerTokens);
}

std::size_t DemoModeReplay::SourceSegmentCount() const
{
    std::size_t count=0;
    if(replayBorrower_)while(replayBorrower_->FindSnapshot("DEMO_MODE_SNAPSHOT",static_cast<int32_t>(count)))++count;
    else for(const auto& node:snapshots_)if(!std::strcmp(node->snapshot.name,"DEMO_MODE_SNAPSHOT"))++count;
    return count;
}
} // namespace hp2

namespace hp2 {
void DemoModeReplay::ServiceRecording(bool snapshotTime,ReplayManagerOwner& replay,const GameTime& clock)
{
    Require(replayBorrower_==&replay,"Demo recording has a different ReplayManager owner");
    if(snapshotTime) {
        if(recordingRequested_ && !recordingSnapshot_) {
            (void)replay.DeleteSnapshot("REPLAY_MANAGER_STATE");
            (void)replay.TakeSnapshot("REPLAY_MANAGER_STATE");
            recordingSnapshot_=replay.TakeSnapshot("DEMO_MODE_SNAPSHOT");
        }
        if(recordingTest_ && testSnapshot_) {
            const auto endBits=static_cast<std::uint32_t>(testSnapshot_->worldTimer)+12000u;
            std::int32_t end;std::memcpy(&end,&endBits,sizeof(end));
            if(clock.worldTimer<end) {
                const auto endLoop=testSnapshot_->endLoopCounter;
                const auto endTime=testSnapshot_->endTime;
                replay.DeleteSnapshot(*testSnapshot_);
                testSnapshot_=replay.TakeSnapshot("DEMO_MODE_SNAPSHOT");
                if(testSnapshot_) {testSnapshot_->endLoopCounter=endLoop;testSnapshot_->endTime=endTime;}
                replay.SortSnapshots("DEMO_MODE_SNAPSHOT");
            }
        }
    }
    if(recordingSnapshot_) {
        recordingSnapshot_->endLoopCounter=clock.worldLoopCounter;
        recordingSnapshot_->endTime=clock.worldTimer;
    }
    if(saveRequested_) {
        SaveRecordingToFile(saveDestination_,replay.Parameters());
        saveRequested_=0;
        saveDestination_.clear();
    }
    if(eraseRequested_) {
        while(replay.DeleteSnapshot("DEMO_MODE_SNAPSHOT")) {}
        (void)replay.DeleteSnapshot("REPLAY_MANAGER_STATE");
        testSnapshot_=nullptr;recordingRequested_=0;recordingSnapshot_=nullptr;eraseRequested_=0;
        // PAL13F63C calls bRefreshTweaker25D2C8 (CODEINE command40).
        // The PC API owns these requests directly; there is no remote tweak
        // mirror to refresh and no hardware cache operation to emulate.
    }
}
}

namespace hp2 {
namespace {
void SaveDemoWord(std::uint8_t* address,std::uint32_t value) noexcept
{
    for (std::size_t i=0;i<4;++i)address[i]=static_cast<std::uint8_t>(value>>(i*8));
}
void AppendDemoChunk(std::vector<std::uint8_t>& file,std::uint32_t id,
    const std::uint8_t* payload,std::size_t count)
{
    Require(count<=std::numeric_limits<std::uint32_t>::max() &&
        count<=std::numeric_limits<std::size_t>::max()-file.size()-8u,
        "Demo recording chunk exceeds native file limits");
    const auto previous=file.size();file.resize(previous+8u+count);
    SaveDemoWord(file.data()+previous,id);
    SaveDemoWord(file.data()+previous+4u,static_cast<std::uint32_t>(count));
    if(count)std::memcpy(file.data()+previous+8u,payload,count);
}
}

void DemoModeReplay::RequestSaveRecording(const std::filesystem::path& path)
{
    Require(!path.empty() && !saveRequested_,
        "Demo Save already has a pending destination or has no filename");
    saveDestination_=path;
    saveRequested_=1;
}

void DemoModeReplay::RequestEraseRecording()
{
    Require(!saveRequested_ && !snapshotRestorePending_,
        "Cannot erase Demo snapshots while saving or restoring them");
    eraseRequested_=1;
}

void DemoModeReplay::SaveRecordingToFile(const std::filesystem::path& path,
    const RaceParameters& parameters) const
{
    Require(!path.empty(),"Demo recording has no explicit output filename");
    const auto countNamed=[&](const char* name) {
        std::size_t count=0;
        if(replayBorrower_) {
            for(std::size_t i=0;i<replayBorrower_->ActiveSnapshotCount();++i)
                if(!std::strcmp(replayBorrower_->ActiveSnapshot(i)->name,name))++count;
        } else for(const auto& entry:snapshots_)
            if(!std::strcmp(entry->snapshot.name,name))++count;
        return count;
    };
    // PAL13F804..13F848 requires both first named groups before file output.
    Require(countNamed("DEMO_MODE_SNAPSHOT") && countNamed("REPLAY_MANAGER_STATE")==1,
        "Demo Save requires race snapshots and one complete manager state");
    const BitStream* stream=replayBorrower_ ? replayBorrower_->State().stream:repeat_.get();
    Require(stream && stream->bytes && stream->writePosition>=stream->dataStart*8u &&
        stream->writePosition<=stream->storageBytes*8ull,
        "Demo Save has no valid recorded input stream");
    bool nativeFormat=false,palFormat=false;
    int32_t clockPolicy=-1;
    const auto inspectFormat=[&](const ReplaySnapshotStorage& entry) {
        if(entry.snapshot.wireFormat==REPLAY_SNAPSHOT_PAL_V8)palFormat=true;
        else if(entry.snapshot.wireFormat==REPLAY_SNAPSHOT_NATIVE)nativeFormat=true;
        else throw std::runtime_error("Demo Save has an unsupported snapshot format");
        Require(entry.snapshot.clockPolicy==GAME_TIME_TIMER_LEGACY ||
            entry.snapshot.clockPolicy==GAME_TIME_TIMER_NATIVE,"Demo Save has an unsupported clock policy");
        if(clockPolicy<0)clockPolicy=entry.snapshot.clockPolicy;
        Require(clockPolicy==entry.snapshot.clockPolicy,"Demo Save cannot mix snapshot clock policies");
        Require(entry.snapshot.wireFormat!=REPLAY_SNAPSHOT_PAL_V8 ||
            entry.snapshot.clockPolicy==GAME_TIME_TIMER_LEGACY,"PAL Demo cannot use native Timer rounding");
    };
    if(replayBorrower_) {
        for(std::size_t i=0;i<replayBorrower_->ActiveSnapshotCount();++i)
            inspectFormat(*replayBorrower_->ActiveSnapshotStorage(i));
    } else for(const auto& entry:snapshots_)inspectFormat(*entry);
    Require(!(nativeFormat && palFormat),"Demo Save cannot mix PAL state with native snapshot encoding");
    const bool palInput=replayBorrower_?
        replayBorrower_->State().synchronizationPolicy==REPLAY_SYNC_PAL_REFERENCE:
        metadata_ && ReadU32(metadata_)==8u;
    Require(palInput==palFormat,"Demo Save cannot relabel source checksums as native recording data");
    ReplayStreamIndex outputIndex;
    outputIndex.Build(*stream);
    Require(outputIndex.FrameCount()>0,"Demo Save cannot publish an empty frame stream");
    // Native authoring captures its first manager node before later input
    // frames exist. Serialize the current manager interval into an output-only
    // node so the finished format9 file describes the whole completed stream.
    // Retained PAL8 state and bytes are never refreshed or rewritten here.
    ReplaySnapshotStorage finalizedManager;
    if(nativeFormat && replayBorrower_) {
        const auto* original=replayBorrower_->FindSnapshot("REPLAY_MANAGER_STATE");
        finalizedManager.allocation.resize(0x9400);
        auto& fresh=finalizedManager.snapshot;
        fresh=*original;fresh.bytes=finalizedManager.allocation.data();
        fresh.capacity=finalizedManager.allocation.size();fresh.cursor=0;fresh.dataSize=0;
        fresh.sectionDepth=-1;fresh.result=REPLAY_SNAPSHOT_OK;
        fresh.recording=1;fresh.restoring=0;fresh.recordingChecksums=0;fresh.verifyingChecksums=0;
        replayBorrower_->DoEndSnapshot(fresh);
        fresh.recording=0;fresh.dataSize=fresh.cursor;
        Require(fresh.result==REPLAY_SNAPSHOT_OK,"Native Demo manager finalization failed");
    }
    const ReplaySnapshot* manager=nullptr;
    if(!finalizedManager.allocation.empty())manager=&finalizedManager.snapshot;
    else if(replayBorrower_)manager=replayBorrower_->FindSnapshot("REPLAY_MANAGER_STATE");
    else for(const auto& node:snapshots_)
        if(!std::strcmp(node->snapshot.name,"REPLAY_MANAGER_STATE")){manager=&node->snapshot;break;}
    Require(manager,"Demo Save has no manager interval");
    const auto range=ReplayManagerOwner::ValidateStateSnapshot(*manager);
    Require(static_cast<std::uint32_t>(range.endLoop)-static_cast<std::uint32_t>(range.beginLoop)==
        outputIndex.FrameCount(),"Demo Save manager interval differs from completed input frames");
    // A fresh record belongs to the live race and current native authoring
    // host. It must not inherit a previous disc recording's race/name.
    std::array<std::uint8_t,0x50> metadata{};
    // A new version makes the different Timer contract explicit. Old native9
    // and PAL8 files continue to load and save with their original policy;
    // older runtimes reject10 instead of silently replaying different clocks.
    SaveDemoWord(metadata.data(),nativeFormat?(clockPolicy==GAME_TIME_TIMER_NATIVE?10u:9u):8u);
    SaveDemoWord(metadata.data()+4,static_cast<std::uint32_t>(parameters.selectedScenario));
    SaveDemoWord(metadata.data()+8,1u);
    (void)bGetComputerName(reinterpret_cast<char*>(metadata.data()+16),63);
    metadata.back()=0;
    std::vector<std::uint8_t> result(8u,0);
    AppendDemoChunk(result,0x39100u,metadata.data(),metadata.size());
    constexpr const char* groups[]={"DEMO_MODE_SNAPSHOT","REPLAY_MANAGER_STATE",
        "CHECKSUM0","CHECKSUM1","CHECKSUM2","CHECKSUM3"};
    std::size_t totalSnapshots=0;
    auto appendSnapshot=[&](const ReplaySnapshotStorage& entry) {
        const ReplaySnapshot& snapshot=entry.snapshot;
        Require(snapshot.dataSize>=0 && snapshot.bytes &&
            static_cast<std::size_t>(snapshot.dataSize)<=snapshot.capacity &&
            static_cast<std::size_t>(snapshot.dataSize)<=std::numeric_limits<std::uint32_t>::max()-0x15bu,
            "Demo Save received an invalid retained snapshot extent");
        const std::size_t used=static_cast<std::size_t>(snapshot.dataSize);
        const std::size_t payloadBytes=(used+0x15bu)&~std::size_t(3u);
        std::vector<std::uint8_t> bytes(payloadBytes,0);
        // Preserve the original opaque source header when this is a loaded
        // PAL file. Newly recorded nodes are serialized from typed fields,
        // with *no* pointer-sized native values written into the source ABI.
        if(entry.authoredPayload) {
            // PAL includes up to three authored tail bytes in the word-rounded
            // snapshot block. A truncated chunk cannot grant access to bytes
            // beyond its own recorded payload, even if dataSize fits capacity.
            Require(payloadBytes<=entry.authoredPayloadBytes,
                "Demo Save original snapshot is missing authored rounded tail bytes");
            std::memcpy(bytes.data(),entry.authoredPayload,payloadBytes);
        }
        {
            std::memcpy(bytes.data()+8u,snapshot.name,32u);
            SaveDemoWord(bytes.data()+0x28u,static_cast<std::uint32_t>(snapshot.worldTimer));
            SaveDemoWord(bytes.data()+0x2cu,static_cast<std::uint32_t>(snapshot.endTime));
            SaveDemoWord(bytes.data()+0x30u,static_cast<std::uint32_t>(snapshot.worldLoopCounter));
            SaveDemoWord(bytes.data()+0x34u,static_cast<std::uint32_t>(snapshot.endLoopCounter));
            SaveDemoWord(bytes.data()+0x38u,nativeFormat?snapshot.wireFormat:0u);
            bytes[0x3cu]=snapshot.recording;bytes[0x3du]=snapshot.restoring;
            bytes[0x3eu]=snapshot.recordingChecksums;bytes[0x3fu]=snapshot.verifyingChecksums;
            bytes[0x40u]=static_cast<std::uint8_t>(snapshot.sectionDepth);
            bytes[0x41u]=snapshot.overflowReported;
            bytes[0x42u]=static_cast<std::uint8_t>(snapshot.nextSectionOrdinal);
            bytes[0x43u]=static_cast<std::uint8_t>(snapshot.nextSectionOrdinal>>8);
            SaveDemoWord(bytes.data()+0x44u,static_cast<std::uint32_t>(snapshot.debugDepth));
            SaveDemoWord(bytes.data()+0x48u,static_cast<std::uint32_t>(snapshot.debugLoopCounter));
            std::uint32_t interesting{};
            std::memcpy(&interesting,&snapshot.totalInterestingValue,sizeof(interesting));
            SaveDemoWord(bytes.data()+0x14cu,interesting);
            SaveDemoWord(bytes.data()+0x150u,static_cast<std::uint32_t>(snapshot.cursor));
            SaveDemoWord(bytes.data()+0x154u,static_cast<std::uint32_t>(snapshot.dataSize));
        }
        if(used)std::memcpy(bytes.data()+0x158u,snapshot.bytes,used);
        AppendDemoChunk(result,0x39101u,bytes.data(),bytes.size());
        ++totalSnapshots;
    };
    // Final PAL13F720 writes nodes by group and insertion order rather than
    // sorting by memory address or by their source chunk offsets.
    for(const char* group:groups) {
        if(replayBorrower_) {
            // The ReplayManager owns the same live nodes during recording.
            for(std::size_t i=0;i<replayBorrower_->ActiveSnapshotCount();++i) {
                const auto* entry=replayBorrower_->ActiveSnapshotStorage(i);
                if(entry && std::strncmp(entry->snapshot.name,group,32u)==0) {
                    if(!finalizedManager.allocation.empty() && !std::strcmp(group,"REPLAY_MANAGER_STATE"))
                        appendSnapshot(finalizedManager);
                    else appendSnapshot(*entry);
                }
            }
        } else for(const auto& entry:snapshots_)
            if(std::strncmp(entry->snapshot.name,group,32u)==0)appendSnapshot(*entry);
    }
    Require(totalSnapshots>0,"Demo Save has no authored race snapshots");
    const std::size_t used=(static_cast<std::size_t>(stream->writePosition/8u)+4u)&~std::size_t(3u);
    Require(used<=stream->storageBytes,"Demo Save bitstream tail is truncated");
    AppendDemoChunk(result,0x39102u,stream->bytes,used);
    // An existing valid recording is never truncated until the complete new
    // image has passed extent validation and its temporary write succeeds.
    auto part=path;
    part+= ".partial";
    try {
        std::ofstream out(part,std::ios::binary|std::ios::trunc);
        Require(bool(out),"Cannot create native Demo recording output");
        out.write(reinterpret_cast<const char*>(result.data()),
            static_cast<std::streamsize>(result.size()));
        out.flush();Require(bool(out),"Cannot write complete native Demo recording");
    } catch (...) {
        std::error_code cleanup;
        std::filesystem::remove(part,cleanup);
        throw;
    }
    std::error_code ec;
    std::filesystem::rename(part,path,ec);
    if(ec) {
        std::error_code cleanup;
        std::filesystem::remove(part,cleanup);
        throw std::runtime_error("Cannot publish complete Demo recording: "+ec.message());
    }
}
} // namespace hp2
