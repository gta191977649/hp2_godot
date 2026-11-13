#include "indep/inc/Collision.hpp"
#include "indep/inc/Contact.hpp"
#include "indep/inc/VoronoiClip.hpp"
#include "indep/inc/Smokeable.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Munitions.hpp"
#include <limits.h>
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/World.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/TrackPath.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>
#include <string.h>
#include "indep/inc/Clans.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/CarLoader.hpp"
#include "indep/inc/SmokeableInfo.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/Chopper.hpp"
#include "indep/inc/CarSounds.hpp"
#include "indep/inc/RealDriver.hpp"
#include "indep/inc/TerrainEffects.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Chopper.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include <array>
#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdio>

// Original World::DoTimestepMove PAL14D898..14D940. Freeze the active
// traversal before running any concrete mover/lifetime owner.
CollisionSystemResult worldServicesAdvanceParticipants(CollisionSystem* system, float elapsed)
{
    CollisionParticipant* active[COLLISION_ID_COUNT];
    size_t count=0;
    for (CollisionShape* shape=system->space->root.next;shape!=&system->space->root;shape=shape->next) {
        if (!shape->active) continue;
        if (count==COLLISION_ID_COUNT) return system->result=COLLISION_SYSTEM_PARTICIPANT_CAPACITY;
        CollisionParticipant* participant=collisionObjectParticipant(system,shape);
        if (!participant || (!participant->movement && !participant->breakable))
            return system->result=COLLISION_SYSTEM_MISSING_OWNER;
        active[count++]=participant;
    }
    for (size_t index=0;index<count;++index) {
        collisionParticipantCapture(system,active[index],elapsed);
        if (system->result!=COLLISION_SYSTEM_OK) return system->result;
    }
    return COLLISION_SYSTEM_OK;
}

void worldSetTimeScale(WorldTiming* world, float scale, float duration,
                       WorldTimeoutCallback callback, void* context)
{
    world->callbackContext = context;
    world->timeScale = scale;
    world->scaleDuration = duration;
    world->callback = callback;
}

void worldResetTimeScale(WorldTiming* world)
{
    world->callbackContext = NULL;
    world->timeScale = 1.0f;
    world->scaleDuration = 0.0f;
    world->pausedSeconds = 0.0f;
    world->callback = NULL;
}

void worldSetPaused(WorldTiming* world, float seconds,
                    WorldTimeoutCallback callback, void* context)
{
    world->callbackContext = context;
    world->pausedSeconds = seconds;
    world->callback = callback;
}

WorldCarTableResult worldBuildCarTable(World* world)
{
    const Car* sentinel = &world->carList;
    int32_t available = 0;
    for (const Car* car = world->carList.next; car != sentinel; car = car->next) {
        const int32_t group = car->driverInfo->driverClass;
        if (group >= 1 && group <= 5) {
            ++available;
            if (available > WORLD_MAX_CARS)
                return WORLD_CARS_TOO_MANY;
        }
    }
    int32_t count = 0;
    // The original function enumerates the same linked list four times:
    // first group 1, then 2, then 4 and 5 together, then group 3. Other
    // group values do not enter the simulation's car table.
    for (int pass = 0; pass < 4; ++pass) {
        for (Car* car = world->carList.next; car != sentinel; car = car->next) {
            const int32_t group = car->driverInfo->driverClass;
            const int selected = (pass == 0 && group == 1) ||
                (pass == 1 && group == 2) ||
                (pass == 2 && (group == 4 || group == 5)) ||
                (pass == 3 && group == 3);
            if (selected) {
                world->carTable[count] = car;
                car->tableIndex = (uint16_t)count;
                ++count;
            }
        }
        if (pass == 0) world->firstGroupEnd = count;
        if (pass == 1) world->secondGroupEnd = count;
        if (pass == 2) world->thirdGroupEnd = count;
    }
    world->totalCars = count;
    for (int32_t i = count; i < WORLD_MAX_CARS; ++i)
        world->carTable[i] = NULL;
    hp2::WorldSynchronizeCarViews(*world);
    return WORLD_CARS_OK;
}

Car* worldGetCarByDriverNumber(const World* world, int32_t driverNumber)
{
    for (int32_t i = 0; i < world->totalCars; ++i) {
        Car* car = world->carTable[i];
        if (car->driverNumber == driverNumber)
            return car;
    }
    return NULL;
}

float worldGetDistanceToClosestPlayer2D(const World* world, const float position[2])
{
    const float dx0 = position[0] - world->playerPositions[0][0];
    const float dy0 = position[1] - world->playerPositions[0][1];
    const float first = sqrtf(dx0 * dx0 + dy0 * dy0);
    const float dx1 = position[0] - world->playerPositions[1][0];
    const float dy1 = position[1] - world->playerPositions[1][1];
    const float second = sqrtf(dx1 * dx1 + dy1 * dy1);
    return first < second ? first : second;
}

float worldGetDistanceToClosestPlayer3D(const World* world, const float position[3])
{
    const float dx0 = position[0] - world->playerPositions[0][0];
    const float dy0 = position[1] - world->playerPositions[0][1];
    const float dz0 = position[2] - world->playerPositions[0][2];
    const float first = sqrtf((dx0 * dx0 + dy0 * dy0) + dz0 * dz0);

    const float dx1 = position[0] - world->playerPositions[1][0];
    const float dy1 = position[1] - world->playerPositions[1][1];
    const float dz1 = position[2] - world->playerPositions[1][2];
    const float second = sqrtf((dx1 * dx1 + dy1 * dy1) + dz1 * dz1);
    return first < second ? first : second;
}

int worldCarIsBefore(const Car* first, const Car* second)
{
    return first->position[0] <= second->position[0];
}

// A new world frame increments the simulation's own frame number.
uint32_t worldAdvanceFrame(World* world)
{
    return ++world->frameCounter;
}

// Player-slot bookkeeping stays in the world rather than the renderer or
// platform layer. The paired flags and planar coordinates retain their
// original per-slot association.
void worldRecordPlayerSlot(World* world, int32_t index,
                           uint8_t flagA, uint8_t flagB,
                           const float positionA[2], const float positionB[2])
{
    // The game's valid slot range is determined when its table is built.
    // Reject corrupt inputs rather than overrunning an unrelated host field.
    if (index < 0 || index >= 40)
        return;
    world->playerSlotFlags[0][index] = flagA;
    world->playerSlotFlags[1][index] = flagB;
    world->lastPlayerSlot = index;
    world->playerSlotPosition[0][index][0] = positionA[0];
    world->playerSlotPosition[0][index][1] = positionA[1];
    world->playerSlotPosition[1][index][0] = positionB[0];
    world->playerSlotPosition[1][index][1] = positionB[1];
}

static int worldSimulationValidBinding(const WorldSimulation* simulation)
{
    if (!simulation || !simulation->world || !simulation->clock || !simulation->collisions ||
        !simulation->collisions->space || simulation->collisions->world != simulation->world ||
        !simulation->players || !simulation->players->services ||
        !simulation->players->services->session || !simulation->particles ||
        simulation->players->world != simulation->world ||
        simulation->players->clock != simulation->clock || !simulation->replay ||
        simulation->replay->clock != simulation->clock || !simulation->pursuitServices ||
        simulation->pursuitServices->world != simulation->world ||
        !simulation->pursuitServices->trackGraph || !simulation->pursuitServices->landmarks ||
        simulation->pursuitServices->manager != simulation->pursuit)
        return 0;
    return simulation->world->totalCars == 0 || (simulation->frames && simulation->drivers);
}

