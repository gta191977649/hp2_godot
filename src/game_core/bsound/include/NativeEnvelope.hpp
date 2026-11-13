#pragma once

#include <algorithm>
#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

struct WorldAudioRequest;

namespace hp2 {

// PAL GetAttack/GetRelease -> IRX253 STREAM SetVoiceData. Only these two
// linear rate codes reach audible key-on: decay has a full-level threshold
// and sustain is infinite. This is a native gain curve, not an IOP/SPU voice,
// register image, scheduler, sample-memory facade or console DSP runtime.
struct NativeEnvelopeParameters {
    std::uint8_t attack{26};
    std::uint8_t release{10};
};
NativeEnvelopeParameters CaptureVoiceEnvelope(const WorldAudioRequest& operation);

// One instance belongs to the actual PCM consumer. Its time is the native
// 48 kHz output clock AFTER resampling; pitch, render/game ticks, decoder
// chunks and owner-thread polling cannot alter the envelope's duration.
class NativeEnvelope final {
public:
    enum class Phase : std::uint8_t { Silent, Attack, Decay, Sustain, Release };
    static constexpr std::uint32_t OutputRate=48000;
    explicit NativeEnvelope(NativeEnvelopeParameters parameters={}) noexcept : parameters_(parameters) {}

    void KeyOn(std::uint64_t frame) noexcept {
        frame_=frame;level_=0;remainder_=0;phase_=Phase::Attack;
    }
    void KeyOff(bool streamStop=false) noexcept {
        if (phase_==Phase::Silent)return;
        remainder_=0; // Each actual key-off resets its rate counter, not its level.
        release_=streamStop ? std::uint8_t{0}:parameters_.release;
        phase_=Phase::Release;
        if (!level_)phase_=Phase::Silent;
    }
    void End() noexcept { level_=0;remainder_=0;phase_=Phase::Silent; }
    Phase State() const noexcept { return phase_; }
    bool Silent() const noexcept { return phase_==Phase::Silent; }
    std::uint16_t Level() const noexcept { return static_cast<std::uint16_t>(level_); }
    float Gain() const noexcept { return static_cast<float>(level_)*(1.0f/32768.0f); }
    std::uint64_t Frame() const noexcept { return frame_; }
    NativeEnvelopeParameters Parameters() const noexcept { return parameters_; }

