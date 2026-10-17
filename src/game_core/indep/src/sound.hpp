// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\sound.cpp`
// (string at 0x002B62E8, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, and the `.hpp` extension is port-side -- only
// the `.cpp` path was recovered.
// `.cpp` path was recovered.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

// TASK-0016 / E-0016-A01,A02. Port-side asset/codec infrastructure, not recovered PS2
// audio hardware code. Original samples and event names are read from the player's assets.
namespace hp2::port::audio {
struct PcmSound {
    std::uint32_t sample_rate = 0;
    std::uint32_t channels = 1;
    float nominal_gain = 1.0f; // Asset descriptor gain; group/user volume handled by host.
    std::vector<std::int16_t> samples;
};

// Mono / observed VAGx interleaved stereo decoder. Codec arithmetic from public format;
// References: FFmpeg n7.1 predictor table and user-authorized map_tools_ps2 rounded,
// clipped-history codec. External reference code does not establish PS2 game equivalence.
// JBC streaming uses 0x8000-byte channel planes per group; common menu samples use16.
// The final shorter group contains equally-sized channel planes. E-0016-A06.
PcmSound decode_vag(const std::vector<std::uint8_t>& bytes, std::size_t interleave_bytes = 16);

// Zero requested pitch projection: ELF00273850 table[129]=4140 at00327B82,
// then0026D258 converts frequency to SPU pitch at hardware48000 Hz. E-0016-A07.
std::uint32_t Sub_00273850(std::uint32_t base_rate);
std::uint16_t SOUND_HzToSPU(std::uint32_t frequency);
// Host playback factor relative to decoded metadata; does not alter PCM or sample_rate.
float neutral_playback_pitch_scale(std::uint32_t decoded_rate);

class GuestSampleBank {
public:
    static GuestSampleBank from_bytes(std::vector<std::uint8_t> bytes);
    // Caller must supply the root resolved by platform::resolve_zzdata_root.
    static GuestSampleBank load_from_zzdata(const std::string& root);
    std::vector<std::string> names() const;
    PcmSound decode(const std::string& event_name) const;
private:
    struct Entry { std::string name; std::size_t offset; std::size_t size; };
    std::vector<std::uint8_t> bytes_;
    std::vector<Entry> entries_;
    std::vector<std::pair<std::string,float>> gains_;
};
} // namespace hp2::port::audio