static int worldSimulationValidCars(const WorldSimulation* simulation)
{
    const World* world = simulation->world;
    if (world->totalCars < 0 || world->totalCars > WORLD_MAX_CARS ||
        simulation->frameCount != (size_t)world->totalCars || world->firstGroupEnd < 0 ||
        world->secondGroupEnd < world->firstGroupEnd || world->thirdGroupEnd < world->secondGroupEnd ||
        world->thirdGroupEnd > world->totalCars)
        return 0;
    for (int32_t index = 0; index < world->totalCars; ++index) {
        const Car* car = world->carTable[index];
        if (!car || !simulation->frames[index] || !simulation->drivers[index]) return 0;
        CarMovement* movement = carFrameMovement(simulation->frames[index]);
        if (!movement || movement->bindings.owner != car ||
            car->controller != simulation->drivers[index] || car->interactionContext != simulation->frames[index] ||
            !car->controllerThink || !car->interaction || !car->movement || !car->move ||
            !car->controls || !car->updateControls)
            return 0;
    }
    const CarFramePlayerRegistry* players = simulation->players;
    if (players->count < 0 || players->count > 2) return 0;
    for (int32_t index = 0; index < players->count; ++index) {
        const CarFramePlayerState* player = players->players[index];
        if (!player || player->registry != players || !player->car ||
            player->movement.seat < 0 || player->movement.seat > 1 ||
            players->playersByNumber[player->movement.seat] != player)
            return 0;
    }
    for (int32_t seat = 0; seat < 2; ++seat) {
        const CarFramePlayerState* player = players->playersByNumber[seat];
        if (player && (!player->car || player->registry != players || player->movement.seat != seat))
            return 0;
    }
    return 1;
}

static WorldSimulationResult worldSimulationThinkCar(WorldSimulation* simulation, int32_t index, float elapsed)
{
    simulation->failedCar = index;
    if (carThink(simulation->world->carTable[index], elapsed) != CAR_THINK_OK ||
        simulation->drivers[index]->result != CAR_DRIVER_OK)
        return simulation->result = WORLD_SIMULATION_DRIVER_FAILURE;
    if (carFrameGetResult(simulation->frames[index]) != CAR_FRAME_OK)
        return simulation->result = WORLD_SIMULATION_FRAME_FAILURE;
    return WORLD_SIMULATION_OK;
}

static int worldSimulationSuppressSurfaceFeedback(const WorldSimulation* simulation)
{
    const int32_t state = simulation->players->services->session->state;
    return state == 4 || state == 7 || state == 5 || state == 9;
}

static WorldSimulationResult worldSimulationFailCar(WorldSimulation* simulation, int32_t index, WorldSimulationResult result)
{
    simulation->failedCar = index;
    return simulation->result = result;
}

static void worldResetPhysicsStep(World* world);

/* PAL14DD10 resets before the Car passes. The alternative14DF98/14DFCC
 * stores reset after PostStep (the likely-branch delay at14DF98 executes
 * only when there are no Cars). One World context remains live throughout
 * integration and collision rewinds; Car environments only borrow it. */
static void worldSimulationResetContactElapsed(WorldSimulation* simulation)
{
    worldResetPhysicsStep(simulation->world);
}

WorldSimulationResult worldDoTimestep(WorldSimulation* simulation, float elapsed)
{
    if (simulation) simulation->stateFailure = {-1, nullptr, -1};
    if (!worldSimulationValidBinding(simulation)) return WORLD_SIMULATION_INVALID_BINDING;
    if (!worldSimulationValidCars(simulation)) return simulation->result = WORLD_SIMULATION_INVALID_CAR_TABLE;
    if (!isfinite(elapsed) || elapsed < 0.0f) return simulation->result = WORLD_SIMULATION_INVALID_BINDING;
    World* world = simulation->world;
    CollisionSystem* collisions = simulation->collisions;
    simulation->result = WORLD_SIMULATION_OK;
    simulation->failedCar = -1;
    collisions->nowTicks = simulation->clock->worldTimer;
    collisions->frame = (uint32_t)simulation->clock->worldLoopCounter;
    if (worldReplaySynchronize(simulation->replay, world) != WORLD_REPLAY_OK) {
        if (simulation->replay->result == WORLD_REPLAY_INVALID_STATE) {
            (void)worldInspectReplayState(world,simulation->clock,&simulation->stateFailure);
            simulation->failedCar=simulation->stateFailure.car;
        }
        return simulation->result = WORLD_SIMULATION_REPLAY_FAILURE;
    }

    worldSimulationResetContactElapsed(simulation);
    for (int32_t seat = 0; seat < 2; ++seat) {
        CarFramePlayerState* player = simulation->players->playersByNumber[seat];
        if (player && player->car) {
            CarMovement* movement = player->car;
            memcpy(world->playerPositions[seat], movement->bindings.state->position, sizeof(float) * 4);
            world->playerTrackDistances[seat] = movement->routeLocation.distance;
        } else {
            for (int32_t axis = 0; axis < 3; ++axis)
                world->playerPositions[seat][axis] = 9999999.0f;
            world->playerTrackDistances[seat] = 9999999.0f;
        }
    }
    if (simulation->traffic && trafficSchedulerUpdate(simulation->traffic, elapsed) != WORLD_SERVICES_OK)
        return simulation->result = WORLD_SIMULATION_SERVICE_FAILURE;
    if (pursuitManagerPreThink(simulation->pursuitServices) != WORLD_SERVICES_OK)
        return simulation->result = WORLD_SIMULATION_SERVICE_FAILURE;

    for (int32_t index = 0; index < world->secondGroupEnd - world->firstGroupEnd; ++index)
        if (worldSimulationThinkCar(simulation, world->firstGroupEnd + index, elapsed) != WORLD_SIMULATION_OK) return simulation->result;
    for (int32_t index = 0; index < world->totalCars - world->thirdGroupEnd; ++index)
        if (worldSimulationThinkCar(simulation, world->thirdGroupEnd + index, elapsed) != WORLD_SIMULATION_OK) return simulation->result;
    for (int32_t index = 0; index < world->firstGroupEnd; ++index)
        if (worldSimulationThinkCar(simulation, index, elapsed) != WORLD_SIMULATION_OK) return simulation->result;
    for (int32_t index = 0; index < world->thirdGroupEnd - world->secondGroupEnd; ++index)
        if (worldSimulationThinkCar(simulation, world->secondGroupEnd + index, elapsed) != WORLD_SIMULATION_OK) return simulation->result;

    for (int32_t index = 0; index < world->firstGroupEnd; ++index)
        if (carFrameSetupForceFeedback(simulation->frames[index]) != CAR_FRAME_OK)
            return worldSimulationFailCar(simulation, index, WORLD_SIMULATION_FRAME_FAILURE);
    for (int32_t index = 0; index < world->totalCars; ++index) {
        if (carDrive(world->carTable[index], elapsed) != CAR_DRIVE_OK ||
            simulation->drivers[index]->result != CAR_DRIVER_OK)
            return worldSimulationFailCar(simulation, index, WORLD_SIMULATION_DRIVER_FAILURE);
    }
    if (simulation->chopper && chopperAdvance(simulation->chopper, elapsed) != WORLD_SERVICES_OK)
        return simulation->result = WORLD_SIMULATION_SERVICE_FAILURE;
    worldServicesPeriodicObjectCull(collisions);
    if (collisions->result != COLLISION_SYSTEM_OK)
        return simulation->result = WORLD_SIMULATION_COLLISION_FAILURE;

    for (int32_t index = 0; index < world->totalCars; ++index)
        if (carFramePreStep(simulation->frames[index], elapsed) != CAR_FRAME_OK)
            return worldSimulationFailCar(simulation, index, WORLD_SIMULATION_FRAME_FAILURE);
    if (worldServicesAdvanceParticipants(collisions, elapsed) != COLLISION_SYSTEM_OK ||
        collisionSystemStep(collisions, elapsed) != COLLISION_SYSTEM_OK)
        return simulation->result = WORLD_SIMULATION_COLLISION_FAILURE;
    for (int32_t index = 0; index < world->totalCars; ++index)
        if (carFramePostStep(simulation->frames[index], elapsed) != CAR_FRAME_OK)
            return worldSimulationFailCar(simulation, index, WORLD_SIMULATION_FRAME_FAILURE);

    worldSimulationResetContactElapsed(simulation);
    for (int32_t index = 0; index < world->totalCars; ++index) {
        if (worldGetDistanceToClosestPlayer3D(world, world->carTable[index]->position) < 100.0f &&
            carFrameInteractionSetup(simulation->frames[index]) != CAR_FRAME_OK)
            return worldSimulationFailCar(simulation, index, WORLD_SIMULATION_FRAME_FAILURE);
    }
    if (!worldSimulationSuppressSurfaceFeedback(simulation)) {
        for (int32_t index = 0; index < world->firstGroupEnd; ++index) {
            if (carFrameUpdateAccelerationFeedback(simulation->frames[index]) != CAR_FRAME_OK)
                return worldSimulationFailCar(simulation, index, WORLD_SIMULATION_FRAME_FAILURE);
            if (!worldSimulationSuppressSurfaceFeedback(simulation) && carFrameUpdateSurfaceFeedback(simulation->frames[index]) != CAR_FRAME_OK)
                return worldSimulationFailCar(simulation, index, WORLD_SIMULATION_FRAME_FAILURE);
        }
    }
    if (worldParticlesAdvance(simulation->particles, elapsed) != WORLD_SERVICES_OK)
        return simulation->result = WORLD_SIMULATION_SERVICE_FAILURE;
    for (int32_t index = 0; index < world->totalCars; ++index) {
        const float distance = worldGetDistanceToClosestPlayer3D(world, world->carTable[index]->position);
        CarMovement* movement = carFrameMovement(simulation->frames[index]);
        if (distance < 150.0f && carMotionClassActionAllowed(movement->bindings.ownerMotion) &&
            carFrameUpdateSurfaceEffects(simulation->frames[index], distance) != CAR_FRAME_OK)
            return worldSimulationFailCar(simulation, index, WORLD_SIMULATION_FRAME_FAILURE);
    }
    /* PAL14E190..14E1BC visits every car for132A70. Its complete body is
     * JR RA/NOP, so this side-effect-free pass is explicitly retired (zero count).
     * PAL12A158/1D9B60/1B52D0 are likewise exact empty tails. The separate
     * authoring editor is handled separately below: its mode3 is a live
     * Drive-camera/input producer, not an empty presentation tail. */
    if (worldEventsDispatch(simulation->events) != WORLD_SERVICES_OK)
        return simulation->result = WORLD_SIMULATION_SERVICE_FAILURE;
    worldSortCarList(world);
    carFramePlayerAdvanceAll(simulation->players);
    for (int32_t seat = 0; seat < simulation->players->count; ++seat)
        if (simulation->players->players[seat] && simulation->players->players[seat]->result)
            return simulation->result = WORLD_SIMULATION_PLAYER_FAILURE;
    if (simulation->race && raceFrameUpdate(simulation->race) != WORLD_SERVICES_OK)
        return simulation->result = WORLD_SIMULATION_SERVICE_FAILURE;
    // PAL14E1F8 dispatches the original editor, whose mode3 is the live
    // Drive camera/input producer. Other authoring modes are not admitted.
    if(simulation->editor && simulation->editor->mode==3 &&
        trackRouteEditorTickCamera(simulation->editor)!=TRACK_ROUTE_EDITOR_OK)
        return simulation->result=WORLD_SIMULATION_SERVICE_FAILURE;
    if(!worldInspectReplayState(world,simulation->clock,&simulation->stateFailure)) {
        simulation->failedCar=simulation->stateFailure.car;
        return simulation->result=WORLD_SIMULATION_NONFINITE_STATE;
    }
    simulation->failedCar = -1;
    return simulation->result;
}

