#include "indep/inc/AIPlan.hpp"
#include "bware/inc/bMath.hpp"
int32_t driverAIActionActive(const DriverAIAction* action) {return action->status==DRIVER_AI_ACTION_ACTIVE;}
int32_t driverAIActionFailed(const DriverAIAction* action) {return action->status==DRIVER_AI_ACTION_FAILED;}
int32_t driverAIActionSucceeded(const DriverAIAction* action) {return action->status==DRIVER_AI_ACTION_SUCCEEDED;}
int32_t driverAIActionTimedOut(const DriverAIAction* action) {return action->status==DRIVER_AI_ACTION_TIMED_OUT && action->hasDeadline!=0;}

float driverAIActionRightMargin(const DriverAIPerception* perception,const CarRecoveryPose* pose)
{
    const float* position=perception->movement->bindings.state->position;
    const float x=position[0]-pose->position[0];
    const float y=position[1]-pose->position[1];
    const float first=x*pose->direction[1];
    const float second=y*pose->direction[0];
    const float across=first-second;
    const float width=pose->halfWidths[1]-perception->lateralExtent;
    return width-across;
}
// PAL1C9018..1C9070 / Beta GetGoalTrackRoutePoint: the recovery target
// is the third following authored road pose. The original has a fallback
// branch that selects the final group pose when a following point cannot be
// supplied. Treating this tail case as MISSING_ROAD aborts a traffic
// driver's recovery action during long races. Validate
// ownership first; preserve the +3 lookup on the ordinary (in-range) path.
const CarRecoveryPose* driverAIActionRecoveryGoalPose(const DriverAIRoadState* road)
{
    if (!road) return NULL;
    const CarRecoveryGroup* group=road->group;
    const int32_t index=carRecoveryOwnedPoseIndex(group,road->recovery.recoveryPose);
    if (index<0) return NULL;
    const int32_t remaining=group->poseCount-1-index;
    return &group->poses[index+(remaining>=3?3:remaining)];
}
#include "indep/inc/AICarController.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/Replay.hpp"
#include "bware/inc/bWare.hpp"
#include <cmath>
#include <cstdlib>
#include <memory>

// AIAction constructors share the existing action allocation and state.

DriverAIAction* driverAIActionInitialize(DriverAIAction* action,int32_t priority,float duration,int32_t nowTicks)
{
    driverAIActionSetTimeout(action,duration,nowTicks);
    driverCommandInitialize(&action->command);
    action->priority=priority;action->enabled=1;action->status=DRIVER_AI_ACTION_ACTIVE;
    action->targetSpeed=0;action->targetDuration=0;action->speedTolerance=0;
    action->useRoadSpeed=1;action->affectsRacePace=1;
    return action;
}

DriverAIAction* driverAIActionInitializeAvoidance(DriverAIAction* action,int32_t priority,
    float duration,CarMovement* target,float retainedArgument,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=0;
    action->behavior.avoidance.target=target;
    action->behavior.avoidance.retainedArgument=retainedArgument;
    action->behavior.avoidance.targetSelectedTicks=nowTicks;
    return action;
}

DriverAIAction* driverAIActionInitializeCollisionAvoidance(DriverAIAction* action,
    int32_t priority,float duration,CarMovement* target,float expiryBaseSeconds,
    int32_t pathClear,int32_t followTarget,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=1;
    action->behavior.collisionAvoidance.target=target;
    action->behavior.collisionAvoidance.pathClear=pathClear;
    action->behavior.collisionAvoidance.lastReplanTicks=nowTicks;
    action->behavior.collisionAvoidance.expiryReferenceTicks=nowTicks;
    action->behavior.collisionAvoidance.expiryBaseSeconds=expiryBaseSeconds;
    action->behavior.collisionAvoidance.expirySeconds=expiryBaseSeconds+2.0f;
    action->behavior.collisionAvoidance.followTarget=followTarget;
    return action;
}

DriverAIAction* driverAIActionInitializeRoadFollowing(DriverAIAction* action,
    int32_t priority,float duration,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=2;
    return action;
}

DriverAIAction* driverAIActionInitializeCruise(DriverAIAction* action,int32_t priority,float duration,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=5;
    action->behavior.cruise.lastProbeTicks=0;
    return action;
}

DriverAIAction* driverAIActionInitializeYielding(DriverAIAction* action,int32_t priority,float duration,
    const DriverAIPerception* perception,const DriverAIRoadState* road,float stopDelaySeconds,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=6;
    action->behavior.yielding.createdTicks=nowTicks;
    action->behavior.yielding.lastProbeTicks=0;
    action->behavior.yielding.collisionPathClear=1;
    action->behavior.yielding.stopDelaySeconds=stopDelaySeconds;
    action->behavior.yielding.rightMarginAvailable=perception&&road?
        2.0f<driverAIActionRightMargin(perception,road->recovery.recoveryPose):1;
    driverCommandAdvance(&action->command,NULL,nowTicks,0.0f);
    return action;
}

DriverAIAction* driverAIActionInitializeRecovery(DriverAIAction* action,int32_t priority,
    float duration,float requestedSeconds,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=7;
    action->behavior.recovery.createdTicks=nowTicks;
    action->behavior.recovery.requestedSeconds=requestedSeconds;
    action->behavior.recovery.hasRequestedDuration=!(fabsf(requestedSeconds-(-1.0f))<=0.0010000000474974513f);
    return action;
}

DriverAIAction* driverAIActionInitializeRoadBlock(DriverAIAction* action,int32_t priority,
    float duration,PursuitRoadBlock* block,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=8;
    action->behavior.roadBlock.block=block;
    return action;
}

DriverAIAction* driverAIActionInitializeContact(DriverAIAction* action,int32_t priority,
    float duration,CarMovement* target,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=9;
    action->behavior.contactResponse.target=target;
    action->behavior.contactResponse.lastReplanTicks=0;
    return action;
}

DriverAIAction* driverAIActionInitializeAirborne(DriverAIAction* action,int32_t priority,
    float duration,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=10;
    action->behavior.airborne.createdTicks=nowTicks;
    action->behavior.airborne.recoveryRequested=0;
    return action;
}

// PAL1C8028 /1C8160. These are real constructor branches in the source
// pointer restore, even though ordinary AIDriver selection does not use them.
DriverAIAction* driverAIActionInitializeNudge(DriverAIAction* action,int32_t priority,
    float duration,CarMovement* target,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=4;
    action->behavior.directTarget.target=target;
    return action;
}

DriverAIAction* driverAIActionInitializeBlock(DriverAIAction* action,int32_t priority,
    float duration,CarMovement* target,int32_t nowTicks)
{
    driverAIActionInitialize(action,priority,duration,nowTicks);
    action->kind=3;
    action->behavior.directTarget.target=target;
    return action;
}

namespace {

// PAL1C6088. Status is a raw low byte in the original stream, while the
// bool uses the original unsigned one-byte-to-four-byte overload.
void snapshotAIPlan(DriverAIAction& action,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"AIPlan");
    replaySnapshotMoveData(snapshot,&action.hasDeadline,1,4,0);
    replaySnapshotMoveData(snapshot,&action.startedTicks,4,-1,0);
    replaySnapshotMoveFloat(snapshot,&action.duration);
    replaySnapshotMoveData(snapshot,&action.status,1,-1,0);
    replaySnapshotEndSection(snapshot);
}

// PAL1C6198. Final contains three floats; beta contains only two.
void snapshotAIAction(DriverAIAction& action,const CarRecoveryPointRegistry* registry,
    ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"AIAction");
    driverCommandSnapshot(&action.command,registry,snapshot);
    replaySnapshotMoveFloat(snapshot,&action.targetSpeed);
    replaySnapshotMoveFloat(snapshot,&action.speedTolerance);
    replaySnapshotMoveFloat(snapshot,&action.targetDuration);
    replaySnapshotMoveData(snapshot,&action.useRoadSpeed,1,4,0);
    replaySnapshotMoveData(snapshot,&action.affectsRacePace,1,4,0);
    replaySnapshotMoveData(snapshot,&action.pathValid,1,4,0);
    replaySnapshotMoveData(snapshot,&action.enabled,1,4,0);
    replaySnapshotMoveInteger(snapshot,&action.priority,4);
    snapshotAIPlan(action,snapshot);
    replaySnapshotEndSection(snapshot);
}

// The released field holds Car*. Native actions borrow that same car's
// current movement owner; its replay identity is the signed driver number.
void snapshotActionCar(CarMovement*& movement,const World* world,ReplaySnapshot* snapshot)
{
    Car* car=movement?movement->bindings.owner:nullptr;
    worldSnapshotCarPointer(world,snapshot,&car);
    if(snapshot->restoring&&snapshot->result==REPLAY_SNAPSHOT_OK) {
        movement=car?car->Movement():nullptr;
        hp2::Require(!car||movement,"AIAction snapshot car has no movement owner");
    }
}

