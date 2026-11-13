#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/CarDamage.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Munitions.hpp"
#include "indep/inc/CollisionBody.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/VoronoiClip.hpp"
#include "indep/inc/Smokeable.hpp"
#include "indep/inc/Contact.hpp"
#include "indep/inc/Collision.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/World.hpp"

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <new>

// PAL InitCollision21E5C8 creates an expandable32-slot bSlotPool.
// Malloc258158..258178 expands by (32>>2)+1 without moving live slots.
struct CollisionImpactBlock {
    CollisionImpactBlock* next{};
    CollisionImpact impacts[(32>>2)+1]{};
};

CollisionShape* collisionBodyById(const CollisionSpace* space, int16_t id)
{
    if (!space || id < 0 || id >= COLLISION_ID_COUNT) return nullptr;
    return space->byId[id];
}

void collisionIdMark(CollisionIdPool* pool, int32_t id)
{
    if (pool->nextFree == id) {
        int32_t candidate = id + 1;
        while (candidate < pool->size && (pool->occupied[candidate >> 3] & (1u << (candidate & 7)))) ++candidate;
        pool->nextFree = candidate;
    }
    pool->occupied[id >> 3] |= (uint8_t)(1u << (id & 7));
}

void collisionIdRelease(CollisionIdPool* pool, int32_t id)
{
    pool->occupied[id >> 3] &= (uint8_t)~(1u << (id & 7));
    if (id < pool->nextFree) pool->nextFree = id;
}

int32_t collisionIdAcquire(CollisionIdPool* pool)
{
    const int32_t id = pool->nextFree;
    if (id >= pool->size) return -1;
    collisionIdMark(pool, id);
    return id;
}

int32_t collisionShapeCompare(const CollisionShape* first, const CollisionShape* second)
{
    if (second->maximum[0] < first->maximum[0]) return 1;
    if (first->maximum[0] < second->maximum[0]) return 0;
    return second->id < first->id;
}

void collisionShapeLock(CollisionShape* shape) { shape->locks = (uint8_t)(shape->locks + 1u); }
void collisionShapeUnlock(CollisionShape* shape) { if (shape->locks) --shape->locks; }

void collisionShapeCopyVelocityPair(const CollisionShape* shape, float linear[4], float angular[4])
{
    memcpy(linear, shape->body->linearVelocity, 3 * sizeof(float));
    memcpy(angular, shape->body->angularVelocity, 3 * sizeof(float));
    linear[3]=angular[3]=0.0f;
}

int32_t collisionShapeOverlapsBounds(const CollisionShape* shape, const float minimum[2], const float maximum[2], float margin)
{
    return shape->minimum[0] <= maximum[0] + margin && minimum[0] - margin <= shape->maximum[0]
        && shape->minimum[1] <= maximum[1] + margin && minimum[1] - margin <= shape->maximum[1];
}

int32_t collisionShapesOverlap(const CollisionShape* first, const CollisionShape* second, float margin)
{
    return first->minimum[0] <= second->maximum[0] + margin && second->minimum[0] - margin <= first->maximum[0]
        && first->minimum[1] <= second->maximum[1] + margin && second->minimum[1] - margin <= first->maximum[1];
}

void collisionParticipantRefresh(CollisionParticipant* participant)
{
    CollisionShape* shape=participant->shape;
    if (participant->movement) {
        shape->body=shape->bodyMode ? carMovementCollisionBody(participant->movement) : NULL;
        VehicleState* state=carMovementState(participant->movement);
        shape->vehicle.position=state->position;
        shape->vehicle.velocity=state->velocity;
        shape->vehicle.orientation=state->orientation;
    } else if (participant->breakable) shape->body=&participant->breakable->body;
}

int32_t collisionParticipantYawSensitive(const CollisionParticipant* participant)
{
    const int kind=participant->shape->kind;
    if (kind<0) return 0;
    if (kind<3) return 1;
    return kind==3 && (participant->polygon->flags&2)!=0;
}

static float* positionOf(const CollisionShape* shape)
{
    return shape->bodyMode ? shape->body->motion.position : shape->vehicle.position;
}
static const float* velocityOf(const CollisionShape* shape)
{
    return shape->bodyMode ? shape->body->linearVelocity : shape->vehicle.velocity;
}
static const float* frameOf(const CollisionShape* shape)
{
    return shape->bodyMode ? shape->body->motion.orientation : shape->vehicle.orientation;
}

void collisionSpaceInitialize(CollisionSpace* space)
{
    memset(space, 0, sizeof(*space));
    space->root.next = space->root.previous = &space->root;
    space->ids.size = COLLISION_ID_COUNT;
    space->ids.occupied = space->occupied;
}

uint32_t collisionPairMask(const CollisionSpace* space, const CollisionShape* first, const CollisionShape* second)
{
    if (first->id < 0 || second->id < 0) return 0;
    const int32_t bit = first->id * COLLISION_ID_COUNT + second->id;
    return space->pairBits[bit >> 3] & (1u << (bit & 7));
}

CollisionShape* collisionShapeInitialize(CollisionSpace* space, CollisionShape* shape, int32_t id, int8_t kind)
{
    shape->kind = kind;
    shape->id = (int16_t)id;
    if (id >= 0) {
        collisionIdMark(&space->ids, shape->id);
        space->byId[shape->id] = shape;
    }
    ++space->shapeCount;
    if (space->peakShapeCount < space->shapeCount) space->peakShapeCount = space->shapeCount;
    shape->label[0] = '\0';
    shape->userData = NULL;
    shape->next = space->root.next;
    shape->previous = &space->root;
    space->root.next->previous = shape;
    space->root.next = shape;
    shape->minimum[0] = shape->minimum[1] = shape->maximum[0] = shape->maximum[1] = -9999999.0f;
    shape->locks = shape->contactCount = shape->pairCount = shape->dirty = 0;
    return shape;
}

CollisionShape* collisionDynamicShapeInitialize(CollisionSpace* space, CollisionShape* shape, int32_t id,
                                               int8_t kind, const void* resourceData)
{
    collisionShapeInitialize(space, shape, id, kind);
    ++space->dynamicCount;
    if (space->peakDynamicCount < space->dynamicCount) space->peakDynamicCount = space->dynamicCount;
    shape->resourceData = resourceData;
    shape->active = 1;
    shape->body = NULL;
    shape->prepareOwner = NULL;
    shape->bodyMode = 0;
    return shape;
}

void collisionSpaceUpdateWidth(CollisionSpace* space, const CollisionShape* shape, float width)
{
    const float previous = shape->maximum[0] - shape->minimum[0];
    if (width == previous) return;
    if (previous < width && space->maximumWidth < width) {
        space->maximumWidth = width;
        return;
    }
    if (width < previous && previous == space->maximumWidth) {
        float maximum = 0.0f;
        for (const CollisionShape* item = space->root.next; item != &space->root; item = item->next) {
            if (item != shape) {
                const float candidate = item->maximum[0] - item->minimum[0];
                if (maximum < candidate) maximum = candidate;
            }
        }
        space->maximumWidth = maximum;
    }
}

void collisionShapeSetBounds(CollisionSpace* space, CollisionShape* shape, const float minimum[2], const float maximum[2])
{
    collisionSpaceUpdateWidth(space, shape, maximum[0] - minimum[0]);
    const float previousMaximum = shape->maximum[0];
    memcpy(shape->minimum, minimum, sizeof(shape->minimum));
    memcpy(shape->maximum, maximum, sizeof(shape->maximum));
    if (previousMaximum == maximum[0]) return;
    while (shape->previous != &space->root && collisionShapeCompare(shape->previous, shape)) {
        CollisionShape* previous = shape->previous;
        shape->previous->next = shape->next;
        shape->next->previous = shape->previous;
        previous->previous->next = shape;
        shape->previous = previous->previous;
        shape->next = previous;
        previous->previous = shape;
    }
    while (shape->next != &space->root && collisionShapeCompare(shape, shape->next)) {
        CollisionShape* next = shape->next;
        shape->previous->next = shape->next;
        shape->next->previous = shape->previous;
        next->next->previous = shape;
        shape->next = next->next;
        shape->previous = next;
        next->next = shape;
    }
}

