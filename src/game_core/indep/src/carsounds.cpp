#include "CarSounds.hpp"
#include "bsound/include/bSound.h"
#include "indep/inc/Timer.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include "CarInfo.hpp"
#include "PhysicsCar.hpp"
#include "RigidBody.hpp"
#include "Race.hpp"
#include "indep/inc/Race.hpp"
#include "Player.hpp"
#include "indep/inc/Collision.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>

/* Exact initialized PAL cells. The explicit global initializer below writes
 * only its actual envelope/azimuth cells; it never resets the live RNG. */
CarSoundGlobals carSoundGlobals = {
    {1, 1}, 1, 1, 0x2AAA, 350.0f,
    {1.2999999523162842f, 260.0f, 0.003846153849735856f},
    {1.2999999523162842f, 130.0f, 0.007692307699471712f},
    0x4000, 0, 0
};

CarSoundPolicy* carSoundCurrentPolicy(void)
{
    return &carSoundGlobals.policy;
}

namespace {

const WorldAudioBank* soundBank(const WorldAudioRequestQueue& audio, const char* name)
{
    if (audio.bankCount && !audio.banks) return nullptr;
    for (size_t index = 0; index < audio.bankCount; ++index)
        if (audio.banks[index].name && std::strcmp(audio.banks[index].name, name) == 0)
            return &audio.banks[index];
    return nullptr;
}

bool bankContainsCue(const WorldAudioBank* bank, const WorldAudioCue* cue)
{
    if (!bank || !cue || (bank->cueCount && !bank->cues)) return false;
    for (size_t index = 0; index < bank->cueCount; ++index)
        if (&bank->cues[index] == cue) return true;
    return false;
}

const WorldAudioBank* uniqueCueBank(const WorldAudioRequestQueue& audio, const WorldAudioCue* cue)
{
    const WorldAudioBank* found = nullptr;
    if (audio.bankCount && !audio.banks) return nullptr;
    for (size_t index = 0; index < audio.bankCount; ++index) {
        if (!bankContainsCue(&audio.banks[index], cue)) continue;
        if (found) return nullptr;
        found = &audio.banks[index];
    }
    return found;
}

} // namespace

void carSoundInitializeGlobals(CarSoundGlobals* globals)
{
    globals->multiplayerAzimuth = 0x2AAA;
    globals->playerEnvelope.inverseDistance = 0.003846153849735856f;
    globals->aiEnvelope.volume = 1.2999999523162842f;
    globals->aiEnvelope.inverseDistance = 0.007692307699471712f;
    globals->playerEnvelope.volume = 1.2999999523162842f;
    globals->playerEnvelope.maximumDistance = 260.0f;
    globals->reflectionAzimuth = 0x4000;
    globals->aiEnvelope.maximumDistance = 130.0f;
}

