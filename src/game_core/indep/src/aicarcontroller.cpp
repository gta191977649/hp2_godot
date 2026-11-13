#include "indep/inc/AICarController.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include <stdlib.h>
#include <string.h>
#include "indep/inc/CarCollisionDetect.hpp"
#include "indep/inc/DrivingLocation.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include <math.h>

void driverAIRoadInitialize(DriverAIRoadState* road,CarDriverKind kind)
{
    road->group=NULL;
    road->recovery.profile=(int32_t)kind;
    road->recovery.recoveryPose=NULL;
    road->recovery.recoveryDelay=0.0f;
    road->recovery.previousSpeed=0.0f;
    road->lane=-1;
    road->selectedTicks=0;
}

void driverPerceptionInitialize(DriverAIPerception* perception)
{
    perception->count=0;
    perception->lateralExtent=0.0f;
    perception->forwardExtent=0.0f;
}

void driverPerceptionReset(DriverAIPerception* perception,CarDriverKind kind)
{
    driverPerceptionInitialize(perception);
    driverAIRoadInitialize(&perception->road,kind);
    driverAIGoalInitialize(&perception->goal);
}

int32_t driverPerceptionFindIndex(const DriverAIPerception* perception,const CarMovement* movement)
{
    for(int32_t index=0;index<perception->count;++index)
        if(perception->neighbors[index].movement==movement)return index;
    return -1;
}

DriverAINeighbor* driverPerceptionFind(DriverAIPerception* perception,const CarMovement* movement)
{
    const int32_t index=driverPerceptionFindIndex(perception,movement);
    return index==-1?NULL:&perception->neighbors[index];
}

DriverAINeighbor* driverPerceptionPrevious(DriverAIPerception* perception,const CarMovement* movement)
{
    const int32_t index=driverPerceptionFindIndex(perception,movement);
    return index<=0?NULL:&perception->neighbors[index-1];
}


float driverAILateralExcess(const DriverAIBase* driver)
{
    const DriverAIRoadState* road=driver->currentRoad;
    if(driverAIRoadEmpty(road))return 0.0f;
    const CarRecoveryPose* pose=road->recovery.recoveryPose;
    const float* point=driver->physics.movement->bindings.state->position;
    const float x=point[0]-pose->position[0];
    const float y=point[1]-pose->position[1];
    const float first=x*pose->direction[1];
    const float second=y*pose->direction[0];
    const float across=first-second;
    if(0.0f<across)return across-pose->halfWidths[1];
    return -(across+pose->halfWidths[0]);
}

int32_t driverAIGoalUsesTarget(int32_t kind)
{
    return kind==5||kind==6;
}
/* PAL193780 and its actual BaseDriver1C0160 effect. This resets performance
 * scaling and enables the same pace owner; grip override is independent. */
void driverAIReleaseBoost(DriverAIBase* driver)
{
    if (!driver) return;
    CarMovement* movement = driver->physics.movement;
    if (movement) {
        if (!movement->bindings.state) {
            driver->result = CAR_DRIVER_INVALID_BINDING;
            return;
        }
        carControllerReleaseBoost(&driver->controller);
    }
    driver->paceEnabled = 1;
}

/* PAL193758 invokes the exact base ForceBoost1C0140, then disables the
 * same AI pace owner. This is vt+64, distinct from ForceToPullOver. */
void driverAIForceBoost(DriverAIBase* driver, float performanceScale)
{
    if (!driver) return;
    CarMovement* movement = driver->physics.movement;
    if (movement) {
        if (!movement->bindings.state) {
            driver->result = CAR_DRIVER_INVALID_BINDING;
            return;
        }
        carControllerForceBoost(&driver->controller,performanceScale);
    }
    driver->paceEnabled = 0;
}

CarDriverResult driverAIInitialize(DriverAIBase* driver,CarMovement* movement,
    const DriverPhysicsVehicle* physics,DriverAIEnvironment* environment,AIControls* controls,
    CarDriverKind kind)
{
    if (!driver || !movement || !physics || physics->movement!=movement || !physics->tuning ||
        !environment || !environment->simulation || !environment->simulation->world ||
        !movement->bindings.owner || !movement->bindings.recoveryPoints ||
        !movement->bindings.recoveryPoints->connections || !movement->bindings.recoveryPoints->connectionCount ||
        !controls || (kind!=CAR_DRIVER_AI && kind!=CAR_DRIVER_TRAFFIC))
        return CAR_DRIVER_INVALID_BINDING;
    memset(driver,0,sizeof(*driver));
    carControllerInitialize(&driver->controller,movement->bindings.owner);
    driver->physics=*physics;driver->environment=environment;driver->controls=controls;driver->kind=kind;
    driver->perception.movement=movement;driverPerceptionReset(&driver->perception,kind);
    driver->currentRoad=&driver->perception.road;driver->currentGoal=&driver->perception.goal;
    driver->activePerception=&driver->perception;
    driver->previousGroupId=-1;driver->thinkDelay=(float)(int16_t)movement->bindings.owner->tableIndex*-.007142857648432255f;
    driver->actionCode=11;driver->progressSampleTicks=6000;driver->stopDelaySeconds=10000000;
    driver->waitMoving=1;driver->selectedConnectionIndex=-1;driver->paceEnabled=1;
    if (!driverHistoryInitialize(&driver->progressHistory,10))
        return driver->result=CAR_DRIVER_ALLOCATION_FAILURE;
    // Final190303..19032C allocates the distinct named DeltaLineSPeed
    // Average for replay. It owns five samples, not a copy of progressHistory.
    if (!driverHistoryInitialize(&driver->deltaLineSpeedHistory,5)) {
        driverHistoryDispose(&driver->progressHistory);
        return driver->result=CAR_DRIVER_ALLOCATION_FAILURE;
    }
    driverAIConnectionsClear(driver);
    driver->nextConnectionIndex=(int32_t)carRecoveryConnectionAtDistance(
        movement->bindings.recoveryPoints,movement->routeLocation.distance);
    movement->bindings.driver=&driver->currentRoad->recovery;
    return driver->result=CAR_DRIVER_OK;
}

void driverAIDispose(DriverAIBase* driver)
{
    free(driver->action);driver->action=NULL;
    driverHistoryDispose(&driver->progressHistory);
    driverHistoryDispose(&driver->deltaLineSpeedHistory);
}

void driverAIBindBelt(DriverAIBase* driver)
{
    BeltMoverBindings* binding=&driver->beltBindings;
    binding->command=&driver->controls->command;binding->vehicle=&driver->physics;
    binding->targetSpeed=&driver->controls->targetSpeed;binding->targetDuration=&driver->controls->targetDuration;
    binding->replanDelayTicks=&driver->replanDelayTicks;binding->world=driver->environment->simulation->world;
    driver->physics.movement->bindings.belt=binding;
}

void driverAIResetAfterPlacement(DriverAIBase* driver)
{
    const CarMovement* movement=driver->physics.movement;
    driver->thinkDelay=(float)(int16_t)movement->bindings.owner->tableIndex*-.007142857648432255f;
    // The source clears this separate replay vector after placement.
    memset(driver->retainedSnapshotPoint,0,sizeof(driver->retainedSnapshotPoint));
    aiControlsClearAngleFilters(driver->controls);
    driverAIConnectionsClear(driver);
    driver->nextConnectionIndex=(int32_t)carRecoveryConnectionAtDistance(
        movement->bindings.recoveryPoints,movement->routeLocation.distance);
}

DriverAIAction* driverAISelectAction(DriverAIBase* driver)
{
    DriverAIAction* candidate=NULL;
    if(driver->kind==CAR_DRIVER_AI)candidate=aiDriverCreateAction(driver->owner.ai,driver->actionCode);
    else if(driver->kind!=CAR_DRIVER_TRAFFIC){driver->result=CAR_DRIVER_INVALID_BINDING;return NULL;}
    /* Traffic inherits the original empty factory. Its cruise action is
     * installed by its actual placement/reset operation. */
    if(driver->actionCode!=11){driver->actionCode=11;return candidate;}
    // Native PC arbitration admits eligible recovery over RoadBlock;
    // other automatic candidates keep the original priority comparison.
    if(candidate&&driver->action&&(driver->action->priority<candidate->priority||
        (driver->action->kind==8&&candidate->kind==7)))return candidate;
    free(candidate);
    return NULL;
}
#include "indep/inc/Replay.hpp"
#include "indep/inc/DrivingChoice.hpp"
#include "bware/inc/bWare.hpp"
// Full PAL193EA0..193EFC, DrivingInformation::DoSnapshot. Nearby-car
// results are transient and are cleared on both record and restore.
void driverPerceptionSnapshot(DriverAIPerception* perception,const CarRecoveryPointRegistry* registry,
    const World* world,ReplaySnapshot* snapshot)
{
    if(snapshot->recording||snapshot->restoring) {
        perception->count=0;
        perception->lateralExtent=0;
        perception->forwardExtent=0;
    }
    driverAIRoadSnapshot(&perception->road,registry,snapshot);
    driverAIGoalSnapshot(&perception->goal,world,snapshot);
}

