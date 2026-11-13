#include "original_video_stream.hpp"

#include "bsound/include/bSound.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/shader_material.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace hp2 {
using namespace godot;

namespace {
void check_av(int result, const char* action) {
    if (result >= 0) return;
    char error[AV_ERROR_MAX_STRING_SIZE]{};
    av_strerror(result, error, sizeof(error));
    throw std::runtime_error(std::string(action) + ": " + error);
}

struct MovieInput {
    explicit MovieInput(FileRange range) : file(std::move(range)) {}
    ~MovieInput() {
        avformat_close_input(&format);
        if (io) av_freep(&io->buffer);
        avio_context_free(&io);
    }

    void open() {
        auto* buffer = static_cast<unsigned char*>(av_malloc(64 * 1024));
        if (!buffer) throw std::bad_alloc();
        io = avio_alloc_context(buffer, 64 * 1024, 0, this, read, nullptr, seek);
        if (!io) {
            av_free(buffer);
            throw std::bad_alloc();
        }
        format = avformat_alloc_context();
        if (!format) throw std::bad_alloc();
        format->pb = io;
        format->flags |= AVFMT_FLAG_CUSTOM_IO;
        check_av(avformat_open_input(&format, nullptr, av_find_input_format("avi"), nullptr), "Opening original AVI");
    }

    static int read(void* user, std::uint8_t* data, int bytes) noexcept {
        auto& input = *static_cast<MovieInput*>(user);
        const auto count = std::min<std::uint64_t>(bytes, input.file.Size() - input.position);
        if (!count) return AVERROR_EOF;
        try {
            input.file.Read(input.position, data, static_cast<std::size_t>(count));
        } catch (...) {
            return AVERROR(EIO);
        }
        input.position += count;
        return static_cast<int>(count);
    }

    static std::int64_t seek(void* user, std::int64_t offset, int flags) noexcept {
        auto& input = *static_cast<MovieInput*>(user);
        const auto size = static_cast<std::int64_t>(input.file.Size());
        const int origin = flags & ~AVSEEK_FORCE;
        if (origin == AVSEEK_SIZE) return size;
        std::int64_t base;
        switch (origin) {
        case SEEK_SET: base = 0; break;
        case SEEK_CUR: base = static_cast<std::int64_t>(input.position); break;
        case SEEK_END: base = size; break;
        default: return AVERROR(EINVAL);
        }
        if (offset < -base || offset > size - base) return AVERROR(EINVAL);
        input.position = base + offset;
        return static_cast<std::int64_t>(input.position);
    }

    FileRange file;
    std::uint64_t position{};
    AVIOContext* io{};
    AVFormatContext* format{};
};

constexpr int audioRate = 44086;
constexpr std::size_t stereoBlockBytes = 0x10000;
}

struct OriginalVideoPlayback::Storage {
    std::unique_ptr<MovieInput> input, audioInput;
    AVCodecContext* decoder{};
    AVPacket* packet{av_packet_alloc()};
    AVPacket* audioPacket{av_packet_alloc()};
    AVFrame* frame{av_frame_alloc()};
    Ref<ShaderMaterial> material;
    std::array<Ref<ImageTexture>,3> planes;
    std::string path, error;
    int videoIndex{-1}, audioIndex{-1};
    double clock{}, duration{}, interval{}, nextFrame{};
    std::size_t frames{}, audioSent{}, audioOffset{}, encodedOffset{};
    bool playing{}, paused{}, videoEof{}, audioEof{}, draining{};
    Ps2AdpcmHistory history;
    std::vector<std::uint8_t> encoded;
    PackedFloat32Array samples;

    ~Storage() {
        av_frame_free(&frame);
        av_packet_free(&audioPacket);
        av_packet_free(&packet);
        avcodec_free_context(&decoder);
    }