const WorldAudioCue* carHornSoundChooseSample(WorldAudioRequestQueue* audio,
    CarSoundGlobals* globals)
{
    if (!audio || !globals || globals->nextHorn < 0 || globals->nextHorn > 2) {
        if (audio) audio->result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    const int32_t selected = globals->nextHorn;
    globals->nextHorn = (selected + 1) % 3;
    char name[] = "Horn01";
    name[5] = static_cast<char>(selected + '1');
    return worldAudioFindInGame(audio, name);
}

CarHornSound::CarHornSound()
    : audio_(nullptr), globals_(nullptr), voice_{nullptr, 0}, pitchScale_(1.0f)
{
}

CarHornSound::~CarHornSound()
{
    Dispose();
}

WorldAudioResult CarHornSound::Initialize(WorldAudioRequestQueue* audio,
    CarSoundGlobals* globals, const WorldAudioCue* sample, const WorldAudioBank* sourceBank)
{
    if (!audio || !globals || voice_.instance) return WORLD_AUDIO_INVALID_BINDING;
    sound3dInitialize(&spatial);
    audio_ = audio;
    globals_ = globals;
    const WorldAudioCue* cue = sample ? sample : carHornSoundChooseSample(audio, globals);
    const WorldAudioBank* bank = sample ?
        (sourceBank ? sourceBank : uniqueCueBank(*audio, sample)) : audio->inGameBank;
    if (!bankContainsCue(bank, cue)) return audio->result = WORLD_AUDIO_INVALID_BINDING;
    WorldAudioResult result = worldAudioInitializeVoiceFromCue(audio, bank, cue, &voice_);
    if (result != WORLD_AUDIO_OK) return result;
    result = worldAudioSetVoiceCategory(audio, voice_, 0);
    if (result != WORLD_AUDIO_OK) return result;
    pitchScale_ = 1.0f;
    return WORLD_AUDIO_OK;
}

WorldAudioResult CarHornSound::Dispose()
{
    if (!voice_.instance) return WORLD_AUDIO_OK;
    const WorldAudioResult result = worldAudioRetireVoice(audio_, voice_);
    if (result == WORLD_AUDIO_OK) voice_ = {nullptr, 0};
    return result;
}

WorldAudioResult CarHornSound::ActivateSound()
{
    if (!audio_ || !globals_) return WORLD_AUDIO_INVALID_BINDING;
    if (!globals_->policy.audioSystemEnabled) return WORLD_AUDIO_OK;
    return worldAudioPlayVoice(audio_, voice_, 0.0f, 0.0f, 0.0f);
}

WorldAudioResult CarHornSound::DeactivateSound()
{
    if (!audio_) return WORLD_AUDIO_INVALID_BINDING;
    return worldAudioStopInstance(audio_, voice_.instance);
}

WorldAudioResult CarHornSound::Pause(int32_t paused)
{
    return worldAudioPauseVoice(audio_, voice_, paused != 0);
}

WorldAudioResult CarHornSound::Update(float volume, uint16_t azimuth, float pitch)
{
    if (!spatial.active) return WORLD_AUDIO_OK;
    WorldAudioResult result = worldAudioSetVoiceVolume(audio_, voice_, volume);
    if (result != WORLD_AUDIO_OK) return result;
    result = worldAudioSetVoiceControlCode(audio_, voice_, azimuth);
    if (result != WORLD_AUDIO_OK) return result;
    const float scaledPitch = pitchScale_ * pitch;
    return worldAudioSetVoicePitchOffset(audio_, voice_, scaledPitch - 1.0f);
}

CarSirenSound::CarSirenSound()
    : audio_(nullptr), globals_(nullptr), clock_(nullptr), voices_{}, current_(0),
      remainingSeconds_(17.0f), broken_(0), targetPitch_(1.0f), currentPitch_(1.0f)
{
}

CarSirenSound::~CarSirenSound()
{
    Dispose();
}

WorldAudioResult CarSirenSound::Initialize(WorldAudioRequestQueue* audio,
    CarSoundGlobals* globals, const GameTime* clock)
{
    if (!audio || !globals || !clock || globals->nextSiren < 0 || globals->nextSiren > 2)
        return WORLD_AUDIO_INVALID_BINDING;
    for (const auto& voice : voices_)
        if (voice.instance) return WORLD_AUDIO_INVALID_BINDING;
    sound3dInitialize(&spatial);
    audio_ = audio;
    globals_ = globals;
    clock_ = clock;
    remainingSeconds_ = 17.0f;
    for (int32_t index = 0; index < 3; ++index) {
        char name[] = "CopSiren01";
        name[9] = static_cast<char>(index + '1');
        const WorldAudioCue* cue = worldAudioFindInGame(audio, name);
        if (!cue) return audio->result = WORLD_AUDIO_INVALID_BINDING;
        WorldAudioResult result = worldAudioInitializeVoiceFromCue(audio, audio->inGameBank, cue, &voices_[index]);
        if (result != WORLD_AUDIO_OK) return result;
        result = worldAudioSetVoiceCategory(audio, voices_[index], 0);
        if (result != WORLD_AUDIO_OK) return result;
    }
    globals->nextSiren = (globals->nextSiren + 1) % 3;
    current_ = globals->nextSiren;
    broken_ = 0;
    targetPitch_ = 1.0f;
    currentPitch_ = 1.0f;
    return WORLD_AUDIO_OK;
}

WorldAudioResult CarSirenSound::Dispose()
{
    WorldAudioResult result = WORLD_AUDIO_OK;
    for (int32_t index = 2; index >= 0; --index) {
        if (!voices_[index].instance) continue;
        const WorldAudioResult retired = worldAudioRetireVoice(audio_, voices_[index]);
        if (retired == WORLD_AUDIO_OK) voices_[index] = {nullptr, 0};
        else if (result == WORLD_AUDIO_OK) result = retired;
    }
    if (audio_) audio_->result = result;
    return result;
}

WorldAudioResult CarSirenSound::SetMode(int32_t mode)
{
    WorldAudioResult result = WORLD_AUDIO_OK;
    if (mode == 0) {
        if (spatial.active) result = Activate(0);
    } else if (!spatial.active) {
        result = Activate(1);
    }
    if (spatial.active) SetBroken(mode == 4);
    return result;
}

void CarSirenSound::SetBroken(int32_t broken)
{
    broken_ = broken;
}

WorldAudioResult CarSirenSound::ActivateSound()
{
    if (!audio_ || !globals_) return WORLD_AUDIO_INVALID_BINDING;
    if (!globals_->policy.audioSystemEnabled) return WORLD_AUDIO_OK;
    return worldAudioPlayVoice(audio_, voices_[current_], 0.0f, 0.0f, 0.0f);
}

WorldAudioResult CarSirenSound::DeactivateSound()
{
    if (!audio_) return WORLD_AUDIO_INVALID_BINDING;
    return worldAudioStopInstance(audio_, voices_[current_].instance);
}

WorldAudioResult CarSirenSound::Pause(int32_t paused)
{
    return worldAudioPauseVoice(audio_, voices_[current_], paused != 0);
}

float CarSirenSound::BrokenPitch()
{
    if (targetPitch_ == currentPitch_)
        targetPitch_ = vectorRandomFloat(&worldAudioRuntime.randomState, 0.8999999761581421f) + 0.10000000149011612f;
    const float upper = currentPitch_ + 0.029999999329447746f;
    if (upper < targetPitch_) currentPitch_ = upper;
    else {
        const float lower = currentPitch_ - 0.029999999329447746f;
        currentPitch_ = targetPitch_ < lower ? lower : targetPitch_;
    }
    return currentPitch_;
}

WorldAudioResult CarSirenSound::Update(float volume, uint16_t azimuth, float pitch)
{
    if (!audio_ || !globals_ || !clock_) return WORLD_AUDIO_INVALID_BINDING;
    if (volume == 0.0f) {
        const WorldAudioResult result = Activate(0);
        if (result != WORLD_AUDIO_OK) return result;
    }
    if (!spatial.active) return WORLD_AUDIO_OK;

    remainingSeconds_ = remainingSeconds_ - clock_->worldTimeElapsed;
    if (remainingSeconds_ < 0.0f) {
        const WorldAudioResult result = worldAudioStopInstance(audio_, voices_[current_].instance);
        if (result != WORLD_AUDIO_OK) return result;
        current_ = (current_ + 1) % 3;
        remainingSeconds_ = 17.0f;
    }
    WorldAudioVoiceRef voice = voices_[current_];
    WorldAudioResult result = worldAudioValidateVoice(audio_, voice);
    if (result != WORLD_AUDIO_OK) return result;
    if (voice.instance->playing) {
        result = worldAudioSetVoiceVolume(audio_, voice, volume);
        if (result != WORLD_AUDIO_OK) return result;
    } else {
        remainingSeconds_ = 17.0f;
        if (globals_->policy.audioSystemEnabled) {
            result = worldAudioPlayVoice(audio_, voice, volume, 0.0f, 0.0f);
            if (result != WORLD_AUDIO_OK) return result;
        }
    }
    if (broken_) pitch = pitch * BrokenPitch();
    result = worldAudioSetVoicePitchOffset(audio_, voice, pitch - 1.0f);
    if (result != WORLD_AUDIO_OK) return result;
    return worldAudioSetVoiceControlCode(audio_, voice, azimuth);
}

namespace {

WorldAudioResult splashSample(WorldAudioRequestQueue* audio, int32_t index,
    WorldAudioVoiceRef* sample)
{
    if (index == -1)
        index = static_cast<int32_t>(vectorRandomBounded(3, &worldAudioRuntime.randomState));
    char name[] = "Splash1";
    name[6] = static_cast<char>(static_cast<uint8_t>(static_cast<uint32_t>(index) + '1'));
    return worldAudioAcquireInGameSample(audio, name, sample);
}

class AiCarSound final : public CarSoundState {
public:
    AiCarSound(CarMovement* movement, const CarSoundBindings& bindings);
    ~AiCarSound() override;
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
};

} // namespace

CarSoundState::CarSoundState(CarMovement* movement, const CarSoundBindings& bindings)
    : movement_(movement), bindings_(bindings), splashLevel_{}, splashCountdown_{},
      horn_{}, engine_{}, siren_{}, tirePop_{nullptr, 0}, tires_{}, envelope_(nullptr)
{
    if (!movement || !movement->bindings.state || !movement->bindings.info ||
        !bindings.audio || !bindings.globals)
        throw std::invalid_argument("Car sound has no actual movement/audio owner");
    for (int32_t index = 0; index < 3; ++index) {
        WorldAudioVoiceRef splash{};
        if (splashSample(bindings.audio, index, &splash) != WORLD_AUDIO_OK ||
            worldAudioSetVoiceCategory(bindings.audio, splash, 0) != WORLD_AUDIO_OK)
            throw std::runtime_error("Car splash samples are absent from the actual bank");
    }
    if (worldAudioAcquireInGameSample(bindings.audio, "TirePop1", &tirePop_) != WORLD_AUDIO_OK ||
        worldAudioSetVoiceCategory(bindings.audio, tirePop_, 0) != WORLD_AUDIO_OK)
        throw std::runtime_error("Car tire-pop sample is absent from the actual bank");
    if (movement->bindings.info->bodyCategory == 1) {
        siren_ = std::make_unique<CarSirenSound>();
        if (siren_->Initialize(bindings.audio, bindings.globals, bindings.clock) != WORLD_AUDIO_OK)
            throw std::runtime_error("Car siren cannot initialize its actual samples");
    }
}

CarSoundState::~CarSoundState()
{
    horn_.reset();
    engine_.reset();
    siren_.reset();
    tires_[0].reset();
    tires_[1].reset();
    // Splash and tire-pop are borrowed bank samples, not private voices.
}

WorldAudioResult CarSoundState::Update(float volume, uint16_t azimuth, float pitch)
{
    (void)azimuth;
    if (!envelope_ || !horn_) return WORLD_AUDIO_INVALID_BINDING;
    if (sound3dUpdate(&spatial, &bindings_.spatial, movement_->bindings.state->position,
        envelope_) != SOUND3D_OK) return WORLD_AUDIO_INVALID_BINDING;
    spatial.volume = spatial.volume * volume;
    spatial.pitch = spatial.pitch * pitch;
    WorldAudioResult result = horn_->Update(spatial.volume, spatial.azimuth, spatial.pitch);
    if (result == WORLD_AUDIO_OK && siren_) result = UpdateSirenSound();
    if (result == WORLD_AUDIO_OK)
        result = UpdateSplash(spatial.volume, splashLevel_[0], splashCountdown_[0]);
    if (result == WORLD_AUDIO_OK)
        result = UpdateSplash(spatial.volume, splashLevel_[1], splashCountdown_[1]);
    return result;
}

WorldAudioResult CarSoundState::UpdateSirenSound()
{
    if (!siren_) return WORLD_AUDIO_INVALID_BINDING;
    WorldAudioResult result = WORLD_AUDIO_OK;
    if (spatial.active) {
        if (!bindings_.frame) return WORLD_AUDIO_INVALID_BINDING;
        result = siren_->SetMode(bindings_.frame->signalEnabled);
    } else if (siren_->spatial.active) {
        result = siren_->Activate(0);
    }
    if (result == WORLD_AUDIO_OK)
        result = siren_->Update(spatial.volume, spatial.azimuth, spatial.pitch);
    return result;
}

const WorldAudioCue* CarSoundState::GetHornSample()
{
    return nullptr; // The original BaseCarSound virtual method returns NULL.
}

WorldAudioResult CarSoundState::UpdateSplash(float volume, int32_t& level, int32_t& countdown)
{
    if (countdown) countdown = static_cast<int32_t>(static_cast<uint32_t>(countdown) - 1u);
    if (!level) return WORLD_AUDIO_OK;
    if (countdown) {
        level = 0;
        return WORLD_AUDIO_OK;
    }
    WorldAudioResult result = WORLD_AUDIO_OK;
    if (bindings_.globals->policy.audioSystemEnabled &&
        bindings_.globals->policy.vehicleAudioEnabled && bindings_.globals->splashEnabled) {
        WorldAudioVoiceRef sample{};
        result = splashSample(bindings_.audio, -1, &sample);
        if (result == WORLD_AUDIO_OK)
            result = worldAudioPlayVoice(bindings_.audio, sample, volume * static_cast<float>(level), -1.0f, 0.0f);
    }
    countdown = 10;
    level = 0;
    return result;
}

WorldAudioResult CarSoundState::Pause(int32_t paused)
{
    if (!horn_ || !engine_) return WORLD_AUDIO_INVALID_BINDING;
    WorldAudioResult result = horn_->Pause(paused);
    if (result == WORLD_AUDIO_OK) result = engine_->Pause(paused);
    if (result == WORLD_AUDIO_OK && siren_) result = siren_->Pause(paused);
    for (auto& tire : tires_) {
        if (result == WORLD_AUDIO_OK && tire) result = tire->Pause(paused);
    }
    for (int32_t index = 0; result == WORLD_AUDIO_OK && index < 3; ++index) {
        WorldAudioVoiceRef sample{};
        result = splashSample(bindings_.audio, index, &sample);
        if (result == WORLD_AUDIO_OK) result = worldAudioPauseVoice(bindings_.audio, sample, paused != 0);
    }
    return result;
}

WorldAudioResult CarSoundState::ScaleRoadNoise(float scale)
{
    for (auto& tire : tires_) if (tire) tire->ScaleRoadNoise(scale);
    return WORLD_AUDIO_OK;
}

bool CarSoundState::InSoundSphere() const
{
    return spatial.distance < 130.0f;
}

WorldAudioResult CarSoundState::DeactivateSound()
{
    if (!horn_ || !engine_) return WORLD_AUDIO_INVALID_BINDING;
    WorldAudioResult result = horn_->Activate(0);
    if (result == WORLD_AUDIO_OK) result = engine_->Activate(0);
    for (auto& tire : tires_) {
        if (result == WORLD_AUDIO_OK && tire) result = tire->Activate(0);
    }
    return result;
}

WorldAudioResult CarSoundState::ActivateSound()
{
    if (!engine_) return WORLD_AUDIO_INVALID_BINDING;
    WorldAudioResult result = engine_->Activate(1);
    for (auto& tire : tires_) {
        if (result == WORLD_AUDIO_OK && tire) result = tire->Activate(1);
    }
    return result;
}

Sound3d* CarSoundState::Engine() const { return engine_.get(); }
BaseTireSound* CarSoundState::Tire(int32_t side) const { return tires_[side != 0].get(); }

int32_t carSoundStartHorn(CarSoundState* sound)
{
    if (!sound || !sound->horn_) return 0;
    if (!sound->bindings_.globals->policy.audioSystemEnabled ||
        !sound->bindings_.globals->policy.vehicleAudioEnabled) return 1;
    return sound->horn_->Activate(1) == WORLD_AUDIO_OK;
}

int32_t carSoundStopHorn(CarSoundState* sound)
{
    if (!sound || !sound->horn_) return 0;
    if (!sound->bindings_.globals->policy.audioSystemEnabled ||
        !sound->bindings_.globals->policy.vehicleAudioEnabled) return 1;
    return sound->horn_->Activate(0) == WORLD_AUDIO_OK;
}

int32_t carSoundHornActive(const CarSoundState* sound)
{
    return sound && sound->horn_ && sound->bindings_.globals->policy.audioSystemEnabled &&
        sound->bindings_.globals->policy.vehicleAudioEnabled && sound->horn_->spatial.active != 0;
}

int32_t carSoundRefreshEffects(CarSoundState* sound)
{
    if (!sound) return 0;
    const CarSoundGlobals& globals = *sound->bindings_.globals;
    if (globals.policy.audioSystemEnabled && globals.policy.vehicleAudioEnabled && globals.tirePopEnabled) {
        if (worldAudioPlayVoice(sound->bindings_.audio, sound->tirePop_, sound->spatial.volume,
            static_cast<float>(sound->spatial.azimuth), sound->spatial.pitch) != WORLD_AUDIO_OK) return 0;
    }
    for (auto& tire : sound->tires_) if (tire) tire->TriggerTireBlowOut();
    return 1;
}

void carSoundSetSurfaceLevel(CarSoundState* sound, int32_t channel, int32_t level)
{
    if (!sound) std::abort(); // Required source Car+854 owner is absent.
    sound->splashLevel_[channel != 0] = level;
}

namespace {

AiCarSound::AiCarSound(CarMovement* movement, const CarSoundBindings& bindings)
    : CarSoundState(movement, bindings)
{
    envelope_ = &bindings.globals->aiEnvelope;
    WorldAudioResult result;
    engine_.reset(carSoundCreateAiEngine(movement, &bindings, &result));
    if (!engine_) throw std::runtime_error("AI car engine sound cannot be constructed");
    auto horn = std::make_unique<CarHornSound>();
    if (horn->Initialize(bindings.audio, bindings.globals, GetHornSample()) != WORLD_AUDIO_OK)
        throw std::runtime_error("AI car horn cannot initialize its actual sample");
    horn_ = std::move(horn);
    tires_[0].reset(carSoundCreateAiTire(movement, &bindings, &result));
    if (!tires_[0]) throw std::runtime_error("AI car tires cannot initialize actual samples");
}

AiCarSound::~AiCarSound() = default;

WorldAudioResult AiCarSound::Update(float volume, uint16_t azimuth, float pitch)
{
    if (!spatial.active) {
        return sound3dUpdateRelativeMotion(&spatial, &bindings_.spatial,
            movement_->bindings.state->position) == SOUND3D_OK ? WORLD_AUDIO_OK : WORLD_AUDIO_INVALID_BINDING;
    }
    WorldAudioResult result = CarSoundState::Update(volume * 1.5f, azimuth, pitch);
    if (result == WORLD_AUDIO_OK) result = engine_->Update(spatial.volume, spatial.azimuth, spatial.pitch);
    if (result == WORLD_AUDIO_OK) result = tires_[0]->Update(spatial.volume, spatial.azimuth, spatial.pitch);
    return result;
}

CarSoundState* createAiCarSound(CarMovement* movement, const CarSoundBindings* bindings,
    WorldAudioResult* result)
{
    if (!bindings) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    try {
        auto* sound = new AiCarSound(movement, *bindings);
        if (result) *result = WORLD_AUDIO_OK;
        return sound;
    } catch (const std::bad_alloc& failure) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_CAPACITY,"AI car sound",failure.what());
        if (result) *result = WORLD_AUDIO_CAPACITY;
    } catch (const std::exception& failure) {
        const auto status=WORLD_AUDIO_INVALID_BINDING;
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(status,"AI car sound",failure.what());
        if (result) *result = status;
    } catch (...) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_INVALID_BINDING,"AI car sound","Non-standard source exception");
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
    }
    return nullptr;
}

} // namespace

