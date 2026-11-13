#include "CarSounds.hpp"
#include "bsound/include/bSound.h"
#include "indep/inc/TerrainType.hpp"
#include "indep/inc/Car.hpp"
#include "CarSounds.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Car.hpp"
#include "Track.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

WorldAudioResult initializeTireSample(const CarSoundBindings& bindings,
    const char* name, uint32_t category, WorldAudioVoiceRef* voice)
{
    const WorldAudioCue* cue = worldAudioFindInGame(bindings.audio, name);
    if (!cue) return WORLD_AUDIO_INVALID_BINDING;
    WorldAudioResult result = worldAudioInitializeVoiceFromCue(bindings.audio,
        bindings.audio->inGameBank, cue, voice);
    if (result == WORLD_AUDIO_OK)
        result = worldAudioSetVoiceCategory(bindings.audio, *voice, category);
    return result;
}

float approachTireVolume(float current, float target)
{
    const float step = 0.3499999940395355f;
    const float upper = current + step;
    if (upper < target) return upper;
    const float lower = current - step;
    return target < lower ? lower : target;
}

class AiTireSound final : public BaseTireSound {
public:
    AiTireSound(CarMovement* movement, const CarSoundBindings& bindings);
    ~AiTireSound() override;
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Pause(int32_t paused) override;

protected:
    WorldAudioResult ActivateSound() override;
    WorldAudioResult DeactivateSound() override;

private:
    WorldAudioResult UpdateSkids(float volume, uint16_t azimuth, float pitch);
    CarMovement* movement_;
    WorldAudioVoiceRef skids_[2];
    int32_t current_;
    float skidVolume_;
    float targetVolume_;
};

} // namespace

BaseTireSound::BaseTireSound(const CarSoundBindings& bindings)
    : bindings_(bindings), previousContact_{1, 1}, impact_{}
{
    if (!bindings.audio || !bindings.globals)
        throw std::invalid_argument("Tire sound has no live audio owner");
    try {
        if (initializeTireSample(bindings_, "TireFlop", 1, &impact_[0]) != WORLD_AUDIO_OK ||
            initializeTireSample(bindings_, "BottomOut", 1, &impact_[1]) != WORLD_AUDIO_OK)
            throw std::runtime_error("Tire impact samples are absent from the actual bank");
    } catch (...) {
        for (int32_t index = 1; index >= 0; --index)
            if (impact_[index].instance) worldAudioRetireVoice(bindings_.audio, impact_[index]);
        throw;
    }
}

BaseTireSound::~BaseTireSound()
{
    for (int32_t index = 1; index >= 0; --index)
        if (impact_[index].instance) worldAudioRetireVoice(bindings_.audio, impact_[index]);
}

WorldAudioResult BaseTireSound::Pause(int32_t paused)
{
    WorldAudioResult result = worldAudioPauseVoice(bindings_.audio, impact_[0], paused != 0);
    if (result == WORLD_AUDIO_OK)
        result = worldAudioPauseVoice(bindings_.audio, impact_[1], paused != 0);
    return result;
}

WorldAudioResult BaseTireSound::UpdateTireImpact(const CarMovement* movement, int32_t side)
{
    if (!movement || !movement->bindings.state) return WORLD_AUDIO_INVALID_BINDING;
    const VehicleState* state = movement->bindings.state;
    int32_t front = 0;
    int32_t rear = 0;
    if (side == -1) {
        front = state->wheelContact[0] != 0 || state->wheelContact[1] != 0;
        rear = state->wheelContact[2] != 0 || state->wheelContact[3] != 0;
    } else if (side == 0) {
        front = state->wheelContact[1];
        rear = state->wheelContact[3];
    } else if (side == 1) {
        front = state->wheelContact[0];
        rear = state->wheelContact[2];
    }
    WorldAudioResult result = WORLD_AUDIO_OK;
    if ((front && !previousContact_[0]) || (rear && !previousContact_[1])) {
        const float vertical = std::fabs(state->velocity[2]) * 0.30000001192092896f;
        const float x = state->angularVelocity[0] * state->angularVelocity[0];
        const float y = state->angularVelocity[1] * state->angularVelocity[1];
        const float angular = std::sqrt(x + y) * 0.800000011920929f;
        float strength = (vertical + angular) * 0.5f;
        const float limited = strength < 1.0f ? strength : 1.0f;
        const CarSoundGlobals& globals = *bindings_.globals;
        if (globals.policy.audioSystemEnabled && globals.policy.vehicleAudioEnabled && globals.splashEnabled) {
            result = worldAudioPlayVoice(bindings_.audio, impact_[0], limited * spatial.volume, 0.0f, 0.0f);
            if (result == WORLD_AUDIO_OK)
                result = worldAudioSetVoiceControlCode(bindings_.audio, impact_[0], spatial.azimuth);
            if (result != WORLD_AUDIO_OK) return result;
        }
        if (0.8999999761581421f < strength && globals.policy.audioSystemEnabled &&
            globals.policy.vehicleAudioEnabled && globals.splashEnabled) {
            const float pitch = vectorRandomFloat(&worldAudioRuntime.randomState,
                0.699999988079071f) - 0.4000000059604645f;
            if (impact_[1].instance->category == UINT32_MAX) {
                if (!bindings_.volumes) return WORLD_AUDIO_INVALID_BINDING;
                strength = strength * hp2::GetVolumeDefault(*bindings_.volumes, 0);
            }
            const float gain = (strength * spatial.volume) * 0.800000011920929f;
            result = worldAudioPlayVoice(bindings_.audio, impact_[1], gain, 0.0f, pitch);
            if (result == WORLD_AUDIO_OK)
                result = worldAudioSetVoiceControlCode(bindings_.audio, impact_[1], spatial.azimuth);
            if (result != WORLD_AUDIO_OK) return result;
        }
    }
    previousContact_[1] = static_cast<uint16_t>(rear);
    previousContact_[0] = static_cast<uint16_t>(front);
    return result;
}

