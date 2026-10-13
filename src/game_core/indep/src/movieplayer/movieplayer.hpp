// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\movieplayer\movieplayer.cpp`
// (string at 0x002BDD08, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, and the `.hpp` extension is port-side -- only
// the `.cpp` path was recovered.
// `.cpp` path was recovered.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "game_core/indep/src/fmenus.hpp"

// Port-side AVI container/backend projection. Original movie reader is 00237760,
// frame decode 00239148/00234B38; E-0016-V02. FFmpeg is a host service, not recovered code.
namespace port::media {
struct AviMovie {
    std::uint32_t width{}, height{}, scale{}, rate{}, sample_rate{};
    std::uint16_t channels{}, bits{};
    std::vector<std::vector<std::uint8_t>> video_frames;
    std::vector<std::uint8_t> audio_data;
};
AviMovie read_avi(const std::vector<std::uint8_t>& bytes);
// Host projection of 00237AD8 -> 00239148 -> 002363A8 bottom-up output.
// E-0016-V06: reverse row order only; preserve pixel/channel order within rows.
void orient_rgba_for_ps2(std::vector<std::uint8_t>& rgba, std::uint32_t width, std::uint32_t height);
class MovieDecoder {
public:
    MovieDecoder(std::uint32_t width, std::uint32_t height);
    ~MovieDecoder();
    MovieDecoder(const MovieDecoder&) = delete;
    MovieDecoder& operator=(const MovieDecoder&) = delete;
    // Returns tightly packed RGBA, maintaining inter-frame prediction state.
    std::vector<std::uint8_t> decode_frame(const std::vector<std::uint8_t>& packet);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
