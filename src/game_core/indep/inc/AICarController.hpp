#ifndef NFSHP2_INDEP_INC_AICARCONTROLLER_HPP
#define NFSHP2_INDEP_INC_AICARCONTROLLER_HPP

#include <stddef.h>
#include "indep/inc/CarController.hpp"
#include <stdint.h>

typedef struct DriverAIPerception DriverAIPerception;
typedef struct DriverAIBase DriverAIBase;

#ifdef __cplusplus
extern "C" {
#endif

void driverPerceptionInitialize(DriverAIPerception* perception);
void driverAIForceBoost(DriverAIBase* driver, float performanceScale);

#ifdef __cplusplus
}
#endif
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/CarInfo.hpp"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct DriverPhysicsTuning {
    float planningSeconds;
    float minimumPlanningDistance;
    float trafficDistanceScale;
    float accelerationScale;
    float predictionSeconds;
    float gravity;
    float accelerationSpeedThreshold;
    float milesPerHourToMetersPerSecond;
} DriverPhysicsTuning;

extern const DriverPhysicsTuning driverPhysicsDefaultTuning;

/* Configuration and movement remain with their actual owners. Authored
 * scalars are immutable; velocity, collision bounds, controls and player
 * state are always read through movement rather than copied into a driver. */
typedef struct DriverPhysicsVehicle {
    CarMovement* movement;
    const CarPerformance* performance;
    const DriverPhysicsTuning* tuning;
    float steeringRangeDegrees;
    int32_t routeVehicleClass;
} DriverPhysicsVehicle;

float driverPlanningDistance(const DriverPhysicsVehicle* vehicle);
void driverCollisionBounds(const DriverPhysicsVehicle* vehicle,
                            float minimum[4], float maximum[4]);
float driverObservedAcceleration(const DriverPhysicsVehicle* vehicle);
float driverBrakingAcceleration(const DriverPhysicsVehicle* vehicle, float speed);
float driverAvailableAcceleration(const DriverPhysicsVehicle* vehicle, float speed);

