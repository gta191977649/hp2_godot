#include "indep/inc/Track.hpp"
#include "bware/inc/bMath.hpp"

#include <float.h>
#include <math.h>
#include <string.h>
#include <array>

// Existing native private operations of this single topology CU.
static inline float terrainSurfaceDot3(const float first[4], const float second[4])
{
    const float x = bScalarMultiply(first[0], second[0]);
    const float y = bScalarMultiply(first[1], second[1]);
    const float z = bScalarMultiply(first[2], second[2]);
    return bScalarAdd(bScalarAdd(x, y), z);
}

static inline void terrainSurfaceCross3(float output[4], const float first[4], const float second[4])
{
    // PAL1516D8 OPMULA,1516DC OPMSUB: the second product's
    // operand order is observable in the source finite multiplier.
    output[0] = bScalarAdd(bScalarMultiply(first[1], second[2]), -bScalarMultiply(second[1], first[2]));
    output[1] = bScalarAdd(bScalarMultiply(first[2], second[0]), -bScalarMultiply(second[2], first[0]));
    output[2] = bScalarAdd(bScalarMultiply(first[0], second[1]), -bScalarMultiply(second[0], first[1]));
    output[3] = 0.0f;
}

static inline void terrainSurfaceDetachPolygon(TerrainTopology *topology, TerrainPolygon *polygon)
{
    if (polygon->previous) polygon->previous->next = polygon->next;
    else topology->oldest = polygon->next;
    if (polygon->next) polygon->next->previous = polygon->previous;
    else topology->newest = polygon->previous;
    polygon->previous = NULL;
    polygon->next = NULL;
}

static inline bool terrainSurfaceEncodedBoundsOverlap(const TerrainEncodedPolygon *polygon,
                                 const float minimum[2], const float maximum[2])
{
    float minX = (float)((int32_t)polygon->x[polygon->extrema & 3] * 8192) * 0.0000152587890625f;
    float minY = (float)((int32_t)polygon->y[(polygon->extrema >> 2) & 3] * 8192) * 0.0000152587890625f;
    float maxX = (float)((int32_t)polygon->x[(polygon->extrema >> 4) & 3] * 8192) * 0.0000152587890625f;
    float maxY = (float)((int32_t)polygon->y[polygon->extrema >> 6] * 8192) * 0.0000152587890625f;
    return minX <= maximum[0] && minY <= maximum[1]
        && minimum[0] <= maxX && minimum[1] <= maxY;
}

static inline float terrainSurfacePolygonElevation(const TerrainPolygon *polygon, const float point[4])
{
    const float x = bScalarMultiply(polygon->plane[0], point[0]);
    const float y = bScalarMultiply(polygon->plane[1], point[1]);
    const float numerator = bScalarAdd(polygon->plane[3], -bScalarAdd(x, y));
    return bScalarDivide(numerator, polygon->plane[2]);
}


bool terrainPolygonContains(const TerrainPolygon *polygon, const float point[4],
                            float *planeDistance)
{
    float vectors[4][4];
    for (int i = 0; i < polygon->vertexCount; ++i) {
        for (size_t axis = 0; axis < 4; ++axis)
            vectors[i][axis] = polygon->vertices[i][axis] - point[axis];
    }
    for (int i = 0; i < polygon->vertexCount; ++i) {
        float edge[4];
        terrainSurfaceCross3(edge, vectors[i], vectors[(i + 1) % polygon->vertexCount]);
        if (terrainSurfaceDot3(edge, polygon->plane) < 0.0f) return false;
    }
    if (planeDistance) *planeDistance = -terrainSurfaceDot3(vectors[0], polygon->plane);
    return true;
}

bool terrainPolygonIntersectSegment(const TerrainPolygon *polygon,
                                    const float first[4], const float second[4],
                                    float intersection[4])
{
    float firstDelta[4], secondDelta[4], point[4];
    for (size_t i = 0; i < 4; ++i) {
        firstDelta[i] = first[i] - polygon->vertices[0][i];
        secondDelta[i] = second[i] - polygon->vertices[0][i];
    }
    float firstDistance = terrainSurfaceDot3(firstDelta, polygon->plane);
    float secondDistance = terrainSurfaceDot3(secondDelta, polygon->plane);
    if (!(firstDistance * secondDistance < 0.0f)) return false;
    float fraction = fabsf(firstDistance) / (fabsf(firstDistance) + fabsf(secondDistance));
    for (size_t i = 0; i < 4; ++i) point[i] = (second[i] - first[i]) * fraction + first[i];
    if (!terrainPolygonContains(polygon, point, NULL)) return false;
    if (intersection) memcpy(intersection, point, sizeof(point));
    return true;
}

bool terrainPolygonTestSegment(const TerrainPolygon *polygon, float *closestDistance,
                               const float first[4], const float second[4],
                               const float minimum[4], const float maximum[4],
                               bool twoSided)
{
    float polygonMinimum[4], polygonMaximum[4], intersection[4], delta[4];
    vectorBoundsFromPoint3(polygonMinimum, polygonMaximum, polygon->vertices[0]);
    for (int i = 1; i < polygon->vertexCount; ++i)
        vectorBoundsExpandPoint3(polygonMinimum, polygonMaximum, polygon->vertices[i]);
    if (!vectorBoundsOverlaps3(polygonMinimum, polygonMaximum, minimum, maximum)) return false;
    if (!terrainPolygonIntersectSegment(polygon, first, second, intersection)) return false;
    for (size_t i = 0; i < 4; ++i) delta[i] = intersection[i] - first[i];
    if (!twoSided && !(terrainSurfaceDot3(delta, polygon->plane) < 0.0f)) return false;
    float distance = sqrtf(terrainSurfaceDot3(delta, delta));
    if (!(distance < *closestDistance)) return false;
    *closestDistance = distance;
    return true;
}

