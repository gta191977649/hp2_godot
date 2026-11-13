#include "indep/inc/Polyhedra.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Polyhedra.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/VoronoiClip.hpp"

#include <math.h>
#include <stddef.h>
#include <string.h>

void collisionGeometryRecenter(CollisionGeometry* geometry, const float center[4], int32_t* recentered)
{
    float negative[4];
    for(int lane=0;lane<3;++lane) negative[lane]=0.0f-center[lane];
    negative[3]=1.0f-center[3];
    for(int index=0;index<geometry->vertexCount;++index) collisionVertexMove(&geometry->vertices[index],negative);
    for(int index=0;index<geometry->faceCount;++index) collisionFaceMove(&geometry->faces[index],negative);
    collisionGeometrySetPlanes(geometry);
    *recentered=1;
}

CollisionBox* collisionBoxInitialize(CollisionBox* box, const float maximum[3], const float minimum[3])
{
    static const uint8_t vertexEdges[8][3] = {
        {0,1,9}, {4,5,9}, {1,2,10}, {5,6,10},
        {3,0,8}, {7,4,8}, {2,3,11}, {6,7,11}
    };
    static const uint8_t faceEdges[6][4] = {
        {0,1,2,3}, {4,5,6,7}, {0,8,4,9},
        {2,10,6,11}, {1,9,5,10}, {3,11,7,8}
    };
    static const uint8_t edgeFaces[12][2] = {
        {2,0}, {0,4}, {0,3}, {5,0}, {1,2}, {4,1},
        {3,1}, {1,5}, {2,5}, {4,2}, {3,4}, {5,3}
    };
    static const uint8_t edgeVertices[12][2] = {
        {0,4}, {0,2}, {2,6}, {4,6}, {1,5}, {1,3},
        {3,7}, {5,7}, {4,5}, {0,1}, {2,3}, {6,7}
    };
    CollisionGeometry* geometry = &box->geometry;
    collisionGeometryInitialize(geometry);
    collisionGeometryBind(geometry, box->vertices, 8, box->edges, 12, box->faces, 6, box->planes, 96);
    for (int vertex = 0; vertex < 8; ++vertex) {
        float point[3];
        for (int axis = 0; axis < 3; ++axis)
            point[axis] = vertex & (1 << axis) ? minimum[axis] : maximum[axis];
        CollisionPlane* adjacent = collisionGeometryNewPlanes(geometry, 3);
        for (int edge = 0; edge < 3; ++edge) adjacent[edge].feature = &box->edges[vertexEdges[vertex][edge]];
        collisionGeometryNewVertex(geometry, point, 3, adjacent);
    }
    for (int face = 0; face < 6; ++face) {
        const int axis = face / 2;
        float normal[3] = {0,0,0};
        normal[axis] = face & 1 ? -1.0f : 1.0f;
        const float offset = face & 1 ? -minimum[axis] : maximum[axis];
        CollisionPlane* adjacent = collisionGeometryNewPlanes(geometry, 4);
        for (int edge = 0; edge < 4; ++edge) adjacent[edge].feature = &box->edges[faceEdges[face][edge]];
        collisionGeometryNewFace(geometry, normal, 4, adjacent, offset);
    }
    for (int edge = 0; edge < 12; ++edge) {
        CollisionPlane* adjacent = collisionGeometryNewPlanes(geometry, 4);
        for (int side = 0; side < 2; ++side) {
            adjacent[side].feature = &box->vertices[edgeVertices[edge][side]];
            adjacent[side + 2].feature = &box->faces[edgeFaces[edge][side]];
        }
        float direction[3];
        for (int axis = 0; axis < 3; ++axis)
            direction[axis] = adjacent[1].feature->vector[axis] - adjacent[0].feature->vector[axis];
        for (int axis = 0; axis < 3; ++axis) {
            if (direction[axis] == 0.0f) continue;
            const float scale = finiteRatio(1.0f, maximum[axis] - minimum[axis]);
            for (int lane = 0; lane < 3; ++lane) direction[lane] = direction[lane] * scale;
            break;
        }
        collisionGeometryNewEdge(geometry, direction, 4, adjacent);
    }
    collisionGeometrySetPlanes(geometry);
    return box;
}

