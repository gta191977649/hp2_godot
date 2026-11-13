#include "indep/src/speech/SEDWedge.hpp"
#include "bsound/include/bSound.h"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bFile.hpp"
#include "bware/inc/bWare.hpp"
#include "Timer.hpp"
#include "CarSounds.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

struct WorldStreamAudioResources {
    const hp2::GameResources* files{};
    const GameTime* clock{};
    SpeechEngineState engine{};
    std::vector<SpeechBankView> bankViews;
    std::vector<uint8_t> bankImage;
    std::vector<uint8_t> eventImage;
    WorldAudioStreamSource chainSource;
    bool cleanupPending{};
    size_t bankCount{};
    std::array<std::unique_ptr<bSegment>,10> segments;
    size_t segmentCount{};
    std::array<uint8_t,2048> eventLog{};

};

namespace {

WorldStreamAudioResult audioResult(WorldStreamAudio& stream,WorldAudioResult result)
{
    stream.result=result==WORLD_AUDIO_OK ? WORLD_STREAM_AUDIO_OK :
        result==WORLD_AUDIO_CAPACITY ? WORLD_STREAM_AUDIO_CAPACITY :
        result==WORLD_AUDIO_RESOURCE_FAILURE ? WORLD_STREAM_AUDIO_RESOURCE_FAILURE :
        WORLD_STREAM_AUDIO_INVALID_BINDING;
    return stream.result;
}

void writeWord(uint8_t* bytes,uint32_t value)
{
    bytes[0]=static_cast<uint8_t>(value);bytes[1]=static_cast<uint8_t>(value>>8);
    bytes[2]=static_cast<uint8_t>(value>>16);bytes[3]=static_cast<uint8_t>(value>>24);
}

bool loadDataFile(const hp2::GameResources& files,const char* filename,
    std::vector<uint8_t>& bytes)
{
    if (!filename)return false;
    const auto file=files.TryOpenPhysical(filename);
    if (!file || file->range.Size()>static_cast<uint64_t>(std::numeric_limits<int32_t>::max()))
        return false;
    bytes=file->range.Read(0,static_cast<size_t>(file->range.Size()));
    return true;
}

void releaseInputImages(WorldStreamAudioResources& owner)
{
    owner.chainSource.file={};
    owner.bankImage.clear();owner.bankImage.shrink_to_fit();
    owner.bankCount=0;
    owner.bankViews.clear();owner.bankViews.shrink_to_fit();
    owner.eventImage.clear();owner.eventImage.shrink_to_fit();
    for (auto& segment:owner.segments)segment.reset();
    owner.segmentCount=0;
}

} // namespace

WorldStreamAudioResult worldStreamAudioBind(WorldStreamAudio& stream,
    WorldAudioMessages& messages,bSound& sound,const hp2::GameResources& files,
    const GameTime& clock,const CarSoundGlobals& globals,const SoundDriverState& driver)
{
    if (stream.resources || stream.initialized || stream.voice.instance ||
        messages.engine || messages.audioEnabled || messages.speechEnabled || !sound.Driver() ||
        sound.Driver()->library!=&sound)
        return stream.result=WORLD_STREAM_AUDIO_INVALID_BINDING;
    auto owner=std::make_unique<WorldStreamAudioResources>();
    owner->files=&files;owner->clock=&clock;
    // PAL205CCC..205D20 constructs the shared base-type1 _STREAM chain and
    // retains it across Cleanup. The live pool owns the actual sample.
    owner->chainSource.bank.name="_STREAM";
    owner->chainSource.cue.name="_STREAM";
    owner->chainSource.bank.cues=&owner->chainSource.cue;
    owner->chainSource.bank.cueCount=1;
    stream.messages=&messages;
    stream.audio=sound.Driver();
    stream.audioEnabled=&globals.policy.audioSystemEnabled;stream.speechEnabled=&driver.speechEnabled;
    messages.audioEnabled=stream.audioEnabled;messages.speechEnabled=stream.speechEnabled;
    stream.state=1;stream.muted=0;stream.pausedForMenu=0;stream.timerMilliseconds=0;
    stream.resources=owner.release();
    return stream.result=WORLD_STREAM_AUDIO_OK;
}

