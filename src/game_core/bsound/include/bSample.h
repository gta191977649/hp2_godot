#ifndef NFSHP2_WORLD_AUDIO_H
#define NFSHP2_WORLD_AUDIO_H

#include <stddef.h>
#include <stdint.h>

struct Sound3dSample;
struct Sound3dSampleRegistry;
struct bSound;
struct cJukeboxPlayer;
struct WorldAudioStreamSource;
struct WorldStreamAudio;

/* Native owner-thread elapsed-time provider. This is not a game/REC or
 * presentation clock. A controlled PCM host may use consumed output frames. */
#ifdef __cplusplus
typedef uint64_t (*WorldAudioNativeClockRead)(void* owner) noexcept;
#else
typedef uint64_t (*WorldAudioNativeClockRead)(void* owner);
#endif

typedef enum WorldAudioCompletionKind {
    WORLD_AUDIO_COMPLETION_NONE,
    WORLD_AUDIO_COMPLETION_POSITIONAL,
    WORLD_AUDIO_COMPLETION_JUKEBOX,
    WORLD_AUDIO_COMPLETION_SPEECH
} WorldAudioCompletionKind;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WorldAudioCue {
    const char* name;
    uint32_t resourceKey;
    /* Borrowed exact authored MSH record. The bSound bank owner retains it
     * through every private sample and unacknowledged backend request. */
    const uint8_t* metadata;
    size_t metadataSize;
} WorldAudioCue;

/* Separate source sound globals. AudioDisabled and the sound RNG are not
 * the audio-system enable flag or the world's default random generator. */
typedef struct WorldAudioRuntimeState {
    int32_t disabled;
    uint32_t randomState;
    /* PAL330F24, initialized1 and written only after the SDK SetAudioMode. */
    int32_t audioMode;
} WorldAudioRuntimeState;
extern WorldAudioRuntimeState worldAudioRuntime;

typedef struct WorldAudioBank {
    const char* name;
    const WorldAudioCue* cues;
    size_t cueCount;
    const uint8_t* metadata;
    size_t metadataSize;
    const uint8_t* encodedSamples;
    size_t encodedSampleSize;
} WorldAudioBank;

typedef struct WorldAudioInstance {
    const WorldAudioBank* bank;
    const WorldAudioCue* cue;
    /* PAL bSample owns both names (+38/+58); a private source sample may
     * outlive its currently loaded region without retaining obsolete PCM. */
    char bankName[33];
    char sampleName[33];
    int32_t tag;
    int32_t autoRelease;
    int32_t occupied;
    int32_t playing;
    float volumeScale;
    float pitchScale;
    float volume;
    float pitchOffset;
    float pan;
    uint16_t controlCode;
    uint64_t generation;
    uint64_t playSequence;
    size_t requestReferences;
    /* 1: STOP submitted, waiting for retained requests. 2: self-retained
     * movie STOP waits for an actual ACK to free a transport slot. */
    int32_t retiring;
    int32_t paused;
    uint32_t category;
    /* A bank sample is the SDK's shared mutable template. Safe-sample
     * copies are private instances even when their immutable cue is equal. */
    int32_t bankSample;
    float fadeTarget;
    float fadeStep;
    /* The authored binary32 slope is evaluated against one native elapsed
     * origin; repeated device polls cannot change this curve or its duration. */
    float fadeOriginVolume;
    uint64_t fadeOriginNanoseconds;
    uint64_t fadeLastNanoseconds;
    /* Nonzero only while the original authored duration still owns the
     * anchor. Direct SetVolume/PLAY preserve slope but replace that anchor. */
    uint64_t fadeDurationNanoseconds;
    /* SDK event 8 releases this exact positional owner. Generation and
     * play-sequence validation precede dispatch; no generic callback. */
    union {
        struct Sound3dSample* completionSample;
        struct cJukeboxPlayer* completionJukebox;
        struct WorldStreamAudio* completionSpeech;
    };
    /* SDK bSample+8C and+0C: a primed idle handle is not playing, and a
     * normal playing sample need not be explicitly primed. */
    int32_t primed;
    int32_t hasHandle;
    /* Original source channel lock, owned by this same sample handle.
     * hasHandle gates its lifetime; it is not a device-ready observation.
     * Queued operations capture these values before later STOP/reuse. */
    uint16_t reservedVoiceId;
    uint16_t reservedVoiceChannels;
    /* Actual bSample+7C mute state is distinct from pause+80. The backend
     * retains its hardware gain token while the logical volume stays live. */
    int32_t muted;
    uint16_t savedMuteVolume;
    WorldAudioCompletionKind completionKind;
    uint8_t sampleType;
    int32_t prebuffered;
    const struct WorldAudioStreamSource* streamSource;
    uint64_t streamOffset;
    uint64_t streamSize;
} WorldAudioInstance;

