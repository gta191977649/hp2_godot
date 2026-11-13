#ifndef NFSHP2_PURSUIT_H
#define NFSHP2_PURSUIT_H

#include "indep/inc/World.hpp"
#include "indep/src/speech/mgr.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/CarState.hpp"
#include <stdint.h>

#include "indep/inc/World.hpp"
#include "indep/inc/DriveTarget.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/spikestrip.hpp"
#include "bsound/include/bSample.h"

// Native carrier for PAL tNearFollowTarget, owned by the actual pursuit.
enum { PURSUIT_VELOCITY_SAMPLES=30 };

typedef struct PursuitVelocityHistory {
    int32_t count;
    float velocity[PURSUIT_VELOCITY_SAMPLES][2];
    float elapsed[PURSUIT_VELOCITY_SAMPLES];
} PursuitVelocityHistory;


struct CarMovement;
struct Chopper;
struct ApplicationLaunchOptions;
struct WorldSimulation;
struct RaceFrame;
struct WorldAudioMessages;
typedef struct WorldSpeechManager WorldSpeechManager;
typedef struct PursuitCollisionEntry PursuitCollisionEntry;
typedef struct PursuitCollisionGroup PursuitCollisionGroup;
typedef struct PursuitCollisionContext PursuitCollisionContext;

/* Immutable authored site values are retained once by the track catalogue.
 * A live block re-reads this pattern when recycled, then owns its actual
 * randomized layout independently of that immutable source selection. */
typedef struct PursuitRoadBlockSite {
    float position[2];
    int32_t directionEligibility;
    int32_t authoredPattern;
    uint32_t authoredOption6C;
    int32_t initiallyActive;
    char name[17];
} PursuitRoadBlockSite;

typedef struct PursuitRoadBlock {
    PursuitCollisionEntry* entries[2];
    float carPositions[2][2];
    uint16_t carHeadings[2];
    const CarRecoveryGroup* roadGroup;
    const CarRecoveryPose* roadPose;
    float width;
    int32_t pattern;
    float center[2];
    float halfSpan[2];
    float groundPoint[4];
    float across[2];
    float along[2];
    float routeDistance;
    int32_t siteIndex;
    uint32_t flags;
    int32_t active;
    int32_t radioChannel;
    const PursuitRoadBlockSite* site;
} PursuitRoadBlock;

typedef struct PursuitSpeedTrapSite {
    char name[17];
    float position[2];
    int32_t authoredFlagFirst;
    int32_t authoredFlagSecond;
} PursuitSpeedTrapSite;

/* This is the same bidirectional entry/trap owner formerly declared in
 * PursuitSpeedTrap.h. Authored sites remain stable and readonly. */
typedef struct PursuitSpeedTrap {
    PursuitCollisionEntry* officer;
    float position[2];
    uint16_t heading;
    const PursuitSpeedTrapSite* site;
    float routeDistance;
    uint32_t flags;
} PursuitSpeedTrap;

typedef struct PursuitBarricadeManager PursuitBarricadeManager;
typedef struct PursuitGroupState PursuitGroupState;
typedef struct PursuitGroupStorage PursuitGroupStorage;
typedef struct PursuitRoadBlockCatalog PursuitRoadBlockCatalog;
typedef struct PursuitSpeedTrapCatalog PursuitSpeedTrapCatalog;
struct PursuitManagerOwnedState;
#ifdef __cplusplus
namespace hp2 { struct PursuitConstructionBindings; }
#endif

struct PursuitCollisionGroup {
    struct CarMovement* target;
    PursuitCollisionEntry* active[2];
    int32_t activeCount;
    PursuitRoadBlock* reserved;
    int32_t currentChannel;
    int32_t nextChannel;
    float elapsed;
    int32_t extendedTimeout;
    struct Chopper* chopper;
    uint32_t requestFlags;
    PursuitVelocityHistory velocityHistory;
    DriverCommand targetPath;
    DriverCommand recentTargetPath;
    /* Source +5D4; independent of the pursuit state at +5CC. */
    int32_t hudLevel;
    /* Source +5CC is independent of the HUD state at +5D4. The remaining
     * typed group state belongs to the same stable native group object. */
    int32_t phase;
    PursuitGroupState* owner;
};

/* Eight racer records are distinct from the manager's indexed officer
 * entries. selectedOfficer is an index into collision.entries (-1: none).
 * The two timers correspond to the original per-racer 2874/2894 arrays. */