CarSoundState* carSoundCreateAi(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result)
{
    return createAiCarSound(movement, bindings, result);
}

CarSoundState* carSoundCreateAiCop(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result)
{
    return createAiCarSound(movement, bindings, result);
}

namespace {

// Final48-byte POVVolumeTable310090. Rows are the actual signed Player
// camera-view byte0..3; the three columns retain their source order.
float playerPovVolumes[4][3] = {
    {0.800000011920929f, 1.0f, 0.75f},
    {0.8999999761581421f, 0.8999999761581421f, 0.8500000238418579f},
    {1.0f, 0.75f, 1.0f},
    {1.25f, 0.4000000059604645f, 0.8999999761581421f}
};

struct PlayerCarAudioFailure { WorldAudioResult result; };

void requirePlayerCarAudio(WorldAudioResult result)
{
    if (result != WORLD_AUDIO_OK) throw PlayerCarAudioFailure{result};
}

CarFramePlayerState* playerCarOwner(const CarMovement* movement)
{
    if (!movement || !movement->frame) return nullptr;
    auto* player = movement->frame->environment.player;
    if (!player) return nullptr;
    return player->car == movement && movement->bindings.player == &player->movement ? player : nullptr;
}

float playerPovVolume(const CarMovement* movement, int32_t type)
{
    int32_t view = 0;
    if (movement->bindings.player) {
        const auto* player = playerCarOwner(movement);
        if (!player) throw PlayerCarAudioFailure{WORLD_AUDIO_INVALID_BINDING};
        view = player->settings.cameraView;
    }
    if (view < 0 || view >= 4) throw PlayerCarAudioFailure{WORLD_AUDIO_INVALID_BINDING};
    if (type == 0) return playerPovVolumes[view][0];
    if (type == 1) return playerPovVolumes[view][2];
    if (type == 2) return playerPovVolumes[view][1];
    return 1.0f;
}

const hp2::CarDefinition& playerCarDefinition(const CarMovement* movement,
    const CarSoundBindings& bindings)
{
    if (!movement || !movement->bindings.info || !bindings.cars || !bindings.cars->CarsLoaded())
        throw std::invalid_argument("Player car sound has no retained physical model");
    const int32_t model = movement->bindings.info->modelCode;
    if (model < 0 || model >= 64)
        throw std::invalid_argument("Player car sound model is outside the actual64-record resource");
    const auto& definition = bindings.cars->Car(static_cast<size_t>(model));
    if (movement->bindings.info->engine != &definition.engineInfo)
        throw std::invalid_argument("Player car sound has a different physical model owner");
    return definition;
}

void soundLocalPoint(CarMovement* movement, float output[2], const float point[2])
{
    VehicleState* state = movement->bindings.state;
    const float* frame = vehicleStateVelocityFrame(state);
    float transpose[16], transformed[4];
    vectorMatrixTranspose(transpose, frame);
    const float relative[4] = {point[0] - state->position[0], point[1] - state->position[1], 0.0f, 0.0f};
    vectorMatrixTransform(transformed, transpose, relative);
    output[0] = transformed[0];
    output[1] = transformed[1];
}

const float* soundBodyPosition(const CollisionShape* shape)
{
    if (shape->bodyMode) return shape->body ? shape->body->motion.position : nullptr;
    return shape->vehicle.position;
}

} // namespace

