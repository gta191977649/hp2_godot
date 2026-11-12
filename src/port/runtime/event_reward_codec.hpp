#pragma once
// TASK-0112 E-0112-R09; port semantic storage carrier. E-0102-RP01/RJ01
// supplies reviewed version offsets/identity projection. No recovered ABI.
#include <array>
#include <cstdint>
#include <vector>
#include <string>
namespace port::driving {class Memory;}
namespace port::runtime {
// Digest is SHA256(source tasks314510[560] || points314740[228]),
// byte-checked against the authorized ELF on2026-11-07. No table payload saved.
inline constexpr const char* event_reward_debug_image_sha256="815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b";
inline constexpr const char* event_reward_debug_tables_sha256="8d0911aa55fc5bea99ec6d250d377ff2ce95f1eb51b2559bea2a41e01c5ecc4e";
// Private save-owner metadata, NOT original game fields or an independent flag19.
// The coupled writer must bind exact stamped main bytes and validate all fields
// before importing any source words. Empty identity/hash is never a valid seal.
struct EventRewardSupplementMetadata {
 std::uint32_t schema=1,main_payload_size=0x114e0;
 std::string debug_image_sha256=event_reward_debug_image_sha256;
 std::string debug_tables_sha256=event_reward_debug_tables_sha256;
 std::string main_payload_sha256;
 std::array<std::uint8_t,16> career_identity{};
 std::uint64_t sequence=0;
};
struct EventRewardTransfer {
 std::array<std::uint32_t,10> task_words{};
 // Physical Debug words0..18 only. Spare5 is retained; index19 aliases abortion.
 std::array<std::uint32_t,19> point_words{};
};
EventRewardTransfer read_debug_event_rewards(const driving::Memory&,std::uint32_t fred);
void write_debug_event_rewards(driving::Memory&,std::uint32_t fred,const EventRewardTransfer&);
// Absence of a supplement: reviewed Retail identities overlay actual freshly
// initialized Debug bits. Never copy Retail final-car17 into Debug abortion.
EventRewardTransfer import_retail_event_rewards(const std::vector<std::uint8_t>& retail,const EventRewardTransfer& debug_baseline);
// An actual sealed source award receipt is required to set Retail final-car17;
// false preserves that independent Retail baseline field bit-for-bit.
std::vector<std::uint8_t> project_retail_event_rewards(const EventRewardTransfer&,const std::vector<std::uint8_t>& retail_baseline,bool actual_final_reward_awarded=false);
// Canonical LE physical payload for a metadata/hash-bound private supplement.
// These functions perform no file I/O and are not a complete save protocol.
std::vector<std::uint8_t> encode_debug_reward_payload(const EventRewardTransfer&);
EventRewardTransfer decode_debug_reward_payload(const std::vector<std::uint8_t>&);
}
