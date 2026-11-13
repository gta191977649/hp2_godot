#include "Sound3dSample.hpp"

#include <cstring>
#include <new>
#include <stdexcept>

Sound3dSampleRegistry::Sound3dSampleRegistry()
    : audio_(nullptr), spatial_{}, enabled_(nullptr), initialized_(false),
      owned_{}, first_(nullptr), last_(nullptr)
{
}

Sound3dSampleRegistry::~Sound3dSampleRegistry()
{
    const WorldAudioResult result = Close();
    if (result != WORLD_AUDIO_OK) {
        // The C++ owner itself is now being destroyed and cannot be retried.
        // Preserve the old terminal best-effort teardown in this path only:
        // each destructor clears its callback before releasing the wrapper;
        // any rejected real STOP remains an error owned by the audio queue.
        while (first_) delete first_;
        if (audio_) audio_->result = result;
    }
    if (audio_ && audio_->positionalSamples == this) audio_->positionalSamples = nullptr;
}

WorldAudioResult Sound3dSampleRegistry::Bind(WorldAudioRequestQueue* audio,
    const Sound3dBindings& spatial, const int32_t* enabled)
{
    if (!audio || !enabled || audio->spatialEnabled != enabled || !spatial.players ||
        !spatial.cameras || !spatial.clock || !spatial.speedOfSound || !spatial.multiplayerAzimuth ||
        (audio->positionalSamples && audio->positionalSamples != this))
        return WORLD_AUDIO_INVALID_BINDING;
    if (first_ && (audio_ != audio || spatial_.players != spatial.players ||
        spatial_.cameras != spatial.cameras || spatial_.clock != spatial.clock || enabled_ != enabled))
        return WORLD_AUDIO_INVALID_BINDING;
    if (audio_ && audio_ != audio && audio_->positionalSamples == this) audio_->positionalSamples = nullptr;
    audio_ = audio;
    spatial_ = spatial;
    enabled_ = enabled;
    audio->positionalSamples = this;
    return Initialize();
}

WorldAudioResult Sound3dSampleRegistry::Initialize()
{
    if (initialized_) return WORLD_AUDIO_OK;
    // PAL2091D0 creates SlotPool(0x110,30,...,0x10). Thirty is its initial
    // count; the same original Malloc/Expand permits later +8 allocations.
    try { owned_.resize(30,nullptr); }
    catch (const std::bad_alloc&) { return WORLD_AUDIO_CAPACITY; }
    initialized_ = true;
    return WORLD_AUDIO_OK;
}

WorldAudioResult Sound3dSampleRegistry::Attach(Sound3dSample* sample)
{
    if (!initialized_ || !audio_ || !enabled_ || !sample) return WORLD_AUDIO_INVALID_BINDING;
    std::size_t slot = owned_.size();
    for (std::size_t index = 0; index < owned_.size(); ++index) {
        if (!owned_[index]) {
            slot = index;
            break;
        }
    }
    if (slot == owned_.size()) {
        // PAL258158..258174 uses (initialCount>>2)+1, not current capacity.
        // Existing objects are separate allocations: growing only their
        // index cannot move a live callback owner, position or list link.
        constexpr std::size_t growth=(30u>>2)+1u;
        if (owned_.max_size()-owned_.size()<growth) return WORLD_AUDIO_CAPACITY;
        try { owned_.resize(owned_.size()+growth,nullptr); }
        catch (const std::bad_alloc&) { return WORLD_AUDIO_CAPACITY; }
    }
    owned_[slot] = sample;
    sample->previous_ = nullptr;
    sample->next_ = first_;
    if (first_) first_->previous_ = sample;
    else last_ = sample;
    first_ = sample;
    return WORLD_AUDIO_OK;
}

void Sound3dSampleRegistry::Detach(Sound3dSample* sample)
{
    if (sample->previous_) sample->previous_->next_ = sample->next_;
    else first_ = sample->next_;
    if (sample->next_) sample->next_->previous_ = sample->previous_;
    else last_ = sample->previous_;
    for (auto& owned : owned_) {
        if (owned == sample) {
            owned = nullptr;
            break;
        }
    }
    sample->next_ = sample->previous_ = nullptr;
}