CarFramePlayerState* BasePlayerCarSound::Player() const
{
    return playerCarOwner(movement_);
}

BasePlayerCarSound::BasePlayerCarSound(CarMovement* movement, const CarSoundBindings& bindings)
    : CarSoundState(movement, bindings), topology_{}, wind_{},
      reflectionPending_{0, 0}, reflectionReady_(1), playerAzimuth_(0)
{
    if (!movement->frame || !movement->bindings.environment ||
        !movement->bindings.environment->contact.terrain ||
        !movement->bindings.environment->contact.terrain->topology ||
        !bindings.race || !bindings.race->services || !bindings.race->services->settings ||
        bindings.frame != &movement->frame->state || !bindings.spatial.players ||
        bindings.spatial.players->soundPolicy != &bindings.globals->policy)
        throw std::invalid_argument("Player car sound has no same-owner frame/topology/configuration");
    // PAL130BC0 clears Car.player before18EB28 constructs this sound owner.
    // Player attachment is required by the later Update consumer, not by
    // this constructor; enforcing it here prevented the actual Car lifetime.
    const auto& definition = playerCarDefinition(movement, bindings);
    // Traffic resources have no player .ctl or per-model sample bank. Keep
    // their InGame horn and the native player engine factory, which owns a
    // dedicated RPM/throttle loop on the player's engine-volume category.
    const bool traffic=definition.bodyCategory==2;
    const auto* modelBank = traffic ? nullptr : soundBank(*bindings.audio, definition.engineSound.c_str());
    topology_ = std::make_unique<TerrainSurface>();
    terrainSurfaceInitialize(topology_.get(), movement->bindings.environment->contact.terrain->topology);
    envelope_ = &bindings.globals->playerEnvelope;
    try {
        const auto* wind = worldAudioFindInGame(bindings.audio, "Wind");
        if (!wind) throw PlayerCarAudioFailure{WORLD_AUDIO_INVALID_BINDING};
        requirePlayerCarAudio(worldAudioInitializeVoiceFromCue(bindings.audio,
            bindings.audio->inGameBank, wind, &wind_));
        requirePlayerCarAudio(worldAudioSetVoiceCategory(bindings.audio, wind_, 0));
        auto horn = std::make_unique<CarHornSound>();
        requirePlayerCarAudio(horn->Initialize(bindings.audio, bindings.globals, GetHornSample(), modelBank));
        horn_ = std::move(horn);
        WorldAudioResult result{};
        engine_.reset(carSoundCreatePlayerEngine(movement, &bindings, &result));
        if (!engine_) throw PlayerCarAudioFailure{result == WORLD_AUDIO_OK ? WORLD_AUDIO_INVALID_BINDING : result};
        tires_[0].reset(carSoundCreatePlayerTire(movement, 0, &bindings, &result));
        if (!tires_[0]) throw PlayerCarAudioFailure{result == WORLD_AUDIO_OK ? WORLD_AUDIO_INVALID_BINDING : result};
        const int32_t mode = bindings.race->services->settings->mode;
        if (mode == 1) {
            tires_[1].reset(carSoundCreatePlayerTire(movement, 1, &bindings, &result));
            if (!tires_[1]) throw PlayerCarAudioFailure{result == WORLD_AUDIO_OK ? WORLD_AUDIO_INVALID_BINDING : result};
        }
        if (mode >= 2) {
            const int32_t driver = movement->bindings.owner->driverNumber;
            if (driver == 0) playerAzimuth_ = static_cast<uint16_t>(0u - bindings.globals->multiplayerAzimuth);
            else if (driver == 1) playerAzimuth_ = bindings.globals->multiplayerAzimuth;
        }
        requirePlayerCarAudio(Activate(1));
    } catch (...) {
        if (wind_.instance) worldAudioRetireVoice(bindings_.audio, wind_);
        terrainSurfaceDispose(topology_.get());
        throw;
    }
}

