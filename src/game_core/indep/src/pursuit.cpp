#include "indep/inc/Pursuit.hpp"
#include "indep/inc/CopDriver.hpp"
#include "bware/inc/bMath.hpp"
#include "Chopper.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Player.hpp"
#include "Language.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/Player.hpp"
#include "indep/src/speech/mgr.hpp"
#include "TrackedController.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "bware/inc/bWare.hpp"

#include <array>
#include <memory>
#include <new>
#include <cmath>
#include <cstdlib>

// PAL331C00 static initialization. Each actual pursuit ctor updates this
// same cell; contexts borrow it rather than retaining independent copies.
PursuitTimingTuning pursuitTimingTuning = {10.0f,10.0f};



namespace {
WorldServicesResult groupSpeechResult(PursuitManager* manager,WorldSpeechResult result)
{
    if(result==WORLD_SPEECH_OK || result==WORLD_SPEECH_SUPPRESSED || result==WORLD_SPEECH_DROPPED)
        return manager->result=WORLD_SERVICES_OK;
    return manager->result=result==WORLD_SPEECH_INVALID_BINDING ? WORLD_SERVICES_INVALID_BINDING : WORLD_SERVICES_OWNER_FAILURE;
}

bool groupOwnerBindings(const PursuitManager* manager,const PursuitCollisionGroup* group)
{
    return manager && group && group->owner && group->owner->manager==manager &&
        group->owner->bindings && manager->parameters && manager->applicationState &&
        manager->collision && manager->collision->speech;
}

bool groupBindings(const PursuitManager* manager,const PursuitCollisionGroup* group)
{
    return groupOwnerBindings(manager,group) && group->target;
}

bool speechSessionSuppressed(const PursuitManager* manager)
{
    const int32_t state=*manager->applicationState;
    return state==4 || state==7 || state==5 || state==9;
}

CarFramePlayerState* groupPlayer(const PursuitCollisionGroup* group)
{
    if(!group->target->bindings.player)return nullptr;
    CarFrame* frame=group->target->frame;
    if(!frame || !frame->environment.player ||
        &frame->environment.player->movement!=group->target->bindings.player)return nullptr;
    return frame->environment.player;
}

float scaledPursuitStateTime(const PursuitGroupState* state,float seconds)
{
    const float laps=state->manager->parameters->pointToPoint ? 1.0f :
        static_cast<float>(state->manager->parameters->totalLaps);
    float length=laps*state->bindings->route->length;
    if(42000.0f<length)length=42000.0f;
    if(length<14000.0f)length=14000.0f;
    const float scaled=length*0.000017857142665889114f;
    const float factor=scaled+0.25f;
    return seconds*factor;
}
}

// PAL22AB30..22AB70. A missing reserved channel falls back to a peek,
// never a pop, from the SAME manager-owned channel deque.
WorldServicesResult pursuitGroupReadSpeaker(PursuitManager* manager,
    const PursuitCollisionGroup* group,int32_t* channel)
{
    if(!manager || !manager->collision || !group || !channel)return WORLD_SERVICES_INVALID_BINDING;
    int32_t value=group->currentChannel;
    if(value==5){
        if(group->reserved)value=group->reserved->radioChannel;
        else{
            const auto& pool=manager->collision->channels;
            if((!pool.full && pool.readIndex==pool.writeIndex) || pool.readIndex<0 || pool.readIndex>=4)
                return manager->result=WORLD_SERVICES_OWNER_FAILURE;
            value=pool.channels[pool.readIndex];
        }
    }
    *channel=value;
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22AA20..22AA50. This is the retained skin's actual colour mask.
WorldServicesResult pursuitGroupReadColour(const PursuitCollisionGroup* group,int32_t* colour)
{
    if(!group || !group->owner || !group->owner->bindings || !group->target || !colour ||
        !group->target->bindings.owner || !group->target->bindings.owner->driverInfo)
        return WORLD_SERVICES_INVALID_BINDING;
    const auto* resources=group->owner->bindings->carResources;
    const auto& appearance=group->target->bindings.owner->driverInfo->appearance;
    if(!resources || !resources->SkinsLoaded())return WORLD_SERVICES_INVALID_BINDING;
    const auto* skin=resources->FindSkin(appearance.modelCode,appearance.skinNumber,true);
    if(!skin)return WORLD_SERVICES_INVALID_BINDING;
    *colour=static_cast<int32_t>(skin->selectionMask);
    return WORLD_SERVICES_OK;
}

// PAL22AA50..22AAF0. Its final language exclusions are deliberately
// retained; the beta table does not define this behavior.
WorldServicesResult pursuitGroupReadExactDescription(const PursuitCollisionGroup* group,int32_t* description)
{
    if(!group || !group->owner || !group->owner->bindings || !group->target || !description ||
        !group->target->bindings.owner || !group->target->bindings.owner->driverInfo)
        return WORLD_SERVICES_INVALID_BINDING;
    const auto* resources=group->owner->bindings->carResources;
    const int32_t model=group->target->bindings.owner->driverInfo->appearance.modelCode;
    if(!resources || !resources->CarsLoaded() || model<0 || static_cast<size_t>(model)>=resources->Cars().size())
        return WORLD_SERVICES_INVALID_BINDING;
    if(resources->Car(static_cast<size_t>(model)).bodyCategory==1){*description=0x10000000;return WORLD_SERVICES_OK;}
    if(model==15 || model==16 || model==22){*description=-1;return WORLD_SERVICES_OK;}
    if(model==5 || model==6 || model==13 || model==14 || model==43 || model==44){
        const auto* language=group->owner->bindings->language;
        if(!language)return WORLD_SERVICES_INVALID_BINDING;
        if(language->currentLanguage==1 || language->currentLanguage==2){*description=-1;return WORLD_SERVICES_OK;}
    }
    *description=worldSpeechConvertCarTypeToDescription(model);
    return WORLD_SERVICES_OK;
}

// PAL22A220..22A25C. Display byte1 is the canonical metric-speed setting.
WorldServicesResult pursuitGroupDisplaySpeed(const PursuitCollisionGroup* group,float* output)
{
    if(!group || !group->target || !group->target->bindings.state || !output || !group->owner ||
        !group->owner->manager || !group->owner->manager->parameters)return WORLD_SERVICES_INVALID_BINDING;
    bool metric=false;
    if(group->target->bindings.player){
        const auto* settings=group->owner->manager->parameters->runtimeSettings;
        if(!settings)return WORLD_SERVICES_INVALID_BINDING;
        metric=settings->display.languageDependentOption!=0;
    }
    *output=group->target->bindings.state->forwardSpeed*(metric ? 3.5999999046325684f : 2.236999988555908f);
    return WORLD_SERVICES_OK;
}

// PAL2253E8..2254E4, including the independent original -50 sentinel.
WorldServicesResult pursuitManagerPossiblyCallSupervisor(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupOwnerBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    if(speechSessionSuppressed(manager))return manager->result=WORLD_SERVICES_OK;
    const float difference=std::fabs(manager->supervisorRemaining-(-50.0f));
    if(!(difference<=0.0010000000474974513f) || !manager->parameters->roadBlocksEnabled)
        return manager->result=WORLD_SERVICES_OK;
    if(!group->target)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    if(!group->target->bindings.player)return manager->result=WORLD_SERVICES_OK;
    if(vectorDefaultRandomBounded(2)!=0)return manager->result=WORLD_SERVICES_OK;
    int32_t channel;
    const auto read=pursuitGroupReadSpeaker(manager,group,&channel);
    if(read!=WORLD_SERVICES_OK)return read;
    const auto result=worldSpeechRaiseSuperRequest(manager->collision->speech,static_cast<uint32_t>(channel));
    manager->supervisorRemaining=7.0f;
    return groupSpeechResult(manager,result);
}

// PAL22B788..22B840. Empty groups or the actual 3/4 random branch use
// the supervisor event; no placeholder voice/queue-busy test is substituted.
WorldServicesResult pursuitGroupBackOffSpeech(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    auto* speech=manager->collision->speech;
    const bool supervisor=std::fabs(manager->supervisorRemaining)<=0.0010000000474974513f;
    if(!group->activeCount || (supervisor && vectorDefaultRandomBounded(4)!=0))
        return groupSpeechResult(manager,worldSpeechRaiseSuperBackOff(speech));
    int32_t channel;
    const auto read=pursuitGroupReadSpeaker(manager,group,&channel);
    if(read!=WORLD_SERVICES_OK)return read;
    return groupSpeechResult(manager,worldSpeechRaiseCopBackOff(speech,static_cast<uint32_t>(channel),supervisor ? 1 : 0));
}

// PAL229FE8..22A18C. All six states are explicit; the owning release
// operation remains a concrete dependency, never a successful no-op.
WorldServicesResult pursuitGroupSwitchState(PursuitManager* manager,PursuitCollisionGroup* group,int32_t phase)
{
    if(!groupOwnerBindings(manager,group) || phase<0 || phase>=6 || !group->owner->bindings->route)
        return WORLD_SERVICES_INVALID_BINDING;
    auto& state=*group->owner;
    static const float durations[6]={30.0f,20.0f,40.0f,300.0f,10.0f,100000.0f};
    group->phase=phase;
    state.stateRemainingSeconds=scaledPursuitStateTime(&state,durations[phase]);
    switch(phase){
    case 0:group->hudLevel=1;group->requestFlags|=2u;state.speechLevel=0;break;
    case 1:state.speechLevel=3;state.calledForBackup=1;group->hudLevel=2;group->requestFlags|=3u;break;
    case 2:state.calledForBackup=1;group->hudLevel=3;group->requestFlags|=7u;state.speechLevel=4;
        return pursuitManagerPossiblyCallSupervisor(manager,group);
    case 3:state.calledForBackup=1;group->hudLevel=4;group->requestFlags|=0x20Fu;state.speechLevel=6;
        return pursuitManagerPossiblyCallSupervisor(manager,group);
    case 4:group->hudLevel=0;break;
    case 5:{
        group->hudLevel=0;
        if(!group->target)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        if(group->target->bindings.player){
            CarFramePlayerState* player=groupPlayer(group);
            if(!player)return manager->result=WORLD_SERVICES_INVALID_BINDING;
            const auto result=pursuitGroupBackOffSpeech(manager,group);
            if(result!=WORLD_SERVICES_OK)return result;
            carFramePlayerStartScore(player,0);
            if(player->result)return manager->result=WORLD_SERVICES_OWNER_FAILURE;
        }
        const Car* car=group->target->bindings.owner;
        if(!car || car->tableIndex>=8)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        manager->participantReset[car->tableIndex].spawnDelay=999999.0f;
        manager->participantReset[car->tableIndex].preventPursuitSeconds=999999.0f;
        return pursuitGroupReleaseAll(manager,group);
    }
    }
    return manager->result=WORLD_SERVICES_OK;
}

namespace {
WorldServicesResult peekChannel(PursuitManager* manager,int32_t* channel)
{
    const auto& pool=manager->collision->channels;
    if((!pool.full && pool.readIndex==pool.writeIndex) || pool.readIndex<0 || pool.readIndex>=4)
        return manager->result=WORLD_SERVICES_OWNER_FAILURE;
    *channel=pool.channels[pool.readIndex];
    return WORLD_SERVICES_OK;
}

WorldServicesResult rotateChannel(PursuitManager* manager)
{
    int32_t channel;
    if(!pursuitChannelPop(&manager->collision->channels,&channel))
        return manager->result=WORLD_SERVICES_OWNER_FAILURE;
    pursuitChannelPush(&manager->collision->channels,channel);
    return WORLD_SERVICES_OK;
}

// PAL22AB70..22ABA0: same retained next channel, with a non-consuming
// lookup of the actual channel deque when the sentinel is five.
}
WorldServicesResult pursuitGroupReadSecondarySpeaker(PursuitManager* manager,
    const PursuitCollisionGroup* group,int32_t* channel)
{
    if(!manager || !group || !channel)return WORLD_SERVICES_INVALID_BINDING;
    *channel=group->nextChannel;
    if(*channel==5)return peekChannel(manager,channel);
    return WORLD_SERVICES_OK;
}
namespace {

WorldServicesResult declareCollision(PursuitManager* manager,PursuitCollisionGroup* group,CarMovement* other)
{
    int32_t channel;
    const auto read=pursuitGroupReadSpeaker(manager,group,&channel);
    if(read!=WORLD_SERVICES_OK)return read;
    if(group->owner->landmark)
        return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitGeneral(manager->collision->speech,static_cast<uint32_t>(channel)));
    if(!other || !other->bindings.owner || !other->bindings.owner->driverInfo)
        return manager->result=WORLD_SERVICES_INVALID_BINDING;
    const bool police=other->bindings.owner->driverInfo->driverClass==4;
    const auto raised=police ? worldSpeechRaiseBeginPursuitCopHitRun(manager->collision->speech,static_cast<uint32_t>(channel),0) :
        worldSpeechRaiseBeginPursuitHitRunTraffic(manager->collision->speech,static_cast<uint32_t>(channel),0);
    const auto result=groupSpeechResult(manager,raised);
    if(result!=WORLD_SERVICES_OK)return result;
    if(police)return pursuitGroupSwitchState(manager,group,2);
    if(group->phase==0)return pursuitGroupSwitchState(manager,group,1);
    return manager->result=WORLD_SERVICES_OK;
}
}

