#pragma once
// TASK-0112 E-0112-R09, E-0102-RJ01. Port-only receipt of actual source flag
// stores. No game callbacks, predicate re-evaluation, RNG or Memory mutation.
#include <array>
#include <cstdint>
#include <stdexcept>
namespace port::race {
struct RewardAwardReceipt {
 std::array<bool,10> task_stores{};
 std::array<bool,20> point_stores{}; //19 records actual alias write, NOT a flag baseline.
 std::uint32_t store_count=0;
 bool source_completed=false;
 bool valid=true;
};
namespace reward_observation_detail {inline thread_local RewardAwardReceipt* active=nullptr;}
class RewardAwardScope {
 RewardAwardReceipt receipt_;
 bool finished_=false;
public:
 RewardAwardScope(){if(reward_observation_detail::active)throw std::logic_error("Nested source reward award observation");reward_observation_detail::active=&receipt_;}
 ~RewardAwardScope(){if(reward_observation_detail::active==&receipt_)reward_observation_detail::active=nullptr;}
 RewardAwardScope(const RewardAwardScope&)=delete;
 RewardAwardScope& operator=(const RewardAwardScope&)=delete;
 // Call only after the actual TheRaceIsOver source invocation returns normally.
 RewardAwardReceipt finish(){if(finished_||reward_observation_detail::active!=&receipt_)throw std::logic_error("Source reward observation already finished or lost owner");reward_observation_detail::active=nullptr;finished_=true;if(!receipt_.valid)throw std::logic_error("Source reward award stores differ from reviewed bounded table domains");receipt_.source_completed=true;return receipt_;}
};
// Inserted immediately AFTER the matching original flag store. Observation has
// fixed storage and cannot throw/allocate/call back into game behavior.
inline void observe_task_reward_store(std::uint32_t index) noexcept {auto* receipt=reward_observation_detail::active;if(!receipt)return;if(index>=10||receipt->store_count>=29){receipt->valid=false;return;}receipt->task_stores[index]=true;++receipt->store_count;}
inline void observe_point_reward_store(std::uint32_t index) noexcept {auto* receipt=reward_observation_detail::active;if(!receipt)return;if(index>=20||index==5||receipt->store_count>=29){receipt->valid=false;return;}receipt->point_stores[index]=true;++receipt->store_count;}
}
