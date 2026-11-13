#ifndef NFSHP2_TRACK_HPP
#define NFSHP2_TRACK_HPP

/* Native topology/cache declarations for the individually identified
 * TrackPolygon, TopologyCoordinate and TopologyTree consumers. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "indep/inc/TerrainType.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Topology tables and cache storage are borrowed and must outlive their users.
 * Cache capacity is between one and 32767 slots. Release surface references
 * before unloading topology views. Coordinates must
 * be finite and fit source 16.16 conversion range. Polygons are nondegenerate;
 * barrier and overhead flags are mutually exclusive. W is a storage lane for
 * vertices and a plane distance for normals returned by elevation queries. */

#define TERRAIN_SURFACE_POLYGON_LIMIT 160

typedef struct TerrainEncodedPolygon {
    uint8_t visited;
    uint8_t extrema;
    int8_t materialIndex;
    uint8_t flags;
    int16_t elevationBand;
    int16_t cacheIndex;
    int16_t x[4];
    int16_t y[4];
    int16_t z[4];
} TerrainEncodedPolygon;

typedef struct TerrainPolygon {
    struct TerrainPolygon *previous;
    struct TerrainPolygon *next;
    TerrainEncodedPolygon *encoded;
    uint8_t references;
    int8_t vertexCount;
    int8_t materialIndex;
    uint8_t flags;
    bool occupied;
    float minimum[2];
    float maximum[2];
    float vertices[4][4];
    float plane[4];
} TerrainPolygon;

typedef struct TerrainBranch { uint16_t children[4]; } TerrainBranch;
typedef struct TerrainSquare {
    uint16_t firstReference;
    int16_t referenceCount;
} TerrainSquare;

typedef struct TerrainTopology {
    const TerrainBranch *branches;
    size_t branchCount;
    const TerrainSquare *squares;
    size_t squareCount;
    TerrainEncodedPolygon *polygons;
    size_t polygonCount;
    const uint16_t *references;
    size_t referenceCount;
    TerrainPolygon *cache;
    size_t cacheCapacity;
    TerrainPolygon *oldest;
    TerrainPolygon *newest;
} TerrainTopology;

typedef enum TerrainChunkKind {
    TERRAIN_CHUNK_BRANCHES,
    TERRAIN_CHUNK_SQUARES,
    TERRAIN_CHUNK_POLYGONS,
    TERRAIN_CHUNK_REFERENCES
} TerrainChunkKind;

typedef struct TerrainTopologyChunk {
    TerrainChunkKind kind;
    const void *records;
    size_t count;
} TerrainTopologyChunk;

typedef struct TerrainSurface {
    TerrainTopology *topology;
    float queryMinimum[2];
    float queryMaximum[2];
    float cacheMinimum[2];
    float cacheMaximum[2];
    TerrainPolygon *polygons[TERRAIN_SURFACE_POLYGON_LIMIT];
    size_t polygonCount;
    size_t groundCount;
    size_t barrierCount;
    size_t overheadCount;
    TerrainPolygon *lastGround;
} TerrainSurface;

TerrainPolygon *terrainPolygonDecode(TerrainTopology *topology,
                                    TerrainPolygon *polygon,
                                    TerrainEncodedPolygon *encoded);
void terrainPolygonRelease(TerrainTopology *topology, TerrainPolygon *polygon);
bool terrainPolygonContains(const TerrainPolygon *polygon, const float point[4],
                            float *planeDistance);
bool terrainPolygonIntersectSegment(const TerrainPolygon *polygon,
                                    const float first[4], const float second[4],
                                    float intersection[4]);
bool terrainPolygonTestSegment(const TerrainPolygon *polygon, float *closestDistance,
                               const float first[4], const float second[4],
                               const float minimum[4], const float maximum[4],
                               bool twoSided);
void terrainBoundsExpandRadius3(float minimum[4], float maximum[4],
                                const float point[4], float radius);
TerrainTopology *terrainTopologyInitialize(TerrainTopology *topology);
void terrainTopologyInitializeCache(TerrainTopology *topology,
                                    TerrainPolygon *storage, size_t capacity);
void terrainTopologyDispose(TerrainTopology *topology);
bool terrainTopologyLoadChunk(TerrainTopology *topology, const TerrainTopologyChunk *chunk);
bool terrainTopologyUnloadChunk(TerrainTopology *topology, TerrainChunkKind kind);
float terrainTopologySquareCorner(float coordinate);
const TerrainSquare *terrainTopologyFindSquare(const TerrainTopology *topology,
                                              float x, float y, bool nearest);
size_t terrainTopologyCollectSquare(TerrainTopology *topology,
                                    TerrainEncodedPolygon **output, size_t capacity,
                                    const TerrainSquare *square,
                                    const float minimum[2], const float maximum[2]);
size_t terrainTopologyPurgeUnused(TerrainTopology *topology);
TerrainPolygon *terrainTopologyAcquire(TerrainTopology *topology,
                                      TerrainEncodedPolygon *polygon);
void terrainTopologyRelease(TerrainTopology *topology, TerrainPolygon *polygon);
size_t terrainTopologyCollectBounds(TerrainTopology *topology,
                                    TerrainPolygon **output, size_t capacity,
                                    const float minimum[2], const float maximum[2]);
void terrainTopologyReleaseMany(TerrainTopology *topology,
                                TerrainPolygon *const *polygons, size_t count);