// Every case is the actual class-specific virtual at vtable+14, reached
// through the independently proved constructor table at final2B75A0.
// Common transfers are factored only here; there are no duplicate owners.
void snapshotConcreteAction(DriverAIAction& action,DriverAIBase& driver,ReplaySnapshot* snapshot)
{
    const auto* registry=driver.physics.movement->bindings.recoveryPoints;
    const World* world=driver.environment->simulation->world;
    switch(action.kind) {
    case 0: { // AIActionPass PAL1C7960..1C79D4
        replaySnapshotBeginSection(snapshot,nullptr,"AIActionPass");
        auto& pass=action.behavior.avoidance;
        replaySnapshotMoveData(snapshot,&pass.targetSelectedTicks,4,-1,0);
        replaySnapshotMoveFloat(snapshot,&pass.retainedArgument);
        snapshotActionCar(pass.target,world,snapshot);
        snapshotAIAction(action,registry,snapshot);
        break;
    }
    case 1: { // AIActionAvoidCars PAL1C6C48..1C6CF4
        replaySnapshotBeginSection(snapshot,nullptr,"AIActionAvoidCars");
        auto& avoid=action.behavior.collisionAvoidance;
        snapshotActionCar(avoid.target,world,snapshot);
        replaySnapshotMoveFloat(snapshot,&avoid.expiryBaseSeconds);
        replaySnapshotMoveData(snapshot,&avoid.pathClear,1,4,0);
        replaySnapshotMoveData(snapshot,&avoid.followTarget,1,4,0);
        replaySnapshotMoveData(snapshot,&avoid.lastReplanTicks,4,-1,0);
        replaySnapshotMoveData(snapshot,&avoid.expiryReferenceTicks,4,-1,0);
        replaySnapshotMoveFloat(snapshot,&avoid.expirySeconds);
        snapshotAIAction(action,registry,snapshot);
        break;
    }
    case 2: // AIActionDraft PAL1C76D0..1C771C
        replaySnapshotBeginSection(snapshot,nullptr,"AIActionDraft");
        snapshotAIAction(action,registry,snapshot);
        break;
    case 3: // AIActionBlock PAL1C81B0..1C820C
    case 4: // AIActionNudgeCar PAL1C8078..1C80D4
        replaySnapshotBeginSection(snapshot,nullptr,action.kind==3?"AIActionBlock":"AIActionNudgeCar");
        snapshotActionCar(action.behavior.directTarget.target,world,snapshot);
        snapshotAIAction(action,registry,snapshot);
        break;
    case 5: // AIActionJustDrive PAL1C7400..1C7458
        replaySnapshotBeginSection(snapshot,nullptr,"AIActionJustDrive");
        snapshotAIAction(action,registry,snapshot);
        replaySnapshotMoveData(snapshot,&action.behavior.cruise.lastProbeTicks,4,-1,0);
        break;
    case 6: { // AIActionPullOver PAL1C8370..1C8400
        replaySnapshotBeginSection(snapshot,nullptr,"AIActionPullOver");
        snapshotAIAction(action,registry,snapshot);
        auto& yielding=action.behavior.yielding;
        replaySnapshotMoveData(snapshot,&yielding.createdTicks,4,-1,0);
        replaySnapshotMoveData(snapshot,&yielding.lastProbeTicks,4,-1,0);
        replaySnapshotMoveData(snapshot,&yielding.collisionPathClear,1,4,0);
        replaySnapshotMoveData(snapshot,&yielding.rightMarginAvailable,1,4,0);
        replaySnapshotMoveFloat(snapshot,&yielding.stopDelaySeconds);
        break;
    }
    case 7: { // AIActionGetUnstuck PAL1C8BB8..1C8C2C
        replaySnapshotBeginSection(snapshot,nullptr,"AIActionGetUnstuck");
        auto& recovery=action.behavior.recovery;
        replaySnapshotMoveData(snapshot,&recovery.createdTicks,4,-1,0);
        replaySnapshotMoveFloat(snapshot,&recovery.requestedSeconds);
        replaySnapshotMoveData(snapshot,&recovery.hasRequestedDuration,1,4,0);
        snapshotAIAction(action,registry,snapshot);
        break;
    }
    case 8: // AIActionAvoidRoadBlock PAL1C96B8..1C9714
        replaySnapshotBeginSection(snapshot,nullptr,"AIActionAvoidRoadBlock");
        snapshotAIAction(action,registry,snapshot);
        pursuitManagerSnapshotRoadBlock(driver.environment->simulation->pursuit,
            snapshot,&action.behavior.roadBlock.block);
        break;
    case 9: // AIActionIJustGotHit PAL1C9CB8..1C9D20
        replaySnapshotBeginSection(snapshot,nullptr,"AIActionIJustGotHit");
        snapshotAIAction(action,registry,snapshot);
        snapshotActionCar(action.behavior.contactResponse.target,world,snapshot);
        replaySnapshotMoveData(snapshot,&action.behavior.contactResponse.lastReplanTicks,4,-1,0);
        break;
    case 10: // AIActionAccidentRecovery PAL1CA240..1CA2A8
        // Final deliberately uses this same section name for both classes.
        replaySnapshotBeginSection(snapshot,nullptr,"AIActionIJustGotHit");
        snapshotAIAction(action,registry,snapshot);
        replaySnapshotMoveData(snapshot,&action.behavior.airborne.createdTicks,4,-1,0);
        replaySnapshotMoveData(snapshot,&action.behavior.airborne.recoveryRequested,1,4,0);
        break;
    default:
        hp2::Require(false,"AIAction snapshot kind is outside the released table");
    }
    replaySnapshotEndSection(snapshot);
}

} // namespace

// PAL1C6268..1C64E8, whole 11-kind pointer overload. The original allocator
// is a console pool; the native owner is the existing malloc/free action.
// Publish a restored allocation only after its real body transfer succeeds.
void driverAIActionSnapshot(DriverAIBase* driver,ReplaySnapshot* snapshot)
{
    hp2::Require(driver&&snapshot&&driver->physics.movement&&driver->environment&&
        driver->environment->simulation&&driver->environment->simulation->world,
        "AIAction snapshot has no live driver/world owner");
    replaySnapshotBeginSection(snapshot,nullptr,"AIActionPtr");
    int8_t present=driver->action?1:0;
    replaySnapshotMoveData(snapshot,&present,1,1,1);
    if(!present||snapshot->result!=REPLAY_SNAPSHOT_OK) {
        // Source false branch intentionally leaves an existing pointer alone.
        replaySnapshotEndSection(snapshot);
        return;
    }
    int32_t kind=driver->action?driver->action->kind:0;
    replaySnapshotMoveData(snapshot,&kind,1,-1,0);
    if(snapshot->result!=REPLAY_SNAPSHOT_OK) {
        replaySnapshotEndSection(snapshot);
        return;
    }
    hp2::Require(kind>=0&&kind<11,"AIAction snapshot kind is outside the released table");
    using ActionOwner=std::unique_ptr<DriverAIAction,decltype(&std::free)>;
    ActionOwner restored(nullptr,&std::free);
    DriverAIAction* action=driver->action;
    if(snapshot->restoring) {
        restored.reset(static_cast<DriverAIAction*>(std::calloc(1,sizeof(DriverAIAction))));
        hp2::Require(restored!=nullptr,"AIAction snapshot allocation failed");
        action=restored.get();
        const auto* environment=driver->physics.movement->bindings.environment;
        hp2::Require(environment&&environment->clock,"AIAction restore has no live world clock");
        const int32_t now=environment->clock->worldTimer;
        switch(kind) {
        case 0:driverAIActionInitializeAvoidance(action,0,0.0f,nullptr,0.0f,now);break;
        case 1:driverAIActionInitializeCollisionAvoidance(action,0,0.0f,nullptr,0.0f,0,0,now);break;
        case 2:driverAIActionInitializeRoadFollowing(action,0,0.0f,now);break;
        case 3:driverAIActionInitializeBlock(action,0,0.0f,nullptr,now);break;
        case 4:driverAIActionInitializeNudge(action,0,0.0f,nullptr,now);break;
        case 5:driverAIActionInitializeCruise(action,0,0.0f,now);break;
        case 6:driverAIActionInitializeYielding(action,0,0.0f,nullptr,nullptr,0.0f,now);break;
        case 7:driverAIActionInitializeRecovery(action,0,0.0f,-1.0f,now);break;
        case 8:driverAIActionInitializeRoadBlock(action,0,0.0f,nullptr,now);break;
        case 9:driverAIActionInitializeContact(action,0,0.0f,nullptr,now);break;
        case 10:driverAIActionInitializeAirborne(action,0,0.0f,now);break;
        }
    }
    if(action)snapshotConcreteAction(*action,*driver,snapshot);
    replaySnapshotEndSection(snapshot);
    if(snapshot->restoring&&snapshot->result==REPLAY_SNAPSHOT_OK) {
        // Main World restore has already recreated the controller, so this
        // is normally null. Direct native restores still have one owner.
        std::free(driver->action);
        driver->action=restored.release();
    }
}

