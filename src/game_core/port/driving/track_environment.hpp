#pragma once
// TASK-0048. Explicit Retail geometry -> Debug driving service adapter.
#include "game_core/port/track/assets.hpp"
#include "game_core/port/track/topology.hpp"
#include "game_core/port/driving/projections.hpp"
#include <memory>
namespace port::driving {
struct TrackEnvironment {
    std::string label;
    int number=0;
    port::track::Topology topology;
    std::array<float,3> start{};
    std::array<float,2> direction{};
};
// True marker0 spawn only. Missing marker/path/contact is an error, never a synthetic spawn.
std::shared_ptr<const TrackEnvironment> load_track_environment(assets::Source&,const port::track::Entry&);
std::shared_ptr<const TrackEnvironment> load_track_environment(const port::track::Scene&);
int debug_terrain_from_retail(int);
struct TrackContact final:ContactProvider {
    std::shared_ptr<const TrackEnvironment> environment;
    float GetElevation(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)override;
};
}
