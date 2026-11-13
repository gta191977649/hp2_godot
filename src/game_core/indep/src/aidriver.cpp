#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Fred.hpp"
#include <cmath>
#include <cstring>
#include "indep/inc/AICarController.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include <stdlib.h>
#include <string.h>
#include "indep/inc/AICarController.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Race.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/AICarController.hpp"

const CarRecoveryGroup* aiDriverCurrentGroup(AIDriver* driver,int32_t refreshIfMissing)
{
    return driverAICurrentGroup(&driver->base,refreshIfMissing);
}

const CarRecoveryPose* aiDriverCurrentPose(AIDriver* driver,int32_t refreshIfMissing)
{
    return driverAICurrentPose(&driver->base,refreshIfMissing);
}

CarDriverResult aiDriverGetResult(const AIDriver* driver)
{
    if(driver->result!=CAR_DRIVER_OK)return driver->result;
    if(driver->base.result!=CAR_DRIVER_OK)return driver->base.result;
    return driver->input.result;
}

void aiDriverSetHandbrakeTime(AIDriver* driver,float seconds)
{
    aiControlsSetHandbrakeTime(&driver->input,seconds);
}

void aiDriverUpdateControls(AIDriver* driver,float elapsed)
{
    aiControlsUpdate(&driver->input,elapsed);
    driver->result=driver->input.result;
}

int32_t aiDriverHornActive(const AIDriver* driver)
{
    return aiControlsHornActive(&driver->input);
}

CarDriverResult aiDriverSetHorn(AIDriver* driver,int32_t enabled)
{
    if(enabled)aiControlsStartHorn(&driver->input);
    else aiControlsStopHorn(&driver->input);
    return driver->result=driver->input.result;
}

void aiDriverThink(AIDriver* driver,float elapsed)
{
    CarMovement* movement=driver->base.physics.movement;
    carMovementSetGripOverride(movement,1);
    if (driver->progressSampleTicks!=0 && driver->progressSampleTicks!=INT32_MAX &&
        !gameTimerAdvance(&driver->progressSampleTicks,elapsed)) {
        const RaceCarEntry* entry=movement->bindings.owner->raceEntry;
        if (!entry || !movement->bindings.route) {
            driver->result=CAR_DRIVER_MISSING_ROAD;return;
        }
        driver->progressSampleTicks=6000;
        driverHistoryPush(&driver->progressHistory,
            fabsf(entry->progressRate)*movement->bindings.route->length);
    }
    driverAIThink(&driver->base,elapsed);
    driver->result=driver->base.result;
}

int32_t aiDriverAcceptTarget(AIDriver* driver,CarMovement* target)
{
    DriverAIBase* base=&driver->base;
    const int32_t now=base->physics.movement->bindings.environment->clock->worldTimer;
    const int32_t elapsed=(int32_t)((uint32_t)now-(uint32_t)driver->selectedTargetTicks);
    const float seconds=(float)elapsed*0.0002500000118743628f;
    if(1.0f<=seconds)driver->selectedTarget=NULL;
    if(target==driver->selectedTarget)return 0;
    const float sample=worldRandomFloat(base->environment->simulation->world,1.0f);
    int32_t profile=base->physics.movement->bindings.owner->driverInfo->behaviorProfile;
    if(profile==-2)profile=0;
    if((uint32_t)profile>=10u||!base->environment->behavior){
        base->result=CAR_DRIVER_INVALID_BINDING;return 0;
    }
    if(sample<base->environment->behavior->profiles[profile].targetAcceptanceChance)return 1;
    driver->selectedTarget=target;
    driver->selectedTargetTicks=now;
    return 0;
}

