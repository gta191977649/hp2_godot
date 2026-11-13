#include "indep/inc/CopDriver.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/eMath.hpp"
#include <math.h>
#include <string.h>

int32_t copDriverLookAhead(float output[2],float* lateralError,const DriverCommand* command,
    const float position[2],const float offset[2],float distance)
{
    if(command->count==1){
        const float x=position[0]-command->points[0].position[0];
        const float y=position[1]-command->points[0].position[1];
        const float xx=x*x;
        const float yy=y*y;
        if(sqrtf(xx+yy)<=distance){
            output[0]=command->points[0].position[0];
            output[1]=command->points[0].position[1];
            *lateralError=0.0f;
            return 1;
        }
    }else if(command->count>=2){
        for(int32_t index=command->count-2;index>=0;--index){
            const float* first=command->points[index].position;
            const float* second=command->points[index+1].position;
            float directionX=second[0]-first[0];
            float directionY=second[1]-first[1];
            const float xx=directionX*directionX;
            const float yy=directionY*directionY;
            const float length=sqrtf(xx+yy);
            if(length!=0.0f){
                const float inverse=finiteRatio(1.0f,length);
                directionY=directionY*inverse;
                directionX=directionX*inverse;
            }
            const float deltaY=position[1]-first[1];
            const float deltaX=position[0]-first[0];
            const float projectedY=deltaY*directionY;
            const float projectedX=deltaX*directionX;
            const float projection=projectedX+projectedY;
            if(!(0.0f<=projection&&projection<length))continue;
            float covered=-projection;
            for(int32_t following=index+1;following<command->count;++following){
                const float* previous=command->points[following-1].position;
                const float* next=command->points[following].position;
                const float dx=next[0]-previous[0];
                const float dy=next[1]-previous[1];
                const float dxSquared=dx*dx;
                const float dySquared=dy*dy;
                const float segmentLength=sqrtf(dxSquared+dySquared);
                covered=covered+segmentLength;
                if(distance<=covered){
                    const float remaining=covered-distance;
                    const float inverse=finiteRatio(1.0f,segmentLength);
                    const float xDirection=dx*inverse;
                    const float yDirection=dy*inverse;
                    const float alongX=xDirection*remaining;
                    const float alongY=yDirection*remaining;
                    const float offsetX=xDirection*offset[0];
                    const float offsetY=yDirection*offset[1];
                    const float baseX=next[0]-alongX;
                    output[0]=(baseX+offsetX)+offsetY;
                    const float tangentY=yDirection*offset[0];
                    const float normalY=(-xDirection)*offset[1];
                    const float baseY=next[1]-alongY;
                    output[1]=(baseY+tangentY)+normalY;
                    const float errorX=position[0]-output[0];
                    const float errorY=position[1]-output[1];
                    const float lateralX=errorX*yDirection;
                    const float lateralY=errorY*(-xDirection);
                    *lateralError=fabsf(lateralX+lateralY);
                    return 1;
                }
            }
            break;
        }
    }
    const int32_t fallback=command->count<2?0:1;
    output[0]=command->points[fallback].position[0];
    output[1]=command->points[fallback].position[1];
    return 0;
}

void copDriverPointFromEnd(float output[2],const DriverCommand* command,const float offset[2])
{
    if(command->count<=0)return;
    int32_t current=command->count-1;
    int32_t following=current;
    if(command->count<2){
        output[0]=command->points[current].position[0];
        output[1]=command->points[current].position[1];
        return;
    }
    float covered=0.0f;
    while(current>0&&covered<=offset[0]){
        following=current;
        --current;
        const float x=command->points[current].position[0]-command->points[following].position[0];
        const float y=command->points[current].position[1]-command->points[following].position[1];
        const float xx=x*x;
        const float yy=y*y;
        covered=covered+sqrtf(xx+yy);
    }
    const float* first=command->points[current].position;
    const float* next=command->points[following].position;
    float x=next[0]-first[0];
    float y=next[1]-first[1];
    const float remaining=covered-offset[0];
    const float xx=x*x;
    const float yy=y*y;
    const float length=sqrtf(xx+yy);
    float scaledX=x,scaledY=y;
    if(0.0f<length){
        const float scale=finiteRatio(remaining,length);
        scaledY=y*scale;
        scaledX=x*scale;
    }
    output[0]=first[0]+scaledX;
    output[1]=first[1]+scaledY;
    const float normalX=y;
    const float normalY=-x;
    const float normalXX=normalX*normalX;
    const float normalYY=normalY*normalY;
    const float normalLength=sqrtf(normalXX+normalYY);
    if(normalLength!=0.0f){
        x=finiteRatio(normalY,normalLength);
        y=finiteRatio(normalX,normalLength);
    }else{x=normalY;y=normalX;}
    output[0]=output[0]+y*offset[1];
    output[1]=output[1]+x*offset[1];
}

#include "indep/inc/Replay.hpp"