TerrainTopology *terrainTopologyInitialize(TerrainTopology *topology)
{
    memset(topology, 0, sizeof(*topology));
    return topology;
}

bool terrainTopologyUnloadChunk(TerrainTopology *topology, TerrainChunkKind kind)
{
    switch (kind) {
    case TERRAIN_CHUNK_BRANCHES: topology->branches = NULL; topology->branchCount = 0; return true;
    case TERRAIN_CHUNK_SQUARES: topology->squares = NULL; topology->squareCount = 0; return true;
    case TERRAIN_CHUNK_POLYGONS: topology->polygons = NULL; topology->polygonCount = 0; return true;
    case TERRAIN_CHUNK_REFERENCES: topology->references = NULL; topology->referenceCount = 0; return true;
    default: return false;
    }
}

float terrainTopologySquareCorner(float coordinate)
{
    int32_t fixed = (int32_t)(coordinate * 65536.0f);
    fixed = (int32_t)((uint32_t)fixed & UINT32_C(0xFFE00000));
    return (float)fixed * 0.0000152587890625f;
}

const TerrainSquare *terrainTopologyFindSquare(const TerrainTopology *topology,
                                              float x, float y, bool nearest)
{
    if (!topology->branches || !topology->squares) return NULL;
    size_t node = 0;
    float cornerX = -16384.0f, cornerY = -16384.0f;
    for (int level = 10; level >= 1; --level) {
        if (node >= topology->branchCount) return NULL;
        float halfSize = ((float)(1U << level) * 32.0f) * 0.5f;
        float middleX = cornerX + halfSize;
        float middleY = cornerY + halfSize;
        unsigned quadrant = (middleX <= x ? 1U : 0U) + (middleY <= y ? 2U : 0U);
        if (quadrant & 1) cornerX = middleX;
        if (quadrant & 2) cornerY = middleY;
        size_t next = topology->branches[node].children[quadrant];
        if (!next) {
            if (!nearest) return NULL;
            for (size_t i = 0; i < 4; ++i) {
                if (topology->branches[node].children[i]) {
                    next = topology->branches[node].children[i];
                    break;
                }
            }
            if (!next) return NULL;
        }
        if (level == 1) return next < topology->squareCount ? &topology->squares[next] : NULL;
        node = next;
    }
    return NULL;
}

size_t terrainTopologyCollectSquare(TerrainTopology *topology,
                                    TerrainEncodedPolygon **output, size_t capacity,
                                    const TerrainSquare *square,
                                    const float minimum[2], const float maximum[2])
{
    if (!square) return 0;
    size_t count = 0;
    for (int i = 0; i < square->referenceCount; ++i) {
        size_t reference = (size_t)square->firstReference + (size_t)i;
        if (reference >= topology->referenceCount) break;
        size_t index = topology->references[reference];
        if (index >= topology->polygonCount) continue;
        TerrainEncodedPolygon *polygon = &topology->polygons[index];
        if (polygon->visited || !terrainSurfaceEncodedBoundsOverlap(polygon, minimum, maximum) || count >= capacity) continue;
        output[count++] = polygon;
        polygon->visited = 1;
    }
    return count;
}

void terrainTopologyRelease(TerrainTopology *topology, TerrainPolygon *polygon)
{
    (void)topology;
    polygon->references = (uint8_t)(polygon->references - 1U);
}

void terrainTopologyReleaseMany(TerrainTopology *topology,
                                TerrainPolygon *const *polygons, size_t count)
{
    for (size_t i = 0; i < count; ++i) terrainTopologyRelease(topology, polygons[i]);
}

#include "Track.hpp"
#include "bware/inc/bWare.hpp"

const TrackScenarioDefinition* trackScenarioFind(const TrackScenarioCatalog* catalog, int32_t oneBasedIndex)
{
    const TrackScenarioDefinition* scenario;
    if (oneBasedIndex <= 0 || (size_t)oneBasedIndex > catalog->scenarioCount) return NULL;
    scenario = &catalog->scenarios[oneBasedIndex - 1];
    return scenario->name[0] ? scenario : NULL;
}

const TrackLandmark* trackLandmarkFind(const TrackScenarioCatalog* catalog, uint32_t key, int32_t occurrence)
{
    int32_t current = 0;
    for (size_t index = 0; index < catalog->landmarkCount; ++index) {
        const TrackLandmark* landmark = &catalog->landmarks[index];
        if (landmark->key != key) continue;
        if (current++ == occurrence) return landmark;
    }
    return NULL;
}

const TrackLandmark* trackLandmarkFindName(const TrackScenarioCatalog* catalog, const char* name, int32_t occurrence)
{
    return trackLandmarkFind(catalog, hashString(name), occurrence);
}

