#include "godot_speech_output.hpp"

#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/core/object.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace hp2 {
using namespace godot;

namespace {
constexpr size_t encodedBlockBytes = 1024;
constexpr size_t ringFrames = 16384;
constexpr size_t prebufferFrames = 1024;
static_assert(std::atomic<size_t>::is_always_lock_free &&
    std::atomic<bool>::is_always_lock_free && std::atomic<float>::is_always_lock_free,
    "Godot's mixer must use lock-free speech buffer operations");

void require_audio(WorldAudioResult result, const char* reason) {
    if (result != WORLD_AUDIO_OK) throw std::runtime_error(reason);
}

uint32_t read_be32(const uint8_t* bytes) {
    return uint32_t{bytes[0]} << 24 | uint32_t{bytes[1]} << 16 |
        uint32_t{bytes[2]} << 8 | uint32_t{bytes[3]};
}
}

struct SpeechBuffer {
    uint32_t rate{};
    std::array<int16_t, ringFrames> samples;
    std::atomic<bool> available{true};
    alignas(64) std::atomic<size_t> written{0};
    alignas(64) std::atomic<size_t> consumed{0};
    std::atomic<bool> cancelled{false};
    std::atomic<bool> eof{false};
    std::atomic<float> left{1}, right{1};
    std::atomic<size_t> underruns{0};
    std::exception_ptr failure; // Published by eof.
    uint64_t preparationUs{}; // Published by written or eof.
    std::chrono::steady_clock::time_point queuedAt;

    // The previous playback and producer have released this slot.
    void reset(uint32_t sampleRate) {
        rate = sampleRate;
        written.store(0, std::memory_order_relaxed);
        consumed.store(0, std::memory_order_relaxed);
        cancelled.store(false, std::memory_order_relaxed);
        eof.store(false, std::memory_order_relaxed);
        underruns.store(0, std::memory_order_relaxed);
        failure = {};
        preparationUs = 0;
        queuedAt = std::chrono::steady_clock::now();
    }

    bool ready() const {
        const bool finished = eof.load(std::memory_order_acquire);
        if (finished && failure) std::rethrow_exception(failure);
        const auto available = written.load(std::memory_order_acquire) -
            consumed.load(std::memory_order_relaxed);
        return available >= prebufferFrames || (available && finished);
    }
};

void OriginalSpeechPlayback::initialize(std::shared_ptr<SpeechBuffer> buffer) {
    buffer_ = std::move(buffer);
}

void OriginalSpeechPlayback::_start(double) {
    playing_.store(true, std::memory_order_relaxed);
    begin_resample();
}

void OriginalSpeechPlayback::_stop() { playing_.store(false, std::memory_order_relaxed); }
bool OriginalSpeechPlayback::_is_playing() const { return playing_.load(std::memory_order_relaxed); }
float OriginalSpeechPlayback::_get_stream_sampling_rate() const {
    return static_cast<float>(buffer_->rate);
}

int32_t OriginalSpeechPlayback::_mix_resampled(AudioFrame* destination, int32_t frames) {
    const auto& buffer = *buffer_;
    if (!_is_playing() || buffer.cancelled.load(std::memory_order_relaxed)) {
        std::fill_n(destination, frames, AudioFrame{});
        return 0;
    }
    // Read EOF before the writer index. Seeing EOF guarantees the last
    // published PCM is visible even if the producer finished this instant.
    const bool eof = buffer.eof.load(std::memory_order_acquire);
    const auto first = buffer.consumed.load(std::memory_order_relaxed);
    const auto last = buffer.written.load(std::memory_order_acquire);
    const auto count = static_cast<int32_t>(std::min<size_t>(frames, last - first));
    const auto left = buffer.left.load(std::memory_order_relaxed) / 32768.0f;
    const auto right = buffer.right.load(std::memory_order_relaxed) / 32768.0f;
    for (int32_t index = 0; index < count; ++index) {
        const auto sample = buffer.samples[(first + index) % ringFrames];
        destination[index] = {sample * left, sample * right};
    }
    buffer_->consumed.store(first + count, std::memory_order_release);
    std::fill_n(destination + count, frames - count, AudioFrame{});
    if (count == frames || eof) return count;
    buffer_->underruns.fetch_add(1, std::memory_order_relaxed);
    return frames;
}