// PAL1A0F20..1A102C. Scalar transfer widths and order are source-defined;
// vector padding is deliberately not serialized.
void copMoverSnapshot(CopMover* mover,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,mover,"CopMover");
    replaySnapshotMoveData(snapshot,mover->velocity,12,-1,0);
    replaySnapshotMoveData(snapshot,&mover->heading,2,2,0);
    replaySnapshotMoveData(snapshot,&mover->targetHeading,2,2,0);
    replaySnapshotMoveFloat(snapshot,&mover->speed);
    replaySnapshotMoveFloat(snapshot,&mover->speedLimit);
    replaySnapshotMoveFloat(snapshot,&mover->targetSpeed);
    replaySnapshotMoveData(snapshot,mover->targetVelocity,12,-1,0);
    replaySnapshotMoveData(snapshot,&mover->mode,1,-1,0);
    replaySnapshotMoveFloat(snapshot,&mover->brakingLimit);
    replaySnapshotMoveFloat(snapshot,&mover->accelerationLimit);
    replaySnapshotMoveData(snapshot,mover->groundNormal,12,-1,0);
    replaySnapshotMoveData(snapshot,&mover->airborne,1,4,0);
    replaySnapshotMoveData(snapshot,mover->lateralAcceleration,8,-1,0);
    replaySnapshotMoveFloat(snapshot,&mover->brakingBlend);
    if(snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK)
        mover->velocity[3]=mover->targetVelocity[3]=mover->groundNormal[3]=0.0f;
    replaySnapshotEndSection(snapshot);
}

// Full PAL1A2B58..1A2C30. Source modes are byte transfers, independent of
// the typed host carrier; no pursuit or road state is duplicated here.
void copDriverSnapshot(CopDriver* driver,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,driver,"CopDriver");
    driverRouteSnapshot(&driver->base,snapshot);
    replaySnapshotMoveData(snapshot,&driver->drivingMode,1,-1,0);
    replaySnapshotMoveData(snapshot,&driver->previousMode,1,-1,0);
    replaySnapshotMoveData(snapshot,&driver->commandFromTarget,1,-1,0);
    replaySnapshotMoveFloat(snapshot,&driver->stalledSeconds);
    replaySnapshotMoveFloat(snapshot,&driver->reverseSeconds);
    replaySnapshotMoveFloat(snapshot,&driver->targetSpeed);
    replaySnapshotMoveData(snapshot,driver->lookAhead,8,-1,0);
    replaySnapshotMoveInteger(snapshot,&driver->entryIndex,1);
    replaySnapshotEndSection(snapshot);
}

// CopDriver allocation and control use typed PC owners.
CarDriverResult copDriverInitialize(CopDriver* driver,CarMovement* movement,PursuitCollisionContext* pursuit)
{
    if(!driver||!movement||!movement->bindings.controls||!pursuit)return CAR_DRIVER_INVALID_BINDING;
    int32_t routeClass;
    switch(movement->info.modelCode){
    case 4:routeClass=4;break;
    case 31:routeClass=3;break;
    case 32:routeClass=0;break;
    case 33:routeClass=1;break;
    case 34:routeClass=2;break;
    default:return CAR_DRIVER_INVALID_BINDING;
    }
    memset(driver,0,sizeof(*driver));
    const CarDriverResult result=driverRouteInitialize(&driver->base,movement,CAR_DRIVER_COP);
    if(result!=CAR_DRIVER_OK)return result;
    driver->pursuit=pursuit;
    driverCommandInitialize(&driver->command);
    CarControls* controls=movement->bindings.controls;
    controls->steeringAngle=0;
    controls->throttle=0.0f;
    controls->brake=0.0f;
    controls->handbrake=0.0f;
    controls->gearRequest=-1;
    controls->nitroRequest=0;
    driver->drivingMode=1;
    driver->previousMode=1;
    driver->commandFromTarget=1;
    driver->entryIndex=-1;
    driver->targetSpeed=29.056772232055664f;
    driver->routeClassLimit=routeClass;
    return CAR_DRIVER_OK;
}

int32_t copDriverCheckStall(CopDriver* driver,float elapsed)
{
    if(!driver->base.movement->bindings.ownerMotion->collision->shape->contactCount)return 0;
    if(driver->drivingMode==2||driver->drivingMode==3){
        if(driver->base.movement->bindings.state->speed<2.0f){
            driver->stalledSeconds=driver->stalledSeconds+elapsed;
            return 1.0f<driver->stalledSeconds;
        }
        driver->stalledSeconds=0.0f;
    }else if(0<=driver->drivingMode&&driver->drivingMode<5)driver->stalledSeconds=0.0f;
    return 0;
}

