#include "indep/inc/Replay.hpp"
#include "indep/inc/Wheel.hpp"
#include "indep/inc/CarState.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/Wheel.hpp"
#include "indep/inc/Skids.hpp"
static inline void wheelCopyFour(float output[4], const float input[4])
{
    const float x = input[0], y = input[1], z = input[2], w = input[3];
    output[0] = x; output[1] = y; output[2] = z; output[3] = w;
}

#include <math.h>
#include <string.h>

void wheelRestoreState(Wheel* wheel, const WheelState* state)
{
    wheel->motion = *state;
}

void wheelSaveState(const Wheel* wheel, WheelState* state)
{
    *state = wheel->motion;
}

float wheelGetAngle(const Wheel* wheel)
{
    return wheel->motion.angle;
}

float wheelGetTorqueResponse(const Wheel* wheel)
{
    return 1.0f / wheel->rotationalInertia;
}

float wheelGetAngularAcceleration(const Wheel* wheel)
{
    const float reaction = bScalarMultiply(wheel->reactionTorque, wheel->torqueInfo->reactionScale);
    const float total = bScalarAdd(wheel->accumulatedDriveTorque, -reaction);
    return bScalarDivide(total, wheel->rotationalInertia);
}

void wheelAddDriveTorque(Wheel* wheel, float torque)
{
    if (!wheel->rotationLocked)
        wheel->accumulatedDriveTorque = bScalarAdd(wheel->accumulatedDriveTorque, torque);
}

float wheelGetTractionLevel(const Wheel* wheel)
{
    return wheel->tractionLevel;
}

void wheelSetMotion(Wheel* wheel, float angle, float angularVelocity)
{
    wheel->motion.angularVelocity = angularVelocity;
    wheel->motion.angle = angle;
}

void wheelSetAngularVelocity(Wheel* wheel, float angularVelocity)
{
    wheel->motion.angularVelocity = angularVelocity;
}

float wheelGetRadius(const Wheel* wheel)
{
    return wheel->torqueInfo->reactionScale;
}

void wheelApplyForceMultiplier(Wheel* wheel, float value)
{
    wheel->forceMultiplier = wheel->forceMultiplier * value;
}

float wheelGetRotationalInertia(const Wheel* wheel) { return wheel->rotationalInertia; }
float wheelGetSteeringAngle(const Wheel* wheel) { return wheel->steeringAngle; }
void wheelSetSteeringAngle(Wheel* wheel, float value) { wheel->steeringAngle = value; }
int32_t wheelIsFront(const Wheel* wheel) { return wheel->torqueInfo->index < 2; }
int32_t wheelIsRear(const Wheel* wheel) { return !(wheel->torqueInfo->index < 2); }
float wheelGetCouplingFeedback(const Wheel* wheel) { return wheel->couplingFeedback; }
float wheelGetAngularSpeedEstimate(const Wheel* wheel) { return wheel->angularSpeedEstimate; }
void wheelSetContactCorrectionDisabled(Wheel* wheel, int8_t disabled)
{ wheel->contactCorrectionDisabled = disabled; }
int8_t wheelGetContactCorrectionDisabled(const Wheel* wheel)
{ return wheel->contactCorrectionDisabled; }
float* wheelContactNormal(Wheel* wheel) { return wheel->contactNormal; }

void wheelSetContactNormal(Wheel* wheel, const float normal[4])
{ wheelCopyFour(wheel->contactNormal, normal); }
void wheelSetContactPoint(Wheel* wheel, const float point[4])
{ wheelCopyFour(wheel->contactPoint, point); }
void wheelSetVelocity(Wheel* wheel, const float velocity[4])
{ wheelCopyFour(wheel->velocity, velocity); }
void wheelSetCarTransform(Wheel* wheel, const float transform[16])
{
    for (int i = 0; i < 4; ++i) wheelCopyFour(wheel->carTransform + 4 * i, transform + 4 * i);
}
void wheelSetWorldTransform(Wheel* wheel, const float transform[16])
{
    for (int i = 0; i < 4; ++i) wheelCopyFour(wheel->worldTransform + 4 * i, transform + 4 * i);
}
void wheelSetContactPresent(Wheel* wheel, int8_t contactPresent) { wheel->contactPresent = contactPresent; }
int8_t wheelGetContactPresent(const Wheel* wheel) { return wheel->contactPresent; }
void wheelSetMaterialIndex(Wheel* wheel, int32_t index) { wheel->materialIndex = index; }
int32_t wheelGetMaterialIndex(const Wheel* wheel) { return wheel->materialIndex; }
float wheelGetSurfaceOffset(const Wheel* wheel)
{ return wheel->contactPresent ? wheel->surfaceOffset : 0.0f; }
void wheelSetSlip(Wheel* wheel, const float slip[2])
{
    wheel->slip[0] = slip[0];
    wheel->slip[1] = slip[1];
    wheel->angularSpeedEstimate = slip[0];
}
float wheelGetSlipRatio(const Wheel* wheel) { return wheel->slipRatio; }
int32_t wheelIsRolling(const Wheel* wheel)
{ return !wheel->contactCorrectionDisabled && !wheel->rotationLocked; }
float wheelGetDriveTorque(const Wheel* wheel) { return wheel->accumulatedDriveTorque; }
void wheelSetBaseForceMultiplier(Wheel* wheel, float value) { wheel->baseForceMultiplier = value; }
float wheelGetHandbrakeTorque(const Wheel* wheel) { return wheel->handbrakeTorque; }