int32_t collisionSpaceOverlaps(const CollisionSpace* space, const float minimum[2], const float maximum[2])
{
    for (const CollisionShape* shape = space->root.next; shape != &space->root; shape = shape->next)
        if (vectorBoundsOverlaps2(shape->minimum, shape->maximum, minimum, maximum)
            && (shape->kind == 1 || shape->kind == 2)) return 1;
    return 0;
}

void collisionShapeComputeLocalBounds(CollisionShape* shape)
{
    float radius = 0.0f, radialRadius = 0.0f, maxZ = 0.0f, minZ = 0.0f;
    float minimum[3] = {0.0f, 0.0f, 0.0f}, maximum[3] = {0.0f, 0.0f, 0.0f};
    for (int index = 0; index < shape->geometry->vertexCount; ++index) {
        const float* point = shape->geometry->vertices[index].vector;
        const float x = point[0] * point[0], y = point[1] * point[1];
        const float xy = x + y, z = point[2] * point[2];
        const float radialLength = sqrtf(xy);
        const float length = sqrtf(xy + z);
        if (radius < length) radius = length;
        if (radialRadius < radialLength) radialRadius = radialLength;
        if (maxZ < point[2]) maxZ = point[2];
        else if (point[2] < minZ) minZ = point[2];
        for (int lane = 0; lane < 3; ++lane) {
            if (maximum[lane] < point[lane]) maximum[lane] = point[lane];
            else if (point[lane] < minimum[lane]) minimum[lane] = point[lane];
        }
    }
    shape->negativeExtentZ = 0.05f - minZ;
    shape->positiveExtentZ = maxZ + 0.05f;
    shape->radialRadius = radialRadius + 0.05f;
    shape->originRadius = radius;
    memcpy(shape->localMinimum, minimum, sizeof(minimum));
    memcpy(shape->localMaximum, maximum, sizeof(maximum));
}

void collisionShapeUpdateBounds(CollisionSpace* space, CollisionShape* shape)
{
    const float* position = positionOf(shape);
    const float radius = shape->radialRadius;
    // PAL1D72F0..1D7368 publishes the EE-rounded extents. Keep the same
    // bounds as the original moving body at float exponent boundaries.
    const float minimum[2] = {bScalarAdd(position[0], -radius), bScalarAdd(position[1], -radius)};
    const float maximum[2] = {bScalarAdd(position[0], radius), bScalarAdd(position[1], radius)};
    collisionShapeSetBounds(space, shape, minimum, maximum);
}

void collisionShapeCapture(CollisionShape* shape, float timestep)
{
    memcpy(shape->previousFrame, frameOf(shape), sizeof(shape->previousFrame));
    memcpy(shape->previousPosition, positionOf(shape), sizeof(shape->previousPosition));
    memcpy(shape->previousVelocity, velocityOf(shape), sizeof(shape->previousVelocity));
    shape->previousPosition[3]=1.0f;
    shape->previousVelocity[3]=0.0f;
    shape->previousTime = timestep;
}

void collisionShapeInterpolatePose(const CollisionShape* shape, float position[4], float frame[16], float rewind)
{
    const float* currentPosition = positionOf(shape);
    const float* currentFrame = frameOf(shape);
    if (shape->previousTime < 0.000001f) {
        memcpy(position, currentPosition, 3 * sizeof(float));
        position[3]=1.0f;
        memcpy(frame, currentFrame, 16 * sizeof(float));
        return;
    }
    const float time = shape->previousTime - rewind;
    const float fraction = finiteRatio(time, shape->previousTime);
    for (int lane = 0; lane < 3; ++lane) {
        const float delta = currentPosition[lane] - shape->previousPosition[lane];
        const float scaled = delta * fraction;
        position[lane] = scaled + shape->previousPosition[lane];
    }
    position[3]=1.0f;
    for (int lane = 0; lane < 16; ++lane) {
        const float delta = currentFrame[lane] - shape->previousFrame[lane];
        const float scaled = delta * fraction;
        frame[lane] = shape->previousFrame[lane] + scaled;
    }
}

void collisionShapeInterpolateVelocity(const CollisionShape* shape, float velocity[4], float rewind)
{
    const float* current = velocityOf(shape);
    velocity[3]=0.0f;
    if (shape->previousTime < 0.000001f) { memcpy(velocity, current, 3 * sizeof(float)); return; }
    const float time = shape->previousTime - rewind;
    const float fraction = finiteRatio(time, shape->previousTime);
    for (int lane = 0; lane < 3; ++lane) {
        const float delta = current[lane] - shape->previousVelocity[lane];
        const float scaled = delta * fraction;
        velocity[lane] = scaled + shape->previousVelocity[lane];
    }
}

void collisionShapeApplyStep(CollisionShape* shape, float rewind)
{
    float position[4], frame[16], velocity[4];
    collisionShapeInterpolatePose(shape, position, frame, rewind);
    collisionShapeInterpolateVelocity(shape, velocity, rewind);
    for (int lane = 0; lane < 3; ++lane) velocity[lane] = velocity[lane] * shape->body->mass;
    memcpy(shape->body->motion.position, position, sizeof(position));
    memcpy(shape->body->motion.orientation, frame, sizeof(frame));
    memcpy(shape->body->motion.linearMomentum, velocity, sizeof(velocity));
}

void collisionShapeFindRange(const CollisionSpace* space, const CollisionShape* shape,
                             CollisionShape** before, CollisionShape** after, float margin)
{
    CollisionShape* first = shape->previous;
    const float minimum = shape->minimum[0] - margin;
    if (first != &space->root && minimum <= first->maximum[0]) {
        do { first = first->previous; } while (first != &space->root && minimum <= first->maximum[0]);
    }
    *before = first;
    CollisionShape* last = shape->next;
    const float maximum = shape->maximum[0] + margin;
    if (last != &space->root && last->minimum[0] <= maximum) {
        do { last = last->next; } while (last != &space->root && last->minimum[0] <= maximum);
    }
    *after = last;
}

int32_t collisionShapeCrossesPlane(const CollisionShape* shape, const float normal[3], float offset)
{
    const float* frame = frameOf(shape);
    const float a = frame[8] * normal[0], b = frame[9] * normal[1], c = frame[10] * normal[2];
    const float axisDot = (a + b) + c;
    const float square = axisDot * axisDot;
    const float sideways = sqrtf(fabsf(1.0f - square));
    const float* position = positionOf(shape);
    const float x = position[0] * normal[0], y = position[1] * normal[1], z = position[2] * normal[2];
    const float distance = ((x + y) + z) - offset;
    float extent = shape->negativeExtentZ;
    if (axisDot * distance < 0.0f) extent = shape->positiveExtentZ;
    const float first = fabsf(axisDot) * extent, second = sideways * shape->radialRadius;
    return fabsf(distance) < first + second;
}

float collisionShapeImpulseEnergy(const CollisionShape* shape, const float impulse[4], const float lever[4])
{
    const float after = rigidBodyImpulseEnergy(shape->body, impulse, lever);
    const float before = rigidBodyKineticEnergy(shape->body);
    return after - before;
}

void collisionShapeStorePosition(CollisionShape* shape, const float position[4])
{
    memcpy(positionOf(shape), position, 3 * sizeof(float));
    positionOf(shape)[3]=1.0f;
}

void collisionSystemRemoveImpact(CollisionSystem* system, CollisionPair* pair)
{
    if (!pair->impact) return;
    --pair->first->shape->contactCount;
    --pair->second->shape->contactCount;
    collisionImpactDispose(system,pair->impact);
    pair->impact=NULL;
}

const CollisionSolverTuning collisionSolverDefaultTuning={
    0.05f,0.3f,1.0f,0.005f,0.1f,0.01f,0.15f,0.000001f,
    0.7692307829856873f,0.4f,0.4f,0.3f,0.1f,0.3f,0.1f,
    {-5.000100135803223f,0.0f}
};

