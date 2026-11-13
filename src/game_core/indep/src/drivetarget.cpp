#include "indep/inc/DriveTarget.hpp"
#include "indep/inc/Replay.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/AICarController.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/DriveTarget.hpp"

#include <math.h>
#include <string.h>
#include <cmath>

// Complete PAL21BA50..21BBAC. The source stores route/point indices in
// parallel narrow arrays; native commands borrow exactly those road owners.
void driverCommandSnapshot(DriverCommand* command,const CarRecoveryPointRegistry* registry,
    ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"DriveTarget");
    replaySnapshotMoveUnsigned(snapshot,&command->roadFlags,1);
    replaySnapshotMoveInteger(snapshot,&command->count,1);
    replaySnapshotMoveInteger(snapshot,&command->closestPoint,1);
    replaySnapshotMoveInteger(snapshot,&command->closestTicks,4);
    replaySnapshotMoveInteger(snapshot,&command->adjustmentTicks,4);
    replaySnapshotMoveFloat(snapshot,&command->length);
    replaySnapshotMoveInteger(snapshot,&command->lastSpeedPoint,1);
    hp2::Require(command->count>=0 && command->count<=DRIVER_COMMAND_CAPACITY,
        "Replay DriveTarget count exceeds its actual source capacity");
    for(int32_t i=0;i<command->count;++i) {
        auto& point=command->points[i];
        int16_t poseIndex=-1;
        int8_t groupId=-1;
        if(!snapshot->restoring && point.road.group) {
            groupId=static_cast<int8_t>(point.road.group->id);
            if(point.road.pose) {
                int32_t found=-1;
                for(int32_t index=0;index<point.road.group->poseCount;++index)
                    if(&point.road.group->poses[index]==point.road.pose){found=index;break;}
                hp2::Require(found>=0 && found<=32767,"Replay DriveTarget pose is not in its retained route");
                poseIndex=static_cast<int16_t>(found);
            }
        }
        replaySnapshotMoveData(snapshot,point.position,12,-1,0);
        replaySnapshotMoveData(snapshot,&poseIndex,2,2,1);
        replaySnapshotMoveFloat(snapshot,&point.targetSpeed);
        replaySnapshotMoveData(snapshot,&groupId,1,1,1);
        replaySnapshotMoveInteger(snapshot,&point.laneIndex,1);
        if(snapshot->restoring) {
            point.road={nullptr,nullptr};
            if(groupId>=0) {
                for(size_t index=0;index<registry->groupCount;++index)
                    if(registry->groups[index].id==groupId){point.road.group=&registry->groups[index];break;}
                hp2::Require(point.road.group,"Replay DriveTarget route id is unavailable");
                if(poseIndex>=0) {
                    hp2::Require(poseIndex<point.road.group->poseCount,"Replay DriveTarget pose index is out of bounds");
                    point.road.pose=&point.road.group->poses[poseIndex];
                }
            }
        }
    }
    replaySnapshotEndSection(snapshot);
}

DriverCommand* driverCommandCopy(DriverCommand* output,const DriverCommand* input)
{
    *output=*input;
    return output;
}

int32_t driverCommandLastSegment(const DriverCommand* command)
{
    return command->lastSpeedPoint==-1?command->count-2:command->lastSpeedPoint-1;
}

void driverCommandAppend(DriverCommand* command,const float point[4],
                          const CarRecoverySelection* road,int32_t laneIndex,
                          int32_t rejectBacktracking,const VehicleState* vehicle)
{
    if(command->count>=DRIVER_COMMAND_CAPACITY)return;
    if(rejectBacktracking){
        if(command->count>=2){
            float previous[4],next[4];
            for(int axis=0;axis<4;++axis){
                previous[axis]=command->points[command->count-1].position[axis]-
                    command->points[command->count-2].position[axis];
                next[axis]=point[axis]-command->points[command->count-1].position[axis];
            }
            if(driverCommandDotThree(previous,next)<0.0f)return;
        }else if(vehicle&&!driverCommandPointAhead(point,vehicle,0.0f))return;
    }
    DriverCommandPoint* added=&command->points[command->count];
    memcpy(added->position,point,sizeof(added->position));
    added->road=road?*road:CarRecoverySelection{NULL,NULL};
    added->laneIndex=laneIndex;
    command->closestTicks=0;
    command->adjustmentTicks=0;
    added->targetSpeed=0.0f;
    if(command->count>0)
        command->length=command->length+driverCommandDistanceThree(point,command->points[command->count-1].position);
    ++command->count;
}

