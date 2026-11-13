#include "indep/inc/Collision.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "CameraMover.hpp"
#include "indep/inc/Sound3d.hpp"
#include "indep/inc/Sound3dSample.hpp"
#include "indep/inc/RaceParameters.hpp"
#include "indep/inc/Smokeable.hpp"
#include "bware/inc/bWare.hpp"
#include <cmath>
#include <initializer_list>
#include <memory>
#include <new>
#include <string.h>

#include "bsound/include/bSample.h"

// Native storage for the original expandable bSlotPool, not physical voices.
// A POD pointer in the C-compatible owner keeps existing zero initialization
// valid; no container object is copied or cleared by its initialization path.
struct CollisionAudioVoiceBlock {
    CollisionAudioVoiceBlock* next{};
    CollisionAudioVoice voices[(40>>2)+1]{};
};

WorldAudioResult worldAudioPlayBustRoadBlock(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager, const float position[4])
{
    if (!queue || !manager || !manager->channelVolumes ||
        manager->channelVolumeCount == 0 || !position)
        return WORLD_AUDIO_INVALID_BINDING;

    /* The original channel-zero query has no side effects and its return
     * value does not feed playback. Keep the actual manager binding. */
    const float unusedChannelVolume = manager->channelVolumes[0];
    (void)unusedChannelVolume;
    WorldAudioInstance* voice = worldAudioPlayAtPointScaled(queue, "FinishLine", position,
        1.0f, 0.5f, -1, 1);
    if (!voice) return queue->result;
    // PAL1E56D4 addresses the private bSample at object+28. Its tail
    // setter writes category+84; it does not assign a generic object tag.
    return worldAudioSetVoiceCategory(queue, worldAudioVoiceReference(voice), UINT32_MAX);
}

WorldAudioResult worldImpactAudioBind(WorldImpactAudio* impact, WorldAudioEffects* audio)
{
    if (!impact || !audio || !audio->queue || impact->registered || impact->collisions ||
        impact->voiceBlocks || impact->firstDetached) return WORLD_AUDIO_INVALID_BINDING;
    for (const auto& voice:impact->voices)
        if (voice.occupied) return WORLD_AUDIO_INVALID_BINDING;
    *impact={};
    impact->audio = audio;
    // Only the effect owner exists here. InGame and the original sample
    // table are published later by CollisionSounds::LoadSampleTable.
    return audio->queue->result;
}

size_t collisionAudioVoiceCapacity(const WorldImpactAudio* owner)
{
    if (!owner) return 0;
    size_t count=40;
    for (auto* block=owner->voiceBlocks;block;block=block->next) count+=(40>>2)+1;
    return count;
}

const CollisionAudioVoice* collisionAudioVoiceAt(const WorldImpactAudio* owner, size_t index)
{
    if (!owner) return nullptr;
    if (index<40) return &owner->voices[index];
    index-=40;
    for (auto* block=owner->voiceBlocks;block;block=block->next) {
        if (index<(40>>2)+1) return &block->voices[index];
        index-=(40>>2)+1;
    }
    return nullptr;
}

WorldAudioResult worldImpactAudioPlay(WorldImpactAudio* impact, const float position[4])
{
    if (!impact || !impact->audio || !impact->audio->enabled) return WORLD_AUDIO_INVALID_BINDING;
    WorldAudioEffects* audio = impact->audio;
    if (!*audio->enabled) return WORLD_AUDIO_OK;
    if (!audio->queue || !audio->randomState || !position ||
        impact->nextIndex < 0 || impact->nextIndex >= 3 ||
        audio->queue->spatialEnabled != audio->enabled) return WORLD_AUDIO_INVALID_BINDING;
    impact->nextIndex = (impact->nextIndex + 1) % 3;
    // PAL1E5538..1E555C reads the same bSample table populated by
    // LoadSampleTable1E4958..1E4990, after the InGame bank is available.
    const WorldAudioVoiceRef voice = impact->woodSamples[impact->nextIndex];
    const float pitchScale = vectorRandomFloat(audio->randomState, 0.6f) + 0.5f;
    const WorldAudioResult valid = worldAudioValidateVoice(audio->queue, voice);
    if (valid != WORLD_AUDIO_OK) return valid;
    (void)worldAudioPlayCueAtPointScaled(audio->queue, voice.instance->bank,
        voice.instance->cue, position, 2.5f, pitchScale, 0, 1);
    return audio->queue->result;
}

