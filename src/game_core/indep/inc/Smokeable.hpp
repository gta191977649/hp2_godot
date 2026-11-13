#ifndef NFSHP2_COLLISION_OBJECTS_H
#define NFSHP2_COLLISION_OBJECTS_H

#include "indep/inc/Collision.hpp"
#include "indep/inc/ComponentPhysics.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CollisionBreakableDefinition CollisionBreakableDefinition;
typedef struct CollisionBreakable CollisionBreakable;
struct ChopperMunition;
struct ChopperMunitionEffects;
struct ChopperEffectInstance;
struct ChopperMunitionRegistry;
struct ReplaySnapshot;
struct Scenery;
struct SceneryInstance;
struct WorldObject;
struct WorldObjectRegistry;
struct CollisionObjectPoolBlock;
struct CollisionEffectGroupBlock;

typedef struct CollisionObjectEffectGroup {
    uint32_t resourceKey;
    struct ChopperEffectInstance* firstEmitter;
    struct ChopperEffectInstance* lastEmitter;
    int32_t started;
    int32_t occupied;
} CollisionObjectEffectGroup;

/* Decoded final-game resources are borrowed. Geometry has already been
 * recentered with centerOfMass. Fragments refer to actual resource records. */
struct CollisionBreakableDefinition {
    const char* name;
    const RigidBodyInfo* body;
    CollisionGeometry* geometry;
    float centerOfMass[4];
    float lifetime;
    float activationImpulse;
    float breakImpulse;
    float restitution;
    float friction;
    int32_t materialIndex;
    const CollisionBreakableDefinition* const* fragments;
    size_t fragmentCount;
    const uint32_t* artKeys;
    size_t artKeyCount;
    const uint32_t* activationEffectGroups;
    size_t activationEffectGroupCount;
    const uint32_t* movingEffectGroups;
    size_t movingEffectGroupCount;
    int32_t retainWhenDistant;
};

typedef struct CollisionPlacedObject {
    const CollisionBreakableDefinition* definition;
    TerrainTopology* topology;
    // Immutable authored Scenery transform used by dynamic promotion.
    float frame[16];
    // The separate StaticCollisionBody orientation belongs to this placement,
    // not to its render input or a writable global identity shared by bodies.
    float staticOrientation[16];
    float minimum[2];
    float maximum[2];
    uint32_t modelKey;
    uint16_t sceneryFlags;
    // The retained SmokeableSpawner names one actual Scenery instance.
    int32_t scenerySection;
    int32_t sceneryInstance;
    struct SceneryInstance* scenery;
    CollisionShape shape;
    CollisionParticipant participant;
    CollisionBreakable* activeObject;
    int32_t occupied;
} CollisionPlacedObject;

struct CollisionBreakable {
    CollisionShape shape;
    CollisionParticipant participant;
    CollisionSystem* system;
    const CollisionBreakableDefinition* definition;
    CollisionPlacedObject* placement;
    // Source embedded WorldObject+3F0, retained in the same native draw list.
    struct WorldObject* visual;
    RigidBody body;
    ComponentSystem components;
    ComponentBinding binding;
    float derivatives[29];
    TerrainSurface terrain;
    float displayFrame[16];
    int32_t displayChanged;
    CollisionObjectEffectGroup* activationEffectGroups;
    size_t activationEffectGroupCount;
    CollisionObjectEffectGroup* movingEffectGroups;
    size_t movingEffectGroupCount;
    uint32_t modelKey;
    float activationImpulse;
    float breakImpulse;
    float elevationOffset;
    int32_t createdTicks;
    int32_t activatedTicks;
    int32_t recentCarContactTicks;
    int32_t simulating;
    /* Final Smokeable+463: constructor-zeroed byte, serialized separately
     * from the +462 simulation flag. No gameplay consumer is inferred. */
    int8_t snapshotFlag;
    int8_t responseSuppressed;
    int8_t activationEffectsState;
    int8_t movingEffectsStarted;
    int32_t occupied;
    /* Borrowed derived owner. The actor and this one physical object share
     * their lifetime; no second body or integration record is created. */
    struct ChopperMunition* munition;
    /* Native stable SlotPool links. Derived munitions never join this pool. */
    CollisionBreakable* nextFree;
    int32_t genericPoolSlot;
};