void driverCommandRecalculateLength(DriverCommand* command)
{
    command->length=0.0f;
    for(int32_t index=1;index<command->count;++index)
        command->length=command->length+driverCommandDistanceThree(command->points[index-1].position,
                                                      command->points[index].position);
}

void driverCommandSetPoint(DriverCommand* command,int32_t index,const float point[4],
                            const CarRecoverySelection* road,int32_t laneIndex,float targetSpeed)
{
    if(index<0||command->count<=index)return;
    DriverCommandPoint* changed=&command->points[index];
    memcpy(changed->position,point,sizeof(changed->position));
    changed->road=road?*road:CarRecoverySelection{NULL,NULL};
    changed->laneIndex=laneIndex;
    changed->targetSpeed=targetSpeed;
    command->closestTicks=0;
    driverCommandRecalculateLength(command);
}

void driverCommandInsertFront(DriverCommand* command,int32_t count)
{
    if(count>=DRIVER_COMMAND_CAPACITY)return;
    for(int32_t index=command->count-count-1;index>=0;--index)
        command->points[index+count]=command->points[index];
    command->count+=count;
    if(command->count>DRIVER_COMMAND_CAPACITY)command->count=DRIVER_COMMAND_CAPACITY;
    for(int32_t index=0;index<count;++index){
        command->points[index].position[0]=0.0f;
        command->points[index].position[1]=0.0f;
        command->points[index].position[2]=0.0f;
        command->points[index].road=CarRecoverySelection{NULL,NULL};
        command->points[index].targetSpeed=0.0f;
    }
    command->adjustmentTicks=0;
    command->closestTicks=0;
}

void driverCommandTrimFront(DriverCommand* command,int32_t count,int32_t updateLength)
{
    if(count>=command->count)return;
    for(int32_t index=0;index<command->count-count;++index)
        command->points[index]=command->points[index+count];
    command->count-=count;
    command->closestTicks=0;
    command->adjustmentTicks=0;
    if(updateLength)driverCommandRecalculateLength(command);
}

float driverCommandLateralAccelerationLimit(const DriverCommand* command,const DriverPhysicsVehicle* vehicle,float speed)
{
    (void)command;(void)vehicle;(void)speed;
    return 30.0f;
}