typedef struct WorldAudioVoiceRef {
    WorldAudioInstance* instance;
    uint64_t generation;
} WorldAudioVoiceRef;

/* Borrowed from the one live audio manager; no unprovided channel is read. */
typedef struct WorldAudioManagerState {
    const WorldAudioBank* frontendBank;
    const float* channelVolumes;
    size_t channelVolumeCount;
    /* Optional write capability to this exact same live mixer array. A
     * control operation requires pointer identity, never a copied snapshot. */
    float* channelControls;
} WorldAudioManagerState;

typedef enum WorldAudioDeliveryState {
    WORLD_AUDIO_REQUEST_FREE,
    WORLD_AUDIO_REQUEST_QUEUED,
    WORLD_AUDIO_REQUEST_INFLIGHT,
    WORLD_AUDIO_REQUEST_ACKNOWLEDGED,
    WORLD_AUDIO_REQUEST_FAILED
} WorldAudioDeliveryState;

typedef enum WorldAudioRequestKind {
    WORLD_AUDIO_PLAY = 0,
    WORLD_AUDIO_STOP = 1,
    WORLD_AUDIO_SET_CONTROL_CODE,
    WORLD_AUDIO_SET_PITCH_OFFSET,
    WORLD_AUDIO_SET_VOLUME,
    WORLD_AUDIO_SET_CHANNEL_VOLUME,
    WORLD_AUDIO_SET_PAUSED,
    WORLD_AUDIO_SET_CATEGORY,
    WORLD_AUDIO_FADE_VOLUME,
    WORLD_AUDIO_SET_AUDIO_MODE,
    WORLD_AUDIO_PRIME,
    WORLD_AUDIO_SET_EFFECT_MIX,
    WORLD_AUDIO_SET_EFFECT_PRESET,
    WORLD_AUDIO_SET_EFFECT_PARAMETER,
    WORLD_AUDIO_SET_MUTED,
    WORLD_AUDIO_UNLOAD_BANK,
    WORLD_AUDIO_SET_REGION_PATH,
    WORLD_AUDIO_LOAD_REGION,
    WORLD_AUDIO_DRIVER_UPDATE,
    WORLD_AUDIO_JUKEBOX_INCLUDE_TRACK,
    WORLD_AUDIO_JUKEBOX_EXCLUDE_TRACK,
    WORLD_AUDIO_SET_CORE_EFFECT_VOLUME,
    WORLD_AUDIO_SET_CORE_EFFECT_PAN,
    WORLD_AUDIO_INITIALIZE_DEVICE,
    WORLD_AUDIO_OPEN_STREAM,
    WORLD_AUDIO_SET_SAMPLE_VOLUME,
    WORLD_AUDIO_APPEND_CHAIN,
    WORLD_AUDIO_OPEN_CHAIN,
    WORLD_AUDIO_SET_CORE_EFFECT_ATTRIBUTE,
    WORLD_AUDIO_SET_CORE_EFFECT_ENABLED
} WorldAudioRequestKind;

