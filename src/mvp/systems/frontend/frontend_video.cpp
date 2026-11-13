#include "frontend_video.hpp"

#include "bsound/include/bSound.h"
#include "bware/inc/bFile.hpp"
#include "indep/src/MoviePlayer/MV3Codec.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/video_stream_player.hpp>
#include <godot_cpp/classes/video_stream_theora.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace hp2 {
using namespace godot;

namespace {
String path_string(const std::filesystem::path& path) {return String(path.c_str());}

void prepare_sound(const std::filesystem::path& input, const std::filesystem::path& output) {
    std::ifstream file(input, std::ios::binary);
    std::vector<std::uint8_t> encoded((std::istreambuf_iterator<char>(file)), {});
    if (!file || encoded.empty()) throw std::runtime_error("FFmpeg returned no authored movie audio");
    const auto pcm = DecodePs2Adpcm(encoded.data(), encoded.size(), 44086, 2, 0x8000, false);
    std::ofstream raw(output, std::ios::binary);
    raw.write(reinterpret_cast<const char*>(pcm.samples.data()), pcm.samples.size() * sizeof(std::int16_t));
    if (!raw) throw std::runtime_error("Could not write decoded movie audio");
}
}

struct FrontendVideo::Storage {
    Storage(VideoStreamPlayer& node, const GameResources& source, const String& executable)
        : player(node), assets(source), ffmpeg(executable) {}
    VideoStreamPlayer& player;
    const GameResources& assets;
    String ffmpeg;
    std::filesystem::path directory, avi, encoded, pcm, ogv;
    std::unordered_map<std::string, std::filesystem::path> converted;
    std::string path;
    std::uint64_t request{};
    std::chrono::steady_clock::time_point deadline;
    int pid{-1};
    bool title{}, extracting{}, started{};

    void launch(PackedStringArray args) {
        pid = OS::get_singleton()->create_process(ffmpeg, args);
        if (pid <= 0) throw std::runtime_error("Could not start FFmpeg for " + path);
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
    }

    void transcode() {
        PackedStringArray args;
        for (const auto* arg : {"-hide_banner", "-loglevel", "error", "-nostdin", "-y", "-i"})
            args.push_back(arg);
        args.push_back(path_string(avi));
        if (title) {
            for (const auto* arg : {"-f", "s16le", "-ar", "44086", "-ac", "2", "-i"})
                args.push_back(arg);
            args.push_back(path_string(pcm));
            for (const auto* arg : {"-map", "0:v:0", "-map", "1:a:0", "-c:a", "libvorbis"})
                args.push_back(arg);
        } else args.push_back("-an");
        for (const auto* arg : {"-vf", "vflip", "-c:v", "libtheora", "-speed_level", "2", "-q:v", "7"})
            args.push_back(arg);
        args.push_back(path_string(ogv));
        launch(args);
    }

    void start_stream(const std::filesystem::path& file) {
        Ref<VideoStreamTheora> stream;
        stream.instantiate();
        stream->set_file(path_string(file));
        player.set_stream(stream);
        player.play();
        if (!player.is_playing()) throw std::runtime_error("Godot could not play the FFmpeg output");
        started = true;
        std::fprintf(stderr, "HP2_MVP_TRACE stage=frontend_video movie=%s request=%llu\n",
            path.c_str(), static_cast<unsigned long long>(request));
    }
};

FrontendVideo::FrontendVideo(VideoStreamPlayer& player, const GameResources& assets, const String& ffmpeg)
    : storage_(std::make_unique<Storage>(player, assets, ffmpeg)) {}

FrontendVideo::~FrontendVideo() {
    auto& s = *storage_;
    if (s.pid > 0 && OS::get_singleton()->is_process_running(s.pid)) OS::get_singleton()->kill(s.pid);
    std::error_code error;
    if (!s.directory.empty()) std::filesystem::remove_all(s.directory, error);
}

bool FrontendVideo::playing() const noexcept {return storage_->player.is_playing();}
std::uint64_t FrontendVideo::request_id() const noexcept {return storage_->request;}

void FrontendVideo::update(MoviePlayer& source) {
    auto& s = *storage_;
    const auto* requested = source.RequestedMovie();
    const auto request = requested ? source.MovieRequestId() : 0;
    if (request != s.request) {
        if (s.pid > 0 && OS::get_singleton()->is_process_running(s.pid)) OS::get_singleton()->kill(s.pid);
        s.pid = -1;
        s.started = false;
        s.player.stop();
        s.player.set_stream({});
        s.request = request;
        if (!requested) return;
        s.path = requested;
        s.title = s.path.rfind("MOVIES\\INTRO\\", 0) == 0;
        if (const auto cached = s.converted.find(s.path); cached != s.converted.end()) {
            s.start_stream(cached->second);
            return;
        }
        if (s.ffmpeg.is_empty()) throw std::runtime_error("Set HP2_FFMPEG to an FFmpeg executable with MV30 and Theora support");
        if (s.directory.empty()) {
            const auto dir = std::filesystem::temp_directory_path() /
                ("hp2-video-" + std::to_string(OS::get_singleton()->get_process_id()));
            if (!std::filesystem::create_directory(dir))
                throw std::runtime_error("Could not create a private temporary video directory");
            s.directory = dir;
        }
        const auto stem = s.directory / std::to_string(request);
        s.avi = stem; s.avi += ".avi";
        s.encoded = stem; s.encoded += ".adpcm";
        s.pcm = stem; s.pcm += ".pcm";
        s.ogv = stem; s.ogv += ".ogv";
        const auto asset = s.assets.OpenPhysical(s.path);
        if (asset.expanded) throw std::runtime_error("Frontend AVI is stored in an unsupported compressed container");
        asset.range.SaveTo(s.avi);
        s.extracting = s.title;
        if (s.extracting) {
            PackedStringArray args;
            for (const auto* arg : {"-hide_banner", "-loglevel", "error", "-nostdin", "-y", "-i"})
                args.push_back(arg);
            args.push_back(path_string(s.avi));
            for (const auto* arg : {"-map", "0:a:0", "-c", "copy", "-f", "data"})
                args.push_back(arg);
            args.push_back(path_string(s.encoded));
            s.launch(args);
        } else s.transcode();
        return;
    }
    if (!request) return;
    if (s.pid > 0) {
        auto& os = *OS::get_singleton();
        if (os.is_process_running(s.pid)) {
            if (std::chrono::steady_clock::now() > s.deadline)
                throw std::runtime_error("FFmpeg timed out for " + s.path);
            return;
        }
        const auto result = os.get_process_exit_code(s.pid);
        s.pid = -1;
        if (result != 0) throw std::runtime_error("FFmpeg failed for " + s.path + ", exit=" + std::to_string(result));
        if (s.extracting) {
            prepare_sound(s.encoded, s.pcm);
            s.extracting = false;
            s.transcode();
        } else {
            s.start_stream(s.ogv);
            s.converted.emplace(s.path, s.ogv);
            std::filesystem::remove(s.avi);
            std::filesystem::remove(s.encoded);
            std::filesystem::remove(s.pcm);
        }
    } else if (s.started && !s.player.is_playing()) {
        s.started = false;
        source.CompleteExternalMovie(request);
    }
}

}
