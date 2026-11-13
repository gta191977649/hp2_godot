#include "indep/inc/DeliveryMover.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Car.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>
#include <string.h>

void deliveryMoverInitialize(DeliveryMover* mover, CarMovement* movement)
{
    mover->movement = movement;
}

void deliveryMoverSetTarget(DeliveryMover* mover, const float position[2], uint16_t heading,
                            float speed, float duration)
{
    CarMovement* movement = mover->movement;
    VehicleState* state = movement->bindings.state;
    memcpy(mover->startPosition, state->position, sizeof(mover->startPosition));
    vectorQuaternionFromMatrixCopy(mover->startOrientation, state->orientation);
    memcpy(mover->startVelocity, state->velocity, sizeof(mover->startVelocity));

    float target[4] = {position[0], position[1], state->position[2], 0};
    float orientation[16];
    const float stationary[2] = {0,0};
    TerrainSurface terrain;
    terrainSurfaceInitialize(&terrain, movement->bindings.environment->contact.terrain->topology);
    terrainSurfaceUpdate(&terrain, target, 1.5f, stationary);
    carMovementAlignTerrainPose(&movement->info, &terrain, orientation, target, heading);
    for (int axis = 0; axis < 4; ++axis)
        mover->positionDelta[axis] = target[axis] - mover->startPosition[axis];
    vectorQuaternionFromMatrixCopy(mover->targetOrientation, orientation);
    for (int axis = 0; axis < 4; ++axis)
        mover->velocityDelta[axis] = orientation[axis] * speed - mover->startVelocity[axis];
    mover->duration = duration;
    mover->elapsed = 0.0f;
    terrainSurfaceDispose(&terrain);
}

float deliveryMoverMove(DeliveryMover* mover, float elapsed)
{
    const float remaining = mover->duration - mover->elapsed;
    if (remaining <= elapsed) elapsed = remaining;
    mover->elapsed += elapsed;
    const float fraction = mover->duration == 0.0f ? 1.0f : mover->elapsed / mover->duration;
    float position[4], velocity[4], quaternion[4], orientation[16];
    for (int axis = 0; axis < 4; ++axis) {
        position[axis] = mover->startPosition[axis] + mover->positionDelta[axis] * fraction;
        velocity[axis] = mover->startVelocity[axis] + mover->velocityDelta[axis] * fraction;
    }
    vectorQuaternionInterpolate(quaternion, mover->startOrientation, mover->targetOrientation, fraction);
    vectorMatrixFromQuaternion(orientation, quaternion);
    vectorMatrixTranspose(orientation, orientation);

    CarMovement* movement = mover->movement;
    VehicleState* state = movement->bindings.state;
    memcpy(state->position, position, sizeof(position));
    carMovementUpdateTerrainPose(movement);
    memcpy(state->velocity, velocity, sizeof(velocity));
    const float speed = sqrtf((velocity[0]*velocity[0] + velocity[1]*velocity[1]) + velocity[2]*velocity[2]);
    state->speed = speed;
    state->velocityFrameValid = 0;
    memcpy(state->orientation, orientation, sizeof(orientation));
    memset(state->angularVelocity, 0, 3*sizeof(float));
    state->heading = vectorFixedAtan2Angle(convertFloatToInt32(orientation[0]*65536.0f),
                                           convertFloatToInt32(orientation[1]*65536.0f));
    state->steeringAngle = 0;
    state->forwardSpeed = speed;
    for (int wheel = 0; wheel < 4; ++wheel) {
        state->wheelAngularVelocity[wheel] = speed / movement->info.wheels[wheel].reactionScale;
        state->wheelAngle[wheel] += state->wheelAngularVelocity[wheel] * elapsed;
        state->suspensionTravel[wheel] = 0.0f;
        vehicleStateSetWheelContact(state, wheel, 1);
        vehicleStateSetWheelMaterial(state, wheel, 1);
        state->wheelSlip[wheel][0] = state->wheelSlip[wheel][1] = 0.0f;
        state->wheelSlipRatio[wheel] = 0.0f;
        state->wheelNormalLoad[wheel] = 0.0f;
        memset(state->suspensionImpulse, 0, sizeof(state->suspensionImpulse));
    }
    vehicleStateUpdateWheelPoses(state);
    carMovementUpdatePowertrainDisplay(movement, &carDriveDisplayDefaultTuning);
    return elapsed;
}
