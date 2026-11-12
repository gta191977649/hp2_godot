#pragma once
// Port-only ownership of complete authored collision-volume source records.
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/track/voronoi.hpp"
namespace port::race {
struct CollisionResources {std::vector<std::uint32_t> owned_allocations,polyhedra;bool source_data_loaded=false;};
CollisionResources load_collision_resources(::port::driving::Memory&,const std::vector<std::vector<std::uint8_t>>& complete_bundles);
void unload_collision_resources(::port::driving::Memory&,CollisionResources&);
// Port-only value view of already fixed original pointers, for shared recovered
// Voronoi math; no replacement layout, topology or geometry is generated here.
::port::track::VoronoiPolyhedron read_collision_polyhedron(const ::port::driving::Memory&,std::uint32_t);
}
