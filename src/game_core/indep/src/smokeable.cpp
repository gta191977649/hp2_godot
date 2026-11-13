#include "indep/inc/Smokeable.hpp"
#include "indep/inc/WorldObject.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/SmokeableInfo.hpp"
#include "indep/inc/Munitions.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/Replay.hpp"

#include <cmath>
#include <climits>
#include <cstring>
#include <new>
#include <type_traits>

/* Native storage for the original expandable SlotPool. These are ownership
 * adapters, not another physical object, replay owner or original function. */
struct CollisionObjectPoolBlock {
    CollisionObjectPoolBlock* next=nullptr;
    std::size_t count=0;
    std::unique_ptr<CollisionBreakable[]> objects;
};
struct CollisionEffectGroupBlock {
    CollisionEffectGroupBlock* next=nullptr;
    std::size_t count=0;
    std::unique_ptr<CollisionObjectEffectGroup[]> groups;
};
static_assert(std::is_trivial<CollisionBreakable>::value &&
    std::is_trivial<CollisionObjectEffectGroup>::value,
    "Native pool arrays must not throw from element construction");
static_assert(std::is_nothrow_default_constructible<CollisionObjectPoolBlock>::value &&
    std::is_nothrow_default_constructible<CollisionEffectGroupBlock>::value,
    "Native pool block construction must not throw after nothrow allocation");

namespace {
float speed(const float velocity[4])
{
    const float x = velocity[0] * velocity[0];
    const float y = velocity[1] * velocity[1];
    const float z = velocity[2] * velocity[2];
    return std::sqrt((x + y) + z);
}

float elapsedSeconds(int32_t now, int32_t before)
{
    const uint32_t difference = static_cast<uint32_t>(now) - static_cast<uint32_t>(before);
    int32_t ticks;
    std::memcpy(&ticks, &difference, sizeof(ticks));
    return static_cast<float>(ticks) * 0.0002500000118743628f;
}

bool timerStarted(int32_t ticks)
{
    return ticks != 0 && ticks != INT32_MAX;
}
}

RigidBodyInfo* hp2::CollisionObjectData::MutableRigidBodyInfo(std::uint32_t key) noexcept
{
    for (const auto& definition : definitions_)
        if (definition->key == key) return &definition->body;
    return nullptr;
}

int16_t collisionSmokeableCreate(CollisionSystem* system, TerrainTopology* topology,
    const CollisionBreakableDefinition* definition, uint32_t modelKey,
    const float point[4], uint16_t yaw)
{
    if (!system) return -1;
    if (!system->space || !system->objects || !topology || !definition || !point) {
        system->result = COLLISION_SYSTEM_MISSING_OWNER;
        return -1;
    }

    TerrainSurface terrain;
    terrainSurfaceInitialize(&terrain, topology);
    float normal[4] = {};
    bool found = false;
    const float elevation = terrainSurfaceElevation(&terrain, point, nullptr, normal, &found);
    if (!found) {
        terrainSurfaceDispose(&terrain);
        return -1;
    }
    CollisionBreakable* object = collisionBreakableCreate(system, definition, topology, modelKey);
    if (!object) {
        terrainSurfaceDispose(&terrain);
        return -1;
    }

    const float heading[3] = {vectorCosineAngle(yaw), rotationSineAngle(yaw), 0.0f};
    float frame[16] = {};
    /* The two ordered cross products are intentional. On flat ground,
     * zero yaw produces forward(-1,0,0), side(0,-1,0), normal(0,0,1). */
    frame[4] = heading[1] * normal[2] - heading[2] * normal[1];
    frame[5] = heading[2] * normal[0] - heading[0] * normal[2];
    frame[6] = heading[0] * normal[1] - heading[1] * normal[0];
    frame[0] = frame[5] * normal[2] - frame[6] * normal[1];
    frame[1] = frame[6] * normal[0] - frame[4] * normal[2];
    frame[2] = frame[4] * normal[1] - frame[5] * normal[0];
    std::memcpy(frame + 8, normal, 3 * sizeof(float));
    float height = (-object->shape.localMinimum[2] - definition->centerOfMass[2]) * normal[2];
    height = elevation + height;
    frame[12] = point[0];
    frame[13] = point[1];
    frame[14] = height + 0.05999999865889549f;
    frame[15] = 1.0f;
    collisionBreakableSetTransform(object, frame, nullptr);
    terrainSurfaceDispose(&terrain);
    return object->shape.id;
}

CollisionSystemResult collisionSmokeableDelete(CollisionSystem* system, int16_t id,
    const CollisionBreakableDefinition* definition)
{
    if (!system || !system->space) return COLLISION_SYSTEM_MISSING_OWNER;
    if (id < 0) return COLLISION_SYSTEM_OK;
    CollisionShape* shape = collisionBodyById(system->space, id);
    if (!shape) return system->result = COLLISION_SYSTEM_OWNER_FAILURE;
    if (shape->kind != 1) return COLLISION_SYSTEM_OK;
    CollisionParticipant* participant = system->participants[id];
    if (!participant || participant->shape != shape || !participant->breakable)
        return system->result = COLLISION_SYSTEM_OWNER_FAILURE;
    CollisionBreakable* object = participant->breakable;
    if (object->definition != definition) return COLLISION_SYSTEM_OK;
    collisionBreakableDestroy(system, object);
    return system->result;
}

CollisionSystemResult collisionObjectRegistryBindEffects(CollisionObjectRegistry* registry,
    ChopperMunitionEffects* effects)
{
    if (!registry || !effects || !effects->particles || !effects->particles->randomState)
        return COLLISION_SYSTEM_MISSING_OWNER;
    if (registry->particleEffects == effects) return COLLISION_SYSTEM_OK;
    if (registry->occupiedObjects) return COLLISION_SYSTEM_OWNER_FAILURE;
    for (size_t i = 0; i < registry->effectGroupCapacity; ++i)
        if (registry->effectGroups[i].occupied) return COLLISION_SYSTEM_OWNER_FAILURE;
    for (auto* block=registry->expandedEffectGroups;block;block=block->next)
        for (size_t i=0;i<block->count;++i)
            if (block->groups[i].occupied) return COLLISION_SYSTEM_OWNER_FAILURE;
    registry->particleEffects = effects;
    return COLLISION_SYSTEM_OK;
}

CollisionSystemResult collisionObjectEffectGroupCreate(CollisionObjectRegistry* registry,
    CollisionObjectEffectGroup* group, uint32_t key)
{
    if (!registry || !registry->particleEffects || !registry->particleEffects->particles ||
        !registry->particleEffects->particles->randomState || !group || group->occupied ||
        group->firstEmitter || group->lastEmitter) return COLLISION_SYSTEM_MISSING_OWNER;
    auto* effects = registry->particleEffects;
    if (!effects->groups || (effects->capacity && !effects->instances))
        return COLLISION_SYSTEM_MISSING_OWNER;
    const ChopperEffectGroup* authored = nullptr;
    for (size_t index = 0; index < effects->groupCount; ++index)
        if (effects->groups[index].key == key) { authored = &effects->groups[index]; break; }
    // SpawnSmokeables PAL21F964/21F974 calls afxGetEmitters1272E0 for both
    // definition lists. Its exact group lookup127198 returns null when no
    // emitter matches, leaving the initialized list empty. The released
    // TRACKB21..26/31..36 legitimately retain such a reference. Required
    // chopper groups still use their separate strict creation contract.
    if (authored) {
        const auto result = chopperEffectsCreatePersistentGroup(effects, key,
            &group->firstEmitter, &group->lastEmitter);
        if (result != WORLD_SERVICES_OK)
            return result == WORLD_SERVICES_CAPACITY ? COLLISION_SYSTEM_EFFECT_CAPACITY : COLLISION_SYSTEM_MISSING_OWNER;
    } else {
        effects->result = WORLD_SERVICES_OK;
    }
    group->resourceKey = key;
    group->occupied = 1;
    for (auto* emitter = group->firstEmitter; emitter; emitter = emitter->next)
        emitter->emissionState = 1;
    return COLLISION_SYSTEM_OK;
}

CollisionSystemResult collisionObjectEffectGroupStart(CollisionBreakable* object,
    CollisionObjectEffectGroup* group)
{
    auto* effects = object->system->objects->particleEffects;
    if (!effects || !effects->particles || !group || !group->occupied)
        return object->system->result = COLLISION_SYSTEM_MISSING_OWNER;
    for (auto* emitter = group->firstEmitter; emitter; emitter = emitter->next)
        if (emitter->particleSystem)
            return object->system->result = COLLISION_SYSTEM_OWNER_FAILURE;
    for (auto* emitter = group->firstEmitter; emitter; emitter = emitter->next)
        if (worldParticlesAddEmitter(effects->particles, emitter) != WORLD_SERVICES_OK)
            return object->system->result = COLLISION_SYSTEM_OWNER_FAILURE;
    group->started = 1;
    return COLLISION_SYSTEM_OK;
}

