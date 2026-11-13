#include "indep/inc/Munitions.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include <string.h>
#include "indep/inc/AICarController.hpp"
#include "indep/inc/CarDamage.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/Race.hpp"
#include "TrackedController.hpp"
#include <math.h>
#include "indep/inc/Collision.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/Replay.hpp"
#include "bware/inc/bMath.hpp"
#include <new>

struct ChopperMunitionPoolBlock {
    ChopperMunitionPoolBlock* next;
    ChopperMunition* actors;
    size_t count;
};

namespace {
constexpr size_t initialMunitionCounts[3]={CHOPPER_ROCKET_CAPACITY,
    CHOPPER_SMOKE_BOMB_CAPACITY,CHOPPER_SPIKE_BOMB_CAPACITY};

void resetMunitionPools(ChopperMunitionRegistry* registry)
{
    registry->initialUsed=0;
    for(size_t kind=0;kind<3;++kind) {
        registry->initialAssigned[kind]=0;
        registry->poolCapacity[kind]=initialMunitionCounts[kind];
        registry->freeLists[kind]=nullptr;
    }
    registry->expanded=nullptr;
}

bool addMunitionBlock(ChopperMunitionRegistry* registry,ChopperMunitionKind kind,size_t count)
{
    auto* block=new(std::nothrow) ChopperMunitionPoolBlock{};
    if(!block)return false;
    block->actors=new(std::nothrow) ChopperMunition[count]{};
    if(!block->actors) {delete block;return false;}
    block->count=count;
    block->next=registry->expanded;
    registry->expanded=block;
    // Same forward-address allocation order as the source expansion. Existing
    // actor and collision pointers remain valid through later block growth.
    for(size_t index=count;index>0;--index) {
        auto* actor=&block->actors[index-1];
        actor->nextFree=registry->freeLists[kind];
        registry->freeLists[kind]=actor;
    }
    return true;
}

ChopperMunition* allocateMunition(ChopperMunitionRegistry* registry,ChopperMunitionKind kind)
{
    if(!registry->freeLists[kind]) {
        if(registry->initialAssigned[kind]<initialMunitionCounts[kind]) {
            // The native caller supplies a shared initial arena; its slots are
            // assigned once to a concrete source pool and never migrate types.
            if(registry->initialUsed<registry->capacity) {
                ++registry->initialAssigned[kind];
                return &registry->actors[registry->initialUsed++];
            }
            // A caller may supply only part of the initial native storage.
            // Allocate its remainder before expanding the source pool.
            const size_t remainder=initialMunitionCounts[kind]-registry->initialAssigned[kind];
            if(!addMunitionBlock(registry,kind,remainder))return nullptr;
            registry->initialAssigned[kind]+=remainder;
        } else {
            // InitRocket/InitSmokeBomb/InitDroppedSpikes + bOMalloc:
            // SlotPool::Malloc258130 calls ExpandSlotPool(initial_count/4+1).
            const size_t growth=initialMunitionCounts[kind]/4+1;
            if(!addMunitionBlock(registry,kind,growth))return nullptr;
            registry->poolCapacity[kind]+=growth;
        }
    }
    auto* actor=registry->freeLists[kind];
    registry->freeLists[kind]=actor->nextFree;
    return actor;
}

void releaseMunitionSlot(ChopperMunition* actor)
{
    actor->physical=nullptr;
    actor->target=nullptr;
    actor->occupied=0;
    actor->nextFree=actor->registry->freeLists[actor->kind];
    actor->registry->freeLists[actor->kind]=actor;
}
}