void copDriverSelectTargetPoint(CopDriver* driver,float output[2])
{
    if(driver->entryIndex<0||(size_t)driver->entryIndex>=driver->pursuit->entryCount){
        driver->base.result=CAR_DRIVER_MISSING_PURSUIT;return;
    }
    PursuitCollisionEntry* entry=driver->pursuit->entries[driver->entryIndex];
    if(!entry||!entry->group)return;
    const CarMovement* movement=driver->base.movement;
    const CarMovement* target=entry->group->target;
    if(entry->drivingState<3||entry->drivingState>12)return;
    const float* ownPosition=movement->bindings.state->position;
    const float* targetPosition=target->bindings.state->position;
    float error;
    if(entry->drivingState==9||entry->drivingState==10||entry->drivingState==11){
        driver->commandFromTarget=0;
        CarDriverResult result=pursuitEntrySelectRoad(entry);
        if(result!=CAR_DRIVER_OK){driver->base.result=result;return;}
        const float* position=entry->drivingState==11?ownPosition:targetPosition;
        result=pursuitEntryBuildRoadCommand(entry,&driver->command,position,0);
        if(result!=CAR_DRIVER_OK){driver->base.result=result;return;}
        float offset[2]={0.0f,0.0f};
        if(entry->drivingState==9){
            const CarDriver* targetDriver=(const CarDriver*)target->bindings.owner->controller;
            const CarRecoveryPose* pose=carDriverCurrentPose(targetDriver,1);
            if(!pose){driver->base.result=CAR_DRIVER_MISSING_ROAD;return;}
            const float x=targetPosition[0]-pose->position[0];
            const float y=targetPosition[1]-pose->position[1];
            const float first=x*pose->direction[1];
            const float second=y*pose->direction[0];
            offset[1]=first-second;
        }
        copDriverLookAhead(output,&error,&driver->command,position,offset,20.0f);
    }else{
        driver->commandFromTarget=1;
        const float x=ownPosition[0]-targetPosition[0];
        const float y=ownPosition[1]-targetPosition[1];
        const float xx=x*x;
        const float yy=y*y;
        if(sqrtf(xx+yy)<20.0f){
            driverCommandCopy(&driver->command,&entry->group->recentTargetPath);
            copDriverPointFromEnd(output,&entry->group->recentTargetPath,entry->targetOffset);
        }else{
            driverCommandCopy(&driver->command,&entry->group->targetPath);
            copDriverLookAhead(output,&error,&entry->group->targetPath,ownPosition,entry->targetOffset,20.0f);
        }
    }
}

static uint16_t headingTo(const float from[2],const float to[2])
{
    const float difference[2]={to[0]-from[0],to[1]-from[1]};
    VectorPolar2 polar;vectorPolarFromCartesian2(&polar,difference);
    return polar.angle;
}

static float distanceXY(const float first[2],const float second[2])
{
    const float x=first[0]-second[0];
    const float y=first[1]-second[1];
    const float xx=x*x;
    const float yy=y*y;
    return sqrtf(xx+yy);
}

static const CarRecoveryPose* currentPose(const CarMovement* movement)
{
    return carDriverCurrentPose((const CarDriver*)movement->bindings.owner->controller,1);
}

static float lateralPosition(const VehicleState* vehicle,const CarRecoveryPose* pose)
{
    const float x=vehicle->position[0]-pose->position[0];
    const float y=vehicle->position[1]-pose->position[1];
    const float first=x*pose->direction[1];
    const float second=y*pose->direction[0];
    return first-second;
}

static float lateralDifference(const CarMovement* movement,const CarMovement* target)
{
    const float targetLateral=lateralPosition(target->bindings.state,currentPose(target));
    const float ownLateral=lateralPosition(movement->bindings.state,currentPose(movement));
    return fabsf(targetLateral-ownLateral);
}

