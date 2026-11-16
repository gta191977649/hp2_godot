#include "bsound/include/bVoice.h"
#include "bsound/include/bSound.h"
#include "indep/src/speech/SEDWedge.hpp"

#include <algorithm>
#include <cstdio>
#include <stdexcept>


bVoiceMgr::bVoiceMgr()
{
    // Native automatic storage replaces GLOBAL_I_VoiceManager27C3B0 and
    // compiler atexit glue; these are not additional accepted ELF bodies.
    for (size_t index=0;index<48;++index) {
        diagnosticLine_[index*3]='-';
        diagnosticLine_[index*3+1]='-';
        diagnosticLine_[index*3+2]=' ';
    }
    Initialize();
}

WorldAudioResult bVoiceMgr::Bind(WorldAudioRequestQueue& queue)
{
    if (queue_ || !queue.library || !queue.instances || !queue.generation)
        return WORLD_AUDIO_INVALID_BINDING;
    queue_=&queue;
    return WORLD_AUDIO_OK;
}

WorldAudioResult bVoiceMgr::Unbind()
{
    for (const auto& voice:voices_)
        if (voice.GetSample().instance)return WORLD_AUDIO_INVALID_BINDING;
    queue_=nullptr;
    return WORLD_AUDIO_OK;
}

// PAL27AC80 clears all48 association pointers, leaving source ids alone.
void bVoiceMgr::Initialize() noexcept
{
    for (auto& voice:voices_)voice.Release();
}


// PAL27AB20. The visualization is diagnostic storage, not playback state.
void bVoiceMgr::PrintVoiceAllocsAndReleases(const char* tag,uint32_t id,const char* name)
{
    if (id>=48 || !tag || !tag[0] || !tag[1])
        throw std::invalid_argument("Invalid physical voice diagnostic");
    ++diagnosticRows_;
    if (diagnosticRows_==20) {
        if (queue_ && queue_->library)queue_->library->Printf(
            "00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47\n");
        diagnosticRows_=0;
    }
    diagnosticLine_[id*3]=tag[0];diagnosticLine_[id*3+1]=tag[1];
    if (queue_ && queue_->library)
        queue_->library->Printf("%s - %s\n",diagnosticLine_.data(),name ? name:"");
    diagnosticLine_[id*3]='-';diagnosticLine_[id*3+1]='-';
}

// PAL27AD00: prefer an already associated physical id; detach its old
// logical sample before replacement. Otherwise select the first vacant
// slot. This is not allocation from the separate255 sample-object pool.
bVoice* bVoiceMgr::GetVoice(uint16_t id,WorldAudioVoiceRef sample,uint64_t sequence)
{
    if (!queue_ || id>=48 || worldAudioValidateVoice(queue_,sample)!=WORLD_AUDIO_OK)
        return nullptr;
    for (auto& voice:voices_) {
        const auto previous=voice.GetSample();
        if (!previous.instance || voice.GetId()!=id)continue;
        // PAL bAssert prints and returns before the real reassignment.
        std::fprintf(stderr,"bVoiceMgr::GetVoice: reused device id %u\n",static_cast<unsigned>(id));

        if (worldAudioValidateVoice(queue_,previous)==WORLD_AUDIO_OK)
            previous.instance->hasHandle=0; // actual bSample::ReleaseVoice275C18
        voice.Set(id,sample,sequence);
        return &voice;
    }
    for (auto& voice:voices_) {
        if (voice.GetSample().instance)continue;
        voice.Set(id,sample,sequence);
        PrintVoiceAllocsAndReleases("VA",id,sample.instance->cue->name);
        return &voice;
    }
    return nullptr;
}

void bVoiceMgr::ReleaseVoice(bVoice& voice) noexcept
{
    // PAL27AF48 reads the old sample then clears only the association.
    voice.Release();
}

void bVoiceMgr::ReleaseSample(WorldAudioVoiceRef sample) noexcept
{
    for (auto& voice:voices_) {
        const auto held=voice.GetSample();
        if (held.instance==sample.instance && held.generation==sample.generation)
            ReleaseVoice(voice);
    }
}