void BaseTireSound::ScaleRoadNoise(float scale)
{
    (void)scale; // The actual base virtual method is empty.
}

void BaseTireSound::TriggerTireBlowOut()
{
    // The actual base virtual method is empty; PlayerTireSound overrides it.
}

namespace {

AiTireSound::AiTireSound(CarMovement* movement, const CarSoundBindings& bindings)
    : BaseTireSound(bindings), movement_(movement), skids_{}, current_(0),
      skidVolume_(0.0f), targetVolume_(0.0f)
{
    if (!movement || !movement->bindings.state)
        throw std::invalid_argument("AI tire sound has no actual vehicle state");
    try {
        if (initializeTireSample(bindings_, "LongitudinalSkid", 0, &skids_[0]) != WORLD_AUDIO_OK ||
            initializeTireSample(bindings_, "GravelSkid", 0, &skids_[1]) != WORLD_AUDIO_OK ||
            Activate(1) != WORLD_AUDIO_OK)
            throw std::runtime_error("AI tire sound cannot initialize its actual samples");
    } catch (...) {
        for (int32_t index = 1; index >= 0; --index)
            if (skids_[index].instance) worldAudioRetireVoice(bindings_.audio, skids_[index]);
        throw;
    }
}

AiTireSound::~AiTireSound()
{
    for (int32_t index = 1; index >= 0; --index)
        if (skids_[index].instance) worldAudioRetireVoice(bindings_.audio, skids_[index]);
}

WorldAudioResult AiTireSound::ActivateSound()
{
    return WORLD_AUDIO_OK; // Original actual virtual body is JR RA / NOP.
}

WorldAudioResult AiTireSound::DeactivateSound()
{
    WorldAudioResult result = worldAudioStopInstance(bindings_.audio, skids_[0].instance);
    if (result == WORLD_AUDIO_OK)
        result = worldAudioStopInstance(bindings_.audio, skids_[1].instance);
    if (result == WORLD_AUDIO_OK)
        result = worldAudioStopInstance(bindings_.audio, skids_[current_].instance);
    return result;
}

WorldAudioResult AiTireSound::Pause(int32_t paused)
{
    WorldAudioResult result = BaseTireSound::Pause(paused);
    if (result == WORLD_AUDIO_OK) result = worldAudioPauseVoice(bindings_.audio, skids_[0], paused != 0);
    if (result == WORLD_AUDIO_OK) result = worldAudioPauseVoice(bindings_.audio, skids_[1], paused != 0);
    return result;
}

WorldAudioResult AiTireSound::Update(float volume, uint16_t azimuth, float pitch)
{
    if (!spatial.active) return WORLD_AUDIO_OK;
    spatial.volume = volume;
    spatial.azimuth = azimuth;
    const WorldAudioResult result = UpdateSkids(volume, azimuth, pitch);
    if (result != WORLD_AUDIO_OK) return result;
    spatial.volume = spatial.volume * 0.75f;
    return UpdateTireImpact(movement_, -1);
}

WorldAudioResult AiTireSound::UpdateSkids(float volume, uint16_t azimuth, float pitch)
{
    (void)pitch;
    const VehicleState* state = movement_->bindings.state;
    const TerrainMaterial* materials = terrainDefaultMaterials(nullptr);
    float maximumSlip = 0.0f;
    int32_t roadWheels = 0;
    for (int32_t wheel = 0; wheel < 4; ++wheel) {
        float slip = 0.0f;
        if (state->wheelContact[wheel]) {
            const float x = state->wheelSlip[wheel][0] * state->wheelSlip[wheel][0];
            const float y = state->wheelSlip[wheel][1] * state->wheelSlip[wheel][1];
            slip = std::sqrt(x + y);
        }
        if (slip < 0.25f) slip = 0.0f;
        maximumSlip = maximumSlip < slip ? slip : maximumSlip;
        const int32_t material = state->material[wheel];
        if (material < 0 || material >= 33) return WORLD_AUDIO_INVALID_BINDING;
        if (static_cast<int8_t>(materials[material].classification & 255u)) ++roadWheels;
    }
    const bool gravel = roadWheels < 2;
    const float skidPitch = state->speed * 0.0101500004529953f + 0.30000001192092896f;
    targetVolume_ = volume * (gravel ? 3.5f : 5.0f);
    if (maximumSlip < 10.0f) targetVolume_ = 0.0f;
    skidVolume_ = approachTireVolume(skidVolume_, targetVolume_);
    const int32_t selected = gravel ? 1 : 0;
    WorldAudioResult result = WORLD_AUDIO_OK;
    if (current_ != selected && skids_[current_].instance->playing) {
        result = worldAudioStopInstance(bindings_.audio, skids_[current_].instance);
        if (result != WORLD_AUDIO_OK) return result;
    }
    const auto voice = skids_[selected];
    if (bindings_.globals->policy.audioSystemEnabled && bindings_.globals->policy.vehicleAudioEnabled) {
        if (!voice.instance->playing && 0.0f < skidVolume_)
            result = worldAudioPlayVoice(bindings_.audio, voice, skidVolume_, 0.0f, skidPitch - 1.0f);
        if (result != WORLD_AUDIO_OK) return result;
        if (voice.instance->playing && skidVolume_ <= 0.0f)
            result = worldAudioStopInstance(bindings_.audio, voice.instance);
        if (result != WORLD_AUDIO_OK) return result;
    }
    if (0.0f < skidVolume_) {
        result = worldAudioSetVoiceVolume(bindings_.audio, voice, skidVolume_);
        if (result == WORLD_AUDIO_OK)
            result = worldAudioSetVoicePitchOffset(bindings_.audio, voice, skidPitch - 1.0f);
        if (result == WORLD_AUDIO_OK)
            result = worldAudioSetVoiceControlCode(bindings_.audio, voice, azimuth);
    } else {
        result = worldAudioStopInstance(bindings_.audio, voice.instance);
    }
    current_ = selected;
    return result;
}

} // namespace

