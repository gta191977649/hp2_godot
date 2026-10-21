#pragma once
// TASK0089 port-side projections; Debug SHA815b60c7f473..., version unknown.
// E0089W01..03, reverse/task0089/water-research.md. No PS2 audio object ABI.
#include "memory.hpp"
#include <vector>
#include <functional>
namespace port::driving {
struct WiperSnapshot {std::int32_t state=0,on_time=0,off_time=0;std::uint32_t world_frame=0;};
struct SplashAudioRequest {unsigned sample_index=0;float volume=0,pitch_octaves=0;};
class SplashAudioMixer {
public:
 void reset(){*this=SplashAudioMixer{};}
 void trigger(bool right,int intensity){intensities_[right?1:0]=intensity;}
 std::vector<SplashAudioRequest> update(Memory&,float volume=1);
private:
 std::array<int,2> intensities_{},timers_{};
};
WiperSnapshot wiper_snapshot(const Memory&,std::uint32_t car);
void turn_wipers_on_for_duration(Memory&,std::uint32_t car,std::int32_t start,std::int32_t end);
// Original grounded water slice only. Listener distance is an explicit port binding.
void trigger_water_terrain_state(Memory&,std::uint32_t car,float distance,SplashAudioMixer&,
 const std::function<void(float,const char*)>& replay);
}
