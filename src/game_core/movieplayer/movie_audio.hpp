#pragma once
#include "game_core/audio/sound.hpp"

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
