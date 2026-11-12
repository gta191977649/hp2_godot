#pragma once
// TASK-0110 E-0110-F04. Port decoded publication of original completed source
// state, never a replacement finish trigger, rank calculation or reward rule.
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/track/route_data.hpp"
#include <vector>
#include "reward_observation.hpp"
namespace port::race {
struct SettlementRow {
 int position=0,driver_number=0,finish_reason=0;
 int packed_time=0,packed_best_lap=0,score=0,total_points=0,bonus=0,tickets=0;
 std::string time_text;
};
struct SettlementSnapshot {
 bool aborted=false;
 int continue_race_type=0,track=0,race_type=0,event_type=0,event_status=0;
 std::vector<SettlementRow> rows;
};
// Exact source data phase0014BAB0..0014BB1C; caller must deliver once. This
// seam exists for native validation; it does not itself transition RC state.
int finalize_race_data(::port::driving::Memory&,const ::port::track::RouteData&,RewardAwardReceipt* receipt=nullptr);
// Requires the actual RC POSTRACE state7; copies source transient rows after
// Fred has awarded points and posted persistent event results.
SettlementSnapshot read_settlement(::port::driving::Memory&,std::uint32_t coordinator);
}
