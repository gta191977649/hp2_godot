#include "indep/inc/World.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/RaceParameters.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <new>
#include <memory>

struct TrafficScheduler {
    TrafficSchedulerBindings bindings;
    const hp2::CarInfoResource* carInfo;
    float remainingSeconds;
    uint32_t cursor;
    std::array<uint32_t,10> forwardTotals;
    std::array<uint32_t,10> reverseTotals;
    WorldServicesResult result;
};

namespace {
CarMovement* Movement(Car* car)
{
    return static_cast<CarMovement*>(car->movement);
}

void trafficSchedulerFillTotals(const CarRecoveryPointRegistry* road,
    std::array<uint32_t,10>& totals, int32_t firstGroup)
{
    uint32_t count=0;
    for (size_t i=0; i<totals.size(); ++i) {
        const int32_t id=firstGroup+static_cast<int32_t>(i);
        const CarRecoveryGroup* group=nullptr;
        for (size_t index=0; index<road->groupCount; ++index)
            if (road->groups[index].id==id) {group=&road->groups[index];break;}
        if (group) count+=static_cast<uint32_t>(group->poseCount);
        totals[i]=count;
    }
}

void trafficSchedulerSetThinkTime(TrafficScheduler* scheduler)
{
    const float density=scheduler->bindings.parameters->trafficDensity;
    const float scaled=(1.0f-density)*15.0f;
    scheduler->remainingSeconds=scaled+1.0f;
}

int32_t trafficSchedulerForwardsOrBackwards(TrafficScheduler* scheduler)
{
    return worldRandomFloat(scheduler->bindings.world,1.0f)<
        scheduler->bindings.parameters->opposingTrafficProbability;
}

int32_t trafficSchedulerCheckVisibility(TrafficScheduler* scheduler,float distance)
{
    const auto* route=scheduler->bindings.world;
    const float delta=worldGetClosestSignedTrackDelta(route,distance,
        scheduler->bindings.race->route->length);
    return std::fabs(delta)<250.0f;
}

[[maybe_unused]] int32_t trafficSchedulerTooClosePoint(TrafficScheduler* scheduler,const float point[2],
    const Car* excluded,int32_t includeInactive)
{
    World* world=scheduler->bindings.world;
    for (Car* car=world->carList.next; car!=&world->carList; car=car->next) {
        if (car==excluded) continue;
        const CarMotionState* motion=Movement(car)->bindings.ownerMotion;
        if (carMotionTransitionFlag(motion)) continue;
        if (!includeInactive && !motion->actionEnabled) continue;
        const float x=point[0]-car->position[0];
        const float y=point[1]-car->position[1];
        if (std::sqrt(x*x+y*y)<100.0f) return 1;
    }
    return 0;
}

int32_t trafficSchedulerTooCloseDistance(TrafficScheduler* scheduler,float distance,
    const Car* excluded,int32_t includeInactive)
{
    World* world=scheduler->bindings.world;
    const TrackRoute* route=scheduler->bindings.race->route;
    for (Car* car=world->carList.next; car!=&world->carList; car=car->next) {
        if (car==excluded) continue;
        const auto* movement=Movement(car);
        if (carMotionTransitionFlag(movement->bindings.ownerMotion)) continue;
        if (!includeInactive && !movement->bindings.ownerMotion->actionEnabled) continue;
        const float delta=trackRouteDistanceDifference(route,distance,movement->routeLocation.distance);
        if (car->driverInfo->driverClass==3) {
            if (std::fabs(delta)<100.0f) return 1;
        } else if (delta<100.0f && 0.0f<delta) return 1;
    }
    return 0;
}

int32_t trafficSchedulerCanPlaceCar(TrafficScheduler* scheduler,const CarRecoverySelection* selection,
    Car* excluded,int32_t includeInactive)
{
    const RaceFrame* race=scheduler->bindings.race;
    const uint32_t bits=static_cast<uint32_t>(scheduler->bindings.clock->worldTimer)-
        static_cast<uint32_t>(race->startTicks);
    int32_t elapsedTicks;std::memcpy(&elapsedTicks,&bits,sizeof(elapsedTicks));
    const float elapsed=static_cast<float>(elapsedTicks)*0.0002500000118743628f;
    if (elapsed<20.0f) {
        if (race->previousCheckpoint<0 || race->previousCheckpoint>=5) {
            scheduler->result=WORLD_SERVICES_INVALID_BINDING;
            return 0;
        }
        const float scaled=race->route->length*0.009999999776482582f;
        const float checkpoint=scaled*race->checkpointPositions[race->previousCheckpoint];
        if (std::fabs(trackRouteDistanceDifference(race->route,
            selection->pose->routeDistance,checkpoint))<2000.0f) return 0;
    }
    if (trafficSchedulerCheckVisibility(scheduler,selection->pose->routeDistance)) return 0;
    return !trafficSchedulerTooCloseDistance(scheduler,selection->pose->routeDistance,excluded,includeInactive);
}

Car* trafficSchedulerRandomPlayerCar(TrafficScheduler* scheduler)
{
    auto* players=scheduler->bindings.players;
    const uint32_t index=worldRandomBounded(scheduler->bindings.world,static_cast<uint32_t>(players->count));
    return players->players[index]->car->bindings.owner;
}

int32_t trafficSchedulerPickRandomSkin(TrafficScheduler* scheduler,const hp2::CarDefinition& car)
{
    const int32_t count=scheduler->carInfo->AvailableSkinCount(static_cast<size_t>(car.modelCode));
    if (!count) return 0;
    const int32_t choice=static_cast<int32_t>(worldRandomBounded(scheduler->bindings.world,static_cast<uint32_t>(count)));
    return scheduler->carInfo->AvailableSkinNumber(static_cast<size_t>(car.modelCode),choice,
        *scheduler->bindings.alternatePursuitSkin!=0);
}

WorldServicesResult trafficSchedulerSwitchSkin(TrafficScheduler* scheduler,Car* car)
{
    const auto& definition=scheduler->carInfo->Car(static_cast<size_t>(Movement(car)->info.modelCode));
    const int32_t skin=trafficSchedulerPickRandomSkin(scheduler,definition);
    if (skin!=car->driverInfo->appearance.skinNumber && carSwitchSkin(car,skin)!=CAR_APPEARANCE_OK)
        return scheduler->result=WORLD_SERVICES_OWNER_FAILURE;
    return WORLD_SERVICES_OK;
}

WorldServicesResult trafficSchedulerTryTeleportCar(TrafficScheduler* scheduler,Car* car)
{
    const int32_t forward=trafficSchedulerForwardsOrBackwards(scheduler);
    Car* reference=trafficSchedulerRandomPlayerCar(scheduler);
    auto* movement=Movement(car);
    const auto* referenceMovement=Movement(reference);
    const TrackRoute* route=scheduler->bindings.race->route;
    const float initialDelta=trackRouteDistanceDifference(route,movement->routeLocation.distance,
        referenceMovement->routeLocation.distance);
    CarRecoverySelection selection{};
    if (!carRecoverySelectPose(&selection,scheduler->bindings.road,
        referenceMovement->bindings.state->position,forward?2:3,0,0.0f)) return WORLD_SERVICES_OK;
    CarRecoverySelection next{};
    if (carRecoveryFindFollowingBranches(scheduler->bindings.road,&next,1,
        selection.group,selection.pose,250.0f,forward)!=1) return WORLD_SERVICES_OK;
    selection=next;
    for (;;) {
        const float delta=trackRouteDistanceDifference(route,selection.pose->routeDistance,
            referenceMovement->routeLocation.distance);
        if (trafficSchedulerCanPlaceCar(scheduler,&selection,car,0)) {
            if (0.0f<initialDelta && initialDelta<delta) return WORLD_SERVICES_OK;
            if (delta<0.0f) return WORLD_SERVICES_OK;
            carMovementRecover(movement,selection.group,selection.pose,24.58650016784668f,1.0f,0,0);
            if (movement->result!=CAR_MOVEMENT_OK || carMovementSetMode(movement,2)!=CAR_MOVEMENT_OK)
                return scheduler->result=WORLD_SERVICES_OWNER_FAILURE;
            const auto result=trafficSchedulerSwitchSkin(scheduler,car);
            if(result!=WORLD_SERVICES_OK)return result;
            auto* frame=car->Frame();
            if(!frame || frame->movement!=movement)
                return scheduler->result=WORLD_SERVICES_INVALID_BINDING;
            // Native PC lifecycle correction: this offscreen placement is a
            // new traffic appearance, although the original reuses its Car.
            // ResetCore has already rebuilt its wheel pose/mover. Retire the
            // old puncture only after placement and skin selection succeed;
            // ordinary recovery and rejected placements retain their damage.
            carFrameClearDisabledWheels(frame);
            return WORLD_SERVICES_OK;
        }
        if (scheduler->result!=WORLD_SERVICES_OK) return scheduler->result;
        if (carRecoveryFindFollowingBranches(scheduler->bindings.road,&next,1,
            selection.group,selection.pose,50.0f,forward)!=1) return WORLD_SERVICES_OK;
        selection=next;
    }
}

int32_t ValidBinding(const TrafficSchedulerBindings* bindings,const hp2::CarInfoResource* carInfo)
{
    return bindings && bindings->world && bindings->clock && bindings->parameters &&
        bindings->race && bindings->race->route && bindings->players && bindings->road &&
        bindings->alternatePursuitSkin && carInfo && carInfo->CarsLoaded() && carInfo->SkinsLoaded();
}

TrafficScheduler* trafficSchedulerInitialize(TrafficScheduler* scheduler)
{
    trafficSchedulerFillTotals(scheduler->bindings.road,scheduler->forwardTotals,50);
    trafficSchedulerFillTotals(scheduler->bindings.road,scheduler->reverseTotals,60);
    trafficSchedulerSetThinkTime(scheduler);
    if (scheduler->remainingSeconds<20.0f) scheduler->remainingSeconds=20.0f;
    scheduler->cursor=0;
    return scheduler;
}

// PickRandomPoint PAL2230D0..2231EC. The cumulative counts select one
// authored route/pose; bit15 retries at most101 draws, then keeps the last
// selection exactly as the original does. The World RNG is shared with cops.
CarRecoverySelection trafficSchedulerPickRandomPoint(TrafficScheduler& scheduler,int32_t forward)
{
    const auto& totals=forward?scheduler.forwardTotals:scheduler.reverseTotals;
    const int32_t firstGroup=forward?50:60;
    hp2::Require(totals.back()!=0,"Traffic direction has no authored road points");
    CarRecoverySelection selected{};
    for(unsigned attempt=0;attempt<101;++attempt) {
        uint32_t point=worldRandomBounded(scheduler.bindings.world,totals.back());
        size_t index=0;while(index<totals.size() && point>=totals[index])++index;
        hp2::Require(index<totals.size(),"Traffic cumulative route counts are invalid");
        if(index)point-=totals[index-1];
        for(size_t i=0;i<scheduler.bindings.road->groupCount;++i) {
            const auto& group=scheduler.bindings.road->groups[i];
            if(group.id==firstGroup+static_cast<int32_t>(index)){selected.group=&group;break;}
        }
        hp2::Require(selected.group && point<static_cast<uint32_t>(selected.group->poseCount),
            "Traffic point does not belong to its authored route");
        selected.pose=&selected.group->poses[point];
        if(selected.pose->flags&0x8000u)break;
    }
    return selected;
}
}