// PAL1513C8, GetNumTrackPositionMarkers. The flat native catalogue retains
// the source list order and counts the same key across every authored marker.
int32_t trackLandmarkCount(const TrackScenarioCatalog* catalog,uint32_t key)
{
    uint32_t count=0;
    for (size_t index=0;index<catalog->landmarkCount;++index)
        if (catalog->landmarks[index].key==key) ++count;
    int32_t result;
    memcpy(&result,&count,sizeof(result));
    return result;
}

#include "Track.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"

#include <cmath>
#include <cstring>
#include <limits>

namespace hp2 {
namespace {
float coordinate(const std::uint8_t* bytes)
{
    const auto bits = ReadU32(bytes);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    Require(std::isfinite(value), "Track landmark has a non-finite coordinate");
    return value;
}
}

TrackScenarioData::TrackScenarioData(const std::vector<std::uint8_t>& track)
    : authored_(track)
{
    bool scenariosFound = false;
    const auto visit = [&](const auto& self, std::size_t start, std::size_t length, unsigned depth) -> void {
        Require(depth <= 32, "Track scenario hierarchy is too deep");
        for (const auto& chunk : ReadChunks(track, start, length)) {
            if (chunk.id == 0x34200u) {
                Require(!hasCurrentInfo_ && chunk.size>=0xa4u,"Track repeats or truncates its current34200 header");
                const auto* bytes=track.data()+chunk.payloadOffset;
                std::memcpy(currentInfo_.name,bytes,sizeof(currentInfo_.name));
                std::memcpy(&currentInfo_.number,bytes+0x41,1);
                std::memcpy(currentInfo_.tireBankName,bytes+0x84,sizeof(currentInfo_.tireBankName));
                currentInfo_.mapOriginX=coordinate(bytes+0x5c);
                currentInfo_.mapOriginY=coordinate(bytes+0x60);
                currentInfo_.mapScale=coordinate(bytes+0x64);
                currentInfo_.mapRotation=ReadU16(bytes+0x68);
                currentInfo_.startLineRotation=ReadU16(bytes+0x6a);
                currentInfo_.finishLineRotation=ReadU16(bytes+0x6c);
                Require(std::memchr(currentInfo_.name,0,sizeof(currentInfo_.name)) &&
                    std::memchr(currentInfo_.tireBankName,0,sizeof(currentInfo_.tireBankName)),
                    "Current track name or tire bank is not terminated in the actual header");
                hasCurrentInfo_=true;
            } else if (chunk.id == 0x34201u) {
                Require(!scenariosFound, "Track repeats its scenario catalog");
                scenariosFound = true;
                Require(chunk.size % 164u == 0, "Track scenario catalog has a partial record");
                scenarios_.resize(chunk.size / 164u);
                Require(scenarios_.size() <= static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()),
                        "Track scenario catalog exceeds its original index range");
                for (std::size_t index = 0; index < scenarios_.size(); ++index) {
                    const auto* bytes = track.data() + chunk.payloadOffset + index * 164u;
                    auto& record = scenarios_[index];
                    std::memcpy(record.name, bytes, sizeof(record.name));
                    Require(std::memchr(record.name, 0, sizeof(record.name)) != nullptr,
                            "Track scenario display name is not terminated");
                    std::memcpy(&record.courseAvailable, bytes + 64, 1);
                    const auto courseIndex = ReadU16(bytes + 70);
                    std::memcpy(&record.courseIndex, &courseIndex, sizeof(courseIndex));
                    std::memcpy(&record.number,bytes+65,1);
                    std::memcpy(&record.baseNumber,bytes+66,1);
                    std::memcpy(&record.routeKind,bytes+67,1);
                    const auto timeOfDay=ReadU16(bytes+68);
                    std::memcpy(&record.timeOfDay,&timeOfDay,sizeof(timeOfDay));
                    for (std::size_t direction=0;direction<2;++direction) {
                        const auto id=ReadU32(bytes+0x48+direction*4);
                        std::memcpy(&record.nameLanguageIds[direction],&id,sizeof(id));
                    }
                    record.policeCharacterKey=ReadU32(bytes+0x54);
                    const auto distanceBits=ReadU32(bytes+0x50);
                    std::memcpy(&record.lengthMiles,&distanceBits,sizeof(distanceBits));
                    for (std::size_t axis=0;axis<2;++axis) {
                        const auto elevation=ReadU32(bytes+0x70+axis*4);
                        std::memcpy(record.elevationFeet+axis,&elevation,sizeof(elevation));
                    }
                    const auto difficulty=ReadU32(bytes+0x78);
                    std::memcpy(&record.difficulty,&difficulty,sizeof(difficulty));
                    const auto racerRouteIndex=ReadU32(bytes+0x7c);
                    std::memcpy(&record.racerRouteIndex,&racerRouteIndex,sizeof(racerRouteIndex));
                    const auto activation = ReadU32(bytes + 128);
                    std::memcpy(&record.connectionActivationSeconds, &activation, sizeof(activation));
                }
                // The original loader explicitly clears populated scenario 54.
                if (scenarios_.size() >= 54 && scenarios_[53].name[0]) scenarios_[53] = {};
            } else if (chunk.id == 0x34146u) {
                Require(chunk.size % 24u == 0, "Track landmark catalog has a partial record");
                for (std::size_t offset = 0; offset < chunk.size; offset += 24) {
                    const auto* bytes = track.data() + chunk.payloadOffset + offset;
                    landmarks_.push_back({ReadU32(bytes + 8), ReadU32(bytes + 12),
                                          {coordinate(bytes + 16), coordinate(bytes + 20)}});
                }
            } else if (chunk.IsContainer()) self(self, chunk.payloadOffset, chunk.size, depth + 1);
        }
    };
    visit(visit, 0, track.size(), 0);
    catalog_ = {scenarios_.data(), scenarios_.size(), landmarks_.data(), landmarks_.size()};
}

}

