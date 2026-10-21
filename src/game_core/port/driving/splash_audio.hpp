#pragma once
// Port-owned original InGame sample binding; E0089W03, water-research.md.
#include "vehicle_audio.hpp"
namespace port::driving {
struct SplashSoundBank {std::array<VehicleAudioSample,3> samples;};
SplashSoundBank load_splash_sound_bank(const AudioAssetReader&);
}