static void setVelocityTarget(CopDriver* driver,const PursuitCollisionEntry* entry,
                               const float difference[2])
{
    CarMovement* movement=driver->base.movement;
    CopMover* mover=movement->cop;
    const CarMovement* target=entry->group->target;
    const VehicleState* vehicle=movement->bindings.state;
    const VehicleState* targetState=target->bindings.state;
    float longitudinalWeight=1.0f;
    float lateralWeight=1.5f;
    float excessSpeed=0.0f;
    float targetVelocity[2];
    if(target->bindings.player){
        pursuitVelocityHistorySample(targetVelocity,&entry->group->velocityHistory,0.25f);
        const float xx=targetVelocity[0]*targetVelocity[0];
        const float yy=targetVelocity[1]*targetVelocity[1];
        const float length=sqrtf(xx+yy);
        if(0.0f<length){
            float prediction[2];
            pursuitVelocityHistorySample(prediction,&entry->group->velocityHistory,0.6499999761581421f);
            const float nextXX=prediction[0]*prediction[0];
            const float nextYY=prediction[1]*prediction[1];
            const float scale=finiteRatio(sqrtf(nextXX+nextYY),length);
            targetVelocity[0]=targetVelocity[0]*scale;
            targetVelocity[1]=targetVelocity[1]*scale;
        }
    }else{
        targetVelocity[0]=targetState->velocity[0];
        targetVelocity[1]=targetState->velocity[1];
    }
    switch(entry->drivingState){
    case 7:case 10:longitudinalWeight=1.5f;excessSpeed=3.0f;break;
    case 8:excessSpeed=3.0f;lateralWeight=10.0f;break;
    case 9:{
        float scale;
        if(lateralDifference(movement,target)<=2.5f){
            if(targetState->forwardSpeed*2.236999988555908f<60.0f){
                targetVelocity[0]=0.0f;targetVelocity[1]=0.0f;break;
            }
            scale=0.5f;
        }else scale=1.100000023841858f;
        targetVelocity[0]=targetVelocity[0]*scale;
        targetVelocity[1]=targetVelocity[1]*scale;
        break;
    }
    case 12:mover->brakingLimit=14.0f;lateralWeight=3.0f;break;
    default:break;
    }
    float direction[2];vectorNormalize2(direction,vehicle->velocity);
    const float first=difference[0]*direction[0];
    const float second=difference[1]*direction[1];
    const float projection=first+second;
    const float parallelX=direction[0]*projection;
    const float parallelY=direction[1]*projection;
    const float lateralX=difference[0]-parallelX;
    const float lateralY=difference[1]-parallelY;
    float velocity[4]={0.0f,0.0f,0.0f,0.0f};
    if(entry->drivingState!=12){
        const float alongX=parallelX*longitudinalWeight;
        const float alongY=parallelY*longitudinalWeight;
        const float acrossX=lateralX*lateralWeight;
        const float acrossY=lateralY*lateralWeight;
        velocity[0]=(alongX+acrossX)+targetVelocity[0];
        velocity[1]=(alongY+acrossY)+targetVelocity[1];
    }
    const float xx=velocity[0]*velocity[0];
    const float yy=velocity[1]*velocity[1];
    const float length=sqrtf((xx+yy)+0.0f);
    if(0.0f<length){
        const float scale=finiteRatio(length+excessSpeed,length);
        for(int axis=0;axis<4;++axis)velocity[axis]=velocity[axis]*scale;
    }
    for(int axis=0;axis<4;++axis)mover->targetVelocity[axis]=velocity[axis];
    mover->targetVelocity[2]=0.0f;
}

static void setHeadingTarget(CopDriver* driver,const PursuitCollisionEntry* entry,float distance,float targetSpeed)
{
    static const float difficultySpeedScale[3]={1.149999976158142f,1.2000000476837158f,1.25f};
    CarMovement* movement=driver->base.movement;
    CopMover* mover=movement->cop;
    const CarMovement* target=entry->group->target;
    const VehicleState* targetState=target->bindings.state;
    mover->mode=0;
    const float difference[2]={driver->lookAhead[0]-movement->bindings.state->position[0],
        driver->lookAhead[1]-movement->bindings.state->position[1]};
    VectorPolar2 polar;
    vectorPolarFromCartesian2(&polar,difference);
    mover->targetHeading=polar.angle;
    float speed=0.0f;
    switch(entry->drivingState){
    case 3:case 4:case 6:case 7:case 8:speed=distance+targetSpeed;break;
    case 5:speed=distance*5.0f+targetSpeed;break;
    case 9:
        if(lateralDifference(movement,target)<=2.5f){
            if(!(targetState->forwardSpeed*2.236999988555908f<60.0f))speed=targetSpeed*0.5f;
        }else speed=targetSpeed*1.100000023841858f;
        break;
    case 10:speed=targetSpeed*1.100000023841858f;break;
    case 11:speed=mover->speedLimit;break;
    default:break;
    }
    int32_t lane=target->info.routeVehicleClass;
    if(driver->routeClassLimit<lane)lane=driver->routeClassLimit;
    const CarRecoveryPose* pose=currentPose(movement);
    if(!pose||lane<0||lane>=21){driver->base.result=CAR_DRIVER_MISSING_ROAD;return;}
    int32_t difficulty=2;
    if(target->bindings.player)difficulty=driver->pursuit->severity;
    if(difficulty<0||difficulty>=3){driver->base.result=CAR_DRIVER_INVALID_BINDING;return;}
    const float authored=(float)((uint32_t)pose->laneSpeeds[lane]*65536u)*0.0000152587890625f;
    const float limit=authored*difficultySpeedScale[difficulty];
    speed=minimumFinite(limit,speed);
    const float minimumSpeed=8.940545082092285f;
    if(minimumSpeed<speed){
        const uint16_t heading=headingTo(movement->bindings.state->position,targetState->position);
        const uint16_t wrapped=(uint16_t)(heading-movement->bindings.state->heading);
        const int32_t headingDifference=wrapped<32768u?(int32_t)wrapped:(int32_t)wrapped-65536;
        const float magnitude=fabsf((float)headingDifference);
        if(16384.0f<magnitude&&1.0f<distance)speed=minimumSpeed;
    }
    mover->targetSpeed=speed;
}

