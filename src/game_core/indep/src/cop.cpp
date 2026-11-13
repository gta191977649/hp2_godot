#include "indep/inc/Pursuit.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/CopDriver.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/Player.hpp"
#include "Chopper.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Collision.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/CarDamage.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/CarController.hpp"
#include "indep/inc/Race.hpp"

#include <math.h>
#include "bware/inc/bWare.hpp"
#include "indep/inc/CarInfo.hpp"
#include <memory>
#include "indep/inc/Replay.hpp"
#include "indep/inc/TrackRoute.hpp"

namespace hp2 {
// tAICop PAL22DB20..22DCC0. The real Car constructor must complete before
// the same shared body resource, controller and World links are initialized.
// The World list owns the Car; the six-slot Cop pool owns only this entry.
void PursuitConstructOwnedCop(PursuitManager& manager,PursuitCollisionEntry& entry,
    RaceDriverInfo& driver,int32_t officerIndex)
{
    Require(manager.world && manager.collision,"AICop has no World/CopManager owner");
    const float origin[3]={0.0f,0.0f,0.0f};
    std::unique_ptr<Car> car(WorldCreateCar(*manager.world,driver,origin,0,-1.0f));
    auto* resources=const_cast<CarInfoResource*>(WorldCarBindings(*manager.world).render->cars);
    auto* sharedBody=resources->MutableRigidBodyInfo(static_cast<size_t>(driver.appearance.modelCode));
    try {
        Require(pursuitOwnedEntryInitializeAfterCar(&manager,&entry,car->Movement(),
            sharedBody,officerIndex)==CAR_DRIVER_OK,"AICop construction failed");
    } catch(...) {
        if(car->next && car->previous)worldRemoveCar(car.get());
        throw;
    }
    car.release();
}

// tPlayerCop PAL22D548..22D5B0 includes the complete tCop base22D3F8.
// Its Car is borrowed from the canonical first World table entry.
void PursuitConstructPlayerCop(PursuitCollisionEntry& entry,Car& car,int32_t officerIndex)
{
    Require(car.Movement(),"PlayerCop requires an actual live Car");
    entry.group=nullptr;entry.officerIndex=officerIndex;entry.lifecycleState=0;
    entry.kind=PURSUIT_COLLISION_EXTERNAL_CAR;entry.movement=car.Movement();
    entry.selectedTargetIndex=-1;entry.previousTargetIndex=-1;
    entry.targetSelectionTimeout=0.0f;entry.previousTargetTimeout=0.0f;
    entry.spikeOwnerSelector=0;entry.state=0;
}
}

int32_t pursuitEntryNearTarget(const PursuitCollisionEntry* entry)
{
    if(!entry->group)return 0;
    const float* target=entry->group->target->bindings.state->position;
    const float* position=entry->movement->bindings.state->position;
    const float x=target[0]-position[0];
    const float y=target[1]-position[1];
    const float xx=x*x;
    const float yy=y*y;
    return sqrtf(xx+yy)<20.0f;
}


void pursuitCollisionSetGroup(PursuitCollisionEntry* entry, PursuitCollisionGroup* group)
{
    entry->group = group;
}

void pursuitCollisionDoneWritingTicket(PursuitCollisionEntry* entry)
{
    if (entry->state == 2) return;
    if (entry->state != 7) pursuitCollisionSetState(entry, 7);
    entry->drivingState = 16;
    entry->drivingStateSeconds = 10.0f;
}

void pursuitCollisionDetach(PursuitCollisionEntry* entry)
{
    if (!entry->group) return;
    pursuitCollisionGroupRemove(entry->group, entry);
    pursuitCollisionSetGroup(entry, NULL);
}

void pursuitEntryHoldPosition(const PursuitCollisionEntry* entry,DriverCommand* command)
{
    driverCommandAdvance(command,NULL,entry->movement->bindings.environment->clock->worldTimer,0.0f);
    driverCommandAppend(command,entry->movement->bindings.state->position,NULL,-1,0,NULL);
}

void pursuitEntryClearSpeedTrap(PursuitCollisionEntry* entry)
{
    if (!entry->speedTrap) return;
    entry->speedTrap->officer = NULL;
    entry->speedTrap = NULL;
}

