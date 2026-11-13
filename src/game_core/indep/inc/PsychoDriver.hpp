#ifndef NFSHP2_INDEP_INC_PSYCHODRIVER_HPP
#define NFSHP2_INDEP_INC_PSYCHODRIVER_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/CarMover.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/DriveTarget.hpp"
#include "indep/inc/Table.hpp"

typedef struct AIControls AIControls;

#ifdef __cplusplus
extern "C" {
#endif


// Native public owner views, preserving the existing host interface.
// Original inline path and each implementation owner are qualified separately.
struct CarSoundState;
typedef struct AIControlsBindings {
    CarMovement* movement;
    const DriverPhysicsVehicle* vehicle;
    struct CarSoundState* sound;
    const GameTime* clock;
} AIControlsBindings;

/* Commands and filters belong to this input owner. Output controls remain
 * the same borrowed storage consumed by the active physical mover. */
typedef struct AIControls {
    AIControlsBindings bindings;
    DriverCommand command;
    DriverFilter headingError;
    DriverFilter velocityHeadingError;
    DriverFilter speedError;
    float targetPoint[2];
    float lookAheadScale;
    float targetSpeed;
    float targetDuration;
    float speedTolerance;
    int32_t enabled;
    float forcedHandbrakeTime;
    int32_t reverseHysteresis;
    CarDriverResult result;
} AIControls;

CarDriverResult aiControlsInitialize(AIControls* controls,const AIControlsBindings* bindings);
void aiControlsDispose(AIControls* controls);
void aiControlsUpdate(AIControls* controls,float elapsed);
float aiControlsWrappedAngleRadians(uint16_t angle);
uint16_t aiControlsMeasureAngles(const AIControls* controls,float* headingError,float* velocityError);
void aiControlsClearAngleFilters(AIControls* controls);
void aiControlsResetAngleFilters(AIControls* controls);
void aiControlsClearSpeedFilter(AIControls* controls);
void aiControlsResetSpeedFilter(AIControls* controls);
int32_t aiControlsHandlingMode(const AIControls* controls);
void aiControlsUpdateTargetPoint(AIControls* controls);
void aiControlsComputePedals(const AIControls* controls,float* throttle,float* brake);
void aiControlsSetTarget(AIControls* controls,int32_t enabled,float speed,float duration,float tolerance);
void aiControlsSetCommand(AIControls* controls,const DriverCommand* command);





void aiControlsSetHandbrakeTime(AIControls* controls,float seconds);


void aiControlsStartHorn(AIControls* controls);
void aiControlsStopHorn(AIControls* controls);
int32_t aiControlsHornActive(const AIControls* controls);


struct ReplaySnapshot;
void aiControlsSnapshot(AIControls* controls,struct ReplaySnapshot* snapshot);
void aiControlsFullThrottle(AIControls* controls);
void aiControlsNeutralGear(AIControls* controls);
float aiControlsTargetDuration(const AIControls* controls);
float aiControlsTargetSpeed(const AIControls* controls);
void aiControlsSetLookAheadScale(AIControls* controls,float scale);
const float* aiControlsTargetPoint(const AIControls* controls);
void aiControlsSetTargetPoint(AIControls* controls,const float point[2]);

#ifdef __cplusplus
}
#endif

#endif