WorldAudioResult Sound3dSampleRegistry::Destroy(Sound3dSample* sample)
{
    if (!sample) return WORLD_AUDIO_INVALID_BINDING;
    bool owned = false;
    for (const auto* candidate : owned_) owned = owned || candidate == sample;
    if (!owned || sample->registry_ != this) return WORLD_AUDIO_INVALID_BINDING;
    if (sample->voice_.instance) {
        const WorldAudioResult valid = worldAudioValidateVoice(audio_, sample->voice_);
        if (valid != WORLD_AUDIO_OK) return valid;
        const auto& voice = *sample->voice_.instance;
        // PAL2095FC stops the private sample before unlink/SDK destruction.
        // The native transport can reject STOP; deleting first would erase
        // the only retryable owner and callback while leaving a live voice.
        // An ordinary playing handle needs one STOP. A playing locked handle
        // retains its lock after that STOP and needs the final release too.
        const std::size_t requests = voice.hasHandle ?
            1u + static_cast<std::size_t>(voice.playing && voice.primed) : 0u;
        if (worldAudioAvailableRequests(audio_) < requests ||
            requests > UINT64_MAX - audio_->nextSequence ||
            requests > SIZE_MAX - voice.requestReferences)
            return audio_->result = WORLD_AUDIO_CAPACITY;
    }
    delete sample;
    return audio_->result;
}

