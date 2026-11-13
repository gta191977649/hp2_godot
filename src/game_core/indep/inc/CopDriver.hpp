#ifndef NFSHP2_COPDRIVER_HPP
#define NFSHP2_COPDRIVER_HPP

// Native declarations use the PC runtime layout.
#include "TrackedController.hpp"
#include "indep/inc/DriveTarget.hpp"
#include "indep/inc/Pursuit.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct CarMovement;
typedef struct CopMover {
    struct CarMovement* movement;
    float velocity[4];
    float lateralAcceleration[2];
    uint16_t heading;
    uint16_t targetHeading;
    float speed;
    float targetSpeed;
    float speedLimit;
    float brakingLimit;
    float accelerationLimit;
    float brakingBlend;
    float targetVelocity[4];
    float groundNormal[4];
    int32_t airborne;
    int8_t mode;
} CopMover;

/* Only the original police model codes have an acceleration definition. */
int32_t copMoverInitialize(CopMover* mover, struct CarMovement* movement);
void copMoverSetAccelerationLimit(CopMover* mover, int32_t modelCode);
float copMoverUpdateHeading(CopMover* mover, float maximumSpeed, float elapsed);
float copMoverMove(CopMover* mover, float elapsed);


int32_t copDriverLookAhead(float output[2],float* lateralError,const DriverCommand* command,
    const float position[2],const float offset[2],float distance);
/* The source contract requires at least one command point. */
void copDriverPointFromEnd(float output[2],const DriverCommand* command,const float offset[2]);


struct CopDriver {
    DriverRoute base;
    DriverCommand command;
    PursuitCollisionContext* pursuit;
    int32_t drivingMode;
    int32_t previousMode;
    int32_t commandFromTarget;
    float stalledSeconds;
    float reverseSeconds;
    float targetSpeed;
    int32_t entryIndex;
    float lookAhead[2];
    int32_t routeClassLimit;
};

CarDriverResult copDriverInitialize(CopDriver* driver,CarMovement* movement,PursuitCollisionContext* pursuit);
void copDriverThink(CopDriver* driver,float elapsed);
struct ReplaySnapshot;
void copMoverSnapshot(struct CopMover* mover,struct ReplaySnapshot* snapshot);
void copDriverSnapshot(CopDriver* driver,struct ReplaySnapshot* snapshot);
int32_t copDriverCheckStall(CopDriver* driver,float elapsed);
void copDriverSelectTargetPoint(CopDriver* driver,float output[2]);

#ifdef __cplusplus
}
#endif
#endif
