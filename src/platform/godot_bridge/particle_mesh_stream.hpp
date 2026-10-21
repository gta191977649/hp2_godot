#pragma once
// Host transport only. No particle simulation/packet/order changes.
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <cstring>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <chrono>
namespace port::godot_bridge {
struct ParticleMeshStream {
    godot::Ref<godot::ArrayMesh> mesh;
    std::size_t capacity=0,active=0;
    std::uint64_t creations=0,surfaces=0,updates=0,bytes=0;
    godot::PackedByteArray vertex_data,attribute_data;
    bool transport_cache=false,layout_valid=false;
    std::uint32_t cached_vstride=0,cached_astride=0,cached_po=0,cached_co=0,cached_uo=0;
    std::uint64_t layout_queries=0;
    double packing_ms=0,upload_ms=0;
    godot::Ref<godot::ArrayMesh> update(const godot::PackedVector3Array& points,
        const godot::PackedVector2Array& uv,const godot::PackedColorArray& colours){
        using Clock=std::chrono::steady_clock;
        const auto packing_begin=Clock::now();packing_ms=upload_ms=0;
        const std::size_t count=points.size();
        if(!count||count%4||uv.size()!=points.size()||colours.size()!=points.size())throw std::invalid_argument("Particle stream requires complete quad attributes");
        active=count/4;
        if(active>std::size_t(std::numeric_limits<std::int32_t>::max())/8)throw std::length_error("Particle stream capacity exceeds host limits");
        if(mesh.is_null()||active>capacity){
            std::size_t next=1;while(next<active)next*=2;
            const auto allocation_begin=Clock::now();
            if(mesh.is_null()){mesh.instantiate();++creations;}else mesh->clear_surfaces();
            upload_ms+=std::chrono::duration<double,std::milli>(Clock::now()-allocation_begin).count();
            capacity=next;
            godot::PackedVector3Array positions;godot::PackedVector2Array texcoords;godot::PackedColorArray colors;godot::PackedInt32Array indices;
            positions.resize(capacity*4);texcoords.resize(capacity*4);colors.resize(capacity*4);indices.resize(capacity*6);
            for(std::size_t q=0;q<capacity;++q)for(unsigned j=0;j<6;++j){constexpr int pattern[]{0,1,2,1,3,2};indices.set(q*6+j,q*4+pattern[j]);}
            godot::Array arrays;arrays.resize(godot::Mesh::ARRAY_MAX);arrays[godot::Mesh::ARRAY_VERTEX]=positions;arrays[godot::Mesh::ARRAY_TEX_UV]=texcoords;arrays[godot::Mesh::ARRAY_COLOR]=colors;arrays[godot::Mesh::ARRAY_INDEX]=indices;
            const auto surface_begin=Clock::now();mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES,arrays,{}, {},godot::Mesh::ARRAY_FLAG_USE_DYNAMIC_UPDATE);++surfaces;
            upload_ms+=std::chrono::duration<double,std::milli>(Clock::now()-surface_begin).count();
            layout_valid=false;
            if(mesh->get_surface_count()!=1)throw std::runtime_error("Particle stream surface upload failed");
        }
        auto* rs=godot::RenderingServer::get_singleton();
        const auto vertices=std::int32_t(capacity*4);
        if(!transport_cache||!layout_valid){
        const auto format=godot::BitField<godot::RenderingServer::ArrayFormat>(std::uint64_t(mesh->surface_get_format(0)));
        const auto vstride=rs->mesh_surface_get_format_vertex_stride(format,vertices),astride=rs->mesh_surface_get_format_attribute_stride(format,vertices);
        const auto po=rs->mesh_surface_get_format_offset(format,vertices,godot::RenderingServer::ARRAY_VERTEX),co=rs->mesh_surface_get_format_offset(format,vertices,godot::RenderingServer::ARRAY_COLOR),uo=rs->mesh_surface_get_format_offset(format,vertices,godot::RenderingServer::ARRAY_TEX_UV);
        // Godot4.4.1 rendering_server.cpp: uncompressed XYZ float3; separate
        // attribute stream RGBA8 followed by float2 UV. Query and fail closed.
        const auto expected=std::uint64_t(godot::Mesh::ARRAY_FORMAT_VERTEX)|godot::Mesh::ARRAY_FORMAT_COLOR|godot::Mesh::ARRAY_FORMAT_TEX_UV|godot::Mesh::ARRAY_FORMAT_INDEX;
        if((std::uint64_t(format)&((1u<<godot::Mesh::ARRAY_MAX)-1u))!=expected||vstride!=12||astride!=12||po!=0||co!=0||uo!=4||(std::uint64_t(format)&godot::Mesh::ARRAY_FLAG_COMPRESS_ATTRIBUTES))throw std::runtime_error("Particle stream requires pinned Godot uncompressed layout");
        cached_vstride=vstride;cached_astride=astride;cached_po=po;cached_co=co;cached_uo=uo;layout_valid=true;++layout_queries;
        }
        const auto vstride=cached_vstride,astride=cached_astride,po=cached_po,co=cached_co,uo=cached_uo;
        vertex_data.resize(std::size_t(vertices)*vstride);attribute_data.resize(std::size_t(vertices)*astride);
        auto* vp=vertex_data.ptrw();auto* ap=attribute_data.ptrw();
        for(std::int32_t i=0;i<vertices;++i){
            // Retain immutable indices: unused capacity quads have four identical
            // valid clip/depth positions and therefore zero raster area.
            const auto p=std::size_t(i)<count?points[i]:points[0];
            const float xyz[]{float(p.x),float(p.y),float(p.z)};std::memcpy(vp+i*vstride+po,xyz,sizeof(xyz));
            const auto c=std::size_t(i)<count?colours[i]:colours[0];
            for(unsigned k=0;k<4;++k)ap[i*astride+co+k]=std::uint8_t(std::clamp(double(c[k])*255.0,0.0,255.0));
            const auto t=std::size_t(i)<count?uv[i]:uv[0];const float st[]{float(t.x),float(t.y)};std::memcpy(ap+i*astride+uo,st,sizeof(st));
        }
        const auto upload_begin=Clock::now();packing_ms=std::chrono::duration<double,std::milli>(upload_begin-packing_begin).count()-upload_ms;
        mesh->surface_update_vertex_region(0,0,vertex_data);mesh->surface_update_attribute_region(0,0,attribute_data);++updates;bytes+=vertex_data.size()+attribute_data.size();upload_ms+=std::chrono::duration<double,std::milli>(Clock::now()-upload_begin).count();return mesh;
    }
};
}
