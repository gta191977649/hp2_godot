#include "indep/inc/Engine.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"

#include <math.h>
#include <cmath>

int32_t driveTrainIsClutchEngaged(const DriveTrain* driveTrain)
{
    return driveTrain->clutchState == 3;
}

int32_t driveTrainGetHighestGear(const DriveTrain* driveTrain)
{
    return driveTrain->info->forwardGearCount + 1;
}

int32_t driveTrainSetShiftTarget(DriveTrain* driveTrain, int32_t gear)
{
    if (2 <= gear && gear <= driveTrainGetHighestGear(driveTrain)) {
        if (driveTrain->shiftTarget == 1)
            driveTrain->shiftWaitSeconds = 0.0f;
        driveTrain->shiftTarget = gear;
    }
    return driveTrain->shiftTarget;
}

void engineSetThrottle(Engine* engine, float throttle)
{
    engine->throttle = throttle;
}

float engineGetThrottle(const Engine* engine)
{
    return engine->throttle;
}

float engineGetEffectiveThrottle(const Engine* engine)
{
    return engine->throttle * engine->throttleOverride;
}

void engineAddRoadTorque(Engine* engine, float torque)
{
    engine->roadTorque = engine->roadTorque + torque;
}

void engineSetNitroEnabled(Engine* engine, int32_t enabled)
{
    engine->nitroEnabled = enabled;
}

void engineSetNitroBoostTimer(Engine* engine, float seconds)
{
    engine->nitroBoostTimer = seconds;
}

float engineGetNitroBoostTimer(const Engine* engine)
{
    return engine->nitroBoostTimer;
}

float engineGetTorqueResponse(const Engine* engine)
{
    return 1.0f / engine->info->momentOfInertia;
}

float engineGetIdleRpm(const Engine* engine)
{
    return engine->info->idleRpm;
}

float engineGetRedlineRpm(const Engine* engine)
{
    return engine->info->redlineRpm;
}

float engineGetMaximumRpm(const Engine* engine)
{
    return engine->info->maximumRpm;
}

const EngineTuning engineDefaultTuning = {1.0499999523162842f, 15.0f};

Engine* engineInitialize(Engine* engine, const EngineInfo* info,
                         const EngineVehicleState* vehicle,
                         const EngineTuning* tuning)
{
    sampledCurveInitialize(&engine->torque, info->torqueCurve, 9,
                           info->idleRpm, info->maximumRpm);
    sampledCurveInitialize(&engine->braking, info->brakingCurve, 3,
                           info->idleRpm, info->maximumRpm);
    engine->info = info;
    engine->vehicle = vehicle;
    engine->tuning = tuning;
    engine->catchupMultiplier = 1.0f;
    engine->hotPursuit = 0;
    engine->nitroEnabled = 0;
    engine->nitroBoostTimer = 0.0f;
    engine->throttleOverride = 1.0f;
    return engine;
}

int32_t engineComputeDerivatives(const Engine* engine, float* output)
{
    const float torque = bScalarAdd(engineGetNetTorque(engine), engine->roadTorque);
    *output = bScalarDivide(torque, engine->info->momentOfInertia);
    return 0;
}

float engineGetCurrentOutputTorque(const Engine* engine)
{
    const float radiansPerSecondToRpm = 9.549304008483887f;
    const float rpm = bScalarMultiply(engine->angularVelocity, radiansPerSecondToRpm);
    float torque = bScalarMultiply(sampledCurveEvaluate(&engine->torque, rpm),
                                    engine->catchupMultiplier);
    if (engine->hotPursuit != 0)
        torque = bScalarMultiply(torque, engine->tuning->pursuitTorqueMultiplier);
    const float limit = bScalarAdd(engine->info->maximumRpm, -100.0f);
    if (rpm < limit)
        return torque;
    return bScalarMultiply(-engine->tuning->overspeedTorqueMultiplier, torque);
}

