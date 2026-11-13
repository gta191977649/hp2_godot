#include "Race.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/PsychoDriver.hpp"
#include "indep/inc/Fred.hpp"
#include "EventTree.hpp"
#include "TrackedController.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/src/speech/mgr.hpp"
#include "Race.hpp"
#include <limits.h>

#include "indep/inc/Race.hpp"
#include "indep/inc/Car.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/PhysicsCar.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/GhostCar.hpp"
#include "indep/inc/Replay.hpp"

#include <math.h>
#include <string.h>
#include <cmath>
#include <new>
#include "bware/inc/bWare.hpp"

// Complete PAL1490F8..149154. Strict greater-than retains the earlier
// creation-order Player on equal times; negative/nonpositive timers cannot
// replace the initial zero. This is MostRecentlyFinished, not first place.
CarFramePlayerState* raceFrameMostRecentlyFinishedPlayer(const RaceFrame* race)
{
    const auto* players=race->services->players;
    int32_t mostRecent=0;
    CarFramePlayerState* found=nullptr;
    for(int32_t i=0;i<players->count;++i) {
        auto* player=players->players[i];
        if(player->drivingLocked && mostRecent<player->drivingLocked) {
            mostRecent=player->drivingLocked;
            found=player;
        }
    }
    return found;
}

// Complete PAL148F68..148FF4. FinishCar can alter the ordered view, so the
// released loop reloads the participant at each index, then sorts once more.
WorldServicesResult raceFrameSimulateFinish(RaceFrame* race)
{
    if (!race || !race->services || race->entryCount>race->entryCapacity ||
        (race->entryCount && !race->entries)) return WORLD_SERVICES_INVALID_BINDING;
    for (size_t index=0;index<race->entryCount;++index) {
        const auto* entry=race->entries[index];
        if (!entry) return race->services->result=WORLD_SERVICES_INVALID_BINDING;
        if (!entry->completionReason) {
            const auto result=raceFrameFinishParticipant(race,index,0,0,1);
            if (result!=WORLD_SERVICES_OK) return result;
        }
    }
    raceFrameSortEntries(race);
    return race->services->result=WORLD_SERVICES_OK;
}

// Complete PAL148FF8..149078. Recording is performed for every live row in
// its actual final order, including AI entries; a prior single-player result
// cannot stand in for this notification.
WorldServicesResult raceFrameNotifyRaceOver(RaceFrame* race)
{
    if (!race || !race->services || !race->services->currentEvent ||
        race->entryCount>race->entryCapacity || (race->entryCount && !race->entries))
        return WORLD_SERVICES_INVALID_BINDING;
    for (size_t index=0;index<race->entryCount;++index) {
        const auto* entry=race->entries[index];
        if (!entry || !entry->car ||
            !race->services->currentEvent->RecordResult(*race,*entry,entry->position))
            return race->services->result=WORLD_SERVICES_INVALID_BINDING;
    }
    return race->services->result=WORLD_SERVICES_OK;
}

// Complete PAL146C90..146E04. The source deliberately omits the Car pointer
// and most timer arrays. Only time-trial mode transfers its three selected
// timers, including the duplicate checkpoint[0] transfer when finish==0.
void raceCarEntryDoSnapshot(RaceCarEntry* entry,const RaceFrame* race,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!entry || !race || !race->services || !race->services->settings ||
        race->finishCheckpoint<0 || race->finishCheckpoint>=5) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
    }
    replaySnapshotBeginSection(snapshot,nullptr,"RacingCar");
    replaySnapshotMoveData(snapshot,&entry->currentCheckpoint,2,2,1);
    replaySnapshotMoveData(snapshot,&entry->furthestCheckpoint,2,2,1);
    replaySnapshotMoveData(snapshot,&entry->lap,1,1,1);
    replaySnapshotMoveData(snapshot,&entry->furthestLap,1,1,1);
    replaySnapshotMoveData(snapshot,&entry->position,1,1,1);
    replaySnapshotMoveData(snapshot,&entry->previousPosition,1,1,1);
    replaySnapshotMoveData(snapshot,&entry->finishPosition,1,1,1);
    replaySnapshotMoveData(snapshot,&entry->advancedLap,1,1,1);
    replaySnapshotMoveData(snapshot,&entry->advancedCheckpoint,1,1,1);
    replaySnapshotMoveData(snapshot,&entry->updateCounter,2,2,1);
    replaySnapshotMoveData(snapshot,&entry->completionReason,2,2,1);
    replaySnapshotMoveFloat(snapshot,&entry->progress);
    replaySnapshotMoveFloat(snapshot,&entry->progressRate);
    replaySnapshotMoveFloat(snapshot,&entry->previousPercent);
    replaySnapshotMoveFloat(snapshot,&entry->displayProgress);
    if(race->services->settings->raceKind==2) {
        replaySnapshotMoveData(snapshot,&entry->checkpointTicks[0],4,4,0);
        replaySnapshotMoveData(snapshot,&entry->checkpointTicks[race->finishCheckpoint],4,4,0);
        replaySnapshotMoveData(snapshot,&entry->segmentTicks[race->finishCheckpoint],4,4,0);
    }
    replaySnapshotEndSection(snapshot);
}

// Complete PAL1477D0..147960. Embedded participant order and the sorted
// pointer view are separate: each physical RacingCar is transferred first,
// then the ordered pointer at that same ordinal is encoded as one byte.
void raceFrameOwnerDoSnapshot(RaceFrameOwner* owner,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!owner || owner->race.entries!=owner->ordered ||
        owner->race.entryCount>RACE_PARTICIPANT_LIMIT || !owner->race.services ||
        !owner->race.services->settings || owner->race.finishCheckpoint<0 ||
        owner->race.finishCheckpoint>=5) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
    }
    auto& race=owner->race;
    replaySnapshotBeginSection(snapshot,nullptr,"Race");
    // The state enum overload writes exactly its low byte without extending
    // the other three native scalar bytes (PAL13CF08, maximumBytes=-1).
    replaySnapshotMoveData(snapshot,&race.state,1,1,0);
    replaySnapshotMoveInteger(snapshot,&race.startedWorldFrame,4);
    replaySnapshotMoveInteger(snapshot,&race.countdownWorldFrame,4);
    replaySnapshotMoveData(snapshot,&race.startTicks,4,4,0);
    replaySnapshotMoveData(snapshot,&race.elapsedTicks,4,4,0);
    // MoveDataForChecksumOnly13D0D0 checks the high two mode bytes. Counts
    // are not part of normal recording/restoration; the real owners stay
    // allocated. Preserve the actual mixed-mode precedence of MoveData.
    if(snapshot->recordingChecksums || snapshot->verifyingChecksums) {
        int32_t count=static_cast<int32_t>(race.entryCount);
        replaySnapshotMoveInteger(snapshot,&count,1);
        if(count<0 || count>RACE_PARTICIPANT_LIMIT) {
            snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;
            replaySnapshotEndSection(snapshot);return;
        }
        race.entryCount=static_cast<size_t>(count);
    }
    replaySnapshotMoveInteger(snapshot,&race.finishedCount,1);
    replaySnapshotMoveUnsigned(snapshot,&race.snapshotFlags,1);
    for(size_t index=0;index<race.entryCount;++index) {
        raceCarEntryDoSnapshot(&owner->participants[index],&race,snapshot);
        int8_t ordinal=-1;
        for(size_t physical=0;physical<RACE_PARTICIPANT_LIMIT;++physical)
            if(race.entries[index]==&owner->participants[physical])ordinal=static_cast<int8_t>(physical);
        if(ordinal<0) {
            snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
            replaySnapshotEndSection(snapshot);return;
        }
        replaySnapshotMoveData(snapshot,&ordinal,1,1,1);
        if(ordinal<0 || ordinal>=RACE_PARTICIPANT_LIMIT) {
            snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;
            replaySnapshotEndSection(snapshot);return;
        }
        race.entries[index]=&owner->participants[static_cast<size_t>(ordinal)];
    }
    if(snapshot->recordingChecksums || snapshot->verifyingChecksums)
        replaySnapshotMoveInteger(snapshot,&race.targetLapCount,1);
    replaySnapshotEndSection(snapshot);
}

