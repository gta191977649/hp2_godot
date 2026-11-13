#ifndef NFSHP2_INDEP_INC_SPIKESTRIP_HPP
#define NFSHP2_INDEP_INC_SPIKESTRIP_HPP

#include "indep/inc/Track.hpp"
#include "indep/inc/WorldObject.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define PURSUIT_SPIKE_SEGMENTS 20
#define PURSUIT_SPIKE_INITIAL_COUNT 10

struct PursuitSpikeRegistry;
struct PursuitSpikePoolBlock;
struct ReplaySnapshot;

typedef struct PursuitSpikeSegment {
    float frame[16];
    int32_t visible;
    WorldObject renderElement;
} PursuitSpikeSegment;

/* The live authored model instances retain their actual world frames and
 * visibility. Rendering consumes this owner; it does not own its lifetime.
 * Set requires finite distinct XY endpoints and source-valid terrain data. */
typedef struct PursuitSpikeStrip {
    float first[2];
    float second[2];
    int32_t ownerSelector;
    int32_t retracting;
    float retractSeconds;
    float lifetime;
    int32_t segmentCount;
    PursuitSpikeSegment segments[PURSUIT_SPIKE_SEGMENTS];
    uint32_t modelKey;
    int32_t occupied;
    struct PursuitSpikeStrip* previous;
    struct PursuitSpikeStrip* next;
    struct PursuitSpikeRegistry* registry;
} PursuitSpikeStrip;

typedef struct PursuitSpikeRegistry {
    PursuitSpikeStrip* strips;
    size_t capacity;
    TerrainTopology* terrain;
    WorldObjectRegistry* objects;
    PursuitSpikeStrip* first;
    PursuitSpikeStrip* last;
    /* Source pool starts at10 and grows by3. Borrowers retain each embedded
     * WorldObject/frame address until the strip's actual destruction. */
    PursuitSpikeStrip* freeStrips;
    struct PursuitSpikePoolBlock* expanded;
    size_t totalCapacity;
} PursuitSpikeRegistry;

void pursuitGroundPoint(TerrainTopology* terrain, float output[4], const float point[2]);
void pursuitSpikeStripSet(PursuitSpikeStrip* strip, TerrainTopology* terrain,
                          const float first[2], const float second[2],
                          int32_t ownerSelector, float lifetime);
int32_t pursuitSpikeStripAdvance(PursuitSpikeStrip* strip, float elapsed);
void pursuitSpikeStripBeginRetraction(PursuitSpikeStrip* strip);
void pursuitSpikeStripCenter(const PursuitSpikeStrip* strip, float output[4]);

/* Native pool ownership replaces console model/pool allocation. These
 * adapters are not additional original functions in the transfer count. */
void pursuitSpikeRegistryInitialize(PursuitSpikeRegistry* registry,
                                    PursuitSpikeStrip* storage, size_t capacity,
                                    TerrainTopology* terrain,
                                    WorldObjectRegistry* objects);
PursuitSpikeStrip* pursuitSpikeCreate(PursuitSpikeRegistry* registry,
                                      const float first[2], const float second[2],
                                      int32_t ownerSelector, float lifetime);
void pursuitSpikeDestroy(PursuitSpikeStrip* strip);
void pursuitSpikeRegistryDispose(PursuitSpikeRegistry* registry);
/* Pool allocation is separate from Init, as required by replay restore. */
PursuitSpikeStrip* pursuitSpikeAllocate(PursuitSpikeRegistry* registry);
void pursuitSpikeStripDoSnapshot(PursuitSpikeStrip* strip,struct ReplaySnapshot* snapshot);

#ifdef __cplusplus
}
#endif
#endif