WorldAudioResult Sound3dSampleRegistry::Reset()
{
    while (first_) {
        // Destroy preflights the complete retirement before the destructor
        // clears the original callback. On failure the same list can retry.
        const WorldAudioResult result = Destroy(first_);
        if (result != WORLD_AUDIO_OK) return result;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult Sound3dSampleRegistry::Close()
{
    const WorldAudioResult result = Reset();
    if (result != WORLD_AUDIO_OK) return result;
    initialized_ = false;
    // Original Close deletes its pool only after Reset destroys the list.
    // Device requests retain their bSample/bank owners independently.
    std::vector<Sound3dSample*>().swap(owned_);
    return result;
}

WorldAudioResult Sound3dSampleRegistry::Update()
{
    if (!audio_ || !enabled_) return WORLD_AUDIO_INVALID_BINDING;
    for (Sound3dSample* sample = first_; sample; ) {
        Sound3dSample* next = sample->next_;
        const WorldAudioVoiceRef voice = sample->Sample();
        if (voice.instance) {
            const WorldAudioResult valid = worldAudioValidateVoice(audio_, voice);
            if (valid != WORLD_AUDIO_OK) return valid;
            if (!voice.instance->paused) {
                const WorldAudioResult result = sample->Update(1.0f, 0, 1.0f);
                if (result != WORLD_AUDIO_OK) return result;
            }
        }
        sample = next;
    }
    return WORLD_AUDIO_OK;
}

WorldAudioResult Sound3dSampleRegistry::Pause(int32_t paused)
{
    if (!audio_) return WORLD_AUDIO_INVALID_BINDING;
    for (Sound3dSample* sample = first_; sample; sample = sample->next_) {
        const WorldAudioResult result = sample->Pause(paused);
        if (result != WORLD_AUDIO_OK) return result;
    }
    return WORLD_AUDIO_OK;
}

Sound3dSample* Sound3dSampleRegistry::Find(WorldAudioVoiceRef voice) const
{
    for (Sound3dSample* sample = first_; sample; sample = sample->next_) {
        const auto candidate = sample->Sample();
        if (candidate.instance == voice.instance && candidate.generation == voice.generation) return sample;
    }
    return nullptr;
}

std::size_t Sound3dSampleRegistry::Count() const
{
    std::size_t count = 0;
    for (const auto* sample : owned_) if (sample) ++count;
    return count;
}

Sound3dSample* Sound3dSampleRegistry::Play(const char* name, const float position[4],
    float volume, float pitch, WorldAudioResult* result)
{
    if (!audio_) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    const WorldAudioCue* cue = worldAudioFindInGame(audio_, name);
    return Play(audio_->inGameBank, cue, position, volume, pitch, result);
}

Sound3dSample* Sound3dSampleRegistry::Play(const WorldAudioBank* bank, const WorldAudioCue* sample,
    const float position[4], float volume, float pitch, WorldAudioResult* result)
{
    if (!audio_ || !enabled_) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    if (!*enabled_) {
        if (result) *result = WORLD_AUDIO_OK;
        return nullptr;
    }
    if (!bank || !sample || !position || !initialized_) {
        if (result) *result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    Sound3dSample* created = nullptr;
    WorldAudioResult status;
    try {
        created = new Sound3dSample(*this, position, volume);
        status = created->InitializeSample(bank, sample);
        if (status == WORLD_AUDIO_OK) {
            created->spatial.dopplerActive = 0;
            status = created->Activate(1);
        }
        if (status == WORLD_AUDIO_OK) status = created->SetDeleteWhenDone(1);
        if (status == WORLD_AUDIO_OK) {
            const WorldAudioVoiceRef reference = created->Sample();
            status = created->Update(position, volume, 0, pitch);
            // An initial zero-volume Update calls Stop and can synchronously
            // delete the owner. Never return the source's dangling pointer.
            if (Find(reference) != created) created = nullptr;
        }
    } catch (const std::bad_alloc&) {
        status = WORLD_AUDIO_CAPACITY;
    } catch (...) {
        status = WORLD_AUDIO_INVALID_BINDING;
    }
    if (status != WORLD_AUDIO_OK && created) {
        Destroy(created);
        created = nullptr;
    }
    if (result) *result = status;
    audio_->result = status;
    return created;
}

Sound3dSample::Sound3dSample(Sound3dSampleRegistry& registry)
    : registry_(&registry), next_(nullptr), previous_(nullptr), voice_{nullptr, 0},
      envelope_{1.0f, 200.0f, 0.004999999888241291f}, positionMode_(0), position_{}
{
    if (registry.Attach(this) != WORLD_AUDIO_OK) throw std::bad_alloc();
}

Sound3dSample::Sound3dSample(Sound3dSampleRegistry& registry,
    const float position[4], float volume)
    : registry_(&registry), next_(nullptr), previous_(nullptr), voice_{nullptr, 0},
      envelope_{volume, 135.0f, 0.007407407276332378f}, positionMode_(1), position_{}
{
    std::memcpy(position_, position, sizeof(position_));
    if (registry.Attach(this) != WORLD_AUDIO_OK) throw std::bad_alloc();
}

Sound3dSample::~Sound3dSample()
{
    WorldAudioResult result = WORLD_AUDIO_OK;
    if (voice_.instance) {
        if (voice_.instance->completionKind==WORLD_AUDIO_COMPLETION_POSITIONAL &&
            voice_.instance->completionSample==this) {
            voice_.instance->completionSample=nullptr;
            voice_.instance->completionKind=WORLD_AUDIO_COMPLETION_NONE;
        }
        if (voice_.instance->playing) result = worldAudioStopInstance(registry_->audio_, voice_.instance);
    }
    registry_->Detach(this);
    if (voice_.instance) {
        const WorldAudioResult retired = worldAudioRetireVoice(registry_->audio_, voice_);
        if (result == WORLD_AUDIO_OK) result = retired;
    }
    registry_->audio_->result = result;
}

WorldAudioResult Sound3dSample::InitializeSample(const WorldAudioBank* bank, const WorldAudioCue* cue)
{
    return worldAudioInitializeVoiceFromCue(registry_->audio_, bank, cue, &voice_);
}

WorldAudioResult Sound3dSample::Update(const float position[4], float volume,
    uint16_t azimuth, float pitch)
{
    if (!position) return WORLD_AUDIO_INVALID_BINDING;
    if (!*registry_->enabled_ || !spatial.active) return WORLD_AUDIO_OK;
    std::memcpy(position_, position, sizeof(position_));
    positionMode_ = 1;
    return Update(volume, azimuth, pitch);
}

WorldAudioResult Sound3dSample::Update(float volume, uint16_t azimuth, float pitch)
{
    (void)azimuth;
    if (*registry_->enabled_ && spatial.active) {
        if (sound3dUpdate(&spatial, &registry_->spatial_, position_, &envelope_) != SOUND3D_OK)
            return WORLD_AUDIO_INVALID_BINDING;
        WorldAudioResult result = SetVolume(spatial.volume * volume);
        if (result != WORLD_AUDIO_OK) return result;
        result = SetPitch(spatial.pitch * pitch);
        if (result != WORLD_AUDIO_OK) return result;
        result = worldAudioSetVoiceControlCode(registry_->audio_, voice_, spatial.azimuth);
        if (result != WORLD_AUDIO_OK) return result;
    }
    if (volume == 0.0f) return DeactivateSound();
    return WORLD_AUDIO_OK;
}

WorldAudioResult Sound3dSample::ActivateSound()
{
    if (!*registry_->enabled_) return WORLD_AUDIO_OK;
    if (!voice_.instance) return WORLD_AUDIO_INVALID_BINDING;
    const WorldAudioResult valid = worldAudioValidateVoice(registry_->audio_, voice_);
    if (valid != WORLD_AUDIO_OK) return valid;
    if (voice_.instance->playing) return WORLD_AUDIO_OK;
    return worldAudioPlayVoice(registry_->audio_, voice_, 0.0f, 0.0f, 0.0f);
}

WorldAudioResult Sound3dSample::DeactivateSound()
{
    if (!voice_.instance) return WORLD_AUDIO_OK; // SDK Stop on an unbound sample has no live voice.
    return worldAudioStopInstance(registry_->audio_, voice_.instance);
}

WorldAudioResult Sound3dSample::Pause(int32_t paused)
{
    if (!voice_.instance) return WORLD_AUDIO_OK;
    return worldAudioPauseVoice(registry_->audio_, voice_, paused != 0);
}

WorldAudioResult Sound3dSample::SetDeleteWhenDone(int32_t enabled)
{
    if (!voice_.instance) return enabled ? WORLD_AUDIO_INVALID_BINDING : WORLD_AUDIO_OK;
    const WorldAudioResult result = worldAudioValidateVoice(registry_->audio_, voice_);
    if (result != WORLD_AUDIO_OK) return result;
    if (voice_.instance->completionKind!=WORLD_AUDIO_COMPLETION_NONE &&
        voice_.instance->completionKind!=WORLD_AUDIO_COMPLETION_POSITIONAL)
        return WORLD_AUDIO_INVALID_BINDING;
    voice_.instance->completionSample = enabled ? this : nullptr;
    voice_.instance->completionKind = enabled ? WORLD_AUDIO_COMPLETION_POSITIONAL : WORLD_AUDIO_COMPLETION_NONE;
    return WORLD_AUDIO_OK;
}

WorldAudioResult Sound3dSample::OnVoiceCompleted(WorldAudioVoiceRef voice)
{
    if (voice.instance != voice_.instance || voice.generation != voice_.generation ||
        registry_->Find(voice) != this) return WORLD_AUDIO_INVALID_BINDING;
    return registry_->Destroy(this);
}

bool Sound3dSample::StopCanDeleteOwner() const
{
    return voice_.instance && voice_.instance->hasHandle &&
        voice_.instance->completionKind==WORLD_AUDIO_COMPLETION_POSITIONAL &&
        voice_.instance->completionSample == this;
}

WorldAudioResult Sound3dSample::SetAzimuth(uint16_t azimuth)
{
    spatial.azimuth = azimuth;
    return worldAudioSetVoiceControlCode(registry_->audio_, voice_, azimuth);
}

WorldAudioResult Sound3dSample::SetVolume(float volume)
{
    spatial.volume = volume;
    return worldAudioSetVoiceVolume(registry_->audio_, voice_, volume);
}

WorldAudioResult Sound3dSample::SetPitch(float pitch)
{
    spatial.pitch = pitch;
    return worldAudioSetVoicePitchOffset(registry_->audio_, voice_, pitch - 1.0f);
}

void Sound3dSample::SetPosition(const float position[4])
{
    std::memcpy(position_, position, sizeof(position_));
}

float* Sound3dSample::Position() { return position_; }
const float* Sound3dSample::Position() const { return position_; }
Sound3dEnvelope* Sound3dSample::Envelope() { return &envelope_; }
WorldAudioVoiceRef Sound3dSample::Sample() const { return voice_; }

/* Compatibility declarations route callers into the one actual positional
 * owner. Position and envelope remain in Sound3dSample, never in bSample. */
WorldAudioInstance* worldAudioPlayCueAtPointScaled(WorldAudioRequestQueue* queue,
    const WorldAudioBank* bank, const WorldAudioCue* cue, const float position[4],
    float initialParameter, float pitchScale, int32_t tag, int32_t autoRelease)
{
    (void)tag; // Legacy host bookkeeping was not the source sample category.
    if (!queue) return nullptr;
    if (!queue->spatialEnabled) {
        queue->result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    if (!*queue->spatialEnabled) {
        queue->result = WORLD_AUDIO_OK;
        return nullptr;
    }
    if (!queue->positionalSamples) {
        queue->result = WORLD_AUDIO_INVALID_BINDING;
        return nullptr;
    }
    Sound3dSample* sample = queue->positionalSamples->Play(bank, cue, position,
        initialParameter, pitchScale, &queue->result);
    if (!sample) return nullptr;
    if (!autoRelease && sample->SetDeleteWhenDone(0) != WORLD_AUDIO_OK) {
        queue->positionalSamples->Destroy(sample);
        return nullptr;
    }
    return sample->Sample().instance;
}

WorldAudioInstance* worldAudioPlayAtPointScaled(WorldAudioRequestQueue* queue, const char* name,
    const float position[4], float initialParameter, float pitchScale,
    int32_t tag, int32_t autoRelease)
{
    const WorldAudioCue* cue = worldAudioFindInGame(queue, name);
    if (!queue || !cue) return nullptr;
    return worldAudioPlayCueAtPointScaled(queue, queue->inGameBank, cue, position,
        initialParameter, pitchScale, tag, autoRelease);
}

WorldAudioInstance* worldAudioPlayAtPoint(WorldAudioRequestQueue* queue, const char* name,
    const float position[4], float initialParameter, int32_t tag, int32_t autoRelease)
{
    return worldAudioPlayAtPointScaled(queue, name, position, initialParameter, 1.0f, tag, autoRelease);
}