// PAL146AE8..146B8C. Integer laps and the256 fractional bins retain the
// source truncation and arithmetic order. NULL output is a real bin query.
const RaceTimeAroundTrackBin* raceTimeAroundTrackEntry(const RaceTimeAroundTrackTable* table,
    float progress,float* seconds)
{
    if(!table || !std::isfinite(progress) || progress>=2147483648.0f)return nullptr;
    if(progress<0.0f)progress=0.0f;
    const int32_t laps=convertFloatToInt32(progress);
    const float whole=static_cast<float>(laps);
    const float fraction=progress-whole;
    const int32_t index=convertFloatToInt32(fraction*256.0f);
    if(index<0 || index>=256)return nullptr;
    const RaceTimeAroundTrackBin* bin=&table->bins[index];
    if(seconds) {
        const float start=static_cast<float>(index)*0.00390625f;
        const float inBin=fraction-start;
        const float priorLaps=whole*table->lapSeconds;
        const float priorTime=priorLaps+bin->secondsAtStart;
        *seconds=priorTime+finiteRatio(inBin,bin->progressPerSecond);
    }
    return bin;
}

// PAL146B90..146BB0, GetTimeAroundTrack. The checked native result makes a
// missing table explicit; it never substitutes a constant lap duration.
WorldServicesResult raceTimeAroundTrackEvaluate(const RaceTimeAroundTrackTable* table,
    float progress,float* seconds)
{
    return seconds && raceTimeAroundTrackEntry(table,progress,seconds) ?
        WORLD_SERVICES_OK : WORLD_SERVICES_INVALID_BINDING;
}

// PAL1467A0..146AE8. Class E6 selects every second authored lane record in
// the source category0 group list. All coordinates/speeds are borrowed from
// the existing road owner and transformed by the real route/checkpoint code.
WorldServicesResult raceTimeAroundTrackBuild(RaceTimeAroundTrackTable* table,const RaceFrame* race,
    const CarRecoveryPointRegistry* road,int32_t vehicleClass)
{
    if(!table || !race || !road || !road->route || road->route!=race->route ||
        (road->groupCount && !road->groups) || vehicleClass<0 || vehicleClass>10)
        return WORLD_SERVICES_INVALID_BINDING;
    memset(table->bins,0,sizeof(table->bins));table->lapSeconds=0.0f;
    const size_t lane=static_cast<size_t>(vehicleClass)*2;
    for(size_t i=0;i<road->groupCount;++i) {
        const CarRecoveryGroup& group=road->groups[i];
        if(group.category!=0 || !(group.laneFlags[lane]&3u))continue;
        if(group.poseCount<0 || (group.poseCount && !group.poses))return WORLD_SERVICES_INVALID_BINDING;
        TrackRouteLocation location;trackRouteLocationInitialize(&location);
        for(int32_t j=0;j<group.poseCount-1;++j) {
            const CarRecoveryPose& first=group.poses[j];
            const CarRecoveryPose& second=group.poses[j+1];
            const float firstAcross=static_cast<float>(static_cast<int32_t>(first.laneOffsetsQuantized[lane])*256)*0.0000152587890625f;
            const float secondAcross=static_cast<float>(static_cast<int32_t>(second.laneOffsetsQuantized[lane])*256)*0.0000152587890625f;
            float firstPoint[2],secondPoint[2];
            carRoadPreviewPointXY(&first,firstPoint,0.0f,firstAcross);
            carRoadPreviewPointXY(&second,secondPoint,0.0f,secondAcross);
            trackRouteUpdateLocation(road->route,&location,firstPoint);
            float begin=raceFrameCheckpointFraction(race,location.progress*100.0f);
            trackRouteUpdateLocation(road->route,&location,secondPoint);
            float end=raceFrameCheckpointFraction(race,location.progress*100.0f);
            float delta=end-begin;
            if(0.5f<delta)begin+=1.0f;
            else if(0.5f<begin-end)end+=1.0f;
            delta=end-begin;
            const float dx=firstPoint[0]-secondPoint[0],dy=firstPoint[1]-secondPoint[1];
            const float xx=dx*dx,yy=dy*dy;
            const float distance=std::sqrt(xx+yy);
            const float speed=static_cast<float>(static_cast<uint32_t>(first.laneSpeeds[lane])*65536u)*0.0000152587890625f;
            const float segmentSeconds=finiteRatio(distance,speed);
            const float rate=finiteRatio(delta,segmentSeconds);
            const RaceTimeAroundTrackBin* entry=raceTimeAroundTrackEntry(table,begin,nullptr);
            if(!entry)return WORLD_SERVICES_INVALID_BINDING;
            if(0.0f<rate && entry->progressPerSecond==0.0f)
                table->bins[static_cast<size_t>(entry-table->bins)].progressPerSecond=rate;
        }
    }
    float seconds=0.0f,previousRate=0.0055555556900799274f;
    for(auto& bin:table->bins) {
        if(bin.progressPerSecond==0.0f)bin.progressPerSecond=previousRate;
        bin.secondsAtStart=seconds;
        previousRate=bin.progressPerSecond;
        const float duration=finiteRatio(0.00390625f,bin.progressPerSecond);
        seconds+=duration;
    }
    table->lapSeconds=seconds;
    return WORLD_SERVICES_OK;
}

// PAL147960..1479F4. The embedded participant storage is distinct from the
// sorted entry-pointer view. Init selects physical row0, or row1 in mode2.
WorldServicesResult raceFrameOwnerInitStage(RaceFrameOwner* owner,int32_t stage,
    const CarRecoveryPointRegistry* road)
{
    if(!owner)return WORLD_SERVICES_INVALID_BINDING;
    RaceFrame* race=&owner->race;
    auto status=raceFrameOwnerPopulate(owner);
    if(status!=WORLD_SERVICES_OK)return status;
    status=raceFrameSetState(race,stage);
    if(status!=WORLD_SERVICES_OK)return status;
    if(!race->services || !race->services->settings || race->timeTable)
        return WORLD_SERVICES_INVALID_BINDING;
    const auto* settings=race->services->settings;
    const size_t selected=settings->specialPursuit && settings->mode==2 ? 1u : 0u;
    if(selected>=race->entryCount || !owner->participants[selected].car)
        return WORLD_SERVICES_INVALID_BINDING;
    const Car* car=owner->participants[selected].car;
    const auto* movement=static_cast<const CarMovement*>(car->movement);
    if(!movement || movement->bindings.owner!=car)return WORLD_SERVICES_INVALID_BINDING;
    auto* table=new(std::nothrow) RaceTimeAroundTrackTable;
    if(!table)return WORLD_SERVICES_CAPACITY;
    status=raceTimeAroundTrackBuild(table,race,road,movement->info.routeVehicleClass);
    if(status!=WORLD_SERVICES_OK){delete table;return status;}
    race->timeTable=table;
    return WORLD_SERVICES_OK;
}

void raceFrameDisposeTimeTable(RaceFrame* race)
{
    if(!race)return;
    delete race->timeTable;race->timeTable=nullptr;
}