// Native typed car-reference adapter; this adds no separate ELF body.
void worldSnapshotCarPointer(const World* world, ReplaySnapshot* snapshot, Car** value)
{
    int32_t number = *value != nullptr ? (*value)->driverNumber : -1;
    // PAL14D590/14D594 narrows the signed driver number to one byte.
    replaySnapshotMoveInteger(snapshot, &number, 1);
    if (snapshot->restoring && snapshot->result == REPLAY_SNAPSHOT_OK)
        *value = number < 0 ? nullptr : worldGetCarByDriverNumber(world, number);
}

/* Final PAL33028C: one mutable world/input pause gate, initial0. */
extern "C" { int32_t worldInputPauseGate = 0; }

void worldTimingInitialize(WorldTiming* world)
{
    memset(world, 0, sizeof(*world));
    world->timeScale = 1.0f;
}

void worldUpdatePause(WorldTiming* world, float elapsed)
{
    if (world->pausedSeconds > 0.0f) {
        const float remaining = world->pausedSeconds - elapsed;
        world->pausedSeconds = remaining;
        if (remaining <= 0.0f) {
            world->pausedSeconds = 0.0f;
            if (world->callback) {
                world->callback(world->callbackContext);
                world->callback = NULL;
                world->callbackContext = NULL;
            }
        }
    }
}

int worldIsPaused(const WorldTiming* world)
{
    return world->pausedSeconds > 0.0f || worldInputPauseGate != 0;
}

static WorldStepResult worldGetTimestepImpl(WorldTiming* world, const GameTime* time,
                                            const WorldStepOptions* options,
                                            const WorldStepIO* io, float* elapsed,
                                            float* recordedElapsed,bool nativeScheduling)
{
    int32_t quantized = time->realTimeQuantized;
    if (options->replaying) {
        if (!io || !io->readQuantized ||
            !io->readQuantized(io->context, 8, 1, &quantized))
            return WORLD_STEP_MISSING_REPLAY_SOURCE;
    } else if (options->recording) {
        if (!io || !io->writeQuantized)
            return WORLD_STEP_MISSING_RECORD_SINK;
        io->writeQuantized(io->context, quantized, 8, 1);
    }

    const float step = gameTimeElapsedFromQuantized(quantized);
    if (recordedElapsed)*recordedElapsed=step;
    if (!nativeScheduling && options->replaying && !options->disableReplayWaitA &&
        !options->disableReplayWaitB) {
        float wait = step - time->realTimeElapsed;
        wait = wait + world->replayWaitSeconds;
        if (wait < 0.0f) wait = 0.0f;
        if (wait > 0.029999999329447746f) wait = 0.029999999329447746f;
        world->replayWaitSeconds = wait;
        if (wait > 0.0f) {
            if (!io || !io->getTicker || !io->tickerMilliseconds)
                return WORLD_STEP_MISSING_REPLAY_CLOCK;
            const uint32_t start = io->getTicker(io->context);
            uint32_t current;
            do {
                current = io->getTicker(io->context);
            } while (io->tickerMilliseconds(io->context, start, current)
                     * 0.0010000000474974513f < wait);
        }
    }

    if (world->scaleDuration > 0.0f) {
        const float remaining = world->scaleDuration - step;
        world->scaleDuration = remaining;
        if (remaining <= 0.0f) {
            world->scaleDuration = 0.0f;
            world->timeScale = 1.0f;
            if (world->callback) {
                world->callback(world->callbackContext);
                world->callback = NULL;
                world->callbackContext = NULL;
            }
        }
    }
    *elapsed = step * world->timeScale;
    return WORLD_STEP_OK;
}

WorldStepResult worldGetTimestep(WorldTiming* world,const GameTime* time,
                                 const WorldStepOptions* options,
                                 const WorldStepIO* io,float* elapsed)
{
    return worldGetTimestepImpl(world,time,options,io,elapsed,nullptr,false);
}

WorldStepResult worldGetNativeTimestep(WorldTiming* world,const GameTime* time,
                                       const WorldStepOptions* options,
                                       const WorldStepIO* io,float* elapsed,
                                       float* recordedElapsed)
{
    return worldGetTimestepImpl(world,time,options,io,elapsed,recordedElapsed,true);
}

