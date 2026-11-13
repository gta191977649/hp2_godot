#include "indep/inc/RigidBody.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>
#include <stddef.h>
#include <string.h>

// Private RigidBody operations. PAL19D848 and its impulse/derivative
// callers own this matrix chain.
static inline void rigidBodyCopyDirection(float output[4], const float input[4])
{
    for (int i = 0; i < 3; ++i) output[i] = input[i];
    output[3] = 0.0f;
}

static inline void rigidBodyCopyPoint(float output[4], const float input[4])
{
    for (int i = 0; i < 3; ++i) output[i] = input[i];
    output[3] = 1.0f;
}

static inline void rigidBodyZeroVector(float output[4])
{
    for (int i = 0; i < 4; ++i) output[i] = 0.0f;
}

static inline void rigidBodyCopyMatrix(float output[16], const float input[16])
{
    for (int i = 0; i < 16; ++i) output[i] = input[i];
}

static inline float rigidBodyDotThree(const float left[4], const float right[4])
{
    const float x = bScalarMultiply(left[0], right[0]);
    const float y = bScalarMultiply(left[1], right[1]);
    const float z = bScalarMultiply(left[2], right[2]);
    const float xy = bScalarAdd(x, y);
    return bScalarAdd(xy, z);
}

static inline float rigidBodyLengthThree(const float value[4])
{
    return bScalarSquareRoot(rigidBodyDotThree(value, value));
}

static inline void rigidBodyCrossThree(float output[4], const float left[4], const float right[4])
{
    const float x = bScalarMultiply(left[1], right[2]);
    const float y = bScalarMultiply(left[2], right[0]);
    const float z = bScalarMultiply(left[0], right[1]);
    const float secondX = bScalarMultiply(right[1], left[2]);
    const float secondY = bScalarMultiply(right[2], left[0]);
    const float secondZ = bScalarMultiply(right[0], left[1]);
    output[0] = bScalarAdd(x, -secondX);
    output[1] = bScalarAdd(y, -secondY);
    output[2] = bScalarAdd(z, -secondZ);
    output[3] = 0.0f;
}

static inline void rigidBodyLimitVector(float output[4], const float input[4], float limit)
{
    rigidBodyCopyDirection(output, input);
    const float length = rigidBodyLengthThree(input);
    if (limit < length) {
        const float scale = bScalarDivide(limit, length);
        for (int i = 0; i < 3; ++i) output[i] = bScalarMultiply(output[i], scale);
    }
}

static inline void rigidBodyWorldInverseInertia(const RigidBody* body, float output[16])
{
    float transpose[16], intermediate[16];
    vectorMatrixTranspose(transpose, body->motion.orientation);
    vectorMatrixMultiply(intermediate, body->motion.orientation, body->inverseInertia);
    vectorMatrixMultiply(output, intermediate, transpose);
}

void rigidBodyRefreshMass(RigidBody* body)
{
    body->mass = body->info->mass;
    body->inverseMass = bScalarDivide(1.0f, body->info->mass);
    vectorMatrixInverseAffine(body->inverseInertia, body->info->inertia);
}

void rigidBodySetCenterOfMass(RigidBody* body, const float center[4])
{
    if (center != NULL) rigidBodyCopyPoint(body->centerOfMass, center);
}

void rigidBodyApplyForce(RigidBody* body, const float force[4], const float lever[4])
{
    float limited[4], moment[4], limitedMoment[4];
    rigidBodyLimitVector(limited, force, 1200.0f);
    for (int i = 0; i < 3; ++i) body->force[i] = bScalarAdd(body->force[i], limited[i]);
    body->force[3] = 0.0f;
    // The moment uses the original force, independently of its linear cap.
    rigidBodyCrossThree(moment, lever, force);
    rigidBodyLimitVector(limitedMoment, moment, 100.0f);
    for (int i = 0; i < 3; ++i) body->torque[i] = bScalarAdd(body->torque[i], limitedMoment[i]);
    body->torque[3] = 0.0f;
}

void rigidBodyAddTorque(RigidBody* body, const float torque[4])
{
    for (int i = 0; i < 3; ++i) body->torque[i] = bScalarAdd(body->torque[i], torque[i]);
    body->torque[3] = 0.0f;
}