TrafficScheduler* trafficSchedulerCreate(const TrafficSchedulerBindings* bindings,
    const hp2::CarInfoResource* carInfo)
{
    if (!ValidBinding(bindings,carInfo)) return nullptr;
    auto* scheduler=new(std::nothrow) TrafficScheduler{};
    if (!scheduler) return nullptr;
    scheduler->bindings=*bindings;scheduler->carInfo=carInfo;
    return trafficSchedulerInitialize(scheduler);
}

void trafficSchedulerDispose(TrafficScheduler* scheduler)
{
    delete scheduler;
}

// Complete PAL222780..2227DC. Authored traffic totals and owner bindings
// are not copied into the stream; only the two mutable source cells move.
void trafficSchedulerDoSnapshot(TrafficScheduler* scheduler,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"TrafficTeleporter");
    replaySnapshotMoveFloat(snapshot,&scheduler->remainingSeconds);
    replaySnapshotMoveUnsigned(snapshot,&scheduler->cursor,4);
    replaySnapshotEndSection(snapshot);
}

namespace hp2 {
// Complete PlaceNewCar PAL2228E0..222A8C. Allocation owns the real Car
// constructor graph; its caller World::AddTrafficCar adds that same object.
Car* TrafficPlaceNewCar(TrafficScheduler* scheduler,RaceDriverInfo& driver)
{
    Require(scheduler && ValidBinding(&scheduler->bindings,scheduler->carInfo),
        "Traffic placement requires the loaded World/road/race owners");
    TrackRouteLocation coordinate;trackRouteLocationInitialize(&coordinate);
    const int32_t forward=trafficSchedulerForwardsOrBackwards(scheduler);
    if(!scheduler->forwardTotals.back() || !scheduler->reverseTotals.back())return nullptr;
    scheduler->result=WORLD_SERVICES_OK;
    for(unsigned attempt=0;attempt<100;++attempt) {
        const auto selected=trafficSchedulerPickRandomPoint(*scheduler,forward);
        if(!trafficSchedulerCanPlaceCar(scheduler,&selected,nullptr,1)) {
            Require(scheduler->result==WORLD_SERVICES_OK,"Traffic placement owner failed");
            continue;
        }
        trackRouteUpdateLocation(scheduler->bindings.race->route,&coordinate,selected.pose->position);
        const uint16_t heading=vectorAtan2Angle(selected.pose->direction[0],selected.pose->direction[1]);
        const float position[3]={selected.pose->position[0],selected.pose->position[1],selected.pose->position[2]};
        const auto& definition=scheduler->carInfo->Car(static_cast<size_t>(driver.appearance.modelCode));
        driver.appearance.skinNumber=static_cast<int8_t>(trafficSchedulerPickRandomSkin(scheduler,definition));
        std::unique_ptr<Car> car(WorldCreateCar(*scheduler->bindings.world,driver,position,heading,
            selected.pose->routeDistance));
        Require(carMovementSetMode(car->Movement(),0)==CAR_MOVEMENT_OK,"Traffic NullMover initialization failed");
        Require(carDriverChange(car->Driver(),car.get(),CAR_DRIVER_TRAFFIC)==CAR_DRIVER_OK,
            "Traffic controller construction failed");
        driverAISetRoad(&car->Driver()->owner.traffic->base,selected.group,selected.pose);
        car->Movement()->bindings.ownerMotion->actionEnabled=0;
        return car.release();
    }
    return nullptr;
}
}