void chopperMunitionRegistryInitialize(ChopperMunitionRegistry* registry,
    ChopperMunition* actors, size_t capacity, const ChopperMunitionBindings* bindings)
{
    memset(registry, 0, sizeof(*registry));
    registry->actors = actors;
    registry->capacity = capacity;
    if (!bindings || (!actors && capacity) || !bindings->collisions || !bindings->terrain ||
        !bindings->rocketDefinition || !bindings->smokeDefinition) {
        registry->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    registry->bindings = *bindings;
    if (capacity) memset(actors, 0, capacity * sizeof(*actors));
    resetMunitionPools(registry);
}

ChopperMunition* chopperMunitionConstruct(ChopperMunitionRegistry* registry,ChopperMunitionKind kind,
    int32_t bodyId,const CollisionBreakableDefinition* definition,uint32_t model)
{
    if (!registry || registry->result != WORLD_SERVICES_OK || !definition ||
        kind<CHOPPER_MUNITION_ROCKET || kind>CHOPPER_MUNITION_SPIKE_BOMB) return NULL;
    ChopperMunition* actor=allocateMunition(registry,kind);
    if (!actor) {
        registry->result = WORLD_SERVICES_CAPACITY;
        return NULL;
    }
    memset(actor, 0, sizeof(*actor));
    actor->registry = registry;
    actor->kind = kind;
    CollisionBreakable* physical = collisionBreakableConstruct(registry->bindings.collisions,
        &actor->physicalStorage, definition, registry->bindings.terrain, model,bodyId);
    if (!physical) {
        registry->result = registry->bindings.collisions->result == COLLISION_SYSTEM_OBJECT_CAPACITY ?
            WORLD_SERVICES_CAPACITY : WORLD_SERVICES_OWNER_FAILURE;
        releaseMunitionSlot(actor);
        return NULL;
    }
    actor->physical = physical;
    actor->occupied = 1;
    physical->munition = actor;
    return actor;
}

ChopperMunition* chopperRocketCreate(ChopperMunitionRegistry* registry)
{
    if(!registry)return nullptr;
    return chopperMunitionConstruct(registry,CHOPPER_MUNITION_ROCKET,-1,
        registry->bindings.rocketDefinition,hashString("ROCKET"));
}

ChopperMunition* chopperSmokeBombCreate(ChopperMunitionRegistry* registry)
{
    if(!registry)return nullptr;
    return chopperMunitionConstruct(registry,CHOPPER_MUNITION_SMOKE_BOMB,-1,
        registry->bindings.smokeDefinition,hashString("SMOKEBOMB"));
}

ChopperMunition* chopperSpikeBombCreate(ChopperMunitionRegistry* registry)
{
    if(!registry)return nullptr;
    return chopperMunitionConstruct(registry,CHOPPER_MUNITION_SPIKE_BOMB,-1,
        registry->bindings.smokeDefinition,hashString("SMOKEBOMB"));
}

void chopperMunitionSetTransform(ChopperMunition* actor, const float frame[16], const float velocity[4])
{
    collisionBreakableSetTransform(actor->physical, frame, velocity);
    collisionBreakableWake(actor->physical);
    collisionBreakableStartActivationEffects(actor->physical);
}

void chopperRocketLaunch(ChopperMunition* actor, const float frame[16],
    const float velocity[4], ChopperTarget* target)
{
    chopperMunitionSetTransform(actor, frame, velocity);
    actor->heading = chopperWrapDegrees((float)vectorAtan2Angle(velocity[0], velocity[1]) * 0.0054931640625f);
    actor->target = target;
}

void chopperMunitionBuildFrame(const ChopperMunition* actor, float output[16])
{
    rotationMatrixIdentity(output);
    memcpy(output + 12, actor->physical->body.motion.position, 4 * sizeof(float));
    output[15] = 1.0f;
}

void chopperMunitionApplyStep(ChopperMunition* actor, float rewind)
{
    actor->skipCapture = 1;
    collisionShapeApplyStep(&actor->physical->shape, rewind);
}

float chopperRocketImpulseFraction(ChopperMunition* actor)
{
    actor->target = NULL;
    return -57575.0f;
}

void chopperRocketApplyImpulse(ChopperMunition* actor)
{
    actor->target = NULL;
}

float chopperSpikeImpulseFraction(ChopperMunition* actor)
{
    actor->triggered = 1;
    return 1.0f;
}

void chopperSpikeApplyImpulse(ChopperMunition* actor)
{
    actor->triggered = 1;
}

void chopperMunitionDestroy(ChopperMunition* actor)
{
    if (!actor || !actor->occupied) return;
    CollisionBreakable* physical = actor->physical;
    if (physical && physical->occupied)
        collisionBreakableDestroy(actor->registry->bindings.collisions, physical);
    // The base destructor normally performs this retirement through its
    // concrete munition backlink. Never add a slot twice to the free list.
    if(actor->occupied)releaseMunitionSlot(actor);
}

void chopperMunitionPhysicalDestroyed(ChopperMunition* actor, CollisionBreakable* physical)
{
    if (!actor || !actor->occupied || actor->physical != physical) return;
    releaseMunitionSlot(actor);
}

void chopperMunitionRegistryDispose(ChopperMunitionRegistry* registry)
{
    if (!registry || (!registry->actors && registry->capacity)) return;
    for (size_t index = 0; index < registry->capacity; ++index)
        chopperMunitionDestroy(&registry->actors[index]);
    for(auto* block=registry->expanded;block;block=block->next)
        for(size_t index=0;index<block->count;++index)
            chopperMunitionDestroy(&block->actors[index]);
    chopperEffectsReleaseOwner(registry->bindings.effects,registry);
    while(registry->expanded) {
        auto* block=registry->expanded;
        registry->expanded=block->next;
        delete[] block->actors;
        delete block;
    }
    resetMunitionPools(registry);
}

void chopperMunitionReleaseTarget(ChopperMunitionRegistry* registry, const Car* car)
{
    for (size_t index = 0; index < registry->capacity; ++index) {
        ChopperMunition* actor = &registry->actors[index];
        if (actor->occupied && actor->target && actor->target->car == car) actor->target = NULL;
    }
    for(auto* block=registry->expanded;block;block=block->next)
        for(size_t index=0;index<block->count;++index) {
            auto* actor=&block->actors[index];
            if(actor->occupied && actor->target && actor->target->car==car)actor->target=nullptr;
        }
}

// Complete Rocket.DoSnapshot1BF910..1BF97C and
// DroppedSpikes.DoSnapshot1BEF90..1BEFF0. SmokeBomb inherits Smokeable's
// actual virtual slot; it has no additional fabricated snapshot payload.
void chopperMunitionDoSnapshot(ChopperMunition* actor,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!actor || !actor->occupied || !actor->physical || !actor->registry) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
    }
    if(actor->kind==CHOPPER_MUNITION_SMOKE_BOMB) {
        collisionBreakableDoSnapshot(actor->physical,snapshot);return;
    }
    replaySnapshotBeginSection(snapshot,actor,
        actor->kind==CHOPPER_MUNITION_ROCKET?"Rocket":"DroppedSpikes");
    if(actor->kind==CHOPPER_MUNITION_ROCKET) {
        replaySnapshotMoveFloat(snapshot,&actor->heading);
        const auto& bindings=actor->registry->bindings;
        Car* target=actor->target?actor->target->car:nullptr;
        worldSnapshotCarPointer(bindings.collisions->world,snapshot,&target);
        if(snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
            actor->target=nullptr;
            for(size_t index=0;index<bindings.vehicleCount;++index)
                if(bindings.vehicles[index].car==target && target)actor->target=&bindings.vehicles[index];
            if(target && !actor->target)snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;
        }
    } else replaySnapshotMoveData(snapshot,&actor->triggered,1,4,0);
    collisionBreakableDoSnapshot(actor->physical,snapshot);
    replaySnapshotEndSection(snapshot);
}