static const float zero[4]={0,0,0,0};
static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
static const CarCollisionPolicy unboundCarPolicy={1,1};

static void changePairMask(CollisionSystem* system, const CollisionShape* first,
                            const CollisionShape* second, int present)
{
    const int32_t bit=first->id*COLLISION_ID_COUNT+second->id;
    const uint8_t mask=(uint8_t)(1u<<(bit&7));
    if (present) system->space->pairBits[bit>>3]|=mask;
    else system->space->pairBits[bit>>3]&=(uint8_t)~mask;
}

void collisionSystemInitialize(CollisionSystem* system, CollisionSpace* space,
                               CollisionPair* pairs, size_t pairCapacity,
                               CollisionQueuedContact* contacts, size_t contactCapacity,
                               CollisionImpact* impacts, size_t impactCapacity,
                               const CollisionSolverTuning* tuning)
{
    memset(system,0,sizeof(*system));
    memset(pairs,0,pairCapacity*sizeof(*pairs));
    memset(contacts,0,contactCapacity*sizeof(*contacts));
    memset(impacts,0,impactCapacity*sizeof(*impacts));
    system->space=space;
    system->pairs=pairs;system->pairCapacity=pairCapacity;
    system->contacts=contacts;system->contactCapacity=contactCapacity;
    system->impacts=impacts;system->impactCapacity=impactCapacity;
    system->tuning=tuning ? tuning : &collisionSolverDefaultTuning;
    system->pairRoot.next=system->pairRoot.previous=&system->pairRoot;
    system->contactRoot.next=system->contactRoot.previous=&system->contactRoot;
}

void collisionSystemBindWorld(CollisionSystem* system, World* world)
{
    system->world=world;
    system->randomState=&world->simulationRandomState;
}

CollisionSystemResult collisionCarShapeInitialize(CollisionSystem* system, CollisionShape* shape,
                                                   CarCollisionState* collision, VehicleState* state,
                                                   CollisionGeometry* geometry, TerrainSurface* terrain)
{
    if (!state->position || !geometry || !terrain) return COLLISION_SYSTEM_MISSING_OWNER;
    const int32_t id=collisionIdAcquire(&system->space->ids);
    if (id<0) return COLLISION_SYSTEM_PARTICIPANT_CAPACITY;
    collisionDynamicShapeInitialize(system->space,shape,id,0,terrain);
    shape->geometry=geometry;
    shape->vehicle.position=state->position;
    shape->vehicle.velocity=state->velocity;
    shape->vehicle.orientation=state->orientation;
    shape->dirty=1;
    collision->shape=shape;
    collision->collisionTicks=collision->suppressedCollisionTicks=
        collision->lastCollisionTimeTicks=system->nowTicks;
    collision->policy=&system->carPolicy;
    collisionShapeComputeLocalBounds(shape);
    // This constructor has bound a VehicleState, before any RigidBody can
    // be attached. Capture that actual owner directly; the polymorphic
    // capture path is used after the shape has entered the simulation.
    memcpy(shape->previousFrame,state->orientation,sizeof(shape->previousFrame));
    memcpy(shape->previousPosition,state->position,sizeof(shape->previousPosition));
    memcpy(shape->previousVelocity,state->velocity,sizeof(shape->previousVelocity));
    shape->previousTime=0.0f;
    collisionShapeUpdateBounds(system->space,shape);
    return COLLISION_SYSTEM_OK;
}

CollisionSystemResult collisionSystemBind(CollisionSystem* system, CollisionParticipant* participant)
{
    const int32_t id=participant->shape->id;
    if (id<0 || id>=COLLISION_ID_COUNT) return COLLISION_SYSTEM_PARTICIPANT_CAPACITY;
    if (participant->movement) {
        if (!participant->movement->initialized || participant->movement->result!=CAR_MOVEMENT_OK)
            return COLLISION_SYSTEM_MISSING_OWNER;
        participant->carState=carMovementOwnerMotion(participant->movement)->collision;
        if (!participant->carState) return COLLISION_SYSTEM_MISSING_OWNER;
        carMovementBindCollision(participant->movement,system,participant);
    }
    system->participants[id]=participant;
    if (participant->carState) {
        participant->carState->shape=participant->shape;
        participant->carState->policy=&system->carPolicy;
    }
    collisionParticipantRefresh(participant);
    return COLLISION_SYSTEM_OK;
}

int32_t collisionParticipantEligible(const CollisionSystem* system, const CollisionParticipant* participant)
{
    (void)system;
    return participant->movement ? carCollisionAllowsControl(participant->carState) : collisionShapeAllowsContact(participant->shape);
}

int32_t collisionParticipantSuppressesResponse(const CollisionSystem* system, const CollisionParticipant* participant)
{
    if (participant->movement) {
        const CarMotionState* state=carMovementOwnerMotion(participant->movement);
        return carMotionWindowActive(state,system->nowTicks) || carMotionTransitionFlag(state)!=0;
    }
    return participant->breakable && participant->breakable->responseSuppressed!=0;
}

int32_t collisionParticipantMovable(const CollisionParticipant* participant)
{
    if (participant->breakable) return participant->breakable->activationImpulse==0.0f;
    return participant->shape->active!=0;
}

void collisionParticipantPrepare(CollisionParticipant* participant)
{
    if (participant->movement) carMovementPrepareCollision(participant->movement);
    else if (participant->breakable) collisionBreakablePrepare(participant->breakable);
}

void collisionParticipantNotify(CollisionParticipant* participant, CollisionParticipant* other)
{
    if (!participant->movement) return;
    carMovementNotifyCollision(participant->movement,other ? other->movement : NULL);
    if (participant->movement->result!=CAR_MOVEMENT_OK) {
        participant->movement->collisionSystem->result=COLLISION_SYSTEM_OWNER_FAILURE;
        return;
    }
    collisionParticipantRefresh(participant);
    participant->shape->dirty=1;
}

void collisionParticipantCapture(CollisionSystem* system, CollisionParticipant* participant, float rewind)
{
    if (!participant->shape->active) return;
    if (participant->movement) {
        collisionShapeCapture(participant->shape,rewind);
        const float captured=carMovementCapture(participant->movement,rewind);
        if (captured<rewind) {
            collisionParticipantNotify(participant,NULL);
            carMovementCapture(participant->movement,rewind-captured);
        }
        collisionShapeUpdateBounds(system->space,participant->shape);
        if (participant->movement->result!=CAR_MOVEMENT_OK) system->result=COLLISION_SYSTEM_OWNER_FAILURE;
    } else if (participant->breakable) {
        ChopperMunition* actor=participant->breakable->munition;
        if (actor) {
            ChopperMunitionRegistry* registry=actor->registry;
            chopperMunitionAdvance(actor,rewind);
            if (registry->result!=WORLD_SERVICES_OK) system->result=COLLISION_SYSTEM_OWNER_FAILURE;
        } else collisionBreakableAdvance(system,participant->breakable,rewind);
    }
}

void collisionParticipantApplyStep(CollisionParticipant* participant, float rewind)
{
    if (participant->breakable && participant->breakable->munition)
        chopperMunitionApplyStep(participant->breakable->munition,rewind);
    else collisionShapeApplyStep(participant->shape,rewind);
}

void collisionParticipantVelocity(const CollisionParticipant* participant, float linear[4], float angular[4])
{
    if (participant->shape->active) collisionShapeCopyVelocityPair(participant->shape,linear,angular);
    else {memcpy(linear,zero,sizeof(zero));memcpy(angular,zero,sizeof(zero));}
}

float collisionParticipantInverseMass(const CollisionSystem* system, const CollisionParticipant* participant)
{
    if (!participant->shape->active || !participant->shape->body) return 0.0f;
    if (participant->breakable && participant->breakable->activationImpulse!=0.0f) return 0.0f;
    const float inverseMass=participant->shape->body->inverseMass;
    return participant->movement ? minimumFinite(inverseMass,system->tuning->heavyBodyInverseMassLimit) : inverseMass;
}