WorldStreamAudioResult worldStreamAudioInitialize(WorldStreamAudio& stream,
    const char* eventsFilename,const char* unusedHeaderFilename,
    const char* dataFilename,const char* banksFilename)
{
    // The released Initialize never reads its HDR argument. A missing HDR
    // is not substituted, synthesized or used to select another resource.
    (void)unusedHeaderFilename;
    if (!stream.resources || stream.resources->cleanupPending || stream.initialized || !stream.audio || !stream.audio->library ||
        stream.audio->library->GetStatus()!=1 || !stream.messages || !dataFilename || !banksFilename)
        return stream.result=WORLD_STREAM_AUDIO_INVALID_BINDING;
    auto& owner=*stream.resources;
    const auto fail=[&](WorldStreamAudioResult result) {
        if (owner.engine.magic==0x01789A34u)(void)spchEngineDeinitialize(&owner.engine);
        releaseInputImages(owner);
        stream.initialized=0;
        return stream.result=result;
    };
    try {
        if (!loadDataFile(*owner.files,banksFilename,owner.bankImage) || owner.bankImage.size()<4)
            return fail(WORLD_STREAM_AUDIO_RESOURCE_FAILURE);
        owner.bankCount=hp2::ReadU32(owner.bankImage.data());
        if (owner.bankCount>2000u ||
            owner.bankCount>owner.bankImage.size()/16u)
            return fail(WORLD_STREAM_AUDIO_RESOURCE_FAILURE);
        // PAL205C68..205C7C passes image + table[index].word1 to AddBank;
        // word2 is not a byte count (its first final English value is zero).
        // The native bounded view ends at the next distinct bank offset,
        // or at EOF for the final bank. Keep table order and aliased offsets.
        std::vector<size_t> bankBoundaries;
        bankBoundaries.reserve(owner.bankCount+1);
        for (size_t index=0;index<owner.bankCount;++index) {
            const size_t offset=hp2::ReadU32(owner.bankImage.data()+index*16u+4);
            if (offset<owner.bankCount*16u || offset>=owner.bankImage.size())
                return fail(WORLD_STREAM_AUDIO_RESOURCE_FAILURE);
            bankBoundaries.push_back(offset);
        }
        bankBoundaries.push_back(owner.bankImage.size());
        std::sort(bankBoundaries.begin(),bankBoundaries.end());
        const auto data=owner.files->TryOpenPhysical(dataFilename);
        if (!data)return fail(WORLD_STREAM_AUDIO_RESOURCE_FAILURE);
        owner.chainSource.file=data->range;
        owner.chainSource.path=dataFilename;
        // PAL205D1C/24 stores24000 into the chain sample's frequency cell.
        // The independent SPCH encoded-data rate below remains4410.
        owner.chainSource.sampleRate=24000;
        if (spchEngineInitialize(&owner.engine,stream.messages,&stream,0,
            static_cast<uint32_t>(spchGetSampleDataRate(22050,16,1)))!=SPEECH_DATABASE_OK)
            return fail(WORLD_STREAM_AUDIO_INVALID_BINDING);
        owner.bankViews.resize(2000);
        if (spchBankBindStorage(&owner.engine.banks,owner.bankViews.data(),owner.bankViews.size(),
                &owner.engine.magic,&owner.engine.cyclePhase)!=SPEECH_DATABASE_OK)
            return fail(WORLD_STREAM_AUDIO_INVALID_BINDING);
        owner.engine.externalOwner=&stream;
        // Actual S_WriteEVLog205AE0 is a bare return. No fake file sink is
        // installed; the library's optional writer remains null while all
        // real Save/Clear/Start/End buffer transitions still execute.
        if (spchEventLogInitialize(&owner.engine,owner.eventLog.data(),
                static_cast<uint32_t>(owner.eventLog.size()),nullptr)!=SPEECH_DATABASE_OK ||
            spchInitializeRuleCallbacks(&owner.engine,&stream)!=SPEECH_DATABASE_OK)
            return fail(WORLD_STREAM_AUDIO_INVALID_BINDING);
        if (!loadDataFile(*owner.files,eventsFilename,owner.eventImage) ||
            spchResolveData(&owner.engine.databases,owner.eventImage.data(),owner.eventImage.size())!=SPEECH_DATABASE_OK)
            return fail(WORLD_STREAM_AUDIO_RESOURCE_FAILURE);
        for (size_t index=0;index<owner.bankCount;++index) {
            auto* entry=owner.bankImage.data()+index*16u;
            const auto offset=hp2::ReadU32(entry+4);
            const auto next=std::upper_bound(bankBoundaries.begin(),bankBoundaries.end(),offset);
            const size_t bytes=*next-offset;
            int32_t bank=-1;
            if (spchAddBank(&owner.engine.banks,
                    {owner.bankImage.data()+offset,bytes},&bank)!=SPEECH_DATABASE_OK)
                return fail(WORLD_STREAM_AUDIO_RESOURCE_FAILURE);
            // Includes the first table word. The count was retained before
            // SPCH overwrites that same BAN image with returned bank indices.
            writeWord(entry,static_cast<uint32_t>(bank));
        }
        if (!stream.voice.instance) {
            const auto created=worldAudioCreateSpeechVoice(stream.audio,&stream,
                &owner.chainSource.bank,&owner.chainSource.cue,&stream.voice);
            if (created!=WORLD_AUDIO_OK)return fail(audioResult(stream,created));
        }
        stream.voice.instance->streamSource=&owner.chainSource;
        for (auto& segment:owner.segments)segment=std::make_unique<bSegment>(*stream.audio->library);
        owner.segmentCount=0;
        worldAudioMessagesClear(stream.messages);
        worldAudioMessagesClear(stream.messages);
        const auto mute=worldStreamAudioMute(&stream,0);
        if (mute!=WORLD_STREAM_AUDIO_OK)return fail(mute);
        stream.initialized=1;
        return stream.result=WORLD_STREAM_AUDIO_OK;
    } catch (...) {
        fail(WORLD_STREAM_AUDIO_RESOURCE_FAILURE);
        throw;
    }
}

