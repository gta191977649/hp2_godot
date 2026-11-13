#ifndef NFSHP2_SPEECH_NFSFSM_HPP
#define NFSHP2_SPEECH_NFSFSM_HPP

#include "indep/src/speech/fsm.hpp"

// PAL20DA78 /20DA88. State zero is the actual embedded ready state.
struct WorldSpeechFsm {
    WorldSpeechState* current = nullptr;
    WorldSpeechState* initial = nullptr;
    std::array<WorldSpeechState,25> states;
    WorldSpeechFsm() = default;
    void SetReadyState() noexcept { current=&states[0]; }
    bool isReadyState() const noexcept { return current==&states[0]; }
    WorldSpeechFsm(const WorldSpeechFsm&) = delete;
    WorldSpeechFsm& operator=(const WorldSpeechFsm&) = delete;
    WorldSpeechFsm(WorldSpeechFsm&&) = delete;
    WorldSpeechFsm& operator=(WorldSpeechFsm&&) = delete;
};

#endif