BasePlayerCarSound::~BasePlayerCarSound()
{
    if (wind_.instance) worldAudioStopInstance(bindings_.audio, wind_.instance);
    if (topology_) {
        terrainSurfaceDispose(topology_.get());
        topology_.reset();
    }
    if (wind_.instance) worldAudioRetireVoice(bindings_.audio, wind_);
}

const WorldAudioCue* BasePlayerCarSound::GetHornSample()
{
    const auto& definition = playerCarDefinition(movement_, bindings_);
    if(definition.bodyCategory==2)return nullptr; // Real InGame Horn01..03 chooser.
    const auto& prefix = definition.playerEngineSamplePrefix;
    const auto end = std::find(prefix.begin(), prefix.end(), '\0');
    if (end == prefix.end()) throw PlayerCarAudioFailure{WORLD_AUDIO_INVALID_BINDING};
    const std::string name = std::string(prefix.begin(), end) + "Horn";
    const auto* bank = soundBank(*bindings_.audio, definition.engineSound.c_str());
    return bank ? worldAudioFindInBank(bank, name.c_str()) : nullptr;
}

WorldAudioVoiceRef BasePlayerCarSound::GetEngineReflectionSample(int32_t index)
{
    if (index == -1) index = static_cast<int32_t>(vectorRandomBounded(2, &worldAudioRuntime.randomState));
    char name[] = "Reflection01";
    name[11] = static_cast<char>(static_cast<uint8_t>(static_cast<uint32_t>(index) + '1'));
    WorldAudioVoiceRef voice{};
    requirePlayerCarAudio(worldAudioAcquireInGameSample(bindings_.audio, name, &voice));
    return voice;
}

