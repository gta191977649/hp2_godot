#pragma once
// Port-side state projection of Debug PlayerTireSound skid/squall/burnout slices.
// E-0083-T01; reverse/task0083/tires.md. No original object/SPU ABI claim.
#include "memory.hpp"
#include <array>
namespace port::driving {
struct TireAudioVoice {bool playing=false;unsigned sample_index=0;float volume=0,pitch_octaves=0;};
struct TireAudioFrame {std::array<TireAudioVoice,10> voices{};};
class TireAudioMixer {
public:
    void reset(){*this=TireAudioMixer{};}
    TireAudioFrame update(const Memory&,std::uint32_t car,float distance_volume=1);
private:
    std::array<float,2> locked_rear_pitch_{{-1,-1}},squall_volume_{};
    std::array<bool,2> previous_road_{{true,true}};
    std::array<TireAudioVoice,10> voices_{};
};
}
