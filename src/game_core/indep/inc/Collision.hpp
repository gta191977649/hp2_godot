#ifndef NFSHP2_INDEP_COLLISION_HPP
#define NFSHP2_INDEP_COLLISION_HPP

#include "indep/inc/VoronoiClip.hpp"
#include "indep/inc/Contact.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/Car.hpp"
#include <stddef.h>
#include "bsound/include/bSample.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CarMovement;
struct CollisionBreakable;
struct CollisionBreakableDefinition;
struct CollisionTerrainShape;
struct CollisionObjectRegistry;
struct World;
struct TerrainEffectEmitters;
struct WorldImpactAudio;
struct CollisionAudioVoice;
struct CollisionAudioVoiceBlock;
struct Sound3dBindings;
struct RaceParameters;
typedef struct CollisionParticipant CollisionParticipant;
typedef struct CollisionPair CollisionPair;
typedef struct CollisionQueuedContact CollisionQueuedContact;
typedef struct CollisionSystem CollisionSystem;

/* The participant is a native owner binding. It does not change the shared
 * geometry/shape storage, and never supplies a substitute collision query. */
struct CollisionParticipant {
    CollisionShape* shape;
    struct CarMovement* movement;
    struct CollisionBreakable* breakable;
    TerrainSurface* terrain;
    TerrainPolygon* polygon;
    CarCollisionState* carState;
};

/* A live pair retains its feature witness across the broadphase and all
 * contact-time searches. Storage remains owned by the caller. */
typedef struct CollisionImpact {
    CollisionPair* pair;
    float firstPoint[4];
    float point[4];
    float impulse[4];
    float direction[4];
    float tangentialSpeed;
    float accumulatedMagnitude;
    int32_t createdTicks;
    int32_t lastTicks;
    int32_t occupied;
    struct CollisionAudioVoice* normalSound;
    struct CollisionAudioVoice* scrapeSound;
    struct CollisionImpact* next;
    struct CollisionImpact* previous;
} CollisionImpact;

void collisionImpactGetCars(const CollisionImpact* impact,Car** first,Car** second);

struct CollisionPair {
    CollisionPair* next;
    CollisionPair* previous;
    CollisionWitness witness;
    CollisionParticipant* first;
    CollisionParticipant* second;
    CollisionImpact* impact;
    int32_t processedCount;
    int32_t occupied;
};

struct CollisionQueuedContact {
    CollisionContact view;
    CollisionQueuedContact* next;
    CollisionQueuedContact* previous;
    CollisionPair* pair;
    float displacement[4];
    uint32_t createdFrame;
    int32_t occupied;
};

typedef struct CollisionSolverTuning {
    float contactDistance;
    float normalImpulseMinimum;
    float tangentSpeedScale;
    float separationInitialStep;
    float separationMaximumStep;
    float separationStepIncrease;
    float separationVerticalLimit;
    float coincidentPointDistance;
    float heavyBodyInverseMassLimit;
    float carInertiaScale;
    float carAngularImpulseScale;
    float carFriction;
    float carRestitution;
    float staticFriction;
    float staticRestitution;
    CollisionContactTuning contactOrder;
} CollisionSolverTuning;

typedef enum CollisionSystemResult {
    COLLISION_SYSTEM_OK,
    COLLISION_SYSTEM_PAIR_CAPACITY,
    COLLISION_SYSTEM_CONTACT_CAPACITY,
    COLLISION_SYSTEM_IMPACT_CAPACITY,
    COLLISION_SYSTEM_PARTICIPANT_CAPACITY,
    COLLISION_SYSTEM_TERRAIN_CAPACITY,
    COLLISION_SYSTEM_OBJECT_CAPACITY,
    COLLISION_SYSTEM_EFFECT_CAPACITY,
    COLLISION_SYSTEM_OWNER_FAILURE,
    COLLISION_SYSTEM_MISSING_OWNER
} CollisionSystemResult;

