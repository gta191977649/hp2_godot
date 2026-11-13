#include "indep/inc/DrivingChoice.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/AICarController.hpp"
#include "bware/inc/bMath.hpp"
#include <math.h>

void driverAIGoalSet(DriverAIGoal* goal,int32_t kind,CarMovement* target,int32_t parameter)
{
    if((driverAIGoalUsesRoad(kind)&&!target)||(driverAIGoalUsesTarget(kind)&&target)){
        goal->kind=kind;goal->target=target;goal->parameter=parameter;
    }
}

int32_t driverAILaneForClass(int32_t selector,int32_t vehicleClass)
{
    return vehicleClass*2+(selector-1);
}

int32_t driverLaneSelectorForIndex(int32_t lane)
{
    return lane%2+1;
}

void driverLaneAdjustLateral(float* lateral,int32_t mode,const CarRecoveryPose* pose,
    int32_t lane,int32_t keepInside,float maximumStep,float halfExtent)
{
    if(mode==3)*lateral=*lateral+maximumStep;
    else if(mode==2)*lateral=*lateral-maximumStep;
    else if(*lateral<0.0f){
        if(maximumStep < -*lateral)*lateral=*lateral+maximumStep;
        else *lateral=0.0f;
    }else if(maximumStep < *lateral)*lateral=*lateral-maximumStep;
    else *lateral=0.0f;
    float lower,upper;
    if(keepInside){
        lower=(1.0f-pose->halfWidths[0])+halfExtent;
        upper=(pose->halfWidths[1]-1.0f)-halfExtent;
    }else{
        lower=-pose->halfWidths[0];
        upper=pose->halfWidths[1];
    }
    const float authored=driverLaneGeometryAuthoredLateral(pose,lane);
    lower=lower-authored;
    upper=upper-authored;
    *lateral=minimumFinite(upper,maximumFinite(*lateral,lower));
}

float driverLaneOutsideRoad(const CarRecoveryPose* pose,float across)
{
    if(0.0f<across)return across-pose->halfWidths[1];
    return -(across+pose->halfWidths[0]);
}


int32_t driverLaneGoalAllowed(const DriverAIGoal* goal,const DriverAIRoadState* road,
    DriverAIPerception* perception,int32_t vehicleClass)
{
    if(driverAIGoalUsesRoad(goal->kind)){
        const int32_t id=driverAIRoadEmpty(road)?-1:road->group->id;
        if((uint32_t)id<50u){
            const int32_t lane=driverAILaneForClass(goal->kind,vehicleClass);
            return (road->group->laneFlags[lane]&3u)!=0;
        }
        if((uint32_t)(id-50)<20u)return goal->kind==1;
        return 0;
    }
    return driverAIGoalUsesTarget(goal->kind)&&driverPerceptionFind(perception,goal->target)!=NULL;
}

int32_t driverLaneGoalsMatch(const DriverAIGoal* first,const DriverAIGoal* second)
{
    if(driverAIGoalUsesTarget(first->kind))
        return first->target==second->target&&first->kind==second->kind;
    return first->kind==second->kind;
}
#include "indep/inc/World.hpp"
#include "indep/inc/Replay.hpp"
// Full PAL241B88..241C00. The native target is the same Car's movement
// view; its encoded identity is the source signed driver-number byte.
void driverAIGoalSnapshot(DriverAIGoal* goal,const World* world,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"DrivingChoice");
    replaySnapshotMoveData(snapshot,&goal->kind,1,-1,0);
    Car* car=goal->target?goal->target->bindings.owner:nullptr;
    worldSnapshotCarPointer(world,snapshot,&car);
    if(snapshot->restoring)goal->target=car?car->Movement():nullptr;
    replaySnapshotMoveData(snapshot,&goal->parameter,1,4,0);
    replaySnapshotEndSection(snapshot);
}

