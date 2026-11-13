#include "indep/inc/Table.hpp"
#include "indep/inc/MellowMover.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>
#include <cmath>
#include <string.h>

void mellovatorSetWheelMotion(Mellovator* component, const float angularVelocity[4])
{
    memcpy(component->motion.wheelAngularVelocity, angularVelocity, sizeof(component->motion.wheelAngularVelocity));
    memset(component->travel, 0, sizeof(component->travel));
    memset(component->rotationLocked, 0, sizeof(component->rotationLocked));
}

#include "indep/inc/Car.hpp"
#include "indep/inc/eMath.hpp"
static float dotThree(const float* first, const float* second)
{
    const float x = bScalarMultiply(first[0], second[0]);
    const float y = bScalarMultiply(first[1], second[1]);
    const float z = bScalarMultiply(first[2], second[2]);
    return bScalarAdd(bScalarAdd(x, y), z);
}

static void crossThree(float output[4], const float* first, const float* second)
{
    const float x = bScalarAdd(bScalarMultiply(first[1], second[2]), -bScalarMultiply(first[2], second[1]));
    const float y = bScalarAdd(bScalarMultiply(first[2], second[0]), -bScalarMultiply(first[0], second[2]));
    const float z = bScalarAdd(bScalarMultiply(first[0], second[1]), -bScalarMultiply(first[1], second[0]));
    output[0] = x; output[1] = y; output[2] = z; output[3] = 0.0f;
}

static bool normalizeThree(float vector[4])
{
    hp2::Require(std::isfinite(vector[0]) && std::isfinite(vector[1]) && std::isfinite(vector[2]),
        "Mellow airborne direction is not finite");
    const float squared=dotThree(vector,vector);
    if (squared>0.0f && std::isfinite(squared)) {
        const float scale=bScalarDivide(1.0f,bScalarSquareRoot(squared));
        for (int axis=0;axis<4;++axis) vector[axis]=bScalarMultiply(vector[axis],scale);
        return true;
    }
    const double length=std::hypot(static_cast<double>(vector[0]),
        std::hypot(static_cast<double>(vector[1]),static_cast<double>(vector[2])));
    if (length==0.0) return false;
    for (int axis=0;axis<4;++axis)vector[axis]=static_cast<float>(vector[axis]/length);
    return true;
}

static int32_t componentSize(void* object)
{ return mellovatorStateSize((Mellovator*)object); }
static void saveComponent(void* object, float* output)
{ memcpy(output, &((Mellovator*)object)->motion, sizeof(MellovatorState)); }
static void restoreComponent(void* object, const float* input)
{ memcpy(&((Mellovator*)object)->motion, input, sizeof(MellovatorState)); }
static void componentDerivatives(void* object, const void* context, float* output)
{ (void)context; mellovatorComputeDerivatives((Mellovator*)object, output); }
static void prepareComponent(void* object, const void* context)
{
    const PhysicsCarEnvironment* environment = (const PhysicsCarEnvironment*)context;
    mellovatorPrepareStep((Mellovator*)object, environment->stepContext->lastSubstepElapsed);
}
static void applyComponentDelta(void* object, const float* derivative, float elapsed)
{
    Mellovator* component = (Mellovator*)object;
    for (int wheel = 0; wheel < 4; ++wheel) {
        const float delta = bScalarMultiply(elapsed, derivative[wheel]);
        component->motion.wheelAngularVelocity[wheel] =
            bScalarAdd(component->motion.wheelAngularVelocity[wheel], delta);
    }
}
static const ComponentOperations componentOperations = {
    componentSize, saveComponent, restoreComponent, componentDerivatives, prepareComponent, applyComponentDelta
};
static void prepareMover(void* object)
{ mellowMoverPrepare((MellowMover*)object); }
static void moveOwner(void* object, float elapsed)
{ (void)mellowMoverMove((MellowMover*)object, elapsed); }

