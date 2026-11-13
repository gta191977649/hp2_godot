#include "bsound/include/bSample.h"
#include "bsound/include/bSound.h"
#include "indep/src/speech/SEDWedge.hpp"
#include "bsound/include/bJukebox.h"
#include "bware/inc/bMath.hpp"
#include "indep/src/MoviePlayer/MV3Codec.h"

#include <cmath>

namespace {

WorldAudioResult sampleVolume(WorldAudioRequestQueue* queue,WorldAudioVoiceRef voice,
    float gain,uint16_t& output)
{
    float scaled=0.0f;
    const auto result=worldAudioCalcVoiceScaledVolume(queue,voice,&scaled);
    if (result!=WORLD_AUDIO_OK) return result;
    const float volume=gain*scaled;
    const float raw=volume*16383.0f;
    if (!std::isfinite(raw) || raw<-2147483648.0f || 2147483648.0f<=raw)
        return WORLD_AUDIO_INVALID_BINDING;
    output=static_cast<uint16_t>(convertFloatToInt32(raw));
    return WORLD_AUDIO_OK;
}

WorldAudioResult publishSampleVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,float volume)
{
    uint16_t raw=0;
    const auto calculated=sampleVolume(queue,voice,volume,raw);
    if (calculated!=WORLD_AUDIO_OK)return calculated;
    const auto submitted=worldAudioSubmitSampleVolume(queue,voice,raw);
    if (submitted!=WORLD_AUDIO_OK)return submitted;
    // Publication is transactional: a full transport queue cannot change
    // the logical gain or consume native fade time without a device command.
    voice.instance->volume=volume;
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult primeCreatedVoice(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef acquired,WorldAudioVoiceRef* output)
{
    const auto primed=worldAudioPrimeVoice(queue,acquired);
    if (primed!=WORLD_AUDIO_OK) {
        // Prime commits neither a reservation nor a request on failure.
        // Retire the unpublished logical slot while its caller still owns
        // the bank/cue/stream; a rejected constructor must not retain them.
        const auto retired=worldAudioRetireVoice(queue,acquired);
        return queue->result=retired==WORLD_AUDIO_OK ? primed : retired;
    }
    *output=acquired;
    return queue->result=WORLD_AUDIO_OK;
}

} // namespace

WorldAudioResult worldAudioResolveVoice(WorldAudioRequestQueue* queue,WorldAudioVoiceRef voice)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK)return valid;
    auto& sample=*voice.instance;
    if (sample.sampleType || !queue->library)return WORLD_AUDIO_OK;
    if (sample.bank && sample.cue && queue->library->OwnsLoadedBank(sample.bank))
        return WORLD_AUDIO_OK;
    if (sample.bankSample || !sample.bankName[0] || !sample.sampleName[0])
        return queue->result=WORLD_AUDIO_INVALID_BINDING;
    const auto* region=queue->library->Region(sample.bankName);
    if (!region)return queue->result=WORLD_AUDIO_INVALID_BINDING;
    const auto* cue=worldAudioFindInBank(&region->Bank(),sample.sampleName);
    if (!cue)return queue->result=WORLD_AUDIO_INVALID_BINDING;
    sample.bank=&region->Bank();sample.cue=cue;
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioCalcVoiceScaledVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,float* volume)
{
    if (!volume || !queue || !queue->library) return WORLD_AUDIO_INVALID_BINDING;
    const auto valid=worldAudioResolveVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    const auto& sample=*voice.instance;
    uint16_t authored=0x3FFF;
    if (!sample.sampleType) {
        if (!sample.cue->metadata || sample.cue->metadataSize!=52)
            return WORLD_AUDIO_INVALID_BINDING;
        authored=uint16_t(sample.cue->metadata[0x24]) |
            uint16_t(sample.cue->metadata[0x25])<<8;
    }
    // PAL273D0C..1C: keep this precise binary32 operation order. The value
    // 00028038 is the actual release GP-66C8 literal, not a double division.
    const float category=queue->library->GetCategoryVolume(sample.category);
    const float normalized=category*0.0000610388815402984619140625f;
    const float authoredVolume=static_cast<float>(authored)*normalized;
    *volume=authoredVolume*sample.volumeScale;
    return WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioSetVoiceScaleVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,float scale)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    voice.instance->volumeScale=scale;
    if (!voice.instance->hasHandle) return WORLD_AUDIO_OK;
    float volume=0.0f;
    const auto scaled=worldAudioCalcVoiceScaledVolume(queue,voice,&volume);
    if (scaled!=WORLD_AUDIO_OK) return scaled;
    const float raw=volume*16383.0f;
    if (!std::isfinite(raw) || raw<-2147483648.0f || 2147483648.0f<=raw)
        return WORLD_AUDIO_INVALID_BINDING;
    return worldAudioSubmitSampleVolume(queue,voice,
        static_cast<uint16_t>(convertFloatToInt32(raw)));
}

