#pragma once

#include "bware/inc/bFile.hpp"

#include <godot_cpp/classes/video_stream.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/video_stream_playback.hpp>

#include <memory>
#include <string>

namespace godot { class ShaderMaterial; }

namespace hp2 {

class OriginalVideoPlayback final : public godot::VideoStreamPlayback {
    GDCLASS(OriginalVideoPlayback, godot::VideoStreamPlayback)

public:
    OriginalVideoPlayback();
    ~OriginalVideoPlayback() override;
    void open(const FileRange& file, const std::string& path, const godot::Ref<godot::ShaderMaterial>& material);
    const std::string& failure() const noexcept;
    std::size_t submitted_audio_frames() const noexcept;
    void _play() override;
    void _stop() override;
    bool _is_playing() const override;
    void _set_paused(bool paused) override;
    bool _is_paused() const override;
    double _get_length() const override;
    double _get_playback_position() const override;
    godot::Ref<godot::Texture2D> _get_texture() const override;
    void _update(double delta) override;
    int32_t _get_channels() const override;
    int32_t _get_mix_rate() const override;

protected:
    static void _bind_methods() {}

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

class OriginalVideoStream final : public godot::VideoStream {
    GDCLASS(OriginalVideoStream, godot::VideoStream)

public:
    void set_playback(const godot::Ref<OriginalVideoPlayback>& playback);
    godot::Ref<godot::VideoStreamPlayback> _instantiate_playback() override;

protected:
    static void _bind_methods() {}

private:
    godot::Ref<OriginalVideoPlayback> playback_;
};

}