#ifdef __cplusplus
}
#endif
#include "indep/inc/CarMover.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#ifdef __cplusplus
extern "C" {
#endif

/* One live configuration owner for the original global AI options. Session
 * state, race settings and vehicle performance remain separate owners. */
typedef struct DriverAIPolicy {
    int32_t disableRacePace;
    int32_t fixedRoadSpeed;
    /* Source SkipFE (GP-5AB4) is shared with Pursuit. */
    const ApplicationLaunchOptions* launch;
    int32_t raceVariantMode;
    int32_t beltMovementEnabled;
} DriverAIPolicy;

extern const DriverAIPolicy driverAIDefaultPolicy;

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct AIControls;
struct WorldSimulation;
struct DriverAIAction;
struct DriverBehavior;
struct TrackScenarioDefinition;
struct CarFramePlayerState;

typedef struct DriverAIRoadState {
    const CarRecoveryGroup* group;
    /* The recovery view contains this road record's only pose pointer.
     * Its route-controller timing fields have no AI meaning. */
    CarMovementDriver recovery;
    int32_t lane;
    int32_t selectedTicks;
} DriverAIRoadState;

typedef struct DriverAIGoal {
    int32_t kind;
    CarMovement* target;
    int32_t parameter;
} DriverAIGoal;

typedef struct DriverAINeighbor {
    float localPosition[2];
    float travelRelativeVelocity[2];
    float effectiveHalfExtents[2];
    CarMovement* movement;
} DriverAINeighbor;

enum { DRIVER_AI_NEIGHBORS=16 };
typedef struct DriverAIPerception {
    CarMovement* movement;
    int32_t count;
    DriverAINeighbor neighbors[DRIVER_AI_NEIGHBORS];
    float lateralExtent;
    float forwardExtent;
    DriverAIRoadState road;
    DriverAIGoal goal;
} DriverAIPerception;

/* This borrows the same aligned car/frame/driver table consumed by World.
 * Neighbors retain owner references; vehicle state is never mirrored. */
typedef struct DriverAIEnvironment {
    struct WorldSimulation* simulation;
    const struct DriverAITuning* tuning;
    const struct DriverBehavior* behavior;
    const struct TrackScenarioDefinition* scenario;
    const int32_t* specialPursuit;
    const DriverAIPolicy* policy;
} DriverAIEnvironment;

typedef struct DriverAITuning {
    int32_t laneCandidateCount;
    float connectionDelay;
    float lateralStepSpeed;
    float lateralStepFastSpeed;
    int32_t predictionWideBounds;
    float laneCollisionHorizon;
    float stationaryPassSpeed;
    float offroadStationaryPassSpeed;
    int32_t defaultMovementMode;
    float collisionReplanDelay;
    float collisionRestoreDelay;
} DriverAITuning;

extern const DriverAITuning driverAIDefaultTuning;

typedef struct DriverAIConnectionChoice {
    int32_t active;
    int32_t accepted;
    int32_t lane;
    uint8_t connectionIndex;
    uint16_t snapshotReservedBits; // Source connection word bits16..31 retained by Replay.
} DriverAIConnectionChoice;

typedef struct DriverAIBase {
    CarController controller;
    DriverPhysicsVehicle physics;
    DriverAIEnvironment* environment;
    struct AIControls* controls;
    DriverAIPerception perception;
    DriverAIRoadState* currentRoad;
    DriverAIGoal* currentGoal;
    DriverAIPerception* activePerception;
    struct DriverAIAction* action;
    CarDriverKind kind;
    union {
        AIDriver* ai;
        TrafficDriver* traffic;
    } owner;
    CarDriverResult result;
    int32_t previousGroupId;
    float thinkDelay;
    int32_t actionCode;
    int32_t replanDelayTicks;
    int32_t waitIntervalTicks;
    int32_t waitStartTicks;
    int32_t waitMoving;
    int32_t restoreModeDelayTicks;
    int32_t progressSampleTicks;
    DriverHistory progressHistory;
    BeltMoverBindings beltBindings;
    float stopDelaySeconds;
    DriverAIConnectionChoice connectionChoices[8];
    int32_t nextConnectionIndex;
    int32_t selectedConnectionIndex;
    CarMovement* planningTarget;
    int32_t connectionDelayTicks;
    int32_t speedOffsetSampleTicks;
    int32_t speedSamplePointOffset;
    int32_t performanceSampleTicks;
    int32_t paceEnabled;
    struct CarFramePlayerState* pacePlayer;
    int32_t lastRecoveryActionTicks;
    int32_t linkedPlayerRemainingTicks;
    int32_t linkedPlayerCooldownTicks;
    /* Replay-owned retained cells.
     * They are distinct from planningTarget and progressHistory: final
     * +24C/+258/+25C, the named DeltaLineSpeed Average at+2B4, and+2C0.
     * The original code only samples/restores these retained cells. */
    Car* retainedSnapshotTarget;
    int32_t retainedSnapshotCounter;
    float retainedSnapshotScalar;
    DriverHistory deltaLineSpeedHistory;
    float retainedSnapshotPoint[4];
} DriverAIBase;

void driverAIReleaseBoost(DriverAIBase* driver);
struct ReplaySnapshot;
void driverAISnapshot(DriverAIBase* driver,struct ReplaySnapshot* snapshot);
void driverPerceptionSnapshot(DriverAIPerception* perception,const CarRecoveryPointRegistry* registry,const World* world,struct ReplaySnapshot* snapshot);

const CarRecoveryGroup* driverAICurrentGroup(DriverAIBase* driver,int32_t refreshIfMissing);
const CarRecoveryPose* driverAICurrentPose(DriverAIBase* driver,int32_t refreshIfMissing);
void driverAIRefreshRoad(DriverAIBase* driver);
void driverAISetStopDelay(DriverAIBase* driver,float seconds);
void driverAISetRoad(DriverAIBase* driver,const CarRecoveryGroup* group,const CarRecoveryPose* pose);
void driverAIFindRoad(DriverAIBase* driver,int32_t firstGroup,int32_t lastGroup);
float driverAILateralExcess(const DriverAIBase* driver);
void driverAIUpdateRoad(DriverAIBase* driver);
void driverAIThink(DriverAIBase* driver,float elapsed);
/* Actual post-action mover policy, including withdrawal of belt admission.
 * A failed real mover transition is retained in driver->result. */
void driverAIUpdateMovementMode(DriverAIBase* driver);
int32_t driverAIChooseLane(DriverAIBase* driver,const CarRecoveryGroup* group,const CarRecoveryPose* pose,
    int32_t lockSelection,int32_t* lane,const DriverAIRoadState* diagnosticRoad);

#ifdef __cplusplus
}
#endif
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/PsychoDriver.hpp"
#ifdef __cplusplus
extern "C" {
#endif

/* Concrete AI owns its decision, road and input state. Accessors may refresh
 * the actual road selection when refreshIfMissing is nonzero. */
struct AIDriver {
    DriverAIBase base;
    AIControls input;
    CarMovement* selectedTarget;
    int32_t selectedTargetTicks;
    int32_t progressSampleTicks;
    DriverHistory progressHistory;
    CarDriverResult result;
};

CarDriverResult aiDriverInitialize(AIDriver* driver,CarMovement* movement,
    const DriverPhysicsVehicle* physics,DriverAIEnvironment* environment,
    const GameTime* clock,struct CarSoundState* sound);
void aiDriverDispose(AIDriver* driver);

const CarRecoveryGroup* aiDriverCurrentGroup(AIDriver* driver, int32_t refreshIfMissing);
const CarRecoveryPose* aiDriverCurrentPose(AIDriver* driver, int32_t refreshIfMissing);
CarDriverResult aiDriverGetResult(const AIDriver* driver);
void aiDriverThink(AIDriver* driver, float elapsed);
void aiDriverSnapshot(AIDriver* driver,struct ReplaySnapshot* snapshot);
void aiDriverUpdateControls(AIDriver* driver, float elapsed);
int32_t aiDriverHornActive(const AIDriver* driver);
CarDriverResult aiDriverSetHorn(AIDriver* driver, int32_t enabled);
void aiDriverSetHandbrakeTime(AIDriver* driver, float seconds);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct ReplaySnapshot;
void aiDriverGetRouteNumbersToUse(int32_t* first,int32_t* last);
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif
struct RaceCarEntry;
void driverAICompareRacePace(DriverAIBase* driver,const struct RaceCarEntry* own,
    const struct RaceCarEntry* player,float* positionResponse,float* speedResponse);
void driverAIUpdateRacePace(DriverAIBase* driver);
#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif

int32_t aiDriverAcceptTarget(AIDriver* driver,CarMovement* target);

#ifdef __cplusplus
}
#endif
#include "indep/inc/AIPlan.hpp"
#ifdef __cplusplus
extern "C" {
#endif

void driverAIReactToTarget(DriverAIBase* driver,CarMovement* target,
    const DriverAIPerception* perception);
/* The caller owns a returned action. A null result may mean that the
 * original selection rejected it; driver->result distinguishes errors. */
DriverAIAction* aiDriverCreateAction(AIDriver* driver,int32_t kind);
DriverAIAction* driverAISelectAction(DriverAIBase* driver);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

CarDriverResult driverAIInitialize(DriverAIBase* driver,CarMovement* movement,
    const DriverPhysicsVehicle* physics,DriverAIEnvironment* environment,AIControls* controls,
    CarDriverKind kind);
void driverAIDispose(DriverAIBase* driver);
void driverAIBindBelt(DriverAIBase* driver);
void driverAIResetAfterPlacement(DriverAIBase* driver);

#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif

void driverAIApplyAction(DriverAIBase* driver);

#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif

void driverAIConnectionsClear(DriverAIBase* driver);
void driverAIConnectionsAdvanceCursor(DriverAIBase* driver);
int32_t driverAIConnectionsFreeSlot(const DriverAIBase* driver);
void driverAIPointAlongHeading(DriverAIBase* driver);
int32_t driverAISelectConnection(DriverAIBase* driver,const CarRecoveryConnection* connection,
    int32_t* lane,const DriverAIRoadState* road);
void driverAIConnectionsUpdate(DriverAIBase* driver);

#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif

void driverAIPlanCurrentAction(DriverAIBase* driver);

#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif

void driverAIOnCollision(DriverAIBase* driver,const CarMovement* other);

#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif

int32_t driverAICanUseBelt(const DriverAIBase* driver);

#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif


void driverPerceptionReset(DriverAIPerception* perception,CarDriverKind kind);
int32_t driverPerceptionFindIndex(const DriverAIPerception* perception,const CarMovement* movement);
DriverAINeighbor* driverPerceptionFind(DriverAIPerception* perception,const CarMovement* movement);
DriverAINeighbor* driverPerceptionPrevious(DriverAIPerception* perception,const CarMovement* movement);
int32_t driverPerceptionNeighborCanReach(const DriverAINeighbor* neighbor,
    const DriverPhysicsVehicle* vehicle,float acceleration);
CarDriverResult driverPerceptionUpdate(DriverAIPerception* perception,
    const DriverPhysicsVehicle* vehicle,const DriverAIEnvironment* environment,int32_t enabled);

#ifdef __cplusplus
}
#endif
#include "CarState.hpp"
#ifdef __cplusplus
extern "C" {
#endif

float* driverPointAlongHeading(float output[4],const VehicleState* state,float distance);
float* driverPointAlongVelocity(float output[4],const VehicleState* state,float distance);

#ifdef __cplusplus
}
#endif
#include "indep/inc/DrivingLocation.hpp"
#include "indep/inc/DrivingChoice.hpp"