typedef struct WorldAudioRequest {
    const WorldAudioBank* bank;
    const WorldAudioCue* cue;
    float gain;
    float pan;
    float variation;
    uint64_t sequence;
    WorldAudioInstance* instance;
    WorldAudioRequestKind kind;
    uint16_t controlCode;
    float parameterValue;
    uint64_t queueGeneration;
    uint64_t instanceGeneration;
    size_t slot;
    WorldAudioDeliveryState deliveryState;
    int32_t channel;
    /* Category captured when this operation is submitted, on the same
     * retained voice; later category changes cannot retarget an old PLAY. */
    uint32_t category;
    uint32_t durationMilliseconds;
    int32_t audioMode;
    /* Ordered SDK effect submissions. These are retained command arguments,
     * not a second live DSP state or evidence of hardware application. */
    int32_t effectMask;
    int32_t effectRoute;
    int32_t effectPreset;
    int32_t effectParameter;
    int32_t effectSlot;
    float effectPan;
    int32_t muted;
    /* Actual retained SDK call arguments, not resource-readiness mirrors. */
    char regionName[33];
    char resourcePath[64];
    int32_t jukeboxGroup;
    int32_t jukeboxTrack;
    int32_t effectCore;
    uint16_t effectCoreVolume;
    int8_t effectCorePan;
    const struct WorldAudioStreamSource* streamSource;
    uint64_t streamOffset;
    uint64_t streamSize;
    uint16_t sampleVolume;
    uint32_t samplePitch;
    int8_t samplePan;
    int32_t deviceUpdateCompleted;
    int32_t releaseDeviceVoice;
    /* Source LockChannels result captured at publication, not looked up
     * from a sample whose later commands may already reserve another id. */
    uint16_t reservedVoiceId;
    uint16_t reservedVoiceChannels;
    /* Read-only native delivery telemetry; never a readiness clock. */
} WorldAudioRequest;

typedef enum WorldAudioResult {
    WORLD_AUDIO_OK,
    WORLD_AUDIO_INVALID_BINDING,
    WORLD_AUDIO_CAPACITY,
    WORLD_AUDIO_STALE_REFERENCE,
    WORLD_AUDIO_RESOURCE_FAILURE,
    WORLD_AUDIO_BACKEND_FAILURE
} WorldAudioResult;

struct GameSession;

/* One owner thread publishes, takes and acknowledges requests. Take borrows
 * a request; Acknowledge ends that borrow after backend submission. The ring
 * and its borrowed resources must outlive every unacknowledged request.
 * Reinitialize/rebind only after draining requests and retiring live voices. */
typedef struct WorldAudioRequestQueue {
    const WorldAudioBank* banks;
    size_t bankCount;
    const WorldAudioBank* inGameBank;
    WorldAudioRequest* requests;
    size_t capacity;
    size_t readIndex;
    size_t count;
    uint64_t nextSequence;
    WorldAudioResult result;
    /* First storage block only. instanceCapacity is the total slot count;
     * all indexed access must use worldAudioInstanceAt. Standalone queues
     * without a library keep their explicitly bound fixed-span capacity. */
    WorldAudioInstance* instances;
    size_t instanceCapacity;
    const int32_t* spatialEnabled;
    uint64_t generation;
    size_t retireIndex;
    size_t retainedCount;
    struct Sound3dSampleRegistry* positionalSamples;
    /* Actual SDK CPU owner. banks/instances/mixer borrow that same owner;
     * standalone queues cannot manufacture a loaded library region. */
    struct bSound* library;
    /* bEffect's original comparison/initialization cells. This records
     * submissions; it never claims that a device has applied the effect. */
    int32_t effectConfigured;
    uint16_t effectComparedType;
    /* A failed/cancelled device operation is terminal transport ownership,
     * never successful playback. Preserve its identity after ring reuse. */
    uint64_t lastFailedSequence;
    WorldAudioRequestKind lastFailedKind;
    WorldAudioNativeClockRead nativeClockRead;
    void* nativeClockOwner;
    /* Provider epochs are arbitrary, including zero. Rebinding a completed
     * host to steady_clock preserves elapsed time through this translation. */
    uint64_t nativeClockOrigin;
    uint64_t nativeClockOffset;
} WorldAudioRequestQueue;

