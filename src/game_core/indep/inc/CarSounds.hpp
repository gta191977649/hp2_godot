#ifndef NFSHP2_CAR_SOUNDS_HPP
#define NFSHP2_CAR_SOUNDS_HPP

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CarSoundPolicy {
    int32_t audioSystemEnabled;
    int32_t vehicleAudioEnabled;
} CarSoundPolicy;

/* Borrow the one live process policy; no copied enabled flag is returned. */
CarSoundPolicy* carSoundCurrentPolicy(void);

typedef struct CarSoundState CarSoundState;

int32_t carSoundStartHorn(CarSoundState* sound);
int32_t carSoundStopHorn(CarSoundState* sound);
int32_t carSoundHornActive(const CarSoundState* sound);
int32_t carSoundRefreshEffects(CarSoundState* sound);
void carSoundSetSurfaceLevel(CarSoundState* sound, int32_t channel, int32_t level);

#ifdef __cplusplus
}

#include "Sound3d.hpp"
#include <exception>
#include "bsound/include/bSample.h"
#include "bsound/include/bVoice.h"
#include <array>
#include <cstdint>
#include <chrono>
#include <memory>
#include <vector>
#include <string>

class DemoDiscManager;

namespace hp2 {
struct FlailerSoundState;

// One persistent sound-settings owner, borrowed by every frontend slider.
// Defaults/balance are the actual mutable sound-module globals. The saved
// speech values are a distinct source output: SaveVolumesForSpeech copies
// the live mixer categories into these cells after a slider change.
// This native representation is neither a beta class layout nor a second
// live mixer array.
struct SoundVolumeSettings {
    float balance{0.699999988079071f};
    std::array<float,6> defaults{{0.3799999952316284f,0.550000011920929f,
        0.33000001311302185f,0.3100000023841858f,0.8500000238418579f,
        0.44999998807907104f}};
    std::array<float,5> speechRegionVolumes{};
    float speechCategorySixVolume{};
};

extern "C++" {
float GetVolumeBalance(const SoundVolumeSettings& settings);
float GetVolumeDefault(const SoundVolumeSettings& settings,std::int32_t channel);
void SaveVolumesForSpeech(FlailerSoundState& state);
void SetVolumeDefaults(FlailerSoundState& state,const DemoDiscManager& demo);
void PlayCommonRegionSample(FlailerSoundState& state,const char* name,float gain);
}

struct ApplicationFrontendFlow;
struct GameFlowBindings;
struct FrontendPlayerManager;
} // namespace hp2

/* These source globals have one live owner. The sound RNG remains in the
 * existing WorldAudioRuntimeState and is never copied into this record. */
struct CarSoundGlobals {
    CarSoundPolicy policy;
    int32_t tirePopEnabled;
    int32_t splashEnabled;
    uint16_t multiplayerAzimuth;
    float speedOfSound;
    Sound3dEnvelope playerEnvelope;
    Sound3dEnvelope aiEnvelope;
    uint16_t reflectionAzimuth;
    int32_t nextHorn;
    int32_t nextSiren;
};

extern CarSoundGlobals carSoundGlobals;
void carSoundInitializeGlobals(CarSoundGlobals* globals);
const WorldAudioCue* carHornSoundChooseSample(WorldAudioRequestQueue* audio,
    CarSoundGlobals* globals);

namespace hp2 { class CarInfoResource; class GameResources; class TrackScenarioData; }
namespace hp2 { struct SoundVolumeSettings; }
struct TrackResourceInfo;
struct CarMovement;
struct RaceFrame;
struct CarFrameState;
struct CarFramePlayerState;
struct TerrainSurface;
struct WorldSimulation;
struct Chopper;
struct RaceParameters;

/* The writable override and shift tables are the actual engine-sound
 * globals, separate from vehicle physics and from the general audio RNG. */
struct CarEngineSoundGlobals {
    float overrideRpm;
    int16_t shiftRpmStep[6];
    float shiftGain[6];
    int32_t lastDamageLevel;
    float damageThresholdChance[3][2];
};
extern CarEngineSoundGlobals carEngineSoundGlobals;

