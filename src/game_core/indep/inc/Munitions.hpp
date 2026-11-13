#ifndef NFSHP2_INDEP_MUNITIONS_HPP
#define NFSHP2_INDEP_MUNITIONS_HPP

#include "indep/inc/Smokeable.hpp"
#include "Chopper.hpp"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/Collision.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct CarFramePlayerRegistry;
struct PursuitManager;
struct ReplayManager;

typedef enum ChopperMunitionKind {
    CHOPPER_MUNITION_ROCKET,
    CHOPPER_MUNITION_SMOKE_BOMB,
    CHOPPER_MUNITION_SPIKE_BOMB
} ChopperMunitionKind;

enum {
    /* Initial SlotPool counts, not maximum live actor counts. Each original
     * pool grows by its own initial_count/4+1 when its free list is empty. */
    CHOPPER_ROCKET_CAPACITY = 3,
    CHOPPER_SMOKE_BOMB_CAPACITY = 14,
    CHOPPER_SPIKE_BOMB_CAPACITY = 8
};

typedef struct ChopperExplosionTuning {
    float innerImpulse;
    float outerImpulse;
    float leverHeight;
    float contactLift;
    float innerRadius;
    float outerRadius;
    int32_t lockPlayer;
} ChopperExplosionTuning;

typedef struct ChopperMunitionBindings {
    CollisionSystem* collisions;
    TerrainTopology* terrain;
    const CollisionBreakableDefinition* rocketDefinition;
    const CollisionBreakableDefinition* smokeDefinition;
    ChopperTarget* vehicles;
    size_t vehicleCount;
    struct CarFramePlayerRegistry* players;
    struct PursuitManager* pursuit;
    struct ReplayManager* replay;
    ChopperMunitionEffects* effects;
    WorldAudioEffects* audio;
    const int32_t* impactPresentationMode;
    int32_t* impactPresentationRequested;
    const CarRecoveryPointRegistry* road;
    WorldImpactAudio* contactAudio;
} ChopperMunitionBindings;

typedef struct ChopperMunition {
    ChopperMunitionRegistry* registry;
    CollisionBreakable* physical;
    /* The derived actor owns its one physical base. The generic32-slot
     * Smokeable pool is a distinct source allocation. */
    CollisionBreakable physicalStorage;
    ChopperMunitionKind kind;
    int32_t skipCapture;
    int32_t triggered;
    ChopperTarget* target;
    float heading;
    int32_t occupied;
    struct ChopperMunition* nextFree;
} ChopperMunition;

struct ChopperMunitionPoolBlock;

/* The derived owner borrows exactly one physical object. Neither the
 * Chopper pass nor a second scheduler integrates that body a second time. */
struct ChopperMunitionRegistry {
    ChopperMunitionBindings bindings;
    ChopperMunition* actors;
    /* Borrowed initial native arena. Expanded blocks never move actors or
     * their embedded physical bases. The three source pools stay distinct. */
    size_t capacity;
    size_t initialUsed;
    size_t initialAssigned[3];
    size_t poolCapacity[3];
    ChopperMunition* freeLists[3];
    struct ChopperMunitionPoolBlock* expanded;
    WorldServicesResult result;
};

void chopperMunitionRegistryInitialize(ChopperMunitionRegistry* registry,
    ChopperMunition* actors, size_t capacity, const ChopperMunitionBindings* bindings);
ChopperMunition* chopperRocketCreate(ChopperMunitionRegistry* registry);
ChopperMunition* chopperSmokeBombCreate(ChopperMunitionRegistry* registry);
ChopperMunition* chopperSpikeBombCreate(ChopperMunitionRegistry* registry);
/* Same initially3/14/8 pools, with the source factory's explicit collision
 * ID and authored definition/model. Native allocation adapter, no new body. */
ChopperMunition* chopperMunitionConstruct(ChopperMunitionRegistry* registry,
    ChopperMunitionKind kind,int32_t bodyId,
    const CollisionBreakableDefinition* definition,uint32_t modelKey);
void chopperMunitionDoSnapshot(ChopperMunition* actor,struct ReplaySnapshot* snapshot);
void chopperMunitionSetTransform(ChopperMunition* actor, const float frame[16], const float velocity[4]);
void chopperRocketLaunch(ChopperMunition* actor, const float frame[16],
    const float velocity[4], ChopperTarget* target);
void chopperMunitionBuildFrame(const ChopperMunition* actor, float output[16]);
void chopperMunitionApplyStep(ChopperMunition* actor, float rewind);
float chopperRocketImpulseFraction(ChopperMunition* actor);
void chopperRocketApplyImpulse(ChopperMunition* actor);
float chopperSpikeImpulseFraction(ChopperMunition* actor);
void chopperSpikeApplyImpulse(ChopperMunition* actor);
void chopperMunitionDestroy(ChopperMunition* actor);
/* Called by the actual base-object destructor before releasing its pool
 * slot. This retires the borrowed actor link without another base destroy. */
void chopperMunitionPhysicalDestroyed(ChopperMunition* actor, CollisionBreakable* physical);
void chopperMunitionRegistryDispose(ChopperMunitionRegistry* registry);
void chopperMunitionReleaseTarget(ChopperMunitionRegistry* registry, const Car* car);

/* Physical update/detonation are concrete functions. Gameplay owners must
 * be bound before these entry points are integrated into the world pass. */
void chopperRocketAdvance(ChopperMunition* actor, float elapsed);
void chopperSmokeBombAdvance(ChopperMunition* actor, float elapsed);
void chopperSpikeBombAdvance(ChopperMunition* actor, float elapsed);
void chopperMunitionAdvance(ChopperMunition* actor, float elapsed);
void chopperMunitionDetonate(ChopperMunition* actor);
void chopperMunitionExplode(ChopperMunition* actor, const ChopperExplosionTuning* tuning,
    int32_t reactionKind);
const ChopperExplosionTuning* chopperMunitionExplosionTuning(ChopperMunitionKind kind);

#ifdef __cplusplus
}

#include <array>
namespace hp2 {
class CollisionObjectData;
/* Final InitMunitions creates3/14/8 initially allocated typed slots. The immutable
 * decoded effect definitions replace read-only prototype clones; all live
 * instances borrow the same application AcidFX pool and carry this owner. */
class MunitionResources final {
public:
    MunitionResources(const ChopperMunitionBindings& bindings,
        const CollisionObjectData& resources);
    ~MunitionResources();
    MunitionResources(const MunitionResources&)=delete;
    MunitionResources& operator=(const MunitionResources&)=delete;
    ChopperMunitionRegistry& Actors() noexcept { return registry_; }
    const ChopperEffectGroup& DetonationGroup(ChopperMunitionKind kind) const;
private:
    std::array<ChopperMunition,CHOPPER_ROCKET_CAPACITY+CHOPPER_SMOKE_BOMB_CAPACITY+CHOPPER_SPIKE_BOMB_CAPACITY> actors_{};
    std::array<const ChopperEffectGroup*,3> detonationGroups_{};
    ChopperMunitionRegistry registry_{};
};
}
#endif
#endif
