#include "indep/inc/PhysicsCar.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Table.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/CarDamage.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/RealDriver.hpp"
#include "indep/inc/TrackedController.hpp"
#include "bware/inc/bWare.hpp"

#include <limits.h>
#include <math.h>
#include <string.h>

// Complete PAL137450..1375EC. These are the existing component instances
// and exact source transfer order, including both momentum timers and the
// source steering virtual call. CarMover::DoSnapshot1309F0 is truly empty.

#include "indep/inc/MellowMover.hpp"
#include "indep/inc/CarMover.hpp"
#include <stdlib.h>
extern "C" void* bMalloc(int,const char*,int,int);
extern "C" void bFree(void*);

// InitPhysics / ClosePhysics own these ten component families. PAL creates
// ordinary bWare pools and the component constructors call bOMalloc, whose
// expansion is (initialCount >> 2) + 1. Host pointers require wider slots;
// linked blocks retain every live component address across that expansion.
struct PhysicsSlotEntry {
    PhysicsSlotEntry* next;
    unsigned char* object;
    unsigned char state; // 0 never acquired, 1 live, 2 released
};

struct PhysicsSlotBlock {
    PhysicsSlotBlock* next;
    unsigned char* memory;
    PhysicsSlotEntry* entries;
    uint32_t count;
};

struct PhysicsSlotPool {
    PhysicsSlotBlock* blocks;
    PhysicsSlotEntry* freeHead;
    size_t nativeBytes;
    size_t stride;
    uint32_t initialCount;
    uint32_t capacity;
    uint32_t liveCount;
    uint32_t peakCount;
};

struct PhysicsPools {
    PhysicsSlotPool slots[PHYSICS_OBJECT_TYPE_COUNT];
};

static const PhysicsObjectInfo componentInfo[PHYSICS_OBJECT_TYPE_COUNT] = {
    {"RigidBodySlotPool", 0x180u, 16u, 1u, 32u},
    {"TwoWheelAckermanSteeringSlotPool", 0x90u, 16u, 1u, 0u},
    {"WheelSlotPool", 0x190u, 16u, 4u, 0u},
    {"EngineSlotPool", 0x5cu, 16u, 1u, 0u},
    {"DriveTrainSlotPool", 0x98u, 16u, 1u, 0u},
    {"AerodynamicsSlotPool", 0x18u, 16u, 1u, 0u},
    {"SuspensionSlotPool", 0xb0u, 16u, 4u, 0u},
    {"MellovatorSlotPool", 0x1c0u, 16u, 1u, 0u},
    {"MellowMoverSlotPool", 0x40u, 16u, 1u, 0u},
    {"NullMoverSlotPool", 0x0cu, 16u, 1u, 0u}
};

static int physicsPoolValidType(PhysicsObjectType type)
{
    return type >= PHYSICS_OBJECT_RIGID_BODY && type < PHYSICS_OBJECT_TYPE_COUNT;
}

const PhysicsObjectInfo* physicsObjectInfo(PhysicsObjectType type)
{
    return physicsPoolValidType(type) ? &componentInfo[type] : NULL;
}

static void physicsPoolFreeStorage(PhysicsSlotPool* slot)
{
    PhysicsSlotBlock* block = slot->blocks;
    while (block != NULL) {
        PhysicsSlotBlock* next = block->next;
        bFree(block->memory);
        bFree(block->entries);
        free(block);
        block = next;
    }
    memset(slot, 0, sizeof(*slot));
}

static PhysicsPoolStatus physicsPoolAddBlock(PhysicsSlotPool* slot, uint32_t count,
                                             const char* name)
{
    if (count == 0) return PHYSICS_POOL_OK;
    if (count > UINT32_MAX - slot->capacity || slot->stride > INT_MAX / count ||
        sizeof(PhysicsSlotEntry) > INT_MAX / count)
        return PHYSICS_POOL_OVERFLOW;
    PhysicsSlotBlock* block = static_cast<PhysicsSlotBlock*>(calloc(1, sizeof(*block)));
    if (block == NULL) return PHYSICS_POOL_OUT_OF_MEMORY;
    block->memory = static_cast<unsigned char*>(bMalloc(static_cast<int>(slot->stride * count), name, 0, 0x10));
    block->entries = static_cast<PhysicsSlotEntry*>(bMalloc(static_cast<int>(sizeof(PhysicsSlotEntry) * count),
                                                           "PhysicsSlotLinks", 0, 0x10));
    if (block->memory == NULL || block->entries == NULL) {
        bFree(block->memory);
        bFree(block->entries);
        free(block);
        return PHYSICS_POOL_OUT_OF_MEMORY;
    }
    block->count = count;
    for (uint32_t i = 0; i < count; ++i) {
        block->entries[i].object = block->memory + static_cast<size_t>(i) * slot->stride;
        block->entries[i].state = 0;
        block->entries[i].next = i + 1 < count ? &block->entries[i + 1] : slot->freeHead;
    }
    // Publish only after all allocations succeed: failure keeps the pool and
    // all borrowed component pointers unchanged.
    block->next = slot->blocks;
    slot->blocks = block;
    slot->freeHead = block->entries;
    slot->capacity += count;
    return PHYSICS_POOL_OK;
}

PhysicsPools* physicsPoolsCreate(uint32_t vehicles,
                                 const size_t nativeBytes[PHYSICS_OBJECT_TYPE_COUNT],
                                 PhysicsPoolStatus* status)
{
    size_t strides[PHYSICS_OBJECT_TYPE_COUNT];
    size_t selectedBytes[PHYSICS_OBJECT_TYPE_COUNT];
    uint32_t capacities[PHYSICS_OBJECT_TYPE_COUNT];
    for (int i = 0; i < PHYSICS_OBJECT_TYPE_COUNT; ++i) {
        const PhysicsObjectInfo* info = &componentInfo[i];
        const size_t bytes = nativeBytes != NULL && nativeBytes[i] > info->originalBytes
            ? nativeBytes[i] : info->originalBytes;
        if (vehicles > (UINT32_MAX - info->permanentExtraInstances) / info->instancesPerVehicle ||
            bytes > SIZE_MAX - (info->allocationAlignment - 1)) {
            if (status != NULL) *status = PHYSICS_POOL_OVERFLOW;
            return NULL;
        }
        capacities[i] = vehicles * info->instancesPerVehicle + info->permanentExtraInstances;
        strides[i] = (bytes + info->allocationAlignment - 1) & ~(info->allocationAlignment - 1);
        if (capacities[i] != 0 && (strides[i] > INT_MAX / capacities[i] ||
                                  sizeof(PhysicsSlotEntry) > INT_MAX / capacities[i])) {
            if (status != NULL) *status = PHYSICS_POOL_OVERFLOW;
            return NULL;
        }
        selectedBytes[i] = bytes;
    }
    PhysicsPools* pools = static_cast<PhysicsPools*>(calloc(1, sizeof(*pools)));
    if (pools == NULL) {
        if (status != NULL) *status = PHYSICS_POOL_OUT_OF_MEMORY;
        return NULL;
    }
    for (int i = 0; i < PHYSICS_OBJECT_TYPE_COUNT; ++i) {
        PhysicsSlotPool* slot = &pools->slots[i];
        slot->nativeBytes = selectedBytes[i];
        slot->stride = strides[i];
        slot->initialCount = capacities[i];
        const PhysicsPoolStatus result = physicsPoolAddBlock(slot, capacities[i], componentInfo[i].name);
        if (result != PHYSICS_POOL_OK) {
            for (int j = 0; j <= i; ++j) physicsPoolFreeStorage(&pools->slots[j]);
            free(pools);
            if (status != NULL) *status = result;
            return NULL;
        }
    }
    if (status != NULL) *status = PHYSICS_POOL_OK;
    return pools;
}