// Full PAL190438..190670, including the final connection index and timer.
// Native packing below is the exact original connection-word format; it
// does not serialize host pointers or padding from the typed native records.
void driverAISnapshot(DriverAIBase* driver,ReplaySnapshot* snapshot)
{
    const auto* registry=driver->physics.movement->bindings.recoveryPoints;
    World* world=driver->environment->simulation->world;
    replaySnapshotBeginSection(snapshot,nullptr,"AICarController");
    carControllerSnapshot(driver->physics.movement,snapshot);
    driverPerceptionSnapshot(&driver->perception,registry,world,snapshot);
    replaySnapshotMoveData(snapshot,&driver->actionCode,1,-1,0);
    replaySnapshotMoveInteger(snapshot,&driver->previousGroupId,1);
    replaySnapshotMoveFloat(snapshot,&driver->thinkDelay);
    driverAIActionSnapshot(driver,snapshot);
    replaySnapshotMoveData(snapshot,&driver->speedOffsetSampleTicks,4,-1,0);
    replaySnapshotMoveInteger(snapshot,&driver->speedSamplePointOffset,4);
    Car* planning=driver->planningTarget?driver->planningTarget->bindings.owner:nullptr;
    worldSnapshotCarPointer(world,snapshot,&planning);
    if(snapshot->restoring)driver->planningTarget=planning?planning->Movement():nullptr;
    replaySnapshotMoveData(snapshot,&driver->performanceSampleTicks,4,-1,0);
    replaySnapshotMoveData(snapshot,&driver->replanDelayTicks,4,-1,0);
    replaySnapshotMoveData(snapshot,&driver->restoreModeDelayTicks,4,-1,0);
    replaySnapshotMoveData(snapshot,&driver->progressSampleTicks,4,-1,0);
    driverHistorySnapshot(&driver->progressHistory,snapshot);
    driverHistorySnapshot(&driver->deltaLineSpeedHistory,snapshot);
    replaySnapshotMoveData(snapshot,driver->retainedSnapshotPoint,16,-1,0);
    worldSnapshotCarPointer(world,snapshot,&driver->retainedSnapshotTarget);
    replaySnapshotMoveData(snapshot,&driver->linkedPlayerRemainingTicks,4,-1,0);
    replaySnapshotMoveData(snapshot,&driver->linkedPlayerCooldownTicks,4,-1,0);
    replaySnapshotMoveInteger(snapshot,&driver->retainedSnapshotCounter,4);
    replaySnapshotMoveFloat(snapshot,&driver->retainedSnapshotScalar);
    replaySnapshotMoveFloat(snapshot,&driver->stopDelaySeconds);
    int8_t playerNumber=driver->pacePlayer?static_cast<int8_t>(driver->pacePlayer->movement.seat):-1;
    replaySnapshotMoveData(snapshot,&playerNumber,1,1,1);
    if(snapshot->restoring) {
        auto* players=driver->environment->simulation->players;
        hp2::Require(playerNumber<2&&playerNumber>=-1,
            "AICarController snapshot player number");
        if(playerNumber>=0)hp2::Require(players&&players->playersByNumber[playerNumber],
            "AICarController snapshot player is not live");
        driver->pacePlayer=playerNumber<0?nullptr:players->playersByNumber[playerNumber];
    }
    replaySnapshotMoveInteger(snapshot,&driver->nextConnectionIndex,4);
    replaySnapshotMoveData(snapshot,&driver->paceEnabled,1,4,0);
    uint32_t connections[8];
    for(int index=0;index<8;++index) {
        const auto& entry=driver->connectionChoices[index];
        connections[index]=(static_cast<uint32_t>(entry.active)&1u)|
            ((static_cast<uint32_t>(entry.accepted)&1u)<<1)|
            ((static_cast<uint32_t>(entry.lane)&63u)<<2)|
            (static_cast<uint32_t>(entry.connectionIndex)<<8)|
            (static_cast<uint32_t>(entry.snapshotReservedBits)<<16);
    }
    replaySnapshotMoveData(snapshot,connections,32,-1,0);
    if(snapshot->restoring)for(int index=0;index<8;++index) {
        auto& entry=driver->connectionChoices[index];const uint32_t value=connections[index];
        entry.active=value&1;entry.accepted=(value>>1)&1;entry.lane=(value>>2)&63;
        entry.connectionIndex=static_cast<uint8_t>(value>>8);
        entry.snapshotReservedBits=static_cast<uint16_t>(value>>16);
    }
    replaySnapshotMoveInteger(snapshot,&driver->selectedConnectionIndex,4);
    replaySnapshotMoveData(snapshot,&driver->connectionDelayTicks,4,-1,0);
    replaySnapshotMoveData(snapshot,&driver->lastRecoveryActionTicks,4,-1,0);
    replaySnapshotEndSection(snapshot);
}
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/RaceCoordinator.hpp"
// Whole original AICarController pace methods, PAL1928F0/192C78.
// All five existing native definitions moved unchanged; no new state owner.
static CarMovement* entryMovement(DriverAIBase* driver,const RaceCarEntry* entry)
{
    WorldSimulation* simulation=driver->environment->simulation;
    if(!entry||!entry->car){driver->result=CAR_DRIVER_INVALID_BINDING;return NULL;}
    const uint32_t index=entry->car->tableIndex;
    if(!simulation->frames||index>=simulation->frameCount||!simulation->frames[index]){
        driver->result=CAR_DRIVER_INVALID_BINDING;return NULL;
    }
    CarMovement* movement=carFrameMovement(simulation->frames[index]);
    if(!movement||movement->bindings.owner!=entry->car){
        driver->result=CAR_DRIVER_INVALID_BINDING;return NULL;
    }
    return movement;
}

static int32_t validRank(DriverAIBase* driver,const RaceCarEntry* entry)
{
    if(!entry||entry->position<1||entry->position>8){
        driver->result=CAR_DRIVER_INVALID_BINDING;return 0;
    }
    return 1;
}

static CarFramePlayerState* playerAtSeat(const CarFramePlayerRegistry* players,int32_t seat)
{
    for(int32_t index=0;index<players->count;++index)
        if(players->players[index]&&players->players[index]->movement.seat==seat)return players->players[index];
    return NULL;
}

