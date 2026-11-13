#include "TrackedController.hpp"
#include <string.h>
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/QuickSpline.hpp"
#include "bware/inc/bMath.hpp"
#include <math.h>
#include "indep/inc/Player.hpp"

CarDriverResult realControllerInitialize(RealController* controller,CarMovement* movement,
                                          const DriverPhysicsVehicle* physics,const RealDriverBindings* input)
{
    if(!controller||!movement||!physics||physics->movement!=movement||!input)
        return CAR_DRIVER_INVALID_BINDING;
    memset(controller,0,sizeof(*controller));
    CarDriverResult result=driverRouteInitialize(&controller->base,movement,CAR_DRIVER_REAL);
    if(result!=CAR_DRIVER_OK)return result;
    controller->physics=*physics;
    result=realDriverInitialize(&controller->input,movement,input);
    if(result!=CAR_DRIVER_OK)return controller->base.result=result;
    if(!driverHistoryInitialize(&controller->lateralVelocity,8))
        return controller->base.result=CAR_DRIVER_ALLOCATION_FAILURE;
    quickSplineInitializeBorrowed(&controller->path,QUICK_SPLINE_EXTRAPOLATE,0,controller->pathStorage,7);
    return CAR_DRIVER_OK;
}

void realControllerDispose(RealController* controller)
{
    realDriverDispose(&controller->input);
    driverHistoryDispose(&controller->lateralVelocity);
    quickSplineDestroy(&controller->path,0);
}

static int32_t laneIndex(const RealController* controller)
{
    return controller->physics.routeVehicleClass*2;
}

static void lanePoint(const CarRecoveryPose* pose,int32_t lane,float output[4])
{
    const float lateral=(float)((int32_t)pose->laneOffsetsQuantized[lane]*256)*0.0000152587890625f;
    driverRoadPointPosition(pose,output,0.0f,lateral);
}

void realControllerBuildRoadCommand(RealController* controller,DriverCommand* command,
                                     const CarRecoverySelection* road,float lateralVelocity,float lateralDistance)
{
    const CarMovement* movement=controller->base.movement;
    const CarRecoveryPointRegistry* registry=movement->bindings.recoveryPoints;
    const float maximumDistance=driverPlanningDistance(&controller->physics);
    const int32_t lane=laneIndex(controller);
    const float speed=movement->bindings.state->speed;
    float minimum[4],maximum[4];
    driverCollisionBounds(&controller->physics,minimum,maximum);
    const float halfWidth=(maximum[1]-minimum[1])*0.5f;
    float distance=0.0f;
    CarRecoverySelection next=*road;
    while(distance<maximumDistance&&command->count<DRIVER_COMMAND_CAPACITY){
        const CarRecoverySelection previous=next;
        const CarDriverResult result=driverRoadAdvanceSelection(registry,&next);
        if(result!=CAR_DRIVER_OK){controller->base.result=result;return;}
        if(!next.group||!next.pose){
            driverCommandAdvance(command,NULL,movement->bindings.environment->clock->worldTimer,0.0f);
            driverCommandBuildStraight(command,&controller->physics);
            return;
        }
        const float x=next.pose->position[0]-previous.pose->position[0];
        const float y=next.pose->position[1]-previous.pose->position[1];
        const float first=x*previous.pose->direction[0];
        const float second=y*previous.pose->direction[1];
        const float duration=finiteRatio(first+second,speed);
        lateralDistance=lateralDistance+lateralVelocity*duration;
        lateralVelocity=lateralVelocity<0.0f?lateralVelocity+duration:lateralVelocity-duration;
        const float lower=halfWidth-next.pose->halfWidths[0];
        const float upper=next.pose->halfWidths[1]-halfWidth;
        const float authored=(float)((int32_t)next.pose->laneOffsetsQuantized[lane]*256)*0.0000152587890625f;
        float lateral=authored+lateralDistance;
        if(lateral<lower)lateral=lower;
        if(upper<lateral)lateral=upper;
        float point[4];driverRoadPointPosition(next.pose,point,0.0f,lateral);
        driverCommandAppend(command,point,&next,lane,0,NULL);
        distance=distance+previous.pose->segmentLength;
    }
}