typedef struct PursuitParticipantReset {
    int32_t selectedOfficer;
    int32_t resetPending;
    int32_t finishOnReset;
    int32_t restoreOverlay;
    float spawnDelay;
    float preventPursuitSeconds;
    float resetDelaySeconds;
    int32_t waitForCinematic;
    int8_t pulloverCount;
} PursuitParticipantReset;

/* Collision, player commands and the world update borrow the same entries
 * and groups. Resource allowances are race-owned manager state, rather than
 * independent flags on the player or a copied selected entry. */
struct PursuitManager {
    PursuitCollisionContext* collision;
    World* world;
    WorldAudioRequestQueue* audio;
    const int32_t* applicationState;
    int32_t roadBlocksRemaining;
    int32_t chopperRequestsRemaining;
    int32_t chopperReserved;
    WorldServicesResult result;
    const GameTime* clock;
    PursuitSpikeRegistry* spikes;
    PursuitCollisionGroup** groups;
    size_t groupCount;
    size_t groupCapacity;
    float remainingSeconds;
    struct WorldSimulation* simulation;
    struct RaceFrame* race;
    const struct RaceParameters* parameters;
    const struct ApplicationLaunchOptions* launch;
    struct WorldAudioMessages* speech;
    PursuitParticipantReset participantReset[8];
    int32_t completedPullovers;
    float speechElapsed;
    float speechGap;
    float supervisorRemaining;
    uint32_t lastFillerEvent;
    PursuitGroupStorage* ownedGroups;
    /* Source +2860, popped once by the real manager initialization when
     * specialPursuit is enabled; individual groups borrow this channel. */
    int32_t selectedRadioChannel;
    /* Source +2908/+290C, derived once by SetCopDimensions from the first
     * actual officer's canonical CarInfo values. No independent catalogue. */
    float copDimensions[2];
    PursuitRoadBlockCatalog* roadBlockCatalog;
    PursuitSpeedTrapCatalog* speedTrapCatalog;
    /* Source28F4/28F8 are independent ctor-zero, once-per-race gates. */
    int32_t beginSpeechSent;
    int32_t introSpeechSent;
    struct PursuitManagerOwnedState* ownedState
#ifdef __cplusplus
        = nullptr
#endif
        ;
#ifdef __cplusplus
    PursuitManager() noexcept = default;
    explicit PursuitManager(const hp2::PursuitConstructionBindings& bindings);
    ~PursuitManager();
    PursuitManager(const PursuitManager&) = delete;
    PursuitManager& operator=(const PursuitManager&) = delete;
#endif
};

/* PAL331C00 is a single writable tuning cell. Constructor difficulty
 * changes remain visible to every group and collision consumer. */
typedef struct PursuitTimingTuning {
    float radioTimeout;
    float captureRadius;
} PursuitTimingTuning;

typedef enum PursuitCollisionEntryKind {
    PURSUIT_COLLISION_EXTERNAL_CAR,
    PURSUIT_COLLISION_OWNED_CAR
} PursuitCollisionEntryKind;

typedef struct PursuitChannelPool {
    int32_t channels[4];
    int32_t readIndex;
    int32_t writeIndex;
    int32_t full;
} PursuitChannelPool;



/* The entry kind denotes the actual concrete source owner. Its eligibility
 * is a property of that class, not a copied flag from another owner. */
struct PursuitCollisionEntry {
    PursuitCollisionGroup* group;
    struct CarMovement* movement;
    PursuitCollisionEntryKind kind;
    int32_t lifecycleState;
    int32_t state;
    int32_t requestedDrivingState;
    float graceRemaining;
    CarRecoverySelection recovery;
    float alongOffset;
    float acrossOffset;
    int32_t laneIndex;
    int32_t selectedTargetIndex;
    int32_t previousTargetIndex;
    float targetSelectionTimeout;
    float previousTargetTimeout;
    float targetOffset[2];
    int32_t spikeOwnerSelector;
    int32_t drivingState;
    /* Source +2C ticket state timer; graceRemaining belongs to +20. */
    float drivingStateSeconds;
    /* Concrete owned-officer payload. External officers do not borrow
     * this record. Source24 is distinct from every road-block owner. */
    PursuitSpeedTrap* speedTrap;
    float offsetRate[2];
    float modeElapsedSeconds;
    float modeSwitchDelaySeconds;
    int32_t roadBlockSlot;
    /* Source +48, independent of path offsets and target-selection timers. */
    float flankSide;
    /* Source base +08 officer index; not the target Car's table index. */
    int32_t officerIndex;
};

typedef enum PursuitCollisionResult {
    PURSUIT_COLLISION_OK,
    PURSUIT_COLLISION_INVALID_BINDING,
    PURSUIT_COLLISION_EMPTY_CHANNEL_POOL
} PursuitCollisionResult;

