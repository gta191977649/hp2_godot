#include "indep/inc/CarCollisionDetect.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "bware/inc/bMath.hpp"
#include <math.h>
#include <string.h>

static inline float driverCollisionPredictionDotTwo(const float first[2],const float second[2])
{
    const float x=first[0]*second[0];
    const float y=first[1]*second[1];
    return x+y;
}

static inline float driverCollisionPredictionDotThree(const float first[4],const float second[4])
{
    const float x=first[0]*second[0];
    const float y=first[1]*second[1];
    const float z=first[2]*second[2];
    return (x+y)+z;
}

static inline void driverCollisionPredictionCrossThree(float output[4],const float first[4],const float second[4])
{
    for(int axis=0;axis<3;++axis){
        const int next=(axis+1)%3;
        const int last=(axis+2)%3;
        const float positive=first[next]*second[last];
        const float negative=second[next]*first[last];
        output[axis]=positive-negative;
    }
    output[3]=0.0f;
}

static inline float driverCollisionPredictionProjectedRadius(const DriverCollisionBox* first,const DriverCollisionBox* second,
    int32_t firstAxis,int32_t secondAxis)
{
    const DriverCollisionBox* basis=firstAxis>=0?first:second;
    const DriverCollisionBox* other=firstAxis>=0?second:first;
    const int32_t selected=firstAxis>=0?firstAxis:secondAxis;
    const float own=fabsf(basis->halfExtents[selected]);
    const float firstDot=driverCollisionPredictionDotTwo(other->axes[0],basis->axes[selected]);
    const float secondDot=driverCollisionPredictionDotTwo(other->axes[1],basis->axes[selected]);
    const float firstRadius=fabsf(other->halfExtents[0]*firstDot);
    const float secondRadius=fabsf(other->halfExtents[1]*secondDot);
    return (own+firstRadius)+secondRadius;
}

static inline void driverCollisionPredictionSetPlanarBasis(DriverCollisionBox* box,float firstX,float firstY)
{
    const float axes[3][4]={{firstX,firstY,0.0f,0.0f},{-firstY,firstX,0.0f,0.0f},
                           {0.0f,0.0f,1.0f,0.0f}};
    memcpy(box->axes,axes,sizeof(axes));
}

static inline const DriverAIBase* driverCollisionPredictionAiOwner(const CarMovement* movement)
{
    const CarDriver* driver=(const CarDriver*)movement->bindings.owner->controller;
    if(driver->kind==CAR_DRIVER_AI)return &driver->owner.ai->base;
    if(driver->kind==CAR_DRIVER_TRAFFIC)return &driver->owner.traffic->base;
    return NULL;
}

static inline int32_t driverCollisionPredictionActionKind(const CarMovement* movement)
{
    const DriverAIBase* driver=driverCollisionPredictionAiOwner(movement);
    return driver&&driver->action?driverAIActionKind(driver->action):11;
}

static inline int32_t driverCollisionPredictionReverseNeedsStraight(const DriverCommand* command,int32_t kind)
{
    return kind==7||kind==11||kind==10||command->count==0;
}

int32_t driverCollisionIntersectIntervals(float* minimum,float* maximum,
    float firstMinimum,float firstMaximum,float secondMinimum,float secondMaximum)
{
    float values[4]={firstMinimum,firstMaximum,secondMinimum,secondMaximum};
    for(int end=3;end>0;--end){
        for(int index=0;index<end;++index){
            if(values[index+1]<values[index]){
                const float value=values[index];
                values[index]=values[index+1];
                values[index+1]=value;
            }
        }
    }
    const float begin=values[1];
    if(firstMinimum<=begin&&begin<=firstMaximum&&secondMinimum<=begin&&begin<=secondMaximum){
        *minimum=begin;
        *maximum=values[2];
        return 1;
    }
    return 0;
}

