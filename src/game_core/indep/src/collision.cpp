#include "indep/inc/Collision.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Smokeable.hpp"
#include "indep/inc/World.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/TerrainEffects.hpp"
#include <cmath>
#include <cstring>

int32_t collisionImpactIsCarPair(const CollisionImpact* impact)
{
    return impact->pair->first->shape->kind==0 && impact->pair->second->shape->kind==0;
}


void collisionImpactGetCars(const CollisionImpact* impact,Car** first,Car** second)
{
    const CollisionPair* pair=impact->pair;
    *first=NULL;
    *second=NULL;
    if(pair->first->shape->kind==0&&pair->first->movement)
        *first=pair->first->movement->bindings.owner;
    if(pair->second->shape->kind==0&&pair->second->movement)
        *second=pair->second->movement->bindings.owner;
}

int32_t collisionImpactInvolvesPlayer(const CollisionImpact* impact)
{
    const CollisionParticipant* first = impact->pair->first;
    const CollisionParticipant* second = impact->pair->second;
    if (first->shape->kind == 0 && first->movement && first->movement->bindings.player)
        return 1;
    return second->shape->kind == 0 && second->movement && second->movement->bindings.player;
}

int32_t collisionImpactInvolvesSmokeable(const CollisionImpact* impact)
{
    return impact->pair->first->shape->kind == 1 || impact->pair->second->shape->kind == 1;
}


typedef struct CollisionMaterialEffects {
    float priority, impulseStart, impulseFull, slideSpeedStart, slideSpeedFull;
} CollisionMaterialEffects;

static const CollisionMaterialEffects materialEffects[33]={
    {0.10000000149011612f,50.0f,100.0f,20.0f,50.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,50.0f},
    {0.5f,50.0f,100.0f,50.0f,100.0f},
    {0.20000000298023224f,50.0f,100.0f,50.0f,100.0f},
    {1.0f,50.0f,100.0f,50.0f,100.0f},
    {1.0f,50.0f,100.0f,50.0f,100.0f},
    {1.0f,15.0f,100.0f,15.0f,50.0f},
    {1.0f,50.0f,100.0f,50.0f,100.0f},
    {1.0f,50.0f,100.0f,50.0f,100.0f},
    {0.20000000298023224f,50.0f,100.0f,50.0f,100.0f},
    {0.5f,50.0f,100.0f,50.0f,100.0f},
    {0.5f,50.0f,100.0f,50.0f,100.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,100.0f},
    {0.5f,50.0f,100.0f,50.0f,100.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,100.0f},
    {0.5f,4.0f,15.0f,15.0f,50.0f},
    {1.0f,50.0f,100.0f,50.0f,100.0f},
    {0.10000000149011612f,10.0f,15.0f,15.0f,50.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,100.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,100.0f},
    {1.0f,50.0f,100.0f,50.0f,100.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,100.0f},
    {0.5f,50.0f,100.0f,50.0f,100.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,100.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,100.0f},
    {0.5f,50.0f,100.0f,50.0f,100.0f},
    {0.5f,50.0f,100.0f,50.0f,100.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,100.0f},
    {1.0f,50.0f,100.0f,50.0f,100.0f},
    {0.20000000298023224f,50.0f,100.0f,50.0f,100.0f},
    {0.0f,50.0f,100.0f,50.0f,100.0f},
    {0.10000000149011612f,50.0f,100.0f,50.0f,50.0f},
    {0.0f,10.0f,15.0f,2.0f,4.0f}
};

static int32_t participantMaterial(const CollisionParticipant* participant)
{
    if (participant->movement) return 32;
    if (participant->breakable) return participant->breakable->definition->materialIndex;
    return participant->polygon->materialIndex;
}

int32_t collisionPairMaterialIndex(const CollisionPair* pair)
{
    const int32_t first=participantMaterial(pair->first),second=participantMaterial(pair->second);
    return materialEffects[second].priority<=materialEffects[first].priority ? first : second;
}