float BasePlayerCarSound::CalcWindVolume(float volume) const
{
    return (playerPovVolume(movement_, 2) * volume) * 0.012000000104308128f;
}

float BasePlayerCarSound::CalcWindPitch(float pitch) const
{
    return pitch * 0.00800000037997961f;
}

WorldAudioResult BasePlayerCarSound::Pause(int32_t paused)
{
    WorldAudioResult result = CarSoundState::Pause(paused);
    if (result != WORLD_AUDIO_OK) return result;
    try {
        requirePlayerCarAudio(worldAudioPauseVoice(bindings_.audio, wind_, paused != 0));
        requirePlayerCarAudio(worldAudioPauseVoice(bindings_.audio, GetEngineReflectionSample(0), paused != 0));
        requirePlayerCarAudio(worldAudioPauseVoice(bindings_.audio, GetEngineReflectionSample(1), paused != 0));
        const auto* player = Player();
        if (!player) return WORLD_AUDIO_INVALID_BINDING;
        if (player->cinematicVoice) {
            const WorldAudioVoiceRef voice = {player->cinematicVoice, player->cinematicVoiceGeneration};
            requirePlayerCarAudio(worldAudioPauseVoice(bindings_.audio, voice, paused != 0));
        }
        return WORLD_AUDIO_OK;
    } catch (const PlayerCarAudioFailure& failure) {
        return failure.result;
    }
}

WorldAudioResult BasePlayerCarSound::UpdateEngineReflection(float volume, uint16_t azimuth, float pitch)
{
    (void)azimuth;
    (void)pitch;
    try {
        if (reflectionPending_[0] || reflectionPending_[1]) {
            const float speed = movement_->bindings.state->speed;
            const float gain = speed * 0.029999999329447746f;
            const float offset = speed * 0.014999999664723873f;
            for (int32_t side = 0; side < 2; ++side) {
                if (!reflectionPending_[side]) continue;
                if (bindings_.globals->policy.audioSystemEnabled && bindings_.globals->policy.vehicleAudioEnabled) {
                    const auto voice = GetEngineReflectionSample(-1);
                    requirePlayerCarAudio(worldAudioPlayVoice(bindings_.audio, voice, gain * volume, 0.0f, offset - 1.0f));
                    const uint16_t angle = side == 0 ? static_cast<uint16_t>(0u - bindings_.globals->reflectionAzimuth) :
                        bindings_.globals->reflectionAzimuth;
                    requirePlayerCarAudio(worldAudioSetVoiceControlCode(bindings_.audio, voice, angle));
                }
                reflectionPending_[side] = 0;
            }
        }
        if (movement_->bindings.state->speed < 13.88888931274414f) {
            reflectionReady_ = 1;
            return WORLD_AUDIO_OK;
        }
        return UpdateDynamicBodyReflections();
    } catch (const PlayerCarAudioFailure& failure) {
        return failure.result;
    }
}