typedef enum CollisionEffectKind {
    COLLISION_EFFECT_CONTACT_BEGIN,
    COLLISION_EFFECT_CONTACT_END,
    COLLISION_EFFECT_SCRAPE,
    COLLISION_EFFECT_IMPACT,
    COLLISION_EFFECT_OBJECT_ACTIVATION,
    COLLISION_EFFECT_GROUP_START,
    COLLISION_EFFECT_GROUP_STOP,
    COLLISION_EFFECT_CAR_IMPACT,
    COLLISION_EFFECT_OBJECT_DESTROY
} CollisionEffectKind;

/* Rendering and audio consume semantic native events. Physics state and
 * resource ownership are updated immediately by the collision owners. */
typedef struct CollisionEffect {
    CollisionEffectKind kind;
    CollisionParticipant* owner;
    uint32_t resourceKey;
    int32_t materialIndex;
    float strength;
    float point[4];
    float velocity[4];
    float frame[16];
} CollisionEffect;

typedef struct CollisionObjectRegistry {
    CollisionBreakable* objects;
    size_t capacity;
    CollisionPlacedObject* placements;
    size_t placementCount;
    CollisionObjectEffectGroup* effectGroups;
    size_t effectGroupCapacity;
    CollisionEffect* effects;
    size_t effectCapacity;
    size_t effectCount;
    /* Borrow the same decoded catalogue, instance pool and CPU particles
     * used by munitions. Groups own their persistent instances in that pool. */
    struct ChopperMunitionEffects* particleEffects;
    struct WorldObjectRegistry* worldObjects; // borrowed canonical draw owner
    /* Source GP-3DF8. Wake/Sleep, not object allocation, change this word. */
    int32_t simulatingCount;
    /* The supplied array is the initial PAL pool. Expansion owns separate
     * blocks, so live shapes, contacts, FX and snapshot borrows never move. */
    CollisionBreakable* freeObjects;
    struct CollisionObjectPoolBlock* expandedObjects;
    struct CollisionEffectGroupBlock* expandedEffectGroups;
    size_t totalObjectCapacity;
    size_t occupiedObjects;
} CollisionObjectRegistry;

void collisionObjectRegistryInitialize(CollisionObjectRegistry* registry,
                                       CollisionBreakable* objects, size_t capacity,
                                       CollisionPlacedObject* placements, size_t placementCount,
                                       CollisionObjectEffectGroup* effectGroups, size_t effectGroupCapacity,
                                       CollisionEffect* effects, size_t effectCapacity);
/* Native storage adapters for the actual expandable source SlotPool. */
size_t collisionObjectRegistryFreeSlots(const CollisionObjectRegistry* registry);
void collisionObjectRegistryDisposeStorage(CollisionObjectRegistry* registry);
CollisionSystemResult collisionObjectRegistryBindEffects(CollisionObjectRegistry* registry,
    struct ChopperMunitionEffects* effects);
/* Native ownership adapters; these add no original function to the count. */
CollisionSystemResult collisionObjectEffectGroupCreate(CollisionObjectRegistry* registry,
    CollisionObjectEffectGroup* group, uint32_t key);
CollisionSystemResult collisionObjectEffectGroupStart(CollisionBreakable* object,
    CollisionObjectEffectGroup* group);
void collisionObjectEffectGroupSetTransform(CollisionObjectEffectGroup* group,
    const float* frame, const float* velocity);
void collisionObjectEffectGroupRelease(CollisionObjectRegistry* registry,
    CollisionObjectEffectGroup* group);
void collisionObjectEffectGroupRecord(CollisionBreakable* object,
    CollisionObjectEffectGroup* group, CollisionEffectKind kind);
CollisionSystemResult collisionPlacedObjectInitialize(CollisionSystem* system, CollisionPlacedObject* placement);
CollisionParticipant* collisionObjectParticipant(const CollisionSystem* system, const CollisionShape* shape);
CollisionParticipant* collisionBreakablePromote(CollisionSystem* system, CollisionParticipant* participant);
int32_t collisionBreakableChildrenAvailable(const CollisionBreakableDefinition* definition, uint32_t modelKey);
CollisionBreakable* collisionBreakableCreate(CollisionSystem* system,
                                             const CollisionBreakableDefinition* definition,
                                             TerrainTopology* topology, uint32_t modelKey);