void driverCommandResample(DriverCommand* command)
{
    if(command->count<3)return;
    hp2::Require(command->count<=DRIVER_COMMAND_CAPACITY,"DriveTarget resample exceeds point capacity");
    float differences[DRIVER_COMMAND_CAPACITY][4];
    float lengths[DRIVER_COMMAND_CAPACITY];
    float points[DRIVER_COMMAND_CAPACITY][4];
    float totalLength=0.0f;
    bool hasRepeatedPoint=false;
    for(int32_t index=0;index<command->count;++index)
        for(float coordinate:command->points[index].position)
            hp2::Require(std::isfinite(coordinate),"DriveTarget resample has a non-finite point");
    for(int32_t index=0;index<command->count-1;++index){
        for(int axis=0;axis<4;++axis){
            differences[index][axis]=bScalarAdd(command->points[index+1].position[axis],-command->points[index].position[axis]);
            hp2::Require(std::isfinite(differences[index][axis]),"DriveTarget resample delta exceeds finite range");
        }
        const float xx=bScalarMultiply(differences[index][0],differences[index][0]);
        const float yy=bScalarMultiply(differences[index][1],differences[index][1]);
        const float zz=bScalarMultiply(differences[index][2],differences[index][2]);
        lengths[index]=bScalarSquareRoot(bScalarAdd(bScalarAdd(xx,yy),zz));
        hp2::Require(std::isfinite(lengths[index]),"DriveTarget resample segment exceeds finite range");
        hasRepeatedPoint=hasRepeatedPoint || lengths[index]==0.0f;
        totalLength=bScalarAdd(totalLength,lengths[index]);
    }
    hp2::Require(std::isfinite(totalLength),"DriveTarget resample length exceeds finite range");
    // Dynamic commands may repeat points even when authored roads do not.
    // Zero-length segments carry no arc distance; skip them. A stationary
    // command retains its points instead of evaluating a saturated 0/0.
    if(hasRepeatedPoint){
        if(totalLength>0.0f){
            int32_t segment=0;
            float traversed=0.0f;
            for(int32_t output=0;output<command->count-1;++output){
                const float distance=(static_cast<float>(output)/static_cast<float>(command->count-1))*totalLength;
                while(segment<command->count-2 &&
                    (lengths[segment]==0.0f || traversed+lengths[segment]<distance)){
                    traversed+=lengths[segment++];
                }
                const float fraction=lengths[segment]>0.0f ? (distance-traversed)/lengths[segment] : 0.0f;
                for(int axis=0;axis<4;++axis)
                    points[output][axis]=command->points[segment].position[axis]+differences[segment][axis]*fraction;
            }
            memcpy(points[command->count-1],command->points[command->count-1].position,sizeof(points[0]));
            for(int32_t index=0;index<command->count;++index)
                memcpy(command->points[index].position,points[index],sizeof(points[index]));
        }
        command->length=totalLength;command->adjustmentTicks=0;command->closestTicks=0;
        return;
    }
    command->length=totalLength;
    memcpy(differences[command->count-1],differences[command->count-2],sizeof(differences[0]));
    lengths[command->count-1]=lengths[command->count-2];
    const float step=totalLength/static_cast<float>(command->count-1);
    /* The original copies every output slot even when rounding or a zero
     * length path leaves a slot unwritten on its stack. Retain the existing
     * point in that case, giving native storage a defined lifetime. */
    for(int32_t index=0;index<command->count;++index)
        memcpy(points[index],command->points[index].position,sizeof(points[index]));
    int32_t segment=0;
    int32_t output=0;
    float residual=0.0f;
    float distance=0.0f;
    float previousDistance=0.0f;
    // PAL21C75C/21C77C accumulate the finite step. Host rounding can terminate
    // this loop one sample early, shortening both the command and its belt.
    while(segment<command->count&&distance<=totalLength&&output<command->count){
        previousDistance=distance;
        distance=bScalarAdd(previousDistance,step);
        const float fraction=residual/lengths[segment];
        residual=bScalarAdd(residual,step);
        for(int axis=0;axis<4;++axis){
            const float scaled=bScalarMultiply(differences[segment][axis],fraction);
            points[output][axis]=bScalarAdd(scaled,command->points[segment].position[axis]);
        }
        ++output;
        if(!(distance<=totalLength))break;
        while(segment<command->count&&lengths[segment]<=residual){
            residual=bScalarAdd(residual,-lengths[segment]);
            ++segment;
        }
    }
    command->length=previousDistance;
    for(int32_t index=0;index<command->count;++index)
        memcpy(command->points[index].position,points[index],sizeof(points[index]));
    command->adjustmentTicks=0;
    command->closestTicks=0;
}

float driverCommandDistanceThree(const float first[4],const float second[4])
{
    const float x=first[0]-second[0];
    const float y=first[1]-second[1];
    const float z=first[2]-second[2];
    const float xx=x*x;
    const float yy=y*y;
    const float zz=z*z;
    return sqrtf((xx+yy)+zz);
}

float driverCommandDotThree(const float first[4],const float second[4])
{
    const float x=first[0]*second[0];
    const float y=first[1]*second[1];
    const float z=first[2]*second[2];
    return (x+y)+z;
}

float driverCommandDistanceTwo(const float first[2],const float second[2])
{
    const float x=first[0]-second[0];
    const float y=first[1]-second[1];
    const float xx=x*x;
    const float yy=y*y;
    return sqrtf(xx+yy);
}


DriverCommand* driverCommandInitialize(DriverCommand* command)
{
    memset(command,0,sizeof(*command));
    command->lastSpeedPoint=-1;
    return command;
}

