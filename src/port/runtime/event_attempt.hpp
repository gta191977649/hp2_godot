#pragma once
// TASK-0112 E-0112-CONT01. Port transaction validation, not an original state.
#include "event_launch.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
namespace port::runtime {
// A storage-only seal. It carries no prepared launch or original runtime state,
// so a caller cannot mistake continuation acknowledgement for another StartRace.
struct EventAttemptBaseline {
 std::uint64_t previous_generation=0,generation=0;
 int tree_type=-1,event_index=-1,event_number=-1;
 std::uint32_t player_points=0,random_state=0;
 std::vector<std::uint8_t> event_progress;
 std::optional<race::EventProfileTransfer> event_profile;
 std::optional<race::EventScoreTransfer> event_scores;
 std::optional<EventSettingsTransfer> event_settings;
 std::optional<EventRewardTransfer> event_rewards;
};
inline EventAttemptBaseline event_attempt_baseline(const race::EventLaunchRequest& r,std::uint64_t previous){return {previous,r.generation,r.tree_type,r.event_index,r.event_number,r.player_points,r.random_state,r.event_progress,r.event_profile,r.event_scores,r.event_settings,r.event_rewards};}
inline void apply_event_attempt_baseline(race::EventLaunchRequest& request,const EventAttemptBaseline& baseline){
 if(request.generation!=baseline.previous_generation||baseline.generation<=baseline.previous_generation||request.tree_type!=baseline.tree_type||request.event_index!=baseline.event_index||request.event_number!=baseline.event_number)throw std::logic_error("Stale or mismatched native event continuation acknowledgement");
 if(baseline.event_progress.size()!=256||bool(request.event_profile)!=bool(baseline.event_profile)||bool(request.event_scores)!=bool(baseline.event_scores)||bool(request.event_settings)!=bool(baseline.event_settings)||bool(request.event_rewards)!=bool(baseline.event_rewards))throw std::logic_error("Native continuation acknowledgement dropped sealed baseline fields");
 auto next=request;next.generation=baseline.generation;next.player_points=baseline.player_points;next.random_state=baseline.random_state;next.event_progress=baseline.event_progress;next.event_profile=baseline.event_profile;next.event_scores=baseline.event_scores;next.event_settings=baseline.event_settings;next.event_rewards=baseline.event_rewards;request=std::move(next);
}
inline std::uint64_t next_event_attempt_generation(std::uint64_t expected,std::uint64_t active,std::uint64_t last_settled,std::uint64_t counter,bool require_saved_outcome){
 if(!expected||expected!=active)throw std::logic_error("Stale authored continuation generation");
 if(require_saved_outcome&&last_settled!=active)throw std::logic_error("Previous source outcome has not been saved; continuation is not authorized");
 if(counter<active||counter==std::numeric_limits<std::uint64_t>::max())throw std::logic_error("Authored continuation generation counter is invalid");
 return counter+1;
}
}