void wheelCorrectReactionTorque(Wheel* wheel, const float localAcceleration[2])
{
    const float radius = wheel->torqueInfo->reactionScale;
    const float previous = wheel->reactionTorque;
    const float reaction = bScalarMultiply(previous, radius);
    const float total = bScalarAdd(wheel->accumulatedDriveTorque, -reaction);
    const float first = bScalarMultiply(wheel->carTransform[0], localAcceleration[0]);
    const float second = bScalarMultiply(wheel->carTransform[4], localAcceleration[1]);
    const float acceleration = bScalarAdd(first, second);
    float correction = bScalarFiniteRatio(total, wheel->rotationalInertia);
    correction = bScalarMultiply(correction, radius);
    correction = bScalarAdd(correction, -acceleration);
    correction = bScalarMultiply(correction, wheel->rotationalInertia);
    correction = bScalarFiniteRatio(correction, radius);
    wheel->reactionTorque = bScalarAdd(previous, correction);
}

float* wheelCarReactionForce(const Wheel* wheel, float output[3])
{
    const float* matrix = wheel->carTransform;
    const float longitudinal = wheel->reactionTorque, lateral = wheel->lateralReactionForce;
    const float lateralZ = bScalarMultiply(matrix[6], lateral);
    const float longitudinalX = bScalarMultiply(matrix[0], longitudinal);
    const float lateralX = bScalarMultiply(matrix[4], lateral);
    const float longitudinalY = bScalarMultiply(matrix[1], longitudinal);
    const float lateralY = bScalarMultiply(matrix[5], lateral);
    const float longitudinalZ = bScalarMultiply(matrix[2], longitudinal);
    output[0] = bScalarAdd(longitudinalX, lateralX);
    output[1] = bScalarAdd(longitudinalY, lateralY);
    output[2] = bScalarAdd(longitudinalZ, lateralZ);
    return output;
}

void wheelEffectsTransformDirection(float output[3], const float matrix[16], const float input[3])
{
    for (int axis = 0; axis < 3; ++axis) {
        const float x = matrix[axis] * input[0];
        const float y = matrix[4 + axis] * input[1];
        const float z = matrix[8 + axis] * input[2];
        output[axis] = (x + y) + z;
    }
}

void wheelPrepareStep(Wheel* wheel)
{
    rotationMatrixIdentity(wheel->orientation);
    const float cosine = rotationSineRadians(bScalarAdd(wheel->motion.angle, 1.5707964897155762f));
    const float sine = rotationSineRadians(wheel->motion.angle);
    wheel->orientation[10] = cosine;
    wheel->orientation[2] = -sine;
    wheel->orientation[0] = cosine;
    wheel->orientation[8] = sine;

    wheel->reactionTorque = 0.0f;
    wheel->lateralReactionForce = 0.0f;
    wheel->accumulatedDriveTorque = 0.0f;
    wheel->angularSpeedEstimate = 0.0f;

    if (628.3179931640625f < wheel->motion.angle) {
        wheel->motion.angle = bScalarAdd(wheel->motion.angle, -628.3179931640625f);
        if (6283.1796875f < wheel->motion.angle)
            wheel->motion.angle = 0.0f;
    } else if (wheel->motion.angle < -628.3179931640625f) {
        wheel->motion.angle = bScalarAdd(wheel->motion.angle, 628.3179931640625f);
        if (wheel->motion.angle < -6283.1796875f)
            wheel->motion.angle = 0.0f;
    }

    if ((wheel->rotationDirection == 0 && wheel->motion.angularVelocity < 0.0f) ||
        (wheel->rotationDirection == 2 && 0.0f < wheel->motion.angularVelocity))
        wheel->motion.angularVelocity = 0.0f;

    if (0.000009999999747378752f < wheel->motion.angularVelocity)
        wheel->rotationDirection = 0;
    else if (wheel->motion.angularVelocity < -0.000009999999747378752f)
        wheel->rotationDirection = 2;
    else
        wheel->rotationDirection = 1;
}