// The car ordering is part of the original world simulation. It determines
// the groups processed by the timestep, not merely the order of rendering.
void worldInitialize(World* world)
{
    // This is base-state initialization, not the full PAL14CD28 resource
    // constructor. In particular the list sentinel is not an owned live Car;
    // never byte-clear its C++ lifetime together with the World fields.
    world->carList.next = &world->carList;
    world->carList.previous = &world->carList;
    world->carList.ownedState = nullptr;
    for (Car*& car : world->carTable) car = nullptr;
    memset(world->carSnapshots, 0, sizeof(world->carSnapshots));
    world->totalCars = 0;
    world->firstGroupEnd = 0;
    world->secondGroupEnd = 0;
    world->thirdGroupEnd = 0;
    memset(world->playerPositions, 0, sizeof(world->playerPositions));
    memset(world->playerTrackDistances, 0, sizeof(world->playerTrackDistances));
    worldTimingInitialize(&world->timing);
    world->frameCounter = 0;
    memset(world->playerSlotFlags, 0, sizeof(world->playerSlotFlags));
    memset(world->playerSlotPosition, 0, sizeof(world->playerSlotPosition));
    world->lastPlayerSlot = 0;
    world->simulationRandomState = 0;
    for(auto*& car:world->aiCars)car=nullptr;
    world->aiCarCount=0;world->ownedState=nullptr;
}

uint32_t worldRandomBounded(World* world, uint32_t bound)
{
    return vectorRandomBounded(bound,&world->simulationRandomState);
}

float worldRandomFloat(World* world,float maximum)
{
    return vectorRandomFloat(&world->simulationRandomState,maximum);
}

void worldAddCar(World* world, Car* car)
{
    Car* last = world->carList.previous;
    car->previous = last;
    car->next = &world->carList;
    last->next = car;
    world->carList.previous = car;
}

void worldRemoveCar(Car* car)
{
    if (!car->next || !car->previous)
        return;
    car->previous->next = car->next;
    car->next->previous = car->previous;
    car->next = NULL;
    car->previous = NULL;
}

Car* worldGetCarByIndex(const World* world, int32_t index)
{
    return index >= 0 && index < world->totalCars ? world->carTable[index] : NULL;
}

float worldGetClosestSignedTrackDelta(const World* world,
                                     float reference, float trackLength)
{
    const float first = trackWrappedDelta(trackLength,
                                          world->playerTrackDistances[0], reference);
    const float second = trackWrappedDelta(trackLength,
                                           world->playerTrackDistances[1], reference);
    return fabsf(first) < fabsf(second) ? first : second;
}

static World* currentWorld;
void worldSetCurrent(World* world)
{
    currentWorld = world;
}

Car* worldFindCurrentCarByDriverNumber(int32_t driverNumber)
{
    return currentWorld ? worldGetCarByDriverNumber(currentWorld, driverNumber) : NULL;
}

uint16_t worldCalculateDebugChecksum(World* world, uint32_t seed,
                                     const void* optionalContribution)
{
    uint32_t checksum = seed;
    for (int32_t i = 0; i < world->totalCars; ++i) {
        const Car* car = world->carTable[i];
        const uint32_t contribution = (uint32_t)carCalculateDebugChecksum(car);
        memcpy(world->carSnapshots[i], car->position, sizeof(car->position));
        checksum = (checksum << 5) + checksum + contribution;
    }
    if (optionalContribution) {
        uint32_t contribution;
        memcpy(&contribution, optionalContribution, sizeof(contribution));
        checksum += contribution;
    }
    return (uint16_t)(checksum + (checksum >> 16));
}

CollisionSystemResult collisionSystemStep(CollisionSystem* system, float elapsed)
{
    (void)elapsed;
    system->result=COLLISION_SYSTEM_OK;
    if (!system->randomState) return system->result=COLLISION_SYSTEM_MISSING_OWNER;
    CollisionPair* pair=system->pairRoot.next;
    while (pair!=&system->pairRoot) {
        CollisionPair* next=pair->next;
        if (collisionPairObsolete(system,pair)) collisionPairDestroy(system,pair);
        pair=next;
    }
    for (CollisionShape* shape=system->space->root.next;shape!=&system->space->root;shape=shape->next)
        if (shape->active && shape->dirty) collisionSystemFindPairs(system,system->participants[shape->id]);
    for (pair=system->pairRoot.next;pair!=&system->pairRoot;pair=pair->next) {
        pair->processedCount=0;
        collisionPairQueueContact(system,pair);
        if (system->result!=COLLISION_SYSTEM_OK) return system->result;
    }
    system->responseIterations=0;
    while (system->contactRoot.next!=&system->contactRoot && system->responseIterations<20) {
        CollisionQueuedContact* contact=system->contactRoot.next;
        CollisionParticipant* first=contact->pair->first;
        CollisionParticipant* second=contact->pair->second;
        collisionContactResolve(system,contact);
        if (system->result!=COLLISION_SYSTEM_OK) return system->result;
        ++contact->pair->processedCount;
        if (first->shape->active) collisionQueueRemoveShape(system,first->shape);
        if (second->shape->active) collisionQueueRemoveShape(system,second->shape);
        CollisionParticipant* affected[COLLISION_ID_COUNT];
        size_t affectedCount=0;
        CollisionParticipant* owners[2]={first,second};
        for (int side=0;side<2;++side) {
            CollisionParticipant* owner=owners[side];
            if (owner->breakable && owner->breakable->breakImpulse==-1.0f)
                affectedCount+=collisionBreakableFracture(system,owner->breakable,affected+affectedCount,
                                                           COLLISION_ID_COUNT-affectedCount);
            else if (owner->shape->active && owner->shape->dirty) affected[affectedCount++]=owner;
        }
        ++system->responseIterations;
        for (size_t index=0;index<affectedCount;++index) collisionSystemFindPairs(system,affected[index]);
        for (pair=system->pairRoot.next;pair!=&system->pairRoot;pair=pair->next)
            if (pair->processedCount<2)
                for (size_t index=0;index<affectedCount;++index)
                    if (pair->first==affected[index] || pair->second==affected[index]) {
                        collisionPairQueueContact(system,pair);
                    }
        if (system->result!=COLLISION_SYSTEM_OK) return system->result;
    }
    while (system->contactRoot.next!=&system->contactRoot) collisionSystemDetachContact(system->contactRoot.next);
    // PAL14DC58..14DC8C traverses the live collision list, including
    // expanded pool blocks. Save next before the actual destructor unlinks.
    for (CollisionImpact* impact=system->firstImpact;impact;) {
        CollisionImpact* next=impact->next;
        const uint32_t bits=(uint32_t)system->nowTicks-(uint32_t)impact->lastTicks;
        int32_t ticks;memcpy(&ticks,&bits,sizeof(ticks));
        if (!((float)ticks*0.0002500000118743628f<0.3f)) collisionSystemRemoveImpact(system,impact->pair);
        impact=next;
    }
    return system->result;
}

int worldInspectReplayState(const World* world,const GameTime* clock,WorldStateFailure* failure)
{
    if(failure)*failure={-1,nullptr,-1};
    const auto reject=[failure](int32_t car,const char* field,int32_t element=-1) {
        if(failure)*failure={car,field,element};
        return 0;
    };
    if(!world)return reject(-1,"world");
    if(!clock)return reject(-1,"clock");
    if(world->totalCars<0 || world->totalCars>WORLD_MAX_CARS)return reject(-1,"totalCars");
    if(!isfinite(clock->worldTimeSeconds))return reject(-1,"clock.worldTimeSeconds");
    if(!isfinite(clock->worldTimeElapsed))return reject(-1,"clock.worldTimeElapsed");
    if(!isfinite(clock->worldTimeRemainder))return reject(-1,"clock.worldTimeRemainder");
    if(!isfinite(world->timing.pausedSeconds))return reject(-1,"timing.pausedSeconds");
    if(!isfinite(world->timing.replayWaitSeconds))return reject(-1,"timing.replayWaitSeconds");
    if(!isfinite(world->timing.timeScale))return reject(-1,"timing.timeScale");
    if(!isfinite(world->timing.scaleDuration))return reject(-1,"timing.scaleDuration");
    for(int32_t index=0;index<world->totalCars;++index) {
        const Car* car=world->carTable[index];
        if(!car)return reject(index,"car");
        if(!car->movement)return reject(index,"movement");
        const auto* movement=static_cast<const CarMovement*>(car->movement);
        const auto* state=movement->bindings.state;
        if(movement->bindings.owner!=car)return reject(index,"movement.owner");
        int32_t element=-1;
        if(const char* field=vehicleStateFirstInvalidField(state,&element))
            return reject(index,field,element);
        for(int32_t lane=0;lane<4;++lane)
            if(!isfinite(car->position[lane]))return reject(index,"car.position",lane);
    }
    return 1;
}