/* Native arrays replace console allocation pools. Failure is reported before
 * using an unavailable slot; no contact is replaced by a fabricated result.
 * Every registered dynamic/terrain shape has an ID in
 * [0,COLLISION_ID_COUNT). Placed objects borrow their separate resource
 * registry until promotion. Bind the actual World's simulation RNG before
 * stepping; its stream is distinct from the default math/effects stream. */
struct CollisionSystem {
    CollisionSpace* space;
    CollisionParticipant* participants[COLLISION_ID_COUNT];
    CollisionPair pairRoot;
    CollisionQueuedContact contactRoot;
    CollisionPair* pairs;
    size_t pairCapacity;
    CollisionQueuedContact* contacts;
    size_t contactCapacity;
    CollisionImpact* impacts;
    size_t impactCapacity;
    /* InitCollision creates32 slots; its expandable source pool grows by9.
     * Separate blocks retain every published pair/impact pointer. */
    struct CollisionImpactBlock* impactBlocks;
    struct CollisionTerrainShape* terrainShapes;
    size_t terrainCapacity;
    struct CollisionObjectRegistry* objects;
    const struct World* world;
    const CollisionSolverTuning* tuning;
    uint32_t* randomState;
    uint32_t frame;
    int32_t nowTicks;
    CarCollisionPolicy carPolicy;
    int32_t objectEffectsEnabled;
    CollisionSystemResult result;
    int32_t responseIterations;
    CollisionImpact* firstImpact;
    CollisionImpact* lastImpact;
    /* Borrow the same world-owned material emitters used by Car effects.
     * The slot stays valid when its catalogue is loaded or released. */
    struct TerrainEffectEmitters* const* terrainEffects;
    struct WorldImpactAudio* contactAudio;
};

extern const CollisionSolverTuning collisionSolverDefaultTuning;

const CollisionImpact* collisionShapeContactAt(const CollisionSystem* system,
                                              const CollisionShape* shape,int32_t index);
int32_t collisionImpactIsCarPair(const CollisionImpact* impact);
int32_t collisionImpactInvolvesPlayer(const CollisionImpact* impact);
int32_t collisionImpactInvolvesSmokeable(const CollisionImpact* impact);

void collisionSystemInitialize(CollisionSystem* system, CollisionSpace* space,
                               CollisionPair* pairs, size_t pairCapacity,
                               CollisionQueuedContact* contacts, size_t contactCapacity,
                               CollisionImpact* impacts, size_t impactCapacity,
                               const CollisionSolverTuning* tuning);
void collisionSystemBindWorld(CollisionSystem* system, struct World* world);
CollisionSystemResult collisionSystemBindTerrainEffects(CollisionSystem* system,
    struct TerrainEffectEmitters* const* emitters);
/* Releases collision-owned pairs, terrain proxies and pooled breakables, and
 * unbinds borrowed cars. The car owner still disposes its own mover. Pending
 * presentation events belong to this World and are cleared at shutdown. */
void collisionSystemDispose(CollisionSystem* system);
CollisionSystemResult collisionCarShapeInitialize(CollisionSystem* system, CollisionShape* shape,
                                                   CarCollisionState* collision, VehicleState* state,
                                                   CollisionGeometry* geometry, TerrainSurface* terrain);
CollisionSystemResult collisionSystemBind(CollisionSystem* system, CollisionParticipant* participant);
void collisionParticipantRefresh(CollisionParticipant* participant);
int32_t collisionParticipantEligible(const CollisionSystem* system, const CollisionParticipant* participant);
int32_t collisionParticipantMovable(const CollisionParticipant* participant);
int32_t collisionParticipantSuppressesResponse(const CollisionSystem* system, const CollisionParticipant* participant);
void collisionParticipantPrepare(CollisionParticipant* participant);
void collisionParticipantCapture(CollisionSystem* system, CollisionParticipant* participant, float rewind);
void collisionParticipantApplyStep(CollisionParticipant* participant, float rewind);
void collisionParticipantNotify(CollisionParticipant* participant, CollisionParticipant* other);
void collisionParticipantVelocity(const CollisionParticipant* participant, float linear[4], float angular[4]);
float collisionParticipantInverseMass(const CollisionSystem* system, const CollisionParticipant* participant);
void collisionParticipantInverseInertia(const CollisionSystem* system, const CollisionParticipant* participant,
                                       float inertia[16]);
