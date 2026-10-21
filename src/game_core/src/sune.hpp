#pragma once
#include "game_core/port/track/environment.hpp"
namespace hp2::recovered {
// Original src/sune.cpp; bounded post-projection six-layer packet generation.
std::vector<port::track::SunQuad> StuffSun(port::driving::Memory&,const port::track::EnvironmentResources&,unsigned,
 float,float,float,unsigned,unsigned,float,unsigned);
}
