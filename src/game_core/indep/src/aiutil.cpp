#include "indep/inc/AICarController.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/CarCollisionDetect.hpp"
#include "indep/inc/DriveTarget.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/spikestrip.hpp"
#include <math.h>
#include <string.h>

/* Native storage for the original route-search Deque operations. */
static inline int carRecoveryPathHistoryEmpty(const CarRecoveryGroupHistory* history)
{
    return !history->full && history->first == history->last;
}

static inline void carRecoveryPathHistoryPush(CarRecoveryGroupHistory* history, const CarRecoveryGroup* group)
{
    if (history->full) return;
    history->groups[history->last] = group;
    history->last = (history->last + 1) % CAR_RECOVERY_PATH_GROUPS;
    history->full = history->first == history->last;
}

static inline int carRecoveryPathHistoryContains(const CarRecoveryGroupHistory* history, const CarRecoveryGroup* group)
{
    int32_t index;
    if (carRecoveryPathHistoryEmpty(history)) return 0;
    index = history->first;
    do {
        if (history->groups[index] == group) return 1;
        index = (index + 1) % CAR_RECOVERY_PATH_GROUPS;
    } while (index != history->last);
    return 0;
}

typedef struct PendingPath {
    CarRecoverySelection selections[CAR_RECOVERY_PATH_GROUPS];
    int32_t first;
    int32_t last;
    int32_t full;
} PendingPath;

#ifdef __cplusplus
extern "C" {
#endif
int32_t restorePendingPath(CarRecoverySelection* current, PendingPath* pending,
    CarRecoveryGroupHistory* history);
#ifdef __cplusplus
}
#endif

// PAL1CD460..1CD4D4, using the real route owner for both authored references.
void carRecoverySelectionSnapshot(const CarRecoveryPointRegistry* registry,
    CarRecoverySelection* selection,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"NearestRoutePointInfo");
    carRecoverySnapshotGroup(registry,snapshot,&selection->group);
    carRecoverySnapshotPoseInGroup(selection->group,snapshot,&selection->pose);
    replaySnapshotEndSection(snapshot);
}

int32_t restorePendingPath(CarRecoverySelection* current, PendingPath* pending,
                                  CarRecoveryGroupHistory* history)
{
    const CarRecoveryGroup* destination;
    if (!pending->full && pending->first == pending->last) return 0;
    *current = pending->selections[pending->first];
    pending->first = (pending->first + 1) % CAR_RECOVERY_PATH_GROUPS;
    pending->full = 0;
    destination = current->group;
    while (!carRecoveryPathHistoryEmpty(history)) {
        const int32_t previous = history->last ? history->last - 1 : CAR_RECOVERY_PATH_GROUPS - 1;
        if (history->groups[previous] == destination) return 1;
        history->last = previous;
        history->full = 0;
    }
    return 1;
}

int32_t carRecoveryAdvanceToConnection(CarRecoverySelection* selection, int32_t forward)
{
    int32_t count = 1;
    while (selection->pose) {
        if ((selection->pose->flags & 255u) != 255u) return count;
        ++count;
        selection->pose = carRecoveryAdjacentPose(selection->group, selection->pose, forward);
    }
    return 0;
}

int32_t driverCollisionNeighborEligible(const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception)
{
    if(!neighbor||neighbor->movement==perception->movement)return 0;
    const float adjustment=neighbor->movement->bindings.player?-0.5f:0.0f;
    const float extent=neighbor->effectiveHalfExtents[1]+perception->forwardExtent;
    const float boundary=extent+adjustment;
    return -boundary<neighbor->localPosition[1];
}

int32_t driverCommandPointAhead(const float point[2],const VehicleState* vehicle,float threshold)
{
    float velocity[2],direction[2];
    const float delta[2]={point[0]-vehicle->position[0],point[1]-vehicle->position[1]};
    vectorNormalize2(velocity,vehicle->velocity);
    vectorNormalize2(direction,delta);
    const float first=velocity[0]*direction[0];
    const float second=velocity[1]*direction[1];
    return threshold<first+second;
}