void collisionParticipantApplyImpulse(const CollisionSystem* system, CollisionParticipant* participant,
                                     const float impulse[4], const float lever[4]);
float collisionParticipantFriction(const CollisionSystem* system, const CollisionParticipant* participant);
float collisionParticipantRestitution(const CollisionSystem* system, const CollisionParticipant* participant);
float collisionParticipantImpulseFraction(const CollisionParticipant* participant,
                                          const float impulse[4], const float lever[4]);
CollisionImpact* collisionPairImpact(CollisionSystem* system, CollisionPair* pair, const float point[4]);
void collisionImpactAccumulate(CollisionSystem* system, CollisionPair* pair,
                               const float point[4], const float impulse[4], float tangentialSpeed);
int32_t collisionParticipantYawSensitive(const CollisionParticipant* participant);
CollisionPair* collisionPairCreate(CollisionSystem* system, CollisionParticipant* first, CollisionParticipant* second);
void collisionPairDestroy(CollisionSystem* system, CollisionPair* pair);
int32_t collisionPairObsolete(const CollisionSystem* system, const CollisionPair* pair);
void collisionQueueRemoveShape(CollisionSystem* system, const CollisionShape* shape);
void collisionPairQueueContact(CollisionSystem* system, CollisionPair* pair);
void collisionSystemFindPairs(CollisionSystem* system, CollisionParticipant* participant);
CollisionSystemResult collisionSystemStep(CollisionSystem* system, float elapsed);

/* Native intrusive-list/lifetime support shared by the original World,
 * Contact and CollisionBody units; these helpers add no ELF body count. */
void collisionSystemDetachContact(CollisionQueuedContact* contact);
void collisionSystemRemoveImpact(CollisionSystem* system, CollisionPair* pair);
void collisionDynamicShapeSnapshot(CollisionParticipant* participant, struct ReplaySnapshot* snapshot);
void collisionCarSnapshot(CollisionSystem* system, CarCollisionState* car, struct ReplaySnapshot* snapshot);
void collisionImpactSnapshot(CollisionImpact* impact, struct ReplaySnapshot* snapshot);
void collisionPairSnapshot(CollisionSystem* system, CollisionPair* pair, struct ReplaySnapshot* snapshot);
void collisionEliminateAllWitnesses(CollisionSystem* system);
void collisionWitnessesSnapshotEverything(CollisionSystem* system, struct ReplaySnapshot* snapshot);

typedef struct WorldAudioEffects {
    WorldAudioRequestQueue* queue;
    uint32_t* randomState;
    const int32_t* enabled;
} WorldAudioEffects;

WorldAudioResult worldAudioPlayBomb(WorldAudioEffects* effects, const float position[4]);

typedef struct CollisionAudioVoice {
    WorldAudioVoiceRef sample;
    struct CollisionAudioVoice* next;
    struct CollisionAudioVoice* previous;
    int32_t occupied;
    int32_t detached;
} CollisionAudioVoice;

/* PAL Register creates an expandable forty-slot software pool. Additional
 * blocks contain (40>>2)+1 == 11 slots; published wrapper addresses stay fixed.
 * Only Unregister releases their storage after actual sample retirement. */
typedef struct WorldImpactAudio {
    WorldAudioEffects* audio;
    int32_t nextIndex;
    WorldAudioVoiceRef metal[3][3];
    int32_t metalIndex[3];
    WorldAudioVoiceRef woodSamples[3];
    WorldAudioVoiceRef foliage[3];
    int32_t foliageIndex;
    WorldAudioVoiceRef scrape[2];
    WorldAudioVoiceRef glass;
    CollisionSystem* collisions;
    const struct Sound3dBindings* spatial;
    const struct RaceParameters* parameters;
    CollisionAudioVoice voices[40];
    struct CollisionAudioVoiceBlock* voiceBlocks;
    CollisionAudioVoice* firstDetached;
    int32_t registered;
    int32_t updateLoaded;
    int32_t tableLoaded;
    uint32_t petrolPumpKey;
    uint32_t gasPumpKey;
} WorldImpactAudio;

