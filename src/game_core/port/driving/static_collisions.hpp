#pragma once
// TASK-0048. Port-owned static-track collision lifetime and callback wiring.
#include "game_core/port/driving/collision.hpp"
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/track/topology.hpp"
#include <map>
#include <string>
namespace port::driving {
struct Config;
struct CollisionMotion {
    port::track::VoronoiPose previous;
    std::array<float,3> velocity{};
    float timestep=0;
};
struct StaticCollisionWitness {port::track::VoronoiPolyhedron wafer;port::track::VoronoiWitness witness;};
struct StaticCollisionEvent {std::size_t polygon=0;ContactTime time;};
class StaticCollisions {
public:
    void configure(Memory&,const Config&,const std::array<float,3>& center_of_gravity,const std::vector<std::uint8_t>& geometry);
    void clear();
    bool ready()const{return !shape.vertices.empty();}
    void process(Memory&,std::uint32_t body,std::uint32_t car,const port::track::Topology&,CollisionMotion&,
        const std::function<void(float)>& move,const std::function<void()>& recompute);
    // Host-owned state and list operations; original World algorithm owns loop/order.
    std::vector<StaticCollisionEvent> contacts(Memory&,std::uint32_t,const port::track::Topology&,const CollisionMotion&);
    void resolve(Memory&,std::uint32_t,std::uint32_t,const StaticCollisionEvent&,CollisionMotion&,
        const std::function<void(float)>&,const std::function<void()>&);
    port::track::VoronoiPolyhedron shape;
    port::track::CollisionCylinder cylinder;
    std::map<std::size_t,StaticCollisionWitness> witnesses;
    std::vector<std::size_t> witness_order;
    std::size_t resolved_contacts=0,applied_impulses=0;
    std::array<float,3> last_impulse{};
    std::map<std::size_t,std::size_t> impulses_by_polygon;
};
port::track::VoronoiPose collision_pose(const Memory&,std::uint32_t body);
}
