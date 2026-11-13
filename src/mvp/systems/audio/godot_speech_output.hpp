#pragma once

#include "bsound/include/bSound.h"

#include <atomic>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_playback_resampled.hpp>

namespace godot { class Node; }

namespace hp2 {
struct SpeechBuffer;

class OriginalSpeechPlayback final : public godot::AudioStreamPlaybackResampled {
    GDCLASS(OriginalSpeechPlayback, godot::AudioStreamPlaybackResampled)
public:
    void initialize(std::shared_ptr<SpeechBuffer> buffer);
    void _start(double position) override;
    void _stop() override;
    bool _is_playing() const override;
    float _get_stream_sampling_rate() const override;
    int32_t _mix_resampled(godot::AudioFrame* destination, int32_t frames) override;
protected:
    static void _bind_methods() {}
private:
    std::shared_ptr<SpeechBuffer> buffer_;
    std::atomic<bool> playing_{false};
};

class OriginalSpeechStream final : public godot::AudioStream {
    GDCLASS(OriginalSpeechStream, godot::AudioStream)
public:
    void initialize(std::shared_ptr<SpeechBuffer> buffer);
    godot::Ref<godot::AudioStreamPlayback> _instantiate_playback() const override;
protected:
    static void _bind_methods() {}
private:
    std::shared_ptr<SpeechBuffer> buffer_;
};

// Port-side radio backend. The original queue still controls source lifetime.
class GodotSpeechOutput final : public NativeAudioVoiceOutput {
public:
    explicit GodotSpeechOutput(godot::Node& owner);
    ~GodotSpeechOutput() override;
    NativeAudioApply Apply(bSound& sound, const WorldAudioRequest& operation) override;
    WorldAudioResult Observe(bVoiceStatusSnapshot& status) override;
    void SetMode(uint8_t mode) override;
    void Close(bSound& sound) override;
private:
    struct State;
    std::unique_ptr<State> state_;
};
}