float* driverPointAlongVelocity(float output[4],const VehicleState* state,float distance)
{
    float delta[4] = {0,0,0,0};
    vectorSetLength3(delta,state->velocity,maximumFinite(5.0f,distance));
    for (int axis = 0; axis < 4; ++axis) output[axis] = delta[axis]+state->position[axis];
    return output;
}

int32_t driverRoadSelectionValid(const CarRecoverySelection* selection)
{
    const CarRecoveryGroup* group = selection->group;
    if (!group || !selection->pose || group->id < 0 || 70 <= group->id) return 0;
    for (int32_t index = 0; index < group->poseCount; ++index)
        if (&group->poses[index] == selection->pose) return 1;
    return 0;
}

int32_t pursuitPointInPolygon(const float point[2], const float vertices[][2], int32_t count)
{
    const float y=point[1];
    const float x=point[0];
    int32_t inside=0;
    if (count<=0) return inside;
    int32_t previous=count-1;
    for (int32_t current=0;current<count;++current) {
        const float currentY=vertices[current][1];
        const float previousY=vertices[previous][1];
        if ((currentY<=y && y<previousY) || (previousY<=y && y<currentY)) {
            const float currentX=vertices[current][0];
            const float differenceY=y-currentY;
            const float differenceX=vertices[previous][0]-currentX;
            const float edgeY=previousY-currentY;
            const float scaled=differenceX*differenceY;
            const float fraction=scaled/edgeY;
            const float crossing=fraction+currentX;
            if (x<crossing) inside^=1;
        }
        previous=current;
    }
    return inside;
}

int32_t vehicleStateGroundedWheelCount(const VehicleState* state)
{
    int32_t contacts = 0;
    for (int32_t wheel = 0; wheel < 4; ++wheel) {
        if (state->wheelContact[wheel] != 0) ++contacts;
    }
    return contacts;
}
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Race.hpp"
#include "bware/inc/bWare.hpp"
#include <utility>

// Complete PAL1CD240. The canonical vehicle state already owns Car+3DC;
// this operation does not change CarMovement::mode or create another car.
void SetAIDriverModesFromPlayers(World* world,const RaceFrame* race,
    const RaceParameters& parameters,const CarFramePlayerRegistry& players)
{
    if (!world || !race)return;
    hp2::Require(world->firstGroupEnd>=0 && world->firstGroupEnd<=world->secondGroupEnd &&
        world->secondGroupEnd<=world->totalCars && world->totalCars<=WORLD_MAX_CARS,
        "AI style update has no valid actual opponent range");
    auto assign=[](Car* car,int32_t style) {
        hp2::Require(car && car->movement,"AI style update has no actual Car movement");
        auto* movement=static_cast<CarMovement*>(car->movement);
        hp2::Require(movement->bindings.owner==car && movement->bindings.state,
            "AI style update has no canonical vehicle state");
        movement->bindings.state->pursuitMode=style;
    };
    if (parameters.mode==2) {
        const auto* first=players.playersByNumber[0];const auto* second=players.playersByNumber[1];
        hp2::Require(first && second && first->car && second->car,
            "Two-player AI style update has no live Players");
        const auto* firstEntry=raceFrameEntryForCar(first->car->bindings.owner);
        const auto* secondEntry=raceFrameEntryForCar(second->car->bindings.owner);
        hp2::Require(firstEntry && secondEntry,"Two-player AI style update has no actual RacingCar records");
        const auto firstPosition=firstEntry->position,secondPosition=secondEntry->position;
        int32_t firstStyle=first->settings.initialVehicleMode,secondStyle=second->settings.initialVehicleMode;
        if (secondPosition<firstPosition)std::swap(firstStyle,secondStyle);
        int32_t threshold=parameters.opponentCount/2;
        if (!(threshold<firstPosition))++threshold;
        if (!(threshold<secondPosition))++threshold;
        for (auto index=world->firstGroupEnd;index<world->secondGroupEnd;++index) {
            auto* car=world->carTable[index];const auto* entry=raceFrameEntryForCar(car);
            hp2::Require(entry,"AI style update has no actual opponent RacingCar record");
            assign(car,threshold<entry->position?secondStyle:firstStyle);
        }
    } else {
        hp2::Require(players.players[0],"AI style update has no first live Player");
        const auto style=players.players[0]->settings.initialVehicleMode;
        for (auto index=world->firstGroupEnd;index<world->secondGroupEnd;++index)
            assign(world->carTable[index],style);
    }
}

