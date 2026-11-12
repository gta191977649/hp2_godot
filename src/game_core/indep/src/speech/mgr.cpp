// TASK-0099 E-0099-W16; Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Exact original source ownership: local/task0099/cop-speech/source-ownership.json.
// Host Memory projection, no native ABI or whole-version equivalence claim.
#include "game_core/port/race/cop_speech.hpp"
#include "game_core/port/not_implemented.hpp"
#include <stdexcept>
namespace port::race {
// Debug00206728 speech::mgr_t constructor, E-0099-W16; exact .mdebug owner.
void CopSpeechBindings::construct_manager(std::uint32_t manager){
 memory_.word(manager+0x5b0,0x2b2ff0);
 construct_speech_fsm(memory_,manager,allocations_);
 memory_.word(manager+0x4c0,0);memory_.word(manager+0x4b8,0);memory_.word(manager+0x4bc,0);
 // speechContext_t0020A6B8 and test002067C0 have proven empty bodies.
 memory_.word(manager+0x5ac,0);memory_.word(manager,memory_.word(manager+4));
 memory_.word(manager+0x4c4,0);
}
// Debug00205158 AllocateVoices is a jump thunk to0020A748; E-0099-W16.
void CopSpeechBindings::allocate_voices(std::uint32_t manager){
 if(!initialized_||manager!=memory_.word(0x328c18))throw std::logic_error("Original police speech owner is not initialized");
 ShuffleCopIds_0020A748(memory_);
}
// Debug00205100/002105E8/00210590/002011A0; E-0099-W16. Null/unused
// original audio state is a recovered branch. Active device calls stay required.
void CopSpeechBindings::flush_queue(std::uint32_t manager){
 if(!initialized_||manager!=memory_.word(0x328c18))throw std::logic_error("Original police speech owner is not initialized");
 memory_.word(manager,manager+0x10);memory_.word(manager+0x4c0,0);memory_.word(manager+0x4bc,memory_.word(manager+0x4b8));
 StopRadioNoise(noise_instance());
 StopHelicopterNoise(noise_instance());
 if(const auto sed=memory_.word(0x327f20))if(memory_.word(sed)==1){
  if(!device_.sed_clear_event_queue)hp2::not_implemented("0x002011A0","initialized original SED queue/StopSentence services are not bound");
  device_.sed_clear_event_queue(memory_,sed);
 }
}
}