TerrainSurface *terrainSurfaceInitialize(TerrainSurface *surface, TerrainTopology *topology);
void terrainSurfaceDispose(TerrainSurface *surface);
void terrainSurfaceReset(TerrainSurface *surface);
void terrainSurfaceUpdate(TerrainSurface *surface, const float point[2],
                          float radius, const float direction[2]);

float terrainSurfaceElevation(TerrainSurface *surface, const float point[4],
                              int32_t *materialIndex, float normal[4], bool *found);
bool terrainSurfaceHasGround(TerrainSurface *surface, const float point[2]);
bool terrainSurfaceIntersectsSegment(const TerrainSurface *surface,
                                     const float first[4], const float second[4],
                                     float radius);
bool terrainSurfaceConstrainSegment(const TerrainSurface *surface, int mode,
                                    bool twoSided, const float first[4],
                                    float second[4], float radius, float extraHeight);
float terrainMaterialNoise(int32_t x, int32_t y);
float terrainMaterialDisplacement(const TerrainMaterial *material,
                                 const float point[4]);

#ifdef __cplusplus
}
#endif


#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fields are named from final-game consumers. This is decoded host data,
 * not a packed console record or a prototype-derived layout. */
typedef struct TrackScenarioDefinition {
    char name[64];
    int8_t courseAvailable;
    int16_t courseIndex;
    int32_t connectionActivationSeconds;
    /* Frontend and gameplay share this one decoded34201 record. The course
     * index above is writable by HighScoreTable::InitStatsTables. */
    int8_t number;
    int8_t baseNumber;
    int8_t routeKind;
    int16_t timeOfDay;
    int32_t nameLanguageIds[2];
    uint32_t policeCharacterKey;
    /* Read by SelectTrackScreen: actual mile/foot labels and conversion
     * branches identify these units; difficulty selects three language IDs. */
    float lengthMiles;
    float elevationFeet[2];
    int32_t difficulty;
    /* InitCars consumes scenario+0x7c after GetTrackInfo. */
    int32_t racerRouteIndex;
} TrackScenarioDefinition;

/* Loaded TRACKB34200 is a separate actual record from the GLOBALB34201
 * array. Only consumed semantic fields are projected; no wire layout is
 * asserted. Audio borrows tireBankName from this retained track owner. */
typedef struct TrackResourceInfo {
    char name[64];
    int8_t number;
    char tireBankName[32];
    /* Actual34200 map transform, consumed by TrackMap1BCA50. */
    float mapOriginX,mapOriginY,mapScale;
    uint16_t mapRotation,startLineRotation,finishLineRotation;
} TrackResourceInfo;

typedef struct TrackLandmark {
    uint32_t key;
    uint32_t authoredValue;
    float position[2];
} TrackLandmark;

typedef struct TrackScenarioCatalog {
    const TrackScenarioDefinition* scenarios;
    size_t scenarioCount;
    const TrackLandmark* landmarks;
    size_t landmarkCount;
} TrackScenarioCatalog;

const TrackScenarioDefinition* trackScenarioFind(const TrackScenarioCatalog* catalog, int32_t oneBasedIndex);
const TrackLandmark* trackLandmarkFind(const TrackScenarioCatalog* catalog, uint32_t key, int32_t occurrence);
int32_t trackLandmarkCount(const TrackScenarioCatalog* catalog,uint32_t key);
const TrackLandmark* trackLandmarkFindName(const TrackScenarioCatalog* catalog, const char* name, int32_t occurrence);

struct TerrainSurface;
int32_t carCameraTerrainOccludes(struct TerrainSurface* surface,const float first[4],const float second[4]);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus



#include <cstdint>
#include <vector>

namespace hp2 {

/* Retained typed34130..34133 storage for the same topology owner. */
class TerrainData final {
public:
    explicit TerrainData(const std::vector<std::uint8_t>& trackBundle,
                         std::size_t cacheCapacity = 1024);
    ~TerrainData();
    TerrainData(const TerrainData&) = delete;
    TerrainData& operator=(const TerrainData&) = delete;
    TerrainTopology& Topology() noexcept { return topology_; }
    std::size_t PolygonCount() const noexcept { return polygons_.size(); }

private:
    std::vector<TerrainBranch> branches_;
    std::vector<TerrainSquare> squares_;
    std::vector<TerrainEncodedPolygon> polygons_;
    std::vector<std::uint16_t> references_;
    std::vector<TerrainPolygon> cache_;
    TerrainTopology topology_{};
};

class TrackScenarioData final {
public:
    explicit TrackScenarioData(const std::vector<std::uint8_t>& track);
    TrackScenarioData(const TrackScenarioData&) = delete;
    TrackScenarioData& operator=(const TrackScenarioData&) = delete;
    const TrackScenarioCatalog& Catalog() const noexcept { return catalog_; }
    const std::vector<TrackScenarioDefinition>& Records() const noexcept { return scenarios_; }
    std::vector<TrackScenarioDefinition>& MutableRecords() noexcept { return scenarios_; }
    const std::vector<std::uint8_t>& Authored() const noexcept { return authored_; }
    const TrackScenarioDefinition* Find(std::int32_t index) const noexcept { return trackScenarioFind(&catalog_, index); }
    const TrackResourceInfo* CurrentInfo() const noexcept {return hasCurrentInfo_ ? &currentInfo_ : nullptr;}

private:
    std::vector<std::uint8_t> authored_;
    std::vector<TrackScenarioDefinition> scenarios_;
    std::vector<TrackLandmark> landmarks_;
    TrackScenarioCatalog catalog_{};
    TrackResourceInfo currentInfo_{};
    bool hasCurrentInfo_{};
};

}

#endif
#endif
