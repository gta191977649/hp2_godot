#include "indep/inc/Replay.hpp"
#include "indep/inc/CarMover.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/DriveTarget.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/QuickSpline.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/World.hpp"

#include <math.h>
#include <string.h>

static float dot3(const float first[4],const float second[4])
{return bScalarAdd((bScalarAdd(bScalarMultiply(first[0], second[0]), bScalarMultiply(first[1], second[1]))), bScalarMultiply(first[2], second[2]));}

static void normalize(float value[4])
{
    const float inverse = bScalarFiniteRatio(1.0f, bScalarSquareRoot(dot3(value,value)));
    for (int axis = 0; axis < 4; ++axis) value[axis] = bScalarMultiply(value[axis], inverse);
}

// PAL2048D0/2048D4,204ED4/204ED8 and205000/205004 retain the actual VU cross with +Z.
// The directional finite MUL cannot replace 1*x by a plain copy/sign change.
static void beltPerpendicular(float output[4], const float forward[4])
{
    output[0] = bScalarAdd(bScalarMultiply(forward[1], 1.0f), -bScalarMultiply(0.0f, forward[2]));
    output[1] = bScalarAdd(bScalarMultiply(forward[2], 0.0f), -bScalarMultiply(1.0f, forward[0]));
    output[2] = bScalarAdd(bScalarMultiply(forward[0], 0.0f), -bScalarMultiply(0.0f, forward[1]));
    output[3] = forward[3];
}

int32_t beltMoverInitialize(BeltMover* mover,CarMovement* movement)
{
    memset(mover,0,sizeof(*mover));mover->movement = movement;
    if (!driverHistoryInitialize(&mover->lateralHistory,30)) return 0;
    quickSplineInitialize(&mover->path,QUICK_SPLINE_EXTRAPOLATE,0,20);
    tCubic1DInitialize(&mover->lateralOffset);
    for (int axis = 0; axis < 2; ++axis) {
        tCubic1DInitialize(&mover->velocityFilter.axes[axis]);
        tCubic1DInitialize(&mover->lateralFilter.axes[axis]);
    }
    const VehicleState* state = movement->bindings.state;
    const float speed = state->speed;
    const float lateralSpeed = speed*(state->angularVelocity[2]*0.4000000059604645f);
    const uint16_t sideHeading = (uint16_t)(state->heading+16384u);
    const float velocity[2] = {speed*vectorCosineAngle(state->heading),speed*rotationSineAngle(state->heading)};
    const float derivative[2] = {lateralSpeed*vectorCosineAngle(sideHeading),lateralSpeed*rotationSineAngle(sideHeading)};
    tCubic2DSetVal(&mover->velocityFilter,velocity);tCubic2DSetdVal(&mover->velocityFilter,derivative);
    mover->lateralFilter.axes[0].duration = mover->lateralFilter.axes[1].duration = 0.30000001192092896f;
    wheelTrailStoreInitialize(&mover->localTrailStore,mover->localTrails,16);
    const PhysicsCarInfo* info = &movement->info;
    for (int wheel = 0; wheel < 4; ++wheel) {
        PhysicsPoolStatus status;
        mover->wheels[wheel] = physicsObjectsAcquireWheel(movement->bindings.pools,&status);
        if (!mover->wheels[wheel]) {beltMoverDispose(mover);return 0;}
        wheelInitialize(mover->wheels[wheel],info->wheels+wheel,info->tires+wheel/2,info->brakes,&mover->wheelVehicle);
    }
    return 1;
}

void beltMoverDispose(BeltMover* mover)
{
    WheelTrailStore* trails = mover->movement->bindings.environment->wheelTrails;
    if (!trails) trails = &mover->localTrailStore;
    driverHistoryDispose(&mover->lateralHistory);quickSplineDestroy(&mover->path,0);
    for (int wheel = 0; wheel < 4; ++wheel) {
        wheelTrailDetach(trails,mover->trails+wheel);
        if (mover->wheels[wheel]) (void)physicsObjectsReleaseWheel(mover->movement->bindings.pools,mover->wheels[wheel]);
        mover->wheels[wheel] = NULL;
    }
}