PhysicsPoolStatus mellowMoverInitialize(MellowMover* mover, const PhysicsCarInfo* info,
                                        CarControls* controls, Car* owner,
                                        CarMotionState* ownerMotion, VehicleState* state,
                                        PhysicsCarEnvironment* environment, PhysicsPools* pools)
{
    if (mover == NULL || info == NULL || controls == NULL || owner == NULL || ownerMotion == NULL ||
        state == NULL || environment == NULL || environment->contact.terrain == NULL || environment->stepContext == NULL ||
        environment->tuning == NULL || environment->contact.tuning == NULL || pools == NULL ||
        info->body == NULL || info->engine == NULL || info->driveTrain == NULL ||
        info->aerodynamics == NULL || info->centerOfMass == NULL || info->wheels == NULL ||
        info->suspension == NULL || info->tires == NULL || info->brakes == NULL)
        return PHYSICS_POOL_INVALID_ARGUMENT;
    memset(mover, 0, sizeof(*mover));
    mover->info = *info;
    mover->controls = controls;
    mover->owner = owner;
    mover->ownerMotion = ownerMotion;
    mover->state = state;
    mover->environment = environment;
    mover->pools = pools;
    state->position = owner->position;
    state->centerOfMass = info->centerOfMass;
    state->wheelInfo = info->wheels;
    memcpy(state->wheelSideOffset, info->wheelSideOffset, sizeof(state->wheelSideOffset));
    componentSystemInitialize(&mover->components, mover->bindings, 2);
    mover->components.stageOwner = mover;
    mover->components.prepareStage = prepareMover;
    PhysicsPoolStatus status = PHYSICS_POOL_OK;
    mover->body = physicsObjectsCreateRigidBody(pools, info->body, &rigidBodyDefaultTuning, &status);
    if (mover->body == NULL) goto failed;
    componentSystemAdd(&mover->components, componentBindRigidBody(mover->body));
    mover->suspension = physicsObjectsAcquireMellovator(pools, &status);
    if (mover->suspension == NULL) goto failed;
    memset(mover->suspension, 0, sizeof(*mover->suspension));
    {
    const MellovatorInfo suspensionInfo = {info->engine, info->driveTrain, info->wheels,
                                         info->suspension, info->centerOfMass, info->preloadDownforceScale};
    mellovatorInitialize(mover->suspension, &suspensionInfo, mover->body, environment->contact.terrain);
    const ComponentBinding binding = {mover->suspension, environment, &componentOperations};
    componentSystemAdd(&mover->components, binding);
    }
    mover->aerodynamics = physicsObjectsCreateAerodynamics(pools, info->aerodynamics, mover->body,
                                                          &aerodynamicsDefaultTuning, &status);
    if (mover->aerodynamics == NULL) goto failed;
    rigidBodySetForwardSpeedLimit(mover->body, info->forwardSpeedLimit);
    environment->stepContext->lastSubstepElapsed = 0.0f;
    mellowMoverCalibrate(mover);
    for (int wheel = 0; wheel < 4; ++wheel) {
        mover->wheels[wheel] = physicsObjectsAcquireWheel(pools, &status);
        if (mover->wheels[wheel] == NULL) goto failed;
        memset(mover->wheels[wheel], 0, sizeof(Wheel));
        wheelInitialize(mover->wheels[wheel], info->wheels + wheel, info->tires + wheel / 2,
                          info->brakes, &mover->wheelView);
    }
    wheelTrailStoreInitialize(&mover->localTrailStore, mover->localTrails, 16);
    mellowMoverLoadState(mover);
    mellowMoverPrepare(mover);
    owner->movement = mover;
    owner->move = moveOwner;
    return PHYSICS_POOL_OK;
failed:
    mellowMoverDispose(mover);
    return status;
}

void mellowMoverDispose(MellowMover* mover)
{
    if (mover == NULL || mover->pools == NULL) return;
    WheelTrailStore* trails = mover->environment->wheelTrails != NULL ?
                             mover->environment->wheelTrails : &mover->localTrailStore;
    for (int wheel = 0; wheel < 4; ++wheel) {
        wheelTrailDetach(trails, &mover->wheelTrails[wheel]);
        if (mover->wheels[wheel] != NULL) physicsObjectsReleaseWheel(mover->pools, mover->wheels[wheel]);
    }
    if (mover->aerodynamics != NULL) physicsObjectsReleaseAerodynamics(mover->pools, mover->aerodynamics);
    if (mover->suspension != NULL) physicsObjectsReleaseMellovator(mover->pools, mover->suspension);
    if (mover->body != NULL) physicsObjectsReleaseRigidBody(mover->pools, mover->body);
    if (mover->owner != NULL && mover->owner->movement == mover) {
        mover->owner->movement = NULL;
        mover->owner->move = NULL;
    }
    memset(mover, 0, sizeof(*mover));
}

void mellowMoverCalibrate(MellowMover* mover)
{
    rigidBodySetCenterOfMass(mover->body, mover->info.centerOfMass);
    rigidBodyRefreshMass(mover->body);
    const float downforce = aerodynamicsEstimateDownforce(mover->aerodynamics, mover->state->forwardSpeed);
    mellovatorCalibrate(mover->suspension, downforce, mover->environment->tuning->useCenterOfMassForLoad);
}

void mellowMoverLoadState(MellowMover* mover)
{
    Mellovator* component = mover->suspension;
    component->throttle = mover->controls->throttle;
    component->brake = mover->controls->brake;
    component->steeringAngle = mover->controls->steeringAngle;
    component->desiredGear = mover->controls->gearRequest;
    rigidBodySetMotion(mover->body, mover->state->position, mover->state->orientation,
                        mover->state->velocity, mover->state->angularVelocity);
    mellovatorSetWheelMotion(component, mover->state->wheelAngularVelocity);
    mellowMoverPrepare(mover);
}