float engineGetNetTorque(const Engine* engine)
{
    const float output = engineGetCurrentOutputTorque(engine);
    const float throttle = bScalarMultiply(engine->throttle, engine->throttleOverride);
    float driveTorque = bScalarMultiply(output, throttle);
    if (engine->nitroEnabled != 0)
        driveTorque = bScalarMultiply(driveTorque, 3.0f);

    const float radiansPerSecondToRpm = 9.549304008483887f;
    const float rpm = bScalarMultiply(engine->angularVelocity, radiansPerSecondToRpm);
    const float braking = -sampledCurveEvaluate(&engine->braking, rpm);
    const float brakingThrottle = bScalarMultiply(engine->throttle, engine->throttleOverride);
    const float brakeTorque = bScalarMultiply(braking, bScalarAdd(1.0f, -brakingThrottle));
    float torque = bScalarAdd(driveTorque, brakeTorque);
    if (engine->vehicle->brakingMode == 1 && torque < 0.0f)
        torque = bScalarMultiply(torque, 0.4000000059604645f);
    return torque;
}

float engineGetOutputTorqueAtRpm(const Engine* engine, float rpm)
{
    float torque = sampledCurveEvaluate(&engine->torque, rpm)
                   * engine->catchupMultiplier;
    if (engine->hotPursuit != 0)
        torque = torque * engine->tuning->pursuitTorqueMultiplier;
    const float limit = engine->info->maximumRpm - 100.0f;
    if (limit <= rpm)
        torque = -engine->tuning->overspeedTorqueMultiplier * torque;
    return torque;
}

void engineUpdateNitro(Engine* engine, int32_t worldTicks)
{
    if (engine->nitroEnabled != 0) {
        const float expiry = engine->nitroBoostTimer + 4.0f;
        const float now = (float)worldTicks * 0.0002500000118743628f;
        if (expiry < now)
            engine->nitroEnabled = 0;
    }
}

void engineConstrainAngularVelocity(Engine* engine)
{
    const float rpmToRadiansPerSecond = 0.10471967607736588f;
    const float idle = bScalarMultiply(engine->info->idleRpm, rpmToRadiansPerSecond);
    if (engine->angularVelocity < idle)
        engine->angularVelocity = idle;
    const float maximum = bScalarMultiply(engine->info->maximumRpm, rpmToRadiansPerSecond);
    if (maximum < engine->angularVelocity)
        engine->angularVelocity = maximum;
    engine->roadTorque = 0.0f;
}

void engineRestoreAngularVelocity(Engine* engine, const float* state)
{
    engine->angularVelocity = *state;
}

void engineSaveAngularVelocity(const Engine* engine, float* state)
{
    *state = engine->angularVelocity;
}

float* engineAngularVelocityState(Engine* engine)
{
    return &engine->angularVelocity;
}

int32_t engineAngularVelocityStateSize(const Engine* engine)
{
    (void)engine;
    return 1;
}

void engineSetThrottleOverride(Engine* engine, float overrideValue)
{
    engine->throttleOverride = overrideValue;
}

void engineSetHotPursuit(Engine* engine, int32_t enabled)
{
    engine->hotPursuit = enabled;
}

int32_t engineGetNitroEnabled(const Engine* engine)
{
    return engine->nitroEnabled;
}

void engineSetCatchupMultiplier(Engine* engine, float multiplier)
{
    engine->catchupMultiplier = multiplier;
}

const EngineInfo* engineGetInfo(const Engine* engine)
{
    return engine->info;
}

void engineSetAngularVelocity(Engine* engine, float velocity)
{
    engine->angularVelocity = velocity;
}

float engineGetAngularVelocity(const Engine* engine)
{
    return engine->angularVelocity;
}

void driveTrainSelectGear(DriveTrain* driveTrain, int32_t gear)
{
    driveTrain->desiredGear = gear;
    driveTrain->currentGear = gear;
    driveTrain->clutchState = 3;
    driveTrain->clutch = 1.0f;
    driveTrain->previousSlipRpm = 0.0f;
}

int32_t driveTrainGetDesiredGear(const DriveTrain* driveTrain)
{
    return driveTrain->desiredGear;
}

int32_t driveTrainGetCurrentGear(const DriveTrain* driveTrain)
{
    return driveTrain->currentGear;
}

int32_t driveTrainGetClutchState(const DriveTrain* driveTrain)
{
    return driveTrain->clutchState;
}

const DriveTrainInfo* driveTrainGetInfo(const DriveTrain* driveTrain)
{
    return driveTrain->info;
}