WorldAudioResult worldAudioPlayBomb(WorldAudioEffects* effects, const float position[4])
{
    if (!effects || !effects->enabled) return WORLD_AUDIO_INVALID_BINDING;
    if (!*effects->enabled) return WORLD_AUDIO_OK;
    auto* queue=effects->queue;
    if (!queue || !effects->randomState || !position || queue->spatialEnabled!=effects->enabled ||
        !queue->positionalSamples) return WORLD_AUDIO_INVALID_BINDING;
    for (int voice=0;voice<2;++voice) {
        const float pitch=vectorRandomFloat(effects->randomState,0.6000000238418579f)+0.5f;
        WorldAudioResult result;
        auto* sample=queue->positionalSamples->Play("Bomb",position,1.0f,pitch,&result);
        if (!sample) return result==WORLD_AUDIO_OK ? WORLD_AUDIO_INVALID_BINDING : result;
        auto* envelope=sample->Envelope();
        envelope->volume=6.0f;
        envelope->inverseDistance=0.0033333334140479565f;
        envelope->maximumDistance=300.0f;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult collisionAudioPlayFoliage(WorldImpactAudio* owner, const float position[4])
{
    if (!owner || !owner->audio || !owner->audio->enabled) return WORLD_AUDIO_INVALID_BINDING;
    if (!*owner->audio->enabled) return WORLD_AUDIO_OK;
    auto* queue=owner->audio->queue;
    if (!queue || !owner->audio->randomState || !queue->positionalSamples || !position)
        return WORLD_AUDIO_INVALID_BINDING;
    owner->foliageIndex=(owner->foliageIndex+1)%3;
    const auto voice=owner->foliage[owner->foliageIndex];
    const float pitch=vectorRandomFloat(owner->audio->randomState,0.6000000238418579f)+0.5f;
    if (!voice.instance) return WORLD_AUDIO_OK;
    WorldAudioResult result=worldAudioValidateVoice(queue,voice);
    if (result!=WORLD_AUDIO_OK) return result;
    queue->positionalSamples->Play(voice.instance->bank,voice.instance->cue,position,
        1.899999976158142f,pitch,&result);
    return result;
}

WorldAudioResult collisionAudioPlayGlass(WorldImpactAudio* owner, const float position[4])
{
    if (!owner || !owner->audio || !owner->audio->enabled) return WORLD_AUDIO_INVALID_BINDING;
    if (!*owner->audio->enabled) return WORLD_AUDIO_OK;
    auto* queue=owner->audio->queue;
    if (!queue || !queue->positionalSamples || !position) return WORLD_AUDIO_INVALID_BINDING;
    const auto voice=owner->glass;
    if (!voice.instance) return WORLD_AUDIO_OK;
    WorldAudioResult result=worldAudioValidateVoice(queue,voice);
    if (result!=WORLD_AUDIO_OK) return result;
    queue->positionalSamples->Play(voice.instance->bank,voice.instance->cue,position,
        1.899999976158142f,1.0f,&result);
    return result;
}

WorldAudioResult collisionAudioPlaySmokeable(WorldImpactAudio* owner, const float position[4],
    const CollisionBreakableDefinition* definition, uint32_t modelKey)
{
    if (!owner || !owner->audio || !owner->audio->enabled) return WORLD_AUDIO_INVALID_BINDING;
    if (!*owner->audio->enabled) return WORLD_AUDIO_OK;
    WorldAudioResult result=definition && definition->materialIndex==28 ?
        collisionAudioPlayFoliage(owner,position) : worldImpactAudioPlay(owner,position);
    if (result!=WORLD_AUDIO_OK || !definition) return result;
    if (modelKey!=owner->petrolPumpKey && modelKey!=owner->gasPumpKey) return WORLD_AUDIO_OK;
    result=worldAudioPlayBomb(owner->audio,position);
    if (result!=WORLD_AUDIO_OK) return result;
    auto* queue=owner->audio->queue;
    if (!queue || !queue->positionalSamples) return WORLD_AUDIO_INVALID_BINDING;
    queue->positionalSamples->Play("PreviewZone",position,1.899999976158142f,1.0f,&result);
    return result;
}

namespace {
WorldAudioRequestQueue* audioQueue(const WorldImpactAudio* owner)
{
    return owner && owner->audio ? owner->audio->queue : nullptr;
}

bool boundContactAudio(const WorldImpactAudio* owner)
{
    return audioQueue(owner) && owner->audio->enabled && owner->audio->randomState &&
        owner->collisions && owner->spatial && owner->parameters && owner->registered &&
        owner->collisions->contactAudio == owner;
}

float pointDistance(const float first[4], const float second[4])
{
    const float x=first[0]-second[0], y=first[1]-second[1], z=first[2]-second[2];
    return std::sqrt((x*x+y*y)+z*z);
}

void unlinkDetached(WorldImpactAudio* owner, CollisionAudioVoice* voice)
{
    if (!voice->detached) return;
    if (voice->previous) voice->previous->next=voice->next;
    else owner->firstDetached=voice->next;
    if (voice->next) voice->next->previous=voice->previous;
    voice->next=voice->previous=nullptr;
    voice->detached=0;
}

void detachVoice(WorldImpactAudio* owner, CollisionAudioVoice* voice)
{
    voice->previous=nullptr;
    voice->next=owner->firstDetached;
    if (voice->next) voice->next->previous=voice;
    owner->firstDetached=voice;
    voice->detached=1;
}

WorldAudioResult destroyVoice(WorldImpactAudio* owner, CollisionAudioVoice* voice)
{
    WorldAudioResult result=worldAudioRetireVoice(audioQueue(owner),voice->sample);
    if (result!=WORLD_AUDIO_OK) return result;
    unlinkDetached(owner,voice);
    *voice={};
    return WORLD_AUDIO_OK;
}

WorldAudioResult copySample(WorldImpactAudio* owner, WorldAudioVoiceRef source,
    CollisionAudioVoice** destination)
{
    *destination=nullptr;
    const auto assign=[&](CollisionAudioVoice& voice) {
        const auto result=worldAudioCopyVoice(audioQueue(owner),source,&voice.sample);
        if (result!=WORLD_AUDIO_OK) return result;
        voice.occupied=1;
        *destination=&voice;
        return WORLD_AUDIO_OK;
    };
    for (auto& voice:owner->voices)
        if (!voice.occupied) return assign(voice);
    auto** next=&owner->voiceBlocks;
    while (*next) {
        for (auto& voice:(*next)->voices)
            if (!voice.occupied) return assign(voice);
        next=&(*next)->next;
    }
    // PAL Malloc expands by the initial count's quarter plus one. Own the
    // unpublished block until the actual bSample copy also succeeds, so an
    // allocation/sample failure cannot publish half-initialized storage.
    std::unique_ptr<CollisionAudioVoiceBlock> block(new(std::nothrow) CollisionAudioVoiceBlock{});
    if (!block) return audioQueue(owner)->result=WORLD_AUDIO_CAPACITY;
    const auto result=assign(block->voices[0]);
    if (result!=WORLD_AUDIO_OK) return result;
    *next=block.release();
    return WORLD_AUDIO_OK;
}

WorldAudioVoiceRef collisionSampleType(WorldImpactAudio* owner, const CollisionImpact* impact)
{
    const int32_t material=collisionPairMaterialIndex(impact->pair);
    if (material>=15 && material<17) {
        owner->nextIndex=(owner->nextIndex+1)%3;
        return owner->woodSamples[owner->nextIndex];
    }
    int32_t strength=convertFloatToInt32((impact->accumulatedMagnitude*0.03333333507180214f)*3.0f);
    if (strength<0) strength=0;
    if (strength>2) strength=2;
    int32_t& index=owner->metalIndex[strength];
    index=(index+1)%3;
    return owner->metal[strength][index];
}

bool oldEnough(const WorldImpactAudio* owner, const CollisionImpact* impact)
{
    // Unsigned subtraction retains the source's wrapping timer arithmetic.
    const int32_t ticks=static_cast<int32_t>(static_cast<uint32_t>(owner->collisions->nowTicks)-
        static_cast<uint32_t>(impact->createdTicks));
    return static_cast<float>(ticks)*0.0002500000118743628f>=0.10000000149011612f;
}

WorldAudioResult loadBankSample(WorldImpactAudio* owner, const char* name, WorldAudioVoiceRef* sample)
{
    auto* queue=audioQueue(owner);
    WorldAudioResult result=worldAudioAcquireInGameSample(queue,name,sample);
    return result==WORLD_AUDIO_OK ? worldAudioSetVoiceCategory(queue,*sample,0) : result;
}
}

WorldAudioResult collisionAudioRegister(WorldImpactAudio* owner, CollisionSystem* collisions,
    const Sound3dBindings* spatial, const RaceParameters* parameters)
{
    if (!audioQueue(owner) || !collisions || !spatial || !parameters ||
        (owner->collisions && owner->collisions!=collisions) ||
        (collisions->contactAudio && collisions->contactAudio!=owner)) return WORLD_AUDIO_INVALID_BINDING;
    owner->collisions=collisions;
    owner->spatial=spatial;
    owner->parameters=parameters;
    owner->updateLoaded=0;
    owner->registered=1;
    collisions->contactAudio=owner;
    if (!owner->petrolPumpKey) {
        owner->petrolPumpKey=hashString("CS_PetrolPump");
        owner->gasPumpKey=hashString("CS_GasPump");
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult collisionAudioLoadSampleTable(WorldImpactAudio* owner)
{
    if (!boundContactAudio(owner)) return WORLD_AUDIO_INVALID_BINDING;
    static const char* const names[3][3]={
        {"CrashMetalSmall01","CrashMetalSmall02","CrashMetalSmall03"},
        {"CrashMetalMedium01","CrashMetalMedium02","CrashMetalMedium03"},
        {"CrashMetalHard01","CrashMetalHard02","CrashMetalHard03"}};
    static const char* const wood[3]={"CrashWood01","CrashWood02","CrashWood03"};
    static const char* const foliage[3]={"CrashFoilage01","CrashFoilage02","CrashFoilage03"};
    for (int strength=0;strength<3;++strength) {
        owner->metalIndex[strength]=0;
        for (int index=0;index<3;++index) {
            WorldAudioResult result=loadBankSample(owner,names[strength][index],&owner->metal[strength][index]);
            if (result!=WORLD_AUDIO_OK) return result;
        }
    }
    owner->nextIndex=0;
    for (int index=0;index<3;++index) {
        WorldAudioResult result=loadBankSample(owner,wood[index],&owner->woodSamples[index]);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    owner->foliageIndex=0;
    for (int index=0;index<3;++index) {
        WorldAudioResult result=loadBankSample(owner,foliage[index],&owner->foliage[index]);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    WorldAudioResult result=loadBankSample(owner,"CrashMetalScrape",&owner->scrape[1]);
    if (result!=WORLD_AUDIO_OK) return result;
    result=loadBankSample(owner,"CrashWoodScrape",&owner->scrape[0]);
    if (result!=WORLD_AUDIO_OK) return result;
    result=loadBankSample(owner,"CrashGlass01",&owner->glass);
    if (result==WORLD_AUDIO_OK) owner->tableLoaded=1;
    return result;
}

WorldAudioResult collisionAudioGetDistanceToCamera(const WorldImpactAudio* owner,
    const float position[4], float* distance)
{
    if (!owner || !owner->spatial || !owner->spatial->cameras || !owner->parameters ||
        !position || !distance) return WORLD_AUDIO_INVALID_BINDING;
    const auto& screens=owner->spatial->cameras->screens;
    if (!screens[1].view) return WORLD_AUDIO_INVALID_BINDING;
    *distance=pointDistance(position,screens[1].view->current.eye);
    if (owner->parameters->mode>=2) {
        if (!screens[2].view) return WORLD_AUDIO_INVALID_BINDING;
        const float other=pointDistance(position,screens[2].view->current.eye);
        if (other<*distance) *distance=other;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult collisionAudioCalcVolume(const WorldImpactAudio* owner,
    const CollisionImpact* impact, float* volume)
{
    if (!impact || !volume) return WORLD_AUDIO_INVALID_BINDING;
    float distance;
    WorldAudioResult result=collisionAudioGetDistanceToCamera(owner,impact->firstPoint,&distance);
    if (result!=WORLD_AUDIO_OK) return result;
    *volume=0.0f;
    if (distance<35.0f) {
        float strength=(impact->accumulatedMagnitude*0.03333333507180214f)*0.6499999761581421f;
        if (strength>1.0f) strength=1.0f;
        float attenuation=35.0f/distance;
        if (attenuation>1.0f) attenuation=1.0f;
        *volume=strength*attenuation;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult collisionAudioCalcScrapeVolume(const WorldImpactAudio* owner,
    const CollisionImpact* impact, float* volume)
{
    WorldAudioResult result=collisionAudioCalcVolume(owner,impact,volume);
    if (result!=WORLD_AUDIO_OK) return result;
    *volume=*volume*(impact->tangentialSpeed*0.1666666716337204f);
    if (*volume>1.0f) *volume=1.0f;
    return WORLD_AUDIO_OK;
}

WorldAudioResult collisionAudioCreateSample(WorldImpactAudio* owner, CollisionImpact* impact)
{
    if (!owner || !owner->audio || !owner->audio->enabled || !owner->collisions ||
        owner->collisions->contactAudio!=owner || !impact || !impact->pair || impact->normalSound)
        return WORLD_AUDIO_INVALID_BINDING;
    // PAL1E5278 tests the live collision timer and selects the template before
    // touching a registered sample or spatial owner. Audio-disabled startup
    // leaves the actual template table empty (SoundLoadRegions1AE7DC), so
    // PAL1E52DC returns no sample. Keep that path without claiming readiness.
    if (!oldEnough(owner,impact)) return WORLD_AUDIO_OK;
    const auto sample=collisionSampleType(owner,impact);
    if (!sample.instance) return WORLD_AUDIO_OK;
    if (!boundContactAudio(owner)) return WORLD_AUDIO_INVALID_BINDING;
    float volume;
    WorldAudioResult result=collisionAudioCalcVolume(owner,impact,&volume);
    if (result!=WORLD_AUDIO_OK || !(0.20000000298023224f<volume)) return result;
    result=copySample(owner,sample,&impact->normalSound);
    if (result!=WORLD_AUDIO_OK || !*owner->audio->enabled) return result;
    auto* queue=audioQueue(owner);
    const float pitch=vectorRandomFloat(owner->audio->randomState,0.6000000238418579f)+0.5f;
    result=worldAudioPlayVoice(queue,impact->normalSound->sample,volume*1.899999976158142f,0.0f,pitch-1.0f);
    if (result!=WORLD_AUDIO_OK) return result;
    uint16_t azimuth;
    if (sound3dComputeAzimuth(owner->spatial,impact->point,&azimuth)!=SOUND3D_OK)
        return WORLD_AUDIO_INVALID_BINDING;
    return worldAudioSetVoiceControlCode(queue,impact->normalSound->sample,azimuth);
}

WorldAudioResult collisionAudioPlayScrape(WorldImpactAudio* owner, CollisionImpact* impact)
{
    if (!owner || !owner->audio || !owner->audio->enabled) return WORLD_AUDIO_INVALID_BINDING;
    // PAL1E57D0 returns on disabled audio before reading the contact or SDK
    // sample owners; sound-load deliberately did not register them.
    if (!*owner->audio->enabled) return WORLD_AUDIO_OK;
    if (!boundContactAudio(owner) || !impact || !impact->pair) return WORLD_AUDIO_INVALID_BINDING;
    if (impact->scrapeSound || !oldEnough(owner,impact)) return WORLD_AUDIO_OK;
    const int32_t material=collisionPairMaterialIndex(impact->pair);
    if (material<15 || material>17) return WORLD_AUDIO_OK;
    const auto sample=owner->scrape[material==17 ? 1 : 0];
    if (!sample.instance) return WORLD_AUDIO_INVALID_BINDING;
    WorldAudioResult result=copySample(owner,sample,&impact->scrapeSound);
    if (result!=WORLD_AUDIO_OK) return result;
    float volume;
    result=collisionAudioCalcScrapeVolume(owner,impact,&volume);
    if (result!=WORLD_AUDIO_OK) return result;
    uint16_t azimuth;
    if (sound3dComputeAzimuth(owner->spatial,impact->point,&azimuth)!=SOUND3D_OK)
        return WORLD_AUDIO_INVALID_BINDING;
    auto* queue=audioQueue(owner);
    result=worldAudioSetVoiceControlCode(queue,impact->scrapeSound->sample,azimuth);
    if (result!=WORLD_AUDIO_OK) return result;
    result=worldAudioPlayVoice(queue,impact->scrapeSound->sample,volume*1.899999976158142f,0.0f,0.0f);
    if (result!=WORLD_AUDIO_OK) return result;
    const float pitch=vectorRandomFloat(owner->audio->randomState,0.36000001430511475f)-0.18000000715255737f;
    return worldAudioSetVoicePitchOffset(queue,impact->scrapeSound->sample,pitch);
}

WorldAudioResult collisionAudioUpdateSample(WorldImpactAudio* owner, CollisionImpact* impact)
{
    if (!boundContactAudio(owner) || !impact) return WORLD_AUDIO_INVALID_BINDING;
    if (!impact->scrapeSound) return WORLD_AUDIO_OK;
    float spatialVolume, collisionVolume;
    if (sound3dComputeVolume(owner->spatial,impact->point,200.0f,&spatialVolume)!=SOUND3D_OK)
        return WORLD_AUDIO_INVALID_BINDING;
    WorldAudioResult result=collisionAudioCalcScrapeVolume(owner,impact,&collisionVolume);
    if (result!=WORLD_AUDIO_OK) return result;
    uint16_t azimuth;
    if (sound3dComputeAzimuth(owner->spatial,impact->point,&azimuth)!=SOUND3D_OK)
        return WORLD_AUDIO_INVALID_BINDING;
    auto* queue=audioQueue(owner);
    result=worldAudioSetVoiceVolume(queue,impact->scrapeSound->sample,
        (spatialVolume*collisionVolume)*1.899999976158142f);
    return result==WORLD_AUDIO_OK ? worldAudioSetVoiceControlCode(queue,impact->scrapeSound->sample,azimuth) : result;
}

WorldAudioResult collisionAudioReleaseSample(WorldImpactAudio* owner, CollisionImpact* impact)
{
    if (!boundContactAudio(owner) || !impact) return WORLD_AUDIO_INVALID_BINDING;
    auto* queue=audioQueue(owner);
    if (impact->normalSound) {
        auto* voice=impact->normalSound;
        WorldAudioResult result=worldAudioValidateVoice(queue,voice->sample);
        if (result!=WORLD_AUDIO_OK) return result;
        if (voice->sample.instance->playing) detachVoice(owner,voice);
        else {
            result=destroyVoice(owner,voice);
            if (result!=WORLD_AUDIO_OK) return result;
        }
        impact->normalSound=nullptr;
    }
    if (impact->scrapeSound) {
        auto* voice=impact->scrapeSound;
        WorldAudioResult result=worldAudioValidateVoice(queue,voice->sample);
        if (result!=WORLD_AUDIO_OK) return result;
        if (voice->sample.instance->playing) {
            result=worldAudioFadeVoiceVolume(queue,voice->sample,0.0f,50);
            if (result!=WORLD_AUDIO_OK) return result;
            detachVoice(owner,voice);
        }
        // Source retains an inactive scrape wrapper until pool teardown.
        impact->scrapeSound=nullptr;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult collisionAudioUpdate(WorldImpactAudio* owner)
{
    if (!boundContactAudio(owner)) return WORLD_AUDIO_INVALID_BINDING;
    if (!owner->collisions->world) return WORLD_AUDIO_OK;
    // IsReady1AF2E8 reads this same sound-loader/global audio gate.
    if (!owner->updateLoaded && worldAudioRuntime.disabled==0) {
        owner->updateLoaded=1;
        WorldAudioResult result=collisionAudioLoadSampleTable(owner);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    for (auto* voice=owner->firstDetached;voice;) {
        auto* next=voice->next;
        float volume;
        WorldAudioResult result=worldAudioGetVoiceVolume(audioQueue(owner),voice->sample,&volume);
        if (result!=WORLD_AUDIO_OK) return result;
        if (!voice->sample.instance->playing || volume<=0.05000000074505806f) {
            result=destroyVoice(owner,voice);
            if (result!=WORLD_AUDIO_OK) return result;
        }
        voice=next;
    }
    for (auto* impact=owner->collisions->firstImpact;impact;impact=impact->next) {
        WorldAudioResult result=collisionAudioUpdateSample(owner,impact);
        if (result!=WORLD_AUDIO_OK) return result;
        if (!impact->normalSound) {
            result=collisionAudioCreateSample(owner,impact);
            if (result!=WORLD_AUDIO_OK) return result;
        }
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult collisionAudioPause(WorldImpactAudio* owner, int32_t paused)
{
    if (!boundContactAudio(owner)) return WORLD_AUDIO_INVALID_BINDING;
    auto* queue=audioQueue(owner);
    for (auto voice:owner->scrape) {
        if (!voice.instance) continue;
        WorldAudioResult result=worldAudioPauseVoice(queue,voice,paused);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    for (auto* impact=owner->collisions->firstImpact;impact;impact=impact->next) {
        for (auto* voice:{impact->normalSound,impact->scrapeSound}) {
            if (!voice) continue;
            WorldAudioResult result=worldAudioPauseVoice(queue,voice->sample,paused);
            if (result!=WORLD_AUDIO_OK) return result;
        }
    }
    for (auto* voice=owner->firstDetached;voice;voice=voice->next) {
        WorldAudioResult result=worldAudioPauseVoice(queue,voice->sample,paused);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    for (int index=0;index<3;++index) {
        for (auto voice:{owner->woodSamples[index],owner->foliage[index]}) {
            if (!voice.instance || !voice.instance->playing) continue;
            WorldAudioResult result=worldAudioPauseVoice(queue,voice,paused);
            if (result!=WORLD_AUDIO_OK) return result;
        }
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult collisionAudioReset(WorldImpactAudio* owner)
{
    if (!audioQueue(owner)) return WORLD_AUDIO_INVALID_BINDING;
    while (owner->firstDetached) {
        WorldAudioResult result=destroyVoice(owner,owner->firstDetached);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult collisionAudioUnregister(WorldImpactAudio* owner)
{
    if (!boundContactAudio(owner)) return WORLD_AUDIO_INVALID_BINDING;
    for (auto* impact=owner->collisions->firstImpact;impact;impact=impact->next)
        if (impact->normalSound || impact->scrapeSound) return WORLD_AUDIO_INVALID_BINDING;
    owner->updateLoaded=0;
    WorldAudioResult result=collisionAudioReset(owner);
    if (result!=WORLD_AUDIO_OK) return result;
    for (auto& voice:owner->voices) {
        if (!voice.occupied) continue;
        result=destroyVoice(owner,&voice);
        if (result!=WORLD_AUDIO_OK) return result;
    }
    for (auto* block=owner->voiceBlocks;block;block=block->next) {
        for (auto& voice:block->voices) {
            if (!voice.occupied) continue;
            result=destroyVoice(owner,&voice);
            if (result!=WORLD_AUDIO_OK) return result;
        }
    }
    // A failed STOP above leaves every block and owner binding available for
    // the next drain/retry. Successful retirement transfers pending payload
    // lifetime to the actual sample queue; it never borrows these wrappers.
    while (owner->voiceBlocks) {
        auto* block=owner->voiceBlocks;
        owner->voiceBlocks=block->next;
        delete block;
    }
    owner->collisions->contactAudio=nullptr;
    owner->collisions=nullptr;
    owner->registered=0;
    return WORLD_AUDIO_OK;
}
