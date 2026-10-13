#pragma once
#include "game_core/indep/src/movieplayer/movieplayer.hpp"
#include "port/render/frame.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace port::godot_bridge {
// Port host service: original AVI bytes -> pinned CPU MV30 codec -> Godot texture/PCM.
class Ps2MoviePlayer : public godot::RefCounted {
    GDCLASS(Ps2MoviePlayer, godot::RefCounted)
protected:
    static void _bind_methods();
public:
    godot::Dictionary play(const godot::String& relative_path, bool loop);
    godot::Dictionary tick(double delta);
    void stop();
    godot::Ref<godot::ImageTexture> get_texture() const { return texture_; }
    // TASK-0018 / D008: the same decoded frame as a backend-independent bitmap, so a
    // render::Resources can hand the live movie to any renderer. Null until the first frame.
    // revision increments per decoded frame; a backend re-uploads only when it changes.
    std::shared_ptr<const port::render::Bitmap> frame_bitmap() const { return bitmap_; }
    std::uint32_t frame_width() const { return movie_.width; }
    std::uint32_t frame_height() const { return movie_.height; }
    godot::Ref<godot::AudioStreamWAV> get_audio_stream() const { return audio_; }
    godot::Dictionary begin_startup(bool skip_movies);
    godot::Dictionary tick_startup(bool movie_done, double delta);
    godot::Dictionary select_startup();
private:
    void upload_frame(const std::vector<std::uint8_t>& bytes);
    godot::Dictionary startup_result(const port::media::StartupResult& result) const;
    port::media::AviMovie movie_;
    std::unique_ptr<port::media::MovieDecoder> decoder_;
    godot::Ref<godot::ImageTexture> texture_;
    std::shared_ptr<port::render::Bitmap> bitmap_;
    godot::Ref<godot::AudioStreamWAV> audio_;
    std::size_t next_frame_ = 0;
    bool loop_ = false, finished_ = false;
    double elapsed_ = 0, startup_elapsed_ = 0;
    godot::String filename_;
    hp2::frontend::Type_0016F000 startup_;
};
}
