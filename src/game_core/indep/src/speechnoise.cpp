// TASK-0099 E-0099-W16; Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Exact original source ownership: local/task0099/cop-speech/source-ownership.json.
// Host Memory projection, no native ABI or whole-version equivalence claim.
#include "game_core/port/race/cop_speech.hpp"
#include "game_core/port/not_implemented.hpp"
namespace port::race {
// Debug002101D0 SpeechNoise constructor; E-0099-W16.
void CopSpeechBindings::construct_noise(std::uint32_t noise){
 memory_.word(noise+0x28,0);memory_.word(noise+0x2c,0);
 memory_.word(noise,0);memory_.word(noise+0x30,0);
}
// Debug002105E8 SpeechNoise::StopRadioNoise; E-0099-W16.
void CopSpeechBindings::StopRadioNoise(std::uint32_t noise){memory_.word(noise+0x28,0);}
// Debug00210590 SpeechNoise::StopHelicopterNoise; E-0099-W16.
void CopSpeechBindings::StopHelicopterNoise(std::uint32_t noise){
 const auto sample=memory_.word(noise);
 if(sample){
  if(!device_.sample_is_playing)hp2::not_implemented("0x0026C590","active police helicopter bSample device query is not bound");
  if(device_.sample_is_playing(memory_,sample)){
   if(!device_.sample_stop)hp2::not_implemented("0x0026C730","active police helicopter bSample stop is not bound");
   device_.sample_stop(memory_,sample);
  }
 }
}
// Debug00210398 SpeechNoise::UnLoadSamples; E-0099-W16. Does not delete samples.
void CopSpeechBindings::unload_noise_samples(){
 const auto noise=noise_instance();
 StopHelicopterNoise(noise);StopRadioNoise(noise);memory_.word(noise,0);
}
}
