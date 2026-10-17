#pragma once
// TASK-0048 E-0048-T03. Exact original header owner ../indep/inc/Polyhedra.hpp
// appears in Debug decompilation. This is a host binding, not the PS2 layout.
#include "game_core/port/track/topology.hpp"
#include "game_core/port/track/voronoi.hpp"
#include "game_core/indep/src/voronoiclip.hpp"
#include <cmath>
#include <stdexcept>
namespace hp2::recovered {
class WaferPolyhedron {
public:
    // Debug 0x002132C8; NFSHP2.ELF SHA-256
    // 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
    // E-0048-T03. Adjacency and feature construction; host vectors own allocation.
    static void Init(port::track::VoronoiPolyhedron& out,const port::track::Polygon& polygon,const std::array<float,3>& center){
        // Original bEqual(v2,v3,0.001f), Debug00325EEC; confirmed componentwise comparison below.
        const bool triangle=std::abs(polygon.vertices[2][0]-polygon.vertices[3][0])<=0.001f
            &&std::abs(polygon.vertices[2][1]-polygon.vertices[3][1])<=0.001f
            &&std::abs(polygon.vertices[2][2]-polygon.vertices[3][2])<=0.001f;
        const unsigned count=triangle?3:4;
        // Original Debug00313750..0031380F imported adjacency symbols, read via Ghidra.
        constexpr unsigned WaferEdgesNearVertices[4][2]={{0,3},{0,1},{1,2},{2,3}};
        constexpr unsigned TriWaferEdgesNearVertices[3][2]={{0,2},{0,1},{1,2}};
        constexpr unsigned WaferVerticesNearEdges[4][2]={{0,1},{1,2},{2,3},{0,3}};
        constexpr unsigned TriWaferVerticesNearEdges[3][2]={{0,1},{1,2},{0,2}};
        constexpr unsigned WaferFacesNearEdges[4][2]={{0,1},{0,1},{0,1},{1,0}};
        port::track::VoronoiPolyhedron pending;pending.vertices.resize(count);pending.edges.resize(count);pending.faces.resize(2);
        for(unsigned i=0;i<count;++i){auto& v=pending.vertices[i];for(unsigned j=0;j<3;++j)v.position[j]=polygon.vertices[i][j]-center[j];
            const auto* adjacency=triangle?TriWaferEdgesNearVertices[i]:WaferEdgesNearVertices[i];v.edges={adjacency[0],adjacency[1]};}
        float distance=0;for(unsigned j=0;j<3;++j)distance+=(polygon.vertices[0][j]-center[j])*polygon.plane[j];
        for(unsigned i=0;i<2;++i){auto& f=pending.faces[i];for(unsigned j=0;j<3;++j)f.plane.normal[j]=i?-polygon.plane[j]:polygon.plane[j];f.plane.distance=i?-distance:distance;
            f.planes.resize(count);for(unsigned j=0;j<count;++j)f.edges.push_back(j);}
        for(unsigned i=0;i<count;++i){auto& e=pending.edges[i];const auto* v=triangle?TriWaferVerticesNearEdges[i]:WaferVerticesNearEdges[i];
            e.vertices={{v[0],v[1]}};const auto* f=WaferFacesNearEdges[triangle?i+1:i];e.faces={{f[0],f[1]}};
            std::array<float,3> direction{};float square=0;for(unsigned j=0;j<3;++j){direction[j]=pending.vertices[v[1]].position[j]-pending.vertices[v[0]].position[j];square+=direction[j]*direction[j];}
            if(!(square>0))throw std::invalid_argument("Degenerate wafer edge");const float inverse=1.0f/std::sqrt(square);
            // Debug VCPolyhedron::NewEdge001D0340 truncates each normalized direction*8192 to signed16.
            for(unsigned j=0;j<3;++j)e.direction[j]=static_cast<std::int16_t>(static_cast<int>(direction[j]*inverse*8192.0f));
        }
        // Debug VCPolyhedron::SetVoronoiPlanes001D0990 order: vertices, faces, edges.
        for(unsigned i=0;i<count;++i)VCVertex::SetVoronoiPlanes(pending,i);
        for(unsigned i=0;i<2;++i)VCFace::SetVoronoiPlanes(pending,i);
        for(unsigned i=0;i<count;++i)VCEdge::SetVoronoiPlanes(pending,i);
        out=std::move(pending);
    }
};
}
#include "game_core/port/assets/elab_chunks.hpp"
#include <cstring>
namespace hp2::recovered {
class PipelinePolyhedron {
public:
    // Debug 0x00213D00; E-0048-T04. Original one-time center-of-gravity correction.
    static void MoveCenterOfGravity(port::track::CollisionVolume& volume,const std::array<float,3>& cog){
        for(auto& v:volume.shape.vertices)for(unsigned j=0;j<3;++j)v.position[j]-=cog[j];
        for(auto& f:volume.shape.faces)f.plane.distance-=((f.plane.normal[0]*cog[0]+f.plane.normal[1]*cog[1])+f.plane.normal[2]*cog[2]);
        for(unsigned i=0;i<volume.shape.vertices.size();++i)VCVertex::SetVoronoiPlanes(volume.shape,i);
        for(unsigned i=0;i<volume.shape.faces.size();++i)VCFace::SetVoronoiPlanes(volume.shape,i);
        for(unsigned i=0;i<volume.shape.edges.size();++i)VCEdge::SetVoronoiPlanes(volume.shape,i);
        volume.center_of_gravity_fixed=true;
    }
};
// Debug 0x00213A30; E-0048-T04. Original collision-volume chunk walk, host-owned output.
inline bool LoaderCollisionVolumes(const std::vector<std::uint8_t>& bytes,const hp2::assets::Chunk& group,std::vector<port::track::CollisionVolume>& volumes){
    if(group.tag!=0x80034020)return false;
    const auto& chunks=group.children;
    if(chunks.size()%5)throw std::invalid_argument("Incomplete collision volume chunk group");
    for(std::size_t i=0;i<chunks.size();i+=5){
        const auto& header=chunks[i];const auto address=(header.header_offset+0x17)&~std::size_t(15);
        if(address>bytes.size()||bytes.size()-address<96||address+96>header.payload_offset()+header.size)throw std::invalid_argument("Truncated pipeline polyhedron header");
        port::track::CollisionVolume volume;std::memcpy(&volume.name_hash,bytes.data()+address+48,4);
        for(unsigned j=0;j<32&&bytes[address+52+j];++j)volume.name.push_back(static_cast<char>(bytes[address+52+j]));
        port::track::VoronoiImage image;std::vector<std::uint8_t>* images[4]={&image.vertices,&image.edges,&image.faces,&image.planes};
        for(unsigned j=0;j<4;++j){const auto& c=chunks[i+j+1];const auto start=(c.header_offset+0x17)&~std::size_t(15);const auto end=c.payload_offset()+c.size;
            if(start>end||end>bytes.size())throw std::invalid_argument("Invalid pipeline polyhedron record alignment");images[j]->assign(bytes.begin()+start,bytes.begin()+end);}
        VCPolyhedron::MemoryImageLoad(volume.shape,image);volumes.push_back(std::move(volume));
    }return true;
}
// Debug 0x00213C98; E-0048-T04. Original first hash match and one-time COG correction.
inline port::track::CollisionVolume* FindPipelinePolyhedron(std::vector<port::track::CollisionVolume>& volumes,std::uint32_t hash,const std::array<float,3>* cog){
    for(auto& volume:volumes)if(volume.name_hash==hash){if(cog&&!volume.center_of_gravity_fixed)PipelinePolyhedron::MoveCenterOfGravity(volume,*cog);return &volume;}return nullptr;
}
}
namespace port::track {
// Port-side complete bundle traversal and transactional publication.
inline std::vector<CollisionVolume> load_collision_volumes(const std::vector<std::uint8_t>& bytes){
    std::vector<CollisionVolume> volumes;std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(bytes,&consumed);
    if(consumed!=bytes.size())throw std::invalid_argument("Incomplete collision volume bundle");
    const auto visit=[&](const auto& self,const auto& list)->void{for(const auto& chunk:list){if(hp2::recovered::LoaderCollisionVolumes(bytes,chunk,volumes))continue;if(chunk.is_container())self(self,chunk.children);}};visit(visit,chunks);return volumes;
}
}
namespace hp2::recovered {
class RectPolyhedron {
public:
    // Debug 0x00212EB0; E-0048-T03. Original constructor as an explicit host operation.
    static void Sub_00212EB0(port::track::VoronoiPolyhedron& out,float x1,float x0,float y1,float y0,float z1,float z0){
        constexpr unsigned RectEdgesNearFaces[6][4]={{0,1,2,3},{4,5,6,7},{0,8,4,9},{2,10,6,11},{1,9,5,10},{3,11,7,8}};
        constexpr unsigned RectEdgesNearVertices[8][3]={{0,1,9},{4,5,9},{1,2,10},{5,6,10},{3,0,8},{7,4,8},{2,3,11},{6,7,11}};
        constexpr unsigned RectFacesNearEdges[12][2]={{2,0},{0,4},{0,3},{5,0},{1,2},{4,1},{3,1},{1,5},{2,5},{4,2},{3,4},{5,3}};
        constexpr unsigned RectVerticesNearEdges[12][2]={{0,4},{0,2},{2,6},{4,6},{1,5},{1,3},{3,7},{5,7},{4,5},{0,1},{2,3},{6,7}};
        port::track::VoronoiPolyhedron p;p.vertices.resize(8);p.edges.resize(12);p.faces.resize(6);
        for(unsigned i=0;i<8;++i){auto& v=p.vertices[i];v.position={{(i&1)?x0:x1,(i&2)?y0:y1,(i&4)?z0:z1}};v.edges.assign(RectEdgesNearVertices[i],RectEdgesNearVertices[i]+3);v.planes.resize(3);}
        const float distances[6]={x1,-x0,y1,-y0,z1,-z0};
        for(unsigned i=0;i<6;++i){auto& f=p.faces[i];f.plane.normal[i/2]=(i&1)?-1.0f:1.0f;f.plane.distance=distances[i];f.edges.assign(RectEdgesNearFaces[i],RectEdgesNearFaces[i]+4);f.planes.resize(4);}
        for(unsigned i=0;i<12;++i){auto& e=p.edges[i];for(unsigned j=0;j<2;++j){e.vertices[j]=RectVerticesNearEdges[i][j];e.faces[j]=RectFacesNearEdges[i][j];}
            std::array<float,3> dir{};for(unsigned j=0;j<3;++j)dir[j]=p.vertices[e.vertices[1]].position[j]-p.vertices[e.vertices[0]].position[j];
            float width=0;if(dir[0]!=0)width=x1-x0;else if(dir[1]!=0)width=y1-y0;else if(dir[2]!=0)width=z1-z0;
            const float inverse=width!=0?1.0f/width:1.0f;for(unsigned j=0;j<3;++j)e.direction[j]=static_cast<std::int16_t>(static_cast<int>(dir[j]*inverse*8192.0f));}
        for(unsigned i=0;i<8;++i)VCVertex::SetVoronoiPlanes(p,i);
        for(unsigned i=0;i<6;++i)VCFace::SetVoronoiPlanes(p,i);
        for(unsigned i=0;i<12;++i)VCEdge::SetVoronoiPlanes(p,i);
        out=std::move(p);
    }
};
}