RaceCarEntry* raceCarEntryResetTiming(RaceCarEntry* entry)
{
    memset(entry->checkpointTicks, 0, sizeof(entry->checkpointTicks));
    memset(entry->segmentTicks, 0, sizeof(entry->segmentTicks));
    memset(entry->previousSegmentTicks, 0, sizeof(entry->previousSegmentTicks));
    memset(entry->bestSegmentTicks, 0, sizeof(entry->bestSegmentTicks));
    memset(entry->previousBestSegmentTicks, 0, sizeof(entry->previousBestSegmentTicks));
    memset(entry->lapTicks, 0, sizeof(entry->lapTicks));
    entry->finishTicks = 0;
    return entry;
}

RaceCarEntry* raceFrameEntryForCar(const Car* car)
{
    return car->raceEntry;
}

void raceFrameSortEntries(RaceFrame* race)
{
    int32_t changed;
    do {
        changed = 0;
        for (size_t index = 0; index + 1 < race->entryCount; ++index) {
            RaceCarEntry* first = race->entries[index];
            RaceCarEntry* second = race->entries[index + 1];
            int32_t swap = 0;
            if (first->finishPosition != 0)
                swap = (int32_t)(index + 1) < (int32_t)first->finishPosition;
            else if (second->finishPosition != 0)
                swap = (int32_t)second->finishPosition < (int32_t)(index + 2);
            else swap = first->displayProgress < second->displayProgress;
            if (swap) {
                race->entries[index] = second;
                race->entries[index + 1] = first;
                changed = 1;
            }
        }
    } while (changed);
    for (size_t index = 0; index < race->entryCount; ++index) {
        RaceCarEntry* entry = race->entries[index];
        entry->previousPosition = entry->position;
        entry->position = (int8_t)(index + 1);
    }
}

RaceCarEntry* raceFrameFindFinishPosition(const RaceFrame* race, int32_t position)
{
    for (size_t index = 0; index < race->entryCount; ++index)
        if ((int32_t)race->entries[index]->finishPosition == position) return race->entries[index];
    return NULL;
}

int32_t raceFrameNextFinishPosition(const RaceFrame* race)
{
    int32_t position = 1;
    while (raceFrameFindFinishPosition(race, position)) ++position;
    return position;
}

static WorldServicesResult fail(RaceFrame* race, WorldServicesResult result)

{

    if (race && race->services) race->services->result = result;

    return result;

}

static int32_t validRace(const RaceFrame* race)

{

    return race && race->services && race->services->settings && race->clock &&

        race->entryCount <= 8 && race->entryCount <= race->entryCapacity &&

        (!race->entryCount || race->entries) && race->finishCheckpoint >= 0 &&

        race->finishCheckpoint < 5;

}

WorldServicesResult raceFrameSetState(RaceFrame* race, int32_t state)

{

    RaceFrameServices* services;

    if (!validRace(race)) return fail(race, WORLD_SERVICES_INVALID_BINDING);

    services = race->services;

    if (!services->world || !services->profile || !services->currentEvent)

        return fail(race, WORLD_SERVICES_INVALID_BINDING);

    race->state = state;

    if (state == 4 && !services->profile->frontend)
        return fail(race, WORLD_SERVICES_INVALID_BINDING);

    if (state == 4 && raceProfileUsesResults(services->profile))

        services->currentEvent->CaptureStartState();

    for (size_t index = 0; index < race->entryCount; ++index) {

        RaceCarEntry* entry = race->entries[index];

        CarDriver* driver = (CarDriver*)entry->car->controller;

        if (!driver) return fail(race, WORLD_SERVICES_INVALID_BINDING);

        if (driver->kind == CAR_DRIVER_REAL) {

            if (!driver->owner.real) return fail(race, WORLD_SERVICES_INVALID_BINDING);

            if (state == 4) realDriverUnlock(&driver->owner.real->input);

            else driver->owner.real->input.forceBrake = 1;

        } else if (driver->kind == CAR_DRIVER_AI && state == 4) {

            if (!driver->owner.ai) return fail(race, WORLD_SERVICES_INVALID_BINDING);

            aiControlsFullThrottle(&driver->owner.ai->input);

        }

        if (state == 4) raceCarEntryBeginRace(entry, race, services->settings->pointToPoint);

    }

    if (state == 3) {

        race->startedWorldFrame = 0;

        race->countdownWorldFrame = race->clock->worldTimeFrames;

    } else if (state == 4) {

        race->startTicks = race->clock->worldTimer;

        gameTimerRoundTime(&race->startTicks, 0.0099999997764825821f);

        race->countdownWorldFrame = 0;

        race->startedWorldFrame = race->clock->worldTimeFrames;

    }

    if (state == 3 || state == 4) {

        World* world = services->world;

        if (world->thirdGroupEnd < 0 || world->totalCars < world->thirdGroupEnd ||

            world->totalCars > WORLD_MAX_CARS)

            return fail(race, WORLD_SERVICES_INVALID_BINDING);

        for (int32_t index = world->thirdGroupEnd; index < world->totalCars; ++index) {

            CarMovement* movement = (CarMovement*)world->carTable[index]->movement;

            if (!movement || !movement->bindings.ownerMotion)

                return fail(race, WORLD_SERVICES_INVALID_BINDING);

            movement->bindings.ownerMotion->actionEnabled = 1;

            if (carMovementSetMode(movement, 2) != CAR_MOVEMENT_OK)

                return fail(race, WORLD_SERVICES_OWNER_FAILURE);

        }

    }

    return fail(race, WORLD_SERVICES_OK);

}

static WorldServicesResult result(RaceFrame* race, WorldServicesResult value)
{
    if (race && race->services) race->services->result = value;
    return value;
}

WorldServicesResult raceFrameCommitGhost(RaceFrame* race, const RaceCarEntry* entry)
{
    RaceFrameServices* services;
    int32_t replace;
    WorldServicesResult status = WORLD_SERVICES_OK;
    if (!race || !entry || !race->services) return result(race, WORLD_SERVICES_INVALID_BINDING);
    if (entry->car != race->recordingCar) return result(race, WORLD_SERVICES_OK);
    services = race->services;
    if (!services->recordingStorage || race->recording != &services->recordingStorage->clip)
        return result(race, WORLD_SERVICES_INVALID_BINDING);
    race->recording->finishTicks = entry->finishTicks;
    replace = !race->playback || (entry->finishTicks < race->playback->finishTicks &&
        race->playback->lapCount == race->targetLapCount);
    if (entry->furthestLap != race->targetLapCount) replace = 0;
    if (replace) {
        if (!services->ghostFile) status = WORLD_SERVICES_INVALID_BINDING;
        else {
            /* The caller ignores the original write return value. A native
             * I/O failure stays on the file owner and must not prevent the
             * race finish or the unconditional recording release. */
            raceGhostFileSave(services->ghostFile, services->recordingStorage);
        }
    }
    raceGhostDisposeRecording(services->recordingStorage);
    services->recordingStorage = NULL;
    race->recordingCar = NULL;
    race->recording = NULL;
    return result(race, status);
}

static int32_t tickDifference(int32_t first, int32_t second)

{

    const uint32_t bits = (uint32_t)first - (uint32_t)second;

    int32_t value;

    memcpy(&value, &bits, sizeof(value));

    return value;

}

