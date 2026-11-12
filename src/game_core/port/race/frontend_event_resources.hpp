#pragma once
// Port-only owner retaining complete authored FRONTB EventTree chunks. Raw
// progress points into the actual source Fred buffers, never decoded profiles.
#include "game_core/port/driving/memory.hpp"
#include <optional>
#include <string>
namespace hp2::debug {
int LoaderEventTree(::port::driving::Memory&,std::uint32_t,std::optional<std::uint32_t> center_w); //001FB138.
int UnloaderEventTree(::port::driving::Memory&,std::uint32_t); //001FB378.
std::uint32_t GetEventTree(const ::port::driving::Memory&,const std::string&); //001FB3F0.
}
namespace port::race {
// Exact selective list/hash component of CRT001FD868, plus the fresh-profile
// event-buffer component of Fred.InitWithDefaults00181D78. Not either whole
// function. Rejects nonzero existing profile bytes rather than erasing them.
void initialize_frontend_event_resource_state(driving::Memory&);
// W80 selective data component of tEventTreeView001F5060 goal-edge construction
// and tEventLeaf.BuildUnlockList001F3630. No UI leaf/model/pose is constructed.
void build_frontend_unlock_map(driving::Memory&,std::uint32_t tree,std::uint32_t info,std::uint32_t map);
class FrontendEventResources {
 driving::Memory& memory_;
 std::vector<std::uint32_t> allocations_,chunks_,trees_;
public:
 explicit FrontendEventResources(driving::Memory& m):memory_(m){}
 void load_bundle(const std::vector<std::uint8_t>&,std::optional<std::uint32_t> center_w);
 void close();
 void release_race_event(); //Port publication cleanup only after native race ownership ends.
 const std::vector<std::uint32_t>& trees()const{return trees_;}
};
}