float beltMoverRelativePosition(BeltMover* mover,const float position[4],float parameter,
                                 float difference[4],float tangent[4])
{
    quickSplineEvaluate(&mover->path,difference,parameter,0);
    quickSplineEvaluate(&mover->path,tangent,parameter,1);
    for (int axis = 0; axis < 4; ++axis) difference[axis] = bScalarAdd(difference[axis],-position[axis]);
    return dot3(difference,tangent);
}

float beltMoverSlope(BeltMover* mover)
{
    if (mover->path.count == 0) return 0.0f;
    float tangent[4];quickSplineEvaluate(&mover->path,tangent,mover->startParameter,1);
    const float squared = dot3(tangent,tangent);
    const float inverse = bScalarFiniteRatio(1.0f,bScalarSquareRoot(squared));
    return bScalarMultiply(-9.800000190734863f,bScalarMultiply(tangent[2],inverse));
}

float beltMoverAvailableAcceleration(BeltMover* mover)
{
    return driverAvailableAcceleration(mover->movement->bindings.belt->vehicle,
                                        bScalarFiniteRatio(mover->velocityParameter,mover->parameterScale));
}

void beltMoverRecalculate(BeltMover* mover)
{
    CarMovement* movement = mover->movement;
    const BeltMoverBindings* binding = movement->bindings.belt;
    const VehicleState* state = movement->bindings.state;
    mover->elapsed = 0.0f;
    mover->parameterScale = bScalarFiniteRatio((float)mover->path.count,binding->command->length);
    const int32_t first = quickSplineFindLocalClosestPoint(&mover->path,state->position,0);
    const float closest = quickSplineFindClosestParameter(&mover->path,state->position,(float)first,0.004999999888241291f);
    float difference[4],tangent[4];
    (void)beltMoverRelativePosition(mover,state->position,closest,difference,tangent);
    mover->startParameter = closest;normalize(tangent);
    mover->velocityParameter = bScalarMultiply(mover->parameterScale,state->speed);
    const float targetVelocity = bScalarMultiply(mover->parameterScale,*binding->targetSpeed);
    const float change = bScalarAdd(bScalarMultiply(targetVelocity,0.8989999890327454f),-mover->velocityParameter);
    const float duration = *binding->targetDuration > 0.0f ? *binding->targetDuration : 0.5f;
    mover->accelerationParameter = bScalarFiniteRatio(change,duration);
    const float upper = bScalarMultiply(mover->parameterScale,beltMoverAvailableAcceleration(mover));
    if (upper < mover->accelerationParameter) mover->accelerationParameter = upper;
    else {
        const float lower = bScalarMultiply(mover->parameterScale,-20.0f);
        if (mover->accelerationParameter < lower) mover->accelerationParameter = lower;
    }
    float perpendicular[4];beltPerpendicular(perpendicular,tangent);
    const float offset = -dot3(difference,perpendicular);
    if (offset != mover->lateralOffset.desiredValue) mover->lateralOffset.mode = 2;
    mover->lateralOffset.value = offset;
    const float derivative = bScalarMultiply(dot3(state->velocity,perpendicular),0.5f);
    if (derivative != mover->lateralOffset.desiredDerivative) mover->lateralOffset.mode = 2;
    mover->lateralOffset.derivative = derivative;mover->lateralOffset.duration = 0.5f;
    mover->velocityFilter.axes[0].duration = mover->velocityFilter.axes[1].duration = 0.4000000059604645f;
}

void beltMoverActivate(BeltMover* mover)
{
    DriverCommand* command = mover->movement->bindings.belt->command;
    mover->lateralOffset.continuous = 0;
    driverCommandResample(command);
    quickSplineSetPointCount(&mover->path,(uint16_t)command->count);
    for (int index = 0; index < command->count; ++index) {
        memcpy(mover->path.points[index],command->points[index].position,4*sizeof(float));
        mover->path.points[index][3] = 0.0f;mover->path.pointsDirty = 1;
    }
    beltMoverRecalculate(mover);
}

