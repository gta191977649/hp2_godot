// TASK-0099 E-0099-W23: original ..\indep\src\polyhedra.cpp ownership is
// corroborated by read-only MCP FDR/SYMR/source-marker/string captures.
#include "game_core/indep/inc/Polyhedra.hpp"
#include "game_core/port/race/collision_resources.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cstring>

namespace port::race {
::port::track::VoronoiPolyhedron read_collision_polyhedron(const ::port::driving::Memory& m,std::uint32_t at){
    ::port::track::VoronoiPolyhedron p;const auto vertices=m.word(at+0x20),edges=m.word(at+0x24),faces=m.word(at+0x28);
    const int nv=m.signed_half(at+0x10),ne=m.signed_half(at+0x12),nf=m.signed_half(at+0x14);
    if(nv<0 || ne<0 || nf<0)throw std::out_of_range("Negative original collision feature count");
    p.vertices.resize(nv);p.edges.resize(ne);p.faces.resize(nf);
    const auto index=[](std::uint32_t pointer,std::uint32_t base,unsigned stride,std::size_t count){
        if(pointer<base || (pointer-base)%stride || (pointer-base)/stride>=count)throw std::out_of_range("Original collision neighbor pointer outside feature span");return (pointer-base)/stride;};
    for(unsigned i=0;i<p.vertices.size();++i){const auto record=vertices+20*i,planes=m.word(record+16);auto& v=p.vertices[i];
        for(unsigned j=0;j<3;++j)v.position[j]=m.scalar(record+4+4*j);
        for(unsigned j=0;j<m.byte(record+1);++j)v.edges.push_back(index(m.word(planes+24*j+20),edges,12,p.edges.size()));}
    for(unsigned i=0;i<p.edges.size();++i){const auto record=edges+12*i,planes=m.word(record+8);auto& e=p.edges[i];
        if(m.byte(record+1)!=4)throw std::out_of_range("Original collision edge must have four neighbor planes");
        for(unsigned j=0;j<3;++j)e.direction[j]=m.signed_half(record+2+2*j);
        for(unsigned j=0;j<2;++j){e.vertices[j]=index(m.word(planes+24*j+20),vertices,20,p.vertices.size());e.faces[j]=index(m.word(planes+24*(j+2)+20),faces,24,p.faces.size());}}
    for(unsigned i=0;i<p.faces.size();++i){const auto record=faces+24*i,planes=m.word(record+20);auto& f=p.faces[i];
        for(unsigned j=0;j<3;++j)f.plane.normal[j]=m.scalar(record+4+4*j);f.plane.distance=m.scalar(record+16);
        for(unsigned j=0;j<m.byte(record+1);++j)f.edges.push_back(index(m.word(planes+24*j+20),edges,12,p.edges.size()));}
    return p;
}
CollisionResources load_collision_resources(::port::driving::Memory& m,const std::vector<std::vector<std::uint8_t>>& bundles){
    if(m.word(0x328a50)!=0x328a50 || m.word(0x328a54)!=0x328a50)throw std::logic_error("Collision source owner requires original initialized empty pipeline list");
    CollisionResources resources;
    for(const auto& bytes:bundles){std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(bytes,&consumed);
        if(consumed!=bytes.size())throw std::out_of_range("Incomplete original collision source bundle");
        const auto copy=[&](std::size_t offset,std::size_t size){if(!size || offset>bytes.size() || size>bytes.size()-offset)throw std::out_of_range("Invalid original collision source span");
            const auto pointer=m.allocate(std::uint32_t(size));std::memcpy(m.view(pointer,size).data,bytes.data()+offset,size);resources.owned_allocations.push_back(pointer);return pointer;};
        const auto visit=[&](const auto& self,const auto& list)->void{for(const auto& group:list){
            if(group.tag!=0x80034020){self(self,group.children);continue;}
            if(group.children.size()%5)throw std::out_of_range("Incomplete original five-chunk collision volume group");
            for(std::size_t i=0;i<group.children.size();i+=5){const auto& h=group.children[i];const auto begin=(h.header_offset+0x17)&~std::size_t(15);
                if(begin+96>h.payload_offset()+h.size)throw std::out_of_range("Truncated original pipeline header");const auto poly=copy(begin,96);
                std::array<std::uint32_t,4> arrays{};std::array<int,4> counts{};constexpr unsigned strides[4]={20,12,24,24};
                for(unsigned j=0;j<4;++j){const auto& child=group.children[i+j+1];const auto start=(child.header_offset+0x17)&~std::size_t(15),end=child.payload_offset()+child.size;
                    if(start>end || (end-start)%strides[j])throw std::out_of_range("Invalid original collision feature span");
                    arrays[j]=end==start?copy(child.header_offset,8)+8:copy(start,end-start);counts[j]=int((end-start)/strides[j]);}
                const auto tail=m.word(0x328a54);m.word(tail,poly);m.word(poly+4,tail);m.word(0x328a54,poly);m.word(poly,0x328a50);
                hp2::recovered::VCPolyhedron::MemoryImageLoad(m,poly,arrays,counts);hp2::recovered::VCPolyhedron::SetVoronoiPlanes(m,poly);resources.polyhedra.push_back(poly);
            }
        }};visit(visit,chunks);
    }
    resources.source_data_loaded=true;return resources;
}
void unload_collision_resources(::port::driving::Memory& m,CollisionResources& resources){
    if(!resources.source_data_loaded)throw std::logic_error("Collision source owner is not loaded");
    for(auto body=m.word(0x3283d8);body!=0x3283d8;body=m.word(body))for(auto poly:resources.polyhedra)
        if(m.word(body+8)==poly)throw std::logic_error("Collision source polyhedron remains referenced by live original body");
    for(auto poly:resources.polyhedra){const auto next=m.word(poly),previous=m.word(poly+4);m.word(previous,next);m.word(next+4,previous);}
    for(auto it=resources.owned_allocations.rbegin();it!=resources.owned_allocations.rend();++it)m.release(*it);resources=CollisionResources{};
}
}
namespace hp2::debug {
void Sub_002141F8(::port::driving::Memory& m){m.word(0x328a54,0x328a50);m.word(0x328a50,0x328a50);}
// Debug00213C98, E-0099-W23. Original first-match lookup and one-time correction.
std::uint32_t FindPipelinePolyhedron(::port::driving::Memory& m,std::uint32_t hash,const std::array<float,3>* cog){
    for(auto poly=m.word(0x328a50);poly!=0x328a50;poly=m.word(poly))if(m.word(poly+0x30)==hash){
        if(cog && m.word(poly+0x54)==0)hp2::recovered::PipelinePolyhedron::MoveCenterOfGravity(m,poly,*cog);return poly;}
    return 0;
}
}
namespace hp2::recovered {
// Debug00213D00, E-0099-W23. Move actual vertices/faces, then recompute planes.
void PipelinePolyhedron::MoveCenterOfGravity(::port::driving::Memory& m,std::uint32_t at,const std::array<float,3>& cog){
    for(int i=0;i<m.signed_half(at+0x10);++i){const auto vertex=m.word(at+0x20)+20*std::uint32_t(i);for(unsigned j=0;j<3;++j)m.scalar(vertex+4+4*j,m.scalar(vertex+4+4*j)-cog[j]);}
    for(int i=0;i<m.signed_half(at+0x14);++i){const auto face=m.word(at+0x28)+24*std::uint32_t(i);
        const float dot=(m.scalar(face+4)*cog[0]+m.scalar(face+8)*cog[1])+m.scalar(face+12)*cog[2];m.scalar(face+16,m.scalar(face+16)-dot);}
    VCPolyhedron::SetVoronoiPlanes(m,at);m.word(at+0x54,1);
}
// Debug00212EB0, E-0099-W23. Embedded arrays and adjacency use the existing
// recovered constructor math, stored in the actual original record layout.
void RectPolyhedron::Sub_00212EB0(::port::driving::Memory& m,std::uint32_t at,float x1,float x0,float y1,float y0,float z1,float z0){
    ::port::track::VoronoiPolyhedron p;Sub_00212EB0(p,x1,x0,y1,y0,z1,z0);
    const auto vertices=at+0x30,edges=at+0xd0,faces=at+0x160,planes=at+0x1f0;m.word(at+8,0);m.word(at+0xc,0);
    const std::array<std::uint32_t,4> arrays{{vertices,edges,faces,planes}};const unsigned counts[4]={8,12,6,96};
    for(unsigned i=0;i<4;++i){m.half(at+0x10+2*i,std::uint16_t(counts[i]));m.half(at+0x18+2*i,std::uint16_t(counts[i]));m.word(at+0x20+4*i,arrays[i]);}
    std::uint32_t cursor=planes;
    const auto plane=[&](const ::port::track::VoronoiPlane& p,std::uint32_t neighbor){const auto result=cursor;for(unsigned j=0;j<3;++j)m.scalar(cursor+4+4*j,p.normal[j]);m.scalar(cursor+16,p.distance);m.word(cursor+20,neighbor);cursor+=24;return result;};
    for(unsigned i=0;i<8;++i){const auto v=vertices+20*i;const auto& item=p.vertices[i];m.byte(v,0);m.byte(v+1,std::uint8_t(item.edges.size()));
        for(unsigned j=0;j<3;++j)m.scalar(v+4+4*j,item.position[j]);m.word(v+16,cursor);
        for(unsigned j=0;j<item.edges.size();++j)plane(item.planes[j],edges+12*item.edges[j]);}
    for(unsigned i=0;i<6;++i){const auto f=faces+24*i;const auto& item=p.faces[i];m.byte(f,2);m.byte(f+1,std::uint8_t(item.edges.size()));
        for(unsigned j=0;j<3;++j)m.scalar(f+4+4*j,item.plane.normal[j]);m.scalar(f+16,item.plane.distance);m.word(f+20,cursor);
        for(unsigned j=0;j<item.edges.size();++j)plane(item.planes[j],edges+12*item.edges[j]);}
    for(unsigned i=0;i<12;++i){const auto e=edges+12*i;const auto& item=p.edges[i];m.byte(e,1);m.byte(e+1,4);
        for(unsigned j=0;j<3;++j)m.half(e+2+2*j,std::uint16_t(item.direction[j]));m.word(e+8,cursor);
        for(unsigned j=0;j<4;++j)plane(item.planes[j],j<2?vertices+20*item.vertices[j]:faces+24*item.faces[j-2]);}
}
}