void OriginalSpeechStream::initialize(std::shared_ptr<SpeechBuffer> buffer) {
    buffer_ = std::move(buffer);
}

Ref<AudioStreamPlayback> OriginalSpeechStream::_instantiate_playback() const {
    if (!buffer_) return {};
    Ref<OriginalSpeechPlayback> playback;
    playback.instantiate();
    playback->initialize(buffer_);
    return playback;
}

struct GodotSpeechOutput::State {
    struct Job {
        std::array<NativeStreamDecodeRange, 10> parts;
        size_t count{};
        std::shared_ptr<SpeechBuffer> buffer;
    };
    struct Voice {
        WorldAudioVoiceRef sample{};
        uint64_t sequence{};
        uint64_t playerId{};
        NativeVoiceMix mix;
        Job building;
        std::shared_ptr<SpeechBuffer> buffer;
        bool started{}, paused{};

        AudioStreamPlayer* player() const {
            if (!playerId) return nullptr;
            return Object::cast_to<AudioStreamPlayer>(ObjectDB::get_instance(playerId));
        }
    };

    Node& owner;
    std::array<Voice, 48> voices;
    uint8_t mode{2};
    size_t opened{}, started{}, completed{}, underruns{}, consumedFrames{};
    uint64_t maxPreparationUs{};
    std::array<std::shared_ptr<SpeechBuffer>, 3> buffers{
        std::make_shared<SpeechBuffer>(), std::make_shared<SpeechBuffer>(),
        std::make_shared<SpeechBuffer>()};
    std::mutex mutex;
    std::condition_variable wake;
    Job pending;
    std::atomic<bool> quitting{false};
    std::thread worker;

    explicit State(Node& node) : owner(node), worker([this] { run(); }) {}

    ~State() {
        quitting.store(true, std::memory_order_relaxed);
        {
            // Synchronize with the worker's idle wait.
            std::lock_guard<std::mutex> lock(mutex);
        }
        wake.notify_one();
        worker.join();
    }

    bool cancelled(const SpeechBuffer& buffer) const {
        return quitting.load(std::memory_order_relaxed) ||
            buffer.cancelled.load(std::memory_order_relaxed);
    }

    void write(SpeechBuffer& buffer, const std::vector<int16_t>& pcm) {
        auto next = buffer.written.load(std::memory_order_relaxed);
        size_t offset = 0;
        while (offset < pcm.size() && !cancelled(buffer)) {
            const auto free = ringFrames - (next - buffer.consumed.load(std::memory_order_acquire));
            const auto count = std::min({free, pcm.size() - offset, ringFrames - next % ringFrames});
            if (!count) {
                // The mixer only reads samples and publishes its index.
                // Wait here for space without waking or locking the mixer.
                std::unique_lock<std::mutex> lock(mutex);
                wake.wait_for(lock, std::chrono::milliseconds(2));
                continue;
            }
            std::copy_n(pcm.data() + offset, count, buffer.samples.data() + next % ringFrames);
            if (next < prebufferFrames && next + count >= prebufferFrames)
                buffer.preparationUs = elapsed_us(buffer);
            next += count;
            offset += count;
            buffer.written.store(next, std::memory_order_release);
        }
    }