void rigidBodyApplyImpulse(RigidBody* body, const float impulse[4], const float lever[4])
{
    float limited[4], moment[4], limitedMoment[4];
    rigidBodyLimitVector(limited, impulse, 1000.0f);
    for (int i = 0; i < 3; ++i)
        body->motion.linearMomentum[i] = bScalarAdd(body->motion.linearMomentum[i], limited[i]);
    body->motion.linearMomentum[3] = 0.0f;
    rigidBodyCrossThree(moment, lever, limited);
    rigidBodyLimitVector(limitedMoment, moment, 10.0f);
    for (int i = 0; i < 3; ++i)
        body->motion.angularMomentum[i] = bScalarAdd(body->motion.angularMomentum[i], limitedMoment[i]);
    body->motion.angularMomentum[3] = 0.0f;
}

void rigidBodyApplyScaledAngularImpulse(RigidBody* body, const float impulse[4],
                                        const float lever[4], float angularScale)
{
    float limited[4], moment[4], limitedMoment[4];
    rigidBodyLimitVector(limited, impulse, 1000.0f);
    for (int i = 0; i < 3; ++i)
        body->motion.linearMomentum[i] = bScalarAdd(body->motion.linearMomentum[i], limited[i]);
    body->motion.linearMomentum[3] = 0.0f;
    rigidBodyCrossThree(moment, lever, limited);
    rigidBodyLimitVector(limitedMoment, moment, 10.0f);
    for (int i = 0; i < 3; ++i) {
        const float change = bScalarMultiply(limitedMoment[i], angularScale);
        body->motion.angularMomentum[i] = bScalarAdd(body->motion.angularMomentum[i], change);
    }
    body->motion.angularMomentum[3] = 0.0f;
}

float rigidBodyKineticEnergy(const RigidBody* body)
{
    const float linear = bScalarMultiply(rigidBodyDotThree(body->motion.linearMomentum, body->linearVelocity), 0.5f);
    const float angular = bScalarMultiply(rigidBodyDotThree(body->motion.angularMomentum, body->angularVelocity), 0.5f);
    return bScalarAdd(linear, angular);
}

void rigidBodyOrientationDerivatives(float output[16], const float angularVelocity[4],
                                     const float orientation[16])
{
    for (int column = 0; column < 3; ++column) {
        const int offset = column * 4;
        const float xFirst = bScalarMultiply(-angularVelocity[2], orientation[offset + 1]);
        const float xSecond = bScalarMultiply(angularVelocity[1], orientation[offset + 2]);
        output[offset] = bScalarAdd(xFirst, xSecond);
        const float yFirst = bScalarMultiply(angularVelocity[2], orientation[offset]);
        const float ySecond = bScalarMultiply(angularVelocity[0], orientation[offset + 2]);
        output[offset + 1] = bScalarAdd(yFirst, -ySecond);
        const float zFirst = bScalarMultiply(-angularVelocity[1], orientation[offset]);
        const float zSecond = bScalarMultiply(angularVelocity[0], orientation[offset + 1]);
        output[offset + 2] = bScalarAdd(zFirst, zSecond);
        output[offset + 3] = 0.0f;
    }
    output[12] = 0.0f;
    output[13] = 0.0f;
    output[14] = 0.0f;
    // This is a derivative: the constant affine bottom row has zero rate.
    output[15] = 0.0f;
}