// Full original StraightLinePredict; accepted body moved without token changes.
void driverCommandBuildStraight(DriverCommand* command,const DriverPhysicsVehicle* context)
{
    const CarMovement* movement=context->movement;
    const VehicleState* vehicle=movement->bindings.state;
    const float distance=driverPlanningDistance(context);
    float direction[4];
    if(vehicle->speed<1.0f){
        float sine,cosine;
        vectorSineCosineAngle(&sine,&cosine,vehicle->heading);
        direction[0]=cosine;direction[1]=sine;direction[2]=0.0f;direction[3]=0.0f;
    }else{
        const float xx=vehicle->velocity[0]*vehicle->velocity[0];
        const float yy=vehicle->velocity[1]*vehicle->velocity[1];
        const float zz=vehicle->velocity[2]*vehicle->velocity[2];
        const float inverseLength=finiteRatio(1.0f,sqrtf((xx+yy)+zz));
        for(int axis=0;axis<4;++axis)direction[axis]=vehicle->velocity[axis]*inverseLength;
    }
    float point[4];
    for(int axis=0;axis<4;++axis){
        const float offset=direction[axis]*distance;
        point[axis]=offset+vehicle->position[axis];
    }
    driverCommandAdvance(command,NULL,movement->bindings.environment->clock->worldTimer,0.0f);
    driverCommandAppend(command,point,NULL,-1,0,NULL);
}

float driverAvailableAcceleration(const DriverPhysicsVehicle* vehicle, float speed)
{
    CarMovement* movement = vehicle->movement;
    const PhysicsCarInfo* info = movement->bindings.info;
    const float scaledSpeed = bScalarFiniteRatio(speed,movement->bindings.state->performanceScale);
    const SampledCurve curve = carPerformanceCurve(vehicle->performance);
    float acceleration = sampledCurveEvaluate(&curve,scaledSpeed);
    const float dragCoefficient = bScalarMultiply(bScalarAdd(1.0f,-movement->bindings.owner->interactionFactor),info->aerodynamics->drag);
    const float dragForce = bScalarMultiply(dragCoefficient,bScalarMultiply(scaledSpeed,scaledSpeed));
    const float speedLimit = bScalarMultiply(info->forwardSpeedLimit,vehicle->tuning->milesPerHourToMetersPerSecond);
    const float dragAcceleration = bScalarFiniteRatio(dragForce,info->body->mass);
    if (speedLimit <= scaledSpeed && dragForce < acceleration) acceleration = dragForce;
    acceleration = bScalarMultiply(acceleration,vehicle->tuning->accelerationScale);
    float slope;
    if (movement->mode == 6) slope = beltMoverSlope(movement->belt);
    else {
        const float* direction = movement->bindings.state->orientation;
        const float squared = bScalarAdd(bScalarAdd(bScalarMultiply(direction[0],direction[0]),
                                                    bScalarMultiply(direction[1],direction[1])),
                                          bScalarMultiply(direction[2],direction[2]));
        const float inverse = bScalarFiniteRatio(1.0f,bScalarSquareRoot(squared));
        slope = bScalarMultiply(-vehicle->tuning->gravity,bScalarMultiply(direction[2],inverse));
    }
    return bScalarAdd(bScalarAdd(acceleration,-dragAcceleration),slope);
}