Engine* driveTrainGetEngine(const DriveTrain* driveTrain)
{
    return driveTrain->engine;
}

float driveTrainGetShiftUpRpm(const DriveTrain* driveTrain, int32_t gear)
{
    return driveTrain->shiftUpRpm[gear];
}

float driveTrainGetShiftDownRpm(const DriveTrain* driveTrain, int32_t gear)
{
    return driveTrain->shiftDownRpm[gear];
}

void driveTrainRestoreClutch(DriveTrain* driveTrain, const float* state)
{
    driveTrain->clutch = *state;
    driveTrain->previousSlipRpm = 0.0f;
}

void driveTrainSaveClutch(const DriveTrain* driveTrain, float* state)
{
    *state = driveTrain->clutch;
}

float* driveTrainClutchState(DriveTrain* driveTrain)
{
    return &driveTrain->clutch;
}

int32_t driveTrainClutchStateSize(const DriveTrain* driveTrain)
{
    (void)driveTrain;
    return 1;
}

const DriveTrainTuning driveTrainDefaultTuning = {
    1, 0.949999988079071f, 0.949999988079071f,
    0.3499999940395355f, 300.0f, 50.0f, 1.2000000476837158f
};

void driveTrainBuildShiftTables(const DriveTrainInfo* info, const EngineInfo* engineInfo,
                                float upshift[9], float downshift[9],
                                const DriveTrainTuning* tuning)
{
    SampledCurve curve;
    sampledCurveInitialize(&curve, engineInfo->torqueCurve, 9,
                           engineInfo->idleRpm, engineInfo->maximumRpm);
    int32_t gear = 2;
    const int32_t highest = info->forwardGearCount + 1;
    for (; gear < highest; ++gear) {
        const float currentRatio = info->gearRatios[gear];
        const float nextRatio = info->gearRatios[gear + 1];
        if (tuning->calculateShiftFromTorque != 0) {
            float rpm = (engineInfo->idleRpm + engineInfo->maximumRpm) * 0.5f;
            const float limit = engineInfo->maximumRpm - 200.0f;
            int searching = 1;
            while (rpm < limit && searching) {
                const float currentTorque = sampledCurveEvaluate(&curve, rpm);
                const float nextRpmProduct = rpm * nextRatio;
                const float nextRpm = nextRpmProduct / currentRatio;
                const float nextTorque = sampledCurveEvaluate(&curve, nextRpm);
                const float reflectedProduct = nextTorque * nextRatio;
                const float reflectedTorque = reflectedProduct / currentRatio;
                if (currentTorque < reflectedTorque) {
                    upshift[gear] = rpm;
                    searching = 0;
                }
                rpm = rpm + 50.0f;
            }
            if (searching)
                upshift[gear] = engineInfo->maximumRpm - 200.0f;
            upshift[gear] = upshift[gear] * tuning->upshiftRpmScale;
        } else {
            const float range = engineInfo->redlineRpm - engineInfo->idleRpm;
            const float offset = info->shiftRpmFraction * range;
            upshift[gear] = engineInfo->idleRpm + offset;
        }
        const float scaled = tuning->downshiftRpmScale * upshift[gear];
        const float nextProduct = scaled * nextRatio;
        downshift[gear + 1] = nextProduct / currentRatio;
    }
    upshift[gear] = engineInfo->maximumRpm;
}

DriveTrain* driveTrainInitialize(DriveTrain* driveTrain, const DriveTrainInfo* info,
                                 Engine* engine, Wheel* const wheels[4],
                                 DriveTrainVehicleState* vehicle,
                                 const DriveTrainTuning* tuning)
{
    driveTrain->engine = engine;
    driveTrain->info = info;
    for (int i = 0; i < 4; ++i)
        driveTrain->wheels[i] = wheels[i];
    driveTrain->vehicle = vehicle;
    driveTrain->tuning = tuning;
    driveTrainBuildShiftTables(info, engine->info, driveTrain->shiftUpRpm,
                               driveTrain->shiftDownRpm, tuning);
    driveTrain->desiredGear = 2;
    driveTrain->clutch = 0.0f;
    driveTrain->clutchState = 2;
    driveTrain->currentGear = 2;
    driveTrain->reengageRequested = 0;
    driveTrain->previousSlipRpm = 0.0f;
    return driveTrain;
}