int worldValidateReplayState(const World* world,const GameTime* clock)
{
    return worldInspectReplayState(world,clock,nullptr);
}

WorldReplayResult worldReplaySynchronize(WorldReplay* replay, World* world)
{
    if (!replay || !world || !replay->clock || !replay->log || !replay->manager ||
        ((replay->manager->restoring || replay->manager->recording) && !replay->manager->stream))
        return WORLD_REPLAY_INVALID_BINDING;
    if(!worldValidateReplayState(world,replay->clock))return replay->result=WORLD_REPLAY_INVALID_STATE;
    const uint16_t checksum = worldCalculateDebugChecksum(world, (uint32_t)replay->clock->worldTimer,
                                                           replay->checksumContribution);
    if (replay->log->replaying)
        (void)replayIntegerLogRead(replay->log, 16, 3);
    else
        replayIntegerLogWrite(replay->log, checksum, 16, 3);
    const uint8_t synchronization = (uint8_t)(checksum + (checksum >> 8) + (uint8_t)world->simulationRandomState);
    if (replay->manager->recording) {
        replayManagerWrite(replay->manager, synchronization, 8, 2);
    } else if (replay->manager->restoring) {
        const uint8_t expected = (uint8_t)replayManagerRead(replay->manager, 8, 2);
        if(replay->manager->exhausted || replay->manager->stream->result!=BIT_STREAM_OK)
            return replay->result=WORLD_REPLAY_STREAM_FAILURE;
        auto& manager=*replay->manager;
        if(manager.synchronizationPolicy==REPLAY_SYNC_PAL_REFERENCE) {
            ++manager.sourceReferenceSamples;
            if(expected!=synchronization) {
                if(manager.sourceReferenceMismatchCount++==0) {
                    manager.firstSourceReferenceMismatchLoop=replay->clock->worldLoopCounter;
                    manager.firstSourceReferenceExpected=expected;
                    manager.firstSourceReferenceActual=synchronization;
                    char detail[224]{};
                    std::snprintf(detail,sizeof(detail),
                        "loop=%d timer=%d source_sync=%u actual_sync=%u random=%08x policy=PAL_reference trajectory_verified=0",
                        replay->clock->worldLoopCounter,replay->clock->worldTimer,
                        static_cast<unsigned>(expected),static_cast<unsigned>(synchronization),world->simulationRandomState);
                    hp2::RecordSessionEvent("replay.source_reference_mismatch",detail);
                }
            }
        } else if(manager.synchronizationPolicy==REPLAY_SYNC_NATIVE_STRICT) {
            if(expected!=synchronization) {
                if(!manager.desynchronized) {
                    char detail[160]{};
                    std::snprintf(detail,sizeof(detail),"loop=%d expected=%u actual=%u policy=native_strict",
                        replay->clock->worldLoopCounter,static_cast<unsigned>(expected),static_cast<unsigned>(synchronization));
                    hp2::RecordSessionEvent("replay.native_sync_mismatch",detail);
                }
                manager.desynchronized=1;
                if(!replay->raceActions || !raceActionsEnqueue(replay->raceActions,11,0))
                    return replay->result=WORLD_REPLAY_ACTION_CAPACITY;
            }
        } else return replay->result=WORLD_REPLAY_INVALID_BINDING;
    }
    if ((replay->log->stream && replay->log->stream->result != BIT_STREAM_OK) ||
        (replay->manager->stream && replay->manager->stream->result != BIT_STREAM_OK))
        return replay->result = WORLD_REPLAY_STREAM_FAILURE;
    return replay->result = WORLD_REPLAY_OK;
}

// World owns these resources once. Arrays are traversal views of actual
// Car owners; physics/controller/render state remains inside each Car.
struct WorldOwnedState {
    hp2::WorldConstructionBindings bindings{};
    CarMovementBindings movement{};
    PhysicsCarEnvironment physics{};
    PhysicsStepContext physicsStep{};
    CarFrameEnvironment frame{};
    CarDriverEnvironment driver{};
    RealDriverBindings real{};
    hp2::CarRenderBindings render{};
    hp2::CarConstructionBindings cars{};
    std::array<CarFrame*,WORLD_MAX_CARS> frames{};
    std::array<CarDriver*,WORLD_MAX_CARS> drivers{};
    std::array<ChopperTarget,WORLD_MAX_CARS> targets{};
    std::array<DriverCommand,3> debugTargets{};
    std::array<WorldObject*,40> debugBoxes{};
    uint32_t sourceControl19C=1;
    std::unique_ptr<TrafficScheduler,decltype(&trafficSchedulerDispose)> traffic{nullptr,trafficSchedulerDispose};
    std::unique_ptr<hp2::CarEffectsResources> effects;
    std::unique_ptr<hp2::ClanResources> clans;
    std::unique_ptr<hp2::SkidResources> skids;
    std::unique_ptr<hp2::MunitionResources> munitions;
    std::unique_ptr<FernShakerSystem,decltype(&fernShakersDispose)> ferns{nullptr,fernShakersDispose};
    std::unique_ptr<hp2::SmokeableScene> smokeables;
    TerrainEffectEmitters* terrainEmitters{};
    std::unique_ptr<PursuitManager> pursuit;
    std::unique_ptr<TrackGraph,decltype(&trackGraphDispose)> graph{nullptr,trackGraphDispose};
    std::unique_ptr<PursuitLandmarkCatalog,decltype(&pursuitLandmarksDispose)> landmarks{nullptr,pursuitLandmarksDispose};
    WorldPursuitServices pursuitServices{};
    std::unique_ptr<Chopper> chopper;
    bool resourcesClosed{};

    void CloseResources() noexcept {
        if(resourcesClosed)return;
        resourcesClosed=true;
        terrainEffectEmittersClose(&terrainEmitters);
        pursuit.reset();graph.reset();landmarks.reset();
        // EliminateAllWitnesses PAL1D6848: release the same native pair list.
        CollisionSystem* collision=bindings.cars->collision;
        while(collision->pairRoot.next!=&collision->pairRoot)
            collisionPairDestroy(collision,collision->pairRoot.next);
        smokeables.reset();ferns.reset();munitions.reset();
        clans.reset();skids.reset();effects.reset();traffic.reset();
        for(auto* object:debugBoxes)if(object)worldObjectDispose(bindings.objects,object);
        // DriveTarget dtor21B988 owns no dynamic storage; native arrays
        // release naturally, without calling an unrelated Clear method.
        collision->terrainEffects=nullptr;
    }
    ~WorldOwnedState(){chopper.reset();CloseResources();}
};

static void worldResetPhysicsStep(World* world)
{
    hp2::Require(world && world->ownedState,"World physics step has no live owner");
    world->ownedState->physicsStep.lastSubstepElapsed=0.0f;
}