int32_t wheelComputeDerivatives(const Wheel* wheel,
                                const WheelStepContext* context,
                                WheelState* derivatives)
{
    derivatives->angle = wheel->motion.angularVelocity;

    const float rolling = bScalarMultiply(wheel->motion.angularVelocity, context->rollingResistance);
    const float rollingTorque = bScalarMultiply(rolling, 0.0020000000949949026f);
    const float reaction = bScalarMultiply(wheel->reactionTorque, wheel->torqueInfo->reactionScale);
    const float steeringResistance = bScalarMultiply(wheel->steeringAngle, 0.10000000149011612f);
    const float steeringTorque = bScalarMultiply(wheel->motion.angularVelocity, steeringResistance);
    const float appliedTorque = bScalarAdd(wheel->accumulatedDriveTorque, -reaction);
    const float afterRolling = bScalarAdd(appliedTorque, -rollingTorque);
    const float totalTorque = bScalarAdd(afterRolling, -steeringTorque);
    derivatives->angularVelocity = bScalarDivide(totalTorque, wheel->rotationalInertia);

    if (wheel->rotationLocked)
        derivatives->angularVelocity = 0.0f;

    // Contact correction follows the lock branch in the original update;
    // locking therefore does not suppress this separate contact contribution.
    if (!wheel->contactCorrectionDisabled && 0.0010000000474974513f < context->elapsed) {
        const float distanceScale = bScalarMultiply(wheel->torqueInfo->reactionScale, context->elapsed);
        const float contactAcceleration = bScalarDivide(wheel->angularSpeedEstimate, distanceScale);
        derivatives->angularVelocity = bScalarAdd(derivatives->angularVelocity, -contactAcceleration);
    }
    return 0;
}

WheelState* wheelState(Wheel* wheel)
{
    return &wheel->motion;
}

int32_t wheelStateSize(const Wheel* wheel)
{
    (void)wheel;
    return 2;
}

float wheelGetAngularVelocity(const Wheel* wheel)
{
    return wheel->motion.angularVelocity;
}

void wheelSetForceMultiplier(Wheel* wheel, float value)
{
    wheel->forceMultiplier = value;
}

void wheelSetBrakeTorque(Wheel* wheel, float value)
{
    wheel->brakeTorque = value;
}

float* wheelContactPoint(Wheel* wheel) { return wheel->contactPoint; }

float* wheelContactForce(Wheel* wheel) { return wheel->worldContactForce; }

const float* wheelGetRestPositionInGeometryFrame(const Wheel* wheel)
{ return wheel->torqueInfo->mountPosition; }

float* wheelSlip(Wheel* wheel) { return wheel->slip; }

float* wheelOrientation(Wheel* wheel) { return wheel->orientation; }

float* wheelCarTransform(Wheel* wheel) { return wheel->carTransform; }

float wheelGetMass(const Wheel* wheel) { (void)wheel; return 1.0f; }

void wheelSetHandbrakeTorque(Wheel* wheel, float value) { wheel->handbrakeTorque = value; }

void vehicleStateInitializeWheelContact(VehicleState* state, Wheel* wheel,
                                        TerrainSurface* terrain,
                                        float angle, float angularVelocity,
                                        float travel, uint16_t steeringAngle)
{
    const int32_t index = wheel->torqueInfo->index;
    wheelSetMotion(wheel, angle, angularVelocity);
    state->steeringAngle = steeringAngle;
    state->wheelAngularVelocity[index] = angularVelocity;
    state->wheelAngle[index] = angle;
    state->suspensionTravel[index] = travel;
    vehicleStateUpdateWheelPose(state, index);

    float carWorldTransform[16];
    memcpy(carWorldTransform, state->orientation, sizeof(carWorldTransform));
    for (int i = 0; i < 3; ++i) carWorldTransform[12 + i] = state->position[i];
    carWorldTransform[15] = 1.0f;
    memcpy(wheel->carTransform, state->wheelVisualTransform[index], sizeof(wheel->carTransform));
    vectorMatrixMultiply(wheel->worldTransform, carWorldTransform, state->wheelVisualTransform[index]);

    float relative[4];
    for (int i = 0; i < 4; ++i)
        relative[i] = wheel->worldTransform[12 + i] - state->position[i];
    const float* angular = state->angularVelocity;
    wheel->velocity[0] = relative[1] * angular[2] - relative[2] * angular[1] + state->velocity[0];
    wheel->velocity[1] = relative[2] * angular[0] - relative[0] * angular[2] + state->velocity[1];
    wheel->velocity[2] = relative[0] * angular[1] - relative[1] * angular[0] + state->velocity[2];
    wheel->velocity[3] = relative[3] + state->velocity[3];

    const float height = terrainSurfaceElevation(terrain, wheel->worldTransform + 12,
                                                  &wheel->materialIndex, wheel->contactNormal, NULL);
    vehicleStateSetWheelContact(state, index, 1);
    vehicleStateSetWheelMaterial(state, index, wheel->materialIndex);
    wheel->contactPoint[0] = wheel->worldTransform[12];
    wheel->contactPoint[1] = wheel->worldTransform[13];
    wheel->contactPoint[2] = height;
    wheel->contactPoint[3] = 0.0f;
    wheel->contactPresent = 1;
}