void collisionObjectEffectGroupSetTransform(CollisionObjectEffectGroup* group,
    const float* frame, const float* velocity)
{
    for (auto* emitter = group->firstEmitter; emitter; emitter = emitter->next) {
        if (frame) std::memcpy(emitter->frame, frame, sizeof(emitter->frame));
        if (velocity) {
            std::memcpy(emitter->carVelocity, velocity, 3 * sizeof(float));
            emitter->carVelocity[3] = 0.0f;
        }
    }
}

void collisionObjectEffectGroupRelease(CollisionObjectRegistry* registry,
    CollisionObjectEffectGroup* group)
{
    if (!registry || !registry->particleEffects || !group || !group->occupied) return;
    chopperEffectsReleasePersistentGroup(registry->particleEffects,
        &group->firstEmitter, &group->lastEmitter);
    if (!group->firstEmitter) *group = {};
}

/* Optional observer records describe actions already performed by the real
 * audio/particle/physical owners. A missing or full diagnostic buffer must
 * not turn successful source lifetime/activation logic into a physics error.
 * This native adapter adds no PAL function. */
CollisionEffect* collisionObjectNewEffect(CollisionSystem* system, CollisionEffectKind kind,
    CollisionParticipant* owner)
{
    CollisionObjectRegistry* registry = system->objects;
    if (!registry || !registry->effects || registry->effectCount>=registry->effectCapacity)
        return nullptr;
    CollisionEffect* effect = &registry->effects[registry->effectCount++];
    std::memset(effect, 0, sizeof(*effect));
    effect->kind = kind;
    effect->owner = owner;
    return effect;
}

void collisionBreakableUpdateTerrain(CollisionBreakable* object)
{
    const float radius = minimumFinite(object->shape.negativeExtentZ, object->shape.radialRadius);
    const float lead = minimumFinite(speed(object->body.linearVelocity) * 0.05f, 6.0f);
    terrainSurfaceUpdate(&object->terrain, object->body.motion.position, radius + lead,
        object->body.linearVelocity);
    collisionShapeUpdateBounds(object->system->space, &object->shape);
}

void collisionBreakableUpdateEffects(CollisionBreakable* object, const float frame[16])
{
    if (!object->simulating) return;
    for (size_t i = 0; i < object->movingEffectGroupCount; ++i) {
        collisionObjectEffectGroupSetTransform(&object->movingEffectGroups[i],frame,object->body.linearVelocity);
    }
    if (object->activationEffectsState == 1) {
        for (size_t i = 0; i < object->activationEffectGroupCount; ++i) {
            collisionObjectEffectGroupSetTransform(&object->activationEffectGroups[i],nullptr,object->body.linearVelocity);
        }
        object->activationEffectsState = 2;
    }
}

void collisionBreakableUpdateLifetime(CollisionSystem* system, CollisionBreakable* object)
{
    if (object->placement && !timerStarted(object->activatedTicks) && !object->shape.pairCount &&
        2.0f < elapsedSeconds(system->nowTicks, object->createdTicks)) {
        /* PAL2211EC..221218 constructs the restored static placement before
         * invoking the dynamic object's deleting destructor. */
        collisionPlacedObjectInitialize(system, object->placement);
        collisionBreakableDestroy(system, object);
        return;
    }
    if (timerStarted(object->activatedTicks) && object->definition->lifetime != 0.0f &&
        object->definition->lifetime <= elapsedSeconds(system->nowTicks, object->activatedTicks)) {
        /* PAL22128C..2212B8 calls the actual derived DC slot, then deletes
         * the same physical object. The base DC method is empty. */
        if (object->munition) chopperMunitionDetonate(object->munition);
        CollisionEffect* event = collisionObjectNewEffect(system, COLLISION_EFFECT_OBJECT_DESTROY,
            &object->participant);
        if (event) std::memcpy(event->point, object->body.motion.position, sizeof(event->point));
        collisionBreakableDestroy(system, object);
    }
}

void collisionBreakableWake(CollisionBreakable* object)
{
    object->shape.dirty=1;object->simulating=1;
    ++object->system->objects->simulatingCount;
    if (!object->movingEffectsStarted) {
        for (size_t i=0;i<object->movingEffectGroupCount;++i) {
            if (collisionObjectEffectGroupStart(object,&object->movingEffectGroups[i])!=COLLISION_SYSTEM_OK) return;
            collisionObjectEffectGroupRecord(object,&object->movingEffectGroups[i],COLLISION_EFFECT_GROUP_START);
        }
        object->movingEffectsStarted=1;
    }
    if (!timerStarted(object->activatedTicks)) object->activatedTicks=object->system->nowTicks;
}

void collisionBreakableStartActivationEffects(CollisionBreakable* object)
{
    for (size_t i=0;i<object->activationEffectGroupCount;++i) {
        if (collisionObjectEffectGroupStart(object,&object->activationEffectGroups[i])!=COLLISION_SYSTEM_OK) return;
        collisionObjectEffectGroupRecord(object,&object->activationEffectGroups[i],COLLISION_EFFECT_GROUP_START);
    }
    object->activationEffectsState=1;
}

void collisionObjectEffectGroupRecord(CollisionBreakable* object, CollisionObjectEffectGroup* emitter, CollisionEffectKind kind)
{
    /* These records are diagnostics. A full presentation queue cannot stop
     * the real CPU emitter or change its random-number sequence. */
    CollisionObjectRegistry* registry=object->system->objects;
    if (registry->effectCount==registry->effectCapacity) return;
    CollisionEffect* event=collisionObjectNewEffect(object->system,kind,&object->participant);
    if (event) {
        event->resourceKey=emitter->resourceKey;
        if (emitter->firstEmitter) {
            std::memcpy(event->frame,emitter->firstEmitter->frame,sizeof(event->frame));
            std::memcpy(event->velocity,emitter->firstEmitter->carVelocity,sizeof(event->velocity));
        }
    }
}


static float length3(const float value[4])
{
    const float x=value[0]*value[0],y=value[1]*value[1],z=value[2]*value[2];
    return sqrtf((x+y)+z);
}

static float secondsSince(int32_t now, int32_t before)
{
    const uint32_t bits=(uint32_t)now-(uint32_t)before;
    int32_t ticks;memcpy(&ticks,&bits,sizeof(ticks));
    return (float)ticks*0.0002500000118743628f;
}








static CollisionObjectEffectGroup* reserveEffectGroups(CollisionSystem* system, const uint32_t* keys, size_t count)
{
    if (!count) return nullptr;
    CollisionObjectRegistry* registry=system->objects;
    CollisionObjectEffectGroup* groups=registry->effectGroups;
    size_t capacity=registry->effectGroupCapacity;
    auto* block=registry->expandedEffectGroups;
    CollisionObjectEffectGroup* selected=nullptr;
    for (;;) {
      for (size_t start=0;start<=capacity;++start) {
        if (count>capacity-start) break;
        size_t freeCount=0;
        while (freeCount<count && !groups[start+freeCount].occupied) ++freeCount;
        if (freeCount==count) {
            selected=groups+start;break;
        }
        start+=freeCount;
      }
      if (selected) break;
      if (!block) {
        // PAL Smokeable's two cloned emitter lists have per-owner storage.
        // Growing the physical pool must not hit the old native57-body
        // handle reservation. Retain stable list addresses for every owner.
        std::unique_ptr<CollisionEffectGroupBlock> added(new(std::nothrow) CollisionEffectGroupBlock);
        if (!added) {system->result=COLLISION_SYSTEM_EFFECT_CAPACITY;return nullptr;}
        added->count=count;
        added->groups.reset(new(std::nothrow) CollisionObjectEffectGroup[count]());
        if (!added->groups) {system->result=COLLISION_SYSTEM_EFFECT_CAPACITY;return nullptr;}
        selected=added->groups.get();
        added->next=registry->expandedEffectGroups;
        registry->expandedEffectGroups=added.release();
        break;
      }
      groups=block->groups.get();capacity=block->count;block=block->next;
    }
    for (size_t i=0;i<count;++i) {
        CollisionObjectEffectGroup* emitter=selected+i;
        memset(emitter,0,sizeof(*emitter));
        const auto result=collisionObjectEffectGroupCreate(registry,emitter,keys[i]);
        if (result!=COLLISION_SYSTEM_OK) {
            for (size_t previous=0;previous<i;++previous)
                collisionObjectEffectGroupRelease(registry,selected+previous);
            system->result=result;return nullptr;
        }
    }
    return selected;
}

static void prepareStage(void* owner)
{
    collisionBreakablePrepare((CollisionBreakable*)owner);
}

