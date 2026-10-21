#pragma once
#include "game_core/port/track/environment.hpp"
namespace hp2::recovered {
struct SkySceneryCandidate{std::size_t instance;std::uint32_t hash;port::track::EnvironmentMatrix matrix;};
struct SkyScenerySection{int section;std::vector<SkySceneryCandidate> values;};
port::track::SkyModel SkyInitModel(const std::vector<SkyScenerySection>&,std::uint32_t);
void SkyLoader(port::track::EnvironmentResources&,const std::vector<SkyScenerySection>&);
const port::track::SkyModel* StuffSky(const port::track::EnvironmentResources&,unsigned view_id,bool draw_sky);
}