PhysicsPoolStatus physicsPoolsClose(PhysicsPools* pools)
{
    if (pools == NULL) return PHYSICS_POOL_INVALID_ARGUMENT;
    for (int i = 0; i < PHYSICS_OBJECT_TYPE_COUNT; ++i)
        if (pools->slots[i].liveCount != 0) return PHYSICS_POOL_STILL_IN_USE;
    physicsPoolsDestroy(pools);
    return PHYSICS_POOL_OK;
}

void physicsPoolsDestroy(PhysicsPools* pools)
{
    if (pools != NULL) {
        for (int i = 0; i < PHYSICS_OBJECT_TYPE_COUNT; ++i) physicsPoolFreeStorage(&pools->slots[i]);
        free(pools);
    }
}

void* physicsPoolAcquire(PhysicsPools* pools, PhysicsObjectType type, PhysicsPoolStatus* status)
{
    if (pools == NULL || !physicsPoolValidType(type)) {
        if (status != NULL) *status = PHYSICS_POOL_INVALID_ARGUMENT;
        return NULL;
    }
    PhysicsSlotPool* slot = &pools->slots[type];
    if (slot->freeHead == NULL) {
        const PhysicsPoolStatus result = physicsPoolAddBlock(slot, (slot->initialCount >> 2) + 1,
                                                           componentInfo[type].name);
        if (result != PHYSICS_POOL_OK) {
            if (status != NULL) *status = result;
            return NULL;
        }
    }
    PhysicsSlotEntry* entry = slot->freeHead;
    slot->freeHead = entry->next;
    // Default source flags15 include the SlotPool::Malloc zeroing bit.
    // Apply it to the widened native slot on first use and on LIFO reuse.
    memset(entry->object, 0, slot->stride);
    entry->state = 1;
    ++slot->liveCount;
    if (slot->liveCount > slot->peakCount) slot->peakCount = slot->liveCount;
    if (status != NULL) *status = PHYSICS_POOL_OK;
    return entry->object;
}

PhysicsPoolStatus physicsPoolRelease(PhysicsPools* pools, PhysicsObjectType type, void* object)
{
    if (pools == NULL || !physicsPoolValidType(type) || object == NULL)
        return PHYSICS_POOL_INVALID_ARGUMENT;
    PhysicsSlotPool* slot = &pools->slots[type];
    const uintptr_t value = reinterpret_cast<uintptr_t>(object);
    for (PhysicsSlotBlock* block = slot->blocks; block != NULL; block = block->next) {
        const uintptr_t begin = reinterpret_cast<uintptr_t>(block->memory);
        if (value < begin) continue;
        const uintptr_t offset = value - begin;
        if (offset >= static_cast<size_t>(block->count) * slot->stride) continue;
        if (offset % slot->stride != 0) return PHYSICS_POOL_FOREIGN_POINTER;
        PhysicsSlotEntry* entry = &block->entries[offset / slot->stride];
        if (entry->state == 0) return PHYSICS_POOL_FOREIGN_POINTER;
        if (entry->state == 2) return PHYSICS_POOL_DOUBLE_RELEASE;
        entry->state = 2;
        entry->next = slot->freeHead;
        slot->freeHead = entry;
        --slot->liveCount;
        return PHYSICS_POOL_OK;
    }
    return PHYSICS_POOL_FOREIGN_POINTER;
}

PhysicsPoolStatus physicsPoolGetUsage(const PhysicsPools* pools, PhysicsObjectType type, PhysicsPoolUsage* out)
{
    if (pools == NULL || !physicsPoolValidType(type) || out == NULL)
        return PHYSICS_POOL_INVALID_ARGUMENT;
    const PhysicsSlotPool* slot = &pools->slots[type];
    out->capacity = slot->capacity;
    out->allocated = slot->liveCount;
    out->peakAllocated = slot->peakCount;
    out->nativeObjectBytes = slot->nativeBytes;
    out->slotStride = slot->stride;
    return PHYSICS_POOL_OK;
}


PhysicsPools* physicsObjectsCreatePools(uint32_t vehicleCapacity, PhysicsPoolStatus* status)
{
    const size_t nativeBytes[PHYSICS_OBJECT_TYPE_COUNT] = {
        sizeof(RigidBody), sizeof(Steering), sizeof(Wheel), sizeof(Engine),
        sizeof(DriveTrain), sizeof(Aerodynamics), sizeof(Suspension),
        sizeof(Mellovator), sizeof(MellowMover), sizeof(NullMover)
    };
    return physicsPoolsCreate(vehicleCapacity, nativeBytes, status);
}

PhysicsPoolStatus physicsObjectsClosePools(PhysicsPools* pools)
{
    return physicsPoolsClose(pools);
}

RigidBody* physicsObjectsAcquireRigidBody(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (RigidBody*)physicsPoolAcquire(pools, PHYSICS_OBJECT_RIGID_BODY, status); }
PhysicsPoolStatus physicsObjectsReleaseRigidBody(PhysicsPools* pools, RigidBody* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_RIGID_BODY, object); }
Steering* physicsObjectsAcquireSteering(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (Steering*)physicsPoolAcquire(pools, PHYSICS_OBJECT_STEERING, status); }
PhysicsPoolStatus physicsObjectsReleaseSteering(PhysicsPools* pools, Steering* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_STEERING, object); }
Wheel* physicsObjectsAcquireWheel(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (Wheel*)physicsPoolAcquire(pools, PHYSICS_OBJECT_WHEEL, status); }
PhysicsPoolStatus physicsObjectsReleaseWheel(PhysicsPools* pools, Wheel* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_WHEEL, object); }
Engine* physicsObjectsAcquireEngine(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (Engine*)physicsPoolAcquire(pools, PHYSICS_OBJECT_ENGINE, status); }
PhysicsPoolStatus physicsObjectsReleaseEngine(PhysicsPools* pools, Engine* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_ENGINE, object); }
DriveTrain* physicsObjectsAcquireDriveTrain(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (DriveTrain*)physicsPoolAcquire(pools, PHYSICS_OBJECT_DRIVE_TRAIN, status); }
PhysicsPoolStatus physicsObjectsReleaseDriveTrain(PhysicsPools* pools, DriveTrain* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_DRIVE_TRAIN, object); }
Aerodynamics* physicsObjectsAcquireAerodynamics(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (Aerodynamics*)physicsPoolAcquire(pools, PHYSICS_OBJECT_AERODYNAMICS, status); }

Suspension* physicsObjectsAcquireSuspension(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (Suspension*)physicsPoolAcquire(pools, PHYSICS_OBJECT_SUSPENSION, status); }
PhysicsPoolStatus physicsObjectsReleaseSuspension(PhysicsPools* pools, Suspension* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_SUSPENSION, object); }
Mellovator* physicsObjectsAcquireMellovator(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (Mellovator*)physicsPoolAcquire(pools, PHYSICS_OBJECT_MELLOVATOR, status); }
PhysicsPoolStatus physicsObjectsReleaseMellovator(PhysicsPools* pools, Mellovator* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_MELLOVATOR, object); }
MellowMover* physicsObjectsAcquireMellowMover(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (MellowMover*)physicsPoolAcquire(pools, PHYSICS_OBJECT_MELLOW_MOVER, status); }
PhysicsPoolStatus physicsObjectsReleaseMellowMover(PhysicsPools* pools, MellowMover* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_MELLOW_MOVER, object); }
NullMover* physicsObjectsAcquireNullMover(PhysicsPools* pools, PhysicsPoolStatus* status)
{ return (NullMover*)physicsPoolAcquire(pools, PHYSICS_OBJECT_NULL_MOVER, status); }
PhysicsPoolStatus physicsObjectsReleaseNullMover(PhysicsPools* pools, NullMover* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_NULL_MOVER, object); }