void rigidBodySetAngularMomentum(RigidBody* body, const float momentum[4])
{ rigidBodyCopyDirection(body->motion.angularMomentum, momentum); }
void rigidBodySetOrientation(RigidBody* body, const float orientation[16])
{ rigidBodyCopyMatrix(body->motion.orientation, orientation); }
void rigidBodySetPosition(RigidBody* body, const float position[4])
{ rigidBodyCopyPoint(body->motion.position, position); }
void rigidBodySetDampingEnabled(RigidBody* body, int32_t enabled)
{ body->dampingEnabled = enabled; }
void rigidBodySetPursuitSpeedEnabled(RigidBody* body, int32_t enabled)
{ body->pursuitSpeedEnabled = enabled; }
float rigidBodyInverseMass(const RigidBody* body) { return body->inverseMass; }
float* rigidBodyForce(RigidBody* body) { return body->force; }
float rigidBodyYawInertia(const RigidBody* body) { return bScalarDivide(1.0f, body->inverseInertia[10]); }
float* rigidBodyCenterOfMass(RigidBody* body) { return body->centerOfMass; }
float* rigidBodyAngularMomentum(RigidBody* body) { return body->motion.angularMomentum; }
const float* rigidBodyAngularVelocity(const RigidBody* body) { return body->angularVelocity; }
float* rigidBodyLinearMomentum(RigidBody* body) { return body->motion.linearMomentum; }
const float* rigidBodyLinearVelocity(const RigidBody* body) { return body->linearVelocity; }
void rigidBodyRestoreState(RigidBody* body, const RigidBodyState* state)
{
    body->motion = *state;
    body->motion.position[3] = 1.0f;
    body->motion.linearMomentum[3] = body->motion.angularMomentum[3] = 0.0f;
}
void rigidBodySaveState(const RigidBody* body, RigidBodyState* state)
{ *state = body->motion; }

static const float angularDampingSamples[6] = {
    0.05000000074505806f, 0.05000000074505806f, 0.25f,
    0.699999988079071f, 2.0f, 5.0f
};
static const float dampingTargetSamples[8] = {
    1.5f, 1.0f, 0.6000000238418579f, 0.2199999988079071f,
    0.05999999865889549f, 0.029999999329447746f,
    0.014999999664723873f, 0.0f
};
const RigidBodyTuning rigidBodyDefaultTuning = {
    {angularDampingSamples, 6, 0.0f, 10.0f, 0.5f},
    {dampingTargetSamples, 8, 0.0f, 1.0f, 7.0f},
    9.800000190734863f, 1.0299999713897705f,
    0.30000001192092896f, 0.8999999761581421f,
    0.30000001192092896f, 1.100000023841858f,
    5.0f, -0.5f, 4.0f, 2.0f, 0.10000000149011612f,
    10.0f, 2.0f, 8.0f
};

RigidBody* rigidBodyInitialize(RigidBody* body, const RigidBodyInfo* info,
                               const RigidBodyTuning* tuning)
{
    body->info = info;
    body->tuning = tuning;
    rigidBodyZeroVector(body->motion.position);
    body->motion.position[3] = 1.0f;
    rotationMatrixIdentity(body->motion.orientation);
    rigidBodyZeroVector(body->motion.linearMomentum);
    rigidBodyZeroVector(body->motion.angularMomentum);
    body->motion.damping = 0.0f;
    rigidBodyZeroVector(body->linearVelocity);
    rigidBodyZeroVector(body->angularVelocity);
    rigidBodyZeroVector(body->force);
    rigidBodyZeroVector(body->torque);
    body->yawTorqueLimit = 0.0f;
    body->forwardSpeedLimit = 0.0f;
    body->pursuitSpeedEnabled = 0;
    body->speedLimitScale = 1.0f;
    body->dampingEnabled = 1;
    rigidBodyRefreshMass(body);
    return body;
}

void rigidBodySetMotion(RigidBody* body, const float position[4],
                        const float orientation[16], const float linearVelocity[4],
                        const float angularVelocity[4])
{
    rigidBodyCopyPoint(body->motion.position, position);
    rigidBodyCopyMatrix(body->motion.orientation, orientation);
    for (int i = 0; i < 3; ++i)
        body->motion.linearMomentum[i] = bScalarMultiply(linearVelocity[i], body->mass);
    body->motion.linearMomentum[3] = 0.0f;
    float inertia[16], transpose[16], intermediate[16], worldInertia[16];
    rotationMatrixIdentity(inertia);
    inertia[0] = bScalarDivide(1.0f, body->inverseInertia[0]);
    inertia[5] = bScalarDivide(1.0f, body->inverseInertia[5]);
    inertia[10] = bScalarDivide(1.0f, body->inverseInertia[10]);
    vectorMatrixTranspose(transpose, body->motion.orientation);
    vectorMatrixMultiply(intermediate, body->motion.orientation, inertia);
    vectorMatrixMultiply(worldInertia, intermediate, transpose);
    vectorMatrixTransformPoint(body->motion.angularMomentum, worldInertia, angularVelocity);
    body->motion.angularMomentum[3] = 0.0f;
}