void collisionGeometryFromTerrainPolygon(CollisionGeometry* geometry,
                                         const TerrainPolygon* polygon, const float center[4])
{
    static const int quadVertexEdges[4][2]={{0,3},{0,1},{1,2},{2,3}};
    static const int triangleVertexEdges[3][2]={{0,2},{0,1},{1,2}};
    static const int edgeFace[4][2]={{0,1},{0,1},{0,1},{1,0}};
    static const int quadEdgeVertices[4][2]={{0,1},{1,2},{2,3},{0,3}};
    static const int triangleEdgeVertices[3][2]={{0,1},{1,2},{0,2}};
    const int triangle=vectorNear3(polygon->vertices[2],polygon->vertices[3],0.001f);
    const int count=triangle ? 3 : 4;
    geometry->vertexCount=geometry->edgeCount=geometry->faceCount=geometry->planeCount=0;
    for (int vertex=0;vertex<count;++vertex) {
        CollisionPlane* adjacent=collisionGeometryNewPlanes(geometry,2);
        for (int side=0;side<2;++side) {
            const int index=triangle ? triangleVertexEdges[vertex][side] : quadVertexEdges[vertex][side];
            adjacent[side].feature=&geometry->edges[index];
        }
        float point[3];
        for (int lane=0;lane<3;++lane) point[lane]=polygon->vertices[vertex][lane]-center[lane];
        collisionGeometryNewVertex(geometry,point,2,adjacent);
    }
    float point[3];
    for (int lane=0;lane<3;++lane) point[lane]=polygon->vertices[0][lane]-center[lane];
    const float x=polygon->plane[0]*point[0],y=polygon->plane[1]*point[1],z=polygon->plane[2]*point[2];
    const float offset=(x+y)+z;
    for (int face=0;face<2;++face) {
        CollisionPlane* adjacent=collisionGeometryNewPlanes(geometry,4);
        for (int edge=0;edge<count;++edge) adjacent[edge].feature=&geometry->edges[edge];
        float normal[3];
        for (int lane=0;lane<3;++lane) normal[lane]=face ? 0.0f-polygon->plane[lane] : polygon->plane[lane];
        collisionGeometryNewFace(geometry,normal,(int8_t)count,adjacent,face ? -offset : offset);
    }
    for (int edge=0;edge<count;++edge) {
        CollisionPlane* adjacent=collisionGeometryNewPlanes(geometry,4);
        for (int side=0;side<2;++side) {
            adjacent[side+2].feature=&geometry->faces[edgeFace[edge+(triangle ? 1 : 0)][side]];
            const int index=triangle ? triangleEdgeVertices[edge][side] : quadEdgeVertices[edge][side];
            adjacent[side].feature=&geometry->vertices[index];
        }
        float direction[3];
        for (int lane=0;lane<3;++lane)
            direction[lane]=adjacent[1].feature->vector[lane]-adjacent[0].feature->vector[lane];
        const float dx=direction[0]*direction[0],dy=direction[1]*direction[1],dz=direction[2]*direction[2];
        const float scale=finiteRatio(1.0f,sqrtf((dx+dy)+dz));
        for (int lane=0;lane<3;++lane) direction[lane]=direction[lane]*scale;
        collisionGeometryNewEdge(geometry,direction,4,adjacent);
    }
    collisionGeometrySetPlanes(geometry);
}

#include "indep/inc/CarInfo.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <stdexcept>

