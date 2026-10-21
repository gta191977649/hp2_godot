#pragma once
// Port-owned projection of Collision lifecycle; E-0083-C01.
#include "collision_sound_state.hpp"
#include <vector>
namespace port::driving {
struct CollisionScrapeVoice {std::uint64_t id=0;std::uint32_t sample_index=0;float volume=0,pitch_octaves=0;};
struct CollisionScrapeFade {CollisionScrapeVoice voice;float adjust=0;};
struct CollisionAudioContact {
    std::uint32_t start=0,last=0;
    std::uint64_t id=0;
    std::optional<CollisionScrapeRequest> scrape;
    int terrain=0;
    float total_impulse=0,scrape_speed=0;
    std::array<float,3> initial{},current{};
    bool sample_created=false;
};
// Original00219230/00219400/00219650 state subset. No guest allocation ABI.
void accumulate_collision_audio(CollisionAudioContact&,Memory&,const std::array<float,3>& point,
    const std::array<float,3>& impulse,float scrape_speed,const std::array<float,3>& listener);
std::optional<CollisionScrapeVoice> update_collision_scrape(CollisionAudioContact&,const Memory&,const std::array<float,3>& listener);
bool collision_audio_alive(const CollisionAudioContact&,const Memory&);
std::optional<CollisionSoundRequest> update_collision_audio(CollisionAudioContact&,
    CollisionSoundSelector&,Memory&,const std::array<float,3>& listener);
}
