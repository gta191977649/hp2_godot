#ifndef NFSHP2_INDEP_INC_ENGINE_HPP
#define NFSHP2_INDEP_INC_ENGINE_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/Table.hpp"
#include "indep/inc/Wheel.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DriveTrain DriveTrain;
typedef struct Engine Engine;
struct ReplaySnapshot;
void engineSnapshot(Engine* engine, struct ReplaySnapshot* snapshot);
void driveTrainSnapshot(DriveTrain* driveTrain, struct ReplaySnapshot* snapshot);

int32_t driveTrainGetHighestGear(const DriveTrain* driveTrain);
int32_t driveTrainIsClutchEngaged(const DriveTrain* driveTrain);
void engineAddRoadTorque(Engine* engine, float torque);
float engineGetNitroBoostTimer(const Engine* engine);
void engineSetNitroBoostTimer(Engine* engine, float seconds);
void engineSetNitroEnabled(Engine* engine, int32_t enabled);
float engineGetMaximumRpm(const Engine* engine);
float engineGetRedlineRpm(const Engine* engine);
float engineGetIdleRpm(const Engine* engine);
float engineGetEffectiveThrottle(const Engine* engine);
float engineGetThrottle(const Engine* engine);
float engineGetTorqueResponse(const Engine* engine);
void engineSetThrottle(Engine* engine, float throttle);

// Physical engine data is decoded from the original vehicle records. The
// precision and order of the original floating-point calculations are kept.
typedef struct EngineInfo {
    float idleRpm;
    float redlineRpm;
    float maximumRpm;
    float momentOfInertia;
    float torqueCurve[9];
    float brakingCurve[3];
} EngineInfo;

typedef struct EngineTuning {
    float pursuitTorqueMultiplier;
    float overspeedTorqueMultiplier;
} EngineTuning;

extern const EngineTuning engineDefaultTuning;

// This is the engine's view of the owning vehicle state. Mode 1 reduces
// negative engine torque; other modes retain the full braking torque.
typedef struct EngineVehicleState {
    int32_t brakingMode;
} EngineVehicleState;

typedef struct Engine {
    float angularVelocity;
    const EngineInfo* info;
    int32_t hotPursuit;
    float throttle;
    float roadTorque;
    float throttleOverride;
    int32_t nitroEnabled;
    float nitroBoostTimer;
    float catchupMultiplier;
    const EngineVehicleState* vehicle;
    const EngineTuning* tuning;
    SampledCurve torque;
    SampledCurve braking;
} Engine;

// Configuration, vehicle state and tuning outlive the engine. Initialization
// preserves the motion, throttle and road torque supplied by the owning car.
Engine* engineInitialize(Engine* engine, const EngineInfo* info,
                         const EngineVehicleState* vehicle,
                         const EngineTuning* tuning);
float engineGetCurrentOutputTorque(const Engine* engine);
float engineGetOutputTorqueAtRpm(const Engine* engine, float rpm);
float engineGetNetTorque(const Engine* engine);
int32_t engineComputeDerivatives(const Engine* engine, float* output);
void engineUpdateNitro(Engine* engine, int32_t worldTicks);

void engineConstrainAngularVelocity(Engine* engine);

void engineSetThrottleOverride(Engine* engine, float overrideValue);

void engineSetHotPursuit(Engine* engine, int32_t enabled);

int32_t engineGetNitroEnabled(const Engine* engine);

void engineSetCatchupMultiplier(Engine* engine, float multiplier);

const EngineInfo* engineGetInfo(const Engine* engine);
void engineSetAngularVelocity(Engine* engine, float velocity);
float engineGetAngularVelocity(const Engine* engine);
void engineRestoreAngularVelocity(Engine* engine, const float* state);
void engineSaveAngularVelocity(const Engine* engine, float* state);
float* engineAngularVelocityState(Engine* engine);
int32_t engineAngularVelocityStateSize(const Engine* engine);

// Native gear, clutch and differential state. Serialized layouts are decoded
// at the resource boundary; all component associations are native pointers.
typedef struct DriveTrainInfo {
    float shiftRpmFraction;
    float frontFinalDrive;
    float rearFinalDrive;
    float rearDriveFraction;
    float clutchStiffness;
    int32_t forwardGearCount;
    float clutchRate;
    float gearRatios[8];
} DriveTrainInfo;