void copDriverThink(CopDriver* driver,float elapsed)
{
    driverRouteAdvance(&driver->base,elapsed);
    if(driver->base.result!=CAR_DRIVER_OK)return;
    if(driver->entryIndex<0||(size_t)driver->entryIndex>=driver->pursuit->entryCount){
        driver->base.result=CAR_DRIVER_MISSING_PURSUIT;return;
    }
    PursuitCollisionEntry* entry=driver->pursuit->entries[driver->entryIndex];
    if(!entry){driver->base.result=CAR_DRIVER_MISSING_PURSUIT;return;}
    CarMovement* movement=driver->base.movement;
    if(movement->mode!=5)return;
    CopMover* mover=movement->cop;
    if(!mover){driver->base.result=CAR_DRIVER_INVALID_BINDING;return;}
    if(copDriverCheckStall(driver,elapsed)&&!pursuitEntryNearTarget(entry)){
        driver->previousMode=driver->drivingMode;
        driver->drivingMode=4;
        driver->reverseSeconds=2.0f;
    }
    mover->brakingLimit=40.0f;
    copMoverSetAccelerationLimit(mover,movement->info.modelCode);
    switch(driver->drivingMode){
    case 0:{
        CarControls* controls=movement->bindings.controls;
        mover->brakingLimit=2.0f;mover->targetSpeed=0.0f;mover->mode=0;
        controls->throttle=0.0f;controls->brake=1.0f;controls->handbrake=1.0f;
        controls->steeringAngle=0;controls->gearRequest=-1;controls->shiftTarget=-1;
        break;
    }
    case 1:mover->targetSpeed=0.0f;mover->mode=0;break;
    case 2:{
        float error;
        mover->mode=0;
        copDriverLookAhead(driver->lookAhead,&error,&driver->command,movement->bindings.state->position,entry->targetOffset,20.0f);
        mover->targetHeading=headingTo(movement->bindings.state->position,driver->lookAhead);
        mover->accelerationLimit=4.0f;mover->targetSpeed=driver->targetSpeed;
        break;
    }
    case 3:{
        if(!entry->group)return;
        const CarMovement* target=entry->group->target;
        const VehicleState* vehicle=movement->bindings.state;
        const VehicleState* targetState=target->bindings.state;
        const float targetSpeed=targetState->speed;
        copDriverSelectTargetPoint(driver,driver->lookAhead);
        if(driver->base.result!=CAR_DRIVER_OK)return;
        const float difference[2]={driver->lookAhead[0]-vehicle->position[0],driver->lookAhead[1]-vehicle->position[1]};
        const float distance=distanceXY(vehicle->position,targetState->position);
        const float first=targetState->velocity[0]*vehicle->velocity[0];
        const float second=targetState->velocity[1]*vehicle->velocity[1];
        if(distance<40.0f&&0.0f<first+second&&13.410818099975586f<=vehicle->speed){
            mover->mode=1;
            setVelocityTarget(driver,entry,difference);
        }else setHeadingTarget(driver,entry,distance,targetSpeed);
        break;
    }
    case 4:
        driver->reverseSeconds=driver->reverseSeconds-elapsed;
        if(driver->reverseSeconds<0.0f){driver->drivingMode=driver->previousMode;return;}
        if(driver->previousMode==2){
            float error;
            copDriverLookAhead(driver->lookAhead,&error,&driver->command,movement->bindings.state->position,entry->targetOffset,20.0f);
        }else if(driver->previousMode==3)copDriverSelectTargetPoint(driver,driver->lookAhead);
        mover->mode=0;
        mover->targetHeading=headingTo(movement->bindings.state->position,driver->lookAhead);
        mover->accelerationLimit=4.0f;mover->targetSpeed=-8.940545082092285f;
        break;
    default:break;
    }
}

// Remaining CopMover bodies: complete PAL1A0DD8/1A0EA0/1A1030/1A1BD0.
// Private C helpers below only factor regions of the original Move body.
void copMoverSetAccelerationLimit(CopMover* mover, int32_t modelCode)
{
    switch (modelCode) {
    case 4: mover->accelerationLimit = 11.25f; break;
    case 31: mover->accelerationLimit = 12.75f; break;
    case 32: case 33: mover->accelerationLimit = 15.0f; break;
    case 34: mover->accelerationLimit = 14.25f; break;
    }
}

