#pragma once
// TASK-0101 E-0101-FR01. Port-owned read-only Debug race/HUD projection.
// No Godot types, original ABI claim, fabricated outcome or clock advancement.
#include "game_core/port/driving/memory.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace port::race {
struct RaceRulesViewSource {
    std::uint32_t race=0, player=0;
    std::optional<std::uint32_t> event, cop_manager, hud_features;
    std::optional<int> tree_type;
};
struct RaceHudView {
    int race_stage=0, countdown_number=0, rank=0, participant_count=0;
    float countdown_progress=0;
    int finish_reason=0, completed_laps=0, lap_limit=0, display_lap=0;
    bool point_to_point=false;
    float progress_percent=0;
    std::int32_t race_elapsed_ticks=0, sample_world_ticks=0;
    std::optional<std::int32_t> lap_elapsed_ticks, target_ticks, clock_signed_delta_ticks;
    std::int32_t clock_display_ticks=0;
    bool clock_target_applied=false, clock_overrun=false;
    std::string clock_text, race_elapsed_text;
    std::int32_t score=0;
    std::optional<std::uint32_t> hud_features;
};
struct PoliceHudView {
    bool being_pursued=false, be_a_cop=false;
    int hud_level=0;
    std::optional<int> pursuit_state, pursuit_substate;
    int tickets=0, ticket_limit=0;
    std::optional<float> be_a_cop_seconds;
    std::optional<int> arrests, arrest_quota, roadblocks_used, choppers_used;
};
struct RaceStageStanding {
    int racer_index=0, driver_number=0;
    std::optional<int> points, standing_rank, recorded_rank;
};
struct RaceRulesView {
    std::optional<int> tree_type, event_type, event_number;
    int race_type=0;
    int nfs_race_mode=0; // RP+254; Fred.SelectedLanguage is independently at+170.
    int challenge_race_mode=0,selected_language=0,loaded_language=0;
    std::string event_name, objective_text;
    std::optional<std::uint16_t> objective_type;
    std::optional<std::uint32_t> objective_data;
    std::optional<int> event_status, old_event_status, stage_index, stage_count;
    std::optional<bool> aborted, has_next_race, result_should_be_discarded;
    RaceHudView hud;
    std::optional<PoliceHudView> police;
    std::vector<RaceStageStanding> standings;
};
// Call at original HUD sampling point before AdvanceWorldTime and publish with
// the same WorldFrame. Null optional service means unavailable, never zero data.
RaceRulesView read_race_rules_view(::port::driving::Memory&,const RaceRulesViewSource&);
}