// PAL1487E8..148940. A finished entry contributes its actual finish timer;
// an unfinished entry contributes the same owned table's progress estimate.
WorldServicesResult raceFrameCalculateSplitTime(const RaceFrame* race,int32_t* ticks,
    const RaceCarEntry* first,const RaceCarEntry* second)
{
    if(!race || !ticks || !first || !second)return WORLD_SERVICES_INVALID_BINDING;
    const bool firstDone=first->completionReason==1,secondDone=second->completionReason==1;
    if(firstDone && secondDone) {
        *ticks=tickDifference(second->finishTicks,first->finishTicks);return WORLD_SERVICES_OK;
    }
    float firstSeconds,secondSeconds;
    auto status=raceTimeAroundTrackEvaluate(race->timeTable,firstDone ?
        static_cast<float>(first->furthestLap) : first->displayProgress,&firstSeconds);
    if(status!=WORLD_SERVICES_OK)return status;
    status=raceTimeAroundTrackEvaluate(race->timeTable,secondDone ?
        static_cast<float>(second->furthestLap) : second->displayProgress,&secondSeconds);
    if(status!=WORLD_SERVICES_OK)return status;
    int32_t estimated;
    physicsSecondsToTicks(&estimated,secondSeconds-firstSeconds);
    uint32_t result=static_cast<uint32_t>(estimated);
    if(firstDone || secondDone) {
        if(!race->clock)return WORLD_SERVICES_INVALID_BINDING;
        const int32_t finish=firstDone ? first->finishTicks : second->finishTicks;
        const int32_t elapsed=tickDifference(tickDifference(race->clock->worldTimer,finish),race->startTicks);
        if(firstDone)result-=static_cast<uint32_t>(elapsed);
        else result+=static_cast<uint32_t>(elapsed);
    }
    memcpy(ticks,&result,sizeof(result));
    return WORLD_SERVICES_OK;
}

static CarFramePlayerState* playerForCar(const RaceFrame* race, const Car* car)

{

    const CarFramePlayerRegistry* players = race->services->players;

    if (!players) return NULL;

    for (int32_t index = 0; index < players->count; ++index) {

        CarFramePlayerState* player = players->players[index];

        if (player && player->car && player->car->bindings.owner == car) return player;

    }

    return NULL;

}

WorldServicesResult raceFrameFinishParticipant(RaceFrame* race, size_t index,

    int32_t finishPosition, int32_t transition, int32_t completionReason)

{

    RaceCarEntry* entry;

    CarFramePlayerState* player;

    CarDriver* driver;

    CarMovement* movement;

    if (!validRace(race) || index >= race->entryCount || !race->services->currentEvent || !race->services->players)

        return fail(race, WORLD_SERVICES_INVALID_BINDING);

    entry = race->entries[index];

    player = playerForCar(race, entry->car);

    movement = (CarMovement*)entry->car->movement;

    driver = (CarDriver*)entry->car->controller;

    if (!driver || !movement) return fail(race, WORLD_SERVICES_INVALID_BINDING);

    if (completionReason == 4) {

        entry->finishTicks = tickDifference(race->clock->worldTimer, race->startTicks);

        gameTimerRoundTime(&entry->finishTicks, 0.0099999997764825821f);

    } else if (completionReason == 5) {

        const float seconds = (float)race->services->settings->timeLimitTicks * 0.00025000001187436283f;

        const float adjusted = seconds + 0.0099999997764825821f;

        entry->finishTicks = convertFloatToInt32(adjusted * 4000.0f + 0.5f);

        gameTimerRoundTime(&entry->finishTicks, 0.0099999997764825821f);

    }

    if (finishPosition == 0) {

        entry->finishPosition = (int8_t)raceFrameNextFinishPosition(race);

        if (entry->furthestLap == race->targetLapCount) {

            entry->finishTicks = tickDifference(entry->checkpointTicks[race->finishCheckpoint], race->startTicks);

            if (player && raceFrameCommitGhost(race, entry) != WORLD_SERVICES_OK)

                return race->services->result;

        } else raceCarEntryEstimateFinish(entry, race, race->targetLapCount, &race->startTicks);

    } else if (finishPosition >= 0) entry->finishPosition = (int8_t)finishPosition;

    entry->completionReason = (int16_t)completionReason;

    entry->displayProgress = entry->progress;

    ++race->finishedCount;

    if (!race->services->currentEvent->RecordResult(*race,*entry,entry->finishPosition))

        return fail(race, WORLD_SERVICES_INVALID_BINDING);

    if (player) {

        carFramePlayerFinish(player);

        if (player->result != CAR_DRIVER_OK || carDriverChange(driver, entry->car, CAR_DRIVER_AI) != CAR_DRIVER_OK)

            return fail(race, WORLD_SERVICES_OWNER_FAILURE);

    }

    if (driver->kind == CAR_DRIVER_AI) driverAISetStopDelay(&driver->owner.ai->base, 15.0f);

    if (transition) {

        if (!movement->bindings.ownerMotion) return fail(race, WORLD_SERVICES_INVALID_BINDING);

        carMotionBeginTransition(movement->bindings.ownerMotion, race->clock->worldTimer);

    }

    return fail(race, WORLD_SERVICES_OK);

}

WorldServicesResult raceFrameUpdate(RaceFrame* race)

{

    RaceFrameServices* services;

    if (!validRace(race)) return fail(race, WORLD_SERVICES_INVALID_BINDING);

    services = race->services;

    if (!services->players || !services->replay || !services->commentary || !race->raceKind || !race->replayRestoring)

        return fail(race, WORLD_SERVICES_INVALID_BINDING);

    if (race->state == 3 && raceFrameCountdownStep(race, services->settings->specialPursuit) == 4 &&

        raceFrameSetState(race, 4) != WORLD_SERVICES_OK)

        return services->result;

    if (race->state == 4) {

        race->elapsedTicks = tickDifference(race->clock->worldTimer, race->startTicks);

        gameTimerRoundTime(&race->elapsedTicks, 0.0099999997764825821f);

    }

    raceFrameAdvanceGhost(race);

    for (size_t index = 0; index < race->entryCount; ++index) {

        RaceCarEntry* entry = race->entries[index];

        if (entry->completionReason) continue;

        raceCarEntryUpdate(entry, race);

        if (playerForCar(race, entry->car) && entry->bestSegmentTicks[race->finishCheckpoint] < race->bestSegmentTicks)

            race->bestSegmentTicks = entry->bestSegmentTicks[race->finishCheckpoint];

    }

    if (race->pendingFinishPosition) {

        const int32_t requestedPosition = race->pendingFinishPosition;

        race->finishedCount = 0;

        for (size_t index = 0; index < race->entryCount; ++index) {

            RaceCarEntry* entry = race->entries[index];

            CarFramePlayerState* player;

            PursuitCollisionEntry* selected;

            int32_t position = requestedPosition, reason = 1;

            if (entry->completionReason) continue;

            player = playerForCar(race, entry->car);

            if (!player || player->movement.seat != race->pendingFinishPlayerIndex) continue;

            selected = pursuitManagerSelectedEntry(services->pursuit);

            if (selected && selected->movement && playerForCar(race, selected->movement->bindings.owner) == player) {

                position = -1;

                reason = requestedPosition < (int32_t)race->entryCount ? 4 : 5;

            }

            if (raceFrameFinishParticipant(race, index, position, 0, reason) != WORLD_SERVICES_OK)

                return services->result;

            break;

        }

        race->pendingFinishPosition = 0;

        race->pendingFinishPlayerIndex = 0;

    }

    raceFrameSortEntries(race);

    raceFrameHighlightPositionChanges(race, services->players, services->replay);

    if (services->settings->raceKind == 3) {

        for (size_t index = 0; index < race->entryCount; ++index) {

            const RaceCarEntry* entry = race->entries[index];

            if (!entry->advancedLap || entry->furthestLap + entry->position != (int32_t)race->entryCount) continue;

            for (size_t later = index + 1; later < race->entryCount; ++later)

                if (!race->entries[later]->completionReason &&

                    raceFrameFinishParticipant(race, later, (int32_t)later + 1, 1, 3) != WORLD_SERVICES_OK)

                    return services->result;

        }

    }

    if (services->settings->raceKind == 2) {

        for (size_t index = 0; index < race->entryCount; ++index) {

            const RaceCarEntry* entry = race->entries[index];

            const int32_t segment = entry->segmentTicks[race->finishCheckpoint];

            if (entry->advancedLap && !entry->completionReason && segment && segment != INT32_MAX &&

                segment <= services->settings->timeLimitTicks &&

                raceFrameFinishParticipant(race, index, 0, 0, 1) != WORLD_SERVICES_OK)

                return services->result;

        }

    }

    for (size_t index = 0; index < race->entryCount; ++index) {

        RaceCarEntry* entry = race->entries[index];

        if (!entry->completionReason && entry->advancedLap && entry->position == 1)

            entry->updateCounter = (int16_t)((uint16_t)entry->updateCounter + 1u);

    }

    if (race->targetLapCount) {

        for (size_t index = 0; index < race->entryCount; ++index) {

            const RaceCarEntry* entry = race->entries[index];

            if (!entry->completionReason && entry->furthestLap == race->targetLapCount &&

                raceFrameFinishParticipant(race, index, 0, 0, 1) != WORLD_SERVICES_OK)

                return services->result;

        }

    }

    for (size_t index = 0; index < race->entryCount; ++index) {

        const RaceCarEntry* entry = race->entries[index];

        if (!entry->completionReason && entry->advancedLap)

            raceCommentaryAnnounceLap(services->commentary, entry->car, entry->furthestLap);

    }

    return fail(race, WORLD_SERVICES_OK);

}