// Complete PAL1A3F60. The old typed record transport is removed. Physical
// coefficients, pointers, inertia and surfaceOffset are deliberately retained.
void wheelSerializePersistentState(Wheel* wheel,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"Wheel");
    replaySnapshotMoveFloat(snapshot,&wheel->motion.angle);
    replaySnapshotMoveFloat(snapshot,&wheel->motion.angularVelocity);
    replaySnapshotMoveInteger(snapshot,&wheel->rotationDirection,4);
    if (snapshot->recording || snapshot->restoring) {
        memset(wheel->orientation, 0, sizeof(wheel->orientation));
        memset(wheel->worldTransform, 0, sizeof(wheel->worldTransform));
        memset(wheel->carTransform, 0, sizeof(wheel->carTransform));
        memset(wheel->velocity, 0, sizeof(wheel->velocity));
        wheel->slip[0] = 0; wheel->slip[1] = 0;
        wheel->accumulatedDriveTorque = 0;
        wheel->rotationLocked = 0;
        wheel->reactionTorque = 0; wheel->lateralReactionForce = 0;
        memset(wheel->worldContactForce, 0, sizeof(wheel->worldContactForce));
        memset(wheel->contactPoint, 0, sizeof(wheel->contactPoint));
        memset(wheel->contactNormal, 0, sizeof(wheel->contactNormal));
        wheel->materialIndex = 0;
        wheel->contactCorrectionDisabled = 0;
        wheel->contactPresent = 0;
        wheel->contactFlags = 0;
        wheel->tractionLevel = 0;
        wheel->angularSpeedEstimate = 0;
        wheel->couplingFeedback = 0;
        wheel->slipRatio = 0;
        wheel->brakeTorque = 0;
        wheel->handbrakeTorque = 0;
        wheel->forceMultiplier = 0;
        wheel->baseForceMultiplier = 0;
        wheel->steeringAngle = 0;
    }
    replaySnapshotEndSection(snapshot);
}

// Individual PAL1A5520 body moved intact from WheelEffects.c.
static float dotThree(const float a[3], const float b[3])
{
    const float x = a[0] * b[0], y = a[1] * b[1], z = a[2] * b[2];
    return (x + y) + z;
}

void wheelEffectsUpdate(const Wheel* wheel, const WheelContactContext* contact,
    WheelTrailStore* store, WheelTrail** attachment, const void* owner,
    const float ownerCenter[3], uint32_t flags, uint8_t disabledWheelMask)
{
    const int strong = 8.0f < wheel->tractionLevel * wheel->slipRatio;
    const float vx = wheel->velocity[0], vy = wheel->velocity[1];
    const float xx = vx * vx, yy = vy * vy;
    if ((!wheel->contactCorrectionDisabled && !strong) || !wheel->contactPresent ||
        ((disabledWheelMask >> wheel->torqueInfo->index) & 1u) || !(1.0f < sqrtf(xx + yy))) {
        wheelTrailDetach(store, attachment);
        return;
    }
    float perpendicular[4] = {-vy, vx, 0.0f, 0.0f};
    const float inverse = finiteRatio(1.0f, sqrtf(dotThree(perpendicular, perpendicular)));
    for (int axis = 0; axis < 3; ++axis) perpendicular[axis] = perpendicular[axis] * inverse;
    const float side[3] = {0.0f, 1.0f, 0.0f};
    float worldSide[3];
    wheelEffectsTransformDirection(worldSide, wheel->worldTransform, side);
    const float alignment = fabsf(dotThree(worldSide, perpendicular));
    const TerrainMaterial* material = contact->materials + wheel->materialIndex;
    float halfWidth = wheel->torqueInfo->halfWidth * material->tireEffectFriction;
    halfWidth = halfWidth * (alignment < 0.5f ? 0.5f : alignment);
    halfWidth = halfWidth * 0.5f;
    vectorSetLength3(perpendicular, perpendicular, halfWidth);
    float first[4], second[4];
    for (int axis = 0; axis < 4; ++axis) {
        first[axis] = wheel->worldTransform[12 + axis] + perpendicular[axis];
        second[axis] = wheel->worldTransform[12 + axis] - perpendicular[axis];
    }
    const float firstHeight = terrainSurfaceElevation(contact->terrain, first, NULL, NULL, NULL);
    const float secondHeight = terrainSurfaceElevation(contact->terrain, second, NULL, NULL, NULL);
    const float firstHalf = firstHeight * 0.5f, secondHalf = secondHeight * 0.5f;
    perpendicular[2] = firstHalf - secondHalf;
    float position[4] = {wheel->worldTransform[12], wheel->worldTransform[13],
                         (firstHeight + secondHeight) * 0.5f, 0.0f};
    vectorSetLength3(perpendicular, perpendicular, halfWidth);
    float intensity = alignment;
    if (!wheel->contactCorrectionDisabled) intensity = intensity * 0.3333333432674408f;
    if (wheel->materialIndex == 4) intensity = 0.0f;
    else if (intensity < 0.30000001192092896f) intensity = 0.30000001192092896f;
    wheelTrailUpdate(store, attachment, owner, ownerCenter, flags, position,
                     perpendicular, wheel->materialIndex, intensity);
}


static const float legacySamples[7] = {0.0f, 0.4000000059604645f, 0.800000011920929f,
    1.2000000476837158f, 1.600000023841858f, 1.75f, 1.649999976158142f};
