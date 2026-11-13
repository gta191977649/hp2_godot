#include "indep/inc/Replay.hpp"
#include "indep/inc/TrafficCar.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Car.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>
#include <string.h>

static const float accelerationSamples[4] = {3.5762181282f,2.68216371536f,2.01162266731f,1.34108185768f};
static const float brakingSamples[4] = {6.70540904999f,13.4108181f,20.1162261963f,26.8216362f};

void trafficCarInitialize(TrafficCar* car, CarMovement* movement)
{
    const VehicleState* state = movement->bindings.state;
    car->movement = movement;
    car->position[0] = state->position[0]; car->position[1] = state->position[1];
    car->headingRadians = (float)state->heading * 0.0000958738019107841f;
    car->speed = state->speed;
    car->savedAcceleration = car->savedSteering = 0.0f;
    sampledCurveInitialize(&car->acceleration, accelerationSamples, 4, 0.0f, 24.5865001678f);
    sampledCurveInitialize(&car->braking, brakingSamples, 4, 0.0f, 24.5865001678f);
}

float trafficCarMove(TrafficCar* car, float elapsed)
{
    // A native zero-duration move preserves the owner and published state.
    // PAL167E68 divides heading delta by elapsed; its exceptional arithmetic
    // must not publish a fabricated angular velocity into PC physics.
    if (elapsed == 0.0f) return 0.0f;
    CarMovement* movement = car->movement;
    VehicleState* state = movement->bindings.state;
    const CarControls* controls = movement->bindings.controls;
    const float rearMount = movement->info.wheels[3].mountPosition[0];
    const float wheelbase = bScalarAdd(movement->info.wheels[0].mountPosition[0], -rearMount);
    const float distance = bScalarMultiply(car->speed, elapsed);
    const float cosine = rotationSineRadians(bScalarAdd(car->headingRadians, 1.5707964897155762f));
    const float sine = rotationSineRadians(car->headingRadians);
    float steering = bScalarMultiply((float)controls->steeringAngle, 0.0000958738019107841f);
    if (3.141590118408203f < steering) steering = bScalarAdd(steering, -6.283180236816406f);
    const float straightThreshold = bScalarMultiply(wheelbase, 0.00999999977648f);
    if (steering < straightThreshold && -straightThreshold < steering) {
        car->position[0] = bScalarAdd(car->position[0], bScalarMultiply(cosine, distance));
        car->position[1] = bScalarAdd(car->position[1], bScalarMultiply(sine, distance));
    } else {
        const float radius = finiteRatio(wheelbase, steering);
        const float direction[2] = {cosine, sine};
        const float perpendicular[2] = {-sine, cosine};
        float rear[2], center[2];
        vectorMultiplyAdd2(rear, car->position, direction, rearMount);
        vectorMultiplyAdd2(center, rear, perpendicular, radius);
        const float angle = finiteRatio(distance, radius);
        const float turnCosine = rotationSineRadians(bScalarAdd(angle, 1.5707964897155762f));
        const float turnSine = rotationSineRadians(angle);
        const float x = bScalarAdd(car->position[0], -center[0]);
        const float y = bScalarAdd(car->position[1], -center[1]);
        const float rotatedX = bScalarAdd(bScalarMultiply(turnCosine, x), -bScalarMultiply(turnSine, y));
        const float rotatedY = bScalarAdd(bScalarMultiply(turnSine, x), bScalarMultiply(turnCosine, y));
        car->position[0] = bScalarAdd(center[0], rotatedX);
        car->position[1] = bScalarAdd(center[1], rotatedY);
        car->headingRadians = bScalarAdd(car->headingRadians, angle);
    }
    // PAL167D04..167D38 keeps both elapsed/control products, the table
    // products, subtraction and final speed addition as separate operations.
    const float acceleration = bScalarMultiply(bScalarMultiply(elapsed, controls->throttle),
        sampledCurveEvaluate(&car->acceleration, car->speed));
    const float braking = bScalarMultiply(bScalarMultiply(elapsed, controls->brake),
        sampledCurveEvaluate(&car->braking, car->speed));
    car->speed = bScalarAdd(car->speed, bScalarAdd(acceleration, -braking));
    if (car->speed < 0.0f) car->speed = 0.0f;
    else if (24.5865001678f < car->speed) car->speed = 24.5865001678f;

    float previousPosition[4];
    memcpy(previousPosition,state->position,sizeof(previousPosition));
    state->position[0] = car->position[0]; state->position[1] = car->position[1];
    carMovementUpdateTerrainPose(movement);
    float velocity[4];
    for (int axis = 0; axis < 4; ++axis) velocity[axis] = state->position[axis]-previousPosition[axis];
    vectorSetLength3(velocity,velocity,car->speed);
    memcpy(state->velocity,velocity,sizeof(velocity));
    state->speed = sqrtf((velocity[0]*velocity[0] + velocity[1]*velocity[1]) + velocity[2]*velocity[2]);
    state->forwardSpeed = car->speed;
    state->velocityFrameValid = 0;
    const uint16_t heading = (uint16_t)convertFloatToInt32(car->headingRadians*10430.3779296875f);
    const int16_t headingDelta = (int16_t)(uint16_t)(heading-state->heading);
    state->heading = heading;
    state->angularVelocity[0] = state->angularVelocity[1] = 0.0f;
    state->angularVelocity[2] = finiteRatio((float)headingDelta * 0.00009587379463482648f,elapsed);
    state->steeringAngle = controls->steeringAngle;
    for (int wheel = 0; wheel < 4; ++wheel) {
        state->wheelAngularVelocity[wheel] = car->speed / movement->info.wheels[wheel].reactionScale;
        state->wheelAngle[wheel] += state->wheelAngularVelocity[wheel]*elapsed;
        state->suspensionTravel[wheel] = 0.0f;
        vehicleStateSetWheelContact(state,wheel,1);
        vehicleStateSetWheelMaterial(state,wheel,1);
        state->wheelSlip[wheel][0] = state->wheelSlip[wheel][1] = 0.0f;
        state->wheelSlipRatio[wheel] = state->wheelNormalLoad[wheel] = 0.0f;
        memset(state->suspensionImpulse,0,sizeof(state->suspensionImpulse));
    }
    vehicleStateUpdateWheelPoses(state);
    vehicleStateUpdateCenterPosition(state);
    carMovementUpdatePowertrainDisplay(movement,&carDriveDisplayDefaultTuning);
    return elapsed;
}

