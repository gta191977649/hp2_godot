#include "indep/inc/HeliSheet.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>
#include <string.h>

void aerialNavigationPolygonInitialize(AerialNavigationPolygon* polygon, const float vertices[4][4],
                                       int32_t count, uint8_t flags)
{
    memset(polygon,0,sizeof(*polygon));
    polygon->vertexCount=(int8_t)count;polygon->flags=flags;
    polygon->minimumX=polygon->maximumX=vertices[0][0];
    for (int32_t vertex=0;vertex<count;++vertex) {
        for (int lane=0;lane<3;++lane) polygon->vertices[vertex][lane]=vertices[vertex][lane];
        polygon->minimumX=minimumFinite(polygon->minimumX,vertices[vertex][0]);
        polygon->maximumX=maximumFinite(polygon->maximumX,vertices[vertex][0]);
    }
}

float aerialNavigationHeight(const AerialNavigationPolygon* polygon, const float point[2])
{
    const float* first=polygon->vertices[0];
    const float* second=polygon->vertices[1];
    const float* third=polygon->vertices[2];
    if (polygon->vertexCount!=3) {
        const float dy=point[1]-third[1],dx=point[0]-third[0];
        const float edgeY=first[1]-third[1],edgeX=first[0]-third[0];
        const float positive=edgeY*dx,negative=edgeX*dy;
        if (!(positive-negative<=0.0f)) {
            first=polygon->vertices[2];second=polygon->vertices[3];third=polygon->vertices[0];
        }
    }
    float firstEdge[3],secondEdge[3],normal[3];
    for (int lane=0;lane<3;++lane) {
        firstEdge[lane]=second[lane]-first[lane];
        secondEdge[lane]=third[lane]-first[lane];
    }
    for (int lane=0;lane<3;++lane) {
        const int next=(lane+1)%3,last=(lane+2)%3;
        const float positive=firstEdge[next]*secondEdge[last],negative=secondEdge[next]*firstEdge[last];
        normal[lane]=positive-negative;
    }
    const float xx=normal[0]*normal[0],yy=normal[1]*normal[1],zz=normal[2]*normal[2];
    const float inverseLength=finiteRatio(1.0f,sqrtf((xx+yy)+zz));
    for (int lane=0;lane<3;++lane) normal[lane]=normal[lane]*inverseLength;
    const float x=normal[0]*polygon->vertices[0][0],y=normal[1]*polygon->vertices[0][1];
    const float z=normal[2]*polygon->vertices[0][2];
    const float constant=(x+y)+z;
    const float px=normal[0]*point[0],py=normal[1]*point[1];
    return finiteRatio(constant-(px+py),normal[2]);
}


void aerialNavigationBind(AerialNavigation* navigation, AerialNavigationPolygon* polygons, int16_t count)
{
    memset(navigation,0,sizeof(*navigation));
    navigation->polygons=polygons;navigation->count=count;
    aerialNavigationBuildLookup(navigation);
}

int32_t aerialNavigationContains(const AerialNavigationPolygon* polygon, const float point[2])
{
    if (point[0]<polygon->minimumX || polygon->maximumX<point[0]) return 0;
    for (int index=0;index<polygon->vertexCount;++index) {
        const float* first=polygon->vertices[index];
        const float* second=polygon->vertices[(index+1)%polygon->vertexCount];
        const float x=point[0]-first[0],y=point[1]-first[1];
        const float edgeX=second[0]-first[0],edgeY=second[1]-first[1];
        const float positive=edgeY*x,negative=edgeX*y;
        if (0.0f<positive-negative) return 0;
    }
    return 1;
}

static int32_t lookupBin(float value,float minimum,float maximum)
{
    const float numerator=value-minimum,denominator=maximum-minimum;
    const float fraction=finiteRatio(numerator,denominator);
    int32_t index=convertFloatToInt32(fraction*256.0f);
    if (index<0) index=0;
    if (index>255) index=255;
    return index;
}

