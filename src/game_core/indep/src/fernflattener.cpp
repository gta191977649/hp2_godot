#include "indep/inc/Scenery.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

namespace {
struct FernShakerInfo {
    uint32_t animationKey=0;
    std::vector<ChopperEffectInstance> emitters;
};

struct FernShaker {
    FernShaker* next=nullptr;
    FernShaker* previous=nullptr;
    uint32_t key=0;
    int32_t startedFrames=0;
    SceneryInstance* instance=nullptr;
    WorldObject* object=nullptr;
    FernShakerInfo* info=nullptr;
    bool occupied=false;
};
}

struct FernShakerSystem {
    Scenery* scenery=nullptr;
    WorldObjectRegistry* objects=nullptr;
    WorldParticleSystem* particles=nullptr;
    const GameTime* clock=nullptr;
    std::array<FernShakerInfo,2> infos;
    std::array<FernShaker,32> storage;
    FernShaker* first=nullptr;
    FernShaker* last=nullptr;
    size_t count=0;
    uint64_t droppedShakes=0;
};

namespace {
void fernInstanceFrame(const SceneryInstance& instance,float frame[16])
{
    // PAL1D9C18 (replacement model) and1D9F84 (particle emitter) both
    // load0x38800000 =1/16384. The collision-only sceneryInstanceFrame
    // helper uses1/32768 and halves the source fern basis here.
    std::fill_n(frame,16,0.0f);
    for(std::size_t row=0;row<3;++row)for(std::size_t column=0;column<3;++column)
        frame[row*4+column]=static_cast<float>(instance.rotation[row*3+column])*(1.0f/16384.0f);
    std::copy_n(instance.position,3,frame+12);frame[15]=1.0f;
}

bool initializeFernInfo(FernShakerInfo& info,const char* name,
    const ChopperEffectGroup* groups,size_t groupCount)
{
    info.animationKey=hashString(name);
    for(size_t group=0;group<groupCount;++group){
        if(groups[group].key!=info.animationKey)continue;
        if(groups[group].count && !groups[group].members)return false;
        info.emitters.resize(groups[group].count);
        for(size_t index=0;index<info.emitters.size();++index){
            const ChopperEffectTemplate* resource=groups[group].members[index];
            if(!resource)return false;
            auto& emitter=info.emitters[index];
            emitter={};
            emitter.resource=resource;
            emitter.particleDefinition=resource->particleDefinition;
            std::memcpy(emitter.frame,resource->frame,sizeof(emitter.frame));
            emitter.occupied=1;
        }
        return true;
    }
    return true;
}

void disposeFern(FernShakerSystem& system,FernShaker& shaker)
{
    if(shaker.instance)shaker.instance->flags&=static_cast<uint16_t>(~0x10u);
    if(shaker.object)worldObjectDispose(system.objects,shaker.object);
    if(shaker.previous)shaker.previous->next=shaker.next;
    else system.first=shaker.next;
    if(shaker.next)shaker.next->previous=shaker.previous;
    else system.last=shaker.previous;
    shaker={};
    --system.count;
}

void initializeFern(FernShakerSystem& system,FernShaker& shaker,uint32_t key)
{
    shaker={};
    shaker.key=key;
    shaker.startedFrames=system.clock->worldTimeFrames;
    shaker.occupied=true;
    shaker.previous=system.last;
    if(system.last)system.last->next=&shaker;
    else system.first=&shaker;
    system.last=&shaker;
    ++system.count;
    SceneryInstance* instance=sceneryFindInstanceByIndex(system.scenery,
        static_cast<int32_t>(key>>16),static_cast<int32_t>(key&0xffffu));
    if(!instance)return;
    ScenerySection* section=system.scenery->sections[key>>16];
    if(instance->modelIndex<0 || static_cast<size_t>(instance->modelIndex)>=section->modelCount)return;
    EditorMarkerModel& model=section->models[instance->modelIndex].levels[0];
    const EditorMarkerRigidAnimation* animation=editorMarkerModelRigidAnimation(&model);
    if(!animation)return;
    float frame[16];
    fernInstanceFrame(*instance,frame);
    WorldObject* object=worldObjectCreate(system.objects,model.key,frame,0);
    if(!object){++system.droppedShakes;return;}
    shaker.instance=instance;
    shaker.object=object;
    instance->flags|=0x10u;
    for(auto& info:system.infos)if(info.animationKey==animation->key){shaker.info=&info;break;}
}

WorldServicesResult shakeFern(FernShakerSystem& system,FernShaker& shaker,const float velocity[4])
{
    if(!shaker.instance || !shaker.object)return WORLD_SERVICES_OK;
    EditorMarkerModel& model=shaker.object->model;
    const EditorMarkerRigidAnimation* animation=editorMarkerModelRigidAnimation(&model);
    if(!animation)return WORLD_SERVICES_OK;
    const float speed=std::sqrt((velocity[0]*velocity[0]+velocity[1]*velocity[1])+velocity[2]*velocity[2]);
    const float scaledSpeed=speed*2.2300000190734863f;
    if(!(5.0f<scaledSpeed))return WORLD_SERVICES_OK;
    if(editorMarkerModelRigidAnimationTime(&model,system.clock->worldTimeFrames)<0){
        const int32_t frame=30.0f<scaledSpeed ? convertFloatToInt32(static_cast<float>(animation->frameCount)*0.5f):0;
        shaker.startedFrames=system.clock->worldTimeFrames;
        editorMarkerModelSetRigidAnimationFrame(&model,frame,system.clock->worldTimeFrames);
    }
    const float strength=std::min(1.0f,std::max(0.0f,(scaledSpeed-5.0f)*0.03999999910593033f));
    if(!(0.0f<strength) || !shaker.info)return WORLD_SERVICES_OK;
    for(auto& emitter:shaker.info->emitters){
        fernInstanceFrame(*shaker.instance,emitter.frame);
        for(int axis=0;axis<3;++axis)emitter.carVelocity[axis]=velocity[axis];
        emitter.carVelocity[3]=0.0f;
        const auto result=worldParticlesTriggerEmitter(system.particles,&emitter,-strength,nullptr);
        if(result!=WORLD_SERVICES_OK)return result;
    }
    return WORLD_SERVICES_OK;
}
}

