#pragma once
// Port ownership of original loaded cop/scenery metadata. No PS2 ABI claim.
// E-0099-W17; Debug NFSHP2.ELF815b... with reviewed original asset schemas.
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/track/route_data.hpp"
#include "game_core/port/track/topology.hpp"
#include <vector>
namespace port::race {
struct CopResources {
    bool source_data_loaded=false;
    const ::port::driving::Memory* owner=nullptr;
    const ::port::track::RouteData* routes=nullptr;
    const ::port::track::Topology* topology=nullptr;
    std::uint32_t roadblocks=0,radar_traps=0,landmarks=0;
    std::vector<std::uint32_t> owned_allocations;
};
// Complete original track bundle; absence is established by parsing the actual
// source, never inferred from imported zero globals. Existing source owners must
// be unloaded before a new owner is attached to the same logical Memory.
CopResources load_cop_resources(::port::driving::Memory&,const std::vector<std::uint8_t>&,
                               const ::port::track::RouteData&,const ::port::track::Topology&);
void unload_cop_resources(::port::driving::Memory&,CopResources&);
}