/* Route search is owned by aiutil.cpp. These native public views borrow
 * the TrackRoute registry and preserve the original twenty-entry history. */
#ifdef __cplusplus
extern "C" {
#endif

#define CAR_RECOVERY_PATH_GROUPS 20

typedef struct CarRecoveryGroupHistory {
    const CarRecoveryGroup* groups[CAR_RECOVERY_PATH_GROUPS];
    int32_t first;
    int32_t last;
    int32_t full;
} CarRecoveryGroupHistory;

const CarRecoveryPose* carRecoveryAdjacentPose(const CarRecoveryGroup* group,
                                              const CarRecoveryPose* pose, int32_t forward);
int32_t carRecoveryAdvanceToConnection(CarRecoverySelection* selection, int32_t forward);
int32_t carRecoveryFindPathGroups(const CarRecoveryPointRegistry* registry,
                                 const CarRecoverySelection* current,
                                 const CarRecoveryGroup* destination,
                                 CarRecoveryGroupHistory* history,
                                 int32_t forward, int32_t avoidRestricted);
int32_t carRecoveryBuildPath(const CarRecoveryPointRegistry* registry,
                            const CarRecoverySelection* current,
                            const CarRecoverySelection* destination,
                            CarRecoverySelection* output, uint32_t capacity,
                            int32_t forward, int32_t avoidRestricted);

#ifdef __cplusplus
}
#endif
#endif