/* The owner must be value-initialized and unregistered before rebinding. */
WorldAudioResult worldImpactAudioBind(WorldImpactAudio* impact, WorldAudioEffects* audio);
size_t collisionAudioVoiceCapacity(const WorldImpactAudio* audio);
const CollisionAudioVoice* collisionAudioVoiceAt(const WorldImpactAudio* audio, size_t index);
WorldAudioResult worldImpactAudioPlay(WorldImpactAudio* impact, const float position[4]);
WorldAudioResult collisionAudioPlayFoliage(WorldImpactAudio* audio, const float position[4]);
WorldAudioResult collisionAudioPlayGlass(WorldImpactAudio* audio, const float position[4]);
WorldAudioResult collisionAudioPlaySmokeable(WorldImpactAudio* audio, const float position[4],
    const struct CollisionBreakableDefinition* definition, uint32_t modelKey);
WorldAudioResult collisionAudioRegister(WorldImpactAudio* audio, CollisionSystem* collisions,
    const struct Sound3dBindings* spatial, const struct RaceParameters* parameters);
WorldAudioResult collisionAudioLoadSampleTable(WorldImpactAudio* audio);
WorldAudioResult collisionAudioUpdate(WorldImpactAudio* audio);
WorldAudioResult collisionAudioPause(WorldImpactAudio* audio, int32_t paused);
WorldAudioResult collisionAudioReset(WorldImpactAudio* audio);
WorldAudioResult collisionAudioUnregister(WorldImpactAudio* audio);
WorldAudioResult collisionAudioGetDistanceToCamera(const WorldImpactAudio* audio,
    const float position[4], float* distance);
WorldAudioResult collisionAudioCalcVolume(const WorldImpactAudio* audio,
    const CollisionImpact* impact, float* volume);
WorldAudioResult collisionAudioCalcScrapeVolume(const WorldImpactAudio* audio,
    const CollisionImpact* impact, float* volume);
WorldAudioResult collisionAudioCreateSample(WorldImpactAudio* audio, CollisionImpact* impact);
WorldAudioResult collisionAudioReleaseSample(WorldImpactAudio* audio, CollisionImpact* impact);
WorldAudioResult collisionAudioPlayScrape(WorldImpactAudio* audio, CollisionImpact* impact);
WorldAudioResult collisionAudioUpdateSample(WorldImpactAudio* audio, CollisionImpact* impact);

typedef struct CollisionTerrainShape {
    CollisionShape shape;
    CollisionParticipant participant;
    CollisionGeometry geometry;
    CollisionFeature vertices[4];
    CollisionFeature edges[4];
    CollisionFeature faces[2];
    CollisionPlane planes[32];
    TerrainTopology* topology;
    TerrainPolygon* polygon;
    int32_t occupied;
} CollisionTerrainShape;

/* Geometry has capacity for four vertices, four edges, two faces and 32
 * adjacency planes. The original duplicate fourth vertex selects a triangle.
 * The supplied origin is subtracted before feature construction. */
void collisionGeometryFromTerrainPolygon(CollisionGeometry* geometry,
                                         const TerrainPolygon* polygon, const float origin[4]);
CollisionSystemResult collisionTerrainShapeInitialize(CollisionSystem* system,
                                                       CollisionTerrainShape* terrain,
                                                       TerrainTopology* topology,
                                                       TerrainPolygon* polygon, int32_t id);
void collisionTerrainShapeDispose(CollisionSystem* system, CollisionTerrainShape* terrain);
void collisionSystemFindTerrainPairs(CollisionSystem* system, CollisionParticipant* participant);
void collisionTerrainShapeSnapshot(CollisionTerrainShape* terrain, struct ReplaySnapshot* snapshot);
void collisionTerrainSnapshotEverything(CollisionSystem* system, TerrainTopology* topology,
                                        struct ReplaySnapshot* snapshot);

#ifdef __cplusplus
}
#endif

#endif