int32_t driverCollisionSeparatedOnAxis(const DriverCollisionBox* first,
    const DriverCollisionBox* second,const float axis[4],const float displacement[4])
{
    const float distance=fabsf(driverCollisionPredictionDotThree(displacement,axis));
    float terms[6];
    for(int index=0;index<3;++index){
        terms[index]=fabsf(first->halfExtents[index]*driverCollisionPredictionDotThree(first->axes[index],axis));
        terms[index+3]=fabsf(second->halfExtents[index]*driverCollisionPredictionDotThree(second->axes[index],axis));
    }
    float radius=terms[0]+terms[1];
    for(int index=2;index<6;++index)radius=radius+terms[index];
    return radius<distance;
}

int32_t driverCollisionBoxesOverlap2D(const DriverCollisionBox* first,
    const DriverCollisionBox* second,const float displacement[2])
{
    for(int index=0;index<4;++index){
        const int axis=index%2==0?1:0;
        const DriverCollisionBox* basis=index<2?first:second;
        const float radius=driverCollisionPredictionProjectedRadius(first,second,index<2?axis:-1,axis);
        const float distance=fabsf(driverCollisionPredictionDotTwo(displacement,basis->axes[axis]));
        if(radius<distance)return 0;
    }
    return 1;
}

int32_t driverCollisionBoxesOverlap3D(const DriverCollisionBox* first,
    const DriverCollisionBox* second,const float displacement[4])
{
    /* The original visits the fifteen pairwise crosses of the two bases
     * in this exact order and stops at the first separating axis. */
    const float* axes[6]={first->axes[0],first->axes[1],first->axes[2],
                          second->axes[0],second->axes[1],second->axes[2]};
    for(int firstAxis=0;firstAxis<5;++firstAxis){
        for(int secondAxis=firstAxis+1;secondAxis<6;++secondAxis){
            float axis[4];
            driverCollisionPredictionCrossThree(axis,axes[firstAxis],axes[secondAxis]);
            if(driverCollisionSeparatedOnAxis(first,second,axis,displacement))return 0;
        }
    }
    return 1;
}

int32_t driverCollisionLowSpeedClose(const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception)
{
    if(!(fabsf(neighbor->movement->bindings.state->speed)<=5.0f))return 0;
    const float lateral=perception->lateralExtent+neighbor->effectiveHalfExtents[0];
    if(!(fabsf(neighbor->localPosition[0])<lateral))return 0;
    const float forward=perception->forwardExtent+neighbor->effectiveHalfExtents[1];
    return neighbor->localPosition[1]<forward+23.0f;
}