WorldAudioResult BasePlayerCarSound::UpdateDynamicBodyReflections()
{
    if (!movement_->collisionSystem || !movement_->collisionSystem->space ||
        !movement_->bindings.ownerMotion || !movement_->bindings.ownerMotion->collision)
        return WORLD_AUDIO_INVALID_BINDING;
    const CollisionSpace* space = movement_->collisionSystem->space;
    CollisionShape* own = movement_->bindings.ownerMotion->collision->shape;
    if (!own || !own->next || !own->previous) return WORLD_AUDIO_INVALID_BINDING;
    const float* ownPosition = soundBodyPosition(own);
    const float* ownVelocity = collisionShapeVelocity(own);
    if (!ownPosition || !ownVelocity) return WORLD_AUDIO_INVALID_BINDING;
    CollisionShape* before = nullptr;
    CollisionShape* after = nullptr;
    collisionShapeFindRange(space, own, &before, &after, 9.0f);
    if (!before || !after) return WORLD_AUDIO_INVALID_BINDING;
    for (CollisionShape* other = before->next; other != after; other = other->next) {
        if (!other || other == &space->root) return WORLD_AUDIO_INVALID_BINDING;
        if (other == own || other->kind == 28) continue;
        const float* otherPosition = soundBodyPosition(other);
        const float* otherVelocity = collisionShapeVelocity(other);
        if (!otherPosition || !otherVelocity) return WORLD_AUDIO_INVALID_BINDING;
        const float dx = otherPosition[0] - ownPosition[0];
        const float dy = otherPosition[1] - ownPosition[1];
        const float dz = otherPosition[2] - ownPosition[2];
        const float xx = dx * dx, yy = dy * dy, zz = dz * dz;
        if (9.0f < std::sqrt((xx + yy) + zz)) continue;
        float local[2];
        soundLocalPoint(movement_, local, otherPosition);
        const float vx = ownVelocity[0] - otherVelocity[0];
        const float vy = ownVelocity[1] - otherVelocity[1];
        const float vx2 = vx * vx, vy2 = vy * vy;
        if (!(13.88888931274414f < std::sqrt(vx2 + vy2))) continue;
        float ownFuture[2], otherFuture[2];
        vectorMultiplyAdd2(ownFuture, ownPosition, ownVelocity, bindings_.clock->worldTimeElapsed);
        vectorMultiplyAdd2(otherFuture, otherPosition, otherVelocity, bindings_.clock->worldTimeElapsed);
        const float futureDelta[2] = {otherFuture[0] - ownFuture[0], otherFuture[1] - ownFuture[1]};
        float futureDirection[2], ownDirection[2], currentDirection[2];
        vectorNormalize2(futureDirection, futureDelta);
        vectorNormalize2(ownDirection, ownVelocity);
        const float currentDelta[2] = {otherPosition[0] - ownPosition[0], otherPosition[1] - ownPosition[1]};
        vectorNormalize2(currentDirection, currentDelta);
        const float currentX = currentDirection[0] * ownDirection[0];
        const float currentY = currentDirection[1] * ownDirection[1];
        const float futureX = futureDirection[0] * ownDirection[0];
        const float futureY = futureDirection[1] * ownDirection[1];
        const float currentDot = currentX + currentY;
        const float futureDot = futureX + futureY;
        if ((currentDot < 0.0f && 0.0f < futureDot) || (0.0f < currentDot && futureDot < 0.0f))
            reflectionPending_[local[0] < 0.0f ? 0 : 1] = 1;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult BasePlayerCarSound::UpdateTopologyReflections()
{
    if (!topology_ || !movement_->bindings.state || !movement_->bindings.state->position)
        return WORLD_AUDIO_INVALID_BINDING;
    terrainSurfaceUpdate(topology_.get(), movement_->bindings.state->position, 7.0f, nullptr);
    const float rightMinimum[2] = {0.0f, -12.0f};
    const float rightMaximum[2] = {5.0f, 12.0f};
    const float leftMinimum[2] = {-5.0f, -12.0f};
    const float leftMaximum[2] = {0.0f, 12.0f};
    int32_t front = 0, middle = 0, back = 0, side = 0;
    for (size_t index = 0; index < topology_->barrierCount; ++index) {
        const size_t offset = topology_->groundCount + index;
        if (offset >= topology_->polygonCount || !topology_->polygons[offset]) return WORLD_AUDIO_INVALID_BINDING;
        const auto* polygon = topology_->polygons[offset];
        float first[2], second[2];
        soundLocalPoint(movement_, first, polygon->minimum);
        soundLocalPoint(movement_, second, polygon->maximum);
        const int32_t right = vectorBoundsContainsPoint2(rightMinimum, rightMaximum, first, 0.0f) &&
            vectorBoundsContainsPoint2(rightMinimum, rightMaximum, second, 0.0f);
        const int32_t left = vectorBoundsContainsPoint2(leftMinimum, leftMaximum, first, 0.0f) &&
            vectorBoundsContainsPoint2(leftMinimum, leftMaximum, second, 0.0f);
        if (!right && !left) continue;
        for (const float* point : {first, second}) {
            if (10.0f < point[1]) front = 1;
            else if (point[1] < -10.0f) back = 1;
            else { middle = 1; side = right ? 1 : -1; }
        }
        if (front || back) break;
    }
    if (reflectionReady_ && middle && !front && !back) {
        reflectionPending_[side == 1 ? 1 : 0] = 1;
        reflectionReady_ = 0;
    } else {
        reflectionReady_ = 1;
    }
    return WORLD_AUDIO_OK;
}

const TerrainSurface* BasePlayerCarSound::ReflectionSurface() const
{
    return topology_.get();
}

WorldAudioResult BasePlayerCarSound::Update(float volume, uint16_t azimuth, float pitch)
{
    auto result = CarSoundState::Update(volume, azimuth, pitch);
    if (result != WORLD_AUDIO_OK) return result;
    try {
        spatial.azimuth = playerAzimuth_;
        const float engineVolume = spatial.volume * playerPovVolume(movement_, 0);
        const float tireVolume = spatial.volume * playerPovVolume(movement_, 1);
        requirePlayerCarAudio(engine_->Update(engineVolume, spatial.azimuth, spatial.pitch));
        for (auto& tire : tires_)
            if (tire) requirePlayerCarAudio(tire->Update(tireVolume, spatial.azimuth, spatial.pitch));
        requirePlayerCarAudio(UpdateEngineReflection(spatial.volume, spatial.azimuth, spatial.pitch));
        auto* player = Player();
        if (!player) return WORLD_AUDIO_INVALID_BINDING;
        if (player->cinematicVoice && !player->transitionAudio) {
            carFramePlayerStopStateAudio(player);
            if (bindings_.globals->policy.audioSystemEnabled && player->cinematicVoice)
                return bindings_.audio->result == WORLD_AUDIO_OK ? WORLD_AUDIO_INVALID_BINDING : bindings_.audio->result;
        }
        return WORLD_AUDIO_OK;
    } catch (const PlayerCarAudioFailure& failure) {
        return failure.result;
    }
}

CarSoundState* carSoundCreatePlayer(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result)
{
    if (!bindings) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    try {
        auto* sound = new BasePlayerCarSound(movement, *bindings);
        if (result) *result = WORLD_AUDIO_OK;
        return sound;
    } catch (const PlayerCarAudioFailure& failure) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(failure.result,"Player car sound","Source audio operation rejected");
        if (result) *result = failure.result;
    } catch (const std::bad_alloc& failure) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_CAPACITY,"Player car sound",failure.what());
        if (result) *result = WORLD_AUDIO_CAPACITY;
    } catch (const std::exception& failure) {
        const auto status=WORLD_AUDIO_INVALID_BINDING;
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(status,"Player car sound",failure.what());
        if (result) *result = status;
    } catch (...) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_INVALID_BINDING,"Player car sound","Non-standard source exception");
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
    }
    return nullptr;
}