static const float loadSamples[7][6] = {
    {0, 0, 0, 0, 0, 0},
    {0, 1.2000000476837158f, 2.299999952316284f, 3, 3, 2.799999952316284f},
    {0, 1.7000000476837158f, 3.200000047683716f, 4.300000190734863f, 5.099999904632568f, 5.199999809265137f},
    {0, 1.7999999523162842f, 3.5f, 4.900000095367432f, 5.800000190734863f, 6.099999904632568f},
    {0, 1.8300000429153442f, 3.5999999046325684f, 5, 5.960000038146973f, 6.400000095367432f},
    {0, 1.8600000143051147f, 3.700000047683716f, 5.099999904632568f, 6.130000114440918f, 6.699999809265137f},
    {0, 1.899999976158142f, 3.799999952316284f, 5.199999809265137f, 6.300000190734863f, 7.099999904632568f}
};

const WheelContactTuning wheelContactDefaultTuning = {
    {legacySamples, 7, 0, 0.22340217232704163f, 26.857393264770508f},
    {{loadSamples[0], 6, 0, 10, 0.5f}, {loadSamples[1], 6, 0, 10, 0.5f},
     {loadSamples[2], 6, 0, 10, 0.5f}, {loadSamples[3], 6, 0, 10, 0.5f},
     {loadSamples[4], 6, 0, 10, 0.5f}, {loadSamples[5], 6, 0, 10, 0.5f},
     {loadSamples[6], 6, 0, 10, 0.5f}},
    0.009999999776482582f, 4, 10, 1.2000000476837158f, 0.10000000149011612f,
    1, 0.8500000238418579f, 1, 2.5f, 0.800000011920929f, 1, 1, 1.5f,
    0.20000000298023224f, 0, 0.15000000596046448f, 0.0860000029206276f, 0
};

static float wheelContactDotThree(const float first[3], const float second[3])
{
    const float x = bScalarMultiply(first[0], second[0]);
    const float y = bScalarMultiply(first[1], second[1]);
    const float z = bScalarMultiply(first[2], second[2]);
    return bScalarAdd(bScalarAdd(x, y), z);
}

static void crossThree(float output[4], const float first[3], const float second[3])
{
    // OPMULA/OPMSUB retain the source operand order of both products.
    const float x = bScalarAdd(bScalarMultiply(first[1], second[2]), -bScalarMultiply(second[1], first[2]));
    const float y = bScalarAdd(bScalarMultiply(first[2], second[0]), -bScalarMultiply(second[2], first[0]));
    const float z = bScalarAdd(bScalarMultiply(first[0], second[1]), -bScalarMultiply(second[0], first[1]));
    output[0] = x; output[1] = y; output[2] = z; output[3] = 0;
}

static void localVelocity(const Wheel* wheel, float output[4])
{
    float inverse[16];
    vectorMatrixTranspose(inverse, wheel->worldTransform);
    inverse[3] = 0; inverse[7] = 0; inverse[11] = 0;
    vectorMatrixTransformPoint(output, inverse, wheel->velocity);
}

static float slipAngle(const float velocity[4])
{
    float angle = bScalarMultiply((float)vectorAtan2Angle(velocity[0], velocity[1]), 9.58738019107841e-5f);
    if (3.141590118408203f < angle) angle = bScalarAdd(angle, -6.283180236816406f);
    if (velocity[0] < 0) {
        angle = bScalarAdd(angle, 3.141590118408203f);
        if (3.141590118408203f < angle) angle = bScalarAdd(angle, -6.283180236816406f);
    }
    return angle;
}

Wheel* wheelInitialize(Wheel* wheel, const WheelTorqueInfo* info,
                       const WheelTireInfo* tire, const WheelBrakeInfo* brakes,
                       const WheelVehicleState* vehicle)
{
    wheel->torqueInfo = info;
    wheel->tireInfo = tire;
    wheel->brakeInfo = brakes;
    wheel->vehicle = vehicle;
    wheel->rotationDirection = 1;
    wheel->baseForceMultiplier = 1;
    wheel->forceMultiplier = 1;
    wheel->contactFlags = 0;
    wheel->couplingFeedback = 0;
    wheel->surfaceOffset = 0;
    return wheel;
}



void wheelInitializeRotationalInertia(Wheel* wheel, const WheelContactTuning* tuning)
{ wheel->rotationalInertia = tuning->rotationalInertia; }

float wheelSetContactLoad(Wheel* wheel, int32_t contactPresent, float load)
{
    wheel->contactPresent = (int8_t)contactPresent;
    if (contactPresent) {
        const float offset = -wheel->torqueInfo->reactionScale;
        for (int i = 0; i < 4; ++i)
            wheel->contactPoint[i] = bScalarAdd(bScalarMultiply(wheel->contactNormal[i], offset), wheel->worldTransform[12 + i]);
    }
    if (0 < load) {
        float lateralNormal[4], projectedNormal[4];
        crossThree(lateralNormal, wheel->worldTransform + 4, wheel->contactNormal);
        crossThree(projectedNormal, lateralNormal, wheel->worldTransform + 4);
        float alignment = bScalarAdd(bScalarMultiply(wheelContactDotThree(projectedNormal, wheel->contactNormal), 4.0f), -3.0f);
        if (alignment < 0.30000001192092896f) alignment = 0.30000001192092896f;
        wheel->tractionLevel = bScalarMultiply(load, alignment);
    } else wheel->tractionLevel = 0;
    return 0;
}