void collisionObjectRegistryInitialize(CollisionObjectRegistry* registry,
                                       CollisionBreakable* objects, size_t capacity,
                                       CollisionPlacedObject* placements, size_t placementCount,
                                       CollisionObjectEffectGroup* effectGroups, size_t effectGroupCapacity,
                                       CollisionEffect* effects, size_t effectCapacity)
{
    memset(registry,0,sizeof(*registry));
    memset(objects,0,capacity*sizeof(*objects));
    memset(effectGroups,0,effectGroupCapacity*sizeof(*effectGroups));
    registry->objects=objects;registry->capacity=capacity;
    registry->totalObjectCapacity=capacity;
    for (size_t index=capacity;index>0;--index) {
        objects[index-1].nextFree=registry->freeObjects;
        registry->freeObjects=&objects[index-1];
    }
    registry->placements=placements;registry->placementCount=placementCount;
    registry->effectGroups=effectGroups;registry->effectGroupCapacity=effectGroupCapacity;
    registry->effects=effects;registry->effectCapacity=effectCapacity;
}

size_t collisionObjectRegistryFreeSlots(const CollisionObjectRegistry* registry)
{
    return registry ? registry->totalObjectCapacity-registry->occupiedObjects : 0;
}

void collisionObjectRegistryDisposeStorage(CollisionObjectRegistry* registry)
{
    // Call only after the real object destructors have returned all borrows.
    while (registry->expandedObjects) {
        auto* block=registry->expandedObjects;
        registry->expandedObjects=block->next;delete block;
    }
    while (registry->expandedEffectGroups) {
        auto* block=registry->expandedEffectGroups;
        registry->expandedEffectGroups=block->next;delete block;
    }
    registry->freeObjects=nullptr;
    registry->occupiedObjects=0;
    registry->totalObjectCapacity=registry->capacity;
    for (size_t index=registry->capacity;index>0;--index) {
        auto& slot=registry->objects[index-1];
        slot.nextFree=registry->freeObjects;registry->freeObjects=&slot;
    }
}

static CollisionBreakable* createGenericSmokeable(CollisionSystem* system,
    const CollisionBreakableDefinition* definition,TerrainTopology* topology,
    uint32_t modelKey,int32_t requestedId)
{
    auto* registry=system->objects;
    if (!registry->freeObjects) {
        // SlotPool::Malloc PAL25815C..258178 expands by the initial count/4+1,
        // not by the current expanded total. SpawnSmokeables starts with32.
        std::unique_ptr<CollisionObjectPoolBlock> added(new(std::nothrow) CollisionObjectPoolBlock);
        if (!added) {system->result=COLLISION_SYSTEM_OBJECT_CAPACITY;return nullptr;}
        added->count=registry->capacity/4+1;
        added->objects.reset(new(std::nothrow) CollisionBreakable[added->count]());
        if (!added->objects) {system->result=COLLISION_SYSTEM_OBJECT_CAPACITY;return nullptr;}
        for (size_t index=added->count;index>0;--index) {
            auto& slot=added->objects[index-1];
            slot.nextFree=registry->freeObjects;registry->freeObjects=&slot;
        }
        registry->totalObjectCapacity+=added->count;
        added->next=registry->expandedObjects;
        registry->expandedObjects=added.release();
    }
    auto* slot=registry->freeObjects;registry->freeObjects=slot->nextFree;
    auto* object=collisionBreakableConstruct(system,slot,definition,topology,modelKey,requestedId);
    if (!object) {
        slot->nextFree=registry->freeObjects;registry->freeObjects=slot;
        return nullptr;
    }
    object->genericPoolSlot=1;
    ++registry->occupiedObjects;
    return object;
}

CollisionSystemResult collisionPlacedObjectInitialize(CollisionSystem* system, CollisionPlacedObject* placement)
{
    if(!system || !system->space || !placement || !placement->definition ||
        !placement->definition->geometry || !placement->topology) {
        if(system)system->result=COLLISION_SYSTEM_MISSING_OWNER;
        return COLLISION_SYSTEM_MISSING_OWNER;
    }
    collisionShapeInitialize(system->space,&placement->shape,-1,2);
    placement->shape.active=0;
    placement->shape.geometry=placement->definition->geometry;
    placement->shape.vehicle.position=placement->frame+12;
    // VirginSmokeable PAL220064 builds an identity StaticCollisionBody
    // orientation, then220078 sets only its position. Its authored scenery
    // render frame is a different input consumed when it becomes dynamic.
    rotationMatrixIdentity(placement->staticOrientation);
    placement->shape.vehicle.orientation=placement->staticOrientation;
    memset(&placement->participant,0,sizeof(placement->participant));
    placement->participant.shape=&placement->shape;
    placement->activeObject=NULL;
    placement->occupied=1;
    // VirginSmokeable ctor PAL21FFA4/21FFAC exposes the same scenery
    // instance that its destructor hides; a copied flag cannot affect draw.
    if(placement->scenery)placement->scenery->flags&=static_cast<uint16_t>(~0x10u);
    collisionShapeSetBounds(system->space,&placement->shape,placement->minimum,placement->maximum);
    return COLLISION_SYSTEM_OK;
}

CollisionParticipant* collisionObjectParticipant(const CollisionSystem* system, const CollisionShape* shape)
{
    if (shape->id>=0 && shape->id<COLLISION_ID_COUNT) return system->participants[shape->id];
    if (system->objects)
        for (size_t i=0;i<system->objects->placementCount;++i)
            if (&system->objects->placements[i].shape==shape) return &system->objects->placements[i].participant;
    return NULL;
}

int32_t collisionBreakableChildrenAvailable(const CollisionBreakableDefinition* definition, uint32_t modelKey)
{
    for (size_t index=0;index<definition->fragmentCount;++index) {
        char suffix[8]="_CHILDx";
        suffix[6]=(char)('1'+index);
        const uint32_t key=hashStringContinue(suffix,modelKey);
        size_t art=0;
        while (art<definition->artKeyCount && definition->artKeys[art]!=key) ++art;
        if (art==definition->artKeyCount) return 0;
    }
    return 1;
}

CollisionBreakable* collisionBreakableCreate(CollisionSystem* system,
                                             const CollisionBreakableDefinition* definition,
                                             TerrainTopology* topology, uint32_t modelKey)
{
    if (!system->objects || !definition || !definition->body || !definition->geometry) {
        system->result=COLLISION_SYSTEM_MISSING_OWNER;
        return NULL;
    }
    return createGenericSmokeable(system,definition,topology,modelKey,-1);
}

CollisionBreakable* collisionBreakableConstruct(CollisionSystem* system,
    CollisionBreakable* object,const CollisionBreakableDefinition* definition,
    TerrainTopology* topology,uint32_t modelKey,int32_t requestedId)
{
    if (!system || !system->space || !system->objects || !object || object->occupied ||
        !definition || !definition->body || !definition->geometry || !topology) {
        if (system)system->result=COLLISION_SYSTEM_MISSING_OWNER;
        return NULL;
    }
    CollisionObjectRegistry* registry=system->objects;
    if(requestedId>=COLLISION_ID_COUNT || requestedId < -1 ||
        (requestedId>=0 && (system->space->ids.occupied[requestedId>>3]&(1u<<(requestedId&7))))) {
        system->result=COLLISION_SYSTEM_OWNER_FAILURE;return NULL;
    }
    const int32_t id=requestedId<0?collisionIdAcquire(&system->space->ids):requestedId;
    if (id<0) {system->result=COLLISION_SYSTEM_PARTICIPANT_CAPACITY;return NULL;}
    if(requestedId>=0)collisionIdMark(&system->space->ids,id);
    memset(object,0,sizeof(*object));
    object->system=system;object->definition=definition;object->modelKey=modelKey;
    object->occupied=1;object->createdTicks=system->nowTicks;
    object->activationImpulse=definition->activationImpulse;
    object->breakImpulse=collisionBreakableChildrenAvailable(definition,modelKey) ? definition->breakImpulse : 0.0f;
    object->activationEffectGroups=reserveEffectGroups(system,definition->activationEffectGroups,definition->activationEffectGroupCount);
    object->activationEffectGroupCount=definition->activationEffectGroupCount;
    object->movingEffectGroups=reserveEffectGroups(system,definition->movingEffectGroups,definition->movingEffectGroupCount);
    object->movingEffectGroupCount=definition->movingEffectGroupCount;
    if(system->result==COLLISION_SYSTEM_OK && registry->worldObjects) {
        // PAL22044C constructs the unlit embedded WorldObject with the
        // actual authored model key. SetInitialPosition/Move own its cells.
        object->visual=worldObjectCreate(registry->worldObjects,modelKey,nullptr,0);
        if(object->visual) {
            object->visual->borrowedFrame=object->displayFrame;
            object->visual->borrowedEnabled=&object->displayChanged;
        } else system->result=COLLISION_SYSTEM_OWNER_FAILURE;
    }
    if (system->result!=COLLISION_SYSTEM_OK) {
        if(object->visual)worldObjectDispose(registry->worldObjects,object->visual);
        if (object->activationEffectGroups)
            for (size_t i=0;i<object->activationEffectGroupCount;++i)
                collisionObjectEffectGroupRelease(registry,&object->activationEffectGroups[i]);
        if (object->movingEffectGroups)
            for (size_t i=0;i<object->movingEffectGroupCount;++i)
                collisionObjectEffectGroupRelease(registry,&object->movingEffectGroups[i]);
        object->occupied=0;collisionIdRelease(&system->space->ids,id);return NULL;
    }
    rigidBodyInitialize(&object->body,definition->body,&rigidBodyDefaultTuning);
    componentSystemInitialize(&object->components,&object->binding,1);
    componentSystemAdd(&object->components,componentBindRigidBody(&object->body));
    object->components.stageOwner=object;object->components.prepareStage=prepareStage;
    terrainSurfaceInitialize(&object->terrain,topology);
    collisionDynamicShapeInitialize(system->space,&object->shape,id,1,&object->terrain);
    object->shape.bodyMode=1;object->shape.body=&object->body;
    object->shape.geometry=definition->geometry;object->shape.dirty=0;
    object->participant.shape=&object->shape;object->participant.breakable=object;
    object->participant.terrain=&object->terrain;
    collisionShapeComputeLocalBounds(&object->shape);
    collisionShapeUpdateBounds(system->space,&object->shape);
    collisionSystemBind(system,&object->participant);
    return object;
}

