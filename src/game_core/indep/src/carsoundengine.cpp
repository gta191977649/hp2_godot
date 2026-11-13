#include "CarSounds.hpp"
#include "bsound/include/bSound.h"
#include "CarInfo.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Table.hpp"
#include "indep/inc/Race.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Player.hpp"
#include "CameraMover.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/CarDamage.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bFile.hpp"
#include "bware/inc/bWare.hpp"

#include <array>
#include <new>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

CarEngineSoundGlobals carEngineSoundGlobals = {
    100.0f,
    {12, 25, 50, 100, 200, 400},
    {0.800000011920929f, 0.6000000238418579f, 0.4000000059604645f,
     0.4000000059604645f, 0.4000000059604645f, 0.800000011920929f},
    0,
    {{30.0f, 0.0f}, {25.0f, 0.0f}, {20.0f, 0.0f}}
};

namespace {

/* Native factoring of the existing scalar smooth helper136630. No extra
 * original method or separate tuning state is introduced. */
float approach(float current, float target, float maximumChange)
{
    const float upper = current + maximumChange;
    if (upper < target) return upper;
    const float lower = current - maximumChange;
    return target < lower ? lower : target;
}

class AiEngineSound final : public Sound3d {
public:
    AiEngineSound(CarMovement* movement, const CarSoundBindings& bindings);
    ~AiEngineSound() override;
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Pause(int32_t paused) override;

protected:
    WorldAudioResult ActivateSound() override;
    WorldAudioResult DeactivateSound() override;

private:
    std::array<char, 10> SampleName() const;
    float RpmPitch();
    float StartLinePitch();
    CarMovement* movement_;
    CarSoundBindings bindings_;
    WorldAudioVoiceRef voice_;
    float detune_;
    float startTarget_;
    float startPitch_;
    DriverHistory history_;
    int32_t previousGear_;
    int32_t shiftFrames_;
    float smoothedRpm_;
    float gainScale_;
};

AiEngineSound::AiEngineSound(CarMovement* movement, const CarSoundBindings& bindings)
    : movement_(movement), bindings_(bindings), voice_{nullptr, 0}, detune_(0.0f),
      startTarget_(-1.0f), startPitch_(0.0f), history_{}, previousGear_(-1),
      shiftFrames_(10), smoothedRpm_(1000.0f), gainScale_(1.0f)
{
    if (!movement || !movement->bindings.state || !movement->bindings.owner ||
        !movement->bindings.info || !movement->bindings.owner->driverInfo || !bindings.audio ||
        !bindings.globals || !bindings.engineGlobals || !bindings.cars || !bindings.cars->CarsLoaded())
        throw std::invalid_argument("AI engine sound has no actual car/resource/audio owner");
    const int32_t physicalModel = movement->bindings.info->modelCode;
    const int32_t appearanceModel = movement->bindings.owner->driverInfo->appearance.modelCode;
    if (physicalModel < 0 || physicalModel >= 64 || appearanceModel < 0 || appearanceModel >= 64)
        throw std::invalid_argument("AI engine sound has an invalid model selection");
    const auto& physical = bindings.cars->Car(static_cast<size_t>(physicalModel));
    if (movement->bindings.info->body != &physical.rigidBodyInfo ||
        movement->bindings.info->engine != &physical.engineInfo)
        throw std::invalid_argument("AI engine sound physical descriptor is not the same retained CarInfo");
    if (!driverHistoryInitialize(&history_, 20)) throw std::bad_alloc();
    try {
        detune_ = -vectorRandomFloat(&worldAudioRuntime.randomState, 0.4000000059604645f);
        const auto name = SampleName();
        const WorldAudioCue* cue = worldAudioFindInGame(bindings.audio, name.data());
        if (!cue) throw std::invalid_argument("AI engine sample is absent from the actual InGame bank");
        if (worldAudioInitializeVoiceFromCue(bindings.audio, bindings.audio->inGameBank, cue, &voice_) != WORLD_AUDIO_OK ||
            worldAudioSetVoiceCategory(bindings.audio, voice_, 0) != WORLD_AUDIO_OK)
            throw std::runtime_error("AI engine sample cannot acquire its private audio instance");
    } catch (...) {
        driverHistoryDispose(&history_);
        if (voice_.instance) worldAudioRetireVoice(bindings.audio, voice_);
        throw;
    }
}

AiEngineSound::~AiEngineSound()
{
    driverHistoryDispose(&history_);
    if (voice_.instance) worldAudioRetireVoice(bindings_.audio, voice_);
}

std::array<char, 10> AiEngineSound::SampleName() const
{
    const auto& appearance = bindings_.cars->Car(static_cast<size_t>(
        movement_->bindings.owner->driverInfo->appearance.modelCode));
    uint16_t index = 0;
    if (appearance.bodyCategory != 2) {
        const auto& physical = bindings_.cars->Car(static_cast<size_t>(movement_->bindings.info->modelCode));
        index = physical.aiEngineSampleIndex;
    }
    std::array<char, 10> name{{'A', 'I', 'E', 'n', 'g', 'i', 'n', 'e', '0', 0}};
    name[8] = static_cast<char>(static_cast<uint8_t>(static_cast<uint32_t>('0') + index));
    return name;
}

float AiEngineSound::RpmPitch()
{
    const VehicleState* vehicle = movement_->bindings.state;
    const float overrideRpm = bindings_.engineGlobals->overrideRpm;
    const float rpm = overrideRpm == 100.0f ? vehicle->engineRpm : overrideRpm;
    if (vehicle->gear != previousGear_) {
        driverHistoryFill(&history_, rpm);
        shiftFrames_ = 0;
    }
    previousGear_ = vehicle->gear;
    driverHistoryPush(&history_, rpm);
    float step = 5000.0f;
    float gain = 1.0f;
    if (shiftFrames_ < 6) {
        step = static_cast<float>(bindings_.engineGlobals->shiftRpmStep[shiftFrames_]);
        gain = bindings_.engineGlobals->shiftGain[shiftFrames_];
    }
    gainScale_ = gain;
    smoothedRpm_ = approach(smoothedRpm_, history_.value, step);
    shiftFrames_ = static_cast<int32_t>(static_cast<uint32_t>(shiftFrames_) + 1u);
    return smoothedRpm_ * 0.00020000000949949026f;
}

float AiEngineSound::StartLinePitch()
{
    if (startTarget_ == -1.0f) {
        startTarget_ = 0.30000001192092896f;
        startPitch_ = 0.30000001192092896f;
    }
    int32_t falling = 0;
    float step = 0.15000000596046448f;
    if (!(startPitch_ < startTarget_)) {
        step = 0.07999999821186066f;
        falling = 1;
    }
    startPitch_ = approach(startPitch_, startTarget_, step);
    const float value = startPitch_;
    if (value == startTarget_ || falling) {
        if (value == 0.30000001192092896f || falling) {
            if (vectorRandomFloat(&worldAudioRuntime.randomState, 100.0f) < 4.0f)
                startTarget_ = 1.0f;
        }
        if (value == 1.0f) startTarget_ = 0.30000001192092896f;
    }
    return value;
}

WorldAudioResult AiEngineSound::Update(float volume, uint16_t azimuth, float pitch)
{
    if (!spatial.active) return WORLD_AUDIO_OK;
    const VehicleState* vehicle = movement_->bindings.state;
    const RaceFrame* race = bindings_.race;
    const bool countdown = race && (race->state == 2 || race->state == 3) &&
        vehicle->speed * 2.236999988555908f < 5.0f;
    float pitchShift;
    float volumeStep = 0.30000001192092896f;
    if (countdown) {
        volumeStep = 0.07500000298023224f;
        pitchShift = StartLinePitch();
    } else {
        pitchShift = RpmPitch();
        const float speedKmh = vehicle->speed * 3.5999999046325684f;
        volume = volume * gainScale_;
        if (speedKmh < 75.0f) {
            const float speedScale = speedKmh * 0.013333333656191826f;
            const float gain = speedScale * 0.5f + 0.5f;
            volume = volume * gain;
        }
    }
    WorldAudioResult result = worldAudioValidateVoice(bindings_.audio, voice_);
    if (result != WORLD_AUDIO_OK) return result;
    if (!voice_.instance->playing) {
        if (bindings_.globals->policy.audioSystemEnabled && bindings_.globals->policy.vehicleAudioEnabled) {
            result = worldAudioPlayVoice(bindings_.audio, voice_, volume, 0.0f, 0.0f);
            if (result != WORLD_AUDIO_OK) return result;
        }
    } else {
        const float smoothedVolume = approach(voice_.instance->volume, volume, volumeStep);
        result = worldAudioSetVoiceVolume(bindings_.audio, voice_, smoothedVolume);
        if (result != WORLD_AUDIO_OK) return result;
    }
    const float scaledPitch = pitchShift * pitch;
    const float shiftedPitch = detune_ + scaledPitch;
    result = worldAudioSetVoicePitchOffset(bindings_.audio, voice_, shiftedPitch - 1.0f);
    if (result != WORLD_AUDIO_OK) return result;
    return worldAudioSetVoiceControlCode(bindings_.audio, voice_, azimuth);
}

WorldAudioResult AiEngineSound::Pause(int32_t paused)
{
    return worldAudioPauseVoice(bindings_.audio, voice_, paused != 0);
}

WorldAudioResult AiEngineSound::DeactivateSound()
{
    return worldAudioStopInstance(bindings_.audio, voice_.instance);
}

WorldAudioResult AiEngineSound::ActivateSound()
{
    if (!bindings_.globals->policy.audioSystemEnabled || !bindings_.globals->policy.vehicleAudioEnabled)
        return WORLD_AUDIO_OK;
    return worldAudioPlayVoice(bindings_.audio, voice_, 0.0f, 0.0f, 0.0f);
}

WorldAudioResult UpdatePlayerNitroEffect(CarMovement* movement, const CarSoundBindings& bindings,
    const std::array<WorldAudioVoiceRef,2>& nitro, float volume, uint16_t azimuth, float pitch)
{
    if (movement->mode != 1) return WORLD_AUDIO_OK;
    if (!movement->physics || !movement->physics->engine)
        return WORLD_AUDIO_INVALID_BINDING;
    const Engine& engine = *movement->physics->engine;
    if (engine.nitroEnabled) {
        if (!nitro[1].instance->playing) {
            auto result = worldAudioPlayVoice(bindings.audio, nitro[0], volume * 2.5f,
                static_cast<float>(azimuth), 0.0f);
            if (result != WORLD_AUDIO_OK) return result;
            return worldAudioPlayVoice(bindings.audio, nitro[1], volume * 2.5f,
                static_cast<float>(azimuth), pitch);
        }
        const float elapsed = (bindings.clock->worldTimeSeconds - engine.nitroBoostTimer) * 0.25f;
        float gain = 1.0f;
        if (0.800000011920929f < elapsed) gain -= (elapsed - 0.800000011920929f) * 5.0f;
        auto result = worldAudioSetVoiceVolume(bindings.audio, nitro[1], (volume * gain) * 2.5f);
        if (result != WORLD_AUDIO_OK) return result;
        result = worldAudioSetVoiceControlCode(bindings.audio, nitro[1], azimuth);
        if (result != WORLD_AUDIO_OK) return result;
        return worldAudioSetVoicePitchOffset(bindings.audio, nitro[1], pitch * (elapsed + 0.5f));
    }
    return nitro[1].instance->playing ? worldAudioStopInstance(bindings.audio, nitro[1].instance) : WORLD_AUDIO_OK;
}

// Explicit PC extension for a human driving an authored traffic vehicle.
// The released InGame bank supplies the real traffic loop; no racer sound
// bank, physical engine or asset is substituted. AI traffic keeps its own
// original AiEngineSound above, including its category and countdown rules.
class NativeTrafficPlayerEngine final : public Sound3d {
public:
    NativeTrafficPlayerEngine(CarMovement* movement,const CarSoundBindings& bindings)
        :movement_(movement),bindings_(bindings)
    {
        if(!movement || !movement->bindings.state || !movement->bindings.owner ||
           !movement->bindings.owner->driverInfo || !movement->bindings.info ||
           !bindings.audio || !bindings.globals || !bindings.clock || !bindings.cars ||
           movement->bindings.info->bodyCategory!=2 ||
           movement->bindings.owner->driverInfo->driverClass!=1)
            throw std::invalid_argument("Traffic player engine has no actual human car/audio owner");
        const auto model=movement->bindings.info->modelCode;
        if(model<0 || model>=64 || movement->bindings.info->engine!=
            &bindings.cars->Car(static_cast<std::size_t>(model)).engineInfo)
            throw std::invalid_argument("Traffic player engine lost its canonical physical model");
        const auto* cue=worldAudioFindInGame(bindings.audio,"AIEngine0");
        if(!cue)throw std::invalid_argument("Traffic player engine loop is absent from the InGame bank");
        try {
            RequireAudio(worldAudioInitializeVoiceFromCue(bindings.audio,bindings.audio->inGameBank,cue,&voice_));
            RequireAudio(worldAudioSetVoiceCategory(bindings.audio,voice_,2));
            for(std::size_t index=0;index<nitro_.size();++index) {
                const auto* nitroCue=worldAudioFindInGame(bindings.audio,index?"NitroLoop":"NitroStart");
                if(!nitroCue)throw std::invalid_argument("Traffic player Nitro cue is absent from the InGame bank");
                RequireAudio(worldAudioInitializeVoiceFromCue(bindings.audio,bindings.audio->inGameBank,nitroCue,&nitro_[index]));
                RequireAudio(worldAudioSetVoiceCategory(bindings.audio,nitro_[index],2));
            }
        } catch(...) {
            for(auto& nitroVoice:nitro_)if(nitroVoice.instance)worldAudioRetireVoice(bindings.audio,nitroVoice);
            if(voice_.instance)worldAudioRetireVoice(bindings.audio,voice_);
            throw;
        }
        smoothedRpm_=movement->bindings.state->engineRpm;
    }
    ~NativeTrafficPlayerEngine() override
    {
        for(auto& nitroVoice:nitro_)if(nitroVoice.instance)worldAudioRetireVoice(bindings_.audio,nitroVoice);
        if(voice_.instance)worldAudioRetireVoice(bindings_.audio,voice_);
    }
    WorldAudioResult Update(float volume,uint16_t azimuth,float pitch) override
    {
        if(!spatial.active)return WORLD_AUDIO_OK;
        auto* player=movement_->frame ? movement_->frame->environment.player : nullptr;
        if(!player || player->car!=movement_ || movement_->bindings.player!=&player->movement)
            return WORLD_AUDIO_INVALID_BINDING;
        const auto& policy=bindings_.globals->policy;
        if(!policy.audioSystemEnabled || !policy.vehicleAudioEnabled)
            return DeactivateSound();
        auto& state=*movement_->bindings.state;
        const auto& engine=*movement_->bindings.info->engine;
        const float elapsed=std::clamp(bindings_.clock->worldTimeElapsed,0.0f,0.1f);
        const float rpm=std::clamp(state.engineRpm,0.0f,engine.maximumRpm);
        smoothedRpm_=approach(smoothedRpm_,rpm,elapsed*(rpm<smoothedRpm_?39000.0f:18000.0f));
        if(previousGear_>=0 && previousGear_!=state.gear)gearGain_=0.6f;
        previousGear_=state.gear;
        gearGain_=approach(gearGain_,1.0f,elapsed*6.0f);
        const float throttle=std::clamp(state.effectiveThrottle,0.0f,1.0f);
        const float wantedVolume=volume*(0.55f+0.45f*throttle)*gearGain_;
        gain_=approach(gain_,wantedVolume,elapsed*8.0f);
        // The same released loop's source pitch reference is 5000 RPM.
        // The human engine follows the actual RPM even while stationary;
        // it has no AI start-line random revs or speed-distance attenuation.
        const float ratio=std::clamp(smoothedRpm_*0.0002f*pitch,0.125f,4.0f);
        state.engineSoundRpm=smoothedRpm_;
        auto result=voice_.instance->playing ?
            worldAudioSetVoiceVolume(bindings_.audio,voice_,gain_) :
            worldAudioPlayVoice(bindings_.audio,voice_,gain_,0.0f,ratio-1.0f);
        if(result==WORLD_AUDIO_OK)result=worldAudioSetVoicePitchOffset(bindings_.audio,voice_,ratio-1.0f);
        if(result==WORLD_AUDIO_OK)result=worldAudioSetVoiceControlCode(bindings_.audio,voice_,azimuth);
        if(result==WORLD_AUDIO_OK)result=UpdatePlayerNitroEffect(movement_,bindings_,nitro_,volume,azimuth,pitch);
        return result;
    }
    WorldAudioResult Pause(int32_t paused) override
    {
        auto result=worldAudioPauseVoice(bindings_.audio,voice_,paused!=0);
        for(auto nitroVoice:nitro_)if(result==WORLD_AUDIO_OK)
            result=worldAudioPauseVoice(bindings_.audio,nitroVoice,paused!=0);
        return result;
    }
private:
    static void RequireAudio(WorldAudioResult result)
    {
        if(result!=WORLD_AUDIO_OK)throw std::runtime_error("Traffic player engine cannot acquire its private native loop");
    }
    WorldAudioResult ActivateSound() override
    {
        if(!bindings_.globals->policy.audioSystemEnabled || !bindings_.globals->policy.vehicleAudioEnabled)
            return WORLD_AUDIO_OK;
        return worldAudioPlayVoice(bindings_.audio,voice_,0.0f,0.0f,0.0f);
    }
    WorldAudioResult DeactivateSound() override
    {
        auto result=worldAudioStopInstance(bindings_.audio,voice_.instance);
        for(auto nitroVoice:nitro_) {
            const auto stopped=worldAudioStopInstance(bindings_.audio,nitroVoice.instance);
            if(result==WORLD_AUDIO_OK)result=stopped;
        }
        return result;
    }
    CarMovement* movement_;
    CarSoundBindings bindings_;
    WorldAudioVoiceRef voice_{};
    std::array<WorldAudioVoiceRef,2> nitro_{};
    float smoothedRpm_{};
    float gain_{};
    float gearGain_{1.0f};
    std::int32_t previousGear_{-1};
};

} // namespace