CarDriverResult aiDriverInitialize(AIDriver* driver,CarMovement* movement,
    const DriverPhysicsVehicle* physics,DriverAIEnvironment* environment,
    const GameTime* clock,struct CarSoundState* sound)
{
    if (!driver) return CAR_DRIVER_INVALID_BINDING;
    memset(driver,0,sizeof(*driver));
    CarDriverResult result=driverAIInitialize(&driver->base,movement,physics,environment,&driver->input,CAR_DRIVER_AI);
    if (result!=CAR_DRIVER_OK) return driver->result=result;
    driver->base.owner.ai=driver;
    const AIControlsBindings input={movement,&driver->base.physics,sound,clock};
    result=aiControlsInitialize(&driver->input,&input);
    if (result!=CAR_DRIVER_OK) {
        driverAIDispose(&driver->base);return driver->result=result;
    }
    if (!driverHistoryInitialize(&driver->progressHistory,5)) {
        aiControlsDispose(&driver->input);driverAIDispose(&driver->base);
        return driver->result=CAR_DRIVER_ALLOCATION_FAILURE;
    }
    driver->progressSampleTicks=6000;
    carMovementSetGripOverride(movement,1);
    driverAIBindBelt(&driver->base);
    return driver->result=CAR_DRIVER_OK;
}

void aiDriverDispose(AIDriver* driver)
{
    aiControlsDispose(&driver->input);driverHistoryDispose(&driver->progressHistory);driverAIDispose(&driver->base);
}

static DriverAIAction* allocateAction(DriverAIBase* driver)
{
    DriverAIAction* action=(DriverAIAction*)malloc(sizeof(*action));
    if(!action)driver->result=CAR_DRIVER_ALLOCATION_FAILURE;
    return action;
}

