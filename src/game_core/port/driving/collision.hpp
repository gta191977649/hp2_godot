#pragma once
// Port-owned value/service bindings, never original Contact or CollisionBody layouts.
#include "game_core/port/track/voronoi.hpp"
#include <functional>
namespace port::driving {
struct StaticContact {std::array<float,3> point{},normal{};};
struct ContactQuery {int status=0;float distance=0;};
using ContactClip=std::function<ContactQuery(float,const std::array<float,3>&)>;
// crowbar is actual car displacement at the host boundary. Original Resolve subtracts
// CrowbarVector for carfirst and adds it for carsecond; the service normalizes that sign.
struct ContactTime {float backup_time=0;std::array<float,3> crowbar{};};
struct StaticImpulse {bool applied=false;std::array<float,3> impulse{},arm{};float scrape_speed=0;};
struct ContactResolveServices {
    std::function<void(float)> backup,move;
    std::function<void(const std::array<float,3>&)> displace;
    std::function<void()> recompute;
    std::function<StaticImpulse()> impulse;
};
}