void collisionBreakableSetTransform(CollisionBreakable* object, const float frame[16], const float velocity[4])
{
    static const float zero[4]={0,0,0,0};
    float orientation[16],position[4],linear[4],offset[4];
    memcpy(orientation,frame,sizeof(orientation));
    orientation[12]=orientation[13]=orientation[14]=0.0f;orientation[15]=1.0f;
    vectorMatrixTransformPoint(offset,orientation,object->definition->centerOfMass);
    for (int lane=0;lane<4;++lane) {
        position[lane]=frame[12+lane]+offset[lane];
        linear[lane]=velocity ? velocity[lane]*object->body.mass : 0.0f;
    }
    rigidBodySetMotion(&object->body,position,orientation,linear,zero);
    memcpy(object->displayFrame,frame,sizeof(object->displayFrame));
    object->displayFrame[14]=object->displayFrame[14]-object->elevationOffset;
    object->displayChanged=1;
    memcpy(object->shape.previousPosition,position,sizeof(position));
    memcpy(object->shape.previousFrame,orientation,sizeof(orientation));
    memcpy(object->shape.previousVelocity,zero,sizeof(zero));
    object->shape.previousTime=5000.0f;
    collisionBreakablePrepare(object);
    collisionShapeUpdateBounds(object->system->space,&object->shape);
    for (size_t i=0;i<object->activationEffectGroupCount;++i)
        collisionObjectEffectGroupSetTransform(&object->activationEffectGroups[i],frame,NULL);
    for (size_t i=0;i<object->movingEffectGroupCount;++i)
        collisionObjectEffectGroupSetTransform(&object->movingEffectGroups[i],frame,NULL);
}

CollisionParticipant* collisionBreakablePromote(CollisionSystem* system, CollisionParticipant* participant)
{
    CollisionPlacedObject* placement=NULL;
    for (size_t i=0;i<system->objects->placementCount;++i)
        if (&system->objects->placements[i].participant==participant) placement=&system->objects->placements[i];
    if (!placement) {system->result=COLLISION_SYSTEM_MISSING_OWNER;return NULL;}
    // The member PAL220194 checks bIsSlotPoolFull before its factory. The
    // wrapper PAL21FE90 still destroys the old virgin on a NULL result;
    // MaybeCreateWitnesses PAL1D79EC then skips this contact and continues.
    // Ordinary factories/fracture/replay use the expandable allocator instead.
    CollisionBreakable* object=nullptr;
    if (collisionObjectRegistryFreeSlots(system->objects)) {
        object=collisionBreakableCreate(system,placement->definition,placement->topology,placement->modelKey);
        if (!object)return nullptr; // retain genuine native owner failures
    }
    if (object) {
        float frame[16];memcpy(frame,placement->frame,sizeof(frame));
        TerrainSurface surface;terrainSurfaceInitialize(&surface,placement->topology);
        bool found;float normal[4];
        const float elevation=terrainSurfaceElevation(&surface,frame+12,NULL,normal,&found);
        if (found) {
            const float bottom=object->shape.localMinimum[2]+object->definition->centerOfMass[2];
            const float projected=bottom*normal[2];
            const float lift=(elevation-(frame[14]+projected))+0.05f;
            if (0.0f<lift) {object->elevationOffset=lift;frame[14]=frame[14]+lift;}
        }
        terrainSurfaceDispose(&surface);
        object->placement=placement;placement->activeObject=object;
        collisionBreakableSetTransform(object,frame,NULL);
    }
    placement->shape.previous->next=placement->shape.next;
    placement->shape.next->previous=placement->shape.previous;
    --system->space->shapeCount;placement->occupied=0;
    placement->shape.previous=placement->shape.next=nullptr;
    // PAL2200E0/2200E8: retiring the VirginSmokeable hides its old static
    // placement. The newly created physical object's actual model remains.
    if(placement->scenery)placement->scenery->flags|=0x10u;
    return object ? &object->participant : NULL;
}

int32_t collisionBreakableRestClass(const CollisionBreakable* object)
{
    const float linear=length3(object->body.linearVelocity),angular=length3(object->body.angularVelocity);
    if (0.7f<linear || 0.7f<angular) return 0;
    if (!object->shape.contactCount) return 0;
    if (0.25f<linear || 0.25f<angular) return 1;
    return 2.0f<secondsSince(object->system->nowTicks,object->recentCarContactTicks) ? 1 : 2;
}

void collisionBreakablePrepare(CollisionBreakable* object)
{
    componentSystemPrepare(&object->components);
    const int32_t rest=collisionBreakableRestClass(object);
    if (rest==1 || rest==2) {
        for (int lane=0;lane<4;++lane) {
            object->body.motion.linearMomentum[lane]=rest==1 ? object->body.motion.linearMomentum[lane]*0.95f : 0.0f;
            object->body.motion.angularMomentum[lane]=rest==1 ? object->body.motion.angularMomentum[lane]*0.95f : 0.0f;
        }
        componentSystemPrepare(&object->components);
    }
}



void collisionBreakableSleep(CollisionBreakable* object)
{
    object->simulating=0;object->shape.dirty=0;
    --object->system->objects->simulatingCount;
    terrainSurfaceReset(&object->terrain);
}



float collisionBreakableImpulseFraction(CollisionBreakable* object, const float impulse[4], const float lever[4])
{
    float fraction=1.0f;
    if (!object->simulating) {
        if (object->activationImpulse!=0.0f) {
            float arm[4],moment[4];
            for (int lane=0;lane<4;++lane) arm[lane]=lever[lane]+object->definition->centerOfMass[lane];
            for (int lane=0;lane<3;++lane) {
                const int next=(lane+1)%3,last=(lane+2)%3;
                const float positive=arm[next]*impulse[last],negative=impulse[next]*arm[last];
                moment[lane]=positive-negative;
            }
            moment[3]=0.0f;
            const float strength=maximumFinite(length3(moment),length3(impulse));
            if (strength<object->activationImpulse) return fraction;
            fraction=object->activationImpulse/strength;
            if (!object->system->contactAudio) {
                object->system->result=COLLISION_SYSTEM_MISSING_OWNER;
                return fraction;
            }
            if (collisionAudioPlaySmokeable(object->system->contactAudio,object->body.motion.position,
                    object->definition,object->modelKey)!=WORLD_AUDIO_OK) {
                object->system->result=COLLISION_SYSTEM_OWNER_FAILURE;
                return fraction;
            }
            CollisionEffect* event=object->system->objectEffectsEnabled ?
                collisionObjectNewEffect(object->system,COLLISION_EFFECT_OBJECT_ACTIVATION,&object->participant) : NULL;
            if (event) {
                event->materialIndex=object->definition->materialIndex;
                memcpy(event->point,object->body.motion.position,sizeof(event->point));
            }
            float opposing[4],centerLever[4];
            for (int lane=0;lane<4;++lane) {
                opposing[lane]=impulse[lane]*(-fraction);
                centerLever[lane]=0.0f-object->definition->centerOfMass[lane];
            }
            centerLever[3]=centerLever[3]+1.0f;
            collisionBreakableWake(object);
            object->activationImpulse=0.0f;
            rigidBodyApplyImpulse(&object->body,opposing,centerLever);
            collisionBreakableStartActivationEffects(object);
        } else collisionBreakableWake(object);
    }
    if (object->breakImpulse!=0.0f) {
        const float xy=fabsf(impulse[0])+fabsf(impulse[1]);
        const float sum=xy+fabsf(impulse[2]);
        const float strength=fraction*sum;
        if (object->breakImpulse<=strength) {
            const float remaining=object->breakImpulse;
            object->breakImpulse=-1.0f;
            fraction=finiteRatio((-fraction)*remaining,strength);
        } else object->breakImpulse=object->breakImpulse-strength*0.5f;
    }
    return fraction;
}

