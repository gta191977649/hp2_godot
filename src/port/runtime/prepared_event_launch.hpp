#pragma once
// TASK-0101 E-0101-PL01. Port semantic checkpoint AFTER original menu SetRace,
// BEFORE selector/effect RNG activity. No guest pointers or whole RP/Fred copy.
#include "game_core/port/race/frontend_event_resources.hpp"
#include <array>
#include <string>
namespace port::runtime {
// Names recovered in SetRace001FA378/StartEvent001F2BD8. The array is a named,
// versioned transport schema, not an address/offset supplied by the caller.
enum class PreparedParameter : unsigned {
 RaceType,TimeTrialTime,Point2Point,NumPlayerCars,NumAICars,OpponentSelect,
 DamageEnabled,TrainingLinesEnabled,HasObjective,NFSRaceMode,nMaxCops,
 bTickets,bRoadBlocks,bBarricadeSmokeables,bSpikeStrips,bHelicopter,
 BeACop,BeACopResetTimerAfterArrest,nNumTicketsBeforeArrest,
 nNumBeACopRoadBlocks,nNumBeACopChoppers,CopStrength,
 BoostScale0,BoostScale1,RandomSeedForCarColors,Count
};
struct PreparedEventLaunch {
 int tree_type=-1,event_number=-1,event_type=-1;
 std::array<std::uint32_t,unsigned(PreparedParameter::Count)> parameters;
 std::uint8_t player_start_position=0;
 std::string objective,reward_name;
 std::vector<std::uint8_t> old_state; // Source aOldState BEFORE NewRandomSeed.
};
PreparedEventLaunch capture_prepared_event(const driving::Memory&,std::uint32_t tree,std::uint32_t info);
// Port destination storage adapter for a previously initialized/prepared RP.
// Source001E9968 default field values/settings/table storage are bound without
// re-running its bRandom draw: that source value is already in the checkpoint.
void initialize_prepared_event_parameters(driving::Memory&);
// Caller first imports the post-SetRace both-tree profile and initializes RP
// storage with the adapter above. Immutable arrays/objective/name are rebound from the new original
// asset owner; only reviewed scalar/text/old-state values cross this seam.
// Does not execute SetRace, NewRandomSeed, ResetState or StartRace.
void bind_prepared_event(driving::Memory&,const race::FrontendEventResources&,const PreparedEventLaunch&);
}