void mellowMoverPrepare(MellowMover* mover)
{
    componentSystemPrepare(&mover->components);
    const float impulse = mellovatorResolveBottomOutImpulses(mover->suspension, mover->state,
        mover->environment->contact.tuning->contactImpulseScale);
    if (0.0f < impulse) componentSystemPrepare(&mover->components);
    mellowMoverCollectForces(mover);
}

void mellowMoverCollectForces(MellowMover* mover)
{
    mellovatorResolveForces(mover->suspension);
    aerodynamicsApplyForces(mover->aerodynamics, NULL);
    mellowMoverStabilizeAirborne(mover);
}

void mellowMoverSetBoost(MellowMover* mover, float boost)
{
    mover->aerodynamics->speedScale = mover->state->performanceScale;
    mover->aerodynamics->dragScale = bScalarAdd(1.0f, -mover->owner->interactionFactor);
    rigidBodySetSpeedLimitScale(mover->body, boost);
    float friction = boost;
    if (30.0f < bScalarMultiply(mover->state->forwardSpeed, 2.236999988555908f) && mover->state->pursuitMode == 1)
        friction = bScalarMultiply(boost, 0.699999988079071f);
    mover->suspension->frictionBoost = friction;
    mover->suspension->boost = boost;
}

void mellowMoverStabilizeAirborne(MellowMover* mover)
{
    RigidBody* body = mover->body;
    const VehicleState* state = mover->state;
    if (state->activeContactCount < 2) {
        static const CurveKey keys[] = {{0.0f, 0.0f}, {0.10000000149011612f, 0.0f}, {2.0f, 20.0f}};
        const KeyedCurve curve = {keys, 3};
        const float speed = bScalarSquareRoot(dotThree(body->angularVelocity, body->angularVelocity));
        const float scale = -bScalarMultiply(body->mass, keyedCurveEvaluate(&curve, speed));
        float torque[4];
        for (int axis = 0; axis < 4; ++axis) torque[axis] = bScalarMultiply(body->angularVelocity[axis], scale);
        rigidBodyAddTorque(body, torque);
    }
    if (state->activeContactCount != 0 || !(0.10000000149011612f < state->orientation[10])) return;
    float direction[4] = {state->velocity[0], state->velocity[1], 0.0f, state->velocity[3]};
    float torque[4];
    // At zero horizontal velocity there is no desired travel heading.
    // Keep angular damping and roll stabilization, but invent no heading.
    if (normalizeThree(direction)) {
        crossThree(torque, direction, state->orientation);
        const float firstScale = bScalarMultiply(body->mass,
            bScalarMultiply(bScalarAdd(dotThree(direction, state->orientation), -1.0f), 100.0f));
        vectorSetLength3(torque, torque, firstScale);
        rigidBodyAddTorque(body, torque);
    }
    const float up[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    float lateral[4];
    crossThree(lateral, up, state->orientation);
    if (!normalizeThree(lateral)) return;
    crossThree(torque, lateral, state->orientation + 4);
    const float secondScale = bScalarMultiply(body->mass,
        bScalarMultiply(bScalarAdd(dotThree(lateral, state->orientation + 4), -1.0f), 100.0f));
    vectorSetLength3(torque, torque, secondScale);
    rigidBodyAddTorque(body, torque);
}

void mellowMoverPublishState(MellowMover* mover, float elapsed)
{
    const WheelContactContext contact=physicsCarContactContext(mover->environment);
    VehicleState* state = mover->state;
    RigidBody* body = mover->body;
    Mellovator* component = mover->suspension;
    memcpy(state->position, body->motion.position, sizeof(body->motion.position));
    memcpy(state->orientation, body->motion.orientation, sizeof(state->orientation));
    vehicleStateUpdateCenterPosition(state);
    memcpy(state->velocity, body->linearVelocity, sizeof(state->velocity));
    state->speed = bScalarSquareRoot(dotThree(body->linearVelocity, body->linearVelocity));
    state->velocityFrameValid = 0;
    memcpy(state->angularVelocity, body->angularVelocity, sizeof(state->angularVelocity));
    state->heading = vectorAtan2Angle(body->motion.orientation[0], body->motion.orientation[1]);
    state->clutchEngaged = 1;
    state->leftSlip = state->rightSlip = 0.0f;
    state->forwardSpeed = dotThree(body->motion.orientation, state->velocity);
    mover->wheelView.speed = state->forwardSpeed;
    mover->wheelView.pursuitMode = state->pursuitMode;
    WheelTrailStore* trails = mover->environment->wheelTrails != NULL ?
                             mover->environment->wheelTrails : &mover->localTrailStore;
    for (int wheel = 0; wheel < 4; ++wheel) {
        const float angularVelocity = component->motion.wheelAngularVelocity[wheel];
        const float angle = bScalarAdd(state->wheelAngle[wheel], bScalarMultiply(elapsed, angularVelocity));
        const uint16_t steering = mover->info.wheels[wheel].index < 2 ? mover->controls->steeringAngle : 0;
        const float travel = minimumFinite(component->travel[wheel], mover->info.suspension[wheel / 2].extensionLimit);
        vehicleStateInitializeWheelContact(state, mover->wheels[wheel], mover->environment->contact.terrain,
                                            angle, angularVelocity, travel, steering);
        vehicleStateSetWheelContact(state, wheel, (int8_t)component->touchingGround[wheel]);
        memcpy(state->wheelSlip[wheel], component->slip[wheel], sizeof(component->slip[wheel]));
        state->wheelSlipRatio[wheel] = 0.0f;
        state->wheelNormalLoad[wheel] = component->suspensionForce[wheel];
        Wheel* visual = mover->wheels[wheel];
        visual->contactPresent = component->touchingGround[wheel] != 0;
        memcpy(visual->slip, component->slip[wheel], sizeof(visual->slip));
        visual->contactCorrectionDisabled = (int8_t)component->slipping[wheel];
        visual->angularSpeedEstimate = component->slip[wheel][0];
        if (carMotionClassActionAllowed(mover->ownerMotion))
            wheelEffectsUpdate(visual, &contact, trails, mover->wheelTrails + wheel,
                mover->owner, state->centerPosition, mover->ownerMotion->wheelEffectsFlags,
                mover->ownerMotion->disabledWheelMask);
    }
    state->gear = component->currentGear;
    state->engineRpm = component->engineRpm;
}

float mellowMoverMove(MellowMover* mover, float elapsed)
{
    mellowMoverCalibrate(mover);
    Mellovator* component = mover->suspension;
    component->throttle = mover->controls->throttle;
    component->brake = maximumFinite(mover->controls->brake, mover->controls->handbrake * 0.5f);
    component->steeringAngle = mover->controls->steeringAngle;
    component->desiredGear = mover->controls->gearRequest;
    mover->aerodynamics->downforceMode = (float)mover->state->activeContactCount * 0.25f;
    mover->aerodynamics->dragReduction = mover->controls->throttle;
    memset(mover->state->suspensionImpulse, 0, sizeof(mover->state->suspensionImpulse));
    mellowMoverSetBoost(mover, mover->state->performanceScale);
    const int32_t steps = convertFloatToInt32(elapsed / 0.00839999970048666f) + 1;
    const float step = elapsed / (float)steps;
    for (int32_t index = 0; index < steps; ++index) {
        mover->environment->stepContext->lastSubstepElapsed = step;
        componentSystemIntegrateEuler(&mover->components, step, mover->derivatives, 33);
        motionMatrixOrthonormalize(mover->body->motion.orientation, mover->body->motion.orientation);
        // The integrator prepared omega before this normalization. Refresh its
        // angular cache without discarding the forces already collected.
        rigidBodyRefreshAngularVelocity(mover->body);
    }
    mellowMoverPublishState(mover, elapsed);
    return elapsed;
}

#include "indep/inc/Replay.hpp"

// PAL21A3D8..21A5C0. Captures the actual simplified suspension owner; tyre
// vectors and wheel flags use the source widths and per-wheel transfer order.
void mellovatorSnapshot(Mellovator* component,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"Mellovator");
    replaySnapshotMoveData(snapshot,component->motion.wheelAngularVelocity,16,-1,0);
    for(int wheel=0;wheel<4;++wheel) {
        replaySnapshotMoveFloat(snapshot,&component->travel[wheel]);
        replaySnapshotMoveFloat(snapshot,&component->travelSpeed[wheel]);
        replaySnapshotMoveFloat(snapshot,&component->suspensionForce[wheel]);
        replaySnapshotMoveInteger(snapshot,&component->touchingGround[wheel],1);
        replaySnapshotMoveData(snapshot,component->tireForce[wheel],8,-1,0);
        replaySnapshotMoveData(snapshot,component->slip[wheel],8,-1,0);
        replaySnapshotMoveData(snapshot,&component->slipping[wheel],1,4,0);
        replaySnapshotMoveInteger(snapshot,&component->rotationLocked[wheel],1);
        replaySnapshotMoveFloat(snapshot,&component->compressionExcess[wheel]);
        replaySnapshotMoveData(snapshot,component->groundNormal[wheel],12,-1,0);
    }
    replaySnapshotMoveFloat(snapshot,&component->engineRpm);
    replaySnapshotMoveInteger(snapshot,&component->currentGear,1);
    replaySnapshotMoveFloat(snapshot,&component->throttle);
    replaySnapshotMoveFloat(snapshot,&component->brake);
    replaySnapshotMoveData(snapshot,&component->steeringAngle,2,2,0);
    replaySnapshotMoveInteger(snapshot,&component->desiredGear,1);
    replaySnapshotMoveFloat(snapshot,&component->boost);
    replaySnapshotMoveFloat(snapshot,&component->frictionBoost);
    replaySnapshotEndSection(snapshot);
}

