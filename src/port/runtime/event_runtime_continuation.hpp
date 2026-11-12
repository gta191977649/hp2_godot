#pragma once
// TASK-0112 E-0112-CONT02. Same-Debug-version native loader handoff only.
// Never a Retail Fred/RP overlay, Godot payload, persistent save or replacement
// StartRace. All guest pointers are excluded and rebound to destination owners.
#include "event_launch.hpp"
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/race/frontend_event_resources.hpp"
#include <array>
#include <vector>
namespace port::race {
struct EventRuntimeContinuation {
 static constexpr unsigned parameter_word_count=40;
 std::uint64_t storage_generation=0;
 int tree_type=-1,event_index=-1,event_number=-1;
 int track=-1,car_id=-1,skin_id=-1;
 bool reverse=false,has_dash=false,vertex_damage=false;
 runtime::PreparedEventLaunch prepared_event;
 // Pointer-free original Debug components: explicit proven spans, not a native
 // host struct memcpy. Padding/unused tails are opaque preserved source bits.
 std::array<std::uint8_t,0x184> frontend_preferences;
 runtime::EventProfilePayload debug_event_blocks;
 std::array<std::uint8_t,812> transient_stats;
 runtime::EventScoreAxes score_axes;
 runtime::EventScoreProfile scores;
 std::array<std::uint32_t,10> task_rewards;
 std::array<std::uint32_t,19> point_rewards;
 std::array<std::uint32_t,4> frontend_attempt_words; //aborted/game/current/players.
 std::array<std::array<std::uint8_t,0x6d0>,2> frontend_players;
 std::uint32_t frontend_checksum=0;
 std::array<std::uint32_t,parameter_word_count> parameter_words;
 std::array<std::uint8_t,512> objective;
 std::array<std::array<std::uint8_t,48>,2> race_players;
 std::array<std::uint8_t,8> race_display;
 // DriverInfo bytes0..19 contain no pointers. +20 is GetCharacterInfo(index2).
 std::vector<std::array<std::uint8_t,20>> drivers;
 std::array<std::uint8_t,16> event_old_state,event_previous_state;
 std::uint16_t event_initial_players=0;
 std::uint32_t event_bonus_prediction=0;
 std::array<std::array<std::uint32_t,3>,8> event_results;
 std::array<std::uint32_t,16> clock_words;
 std::uint32_t random_state=0,audio_mode=0;
 std::array<std::uint32_t,16> category_volume_bits;
};
// Requires actual source pending UNLOADING_TRACK_SPECIFIC -> LOADING_TRACK_SPECIFIC.
// Caller first obtains any previous settlement and acknowledges its private save.
EventRuntimeContinuation capture_event_runtime_continuation(driving::Memory&,const EventLaunchRequest&,const FrontendEventResources&);
// Destination must have loaded its own EventTree/Car/Character source resources
// and bound prepared_event. This restores mutable source state without SetRace,
// NewRandomSeed, StartRace, SetupCarsForRace or another SetupNextRace call.
void restore_event_runtime_continuation(driving::Memory&,const EventLaunchRequest&,const EventRuntimeContinuation&);
// Call after source sound lookup/voices initialization, before RC construction.
void restore_event_continuation_audio(driving::Memory&,const EventRuntimeContinuation&);
}