WorldStreamAudioResult worldStreamAudioCleanup(WorldStreamAudio* stream)
{
    if (!stream)return WORLD_STREAM_AUDIO_INVALID_BINDING;
    if (!stream->resources)return stream->initialized?WORLD_STREAM_AUDIO_INVALID_BINDING:WORLD_STREAM_AUDIO_OK;
    auto& owner=*stream->resources;
    if (owner.cleanupPending) {
        // Only actual acknowledgement of the STOP and every earlier retained
        // command ends the physical DAT borrow. Completion event8 alone does
        // not release file data, nor does an earlier PLAY acknowledgement.
        if (stream->voice.instance && stream->voice.instance->requestReferences)
            return stream->result=WORLD_STREAM_AUDIO_BUSY;
        releaseInputImages(owner);owner.cleanupPending=false;
        return stream->result=WORLD_STREAM_AUDIO_OK;
    }
    if (stream->initialized!=1)return stream->result=WORLD_STREAM_AUDIO_OK;
    if (!stream->audio || !stream->voice.instance)
        return stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;
    if (stream->voice.instance->hasHandle && worldAudioAvailableRequests(stream->audio)<1)
        return stream->result=WORLD_STREAM_AUDIO_CAPACITY;
    const auto stopped=worldAudioStopInstance(stream->audio,stream->voice.instance);
    if (stopped!=WORLD_AUDIO_OK)return audioResult(*stream,stopped);
    if (spchEngineDeinitialize(&owner.engine)!=SPEECH_DATABASE_OK)
        return stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;
    stream->voice.instance->completionSpeech=nullptr;
    stream->voice.instance->completionKind=WORLD_AUDIO_COMPLETION_NONE;
    stream->initialized=0;stream->pausedForMenu=0;owner.cleanupPending=true;
    return worldStreamAudioCleanup(stream);
}

WorldStreamAudioResult worldStreamAudioDestroy(WorldStreamAudio* stream)
{
    if (!stream)return WORLD_STREAM_AUDIO_INVALID_BINDING;
    const auto cleaned=worldStreamAudioCleanup(stream);
    if (cleaned!=WORLD_STREAM_AUDIO_OK)return cleaned;
    if (stream->voice.instance) {
        auto* sample=stream->voice.instance;
        if (sample->occupied && sample->generation==stream->voice.generation) {
            if (sample->requestReferences)return stream->result=WORLD_STREAM_AUDIO_BUSY;
            if (!sample->retiring) {
                const auto retired=worldAudioRetireVoice(stream->audio,stream->voice);
                if (retired!=WORLD_AUDIO_OK)return audioResult(*stream,retired);
            }
            if (sample->requestReferences)return stream->result=WORLD_STREAM_AUDIO_BUSY;
        }
        // Acknowledging the final STOP can already have cleared/reused this
        // pool slot. That generation no longer borrows our chain metadata.
        stream->voice={};
    }
    if (stream->messages && stream->messages->engine==&stream->resources->engine)
        stream->messages->engine=nullptr;
    if (stream->messages) {
        if(stream->messages->audioEnabled==stream->audioEnabled)stream->messages->audioEnabled=nullptr;
        if(stream->messages->speechEnabled==stream->speechEnabled)stream->messages->speechEnabled=nullptr;
    }
    delete stream->resources;stream->resources=nullptr;
    stream->messages=nullptr;stream->audio=nullptr;
    stream->audioEnabled=nullptr;stream->speechEnabled=nullptr;
    return stream->result=WORLD_STREAM_AUDIO_OK;
}