static int hasCarContact(const CollisionSystem* system, const CollisionParticipant* participant)
{
    for (const CollisionPair* pair=system->pairRoot.next;pair!=&system->pairRoot;pair=pair->next)
        if (pair->impact && (pair->first==participant || pair->second==participant)
                && (pair->first->movement || pair->second->movement)) return 1;
    return 0;
}

void collisionBreakableAdvance(CollisionSystem* system, CollisionBreakable* object, float elapsed)
{
    collisionShapeCapture(&object->shape,elapsed);
    if (object->simulating) {
        rigidBodySetCenterOfMass(&object->body,NULL);
        rigidBodyRefreshMass(&object->body);
        const int drag=object->modelKey==UINT32_C(0x98C6023C) || object->modelKey==UINT32_C(0x8AC70E76);
        object->body.dampingEnabled=1;
        const int32_t steps=(int32_t)(elapsed/0.01f)+1;
        const float substep=elapsed/(float)steps;
        for (int32_t step=0;step<steps;++step) {
            if (drag) {
                const float scale=(-length3(object->body.linearVelocity))*0.01f;
                float force[4];
                for (int lane=0;lane<4;++lane) force[lane]=object->body.linearVelocity[lane]*scale;
                const float lever[4]={0,0,0,0};
                rigidBodyApplyForce(&object->body,force,lever);
            }
            componentSystemIntegrateEuler(&object->components,substep,object->derivatives,29);
            motionMatrixOrthonormalize(object->body.motion.orientation,object->body.motion.orientation);
            // Native floating-point adaptation: Euler's source Prepare runs
            // before the source orientation projection. Refresh this body's
            // cached auxiliaries from the published orthonormal frame so a
            // small-inertia debris body cannot feed stale amplified angular
            // velocity into the next native step. Do not repeat the Smokeable
            // rest adjustment or apply another impulse/force integration.
            rigidBodyPrepareStep(&object->body);
        }
        if (2.0f<secondsSince(system->nowTicks,object->recentCarContactTicks) && object->shape.contactCount) {
            const CollisionImpact* firstImpact=NULL;
            for (CollisionPair* pair=system->pairRoot.next;pair!=&system->pairRoot;pair=pair->next)
                if (pair->impact && (pair->first==&object->participant || pair->second==&object->participant)) {
                    firstImpact=pair->impact;break;
                }
            if (firstImpact && 0.5f<secondsSince(system->nowTicks,firstImpact->createdTicks)
                    && length3(object->body.linearVelocity)<1.5f && length3(object->body.angularVelocity)<4.0f)
                collisionBreakableSleep(object);
        }
        collisionBreakableUpdateTerrain(object);
        if (hasCarContact(system,&object->participant)) object->recentCarContactTicks=system->nowTicks;
    }
    float offset[4];
    vectorMatrixTransformPoint(offset,object->body.motion.orientation,object->definition->centerOfMass);
    float frame[16];
    memcpy(frame,object->body.motion.orientation,sizeof(frame));
    for (int lane=0;lane<4;++lane) frame[12+lane]=object->body.motion.position[lane]-offset[lane];
    frame[14]=frame[14]-object->elevationOffset;frame[15]=1.0f;
    object->displayChanged=0;
    if (!object->responseSuppressed || (system->frame&1u)) {
        memcpy(object->displayFrame,frame,sizeof(frame));object->displayChanged=1;
    }
    for (int lane=0;lane<4;++lane) frame[12+lane]=frame[12+lane]+offset[lane];
    frame[15]=1.0f;
    collisionBreakableUpdateEffects(object,frame);
    collisionBreakableUpdateLifetime(system,object);
    if (!object->occupied) return;
    if (object->activationImpulse==0.0f) {
        object->elevationOffset=object->elevationOffset-elapsed*0.5f;
        if (object->elevationOffset<0.0f) object->elevationOffset=0.0f;
    }
}

size_t collisionBreakableFracture(CollisionSystem* system, CollisionBreakable* object,
                                  CollisionParticipant** output, size_t capacity)
{
    const size_t count=object->definition->fragmentCount;
    if (count>capacity) {system->result=COLLISION_SYSTEM_PARTICIPANT_CAPACITY;return 0;}
    for (size_t index=0;index<count;++index) {
        char suffix[8]="_CHILDx";
        suffix[6]=(char)('1'+index);
        const uint32_t key=hashStringContinue(suffix,object->modelKey);
        CollisionBreakable* fragment=collisionBreakableCreate(system,object->definition->fragments[index],
                                                              object->terrain.topology,key);
        if (!fragment) return index;
        collisionBreakableSetTransform(fragment,object->displayFrame,NULL);
        collisionBreakableWake(fragment);
        output[index]=&fragment->participant;
    }
    collisionBreakableDestroy(system,object);
    return count;
}

void collisionBreakableDestroy(CollisionSystem* system, CollisionBreakable* object)
{
    if (object->munition) {
        ChopperMunition* actor = object->munition;
        object->munition = NULL;
        chopperMunitionPhysicalDestroyed(actor, object);
    }
    collisionQueueRemoveShape(system,&object->shape);
    CollisionPair* pair=system->pairRoot.next;
    while (pair!=&system->pairRoot) {
        CollisionPair* next=pair->next;
        if (pair->first==&object->participant || pair->second==&object->participant) collisionPairDestroy(system,pair);
        pair=next;
    }
    CollisionObjectEffectGroup* lists[2]={object->activationEffectGroups,object->movingEffectGroups};
    const size_t counts[2]={object->activationEffectGroupCount,object->movingEffectGroupCount};
    for (int list=0;list<2;++list)
        for (size_t i=0;i<counts[list];++i) {
            CollisionObjectEffectGroup* emitter=&lists[list][i];
            if (emitter->started) collisionObjectEffectGroupRecord(object,emitter,COLLISION_EFFECT_GROUP_STOP);
            collisionObjectEffectGroupRelease(system->objects,emitter);
        }
    terrainSurfaceDispose(&object->terrain);
    object->shape.previous->next=object->shape.next;
    object->shape.next->previous=object->shape.previous;
    const int32_t id=object->shape.id;
    collisionIdRelease(&system->space->ids,id);
    system->space->byId[id]=NULL;system->participants[id]=NULL;
    --system->space->shapeCount;--system->space->dynamicCount;
    if (object->placement) object->placement->activeObject=NULL;
    // PAL22080C destroys the same embedded WorldObject after FX retirement.
    if(object->visual)worldObjectDispose(system->objects->worldObjects,object->visual);
    object->visual=nullptr;
    object->occupied=0;
    if (object->genericPoolSlot) {
        auto* registry=system->objects;
        object->genericPoolSlot=0;
        object->nextFree=registry->freeObjects;registry->freeObjects=object;
        --registry->occupiedObjects;
    }
}

// Full PAL DeleteFarAwaySmokeableCollisionBodies21F610 and SmokeableThink
// 21F5E8. Staged retirement preserves the pre-destruction free-slot count.
static int alreadyRetired(CollisionBreakable* const* retired,size_t count,const CollisionBreakable* object)
{
    for (size_t index=0;index<count;++index) if (retired[index]==object) return 1;
    return 0;
}

void worldServicesCullBreakables(CollisionSystem* system)
{
    if (!system->objects) return;
    CollisionBreakable* retired[COLLISION_ID_COUNT];
    size_t count=0;
    for (CollisionShape* shape=system->space->root.next;shape!=&system->space->root;shape=shape->next) {
        if (shape->kind!=1) continue;
        CollisionParticipant* participant=collisionObjectParticipant(system,shape);
        CollisionBreakable* object=participant->breakable;
        if ((!object->definition->retainWhenDistant && speed(object->body.linearVelocity)>1000.0f) ||
                fabsf(object->body.motion.orientation[0])>50.0f) retired[count++]=object;
    }
    if (collisionObjectRegistryFreeSlots(system->objects)<10) {
        for (CollisionShape* shape=system->space->root.next;shape!=&system->space->root;shape=shape->next) {
            if (shape->kind!=1) continue;
            CollisionParticipant* participant=collisionObjectParticipant(system,shape);
            CollisionBreakable* object=participant->breakable;
            if (alreadyRetired(retired,count,object) || object->simulating || object->definition->retainWhenDistant) continue;
            float age=0;
            if (object->activatedTicks && object->activatedTicks!=INT32_MAX) {
                const uint32_t bits=(uint32_t)system->nowTicks-(uint32_t)object->activatedTicks;
                int32_t ticks;memcpy(&ticks,&bits,sizeof(ticks));
                age=(float)ticks*0.0002500000118743628f;
            }
            if (age>45.0f && worldGetDistanceToClosestPlayer3D(system->world,object->body.motion.position)>120.0f)
                retired[count++]=object;
        }
    }
    for (size_t index=0;index<count;++index) collisionBreakableDestroy(system,retired[index]);
}