void rigidBodyApplyLocalForce(RigidBody* body, const float force[4], const float lever[4])
{
    float worldForce[4], worldLever[4];
    vectorMatrixTransformPoint(worldForce, body->motion.orientation, force);
    vectorMatrixTransformPoint(worldLever, body->motion.orientation, lever);
    worldForce[3] = worldLever[3] = 0.0f;
    rigidBodyApplyForce(body, worldForce, worldLever);
}

void rigidBodyPredictImpulseVelocity(const RigidBody* body, float linear[4],
                                     float angular[4], const float impulse[4],
                                     const float lever[4])
{
    float moment[4], momentum[4] = {}, inverse[16];
    rigidBodyCrossThree(moment, lever, impulse);
    for (int i = 0; i < 3; ++i) {
        const float total = bScalarAdd(body->motion.linearMomentum[i], impulse[i]);
        linear[i] = bScalarMultiply(total, body->inverseMass);
    }
    linear[3] = 0.0f;
    for (int i = 0; i < 3; ++i) momentum[i] = bScalarAdd(body->motion.angularMomentum[i], moment[i]);
    rigidBodyWorldInverseInertia(body, inverse);
    vectorMatrixTransformPoint(angular, inverse, momentum);
    angular[3] = 0.0f;
}

float rigidBodyImpulseEnergy(const RigidBody* body, const float impulse[4], const float lever[4])
{
    float momentum[4] = {}, angularMomentum[4] = {}, moment[4], linear[4] = {}, angular[4], inverse[16];
    for (int i = 0; i < 3; ++i) momentum[i] = bScalarAdd(body->motion.linearMomentum[i], impulse[i]);
    rigidBodyCrossThree(moment, lever, impulse);
    for (int i = 0; i < 3; ++i) {
        angularMomentum[i] = bScalarAdd(body->motion.angularMomentum[i], moment[i]);
        linear[i] = bScalarMultiply(momentum[i], body->inverseMass);
    }
    rigidBodyWorldInverseInertia(body, inverse);
    vectorMatrixTransformPoint(angular, inverse, angularMomentum);
    angular[3] = 0.0f;
    const float translation = bScalarMultiply(rigidBodyDotThree(momentum, linear), 0.5f);
    const float rotation = bScalarMultiply(rigidBodyDotThree(angularMomentum, angular), 0.5f);
    return bScalarAdd(translation, rotation);
}