Sound3d* carSoundCreateAiEngine(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result)
{
    if (!bindings) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    try {
        Sound3d* sound = new AiEngineSound(movement, *bindings);
        if (result) *result = WORLD_AUDIO_OK;
        return sound;
    } catch (const std::bad_alloc& failure) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_CAPACITY,"AI engine sound",failure.what());
        if (result) *result = WORLD_AUDIO_CAPACITY;
    } catch (const std::exception& failure) {
        const auto status=bindings->audio && bindings->audio->result != WORLD_AUDIO_OK ?
            bindings->audio->result : WORLD_AUDIO_INVALID_BINDING;
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(status,"AI engine sound",failure.what());
        if (result) *result = status;
    } catch (...) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_INVALID_BINDING,"AI engine sound","Non-standard source exception");
        if (result) *result = bindings->audio && bindings->audio->result != WORLD_AUDIO_OK ?
            bindings->audio->result : WORLD_AUDIO_INVALID_BINDING;
    }
    return nullptr;
}

namespace {

/* Exact immutable PAL tables31BC80/31BD08. These are the engine's gain and
 * detune lookup data, not generated exponential/curve approximations. */
const std::array<int8_t, 128> playerEngineGainTable = {{
    0, 3, 7, 10, 13, 16, 19, 22, 24, 27, 29, 31, 33, 35, 37, 39,
    41, 43, 45, 46, 48, 50, 51, 53, 54, 55, 57, 58, 60, 61, 62, 63,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 84, 85, 86, 87, 88, 88, 89, 90, 91, 91, 92, 93,
    94, 94, 95, 96, 96, 97, 98, 98, 99, 100, 100, 101, 101, 102, 103, 103,
    104, 104, 105, 106, 106, 107, 107, 108, 108, 109, 109, 110, 110, 111, 111, 112,
    112, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 118, 118, 119, 119, 119,
    120, 120, 121, 121, 122, 122, 122, 123, 123, 124, 124, 125, 125, 125, 126, 127
 }};
const std::array<uint8_t, 256> playerEngineDetuneTable = {{
    0, 1, 2, 2, 3, 4, 4, 5, 6, 7, 7, 8, 9, 9, 10, 11,
    12, 12, 13, 14, 14, 15, 16, 17, 17, 18, 19, 20, 20, 21, 22, 23,
    23, 24, 25, 26, 26, 27, 28, 29, 30, 30, 31, 32, 33, 33, 34, 35,
    36, 37, 37, 38, 39, 40, 41, 41, 42, 43, 44, 45, 45, 46, 47, 48,
    49, 50, 50, 51, 52, 53, 54, 55, 55, 56, 57, 58, 59, 60, 61, 61,
    62, 63, 64, 65, 66, 67, 67, 68, 69, 70, 71, 72, 73, 74, 75, 75,
    76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 86, 87, 88, 89, 90,
    91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 119, 120, 121, 122,
    123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
    139, 140, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 154, 155, 156,
    157, 158, 159, 160, 161, 163, 164, 165, 166, 167, 168, 169, 171, 172, 173, 174,
    175, 176, 178, 179, 180, 181, 182, 183, 185, 186, 187, 188, 189, 191, 192, 193,
    194, 196, 197, 198, 199, 200, 202, 203, 204, 205, 207, 208, 209, 210, 212, 213,
    214, 216, 217, 218, 219, 221, 222, 223, 225, 226, 227, 229, 230, 231, 232, 234,
    235, 236, 238, 239, 240, 242, 243, 245, 246, 247, 249, 250, 251, 253, 254, 255
 }};

int16_t playerEngineShort(int32_t value)
{
    const uint16_t bits = static_cast<uint16_t>(static_cast<uint32_t>(value));
    int16_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

int8_t playerEngineByte(int32_t value)
{
    const uint8_t bits = static_cast<uint8_t>(static_cast<uint32_t>(value));
    int8_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

int32_t playerEngineShift(int32_t value, unsigned bits)
{
    if (value >= 0) return value >> bits;
    return -static_cast<int32_t>((-static_cast<int64_t>(value) + ((int64_t(1) << bits) - 1)) >> bits);
}

void requireEngineAudio(WorldAudioResult result)
{
    if (result != WORLD_AUDIO_OK) throw std::runtime_error("Player engine audio operation failed");
}

const WorldAudioBank* playerEngineBank(const WorldAudioRequestQueue& audio, const char* name)
{
    if (!name || (!audio.banks && audio.bankCount)) return nullptr;
    for (size_t index = 0; index < audio.bankCount; ++index)
        if (audio.banks[index].name && std::strcmp(audio.banks[index].name, name) == 0)
            return &audio.banks[index];
    return nullptr;
}

struct PlayerEngineVoiceParameters {
    int16_t volume = 0;
    int16_t balance = 0;
    float pitch = 0.0f;
    uint16_t azimuth = 0;
};

/* VoiceAttr owns one private SDK sample. Its cached four source parameters
 * are distinct from the same sample's SDK playback/fade/pause state. */
class PlayerEngineVoice final {
public:
    PlayerEngineVoice() = default;
    PlayerEngineVoice(const PlayerEngineVoice&) = delete;
    PlayerEngineVoice& operator=(const PlayerEngineVoice&) = delete;
    ~PlayerEngineVoice();
    WorldAudioResult Dispose();
    void Bind(const CarSoundBindings& bindings) { bindings_ = &bindings; }
    void Copy(const WorldAudioBank* bank, const WorldAudioCue* cue);
    WorldAudioResult Play(const PlayerEngineVoiceParameters& parameters);
    WorldAudioResult Stop();
    WorldAudioResult SetVolume(int16_t volume);
    WorldAudioResult SetPitch(float pitch);
    WorldAudioResult SetAzimuth(uint16_t azimuth);
    WorldAudioResult Pause(int32_t paused);
    bool Playing() const { return voice_.instance && voice_.instance->playing; }
    WorldAudioVoiceRef Sample() const { return voice_; }
    PlayerEngineVoiceParameters parameters;
private:
    const CarSoundBindings* bindings_ = nullptr;
    WorldAudioVoiceRef voice_{nullptr, 0};
};

PlayerEngineVoice::~PlayerEngineVoice()
{
    Dispose();
}

WorldAudioResult PlayerEngineVoice::Dispose()
{
    if (!voice_.instance) return WORLD_AUDIO_OK;
    const auto result = worldAudioRetireVoice(bindings_->audio, voice_);
    if (result == WORLD_AUDIO_OK) voice_ = {nullptr, 0};
    return result;
}

void PlayerEngineVoice::Copy(const WorldAudioBank* bank, const WorldAudioCue* cue)
{
    if (!bindings_ || !bindings_->audio || !bank || !cue)
        throw std::invalid_argument("Player engine sample has no retained bank/cue owner");
    if (voice_.instance) requireEngineAudio(worldAudioRetireVoice(bindings_->audio, voice_));
    requireEngineAudio(worldAudioInitializeVoiceFromCue(bindings_->audio, bank, cue, &voice_));
    requireEngineAudio(worldAudioSetVoiceCategory(bindings_->audio, voice_, 2));
}

WorldAudioResult PlayerEngineVoice::Play(const PlayerEngineVoiceParameters& value)
{
    parameters = value;
    if (parameters.volume < 0) parameters.volume = 0;
    if (!voice_.instance) return WORLD_AUDIO_OK; // Actual empty bSample has no source handle.
    if (bindings_->globals->policy.audioSystemEnabled && bindings_->globals->policy.vehicleAudioEnabled) {
        const float volume = static_cast<float>(parameters.volume) * 0.007874015718698502f;
        const auto result = worldAudioPlayVoice(bindings_->audio, voice_, volume, 0.0f, parameters.pitch);
        if (result != WORLD_AUDIO_OK) return result;
    }
    return worldAudioSetVoiceControlCode(bindings_->audio, voice_, parameters.azimuth);
}

WorldAudioResult PlayerEngineVoice::Stop()
{
    if (parameters.volume) return SetVolume(0);
    return voice_.instance ? worldAudioStopInstance(bindings_->audio, voice_.instance) : WORLD_AUDIO_OK;
}

WorldAudioResult PlayerEngineVoice::SetVolume(int16_t value)
{
    const int32_t upper = parameters.volume + 15;
    const int32_t lower = parameters.volume - 15;
    int16_t next = value > upper ? playerEngineShort(upper) :
        value < lower ? playerEngineShort(lower) : value;
    if (next == parameters.volume) return WORLD_AUDIO_OK;
    if (next < 0) next = 0;
    parameters.volume = next;
    if (!voice_.instance) return WORLD_AUDIO_OK;
    return worldAudioSetVoiceVolume(bindings_->audio, voice_,
        static_cast<float>(next) * 0.007874015718698502f);
}

WorldAudioResult PlayerEngineVoice::SetPitch(float value)
{
    if (parameters.pitch == value) return WORLD_AUDIO_OK;
    parameters.pitch = value;
    return voice_.instance ? worldAudioSetVoicePitchOffset(bindings_->audio, voice_, value) : WORLD_AUDIO_OK;
}

WorldAudioResult PlayerEngineVoice::SetAzimuth(uint16_t value)
{
    if (parameters.azimuth == value) return WORLD_AUDIO_OK;
    parameters.azimuth = value;
    return voice_.instance ? worldAudioSetVoiceControlCode(bindings_->audio, voice_, value) : WORLD_AUDIO_OK;
}

WorldAudioResult PlayerEngineVoice::Pause(int32_t paused)
{
    return voice_.instance ? worldAudioPauseVoice(bindings_->audio, voice_, paused) : WORLD_AUDIO_OK;
}

struct PlayerEngineTable {
    std::vector<uint8_t> bytes;
    std::array<const int8_t*, 8> curves[2]{};
    int8_t SampleIndex(size_t index) const
    {
        if (bytes.size() < 0x168 || index >= 8)
            throw std::invalid_argument("Player engine curve sample index is outside its authored table");
        return playerEngineByte(bytes[0x20 + index]);
    }
    void Resolve();
};

void PlayerEngineTable::Resolve()
{
    if (bytes.size() < 0x168 || std::memcmp(bytes.data(), "CRDl", 4) != 0 || bytes[4] != 2)
        throw std::invalid_argument("Player engine curve file has an invalid final format");
    /* Windows pointer views replace32-bit relocated addresses. The sole
     * retained source buffer still owns all512-byte signed curve windows. */
    for (size_t plane = 0; plane < 2; ++plane) {
        for (size_t index = 0; index < 8; ++index) {
            curves[plane][index] = nullptr;
            if (SampleIndex(index) < 0) continue;
            const size_t field = 0x128 + plane * 0x20 + index * 4;
            const int32_t relative = static_cast<int32_t>(hp2::ReadU32(bytes.data() + field));
            const int64_t offset = static_cast<int64_t>(field) + relative;
            if (offset < 0 || static_cast<uint64_t>(offset) + 512u > bytes.size())
                throw std::invalid_argument("Player engine curve exceeds its actual retained file");
            curves[plane][index] = reinterpret_cast<const int8_t*>(bytes.data() + offset);
        }
    }
    bytes[5] = 1;
}

class PlayerEngineSound final : public Sound3d {
public:
    PlayerEngineSound(CarMovement* movement, const CarSoundBindings& bindings);
    ~PlayerEngineSound() override;
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Pause(int32_t paused) override;
    WorldAudioResult UpdateCarDamage(float volume, uint16_t azimuth, float pitch);
private:
    struct Configuration {
        int32_t present = -1;
        int16_t external = 0;
        int16_t enabled = 1;
        int32_t detune = 0;
        float gain = 1.0f;
    };
    void Initialize();
    void ResolveTables();
    void RampThrottle();
    void UpdateGearShiftEffect();
    void UpdateToRedLineRpm(float& rpm);
    WorldAudioResult UpdateNitroEffect(float volume, uint16_t azimuth, float pitch);
    void UpdateEngineDucker();
    void UpdateBlownRodDamage(int32_t damage);
    WorldAudioResult UpdateEngine();
    void Set(int32_t volume, int32_t rpmIndex, int32_t throttle, int32_t distanceSquared,
        int32_t pitchFixed, uint16_t azimuth, float viewDot, int32_t cameraView);
    [[maybe_unused]] int32_t DetuneToLinear(int32_t detune) const;
    const hp2::CarDefinition& Definition() const;
    CarFramePlayerState* Player() const;
    WorldAudioResult ActivateSound() override;
    WorldAudioResult DeactivateSound() override;
    CarMovement* movement_;
    CarSoundBindings bindings_;
    std::array<WorldAudioVoiceRef,2> nitro_{};
    std::array<PlayerEngineVoice, 16> primary_;
    std::array<PlayerEngineVoice, 16> secondary_;
    std::array<int8_t, 12> restartDelay_{};
    std::array<Configuration, 12> configurations_;
    PlayerEngineTable tables_[2];
    float parameter3B0_ = 0.0f;
    float parameter3B2_ = 0.0f;
    float gearGain_ = 1.0f;
    int32_t gearShift_ = 0;
    float blownRodRemaining_ = 0.0f;
    int32_t volume_ = 0;
    int32_t throttle_ = 0;
    int32_t blend_ = 0;
    int16_t pitchFixed_ = 0;
    uint16_t azimuth_ = 0;
    uint16_t stereoSpread_ = 0;
    int8_t cameraBlend_ = 0;
    int8_t channelCount_ = 0;
    int16_t rpm_ = 0;
    int16_t previousRpm_ = 0;
    int32_t detuneScale_ = 0;
    int32_t detuneBias_ = 0;
    std::array<int16_t, 4> cameraBlends_{};
    bool stereo_ = true;
    int32_t backfireAccumulator_ = 0;
    float smoothedRpm_;
    float duckElapsed_ = 0.0f;
    float duckGain_ = 1.0f;
    int32_t audioModeAtInit_ = 0;
};

const hp2::CarDefinition& PlayerEngineSound::Definition() const
{
    return bindings_.cars->Car(static_cast<size_t>(movement_->bindings.info->modelCode));
}

CarFramePlayerState* PlayerEngineSound::Player() const
{
    if (!bindings_.spatial.players || !movement_) return nullptr;
    for (auto* player : bindings_.spatial.players->playersByNumber) {
        if (player && player->car == movement_ &&
            movement_->bindings.player == &player->movement)
            return player;
    }
    return nullptr;
}

PlayerEngineSound::PlayerEngineSound(CarMovement* movement, const CarSoundBindings& bindings)
    : movement_(movement), bindings_(bindings), smoothedRpm_(0.0f)
{
    if (!movement || !movement->bindings.info || !movement->bindings.state ||
        !movement->bindings.owner || !bindings.audio ||
        !bindings.globals || !bindings.engineGlobals || !bindings.cars || !bindings.cars->CarsLoaded() ||
        !bindings.files || !bindings.clock || !bindings.manager || !bindings.session ||
        !bindings.spatial.players || !bindings.spatial.cameras)
        throw std::invalid_argument("Player engine has no actual car/resource/audio/camera owner");
    // The full Car constructor creates sound while Car.player is still null.
    // Player() is checked by its real Update consumer after RC attaches it.
    const int32_t model = movement->bindings.info->modelCode;
    if (model < 0 || model >= 64 || movement->bindings.info->engine != &Definition().engineInfo)
        throw std::invalid_argument("Player engine descriptor is not the canonical retained model");
    smoothedRpm_ = bindings.engineGlobals->overrideRpm == 100.0f ?
        movement->bindings.state->engineRpm : bindings.engineGlobals->overrideRpm;
    for (auto& voice : primary_) voice.Bind(bindings_);
    for (auto& voice : secondary_) voice.Bind(bindings_);
    try {
        Initialize();
        for (size_t index = 0; index < 2; ++index) {
            const auto* cue = worldAudioFindInGame(bindings.audio, index ? "NitroLoop" : "NitroStart");
            if (!cue) throw std::invalid_argument("Player engine Nitro sample is absent from the real bank");
            requireEngineAudio(worldAudioInitializeVoiceFromCue(bindings.audio, bindings.audio->inGameBank, cue, &nitro_[index]));
            requireEngineAudio(worldAudioSetVoiceCategory(bindings.audio, nitro_[index], 2));
        }
        requireEngineAudio(Activate(1));
    } catch (...) {
        for (auto& voice : nitro_) if (voice.instance) worldAudioRetireVoice(bindings.audio, voice);
        throw;
    }
}

PlayerEngineSound::~PlayerEngineSound()
{
    /* PAL stops each primary/secondary pair before releasing curve files and
     * destructing all16+16 actual sample owners in reverse order. */
    for (size_t index = 0; index < 12; ++index) {
        const auto first = primary_[index].Sample();
        const auto second = secondary_[index].Sample();
        if (first.instance) worldAudioStopInstance(bindings_.audio, first.instance);
        if (second.instance) worldAudioStopInstance(bindings_.audio, second.instance);
    }
    for (auto& table : tables_) table.bytes.clear();
    for (size_t index = secondary_.size(); index-- > 0; ) secondary_[index].Dispose();
    for (size_t index = primary_.size(); index-- > 0; ) primary_[index].Dispose();
    for (size_t index = 2; index-- > 0; )
        if (nitro_[index].instance) worldAudioRetireVoice(bindings_.audio, nitro_[index]);
}

void PlayerEngineSound::Initialize()
{
    if (!bindings_.globals->policy.audioSystemEnabled) return;
    const auto* common = playerEngineBank(*bindings_.audio, "common");
    if (!common) throw std::invalid_argument("Player engine has no actual common sound region");
    static constexpr const char* menuNames[6] = {
        "MenuStart", "MenuLeftRight", "MenuUpDown", "MenuSelect", "MenuCancel", "MenuWrong"
    };
    for (const auto* name : menuNames) {
        const auto* cue = worldAudioFindInBank(common, name);
        if (!cue) throw std::invalid_argument("Player engine common sample is missing");
        WorldAudioVoiceRef voice{};
        requireEngineAudio(worldAudioAcquireBankSample(bindings_.audio, common, cue, &voice));
        requireEngineAudio(worldAudioStopInstance(bindings_.audio, voice.instance));
    }
    /* The PAL initialization explicitly gives the one SDK owner three
     * updates separated by two16ms waits. No per-engine audio timer exists. */
    for (int index = 0; index < 3; ++index) {
        if (!bindings_.audio->library)
            throw std::invalid_argument("Player engine has no same bSound owner");
        requireEngineAudio(bindings_.audio->library->Update());
        if (index != 2) std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    const auto& car = Definition();
    const auto* bank = playerEngineBank(*bindings_.audio, car.engineSound.c_str());
    if (!bank) throw std::invalid_argument("Player engine model sound region is missing");
    audioModeAtInit_ = worldAudioGetMode();
    for (size_t index = 0; index < 4; ++index) cameraBlends_[index] = car.engineThrottleRamp[index];
    detuneScale_ = car.engineDetuneScale;
    detuneBias_ = car.engineDetuneBias;
    parameter3B0_ = static_cast<float>(car.engineParameter3B0);
    parameter3B2_ = static_cast<float>(car.engineParameter3B2);
    static constexpr const char* suffixes[12] = {
        "lelo", "lemd", "lehi", "lxlo", "lxmd", "lxhi",
        "ceid", "cemd", "cehi", "cxid", "cxmd", "cxhi"
    };
    const auto end = std::find(car.playerEngineSamplePrefix.begin(), car.playerEngineSamplePrefix.end(), '\0');
    const std::string prefix(car.playerEngineSamplePrefix.begin(), end);
    for (size_t index = 0; index < 12; ++index) {
        auto& config = configurations_[index];
        config.detune = car.engineVoiceDetune[index];
        config.gain = car.engineVoiceGain[index];
        config.external = suffixes[index][1] == 'x' ? 1 : 0;
        const std::string name = prefix + suffixes[index];
        const auto* cue = worldAudioFindInBank(bank, name.c_str());
        if (!cue) throw std::invalid_argument("Player engine authored sample is missing");
        config.present = 1;
        primary_[index].Copy(bank, cue);
        secondary_[index].Copy(bank, cue);
        if (index < 6) {
            requireEngineAudio(worldAudioPrimeVoice(bindings_.audio, primary_[index].Sample()));
            if (bindings_.spatial.players->count == 1)
                requireEngineAudio(worldAudioPrimeVoice(bindings_.audio, secondary_[index].Sample()));
        }
    }
    tables_[0].bytes = bindings_.files->Load("SOUND/ENGINE.CTB");
    tables_[1].bytes = bindings_.files->Load("SOUND/ENGINE.LTB");
    ResolveTables();
}

void PlayerEngineSound::ResolveTables()
{
    for (auto& table : tables_) table.Resolve();
    volume_ = 0;
    rpm_ = 0;
    channelCount_ = 0;
    for (int index = 0; index < 16; ++index) {
        /* This is the source's index/8 here, distinct from the per-channel
         * index%8 used later by UpdateEngine. Do not silently normalize it. */
        if (tables_[index < 8 ? 0 : 1].SampleIndex(static_cast<size_t>(index / 8)) >= 0)
            channelCount_ = static_cast<int8_t>(index + 1);
    }
}

void PlayerEngineSound::RampThrottle()
{
    const int32_t requested = convertFloatToInt32(movement_->bindings.state->effectiveThrottle * 255.0f);
    const int32_t difference = requested - throttle_;
    if (throttle_ < requested) {
        throttle_ += difference / 2;
        if (throttle_ >= 254) throttle_ = 255;
    } else if (requested < throttle_) {
        throttle_ += difference / 8;
        if (throttle_ < 9) throttle_ = 0;
    }
    throttle_ = std::clamp(throttle_, 0, 255);
}

void PlayerEngineSound::UpdateGearShiftEffect()
{
    if (movement_->mode != 1) return;
    if (!movement_->physics || !movement_->physics->driveTrain)
        throw std::invalid_argument("Player engine gear effect has no actual physical drivetrain");
    const DriveTrain& train = *movement_->physics->driveTrain;
    if (train.currentGear < train.desiredGear) gearShift_ = 1;
    else if (train.desiredGear < train.currentGear) gearShift_ = 0;
    float target = 1.0f;
    const float rpm = bindings_.engineGlobals->overrideRpm == 100.0f ?
        movement_->bindings.state->engineRpm : bindings_.engineGlobals->overrideRpm;
    if (gearShift_ && 1500.0f < rpm && (train.clutchState == 0 || train.clutchState == 2))
        target = std::max(0.4000000059604645f, train.clutch);
    gearGain_ = approach(gearGain_, target, 0.20000000298023224f);
}

void PlayerEngineSound::UpdateToRedLineRpm(float& rpm)
{
    if (movement_->mode == 1) {
        if (!movement_->physics || !movement_->physics->driveTrain || !movement_->physics->engine)
            throw std::invalid_argument("Player engine redline effect has no actual physical components");
        const DriveTrain& train = *movement_->physics->driveTrain;
        if (train.currentGear >= 2) {
            if (train.currentGear >= 9) throw std::invalid_argument("Player engine physical gear is outside its source table");
            const auto& info = *movement_->physics->engine->info;
            const float limit = info.redlineRpm + parameter3B0_;
            const float shift = std::min(train.shiftUpRpm[train.currentGear], limit);
            const float scale = limit / shift;
            rpm *= scale;
            if (movement_->bindings.state->gear == Definition().driveTrainInfo.forwardGearCount + 1)
                rpm += parameter3B2_;
            rpm = std::min(rpm, info.maximumRpm * scale);
        }
    }
    movement_->bindings.state->engineSoundRpm = rpm;
}

WorldAudioResult PlayerEngineSound::UpdateNitroEffect(float volume, uint16_t azimuth, float pitch)
{
    return UpdatePlayerNitroEffect(movement_,bindings_,nitro_,volume,azimuth,pitch);
}

void PlayerEngineSound::UpdateEngineDucker()
{
    if (!bindings_.manager->channelVolumes || bindings_.manager->channelVolumeCount <= 4)
        throw std::invalid_argument("Player engine ducker has no actual category4 mixer owner");
    if (!(0.05000000074505806f < bindings_.manager->channelVolumes[4])) return;
    if (movement_->bindings.state->effectiveThrottle < 0.5f) {
        duckElapsed_ = 0.0f;
        duckGain_ = approach(duckGain_, 1.0f, 0.00800000037997961f);
    } else {
        duckElapsed_ += bindings_.clock->worldTimeElapsed;
        if (5.0f < duckElapsed_)
            duckGain_ = approach(duckGain_, 0.75f, 0.00800000037997961f);
    }
}

void PlayerEngineSound::UpdateBlownRodDamage(int32_t damage)
{
    blownRodRemaining_ -= bindings_.clock->worldTimeElapsed;
    if (blownRodRemaining_ > 0.0f) return;
    if (static_cast<float>(rpm_) < 5.0f) {
        blownRodRemaining_ = 0.0f;
        return;
    }
    /* The PAL evaluates this clamp but discards its value. It does not emit
     * a fabricated rod sample or scale the subsequent source timer. */
    const float unused = std::clamp(static_cast<float>(damage - 50) * 10.0f, 0.0f, 1.0f);
    (void)unused;
    blownRodRemaining_ = std::max(20.0f / static_cast<float>(rpm_), 0.12999999523162842f);
}

WorldAudioResult PlayerEngineSound::UpdateCarDamage(float volume, uint16_t azimuth, float pitch)
{
    (void)volume;
    (void)pitch;
    if (!movement_->bindings.damage) return WORLD_AUDIO_INVALID_BINDING;
    int32_t damage = 0;
    for (uint32_t item = 0; item < 15; ++item)
        damage += static_cast<int32_t>(carDamageItemLevel(movement_->bindings.damage->levels, item));
    if (damage >= 50) UpdateBlownRodDamage(damage);
    if (damage >= 26) {
        backfireAccumulator_ = static_cast<int32_t>(static_cast<uint32_t>(backfireAccumulator_) + static_cast<uint32_t>(throttle_));
        if (throttle_ < 128 && backfireAccumulator_ > 100000 &&
            vectorRandomBounded(100, &worldAudioRuntime.randomState) == 0) {
            WorldAudioVoiceRef voice{};
            auto result = worldAudioAcquireInGameSample(bindings_.audio, "TirePop1", &voice);
            if (result != WORLD_AUDIO_OK) return result;
            result = worldAudioPlayVoice(bindings_.audio, voice, 0.0f, 0.0f, 0.0f);
            if (result != WORLD_AUDIO_OK) return result;
            result = worldAudioSetVoiceControlCode(bindings_.audio, voice, azimuth);
            if (result != WORLD_AUDIO_OK) return result;
            result = worldAudioSetVoicePitchOffset(bindings_.audio, voice, 1.5f);
            if (result != WORLD_AUDIO_OK) return result;
        }
    }
    backfireAccumulator_ = 0;
    float chance = 0.0f;
    for (const auto& row : bindings_.engineGlobals->damageThresholdChance) {
        if (convertFloatToInt32(row[0]) < damage) {
            chance = row[1];
            break;
        }
    }
    if (chance != 0.0f && vectorRandomFloat(&worldAudioRuntime.randomState, 100.0f) <= chance)
        rpm_ = playerEngineShort(convertFloatToInt32(static_cast<float>(rpm_) * 0.5f));
    if (bindings_.engineGlobals->lastDamageLevel != damage)
        bindings_.engineGlobals->lastDamageLevel = damage;
    return WORLD_AUDIO_OK;
}

void PlayerEngineSound::Set(int32_t volume, int32_t rpmIndex, int32_t throttle,
    int32_t distanceSquared, int32_t pitchFixed, uint16_t azimuth, float viewDot, int32_t cameraView)
{
    (void)distanceSquared;
    (void)azimuth;
    rpm_ = playerEngineShort(rpmIndex);
    pitchFixed_ = playerEngineShort(playerEngineShift(pitchFixed, 4));
    blend_ = std::min(127, playerEngineShift(throttle, 5) + throttle);
    if (blend_ < 4) blend_ = 0;
    if (cameraView == -1) {
        const float scale = viewDot < 0.0f ? 30.0f : 60.0f;
        cameraBlend_ = playerEngineByte(convertFloatToInt32(viewDot * scale + 44.0f));
    } else {
        if (cameraView < 0 || cameraView >= 4) throw std::invalid_argument("Player engine camera configuration is outside its authored window");
        cameraBlend_ = playerEngineByte(cameraBlends_[cameraView]);
    }
    cameraBlend_ = std::min<int8_t>(cameraBlend_, 127);
    volume_ = volume;
}

int32_t PlayerEngineSound::DetuneToLinear(int32_t detune) const
{
    int32_t linear = 4096;
    while (detune >= 1200) {
        detune -= 1200;
        linear = static_cast<int32_t>(static_cast<uint32_t>(linear) << 1);
    }
    while (detune < -1199) {
        detune += 1200;
        linear = playerEngineShift(linear, 1);
    }
    int32_t index = playerEngineShift(detune * 13981, 16);
    index = std::max(index, -255);
    const uint32_t value = playerEngineDetuneTable[static_cast<size_t>(index < 0 ? index + 256 : index)] + 256u;
    const int32_t product = static_cast<int32_t>(static_cast<uint32_t>(linear) * value);
    return playerEngineShift(product, index < 0 ? 9 : 8);
}

WorldAudioResult PlayerEngineSound::UpdateEngine()
{
    UpdateGearShiftEffect();
    previousRpm_ = rpm_;
    if (rpm_ < 0 || rpm_ >= 512 || blend_ < 0 || blend_ >= 128 || cameraBlend_ < 0)
        return WORLD_AUDIO_INVALID_BINDING;
    for (int channel = 0; channel < channelCount_; ++channel) {
        const int bank = channel < 8 ? 0 : 1;
        const size_t lane = static_cast<size_t>(channel < 8 ? channel : channel - 8);
        const int sample = tables_[bank].SampleIndex(lane);
        if (sample < 0) continue;
        if (sample >= 12) return WORLD_AUDIO_INVALID_BINDING;
        auto& config = configurations_[sample];
        if (config.present == -1) continue;
        const int8_t* curve = tables_[bank].curves[0][lane];
        if (!curve) return WORLD_AUDIO_INVALID_BINDING;
        int gain = playerEngineShift(static_cast<int32_t>(curve[rpm_]) * volume_, 7);
        const float normalized = static_cast<float>(static_cast<int32_t>(rpm_) * 127) * 0.0022727272007614374f;
        const int8_t normalizedByte = playerEngineByte(convertFloatToInt32(normalized));
        const int detune = detuneBias_ + config.detune +
            playerEngineShift((static_cast<int>(normalizedByte) - 64) * detuneScale_, 6);
        const float pitch = static_cast<float>(detune) * 0.0008333333535119891f;
        gain = playerEngineShift(gain * playerEngineGainTable[bank ? blend_ : 127 - blend_], 7);
        auto& first = primary_[sample];
        auto& second = secondary_[sample];
        if (!gain) {
            if (first.Playing()) {
                auto result = first.Stop();
                if (result != WORLD_AUDIO_OK) return result;
                result = second.Stop();
                if (result != WORLD_AUDIO_OK) return result;
            }
            continue;
        }
        gain = std::min(playerEngineShift(gain * 5, 2), 127);
        const int camera = config.external ? cameraBlend_ : 127 - cameraBlend_;
        gain = playerEngineShift(gain * playerEngineGainTable[static_cast<size_t>(camera)], 7);
        float scale = gearGain_ * duckGain_;
        scale *= config.gain;
        if (!audioModeAtInit_) scale *= 1.2999999523162842f;
        const int volume = convertFloatToInt32(static_cast<float>(gain) * scale);
        const uint16_t left = static_cast<uint16_t>(azimuth_ - stereoSpread_);
        const uint16_t right = static_cast<uint16_t>(azimuth_ + stereoSpread_);
        PlayerEngineVoiceParameters parameters;
        parameters.volume = playerEngineShort(volume);
        parameters.balance = 64;
        parameters.pitch = pitch;
        parameters.azimuth = left;
        if (!first.Playing()) {
            restartDelay_[sample] = 2;
            if (config.enabled) {
                const auto result = first.Play(parameters);
                if (result != WORLD_AUDIO_OK) return result;
            }
            continue;
        }
        if (stereo_ && !second.Playing() && restartDelay_[sample] <= 0) {
            parameters.azimuth = right;
            if (config.enabled) {
                const auto result = second.Play(parameters);
                if (result != WORLD_AUDIO_OK) return result;
            }
        } else {
            restartDelay_[sample] = playerEngineByte(static_cast<int32_t>(restartDelay_[sample]) - 1);
        }
        if (!config.enabled) {
            if (first.Playing()) {
                const auto result = first.Stop();
                if (result != WORLD_AUDIO_OK) return result;
            }
            if (second.Playing()) {
                const auto result = second.Stop();
                if (result != WORLD_AUDIO_OK) return result;
            }
        }
        const int16_t boundedVolume = playerEngineShort(std::max(volume, 0));
        auto result = first.SetVolume(boundedVolume);
        if (result != WORLD_AUDIO_OK) return result;
        result = second.SetVolume(boundedVolume);
        if (result != WORLD_AUDIO_OK) return result;
        result = first.SetAzimuth(left);
        if (result != WORLD_AUDIO_OK) return result;
        result = second.SetAzimuth(right);
        if (result != WORLD_AUDIO_OK) return result;
        result = first.SetPitch(pitch);
        if (result != WORLD_AUDIO_OK) return result;
        result = second.SetPitch(pitch);
        if (result != WORLD_AUDIO_OK) return result;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult PlayerEngineSound::Update(float volume, uint16_t azimuth, float pitch)
{
    if (!spatial.active || !bindings_.globals->policy.audioSystemEnabled ||
        !bindings_.globals->policy.vehicleAudioEnabled) return WORLD_AUDIO_OK;
    CarFramePlayerState* player = Player();
    if (!player || !player->cameras || player->cameras->viewNumber < 0 || player->cameras->viewNumber >= 4)
        return WORLD_AUDIO_INVALID_BINDING;
    const CarCinematicView* view = bindings_.spatial.cameras->screens[player->cameras->viewNumber].view;
    if (!view) return WORLD_AUDIO_INVALID_BINDING;
    RampThrottle();
    const auto* state = movement_->bindings.state;
    const float x = state->position[0] - view->current.eye[0];
    const float y = state->position[1] - view->current.eye[1];
    const float z = state->position[2] - view->current.eye[2];
    const float distance = std::sqrt((x * x + y * y) + z * z);
    float rpm = bindings_.engineGlobals->overrideRpm == 100.0f ? state->engineRpm : bindings_.engineGlobals->overrideRpm;
    rpm -= static_cast<float>(Definition().enginePitchOrigin);
    UpdateToRedLineRpm(rpm);
    smoothedRpm_ = rpm > smoothedRpm_ + 270.0f ? smoothedRpm_ + 270.0f :
        rpm < smoothedRpm_ - 650.0f ? smoothedRpm_ - 650.0f : rpm;
    UpdateEngineDucker();
    auto result = UpdateNitroEffect(volume, azimuth, pitch);
    if (result != WORLD_AUDIO_OK) return result;
    const int32_t volumeFixed = convertFloatToInt32(volume * 127.0f);
    const int32_t rpmIndex = std::min(440, convertFloatToInt32((smoothedRpm_ / Definition().engineInfo.maximumRpm) * 440.0f));
    const int32_t distanceSquared = convertFloatToInt32(distance * distance);
    const int32_t pitchFixed = convertFloatToInt32(pitch * 65536.0f);
    int32_t cameraView = player->settings.cameraView;
    float viewDot = 0.0f;
    const int32_t session = bindings_.session->state;
    if (session == 4 || session == 7 || session == 5 || session == 9) {
        cameraView = -1;
        viewDot = (view->current.forward[0] * state->orientation[4] +
            view->current.forward[1] * state->orientation[5]) + view->current.forward[2] * state->orientation[6];
    }
    if (azimuth == 0 && worldAudioGetMode() == 1) {
        stereo_ = true;
        azimuth_ = 0;
        stereoSpread_ = 8000;
    } else if (azimuth == 0 && worldAudioGetMode() == 2) {
        stereo_ = true;
        azimuth_ = 0;
        stereoSpread_ = 6000;
    } else {
        stereo_ = false;
        azimuth_ = azimuth;
        stereoSpread_ = 0;
    }
    Set(volumeFixed, rpmIndex, throttle_, distanceSquared, pitchFixed, azimuth, viewDot, cameraView);
    return UpdateEngine();
}

WorldAudioResult PlayerEngineSound::Pause(int32_t paused)
{
    for (size_t index = 0; index < 12; ++index) {
        auto result = primary_[index].Pause(paused);
        if (result != WORLD_AUDIO_OK) return result;
        result = secondary_[index].Pause(paused);
        if (result != WORLD_AUDIO_OK) return result;
    }
    for (auto voice : nitro_) if (voice.instance) {
        const auto result = worldAudioPauseVoice(bindings_.audio, voice, paused);
        if (result != WORLD_AUDIO_OK) return result;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult PlayerEngineSound::ActivateSound()
{
    return WORLD_AUDIO_OK; // Actual PAL210408 is JR RA/NOP.
}

WorldAudioResult PlayerEngineSound::DeactivateSound()
{
    return WORLD_AUDIO_OK; // Actual PAL210400 is JR RA/NOP; no invented sample stop.
}

} // namespace

Sound3d* carSoundCreatePlayerEngine(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result)
{
    if (!bindings) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    try {
        Sound3d* sound = movement && movement->bindings.info && movement->bindings.info->bodyCategory==2 ?
            static_cast<Sound3d*>(new NativeTrafficPlayerEngine(movement,*bindings)) :
            static_cast<Sound3d*>(new PlayerEngineSound(movement,*bindings));
        if (result) *result = WORLD_AUDIO_OK;
        return sound;
    } catch (const std::bad_alloc& failure) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_CAPACITY,"Player engine sound",failure.what());
        if (result) *result = WORLD_AUDIO_CAPACITY;
    } catch (const std::exception& failure) {
        const auto status=bindings->audio && bindings->audio->result != WORLD_AUDIO_OK ?
            bindings->audio->result : WORLD_AUDIO_INVALID_BINDING;
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(status,"Player engine sound",failure.what());
        if (result) *result = status;
    } catch (...) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_INVALID_BINDING,"Player engine sound","Non-standard source exception");
        if (result) *result = bindings->audio && bindings->audio->result != WORLD_AUDIO_OK ?
            bindings->audio->result : WORLD_AUDIO_INVALID_BINDING;
    }
    return nullptr;
}

WorldAudioResult carSoundUpdatePlayerEngineDamage(Sound3d* engine,
    float volume, uint16_t azimuth, float pitch)
{
    auto* player = dynamic_cast<PlayerEngineSound*>(engine);
    return player ? player->UpdateCarDamage(volume, azimuth, pitch) : WORLD_AUDIO_INVALID_BINDING;
}
