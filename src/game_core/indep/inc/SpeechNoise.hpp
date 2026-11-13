#ifndef NFSHP2_SPEECH_NOISE_HPP
#define NFSHP2_SPEECH_NOISE_HPP

#include "bsound/include/bSample.h"

#ifdef __cplusplus
#include <array>

/* The game speech manager borrows this same noise owner. Named samples are
 * retained voices in the common audio pool, not copied playback flags. */
struct SpeechNoise {
    SpeechNoise();
    SpeechNoise(const SpeechNoise&)=delete;
    SpeechNoise& operator=(const SpeechNoise&)=delete;

    static SpeechNoise* getInstance()
    {
        static SpeechNoise instance;
        return &instance;
    }

    WorldAudioResult LoadSamples();
    void RequestSamples();
    WorldAudioResult UnLoadSamples();
    WorldAudioResult Update();
    WorldAudioResult Mute(bool muted);
    WorldAudioResult StartHelicopterNoise();
    WorldAudioResult StopHelicopterNoise();
    void StartRadioNoise();
    void StopRadioNoise();
    int32_t IsRadioNoise() const;
    WorldAudioResult BeginSquelchNoise();
    // A negative result exposes a missing/stale sample binding. It must not
    // be interpreted as a completed speech or successful silent playback.
    int32_t IsSquelchNoisePlaying() const;

    /* Native ownership adapters, not additional original functions. Audio
     * storage and its runtime state must outlive this binding. */
    WorldAudioResult Bind(WorldAudioRequestQueue* audio,WorldAudioRuntimeState* runtime);
    // Terminal native lifetime only: the same queue must have released its
    // real library, banks, sample storage and retained device operations.
    WorldAudioResult UnbindAfterAudioShutdown(WorldAudioRequestQueue& audio);
    WorldAudioResult BindHelicopter(WorldAudioVoiceRef voice);

private:
    WorldAudioResult SamplePlaying(WorldAudioVoiceRef voice,int32_t& playing) const;
    WorldAudioVoiceRef helicopter_{};
    std::array<WorldAudioVoiceRef,9> samples_{};
    int32_t radioNoise_=0;
    int32_t loadRequested_=0;
    int32_t muted_=0;
    WorldAudioRequestQueue* audio_=nullptr;
    WorldAudioRuntimeState* runtime_=&worldAudioRuntime;
};
#else
typedef struct SpeechNoise SpeechNoise;
#endif
#endif