World::World(const hp2::WorldConstructionBindings& b):World()
{
    static std::uint64_t nextPresentationIdentity = 0;
    presentationGeneration = ++nextPresentationIdentity;
    hp2::Require(b.clock && b.parameters && b.launch && b.simulation && b.cars && b.traffic &&
        b.cameraShake && b.scenery && b.objects && b.resources && b.sceneryObjects && b.inGameObjects &&
        b.effects && b.effects->particles && b.trackBytes && b.trackSize &&
        b.cars->movement && b.cars->physics && b.cars->frame && b.cars->driver && b.cars->render &&
        b.cars->collision && b.cars->collision->space && !b.cars->collision->objects &&
        b.cars->render->cars && b.cars->events && b.cars->topology && b.traffic->race && b.traffic->players,
        "World constructor has incomplete or already-owned loaded resources");
    hp2::Require(!currentWorld,"Only one actual live World may own the process services");
    worldInitialize(this);
    auto storage=std::make_unique<WorldOwnedState>();storage->bindings=b;
    frameCounter=1; // Final14CD28 initializes the canonical source+198 cell.
    for(auto& target:storage->debugTargets)driverCommandInitialize(&target);
    worldSetCurrent(this);gameTimeInitializeWorldTime(b.clock);
    simulationRandomState=vectorDefaultRandomBounded(0x7fffffffu);
    worldResetTimeScale(&timing);
    auto& simulation=*b.simulation;
    simulation.world=this;simulation.clock=b.clock;simulation.collisions=b.cars->collision;
    simulation.events=b.cars->events;simulation.particles=b.effects->particles;
    simulation.race=b.traffic->race;simulation.players=b.traffic->players;
    collisionSystemBindWorld(b.cars->collision,this);
    try {
        auto trafficBindings=*b.traffic;trafficBindings.world=this;trafficBindings.clock=b.clock;
        trafficBindings.parameters=b.parameters;
        storage->traffic.reset(trafficSchedulerCreate(&trafficBindings,b.cars->render->cars));
        hp2::Require(storage->traffic!=nullptr,"TrafficTeleporter requires the loaded road and skin catalogues");
        storage->effects=std::make_unique<hp2::CarEffectsResources>();
        storage->clans=std::make_unique<hp2::ClanResources>(*b.clock);
        storage->skids=std::make_unique<hp2::SkidResources>(*b.resources);
        storage->clans->BindSkids(*storage->skids);
        ChopperMunitionBindings munitions{};
        munitions.collisions=b.cars->collision;munitions.terrain=b.cars->topology;
        munitions.vehicles=storage->targets.data();munitions.players=b.traffic->players;
        munitions.replay=simulation.replay?simulation.replay->manager:b.cars->frame->replayManager;
        munitions.effects=b.effects;munitions.audio=b.audio;
        munitions.impactPresentationMode=b.impactPresentationMode;
        munitions.impactPresentationRequested=b.impactPresentationRequested;
        munitions.road=b.traffic->road;munitions.contactAudio=b.cars->collision->contactAudio;
        storage->munitions=std::make_unique<hp2::MunitionResources>(munitions,*b.inGameObjects);
        // The retained FernFlattener is distinct from the active shaker below.
        // PAL Init1D9A60/Service1D9B58 are empty, but Close1D9A68 conditionally
        // retires its list/pool. This PC graph has no flattener factory, list,
        // or pool to retire; do not introduce an unused PS2 storage replica.
        // Active FernShaker resources are owned and closed by storage->ferns.
        storage->ferns.reset(fernShakersCreate(b.scenery,b.objects,b.effects->particles,
            b.effects->groups,b.effects->groupCount,b.clock));
        hp2::Require(storage->ferns!=nullptr,"World fern-shaker allocation failed");
        carCameraShakeReset(b.cameraShake);
        storage->smokeables=std::make_unique<hp2::SmokeableScene>(*b.cars->collision,*b.sceneryObjects,
            *b.effects,*b.parameters,b.scenery,b.objects);
        hp2::Require(terrainEffectEmittersInitialize(&storage->terrainEmitters,b.effects->particles,
            b.effects->groups,b.effects->groupCount)==WORLD_SERVICES_OK,"World terrain emitter construction failed");
        hp2::Require(collisionSystemBindTerrainEffects(b.cars->collision,&storage->terrainEmitters)==COLLISION_SYSTEM_OK,
            "World collision uses a different terrain emitter owner");
        if(b.parameters->policeDriverCount>0) {
            hp2::Require(b.pursuit && b.audioRequests && b.messages && b.speech,
                "Enabled cops require actual language/speech/audio and group owners");
            hp2::PursuitConstructionBindings cops{};
            cops.world=this;cops.clock=b.clock;cops.simulation=b.simulation;cops.race=b.traffic->race;
            cops.parameters=b.parameters;cops.launch=b.launch;
            cops.audio=b.audioRequests;cops.messages=b.messages;cops.speech=b.speech;cops.groups=b.pursuit;
            cops.road=b.traffic->road;cops.terrain=b.cars->topology;cops.trackBytes=b.trackBytes;cops.trackSize=b.trackSize;
            cops.objects=b.objects;
            storage->pursuit=std::make_unique<PursuitManager>(cops);
            storage->munitions->Actors().bindings.pursuit=storage->pursuit.get();
        }
        storage->graph.reset(trackGraphCreate(b.traffic->road));
        storage->landmarks.reset(pursuitLandmarksCreate(b.trackBytes,b.trackSize));
        hp2::Require(storage->graph && storage->landmarks,"World track graph/landmarks are invalid");
        storage->pursuitServices={storage->pursuit.get(),this,storage->graph.get(),storage->landmarks.get()};
        storage->movement=*b.cars->movement;storage->physics=*b.cars->physics;
        storage->physics.stepContext=&storage->physicsStep;
        storage->frame=*b.cars->frame;storage->driver=*b.cars->driver;storage->render=*b.cars->render;
        storage->movement.pursuit=storage->pursuit?storage->pursuit->collision:nullptr;
        storage->physics.wheelTrails=&storage->skids->Trails();
        // Copy the borrow, not the countdown value: Car owns a copied
        // environment, while Race remains the sole owner of its live stage.
        storage->physics.raceStage=&b.traffic->race->state;
        storage->frame.world=this;storage->frame.clock=b.clock;storage->frame.settings=b.parameters;
        storage->frame.replayManager=simulation.replay?simulation.replay->manager:b.cars->frame->replayManager;
        storage->frame.fernShakers=storage->ferns.get();storage->frame.frames=storage->frames.data();storage->frame.frameCount=0;
        storage->driver.pursuit=storage->movement.pursuit;storage->driver.clock=b.clock;
        if(b.cars->driver->real) {
            storage->real=*b.cars->driver->real;
            storage->real.replay=storage->frame.replayManager;
            storage->real.pursuitManager=storage->pursuit.get();storage->real.chopper=nullptr;
            storage->driver.real=&storage->real;
            if(storage->real.inputRouter)storage->real.inputRouter->world=this;
        }
        storage->render.carEffects=storage->effects.get();storage->render.terrain=&storage->terrainEmitters;
        storage->cars=*b.cars;storage->cars.movement=&storage->movement;storage->cars.physics=&storage->physics;
        storage->cars.frame=&storage->frame;storage->cars.driver=&storage->driver;storage->cars.render=&storage->render;
        simulation.traffic=storage->traffic.get();simulation.pursuit=storage->pursuit.get();simulation.chopper=nullptr;
        simulation.pursuitServices=&storage->pursuitServices;
        simulation.frames=storage->frames.data();simulation.drivers=storage->drivers.data();simulation.frameCount=0;
        simulation.result=WORLD_SIMULATION_OK;simulation.failedCar=-1;
        ownedState=storage.release();
    } catch(...) {
        if(storage->real.inputRouter && storage->real.inputRouter->world==this)
            storage->real.inputRouter->world=nullptr;
        storage.reset();worldSetCurrent(nullptr);gameTimeInitializeWorldTime(b.clock);
        b.cars->collision->world=nullptr;b.cars->collision->randomState=nullptr;
        simulation.world=nullptr;simulation.frames=nullptr;simulation.drivers=nullptr;simulation.frameCount=0;
        simulation.traffic=nullptr;simulation.pursuit=nullptr;simulation.chopper=nullptr;simulation.pursuitServices=nullptr;
        throw;
    }
}

World::~World()
{
    if(!ownedState)return;
    // Normal RC lifetime has already retired all Cars. Failed/abandoned
    // construction retains the same source list and releases its real owners.
    hp2::WorldPrepareCarCleanup(*this);
    while(carList.next!=&carList) {Car* car=carList.next;worldRemoveCar(car);delete car;}
    auto* simulation=ownedState->bindings.simulation;
    if(simulation->replay)simulation->replay->checksumContribution=nullptr;
    ownedState->chopper.reset();
    auto* clock=ownedState->bindings.clock;
    auto* collision=ownedState->cars.collision;
    if(ownedState->real.inputRouter && ownedState->real.inputRouter->world==this)
        ownedState->real.inputRouter->world=nullptr;
    delete ownedState;ownedState=nullptr;
    gameTimeInitializeWorldTime(clock);worldSetCurrent(nullptr);clock->worldLoopCounter=100000;
    collision->world=nullptr;collision->randomState=nullptr;
    simulation->world=nullptr;simulation->frames=nullptr;simulation->drivers=nullptr;simulation->frameCount=0;
    simulation->traffic=nullptr;simulation->pursuit=nullptr;simulation->chopper=nullptr;simulation->pursuitServices=nullptr;
}