// Remaining existing Mellovator implementation, individually proven original
// unit; shares the identical native dotThree helper already in this file.




static void wheelLever(const Mellovator* component, int wheel, float lever[4])
{
    // Original VU SUB from vf0 precedes ADD of the wheel mount. Preserve both
    // finite operation boundaries, including the homogeneous component.
    for (int axis = 0; axis < 4; ++axis)
        lever[axis] = bScalarAdd(bScalarAdd(axis == 3 ? 1.0f : 0.0f, -component->info.centerOfMass[axis]),
                                component->info.wheels[wheel].mountPosition[axis]);
}

Mellovator* mellovatorInitialize(Mellovator* component, const MellovatorInfo* info,
                                RigidBody* body, TerrainSurface* terrain)
{
    component->info = *info;
    component->body = body;
    component->terrain = terrain;
    sampledCurveInitialize(&component->torqueCurve, info->engine->torqueCurve, 9,
                            info->engine->idleRpm, info->engine->maximumRpm);
    sampledCurveInitialize(&component->brakingCurve, info->engine->brakingCurve, 3,
                            info->engine->idleRpm, info->engine->maximumRpm);
    DriveTrainTuning shiftTuning = driveTrainDefaultTuning;
    shiftTuning.upshiftRpmScale = 1.0f;
    driveTrainBuildShiftTables(info->driveTrain, info->engine, component->upshiftRpm,
                                component->downshiftRpm, &shiftTuning);
    component->boost = 1.0f;
    component->currentGear = 2;
    return component;
}

