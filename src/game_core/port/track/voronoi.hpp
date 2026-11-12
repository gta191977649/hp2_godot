#pragma once
// TASK-0048. Port-owned storage for reconstructed Debug Voronoi geometry.
#include <array>
#include <cstdint>
#include <vector>
#include <string>
namespace port::track {
struct VoronoiPlane { std::array<float,3> normal{};float distance=0; };
struct VoronoiVertex {std::array<float,3> position{};std::vector<unsigned> edges;std::vector<VoronoiPlane> planes;};
struct VoronoiEdge {std::array<std::int16_t,3> direction{};std::array<unsigned,2> vertices{},faces{};std::array<VoronoiPlane,4> planes{};};
struct VoronoiFace {VoronoiPlane plane;std::vector<unsigned> edges;std::vector<VoronoiPlane> planes;};
struct VoronoiImage {std::vector<std::uint8_t> vertices,edges,faces,planes;};
struct VoronoiPolyhedron {std::vector<VoronoiVertex> vertices;std::vector<VoronoiEdge> edges;std::vector<VoronoiFace> faces;};
struct VoronoiPose {std::array<std::array<float,3>,3> basis{{{{1,0,0}},{{0,1,0}},{{0,0,1}}}};std::array<float,3> position{};};
struct VoronoiTransform {std::array<std::array<float,3>,3> basis{};std::array<float,3> translation{};};
struct VoronoiTransformedFeature {std::uint32_t feature=0xffffffffu;std::array<float,3> head{},tail{},edge{};};
struct VoronoiPlaneNeighbor {VoronoiPlane plane;std::uint32_t feature=0xffffffffu;};
struct VoronoiClip {float minimum=0,maximum=1;std::uint32_t neighbor_min=0xffffffffu,neighbor_max=0xffffffffu;bool penetration=false;};
// Port-only write observations let the raw Memory binding preserve original CP
// scalar-only stores and distinguish known vector lanes from unknown stack pads.
enum class VoronoiPointWrite : std::uint8_t {none,xyz_only,vector_w0,vector_w1,unknown_w};
struct VoronoiWitness {std::uint32_t feature1=0,feature2=0;std::array<float,3> point1{},point2{};float distance=0;int switch_state=0;
 std::array<VoronoiPointWrite,2> port_point_writes{};
 std::array<std::uint32_t,2> port_point_write_sites{};
};
struct CollisionVolume {std::uint32_t name_hash=0;std::string name;VoronoiPolyhedron shape;bool center_of_gravity_fixed=false;};
}
