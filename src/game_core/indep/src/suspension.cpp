#include "indep/inc/Suspension.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Wheel.hpp"
static inline float suspensionDotThree(const float first[3], const float second[3])
{
    const float xy = first[0] * second[0] + first[1] * second[1];
    return xy + first[2] * second[2];
}

#include <string.h>
#include "indep/inc/Replay.hpp"

void suspensionInitializeMount(Suspension* suspension, float preload)
{
    suspension->preload = preload;
    for (int i = 0; i < 4; ++i)
        suspension->mountOffset[i] = bScalarAdd(suspension->wheel->torqueInfo->mountPosition[i], -suspension->body->centerOfMass[i]);
}

void suspensionResetMotion(Suspension* suspension)
{
    suspension->motion.travel = 0;
    suspension->motion.travelSpeed = 0;
}

float* suspensionPointVelocity(float output[4], const float linearVelocity[4],
                                const float angularVelocity[4], const float lever[4])
{
    // PAL OPMULA/OPMSUB reverse the operands of the second product.
    const float x = bScalarAdd(bScalarMultiply(angularVelocity[1], lever[2]), -bScalarMultiply(lever[1], angularVelocity[2]));
    const float y = bScalarAdd(bScalarMultiply(angularVelocity[2], lever[0]), -bScalarMultiply(lever[2], angularVelocity[0]));
    const float z = bScalarAdd(bScalarMultiply(angularVelocity[0], lever[1]), -bScalarMultiply(lever[0], angularVelocity[1]));
    output[0] = bScalarAdd(linearVelocity[0], x);
    output[1] = bScalarAdd(linearVelocity[1], y);
    output[2] = bScalarAdd(linearVelocity[2], z);
    output[3] = bScalarAdd(linearVelocity[3], angularVelocity[3]);
    return output;
}

float suspensionGetAntiRollRate(const Suspension* suspension) { return suspension->info->antiRollRate; }
float suspensionGetDamping(const Suspension* suspension)
{ return suspension->motion.travelSpeed <= 0 ? suspension->info->negativeDamping : suspension->info->positiveDamping; }
float suspensionGetSpringRate(const Suspension* suspension) { return suspension->info->springRate; }
float suspensionGetNonlinearRate(const Suspension* suspension) { return suspension->info->nonlinearRate; }
void suspensionSetTuningValue(Suspension* suspension, float value) { suspension->tuningValue = value; }
int32_t suspensionGetIndex(const Suspension* suspension) { return suspension->steeringSide; }
float* suspensionMountOffset(Suspension* suspension) { return suspension->mountOffset; }

void suspensionRestoreState(Suspension* suspension, const SuspensionState* state)
{
    suspension->motion = *state;
}

void suspensionSaveState(const Suspension* suspension, SuspensionState* state)
{
    *state = suspension->motion;
}

SuspensionState* suspensionComputeDerivatives(const Suspension* suspension,
                                               SuspensionState* derivatives)
{
    // Suspension motion is assigned by the contact pass. Its integrable
    // interface contributes no additional change to those two state values.
    (void)suspension;
    derivatives->travel = 0.0f;
    derivatives->travelSpeed = 0.0f;
    return derivatives;
}

float suspensionGetTravelLimitDelta(const Suspension* suspension)
{
    if (suspension->motion.travel < suspension->info->extensionLimit)
        return 0.0f;
    return suspension->info->extensionLimit - suspension->motion.travel;
}

int32_t suspensionAtBumpStop(const Suspension* suspension)
{
    return suspension->motion.travel == suspension->info->bumpStop;
}

Suspension* suspensionInitialize(Suspension* suspension, int32_t index,
                                  RigidBody* body, Wheel* wheel,
                                  const Steering* steering, const SuspensionInfo* info)
{
    suspension->steeringSide = index;
    suspension->body = body;
    suspension->wheel = wheel;
    suspension->steering = steering;
    suspension->info = info;
    suspension->pairedSuspension = NULL;
    return suspension;
}

void suspensionSerializePersistentState(Suspension* suspension, ReplaySnapshot* snapshot)
{
    // Complete PAL1A5ED0..1A5F88 replaces the typed transport adaptation.
    replaySnapshotBeginSection(snapshot,nullptr,"Suspension");
    replaySnapshotMoveFloat(snapshot,&suspension->motion.travel);
    replaySnapshotMoveFloat(snapshot,&suspension->motion.travelSpeed);
    if (snapshot->recording || snapshot->restoring) {
        suspension->contactForce = 0;
        memset(suspension->position, 0, sizeof(suspension->position));
        memset(suspension->transform, 0, sizeof(suspension->transform));
        suspension->contactPresent = 0;
        suspension->contactResult = 0;
    }
    replaySnapshotEndSection(snapshot);
}