int32_t worldStreamAudioTimerGetTick(const WorldStreamAudio* stream)
{
    if (!stream || !std::isfinite(stream->timerMilliseconds) ||
        stream->timerMilliseconds<-2147483648.0f || stream->timerMilliseconds>=2147483648.0f)
        return std::numeric_limits<int32_t>::min();
    return convertFloatToInt32(stream->timerMilliseconds);
}

int32_t worldStreamAudioRequestStop(WorldStreamAudio* stream)
{
    if (!stream || !stream->audio || !stream->voice.instance) {
        if (stream)stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;
        return 0;
    }
    return audioResult(*stream,worldAudioStopInstance(stream->audio,stream->voice.instance))==WORLD_STREAM_AUDIO_OK;
}

int32_t worldStreamAudioStop(WorldStreamAudio* stream)
{
    if (!stream || stream->initialized!=1)return 0;
    if (!worldStreamAudioRequestStop(stream))return 0;
    // PAL273360 is the released empty chain method called here. It changes
    // no handle/prime flag; do not invent an unlock/release implementation.
    // A sentence STOP may be followed by another queued sentence while the
    // pause menu still owns the audio cursor. Keep the native pause intent
    // until the coordinator resumes or the entire speech owner is cleaned.
    stream->state=1;
    return 1;
}

int32_t worldStreamAudioCancel(WorldStreamAudio* stream)
{
    if (!stream || stream->initialized!=1)return 0;
    if (!stream->messages){stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;return 0;}
    worldAudioMessagesClear(stream->messages);
    if (stream->state!=1 && !worldStreamAudioStop(stream))return 0;
    return 1;
}

void worldStreamAudioFade(WorldStreamAudio* stream,const int32_t* audioEnabled,float duration)
{
    (void)duration;
    if (!stream || !audioEnabled) {
        if (stream)stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;
        return;
    }
    if (!*audioEnabled || (stream->state!=2 && stream->state!=32))return;
    if (!worldStreamAudioRequestStop(stream))return;
    stream->state=1;
}

WorldStreamAudioResult worldStreamAudioMute(WorldStreamAudio* stream,int32_t muted)
{
    if (!stream)return WORLD_STREAM_AUDIO_INVALID_BINDING;
    if (stream->state==2 || stream->state==32) {
        const auto result=(!muted && stream->pausedForMenu)
            ? worldAudioUnmuteVoiceAtCurrentVolume(stream->audio,stream->voice)
            : worldAudioMuteVoice(stream->audio,stream->voice,muted);
        if (result!=WORLD_AUDIO_OK)return audioResult(*stream,result);
    }
    stream->muted=muted;
    return stream->result=WORLD_STREAM_AUDIO_OK;
}

WorldStreamAudioResult worldStreamAudioPauseForMenu(WorldStreamAudio* stream,int32_t paused)
{
    if(!stream)return WORLD_STREAM_AUDIO_INVALID_BINDING;
    const int32_t wanted=paused!=0;
    if(stream->pausedForMenu==wanted)return stream->result=WORLD_STREAM_AUDIO_OK;
    // Suspend the same retained voice that PAL speechMute muted. The device
    // command preserves the sample cursor, unlike SET_MUTED. An unprimed
    // voice retains the request and applies it once its ready callback starts.
    if ((stream->state==2 || stream->state==32) && stream->voice.instance) {
        if(!stream->audio)return stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;
        const auto result=worldAudioPauseVoice(stream->audio,stream->voice,wanted);
        if(result!=WORLD_AUDIO_OK)return audioResult(*stream,result);
    }
    stream->pausedForMenu=wanted;
    return stream->result=WORLD_STREAM_AUDIO_OK;
}

int32_t worldStreamAudioGetEventPending(WorldStreamAudio* stream,int32_t channel)
{
    if (!stream || stream->initialized!=1)return 0;
    if (!stream->messages || channel<0 || channel>=WORLD_AUDIO_BANK_LIMIT) {
        stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;
        return 0;
    }
    return stream->messages->bankReferences[channel]!=0;
}