void mellovatorCalibrate(Mellovator* component, float downforce, int useCenterOfMass)
{
    const float center = useCenterOfMass ? component->info.centerOfMass[0] : 0.0f;
    const float front = bScalarAdd(component->info.wheels[0].mountPosition[0], -center);
    const float rear = bScalarAdd(component->info.wheels[3].mountPosition[0], -center);
    const float rearFraction = bScalarMultiply(bScalarDivide(front, bScalarAdd(front, -rear)), 0.5f);
    const float aerodynamicLoad = bScalarMultiply(component->info.preloadDownforceScale, downforce);
    const float load = bScalarAdd(bScalarMultiply(component->body->mass, 9.800000190734863f), aerodynamicLoad);
    const float frontFraction = bScalarAdd(0.5f, -rearFraction);
    component->preload[0] = component->preload[1] = bScalarMultiply(load, frontFraction);
    component->preload[2] = component->preload[3] = bScalarMultiply(load, rearFraction);
}






float* mellovatorComputeDerivatives(const Mellovator* component, float derivatives[4])
{
    static const CurveKey keys[] = {{0.0f, 0.30000001192092896f}, {20.0f, 0.5f}};
    const KeyedCurve damping = {keys, 2};
    const float speed = bScalarSquareRoot(dotThree(component->body->linearVelocity, component->body->linearVelocity));
    for (int wheel = 0; wheel < 4; ++wheel) {
        float torque = 0.0f;
        if (!component->rotationLocked[wheel]) {
            if (wheel >= 2) {
                const float rpm = bScalarDivide(component->engineRpm, component->boost);
                torque = bScalarMultiply(component->throttle, sampledCurveEvaluate(&component->torqueCurve, rpm));
                const float drag = bScalarMultiply(bScalarAdd(1.0f, -component->throttle),
                                                   sampledCurveEvaluate(&component->brakingCurve, rpm));
                torque = bScalarAdd(torque, -drag);
                if (component->throttle < 0.05000000074505806f && component->engineRpm == component->info.engine->idleRpm)
                    torque = 0.0f;
                torque = bScalarMultiply(torque, component->info.driveTrain->rearFinalDrive);
                const float ratio = bScalarMultiply(component->info.driveTrain->gearRatios[component->currentGear], 0.5f);
                torque = bScalarMultiply(torque, ratio);
            }
            const float angular = minimumFinite(component->motion.wheelAngularVelocity[wheel], 20.0f);
            const float braking = bScalarMultiply(component->brake, bScalarMultiply(angular, -0.5f));
            float longitudinalScale = 1.0f;
            float dampingScale = 1.25f;
            if (1.0f < component->boost) {
                const float excess = bScalarAdd(component->boost, -1.0f);
                dampingScale = bScalarMultiply(bScalarAdd(1.0f, bScalarMultiply(excess, 2.5f)), dampingScale);
                longitudinalScale = bScalarAdd(1.0f, bScalarMultiply(excess, 3.0f));
            }
            float resistance = keyedCurveEvaluate(&damping, speed);
            if (wheel >= 2) resistance = bScalarDivide(resistance, bScalarMultiply(longitudinalScale, dampingScale));
            torque = bScalarAdd(bScalarMultiply(torque, longitudinalScale), braking);
            torque = bScalarAdd(torque, -bScalarMultiply(component->tireForce[wheel][0], resistance));
        }
        derivatives[wheel] = bScalarMultiply(torque, 124.99999237060547f);
    }
    return derivatives;
}

