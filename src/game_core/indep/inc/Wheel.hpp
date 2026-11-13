#ifndef NFSHP2_INDEP_INC_WHEEL_HPP
#define NFSHP2_INDEP_INC_WHEEL_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/Table.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Wheel Wheel;
typedef struct WheelState WheelState;

void wheelAddDriveTorque(Wheel* wheel, float torque);
float wheelGetTorqueResponse(const Wheel* wheel);
float wheelGetAngularAcceleration(const Wheel* wheel);
float wheelGetTractionLevel(const Wheel* wheel);
float wheelGetRadius(const Wheel* wheel);
float wheelGetAngle(const Wheel* wheel);
void wheelApplyForceMultiplier(Wheel* wheel, float value);
void wheelRestoreState(Wheel* wheel, const WheelState* state);
void wheelSaveState(const Wheel* wheel, WheelState* state);
float wheelGetRotationalInertia(const Wheel* wheel);
float wheelGetSteeringAngle(const Wheel* wheel);
void wheelSetSteeringAngle(Wheel* wheel, float value);
int32_t wheelIsFront(const Wheel* wheel);
int32_t wheelIsRear(const Wheel* wheel);
float wheelGetCouplingFeedback(const Wheel* wheel);
float wheelGetAngularSpeedEstimate(const Wheel* wheel);
void wheelSetContactCorrectionDisabled(Wheel* wheel, int8_t disabled);
int8_t wheelGetContactCorrectionDisabled(const Wheel* wheel);
float* wheelContactNormal(Wheel* wheel);
void wheelSetContactNormal(Wheel* wheel, const float normal[4]);
void wheelSetContactPoint(Wheel* wheel, const float point[4]);
void wheelSetVelocity(Wheel* wheel, const float velocity[4]);
void wheelSetCarTransform(Wheel* wheel, const float transform[16]);
void wheelSetWorldTransform(Wheel* wheel, const float transform[16]);
void wheelSetContactPresent(Wheel* wheel, int8_t contactPresent);
int8_t wheelGetContactPresent(const Wheel* wheel);
void wheelSetMaterialIndex(Wheel* wheel, int32_t index);
int32_t wheelGetMaterialIndex(const Wheel* wheel);
float wheelGetSurfaceOffset(const Wheel* wheel);
void wheelSetSlip(Wheel* wheel, const float slip[2]);
float wheelGetSlipRatio(const Wheel* wheel);
int32_t wheelIsRolling(const Wheel* wheel);
float wheelGetDriveTorque(const Wheel* wheel);
void wheelSetBaseForceMultiplier(Wheel* wheel, float value);
float wheelGetHandbrakeTorque(const Wheel* wheel);

typedef struct WheelState {
    float angle;
    float angularVelocity;
} WheelState;

typedef enum PhysicsStateTransfer {
    PHYSICS_STATE_SAVE,
    PHYSICS_STATE_RESTORE,
    PHYSICS_STATE_PRESERVE
} PhysicsStateTransfer;

// Wheel coefficients are decoded into a native structure at the resource boundary.
// The original component follows a pointer to its wheel definition.
typedef struct WheelTorqueInfo {
    float reactionScale;
    float mountPosition[4];
    float halfWidth;
    int32_t index;
} WheelTorqueInfo;

typedef struct WheelTireInfo {
    float slipAngleLimit;
    float referenceLoad;
    float lateralScale;
    float staticGrip;
    float slidingGrip;
} WheelTireInfo;

typedef struct WheelBrakeInfo {
    float serviceTorque;
    float handbrakeTorque;
} WheelBrakeInfo;

typedef struct WheelVehicleState {
    float speed;
    int32_t pursuitMode;
} WheelVehicleState;

// The current surface supplies rolling resistance. Elapsed time is the
// published world timestep, shared by every component in the integration pass.
typedef struct WheelStepContext {
    float elapsed;
    float rollingResistance;
} WheelStepContext;

typedef struct Wheel {
    const WheelTorqueInfo* torqueInfo;
    WheelState motion;
    float rotationalInertia;
    float accumulatedDriveTorque;
    float reactionTorque;
    float angularSpeedEstimate;
    float tractionLevel;
    float forceMultiplier;
    float handbrakeTorque;
    float brakeTorque;
    int32_t rotationLocked;
    int8_t contactCorrectionDisabled;
    float steeringAngle;
    int32_t rotationDirection;
    float lateralReactionForce;
    float orientation[16];
    int8_t contactPresent;
    float couplingFeedback;
    const WheelTireInfo* tireInfo;
    const WheelBrakeInfo* brakeInfo;
    const WheelVehicleState* vehicle;
    float worldTransform[16];
    float carTransform[16];
    float velocity[4];
    float slip[2];
    float worldContactForce[4];
    float contactPoint[4];
    float contactNormal[4];
    int32_t materialIndex;
    uint16_t contactFlags;
    float surfaceOffset;
    float slipRatio;
    float baseForceMultiplier;
} Wheel;

