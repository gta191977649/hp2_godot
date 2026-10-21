#include "track_environment.hpp"
#include <stdexcept>
namespace port::driving {
int debug_terrain_from_retail(int value){
    // E-0048-D01: all 33 embedded Type indices and exact Name strings independently read
    // from Retail318c00 and Debug3102e8. Identity is a proven semantic map, not an ABI cast.
    if(value<0||value>=33)throw std::runtime_error("Unmapped Retail terrain type");
    // E-0087-W01: original elevation/Wheel and static wafer/Contact paths admit
    // PUDDLE4, DEEP_WATER5 and MUD_PUDDLE20. Wheel coefficients come from their
    // original rows; static friction/bounciness are original terrain-independent
    // CollisionBody constants (Debug001D5CC8/001D5CD8), not ROAD substitution.
    if(value==32)throw std::runtime_error("Track CAR terrain ownership is not recovered");
    return value;
}
float TrackContact::GetElevation(Memory& m,std::uint32_t,std::uint32_t point,std::uint32_t material,std::uint32_t plane,std::uint32_t found){
    if(!environment)throw std::logic_error("Track contact has no environment");
    const auto p=m.vector(point);
    const auto hit=environment->topology.elevation({{p[0],p[1],p[2]}});
    const auto terrain=debug_terrain_from_retail(hit.terrain);
    if(material)m.word(material,terrain);
    if(plane)m.vector(plane,hit.plane);
    if(found)m.word(found,hit.found?1:0);
    return hit.height;
}
}