void worldRegisterAiCar(World* world,Car* car)
{
    hp2::Require(world && car && car->next && car->previous,"AI membership requires the linked World Car");
    for(uint32_t i=0;i<world->aiCarCount;++i)hp2::Require(world->aiCars[i]!=car,"AI Car already belongs to this World");
    hp2::Require(world->aiCarCount<8,"World AI-car membership exceeds the original racer capacity");
    world->aiCars[world->aiCarCount++]=car;
}
void worldUnregisterAiCar(World* world,Car* car)
{
    if(!world)return;
    for(uint32_t i=0;i<world->aiCarCount;++i)if(world->aiCars[i]==car) {
        for(uint32_t j=i+1;j<world->aiCarCount;++j)world->aiCars[j-1]=world->aiCars[j];
        world->aiCars[--world->aiCarCount]=nullptr;return;
    }
}

namespace hp2 {
const CarConstructionBindings& WorldCarBindings(const World& world)
{
    Require(world.ownedState,"World Car creation requires the real resource lifetime");return world.ownedState->cars;
}
Car* WorldCreateCar(World& world,RaceDriverInfo& driver,const float* position,uint16_t heading,float routeDistance)
{
    auto bindings=WorldCarBindings(world);
    Require(driver.appearance.modelCode>=0 && driver.appearance.modelCode<64,"Car selection exceeds the canonical catalogue");
    bindings.definition=&bindings.render->cars->Car(static_cast<size_t>(driver.appearance.modelCode));
    auto* loaded=carLoader.FindLoadedCar(bindings.definition->geometryCompressedPath.c_str());
    Require(loaded && loaded->loaded && loaded->file && loaded->file->Resources().collisionObjects,
        "Requested Car has no loaded final model/damage/collision resource file");
    std::vector<CollisionResource*> hulls;
    for(const auto& hull:loaded->file->Resources().collisionObjects->CollisionResources())hulls.push_back(hull.get());
    // PAL1D7F30..1D7F64 permits a missing authored *_CV resource. The
    // original CarCollisionBody constructor below owns its primitive in
    // that case; fourteen final traffic models legitimately take this path.
    bindings.collisionResources=hulls.data();bindings.collisionResourceCount=hulls.size();
    return new Car(driver,position,heading,routeDistance,bindings);
}
Car* WorldSpawnTraffic(World& world,RaceDriverInfo& driver)
{
    Require(world.ownedState,"Traffic construction requires the real World");
    Car* car=TrafficPlaceNewCar(world.ownedState->traffic.get(),driver);
    if(car)worldAddCar(&world,car);
    return car;
}
PursuitCollisionEntry* WorldCreateCop(World& world,RaceDriverInfo* driver,bool playerCop)
{
    Require(world.ownedState && world.ownedState->pursuit,"Cop creation requires the enabled real CopManager");
    return PursuitCreateCop(*world.ownedState->pursuit,driver,playerCop);
}
PursuitManager* WorldPursuitManager(World& world)
{return world.ownedState?world.ownedState->pursuit.get():nullptr;}
ClanResources& WorldClans(World& world)
{Require(world.ownedState && world.ownedState->clans,"World clans are not initialized");return *world.ownedState->clans;}
SkidResources& WorldSkids(World& world)
{Require(world.ownedState && world.ownedState->skids,"World skids are not initialized");return *world.ownedState->skids;}
WorldObjectRegistry& WorldObjects(World& world)
{
    Require(world.ownedState && !world.ownedState->resourcesClosed && world.ownedState->bindings.objects,
        "World objects have no live original World resource lifetime");
    return *world.ownedState->bindings.objects;
}
Chopper* WorldCreateChopper(World& world)
{
    Require(world.ownedState && !world.ownedState->chopper,"World already has a Chopper or has no resources");
    auto& state=*world.ownedState;const auto& b=state.bindings;
    ChopperConstructionBindings chopper{};
    chopper.environment.world=&world;chopper.environment.navigation=b.navigation;chopper.environment.road=b.traffic->road;
    chopper.environment.route=state.movement.route;chopper.environment.targets=state.targets.data();
    chopper.environment.targetCount=static_cast<size_t>(world.totalCars);
    chopper.environment.profiles=chopperAttackProfiles(&chopper.environment.profileCount);
    chopper.environment.munitions=&state.munitions->Actors();chopper.environment.particles=b.effects->particles;
    chopper.terrain=b.cars->topology;chopper.objects=b.objects;chopper.effects=b.effects;chopper.sound=b.cars->sound;
    chopper.parameters=b.parameters;
    state.chopper=std::make_unique<Chopper>(chopper);b.simulation->chopper=state.chopper.get();
    // PAL14E2B8..14E2D0 adds the live Chopper matrix translation-X word.
    // Borrow its stable matrix cell; snapshot/flight update that same owner.
    if(b.simulation->replay)b.simulation->replay->checksumContribution=&state.chopper->frame[12];
    state.real.chopper=state.chopper.get();
    for(Car* car=world.carList.next;car!=&world.carList;car=car->next)
        car->BindWorldServices(state.frame.replayManager,state.pursuit.get(),state.chopper.get());
    return state.chopper.get();
}
void WorldDestroyChopper(World& world)
{
    Require(world.ownedState,"Chopper destruction requires its World owner");
    auto& state=*world.ownedState;
    if(state.pursuit)for(size_t i=0;i<state.pursuit->groupCount;++i) {
        auto* group=state.pursuit->groups[i];
        if(group->chopper==state.chopper.get())group->chopper=nullptr;
    }
    state.real.chopper=nullptr;
    for(Car* car=world.carList.next;car!=&world.carList;car=car->next)
        car->BindWorldServices(state.frame.replayManager,state.pursuit.get(),nullptr);
    if(state.bindings.simulation->replay)state.bindings.simulation->replay->checksumContribution=nullptr;
    state.chopper.reset();state.bindings.simulation->chopper=nullptr;
}
void WorldSynchronizeCarViews(World& world)
{
    if(!world.ownedState)return;
    auto& state=*world.ownedState;
    Require(world.totalCars>=0 && world.totalCars<=WORLD_MAX_CARS,"World has an invalid car table");
    const size_t count=static_cast<size_t>(world.totalCars);
    for(size_t i=0;i<WORLD_MAX_CARS;++i) {
        Car* car=i<count?world.carTable[i]:nullptr;
        state.frames[i]=car?car->Frame():nullptr;state.drivers[i]=car?car->Driver():nullptr;
        state.targets[i]={car,car?car->Movement():nullptr,state.drivers[i]};
        if(car) {
            Require(state.frames[i] && state.drivers[i],"World table contains an incomplete Car owner");
            state.frames[i]->environment.frames=state.frames.data();state.frames[i]->environment.frameCount=count;
        }
    }
    state.frame.frameCount=count;state.bindings.simulation->frameCount=count;
    state.munitions->Actors().bindings.vehicleCount=count;
    if(state.chopper)ChopperSynchronizeTargets(*state.chopper);
}
void WorldPrepareCarCleanup(World& world)
{
    if(!world.ownedState)return;
    auto& state=*world.ownedState;
    if(state.pursuit)while(state.pursuit->groupCount)
        Require(pursuitManagerDeleteGroup(state.pursuit.get(),state.pursuit->groups[0])==WORLD_SERVICES_OK,
            "World could not retire a pursuit before its Car owner");
    for(Car* car=world.carList.next;car!=&world.carList;car=car->next)
        chopperMunitionReleaseTarget(&state.munitions->Actors(),car);
    if(state.chopper) {
        state.chopper->target=nullptr;state.chopper->environment.targetCount=0;
    }
    if(state.pursuit)for(size_t i=0;i<state.pursuit->collision->entryCount;++i)
        state.pursuit->collision->entries[i]->movement=nullptr;
}
}

