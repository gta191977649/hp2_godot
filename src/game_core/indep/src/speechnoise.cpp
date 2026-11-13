#include "indep/inc/SpeechNoise.hpp"
#include "bware/inc/bMath.hpp"

namespace {
const char* const sampleNames[9]={
    "RadioNoiseSquelch01","RadioNoiseSquelch02",
    "RadioNoise01","RadioNoise02","RadioNoise03","RadioNoise04",
    "RadioNoise05","RadioNoise06","RadioNoise07"
};
}

SpeechNoise::SpeechNoise() = default;

WorldAudioResult SpeechNoise::Bind(WorldAudioRequestQueue* audio,WorldAudioRuntimeState* runtime)
{
    if(!audio || !runtime || (audio_ && (audio_!=audio || runtime_!=runtime)))
        return WORLD_AUDIO_INVALID_BINDING;
    audio_=audio;
    runtime_=runtime;
    return WORLD_AUDIO_OK;
}

WorldAudioResult SpeechNoise::UnbindAfterAudioShutdown(WorldAudioRequestQueue& audio)
{
    if(audio_!=&audio || audio.library || audio.banks || audio.bankCount ||
        audio.inGameBank || audio.instances || audio.instanceCapacity ||
        audio.spatialEnabled || audio.count || audio.retainedCount)
        return WORLD_AUDIO_INVALID_BINDING;
    // Bank samples are borrowed. Their real audio owner has already
    // retired them; do not enqueue new STOPs or dereference expired voices.
    helicopter_={};samples_={};
    radioNoise_=0;loadRequested_=0;muted_=0;
    audio_=nullptr;runtime_=nullptr;
    return WORLD_AUDIO_OK;
}

WorldAudioResult SpeechNoise::BindHelicopter(WorldAudioVoiceRef voice)
{
    if(!audio_)return WORLD_AUDIO_INVALID_BINDING;
    const auto result=worldAudioValidateVoice(audio_,voice);
    if(result!=WORLD_AUDIO_OK)return result;
    helicopter_=voice;
    return WORLD_AUDIO_OK;
}

WorldAudioResult SpeechNoise::LoadSamples()
{
    if(!audio_ || !audio_->instances)return WORLD_AUDIO_INVALID_BINDING;
    // The source performs its own InGame-bank lookup on each load. The
    // shared native cache need not have been primed by another sound caller.
    if(!worldAudioFindInGame(audio_,sampleNames[0]))return WORLD_AUDIO_INVALID_BINDING;
    std::array<const WorldAudioCue*,9> cues{};
    std::array<WorldAudioVoiceRef,9> voices{};
    size_t required=0;
    for(size_t index=0;index<cues.size();++index){
        cues[index]=worldAudioFindInBank(audio_->inGameBank,sampleNames[index]);
        if(!cues[index])return WORLD_AUDIO_INVALID_BINDING;
        for(size_t slot=0;slot<audio_->instanceCapacity;++slot){
            auto* stored=worldAudioInstanceAt(audio_,slot);
            if(!stored)return WORLD_AUDIO_INVALID_BINDING;
            auto& candidate=*stored;
            if(candidate.occupied && candidate.bankSample && !candidate.retiring &&
                candidate.bank==audio_->inGameBank && candidate.cue==cues[index]){
                voices[index]=worldAudioVoiceReference(&candidate);
                break;
            }
        }
        if(!voices[index].instance)++required;
    }
    const auto capacity=worldAudioEnsureFreeInstances(audio_,required);
    if(capacity!=WORLD_AUDIO_OK)return capacity;
    for(size_t index=0;index<samples_.size();++index){
        if(!voices[index].instance){
            const auto result=worldAudioAcquireInGameSample(audio_,sampleNames[index],&voices[index]);
            if(result!=WORLD_AUDIO_OK)return result;
        }
        // PAL215610..215770 only borrows these nine region samples and
        // assigns category6. They obtain a channel on actual PLAY and
        // release it on completion; unlike the SED stream/player engine,
        // this owner never locks nine idle physical voices with Prime.
        const auto result=worldAudioSetVoiceCategory(audio_,voices[index],6);
        if(result!=WORLD_AUDIO_OK)return result;
        samples_[index]=voices[index];
    }
    loadRequested_=0;
    return WORLD_AUDIO_OK;
}

