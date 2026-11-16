#pragma once

#include "bsound/include/bSound.h"

#include <atomic>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_playback_resampled.hpp>
#include <godot_cpp/classes/node.hpp>

namespace hp2 {
struct DevicePcm;

class DevicePcmPlayback final : public godot::AudioStreamPlaybackResampled {
    GDCLASS(DevicePcmPlayback, godot::AudioStreamPlaybackResampled)
public:
    std::shared_ptr<DevicePcm> pcm;
    void _start(double) override;
    void _stop() override;
    bool _is_playing() const override;
    float _get_stream_sampling_rate() const override;
    int32_t _mix_resampled(godot::AudioFrame* destination, int32_t frames) override;

protected:
    static void _bind_methods() {}

private:
    std::atomic<bool> playing_{false};
    size_t position_{};
};

class DevicePcmStream final : public godot::AudioStream {
    GDCLASS(DevicePcmStream, godot::AudioStream)
public:
    std::shared_ptr<DevicePcm> pcm;
    godot::Ref<godot::AudioStreamPlayback> _instantiate_playback() const override;

protected:
    static void _bind_methods() {}
};

// A host device consumes the original queue. Godot owns the physical players
// and effect buses, and the audio thread only reads detached PCM.
class GodotAudioDevice final {
public:
    explicit GodotAudioDevice(godot::Node& owner);
    ~GodotAudioDevice();
    WorldAudioResult Open(bSound& sound);
    WorldAudioResult Pump();
    WorldAudioResult Close();

private:
    struct State;
    std::unique_ptr<State> state_;
};
}