void collisionParticipantInverseInertia(const CollisionSystem* system, const CollisionParticipant* participant,
                                       float inertia[16])
{
    if (!participant->shape->active || (participant->breakable && participant->breakable->activationImpulse!=0.0f)) {
        memset(inertia,0,16*sizeof(float));
        return;
    }
    memcpy(inertia,participant->shape->body->inverseInertia,16*sizeof(float));
    if (participant->movement)
        for (int lane=0;lane<12;++lane) inertia[lane]=inertia[lane]*system->tuning->carInertiaScale;
}

void collisionParticipantApplyImpulse(const CollisionSystem* system, CollisionParticipant* participant,
                                     const float impulse[4], const float lever[4])
{
    if (participant->movement) {
        RigidBody* body=participant->shape->body;
        float scale=1.0f;
        if (system->tuning->heavyBodyInverseMassLimit<body->inverseMass)
            scale=system->tuning->heavyBodyInverseMassLimit/body->inverseMass;
        float scaledImpulse[4]={};
        for (int lane=0;lane<3;++lane) scaledImpulse[lane]=impulse[lane]*scale;
        rigidBodyApplyScaledAngularImpulse(body,scaledImpulse,lever,system->tuning->carAngularImpulseScale);
        collisionCarRecordImpact(participant,scaledImpulse,lever);
    } else if (participant->breakable) {
        ChopperMunition* actor=participant->breakable->munition;
        if (actor && actor->kind==CHOPPER_MUNITION_ROCKET) chopperRocketApplyImpulse(actor);
        else if (actor && actor->kind==CHOPPER_MUNITION_SPIKE_BOMB) chopperSpikeApplyImpulse(actor);
        else if (participant->breakable->simulating)
            rigidBodyApplyImpulse(&participant->breakable->body,impulse,lever);
    }
}

float collisionParticipantFriction(const CollisionSystem* system, const CollisionParticipant* participant)
{
    if (participant->breakable) return participant->breakable->definition->friction;
    return participant->movement ? system->tuning->carFriction : system->tuning->staticFriction;
}

float collisionParticipantRestitution(const CollisionSystem* system, const CollisionParticipant* participant)
{
    if (participant->breakable) return participant->breakable->definition->restitution;
    return participant->movement ? system->tuning->carRestitution : system->tuning->staticRestitution;
}

float collisionParticipantImpulseFraction(const CollisionParticipant* participant,
                                          const float impulse[4], const float lever[4])
{
    if (participant->breakable) {
        ChopperMunition* actor=participant->breakable->munition;
        if (actor && actor->kind==CHOPPER_MUNITION_ROCKET) return chopperRocketImpulseFraction(actor);
        if (actor && actor->kind==CHOPPER_MUNITION_SPIKE_BOMB) return chopperSpikeImpulseFraction(actor);
        return collisionBreakableImpulseFraction(participant->breakable,impulse,lever);
    }
    return 1.0f;
}

CollisionPair* collisionPairCreate(CollisionSystem* system, CollisionParticipant* first, CollisionParticipant* second)
{
    size_t slot=0;
    while (slot<system->pairCapacity && system->pairs[slot].occupied) ++slot;
    if (slot==system->pairCapacity) {system->result=COLLISION_SYSTEM_PAIR_CAPACITY;return NULL;}
    CollisionPair* pair=&system->pairs[slot];
    memset(pair,0,sizeof(*pair));
    collisionWitnessInitialize(&pair->witness,first->shape->geometry,second->shape->geometry);
    pair->first=first;pair->second=second;pair->occupied=1;
    pair->next=&system->pairRoot;pair->previous=system->pairRoot.previous;
    pair->previous->next=pair;system->pairRoot.previous=pair;
    changePairMask(system,first->shape,second->shape,1);
    changePairMask(system,second->shape,first->shape,1);
    ++first->shape->pairCount;++second->shape->pairCount;
    return pair;
}

void collisionPairDestroy(CollisionSystem* system, CollisionPair* pair)
{
    collisionSystemRemoveImpact(system,pair);
    pair->previous->next=pair->next;pair->next->previous=pair->previous;
    --pair->first->shape->pairCount;--pair->second->shape->pairCount;
    changePairMask(system,pair->first->shape,pair->second->shape,0);
    changePairMask(system,pair->second->shape,pair->first->shape,0);
    CollisionParticipant* owners[2]={pair->first,pair->second};
    for (int side=0;side<2;++side) if (owners[side]->polygon && !owners[side]->shape->pairCount) {
        for (size_t index=0;index<system->terrainCapacity;++index)
            if (&system->terrainShapes[index].participant==owners[side])
                collisionTerrainShapeDispose(system,&system->terrainShapes[index]);
    }
    pair->occupied=0;
}

int32_t collisionPairObsolete(const CollisionSystem* system, const CollisionPair* pair)
{
    if (pair->impact) return 0;
    const CollisionShape* first=pair->first->shape;
    const CollisionShape* second=pair->second->shape;
    if (!collisionShapesOverlap(first,second,0.0f)) return 1;
    if (!first->dirty && !second->dirty) return 1;
    return !collisionParticipantEligible(system,pair->first) || !collisionParticipantEligible(system,pair->second);
}

void collisionPairQueueContact(CollisionSystem* system, CollisionPair* pair)
{
    if (!collisionParticipantEligible(system,pair->first) || !collisionParticipantEligible(system,pair->second)) return;
    CollisionShape* first=pair->first->shape;
    CollisionShape* second=pair->second->shape;
    if ((first->kind==0 && !pair->first->carState) ||
        (second->kind==0 && !pair->second->carState)) {
        system->result=COLLISION_SYSTEM_MISSING_OWNER;
        return;
    }
    const float* firstFrame=first->active ? frameOf(first) : identity;
    const float* secondFrame=second->active ? frameOf(second) : identity;
    const float* firstPosition=first->active ? positionOf(first) : zero;
    const float* secondPosition=second->active ? positionOf(second) : zero;
    if (collisionWitnessQuery(&pair->witness,firstFrame,firstPosition,secondFrame,secondPosition,system->randomState)!=2) return;
    // PAL1D6744/1D6754 bypasses suppression for either static participant.
    // A suppressed moving car must still resolve contacts with the track.
    if (first->active && second->active &&
        (collisionParticipantSuppressesResponse(system,pair->first) || collisionParticipantSuppressesResponse(system,pair->second))) {
        if (first->kind==0) pair->first->carState->suppressedCollisionTicks=system->nowTicks;
        if (second->kind==0) pair->second->carState->suppressedCollisionTicks=system->nowTicks;
        return;
    }
    CollisionParticipant* owners[2]={pair->first,pair->second};
    for (int side=0;side<2;++side) if (owners[side]->shape->kind==0) {
        if (collisionParticipantYawSensitive(owners[1-side]))
            owners[side]->carState->lastCollisionTimeTicks=system->nowTicks;
        owners[side]->carState->collisionTicks=system->nowTicks;
    }
    size_t slot=0;
    while (slot<system->contactCapacity && system->contacts[slot].occupied) ++slot;
    if (slot==system->contactCapacity) {system->result=COLLISION_SYSTEM_CONTACT_CAPACITY;return;}
    CollisionQueuedContact* contact=&system->contacts[slot];
    contact->view.first=first;contact->view.second=second;contact->pair=pair;
    contact->view.time=collisionContactFindTime(system,contact);
    contact->createdFrame=system->frame;contact->occupied=1;
    CollisionQueuedContact* next=system->contactRoot.next;
    while (next!=&system->contactRoot && collisionContactOrder(&contact->view,&next->view,&system->tuning->contactOrder)) next=next->next;
    contact->next=next;contact->previous=next->previous;
    next->previous->next=contact;next->previous=contact;
}