// PAL1CB350..1CB3CC / beta1C8368 GetVectorOnTrack, unchanged accepted body.
void pursuitGroundPoint(TerrainTopology* terrain, float output[4], const float point[2])
{
    TerrainSurface surface;
    const float query[4] = {point[0], point[1], 0.0f, 0.0f};
    terrainSurfaceInitialize(&surface, terrain);
    output[0] = point[0];
    output[1] = point[1];
    output[2] = terrainSurfaceElevation(&surface, query, NULL, NULL, NULL);
    output[3] = 0.0f;
    terrainSurfaceDispose(&surface);
}


// Road algorithms and resource ownership.
// Individual TrackRoute/AIUtil PDR+MAP/PAL evidence: road_paths77.json.
// Existing PAL algorithms are retained; only native C++ casts are adapted.
static float pointDistance(const CarRecoveryPose* pose, const float point[2])
{
    const float x = pose->position[0] - point[0];
    const float y = pose->position[1] - point[1];
    const float xx = x * x;
    const float yy = y * y;
    return sqrtf(xx + yy);
}

int32_t carRecoverySelectPose(CarRecoverySelection* selection, const CarRecoveryPointRegistry* registry,
                        const float point[2], int32_t categoryMode, uint32_t flags,
                        float routeDistance)
{
    float maximumDistance = 999999.0f;
    const CarRecoveryGroup* selectedGroup = NULL;
    const CarRecoveryPose* selectedPose = NULL;
    int32_t lowerIds = registry->reverse == 0;
    if (categoryMode == 3) lowerIds = !lowerIds;
    for (size_t index = 0; index < registry->groupCount; ++index) {
        const CarRecoveryGroup* group = &registry->groups[index];
        if (group->category != (categoryMode == 0 ? 0 : 1)) continue;
        if (categoryMode != 0 && categoryMode != 1 && (group->id < 60) != lowerIds) continue;
        if (group->flags & 4u) continue;
        const CarRecoveryPose* pose = carRecoveryFindInGroup(registry, group, point, maximumDistance,
                                                           routeDistance, flags);
        if (!pose) continue;
        const float distance = flags & 32u ? carRecoveryDistance(registry->route, pose, point, routeDistance) :
                                           pointDistance(pose, point);
        if (distance < maximumDistance) {
            maximumDistance = distance;
            selectedGroup = group;
            selectedPose = pose;
        }
    }
    if (!selectedGroup || !selectedPose) return 0;
    selection->group = selectedGroup;
    selection->pose = selectedPose;
    return 1;
}

CarDriverResult driverRoadNextPose(const CarRecoveryPointRegistry* registry, CarRecoverySelection* selection)
{
    if (!selection || carRecoveryOwnedPoseIndex(selection->group,selection->pose)<0)
        return CAR_DRIVER_INVALID_BINDING;
    const CarRecoveryPose* pose = selection->pose;
    const CarRecoveryGroup* group = selection->group;
    const int32_t index = carRecoveryOwnedPoseIndex(group,pose);
    if ((pose->flags & 255u) != 255u) {
        const CarRecoveryConnection* connection = carRecoveryPointConnection(registry, pose);
        if (!connection) return CAR_DRIVER_MISSING_ROAD;
        const uint8_t category = connection->flags & 7u;
        if (!(connection->flags & 8u) && (category == 0 || category == 2)) {
            selection->group = carRecoveryConnectionGroup(registry, connection);
            selection->pose = carRecoveryConnectionPose(registry, connection);
            return selection->group && selection->pose ? CAR_DRIVER_OK : CAR_DRIVER_MISSING_ROAD;
        }
    }
    // A validated index avoids scanning every authored point for each traffic
    // and police AI advance. The normal next-point contract is unchanged.
    selection->pose = index + 1 < group->poseCount ? &group->poses[index + 1] : NULL;
    return CAR_DRIVER_OK;
}