// Final immutable61-sample stopping-distance table at2FAD90, initialized by
// PAL167FE0. Two acceleration/brake curve views borrow their existing tables.
namespace {
const float stoppingDistanceSamples[61]={
    0.0f,0.0179999992f,0.063000001f,0.133000001f,0.224999994f,0.337000012f,
    0.467999995f,0.616999984f,0.782999992f,0.963f,1.15699995f,1.36500001f,
    1.58500004f,1.81599998f,2.05900002f,2.31200004f,2.57399988f,2.84500003f,
    3.125f,3.41400003f,3.71000004f,4.01300001f,4.32399988f,4.64099979f,
    4.96500015f,5.29500008f,5.63000011f,5.97200012f,6.31799984f,6.67000008f,
    7.02699995f,7.38800001f,7.75400019f,8.125f,8.4989996f,8.87800026f,
    9.26000023f,9.64700031f,10.0369997f,10.4300003f,10.8269997f,11.2279997f,
    11.6309996f,12.0369997f,12.4469995f,12.8590002f,13.2740002f,13.6920004f,
    14.1129999f,14.5360003f,14.9619999f,15.3900003f,15.8199997f,16.2530003f,
    16.6870003f,17.1240005f,17.5639992f,18.0049992f,18.448f,18.8929996f,
    19.3400002f,
};
}

float trafficCarCalculateStopDistance(float sourceUnused,float speed)
{
    (void)sourceUnused; // PAL167FB8 deliberately consumes only f13.
    SampledCurve curve;
    sampledCurveInitialize(&curve,stoppingDistanceSamples,61,0.0f,24.58650016784668f);
    return sampledCurveEvaluate(&curve,speed);
}

// Full PAL167A60; CarMover::DoSnapshot1309F0 is an actual JR/NOP and is
// intentionally omitted. No empty runtime method is substituted for it.
void trafficCarSnapshot(TrafficCar* car,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"TrafficCar");
    replaySnapshotMoveData(snapshot,car->position,8,-1,0);
    replaySnapshotMoveFloat(snapshot,&car->headingRadians);
    replaySnapshotMoveFloat(snapshot,&car->speed);
    replaySnapshotMoveFloat(snapshot,&car->savedAcceleration);
    replaySnapshotMoveFloat(snapshot,&car->savedSteering);
    replaySnapshotEndSection(snapshot);
}