CollisionImpact* collisionPairImpact(CollisionSystem* system, CollisionPair* pair, const float point[4])
{
    if (!pair->impact) {
        CollisionImpact* available=nullptr;
        for (size_t slot=0;slot<system->impactCapacity;++slot)
            if (!system->impacts[slot].occupied) {available=&system->impacts[slot];break;}
        CollisionImpactBlock** nextBlock=&system->impactBlocks;
        while (!available && *nextBlock) {
            for (auto& impact:(*nextBlock)->impacts)
                if (!impact.occupied) {available=&impact;break;}
            nextBlock=&(*nextBlock)->next;
        }
        if (!available) {
            auto* block=new (std::nothrow) CollisionImpactBlock{};
            if (!block) {system->result=COLLISION_SYSTEM_IMPACT_CAPACITY;return NULL;}
            *nextBlock=block;
            available=&block->impacts[0];
        }
        pair->impact=available;
        collisionImpactInitialize(system,pair->impact,pair,point);
        ++pair->first->shape->contactCount;++pair->second->shape->contactCount;
    }
    return pair->impact;
}

void collisionSystemFindPairs(CollisionSystem* system, CollisionParticipant* participant)
{
    if (!collisionParticipantEligible(system,participant)) return;
    collisionSystemFindTerrainPairs(system,participant);
    CollisionShape* shape=participant->shape;
    CollisionShape* before=shape->previous;
    CollisionShape* after=shape->next;
    const CollisionShape* root=&system->space->root;
    while (before!=root && shape->minimum[0]<=before->maximum[0]) before=before->previous;
    const float limit=shape->maximum[0]+system->space->maximumWidth;
    while (after!=root && after->maximum[0]<=limit) after=after->next;
    CollisionShape* other=before==root ? system->space->root.next : before->next;
    while (other!=after) {
        CollisionShape* next=other->next;
        if (other!=shape && collisionShapesOverlap(shape,other,0.0f)
                && !collisionPairMask(system->space,shape,other)) {
            CollisionParticipant* partner=collisionObjectParticipant(system,other);
            if (partner && collisionParticipantEligible(system,partner)) {
                if (other->kind==2) partner=collisionBreakablePromote(system,partner);
                if (partner && collisionShapesOverlap(shape,partner->shape,0.0f))
                    collisionPairCreate(system,participant,partner);
            }
        }
        if (system->result!=COLLISION_SYSTEM_OK) return;
        other=next;
    }
}

void collisionSystemDispose(CollisionSystem* system)
{
    while (system->contactRoot.next!=&system->contactRoot) collisionSystemDetachContact(system->contactRoot.next);
    while (system->pairRoot.next!=&system->pairRoot) collisionPairDestroy(system,system->pairRoot.next);
    // All source impact destructors have run before their pool storage dies.
    while (system->impactBlocks) {
        auto* block=system->impactBlocks;
        system->impactBlocks=block->next;
        delete block;
    }
    if (system->objects) {
        CollisionObjectRegistry* registry=system->objects;
        // Concrete munitions embed their own physical base, outside the
        // generic32-slot pool. The actual collision ID map sees both owners.
        for (size_t id=0;id<COLLISION_ID_COUNT;++id) {
            CollisionParticipant* participant=system->participants[id];
            if (participant && participant->breakable)
                collisionBreakableDestroy(system,participant->breakable);
        }
        for (size_t i=0;i<registry->placementCount;++i) {
            CollisionPlacedObject* placement=&registry->placements[i];
            if (placement->occupied) {
                placement->shape.previous->next=placement->shape.next;
                placement->shape.next->previous=placement->shape.previous;
                placement->shape.next=placement->shape.previous=NULL;
                --system->space->shapeCount;
            }
            placement->occupied=0;placement->activeObject=NULL;
        }
        if (registry->effectCount) memset(registry->effects,0,registry->effectCount*sizeof(*registry->effects));
        registry->effectCount=0;
        collisionObjectRegistryDisposeStorage(registry);
    }
    for (size_t i=0;i<system->terrainCapacity;++i)
        if (system->terrainShapes[i].occupied) collisionTerrainShapeDispose(system,&system->terrainShapes[i]);
    CollisionShape* shape=system->space->root.next;
    while (shape!=&system->space->root) {
        CollisionShape* next=shape->next;
        CollisionParticipant* participant=collisionObjectParticipant(system,shape);
        if (participant) {
            if (participant->movement) carMovementBindCollision(participant->movement,NULL,NULL);
            if (participant->carState) participant->carState->policy=&unboundCarPolicy;
        }
        if (shape->id>=0) {
            collisionIdRelease(&system->space->ids,shape->id);
            system->space->byId[shape->id]=NULL;system->participants[shape->id]=NULL;
        }
        --system->space->shapeCount;
        if (shape->active) --system->space->dynamicCount;
        shape->body=NULL;shape->prepareOwner=NULL;shape->bodyMode=0;
        shape->geometry=NULL;shape->resourceData=NULL;
        memset(&shape->vehicle,0,sizeof(shape->vehicle));
        shape->active=shape->dirty=0;
        shape->next=shape->previous=NULL;
        shape=next;
    }
    system->space->root.next=system->space->root.previous=&system->space->root;
    system->space->maximumWidth=0.0f;
    system->world=NULL;system->randomState=NULL;system->objects=NULL;
    system->terrainEffects=NULL;
    system->terrainShapes=NULL;system->terrainCapacity=0;
}

const CollisionImpact* collisionShapeContactAt(const CollisionSystem* system,
                                              const CollisionShape* shape,int32_t index)
{
    if (index >= shape->contactCount) return NULL;
    int32_t current=0;
    for (const CollisionPair* pair=system->pairRoot.next;pair!=&system->pairRoot;pair=pair->next) {
        if (pair->impact && (pair->first->shape==shape || pair->second->shape==shape)) {
            if (current==index) return pair->impact;
            ++current;
        }
    }
    return NULL;
}

static const float origin[4]={0,0,0,0};

CollisionSystemResult collisionTerrainShapeInitialize(CollisionSystem* system,
                                                       CollisionTerrainShape* terrain,
                                                       TerrainTopology* topology,
                                                       TerrainPolygon* polygon, int32_t id)
{
    if (!system || !system->space || !terrain || !topology || !polygon || !polygon->encoded)
        return COLLISION_SYSTEM_MISSING_OWNER;
    // Both the ordinary terrain broadphase and replay restoration create
    // static bodies. Never overwrite another live shape during restoration.
    if (id < 0 || id >= COLLISION_ID_COUNT || system->space->byId[id] || system->participants[id])
        return COLLISION_SYSTEM_PARTICIPANT_CAPACITY;
    memset(terrain,0,sizeof(*terrain));
    terrain->topology=topology;
    terrain->polygon=terrainTopologyAcquire(topology,polygon->encoded);
    if (!terrain->polygon) {
        // collisionSystemFindTerrainPairs has already reserved the ID. The
        // decoded-polygon cache can be exhausted on wide, busy tracks; an
        // unsuccessful static body must not consume one of the 206 IDs.
        collisionIdRelease(&system->space->ids,id);
        return COLLISION_SYSTEM_TERRAIN_CAPACITY;
    }
    collisionShapeInitialize(system->space,&terrain->shape,id,3);
    collisionGeometryBind(&terrain->geometry,terrain->vertices,4,terrain->edges,4,
                           terrain->faces,2,terrain->planes,32);
    collisionGeometryFromTerrainPolygon(&terrain->geometry,terrain->polygon,origin);
    terrain->shape.geometry=&terrain->geometry;
    terrain->shape.vehicle.position=(float*)origin;
    terrain->shape.vehicle.velocity=(float*)origin;
    terrain->shape.vehicle.orientation=(float*)identity;
    terrain->shape.resourceData=terrain->polygon;
    terrain->shape.active=0;
    terrain->participant.shape=&terrain->shape;
    terrain->participant.polygon=terrain->polygon;
    terrain->occupied=1;
    collisionShapeSetBounds(system->space,&terrain->shape,terrain->polygon->minimum,terrain->polygon->maximum);
    const CollisionSystemResult result=collisionSystemBind(system,&terrain->participant);
    if (result!=COLLISION_SYSTEM_OK) {
        // Roll back the list insertion, topology reference, participant slot
        // and ID as one failed construction (the success path is unchanged).
        collisionTerrainShapeDispose(system,terrain);
    }
    return result;
}