void driverAIActionSetTimeout(DriverAIAction* action,float duration,int32_t nowTicks)
{
    action->hasDeadline=duration>=0;
    action->duration=duration;
    const float seconds=(float)nowTicks*.0002500000118743628f;
    action->startedTicks=convertFloatToInt32(seconds*4000+.5f);
}
int32_t driverAIActionDeadlinePassed(const DriverAIAction* action,int32_t nowTicks)
{
    const int32_t elapsed=(int32_t)((uint32_t)nowTicks-(uint32_t)action->startedTicks);
    return action->duration<(float)elapsed*.0002500000118743628f && action->hasDeadline!=0;
}
int32_t driverAIActionFinished(const DriverAIAction* action)
{
    return action->status==DRIVER_AI_ACTION_SUCCEEDED || action->status==DRIVER_AI_ACTION_FAILED || driverAIActionTimedOut(action);
}
void driverAIActionUpdateStatus(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    action->status=DRIVER_AI_ACTION_ACTIVE;
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    int32_t timedOut=0;
    switch(action->kind){
    case 0:case 2:case 5:case 9:case 10:
        timedOut=driverAIActionDeadlinePassed(action,now);break;
    case 3:case 4:
        timedOut=driverAIActionDeadlinePassed(action,now)&&!action->pathValid;break;
    case 7:timedOut=driverAIActionRecoveryTimedOut(action,driver);break;
    case 1:case 6:case 8:break;
    default:driver->result=CAR_DRIVER_INVALID_BINDING;return;
    }
    if(timedOut)action->status=DRIVER_AI_ACTION_TIMED_OUT;
    int32_t failed=0;
    if(action->kind==0)failed=driverAIActionAvoidanceFailed(action,driver,perception);
    else if(action->kind==7)failed=driverAIActionRecoveryFailed(action,driver,perception);
    if(failed)action->status=DRIVER_AI_ACTION_FAILED;
    int32_t complete=0;
    switch(action->kind){
    case 0:complete=driverAIActionAvoidanceComplete(action,driver,perception);break;
    case 1:complete=driverAIActionCollisionAvoidanceComplete(action,perception,now);break;
    case 6:complete=driverAIActionYieldingComplete(driver);break;
    case 7:complete=driverAIActionRecoveryComplete(action,driver,perception);break;
    case 8:complete=driverAIActionRoadBlockComplete(action,perception);break;
    case 9:complete=driverAIActionContactComplete(action,driver,perception);break;
    case 10:complete=driverAIActionAirborneComplete(action,perception);break;
    case 3:case 4:complete=1;break;
    case 2:case 5:break;
    }
    if(complete)action->status=DRIVER_AI_ACTION_SUCCEEDED;
}
void driverAIActionBuildPlan(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    switch(action->kind){
    case 3:case 4:action->pathValid=1;break;
    case 0:driverAIActionPlanAvoidance(action,driver,perception);break;
    case 1:driverAIActionPlanCollisionAvoidance(action,driver,perception);break;
    case 2:driverAIActionPlanRoadFollowing(action,driver,perception);break;
    case 5:driverAIActionPlanCruise(action,driver,perception);break;
    case 6:driverAIActionPlanYielding(action,driver,perception);break;
    case 7:driverAIActionPlanRecovery(action,driver,perception);break;
    case 8:driverAIActionPlanRoadBlock(action,driver,perception);break;
    case 9:driverAIActionPlanContact(action,driver,perception);break;
    case 10:driverAIActionPlanAirborne(action,driver,perception);break;
    default:driver->result=CAR_DRIVER_INVALID_BINDING;break;
    }
}
#include "indep/inc/PsychoDriver.hpp"
#include <math.h>
static float lateralAt(const CarRecoveryPose* pose,const float point[2])
{
    const float x=point[0]-pose->position[0];
    const float y=point[1]-pose->position[1];
    const float first=x*pose->direction[1];
    const float second=y*pose->direction[0];
    return first-second;
}
void driverAIActionAccelerateFor(DriverAIAction* action,const DriverPhysicsVehicle* vehicle,float seconds)
{
    const float speed=vehicle->movement->bindings.state->speed;
    const float acceleration=driverAvailableAcceleration(vehicle,speed);
    const float change=acceleration*seconds;
    action->targetDuration=seconds;
    action->targetSpeed=speed+change;
}
int32_t driverAIActionRoadBoundary(const DriverAIRoadState* road,const DriverAIBase* driver,
    int32_t considerTargetPoint)
{
    const VehicleState* state=driver->physics.movement->bindings.state;
    const CarRecoveryPose* pose=road->recovery.recoveryPose;
    const float currentLateral=lateralAt(pose,state->position);
    const float left=pose->halfWidths[0]+currentLateral;
    const float right=pose->halfWidths[1]-currentLateral;
    int32_t result=0;
    float predicted[2];
    if(1.0f<fabsf(state->speed)){
        vectorMultiplyAdd2(predicted,state->position,state->velocity,0.10000000149011612f);
        const float nextLateral=lateralAt(pose,predicted);
        const float nextLeft=pose->halfWidths[0]+nextLateral;
        const float nextRight=pose->halfWidths[1]-nextLateral;
        const float difference=nextLeft-left;
        const float rate=difference*10.0f;
        float seconds=99999.0f;
        if(0.0010000000474974513f<rate&&0.0f<right&&0.0f<left)seconds=finiteRatio(right,rate);
        if(seconds<=1.0f||(right<0.0f&&nextRight<right)||(left<0.0f&&nextLeft<left))
            result=0.0f<state->forwardSpeed?1:2;
    }else{
        const float direction[2]={vectorCosineAngle(state->heading),rotationSineAngle(state->heading)};
        vectorMultiplyAdd2(predicted,state->position,direction,2.0f);
        const float nextLateral=lateralAt(pose,predicted);
        const float nextLeft=pose->halfWidths[0]+nextLateral;
        const float nextRight=pose->halfWidths[1]-nextLateral;
        const float rightChange=nextRight-right;
        const float leftChange=nextLeft-left;
        if((nextRight<0.0f&&rightChange<0.0f)||(nextLeft<0.0f&&leftChange<0.0f))result=1;
    }
    if(result==1&&considerTargetPoint&&driverCommandPointAhead(driver->controls->targetPoint,state,0.5f))result=0;
    if(result==2){
        float heading[2],roadDirection[2];
        vectorNormalize2(heading,state->orientation);
        vectorNormalize2(roadDirection,pose->direction);
        const float first=heading[0]*roadDirection[0];
        const float second=heading[1]*roadDirection[1];
        if(0.34200000762939453f<first+second)result=0;
    }
    return result;
}
int32_t driverAIActionContactSide(const CollisionImpact* impact,const DriverPhysicsVehicle* vehicle)
{
    float minimum[4],maximum[4],point[2];
    driverCollisionBounds(vehicle,minimum,maximum);
    driverPerceptionLocalPoint(vehicle->movement->bindings.state,point,impact->point);
    const float upper=maximum[1]*0.949999988079071f;
    const float lower=minimum[1]*0.949999988079071f;
    if(point[0]<upper&&lower<point[0])return 0.0f<point[1]?0:1;
    return 0.0f<point[0]?3:2;
}
#include "indep/inc/DrivingChoice.hpp"
int32_t driverAIActionFacesGround(CarMovement* movement,float threshold)
{
    const VehicleState* state=movement->bindings.state;
    float normal[4];
    terrainSurfaceElevation(movement->bindings.environment->contact.terrain,state->position,NULL,normal,NULL);
    const float* up=&state->orientation[8];
    const float xx=up[0]*up[0];
    const float yy=up[1]*up[1];
    const float zz=up[2]*up[2];
    const float length=sqrtf((xx+yy)+zz);
    const float inverse=finiteRatio(1.0f,length);
    const float x=up[0]*inverse;
    const float y=up[1]*inverse;
    const float z=up[2]*inverse;
    const float first=x*normal[0];
    const float second=y*normal[1];
    const float third=z*normal[2];
    return threshold<(first+second)+third;
}
int32_t driverAIActionAirbornePriority(DriverAIPerception* perception)
{
    CarMovement* movement=perception->movement;
    return movement->bindings.state->activeContactCount<2&&
        !driverAIActionFacesGround(movement,0.7070000171661377f)?500:-1;
}
int32_t driverAIActionAirborneComplete(const DriverAIAction* action,DriverAIPerception* perception)
{
    if(action->behavior.airborne.recoveryRequested)return 1;
    CarMovement* movement=perception->movement;
    return movement->bindings.state->activeContactCount==4&&
        driverAIActionFacesGround(movement,0.7070000171661377f);
}
void driverAIActionPlanAirborne(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    DriverAIGoal* goal=&perception->goal;
    DriverAIRoadState* road=&perception->road;
    if(!driverAIGoalUsesRoad(goal->kind))driverAIGoalSet(goal,driverLaneSelectorForIndex(road->lane),NULL,0);
    driverLanePlanGoal(goal,&action->command,road,perception,driver,1);
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    const int32_t elapsed=(int32_t)((uint32_t)now-(uint32_t)action->behavior.airborne.createdTicks);
    const float seconds=(float)elapsed*0.0002500000118743628f;
    action->targetDuration=0.0f;action->useRoadSpeed=0;action->targetSpeed=0.0f;
    action->speedTolerance=0.0f;action->pathValid=1;action->enabled=0;
    if(5.0f<=seconds&&!action->behavior.airborne.recoveryRequested){
        CarMovement* movement=perception->movement;
        carMotionRequestRecovery(movement->bindings.ownerMotion,&movement->bindings.environment->recovery);
        action->behavior.airborne.recoveryRequested=1;
    }
}
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Race.hpp"
static float secondsSince(int32_t now,int32_t previous)
{
    const int32_t elapsed=(int32_t)((uint32_t)now-(uint32_t)previous);
    return (float)elapsed*0.0002500000118743628f;
}
int32_t driverAIActionAvoidanceTargetAllowed(const DriverAINeighbor* neighbor,
    const DriverAIGoal* goal,int32_t followTarget)
{
    const CarMovement* target=neighbor->movement;
    if(target->bindings.ownerMotion->inputCategory==CAR_DRIVER_TRAFFIC||
       !(20.0f<target->bindings.state->speed))return 1;
    return driverAIGoalUsesRoad(goal->kind)||followTarget!=0;
}
int32_t driverAIActionAvoidancePriority(DriverAIBase* driver,DriverAIPerception* perception,
    CarMovement** target,DriverAIGoal* goal)
{
    if(raceFrameConnectionWindowActive(driver->environment->simulation->race,
       perception->movement->bindings.owner,&raceConnectionDefaultTiming))return -1;
    const float acceleration=driverAvailableAcceleration(&driver->physics,perception->movement->bindings.state->speed);
    const int32_t limit=driverPerceptionFindIndex(perception,perception->movement);
    for(int32_t index=0;index<limit;++index){
        const DriverAINeighbor* neighbor=&perception->neighbors[index];
        const float forward=neighbor->localPosition[1];
        if(!(0.0f<forward)||!(neighbor->localPosition[0]<forward+forward)||
           !driverPerceptionNeighborCanReach(neighbor,&driver->physics,acceleration))continue;
        DriverCommand alternative;
        driverCommandInitialize(&alternative);
        *goal=perception->goal;
        int32_t follow=0;
        const int32_t clear=driverLaneAvoidNeighbor(goal,&alternative,&follow,neighbor,&perception->road,
            perception,driver,1,3.0f);
        if(clear&&driverAIActionAvoidanceTargetAllowed(neighbor,goal,follow)){
            *target=neighbor->movement;
            return 300;
        }
    }
    return -1;
}
int32_t driverAIActionAvoidanceComplete(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    if(0<perception->count&&perception->neighbors[0].movement!=action->behavior.avoidance.target){
        CarMovement* candidate=NULL;
        DriverAIGoal goal;
        driverAIGoalInitialize(&goal);
        if(0<=driverAIActionAvoidancePriority(driver,perception,&candidate,&goal)&&
           !driverLaneGoalsMatch(&perception->goal,&goal)){
            perception->goal=goal;
            action->behavior.avoidance.target=candidate;
            action->behavior.avoidance.targetSelectedTicks=now;
        }
    }
    int32_t complete=0;
    if(!(secondsSince(now,action->behavior.avoidance.targetSelectedTicks)<3.0f)){
        const DriverAINeighbor* neighbor=driverPerceptionFind(perception,action->behavior.avoidance.target);
        if(!neighbor)complete=1;
        else{
            const float extent=perception->forwardExtent+neighbor->effectiveHalfExtents[1];
            const float boundary=-(extent+10.0f);
            if(neighbor->localPosition[1]<boundary)complete=1;
        }
    }
    if(complete)driverAIRoadMarkSelected(&perception->road,1,now);
    return complete;
}
int32_t driverAIActionAvoidanceFailed(const DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    const DriverAINeighbor* neighbor=driverPerceptionFind(perception,action->behavior.avoidance.target);
    int32_t failed=!neighbor;
    if(neighbor&&secondsSince(now,action->behavior.avoidance.targetSelectedTicks)<3.0f&&
       0.0f<neighbor->localPosition[1])failed=!driverPerceptionNeighborCanReach(neighbor,&driver->physics,-1.0f);
    if(failed)driverAIRoadMarkSelected(&perception->road,1,now);
    return failed;
}
void driverAIActionPlanAvoidance(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    DriverAIRoadState* road=&perception->road;
    DriverAIGoal* goal=&perception->goal;
    driverCommandAdvance(&action->command,NULL,now,0.0f);
    action->useRoadSpeed=0;action->enabled=1;action->affectsRacePace=1;action->pathValid=1;
    const DriverAINeighbor* neighbor=driverPerceptionFind(perception,action->behavior.avoidance.target);
    if(neighbor){
        if(0.0f<neighbor->localPosition[1]){
            int32_t follow=0;
            driverLaneAvoidNeighbor(goal,&action->command,&follow,neighbor,road,perception,driver,1,
                driver->environment->tuning->laneCollisionHorizon);
        }else{
            if(!driverAIGoalUsesRoad(goal->kind))driverAIGoalSet(goal,driverLaneSelectorForIndex(road->lane),NULL,0);
            float speed=0.0f,time=99999.0f;
            CarMovement* collision=NULL;
            driverLaneEvaluateGoal(goal,&action->command,road,perception,driver,1,0,
                driver->environment->tuning->laneCollisionHorizon,&speed,&collision,&time);
        }
    }else{
        if(!driverAIGoalUsesRoad(goal->kind))driverAIGoalSet(goal,driverLaneSelectorForIndex(road->lane),NULL,0);
        driverLanePlanGoal(goal,&action->command,road,perception,driver,1);
        action->pathValid=0;
    }
    action->useRoadSpeed=1;
}
#include "indep/inc/CarCollisionDetect.hpp"
static int32_t connectionWindow(const DriverAIBase* driver,const CarMovement* movement)
{
    return raceFrameConnectionWindowActive(driver->environment->simulation->race,
        movement->bindings.owner,&raceConnectionDefaultTiming);
}
void driverAIActionMatchNeighborSpeed(DriverAIAction* action,const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception,float speedReduction)
{
    const float speed=neighbor->movement->bindings.state->speed-speedReduction;
    action->targetSpeed=maximumFinite(speed,0.0f);
    const float centerGap=neighbor->localPosition[1]-perception->forwardExtent;
    const float gap=centerGap-neighbor->effectiveHalfExtents[1];
    const float duration=finiteRatio(gap,neighbor->travelRelativeVelocity[1]);
    action->targetDuration=maximumFinite(duration,0.10000000149011612f);
}
int32_t driverAIActionCollisionAvoidancePriority(DriverAIBase* driver,DriverAIPerception* perception,
    CarMovement** target,DriverAIGoal* goal,float* collisionTime,int32_t* pathClear,int32_t* followTarget)
{
    DriverCommand alternative;
    driverCommandInitialize(&alternative);
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    driverCommandAdvance(&alternative,NULL,now,0.0f);
    *collisionTime=99999.0f;*pathClear=0;*followTarget=0;
    float time=99999.0f;
    CarMovement* collision=driverCollisionPredictNeighbors(driver,&driver->controls->command,
        perception,&perception->road,driver->environment->tuning->laneCollisionHorizon,
        &time,driver->environment->simulation->world);
    if(!collision||!(time<driver->environment->tuning->laneCollisionHorizon))return -1;
    *target=collision;*collisionTime=time;
    *goal=perception->goal;
    if(connectionWindow(driver,perception->movement)&&!collision->bindings.player){
        *pathClear=1;*followTarget=0;
    }else{
        *pathClear=driverLaneSelectGoal(goal,&alternative,&perception->road,perception,driver,
            followTarget,1,driver->environment->tuning->laneCollisionHorizon);
    }
    return 400;
}
int32_t driverAIActionCollisionAvoidanceComplete(const DriverAIAction* action,
    DriverAIPerception* perception,int32_t nowTicks)
{
    const float elapsed=secondsSince(nowTicks,action->behavior.collisionAvoidance.expiryReferenceTicks);
    const int32_t complete=!(elapsed<=action->behavior.collisionAvoidance.expirySeconds);
    if(complete)driverAIRoadMarkSelected(&perception->road,1,nowTicks);
    return complete;
}
void driverAIActionPlanCollisionAvoidance(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    DriverAIGoal* goal=&perception->goal;
    DriverAIRoadState* road=&perception->road;
    DriverAINeighbor* goalNeighbor=driverPerceptionFind(perception,goal->target);
    DriverAINeighbor* target=driverPerceptionFind(perception,action->behavior.collisionAvoidance.target);
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    driverCommandCopy(&action->command,&driver->controls->command);
    action->speedTolerance=0.0f;action->enabled=1;action->useRoadSpeed=1;
    const int32_t stationary=target&&perception->movement->bindings.state->speed<10.0f&&
        target->movement->bindings.state->speed<5.0f;
    int32_t replan=!connectionWindow(driver,perception->movement)||
        action->behavior.collisionAvoidance.target->bindings.player!=NULL;
    replan=replan&&0.30000001192092896f<secondsSince(now,action->behavior.collisionAvoidance.lastReplanTicks);
    if(replan){
        if(driverAIGoalUsesRoad(goal->kind)&&stationary){
            driverAIGoalSet(goal,5,target->movement,0);
            if(!driverLaneTargetHasRoom(goal,target,perception))driverAIGoalSet(goal,6,target->movement,0);
        }
        action->behavior.collisionAvoidance.followTarget=0;
        action->behavior.collisionAvoidance.pathClear=driverLaneSelectGoal(goal,&action->command,road,
            perception,driver,&action->behavior.collisionAvoidance.followTarget,1,
            driver->environment->tuning->laneCollisionHorizon);
        if(driverAIGoalUsesRoad(goal->kind)){
            const int32_t lane=driverAILaneForClass(goal->kind,driver->physics.routeVehicleClass);
            if(lane!=road->lane)driverAIRoadSetLane(road,lane,0,now);
            goalNeighbor=NULL;
        }else if(driverAIGoalUsesTarget(goal->kind))goalNeighbor=driverPerceptionFind(perception,goal->target);
        action->behavior.collisionAvoidance.lastReplanTicks=now;
    }
    if(!action->command.count||!replan||!goal->kind){
        if(!goal->kind||!driverLaneGoalAllowed(goal,road,perception,driver->physics.routeVehicleClass)){
            action->behavior.collisionAvoidance.pathClear=0;
            driverAIGoalSet(goal,driverLaneSelectorForIndex(road->lane),NULL,0);
        }
        float speed=-1.0f,time=99999.0f;
        CarMovement* collision=NULL;
        action->behavior.collisionAvoidance.pathClear=driverLaneEvaluateGoal(goal,&action->command,road,
            perception,driver,0,action->behavior.collisionAvoidance.followTarget,
            driver->environment->tuning->laneCollisionHorizon,&speed,&collision,&time);
        if(!action->command.count){
            driverLanePlanGoal(goal,&action->command,road,perception,driver,1);
            action->behavior.collisionAvoidance.pathClear=0;
        }
    }
    if(!action->behavior.collisionAvoidance.pathClear&&
       (driverCollisionNeighborEligible(target,perception)||driverCollisionNeighborEligible(goalNeighbor,perception))){
        DriverAINeighbor* candidate=target&&!carMotionWindowActive(target->movement->bindings.ownerMotion,now)?target:NULL;
        if(candidate){
            const CarMovement* movement=candidate->movement;
            float longitudinal=candidate->localPosition[1],minimum=0.0f;
            if(movement->bindings.ownerMotion->inputCategory==CAR_DRIVER_REAL){
                const float change=candidate->travelRelativeVelocity[1]*0.10000000149011612f;
                longitudinal=longitudinal+change;
                minimum=perception->forwardExtent*0.5f;
            }
            int32_t constrain=minimum<longitudinal;
            const float halfWidth=perception->lateralExtent+candidate->effectiveHalfExtents[0];
            if(!(fabsf(candidate->localPosition[0])<halfWidth+1.0f)&&!movement->bindings.player)constrain=0;
            if(constrain){
                action->useRoadSpeed=0;
                if(stationary)action->useRoadSpeed=1;
                else driverAIActionMatchNeighborSpeed(action,candidate,perception,0.10000000149011612f);
                const int32_t group=movement->bindings.owner->driverInfo->driverClass;
                if(group==3||group==4)action->targetSpeed=maximumFinite(action->targetSpeed,20.0f);
            }
        }
    }
    action->pathValid=1;
}
#include "indep/inc/Collision.hpp"
static DriverAINeighbor* findCar(DriverAIPerception* perception,const Car* car)
{
    for(int32_t index=0;index<perception->count;++index){
        DriverAINeighbor* neighbor=&perception->neighbors[index];
        if(neighbor->movement->bindings.owner==car)return neighbor;
    }
    return NULL;
}
int32_t driverAIActionContactPriority(DriverAIBase* driver,DriverAIPerception* perception,
    CarMovement** target)
{
    *target=NULL;
    const CarMovement* movement=perception->movement;
    const CollisionShape* shape=movement->bindings.ownerMotion->collision->shape;
    DriverAINeighbor* best=NULL;
    CarMovement* selected=NULL;
    for(int32_t index=0;index<shape->contactCount;++index){
        const CollisionImpact* impact=collisionShapeContactAt(driver->environment->simulation->collisions,shape,index);
        if(!impact){driver->result=CAR_DRIVER_INVALID_BINDING;return -1;}
        if(!collisionImpactIsCarPair(impact))continue;
        Car* first;
        Car* second;
        collisionImpactGetCars(impact,&first,&second);
        const Car* other=first==movement->bindings.owner?second:first;
        DriverAINeighbor* neighbor=findCar(perception,other);
        if(!neighbor)continue;
        int32_t choose=!best||!selected;
        if(best&&selected){
            const float previous=best->localPosition[1];
            const float current=neighbor->localPosition[1];
            choose=(previous<0.0f&&0.0f<current)||current<previous;
        }
        if(choose){best=neighbor;selected=neighbor->movement;}
    }
    if(!selected)return -1;
    *target=selected;
    return 400;
}
int32_t driverAIActionContactComplete(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    CarMovement* target=NULL;
    const int32_t complete=driverAIActionContactPriority(driver,perception,&target)==-1;
    if(!complete)action->behavior.contactResponse.target=target;
    return complete||!action->behavior.contactResponse.target||
        !driverPerceptionFind(perception,action->behavior.contactResponse.target);
}
void driverAIActionPlanContact(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    DriverAINeighbor* neighbor=driverPerceptionFind(perception,action->behavior.contactResponse.target);
    DriverAIRoadState* road=&perception->road;
    DriverAIGoal* goal=&perception->goal;
    const CarMovement* own=perception->movement;
    const int32_t now=own->bindings.environment->clock->worldTimer;
    driverCommandCopy(&action->command,&driver->controls->command);
    const int32_t elapsed=(int32_t)((uint32_t)now-(uint32_t)action->behavior.contactResponse.lastReplanTicks);
    const float seconds=(float)elapsed*0.0002500000118743628f;
    action->enabled=1;action->speedTolerance=0.0f;action->useRoadSpeed=0;
    if(0.20000000298023224f<seconds||!action->command.count){
        action->behavior.contactResponse.lastReplanTicks=now;
        int32_t follow=0;
        driverLaneSelectGoal(goal,&action->command,road,perception,driver,&follow,1,
            driver->environment->tuning->laneCollisionHorizon);
    }
    if(driverAIActionRoadBoundary(road,driver,0)==1)driverAIActionBrakeFor(action,&driver->physics,0.20000000298023224f);
    else if(!neighbor)action->useRoadSpeed=1;
    else{
        const CarMovement* target=neighbor->movement;
        const int32_t ownGroup=own->bindings.owner->driverInfo->driverClass;
        const float longitudinal=neighbor->localPosition[1];
        if(target->bindings.player&&ownGroup!=3){
            const float extent=perception->forwardExtent+neighbor->effectiveHalfExtents[1];
            const float front=extent-2.799999952316284f;
            const float rear=(-perception->forwardExtent-neighbor->effectiveHalfExtents[1])+2.799999952316284f;
            if(front<longitudinal)driverAIActionMatchNeighborSpeed(action,neighbor,perception,6.0f);
            else if(longitudinal<rear)driverAIActionAccelerateFor(action,&driver->physics,0.20000000298023224f);
            else action->useRoadSpeed=1;
        }else if(0.0f<longitudinal){
            const float first=own->bindings.state->orientation[0]*target->bindings.state->orientation[0];
            const float second=own->bindings.state->orientation[1]*target->bindings.state->orientation[1];
            if(ownGroup==3&&first+second<0.0f){
                action->targetSpeed=own->bindings.state->speed;
                action->targetDuration=0.0f;action->enabled=0;
            }else driverAIActionMatchNeighborSpeed(action,neighbor,perception,0.10000000149011612f);
        }else driverAIActionAccelerateFor(action,&driver->physics,0.20000000298023224f);
    }
    if(!action->useRoadSpeed&&action->targetSpeed<20.0f)action->targetSpeed=20.0f;
    action->pathValid=1;
}
void driverAIActionPlanCruise(DriverAIAction* action,DriverAIBase* driver,DriverAIPerception* perception)
{
    DriverAIRoadState* road=&perception->road;
    DriverAIGoal* goal=&perception->goal;
    const int32_t now=driver->physics.movement->bindings.environment->clock->worldTimer;
    const int32_t elapsed=(int32_t)((uint32_t)now-(uint32_t)action->behavior.cruise.lastProbeTicks);
    int32_t planned=0;
    if(3.0f<=(float)elapsed*.0002500000118743628f){
        action->behavior.cruise.lastProbeTicks=now;
        if(goal->kind!=1){
            DriverCommand command;
            DriverAIGoal candidate;
            driverCommandInitialize(&command);
            driverAIGoalInitialize(&candidate);
            driverAIGoalSet(&candidate,1,NULL,0);
            float speed=0,time=99999;
            CarMovement* collision=NULL;
            if(driverLaneEvaluateRoadGoal(&candidate,&command,road,perception,driver,0,
                    driver->environment->tuning->laneCollisionHorizon,&speed,&collision,&time)){
                *goal=candidate;
                driverAIRoadSetLane(road,driverAILaneForClass(1,driver->physics.routeVehicleClass),1,now);
                driverCommandCopy(&action->command,&command);
                planned=1;
            }
        }
    }
    if(!planned){
        if(!driverAIGoalUsesRoad(goal->kind)){
            driverAIGoalSet(goal,1,NULL,0);
            driverAIRoadSetLane(road,driverAILaneForClass(1,driver->physics.routeVehicleClass),1,now);
        }
        driverLanePlanRoadGoal(goal,&action->command,road,perception,driver,0);
    }
    action->speedTolerance=0;
    action->pathValid=1;
    action->useRoadSpeed=1;
    action->enabled=1;
    action->affectsRacePace=1;
}
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/CameraMover.hpp"
static float driverAIRecoveryActionSecondsSince(int32_t now,int32_t previous)
{
    const int32_t ticks=(int32_t)((uint32_t)now-(uint32_t)previous);
    return (float)ticks*0.0002500000118743628f;
}
static void finishRecovery(DriverAIBase* driver)
{
    driverAIRefreshRoad(driver);
    aiControlsResetAngleFilters(driver->controls);
}
int32_t driverAIActionRecoveryBlocked(const DriverAIBase* driver,const DriverAIPerception* perception)
{
    const CarMovement* own=perception->movement;
    const int32_t now=own->bindings.environment->clock->worldTimer;
    if(carMotionWindowActive(own->bindings.ownerMotion,now)||!(own->bindings.state->speed<=15.0f))return 0;
    for(int32_t index=0;index<perception->count;++index){
        const DriverAINeighbor* neighbor=&perception->neighbors[index];
        if(neighbor->movement==own||!(fabsf(neighbor->movement->bindings.state->speed)<=
           driver->environment->tuning->stationaryPassSpeed))continue;
        const float width=perception->lateralExtent+neighbor->effectiveHalfExtents[0];
        if(!(fabsf(neighbor->localPosition[0])<width)||!(0.0f<neighbor->localPosition[1]))continue;
        const float extent=perception->forwardExtent+neighbor->effectiveHalfExtents[1];
        if(neighbor->localPosition[1]<extent+9.0f)return 1;
    }
    return 0;
}
int32_t driverAIActionRecoveryPriority(DriverAIBase* driver,DriverAIPerception* perception)
{
    CarMovement* own=perception->movement;
    DriverAIRoadState* road=&perception->road;
    if(raceFrameConnectionWindowActive(driver->environment->simulation->race,
       own->bindings.owner,&raceConnectionDefaultTiming)||driverAIRoadEmpty(road)||
       own->bindings.state->activeContactCount<3)return -1;
    const CarRecoveryPose* pose=road->recovery.recoveryPose;
    const float x=own->bindings.state->position[0]-pose->position[0];
    const float y=own->bindings.state->position[1]-pose->position[1];
    const float first=x*pose->direction[1];
    const float second=y*pose->direction[0];
    const float side=first-second;
    const float outside=0.0f<side?side-pose->halfWidths[1]:-(side+pose->halfWidths[0]);
    int32_t selected=20.0f<=outside;
    if(!selected){
        const int32_t kind=own->bindings.ownerMotion->inputCategory;
        const float speed=own->bindings.state->speed;
        if((kind==CAR_DRIVER_AI&&speed<=25.0f)||(kind==CAR_DRIVER_TRAFFIC&&speed<=20.0f))
            selected=driverAIActionRoadBoundary(road,driver,1)==1;
        if(!selected)selected=driverAIActionRecoveryBlocked(driver,perception);
    }
    const CollisionShape* shape=own->bindings.ownerMotion->collision->shape;
    const int32_t now=own->bindings.environment->clock->worldTimer;
    for(int32_t index=0;index<shape->contactCount&&!selected;++index){
        const CollisionImpact* impact=collisionShapeContactAt(driver->environment->simulation->collisions,shape,index);
        if(!impact){driver->result=CAR_DRIVER_INVALID_BINDING;return -1;}
        if(driverAIActionContactSide(impact,&driver->physics)!=0)continue;
        if(0.4000000059604645f<=driverAIRecoveryActionSecondsSince(now,impact->createdTicks)){
            const float speed=own->bindings.state->speed;
            if(speed<1.0f||fabsf(speed-1.0f)<=0.0010000000474974513f)selected=1;
        }
    }
    if(!selected)return -1;
    driverAIRefreshRoad(driver);
    return 500;
}
static float authoredLateral(const CarRecoveryPose* pose,int32_t lane)
{
    return (float)((int32_t)pose->laneOffsetsQuantized[lane]*256)*0.0000152587890625f;
}
CarDriverResult driverAIActionRecoveryGoalXY(const DriverAIRoadState* road,float output[2])
{
    const CarRecoveryPose* pose=driverAIActionRecoveryGoalPose(road);
    if(!pose||road->lane<0||road->lane>=21)return CAR_DRIVER_MISSING_ROAD;
    carRoadPreviewPointXY(pose,output,0.0f,authoredLateral(pose,road->lane));
    return CAR_DRIVER_OK;
}
CarDriverResult driverAIActionRecoveryGoalPosition(const DriverAIRoadState* road,float output[4])
{
    const CarRecoveryPose* pose=driverAIActionRecoveryGoalPose(road);
    if(!pose||road->lane<0||road->lane>=21)return CAR_DRIVER_MISSING_ROAD;
    driverRoadPointPosition(pose,output,0.0f,authoredLateral(pose,road->lane));
    return CAR_DRIVER_OK;
}
int32_t driverAIActionRecoveryComplete(const DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    if(action->behavior.recovery.hasRequestedDuration&&
       !(action->behavior.recovery.requestedSeconds<=driverAIRecoveryActionSecondsSince(now,action->behavior.recovery.createdTicks)))return 0;
    float goal[2];
    const CarDriverResult result=driverAIActionRecoveryGoalXY(&perception->road,goal);
    if(result!=CAR_DRIVER_OK){driver->result=result;return 0;}
    const VehicleState* state=perception->movement->bindings.state;
    float toGoal[2]={goal[0]-state->position[0],goal[1]-state->position[1]};
    float heading[2];
    vectorNormalize2(toGoal,toGoal);vectorNormalize2(heading,state->orientation);
    const float first=heading[0]*toGoal[0];
    const float second=heading[1]*toGoal[1];
    const int32_t complete=0.9800000190734863f<=first+second&&!driverAIActionRecoveryBlocked(driver,perception);
    if(complete)finishRecovery(driver);
    return complete;
}
int32_t driverAIActionRecoveryTimedOut(const DriverAIAction* action,DriverAIBase* driver)
{
    const int32_t now=driver->physics.movement->bindings.environment->clock->worldTimer;
    const int32_t timedOut=driverAIActionDeadlinePassed(action,now);
    if(timedOut)finishRecovery(driver);
    return timedOut;
}
int32_t driverAIActionRecoveryFailed(const DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    CarMovement* own=perception->movement;
    const int32_t now=own->bindings.environment->clock->worldTimer;
    int32_t failed=driverAIRoadEmpty(&perception->road)||own->bindings.state->activeContactCount<3;
    if(!failed&&(!action->behavior.recovery.hasRequestedDuration||
       action->behavior.recovery.requestedSeconds<=driverAIRecoveryActionSecondsSince(now,action->behavior.recovery.createdTicks))){
        failed=driverAIActionRoadBoundary(&perception->road,driver,1)==2;
        const CollisionShape* shape=own->bindings.ownerMotion->collision->shape;
        for(int32_t index=0;index<shape->contactCount&&!failed;++index){
            const CollisionImpact* impact=collisionShapeContactAt(driver->environment->simulation->collisions,shape,index);
            if(!impact){driver->result=CAR_DRIVER_INVALID_BINDING;return 0;}
            if(driverAIActionContactSide(impact,&driver->physics)!=1)continue;
            if(0.4000000059604645f<=driverAIRecoveryActionSecondsSince(now,impact->createdTicks)){
                const float speed=own->bindings.state->speed;
                if(fabsf(speed)<1.0f||fabsf(speed-1.0f)<=0.0010000000474974513f)failed=1;
            }
        }
    }
    if(failed)finishRecovery(driver);
    return failed;
}
void driverAIActionPlanRecovery(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    DriverAIRoadState* road=&perception->road;
    DriverAIGoal* goal=&perception->goal;
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    if(!driverAIGoalUsesRoad(goal->kind))driverAIGoalSet(goal,driverLaneSelectorForIndex(road->lane),NULL,0);
    driverCommandAdvance(&action->command,NULL,now,0.0f);
    action->speedTolerance=0.0f;action->useRoadSpeed=0;action->affectsRacePace=0;
    action->pathValid=1;action->enabled=1;
    if(driverAIRoadEmpty(road)){action->pathValid=0;return;}
    float position[4];
    CarDriverResult result=driverAIActionRecoveryGoalPosition(road,position);
    if(result!=CAR_DRIVER_OK){driver->result=result;return;}
    const CarRecoverySelection selection={road->group,road->recovery.recoveryPose};
    driverCommandAppend(&action->command,position,&selection,road->lane,0,NULL);
    result=driverLaneBuildRoad(&action->command,road,perception,1,1,driver);
    if(result!=CAR_DRIVER_OK){driver->result=result;return;}
    action->targetSpeed=perception->movement->bindings.ownerMotion->inputCategory==CAR_DRIVER_AI?-15.0f:-6.0f;
}
int32_t driverAIActionRoadFollowingPriority(DriverAIPerception* perception,const CarMovement* target)
{
    return driverPerceptionPrevious(perception,target)?200:-1;
}
void driverAIActionPlanRoadFollowing(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    DriverAIGoal* goal=&perception->goal;
    DriverAIRoadState* road=&perception->road;
    const int32_t now=perception->movement->bindings.environment->clock->worldTimer;
    if(!driverAIGoalUsesRoad(goal->kind)){
        driverAIGoalSet(goal,1,NULL,0);
        driverAIRoadSetLane(road,driverAILaneForClass(1,driver->physics.routeVehicleClass),1,now);
    }
    driverLanePlanGoal(goal,&action->command,road,perception,driver,0);
    action->speedTolerance=0.0f;
    action->affectsRacePace=1;action->useRoadSpeed=1;action->enabled=1;
    DriverAINeighbor* neighbor=driverPerceptionPrevious(perception,perception->movement);
    if(neighbor){
        const float halfWidth=perception->lateralExtent+neighbor->effectiveHalfExtents[0];
        if(fabsf(neighbor->localPosition[0])<halfWidth&&neighbor->localPosition[1]<6.0f){
            const float difference=20.0f-neighbor->localPosition[1];
            const float speed=neighbor->movement->bindings.state->speed-difference;
            if(speed<action->targetSpeed){
                action->targetSpeed=speed;action->targetDuration=0.0f;action->useRoadSpeed=0;
            }
        }
    }
    action->pathValid=1;
}
#include "indep/inc/TrackPath.hpp"
static float driverAIRoadBlockActionAuthoredLateral(const CarRecoveryPose* pose,int32_t lane)
{
    const int32_t shifted=(int32_t)pose->laneOffsetsQuantized[lane]*256;
    return (float)shifted*0.0000152587890625f;
}
int32_t driverAIActionRoadBlockPriority(DriverAIBase* driver,DriverAIPerception* perception,
    PursuitRoadBlock** selected)
{
    *selected=NULL;
    PursuitManager* manager=driver->environment->simulation->pursuit;
    if(!manager)return -1;
    const CarMovement* movement=perception->movement;
    const float range=maximumFinite(movement->bindings.state->speed*2.5f,100.0f);
    for(int32_t index=0;index<pursuitManagerRoadBlockCount(manager);++index){
        PursuitRoadBlock* block=pursuitManagerRoadBlockAt(manager,index);
        if(!block->entries[0]&&!block->entries[1])continue;
        const float distance=trackWrappedDelta(movement->bindings.route->length,
            movement->routeLocation.distance,block->routeDistance);
        if(distance< -range){*selected=block;return 600;}
    }
    return -1;
}
int32_t driverAIActionRoadBlockComplete(const DriverAIAction* action,
    const DriverAIPerception* perception)
{
    const PursuitRoadBlock* block=action->behavior.roadBlock.block;
    if(!block->entries[0]&&!block->entries[1])return 1;
    const CarMovement* movement=perception->movement;
    const float distance=trackWrappedDelta(movement->bindings.route->length,
        movement->routeLocation.distance,block->routeDistance);
    return 15.0f<distance;
}
static float driverAIRoadBlockActionLateralAt(const float point[2],const CarRecoveryPose* pose)
{
    const float x=point[0]-pose->position[0];
    const float y=point[1]-pose->position[1];
    const float first=x*pose->direction[1];
    const float second=y*pose->direction[0];
    return first-second;
}

