#include "tire_audio.hpp"
#include "collision_audio.hpp"
#include <algorithm>
namespace port::driving {
// Debug PlayerTireSound constructor0020D660, E-0083-T01.
TireSoundBank load_tire_sound_bank(const AudioAssetReader& read){
    const auto samples=load_ingame_sound_samples(read,{"LongitudinalSkid","GravelSkid","SquallSkid","BurnOutPeel"});
    TireSoundBank bank;std::copy(samples.begin(),samples.end(),bank.samples.begin());return bank;
}
}
