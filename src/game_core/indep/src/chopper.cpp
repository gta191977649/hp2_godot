#include "indep/inc/Chopper.hpp"
#include "indep/inc/Replay.hpp"

#include "indep/inc/Chopper.hpp"
#include "indep/inc/Chopper.hpp"
#include "indep/inc/Munitions.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Player.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/WorldObject.hpp"
#include "indep/inc/CarSounds.hpp"
#include "indep/inc/RaceParameters.hpp"
#include "indep/inc/EasterEggs.hpp"
#include "bware/inc/bWare.hpp"
#include <array>
#include <memory>
#include <algorithm>

#include <string.h>

#include "indep/inc/CameraMover.hpp"
#include "Chopper.hpp"
#include "TrackRoute.hpp"
#include "Sound3d.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/TrackPath.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>

// Native ownership of source+0/+4 and the embedded downwash list. All
// target views point to the SAME World Cars; no vehicle state is copied.
struct ChopperOwnedState {
    WorldObjectRegistry* objects{};
    WorldObject* model{};
    std::unique_ptr<Sound3d> sound;
    ChopperDownwash downwash{};
    ChopperCommands commands{};
    std::array<ChopperTarget,WORLD_MAX_CARS> targets{};
    ~ChopperOwnedState() {
        chopperDownwashDispose(&downwash);
        sound.reset();
        if(model)worldObjectDispose(objects,model);
    }
};

// ClearVariables PAL1E7C38..1E7D04 intentionally preserves the phase and
// embedded terrain owner. Its route coordinate is refreshed at the origin.
void chopperClearVariables(Chopper* chopper)
{
    hp2::Require(chopper && chopper->environment.route,"Chopper clear has no loaded route");
    chopper->planningCooldown=0.0f;chopper->target=nullptr;chopper->movementMode=0;
    chopper->current={};chopper->departure={};chopper->previousTarget={};
    std::fill_n(chopper->position,3,0.0f);rotationMatrixIdentity(chopper->frame);
    trackRouteUpdateLocation(chopper->environment.route,&chopper->routeLocation,chopper->position);
    chopper->attackSeconds=25.0f;chopper->forceFirstPlayerTarget=0;
    chopper->horizontalSpeed=0.0f;chopper->forwardAcceleration=0.0f;chopper->yawRate=0.0f;
    chopper->roll=0.0f;chopper->desiredRoll=0.0f;chopper->yaw=0.0f;chopper->pitch=0.0f;
    chopper->desiredPitch=0.0f;chopper->turnRate=0.0f;chopper->verticalVelocity=0.0f;
    chopper->groundHeight=0.0f;chopper->downwashEnabled=0;chopper->targetDistance=0.0f;
    chopper->attackProfile=0;chopper->drops=0;chopper->launchPattern=0;
    chopper->dropDelay=0.0f;chopper->totalDrops=0;chopper->blockedSeconds=0.0f;
}

// Final PAL1E7D08..1E7F80. Native targets and route references are converted
// through their original stable indices; no host pointer bytes enter replay.
void chopperDoSnapshot(Chopper* chopper,ReplaySnapshot* snapshot)
{
    hp2::Require(chopper && snapshot && chopper->environment.world &&
        chopper->environment.road && chopper->environment.route,
        "Chopper snapshot has no actual World/route owners");
    replaySnapshotBeginSection(snapshot,chopper,"Chopper");
    if(snapshot->restoring) {
        chopperHide(chopper);
        hp2::Require(chopper->result==WORLD_SERVICES_OK,"Chopper snapshot could not retire its real sound/terrain state");
    }
    replaySnapshotMoveData(snapshot,&chopper->phase,1,-1,0);
    if(chopper->phase!=6) {
        replaySnapshotMoveFloat(snapshot,&chopper->planningCooldown);
        Car* target=chopper->target?chopper->target->car:nullptr;
        worldSnapshotCarPointer(chopper->environment.world,snapshot,&target);
        if(snapshot->restoring) {
            chopper->target=nullptr;
            for(std::size_t index=0;index<chopper->environment.targetCount;++index)
                if(chopper->environment.targets[index].car==target && target) {
                    chopper->target=&chopper->environment.targets[index];break;
                }
            hp2::Require(!target || chopper->target,"Chopper snapshot target is absent from its real World target views");
        }
        replaySnapshotMoveData(snapshot,&chopper->movementMode,1,-1,0);
        carRecoverySnapshotGroup(chopper->environment.road,snapshot,&chopper->current.group);
        carRecoverySnapshotPoseInGroup(chopper->current.group,snapshot,&chopper->current.pose);
        carRecoverySnapshotGroup(chopper->environment.road,snapshot,&chopper->departure.group);
        carRecoverySnapshotPoseInGroup(chopper->departure.group,snapshot,&chopper->departure.pose);
        carRecoverySnapshotGroup(chopper->environment.road,snapshot,&chopper->previousTarget.group);
        carRecoverySnapshotPoseInGroup(chopper->previousTarget.group,snapshot,&chopper->previousTarget.pose);
        trackRouteLocationSnapshot(chopper->environment.route,&chopper->routeLocation,snapshot);
        replaySnapshotMoveData(snapshot,chopper->position,12,-1,0);
        replaySnapshotMoveMatrix(snapshot,chopper->frame,64);
        replaySnapshotMoveFloat(snapshot,&chopper->horizontalSpeed);
        float acceleration[2]={chopper->forwardAcceleration,chopper->yawRate};
        replaySnapshotMoveData(snapshot,acceleration,8,-1,0);
        if(snapshot->restoring) {chopper->forwardAcceleration=acceleration[0];chopper->yawRate=acceleration[1];}
        replaySnapshotMoveFloat(snapshot,&chopper->roll);
        replaySnapshotMoveFloat(snapshot,&chopper->desiredRoll);
        replaySnapshotMoveFloat(snapshot,&chopper->yaw);
        replaySnapshotMoveFloat(snapshot,&chopper->pitch);
        replaySnapshotMoveFloat(snapshot,&chopper->desiredPitch);
        replaySnapshotMoveFloat(snapshot,&chopper->turnRate);
        replaySnapshotMoveFloat(snapshot,&chopper->verticalVelocity);
        replaySnapshotMoveFloat(snapshot,&chopper->groundHeight);
        replaySnapshotMoveData(snapshot,&chopper->downwashEnabled,1,4,0);
        replaySnapshotMoveFloat(snapshot,&chopper->targetDistance);
        replaySnapshotMoveData(snapshot,&chopper->attackProfile,1,-1,0);
        replaySnapshotMoveInteger(snapshot,&chopper->drops,4);
        replaySnapshotMoveFloat(snapshot,&chopper->dropDelay);
        replaySnapshotMoveInteger(snapshot,&chopper->launchPattern,4);
        replaySnapshotMoveFloat(snapshot,&chopper->attackSeconds);
        replaySnapshotMoveFloat(snapshot,&chopper->blockedSeconds);
        replaySnapshotMoveInteger(snapshot,&chopper->totalDrops,4);
    } else if(snapshot->recording || snapshot->restoring) {
        // The released body adds this mode gate: checksum-only traversal
        // must not clear the parked Chopper's live variables.
        chopperClearVariables(chopper);
    }
    replaySnapshotEndSection(snapshot);
}

