#include "indep/inc/Pursuit.hpp"
#include "indep/inc/SmokeableInfo.hpp"
#include "indep/inc/Smokeable.hpp"
#include "indep/inc/CollisionBody.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"

#include <new>
#include "indep/inc/Replay.hpp"
#include "bware/inc/bWare.hpp"

namespace {
bool barricadeBindings(const PursuitBarricadeManager* manager)
{
    return manager && manager->collision && manager->collision->space &&
        manager->collision->objects && manager->topology && manager->resources;
}

WorldServicesResult collisionResult(CollisionSystemResult result)
{
    return result==COLLISION_SYSTEM_OK ? WORLD_SERVICES_OK : WORLD_SERVICES_OWNER_FAILURE;
}
}

// Native allocation/binding only. The original constructor is the method
// below; this adapter does not add another original body to the accounting.
PursuitBarricadeManager* pursuitBarricadesCreate(CollisionSystem* collision,
    TerrainTopology* topology,hp2::CollisionObjectData* resources)
{
    auto* manager=new(std::nothrow) PursuitBarricadeManager;
    if(!manager)return nullptr;
    manager->collision=collision;manager->topology=topology;manager->resources=resources;
    if(!barricadeBindings(manager)){delete manager;return nullptr;}
    pursuitBarricadesInitialize(manager);
    return manager;
}

// PAL24EF88..24EFD4. No physical object is allocated by this constructor.
void pursuitBarricadesInitialize(PursuitBarricadeManager* manager)
{
    for(int index=0;index<10;++index){
        manager->objectIds[index]=-1;
        manager->remainingSeconds[index]=-1.0f;
    }
    manager->result=WORLD_SERVICES_OK;
}

// PAL24EFD8..24F0A8. Every still-owned signed ID is deleted through the
// same collision template/registry. The resource body is not restored.
WorldServicesResult pursuitBarricadesDispose(PursuitBarricadeManager* manager)
{
    if(!barricadeBindings(manager))return WORLD_SERVICES_INVALID_BINDING;
    for(int index=0;index<10;++index){
        const int16_t id=manager->objectIds[index];
        if(id==-1)continue;
        const auto* definition=manager->resources->Find(hashString("CS_RoadBlock_Barrel"));
        const auto result=collisionSmokeableDelete(manager->collision,id,definition);
        if(result!=COLLISION_SYSTEM_OK)return manager->result=collisionResult(result);
    }
    delete manager;
    return WORLD_SERVICES_OK;
}

// PAL24F0A8..24F1DC. The first free slot receives the actual object's ID.
WorldServicesResult pursuitBarricadesCreateAt(PursuitBarricadeManager* manager,
    const float position[4],int16_t* objectId)
{
    if(!barricadeBindings(manager) || !position || !objectId)
        return WORLD_SERVICES_INVALID_BINDING;
    *objectId=-1;
    for(int index=0;index<10;++index){
        if(manager->objectIds[index]!=-1)continue;
        const uint32_t resourceKey=hashString("CS_RoadBlock_Barrel");
        const auto* definition=manager->resources->Find(resourceKey);
        if(!definition)continue;
        RigidBodyInfo* body=manager->resources->MutableRigidBodyInfo(resourceKey);
        if(!body || definition->body!=body)
            return manager->result=WORLD_SERVICES_INVALID_BINDING;
        const float scale=finiteRatio(10.0f,body->mass);
        body->mass=body->mass*scale;
        for(int lane=0;lane<12;++lane)body->inertia[lane]=body->inertia[lane]*scale;
        const int16_t id=collisionSmokeableCreate(manager->collision,manager->topology,
            definition,hashString("ROADBLOCK_BARREL"),position,0);
        manager->objectIds[index]=id;
        *objectId=id;
        return manager->result=collisionResult(manager->collision->result);
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL24F1E0..24F268. Mark the SAME live object to alternate its display
// updates, while the timer remains owned by this barricade manager.
WorldServicesResult pursuitBarricadesBlink(PursuitBarricadeManager* manager,float seconds)
{
    if(!barricadeBindings(manager))return WORLD_SERVICES_INVALID_BINDING;
    for(int index=0;index<10;++index){
        const int16_t id=manager->objectIds[index];
        if(id==-1)continue;
        manager->remainingSeconds[index]=seconds;
        CollisionShape* shape=collisionBodyById(manager->collision->space,id);
        if(!shape)continue;
        CollisionParticipant* participant=collisionObjectParticipant(manager->collision,shape);
        if(!participant || !participant->breakable)
            return manager->result=WORLD_SERVICES_INVALID_BINDING;
        participant->breakable->responseSuppressed=1;
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL24F268..24F354. Exact -1 sentinel, strict negative expiry, and
// unchanged IDs/timers for objects whose countdown has not been enabled.
WorldServicesResult pursuitBarricadesAdvance(PursuitBarricadeManager* manager,float elapsed)
{
    if(!barricadeBindings(manager))return WORLD_SERVICES_INVALID_BINDING;
    for(int index=0;index<10;++index){
        const int16_t id=manager->objectIds[index];
        if(id==-1 || manager->remainingSeconds[index]==-1.0f)continue;
        const float remaining=manager->remainingSeconds[index]-elapsed;
        manager->remainingSeconds[index]=remaining;
        if(!(remaining<0.0f))continue;
        const auto* definition=manager->resources->Find(hashString("CS_RoadBlock_Barrel"));
        const auto result=collisionSmokeableDelete(manager->collision,id,definition);
        if(result!=COLLISION_SYSTEM_OK)return manager->result=collisionResult(result);
        manager->remainingSeconds[index]=-1.0f;
        manager->objectIds[index]=-1;
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL24F358..24F3E4: timer then signed object handle, for all ten slots.
// Collision's own snapshot restores each object's body and lifetime.
void pursuitBarricadesDoSnapshot(PursuitBarricadeManager* manager,ReplaySnapshot* snapshot)
{
    hp2::Require(manager && snapshot,"Barricade snapshot has no owner");
    replaySnapshotBeginSection(snapshot,nullptr,"BarricadeManager");
    for(int index=0;index<10;++index){
        replaySnapshotMoveFloat(snapshot,&manager->remainingSeconds[index]);
        replaySnapshotMoveData(snapshot,&manager->objectIds[index],2,2,1);
    }
    replaySnapshotEndSection(snapshot);
}
