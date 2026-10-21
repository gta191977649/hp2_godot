#pragma once
// Port-side collision audio state projection; E-0083-A01.
#include "memory.hpp"
#include <array>
#include <optional>
namespace port::driving {
struct CollisionSoundRequest {std::uint32_t sample_index;float volume,pitch_octaves;bool playing=true;};
class CollisionSoundSelector {
public:
    // GetCollisionSoundType001E0C18 increments before selecting; terrain already
    // comes from original body-priority selection, not renderer material guesses.
    std::uint32_t select(const Memory&,int terrain,float total_impulse);
    void reset(){indices_={};}
private:
    std::array<unsigned,5> indices_{};
};
// Original CalcCollisionVolume001E11E8 / CreateSample001E1838 bounded slices.
float collision_sound_volume(const Memory&,float total_impulse,float camera_distance);
bool collision_sound_ready(const Memory&,std::uint32_t age_ticks);
// Uses original sound::Random001ACFA8 and SoundRandomSeed327EEC only for
// accepted SoundEnabled playback. A disabled sound still allocates a request.
std::optional<CollisionSoundRequest> collision_sound_request(CollisionSoundSelector&,
    Memory&,int terrain,float total_impulse,float camera_distance,
    std::uint32_t age_ticks);
// Port-side persistent scrape request; original PlayScrape001E1D80,
// StartScraping001E1288 and UpdateSample001E13C0. Caller invokes start only
// for a fresh AddImpulse00219400 and owns the non-null Scrape lifetime gate.
struct CollisionScrapeRequest {std::uint32_t sample_index;float volume,pitch_octaves;};
std::optional<CollisionScrapeRequest> collision_scrape_start(Memory&,int terrain,
    float total_impulse,float scrape_speed,float initial_distance,std::uint32_t age_ticks);
float collision_scrape_update_volume(const Memory&,float total_impulse,
    float scrape_speed,float initial_distance,float current_distance);
// Original bSample::Fade0026C910 / Update0026CB40 raw sample-volume units.
// One call is one sound-service update; host cadence equivalence is unverified.
float collision_scrape_fade_adjust(const Memory&,float current_volume);
float collision_scrape_fade_step(float current_volume,float adjust);
bool collision_scrape_fade_finished(const Memory&,float current_volume);
}