/* Camera path support. */
int32_t carCameraTerrainOccludes(TerrainSurface* surface,const float first[4],const float second[4])
{
    const float x=first[0]-second[0],y=first[1]-second[1],z=first[2]-second[2];
    const float xx=x*x,yy=y*y,zz=z*z;
    const float length=sqrtf((xx+yy)+zz);
    if(length<30.0f){
        float middle[4];
        for(int axis=0;axis<4;++axis){const float sum=first[axis]+second[axis];middle[axis]=sum*0.5f;}
        terrainSurfaceUpdate(surface,middle,length*0.5f,NULL);
        return terrainSurfaceIntersectsSegment(surface,first,second,0.5f);
    }
    const float scaled=length*0.03333333507180214f;
    int32_t samples=convertFloatToInt32(scaled);
    if((float)samples<scaled)++samples;
    const float divisor=(float)samples;
    const float stepLength=finiteRatio(length,divisor);
    float delta[4],previous[4];
    for(int axis=0;axis<4;++axis){delta[axis]=first[axis]-second[axis];previous[axis]=second[axis];}
    for(int32_t index=1;index<=samples;++index){
        const float fraction=finiteRatio((float)index,divisor);
        float current[4];
        for(int axis=0;axis<4;++axis){const float offset=delta[axis]*fraction;current[axis]=second[axis]+offset;}
        float middle[4];
        for(int axis=0;axis<4;++axis){const float sum=current[axis]+previous[axis];middle[axis]=sum*0.5f;}
        terrainSurfaceUpdate(surface,middle,stepLength*0.5f,NULL);
        if(terrainSurfaceIntersectsSegment(surface,current,previous,0.5f))return 1;
        memcpy(previous,current,sizeof(previous));
    }
    return 0;
}

// Remaining TrackPolygon/TopologyCoordinate/TopologyTree bodies;
TerrainPolygon *terrainPolygonDecode(TerrainTopology *topology,
                                    TerrainPolygon *polygon,
                                    TerrainEncodedPolygon *encoded)
{
    polygon->materialIndex = encoded->materialIndex;
    polygon->flags = encoded->flags;
    polygon->references = 0;
    polygon->minimum[0] = (float)((int32_t)encoded->x[encoded->extrema & 3] * 8192) * 0.0000152587890625f;
    polygon->minimum[1] = (float)((int32_t)encoded->y[(encoded->extrema >> 2) & 3] * 8192) * 0.0000152587890625f;
    polygon->maximum[0] = (float)((int32_t)encoded->x[(encoded->extrema >> 4) & 3] * 8192) * 0.0000152587890625f;
    polygon->maximum[1] = (float)((int32_t)encoded->y[encoded->extrema >> 6] * 8192) * 0.0000152587890625f;
    for (size_t i = 0; i < 4; ++i) {
        uint32_t heightBits = (uint32_t)(int32_t)encoded->z[i] * 256U
                            + (uint32_t)(int32_t)encoded->elevationBand * 65536U;
        int32_t height;
        memcpy(&height, &heightBits, sizeof(height));
        polygon->vertices[i][0] = (float)((int32_t)encoded->x[i] * 8192) * 0.0000152587890625f;
        polygon->vertices[i][1] = (float)((int32_t)encoded->y[i] * 8192) * 0.0000152587890625f;
        polygon->vertices[i][2] = (float)height * 0.0000152587890625f;
        polygon->vertices[i][3] = 0.0f;
    }
    polygon->vertexCount = (encoded->flags & 16) ? 4 : 3;
    if (encoded->flags & 4) {
        for (size_t i = 0; i < 4; ++i) polygon->vertices[i][2] *= 4.0f;
    }
    float first[4], second[4], normal[4];
    for (size_t i = 0; i < 4; ++i) {
        first[i] = bScalarAdd(polygon->vertices[1][i], -polygon->vertices[0][i]);
        second[i] = bScalarAdd(polygon->vertices[2][i], -polygon->vertices[0][i]);
    }
    terrainSurfaceCross3(normal, first, second);
    const float inverseLength = bScalarDivide(1.0f, bScalarSquareRoot(terrainSurfaceDot3(normal, normal)));
    for (size_t i = 0; i < 3; ++i) polygon->plane[i] = bScalarMultiply(normal[i], inverseLength);
    polygon->plane[3] = terrainSurfaceDot3(polygon->vertices[0], polygon->plane);
    polygon->encoded = encoded;
    encoded->cacheIndex = (int16_t)(polygon - topology->cache);
    polygon->occupied = true;
    return polygon;
}

void terrainPolygonRelease(TerrainTopology *topology, TerrainPolygon *polygon)
{
    (void)topology;
    if (polygon->encoded) polygon->encoded->cacheIndex = -1;
    polygon->occupied = false;
    polygon->encoded = NULL;
}

void terrainTopologyInitializeCache(TerrainTopology *topology,
                                    TerrainPolygon *storage, size_t capacity)
{
    topology->cache = storage;
    topology->cacheCapacity = capacity;
    topology->oldest = NULL;
    topology->newest = NULL;
    memset(storage, 0, capacity * sizeof(*storage));
}

