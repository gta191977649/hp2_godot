#pragma once
// TASK-0110 E-0110-F03. Exact Debug class/function names, raw logical storage.
// Original source/header path is unknown; this root placement/header extension
// is port-side (TASK-0029), not evidence of an original translation unit.
#include "game_core/port/driving/memory.hpp"
namespace hp2::debug {
struct NFSTaskParameters {
 static bool CheckForCompletion(::port::driving::Memory&,std::uint32_t task,std::uint32_t racing_car); //002441E0.
};
struct NFSPointsRewards {
 static bool CheckForCompletion(const ::port::driving::Memory&,std::uint32_t reward,int points); //002445E8.
};
int GetTaskRewardList(::port::driving::Memory&,std::uint32_t output,std::uint32_t count,std::uint32_t racing_car); //002443C8.
int GetPointsRewardList(::port::driving::Memory&,std::uint32_t output,std::uint32_t count,int points); //002445F8.
void CheckForTaskAndPointRewards(::port::driving::Memory&,std::uint32_t race); //00244D00.
}