// Native shutdown adapter for the owning race frame.
// The source Race+6B0 table destructor precedes clearing its one host owner.
void raceFrameOwnerDispose(RaceFrameOwner* owner)
{
    if (!owner) return;
    RaceFrame* race = &owner->race;
    raceFrameDisposeTimeTable(race);
    if (race->services && race->services->recordingStorage &&
        race->recording == &race->services->recordingStorage->clip) {
        raceGhostDisposeRecording(race->services->recordingStorage);
        race->services->recordingStorage = NULL;
    }
    for (size_t index = 0; index < RACE_PARTICIPANT_LIMIT; ++index) {
        Car* car = owner->participants[index].car;
        if (car && car->raceEntry == &owner->participants[index]) car->raceEntry = NULL;
    }
    memset(owner, 0, sizeof(*owner));
}

// Existing accepted Race owner and timing bodies, individually placed in51.
WorldServicesResult raceFrameOwnerInitialize(RaceFrameOwner* owner, RaceFrameServices* services,
    const GameTime* clock, const TrackRoute* route, const TrackScenarioCatalog* course)
{
    RaceFrame* race;
    if (!owner || !services || !services->settings || !services->session || !clock || !route || !course || !services->replay)
        return WORLD_SERVICES_INVALID_BINDING;
    memset(owner, 0, sizeof(*owner));
    race = &owner->race;
    race->entries = owner->ordered;
    race->entryCapacity = RACE_PARTICIPANT_LIMIT;
    race->clock = clock;
    race->route = route;
    // PAL147104/14723C read Replay.restoring;1471B8 reads RaceParameters+20.
    // Borrow those live owners so snapshot restore changes accounting immediately.
    race->raceKind = &services->settings->raceKind;
    race->replayRestoring = &services->replay->restoring;
    race->services = services;
    for (size_t index = 0; index < RACE_PARTICIPANT_LIMIT; ++index)
        raceCarEntryResetTiming(&owner->participants[index]);
    race->targetLapCount = services->settings->totalLaps;
    if (race->targetLapCount && services->settings->pointToPoint) race->targetLapCount = 1;
    race->bestSegmentTicks = INT32_MAX;
    raceFrameBuildCheckpoints(race, course, services->settings->reverse, services->settings->pointToPoint);
    return services->result = WORLD_SERVICES_OK;
}

WorldServicesResult raceFrameOwnerAddCar(RaceFrameOwner* owner, Car* car)
{
    RaceFrame* race;
    CarMovement* movement;
    RaceCarEntry* entry;
    if (!owner || !car || !car->movement) return WORLD_SERVICES_INVALID_BINDING;
    race = &owner->race;
    movement = (CarMovement*)car->movement;
    if (!race->services || !race->clock || !race->route || !race->replayRestoring || !race->raceKind ||
        !movement->bindings.state || race->entries != owner->ordered)
        return WORLD_SERVICES_INVALID_BINDING;
    if (race->entryCount >= RACE_PARTICIPANT_LIMIT) return WORLD_SERVICES_CAPACITY;
    entry = &owner->participants[race->entryCount];
    raceCarEntryInitialize(entry, car, race);
    race->entries[race->entryCount++] = entry;
    return race->services->result = WORLD_SERVICES_OK;
}

WorldServicesResult raceFrameOwnerPopulate(RaceFrameOwner* owner)
{
    RaceFrame* race;
    const World* world;
    if (!owner || !owner->race.services || !owner->race.services->world)
        return WORLD_SERVICES_INVALID_BINDING;
    race = &owner->race;
    world = race->services->world;
    if (world->firstGroupEnd < 0 || world->secondGroupEnd < world->firstGroupEnd ||
        world->secondGroupEnd > world->totalCars || world->totalCars > WORLD_MAX_CARS)
        return race->services->result = WORLD_SERVICES_INVALID_BINDING;
    if (race->entryCount + (size_t)world->secondGroupEnd > RACE_PARTICIPANT_LIMIT)
        return race->services->result = WORLD_SERVICES_CAPACITY;
    for (int32_t index = 0; index < world->firstGroupEnd; ++index) {
        const WorldServicesResult status = raceFrameOwnerAddCar(owner, world->carTable[index]);
        if (status != WORLD_SERVICES_OK) return race->services->result = status;
    }
    for (int32_t index = world->firstGroupEnd; index < world->secondGroupEnd; ++index) {
        const WorldServicesResult status = raceFrameOwnerAddCar(owner, world->carTable[index]);
        if (status != WORLD_SERVICES_OK) return race->services->result = status;
    }
    raceFrameSortEntries(race);
    raceFrameSortEntries(race);
    return race->services->result = WORLD_SERVICES_OK;
}

void raceCarEntryBeginRace(RaceCarEntry* entry, const RaceFrame* race, int32_t pointToPoint)
{
    entry->checkpointTicks[0] = race->clock->worldTimer;
    gameTimerRoundTime(&entry->checkpointTicks[0], 0.0099999997764825821f);
    entry->furthestCheckpoint = 0;
    if (pointToPoint) {
        entry->currentCheckpoint = 0;
        entry->lap = 0;
    }
}

void raceCarEntryEstimateFinish(RaceCarEntry* entry, const RaceFrame* race, int32_t laps, const int32_t* startTicks)
{
    const float lapCount = (float)laps;
    float fraction = entry->progress / lapCount;
    float seconds;
    if (fraction < 0.0099999997764825821f) fraction = 0.0099999997764825821f;
    seconds = ((float)tickDifference(race->clock->worldTimer, *startTicks) * 0.00025000001187436283f) / fraction;
    physicsSecondsToTicks(&entry->finishTicks, seconds);
    gameTimerRoundTime(&entry->finishTicks, 0.0099999997764825821f);
    if (entry->furthestLap == 0 && laps > 0) {
        const int32_t ticks = tickDifference(race->clock->worldTimer, entry->checkpointTicks[race->previousCheckpoint]);
        const float segmentSeconds = (((float)ticks * 0.00025000001187436283f) / fraction) / lapCount;
        const float scaled = segmentSeconds * 4000.0f;
        entry->bestSegmentTicks[race->finishCheckpoint] = convertFloatToInt32(scaled + 0.5f);
        gameTimerRoundTime(&entry->bestSegmentTicks[race->finishCheckpoint], 0.0099999997764825821f);
    }
}

