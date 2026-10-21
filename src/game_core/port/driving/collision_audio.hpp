#pragma once
// Port-side binding of original CollisionSounds slices; E-0083-A01.
// Debug815b60c7f473...332f1b. No original object/SPU ABI claim.
#include "vehicle_audio.hpp"
#include <optional>
namespace port::driving {
std::vector<VehicleAudioSample> load_ingame_sound_samples(const AudioAssetReader&,const std::vector<std::string>& names);
struct CollisionSoundBank {std::array<VehicleAudioSample,18> samples;};
CollisionSoundBank load_collision_sound_bank(const AudioAssetReader&);
}