void mellovatorGroundHeightsAndPositions(Mellovator* component, float positions[4][4], float heights[4])
{
    for (int wheel = 0; wheel < 4; ++wheel) {
        float lever[4];
        wheelLever(component, wheel, lever);
        vectorMatrixTransformPoint(positions[wheel], component->body->motion.orientation, lever);
        for (int axis = 0; axis < 4; ++axis)
            positions[wheel][axis] = bScalarAdd(positions[wheel][axis], component->body->motion.position[axis]);
        heights[wheel] = terrainSurfaceElevation(component->terrain, positions[wheel], NULL,
                                                  component->groundNormal[wheel], NULL);
    }
}

// The two contact-force owners use explicit PAL scalar/VU boundaries. Keep
// this helper local to those owners; other mover arithmetic has its own proof.
static float mellovatorContactDot(const float* first, const float* second)
{
    const float x=bScalarMultiply(first[0],second[0]);
    const float y=bScalarMultiply(first[1],second[1]);
    const float z=bScalarMultiply(first[2],second[2]);
    return bScalarAdd(bScalarAdd(x,y),z);
}

void mellovatorComputeSuspensionForces(Mellovator* component, float elapsed)
{
    float positions[4][4], heights[4];
    mellovatorGroundHeightsAndPositions(component, positions, heights);
    for (int wheel = 0; wheel < 4; ++wheel) {
        const SuspensionInfo* info = component->info.suspension + wheel / 2;
        const float alignment = mellovatorContactDot(component->groundNormal[wheel], component->body->motion.orientation + 8);
        float travel = alignment < 0.0010000000474974513f ? -100.0f :
                       bScalarDivide(bScalarAdd(heights[wheel],-positions[wheel][2]),alignment);
        travel = bScalarAdd(travel,component->info.wheels[wheel].mountPosition[3]);
        const float previous = component->travel[wheel];
        if (travel < info->extensionStop) {
            component->travel[wheel] = info->extensionStop;
            component->touchingGround[wheel] = 0;
            component->compressionExcess[wheel] = 0.0f;
        } else if (info->bumpStop < travel) {
            component->travel[wheel] = info->bumpStop;
            component->touchingGround[wheel] = 1;
            component->compressionExcess[wheel] = bScalarAdd(travel,-info->bumpStop);
        } else {
            component->travel[wheel] = travel;
            component->touchingGround[wheel] = 1;
            component->compressionExcess[wheel] = 0.0f;
        }
        const float current = component->travel[wheel];
        const float speed = bScalarDivide(bScalarAdd(current,-previous),maximumFinite(elapsed,0.00009999999747378752f));
        component->travelSpeed[wheel] = speed;
        float force = 0.0f;
        if (component->touchingGround[wheel]) {
            const float nonlinearity = bScalarMultiply(maximumFinite(current,0.0f),info->nonlinearRate);
            const float spring = bScalarMultiply(info->springRate,bScalarAdd(nonlinearity,1.0f));
            force = bScalarAdd(component->preload[wheel],bScalarMultiply(current,spring));
            const float damping = speed <= 0.0f ? info->negativeDamping : info->positiveDamping;
            force = bScalarAdd(force,bScalarMultiply(speed,damping));
            force = bScalarAdd(force,bScalarMultiply(info->antiRollRate,bScalarAdd(current,-component->travel[wheel ^ 1])));
            if (force < 0.0f) force = 0.0f;
        }
        component->suspensionForce[wheel] = force;
    }
}

void mellovatorPrepareStep(Mellovator* component, float elapsed)
{
    mellovatorComputeSuspensionForces(component, elapsed);
    for (int wheel = 0; wheel < 4; ++wheel) {
        if (0.949999988079071f < component->brake && fabsf(component->motion.wheelAngularVelocity[wheel]) < 30.0f) {
            component->motion.wheelAngularVelocity[wheel] = 0.0f;
            component->rotationLocked[wheel] = 1;
        }
        if (component->brake < 0.20000000298023224f) component->rotationLocked[wheel] = 0;
    }
    const DriveTrainInfo* drive = component->info.driveTrain;
    const float angular = bScalarAdd(component->motion.wheelAngularVelocity[2], component->motion.wheelAngularVelocity[3]);
    const float drivenRpm = bScalarMultiply(bScalarMultiply(drive->rearFinalDrive, angular), 4.774652004241943f);
    int32_t gear = component->currentGear;
    float rpm = bScalarMultiply(drivenRpm, drive->gearRatios[gear]);
    if (component->desiredGear == -1) {
        if (gear < 2) component->currentGear = gear = 2;
        while (gear - 1 < drive->forwardGearCount && component->upshiftRpm[gear] < rpm) {
            component->currentGear = ++gear;
            rpm = bScalarMultiply(drivenRpm, drive->gearRatios[gear]);
        }
        while (gear - 1 >= 2 && rpm < component->downshiftRpm[gear]) {
            component->currentGear = --gear;
            rpm = bScalarMultiply(drivenRpm, drive->gearRatios[gear]);
        }
    } else if (component->desiredGear == 0) {
        component->currentGear = 0;
        rpm = bScalarMultiply(drivenRpm, drive->gearRatios[0]);
    }
    if (rpm < component->info.engine->idleRpm) rpm = component->info.engine->idleRpm;
    else if (component->info.engine->maximumRpm < rpm) rpm = component->info.engine->maximumRpm;
    component->engineRpm = rpm;
}

