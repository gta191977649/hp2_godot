#include "godot_audio_device.hpp"

#include "godot_speech_output.hpp"

#include <godot_cpp/classes/audio_effect_reverb.hpp>
#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <future>
#include <map>
#include <utility>

namespace hp2 {
using namespace godot;

namespace {
constexpr size_t ringFrames = 65536;
static_assert(std::atomic<size_t>::is_always_lock_free && std::atomic<float>::is_always_lock_free &&
    std::atomic<bool>::is_always_lock_free, "Godot mixer PCM reads must be lock free");
struct AudioFailure { WorldAudioResult result; const char* reason; };

void require_audio(WorldAudioResult result, const char* reason)
{
    if (result != WORLD_AUDIO_OK) throw AudioFailure{result, reason};
}

void check(bool valid, const char* reason)
{
    if (!valid) throw AudioFailure{WORLD_AUDIO_INVALID_BINDING, reason};
}
}

struct DevicePcm {
    const uint32_t rate;
    const uint16_t channels;
    const std::shared_ptr<const PcmWave> resident;
    const bool loop;
    std::vector<AudioFrame> ring;
    std::atomic<size_t> written{0}, consumed{0};
    std::atomic<bool> ended{false}, cancelled{false};
    std::array<std::atomic<float>, 4> coefficients{};

    DevicePcm(uint32_t sampleRate, uint16_t count, std::shared_ptr<const PcmWave> cue = {}, bool repeat = false)
        : rate(sampleRate), channels(count), resident(std::move(cue)), loop(repeat),
          ring(resident ? 0 : ringFrames)
    {
        check(rate && (channels == 1 || channels == 2), "Invalid PCM format");
    }

    size_t buffered() const
    {
        return written.load(std::memory_order_acquire) - consumed.load(std::memory_order_acquire);
    }

    size_t write(const PcmWave& wave, size_t offset)
    {
        check(wave.channels == channels && wave.sampleRate == rate, "Stream PCM format changed");
        const auto next = written.load(std::memory_order_relaxed);
        const auto count = std::min(wave.samples.size() / channels - offset, ringFrames - buffered());
        for (size_t i = 0; i < count; ++i) {
            const auto* sample = wave.samples.data() + (offset + i) * channels;
            ring[(next + i) % ringFrames] = {sample[0] / 32768.0f, channels == 2 ? sample[1] / 32768.0f : 0.0f};
        }
        written.store(next + count, std::memory_order_release);
        return count;
    }
};

void DevicePcmPlayback::_start(double)
{
    position_ = 0;
    playing_.store(true, std::memory_order_relaxed);
    begin_resample();
}

void DevicePcmPlayback::_stop() { playing_.store(false, std::memory_order_relaxed); }
bool DevicePcmPlayback::_is_playing() const { return playing_.load(std::memory_order_relaxed); }
float DevicePcmPlayback::_get_stream_sampling_rate() const { return static_cast<float>(pcm->rate); }

int32_t DevicePcmPlayback::_mix_resampled(AudioFrame* destination, int32_t frames)
{
    std::array<float, 4> gain{};
    for (size_t i = 0; i < gain.size(); ++i)
        gain[i] = pcm->coefficients[i].load(std::memory_order_relaxed);
    int32_t count = 0;
    if (_is_playing() && !pcm->cancelled.load(std::memory_order_relaxed)) {
        if (const auto& wave = pcm->resident) {
            const auto end = wave->samples.size() / pcm->channels;
            const auto loopEnd = wave->loopEndFrame > wave->loopStartFrame
                ? std::min<size_t>(wave->loopEndFrame, end) : end;
            while (count < frames && position_ < end) {
                const auto* source = wave->samples.data() + position_ * pcm->channels;
                const auto left = source[0] / 32768.0f;
                const auto right = pcm->channels == 2 ? source[1] / 32768.0f : 0.0f;
                destination[count++] = {left * gain[0] + right * gain[2], left * gain[1] + right * gain[3]};
                if (++position_ == loopEnd && pcm->loop) position_ = wave->loopEndFrame > wave->loopStartFrame
                    ? wave->loopStartFrame : 0;
            }
        } else {
            const bool ended = pcm->ended.load(std::memory_order_acquire);
            const auto first = pcm->consumed.load(std::memory_order_relaxed);
            const auto last = pcm->written.load(std::memory_order_acquire);
            count = static_cast<int32_t>(std::min<size_t>(frames, last - first));
            for (int32_t i = 0; i < count; ++i) {
                const auto frame = pcm->ring[(first + i) % ringFrames];
                destination[i] = {frame.left * gain[0] + frame.right * gain[2], frame.left * gain[1] + frame.right * gain[3]};
            }
            pcm->consumed.store(first + count, std::memory_order_release);
            std::fill(destination + count, destination + frames, AudioFrame{});
            if (!ended) return frames;
        }
    }
    std::fill(destination + count, destination + frames, AudioFrame{});
    return count;
}

Ref<AudioStreamPlayback> DevicePcmStream::_instantiate_playback() const
{
    Ref<DevicePcmPlayback> playback;
    playback.instantiate();
    playback->pcm = pcm;
    return playback;
}

struct GodotAudioDevice::State {
    struct Voice {
        WorldAudioVoiceRef sample{};
        uint64_t sequence{};
        uint64_t playerId{};
        const WorldAudioBank* bank{};
        const WorldAudioStreamSource* movie{};
        std::shared_ptr<DevicePcm> pcm;
        NativeVoiceMix mix{};
        Ps2AdpcmHistory movieHistory{};
        std::future<NativeJukeboxStart> openMusic;
        std::shared_ptr<NativeJukeboxStream> music;
        std::future<NativeJukeboxStream::Chunk> musicChunk;
        std::deque<PcmWave> pending;
        size_t offset{};
        bool started{}, opened{}, paused{}, endOfFile{}, wet{};

