#ifndef NFSHP2_SOUND3D_SAMPLE_HPP
#define NFSHP2_SOUND3D_SAMPLE_HPP

#include "Sound3d.hpp"

#include <cstddef>
#include <vector>

struct Sound3dSample;

/* The original global list and its initially thirty-object pool are one
 * owner. Only this pointer index grows; published objects/links stay put. */
struct Sound3dSampleRegistry {
    Sound3dSampleRegistry();
    ~Sound3dSampleRegistry();
    Sound3dSampleRegistry(const Sound3dSampleRegistry&) = delete;
    Sound3dSampleRegistry& operator=(const Sound3dSampleRegistry&) = delete;
    WorldAudioResult Bind(WorldAudioRequestQueue* audio,
        const Sound3dBindings& spatial, const int32_t* enabled);
    WorldAudioResult Initialize();
    // A retained-queue capacity error preserves pending owners/list/storage;
    // drain the actual consumer, then retry before destroying the registry.
    WorldAudioResult Reset();
    WorldAudioResult Close();
    WorldAudioResult Update();
    WorldAudioResult Pause(int32_t paused);
    Sound3dSample* Play(const char* name, const float position[4],
        float volume, float pitch, WorldAudioResult* result);
    Sound3dSample* Play(const WorldAudioBank* bank, const WorldAudioCue* sample,
        const float position[4], float volume, float pitch, WorldAudioResult* result);
    WorldAudioResult Destroy(Sound3dSample* sample);
    Sound3dSample* Find(WorldAudioVoiceRef voice) const;
    std::size_t Count() const;

private:
    friend struct Sound3dSample;
    WorldAudioResult Attach(Sound3dSample* sample);
    void Detach(Sound3dSample* sample);
    WorldAudioRequestQueue* audio_;
    Sound3dBindings spatial_;
    const int32_t* enabled_;
    bool initialized_;
    std::vector<Sound3dSample*> owned_;
    Sound3dSample* first_;
    Sound3dSample* last_;
};

struct Sound3dSample : Sound3d {
    ~Sound3dSample() override;
    WorldAudioResult InitializeSample(const WorldAudioBank* bank, const WorldAudioCue* cue);
    WorldAudioResult Update(float volume, uint16_t azimuth, float pitch) override;
    WorldAudioResult Update(const float position[4], float volume, uint16_t azimuth, float pitch);
    WorldAudioResult Pause(int32_t paused) override;
    WorldAudioResult SetDeleteWhenDone(int32_t enabled);
    WorldAudioResult SetAzimuth(uint16_t azimuth);
    WorldAudioResult SetVolume(float volume);
    WorldAudioResult SetPitch(float pitch);
    void SetPosition(const float position[4]);
    float* Position();
    const float* Position() const;
    Sound3dEnvelope* Envelope();
    WorldAudioVoiceRef Sample() const;
    WorldAudioResult OnVoiceCompleted(WorldAudioVoiceRef voice);

protected:
    explicit Sound3dSample(Sound3dSampleRegistry& registry);
    Sound3dSample(Sound3dSampleRegistry& registry, const float position[4], float volume);
    WorldAudioResult ActivateSound() override;
    WorldAudioResult DeactivateSound() override;

private:
    friend class Sound3d;
    friend struct Sound3dSampleRegistry;
    bool StopCanDeleteOwner() const;
    Sound3dSampleRegistry* registry_;
    Sound3dSample* next_;
    Sound3dSample* previous_;
    WorldAudioVoiceRef voice_;
    Sound3dEnvelope envelope_;
    int32_t positionMode_;
    float position_[4];
};

#endif