CarDriverResult driverRoadAdvanceSelection(const CarRecoveryPointRegistry* registry, CarRecoverySelection* selection)
{
    if (!selection || !selection->pose || !selection->group) return CAR_DRIVER_OK;
    const CarRecoveryPose* previous = selection->pose;
    CarDriverResult result = driverRoadNextPose(registry, selection);
    if (result != CAR_DRIVER_OK || (previous->flags & 255u) == 255u || !selection->pose) return result;
    const float x = selection->pose->position[0] - previous->position[0];
    const float y = selection->pose->position[1] - previous->position[1];
    const float first = x * previous->direction[0];
    const float second = y * previous->direction[1];
    if (first + second < 0.0f) {
        result = driverRoadNextPose(registry, selection);
        if (result != CAR_DRIVER_OK || !selection->pose) return CAR_DRIVER_MISSING_ROAD;
        const float deltaX = previous->position[0] - selection->pose->position[0];
        const float deltaY = previous->position[1] - selection->pose->position[1];
        const float xx = deltaX * deltaX;
        const float yy = deltaY * deltaY;
        if (sqrtf(xx + yy) < 8.0f) result = driverRoadNextPose(registry, selection);
    }
    return result;
}


typedef struct PendingBranch {
    CarRecoverySelection selection;
    float remaining;
} PendingBranch;

static const CarRecoveryPose* adjacentPose(const CarRecoveryGroup* group,
                                           const CarRecoveryPose* pose,int32_t forward)
{
    // Share the validated original-route owner with regular road traversal.
    return carRecoveryAdjacentPose(group,pose,forward);
}

static void pushFront(PendingBranch queue[20],int32_t* first,int32_t last,int32_t* full,
                       CarRecoverySelection selection,float remaining)
{
    if (*full) return;
    *first=*first ? *first-1 : 19;
    *full=*first==last;queue[*first].selection=selection;queue[*first].remaining=remaining;
}

int32_t carRecoveryFindFollowingBranches(const CarRecoveryPointRegistry* registry,
                                         CarRecoverySelection* output,int32_t capacity,
                                         const CarRecoveryGroup* group,const CarRecoveryPose* pose,
                                         float distance,int32_t forward)
{
    if (!output || capacity <= 0 || carRecoveryOwnedPoseIndex(group,pose) < 0) return 0;
    PendingBranch queue[20];
    int32_t first=0,last=0,full=0,count=0,pops=0,steps=1;
    float remaining=forward ? distance : distance+pose->segmentLength;
    for (;;) {
        remaining-=pose->segmentLength;
        if (remaining<0.0f) {
            output[count++]=CarRecoverySelection{group,pose};
            if (count==capacity) return count;
            pose=NULL;
        } else {
            int32_t advance=1;
            if ((pose->flags&255u)!=255u) {
                const CarRecoveryConnection* connection=carRecoveryPointConnection(registry,pose);
                if (!connection) return 0;
                const int32_t reverseConnection=(connection->flags&8u)!=0;
                if (forward ? !reverseConnection : reverseConnection) {
                    advance=0;
                    const uint32_t kind=connection->flags&7u;
                    if (kind==3u) remaining=-1.0f;
                    else {
                        const CarRecoveryGroup* branchGroup=carRecoveryConnectionGroup(registry,connection);
                        const CarRecoveryPose* branchPose=carRecoveryConnectionPose(registry,connection);
                        if (!branchGroup || !branchPose) return 0;
                        const float dx=branchPose->position[0]-pose->position[0];
                        const float dy=branchPose->position[1]-pose->position[1];
                        const float x=dx*pose->direction[0],y=dy*pose->direction[1];
                        const float branchRemaining=(remaining+pose->segmentLength)-(x+y);
                        if (kind==2u) { group=branchGroup;pose=branchPose;remaining=branchRemaining; }
                        else {
                            const CarRecoveryPose* next=adjacentPose(group,pose,forward);
                            if (next) pushFront(queue,&first,last,&full,CarRecoverySelection{group,next},remaining);
                            if (!kind) pushFront(queue,&first,last,&full,
                                                  CarRecoverySelection{branchGroup,branchPose},branchRemaining);
                            else if (!full) {
                                queue[last]=PendingBranch{{branchGroup,branchPose},branchRemaining};
                                if (++last==20) last=0;
                                full=last==first;
                            }
                            pose=NULL;
                        }
                    }
                }
            }
            if (advance) pose=adjacentPose(group,pose,forward);
            if (pose && !(pose->flags&0x8000u)) pose=NULL;
        }
        if (!pose) {
            if (!full && first==last) return count;
            group=queue[first].selection.group;pose=queue[first].selection.pose;
            remaining=queue[first].remaining;
            if (++first==20) first=0;
            full=0;
            if (++pops>=71) return 0;
        }
        if (++steps>0x11800) return 0;
    }
}