    static uint64_t elapsed_us(const SpeechBuffer& buffer) {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - buffer.queuedAt).count();
    }

    void decode(const Job& job, PcmWave& pcm) {
        auto& buffer = *job.buffer;
        std::array<uint8_t, encodedBlockBytes> encoded;
        for (size_t part = 0; part < job.count && !cancelled(buffer); ++part) {
            const auto& source = job.parts[part];
            auto offset = source.offset;
            auto bytes = source.bytes;
            std::array<uint8_t, 0x30> header{};
            if (bytes < 16 || source.channels != 1 || source.sampleRate != buffer.rate)
                throw std::invalid_argument("Invalid original mono speech segment extent or rate");
            source.file.Read(offset, header.data(), static_cast<size_t>(std::min<uint64_t>(bytes, header.size())));
            const auto magic = read_be32(header.data());
            if (magic == 0x56414770) { // VAGp version 2 mono, as in DecodeSoundStream.
                const auto payload = read_be32(header.data() + 12);
                if (bytes < header.size() || read_be32(header.data() + 4) != 2 ||
                    read_be32(header.data() + 16) != source.sampleRate ||
                    payload > bytes - header.size())
                    throw std::invalid_argument("Invalid original VAGp speech header");
                offset += header.size();
                bytes = payload;
            } else if (magic == 0x52494646) {
                throw std::invalid_argument("Godot speech transport requires original ADPCM, received RIFF");
            }
            if (!bytes || bytes % 16) throw std::invalid_argument("Invalid speech ADPCM block extent");
            Ps2AdpcmHistory history;
            while (bytes && !cancelled(buffer)) {
                const auto count = static_cast<size_t>(std::min<uint64_t>(bytes, encoded.size()));
                source.file.Read(offset, encoded.data(), count);
                DecodePs2AdpcmInto(encoded.data(), count, buffer.rate, 1, 16, false, &history, pcm);
                write(buffer, pcm.samples);
                offset += count;
                bytes -= count;
            }
        }
        if (buffer.written.load(std::memory_order_relaxed) < prebufferFrames)
            buffer.preparationUs = elapsed_us(buffer);
    }

    void run() {
        PcmWave pcm; // Capacity is reused across blocks and sentences.
        for (;;) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mutex);
                wake.wait(lock, [this] { return quitting.load(std::memory_order_relaxed) || pending.buffer; });
                if (quitting.load(std::memory_order_relaxed)) return;
                job = std::move(pending);
            }
            try {
                decode(job, pcm);
            } catch (...) {
                job.buffer->failure = std::current_exception();
            }
            job.buffer->eof.store(true, std::memory_order_release);
        }
    }

    Voice* find(const WorldAudioRequest& operation) {
        for (auto& voice : voices)
            if (voice.sample.instance == operation.instance &&
                voice.sample.generation == operation.instanceGeneration)
                return &voice;
        return nullptr;
    }

    void publish_mix(Voice& voice) {
        if (!voice.buffer) return;
        const auto gain = voice.mix.Coefficients();
        voice.buffer->left.store(gain[0], std::memory_order_relaxed);
        voice.buffer->right.store(gain[1], std::memory_order_relaxed);
    }

    static void set_pitch(Voice& voice, uint32_t frequency) {
        auto* player = voice.player();
        if (!player || !voice.buffer) return;
        if (!frequency) throw std::invalid_argument("Invalid original speech pitch");
        player->set_pitch_scale(static_cast<float>(frequency) / voice.buffer->rate);
    }

    void stop(Voice& voice) {
        if (voice.buffer) {
            voice.buffer->cancelled.store(true, std::memory_order_relaxed);
        }
        if (auto* player = voice.player()) {
            player->stop();
            player->set_stream({});
        }
        if (voice.buffer) {
            underruns += voice.buffer->underruns.load(std::memory_order_relaxed);
            consumedFrames += voice.buffer->consumed.load(std::memory_order_relaxed);
        }
        voice.started = voice.paused = false;
        voice.buffer.reset();
        voice.building = {};
        wake.notify_one();
    }
};

GodotSpeechOutput::GodotSpeechOutput(Node& owner) : state_(std::make_unique<State>(owner)) {}
GodotSpeechOutput::~GodotSpeechOutput() = default;