void collisionTerrainShapeDispose(CollisionSystem* system, CollisionTerrainShape* terrain)
{
    CollisionShape* shape=&terrain->shape;
    shape->previous->next=shape->next;
    shape->next->previous=shape->previous;
    collisionIdRelease(&system->space->ids,shape->id);
    system->space->byId[shape->id]=NULL;
    system->participants[shape->id]=NULL;
    --system->space->shapeCount;
    terrainTopologyRelease(terrain->topology,terrain->polygon);
    terrain->occupied=0;
}

void collisionSystemFindTerrainPairs(CollisionSystem* system, CollisionParticipant* participant)
{
    if (!participant->terrain) return;
    TerrainPolygon* candidates[48];
    size_t count=0;
    TerrainSurface* surface=participant->terrain;
    CollisionShape* shape=participant->shape;
    for (size_t index=0;index<surface->polygonCount;++index) {
        TerrainPolygon* polygon=surface->polygons[index];
        // The original candidate predicate is (not resident && XY overlap &&
        // crosses plane). Only check the resident terrain owner for polygons
        // inside the moving shape's XY interval. On wide tracks this avoids
        // scanning the fixed terrain cache for every distant polygon, while
        // preserving the source's polygon order, 48-entry limit and contacts.
        if (!vectorBoundsOverlaps2(shape->minimum,shape->maximum,polygon->minimum,polygon->maximum)) continue;
        int exists=0;
        for (size_t current=0;current<system->terrainCapacity;++current)
            if (system->terrainShapes[current].occupied && system->terrainShapes[current].polygon==polygon) exists=1;
        if (exists) continue;
        if (!collisionShapeCrossesPlane(shape,polygon->plane,polygon->plane[3])) continue;
        if (count<48) candidates[count++]=polygon;
    }
    /* Cache allocation order is not resource order. Match the source's stable
     * encoded-polygon ordering before creating native owners. */
    for (size_t i=1;i<count;++i) {
        TerrainPolygon* value=candidates[i];
        size_t j=i;
        while (j && value->encoded<candidates[j-1]->encoded) {candidates[j]=candidates[j-1];--j;}
        candidates[j]=value;
    }
    for (size_t i=0;i<count;++i) {
        size_t slot=0;
        while (slot<system->terrainCapacity && system->terrainShapes[slot].occupied) ++slot;
        if (slot==system->terrainCapacity) {system->result=COLLISION_SYSTEM_TERRAIN_CAPACITY;return;}
        const int32_t id=collisionIdAcquire(&system->space->ids);
        if (id<0) {system->result=COLLISION_SYSTEM_PARTICIPANT_CAPACITY;return;}
        const CollisionSystemResult result=collisionTerrainShapeInitialize(system,&system->terrainShapes[slot],
                                                                           surface->topology,candidates[i],id);
        if (result!=COLLISION_SYSTEM_OK) {system->result=result;return;}
    }
}

#include "indep/inc/CarInfo.hpp"
#include "indep/inc/Timer.hpp"
#include "bware/inc/bWare.hpp"
#include <cstdio>
#include <string>

namespace hp2 {
CarCollisionBody::CarCollisionBody(CollisionSystem& system,Car& car,VehicleState& vehicle,
    TerrainSurface& terrain,const CarDefinition& type,CollisionResource* const* resources,
    std::size_t count,const GameTime& clock):system_(system),car_(car)
{
    // PAL1D7E48 uses the same CarState pose and terrain owner. Source kind0
    // selects a car; a successful constructor is never a standalone box.
    Require(system.space && vehicle.position==car.position && (resources || !count),
        "Car collision construction has mismatched world/position owners");
    auto& space=*system.space;
    const auto id=collisionIdAcquire(&space.ids);
    Require(id>=0,"Car collision ID pool exhausted");
    collisionDynamicShapeInitialize(&space,&shape_,id,0,&terrain);
    ++space.carCount;if (space.peakCarCount<space.carCount)space.peakCarCount=space.carCount;
    shape_.userData=&car;
    shape_.vehicle={vehicle.position,vehicle.velocity,vehicle.orientation};
    // PAL1D7EB8/1D7EC0 publishes the moving-car broadphase flag here.
    // Kinematic and Mellow movers retain it; only PhysicsCar's rest logic
    // subsequently clears it. Without this initial value an AI car cannot
    // discover its terrain/wall pairs until another body hits it first.
    shape_.dirty=1;
    state_.shape=&shape_;state_.policy=&system.carPolicy;
    state_.collisionTicks=state_.suppressedCollisionTicks=
        state_.lastCollisionTimeTicks=clock.worldTimer;
    std::snprintf(shape_.label,sizeof(shape_.label),"Car%d",static_cast<int>(car.driverNumber));
    try {
        // PAL1D7F14 selects CarTypeInfo+20 for the authored hull, matching
        // shared NFS Edition geometry instead of the variant's reward name.
        const auto name=type.baseName+"_CV";
        if (auto* resource=FindPipelinePolyhedron(resources,count,hashString(name.c_str()),type.centerOfMass.data()))
            shape_.geometry=&resource->Geometry();
        else {
            // These are the six actual PAL GP9134..9148 constants. The
            // missing-resource branch really allocates this primitive.
            fallback_=std::make_unique<CollisionBox>();
            const float maximum[3]={2.1649999618530273f,0.9710000157356262f,0.6240000128746033f};
            const float minimum[3]={-2.2170000076293945f,-0.9710000157356262f,-0.20000000298023224f};
            collisionBoxInitialize(fallback_.get(),maximum,minimum);
            shape_.geometry=&fallback_->geometry;
        }
        collisionShapeComputeLocalBounds(&shape_);
        collisionShapeUpdateBounds(&space,&shape_);
    } catch (...) {
        shape_.previous->next=shape_.next;shape_.next->previous=shape_.previous;
        space.byId[id]=nullptr;collisionIdRelease(&space.ids,id);
        --space.carCount;--space.dynamicCount;--space.shapeCount;throw;
    }
}

CarCollisionBody::~CarCollisionBody()
{
    // PAL1D7FA0 and its actual DynamicCollisionBody/CollisionBody cleanup:
    // owned fallback only, broad-phase width/list, all pairs, and the ID.
    auto& space=*system_.space;
    --space.carCount;
    collisionSpaceUpdateWidth(&space,&shape_,0.0f);
    shape_.previous->next=shape_.next;shape_.next->previous=shape_.previous;
    for (auto* pair=system_.pairRoot.next;pair!=&system_.pairRoot;) {
        auto* next=pair->next;
        if (pair->first->shape==&shape_ || pair->second->shape==&shape_)collisionPairDestroy(&system_,pair);
        pair=next;
    }
    collisionQueueRemoveShape(&system_,&shape_);
    if (shape_.id>=0) {
        space.byId[shape_.id]=nullptr;system_.participants[shape_.id]=nullptr;
        collisionIdRelease(&space.ids,shape_.id);
    }
    --space.dynamicCount;--space.shapeCount;
}
} // namespace hp2

#include "indep/inc/Replay.hpp"

// PAL1D6A98..1D6B4C. Locks and dirty share the original packed byte;
// shape identity is verification-only and does not get restored from IDs.
void collisionShapeSnapshot(CollisionShape* shape,ReplaySnapshot* snapshot)
{
    if (!shape || !snapshot) {if(snapshot)snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;}
    uint8_t flags=static_cast<uint8_t>(shape->locks+(static_cast<uint8_t>(shape->dirty)<<4));
    replaySnapshotMoveData(snapshot,&flags,1,1,0);
    if (snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
        shape->locks=flags&15u;shape->dirty=static_cast<int8_t>(flags>>4);
    }
    replaySnapshotVerifyData(snapshot,&shape->id,2,2,1);
    replaySnapshotVerifyData(snapshot,&shape->kind,1,1,1);
    replaySnapshotVerifyData(snapshot,&shape->active,1,1,1);
}

