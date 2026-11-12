#pragma once
// Port process lifetime/boundaries for Debug speech::mgr_t and SpeechNoise.
// TASK-0099 E-0099-W16; NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/driving/memory.hpp"
#include <functional>
#include <utility>
#include <vector>
namespace port::race {
struct SpeechDeviceServices {
    // Whole actual bSample/SED operations. Fresh original startup reaches none
    // of these: SpeechNoise ctor initializes Helicopter to null, g_pCopSED is
    // owned by the separate speech-bank loader. Nonempty paths never no-op.
    std::function<bool(port::driving::Memory&,std::uint32_t)> sample_is_playing;
    std::function<void(port::driving::Memory&,std::uint32_t)> sample_stop;
    std::function<void(port::driving::Memory&,std::uint32_t)> sed_clear_event_queue;
};
// Debug00207630 constructor and its real fragmented transition storage.
// Source ownership: ..\indep\src\speech\nfsfsm.cpp; E-0099-W16.
void construct_speech_fsm(port::driving::Memory&,std::uint32_t,std::vector<std::uint32_t>& allocations);
// Debug0020A748 in original speech/speechcontext.cpp; port Memory projection.
void ShuffleCopIds_0020A748(port::driving::Memory&);
class CopSpeechBindings {
    port::driving::Memory& memory_;
    SpeechDeviceServices device_;
    std::vector<std::uint32_t> allocations_;
    bool owns_manager_=false,initialized_=false;
    void construct_manager(std::uint32_t); // Debug00206728, original speech/mgr.cpp.
    void construct_noise(std::uint32_t); // Debug002101D0, original speechnoise.cpp.
    void StopRadioNoise(std::uint32_t); // Debug002105E8, speechnoise.cpp.
    void StopHelicopterNoise(std::uint32_t); // Debug00210590, speechnoise.cpp.
    std::uint32_t noise_instance(); // Actual001AD768 singleton, not other inline copies.
public:
    explicit CopSpeechBindings(port::driving::Memory& m,SpeechDeviceServices device={})
        :memory_(m),device_(std::move(device)){}
    CopSpeechBindings(const CopSpeechBindings&)=delete;
    CopSpeechBindings& operator=(const CopSpeechBindings&)=delete;
    // Original00223490 ->00204DC0 ->00206728, then publishes CopSpeechMgr.
    // Host explicit shutdown replaces process atexit ownership, not constructor work.
    void init();
    // Debug00205158 thunk ->0020A748; exactly8 default-global RNG draws.
    void allocate_voices(std::uint32_t manager);
    // Debug00205100 with actual queue/noise state and required active-device paths.
    void flush_queue(std::uint32_t manager);
    // Debug00210398. Proven unload clears Helicopter after stopping its noise;
    // it does not fabricate sample destruction or clear the other sample arrays.
    void unload_noise_samples();
    // Port owner releases its constructor allocations and resets its static guard.
    // Original mgr destructor stops the FSM; SpeechNoise static destructor is
    // proven empty. This does not silently stop an active audio device.
    void shutdown();
    std::uint32_t manager()const{return memory_.word(0x328c18);}
};
}
