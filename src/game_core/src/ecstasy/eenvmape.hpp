#pragma once
#include "game_core/port/track/environment.hpp"
namespace hp2::recovered {
// Original TU src/ecstasy/eenvmape.cpp; explicit host values, not PS2 ABI.
void ComputeReflectionST(port::track::EnvironmentVector&,std::array<float,2>&);
port::track::ReflectionTables CreateRefSampleTable();
port::track::EnvironmentWarp CreateEnvMap(const port::track::ReflectionTables&,unsigned,unsigned,unsigned);
port::track::EnvironmentWarp CreateBackEnvMap(const port::track::ReflectionTables&,unsigned,unsigned,unsigned);
std::vector<port::track::EnvironmentWarp> eCreateEnvMapPrims(const port::track::ReflectionTables&,
    const std::array<bool,6>&,unsigned,unsigned);
class eEnvMap {
public:
    static port::track::EnvironmentCameras UpdateCameras(const port::track::EnvironmentVector&,
        const port::track::EnvironmentVector&,std::uint32_t,int view_mode,bool in_game);
};
}