int32_t driveTrainIsRearDriven(const DriveTrain* driveTrain)
{
    return 0.0010000000474974513f <= driveTrain->info->rearDriveFraction;
}

int32_t driveTrainIsFrontDriven(const DriveTrain* driveTrain)
{
    return driveTrain->info->rearDriveFraction <= 0.9990000128746033f;
}

int32_t driveTrainGetPreviousGear(const DriveTrain* driveTrain)
{
    return driveTrain->previousGear;
}

int32_t driveTrainGetShiftTarget(const DriveTrain* driveTrain)
{
    return driveTrain->shiftTarget;
}

float driveTrainGetDrivenAngularVelocity(const DriveTrain* driveTrain)
{
    const DriveTrainInfo* info = driveTrain->info;
    float velocity = 0.0f;
    if (driveTrainIsRearDriven(driveTrain)) {
        const float sum = bScalarAdd(driveTrain->wheels[2]->motion.angularVelocity,
                                     driveTrain->wheels[3]->motion.angularVelocity);
        const float average = bScalarMultiply(sum, 0.5f);
        velocity = bScalarMultiply(average, info->rearFinalDrive);
    }
    if (driveTrainIsFrontDriven(driveTrain)) {
        const float sum = bScalarAdd(driveTrain->wheels[0]->motion.angularVelocity,
                                     driveTrain->wheels[1]->motion.angularVelocity);
        const float average = bScalarMultiply(sum, 0.5f);
        const float front = bScalarMultiply(average, info->frontFinalDrive);
        velocity = bScalarAdd(velocity, front);
        if (driveTrainIsRearDriven(driveTrain))
            velocity = bScalarMultiply(velocity, 0.5f);
    }
    return bScalarMultiply(velocity, info->gearRatios[driveTrain->currentGear]);
}

float driveTrainGetCouplingFeedback(const DriveTrain* driveTrain)
{
    float feedback = 0.0f;
    if (driveTrainIsRearDriven(driveTrain)) {
        const float sum = driveTrain->wheels[2]->couplingFeedback
                          + driveTrain->wheels[3]->couplingFeedback;
        feedback = sum * 0.5f;
    }
    if (driveTrainIsFrontDriven(driveTrain)) {
        const float sum = driveTrain->wheels[0]->couplingFeedback
                          + driveTrain->wheels[1]->couplingFeedback;
        const float front = sum * 0.5f;
        const float combined = feedback + front;
        feedback = combined * 0.5f;
    }
    return feedback;
}

void driveTrainSetDrivenAngularVelocity(DriveTrain* driveTrain, float velocity)
{
    const float difference = velocity - driveTrainGetDrivenAngularVelocity(driveTrain);
    const float speedMph = driveTrain->vehicle->speedMetersPerSecond * 2.236999988555908f;
    const DriveTrainInfo* info = driveTrain->info;
    for (int axle = 1; axle >= 0; --axle) {
        if ((axle == 1 && !driveTrainIsRearDriven(driveTrain)) ||
            (axle == 0 && !driveTrainIsFrontDriven(driveTrain)))
            continue;
        Wheel* first = driveTrain->wheels[axle * 2];
        Wheel* second = driveTrain->wheels[axle * 2 + 1];
        const float finalDrive = axle == 1 ? info->rearFinalDrive : info->frontFinalDrive;
        const float ratio = info->gearRatios[driveTrain->currentGear] * finalDrive;
        const float delta = difference / ratio;
        float firstVelocity = first->motion.angularVelocity;
        float secondVelocity = second->motion.angularVelocity;
        if (speedMph < 40.0f || (!first->contactPresent && !second->contactPresent)) {
            const float sum = firstVelocity + secondVelocity;
            secondVelocity = sum * 0.5f;
            firstVelocity = secondVelocity;
        }
        wheelSetAngularVelocity(first, firstVelocity + delta);
        wheelSetAngularVelocity(second, secondVelocity + delta);
    }
}

