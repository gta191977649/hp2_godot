#pragma once
// TASK-0110. Port-owned semantic boundary between the Retail profile/frontend
// projection and the separately versioned Debug source World. Never an ABI copy.
#include <cstdint>
#include <vector>
#include <optional>
#include "event_profile_codec.hpp"
#include "prepared_event_launch.hpp"
#include "event_score_codec.hpp"
#include "event_settings_codec.hpp"
#include "event_reward_codec.hpp"
namespace port::race {
// E-0101-EP01: explicit Retail profile blocks and globally shared event32
// layout. No Fred/profile pointers or whole-version overlay cross this seam.
struct EventProfileTransfer {
    ::port::runtime::EventProfileLayout layout{::port::runtime::EventProfileVersion::Retail,{},{{0,0}}};
    ::port::runtime::EventProfilePayload blocks;
};
// E-0102-SP03: explicit persistent career baseline, outside SetRace checkpoint.
struct EventScoreTransfer {
    ::port::runtime::EventScoreAxes axes;
    ::port::runtime::EventScoreProfile profile;
};
struct EventLaunchRequest {
    std::uint64_t generation=0;
    int tree_type=-1,event_index=-1,event_number=-1;
    int car_id=-1,skin_id=-1,language=0;
    int track=-1;
    bool reverse=false;
    int driving_style=0,transmission=0,preferred_gender=0;
    bool has_dash=false,vertex_damage=false;
    std::uint32_t player_points=0;
    std::uint32_t random_state=0; // Live original RNG329060 identity, not a new seed or persisted profile field.
    // Full selected-tree profile block, with original per-event state values.
    // Consumers must map each reviewed rank/time record by its owning version's
    // layout; these bytes must never be overlaid onto a Debug Fred allocation.
    std::vector<std::uint8_t> event_progress;
    std::vector<int> event_progress_offsets;
    std::optional<EventProfileTransfer> event_profile;
    // Produced once by original menu SetRace, before selector/effect draws.
    // World rebinds this semantic checkpoint instead of executing SetRace again.
    std::optional<::port::runtime::PreparedEventLaunch> prepared_event;
    std::optional<EventScoreTransfer> event_scores;
    std::optional<::port::runtime::EventSettingsTransfer> event_settings;
    std::optional<::port::runtime::EventRewardTransfer> event_rewards;
};
struct EventSettlement {
    bool final_reward_awarded=false; // Sealed actual source alias19 award-store receipt only.
    std::uint64_t generation=0;
    int tree_type=-1,event_index=-1,event_number=-1;
    bool aborted=false;
    std::uint32_t player_points=0;
    std::uint32_t random_state=0; // Source World RNG329060 after the completed outcome.
    std::vector<std::uint8_t> event_progress;
    // Reverse-codec output in the request's original Retail layout/baseline.
    // Legacy selected256-byte publication remains for existing validators.
    std::optional<::port::runtime::EventProfilePayload> event_profile;
    std::optional<::port::runtime::EventScoreProfile> event_scores;
    std::optional<::port::runtime::EventSettingsTransfer> event_settings;
    std::optional<::port::runtime::EventRewardTransfer> event_rewards;
};
}