    bool decode_video() {
        for (;;) {
            const int result = avcodec_receive_frame(decoder, frame);
            if (result == 0) return true;
            if (result == AVERROR_EOF) return false;
            if (result != AVERROR(EAGAIN)) check_av(result, "Decoding MV30");
            if (draining) return false;
            for (;;) {
                const auto read = av_read_frame(input->format, packet);
                if (read == AVERROR_EOF) {
                    check_av(avcodec_send_packet(decoder, nullptr), "Draining MV30");
                    draining = true;
                    break;
                }
                check_av(read, "Reading original AVI");
                const bool video = packet->stream_index == videoIndex;
                const int sent = video ? avcodec_send_packet(decoder, packet) : 0;
                av_packet_unref(packet);
                if (video) {
                    check_av(sent, "Sending MV30 packet");
                    break;
                }
            }
        }
    }

    void show_frame() {
        if (frame->format != AV_PIX_FMT_YUV420P && frame->format != AV_PIX_FMT_YUVJ420P)
            throw std::runtime_error("Unexpected MV30 decoded pixel format");
        for (int plane = 0; plane < 3; ++plane) {
            const int width = plane ? (frame->width + 1) / 2 : frame->width;
            const int height = plane ? (frame->height + 1) / 2 : frame->height;
            PackedByteArray data;
            data.resize(std::int64_t(width) * height);
            auto* bytes = data.ptrw();
            for (int y = 0; y < height; ++y)
                std::memcpy(bytes + y * width, frame->data[plane] +
                    std::ptrdiff_t(y) * frame->linesize[plane], width);
            const auto image = Image::create_from_data(width, height, false, Image::FORMAT_R8, data);
            if (image.is_null()) throw std::runtime_error("Godot rejected an MV30 texture plane");
            if (planes[plane].is_valid()) planes[plane]->update(image);
            else planes[plane] = ImageTexture::create_from_image(image);
            if (planes[plane].is_null()) throw std::runtime_error("Godot could not allocate an MV30 texture plane");
        }
        material->set_shader_parameter("chroma_u", planes[1]);
        material->set_shader_parameter("chroma_v", planes[2]);
    }

    bool decode_audio() {
        while (encoded.size() - encodedOffset < stereoBlockBytes && !audioEof) {
            const auto result = av_read_frame(audioInput->format, audioPacket);
            if (result == AVERROR_EOF) {
                audioEof = true;
                break;
            }
            check_av(result, "Reading original movie audio");
            if (audioPacket->stream_index == audioIndex && audioPacket->size > 0)
                encoded.insert(encoded.end(), audioPacket->data, audioPacket->data + audioPacket->size);
            av_packet_unref(audioPacket);
        }
        const auto count = std::min(stereoBlockBytes, encoded.size() - encodedOffset);
        if (!count) return false;
        const auto pcm = DecodePs2Adpcm(encoded.data() + encodedOffset, count, audioRate, 2, 0x8000, false, &history);
        encodedOffset += count;
        if (encodedOffset >= stereoBlockBytes) {
            encoded.erase(encoded.begin(), encoded.begin() + encodedOffset);
            encodedOffset = 0;
        }
        samples.resize(pcm.samples.size());
        auto* output = samples.ptrw();
        std::transform(pcm.samples.begin(), pcm.samples.end(), output,
            [](std::int16_t value) { return value / 32768.0f; });
        audioOffset = 0;
        return !samples.is_empty();
    }
};

OriginalVideoPlayback::OriginalVideoPlayback() = default;
OriginalVideoPlayback::~OriginalVideoPlayback() = default;