float driveTrainGetDrivenAngularAcceleration(const DriveTrain* driveTrain)
{
    const DriveTrainInfo* info = driveTrain->info;
    float acceleration = 0.0f;
    if (driveTrainIsRearDriven(driveTrain)) {
        const float first = wheelGetAngularAcceleration(driveTrain->wheels[2]);
        const float second = wheelGetAngularAcceleration(driveTrain->wheels[3]);
        const float sum = bScalarAdd(first, second);
        const float average = bScalarMultiply(sum, 0.5f);
        acceleration = bScalarMultiply(average, info->rearFinalDrive);
    }
    if (driveTrainIsFrontDriven(driveTrain)) {
        const float first = wheelGetAngularAcceleration(driveTrain->wheels[0]);
        const float second = wheelGetAngularAcceleration(driveTrain->wheels[1]);
        const float sum = bScalarAdd(first, second);
        const float average = bScalarMultiply(sum, 0.5f);
        const float front = bScalarMultiply(average, info->frontFinalDrive);
        acceleration = bScalarAdd(acceleration, front);
        if (driveTrainIsRearDriven(driveTrain))
            acceleration = bScalarMultiply(acceleration, 0.5f);
    }
    return bScalarMultiply(acceleration, info->gearRatios[driveTrain->currentGear]);
}

int32_t driveTrainShouldDisengageAtIdle(const DriveTrain* driveTrain)
{
    if (!(driveTrain->engine->throttle < 0.30000001192092896f))
        return 0;
    const float idle = driveTrain->engine->info->idleRpm * 0.10471967607736588f;
    const float threshold = idle + 0.00009999999747378752f;
    return driveTrain->drivenAngularVelocity <= threshold;
}

int32_t driveTrainIsSynchronized(const DriveTrain* driveTrain)
{
    const float difference = driveTrain->engine->angularVelocity
                             - driveTrain->drivenAngularVelocity;
    return fabsf(difference) <= driveTrain->tuning->synchronizationTolerance;
}

void driveTrainPrepareStep(DriveTrain* driveTrain)
{
    driveTrain->drivenAngularVelocity = driveTrainGetDrivenAngularVelocity(driveTrain);
    Engine* engine = driveTrain->engine;
    switch (driveTrain->clutchState) {
    case 3:
        engineSetThrottleOverride(engine, 1.0f);
        if (driveTrain->currentGear != 1) {
            const float difference = driveTrain->drivenAngularVelocity - engine->angularVelocity;
            const float feedback = driveTrainGetCouplingFeedback(driveTrain);
            if (0.20000000298023224f < engine->throttle && 0.0f < feedback) {
                driveTrain->drivenAngularVelocity = engine->angularVelocity;
                driveTrainSetDrivenAngularVelocity(driveTrain, engine->angularVelocity);
            } else {
                const float change = difference * 0.03999999910593033f;
                engine->angularVelocity = engine->angularVelocity + change;
            }
            if (driveTrainShouldDisengageAtIdle(driveTrain))
                driveTrain->clutchState = 0;
        }
        if (driveTrain->currentGear != driveTrain->desiredGear) {
            driveTrain->clutchState = 0;
            driveTrain->reengageRequested = 1;
        }
        break;
    case 2:
        engineSetThrottleOverride(engine, 0.699999988079071f);
        if (1.0f <= driveTrain->clutch) {
            driveTrain->clutch = 1.0f;
            if (driveTrainIsSynchronized(driveTrain) || driveTrain->currentGear == 1)
                driveTrain->clutchState = 3;
        }
        if (driveTrainShouldDisengageAtIdle(driveTrain))
            driveTrain->clutchState = 0;
        break;
    case 0:
        engineSetThrottleOverride(engine, 0.0f);
        if (driveTrain->clutch <= engine->throttleOverride) {
            driveTrain->clutchState = 1;
            driveTrain->clutch = engine->throttleOverride;
        }
        break;
    case 1:
        driveTrain->currentGear = driveTrain->desiredGear;
        engineSetThrottleOverride(engine, 0.0f);
        if (driveTrain->reengageRequested || 0.30000001192092896f < engine->throttle) {
            driveTrain->reengageRequested = 0;
            driveTrain->clutchState = 2;
        }
        break;
    default:
        break;
    }
}

int32_t driveTrainComputeDerivatives(const DriveTrain* driveTrain, float* output)
{
    static const float rates[4] = {-3.0f, 0.0f, 1.5f, 0.0f};
    *output = driveTrain->info->clutchRate * rates[driveTrain->clutchState];
    return 0;
}