    void AdvanceTo(std::uint64_t frame) noexcept {
        if (frame>frame_)AdvanceFrames(frame-frame_);
    }
    void AdvanceFrames(std::uint64_t frames) noexcept {
        frame_+=std::min(frames,std::numeric_limits<std::uint64_t>::max()-frame_);
        // At most attack -> one output-frame decay -> sustain, or release
        // -> silence. Long pauses are evaluated arithmetically, never by a
        // loop over elapsed audio frames on the real-time processing thread.
        while (frames) {
            if (phase_==Phase::Silent || phase_==Phase::Sustain)return;
            if (phase_==Phase::Decay) { --frames;phase_=Phase::Sustain;continue; }
            const bool attack=phase_==Phase::Attack;
            const unsigned rate=attack ? (parameters_.attack&127u):(release_&31u);
            if (rate==(attack ? 127u:31u))return; // Authored infinity code.
            const unsigned shift=attack ? rate/4u:rate;
            const unsigned spacing=shift>11u ? std::min(15u,shift-11u):0u;
            const std::uint32_t interval=std::uint32_t(1)<<spacing;
            const unsigned base=attack ? 7u-(rate&3u):8u;
            const std::uint32_t delta=base<<(shift<11u ? 11u-shift:0u);
            const std::uint32_t distance=attack ? 32767u-level_:level_;
            const std::uint64_t steps=(distance+delta-1u)/delta;
            const std::uint64_t needed=steps*interval-remainder_;
            if (frames>=needed) {
                frames-=needed;remainder_=0;
                level_=attack ? 32767u:0u;
                phase_=attack ? Phase::Decay:Phase::Silent;
            } else {
                const std::uint64_t elapsed=remainder_+frames;
                const auto change=static_cast<std::uint32_t>(elapsed/interval)*delta;
                level_=attack ? level_+change:level_-change;
                remainder_=static_cast<std::uint32_t>(elapsed%interval);
                return;
            }
        }
    }
private:
    NativeEnvelopeParameters parameters_;
    std::uint64_t frame_{};
    std::uint32_t level_{},remainder_{};
    std::uint8_t release_{10};
    Phase phase_{Phase::Silent};
};

struct NativeOutputClock {
    std::atomic<std::uint64_t> frames{0};
};

// Portable body of the production XAPO. The owner publishes only command
// epochs; the audio thread alone touches the curve. No allocations, locks,
// file I/O, game pointers or callbacks are used by Process. A shared master
// output clock includes time during a stopped/paused source voice.
class NativeEnvelopeOutput final {
public:
    NativeEnvelopeOutput(NativeOutputClock& clock,NativeEnvelopeParameters parameters) noexcept :
        clock_(clock),envelope_(parameters) {}
    void KeyOn() noexcept { Publish(1); }
    void KeyOff(bool streamStop) noexcept { Publish(streamStop ? 3u:2u); }
    bool Finished() const noexcept {
        return completed_.load(std::memory_order_acquire)==command_.load(std::memory_order_acquire);
    }
    bool Process(float* pcm,std::size_t frames,std::size_t channels,bool valid) noexcept {
        const auto start=clock_.frames.load(std::memory_order_acquire);
        const auto command=command_.load(std::memory_order_acquire);
        if (command!=applied_) {
            if ((command&3u)==1u) { envelope_.KeyOn(start);holdInput_=false;lastInput_={}; }
            else {
                envelope_.AdvanceTo(start);holdInput_=(command&3u)==3u;
                envelope_.KeyOff(holdInput_);
            }
            applied_=command;
        } else envelope_.AdvanceTo(start);
        // XAPO SILENT input need not be initialized. Ordinary starvation
        // replaces the previous source value with zero; a stream key-off
        // already in progress holds its last value for its two-frame tail.
        if (!valid && !holdInput_ && frames)lastInput_={};
        const bool outputValid=valid || (holdInput_ && !envelope_.Silent());
        if (outputValid && !holdInput_ &&
            (envelope_.State()==NativeEnvelope::Phase::Sustain || envelope_.Silent())) {
            // These two phases have a constant gain and no remaining rate
            // transitions. Preserve the last unscaled input for a later
            // stream key-off, then advance the same output clock once.
            const float gain=envelope_.Gain();
            if (frames)for (std::size_t channel=0;channel<channels;++channel)
                lastInput_[channel]=pcm[(frames-1u)*channels+channel];
            for (std::size_t sample=0;sample<frames*channels;++sample)
                pcm[sample]*=gain;
            envelope_.AdvanceFrames(frames);
        } else if (outputValid) {
            for (std::size_t frame=0;frame<frames;++frame) {
                envelope_.AdvanceFrames(1);
                const float gain=envelope_.Gain();
                for (std::size_t channel=0;channel<channels;++channel) {
                    if (!holdInput_)lastInput_[channel]=pcm[frame*channels+channel];
                    pcm[frame*channels+channel]=lastInput_[channel]*gain;
                }
            }
        } else envelope_.AdvanceFrames(frames);
        completed_.store(envelope_.Silent() ? applied_:0,std::memory_order_release);
        return outputValid;
    }
private:
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
        "Native x64 audio command/output clocks must be lock-free");
    NativeOutputClock& clock_;
    NativeEnvelope envelope_;
    std::atomic<std::uint64_t> command_{0},completed_{0};
    std::uint64_t ownerEpoch_{},applied_{};
    std::array<float,2> lastInput_{};
    bool holdInput_{};
    void Publish(std::uint64_t kind) noexcept {
        ownerEpoch_+=4u;
        command_.store(ownerEpoch_|kind,std::memory_order_release);
    }
};

} // namespace hp2
