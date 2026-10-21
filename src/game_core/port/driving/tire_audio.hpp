#pragma once
// Port-side original tire sample bank; decoding shared with the InGame bank.
#include "vehicle_audio.hpp"
#include "tire_audio_state.hpp"
namespace port::driving {
struct TireSoundBank {std::array<VehicleAudioSample,4> samples;};
TireSoundBank load_tire_sound_bank(const AudioAssetReader&);
}