static float differentialShare(const DriveTrain* driveTrain, const Wheel* first,
                                const Wheel* second, int rear)
{
    float limit = 0.5f;
    if (first->contactPresent && second->contactPresent)
        limit = driveTrain->tuning->limitedSlipFraction;
    if (driveTrain->vehicle->differentialMode == 1)
        return bScalarAdd(1.0f, -limit);
    if (driveTrain->vehicle->differentialMode == 2)
        return limit;
    const float firstVelocity=first->motion.angularVelocity;
    const float secondVelocity=second->motion.angularVelocity;
    hp2::Require(std::isfinite(firstVelocity) && std::isfinite(secondVelocity),
        "Differential wheel state is not finite");
    const float sum = bScalarAdd(firstVelocity,secondVelocity);
    // A stationary axle (or equal counter-rotation) has no signed speed
    // ratio. Split its torque symmetrically, conserving the axle total;
    // do not turn0/0 into an EE-style saturated number and a side bias.
    if (sum==0.0f) return 0.5f;
    const float numerator=rear ? firstVelocity:secondVelocity;
    // Finite wheel speeds can overflow their float sum. Widen only that
    // exceptional addition; ordinary valid ratios keep their float order.
    float share=std::isfinite(sum) ? bScalarDivide(numerator,sum) :
        static_cast<float>(static_cast<double>(numerator)/
            (static_cast<double>(firstVelocity)+static_cast<double>(secondVelocity)));
    if (share < limit)
        share = bScalarAdd(1.0f, -bScalarMultiply(driveTrain->tuning->limitedSlipFraction, 0.5f));
    else if (bScalarAdd(1.0f, -limit) < share)
        share = bScalarMultiply(driveTrain->tuning->limitedSlipFraction, 0.5f);
    return share;
}

void driveTrainApplyTorque(DriveTrain* driveTrain, float torque)
{
    if (driveTrain->currentGear == 1)
        return;
    Engine* engine = driveTrain->engine;
    engine->roadTorque = bScalarAdd(engine->roadTorque, -torque);
    const DriveTrainInfo* info = driveTrain->info;
    const float geared = bScalarMultiply(torque, info->gearRatios[driveTrain->currentGear]);
    const float rear = bScalarMultiply(geared, info->rearDriveFraction);
    const float front = bScalarMultiply(geared, bScalarAdd(1.0f, -info->rearDriveFraction));
    const float rearTorque = bScalarMultiply(rear, info->rearFinalDrive);
    const float frontTorque = bScalarMultiply(front, info->frontFinalDrive);
    float share = differentialShare(driveTrain, driveTrain->wheels[0], driveTrain->wheels[1], 0);
    const float firstFront = bScalarMultiply(frontTorque, bScalarAdd(1.0f, -share));
    const float secondFront = bScalarMultiply(frontTorque, share);
    wheelAddDriveTorque(driveTrain->wheels[0], firstFront);
    wheelAddDriveTorque(driveTrain->wheels[1], secondFront);
    share = differentialShare(driveTrain, driveTrain->wheels[2], driveTrain->wheels[3], 1);
    const float firstRear = bScalarMultiply(rearTorque, share);
    const float secondRear = bScalarMultiply(rearTorque, bScalarAdd(1.0f, -share));
    wheelAddDriveTorque(driveTrain->wheels[2], firstRear);
    wheelAddDriveTorque(driveTrain->wheels[3], secondRear);
}