void aerialNavigationBuildLookup(AerialNavigation* navigation)
{
    navigation->minimumX=100000000.0f;navigation->maximumX=-100000000.0f;navigation->maximumSpan=0;
    for (int index=0;index<navigation->count;++index) {
        const AerialNavigationPolygon* polygon=&navigation->polygons[index];
        navigation->minimumX=minimumFinite(navigation->minimumX,polygon->minimumX);
        navigation->maximumX=maximumFinite(navigation->maximumX,polygon->maximumX);
        navigation->maximumSpan=maximumFinite(navigation->maximumSpan,polygon->maximumX-polygon->minimumX);
    }
    int polygon=0;
    for (int bin=0;bin<256;++bin) {
        while (polygon<navigation->count-1 &&
                lookupBin(navigation->polygons[polygon].minimumX,navigation->minimumX,navigation->maximumX)<bin)
            ++polygon;
        navigation->bins[bin]=(int16_t)polygon;
    }
}

int32_t aerialNavigationQuery(AerialNavigation* navigation, const float point[2], float* height, uint32_t* flags)
{
    if (!navigation->count) return 0;
    int selected=-1,cached=-1;
    for (int index=0;index<3;++index) {
        const int candidate=navigation->recent[index];
        if (candidate>=0 && aerialNavigationContains(&navigation->polygons[candidate],point)) {
            selected=candidate;cached=index;break;
        }
    }
    if (selected<0) {
        const int seed=navigation->bins[lookupBin(point[0],navigation->minimumX,navigation->maximumX)];
        for (int index=seed;index<navigation->count && navigation->polygons[index].minimumX<=point[0];++index)
            if (aerialNavigationContains(&navigation->polygons[index],point)) {selected=index;break;}
        if (selected<0)
            for (int index=seed-1;index>=0 && point[0]-navigation->maximumSpan<=navigation->polygons[index].minimumX;--index)
                if (aerialNavigationContains(&navigation->polygons[index],point)) {selected=index;break;}
        cached=2;
    }
    if (selected<0) return 0;
    const AerialNavigationPolygon* polygon=&navigation->polygons[selected];
    if (height) *height=aerialNavigationHeight(polygon,point);
    if (flags) *flags=polygon->flags;
    for (int index=cached-1;index>=0;--index) navigation->recent[index+1]=navigation->recent[index];
    navigation->recent[0]=(int16_t)selected;
    return 1;
}

// Native typed resource transport owned by the original HeliSheet unit.
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace hp2 {
namespace {
float real(const std::uint8_t* bytes)
{
    const std::uint32_t bits=ReadU32(bytes);
    float value;std::memcpy(&value,&bits,sizeof(value));
    Require(std::isfinite(value),"Airborne corridor contains a non-finite coordinate");
    return value;
}
}

AerialNavigationData::AerialNavigationData(const std::vector<std::uint8_t>& track)
{
    for (const auto& group : ReadChunks(track)) {
        if (group.id!=0x80034800u) continue;
        Chunk header{},records{};
        for (const auto& chunk : ReadChunks(track,group.payloadOffset,group.size)) {
            if (chunk.id==0x34801u) header=chunk;
            if (chunk.id==0x34802u) records=chunk;
        }
        Require(header.id && header.size>=536 && records.id,"Incomplete airborne road corridor resource");
        const std::size_t first=(records.payloadOffset+15u)&~std::size_t(15u);
        Require(first<=records.endOffset && (records.endOffset-first)%64==0,
                "Airborne road corridor has a partial aligned polygon");
        const std::size_t count=(records.endOffset-first)/64;
        Require(count<=static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max()) &&
                    ReadU16(track.data()+header.payloadOffset)==count,"Airborne road corridor count differs");
        polygons_.resize(count);
        for (std::size_t index=0;index<count;++index) {
            const auto* bytes=track.data()+first+64*index;
            auto& polygon=polygons_[index];
            polygon.vertexCount=static_cast<std::int8_t>(bytes[44]);polygon.flags=bytes[45];
            Require(polygon.vertexCount==3 || polygon.vertexCount==4,"Airborne polygon is not a triangle or quad");
            for (int vertex=0;vertex<4;++vertex)
                for (int lane=0;lane<3;++lane) polygon.vertices[vertex][lane]=real(bytes+vertex*16+lane*4);
            polygon.minimumX=real(bytes+12);polygon.maximumX=real(bytes+28);
            Require(polygon.minimumX<=polygon.maximumX &&
                        (!index || polygons_[index-1].minimumX<=polygon.minimumX),
                    "Airborne corridor polygons are not in their authored X order");
        }
        aerialNavigationBind(&navigation_,polygons_.data(),static_cast<std::int16_t>(count));
        return;
    }
    aerialNavigationBind(&navigation_,nullptr,0);
}
}