float wheelGroundDisplacement(Wheel* wheel, const WheelContactContext* context,
                               const float worldPoint[4], const float bodyOrientation[16])
{
    return wheelQueryGroundDisplacement(wheel, context, wheel->contactNormal,
                                        &wheel->materialIndex, worldPoint, bodyOrientation);
}

float wheelQueryGroundDisplacement(Wheel* wheel, const WheelContactContext* context,
                                    float normal[4], int32_t* materialIndex,
                                    const float worldPoint[4], const float bodyOrientation[16])
{
    const float height = terrainSurfaceElevation(context->terrain, worldPoint, materialIndex, normal, NULL);
    float surfacePoint[4] = {worldPoint[0], worldPoint[1], height, 0};
    const float displacement = terrainMaterialDisplacement(context->materials + *materialIndex, surfacePoint);
    wheel->surfaceOffset = displacement;
    surfacePoint[2] = bScalarAdd(surfacePoint[2], displacement);
    const float alignment = wheelContactDotThree(normal, bodyOrientation + 8);
    if (alignment < 0.0010000000474974513f) {
        *materialIndex = 0;
        return -100;
    }
    float difference[4];
    for (int i = 0; i < 4; ++i) difference[i] = bScalarAdd(surfacePoint[i], -worldPoint[i]);
    return bScalarFiniteRatio(wheelContactDotThree(normal, difference), alignment);
}

void wheelUpdateBrakeLock(Wheel* wheel, const WheelContactContext* context, float availableGrip)
{
    const WheelContactTuning* tuning = context->tuning;
    float service = bScalarMultiply(tuning->serviceBrakeScale, wheel->brakeTorque);
    float handbrake = bScalarMultiply(tuning->handbrakeScale, wheel->handbrakeTorque);
    handbrake = bScalarMultiply(handbrake, wheel->brakeInfo->handbrakeTorque);
    service = bScalarMultiply(service, wheel->brakeInfo->serviceTorque);
    const float total = bScalarAdd(service, handbrake);
    const float threshold = bScalarMultiply(total, tuning->brakeLockScale);
    float resistance = bScalarMultiply(fabsf(wheel->motion.angularVelocity), tuning->angularBrakeResistance);
    availableGrip = bScalarMultiply(availableGrip, wheel->torqueInfo->reactionScale);
    availableGrip = bScalarAdd(availableGrip, resistance);
    if (availableGrip < threshold) {
        wheel->rotationLocked = 1 < threshold;
        wheel->motion.angularVelocity = 0;
    } else wheel->rotationLocked = 0;
}

float wheelHighSpeedGrip(const Wheel* wheel, const WheelContactTuning* tuning)
{
    if (wheel->rotationLocked || wheel->motion.angularVelocity < 0 || wheel->torqueInfo->index < 2) return 1;
    float fraction = bScalarMultiply(bScalarAdd(wheel->vehicle->speed, -13.410818099975586f), 0.11185000091791153f);
    if (1 < fraction) fraction = 1;
    else if (fraction < 0) fraction = 0;
    const double base = (double)tuning->highSpeedGrip;
    const double difference = 1.0 - base;
    const double contribution = difference * (double)fraction;
    return (float)(base + contribution);
}

float wheelEvaluateLateralForce(const Wheel* wheel, const WheelContactTuning* tuning,
                                 float load, float absoluteSlipAngle)
{
    if (!tuning->useLoadLateralCurves) {
        const float sample = sampledCurveEvaluate(&tuning->legacyLateralCurve, absoluteSlipAngle);
        const float scaledSample = bScalarMultiply(tuning->legacyLateralScale, sample);
        const float scaledLoad = bScalarMultiply(wheel->tractionLevel, tuning->gripScale);
        return bScalarMultiply(scaledSample, scaledLoad);
    }
    const float angleScale = bScalarFiniteRatio(12.0f, wheel->tireInfo->slipAngleLimit);
    float coordinate = bScalarMultiply(absoluteSlipAngle, 57.2957763671875f);
    coordinate = bScalarMultiply(coordinate, angleScale);
    coordinate = bScalarMultiply(coordinate, 0.5f);
    const int32_t index = (int32_t)coordinate;
    const float fraction = bScalarAdd(coordinate, -(float)index);
    float scaledLoad = bScalarMultiply(load, tuning->lateralLoadScale);
    scaledLoad = bScalarFiniteRatio(scaledLoad, wheel->tireInfo->referenceLoad);
    float sample;
    if (index < 6) {
        const float first = sampledCurveEvaluate(&tuning->loadLateralCurves[index], scaledLoad);
        const float second = sampledCurveEvaluate(&tuning->loadLateralCurves[index + 1], scaledLoad);
        const float difference = bScalarAdd(second, -first);
        sample = bScalarAdd(first, bScalarMultiply(fraction, difference));
    } else sample = sampledCurveEvaluate(&tuning->loadLateralCurves[6], scaledLoad);
    const float scale = bScalarMultiply(wheel->tireInfo->lateralScale, tuning->loadLateralScale);
    return bScalarMultiply(scale, sample);
}