int32_t raceFrameCountdownStep(const RaceFrame* race, int32_t specialPursuit)
{
    const int32_t now = race->clock->worldTimeFrames;
    const float duration = specialPursuit ? 1.0f : 0.89999997615814209f;
    if (race->startedWorldFrame != 0)
        return tickDifference(now, race->startedWorldFrame) > 60 ? 0 : 4;
    if (race->countdownWorldFrame != 0) {
        const float elapsed = (float)tickDifference(now, race->countdownWorldFrame) * 0.01666666753590107f;
        if (elapsed < duration) return 3;
        if (elapsed < duration + duration) return 2;
        if (elapsed < duration * 3.0f) return 1;
        return 4;
    }
    return 0;
}

// PAL148188. This reads the same frame counters as GetCountdownNumber;
// neither the seconds clock nor a UI-owned timer supplies this fraction.
float raceFrameCountdownProgress(const RaceFrame* race,int32_t specialPursuit)
{
    const float duration=specialPursuit ? 1.0f : 0.89999997615814209f;
    const float total=duration*4.0f;
    const int32_t now=race->clock->worldTimeFrames;
    float progress=0.0f;
    if (race->startedWorldFrame) {
        const float elapsed=static_cast<float>(tickDifference(now,race->startedWorldFrame));
        const float elapsedSeconds=elapsed*0.01666666753590107f;
        progress=duration*3.0f+elapsedSeconds;
    } else if (race->countdownWorldFrame) {
        const float elapsed=static_cast<float>(tickDifference(now,race->countdownWorldFrame));
        progress=elapsed*0.01666666753590107f;
    }
    return progress/total;
}


int32_t carFramePlayerAllDrivingLocked(const CarFramePlayerRegistry* registry)
{
    for(int32_t index=0;index<registry->count;++index)
        if(!registry->players[index]->drivingLocked)return 0;
    return 1;
}