void driverAIActionPlanRoadBlock(DriverAIAction* action,DriverAIBase* driver,
    DriverAIPerception* perception)
{
    DriverAIRoadState* road=&perception->road;
    DriverAIGoal* goal=&perception->goal;
    const CarMovement* movement=perception->movement;
    const int32_t now=movement->bindings.environment->clock->worldTimer;
    const PursuitRoadBlock* block=action->behavior.roadBlock.block;
    if(!driverAIGoalUsesRoad(goal->kind))driverAIGoalSet(goal,driverLaneSelectorForIndex(road->lane),NULL,0);
    driverCommandAdvance(&action->command,NULL,now,0.0f);
    action->speedTolerance=0.0f;action->enabled=1;action->useRoadSpeed=0;
    const CarRecoveryPose* pose=road->recovery.recoveryPose;
    const float initial=driverAIRoadBlockActionLateralAt(movement->bindings.state->position,pose)-driverAIRoadBlockActionAuthoredLateral(pose,road->lane);
    float final=0.0f;
    const CarRecoveryPose* targetPose=NULL;
    /* The shared record retains its historical neutral member name. These
     * two values are consumed here as a world XY point. */
    const float* targetPoint=block->along;
    if(road->group==block->roadGroup){
        targetPose=block->roadPose;
        final=driverAIRoadBlockActionLateralAt(targetPoint,targetPose)-driverAIRoadBlockActionAuthoredLateral(targetPose,road->lane);
    }else{
        // PAL1C995C keeps the current road pose in a2 through the alternate
        // group branch to1C9A40. A null start makes1C1D30 fail immediately
        // and incorrectly plans toward the group's last point every time.
        targetPose=carRecoveryFirstFollowingPose(road->group,targetPoint,pose);
        if(targetPose){
            const float side=driverAIRoadBlockActionLateralAt(targetPoint,targetPose);
            const float outside=0.0f<side?side-targetPose->halfWidths[1]:-(side+targetPose->halfWidths[0]);
            if(outside<5.0f)final=side-driverAIRoadBlockActionAuthoredLateral(targetPose,road->lane);
            else targetPose=NULL;
        }
        if(!targetPose){
            targetPose=&road->group->poses[road->group->poseCount-1];
            final=initial;
        }
    }
    DriverLanePoints points;
    CarDriverResult result=driverLaneBuildPointRange(&points,road,targetPose);
    if(result!=CAR_DRIVER_OK){driver->result=result;return;}
    driverLaneAppendTransition(&points,&action->command,perception,points.count-1,0,initial,final);
    result=driverLaneBuildRoad(&action->command,road,perception,1,0,driver);
    if(result!=CAR_DRIVER_OK){driver->result=result;return;}
    action->useRoadSpeed=1;
    float time=0.0f;
    CarMovement* collision=driverCollisionPredictNeighbors(driver,&action->command,perception,road,
        1.0f,&time,driver->environment->simulation->world);
    if(collision&&collision->bindings.info->bodyCategory!=1&&collision->bindings.owner->driverInfo->driverClass!=3){
        const DriverAINeighbor* neighbor=driverPerceptionFind(perception,collision);
        if(neighbor&&0.0f<neighbor->localPosition[1]){
            action->useRoadSpeed=0;
            driverAIActionMatchNeighborSpeed(action,neighbor,perception,0.10000000149011612f);
            action->targetSpeed=maximumFinite(action->targetSpeed,30.0f);
        }
    }
    action->pathValid=1;
}
#include "Chopper.hpp"
static int32_t elapsedTicks(int32_t now,int32_t previous)
{
    return (int32_t)((uint32_t)now-(uint32_t)previous);
}
static float distanceXY(const float first[2],const float second[2])
{
    const float x=first[0]-second[0];
    const float y=first[1]-second[1];
    const float xx=x*x;
    const float yy=y*y;
    return sqrtf(xx+yy);
}
static int32_t signalWithinRange(DriverAIBase* driver,CarMovement* movement)
{
    const WorldSimulation* simulation=driver->environment->simulation;
    if(!movement||!movement->bindings.owner){driver->result=CAR_DRIVER_INVALID_BINDING;return 0;}
    const Car* car=movement->bindings.owner;
    const size_t index=car->tableIndex;
    if(!simulation->frames||index>=simulation->frameCount||index>=WORLD_MAX_CARS||
       simulation->world->carTable[index]!=car||!simulation->frames[index]){
        driver->result=CAR_DRIVER_INVALID_BINDING;return 0;
    }
    if(!carFrameState(simulation->frames[index])->signalMode)return 0;
    const float range=maximumFinite(movement->bindings.state->speed*3.5f,100.0f);
    return distanceXY(movement->bindings.state->position,
        driver->physics.movement->bindings.state->position)<=range;
}
void driverAIActionBrakeFor(DriverAIAction* action,const DriverPhysicsVehicle* vehicle,float seconds)
{
    const float speed=vehicle->movement->bindings.state->speed;
    const float acceleration=driverBrakingAcceleration(vehicle,speed);
    const float change=acceleration*seconds;
    action->targetDuration=seconds;
    action->targetSpeed=speed+change;
}
int32_t driverAIActionYieldingPriority(DriverAIBase* driver)
{
    const CarMovement* own=driver->physics.movement;
    if(own->bindings.ownerMotion->inputCategory!=CAR_DRIVER_TRAFFIC)return -1;
    const WorldSimulation* simulation=driver->environment->simulation;
    const Chopper* chopper=simulation->chopper;
    if(chopper&&((uint32_t)chopper->phase<3u||chopper->phase==4)&&
       distanceXY(chopper->position,own->bindings.state->position)<=150.0f)return 600;
    if(simulation->pursuit){
        const PursuitCollisionContext* context=simulation->pursuit->collision;
        if(!context||context->primaryEntryCount>context->entryCount||
           (context->primaryEntryCount&&!context->entries)){
            driver->result=CAR_DRIVER_INVALID_BINDING;return -1;
        }
        for(size_t index=0;index<context->primaryEntryCount;++index){
            const PursuitCollisionEntry* entry=context->entries[index];
            if(entry&&entry->state!=2&&signalWithinRange(driver,entry->movement))return 600;
            if(driver->result!=CAR_DRIVER_OK)return -1;
        }
    }
    const CarFramePlayerRegistry* registry=simulation->players;
    if(registry){
        if(registry->count<0||registry->count>2){driver->result=CAR_DRIVER_INVALID_BINDING;return -1;}
        for(int32_t index=0;index<registry->count;++index){
            const CarFramePlayerState* player=registry->players[index];
            if(!player){driver->result=CAR_DRIVER_INVALID_BINDING;return -1;}
            if(signalWithinRange(driver,player->car))return 600;
            if(driver->result!=CAR_DRIVER_OK)return -1;
        }
    }
    return -1;
}
int32_t driverAIActionYieldingComplete(DriverAIBase* driver)
{
    return driver->physics.movement->bindings.ownerMotion->inputCategory==CAR_DRIVER_TRAFFIC&&
        driverAIActionYieldingPriority(driver)==-1;
}
void driverAIActionPlanYielding(DriverAIAction* action,DriverAIBase* driver,DriverAIPerception* perception)
{
    CarMovement* movement=perception->movement;
    DriverAIRoadState* road=&perception->road;
    DriverAIGoal* goal=&perception->goal;
    const int32_t now=movement->bindings.environment->clock->worldTimer;
    driverCommandCopy(&action->command,&driver->controls->command);
    action->useRoadSpeed=0;action->enabled=1;action->speedTolerance=0.0f;action->affectsRacePace=0;
    const int32_t special=movement->bindings.owner->driverInfo->behaviorProfile==-2;
    const float stopDelay=action->behavior.yielding.stopDelaySeconds;
    const int32_t unlimited=fabsf(stopDelay-(-999.0f))<=0.0010000000474974513f;
    const float elapsed=(float)elapsedTicks(now,action->behavior.yielding.createdTicks)*0.0002500000118743628f;
    if(unlimited||elapsed<stopDelay){
        const float sinceProbe=(float)elapsedTicks(now,action->behavior.yielding.lastProbeTicks)*0.0002500000118743628f;
        if(0.30000001192092896f<sinceProbe||!action->command.count){
            action->behavior.yielding.lastProbeTicks=now;
            if(!driverAIGoalUsesRoad(goal->kind))
                driverAIGoalSet(goal,driverLaneSelectorForIndex(road->lane),NULL,0);
            action->behavior.yielding.collisionPathClear=1;
            if(special)driverLanePlanGoal(goal,&action->command,road,perception,driver,1);
            else{
                driverCommandAdvance(&action->command,NULL,now,0.0f);
                const CarDriverResult result=driverLaneBuildRoad(&action->command,road,perception,3,0,driver);
                if(result!=CAR_DRIVER_OK){driver->result=result;return;}
            }
            float time=0.0f;
            CarMovement* collision=driverCollisionPredictNeighbors(driver,&action->command,perception,road,
                driver->environment->tuning->laneCollisionHorizon,&time,driver->environment->simulation->world);
            if(collision&&time<2.0f){
                DriverCommand alternative;
                DriverAIGoal candidate;
                driverCommandInitialize(&alternative);driverAIGoalInitialize(&candidate);
                driverAIGoalSet(&candidate,5,collision,0);
                int32_t follow=1;
                action->behavior.yielding.collisionPathClear=driverLaneSelectGoal(&candidate,&alternative,road,
                    perception,driver,&follow,1,driver->environment->tuning->laneCollisionHorizon);
                driverCommandCopy(&action->command,&alternative);
                *goal=candidate;
            }
        }
        const float speed=movement->bindings.state->speed;
        if(special){
            const float braking=driverBrakingAcceleration(&driver->physics,speed);
            const float scale=action->behavior.yielding.collisionPathClear?0.12000000476837158f:0.20000000298023224f;
            const float change=braking*scale;
            action->targetSpeed=speed+change;
        }else if(!driverAIRoadEmpty(road)&&action->behavior.yielding.collisionPathClear){
            int32_t brake=0;
            if(15.0f<speed){action->behavior.yielding.rightMarginAvailable=1;brake=1;}
            else{
                const float margin=driverAIActionRightMargin(perception,road->recovery.recoveryPose);
                if(action->behavior.yielding.rightMarginAvailable){
                    if(margin<2.0f)action->behavior.yielding.rightMarginAvailable=0;
                }else if(4.0f<margin)action->behavior.yielding.rightMarginAvailable=1;
                brake=!action->behavior.yielding.rightMarginAvailable;
            }
            if(brake){driverAIActionBrakeFor(action,&driver->physics,0.20000000298023224f);action->speedTolerance=2.0f;}
            else{action->targetSpeed=14.0f;action->speedTolerance=2.0f;}
        }else driverAIActionBrakeFor(action,&driver->physics,0.20000000298023224f);
        if(action->targetSpeed<0.5f){action->targetSpeed=0.0f;action->speedTolerance=0.0f;}
        else{
            const CollisionShape* shape=movement->bindings.ownerMotion->collision->shape;
            for(int32_t index=0;index<shape->contactCount;++index){
                const CollisionImpact* impact=collisionShapeContactAt(driver->environment->simulation->collisions,shape,index);
                if(!impact){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
                if(collisionImpactIsCarPair(impact)){action->targetSpeed=0.0f;action->speedTolerance=0.0f;break;}
            }
        }
    }else{
        DriverAIGoal stopping;
        driverAIGoalInitialize(&stopping);
        driverAIGoalSet(&stopping,driverLaneSelectorForIndex(road->lane),NULL,0);
        driverLanePlanGoal(&stopping,&action->command,road,perception,driver,1);
        action->speedTolerance=0.0f;action->targetSpeed=0.0f;
    }
    action->pathValid=1;
    action->targetDuration=0.20000000298023224f;
}


/* Retained native status accessor, PAL1CABF0. Several Beta getters have
 * identical bytes; its individual address identity remains unresolved. */

int32_t driverAIActionStatus(const DriverAIAction* action) {return (int32_t)action->status;}