float beltMoverLateralGrip(BeltMover* mover,float acceleration)
{
    const PhysicsCarInfo* info = &mover->movement->info;
    const float magnitude = fabsf(acceleration);
    const float sign = bScalarFiniteRatio(acceleration, magnitude);
    const float speed = bScalarFiniteRatio(mover->velocityParameter, mover->parameterScale);
    const float lateralScale = bScalarMultiply((bScalarAdd(info->tires[1].lateralScale, info->tires[0].lateralScale)), 0.5f);
    const float downforce = bScalarMultiply((bScalarAdd(info->aerodynamics->frontDownforce, info->aerodynamics->rearDownforce)), 0.5f);
    const float requestedForce = bScalarMultiply((bScalarMultiply(info->body->mass, magnitude)), lateralScale);
    const float grip = mover->sliding ? bScalarAdd(info->tires[1].slidingGrip, info->tires[0].slidingGrip) :
                                      bScalarAdd(info->tires[1].staticGrip, info->tires[0].staticGrip);
    const float normalForce = bScalarMultiply((bScalarAdd(bScalarMultiply(speed, downforce), bScalarMultiply(9.800000190734863f, info->body->mass))), (bScalarMultiply(grip, 0.5f)));
    CurveKey keys[4] = {{0,0},{bScalarMultiply(normalForce, 0.8999999761581421f),mover->sliding ? 0.4000000059604645f : 0.10000000149011612f},
        {bScalarMultiply(normalForce, 1.100000023841858f),mover->sliding ? 0.6000000238418579f : 0.4000000059604645f},
        {bScalarMultiply(normalForce, 1.5f),mover->sliding ? 1.0f : 0.699999988079071f}};
    const KeyedCurve curve = {keys,4};
    const int32_t sliding = normalForce < requestedForce;
    if (sliding != mover->sliding) {
        if (sliding) mover->lateralFilter.axes[0].duration = mover->lateralFilter.axes[1].duration = 1.0f;
        else {
            mover->lateralFilter.axes[0].duration = mover->lateralFilter.axes[1].duration = 0.30000001192092896f;
            float derivative[2];tCubic2DGetdVal(&mover->lateralFilter,derivative);
            derivative[0] = bScalarMultiply(derivative[0], 1.600000023841858f);derivative[1] = bScalarMultiply(derivative[1], 1.600000023841858f);
            tCubic2DSetdVal(&mover->lateralFilter,derivative);
        }
        mover->sliding = sliding;
    }
    return bScalarMultiply(sign, keyedCurveEvaluate(&curve,requestedForce));
}

float beltMoverPredictRoadError(BeltMover* mover,float duration)
{
    const PhysicsCarInfo* info = &mover->movement->info;
    const float speed = bScalarFiniteRatio(mover->velocityParameter, mover->parameterScale);
    const float downforce = bScalarMultiply((bScalarAdd(info->aerodynamics->frontDownforce, info->aerodynamics->rearDownforce)), 0.5f);
    const float gravity = bScalarAdd(bScalarFiniteRatio(bScalarMultiply(speed, downforce), info->body->mass), 9.800000190734863f);
    float position[4],velocity[4];
    beltMoverPosition(mover,position,mover->elapsed);beltMoverVelocity(mover,velocity,mover->elapsed);
    float z = position[2], vz = velocity[2], previousTime = 0.0f,penetration = 0.0f;
    for (int index = 1; index < 10; ++index) {
        const float time = bScalarMultiply((bScalarMultiply(duration, (float)index)), 0.10000000149011612f);
        const float step = bScalarAdd(time, -(previousTime));
        const float elapsed = bScalarAdd(mover->elapsed, time);
        const float fallingZ = bScalarAdd(bScalarMultiply(step, (bScalarAdd(bScalarMultiply(step, (-gravity)), vz))), z);
        beltMoverPosition(mover,position,elapsed);
        const float depth = bScalarAdd(fallingZ, -(position[2]));
        if (0.0f < depth) penetration = bScalarAdd(penetration, depth);
        else {
            previousTime = time;beltMoverVelocity(mover,velocity,elapsed);
            z = position[2];vz = velocity[2];
        }
    }
    return bScalarMultiply(penetration, 0.10000000149011612f);
}

