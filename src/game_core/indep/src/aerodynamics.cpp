#include "indep/inc/Aerodynamics.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>
#include <stddef.h>
#include "indep/inc/Replay.hpp"

void aerodynamicsResetTransientState(Aerodynamics* aerodynamic, ReplaySnapshot* snapshot)
{
    // Complete PAL1ADD48 has no stream transfer; it tests the actual two
    // replay mode bytes and resets these same three transient fields.
    if (snapshot->recording || snapshot->restoring) {
        aerodynamic->downforceMode = 0.0f;
        aerodynamic->speedScale = 0.0f;
        aerodynamic->dragReduction = 0.0f;
    }
}

float aerodynamicsEstimateDownforce(const Aerodynamics* aerodynamic, float speed)
{
    const float nonnegative = maximumFinite(0.0f, speed);
    const float combined = bScalarAdd(aerodynamic->info->frontDownforce, aerodynamic->info->rearDownforce);
    return bScalarMultiply(nonnegative, combined);
}



const AerodynamicsTuning aerodynamicsDefaultTuning = {2.0f, -0.10000000149011612f};

static float dotThree(const float left[4], const float right[4])
{
    const float x = bScalarMultiply(left[0], right[0]);
    const float y = bScalarMultiply(left[1], right[1]);
    const float z = bScalarMultiply(left[2], right[2]);
    return bScalarAdd(bScalarAdd(x, y), z);
}

void aerodynamicsInitialize(Aerodynamics* aerodynamic, const AerodynamicsInfo* info,
                            RigidBody* body, const AerodynamicsTuning* tuning)
{
    aerodynamic->info = info;
    aerodynamic->body = body;
    aerodynamic->tuning = tuning;
    aerodynamic->speedScale = 1.0f;
    aerodynamic->dragScale = 1.0f;
    aerodynamic->dragReduction = 0.0f;
    aerodynamic->downforceMode = 0.0f;
}

void aerodynamicsApplyForces(Aerodynamics* aerodynamic, float* totalDownforce)
{
    RigidBody* body = aerodynamic->body;
    const float speed = bScalarSquareRoot(dotThree(body->linearVelocity, body->linearVelocity));
    const float scaledSpeed = bScalarFiniteRatio(speed, aerodynamic->speedScale);
    const float initial = bScalarMultiply(scaledSpeed, aerodynamic->dragScale);
    float drag = bScalarMultiply(initial, aerodynamic->info->drag);
    const float boost = bScalarAdd(aerodynamic->tuning->boostedDrag, -1.0f);
    const float reduction = bScalarAdd(1.0f, -aerodynamic->dragReduction);
    const float boosted = bScalarMultiply(drag, boost);
    const float extra = bScalarMultiply(boosted, reduction);
    drag = bScalarAdd(drag, extra);
    const float dragScale = -drag;
    float dragForce[4], dragLever[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 4; ++i) dragForce[i] = bScalarMultiply(body->linearVelocity[i], dragScale);
    if (0.5f <= aerodynamic->downforceMode)
        dragLever[2] = bScalarMultiply(aerodynamic->tuning->dragLeverZ, reduction);
    rigidBodyApplyForce(body, dragForce, dragLever);

    float facing = body->motion.orientation[10];
    if (facing < 0.0f) facing = 0.0f;
    if (0.5f <= aerodynamic->downforceMode) facing = 1.0f;
    float direction[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    if (0.00009999999747378752f < scaledSpeed) {
        const float inverseSpeed = bScalarFiniteRatio(1.0f, scaledSpeed);
        for (int i = 0; i < 4; ++i)
            direction[i] = bScalarMultiply(body->linearVelocity[i], inverseSpeed);
    }
    float alignment = dotThree(direction, body->motion.orientation);
    if (alignment < 0.0f) alignment = 0.0f;
    const float squaredAlignment = bScalarMultiply(alignment, alignment);
    const float fourthAlignment = bScalarMultiply(squaredAlignment, squaredAlignment);
    float front, rear;
    if (aerodynamic->downforceMode == 0.0f) {
        const float frontSpeed = bScalarMultiply(scaledSpeed, 0.4000000059604645f);
        const float rearSpeed = bScalarMultiply(scaledSpeed, 0.4099999964237213f);
        const float influence = bScalarMultiply(fourthAlignment, facing);
        const float frontInfluence = bScalarMultiply(frontSpeed, influence);
        const float rearInfluence = bScalarMultiply(rearSpeed, influence);
        const float combined = bScalarAdd(aerodynamic->info->frontDownforce, aerodynamic->info->rearDownforce);
        front = bScalarMultiply(frontInfluence, combined);
        rear = bScalarMultiply(rearInfluence, combined);
    } else {
        const float influence = bScalarMultiply(fourthAlignment, facing);
        const float rearSpeed = bScalarMultiply(scaledSpeed, aerodynamic->info->rearDownforce);
        const float frontSpeed = bScalarMultiply(scaledSpeed, aerodynamic->info->frontDownforce);
        rear = bScalarMultiply(rearSpeed, influence);
        front = bScalarMultiply(frontSpeed, influence);
    }
    const float frontForce[4] = {0.0f, 0.0f, -front, 0.0f};
    const float rearForce[4] = {0.0f, 0.0f, -rear, 0.0f};
    const float frontLever[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    const float rearLever[4] = {-1.0f, 0.0f, 0.0f, 0.0f};
    rigidBodyApplyLocalForce(body, frontForce, frontLever);
    rigidBodyApplyLocalForce(body, rearForce, rearLever);
    if (totalDownforce != NULL) *totalDownforce = bScalarAdd(front, rear);
}