void SpeechNoise::RequestSamples()
{
    loadRequested_=1;
}

WorldAudioResult SpeechNoise::UnLoadSamples()
{
    const auto result=StopHelicopterNoise();
    if(result!=WORLD_AUDIO_OK)return result;
    StopRadioNoise();
    helicopter_={};
    // PAL1AF448 calls this before retiring the actual InGame region at
    // 1AF484. These nine references borrow that region's master samples;
    // unlike the old raw addresses, native generation references cannot
    // outlive their owner. End those borrows at the same unload boundary.
    // SED Cleanup may still publish its original event8/state32 while
    // frontend files load. Keep that callback and Update's RNG/null branch;
    // the next RequestSamples/LoadSamples performs nine fresh bank lookups.
    samples_={};
    return WORLD_AUDIO_OK;
}

WorldAudioResult SpeechNoise::Update()
{
    if(!runtime_)return WORLD_AUDIO_INVALID_BINDING;
    if(loadRequested_ && !runtime_->disabled){
        const auto result=LoadSamples();
        if(result!=WORLD_AUDIO_OK)return result;
    }
    if(runtime_->disabled || muted_ || !radioNoise_)return WORLD_AUDIO_OK;
    if(vectorRandomFloat(&runtime_->randomState,1.0f)<0.029999999329447746f){
        const float gain=vectorRandomFloat(&runtime_->randomState,0.40000000596046448f)+0.60000002384185791f;
        const auto index=vectorRandomBounded(7,&runtime_->randomState);
        const auto voice=samples_[2+index];
        if(!voice.instance)return WORLD_AUDIO_OK; // Actual final null-sample gate.
        return worldAudioPlayVoice(audio_,voice,gain,0.0f,1.0f);
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult SpeechNoise::Mute(bool muted)
{
    muted_=muted;
    for(size_t index=2;index<samples_.size();++index){
        const auto voice=samples_[index];
        if(!voice.instance)continue;
        const auto result=worldAudioPauseVoice(audio_,voice,muted);
        if(result!=WORLD_AUDIO_OK)return result;
    }
    return helicopter_.instance ? worldAudioPauseVoice(audio_,helicopter_,muted) : WORLD_AUDIO_OK;
}

WorldAudioResult SpeechNoise::StartHelicopterNoise()
{
    if(!helicopter_.instance)return WORLD_AUDIO_OK;
    return worldAudioPlayVoice(audio_,helicopter_,1.0f,0.0f,0.0f);
}

WorldAudioResult SpeechNoise::SamplePlaying(WorldAudioVoiceRef voice,int32_t& playing) const
{
    const auto result=worldAudioValidateVoice(audio_,voice);
    if(result!=WORLD_AUDIO_OK)return result;
    playing=voice.instance->playing!=0;
    return WORLD_AUDIO_OK;
}

WorldAudioResult SpeechNoise::StopHelicopterNoise()
{
    if(!helicopter_.instance)return WORLD_AUDIO_OK;
    int32_t playing=0;
    const auto result=SamplePlaying(helicopter_,playing);
    if(result!=WORLD_AUDIO_OK)return result;
    return playing ? worldAudioStopInstance(audio_,helicopter_.instance) : WORLD_AUDIO_OK;
}

void SpeechNoise::StartRadioNoise()
{
    radioNoise_=1;
}

void SpeechNoise::StopRadioNoise()
{
    radioNoise_=0;
}

int32_t SpeechNoise::IsRadioNoise() const
{
    return radioNoise_;
}

WorldAudioResult SpeechNoise::BeginSquelchNoise()
{
    if(!runtime_)return WORLD_AUDIO_INVALID_BINDING;
    const auto index=vectorRandomBounded(2,&runtime_->randomState);
    return worldAudioPlayVoice(audio_,samples_[index],1.0f,0.0f,0.0f);
}

int32_t SpeechNoise::IsSquelchNoisePlaying() const
{
    int32_t playing=0;
    if(SamplePlaying(samples_[0],playing)!=WORLD_AUDIO_OK)return -1;
    if(playing)return 1;
    if(SamplePlaying(samples_[1],playing)!=WORLD_AUDIO_OK)return -1;
    return playing;
}