uint16_t driverCommandHeading(const DriverCommand* command,int32_t first)
{
    if(first<0)first=0;
    if(command->count-2<first)first=command->count-2;
    const float x=command->points[first+1].position[0]-command->points[first].position[0];
    const float y=command->points[first+1].position[1]-command->points[first].position[1];
    return vectorAtan2Angle(x,y);
}

int32_t driverCommandRoadAt(const DriverCommand* command,int32_t index,CarRecoverySelection* road)
{
    *road=CarRecoverySelection{NULL,NULL};
    if(index<0||command->count<=index)return 0;
    const CarRecoverySelection* authored=&command->points[index].road;
    if(!authored->group||!authored->pose)return 0;
    *road=*authored;
    return 1;
}

int32_t driverCommandClosestPoint(DriverCommand* command,const VehicleState* vehicle,int32_t nowTicks)
{
    if(nowTicks==command->closestTicks)return command->closestPoint;
    if(command->count<=0){command->closestPoint=0;command->closestTicks=0;return 0;}
    const float threshold=vehicle->forwardSpeed>30.0f?0.0f:0.34200000762939453f;
    float velocity[2];
    vectorNormalize2(velocity,vehicle->velocity);
    int32_t selected=command->count-1;
    for(int32_t index=0;index<command->count;++index){
        const float delta[2]={command->points[index].position[0]-vehicle->position[0],
                              command->points[index].position[1]-vehicle->position[1]};
        float direction[2];vectorNormalize2(direction,delta);
        const float first=direction[0]*velocity[0];
        const float second=direction[1]*velocity[1];
        if(threshold<first+second){
            const float xx=delta[0]*delta[0];
            const float yy=delta[1]*delta[1];
            const float minimumSquaredDistance=16.0f;
            if(minimumSquaredDistance<xx+yy){selected=index;break;}
        }
    }
    command->closestPoint=selected;
    command->closestTicks=nowTicks;
    return selected;
}

int32_t driverCommandPointAtDistance(DriverCommand* command,const VehicleState* vehicle,
                                     int32_t nowTicks,float distance,float output[4])
{
    if(command->count==0)return -1;
    if(command->count==1){
        float delta[4];
        for(int axis=0;axis<4;++axis)delta[axis]=command->points[0].position[axis]-vehicle->position[axis];
        vectorSetLength3(delta,delta,distance);
        for(int axis=0;axis<4;++axis)output[axis]=vehicle->position[axis]+delta[axis];
        return 0;
    }
    int32_t index=driverCommandClosestPoint(command,vehicle,nowTicks);
    const int32_t first=index;
    float cumulative[DRIVER_COMMAND_CAPACITY];
    cumulative[index]=driverCommandDistanceThree(command->points[index].position,vehicle->position);
    if(cumulative[index]<distance){
        ++index;
        while(index<command->count){
            cumulative[index]=driverCommandDistanceThree(command->points[index-1].position,command->points[index].position)+cumulative[index-1];
            if(distance<=cumulative[index])break;
            ++index;
        }
    }
    const int32_t last=command->count-1;
    if(index>last){
        float delta[4];
        for(int axis=0;axis<4;++axis)delta[axis]=command->points[last].position[axis]-command->points[last-1].position[axis];
        vectorSetLength3(delta,delta,distance-cumulative[last]);
        for(int axis=0;axis<4;++axis)output[axis]=command->points[last].position[axis]+delta[axis];
        return last;
    }
    const float* origin;
    float previous;
    int32_t segment;
    if(first<index){segment=index-1;previous=cumulative[segment];origin=command->points[segment].position;}
    else{segment=-1;previous=0.0f;origin=vehicle->position;}
    const float fraction=finiteRatio(distance-previous,cumulative[index]-previous);
    for(int axis=0;axis<4;++axis){
        const float delta=command->points[index].position[axis]-origin[axis];
        const float scaled=delta*fraction;
        output[axis]=scaled+origin[axis];
    }
    return segment;
}

float driverCommandSegmentTime(float distance,float firstSpeed,float secondSpeed)
{
    if(fabsf(firstSpeed)<=0.0010000000474974513f&&fabsf(secondSpeed)<=0.0010000000474974513f)
        return 9999999.0f;
    const float mean=(firstSpeed+secondSpeed)*0.5f;
    return finiteRatio(distance,mean);
}