#include "indep/inc/AIPlan.hpp"
#include "indep/inc/CarCollisionDetect.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/TrackPath.hpp"
CarDriverResult driverLaneBuildPointRange(DriverLanePoints* points,const DriverAIRoadState* road,
    const CarRecoveryPose* last)
{
    points->count=0;
    if(driverAIRoadEmpty(road)||!last || road->lane<0 || road->lane>=21)
        return CAR_DRIVER_MISSING_ROAD;
    const CarRecoveryGroup* group=road->group;
    const int32_t first=carRecoveryOwnedPoseIndex(group,road->recovery.recoveryPose);
    const int32_t finish=carRecoveryOwnedPoseIndex(group,last);
    if(first<0||first>=group->poseCount||finish<0||finish>=group->poseCount)return CAR_DRIVER_MISSING_ROAD;
    for(int32_t index=first;index<=finish&&points->count<DRIVER_COMMAND_CAPACITY;++index){
        DriverLanePoint* point=&points->points[points->count++];
        point->road.group=group;point->road.pose=&group->poses[index];point->lane=road->lane;
        point->lateralDelta=0.0f;point->changedConnection=0;
    }
    return CAR_DRIVER_OK;
}
#include "indep/inc/CameraMover.hpp"
static float authoredLateral(const DriverLanePoint* point)
{
    return (float)((int32_t)point->road.pose->laneOffsetsQuantized[point->lane]*256)*0.0000152587890625f;
}
static float distanceXY(const float first[2],const float second[2])
{
    const float x=first[0]-second[0];
    const float y=first[1]-second[1];
    const float xx=x*x;
    const float yy=y*y;
    return sqrtf(xx+yy);
}
void driverLaneAppendTransition(const DriverLanePoints* points,DriverCommand* command,
    const DriverAIPerception* perception,int32_t targetIndex,int32_t keepInside,
    float initialLateral,float finalLateral)
{
    if(!points || !command || !perception || targetIndex<3 ||
        points->count>DRIVER_COMMAND_CAPACITY || targetIndex-2>=points->count) return;
    const int32_t last=targetIndex-2;
    // Every authored point and lane must be live before writing any of the
    // resulting driver command. An incomplete command can lead AI through
    // scenery after the track owner was changed during Replay/menu teardown.
    for(int32_t i=0;i<=last;++i) {
        const DriverLanePoint& point=points->points[i];
        if (carRecoveryOwnedPoseIndex(point.road.group,point.road.pose)<0 ||
            point.lane<0 || point.lane>=21) return;
    }
    const DriverLanePoint* first=&points->points[0];
    const DriverLanePoint* finish=&points->points[last];
    float start[2],end[2];
    carRoadPreviewPointXY(first->road.pose,start,0.0f,authoredLateral(first));
    carRoadPreviewPointXY(finish->road.pose,end,0.0f,authoredLateral(finish));
    const float slope=finiteRatio(finalLateral-initialLateral,distanceXY(start,end));
    for(int32_t index=0;index<=last&&command->count<DRIVER_COMMAND_CAPACITY;++index){
        const DriverLanePoint* point=&points->points[index];
        const CarRecoveryPose* pose=point->road.pose;
        const float authored=authoredLateral(point);
        float center[2];
        carRoadPreviewPointXY(pose,center,0.0f,authored);
        const float change=distanceXY(start,center)*slope;
        float lateral=initialLateral+change;
        if(initialLateral<finalLateral){
            if(finalLateral<lateral)lateral=finalLateral;
        }else if(finalLateral<=initialLateral&&lateral<finalLateral)lateral=finalLateral;
        const float across=(lateral+authored)+point->lateralDelta;
        float lower,upper;
        if(keepInside){
            lower=(1.0f-pose->halfWidths[0])+perception->lateralExtent;
            upper=(pose->halfWidths[1]-1.0f)-perception->lateralExtent;
        }else{
            lower=-pose->halfWidths[0];upper=pose->halfWidths[1];
        }
        const float bounded=minimumFinite(upper,maximumFinite(across,lower));
        float position[4];driverRoadPointPosition(pose,position,0.0f,bounded);
        driverCommandAppend(command,position,&point->road,point->lane,1,NULL);
    }
}
CarDriverResult driverLaneContinueTargetCommand(const DriverAIGoal* goal,DriverCommand* command,
    DriverAIPerception* perception,const DriverLanePoints* points,float acrossOffset)
{
    CarMovement* target=goal->target;
    const VehicleState* targetState=target->bindings.state;
    const VehicleState* state=perception->movement->bindings.state;
    if(!(5.0f<targetState->speed)||!(5.0f<state->speed)||targetState->activeContactCount<3)
        return CAR_DRIVER_OK;
    float ownDirection[2],targetDirection[2];
    vectorNormalize2(ownDirection,state->velocity);
    vectorNormalize2(targetDirection,targetState->velocity);
    const float x=ownDirection[0]*targetDirection[0];
    const float y=ownDirection[1]*targetDirection[1];
    if(!(0.707099974155426f<x+y))return CAR_DRIVER_OK;
    CarDriver* controller=(CarDriver*)target->bindings.owner->controller;
    DriverCommand* targetCommand=carDriverMutableCommand(controller);
    if(!targetCommand)return CAR_DRIVER_INVALID_BINDING;
    const int32_t now=target->bindings.environment->clock->worldTimer;
    int32_t index=driverCommandClosestPoint(targetCommand,targetState,now);
    int32_t nearest=0;
    CarRecoverySelection road;
    while(command->count<DRIVER_COMMAND_CAPACITY&&driverCommandRoadAt(targetCommand,index,&road)){
        const CarRecoveryPose* pose=road.pose;
        const float* position=targetCommand->points[index].position;
        const float dx=position[0]-pose->position[0];
        const float dy=position[1]-pose->position[1];
        const float first=dx*pose->direction[1];
        const float second=dy*pose->direction[0];
        float across=(first-second)+acrossOffset;
        float lower=-999999.0f,upper=999999.0f;
        if(road.group->category!=1){
            lower=-pose->halfWidths[0];upper=pose->halfWidths[1];
        }else{
            nearest=driverLaneNearestFollowingPoint(points,position,nearest);
            if(nearest!=-1){
                const CarRecoveryPose* ownPose=points->points[nearest].road.pose;
                float left[2],right[2];
                carRoadPreviewPointXY(ownPose,left,0.0f,-ownPose->halfWidths[0]);
                carRoadPreviewPointXY(ownPose,right,0.0f,ownPose->halfWidths[1]);
                const float leftX=(left[0]-pose->position[0])*pose->direction[1];
                const float leftY=(left[1]-pose->position[1])*pose->direction[0];
                const float rightX=(right[0]-pose->position[0])*pose->direction[1];
                const float rightY=(right[1]-pose->position[1])*pose->direction[0];
                const float firstSide=leftX-leftY;
                const float secondSide=rightX-rightY;
                if(firstSide<secondSide){lower=firstSide;upper=secondSide;}
                else{lower=secondSide;upper=firstSide;}
            }
        }
        const float margin=perception->lateralExtent+1.0f;
        lower=lower+margin;upper=upper-margin;
        across=minimumFinite(upper,maximumFinite(across,lower));
        float next[4];driverRoadPointPosition(pose,next,0.0f,across);
        driverCommandAppend(command,next,&road,targetCommand->points[index].laneIndex,1,state);
        ++index;
    }
    return CAR_DRIVER_OK;
}
int32_t driverLaneBuildTargetPath(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver)
{
    DriverAINeighbor* neighbor=driverPerceptionFind(perception,goal->target);
    if(!neighbor){driver->result=CAR_DRIVER_INVALID_BINDING;return 0;}
    DriverLanePoints points;
    float initialLateral=0.0f;
    CarDriverResult result=driverLaneBuildPoints(&points,command,road,perception,driver,&initialLateral);
    if(result!=CAR_DRIVER_OK){driver->result=result;return 0;}
    int32_t nearest=-1;
    if(points.count)nearest=driverLaneNearestFollowingPoint(&points,goal->target->bindings.state->position,0);
    if(nearest!=-1){
        float passing=driverLanePassingMargin(driver,perception,neighbor,&points,nearest);
        if(goal->parameter)passing=passing+4.0f;
        const DriverLanePoint* point=&points.points[nearest];
        const CarRecoveryPose* pose=point->road.pose;
        const float* targetPosition=goal->target->bindings.state->position;
        const float dx=targetPosition[0]-pose->position[0];
        const float dy=targetPosition[1]-pose->position[1];
        const float firstSide=dx*pose->direction[1];
        const float secondSide=dy*pose->direction[0];
        const float side=firstSide-secondSide;
        if(driverLaneOutsideRoad(pose,side)<5.0f){
            /* The original optional call here only counts down through NOPs. */
            float sign=goal->kind==5?-1.0f:1.0f;
            const int32_t ownClass=perception->movement->bindings.owner->driverInfo->driverClass;
            if(ownClass==3&&driver->environment->tuning->predictionWideBounds)sign=-sign;
            const float firstWidth=perception->lateralExtent+neighbor->effectiveHalfExtents[0];
            const float totalWidth=firstWidth+passing;
            const float offset=sign*totalWidth;
            float across=(side+offset)+point->lateralDelta;
            const float lower=perception->lateralExtent-(pose->halfWidths[0]-1.0f);
            const float upper=(pose->halfWidths[1]-1.0f)-perception->lateralExtent;
            across=minimumFinite(upper,maximumFinite(across,lower));
            const float finalLateral=across-authoredLateral(point);
            const float alongX=dx*pose->direction[0];
            const float alongY=dy*pose->direction[1];
            float position[4];driverRoadPointPosition(pose,position,alongX+alongY,across);
            driverLaneAppendTransition(&points,command,perception,nearest,1,initialLateral,finalLateral);
            driverCommandAppend(command,position,&point->road,point->lane,1,NULL);
            command->lastSpeedPoint=command->count-1;
            const CarMovement* target=goal->target;
            const int32_t targetClass=target->bindings.owner->driverInfo->driverClass;
            const int32_t continuePath=target->bindings.player||(targetClass==3&&ownClass!=3);
            if(continuePath&&targetClass!=4){
                result=driverLaneContinueTargetCommand(goal,command,perception,&points,offset);
                if(result!=CAR_DRIVER_OK){driver->result=result;return 0;}
            }
            result=driverLaneBuildRoad(command,road,perception,1,1,driver);
            if(result!=CAR_DRIVER_OK){driver->result=result;return 0;}
            return 1;
        }
    }
    result=driverLaneBuildRoad(command,road,perception,1,1,driver);
    if(result!=CAR_DRIVER_OK)driver->result=result;
    return 0;
}
#include "indep/inc/TrafficDriver.hpp"
static float driverLaneDecisionAuthoredLateral(const CarRecoveryPose* pose,int32_t lane)
{
    return (float)((int32_t)pose->laneOffsetsQuantized[lane]*256)*0.0000152587890625f;
}
static float lateralPosition(const CarRecoveryPose* pose,const float position[2])
{
    const float x=(position[0]-pose->position[0])*pose->direction[1];
    const float y=(position[1]-pose->position[1])*pose->direction[0];
    return x-y;
}
int32_t driverLaneTargetHasRoom(const DriverAIGoal* goal,const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception)
{
    if(driverAIGoalUsesRoad(goal->kind))return 1;
    if(!driverAIGoalUsesTarget(goal->kind))return 0;
    const CarDriver* target=(const CarDriver*)goal->target->bindings.owner->controller;
    if(!(carDriverLateralExcess(target)<-0.0010000000474974513f))return 1;
    const CarRecoveryPose* pose=carDriverCurrentPose(target,1);
    const float across=lateralPosition(pose,goal->target->bindings.state->position);
    const float width=neighbor->effectiveHalfExtents[0]+perception->lateralExtent;
    const float clearance=width+5.0f;
    if(goal->kind==5)return -pose->halfWidths[0]<across-clearance;
    return across+clearance<pose->halfWidths[1];
}
int32_t driverLanePreferredPassingSide(const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception,const DriverAIRoadState* road)
{
    const float curvature=driverLaneRoadCurvature(road);
    const CarRecoveryPose* first=road->recovery.recoveryPose;
    const VehicleState* other=neighbor->movement->bindings.state;
    const CarRecoveryPose* following=carRecoveryFirstFollowingPose(road->group,other->position,first);
    if(!following)following=first;
    const float raw=lateralPosition(following,other->position);
    const float own=lateralPosition(first,perception->movement->bindings.state->position)-driverLaneDecisionAuthoredLateral(first,road->lane);
    const float target=raw-driverLaneDecisionAuthoredLateral(following,road->lane);
    int32_t selected=1,forced=0;
    if(perception->movement->bindings.owner->driverInfo->driverClass==3&&target<5.0f){selected=0;forced=1;}
    if(!forced&&fabsf(own)<=1.0f&&fabsf(target)<=1.0f&&50.0f<neighbor->localPosition[1]&&
       0.11999999731779099f<=fabsf(curvature)){
        selected=0.0f<curvature?0:1;forced=1;
    }
    if(!forced)selected=target<own?0:1;
    const float width=neighbor->effectiveHalfExtents[0]+perception->lateralExtent;
    const float clearance=width+5.0f;
    const int32_t fits=selected?!(raw-clearance < -following->halfWidths[0]):
        !(following->halfWidths[1]<raw+clearance);
    return fits?selected:selected^1;
}
CarDriverResult driverLanePublishTarget(const DriverAIGoal* goal,DriverAIBase* driver,
    int32_t enabled,DriverAIPerception* perception)
{
    if(driverAIGoalUsesRoad(goal->kind)){
        driver->planningTarget=NULL;return CAR_DRIVER_OK;
    }
    if(!driverAIGoalUsesTarget(goal->kind))return CAR_DRIVER_OK;
    if(!enabled){driver->planningTarget=NULL;return CAR_DRIVER_OK;}
    CarMovement* target=goal->target;
    int32_t allowed=1;
    if(target->bindings.owner->driverInfo->driverClass==3){
        const float x=target->bindings.state->orientation[0]*perception->movement->bindings.state->orientation[0];
        const float y=target->bindings.state->orientation[1]*perception->movement->bindings.state->orientation[1];
        if(x+y<0.0f){
            const CarDriver* controller=(const CarDriver*)target->bindings.owner->controller;
            const DriverAIBase* other=NULL;
            if(controller->kind==CAR_DRIVER_AI&&controller->owner.ai)other=&controller->owner.ai->base;
            if(controller->kind==CAR_DRIVER_TRAFFIC&&controller->owner.traffic)other=&controller->owner.traffic->base;
            if(!other||!other->currentGoal)return CAR_DRIVER_INVALID_BINDING;
            const DriverAIGoal* current=other->currentGoal;
            const int32_t action=other->action?driverAIActionKind(other->action):11;
            if(driverAIGoalUsesTarget(current->kind)&&current->target==perception->movement){
                if((current->kind==5&&goal->kind==6)||(current->kind==6&&goal->kind==5))allowed=0;
            }
            if(action==6&&goal->kind==5)allowed=0;
        }
    }
    if(allowed){
        DriverAINeighbor* neighbor=driverPerceptionFind(perception,target);
        if(!neighbor)return CAR_DRIVER_INVALID_BINDING;
        if(driverLaneTargetHasRoom(goal,neighbor,perception))driver->planningTarget=target;
    }
    return CAR_DRIVER_OK;
}
void driverLaneAppendCommandPoint(DriverCommand* command,const DriverLanePoint* point,float lateral)
{
    float position[4];
    const float across=lateral+driverLaneGeometryAuthoredLateral(point->road.pose,point->lane);
    driverRoadPointPosition(point->road.pose,position,0.0f,across);
    driverCommandAppend(command,position,&point->road,point->lane,1,NULL);
}
int32_t driverLanePlanGoal(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,int32_t mode)
{
    const CarMovement* movement=perception->movement;
    const CarDriver* controller=(const CarDriver*)movement->bindings.owner->controller;
    const DriverCommand* input=carDriverCommand(controller);
    if(!input){driver->result=CAR_DRIVER_INVALID_BINDING;return 0;}
    const int32_t now=movement->bindings.environment->clock->worldTimer;
    driverCommandCopy(command,input);
    driverCommandAdvance(command,movement->bindings.state,now,0.0f);
    const int32_t allowed=driverLaneGoalAllowed(goal,road,perception,driver->physics.routeVehicleClass);
    int32_t accepted=0;
    CarDriverResult result=CAR_DRIVER_OK;
    if(allowed){
        if(driverAIGoalUsesRoad(goal->kind)){
            DriverAIRoadState selected=*road;
            const int32_t lane=driverAILaneForClass(goal->kind,driver->physics.routeVehicleClass);
            driverAIRoadSetLane(&selected,lane,0,now);
            result=driverLaneBuildRoad(command,&selected,perception,mode,1,driver);
            accepted=1;
        }else accepted=driverLaneBuildTargetPath(goal,command,road,perception,driver);
    }else result=driverLaneBuildRoad(command,road,perception,mode,1,driver);
    if(result!=CAR_DRIVER_OK){driver->result=result;return 0;}
    /* This caller disables target publication; an unrelated goal kind does
     * not overwrite the current target. */
    result=driverLanePublishTarget(goal,driver,0,perception);
    if(result!=CAR_DRIVER_OK){driver->result=result;return 0;}
    return accepted;
}
int32_t driverLaneEvaluateGoal(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,
    int32_t mode,int32_t followTarget,float horizon,float* speed,CarMovement** collision,float* time)
{
    *speed=-1.0f;
    *collision=NULL;
    *time=99999.0f;
    if(!driverLanePlanGoal(goal,command,road,perception,driver,mode))return 0;
    *speed=driverCommandPlanSpeed(command,&driver->physics);
    if(!(-1.0f<*speed))return 0;
    if(followTarget){
        *speed=*speed*1.2000000476837158f;
        driver->planningTarget=goal->target;
    }else driver->planningTarget=NULL;
    *collision=driverCollisionPredictNeighbors(driver,command,perception,road,horizon,time,
        driver->environment->simulation->world);
    return horizon<*time;
}
#include "indep/inc/Race.hpp"
static float driverLanePlanAuthoredLateral(const CarRecoveryPose* pose,int32_t lane)
{
    return (float)((int32_t)pose->laneOffsetsQuantized[lane]*256)*0.0000152587890625f;
}
float driverLaneLateralStep(const DriverAIBase* driver,const DriverAIRoadState* road,int32_t mode)
{
    if(mode==1)return 0.0f;
    if(mode==2||mode==3||driver->physics.movement->bindings.ownerMotion->inputCategory==CAR_DRIVER_TRAFFIC)
        return 0.5f;
    const WorldSimulation* simulation=driver->environment->simulation;
    if(raceFrameConnectionWindowActive(simulation->race,driver->physics.movement->bindings.owner,
        &raceConnectionDefaultTiming))return 0.0f;
    const float speed=driver->physics.movement->bindings.state->speed;
    if(speed<=driver->environment->tuning->lateralStepSpeed)return 0.0f;
    if(speed<=driver->environment->tuning->lateralStepFastSpeed)return 0.25f;
    const int32_t now=driver->physics.movement->bindings.environment->clock->worldTimer;
    return driverAIRoadSelectionLocked(road,now)?0.25f:0.5f;
}
int32_t driverLaneAdvancePoint(DriverAIBase* driver,CarRecoverySelection* road,int32_t* lane,
    float* lateralDelta,int32_t* changedConnection)
{
    // The command can outlive a route selection during Replay or a road
    // branch transition. Its point must still belong to this exact group.
    if (!driver || !road || !lane || !lateralDelta || !changedConnection ||
        *lane<0 || *lane>=21 ||
        carRecoveryOwnedPoseIndex(road->group,road->pose)<0) {
        if (driver) driver->result=CAR_DRIVER_MISSING_ROAD;
        return 0;
    }
    const CarRecoveryPointRegistry* registry=driver->physics.movement->bindings.recoveryPoints;
    const CarRecoveryPose* current=road->pose;
    if((current->flags&255u)!=255u){
        const CarRecoveryConnection* connection=carRecoveryPointConnection(registry,current);
        if(!connection){driver->result=CAR_DRIVER_MISSING_ROAD;return 0;}
        const float previousLateral=driverLanePlanAuthoredLateral(current,*lane);
        if(driverAISelectConnection(driver,connection,lane,driver->currentRoad)){
            float point[4];
            driverRoadPointPosition(current,point,0.0f,previousLateral);
            road->group=carRecoveryConnectionGroup(registry,connection);
            road->pose=carRecoveryConnectionPose(registry,connection);
            if(!road->group||!road->pose){driver->result=CAR_DRIVER_MISSING_ROAD;return 0;}
            const float x=point[0]-road->pose->position[0];
            const float y=point[1]-road->pose->position[1];
            const float first=x*road->pose->direction[1];
            const float second=y*road->pose->direction[0];
            *lateralDelta=(first-second)-driverLanePlanAuthoredLateral(road->pose,*lane);
            *changedConnection=1;
            return 1;
        }
        if(driver->result!=CAR_DRIVER_OK)return 0;
    }
    const int32_t next=carRecoveryOwnedPoseIndex(road->group,current)+1;
    if(next>=road->group->poseCount)return 0;
    road->pose=&road->group->poses[next];
    *lateralDelta=0.0f;
    *changedConnection=0;
    return 1;
}
static void appendLanePoint(DriverLanePoints* points,const CarRecoverySelection* road,
    int32_t lane,float lateralDelta,int32_t changedConnection)
{
    DriverLanePoint* point=&points->points[points->count++];
    point->road=*road;
    point->lane=lane;
    point->lateralDelta=lateralDelta;
    point->changedConnection=changedConnection;
}
CarDriverResult driverLaneBuildPoints(DriverLanePoints* points,const DriverCommand* command,
    const DriverAIRoadState* road,const DriverAIPerception* perception,DriverAIBase* driver,float* lateral)
{
    points->count=0;
    CarRecoverySelection cursor={road->group,road->recovery.recoveryPose};
    int32_t lane=road->lane,changed=0;
    float lateralDelta=0.0f;
    if(command->count>0){
        const DriverCommandPoint* last=&command->points[command->count-1];
        cursor.group=last->road.group;
        if(perception->movement->bindings.owner->driverInfo->driverClass==2&&cursor.group&&cursor.group->category==1)
            cursor.group=road->group;
        else{
            cursor.pose=last->road.pose;
            lane=last->laneIndex;
        }
        if(!cursor.group||!cursor.pose)return CAR_DRIVER_MISSING_ROAD;
        for(;;){
            if(!driverLaneAdvancePoint(driver,&cursor,&lane,&lateralDelta,&changed))return driver->result;
            const float x=last->position[0]-cursor.pose->position[0];
            const float y=last->position[1]-cursor.pose->position[1];
            const float first=x*cursor.pose->direction[0];
            const float second=y*cursor.pose->direction[1];
            if(!(-10.0f<first+second))break;
        }
    }
    if(!cursor.pose)return CAR_DRIVER_OK;
    const float* position=perception->movement->bindings.state->position;
    if(command->count>0){
        const DriverCommandPoint* last=&command->points[command->count-1];
        CarRecoverySelection previous;
        const CarRecoveryPose* basis=driverCommandRoadAt(command,command->count-1,&previous)?previous.pose:cursor.pose;
        const float x=last->position[0]-basis->position[0];
        const float y=last->position[1]-basis->position[1];
        const float first=x*basis->direction[1];
        const float second=y*basis->direction[0];
        *lateral=first-second;
    }else{
        const float x=position[0]-cursor.pose->position[0];
        const float y=position[1]-cursor.pose->position[1];
        const float first=x*cursor.pose->direction[1];
        const float second=y*cursor.pose->direction[0];
        *lateral=first-second;
    }
    *lateral=*lateral-driverLanePlanAuthoredLateral(cursor.pose,road->lane);
    appendLanePoint(points,&cursor,lane,lateralDelta,0);
    if(!driverLaneAdvancePoint(driver,&cursor,&lane,&lateralDelta,&changed))return driver->result;
    appendLanePoint(points,&cursor,lane,lateralDelta,0);
    const float x=position[0]-cursor.pose->position[0];
    const float y=position[1]-cursor.pose->position[1];
    float distance=sqrtf(x*x+y*y);
    const float range=driverPlanningDistance(&driver->physics);
    while((distance<=range||points->count<5)&&points->count<DRIVER_COMMAND_CAPACITY){
        distance=distance+cursor.pose->segmentLength;
        if(!driverLaneAdvancePoint(driver,&cursor,&lane,&lateralDelta,&changed))return driver->result;
        appendLanePoint(points,&cursor,lane,lateralDelta,changed);
    }
    return driver->result;
}
CarDriverResult driverLaneBuildRoad(DriverCommand* command,const DriverAIRoadState* road,
    DriverAIPerception* perception,int32_t mode,int32_t keepInside,DriverAIBase* driver)
{
    if(!road||!command||!perception||driverAIRoadEmpty(road))
        return CAR_DRIVER_OK;
    command->roadFlags=(road->recovery.recoveryPose->flags>>16)&63u;
    if(command->count>=DRIVER_COMMAND_CAPACITY)return CAR_DRIVER_OK;
    DriverLanePoints points;
    float lateral=0.0f;
    CarDriverResult result=driverLaneBuildPoints(&points,command,road,perception,driver,&lateral);
    if(result!=CAR_DRIVER_OK)return result;
    float step=driverLaneLateralStep(driver,road,mode);
    float offsets[DRIVER_COMMAND_CAPACITY];
    for(int32_t index=0;index<points.count;++index){
        const DriverLanePoint* point=&points.points[index];
        if(mode==0&&point->changedConnection)step=minimumFinite(step,0.25f);
        driverLaneAdjustLateral(&lateral,mode,point->road.pose,road->lane,
            keepInside,step,perception->lateralExtent);
        lateral=lateral+point->lateralDelta;
        offsets[index]=lateral;
    }
    const int32_t disabled=driver->environment->simulation->collisions->carPolicy.secondaryCollisionDisabled;
    for(int32_t index=0;index<points.count;++index){
        if(disabled&&mode==0)offsets[index]=0.0f;
        driverLaneAppendCommandPoint(command,&points.points[index],offsets[index]);
    }
    /* This caller constructs a road goal (lane parity selects kind 1 or 2).
     * Its original goal-publication branch clears the same planner target. */
    driver->planningTarget=NULL;
    return CAR_DRIVER_OK;
}
int32_t driverLaneNearestFollowingPoint(const DriverLanePoints* points,const float position[2],int32_t first)
{
    float nearestDistance=100000000376832.0f;
    int32_t nearest=-1;
    for(int32_t index=first<0?0:first;index<points->count;){
        const CarRecoveryPose* pose=points->points[index].road.pose;
        const float x=pose->position[0]-position[0];
        const float y=pose->position[1]-position[1];
        const float distance=sqrtf(x*x+y*y);
        const float firstProjection=(position[0]-pose->position[0])*pose->direction[0];
        const float secondProjection=(position[1]-pose->position[1])*pose->direction[1];
        const float projection=-(firstProjection+secondProjection);
        if(0.0f<projection&&distance<nearestDistance){nearestDistance=distance;nearest=index;}
        const int32_t skip=convertFloatToInt32(distance*0.0416666679084301f)-2;
        index=index+(0<skip?skip:0)+1;
    }
    return nearest;
}
float driverLanePassingMargin(const DriverAIBase* driver,const DriverAIPerception* perception,
    const DriverAINeighbor* neighbor,const DriverLanePoints* points,int32_t nearest)
{
    if(!neighbor)return 0.0f;
    float clearance=0.0f;
    if(points->count){
        if(nearest==-1)
            nearest=driverLaneNearestFollowingPoint(points,neighbor->movement->bindings.state->position,0);
        if(nearest!=-1){
            const DriverLanePoint* first=&points->points[0];
            const DriverLanePoint* target=&points->points[nearest];
            const float* ownPosition=perception->movement->bindings.state->position;
            const float* targetPosition=neighbor->movement->bindings.state->position;
            const CarRecoveryPose* ownPose=first->road.pose;
            const CarRecoveryPose* targetPose=target->road.pose;
            const float ownX=(ownPosition[0]-ownPose->position[0])*ownPose->direction[1];
            const float ownY=(ownPosition[1]-ownPose->position[1])*ownPose->direction[0];
            const float targetX=(targetPosition[0]-targetPose->position[0])*targetPose->direction[1];
            const float targetY=(targetPosition[1]-targetPose->position[1])*targetPose->direction[0];
            const float ownLateral=((ownX-ownY)-authoredLateral(first))+first->lateralDelta;
            const float targetLateral=((targetX-targetY)-authoredLateral(target))+target->lateralDelta;
            const float distance=fabsf(ownLateral-targetLateral)-perception->lateralExtent;
            clearance=distance-neighbor->effectiveHalfExtents[0];
        }
    }
    float margin=2.0f;
    if(perception->movement->bindings.owner->driverInfo->driverClass!=3){
        const int32_t offroad=neighbor->movement->bindings.owner->driverInfo->driverClass==3;
        const float threshold=offroad?driver->environment->tuning->offroadStationaryPassSpeed:
            driver->environment->tuning->stationaryPassSpeed;
        if(fabsf(neighbor->movement->bindings.state->speed)<=threshold)margin=6.0f;
        else if(neighbor->movement->bindings.player)margin=1.5f;
    }
    return maximumFinite(clearance,margin);
}
#include "indep/inc/PsychoDriver.hpp"
int32_t driverLanePlanRoadGoal(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,int32_t mode)
{
    if(!driverAIGoalUsesRoad(goal->kind)){
        driver->result=CAR_DRIVER_INVALID_BINDING;
        return 0;
    }
    const int32_t now=driver->physics.movement->bindings.environment->clock->worldTimer;
    driverCommandCopy(command,&driver->controls->command);
    driverCommandAdvance(command,driver->physics.movement->bindings.state,now,0.0f);
    const int32_t allowed=driverLaneGoalAllowed(goal,road,perception,driver->physics.routeVehicleClass);
    CarDriverResult result;
    if(allowed){
        DriverAIRoadState selected=*road;
        const int32_t lane=driverAILaneForClass(goal->kind,driver->physics.routeVehicleClass);
        driverAIRoadSetLane(&selected,lane,0,now);
        result=driverLaneBuildRoad(command,&selected,perception,mode,1,driver);
    }else result=driverLaneBuildRoad(command,road,perception,mode,1,driver);
    if(result!=CAR_DRIVER_OK){driver->result=result;return 0;}
    driver->planningTarget=NULL;
    return allowed;
}
int32_t driverLaneEvaluateRoadGoal(const DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,int32_t mode,float horizon,
    float* speed,CarMovement** collision,float* time)
{
    *speed=-1.0f;
    *collision=NULL;
    *time=99999.0f;
    if(!driverLanePlanRoadGoal(goal,command,road,perception,driver,mode))return 0;
    *speed=driverCommandPlanSpeed(command,&driver->physics);
    if(!(-1.0f<*speed))return 0;
    driver->planningTarget=NULL;
    *collision=driverCollisionPredictNeighbors(driver,command,perception,road,horizon,time,
        driver->environment->simulation->world);
    return horizon<*time;
}
static void copyGoal(DriverAIGoal* destination,const DriverAIGoal* source)
{
    driverAIGoalSet(destination,source->kind,source->target,source->parameter);
}
int32_t driverLaneConsiderGoal(const DriverAIGoal* goal,int32_t followTarget,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,
    DriverLaneSearch* search,CarMovement* previousCollision,float horizon)
{
    if(perception->movement->bindings.owner->driverInfo->driverClass==3&&
       driver->environment->tuning->predictionWideBounds){
        if(!driverAIGoalUsesTarget(goal->kind)||
           (goal->target->bindings.owner->driverInfo->driverClass!=2&&!goal->target->bindings.player))return 0;
    }
    float speed=-1.0f;
    search->collision=NULL;search->collisionTime=99999.0f;
    const int32_t clear=driverLaneEvaluateGoal(goal,&search->candidateCommand,road,perception,driver,
        0,followTarget,horizon,&speed,&search->collision,&search->collisionTime);
    if(clear&&search->bestSpeed<speed){
        search->accepted=1;
        copyGoal(&search->bestGoal,goal);
        driverCommandCopy(&search->bestCommand,&search->candidateCommand);
        search->followTarget=0;search->bestSpeed=speed;
    }
    const int32_t usable=!(fabsf(speed-(-1.0f))<=0.0010000000474974513f);
    if(driverAIGoalUsesTarget(goal->kind)&&goal->parameter==0&&goal->target==previousCollision&&followTarget){
        copyGoal(&search->fallbackGoal,goal);
        driverCommandCopy(&search->fallbackCommand,&search->candidateCommand);
        search->fallbackTime=search->collisionTime;
    }
    return usable;
}
int32_t driverLaneSelectGoal(DriverAIGoal* goal,DriverCommand* command,
    const DriverAIRoadState* road,DriverAIPerception* perception,DriverAIBase* driver,
    int32_t* followTarget,int32_t tryCurrent,float horizon)
{
    DriverLaneSearch search;
    driverCommandInitialize(&search.fallbackCommand);
    driverCommandInitialize(&search.bestCommand);
    driverCommandInitialize(&search.candidateCommand);
    driverAIGoalInitialize(&search.bestGoal);driverAIGoalInitialize(&search.fallbackGoal);
    search.accepted=0;search.followTarget=0;search.bestSpeed=-1.0f;search.fallbackTime=99999.0f;
    search.collision=NULL;search.collisionTime=99999.0f;
    DriverAIGoal current;
    driverAIGoalInitialize(&current);
    driverAIGoalSet(&current,driverLaneSelectorForIndex(road->lane),NULL,0);
    if(driverLaneGoalAllowed(goal,road,perception,driver->physics.routeVehicleClass))copyGoal(&current,goal);
    CarMovement* previousCollision=NULL;
    float currentSpeed=-1.0f,currentTime=99999.0f;
    if(tryCurrent){
        int32_t follow=0;
        if(driverAIGoalUsesTarget(current.kind)){
            DriverAINeighbor* neighbor=driverPerceptionFind(perception,current.target);
            if(neighbor){
                const int32_t side=driverLanePreferredPassingSide(neighbor,perception,road);
                follow=side?current.kind==5:current.kind==6;
            }
        }
        driverLaneConsiderGoal(&current,follow,road,perception,driver,&search,NULL,horizon);
        currentSpeed=search.bestSpeed;currentTime=search.collisionTime;
        previousCollision=search.collision;
        if(follow&&current.target==search.collision){
            search.fallbackGoal=current;
            driverCommandCopy(&search.fallbackCommand,&search.candidateCommand);
        }
    }
    DriverAIGoal candidate;
    driverAIGoalInitialize(&candidate);
    const int32_t count=driver->environment->tuning->laneCandidateCount;
    for(int32_t selector=1;!search.accepted&&selector<=count;++selector){
        driverAIGoalSet(&candidate,selector,NULL,0);
        if(tryCurrent&&driverLaneGoalsMatch(&candidate,&current))continue;
        driverLaneConsiderGoal(&candidate,0,road,perception,driver,&search,previousCollision,horizon);
    }
    if(!search.accepted){
        for(int32_t index=0;index<perception->count;++index){
            const DriverAINeighbor* neighbor=&perception->neighbors[index];
            if(!driverCollisionNeighborEligible(neighbor,perception))continue;
            const int32_t side=driverLanePreferredPassingSide(neighbor,perception,road);
            int32_t usable=0;
            driverAIGoalSet(&candidate,5,neighbor->movement,0);
            if(!tryCurrent||!driverLaneGoalsMatch(&candidate,&current))
                usable=driverLaneConsiderGoal(&candidate,side,road,perception,driver,&search,previousCollision,horizon)!=0;
            driverAIGoalSet(&candidate,6,neighbor->movement,0);
            if(!tryCurrent||!driverLaneGoalsMatch(&candidate,&current)){
                const int32_t evaluated=driverLaneConsiderGoal(&candidate,side^1,road,perception,driver,&search,previousCollision,horizon);
                usable=usable||evaluated!=0;
            }
            if(!usable){
                const float* other=neighbor->movement->bindings.state->velocity;
                const float* own=perception->movement->bindings.state->velocity;
                const float first=other[0]*own[0];
                const float second=other[1]*own[1];
                if(first+second<0.0f){
                    driverAIGoalSet(&candidate,side?5:6,neighbor->movement,1);
                    driverLaneConsiderGoal(&candidate,1,road,perception,driver,&search,previousCollision,horizon);
                }
            }
        }
    }
    if(search.accepted){
        copyGoal(goal,&search.bestGoal);driverCommandCopy(command,&search.bestCommand);
        *followTarget=search.followTarget;
    }else if(driverLaneGoalAllowed(&search.fallbackGoal,road,perception,driver->physics.routeVehicleClass)&&
             currentTime-0.10000000149011612f<=search.fallbackTime){
        copyGoal(goal,&search.fallbackGoal);driverCommandCopy(command,&search.fallbackCommand);*followTarget=1;
    }else if(!(fabsf(currentSpeed-(-1.0f))<=0.0010000000474974513f)){
        copyGoal(goal,&current);driverLanePlanGoal(&current,command,road,perception,driver,1);*followTarget=0;
    }else{
        driverAIGoalSet(goal,driverLaneSelectorForIndex(road->lane),NULL,0);
        const CarDriverResult result=driverLaneBuildRoad(command,road,perception,1,1,driver);
        if(result!=CAR_DRIVER_OK)driver->result=result;
        *followTarget=0;search.accepted=0;
    }
    const CarDriverResult published=driverLanePublishTarget(goal,driver,*followTarget,perception);
    if(published!=CAR_DRIVER_OK)driver->result=published;
    return search.accepted;
}
int32_t driverLaneAvoidNeighbor(DriverAIGoal* goal,DriverCommand* command,int32_t* followTarget,
    const DriverAINeighbor* neighbor,const DriverAIRoadState* road,DriverAIPerception* perception,
    DriverAIBase* driver,int32_t tryCurrent,float horizon)
{
    const int32_t side=driverLanePreferredPassingSide(neighbor,perception,road);
    DriverCommand original;
    driverCommandCopy(&original,command);
    DriverAIGoal current;
    driverAIGoalInitialize(&current);
    if(goal->kind)copyGoal(&current,goal);
    const float threshold=driver->environment->tuning->stationaryPassSpeed;
    const int32_t moving=threshold<neighbor->movement->bindings.state->speed||
        threshold<perception->movement->bindings.state->speed;
    float speed=-1.0f,time=99999.0f;
    CarMovement* collision=NULL;
    int32_t accepted=0;
    if(tryCurrent&&(!driverAIGoalUsesRoad(current.kind)||moving)&&
       driverLaneGoalAllowed(&current,road,perception,driver->physics.routeVehicleClass)&&
       (!driverAIGoalUsesTarget(current.kind)||current.target==neighbor->movement)){
        int32_t follow=0;
        if(current.kind==5)follow=side;
        if(current.kind==6)follow=side^1;
        driverCommandCopy(command,&original);
        if(driverLaneEvaluateGoal(&current,command,road,perception,driver,0,follow,horizon,&speed,&collision,&time)){
            accepted=1;*followTarget=follow;
        }
    }
    DriverAIGoal candidate;
    driverAIGoalInitialize(&candidate);
    const int32_t count=driver->environment->tuning->laneCandidateCount;
    for(int32_t selector=1;!accepted&&moving&&selector<=count;++selector){
        driverAIGoalSet(&candidate,selector,NULL,0);
        if(tryCurrent&&driverLaneGoalsMatch(&candidate,&current))continue;
        driverCommandCopy(command,&original);
        if(driverLaneEvaluateGoal(&candidate,command,road,perception,driver,0,0,horizon,&speed,&collision,&time)){
            copyGoal(goal,&candidate);accepted=1;*followTarget=0;
        }
    }
    if(!accepted){
        DriverAIGoal alternatives[2];
        for(int index=0;index<2;++index){
            driverAIGoalInitialize(&alternatives[index]);
            driverAIGoalSet(&alternatives[index],index==0?(side?5:6):(side?6:5),neighbor->movement,0);
        }
        for(int index=0;index<2&&!accepted;++index){
            const DriverAIGoal* next=&alternatives[index];
            if(tryCurrent&&driverLaneGoalsMatch(next,&current))continue;
            if(!moving&&!driverLaneTargetHasRoom(next,neighbor,perception))continue;
            driverCommandCopy(command,&original);
            const int32_t follow=index==0;
            if(driverLaneEvaluateGoal(next,command,road,perception,driver,0,follow,horizon,&speed,&collision,&time)){
                copyGoal(goal,next);accepted=1;*followTarget=follow;
            }
        }
    }
    if(!accepted){
        DriverAIGoal fallback;
        driverAIGoalInitialize(&fallback);
        if(driverLaneGoalAllowed(&current,road,perception,driver->physics.routeVehicleClass))copyGoal(&fallback,&current);
        else driverAIGoalSet(&fallback,driverLaneSelectorForIndex(road->lane),NULL,0);
        driverLanePlanGoal(&fallback,command,road,perception,driver,1);
        copyGoal(goal,&fallback);
    }
    const CarDriverResult published=driverLanePublishTarget(goal,driver,*followTarget,perception);
    if(published!=CAR_DRIVER_OK)driver->result=published;
    return accepted;
}
void driverAIGoalInitialize(DriverAIGoal* goal)
{
    goal->kind=0;goal->target=NULL;goal->parameter=0;
}
