#include "indep/inc/TrafficDriver.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "bware/inc/bMath.hpp"
#include <stdlib.h>
#include "indep/inc/AICarController.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/AICarController.hpp"
#include <string.h>


const CarRecoveryGroup* trafficDriverCurrentGroup(TrafficDriver* driver,int32_t refreshIfMissing)
{
    return driverAICurrentGroup(&driver->base,refreshIfMissing);
}

const CarRecoveryPose* trafficDriverCurrentPose(TrafficDriver* driver,int32_t refreshIfMissing)
{
    return driverAICurrentPose(&driver->base,refreshIfMissing);
}

CarDriverResult trafficDriverGetResult(const TrafficDriver* driver)
{
    if(driver->result!=CAR_DRIVER_OK)return driver->result;
    if(driver->base.result!=CAR_DRIVER_OK)return driver->base.result;
    return driver->input.result;
}

void trafficDriverUpdateControls(TrafficDriver* driver,float elapsed)
{
    aiControlsUpdate(&driver->input,elapsed);
    driver->result=driver->input.result;
}

int32_t trafficDriverHornActive(const TrafficDriver* driver)
{
    return aiControlsHornActive(&driver->input);
}

CarDriverResult trafficDriverSetHorn(TrafficDriver* driver,int32_t enabled)
{
    if(enabled)aiControlsStartHorn(&driver->input);
    else aiControlsStopHorn(&driver->input);
    return driver->result=driver->input.result;
}



void trafficDriverActionTarget(TrafficDriver* driver,float* speed,float* duration,float* tolerance)
{
    DriverAIBase* base=&driver->base;
    DriverAIRoadState* road=base->currentRoad;
    DriverAIAction* action=base->action;
    *speed=0;*duration=0;*tolerance=0;
    float roadSpeed=24.58650016784668f;
    float window=0;
    if(!driverAIRoadEmpty(road) &&
       carRecoveryOwnedPoseIndex(road->group,road->recovery.recoveryPose)>=0){
        const CarRecoveryGroup* group=road->group;
        const CarRecoveryPose* stop=NULL;
        const CarRecoveryPose* pose=road->recovery.recoveryPose;
        int32_t index=carRecoveryOwnedPoseIndex(group,pose);
        float distance=0;
        for(int32_t count=0;distance<75&&count<=18&&index<group->poseCount;++count,++index,++pose){
            const CarRecoveryConnection* connection=(pose->flags&255u)==255u?NULL:
                carRecoveryPointConnection(base->physics.movement->bindings.recoveryPoints,pose);
            if(connection&&(connection->flags&0x20u)&&!(connection->flags&0x08u)){
                stop=pose;break;
            }
            distance=distance+pose->segmentLength;
        }
        if(stop){
            window=2;
            roadSpeed=distance*.3278200030326843f;
            if(distance<10){
                const CarRecoveryConnection* connection=carRecoveryPointConnection(
                    base->physics.movement->bindings.recoveryPoints,stop);
                driverAIRoadAssign(road,carRecoveryConnectionGroup(base->physics.movement->bindings.recoveryPoints,connection),
                    carRecoveryConnectionPose(base->physics.movement->bindings.recoveryPoints,connection),0,0,
                    base->physics.movement->bindings.environment->clock->worldTimer);
                driver->roadBranch=(driverAIRoadEmpty(road)?-1:road->group->id)<60;
            }
        }
    }
    const DriverCommand* command=&action->command;
    if(command->count>=3){
        const int32_t middle=command->count/2;
        float first[2]={command->points[middle].position[0]-command->points[0].position[0],
                        command->points[middle].position[1]-command->points[0].position[1]};
        float second[2]={command->points[command->count-1].position[0]-command->points[middle].position[0],
                         command->points[command->count-1].position[1]-command->points[middle].position[1]};
        vectorNormalize2(first,first);vectorNormalize2(second,second);
        if(first[0]*second[0]+first[1]*second[1]<.6399999856948853f){roadSpeed=10;window=2;}
    }
    roadSpeed=maximumFinite(roadSpeed,10);
    if(action->useRoadSpeed){*speed=roadSpeed;*duration=0;*tolerance=window;}
    else {*speed=action->targetSpeed;*duration=action->targetDuration;*tolerance=action->speedTolerance;}
    *speed=minimumFinite(*speed,roadSpeed);
}

void trafficDriverApplyAction(TrafficDriver* driver)
{
    DriverAIBase* base=&driver->base;
    driverAIActionBuildPlan(base->action,base,&base->perception);
    if(base->result!=CAR_DRIVER_OK)return;
    float speed,duration,tolerance;
    trafficDriverActionTarget(driver,&speed,&duration,&tolerance);
    aiControlsSetCommand(base->controls,&base->action->command);
    aiControlsSetTarget(base->controls,base->action->enabled,speed,duration,tolerance);
    driverCommandAdvance(&base->action->command,NULL,
        base->physics.movement->bindings.environment->clock->worldTimer,0);
}