// PAL1D7050..1D7158. The terrain interest cache is retired in both real
// capture and restore; transient swept poses are recomputed by the next step.
void collisionDynamicShapeSnapshot(CollisionParticipant* participant,ReplaySnapshot* snapshot)
{
    if (!snapshot)return;
    if (!participant || !participant->shape || !participant->terrain) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
    }
    auto& shape=*participant->shape;
    replaySnapshotBeginSection(snapshot,&shape,"DynamicCollisionBody");
    collisionShapeSnapshot(&shape,snapshot);
    replaySnapshotMoveInteger(snapshot,&shape.bodyMode,1);
    if(snapshot->recording || snapshot->restoring)terrainSurfaceReset(participant->terrain);
    replaySnapshotVerifyData(snapshot,&shape.negativeExtentZ,4,-1,0);
    replaySnapshotVerifyData(snapshot,&shape.positiveExtentZ,4,-1,0);
    replaySnapshotVerifyData(snapshot,&shape.radialRadius,4,-1,0);
    replaySnapshotVerifyData(snapshot,shape.localMinimum,12,-1,0);
    replaySnapshotVerifyData(snapshot,shape.localMaximum,12,-1,0);
    if(snapshot->recording || snapshot->restoring) {
        shape.previousTime=0.0f;
        memset(shape.previousFrame,0,sizeof(shape.previousFrame));
        memset(shape.previousPosition,0,sizeof(shape.previousPosition));
        memset(shape.previousVelocity,0,sizeof(shape.previousVelocity));
    }
    replaySnapshotEndSection(snapshot);
}

// PAL1D8058..1D8134. The actual CarMovement owns the physics objects that
// SetupPhysicsPointers1D8138 rebinds; no second RigidBody is allocated here.
void collisionCarSnapshot(CollisionSystem* system,CarCollisionState* car,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!system || !system->space || !car || !car->shape || car->shape->id<0 ||
        car->shape->id>=COLLISION_ID_COUNT) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;}
    auto* participant=system->participants[car->shape->id];
    if(!participant || participant->shape!=car->shape || participant->carState!=car || !participant->movement ||
        !participant->movement->bindings.owner) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;}
    auto& shape=*car->shape;auto& movement=*participant->movement;
    replaySnapshotBeginSection(snapshot,car,"CarCollisionBody");
    collisionDynamicShapeSnapshot(participant,snapshot);
    int8_t driver=static_cast<int8_t>(movement.bindings.owner->driverNumber);
    replaySnapshotVerifyData(snapshot,&driver,1,1,1);
    if(snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
        shape.body=nullptr;shape.prepareOwner=nullptr;
        if(shape.bodyMode && (movement.mode==1 || movement.mode==4)) {
            shape.body=carMovementCollisionBody(&movement);
            shape.prepareOwner=movement.mode==1 && movement.physics ? static_cast<void*>(&movement.physics->components) :
                movement.mode==4 && movement.mellow ? static_cast<void*>(&movement.mellow->components) : nullptr;
            if(!shape.body || !shape.prepareOwner)snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
        }
    }
    if((snapshot->recording || snapshot->restoring) && snapshot->result==REPLAY_SNAPSHOT_OK)
        collisionShapeUpdateBounds(system->space,&shape);
    replaySnapshotVerifyData(snapshot,shape.minimum,8,-1,0);
    replaySnapshotVerifyData(snapshot,shape.maximum,8,-1,0);
    replaySnapshotMoveInteger(snapshot,&car->collisionTicks,4);
    replaySnapshotMoveInteger(snapshot,&car->suppressedCollisionTicks,4);
    replaySnapshotMoveInteger(snapshot,&car->lastCollisionTimeTicks,4);
    replaySnapshotEndSection(snapshot);
}

// PAL1D6E70..1D6EE8 verifies the actual broadphase ordering. It is not a
// second shape serializer, and ordinary capture/restore adds no ID bytes.
void collisionBodiesSnapshotEverything(CollisionSpace* space,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!space) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;}
    replaySnapshotBeginSection(snapshot,nullptr,"AllCollisionBodies");
    for(auto* shape=space->root.next;shape!=&space->root;shape=shape->next) {
        uint16_t id=static_cast<uint16_t>(shape->id);
        replaySnapshotVerifyData(snapshot,&id,2,2,0);
    }
    replaySnapshotEndSection(snapshot);
}

// PAL1D6848..1D688C: the original witness destructor also retires its live
// impact/audio and zero-reference track polygon. Keep that exact owner path.
void collisionEliminateAllWitnesses(CollisionSystem* system)
{
    while(system->pairRoot.next!=&system->pairRoot)
        collisionPairDestroy(system,system->pairRoot.next);
}

// PAL1D6168..1D6208 restores the real Collision owner only when its stream
// presence byte is set. A restored collision is born without a sound point.
void collisionPairSnapshot(CollisionSystem* system,CollisionPair* pair,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!system || !pair || !pair->occupied) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;}
    replaySnapshotBeginSection(snapshot,pair,"CollisionBodyWitness");
    collisionWitnessSnapshot(&pair->witness,snapshot);
    int8_t hasImpact=pair->impact!=nullptr;
    replaySnapshotMoveData(snapshot,&hasImpact,1,1,1);
    if(hasImpact && snapshot->result==REPLAY_SNAPSHOT_OK) {
        if(snapshot->restoring && !collisionPairImpact(system,pair,nullptr))
            snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
        if(pair->impact)collisionImpactSnapshot(pair->impact,snapshot);
        else snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
    }
    replaySnapshotEndSection(snapshot);
}

// PAL1D6208..1D6380. Source IDs resolve through the one CollisionSpace;
// transient native slot positions and pointer widths are never serialized.
void collisionWitnessesSnapshotEverything(CollisionSystem* system,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!system || !system->space) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;}
    replaySnapshotBeginSection(snapshot,nullptr,"AllCollisionBodyWitnesses");
    int32_t count=0;
    if(!snapshot->restoring)
        for(auto* pair=system->pairRoot.next;pair!=&system->pairRoot;pair=pair->next)++count;
    replaySnapshotMoveInteger(snapshot,&count,2);
    if(snapshot->restoring) {
        if(count<0 || static_cast<size_t>(count)>system->pairCapacity)
            snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;
        for(int32_t index=0;index<count && snapshot->result==REPLAY_SNAPSHOT_OK;++index) {
            int32_t first=-1,second=-1;
            replaySnapshotMoveInteger(snapshot,&first,2);replaySnapshotMoveInteger(snapshot,&second,2);
            if(snapshot->result!=REPLAY_SNAPSHOT_OK)break;
            if(first<0 || second<0 || first>=COLLISION_ID_COUNT || second>=COLLISION_ID_COUNT || first==second ||
                !system->participants[first] || !system->participants[second] ||
                collisionPairMask(system->space,system->participants[first]->shape,system->participants[second]->shape)) {
                snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;break;
            }
            auto* pair=collisionPairCreate(system,system->participants[first],system->participants[second]);
            if(!pair) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;break;}
            collisionPairSnapshot(system,pair,snapshot);
        }
    } else {
        for(auto* pair=system->pairRoot.next;pair!=&system->pairRoot;pair=pair->next) {
            int32_t first=pair->first->shape->id,second=pair->second->shape->id;
            replaySnapshotMoveInteger(snapshot,&first,2);replaySnapshotMoveInteger(snapshot,&second,2);
            collisionPairSnapshot(system,pair,snapshot);
        }
    }
    replaySnapshotEndSection(snapshot);
}

// PAL1D8918..1D8980, including StaticCollisionBody's real base tail1D8760.
void collisionTerrainShapeSnapshot(CollisionTerrainShape* terrain,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!terrain || !terrain->occupied) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;}
    replaySnapshotBeginSection(snapshot,terrain,"TrackPolygonCollisionBody");
    collisionShapeSnapshot(&terrain->shape,snapshot);
    replaySnapshotVerifyData(snapshot,terrain->shape.minimum,8,-1,0);
    replaySnapshotVerifyData(snapshot,terrain->shape.maximum,8,-1,0);
    replaySnapshotEndSection(snapshot);
}

