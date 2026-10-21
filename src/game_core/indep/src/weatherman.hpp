#pragma once
#include "game_core/port/track/environment.hpp"
namespace hp2::recovered {
// Original indep/src/weatherman.cpp, Retail001A84F0 / Debug001A5CD0.
port::track::WeatherResult WeatherManQuery(const std::vector<port::track::WeatherRegion>&,
    const port::track::EnvironmentVector&);
}