CollisionSystemResult collisionSystemBindTerrainEffects(CollisionSystem* system,
    TerrainEffectEmitters* const* emitters)
{
    if (!system || !emitters || !*emitters) return COLLISION_SYSTEM_MISSING_OWNER;
    const auto* particles=terrainEffectEmittersParticles(*emitters);
    if (!particles || !particles->randomState) return COLLISION_SYSTEM_MISSING_OWNER;
    if (system->objects && system->objects->particleEffects &&
        system->objects->particleEffects->particles!=particles) return COLLISION_SYSTEM_OWNER_FAILURE;
    system->terrainEffects=emitters;
    return COLLISION_SYSTEM_OK;
}

namespace {
bool triggerContactEffect(CollisionSystem* system,int32_t channel,int32_t material,
    float strength,const float point[4],const float velocity[4])
{
    if (!system->terrainEffects || !*system->terrainEffects) {
        system->result=COLLISION_SYSTEM_MISSING_OWNER;
        return false;
    }
    if (terrainEffectEmittersTrigger(*system->terrainEffects,channel,material,strength,point,velocity)!=WORLD_SERVICES_OK) {
        system->result=COLLISION_SYSTEM_OWNER_FAILURE;
        return false;
    }
    return true;
}

void recordContactEffect(CollisionSystem* system,const CollisionPair* pair,
    CollisionEffectKind kind,int32_t material,float strength,const float point[4],const float velocity[4])
{
    const auto* registry=system->objects;
    if (!registry || registry->effectCount==registry->effectCapacity) return;
    auto* event=collisionObjectNewEffect(system,kind,pair->first);
    if (!event) return;
    event->materialIndex=material;
    event->strength=strength;
    std::memcpy(event->point,point,sizeof(event->point));
    std::memcpy(event->velocity,velocity,sizeof(event->velocity));
}
}

void collisionRecordContactEffects(CollisionSystem* system, const CollisionPair* pair,
                                   const float point[4], const float impulse[4], float magnitude,
                                   float tangentialSpeed)
{
    if (!system->world || !(worldGetDistanceToClosestPlayer2D(system->world,point)<50.0f)) return;
    const int32_t material=collisionPairMaterialIndex(pair);
    const CollisionMaterialEffects* response=&materialEffects[material];
    if (response->slideSpeedStart<tangentialSpeed) {
        const float fraction=finiteRatio(tangentialSpeed-response->slideSpeedStart,
                                          response->slideSpeedFull-response->slideSpeedStart);
        const float strength=minimumFinite(1.0f,maximumFinite(fraction,0.0f));
        const CollisionShape* shape=pair->first->shape;
        const float zero[4]={0,0,0,0};
        const float* velocity=shape->active ? (shape->bodyMode ? shape->body->linearVelocity : shape->vehicle.velocity) : zero;
        if (!velocity) {system->result=COLLISION_SYSTEM_MISSING_OWNER;return;}
        // Both PAL velocity queries read the first body. Their comparison
        // cannot select the other participant's velocity.
        if (!triggerContactEffect(system,6,material,strength,point,velocity)) return;
        recordContactEffect(system,pair,COLLISION_EFFECT_SCRAPE,material,strength,point,velocity);
    }
    if (response->impulseStart<magnitude) {
        const float fraction=finiteRatio(magnitude-response->impulseStart,response->impulseFull-response->impulseStart);
        const float strength=minimumFinite(1.0f,maximumFinite(fraction,0.0f));
        const float zero[4]={0,0,0,0};
        if (!triggerContactEffect(system,5,material,strength,point,zero)) return;
        recordContactEffect(system,pair,COLLISION_EFFECT_IMPACT,material,strength,point,zero);
    }
    // PlayScrape can create a voice and consume pitch RNG even at zero
    // volume. PAL21E8C0..21E984 admits it only above the material threshold.
    if (response->slideSpeedStart<tangentialSpeed) {
        if (!system->contactAudio) {
            system->result=COLLISION_SYSTEM_MISSING_OWNER;
            return;
        }
        if (collisionAudioPlayScrape(system->contactAudio,pair->impact)!=WORLD_AUDIO_OK) {
            system->result=COLLISION_SYSTEM_OWNER_FAILURE;
            return;
        }
    }
    const float scale=finiteRatio(1.0f,magnitude);
    for (int lane=0;lane<4;++lane) pair->impact->direction[lane]=impulse[lane]*scale;
}

