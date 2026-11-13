#ifndef NFSHP2_INDEP_INC_AIPLAN_HPP
#define NFSHP2_INDEP_INC_AIPLAN_HPP
#include "indep/inc/DriveTarget.hpp"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum DriverAIActionStatus {
    DRIVER_AI_ACTION_ACTIVE,
    DRIVER_AI_ACTION_SUCCEEDED,
    DRIVER_AI_ACTION_FAILED,
    DRIVER_AI_ACTION_TIMED_OUT
} DriverAIActionStatus;

struct PursuitRoadBlock;
struct CarMovement;

/* The action owns the path proposed by its concrete behavior. Selection
 * priority and behavior kind belong to this owner, not to the driver. */
typedef struct DriverAIAction {
    int32_t kind;
    int32_t priority;
    DriverCommand command;
    int32_t hasDeadline;
    int32_t startedTicks;
    float duration;
    DriverAIActionStatus status;
    int32_t pathValid;
    float targetSpeed;
    float targetDuration;
    float speedTolerance;
    int32_t useRoadSpeed;
    int32_t enabled;
    int32_t affectsRacePace;
    union {
        struct { int32_t lastProbeTicks; } cruise;
        struct { struct CarMovement* target; } directTarget; /* Block/Nudge source+260. */
        struct {
            struct CarMovement* target;
            int32_t targetSelectedTicks;
            float retainedArgument; /* Source+264, constructor f13; no gameplay consumer. */
        } avoidance;
        struct {
            struct CarMovement* target;
            int32_t pathClear;
            int32_t lastReplanTicks;
            int32_t expiryReferenceTicks;
            float expirySeconds;
            float expiryBaseSeconds; /* Source+264; producer constructor f13. */
            int32_t followTarget;
        } collisionAvoidance;
        struct {
            int32_t createdTicks;
            float requestedSeconds;
            int32_t hasRequestedDuration;
        } recovery;
        struct { struct PursuitRoadBlock* block; } roadBlock;
        struct {
            struct CarMovement* target;
            int32_t lastReplanTicks;
        } contactResponse;
        struct {
            int32_t createdTicks;
            int32_t recoveryRequested;
        } airborne;
        struct {
            int32_t createdTicks;
            int32_t lastProbeTicks;
            int32_t collisionPathClear;
            int32_t rightMarginAvailable;
            float stopDelaySeconds;
        } yielding;
    } behavior;
} DriverAIAction;

static inline int32_t driverAIActionKind(const DriverAIAction* action)
{
    return action->kind;
}

DriverAIAction* driverAIActionInitialize(DriverAIAction* action,int32_t priority,float duration,int32_t nowTicks);
void driverAIActionSetTimeout(DriverAIAction* action,float duration,int32_t nowTicks);
int32_t driverAIActionStatus(const DriverAIAction* action);




int32_t driverAIActionDeadlinePassed(const DriverAIAction* action,int32_t nowTicks);
int32_t driverAIActionFinished(const DriverAIAction* action);
struct DriverAIBase;
struct DriverAIPerception;
DriverAIAction* driverAIActionInitializeCruise(DriverAIAction* action,int32_t priority,float duration,int32_t nowTicks);
void driverAIActionPlanCruise(DriverAIAction* action,struct DriverAIBase* driver,
    struct DriverAIPerception* perception);


struct DriverAIRoadState;
struct ReplaySnapshot;
int32_t driverAIActionActive(const DriverAIAction* action);
int32_t driverAIActionFailed(const DriverAIAction* action);
int32_t driverAIActionSucceeded(const DriverAIAction* action);
int32_t driverAIActionTimedOut(const DriverAIAction* action);
DriverAIAction* driverAIActionInitializeAvoidance(DriverAIAction* action,int32_t priority,
    float duration,struct CarMovement* target,float retainedArgument,int32_t nowTicks);
DriverAIAction* driverAIActionInitializeCollisionAvoidance(DriverAIAction* action,
    int32_t priority,float duration,struct CarMovement* target,float expiryBaseSeconds,
    int32_t pathClear,int32_t followTarget,int32_t nowTicks);
DriverAIAction* driverAIActionInitializeRoadFollowing(DriverAIAction* action,
    int32_t priority,float duration,int32_t nowTicks);
DriverAIAction* driverAIActionInitializeNudge(DriverAIAction* action,int32_t priority,
    float duration,struct CarMovement* target,int32_t nowTicks);
DriverAIAction* driverAIActionInitializeBlock(DriverAIAction* action,int32_t priority,
    float duration,struct CarMovement* target,int32_t nowTicks);
DriverAIAction* driverAIActionInitializeYielding(DriverAIAction* action,int32_t priority,float duration,
    const struct DriverAIPerception* perception,const struct DriverAIRoadState* road,
    float stopDelaySeconds,int32_t nowTicks);
DriverAIAction* driverAIActionInitializeRecovery(DriverAIAction* action,int32_t priority,
    float duration,float requestedSeconds,int32_t nowTicks);
DriverAIAction* driverAIActionInitializeRoadBlock(DriverAIAction* action,int32_t priority,
    float duration,struct PursuitRoadBlock* block,int32_t nowTicks);
DriverAIAction* driverAIActionInitializeContact(DriverAIAction* action,int32_t priority,
    float duration,struct CarMovement* target,int32_t nowTicks);
DriverAIAction* driverAIActionInitializeAirborne(DriverAIAction* action,int32_t priority,
    float duration,int32_t nowTicks);
void driverAIActionSnapshot(struct DriverAIBase* driver,struct ReplaySnapshot* snapshot);