RigidBodyState* rigidBodyComputeDerivatives(RigidBody* body, RigidBodyState* derivatives)
{
    const RigidBodyTuning* tuning = body->tuning;
    if (body->forwardSpeedLimit != 0.0f) {
        const float scaled = bScalarMultiply(body->forwardSpeedLimit, body->speedLimitScale);
        float limit = bScalarAdd(scaled, 1.899999976158142f);
        if (body->pursuitSpeedEnabled) limit = bScalarMultiply(limit, tuning->pursuitSpeedScale);
        float slope = body->motion.orientation[2];
        if (0.0f < slope) {
            if (tuning->uphillLimit < slope) slope = tuning->uphillLimit;
            const float range = bScalarAdd(1.0f, -tuning->uphillSpeedScale);
            const float offset = bScalarMultiply(range, slope);
            const float fraction = bScalarDivide(offset, tuning->uphillLimit);
            const float scale = bScalarAdd(1.0f, -fraction);
            limit = bScalarMultiply(limit, scale);
        } else {
            slope = -slope;
            if (tuning->downhillLimit < slope) slope = tuning->downhillLimit;
            const float range = bScalarAdd(tuning->downhillSpeedScale, -1.0f);
            const float offset = bScalarMultiply(range, slope);
            const float fraction = bScalarDivide(offset, tuning->downhillLimit);
            const float scale = bScalarAdd(fraction, 1.0f);
            limit = bScalarMultiply(limit, scale);
        }
        const float x = bScalarMultiply(body->linearVelocity[0], body->linearVelocity[0]);
        const float y = bScalarMultiply(body->linearVelocity[1], body->linearVelocity[1]);
        const float speed = bScalarMultiply(bScalarSquareRoot(bScalarAdd(x, y)), 2.236024856567383f);
        if (limit < speed && 0.0f < rigidBodyDotThree(body->motion.orientation, body->force)) {
            float transpose[16], local[4], world[4];
            vectorMatrixTranspose(transpose, body->motion.orientation);
            vectorMatrixTransformPoint(local, transpose, body->force);
            const float downhill = -slope;
            float braking = bScalarMultiply(downhill, tuning->slopeBrakeScale);
            if (tuning->minimumSlopeBrake < braking) braking = tuning->minimumSlopeBrake;
            local[0] = braking;
            vectorMatrixTransformPoint(world, body->motion.orientation, local);
            rigidBodyCopyDirection(body->force, world);
        }
    }
    rigidBodyCopyDirection(derivatives->position, body->linearVelocity);
    rigidBodyOrientationDerivatives(derivatives->orientation, body->angularVelocity,
                                    body->motion.orientation);
    rigidBodyCopyDirection(derivatives->linearMomentum, body->force);
    rigidBodyCopyDirection(derivatives->angularMomentum, body->torque);
    const float angularSpeed = rigidBodyLengthThree(body->angularVelocity);
    const float angularDecay = -sampledCurveEvaluate(&tuning->angularDamping, angularSpeed);
    for (int i = 0; i < 3; ++i) {
        const float correction = bScalarMultiply(body->motion.angularMomentum[i], angularDecay);
        derivatives->angularMomentum[i] = bScalarAdd(derivatives->angularMomentum[i], correction);
    }
    const float linearSpeed = rigidBodyLengthThree(body->linearVelocity);
    float ratio = bScalarDivide(linearSpeed, tuning->dampingLinearScale);
    const float angularRatio = bScalarDivide(angularSpeed, tuning->dampingAngularScale);
    if (ratio < angularRatio) ratio = angularRatio;
    const float target = sampledCurveEvaluate(&tuning->dampingTarget, ratio);
    derivatives->damping = body->motion.damping < target ? tuning->dampingRise : -tuning->dampingFall;
    if (body->dampingEnabled) {
        const float angularScale = bScalarMultiply(-tuning->angularDampingScale, body->motion.damping);
        for (int i = 0; i < 3; ++i) {
            const float correction = bScalarMultiply(body->motion.angularMomentum[i], angularScale);
            derivatives->angularMomentum[i] = bScalarAdd(derivatives->angularMomentum[i], correction);
        }
        const float linearScale = bScalarMultiply(-tuning->linearDampingScale, body->motion.damping);
        for (int i = 0; i < 3; ++i) {
            const float correction = bScalarMultiply(body->motion.linearMomentum[i], linearScale);
            derivatives->linearMomentum[i] = bScalarAdd(derivatives->linearMomentum[i], correction);
        }
    }
    if (body->yawTorqueLimit != 0.0f) {
        if (body->yawTorqueLimit < body->torque[2])
            derivatives->angularMomentum[2] = body->yawTorqueLimit;
        else {
            const float lower = -body->yawTorqueLimit;
            if (body->torque[2] < lower) derivatives->angularMomentum[2] = lower;
        }
    }
    return derivatives;
}

// Native angular cache refresh; no force collection or momentum update.
void rigidBodyRefreshAngularVelocity(RigidBody* body)
{
    float inverse[16];
    rigidBodyWorldInverseInertia(body, inverse);
    vectorMatrixTransformPoint(body->angularVelocity, inverse, body->motion.angularMomentum);
    body->angularVelocity[3] = 0.0f;
}

// PAL19D870..19D884 uses momentum as Fs and inverse mass as Ft.
// The source product's discarded carry makes this order significant.