WorldAudioResult bVoiceMgr::ReservationMap(std::array<bool,48>& used,size_t& channels) const
{
    used.fill(false);channels=0;
    if(!queue_ || !queue_->library)return WORLD_AUDIO_INVALID_BINDING;
    for(size_t index=0;index<queue_->instanceCapacity;++index) {
        const auto* sample=worldAudioInstanceAt(queue_,index);
        if(!sample)return WORLD_AUDIO_INVALID_BINDING;
        if(!sample->occupied || !sample->hasHandle)continue;
        const auto first=sample->reservedVoiceId,count=sample->reservedVoiceChannels;
        if((count!=1 && count!=2) || first>=48 || count>48-first)
            return WORLD_AUDIO_INVALID_BINDING;
        for(uint16_t offset=0;offset<count;++offset) {
            if(used[first+offset])return WORLD_AUDIO_INVALID_BINDING;
            used[first+offset]=true;++channels;
        }
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult bVoiceMgr::SelectChannels(WorldAudioVoiceRef sample,uint16_t& first,
    uint16_t& channels,bool& available) const
{
    available=false;first=48;channels=0;
    if(!queue_)return WORLD_AUDIO_INVALID_BINDING;
    const auto valid=worldAudioValidateVoice(queue_,sample);
    if(valid!=WORLD_AUDIO_OK)return valid;
    const auto& source=*sample.instance;
    if(source.sampleType==1)channels=1;
    else if(source.sampleType==3)channels=2;
    else if(!source.sampleType && source.cue && source.cue->metadata && source.cue->metadataSize==52)
        channels=static_cast<uint16_t>(hp2::GetChannelNum(source.cue->metadata[0x21])+1u);
    else return WORLD_AUDIO_INVALID_BINDING;
    if(channels!=1 && channels!=2)return WORLD_AUDIO_INVALID_BINDING;
    std::array<bool,48> used{};size_t total=0;
    const auto mapped=ReservationMap(used,total);if(mapped!=WORLD_AUDIO_OK)return mapped;
    if(source.hasHandle) {
        if(source.reservedVoiceChannels!=channels)return WORLD_AUDIO_INVALID_BINDING;
        first=source.reservedVoiceId;available=true;return WORLD_AUDIO_OK;
    }
    // Full PAL2638C8..2638E8 calls26C560(47,0,count,false,locks).
    // On the reverse scan the first channel is the lowest in the contiguous
    // run. A completed source remains locked until its original STOP path.
    uint16_t run=0;
    for(int id=47;id>=0;--id) {
        run=used[static_cast<size_t>(id)] ? 0:static_cast<uint16_t>(run+1u);
        if(run==channels) {first=static_cast<uint16_t>(id);available=true;break;}
    }
    return WORLD_AUDIO_OK;
}


WorldAudioResult bVoiceMgr::Assign(const WorldAudioRequest& operation,uint16_t deviceId)
{
    if (!queue_ || deviceId>=48 || operation.reservedVoiceId!=deviceId ||
        (operation.reservedVoiceChannels!=1 && operation.reservedVoiceChannels!=2) ||
        operation.reservedVoiceChannels>48-deviceId || operation.queueGeneration!=queue_->generation ||
        operation.slot>=queue_->capacity || !operation.instance)
        return WORLD_AUDIO_INVALID_BINDING;
    const auto& retained=queue_->requests[operation.slot];
    if (retained.sequence!=operation.sequence || retained.deliveryState!=WORLD_AUDIO_REQUEST_INFLIGHT ||
        retained.instance!=operation.instance || retained.instanceGeneration!=operation.instanceGeneration ||
        retained.kind!=operation.kind || retained.reservedVoiceId!=operation.reservedVoiceId ||
        retained.reservedVoiceChannels!=operation.reservedVoiceChannels)
        return WORLD_AUDIO_STALE_REFERENCE;
    if (operation.kind!=WORLD_AUDIO_PLAY && operation.kind!=WORLD_AUDIO_PRIME &&
        operation.kind!=WORLD_AUDIO_OPEN_STREAM && operation.kind!=WORLD_AUDIO_OPEN_CHAIN)
        return WORLD_AUDIO_INVALID_BINDING;
    const WorldAudioVoiceRef sample{operation.instance,operation.instanceGeneration};
    if (worldAudioValidateVoice(queue_,sample)!=WORLD_AUDIO_OK)return WORLD_AUDIO_STALE_REFERENCE;
    // Replaying an existing native source advances its operation identity;
    // it is not a second original GetVoice allocation. In particular, an
    // older queued PLAY must not undo a newer producer STOP's hasHandle=0.
    for(auto& voice:voices_) {
        const auto held=voice.GetSample();
        if(voice.GetId()==deviceId && held.instance==sample.instance &&
            held.generation==sample.generation) {
            voice.Set(deviceId,sample,operation.sequence);
            return WORLD_AUDIO_OK;
        }
    }
    if (!GetVoice(deviceId,sample,operation.sequence))return WORLD_AUDIO_CAPACITY;
    // PRIME/PLAY already published their ordered handle intent. A physical
    // assignment cannot overwrite later source state while commands drain.
    return WORLD_AUDIO_OK;
}

WorldAudioResult bVoiceMgr::CompleteStop(const WorldAudioRequest& operation)
{
    if(!queue_ || operation.queueGeneration!=queue_->generation ||
        operation.slot>=queue_->capacity || operation.kind!=WORLD_AUDIO_STOP ||
        !operation.instance)return WORLD_AUDIO_INVALID_BINDING;
    const auto& retained=queue_->requests[operation.slot];
    if(retained.kind!=WORLD_AUDIO_STOP || retained.sequence!=operation.sequence ||
        retained.queueGeneration!=operation.queueGeneration ||
        retained.deliveryState!=WORLD_AUDIO_REQUEST_INFLIGHT ||
        retained.instance!=operation.instance ||
        retained.instanceGeneration!=operation.instanceGeneration ||
        retained.releaseDeviceVoice!=operation.releaseDeviceVoice ||
        !operation.instance->occupied ||
        operation.instance->generation!=operation.instanceGeneration)
        return WORLD_AUDIO_STALE_REFERENCE;
    for(const auto& voice:voices_) {
        const auto held=voice.GetSample();
        if(held.instance==operation.instance && held.generation==operation.instanceGeneration &&
            voice.OperationSequence()>operation.sequence)
            return WORLD_AUDIO_STALE_REFERENCE;
    }
    // PAL274698..27473C retains primed handles and releases ordinary ones.
    // The native transport keeps the association until actual device STOP
    // has finished, including PLAY->STOP produced before the first pump.
    // Do not clear hasHandle here: a later queued PLAY may already own it.
    if(operation.releaseDeviceVoice)
        ReleaseSample({operation.instance,operation.instanceGeneration});
    return WORLD_AUDIO_OK;
}

WorldAudioResult bVoiceMgr::CompleteUpdate(const WorldAudioRequest& operation,
    const bVoiceStatusSnapshot& status)
{
    if (!queue_ || operation.queueGeneration!=queue_->generation ||
        operation.kind!=WORLD_AUDIO_DRIVER_UPDATE || operation.slot>=queue_->capacity)
        return WORLD_AUDIO_INVALID_BINDING;
    auto& retained=queue_->requests[operation.slot];
    if (retained.kind!=WORLD_AUDIO_DRIVER_UPDATE || retained.sequence!=operation.sequence ||
        retained.queueGeneration!=operation.queueGeneration ||
        retained.deliveryState!=WORLD_AUDIO_REQUEST_INFLIGHT || retained.deviceUpdateCompleted)
        return WORLD_AUDIO_STALE_REFERENCE;
    // Consuming a real status packet does not acknowledge its transport
    // request. The backend still has to finish its own submission borrow.
    bool started=false;
    const auto result=Update(status,&started);
    if (started)retained.deviceUpdateCompleted=1;
    return result;
}

// PAL27B2F0: actual driver observations select Stop; every still bound
// sample then runs its original update. Snapshot data is supplied by the
// device, matched to the retained DRIVER_UPDATE, never synthesized here.
WorldAudioResult bVoiceMgr::Update(const bVoiceStatusSnapshot& status,bool* operationStarted)
{
    if (operationStarted)*operationStarted=false;
    if (!queue_)return WORLD_AUDIO_INVALID_BINDING;
    size_t commands=0;
    for (const auto& voice:voices_) {
        const auto sample=voice.GetSample();
        if (!sample.instance)continue;
        if (worldAudioValidateVoice(queue_,sample)!=WORLD_AUDIO_OK)return WORLD_AUDIO_STALE_REFERENCE;
        const auto& observed=status[voice.GetId()];
        if (observed.sample.instance!=sample.instance || observed.sample.generation!=sample.generation ||
            observed.operationSequence!=voice.OperationSequence())return WORLD_AUDIO_STALE_REFERENCE;
        commands+=4; // Stop + sample volume + a real ready callback's Play/volume
    }
    if (worldAudioAvailableRequests(queue_)<commands || queue_->nextSequence>UINT64_MAX-commands)
        return WORLD_AUDIO_CAPACITY;
    if (operationStarted)*operationStarted=true;
    for (auto& voice:voices_) {
        const auto sample=voice.GetSample();
        if (!sample.instance)continue;
        const auto& observed=status[voice.GetId()];
        // A source may have published a later PLAY/OPEN before this older
        // DRIVER_UPDATE reaches the consumer. Its earlier device observation
        // cannot stop or fade that newer source intent.
        if(sample.instance->playSequence!=UINT64_MAX &&
            sample.instance->playSequence>observed.operationSequence)continue;
        if (!observed.playing && (!sample.instance->primed || sample.instance->playing)) {
            PrintVoiceAllocsAndReleases("US",voice.GetId(),sample.instance->cue->name);
            auto* speech=sample.instance->completionKind==WORLD_AUDIO_COMPLETION_SPEECH
                ? sample.instance->completionSpeech:nullptr;
            const auto result=worldAudioStopInstance(queue_,sample.instance);
            if (result!=WORLD_AUDIO_OK)return result;
            // STOP changes playSequence. Use the actual device's completed
            // PLAY identity so only the displayed sentence gets its EOF.
            if(speech && speech->caption.sequence==observed.operationSequence)speech->caption.finished=1;
            if (!sample.instance->primed)ReleaseVoice(voice);
        }
        // A real completion callback may retire the sample. Its queued
        // STOP still retains resources, but its CPU object must not update.
        if (!sample.instance->occupied || sample.instance->retiring ||
            sample.instance->generation!=sample.generation)continue;
        const auto result=worldAudioUpdateSample(queue_,sample,observed.prebuffered);
        if (result!=WORLD_AUDIO_OK)return result;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult bVoiceMgr::PrintVoiceUsage(const bVoiceStatusSnapshot& status) const
{
    if (!queue_ || !queue_->library)return WORLD_AUDIO_INVALID_BINDING;
    uint32_t used=0,lost=0;
    for (auto at=voices_.rbegin();at!=voices_.rend();++at) {
        const auto sample=at->GetSample();if (!sample.instance)continue;
        if (worldAudioValidateVoice(queue_,sample)!=WORLD_AUDIO_OK)return WORLD_AUDIO_STALE_REFERENCE;
        const auto& observed=status[at->GetId()];
        if (observed.sample.instance!=sample.instance || observed.sample.generation!=sample.generation ||
            observed.operationSequence!=at->OperationSequence())return WORLD_AUDIO_STALE_REFERENCE;
        const auto channels=observed.channels;
        if (channels!=1 && channels!=2)return WORLD_AUDIO_INVALID_BINDING;
        queue_->library->Printf("*** Voice %d allocated to sample %s (%d channels) ***\n",
            static_cast<int>(at->GetId()),sample.instance->cue->name,static_cast<int>(channels));
        used+=channels==2 ? 2u:1u;
    }
    queue_->library->Printf("*** TOTAL NUM VOICES IN USE %d *** \n",static_cast<int>(used));
    queue_->library->Printf("Possible lost voice(s): ");
    for (uint16_t id=0;id<48;++id) {
        if (!status[id].playing)continue;
        const bool associated=std::any_of(voices_.begin(),voices_.end(),[&](const bVoice& voice) {
            if (!voice.GetSample().instance)return false;
            return voice.GetId()==id || (status[voice.GetId()].channels==2 && voice.GetId()+1u==id);
        });
        if (!associated) { queue_->library->Printf("%d, ",static_cast<int>(id));++lost; }
    }
    if (lost)queue_->library->Printf(" = %d total Lost Voice(s)\n",static_cast<int>(lost));
    else queue_->library->Printf(" NONE \n");
    // PAL's final hardware-specific low-level dump266010 has no gameplay
    // writes. Device-specific diagnostics remain owned by the actual backend.
    return WORLD_AUDIO_OK;
}