void wheelComputeContactForces(Wheel* wheel, const WheelContactContext* context,
                                int32_t preserveDriveTorque)
{
    const WheelContactTuning* tuning = context->tuning;
    const float angularVelocity = wheel->motion.angularVelocity;
    float service = bScalarMultiply(tuning->serviceBrakeScale, wheel->brakeTorque);
    float handbrake = bScalarMultiply(tuning->handbrakeScale, wheel->handbrakeTorque);
    handbrake = bScalarMultiply(handbrake, wheel->brakeInfo->handbrakeTorque);
    service = bScalarMultiply(service, wheel->brakeInfo->serviceTorque);
    if (0 < angularVelocity) service = -service;
    if (!wheel->rotationLocked) wheel->accumulatedDriveTorque = bScalarAdd(wheel->accumulatedDriveTorque, service);
    const float signedHandbrake = 0 < angularVelocity ? -handbrake : handbrake;
    if (!wheel->rotationLocked) wheel->accumulatedDriveTorque = bScalarAdd(wheel->accumulatedDriveTorque, signedHandbrake);
    float velocity[4];
    localVelocity(wheel, velocity);
    const float angle = slipAngle(velocity);
    const float longitudinalSlip = bScalarAdd(bScalarMultiply(angularVelocity, wheel->torqueInfo->reactionScale), -velocity[0]);
    wheel->angularSpeedEstimate = longitudinalSlip;
    wheel->couplingFeedback = longitudinalSlip;
    if (longitudinalSlip < -0.5f && wheel->accumulatedDriveTorque < 0 &&
        handbrake == 0 && !preserveDriveTorque)
        wheel->accumulatedDriveTorque = bScalarMultiply(wheel->accumulatedDriveTorque, 0.15000000596046448f);
    const float slipSquared = bScalarMultiply(longitudinalSlip, longitudinalSlip);
    const float lateralSquared = bScalarMultiply(velocity[1], velocity[1]);
    const float totalSlip = bScalarSquareRoot(bScalarAdd(slipSquared, lateralSquared));
    int sliding = 0;
    float slidingScale = 0;
    float grip = 1;
    if (totalSlip != 0 && (velocity[1] != 0 || velocity[0] != 0)) {
        float tireGrip = bScalarMultiply(wheel->tireInfo->slidingGrip, wheel->baseForceMultiplier);
        const float surfaceGrip = bScalarMultiply(wheel->forceMultiplier, context->materials[wheel->materialIndex].slidingGrip);
        grip = bScalarMultiply(tireGrip, surfaceGrip);
        if (wheel->vehicle->pursuitMode == 1) grip = bScalarMultiply(grip, wheelHighSpeedGrip(wheel, tuning));
        float maximumForce = bScalarMultiply(tuning->gripScale, wheel->tractionLevel);
        maximumForce = bScalarMultiply(maximumForce, grip);
        slidingScale = bScalarFiniteRatio(maximumForce, totalSlip);
        const float speed = bScalarSquareRoot(bScalarAdd(bScalarMultiply(velocity[0], velocity[0]), lateralSquared));
        float availableGrip = bScalarFiniteRatio(maximumForce, speed);
        availableGrip = bScalarMultiply(fabsf(velocity[0]), availableGrip);
        wheelUpdateBrakeLock(wheel, context, availableGrip);
        sliding = 0.5f < fabsf(longitudinalSlip) ||
                  wheel->tireInfo->slipAngleLimit < fabsf(angle) || wheel->rotationLocked;
    }
    if (sliding) {
        wheel->contactCorrectionDisabled = 1;
        wheel->reactionTorque = bScalarMultiply(slidingScale, longitudinalSlip);
        wheel->lateralReactionForce = bScalarMultiply(-slidingScale, velocity[1]);
        if (bScalarMultiply(wheel->vehicle->speed, 2.236999988555908f) < 1 && 0.10000000149011612f < grip) {
            wheel->lateralReactionForce = bScalarFiniteRatio(wheel->lateralReactionForce, grip);
            wheel->reactionTorque = bScalarFiniteRatio(wheel->reactionTorque, grip);
        }
        wheel->slip[0] = longitudinalSlip;
        wheel->slip[1] = velocity[1];
    } else {
        wheel->contactCorrectionDisabled = 0;
        wheel->reactionTorque = bScalarFiniteRatio(wheel->accumulatedDriveTorque, wheel->torqueInfo->reactionScale);
        const float direction = 0 < velocity[0] ? 1.0f : -1.0f;
        const float load = bScalarMultiply(tuning->gripScale, wheel->tractionLevel);
        float lateral = wheelEvaluateLateralForce(wheel, tuning, load, fabsf(angle));
        if (0 < angle) lateral = -lateral;
        wheel->lateralReactionForce = bScalarMultiply(direction, lateral);
        wheel->slip[0] = 0; wheel->slip[1] = 0;
    }
}