float beltMoverMove(BeltMover* mover,float elapsed)
{
    // A native zero-duration capture preserves the current mover state. The
    // original yaw quotient (PAL2050E8) divides by elapsed without a zero
    // branch; a saturated result is not a physical angular velocity and can
    // overflow when the next mover imports it. Do not advance curves, sample
    // history, wheels or replan timers when no simulation time has passed.
    if (elapsed == 0.0f) return elapsed;
    CarMovement* movement = mover->movement;
    const BeltMoverBindings* binding = movement->bindings.belt;
    VehicleState* state = movement->bindings.state;
    if (mover->path.count < 2) beltMoverActivate(mover);
    float previousPosition[4];memcpy(previousPosition,state->position,sizeof(previousPosition));
    if (0.0020000000949949026f < beltMoverPredictRoadError(mover,0.5f)) *binding->replanDelayTicks = 2400;
    mover->elapsed = bScalarAdd(mover->elapsed, elapsed);
    const float speed = bScalarFiniteRatio(bScalarAdd(mover->velocityParameter, bScalarMultiply(mover->accelerationParameter, mover->elapsed)), mover->parameterScale);

    float velocity[4];beltMoverVelocity(mover,velocity,bScalarAdd(mover->elapsed, 0.5f));
    tCubic2DSetValDesired(&mover->velocityFilter,velocity);tCubic2DUpdate(&mover->velocityFilter,elapsed);
    tCubic2DGetVal(&mover->velocityFilter,velocity);
    float forward[4];beltMoverVelocity(mover,forward,bScalarAdd(mover->elapsed, 0.30000001192092896f));
    float perpendicular[4];beltPerpendicular(perpendicular,forward);normalize(perpendicular);
    float acceleration[4];beltMoverAcceleration(mover,acceleration,bScalarAdd(mover->elapsed, 0.30000001192092896f));
    // PAL204F28 multiplies perpendicular by acceleration, in that order.
    driverHistoryPush(&mover->lateralHistory,dot3(perpendicular,acceleration));
    const float grip = beltMoverLateralGrip(mover,mover->lateralHistory.value);
    const float sideSpeed = bScalarMultiply(speed, (bScalarMultiply(grip, 0.30000001192092896f)));
    float lateral[4];
    for (int axis = 0; axis < 4; ++axis) lateral[axis] = bScalarMultiply(perpendicular[axis], sideSpeed);
    tCubic2DSetValDesired(&mover->lateralFilter,lateral);tCubic2DUpdate(&mover->lateralFilter,elapsed);
    tCubic2DGetVal(&mover->lateralFilter,lateral);
    for (int axis = 0; axis < 4; ++axis) velocity[axis] = bScalarAdd(velocity[axis], lateral[axis]);
    const uint16_t heading = vectorAtan2Angle(velocity[0],velocity[1]);
    const uint16_t previousHeading = state->heading;state->heading = heading;

    beltMoverVelocity(mover,forward,mover->elapsed);
    beltPerpendicular(perpendicular,forward);normalize(perpendicular);
    if (-grip != mover->lateralOffset.value) mover->lateralOffset.mode = 2;
    mover->lateralOffset.desiredValue = -grip;
    tCubic1DUpdate(&mover->lateralOffset,elapsed,0.0f,0.0f);
    float position[4];beltMoverPosition(mover,position,mover->elapsed);
    for (int axis = 0; axis < 4; ++axis) state->position[axis] = bScalarAdd(bScalarMultiply(perpendicular[axis], mover->lateralOffset.value), position[axis]);
    carMovementUpdateTerrainPose(movement);vehicleStateUpdateCenterPosition(state);
    const int16_t headingDelta = (int16_t)(uint16_t)(heading-previousHeading);
    state->angularVelocity[0] = state->angularVelocity[1] = 0.0f;
    state->angularVelocity[2] = finiteRatio((float)headingDelta*0.00009587379463482648f,elapsed);
    for (int axis = 0; axis < 4; ++axis) velocity[axis] = state->position[axis]-previousPosition[axis];
    if (fabsf(elapsed) <= 0.00009999999747378752f) vectorSetLength3(velocity,velocity,speed);
    else {
        const float inverse = finiteRatio(1.0f,elapsed);
        for (int axis = 0; axis < 4; ++axis) velocity[axis] *= inverse;
    }
    memcpy(state->velocity,velocity,sizeof(velocity));
    state->forwardSpeed = state->speed = sqrtf(dot3(velocity,velocity));state->velocityFrameValid = 0;
    const float slip[2] = {speed-dot3(velocity,state->orientation),-dot3(velocity,state->orientation+4)};
    mover->wheelVehicle.speed = state->forwardSpeed;mover->wheelVehicle.pursuitMode = state->pursuitMode;
    for (int wheel = 0; wheel < 4; ++wheel) {
        Wheel* component = mover->wheels[wheel];
        const int front = component->torqueInfo->index < 2;
        const float angle = state->wheelAngle[wheel] + state->wheelAngularVelocity[wheel]*mover->elapsed;
        const float angularVelocity = finiteRatio(speed,movement->info.wheels[wheel].reactionScale);
        vehicleStateInitializeWheelContact(state,component,movement->bindings.environment->contact.terrain,
            angle,angularVelocity,0.0f,front ? movement->bindings.controls->steeringAngle : 0);
        state->wheelSlip[wheel][0] = slip[0];state->wheelSlip[wheel][1] = slip[1];state->wheelSlipRatio[wheel] = 0.0f;
        if (!front) {
            component->slip[0] = slip[0];component->slip[1] = slip[1];
            component->contactCorrectionDisabled = (int8_t)mover->sliding;
            component->angularSpeedEstimate = slip[0];
        }
    }
    if (carMotionClassActionAllowed(movement->bindings.ownerMotion) &&
        worldGetDistanceToClosestPlayer3D(binding->world,state->position) < 200.0f) {
        WheelTrailStore* trails = movement->bindings.environment->wheelTrails;
        if (!trails) trails = &mover->localTrailStore;
        const auto& resources=movement->bindings.environment->contact;
        const WheelContactContext contact={resources.terrain,resources.materials,resources.materialCount,elapsed,resources.tuning};
        for (int wheel = 0; wheel < 4; ++wheel)
            wheelEffectsUpdate(mover->wheels[wheel],&contact,trails,mover->trails+wheel,
                movement->bindings.owner,state->centerPosition,movement->bindings.ownerMotion->wheelEffectsFlags,
                movement->bindings.ownerMotion->disabledWheelMask);
    }
    carMovementUpdatePowertrainDisplay(movement,&carDriveDisplayDefaultTuning);
    return elapsed;
}