WorldServicesResult trafficSchedulerUpdate(TrafficScheduler* scheduler,float elapsed)
{
    if (!scheduler || !ValidBinding(&scheduler->bindings,scheduler->carInfo)) return WORLD_SERVICES_INVALID_BINDING;
    auto& bindings=scheduler->bindings;World* world=bindings.world;
    scheduler->result=WORLD_SERVICES_OK;
    if (bindings.parameters->trafficDensity<0.4000000059604645f ||
        world->totalCars==world->thirdGroupEnd || bindings.race->state!=4 || !bindings.players->count)
        return WORLD_SERVICES_OK;
    if (world->thirdGroupEnd<0 || world->thirdGroupEnd>world->totalCars || world->totalCars>WORLD_MAX_CARS ||
        bindings.players->count<0 || bindings.players->count>2)
        return scheduler->result=WORLD_SERVICES_INVALID_BINDING;
    scheduler->remainingSeconds-=elapsed;
    if (0.0f<scheduler->remainingSeconds) return WORLD_SERVICES_OK;
    trafficSchedulerSetThinkTime(scheduler);
    const uint32_t trafficCount=static_cast<uint32_t>(world->totalCars-world->thirdGroupEnd);
    if (scheduler->cursor>=trafficCount) return scheduler->result=WORLD_SERVICES_INVALID_BINDING;
    uint32_t index=scheduler->cursor;
    for (;;) {
        ++index;if (index>=trafficCount) index=0;
        Car* car=world->carTable[world->thirdGroupEnd+index];
        if (!car || !car->movement || !car->driverInfo)
            return scheduler->result=WORLD_SERVICES_INVALID_BINDING;
        if (!trafficSchedulerCheckVisibility(scheduler,Movement(car)->routeLocation.distance)) {
            const auto result=trafficSchedulerTryTeleportCar(scheduler,car);
            scheduler->cursor=index;
            return result;
        }
        if (index==scheduler->cursor) {scheduler->cursor=index;return WORLD_SERVICES_OK;}
    }
}