int32_t copMoverInitialize(CopMover* mover, CarMovement* movement)
{
    if (!movement->bindings.pursuit || movement->bindings.pursuit->severity<0 ||
        movement->bindings.pursuit->severity>2) return 0;
    const int32_t model = movement->info.modelCode;
    if (model != 4 && (model < 31 || model > 34)) return 0;
    const VehicleState* state = movement->bindings.state;
    mover->movement = movement;
    memcpy(mover->velocity,state->velocity,3*sizeof(float));
    mover->velocity[3]=0.0f;
    memcpy(mover->targetVelocity,mover->velocity,sizeof(mover->targetVelocity));
    mover->speed = mover->targetSpeed = state->forwardSpeed;
    mover->heading = mover->targetHeading = state->heading;
    mover->speedLimit = 84.9351806640625f;
    mover->mode = 0;
    copMoverSetAccelerationLimit(mover,model);
    mover->brakingLimit = 40.0f;
    mover->lateralAcceleration[0] = mover->lateralAcceleration[1] = 0.0f;
    mover->groundNormal[0] = mover->groundNormal[1] = 0.0f;
    mover->groundNormal[2] = 1.0f;
    mover->groundNormal[3] = 0.0f;
    mover->airborne = 0;
    mover->brakingBlend = 0.0f;
    return 1;
}

float copMoverUpdateHeading(CopMover* mover, float maximumSpeed, float elapsed)
{
    const float maximumError = mover->speed >= 0.0f ?
        finiteRatio(-9000.0f,maximumSpeed)*fabsf(mover->speed) + 12000.0f : 12000.0f;
    float error = (float)(int16_t)(uint16_t)(mover->targetHeading-mover->heading);
    if (maximumError < fabsf(error)) error *= finiteRatio(maximumError,fabsf(error));
    float rate = error * (fabsf(mover->speed)*0.30000001192092896f);
    if (21845.0f < fabsf(rate)) rate = rate >= 0.0f ? 21845.0f : -21845.0f;
    float step = rate*elapsed;
    if (fabsf(error) < fabsf(step)) step = error;
    mover->heading = (uint16_t)(mover->heading + convertFloatToInt32(step));
    return rate;
}

static void constrainToGround(CopMover* mover)
{
    const float dot = (mover->velocity[0]*mover->groundNormal[0] +
                       mover->velocity[1]*mover->groundNormal[1]) + mover->velocity[2]*mover->groundNormal[2];
    const float projection = minimumFinite(dot,4.0f);
    for (int axis = 0; axis < 3; ++axis) mover->velocity[axis] -= mover->groundNormal[axis]*projection;
    if (4.0f < dot) mover->airborne = 1;
}

static void alignToGround(CopMover* mover)
{
    CarMovement* movement = mover->movement;
    VehicleState* state = movement->bindings.state;
    static const float offsets[5][2] = {{0,0},{1,0},{-1,0},{0,1},{0,-1}};
    float heights[5], maximum = -1000000.0f, minimum = 1000000.0f;
    for (int index = 0; index < 5; ++index) {
        const float point[4] = {offsets[index][0]+state->position[0], offsets[index][1]+state->position[1],state->position[2],0};
        heights[index] = terrainSurfaceElevation(movement->bindings.environment->contact.terrain,point,NULL,NULL,NULL);
        maximum = maximumFinite(maximum,heights[index]); minimum = minimumFinite(minimum,heights[index]);
    }
    if (fabsf(maximum-minimum) <= 2.0f) {
        const float positiveX = heights[1]-heights[0], negativeX = heights[2]-heights[0];
        const float positiveY = heights[3]-heights[0], negativeY = heights[4]-heights[0];
        const float slopeX = (positiveX-negativeX)*0.5f;
        const float slopeY = (positiveY-negativeY)*0.5f;
        const uint16_t yawTilt = (uint16_t)(0u-(uint32_t)convertFloatToInt32(slopeX*10435.66796875f));
        const uint16_t pitchTilt = (uint16_t)convertFloatToInt32(slopeY*10435.66796875f);
        const WheelTorqueInfo* wheel = &movement->info.wheels[3];
        state->position[2] = ((heights[0]+wheel->reactionScale)-wheel->mountPosition[2]) + movement->info.centerOfMass[2];
        float orientation[16]; rotationMatrixIdentity(orientation);
        motionMatrixRotateYaw(orientation,orientation,state->heading);
        motionMatrixRotateX(orientation,orientation,pitchTilt);
        motionMatrixRotateY(state->orientation,orientation,yawTilt);
        for (int axis = 0; axis < 3; ++axis) state->centerPosition[axis] = state->position[axis]-movement->info.centerOfMass[axis];
        state->centerPosition[3]=1.0f;
    } else mover->airborne = 1;
}