BaseTireSound* carSoundCreateAiTire(CarMovement* movement,
    const CarSoundBindings* bindings, WorldAudioResult* result)
{
    if (!bindings) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    try {
        auto* tire = new AiTireSound(movement, *bindings);
        if (result) *result = WORLD_AUDIO_OK;
        return tire;
    } catch (const std::bad_alloc& failure) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_CAPACITY,"AI tire sound",failure.what());
        if (result) *result = WORLD_AUDIO_CAPACITY;
    } catch (const std::exception& failure) {
        const auto status=WORLD_AUDIO_INVALID_BINDING;
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(status,"AI tire sound",failure.what());
        if (result) *result = status;
    } catch (...) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_INVALID_BINDING,"AI tire sound","Non-standard source exception");
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
    }
    return nullptr;
}

namespace {

const WorldAudioBank* findTireTrackBank(const WorldAudioRequestQueue& audio, const char* name)
{
    if (audio.bankCount && !audio.banks)
        throw std::invalid_argument("Track sound-bank catalogue has no retained owner");
    for (size_t index = 0; index < audio.bankCount; ++index)
        if (audio.banks[index].name && std::strcmp(audio.banks[index].name, name) == 0)
            return &audio.banks[index];
    return nullptr;
}

struct TireAudioFailure { WorldAudioResult result; };

void requireTireAudio(WorldAudioResult result)
{
    if (result != WORLD_AUDIO_OK) throw TireAudioFailure{result};
}

// PAL GP-3F78/3F74/3F68/-3420: one live owner shared by both tire objects.
// Both TerrainOneShot members use the same cooldown, not per-car copies.
struct TireSoundGlobals {
    int32_t roadNoiseEnabled;
    float oneShotDelay;
    float noisePitchScale;
    uint16_t thumperAzimuth;
};
TireSoundGlobals tireSoundGlobals = {1, 0.0f, 0.006499999668449163f, 0x2000};

class TerrainOneShot {
public:
    explicit TerrainOneShot(const CarSoundBindings& bindings)
        : bindings_(bindings), voice_{}, delay_(nullptr), side_(0), entered_(0) {}
    ~TerrainOneShot();
    void Initialize(const WorldAudioBank* bank, const WorldAudioCue* cue,
        float* delay, int32_t side);
    WorldAudioResult Pause(int32_t paused);
    WorldAudioResult Update(const CarMovement* movement, int32_t terrain,
        float volume, uint16_t azimuth, float pitch);
private:
    const CarSoundBindings& bindings_;
    WorldAudioVoiceRef voice_;
    float* delay_;
    int32_t side_;
    int32_t entered_;
};

TerrainOneShot::~TerrainOneShot()
{
    if (voice_.instance) worldAudioRetireVoice(bindings_.audio, voice_);
}

void TerrainOneShot::Initialize(const WorldAudioBank* bank, const WorldAudioCue* cue,
    float* delay, int32_t side)
{
    if (!cue) return; // The source Init(NULL) leaves this concrete object inactive.
    requireTireAudio(worldAudioInitializeVoiceFromCue(bindings_.audio, bank, cue, &voice_));
    requireTireAudio(worldAudioSetVoiceCategory(bindings_.audio, voice_, 1));
    delay_ = delay;
    side_ = side;
    entered_ = 0;
}

WorldAudioResult TerrainOneShot::Pause(int32_t paused)
{
    return voice_.instance ? worldAudioPauseVoice(bindings_.audio, voice_, paused != 0) : WORLD_AUDIO_OK;
}

WorldAudioResult TerrainOneShot::Update(const CarMovement* movement, int32_t terrain,
    float volume, uint16_t azimuth, float pitch)
{
    if (!voice_.instance) return WORLD_AUDIO_OK;
    const auto& state = *movement->bindings.state;
    const int32_t first = side_ == 0 ? 0 : 3;
    const int32_t second = side_ == 0 ? 1 : 2;
    if (state.peakMaterial[first] != terrain && state.peakMaterial[second] != terrain) {
        entered_ = 0;
        return WORLD_AUDIO_OK;
    }
    if (entered_ || (delay_ && 0.0f < *delay_)) return WORLD_AUDIO_OK;
    const auto result = worldAudioPlayVoice(bindings_.audio, voice_, volume,
        static_cast<float>(azimuth), pitch - 1.0f);
    if (result != WORLD_AUDIO_OK) return result;
    entered_ = 1;
    if (delay_) *delay_ = 0.30000001192092896f;
    return WORLD_AUDIO_OK;
}

class PlayerTireSound final : public BaseTireSound {
public:
    PlayerTireSound(CarMovement* movement, int32_t side, const CarSoundBindings& bindings);
    ~PlayerTireSound() override;
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Pause(int32_t paused) override;
    void ScaleRoadNoise(float scale) override;
    void TriggerTireBlowOut() override;
protected:
    WorldAudioResult ActivateSound() override;
    WorldAudioResult DeactivateSound() override;
private:
    enum Voice { Leaves, RoadNoise, GravelNoise, LongitudinalSkid, GravelSkid,
        SquallSkid, SecondLongitudinalSkid, BurnOut, TireFlat, MetalScrape, VoiceCount };
    void SetSamplePitch(WorldAudioVoiceRef voice, float pitch);
    void CalcTireSquallVolumeAndPitch(float& volume, float& pitch);
    void CalcTireSkidVolumeAndPitch(float& volume, float& pitch);
    float CalcTireNoiseVolume() const;
    float CalcTireNoisePitch() const;
    float CalcTireSquallVolume() const;
    float CalcTireSquallPitch() const;
    void UpdateSquall(int32_t road, float volume, uint16_t azimuth, float pitch);
    void UpdateLeaves(float volume, uint16_t azimuth, float pitch);
    void ResetThumperTime();
    void UpdateThumperTires();
    void Copy(Voice which, const char* name);
    void Stop(Voice which);
    bool Playing(Voice which) const;
    void Play(Voice which, float volume, float pan, float pitch);
    void Volume(Voice which, float volume);
    void Azimuth(Voice which, uint16_t azimuth);
    CarMovement* movement_;
    int32_t side_;
    float skidPitch_;
    float squallPitch_;
    float squallVolume_;
    float roadNoiseScale_;
    float thumperRemaining_;
    int32_t thumperRequested_;
    int32_t road_;
    std::array<WorldAudioVoiceRef, VoiceCount> voices_{};
    TerrainOneShot metalBump_;
    TerrainOneShot tireFlop_;
};

void PlayerTireSound::Copy(Voice which, const char* name)
{
    requireTireAudio(initializeTireSample(bindings_, name, 1, &voices_[which]));
}

void PlayerTireSound::Stop(Voice which)
{
    if (voices_[which].instance)
        requireTireAudio(worldAudioStopInstance(bindings_.audio, voices_[which].instance));
}

bool PlayerTireSound::Playing(Voice which) const
{
    return voices_[which].instance && voices_[which].instance->playing;
}

void PlayerTireSound::Play(Voice which, float volume, float pan, float pitch)
{
    requireTireAudio(worldAudioPlayVoice(bindings_.audio, voices_[which], volume, pan, pitch));
}

void PlayerTireSound::Volume(Voice which, float volume)
{
    requireTireAudio(worldAudioSetVoiceVolume(bindings_.audio, voices_[which], volume));
}

void PlayerTireSound::Azimuth(Voice which, uint16_t azimuth)
{
    requireTireAudio(worldAudioSetVoiceControlCode(bindings_.audio, voices_[which], azimuth));
}

PlayerTireSound::PlayerTireSound(CarMovement* movement, int32_t side, const CarSoundBindings& bindings)
    : BaseTireSound(bindings), movement_(movement), side_(side), skidPitch_(-1.0f),
      squallPitch_(1.0f), squallVolume_(0.0f), roadNoiseScale_(1.0f),
      thumperRemaining_(0.0f), thumperRequested_(0), road_(1),
      metalBump_(bindings_), tireFlop_(bindings_)
{
    if (!movement || !movement->bindings.state || !bindings.clock || !bindings.frame ||
        !bindings.manager || !bindings.trackInfo || (side != 0 && side != 1))
        throw std::invalid_argument("Player tires require their actual car/frame/track/audio owners");
    const char* name = bindings.trackInfo->tireBankName;
    if (!std::memchr(name, 0, sizeof(bindings.trackInfo->tireBankName)))
        throw std::invalid_argument("Player tire track bank has no bounded name");
    if (findTireTrackBank(*bindings.audio, name) != bindings.trackBank)
        throw std::invalid_argument("Player tire track bank is not from the retained34200 record");
    try {
        Copy(RoadNoise, "RoadNoise");
        Copy(GravelNoise, "GravelNoise");
        Copy(LongitudinalSkid, "LongitudinalSkid");
        Copy(GravelSkid, "GravelSkid");
        Copy(SquallSkid, "SquallSkid");
        Copy(TireFlat, "TireFlat");
        Copy(MetalScrape, "CrashMetalScrape");
        Copy(BurnOut, "BurnOutPeel");
        Copy(SecondLongitudinalSkid, "GravelSkid");
        requireTireAudio(worldAudioSetVoiceCategory(bindings.audio, impact_[1], UINT32_MAX));
        if (bindings.trackBank) {
            const auto* cue = worldAudioFindInBank(bindings.trackBank, "Leaves");
            if (cue) {
                requireTireAudio(worldAudioInitializeVoiceFromCue(bindings.audio, bindings.trackBank, cue, &voices_[Leaves]));
                requireTireAudio(worldAudioSetVoiceCategory(bindings.audio, voices_[Leaves], 1));
            }
        }
        metalBump_.Initialize(bindings.audio->inGameBank,
            worldAudioFindInGame(bindings.audio, "MetalBump"), &tireSoundGlobals.oneShotDelay, side);
        tireFlop_.Initialize(bindings.audio->inGameBank,
            worldAudioFindInGame(bindings.audio, "TireFlop"), &tireSoundGlobals.oneShotDelay, side);
        if (bindings.globals->policy.audioSystemEnabled && bindings.globals->policy.vehicleAudioEnabled &&
            tireSoundGlobals.roadNoiseEnabled)
            Play(RoadNoise, 0.0f, 0.0f, 0.0f);
        ResetThumperTime();
        requireTireAudio(Activate(1));
    } catch (...) {
        for (auto& voice : voices_)
            if (voice.instance) worldAudioRetireVoice(bindings_.audio, voice);
        throw;
    }
}

PlayerTireSound::~PlayerTireSound()
{
    // PAL212D5C..212DAC stops these10 actual samples before its SDK thread
    // scheduling boundary2783B0. Windows yields its current thread here;
    // this is not a second audio clock or an emulated sound-driver thread.
    static constexpr Voice stopOrder[] = {RoadNoise, GravelNoise, LongitudinalSkid,
        GravelSkid, SquallSkid, SecondLongitudinalSkid, BurnOut, TireFlat, MetalScrape, Leaves};
    for (Voice which : stopOrder)
        if (voices_[which].instance) worldAudioStopInstance(bindings_.audio, voices_[which].instance);
    std::this_thread::yield();
    static constexpr Voice retireOrder[] = {MetalScrape, TireFlat, BurnOut,
        SecondLongitudinalSkid, SquallSkid, GravelSkid, LongitudinalSkid,
        GravelNoise, RoadNoise, Leaves};
    for (Voice which : retireOrder)
        if (voices_[which].instance) worldAudioRetireVoice(bindings_.audio, voices_[which]);
}

void PlayerTireSound::SetSamplePitch(WorldAudioVoiceRef voice, float pitch)
{
    requireTireAudio(worldAudioSetVoicePitchOffset(bindings_.audio, voice, pitch - 1.0f));
}

float PlayerTireSound::CalcTireNoiseVolume() const
{
    const auto& state = *movement_->bindings.state;
    const int32_t first = side_ == 1 ? 0 : 1;
    const int32_t second = side_ == 1 ? 3 : 2;
    float a = 0.0f, b = 0.0f;
    if (state.wheelContact[first]) a = std::fabs(state.wheelAngularVelocity[first]) * 0.009499999694526196f;
    if (state.wheelContact[second]) b = std::fabs(state.wheelAngularVelocity[second]) * 0.009499999694526196f;
    const float value = (a + b) * 0.5f;
    return 1.0f < value ? 1.0f : value;
}

float PlayerTireSound::CalcTireNoisePitch() const
{
    const auto& state = *movement_->bindings.state;
    const int32_t first = side_ == 1 ? 0 : 1;
    const int32_t second = side_ == 1 ? 3 : 2;
    const float a = state.wheelContact[first] ? std::fabs(state.wheelAngularVelocity[first]) : 0.0f;
    const float b = state.wheelContact[second] ? std::fabs(state.wheelAngularVelocity[second]) : 0.0f;
    return (a < b ? b : a) * tireSoundGlobals.noisePitchScale + (side_ == 0 ? 0.15000000596046448f : 0.0f);
}

float PlayerTireSound::CalcTireSquallVolume() const
{
    const auto& state = *movement_->bindings.state;
    if (std::fabs(state.speed) < 11.0f) return 0.0f;
    const int32_t first = side_ == 0 ? 0 : 1;
    const int32_t second = side_ == 0 ? 3 : 2;
    const float a = state.wheelContact[first] ? std::fabs(state.wheelSlipRatio[first]) : 0.0f;
    const float b = state.wheelContact[second] ? std::fabs(state.wheelSlipRatio[second]) : 0.0f;
    const float slip = (a + b) * 0.5f;
    float volume = 0.0f;
    if (0.699999988079071f < slip) volume = slip * 1.975000023841858f;
    return 1.0f < volume ? 1.0f : volume;
}

float PlayerTireSound::CalcTireSquallPitch() const
{
    // The original contact/slip loads are overwritten before the return;
    // the resulting value depends on sourceCar+3C4 and this side alone.
    return movement_->bindings.state->speed * 0.0101500004529953f +
        (side_ == 0 ? 0.15000000596046448f : 0.0f);
}

void PlayerTireSound::CalcTireSquallVolumeAndPitch(float& volume, float& pitch)
{
    const float desired = CalcTireSquallVolume();
    squallPitch_ = CalcTireSquallPitch();
    if (desired < squallVolume_) {
        const float next = squallVolume_ - 0.15000000596046448f;
        squallVolume_ = desired < next ? next : desired;
    } else if (squallVolume_ < desired) {
        const float next = squallVolume_ + 0.07500000298023224f;
        squallVolume_ = next < desired ? next : desired;
    }
    volume = squallVolume_;
    pitch = squallPitch_;
}

void PlayerTireSound::CalcTireSkidVolumeAndPitch(float& volume, float& pitch)
{
    const auto& state = *movement_->bindings.state;
    const int32_t first = side_ == 1 ? 2 : 0;
    const auto slip = [&](int32_t wheel) {
        float value = 0.0f;
        if (state.wheelContact[wheel]) {
            const float x = state.wheelSlip[wheel][0] * state.wheelSlip[wheel][0];
            const float y = state.wheelSlip[wheel][1] * state.wheelSlip[wheel][1];
            value = std::sqrt(x + y);
        }
        return value < 0.25f ? 0.0f : value;
    };
    const float sum = slip(first) + slip(first + 1);
    const float mean = static_cast<float>(static_cast<double>(sum) * 0.5);
    volume = 0.0f;
    pitch = 1.0f;
    if (side_ == 1) {
        if (mean < 0.009999999776482582f) { skidPitch_ = -1.0f; return; }
        volume = mean * 0.30000001192092896f;
        if (skidPitch_ != -1.0f) { pitch = skidPitch_; return; }
        const float proposed = (pitch * 0.05999999865889549f) * mean;
        pitch = proposed < 1.0f ? 1.0f : 1.2000000476837158f < proposed ? 1.2000000476837158f : proposed;
        skidPitch_ = pitch;
    } else {
        volume = mean * 0.4000000059604645f;
        const float proposed = (pitch * 0.07000000029802322f) * mean;
        pitch = proposed < 1.0f ? 1.0f : 1.25f < proposed ? 1.25f : proposed;
    }
}

WorldAudioResult PlayerTireSound::Pause(int32_t paused)
{
    WorldAudioResult result = BaseTireSound::Pause(paused);
    const Voice order[] = {RoadNoise, GravelNoise, LongitudinalSkid, GravelSkid,
        SquallSkid, SecondLongitudinalSkid, BurnOut, TireFlat, MetalScrape, Leaves};
    for (Voice which : order) {
        if (result == WORLD_AUDIO_OK && voices_[which].instance)
            result = worldAudioPauseVoice(bindings_.audio, voices_[which], paused != 0);
    }
    if (result == WORLD_AUDIO_OK) result = metalBump_.Pause(paused);
    if (result == WORLD_AUDIO_OK) result = tireFlop_.Pause(paused);
    return result;
}

WorldAudioResult PlayerTireSound::ActivateSound()
{
    return WORLD_AUDIO_OK; // Actual produced vtable target212E90 is JR RA/NOP.
}

WorldAudioResult PlayerTireSound::DeactivateSound()
{
    return WORLD_AUDIO_OK; // Actual produced vtable target212E88 is JR RA/NOP.
}

void PlayerTireSound::ScaleRoadNoise(float scale)
{
    roadNoiseScale_ = scale;
}

void PlayerTireSound::TriggerTireBlowOut()
{
    thumperRequested_ = 1;
}

void PlayerTireSound::ResetThumperTime()
{
    thumperRemaining_ = 0.0f;
    if (Playing(MetalScrape)) Stop(MetalScrape);
}

void PlayerTireSound::UpdateThumperTires()
{
    thumperRemaining_ -= bindings_.clock->worldTimeElapsed;
    if (thumperRemaining_ <= 0.0f) {
        const auto& state = *movement_->bindings.state;
        const float speed = std::fabs(state.speed);
        if (speed < 3.0f) {
            ResetThumperTime();
        } else {
            const int32_t first = side_ == 1 ? 0 : 1;
            const int32_t second = side_ == 1 ? 3 : 2;
            if ((state.wheelContact[first] || state.wheelContact[second]) &&
                bindings_.globals->policy.audioSystemEnabled && bindings_.globals->policy.vehicleAudioEnabled &&
                bindings_.globals->tirePopEnabled) {
                Play(TireFlat, spatial.volume, 0.0f, spatial.pitch - 1.0f);
                const uint16_t offset = side_ == 0 ? tireSoundGlobals.thumperAzimuth :
                    static_cast<uint16_t>(0u - tireSoundGlobals.thumperAzimuth);
                Azimuth(TireFlat, static_cast<uint16_t>(spatial.azimuth + offset));
            }
            thumperRemaining_ = speed == 0.0f ? 0.0f : 1.899999976158142f / speed;
            if (thumperRemaining_ < 0.03500000014901161f) thumperRemaining_ = 0.03500000014901161f;
        }
    }
    if (bindings_.frame->disabledWheelCount > 0 && road_ && spatial.active) {
        const float speed = movement_->bindings.state->speed;
        const float gain = spatial.volume * (speed * 0.03999999910593033f);
        const float pitch = spatial.pitch * (speed * 0.004000000189989805f + 1.399999976158142f);
        if (!Playing(MetalScrape)) {
            Play(MetalScrape, gain, 0.0f, pitch - 1.0f);
            Azimuth(MetalScrape, spatial.azimuth);
        } else {
            Volume(MetalScrape, gain);
            Azimuth(MetalScrape, spatial.azimuth);
            SetSamplePitch(voices_[MetalScrape], pitch);
        }
    } else if (Playing(MetalScrape)) {
        Stop(MetalScrape);
    }
}

void PlayerTireSound::UpdateLeaves(float volume, uint16_t azimuth, float pitch)
{
    if (!voices_[Leaves].instance) return;
    const auto& state = *movement_->bindings.state;
    const int32_t first = side_ == 0 ? 0 : 3;
    const int32_t second = side_ == 0 ? 1 : 2;
    const auto leaves = [](int32_t material) { return material == 23 || material == 24 || material == 27; };
    if (leaves(state.material[first]) || leaves(state.material[second])) {
        if (!Playing(Leaves)) {
            Play(Leaves, volume, static_cast<float>(azimuth), pitch - 1.0f);
        } else {
            Volume(Leaves, volume);
            Azimuth(Leaves, azimuth);
            // PAL213EEC passes the incoming scale directly to this SDK call.
            requireTireAudio(worldAudioSetVoicePitchOffset(bindings_.audio, voices_[Leaves], pitch));
        }
    } else if (Playing(Leaves)) {
        Stop(Leaves);
    }
}

void PlayerTireSound::UpdateSquall(int32_t road, float volume, uint16_t azimuth, float pitch)
{
    (void)pitch;
    const Voice selected = road ? SquallSkid : SecondLongitudinalSkid;
    const Voice other = road ? SecondLongitudinalSkid : SquallSkid;
    if (bindings_.globals->policy.audioSystemEnabled && Playing(other)) Stop(other);
    float gain = 0.0f, samplePitch = 1.0f;
    CalcTireSquallVolumeAndPitch(gain, samplePitch);
    gain *= volume;
    if (selected != other && bindings_.globals->policy.audioSystemEnabled &&
        bindings_.globals->policy.vehicleAudioEnabled) {
        if (!Playing(selected) && 0.0f < gain) {
            Play(selected, gain, 0.0f, samplePitch - 1.0f);
        } else if (Playing(selected) && gain <= 0.0f) {
            Stop(selected);
        }
    }
    if (0.0f < gain) {
        const float current = voices_[selected].instance->volume;
        const float next = gain < current ? std::max(current - 0.05000000074505806f, gain) :
            std::min(current + 0.05000000074505806f, gain);
        Volume(selected, next);
        SetSamplePitch(voices_[selected], samplePitch);
        Azimuth(selected, azimuth);
    } else {
        Stop(selected);
    }
}

WorldAudioResult PlayerTireSound::Update(float volume, uint16_t azimuth, float pitch)
{
    if (!spatial.active) return WORLD_AUDIO_OK;
    try {
        const auto& state = *movement_->bindings.state;
        const int32_t first = side_ == 1 ? 0 : 1;
        const int32_t second = side_ == 1 ? 3 : 2;
        const auto* materials = terrainDefaultMaterials(nullptr);
        if (state.material[first] < 0 || state.material[first] >= 33 ||
            state.material[second] < 0 || state.material[second] >= 33)
            return WORLD_AUDIO_INVALID_BINDING;
        const auto& a = materials[state.material[first]];
        const auto& b = materials[state.material[second]];
        int32_t road = static_cast<int8_t>(a.classification & 255u) &&
            static_cast<int8_t>(b.classification & 255u);
        spatial.volume = volume;
        spatial.pitch = pitch;
        spatial.azimuth = azimuth;
        const float noiseVolume = CalcTireNoiseVolume() * volume;
        if (a.type != 4 && b.type != 4) road_ = road;
        road = road_;
        UpdateLeaves(noiseVolume, azimuth, pitch);
        if (0.0f < tireSoundGlobals.oneShotDelay)
            tireSoundGlobals.oneShotDelay -= bindings_.clock->worldTimeElapsed;
        requireTireAudio(metalBump_.Update(movement_, 30, volume * 1.2999999523162842f, azimuth, pitch));
        requireTireAudio(tireFlop_.Update(movement_, 31, volume * 1.5f, azimuth, pitch * 0.800000011920929f));
        thumperRequested_ = a.type == 19 || b.type == 18 || bindings_.frame->disabledWheelCount > 0;
        if (thumperRequested_) UpdateThumperTires(); else ResetThumperTime();
        requireTireAudio(UpdateTireImpact(movement_, side_));

        const float sidePitch = side_ == 0 ? 0.15000000596046448f : 0.0f;
        float skidVolume = 0.0f, skidPitch = 1.0f;
        CalcTireSkidVolumeAndPitch(skidVolume, skidPitch);
        if (!road) skidPitch = sidePitch + 1.0f;
        skidVolume *= volume;
        const float maximum = road ? 3.5f : 2.75f;
        skidVolume = skidVolume < 0.0f ? 0.0f : maximum < skidVolume ? maximum : skidVolume;
        const Voice selected = road ? LongitudinalSkid : GravelSkid;
        const Voice other = road ? GravelSkid : LongitudinalSkid;
        const bool enabled = bindings_.globals->policy.audioSystemEnabled && bindings_.globals->policy.vehicleAudioEnabled;
        if (enabled) {
            if (!Playing(selected) && 0.0f < skidVolume) Play(selected, skidVolume, 0.0f, skidPitch - 1.0f);
            if (Playing(selected) && skidVolume <= 0.0f) Stop(selected);
        }
        if (Playing(other)) Stop(other);
        if (0.0f < skidVolume) {
            Volume(selected, skidVolume);
            SetSamplePitch(voices_[selected], skidPitch);
            Azimuth(selected, azimuth);
        } else {
            Stop(selected);
        }
        UpdateSquall(road, volume, azimuth, pitch);

        const float rear = static_cast<float>(static_cast<double>(state.wheelSlip[2][0] + state.wheelSlip[3][0]) * .5);
        const float slip = static_cast<float>(static_cast<double>(rear) +
            static_cast<double>(state.wheelSlip[0][0] + state.wheelSlip[1][0]) * .5);
        if (road && 0.20000000298023224f < state.effectiveThrottle && 3.0f < slip && state.speed < 30.0f) {
            if (enabled && !Playing(BurnOut)) Play(BurnOut, 0.0f, 0.0f, sidePitch);
            const float gain = std::min(slip * 0.0625f, 3.0f) * volume;
            Volume(BurnOut, gain);
            Azimuth(BurnOut, azimuth);
        } else if (Playing(BurnOut)) {
            Stop(BurnOut);
        }

        if (road) {
            if (enabled && tireSoundGlobals.roadNoiseEnabled && !Playing(RoadNoise)) {
                if (!bindings_.manager->channelVolumes || bindings_.manager->channelVolumeCount <= 4)
                    return WORLD_AUDIO_INVALID_BINDING;
                if (bindings_.manager->channelVolumes[4] == 0.0f)
                    Play(RoadNoise, 0.0f, static_cast<float>(azimuth), CalcTireNoisePitch() - 1.0f);
            }
            Volume(RoadNoise, noiseVolume * roadNoiseScale_);
            Azimuth(RoadNoise, azimuth);
            SetSamplePitch(voices_[RoadNoise], CalcTireNoisePitch());
            if (Playing(GravelNoise)) Stop(GravelNoise);
        } else {
            const float samplePitch = pitch + sidePitch;
            if (enabled && tireSoundGlobals.roadNoiseEnabled && !Playing(GravelNoise))
                Play(GravelNoise, noiseVolume, samplePitch - 1.0f, 0.0f);
            Volume(GravelNoise, noiseVolume * roadNoiseScale_);
            Azimuth(GravelNoise, azimuth);
            SetSamplePitch(voices_[GravelNoise], samplePitch);
            if (Playing(RoadNoise)) Stop(RoadNoise);
        }
        return WORLD_AUDIO_OK;
    } catch (const TireAudioFailure& failure) {
        return failure.result;
    }
}

} // namespace