WorldAudioResult worldAudioSetVoiceVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,float volume)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    if (!voice.instance->hasHandle) return WORLD_AUDIO_OK;
    // PAL273D50 stores requested volume only when a voice handle exists.
    // A direct gain change does not cancel the original fade slope. Its
    // later progress starts from this gain at the native command time.
    auto& sample=*voice.instance;
    uint64_t now=0;
    if (sample.fadeStep!=0.0f) {
        const auto clock=worldAudioReadNativeClock(queue,&now);
        if (clock!=WORLD_AUDIO_OK)return clock;
        if (now<sample.fadeLastNanoseconds)return queue->result=WORLD_AUDIO_INVALID_BINDING;
    }
    const auto result=publishSampleVolume(queue,voice,volume);
    if (result!=WORLD_AUDIO_OK)return result;
    if (sample.fadeStep!=0.0f) {
        sample.fadeOriginVolume=volume;
        sample.fadeOriginNanoseconds=sample.fadeLastNanoseconds=now;
        sample.fadeDurationNanoseconds=0;
    }
    return queue->result=WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioUnmuteVoiceAtCurrentVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK)return valid;
    auto& sample=*voice.instance;
    if (!sample.hasHandle || !sample.muted)
        return worldAudioMuteVoice(queue,voice,0);
    uint16_t raw=0;
    const auto calculated=sampleVolume(queue,voice,sample.volume,raw);
    if (calculated!=WORLD_AUDIO_OK)return queue->result=calculated;
    // The native menu suspends this retained cursor, but its Audio screen
    // may update the category while it is muted. PAL Mute restores its old
    // saved gain; follow that operation with the current category gain.
    // Preflight the complete owner-thread transaction before changing mute
    // intent. This refresh does not advance or re-anchor a sample fade.
    if (worldAudioAvailableRequests(queue)<2 || queue->nextSequence>UINT64_MAX-2 ||
        sample.requestReferences>SIZE_MAX-2)
        return queue->result=WORLD_AUDIO_CAPACITY;
    const auto unmuted=worldAudioMuteVoice(queue,voice,0);
    if (unmuted!=WORLD_AUDIO_OK)return unmuted;
    return worldAudioSubmitSampleVolume(queue,voice,raw);
}