int32_t realControllerMeasureLateralMotion(RealController* controller,float* distance,float* velocity,
                                           const CarRecoverySelection* road,float elapsed)
{
    const int32_t lane=laneIndex(controller);
    float points[4][4];
    int32_t count=0;
    int32_t index=0;
    while(index<controller->historyCount){
        if(driverRoadSelectionValid(&controller->history[index]))
            lanePoint(controller->history[index].pose,lane,points[count++]);
        ++index;
    }
    const CarRecoveryPointRegistry* registry=controller->base.movement->bindings.recoveryPoints;
    CarRecoverySelection next;
    if(controller->historyCount>0&&driverRoadSelectionValid(&controller->history[controller->historyCount-1])){
        next=controller->history[controller->historyCount-1];
        const CarDriverResult result=driverRoadAdvanceSelection(registry,&next);
        if(result!=CAR_DRIVER_OK){controller->base.result=result;return 0;}
    }else next=*road;
    index=count;
    while(index<4&&driverRoadSelectionValid(&next)){
        lanePoint(next.pose,lane,points[count++]);
        ++index;
        const CarDriverResult result=driverRoadAdvanceSelection(registry,&next);
        if(result!=CAR_DRIVER_OK){controller->base.result=result;return 0;}
    }
    if(count<controller->path.paddingCount){
        *distance=0.0f;
        *velocity=controller->lateralVelocity.value;
        return 0;
    }
    quickSplineSetPointCount(&controller->path,(uint16_t)count);
    for(int32_t point=0;point<count;++point){
        memcpy(controller->path.points[point],points[point],sizeof(points[point]));
        controller->path.points[point][3]=0.0f;
        controller->path.pointsDirty=1;
    }
    const VehicleState* vehicle=controller->base.movement->bindings.state;
    const float parameter=quickSplineFindClosestParameter(&controller->path,vehicle->position,0.0f,0.1f);
    float point[4];quickSplineEvaluate(&controller->path,point,parameter,0);
    const float x=point[0]-vehicle->position[0];
    const float y=point[1]-vehicle->position[1];
    const float xx=x*x;
    const float yy=y*y;
    *distance=sqrtf(xx+yy);
    const float first=vehicle->orientation[4]*x;
    const float second=vehicle->orientation[5]*y;
    if(first+second<0.0f)*distance=-*distance;
    driverHistoryPush(&controller->lateralVelocity,finiteRatio(*distance-controller->previousLateral,elapsed));
    *velocity=controller->lateralVelocity.value;
    controller->previousLateral=*distance;
    if(controller->historyCount>0&&controller->history[controller->historyCount-1].pose==road->pose)return 1;
    if(!road->group)return 1;
    if(controller->historyCount==4){
        for(int32_t prior=0;prior<controller->historyCount-1;++prior)
            controller->history[prior]=controller->history[prior+1];
    }else ++controller->historyCount;
    controller->history[controller->historyCount-1]=*road;
    return 1;
}

void realControllerThink(RealController* controller,float elapsed)
{
    DriverCommand command;
    driverCommandInitialize(&command);
    driverRouteAdvance(&controller->base,elapsed);
    if(controller->base.result!=CAR_DRIVER_OK)return;
    const CarRecoveryGroup* group=driverRouteCurrentGroup(&controller->base);
    const CarRecoveryPose* pose=controller->base.recovery.recoveryPose;
    const CarRecoverySelection current={group,pose};
    elapsed=maximumFinite(elapsed,0.0010000000474974513f);
    int32_t planned=0;
    if(!controller->input.player->wrongWay){
        CarRecoverySelection following=current;
        const CarDriverResult result=driverRoadAdvanceSelection(
            controller->base.movement->bindings.recoveryPoints,&following);
        if(result!=CAR_DRIVER_OK){controller->base.result=result;return;}
        float distance=0.0f,velocity=0.0f;
        if(group&&pose&&realControllerMeasureLateralMotion(controller,&distance,&velocity,&current,elapsed)){
            if(10.0f<=controller->base.movement->bindings.state->forwardSpeed){
                realControllerBuildRoadCommand(controller,&command,&current,velocity,distance);
                driverCommandBlendPredictedPath(&command,&controller->physics);
            }else driverCommandBuildStraight(&command,&controller->physics);
            planned=1;
        }
    }
    if(!planned){
        if(10.0f<=controller->base.movement->bindings.state->forwardSpeed)
            driverCommandBuildVelocityPath(&command,&controller->physics);
        else driverCommandBuildStraight(&command,&controller->physics);
    }
    if(controller->base.result!=CAR_DRIVER_OK)return;
    driverCommandCopy(&controller->input.command,&command);
    /* The original optional road-editor display and text diagnostics have no
     * gameplay state. The owned command above is the live control input. */
}

#include "indep/inc/Replay.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "bware/inc/bWare.hpp"

// Full PAL23A560..23A634. The scratch spline is cleared only during an
// actual record/restore, exactly as in the source checksum gate.
void realControllerSnapshot(RealController* controller,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"RealController");
    if(snapshot->recording||snapshot->restoring)quickSplineSetPointCount(&controller->path,0);
    replaySnapshotMoveFloat(snapshot,&controller->previousLateral);
    driverHistorySnapshot(&controller->lateralVelocity,snapshot);
    replaySnapshotMoveInteger(snapshot,&controller->historyCount,4);
    hp2::Require(controller->historyCount>=0&&controller->historyCount<=4,"RealController snapshot history count");
    for(int index=0;index<controller->historyCount;++index)
        carRecoverySelectionSnapshot(controller->base.movement->bindings.recoveryPoints,&controller->history[index],snapshot);
    driverRouteSnapshot(&controller->base,snapshot);
    replaySnapshotEndSection(snapshot);
}
