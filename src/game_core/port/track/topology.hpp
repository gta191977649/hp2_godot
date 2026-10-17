#pragma once
// TASK-0048. Port-owned storage/query interfaces, not original object layouts.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace port::track {
struct Polygon {
    std::array<std::array<float,3>,4> vertices{};
    std::array<float,4> plane{};
    std::array<float,4> bounds{};
    std::uint8_t count=0,flags=0;
    int terrain=0;
};
// Host values for the original DynamicCollisionBody cylinder broad phase.
struct CollisionCylinder {
    std::array<float,3> center{},axis{{0,0,1}};
    float base_height=0,top_height=0,radius=0;
};
struct Contact {
    bool found=false;
    float height=0;
    std::array<float,4> plane{{0,0,-1,0}};
    int terrain=1;
    std::size_t polygon=static_cast<std::size_t>(-1);
};
class Topology {
public:
    // Input is a complete uncompressed eLab bundle. Transactional on failure.
    void load(const std::vector<std::uint8_t>&);
    void clear();
    bool ready() const { return !nodes.empty()&&!squares.empty()&&!polygons.empty()&&!indices.empty(); }
    // Fresh bounded original candidate query; no cross-call TopologyCoordinate cache emulation.
    Contact elevation(const std::array<float,3>&) const;
    std::vector<std::size_t> intersecting(const std::array<float,2>& minimum,
        const std::array<float,2>& maximum,std::size_t capacity=160) const;
    // Original cylinder broad-phase predicate; no collision response is implied.
    std::vector<std::size_t> collision_candidates(const CollisionCylinder&) const;
    std::vector<std::array<std::uint16_t,4>> nodes;
    std::vector<std::array<std::uint16_t,6>> squares;
    std::vector<Polygon> polygons;
    std::vector<std::uint16_t> indices;
};
}