WorldStreamAudioResult worldStreamAudioAddSentencePart(WorldStreamAudio* stream,
    int32_t bankIndex,uint32_t offset,uint32_t bytes)
{
    if (!stream || !stream->resources)return WORLD_STREAM_AUDIO_INVALID_BINDING;
    auto& owner=*stream->resources;
    // The source silently ignores an out-of-range bank index.
    if (bankIndex<0 || static_cast<size_t>(bankIndex)>=owner.bankCount)
        return stream->result=WORLD_STREAM_AUDIO_OK;
    if (owner.segmentCount>=owner.segments.size())
        return stream->result=WORLD_STREAM_AUDIO_CAPACITY;
    auto& segment=owner.segments[owner.segmentCount];
    if (!segment)return stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;
    const auto base=hp2::ReadU32(owner.bankImage.data()+static_cast<size_t>(bankIndex)*16u+12);
    const uint32_t absolute=base+offset; // PAL32-bit addition, including wrap.
    if (!hp2::Inside(absolute,bytes,owner.chainSource.file.Size()))
        return stream->result=WORLD_STREAM_AUDIO_RESOURCE_FAILURE;
    segment->Cleanup();
    segment->Initialize("Segment",absolute,bytes,0);
    ++owner.segmentCount;
    return stream->result=WORLD_STREAM_AUDIO_OK;
}

int32_t worldStreamAudioSentencePartRequest(WorldStreamAudio* stream,
    const uint32_t* parameters,size_t parameterCount)
{
    if (!stream || !parameters || parameterCount<3) {
        if (stream)stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;
        return -1;
    }
    int32_t bankIndex;
    std::memcpy(&bankIndex,parameters,sizeof(bankIndex));
    if (worldStreamAudioAddSentencePart(stream,bankIndex,parameters[1],parameters[2])!=WORLD_STREAM_AUDIO_OK)
        return -1;
    // PAL205A28 always returns0 after the actual AddSentencePart call.
    return 0;
}

WorldAudioResult worldStreamAudioSampleCallback(WorldStreamAudio* stream,
    uint32_t event,WorldAudioVoiceRef voice)
{
    if (!stream || !stream->audio || voice.instance!=stream->voice.instance ||
        voice.generation!=stream->voice.generation)
        return WORLD_AUDIO_INVALID_BINDING;
    if (event==4) {
        if (!stream->audioEnabled || !stream->speechEnabled)return WORLD_AUDIO_INVALID_BINDING;
        const bool play=*stream->audioEnabled && *stream->speechEnabled;
        const size_t needed=play ? (stream->pausedForMenu ? 3u:2u):1u;
        if (worldAudioAvailableRequests(stream->audio)<needed)return WORLD_AUDIO_CAPACITY;
        if (play) {
            const auto result=worldAudioPlayVoice(stream->audio,voice,1.0f,0.0f,0.0f);
            if (result!=WORLD_AUDIO_OK)return result;
        }
        const auto volume=worldAudioSetVoiceVolume(stream->audio,voice,1.0f);
        if(volume!=WORLD_AUDIO_OK)return volume;
        if (play && stream->pausedForMenu)
            return worldAudioPauseVoice(stream->audio,voice,1);
        return WORLD_AUDIO_OK;
    }
    if (event==8)stream->state=32;
    return WORLD_AUDIO_OK;
}

const SpeechEngineState* worldStreamAudioEngine(const WorldStreamAudio* stream)
{
    return stream && stream->resources ? &stream->resources->engine : nullptr;
}

size_t worldStreamAudioBankCount(const WorldStreamAudio* stream)
{
    return stream && stream->resources ? stream->resources->bankCount : 0;
}

size_t worldStreamAudioSegmentCount(const WorldStreamAudio* stream)
{
    return stream && stream->resources ? stream->resources->segmentCount : 0;
}

int32_t worldStreamAudioQueueSentenceEvent(WorldStreamAudio* stream,int32_t channel)
{
    if (!stream || stream->initialized!=1)return 0;
    if (!stream->resources || channel<0 || channel>=8) {
        stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;return 0;
    }
    auto& engine=stream->resources->engine;
    engine.operationResult=SPEECH_DATABASE_OK;
    (void)spchChoose(&engine,channel);(void)spchPlay(&engine,channel);
    if (engine.operationResult!=SPEECH_DATABASE_OK) {
        stream->result=engine.operationResult==SPEECH_DATABASE_CAPACITY?
            WORLD_STREAM_AUDIO_CAPACITY:WORLD_STREAM_AUDIO_RESOURCE_FAILURE;
        return 0;
    }
    return 1;
}