void rigidBodyPrepareStep(RigidBody* body)
{
    for (int i = 0; i < 3; ++i)
        body->linearVelocity[i] = bScalarMultiply(body->motion.linearMomentum[i], body->inverseMass);
    body->linearVelocity[3] = 0.0f;
    rigidBodyRefreshAngularVelocity(body);
    if (body->motion.damping < 0.0f) body->motion.damping = 0.0f;
    for (int i = 0; i < 12; ++i) body->worldTransform[i] = body->motion.orientation[i];
    rigidBodyCopyPoint(body->worldTransform + 12, body->motion.position);
    body->worldTransform[15] = 1.0f;
    rigidBodyZeroVector(body->force);
    rigidBodyZeroVector(body->torque);
    const float force[4] = {0.0f, 0.0f, bScalarMultiply(-body->tuning->gravity, body->mass), 0.0f};
    const float lever[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    rigidBodyApplyForce(body, force, lever);
}

float rigidBodyHorizontalAccelerationG(const RigidBody* body)
{
    float x = bScalarMultiply(rigidBodyDotThree(body->force, body->motion.orientation), body->inverseMass);
    float y = bScalarMultiply(rigidBodyDotThree(body->force, body->motion.orientation + 4), body->inverseMass);
    x = bScalarMultiply(x, 0.10204081237316132f);
    y = bScalarMultiply(y, 0.10204081237316132f);
    const float squaredX = bScalarMultiply(x, x);
    const float squaredY = bScalarMultiply(y, y);
    return bScalarSquareRoot(bScalarAdd(squaredX, squaredY));
}

RigidBodyState* rigidBodyState(RigidBody* body) { return &body->motion; }

int32_t rigidBodyStateSize(const RigidBody* body) { (void)body; return 29; }

void rigidBodySetLinearMomentum(RigidBody* body, const float momentum[4])
{ rigidBodyCopyDirection(body->motion.linearMomentum, momentum); }

void rigidBodySetSpeedLimitScale(RigidBody* body, float scale)
{ body->speedLimitScale = scale; }

void rigidBodySetForwardSpeedLimit(RigidBody* body, float limit)
{ body->forwardSpeedLimit = limit; }

void rigidBodySetYawTorqueLimit(RigidBody* body, float limit)
{ body->yawTorqueLimit = limit; }

const float* rigidBodyInverseInertia(const RigidBody* body) { return body->inverseInertia; }

float rigidBodyMass(const RigidBody* body) { return body->mass; }

const float* rigidBodyWorldTransform(const RigidBody* body) { return body->worldTransform; }

float* rigidBodyOrientation(RigidBody* body) { return body->motion.orientation; }

float* rigidBodyPosition(RigidBody* body) { return body->motion.position; }

#include "indep/inc/Replay.hpp"

// PAL19CA70..19CC40. Source orientation columns are three XYZ vectors, not
// a raw native matrix. The compact momentum flag uses one unsigned byte.
void rigidBodySnapshot(RigidBody* body,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"RigidBody");
    replaySnapshotMoveData(snapshot,body->motion.position,12,-1,0);
    replaySnapshotMoveData(snapshot,body->motion.orientation,12,-1,0);
    replaySnapshotMoveData(snapshot,body->motion.orientation+4,12,-1,0);
    replaySnapshotMoveData(snapshot,body->motion.orientation+8,12,-1,0);
    if(snapshot->recording || snapshot->restoring) {
        body->dampingEnabled=1;
        body->motion.orientation[3]=body->motion.orientation[7]=body->motion.orientation[11]=0.0f;
        body->motion.orientation[12]=body->motion.orientation[13]=body->motion.orientation[14]=0.0f;
        body->motion.orientation[15]=1.0f;
    }
    replaySnapshotMoveFloat(snapshot,&body->motion.damping);
    int32_t stopped=body->motion.linearMomentum[0]==0.0f && body->motion.linearMomentum[1]==0.0f &&
        body->motion.linearMomentum[2]==0.0f && body->motion.angularMomentum[0]==0.0f &&
        body->motion.angularMomentum[1]==0.0f && body->motion.angularMomentum[2]==0.0f;
    replaySnapshotMoveData(snapshot,&stopped,1,4,0);
    if(!stopped) {
        replaySnapshotMoveData(snapshot,body->motion.linearMomentum,12,-1,0);
        replaySnapshotMoveData(snapshot,body->motion.angularMomentum,12,-1,0);
    } else if(snapshot->recording || snapshot->restoring) {
        for(int axis=0;axis<3;++axis) {
            body->motion.linearMomentum[axis]=0.0f;body->motion.angularMomentum[axis]=0.0f;
        }
    }
    if(snapshot->recording || snapshot->restoring) {
        // The source snapshot transfers XYZ only; padding is not wire state.
        body->motion.position[3] = 1.0f;
        body->motion.linearMomentum[3] = body->motion.angularMomentum[3] = 0.0f;
        rigidBodyPrepareStep(body);
    }
    replaySnapshotEndSection(snapshot);
}