static int requireArgument(const void* argument, PhysicsPoolStatus* status)
{
    if (argument != NULL) return 1;
    if (status != NULL) *status = PHYSICS_POOL_INVALID_ARGUMENT;
    return 0;
}

RigidBody* physicsObjectsCreateRigidBody(PhysicsPools* pools, const RigidBodyInfo* info,
                                        const RigidBodyTuning* tuning, PhysicsPoolStatus* status)
{
    if (!requireArgument(info, status) || !requireArgument(tuning, status)) return NULL;
    RigidBody* body = physicsObjectsAcquireRigidBody(pools, status);
    if (body != NULL) *body = RigidBody{};
    return body != NULL ? rigidBodyInitialize(body, info, tuning) : NULL;
}

Engine* physicsObjectsCreateEngine(PhysicsPools* pools, const EngineInfo* info,
                                   const EngineVehicleState* vehicle, const EngineTuning* tuning,
                                   PhysicsPoolStatus* status)
{
    if (!requireArgument(info, status) || !requireArgument(vehicle, status) ||
        !requireArgument(tuning, status)) return NULL;
    Engine* engine = physicsObjectsAcquireEngine(pools, status);
    if (engine != NULL) *engine = Engine{};
    return engine != NULL ? engineInitialize(engine, info, vehicle, tuning) : NULL;
}

DriveTrain* physicsObjectsCreateDriveTrain(PhysicsPools* pools, const DriveTrainInfo* info,
                                          Engine* engine, Wheel* const wheels[4],
                                          DriveTrainVehicleState* vehicle,
                                          const DriveTrainTuning* tuning, PhysicsPoolStatus* status)
{
    if (!requireArgument(info, status) || !requireArgument(engine, status) ||
        !requireArgument(wheels, status) || !requireArgument(vehicle, status) ||
        !requireArgument(tuning, status)) return NULL;
    for (int i = 0; i < 4; ++i) if (!requireArgument(wheels[i], status)) return NULL;
    DriveTrain* driveTrain = physicsObjectsAcquireDriveTrain(pools, status);
    if (driveTrain != NULL) *driveTrain = DriveTrain{};
    return driveTrain != NULL ? driveTrainInitialize(driveTrain, info, engine, wheels, vehicle, tuning) : NULL;
}

Steering* physicsObjectsCreateSteering(PhysicsPools* pools, const SteeringGeometry* geometry,
                                      PhysicsPoolStatus* status)
{
    if (!requireArgument(geometry, status)) return NULL;
    Steering* steering = physicsObjectsAcquireSteering(pools, status);
    if (steering != NULL) *steering = Steering{};
    if (steering != NULL) steeringInitialize(steering, geometry);
    return steering;
}

Aerodynamics* physicsObjectsCreateAerodynamics(PhysicsPools* pools, const AerodynamicsInfo* info,
                                               RigidBody* body, const AerodynamicsTuning* tuning,
                                               PhysicsPoolStatus* status)
{
    if (!requireArgument(info, status) || !requireArgument(body, status) ||
        !requireArgument(tuning, status)) return NULL;
    Aerodynamics* aerodynamic = physicsObjectsAcquireAerodynamics(pools, status);
    if (aerodynamic != NULL) *aerodynamic = Aerodynamics{};
    if (aerodynamic != NULL) aerodynamicsInitialize(aerodynamic, info, body, tuning);
    return aerodynamic;
}


void physicsCarSnapshot(PhysicsCar* car,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"PhysicsCar");
    replaySnapshotMoveFloat(snapshot,&car->gripScale);
    rigidBodySnapshot(car->body,snapshot);
    steeringDoSnapshot(car->steering,snapshot);
    for(int wheel=0;wheel<4;++wheel) {
        wheelSerializePersistentState(car->wheels[wheel],snapshot);
        suspensionSerializePersistentState(car->suspension[wheel],snapshot);
    }
    engineSnapshot(car->engine,snapshot);
    driveTrainSnapshot(car->driveTrain,snapshot);
    aerodynamicsResetTransientState(car->aerodynamics,snapshot);
    replaySnapshotMoveFloat(snapshot,&car->brakeRecovery);
    replaySnapshotMoveData(snapshot,&car->momentumHoldTicks,4,-1,0);
    replaySnapshotMoveData(snapshot,&car->momentumHoldStart,4,-1,0);
    replaySnapshotMoveData(snapshot,&car->momentumHold,1,4,0);
    replaySnapshotMoveData(snapshot,&car->momentumHoldArmed,1,4,0);
    replaySnapshotMoveInteger(snapshot,&car->momentumHoldStarts,4);
    replaySnapshotMoveData(snapshot,&car->uprightRecoveryStart,4,-1,0);
    replaySnapshotEndSection(snapshot);
}

const PhysicsCarTuning physicsCarDefaultTuning = {
    0.8500000238418579f, 0.004399999976158142f,
    1.5f, 3.0f, 1.0f, 15.0f, 0.4000000059604645f, 1.0f,
    0.1666666716337204f, 0.1963493824005127f, 0.30000001192092896f,
    6.0f, 1, 0
};

static float dotThree(const float first[3], const float second[3])
{
    const float x = first[0] * second[0];
    const float y = first[1] * second[1];
    const float z = first[2] * second[2];
    const float xy = x + y;
    return xy + z;
}

static void toBodyFrame(float output[3], const float orientation[16], const float value[3])
{
    // PAL138FBC uses the original transpose and VU point-transform sequence.
    float inverse[16], transformed[4];
    motionMatrixTransposeRotation(inverse, orientation);
    vectorMatrixTransformPoint(transformed, inverse, value);
    for (int axis = 0; axis < 3; ++axis) output[axis] = transformed[axis];
}

static int32_t elapsedTicks(int32_t now, int32_t start)
{
    const uint32_t elapsed = (uint32_t)now - (uint32_t)start;
    int32_t result;
    memcpy(&result, &elapsed, sizeof(result));
    return result;
}

static void refreshComponentViews(PhysicsCar* car)
{
    car->engineView.brakingMode = car->state->pursuitMode;
    car->driveTrainView.speedMetersPerSecond = car->state->speed;
    car->driveTrainView.differentialMode = car->state->tractionAssistPhase;
    car->wheelView.speed = car->state->forwardSpeed;
    car->wheelView.pursuitMode = car->state->pursuitMode;
    for (int i = 0; i < 4; ++i) {
        car->wheelSteps[i].elapsed = car->environment->stepContext->lastSubstepElapsed;
        car->wheelSteps[i].rollingResistance =
            car->environment->contact.materials[car->wheels[i]->materialIndex].rollingResistance;
    }
}