/* Initialize the base embedded in a concrete derived munition. The owner
 * supplies an unused slot; geometry, IDs, contacts and effects stay canonical. */
CollisionBreakable* collisionBreakableConstruct(CollisionSystem* system,
    CollisionBreakable* object,const CollisionBreakableDefinition* definition,
    TerrainTopology* topology,uint32_t modelKey,int32_t requestedId);
void collisionBreakableDoSnapshot(CollisionBreakable* object,struct ReplaySnapshot* snapshot);
void collisionBreakableSetTransform(CollisionBreakable* object, const float frame[16], const float velocity[4]);
/* Generic Smokeable placement uses the existing physical pool and terrain.
 * Returns the original signed body ID, or -1 when placement is unavailable. */
int16_t collisionSmokeableCreate(CollisionSystem* system, TerrainTopology* topology,
    const CollisionBreakableDefinition* definition, uint32_t modelKey,
    const float point[4], uint16_t yaw);
CollisionSystemResult collisionSmokeableDelete(CollisionSystem* system, int16_t id,
    const CollisionBreakableDefinition* definition);
void collisionBreakablePrepare(CollisionBreakable* object);
int32_t collisionBreakableRestClass(const CollisionBreakable* object);
void collisionBreakableWake(CollisionBreakable* object);
void collisionBreakableSleep(CollisionBreakable* object);
void collisionBreakableStartActivationEffects(CollisionBreakable* object);
float collisionBreakableImpulseFraction(CollisionBreakable* object, const float impulse[4], const float lever[4]);
void collisionBreakableAdvance(CollisionSystem* system, CollisionBreakable* object, float elapsed);
/* Shared portions of Smokeable::DoTimeStep used by derived munitions. */
void collisionBreakableUpdateTerrain(CollisionBreakable* object);
void collisionBreakableUpdateEffects(CollisionBreakable* object, const float frame[16]);
void collisionBreakableUpdateLifetime(CollisionSystem* system, CollisionBreakable* object);
size_t collisionBreakableFracture(CollisionSystem* system, CollisionBreakable* object,
                                  CollisionParticipant** output, size_t capacity);
void collisionBreakableDestroy(CollisionSystem* system, CollisionBreakable* object);
/* Source SmokeableThink and DeleteFarAwaySmokeableCollisionBodies. */
void worldServicesCullBreakables(CollisionSystem* system);
void worldServicesPeriodicObjectCull(CollisionSystem* system);
void collisionCarRecordImpact(CollisionParticipant* participant, const float impulse[4], const float lever[4]);
void collisionImpactInitialize(CollisionSystem* system, CollisionImpact* impact, CollisionPair* pair,
                                const float point[4]);
void collisionImpactDispose(CollisionSystem* system, CollisionImpact* impact);
int32_t collisionPairMaterialIndex(const CollisionPair* pair);
void collisionRecordContactEffects(CollisionSystem* system, const CollisionPair* pair,
                                   const float point[4], const float impulse[4], float magnitude,
                                   float tangentialSpeed);
/* Shared native event storage; this adapter adds no original function. */
CollisionEffect* collisionObjectNewEffect(CollisionSystem* system, CollisionEffectKind kind,
                                          CollisionParticipant* owner);

#ifdef __cplusplus
}
#include <memory>
struct RaceParameters;
namespace hp2 {
class CollisionObjectData;
/* SpawnSmokeables/ClearSmokeables initially own32 generic physical slots and the
 * placed shapes in the loaded catalogue. Derived munition bases belong to
 * their own typed actors, all registered in the same CollisionSystem. */
class SmokeableScene final {
public:
    SmokeableScene(CollisionSystem& collision,CollisionObjectData& data,
        ChopperMunitionEffects& effects,const RaceParameters& parameters,
        Scenery* scenery=nullptr,WorldObjectRegistry* worldObjects=nullptr);
    ~SmokeableScene();
    SmokeableScene(const SmokeableScene&)=delete;
    SmokeableScene& operator=(const SmokeableScene&)=delete;
    CollisionObjectRegistry& Registry() noexcept;
    std::size_t SpawnedPlacements() const noexcept;
    void DoSnapshot(ReplaySnapshot& snapshot,ChopperMunitionRegistry& munitions,
        const CollisionObjectData& inGameObjects);
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
#endif
#endif