// PAL22AFB8..22B288. Declaration selects actual collision, description,
// ticket-history and speed branches before enqueuing the same speech event.
WorldServicesResult pursuitGroupDeclareSpeech(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    auto* speech=manager->collision->speech;
    if(manager->parameters->specialPursuit){
        const auto result=groupSpeechResult(manager,worldSpeechFlush(speech));
        if(result!=WORLD_SERVICES_OK)return result;
    }
    int32_t description;
    auto result=pursuitGroupReadExactDescription(group,&description);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    const bool involved=group->target->bindings.player || manager->parameters->specialPursuit;
    if(!involved){
        if(*manager->applicationState==8 || *manager->applicationState==9)return manager->result=WORLD_SERVICES_OK;
        int32_t channel,colour;
        result=description==-1 ? pursuitGroupReadSpeaker(manager,group,&channel) : peekChannel(manager,&channel);
        if(result!=WORLD_SERVICES_OK)return result;
        result=pursuitGroupReadColour(group,&colour);if(result!=WORLD_SERVICES_OK)return manager->result=result;
        const auto raised=description==-1 ? worldSpeechRaiseBeginPursuitIdGeneral(speech,static_cast<uint32_t>(channel),1,colour) :
            worldSpeechRaiseBeginPursuitIdSpecific(speech,static_cast<uint32_t>(channel),0,description,colour);
        result=groupSpeechResult(manager,raised);if(result!=WORLD_SERVICES_OK)return result;
        return manager->result=rotateChannel(manager);
    }
    if(!manager->parameters->specialPursuit){
        CarMovement* other=nullptr;
        if(pursuitCarCausedRecentCollision(group->target,&other)!=CAR_DRIVER_OK)
            return manager->result=WORLD_SERVICES_OWNER_FAILURE;
        if(other){
            result=declareCollision(manager,group,other);
            if(result!=WORLD_SERVICES_OK)return result;
            group->owner->speechLevel=1;group->owner->specialDeclarationSent=1;
            return manager->result=WORLD_SERVICES_OK;
        }
        int32_t tickets;
        result=pursuitManagerGetPulloverCount(manager,group->target->bindings.owner,&tickets);
        if(result!=WORLD_SERVICES_OK)return manager->result=result;
        if(tickets>0){
            result=pursuitGroupSwitchState(manager,group,1);if(result!=WORLD_SERVICES_OK)return result;
            const int32_t choice=vectorDefaultRandomBounded(10);
            if(choice<8 && description!=-1){
                int32_t channel,colour;
                result=pursuitGroupReadSpeaker(manager,group,&channel);if(result!=WORLD_SERVICES_OK)return result;
                result=pursuitGroupReadColour(group,&colour);if(result!=WORLD_SERVICES_OK)return manager->result=result;
                return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitAgain(speech,static_cast<uint32_t>(channel),description,colour));
            }
        }
    }else{
        int32_t channel,colour;
        result=pursuitGroupReadSpeaker(manager,group,&channel);if(result!=WORLD_SERVICES_OK)return result;
        result=pursuitGroupReadColour(group,&colour);if(result!=WORLD_SERVICES_OK)return manager->result=result;
        return groupSpeechResult(manager,description==-1 ?
            worldSpeechRaiseBeginPursuitIdGeneral(speech,static_cast<uint32_t>(channel),1,colour) :
            worldSpeechRaiseBeginPursuitIdSpecific(speech,static_cast<uint32_t>(channel),0,description,colour));
    }
    float speed;
    result=pursuitGroupDisplaySpeed(group,&speed);if(result!=WORLD_SERVICES_OK)return manager->result=result;
    if(100.0f<speed && vectorDefaultRandomBounded(10)<7){
        int32_t channel;
        result=pursuitGroupReadSpeaker(manager,group,&channel);if(result!=WORLD_SERVICES_OK)return result;
        result=pursuitGroupDisplaySpeed(group,&speed);if(result!=WORLD_SERVICES_OK)return manager->result=result;
        return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitSpeed(speech,static_cast<uint32_t>(channel),speed));
    }
    const int32_t choice=vectorDefaultRandomBounded(2);
    int32_t channel;
    result=pursuitGroupReadSpeaker(manager,group,&channel);if(result!=WORLD_SERVICES_OK)return result;
    return groupSpeechResult(manager,choice==0 ? worldSpeechRaiseBeginPursuitErratic(speech,static_cast<uint32_t>(channel)) :
        worldSpeechRaiseBeginPursuitGeneral(speech,static_cast<uint32_t>(channel)));
}

// Final PAL22B288..22B3F8: JoinPursuitSpeech passes the group's landmark
// to20AC40 in a3. The beta producer omitted this released argument.
WorldServicesResult pursuitGroupJoinSpeech(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    if(!group->target->bindings.player)return manager->result=WORLD_SERVICES_OK;
    auto* speech=manager->collision->speech;
    if(group->currentChannel==5){
        if(!pursuitChannelPop(&manager->collision->channels,&group->currentChannel))
            return manager->result=WORLD_SERVICES_OWNER_FAILURE;
        if(group->owner->landmark){
            const bool supervisor=std::fabs(manager->supervisorRemaining)<=0.0010000000474974513f;
            return groupSpeechResult(manager,worldSpeechRaiseSuspectFound(speech,
                static_cast<uint32_t>(group->currentChannel),supervisor ? 1 : 0,group->owner->landmark));
        }
        int32_t description;
        auto result=pursuitGroupReadExactDescription(group,&description);if(result!=WORLD_SERVICES_OK)return manager->result=result;
        int32_t channel;
        result=pursuitGroupReadSpeaker(manager,group,&channel);if(result!=WORLD_SERVICES_OK)return result;
        if(description==-1)return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitGeneral(speech,static_cast<uint32_t>(channel)));
        int32_t colour;
        result=pursuitGroupReadColour(group,&colour);if(result!=WORLD_SERVICES_OK)return manager->result=result;
        return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitAgain(speech,static_cast<uint32_t>(channel),description,colour));
    }
    if(group->owner->joinSpeechSent || group->phase<=0)return manager->result=WORLD_SERVICES_OK;
    group->owner->joinSpeechSent=1;
    int32_t channel;
    if(group->nextChannel==5 && manager->parameters->mode==1){
        if(!pursuitChannelPop(&manager->collision->channels,&channel))return manager->result=WORLD_SERVICES_OWNER_FAILURE;
        group->nextChannel=channel;
    }else{
        auto result=peekChannel(manager,&channel);if(result!=WORLD_SERVICES_OK)return result;
        result=rotateChannel(manager);if(result!=WORLD_SERVICES_OK)return result;
    }
    int32_t speaker;
    const auto result=pursuitGroupReadSpeaker(manager,group,&speaker);if(result!=WORLD_SERVICES_OK)return result;
    return groupSpeechResult(manager,worldSpeechRaiseBackupConfirm(speech,static_cast<uint32_t>(channel),static_cast<uint32_t>(speaker)));
}

namespace {
void updateRecordedPath(DriverCommand* command,const float point[4],bool targetPath)
{
    bool append=command->count<2;
    if(!append){
        const auto& previous=command->points[command->count-2];
        const auto& last=command->points[command->count-1];
        const float x=previous.position[0]-last.position[0];
        const float y=previous.position[1]-last.position[1];
        const float xx=x*x,yy=y*y;
        const float distance=std::sqrt(xx+yy);
        append=targetPath ? 20.0f<=distance : 1.0f<distance;
    }
    if(append){
        if(command->count==DRIVER_COMMAND_CAPACITY)driverCommandTrimFront(command,1,1);
        // Real source21C1C8 is the XY adapter to3D Append21C2B8: Z=0,
        // unchanged packed-road sentinels, and the documented native W=0.
        driverCommandAppend(command,point,nullptr,-1,0,nullptr);
    }else{
        // Real source21DA08 similarly supplies XY/Z=0 to21DA50.
        driverCommandSetPoint(command,command->count-1,point,nullptr,-1,0.0f);
    }
}
}

// PAL22A360..22A544. Both paths and prediction history are the same
// embedded owners read by CopDriver; no sampled or duplicated target state.
WorldServicesResult pursuitGroupUpdateDriveTargets(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group) || !manager->clock || !group->target->bindings.state ||
        !group->target->bindings.state->position || group->targetPath.count<0 ||
        group->targetPath.count>DRIVER_COMMAND_CAPACITY || group->recentTargetPath.count<0 ||
        group->recentTargetPath.count>DRIVER_COMMAND_CAPACITY)return WORLD_SERVICES_INVALID_BINDING;
    const auto* vehicle=group->target->bindings.state;
    const float point[4]={vehicle->position[0],vehicle->position[1],0.0f,0.0f};
    updateRecordedPath(&group->targetPath,point,true);
    float unchangedPosition[2];
    pursuitVelocityHistoryPush(unchangedPosition,&group->velocityHistory,
        vehicle->position,vehicle->velocity,manager->clock->worldTimeElapsed);
    updateRecordedPath(&group->recentTargetPath,point,false);
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22A190..22A220, including all six jump-table destinations. The
// terminal result belongs to phase 5; phase 3 deliberately stays phase 3.
WorldServicesResult pursuitGroupAdvanceState(PursuitManager* manager,
    PursuitCollisionGroup* group,int32_t* finished)
{
    if(!groupBindings(manager,group) || !finished)return WORLD_SERVICES_INVALID_BINDING;
    int32_t next=3;
    *finished=0;
    switch(group->phase) {
    case 0:next=1;break;
    case 1:next=2;break;
    case 2:case 3:next=3;break;
    case 4:next=4;break;
    case 5:*finished=1;break;
    }
    if(next!=group->phase && !*finished)return pursuitGroupSwitchState(manager,group,next);
    return manager->result=WORLD_SERVICES_OK;
}