void worldServicesPeriodicObjectCull(CollisionSystem* system)
{
    if (((system->frame+77u)&127u)==0) worldServicesCullBreakables(system);
}

void collisionCarRecordImpact(CollisionParticipant* participant, const float impulse[4], const float lever[4])
{
    const float x=impulse[0]*impulse[0],y=impulse[1]*impulse[1],z=impulse[2]*impulse[2];
    carMovementRecordCollisionImpact(participant->movement,impulse,lever,sqrtf((x+y)+z));
}

// Same retained34026/34027 native decoder; definitions do not borrow terrain.
#include "indep/inc/TerrainType.hpp"
#include "bware/inc/bChunk.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace hp2 {
namespace {
float real(const std::uint8_t* bytes)
{
    const auto bits = ReadU32(bytes);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    Require(std::isfinite(value), "Collision object contains a non-finite parameter");
    return value;
}

std::int16_t half(const std::uint8_t* bytes)
{
    const auto bits = ReadU16(bytes);
    std::int16_t value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::size_t aligned(const Chunk& chunk, std::size_t recordBytes)
{
    const auto start = (chunk.payloadOffset + 15u) & ~std::size_t(15u);
    Require(start <= chunk.endOffset && (chunk.endOffset - start) % recordBytes == 0,
            "Collision scene has a partial aligned record");
    return start;
}

std::string text(const std::uint8_t* bytes, std::size_t capacity)
{
    const auto* end = std::find(bytes, bytes + capacity, std::uint8_t(0));
    Require(end != bytes + capacity, "Collision object name is not terminated");
    return {reinterpret_cast<const char*>(bytes), static_cast<std::size_t>(end - bytes)};
}

struct SceneGroup {
    Chunk models{}, instances{};
};
} // namespace

CollisionObjectData::CollisionObjectData(const std::vector<std::uint8_t>& track, TerrainTopology& topology)
    : CollisionObjectData(track,&topology) {}

CollisionObjectData::CollisionObjectData(const std::vector<std::uint8_t>& track, TerrainTopology* topology)
    : geometry_(ReadCollisionResources(track, true))
{
    std::vector<Chunk> definitionChunks, placementChunks;
    std::map<std::int32_t,SceneGroup> scene;
    const auto visit = [&](const auto& self, std::size_t start, std::size_t size, unsigned depth) -> void {
        Require(depth <= 32, "Collision scene nesting exceeds 32 levels");
        for (const auto& chunk : ReadChunks(track, start, size)) {
            if (chunk.id == 0x34026u) definitionChunks.push_back(chunk);
            else if (chunk.id == 0x34027u) placementChunks.push_back(chunk);
            else if (chunk.id == 0x34003u) {
                const auto dataStart = (chunk.payloadOffset + 15u) & ~std::size_t(15u);
                Require(chunk.endOffset - dataStart >= 12, "Truncated scene model header");
                artKeys_.push_back(ReadU32(track.data() + dataStart + 8));
            } else if (chunk.id == 0x80034100u) {
                std::int32_t number = -1;
                SceneGroup group;
                for (const auto& child : ReadChunks(track, chunk.payloadOffset, chunk.size)) {
                    if (child.id == 0x34101u) {
                        const auto header = (child.payloadOffset + 15u) & ~std::size_t(15u);
                        Require(header <= child.endOffset && child.endOffset - header >= 12,
                                "Truncated scenery group header");
                        number = static_cast<std::int32_t>(ReadU32(track.data() + header + 8));
                    } else if (child.id == 0x34102u) group.models = child;
                    else if (child.id == 0x34103u) group.instances = child;
                }
                Require(number >= 0 && group.models.id && group.instances.id && scene.emplace(number, group).second,
                        "Incomplete or repeated scenery group");
            } else if (chunk.IsContainer()) self(self, chunk.payloadOffset, chunk.size, depth + 1);
        }
    };
    visit(visit, 0, track.size(), 0);
    std::sort(artKeys_.begin(), artKeys_.end());
    artKeys_.erase(std::unique(artKeys_.begin(), artKeys_.end()), artKeys_.end());
    std::set<std::uint32_t> keys;
    for (const auto& chunk : definitionChunks) {
        for (auto cursor = aligned(chunk, 224); cursor < chunk.endOffset; cursor += 224) {
            const auto* bytes = track.data() + cursor;
            auto definition = std::make_unique<Definition>();
            definition->name = text(bytes + 8, 24);
            definition->key = ReadU32(bytes + 112);
            Require(keys.insert(definition->key).second && hashString(definition->name.c_str()) == definition->key,
                    "Collision object definition repeats a key or has a name/hash mismatch");
            definition->body.mass = real(bytes + 32);
            Require(definition->body.mass > 0.0f, "Collision object has nonpositive mass");
            for (int lane = 0; lane < 16; ++lane) definition->body.inertia[lane] = real(bytes + 48 + lane * 4);
            auto& out = definition->native;
            out.name = definition->name.c_str();
            out.body = &definition->body;
            for (int lane = 0; lane < 4; ++lane) out.centerOfMass[lane] = real(bytes + 160 + lane * 4);
            out.lifetime = real(bytes + 176);
            out.activationImpulse = real(bytes + 180);
            out.breakImpulse = real(bytes + 184);
            out.restitution = real(bytes + 188);
            out.materialIndex = terrainTypeMatchKey(ReadU32(bytes + 196));
            out.friction = real(bytes + 200);
            out.retainWhenDistant = ReadU32(bytes + 204) != 0;
            out.fragmentCount = ReadU32(bytes + 124);
            Require(out.fragmentCount <= definition->fragments.size(), "Collision object has too many fragment definitions");
            for (int child = 0; child < 4; ++child) definition->fragmentKeys[child] = ReadU32(bytes + 144 + child * 4);
            out.fragments = definition->fragments.data();
            out.artKeys = artKeys_.data();
            out.artKeyCount = artKeys_.size();
            definition->activationGroups[0] = ReadU32(bytes + 116);
            definition->movingGroups[0] = ReadU32(bytes + 120);
            out.activationEffectGroups = definition->activationGroups.data();
            out.activationEffectGroupCount = definition->activationGroups[0] ? 1u : 0u;
            out.movingEffectGroups = definition->movingGroups.data();
            out.movingEffectGroupCount = definition->movingGroups[0] ? 1u : 0u;
            for (const auto& hull : geometry_) {
                if (hull->Key() != definition->key) continue;
                hull->Recenter(out.centerOfMass);
                out.geometry = &hull->Geometry();
                break;
            }
            // A catalogue entry may be unused, but a physical instance must
            // have its authored hull. Both registration and dynamic creation
            // reject a missing hull before acquiring any owners/IDs. A generic
            // three-metre cube was not an evidenced collision representation.
            definitions_.push_back(std::move(definition));
        }
    }
    for (const auto& definition : definitions_) {
        for (std::size_t child = 0; child < definition->native.fragmentCount; ++child) {
            definition->fragments[child] = Find(definition->fragmentKeys[child]);
            Require(definition->fragments[child] != nullptr, "Collision fragment definition is missing");
        }
    }
    for (const auto& chunk : placementChunks) {
        Require(chunk.size % 8 == 0, "Collision placement array has a partial record");
        for (auto cursor = chunk.payloadOffset; cursor < chunk.endOffset; cursor += 8) {
            const auto* record = track.data() + cursor;
            const auto* definition = Find(ReadU32(record));
            if (!definition) continue;
            const auto group = scene.find(half(record + 4));
            Require(group != scene.end(), "Collision placement references an unknown scenery group");
            const auto& models = group->second.models;
            const auto& instances = group->second.instances;
            Require(models.size % 40 == 0, "Scenery model array has a partial record");
            const auto start = aligned(instances, 48);
            const int instanceIndex = half(record + 6);
            Require(instanceIndex >= 0 && static_cast<std::size_t>(instanceIndex) < (instances.endOffset - start) / 48,
                    "Collision placement references an unknown scenery instance");
            const auto* instance = track.data() + start + static_cast<std::size_t>(instanceIndex) * 48;
            const int modelIndex = half(instance + 12);
            Require(modelIndex >= 0 && static_cast<std::size_t>(modelIndex) < models.size / 40,
                    "Collision scenery instance references an unknown model");
            CollisionPlacedObject placement{};
            placement.definition = definition;
            Require(topology!=nullptr,"Collision placement requires its actual terrain topology");
            placement.topology = topology;
            placement.sceneryFlags = static_cast<std::uint16_t>(half(instance + 14));
            placement.scenerySection=half(record+4);
            placement.sceneryInstance=instanceIndex;
            placement.modelKey = ReadU32(track.data() + models.payloadOffset + static_cast<std::size_t>(modelIndex) * 40);
            int16_t rotation[9];
            for(int lane=0;lane<9;++lane)rotation[lane]=half(instance+28+lane*2);
            sceneryNormalizeSmokeableRotation(rotation);
            for (int axis = 0; axis < 3; ++axis) {
                for (int lane = 0; lane < 3; ++lane)
                    // ConvertToSmokeable PAL2201AC loads0x38800000,
                    // exactly the render scale1/16384 for all nine lanes.
                    // Using1/32768 here halves the promoted visible model.
                    placement.frame[axis * 4 + lane] = static_cast<float>(rotation[axis*3+lane]) * 0.00006103515625f;
                placement.frame[12 + axis] = real(instance + 16 + axis * 4);
            }
            placement.frame[15] = 1.0f;
            placement.minimum[0] = static_cast<float>(half(instance));
            placement.minimum[1] = static_cast<float>(half(instance + 2));
            placement.maximum[0] = static_cast<float>(half(instance + 6));
            placement.maximum[1] = static_cast<float>(half(instance + 8));
            placements_.push_back(placement);
        }
    }
}

CollisionObjectData::~CollisionObjectData() = default;

const CollisionBreakableDefinition* CollisionObjectData::Find(std::uint32_t key) const noexcept
{
    for (const auto& definition : definitions_) if (definition->key == key) return &definition->native;
    return nullptr;
}

} // namespace hp2