struct WorldImpactAudio;

struct CarSoundBindings {
    WorldAudioRequestQueue* audio;
    CarSoundGlobals* globals;
    CarEngineSoundGlobals* engineGlobals;
    const hp2::CarInfoResource* cars;
    const GameTime* clock;
    const RaceFrame* race;
    Sound3dBindings spatial;
    const CarFrameState* frame;
    const hp2::SoundVolumeSettings* volumes;
    const hp2::GameResources* files;
    const WorldAudioManagerState* manager;
    const GameSession* session;
    /* Borrow the canonical current TRACKB34200 record and the result of its
     * +84 bank-name lookup. A missing SDK bank is a genuine NULL result;
     * an absent source resource binding is an explicit native error. */
    const TrackResourceInfo* trackInfo;
    const WorldAudioBank* trackBank;
    // Native owner-thread transport progress while a recorded completion
    // waits for its real bank. This never advances GameTime or presentation.
    void* physicalProgressOwner{};
    WorldAudioResult (*servicePhysicalAudio)(void*){};
};

struct CollisionSystem;

WorldAudioResult carSoundBindTrackBank(CarSoundBindings& bindings,
    const hp2::TrackScenarioData& track);

/* Sound-module cells, independent of launch flags, live SDK mixer gains and
 * the region-loading counter. PAL GP-4A60/-4A5C/-4A58/-49D0 respectively. */
enum class SoundRegionCompletion { None, FrontendReady, TrackReady };
struct SoundRegionRecord {
    std::array<char,32> name{};
    int32_t loaded{};
};

struct SoundDriverState {
    int32_t suppressWorldAudio;
    float currentMuffler;
    float desiredMuffler;
    float lastCategoryScale;
    // Final331848 is initialized1 and read by LoadLocalizedSpeechFiles and
    // SED's event4 callback. It is independent of audioSystemEnabled and
    // the language manager's textEnabled flag.
    int32_t speechEnabled{1};
    std::array<SoundRegionRecord,4> regions{};
    int32_t commonRegionLoaded{};
    SoundRegionCompletion completion{};
    hp2::ApplicationFrontendFlow* completionFlow{};
    const hp2::GameFlowBindings* completionBindings{};
    /* Native replacement for the SDK ticker pair, independent of world/real
     * GameTime and the actual platform video-blank counter. */
    std::chrono::steady_clock::time_point loadStarted{};
    const WorldAudioBank* frontendRegionCache{};
    float delayedJukeboxSeconds{};
    /* Four independent released process cells GP-49F0/-49EC/-49E8/-49E4,
     * all initialized1; the actual favourite/shuffle consumers clear them. */
    std::array<int32_t,3> favouriteEligible{{1,1,1}};
    int32_t initialJukeboxShuffle{1};
    std::chrono::steady_clock::time_point lastUpdate{};
    int32_t printVoicesRequested{};
    /* TriggerTimerWarningSound's one process cell, PAL003324A4 in .sbss.
     * It is shared by both HUDs and starts at zero in the released image. */
    int32_t timerWarningSecond{};
    // ExtractJukeboxTrackStrings retains one split metadata record for the
    // current (group,track) pair. This is the sound module's actual cache.
    std::uint32_t jukeboxMetadataKey{UINT32_MAX};
    std::array<std::string,16> jukeboxMetadata{};
    std::size_t jukeboxMetadataCount{};
};
extern SoundDriverState soundDriverState;
WorldAudioResult soundPlayTimerCountdown(WorldAudioRequestQueue* audio,
    const int32_t* remainingTicks);

float soundGetCategoryVolume(const hp2::SoundVolumeSettings& settings,
    int32_t category);
int32_t soundIsNisPlaying(const CarFramePlayerRegistry& players);
WorldAudioResult soundScaleCategoryVolumes(const CarSoundBindings& bindings,
    int32_t includeSpeech, float scale);