typedef struct DriveTrainVehicleState {
    float speedMetersPerSecond;
    int32_t differentialMode;
} DriveTrainVehicleState;

typedef struct DriveTrainTuning {
    int32_t calculateShiftFromTorque;
    float upshiftRpmScale;
    float downshiftRpmScale;
    float limitedSlipFraction;
    float maximumCouplingSlip;
    float synchronizationTolerance;
    float downshiftOverrevMultiplier;
} DriveTrainTuning;

extern const DriveTrainTuning driveTrainDefaultTuning;

typedef struct DriveTrain {
    float clutch;
    const DriveTrainInfo* info;
    DriveTrainVehicleState* vehicle;
    int32_t desiredGear;
    int32_t currentGear;
    int32_t clutchState;
    float shiftUpRpm[9];
    float shiftDownRpm[9];
    float previousSlipRpm;
    Engine* engine;
    Wheel* wheels[4];
    const DriveTrainTuning* tuning;
    int32_t previousGear;
    int32_t reengageRequested;
    float drivenAngularVelocity;
    float shiftWaitSeconds;
    int32_t shiftTarget;
} DriveTrain;

// Gear indices are reverse=0, neutral=1, then the forward gears. Configuration
// supports the six forward gears present in the original vehicle records.
void driveTrainBuildShiftTables(const DriveTrainInfo* info, const EngineInfo* engineInfo,
                                float upshift[9], float downshift[9],
                                const DriveTrainTuning* tuning);
DriveTrain* driveTrainInitialize(DriveTrain* driveTrain, const DriveTrainInfo* info,
                                 Engine* engine, Wheel* const wheels[4],
                                 DriveTrainVehicleState* vehicle,
                                 const DriveTrainTuning* tuning);
float driveTrainGetDrivenAngularVelocity(const DriveTrain* driveTrain);
float driveTrainGetCouplingFeedback(const DriveTrain* driveTrain);
void driveTrainSetDrivenAngularVelocity(DriveTrain* driveTrain, float velocity);
float driveTrainGetDrivenAngularAcceleration(const DriveTrain* driveTrain);
void driveTrainPrepareStep(DriveTrain* driveTrain);
int32_t driveTrainComputeDerivatives(const DriveTrain* driveTrain, float* output);
void driveTrainApplyTorque(DriveTrain* driveTrain, float torque);
void driveTrainResolveForces(DriveTrain* driveTrain);
void driveTrainBalanceAcceleration(DriveTrain* driveTrain);
int32_t driveTrainShouldDisengageAtIdle(const DriveTrain* driveTrain);
int32_t driveTrainIsSynchronized(const DriveTrain* driveTrain);
int32_t driveTrainSetShiftTarget(DriveTrain* driveTrain, int32_t gear);
int32_t driveTrainUpdateRequestedGear(DriveTrain* driveTrain, int32_t request,
                                      float elapsed);
int32_t driveTrainIsRearDriven(const DriveTrain* driveTrain);
int32_t driveTrainIsFrontDriven(const DriveTrain* driveTrain);

int32_t driveTrainGetPreviousGear(const DriveTrain* driveTrain);
int32_t driveTrainGetShiftTarget(const DriveTrain* driveTrain);

void driveTrainSelectGear(DriveTrain* driveTrain, int32_t gear);
int32_t driveTrainGetDesiredGear(const DriveTrain* driveTrain);
int32_t driveTrainGetCurrentGear(const DriveTrain* driveTrain);
int32_t driveTrainGetClutchState(const DriveTrain* driveTrain);

const DriveTrainInfo* driveTrainGetInfo(const DriveTrain* driveTrain);
Engine* driveTrainGetEngine(const DriveTrain* driveTrain);
float driveTrainGetShiftUpRpm(const DriveTrain* driveTrain, int32_t gear);
float driveTrainGetShiftDownRpm(const DriveTrain* driveTrain, int32_t gear);
void driveTrainRestoreClutch(DriveTrain* driveTrain, const float* state);
void driveTrainSaveClutch(const DriveTrain* driveTrain, float* state);
float* driveTrainClutchState(DriveTrain* driveTrain);
int32_t driveTrainClutchStateSize(const DriveTrain* driveTrain);

#ifdef __cplusplus
}
#endif

#endif
