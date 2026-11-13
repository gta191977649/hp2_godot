#ifndef NFSHP2_INDEP_INC_DRIVINGCHOICE_HPP
#define NFSHP2_INDEP_INC_DRIVINGCHOICE_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct DriverAIGoal;
struct World;
struct ReplaySnapshot;
void driverAIGoalSnapshot(struct DriverAIGoal* goal,const struct World* world,struct ReplaySnapshot* snapshot);
int32_t driverAIGoalUsesRoad(int32_t kind);
int32_t driverAIGoalUsesTarget(int32_t kind);

#ifdef __cplusplus
}
#endif
#include "indep/inc/AICarController.hpp"
#ifdef __cplusplus
extern "C" {
#endif
void driverAIGoalInitialize(DriverAIGoal* goal);
void driverAIGoalSet(DriverAIGoal* goal,int32_t kind,CarMovement* target,int32_t parameter);


int32_t driverAILaneForClass(int32_t selector,int32_t vehicleClass);
#ifdef __cplusplus
}
#endif

#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/DriveTarget.hpp"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct DriverLanePoint {
    CarRecoverySelection road;
    int32_t lane;
    float lateralDelta;
    int32_t changedConnection;
} DriverLanePoint;

typedef struct DriverLanePoints {
    int32_t count;
    DriverLanePoint points[DRIVER_COMMAND_CAPACITY];
} DriverLanePoints;

int32_t driverLaneGoalAllowed(const DriverAIGoal* goal,const DriverAIRoadState* road,
    DriverAIPerception* perception,int32_t vehicleClass);
int32_t driverLaneGoalsMatch(const DriverAIGoal* first,const DriverAIGoal* second);
int32_t driverLaneSelectorForIndex(int32_t lane);
void driverLaneAdjustLateral(float* lateral,int32_t mode,const CarRecoveryPose* pose,
    int32_t lane,int32_t keepInside,float maximumStep,float halfExtent);
float driverLaneLateralStep(const DriverAIBase* driver,const DriverAIRoadState* road,int32_t mode);
int32_t driverLaneAdvancePoint(DriverAIBase* driver,CarRecoverySelection* road,int32_t* lane,
    float* lateralDelta,int32_t* changedConnection);
CarDriverResult driverLaneBuildPoints(DriverLanePoints* points,const DriverCommand* command,
    const DriverAIRoadState* road,const DriverAIPerception* perception,DriverAIBase* driver,float* lateral);
void driverLaneAppendCommandPoint(DriverCommand* command,const DriverLanePoint* point,float lateral);
int32_t driverLaneNearestFollowingPoint(const DriverLanePoints* points,const float position[2],int32_t first);
float driverLanePassingMargin(const DriverAIBase* driver,const DriverAIPerception* perception,
    const DriverAINeighbor* neighbor,const DriverLanePoints* points,int32_t nearest);

CarDriverResult driverLaneBuildRoad(DriverCommand* command,const DriverAIRoadState* road,
    DriverAIPerception* perception,int32_t mode,int32_t keepInside,DriverAIBase* driver);
/* Complete road branches of the shared planner. Actor goals use a separate path. */
int32_t driverLanePlanRoadGoal(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,int32_t mode);
int32_t driverLaneEvaluateRoadGoal(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,int32_t mode,float horizon,
    float* speed,CarMovement** collision,float* time);
void driverLaneAppendTransition(const DriverLanePoints* points,DriverCommand* command,
    const DriverAIPerception* perception,int32_t targetIndex,int32_t keepInside,
    float initialLateral,float finalLateral);
CarDriverResult driverLaneContinueTargetCommand(const DriverAIGoal* goal,DriverCommand* command,
    DriverAIPerception* perception,const DriverLanePoints* points,float acrossOffset);
int32_t driverLaneBuildTargetPath(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver);
int32_t driverLanePlanGoal(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,int32_t mode);
int32_t driverLaneEvaluateGoal(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,
    int32_t mode,int32_t followTarget,float horizon,float* speed,CarMovement** collision,float* time);
float driverLaneRoadCurvature(const DriverAIRoadState* road);
int32_t driverLaneTargetHasRoom(const DriverAIGoal* goal,const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception);
int32_t driverLanePreferredPassingSide(const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception,const DriverAIRoadState* road);
CarDriverResult driverLanePublishTarget(const DriverAIGoal* goal,DriverAIBase* driver,
    int32_t enabled,DriverAIPerception* perception);

#ifdef __cplusplus
}
#endif



#ifdef __cplusplus
extern "C" {
#endif

/* One planning pass owns these candidates. Persistent commands remain with
 * their concrete input/action owners. */
typedef struct DriverLaneSearch {
    DriverCommand fallbackCommand;
    DriverCommand bestCommand;
    DriverCommand candidateCommand;
    DriverAIGoal bestGoal;
    DriverAIGoal fallbackGoal;
    int32_t accepted;
    int32_t followTarget;
    float bestSpeed;
    float fallbackTime;
    CarMovement* collision;
    float collisionTime;
} DriverLaneSearch;

int32_t driverLaneConsiderGoal(const DriverAIGoal* goal,int32_t followTarget,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,
    DriverLaneSearch* search,CarMovement* previousCollision,float horizon);
int32_t driverLaneSelectGoal(DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,
    int32_t* followTarget,int32_t tryCurrent,float horizon);
int32_t driverLaneAvoidNeighbor(DriverAIGoal* goal,DriverCommand* command,int32_t* followTarget,
    const DriverAINeighbor* neighbor,const DriverAIRoadState* road,DriverAIPerception* perception,
    DriverAIBase* driver,int32_t tryCurrent,float horizon);

#ifdef __cplusplus
}
#endif
#include "indep/inc/TrackRoute.hpp"
#include "bware/inc/bMath.hpp"
static inline float driverLaneGeometryAuthoredLateral(const CarRecoveryPose* pose,int32_t lane)
{
    return (float)((int32_t)pose->laneOffsetsQuantized[lane]*256)*0.0000152587890625f;
}


#ifdef __cplusplus
extern "C" {
#endif
CarDriverResult driverLaneBuildPointRange(DriverLanePoints* points,const DriverAIRoadState* road,
    const CarRecoveryPose* last);
#ifdef __cplusplus
}
#endif
#endif
