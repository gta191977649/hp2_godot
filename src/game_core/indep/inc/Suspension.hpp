#ifndef NFSHP2_INDEP_INC_SUSPENSION_HPP
#define NFSHP2_INDEP_INC_SUSPENSION_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/RigidBody.hpp"
#include "indep/inc/Wheel.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SteeringGeometry {
    float frontAxleX;
    float rearAxleX;
    float leftWheelY;
    float rightWheelY;
} SteeringGeometry;

typedef struct Steering {
    const SteeringGeometry* geometry;
    float leftTransform[16];
    float rightTransform[16];
} Steering;

// Geometry outlives steering. Steering angle uses one turn per 65536 units.
void steeringInitialize(Steering* steering, const SteeringGeometry* geometry);
void steeringSetAngle(Steering* steering, uint16_t angle);

struct ReplaySnapshot;
void steeringDoSnapshot(Steering* steering, struct ReplaySnapshot* snapshot);

typedef struct Suspension Suspension;
typedef struct SuspensionState SuspensionState;

float suspensionGetTravelLimitDelta(const Suspension* suspension);
int32_t suspensionAtBumpStop(const Suspension* suspension);
void suspensionRestoreState(Suspension* suspension, const SuspensionState* state);
void suspensionSaveState(const Suspension* suspension, SuspensionState* state);
float suspensionGetAntiRollRate(const Suspension* suspension);
float suspensionGetDamping(const Suspension* suspension);
float suspensionGetSpringRate(const Suspension* suspension);
float suspensionGetNonlinearRate(const Suspension* suspension);
void suspensionSetTuningValue(Suspension* suspension, float value);
int32_t suspensionGetIndex(const Suspension* suspension);
float* suspensionMountOffset(Suspension* suspension);

typedef struct SuspensionState {
    float travel;
    float travelSpeed;
} SuspensionState;

// Source configuration is a separate object shared with the suspension.
// Its disk offsets are handled by the vehicle-data decoder.
typedef struct SuspensionInfo {
    float bumpStop;
    float extensionLimit;
    float nonlinearRate;
    float springRate;
    float positiveDamping;
    float negativeDamping;
    float antiRollRate;
    float extensionStop;
} SuspensionInfo;

typedef struct Suspension {
    SuspensionState motion;
    const SuspensionInfo* info;
    struct Suspension* pairedSuspension;
    Wheel* wheel;
    RigidBody* body;
    const Steering* steering;
    int32_t steeringSide;
    float contactForce;
    float position[4];
    float transform[16];
    int32_t contactPresent;
    float tuningValue;
    float preload;
    float mountOffset[4];
    float contactResult;
} Suspension;

typedef struct WheelContactContext WheelContactContext;

Suspension* suspensionInitialize(Suspension* suspension, int32_t index,
                                  RigidBody* body, Wheel* wheel,
                                  const Steering* steering, const SuspensionInfo* info);
struct ReplaySnapshot;
void suspensionSerializePersistentState(Suspension* suspension, struct ReplaySnapshot* snapshot);
void suspensionInitializeMount(Suspension* suspension, float preload);
void suspensionResetMotion(Suspension* suspension);
void suspensionPrepareContact(Suspension* suspension, const WheelContactContext* context);
void suspensionSetContactLoad(Suspension* suspension, float extraLoad);
void suspensionApplyContactForce(Suspension* suspension, const WheelContactContext* context);
float* suspensionPointVelocity(float output[4], const float linearVelocity[4],
                                const float angularVelocity[4], const float lever[4]);
float suspensionResolveContactImpulse(Suspension* suspension, const WheelContactContext* context);

float* suspensionTransform(Suspension* suspension);

SuspensionState* suspensionState(Suspension* suspension);
int32_t suspensionStateSize(const Suspension* suspension);
SuspensionState* suspensionComputeDerivatives(const Suspension* suspension,
                                               SuspensionState* derivatives);
float suspensionGetTravel(const Suspension* suspension);
float suspensionGetTravelSpeed(const Suspension* suspension);

void suspensionSetPairedSuspension(Suspension* suspension, Suspension* paired);

#ifdef __cplusplus
}
#endif

#endif