// PAL1D8980..1D8B14. Encoded polygon indices select authored topology. The
// 0xFFFF sentinel terminates the stream; live terrain slots are not disk data.
void collisionTerrainSnapshotEverything(CollisionSystem* system,TerrainTopology* topology,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!system || !system->space || !topology || (!topology->polygons && topology->polygonCount)) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
    }
    replaySnapshotBeginSection(snapshot,nullptr,"AllTrackPolygonCollisionBodies");
    int32_t count=0;
    for(size_t index=0;index<system->terrainCapacity;++index)if(system->terrainShapes[index].occupied)++count;
    replaySnapshotVerifyData(snapshot,&count,4,4,1);
    if(snapshot->restoring) {
        for(;;) {
            uint16_t polygonIndex=0xffff;
            replaySnapshotMoveData(snapshot,&polygonIndex,2,2,0);
            if(snapshot->result!=REPLAY_SNAPSHOT_OK || polygonIndex==0xffff)break;
            uint16_t id=0xffff;
            replaySnapshotMoveData(snapshot,&id,2,2,0);
            if(snapshot->result!=REPLAY_SNAPSHOT_OK)break;
            if(polygonIndex>=topology->polygonCount || id>=COLLISION_ID_COUNT || system->space->byId[id]) {
                snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;break;
            }
            size_t slot=0;while(slot<system->terrainCapacity && system->terrainShapes[slot].occupied)++slot;
            if(slot==system->terrainCapacity) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;break;}
            auto* polygon=terrainTopologyAcquire(topology,&topology->polygons[polygonIndex]);
            if(!polygon) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;break;}
            // Borrow the real decoded cache entry while the existing original
            // constructor acquires its own lasting reference, then release ours.
            const auto result=collisionTerrainShapeInitialize(system,&system->terrainShapes[slot],topology,polygon,id);
            terrainTopologyRelease(topology,polygon);
            if(result!=COLLISION_SYSTEM_OK) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;break;}
            collisionTerrainShapeSnapshot(&system->terrainShapes[slot],snapshot);
        }
    } else {
        for(auto* shape=system->space->root.next;shape!=&system->space->root;shape=shape->next) {
            if(shape->kind!=3)continue;
            CollisionTerrainShape* terrain=nullptr;
            for(size_t index=0;index<system->terrainCapacity;++index)
                if(system->terrainShapes[index].occupied && &system->terrainShapes[index].shape==shape)terrain=&system->terrainShapes[index];
            if(!terrain || terrain->topology!=topology || !terrain->polygon) {
                snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;break;
            }
            const auto index=terrain->polygon->encoded-topology->polygons;
            if(index<0 || static_cast<size_t>(index)>=topology->polygonCount || index>=0xffff) {
                snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;break;
            }
            uint16_t polygonIndex=static_cast<uint16_t>(index),id=static_cast<uint16_t>(shape->id);
            replaySnapshotMoveData(snapshot,&polygonIndex,2,2,0);replaySnapshotMoveData(snapshot,&id,2,2,0);
            collisionTerrainShapeSnapshot(terrain,snapshot);
        }
        uint16_t end=0xffff;replaySnapshotMoveData(snapshot,&end,2,2,0);
    }
    replaySnapshotEndSection(snapshot);
}

namespace {
static int collisionLocalVector65(CarMovement* movement, float local[4], const float value[4])
{
    const RigidBody* body = carMovementCollisionBody(movement);
    if (!body) { movement->result = CAR_MOVEMENT_INVALID_BINDING; return 0; }
    float inverse[16];
    vectorMatrixTranspose(inverse, body->motion.orientation);
    vectorMatrixTransformPoint(local, inverse, value);
    return 1;
}
}

// PAL1D83D0 CarCollisionBody::AddDamage.
void carMovementRecordCollisionDamage(CarMovement* movement, const float lever[4], float magnitude)
{
    if (movement->bindings.damage) {
        float local[4];
        if (!collisionLocalVector65(movement, local, lever)) return;
        carDamageAdd(movement->bindings.damage, local, magnitude * 0.013000000268220901f);
    }
    const CarMovementPlayer* player = movement->bindings.player;
    if (player && (movement->bindings.race && movement->bindings.race->mode != 4 && movement->bindings.race->mode != 7 && movement->bindings.race->mode != 5 && movement->bindings.race->mode != 8 && movement->bindings.race->mode != 9)) {
        const auto* feedback=movement->bindings.feedback;
        if(!feedback || !feedback->input || !feedback->flow ||
           !feedback->frontend || !feedback->players) {
            movement->result=CAR_MOVEMENT_INVALID_BINDING;
            return;
        }
        // PAL1D84B0 calls PortRumbleImpact synchronously. Its existing source
        // gates read the same live flow, session and player option owners.
        joystickPortRumbleImpact(*feedback->input,player->controllerPort,magnitude,
            *feedback->flow,feedback->session,*feedback->frontend,*feedback->players);
    }
    if (movement->bindings.pursuit && 1.0f <= magnitude) {
        int32_t carContact = 0;
        const CollisionSystem* system = movement->collisionSystem;
        if (!system || !movement->collisionParticipant) {
            movement->result = CAR_MOVEMENT_INVALID_BINDING;
            return;
        }
        for (const CollisionPair* pair = system->pairRoot.next; pair != &system->pairRoot; pair = pair->next) {
            if (!pair->impact || (pair->first != movement->collisionParticipant &&
                                  pair->second != movement->collisionParticipant)) continue;
            if (pair->first->shape->kind == 0 && pair->second->shape->kind == 0) {
                carContact = 1;
                break;
            }
        }
        if (pursuitCollisionRecord(movement->bindings.pursuit, movement, magnitude, carContact) != PURSUIT_COLLISION_OK)
            movement->result = CAR_MOVEMENT_PURSUIT_FAILURE;
    }
}

// PAL1D8550 CarCollisionBody::AnimateCollisionReaction.
void carMovementRecordCollisionFeedback(CarMovement* movement, const float impulse[4], float magnitude)
{
    auto* visual=movement->bindings.owner ? movement->bindings.owner->visual : nullptr;
    if (visual && 4.0f < magnitude) {
        int32_t variant = 6;
        int32_t priority = 2;
        if (20.0f < magnitude) {
            float local[4];
            if (!collisionLocalVector65(movement, local, impulse)) return;
            if (fabsf(local[1]) < fabsf(local[0])) variant = 0.0f < local[0] ? 10 : 7;
            else variant = 0.0f < local[1] ? 9 : 8;
            priority = 3;
        }
        // PAL1D864C targets AnimatedDriver::BeginCollision, not audio.
        carDriverVisualRequestAnimation(visual,variant,priority);
        if(carDriverVisualResult(visual)!=CAR_DRIVER_VISUAL_OK) {
            movement->result=CAR_MOVEMENT_PRESENTATION_FAILURE;
            return;
        }
    }
    if (movement->bindings.player && 4.0f < magnitude) {
        float local[4];
        if (!collisionLocalVector65(movement, local, impulse)) return;
        carMovementSteeringImpact(movement, local, magnitude);
    }
}

// Checked native adapter for the two distinct original dispatch operations.
void carMovementRecordCollisionImpact(CarMovement* movement, const float impulse[4],
                                      const float lever[4], float magnitude)
{
    carMovementRecordCollisionDamage(movement, lever, magnitude);
    if(movement->result==CAR_MOVEMENT_OK)
        carMovementRecordCollisionFeedback(movement, impulse, magnitude);
}


// C ABI entry point for the native member.
bool carCollisionAllowsControl(const CarCollisionState *collision)
{
    if (collision->policy->collisionDisabled || collision->policy->secondaryCollisionDisabled) return false;
    return collisionShapeAllowsContact(collision->shape);
}
