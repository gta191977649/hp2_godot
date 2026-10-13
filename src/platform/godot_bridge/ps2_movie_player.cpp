#include "ps2_movie_player.hpp"
#include "game_core/movieplayer/movie_audio.hpp"
#include "platform/ps2_asset_root.hpp"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <cmath>
#include <cstring>
#include <fstream>

namespace port::godot_bridge {
void Ps2MoviePlayer::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("play", "relative_path", "loop"), &Ps2MoviePlayer::play);
    godot::ClassDB::bind_method(godot::D_METHOD("tick", "delta"), &Ps2MoviePlayer::tick);
    godot::ClassDB::bind_method(godot::D_METHOD("stop"), &Ps2MoviePlayer::stop);
    godot::ClassDB::bind_method(godot::D_METHOD("get_texture"), &Ps2MoviePlayer::get_texture);
    godot::ClassDB::bind_method(godot::D_METHOD("get_audio_stream"), &Ps2MoviePlayer::get_audio_stream);
    godot::ClassDB::bind_method(godot::D_METHOD("begin_startup", "skip_movies"), &Ps2MoviePlayer::begin_startup);
    godot::ClassDB::bind_method(godot::D_METHOD("tick_startup", "movie_done", "delta"), &Ps2MoviePlayer::tick_startup);
    godot::ClassDB::bind_method(godot::D_METHOD("select_startup"), &Ps2MoviePlayer::select_startup);
}
void Ps2MoviePlayer::upload_frame(const std::vector<std::uint8_t>& bytes) {
    // Keep a backend-independent copy first: the native renderer samples this, not the
    // ImageTexture, which exists only for the older Control-based host scene.
    const auto revision = bitmap_ ? bitmap_->revision + 1 : 1;
    bitmap_ = std::make_shared<port::render::Bitmap>();
    bitmap_->width = static_cast<int>(movie_.width);
    bitmap_->height = static_cast<int>(movie_.height);
    bitmap_->rgba = bytes;
    bitmap_->revision = revision;
    godot::PackedByteArray pixels;
    pixels.resize(static_cast<int64_t>(bytes.size()));
    if (!bytes.empty()) std::memcpy(pixels.ptrw(), bytes.data(), bytes.size());
    auto image = godot::Image::create_from_data(movie_.width, movie_.height, false, godot::Image::FORMAT_RGBA8, pixels);
    if (texture_.is_null()) texture_ = godot::ImageTexture::create_from_image(image);
    else texture_->update(image);
}
void Ps2MoviePlayer::stop() {
    decoder_.reset(); movie_ = {}; texture_.unref(); audio_.unref(); bitmap_.reset();
    filename_ = ""; elapsed_ = 0; next_frame_ = 0; finished_ = false;
}
godot::Dictionary Ps2MoviePlayer::play(const godot::String& relative_path, bool loop) {
    godot::Dictionary result;
    try {
        // E-0016-V02: background requests for the same filename preserve current playback.
        if (loop && filename_ == relative_path && decoder_) { result["ok"] = true; return result; }
        stop();
        const auto project = godot::ProjectSettings::get_singleton()->globalize_path("res://");
        const auto root = hp2::platform::resolve_zzdata_root(project.utf8().get_data());
        if (!root) throw std::runtime_error("PS2 asset root not configured");
        const auto relative = std::string(relative_path.utf8().get_data());
        // Filenames are recovered literals; restrict the host seam to ZZDATA/MOVIES.
        if (relative.rfind("MOVIES/", 0) != 0 || relative.find("..") != std::string::npos)
            throw std::runtime_error("movie path must stay within ZZDATA/MOVIES");
        std::ifstream file(*root + "/" + relative, std::ios::binary);
        if (!file) throw std::runtime_error("could not open original movie: " + relative);
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
        movie_ = port::media::read_avi(bytes);
        decoder_ = std::make_unique<port::media::MovieDecoder>(movie_.width, movie_.height);
        loop_ = loop; filename_ = relative_path;
        upload_frame(decoder_->decode_frame(movie_.video_frames[0])); next_frame_ = 1;
        if (!movie_.audio_data.empty()) {
            const auto decoded = hp2::port::audio::decode_movie_audio(movie_.audio_data);
            godot::PackedByteArray samples;
            samples.resize(static_cast<int64_t>(decoded.pcm.samples.size() * 2));
            for (std::size_t i = 0; i < decoded.pcm.samples.size(); ++i) {
                const auto value = static_cast<std::uint16_t>(decoded.pcm.samples[i]);
                samples.set(i * 2, value & 0xff);
                samples.set(i * 2 + 1, value >> 8);
            }
            audio_.instantiate();
            audio_->set_format(godot::AudioStreamWAV::FORMAT_16_BITS);
            audio_->set_mix_rate(decoded.pcm.sample_rate); audio_->set_stereo(true);
            audio_->set_data(samples);
            result["audio_pitch_scale"] = decoded.pitch_scale;
        }
        result["ok"] = true; result["frames"] = static_cast<int64_t>(movie_.video_frames.size());
        result["width"] = movie_.width; result["height"] = movie_.height;
        result["fps"] = static_cast<double>(movie_.rate) / movie_.scale;
    } catch (const std::exception& error) {
        stop(); result["ok"] = false; result["not_implemented"] = godot::String::utf8(error.what());
    }
    return result;
}
godot::Dictionary Ps2MoviePlayer::tick(double delta) {
    godot::Dictionary result;
    try {
        if (!std::isfinite(delta) || delta < 0) throw std::runtime_error("movie delta must be finite and nonnegative");
        if (decoder_ && !finished_) {
            elapsed_ += delta;
            const double fps = static_cast<double>(movie_.rate) / movie_.scale;
            const auto frame_count = movie_.video_frames.size();
            const double duration = frame_count / fps;
            // E-0016-A07: audio starts immediately; original video clock lags by 0.16 s.
            const double video_time = std::max(0.0, elapsed_ - (audio_.is_valid() ? 0.16 : 0.0));
            if (loop_ && elapsed_ >= duration) {
                elapsed_ = std::fmod(elapsed_, duration);
                decoder_ = std::make_unique<port::media::MovieDecoder>(movie_.width, movie_.height);
                next_frame_ = 0;
            }
            const auto due = std::min(frame_count - 1, static_cast<std::size_t>(std::floor(video_time * fps)));
            while (next_frame_ <= due) {
                const auto pixels = decoder_->decode_frame(movie_.video_frames[next_frame_++]);
                if (next_frame_ == due + 1) upload_frame(pixels);
            }
            if (!loop_ && video_time >= duration) finished_ = true;
        }
        result["ok"] = true; result["finished"] = finished_;
        result["frame"] = static_cast<int64_t>(next_frame_ == 0 ? 0 : next_frame_ - 1);
    } catch (const std::exception& error) {
        result["ok"] = false; result["not_implemented"] = godot::String::utf8(error.what());
    }
    return result;
}
godot::Dictionary Ps2MoviePlayer::startup_result(const port::media::StartupResult& action) const {
    godot::Dictionary result;
    result["action"] = static_cast<int>(action.action);
    result["movie"] = action.filename == nullptr ? godot::String() : godot::String::utf8(action.filename);
    result["state"] = startup_.Field_0x0c;
    result["start_music"] = action.start_music;
    const auto ticks = static_cast<std::uint32_t>(std::fmod(startup_elapsed_ * 4000, 4294967296.0)) + 1u;
    result["can_start"] = startup_.Sub_0016F4F8(ticks);
    return result;
}
godot::Dictionary Ps2MoviePlayer::begin_startup(bool skip_movies) {
    startup_elapsed_ = 0;
    return startup_result(startup_.Construct_0016F000(skip_movies, false, 0, 1));
}
godot::Dictionary Ps2MoviePlayer::tick_startup(bool movie_done, double delta) {
    startup_elapsed_ += delta;
    const auto ticks = static_cast<std::uint32_t>(std::fmod(startup_elapsed_ * 4000, 4294967296.0)) + 1u;
    return startup_result(startup_.Update_0016F1F0(movie_done, ticks));
}
godot::Dictionary Ps2MoviePlayer::select_startup() {
    const auto ticks = static_cast<std::uint32_t>(std::fmod(startup_elapsed_ * 4000, 4294967296.0)) + 1u;
    return startup_result(startup_.Activate_0016EEF8(ticks));
}
}