void trafficDriverResetAfterPlacement(TrafficDriver* driver)
{
    DriverAIBase* base=&driver->base;
    if(base->perception.road.group)driver->roadBranch=(uint32_t)(base->perception.road.group->id-50)<10u;
    free(base->action);
    base->action=NULL;
    driverAIConnectionsClear(base);
    DriverAIAction* action=(DriverAIAction*)calloc(1,sizeof(*action));
    if(!action){base->result=driver->result=CAR_DRIVER_ALLOCATION_FAILURE;return;}
    base->action=driverAIActionInitializeCruise(action,0,-1,
        base->physics.movement->bindings.environment->clock->worldTimer);
    trafficDriverApplyAction(driver);
    driver->result=base->result;
}



static DriverAIAction* replaceAction(DriverAIBase* driver)
{
    free(driver->action);
    driver->action=NULL;
    DriverAIAction* action=(DriverAIAction*)malloc(sizeof(*action));
    if(!action)driver->result=CAR_DRIVER_ALLOCATION_FAILURE;
    return action;
}

static int32_t setMode(DriverAIBase* driver,int32_t mode)
{
    if(carMovementSetMode(driver->physics.movement,mode)==CAR_MOVEMENT_OK)return 1;
    driver->result=CAR_DRIVER_MOVEMENT_FAILURE;return 0;
}

static int32_t selectedThisFrame(const DriverAIBase* driver)
{
    const WorldSimulation* simulation=driver->environment->simulation;
    const Car* car=driver->physics.movement->bindings.owner;
    if(car->driverInfo->driverClass!=3)return driver->thinkDelay<=0.0f;
    const int32_t phase=simulation->clock->worldLoopCounter%12;
    if(!(phase&1))return 0;
    const int32_t index=phase/2;
    const World* world=simulation->world;
    return index>=0&&index<world->totalCars-world->thirdGroupEnd&&
        world->carTable[world->thirdGroupEnd+index]==car;
}

void trafficDriverThink(TrafficDriver* owner,float elapsed)
{
    DriverAIBase* driver=&owner->base;
    CarMovement* movement=driver->physics.movement;
    DriverAIPerception* perception=&driver->perception;
    driver->thinkDelay=driver->thinkDelay-elapsed;
    if(!selectedThisFrame(driver))return;
    driverAIConnectionsUpdate(driver);
    if(driver->result!=CAR_DRIVER_OK)return;
    driver->thinkDelay=driver->thinkDelay+0.20000000298023224f;
    driver->result=driverPerceptionUpdate(perception,&driver->physics,driver->environment,
        driver->currentRoad!=NULL);
    if(driver->result!=CAR_DRIVER_OK)return;
    driverAIUpdateRoad(driver);
    if(driver->result!=CAR_DRIVER_OK||driverAIRoadEmpty(driver->currentRoad))return;
    const int32_t now=movement->bindings.environment->clock->worldTimer;
    if(driver->action){
        driverAIActionUpdateStatus(driver->action,driver,perception);
        if(driver->result!=CAR_DRIVER_OK)return;
        int32_t priority=driverAIActionFinished(driver->action)?-1:driver->action->priority;
        const int32_t recoveryTicks=(int32_t)((uint32_t)now-(uint32_t)driver->lastRecoveryActionTicks);
        if(priority<500&&2.0f<(float)recoveryTicks*0.0002500000118743628f){
            const int32_t next=driverAIActionRecoveryPriority(driver,perception);
            if(driver->result!=CAR_DRIVER_OK)return;
            if(priority<next){
                DriverAIAction* action=replaceAction(driver);
                if(!action)return;
                driver->action=driverAIActionInitializeRecovery(action,next,5.0f,-1.0f,now);
                driver->lastRecoveryActionTicks=now;priority=driver->action->priority;
            }
        }
        if(priority<500){
            const int32_t next=driverAIActionAirbornePriority(perception);
            if(driver->result!=CAR_DRIVER_OK)return;
            if(priority<next){
                DriverAIAction* action=replaceAction(driver);
                if(!action)return;
                driver->action=driverAIActionInitializeAirborne(action,next,-1.0f,now);
                priority=driver->action->priority;
            }
        }
        if(priority<400){
            CarMovement* target=NULL;
            const int32_t next=driverAIActionContactPriority(driver,perception,&target);
            if(driver->result!=CAR_DRIVER_OK)return;
            if(priority<next){
                free(driver->action);driver->action=NULL;
                driverAIReactToTarget(driver,target,perception);
                if(driver->result!=CAR_DRIVER_OK)return;
                DriverAIAction* action=(DriverAIAction*)malloc(sizeof(*action));
                if(!action){driver->result=CAR_DRIVER_ALLOCATION_FAILURE;return;}
                driver->action=driverAIActionInitializeContact(action,next,0.0f,target,now);
                priority=driver->action->priority;
            }
        }
        if(priority<400){
            CarMovement* target=NULL;
            DriverAIGoal goal;
            driverAIGoalInitialize(&goal);
            float collisionTime=0.0f;
            int32_t pathClear=1,followTarget=0;
            const int32_t next=driverAIActionCollisionAvoidancePriority(driver,perception,
                &target,&goal,&collisionTime,&pathClear,&followTarget);
            if(driver->result!=CAR_DRIVER_OK)return;
            if(priority<next){
                DriverAIAction* action=replaceAction(driver);
                if(!action)return;
                driver->action=driverAIActionInitializeCollisionAvoidance(action,next,0.0f,target,
                    (float)(uint32_t)pathClear,followTarget,0,now);
                perception->goal=goal;
                driverAIReactToTarget(driver,target,perception);
                if(driver->result!=CAR_DRIVER_OK)return;
                if(movement->mode==2&&!setMode(driver,4))return;
                priority=driver->action->priority;
            }
        }
        if(priority<600){
            const int32_t next=driverAIActionYieldingPriority(driver);
            if(driver->result!=CAR_DRIVER_OK)return;
            if(priority<next){
                DriverAIAction* action=replaceAction(driver);
                if(!action)return;
                driver->action=driverAIActionInitializeYielding(action,next,-1.0f,driver->activePerception,
                    driver->currentRoad,-999.0f,now);
                if(movement->mode==2&&!setMode(driver,4))return;
                priority=driver->action->priority;
            }
        }
        if(priority<0){
            DriverAIAction* action=replaceAction(driver);
            if(!action)return;
            driver->action=driverAIActionInitializeCruise(action,0,-1.0f,now);
        }
    }
    if(!driver->action){
        DriverAIAction* action=(DriverAIAction*)malloc(sizeof(*action));
        if(!action){driver->result=CAR_DRIVER_ALLOCATION_FAILURE;return;}
        driver->action=driverAIActionInitializeCruise(action,0,-1.0f,now);
    }
    if(driverAIGoalUsesRoad(driver->currentGoal->kind)){
        const int32_t lane=driverAILaneForClass(driver->currentGoal->kind,driver->physics.routeVehicleClass);
        if(driver->currentRoad->lane!=lane)driverAIRoadSetLane(driver->currentRoad,lane,0,now);
    }
    trafficDriverApplyAction(owner);
    owner->result=driver->result;
}