NativeAudioApply GodotSpeechOutput::Apply(bSound& sound, const WorldAudioRequest& operation) {
    using ApplyResult = NativeAudioApply;
    auto& state = *state_;
    if (operation.instance->retiring && operation.kind != WORLD_AUDIO_STOP)
        return ApplyResult::Cancelled;

    auto* voice = state.find(operation);
    auto assign = [&](State::Voice& target) {
        require_audio(sound.Voices().Assign(operation, static_cast<uint16_t>(&target - state.voices.data())),
            "Original voice manager rejected Godot speech identity");
        target.sequence = operation.sequence;
    };

    if (operation.kind == WORLD_AUDIO_PRIME) {
        const auto id = operation.reservedVoiceId;
        const auto* source = operation.instance->streamSource;
        if (id >= state.voices.size() || operation.reservedVoiceChannels != 1 ||
            !source || source->sampleRate < 1000 || source->sampleRate > 200000)
            throw std::invalid_argument("PRIME has no valid original mono speech reservation");
        auto& target = state.voices[id];
        if (target.sample.instance && voice != &target)
            throw std::invalid_argument("Godot speech voice is still owned by another reservation");
        state.stop(target);
        target.sample = {operation.instance, operation.instanceGeneration};
        target.mix.Initialize(1, false, 0x3fff, 0, state.mode);
        assign(target);
        return ApplyResult::Applied;
    }
    if (operation.kind == WORLD_AUDIO_STOP) {
        if (voice) state.stop(*voice);
        require_audio(sound.Voices().CompleteStop(operation), "Original voice manager rejected Godot speech STOP");
        if (voice && operation.releaseDeviceVoice) voice->sample = {};
        return ApplyResult::Applied;
    }
    if (!voice) throw std::invalid_argument("Godot speech command lost its original primed voice");

    switch (operation.kind) {
    case WORLD_AUDIO_APPEND_CHAIN:
        if (voice->building.count >= voice->building.parts.size() - 1)
            throw std::invalid_argument("Original speech chain exceeds its part capacity");
        voice->building.parts[voice->building.count++] = CaptureStreamDecodeRange(operation);
        break;
    case WORLD_AUDIO_OPEN_STREAM:
    case WORLD_AUDIO_OPEN_CHAIN: {
        std::unique_lock<std::mutex> lock(state.mutex, std::try_to_lock);
        if (!lock || state.pending.buffer) return ApplyResult::Blocked;
        if (voice->buffer) throw std::invalid_argument("Godot speech OPEN requires ordered STOP before reuse");
        const auto available = std::find_if(state.buffers.begin(), state.buffers.end(),
            [](const auto& buffer) { return buffer->available.load(std::memory_order_acquire); });
        if (available == state.buffers.end()) return ApplyResult::Blocked;
        auto range = CaptureStreamDecodeRange(operation);
        auto& building = voice->building;
        const auto& storage = *available;
        storage->available.store(false, std::memory_order_relaxed);
        // The deleter publishes reuse only after both the mixer and worker
        // relinquish this sentence, including Godot's asynchronous STOP.
        building.buffer = std::shared_ptr<SpeechBuffer>(storage.get(), [storage](SpeechBuffer* buffer) {
            buffer->available.store(true, std::memory_order_release);
        });
        building.buffer->reset(range.sampleRate);
        building.parts[building.count++] = std::move(range);
        assign(*voice);
        voice->buffer = building.buffer;
        state.pending = std::move(building);
        building = {};
        ++state.opened;
        lock.unlock();
        state.wake.notify_one();
        break;
    }
    case WORLD_AUDIO_PLAY: {
        if (!voice->buffer || voice->started || !voice->buffer->ready())
            throw std::invalid_argument("Godot speech PLAY has no prepared original stream");
        auto* player = voice->player();
        if (!player) {
            player = memnew(AudioStreamPlayer);
            player->set_playback_type(AudioServer::PLAYBACK_TYPE_STREAM);
            player->set_process_mode(Node::PROCESS_MODE_ALWAYS);
            state.owner.add_child(player);
            voice->playerId = player->get_instance_id();
        }
        Ref<OriginalSpeechStream> stream;
        stream.instantiate();
        stream->initialize(voice->buffer);
        voice->mix.Initialize(1, false, operation.sampleVolume, operation.samplePan, state.mode);
        state.publish_mix(*voice);
        state.set_pitch(*voice, operation.samplePitch);
        player->set_stream(stream);
        player->set_stream_paused(false);
        player->play();
        if (!player->is_playing()) throw std::runtime_error("Godot rejected original speech playback");
        voice->started = true;
        voice->paused = false;
        ++state.started;
        state.maxPreparationUs = std::max(state.maxPreparationUs, voice->buffer->preparationUs);
        assign(*voice);
        break;
    }
    case WORLD_AUDIO_SET_SAMPLE_VOLUME:
        voice->mix.SetVolume(operation.sampleVolume, state.mode);
        state.publish_mix(*voice);
        break;
    case WORLD_AUDIO_SET_CONTROL_CODE:
        voice->mix.SetAzimuth(operation.controlCode, state.mode);
        state.publish_mix(*voice);
        break;
    case WORLD_AUDIO_SET_PITCH_OFFSET:
        state.set_pitch(*voice, operation.samplePitch);
        break;
    case WORLD_AUDIO_SET_MUTED:
        voice->mix.Mute(operation.muted != 0, operation.instance->savedMuteVolume, state.mode);
        state.publish_mix(*voice);
        break;
    case WORLD_AUDIO_SET_PAUSED:
        voice->paused = operation.parameterValue != 0;
        if (auto* player = voice->player()) player->set_stream_paused(voice->paused);
        break;
    case WORLD_AUDIO_FADE_VOLUME:
        // The source evaluates the envelope and publishes SET_SAMPLE_VOLUME.
        if (!std::isfinite(operation.parameterValue) || operation.durationMilliseconds <= 32)
            throw std::invalid_argument("Invalid original speech fade");
        break;
    default:
        throw std::invalid_argument("Unsupported original speech transport command");
    }
    return ApplyResult::Applied;
}