WorldAudioResult soundUpdateVolumeMuffler(const CarSoundBindings& bindings,
    const hp2::ApplicationFrontendFlow& flow);

class bRegion;
struct ReplayIntegerLog;
WorldAudioResult soundLoadRegion(const CarSoundBindings& bindings,const char* name);
WorldAudioResult soundDriverCallback(const CarSoundBindings& bindings,
    uint32_t event,const bRegion* region);
int32_t soundRealRegionLoadingComplete(const CarSoundBindings& bindings);
int32_t soundRegionLoadingInProgress();
int32_t soundRegionLoadingComplete();
WorldAudioResult soundServiceRegionLoading(const CarSoundBindings& bindings,
    ReplayIntegerLog* joylog);
WorldAudioResult soundJukeboxSetSelection(const CarSoundBindings& bindings,
    const hp2::FrontendPlayerManager& frontend,
    const RaceParameters& parameters,int32_t selection);
WorldAudioResult soundJukeboxStart(const CarSoundBindings& bindings,
    const hp2::FrontendPlayerManager& frontend,const RaceParameters& parameters,
    const hp2::ApplicationFrontendFlow& flow,bool unused);
WorldAudioResult soundJukeboxStop(const CarSoundBindings& bindings);
WorldAudioResult soundJukeboxDelayedStart(const CarSoundBindings& bindings,float seconds);
bool soundJukeboxIsPlaying(const CarSoundBindings& bindings,
    const hp2::ApplicationFrontendFlow& flow);
std::int32_t soundJukeboxGetCurrentTrack();
const char* soundJukeboxGetTrackName(const CarSoundBindings& bindings,std::int32_t group=-1,std::int32_t track=-1);
const char* soundJukeboxGetTrackArtist(const CarSoundBindings& bindings,std::int32_t group=-1,std::int32_t track=-1);
const char* soundJukeboxGetTrackLabel(const CarSoundBindings& bindings,std::int32_t group=-1,std::int32_t track=-1);
const char* soundJukeboxGetTrackGenre(const CarSoundBindings& bindings,std::int32_t group=-1,std::int32_t track=-1);
std::int32_t soundJukeboxGetTrackStatus(const CarSoundBindings& bindings,std::int32_t group=-1,std::int32_t track=-1);
WorldAudioResult soundMuteCarSounds(const WorldSimulation* simulation,bool muted);
WorldAudioResult soundMute(const CarSoundBindings& bindings,
    const WorldSimulation* simulation,bool muted);
WorldAudioResult soundMuteReplay(const CarSoundBindings& bindings,
    const WorldSimulation* simulation,bool muted);
WorldAudioResult soundUpdate(const CarSoundBindings& bindings,
    const WorldSimulation* simulation,WorldStreamAudio& stream,
    const hp2::FrontendPlayerManager& frontend,const RaceParameters& parameters,
    const hp2::ApplicationFrontendFlow& flow,ReplayIntegerLog* joylog,
    const bVoiceStatusSnapshot* observedDevice=nullptr);
WorldAudioResult soundUnloadRegions(const CarSoundBindings& bindings,WorldImpactAudio* collisions);
WorldAudioResult soundLoadRegionsFrontend(const CarSoundBindings& bindings,
    const hp2::FrontendPlayerManager& frontend,
    const RaceParameters& parameters,hp2::ApplicationFrontendFlow* completionFlow);
WorldAudioResult soundLoadRegionsInGame(const CarSoundBindings& bindings,
    const hp2::FrontendPlayerManager& frontend,const RaceParameters& parameters,
    WorldImpactAudio* collisions,CollisionSystem* collisionSystem,
    hp2::ApplicationFrontendFlow& flow,const hp2::GameFlowBindings& gameFlow);

Sound3d* carSoundCreateAiEngine(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result);
Sound3d* carSoundCreatePlayerEngine(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result);
WorldAudioResult carSoundUpdatePlayerEngineDamage(Sound3d* engine,
    float volume, uint16_t azimuth, float pitch);