const ChopperExplosionTuning* chopperMunitionExplosionTuning(ChopperMunitionKind kind)
{
    static const ChopperExplosionTuning rocket = {20,20,10,8,0,6,0};
    static const ChopperExplosionTuning smoke = {25,10,10,5,3,6,1};
    static const ChopperExplosionTuning spikes = {1,0,10,5,2,8,0};
    return kind == CHOPPER_MUNITION_ROCKET ? &rocket :
        kind == CHOPPER_MUNITION_SMOKE_BOMB ? &smoke : &spikes;
}

static ChopperTarget* vehicle(ChopperMunitionRegistry* registry, const Car* car)
{
    for (size_t index = 0; index < registry->bindings.vehicleCount; ++index)
        if (registry->bindings.vehicles[index].car == car) return &registry->bindings.vehicles[index];
    return NULL;
}

static CarFramePlayerState* player(const CarFramePlayerRegistry* players, const Car* car)
{
    for (int32_t index = 0; index < players->count; ++index) {
        CarFramePlayerState* value = players->players[index];
        if (value && value->car && value->car->bindings.owner == car) return value;
    }
    return NULL;
}

void chopperMunitionExplode(ChopperMunition* actor, const ChopperExplosionTuning* tuning,
    int32_t reactionKind)
{
    ChopperMunitionRegistry* registry = actor->registry;
    const ChopperMunitionBindings* bindings = &registry->bindings;
    CollisionSystem* collisions = bindings->collisions;
    const World* world = collisions->world;
    if (!world || !bindings->players || !bindings->audio ||
        (bindings->vehicleCount && !bindings->vehicles) || world->totalCars < 0 || world->totalCars > WORLD_MAX_CARS) {
        registry->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    const float* origin = actor->physical->body.motion.position;
    const float lever[4] = {0, 0, tuning->leverHeight, 0};
    for (int32_t index = 0; index < world->totalCars; ++index) {
        Car* car = world->carTable[index];
        ChopperTarget* binding = vehicle(registry, car);
        if (!binding || !binding->movement || !binding->movement->bindings.state ||
            !binding->movement->bindings.ownerMotion || !binding->driver) {
            registry->result = WORLD_SERVICES_INVALID_BINDING;
            return;
        }
        CarMovement* movement = binding->movement;
        if (!movement->mode) continue;
        const float x = car->position[0] - origin[0];
        const float y = car->position[1] - origin[1];
        const float yy = y * y, xx = x * x;
        const float distance = sqrtf(xx + yy);
        if (!(distance < tuning->outerRadius)) continue;
        float strength;
        if (distance < tuning->innerRadius) strength = tuning->innerImpulse;
        else {
            const float remaining = tuning->outerRadius - distance;
            const float strengthRange = tuning->innerImpulse - tuning->outerImpulse;
            const float radiusRange = tuning->outerRadius - tuning->innerRadius;
            strength = tuning->outerImpulse + (strengthRange * remaining) / radiusRange;
        }
        float impulse[4] = {x, y, 0, car->position[3] - origin[3]};
        vectorSetLength3(impulse, impulse, strength);
        if (movement->bindings.state->activeContactCount > 0) impulse[2] += tuning->contactLift;
        if (!movement->collisionParticipant) {
            registry->result = WORLD_SERVICES_INVALID_BINDING;
            return;
        }
        collisionParticipantNotify(movement->collisionParticipant, NULL);
        if (collisions->result != COLLISION_SYSTEM_OK) {
            registry->result = WORLD_SERVICES_OWNER_FAILURE;
            return;
        }
        collisionParticipantApplyImpulse(collisions, movement->collisionParticipant, impulse, lever);
        if (collisions->result != COLLISION_SYSTEM_OK) {
            registry->result = WORLD_SERVICES_OWNER_FAILURE;
            return;
        }
        CarFramePlayerState* human = player(bindings->players, car);
        const int32_t category = movement->bindings.ownerMotion->inputCategory;
        if (human) {
            if (reactionKind == 1) {
                if (!bindings->impactPresentationMode) {
                    registry->result = WORLD_SERVICES_INVALID_BINDING;
                    return;
                }
                if (*bindings->impactPresentationMode == 1) {
                    if (!bindings->impactPresentationRequested) {
                        registry->result = WORLD_SERVICES_INVALID_BINDING;
                        return;
                    }
                    *bindings->impactPresentationRequested = 1;
                }
            }
            if (!(tuning->innerImpulse * 0.800000011920929f < strength)) continue;
            if (movement->bindings.damage) carDamageSetFull(movement->bindings.damage);
            if (category == 1) {
                if (binding->driver->kind != CAR_DRIVER_REAL || !binding->driver->owner.real) {
                    registry->result = WORLD_SERVICES_INVALID_BINDING;
                    return;
                }
                realDriverSetHandbrakeTime(&binding->driver->owner.real->input, 3.5f);
            }
            if (tuning->lockPlayer) {
                if (!car->raceEntry) {
                    registry->result = WORLD_SERVICES_INVALID_BINDING;
                    return;
                }
                if (!car->raceEntry->completionReason) {
                    (void)carFramePlayerStartState(human, 1, 0, 1, 1);
                    if (human->result != CAR_FRAME_OK) {
                        registry->result = WORLD_SERVICES_OWNER_FAILURE;
                        return;
                    }
                }
            }
        } else if (category == 2 && (movement->mode == 1 || movement->mode == 4)) {
            if (binding->driver->kind != CAR_DRIVER_AI || !binding->driver->owner.ai) {
                registry->result = WORLD_SERVICES_INVALID_BINDING;
                return;
            }
            aiDriverSetHandbrakeTime(binding->driver->owner.ai, 3.5f);
        }
    }
    if (worldAudioPlayBomb(bindings->audio, origin) != WORLD_AUDIO_OK)
        registry->result = WORLD_SERVICES_OWNER_FAILURE;
}

static void placeSpikes(ChopperMunition* actor)
{
    ChopperMunitionRegistry* registry = actor->registry;
    const ChopperMunitionBindings* bindings = &registry->bindings;
    const float* point = actor->physical->body.motion.position;
    bool found;
    const float height = terrainSurfaceElevation(&actor->physical->terrain, point, NULL, NULL, &found);
    if (!found || 1.0f < point[2] - height) return;
    if (!bindings->road) { registry->result = WORLD_SERVICES_INVALID_BINDING; return; }
    CarRecoverySelection selection;
    if (!carRecoverySelectPose(&selection, bindings->road, point, 0, 2, 0.0f)) return;
    const CarRecoveryPose* pose = selection.pose;
    const float first = (point[0] - pose->position[0]) * pose->direction[1];
    const float second = (point[1] - pose->position[1]) * pose->direction[0];
    const float lateral = first - second;
    const float margin = lateral > 0 ? pose->halfWidths[1] - lateral : lateral + pose->halfWidths[0];
    if (margin < 0.0f) return;
    float positive = 2.0f, negative = 2.0f;
    if (lateral > 0) positive = margin < positive ? margin : positive;
    else negative = margin < negative ? margin : negative;
    const float across[2] = {pose->direction[1], -pose->direction[0]};
    float ends[2][2];
    vectorMultiplyAdd2(ends[1], point, across, positive);
    vectorMultiplyAdd2(ends[0], point, across, -negative);
    if (!bindings->pursuit || !bindings->pursuit->spikes) {
        registry->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    if (!pursuitSpikeCreate(bindings->pursuit->spikes, ends[0], ends[1], -1, 20.0f))
        registry->result = WORLD_SERVICES_OWNER_FAILURE;
}

void chopperMunitionDetonate(ChopperMunition* actor)
{
    ChopperMunitionRegistry* registry = actor->registry;
    if (!registry->bindings.effects) return;
    float frame[16];
    chopperMunitionBuildFrame(actor, frame);
    // Final1BEFF0 initializes dropped-spike prototypes with ROCKETEXPLD.
    // The earlier hand-seeded fixture used the beta's smoke group here.
    const uint32_t group = hashString(actor->kind == CHOPPER_MUNITION_SMOKE_BOMB ? "SMOKEBOMBEXPLD" : "ROCKETEXPLD");
    ChopperMunitionEffects* effects=registry->bindings.effects;
    ChopperEffectInstance* previousFirst=effects->first;
    const auto visualResult = chopperEffectsSpawnGroup(effects, group, frame);
    // Both members of an authored detonation clone carry the registry owner,
    // not the rocket's ephemeral actor (destroyed later in the same tick).
    for (ChopperEffectInstance* instance=effects->first;instance!=previousFirst;instance=instance->next)
        instance->lifetimeOwner=registry;
    if (visualResult != WORLD_SERVICES_OK && visualResult != WORLD_SERVICES_CAPACITY) {
        registry->result = visualResult;
        return;
    }
    // The fixed original graphics pool can exhaust. Losing a transient FX
    // slot must not cancel the actual collision impulses, player damage,
    // controls or planted spike field. Only an invalid catalogue is fatal.
    const int32_t reaction = actor->kind == CHOPPER_MUNITION_ROCKET ? 1 :
        actor->kind == CHOPPER_MUNITION_SPIKE_BOMB ? 2 : 0;
    registry->result = WORLD_SERVICES_OK;
    chopperMunitionExplode(actor, chopperMunitionExplosionTuning(actor->kind), reaction);
    if (registry->result == WORLD_SERVICES_OK && actor->kind == CHOPPER_MUNITION_SPIKE_BOMB)placeSpikes(actor);
    if (visualResult == WORLD_SERVICES_CAPACITY)++effects->droppedRequests;
}

static void detonateAndDestroy(ChopperMunition* actor)
{
    chopperMunitionDetonate(actor);
    chopperMunitionDestroy(actor);
}

void chopperRocketAdvance(ChopperMunition* actor, float elapsed)
{
    if (actor->skipCapture) {
        actor->skipCapture = 0;
        return;
    }
    if (!actor->target) {
        detonateAndDestroy(actor);
        return;
    }
    CollisionBreakable* object = actor->physical;
    RigidBody* body = &object->body;
    collisionShapeCapture(&object->shape, elapsed);
    const float* position = body->motion.position;
    const float height = terrainSurfaceElevation(&object->terrain, position, NULL, NULL, NULL);
    // PAL1BF694..1BF7D4 retains separate EE/VU operation boundaries. Host
    // rounding changes the homing heading, then velocity, display and bounds
    // even when the actual terrain and the original angle table agree.
    const float desiredHeight = bScalarAdd(height, 0.8999999761581421f);
    const float targetX = bScalarAdd(actor->target->car->position[0], -position[0]);
    const float targetY = bScalarAdd(actor->target->car->position[1], -position[1]);
    const float desiredHeading = chopperWrapDegrees(bScalarMultiply((float)vectorAtan2Angle(targetX, targetY), 0.0054931640625f));
    const float difference = chopperWrapDegrees(bScalarAdd(desiredHeading, -actor->heading));
    const float limit = bScalarMultiply(elapsed, 90.0f);
    const float turn = minimumFinite(maximumFinite(-limit, difference), limit);
    actor->heading = bScalarAdd(actor->heading, turn);
    const uint16_t angle = (uint16_t)(convertFloatToInt32(bScalarMultiply(-actor->heading, 65536.0f)) / 360);
    float* velocity = body->linearVelocity;
    velocity[0] = bScalarMultiply(vectorCosineAngle(angle), 30.0f);
    velocity[1] = bScalarMultiply(-rotationSineAngle(angle), 30.0f);
    const float heightError = bScalarAdd(desiredHeight, -position[2]);
    velocity[2] = minimumFinite(maximumFinite(bScalarMultiply(heightError, 12.0f), -25.0f), 25.0f);
    float nextPosition[4];
    for (int lane = 0; lane < 4; ++lane) {
        const float change = bScalarMultiply(velocity[lane], elapsed);
        nextPosition[lane] = bScalarAdd(change, position[lane]);
    }
    float frame[16];
    rotationMatrixIdentity(frame);
    motionMatrixRotateY(frame, frame, vectorAtan2Angle(velocity[2], 30.0f));
    const uint16_t yaw = (uint16_t)(convertFloatToInt32(bScalarMultiply(actor->heading, 65536.0f)) / 360);
    motionMatrixRotateYaw(frame, frame, yaw);
    const float angularVelocity[4] = {0, 0, 0, 0};
    rigidBodySetMotion(body, nextPosition, frame, velocity, angularVelocity);
    memcpy(frame + 12, nextPosition, 3 * sizeof(float));
    memcpy(object->displayFrame, frame, sizeof(frame));
    object->displayChanged = 1;
    collisionBreakableUpdateTerrain(object);
    collisionBreakableUpdateEffects(object, frame);
    collisionBreakableUpdateLifetime(actor->registry->bindings.collisions, object);
}

void chopperSmokeBombAdvance(ChopperMunition* actor, float elapsed)
{
    CollisionSystem* system = actor->registry->bindings.collisions;
    CollisionBreakable* object = actor->physical;
    if (!actor->skipCapture) {
        for (int32_t index = 0; index < object->shape.contactCount; ++index) {
            const CollisionImpact* impact = collisionShapeContactAt(system, &object->shape, index);
            Car* first;
            Car* second;
            collisionImpactGetCars(impact, &first, &second);
            if (!first && !second) continue;
            if (!impact->normalSound) {
                if (!actor->registry->bindings.contactAudio ||
                    worldImpactAudioPlay(actor->registry->bindings.contactAudio, impact->firstPoint) != WORLD_AUDIO_OK) {
                    actor->registry->result = WORLD_SERVICES_OWNER_FAILURE;
                    return;
                }
            }
            detonateAndDestroy(actor);
            return;
        }
    }
    actor->skipCapture = 0;
    collisionBreakableAdvance(system, object, elapsed);
}

void chopperSpikeBombAdvance(ChopperMunition* actor, float elapsed)
{
    if (!actor->skipCapture && actor->triggered) {
        detonateAndDestroy(actor);
        return;
    }
    actor->skipCapture = 0;
    collisionBreakableAdvance(actor->registry->bindings.collisions, actor->physical, elapsed);
}

void chopperMunitionAdvance(ChopperMunition* actor, float elapsed)
{
    switch (actor->kind) {
    case CHOPPER_MUNITION_ROCKET: chopperRocketAdvance(actor, elapsed); break;
    case CHOPPER_MUNITION_SMOKE_BOMB: chopperSmokeBombAdvance(actor, elapsed); break;
    case CHOPPER_MUNITION_SPIKE_BOMB: chopperSpikeBombAdvance(actor, elapsed); break;
    }
}

#include "indep/inc/SmokeableInfo.hpp"
#include "bware/inc/bWare.hpp"
#include <stdexcept>

namespace hp2 {
MunitionResources::MunitionResources(const ChopperMunitionBindings& bindings,
    const CollisionObjectData& resources)
{
    // InitMunitions precedes SpawnSmokeables in the actual World constructor.
    // No actor is created here; its later allocation requires the registered
    // Smokeable pool in collisionBreakableConstruct, never a private pool.
    Require(bindings.collisions && bindings.terrain &&
        bindings.effects && bindings.effects->particles,
        "Munitions require the live collision, terrain and AcidFX owners");
    ChopperMunitionBindings actual=bindings;
    actual.rocketDefinition=resources.Find(hashString("CS_Rocket"));
    actual.smokeDefinition=resources.Find(hashString("CS_SMOKEBOMB"));
    Require(actual.rocketDefinition && actual.smokeDefinition,
        "Final INGAMEB munition physical definitions are missing");
    const char* names[3]={"ROCKETEXPLD","SMOKEBOMBEXPLD","ROCKETEXPLD"};
    for (std::size_t kind=0;kind<detonationGroups_.size();++kind) {
        const uint32_t key=hashString(names[kind]);
        for (std::size_t group=0;group<actual.effects->groupCount;++group)
            if (actual.effects->groups[group].key==key) {
                detonationGroups_[kind]=&actual.effects->groups[group];break;
            }
        Require(detonationGroups_[kind]!=nullptr,
            "Final munition detonation definition is absent from the live effect catalogue");
    }
    chopperMunitionRegistryInitialize(&registry_,actors_.data(),actors_.size(),&actual);
    Require(registry_.result==WORLD_SERVICES_OK,"Munition actor registry rejected its real owners");
}

MunitionResources::~MunitionResources()
{
    chopperMunitionRegistryDispose(&registry_);
}

const ChopperEffectGroup& MunitionResources::DetonationGroup(ChopperMunitionKind kind) const
{
    if (kind<CHOPPER_MUNITION_ROCKET || kind>CHOPPER_MUNITION_SPIKE_BOMB)
        throw std::out_of_range("Munition kind");
    return *detonationGroups_[static_cast<std::size_t>(kind)];
}
}
