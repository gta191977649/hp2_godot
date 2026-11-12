#pragma once
// Port-owned binding of recovered race setup to shared original resources.
#include "game_core/port/race/car_construction.hpp"
#include "game_core/port/track/position_markers.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include <optional>
namespace port::race {
// Independent explicit port operands for original uninitialized InitCars locals.
// No equality with CarConstructionServices' unrelated local vector lane is assumed.
struct RaceStartOperands {
 std::array<std::optional<std::uint32_t>,8> racer_start_w;
 std::optional<hp2::debug::NearestRoutePointInfo> negative_override_starting_nri;
};
}
namespace hp2::debug {
// Debug00149E80, E-0099-W34, original indep/src/racecoordinator.cpp.
// Locations are original16-byte RacerStartLocation records; only XYZ is stored.
void CalculateRacerPositions(::port::driving::Memory&,int num_racers,std::uint32_t locations,const ::port::track::RouteData&);
// Debug00149D78, E-0099-W34. Original Car/mode calls precede World publication.
void AddRacingCar(::port::driving::Memory&,std::uint32_t driver,std::uint32_t start_location,int control_mode,const ::port::race::CarConstructionServices&,::port::race::PhysicsConstructionServices&);
}