void driveTrainResolveForces(DriveTrain* driveTrain)
{
    if (driveTrain->clutchState == 3) {
        driveTrainApplyTorque(driveTrain, engineGetNetTorque(driveTrain->engine));
    } else if (driveTrain->clutchState == 0 || driveTrain->clutchState == 2) {
        Engine* engine = driveTrain->engine;
        float slip = bScalarAdd(engine->angularVelocity, -driveTrain->drivenAngularVelocity);
        const float maximum = driveTrain->tuning->maximumCouplingSlip;
        if (maximum < slip)
            slip = maximum;
        else if (slip < -maximum)
            slip = -maximum;
        const float difference = bScalarAdd(driveTrain->drivenAngularVelocity, -engine->angularVelocity);
        const float slipRpm = bScalarMultiply(difference, 9.549304008483887f);
        float stiffness = driveTrain->info->clutchStiffness;
        if (driveTrain->previousSlipRpm != 0.0f) {
            const float signProduct = bScalarMultiply(slipRpm, driveTrain->previousSlipRpm);
            if (signProduct < 0.0f && 2000.0f < fabsf(slipRpm) &&
                2000.0f < fabsf(driveTrain->previousSlipRpm))
                stiffness = bScalarMultiply(stiffness, 0.5f);
        }
        driveTrain->previousSlipRpm = slipRpm;
        const float coupling = bScalarMultiply(slip, stiffness);
        driveTrainApplyTorque(driveTrain, bScalarMultiply(coupling, driveTrain->clutch));
    }
}

void driveTrainBalanceAcceleration(DriveTrain* driveTrain)
{
    if (driveTrain->clutchState != 3)
        return;
    Engine* engine = driveTrain->engine;
    const float total = bScalarAdd(engineGetNetTorque(engine), engine->roadTorque);
    const float engineAcceleration = bScalarDivide(total, engine->info->momentOfInertia);
    const float wheelAcceleration = driveTrainGetDrivenAngularAcceleration(driveTrain);
    const float difference = bScalarAdd(engineAcceleration, -wheelAcceleration);
    float response = bScalarDivide(1.0f, engine->info->momentOfInertia);
    const DriveTrainInfo* info = driveTrain->info;
    const float rearFraction = info->rearDriveFraction;
    if (driveTrainIsFrontDriven(driveTrain)) {
        const float reciprocal = bScalarDivide(1.0f, driveTrain->wheels[0]->rotationalInertia);
        const float ratio = bScalarMultiply(info->frontFinalDrive, info->gearRatios[driveTrain->currentGear]);
        const float fraction = bScalarMultiply(bScalarAdd(1.0f, -rearFraction), 0.5f);
        const float weighted = bScalarMultiply(ratio, reciprocal);
        const float product = bScalarMultiply(fraction, weighted);
        const float contribution = bScalarMultiply(product, ratio);
        response = bScalarAdd(response, contribution);
    }
    if (driveTrainIsRearDriven(driveTrain)) {
        const float reciprocal = bScalarDivide(1.0f, driveTrain->wheels[2]->rotationalInertia);
        const float ratio = bScalarMultiply(info->rearFinalDrive, info->gearRatios[driveTrain->currentGear]);
        const float fraction = bScalarMultiply(rearFraction, 0.5f);
        const float weighted = bScalarMultiply(ratio, reciprocal);
        const float product = bScalarMultiply(fraction, weighted);
        const float contribution = bScalarMultiply(product, ratio);
        response = bScalarAdd(response, contribution);
    }
    driveTrainApplyTorque(driveTrain, bScalarDivide(difference, response));
}