static int32_t suspensionSize(void* object)
{ return suspensionStateSize((Suspension*)object); }
static void saveSuspension(void* object, float* output)
{
    const Suspension* suspension = (const Suspension*)object;
    output[0] = suspension->motion.travel;
    output[1] = suspension->motion.travelSpeed;
}
static void restoreSuspension(void* object, const float* input)
{
    const SuspensionState state = {input[0], input[1]};
    suspensionRestoreState((Suspension*)object, &state);
}
static void suspensionDerivatives(void* object, const void* context, float* output)
{
    SuspensionState derivatives;
    (void)context;
    suspensionComputeDerivatives((Suspension*)object, &derivatives);
    output[0] = derivatives.travel;
    output[1] = derivatives.travelSpeed;
}
static void prepareSuspension(void* object, const void* context)
{
    const WheelContactContext contact=physicsCarContactContext((const PhysicsCarEnvironment*)context);
    suspensionPrepareContact((Suspension*)object,&contact);
}
static void applySuspensionDelta(void* object, const float* derivatives, float elapsed)
{
    Suspension* suspension = (Suspension*)object;
    const float travel = bScalarMultiply(elapsed, derivatives[0]);
    const float speed = bScalarMultiply(elapsed, derivatives[1]);
    suspension->motion.travel = bScalarAdd(suspension->motion.travel, travel);
    suspension->motion.travelSpeed = bScalarAdd(suspension->motion.travelSpeed, speed);
}
static const ComponentOperations suspensionOperations = {
    suspensionSize, saveSuspension, restoreSuspension, suspensionDerivatives,
    prepareSuspension, applySuspensionDelta
};

static void prepareIntegratedCar(void* object)
{ physicsCarPrepare((PhysicsCar*)object); }
static void moveCar(void* object, float elapsed)
{ (void)physicsCarMove((PhysicsCar*)object, elapsed); }

WheelContactContext physicsCarContactContext(const PhysicsCarEnvironment* environment)
{
    const WheelContactContext contact={environment->contact.terrain,environment->contact.materials,
        environment->contact.materialCount,environment->stepContext->lastSubstepElapsed,environment->contact.tuning};
    return contact;
}

PhysicsPoolStatus physicsCarInitialize(PhysicsCar* car, const PhysicsCarInfo* info,
                                       CarControls* controls, Car* owner,
                                       CarMotionState* ownerMotion, VehicleState* state,
                                       PhysicsCarEnvironment* environment, PhysicsPools* pools)
{
    if (car == NULL || info == NULL || controls == NULL || owner == NULL ||
        ownerMotion == NULL || ownerMotion->collision == NULL || ownerMotion->collision->shape == NULL ||
        ownerMotion->collision->policy == NULL || state == NULL ||
        environment == NULL || environment->clock == NULL || environment->tuning == NULL || environment->settings == NULL ||
        environment->stepContext == NULL ||
        environment->contact.terrain == NULL || environment->contact.materials == NULL ||
        environment->contact.materialCount == 0 || environment->contact.tuning == NULL ||
        environment->raceStage == NULL ||
        pools == NULL || info->body == NULL || info->centerOfMass == NULL ||
        info->steering == NULL || info->engine == NULL || info->driveTrain == NULL ||
        info->aerodynamics == NULL || info->wheels == NULL || info->tires == NULL ||
        info->brakes == NULL || info->suspension == NULL)
        return PHYSICS_POOL_INVALID_ARGUMENT;
    // Traffic cars deliberately have no CarDamage owner. The original
    // PhysicsCar accepts them and Move checks that optional owner before
    // asking BodyDamage for the enabled damage amount.
    memset(car, 0, sizeof(*car));
    car->info = *info;
    car->controls = controls;
    car->owner = owner;
    car->ownerMotion = ownerMotion;
    car->state = state;
    car->environment = environment;
    car->pools = pools;
    wheelTrailStoreInitialize(&car->localTrailStore, car->localTrails, 16);
    state->position = owner->position;
    state->centerOfMass = info->centerOfMass;
    state->wheelInfo = info->wheels;
    memcpy(state->wheelSideOffset, info->wheelSideOffset, sizeof(state->wheelSideOffset));
    componentSystemInitialize(&car->components, car->bindings, 11);
    car->components.stageOwner = car;
    car->components.prepareStage = prepareIntegratedCar;
    car->gripScale = 1.0f;
    PhysicsPoolStatus status = PHYSICS_POOL_OK;
    car->body = physicsObjectsCreateRigidBody(pools, info->body, &rigidBodyDefaultTuning, &status);
    if (car->body == NULL) goto failed;
    rigidBodySetForwardSpeedLimit(car->body, info->forwardSpeedLimit);
    rigidBodySetPursuitSpeedEnabled(car->body, info->pursuitConfiguration != 0);
    componentSystemAdd(&car->components, componentBindRigidBody(car->body));
    car->steering = physicsObjectsCreateSteering(pools, info->steering, &status);
    if (car->steering == NULL) goto failed;
    for (int i = 0; i < 4; ++i) {
        car->wheels[i] = physicsObjectsAcquireWheel(pools, &status);
        if (car->wheels[i] == NULL) goto failed;
        memset(car->wheels[i], 0, sizeof(*car->wheels[i]));
        wheelInitialize(car->wheels[i], info->wheels + i, info->tires + i / 2,
                        info->brakes, &car->wheelView);
        car->suspension[i] = physicsObjectsAcquireSuspension(pools, &status);
        if (car->suspension[i] == NULL) goto failed;
        memset(car->suspension[i], 0, sizeof(*car->suspension[i]));
        suspensionInitialize(car->suspension[i], i, car->body, car->wheels[i],
                              i < 2 ? car->steering : NULL, info->suspension + i / 2);
        componentSystemAdd(&car->components, componentBindWheel(car->wheels[i], car->wheelSteps + i));
        const ComponentBinding binding = {
            car->suspension[i], environment, &suspensionOperations
        };
        componentSystemAdd(&car->components, binding);
    }
    for (int i = 0; i < 4; ++i)
        suspensionSetPairedSuspension(car->suspension[i], car->suspension[i ^ 1]);
    car->engine = physicsObjectsCreateEngine(pools, info->engine, &car->engineView,
                                             &engineDefaultTuning, &status);
    if (car->engine == NULL) goto failed;
    engineSetHotPursuit(car->engine, info->pursuitConfiguration != 0);
    componentSystemAdd(&car->components, componentBindEngine(car->engine));
    car->driveTrain = physicsObjectsCreateDriveTrain(pools, info->driveTrain, car->engine,
        car->wheels, &car->driveTrainView, &driveTrainDefaultTuning, &status);
    if (car->driveTrain == NULL) goto failed;
    componentSystemAdd(&car->components, componentBindDriveTrain(car->driveTrain));
    car->aerodynamics = physicsObjectsCreateAerodynamics(pools, info->aerodynamics, car->body,
        &aerodynamicsDefaultTuning, &status);
    if (car->aerodynamics == NULL) goto failed;
    environment->stepContext->lastSubstepElapsed = 0.0f;
    refreshComponentViews(car);
    physicsCarPrepareAxleLoads(car);
    physicsCarLoadState(car);
    physicsCarPrepare(car);
    car->brakeRecovery = 1.0f;
    car->momentumHold = car->momentumHoldArmed = 1;
    car->momentumHoldStarts = 2;
    owner->movement = car;
    owner->move = moveCar;
    return PHYSICS_POOL_OK;
failed:
    physicsCarDispose(car);
    return status;
}