WorldAudioResult GodotSpeechOutput::Observe(bVoiceStatusSnapshot& status) {
    for (size_t id = 0; id < state_->voices.size(); ++id) {
        auto& voice = state_->voices[id];
        if (!voice.sample.instance) continue;
        const bool ready = voice.buffer && voice.buffer->ready();
        if (voice.started && !voice.paused) {
            const auto* player = voice.player();
            if (!player) throw std::runtime_error("Godot speech player was freed during playback");
            if (!player->is_playing()) {
                voice.started = false;
                ++state_->completed;
            }
        }
        status[id] = {voice.sample, voice.sequence, voice.started, ready, 1};
    }
    return WORLD_AUDIO_OK;
}

void GodotSpeechOutput::SetMode(uint8_t mode) {
    state_->mode = mode;
    for (auto& voice : state_->voices) {
        if (!voice.sample.instance) continue;
        voice.mix.SetMode(mode);
        state_->publish_mix(voice);
    }
}

void GodotSpeechOutput::Close(bSound& sound) {
    for (auto& voice : state_->voices) {
        state_->stop(voice);
        const auto sample = voice.sample;
        if (sample.instance && sample.instance->occupied && sample.instance->generation == sample.generation) {
            sample.instance->hasHandle = 0;
            sample.instance->playing = 0;
            sound.Voices().ReleaseSample(sample);
        }
        voice.sample = {};
        if (auto* player = voice.player()) player->queue_free();
        voice.playerId = 0;
    }
    std::fprintf(stderr,
        "HP2_MVP_GODOT_SPEECH opened=%zu started=%zu completed=%zu consumed_frames=%zu underrun_blocks=%zu max_prepare_us=%llu\n",
        state_->opened, state_->started, state_->completed, state_->consumedFrames, state_->underruns,
        static_cast<unsigned long long>(state_->maxPreparationUs));
}
}