static void accelerateAlongHeading(CopMover* mover, float maximumSpeed, float maximumAcceleration,float elapsed)
{
    CarMovement* movement = mover->movement;
    VehicleState* state = movement->bindings.state;
    CarControls* controls = movement->bindings.controls;
    (void)copMoverUpdateHeading(mover,maximumSpeed,elapsed);
    state->heading = mover->heading;
    float acceleration = finiteRatio(mover->targetSpeed-mover->speed,elapsed);
    const float magnitude = fabsf(acceleration);
    if (acceleration < 0.0f) {
        mover->brakingBlend += elapsed+elapsed;
        if (1.0f < mover->brakingBlend) mover->brakingBlend = 1.0f;
        const float limit = mover->brakingLimit*mover->brakingBlend;
        if (limit < magnitude) acceleration *= finiteRatio(limit,magnitude);
        if (mover->brakingLimit*0.30000001192092896f < magnitude || mover->brakingBlend == 1.0f)
            controls->brake = finiteRatio(magnitude,mover->brakingLimit);
    } else {
        mover->brakingBlend = 0.0f;
        if (maximumAcceleration < magnitude) {
            acceleration *= finiteRatio(maximumAcceleration,magnitude);
            carFrameSetSpecialEffect(movement->frame,1);
        }
        controls->brake = 0.0f;
    }
    mover->speed += acceleration*elapsed;
    if (maximumSpeed < fabsf(mover->speed)) mover->speed *= finiteRatio(maximumSpeed,fabsf(mover->speed));
    state->forwardSpeed = mover->speed;
    mover->velocity[0] = mover->speed * vectorCosineAngle(mover->heading);
    mover->velocity[1] = mover->speed * rotationSineAngle(mover->heading);
    constrainToGround(mover);
    memcpy(state->velocity,mover->velocity,sizeof(mover->velocity));
    state->speed = sqrtf((mover->velocity[0]*mover->velocity[0] + mover->velocity[1]*mover->velocity[1]) + mover->velocity[2]*mover->velocity[2]);
    state->velocityFrameValid = 0;
}

static void accelerateTowardVelocity(CopMover* mover,float maximumSpeed,float maximumAcceleration,float elapsed)
{
    CarMovement* movement = mover->movement;
    VehicleState* state = movement->bindings.state;
    CarControls* controls = movement->bindings.controls;
    const float speed = sqrtf((mover->velocity[0]*mover->velocity[0] + mover->velocity[1]*mover->velocity[1]) + mover->velocity[2]*mover->velocity[2]);
    // At rest there is no longitudinal velocity axis. Keep the requested
    // acceleration in the existing bounded lateral controller, rather than
    // importing the exceptional 0/0 projection from PAL1A13D4/1A13DC.
    const float x = speed == 0.0f ? 0.0f : finiteRatio(mover->velocity[0],speed);
    const float y = speed == 0.0f ? 0.0f : finiteRatio(mover->velocity[1],speed);
    const float ax = finiteRatio(mover->targetVelocity[0]-mover->velocity[0],elapsed);
    const float ay = finiteRatio(mover->targetVelocity[1]-mover->velocity[1],elapsed);
    const float along = ax*x + ay*y;
    float longitudinal[2] = {along*x,along*y};
    const float oldLateral[2] = {mover->lateralAcceleration[0],mover->lateralAcceleration[1]};
    mover->lateralAcceleration[0] = ax-longitudinal[0]; mover->lateralAcceleration[1] = ay-longitudinal[1];
    float magnitude = sqrtf(mover->lateralAcceleration[0]*mover->lateralAcceleration[0] + mover->lateralAcceleration[1]*mover->lateralAcceleration[1]);
    if (35.0f < magnitude) {
        const float scale = finiteRatio(35.0f,magnitude);
        mover->lateralAcceleration[0] *= scale; mover->lateralAcceleration[1] *= scale;
    }
    float jerk[2] = {finiteRatio(mover->lateralAcceleration[0]-oldLateral[0],elapsed),
                     finiteRatio(mover->lateralAcceleration[1]-oldLateral[1],elapsed)};
    magnitude = sqrtf(jerk[0]*jerk[0]+jerk[1]*jerk[1]);
    if (150.0f < magnitude) {
        const float scale = finiteRatio(150.0f,magnitude);
        jerk[0] *= scale; jerk[1] *= scale;
        mover->lateralAcceleration[1] = oldLateral[1]+jerk[1]*elapsed;
        mover->lateralAcceleration[0] = oldLateral[0]+jerk[0]*elapsed;
    }
    magnitude = sqrtf(longitudinal[0]*longitudinal[0]+longitudinal[1]*longitudinal[1]);
    if (along < 0.0f) {
        mover->brakingBlend += elapsed+elapsed;
        if (1.0f < mover->brakingBlend) mover->brakingBlend = 1.0f;
        const float limit = mover->brakingLimit*mover->brakingBlend;
        if (limit < magnitude) {
            const float scale = finiteRatio(limit,magnitude);
            longitudinal[1] *= scale; longitudinal[0] *= scale;
            magnitude = limit;
        }
        if (mover->brakingLimit*0.30000001192092896f < magnitude || mover->brakingBlend == 1.0f)
            controls->brake = finiteRatio(magnitude,mover->brakingLimit);
    } else {
        mover->brakingBlend = 0.0f;controls->brake = 0.0f;
        if (maximumAcceleration < magnitude) {
            const float scale = finiteRatio(maximumAcceleration,magnitude);
            longitudinal[1] *= scale; longitudinal[0] *= scale;
            carFrameSetSpecialEffect(movement->frame,1);
        }
    }
    mover->velocity[0] += (longitudinal[0]+mover->lateralAcceleration[0])*elapsed;
    mover->velocity[1] += (longitudinal[1]+mover->lateralAcceleration[1])*elapsed;
    constrainToGround(mover);
    float velocityLength = sqrtf((mover->velocity[0]*mover->velocity[0] + mover->velocity[1]*mover->velocity[1]) + mover->velocity[2]*mover->velocity[2]);
    if (maximumSpeed < fabsf(velocityLength)) {
        const float scale = finiteRatio(maximumSpeed,fabsf(velocityLength));
        for (int axis = 0; axis < 3; ++axis) mover->velocity[axis] *= scale;
        velocityLength *= scale;
    }
    memcpy(state->velocity,mover->velocity,sizeof(mover->velocity));
    state->forwardSpeed = velocityLength;
    const float squared = (mover->velocity[0]*mover->velocity[0] + mover->velocity[1]*mover->velocity[1]) + mover->velocity[2]*mover->velocity[2];
    state->speed = sqrtf(squared);state->velocityFrameValid = 0;
    const float inverse = finiteRatio(1.0f,sqrtf(squared));
    mover->targetHeading = vectorFixedAtan2Angle(convertFloatToInt32((mover->velocity[0]*inverse)*65536.0f),
                                               convertFloatToInt32((mover->velocity[1]*inverse)*65536.0f));
    (void)copMoverUpdateHeading(mover,maximumSpeed,elapsed);
    state->heading = mover->heading;
}