void terrainTopologyDispose(TerrainTopology *topology)
{
    TerrainPolygon *polygon = topology->oldest;
    while (polygon) {
        TerrainPolygon *next = polygon->next;
        terrainPolygonRelease(topology, polygon);
        polygon = next;
    }
    topology->cache = NULL;
    topology->cacheCapacity = 0;
    topology->oldest = NULL;
    topology->newest = NULL;
}

bool terrainTopologyLoadChunk(TerrainTopology *topology, const TerrainTopologyChunk *chunk)
{
    switch (chunk->kind) {
    case TERRAIN_CHUNK_BRANCHES:
        topology->branches = (const TerrainBranch *)chunk->records;
        topology->branchCount = chunk->count;
        return true;
    case TERRAIN_CHUNK_SQUARES:
        topology->squares = (const TerrainSquare *)chunk->records;
        topology->squareCount = chunk->count;
        return true;
    case TERRAIN_CHUNK_POLYGONS:
        topology->polygons = (TerrainEncodedPolygon *)chunk->records;
        topology->polygonCount = chunk->count;
        for (size_t i = 0; i < chunk->count; ++i) topology->polygons[i].cacheIndex = -1;
        return true;
    case TERRAIN_CHUNK_REFERENCES:
        topology->references = (const uint16_t *)chunk->records;
        topology->referenceCount = chunk->count;
        return true;
    default:
        return false;
    }
}

size_t terrainTopologyPurgeUnused(TerrainTopology *topology)
{
    size_t released = 0;
    TerrainPolygon *polygon = topology->oldest;
    while (polygon && released < 101) {
        TerrainPolygon *next = polygon->next;
        if (!polygon->references) {
            terrainSurfaceDetachPolygon(topology, polygon);
            terrainPolygonRelease(topology, polygon);
            ++released;
        }
        polygon = next;
    }
    return released;
}

TerrainPolygon *terrainTopologyAcquire(TerrainTopology *topology,
                                      TerrainEncodedPolygon *encoded)
{
    TerrainPolygon *polygon = NULL;
    if (encoded->cacheIndex >= 0 && (size_t)encoded->cacheIndex < topology->cacheCapacity) {
        polygon = &topology->cache[encoded->cacheIndex];
    } else {
        for (int attempt = 0; attempt < 2 && !polygon; ++attempt) {
            for (size_t i = 0; i < topology->cacheCapacity; ++i) {
                if (!topology->cache[i].occupied) { polygon = &topology->cache[i]; break; }
            }
            if (!polygon && attempt == 0) terrainTopologyPurgeUnused(topology);
        }
        if (!polygon) return NULL;
        terrainPolygonDecode(topology, polygon, encoded);
        polygon->previous = topology->newest;
        polygon->next = NULL;
        if (topology->newest) topology->newest->next = polygon;
        else topology->oldest = polygon;
        topology->newest = polygon;
    }
    polygon->references = (uint8_t)(polygon->references + 1U);
    return polygon;
}

size_t terrainTopologyCollectBounds(TerrainTopology *topology,
                                    TerrainPolygon **output, size_t capacity,
                                    const float minimum[2], const float maximum[2])
{
    TerrainEncodedPolygon *records[TERRAIN_SURFACE_POLYGON_LIMIT];
    size_t count = 0;
    if (capacity > TERRAIN_SURFACE_POLYGON_LIMIT) capacity = TERRAIN_SURFACE_POLYGON_LIMIT;
    if (-32768.0f < minimum[0] && maximum[0] < 32767.0f
            && -32768.0f < minimum[1] && maximum[1] < 32767.0f) {
        for (float y = terrainTopologySquareCorner(minimum[1]); y <= maximum[1]; y += 32.0f) {
            for (float x = terrainTopologySquareCorner(minimum[0]); x <= maximum[0]; x += 32.0f) {
                const TerrainSquare *square = terrainTopologyFindSquare(topology, x, y, false);
                count += terrainTopologyCollectSquare(topology, records + count, capacity - count, square, minimum, maximum);
            }
        }
    }
    size_t acquired = 0;
    for (size_t i = 0; i < count; ++i) {
        records[i]->visited = 0;
        TerrainPolygon *polygon = terrainTopologyAcquire(topology, records[i]);
        if (polygon) output[acquired++] = polygon;
    }
    return acquired;
}

TerrainSurface *terrainSurfaceInitialize(TerrainSurface *surface, TerrainTopology *topology)
{
    memset(surface, 0, sizeof(*surface));
    surface->topology = topology;
    terrainSurfaceReset(surface);
    return surface;
}

void terrainSurfaceDispose(TerrainSurface *surface)
{
    terrainTopologyReleaseMany(surface->topology, surface->polygons, surface->polygonCount);
    surface->polygonCount = 0;
    surface->lastGround = NULL;
}

void terrainSurfaceReset(TerrainSurface *surface)
{
    vectorBoundsInitialize2(surface->queryMinimum, surface->queryMaximum);
    vectorBoundsInitialize2(surface->cacheMinimum, surface->cacheMaximum);
    terrainTopologyReleaseMany(surface->topology, surface->polygons, surface->polygonCount);
    surface->lastGround = NULL;
    surface->polygonCount = 0;
    surface->groundCount = 0;
    surface->barrierCount = 0;
    surface->overheadCount = 0;
}