WheelState* wheelState(Wheel* wheel);
int32_t wheelStateSize(const Wheel* wheel);

float wheelGetAngularVelocity(const Wheel* wheel);

int32_t wheelComputeDerivatives(const Wheel* wheel,
                                const WheelStepContext* context,
                                WheelState* derivatives);
// Refresh rotation from the current motion and clear per-step force inputs
// before the following contact/force-collection pass.
void wheelPrepareStep(Wheel* wheel);

void wheelSetMotion(Wheel* wheel, float angle, float angularVelocity);
void wheelSetAngularVelocity(Wheel* wheel, float angularVelocity);

void wheelSetForceMultiplier(Wheel* wheel, float value);

void wheelSetBrakeTorque(Wheel* wheel, float value);

float* wheelContactPoint(Wheel* wheel);
float* wheelContactForce(Wheel* wheel);
const float* wheelGetRestPositionInGeometryFrame(const Wheel* wheel);

float* wheelSlip(Wheel* wheel);

float* wheelOrientation(Wheel* wheel);
float* wheelCarTransform(Wheel* wheel);

float wheelGetMass(const Wheel* wheel);

void wheelSetHandbrakeTorque(Wheel* wheel, float value);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif
struct ReplaySnapshot;
void wheelSerializePersistentState(Wheel* wheel,struct ReplaySnapshot* snapshot);
typedef struct TerrainSurface TerrainSurface;
typedef struct TerrainMaterial TerrainMaterial;

typedef struct WheelContactTuning {
    SampledCurve legacyLateralCurve;
    SampledCurve loadLateralCurves[7];
    float rotationalInertia;
    float serviceBrakeScale;
    float handbrakeScale;
    float brakeLockScale;
    float angularBrakeResistance;
    float gripScale;
    float highSpeedGrip;
    float legacyLateralScale;
    float loadLateralScale;
    float lateralLoadScale;
    int32_t useLoadLateralCurves;
    int32_t forceFrameMode;
    float drivenReactionScale;
    float contactImpulseScale;
    float bumpImpactScale;
    float bumpImpactLimit;
    float wheelTiltScale;
    int32_t suppressBodyForces;
} WheelContactTuning;

extern const WheelContactTuning wheelContactDefaultTuning;

// Terrain and material records are decoded native data. The current material
// index must select a record in this table throughout one force pass.
typedef struct WheelContactContext {
    TerrainSurface* terrain;
    const TerrainMaterial* materials;
    size_t materialCount;
    float elapsed;
    const WheelContactTuning* tuning;
} WheelContactContext;

Wheel* wheelInitialize(Wheel* wheel, const WheelTorqueInfo* info,
                       const WheelTireInfo* tire, const WheelBrakeInfo* brakes,
                       const WheelVehicleState* vehicle);
void wheelInitializeRotationalInertia(Wheel* wheel, const WheelContactTuning* tuning);
float wheelSetContactLoad(Wheel* wheel, int32_t contactPresent, float load);
float wheelGroundDisplacement(Wheel* wheel, const WheelContactContext* context,
                               const float worldPoint[4], const float bodyOrientation[16]);
float wheelQueryGroundDisplacement(Wheel* wheel, const WheelContactContext* context,
                                    float normal[4], int32_t* materialIndex,
                                    const float worldPoint[4], const float bodyOrientation[16]);
void wheelUpdateBrakeLock(Wheel* wheel, const WheelContactContext* context,
                           float availableGrip);
float wheelHighSpeedGrip(const Wheel* wheel, const WheelContactTuning* tuning);
void wheelComputeContactForces(Wheel* wheel, const WheelContactContext* context,
                                int32_t preserveDriveTorque);
float wheelEvaluateLateralForce(const Wheel* wheel, const WheelContactTuning* tuning,
                                 float load, float absoluteSlipAngle);
void wheelCorrectReactionTorque(Wheel* wheel, const float localAcceleration[2]);
void wheelResolveContactForce(Wheel* wheel, const WheelContactContext* context);
float* wheelWorldReactionForce(const Wheel* wheel, float output[4]);
float* wheelCarReactionForce(const Wheel* wheel, float output[3]);

#ifdef __cplusplus
}
#endif

#endif