float* beltMoverPosition(BeltMover* mover,float output[4],float elapsed)
{
    const float parameter = bScalarAdd(mover->startParameter, bScalarMultiply(elapsed, (bScalarAdd(mover->velocityParameter, bScalarMultiply(elapsed, mover->accelerationParameter)))));
    quickSplineEvaluate(&mover->path,output,parameter,0);
    if (parameter < 0.0f && output[2] < mover->path.points[0][2]) output[2] = mover->path.points[0][2];
    return output;
}

float* beltMoverVelocity(BeltMover* mover,float output[4],float elapsed)
{
    const float twiceElapsed = bScalarAdd(elapsed, elapsed);
    const float parameter = bScalarAdd(bScalarMultiply(elapsed, (bScalarAdd(bScalarMultiply(elapsed, mover->accelerationParameter), mover->velocityParameter))), mover->startParameter);
    const float derivative = bScalarAdd(bScalarMultiply(twiceElapsed, mover->accelerationParameter), mover->velocityParameter);
    quickSplineEvaluate(&mover->path,output,parameter,1);
    for (int axis = 0; axis < 4; ++axis) output[axis] = bScalarMultiply(output[axis], derivative);
    return output;
}

float* beltMoverAcceleration(BeltMover* mover,float output[4],float elapsed)
{
    const float twiceElapsed = bScalarAdd(elapsed, elapsed);
    const float parameter = bScalarAdd(bScalarMultiply(elapsed, (bScalarAdd(bScalarMultiply(elapsed, mover->accelerationParameter), mover->velocityParameter))), mover->startParameter);
    const float derivative = bScalarAdd(bScalarMultiply(twiceElapsed, mover->accelerationParameter), mover->velocityParameter);
    const float secondDerivative = bScalarAdd(mover->accelerationParameter, mover->accelerationParameter);
    float first[4],second[4];
    quickSplineEvaluate(&mover->path,first,parameter,1);
    quickSplineEvaluate(&mover->path,second,parameter,2);
    const float squaredDerivative = bScalarMultiply(derivative, derivative);
    for (int axis = 0; axis < 4; ++axis) output[axis] = bScalarAdd(bScalarMultiply(first[axis], secondDerivative), bScalarMultiply(second[axis], squaredDerivative));
    return output;
}