        AudioStreamPlayer* player() const
        {
            return playerId ? Object::cast_to<AudioStreamPlayer>(ObjectDB::get_instance(playerId)) : nullptr;
        }

        void publish_mix()
        {
            if (!pcm) return;
            const auto gain = mix.Coefficients();
            for (size_t i = 0; i < gain.size(); ++i)
                pcm->coefficients[i].store(gain[i], std::memory_order_relaxed);
        }

        void retire()
        {
            if (pcm) pcm->cancelled.store(true, std::memory_order_relaxed);
            if (auto* node = player()) {
                node->stop();
                node->set_stream({});
                node->queue_free();
            }
            *this = Voice{};
        }
    };

    struct Effect {
        StringName bus;
        Ref<AudioEffectReverb> reverb;
        uint16_t volume{};
        bool enabled{};
    };

    Node& owner;
    GodotSpeechOutput speech;
    bSound* sound{};
    WorldAudioRequestQueue* queue{};
    NativeAudioCommandPump pump;
    std::array<Voice, 48> voices;
    std::array<Effect, 2> effects;
    std::map<const WorldAudioBank*, NativeSoundBankCache> banks;
    uint8_t mode{2};

    explicit State(Node& node) : owner(node), speech(node) {}

    template <typename T> static bool ready(const std::future<T>& task)
    {
        return task.valid() && task.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    void assign(Voice& voice, const WorldAudioRequest& request)
    {
        require_audio(sound->Voices().Assign(request, static_cast<uint16_t>(&voice - voices.data())),
            "Godot device voice assignment failed");
        voice.sequence = request.sequence;
    }

    Voice& reserve(const WorldAudioRequest& request)
    {
        check(request.instance && request.reservedVoiceId < voices.size() &&
            (request.reservedVoiceChannels == 1 || request.reservedVoiceChannels == 2) &&
            request.reservedVoiceId + request.reservedVoiceChannels <= voices.size(), "Invalid Godot device reservation");
        auto& voice = voices[request.reservedVoiceId];
        check(!voice.sample.instance || (voice.sample.instance == request.instance &&
            voice.sample.generation == request.instanceGeneration), "Godot device reservation was not retired");
        voice.retire();
        voice.sample = {request.instance, request.instanceGeneration};
        voice.bank = request.bank;
        voice.wet = request.cue && request.cue->metadata && request.cue->metadataSize == 52 &&
            (request.cue->metadata[0x26] || request.cue->metadata[0x27]);
        voice.mix.Initialize(request.reservedVoiceChannels, request.instance->sampleType == 0, 0x3fff, 0, mode);
        return voice;
    }

    Voice* find(const WorldAudioRequest& request)
    {
        const auto voice = std::find_if(voices.begin(), voices.end(), [&](const auto& cell) {
            return cell.sample.instance == request.instance && cell.sample.generation == request.instanceGeneration;
        });
        return voice == voices.end() ? nullptr : &*voice;
    }

    void refill(Voice& voice)
    {
        if (!voice.pcm || voice.pcm->resident) return;
        if (ready(voice.musicChunk)) {
            auto chunk = voice.musicChunk.get();
            voice.pending.push_back(std::move(chunk.pcm));
            voice.endOfFile = chunk.last;
        }
        if (voice.movie && voice.pending.empty() && voice.pcm->buffered() < ringFrames / 2) {
            PcmWave chunk;
            if (ReadMoviePcmChunk(*voice.movie, voice.movieHistory, chunk)) voice.pending.push_back(std::move(chunk));
        }
        while (!voice.pending.empty()) {
            auto& chunk = voice.pending.front();
            voice.offset += voice.pcm->write(chunk, voice.offset);
            if (voice.offset < chunk.samples.size() / chunk.channels) break;
            voice.offset = 0;
            voice.pending.pop_front();
        }
        if (voice.endOfFile && voice.pending.empty())
            voice.pcm->ended.store(true, std::memory_order_release);
        if (voice.music && !voice.endOfFile && !voice.musicChunk.valid() &&
            voice.pending.empty() && voice.pcm->buffered() < ringFrames / 2) {
            voice.musicChunk = std::async(std::launch::async, [stream = voice.music] { return stream->Read(); });
        }
    }

    void observe(const WorldAudioRequest& request)
    {
        bVoiceStatusSnapshot snapshot{};
        for (size_t id = 0; id < voices.size(); ++id) {
            auto& voice = voices[id];
            if (!voice.sample.instance) continue;
            const auto* player = voice.player();
            const bool running = voice.started && (voice.paused || (player && player->is_playing()));
            const bool buffered = voice.pcm && (voice.pcm->resident || voice.pcm->buffered() != 0);
            snapshot[id] = {voice.sample, voice.sequence, running,
                voice.opened && buffered, static_cast<uint16_t>(voice.mix.channels)};
        }
        require_audio(speech.Observe(snapshot), "Godot speech observation failed");
        require_audio(sound->Voices().CompleteUpdate(request, snapshot), "Godot device observation failed");
        for (const auto& observed : snapshot) {
            const auto* sample = observed.sample.instance;
            if (!observed.prebuffered || !sample || !sample->occupied || sample->retiring ||
                sample->generation != observed.sample.generation || sample->prebuffered ||
                sample->playSequence != observed.operationSequence) continue;
            const auto result = sample->sampleType == 1
                ? worldAudioSpeechEvent(queue, observed.sample, observed.operationSequence, 4)
                : worldAudioStreamEvent(queue, observed.sample, observed.operationSequence, 4);
            require_audio(result, "Godot device stream readiness failed");
        }
    }

    void update_effect(Effect& effect)
    {
        effect.reverb->set_wet(effect.enabled ? effect.volume / 32768.0f : 0.0f);
    }

    void initialize()
    {
        auto& server = *AudioServer::get_singleton();
        for (size_t i = 0; i < effects.size(); ++i) {
            auto& effect = effects[i];
            const auto id = server.get_bus_count();
            server.add_bus();
            server.set_bus_name(id, String("HP2_Reverb_") + String::num_uint64(owner.get_instance_id()) +
                "_" + String::num_uint64(i));
            effect.bus = server.get_bus_name(id);
            effect.reverb.instantiate();
            effect.reverb->set_dry(1.0f);
            update_effect(effect);
            server.add_bus_effect(id, effect.reverb);
        }
    }

    void effect_command(const WorldAudioRequest& request)
    {
        check(request.effectCore >= 0 && request.effectCore < static_cast<int32_t>(effects.size()), "Invalid reverb core");
        auto& effect = effects[request.effectCore];
        switch (request.kind) {
        case WORLD_AUDIO_SET_CORE_EFFECT_VOLUME: effect.volume = request.effectCoreVolume; break;
        case WORLD_AUDIO_SET_CORE_EFFECT_PAN:
            // Godot reverb keeps the input stereo image. The source voice
            // still uses the authored azimuth matrix for both channels.
            break;
        case WORLD_AUDIO_SET_CORE_EFFECT_ENABLED: effect.enabled = request.effectPreset != 0; break;
        case WORLD_AUDIO_SET_CORE_EFFECT_ATTRIBUTE:
            switch (request.effectParameter) {
            case 1: check(request.effectPreset == 6, "Unsupported reverb preset"); effect.enabled = false; break;
            case 2: effect.volume = static_cast<uint16_t>(request.effectPreset); break;
            case 3: break;
            case 4: case 5: check(request.effectPreset == 0, "Unsupported reverb extension"); break;
            default: check(false, "Unsupported reverb parameter");
            }
            break;
        default: check(false, "Unsupported reverb operation");
        }
        update_effect(effect);
    }

    void play(Voice* voice, const WorldAudioRequest& request)
    {
        if (request.instance->sampleType == 0) {
            const auto bank = banks.find(request.bank);
            check(bank != banks.end() && request.cue && request.cue->metadataSize == 52, "Resident cue has no loaded bank");
            const auto base = reinterpret_cast<uintptr_t>(request.bank->metadata);
            const auto address = reinterpret_cast<uintptr_t>(request.cue->metadata);
            check(base && address >= base && address - base <= request.bank->metadataSize &&
                request.bank->metadataSize - (address - base) >= 52, "Resident cue exceeds bank metadata");
            auto pcm = bank->second.Acquire(address - base);
            voice = &reserve(request);
            const auto rate = pcm->sampleRate;
            const auto channels = pcm->channels;
            const bool loop = GetLoop(request.cue->metadata[0x21]) != 0;
            voice->pcm = std::make_shared<DevicePcm>(rate, channels, std::move(pcm), loop);
        }
        check(voice && voice->pcm && (voice->opened || voice->pcm->resident), "PLAY has no prepared PCM");
        auto* player = voice->player();
        if (!player) {
            player = memnew(AudioStreamPlayer);
            player->set_playback_type(AudioServer::PLAYBACK_TYPE_STREAM);
            player->set_process_mode(Node::PROCESS_MODE_ALWAYS);
            owner.add_child(player);
            voice->playerId = player->get_instance_id();
        }
        if (voice->wet) player->set_bus(effects[static_cast<size_t>(voice - voices.data()) / 24].bus);
        voice->mix.Initialize(voice->pcm->channels, request.instance->sampleType == 0,
            request.sampleVolume, request.samplePan, mode);
        voice->publish_mix();
        pitch(*voice, request.samplePitch);
        Ref<DevicePcmStream> stream;
        stream.instantiate();
        stream->pcm = voice->pcm;
        player->set_stream(stream);
        player->play();
        check(player->is_playing(), "Godot rejected audio playback");
        voice->started = true;
        assign(*voice, request);
    }

    static void pitch(Voice& voice, uint32_t frequency)
    {
        if (auto* player = voice.player()) {
            check(frequency != 0, "Invalid source pitch");
            player->set_pitch_scale(static_cast<float>(frequency) / voice.pcm->rate);
        }
    }

    NativeAudioApply open(Voice& voice, const WorldAudioRequest& request)
    {
        if (request.streamSource && request.streamSource->movie) {
            if (!voice.pcm) {
                voice.movie = request.streamSource;
                voice.pcm = std::make_shared<DevicePcm>(voice.movie->sampleRate, 2);
            }
            refill(voice);
            if (!voice.pcm->buffered()) return NativeAudioApply::Blocked;
        } else if (request.instance->completionKind == WORLD_AUDIO_COMPLETION_JUKEBOX) {
            if (!voice.openMusic.valid()) {
                auto range = CaptureStreamDecodeRange(request);
                voice.openMusic = std::async(std::launch::async, [range = std::move(range)] {
                    return PrepareJukeboxStream(range);
                });
                return NativeAudioApply::Blocked;
            }
            if (!ready(voice.openMusic)) return NativeAudioApply::Blocked;
            auto music = voice.openMusic.get();
            voice.pcm = std::make_shared<DevicePcm>(music.first.pcm.sampleRate, music.first.pcm.channels);
            voice.music = std::move(music.stream);
            voice.endOfFile = music.first.last;
            voice.pending.push_back(std::move(music.first.pcm));
            refill(voice);
        } else {
            auto pcm = DecodeSoundStreamRange(request);
            if (!voice.pcm) voice.pcm = std::make_shared<DevicePcm>(pcm.sampleRate, pcm.channels);
            voice.pending.push_back(std::move(pcm));
            voice.endOfFile = request.kind != WORLD_AUDIO_APPEND_CHAIN;
            refill(voice);
            if (!voice.endOfFile) return NativeAudioApply::Applied;
        }
        voice.opened = true;
        assign(voice, request);
        return NativeAudioApply::Applied;
    }

    NativeAudioApply apply(const WorldAudioRequest& request)
    {
        using Applied = NativeAudioApply;
        if (request.instance && request.instance->sampleType == 1) return speech.Apply(*sound, request);
        if (request.instance && request.instance->retiring && request.kind != WORLD_AUDIO_STOP)
            return Applied::Cancelled;
        auto* voice = request.instance ? find(request) : nullptr;
        switch (request.kind) {
        case WORLD_AUDIO_INITIALIZE_DEVICE: initialize(); break;
        case WORLD_AUDIO_LOAD_REGION:
            check(request.bank && !banks.count(request.bank), "Invalid Godot device bank load");
            banks.try_emplace(request.bank, *request.bank);
            require_audio(sound->CompleteRegion(request, true), "Godot device bank completion failed");
            break;
        case WORLD_AUDIO_UNLOAD_BANK:
            for (auto& held : voices) {
                if (held.bank != request.bank) continue;
                const auto sample = held.sample;
                held.retire();
                sound->Voices().ReleaseSample(sample);
            }
            check(banks.erase(request.bank) == 1, "Godot device bank is not loaded");
            break;
        case WORLD_AUDIO_DRIVER_UPDATE: observe(request); break;
        case WORLD_AUDIO_SET_REGION_PATH:
            check(std::memchr(request.resourcePath, 0, sizeof(request.resourcePath)) != nullptr, "Invalid audio resource path");
            break;
        case WORLD_AUDIO_SET_CHANNEL_VOLUME:
            check(request.channel >= 0 && request.channel < 16 && std::isfinite(request.parameterValue), "Invalid category gain");
            break;
        case WORLD_AUDIO_SET_AUDIO_MODE:
            check(request.audioMode >= 0 && request.audioMode <= 2, "Invalid audio mode");
            mode = static_cast<uint8_t>(request.audioMode + 1);
            speech.SetMode(mode);
            for (auto& held : voices) {
                if (!held.sample.instance) continue;
                held.mix.SetMode(mode);
                held.publish_mix();
            }
            break;
        case WORLD_AUDIO_SET_CORE_EFFECT_VOLUME: case WORLD_AUDIO_SET_CORE_EFFECT_PAN:
        case WORLD_AUDIO_SET_CORE_EFFECT_ATTRIBUTE: case WORLD_AUDIO_SET_CORE_EFFECT_ENABLED:
            effect_command(request); break;
        case WORLD_AUDIO_PRIME: assign(reserve(request), request); break;
        case WORLD_AUDIO_PLAY: play(voice, request); break;
        case WORLD_AUDIO_OPEN_STREAM: case WORLD_AUDIO_APPEND_CHAIN: case WORLD_AUDIO_OPEN_CHAIN:
            if (!voice) voice = &reserve(request);
            return open(*voice, request);
        case WORLD_AUDIO_STOP:
            if (voice) {
                const auto sample = voice->sample;
                const auto sequence = voice->sequence;
                const auto channels = voice->mix.channels;
                voice->retire();
                if (!request.releaseDeviceVoice) {
                    voice->sample = sample;
                    voice->sequence = sequence;
                    voice->mix.Initialize(channels, request.instance->sampleType == 0, 0x3fff, 0, mode);
                }
            }
            require_audio(sound->Voices().CompleteStop(request), "Godot device STOP failed");
            break;
        case WORLD_AUDIO_SET_SAMPLE_VOLUME:
            check(voice != nullptr, "Gain command has no voice");
            voice->mix.SetVolume(request.sampleVolume, mode); voice->publish_mix(); break;
        case WORLD_AUDIO_SET_PITCH_OFFSET:
            check(voice != nullptr, "Pitch command has no voice");
            pitch(*voice, request.samplePitch); break;
        case WORLD_AUDIO_SET_CONTROL_CODE:
            check(voice != nullptr, "Azimuth command has no voice");
            voice->mix.SetAzimuth(request.controlCode, mode); voice->publish_mix(); break;
        case WORLD_AUDIO_SET_MUTED:
            check(voice != nullptr, "Mute command has no voice");
            voice->mix.Mute(request.muted != 0, request.instance->savedMuteVolume, mode); voice->publish_mix(); break;
        case WORLD_AUDIO_SET_PAUSED:
            check(voice != nullptr, "Pause command has no voice");
            voice->paused = request.parameterValue != 0;
            if (auto* player = voice->player()) player->set_stream_paused(voice->paused);
            break;
        case WORLD_AUDIO_FADE_VOLUME:
            check(std::isfinite(request.parameterValue) && request.durationMilliseconds > 32, "Invalid fade");
            break;
        default: throw AudioFailure{WORLD_AUDIO_BACKEND_FAILURE, "Unsupported Godot audio request"};
        }
        return Applied::Applied;
    }

    WorldAudioResult step(size_t limit)
    {
        try {
            for (auto& voice : voices) refill(voice);
            return pump.Pump(*queue, limit, [this](const auto& request) { return apply(request); });
        } catch (const AudioFailure& error) {
            UtilityFunctions::push_error("HP2 audio: ", error.reason, " (request ", pump.Current() ? pump.Current()->kind : -1, ")");
            return error.result;
        } catch (const std::exception& error) {
            UtilityFunctions::push_error("HP2 audio: ", error.what());
            return WORLD_AUDIO_RESOURCE_FAILURE;
        }
    }

    WorldAudioResult close()
    {
        if (!sound) return WORLD_AUDIO_OK;
        speech.Close(*sound);
        for (auto& voice : voices) {
            const auto sample = voice.sample;
            voice.retire();
            if (sample.instance && sample.instance->occupied && sample.instance->generation == sample.generation) {
                sample.instance->hasHandle = 0;
                sample.instance->playing = 0;
                sound->Voices().ReleaseSample(sample);
            }
        }
        banks.clear();
        auto& server = *AudioServer::get_singleton();
        for (auto& effect : effects) {
            if (effect.reverb.is_null()) continue;
            const auto id = server.get_bus_index(effect.bus);
            if (id > 0) server.remove_bus(id);
            effect.reverb.unref();
        }
        auto* library = std::exchange(sound, nullptr);
        return library->CancelDeviceRequestsAfterClose();
    }
};

GodotAudioDevice::GodotAudioDevice(Node& owner) : state_(std::make_unique<State>(owner)) {}
GodotAudioDevice::~GodotAudioDevice() { state_->close(); }

WorldAudioResult GodotAudioDevice::Open(bSound& sound)
{
    state_->queue = sound.HostDeviceRequests();
    if (!state_->queue || state_->sound) return WORLD_AUDIO_INVALID_BINDING;
    state_->sound = &sound;
    const auto result = state_->step(1);
    if (result != WORLD_AUDIO_OK) state_->close();
    return result;
}

WorldAudioResult GodotAudioDevice::Pump()
{
    return state_->sound ? state_->step(256) : WORLD_AUDIO_INVALID_BINDING;
}

WorldAudioResult GodotAudioDevice::Close() { return state_->close(); }
}