void driverCommandAdvance(DriverCommand* command,const VehicleState* vehicle,
                           int32_t nowTicks,float distance)
{
    if(command->count==0)command->length=0.0f;
    else if(fabsf(distance)<=0.0010000000474974513f||distance<0.0f){
        command->count=0;
        command->length=0.0f;
    }else{
        int32_t first=0;
        while(first<command->count){
            const float x=command->points[first].position[0]-vehicle->position[0];
            const float y=command->points[first].position[1]-vehicle->position[1];
            const float firstProjection=vehicle->velocity[0]*x;
            const float secondProjection=vehicle->velocity[1]*y;
            if(0.0f<firstProjection+secondProjection)break;
            ++first;
        }
        if(first>=command->count){command->count=0;command->length=0.0f;}
        else{
            if(first>0)driverCommandTrimFront(command,first,0);
            float point[4];
            const int32_t segment=driverCommandPointAtDistance(command,vehicle,nowTicks,distance,point);
            if(segment<command->count-1){
                int32_t previous;
                if(segment>=0){previous=segment;command->count=segment+1;}
                else{previous=driverCommandClosestPoint(command,vehicle,nowTicks);command->count=0;}
                const CarRecoverySelection road=command->points[previous].road;
                const int32_t lane=command->points[previous].laneIndex;
                driverCommandAppend(command,point,&road,lane,0,NULL);
                driverCommandRecalculateLength(command);
            }
        }
    }
    command->adjustmentTicks=0;
    command->lastSpeedPoint=-1;
    command->closestTicks=0;
}



void driverCommandBuildVelocityPath(DriverCommand* command,const DriverPhysicsVehicle* context)
{
    const CarMovement* movement=context->movement;
    const VehicleState* vehicle=movement->bindings.state;
    driverCommandAdvance(command,NULL,movement->bindings.environment->clock->worldTimer,0.0f);
    const float maximumDistance=driverPlanningDistance(context);
    float position[2]={vehicle->position[0],vehicle->position[1]};
    float velocity[2]={vehicle->velocity[0],vehicle->velocity[1]};
    const float* acceleration=vehicle->linearAcceleration;
    const float aa=acceleration[0]*acceleration[0];
    const float ab=acceleration[1]*acceleration[1];
    const float accelerationLength=sqrtf(aa+ab);
    const float vv=velocity[0]*velocity[0];
    const float vw=velocity[1]*velocity[1];
    if(sqrtf(vv+vw)*0.75f<accelerationLength){driverCommandBuildStraight(command,context);return;}
    float distance=0.0f;
    while(distance<maximumDistance&&command->count<DRIVER_COMMAND_CAPACITY){
        float nextVelocity[2],nextPosition[4]={0.0f,0.0f,0.0f,0.0f};
        vectorMultiplyAdd2(nextVelocity,velocity,acceleration,0.1f);
        vectorMultiplyAdd2(nextPosition,position,nextVelocity,0.1f);
        driverCommandAppend(command,nextPosition,NULL,-1,1,NULL);
        const float x=position[0]-nextPosition[0];
        const float y=position[1]-nextPosition[1];
        const float xx=x*x;
        const float yy=y*y;
        distance=distance+sqrtf(xx+yy);
        const float first=nextVelocity[0]*vehicle->velocity[0];
        const float second=nextVelocity[1]*vehicle->velocity[1];
        position[0]=nextPosition[0];position[1]=nextPosition[1];
        velocity[0]=nextVelocity[0];velocity[1]=nextVelocity[1];
        if(first+second<0.0f)break;
    }
}