void terrainSurfaceUpdate(TerrainSurface *surface, const float point[2],
                          float radius, const float direction[2])
{
    vectorBoundsInitialize2(surface->queryMinimum, surface->queryMaximum);
    vectorBoundsExpandRadius2(surface->queryMinimum, surface->queryMaximum, point, radius);
    if (vectorBoundsContainsBounds2(surface->cacheMinimum, surface->cacheMaximum,
                                    surface->queryMinimum, surface->queryMaximum)) return;
    terrainTopologyReleaseMany(surface->topology, surface->polygons, surface->polygonCount);
    surface->polygonCount = 0;
    surface->groundCount = 0;
    surface->barrierCount = 0;
    surface->overheadCount = 0;
    surface->lastGround = NULL;
    float padding = radius + 0.5f;
    if (direction) {
        float lookAhead = 0.15000000596046448f;
        for (int attempt = 0; attempt < 5; ++attempt) {
            float target[2];
            vectorBoundsInitialize2(surface->cacheMinimum, surface->cacheMaximum);
            vectorBoundsExpandRadius2(surface->cacheMinimum, surface->cacheMaximum, point, padding);
            vectorMultiplyAdd2(target, point, direction, lookAhead);
            vectorBoundsExpandRadius2(surface->cacheMinimum, surface->cacheMaximum, target, padding);
            float width = surface->cacheMinimum[0] - surface->cacheMaximum[0];
            float height = surface->cacheMinimum[1] - surface->cacheMaximum[1];
            if (sqrtf(width * width + height * height) < 28.799999237060547f) break;
            lookAhead *= 0.75f;
            if (attempt == 3) lookAhead = 0.0f;
        }
    } else {
        vectorBoundsInitialize2(surface->cacheMinimum, surface->cacheMaximum);
        vectorBoundsExpandRadius2(surface->cacheMinimum, surface->cacheMaximum, point, padding);
    }
    TerrainPolygon *polygons[TERRAIN_SURFACE_POLYGON_LIMIT];
    size_t count = terrainTopologyCollectBounds(surface->topology, polygons,
                                                TERRAIN_SURFACE_POLYGON_LIMIT,
                                                surface->cacheMinimum, surface->cacheMaximum);
    size_t written = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!(polygons[i]->flags & 10)) surface->polygons[written++] = polygons[i];
    }
    surface->groundCount = written;
    for (size_t i = 0; i < count; ++i) {
        if (polygons[i]->flags & 2) surface->polygons[written++] = polygons[i];
    }
    surface->barrierCount = written - surface->groundCount;
    for (size_t i = 0; i < count; ++i) {
        if (polygons[i]->flags & 8) surface->polygons[written++] = polygons[i];
    }
    surface->overheadCount = written - surface->groundCount - surface->barrierCount;
    surface->polygonCount = count;
}

static float maximumEdgeSide(const TerrainPolygon *polygon, const float point[4])
{
    static const unsigned edges[4][2] = {{0, 1}, {1, 2}, {3, 0}, {2, 3}};
    float side = -1.0f;
    for (int i = 0; i < polygon->vertexCount; ++i) {
        const float *first = polygon->vertices[edges[i][0]];
        const float *second = polygon->vertices[edges[i][1]];
        const float firstProduct = bScalarMultiply(bScalarAdd(second[1], -first[1]), bScalarAdd(point[0], -first[0]));
        const float secondProduct = bScalarMultiply(bScalarAdd(second[0], -first[0]), bScalarAdd(point[1], -first[1]));
        const float cross = bScalarAdd(firstProduct, -secondProduct);
        if (side < cross) side = cross;
        if (0.0f < side) break;
    }
    return side;
}

float terrainSurfaceElevation(TerrainSurface *surface, const float point[4],
                              int32_t *materialIndex, float normal[4], bool *found)
{
    if (found) *found = true;
    if (surface->lastGround && maximumEdgeSide(surface->lastGround, point) < 0.0f) {
        TerrainPolygon *polygon = surface->lastGround;
        float height = terrainSurfacePolygonElevation(polygon, point);
        if (materialIndex) *materialIndex = polygon->materialIndex;
        if (normal) memcpy(normal, polygon->plane, sizeof(polygon->plane));
        return height;
    }
    if (!vectorBoundsContainsPoint2(surface->queryMinimum, surface->queryMaximum, point, 0.0f))
        terrainSurfaceUpdate(surface, point, 8.0f, NULL);
    float height = -FLT_MAX;
    float limit = bScalarAdd(point[2], 1.0f);
    int selected = -1;
    for (int pass = 0; pass < 2 && selected < 0; ++pass) {
        for (size_t i = 0; i < surface->groundCount; ++i) {
            TerrainPolygon *polygon = surface->polygons[i];
            if (polygon->minimum[0] > point[0] || point[0] > polygon->maximum[0]
                    || polygon->minimum[1] > point[1] || point[1] > polygon->maximum[1]) continue;
            float side = maximumEdgeSide(polygon, point);
            if (!(side <= 0.0f)) continue;
            float candidate = terrainSurfacePolygonElevation(polygon, point);
            if (!(height < candidate && candidate < limit)) continue;
            height = candidate;
            if (!(polygon->flags & 1) && side < 0.0f) {
                surface->lastGround = polygon;
                selected = (int)i;
                break;
            }
            selected = (int)i;
        }
        limit = FLT_MAX;
    }
    if (selected < 0) {
        if (found) *found = false;
        if (materialIndex) *materialIndex = 1;
        if (normal) { normal[0] = 0.0f; normal[1] = 0.0f; normal[2] = -1.0f; }
        return point[2];
    }
    TerrainPolygon *polygon = surface->polygons[selected];
    if (selected > 4) {
        memmove(surface->polygons + 1, surface->polygons,
                (size_t)selected * sizeof(surface->polygons[0]));
        surface->polygons[0] = polygon;
    }
    if (materialIndex) *materialIndex = polygon->materialIndex;
    if (normal) memcpy(normal, polygon->plane, sizeof(polygon->plane));
    return height;
}