namespace {

// Actual closest-sound pointer view3100C0 and GP-4FB0/-4FAC. It never owns
// cars or sound components and intentionally replaces the first farther
// candidate; it is not a different sort/priority algorithm.
struct CarSoundSchedulingState {
    std::array<CarSoundState*, 6> closest{};
    int32_t limit = 0;
    int32_t inserted = 0;
};
CarSoundSchedulingState carSoundScheduling;

void maybeInsertCarSound(CarSoundState* sound)
{
    if (!sound->InSoundSphere()) return;
    for (int32_t index = 0; index < carSoundScheduling.limit; ++index) {
        auto& slot = carSoundScheduling.closest[static_cast<size_t>(index)];
        if (!slot) { slot = sound; ++carSoundScheduling.inserted; return; }
    }
    for (int32_t index = 0; index < carSoundScheduling.limit; ++index) {
        auto& slot = carSoundScheduling.closest[static_cast<size_t>(index)];
        if (sound->spatial.distance < slot->spatial.distance) { slot = sound; return; }
    }
}

WorldAudioResult maybeActivateCarSound(CarSoundState* sound)
{
    int32_t active = 0;
    for (int32_t index = 0; index < carSoundScheduling.limit; ++index)
        if (carSoundScheduling.closest[static_cast<size_t>(index)] == sound) { active = 1; break; }
    return sound->Activate(active);
}

} // namespace

WorldAudioResult carSoundUpdateAll(const WorldSimulation* simulation)
{
    if (!simulation) return WORLD_AUDIO_INVALID_BINDING;
    if (!simulation->world) return WORLD_AUDIO_OK; // Exact absent-world source branch18EA28.
    if (!simulation->race || !simulation->race->services || !simulation->race->services->settings ||
        !simulation->frames) return WORLD_AUDIO_INVALID_BINDING;
    carSoundScheduling.limit = simulation->race->services->settings->mode > 1 ? 3 : 6;
    const auto movementFor = [&](const Car* car) -> CarMovement* {
        if (!car->movement || car->tableIndex >= simulation->frameCount) return nullptr;
        auto* movement = static_cast<CarMovement*>(car->movement);
        const CarFrame* frame = simulation->frames[car->tableIndex];
        if (!frame || frame->movement != movement || movement->frame != frame || movement->bindings.owner != car)
            return nullptr;
        return movement;
    };
    World* world = simulation->world;
    for (Car* car = world->carList.next; car != &world->carList; car = car->next) {
        auto* movement = movementFor(car);
        if (!movement || !carMovementUpdateSound(movement)) return WORLD_AUDIO_INVALID_BINDING;
    }
    for (int32_t index = 0; index < carSoundScheduling.limit; ++index)
        carSoundScheduling.closest[static_cast<size_t>(index)] = nullptr;
    carSoundScheduling.inserted = 0;
    for (Car* car = world->carList.next; car != &world->carList; car = car->next) {
        if (!car->driverInfo) return WORLD_AUDIO_INVALID_BINDING;
        if (car->driverInfo->driverClass == 1) continue;
        auto* movement = movementFor(car);
        if (!movement || !movement->frame->environment.sound) return WORLD_AUDIO_INVALID_BINDING;
        maybeInsertCarSound(movement->frame->environment.sound);
    }
    for (Car* car = world->carList.next; car != &world->carList; car = car->next) {
        if (!car->driverInfo) return WORLD_AUDIO_INVALID_BINDING;
        if (car->driverInfo->driverClass == 1) continue;
        auto* movement = movementFor(car);
        if (!movement || !movement->frame->environment.sound) return WORLD_AUDIO_INVALID_BINDING;
        const auto result = maybeActivateCarSound(movement->frame->environment.sound);
        if (result != WORLD_AUDIO_OK) return result;
    }
    return WORLD_AUDIO_OK;
}