void driverCommandBlendPredictedPath(DriverCommand* command,const DriverPhysicsVehicle* context)
{
    const CarMovement* movement=context->movement;
    const int32_t nowTicks=movement->bindings.environment->clock->worldTimer;
    if(command->adjustmentTicks==nowTicks||movement->bindings.player)return;
    command->adjustmentTicks=nowTicks;
    const VehicleState* vehicle=movement->bindings.state;
    float position[2]={vehicle->position[0],vehicle->position[1]};
    float velocity[2]={vehicle->velocity[0],vehicle->velocity[1]};
    const float* acceleration=vehicle->linearAcceleration;
    const float speed=vehicle->speed;
    float totalTime=0.0f;
    int32_t index=driverCommandClosestPoint(command,vehicle,nowTicks);
    if(speed<1.0f||index>=command->count)return;
    do{
        const float* previous=index==command->closestPoint?vehicle->position:command->points[index-1].position;
        const float x=previous[0]-command->points[index].position[0];
        const float y=previous[1]-command->points[index].position[1];
        const float xx=x*x;
        const float yy=y*y;
        const float duration=finiteRatio(sqrtf(xx+yy),speed);
        totalTime=totalTime+duration;
        float nextVelocity[2],predicted[2];
        vectorMultiplyAdd2(nextVelocity,velocity,acceleration,duration);
        vectorMultiplyAdd2(predicted,position,nextVelocity,duration);
        float weight=totalTime*1.3333333730697632f;
        if(1.0f<weight)weight=1.0f;
        const float predictedWeight=1.0f-weight;
        const float originalX=command->points[index].position[0]*weight;
        const float originalY=command->points[index].position[1]*weight;
        const float predictedX=predicted[0]*predictedWeight;
        const float predictedY=predicted[1]*predictedWeight;
        const float changed[4]={originalX+predictedX,originalY+predictedY,0.0f,0.0f};
        const CarRecoverySelection road=command->points[index].road;
        const int32_t lane=command->points[index].laneIndex;
        velocity[0]=nextVelocity[0];velocity[1]=nextVelocity[1];
        position[0]=predicted[0];position[1]=predicted[1];
        driverCommandSetPoint(command,index,changed,&road,lane,0.0f);
        ++index;
    }while(totalTime<0.75f&&index<command->count);
}

int32_t driverCommandCheckLateralAcceleration(const DriverCommand* command,const DriverPhysicsVehicle* vehicle,
    const float initialVelocity[2],const float finalVelocity[2],float* finalSpeed,float distance,float initialSpeed)
{
    const float x=finalVelocity[0]-initialVelocity[0];
    const float y=finalVelocity[1]-initialVelocity[1];
    const float time=driverCommandSegmentTime(distance,initialSpeed,*finalSpeed);
    if(fabsf(time)<=0.0010000000474974513f){*finalSpeed=initialSpeed;return 1;}
    const float inverseTime=finiteRatio(1.0f,time);
    const float changeX=x*inverseTime;
    const float changeY=y*inverseTime;
    const float xx=initialVelocity[0]*initialVelocity[0];
    const float yy=initialVelocity[1]*initialVelocity[1];
    const float dotX=initialVelocity[0]*changeX;
    const float dotY=initialVelocity[1]*changeY;
    const float along=finiteRatio(dotX+dotY,xx+yy);
    const float alongX=initialVelocity[0]*along;
    const float alongY=initialVelocity[1]*along;
    const float lateralX=changeX-alongX;
    const float lateralY=changeY-alongY;
    const float lateralXX=lateralX*lateralX;
    const float lateralYY=lateralY*lateralY;
    const float lateral=sqrtf(lateralXX+lateralYY);
    const float alongXX=alongX*alongX;
    const float alongYY=alongY*alongY;
    const float limit=driverCommandLateralAccelerationLimit(command,vehicle,sqrtf(alongXX+alongYY));
    return lateral<limit||fabsf(lateral-limit)<=0.0010000000474974513f;
}

int32_t driverCommandCheckStartingTurn(const DriverCommand* command,const DriverPhysicsVehicle* context,
    const float point[2],float* targetSpeed,float lookaheadSeconds)
{
    const VehicleState* vehicle=context->movement->bindings.state;
    const float distance=lookaheadSeconds*fabsf(vehicle->forwardSpeed);
    float direction[2]={point[0]-vehicle->position[0],point[1]-vehicle->position[1]};
    vectorSetLength2(direction,direction,*targetSpeed);
    return driverCommandCheckLateralAcceleration(command,context,vehicle->velocity,direction,targetSpeed,distance,vehicle->speed);
}

