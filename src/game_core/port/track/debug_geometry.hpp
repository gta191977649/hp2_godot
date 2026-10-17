#pragma once
// TASK-0049. Port-side diagnostic geometry only; no recovered rendering or material-color claim.
#include "game_core/port/track/topology.hpp"
#include "game_core/port/driving/surface.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
namespace port::track {
using DebugColor=std::array<float,3>;
struct DebugTriangle {std::array<std::array<float,3>,3> vertices{};std::size_t polygon=0;int terrain=0;DebugColor color{};};
struct DebugEdge {std::array<std::array<float,3>,2> vertices{};std::size_t polygon=0;int terrain=0;DebugColor color{};};
struct DebugLegend {int id=0;std::string name;DebugColor color{};bool known=false;std::size_t polygon_count=0;};
struct DebugGeometry {std::vector<DebugTriangle> triangles;std::vector<DebugEdge> edges;std::vector<DebugLegend> legend;std::size_t skipped_polygons=0;};
inline constexpr DebugColor unknown_debug_color{{1,0,1}};
// Five original tool colors remain shared with the synthetic Test drive palette.
// All additional colors are deterministic diagnostic hues, never original texture colors.
inline DebugColor debug_material_color(int terrain,bool known){
    if(!known)return unknown_debug_color;
    for(std::size_t i=0;i<port::driving::surface_terrain_ids.size();++i)
        if(terrain==port::driving::surface_terrain_ids[i])return port::driving::surface_colors[i];
    const double hue=terrain*0.6180339887498949-std::floor(terrain*0.6180339887498949);
    const double sector=hue*6;const auto index=static_cast<int>(sector);const float fraction=static_cast<float>(sector-index);
    constexpr float value=0.85f,saturation=0.55f;const float p=value*(1-saturation),q=value*(1-fraction*saturation),t=value*(1-(1-fraction)*saturation);
    switch(index%6){case 0:return {{value,t,p}};case 1:return {{q,value,p}};case 2:return {{p,value,t}};case 3:return {{p,q,value}};case 4:return {{t,p,value}};default:return {{value,p,q}};}
}
// E-0049-G01: exact elevation candidate flag exclusion; zero-Z planes cannot produce finite elevation.
inline bool debug_road_eligible(const Polygon& polygon){return (polygon.flags&(2|8))==0&&polygon.plane[2]!=0;}
inline DebugGeometry build_debug_geometry(const Topology& topology,const std::vector<port::driving::SurfaceMaterial>& catalog){
    std::map<int,std::string> names;
    for(const auto& entry:catalog){if(entry.id<0||entry.id>=33)throw std::invalid_argument("Debug material catalogue ID is outside the original33-row table");
        if(!names.emplace(entry.id,entry.name).second)throw std::invalid_argument("Duplicate debug material catalogue ID");}
    DebugGeometry result;std::map<int,DebugLegend> legend;
    for(std::size_t index=0;index<topology.polygons.size();++index){const auto& polygon=topology.polygons[index];
        if(polygon.count!=3&&polygon.count!=4)throw std::invalid_argument("Debug contact polygon is neither triangle nor quad");
        for(float value:polygon.plane)if(!std::isfinite(value))throw std::invalid_argument("Nonfinite debug contact plane");
        for(unsigned i=0;i<polygon.count;++i)for(float value:polygon.vertices[i])if(!std::isfinite(value))throw std::invalid_argument("Nonfinite debug contact vertex");
        if(!debug_road_eligible(polygon)){++result.skipped_polygons;continue;}
        const auto name=names.find(polygon.terrain);const bool known=name!=names.end()&&!name->second.empty();const auto color=debug_material_color(polygon.terrain,known);
        auto item=legend.emplace(polygon.terrain,DebugLegend{polygon.terrain,known?name->second:"Unknown",color,known,0}).first;++item->second.polygon_count;
        std::array<std::array<float,3>,4> vertices=polygon.vertices;
        // GetElevation uses the first-three-vertex plane throughout the whole XY footprint.
        // Quads can have a noncoplanar fourth stored vertex: display the actual contact plane,
        // retaining every original XY boundary and independent road layer. Never mutate Topology.
        for(unsigned i=0;i<polygon.count;++i){vertices[i][2]=(polygon.plane[3]-(polygon.plane[0]*vertices[i][0]+polygon.plane[1]*vertices[i][1]))/polygon.plane[2];
            if(!std::isfinite(vertices[i][2]))throw std::invalid_argument("Debug contact elevation is not finite");}
        result.triangles.push_back({{{vertices[0],vertices[1],vertices[2]}},index,polygon.terrain,color});
        if(polygon.count==4)result.triangles.push_back({{{vertices[0],vertices[2],vertices[3]}},index,polygon.terrain,color});
        for(unsigned i=0;i<polygon.count;++i)result.edges.push_back({{{vertices[i],vertices[(i+1)%polygon.count]}},index,polygon.terrain,color});
    }
    for(auto& item:legend)result.legend.push_back(std::move(item.second));return result;
}
}