CarDriverResult pursuitEntryUpdateMovementMode(PursuitCollisionEntry* entry)
{
    if (!entry || !entry->movement) return CAR_DRIVER_INVALID_BINDING;
    CarMovement* movement = entry->movement;
    if (movement->mode == 0 || entry->state == 2) return CAR_DRIVER_OK;
    VehicleState* state = movement->bindings.state;
    if (!state || !movement->bindings.environment || !movement->bindings.environment->clock)
        return CAR_DRIVER_INVALID_BINDING;
    const float elapsed = movement->bindings.environment->clock->worldTimeElapsed;

    if (movement->mode != 5) {
        entry->modeElapsedSeconds = entry->modeElapsedSeconds + elapsed;
        if (!(0.0f <= entry->modeElapsedSeconds)) return CAR_DRIVER_OK;

        int32_t unstable = vehicleStateGroundedWheelCount(state) < 4;
        if (!unstable) {
            VectorPolar2 velocity;
            vectorPolarFromCartesian2(&velocity, state->velocity);
            int32_t headingDifference = (uint16_t)(velocity.angle - state->heading);
            if (headingDifference >= 32768) headingDifference -= 65536;
            if (headingDifference < 0) headingDifference = -headingDifference;
            // Exact startup230D00..18 writes unsigned angle0x071c, rather
            // than the zero value of the uninitialized console BSS cell.
            unstable = 1820 < headingDifference && 17.88109016418457f < state->speed;
            if (!unstable) {
                const float xx = state->angularVelocity[0] * state->angularVelocity[0];
                const float yy = state->angularVelocity[1] * state->angularVelocity[1];
                const float zz = state->angularVelocity[2] * state->angularVelocity[2];
                unstable = 1.0f < sqrtf((xx + yy) + zz);
            }
        }
        if (unstable) entry->modeSwitchDelaySeconds = 0.10000000149011612f;
        entry->modeSwitchDelaySeconds = entry->modeSwitchDelaySeconds - elapsed;
        if (entry->modeSwitchDelaySeconds <= 0.0f) {
            entry->modeSwitchDelaySeconds = 0.0f;
            if (carMovementSetMode(movement, 5) != CAR_MOVEMENT_OK)
                return CAR_DRIVER_MOVEMENT_FAILURE;
        }
        return CAR_DRIVER_OK;
    }

    entry->modeElapsedSeconds = 0.0f;
    if (!movement->cop) return CAR_DRIVER_INVALID_BINDING;
    if (!movement->cop->airborne) return CAR_DRIVER_OK;
    CarControls* controls = movement->bindings.controls;
    const Car* car = movement->bindings.owner;
    const CarDriver* driver = car ? (const CarDriver*)car->controller : NULL;
    // The recovered owned-police factory installs the concrete Cop driver
    // and its Null input. Other input families need their actual slot3c.
    if (!controls || !driver || driver->kind != CAR_DRIVER_COP)
        return CAR_DRIVER_INVALID_BINDING;
    if (2.0f < state->velocity[2]) state->velocity[2] = 2.0f;
    const float xx = state->velocity[0] * state->velocity[0];
    const float yy = state->velocity[1] * state->velocity[1];
    const float zz = state->velocity[2] * state->velocity[2];
    state->speed = sqrtf((xx + yy) + zz);
    state->velocityFrameValid = 0;
    entry->modeSwitchDelaySeconds = 0.10000000149011612f;
    if (carMovementSetMode(movement, 4) != CAR_MOVEMENT_OK)
        return CAR_DRIVER_MOVEMENT_FAILURE;
    controls->gearRequest = -1;
    controls->brake = 0.0f;
    controls->throttle = 1.0f;
    controls->handbrake = 0.0f;
    controls->steeringAngle = 0;
    // Actual input vtable2AE7C0 slot3c resolves to136BD8 (JRRA/NOP).
    // Its unused forwardSpeed/.5/brake/1 arguments create no new state.
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryInitializeSpeedTrap(PursuitCollisionEntry* entry)
{
    if (!entry || !entry->movement) return CAR_DRIVER_INVALID_BINDING;
    CarMovement* movement = entry->movement;
    Car* car = movement->bindings.owner;
    CarDriver* driver = car ? (CarDriver*)car->controller : NULL;
    VehicleState* state = movement->bindings.state;
    CarMotionState* motion = movement->bindings.ownerMotion;
    if (!driver || driver->kind != CAR_DRIVER_COP || !driver->owner.cop ||
        !state || !motion || !movement->frame)
        return CAR_DRIVER_INVALID_BINDING;

    pursuitCollisionDetach(entry);
    entry->drivingState = 0;
    motion->transitionCounter = 0;
    state->forwardSpeed = 0.0f;
    driver->owner.cop->drivingMode = 1;
    state->velocity[0] = 0.0f;
    state->velocity[1] = 0.0f;
    state->velocity[2] = 0.0f;
    // The original stack padding is not a fourth velocity component.
    state->velocity[3] = 0.0f;
    state->speed = 0.0f;
    entry->drivingStateSeconds = 15.0f;
    state->velocityFrameValid = 0;
    movement->frame->state.signalMode = 0;
    movement->frame->state.signalEnabled = 0;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryInitializeRoadBlock(PursuitCollisionEntry* entry)
{
    if (!entry || !entry->movement) return CAR_DRIVER_INVALID_BINDING;
    CarMovement* movement = entry->movement;
    Car* car = movement->bindings.owner;
    CarDriver* driver = car ? (CarDriver*)car->controller : NULL;
    VehicleState* state = movement->bindings.state;
    CarMotionState* motion = movement->bindings.ownerMotion;
    if (!driver || driver->kind != CAR_DRIVER_COP || !driver->owner.cop ||
        !state || !motion || !movement->frame)
        return CAR_DRIVER_INVALID_BINDING;
    entry->drivingState = 0;
    motion->transitionCounter = 0;
    state->forwardSpeed = 0.0f;
    driver->owner.cop->drivingMode = 1;
    state->velocity[0] = 0.0f;
    state->velocity[1] = 0.0f;
    state->velocity[2] = 0.0f;
    state->velocity[3] = 0.0f;
    state->speed = 0.0f;
    movement->frame->state.signalMode = 1;
    state->velocityFrameValid = 0;
    movement->frame->state.signalEnabled = 0;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryChangeState(PursuitManager* manager, PursuitCollisionEntry* entry,
    int32_t previousState, int32_t newState)
{
    if (!manager || !entry || !entry->movement) return CAR_DRIVER_INVALID_BINDING;
    CarMovement* movement = entry->movement;
    Car* car = movement->bindings.owner;
    CarDriver* driver = car ? (CarDriver*)car->controller : NULL;
    CarMotionState* motion = movement->bindings.ownerMotion;
    if (!driver || driver->kind != CAR_DRIVER_COP || !driver->owner.cop ||
        !motion || !motion->collision || !motion->collision->shape || !movement->frame)
        return CAR_DRIVER_INVALID_BINDING;
    CopDriver* cop = driver->owner.cop;
    CarFrameState* frame = &movement->frame->state;
    entry->drivingStateSeconds = 0.0f;

    switch (previousState) {
    case 1:
        collisionShapeUnlock(motion->collision->shape);
        motion->actionEnabled = 1;
        if (carMovementSetMode(movement, 5) != CAR_MOVEMENT_OK)
            return CAR_DRIVER_MOVEMENT_FAILURE;
        if (pursuitManagerResetCarDamage(manager, car) != WORLD_SERVICES_OK)
            return CAR_DRIVER_MISSING_PURSUIT;
        break;
    case 2:
        if (pursuitManagerResetCarDamage(manager, car) != WORLD_SERVICES_OK)
            return CAR_DRIVER_MISSING_PURSUIT;
        break;
    case 3:
        pursuitEntryClearSpeedTrap(entry);
        break;
    default:
        break;
    }

    switch (newState) {
    case 1:
        pursuitCollisionDetach(entry);
        motion->transitionCounter = 0;
        frame->signalEnabled = 0;
        frame->signalMode = 0;
        if (carMovementSetMode(movement, 0) != CAR_MOVEMENT_OK)
            return CAR_DRIVER_MOVEMENT_FAILURE;
        cop->drivingMode = 1;
        collisionShapeLock(motion->collision->shape);
        motion->actionEnabled = 0;
        if (previousState == 2) {
            entry->drivingState = 13;
            entry->drivingStateSeconds = 1.0f;
        } else {
            entry->drivingState = 14;
        }
        break;
    case 2: {
        pursuitEntryClearSpeedTrap(entry);
        pursuitCollisionDetach(entry);
        cop->drivingMode = 0;
        entry->drivingState = 0;
        frame->signalEnabled = 4;
        frame->signalMode = 1;
        motion->transitionCounter = 0;
        if (carMovementSetMode(movement, 4) != CAR_MOVEMENT_OK)
            return CAR_DRIVER_MOVEMENT_FAILURE;
        CarControls* controls = movement->bindings.controls;
        if (!controls) return CAR_DRIVER_MISSING_INPUT;
        controls->throttle = 0.0f;
        controls->brake = 0.30000001192092896f;
        controls->handbrake = 1.0f;
        controls->shiftTarget = -1;
        controls->steeringAngle = 0;
        controls->gearRequest = -1;
        // This class's actual Null input slot3c is empty136BD8. Source
        // supplies throttle/10/throttle/1; no controller state is created.
        break;
    }
    case 3:
        if (pursuitManagerResetCarDamage(manager, car) != WORLD_SERVICES_OK)
            return CAR_DRIVER_MISSING_PURSUIT;
        return pursuitEntryInitializeSpeedTrap(entry);
    case 4:
        if (pursuitManagerResetCarDamage(manager, car) != WORLD_SERVICES_OK)
            return CAR_DRIVER_MISSING_PURSUIT;
        return pursuitEntryInitializeRoadBlock(entry);
    case 5: {
        pursuitCollisionDetach(entry);
        motion->transitionCounter = 0;
        CarDriverResult result = pursuitEntrySelectRoad(entry);
        if (result != CAR_DRIVER_OK) return result;
        DriverCommand* command = carDriverMutableCommand(driver);
        if (!command) return CAR_DRIVER_MISSING_INPUT;
        result = pursuitEntryBuildRoadCommand(entry, command, movement->bindings.state->position, 0);
        if (result != CAR_DRIVER_OK) return result;
        cop->drivingMode = 2;
        frame->signalMode = 0;
        frame->signalEnabled = 0;
        // PAL retains drivingState here; the beta-only clear is absent.
        break;
    }
    case 6:
        frame->signalMode = 2;
        cop->drivingMode = 3;
        entry->requestedDrivingState = 3;
        frame->signalEnabled = 1;
        motion->transitionCounter = 1;
        entry->drivingState = 0;
        break;
    case 7:
        // Source calls SetPursuit(NULL) directly, not ClearPursuit.
        pursuitCollisionSetGroup(entry, NULL);
        cop->drivingMode = 1;
        entry->drivingState = 15;
        frame->signalMode = 1;
        motion->transitionCounter = 1;
        frame->signalEnabled = 0;
        break;
    case 8:
        pursuitEntryClearSpeedTrap(entry);
        pursuitCollisionDetach(entry);
        if (pursuitManagerResetCarDamage(manager, car) != WORLD_SERVICES_OK)
            return CAR_DRIVER_MISSING_PURSUIT;
        cop->drivingMode = 1;
        frame->signalMode = 0;
        entry->drivingState = 0;
        motion->transitionCounter = 0;
        frame->signalEnabled = 0;
        break;
    default:
        break;
    }
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryDoLimbo(PursuitCollisionEntry* entry)
{
    if (!entry || !entry->movement || !entry->movement->bindings.owner ||
        !entry->movement->bindings.environment || !entry->movement->bindings.environment->clock)
        return CAR_DRIVER_INVALID_BINDING;
    CarDriver* driver = (CarDriver*)entry->movement->bindings.owner->controller;
    DriverCommand* command = driver ? carDriverMutableCommand(driver) : NULL;
    if (!command) return CAR_DRIVER_MISSING_INPUT;
    pursuitEntryHoldPosition(entry, command);
    if (entry->drivingState == 13 && entry->drivingStateSeconds == 0.0f)
        entry->drivingState = 14;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryDoDisabled(PursuitCollisionEntry* entry, const World* world)
{
    if (!entry || !entry->movement || !entry->movement->bindings.owner || !world ||
        !entry->movement->bindings.environment || !entry->movement->bindings.environment->clock)
        return CAR_DRIVER_INVALID_BINDING;
    CarDriver* driver = (CarDriver*)entry->movement->bindings.owner->controller;
    DriverCommand* command = driver ? carDriverMutableCommand(driver) : NULL;
    if (!command) return CAR_DRIVER_MISSING_INPUT;
    pursuitEntryHoldPosition(entry, command);
    const float distance = worldGetDistanceToClosestPlayer2D(world, entry->movement->bindings.state->position);
    if (!(distance < 250.0f)) pursuitCollisionSetState(entry, 1);
    return CAR_DRIVER_OK;
}

int32_t pursuitEntryCanSeeTarget(const PursuitCollisionEntry* entry, const CarMovement* target)
{
    const VehicleState* own = entry->movement->bindings.state;
    const VehicleState* other = target->bindings.state;
    const float x = other->position[0] - own->position[0];
    const float y = other->position[1] - own->position[1];
    const float xx = x * x;
    const float yy = y * y;
    if (!(sqrtf(xx + yy) < 50.0f)) return 0;
    const float vx = other->velocity[0];
    const float vy = other->velocity[1];
    const float vxx = vx * vx;
    const float vyy = vy * vy;
    const float speed = sqrtf(vxx + vyy);
    if (!(0.0f < speed)) return 0;
    const float reciprocal = 1.0f / speed;
    const float unitX = vx * reciprocal;
    const float unitY = vy * reciprocal;
    const float alongX = unitX * x;
    const float alongY = unitY * y;
    return 20.0f <= alongX + alongY;
}

CarDriverResult pursuitEntryAcquire(PursuitCollisionEntry* entry, int32_t previousState,
    const CarMovement* target)
{
    if (!entry || !entry->movement || !entry->movement->bindings.route || !target)
        return CAR_DRIVER_INVALID_BINDING;
    if (previousState != entry->drivingState) {
        entry->laneIndex = 0;
        entry->alongOffset = 0.0f;
        entry->acrossOffset = 0.0f;
    }
    const float delta = trackWrappedDelta(entry->movement->bindings.route->length,
        entry->movement->routeLocation.distance, target->routeLocation.distance);
    if (-180.0f < delta) entry->requestedDrivingState = 3;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryStop(PursuitCollisionEntry* entry, int32_t previousState,
    const CarMovement* target)
{
    if (!entry || !entry->movement || !entry->movement->bindings.state ||
        !target || !target->bindings.state)
        return CAR_DRIVER_INVALID_BINDING;
    if (previousState != entry->drivingState) {
        entry->alongOffset = 0.0f;
        entry->drivingStateSeconds = 0.5f;
        entry->acrossOffset = 0.0f;
    }
    if (entry->drivingStateSeconds <= 0.0f) {
        const VehicleState* other = target->bindings.state;
        const float speed = fabsf(other->forwardSpeed * 2.236999988555908f);
        if (5.0f < speed) {
            entry->requestedDrivingState = 5;
        } else {
            const float x = other->position[0] - entry->movement->bindings.state->position[0];
            const float y = other->position[1] - entry->movement->bindings.state->position[1];
            const float xx = x * x;
            const float yy = y * y;
            if (10.0f < sqrtf(xx + yy)) entry->requestedDrivingState = 5;
        }
    }
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryGetAhead(PursuitCollisionEntry* entry, int32_t previousState)
{
    if (!entry || !entry->movement || !entry->movement->bindings.owner ||
        !entry->movement->bindings.state)
        return CAR_DRIVER_INVALID_BINDING;
    if (previousState == entry->drivingState) return CAR_DRIVER_OK;
    entry->alongOffset = 0.0f;
    const CarDriver* driver = (const CarDriver*)entry->movement->bindings.owner->controller;
    if (!driver) return CAR_DRIVER_MISSING_INPUT;
    const CarRecoveryPose* pose = carDriverCurrentPose(driver, 1);
    if (!pose) return CAR_DRIVER_MISSING_ROAD;
    const float x = entry->movement->bindings.state->position[0] - pose->position[0];
    const float y = entry->movement->bindings.state->position[1] - pose->position[1];
    const float first = x * pose->direction[1];
    const float second = y * pose->direction[0];
    entry->laneIndex = -1;
    entry->acrossOffset = first - second;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryBestFlankSide(const PursuitCollisionEntry* entry,
    const CarMovement* target, float* side)
{
    if (!entry || !entry->movement || !entry->movement->bindings.state ||
        !target || !target->bindings.owner || !target->bindings.state || !side)
        return CAR_DRIVER_INVALID_BINDING;
    const CarDriver* driver = (const CarDriver*)target->bindings.owner->controller;
    if (!driver) return CAR_DRIVER_MISSING_INPUT;
    // Preserve the real group query even though only its refresh effect is
    // used; the next call obtains the same target's current authored pose.
    (void)carDriverCurrentGroup(driver, 1);
    const CarRecoveryPose* pose = carDriverCurrentPose(driver, 1);
    if (!pose) return CAR_DRIVER_MISSING_ROAD;
    const float x = target->bindings.state->position[0] - pose->position[0];
    const float y = target->bindings.state->position[1] - pose->position[1];
    const float first = x * pose->direction[1];
    const float second = y * pose->direction[0];
    const float across = first - second;
    const float leftClearance = -pose->halfWidths[0] - across;
    const int32_t left = leftClearance <= -3.0f;
    const float rightClearance = pose->halfWidths[1] - across;
    const int32_t right = 3.0f <= rightClearance;
    float local[2];
    driverPerceptionLocalPoint(target->bindings.state, local, entry->movement->bindings.state->position);
    const int32_t close = fabsf(local[1]) < 2.5f;
    float result = 0.0f;
    if (local[0] < 0.0f) {
        if (left) result = -1.0f;
        else if (!close && right) result = 1.0f;
    } else {
        if (right) result = 1.0f;
        else if (!close && left) result = -1.0f;
    }
    *side = result;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitExternalEntryThink(PursuitManager* manager, PursuitCollisionEntry* entry)
{
    if (!manager || !manager->world || !manager->clock || !entry ||
        entry->kind != PURSUIT_COLLISION_EXTERNAL_CAR || !entry->movement)
        return CAR_DRIVER_INVALID_BINDING;
    CarMovement* movement = entry->movement;
    CarFrame* frame = movement->frame;
    if (!frame || frame->movement != movement) return CAR_DRIVER_INVALID_BINDING;

    int32_t replayActive = 0;
    if (movement->bindings.player) {
        const CarFramePlayerState* player = frame->environment.player;
        if (!player || !player->replay || player->car != movement ||
            &player->movement != movement->bindings.player)
            return CAR_DRIVER_INVALID_BINDING;
        replayActive = carFramePlayerReplayActive(player);
    }
    if (movement->bindings.player && !replayActive) {
        const float elapsed = manager->clock->worldTimeElapsed;
        entry->targetSelectionTimeout = entry->targetSelectionTimeout - elapsed;
        if (entry->targetSelectionTimeout <= 0.0f) {
            entry->targetSelectionTimeout = 0.0f;
            entry->selectedTargetIndex = -1;
        }
        entry->previousTargetTimeout = entry->previousTargetTimeout - elapsed;
        if (entry->previousTargetTimeout <= 0.0f) {
            entry->previousTargetTimeout = 0.0f;
            entry->previousTargetIndex = -1;
        }
    }

    const int32_t racers = manager->world->secondGroupEnd;
    if (racers < 0 || racers > 8) return CAR_DRIVER_INVALID_BINDING;
    int32_t pending = 0;
    for (int32_t index = 0; index < racers; ++index) {
        pending = manager->participantReset[index].resetPending;
        if (pending) break;
    }
    PursuitCollisionGroup* group = entry->group;
    if (group) {
        frame->state.signalEnabled = 1;
        frame->state.signalMode = 2;
    } else if (pending) {
        frame->state.signalEnabled = 0;
        frame->state.signalMode = 2;
    }
    if (group && group->chopper && group->chopper->totalDrops >= 4) {
        if (pursuitGroupReleaseChopper(manager, group) != WORLD_SERVICES_OK)
            return CAR_DRIVER_MISSING_PURSUIT;
    }
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryTeleport(PursuitCollisionEntry* entry, const float position[2], uint16_t heading)
{
    if (!entry) return CAR_DRIVER_INVALID_BINDING;
    // This optional-Car branch exists in the actual original method.
    if (!entry->movement) return CAR_DRIVER_OK;
    CarMovement* movement = entry->movement;
    Car* car = movement->bindings.owner;
    CarDriver* driver = car ? (CarDriver*)car->controller : nullptr;
    // The PAL successful path immediately calls SetMode(5) followed by
    // Car::ResetToPosition. The native owner may reject a missing route,
    // controller or collision body; detect those missing dependencies before
    // disposing the *previous* mover. B28 rolled back Pursuit's links but
    // could not repair a partially destroyed CarMovement on this failure.
    const auto& binding=movement->bindings;
    if(!position || !driver || !driver->environment ||
        driver->environment->movement!=movement || !binding.route ||
        !binding.state || !binding.controls || !binding.ownerMotion ||
        !binding.ownerMotion->collision || !binding.ownerMotion->collision->shape ||
        !binding.environment || !binding.environment->contact.terrain ||
        !binding.pools)
        return CAR_DRIVER_INVALID_BINDING;
    const float location[3] = {position[0], position[1], 0.0f};
    if (carMovementSetMode(movement, 5) != CAR_MOVEMENT_OK)
        return CAR_DRIVER_MOVEMENT_FAILURE;
    if (carMovementPlaceAndNotify(movement, driver, location, heading, 0.0f) != CAR_MOVEMENT_OK)
        return CAR_DRIVER_MOVEMENT_FAILURE;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryAssignSpeedTrap(PursuitCollisionEntry* entry, PursuitSpeedTrap* trap)
{
    if (!entry || !trap) return CAR_DRIVER_INVALID_BINDING;
    // Native movement placement can report a resource/terrain failure. The
    // original successful PAL sequence is unchanged, but a failed native
    // transition must not publish a new officer in two owners at once.
    PursuitSpeedTrap* previousTrap=entry->speedTrap;
    PursuitCollisionEntry* previousOfficer=trap->officer;
    entry->speedTrap = trap;
    trap->officer = entry;
    const CarDriverResult result = pursuitEntryTeleport(entry, trap->position, trap->heading);
    if (result != CAR_DRIVER_OK) {
        entry->speedTrap=previousTrap;trap->officer=previousOfficer;
        return result;
    }
    if(previousTrap && previousTrap!=trap && previousTrap->officer==entry)
        previousTrap->officer=nullptr;
    pursuitCollisionSetState(entry, 3);
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryAssignRoadBlock(PursuitCollisionEntry* entry, const PursuitRoadBlock* block,
    PursuitCollisionGroup* group, int32_t slot)
{
    if (!entry || !block || slot < 0 || slot >= 2) return CAR_DRIVER_INVALID_BINDING;
    // A rejected native placement must leave this officer attached to its
    // preceding pursuit, rather than a half-published new roadblock slot.
    PursuitCollisionGroup* previousGroup=entry->group;
    const std::int32_t previousSlot=entry->roadBlockSlot;
    pursuitCollisionSetGroup(entry, group);
    entry->roadBlockSlot = slot;
    const CarDriverResult result = pursuitEntryTeleport(entry, block->carPositions[slot], block->carHeadings[slot]);
    if (result != CAR_DRIVER_OK) {
        pursuitCollisionSetGroup(entry,previousGroup);
        entry->roadBlockSlot=previousSlot;
        return result;
    }
    pursuitCollisionSetState(entry, 4);
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryTargetAllowed(PursuitManager* manager, CarMovement* target,
    int32_t checkSpeed, int32_t* allowed)
{
    if (!manager || !manager->collision || !target || !target->bindings.owner ||
        !target->bindings.state || !allowed)
        return CAR_DRIVER_INVALID_BINDING;
    const PursuitCollisionContext* collision = manager->collision;
    if (collision->activeEntry != SIZE_MAX && collision->activeEntry >= collision->entryCount)
        return CAR_DRIVER_MISSING_PURSUIT;
    const PursuitCollisionEntry* selected = pursuitManagerSelectedEntry(manager);
    *allowed = 0;
    if (selected && selected->movement == target) return CAR_DRIVER_OK;
    const uint16_t index = target->bindings.owner->tableIndex;
    if (index >= 8) return CAR_DRIVER_INVALID_BINDING;
    const PursuitParticipantReset* racer = &manager->participantReset[index];
    if (racer->resetPending || 0.0f < racer->spawnDelay) return CAR_DRIVER_OK;
    CarMovement* other = nullptr;
    const CarDriverResult contact = pursuitCarCausedRecentCollision(target, &other);
    if (contact != CAR_DRIVER_OK) return contact;
    // PAL22EB48 takes the return with v0=1 in its delay slot. A real
    // causing-collision Car is eligible before the visibility/speed test.
    if (other) {
        *allowed = 1;
        return CAR_DRIVER_OK;
    }
    // PAL22EB50..5C returns false when the patrol cannot check speed.
    if (!checkSpeed) return CAR_DRIVER_OK;
    const float speed = fabsf(target->bindings.state->forwardSpeed * 2.236999988555908f);
    if (80.0f < speed) {
        *allowed = 1;
        return CAR_DRIVER_OK;
    }
    PursuitCollisionGroup* group = nullptr;
    if (pursuitManagerFindPursuitForCar(manager, target->bindings.owner, &group) != WORLD_SERVICES_OK)
        return CAR_DRIVER_MISSING_PURSUIT;
    *allowed = group != nullptr;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryDoRoadBlock(PursuitCollisionEntry* entry, const World* world)
{
    if (!entry || !entry->movement || !entry->movement->bindings.owner ||
        !entry->movement->bindings.state || !world || !entry->movement->bindings.environment ||
        !entry->movement->bindings.environment->clock)
        return CAR_DRIVER_INVALID_BINDING;
    CarMovement* movement = entry->movement;
    CarDriver* driver = (CarDriver*)movement->bindings.owner->controller;
    DriverCommand* command = driver ? carDriverMutableCommand(driver) : nullptr;
    if (!command) return CAR_DRIVER_MISSING_INPUT;
    pursuitEntryHoldPosition(entry, command);

    const PursuitRoadBlock* block = entry->group ? entry->group->reserved : nullptr;
    if (block) {
        const int32_t slot = entry->roadBlockSlot;
        if (slot < 0 || slot >= 2) return CAR_DRIVER_INVALID_BINDING;
        const float* position = block->carPositions[slot];
        const VehicleState* state = movement->bindings.state;
        const float x = position[0] - state->position[0];
        const float y = position[1] - state->position[1];
        const float xx = x * x;
        const float yy = y * y;
        const float distance = sqrtf(xx + yy);
        int32_t angle = (uint16_t)(block->carHeadings[slot] - state->heading);
        if (angle >= 32768) angle -= 65536;
        if (angle < 0) angle = -angle;
        if (0.009999999776482582f < distance || 100 < angle) {
            int32_t clear = 0;
            const CarDriverResult query = pursuitCanTeleport(world, movement, position, &clear);
            if (query != CAR_DRIVER_OK) return query;
            if (clear) {
                const CarDriverResult placed = pursuitEntryTeleport(entry, position, block->carHeadings[slot]);
                if (placed != CAR_DRIVER_OK) return placed;
            }
        }
    }
    // PAL's shared epilogue refreshes grace even without a reserved block;
    // the earlier beta body returned directly and has no such store.
    entry->graceRemaining = 3.0f;
    return CAR_DRIVER_OK;
}

namespace {

// Final-build transition called by the actual CaughtSpeeder/HotPursuit
// consumers. This has no independently licensed beta method identity.
void beginPatrol(PursuitCollisionEntry* entry)
{
    pursuitCollisionSetState(entry, 5);
    const float forwardSpeed = entry->movement->bindings.state->forwardSpeed;
    entry->drivingState = 13.410818099975586f < forwardSpeed ? 1 : 2;
}

// Native factoring of the three original, ordered racer-search loops.
// All references remain borrowed from the same World/Car/Movement owners.
CarDriverResult searchForPursuitTargets(PursuitManager* manager, PursuitCollisionEntry* entry,
    bool visibilityControlsSpeed)
{
    World* world = manager->world;
    const int32_t racers = world->secondGroupEnd;
    if (racers < 0 || racers > 8 || racers > world->totalCars)
        return CAR_DRIVER_INVALID_BINDING;
    for (int32_t index = 0; index < racers; ++index) {
        Car* car = world->carTable[index];
        CarMovement* target = car ? static_cast<CarMovement*>(car->movement) : nullptr;
        if (!target || target->bindings.owner != car || !target->bindings.state)
            return CAR_DRIVER_INVALID_BINDING;
        const float* own = entry->movement->bindings.state->position;
        const float* other = target->bindings.state->position;
        const float x = other[0] - own[0];
        const float y = other[1] - own[1];
        const float xx = x * x;
        const float yy = y * y;
        if (!(sqrtf(xx + yy) < 45.0f)) continue;
        if (!(fabsf(other[2] - own[2]) < 2.5f)) continue;
        const int32_t checkSpeed = visibilityControlsSpeed ? pursuitEntryCanSeeTarget(entry, target) : 1;
        int32_t allowed = 0;
        const CarDriverResult query = pursuitEntryTargetAllowed(manager, target, checkSpeed, &allowed);
        if (query != CAR_DRIVER_OK) return query;
        if (!allowed) continue;
        int32_t created = 0;
        if (pursuitManagerCreatePursuit(manager, entry, car, &created) != WORLD_SERVICES_OK)
            return CAR_DRIVER_MISSING_PURSUIT;
        if (created) pursuitCollisionSetState(entry, 6);
        // The original continues after changing lifecycle state; later
        // candidates still reach the same actual manager creation method.
    }
    return CAR_DRIVER_OK;
}

}

CarDriverResult pursuitEntryDoSpeedTrap(PursuitManager* manager, PursuitCollisionEntry* entry)
{
    if (!manager || !manager->world || !entry || !entry->movement ||
        !entry->movement->bindings.owner || !entry->movement->bindings.state ||
        !entry->movement->bindings.environment || !entry->movement->bindings.environment->clock)
        return CAR_DRIVER_INVALID_BINDING;
    CarMovement* movement = entry->movement;
    CarDriver* driver = static_cast<CarDriver*>(movement->bindings.owner->controller);
    DriverCommand* command = driver ? carDriverMutableCommand(driver) : nullptr;
    if (!command) return CAR_DRIVER_MISSING_INPUT;
    pursuitEntryHoldPosition(entry, command);
    const VehicleState* state = movement->bindings.state;
    if (entry->drivingStateSeconds <= 0.0f &&
        !(worldGetDistanceToClosestPlayer2D(manager->world, state->position) < 250.0f)) {
        pursuitCollisionSetState(entry, 1);
        return CAR_DRIVER_OK;
    }

    const PursuitSpeedTrap* trap = entry->speedTrap;
    if (!trap) return CAR_DRIVER_MISSING_PURSUIT;
    const float x = trap->position[0] - state->position[0];
    const float y = trap->position[1] - state->position[1];
    const float xx = x * x;
    const float yy = y * y;
    int32_t angle = (uint16_t)(trap->heading - state->heading);
    if (angle >= 32768) angle -= 65536;
    if (angle < 0) angle = -angle;
    if (0.009999999776482582f < sqrtf(xx + yy) || 100 < angle) {
        int32_t clear = 0;
        const CarDriverResult query = pursuitCanTeleport(manager->world, movement, trap->position, &clear);
        if (query != CAR_DRIVER_OK) return query;
        if (clear) {
            const CarDriverResult placed = pursuitEntryTeleport(entry, trap->position, trap->heading);
            if (placed != CAR_DRIVER_OK) return placed;
        }
    }
    return searchForPursuitTargets(manager, entry, false);
}

CarDriverResult pursuitEntryDoPatrol(PursuitManager* manager, PursuitCollisionEntry* entry)
{
    if (!manager || !manager->world || !entry || !entry->movement ||
        !entry->movement->bindings.owner || !entry->movement->bindings.state)
        return CAR_DRIVER_INVALID_BINDING;
    CarMovement* movement = entry->movement;
    CarDriver* driver = static_cast<CarDriver*>(movement->bindings.owner->controller);
    DriverCommand* command = driver ? carDriverMutableCommand(driver) : nullptr;
    if (!command) return CAR_DRIVER_MISSING_INPUT;
    entry->alongOffset = 0.0f;
    entry->targetOffset[1] = 0.0f;
    entry->targetOffset[0] = 0.0f;
    entry->acrossOffset = 0.0f;
    const CarDriverResult plan = pursuitEntryBuildRoadCommand(entry, command, movement->bindings.state->position, 0);
    if (plan != CAR_DRIVER_OK) return plan;
    if (!(worldGetDistanceToClosestPlayer2D(manager->world, movement->bindings.state->position) < 250.0f)) {
        pursuitCollisionSetState(entry, 1);
        return CAR_DRIVER_OK;
    }
    return searchForPursuitTargets(manager, entry, true);
}

CarDriverResult pursuitEntryDoCaughtSpeeder(PursuitManager* manager, PursuitCollisionEntry* entry)
{
    if (!manager || !manager->world || !entry || !entry->movement ||
        !entry->movement->bindings.owner || !entry->movement->bindings.state ||
        !entry->movement->bindings.environment || !entry->movement->bindings.environment->clock)
        return CAR_DRIVER_INVALID_BINDING;
    CarMovement* movement = entry->movement;
    CarDriver* driver = static_cast<CarDriver*>(movement->bindings.owner->controller);
    DriverCommand* command = driver ? carDriverMutableCommand(driver) : nullptr;
    if (!command) return CAR_DRIVER_MISSING_INPUT;
    pursuitEntryHoldPosition(entry, command);
    if (entry->drivingState != 16) return CAR_DRIVER_OK;
    if (!(worldGetDistanceToClosestPlayer2D(manager->world, movement->bindings.state->position) < 250.0f)) {
        pursuitCollisionSetState(entry, 1);
    } else if (entry->drivingStateSeconds == 0.0f) {
        beginPatrol(entry);
    }
    return searchForPursuitTargets(manager, entry, false);
}

namespace {

// Source31C5C8: one mutable table, reset by every real owned-officer
// constructor. Queries never manufacture a per-entry or per-group copy.
int32_t zonePermissions[6][6];

}

void pursuitZonePermissionsInitialize(void)
{
    zonePermissions[0][0] = 0; zonePermissions[0][1] = 0; zonePermissions[0][2] = 0;
    zonePermissions[0][3] = 0; zonePermissions[0][4] = 1; zonePermissions[0][5] = 0;
    zonePermissions[1][0] = 1; zonePermissions[1][1] = 0; zonePermissions[1][2] = 1;
    zonePermissions[1][3] = 0; zonePermissions[1][4] = 1; zonePermissions[1][5] = 1;
    zonePermissions[2][0] = 1; zonePermissions[2][1] = 1; zonePermissions[2][2] = 0;
    zonePermissions[2][3] = 1; zonePermissions[2][4] = 1; zonePermissions[2][5] = 0;
    zonePermissions[3][0] = 1; zonePermissions[3][1] = 0; zonePermissions[3][2] = 1;
    zonePermissions[3][3] = 0; zonePermissions[3][4] = 1; zonePermissions[3][5] = 1;
    zonePermissions[4][0] = 1; zonePermissions[4][1] = 1; zonePermissions[4][2] = 1;
    zonePermissions[4][3] = 1; zonePermissions[4][4] = 0; zonePermissions[4][5] = 1;
    zonePermissions[5][0] = 1; zonePermissions[5][1] = 1; zonePermissions[5][2] = 0;
    zonePermissions[5][3] = 1; zonePermissions[5][4] = 1; zonePermissions[5][5] = 0;
}

CarDriverResult pursuitZonePermission(int32_t requested, int32_t occupied, int32_t* allowed)
{
    if (!allowed) return CAR_DRIVER_INVALID_BINDING;
    if (requested == 6 || occupied == 6) {
        *allowed = 1;
        return CAR_DRIVER_OK;
    }
    if (requested < 0 || requested >= 6 || occupied < 0 || occupied >= 6)
        return CAR_DRIVER_INVALID_BINDING;
    *allowed = zonePermissions[occupied][requested];
    return CAR_DRIVER_OK;
}

int32_t pursuitZoneFromDrivingState(int32_t state, float side)
{
    switch (state) {
    case 3: return 4;
    case 4: return side < 0.0f ? 3 : 5;
    case 7:
    case 8:
    case 10: return side < 0.0f ? 1 : 2;
    case 9: return 0;
    default: return 6;
    }
}

CarDriverResult pursuitEntryFollow(PursuitManager* manager, PursuitCollisionEntry* entry,
    int32_t previousState, const CarMovement* target)
{
    if (!manager || !manager->world || !manager->collision || !entry || !entry->group ||
        !entry->movement || !entry->movement->bindings.state || !target || !target->bindings.state)
        return CAR_DRIVER_INVALID_BINDING;
    PursuitCollisionGroup* group = entry->group;
    const bool leading = group->active[0] == entry;
    if (previousState != entry->drivingState) {
        entry->drivingStateSeconds = 3.0f;
        entry->alongOffset = leading ? 6.5f : 13.0f;
        entry->acrossOffset = 0.0f;
    }
    const float* own = entry->movement->bindings.state->position;
    const float* other = target->bindings.state->position;
    const float x = other[0] - own[0];
    const float y = other[1] - own[1];
    const float xx = x * x;
    const float yy = y * y;
    if (sqrtf(xx + yy) < 21.0f) {
        if (entry->alongOffset == 0.0f) entry->alongOffset = leading ? 6.5f : 13.0f;
        const float randomBounds[3] = {3.0f, 2.0f, 1.0f};
        const int32_t severity = target->bindings.player ? manager->collision->severity : 2;
        if (severity < 0 || severity >= 3) return CAR_DRIVER_INVALID_BINDING;
        // This draw precedes the timer/contact tests in the original.
        const float draw = worldRandomFloat(manager->world, randomBounds[severity]);
        if (draw < 1.0f && entry->drivingStateSeconds == 0.0f) {
            int32_t collided = 0;
            const CarDriverResult contact = pursuitCarsCollided(entry->movement, target, &collided);
            if (contact != CAR_DRIVER_OK) return contact;
            if (!collided) {
                float side = 0.0f;
                const CarDriverResult flank = pursuitEntryBestFlankSide(entry, target, &side);
                if (flank != CAR_DRIVER_OK) return flank;
                if (side != 0.0f) {
                    const PursuitCollisionEntry* partner = leading ? group->active[1] : group->active[0];
                    int32_t allowed = 1;
                    if (partner && partner->movement) {
                        const int32_t requested = pursuitZoneFromDrivingState(4, side);
                        const int32_t occupied = pursuitZoneFromDrivingState(partner->drivingState, partner->flankSide);
                        const CarDriverResult permission = pursuitZonePermission(requested, occupied, &allowed);
                        if (permission != CAR_DRIVER_OK) return permission;
                    }
                    if (allowed) {
                        entry->flankSide = side;
                        entry->requestedDrivingState = 4;
                    }
                }
            }
        }
        if (fabsf(target->bindings.state->forwardSpeed * 2.236999988555908f) < 20.0f)
            entry->requestedDrivingState = 5;
    } else {
        entry->drivingStateSeconds = 3.0f;
        entry->alongOffset = leading ? 6.5f : 13.0f;
        entry->acrossOffset = leading ? 0.0f : 2.5f;
    }
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryLowSpeedFollow(PursuitCollisionEntry* entry, int32_t previousState,
    const CarMovement* target)
{
    if (!entry || !entry->movement || !entry->movement->bindings.state || !target || !target->bindings.state)
        return CAR_DRIVER_INVALID_BINDING;
    if (previousState != entry->drivingState) {
        entry->acrossOffset = 0.0f;
        entry->alongOffset = 6.0f;
    }
    const float speed = fabsf(target->bindings.state->forwardSpeed * 2.236999988555908f);
    if (20.0f < speed) entry->requestedDrivingState = 3;
    if (speed < 5.0f) {
        const float x = target->bindings.state->position[0] - entry->movement->bindings.state->position[0];
        const float y = target->bindings.state->position[1] - entry->movement->bindings.state->position[1];
        const float yy = y * y;
        const float xx = x * x;
        if (sqrtf(xx + yy) < 10.0f) entry->requestedDrivingState = 12;
    }
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryFlank(PursuitManager* manager, PursuitCollisionEntry* entry,
    int32_t previousState, const CarMovement* target)
{
    if (!manager || !manager->world || !entry || !entry->group || !entry->movement ||
        !entry->movement->bindings.state || !target || !target->bindings.state)
        return CAR_DRIVER_INVALID_BINDING;
    if (previousState != entry->drivingState) {
        entry->alongOffset = 0.0f;
        entry->drivingStateSeconds = 2.25f;
        entry->acrossOffset = entry->flankSide * 3.0f;
    }
    float side = 0.0f;
    const CarDriverResult flank = pursuitEntryBestFlankSide(entry, target, &side);
    if (flank != CAR_DRIVER_OK) return flank;
    if (side != entry->flankSide) {
        entry->requestedDrivingState = 4;
        return CAR_DRIVER_OK;
    }
    const float relative[2] = {
        entry->movement->bindings.state->position[0] - target->bindings.state->position[0],
        entry->movement->bindings.state->position[1] - target->bindings.state->position[1]};
    VectorPolar2 polar;
    vectorPolarFromCartesian2(&polar, relative);
    int32_t angle = (uint16_t)(polar.angle - target->bindings.state->heading);
    if (angle >= 32768) angle -= 65536;
    if (angle < 0) angle = -angle;
    const bool forward = angle < 0x4000;
    if (forward) {
        if (target->bindings.player) {
            int32_t next = 10;
            if (entry->group->requestFlags & 4u) {
                const uint32_t draw = worldRandomBounded(manager->world, 15);
                if (!(draw < 2 && entry->group->active[0] == entry)) next = 8;
            }
            entry->requestedDrivingState = next;
        } else if (entry->group->requestFlags & 4u) {
            entry->requestedDrivingState = worldRandomBounded(manager->world, 15) < 2 ? 10 : 8;
        }
    }
    const float desired[2] = {entry->alongOffset, entry->acrossOffset};
    if (entry->drivingStateSeconds == 0.0f &&
        vectorNear2(entry->targetOffset, desired, 0.009999999776482582f)) {
        entry->drivingStateSeconds = 2.25f;
        if (!forward && angle >= 0x6000) entry->requestedDrivingState = 4;
    }
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryBump(PursuitManager* manager, PursuitCollisionEntry* entry,
    int32_t previousState, CarMovement* target)
{
    if (!manager || !manager->collision || !manager->parameters || !entry || !entry->movement ||
        !entry->movement->bindings.state || !target || !target->bindings.state)
        return CAR_DRIVER_INVALID_BINDING;
    const float strengths[3] = {7.0f, 10.0f, 15.0f};
    const int32_t severity = target->bindings.player ? manager->collision->severity : 2;
    if (severity < 0 || severity >= 3) return CAR_DRIVER_INVALID_BINDING;
    if (previousState != entry->drivingState) entry->drivingStateSeconds = 1.75f;
    const float timer = entry->drivingStateSeconds;
    const float edge = fabsf(timer - 0.875f);
    const float factor = (edge + edge) / 1.75f;
    const float sideOffset = entry->flankSide * 3.0f;
    entry->alongOffset = 0.0f;
    entry->offsetRate[1] = 100.0f;
    entry->offsetRate[0] = 100.0f;
    entry->acrossOffset = factor * sideOffset;
    if (timer == 0.0f) {
        entry->requestedDrivingState = 7;
        return CAR_DRIVER_OK;
    }
    if (manager->parameters->pursuitDifficulty == 2) return CAR_DRIVER_OK;
    int32_t collided = 0;
    const CarDriverResult contact = pursuitCarsCollided(entry->movement, target, &collided);
    if (contact != CAR_DRIVER_OK) return contact;
    if (!collided) return CAR_DRIVER_OK;
    if (timer < 0.4375f) {
        float direction[4]={};
        for (int lane = 0; lane < 3; ++lane)
            direction[lane] = target->bindings.state->position[lane] - entry->movement->bindings.state->position[lane];
        const float xx = direction[0] * direction[0];
        const float yy = direction[1] * direction[1];
        const float zz = direction[2] * direction[2];
        const float reciprocal = finiteRatio(1.0f, sqrtf((xx + yy) + zz));
        float impulse[4]={};
        for (int lane = 0; lane < 3; ++lane) {
            const float unit = direction[lane] * reciprocal;
            impulse[lane] = unit * strengths[severity];
        }
        if (target->mode != 0) {
            CollisionParticipant* participant = target->collisionParticipant;
            if (!participant || participant->movement != target || !target->collisionSystem)
                return CAR_DRIVER_INVALID_BINDING;
            collisionParticipantNotify(participant, nullptr);
            if (target->result != CAR_MOVEMENT_OK) return CAR_DRIVER_MOVEMENT_FAILURE;
            if (!participant->shape || !participant->shape->body) return CAR_DRIVER_INVALID_BINDING;
            const float lever[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            collisionParticipantApplyImpulse(target->collisionSystem, participant, impulse, lever);
        }
    }
    entry->requestedDrivingState = 7;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryFollowBackwards(PursuitManager* manager, PursuitCollisionEntry* entry,
    const CarMovement* target)
{
    if (!manager || !manager->world || !entry || !entry->group || !entry->movement ||
        !entry->movement->bindings.state || !target || !target->bindings.state || !target->bindings.owner)
        return CAR_DRIVER_INVALID_BINDING;
    const float x = target->bindings.state->position[0] - entry->movement->bindings.state->position[0];
    const float y = target->bindings.state->position[1] - entry->movement->bindings.state->position[1];
    const float xx = x * x;
    const float yy = y * y;
    const float distance = sqrtf(xx + yy);
    if (entry->group->active[0] == entry) {
        if (distance < 20.0f) {
            entry->alongOffset = 6.5f;
            if (entry->drivingStateSeconds == 0.0f) {
                float side = 0.0f;
                const CarDriverResult flank = pursuitEntryBestFlankSide(entry, target, &side);
                if (flank != CAR_DRIVER_OK) return flank;
                const float distances[3] = {1.0f, 0.5f, 0.25f};
                const uint32_t index = worldRandomBounded(manager->world, 3);
                entry->drivingStateSeconds = 1.25f;
                entry->acrossOffset = distances[index] * side;
            }
        } else {
            entry->alongOffset = 0.0f;
            entry->acrossOffset = 0.0f;
        }
    } else {
        entry->alongOffset = distance < 20.0f ? 13.0f : 0.0f;
        entry->acrossOffset = 2.5f;
    }
    const VehicleState* state = target->bindings.state;
    if (fabsf(state->forwardSpeed * 2.236999988555908f) < 5.0f && distance < 10.0f)
        entry->requestedDrivingState = 12;
    const CarDriver* driver = static_cast<const CarDriver*>(target->bindings.owner->controller);
    if (!driver) return CAR_DRIVER_MISSING_INPUT;
    const CarRecoveryPose* pose = carDriverCurrentPose(driver, 1);
    if (!pose) return CAR_DRIVER_MISSING_ROAD;
    const float alongX = state->velocity[0] * pose->direction[0];
    const float alongY = state->velocity[1] * pose->direction[1];
    if (1.0f < alongX + alongY) entry->requestedDrivingState = 3;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryFollowOffset(PursuitCollisionEntry* entry, int32_t previousState,
    const CarMovement* target)
{
    if (!entry || !entry->movement || !entry->movement->bindings.state || !target || !target->bindings.state)
        return CAR_DRIVER_INVALID_BINDING;
    if (previousState != entry->drivingState) entry->drivingStateSeconds = 2.0f;
    entry->alongOffset = 6.5f;
    entry->acrossOffset = entry->flankSide * 3.0f;
    float side = 0.0f;
    const CarDriverResult flank = pursuitEntryBestFlankSide(entry, target, &side);
    if (flank != CAR_DRIVER_OK) return flank;
    if (side != entry->flankSide) {
        entry->requestedDrivingState = 3;
        return CAR_DRIVER_OK;
    }
    const float desired[2] = {entry->alongOffset, entry->acrossOffset};
    if (entry->drivingStateSeconds != 0.0f ||
        !vectorNear2(entry->targetOffset, desired, 0.009999999776482582f))
        return CAR_DRIVER_OK;
    int32_t collided = 0;
    const CarDriverResult contact = pursuitCarsCollided(entry->movement, target, &collided);
    if (contact != CAR_DRIVER_OK) return contact;
    if (collided) {
        entry->drivingStateSeconds = 2.0f;
        return CAR_DRIVER_OK;
    }
    const float relative[2] = {
        entry->movement->bindings.state->position[0] - target->bindings.state->position[0],
        entry->movement->bindings.state->position[1] - target->bindings.state->position[1]};
    VectorPolar2 polar;
    vectorPolarFromCartesian2(&polar, relative);
    int32_t angle = (uint16_t)(polar.angle - target->bindings.state->heading);
    if (angle >= 32768) angle -= 65536;
    if (angle < 0) angle = -angle;
    entry->requestedDrivingState = angle >= 0x6000 && 15.0f < polar.radius ? 3 : 7;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitEntryBlock(PursuitCollisionEntry* entry, int32_t previousState,
    const CarMovement* target)
{
    if (!entry || !entry->movement || !entry->movement->bindings.state ||
        !entry->movement->bindings.route || !target || !target->bindings.state || !target->bindings.owner)
        return CAR_DRIVER_INVALID_BINDING;
    if (previousState != entry->drivingState) {
        entry->alongOffset = 0.0f;
        entry->laneIndex = -1;
    }
    entry->alongOffset = 0.0f;
    entry->offsetRate[1] = 100.0f;
    entry->offsetRate[0] = 100.0f;
    entry->acrossOffset = 0.0f;
    const float x = target->bindings.state->position[0] - entry->movement->bindings.state->position[0];
    const float y = target->bindings.state->position[1] - entry->movement->bindings.state->position[1];
    const float xx = x * x;
    const float yy = y * y;
    const float distance = sqrtf(xx + yy);
    const float delta = trackWrappedDelta(entry->movement->bindings.route->length,
        entry->movement->routeLocation.distance, target->routeLocation.distance);
    const VehicleState* state = target->bindings.state;
    if (delta < 0.0f) {
        entry->requestedDrivingState = 3;
    } else {
        const CarDriver* driver = static_cast<const CarDriver*>(target->bindings.owner->controller);
        if (!driver) return CAR_DRIVER_MISSING_INPUT;
        const CarRecoveryPose* pose = carDriverCurrentPose(driver, 1);
        if (!pose) return CAR_DRIVER_MISSING_ROAD;
        // Source reads total speed(+3C4) here, separately from the signed
        // forward speed(+3C0) in the following close-follow test.
        if (20.0f < fabsf(state->speed * 2.236999988555908f)) {
            const float alongX = state->velocity[0] * pose->direction[0];
            const float alongY = state->velocity[1] * pose->direction[1];
            if (alongX + alongY < 0.0f && 20.0f < distance) entry->requestedDrivingState = 6;
        }
    }
    if (fabsf(state->forwardSpeed * 2.236999988555908f) < 20.0f && 10.0f < distance)
        entry->requestedDrivingState = 5;
    return CAR_DRIVER_OK;
}

namespace {

// Original inline two-coordinate distance used by Disable. The old neutral
// Record owner still has its own pure local arithmetic; neither has state.
float distance(const CarMovement* first, const CarMovement* second)
{
    const float* a = first->bindings.state->position;
    const float* b = second->bindings.state->position;
    const float x = a[0] - b[0], y = a[1] - b[1];
    const float squaredX = x * x, squaredY = y * y;
    return sqrtf(squaredX + squaredY);
}

}

void pursuitCollisionDisable(PursuitCollisionContext* context, PursuitCollisionEntry* entry, int32_t event)
{
    if (entry->lifecycleState == 1 || entry->lifecycleState == 2) return;
    if (entry->movement->bindings.damage) carDamageSetFull(entry->movement->bindings.damage);
    PursuitCollisionGroup* group = entry->group;
    if (group) {
        CarMovement* target = group->target;
        if (target->bindings.player && distance(target, entry->movement) < 75.0f)
            pursuitCollisionChangeRadio(context, group, event);
        if (target && target->bindings.player) {
            const uint16_t index = target->bindings.owner->tableIndex;
            if (index >= 8 || !context->manager || context->manager->collision != context) {
                context->result = PURSUIT_COLLISION_INVALID_BINDING;
                return;
            }
            float* timer = &context->manager->participantReset[index].preventPursuitSeconds;
            *timer = *timer + 10.0f;
        }
        pursuitCollisionDetach(entry);
    }
    pursuitCollisionSetState(entry, 2);
}

CarDriverResult pursuitEntryDoHotPursuit(PursuitManager* manager, PursuitCollisionEntry* entry)
{
    if (!manager || !entry || !entry->group || !entry->group->target || !entry->movement ||
        !entry->movement->bindings.route || !entry->movement->bindings.state)
        return CAR_DRIVER_INVALID_BINDING;
    CarMovement* target = entry->group->target;
    if (!target->bindings.owner || !target->bindings.state) return CAR_DRIVER_INVALID_BINDING;
    const int32_t previous = entry->drivingState;
    if (previous != entry->requestedDrivingState) entry->drivingState = entry->requestedDrivingState;
    const float delta = trackWrappedDelta(entry->movement->bindings.route->length,
        entry->movement->routeLocation.distance, target->routeLocation.distance);
    if (450.0f <= fabsf(delta)) {
        beginPatrol(entry);
        pursuitCollisionDetach(entry);
        return CAR_DRIVER_OK;
    }
    if (delta <= -180.0f) {
        entry->requestedDrivingState = 11;
    } else if (6.0f <= delta && 20.0f <= target->bindings.state->forwardSpeed * 2.236999988555908f) {
        const CarDriver* driver = static_cast<const CarDriver*>(target->bindings.owner->controller);
        if (!driver) return CAR_DRIVER_MISSING_INPUT;
        const CarRecoveryPose* pose = carDriverCurrentPose(driver, 1);
        if (!pose) return CAR_DRIVER_MISSING_ROAD;
        const float alongX = target->bindings.state->velocity[0] * pose->direction[0];
        const float alongY = target->bindings.state->velocity[1] * pose->direction[1];
        entry->requestedDrivingState = alongX + alongY < 0.0f ? 6 : 9;
    }
    // Requests above apply to the following frame. The original dispatches
    // the CURRENT state selected at entry and passes its previous value.
    switch (entry->drivingState) {
    case 3: return pursuitEntryFollow(manager, entry, previous, target);
    case 4: return pursuitEntryFollowOffset(entry, previous, target);
    case 5: return pursuitEntryLowSpeedFollow(entry, previous, target);
    case 6: return pursuitEntryFollowBackwards(manager, entry, target);
    case 7: return pursuitEntryFlank(manager, entry, previous, target);
    case 8: return pursuitEntryBump(manager, entry, previous, target);
    case 9: return pursuitEntryBlock(entry, previous, target);
    case 10: return pursuitEntryGetAhead(entry, previous);
    case 11: return pursuitEntryAcquire(entry, previous, target);
    case 12: return pursuitEntryStop(entry, previous, target);
    default: return CAR_DRIVER_OK;
    }
}

CarDriverResult pursuitOwnedEntryThink(PursuitManager* manager, PursuitCollisionEntry* entry)
{
    if (!manager || !manager->world || !manager->clock || !manager->collision || !entry ||
        entry->kind != PURSUIT_COLLISION_OWNED_CAR || !entry->movement ||
        !entry->movement->bindings.state || !entry->movement->bindings.ownerMotion ||
        !entry->movement->bindings.environment || entry->movement->bindings.environment->clock != manager->clock)
        return CAR_DRIVER_INVALID_BINDING;
    const float elapsed = manager->clock->worldTimeElapsed;
    if (0.0f < entry->graceRemaining) entry->graceRemaining = entry->graceRemaining - elapsed;
    // The source's selected-driver command-model overlay is an explicitly
    // retired console presentation edge; no gameplay command is replaced.
    const CarDriverResult mode = pursuitEntryUpdateMovementMode(entry);
    if (mode != CAR_DRIVER_OK) return mode;
    const VehicleState* state = entry->movement->bindings.state;
    PursuitCollisionContext* collision = manager->collision;
    if (state->orientation[10] <= 0.10000000149011612f && state->speed * 2.236999988555908f < 10.0f) {
        collision->result = PURSUIT_COLLISION_OK;
        pursuitCollisionDisable(collision, entry, 1);
        if (collision->result != PURSUIT_COLLISION_OK) return CAR_DRIVER_MISSING_PURSUIT;
    }
    if (entry->movement->bindings.ownerMotion->disabledWheelMask != 0) {
        collision->result = PURSUIT_COLLISION_OK;
        pursuitCollisionDisable(collision, entry, 0);
        if (collision->result != PURSUIT_COLLISION_OK) return CAR_DRIVER_MISSING_PURSUIT;
    }
    entry->drivingStateSeconds = entry->drivingStateSeconds - elapsed;
    if (entry->drivingStateSeconds < 0.0f) entry->drivingStateSeconds = 0.0f;
    if (entry->lifecycleState != entry->state) {
        const CarDriverResult transition = pursuitEntryChangeState(manager, entry, entry->lifecycleState, entry->state);
        if (transition != CAR_DRIVER_OK) return transition;
        entry->lifecycleState = entry->state;
    }
    CarDriverResult result = CAR_DRIVER_OK;
    switch (entry->state) {
    case 1: result = pursuitEntryDoLimbo(entry); break;
    case 2: result = pursuitEntryDoDisabled(entry, manager->world); break;
    case 3: result = pursuitEntryDoSpeedTrap(manager, entry); break;
    case 4: result = pursuitEntryDoRoadBlock(entry, manager->world); break;
    case 5: result = pursuitEntryDoPatrol(manager, entry); break;
    case 6: result = pursuitEntryDoHotPursuit(manager, entry); break;
    case 7: result = pursuitEntryDoCaughtSpeeder(manager, entry); break;
    case 8: {
        Car* car = entry->movement->bindings.owner;
        CarDriver* driver = car ? static_cast<CarDriver*>(car->controller) : nullptr;
        DriverCommand* command = driver ? carDriverMutableCommand(driver) : nullptr;
        if (!command) return CAR_DRIVER_MISSING_INPUT;
        pursuitEntryHoldPosition(entry, command);
        break;
    }
    default: break;
    }
    if (result != CAR_DRIVER_OK) return result;
    const float desired[2] = {entry->alongOffset, entry->acrossOffset};
    for (int axis = 0; axis < 2; ++axis) {
        const float current = entry->targetOffset[axis];
        const float delta = desired[axis] - current;
        // At zero delta the original saturating0/0 direction is clipped
        // back to delta. Express that final result without an intermediate
        // IEEE infinity/NaN, including a zero-duration frame.
        const float direction = delta == 0.0f ? 0.0f : finiteRatio(delta, fabsf(delta));
        const float velocity = direction * entry->offsetRate[axis];
        const float step = velocity * elapsed;
        entry->targetOffset[axis] = current + (fabsf(delta) < fabsf(step) ? delta : step);
    }
    entry->offsetRate[1] = 3.0f;
    entry->offsetRate[0] = 6.0f;
    return CAR_DRIVER_OK;
}

CarDriverResult pursuitOwnedEntryInitializeAfterCar(PursuitManager* manager, PursuitCollisionEntry* entry,
    CarMovement* movement, RigidBodyInfo* sharedModelBody, int32_t officerIndex)
{
    // Native ownership boundary ONLY. The actual allocating Car constructor
    // (including rendering/audio lifetime) must finish before this tail.
    // This adapter is not a whole22DB20 or130A48 implementation/count.
    if (!manager || !manager->world || !manager->collision || !manager->clock || !entry ||
        !movement || !movement->bindings.owner || !movement->bindings.state ||
        !movement->bindings.ownerMotion || !movement->bindings.environment ||
        movement->bindings.environment->clock != manager->clock || movement->mode != 0 ||
        !movement->inactive || !movement->frame || movement->frame->movement != movement ||
        movement->frame->environment.world != manager->world ||
        movement->bindings.pursuit != manager->collision || !sharedModelBody ||
        sharedModelBody != movement->info.body || !(0.0f < sharedModelBody->mass) ||
        officerIndex < 0 || officerIndex >= WORLD_MAX_CARS)
        return CAR_DRIVER_INVALID_BINDING;
    if (manager->collision->severity < 0 || manager->collision->severity >= 3)
        return CAR_DRIVER_INVALID_BINDING;
    Car* car = movement->bindings.owner;
    CarDriver* driver = static_cast<CarDriver*>(car->controller);
    if (!driver || driver->kind != CAR_DRIVER_NULL || !driver->nullOwner ||
        driver->nullOwner->movement != movement || !driver->environment ||
        driver->environment->movement != movement || driver->environment->pursuit != manager->collision ||
        car->next || car->previous || !manager->world->carList.previous || !manager->world->carList.next ||
        (manager->collision->manager && manager->collision->manager != manager))
        return CAR_DRIVER_INVALID_BINDING;
    if (movement->info.modelCode != 4 && (movement->info.modelCode < 31 || movement->info.modelCode > 34))
        return CAR_DRIVER_INVALID_BINDING;

    manager->collision->manager = manager;
    entry->group = nullptr;
    entry->officerIndex = officerIndex;
    entry->lifecycleState = 0;
    entry->kind = PURSUIT_COLLISION_OWNED_CAR;
    entry->roadBlockSlot = 0;
    entry->recovery.group = nullptr;
    entry->recovery.pose = nullptr;
    entry->drivingStateSeconds = 0.0f;
    const float masses[3] = {1.5f, 1.75f, 2.0f};
    const float oldMass = sharedModelBody->mass;
    const float ratio = finiteRatio(masses[manager->collision->severity], oldMass);
    entry->movement = movement;
    worldAddCar(manager->world, car);
    // Source mutates the SAME per-model resource, observable by every other
    // live borrower. Do not update a body's cached mass, rebuild performance
    // tables, modify momenta or restore the resource when the officer dies.
    sharedModelBody->mass = oldMass * ratio;
    for (int lane = 0; lane < 12; ++lane)
        sharedModelBody->inertia[lane] = sharedModelBody->inertia[lane] * ratio;
    const CarDriverResult changed = carDriverChange(driver, car, CAR_DRIVER_COP);
    if (changed != CAR_DRIVER_OK) return changed;
    CopDriver* cop = driver->owner.cop;
    if (!cop || cop->base.movement != movement) return CAR_DRIVER_MISSING_INPUT;
    cop->drivingMode = 1;
    cop->entryIndex = entry->officerIndex;
    if (carMovementSetMode(movement, 0) != CAR_MOVEMENT_OK) return CAR_DRIVER_MOVEMENT_FAILURE;
    movement->bindings.ownerMotion->actionEnabled = 0;
    entry->modeElapsedSeconds = 0.0f;
    entry->state = 1;
    entry->targetOffset[1] = 0.0f;
    entry->targetOffset[0] = 0.0f;
    entry->acrossOffset = 0.0f;
    entry->alongOffset = 0.0f;
    entry->flankSide = 0.0f;
    entry->modeSwitchDelaySeconds = 0.0f;
    entry->laneIndex = 0;
    entry->offsetRate[0] = 6.0f;
    entry->offsetRate[1] = 3.0f;
    movement->bindings.ownerMotion->transitionCounter = 0;
    movement->frame->state.signalEnabled = 0;
    movement->frame->state.signalMode = 0;
    pursuitZonePermissionsInitialize();
    entry->graceRemaining = 0.0f;
    return CAR_DRIVER_OK;
}

WorldServicesResult pursuitEntryReturnToPatrol(PursuitCollisionEntry* entry)
{
    if (!entry || !entry->movement || !entry->movement->bindings.state)
        return WORLD_SERVICES_INVALID_BINDING;
    beginPatrol(entry);
    return WORLD_SERVICES_OK;
}

// PAL22D700..22D7E4. A PlayerCop horn changes the actual chased car, or
// toggles only Car+808 when no target and no pursuit exist. The independent
// signal-enabled field is not written by this source method.
WorldServicesResult pursuitExternalEntryChangeChaseTarget(PursuitManager* manager,PursuitCollisionEntry* entry)
{
    if(!manager || !entry || entry->kind!=PURSUIT_COLLISION_EXTERNAL_CAR || !entry->movement)
        return WORLD_SERVICES_INVALID_BINDING;
    CarMovement* movement=entry->movement;
    if(!movement->bindings.player)return manager->result=WORLD_SERVICES_OK;
    CarFrame* frame=movement->frame;
    if(!frame || !frame->environment.player ||
        &frame->environment.player->movement!=movement->bindings.player)
        return manager->result=WORLD_SERVICES_INVALID_BINDING;
    if(carFramePlayerReplayActive(frame->environment.player))return manager->result=WORLD_SERVICES_OK;
    manager->result=WORLD_SERVICES_OK;
    CarMovement* target=pursuitManagerSelectHornTarget(manager,entry);
    if(manager->result!=WORLD_SERVICES_OK)return manager->result;
    if(target) {
        if(entry->group) {
            if(entry->group->target!=target)return pursuitGroupChangeChasedCar(manager,entry->group,target);
            if(!manager->audio)return manager->result=WORLD_SERVICES_INVALID_BINDING;
            const WorldAudioResult audio=worldAudioPlayInGame(manager->audio,"BeACopRequestFailed",1.0f,0.0f,0.0f);
            if(audio==WORLD_AUDIO_CAPACITY)return manager->result=WORLD_SERVICES_CAPACITY;
            return manager->result=audio==WORLD_AUDIO_OK ? WORLD_SERVICES_OK : WORLD_SERVICES_INVALID_BINDING;
        }
        if(!target->bindings.owner)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        int32_t created=0;
        return pursuitManagerCreatePursuit(manager,entry,target->bindings.owner,&created);
    }
    if(!entry->group)frame->state.signalMode=frame->state.signalMode ? 0 : 1;
    return manager->result=WORLD_SERVICES_OK;
}

// Original behavior: PAL22D898..22DB1C.
CarMovement* pursuitManagerSelectHornTarget(PursuitManager* manager, PursuitCollisionEntry* entry)
{
    float scores[WORLD_MAX_CARS];
    float bestScore = -1.0f;
    int32_t selected = -1;
    const Car* observer;
    int32_t index;
    if (!manager || !manager->world || !manager->clock || !entry || !entry->movement ||
        !entry->movement->bindings.owner || !entry->movement->bindings.state ||
        manager->world->secondGroupEnd > WORLD_MAX_CARS) {
        if (manager) manager->result = WORLD_SERVICES_INVALID_BINDING;
        return NULL;
    }
    observer = entry->movement->bindings.owner;
    for (index = 0; index < manager->world->secondGroupEnd; ++index) {
        const Car* car = manager->world->carTable[index];
        const CarMovement* movement;
        scores[index] = -1.0f;
        if (!car || !car->raceEntry || !car->movement) {
            manager->result = WORLD_SERVICES_INVALID_BINDING;
            return NULL;
        }
        movement = (const CarMovement*)car->movement;
        if (!movement->bindings.ownerMotion || !movement->bindings.state) {
            manager->result = WORLD_SERVICES_INVALID_BINDING;
            return NULL;
        }
        if (car != observer && !carMotionWindowActive(movement->bindings.ownerMotion, manager->clock->worldTimer) &&
            !car->raceEntry->completionReason) {
            const float x = observer->position[0] - car->position[0];
            const float y = observer->position[1] - car->position[1];
            const float xx = x * x;
            const float yy = y * y;
            const float distance = sqrtf(xx + yy);
            if ((fabsf(observer->position[2] - car->position[2]) < 20.0f && distance < 175.0f &&
                  driverCarInHeadingCone(entry->movement->bindings.state, movement->bindings.state, 10000)) ||
                distance < 20.0f)
                scores[index] = 185.0f - distance;
        }
    }
    if (entry->selectedTargetIndex >= 0 && entry->selectedTargetIndex < manager->world->secondGroupEnd &&
        scores[entry->selectedTargetIndex] > -1.0f) scores[entry->selectedTargetIndex] = 0.0f;
    if (entry->previousTargetIndex >= 0 && entry->previousTargetIndex < manager->world->secondGroupEnd &&
        scores[entry->previousTargetIndex] > -1.0f) scores[entry->previousTargetIndex] = 5.0f;
    for (index = 0; index < manager->world->secondGroupEnd; ++index) {
        if (scores[index] > bestScore) {
            bestScore = scores[index];
            selected = index;
        }
    }
    if (selected == -1) return NULL;
    entry->previousTargetIndex = entry->selectedTargetIndex;
    entry->selectedTargetIndex = selected;
    entry->previousTargetTimeout = entry->targetSelectionTimeout;
    entry->targetSelectionTimeout = 5.0f;
    return (CarMovement*)manager->world->carTable[selected]->movement;
}

// PAL22D440..22D530. All four state cells are raw one-byte enum transfers;
// they must not be sign-extended like the separate signed index overloads.
void pursuitEntryBaseDoSnapshot(PursuitManager* manager,PursuitCollisionEntry* entry,
    ReplaySnapshot* snapshot)
{
    hp2::Require(manager && manager->world && entry && snapshot,
        "Cop snapshot has no canonical World/manager/entry");
    replaySnapshotBeginSection(snapshot,nullptr,"tCop");
    Car* car=entry->movement?entry->movement->bindings.owner:nullptr;
    worldSnapshotCarPointer(manager->world,snapshot,&car);
    if(snapshot->restoring)entry->movement=car?car->Movement():nullptr;
    pursuitManagerSnapshotGroup(manager,snapshot,&entry->group);
    if(entry->group && (!pursuitGroupContainsCop(entry->group,entry) || entry->group->owner->id<0))
        entry->group=nullptr;
    replaySnapshotMoveData(snapshot,&entry->lifecycleState,1,-1,0);
    replaySnapshotMoveData(snapshot,&entry->drivingState,1,-1,0);
    replaySnapshotMoveData(snapshot,&entry->state,1,-1,0);
    replaySnapshotMoveData(snapshot,&entry->requestedDrivingState,1,-1,0);
    replaySnapshotEndSection(snapshot);
}

// PAL22D7E8..22D894. PlayerCop retains its original base size: unlike AICop,
// its two selection indices begin at20 and no AICop grace timer is present.
void pursuitPlayerCopDoSnapshot(PursuitManager* manager,PursuitCollisionEntry* entry,
    ReplaySnapshot* snapshot)
{
    hp2::Require(entry && entry->kind==PURSUIT_COLLISION_EXTERNAL_CAR,
        "PlayerCop snapshot was given a different concrete officer");
    replaySnapshotBeginSection(snapshot,entry,"tPlayerCop");
    pursuitEntryBaseDoSnapshot(manager,entry,snapshot);
    replaySnapshotMoveInteger(snapshot,&entry->selectedTargetIndex,1);
    replaySnapshotMoveFloat(snapshot,&entry->targetSelectionTimeout);
    replaySnapshotMoveInteger(snapshot,&entry->previousTargetIndex,1);
    replaySnapshotMoveFloat(snapshot,&entry->previousTargetTimeout);
    replaySnapshotMoveData(snapshot,&entry->spikeOwnerSelector,1,4,0);
    replaySnapshotEndSection(snapshot);
}

// PAL22DD20..22DE70. The restored speed-trap back-reference and route
// references belong to the same manager catalogue and immutable road owner.
void pursuitAiCopDoSnapshot(PursuitManager* manager,PursuitCollisionEntry* entry,
    ReplaySnapshot* snapshot)
{
    hp2::Require(entry && entry->kind==PURSUIT_COLLISION_OWNED_CAR,
        "AICop snapshot was given a different concrete officer");
    replaySnapshotBeginSection(snapshot,entry,"tCop");
    pursuitEntryBaseDoSnapshot(manager,entry,snapshot);
    pursuitManagerSnapshotSpeedTrap(manager,snapshot,&entry->speedTrap);
    replaySnapshotMoveInteger(snapshot,&entry->roadBlockSlot,1);
    replaySnapshotMoveFloat(snapshot,&entry->modeElapsedSeconds);
    replaySnapshotMoveFloat(snapshot,&entry->modeSwitchDelaySeconds);
    replaySnapshotMoveInteger(snapshot,&entry->laneIndex,4);
    replaySnapshotMoveFloat(snapshot,&entry->drivingStateSeconds);
    for(float& value:entry->targetOffset)replaySnapshotMoveFloat(snapshot,&value);
    replaySnapshotMoveFloat(snapshot,&entry->alongOffset);
    replaySnapshotMoveFloat(snapshot,&entry->acrossOffset);
    for(float& value:entry->offsetRate)replaySnapshotMoveFloat(snapshot,&value);
    replaySnapshotMoveFloat(snapshot,&entry->flankSide);
    hp2::Require(entry->movement && entry->movement->bindings.recoveryPoints,
        "AICop snapshot has no original road registry");
    carRecoverySnapshotGroup(entry->movement->bindings.recoveryPoints,snapshot,&entry->recovery.group);
    carRecoverySnapshotPoseInGroup(entry->recovery.group,snapshot,&entry->recovery.pose);
    if(snapshot->restoring && entry->speedTrap)entry->speedTrap->officer=entry;
    replaySnapshotMoveFloat(snapshot,&entry->graceRemaining);
    replaySnapshotEndSection(snapshot);
}

// PAL22D538..22D540 SetDesiredLogicState; this is separate from the current lifecycle state.
void pursuitCollisionSetState(PursuitCollisionEntry* entry, int32_t state)
{
    entry->state = state;
}

// Native typed Car controller projection used by the actual road command owner.
static const CarDriver* controller(const CarMovement* movement)
{
    return (const CarDriver*)movement->bindings.owner->controller;
}

// PAL230638..2307D0 InitializeDriveTarget. Final substate2/9/10/11 and explicit current-road refresh are authoritative.
CarDriverResult pursuitEntrySelectRoad(PursuitCollisionEntry* entry)
{
    entry->recovery.group=NULL;
    entry->recovery.pose=NULL;
    const CarMovement* movement=entry->movement;
    const CarRecoveryPointRegistry* registry=movement->bindings.recoveryPoints;
    if(entry->state==5){
        int32_t selected=0;
        if(entry->drivingState==2)
            selected=carRecoverySelectPose(&entry->recovery,registry,movement->bindings.state->position,3,2u,0.0f);
        if(selected){
            const CarRecoveryPose* current=carDriverCurrentPose(controller(movement),1);
            const float x=entry->recovery.pose->position[0]-current->position[0];
            const float y=entry->recovery.pose->position[1]-current->position[1];
            const float first=x*current->direction[1];
            const float second=y*current->direction[0];
            float side=first-second;
            float width=current->halfWidths[1];
            if(side<0.0f){side=-side;width=current->halfWidths[0];}
            if(width<side)selected=0;
        }
        if(!selected)
            carRecoverySelectPose(&entry->recovery,registry,movement->bindings.state->position,0,2u,0.0f);
    }else if(entry->state==6){
        const CarMovement* selected=NULL;
        if(entry->drivingState==9||entry->drivingState==10){
            if(!entry->group||!entry->group->target)return CAR_DRIVER_MISSING_PURSUIT;
            selected=entry->group->target;
        }else if(entry->drivingState==11)selected=movement;
        if(selected){
            entry->recovery.group=carDriverCurrentGroup(controller(selected),1);
            entry->recovery.pose=carDriverCurrentPose(controller(selected),1);
        }
    }
    return CAR_DRIVER_OK;
}

// PAL2307D0..230A5C UpdateDriveTarget; bounded original road refresh and20 authored command points.
CarDriverResult pursuitEntryBuildRoadCommand(PursuitCollisionEntry* entry,DriverCommand* command,
    const float position[2],int32_t keepAuthoredStart)
{
    const CarRecoveryPointRegistry* registry=entry->movement->bindings.recoveryPoints;
    if(!keepAuthoredStart){
        int32_t refreshed=0;
        int32_t advances=0;
        for(;;){
            if(advances==10){
                if(refreshed)break;
                const CarDriverResult result=pursuitEntrySelectRoad(entry);
                if(result!=CAR_DRIVER_OK)return result;
                refreshed=1;advances=0;
            }
            const CarRecoveryPose* pose=entry->recovery.pose;
            if(!pose)return CAR_DRIVER_MISSING_ROAD;
            const float x=position[0]-pose->position[0];
            const float y=position[1]-pose->position[1];
            const float first=x*pose->direction[0];
            const float second=y*pose->direction[1];
            int32_t advance=0;
            if(0.0f<first+second)advance=advances++<10;
            while(advance){
                CarRecoverySelection next=entry->recovery;
                const CarDriverResult result=driverRoadNextPose(registry,&next);
                if(result!=CAR_DRIVER_OK)return result;
                if(next.pose)entry->recovery=next;
                pose=entry->recovery.pose;
                const float nextX=position[0]-pose->position[0];
                const float nextY=position[1]-pose->position[1];
                const float projectedX=nextX*pose->direction[0];
                const float projectedY=nextY*pose->direction[1];
                if(!(0.0f<projectedX+projectedY))break;
                advance=advances++<10;
            }
            if(advances!=10)break;
        }
    }
    driverCommandAdvance(command,NULL,entry->movement->bindings.environment->clock->worldTimer,0.0f);
    CarRecoverySelection road=entry->recovery;
    for(int32_t index=0;index<DRIVER_COMMAND_CAPACITY;++index){
        if(!road.group||!road.pose)return CAR_DRIVER_MISSING_ROAD;
        float across=entry->acrossOffset;
        if(entry->laneIndex!=-1){
            const float authored=(float)((int32_t)road.pose->laneOffsetsQuantized[entry->laneIndex]*256)*0.0000152587890625f;
            across=across+authored;
        }
        float point[4];driverRoadPointPosition(road.pose,point,entry->alongOffset,across);
        driverCommandAppend(command,point,&road,0,0,NULL);
        const CarDriverResult result=driverRoadNextPose(registry,&road);
        if(result!=CAR_DRIVER_OK)return result;
        if(!road.pose)break;
    }
    if(!keepAuthoredStart){
        command->points[0].position[0]=position[0];
        command->points[0].position[1]=position[1];
    }
    return CAR_DRIVER_OK;
}