int32_t driverCommandCheckTurn(const DriverCommand* command,const DriverPhysicsVehicle* context,
    const float first[2],const float current[2],const float next[2],float* targetSpeed,float distance,float initialSpeed)
{
    if(fabsf(initialSpeed)<=0.0010000000474974513f)return 1;
    if(fabsf(distance)<=0.0010000000474974513f){*targetSpeed=initialSpeed;return 1;}
    const float firstDelta[2]={current[0]-first[0],current[1]-first[1]};
    float initialVelocity[2];vectorSetLength2(initialVelocity,firstDelta,initialSpeed);
    float finalVelocity[2]={next[0]-current[0],next[1]-current[1]};
    vectorSetLength2(finalVelocity,finalVelocity,*targetSpeed);
    return driverCommandCheckLateralAcceleration(command,context,initialVelocity,finalVelocity,targetSpeed,distance,initialSpeed);
}

float driverCommandPlanSpeed(DriverCommand* command,const DriverPhysicsVehicle* context)
{
    if(command->count==0)return -1.0f;
    const CarMovement* movement=context->movement;
    const VehicleState* vehicle=movement->bindings.state;
    float distances[DRIVER_COMMAND_CAPACITY];
    int32_t accepted=1;
    int32_t finished=command->count==1;
    if(finished){
        command->points[0].targetSpeed=vehicle->speed;
        distances[0]=driverCommandDistanceTwo(vehicle->position,command->points[0].position);
    }
    const int32_t first=driverCommandClosestPoint(command,vehicle,movement->bindings.environment->clock->worldTimer);
    if(first>=command->count)return -1.0f;
    if(!finished){
        for(int32_t index=first;index<command->count;++index){
            command->points[index].targetSpeed=vehicle->speed;
            const float* previous=index==first?vehicle->position:command->points[index-1].position;
            distances[index]=driverCommandDistanceTwo(command->points[index].position,previous);
        }
        int32_t index=first;
        float time=driverCommandSegmentTime(distances[first],vehicle->speed,command->points[first].targetSpeed);
        if(time<0.4000000059604645f){
            while(index<command->count){
                ++index;
                if(index>=command->count)break;
                const float distance=driverCommandDistanceTwo(vehicle->position,command->points[index].position);
                time=driverCommandSegmentTime(distance,vehicle->speed,command->points[index].targetSpeed);
                if(!(time<0.4000000059604645f))break;
            }
        }
        if(index+1<command->count)
            accepted=driverCommandCheckStartingTurn(command,context,command->points[index].position,
                                                     &command->points[index].targetSpeed,0.5f);
        if(!accepted)finished=1;
        ++index;
        const int32_t last=driverCommandLastSegment(command);
        while(!finished&&index<=last){
            const int32_t previous=index-1;
            float distance=driverCommandDistanceTwo(command->points[previous].position,command->points[index].position);
            time=driverCommandSegmentTime(distance,command->points[previous].targetSpeed,command->points[index].targetSpeed);
            if(time<0.4000000059604645f&&index<command->count){
                do{
                    ++index;
                    if(index>=command->count)break;
                    distance=driverCommandDistanceTwo(command->points[previous].position,command->points[index].position);
                    time=driverCommandSegmentTime(distance,command->points[previous].targetSpeed,command->points[index].targetSpeed);
                }while(time<0.4000000059604645f&&index<command->count);
            }
            if(index<=last){
                accepted=driverCommandCheckTurn(command,context,command->points[previous].position,
                    command->points[index].position,command->points[index+1].position,&command->points[index].targetSpeed,
                    distance,command->points[previous].targetSpeed);
                ++index;
            }else finished=1;
            if(!accepted)finished=1;
        }
    }
    if(!accepted)return -1.0f;
    float totalDistance=distances[first];
    const float halfDistance=distances[first]*0.5f;
    float weighted=halfDistance*(command->points[first].targetSpeed+vehicle->speed);
    for(int32_t index=first+1;index<command->count;++index){
        const float half=distances[index]*0.5f;
        totalDistance=totalDistance+distances[index];
        const float speed=command->points[index].targetSpeed+command->points[index-1].targetSpeed;
        weighted=weighted+half*speed;
    }
    return 0.0f<totalDistance?finiteRatio(weighted,totalDistance):-1.0f;
}