#ifdef __cplusplus
}
#endif
#include "indep/inc/AICarController.hpp"
#ifdef __cplusplus
extern "C" {
#endif

void driverAIActionUpdateStatus(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);
void driverAIActionBuildPlan(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif
#include "indep/inc/Collision.hpp"
#ifdef __cplusplus
extern "C" {
#endif

void driverAIActionAccelerateFor(DriverAIAction* action,const DriverPhysicsVehicle* vehicle,float seconds);
int32_t driverAIActionRoadBoundary(const DriverAIRoadState* road,const DriverAIBase* driver,
    int32_t considerTargetPoint);
int32_t driverAIActionContactSide(const CollisionImpact* impact,const DriverPhysicsVehicle* vehicle);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

DriverAIAction* driverAIActionInitializeAirborne(DriverAIAction* action,int32_t priority,
    float duration,int32_t nowTicks);
int32_t driverAIActionFacesGround(CarMovement* movement,float threshold);
int32_t driverAIActionAirbornePriority(DriverAIPerception* perception);
int32_t driverAIActionAirborneComplete(const DriverAIAction* action,DriverAIPerception* perception);
void driverAIActionPlanAirborne(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

int32_t driverAIActionAvoidanceTargetAllowed(const DriverAINeighbor* neighbor,
    const DriverAIGoal* goal,int32_t followTarget);
int32_t driverAIActionAvoidancePriority(DriverAIBase* driver,DriverAIPerception* perception,
    CarMovement** target,DriverAIGoal* goal);
int32_t driverAIActionAvoidanceComplete(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);
int32_t driverAIActionAvoidanceFailed(const DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);
void driverAIActionPlanAvoidance(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

void driverAIActionMatchNeighborSpeed(DriverAIAction* action,const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception,float speedReduction);
DriverAIAction* driverAIActionInitializeCollisionAvoidance(DriverAIAction* action,
    int32_t priority,float duration,CarMovement* target,float expiryBaseSeconds,
    int32_t pathClear,int32_t followTarget,int32_t nowTicks);
int32_t driverAIActionCollisionAvoidancePriority(DriverAIBase* driver,DriverAIPerception* perception,
    CarMovement** target,DriverAIGoal* goal,float* collisionTime,int32_t* pathClear,int32_t* followTarget);
int32_t driverAIActionCollisionAvoidanceComplete(const DriverAIAction* action,
    DriverAIPerception* perception,int32_t nowTicks);
void driverAIActionPlanCollisionAvoidance(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

DriverAIAction* driverAIActionInitializeContact(DriverAIAction* action,int32_t priority,
    float duration,CarMovement* target,int32_t nowTicks);
int32_t driverAIActionContactPriority(DriverAIBase* driver,DriverAIPerception* perception,
    CarMovement** target);
int32_t driverAIActionContactComplete(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);
void driverAIActionPlanContact(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

DriverAIAction* driverAIActionInitializeRecovery(DriverAIAction* action,int32_t priority,
    float duration,float requestedSeconds,int32_t nowTicks);
int32_t driverAIActionRecoveryBlocked(const DriverAIBase* driver,const DriverAIPerception* perception);
int32_t driverAIActionRecoveryPriority(DriverAIBase* driver,DriverAIPerception* perception);
const CarRecoveryPose* driverAIActionRecoveryGoalPose(const DriverAIRoadState* road);
CarDriverResult driverAIActionRecoveryGoalXY(const DriverAIRoadState* road,float output[2]);
CarDriverResult driverAIActionRecoveryGoalPosition(const DriverAIRoadState* road,float output[4]);
int32_t driverAIActionRecoveryComplete(const DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);
int32_t driverAIActionRecoveryTimedOut(const DriverAIAction* action,DriverAIBase* driver);
int32_t driverAIActionRecoveryFailed(const DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);
void driverAIActionPlanRecovery(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

DriverAIAction* driverAIActionInitializeRoadFollowing(DriverAIAction* action,
    int32_t priority,float duration,int32_t nowTicks);
int32_t driverAIActionRoadFollowingPriority(DriverAIPerception* perception,const CarMovement* target);
void driverAIActionPlanRoadFollowing(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif
#include "indep/inc/DrivingChoice.hpp"
#include "indep/inc/Pursuit.hpp"
#ifdef __cplusplus
extern "C" {
#endif

DriverAIAction* driverAIActionInitializeRoadBlock(DriverAIAction* action,int32_t priority,
    float duration,PursuitRoadBlock* block,int32_t nowTicks);
int32_t driverAIActionRoadBlockPriority(DriverAIBase* driver,DriverAIPerception* perception,
    PursuitRoadBlock** block);
int32_t driverAIActionRoadBlockComplete(const DriverAIAction* action,
    const DriverAIPerception* perception);

void driverAIActionPlanRoadBlock(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

void driverAIActionBrakeFor(DriverAIAction* action,const DriverPhysicsVehicle* vehicle,float seconds);
float driverAIActionRightMargin(const DriverAIPerception* perception,const CarRecoveryPose* pose);
DriverAIAction* driverAIActionInitializeYielding(DriverAIAction* action,int32_t priority,float duration,
    const DriverAIPerception* perception,const DriverAIRoadState* road,float stopDelaySeconds,int32_t nowTicks);
int32_t driverAIActionYieldingPriority(DriverAIBase* driver);
int32_t driverAIActionYieldingComplete(DriverAIBase* driver);
void driverAIActionPlanYielding(DriverAIAction* action,DriverAIBase* driver,DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif

#endif