#include "indep/inc/RaceParameters.hpp"
#include <array>
#include <stdexcept>

namespace hp2 {
const CollisionBreakableDefinition& CollisionObjectData::DefinitionAt(std::size_t index) const
{
    return definitions_.at(index)->native;
}

namespace {
void disposePlacedObject(CollisionSystem& collision,CollisionPlacedObject& placement)
{
    collisionQueueRemoveShape(&collision,&placement.shape);
    for(auto* pair=collision.pairRoot.next;pair!=&collision.pairRoot;) {
        auto* next=pair->next;
        if(pair->first==&placement.participant || pair->second==&placement.participant)
            collisionPairDestroy(&collision,pair);
        pair=next;
    }
    placement.shape.previous->next=placement.shape.next;
    placement.shape.next->previous=placement.shape.previous;
    placement.shape.next=placement.shape.previous=nullptr;
    --collision.space->shapeCount;
    placement.occupied=0;placement.activeObject=nullptr;
    if(placement.scenery)placement.scenery->flags|=0x10u;
}

// DeleteSmokeableCollisionBodies, full PAL21F508..21F5E4. Detach all kinds1/2
// in source order first, then reinsert each actual node for its destructor.
// No secondary physical object or registry is introduced by the temporary list.
void deleteSmokeableCollisionBodies(CollisionSystem& collision)
{
    CollisionShape pending{};pending.next=pending.previous=&pending;
    auto& root=collision.space->root;
    for(auto* shape=root.next;shape!=&root;) {
        auto* next=shape->next;
        if(shape->kind==1 || shape->kind==2) {
            shape->previous->next=shape->next;shape->next->previous=shape->previous;
            shape->previous=pending.previous;shape->next=&pending;
            pending.previous->next=shape;pending.previous=shape;
        }
        shape=next;
    }
    while(pending.next!=&pending) {
        auto* shape=pending.next;
        shape->previous->next=shape->next;shape->next->previous=shape->previous;
        shape->next=root.next;shape->previous=&root;
        root.next->previous=shape;root.next=shape;
        if(shape->kind==1) {
            auto* participant=collisionObjectParticipant(&collision,shape);
            Require(participant && participant->breakable,"Smokeable list lost its actual physical owner");
            collisionBreakableDestroy(&collision,participant->breakable);
        } else {
            auto& registry=*collision.objects;
            std::size_t index=0;
            while(index<registry.placementCount && &registry.placements[index].shape!=shape)++index;
            Require(index<registry.placementCount,"VirginSmokeable lost its authored placement");
            disposePlacedObject(collision,registry.placements[index]);
        }
    }
}

// Full VirginSmokeable.DoSnapshot220128..220174; StaticCollisionBody's
// 1D8760 is only a tail to CollisionBody.DoSnapshot, with no extra bounds.
void virginSmokeableDoSnapshot(CollisionPlacedObject& object,ReplaySnapshot& snapshot)
{
    replaySnapshotBeginSection(&snapshot,nullptr,"VirginSmokeable");
    collisionShapeSnapshot(&object.shape,&snapshot);
    replaySnapshotEndSection(&snapshot);
}

// PAL21EC98 creator dispatch. Its final registrations are closed to the
// generic, Rocket, SmokeBomb and DroppedSpikes native typed pools. Body ID,
// definition and art key come from the stream, never from a replacement car.
CollisionBreakable* recreateSmokeable(CollisionSystem& collision,
    ChopperMunitionRegistry& munitions,int32_t type,int32_t id,
    const CollisionBreakableDefinition& definition,uint32_t model)
{
    if(type==0) {
        return createGenericSmokeable(&collision,&definition,munitions.bindings.terrain,model,id);
    }
    const auto kind=type==1?CHOPPER_MUNITION_ROCKET:
        type==2?CHOPPER_MUNITION_SMOKE_BOMB:CHOPPER_MUNITION_SPIKE_BOMB;
    auto* actor=chopperMunitionConstruct(&munitions,kind,id,&definition,model);
    return actor?actor->physical:nullptr;
}
}

struct SmokeableScene::Storage {
    CollisionSystem* collision;
    CollisionObjectData* data;
    CollisionObjectRegistry registry{};
    std::array<CollisionBreakable,32> bodies{};
    std::vector<CollisionObjectEffectGroup> effectGroups;
    std::vector<CollisionObjectEffectGroup> prototypes;
    std::size_t spawned=0;
    bool attached=false;

