#pragma once
// TASK-0099 E-0099-W40. Explicit PC hardware mapping; no recovered SPU DSP ABI.
// Policy: docs/DECISIONS.md, 2026-10-29 W40 PC sound device boundary.
#include "game_core/port/driving/memory.hpp"
#include <array>
#include <memory>
#include <mutex>
#include <string>
namespace port::race {
class CarSoundResources;
struct DecodedSoundPatch {
 std::string name;
 std::vector<std::int16_t> samples;
 std::uint32_t sample_rate=0;
 bool repeat=false;
 std::uint32_t loop_begin=0,loop_end=0;
};
struct PcmChannelState {
 bool active=false,effect_enabled=false;
 std::uint16_t patch_id=0xffff,left=0,right=0,pitch=0,attack_release=0,loop=0,offset=0,smooth_duration=0;
 double cursor=0;
 std::shared_ptr<const DecodedSoundPatch> patch;
};
class PcmCommandDevice {
 mutable std::mutex mutex_;
 std::array<PcmChannelState,48> channels_{};
 std::uint64_t consumed_commands_=0;
public:
 PcmCommandDevice()=default;
 PcmCommandDevice(const PcmCommandDevice&)=delete;
 PcmCommandDevice& operator=(const PcmCommandDevice&)=delete;
 // Consume the real complete senddata batch, committing only after validation.
 // Returns its halfword extent. Does not itself reset recovered EE counters.
 unsigned consume(driving::Memory&,CarSoundResources&);
 // Concrete interleaved stereo PCM output at the PC device's explicit clock.
 // Linear resampling/gain/loop/gating are port hardware choices. Raw ADSR and
 // effects remain inspectable; no SPU filter/reverb/envelope equivalence claimed.
 std::vector<float> render(std::size_t frames,unsigned output_rate);
 std::array<PcmChannelState,48> snapshot()const;
 std::uint64_t consumed_commands()const;
 // Apply only the observed resident48-key feedback rule from00263C10.
 // No invented full IOP/stream/CD status reply is published.
 void publish_resident_status(driving::Memory&)const;
};
}