// RacingCar lap values retain their signed byte representation.
static int8_t lapValue(int32_t value)
{
    const uint8_t bits = (uint8_t)value;
    int8_t result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

static int16_t checkpointValue(int32_t value)
{
    const uint16_t bits = (uint16_t)value;
    int16_t result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

void raceCarEntryInitialize(RaceCarEntry* entry, Car* car, const RaceFrame* race)
{
    const CarMovement* movement = (const CarMovement*)car->movement;
    memset(entry, 0, sizeof(*entry));
    entry->car = car;
    car->raceEntry = entry;
    entry->previousPercent = movement->routeLocation.progress * 100.0f;
    entry->lap = -1;
    entry->currentCheckpoint = entry->furthestCheckpoint = -1;
    for (int32_t index = 0; index < 5; ++index) {
        entry->bestSegmentTicks[index] = INT32_MAX;
        entry->previousBestSegmentTicks[index] = INT32_MAX;
    }
    raceCarEntryUpdate(entry, race);
    entry->updateCounter = 0;
}

void raceCarEntryUpdate(RaceCarEntry* entry, const RaceFrame* race)
{
    const CarMovement* movement = (const CarMovement*)entry->car->movement;
    const VehicleState* state = movement->bindings.state;
    const float currentPercent = movement->routeLocation.progress * 100.0f;
    entry->advancedLap = entry->advancedCheckpoint = 0;
    for (int32_t index = 0; index < race->checkpointCount; ++index) {
        RaceCheckpointCrossing crossing;
        if (!raceCarEntryCheckCheckpoint(entry, race, &crossing,
                                         currentPercent, race->checkpointPositions[index])) continue;
        if (crossing.forward) {
            int16_t checkpoint;
            if (index == race->finishCheckpoint) {
                entry->lap = lapValue((int32_t)entry->lap + 1);
                if (entry->furthestLap < entry->lap) {
                    entry->furthestLap = entry->lap;
                    entry->advancedLap = 1;
                }
            }
            checkpoint = checkpointValue((int32_t)entry->lap * 5 + index);
            entry->currentCheckpoint = checkpoint;
            if (entry->furthestCheckpoint < checkpoint) {
                int32_t ticks = tickDifference(race->clock->worldTimer, crossing.correctionTicks);
                entry->furthestCheckpoint = checkpoint;
                entry->advancedCheckpoint = 1;
                gameTimerRoundTime(&ticks, 0.0099999997764825821f);
                if (*race->replayRestoring == 0) {
                    const int32_t first = entry->checkpointTicks[0];
                    if (first != 0 && first != INT32_MAX) {
                        entry->previousSegmentTicks[index] = entry->segmentTicks[index];
                        entry->segmentTicks[index] = tickDifference(ticks, first);
                        entry->previousBestSegmentTicks[index] = entry->bestSegmentTicks[index];
                        if (entry->segmentTicks[index] < entry->bestSegmentTicks[index])
                            entry->bestSegmentTicks[index] = entry->segmentTicks[index];
                        if (index == race->finishCheckpoint && (uint8_t)entry->lap < 11u)
                            entry->lapTicks[(uint8_t)entry->lap] = entry->segmentTicks[index];
                    }
                    entry->checkpointTicks[index] = ticks;
                } else if (*race->raceKind == 2 && index == race->finishCheckpoint) {
                    const int32_t first = entry->checkpointTicks[0];
                    if (first != 0 && first != INT32_MAX)
                        entry->segmentTicks[index] = tickDifference(ticks, first);
                    entry->checkpointTicks[index] = ticks;
                }
            }
        } else if (index == race->finishCheckpoint)
            entry->lap = lapValue((int32_t)entry->lap - 1);
    }
    if (*race->replayRestoring == 0) {
        if (state->activeContactCount >= 3) {
            const float previous = entry->maximumSpeed;
            float speed;
            if (movement->bindings.player)
                speed = maximumFinite(previous, state->forwardSpeed);
            else {
                const CarDriver* driver = (const CarDriver*)entry->car->controller;
                speed = carDriverRaceSpeedMetric(driver) / maximumFinite(1.0f, state->performanceScale);
                speed = minimumFinite(speed, fabsf(state->forwardSpeed));
                speed = minimumFinite(speed, movement->bindings.info->forwardSpeedLimit * 0.44702726602554321f);
                speed = maximumFinite(previous, speed);
                if (speed < 0.0f || 1000.0f < speed) speed = previous;
            }
            if (speed != previous) entry->maximumSpeed = speed;
        }
        {
            const float seconds = (float)race->elapsedTicks * 0.00025000001187436283f;
            if (0.0f < seconds) entry->averageSpeed = state->displacementDistance / seconds;
        }
    }
    {
        const float previous = entry->progress;
        const float elapsed = race->clock->worldTimeElapsed;
        const float fraction = raceFrameCheckpointFraction(race, currentPercent);
        entry->progress = (float)entry->lap + fraction;
        entry->displayProgress = entry->progress;
        if (0.0f < elapsed) {
            const float rate = (entry->progress - previous) / elapsed;
            entry->progressRate = entry->progressRate * 0.33300000429153442f
                                + rate * 0.66699999570846558f;
        }
    }
    entry->previousPercent = currentPercent;
}

CarFramePlayerState* raceCarEntryPlayer(const RaceCarEntry* entry,const RaceFrame* race)
{
    if(!entry || !entry->car || !race || !race->services || !race->services->players)return nullptr;
    const auto& registry=*race->services->players;
    for(int32_t i=0;i<registry.count;++i) {
        auto* player=registry.players[i];
        if(player && player->car && player->car->bindings.owner==entry->car)return player;
    }
    return nullptr;
}
int32_t raceCarEntryBestLapTicks(const RaceCarEntry* entry,const RaceFrame* race)
{
    hp2::Require(entry && race && race->finishCheckpoint>=0 && race->finishCheckpoint<5,
        "Best lap query has no actual race checkpoint owner");
    return entry->bestSegmentTicks[race->finishCheckpoint];
}

#include "indep/inc/Replay.hpp"
// Existing accepted position-change highlighting, one original Race owner.
void raceFrameHighlightPositionChanges(const RaceFrame* race,
    const CarFramePlayerRegistry* players, ReplayManager* replay)
{
    for (int32_t index = 0; index < players->count; ++index) {
        const CarFramePlayerState* player = players->players[index];
        const Car* car;
        const RaceCarEntry* entry;
        const Car* other;
        const CarMovement *movement, *otherMovement;
        float speed, otherSpeed, dx, dy, dz, distance;
        if (!player) continue;
        car = player->car->bindings.owner;
        entry = car->raceEntry;
        if (entry->position == entry->previousPosition) continue;
        /* Previous position refers to a valid participant in the ordered
         * table. Race binding validates that original domain. */
        other = race->entries[entry->previousPosition - 1]->car;
        movement = (const CarMovement*)car->movement;
        otherMovement = (const CarMovement*)other->movement;
        speed = movement->bindings.state->speed;
        otherSpeed = otherMovement->bindings.state->speed;
        if (!(40.0f < speed)) continue;
        dx = car->position[0] - other->position[0];
        dy = car->position[1] - other->position[1];
        dz = car->position[2] - other->position[2];
        distance = sqrtf((dx * dx + dy * dy) + dz * dz);
        if (!(distance < 25.0f)) continue;
        if (entry->position < entry->previousPosition)
            replayManagerAddHighlight(replay, 4.0f / (float)entry->position + 1.0f);
        else replayManagerAddHighlight(replay, 2.0f / (float)entry->previousPosition);
        if (fabsf(speed - otherSpeed) < 7.0f) replayManagerAddHighlight(replay, 2.5f);
    }
}

int32_t raceFinishReasonLanguageId(int32_t reason)
{
    // PAL2F0BC0, independently related to beta GetFinishReasonID146578.
    // Runtime reasons are the original enum0..5; invalid native input does
    // not index the adjacent method-pointer table.
    static const int32_t ids[6]={300104,300101,300103,300105,970002,970002};
    hp2::Require(reason>=0 && reason<6,"Race finish reason is outside its original six entries");
    return ids[reason];
}


int32_t raceFrameAddCheckpoint(RaceFrame* race, const TrackScenarioCatalog* catalog,
                               const char* name, float position[2], float fallbackPercent)
{
    const TrackLandmark* landmark;
    position[0] = position[1] = 0.0f;
    if (100.0f < fallbackPercent) fallbackPercent -= 100.0f;
    landmark = trackLandmarkFindName(catalog, name, 0);
    if (landmark) {
        TrackRouteLocation location;
        trackRouteLocationAtPoint(race->route, &location, landmark->position);
        position[0] = landmark->position[0];
        position[1] = landmark->position[1];
        race->checkpointPositions[race->checkpointCount] = location.progress * 100.0f;
        return race->checkpointCount++;
    }
    if (0.0f <= fallbackPercent) {
        race->checkpointPositions[race->checkpointCount] = fallbackPercent;
        return race->checkpointCount++;
    }
    return -1;
}

void raceFrameBuildCheckpoints(RaceFrame* race, const TrackScenarioCatalog* catalog,
                                int32_t reverse, int32_t pointToPoint)
{
    float point[2];
    float start;
    race->checkpointCount = 0;
    race->previousCheckpoint = (int8_t)raceFrameAddCheckpoint(race, catalog,
        reverse ? "BackwardStartLine" : "ForwardStartLine", race->startPosition, 5.0f);
    start = race->checkpointPositions[race->previousCheckpoint];
    if (!reverse) {
        raceFrameAddCheckpoint(race, catalog, "SplitTimeBoundary1", point, start + 33.0f);
        raceFrameAddCheckpoint(race, catalog, "SplitTimeBoundary2", point, start + 66.0f);
        raceFrameAddCheckpoint(race, catalog, "SplitTimeBoundary3", point, -1.0f);
    } else {
        raceFrameAddCheckpoint(race, catalog, "SplitTimeBoundary3", point, -1.0f);
        raceFrameAddCheckpoint(race, catalog, "SplitTimeBoundary2", point, start + 33.0f);
        raceFrameAddCheckpoint(race, catalog, "SplitTimeBoundary1", point, start + 66.0f);
    }
    if (pointToPoint) {
        race->finishCheckpoint = (int8_t)raceFrameAddCheckpoint(race, catalog,
            reverse ? "BackwardFinishLine" : "ForwardFinishLine", race->finishPosition, start + 95.0f);
    } else {
        race->finishCheckpoint = race->previousCheckpoint;
        race->finishPosition[0] = race->startPosition[0];
        race->finishPosition[1] = race->startPosition[1];
    }
}

int32_t raceCarEntryCheckCheckpoint(const RaceCarEntry* entry, const RaceFrame* race,
                                   RaceCheckpointCrossing* crossing,
                                   float currentPercent, float checkpointPercent)
{
    const float current = trackWrappedDelta100(currentPercent, checkpointPercent);
    const float previous = trackWrappedDelta100(entry->previousPercent, checkpointPercent);
    const float distance = (current * race->route->length) * 0.0099999997764825821f;
    const CarMovement* movement = (const CarMovement*)entry->car->movement;
    const float speed = movement->bindings.state->forwardSpeed;
    float seconds = 0.0f;
    crossing->forward = crossing->backward = crossing->correctionTicks = 0;
    if (25.0f < fabsf(current)) return 0;
    if (previous < 0.0f && 0.0f <= current) crossing->forward = 1;
    else if (0.0f <= previous && current < 0.0f) crossing->backward = 1;
    else return 0;
    if (1.0f < speed) seconds = distance / speed;
    crossing->correctionTicks = convertFloatToInt32(seconds * 4000.0f + 0.5f);
    return 1;
}

float raceFrameCheckpointFraction(const RaceFrame* race, float currentPercent)
{
    float delta = trackWrappedDelta100(currentPercent, race->checkpointPositions[race->finishCheckpoint]);
    if (delta < 0.0f) delta += 100.0f;
    return delta * 0.0099999997764825821f;
}

void raceFrameAdvanceGhost(RaceFrame* race)
{
    if (race->state == 4 && race->recordingCar) {
        const CarMovement* movement = (const CarMovement*)race->recordingCar->movement;
        float frame[16];
        memcpy(frame, movement->bindings.state->orientation, sizeof(frame));
        memcpy(frame + 12, race->recordingCar->position, 4 * sizeof(float));
        raceGhostCapture(race->recording, frame, &race->elapsedTicks);
    }
    if (race->playback && race->elapsedTicks != 0 && race->elapsedTicks != INT32_MAX) {
        float frame[16];
        if (raceGhostSample(race->playback, frame, &race->elapsedTicks)) {
            frame[14] += -0.5f;
            memcpy(race->ghostModel->frame, frame, sizeof(frame));
            race->ghostModel->visible = 1;
        } else race->ghostModel->visible = 0;
    }
}

const RaceConnectionTiming raceConnectionDefaultTiming = {12.0f, 0.5f};

int32_t raceFrameIsDriving(const RaceFrame* race, int32_t specialPursuit)
{
    return race->state == 4 || (specialPursuit != 0 && race->state >= 2);
}

int32_t raceFrameConnectionWindowActive(const RaceFrame* race, const Car* car,
                                        const RaceConnectionTiming* timing)
{
    const uint32_t elapsedBits = (uint32_t)race->clock->worldTimer - (uint32_t)race->startTicks;
    int32_t elapsedTicks;
    float duration = timing->baseSeconds;
    memcpy(&elapsedTicks, &elapsedBits, sizeof(elapsedTicks));
    if (101 <= car->driverNumber && car->driverNumber <= 149)
        duration += (float)(100 - car->driverNumber) * timing->driverStepSeconds;
    return (float)elapsedTicks * 0.00025000001187436283f <= duration;
}

// Native filesystem transport for Race::MaybeSaveGhostCarData; no extra original-body claim.

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {
constexpr std::size_t headerSize = 160;
constexpr std::size_t sampleSize = 80;

void put32(unsigned char* output, std::uint32_t value)
{
    for (unsigned lane = 0; lane < 4; ++lane)
        output[lane] = static_cast<unsigned char>(value >> (lane * 8));
}

std::uint32_t get32(const unsigned char* input)
{
    return std::uint32_t(input[0]) | (std::uint32_t(input[1]) << 8) |
        (std::uint32_t(input[2]) << 16) | (std::uint32_t(input[3]) << 24);
}

void putSigned(unsigned char* output, std::int32_t value)
{
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    put32(output, bits);
}

std::int32_t getSigned(const unsigned char* input)
{
    const std::uint32_t bits = get32(input);
    std::int32_t value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

bool valid(const RaceGhostClip& clip)
{
    if (!clip.samples || clip.count < 0 || clip.count > RACE_GHOST_SAMPLE_LIMIT ||
        static_cast<std::size_t>(clip.count) > clip.capacity || clip.cursor < 0 ||
        (clip.count ? clip.cursor >= clip.count : clip.cursor != 0)) return false;
    for (std::int32_t index = 0; index < clip.count; ++index) {
        const auto& sample = clip.samples[index];
        if (index && sample.ticks < clip.samples[index - 1].ticks) return false;
        for (float value : sample.frame)
            if (!std::isfinite(value)) return false;
    }
    return true;
}
}

extern "C" RaceGhostFileResult raceGhostFileSave(RaceGhostFile* file, const RaceGhostStorage* recording)
{
    if (!file) return RACE_GHOST_FILE_INVALID;
    if (!file->filename || !*file->filename || !recording || !valid(recording->clip))
        return file->result = RACE_GHOST_FILE_INVALID;
    try {
        const RaceGhostClip& clip = recording->clip;
        /* This buffer is the file format, not an image of the native owner.
         * Source header reserved bytes and sample alignment are written as
         * zero instead of disclosing uninitialized console memory. */
        std::vector<unsigned char> bytes(headerSize + static_cast<std::size_t>(clip.count) * sampleSize, 0);
        put32(bytes.data(), 3);
        putSigned(bytes.data() + 4, clip.finishTicks);
        putSigned(bytes.data() + 8, clip.lapCount);
        put32(bytes.data() + 12, std::uint32_t(recording->identifiers[0]) |
            (std::uint32_t(recording->identifiers[1]) << 16));
        putSigned(bytes.data() + 16, clip.count);
        putSigned(bytes.data() + 20, clip.cursor);
        putSigned(bytes.data() + 24, clip.lastCaptureTicks);
        std::memcpy(bytes.data() + 32, recording->label, sizeof(recording->label));
        for (std::int32_t index = 0; index < clip.count; ++index) {
            unsigned char* sample = bytes.data() + headerSize + static_cast<std::size_t>(index) * sampleSize;
            for (unsigned lane = 0; lane < 16; ++lane) {
                std::uint32_t bits;
                std::memcpy(&bits, &clip.samples[index].frame[lane], sizeof(bits));
                put32(sample + lane * 4, bits);
            }
            putSigned(sample + 64, clip.samples[index].ticks);
        }
        std::ofstream output(std::filesystem::u8path(file->filename), std::ios::binary | std::ios::trunc);
        if (!output) return file->result = RACE_GHOST_FILE_IO_ERROR;
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        output.flush();
        return file->result = output ? RACE_GHOST_FILE_OK : RACE_GHOST_FILE_IO_ERROR;
    } catch (...) {
        return file->result = RACE_GHOST_FILE_IO_ERROR;
    }
}

extern "C" RaceGhostFileResult raceGhostFileLoad(RaceGhostFile* file, RaceGhostStorage* playback)
{
    if (!file) return RACE_GHOST_FILE_INVALID;
    if (!file->filename || !*file->filename || !playback)
        return file->result = RACE_GHOST_FILE_INVALID;
    try {
        std::ifstream input(std::filesystem::u8path(file->filename), std::ios::binary | std::ios::ate);
        if (!input) return file->result = RACE_GHOST_FILE_IO_ERROR;
        const auto length = input.tellg();
        if (length < static_cast<std::streamoff>(headerSize) ||
            length > static_cast<std::streamoff>(headerSize + RACE_GHOST_SAMPLE_LIMIT * sampleSize))
            return file->result = RACE_GHOST_FILE_INVALID;
        std::vector<unsigned char> bytes(static_cast<std::size_t>(length));
        input.seekg(0);
        input.read(reinterpret_cast<char*>(bytes.data()), length);
        if (!input) return file->result = RACE_GHOST_FILE_IO_ERROR;
        const std::int32_t count = getSigned(bytes.data() + 16);
        const std::int32_t cursor = getSigned(bytes.data() + 20);
        if (get32(bytes.data()) != 3 || count < 0 || count > RACE_GHOST_SAMPLE_LIMIT || cursor < 0 ||
            (count ? cursor >= count : cursor != 0) ||
            bytes.size() != headerSize + static_cast<std::size_t>(count) * sampleSize)
            return file->result = RACE_GHOST_FILE_INVALID;
        // Validate the detached file bytes before publishing any part of the
        // live owner. The sampler requires ordered timestamps and finite
        // poses; equal timestamps retain their original zero-duration rule.
        for (std::int32_t index = 0; index < count; ++index) {
            const unsigned char* sample = bytes.data() + headerSize +
                static_cast<std::size_t>(index) * sampleSize;
            if (index && getSigned(sample + 64) < getSigned(sample - sampleSize + 64))
                return file->result = RACE_GHOST_FILE_INVALID;
            for (unsigned lane = 0; lane < 16; ++lane) {
                const std::uint32_t bits = get32(sample + lane * 4);
                float value;
                std::memcpy(&value, &bits, sizeof(value));
                if (!std::isfinite(value)) return file->result = RACE_GHOST_FILE_INVALID;
            }
        }
        raceGhostDataInitialize(playback, 0, 0, getSigned(bytes.data() + 8));
        playback->clip.finishTicks = getSigned(bytes.data() + 4);
        playback->clip.count = count;
        playback->clip.cursor = cursor;
        playback->clip.lastCaptureTicks = getSigned(bytes.data() + 24);
        const std::uint32_t identifiers = get32(bytes.data() + 12);
        playback->identifiers[0] = static_cast<std::uint16_t>(identifiers);
        playback->identifiers[1] = static_cast<std::uint16_t>(identifiers >> 16);
        std::memcpy(playback->label, bytes.data() + 32, sizeof(playback->label));
        for (std::int32_t index = 0; index < count; ++index) {
            const unsigned char* sample = bytes.data() + headerSize + static_cast<std::size_t>(index) * sampleSize;
            for (unsigned lane = 0; lane < 16; ++lane) {
                const std::uint32_t bits = get32(sample + lane * 4);
                std::memcpy(&playback->samples[index].frame[lane], &bits, sizeof(bits));
            }
            playback->samples[index].ticks = getSigned(sample + 64);
        }
        return file->result = RACE_GHOST_FILE_OK;
    } catch (...) {
        return file->result = RACE_GHOST_FILE_IO_ERROR;
    }
}