    Storage(CollisionSystem& c,CollisionObjectData& d):collision(&c),data(&d) {}
    ~Storage()
    {
        if (!attached)return;
        // ClearSmokeables21FAA8 uses the same complete deletion traversal as
        // replay restoration, then releases its actual definition prototypes.
        deleteSmokeableCollisionBodies(*collision);
        for (auto& prototype:prototypes)
            if (prototype.occupied)collisionObjectEffectGroupRelease(&registry,&prototype);
        collisionObjectRegistryDisposeStorage(&registry);
        if (collision->objects==&registry)collision->objects=nullptr;
    }
};

SmokeableScene::SmokeableScene(CollisionSystem& collision,CollisionObjectData& data,
    ChopperMunitionEffects& effects,const RaceParameters& parameters,
    Scenery* scenery,WorldObjectRegistry* worldObjects)
    :storage_(std::make_unique<Storage>(collision,data))
{
    Require(collision.space && !collision.objects && effects.particles,
        "Smokeable scene requires its unbound canonical collision and live FX owners");
    for (std::size_t i=0;i<data.PlacementCount();++i)
        Require(!data.Placements()[i].occupied && !data.Placements()[i].activeObject,
            "A second scene cannot own the same loaded placement");
    auto& s=*storage_;
    // Headless component fixtures may omit both drawing owners. The real
    // World supplies both canonical owners; partial bindings are rejected.
    Require((scenery!=nullptr)==(worldObjects!=nullptr),"Smokeable drawing owners are only partly bound");
    for(std::size_t i=0;i<data.PlacementCount();++i) {
        auto& placement=data.Placements()[i];
        placement.scenery=scenery?sceneryFindInstanceByIndex(scenery,
            placement.scenerySection,placement.sceneryInstance):nullptr;
        Require(!scenery || placement.scenery,"Smokeable spawner lost its actual scenery instance");
        if(placement.scenery) {
            // The real scene owns the post-reverse, rebuilt orientation.
            // Promotion and snapshot recreation consume the same generation
            // as its already prepared static render/footprint caches.
            for(int row=0;row<3;++row) {
                for(int lane=0;lane<3;++lane)
                    placement.frame[row*4+lane]=static_cast<float>(placement.scenery->rotation[row*3+lane])/16384.0f;
                placement.frame[row*4+3]=0.0f;
                placement.frame[12+row]=placement.scenery->position[row];
            }
            placement.frame[15]=1.0f;
        }
    }
    std::size_t maximumGroups=0,prototypeCount=0;
    for (std::size_t i=0;i<data.DefinitionCount();++i) {
        const auto& definition=data.DefinitionAt(i);
        const auto count=definition.activationEffectGroupCount+definition.movingEffectGroupCount;
        maximumGroups=std::max(maximumGroups,count);
        prototypeCount+=count;
    }
    //32 generic bases and the separate3/14/8 derived bases share authored
    //effect-group handles. No second copy of a physical object is allocated.
    s.effectGroups.resize(std::max<std::size_t>(2,maximumGroups)*57);
    s.prototypes.resize(prototypeCount);
    collisionObjectRegistryInitialize(&s.registry,s.bodies.data(),s.bodies.size(),
        data.Placements(),data.PlacementCount(),s.effectGroups.data(),s.effectGroups.size(),nullptr,0);
    s.registry.worldObjects=worldObjects;
    Require(collisionObjectRegistryBindEffects(&s.registry,&effects)==COLLISION_SYSTEM_OK,
        "Smokeable FX pool rejected its same-particle-owner binding");
    collision.objects=&s.registry;
    s.attached=true;
    std::size_t prototype=0;
    for (std::size_t i=0;i<data.DefinitionCount();++i) {
        const auto& definition=data.DefinitionAt(i);
        const uint32_t* lists[2]={definition.activationEffectGroups,definition.movingEffectGroups};
        const std::size_t counts[2]={definition.activationEffectGroupCount,definition.movingEffectGroupCount};
        for (int list=0;list<2;++list)for (std::size_t index=0;index<counts[list];++index) {
            Require(collisionObjectEffectGroupCreate(&s.registry,&s.prototypes[prototype++],lists[list][index])==COLLISION_SYSTEM_OK,
                "Smokeable definition requires an unavailable final effect group");
        }
    }
    // The release initializes the32-slot pool and all prototype lists even
    // with zero placements. Its mode2 branch alone skips scenery flag1.
    for (std::size_t i=0;i<data.PlacementCount();++i) {
        CollisionPlacedObject& placement=data.Placements()[i];
        Require(!placement.occupied && !placement.activeObject,
            "A second scene cannot own the same loaded placement");
        if (parameters.mode==2 && (placement.sceneryFlags&1u))continue;
        Require(collisionPlacedObjectInitialize(&collision,&placement)==COLLISION_SYSTEM_OK,
            "Final authored placement failed its canonical collision registration");
        ++s.spawned;
    }
}

SmokeableScene::~SmokeableScene()=default;
CollisionObjectRegistry& SmokeableScene::Registry() noexcept {return storage_->registry;}
std::size_t SmokeableScene::SpawnedPlacements() const noexcept {return storage_->spawned;}

// SnapshotAllSmokeables, full PAL21FB30..21FE90. The three final CheckPoint
// calls13D0F8 have empty diagnostic bodies; they add no stream bytes.
void SmokeableScene::DoSnapshot(ReplaySnapshot& snapshot,ChopperMunitionRegistry& munitions,
    const CollisionObjectData& inGameObjects)
{
    auto& state=*storage_;auto& collision=*state.collision;auto& registry=state.registry;
    Require(collision.objects==&registry && munitions.bindings.collisions==&collision &&
        munitions.bindings.terrain,"Smokeable snapshot requires the same live collision/munition owners");
    replaySnapshotBeginSection(&snapshot,nullptr,"AllSmokeables");
    replaySnapshotMoveInteger(&snapshot,&registry.simulatingCount,4);
    if(snapshot.restoring && snapshot.result==REPLAY_SNAPSHOT_OK) {
        deleteSmokeableCollisionBodies(collision);state.spawned=0;
        for(;;) {
            uint32_t index=0;replaySnapshotMoveUnsigned(&snapshot,&index,2);
            if(snapshot.result!=REPLAY_SNAPSHOT_OK || index==0xffffu)break;
            if(index>=registry.placementCount || registry.placements[index].occupied) {
                snapshot.result=REPLAY_SNAPSHOT_INVALID_TRANSFER;break;
            }
            auto& placement=registry.placements[index];
            if(collisionPlacedObjectInitialize(&collision,&placement)!=COLLISION_SYSTEM_OK) {
                snapshot.result=REPLAY_SNAPSHOT_INVALID_BINDING;break;
            }
            ++state.spawned;virginSmokeableDoSnapshot(placement,snapshot);
        }
        while(snapshot.result==REPLAY_SNAPSHOT_OK) {
            uint32_t id=0;replaySnapshotMoveUnsigned(&snapshot,&id,2);
            if(snapshot.result!=REPLAY_SNAPSHOT_OK || id==0xffffu)break;
            uint32_t definitionKey=0,model=0;int32_t type=0;
            replaySnapshotMoveUnsigned(&snapshot,&definitionKey,4);
            replaySnapshotMoveUnsigned(&snapshot,&model,4);
            replaySnapshotMoveData(&snapshot,&type,1,-1,0);
            if(snapshot.result!=REPLAY_SNAPSHOT_OK)break;
            const auto* definition=state.data->Find(definitionKey);
            if(!definition)definition=inGameObjects.Find(definitionKey);
            if(id>=COLLISION_ID_COUNT || type<0 || type>3 || !definition) {
                snapshot.result=REPLAY_SNAPSHOT_INVALID_TRANSFER;break;
            }
            auto* object=recreateSmokeable(collision,munitions,type,static_cast<int32_t>(id),*definition,model);
            if(!object) {snapshot.result=REPLAY_SNAPSHOT_INVALID_BINDING;break;}
            if(object->munition)chopperMunitionDoSnapshot(object->munition,&snapshot);
            else collisionBreakableDoSnapshot(object,&snapshot);
        }
    } else if(!snapshot.restoring) {
        auto& root=collision.space->root;
        for(auto* shape=root.next;shape!=&root;shape=shape->next) {
            if(shape->kind!=2)continue;
            std::size_t index=0;
            while(index<registry.placementCount && &registry.placements[index].shape!=shape)++index;
            Require(index<registry.placementCount && index<0x8000u,"VirginSmokeable has no encodable authored index");
            int32_t encoded=static_cast<int32_t>(index);
            replaySnapshotMoveInteger(&snapshot,&encoded,2);
            virginSmokeableDoSnapshot(registry.placements[index],snapshot);
        }
        int32_t terminator=-1;replaySnapshotMoveInteger(&snapshot,&terminator,2);
        for(auto* shape=root.next;shape!=&root;shape=shape->next) {
            if(shape->kind!=1)continue;
            auto* participant=collisionObjectParticipant(&collision,shape);
            Require(participant && participant->breakable,"Smokeable snapshot lost a live body");
            auto* object=participant->breakable;
            uint32_t id=static_cast<uint16_t>(shape->id),key=hashString(object->definition->name),model=object->modelKey;
            int32_t type=object->munition?(object->munition->kind==CHOPPER_MUNITION_ROCKET?1:
                object->munition->kind==CHOPPER_MUNITION_SMOKE_BOMB?2:3):0;
            replaySnapshotMoveUnsigned(&snapshot,&id,2);replaySnapshotMoveUnsigned(&snapshot,&key,4);
            replaySnapshotMoveUnsigned(&snapshot,&model,4);replaySnapshotMoveData(&snapshot,&type,1,-1,0);
            if(object->munition)chopperMunitionDoSnapshot(object->munition,&snapshot);
            else collisionBreakableDoSnapshot(object,&snapshot);
        }
        replaySnapshotMoveInteger(&snapshot,&terminator,2);
    }
    replaySnapshotEndSection(&snapshot);
}
}

// Smokeable.DoSnapshot2208A0..2209FC, final-only activationImpulse included.
void collisionBreakableDoSnapshot(CollisionBreakable* object,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!object || !object->occupied || !object->system || !object->system->objects) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
    }
    replaySnapshotBeginSection(snapshot,object,"Smokeable");
    collisionDynamicShapeSnapshot(&object->participant,snapshot);
    replaySnapshotMoveInteger(snapshot,&object->simulating,1);
    replaySnapshotMoveData(snapshot,&object->snapshotFlag,1,-1,1);
    uint32_t key=hashString(object->definition->name);
    replaySnapshotVerifyData(snapshot,&key,4,-1,0);
    replaySnapshotMoveFloat(snapshot,&object->activationImpulse);
    replaySnapshotMoveFloat(snapshot,&object->breakImpulse);
    replaySnapshotMoveInteger(snapshot,&object->activatedTicks,4);
    replaySnapshotMoveInteger(snapshot,&object->createdTicks,4);
    replaySnapshotMoveInteger(snapshot,&object->recentCarContactTicks,4);
    replaySnapshotMoveFloat(snapshot,&object->elevationOffset);
    auto& registry=*object->system->objects;
    int32_t placement=-1;
    if(!snapshot->restoring && object->placement) {
        std::size_t index=0;
        while(index<registry.placementCount && &registry.placements[index]!=object->placement)++index;
        hp2::Require(index<registry.placementCount && index<0x8000u,"Smokeable has no encodable spawner");
        placement=static_cast<int32_t>(index);
    }
    replaySnapshotMoveInteger(snapshot,&placement,2);
    if(snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
        if(placement<-1 || (placement>=0 && static_cast<size_t>(placement)>=registry.placementCount))
            snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;
        else {
            object->placement=placement<0?nullptr:&registry.placements[placement];
            if(object->placement)object->placement->activeObject=object;
        }
    }
    rigidBodySnapshot(&object->body,snapshot);
    if(snapshot->recording || snapshot->restoring)collisionShapeUpdateBounds(object->system->space,&object->shape);
    replaySnapshotVerifyData(snapshot,object->shape.minimum,8,-1,0);
    replaySnapshotVerifyData(snapshot,object->shape.maximum,8,-1,0);
    replaySnapshotEndSection(snapshot);
}


// C ABI entry point for the native member.
bool collisionShapeAllowsContact(const CollisionShape *shape)
{
    return shape->locks == 0;
}