void physicsCarDispose(PhysicsCar* car)
{
    if (car == NULL || car->pools == NULL) return;
    WheelTrailStore* trails = car->environment->wheelTrails != NULL ?
        car->environment->wheelTrails : &car->localTrailStore;
    for (int i = 0; i < 4; ++i) wheelTrailDetach(trails, car->wheelTrails + i);
    PhysicsPools* pools = car->pools;
    if (car->steering != NULL) physicsObjectsReleaseSteering(pools, car->steering);
    if (car->aerodynamics != NULL) physicsObjectsReleaseAerodynamics(pools, car->aerodynamics);
    if (car->body != NULL) physicsObjectsReleaseRigidBody(pools, car->body);
    for (int i = 0; i < 4; ++i) {
        if (car->wheels[i] != NULL) physicsObjectsReleaseWheel(pools, car->wheels[i]);
        if (car->suspension[i] != NULL) physicsObjectsReleaseSuspension(pools, car->suspension[i]);
    }
    if (car->engine != NULL) physicsObjectsReleaseEngine(pools, car->engine);
    if (car->driveTrain != NULL) physicsObjectsReleaseDriveTrain(pools, car->driveTrain);
    if (car->owner != NULL && car->owner->movement == car) {
        car->owner->movement = NULL;
        car->owner->move = NULL;
    }
    memset(car, 0, sizeof(*car));
}

void physicsCarPrepareAxleLoads(PhysicsCar* car)
{
    rigidBodySetCenterOfMass(car->body, car->info.centerOfMass);
    rigidBodyRefreshMass(car->body);
    const float center = car->environment->tuning->useCenterOfMassForLoad ? car->info.centerOfMass[0] : 0.0f;
    const float front = bScalarAdd(car->info.steering->frontAxleX, -center);
    const float rear = bScalarAdd(car->info.steering->rearAxleX, -center);
    const float halfRatio = bScalarMultiply(bScalarDivide(front, bScalarAdd(front, -rear)), 0.5f);
    const float frontRatio = bScalarAdd(0.5f, -halfRatio);
    const float downforce = aerodynamicsEstimateDownforce(car->aerodynamics, car->state->forwardSpeed);
    const float gravityLoad = bScalarMultiply(car->body->mass, 9.800000190734863f);
    const float aerodynamicLoad = bScalarMultiply(car->info.preloadDownforceScale, downforce);
    const float load = bScalarAdd(gravityLoad, aerodynamicLoad);
    const float frontLoad = bScalarMultiply(load, frontRatio);
    const float rearLoad = bScalarMultiply(load, halfRatio);
    suspensionInitializeMount(car->suspension[0], frontLoad);
    suspensionInitializeMount(car->suspension[1], frontLoad);
    suspensionInitializeMount(car->suspension[2], rearLoad);
    suspensionInitializeMount(car->suspension[3], rearLoad);
    for (int i = 0; i < 4; ++i)
        wheelInitializeRotationalInertia(car->wheels[i], car->environment->contact.tuning);
}

void physicsCarLoadState(PhysicsCar* car)
{
    VehicleState* state = car->state;
    rigidBodySetMotion(car->body, state->position, state->orientation,
                        state->velocity, state->angularVelocity);
    for (int i = 0; i < 4; ++i) {
        suspensionResetMotion(car->suspension[i]);
        wheelSetMotion(car->wheels[i], state->wheelAngle[i], state->wheelAngularVelocity[i]);
    }
    car->engine->angularVelocity = state->engineRpm * 0.10471967607736588f;
    car->driveTrain->desiredGear = car->driveTrain->currentGear = state->gear;
    car->driveTrain->clutchState = 3;
    car->driveTrain->clutch = 1.0f;
    car->driveTrain->previousSlipRpm = 0.0f;
}

void physicsCarPrepare(PhysicsCar* car)
{
    componentSystemPrepare(&car->components);
    refreshComponentViews(car);
    physicsCarResolveSuspensionImpulses(car);
    physicsCarCollectForces(car);
}

int32_t physicsCarRestState(const PhysicsCar* car)
{
    const RigidBody* body = car->body;
    const VehicleState* state = car->state;
    if (0.30000001192092896f < car->controls->throttle && state->activeContactCount > 0)
        return 0;
    const float linearSpeed = sqrtf(dotThree(body->linearVelocity, body->linearVelocity));
    const float angularSpeed = sqrtf(dotThree(body->angularVelocity, body->angularVelocity));
    if (0.699999988079071f < linearSpeed || 0.699999988079071f < angularSpeed ||
        0.25f < fabsf(body->motion.orientation[2]) ||
        0.25f < fabsf(body->motion.orientation[6]) ||
        carMotionWindowActive(car->ownerMotion, car->environment->clock->worldTimer))
        return 0;
    if (state->activeContactCount == 0 && !car->ownerMotion->collision->shape->contactCount)
        return 0;
    return 0.25f < linearSpeed || 0.25f < angularSpeed ? 1 : 2;
}