class CarHornSound final : public Sound3d {
public:
    CarHornSound();
    ~CarHornSound() override;
    WorldAudioResult Initialize(WorldAudioRequestQueue* audio,
        CarSoundGlobals* globals, const WorldAudioCue* sample,
        const WorldAudioBank* sourceBank = nullptr);
    WorldAudioResult Dispose();
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Pause(int32_t paused) override;

protected:
    WorldAudioResult ActivateSound() override;
    WorldAudioResult DeactivateSound() override;

private:
    WorldAudioRequestQueue* audio_;
    CarSoundGlobals* globals_;
    WorldAudioVoiceRef voice_;
    float pitchScale_;
};

class CarSirenSound final : public Sound3d {
public:
    CarSirenSound();
    ~CarSirenSound() override;
    WorldAudioResult Initialize(WorldAudioRequestQueue* audio,
        CarSoundGlobals* globals, const GameTime* clock);
    WorldAudioResult Dispose();
    WorldAudioResult SetMode(int32_t mode);
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Pause(int32_t paused) override;
    void SetBroken(int32_t broken);
    float BrokenPitch();

protected:
    WorldAudioResult ActivateSound() override;
    WorldAudioResult DeactivateSound() override;

private:
    WorldAudioRequestQueue* audio_;
    CarSoundGlobals* globals_;
    const GameTime* clock_;
    WorldAudioVoiceRef voices_[3];
    int32_t current_;
    float remainingSeconds_;
    int32_t broken_;
    float targetPitch_;
    float currentPitch_;
};

class BaseTireSound : public Sound3d {
public:
    explicit BaseTireSound(const CarSoundBindings& bindings);
    ~BaseTireSound() override;
    WorldAudioResult Pause(int32_t paused) override;
    WorldAudioResult UpdateTireImpact(const CarMovement* movement, int32_t side);
    virtual void ScaleRoadNoise(float scale);
    virtual void TriggerTireBlowOut();

protected:
    CarSoundBindings bindings_;
    uint16_t previousContact_[2];
    WorldAudioVoiceRef impact_[2];
};

BaseTireSound* carSoundCreateAiTire(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result);
BaseTireSound* carSoundCreatePlayerTire(CarMovement* movement, int32_t side,
    const CarSoundBindings* bindings, WorldAudioResult* result);

/* Native name retained for the existing C ABI. This is the actual
 * BaseCarSound owner, not an event/surface-level mirror of that owner. */
struct CarSoundState : Sound3d {
    ~CarSoundState() override;
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Pause(int32_t paused) override;
    WorldAudioResult ScaleRoadNoise(float scale);
    bool InSoundSphere() const;
    Sound3d* Engine() const;
    BaseTireSound* Tire(int32_t side) const;

protected:
    CarSoundState(CarMovement* movement, const CarSoundBindings& bindings);
    WorldAudioResult ActivateSound() override;
    WorldAudioResult DeactivateSound() override;
    virtual const WorldAudioCue* GetHornSample();
    WorldAudioResult UpdateSirenSound();
    WorldAudioResult UpdateSplash(float volume, int32_t& level, int32_t& countdown);
    CarMovement* movement_;
    CarSoundBindings bindings_;
    int32_t splashLevel_[2];
    int32_t splashCountdown_[2];
    std::unique_ptr<Sound3d> horn_;
    std::unique_ptr<Sound3d> engine_;
    std::unique_ptr<CarSirenSound> siren_;
    WorldAudioVoiceRef tirePop_;
    std::array<std::unique_ptr<BaseTireSound>, 2> tires_;
    const Sound3dEnvelope* envelope_;

    friend int32_t carSoundStartHorn(CarSoundState* sound);
    friend int32_t carSoundStopHorn(CarSoundState* sound);
    friend int32_t carSoundHornActive(const CarSoundState* sound);
    friend int32_t carSoundRefreshEffects(CarSoundState* sound);
    friend void carSoundSetSurfaceLevel(CarSoundState* sound, int32_t channel, int32_t level);
};

