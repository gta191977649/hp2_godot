#include "collision_events.hpp"
#include <cmath>
namespace port::driving {
// Debug Collision::AddImpulse00219400; E-0083-C01.
void accumulate_collision_audio(CollisionAudioContact& state,Memory& m,
 const std::array<float,3>& point,const std::array<float,3>& impulse,float scrape,const std::array<float,3>& listener){
 state.total_impulse+=std::sqrt((impulse[0]*impulse[0]+impulse[1]*impulse[1])+impulse[2]*impulse[2]);
 state.last=m.word(0x327cf8);state.current=point;state.scrape_speed=scrape;
 if(!state.scrape){float squared=0;for(unsigned i=0;i<3;++i){const auto d=state.initial[i]-listener[i];squared+=d*d;}
 state.scrape=collision_scrape_start(m,state.terrain,state.total_impulse,scrape,std::sqrt(squared),m.word(0x327cf8)-state.start);}

}
// Debug CollisionSounds::UpdateSample001E13C0; E-0083-C01/A01.
std::optional<CollisionScrapeVoice> update_collision_scrape(CollisionAudioContact& state,const Memory& m,const std::array<float,3>& listener){
 if(!state.scrape)return {};
 const auto distance=[&](const auto& point){float squared=0;for(unsigned i=0;i<3;++i){const auto d=point[i]-listener[i];squared+=d*d;}return std::sqrt(squared);};
 state.scrape->volume=collision_scrape_update_volume(m,state.total_impulse,state.scrape_speed,distance(state.initial),distance(state.current));
 return CollisionScrapeVoice{state.id,state.scrape->sample_index,state.scrape->volume,state.scrape->pitch_octaves};
}
// Debug Collision::Update00219650: signed packed-time delta and strict lifetime.
bool collision_audio_alive(const CollisionAudioContact& state,const Memory& m){
 return float(static_cast<std::int32_t>(m.word(0x327cf8)-state.last))*m.scalar(0x325f6c)<m.scalar(0x328b1c);
}
// Debug CollisionSounds::Update001E1468 supported impact branch only.
std::optional<CollisionSoundRequest> update_collision_audio(CollisionAudioContact& state,
 CollisionSoundSelector& selector,Memory& m,const std::array<float,3>& listener){
 if(state.sample_created)return {};
 float squared=0;for(unsigned i=0;i<3;++i){const float d=state.initial[i]-listener[i];squared+=d*d;}
 auto request=collision_sound_request(selector,m,state.terrain,state.total_impulse,std::sqrt(squared),m.word(0x327cf8)-state.start);
 if(request)state.sample_created=true;
 return request;
}
}