int32_t driveTrainUpdateRequestedGear(DriveTrain* driveTrain, int32_t request,
                                      float elapsed)
{
    const int32_t current = driveTrain->currentGear;
    Engine* engine = driveTrain->engine;
    const float radiansToRpm = 9.549304008483887f;
    if (request == -1) {
        driveTrain->shiftTarget = 1;
        if (current < 2) {
            driveTrain->desiredGear = 2;
            return request;
        }
        if (driveTrain->clutchState != 3)
            return request;
        const float rpm = engine->angularVelocity * radiansToRpm;
        float lower;
        if (current >= 3)
            lower = driveTrain->shiftDownRpm[current];
        else if (engine->throttle < 0.20000000298023224f)
            lower = engine->info->idleRpm + 100.0f;
        else
            lower = 0.0f;
        if (driveTrain->shiftUpRpm[current] < rpm &&
            current < driveTrainGetHighestGear(driveTrain)) {
            int canShift = 1;
            for (int wheel = 0; wheel < 4; ++wheel) {
                if (!(driveTrain->wheels[wheel]->angularSpeedEstimate < 4.0f) ||
                    !driveTrain->wheels[wheel]->contactPresent) {
                    canShift = 0;
                    break;
                }
            }
            if (canShift) {
                driveTrain->desiredGear = current + 1;
                return request;
            }
        }
        if (rpm < lower && current >= 2)
            driveTrain->desiredGear = current - 1;
        return request;
    }

    if (request != -2) {
        driveTrain->desiredGear = request;
        return request;
    }

    driveTrain->shiftWaitSeconds = driveTrain->shiftWaitSeconds + elapsed;
    if (driveTrain->clutchState != 3)
        return request;
    const float rpm = engine->angularVelocity * radiansToRpm;
    const float idle = engine->info->idleRpm;
    const float maximum = engine->info->maximumRpm;
    float lower;
    if (current >= 3) {
        const float overrev = maximum * driveTrain->tuning->downshiftOverrevMultiplier;
        const float currentProduct = overrev * driveTrain->info->gearRatios[current];
        lower = currentProduct / driveTrain->info->gearRatios[current - 1];
    } else if (engine->throttle < 0.20000000298023224f) {
        lower = idle + 100.0f;
    } else {
        lower = 0.0f;
    }

    const int32_t desired = driveTrain->desiredGear;
    const int32_t target = driveTrain->shiftTarget;
    if (desired != target) {
        driveTrain->previousGear = current;
        if (target < desired) {
            if (lower < rpm) {
                driveTrain->desiredGear = current;
                driveTrain->shiftWaitSeconds = 0.0f;
                driveTrain->shiftTarget = current;
                return request;
            }
            driveTrain->shiftWaitSeconds = 0.0f;
            driveTrain->desiredGear = current - 1;
        } else if (desired < target) {
            driveTrain->shiftWaitSeconds = 0.0f;
            driveTrain->desiredGear = current + 1;
        } else if (rpm < idle + 100.0f && current >= 3) {
            driveTrain->desiredGear = current - 1;
            request = -1;
        }
        return request;
    }

    if (10.0f < driveTrain->shiftWaitSeconds)
        return -1;
    if (driveTrain->previousGear < current && maximum - 300.0f < rpm)
        return -1;
    if (rpm < idle + 100.0f && current >= 3) {
        driveTrain->desiredGear = current - 1;
        request = -1;
    }
    return request;
}

#include "indep/inc/Replay.hpp"

// PAL18A8E8..18A988. The pursuit/catchup multiplier is transient; physical
// throttle, road torque, nitro timer and the compact enable flag are captured.
void engineSnapshot(Engine* engine,ReplaySnapshot* snapshot)
{
    if(snapshot->recording || snapshot->restoring)engine->catchupMultiplier=0.0f;
    replaySnapshotBeginSection(snapshot,nullptr,"Engine");
    replaySnapshotMoveFloat(snapshot,&engine->angularVelocity);
    replaySnapshotMoveFloat(snapshot,&engine->throttle);
    replaySnapshotMoveFloat(snapshot,&engine->roadTorque);
    replaySnapshotMoveFloat(snapshot,&engine->throttleOverride);
    replaySnapshotMoveFloat(snapshot,&engine->nitroBoostTimer);
    replaySnapshotMoveData(snapshot,&engine->nitroEnabled,1,4,0);
    replaySnapshotEndSection(snapshot);
}

// Full final248-byte PAL18AF10..18B008; beta omits the final previous-slip
// transfer. Native component links and authored shift tables stay borrowed.
void driveTrainSnapshot(DriveTrain* train,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"DriveTrain");
    replaySnapshotMoveFloat(snapshot,&train->clutch);
    replaySnapshotMoveData(snapshot,&train->desiredGear,4,-1,0);
    replaySnapshotMoveData(snapshot,&train->shiftTarget,4,-1,0);
    replaySnapshotMoveData(snapshot,&train->previousGear,4,-1,0);
    replaySnapshotMoveData(snapshot,&train->currentGear,4,-1,0);
    replaySnapshotMoveData(snapshot,&train->clutchState,4,-1,0);
    replaySnapshotMoveInteger(snapshot,&train->reengageRequested,4);
    replaySnapshotMoveFloat(snapshot,&train->drivenAngularVelocity);
    replaySnapshotMoveFloat(snapshot,&train->shiftWaitSeconds);
    replaySnapshotMoveFloat(snapshot,&train->previousSlipRpm);
    replaySnapshotEndSection(snapshot);
}