DriverAIAction* aiDriverCreateAction(AIDriver* owner,int32_t kind)
{
    DriverAIBase* driver=&owner->base;
    const int32_t now=driver->physics.movement->bindings.environment->clock->worldTimer;
    DriverAIAction* action;
    if(kind!=11){
        action=allocateAction(driver);
        if(!action)return NULL;
        switch(kind){
        case 2:return driverAIActionInitializeRoadFollowing(action,200,0.0f,now);
        case 5:return driverAIActionInitializeCruise(action,0,0.0f,now);
        case 6:{
            driverAIActionInitializeYielding(action,600,0.0f,driver->activePerception,
                driver->currentRoad,driver->stopDelaySeconds,now);
            driver->stopDelaySeconds=10000000.0f;
            return action;
        }
        case 7:return driverAIActionInitializeRecovery(action,500,5.0f,-1.0f,now);
        case 10:return driverAIActionInitializeAirborne(action,500,0.0f,now);
        default:return driverAIActionInitializeYielding(action,600,0.0f,
            driver->activePerception,driver->currentRoad,3.0f,now);
        }
    }

    const int32_t previousKind=driver->action?driver->action->kind:11;
    const int32_t previousPriority=driver->action?driver->action->priority:-1;
    DriverAIPerception* perception=&driver->perception;
    const int32_t recoveryTicks=(int32_t)((uint32_t)now-(uint32_t)driver->lastRecoveryActionTicks);
    // Native PC arbitration lets the existing recovery checks interrupt a
    // RoadBlock action with the original priority, history and cooldown.
    if(previousKind!=7&&(previousPriority<=500||previousKind==8)&&
        6.0f<(float)recoveryTicks*0.0002500000118743628f){
        const int32_t priority=driverAIActionRecoveryPriority(driver,perception);
        if(driver->result!=CAR_DRIVER_OK)return NULL;
        if(priority>=0){
            float requestedSeconds=-1.0f;
            if(0.0f<driverAILateralExcess(driver)&&owner->progressHistory.count==owner->progressHistory.capacity&&
                owner->progressHistory.value<0.20000000298023224f)
                requestedSeconds=1.600000023841858f;
            action=allocateAction(driver);
            if(!action)return NULL;
            driverAIActionInitializeRecovery(action,priority,5.0f,requestedSeconds,now);
            driver->lastRecoveryActionTicks=now;
            return action;
        }
    }
    // Let recovery finish under its own criteria before proposing RoadBlock.
    if(previousKind!=8&&previousKind!=7&&previousPriority<=600){
        PursuitRoadBlock* block=NULL;
        const int32_t priority=driverAIActionRoadBlockPriority(driver,perception,&block);
        if(driver->result!=CAR_DRIVER_OK)return NULL;
        if(priority>=0){
            action=allocateAction(driver);
            return action?driverAIActionInitializeRoadBlock(action,priority,-1.0f,block,now):NULL;
        }
    }
    if(previousKind!=9&&previousPriority<=400){
        CarMovement* target=NULL;
        const int32_t priority=driverAIActionContactPriority(driver,perception,&target);
        if(driver->result!=CAR_DRIVER_OK)return NULL;
        if(priority>=0){
            driverAIReactToTarget(driver,target,driver->activePerception);
            if(driver->result!=CAR_DRIVER_OK)return NULL;
            action=allocateAction(driver);
            return action?driverAIActionInitializeContact(action,priority,-1.0f,target,now):NULL;
        }
    }
    DriverAIGoal goal;
    driverAIGoalInitialize(&goal);
    if(previousKind!=1&&previousPriority<=400){
        CarMovement* target=NULL;
        float collisionTime=0.0f;
        int32_t pathClear=1,followTarget=0;
        const int32_t priority=driverAIActionCollisionAvoidancePriority(driver,perception,
            &target,&goal,&collisionTime,&pathClear,&followTarget);
        if(driver->result!=CAR_DRIVER_OK)return NULL;
        if(priority>=0){
            driverAIReactToTarget(driver,target,driver->activePerception);
            if(driver->result!=CAR_DRIVER_OK)return NULL;
            if(aiDriverAcceptTarget(owner,target)){
                perception->goal=goal;
                action=allocateAction(driver);
                return action?driverAIActionInitializeCollisionAvoidance(action,priority,0.0f,target,
                    (float)(uint32_t)pathClear,followTarget,0,now):NULL;
            }
            if(driver->result!=CAR_DRIVER_OK)return NULL;
        }
    }
    if(previousKind!=10&&previousPriority<=500){
        const int32_t priority=driverAIActionAirbornePriority(perception);
        if(driver->result!=CAR_DRIVER_OK)return NULL;
        if(priority>=0){
            action=allocateAction(driver);
            return action?driverAIActionInitializeAirborne(action,priority,-1.0f,now):NULL;
        }
    }
    if(previousKind!=0&&previousPriority<=300){
        CarMovement* target=NULL;
        const int32_t priority=driverAIActionAvoidancePriority(driver,perception,&target,&goal);
        if(driver->result!=CAR_DRIVER_OK)return NULL;
        if(priority>=0&&aiDriverAcceptTarget(owner,target)){
            perception->goal=goal;
            action=allocateAction(driver);
            // PAL130624 retains an unwritten score-output stack cell. It has no
            // gameplay reader; define the native retained snapshot word as zero.
            return action?driverAIActionInitializeAvoidance(action,priority,5.0f,target,0.0f,now):NULL;
        }
        if(driver->result!=CAR_DRIVER_OK)return NULL;
    }
    action=allocateAction(driver);
    return action?driverAIActionInitializeCruise(action,0,-1.0f,now):NULL;
}