// PAL274A98..274C10 supplies the slope, strict target clamp and readiness
// callback. The native adaptation evaluates the same linear slope against
// elapsed audio time, instead of making its duration depend on device polls.
// The actual backend still supplies readiness; no completion is fabricated.
WorldAudioResult worldAudioUpdateSample(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,int32_t backendPrebuffered)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK)return valid;
    auto& sample=*voice.instance;
    if (!sample.hasHandle || sample.fadeStep==0.0f)return WORLD_AUDIO_OK;
    uint64_t now=0;
    const auto clock=worldAudioReadNativeClock(queue,&now);
    if (clock!=WORLD_AUDIO_OK)return clock;
    if (now<sample.fadeLastNanoseconds || now<sample.fadeOriginNanoseconds)
        return queue->result=WORLD_AUDIO_INVALID_BINDING;
    if (now!=sample.fadeLastNanoseconds) {
        // Fade274934 multiplies milliseconds by this exact binary32
        // nominal update rate. Repeated evaluation from one anchor avoids
        // roundoff drift at different native service/presentation rates.
        const double updates=static_cast<double>(now-sample.fadeOriginNanoseconds)*
            (static_cast<double>(0.059988003224134445f)/1000000.0);
        float volume=static_cast<float>(static_cast<double>(sample.fadeOriginVolume)+
            static_cast<double>(sample.fadeStep)*updates);
        float step=sample.fadeStep;
        if ((step<0.0f && volume<sample.fadeTarget) ||
            (!(step<0.0f) && sample.fadeTarget<volume)) {
            volume=sample.fadeTarget;
            step=0.0f;
        }
        // The source binary32 division can leave a tiny remainder at the
        // exact requested millisecond endpoint. Publish that authored
        // target exactly; keep the original strict-crossing slope state.
        // Explicit gain/PLAY reanchors clear this marker and retain slope.
        if (sample.fadeDurationNanoseconds &&
            now-sample.fadeOriginNanoseconds>=sample.fadeDurationNanoseconds)
            volume=sample.fadeTarget;
        const auto submitted=publishSampleVolume(queue,voice,volume);
        if (submitted!=WORLD_AUDIO_OK)return submitted;
        sample.fadeStep=step;
        sample.fadeLastNanoseconds=now;
    }
    const bool callback=sample.completionKind!=WORLD_AUDIO_COMPLETION_NONE ||
        (sample.sampleType==3 && sample.streamSource && sample.streamSource->movie);
    if (!sample.hasHandle || sample.prebuffered || !callback || !backendPrebuffered)
        return WORLD_AUDIO_OK;
    if (sample.sampleType==1)return worldAudioSpeechEvent(queue,voice,sample.playSequence,4);
    if (sample.sampleType==3)return worldAudioStreamEvent(queue,voice,sample.playSequence,4);
    return WORLD_AUDIO_INVALID_BINDING;
}

WorldAudioResult worldAudioCreateStreamVoice(WorldAudioRequestQueue* queue,
    WorldAudioStreamSource* source,WorldAudioVoiceRef* voice)
{
    if (!queue || !queue->library || !source || !voice || voice->instance ||
        (!source->file.Valid() && !source->movie) || source->path.empty()) return WORLD_AUDIO_INVALID_BINDING;
    if (worldAudioAvailableRequests(queue)<1) return WORLD_AUDIO_CAPACITY;
    source->bank.name="_STREAMS";
    source->cue.name=source->path.c_str();
    source->bank.cues=&source->cue;
    source->bank.cueCount=1;
    WorldAudioVoiceRef pending{};
    const auto acquired=worldAudioAcquireVoice(queue,&source->bank,&source->cue,-1,0,&pending);
    if (acquired!=WORLD_AUDIO_OK) return acquired;
    pending.instance->sampleType=3; // Actual bSampleStream(2) constructs base type3.
    pending.instance->streamSource=source;
    // PAL280444 locks the physical voice during stream construction. The
    // device allocation is queued; no prebuffered/playing state is invented.
    return primeCreatedVoice(queue,pending,voice);
}

WorldAudioResult worldAudioOpenStream(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,WorldAudioStreamSource* source,uint64_t offset,uint64_t size)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    if (!source || voice.instance->sampleType!=3 || voice.instance->streamSource!=source ||
        voice.instance->bank!=&source->bank ||
        (source->movie ? (offset!=0 || size!=0x7fffffffu) :
         (offset>source->file.Size() || size>source->file.Size()-offset || !source->sampleRate)))
        return WORLD_AUDIO_INVALID_BINDING;
    return worldAudioSubmitStreamOpen(queue,voice,source,offset,size);
}