/* These native registry helpers retain pointer identity across original
 * SlotPool expansion. They do not allocate/complete a physical voice. */
WorldAudioInstance* worldAudioInstanceAt(const WorldAudioRequestQueue* queue, size_t index);
WorldAudioResult worldAudioEnsureFreeInstances(WorldAudioRequestQueue* queue, size_t count);
WorldAudioResult worldAudioResolveVoice(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice);

void worldAudioInitialize(WorldAudioRequestQueue* queue,
                          const WorldAudioBank* banks, size_t bankCount,
                          WorldAudioRequest* requests, size_t capacity);
/* A custom provider may replace the clock only with no live sample fade.
 * Passing both arguments null restores the native steady clock, preserving
 * elapsed time and all sample anchors even during terminal device closure. */
WorldAudioResult worldAudioBindNativeClock(WorldAudioRequestQueue* queue,
    void* owner, WorldAudioNativeClockRead read);
WorldAudioResult worldAudioReadNativeClock(WorldAudioRequestQueue* queue,
    uint64_t* nanoseconds);
const WorldAudioCue* worldAudioFindInGame(WorldAudioRequestQueue* queue, const char* name);
const WorldAudioCue* worldAudioFindInBank(const WorldAudioBank* bank, const char* name);
WorldAudioResult worldAudioPlayCue(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue,
    float gain, float pan, float variation);
/* Native SDK boundary for the live mixer. The CPU value changes before the
 * caller's refresh reads it; the physical backend command uses the same
 * ordered queue as other audio work. This is not a port of the SDK mixer. */
WorldAudioResult worldAudioSetChannelVolume(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager, int32_t channel, float value);
/* Ordered SDK bank release after the real bRegion has retired its samples.
 * The immutable bank metadata outlives acknowledgement of this request. */
WorldAudioResult worldAudioUnloadBank(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank,const char* name);
WorldAudioResult worldAudioSetRegionPath(WorldAudioRequestQueue* queue,const char* path);
WorldAudioResult worldAudioLoadRegion(WorldAudioRequestQueue* queue,const char* name);
WorldAudioResult worldAudioUpdateDriver(WorldAudioRequestQueue* queue);
/* Physical-device transports called by the actual bSound CPU owner. They
 * capture arguments only. Readiness is a separate checked completion. */
WorldAudioResult worldAudioSubmitRegionLoad(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank,const char* name);
WorldAudioResult worldAudioSubmitRegionPath(WorldAudioRequestQueue* queue,const char* path);
WorldAudioResult worldAudioSubmitDeviceInitialization(WorldAudioRequestQueue* queue);
WorldAudioResult worldAudioSubmitChannelVolume(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager,int32_t channel,float value);
WorldAudioResult worldAudioSetJukeboxTrackIncluded(WorldAudioRequestQueue* queue,
    int32_t group,int32_t track,int32_t included);
WorldAudioResult worldAudioSetMode(WorldAudioRequestQueue* queue, int32_t mode);
WorldAudioResult worldAudioSetEffectMix(WorldAudioRequestQueue* queue,
    int32_t mask, int32_t route, float mix, float pan);
WorldAudioResult worldAudioSetCoreEffectVolume(WorldAudioRequestQueue* queue,
    int32_t core,uint16_t volume);
WorldAudioResult worldAudioSetCoreEffectPan(WorldAudioRequestQueue* queue,
    int32_t core,int8_t pan);
