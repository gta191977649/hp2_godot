#pragma once
// TASK-0112 E-0112-R09. Port-only coupled private save, not PS2 card hardware.
#include "ps2_profile_store.hpp"
#include "port/runtime/event_reward_codec.hpp"
namespace hp2::platform {
struct RewardStoreTestFaults {
 bool fail_after_journal=false,fail_after_main=false,fail_after_supplement=false;
 AtomicFileTestFaults journal_write,main_write,supplement_write;
};
// Applies recovery first. Private validated supplement returned only for the
// exact private main; arbitrary configured inputs never acquire a sidecar.
// Coupled corruption/recovery failure throws; it must not become fresh-career
// fallback. Only ordinary absent input retains ProfileLoad.loaded=false.
ProfileLoad load_private_reward_profile(const std::string& search_start,int region,
 memorycard::Profile_002FC050&,std::optional<port::runtime::EventRewardTransfer>&);
// Candidate and caller remain untouched until both endpoints are verified and
// intent cleared. Error may leave durable recovery intent; never infer success.
std::string store_private_reward_profile(const std::string& search_start,int region,
 memorycard::Profile_002FC050&,const port::runtime::EventRewardTransfer&,
 const RewardStoreTestFaults& test_faults={});
// Canonical SHA256 for metadata binding; input is exact stamped source bytes.
std::string reward_store_sha256(const std::vector<std::uint8_t>&);
}