void driverCollisionAxisInterval(float* minimum,float* maximum,
    float relativeVelocity,float displacement,float radius)
{
    if(fabsf(relativeVelocity)<0.00009999999747378752f){
        if(radius<fabsf(displacement)){
            *minimum=9999.0f;
            *maximum=9999.0f;
        }else{
            *minimum=-9999.0f;
            *maximum=9999.0f;
        }
    }else{
        const float sum=radius+displacement;
        const float difference=radius-displacement;
        *maximum=finiteRatio(difference,relativeVelocity);
        *minimum=finiteRatio(-sum,relativeVelocity);
        if(relativeVelocity<0.0f){
            const float value=*minimum;
            *minimum=*maximum;
            *maximum=value;
        }
    }
}
float driverCollisionBoxTime2D(const DriverCollisionBox* first,
    const DriverCollisionBox* second,const float displacement[2],const float relativeVelocity[2])
{
    float intervals[4][2];
    for(int index=0;index<4;++index){
        const int axis=index%2==0?1:0;
        const DriverCollisionBox* basis=index<2?first:second;
        const float radius=driverCollisionPredictionProjectedRadius(first,second,index<2?axis:-1,axis);
        const float velocity=driverCollisionPredictionDotTwo(relativeVelocity,basis->axes[axis]);
        const float position=driverCollisionPredictionDotTwo(displacement,basis->axes[axis]);
        driverCollisionAxisInterval(&intervals[index][0],&intervals[index][1],velocity,position,radius);
    }
    float minimum,maximum;
    if(!driverCollisionIntersectIntervals(&minimum,&maximum,intervals[0][0],intervals[0][1],intervals[1][0],intervals[1][1])||
       !driverCollisionIntersectIntervals(&minimum,&maximum,minimum,maximum,intervals[2][0],intervals[2][1])||
       !driverCollisionIntersectIntervals(&minimum,&maximum,minimum,maximum,intervals[3][0],intervals[3][1]))
        minimum=maximum=99999.0f;
    if(0.0f<minimum)return minimum;
    if(minimum<0.0f&&0.0f<maximum)return 0.0f;
    return 99999.0f;
}
void driverCollisionBuildPathBoxes(const DriverCommand* command,DriverCollisionBox* boxes,
    const DriverPhysicsVehicle* vehicle,float scale)
{
    if(!command||command->count<=0||!boxes||!vehicle)return;
    float minimum[4],maximum[4],extents[3];
    driverCollisionBounds(vehicle,minimum,maximum);
    for(int axis=0;axis<3;++axis){
        const float difference=maximum[axis]-minimum[axis];
        const float half=difference*0.5f;
        extents[axis]=fabsf(half)*scale;
    }
    float sine,cosine;
    vectorSineCosineAngle(&sine,&cosine,vehicle->movement->bindings.state->heading);
    driverCollisionPredictionSetPlanarBasis(&boxes[0],cosine,sine);
    memcpy(boxes[0].halfExtents,extents,sizeof(extents));
    memcpy(boxes[0].center,command->points[0].position,sizeof(boxes[0].center));
    for(int32_t index=1;index<command->count;++index){
        float direction[2]={command->points[index-1].position[0]-command->points[index].position[0],
                            command->points[index-1].position[1]-command->points[index].position[1]};
        vectorNormalize2(direction,direction);
        driverCollisionPredictionSetPlanarBasis(&boxes[index],direction[0],direction[1]);
        memcpy(boxes[index].halfExtents,extents,sizeof(extents));
        memcpy(boxes[index].center,command->points[index].position,sizeof(boxes[index].center));
    }
}
float driverCollisionVelocityTime(const DriverPhysicsVehicle* first,
    const DriverPhysicsVehicle* second,float scale)
{
    const VehicleState* firstState=first->movement->bindings.state;
    const VehicleState* secondState=second->movement->bindings.state;
    float firstMinimum[4],firstMaximum[4],secondMinimum[4],secondMaximum[4];
    driverCollisionBounds(first,firstMinimum,firstMaximum);
    driverCollisionBounds(second,secondMinimum,secondMaximum);
    DriverCollisionBox boxes[2];
    const DriverPhysicsVehicle* vehicles[2]={first,second};
    const float* maxima[2]={firstMaximum,secondMaximum};
    for(int index=0;index<2;++index){
        const uint16_t heading=vehicles[index]->movement->bindings.state->heading;
        driverCollisionPredictionSetPlanarBasis(&boxes[index],vectorCosineAngle(heading),rotationSineAngle(heading));
        boxes[index].halfExtents[0]=fabsf(maxima[index][1]*scale);
        boxes[index].halfExtents[1]=fabsf(maxima[index][0]*scale);
        boxes[index].halfExtents[2]=fabsf(maxima[index][2]*scale);
    }
    const float position[2]={secondState->position[0]-firstState->position[0],
                             secondState->position[1]-firstState->position[1]};
    const float velocity[2]={secondState->velocity[0]-firstState->velocity[0],
                             secondState->velocity[1]-firstState->velocity[1]};
    return driverCollisionBoxTime2D(&boxes[0],&boxes[1],position,velocity);
}
int32_t driverCollisionPredictPair(const DriverPhysicsVehicle* first,const DriverCommand* firstCommand,
    const DriverPhysicsVehicle* second,const DriverCommand* secondCommand,float horizon,float* time)
{
    const CarMovement* firstMovement=first->movement;
    const CarMovement* secondMovement=second->movement;
    const VehicleState* firstState=firstMovement->bindings.state;
    const VehicleState* secondState=secondMovement->bindings.state;
    const CarDriver* firstDriver=(const CarDriver*)firstMovement->bindings.owner->controller;
    const CarDriver* secondDriver=(const CarDriver*)secondMovement->bindings.owner->controller;
    DriverCommand paths[2];
    driverCommandCopy(&paths[0],firstCommand);
    driverCommandCopy(&paths[1],secondCommand);
    if(firstMovement->bindings.owner->driverInfo->driverClass==3&&
       -0.25f<carDriverLateralExcess(firstDriver))
        driverCommandBuildVelocityPath(&paths[0],first);
    else driverCommandBlendPredictedPath(&paths[0],first);
    if(secondMovement->bindings.owner->driverInfo->driverClass==3){
        if(-0.25f<carDriverLateralExcess(secondDriver))
            driverCommandBuildVelocityPath(&paths[1],second);
        else if(driverCollisionPredictionActionKind(secondMovement)!=5)
            driverCommandBlendPredictedPath(&paths[1],second);
    }
    float firstScale=firstMovement->bindings.ownerMotion->inputCategory==3?1.2999999523162842f:1.0f;
    float secondScale=secondMovement->bindings.ownerMotion->inputCategory==3?1.2999999523162842f:1.0f;
    float firstSpeed=firstState->speed;
    float secondSpeed=secondState->speed;
    if(firstState->forwardSpeed<0.0f&&driverCollisionPredictionReverseNeedsStraight(&paths[0],driverCollisionPredictionActionKind(firstMovement))){
        driverCommandBuildStraight(&paths[0],first);
        firstSpeed=fabsf(firstState->speed);
        firstScale=1.2999999523162842f;
    }
    const int32_t secondCategory=secondMovement->bindings.ownerMotion->inputCategory;
    if(secondState->forwardSpeed<0.0f&&(secondCategory==2||secondCategory==3)&&
       driverCollisionPredictionReverseNeedsStraight(&paths[1],driverCollisionPredictionActionKind(secondMovement))){
        driverCommandBuildStraight(&paths[1],second);
        secondSpeed=fabsf(secondState->speed);
        secondScale=1.2999999523162842f;
    }
    firstSpeed=maximumFinite(firstSpeed,0.0010000000474974513f);
    secondSpeed=maximumFinite(secondSpeed,0.0010000000474974513f);
    if(firstSpeed<2.0f)firstScale=1.0f;
    if(secondSpeed<2.0f)secondScale=1.0f;
    const DriverAIBase* firstAI=driverCollisionPredictionAiOwner(firstMovement);
    if(firstMovement->bindings.owner->driverInfo->driverClass==3&&
       firstAI->environment->tuning->predictionWideBounds)
        firstScale=1000.0f;
    DriverCollisionBox boxes[2][DRIVER_COMMAND_CAPACITY];
    driverCollisionBuildPathBoxes(&paths[0],boxes[0],first,firstScale);
    driverCollisionBuildPathBoxes(&paths[1],boxes[1],second,secondScale);
    int32_t firstIndex=driverCommandClosestPoint(&paths[0],firstState,
        firstMovement->bindings.environment->clock->worldTimer);
    int32_t secondIndex=driverCommandClosestPoint(&paths[1],secondState,
        secondMovement->bindings.environment->clock->worldTimer);
    float firstPosition[2]={firstState->position[0],firstState->position[1]};
    float secondPosition[2]={secondState->position[0],secondState->position[1]};
    float accumulated=0.0f;
    float collisionTime=99999.0f;
    while(firstIndex<paths[0].count&&secondIndex<paths[1].count&&accumulated<=horizon){
        float firstDelta[2]={boxes[0][firstIndex].center[0]-firstPosition[0],
                              boxes[0][firstIndex].center[1]-firstPosition[1]};
        float secondDelta[2]={boxes[1][secondIndex].center[0]-secondPosition[0],
                               boxes[1][secondIndex].center[1]-secondPosition[1]};
        const float firstLength=sqrtf(driverCollisionPredictionDotTwo(firstDelta,firstDelta));
        const float secondLength=sqrtf(driverCollisionPredictionDotTwo(secondDelta,secondDelta));
        const float firstSeconds=finiteRatio(firstLength,firstSpeed);
        const float secondSeconds=finiteRatio(secondLength,secondSpeed);
        const float step=minimumFinite(firstSeconds,secondSeconds);
        const float displacement[2]={secondPosition[0]-firstPosition[0],
                                     secondPosition[1]-firstPosition[1]};
        float firstVelocity[2],secondVelocity[2];
        vectorSetLength2(firstVelocity,firstDelta,firstSpeed);
        vectorSetLength2(secondVelocity,secondDelta,secondSpeed);
        const float relativeVelocity[2]={secondVelocity[0]-firstVelocity[0],
                                         secondVelocity[1]-firstVelocity[1]};
        const float intersection=driverCollisionBoxTime2D(&boxes[0][firstIndex],
            &boxes[1][secondIndex],displacement,relativeVelocity);
        if(intersection<=step){collisionTime=accumulated+intersection;break;}
        const float firstFraction=finiteRatio(step*firstSpeed,firstLength);
        const float secondFraction=finiteRatio(step*secondSpeed,secondLength);
        for(int axis=0;axis<2;++axis){
            const float firstOffset=firstDelta[axis]*firstFraction;
            const float secondOffset=secondDelta[axis]*secondFraction;
            firstPosition[axis]=firstPosition[axis]+firstOffset;
            secondPosition[axis]=secondPosition[axis]+secondOffset;
        }
        if(fabsf(firstSeconds-step)<=0.0010000000474974513f)++firstIndex;
        else ++secondIndex;
        accumulated=accumulated+step;
    }
    *time=collisionTime;
    return collisionTime<99999.0f&&
        !(fabsf(collisionTime-99999.0f)<=0.0010000000474974513f);
}
CarMovement* driverCollisionPredictNeighbors(const DriverAIBase* driver,
    const DriverCommand* command,const DriverAIPerception* perception,const DriverAIRoadState* road,
    float horizon,float* time,World* world)
{
    if(!road||driverAIRoadEmpty(road)||command->count<=0||command->count>DRIVER_COMMAND_CAPACITY)
        return NULL;
    const DriverPhysicsVehicle* vehicle=&driver->physics;
    CarMovement* closest=NULL;
    float closestTime=99999.0f;
    for(int32_t index=0;index<perception->count;++index){
        const DriverAINeighbor* neighbor=&perception->neighbors[index];
        if(!driverCollisionNeighborEligible(neighbor,perception))continue;
        float candidateTime=1234.0f;
        int32_t collision=driverCollisionLowSpeedClose(neighbor,perception);
        if(collision)candidateTime=0.0f;
        if(!collision&&!(fabsf(vehicle->movement->bindings.state->speed)<=0.0010000000474974513f)&&
           neighbor->movement!=driver->planningTarget){
            DriverPhysicsVehicle other=*vehicle;
            other.movement=neighbor->movement;
            const CarDriver* otherDriver=(const CarDriver*)other.movement->bindings.owner->controller;
            const DriverCommand* otherCommand=carDriverCommand(otherDriver);
            if(!otherCommand){
                vehicle->movement->result=CAR_MOVEMENT_INVALID_BINDING;
                return NULL;
            }
            collision=driverCollisionPredictPair(vehicle,command,&other,otherCommand,horizon,&candidateTime);
        }
        if(collision&&candidateTime<closestTime){
            closest=neighbor->movement;
            closestTime=candidateTime;
        }
        worldAdvanceFrame(world);
    }
    *time=closestTime;
    return closest;
}