WorldAudioResult worldAudioSetEffectPreset(WorldAudioRequestQueue* queue, int32_t preset);
WorldAudioResult worldAudioSubmitEffectControl(WorldAudioRequestQueue* queue,
    int32_t core,int32_t attribute,int32_t value);
int32_t worldAudioGetMode(void);
WorldAudioResult worldAudioPlayInGame(WorldAudioRequestQueue* queue, const char* name,
                                      float gain, float pan, float variation);
/* A named bank voice is retained between plays. Its live playing state is
 * shared by all callers and completion uses the existing checked voice API. */
WorldAudioResult worldAudioPlayInGameIfIdle(WorldAudioRequestQueue* queue,
    const char* name, float gain, float pan, float variation);
WorldAudioResult worldAudioPlayScore(WorldAudioRequestQueue* queue,
    const struct GameSession* session, int32_t score);
int worldAudioTake(WorldAudioRequestQueue* queue, WorldAudioRequest* request);
WorldAudioResult worldAudioAcknowledge(WorldAudioRequestQueue* queue,
    const WorldAudioRequest* request);
WorldAudioResult worldAudioReject(WorldAudioRequestQueue* queue,
    const WorldAudioRequest* request);
size_t worldAudioAvailableRequests(const WorldAudioRequestQueue* queue);
void worldAudioBindInstances(WorldAudioRequestQueue* queue, WorldAudioInstance* instances,
                             size_t capacity, const int32_t* enabled);
WorldAudioInstance* worldAudioPlayAtPoint(WorldAudioRequestQueue* queue, const char* name,
                                         const float position[4], float initialParameter,
                                         int32_t tag, int32_t autoRelease);
WorldAudioInstance* worldAudioPlayAtPointScaled(WorldAudioRequestQueue* queue, const char* name,
                                         const float position[4], float initialParameter, float pitchScale,
                                         int32_t tag, int32_t autoRelease);
WorldAudioInstance* worldAudioPlayCueAtPointScaled(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue, const float position[4],
    float initialParameter, float pitchScale, int32_t tag, int32_t autoRelease);
/* Positional FinishLine cue used after the road-block/cinematic countdown.
 * The historical source method name does not select a different cue. */
WorldAudioResult worldAudioPlayBustRoadBlock(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager, const float position[4]);
WorldAudioVoiceRef worldAudioVoiceReference(const WorldAudioInstance* instance);
WorldAudioResult worldAudioValidateVoice(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice);
WorldAudioResult worldAudioAcquireVoice(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue, int32_t tag,
    int32_t autoRelease, WorldAudioVoiceRef* voice);
/* Borrow the one bank-owned sample from the same instance pool. Borrowers
 * must not retire it on release; the bank/audio owner ends its lifetime. */
WorldAudioResult worldAudioAcquireBankSample(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue, WorldAudioVoiceRef* voice);
WorldAudioResult worldAudioAcquireInGameSample(WorldAudioRequestQueue* queue,
    const char* name, WorldAudioVoiceRef* voice);
/* SDK Initialize(sample) copies only its immutable name/bank binding.
 * Destination category, gain, pause and playback state are not copied. A
 * null destination creates a fresh private sample with ctor defaults. */
WorldAudioResult worldAudioInitializeVoiceFromCue(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue, WorldAudioVoiceRef* destination);
WorldAudioResult worldAudioCopyVoice(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef source, WorldAudioVoiceRef* destination);
WorldAudioResult worldAudioPlayVoice(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice,
    float gain, float pan, float variation);
WorldAudioResult worldAudioPrimeVoice(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice);
WorldAudioResult worldAudioRetireVoice(WorldAudioRequestQueue* queue, WorldAudioVoiceRef voice);
/* Completion identifies the PLAY that finished, even if this retained voice
 * has subsequently been played again. */
WorldAudioResult worldAudioCompleteVoice(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, uint64_t playSequence);
WorldAudioResult worldAudioSetVoiceParameters(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, uint16_t controlCode, float pitchOffset, float volume);
/* Single SDK operations retain their own order and capacity transaction.
 * Pause preserves the retained voice and its playback cursor. */