void suspensionPrepareContact(Suspension* suspension, const WheelContactContext* context)
{
    RigidBody* body = suspension->body;
    Wheel* wheel = suspension->wheel;
    const SuspensionInfo* info = suspension->info;
    const WheelContactTuning* tuning = context->tuning;
    float worldMount[4];
    vectorMatrixTransformPoint(worldMount, body->motion.orientation, suspension->mountOffset);
    for (int i = 0; i < 4; ++i) worldMount[i] = bScalarAdd(worldMount[i], body->motion.position[i]);
    const float groundDistance = wheelGroundDisplacement(wheel, context, worldMount, body->motion.orientation);
    const float requestedTravel = bScalarAdd(groundDistance, wheel->torqueInfo->reactionScale);
    const float oldTravel = suspension->motion.travel;
    float excess = 0;
    if (info->bumpStop < requestedTravel) {
        excess = bScalarAdd(requestedTravel, -info->bumpStop);
        suspension->motion.travel = info->bumpStop;
        suspension->contactPresent = 1;
        if (tuning->bumpImpactLimit < excess) excess = 0;
    } else if (requestedTravel < info->extensionStop) {
        suspension->motion.travel = info->extensionStop;
        suspension->contactPresent = 0;
    } else {
        suspension->motion.travel = requestedTravel;
        suspension->contactPresent = 1;
    }
    if (0.00009999999747378752f < context->elapsed) {
        const float change = bScalarAdd(suspension->motion.travel, -oldTravel);
        suspension->motion.travelSpeed = bScalarFiniteRatio(change, context->elapsed);
    }
    for (int i = 0; i < 4; ++i) suspension->position[i] = suspension->mountOffset[i];
    suspension->position[2] = bScalarAdd(suspension->position[2], suspension->motion.travel);
    float local[16];
    rotationMatrixIdentity(local);
    if (wheel->steeringAngle != 0) {
        float tilt[16];
        rotationMatrixIdentity(tilt);
        float angle = bScalarMultiply(tuning->wheelTiltScale, wheel->steeringAngle);
        angle = bScalarMultiply(angle, rotationSineRadians(wheel->motion.angle));
        const float cosine = rotationSineRadians(bScalarAdd(angle, 1.5707964897155762f));
        const float sine = rotationSineRadians(angle);
        tilt[0] = cosine; tilt[5] = cosine;
        tilt[1] = sine; tilt[4] = -sine;
        vectorMatrixMultiply(local, local, tilt);
    }
    for (int i = 0; i < 3; ++i) local[12 + i] = suspension->position[i];
    local[15] = 1;
    for (int i = 0; i < 16; ++i) suspension->transform[i] = local[i];
    if (suspension->steering != NULL) {
        const float* steering = suspension->steeringSide == 0 ?
            suspension->steering->leftTransform : suspension->steering->rightTransform;
        vectorMatrixMultiply(local, local, steering);
    }
    wheelSetCarTransform(wheel, local);
    float lever[4], velocity[4];
    vectorMatrixTransformPoint(lever, body->motion.orientation, suspension->position);
    suspensionPointVelocity(velocity, body->linearVelocity, body->angularVelocity, lever);
    wheelSetVelocity(wheel, velocity);
    vectorMatrixMultiply(wheel->worldTransform, body->worldTransform, local);
    if (suspension->contactPresent) {
        const float nonlinearTravel = 0 < suspension->motion.travel ? suspension->motion.travel : 0;
        const float springMultiplier = bScalarAdd(bScalarMultiply(nonlinearTravel, info->nonlinearRate), 1.0f);
        const float springRate = bScalarMultiply(info->springRate, springMultiplier);
        const float springForce = bScalarMultiply(suspension->motion.travel, springRate);
        const float damping = suspension->motion.travelSpeed <= 0 ? info->negativeDamping : info->positiveDamping;
        float force = bScalarAdd(suspension->preload, springForce);
        force = bScalarAdd(force, bScalarMultiply(suspension->motion.travelSpeed, damping));
        const float travelDifference = bScalarAdd(suspension->motion.travel, -suspension->pairedSuspension->motion.travel);
        const float antiRoll = bScalarMultiply(info->antiRollRate, travelDifference);
        force = bScalarAdd(force, antiRoll);
        force = bScalarAdd(force, bScalarMultiply(excess, tuning->bumpImpactScale));
        suspension->contactForce = force < 0 ? 0 : force;
    } else suspension->contactForce = 0;
}