namespace hp2::recovered {
// Debug002132C8, E-0099-W38. Fresh type/adjacency/consumer review corroborates
// canonical E-0048 finite construction; serialize its actual embedded storage.
void WaferPolyhedron::Init(::port::driving::Memory& m,std::uint32_t p,std::uint32_t tp,const std::array<float,3>& center){
 ::port::track::Polygon source;source.count=m.byte(tp+9);for(unsigned i=0;i<4;++i)for(unsigned j=0;j<3;++j)source.vertices[i][j]=m.scalar(tp+0x30+i*16+j*4);
 for(unsigned i=0;i<4;++i)source.plane[i]=m.scalar(tp+0x70+i*4);
 ::port::track::VoronoiPolyhedron shape;Init(shape,source,center);const auto n=static_cast<unsigned>(shape.vertices.size());
 const std::uint32_t vertices=p+0x30,edges=p+0x80,faces=p+0xb0,planes=p+0xe0;const unsigned counts[]{n,n,2,6*n+8};const std::uint32_t arrays[]{vertices,edges,faces,planes};
 m.word(p+8,0);for(unsigned i=0;i<4;++i){m.half(p+0x10+i*2,static_cast<std::uint16_t>(counts[i]));m.half(p+0x18+i*2,static_cast<std::uint16_t>(counts[i]));m.word(p+0x20+i*4,arrays[i]);}
 auto cursor=planes;
 const auto store_plane=[&](std::uint32_t at,const ::port::track::VoronoiPlane& plane,std::uint32_t neighbor){for(unsigned i=0;i<3;++i)m.scalar(at+4+i*4,plane.normal[i]);m.scalar(at+16,plane.distance);m.word(at+20,neighbor);};
 for(unsigned i=0;i<n;++i){const auto at=vertices+i*20;const auto& v=shape.vertices[i];m.byte(at,0);m.byte(at+1,2);for(unsigned j=0;j<3;++j)m.scalar(at+4+j*4,v.position[j]);m.word(at+16,cursor);for(unsigned j=0;j<2;++j)store_plane(cursor+j*24,v.planes[j],edges+12*v.edges[j]);cursor+=48;}
 for(unsigned i=0;i<2;++i){const auto at=faces+i*24;const auto& f=shape.faces[i];m.byte(at,2);m.byte(at+1,static_cast<std::uint8_t>(n));for(unsigned j=0;j<3;++j)m.scalar(at+4+j*4,f.plane.normal[j]);m.scalar(at+16,f.plane.distance);m.word(at+20,cursor);for(unsigned j=0;j<n;++j)store_plane(cursor+j*24,f.planes[j],edges+12*f.edges[j]);
  // Original NewVoronoiPlanes(4), even for a3-edge triangle face.
  cursor+=96;
 }
 for(unsigned i=0;i<n;++i){const auto at=edges+i*12;const auto& e=shape.edges[i];m.byte(at,1);m.byte(at+1,4);for(unsigned j=0;j<3;++j)m.half(at+2+j*2,static_cast<std::uint16_t>(e.direction[j]));m.word(at+8,cursor);for(unsigned j=0;j<4;++j)store_plane(cursor+j*24,e.planes[j],j<2?vertices+20*e.vertices[j]:faces+24*e.faces[j-2]);cursor+=96;}
}
}