int32_t worldStreamAudioPlaySentence(WorldStreamAudio* stream,int32_t channel,float volume)
{
    (void)channel;(void)volume; // Both incoming values are unused in the complete PAL body.
    if (!stream || stream->initialized!=1)return 0;
    if (!stream->resources || !stream->audio || !stream->audio->library || !stream->voice.instance) {
        stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;return 0;
    }
    auto& owner=*stream->resources;
    const bool stop=stream->state==2 || stream->state==32;
    // SetPath("") and its source restore each use the same ordered device
    // transport. Reserve them together with all chain parts before Stop or
    // any segment publication; otherwise capacity could strand a half-chain.
    const size_t required=owner.segmentCount+(stop?1u:0u)+(owner.segmentCount?2u:0u);
    if (worldAudioAvailableRequests(stream->audio)<required ||
        required>UINT64_MAX-stream->audio->nextSequence ||
        stream->voice.instance->requestReferences>SIZE_MAX-required) {
        stream->result=WORLD_STREAM_AUDIO_CAPACITY;return 0;
    }
    for (size_t i=0;i<owner.segmentCount;++i) {
        const auto& segment=owner.segments[i];
        if (!segment || segment->GetSize()<48 ||
            !hp2::Inside(segment->GetOffset(),segment->GetSize(),owner.chainSource.file.Size())) {
            stream->result=WORLD_STREAM_AUDIO_RESOURCE_FAILURE;return 0;
        }
    }
    if (stop && !worldStreamAudioStop(stream))return 0;
    if (!owner.segmentCount)return 1;
    const auto category=worldAudioSetVoiceCategory(stream->audio,stream->voice,6);
    if (category!=WORLD_AUDIO_OK) {audioResult(*stream,category);return 0;}
    auto& sample=*stream->voice.instance;
    if (sample.completionKind!=WORLD_AUDIO_COMPLETION_NONE &&
        (sample.completionKind!=WORLD_AUDIO_COMPLETION_SPEECH || sample.completionSpeech!=stream)) {
        stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;return 0;
    }
    sample.completionKind=WORLD_AUDIO_COMPLETION_SPEECH;sample.completionSpeech=stream;
    const auto path=stream->audio->library->SetPath("");
    if (path!=WORLD_AUDIO_OK) {audioResult(*stream,path);return 0;}
    bSampleChain chain(*stream->audio,stream->voice,owner.chainSource);
    for (size_t i=0;i<owner.segmentCount;++i) {
        const auto& segment=*owner.segments[i];
        const auto result=chain.AddToChain(owner.chainSource.path.c_str(),segment.GetOffset()+48u,
            segment.GetSize()-48u,i+1u==owner.segmentCount);
        if (result!=WORLD_AUDIO_OK) {
            (void)stream->audio->library->SetPath("sound\\");audioResult(*stream,result);return 0;
        }
    }
    const auto restored=stream->audio->library->SetPath("sound\\");
    if (restored!=WORLD_AUDIO_OK) {audioResult(*stream,restored);return 0;}
    owner.segmentCount=0;stream->state=2;stream->result=WORLD_STREAM_AUDIO_OK;return 1;
}

WorldStreamAudioResult worldStreamAudioUpdate(WorldStreamAudio* stream)
{
    if (!stream)return WORLD_STREAM_AUDIO_INVALID_BINDING;
    if (!stream->initialized || stream->muted)return stream->result=WORLD_STREAM_AUDIO_OK;
    if (!stream->resources || !stream->resources->clock)
        return stream->result=WORLD_STREAM_AUDIO_INVALID_BINDING;
    // PAL206210 reads GP-4C90, the 60 Hz RealTimeFrames owner. The
    // separate GP-4C98 RealTimer counts 4000 Hz ticks. Keep the source
    // multiplication by literal3C888889 then1000 on the frame counter.
    const float seconds=static_cast<float>(stream->resources->clock->realTimeFrames)*0.01666666753590106964111328125f;
    stream->timerMilliseconds=seconds*1000.0f;
    if (stream->state==32)stream->state=1; //273360 has no released effects.
    if ((stream->state==1 || stream->state==8) && worldStreamAudioGetEventPending(stream,0)) {
        if (!worldStreamAudioQueueSentenceEvent(stream,0))return stream->result;
        if (!worldStreamAudioPlaySentence(stream,0,1.0f))return stream->result;
    }
    return stream->result=WORLD_STREAM_AUDIO_OK;
}