#include "indep/inc/eMath.hpp"
int32_t driverPerceptionNeighborCanReach(const DriverAINeighbor* neighbor,
    const DriverPhysicsVehicle* vehicle,float acceleration)
{
    const float observed=driverObservedAcceleration(vehicle);
    if(acceleration<0.0f){
        /* The original evaluates this query but keeps its input acceleration.
         * Retain the call because a spline mover can refresh its slope. */
        (void)driverAvailableAcceleration(vehicle,vehicle->movement->bindings.state->speed);
    }
    const float xx=neighbor->localPosition[0]*neighbor->localPosition[0];
    const float yy=neighbor->localPosition[1]*neighbor->localPosition[1];
    const float distance=sqrtf(xx+yy);
    const float relativeX=neighbor->travelRelativeVelocity[0]*neighbor->travelRelativeVelocity[0];
    const float relativeY=neighbor->travelRelativeVelocity[1]*neighbor->travelRelativeVelocity[1];
    const float relativeSpeed=sqrtf(relativeX+relativeY);
    DriverPhysicsVehicle other=*vehicle;
    other.movement=neighbor->movement;
    const VehicleState* state=vehicle->movement->bindings.state;
    const VehicleState* otherState=other.movement->bindings.state;
    const float first=state->orientation[0]*otherState->orientation[0];
    const float second=state->orientation[1]*otherState->orientation[1];
    const float relative=0.0f<first+second?
        observed-driverObservedAcceleration(&other):observed+driverObservedAcceleration(&other);
    const float seconds=vehicle->tuning->predictionSeconds;
    const float scaled=relative*seconds;
    const float halfTime=seconds*0.5f;
    const float change=scaled*halfTime;
    const float travel=relativeSpeed*seconds+change;
    return distance<travel;
}
#include "indep/inc/DrivingChoice.hpp"
#include "indep/inc/CameraMover.hpp"
static float authoredLateral(const CarRecoveryPose* pose,int32_t lane)
{
    return (float)((int32_t)pose->laneOffsetsQuantized[lane]*256)*0.0000152587890625f;
}
float driverLaneRoadCurvature(const DriverAIRoadState* road)
{
    if(driverAIRoadEmpty(road))return 0.0f;
    const CarRecoveryGroup* group=road->group;
    const int32_t first=carRecoveryOwnedPoseIndex(group,road->recovery.recoveryPose);
    if(first<0 || group->poseCount-first<13 || road->lane<0 || road->lane>=21)return 0.0f;
    float positions[3][2];
    for(int index=0;index<3;++index){
        const CarRecoveryPose* pose=&group->poses[first+index*6];
        carRoadPreviewPointXY(pose,positions[index],0.0f,authoredLateral(pose,road->lane));
    }
    float initial[2]={positions[0][0]-positions[1][0],positions[0][1]-positions[1][1]};
    float final[2]={positions[1][0]-positions[2][0],positions[1][1]-positions[2][1]};
    vectorNormalize2(initial,initial);vectorNormalize2(final,final);
    const float firstProduct=initial[0]*final[1];
    const float secondProduct=initial[1]*final[0];
    const float cross=firstProduct-secondProduct;
    const float squared=cross*cross;
    const float magnitude=sqrtf((0.0f+0.0f)+squared);
    return 0.0f<=cross?-magnitude:magnitude;
}
float driverPlanningDistance(const DriverPhysicsVehicle* vehicle)
{
    const float distance = vehicle->movement->bindings.state->speed * vehicle->tuning->planningSeconds;
    return maximumFinite(distance, vehicle->tuning->minimumPlanningDistance);
}
float driverBrakingAcceleration(const DriverPhysicsVehicle* vehicle, float speed)
{
    const PhysicsCarInfo* info = vehicle->movement->bindings.info;
    const float downforce = info->aerodynamics->frontDownforce + info->aerodynamics->rearDownforce;
    const float averageDownforce = downforce * 0.5f;
    const float averageGrip = (info->tires[1].staticGrip + info->tires[0].staticGrip) * 0.5f;
    const float weight = vehicle->tuning->gravity * info->body->mass;
    const float load = fabsf(speed) * averageDownforce + weight;
    const float gripForce = load * averageGrip;
    return -finiteRatio(gripForce, info->body->mass);
}
const DriverPhysicsTuning driverPhysicsDefaultTuning = {
    2.0f, 20.0f, 3.0f, 1.0f, 3.0f, 9.80000019073486328f,
    0.100000001490116119f, 0.447027266025543213f
};
float* driverPointAlongHeading(float output[4],const VehicleState* state,float distance)
{
    float sine,cosine;vectorSineCosineAngle(&sine,&cosine,state->heading);
    distance = maximumFinite(5.0f,distance);
    const float delta[4] = {cosine*distance,sine*distance,0,0};
    for (int axis = 0; axis < 4; ++axis) output[axis] = delta[axis]+state->position[axis];
    return output;
}