float copMoverMove(CopMover* mover,float elapsed)
{
    // No time has passed: preserve controls, terrain pose and mover history
    // before the velocity controller can divide its requested delta by time.
    if (elapsed == 0.0f) return 0.0f;
    CarMovement* movement = mover->movement;
    VehicleState* state = movement->bindings.state;
    bool hasGround;
    (void)terrainSurfaceElevation(movement->bindings.environment->contact.terrain,state->position,NULL,mover->groundNormal,&hasGround);
    if (!hasGround) {
        mover->groundNormal[0] = mover->groundNormal[1] = 0.0f;
        mover->groundNormal[2] = 1.0f;
    }
    mover->groundNormal[3]=0.0f;
    mover->heading = state->heading;mover->speed = state->forwardSpeed;
    memcpy(mover->velocity,state->velocity,3*sizeof(float));
    mover->velocity[3]=0.0f;
    static const float difficultyBonus[3] = {-4.470272541046143f,-2.2351362705230713f,0};
    // PAL1A1104..1A114C reads the manager's copied/clamped pursuit
    // severity, independently of the racing opponents' difficulty.
    const float maximumSpeed = mover->speedLimit + difficultyBonus[movement->bindings.pursuit->severity];
    const float maximumAcceleration = (-mover->accelerationLimit/maximumSpeed)*fabsf(mover->speed) + mover->accelerationLimit;
    if (maximumSpeed < mover->targetSpeed) mover->targetSpeed = maximumSpeed;
    mover->airborne = 0;
    carFrameSetSpecialEffect(movement->frame,0);
    if (mover->mode == 0) accelerateAlongHeading(mover,maximumSpeed,maximumAcceleration,elapsed);
    else if (mover->mode == 1) accelerateTowardVelocity(mover,maximumSpeed,maximumAcceleration,elapsed);
    for (int axis = 0; axis < 3; ++axis) state->position[axis] += mover->velocity[axis]*elapsed;
    state->position[3]=1.0f;
    if (!mover->airborne) alignToGround(mover);
    vehicleStateUpdateCenterPosition(state);
    memset(state->angularVelocity,0,sizeof(state->angularVelocity));
    state->steeringAngle = movement->bindings.controls->steeringAngle;
    for (int wheel = 0; wheel < 4; ++wheel) {
        state->wheelAngularVelocity[wheel] = mover->speed / movement->info.wheels[wheel].reactionScale;
        state->wheelAngle[wheel] += state->wheelAngularVelocity[wheel]*elapsed;
        state->suspensionTravel[wheel] = 0.0f;
        vehicleStateSetWheelContact(state,wheel,1);vehicleStateSetWheelMaterial(state,wheel,1);
        state->wheelSlip[wheel][0] = state->wheelSlip[wheel][1] = 0.0f;
        state->wheelSlipRatio[wheel] = state->wheelNormalLoad[wheel] = 0.0f;
        memset(state->suspensionImpulse,0,sizeof(state->suspensionImpulse));
    }
    vehicleStateUpdateWheelPoses(state);
    carMovementUpdatePowertrainDisplay(movement,&carDriveDisplayDefaultTuning);
    return elapsed;
}