void driverAICompareRacePace(DriverAIBase* driver,const RaceCarEntry* own,
    const RaceCarEntry* player,float* positionResponse,float* speedResponse)
{
    *positionResponse=0.0f;
    *speedResponse=0.0f;
    const WorldSimulation* simulation=driver->environment->simulation;
    const RaceParameters* settings=simulation->race->services->settings;
    if(!settings->relativePaceEnabled)return;
    const DriverBehavior* behavior=driver->environment->behavior;
    const DriverAIPolicy* policy=driver->environment->policy;
    if(!policy||!policy->launch||!behavior){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
    float progress=own->progress-player->progress;
    if(settings->specialPursuit||(policy->launch->skipFrontend&&policy->raceVariantMode)){
        const float shifted=progress+0.5f;
        float whole=truncf(shifted);
        if(shifted<whole)whole=whole-1.0f;
        progress=(shifted-whole)-0.5f;
    }
    const float distance=simulation->race->route->length*progress;
    if(!(distance<=behavior->maximumPaceDistance))return;
    if(!validRank(driver,own)||!validRank(driver,player))return;
    CarMovement* ownMovement=entryMovement(driver,own);
    CarMovement* playerMovement=entryMovement(driver,player);
    if(!ownMovement||!playerMovement)return;
    const KeyedCurve first={behavior->positionResponse,7};
    const KeyedCurve second={behavior->speedResponse,7};
    const float firstValue=keyedCurveEvaluate(&first,distance);
    const float secondValue=keyedCurveEvaluate(&second,distance);
    CarCameraParameterMap mapping;
    carCameraParameterMapInitialize(&mapping,0.0f,firstValue,9.0f,secondValue);
    int32_t profile=driver->physics.movement->bindings.owner->driverInfo->behaviorProfile;
    if(profile<0)profile=0;
    const float profileValue=(float)profile;
    const float response=carCameraParameterMapEvaluate(&mapping,profileValue);
    const float firstDistance=distance*0.009999999776482582f;
    const float scaledDistance=firstDistance*distance;
    const float denominator=scaledDistance+1.0f;
    const float speedDifference=ownMovement->bindings.state->speed-playerMovement->bindings.state->speed;
    const float scaledDifference=speedDifference*-0.004999999888241291f;
    const float correction=finiteRatio(scaledDifference,denominator);
    const float limited=minimumFinite(0.05000000074505806f,
        maximumFinite(correction,-0.05000000074505806f));
    *positionResponse=response+limited;
    const int32_t difference=(int32_t)player->position-(int32_t)own->position;
    const int32_t index=difference>=0?difference+7:difference+8;
    carCameraParameterMapInitialize(&mapping,0.0f,behavior->rankMinimumResponse[index],
        9.0f,behavior->rankMaximumResponse[index]);
    *speedResponse=carCameraParameterMapEvaluate(&mapping,profileValue);
}

void driverAIUpdateRacePace(DriverAIBase* driver)
{
    CarMovement* movement=driver->physics.movement;
    RaceCarEntry* own=movement->bindings.owner->raceEntry;
    if(!own||!driver->paceEnabled||movement->bindings.player)return;
    WorldSimulation* simulation=driver->environment->simulation;
    const DriverBehavior* behavior=driver->environment->behavior;
    const DriverAIPolicy* policy=driver->environment->policy;
    if(!behavior||!policy||!simulation->race||!simulation->race->services||
        !simulation->race->services->settings||!simulation->race->services->session){
        driver->result=CAR_DRIVER_INVALID_BINDING;return;
    }
    const RaceParameters* settings=simulation->race->services->settings;
    // Both distance/rank correction and the later player-average-speed
    // multiplier belong to Catch Up. Disabling only CompareRacePace left
    // the second multiplier active, and the interval gate retained a prior
    // boost. Clear only these pacing outputs; authored AI skill stays live.
    if(!settings->relativePaceEnabled){
        driver->pacePlayer=NULL;
        movement->bindings.state->performanceScale=1.0f;
        return;
    }
    const int32_t now=movement->bindings.environment->clock->worldTimer;
    const int32_t elapsed=(int32_t)((uint32_t)now-(uint32_t)driver->performanceSampleTicks);
    if(!(behavior->performanceIntervalSeconds<
        (float)elapsed*0.0002500000118743628f))return;
    driver->performanceSampleTicks=now;
    driver->pacePlayer=NULL;
    const int32_t affectsPace=driver->action&&driver->action->affectsRacePace!=0;
    const float raceSeconds=(float)simulation->race->elapsedTicks*0.0002500000118743628f;
    float performance=1.0f;
    if(!policy->disableRacePace&&simulation->race->services->session->state!=8&&settings->mode>0){
        if(!affectsPace){movement->bindings.state->performanceScale=performance;return;}
        if(!(raceSeconds<30.0f)){
            CarFramePlayerRegistry* players=simulation->players;
            if(!players){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
            float positionResponse=0.0f,speedResponse=0.0f,contribution=1.0f;
            CarFramePlayerState* selected=NULL;
            if(players->count==1)selected=players->players[0];
            else if(players->count==2){
                CarFramePlayerState* first=playerAtSeat(players,1);
                CarFramePlayerState* second=playerAtSeat(players,0);
                if(!first||!second||!first->car||!second->car){
                    driver->result=CAR_DRIVER_INVALID_BINDING;return;
                }
                RaceCarEntry* firstEntry=first->car->bindings.owner->raceEntry;
                RaceCarEntry* secondEntry=second->car->bindings.owner->raceEntry;
                if(!validRank(driver,own)||!validRank(driver,firstEntry)||!validRank(driver,secondEntry))return;
                if(firstEntry->position<secondEntry->position){
                    CarFramePlayerState* swap=first;first=second;second=swap;
                    RaceCarEntry* entrySwap=firstEntry;firstEntry=secondEntry;secondEntry=entrySwap;
                }
                int32_t pivot=settings->opponentCount/2;
                int32_t threshold=pivot<secondEntry->position?pivot:pivot+1;
                if(!(threshold<firstEntry->position))++threshold;
                selected=threshold<own->position?first:second;
            }
            if(selected){
                if(!selected->car||!selected->car->bindings.owner->raceEntry||
                    (uint32_t)selected->movement.seat>=2u){
                    driver->result=CAR_DRIVER_INVALID_BINDING;return;
                }
                driverAICompareRacePace(driver,own,selected->car->bindings.owner->raceEntry,
                    &positionResponse,&speedResponse);
                if(driver->result!=CAR_DRIVER_OK)return;
                driver->pacePlayer=selected;
                contribution=settings->playerPaceContribution[selected->movement.seat];
            }
            const float base=positionResponse+1.0f;
            const float delta=speedResponse*contribution;
            performance=base+delta;
        }
    }
    if(affectsPace&&driver->pacePlayer){
        int32_t profile=movement->bindings.owner->driverInfo->behaviorProfile;
        if(profile<0)profile=0;
        if(profile>=10){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
        const float limit=behavior->profiles[profile].playerPerformanceLimit;
        if(carFramePlayerExceededSpeedRatio(driver->pacePlayer,limit)){
            RaceCarEntry* entry=driver->pacePlayer->car->bindings.owner->raceEntry;
            if(!validRank(driver,entry))return;
            const float ratio=carFramePlayerAverageSpeedRatio(driver->pacePlayer);
            const float* gains=1.0f<=ratio?behavior->paceAboveOneGains[entry->position-1]:
                behavior->paceBelowOneGains[entry->position-1];
            SampledCurve curve;
            sampledCurveInitialize(&curve,gains,2,0.0f,9.0f);
            const float difference=ratio-1.0f;
            const float gain=sampledCurveEvaluate(&curve,(float)profile);
            const float scaled=difference*gain;
            const float adjusted=scaled+1.0f;
            performance=performance*maximumFinite(adjusted,limit);
        }
    }
    movement->bindings.state->performanceScale=performance;
}

#include "indep/inc/PsychoDriver.hpp"
#include <limits.h>
static int32_t timerRunning(int32_t ticks)
{
    return ticks!=0&&ticks!=INT32_MAX;
}
static int32_t roadSpeedScale(const DriverBehavior* behavior,int32_t profile,float* scale)
{
    if(profile==-2)*scale=behavior->roadSpeedScaleNegativeTwo;
    else if(profile==-1)*scale=behavior->roadSpeedScaleNegativeOne;
    else if((uint32_t)profile<10u)*scale=behavior->profiles[profile].roadSpeedScale;
    else return 0;
    return 1;
}
static const CarRecoveryPose* speedSamplePose(const DriverAIRoadState* road,int32_t offset)
{
    const CarRecoveryGroup* group=road->group;
    if(!group||!group->poses||!road->recovery.recoveryPose||group->poseCount<=0)return NULL;
    const uintptr_t first=(uintptr_t)group->poses;
    const uintptr_t current=(uintptr_t)road->recovery.recoveryPose;
    if(current<first||(current-first)%sizeof(*group->poses))return NULL;
    const uintptr_t index=(current-first)/sizeof(*group->poses);
    if(index>=(uintptr_t)group->poseCount)return NULL;
    int32_t selected=(int32_t)((uint32_t)index+(uint32_t)offset);
    if(selected<0)selected=0;
    if(group->poseCount-1<selected)selected=group->poseCount-1;
    return &group->poses[selected];
}
void driverAIApplyAction(DriverAIBase* driver)
{
    DriverAIAction* action=driver->action;
    if(!action){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
    aiControlsSetCommand(driver->controls,&action->command);
    driverAIUpdateRacePace(driver);
    if(driver->result!=CAR_DRIVER_OK)return;
    DriverAIRoadState* road=driver->currentRoad;
    float speed=0.0f,duration=0.0f,tolerance=0.0f;
    int32_t enabled=1;
    if(!driverAIRoadEmpty(road)){
        const DriverAIPolicy* policy=driver->environment->policy;
        CarMovement* movement=driver->physics.movement;
        const CarCollisionState* collision=movement->bindings.ownerMotion->collision;
        if(!policy||!collision||!collision->policy){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
        if(policy->fixedRoadSpeed){
            speed=15.0f;tolerance=10.0f;
        }else if(collision->policy->collisionDisabled){
            if((uint32_t)road->lane>=21u){driver->result=CAR_DRIVER_MISSING_ROAD;return;}
            speed=(float)road->recovery.recoveryPose->laneSpeeds[road->lane];
        }else{
            if(!action->useRoadSpeed){
                speed=action->targetSpeed;
                duration=action->targetDuration;
            }else{
                if((uint32_t)road->lane>=21u){driver->result=CAR_DRIVER_MISSING_ROAD;return;}
                const CarRecoveryPointRegistry* registry=movement->bindings.recoveryPoints;
                const DriverBehavior* behavior=driver->environment->behavior;
                const int32_t profile=movement->bindings.owner->driverInfo->behaviorProfile;
                if(driver->selectedConnectionIndex!=-1){
                    if(driver->selectedConnectionIndex<0||
                        (size_t)driver->selectedConnectionIndex>=registry->connectionCount){
                        driver->result=CAR_DRIVER_MISSING_ROAD;return;
                    }
                    const CarRecoveryConnection* connection=&registry->connections[driver->selectedConnectionIndex];
                    const CarRecoveryPose* pose=carRecoveryConnectionPose(registry,connection);
                    if(!pose){driver->result=CAR_DRIVER_MISSING_ROAD;return;}
                    speed=(float)pose->laneSpeeds[road->lane];
                }else if(timerRunning(driver->connectionDelayTicks))speed=99999.0f;
                else{
                    if(!behavior){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
                    const int32_t now=movement->bindings.environment->clock->worldTimer;
                    const int32_t elapsed=(int32_t)((uint32_t)now-(uint32_t)driver->speedOffsetSampleTicks);
                    if((uint32_t)profile<10u&&behavior->speedOffsetIntervalSeconds<
                        (float)elapsed*0.0002500000118743628f){
                        const uint32_t range=(uint32_t)behavior->profiles[profile].speedSamplePointRange+1u;
                        const uint32_t sample=worldRandomBounded(driver->environment->simulation->world,range*2u);
                        driver->speedSamplePointOffset=(int32_t)(sample-range);
                        driver->speedOffsetSampleTicks=now;
                    }
                    const CarRecoveryPose* pose=speedSamplePose(road,driver->speedSamplePointOffset);
                    if(!pose){driver->result=CAR_DRIVER_MISSING_ROAD;return;}
                    speed=(float)pose->laneSpeeds[road->lane];
                    if(fabsf(speed)<=0.0010000000474974513f)speed=30.0f;
                    float scale;
                    if(!roadSpeedScale(behavior,profile,&scale)){
                        driver->result=CAR_DRIVER_INVALID_BINDING;return;
                    }
                    speed=speed*scale;
                }
                const WorldSimulation* simulation=driver->environment->simulation;
                if(!simulation->race||!simulation->race->services||!simulation->race->services->settings){
                    driver->result=CAR_DRIVER_INVALID_BINDING;return;
                }
                if(!policy->launch){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
                if(simulation->race->services->settings->specialPursuit||
                    (policy->launch->skipFrontend&&policy->raceVariantMode))speed=speed*0.75f;
                speed=speed*movement->bindings.state->performanceScale;
            }
            tolerance=action->speedTolerance;
            enabled=action->enabled;
        }
    }
    aiControlsSetTarget(driver->controls,enabled,speed,duration,tolerance);
    driverCommandAdvance(&action->command,NULL,
        driver->physics.movement->bindings.environment->clock->worldTimer,0.0f);
}
#include "Track.hpp"
void driverAIConnectionsClear(DriverAIBase* driver)
{
    for (int index=0;index<8;++index) driver->connectionChoices[index].active=0;
}
void driverAIConnectionsAdvanceCursor(DriverAIBase* driver)
{
    if (!driver || !driver->physics.movement) {
        if(driver)driver->result=CAR_DRIVER_INVALID_BINDING;
        return;
    }
    const CarMovement* movement=driver->physics.movement;
    const CarRecoveryPointRegistry* registry=movement->bindings.recoveryPoints;
    if (!registry || !registry->connections || !registry->connectionCount) {
        driver->result=CAR_DRIVER_MISSING_ROAD;return;
    }
    const float distance=movement->routeLocation.distance;
    // Replay stores this cursor as a plain int. If the referenced route was
    // retired or the snapshot contains an out-of-range index, rebuild from
    // the actual route-distance producer, never index stale memory.
    if (driver->nextConnectionIndex<0 ||
        (size_t)driver->nextConnectionIndex>=registry->connectionCount) {
        driver->nextConnectionIndex=(int32_t)carRecoveryConnectionAtDistance(registry,distance);
    }
    while (registry->connections[driver->nextConnectionIndex].routeDistance<distance) {
        ++driver->nextConnectionIndex;
        if ((size_t)driver->nextConnectionIndex>=registry->connectionCount) {
            driver->nextConnectionIndex=0;break;
        }
    }
}
int32_t driverAIConnectionsFreeSlot(const DriverAIBase* driver)
{
    for (int index=0;index<8;++index) if (!driver->connectionChoices[index].active) return index;
    return -1;
}
void driverAIPointAlongHeading(DriverAIBase* driver)
{
    const VehicleState* state=driver->physics.movement->bindings.state;
    const float cosine=vectorCosineAngle(state->heading);
    const float sine=rotationSineAngle(state->heading);
    const float point[2]={state->position[0]+cosine,state->position[1]+sine};
    aiControlsSetTargetPoint(driver->controls,point);
}
static int32_t currentLane(const DriverAIBase* driver,const DriverAIRoadState* road)
{
    return driverAIRoadEmpty(road) ? driverAILaneForClass(1,driver->physics.routeVehicleClass):road->lane;
}
int32_t driverAISelectConnection(DriverAIBase* driver,const CarRecoveryConnection* connection,
    int32_t* lane,const DriverAIRoadState* road)
{
    if (!driver || !lane || !driver->physics.movement) {
        if (driver) driver->result=CAR_DRIVER_INVALID_BINDING;
        return 0;
    }
    const CarMovement* movement=driver->physics.movement;
    const CarRecoveryPointRegistry* registry=movement->bindings.recoveryPoints;
    const int32_t ownedIndex=carRecoveryOwnedConnectionIndex(registry,connection);
    if (ownedIndex<0) {
        driver->result=CAR_DRIVER_MISSING_ROAD;
        return 0;
    }
    const uint8_t index=static_cast<uint8_t>(ownedIndex);
    const int32_t kind=connection->flags&7;
    if ((connection->flags&8) || ((connection->flags&64) && kind!=2)) {
        *lane=currentLane(driver,road);return 0;
    }
    for (int slot=0;slot<8;++slot) {
        const DriverAIConnectionChoice* choice=&driver->connectionChoices[slot];
        if (choice->active && choice->connectionIndex==index) {*lane=choice->lane;return choice->accepted;}
    }
    const int32_t slot=driverAIConnectionsFreeSlot(driver);
    if (slot<0) {driver->result=CAR_DRIVER_CAPACITY;return 0;}
    int32_t accepted=kind==2;
    if (kind!=2) {
        const DriverAIEnvironment* environment=driver->environment;
        const RaceFrame* race=environment->simulation->race;
        const int32_t cutoff=environment->scenario->connectionActivationSeconds;
        const int32_t enabled=cutoff==-1 ? !raceFrameConnectionWindowActive(race,movement->bindings.owner,&raceConnectionDefaultTiming):
            (float)cutoff<(float)race->elapsedTicks*.0002500000118743628f;
        if (enabled) {
            if (kind==4) accepted=worldRandomFloat(environment->simulation->world,1)<.5f;
            else if (kind!=5) {
                int32_t profile=movement->bindings.owner->driverInfo->behaviorProfile;
                if (profile<0) profile=0;else if (profile>9) profile=9;
                const float value=worldRandomFloat(environment->simulation->world,1);
                const float* chances=environment->behavior->profiles[profile].connectionCumulativeChances;
                int32_t selection=0;
                if (value<=chances[0]) selection=2;
                else if (value<=chances[1]) selection=1;
                else if (value<=chances[2]) selection=3;
                if (selection==0 || selection==1) accepted=kind==0;
                else if (selection==2) accepted=kind==1;
            }
        }
    }
    const CarRecoveryGroup* group=carRecoveryConnectionGroup(registry,connection);
    const CarRecoveryPose* pose=carRecoveryConnectionPose(registry,connection);
    if (kind!=2) {
        if (accepted) {
            accepted=driverAIChooseLane(driver,group,pose,1,lane,road);
            if (accepted && connection->properties==2) driver->selectedConnectionIndex=index;
        }
        if (!accepted) *lane=road->lane;
    }
    if (accepted && *lane<0) driverAIChooseLane(driver,group,pose,1,lane,road);
    if ((uint32_t)*lane>=2) *lane=currentLane(driver,road);
    DriverAIConnectionChoice* choice=&driver->connectionChoices[slot];
    choice->active=1;choice->accepted=accepted;choice->lane=*lane&63;choice->connectionIndex=index;
    return accepted;
}
void driverAIConnectionsUpdate(DriverAIBase* driver)
{
    if (!driver || !driver->physics.movement || !driver->physics.movement->bindings.state) {
        if(driver)driver->result=CAR_DRIVER_INVALID_BINDING;
        return;
    }
    const CarMovement* movement=driver->physics.movement;
    const CarRecoveryPointRegistry* registry=movement->bindings.recoveryPoints;
    if (!registry || !registry->route || !registry->connections || !registry->connectionCount) {
        driver->result=CAR_DRIVER_MISSING_ROAD;return;
    }
    const float distance=movement->routeLocation.distance;
    if (driver->nextConnectionIndex<0 ||
        (size_t)driver->nextConnectionIndex>=registry->connectionCount)
        driver->nextConnectionIndex=(int32_t)carRecoveryConnectionAtDistance(registry,distance);
    for (int slot=0;slot<8;++slot) {
        DriverAIConnectionChoice* choice=&driver->connectionChoices[slot];
        if (choice->active && (size_t)choice->connectionIndex>=registry->connectionCount) {
            choice->active=0;
            if (choice->connectionIndex==driver->selectedConnectionIndex)
                driver->selectedConnectionIndex=-1;
            continue;
        }
        if (choice->active && trackRouteDistanceDifference(registry->route,distance,
                registry->connections[choice->connectionIndex].routeDistance)>10) {
            choice->active=0;
            if (choice->connectionIndex==driver->selectedConnectionIndex) driver->selectedConnectionIndex=-1;
        }
    }
    const DriverPhysicsTuning* tuning=driver->physics.tuning ? driver->physics.tuning:&driverPhysicsDefaultTuning;
    const float horizon=minimumFinite(maximumFinite(tuning->planningSeconds*(movement->bindings.state->speed*1.1f),100),250);
    // A valid route can have fewer connections than the eight remembered
    // choices. All of them may be behind the car, or marked non-selectable.
    // Neither case consumes a choice slot. End the search after one authored
    // table revolution instead of spinning World forever. Routes that fill
    // the available choices still follow the original source ordering.
    size_t inspected=0;
    while (driverAIConnectionsFreeSlot(driver)!=-1 && inspected<registry->connectionCount) {
        ++inspected;
        const CarRecoveryConnection* connection=&registry->connections[driver->nextConnectionIndex];
        const float delta=trackRouteDistanceDifference(registry->route,connection->routeDistance,distance);
        if (delta<0) {
            ++driver->nextConnectionIndex;
            if ((size_t)driver->nextConnectionIndex>=registry->connectionCount) driver->nextConnectionIndex=0;
        } else {
            if (!(delta<horizon)) break;
            int32_t lane=-1;driverAISelectConnection(driver,connection,&lane,driver->currentRoad);
            if (driver->result!=CAR_DRIVER_OK) return;
            ++driver->nextConnectionIndex;
            if ((size_t)driver->nextConnectionIndex>=registry->connectionCount) driver->nextConnectionIndex=0;
        }
    }
}
static int32_t evaluateRoadCandidate(DriverAIBase* driver,const DriverAIGoal* goal,
    DriverCommand* command,const DriverAIRoadState* road)
{
    float speed,time;
    CarMovement* collision;
    const float horizon=driver->environment->tuning->laneCollisionHorizon;
    return driverLaneEvaluateRoadGoal(goal,command,road,driver->activePerception,driver,0,horizon,&speed,&collision,&time);
}
int32_t driverAIChooseLane(DriverAIBase* driver,const CarRecoveryGroup* group,const CarRecoveryPose* pose,
    int32_t lockSelection,int32_t* lane,const DriverAIRoadState* diagnosticRoad)
{
    (void)diagnosticRoad; /* The original optional call publishes editor selection only. */
    if(!driver||!lane||!driver->controls||!driver->activePerception||!driver->environment||
       !driver->environment->tuning||!driver->environment->simulation){
        if(driver)driver->result=CAR_DRIVER_INVALID_BINDING;
        return 0;
    }
    const int32_t candidates=driver->environment->tuning->laneCandidateCount;
    if(candidates>4){driver->result=CAR_DRIVER_INVALID_BINDING;return 0;}
    DriverCommand command;
    driverCommandInitialize(&command);
    DriverAIGoal goal;
    driverAIGoalInitialize(&goal);
    DriverAIRoadState road;
    driverAIRoadInitialize(&road,driver->kind);
    *lane=driverAILaneForClass(1,driver->physics.routeVehicleClass);
    const int32_t now=driver->physics.movement->bindings.environment->clock->worldTimer;
    driverAIRoadAssign(&road,group,pose,*lane,lockSelection,now);
    if(!group||!pose)return 0;
    for(int32_t selector=1;selector<=candidates;++selector){
        driverAIGoalSet(&goal,selector,NULL,0);
        const int32_t candidate=driverAILaneForClass(selector,driver->physics.routeVehicleClass);
        driverAIRoadSetLane(&road,candidate,lockSelection,now);
        if(evaluateRoadCandidate(driver,&goal,&command,&road)){
            *lane=driverAILaneForClass(selector,driver->physics.routeVehicleClass);
            return 1;
        }
        if(driver->result!=CAR_DRIVER_OK)return 0;
    }
    return 0;
}
/* This caller creates goals without a target. The original goal setter
 * therefore permits only road goals here. Keep the reachable road branches
 * together while the separate actor-path planner is recovered. */
void driverAIPlanCurrentAction(DriverAIBase* driver)
{
    DriverAIAction* action=driver->action;
    if(!action){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
    driverAIActionBuildPlan(action,driver,&driver->perception);
    if(driver->result!=CAR_DRIVER_OK)return;
    if(!action->pathValid){
        free(driver->action);
        driver->action=NULL;
        if(driver->kind!=CAR_DRIVER_AI){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
        driver->action=aiDriverCreateAction(driver->owner.ai,2);
        if(!driver->action||driver->result!=CAR_DRIVER_OK)return;
        driverAIActionBuildPlan(driver->action,driver,&driver->perception);
        if(driver->result!=CAR_DRIVER_OK)return;
    }
    if(driverAIGoalUsesRoad(driver->currentGoal->kind)){
        const int32_t lane=driverAILaneForClass(driver->currentGoal->kind,driver->physics.routeVehicleClass);
        if(lane!=driver->currentRoad->lane)
            driverAIRoadSetLane(driver->currentRoad,lane,0,
                driver->physics.movement->bindings.environment->clock->worldTimer);
    }
}
const DriverAIPolicy driverAIDefaultPolicy={0,0,&applicationLaunchOptions,0,1};
void driverAIOnCollision(DriverAIBase* driver,const CarMovement* other)
{
    CarMovement* movement=driver->physics.movement;
    const DriverAITuning* tuning=driver->environment->tuning;
    if (movement->mode==6) driver->replanDelayTicks=convertFloatToInt32(tuning->collisionReplanDelay);
    const int32_t mode=other && other->bindings.player ? 1:tuning->defaultMovementMode;
    if (mode!=movement->mode && carMovementSetMode(movement,mode)!=CAR_MOVEMENT_OK) {
        driver->result=CAR_DRIVER_MOVEMENT_FAILURE;return;
    }
    if (mode==1) driver->restoreModeDelayTicks=convertFloatToInt32(tuning->collisionRestoreDelay);
}
static const CarRecoveryGroup* groupById(const CarRecoveryPointRegistry* registry,int32_t id)
{
    if(id<0)return NULL;
    for(size_t index=0;index<registry->groupCount;++index)
        if(registry->groups[index].id==id)return &registry->groups[index];
    return NULL;
}
const CarRecoveryGroup* driverAICurrentGroup(DriverAIBase* driver,int32_t refreshIfMissing)
{
    if(refreshIfMissing && (driverAIRoadEmpty(driver->currentRoad) ||
        carRecoveryOwnedPoseIndex(driver->currentRoad->group,
                                  driver->currentRoad->recovery.recoveryPose)<0))
        driverAIRefreshRoad(driver);
    return driver->currentRoad->group;
}
const CarRecoveryPose* driverAICurrentPose(DriverAIBase* driver,int32_t refreshIfMissing)
{
    if(refreshIfMissing && (driverAIRoadEmpty(driver->currentRoad) ||
        carRecoveryOwnedPoseIndex(driver->currentRoad->group,
                                  driver->currentRoad->recovery.recoveryPose)<0))
        driverAIRefreshRoad(driver);
    return driver->currentRoad->recovery.recoveryPose;
}
void driverAIRefreshRoad(DriverAIBase* driver)
{
    DriverAIRoadState* road=driver->currentRoad;
    driverAIRoadInitialize(road,driver->kind);
    int32_t first=0,last=49;
    if(driver->kind==CAR_DRIVER_TRAFFIC){
        if(!driver->owner.traffic){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
        first=driver->owner.traffic->roadBranch?50:60;
        last=driver->owner.traffic->roadBranch?59:69;
    }else if(driver->kind!=CAR_DRIVER_AI){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
    driverAIFindRoad(driver,first,last);
}
void driverAIFindRoad(DriverAIBase* driver,int32_t firstGroup,int32_t lastGroup)
{
    const CarMovement* movement=driver->physics.movement;
    const CarRecoveryPointRegistry* registry=movement->bindings.recoveryPoints;
    const CarRecoveryGroup* previous=driver->currentRoad->group;
    const CarRecoveryGroup* selectedGroup=NULL;
    const CarRecoveryPose* selectedPose=NULL;
    float closest=999999.0f;
    for(int32_t pass=0;pass<2&&!selectedGroup;++pass){
        for(int32_t id=firstGroup;id<=lastGroup;++id){
            const CarRecoveryGroup* group=groupById(registry,id);
            if(!group)continue;
            if(pass==0&&!carRecoveryGroupContainsRouteDistance(group,movement->routeLocation.distance))continue;
            const CarRecoveryPose* pose=carRecoveryFindInGroup(registry,group,movement->bindings.state->position,999999.0f,0.0f,3u);
            if(!pose)continue;
            float point[4];driverRoadPointPosition(pose,point,0.0f,0.0f);
            const float x=movement->bindings.state->position[0]-point[0];
            const float y=movement->bindings.state->position[1]-point[1];
            const float z=movement->bindings.state->position[2]-point[2];
            const float xx=x*x;
            const float yy=y*y;
            const float zz=z*z;
            const float distance=sqrtf((xx+yy)+zz);
            if(distance<closest&&group!=previous){selectedGroup=group;selectedPose=pose;closest=distance;}
        }
    }
    if(selectedGroup&&selectedPose)driverAISetRoad(driver,selectedGroup,selectedPose);
    else driverAIRoadInitialize(driver->currentRoad,driver->kind);
}
void driverAISetRoad(DriverAIBase* driver,const CarRecoveryGroup* group,const CarRecoveryPose* pose)
{
    if(!group||!pose)return;
    if(carRecoveryOwnedPoseIndex(group,pose)<0) {
        driver->result=CAR_DRIVER_MISSING_ROAD;
        return;
    }
    driver->previousGroupId=driver->currentRoad->group?driver->currentRoad->group->id:-1;
    int32_t lane=0;
    driverAIChooseLane(driver,group,pose,1,&lane,NULL);
    if(driver->result!=CAR_DRIVER_OK)return;
    const int32_t now=driver->physics.movement->bindings.environment->clock->worldTimer;
    driverAIRoadAssign(driver->currentRoad,group,pose,lane,1,now);
}
void driverAIUpdateRoad(DriverAIBase* driver)
{
    DriverAIRoadState* road=driver->currentRoad;
    if(driverAIRoadEmpty(road)){
        driverAIRefreshRoad(driver);
        return;
    }
    const CarRecoveryGroup* group=road->group;
    const CarRecoveryPose* previous=road->recovery.recoveryPose;
    const int32_t previousIndex=carRecoveryOwnedPoseIndex(group,previous);
    if(previousIndex<0) {
        // A stale route after a track/Replay owner change must be reacquired
        // through the existing AI road refresh, not dereferenced as geometry.
        driverAIRefreshRoad(driver);
        return;
    }
    const float* position=driver->physics.movement->bindings.state->position;
    const float x=position[0]-previous->position[0];
    const float y=position[1]-previous->position[1];
    const float first=x*previous->direction[1];
    const float second=y*previous->direction[0];
    const float lateral=first-second;
    const float halfExtent=driver->perception.lateralExtent;
    if(previous->halfWidths[1]<lateral-halfExtent||lateral+halfExtent < -previous->halfWidths[0]){
        driverAIRefreshRoad(driver);
        return;
    }
    const CarRecoveryPose* following=carRecoveryFirstFollowingPose(group,position,previous);
    const int32_t atEnd=following==NULL;
    if(atEnd)following=&group->poses[group->poseCount-1];
    road->recovery.recoveryPose=following;
    int32_t changed=0;
    const int32_t followingIndex=carRecoveryOwnedPoseIndex(group,following);
    if(followingIndex<0) {
        driverAIRefreshRoad(driver);
        return;
    }
    for(int32_t index=previousIndex;index<=followingIndex;++index){
        const CarRecoveryPose* pose=&group->poses[index];
        if((pose->flags&255u)==255u)continue;
        const CarRecoveryConnection* connection=carRecoveryPointConnection(
            driver->physics.movement->bindings.recoveryPoints,pose);
        if(!connection){driver->result=CAR_DRIVER_MISSING_ROAD;return;}
        int32_t lane=-1;
        if(driverAISelectConnection(driver,connection,&lane,road)){
            driver->previousGroupId=driverAIRoadEmpty(road)?-1:road->group->id;
            const CarRecoveryPointRegistry* registry=driver->physics.movement->bindings.recoveryPoints;
            const CarRecoveryGroup* nextGroup=carRecoveryConnectionGroup(registry,connection);
            const CarRecoveryPose* nextPose=carRecoveryConnectionPose(registry,connection);
            if (carRecoveryOwnedPoseIndex(nextGroup,nextPose)<0) {
                driver->result=CAR_DRIVER_MISSING_ROAD;
                return;
            }
            const int32_t now=driver->physics.movement->bindings.environment->clock->worldTimer;
            driverAIRoadAssign(road,nextGroup,nextPose,lane,1,now);
            changed=1;
            if(connection->properties==1u)
                driver->connectionDelayTicks=convertFloatToInt32(driver->environment->tuning->connectionDelay);
            break;
        }
        if(driver->result!=CAR_DRIVER_OK)return;
    }
    if(atEnd&&!changed)driverAIRefreshRoad(driver);
}
#include "indep/inc/Timer.hpp"
static int32_t setMode(DriverAIBase* driver,int32_t mode)
{
    if(carMovementSetMode(driver->physics.movement,mode)==CAR_MOVEMENT_OK)return 1;
    driver->result=CAR_DRIVER_MOVEMENT_FAILURE;
    return 0;
}

void driverAIUpdateMovementMode(DriverAIBase* driver)
{
    CarMovement* movement=driver->physics.movement;
    const auto* policy=driver->environment->policy;
    // PAL1919B0: NeedBelt=false also exits an already active belt mover.
    if(policy->beltMovementEnabled && driverAICanUseBelt(driver)){
        if(movement->mode!=6&&!setMode(driver,6))return;
        beltMoverActivate(movement->belt);
    }else if(movement->mode==6)
        (void)setMode(driver,driver->environment->tuning->defaultMovementMode);
}
static void setWaitingInterval(DriverAIBase* driver,float randomRange,float base)
{
    const float interval=vectorDefaultRandomFloat(randomRange)+base;
    const float ticks=interval*4000.0f;
    driver->waitIntervalTicks=convertFloatToInt32(ticks+0.5f);
    const int32_t now=driver->physics.movement->bindings.environment->clock->worldTimer;
    const float seconds=(float)now*0.0002500000118743628f;
    const float roundedTicks=seconds*4000.0f;
    driver->waitStartTicks=convertFloatToInt32(roundedTicks+0.5f);
}
static DriverAIAction* createCruise(DriverAIBase* driver)
{
    DriverAIAction* action=(DriverAIAction*)malloc(sizeof(*action));
    if(!action){driver->result=CAR_DRIVER_ALLOCATION_FAILURE;return NULL;}
    return driverAIActionInitializeCruise(action,0,-1.0f,
        driver->physics.movement->bindings.environment->clock->worldTimer);
}
int32_t driverAICanUseBelt(const DriverAIBase* driver)
{
    const CarMovement* movement=driver->physics.movement;
    if(driver->controls->command.count<3||*movement->bindings.applicationState==8||
        timerRunning(driver->replanDelayTicks)||timerRunning(driver->restoreModeDelayTicks)||
        movement->bindings.state->forwardSpeed<5.0f||movement->bindings.state->activeContactCount<=0||
        !driver->action)return 0;
    const int32_t kind=driver->action->kind;
    if(kind==6||kind==7||kind==10||kind==8||kind==9||
        movement->bindings.owner->driverInfo->behaviorProfile==-2)return 0;
    const WorldSimulation* simulation=driver->environment->simulation;
    if(worldGetDistanceToClosestPlayer3D(simulation->world,movement->bindings.state->position)<120.0f)return 0;
    return !((float)simulation->race->elapsedTicks*0.0002500000118743628f<30.0f);
}
static void sampleProgress(DriverAIBase* driver,float elapsed)
{
    if(!driver->action||driver->action->kind==6||!timerRunning(driver->progressSampleTicks)||
        gameTimerAdvance(&driver->progressSampleTicks,elapsed))return;
    CarMovement* movement=driver->physics.movement;
    const RaceCarEntry* entry=movement->bindings.owner->raceEntry;
    if(!entry||!movement->bindings.route){driver->result=CAR_DRIVER_MISSING_ROAD;return;}
    driver->progressSampleTicks=6000;
    const float rate=fabsf(entry->progressRate);
    driverHistoryPush(&driver->progressHistory,movement->bindings.route->length*rate);
    const WorldSimulation* simulation=driver->environment->simulation;
    const int32_t nearPlayer=worldGetDistanceToClosestPlayer3D(simulation->world,
        movement->bindings.state->position)<50.0f;
    const int32_t now=movement->bindings.environment->clock->worldTimer;
    if(!carMotionWindowActive(movement->bindings.ownerMotion,now)&&
        driver->progressHistory.count==driver->progressHistory.capacity&&
        driver->progressHistory.value<0.20000000298023224f&&!nearPlayer)
        carMotionRequestRecovery(movement->bindings.ownerMotion,&movement->bindings.environment->recovery);
}
static int32_t selectedThisFrame(const DriverAIBase* driver)
{
    const WorldSimulation* simulation=driver->environment->simulation;
    const World* world=simulation->world;
    const Car* car=driver->physics.movement->bindings.owner;
    // Scheduling uses simulation iterations. The elapsed-time frame
    // accumulator can advance by more than one and skip eligible Cars.
    const int32_t frame=simulation->clock->worldLoopCounter;
    const int32_t group=car->driverInfo->driverClass;
    if(group==2){
        const int32_t phase=frame%8;
        return phase<world->secondGroupEnd-world->firstGroupEnd&&phase>=0&&
            world->carTable[world->firstGroupEnd+phase]==car;
    }
    if(group==3){
        const int32_t phase=frame%6;
        return phase<world->totalCars-world->thirdGroupEnd&&phase>=0&&
            world->carTable[world->thirdGroupEnd+phase]==car;
    }
    return driver->thinkDelay<=driver->activePerception->lateralExtent;
}
static void updateTargetPath(DriverAIBase* driver)
{
    DriverAIGoal* goal=driver->currentGoal;
    if(!driverAIGoalUsesTarget(goal->kind)||!goal->target->bindings.player)return;
    driver->result=driverPerceptionUpdate(driver->activePerception,&driver->physics,
        driver->environment,driver->currentRoad!=NULL);
    if(driver->result!=CAR_DRIVER_OK)return;
    if(!driverPerceptionFind(driver->activePerception,goal->target))return;
    DriverCommand command;
    driverCommandInitialize(&command);
    driverLanePlanGoal(goal,&command,driver->currentRoad,driver->activePerception,driver,0);
    if(driver->result==CAR_DRIVER_OK)aiControlsSetCommand(driver->controls,&command);
}
void driverAIThink(DriverAIBase* driver,float elapsed)
{
    CarMovement* movement=driver->physics.movement;
    const DriverAIEnvironment* environment=driver->environment;
    WorldSimulation* simulation=environment->simulation;
    const DriverAIPolicy* policy=environment->policy;
    const DriverAITuning* tuning=environment->tuning;
    if(!policy||!tuning||!environment->specialPursuit||!simulation||!simulation->race||
        !simulation->clock||!simulation->world||!simulation->collisions||!movement->bindings.applicationState){
        driver->result=CAR_DRIVER_INVALID_BINDING;return;
    }
    const int32_t now=movement->bindings.environment->clock->worldTimer;
    if(!raceFrameIsDriving(simulation->race,*environment->specialPursuit)){
        float targetSpeed=0.0f;
        if(simulation->race->state>=2){
            if(!timerRunning(driver->waitIntervalTicks)){
                driver->waitMoving=vectorDefaultRandomBounded(2)!=0;
                if(driver->waitMoving)setWaitingInterval(driver,0.25f,0.3499999940395355f);
                else setWaitingInterval(driver,1.0f,1.5f);
            }else{
                const float interval=(float)driver->waitIntervalTicks*0.0002500000118743628f;
                const int32_t ticks=(int32_t)((uint32_t)now-(uint32_t)driver->waitStartTicks);
                const float seconds=(float)ticks*0.0002500000118743628f;
                if(interval<seconds){
                    driver->waitMoving^=1;
                    if(driver->waitMoving)setWaitingInterval(driver,0.25f,0.3499999940395355f);
                    else setWaitingInterval(driver,1.0f,1.5f);
                }
            }
            if(driver->waitMoving)targetSpeed=100.0f;
        }
        aiControlsSetTarget(driver->controls,1,targetSpeed,0.0f,0.0f);
        driverAIPointAlongHeading(driver);
        return;
    }
    sampleProgress(driver,elapsed);
    if(driver->result!=CAR_DRIVER_OK)return;
    if(timerRunning(driver->restoreModeDelayTicks)&&
        !gameTimerAdvance(&driver->restoreModeDelayTicks,elapsed)){
        if(movement->bindings.state->activeContactCount<4)driver->restoreModeDelayTicks=4000;
        else if(!setMode(driver,tuning->defaultMovementMode))return;
    }
    if(timerRunning(driver->replanDelayTicks)){
        if(movement->mode==6&&!setMode(driver,tuning->defaultMovementMode))return;
        gameTimerAdvance(&driver->replanDelayTicks,elapsed);
    }
    gameTimerAdvance(&driver->linkedPlayerRemainingTicks,elapsed);
    gameTimerAdvance(&driver->linkedPlayerCooldownTicks,elapsed);
    gameTimerAdvance(&driver->connectionDelayTicks,elapsed);
    DriverAIPerception* perception=driver->activePerception;
    driver->thinkDelay=driver->thinkDelay-elapsed;
    perception->count=0;perception->lateralExtent=0.0f;perception->forwardExtent=0.0f;
    if(!selectedThisFrame(driver)&&driver->action){updateTargetPath(driver);return;}
    driver->thinkDelay=driver->thinkDelay+0.10000000149011612f;
    driverAIConnectionsUpdate(driver);
    if(driver->result!=CAR_DRIVER_OK)return;
    driver->result=driverPerceptionUpdate(perception,&driver->physics,environment,driver->currentRoad!=NULL);
    if(driver->result!=CAR_DRIVER_OK)return;
    driverAIUpdateRoad(driver);
    if(driver->result!=CAR_DRIVER_OK)return;
    if(driverAIRoadEmpty(driver->currentRoad)){
        driverAIPointAlongHeading(driver);
        aiControlsSetTarget(driver->controls,1,0.0f,0.0f,0.0f);
        return;
    }
    if(simulation->collisions->carPolicy.collisionDisabled){
        if(!driver->action)driver->action=createCruise(driver);
        else if(driver->action->kind!=5){
            free(driver->action);driver->action=NULL;
            if(driver->kind==CAR_DRIVER_AI)driver->action=aiDriverCreateAction(driver->owner.ai,5);
        }else driverAIActionBuildPlan(driver->action,driver,&driver->perception);
    }else{
        if(!driver->action)driver->action=createCruise(driver);
        else{
            driverAIActionUpdateStatus(driver->action,driver,&driver->perception);
            if(driver->result!=CAR_DRIVER_OK)return;
            if(driverAIActionFinished(driver->action)){
                free(driver->action);driver->action=NULL;
                if(driver->kind==CAR_DRIVER_AI)driver->action=aiDriverCreateAction(driver->owner.ai,11);
            }else{
                DriverAIAction* selected=driverAISelectAction(driver);
                if(selected){free(driver->action);driver->action=selected;}
            }
        }
        if(driver->action&&driver->result==CAR_DRIVER_OK)driverAIPlanCurrentAction(driver);
    }
    if(driver->result!=CAR_DRIVER_OK)return;
    if(!driver->action){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
    driverAIApplyAction(driver);
    if(driver->result!=CAR_DRIVER_OK)return;
    driverAIUpdateMovementMode(driver);
    if(driver->result!=CAR_DRIVER_OK)return;
    perception->forwardExtent=0.0f;perception->count=0;perception->lateralExtent=0.0f;
}
static CarMovement* movementForCar(const WorldSimulation* simulation,const Car* car)
{
    const size_t index=car->tableIndex;
    if(index>=simulation->frameCount||index>=WORLD_MAX_CARS||
       simulation->world->carTable[index]!=car||!simulation->frames[index])return NULL;
    return carFrameMovement(simulation->frames[index]);
}
CarDriverResult driverPerceptionUpdate(DriverAIPerception* perception,
    const DriverPhysicsVehicle* vehicle,const DriverAIEnvironment* environment,int32_t enabled)
{
    if(!perception)return CAR_DRIVER_INVALID_BINDING;
    if(!perception->movement||!enabled)return CAR_DRIVER_OK;
    if(!vehicle||!vehicle->movement||perception->movement!=vehicle->movement)
        return CAR_DRIVER_INVALID_BINDING;
    if(!environment||!environment->simulation||!environment->simulation->world||
       !environment->simulation->frames)return CAR_DRIVER_INVALID_BINDING;
    const WorldSimulation* simulation=environment->simulation;
    CarMovement* movement=vehicle->movement;
    VehicleState* state=movement->bindings.state;
    memset(perception->neighbors,0,sizeof(perception->neighbors));
    float range=driverPlanningDistance(vehicle);
    const float acceleration=driverAvailableAcceleration(vehicle,state->speed);
    if(movement->bindings.ownerMotion->inputCategory==CAR_DRIVER_TRAFFIC)
        range*=vehicle->tuning->trafficDistanceScale;
    int32_t count=0;
    const Car* root=&simulation->world->carList;
    for(const Car* car=root->next;car!=root;car=car->next){
        if(count>=DRIVER_AI_NEIGHBORS)continue;
        CarMovement* other=movementForCar(simulation,car);
        if(!other)return CAR_DRIVER_INVALID_BINDING;
        const CarMotionState* motion=other->bindings.ownerMotion;
        if(((motion->transitionFlag^1)&(int32_t)motion->actionEnabled)==0)continue;
        DriverAINeighbor* neighbor=&perception->neighbors[count];
        const VehicleState* otherState=other->bindings.state;
        driverPerceptionLocalPoint(state,neighbor->localPosition,otherState->position);
        for(int lane=0;lane<2;++lane)
            neighbor->travelRelativeVelocity[lane]=otherState->velocity[lane]-state->velocity[lane];
        neighbor->movement=other;
        const float xSquared=neighbor->localPosition[0]*neighbor->localPosition[0];
        const float ySquared=neighbor->localPosition[1]*neighbor->localPosition[1];
        const float distance=sqrtf(xSquared+ySquared);
        const float routeDistance=fabsf(trackRouteDistanceDifference(movement->bindings.route,
            other->routeLocation.distance,movement->routeLocation.distance));
        if(range<distance&&!driverPerceptionNeighborCanReach(neighbor,vehicle,acceleration))continue;
        if(range*3.0f<routeDistance)continue;
        ++count;
        const uint16_t heading=vectorAtan2Angle(state->velocity[0],state->velocity[1]);
        float yaw[16],velocity[4]={neighbor->travelRelativeVelocity[0],
            neighbor->travelRelativeVelocity[1],otherState->velocity[2]-state->velocity[2],0.0f};
        rotationMatrixIdentity(yaw);
        motionMatrixRotateYaw(yaw,yaw,(uint16_t)(0x4000u-heading));
        vectorMatrixTransformPoint(velocity,yaw,velocity);
        neighbor->travelRelativeVelocity[0]=velocity[0];
        neighbor->travelRelativeVelocity[1]=velocity[1];
        float minimum[4],maximum[4];
        DriverPhysicsVehicle otherVehicle=*vehicle;otherVehicle.movement=other;
        driverCollisionBounds(&otherVehicle,minimum,maximum);
        const float halfX=(maximum[0]-minimum[0])*0.5f;
        const float halfY=(maximum[1]-minimum[1])*0.5f;
        const float cosine=vectorCosineAngle((uint16_t)(otherState->heading-state->heading));
        const float weight=cosine*cosine;
        const float complement=1.0f-weight;
        neighbor->effectiveHalfExtents[0]=weight*halfY+complement*halfX;
        neighbor->effectiveHalfExtents[1]=weight*halfX+complement*halfY;
    }
    perception->count=count;
    DriverAINeighbor sorted[DRIVER_AI_NEIGHBORS];
    memcpy(sorted,perception->neighbors,sizeof(sorted));
    DriverAINeighbor* order[DRIVER_AI_NEIGHBORS];
    for(int32_t index=0;index<count;++index)order[index]=&sorted[index];
    for(int32_t pass=0;pass<count;++pass){
        for(int32_t index=0;index<count-1;++index){
            if(order[index]->localPosition[1]<order[index+1]->localPosition[1]){
                DriverAINeighbor* swap=order[index];order[index]=order[index+1];order[index+1]=swap;
            }
        }
    }
    for(int32_t index=0;index<count;++index)perception->neighbors[index]=*order[index];
    float minimum[4],maximum[4];
    driverCollisionBounds(vehicle,minimum,maximum);
    perception->lateralExtent=maximum[1];
    perception->forwardExtent=maximum[0];
    return CAR_DRIVER_OK;
}


void driverAIReactToTarget(DriverAIBase* driver,CarMovement* target,
    const DriverAIPerception* perception)
{
    const int32_t bodyCategory=target->bindings.info->bodyCategory;
    if(bodyCategory==1||bodyCategory==2)return;
    CarMovement* own=driver->physics.movement;
    if(!own->bindings.applicationState){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
    if(*own->bindings.applicationState!=8&&target->bindings.ownerMotion->inputCategory!=1)return;
    const DriverAINeighbor* neighbor=driverPerceptionFind(driver->activePerception,target);
    if(!neighbor){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
    const int32_t category=own->bindings.ownerMotion->inputCategory;
    if(category==2){
        if(!driverCollisionNeighborEligible(neighbor,perception))return;
    }else if(category!=3)return;
    WorldSimulation* simulation=driver->environment->simulation;
    const uint32_t index=own->bindings.owner->tableIndex;
    if(index>=simulation->frameCount||!simulation->frames||!simulation->frames[index]||
        carFrameMovement(simulation->frames[index])!=own){
        driver->result=CAR_DRIVER_INVALID_BINDING;return;
    }
    World* world=simulation->world;
    // PAL1930B4..1930F4: retain each scalar rounding boundary after the
    // actual World RNG calls, including the separate final-release offset.
    const float first=bScalarAdd(worldRandomFloat(world,0.4000000059604645f),0.30000001192092896f);
    const float interval=bScalarAdd(first,-0.20000000298023224f);
    const float second=bScalarAdd(worldRandomFloat(world,0.20000000298023224f),0.15000000596046448f);
    const float offset=bScalarAdd(second,-0.10000000149011612f);
    const float third=bScalarAdd(worldRandomFloat(world,0.4000000059604645f),0.30000001192092896f);
    float duration=bScalarAdd(third,-0.20000000298023224f);
    int32_t phase=worldRandomFloat(world,1.0f)<0.5f;
    float controlChance=1.0f;
    if(50.0f<neighbor->travelRelativeVelocity[1]){
        phase=1;
        duration=bScalarAdd(duration,0.6000000238418579f);
    }else if(!(own->bindings.owner->driverInfo->driverClass==3&&neighbor->travelRelativeVelocity[1]<-3.0f)){
        if(target->bindings.player)controlChance=0.75f;
        else controlChance=perception->count<6?0.07500000298023224f:0.05000000074505806f;
    }
    const int32_t control=worldRandomFloat(world,1.0f)<controlChance;
    int32_t visual=0;
    if(control){
        const float visualChance=50.0f<neighbor->travelRelativeVelocity[1]?0.8999999761581421f:0.5f;
        visual=worldRandomFloat(world,1.0f)<visualChance;
    }
    carFrameBeginBlink(simulation->frames[index],visual,control,phase,interval,offset,duration);
}


/* PAL-only controller stop-delay helper, PAL1908D8..1908F0.
 * Original callers pass their actual AICarController; no Beta method name
 * is inferred for this release addition. */

void driverAISetStopDelay(DriverAIBase* driver,float seconds)
{
    driver->stopDelaySeconds=minimumFinite(driver->stopDelaySeconds,seconds);
    driver->actionCode=6;
}