static void addSlopeHold(float force[4],float slopeX,float slopeY,
    float load,float bodyUpZ,float frictionLimit)
{
    hp2::Require(std::isfinite(slopeX) && std::isfinite(slopeY) &&
        std::isfinite(load) && load>=0.0f && std::isfinite(bodyUpZ) &&
        std::isfinite(frictionLimit) && frictionLimit>=0.0f,
        "Mellow contact slope support has invalid physical state");
    // PAL21B37C/21B3CC divides by body-up.Z, which can be zero even
    // with a valid terrain contact. This is a slope-holding correction,
    // separate from the real suspension/tire force already in force[].
    // Its native domain is bounded by the existing contact friction budget.
    const double x=static_cast<double>(slopeX)*load;
    const double y=static_cast<double>(slopeY)*load;
    const double projected=std::hypot(x,y);
    if (projected==0.0 || frictionLimit==0.0f) return;
    if (bodyUpZ!=0.0f && projected<=static_cast<double>(frictionLimit)*std::fabs(bodyUpZ)) {
        // Preserve the ordinary finite float operation order. A wider
        // intermediate is needed only if the ratio itself would overflow
        // before multiplication by a tiny, otherwise valid slope component.
        if (std::fabs(static_cast<double>(load)/bodyUpZ)<=FLT_MAX) {
            const float ratio=bScalarDivide(load,bodyUpZ);
            force[0]=bScalarAdd(force[0],bScalarMultiply(slopeX,ratio));
            force[1]=bScalarAdd(force[1],bScalarMultiply(slopeY,ratio));
        } else {
            force[0]=bScalarAdd(force[0],static_cast<float>(x/bodyUpZ));
            force[1]=bScalarAdd(force[1],static_cast<float>(y/bodyUpZ));
        }
    } else {
        // At exactly sideways orientation choose the upright-side limit;
        // the sign of an incidental IEEE negative zero is not a game rule.
        const double scale=(bodyUpZ<0.0f ? -frictionLimit:frictionLimit)/projected;
        force[0]=bScalarAdd(force[0],static_cast<float>(x*scale));
        force[1]=bScalarAdd(force[1],static_cast<float>(y*scale));
    }
    hp2::Require(std::isfinite(force[0]) && std::isfinite(force[1]),
        "Mellow contact force exceeds the native finite domain");
}

