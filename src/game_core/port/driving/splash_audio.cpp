#include "splash_audio.hpp"
#include "collision_audio.hpp"
#include <algorithm>
namespace port::driving {
// Debug BaseCarSound::GetSplashSample0018B958; E0089W03.
// The shared loader preserves original InGame patch volume and ADPCM flags.
SplashSoundBank load_splash_sound_bank(const AudioAssetReader& read){
 const auto samples=load_ingame_sound_samples(read,{"Splash1","Splash2","Splash3"});
 SplashSoundBank out;std::copy(samples.begin(),samples.end(),out.samples.begin());return out;
}
}