WorldAudioResult carSoundBindTrackBank(CarSoundBindings& bindings,
    const hp2::TrackScenarioData& track)
{
    if (!bindings.audio) return WORLD_AUDIO_INVALID_BINDING;
    try {
        const TrackResourceInfo* info = track.CurrentInfo();
        if (!info || !std::memchr(info->tireBankName, 0, sizeof(info->tireBankName)))
            return WORLD_AUDIO_INVALID_BINDING;
        const auto* bank = findTireTrackBank(*bindings.audio, info->tireBankName);
        bindings.trackInfo = info;
        bindings.trackBank = bank;
        return WORLD_AUDIO_OK;
    } catch (const std::exception&) {
        return WORLD_AUDIO_INVALID_BINDING;
    }
}

BaseTireSound* carSoundCreatePlayerTire(CarMovement* movement, int32_t side,
    const CarSoundBindings* bindings, WorldAudioResult* result)
{
    if (!bindings) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    try {
        auto* tire = new PlayerTireSound(movement, side, *bindings);
        if (result) *result = WORLD_AUDIO_OK;
        return tire;
    } catch (const TireAudioFailure& failure) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(failure.result,"Player tire sound","Source audio operation rejected");
        if (result) *result = failure.result;
    } catch (const std::bad_alloc& failure) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_CAPACITY,"Player tire sound",failure.what());
        if (result) *result = WORLD_AUDIO_CAPACITY;
    } catch (const std::exception& failure) {
        const auto status=WORLD_AUDIO_INVALID_BINDING;
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(status,"Player tire sound",failure.what());
        if (result) *result = status;
    } catch (...) {
        if (bindings->audio && bindings->audio->library)
            bindings->audio->library->RecordSourceFailure(WORLD_AUDIO_INVALID_BINDING,"Player tire sound","Non-standard source exception");
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
    }
    return nullptr;
}