void physicsCarResolveSuspensionImpulses(PhysicsCar* car)
{
    const WheelContactContext contact=physicsCarContactContext(car->environment);
    int changed = 0;
    RigidBody* body = car->body;
    for (int i = 0; i < 4; ++i) {
        if (suspensionAtBumpStop(car->suspension[i])) {
            changed = 1;
            const float impulse = suspensionResolveContactImpulse(car->suspension[i],&contact);
            car->state->suspensionImpulse[i] = car->state->suspensionImpulse[i] + impulse;
        }
    }
    const int32_t rest = physicsCarRestState(car);
    if (rest == 0 || rest == 1) {
        if (rest == 1) {
            for (int axis = 0; axis < 4; ++axis) {
                body->motion.linearMomentum[axis] = body->motion.linearMomentum[axis] * 0.949999988079071f;
                body->motion.angularMomentum[axis] = body->motion.angularMomentum[axis] * 0.949999988079071f;
            }
            for (int i = 0; i < 4; ++i)
                wheelSetAngularVelocity(car->wheels[i],
                    car->wheels[i]->motion.angularVelocity * 0.949999988079071f);
            changed = 1;
        }
        if (0.800000011920929f < sqrtf(dotThree(body->linearVelocity, body->linearVelocity)))
            car->uprightRecoveryStart = 0;
        car->ownerMotion->collision->shape->dirty = 1;
    } else if (rest == 2) {
        /* The console scratch vector had an unused fourth lane. Native momenta
         * retain that lane while the three physical axes are brought to rest. */
        for (int axis = 0; axis < 3; ++axis) {
            body->motion.linearMomentum[axis] = 0.0f;
            body->motion.angularMomentum[axis] = 0.0f;
        }
        if (car->state->activeContactCount > 0)
            for (int i = 0; i < 4; ++i) wheelSetAngularVelocity(car->wheels[i], 0.0f);
        changed = 1;
        if (body->motion.orientation[10] < 0.10000000149011612f) {
            const int32_t start = car->uprightRecoveryStart;
            const int32_t now = car->environment->clock->worldTimer;
            if (start != 0 && start != INT32_MAX) {
                const float elapsed = (float)elapsedTicks(now, start) * 0.0002500000118743628f;
                if (1.0f < elapsed)
                    carMotionRequestRecovery(car->ownerMotion, &car->environment->recovery);
            } else {
                car->uprightRecoveryStart = now;
            }
        } else {
            car->uprightRecoveryStart = 0;
        }
        car->ownerMotion->collision->shape->dirty = 0;
    }
    if (car->environment->requestUpwardImpulse && car->ownerMotion->inputCategory == 1) {
        const float impulse[4] = {0.0f, 0.0f, 25.0f, 0.0f};
        const float lever[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        rigidBodyApplyImpulse(body, impulse, lever);
        car->environment->requestUpwardImpulse = 0;
        changed = 1;
    }
    if (changed) {
        car->environment->stepContext->lastSubstepElapsed = 0.0f;
        refreshComponentViews(car);
        componentSystemPrepare(&car->components);
    }
}

void physicsCarCollectWheelForces(PhysicsCar* car)
{
    const WheelContactContext contact=physicsCarContactContext(car->environment);
    driveTrainResolveForces(car->driveTrain);
    for (int i = 0; i < 4; ++i)
        wheelComputeContactForces(car->wheels[i], &contact,
                                   car->ownerMotion->collision->shape->contactCount);
    float baseForce[3];
    toBodyFrame(baseForce, car->body->motion.orientation, car->body->force);
    for (int pass = 0; pass < 5; ++pass) {
        float force[3] = {baseForce[0], baseForce[1], baseForce[2]};
        float yawTorque = 0.0f;
        for (int i = 0; i < 4; ++i) {
            float reaction[3];
            wheelCarReactionForce(car->wheels[i], reaction);
            for (int axis = 0; axis < 3; ++axis) force[axis] = bScalarAdd(force[axis], reaction[axis]);
            const float* lever = car->suspension[i]->mountOffset;
            const float first = bScalarMultiply(lever[0], reaction[1]);
            const float second = bScalarMultiply(lever[1], reaction[0]);
            yawTorque = bScalarAdd(yawTorque, first);
            yawTorque = bScalarAdd(yawTorque, -second);
        }
        const float accelerationX = bScalarDivide(force[0], car->body->mass);
        const float accelerationY = bScalarDivide(force[1], car->body->mass);
        const float yawAcceleration = bScalarDivide(yawTorque, rigidBodyYawInertia(car->body));
        for (int i = 0; i < 4; ++i) {
            Wheel* wheel = car->wheels[i];
            if (!wheel->contactCorrectionDisabled && !wheel->rotationLocked) {
                const float* lever = car->suspension[i]->mountOffset;
                const float acceleration[2] = {
                    bScalarAdd(accelerationX, -bScalarMultiply(yawAcceleration, lever[1])),
                    bScalarAdd(accelerationY, bScalarMultiply(yawAcceleration, lever[0]))
                };
                wheelCorrectReactionTorque(wheel, acceleration);
            }
        }
        driveTrainBalanceAcceleration(car->driveTrain);
    }
    for (int i = 0; i < 4; ++i)
        wheelResolveContactForce(car->wheels[i], &contact);
}

void physicsCarCollectForces(PhysicsCar* car)
{
    float downforce;
    aerodynamicsApplyForces(car->aerodynamics, &downforce);
    const float load = car->environment->tuning->useDownforceForTireLoad ? bScalarMultiply(downforce, 0.25f) : 0.0f;
    for (int i = 0; i < 4; ++i) suspensionSetContactLoad(car->suspension[i], load);
    physicsCarCollectWheelForces(car);
    const WheelContactContext contact=physicsCarContactContext(car->environment);
    for (int i = 0; i < 4; ++i)
        suspensionApplyContactForce(car->suspension[i], &contact);
    physicsCarDampAirborneRotation(car);
}

void physicsCarDampAirborneRotation(PhysicsCar* car)
{
    static const CurveKey keys[3] = {{0.0f, 0.0f}, {0.4000000059604645f, 0.0f}, {2.0f, 20.0f}};
    const KeyedCurve curve = {keys, 3};
    if (car->state->activeContactCount < 2) {
        const float* angular = car->body->angularVelocity;
        const float speed = sqrtf(dotThree(angular, angular));
        const float scale = car->body->mass * keyedCurveEvaluate(&curve, speed);
        float torque[4];
        for (int axis = 0; axis < 4; ++axis) torque[axis] = -scale * angular[axis];
        rigidBodyAddTorque(car->body, torque);
    }
}

void physicsCarSetGripScale(PhysicsCar* car, float scale)
{
    float engineScale = scale;
    if (car->ownerMotion->disabledWheelMask && car->ownerMotion->collision->shape->contactCount)
        engineScale = scale * 0.4000000059604645f;
    car->engine->catchupMultiplier = engineScale;
    rigidBodySetSpeedLimitScale(car->body, scale);
    car->aerodynamics->speedScale = scale;
    car->aerodynamics->dragScale = 1.0f - car->owner->interactionFactor;
    for (int i = 0; i < 4; ++i) wheelSetBaseForceMultiplier(car->wheels[i], scale);
}

void physicsCarPublishState(PhysicsCar* car)
{
    VehicleState* state = car->state;
    const RigidBody* body = car->body;
    memcpy(state->position, body->motion.position, sizeof(body->motion.position));
    memcpy(state->orientation, body->motion.orientation, sizeof(state->orientation));
    vehicleStateUpdateCenterPosition(state);
    memcpy(state->velocity, body->linearVelocity, sizeof(state->velocity));
    state->speed = sqrtf(dotThree(state->velocity, state->velocity));
    state->velocityFrameValid = 0;
    memcpy(state->angularVelocity, body->angularVelocity, sizeof(state->angularVelocity));
    state->heading = vectorFixedAtan2Angle((int32_t)(state->orientation[0] * 65536.0f),
                                          (int32_t)(state->orientation[1] * 65536.0f));
    state->steeringAngle = car->controls->steeringAngle;
    state->forwardSpeed = dotThree(state->orientation, state->velocity);
    for (int i = 0; i < 4; ++i) {
        const Wheel* wheel = car->wheels[i];
        const Suspension* suspension = car->suspension[i];
        state->wheelAngularVelocity[i] = wheel->motion.angularVelocity;
        state->wheelAngle[i] = wheel->motion.angle;
        float travel = suspension->motion.travel;
        if (suspension->info->extensionLimit <= travel)
            travel = travel + (suspension->info->extensionLimit - travel);
        state->suspensionTravel[i] = travel;
        vehicleStateSetWheelContact(state, i, wheel->contactPresent);
        vehicleStateSetWheelMaterial(state, i, wheel->materialIndex);
        state->wheelSlipRatio[i] = wheel->slipRatio;
        memcpy(state->wheelSlip[i], wheel->slip, sizeof(wheel->slip));
        state->wheelNormalLoad[i] = wheel->tractionLevel;
        state->wheelSurfaceOffset[i] = wheel->contactPresent ? wheel->surfaceOffset : 0.0f;
    }
    state->effectiveThrottle = car->engine->throttle * car->engine->throttleOverride;
    state->gear = car->driveTrain->currentGear;
    state->clutchEngaged = car->driveTrain->clutchState == 3;
    state->engineRpm = car->engine->angularVelocity * 9.549304008483887f;
    state->leftSlip = physicsMaximum(fabsf(car->wheels[0]->angularSpeedEstimate),
                                     fabsf(car->wheels[3]->angularSpeedEstimate)) /
                      car->environment->tuning->slipDisplayScale;
    state->rightSlip = physicsMaximum(fabsf(car->wheels[1]->angularSpeedEstimate),
                                      fabsf(car->wheels[2]->angularSpeedEstimate)) /
                       car->environment->tuning->slipDisplayScale;
    if (1.0f < state->leftSlip) state->leftSlip = 1.0f;
    if (1.0f < state->rightSlip) state->rightSlip = 1.0f;
    state->averageSlipRatio = ((state->wheelSlipRatio[0] + state->wheelSlipRatio[1]) +
                              state->wheelSlipRatio[2]) + state->wheelSlipRatio[3];
    state->averageSlipRatio = state->averageSlipRatio * 0.25f;
    vehicleStateUpdateWheelPoses(state);
}

static void updateTractionAssist(PhysicsCar* car, float speedMph)
{
    VehicleState* state = car->state;
    const PhysicsCarTuning* tuning = car->environment->tuning;
    const float elapsed = car->environment->clock->worldTimeElapsed;
    if (car->ownerMotion->inputCategory != CAR_DRIVER_REAL) return;
    // PAL137FE4 reads the current RealDriver port (+234 on the original
    // controls owner), then11D4F8 classifies that port's current device.
    // The native driver owns/restores the same port. A shared environment
    // boolean cannot represent hotplug, two players or snapshot restore.
    const auto* driver=static_cast<const CarDriver*>(car->owner->controller);
    hp2::Require(driver && driver->kind==CAR_DRIVER_REAL && driver->owner.real,
        "Traction assist has no actual RealDriver owner");
    const auto& input=driver->owner.real->input;
    hp2::Require(input.movement && input.movement->bindings.owner==car->owner && input.inputRouter,
        "Traction assist input belongs to a different car or has no device owner");
    if (joystickIsTypeWheel(input.inputRouter,input.port)) return;
    if (state->tractionAssistPhase != 0) {
        state->tractionAssistTime = state->tractionAssistTime - elapsed;
        if (state->tractionAssistTime < 0.0f) {
            state->tractionAssistPhase = 0;
            return;
        }
        if (tuning->tractionReleaseHeadingError < fabsf(vehicleStateMovingHeadingDifference(state))) {
            state->tractionAssistPhase = 0;
            return;
        }
        for (int i = 0; i < 4; ++i) {
            if (!(car->wheels[i]->accumulatedDriveTorque < tuning->tractionReleaseSlip)) {
                state->tractionAssistDwell = 0.0f;
                return;
            }
        }
        state->tractionAssistDwell = state->tractionAssistDwell + elapsed;
        if (tuning->tractionDwellSeconds < state->tractionAssistDwell)
            state->tractionAssistPhase = 0;
        return;
    }
    if (state->pursuitMode != 1 || car->controls->steeringAngle != 0 ||
        !(0.0f < speedMph) || !(speedMph < tuning->tractionMaximumSpeed) ||
        !(fabsf(vehicleStateMovingHeadingDifference(state)) < tuning->tractionEntryHeadingError))
        return;
    const float frontLeft = car->wheels[0]->accumulatedDriveTorque;
    const float frontRight = car->wheels[1]->accumulatedDriveTorque;
    const float rearRight = car->wheels[2]->accumulatedDriveTorque;
    const float rearLeft = car->wheels[3]->accumulatedDriveTorque;
    int selected = -1;
    if (tuning->tractionEntrySlip < rearRight || tuning->tractionEntrySlip < rearLeft) {
        if (rearLeft < rearRight) {
            selected = 2;
            state->tractionAssistPhase = 1;
        } else {
            selected = 3;
            state->tractionAssistPhase = 2;
        }
    } else if (tuning->tractionEntrySlip < frontLeft || tuning->tractionEntrySlip < frontRight) {
        if (frontLeft < frontRight) {
            selected = 1;
            state->tractionAssistPhase = 1;
        } else {
            selected = 0;
            state->tractionAssistPhase = 2;
        }
    }
    if (selected < 0 || carMotionWindowActive(car->ownerMotion, car->environment->clock->worldTimer))
        return;
    float slipRatio = (car->wheels[selected]->accumulatedDriveTorque - tuning->tractionEntrySlip) /
                      (tuning->tractionFullSlip - tuning->tractionEntrySlip);
    if (1.0f < slipRatio) slipRatio = 1.0f;
    float minimum = tuning->tractionMinimumBrakeScale;
    const TerrainMaterial* materials = car->environment->contact.materials;
    if (materials[car->wheels[2]->materialIndex].staticGrip != 1.0f ||
        materials[car->wheels[3]->materialIndex].staticGrip != 1.0f)
        minimum = minimum * 1.25f;
    float scale = (speedMph / tuning->tractionMaximumSpeed) * (1.0f - slipRatio);
    scale = scale * (1.0f - minimum);
    state->tractionAssistTime = tuning->tractionHoldSeconds;
    state->tractionAssistScale = minimum + scale;
}

static void updateMomentumHold(PhysicsCar* car)
{
    const PhysicsCarEnvironment* environment = car->environment;
    // PAL138868..1389B0 reads the live Race stage. Stage4 bypasses the
    // starting-line hold without clearing its persistent countdown state.
    const int32_t raceStage = *environment->raceStage;
    if (raceStage == 4) return;
    if (raceStage >= 2) {
        const int32_t now = environment->clock->worldTimer;
        const float holdSeconds = (float)car->momentumHoldTicks * 0.0002500000118743628f;
        const float passedSeconds = (float)elapsedTicks(now, car->momentumHoldStart) * 0.0002500000118743628f;
        if (holdSeconds < passedSeconds) car->momentumHold = 1;
        if (0.5f < car->controls->throttle) {
            if (car->momentumHoldArmed && car->momentumHoldStarts != 0) {
                const float duration = vectorDefaultRandomFloat(0.15000000596046448f) + 0.4000000059604645f;
                car->momentumHoldTicks = convertFloatToInt32(duration * 4000.0f + 0.5f);
                car->momentumHold = car->momentumHoldArmed = 0;
                --car->momentumHoldStarts;
                const float seconds = (float)now * 0.0002500000118743628f;
                car->momentumHoldStart = convertFloatToInt32(seconds * 4000.0f + 0.5f);
            }
        } else {
            car->momentumHold = car->momentumHoldArmed = 1;
        }
    }
    if (car->momentumHold)
        for (int axis = 0; axis < 3; ++axis) car->body->motion.linearMomentum[axis] = 0.0f;
}

float physicsCarMove(PhysicsCar* car, float elapsed)
{
    CarControls* controls = car->controls;
    VehicleState* state = car->state;
    PhysicsCarEnvironment* environment = car->environment;
    const PhysicsCarTuning* tuning = environment->tuning;
    physicsCarSetGripScale(car, state->performanceScale);
    physicsCarPrepareAxleLoads(car);
    if (controls->brake < controls->throttle && controls->handbrake < controls->throttle) {
        if (controls->brake < 0.5f) controls->brake = 0.0f;
        if (controls->handbrake < 0.5f) controls->handbrake = 0.0f;
    } else if (controls->throttle < 0.5f) {
        controls->throttle = 0.0f;
    }
    steeringSetAngle(car->steering, controls->steeringAngle);
    engineSetThrottle(car->engine, controls->throttle);
    driveTrainSetShiftTarget(car->driveTrain, controls->shiftTarget);
    if (controls->nitroRequest) {
        engineSetNitroEnabled(car->engine, 1);
        engineSetNitroBoostTimer(car->engine, (float)environment->clock->worldTimer * 0.0002500000118743628f);
    }
    engineUpdateNitro(car->engine, environment->clock->worldTimer);
    controls->nitroRequest = 0;
    controls->gearRequest = driveTrainUpdateRequestedGear(car->driveTrain, controls->gearRequest,
                                                          environment->clock->worldTimeElapsed);
    controls->shiftTarget = car->driveTrain->shiftTarget;
    rigidBodySetDampingEnabled(car->body,
        !(0.30000001192092896f < controls->throttle ||
          (state->activeContactCount == 4 && 0.15000000596046448f < fabsf(state->orientation[2]))));
    float grip = car->gripScale;
    if (state->pursuitMode == 0) grip = grip * tuning->nonPursuitGripScale;
    memset(state->suspensionImpulse, 0, sizeof(state->suspensionImpulse));
    const float speedMph = state->forwardSpeed * 2.236999988555908f;
    updateTractionAssist(car, speedMph);
    float brakeRecovery = car->brakeRecovery;
    if (0.5f <= controls->handbrake) {
        brakeRecovery = brakeRecovery - elapsed * 20.0f;
        if (brakeRecovery < 0.5f) brakeRecovery = 0.5f;
    } else {
        brakeRecovery = brakeRecovery + elapsed;
        if (1.0f < brakeRecovery) brakeRecovery = 1.0f;
    }
    car->brakeRecovery = brakeRecovery;
    static const float serviceWeights[4] = {1.0f, 1.0f, 0.6000000238418579f, 0.6000000238418579f};
    static const float handbrakeWeights[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    for (int i = 0; i < 4; ++i) {
        Wheel* wheel = car->wheels[i];
        // PAL138380..138458: all wheels use service braking in stage4;
        // before/after racing only a held front axle receives that brake.
        if (*environment->raceStage == 4 || (car->momentumHold && i < 2)) {
            if (0.800000011920929f < controls->throttle && 0.5f < controls->brake &&
                speedMph < 10.0f && i >= 2)
                wheel->brakeTorque = speedMph * 0.05000000074505806f;
            else
                wheel->brakeTorque = controls->brake * serviceWeights[i];
        } else {
            wheel->brakeTorque = 0.0f;
        }
        float handbrake = controls->handbrake * handbrakeWeights[i];
        if (state->pursuitMode == 0) {
            if (0.20000000298023224f < handbrake) handbrake = handbrake + 0.5f;
        } else if (0.20000000298023224f < handbrake &&
                   0.30000001192092896f < fabsf(vehicleStateMovingHeadingDifference(state)) && speedMph < 80.0f) {
            handbrake = handbrake + 0.5f;
        }
        wheelSetHandbrakeTorque(wheel, handbrake);
        if (i == 2) {
            float magnitude = car->body->motion.orientation[2];
            const float headingError = vehicleStateMovingHeadingDifference(state);
            const float collisionBoost = carMotionPostCollisionYaw(car->ownerMotion, environment->clock->worldTimer);
            YawTuning yawTuning;
            physicsInitializeYawTuning(&yawTuning);
            const float boost = physicsYawFrictionBoost(&yawTuning, state->pursuitMode, &magnitude,
                headingError, car->wheels[2]->handbrakeTorque, state->speed, car->info.yawBaseCoefficient,
                car->info.yawCollisionCoefficient, collisionBoost);
            grip = grip * (1.0f + brakeRecovery * (boost - 1.0f));
        }
        wheelSetForceMultiplier(wheel, grip);
        if (((i == 0 || i == 3) && state->tractionAssistPhase == 1) ||
            ((i == 1 || i == 2) && state->tractionAssistPhase == 2))
            wheelSetForceMultiplier(wheel, grip * state->tractionAssistScale);
        wheelSetSteeringAngle(wheel, ((car->ownerMotion->disabledWheelMask >> i) & 1u) ? 1.0f : 0.0f);
    }
    if (car->owner->physicsBoost != 0.0f && !car->ownerMotion->disabledWheelMask)
        physicsCarSetGripScale(car, car->owner->physicsBoost);
    car->aerodynamics->dragReduction = controls->throttle;
    car->aerodynamics->downforceMode = (float)state->activeContactCount * 0.25f;
    float yawLimit = state->pursuitMode == 0 ? car->info.ordinaryYawLimit : car->info.pursuitYawLimit;
    if (state->pursuitMode != 0 && yawLimit == 0.0f) {
        const float headingError = vehicleStateMovingHeadingDifference(state);
        if (headingError * car->body->motion.angularMomentum[2] < 0.0f &&
            0.10000000149011612f < fabsf(headingError)) yawLimit = 20.0f;
    }
    rigidBodySetYawTorqueLimit(car->body, yawLimit);
    float damageScale = 1.0f;
    // PAL1386F4..138730 keeps the full speed limit when Car+848 is null.
    // BodyDamage::GetTotalDamage applies the live race damage option itself.
    if (car->ownerMotion->damage)
        damageScale = 1.0f - bodyDamageVisibleTotal(&car->ownerMotion->damage->body) * 0.029999999329447746f;
    if (car->engine->nitroEnabled) rigidBodySetForwardSpeedLimit(car->body, 0.0f);
    else if (car->ownerMotion->disabledWheelMask) rigidBodySetForwardSpeedLimit(car->body, 60.0f);
    else rigidBodySetForwardSpeedLimit(car->body, car->info.forwardSpeedLimit * damageScale);
    const int32_t steps = convertFloatToInt32(elapsed / tuning->maximumStep) + 1;
    const float step = elapsed / (float)steps;
    for (int32_t i = 0; i < steps; ++i) {
        environment->stepContext->lastSubstepElapsed = step;
        refreshComponentViews(car);
        componentSystemIntegrateEuler(&car->components, step, car->derivatives,
                                        sizeof(car->derivatives) / sizeof(car->derivatives[0]));
        motionMatrixOrthonormalize(car->body->motion.orientation, car->body->motion.orientation);
        // The integrator prepared omega before this normalization. Refresh its
        // angular cache without discarding the forces already collected.
        rigidBodyRefreshAngularVelocity(car->body);
        updateMomentumHold(car);
    }
    physicsCarPublishState(car);
    if (carMotionClassActionAllowed(car->ownerMotion)) {
        const WheelContactContext contact=physicsCarContactContext(environment);
        WheelTrailStore* trails = environment->wheelTrails != NULL ?
            environment->wheelTrails : &car->localTrailStore;
        for (int i = 0; i < 4; ++i)
            wheelEffectsUpdate(car->wheels[i], &contact, trails, car->wheelTrails + i,
                car->owner, state->centerPosition, car->ownerMotion->wheelEffectsFlags,
                car->ownerMotion->disabledWheelMask);
    }
    return elapsed;
}

PhysicsPoolStatus physicsObjectsReleaseAerodynamics(PhysicsPools* pools, Aerodynamics* object)
{ return physicsPoolRelease(pools, PHYSICS_OBJECT_AERODYNAMICS, object); }

void physicsInitializeYawTuning(YawTuning* tuning)
{
    tuning->lowSpeedYawBoost = 0.0f;
    tuning->highSpeedYawBoost = 1.0f;
    tuning->lowSpeedSpeed = 0.0f;
    tuning->highSpeedSpeed = 30.0f;
    tuning->maxYawBonus = 0.699999988079071f;
}

float physicsYawFrictionBoost(const YawTuning* tuning, int scaleDelta,
                              float* magnitude, float yaw, float handbrake,
                              float speed, float baseCoefficient,
                              float coefficientDelta, float coefficientScale)
{
    const float absolute = fabsf(*magnitude);
    *magnitude = absolute;
    const float initial = absolute + 1.0f;
    const float yawMagnitude = fabsf(yaw);

    if (scaleDelta)
        baseCoefficient += coefficientScale * coefficientDelta;
    else
        baseCoefficient += coefficientDelta;

    if (0.5f < handbrake && yawMagnitude < 0.349065899848938f)
        return initial;

    const float weightedYaw = yawMagnitude * baseCoefficient;
    const float speedRange = tuning->highSpeedSpeed - tuning->lowSpeedSpeed;
    const float ratio = (speed - tuning->lowSpeedSpeed) / speedRange;
    const float boostRange = tuning->highSpeedYawBoost - tuning->lowSpeedYawBoost;
    const float speedBoost = tuning->lowSpeedYawBoost + boostRange * ratio;
    float bonus = weightedYaw * speedBoost;
    if (tuning->maxYawBonus < bonus) bonus = tuning->maxYawBonus;
    return initial + bonus;
}
