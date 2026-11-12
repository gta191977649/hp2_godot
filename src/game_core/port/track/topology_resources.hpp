#pragma once
// Port-owned lifetime carrier for loaded original raw topology records.
// TASK-0099 E-0099-W26; explicit Retail resource -> Debug consumer binding.
#include "game_core/port/driving/memory.hpp"
#include <vector>
namespace port::track {
struct TopologyResources {
 ::port::driving::Memory* owner=nullptr;
 std::vector<std::uint32_t> allocations;
 bool loaded=false;
};
TopologyResources load_retail_topology_resources(::port::driving::Memory&,const std::vector<std::uint8_t>&);
void unload_topology_resources(::port::driving::Memory&,TopologyResources&);
}