void suspensionSetContactLoad(Suspension* suspension, float extraLoad)
{
    float load = suspension->contactForce;
    if (suspension->contactPresent) load = bScalarAdd(load, extraLoad);
    suspension->contactResult = wheelSetContactLoad(suspension->wheel, suspension->contactPresent, load);
}

void suspensionApplyContactForce(Suspension* suspension, const WheelContactContext* context)
{
    if (!suspension->contactPresent) return;
    float lever[4];
    for (int i = 0; i < 4; ++i)
        lever[i] = bScalarAdd(suspension->wheel->contactPoint[i], -suspension->body->motion.position[i]);
    if (!context->tuning->suppressBodyForces)
        rigidBodyApplyForce(suspension->body, suspension->wheel->worldContactForce, lever);
}

float suspensionResolveContactImpulse(Suspension* suspension, const WheelContactContext* context)
{
    float lever[4], velocity[4];
    vectorMatrixTransformPoint(lever, suspension->body->motion.orientation, suspension->position);
    suspensionPointVelocity(velocity, suspension->body->linearVelocity,
                             suspension->body->angularVelocity, lever);
    float impulse = -suspensionDotThree(velocity, suspension->wheel->contactNormal);
    if (impulse < 0) return 0;
    impulse = impulse * context->tuning->contactImpulseScale;
    impulse = impulse * suspension->body->mass;
    float vector[4];
    for (int i = 0; i < 4; ++i) vector[i] = suspension->wheel->contactNormal[i] * impulse;
    if (!context->tuning->suppressBodyForces)
        rigidBodyApplyImpulse(suspension->body, vector, lever);
    return impulse;
}

SuspensionState* suspensionState(Suspension* suspension)
{
    return &suspension->motion;
}

int32_t suspensionStateSize(const Suspension* suspension)
{
    (void)suspension;
    return 2;
}

float suspensionGetTravel(const Suspension* suspension)
{
    return suspension->motion.travel;
}

float suspensionGetTravelSpeed(const Suspension* suspension)
{
    return suspension->motion.travelSpeed;
}

void suspensionSetPairedSuspension(Suspension* suspension, Suspension* paired)
{
    suspension->pairedSuspension = paired;
}

float* suspensionTransform(Suspension* suspension) { return suspension->transform; }

// Initialize the steering geometry and retained suspension state.
void steeringInitialize(Steering* steering, const SteeringGeometry* geometry)
{
    steering->geometry = geometry;
}

void steeringSetAngle(Steering* steering, uint16_t angle)
{
    float radians = bScalarMultiply((float)angle, 0.0000958738019107841f);
    const float wheelbase = bScalarAdd(steering->geometry->frontAxleX, -steering->geometry->rearAxleX);
    const float track = bScalarAdd(steering->geometry->leftWheelY, -steering->geometry->rightWheelY);
    if (3.141590118408203f < radians) radians = bScalarAdd(radians, -6.283180236816406f);
    const int positive = !(radians < 0.0f);
    if (!positive) radians = -radians;
    const float trackChange = bScalarMultiply(track, radians);
    const float axleChange = bScalarMultiply(wheelbase, radians);
    const float adjustedWheelbase = bScalarAdd(wheelbase, trackChange);
    const float adjustedAngle = bScalarDivide(axleChange, adjustedWheelbase);
    const float left = positive ? radians : -adjustedAngle;
    const float right = positive ? adjustedAngle : -radians;
    const float quarterTurn = 1.5707964897155762f;
    rotationMatrixIdentity(steering->leftTransform);
    float cosine = rotationSineRadians(bScalarAdd(left, quarterTurn));
    float sine = rotationSineRadians(left);
    steering->leftTransform[5] = cosine;
    steering->leftTransform[0] = cosine;
    steering->leftTransform[1] = sine;
    steering->leftTransform[4] = -sine;
    rotationMatrixIdentity(steering->rightTransform);
    cosine = rotationSineRadians(bScalarAdd(right, quarterTurn));
    sine = rotationSineRadians(right);
    steering->rightTransform[5] = cosine;
    steering->rightTransform[0] = cosine;
    steering->rightTransform[1] = sine;
    steering->rightTransform[4] = -sine;
}

// Full PAL1A5D10..1A5D60 uses the original matrix overload. Its nominal
//128-byte argument does not change that overload's one-matrix encoding.
void steeringDoSnapshot(Steering* steering,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"TwoWheelAckermanSteering");
    replaySnapshotMoveMatrix(snapshot,steering->leftTransform,128);
    replaySnapshotEndSection(snapshot);
}