WorldAudioResult worldAudioSetVoiceControlCode(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,uint16_t controlCode);
WorldAudioResult worldAudioSetVoiceVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, float volume);
WorldAudioResult worldAudioSetVoicePitchOffset(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, float pitchOffset);
WorldAudioResult worldAudioSetVoiceCategory(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, uint32_t category);
/* Source Fade multiplies its factor by the actual authored/category scale.
 * Native elapsed audio time advances that same slope independently of how
 * many driver updates or presentation frames the host processes. */
WorldAudioResult worldAudioFadeVoiceVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, float target, uint32_t milliseconds);
WorldAudioResult worldAudioUpdateSample(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,int32_t backendPrebuffered);
WorldAudioResult worldAudioGetVoiceVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, float* volume);
WorldAudioResult worldAudioPauseVoice(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,int32_t paused);
WorldAudioResult worldAudioMuteVoice(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice, int32_t muted);
/* A native menu retains the playback cursor while category settings may
 * change. Resume restores the current resolved gain after the original mute
 * operation; both device requests are admitted before either is published. */
WorldAudioResult worldAudioUnmuteVoiceAtCurrentVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice);
WorldAudioResult worldAudioSetVoiceScaleVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,float scale);
WorldAudioResult worldAudioCalcVoiceScaledVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,float* volume);
WorldAudioResult worldAudioSubmitSampleVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,uint16_t volume);
WorldAudioResult worldAudioSubmitStreamOpen(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,const struct WorldAudioStreamSource* source,uint64_t offset,uint64_t size);
WorldAudioResult worldAudioCalcVoicePitch(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,float pitch,uint32_t* value);
WorldAudioResult worldAudioCalcVoicePan(float pan,int8_t* value);
WorldAudioResult worldAudioCalcVoiceVolume(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,float volume,uint16_t* value);
WorldAudioResult worldAudioSubmitChainPart(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,const struct WorldAudioStreamSource* source,
    uint32_t offset,uint32_t bytes,int32_t last,uint32_t pitch,uint16_t volume,int8_t pan);
WorldAudioResult worldAudioPlayFile(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,struct WorldAudioStreamSource* source,const char* filename,
    uint32_t offset,uint32_t bytes);
/* Named bSampleStream lifecycle. The JBC owner supplies an actual retained
 * file range. A submitted OPEN is pending until the real backend emits4. */
WorldAudioResult worldAudioCreateStreamVoice(WorldAudioRequestQueue* queue,
    struct WorldAudioStreamSource* source,WorldAudioVoiceRef* voice);
WorldAudioResult worldAudioOpenStream(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,struct WorldAudioStreamSource* source,uint64_t offset,uint64_t size);
/* The released SED sample is base type1. It borrows the same instance pool
 * and device queue as positional/JBC/movie audio. Event4 is a real backend
 * ready notification; event8 is checked against its exact play sequence. */
WorldAudioResult worldAudioCreateSpeechVoice(WorldAudioRequestQueue* queue,
    struct WorldStreamAudio* owner,const WorldAudioBank* bank,const WorldAudioCue* cue,
    WorldAudioVoiceRef* voice);
WorldAudioResult worldAudioSpeechEvent(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,uint64_t sequence,uint32_t event);
WorldAudioResult worldAudioStreamEvent(WorldAudioRequestQueue* queue,
    WorldAudioVoiceRef voice,uint64_t sequence,uint32_t event);
WorldAudioResult worldAudioPlayInstance(WorldAudioRequestQueue* queue, WorldAudioInstance* instance,
                                         float gain, float pan, float variation);
WorldAudioResult worldAudioStopInstance(WorldAudioRequestQueue* queue, WorldAudioInstance* instance);
/* The three ordered parameter operations address the retained voice. The
 * queue must accept all three before its live parameter state is changed. */