WorldAudioResult worldAudioSetVoiceCategory(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,uint32_t category)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    // The whole released bSample.SetCategory273618 only writes sample+84.
    // There is no physical-device command or second category owner.
    voice.instance->category=category;
    return WORLD_AUDIO_OK;
}

WorldAudioResult worldAudioStreamEvent(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,uint64_t sequence,uint32_t event)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK) return valid;
    auto& sample=*voice.instance;
    if (sample.sampleType==3 && sample.streamSource && sample.streamSource->movie) {
        if (sample.playSequence!=sequence)return WORLD_AUDIO_STALE_REFERENCE;
        if (event==8)return worldAudioCompleteVoice(queue,voice,sequence);
        if (event!=4)return WORLD_AUDIO_INVALID_BINDING;
        if (sample.prebuffered || sample.playing)return WORLD_AUDIO_STALE_REFERENCE;
        sample.prebuffered=1;
        return worldAudioPlayVoice(queue,voice,1.0f,0.0f,0.0f);
    }
    if (sample.sampleType!=3 || sample.completionKind!=WORLD_AUDIO_COMPLETION_JUKEBOX ||
        !sample.completionJukebox || sample.playSequence!=sequence)
        return WORLD_AUDIO_STALE_REFERENCE;
    if (event==8) return worldAudioCompleteVoice(queue,voice,sequence);
    if (event==4) {
        if (sample.prebuffered || sample.playing) return WORLD_AUDIO_STALE_REFERENCE;
        sample.prebuffered=1;
    } else if (event==32) {
        if (sample.completionJukebox->state!=5) return WORLD_AUDIO_STALE_REFERENCE;
    } else if (event!=64 && event!=128) {
        return WORLD_AUDIO_INVALID_BINDING;
    }
    return sample.completionJukebox->SampleCallback(event,voice);
}

std::int32_t worldAudioReadMovieStream(const WorldAudioStreamSource& source,
    void* output,std::uint32_t bytes)
{
    if (!source.movie)return 0; // A retired provider supplies no decoded bytes.
    return source.movie->GetAudio(output,0,bytes);
}

WorldAudioResult worldAudioCreateSpeechVoice(WorldAudioRequestQueue* queue,
    WorldStreamAudio* owner,const WorldAudioBank* bank,const WorldAudioCue* cue,
    WorldAudioVoiceRef* voice)
{
    if (!queue || !queue->library || queue->library->GetStatus()!=1 || !owner ||
        owner->audio!=queue || !voice || voice->instance || !bank || !cue)
        return WORLD_AUDIO_INVALID_BINDING;
    if (worldAudioAvailableRequests(queue)<1)return WORLD_AUDIO_CAPACITY;
    WorldAudioVoiceRef pending{};
    const auto acquired=worldAudioAcquireVoice(queue,bank,cue,-1,0,&pending);
    if (acquired!=WORLD_AUDIO_OK)return acquired;
    pending.instance->sampleType=1; // PAL205CC4 constructs bSample(1).
    // The real PlaySentence installs the SED callback later. Initialize
    // only constructs/primes this sample; early dispatch changes Cleanup.
    // PAL205CF8 calls real bSample::Prime273388. Allocation is an ordered
    // command on the existing device queue, not a fabricated ready handle.
    return primeCreatedVoice(queue,pending,voice);
}