bool terrainSurfaceHasGround(TerrainSurface *surface, const float point[2])
{
    float sample[4] = {point[0], point[1], 99999.1015625f, 0.0f};
    bool found;
    (void)terrainSurfaceElevation(surface, sample, NULL, NULL, &found);
    return found;
}

bool terrainSurfaceIntersectsSegment(const TerrainSurface *surface,
                                     const float first[4], const float second[4], float radius)
{
    float minimum[4], maximum[4];
    vectorBoundsFromPoint3(minimum, maximum, first);
    terrainBoundsExpandRadius3(minimum, maximum, second, radius);
    float closestDistance = 999999.0f;
    for (size_t i = 0; i < surface->polygonCount; ++i) {
        if (terrainPolygonTestSegment(surface->polygons[i], &closestDistance,
                                       first, second, minimum, maximum, false)) return true;
    }
    return false;
}

bool terrainSurfaceConstrainSegment(const TerrainSurface *surface, int mode,
                                    bool twoSided, const float first[4],
                                    float second[4], float radius, float extraHeight)
{
    float minimum[4], maximum[4];
    vectorBoundsFromPoint3(minimum, maximum, first);
    terrainBoundsExpandRadius3(minimum, maximum, second, radius);
    float closestDistance = 999999.0f;
    const TerrainPolygon *hit = NULL;
    for (size_t i = 0; i < surface->polygonCount; ++i) {
        if (terrainPolygonTestSegment(surface->polygons[i], &closestDistance,
                                       first, second, minimum, maximum, twoSided)) hit = surface->polygons[i];
    }
    float delta[4];
    for (size_t i = 0; i < 4; ++i) delta[i] = second[i] - first[i];
    float length = sqrtf(terrainSurfaceDot3(delta, delta));
    if (!(closestDistance < length)) return false;
    if (mode == 1 || (mode == 0 && (hit->flags & 2))) {
        float distance = closestDistance - radius;
        float fraction = distance / length;
        vectorSetLength3(delta, delta, distance);
        if (mode == 0) delta[2] += extraHeight * (1.0f - fraction);
        for (size_t i = 0; i < 4; ++i) second[i] = first[i] + delta[i];
        return true;
    }
    if (mode != 0) return true;
    float elevation = terrainSurfacePolygonElevation(hit, second);
    second[2] = (hit->flags & 8) ? elevation - radius : elevation + radius;
    return true;
}

float terrainMaterialNoise(int32_t x, int32_t y)
{
    static const float samples[16] = {
        -0.5f, -0.2f, 0.5f, 0.2f, -0.4f, -0.1f, 0.2f, 0.5f,
        -0.2f, -0.5f, 0.4f, -0.3f, 0.3f, 0.0f, -0.2f, 0.1f
    };
    uint32_t first = (uint32_t)x * (uint32_t)y;
    uint32_t second = ((uint32_t)x + 1U) * ((uint32_t)y + 1U);
    return samples[(first * 11U + second * 13U) & 15U];
}

float terrainMaterialDisplacement(const TerrainMaterial *material, const float point[4])
{
    const float x = bScalarMultiply(point[0], material->displacementFrequency);
    const float y = bScalarMultiply(point[1], material->displacementFrequency);
    float wholeX = (float)(int32_t)x;
    float wholeY = (float)(int32_t)y;
    if (x < wholeX) wholeX = bScalarAdd(wholeX, -1.0f);
    if (y < wholeY) wholeY = bScalarAdd(wholeY, -1.0f);
    int32_t cellX = (int32_t)wholeX, cellY = (int32_t)wholeY;
    const float partX = bScalarAdd(x, -wholeX), partY = bScalarAdd(y, -wholeY);
    const float remainingY = bScalarAdd(1.0f, -partY);
    const float remainingX = bScalarAdd(1.0f, -partX);
    const float first = bScalarMultiply(bScalarMultiply(remainingX, remainingY), terrainMaterialNoise(cellX, cellY));
    const float second = bScalarMultiply(bScalarMultiply(partX, remainingY), terrainMaterialNoise(cellX + 1, cellY));
    float sum = bScalarAdd(first, second);
    const float third = bScalarMultiply(bScalarMultiply(remainingX, partY), terrainMaterialNoise(cellX, cellY + 1));
    sum = bScalarAdd(sum, third);
    const float fourth = bScalarMultiply(bScalarMultiply(partX, partY), terrainMaterialNoise(cellX + 1, cellY + 1));
    sum = bScalarAdd(sum, fourth);
    return bScalarMultiply(sum, material->displacementAmplitude);
}