typedef struct PursuitCollisionContext {
    PursuitCollisionEntry** entries;
    size_t entryCount;
    size_t activeEntry;
    int32_t severity;
    const struct RaceParameters* parameters;
    WorldSpeechManager* speech;
    const PursuitTimingTuning* timing;
    float damageLevels[WORLD_MAX_CARS];
    PursuitChannelPool channels;
    PursuitCollisionResult result;
    size_t primaryEntryCount;
    /* Same source manager supplies participant timers; no copied score or
     * prevent-pursuit array is stored in this collision projection. */
    struct PursuitManager* manager;
} PursuitCollisionContext;

#ifdef __cplusplus
extern "C" {
#endif
extern PursuitTimingTuning pursuitTimingTuning;
void pursuitVelocityHistoryInitialize(PursuitVelocityHistory* history);
float* pursuitVelocityHistoryPush(float output[2],PursuitVelocityHistory* history,
    const float position[2],const float velocity[2],float elapsed);
float* pursuitVelocityHistorySample(float output[2],const PursuitVelocityHistory* history,float seconds);

void pursuitCollisionInitialize(PursuitCollisionContext* context);
void pursuitChannelPush(PursuitChannelPool* pool, int32_t channel);
int32_t pursuitChannelPop(PursuitChannelPool* pool, int32_t* channel);
PursuitCollisionEntry* pursuitCollisionFind(const PursuitCollisionContext* context, const struct CarMovement* movement);
/* Released2274A8 and2289F8, queried by the original Player HUD. */
float pursuitManagerRadarIntensity(const PursuitManager* manager,const Car* car,int32_t* activePursuit);
int32_t pursuitManagerShouldDisplayBustedText(const PursuitManager* manager);
void pursuitCollisionSetGroup(PursuitCollisionEntry* entry, PursuitCollisionGroup* group);
void pursuitCollisionSetState(PursuitCollisionEntry* entry, int32_t state);
void pursuitCollisionDoneWritingTicket(PursuitCollisionEntry* entry);
int32_t pursuitCollisionGroupRemove(PursuitCollisionGroup* group, PursuitCollisionEntry* entry);
void pursuitCollisionDetach(PursuitCollisionEntry* entry);
int32_t pursuitCollisionRadioSuppressed(const PursuitCollisionContext* context, const PursuitCollisionGroup* group);
void pursuitCollisionChangeRadio(PursuitCollisionContext* context, PursuitCollisionGroup* group, int32_t event);
void pursuitCollisionDisable(PursuitCollisionContext* context, PursuitCollisionEntry* entry, int32_t event);
PursuitCollisionResult pursuitCollisionRecord(PursuitCollisionContext* context, struct CarMovement* movement,
                                             float magnitude, int32_t carContact);

PursuitCollisionEntry* pursuitManagerSelectedEntry(const PursuitManager* manager);
WorldServicesResult pursuitManagerRequestRoadBlock(PursuitManager* manager);
WorldServicesResult pursuitManagerRequestChopper(PursuitManager* manager);
WorldServicesResult pursuitManagerSignalHorn(PursuitManager* manager);
WorldServicesResult pursuitManagerResetParticipant(PursuitManager* manager, int32_t index);
WorldServicesResult pursuitManagerResetCarDamage(PursuitManager* manager, Car* car);
/* Owning creation graph is pending; callers must not substitute success. */
WorldServicesResult pursuitManagerCreatePursuit(PursuitManager* manager,
    PursuitCollisionEntry* entry, Car* target, int32_t* created);
WorldServicesResult pursuitManagerMonitorPullovers(PursuitManager* manager);
WorldServicesResult pursuitManagerReclaimPatrollers(PursuitManager* manager);
WorldServicesResult pursuitManagerGetPulloverCount(const PursuitManager* manager, const Car* car, int32_t* count);
WorldServicesResult pursuitManagerSetNextSpeechGap(PursuitManager* manager);
PursuitCollisionGroup* pursuitManagerFindPlayerPursuit(PursuitManager* manager);
WorldServicesResult carRecoveryFind(const PursuitManager* manager,
    const Car* car, PursuitCollisionGroup** group);
WorldServicesResult carRecoveryStatus(const PursuitManager* manager,
    const Car* car, int32_t* hudLevel);
WorldServicesResult pursuitManagerFindPursuitForCar(const PursuitManager* manager,
    const Car* car, PursuitCollisionGroup** group);
WorldServicesResult pursuitGroupReleaseChopper(PursuitManager* manager,
    PursuitCollisionGroup* group);
struct CarMovement* pursuitManagerSelectHornTarget(PursuitManager* manager, PursuitCollisionEntry* entry);
int32_t pursuitManagerRoadBlockCount(const PursuitManager* manager);
PursuitRoadBlock* pursuitManagerRoadBlockAt(const PursuitManager* manager, int32_t index);
/* All references borrow their live canonical owners; source managerA0 was
 * unused by this original method. No missing-owner defaults are supplied. */
float pursuitBaseSpeedTrapSpawnTime(World* world, const struct RaceParameters* parameters,
    const TrackRoute* route, const struct ApplicationLaunchOptions* launch);

struct PursuitManager;
struct RigidBodyInfo;

CarDriverResult pursuitEntrySelectRoad(PursuitCollisionEntry* entry);
CarDriverResult pursuitEntryBuildRoadCommand(PursuitCollisionEntry* entry,DriverCommand* command,
    const float position[2],int32_t keepAuthoredStart);
void pursuitEntryHoldPosition(const PursuitCollisionEntry* entry,DriverCommand* command);
int32_t pursuitEntryNearTarget(const PursuitCollisionEntry* entry);
void pursuitEntryClearSpeedTrap(PursuitCollisionEntry* entry);
CarDriverResult pursuitEntryUpdateMovementMode(PursuitCollisionEntry* entry);
CarDriverResult pursuitEntryInitializeSpeedTrap(PursuitCollisionEntry* entry);
CarDriverResult pursuitEntryInitializeRoadBlock(PursuitCollisionEntry* entry);
CarDriverResult pursuitEntryChangeState(struct PursuitManager* manager, PursuitCollisionEntry* entry,
    int32_t previousState, int32_t newState);
CarDriverResult pursuitExternalEntryThink(struct PursuitManager* manager, PursuitCollisionEntry* entry);
CarDriverResult pursuitEntryDoLimbo(PursuitCollisionEntry* entry);
CarDriverResult pursuitEntryDoDisabled(PursuitCollisionEntry* entry, const World* world);
CarDriverResult pursuitEntryDoRoadBlock(PursuitCollisionEntry* entry, const World* world);
CarDriverResult pursuitEntryDoSpeedTrap(struct PursuitManager* manager, PursuitCollisionEntry* entry);
CarDriverResult pursuitEntryDoPatrol(struct PursuitManager* manager, PursuitCollisionEntry* entry);
CarDriverResult pursuitEntryDoCaughtSpeeder(struct PursuitManager* manager, PursuitCollisionEntry* entry);
void pursuitZonePermissionsInitialize(void);
CarDriverResult pursuitZonePermission(int32_t requested, int32_t occupied, int32_t* allowed);
int32_t pursuitZoneFromDrivingState(int32_t state, float side);
CarDriverResult pursuitEntryFollow(struct PursuitManager* manager, PursuitCollisionEntry* entry,
    int32_t previousState, const struct CarMovement* target);
CarDriverResult pursuitEntryLowSpeedFollow(PursuitCollisionEntry* entry, int32_t previousState,
    const struct CarMovement* target);
CarDriverResult pursuitEntryFlank(struct PursuitManager* manager, PursuitCollisionEntry* entry,
    int32_t previousState, const struct CarMovement* target);
CarDriverResult pursuitEntryBump(struct PursuitManager* manager, PursuitCollisionEntry* entry,
    int32_t previousState, struct CarMovement* target);
CarDriverResult pursuitEntryFollowBackwards(struct PursuitManager* manager, PursuitCollisionEntry* entry,
    const struct CarMovement* target);
CarDriverResult pursuitEntryFollowOffset(PursuitCollisionEntry* entry, int32_t previousState,
    const struct CarMovement* target);
CarDriverResult pursuitEntryBlock(PursuitCollisionEntry* entry, int32_t previousState,
    const struct CarMovement* target);
CarDriverResult pursuitEntryDoHotPursuit(struct PursuitManager* manager, PursuitCollisionEntry* entry);
CarDriverResult pursuitOwnedEntryThink(struct PursuitManager* manager, PursuitCollisionEntry* entry);
/* Native constructor-tail ownership adapter only, not an allocating Car
 * factory. sharedModelBody must come from the sole CarInfoResource owner. */
CarDriverResult pursuitOwnedEntryInitializeAfterCar(struct PursuitManager* manager, PursuitCollisionEntry* entry,
    struct CarMovement* movement, struct RigidBodyInfo* sharedModelBody, int32_t officerIndex);
int32_t pursuitEntryCanSeeTarget(const PursuitCollisionEntry* entry, const struct CarMovement* target);
CarDriverResult pursuitEntryAcquire(PursuitCollisionEntry* entry, int32_t previousState,
    const struct CarMovement* target);
CarDriverResult pursuitEntryStop(PursuitCollisionEntry* entry, int32_t previousState,
    const struct CarMovement* target);
CarDriverResult pursuitEntryGetAhead(PursuitCollisionEntry* entry, int32_t previousState);
CarDriverResult pursuitEntryBestFlankSide(const PursuitCollisionEntry* entry,
    const struct CarMovement* target, float* side);
CarDriverResult pursuitCanTeleport(const World* world, const struct CarMovement* movement,
    const float position[2], int32_t* clear);
CarDriverResult pursuitCarCausedRecentCollision(struct CarMovement* movement,
    struct CarMovement** other);
CarDriverResult pursuitCarsCollided(struct CarMovement* first, const struct CarMovement* second,
    int32_t* collided);
CarDriverResult pursuitEntryTeleport(PursuitCollisionEntry* entry, const float position[2], uint16_t heading);
CarDriverResult pursuitEntryAssignSpeedTrap(PursuitCollisionEntry* entry, PursuitSpeedTrap* trap);
CarDriverResult pursuitEntryAssignRoadBlock(PursuitCollisionEntry* entry, const PursuitRoadBlock* block,
    PursuitCollisionGroup* group, int32_t slot);
CarDriverResult pursuitEntryTargetAllowed(struct PursuitManager* manager, struct CarMovement* target,
    int32_t checkSpeed, int32_t* allowed);

int32_t driverCarInHeadingCone(const VehicleState* observer,const VehicleState* target,
                               uint16_t maximumDifference);

float driverPursuitRecoveryDirection(const PursuitCollisionContext* pursuit);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace hp2 { class CollisionObjectData; class CarInfoResource; extern "C++" { class LanguageManager; } }

WorldServicesResult pursuitCarTopSpeed(const struct CarMovement* movement,
    const hp2::CarInfoResource* resources,float* speed);

/* Checked native access to the existing PAL-only patrol transition helper.
 * No independent beta method identity or additional original body is claimed. */
WorldServicesResult pursuitEntryReturnToPatrol(PursuitCollisionEntry* entry);

/* The manager owns only its ten IDs/timers. The actual objects, resource
 * bodies and signed ID registry belong to the existing collision owners. */
struct PursuitBarricadeManager {
    CollisionSystem* collision;
    TerrainTopology* topology;
    hp2::CollisionObjectData* resources;
    int16_t objectIds[10];
    float remainingSeconds[10];
    WorldServicesResult result;
};

PursuitBarricadeManager* pursuitBarricadesCreate(CollisionSystem* collision,
    TerrainTopology* topology,hp2::CollisionObjectData* resources);
WorldServicesResult pursuitBarricadesDispose(PursuitBarricadeManager* manager);
void pursuitBarricadesInitialize(PursuitBarricadeManager* manager);
WorldServicesResult pursuitBarricadesCreateAt(PursuitBarricadeManager* manager,
    const float position[4],int16_t* objectId);
WorldServicesResult pursuitBarricadesBlink(PursuitBarricadeManager* manager,float seconds);
WorldServicesResult pursuitBarricadesAdvance(PursuitBarricadeManager* manager,float elapsed);

struct PursuitGroupBindings {
    CollisionSystem* collision;
    TerrainTopology* topology;
    hp2::CollisionObjectData* collisionResources;
    const hp2::CarInfoResource* carResources;
    const hp2::LanguageManager* language;
    const TrackRoute* route;
};

/* Native representation of the embedded RoadBlockStateMachine. Its
 * uninitialized original alignment word is omitted rather than copied. */
struct PursuitRoadBlockMachine {
    int32_t state;
    /* Source constructor leaves this timer untouched until activation. */
    float remainingSeconds;
    uint32_t requests;
    int32_t result;
    PursuitCollisionGroup* group;
};

/* One allocation owns this state and the public group as its first member.
 * The manager's ordered and ID-indexed views point to this SAME group. */
struct PursuitGroupState {
    PursuitCollisionGroup group;
    PursuitManager* manager;
    const PursuitGroupBindings* bindings;
    int8_t id;
    int32_t copLimit;
    int32_t allocationMode;
    int32_t speechLevel;
    uint32_t landmark;
    int32_t joinSpeechSent;
    int32_t specialDeclarationSent;
    int32_t calledForBackup;
    float stopConfirmationSeconds;
    float stateRemainingSeconds;
    float roadBlockApproachSeconds;
    float chopperWaitSeconds;
    float nextChopperWaitSeconds;
    float maximumTargetSpeed;
    float chaseDirection;
    float wrongDirectionSeconds;
    float disabledRemainingSeconds;
    PursuitRoadBlockMachine roadBlock;
    int32_t roadBlocksUsed;
    PursuitBarricadeManager* barricades;
};

WorldServicesResult pursuitManagerBindGroupStorage(PursuitManager* manager,
    const PursuitGroupBindings* bindings);
WorldServicesResult pursuitManagerAllocateGroup(PursuitManager* manager,int32_t id,
    struct CarMovement* target,PursuitCollisionGroup** output);
WorldServicesResult pursuitGroupInitialize(PursuitManager* manager,PursuitGroupState* state,
    const PursuitGroupBindings* bindings,int32_t id,struct CarMovement* target);
WorldServicesResult pursuitGroupSwitchState(PursuitManager* manager,
    PursuitCollisionGroup* group,int32_t phase);
WorldServicesResult pursuitGroupReadSpeaker(PursuitManager* manager,
    const PursuitCollisionGroup* group,int32_t* channel);
WorldServicesResult pursuitGroupReadExactDescription(const PursuitCollisionGroup* group,int32_t* description);
WorldServicesResult pursuitGroupReadColour(const PursuitCollisionGroup* group,int32_t* colour);
WorldServicesResult pursuitGroupDisplaySpeed(const PursuitCollisionGroup* group,float* speed);
WorldServicesResult pursuitManagerPossiblyCallSupervisor(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupBackOffSpeech(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupReleaseAll(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupDisbandRoadBlock(PursuitManager* manager,
    PursuitCollisionGroup* group,int32_t considerTarget);
WorldServicesResult pursuitManagerRemoveSpikes(PursuitManager* manager,int32_t selector);
WorldServicesResult pursuitManagerDeleteGroup(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitManagerDisposeGroupStorage(PursuitManager* manager);
WorldServicesResult pursuitManagerNextPursuitId(const PursuitManager* manager,int32_t* id);
WorldServicesResult pursuitGroupDeclareSpeech(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupJoinSpeech(PursuitManager* manager,PursuitCollisionGroup* group);
/* Independent actual data tuning at PAL2EFFE0, initially zero. */
extern int32_t pursuitLimitAiPursuitsToOne;
WorldServicesResult pursuitGroupUpdateDriveTargets(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitManagerChopperAvailable(const PursuitManager* manager,int32_t* available);
WorldServicesResult pursuitManagerAcquireChopper(PursuitManager* manager,struct Chopper** chopper);
WorldServicesResult pursuitGroupUpdateChopper(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupAdvanceState(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* finished);
WorldServicesResult pursuitGroupEarlyCollision(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupRaiseBackupSpeech(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupDeclarePursuitSpeech(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupLevelZeroSpeech(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupSupervisorInitiates(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitManagerActivateSupervisor(PursuitManager* manager);
WorldServicesResult pursuitGroupShouldBackOff(PursuitManager* manager,const PursuitCollisionGroup* group,int32_t* backOff);
WorldServicesResult pursuitGroupAboutToSpeak(const PursuitCollisionGroup* group,int32_t* imminent);
WorldServicesResult pursuitGroupSelectPrimary(PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupActivateRoadBlock(PursuitManager* manager,PursuitCollisionGroup* group);

/* Actual tWordQueue used only by the road-block layout producer/consumer.
 * Thirty storage positions retain the original 29-word usable capacity. */
class PursuitWordQueue final {
public:
    PursuitWordQueue() noexcept;
    bool Store(uint32_t value) noexcept;
    bool Remove(uint32_t& value) noexcept;
    // Native capacity preflight for the original 29-word usable ring.
    // Prevents publishing half a typed road-block command on exhaustion.
    bool CanStore(size_t count) const noexcept;
private:
    uint32_t words_[30];
    uint32_t read_,write_;
};

WorldServicesResult pursuitManagerUpdateCopDimensions(PursuitManager* manager,
    const hp2::CarInfoResource* resources);
float pursuitProjectedCopWidth(const PursuitManager* manager,uint16_t heading);
WorldServicesResult pursuitBuildRoadBlockLayout(PursuitWordQueue& queue,
    const int32_t* elements,size_t elementCount,const PursuitRoadBlock* block,
    const PursuitManager* manager,float spacing);
extern float pursuitRoadBlockFillWidth;
WorldServicesResult pursuitGroupReadSecondarySpeaker(PursuitManager* manager,
    const PursuitCollisionGroup* group,int32_t* channel);
WorldServicesResult pursuitTryFillerSpeech(PursuitManager* manager,PursuitCollisionGroup* group,
    uint32_t* usedBanks,int32_t* spoken);
WorldServicesResult pursuitSayFillerSpeech(PursuitManager* manager,PursuitCollisionGroup* group,uint32_t* usedBanks);
WorldServicesResult pursuitManagerTimeBasedSpeech(PursuitManager* manager);
WorldServicesResult pursuitGroupRoadBlockSpacing(const PursuitCollisionGroup* group,float* spacing);
WorldServicesResult pursuitManagerCalculateRoadBlock(PursuitManager* manager,PursuitRoadBlock* block,
    PursuitCollisionGroup* group,int32_t* carCount);
WorldServicesResult pursuitManagerLoadRoadBlockSites(PursuitManager* manager,const uint8_t* track,size_t size,
    const CarRecoveryPointRegistry* road,TerrainTopology* terrain);
WorldServicesResult pursuitManagerInitializeRoadBlocks(PursuitManager* manager,
    const CarRecoveryPointRegistry* road,TerrainTopology* terrain);
WorldServicesResult pursuitManagerDisposeRoadBlockSites(PursuitManager* manager);
PursuitRoadBlock* pursuitManagerRoadBlockBySite(const PursuitManager* manager,int32_t authoredIndex);
WorldServicesResult pursuitPositionClear(const World* world,const float position[2],int32_t* clear);
WorldServicesResult trackGraphFindRoadBlock(const TrackGraph* graph,PursuitManager* manager,
    Car* car,float minimumDistance,float maximumDistance,int32_t forward,PursuitRoadBlock** result);
WorldServicesResult pursuitManagerFindCopsForRoadBlock(PursuitManager* manager,
    PursuitCollisionEntry* output[2],int32_t requested,int32_t* count);
WorldServicesResult pursuitManagerRequestBlock(PursuitManager* manager,PursuitCollisionGroup* group,
    float direction,PursuitRoadBlock** block);
WorldServicesResult pursuitGroupRoadBlockSettingSpeech(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupCreateRoadBlock(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupUpdateRoadBlockMachine(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitGroupRoadBlockDefeatedSpeech(PursuitManager* manager,PursuitCollisionGroup* group,int32_t through);
WorldServicesResult pursuitGroupHasPassedRoadBlock(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* passed);
WorldServicesResult pursuitManagerAnyCopNear(PursuitManager* manager,struct CarMovement* target,float distance,int32_t* near);
WorldServicesResult pursuitManagerAnyCopContact(PursuitManager* manager,struct CarMovement* target,int32_t* collided);
WorldServicesResult pursuitGroupCanStopTarget(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* stoppable);
WorldServicesResult pursuitGroupRoadBlockDefeated(PursuitManager* manager,PursuitCollisionGroup* group,int32_t through);
/* Native query for the original reserved-officer, speed and 3D-distance gate;
 * the caller retains player score, speech and Replay publication order. */
WorldServicesResult pursuitGroupRoadBlockHighlightEligibility(const PursuitCollisionGroup* group,int32_t* eligible);
WorldServicesResult pursuitGroupUpdateRoadBlocks(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitManagerSelectArrestingOfficer(PursuitManager* manager,
    const PursuitCollisionGroup* group,PursuitCollisionEntry** officer);
WorldServicesResult pursuitManagerRemoveSpeedTrapsForCar(PursuitManager* manager,Car* car);
WorldServicesResult pursuitGroupAiTicketSpeech(PursuitManager* manager,PursuitCollisionGroup* group);
WorldServicesResult pursuitManagerCreatePullover(PursuitManager* manager,
    PursuitCollisionGroup* group,Car* car,int32_t countPullover);
WorldServicesResult pursuitGroupUpdate(PursuitManager* manager,
    PursuitCollisionGroup* group,int32_t* finished);
WorldServicesResult pursuitManagerPursuitManagement(PursuitManager* manager);
int32_t pursuitMotionSegmentsIntersect(const float first[2],const float firstMotion[2],
    const float second[2],const float secondMotion[2]);
WorldServicesResult pursuitManagerMonitorSpikeBelts(PursuitManager* manager);
WorldServicesResult pursuitGroupChangeChasedCar(PursuitManager* manager,
    PursuitCollisionGroup* group,struct CarMovement* target);
WorldServicesResult pursuitExternalEntryChangeChaseTarget(PursuitManager* manager,
    PursuitCollisionEntry* entry);
WorldServicesResult pursuitManagerLoadSpeedTrapSites(PursuitManager* manager,const uint8_t* track,size_t size,
    const CarRecoveryPointRegistry* road,TerrainTopology* terrain);
WorldServicesResult pursuitManagerInitializeSpeedTraps(PursuitManager* manager,
    const CarRecoveryPointRegistry* road,TerrainTopology* terrain);
WorldServicesResult pursuitManagerDisposeSpeedTrapSites(PursuitManager* manager);
PursuitSpeedTrap* pursuitManagerSpeedTrapBySite(const PursuitManager* manager,int32_t authoredIndex);
WorldServicesResult trackGraphFindSpeedTrap(const TrackGraph* graph,PursuitManager* manager,
    Car* car,float minimumDistance,float maximumDistance,int32_t forward,PursuitSpeedTrap** result);
WorldServicesResult pursuitManagerSpawnSpeedTraps(PursuitManager* manager);
WorldServicesResult pursuitManagerThink(PursuitManager* manager);
WorldServicesResult pursuitManagerBeACopThink(PursuitManager* manager);
WorldServicesResult pursuitManagerFindSuitableSpeedTrap(PursuitManager* manager,PursuitSpeedTrap** result);

/* The same manager/pool/catalogue owners are traversed on record and restore.
 * Console pointer bytes never enter the portable replay stream. */
void pursuitManagerDoSnapshot(PursuitManager* manager,ReplaySnapshot* snapshot);
void pursuitManagerSnapshotRoadBlock(PursuitManager* manager,ReplaySnapshot* snapshot,PursuitRoadBlock** block);
void pursuitManagerSnapshotSpeedTrap(PursuitManager* manager,ReplaySnapshot* snapshot,PursuitSpeedTrap** trap);
void pursuitManagerSnapshotGroup(PursuitManager* manager,ReplaySnapshot* snapshot,PursuitCollisionGroup** group);
void pursuitManagerSnapshotCop(PursuitManager* manager,ReplaySnapshot* snapshot,PursuitCollisionEntry** entry);
void pursuitGroupDoSnapshot(PursuitCollisionGroup* group,ReplaySnapshot* snapshot);
int32_t pursuitGroupContainsCop(const PursuitCollisionGroup* group,const PursuitCollisionEntry* entry);
void pursuitEntryBaseDoSnapshot(PursuitManager* manager,PursuitCollisionEntry* entry,ReplaySnapshot* snapshot);
void pursuitPlayerCopDoSnapshot(PursuitManager* manager,PursuitCollisionEntry* entry,ReplaySnapshot* snapshot);
void pursuitAiCopDoSnapshot(PursuitManager* manager,PursuitCollisionEntry* entry,ReplaySnapshot* snapshot);
void pursuitVelocityHistoryDoSnapshot(PursuitVelocityHistory* history,ReplaySnapshot* snapshot);
void pursuitRoadBlockMachineDoSnapshot(PursuitRoadBlockMachine* machine,ReplaySnapshot* snapshot,
    PursuitCollisionGroup* group);
void pursuitBarricadesDoSnapshot(PursuitBarricadeManager* manager,ReplaySnapshot* snapshot);

namespace hp2 {
/* Final CopManager owns six AI entries, one borrowed player entry, six
 * pursuits and ten strips. Track/resource/service inputs remain borrowed. */
struct PursuitConstructionBindings {
    World* world{};
    const GameTime* clock{};
    WorldSimulation* simulation{};
    RaceFrame* race{};
    const RaceParameters* parameters{};
    const ApplicationLaunchOptions* launch{};
    WorldAudioRequestQueue* audio{};
    WorldAudioMessages* messages{};
    WorldSpeechManager* speech{};
    const PursuitGroupBindings* groups{};
    const CarRecoveryPointRegistry* road{};
    TerrainTopology* terrain{};
    const uint8_t* trackBytes{};
    size_t trackSize{};
    WorldObjectRegistry* objects{};
};
PursuitCollisionEntry* PursuitCreateCop(PursuitManager& manager,RaceDriverInfo* driver,bool playerCop);
void PursuitConstructOwnedCop(PursuitManager& manager,PursuitCollisionEntry& entry,
    RaceDriverInfo& driver,int32_t officerIndex);
void PursuitConstructPlayerCop(PursuitCollisionEntry& entry,Car& car,int32_t officerIndex);
}
#endif

#include "indep/inc/CopChase.hpp"

#endif