namespace {
ChopperTarget* groupChopperTarget(PursuitCollisionGroup* group)
{
    Chopper* chopper=group->chopper;
    if(!chopper || !group->target->bindings.owner ||
        (chopper->environment.targetCount && !chopper->environment.targets))return nullptr;
    for(size_t i=0;i<chopper->environment.targetCount;++i) {
        ChopperTarget* target=&chopper->environment.targets[i];
        if(target->car==group->target->bindings.owner && target->movement==group->target)return target;
    }
    return nullptr;
}
WorldServicesResult groupFollowChopper(PursuitManager* manager,PursuitCollisionGroup* group)
{
    ChopperTarget* target=groupChopperTarget(group);
    if(!target)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    // PAL ignores the navigation return; a genuine unmet native binding is
    // still reported through the existing concrete Chopper owner.
    chopperShiftTarget(group->chopper,target);
    return manager->result=group->chopper->result;
}
}

// PAL229678..229970. Every profile draw uses the same World RNG and keeps
// the original branch order; 14E520(world,2) is Random, not a car query.
WorldServicesResult pursuitGroupUpdateChopper(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group) || !manager->clock || !manager->world ||
        !group->target->bindings.owner || !group->target->bindings.ownerMotion)
        return WORLD_SERVICES_INVALID_BINDING;
    PursuitGroupState& state=*group->owner;
    if(group->chopper) {
        if(group->chopper->forceFirstPlayerTarget)return manager->result=WORLD_SERVICES_OK;
    } else if(group->requestFlags&0x20u) {
        int32_t available=0;
        auto result=pursuitManagerChopperAvailable(manager,&available);
        if(result!=WORLD_SERVICES_OK)return manager->result=result;
        if(available) {
            int32_t speaker=0;
            result=pursuitGroupReadSpeaker(manager,group,&speaker);
            if(result!=WORLD_SERVICES_OK)return result;
            result=groupSpeechResult(manager,worldSpeechRaiseRotorRequest(manager->collision->speech,
                speaker,std::fabs(manager->supervisorRemaining)<=0.0010000000474974513f));
            if(result!=WORLD_SERVICES_OK)return result;
            group->extendedTimeout=1;state.chopperWaitSeconds=10.0f;
            result=pursuitManagerAcquireChopper(manager,&group->chopper);
            if(result!=WORLD_SERVICES_OK)return manager->result=result;
            if(group->chopper)group->chopper->totalDrops=0;
            if(manager->parameters->specialPursuit)group->requestFlags&=~0x20u;
        }
    }
    if(!group->extendedTimeout)return manager->result=WORLD_SERVICES_OK;
    if(state.chaseDirection==-1.0f && group->extendedTimeout!=1) {
        if(!group->chopper)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        // Complete PAL1EADF0..1EAE08 callee, factored here without a
        // separate name/count claim: release phase 4 back to phase 0.
        if(group->chopper->phase==4)group->chopper->phase=0;
        group->extendedTimeout=2;state.chopperWaitSeconds=10.0f;
    }
    state.chopperWaitSeconds-=manager->clock->worldTimeElapsed;
    if(!(state.chopperWaitSeconds<0.0f))return manager->result=WORLD_SERVICES_OK;
    switch(group->extendedTimeout) {
    case 1:
        if(group->chopper) {
            const auto result=groupSpeechResult(manager,worldSpeechRaiseRotorConfirm(manager->collision->speech));
            if(result!=WORLD_SERVICES_OK)return result;
            group->extendedTimeout=2;state.chopperWaitSeconds=5.0f;
        } else if(manager->parameters->specialPursuit) {
            const auto result=groupSpeechResult(manager,worldSpeechRaiseDispatchHelicopterNA(
                manager->collision->speech,manager->selectedRadioChannel));
            if(result!=WORLD_SERVICES_OK)return result;
            group->extendedTimeout=0;
        }
        break;
    case 2:
        group->extendedTimeout=3;state.chopperWaitSeconds=15.0f;
        return groupFollowChopper(manager,group);
    case 3: {
        if(!group->chopper)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        state.chopperWaitSeconds=state.nextChopperWaitSeconds;
        state.nextChopperWaitSeconds-=2.0f;
        if(state.nextChopperWaitSeconds<6.0f)state.nextChopperWaitSeconds=6.0f;
        const bool disabled=group->target->bindings.ownerMotion->disabledWheelMask!=0;
        int32_t profile=1;
        if((group->requestFlags&0x80u) && !disabled && worldRandomBounded(manager->world,2)==0)profile=3;
        else if(group->chopper->attackProfile && (group->requestFlags&0x100u) &&
            !disabled && worldRandomBounded(manager->world,2)==0)profile=0;
        else if(group->requestFlags&0x40u)profile=worldRandomBounded(manager->world,2)==0 ? 1 : 2;
        if(!group->chopper->target || group->chopper->target->car!=group->target->bindings.owner)
            return groupFollowChopper(manager,group);
        chopperStartAttack(group->chopper,profile);
        return manager->result=group->chopper->result;
    }
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22ABA0..22ACC0. A real recent collision is consumed only once. The
// first pursuing officer has a distinct speech path from another police car.
WorldServicesResult pursuitGroupEarlyCollision(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    if(!group->target->bindings.player || group->owner->specialDeclarationSent)
        return manager->result=WORLD_SERVICES_OK;
    CarMovement* other=nullptr;
    if(pursuitCarCausedRecentCollision(group->target,&other)!=CAR_DRIVER_OK)
        return manager->result=WORLD_SERVICES_INVALID_BINDING;
    if(!other)return manager->result=WORLD_SERVICES_OK;
    group->owner->specialDeclarationSent=1;
    if(!group->owner->speechLevel)return declareCollision(manager,group,other);
    const bool firstCop=group->activeCount>0 && group->active[0] && group->active[0]->movement==other;
    if(firstCop) {
        int32_t speaker=0;
        const auto result=pursuitGroupReadSpeaker(manager,group,&speaker);
        if(result!=WORLD_SERVICES_OK)return result;
        const auto raised=groupSpeechResult(manager,worldSpeechRaisePursuitCopHit(
            manager->collision->speech,speaker,group->owner->landmark));
        if(raised!=WORLD_SERVICES_OK)return raised;
        return pursuitGroupSwitchState(manager,group,2);
    }
    if(!other->bindings.owner || !other->bindings.owner->driverInfo)
        return manager->result=WORLD_SERVICES_INVALID_BINDING;
    if(other->bindings.owner->driverInfo->driverClass==4)return pursuitGroupSwitchState(manager,group,2);
    int32_t speaker=0;
    auto result=pursuitGroupReadSpeaker(manager,group,&speaker);
    if(result!=WORLD_SERVICES_OK)return result;
    result=groupSpeechResult(manager,worldSpeechRaiseBeginPursuitHitRunTraffic(manager->collision->speech,speaker,0));
    if(result!=WORLD_SERVICES_OK)return result;
    if(!group->phase)return pursuitGroupSwitchState(manager,group,1);
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22ACC0..22AD30; a landmark chooses the location-bearing producer.
WorldServicesResult pursuitGroupRaiseBackupSpeech(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    if(!group->target->bindings.player)return manager->result=WORLD_SERVICES_OK;
    int32_t speaker=0;
    const auto result=pursuitGroupReadSpeaker(manager,group,&speaker);
    if(result!=WORLD_SERVICES_OK)return result;
    return groupSpeechResult(manager,group->owner->landmark ?
        worldSpeechRaiseBackupRequestLocation(manager->collision->speech,speaker,group->owner->landmark) :
        worldSpeechRaiseBackupRequest(manager->collision->speech,speaker));
}

// PAL22AD30..22AEF8. This later identification declaration is independent
// of StartPursuitSpeech22AFB8 and retains its original 40-way choice.
WorldServicesResult pursuitGroupDeclarePursuitSpeech(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    if(!group->target->bindings.player && !manager->parameters->specialPursuit)
        return manager->result=WORLD_SERVICES_OK;
    int32_t description=0;
    auto result=pursuitGroupReadExactDescription(group,&description);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    if(description==-1) {
        int32_t speaker=0,colour=0;
        result=pursuitGroupReadSpeaker(manager,group,&speaker);if(result!=WORLD_SERVICES_OK)return result;
        result=pursuitGroupReadColour(group,&colour);if(result!=WORLD_SERVICES_OK)return manager->result=result;
        return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitIdGeneral(manager->collision->speech,speaker,1,colour));
    }
    const uint32_t choice=vectorDefaultRandomBounded(40);
    int32_t speaker=0;
    result=pursuitGroupReadSpeaker(manager,group,&speaker);if(result!=WORLD_SERVICES_OK)return result;
    if(choice==0 || choice<26) {
        int32_t colour=0;
        result=pursuitGroupReadColour(group,&colour);if(result!=WORLD_SERVICES_OK)return manager->result=result;
        if(choice==0)return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitIdRepeated(
            manager->collision->speech,speaker,0,description,colour));
        if(choice<17)return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitIdSpecific(
            manager->collision->speech,speaker,0,description,colour));
        return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitIdGeneral(manager->collision->speech,speaker,1,colour));
    }
    if(choice<36)return groupSpeechResult(manager,worldSpeechRaiseCopPursuitAvoidance(manager->collision->speech,speaker));
    return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitNoId(manager->collision->speech,speaker));
}