WorldAudioResult worldAudioSetInstanceParameters(WorldAudioRequestQueue* queue,
    WorldAudioInstance* instance, uint16_t controlCode, float pitchOffset, float volume);

#ifdef __cplusplus
}

#include "bware/inc/bFile.hpp"
#include <string>
#include <array>
namespace hp2 { class MoviePlayer; }

/* Five complete bSegment bodies have individual PAL identity/path proof
 * (speech_segments46_paths_accepted_r22.json). Native members express the
 * observed file range; they are not a layout copied from the beta type. */
class bSegment final {
public:
    explicit bSegment(const bSound& sound);
    void Initialize(const char* filename,uint32_t offset,uint32_t bytes,uint32_t flags);
    void Cleanup();
    uint32_t GetOffset() const noexcept;
    uint32_t GetSize() const noexcept;
private:
    std::array<char,256> filename_{};
    bool initialized_{};
    uint32_t offset_{};
    uint32_t bytes_{};
    uint32_t cursor_{};
    uint32_t flags_{};
};

/* Native lifetime of one actual stream source. The range retains the same
 * opened JBC file; its bank/cue are the source's special _STREAMS sample
 * binding, not another loaded region or a copied PCM catalogue. */
struct WorldAudioStreamSource final {
    hp2::FileRange file;
    std::string path;
    uint32_t sampleRate{};
    WorldAudioBank bank{};
    WorldAudioCue cue{};
    // Mutually exclusive source: a retained cartridge file, or the actual
    // MoviePlayer ring callback. Closing a movie detaches that callback before
    // its buffers disappear. Pending device requests retain this same source.
    hp2::MoviePlayer* movie{};
    std::shared_ptr<WorldAudioStreamSource> retainedByVoice;
};

/* Original inline AddToChain PAL206760..206838 / BETA bsound/include/bSample.h.
 * This native view borrows the existing voice/source/queue; it owns none of
 * their state. Every range uses the retained physical DAT file. */
class bSampleChain final {
public:
    bSampleChain(WorldAudioRequestQueue& queue,WorldAudioVoiceRef voice,WorldAudioStreamSource& source)
        : queue_(queue),voice_(voice),source_(source) {}
    WorldAudioResult AddToChain(const char* filename,uint32_t offset,uint32_t bytes,bool last)
    {
        if (last) return worldAudioPlayFile(&queue_,voice_,&source_,filename,offset,bytes);
        const auto valid=worldAudioValidateVoice(&queue_,voice_);
        if (valid!=WORLD_AUDIO_OK) return valid;
        uint32_t pitch=0;uint16_t volume=0;int8_t pan=0;
        auto result=worldAudioCalcVoicePitch(&queue_,voice_,voice_.instance->pitchOffset,&pitch);
        if (result!=WORLD_AUDIO_OK) return result;
        result=worldAudioCalcVoiceVolume(&queue_,voice_,voice_.instance->volume,&volume);
        if (result!=WORLD_AUDIO_OK) return result;
        result=worldAudioCalcVoicePan(voice_.instance->pan,&pan);
        if (result!=WORLD_AUDIO_OK) return result;
        if (!filename || source_.path!=filename) return WORLD_AUDIO_INVALID_BINDING;
        return worldAudioSubmitChainPart(&queue_,voice_,&source_,offset,bytes,0,pitch,volume,pan);
    }
private:
    WorldAudioRequestQueue& queue_;
    WorldAudioVoiceRef voice_;
    WorldAudioStreamSource& source_;
};
// Explicit native data-provider boundary, not a device completion or a
// substitute audio renderer. PAL callback result1 means the whole requested
// span was filled;0 means a short FIFO read (zero-filled) or detached provider.
std::int32_t worldAudioReadMovieStream(const WorldAudioStreamSource& source,
    void* output, std::uint32_t bytes);
#endif
#endif