class BasePlayerCarSound final : public CarSoundState {
public:
    BasePlayerCarSound(CarMovement* movement, const CarSoundBindings& bindings);
    ~BasePlayerCarSound() override;
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Pause(int32_t paused) override;
    WorldAudioVoiceRef GetEngineReflectionSample(int32_t index);
    float CalcWindVolume(float volume) const;
    float CalcWindPitch(float pitch) const;
    WorldAudioResult UpdateEngineReflection(float volume, uint16_t azimuth, float pitch);
    WorldAudioResult UpdateDynamicBodyReflections();
    WorldAudioResult UpdateTopologyReflections();
    /* Borrow the same source+50 topology owner for SoundScape reflection. */
    const TerrainSurface* ReflectionSurface() const;
protected:
    const WorldAudioCue* GetHornSample() override;
private:
    CarFramePlayerState* Player() const;
    std::unique_ptr<TerrainSurface> topology_;
    WorldAudioVoiceRef wind_;
    int32_t reflectionPending_[2];
    int32_t reflectionReady_;
    uint16_t playerAzimuth_;
};

CarSoundState* carSoundCreatePlayer(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result);
CarSoundState* carSoundCreateAi(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result);
CarSoundState* carSoundCreateAiCop(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result);
WorldAudioResult carSoundUpdateAll(const WorldSimulation* simulation);

/* The source query reads the sole live SoundScape inside-tunnel value. */
int32_t soundScapeIsInsideTunnel();
/* The actual writable DSP controls. Current/applied-to-queue caches belong to
 * SoundScape itself; no hardware state is inferred from these desired values.
 * PAL GP: mix-4690, pan-468C, preset-4684, parameters-467C/-4674/-466C/-4664. */
struct SoundScapeControls {
    float desiredMix;
    float pan;
    int32_t preset;
    std::array<float, 4> parameters;
};
extern SoundScapeControls soundScapeControls;

/* One retained current-track resource, live sample pool and DSP controls.
 * Weather modulation is its separate source entry, not an extra frame phase. */
WorldAudioResult soundScapeBindServices(const CarSoundBindings& bindings);
WorldAudioResult soundScapeLoadResources(const CarSoundBindings& bindings,
    const hp2::TrackScenarioData& track);
WorldAudioResult soundScapeUnloadResources(const hp2::TrackScenarioData& track);
// Native terminal release: detach authored borrows even if a voice fails.
std::exception_ptr soundScapeShutdownResources(const hp2::TrackScenarioData& track) noexcept;
/* The global secondary bSample members survive resource unload. Release
 * them at application audio shutdown while the same queue is still alive. */
WorldAudioResult soundScapeDispose();
WorldAudioResult soundScapeUpdate();
WorldAudioResult soundScapeUpdateWeather();
WorldAudioResult soundScapeUpdateReflectionReverb(CarMovement* first, CarMovement* second);
WorldAudioResult soundScapeMute(int32_t muted);
WorldAudioResult soundScapeSelectReverbArea(CarMovement* first, CarMovement* second,
    int32_t* selectedPlayer);

class ChopperSound final : public Sound3d {
public:
    ChopperSound(Chopper* chopper, const CarSoundBindings& bindings);
    ~ChopperSound() override;
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Pause(int32_t paused) override;
    void AdjustForTunnelVolume(float& volume);
protected:
    WorldAudioResult ActivateSound() override;
    WorldAudioResult DeactivateSound() override;
private:
    Chopper* chopper_;
    CarSoundBindings bindings_;
    WorldAudioVoiceRef voice_;
    int32_t previousInsideTunnel_;
    float tunnelTransition_;
    friend WorldAudioResult carSoundBindChopper(Chopper*, const CarSoundBindings*);
    friend WorldAudioResult carSoundDisposeChopper(Chopper*);
};

Sound3d* carSoundCreateChopper(Chopper* chopper, const CarSoundBindings* bindings,
    WorldAudioResult* result);
WorldAudioResult carSoundBindChopper(Chopper* chopper, const CarSoundBindings* bindings);
WorldAudioResult carSoundDisposeChopper(Chopper* chopper);

#endif
#endif
