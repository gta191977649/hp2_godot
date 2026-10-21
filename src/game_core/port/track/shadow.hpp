#pragma once
#include "environment.hpp"
#include "topology.hpp"
namespace port::track {
struct ShadowLight {EnvironmentVector direction{};float inverse_z=0;};
struct ShadowVertex {EnvironmentVector position{};std::array<float,2> uv{};};
struct ShadowStrip {bool ramp=true;std::vector<ShadowVertex> vertices;std::array<std::uint8_t,4> colour{};};
struct ShadowPacket {std::vector<ShadowStrip> strips;};
struct ShadowInput {std::vector<EnvironmentVector> collision_vertices;EnvironmentMatrix local_world{};EnvironmentVector car_position{};const Topology* topology=nullptr;float projected_size=0,lod_swap1=45,lod_swap2=20;unsigned car_lod=0;EnvironmentVector bounds_min{},bounds_max{};std::array<EnvironmentVector,4> wheels{};bool fancy=true,fallback=true;};
}
namespace hp2::recovered {
int cmpl(const port::track::EnvironmentVector*,const port::track::EnvironmentVector*);
int cmph(const port::track::EnvironmentVector*,const port::track::EnvironmentVector*);
int make_chain(std::vector<const port::track::EnvironmentVector*>&,std::size_t,int,bool);
std::vector<port::track::EnvironmentVector> ch2d(const std::vector<port::track::EnvironmentVector>&);
std::vector<port::track::EnvironmentVector> convex_hull(const std::vector<port::track::EnvironmentVector>&,const port::track::Topology&,float,float,const port::track::EnvironmentVector&,const port::track::EnvironmentVector&);
std::vector<port::track::EnvironmentVector> smooth_shadow_corners(const std::vector<port::track::EnvironmentVector>&);
port::track::ShadowLight sh_Setup(port::driving::Memory&,const port::track::SunResources&,const port::track::EnvironmentVector&);
}
