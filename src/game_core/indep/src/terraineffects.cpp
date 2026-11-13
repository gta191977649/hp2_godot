#include "indep/inc/TerrainEffects.hpp"
#include "indep/inc/TerrainType.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include <array>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

struct TerrainEffectEmitters {
    WorldParticleSystem* particles=nullptr;
    std::array<std::array<std::vector<ChopperEffectInstance>,33>,7> channels;
    size_t count=0;
};

namespace {
bool copyTerrainEmitters(TerrainEffectEmitters& emitters,const ChopperEffectGroup* groups,size_t groupCount)
{
    size_t materialCount=0;
    const TerrainMaterial* materials=terrainDefaultMaterials(&materialCount);
    if (!materials || materialCount!=33) return false;
    for(size_t material=0;material<materialCount;++material)for(size_t effect=0;effect<7;++effect){
        const char* name=materials[material].effects[effect].name;
        if(!name[0])continue;
        const uint32_t key=hashString(name);
        for(size_t group=0;group<groupCount;++group){
            if(groups[group].key!=key)continue;
            if(groups[group].count && !groups[group].members)return false;
            auto& instances=emitters.channels[effect][material];
            instances.resize(groups[group].count);
            for(size_t index=0;index<instances.size();++index){
                const ChopperEffectTemplate* resource=groups[group].members[index];
                if(!resource || resource->groupKey!=key)return false;
                auto& instance=instances[index];
                instance={};
                instance.resource=resource;
                instance.particleDefinition=resource->particleDefinition;
                std::memcpy(instance.frame,resource->frame,sizeof(instance.frame));
                instance.occupied=1;
            }
            emitters.count+=instances.size();
            break;
        }
    }
    return true;
}

void releaseTerrainEmitters(TerrainEffectEmitters& emitters)
{
    for(auto& channel:emitters.channels)for(auto& group:channel)for(auto& instance:group){
        if(instance.particleSystem)worldParticlesRemoveEmitter(instance.particleSystem,&instance);
        for(WorldParticleGroup* live=emitters.particles->firstGroup;live;live=live->next)
            if(live->emitter==&instance)live->emitter=nullptr;
    }
}
}

WorldServicesResult terrainEffectEmittersInitialize(TerrainEffectEmitters** owner,
    WorldParticleSystem* particles,const ChopperEffectGroup* groups,size_t groupCount)
{
    if(!owner)return WORLD_SERVICES_INVALID_BINDING;
    if(*owner)return WORLD_SERVICES_OK;
    if(!particles || (groupCount && !groups))return WORLD_SERVICES_INVALID_BINDING;
    try {
        auto emitters=std::make_unique<TerrainEffectEmitters>();
        emitters->particles=particles;
        if(!copyTerrainEmitters(*emitters,groups,groupCount))return WORLD_SERVICES_INVALID_BINDING;
        *owner=emitters.release();
        return WORLD_SERVICES_OK;
    } catch(const std::bad_alloc&) {return WORLD_SERVICES_CAPACITY;}
}

void terrainEffectEmittersClose(TerrainEffectEmitters** owner)
{
    if(!owner || !*owner)return;
    releaseTerrainEmitters(**owner);
    delete *owner;
    *owner=nullptr;
}

WorldServicesResult terrainEffectEmittersTrigger(TerrainEffectEmitters* emitters,
    int32_t effect,int32_t material,float strength,const float position[4],const float velocity[4])
{
    if(!emitters || effect<0 || effect>=7 || material<0 || material>=33 || !position || !velocity)
        return WORLD_SERVICES_INVALID_BINDING;
    auto& instances=emitters->channels[static_cast<size_t>(effect)][static_cast<size_t>(material)];
    if(instances.empty())return WORLD_SERVICES_OK;
    float frame[16];
    rotationMatrixIdentity(frame);
    for(int axis=0;axis<3;++axis)frame[12+axis]=position[axis];
    for(auto& instance:instances){
        std::memcpy(instance.frame,frame,sizeof(frame));
        for(int axis=0;axis<3;++axis)instance.carVelocity[axis]=velocity[axis];
        instance.carVelocity[3]=0.0f;
        const auto result=worldParticlesTriggerEmitter(emitters->particles,&instance,-strength,nullptr);
        if(result!=WORLD_SERVICES_OK)return result;
    }
    return WORLD_SERVICES_OK;
}

size_t terrainEffectEmittersCount(const TerrainEffectEmitters* emitters)
{
    return emitters ? emitters->count:0;
}

const WorldParticleSystem* terrainEffectEmittersParticles(const TerrainEffectEmitters* emitters)
{
    return emitters ? emitters->particles : nullptr;
}

ChopperEffectInstance* terrainEffectEmittersGroup(TerrainEffectEmitters* emitters,
    int32_t effect,int32_t material,size_t* count)
{
    if(count)*count=0;
    if(!emitters || effect<0 || effect>=7 || material<0 || material>=33)return nullptr;
    auto& group=emitters->channels[static_cast<size_t>(effect)][static_cast<size_t>(material)];
    if(count)*count=group.size();
    return group.empty() ? nullptr:group.data();
}
