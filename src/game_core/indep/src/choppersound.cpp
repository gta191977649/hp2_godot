#include "CarSounds.hpp"
#include "Chopper.hpp"
#include "indep/inc/Timer.hpp"

#include <algorithm>
#include <new>
#include <stdexcept>

namespace {

// Source215598 initializes the same envelope to5,200 and the actual0.005
// literal. Static native data initialization is not another ELF body claim.
const Sound3dEnvelope chopperSoundEnvelope = {5.0f, 200.0f, 0.004999999888241291f};

void requireChopperAudio(WorldAudioResult result)
{
    if (result != WORLD_AUDIO_OK) throw std::runtime_error("Chopper shared audio sample is unavailable");
}

} // namespace

ChopperSound::ChopperSound(Chopper* chopper, const CarSoundBindings& bindings)
    : chopper_(chopper), bindings_(bindings), voice_{}, previousInsideTunnel_(0), tunnelTransition_(0.0f)
{
    if (!chopper || !bindings.audio || !bindings.globals || !bindings.clock ||
        bindings.spatial.clock != bindings.clock ||
        bindings.spatial.speedOfSound != &bindings.globals->speedOfSound ||
        !bindings.spatial.players || !bindings.spatial.cameras)
        throw std::invalid_argument("Chopper sound has no actual movement/listener/audio owners");
    if (bindings.globals->policy.audioSystemEnabled) {
        requireChopperAudio(worldAudioAcquireInGameSample(bindings.audio, "ChopperEngine", &voice_));
        requireChopperAudio(worldAudioSetVoiceCategory(bindings.audio, voice_, 0));
        if (voice_.instance->playing)
            requireChopperAudio(worldAudioStopInstance(bindings.audio, voice_.instance));
    }
}

ChopperSound::~ChopperSound() = default;

void ChopperSound::AdjustForTunnelVolume(float& volume)
{
    const int32_t inside = soundScapeIsInsideTunnel();
    if (inside) {
        if (!previousInsideTunnel_) {
            previousInsideTunnel_ = inside;
            tunnelTransition_ = 1.0f;
        } else if (0.0f < tunnelTransition_) {
            tunnelTransition_ = tunnelTransition_ - bindings_.clock->worldTimeElapsed;
            const float adjusted = volume * tunnelTransition_;
            volume = std::max(adjusted, 0.0f);
        } else {
            volume = 0.0f;
        }
    } else if (previousInsideTunnel_) {
        if (tunnelTransition_ <= 0.0f) {
            tunnelTransition_ = 1.0f;
            volume = 0.0f;
        } else {
            volume = volume * tunnelTransition_;
            tunnelTransition_ = 1.0f - tunnelTransition_;
        }
        previousInsideTunnel_ = inside;
    } else if (0.0f < tunnelTransition_) {
        tunnelTransition_ = tunnelTransition_ - bindings_.clock->worldTimeElapsed;
        const float scale = 1.0f - tunnelTransition_;
        volume = volume * scale;
    }
}

WorldAudioResult ChopperSound::Update(float volume, uint16_t azimuth, float pitch)
{
    (void)volume;
    (void)azimuth;
    (void)pitch;
    if (!bindings_.globals->policy.audioSystemEnabled || !spatial.active || !voice_.instance)
        return WORLD_AUDIO_OK;
    if (sound3dUpdate(&spatial, &bindings_.spatial, chopper_->position,
        &chopperSoundEnvelope) != SOUND3D_OK) return WORLD_AUDIO_INVALID_BINDING;
    AdjustForTunnelVolume(spatial.volume);
    if (voice_.instance->playing) {
        WorldAudioResult result = worldAudioSetVoiceVolume(bindings_.audio, voice_, spatial.volume);
        if (result == WORLD_AUDIO_OK)
            result = worldAudioSetVoiceControlCode(bindings_.audio, voice_, spatial.azimuth);
        if (result == WORLD_AUDIO_OK)
            result = worldAudioSetVoicePitchOffset(bindings_.audio, voice_, spatial.pitch - 1.0f);
        return result;
    }
    return worldAudioPlayVoice(bindings_.audio, voice_, spatial.volume,
        static_cast<float>(spatial.azimuth), spatial.pitch - 1.0f);
}

WorldAudioResult ChopperSound::Pause(int32_t paused)
{
    if (!bindings_.globals->policy.audioSystemEnabled || !voice_.instance) return WORLD_AUDIO_OK;
    return worldAudioPauseVoice(bindings_.audio, voice_, paused != 0);
}

WorldAudioResult ChopperSound::DeactivateSound()
{
    if (!bindings_.globals->policy.audioSystemEnabled || !voice_.instance) return WORLD_AUDIO_OK;
    return worldAudioStopInstance(bindings_.audio, voice_.instance);
}

WorldAudioResult ChopperSound::ActivateSound()
{
    if (!bindings_.globals->policy.audioSystemEnabled || !voice_.instance) return WORLD_AUDIO_OK;
    return worldAudioPlayVoice(bindings_.audio, voice_, spatial.volume,
        static_cast<float>(spatial.azimuth), spatial.pitch - 1.0f);
}

Sound3d* carSoundCreateChopper(Chopper* chopper, const CarSoundBindings* bindings,
    WorldAudioResult* result)
{
    if (!bindings) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    try {
        auto* sound = new ChopperSound(chopper, *bindings);
        if (result) *result = WORLD_AUDIO_OK;
        return sound;
    } catch (const std::bad_alloc&) {
        if (result) *result = WORLD_AUDIO_CAPACITY;
    } catch (...) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
    }
    return nullptr;
}

WorldAudioResult carSoundBindChopper(Chopper* chopper, const CarSoundBindings* bindings)
{
    if (!chopper || chopper->sound) return WORLD_AUDIO_INVALID_BINDING;
    WorldAudioResult result{};
    chopper->sound = carSoundCreateChopper(chopper, bindings, &result);
    return result;
}

WorldAudioResult carSoundDisposeChopper(Chopper* chopper)
{
    if (!chopper) return WORLD_AUDIO_INVALID_BINDING;
    if (!chopper->sound) return WORLD_AUDIO_OK;
    auto* sound = dynamic_cast<ChopperSound*>(chopper->sound);
    if (!sound || sound->chopper_ != chopper) return WORLD_AUDIO_INVALID_BINDING;
    delete sound;
    chopper->sound = nullptr;
    // The shared bank retains its actual sample and queued requests. Source
    // ChopperSound's destructor neither stops nor destroys that bank sample.
    return WORLD_AUDIO_OK;
}