// A source address is an opaque file token, never a host pointer and never
// a seat number. Unknown v8 tokens fail before publishing the live callback.
void worldSnapshotTimingCallback(WorldTiming* timing,CarFramePlayerRegistry* players,
    ReplaySnapshot* snapshot)
{
    if(!snapshot || snapshot->result!=REPLAY_SNAPSHOT_OK)return;
    if(!timing || (snapshot->wireFormat!=REPLAY_SNAPSHOT_NATIVE &&
                  snapshot->wireFormat!=REPLAY_SNAPSHOT_PAL_V8)) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
    }
    const bool authored=snapshot->wireFormat==REPLAY_SNAPSHOT_PAL_V8;
    const uint32_t knownCallback=authored?0x177C80u:1u;
    uint32_t callback=0,argument=0;
    if(!snapshot->restoring && timing->callback) {
        auto* player=static_cast<CarFramePlayerState*>(timing->callbackContext);
        if(timing->callback!=carFramePlayerCinematicTimeout || !player || !players ||
            player->movement.seat<0 || player->movement.seat>=2 ||
            players->playersByNumber[player->movement.seat]!=player) {
            snapshot->result=REPLAY_SNAPSHOT_UNRESOLVED_REFERENCE;return;
        }
        callback=knownCallback;
        const auto seat=static_cast<uint32_t>(player->movement.seat);
        argument=authored?snapshot->sourcePlayerTokens[seat]:seat;
        if(authored && !argument) {
            snapshot->result=REPLAY_SNAPSHOT_UNRESOLVED_REFERENCE;return;
        }
    } else if(!snapshot->restoring && timing->callbackContext) {
        snapshot->result=REPLAY_SNAPSHOT_UNRESOLVED_REFERENCE;return;
    }
    replaySnapshotMoveUnsigned(snapshot,&callback,4);
    replaySnapshotMoveUnsigned(snapshot,&argument,4);
    if(!snapshot->restoring || snapshot->result!=REPLAY_SNAPSHOT_OK)return;
    if(!callback) {
        if(argument) {snapshot->result=REPLAY_SNAPSHOT_UNRESOLVED_REFERENCE;return;}
        timing->callback=nullptr;timing->callbackContext=nullptr;return;
    }
    int32_t seat=-1;
    if(authored) {
        for(int32_t index=0;index<2;++index)
            if(argument && snapshot->sourcePlayerTokens[index]==argument) {
                if(seat!=-1) {snapshot->result=REPLAY_SNAPSHOT_UNRESOLVED_REFERENCE;return;}
                seat=index;
            }
    } else if(argument<2)seat=static_cast<int32_t>(argument);
    if(callback!=knownCallback || seat<0 || !players || !players->playersByNumber[seat]) {
        snapshot->result=REPLAY_SNAPSHOT_UNRESOLVED_REFERENCE;return;
    }
    timing->callback=carFramePlayerCinematicTimeout;
    timing->callbackContext=players->playersByNumber[seat];
}

// Full PAL14D148..14D334. Transient effects, concrete Cars and collision
// graphs retain their original restore order and their actual single owners.
void worldDoSnapshot(World* world,ReplaySnapshot* snapshot)
{
    hp2::Require(world&&world->ownedState&&snapshot,"World snapshot has no actual world/resource owner");
    auto& state=*world->ownedState;
    auto& timing=world->timing;
    replaySnapshotBeginSection(snapshot,nullptr,"World");
    replaySnapshotMoveUnsigned(snapshot,&world->simulationRandomState,4);
    // PAL14D190 transfers World+17C (pause) before14D19C+184 (scale).
    // The authored replay bytes use this order independently of native layout.
    replaySnapshotMoveFloat(snapshot,&timing.pausedSeconds);
    replaySnapshotMoveFloat(snapshot,&timing.timeScale);
    replaySnapshotMoveFloat(snapshot,&timing.scaleDuration);
    worldSnapshotTimingCallback(&timing,state.bindings.simulation->players,snapshot);
    replaySnapshotMoveData(snapshot,world->playerPositions[0],12,-1,0);
    replaySnapshotMoveData(snapshot,world->playerPositions[1],12,-1,0);
    replaySnapshotMoveFloat(snapshot,&world->playerTrackDistances[0]);
    replaySnapshotMoveFloat(snapshot,&world->playerTrackDistances[1]);
    replaySnapshotMoveUnsigned(snapshot,&world->frameCounter,4);
    replaySnapshotMoveUnsigned(snapshot,&state.sourceControl19C,4);
    if(snapshot->restoring) {
        state.clans->Flush();
        worldParticlesKillAllActiveEffects(state.bindings.effects->particles);
        fernShakersFlush(state.ferns.get());
        carGlareResetRequested=1;
        carCameraShakeReset(state.bindings.cameraShake);
    }
    gameTimeSnapshot(state.bindings.clock,snapshot);
    if(state.traffic)trafficSchedulerDoSnapshot(state.traffic.get(),snapshot);
    if(state.graph)trackGraphDoSnapshot(state.graph.get(),snapshot);
    for(int32_t index=0;index<world->totalCars;++index)carDoSnapshot(world->carTable[index],snapshot);
    // SnapshotCops227CD8 is exactly this nullable live-manager dispatch.
    if(state.pursuit)pursuitManagerDoSnapshot(state.pursuit.get(),snapshot);
    if(state.chopper)chopperDoSnapshot(state.chopper.get(),snapshot);
    CollisionSystem* collisions=state.bindings.cars->collision;
    if(snapshot->restoring)collisionEliminateAllWitnesses(collisions);
    state.smokeables->DoSnapshot(*snapshot,state.munitions->Actors(),*state.bindings.inGameObjects);
    collisionTerrainSnapshotEverything(collisions,state.bindings.cars->topology,snapshot);
    collisionBodiesSnapshotEverything(collisions->space,snapshot);
    collisionWitnessesSnapshotEverything(collisions,snapshot);
    replaySnapshotEndSection(snapshot);
}

/* World::DoTimestep car passes. */
static int validTable(const World* world)
{
    if (world == 0 || world->totalCars < 0 ||
        world->totalCars > WORLD_MAX_CARS ||
        world->firstGroupEnd < 0 ||
        world->secondGroupEnd < world->firstGroupEnd ||
        world->thirdGroupEnd < world->secondGroupEnd ||
        world->thirdGroupEnd > world->totalCars)
        return 0;
    for (int32_t index = 0; index < world->totalCars; ++index) {
        if (world->carTable[index] == 0)
            return 0;
    }
    return 1;
}

WorldCarOrderResult worldThinkCars(World* world, float elapsed)
{
    if (!validTable(world))
        return WORLD_CAR_ORDER_INVALID_TABLE;
    // No partial progression when a required original subsystem is absent.
    for (int32_t index = 0; index < world->totalCars; ++index) {
        const Car* car = world->carTable[index];
        if (!car->interaction || !car->controller || !car->controllerThink)
            return WORLD_CAR_ORDER_MISSING_COMPONENT;
    }

    // The timestep prioritizes these ranges in a different order from
    // the car table's construction. Preserve that distinction.
    const int32_t first[] = {
        world->firstGroupEnd, world->thirdGroupEnd,
        0, world->secondGroupEnd
    };
    const int32_t last[] = {
        world->secondGroupEnd, world->totalCars,
        world->firstGroupEnd, world->thirdGroupEnd
    };
    for (int pass = 0; pass != 4; ++pass) {
        for (int32_t index = first[pass]; index < last[pass]; ++index)
            (void)carThink(world->carTable[index], elapsed);
    }
    return WORLD_CAR_ORDER_OK;
}

WorldCarOrderResult worldDriveCars(World* world, float elapsed)
{
    if (!validTable(world))
        return WORLD_CAR_ORDER_INVALID_TABLE;
    for (int32_t index = 0; index < world->totalCars; ++index) {
        const Car* car = world->carTable[index];
        if (!car->controls || !car->updateControls)
            return WORLD_CAR_ORDER_MISSING_COMPONENT;
    }
    for (int32_t index = 0; index < world->totalCars; ++index)
        (void)carDrive(world->carTable[index], elapsed);
    return WORLD_CAR_ORDER_OK;
}