Chopper::Chopper(const hp2::ChopperConstructionBindings& bindings)
{
    hp2::Require(bindings.terrain && bindings.objects && bindings.effects && bindings.sound &&
        bindings.parameters && bindings.environment.world &&
        bindings.environment.route && bindings.environment.route->nodeCount>0 &&
        bindings.environment.road && bindings.environment.navigation &&
        bindings.environment.munitions && bindings.environment.particles,
        "Chopper construction has incomplete loaded World/resource owners");
    environment=bindings.environment;result=WORLD_SERVICES_OK;visible=0;position[3]=0.0f;
    sound=nullptr;commands=nullptr;downwash=nullptr;
    terrainSurfaceInitialize(&terrain,bindings.terrain);
    trackRouteLocationInitialize(&routeLocation);
    auto storage=std::make_unique<ChopperOwnedState>();storage->objects=bindings.objects;
    try {
        // Final PAL2B95B8; the beta supplies no resource bytes.
        storage->model=worldObjectCreate(bindings.objects,hashString("TRACK_HELICOPTER"),nullptr,1);
        hp2::Require(storage->model,"Chopper model TRACK_HELICOPTER is unavailable");
        WorldAudioResult audioResult;
        storage->sound.reset(carSoundCreateChopper(this,bindings.sound,&audioResult));
        hp2::Require(storage->sound && audioResult==WORLD_AUDIO_OK,"Chopper sound construction failed");
        sound=storage->sound.get();
        hp2::Require(chopperDownwashInitialize(&storage->downwash,bindings.effects)==WORLD_SERVICES_OK,
            "Chopper XEM_HELI_DUST emitter construction failed");
        downwash=&storage->downwash;commands=&storage->commands;
        chopperClearVariables(this);phase=6;
        if(!bindings.parameters->fastSpeedTrapSpawns && !bindings.parameters->demoReplay) {
            // PAL cell31D7F4 is gEasterEggs.records, not loaded TrackInfo.
            // Its+0x148 is record5.unlocked (5*0x38+0x30).
            const auto& eggs=hp2::easterEggs;
            hp2::Require(eggs.records && eggs.recordCount>5,"Chopper has no canonical EasterEggs table");
            if(eggs.records[5].unlocked)forceFirstPlayerTarget=1;
        }
        ownedState=storage.release();hp2::ChopperSynchronizeTargets(*this);
    } catch(...) {
        // Synchronization validates the published owner. Retire it too if
        // validation fails after release(), just as before publication.
        delete ownedState;ownedState=nullptr;
        sound=nullptr;commands=nullptr;downwash=nullptr;
        terrainSurfaceDispose(&terrain);throw;
    }
}

Chopper::~Chopper()
{
    if(!ownedState)return;
    delete ownedState;ownedState=nullptr;sound=nullptr;downwash=nullptr;commands=nullptr;
    terrainSurfaceDispose(&terrain);
}

namespace hp2 {
void ChopperSynchronizeTargets(Chopper& chopper)
{
    if(!chopper.ownedState)return;
    World& world=*chopper.environment.world;
    Require(world.totalCars>=0 && world.totalCars<=WORLD_MAX_CARS,"Chopper has an invalid World car table");
    Car* previous=chopper.target?chopper.target->car:nullptr;chopper.target=nullptr;
    auto& targets=chopper.ownedState->targets;
    for(size_t i=0;i<targets.size();++i) {
        Car* car=i<static_cast<size_t>(world.totalCars)?world.carTable[i]:nullptr;
        targets[i]={car,car?car->Movement():nullptr,car?car->Driver():nullptr};
        if(car && car==previous)chopper.target=&targets[i];
    }
    chopper.environment.targets=targets.data();chopper.environment.targetCount=static_cast<size_t>(world.totalCars);
}
}

void chopperApproach(float target,float* value,float maximumStep)
{
    const float difference=fabsf(target-*value);
    if (difference<maximumStep) maximumStep=difference;
    if (target<*value) maximumStep=-maximumStep;
    *value+=maximumStep;
}

void chopperRotateYaw(Chopper* chopper,float degrees)
{
    chopper->yaw-=degrees;
    if (chopper->yaw<0.0f) chopper->yaw+=360.0f;
    else if (chopper->yaw>360.0f) chopper->yaw-=360.0f;
}

float chopperMinimumAcceleration(const Chopper* chopper)
{
    return chopper->phase==1 || chopper->phase==2 ? -120.0f : -60.0f;
}

float chopperResponseScale(float value) { return value*100.0f; }

void chopperChooseYawRate(Chopper* chopper,const float desired[2],float error)
{
    chopper->yawRate=fabsf(error)>=90.0f ? 0.0f : chopperResponseScale(desired[1]);
    const float magnitude=fabsf(chopper->yawRate);chopper->desiredRoll=0.0f;
    if (magnitude>5.0f) {
        float roll=magnitude-5.0f;if (roll>20.0f) roll=20.0f;
        if (chopper->yawRate<0.0f) roll=-roll;
        chopper->desiredRoll=roll;
    }
}

void chopperChooseTurnRate(Chopper* chopper,float desired,float scale)
{
    const float limit=scale*90.0f;
    chopper->turnRate=desired< -limit ? -limit : (desired>limit ? limit : desired);
    chopper->desiredRoll=0.0f;
}

void chopperSetGoalSpeed(const Chopper* chopper, float goal[3])
{
    goal[2] = chopper->target->movement->bindings.state->speed + 30.0f;
}

void chopperSetCurrentGoal(const Chopper* chopper, float goal[3])
{
    goal[0] = chopper->current.pose->position[0];
    goal[1] = chopper->current.pose->position[1];
}

WorldServicesResult chopperDownwashInitialize(ChopperDownwash* downwash, ChopperMunitionEffects* effects)
{
    if (!downwash || !effects) return WORLD_SERVICES_INVALID_BINDING;
    memset(downwash, 0, sizeof(*downwash));
    downwash->effects = effects;
    return chopperEffectsCreatePersistentGroup(effects, hashString("XEM_HELI_DUST"),
        &downwash->first, &downwash->last);
}

void chopperDownwashDispose(ChopperDownwash* downwash)
{
    if (!downwash || !downwash->effects) return;
    chopperEffectsReleasePersistentGroup(downwash->effects, &downwash->first, &downwash->last);
}