void OriginalVideoPlayback::open(const FileRange& file, const std::string& path, const Ref<ShaderMaterial>& material) {
    auto s = std::make_unique<Storage>();
    s->path = path;
    s->material = material;
    if (!s->packet || !s->audioPacket || !s->frame) throw std::bad_alloc();
    s->input = std::make_unique<MovieInput>(file);
    s->input->open();
    auto& format = *s->input->format;
    for (unsigned i = 0; i < format.nb_streams; ++i) {
        const auto& parameters = *format.streams[i]->codecpar;
        if (parameters.codec_id == AV_CODEC_ID_MV30 && s->videoIndex < 0) s->videoIndex = i;
        if (parameters.codec_type == AVMEDIA_TYPE_AUDIO && s->audioIndex < 0) s->audioIndex = i;
    }
    if (s->videoIndex < 0) throw std::runtime_error("Original AVI has no MV30 stream: " + path);
    const auto& video = *format.streams[s->videoIndex];
    const auto* codec = avcodec_find_decoder(AV_CODEC_ID_MV30);
    if (!codec) throw std::runtime_error("FFmpeg SDK lacks its MV30 decoder");
    s->decoder = avcodec_alloc_context3(codec);
    if (!s->decoder) throw std::bad_alloc();
    check_av(avcodec_parameters_to_context(s->decoder, video.codecpar), "Reading MV30 parameters");
    check_av(avcodec_open2(s->decoder, codec, nullptr), "Opening MV30 decoder");
    s->interval = av_q2d(video.time_base);
    if (!std::isfinite(s->interval) || s->interval <= 0 || s->interval > 1)
        throw std::runtime_error("Original AVI has an invalid video time base");
    s->duration = video.duration > 0 ? video.duration * s->interval : 0;
    if (s->audioIndex >= 0) {
        s->audioInput = std::make_unique<MovieInput>(file);
        s->audioInput->open();
    }
    if (!s->decode_video()) throw std::runtime_error("Original AVI has no MV30 frames");
    s->show_frame();
    s->frames = 1;
    s->nextFrame = s->interval;
    storage_ = std::move(s);
}

const std::string& OriginalVideoPlayback::failure() const noexcept { return storage_->error; }
std::size_t OriginalVideoPlayback::submitted_audio_frames() const noexcept { return storage_->audioSent; }
void OriginalVideoPlayback::_play() { if (storage_) storage_->playing = true; }
void OriginalVideoPlayback::_stop() { if (storage_) storage_->playing = false; }
bool OriginalVideoPlayback::_is_playing() const { return storage_ && storage_->playing; }
void OriginalVideoPlayback::_set_paused(bool paused) { if (storage_) storage_->paused = paused; }
bool OriginalVideoPlayback::_is_paused() const { return storage_ && storage_->paused; }
double OriginalVideoPlayback::_get_length() const { return storage_ ? storage_->duration : 0; }
double OriginalVideoPlayback::_get_playback_position() const { return storage_ ? storage_->clock : 0; }
int32_t OriginalVideoPlayback::_get_channels() const { return storage_ && storage_->audioInput ? 2 : 0; }
int32_t OriginalVideoPlayback::_get_mix_rate() const { return audioRate; }
Ref<Texture2D> OriginalVideoPlayback::_get_texture() const { return storage_ ? Ref<Texture2D>(storage_->planes[0]) : Ref<Texture2D>(); }

void OriginalVideoPlayback::_update(double delta) {
    if (!_is_playing() || _is_paused()) return;
    auto& s = *storage_;
    try {
        s.clock += delta;
        if (s.audioInput) {
            const auto target = static_cast<std::size_t>((s.clock + 0.12) * audioRate);
            while (s.audioSent < target) {
                if (s.audioOffset == std::size_t(s.samples.size()) && !s.decode_audio()) break;
                const auto available = (s.samples.size() - s.audioOffset) / 2;
                const auto accepted = mix_audio(std::min<std::size_t>(available, target - s.audioSent), s.samples, s.audioOffset);
                if (accepted <= 0) break;
                s.audioSent += accepted;
                s.audioOffset += 2 * accepted;
            }
        }
        // PAL movie audio starts before video. Decode all intervening MV30
        // delta frames, and upload only the latest frame due for presentation.
        const auto videoTime = s.clock - (s.audioInput ? 0.16 : 0.0);
        bool changed = false;
        for (int decoded = 0; decoded < 8 && videoTime >= s.nextFrame && !s.videoEof; ++decoded) {
            if (!s.decode_video()) {
                s.videoEof = true;
                break;
            }
            s.nextFrame = ++s.frames * s.interval;
            changed = true;
        }
        if (changed) s.show_frame();
        if (s.videoEof) s.playing = false;
    } catch (const std::exception& error) {
        s.error = s.path + ": " + error.what();
        s.playing = false;
    }
}

void OriginalVideoStream::set_playback(const Ref<OriginalVideoPlayback>& playback) { playback_ = playback; }
Ref<VideoStreamPlayback> OriginalVideoStream::_instantiate_playback() { return playback_; }

}
