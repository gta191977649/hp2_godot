#pragma once
// TASK-0101 E-0101-EP01. Port-only, explicitly versioned progression codec.
// Original allocation/state-field provenance is in event-type-adapter-audit.md.
// No original profile/Fred ABI, raw pointer transfer or whole-version overlay.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace port::runtime {
enum class EventProfileVersion { Retail, Debug };
struct EventStateDefinition {int number=0,type=-1;};
struct EventTreeDefinition {int tree_type=-1;std::vector<EventStateDefinition> events;};
struct EventStateLocation {
 int tree_type=-1;std::size_t offset=0;
 bool operator==(const EventStateLocation& other)const{return tree_type==other.tree_type&&offset==other.offset;}
};
struct EventStateLayout {EventStateDefinition event;EventStateLocation location;std::size_t size=0;};
struct EventProfileLayout {
 EventProfileVersion version;
 std::array<std::vector<EventStateLayout>,2> trees;
 std::array<std::size_t,2> used{{0,0}};
};
using EventProfilePayload=std::array<std::array<std::uint8_t,256>,2>;
// Supply both authored trees in their actual original loader order. Exact
// CalcStatePointers001F9EE0/Retail001FE988 two-pass alignment and shared event32
// alias are represented as block/offset metadata, never serialized addresses.
EventProfileLayout build_event_profile_layout(EventProfileVersion,
 const std::vector<EventTreeDefinition>& original_load_order);
// Both layouts and both payload blocks are explicit. The destination baseline
// preserves padding and Retail KO opaque+4..7 bytes. No implicit zero/reset,
// byte-for-byte KO overlay, RNG draw, status/unlock calculation or file IO.
EventProfilePayload transcode_event_profile(const EventProfileLayout& source_layout,
 const EventProfilePayload& source,const EventProfileLayout& destination_layout,
 const EventProfilePayload& destination_baseline);
}
