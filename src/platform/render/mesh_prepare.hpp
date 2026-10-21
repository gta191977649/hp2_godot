#pragma once
// Port-side CPU transport only. No recovered scheduling, Godot types or GPU access.
#include "port/render/vehicle.hpp"
#include <memory>
#include <limits>
#include <stdexcept>

namespace port::render {
struct MeshAttributes {
    std::vector<std::array<float,3>> positions;
    std::vector<std::array<float,2>> uv;
    std::vector<std::array<float,4>> colours, normals;
    std::vector<std::int32_t> indices;
};
struct MeshPreparePart {
    // Exactly one source: immutable decoded production data, or a host test snapshot.
    std::shared_ptr<const VehicleSurface> decoded;
    std::shared_ptr<const MeshAttributes> attributes;
    std::array<float,16> matrix{};
    std::size_t first_vertex=0,vertices=0,first_index=0,indices=0;
};
struct MeshPrepareInput {
    std::vector<MeshPreparePart> parts;
    std::size_t vertices=0,indices=0;
    bool baked_world=false;
};
struct MeshPrepareOutput : MeshAttributes {
    std::array<std::vector<std::array<float,4>>,3> rows;
    std::array<float,3> bounds_position{},bounds_size{};
    bool have_bounds=false;
};
inline MeshPrepareOutput prepare_mesh(const MeshPrepareInput& input) {
    constexpr auto maximum=std::size_t(std::numeric_limits<std::int32_t>::max());
    if(input.parts.empty())throw std::invalid_argument("Empty CPU mesh batch");
    if(input.vertices>maximum||input.indices>maximum)throw std::length_error("CPU mesh exceeds signed array capacity");
    MeshPrepareOutput out;
    out.positions.resize(input.vertices);out.uv.resize(input.vertices);
    out.colours.resize(input.vertices);out.normals.resize(input.vertices);out.indices.resize(input.indices);
    if(input.baked_world)for(auto& row:out.rows)row.resize(input.vertices);
    std::size_t vo=0,io=0;
    for(const auto& part:input.parts){
        if(bool(part.decoded)==bool(part.attributes))throw std::invalid_argument("CPU mesh requires exactly one immutable source");
        const auto count=part.decoded?part.decoded->vertices.size():part.attributes->positions.size();
        const auto indices=part.decoded?part.decoded->indices.size():part.attributes->indices.size();
        if(count!=part.vertices||indices!=part.indices||part.first_vertex!=vo||part.first_index!=io||vo>input.vertices||count>input.vertices-vo||io>input.indices||indices>input.indices-io)
            throw std::runtime_error("CPU mesh source attributes/subranges changed");
        if(part.attributes&&(part.attributes->uv.size()!=count||part.attributes->colours.size()!=count||part.attributes->normals.size()!=count))throw std::runtime_error("CPU mesh source attributes changed");
        const auto& m=part.matrix;
        const std::array<std::array<float,4>,3> rows{{{{m[5],m[9],m[1],m[13]}},{{m[6],m[10],m[2],m[14]}},{{m[4],m[8],m[0],m[12]}}}};
        for(std::size_t i=0;i<count;++i){
            if(part.decoded){const auto& v=part.decoded->vertices[i];
                if(!v.has_color)throw std::runtime_error("Original track VIF colour stream is absent");
                out.positions[vo+i]={{v.position[1],v.position[2],v.position[0]}};out.uv[vo+i]=v.uv;
                const auto c=v.raw_color;
                out.colours[vo+i]={{float((c&31)*8)/255.f,float(((c>>5)&31)*8)/255.f,float(((c>>10)&31)*8)/255.f,float((c&0x8000)?128:0)/255.f}};
                out.normals[vo+i]={{v.normal[0],v.normal[1],v.normal[2],0}};
            }else{out.positions[vo+i]=part.attributes->positions[i];out.uv[vo+i]=part.attributes->uv[i];out.colours[vo+i]=part.attributes->colours[i];out.normals[vo+i]=part.attributes->normals[i];}
            if(input.baked_world){
                for(unsigned r=0;r<3;++r)out.rows[r][vo+i]=rows[r];
                const auto& p=out.positions[vo+i];
                const std::array<float,3> world{{m[1]*p[2]+m[5]*p[0]+m[9]*p[1]+m[13],m[2]*p[2]+m[6]*p[0]+m[10]*p[1]+m[14],m[0]*p[2]+m[4]*p[0]+m[8]*p[1]+m[12]}};
                if(!out.have_bounds){out.bounds_position=world;out.have_bounds=true;}
                else for(unsigned axis=0;axis<3;++axis){
                    // Match Godot AABB::expand_to arithmetic and comparison order.
                    float begin=out.bounds_position[axis],end=begin+out.bounds_size[axis];
                    if(world[axis]<begin)begin=world[axis];if(world[axis]>end)end=world[axis];
                    out.bounds_position[axis]=begin;out.bounds_size[axis]=end-begin;
                }
            }
        }
        for(std::size_t i=0;i<indices;++i){
            const auto index=part.decoded?std::int64_t(part.decoded->indices[i]):std::int64_t(part.attributes->indices[i]);
            if(index<0||std::uint64_t(index)>=count)throw std::runtime_error("CPU mesh source index is invalid");
            out.indices[io+i]=std::int32_t(vo+index);
        }
        vo+=count;io+=indices;
    }
    if(vo!=input.vertices||io!=input.indices)throw std::runtime_error("CPU mesh aggregate counts changed");
    return out;
}
} // namespace port::render