// Original FindRoutesToTarget / FindPointsToTarget, PAL1CC270..1CC834.


static void pendingPushFront(PendingPath* pending, CarRecoverySelection selection)
{
    if (pending->full) return;
    pending->first = pending->first ? pending->first - 1 : CAR_RECOVERY_PATH_GROUPS - 1;
    pending->selections[pending->first] = selection;
    pending->full = pending->first == pending->last;
}

int32_t carRecoveryFindPathGroups(const CarRecoveryPointRegistry* registry,
                                 const CarRecoverySelection* current,
                                 const CarRecoveryGroup* destination,
                                 CarRecoveryGroupHistory* history,
                                 int32_t forward, int32_t avoidRestricted)
{
    if (!registry || !current || !destination || !history ||
        carRecoveryOwnedPoseIndex(current->group,current->pose)<0) return 0;
    PendingPath pending = {};
    CarRecoverySelection selection = *current;
    carRecoveryPathHistoryPush(history, selection.group);
    if (selection.group == destination) return 1;
    for (;;) {
        const CarRecoveryConnection* connection;
        const CarRecoveryGroup* next;
        uint32_t policy;
        if (carRecoveryOwnedPoseIndex(selection.group,selection.pose)<0) break;
        if ((selection.pose->flags & 255u) == 255u) {
            selection.pose = carRecoveryAdjacentPose(selection.group, selection.pose, forward);
            if (selection.pose && avoidRestricted && (selection.pose->flags & 0x4000u)) {
                if (!restorePendingPath(&selection, &pending, history)) break;
            }
            continue;
        }
        connection = carRecoveryPointConnection(registry, selection.pose);
        if (!connection) break;
        if (forward ? (connection->flags & 8u) != 0 : (connection->flags & 8u) == 0) {
            selection.pose = carRecoveryAdjacentPose(selection.group, selection.pose, forward);
            if (selection.pose && avoidRestricted && (selection.pose->flags & 0x4000u)) {
                if (!restorePendingPath(&selection, &pending, history)) break;
            }
            continue;
        }
        next = carRecoveryConnectionGroup(registry, connection);
        if (!next) break;
        if (next == destination) {
            carRecoveryPathHistoryPush(history, next);
            return 1;
        }
        policy = connection->flags & 7u;
        if (carRecoveryPathHistoryContains(history, next)) {
            if (policy == 2u || policy == 3u) {
                if (!restorePendingPath(&selection, &pending, history)) break;
            }
            selection.pose = carRecoveryAdjacentPose(selection.group, selection.pose, forward);
            if (selection.pose && avoidRestricted && (selection.pose->flags & 0x4000u)) {
                if (!restorePendingPath(&selection, &pending, history)) break;
            }
            continue;
        }
        carRecoveryPathHistoryPush(history, next);
        if (policy != 2u) {
            CarRecoverySelection continuation = selection;
            continuation.pose = carRecoveryAdjacentPose(selection.group, selection.pose, forward);
            pendingPushFront(&pending, continuation);
        }
        selection.group = next;
        selection.pose = carRecoveryConnectionPose(registry, connection);
        selection.pose = carRecoveryAdjacentPose(selection.group, selection.pose, forward);
        if (selection.pose && avoidRestricted && (selection.pose->flags & 0x4000u)) {
            if (!restorePendingPath(&selection, &pending, history)) break;
        }
    }
    history->last = history->first;
    history->full = 0;
    return 0;
}