// PAL2295D8..229678; comparisons are strict, and the source advances the
// speech level even when the actual producer suppresses or drops its event.
WorldServicesResult pursuitGroupLevelZeroSpeech(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    PursuitGroupState& state=*group->owner;
    if(state.speechLevel==0 && state.stateRemainingSeconds<16.0f) {
        const auto result=pursuitGroupDeclarePursuitSpeech(manager,group);
        if(result!=WORLD_SERVICES_OK)return result;
        state.speechLevel=1;
    } else if(state.speechLevel==1 && state.stateRemainingSeconds<3.0f) {
        const auto result=pursuitGroupRaiseBackupSpeech(manager,group);
        if(result!=WORLD_SERVICES_OK)return result;
        state.speechLevel=2;
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22B908..22B98C; speaker/speed/location/description/colour read order
// is retained before the one concrete speech producer receives its values.
WorldServicesResult pursuitGroupSupervisorInitiates(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    int32_t speaker=0,description=0,colour=0;float speed=0.0f;
    auto result=pursuitGroupReadSpeaker(manager,group,&speaker);if(result!=WORLD_SERVICES_OK)return result;
    result=pursuitGroupDisplaySpeed(group,&speed);if(result!=WORLD_SERVICES_OK)return manager->result=result;
    const uint32_t location=group->owner->landmark;
    result=pursuitGroupReadExactDescription(group,&description);if(result!=WORLD_SERVICES_OK)return manager->result=result;
    result=pursuitGroupReadColour(group,&colour);if(result!=WORLD_SERVICES_OK)return manager->result=result;
    return groupSpeechResult(manager,worldSpeechRaiseSuperInitiate(manager->collision->speech,
        speaker,location,description,colour,speed));
}

// PAL225308..2253E4. The source returns after the first eligible player
// group; it does not announce every group when the countdown crosses zero.
WorldServicesResult pursuitManagerActivateSupervisor(PursuitManager* manager)
{
    if(!manager || !manager->applicationState || !manager->clock || !manager->collision ||
        !manager->collision->speech || (manager->groupCount && !manager->groups))
        return WORLD_SERVICES_INVALID_BINDING;
    if(speechSessionSuppressed(manager))return manager->result=WORLD_SERVICES_OK;
    if(0.0f<manager->supervisorRemaining) {
        manager->supervisorRemaining-=manager->clock->worldTimeElapsed;
        if(manager->supervisorRemaining<=0.0f) {
            manager->supervisorRemaining=0.0f;
            for(size_t i=0;i<manager->groupCount;++i) {
                PursuitCollisionGroup* group=manager->groups[i];
                if(!group || !group->target)return manager->result=WORLD_SERVICES_INVALID_BINDING;
                if(!group->target->bindings.player)continue;
                int32_t description=0;
                const auto result=pursuitGroupReadExactDescription(group,&description);
                if(result!=WORLD_SERVICES_OK)return manager->result=result;
                if(description!=-1 && static_cast<uint32_t>(group->phase-2)<2)
                    return pursuitGroupSupervisorInitiates(manager,group);
            }
        }
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22C2D0..22C390. A disabled target keeps the chase active. The final
// 2000 metres of an eligible four-or-more-lap race call it off. Progress is the SAME
// RaceCarEntry.displayProgress(source+20), not ordinal/rank or a copied lap counter.
WorldServicesResult pursuitGroupShouldBackOff(PursuitManager* manager,
    const PursuitCollisionGroup* group,int32_t* backOff)
{
    if(!groupBindings(manager,group) || !backOff || !group->target->bindings.ownerMotion)
        return WORLD_SERVICES_INVALID_BINDING;
    *backOff=0;
    // PAL22C2F0 branches to the return with s2=0 in its delay slot. A
    // disabled wheel keeps this pursuit alive for the original stop/timer
    // predicate below Update; it must not award an immediate escape.
    if(group->target->bindings.ownerMotion->disabledWheelMask)return WORLD_SERVICES_OK;
    if(manager->parameters->pointToPoint || raceParametersIsFreeRun(manager->parameters) ||
        manager->parameters->totalLaps<4)return WORLD_SERVICES_OK;
    if(!group->target->bindings.owner || !group->target->bindings.owner->raceEntry ||
        !group->owner->bindings->route)return WORLD_SERVICES_INVALID_BINDING;
    const float left=static_cast<float>(manager->parameters->totalLaps)-group->target->bindings.owner->raceEntry->displayProgress;
    const float distance=left*group->owner->bindings->route->length;
    *backOff=distance<=2000.0f;
    return WORLD_SERVICES_OK;
}

// PAL22B9E8..22BAB8. A reserved road block is tested in XY, exactly like
// the original, before the independent chopper-request phase is checked.
WorldServicesResult pursuitGroupAboutToSpeak(const PursuitCollisionGroup* group,int32_t* imminent)
{
    if(!group || !group->owner || !imminent)return WORLD_SERVICES_INVALID_BINDING;
    *imminent=1;
    if(8.0f<group->elapsed || group->phase==0)return WORLD_SERVICES_OK;
    if(group->phase==2 && group->owner->stateRemainingSeconds<13.0f)return WORLD_SERVICES_OK;
    if(!group->target || !group->target->bindings.ownerMotion)return WORLD_SERVICES_INVALID_BINDING;
    if(group->target->bindings.ownerMotion->disabledWheelMask)return WORLD_SERVICES_OK;
    if(group->reserved) {
        if(!group->target->bindings.state)return WORLD_SERVICES_INVALID_BINDING;
        const float dx=group->reserved->center[0]-group->target->bindings.state->position[0];
        const float dy=group->reserved->center[1]-group->target->bindings.state->position[1];
        const float y2=dy*dy;const float x2=dx*dx;
        if(std::sqrt(x2+y2)<150.0f)return WORLD_SERVICES_OK;
    }
    *imminent=group->extendedTimeout==1;
    return WORLD_SERVICES_OK;
}

// PAL229ED0..229F70. Reordering changes only the two borrowed officer
// pointers in this SAME pursuit; officer back-links remain untouched.
WorldServicesResult pursuitGroupSelectPrimary(PursuitCollisionGroup* group)
{
    if(!group || !group->target || !group->target->bindings.state)return WORLD_SERVICES_INVALID_BINDING;
    PursuitCollisionEntry* first=group->active[0];PursuitCollisionEntry* second=group->active[1];
    if(!first || !second)return WORLD_SERVICES_OK;
    if(!first->movement || !second->movement || !first->movement->bindings.state ||
        !second->movement->bindings.state)return WORLD_SERVICES_INVALID_BINDING;
    const float* target=group->target->bindings.state->position;
    const float* a=first->movement->bindings.state->position;
    const float* b=second->movement->bindings.state->position;
    const float ax=target[0]-a[0],ay=target[1]-a[1];
    const float ay2=ay*ay,ax2=ax*ax;
    const float firstDistance=std::sqrt(ax2+ay2);
    const float bx=target[0]-b[0],by=target[1]-b[1];
    const float by2=by*by,bx2=bx*bx;
    const float secondDistance=std::sqrt(bx2+by2);
    if(30.0f<firstDistance-secondDistance){group->active[0]=second;group->active[1]=first;}
    return WORLD_SERVICES_OK;
}

// PAL22CC38..22CDD0. The state-machine request history and the group's
// capability flags are distinct source fields, both retained by one owner.
WorldServicesResult pursuitGroupActivateRoadBlock(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    PursuitRoadBlockMachine& machine=group->owner->roadBlock;
    if(machine.group!=group)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    if(machine.state)return manager->result=WORLD_SERVICES_OK;
    const bool special=manager->parameters->specialPursuit!=0;
    if(!(machine.requests&1u) || special) {
        int32_t speaker=0;
        auto result=pursuitGroupReadSpeaker(manager,group,&speaker);if(result!=WORLD_SERVICES_OK)return result;
        const bool supervisor=std::fabs(manager->supervisorRemaining)<=0.0010000000474974513f;
        WorldSpeechResult speech;
        if(machine.requests&1u)speech=worldSpeechRaiseRequestAnotherRoadblock(manager->collision->speech,speaker,supervisor);
        else if(group->requestFlags&0x10u)speech=worldSpeechRaiseSpikeStripRequest(manager->collision->speech,speaker,supervisor);
        else speech=worldSpeechRaiseRoadblockRequest(manager->collision->speech,speaker,supervisor);
        result=groupSpeechResult(manager,speech);if(result!=WORLD_SERVICES_OK)return result;
        if(!(machine.requests&1u))machine.requests|=1u;
    }
    if(special) {
        group->requestFlags&=~8u;
        if(manager->roadBlocksRemaining)--manager->roadBlocksRemaining;
        else return groupSpeechResult(manager,worldSpeechRaiseDispatchRoadBlockNA(manager->collision->speech,manager->selectedRadioChannel));
    }
    machine.state=1;machine.remainingSeconds=8.0f;
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22CA78..22CB30. Difficulty is the actual sole live manager field;
// the immutable three-value offset table is2BD450 {+.5,0,-.25}.
WorldServicesResult pursuitGroupRoadBlockSpacing(const PursuitCollisionGroup* group,float* spacing)
{
    if(!group || !group->owner || !group->owner->manager || !spacing ||
        !group->owner->manager->collision)return WORLD_SERVICES_INVALID_BINDING;
    const int32_t difficulty=group->owner->manager->collision->severity;
    if(difficulty<0 || difficulty>2)return WORLD_SERVICES_INVALID_BINDING;
    float base=2.75f;
    switch(group->owner->roadBlocksUsed) {
    case 0:base=3.75f;break;
    case 1:base=3.5f;break;
    case 2:base=3.0f;break;
    }
    constexpr float offsets[3]={0.5f,0.0f,-0.25f};
    *spacing=base+offsets[difficulty];return WORLD_SERVICES_OK;
}

// PAL22B5D0..22B788. Landmark lookup precedes the unconditional twenty-way
// RNG draw; source spike-side and ordinary-roadblock producers stay distinct.
WorldServicesResult pursuitGroupRoadBlockSettingSpeech(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group) || !group->reserved || !manager->simulation ||
        !manager->simulation->pursuitServices || !manager->simulation->pursuitServices->landmarks)
        return WORLD_SERVICES_INVALID_BINDING;
    uint32_t location=0;
    if(const auto* landmark=pursuitLandmarkFind(manager->simulation->pursuitServices->landmarks,
        group->reserved->groundPoint))location=landmark->tag;
    const uint32_t choice=vectorDefaultRandomBounded(20);
    const bool supervisor=std::fabs(manager->supervisorRemaining)<=0.0010000000474974513f;
    const int32_t channel=group->reserved->radioChannel;
    WorldSpeechResult result;
    if(group->requestFlags&0x10u) {
        if(choice<3)result=worldSpeechRaiseSpikeStripSet(manager->collision->speech,channel,location,supervisor);
        else if(group->reserved->pattern==2)result=worldSpeechRaiseSpikeStripSetRight(manager->collision->speech,channel,location,supervisor);
        else result=worldSpeechRaiseSpikeStripSetLeft(manager->collision->speech,channel,location,supervisor);
    } else if(choice<3)result=worldSpeechRaiseRoadBlockSetting(manager->collision->speech,channel,supervisor);
    else result=worldSpeechRaiseRoadBlockSet(manager->collision->speech,channel,supervisor,location);
    return groupSpeechResult(manager,result);
}

// PAL229E30..229ED0. The same concrete manager/catalogue determines whether
// a block exists; channels are consumed only for a real successful block.
WorldServicesResult pursuitGroupCreateRoadBlock(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    if(group->owner->allocationMode==1)group->requestFlags|=0x10u;
    PursuitRoadBlock* block=nullptr;
    const auto result=pursuitManagerRequestBlock(manager,group,group->owner->chaseDirection,&block);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    group->reserved=block;
    if(!block)return manager->result=WORLD_SERVICES_OK;
    ++group->owner->roadBlocksUsed;group->owner->roadBlockApproachSeconds=3.5f;
    if(group->target->bindings.player || manager->parameters->specialPursuit) {
        if(!pursuitChannelPop(&manager->collision->channels,&block->radioChannel))
            return manager->result=WORLD_SERVICES_OWNER_FAILURE;
        return pursuitGroupRoadBlockSettingSpeech(manager,group);
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22CDD8..22CE60. Timers continue clamping in every nonzero state.
// Failed site selection remains state1/timer0 and retries on the next pass.
WorldServicesResult pursuitGroupUpdateRoadBlockMachine(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group) || !manager->clock)return WORLD_SERVICES_INVALID_BINDING;
    PursuitRoadBlockMachine& machine=group->owner->roadBlock;
    if(machine.group!=group)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    if(machine.state) {
        machine.remainingSeconds-=manager->clock->worldTimeElapsed;
        if(machine.remainingSeconds<=0.0f)machine.remainingSeconds=0.0f;
        if(machine.state==1 && machine.remainingSeconds==0.0f) {
            const auto result=pursuitGroupCreateRoadBlock(manager,group);
            if(result!=WORLD_SERVICES_OK)return result;
            if(group->reserved)machine.state=2;
        }
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22B3F8..22B5D0. Preserve the source's boolean conversion of the
// speaker argument on the rotor-failure branch, rather than repairing it.
WorldServicesResult pursuitGroupRoadBlockDefeatedSpeech(PursuitManager* manager,PursuitCollisionGroup* group,int32_t through)
{
    if(!groupBindings(manager,group) || !group->reserved)return WORLD_SERVICES_INVALID_BINDING;
    const bool supervisor=std::fabs(manager->supervisorRemaining)<=0.0010000000474974513f;
    int32_t speaker=0;WorldServicesResult status;
    WorldSpeechResult result;
    if(group->requestFlags&0x10u) {
        if(group->extendedTimeout==3 && vectorDefaultRandomBounded(3)==0) {
            status=pursuitGroupReadSpeaker(manager,group,&speaker);if(status!=WORLD_SERVICES_OK)return status;
            result=worldSpeechRaiseRotorSpikeFailed(manager->collision->speech,supervisor,speaker!=0);
        } else {
            status=pursuitGroupReadSpeaker(manager,group,&speaker);if(status!=WORLD_SERVICES_OK)return status;
            result=worldSpeechRaiseSpikeStripFailure(manager->collision->speech,group->reserved->radioChannel,supervisor,speaker);
        }
    } else {
        status=pursuitGroupReadSpeaker(manager,group,&speaker);if(status!=WORLD_SERVICES_OK)return status;
        result=through ? worldSpeechRaiseRoadBlockFailureThrough(manager->collision->speech,group->reserved->radioChannel,supervisor,speaker) :
            worldSpeechRaiseRoadBlockFailureAvoid(manager->collision->speech,group->reserved->radioChannel,supervisor,speaker);
    }
    status=groupSpeechResult(manager,result);if(status!=WORLD_SERVICES_OK)return status;
    if(!manager->parameters->specialPursuit)return groupSpeechResult(manager,worldSpeechRaiseRequestAnotherRoadblock(
        manager->collision->speech,group->reserved->radioChannel,supervisor));
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22C390..22C5A0. Real contact owners drive handbrake/cinematic/audio;
// the final geometric test independently decides whether the block passed.
WorldServicesResult pursuitGroupHasPassedRoadBlock(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* passed)
{
    if(!groupBindings(manager,group) || !passed || !group->reserved || !group->target->bindings.state ||
        !group->target->bindings.ownerMotion)return WORLD_SERVICES_INVALID_BINDING;
    *passed=0;
    if(group->target->bindings.ownerMotion->disabledWheelMask)return manager->result=WORLD_SERVICES_OK;
    if(group->target->bindings.player) {
        CarFramePlayerState* player=groupPlayer(group);
        if(!player || !group->target->frame || !group->target->frame->environment.driver)
            return manager->result=WORLD_SERVICES_INVALID_BINDING;
        for(int32_t i=0;i<2;++i) {
            const PursuitCollisionEntry* entry=group->reserved->entries[i];if(!entry)continue;
            if(!entry->movement)return manager->result=WORLD_SERVICES_INVALID_BINDING;
            int32_t collided=0;
            if(pursuitCarsCollided(group->target,entry->movement,&collided)!=CAR_DRIVER_OK)
                return manager->result=WORLD_SERVICES_INVALID_BINDING;
            if(!collided)continue;
            CarDriver* driver=group->target->frame->environment.driver;
            if(group->target->mode==1) {
                if(driver->kind!=CAR_DRIVER_REAL || !driver->owner.real)return manager->result=WORLD_SERVICES_INVALID_BINDING;
                realDriverSetHandbrakeTime(&driver->owner.real->input,5.0f);
            } else if(group->target->mode==2) {
                if(driver->kind==CAR_DRIVER_AI) {
                    if(!driver->owner.ai)return manager->result=WORLD_SERVICES_INVALID_BINDING;
                    aiDriverSetHandbrakeTime(driver->owner.ai,4.0f);
                } else if(driver->kind==CAR_DRIVER_TRAFFIC) {
                    if(!driver->owner.traffic)return manager->result=WORLD_SERVICES_INVALID_BINDING;
                    aiControlsSetHandbrakeTime(&driver->owner.traffic->input,4.0f);
                }
            }
            if(35.76218032836914f<std::fabs(group->target->bindings.state->forwardSpeed) &&
                carFramePlayerStartState(player,1,0,1,1)) {
                if(!player->registry || !player->registry->services || !player->registry->services->audio ||
                    !player->registry->services->audioManager)return manager->result=WORLD_SERVICES_INVALID_BINDING;
                const auto audio=worldAudioPlayBustRoadBlock(player->registry->services->audio,
                    player->registry->services->audioManager,group->target->bindings.state->position);
                if(audio!=WORLD_AUDIO_OK)return manager->result=audio==WORLD_AUDIO_CAPACITY ? WORLD_SERVICES_CAPACITY : WORLD_SERVICES_OWNER_FAILURE;
            }
            if(player->result)return manager->result=WORLD_SERVICES_OWNER_FAILURE;
            break;
        }
    }
    const PursuitRoadBlock& block=*group->reserved;
    if(!block.roadPose)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    const float* target=group->target->bindings.state->position;
    const float dx=block.groundPoint[0]-target[0],dy=block.groundPoint[1]-target[1];
    const float x2=dx*dx,y2=dy*dy;
    if(std::sqrt(x2+y2)<block.width) {
        const float* origin=block.roadPose->position;const float* direction=block.roadPose->direction;
        const float targetAlong=(target[0]-origin[0])*direction[0]+(target[1]-origin[1])*direction[1];
        const float blockAlong=(block.groundPoint[0]-origin[0])*direction[0]+(block.groundPoint[1]-origin[1])*direction[1];
        *passed=5.0f<group->owner->chaseDirection*(targetAlong-blockAlong);
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22BEA0..22C2D0. Capture policy uses the actual car window/countdown,
// speed/up vector, per-racer reset state and the same selected officer.
WorldServicesResult pursuitGroupCanStopTarget(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* stoppable)
{
    if(!groupBindings(manager,group) || !manager->clock || !stoppable || !group->target->bindings.owner ||
        !group->target->bindings.state || !group->target->bindings.ownerMotion || !manager->collision->timing)
        return WORLD_SERVICES_INVALID_BINDING;
    *stoppable=0;
    if(!manager->parameters->pulloversEnabled)return manager->result=WORLD_SERVICES_OK;
    CarMovement* target=group->target;Car* car=target->bindings.owner;
    if(car->tableIndex>=8)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    if(manager->participantReset[car->tableIndex].resetPending || carMotionTransitionFlag(target->bindings.ownerMotion))
        return manager->result=WORLD_SERVICES_OK;
    if(!car->raceEntry)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    if(car->raceEntry->completionReason)return manager->result=WORLD_SERVICES_OK;
    const VehicleState& vehicle=*target->bindings.state;
    const bool window=carMotionWindowActive(target->bindings.ownerMotion,manager->clock->worldTimer);
    const bool player=target->bindings.player!=nullptr;
    float& confirm=group->owner->stopConfirmationSeconds;
    if(manager->parameters->specialPursuit) {
        PursuitCollisionEntry* selected=pursuitManagerSelectedEntry(manager);
        if(!selected || !selected->movement || !selected->movement->bindings.state)
            return manager->result=WORLD_SERVICES_INVALID_BINDING;
        const VehicleState& cop=*selected->movement->bindings.state;
        const float x=vehicle.position[0]-cop.position[0],y=vehicle.position[1]-cop.position[1];
        const float x2=x*x,y2=y*y;const float distance=std::sqrt(x2+y2);
        bool captured=window && (player ? car->driveCountdown!=0 : distance<30.0f);
        bool restore=true;
        float radius=manager->collision->timing->captureRadius;
        if(manager->parameters->mode==2)radius*=1.100000023841858f;
        if(!captured && distance<radius) {
            if(target->bindings.ownerMotion->disabledWheelMask && vehicle.speed<0.10000000149011612f)captured=true;
            if(!captured && vehicle.orientation[10]<=0.10000000149011612f &&
                vehicle.speed*2.236999988555908f<10.0f)captured=true;
            if(!captured) {
                const int32_t difficulty=manager->collision->severity;
                if(difficulty<0 || difficulty>2)return manager->result=WORLD_SERVICES_INVALID_BINDING;
                constexpr float threshold[3]={8.940545082092285f,6.705409049987793f,6.705409049987793f};
                float speed=threshold[difficulty];if(manager->parameters->mode==2)speed*=1.5f;
                if(vehicle.speed<speed) {
                    confirm-=manager->clock->worldTimeElapsed;restore=false;
                    if(confirm<0.0f)captured=true;
                }
            }
        }
        if(captured)*stoppable=cop.speed<6.705409049987793f;
        else if(restore) {
            confirm+=manager->clock->worldTimeElapsed;if(2.0f<confirm)confirm=2.0f;
        }
        return manager->result=WORLD_SERVICES_OK;
    }
    if(group->owner->disabledRemainingSeconds<=0.0f)*stoppable=1;
    int32_t nearby=0;
    if(group->active[0]) {
        const auto result=pursuitManagerAnyCopNear(manager,target,manager->collision->timing->captureRadius,&nearby);
        if(result!=WORLD_SERVICES_OK)return manager->result=result;
    } else {
        if(!car->driverInfo)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        nearby=car->driverInfo->driverClass==2;
    }
    if(vehicle.speed*2.236999988555908f<10.0f && nearby) {
        confirm-=manager->clock->worldTimeElapsed;
        if(confirm<=0.0f)*stoppable=1;
        else if(confirm<=1.0f) {
            int32_t collided=0;const auto result=pursuitManagerAnyCopContact(manager,target,&collided);
            if(result!=WORLD_SERVICES_OK)return manager->result=result;
            if(collided)*stoppable=1;
        }
    } else confirm=2.0f;
    if(target->bindings.ownerMotion->disabledWheelMask && vehicle.speed<0.10000000149011612f)*stoppable=1;
    else if(window && (!player || car->driveCountdown!=0))*stoppable=1;
    else if(vehicle.orientation[10]<=0.10000000149011612f && vehicle.speed*2.236999988555908f<10.0f)*stoppable=1;
    return manager->result=WORLD_SERVICES_OK;
}

// The geometric admission inside PAL229970..229B58 is relative to the
// reserved block's first officer, not an independently active chase car.
WorldServicesResult pursuitGroupRoadBlockHighlightEligibility(const PursuitCollisionGroup* group,int32_t* eligible)
{
    if(!group || !eligible || !group->target || !group->target->bindings.state)
        return WORLD_SERVICES_INVALID_BINDING;
    *eligible=0;
    const PursuitCollisionEntry* first=group->reserved ? group->reserved->entries[0] : nullptr;
    if(!first || !first->movement || !(20.0f<group->target->bindings.state->speed))return WORLD_SERVICES_OK;
    if(!first->movement->bindings.state)return WORLD_SERVICES_INVALID_BINDING;
    const float* a=group->target->bindings.state->position;
    const float* b=first->movement->bindings.state->position;
    const float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];
    const float x2=x*x,y2=y*y,z2=z*z;
    *eligible=std::sqrt((x2+y2)+z2)<20.0f;
    return WORLD_SERVICES_OK;
}

// PAL229970..229B58, including the source's independent escalation tail.
WorldServicesResult pursuitGroupRoadBlockDefeated(PursuitManager* manager,PursuitCollisionGroup* group,int32_t through)
{
    if(!groupBindings(manager,group) || !group->target->bindings.state || !manager->collision->timing)
        return WORLD_SERVICES_INVALID_BINDING;
    const bool involved=group->target->bindings.player || manager->parameters->specialPursuit;
    bool backedOff=false;
    CarFramePlayerState* player=nullptr;
    if(involved) {
        if(group->target->bindings.player) {
            player=groupPlayer(group);if(!player)return manager->result=WORLD_SERVICES_INVALID_BINDING;
            int32_t backOff=0;const auto status=pursuitGroupShouldBackOff(manager,group,&backOff);
            if(status!=WORLD_SERVICES_OK)return manager->result=status;
            if(backOff) {
                const auto result=pursuitGroupSwitchState(manager,group,5);if(result!=WORLD_SERVICES_OK)return result;
                backedOff=true;
            }
        }
        if(!backedOff) {
            auto status=pursuitGroupRoadBlockDefeatedSpeech(manager,group,through);
            if(status!=WORLD_SERVICES_OK)return status;
            if(player) {
                carFramePlayerStartScore(player,8);
                if(player->result)return manager->result=WORLD_SERVICES_OWNER_FAILURE;
                int32_t eligible=0;
                const auto admission=pursuitGroupRoadBlockHighlightEligibility(group,&eligible);
                if(admission!=WORLD_SERVICES_OK)return manager->result=admission;
                if(eligible) {
                    if(!player->registry || !player->registry->services || !player->registry->services->replay)
                        return manager->result=WORLD_SERVICES_INVALID_BINDING;
                    replayManagerAddHighlight(player->registry->services->replay,10.0f);
                }
            }
            if(!manager->parameters->specialPursuit && group->owner->speechLevel==6) {
                bool request=false;
                if(!pursuitLimitAiPursuitsToOne) {
                    const int32_t laps=manager->parameters->pointToPoint ? 1 : manager->parameters->totalLaps;
                    if(laps==2)request=true;
                    else if(worldRandomBounded(manager->world,3)<2)request=manager->parameters->chopperEnabled!=0;
                } else request=manager->parameters->chopperEnabled!=0;
                if(request){group->elapsed+=manager->collision->timing->radioTimeout;group->requestFlags|=0x20u;}
            }
        }
    }
    if(group->owner->roadBlocksUsed>=5)group->requestFlags|=0x80u;
    else if(group->owner->roadBlocksUsed>=3) {
        if(manager->parameters->authoredPursuitOption26C)group->requestFlags|=0x110u;
    } else group->requestFlags|=0x40u;
    return manager->result=WORLD_SERVICES_OK;
}

// PAL229B58..229DD8. Passed/avoided, disabled and ordinary timeout paths
// all use the SAME reserved block and actual disband/return-to-patrol graph.
WorldServicesResult pursuitGroupUpdateRoadBlocks(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group) || !manager->clock || !manager->collision->timing ||
        !group->target->bindings.ownerMotion || !group->target->bindings.owner)
        return WORLD_SERVICES_INVALID_BINDING;
    const bool disabled=group->target->bindings.ownerMotion->disabledWheelMask!=0;
    bool bypassEmptyTimer=false;
    if(group->reserved) {
        if(disabled) {
            if(!group->target->bindings.owner->driverInfo)return manager->result=WORLD_SERVICES_INVALID_BINDING;
            if(group->target->bindings.owner->driverInfo->driverClass==2 && manager->parameters->specialPursuit) {
                const auto result=groupSpeechResult(manager,worldSpeechRaiseSpikeStripTireBlow(
                    manager->collision->speech,manager->selectedRadioChannel,0));if(result!=WORLD_SERVICES_OK)return result;
            }
            const auto result=pursuitGroupDisbandRoadBlock(manager,group,1);if(result!=WORLD_SERVICES_OK)return result;
        }
        if(group->reserved) {
            int32_t passed=0;
            auto result=pursuitGroupHasPassedRoadBlock(manager,group,&passed);if(result!=WORLD_SERVICES_OK)return result;
            if(passed) {
                group->elapsed=manager->collision->timing->radioTimeout;
                if(!disabled){result=pursuitGroupRoadBlockDefeated(manager,group,1);if(result!=WORLD_SERVICES_OK)return result;}
                result=pursuitGroupDisbandRoadBlock(manager,group,1);if(result!=WORLD_SERVICES_OK)return result;
            }
            if(group->reserved) {
                group->owner->roadBlockApproachSeconds-=manager->clock->worldTimeElapsed;
                if(group->owner->roadBlockApproachSeconds<=0.0f) {
                    group->owner->roadBlockApproachSeconds=0.0f;
                    if(!group->owner->bindings->route)return manager->result=WORLD_SERVICES_INVALID_BINDING;
                    const float length=group->owner->bindings->route->length;
                    const float blockDistance=group->reserved->routeDistance;
                    const float targetDistance=group->target->routeLocation.distance;
                    float distance=group->owner->chaseDirection==1.0f ?
                        (blockDistance+100.0f)-targetDistance : (targetDistance+100.0f)-blockDistance;
                    if(distance<0.0f)distance+=length;
                    if(length*0.5f<distance) {
                        group->elapsed=manager->collision->timing->radioTimeout;
                        if(!disabled){result=pursuitGroupRoadBlockDefeated(manager,group,0);if(result!=WORLD_SERVICES_OK)return result;}
                        result=pursuitGroupDisbandRoadBlock(manager,group,1);if(result!=WORLD_SERVICES_OK)return result;
                        bypassEmptyTimer=true;
                    }
                }
            }
        }
    }
    if(!group->reserved && !bypassEmptyTimer) {
        if(manager->parameters->specialPursuit)group->elapsed=0.0f;
        else {group->elapsed-=manager->clock->worldTimeElapsed;if(group->elapsed<0.0f)group->elapsed=0.0f;}
    }
    if(!group->reserved && (group->requestFlags&8u)) {
        if(manager->parameters->specialPursuit || (manager->parameters->roadBlocksEnabled && !disabled &&
            group->target->bindings.player && group->elapsed<=0.0f))return pursuitGroupActivateRoadBlock(manager,group);
    }
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22B840..22B904. AI ticket speech rotates the actual channel deque
// before choosing the exact-description or generic radio-arrest producer.
WorldServicesResult pursuitGroupAiTicketSpeech(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    const int32_t session=*manager->applicationState;
    if(session==8 || session==9)return manager->result=WORLD_SERVICES_OK;
    int32_t channel=manager->selectedRadioChannel;
    if(!manager->parameters->specialPursuit) {
        if(!pursuitChannelPop(&manager->collision->channels,&channel))
            return manager->result=WORLD_SERVICES_OWNER_FAILURE;
        pursuitChannelPush(&manager->collision->channels,channel);
    }
    int32_t description;
    auto result=pursuitGroupReadExactDescription(group,&description);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    if(vectorDefaultRandomBounded(3)==0 || description==-1)
        return groupSpeechResult(manager,worldSpeechRaiseRadioArrest(manager->collision->speech,channel));
    return groupSpeechResult(manager,worldSpeechRaiseCarStopped(manager->collision->speech,channel,description));
}

// PAL22CB30..22CC00. Retarget the SAME pursuit; its phase, officers and
// path histories retain their source lifetime. The helicopter clears its
// followed car, while remaining reserved by this group.
WorldServicesResult pursuitGroupChangeChasedCar(PursuitManager* manager,
    PursuitCollisionGroup* group,CarMovement* target)
{
    if(!groupBindings(manager,group) || !target || !target->bindings.owner)
        return WORLD_SERVICES_INVALID_BINDING;
    group->target=target;
    if(group->chopper)chopperShiftTarget(group->chopper,nullptr);
    int32_t speaker=manager->selectedRadioChannel;
    auto result=WORLD_SERVICES_OK;
    if(!manager->parameters->specialPursuit) {
        result=pursuitGroupReadSpeaker(manager,group,&speaker);if(result!=WORLD_SERVICES_OK)return result;
    }
    int32_t description,colour;
    result=pursuitGroupReadExactDescription(group,&description);if(result!=WORLD_SERVICES_OK)return manager->result=result;
    result=pursuitGroupReadColour(group,&colour);if(result!=WORLD_SERVICES_OK)return manager->result=result;
    if(description==-1)return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitIdGeneral(
        manager->collision->speech,static_cast<uint32_t>(speaker),1,colour));
    return groupSpeechResult(manager,worldSpeechRaiseBeginPursuitIdSpecific(
        manager->collision->speech,static_cast<uint32_t>(speaker),0,description,colour));
}

// PAL229288..2295C8, the whole pursuit-group cadence. The caller owns
// deletion; all group resources remain live until it consumes `finished`.
WorldServicesResult pursuitGroupUpdate(PursuitManager* manager,PursuitCollisionGroup* group,int32_t* finished)
{
    if(!groupBindings(manager,group) || !finished || !manager->clock || !group->owner->barricades ||
        !group->target->bindings.owner || !group->target->bindings.ownerMotion ||
        !group->target->bindings.state || !manager->simulation ||
        !manager->simulation->pursuitServices || !manager->simulation->pursuitServices->landmarks)
        return WORLD_SERVICES_INVALID_BINDING;
    *finished=0;
    auto* state=group->owner;
    CarMovement* target=group->target;
    Car* car=target->bindings.owner;
    const float elapsed=manager->clock->worldTimeElapsed;
    auto result=pursuitBarricadesAdvance(state->barricades,elapsed);
    if(result!=WORLD_SERVICES_OK)return manager->result=result;
    const PursuitLandmark* landmark=pursuitLandmarkFind(manager->simulation->pursuitServices->landmarks,
        target->bindings.state->position);
    state->landmark=landmark ? landmark->tag : 0;
    if(target->bindings.ownerMotion->disabledWheelMask)state->disabledRemainingSeconds-=elapsed;
    if(manager->parameters->mode!=1) {
        if(!car->raceEntry)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        if(car->raceEntry->completionReason) {*finished=1;return manager->result=WORLD_SERVICES_OK;}
    }
    if(manager->parameters->specialPursuit) {
        if(manager->parameters->mode==2) {
            if(!target->frame || !target->frame->environment.driver)return manager->result=WORLD_SERVICES_INVALID_BINDING;
            const CarRecoveryPose* pose=carDriverCurrentPose(target->frame->environment.driver,1);
            if(!pose)return manager->result=WORLD_SERVICES_INVALID_BINDING;
            const float x=target->bindings.state->velocity[0]*pose->direction[0];
            const float y=target->bindings.state->velocity[1]*pose->direction[1];
            const float direction=0.0f<=x+y ? 1.0f : -1.0f;
            if(0.0f<direction*state->chaseDirection)state->wrongDirectionSeconds=0.0f;
            else {
                state->wrongDirectionSeconds+=elapsed;
                if(8.0f<=state->wrongDirectionSeconds) {
                    state->wrongDirectionSeconds=0.0f;state->chaseDirection=direction;
                }
            }
        }
    } else {
        if(state->maximumTargetSpeed<target->bindings.state->forwardSpeed)
            state->maximumTargetSpeed=target->bindings.state->forwardSpeed;
        if(group->activeCount<0 || group->activeCount>2)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        for(int32_t i=0;i<group->activeCount;++i) {
            const auto* entry=group->active[i];
            if(!entry || !entry->movement)return manager->result=WORLD_SERVICES_INVALID_BINDING;
            if(entry->movement->mode==5) {
                if(!entry->movement->cop)return manager->result=WORLD_SERVICES_INVALID_BINDING;
                entry->movement->cop->speedLimit=state->maximumTargetSpeed;
            }
        }
        result=pursuitGroupUpdateDriveTargets(manager,group);if(result!=WORLD_SERVICES_OK)return result;
        result=pursuitGroupSelectPrimary(group);if(result!=WORLD_SERVICES_OK)return manager->result=result;
        if(manager->parameters->raceKind==3) {
            if(!car->raceEntry)return manager->result=WORLD_SERVICES_INVALID_BINDING;
            *finished=car->raceEntry->completionReason!=0;
        }
        int32_t backOff;
        result=pursuitGroupShouldBackOff(manager,group,&backOff);if(result!=WORLD_SERVICES_OK)return result;
        if(backOff) {
            result=pursuitGroupSwitchState(manager,group,5);if(result!=WORLD_SERVICES_OK)return result;
            *finished=1;
        } else if(group->activeCount || group->phase>=3)state->stateRemainingSeconds-=elapsed;
        if(state->stateRemainingSeconds<=0.0f) {
            int32_t end;
            result=pursuitGroupAdvanceState(manager,group,&end);if(result!=WORLD_SERVICES_OK)return result;
            if(end)*finished=1;
        }
    }
    if(group->phase!=4 && group->phase!=5) {
        if(*finished)return manager->result=WORLD_SERVICES_OK;
        int32_t canStop;
        result=pursuitGroupCanStopTarget(manager,group,&canStop);if(result!=WORLD_SERVICES_OK)return result;
        if(canStop) {
            result=pursuitManagerCreatePullover(manager,group,car,1);if(result!=WORLD_SERVICES_OK)return result;
            *finished=1;
            result=pursuitGroupSwitchState(manager,group,4);if(result!=WORLD_SERVICES_OK)return result;
            result=pursuitGroupReleaseAll(manager,group);if(result!=WORLD_SERVICES_OK)return result;
        }
    }
    if(*finished)return manager->result=WORLD_SERVICES_OK;
    result=pursuitGroupUpdateRoadBlockMachine(manager,group);if(result!=WORLD_SERVICES_OK)return result;
    result=pursuitGroupUpdateRoadBlocks(manager,group);if(result!=WORLD_SERVICES_OK)return result;
    result=pursuitGroupUpdateChopper(manager,group);if(result!=WORLD_SERVICES_OK)return result;
    if(!manager->parameters->specialPursuit) {
        switch(group->phase) {
        case 0:result=pursuitGroupLevelZeroSpeech(manager,group);if(result!=WORLD_SERVICES_OK)return result;
            [[fallthrough]];
        case 1:result=pursuitGroupEarlyCollision(manager,group);if(result!=WORLD_SERVICES_OK)return result;break;
        case 5:*finished=1;break;
        default:break;
        }
    }
    return manager->result=WORLD_SERVICES_OK;
}






// PAL228FC8..229198. The final-only ten-second disabled timer is retained.
// This body is pending complete dependent state/lifetime graph validation.
WorldServicesResult pursuitGroupInitialize(PursuitManager* manager,PursuitGroupState* state,
    const PursuitGroupBindings* bindings,int32_t id,CarMovement* target)
{
    if(!manager || !state || !bindings || !manager->collision || !manager->parameters ||
        !manager->applicationState || manager->collision->severity<0 || manager->collision->severity>2 ||
        id<0 || id>=6 || !bindings->carResources)
        return WORLD_SERVICES_INVALID_BINDING;
    PursuitCollisionGroup& group=state->group;
    group.owner=state;
    state->manager=manager;state->bindings=bindings;
    group.target=target;
    driverCommandInitialize(&group.targetPath);
    driverCommandInitialize(&group.recentTargetPath);
    pursuitVelocityHistoryInitialize(&group.velocityHistory);
    state->roadBlock.group=&group;
    state->roadBlock.state=0;state->roadBlock.requests=0;state->roadBlock.result=0;
    state->id=static_cast<int8_t>(id);
    group.activeCount=0;group.hudLevel=0;state->roadBlocksUsed=0;
    state->stopConfirmationSeconds=2.0f;
    state->roadBlockApproachSeconds=3.5f;
    static const float timeouts[3]={20.0f,15.0f,10.0f};
    pursuitTimingTuning.radioTimeout=timeouts[manager->collision->severity];
    group.elapsed=0.0f;
    state->chopperWaitSeconds=10.0f;state->nextChopperWaitSeconds=12.0f;
    group.nextChannel=5;
    group.active[0]=nullptr;group.active[1]=nullptr;
    state->landmark=0;state->joinSpeechSent=0;state->specialDeclarationSent=0;state->calledForBackup=0;
    group.reserved=nullptr;group.requestFlags=0;group.chopper=nullptr;group.extendedTimeout=0;
    if(target){
        float topSpeed;
        const auto result=pursuitCarTopSpeed(target,bindings->carResources,&topSpeed);
        if(result!=WORLD_SERVICES_OK)return manager->result=result;
        state->maximumTargetSpeed=topSpeed;
        if(manager->parameters->specialPursuit)group.currentChannel=manager->selectedRadioChannel;
        else if(target->bindings.player &&
            !pursuitChannelPop(&manager->collision->channels,&group.currentChannel))
            return manager->result=WORLD_SERVICES_OWNER_FAILURE;
    }
    const auto switched=pursuitGroupSwitchState(manager,&group,*manager->applicationState==8 ? 3 : 0);
    if(switched!=WORLD_SERVICES_OK)return switched;
    state->chaseDirection=1.0f;state->wrongDirectionSeconds=0.0f;
    state->barricades=pursuitBarricadesCreate(bindings->collision,bindings->topology,bindings->collisionResources);
    if(!state->barricades)return manager->result=WORLD_SERVICES_OWNER_FAILURE;
    state->disabledRemainingSeconds=10.0f;
    return manager->result=WORLD_SERVICES_OK;
}




// PAL22C5A0..22C740. The two reserved officers are real entries; optional
// nearby re-acquisition calls the concrete manager creation operation.
WorldServicesResult pursuitGroupDisbandRoadBlock(PursuitManager* manager,
    PursuitCollisionGroup* group,int32_t considerTarget)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    if(!group->reserved)return manager->result=WORLD_SERVICES_OK;
    for(int index=0;index<2;++index){
        PursuitCollisionEntry* entry=group->reserved->entries[index];
        if(!entry)continue;
        pursuitCollisionGroupRemove(group,entry);
        if(pursuitEntryReturnToPatrol(entry)!=WORLD_SERVICES_OK)
            return manager->result=WORLD_SERVICES_OWNER_FAILURE;
        if(!considerTarget)continue;
        if(!entry->movement || !entry->movement->bindings.state || !group->target->bindings.state ||
            !group->target->bindings.owner)return manager->result=WORLD_SERVICES_INVALID_BINDING;
        const float* target=group->target->bindings.state->position;
        const float* officer=entry->movement->bindings.state->position;
        const float x=target[0]-officer[0],y=target[1]-officer[1];
        const float xx=x*x,yy=y*y;
        if(std::sqrt(xx+yy)<30.0f){
            int32_t created=0;
            const auto result=pursuitManagerCreatePursuit(manager,entry,group->target->bindings.owner,&created);
            if(result!=WORLD_SERVICES_OK)return result;
            if(created)pursuitCollisionSetState(entry,6);
        }
    }
    const bool playerInvolved=group->target->bindings.player || manager->parameters->specialPursuit;
    if(playerInvolved){
        const int32_t channel=group->reserved->radioChannel;
        if(group->target->bindings.player && group->nextChannel==5 && manager->parameters->mode==1)
            group->nextChannel=channel;
        else pursuitChannelPush(&manager->collision->channels,channel);
    }
    const auto removed=pursuitManagerRemoveSpikes(manager,group->reserved->siteIndex);
    if(removed!=WORLD_SERVICES_OK)return removed;
    if(!group->owner->barricades)return manager->result=WORLD_SERVICES_INVALID_BINDING;
    const auto blink=pursuitBarricadesBlink(group->owner->barricades,2.0f);
    if(blink!=WORLD_SERVICES_OK)return manager->result=blink;
    group->reserved=nullptr;
    group->owner->roadBlock.state=0;
    return manager->result=WORLD_SERVICES_OK;
}

// PAL22A260..22A310. Preserve the source's changing loop bound and index
// progression while Remove compacts the two active slots.
WorldServicesResult pursuitGroupReleaseAll(PursuitManager* manager,PursuitCollisionGroup* group)
{
    if(!groupBindings(manager,group))return WORLD_SERVICES_INVALID_BINDING;
    for(int32_t index=0;index<group->activeCount;++index){
        PursuitCollisionEntry* entry=group->active[index];
        if(!entry)continue;
        pursuitCollisionGroupRemove(group,entry);
        if(pursuitCollisionEntryEligible(entry) && pursuitEntryReturnToPatrol(entry)!=WORLD_SERVICES_OK)
            return manager->result=WORLD_SERVICES_OWNER_FAILURE;
    }
    const auto result=pursuitGroupDisbandRoadBlock(manager,group,0);
    if(result!=WORLD_SERVICES_OK)return result;
    return pursuitGroupReleaseChopper(manager,group);
}






WorldServicesResult pursuitGroupReleaseChopper(PursuitManager* manager,
    PursuitCollisionGroup* group)
{
    if (!group) {
        if (manager) manager->result = WORLD_SERVICES_INVALID_BINDING;
        return WORLD_SERVICES_INVALID_BINDING;
    }
    Chopper* chopper = group->chopper;
    if (!chopper || chopper->forceFirstPlayerTarget) {
        if (manager) manager->result = WORLD_SERVICES_OK;
        return WORLD_SERVICES_OK;
    }
    if (!manager) return WORLD_SERVICES_INVALID_BINDING;
    manager->chopperReserved = 0;
    /* The original ignores FollowThisCar's result. Even its navigation gate
     * must not prevent the subsequent release of this group's references. */
    chopperShiftTarget(chopper, nullptr);
    group->extendedTimeout = 0;
    group->chopper = nullptr;
    return manager->result = WORLD_SERVICES_OK;
}

float* pursuitVelocityHistorySample(float output[2],const PursuitVelocityHistory* history,float seconds)
{
    float x=history->velocity[0][0];
    float y=history->velocity[0][1];
    float accumulated=0.0f;
    for(int32_t index=0;index<history->count;++index){
        const float elapsed=history->elapsed[index];
        accumulated=accumulated+elapsed;
        if(seconds<=accumulated){
            accumulated=accumulated-history->elapsed[0];
            if(index<history->count-1){
                const float changeX=history->velocity[index][0]-history->velocity[index+1][0];
                const float changeY=history->velocity[index][1]-history->velocity[index+1][1];
                const float rateX=finiteRatio(changeX,elapsed);
                const float rateY=finiteRatio(changeY,elapsed);
                const float offsetX=rateX*accumulated;
                const float offsetY=rateY*accumulated;
                x=history->velocity[index][0]+offsetX;
                y=history->velocity[index][1]+offsetY;
            }
            break;
        }
    }
    output[0]=x;
    output[1]=y;
    return output;
}


int32_t pursuitCollisionGroupRemove(PursuitCollisionGroup* group, PursuitCollisionEntry* entry)
{
    if (!entry) return 0;
    for (int index = 0; index < group->activeCount; ++index) {
        if (group->active[index] != entry) continue;
        group->active[index] = NULL;
        --group->activeCount;
        if (!group->active[0]) {
            group->active[0] = group->active[1];
            group->active[1] = NULL;
        }
        pursuitCollisionSetGroup(entry, NULL);
        return 1;
    }
    if (group->reserved) {
        for (int index = 0; index < 2; ++index) {
            if (group->reserved->entries[index] != entry) continue;
            group->reserved->entries[index] = NULL;
            pursuitCollisionSetGroup(entry, NULL);
            return 1;
        }
    }
    return 0;
}

// PAL22C920..22C990. Includes both chased officers and a reserved road block.
int32_t pursuitGroupContainsCop(const PursuitCollisionGroup* group,const PursuitCollisionEntry* entry)
{
    for(int32_t index=0;index<group->activeCount;++index)
        if(group->active[index]==entry)return 1;
    if(group->reserved)
        for(const auto* officer:group->reserved->entries)if(officer==entry)return 1;
    return 0;
}

// PAL22D180..22D204: all30 history slots are serialized, including the
// retained unused tail. Count uses the signed one-byte integer overload.
void pursuitVelocityHistoryDoSnapshot(PursuitVelocityHistory* history,ReplaySnapshot* snapshot)
{
    hp2::Require(history && snapshot,"Pursuit history snapshot has no owner");
    replaySnapshotMoveInteger(snapshot,&history->count,1);
    hp2::Require(history->count>=0 && history->count<=PURSUIT_VELOCITY_SAMPLES,
        "Pursuit history snapshot count is outside its original capacity");
    for(int index=0;index<PURSUIT_VELOCITY_SAMPLES;++index){
        replaySnapshotMoveData(snapshot,history->velocity[index],8,-1,0);
        replaySnapshotMoveFloat(snapshot,&history->elapsed[index]);
    }
}

// PAL22CE60..22CEE0 has no nested section. The source always rewrites its
// owning pursuit borrow after the four scalar transfers.
void pursuitRoadBlockMachineDoSnapshot(PursuitRoadBlockMachine* machine,ReplaySnapshot* snapshot,
    PursuitCollisionGroup* group)
{
    hp2::Require(machine && snapshot && group,"Road-block snapshot has no pursuit owner");
    replaySnapshotMoveData(snapshot,&machine->state,1,-1,0);
    replaySnapshotMoveFloat(snapshot,&machine->remainingSeconds);
    replaySnapshotMoveUnsigned(snapshot,&machine->requests,4);
    replaySnapshotMoveInteger(snapshot,&machine->result,4);
    machine->group=group;
}

// PAL22A548..22AA1C. Complete group stream, using canonical references
// restored by CopManager before any officer's own back-reference is read.
void pursuitGroupDoSnapshot(PursuitCollisionGroup* group,ReplaySnapshot* snapshot)
{
    hp2::Require(group && group->owner && snapshot,"Pursuit snapshot has no pool owner");
    auto& state=*group->owner;
    PursuitManager* manager=state.manager;
    hp2::Require(manager && manager->world && manager->simulation && state.barricades,
        "Pursuit snapshot has incomplete World/barricade owners");
    replaySnapshotBeginSection(snapshot,group,"tPursuit");
    replaySnapshotVerifyData(snapshot,&state.id,1,1,1);
    Car* target=group->target?group->target->bindings.owner:nullptr;
    worldSnapshotCarPointer(manager->world,snapshot,&target);
    if(snapshot->restoring)group->target=target?target->Movement():nullptr;
    pursuitManagerSnapshotRoadBlock(manager,snapshot,&group->reserved);
    if(group->reserved){
        replaySnapshotMoveData(snapshot,&group->reserved->radioChannel,4,4,0);
        for(auto& officer:group->reserved->entries)pursuitManagerSnapshotCop(manager,snapshot,&officer);
    }
    replaySnapshotMoveData(snapshot,&group->currentChannel,4,4,0);
    replaySnapshotMoveData(snapshot,&group->nextChannel,4,4,0);
    replaySnapshotMoveInteger(snapshot,&state.copLimit,4);
    replaySnapshotMoveInteger(snapshot,&state.allocationMode,4);
    replaySnapshotMoveInteger(snapshot,&state.roadBlocksUsed,4);
    int32_t hasChopper=group->chopper?1:0;
    replaySnapshotMoveInteger(snapshot,&hasChopper,4);
    if(snapshot->restoring){
        group->chopper=hasChopper?manager->simulation->chopper:nullptr;
        hp2::Require(!hasChopper || group->chopper,"Pursuit snapshot refers to an absent World chopper");
    }
    replaySnapshotMoveFloat(snapshot,&state.chopperWaitSeconds);
    replaySnapshotMoveFloat(snapshot,&state.nextChopperWaitSeconds);
    replaySnapshotMoveData(snapshot,&group->extendedTimeout,1,-1,0);
    replaySnapshotMoveInteger(snapshot,&group->activeCount,4);
    hp2::Require(group->activeCount>=0 && group->activeCount<=2,"Pursuit snapshot has too many chase officers");
    for(int index=0;index<group->activeCount;++index)
        pursuitManagerSnapshotCop(manager,snapshot,&group->active[index]);
    replaySnapshotMoveData(snapshot,&group->phase,1,-1,0);
    replaySnapshotMoveData(snapshot,&state.speechLevel,1,-1,0);
    replaySnapshotMoveData(snapshot,&state.landmark,1,-1,0);
    replaySnapshotMoveFloat(snapshot,&state.stateRemainingSeconds);
    replaySnapshotMoveFloat(snapshot,&group->elapsed);
    replaySnapshotMoveFloat(snapshot,&state.roadBlockApproachSeconds);
    replaySnapshotMoveFloat(snapshot,&state.stopConfirmationSeconds);
    replaySnapshotMoveFloat(snapshot,&state.chaseDirection);
    replaySnapshotMoveFloat(snapshot,&state.wrongDirectionSeconds);
    replaySnapshotMoveFloat(snapshot,&state.disabledRemainingSeconds);
    replaySnapshotMoveData(snapshot,&state.joinSpeechSent,1,4,0);
    replaySnapshotMoveData(snapshot,&state.specialDeclarationSent,1,4,0);
    replaySnapshotMoveData(snapshot,&state.calledForBackup,1,4,0);
    replaySnapshotMoveFloat(snapshot,&state.maximumTargetSpeed);
    replaySnapshotMoveUnsigned(snapshot,&group->requestFlags,4);
    replaySnapshotMoveInteger(snapshot,&group->hudLevel,1);
    hp2::Require(group->target && group->target->bindings.recoveryPoints,
        "Pursuit snapshot has no actual target/road registry");
    const auto* registry=group->target->bindings.recoveryPoints;
    driverCommandSnapshot(&group->targetPath,registry,snapshot);
    driverCommandSnapshot(&group->recentTargetPath,registry,snapshot);
    if(group->target->bindings.player)pursuitVelocityHistoryDoSnapshot(&group->velocityHistory,snapshot);
    pursuitRoadBlockMachineDoSnapshot(&state.roadBlock,snapshot,group);
    pursuitBarricadesDoSnapshot(state.barricades,snapshot);
    replaySnapshotEndSection(snapshot);
}

// PAL229DD8..229E30 RoadBlockEncounteredRecently; same live disabled-wheel/time owners.
int32_t pursuitCollisionRadioSuppressed(const PursuitCollisionContext* context, const PursuitCollisionGroup* group)
{
    if (group->target->bindings.ownerMotion->disabledWheelMask) return 1;
    if (!context->timing) abort();
    float timeout = context->timing->radioTimeout;
    if (group->extendedTimeout == 1) timeout = timeout + timeout;
    return timeout - 4.0f < group->elapsed;
}

// Native typed speech payload adapter for the proven RemovePrimarySpeaker graph.
static void queueRadio(PursuitCollisionContext* context, uint32_t type, int32_t previous, int32_t current)
{
    WorldSpeechContext* payload = worldSpeechNextContext(context->speech);
    if (!payload) {
        context->result = PURSUIT_COLLISION_INVALID_BINDING;
        return;
    }
    payload->fromId = (uint32_t)previous;
    payload->toId = (uint32_t)current;
    const WorldSpeechResult result = worldSpeechPushEvent(context->speech, type);
    /* Original push silently suppresses these session modes and drops a
     * full two-event ring. Missing native ownership is a different error. */
    if (result == WORLD_SPEECH_INVALID_BINDING || result == WORLD_SPEECH_AUDIO_FAILURE)
        context->result = PURSUIT_COLLISION_INVALID_BINDING;
}

// PAL22C990..22CA78 RemovePrimarySpeaker, including actual speaker-pool order and RNG.
void pursuitCollisionChangeRadio(PursuitCollisionContext* context, PursuitCollisionGroup* group, int32_t event)
{
    if (!group->target->bindings.player || pursuitCollisionRadioSuppressed(context, group)) return;
    const int32_t previous = group->currentChannel;
    pursuitChannelPush(&context->channels, previous);
    if (group->nextChannel != 5) {
        group->currentChannel = group->nextChannel;
        group->nextChannel = 5;
    } else if (group->activeCount < 2) group->currentChannel = group->nextChannel;
    else if (!pursuitChannelPop(&context->channels, &group->currentChannel)) {
        context->result = PURSUIT_COLLISION_EMPTY_CHANNEL_POOL;
        return;
    }
    if (!event) queueRadio(context, 0x28u, previous, group->currentChannel);
    else if (vectorDefaultRandomBounded(10) < 2) queueRadio(context, 0x26u, previous, group->currentChannel);
    else queueRadio(context, 0x27u, previous, group->currentChannel);
}

// PAL22CEE0..22CF68 tNearFollowTarget constructor; native vtable-free carrier.
void pursuitVelocityHistoryInitialize(PursuitVelocityHistory* history)
{
    history->count=0;
    for(int32_t index=0;index<PURSUIT_VELOCITY_SAMPLES;++index){
        history->velocity[index][0]=0.0f;
        history->velocity[index][1]=0.0f;
        history->elapsed[index]=0.0f;
    }
}

// PAL22CF90..22D080 tNearFollowTarget Update; the exact30-slot history remains with its actual pursuit owner.
float* pursuitVelocityHistoryPush(float output[2],PursuitVelocityHistory* history,
    const float position[2],const float velocity[2],float elapsed)
{
    for(int32_t index=PURSUIT_VELOCITY_SAMPLES-1;index>0;--index){
        history->velocity[index][0]=history->velocity[index-1][0];
        history->velocity[index][1]=history->velocity[index-1][1];
        history->elapsed[index]=history->elapsed[index-1];
    }
    history->velocity[0][0]=velocity[0];
    history->velocity[0][1]=velocity[1];
    history->elapsed[0]=elapsed;
    if(history->count<PURSUIT_VELOCITY_SAMPLES)++history->count;
    const float x=position[0],y=position[1];
    output[0]=x;
    output[1]=y;
    return output;
}