WorldAudioResult worldAudioSpeechEvent(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,uint64_t sequence,uint32_t event)
{
    const auto valid=worldAudioValidateVoice(queue,voice);
    if (valid!=WORLD_AUDIO_OK)return valid;
    auto& sample=*voice.instance;
    if (sample.sampleType!=1 || sample.completionKind!=WORLD_AUDIO_COMPLETION_SPEECH ||
        !sample.completionSpeech || sample.playSequence!=sequence || sequence==UINT64_MAX)
        return WORLD_AUDIO_STALE_REFERENCE;
    if (event==8)return worldAudioCompleteVoice(queue,voice,sequence);
    if (event==4) {
        if (sample.prebuffered || sample.playing)return WORLD_AUDIO_STALE_REFERENCE;
        // Do not consume readiness if the actual callback cannot queue its
        // Play/SetVolume operations. The backend can deliver it again.
        sample.prebuffered=1; // supplied by this actual ready event, not OPEN
        const auto result=worldStreamAudioSampleCallback(sample.completionSpeech,event,voice);
        if (result!=WORLD_AUDIO_OK)sample.prebuffered=0;
        return result;
    }
    return worldStreamAudioSampleCallback(sample.completionSpeech,event,voice);
}

WorldAudioResult worldAudioCalcVoicePitch(WorldAudioRequestQueue* queue,WorldAudioVoiceRef voice,float pitch,uint32_t* value)
{
    if (!value || !std::isfinite(pitch) || pitch<-2147483648.0f || pitch>=2147483648.0f)
        return WORLD_AUDIO_INVALID_BINDING;
    const auto valid=worldAudioResolveVoice(queue,voice);if (valid!=WORLD_AUDIO_OK) return valid;
    const auto& sample=*voice.instance;
    uint32_t base=0;
    if (!sample.sampleType) {
        if (!sample.cue->metadata || sample.cue->metadataSize!=52) return WORLD_AUDIO_INVALID_BINDING;
        base=uint16_t(sample.cue->metadata[0x22])|(uint16_t(sample.cue->metadata[0x23])<<8);
    } else {
        if (!sample.streamSource) return WORLD_AUDIO_INVALID_BINDING;
        base=sample.streamSource->sampleRate;
    }
    int32_t integer=static_cast<int8_t>(static_cast<uint8_t>(convertFloatToInt32(pitch)));
    if (std::abs(integer)>=7) {
        if (pitch>7.0f) pitch=7.0f;
        if (pitch<-7.0f) pitch=-7.0f;
        integer=static_cast<int8_t>(static_cast<uint8_t>(convertFloatToInt32(pitch)));
    }
    // Actual final PAL327A80 table, exactly258 halfwords; its SHA is in
    // audio_chain49_paths.json. No generated pow approximation or beta data.
    static constexpr uint16_t fine[258]={
        1024,1035,1046,1057,1069,1080,1092,1104,1116,1128,1141,1153,1166,1178,1191,1204,
        1217,1231,1244,1257,1271,1285,1299,1313,1327,1342,1357,1371,1386,1401,1417,1432,
        1448,1463,1479,1495,1512,1528,1545,1562,1579,1596,1613,1631,1649,1667,1685,1703,
        1722,1740,1759,1779,1798,1817,1837,1857,1878,1898,1919,1940,1961,1982,2004,2025,
        2048,2070,2092,2115,2138,2161,2185,2209,2233,2257,2282,2307,2332,2357,2383,2409,
        2435,2462,2488,2515,2543,2571,2599,2627,2655,2684,2714,2743,2773,2803,2834,2865,
        2896,2927,2959,2991,3024,3057,3090,3124,3158,3192,3227,3262,3298,3334,3370,3407,
        3444,3481,3519,3558,3596,3635,3675,3715,3756,3796,3838,3880,3922,3965,4008,4051,
        4096,4140,4185,4231,4277,4323,4371,4418,4466,4515,4564,4614,4664,4715,4766,4818,
        4870,4924,4977,5031,5086,5142,5198,5254,5311,5369,5428,5487,5547,5607,5668,5730,
        5792,5855,5919,5983,6049,6114,6181,6248,6316,6385,6455,6525,6596,6668,6741,6814,
        6888,6963,7039,7116,7193,7271,7351,7431,7512,7593,7676,7760,7844,7930,8016,8103,
        8192,8281,8371,8462,8554,8647,8742,8837,8933,9030,9129,9228,9328,9430,9533,9637,
        9741,9848,9955,10063,10173,10284,10396,10509,10623,10739,10856,10974,11094,11214,11336,11460,
        11585,11711,11838,11967,12098,12229,12363,12497,12633,12771,12910,13051,13193,13336,13482,13628,
        13777,13927,14078,14232,14387,14543,14702,14862,15024,15187,15353,15520,15689,15860,16032,16207,
        16384,0,
    };
    static constexpr float coarse[16]={0.00390625f,0.0078125f,0.015625f,0.03125f,0.0625f,0.125f,0.25f,0.5f,1.0f,2.0f,4.0f,8.0f,16.0f,32.0f,64.0f,128.0f};
    float multiplier=1.0f;
    if (pitch>=-2.0f && pitch<=2.0f) {
        const float shifted=pitch+2.0f;
        const int32_t index=convertFloatToInt32(shifted*64.0f)+1;
        if (index<0 || index>=258) return WORLD_AUDIO_INVALID_BINDING;
        multiplier=static_cast<float>(fine[index])*0.000244140625f;
    } else {
        if (integer<-7 || integer>7) return WORLD_AUDIO_INVALID_BINDING;
        const float fraction=pitch-static_cast<float>(integer);
        const float delta=coarse[integer>0?integer+8:integer+7]*fraction;
        multiplier=coarse[integer+8]+delta;
    }
    const float scaled=static_cast<float>(base)*multiplier;
    if (!std::isfinite(scaled) || scaled<-2147483648.0f || scaled>=2147483648.0f)
        return WORLD_AUDIO_INVALID_BINDING;
    *value=static_cast<uint32_t>(convertFloatToInt32(scaled));return WORLD_AUDIO_OK;
}
WorldAudioResult worldAudioCalcVoicePan(float pan,int8_t* value)
{
    const float raw=pan*255.0f;
    if (!value || !std::isfinite(raw) || raw<-2147483648.0f || raw>=2147483648.0f)
        return WORLD_AUDIO_INVALID_BINDING;
    *value=static_cast<int8_t>(static_cast<uint8_t>(convertFloatToInt32(raw)));return WORLD_AUDIO_OK;
}
WorldAudioResult worldAudioCalcVoiceVolume(WorldAudioRequestQueue* queue,WorldAudioVoiceRef voice,float volume,uint16_t* value)
{
    if (!value) return WORLD_AUDIO_INVALID_BINDING;
    return sampleVolume(queue,voice,volume,*value);
}
WorldAudioResult worldAudioPlayFile(WorldAudioRequestQueue* queue,WorldAudioVoiceRef voice,
    WorldAudioStreamSource* source,const char* filename,uint32_t offset,uint32_t bytes)
{
    const auto valid=worldAudioValidateVoice(queue,voice);if (valid!=WORLD_AUDIO_OK) return valid;
    if (!source || !filename || source->path!=filename || voice.instance->sampleType!=1 ||
        voice.instance->streamSource!=source || !voice.instance->hasHandle) return WORLD_AUDIO_INVALID_BINDING;
    uint32_t pitch=0;uint16_t volume=0;
    auto result=worldAudioCalcVoicePitch(queue,voice,0.0f,&pitch);if (result!=WORLD_AUDIO_OK) return result;
    result=worldAudioCalcVoiceVolume(queue,voice,voice.instance->volume,&volume);if (result!=WORLD_AUDIO_OK) return result;
    result=worldAudioSubmitChainPart(queue,voice,source,offset,bytes,1,pitch,volume,0);
    if (result!=WORLD_AUDIO_OK) return result;
    // Final PAL273FF8 sets the filename/range, then Play resets pan/pitch.
    // The SDK's actual asynchronous ready event is kept distinct from PLAY.
    source->cue.name=source->path.c_str();voice.instance->pan=0;voice.instance->pitchOffset=0;
    return WORLD_AUDIO_OK;
}
