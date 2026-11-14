#include "frontend_video.hpp"

#include "assets/shader_resource.hpp"

#include "original_video_stream.hpp"
#include "indep/src/MoviePlayer/MV3Codec.h"

#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/video_stream_player.hpp>

#include <chrono>
#include <cstdio>
#include <stdexcept>

namespace hp2 {
using namespace godot;

struct FrontendVideo::Storage {
    Storage(VideoStreamPlayer& node, const GameResources& source) : player(node), assets(source) {
        Ref<Shader> shader;
        shader = load_shader("res://shader/frontend_video.gdshader");
        material.instantiate();
        material->set_shader(shader);
        player.set_material(material);
    }

    VideoStreamPlayer& player;
    const GameResources& assets;
    Ref<ShaderMaterial> material;
    Ref<OriginalVideoPlayback> playback;
    std::uint64_t request{};
    bool started{};
};

FrontendVideo::FrontendVideo(VideoStreamPlayer& player, const GameResources& assets)
    : storage_(std::make_unique<Storage>(player, assets)) {}
FrontendVideo::~FrontendVideo() = default;

bool FrontendVideo::playing() const noexcept {return storage_->player.is_playing();}
std::uint64_t FrontendVideo::request_id() const noexcept {return storage_->request;}

void FrontendVideo::update(MoviePlayer& source) {
    auto& s = *storage_;
    const auto* path = source.RequestedMovie();
    const auto request = path ? source.MovieRequestId() : 0;
    if (request != s.request) {
        if (s.playback.is_valid() && !s.playback->failure().empty())
            throw std::runtime_error(s.playback->failure());
        if (s.playback.is_valid() && s.playback->_get_channels() > 0)
            std::fprintf(stderr, "HP2_MVP_TRACE stage=frontend_video_audio request=%llu mixed_frames=%zu\n",
                static_cast<unsigned long long>(s.request), s.playback->submitted_audio_frames());
        s.player.stop();
        s.player.set_stream({});
        s.playback.unref();
        s.request = request;
        s.started = false;
        if (!path) return;
        const auto start = std::chrono::steady_clock::now();
        const auto asset = s.assets.OpenPhysical(path);
        if (asset.expanded) throw std::runtime_error("Original AVI is in a compressed container");
        s.playback.instantiate();
        s.playback->open(asset.range, path, s.material);
        Ref<OriginalVideoStream> stream;
        stream.instantiate();
        stream->set_playback(s.playback);
        s.player.set_stream(stream);
        s.player.play();
        if (!s.player.is_playing()) throw std::runtime_error("Godot could not start the original AVI");
        s.started = true;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        std::fprintf(stderr, "HP2_MVP_TRACE stage=frontend_video movie=%s request=%llu backend=ffmpeg_avi startup_ms=%lld\n",
            path, static_cast<unsigned long long>(request), static_cast<long long>(ms));
    } else if (s.playback.is_valid() && !s.playback->failure().empty()) {
        throw std::runtime_error(s.playback->failure());
    } else if (s.started && !s.player.is_playing()) {
        s.started = false;
        source.CompleteExternalMovie(request);
    }
}

}