void collisionImpactAccumulate(CollisionSystem* system, CollisionPair* pair,
                               const float point[4], const float impulse[4], float tangentialSpeed)
{
    CollisionImpact* impact=pair->impact;
    const float x=impulse[0]*impulse[0],y=impulse[1]*impulse[1],z=impulse[2]*impulse[2];
    const float magnitude=sqrtf((x+y)+z);
    impact->lastTicks=system->nowTicks;
    impact->accumulatedMagnitude=impact->accumulatedMagnitude+magnitude;
    impact->tangentialSpeed=tangentialSpeed;
    memcpy(impact->point,point,4*sizeof(float));
    for (int lane=0;lane<4;++lane) impact->impulse[lane]=impact->impulse[lane]+impulse[lane];
    collisionRecordContactEffects(system,pair,point,impulse,magnitude,tangentialSpeed);
}


void collisionImpactInitialize(CollisionSystem* system, CollisionImpact* impact, CollisionPair* pair,
                                const float point[4])
{
    memset(impact,0,sizeof(*impact));
    impact->pair=pair;impact->occupied=1;
    impact->previous=system->lastImpact;
    if(system->lastImpact)system->lastImpact->next=impact;
    else system->firstImpact=impact;
    system->lastImpact=impact;
    if (point) {
        memcpy(impact->firstPoint,point,sizeof(impact->firstPoint));
        if (system->world && worldGetDistanceToClosestPlayer3D(system->world,point)<100.0f) {
            if (!system->contactAudio) system->result=COLLISION_SYSTEM_MISSING_OWNER;
            else if (collisionAudioCreateSample(system->contactAudio,impact)!=WORLD_AUDIO_OK)
                system->result=COLLISION_SYSTEM_OWNER_FAILURE;
        }
    }
    // The original first attempt observes the zeroed constructor timestamp.
    impact->createdTicks=impact->lastTicks=system->nowTicks;
}

void collisionImpactDispose(CollisionSystem* system, CollisionImpact* impact)
{
    if(impact->previous)impact->previous->next=impact->next;
    else system->firstImpact=impact->next;
    if(impact->next)impact->next->previous=impact->previous;
    else system->lastImpact=impact->previous;
    if (impact->normalSound || impact->scrapeSound) {
        if (!system->contactAudio) system->result=COLLISION_SYSTEM_MISSING_OWNER;
        else if (collisionAudioReleaseSample(system->contactAudio,impact)!=WORLD_AUDIO_OK)
            system->result=COLLISION_SYSTEM_OWNER_FAILURE;
    }
    impact->occupied=0;
    impact->next=impact->previous=NULL;
}

#include "indep/inc/Replay.hpp"

// PAL21E760..21E7EC. Sound handles and native intrusive links stay with the
// live owner; the source stream carries timers, three vectors and magnitude.
void collisionImpactSnapshot(CollisionImpact* impact,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!impact || !impact->occupied) {snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;}
    replaySnapshotBeginSection(snapshot,impact,"Collision");
    replaySnapshotMoveInteger(snapshot,&impact->createdTicks,4);
    replaySnapshotMoveInteger(snapshot,&impact->lastTicks,4);
    replaySnapshotMoveData(snapshot,impact->firstPoint,12,-1,0);
    replaySnapshotMoveData(snapshot,impact->point,12,-1,0);
    replaySnapshotMoveData(snapshot,impact->impulse,12,-1,0);
    replaySnapshotMoveFloat(snapshot,&impact->accumulatedMagnitude);
    replaySnapshotEndSection(snapshot);
}