namespace {
void BeginCurveSnapshot(void* context,const char* name)
{
    replaySnapshotBeginSection(static_cast<ReplaySnapshot*>(context),nullptr,name);
}
void MoveCurveSnapshot(void* context,void* data,size_t size)
{
    replaySnapshotMoveData(static_cast<ReplaySnapshot*>(context),data,static_cast<int32_t>(size),-1,0);
}
void EndCurveSnapshot(void* context)
{
    replaySnapshotEndSection(static_cast<ReplaySnapshot*>(context));
}
}

// Complete PAL204468; every serialized object is the mover's same actual
// curve/history/wheel owner. Original empty CarMover snapshot and
// ClearCalculatedVariables hooks are excluded with their exact source proof.
void beltMoverSnapshot(BeltMover* mover,ReplaySnapshot* snapshot)
{
    static_assert(sizeof(tCubic1D)==48 && sizeof(tCubic2D)==96,
        "Belt snapshot scalar curve representation changed");
    replaySnapshotBeginSection(snapshot,nullptr,"BeltMover");
    replaySnapshotMoveFloat(snapshot,&mover->startParameter);
    replaySnapshotMoveFloat(snapshot,&mover->velocityParameter);
    replaySnapshotMoveFloat(snapshot,&mover->accelerationParameter);
    replaySnapshotMoveFloat(snapshot,&mover->elapsed);
    replaySnapshotMoveFloat(snapshot,&mover->parameterScale);
    replaySnapshotMoveUnsigned(snapshot,reinterpret_cast<uint32_t*>(&mover->sliding),1);
    replaySnapshotMoveData(snapshot,&mover->lateralOffset,48,-1,0);
    replaySnapshotMoveData(snapshot,&mover->velocityFilter,96,-1,0);
    replaySnapshotMoveData(snapshot,&mover->lateralFilter,96,-1,0);
    CurveSnapshot curve{snapshot,BeginCurveSnapshot,MoveCurveSnapshot,EndCurveSnapshot,snapshot->recording || snapshot->restoring};
    quickSplineSnapshot(&mover->path,&curve,3);
    driverHistorySnapshot(&mover->lateralHistory,snapshot);
    for (int wheel=0;wheel<4;++wheel)
        wheelSerializePersistentState(mover->wheels[wheel],snapshot);
    replaySnapshotEndSection(snapshot);
}