// Native decoded-resource/cache lifetime of the same identified topology owner.
namespace hp2 {
namespace {
constexpr std::uint32_t kBranches = 0x00034130;
constexpr std::uint32_t kSquares = 0x00034131;
constexpr std::uint32_t kPolygons = 0x00034132;
constexpr std::uint32_t kReferences = 0x00034133;

void FindTopology(const std::vector<std::uint8_t>& bytes,
                  std::size_t begin, std::size_t length, unsigned depth,
                  std::array<Chunk, 4>& found, std::array<bool, 4>& present) {
    Require(depth < 32, "Terrain chunk nesting exceeds limit");
    for (const auto& chunk : ReadChunks(bytes, begin, length)) {
        if (chunk.id >= kBranches && chunk.id <= kReferences) {
            const auto index = chunk.id - kBranches;
            Require(!present[index], "Duplicate topology chunk");
            found[index] = chunk;
            present[index] = true;
        } else if (chunk.IsContainer()) {
            FindTopology(bytes, chunk.payloadOffset, chunk.size, depth + 1, found, present);
        }
    }
}

std::size_t AlignedStart(const Chunk& chunk, std::size_t recordSize) {
    const auto begin = (chunk.payloadOffset + 15) & ~std::size_t(15);
    Require(begin <= chunk.endOffset && (chunk.endOffset - begin) % recordSize == 0,
            "Malformed aligned terrain records");
    return begin;
}
} // namespace

TerrainData::TerrainData(const std::vector<std::uint8_t>& bytes, std::size_t cacheCapacity)
    : cache_(cacheCapacity) {
    Require(cacheCapacity > 0 && cacheCapacity <= 32767, "Terrain cache capacity invalid");
    std::array<Chunk, 4> chunks{};
    std::array<bool, 4> present{};
    FindTopology(bytes, 0, bytes.size(), 0, chunks, present);
    for (bool exists : present) Require(exists, "Track bundle is missing collision topology");
    for (auto cursor = AlignedStart(chunks[0], 8); cursor < chunks[0].endOffset; cursor += 8) {
        TerrainBranch branch{};
        for (unsigned i = 0; i < 4; ++i) branch.children[i] = ReadU16(bytes.data() + cursor + i * 2);
        branches_.push_back(branch);
    }
    for (auto cursor = AlignedStart(chunks[1], 12); cursor < chunks[1].endOffset; cursor += 12) {
        TerrainSquare square{};
        square.firstReference = ReadU16(bytes.data() + cursor + 8);
        square.referenceCount = static_cast<std::int16_t>(ReadU16(bytes.data() + cursor + 10));
        Require(square.referenceCount >= 0, "Negative terrain square reference count");
        squares_.push_back(square);
    }
    Require(chunks[2].size % 32 == 0, "Malformed encoded terrain polygon records");
    for (auto cursor = chunks[2].payloadOffset; cursor < chunks[2].endOffset; cursor += 32) {
        const auto* record = bytes.data() + cursor;
        TerrainEncodedPolygon polygon{};
        polygon.visited = record[0];
        polygon.extrema = record[1];
        polygon.materialIndex = static_cast<std::int8_t>(record[2]);
        polygon.flags = record[3];
        Require((polygon.flags & 10) != 10, "Terrain polygon combines barrier and overhead categories");
        polygon.elevationBand = static_cast<std::int16_t>(ReadU16(record + 4));
        polygon.cacheIndex = -1;
        for (unsigned i = 0; i < 4; ++i) {
            polygon.x[i] = static_cast<std::int16_t>(ReadU16(record + 8 + i * 2));
            polygon.y[i] = static_cast<std::int16_t>(ReadU16(record + 16 + i * 2));
            polygon.z[i] = static_cast<std::int16_t>(ReadU16(record + 24 + i * 2));
        }
        polygons_.push_back(polygon);
    }
    Require(chunks[3].size % 2 == 0, "Malformed terrain polygon index list");
    for (auto cursor = chunks[3].payloadOffset; cursor < chunks[3].endOffset; cursor += 2) {
        const auto index = ReadU16(bytes.data() + cursor);
        Require(index < polygons_.size(), "Terrain polygon reference outside table");
        references_.push_back(index);
    }
    Require(!branches_.empty() && !squares_.empty() && !polygons_.empty(), "Empty collision topology");
    for (const auto& branch : branches_) {
        for (auto index : branch.children)
            Require(index < branches_.size() || index < squares_.size(), "Terrain node index outside tables");
    }
    for (const auto& square : squares_)
        Require(static_cast<std::size_t>(square.firstReference) + square.referenceCount <= references_.size(),
                "Terrain square index range outside table");
    terrainTopologyInitialize(&topology_);
    terrainTopologyInitializeCache(&topology_, cache_.data(), cache_.size());
    const TerrainTopologyChunk views[] = {
        {TERRAIN_CHUNK_BRANCHES, branches_.data(), branches_.size()},
        {TERRAIN_CHUNK_SQUARES, squares_.data(), squares_.size()},
        {TERRAIN_CHUNK_POLYGONS, polygons_.data(), polygons_.size()},
        {TERRAIN_CHUNK_REFERENCES, references_.data(), references_.size()}
    };
    for (const auto& view : views) terrainTopologyLoadChunk(&topology_, &view);
}

TerrainData::~TerrainData() { terrainTopologyDispose(&topology_); }
} // namespace hp2