void chopperUpdateDownwash(Chopper* chopper, float elapsed)
{
    const float height = chopper->position[2] - chopper->groundHeight;
    const float intensity = (11.0f - height) * 0.10000000149011612f;
    if (!(0.0f < intensity)) return;
    ChopperDownwash* downwash = chopper->downwash;
    if (!downwash || !downwash->effects) {
        chopper->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    float frame[16];
    rotationMatrixIdentity(frame);
    frame[12] = chopper->position[0];
    frame[13] = chopper->position[1];
    frame[14] = chopper->groundHeight;
    const float delta = (-minimumFinite(intensity, 1.0f)) * elapsed;
    for (ChopperEffectInstance* instance = downwash->first; instance; instance = instance->next) {
        const WorldServicesResult result = chopperEffectsEmit(downwash->effects, instance, frame, delta);
        if (result != WORLD_SERVICES_OK) { chopper->result = result; return; }
    }
}

static ChopperTarget* firstTarget(Chopper* chopper)
{
    const World* world = chopper->environment.world;
    if (!world || world->totalCars <= 0) {
        chopper->result = WORLD_SERVICES_INVALID_BINDING;
        return NULL;
    }
    for (size_t index = 0; index < chopper->environment.targetCount; ++index)
        if (chopper->environment.targets[index].car == world->carTable[0])
            return &chopper->environment.targets[index];
    chopper->result = WORLD_SERVICES_INVALID_BINDING;
    return NULL;
}

void chopperProcessCommands(Chopper* chopper)
{
    if (!chopper->commands) { chopper->result = WORLD_SERVICES_INVALID_BINDING; return; }
    if (chopper->forceFirstPlayerTarget) {
        ChopperTarget* first = firstTarget(chopper);
        if (!first) return;
        if (chopper->target != first) chopperShiftTarget(chopper, first);
    }
    if (chopper->commands->toggleTarget) {
        if (chopper->phase == 6) {
            ChopperTarget* first = firstTarget(chopper);
            if (!first) return;
            chopperShiftTarget(chopper, first);
        } else chopperShiftTarget(chopper, NULL);
        chopper->commands->toggleTarget = 0;
    }
    if (chopper->commands->dropSmokeAtFirstPlayer) {
        ChopperMunitionRegistry* registry = chopper->environment.munitions;
        if (!registry || !registry->bindings.players || registry->bindings.players->count <= 0) {
            chopper->result = WORLD_SERVICES_INVALID_BINDING;
            return;
        }
        const CarMovement* first = registry->bindings.players->players[0]->car;
        if (!first || !first->bindings.state || !first->bindings.owner) {
            chopper->result = WORLD_SERVICES_INVALID_BINDING;
            return;
        }
        float frame[16];
        memcpy(frame, first->bindings.state->orientation, sizeof(frame));
        memcpy(frame + 12, first->bindings.owner->position, 3 * sizeof(float));
        frame[14] = frame[14] + 15.0f;
        ChopperMunition* smoke = chopperSmokeBombCreate(registry);
        if (!smoke) { chopper->result = registry->result; return; }
        chopperMunitionSetTransform(smoke, frame, first->bindings.state->velocity);
        chopper->commands->dropSmokeAtFirstPlayer = 0;
    }
}

void chopperUpdateGuidance(Chopper* chopper)
{
    float goal[3];
    chopper->movementMode = 0;
    if (chopperPlanGoal(chopper, goal) || chopper->result != WORLD_SERVICES_OK) return;
    if (chopperTryTurnInPlace(chopper, goal)) return;
    const float error = chopperWrapDegrees((float)vectorAtan2Angle(goal[0], goal[1]) * 0.0054931640625f);
    if (fabsf(chopper->horizontalSpeed) <= 0.0010000000474974513f && 20.0f < fabsf(error)) {
        chopper->movementMode = 1;
        chopperChooseTurnRate(chopper, error, 2.0f);
        return;
    }
    chopperChooseYawRate(chopper, goal, error);
    chopperChooseAcceleration(chopper, goal);
    chopperAvoidCorridorEdges(chopper);
}

WorldServicesResult chopperAdvance(Chopper* chopper, float elapsed)
{
    if (!chopper || !chopper->environment.particles || !chopper->environment.munitions ||
        !chopper->environment.munitions->bindings.effects ||
        chopper->environment.munitions->bindings.effects->particles != chopper->environment.particles)
        return WORLD_SERVICES_INVALID_BINDING;
    chopperProcessCommands(chopper);
    if (chopper->result != WORLD_SERVICES_OK) return chopper->result;
    ChopperMunitionEffects* effects = chopper->environment.munitions->bindings.effects;
    chopperEffectsSweep(effects);
    if (effects->result != WORLD_SERVICES_OK) return chopper->result = effects->result;
    if (chopper->phase == 6) return chopper->result;
    chopper->planningCooldown = chopper->planningCooldown - elapsed;
    if (chopper->planningCooldown < 0.0f) {
        chopper->planningCooldown = chopper->planningCooldown + 0.10000000149011612f;
        chopperUpdateGuidance(chopper);
        if (chopper->result != WORLD_SERVICES_OK) return chopper->result;
    }
    if (chopper->phase == 3) return chopper->result;
    const int32_t moved = chopperMove(chopper, elapsed);
    if (chopper->result != WORLD_SERVICES_OK) return chopper->result;
    /* DrawChopper's Render member publishes this live frame/visibility to
     * the model. The native renderer borrows Chopper.frame and modelKey;
     * it does not maintain another physical transform or gate emission. */
    chopper->visible = 1;
    if(chopper->ownedState) {
        auto* model=chopper->ownedState->model;
        std::copy_n(chopper->frame,16,model->frame);model->hasFrame=1;
    }
    if (chopper->downwashEnabled) chopperUpdateDownwash(chopper, elapsed);
    if (chopper->result != WORLD_SERVICES_OK) return chopper->result;
    if (moved) chopper->blockedSeconds = 0.0f;
    else {
        chopper->blockedSeconds = chopper->blockedSeconds + elapsed;
        if (0.5f <= chopper->blockedSeconds) {
            chopper->blockedSeconds = 0.0f;
            chopper->phase = 5;
        }
    }
    return chopper->result;
}

void chopperBuildFrame(Chopper* chopper)
{
    float yaw=180.0f-chopper->yaw;if (yaw<0.0f) yaw+=360.0f;
    rotationMatrixIdentity(chopper->frame);
    chopper->frame[0]=chopper->frame[5]=chopper->frame[10]=1.1f;
    motionMatrixRotateX(chopper->frame,chopper->frame,(uint16_t)(convertFloatToInt32(chopper->roll*65536.0f)/360));
    motionMatrixRotateY(chopper->frame,chopper->frame,(uint16_t)(convertFloatToInt32((360.0f-chopper->pitch)*65536.0f)/360));
    motionMatrixRotateYaw(chopper->frame,chopper->frame,(uint16_t)(convertFloatToInt32(yaw*65536.0f)/360));
    chopper->frame[12]=chopper->position[0];chopper->frame[13]=chopper->position[1];chopper->frame[14]=chopper->position[2]+2.3f;
}

void chopperAttackGoal(Chopper* chopper, float goal[3])
{
    const CarRecoveryPose* pose = carDriverCurrentPose(chopper->target->driver, 1);
    if (pose != chopper->previousTarget.pose) {
        const CarRecoveryGroup* group = carDriverCurrentGroup(chopper->target->driver, 1);
        const float x = pose->position[0] - chopper->previousTarget.pose->position[0];
        const float y = pose->position[1] - chopper->previousTarget.pose->position[1];
        const float xx = x * x, yy = y * y;
        float distance = sqrtf(xx + yy);
        if (group != chopper->previousTarget.group || chopper->previousTarget.pose < pose)
            distance = -distance;
        chopper->targetDistance += distance;
        chopper->previousTarget.group = group;
        chopper->previousTarget.pose = pose;
    }
    chopperRefreshAttackDeparture(chopper);
    if (chopper->result != WORLD_SERVICES_OK) return;
    int32_t drop = 0;
    if (chopper->drops > 0) {
        chopper->dropDelay -= 0.1f;
        drop = chopper->dropDelay < 0.0f;
    } else {
        const float x = chopper->position[0] - chopper->departure.pose->position[0];
        const float y = chopper->position[1] - chopper->departure.pose->position[1];
        const float xx = x * x, yy = y * y;
        if (sqrtf(xx + yy) < 30.0f) {
            const CarRecoveryPose* current = chopper->current.pose;
            const float lateralX = (chopper->position[0] - current->position[0]) * current->direction[1];
            const float lateralY = (chopper->position[1] - current->position[1]) * current->direction[0];
            float lateral = lateralX - lateralY;
            if (0.0f < lateral) lateral -= current->halfWidths[1];
            else lateral = -(lateral + current->halfWidths[0]);
            drop = lateral < 0.0f;
        }
    }
    if (drop && aerialNavigationQuery(chopper->environment.navigation, chopper->position, NULL, NULL)) {
        chopperRefreshGround(chopper);
        if (chopper->position[2] - chopper->groundHeight <= 10.0f) {
            chopperDropMunitions(chopper);
            if (chopper->result != WORLD_SERVICES_OK) return;
            ++chopper->drops;
            ++chopper->totalDrops;
            chopper->attackSeconds = 25.0f;
            const ChopperAttackProfile* profile = &chopper->environment.profiles[chopper->attackProfile];
            chopper->dropDelay = profile->dropDelay;
            if (chopper->drops >= profile->dropLimit) {
                chopper->phase = 0;
                return;
            }
        }
    }
    if (chopper->attackSeconds == 0.0f) chopper->phase = 0;
    else chopperPlanPathGoal(chopper, goal, &chopper->departure, 1);
}

int32_t chopperPlanGoal(Chopper* chopper, float goal[3])
{
    if (!chopper->target || !chopper->target->car || !chopper->target->movement ||
        !chopper->target->movement->bindings.state || !chopper->target->driver ||
        !chopper->environment.navigation || !chopper->environment.profiles ||
        chopper->attackProfile < 0 || (size_t)chopper->attackProfile >= chopper->environment.profileCount) {
        chopper->result = WORLD_SERVICES_INVALID_BINDING;
        return 1;
    }
    chopper->attackSeconds -= 0.1f;
    if (chopper->attackSeconds < 0.0f) chopper->attackSeconds = 0.0f;
    float height = 0.0f;
    uint32_t flags = 0;
    int32_t found = 0;
    if (chopper->phase != 5) {
        const VehicleState* target = chopper->target->movement->bindings.state;
        vectorMultiplyAdd2(goal, target->position, target->velocity, 0.3f);
        found = aerialNavigationQuery(chopper->environment.navigation, goal, &height, &flags);
    }
    if (chopper->phase != 3) {
        chopperRefreshCurrentRoad(chopper);
        if (chopper->result != WORLD_SERVICES_OK) return 1;
    }
    goal[2] = -1.0f;
    if (chopper->phase == 4) {
        chopperAttackGoal(chopper, goal);
        if (chopper->result != WORLD_SERVICES_OK) return 1;
    } else if (chopper->phase == 1 || chopper->phase == 2) {
        if (found) {
            const float x = goal[0] - chopper->position[0], y = goal[1] - chopper->position[1];
            const float xx = x * x, yy = y * y;
            if (sqrtf(xx + yy) < 60.0f || chopper->attackSeconds == 0.0f) chopper->phase = 0;
        }
        if (chopper->phase == 1) {
            const float x = chopper->position[0] - chopper->departure.pose->position[0];
            const float y = chopper->position[1] - chopper->departure.pose->position[1];
            const float xx = x * x, yy = y * y;
            if (sqrtf(xx + yy) < 30.0f) {
                chopper->phase = 2;
                chopper->attackSeconds = 45.0f;
            }
            if (chopper->phase == 2) {
                goal[0] = chopper->departure.pose->position[0];
                goal[1] = chopper->departure.pose->position[1];
            } else chopperPlanPathGoal(chopper, goal, &chopper->departure, 1);
        }
    } else if (chopper->phase == 3) {
        if (chopperEnterCorridor(chopper, goal, found, flags, height)) return 1;
    } else if (chopper->phase == 5) {
        if (chopperLeaveCorridor(chopper, goal)) return 1;
    }
    if (chopper->result != WORLD_SERVICES_OK) return 1;
    if (chopper->phase == 0) chopperFollowGoal(chopper, goal, found, flags);
    if (chopper->result != WORLD_SERVICES_OK) return 1;
    chopperKeepGoalClearance(chopper, goal);
    chopperDestinationToLocal(chopper, goal);
    return 0;
}

void chopperCreateMunition(Chopper* chopper, const float frame[16], float spread)
{
    ChopperMunitionRegistry* registry = chopper->environment.munitions;
    if (!registry || !chopper->target || !chopper->target->movement ||
        !chopper->target->movement->bindings.state) {
        chopper->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    const CarMovement* target = chopper->target->movement;
    if (target->bindings.player && 30.0f < target->bindings.state->speed) {
        if (!registry->bindings.replay) {
            chopper->result = WORLD_SERVICES_INVALID_BINDING;
            return;
        }
        replayManagerAddHighlight(registry->bindings.replay, 4.0f);
    }
    const ChopperAttackProfile* profile = &chopper->environment.profiles[chopper->attackProfile];
    float velocity[4] = {0, 0, 0, 0};
    ChopperMunition* actor;
    if (profile->munitionKind == CHOPPER_MUNITION_ROCKET) {
        chopperRocketLaunchVelocity(chopper, velocity, frame + 12);
        actor = chopperRocketCreate(registry);
        if (actor) chopperRocketLaunch(actor, frame, velocity, chopper->target);
    } else {
        chopperBombLaunchVelocity(chopper, velocity, spread);
        actor = profile->munitionKind == CHOPPER_MUNITION_SMOKE_BOMB ?
            chopperSmokeBombCreate(registry) : chopperSpikeBombCreate(registry);
        if (actor) chopperMunitionSetTransform(actor, frame, velocity);
    }
    if (!actor) chopper->result = registry->result;
}

void chopperDropMunitions(Chopper* chopper)
{
    if (!chopper->environment.profiles || chopper->attackProfile < 0 ||
        (size_t)chopper->attackProfile >= chopper->environment.profileCount) {
        chopper->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    const ChopperAttackProfile* profile = &chopper->environment.profiles[chopper->attackProfile];
    const int32_t count = profile->projectilesPerDrop;
    float frame[16];
    memcpy(frame, chopper->frame, sizeof(frame));
    if (count == 1) {
        if (profile->munitionKind == CHOPPER_MUNITION_ROCKET) {
            frame[14] = frame[14] - 1.4700000286102295f;
            float offset[2] = {0.05999999865889549f,
                chopper->launchPattern < 2 ? 1.2999999523162842f : 1.619999885559082f};
            if (chopper->launchPattern % 2 == 1) offset[1] = -offset[1];
            chopperRotate2(offset, offset, chopper->yaw);
            frame[12] = frame[12] + offset[0];
            frame[13] = frame[13] + offset[1];
            ++chopper->launchPattern;
            if (chopper->launchPattern == 4) chopper->launchPattern = 0;
        } else frame[14] = frame[14] - 2.299999952316284f;
        float spread = profile->spread;
        if (0.0f < spread) {
            if (!chopper->environment.world) {
                chopper->result = WORLD_SERVICES_INVALID_BINDING;
                return;
            }
            const float variation = worldRandomFloat(chopper->environment.world, spread);
            spread = variation - spread * 0.5f;
        }
        chopperCreateMunition(chopper, frame, spread);
        return;
    }
    float offset[2] = {0, -(float)(count - 1)};
    chopperRotate2(offset, offset, chopper->yaw);
    const float scale = finiteRatio(-2.0f, (float)count);
    const float halfSpread = profile->spread * 0.5f;
    frame[12] = frame[12] + offset[0];
    frame[13] = frame[13] + offset[1];
    frame[14] = frame[14] - 2.299999952316284f;
    offset[0] = offset[0] * scale;
    offset[1] = offset[1] * scale;
    float spread = (-halfSpread) * (float)(count - 1);
    for (int32_t index = 0; index < count; ++index) {
        chopperCreateMunition(chopper, frame, spread);
        if (chopper->result != WORLD_SERVICES_OK) return;
        frame[12] = frame[12] + offset[0];
        frame[13] = frame[13] + offset[1];
        spread = spread + profile->spread;
    }
}

void chopperBombLaunchVelocity(const Chopper* chopper, float output[4], float spread)
{
    const ChopperAttackProfile* profile = &chopper->environment.profiles[chopper->attackProfile];
    output[0] = profile->lateralLaunchSpeed;
    output[2] = profile->launchSpeed;
    const float height = chopper->groundHeight - chopper->position[2];
    const float fallTime = finiteRatio(height, profile->launchSpeed);
    output[1] = finiteRatio(spread, fallTime);
    chopperRotate2(output, output, chopper->yaw);
}

void chopperRocketLaunchVelocity(const Chopper* chopper, float output[4], const float origin[4])
{
    for (int lane = 0; lane < 4; ++lane) output[lane] = chopper->target->car->position[lane] - origin[lane];
    const ChopperAttackProfile* profile = &chopper->environment.profiles[chopper->attackProfile];
    vectorSetLength3(output, output, profile->launchSpeed);
}

int32_t chopperUpdateSound(Chopper* chopper)
{
    if (!chopper || !chopper->sound) return 0;
    return chopper->sound->Update(1.0f, 0, 1.0f) == WORLD_AUDIO_OK;
}

// PAL1EAD80..1EADAC: dispatch to the existing child, without another mute
// state or replacing the actual ChopperSound/Sound3d owner.
int32_t chopperMuteSound(Chopper* chopper,int32_t muted)
{
    if (!chopper || !chopper->sound)return 0;
    return chopper->sound->Pause(muted)==WORLD_AUDIO_OK;
}

// Individual full PAL identities in proof_attempt03.json. Retired ChopperPhase.c.
void chopperPlace(Chopper* chopper, const float point[2], float height)
{
    if (!chopper->target || !chopper->target->movement || !chopper->target->movement->bindings.state) {
        chopper->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    if (!chopper->sound || chopper->sound->Activate(1)!=WORLD_AUDIO_OK) {
        chopper->result=WORLD_SERVICES_OWNER_FAILURE;return;
    }
    const VehicleState* target = chopper->target->movement->bindings.state;
    chopper->position[0] = point[0];
    chopper->position[1] = point[1];
    chopper->position[2] = height;
    chopper->yaw = (float)target->heading * 0.0054931640625f;
    chopper->pitch = chopper->desiredPitch = 0.0f;
    chopper->roll = chopper->desiredRoll = 0.0f;
    chopper->horizontalSpeed = 0.0f;
    chopper->routeLocation.node = NULL;
    trackRouteUpdateLocation(chopper->environment.route, &chopper->routeLocation, chopper->position);
    terrainSurfaceUpdate(&chopper->terrain, chopper->position, 1.0f, NULL);
    chopper->groundHeight = target->position[2];
    chopperRefreshGround(chopper);
    chopperResetCurrentRoad(chopper);
    chopperBuildFrame(chopper);
}

void chopperHide(Chopper* chopper)
{
    chopper->visible = 0;
    if (!chopper->sound || chopper->sound->Activate(0)!=WORLD_AUDIO_OK) {
        chopper->result=WORLD_SERVICES_OWNER_FAILURE;return;
    }
    if(chopper->ownedState)chopper->ownedState->model->hasFrame=0;
    terrainSurfaceReset(&chopper->terrain);
}

int32_t chopperEnterCorridor(Chopper* chopper, float goal[3], int32_t corridorFound,
    uint32_t flags, float corridorHeight)
{
    if (!corridorFound || (flags & 2u)) return 1;
    chopperPlace(chopper, goal, corridorHeight + 150.0f);
    if (chopper->result != WORLD_SERVICES_OK) return 1;
    chopper->phase = 0;
    chopper->downwashEnabled = (int32_t)((flags ^ 1u) & 1u);
    chopper->verticalVelocity = -25.0f;
    return 0;
}

int32_t chopperLeaveCorridor(Chopper* chopper, float goal[3])
{
    float height;
    uint32_t flags;
    if (aerialNavigationQuery(chopper->environment.navigation, chopper->position, &height, &flags)) {
        if (height + 150.0f <= chopper->position[2]) {
            chopperHide(chopper);
            if (chopper->result != WORLD_SERVICES_OK) return 1;
            chopper->phase = 6;
            return 1;
        }
        if (!(flags & 2u) || chopper->verticalVelocity != 0.0f) {
            chopper->verticalVelocity = 25.0f;
            goal[0] = chopper->position[0];
            goal[1] = chopper->position[1];
            return 0;
        }
    }
    CarRecoverySelection last = chopper->current;
    CarRecoverySelection next = chopper->current;
    for (;;) {
        if (driverRoadNextPose(chopper->environment.road, &next) != CAR_DRIVER_OK) {
            chopper->result = WORLD_SERVICES_OWNER_FAILURE;
            return 1;
        }
        if (!next.pose) break;
        last = next;
        const float x = next.pose->position[0] - chopper->position[0];
        const float y = next.pose->position[1] - chopper->position[1];
        const float xx = x * x, yy = y * y;
        if (30.0f < sqrtf(xx + yy)) break;
    }
    goal[0] = last.pose->position[0];
    goal[1] = last.pose->position[1];
    goal[2] = 50.0f;
    return 0;
}

void chopperFollowGoal(Chopper* chopper, float goal[3], int32_t corridorFound, uint32_t flags)
{
    if (chopper->verticalVelocity != 0.0f && ((flags & 2u) || !corridorFound)) {
        goal[0] = chopper->position[0];
        goal[1] = chopper->position[1];
        return;
    }
    const float x = goal[0] - chopper->position[0];
    const float y = goal[1] - chopper->position[1];
    const float xx = x * x, yy = y * y;
    const int32_t close = sqrtf(xx + yy) < chopperPlanningLookahead(chopper);
    if (!chopper->target || !chopper->target->driver) {
        chopper->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    const CarRecoveryGroup* group = carDriverCurrentGroup(chopper->target->driver, 1);
    const CarRecoveryPose* pose = carDriverCurrentPose(chopper->target->driver, 1);
    if (!group || !pose) {
        chopperConstrainCurrentGoal(chopper, goal);
        return;
    }
    const CarRecoveryPose* following = carRecoveryFirstFollowingPose(group, goal, pose);
    if (following) pose = following;
    if (corridorFound && close) {
        if (20.0f < chopper->horizontalSpeed) {
            const float alongX = (goal[0] - pose->position[0]) * pose->direction[0];
            const float alongY = (goal[1] - pose->position[1]) * pose->direction[1];
            carRoadPreviewPointXY(pose, goal, alongX + alongY, 0.0f);
        }
        return;
    }
    const CarRecoverySelection selection = {group, pose};
    if ((!corridorFound && (pose->flags & 0x4000u)) || chopperIsGroupBehind(chopper, group))
        chopperChooseRepositionGoal(chopper, goal, &selection);
    else if (!close)
        chopperPlanPathGoal(chopper, goal, &selection, chopperTargetBehind(chopper));
    else chopperConstrainGoalToCorridor(chopper, goal, &selection);
}

// Chopper planning uses the live pursuit and road state.
static float planarDistance(const float first[2], const float second[2])
{
    const float x = first[0] - second[0];
    const float y = first[1] - second[1];
    const float xx = x * x;
    const float yy = y * y;
    return sqrtf(xx + yy);
}

void chopperResetCurrentRoad(Chopper* chopper)
{
    CarRecoverySelection selected;
    if (carRecoverySelectPose(&selected, chopper->environment.road, chopper->position, 0, 6, 0.0f))
        chopper->current = selected;
}

void chopperRefreshCurrentRoad(Chopper* chopper)
{
    const CarRecoveryGroup* group = chopper->current.group;
    const CarRecoveryPose* pose = carRecoveryFindInGroup(chopper->environment.road, group,
        chopper->position, 24.0f, 0.0f, 6);
    if (!pose && (chopper->current.pose->flags & 255u) != 255u) {
        const CarRecoveryConnection* connection = carRecoveryPointConnection(chopper->environment.road,
                                                                             chopper->current.pose);
        group = carRecoveryConnectionGroup(chopper->environment.road, connection);
        pose = carRecoveryFindInGroup(chopper->environment.road, group, chopper->position, 24.0f, 0.0f, 6);
    }
    if (pose) chopper->current = CarRecoverySelection{group, pose};
    else chopperResetCurrentRoad(chopper);
}

int32_t chopperBacktrackToBranch(const Chopper* chopper, CarRecoverySelection* selection, int32_t forward)
{
    int32_t count = 0;
    const int32_t reverse = forward ^ 1;
    for (;;) {
        const int32_t walked = carRecoveryAdvanceToConnection(selection, reverse);
        const CarRecoveryConnection* connection;
        if (!walked) return 0;
        count += walked;
        connection = carRecoveryPointConnection(chopper->environment.road, selection->pose);
        selection->group = carRecoveryConnectionGroup(chopper->environment.road, connection);
        selection->pose = carRecoveryConnectionPose(chopper->environment.road, connection);
        if ((connection->flags & 7u) != 2u) return count;
        ++count;
        selection->pose = carRecoveryAdjacentPose(selection->group, selection->pose, reverse);
    }
}

int32_t chopperFindNearbyAlternative(const Chopper* chopper, CarRecoverySelection* output,
                                    const CarRecoverySelection* current, int32_t forward,
                                    float maximumDistance)
{
    int32_t remaining;
    *output = *current;
    remaining = chopperBacktrackToBranch(chopper, output, forward);
    if (!remaining) return 0;
    remaining *= 2;
    while (remaining) {
        if (planarDistance(output->pose->position, chopper->position) < maximumDistance) return 1;
        output->pose = carRecoveryAdjacentPose(output->group, output->pose, forward);
        if (!output->pose) return 0;
        --remaining;
    }
    return 0;
}

float chopperPlanningLookahead(const Chopper* chopper)
{
    const float distance = chopper->horizontalSpeed * 0.1f;
    return distance < 30.0f ? 30.0f : distance;
}

void chopperPlanPathGoal(Chopper* chopper, float goal[3], const CarRecoverySelection* destination,
                          int32_t forward)
{
    CarRecoverySelection path[5];
    CarRecoverySelection alternative;
    const float lookahead = chopperPlanningLookahead(chopper);
    const int32_t distant = lookahead < planarDistance(chopper->current.pose->position, chopper->position);
    int32_t count;
    int32_t selected;
    if (chopper->current.pose == destination->pose || distant) {
        if (distant) chopperSetGoalSpeed(chopper, goal);
        chopperSetCurrentGoal(chopper, goal);
        return;
    }
    count = carRecoveryBuildPath(chopper->environment.road, &chopper->current, destination,
                                 path, 5, forward, 1);
    if (!count && chopperFindNearbyAlternative(chopper, &alternative, &chopper->current, forward, lookahead))
        count = carRecoveryBuildPath(chopper->environment.road, &alternative, destination, path, 5, forward, 1);
    if (!count) return;
    for (selected = 0; selected < count; ++selected) {
        if (path[selected].pose == destination->pose ||
            lookahead < planarDistance(path[selected].pose->position, chopper->position)) break;
    }
    if (selected == count) selected = count - 1;
    if (path[selected].pose != destination->pose) chopperSetGoalSpeed(chopper, goal);
    goal[0] = path[selected].pose->position[0];
    goal[1] = path[selected].pose->position[1];
}

void chopperConstrainGoalToCorridor(const Chopper* chopper, float goal[3],
                                    const CarRecoverySelection* road)
{
    const CarRecoveryPose* pose = road->pose;
    const float x = goal[0] - pose->position[0];
    const float y = goal[1] - pose->position[1];
    const float acrossFirst = x * pose->direction[1];
    const float acrossSecond = y * pose->direction[0];
    const float alongFirst = x * pose->direction[0];
    const float alongSecond = y * pose->direction[1];
    float across = (acrossFirst - acrossSecond) * 0.7f;
    float along = alongFirst + alongSecond;
    do {
        along *= 0.7f;
        carRoadPreviewPointXY(pose, goal, along, across);
        if (aerialNavigationQuery(chopper->environment.navigation, goal, NULL, NULL)) break;
        across *= 0.7f;
    } while (1);
}

void chopperConstrainCurrentGoal(const Chopper* chopper, float goal[3])
{
    chopperConstrainGoalToCorridor(chopper, goal, &chopper->current);
}

void chopperChooseRepositionGoal(Chopper* chopper, float goal[3],
                                 const CarRecoverySelection* first)
{
    CarRecoverySelection selection = *first;
    for (;;) {
        const CarRecoveryConnection* connection;
        const CarRecoveryPose* nearest;
        if (!selection.pose) {
            chopperConstrainCurrentGoal(chopper, goal);
            return;
        }
        if ((selection.pose->flags & 0x4000u) || (selection.pose->flags & 255u) == 255u) {
            if (driverRoadNextPose(chopper->environment.road, &selection) != CAR_DRIVER_OK) {
                chopper->result = WORLD_SERVICES_OWNER_FAILURE;
                return;
            }
            continue;
        }
        connection = carRecoveryPointConnection(chopper->environment.road, selection.pose);
        if (connection->flags & 16u) {
            if (driverRoadNextPose(chopper->environment.road, &selection) != CAR_DRIVER_OK) {
                chopper->result = WORLD_SERVICES_OWNER_FAILURE;
                return;
            }
            continue;
        }
        if (connection->flags & 8u) chopper->departure = selection;
        else {
            chopper->departure.group = carRecoveryConnectionGroup(chopper->environment.road, connection);
            chopper->departure.pose = carRecoveryConnectionPose(chopper->environment.road, connection);
        }
        nearest = carRecoveryFindInGroup(chopper->environment.road, chopper->departure.group,
                                         chopper->position, 400.0f, 0.0f, 0);
        if (nearest && chopper->departure.pose < nearest) {
            chopper->departure = chopper->current;
            chopperSetCurrentGoal(chopper, goal);
            return;
        }
        chopper->phase = 1;
        chopper->attackSeconds = 60.0f;
        chopperPlanPathGoal(chopper, goal, &chopper->departure, 1);
        return;
    }
}

int32_t chopperKeepGoalClearance(Chopper* chopper, float goal[3])
{
    int32_t attempt;
    for (attempt = 0; attempt < 3; ++attempt) {
        const int32_t behind = chopperNavigationProbe(chopper, goal[0] - 4.5f, goal[1]);
        const int32_t ahead = chopperNavigationProbe(chopper, goal[0] + 4.5f, goal[1]);
        const int32_t right = chopperNavigationProbe(chopper, goal[0], goal[1] + 4.5f);
        const int32_t left = chopperNavigationProbe(chopper, goal[0], goal[1] - 4.5f);
        int32_t changed = 0;
        if (!behind && ahead) { goal[0] += 1.5f; changed = 1; }
        else if (!ahead && behind) { goal[0] -= 1.5f; changed = 1; }
        if (!right && left) { goal[1] -= 1.5f; changed = 1; }
        else if (!left && right) { goal[1] += 1.5f; changed = 1; }
        if (!changed) return 1;
    }
    return 0;
}

int32_t chopperIsGroupBehind(const Chopper* chopper, const CarRecoveryGroup* group)
{
    CarRecoverySelection previous = chopper->current;
    if (chopper->current.group == group) return 0;
    if (!chopperBacktrackToBranch(chopper, &previous, 1)) return 0;
    return previous.group == group;
}

// Released attack profile data and the remaining original Chopper methods.
const ChopperAttackProfile* chopperAttackProfiles(size_t* count)
{
    static const ChopperAttackProfile profiles[]={
        {0,2,20,0,1,1,0,0},{1,2,-10,0,1,8,30,0.5f},{1,1.5f,-10,0,2,3,5,0.5f},
        {2,2,-10,0,1,1,0,0},{2,1,-10,0,1,2,4,1}};
    if (count) *count=sizeof(profiles)/sizeof(profiles[0]);
    return profiles;
}

int32_t chopperTargetBehind(const Chopper* chopper)
{
    return trackWrappedDelta(chopper->environment.route->length,chopper->routeLocation.distance,
                               chopper->target->movement->routeLocation.distance)<=0.0f;
}

int32_t chopperShiftTarget(Chopper* chopper,ChopperTarget* target)
{
    if (chopper->forceFirstPlayerTarget) {
        Car* first=chopper->environment.world->carTable[0];
        for (size_t index=0;index<chopper->environment.targetCount;++index)
            if (chopper->environment.targets[index].car==first) { target=&chopper->environment.targets[index];break; }
    }
    if (!chopper->environment.navigation || !carRecoveryConnectionsAvailable(chopper->environment.road)) return 0;
    if (!target) {
        if (chopper->phase==3 || chopper->phase==6) { chopper->target=NULL;chopper->phase=6; }
        else chopper->phase=5;
    } else if (chopper->phase==3 || chopper->phase==6) { chopper->target=target;chopper->phase=3; }
    else return 0;
    chopper->planningCooldown=0;return 1;
}

void chopperChooseAttackDeparture(Chopper* chopper,float distance)
{
    CarRecoverySelection choices[5];
    const CarRecoveryGroup* group=carDriverCurrentGroup(chopper->target->driver,1);
    const CarRecoveryPose* pose=carDriverCurrentPose(chopper->target->driver,1);
    const int32_t count=carRecoveryFindFollowingBranches(chopper->environment.road,choices,5,group,pose,distance+24.0f,1);
    chopper->targetDistance=distance;
    for (int32_t index=0;index<count;++index)
        if (!(choices[index].pose->flags&0x4000u)) { chopper->departure=choices[index];break; }
}

void chopperAdvanceAttackDeparture(Chopper* chopper)
{
    CarRecoverySelection departure=chopper->departure;
    for (;;) {
        uint32_t flags;
        if (!departure.pose || !aerialNavigationQuery(chopper->environment.navigation,departure.pose->position,NULL,&flags)) {
            chopper->result=WORLD_SERVICES_OWNER_FAILURE;return;
        }
        if (!(flags&1u)) break;
        chopper->targetDistance+=departure.pose->segmentLength;
        if (driverRoadNextPose(chopper->environment.road,&departure)!=CAR_DRIVER_OK) {
            chopper->result=WORLD_SERVICES_OWNER_FAILURE;return;
        }
    }
    chopper->departure=departure;
}

void chopperRefreshAttackDeparture(Chopper* chopper)
{
    float distance=chopper->environment.profiles[chopper->attackProfile].leadSeconds*chopper->target->movement->bindings.state->speed;
    if (distance<100.0f) distance=100.0f;
    if (chopper->targetDistance+24.0f<distance) {
        chopperChooseAttackDeparture(chopper,distance);chopperAdvanceAttackDeparture(chopper);
    }
}

int32_t chopperStartAttack(Chopper* chopper,int32_t profile)
{
    if (chopper->phase) { if (chopper->target) chopperShiftTarget(chopper,chopper->target);return 0; }
    chopper->dropDelay=0;chopper->attackSeconds=25.0f;
    const float* target=chopper->target->car->position;
    const float x=target[0]-chopper->position[0],y=target[1]-chopper->position[1];
    const float xx=x*x,yy=y*y;
    chopper->phase=4;chopper->attackProfile=profile;chopper->drops=0;chopper->targetDistance=sqrtf(xx+yy);
    if (chopperTargetBehind(chopper)) chopper->targetDistance=-chopper->targetDistance;
    chopper->departure=chopper->current;
    chopper->previousTarget.group=carDriverCurrentGroup(chopper->target->driver,1);
    chopper->previousTarget.pose=carDriverCurrentPose(chopper->target->driver,1);
    chopperRefreshAttackDeparture(chopper);return 1;
}

void chopperNotifyHorn(Chopper* chopper)
{
    if (chopper && chopper->forceFirstPlayerTarget) chopperStartAttack(chopper,1);
}


void chopperToLocal(const Chopper* chopper,float output[2],const float input[2])
{
    chopperRotate2(output,input,360.0f-chopper->yaw);
}

void chopperDestinationToLocal(const Chopper* chopper,float point[2])
{
    point[0]-=chopper->position[0];point[1]-=chopper->position[1];chopperToLocal(chopper,point,point);
}

void chopperChooseAcceleration(Chopper* chopper,const float desired[3])
{
    float acceleration;
    if (desired[2]>0.0f && desired[0]>0.0f) acceleration=(desired[2]-chopper->horizontalSpeed)*10.0f;
    else acceleration=chopperResponseScale(desired[0]-chopper->horizontalSpeed*0.1f);
    const float minimum=chopperMinimumAcceleration(chopper);
    if (acceleration<minimum) acceleration=minimum;
    else if (acceleration>20.0f) acceleration=20.0f;
    chopper->forwardAcceleration=acceleration;
}

int32_t chopperTryTurnInPlace(Chopper* chopper,const float desired[2])
{
    const float x=desired[0]*desired[0],y=desired[1]*desired[1];
    if (!(fabsf(chopper->horizontalSpeed)<=0.001f) || !(sqrtf(x+y)<5.0f) ||
            !(chopper->forwardAcceleration<0.1f)) return 0;
    chopper->movementMode=1;
    float targetHeading;
    if (!chopper->phase) targetHeading=(float)chopper->target->movement->bindings.state->heading*0.0054931640625f;
    else {
        const float* target=chopper->target->movement->bindings.state->position;
        targetHeading=(float)vectorAtan2Angle(target[0]-chopper->position[0],target[1]-chopper->position[1])*0.0054931640625f;
    }
    chopperChooseTurnRate(chopper,chopperWrapDegrees(chopper->yaw-(360.0f-targetHeading)),1.0f);return 1;
}

int32_t chopperNavigationProbe(Chopper* chopper,float forward,float lateral)
{
    const float local[2]={forward,lateral};float point[2];chopperRotate2(point,local,chopper->yaw);
    point[0]+=chopper->position[0];point[1]+=chopper->position[1];
    return aerialNavigationQuery(chopper->environment.navigation,point,NULL,NULL);
}

void chopperAvoidCorridorEdges(Chopper* chopper)
{
    const float forecast=(chopper->horizontalSpeed+chopper->forwardAcceleration*0.05f)*0.1f;
    const float yaw=chopper->yawRate*0.05f,correction=chopper->horizontalSpeed*0.29999998211860657f;
    if (!chopperNavigationProbe(chopper,forecast,yaw+4.5f)) chopper->yawRate-=correction;
    else if (!chopperNavigationProbe(chopper,forecast,yaw-4.5f)) chopper->yawRate+=correction;
    if (!chopperNavigationProbe(chopper,forecast+4.5f,yaw)) chopper->forwardAcceleration-=correction;
}



int32_t chopperRefreshGround(Chopper* chopper)
{
    const float point[4]={chopper->position[0],chopper->position[1],chopper->groundHeight,0};int32_t material;
    chopper->groundHeight=terrainSurfaceElevation(&chopper->terrain,point,&material,NULL,NULL);return material;
}

int32_t chopperMove(Chopper* chopper,float elapsed)
{
    int32_t attempts=0;
    if (chopper->movementMode==1) {
        chopperRotateYaw(chopper,chopper->turnRate*elapsed);
        if (!(fabsf(chopper->verticalVelocity)<=0.001f)) {
            chopper->position[2]+=chopper->verticalVelocity*elapsed;
            if (chopper->verticalVelocity<0.0f) {
                float height;
                if (aerialNavigationQuery(chopper->environment.navigation,chopper->position,&height,NULL) &&
                        chopper->position[2]<height) {
                    chopper->position[2]=height;chopper->verticalVelocity=0;
                }
            }
        }
    } else {
        const float acceleration=elapsed*chopper->forwardAcceleration;
        const float yawStep=elapsed*chopper->yawRate;
        chopper->horizontalSpeed+=acceleration;
        if (chopper->horizontalSpeed<0.0f) chopper->horizontalSpeed=0.0f;
        else if (chopper->horizontalSpeed>100.0f) chopper->horizontalSpeed=100.0f;
        chopper->desiredPitch=chopper->horizontalSpeed*0.15f;
        if (chopper->desiredPitch>20.0f) chopper->desiredPitch=20.0f;
        if (!(fabsf(yawStep)<=0.001f) && chopper->horizontalSpeed>1.0f) {
            float angle=(float)vectorAtan2Angle(chopper->horizontalSpeed,yawStep)*0.0054931640625f;
            if (angle>=180.0f) angle-=360.0f;
            chopperRotateYaw(chopper,angle*elapsed);
        }
        float relative[2]={chopper->current.pose->position[0]-chopper->position[0],
                           chopper->current.pose->position[1]-chopper->position[1]};
        chopperToLocal(chopper,relative,relative);
        const int32_t turnRight=relative[1]>0.0f;
        float point[2],delta[2],height;uint32_t flags;
        for (;;) {
            const float velocity[2]={chopper->horizontalSpeed,0};
            chopperRotate2(delta,velocity,chopper->yaw);
            delta[0]*=elapsed;delta[1]*=elapsed;
            point[0]=chopper->position[0]+delta[0];point[1]=chopper->position[1]+delta[1];
            if (aerialNavigationQuery(chopper->environment.navigation,point,&height,&flags)) break;
            if (++attempts==4) {
                point[0]=chopper->position[0];point[1]=chopper->position[1];
                height=chopper->position[2];flags=0;break;
            }
            chopper->planningCooldown=0;chopperRotateYaw(chopper,turnRight ? 5.0f : -5.0f);
        }
        chopper->position[0]=point[0];chopper->position[1]=point[1];
        if (!(fabsf(chopper->verticalVelocity)<=0.001f)) {
            chopper->position[2]+=chopper->verticalVelocity*elapsed;
            if (chopper->verticalVelocity<0.0f && chopper->position[2]<height) {
                chopper->position[2]=height;chopper->verticalVelocity=0;
            }
        } else chopper->position[2]=height;
        chopper->downwashEnabled=(int32_t)((flags^1u)&1u);
        terrainSurfaceUpdate(&chopper->terrain,chopper->position,1.0f,delta);
        chopperRefreshGround(chopper);
        trackRouteUpdateLocation(chopper->environment.route,&chopper->routeLocation,chopper->position);
    }
    chopperApproach(chopper->desiredPitch,&chopper->pitch,elapsed*20.0f);
    chopperApproach(chopper->desiredRoll,&chopper->roll,elapsed*20.0f);
    chopperBuildFrame(chopper);return attempts!=4;
}