void mellovatorResolveForces(Mellovator* component)
{
    RigidBody* body = component->body;
    const float* frame = body->motion.orientation;
    for (int wheel = 0; wheel < 4; ++wheel) {
        float localLever[4], worldLever[4];
        // PAL21AFB0 SUB from vf0, then21AFC8 ADD the wheel mount.
        for(int axis=0;axis<4;++axis)
            localLever[axis]=bScalarAdd(bScalarAdd(axis==3?1.0f:0.0f,-component->info.centerOfMass[axis]),
                component->info.wheels[wheel].mountPosition[axis]);
        vectorMatrixTransformPoint(worldLever, frame, localLever);
        component->slipping[wheel] = 0;
        float localForce[4] = {0.0f, 0.0f, component->suspensionForce[wheel], 0.0f};
        if (component->touchingGround[wheel]) {
            const float velocityX = bScalarAdd(body->linearVelocity[0],-bScalarMultiply(body->angularVelocity[2],worldLever[1]));
            const float velocityY = bScalarAdd(body->linearVelocity[1],bScalarMultiply(body->angularVelocity[2],worldLever[0]));
            float heading = bScalarMultiply((float)vectorAtan2Angle(frame[0],frame[1]),0.0000958738019107841f);
            const float steering = wheel < 2 ? bScalarMultiply((float)component->steeringAngle,0.0000958738019107841f) : 0.0f;
            if(wheel<2)heading = bScalarAdd(heading,steering);
            const float cosine = rotationSineRadians(bScalarAdd(heading,1.5707964897155762f));
            const float sine = rotationSineRadians(heading);
            const float longitudinal = bScalarAdd(bScalarMultiply(velocityX,cosine),bScalarMultiply(velocityY,sine));
            const float lateral = bScalarAdd(bScalarMultiply(-velocityX,sine),bScalarMultiply(velocityY,cosine));
            const float rolling = bScalarMultiply(component->motion.wheelAngularVelocity[wheel],component->info.wheels[wheel].mountPosition[3]);
            const float slipX = bScalarAdd(longitudinal,-rolling);
            component->slip[wheel][0] = slipX;
            component->slip[wheel][1] = lateral;
            const float magnitude = bScalarSquareRoot(bScalarAdd(bScalarMultiply(slipX,slipX),bScalarMultiply(lateral,lateral)));
            float friction = 1.0f;
            if (bScalarMultiply(component->frictionBoost,0.05000000074505806f) < magnitude)
                friction = bScalarMultiply(component->frictionBoost,0.25f);
            if (6.0f < magnitude) component->slipping[wheel] = 1;
            const float scale = bScalarMultiply(component->suspensionForce[wheel],1.25f);
            const float tireX = bScalarMultiply(bScalarMultiply(-slipX,friction),scale);
            const float tireY = bScalarMultiply(bScalarMultiply(-lateral,friction),scale);
            const float angle = steering;
            const float wheelCosine = rotationSineRadians(bScalarAdd(angle,1.5707964897155762f));
            const float wheelSine = rotationSineRadians(angle);
            localForce[0] = bScalarAdd(bScalarMultiply(wheelCosine,tireX),-bScalarMultiply(wheelSine,tireY));
            localForce[1] = bScalarAdd(bScalarMultiply(wheelSine,tireX),bScalarMultiply(wheelCosine,tireY));
        }
        float force[4];
        vectorMatrixTransformPoint(force, frame, localForce);
        if (component->touchingGround[wheel]) {
            const float magnitude = bScalarSquareRoot(bScalarAdd(bScalarMultiply(force[0],force[0]),bScalarMultiply(force[1],force[1])));
            const float limit = bScalarMultiply(bScalarMultiply(component->suspensionForce[wheel],1000.0f),component->frictionBoost);
            if (limit < magnitude) {
                const float scale = bScalarDivide(limit,magnitude);
                force[0] = bScalarMultiply(force[0],scale);
                force[1] = bScalarMultiply(force[1],scale);
            }
            float boost = bScalarAdd(component->boost,-1.0f);
            boost = 0.0f < boost ? bScalarAdd(bScalarMultiply(boost,2.5f),1.0f) : 1.0f;
            force[0] = bScalarMultiply(force[0],boost);
            force[1] = bScalarMultiply(force[1],boost);
            if (component->rotationLocked[wheel] ||
                (0.4000000059604645f < component->brake &&
                 bScalarSquareRoot(mellovatorContactDot(body->linearVelocity,body->linearVelocity)) < 0.10000000149011612f)) {
                addSlopeHold(force,-frame[8],-frame[9],
                    component->suspensionForce[wheel],frame[10],limit);
            } else if (magnitude < limit) {
                const float slopeX = bScalarMultiply(-frame[8],fabsf(frame[4]));
                const float slopeY = bScalarMultiply(-frame[9],fabsf(frame[5]));
                addSlopeHold(force,slopeX,slopeY,
                    component->suspensionForce[wheel],frame[10],limit);
            }
        }
        rigidBodyApplyForce(body, force, worldLever);
        component->tireForce[wheel][0] = localForce[0];
        component->tireForce[wheel][1] = localForce[1];
    }
}

float mellovatorResolveBottomOutImpulses(Mellovator* component, VehicleState* state,
                                        float impulseScale)
{
    float total = 0.0f;
    for (int wheel = 0; wheel < 4; ++wheel) {
        if (!(0.0f < component->compressionExcess[wheel])) continue;
        float local[4], lever[4], velocity[4];
        wheelLever(component, wheel, local);
        local[2] = bScalarAdd(local[2], component->travel[wheel]);
        vectorMatrixTransformPoint(lever, component->body->motion.orientation, local);
        suspensionPointVelocity(velocity, component->body->linearVelocity,
                                  component->body->angularVelocity, lever);
        float impulse = -dotThree(velocity, component->groundNormal[wheel]);
        if (!(0.0f < impulse)) continue;
        impulse = bScalarMultiply(impulse, impulseScale);
        impulse = bScalarMultiply(impulse, component->body->mass);
        float vector[4];
        for (int axis = 0; axis < 4; ++axis)
            vector[axis] = bScalarMultiply(component->groundNormal[wheel][axis], impulse);
        total = bScalarAdd(total, impulse);
        rigidBodyApplyImpulse(component->body, vector, lever);
        state->suspensionImpulse[wheel] = bScalarAdd(state->suspensionImpulse[wheel], impulse);
    }
    return total;
}

// Full PAL2198C8..219974, MellowMover::DoSnapshot. Every component is the
// mover's existing pool allocation; the source empty CarMover hook adds no data.
void mellowMoverSnapshot(MellowMover* mover,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,mover,"MellowMover");
    rigidBodySnapshot(mover->body,snapshot);
    mellovatorSnapshot(mover->suspension,snapshot);
    aerodynamicsResetTransientState(mover->aerodynamics,snapshot);
    for(int wheel=0;wheel<4;++wheel)
        wheelSerializePersistentState(mover->wheels[wheel],snapshot);
    replaySnapshotEndSection(snapshot);
}
