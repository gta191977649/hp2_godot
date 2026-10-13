// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#pragma once
#include "game_core/indep/src/sound.hpp"

namespace hp2::port::audio {
// Port-side raw movie ADPCM adapter. E-0016-A07: movieplayer002380A8 supplies
// basefrequency0xAA66; sound00273850+0026D258 produce SPU pitch3762 at48000 Hz.
// The AVI WAVEFORMATEX carrier (8-bit mono) does not describe this hardware stream.
struct MovieAudio {
    PcmSound pcm; // Signed16 stereo, host mix_rate48000.
    float pitch_scale=3762.0f/4096.0f; // Exact original SPU pitch fraction.
};
MovieAudio decode_movie_audio(const std::vector<std::uint8_t>& original_audio_bytes);
} // namespace hp2::port::audio