void wheelResolveContactForce(Wheel* wheel, const WheelContactContext* context)
{
    const WheelContactTuning* tuning = context->tuning;
    const int driven = 0 < wheel->accumulatedDriveTorque && !wheel->rotationLocked;
    if (driven) wheel->reactionTorque = bScalarMultiply(wheel->reactionTorque, tuning->drivenReactionScale);
    const float reactionSquared = bScalarMultiply(wheel->reactionTorque, wheel->reactionTorque);
    const float lateralSquared = bScalarMultiply(wheel->lateralReactionForce, wheel->lateralReactionForce);
    const float magnitude = bScalarSquareRoot(bScalarAdd(reactionSquared, lateralSquared));
    const TerrainMaterial* material = context->materials + wheel->materialIndex;
    const float materialGrip = bScalarMultiply(material->staticGrip, wheel->tireInfo->staticGrip);
    float grip = bScalarMultiply(tuning->gripScale, wheel->baseForceMultiplier);
    grip = bScalarMultiply(grip, materialGrip);
    const float scaledLoad = bScalarMultiply(wheel->tractionLevel, wheel->forceMultiplier);
    float maximumForce = bScalarMultiply(grip, scaledLoad);
    if (wheel->vehicle->pursuitMode == 1) maximumForce = bScalarMultiply(maximumForce, wheelHighSpeedGrip(wheel, tuning));
    if (maximumForce < magnitude && 0.0010000000474974513f < magnitude) {
        const float scale = bScalarFiniteRatio(maximumForce, magnitude);
        wheel->reactionTorque = bScalarMultiply(wheel->reactionTorque, scale);
        wheel->lateralReactionForce = bScalarMultiply(wheel->lateralReactionForce, scale);
    } else if (driven) wheel->reactionTorque = bScalarFiniteRatio(wheel->reactionTorque, tuning->drivenReactionScale);
    if (0.0010000000474974513f < maximumForce) {
        wheel->slipRatio = bScalarFiniteRatio(magnitude, maximumForce);
        if (1 < wheel->slipRatio) wheel->slipRatio = 1;
    } else wheel->slipRatio = 1;
    if (!wheelIsRolling(wheel)) wheel->slipRatio = 1;
    if (!wheel->contactPresent) wheel->slipRatio = 0;
    float velocity[4];
    localVelocity(wheel, velocity);
    const float angle = slipAngle(velocity);
    const float lateralX = bScalarMultiply(rotationSineRadians(angle), wheel->lateralReactionForce);
    if (1 < fabsf(velocity[0]))
        wheel->reactionTorque = bScalarAdd(wheel->reactionTorque, -bScalarMultiply(lateralX, 0.5f));
    float frame[16];
    rotationMatrixIdentity(frame);
    if (tuning->forceFrameMode == 1 || tuning->forceFrameMode == 4) {
        for (int i = 0; i < 12; ++i) frame[i] = wheel->worldTransform[i];
        if (tuning->forceFrameMode == 4) {
            for (int i = 0; i < 4; ++i) frame[8 + i] = wheel->contactNormal[i];
            frame[11] = 0;
        }
    } else if (tuning->forceFrameMode == 2 || tuning->forceFrameMode == 3) {
        const float firstProjection = -wheelContactDotThree(wheel->contactNormal, wheel->worldTransform);
        for (int i = 0; i < 4; ++i)
            frame[i] = bScalarAdd(bScalarMultiply(wheel->contactNormal[i], firstProjection), wheel->worldTransform[i]);
        frame[3] = 0;
        if (tuning->forceFrameMode == 2) {
            const float secondProjection = -wheelContactDotThree(wheel->contactNormal, wheel->worldTransform + 4);
            for (int i = 0; i < 4; ++i)
                frame[4 + i] = bScalarAdd(bScalarMultiply(wheel->contactNormal[i], secondProjection), wheel->worldTransform[4 + i]);
            frame[7] = 0;
        } else {
            const float inverseLength = bScalarFiniteRatio(1.0f, bScalarSquareRoot(wheelContactDotThree(frame, frame)));
            for (int i = 0; i < 4; ++i) frame[i] = bScalarMultiply(frame[i], inverseLength);
            frame[3] = 0;
            crossThree(frame + 4, wheel->contactNormal, frame);
        }
        for (int i = 0; i < 4; ++i) frame[8 + i] = wheel->contactNormal[i];
        frame[11] = 0;
    }
    const float localForce[4] = {wheel->reactionTorque, wheel->lateralReactionForce, wheel->tractionLevel, 0};
    vectorMatrixTransformPoint(wheel->worldContactForce, frame, localForce);
}

float* wheelWorldReactionForce(const Wheel* wheel, float output[4])
{
    float frame[16];
    for (int i = 0; i < 12; ++i) frame[i] = wheel->worldTransform[i];
    frame[12] = 0; frame[13] = 0; frame[14] = 0; frame[15] = 1;
    const float force[4] = {wheel->reactionTorque, wheel->lateralReactionForce, 0, 1};
    vectorMatrixTransformPoint(output, frame, force);
    return output;
}
