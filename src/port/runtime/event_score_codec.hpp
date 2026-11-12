#pragma once
// TASK-0102 E-0102-SP01/I01. Port-only semantic score transport. Original
// named fields/axes and version evidence: reverse/task0102/score-profile-audit.md.
// No Godot dependency, raw Fred overlay, score insertion, RNG or file I/O.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace port::driving { class Memory; }
namespace port::runtime {
enum class EventScoreVersion { Retail, Debug };
struct EventScoreTrack {
 int number=0,same_as=0,score_index=-1;
};
struct EventScoreAxes {
 std::array<int,9> lap_counts{}; // Explicit original lookup table; no default rules.
 std::vector<EventScoreTrack> tracks; // Named/valid tracks after source initialization.
};
struct EventScoreRecord {
 std::int32_t time=0;
 std::int16_t name_index=0,car_type=0;
};
struct EventScoreProfile {
 std::array<EventScoreRecord,7200> total{};
 std::array<EventScoreRecord,800> lap{};
 std::array<std::array<std::uint8_t,12>,128> names{};
};
// Check the original canonical/alias assignment,16-bucket domain and9 lap
// choices. A reordered input vector is allowed; unsupported alias chains and
// ambiguous identities are rejected rather than assigned a guessed bucket.
void validate_event_score_axes(const EventScoreAxes&);
// Read-only observation of actual source TrackInfo.HighScoreIndex metadata.
// Caller must already have run original InitStatsTables on loaded TrackInfo.
EventScoreAxes read_debug_event_score_axes(const driving::Memory&);
// Independently derive Retail metadata from the actual GLOBALB TrackInfo chunk
// (A4 rows). Retail153608 loader clearing and186708 canonical/alias order;
// no raw asset mutation or score initialization occurs here.
EventScoreAxes read_retail_event_score_axes(const std::vector<std::uint8_t>& global);
// Each named field is decoded/encoded separately. Whole-profile baseline size
// is exact for the explicitly selected version. Unknown/unrelated bytes remain
// byte-identical. Return-by-value makes all failures leave caller inputs intact.
EventScoreProfile decode_event_scores(EventScoreVersion,const EventScoreAxes&,
 const std::vector<std::uint8_t>& profile);
std::vector<std::uint8_t> encode_event_scores(EventScoreVersion,const EventScoreAxes&,
 const EventScoreProfile&,const std::vector<std::uint8_t>& profile_baseline);
// Both independently derived axis maps must agree. No arbitrary remapping or
// coalescing is introduced. All8000 records, including unused bucket15, survive.
EventScoreProfile transcode_event_scores(const EventScoreAxes& source_axes,
 const EventScoreProfile&,const EventScoreAxes& destination_axes);
// These functions touch only named scores/names at the proven Debug offsets.
// Import preflights EVERY destination span before any write. It neither calls
// InitStatsTables nor inserts results: call AFTER original metadata/sentinel
// initialization and BEFORE original StartRace/SetRecordTimesToBeat.
EventScoreProfile read_debug_event_scores(const driving::Memory&,std::uint32_t fred,
 const EventScoreAxes&);
void write_debug_event_scores(driving::Memory&,std::uint32_t fred,
 const EventScoreAxes&,const EventScoreProfile&);
// Content is lossless source bits: signed time, all16-bit name/car patterns,
// and all12 name bytes including dirty tails/full non-NUL slots. This codec
// does not assert malformed imported strings/indices are safe game operands.
}