int32_t carRecoveryBuildPath(const CarRecoveryPointRegistry* registry,
                            const CarRecoverySelection* current,
                            const CarRecoverySelection* destination,
                            CarRecoverySelection* output, uint32_t capacity,
                            int32_t forward, int32_t avoidRestricted)
{
    if (!registry || !current || !destination || !output || !capacity ||
        carRecoveryOwnedPoseIndex(current->group,current->pose)<0 ||
        carRecoveryOwnedPoseIndex(destination->group,destination->pose)<0) return 0;
    CarRecoveryGroupHistory history = {};
    CarRecoverySelection selection = *current;
    int32_t count = 0;
    if (current->pose == destination->pose) {
        output[0] = *destination;
        return 1;
    }
    if (avoidRestricted && (selection.pose->flags & 0x4000u)) return 0;
    for (;;) {
        int32_t advance = 1;
        if ((selection.pose->flags & 255u) != 255u) {
            const CarRecoveryConnection* connection = carRecoveryPointConnection(registry, selection.pose);
            if (!connection) return count;
            if (forward ? !(connection->flags & 8u) : (connection->flags & 8u)) {
                const CarRecoveryGroup* next;
                const CarRecoveryGroup* expected;
                const uint32_t policy = connection->flags & 7u;
                if (policy == 3u) {
                    selection.pose = NULL;
                    advance = 0;
                } else {
                    if (policy == 2u) {
                        selection.group = carRecoveryConnectionGroup(registry, connection);
                        selection.pose = carRecoveryConnectionPose(registry, connection);
                        advance = 0;
                    } else {
                        carRecoveryFindPathGroups(registry, &selection, destination->group,
                                                  &history, forward, avoidRestricted);
                        if (carRecoveryPathHistoryEmpty(&history)) return count;
                        expected = history.groups[history.first];
                        next = carRecoveryConnectionGroup(registry, connection);
                        if (expected == next) {
                            selection.group = next;
                            selection.pose = carRecoveryConnectionPose(registry, connection);
                            advance = 0;
                        }
                    }
                }
            }
        }
        if (advance) selection.pose = carRecoveryAdjacentPose(selection.group, selection.pose, forward);
        if (!selection.pose || (avoidRestricted && (selection.pose->flags & 0x4000u))) return count;
        output[count++] = selection;
        if (selection.pose == destination->pose || (uint32_t)count >= capacity) return count;
    }
}