CarDriverResult trafficDriverInitialize(TrafficDriver* driver,CarMovement* movement,
    const DriverPhysicsVehicle* physics,DriverAIEnvironment* environment,
    const GameTime* clock,struct CarSoundState* sound)
{
    if (!driver) return CAR_DRIVER_INVALID_BINDING;
    memset(driver,0,sizeof(*driver));
    CarDriverResult result=driverAIInitialize(&driver->base,movement,physics,environment,&driver->input,CAR_DRIVER_TRAFFIC);
    if (result!=CAR_DRIVER_OK) return driver->result=result;
    driver->base.owner.traffic=driver;
    const AIControlsBindings input={movement,&driver->base.physics,sound,clock};
    result=aiControlsInitialize(&driver->input,&input);
    if (result!=CAR_DRIVER_OK) {
        driverAIDispose(&driver->base);return driver->result=result;
    }
    driver->roadBranch=1;driver->base.actionCode=11;
    driver->base.thinkDelay=(float)(int16_t)movement->bindings.owner->tableIndex*-.01428571529686451f;
    driverAIBindBelt(&driver->base);
    return driver->result=CAR_DRIVER_OK;
}

void trafficDriverDispose(TrafficDriver* driver)
{
    aiControlsDispose(&driver->input);driverAIDispose(&driver->base);
}

// PAL18CAE8: independent route-number bands for the real roadBranch owner.
void trafficDriverGetRouteNumbersToUse(const TrafficDriver* driver,int32_t* first,int32_t* last)
{
    if (driver->roadBranch) { *first=50;*last=59; }
    else { *first=60;*last=69; }
}

// Complete original inline setter PAL18D030, final AI base field+2D0.
void trafficDriverSetDirection(TrafficDriver* driver,int32_t roadBranch)
{
    driver->roadBranch=roadBranch;
}


int32_t driverAIGoalUsesRoad(int32_t kind)
{
    return 1<=kind&&kind<=4;
}

#include "indep/inc/Replay.hpp"
// Full PAL18C478..18C4D4.
void trafficDriverSnapshot(TrafficDriver* driver,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"TrafficDriver");
    replaySnapshotMoveData(snapshot,&driver->roadBranch,1,4,0);
    driverAISnapshot(&driver->base,snapshot);
    replaySnapshotEndSection(snapshot);
}