FernShakerSystem* fernShakersCreate(Scenery* scenery,WorldObjectRegistry* objects,
    WorldParticleSystem* particles,const ChopperEffectGroup* groups,size_t groupCount,
    const GameTime* clock)
{
    if(!scenery || !objects || !objects->models || !particles || !clock || (groupCount&&!groups))return nullptr;
    try {
        auto system=std::make_unique<FernShakerSystem>();
        system->scenery=scenery;system->objects=objects;system->particles=particles;system->clock=clock;
        if(!initializeFernInfo(system->infos[0],"BUSHSHAKE_A",groups,groupCount) ||
           !initializeFernInfo(system->infos[1],"BUSHSHAKE_B",groups,groupCount))return nullptr;
        return system.release();
    } catch(const std::bad_alloc&) {return nullptr;}
}

void fernShakersFlush(FernShakerSystem* system)
{
    if(system)while(system->first)disposeFern(*system,*system->first);
}

void fernShakersDispose(FernShakerSystem* system)
{
    if(!system)return;
    // Live particles may outlast their shared emitter catalogue.
    for(auto& info:system->infos)for(auto& emitter:info.emitters){
        if(emitter.particleSystem)worldParticlesRemoveEmitter(emitter.particleSystem,&emitter);
        for(WorldParticleGroup* group=system->particles->firstGroup;group;group=group->next)
            if(group->emitter==&emitter)group->emitter=nullptr;
    }
    fernShakersFlush(system);
    delete system;
}

WorldServicesResult fernShakersShake(FernShakerSystem* system,uint32_t key,const float velocity[4])
{
    if(!system || !velocity)return WORLD_SERVICES_INVALID_BINDING;
    for(FernShaker* shaker=system->first;shaker;){
        FernShaker* next=shaker->next;
        const uint32_t age=static_cast<uint32_t>(system->clock->worldTimeFrames)-static_cast<uint32_t>(shaker->startedFrames);
        int32_t signedAge;
        std::memcpy(&signedAge,&age,sizeof(age));
        if(signedAge>=241)disposeFern(*system,*shaker);
        shaker=next;
    }
    for(FernShaker* shaker=system->first;shaker;shaker=shaker->next)
        if(shaker->key==key)return shakeFern(*system,*shaker,velocity);
    for(auto& shaker:system->storage)if(!shaker.occupied){
        initializeFern(*system,shaker,key);
        return shakeFern(*system,shaker,velocity);
    }
    ++system->droppedShakes;
    return WORLD_SERVICES_OK;
}

size_t fernShakersCount(const FernShakerSystem* system)
{
    return system ? system->count:0;
}