namespace hp2 {
namespace {
float ReadBehaviorFloat(const std::uint8_t* bytes)
{
    const auto bits=ReadU32(bytes);
    float value;
    std::memcpy(&value,&bits,sizeof(value));
    Require(std::isfinite(value),"Non-finite driver behavior value");
    return value;
}
}
DriverBehaviorData::DriverBehaviorData(const std::vector<std::uint8_t>& globalBundle)
{
    bool found=false;
    for (const auto& chunk:ReadChunks(globalBundle)) {
        if (chunk.id!=0x34b00u) continue;
        Require(chunk.size>=992,"Incomplete driver difficulty configuration");
        const auto* payload=globalBundle.data()+chunk.payloadOffset;
        for (std::size_t profile=0;profile<10;++profile) {
            const auto* bytes=globalBundle.data()+chunk.payloadOffset+0x170+profile*28;
            for (int chance=0;chance<3;++chance) {
                const auto bits=ReadU32(bytes+chance*4);float value;
                std::memcpy(&value,&bits,sizeof(value));
                Require(std::isfinite(value),"Non-finite driver connection chance");
                settings_.profiles[profile].connectionCumulativeChances[chance]=value;
            }
            const auto bits=ReadU32(bytes+12);float acceptance;
            std::memcpy(&acceptance,&bits,sizeof(acceptance));
            Require(std::isfinite(acceptance),"Non-finite driver target acceptance chance");
            settings_.profiles[profile].targetAcceptanceChance=acceptance;
            settings_.profiles[profile].roadSpeedScale=ReadBehaviorFloat(bytes+16);
            settings_.profiles[profile].playerPerformanceLimit=ReadBehaviorFloat(bytes+20);
            settings_.profiles[profile].speedSamplePointRange=bytes[24];
        }
        for(std::size_t key=0;key<7;++key){
            settings_.positionResponse[key].coordinate=ReadBehaviorFloat(payload+key*8);
            settings_.positionResponse[key].value=ReadBehaviorFloat(payload+key*8+4);
            settings_.speedResponse[key].coordinate=ReadBehaviorFloat(payload+0x38+key*8);
            settings_.speedResponse[key].value=ReadBehaviorFloat(payload+0x38+key*8+4);
        }
        for(std::size_t index=0;index<15;++index){
            settings_.rankMinimumResponse[index]=ReadBehaviorFloat(payload+0x70+index*4);
            settings_.rankMaximumResponse[index]=ReadBehaviorFloat(payload+0xB0+index*4);
        }
        for(std::size_t rank=0;rank<8;++rank)
            for(std::size_t sample=0;sample<2;++sample){
                settings_.paceAboveOneGains[rank][sample]=ReadBehaviorFloat(payload+0xF0+rank*8+sample*4);
                settings_.paceBelowOneGains[rank][sample]=ReadBehaviorFloat(payload+0x130+rank*8+sample*4);
            }
        settings_.speedOffsetIntervalSeconds=ReadBehaviorFloat(payload+0x3D4);
        settings_.performanceIntervalSeconds=ReadBehaviorFloat(payload+0x3D8);
        settings_.maximumPaceDistance=ReadBehaviorFloat(payload+0x3DC);
        settings_.roadSpeedScaleNegativeTwo=ReadBehaviorFloat(payload+0x148);
        settings_.roadSpeedScaleNegativeOne=ReadBehaviorFloat(payload+0x164);
        for (std::size_t group=0;group<2;++group)
            for (std::size_t difficulty=0;difficulty<3;++difficulty)
                for (std::size_t profile=0;profile<10;++profile)
                    settings_.opponentProfileCumulativeChances[group][difficulty][profile]=
                        ReadBehaviorFloat(payload+0x2c4+group*120+difficulty*40+profile*4);
        for (std::size_t difficulty=0;difficulty<3;++difficulty)
            for (std::size_t vehicleClass=0;vehicleClass<5;++vehicleClass)
                settings_.playerPaceContributionByDifficulty[difficulty][vehicleClass]=
                    ReadBehaviorFloat(payload+0x288+difficulty*20+vehicleClass*4);
        found=true;
    }
    Require(found,"Driver difficulty configuration is absent");
}
}

// PAL130890, both routes belong to the ordinary race group.
void aiDriverGetRouteNumbersToUse(int32_t* first,int32_t* last)
{
    *first=0;
    *last=49;
}

#include "indep/inc/Replay.hpp"
// Full PAL130108..130188; the child retains its separate target/timers/history.
void aiDriverSnapshot(AIDriver* driver,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"AIDriver");
    driverAISnapshot(&driver->base,snapshot);
    Car* target=driver->selectedTarget?driver->selectedTarget->bindings.owner:nullptr;
    worldSnapshotCarPointer(driver->base.environment->simulation->world,snapshot,&target);
    if(snapshot->restoring)driver->selectedTarget=target?target->Movement():nullptr;
    replaySnapshotMoveData(snapshot,&driver->selectedTargetTicks,4,-1,0);
    replaySnapshotMoveData(snapshot,&driver->progressSampleTicks,4,-1,0);
    driverHistorySnapshot(&driver->progressHistory,snapshot);
    replaySnapshotEndSection(snapshot);
}