namespace hp2 {
namespace {
constexpr std::uint32_t kContainer=0x80034020u;
constexpr std::array<std::uint32_t,5> kRecordIds={0x34021u,0x34024u,0x34023u,0x34022u,0x34025u};
constexpr std::array<std::size_t,4> kRecordBytes={20,12,24,24};

float real(const std::uint8_t* bytes) {
    const auto word=ReadU32(bytes);float value;std::memcpy(&value,&word,4);
    Require(std::isfinite(value),"Collision resource contains a non-finite value");return value;
}
std::int16_t signedHalf(const std::uint8_t* bytes) {
    const std::uint16_t raw=static_cast<std::uint16_t>(bytes[0]|(std::uint16_t(bytes[1])<<8));
    std::int16_t result;std::memcpy(&result,&raw,2);return result;
}
std::size_t dataStart(const Chunk& chunk) {
    const auto start=(chunk.payloadOffset+15u)&~std::size_t(15u);
    Require(start<=chunk.endOffset,"Truncated aligned collision chunk");return start;
}
}

struct CollisionResourceReader {
    static std::unique_ptr<CollisionResource> Record(const std::vector<std::uint8_t>& data,
                                                    const std::array<Chunk,5>& chunks) {
        auto resource=std::make_unique<CollisionResource>();
        std::array<std::size_t,5> starts{};
        for(std::size_t i=0;i<5;++i) {
            Require(chunks[i].id==kRecordIds[i],"Collision resource has an unexpected record sequence");
            starts[i]=dataStart(chunks[i]);
        }
        Require(chunks[0].endOffset-starts[0]>=53,"Truncated collision object header");
        const auto* header=data.data()+starts[0];
        resource->key_=ReadU32(header+48);
        const auto* nameEnd=static_cast<const std::uint8_t*>(std::memchr(header+52,0,chunks[0].endOffset-starts[0]-52));
        Require(nameEnd!=nullptr,"Collision object name is not terminated");
        resource->name_.assign(reinterpret_cast<const char*>(header+52),static_cast<std::size_t>(nameEnd-(header+52)));
        Require(!resource->name_.empty() && hashString(resource->name_.c_str())==resource->key_,"Collision object name/hash mismatch");
        std::array<std::size_t,4> counts{};
        for(std::size_t i=0;i<4;++i) {
            const auto bytes=chunks[i+1].endOffset-starts[i+1];
            Require(bytes%kRecordBytes[i]==0,"Collision array has a partial record");
            counts[i]=bytes/kRecordBytes[i];
            Require(counts[i]>0 && counts[i]<=static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max()),"Collision array count is outside the supported original range");
        }
        resource->vertices_.resize(counts[0]);resource->edges_.resize(counts[1]);resource->faces_.resize(counts[2]);resource->planes_.resize(counts[3]);
        std::vector<std::uint32_t> vertexPlanes(counts[0]),edgePlanes(counts[1]),facePlanes(counts[2]),planeFeatures(counts[3]);
        const std::array<std::vector<CollisionFeature>*,3> arrays={&resource->vertices_,&resource->edges_,&resource->faces_};
        const std::array<std::vector<std::uint32_t>*,3> links={&vertexPlanes,&edgePlanes,&facePlanes};
        for(std::size_t kind=0;kind<3;++kind) {
            for(std::size_t index=0;index<counts[kind];++index) {
                const auto* encoded=data.data()+starts[kind+1]+index*kRecordBytes[kind];
                auto& feature=(*arrays[kind])[index];
                Require(encoded[0]==kind && encoded[1]<=127,"Collision feature kind or adjacency count is invalid");
                feature.kind=static_cast<std::int8_t>(kind);feature.adjacentCount=static_cast<std::int8_t>(encoded[1]);
                if(kind==1) {
                    for(int lane=0;lane<3;++lane)feature.encodedDirection[lane]=signedHalf(encoded+2+lane*2);
                } else {
                    for(int lane=0;lane<3;++lane)feature.vector[lane]=real(encoded+4+lane*4);
                    if(kind==2) feature.offset=real(encoded+16);
                }
                const auto link=ReadU32(encoded+(kind==0?16:kind==1?8:20));
                Require(link<=counts[3] && static_cast<std::size_t>(feature.adjacentCount)<=counts[3]-link,"Collision adjacency span is outside plane storage");
                (*links[kind])[index]=link;
                if(kind==1)Require(feature.adjacentCount==4,"Collision edge requires two endpoints and two faces");
            }
        }
        for(std::size_t index=0;index<counts[3];++index) {
            const auto* encoded=data.data()+starts[4]+index*24;auto& plane=resource->planes_[index];
            for(int lane=0;lane<3;++lane)plane.normal[lane]=real(encoded+4+lane*4);
            plane.offset=real(encoded+16);const auto code=ReadU32(encoded+20);planeFeatures[index]=code;
            if(code!=UINT32_MAX) {
                const auto kind=code&15u;const auto featureIndex=code>>4;
                Require(kind<3 && featureIndex<counts[kind],"Collision plane feature code is invalid");
            }
        }
        CollisionGeometryLinks indices{vertexPlanes.data(),edgePlanes.data(),facePlanes.data(),planeFeatures.data()};
        collisionGeometryBindIndexed(&resource->geometry_,resource->vertices_.data(),static_cast<std::int16_t>(counts[0]),resource->edges_.data(),static_cast<std::int16_t>(counts[1]),resource->faces_.data(),static_cast<std::int16_t>(counts[2]),resource->planes_.data(),static_cast<std::int16_t>(counts[3]),&indices);
        for(const auto& edge:resource->edges_) {
            Require(edge.adjacent[0].feature && edge.adjacent[0].feature->kind==0 && edge.adjacent[1].feature && edge.adjacent[1].feature->kind==0,"Collision edge endpoints are invalid");
            Require(edge.adjacent[2].feature && edge.adjacent[2].feature->kind==2 && edge.adjacent[3].feature && edge.adjacent[3].feature->kind==2,"Collision edge face links are invalid");
        }
        for(const auto& vertex:resource->vertices_)
            for(int index=0;index<vertex.adjacentCount;++index)Require(vertex.adjacent[index].feature && vertex.adjacent[index].feature->kind==1,"Collision vertex edge link is invalid");
        for(const auto& face:resource->faces_) {
            Require(face.adjacentCount>=3,"Collision face has fewer than three edges");
            for(int index=0;index<face.adjacentCount;++index) {
                const auto* edge=face.adjacent[index].feature;
                Require(edge && edge->kind==1,"Collision face edge link is invalid");
                Require(collisionNextEdgeInFace(edge->adjacent[0].feature,edge,&face)!=nullptr,"Collision face topology is not closed");
            }
        }
        collisionGeometrySetPlanes(&resource->geometry_);
        return resource;
    }
};

std::vector<std::unique_ptr<CollisionResource>> ReadCollisionResources(const std::vector<std::uint8_t>& data, bool allowEmpty) {
    std::vector<std::unique_ptr<CollisionResource>> resources;
    std::set<std::uint32_t> keys;
    const auto visit=[&](const auto& self,std::size_t offset,std::size_t bytes,unsigned depth)->void {
        Require(depth<=32,"Collision chunk nesting exceeds 32 levels");
        for(const auto& chunk:ReadChunks(data,offset,bytes)) {
            if(chunk.id==kContainer) {
                const auto children=ReadChunks(data,chunk.payloadOffset,chunk.size);
                Require(children.size()%5==0,"Collision container has an incomplete object group");
                for(std::size_t index=0;index<children.size();index+=5) {
                    std::array<Chunk,5> record{};for(std::size_t field=0;field<5;++field)record[field]=children[index+field];
                    auto resource=CollisionResourceReader::Record(data,record);
                    Require(keys.insert(resource->Key()).second,"Collision resource repeats an object key");resources.push_back(std::move(resource));
                }
            } else if(chunk.IsContainer()) self(self,chunk.payloadOffset,chunk.size,depth+1);
        }
    };
    visit(visit,0,data.size(),0);
    Require(allowEmpty || !resources.empty(),"Resource contains no original collision geometry");
    return resources;
}

void CollisionResource::Recenter(const float center[4]) {
    if(!recentered_) collisionGeometryRecenter(&geometry_,center,&recentered_);
}

std::unique_ptr<CollisionResource> LoadCarCollisionResource(const GameResources& resources,const CarDefinition& car) {
    auto objects=ReadCollisionResources(resources.Load(car.geometryCompressedPath), true);
    const auto name=car.baseName+"_CV";const auto key=hashString(name.c_str());
    for(auto& object:objects) if(object->Key()==key) {
        Require(object->Name()==name,"Collision geometry key has an ambiguous name");
        object->Recenter(car.centerOfMass.data());return std::move(object);
    }
    auto resource = std::make_unique<CollisionResource>();
    resource->name_ = name;
    resource->key_ = key;
    resource->box_ = std::make_unique<CollisionBox>();
    const float maximum[3] = {2.1649999618530273f, 0.9710000157356262f, 0.6240000128746033f};
    const float minimum[3] = {-2.2170000076293945f, -0.9710000157356262f, -0.20000000298023224f};
    collisionBoxInitialize(resource->box_.get(), maximum, minimum);
    resource->geometry_ = resource->box_->geometry;
    resource->recentered_ = 1;
    return resource;
}
} // namespace hp2

namespace hp2 {
CollisionResource* FindPipelinePolyhedron(CollisionResource* const* resources,std::size_t count,
    std::uint32_t key,const float* center)
{
    // Complete PAL219078: retain source load order and recenter only the
    // original shared resource, once. A missing key remains a real null.
    for (std::size_t index=0;index<count;++index)if (resources[index]->Key()==key) {
        if (center)resources[index]->Recenter(center);
        return resources[index];
    }
    return nullptr;
}
} // namespace hp2
