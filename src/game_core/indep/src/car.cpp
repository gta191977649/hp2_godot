#include "indep/inc/PhysicsCar.hpp"
#include "indep/inc/CopDriver.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TerrainType.hpp"
#include "indep/inc/CarDamage.hpp"
#include "CarSounds.hpp"
#include "bsound/include/bSound.h"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/CarMover.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/DriveTarget.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/PhysicsCar.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/EventManager.hpp"

#include "indep/inc/Scenery.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/RaceParameters.hpp"
#include "indep/inc/RealDriver.hpp"
#include "indep/inc/TrackedController.hpp"
#include "indep/inc/FlailerChunks.hpp"
#include "indep/inc/Texture.hpp"
#include "bware/inc/bWare.hpp"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <new>
#include <cmath>
#include <cstring>
#include <limits>

// PAL132960 uses the same one-alignment-bit ADD as the source math
// helpers. The following power-of-two scale is exact in its valid domain.


int32_t carCalculateDebugChecksum(const Car* car)
{
    const float sum = bScalarAdd(car->position[0], car->position[1]);
    const float scaled = sum * 65536.0f;
    return convertFloatToInt32(scaled);
}

CarFrameResult carFrameInteractionSetup(CarFrame* frame)
{
    if(!frame || !frame->movement || !frame->movement->bindings.state ||
       !frame->environment.clock)return CAR_FRAME_INVALID_BINDING;
    if(!frame->environment.spatial)return frame->result=CAR_FRAME_MISSING_SPATIAL_OWNER;
    const VehicleState& state=*frame->movement->bindings.state;
    float point[3];
    for(int axis=0;axis<3;++axis)point[axis]=state.velocity[axis]*0.15000000596046448f+state.centerPosition[axis];
    WorldEvent* const* events=worldEventsQuery(frame->environment.spatial,point);
    if(!events)return frame->result=CAR_FRAME_OK;
    for(size_t index=0;events[index];++index){
        WorldEvent* event=events[index];
        if(event->mask==0x10003u){
            if(!event->trigger || !frame->environment.fernShakers)
                return frame->result=CAR_FRAME_INVALID_BINDING;
            const auto result=fernShakersShake(frame->environment.fernShakers,event->trigger->value,state.velocity);
            if(result!=WORLD_SERVICES_OK)return frame->result=CAR_FRAME_INVALID_BINDING;
            if(frame->environment.player &&
               ((static_cast<uint32_t>(frame->environment.clock->worldLoopCounter)+3u)&3u)==0u && state.speed>30.0f){
                if(!frame->environment.replayManager)return frame->result=CAR_FRAME_INVALID_BINDING;
                replayManagerAddHighlight(frame->environment.replayManager,2.0f);
            }
        }else if(event->mask==0x10005u){
            CarFramePlayerState* player=frame->environment.player;
            if(!player)continue;
            if(!event->trigger || !frame->environment.settings)return frame->result=CAR_FRAME_INVALID_BINDING;
            const uint32_t value=event->trigger->value;
            const int32_t reverse=frame->environment.settings->reverse;
            const bool applies=(reverse==0 && (value&0xffu)==0u) || (reverse==1 && (value&0xffu)!=0u);
            if(applies && state.speed>40.0f && !player->wrongWay &&
               !carFramePlayerTriggerVisibleDrivingLine(player,value>>16,static_cast<float>((value>>8)&0xffu)))
                return frame->result=CAR_FRAME_PLAYER_TRANSITION_FAILURE;
        }else worldEventsDelete(frame->environment.spatial,event);
    }
    return frame->result=CAR_FRAME_OK;
}

void carMovementSetGripOverride(CarMovement* movement, float value)
{
    movement->frictionBoost = value;
    if (movement->mode == 1) movement->physics->gripScale = value;
}

void driverCollisionBounds(const DriverPhysicsVehicle* vehicle,
                            float minimum[4], float maximum[4])
{
    const CollisionShape* shape = vehicle->movement->bindings.ownerMotion->collision->shape;
    for (int lane = 0; lane < 3; ++lane) {
        minimum[lane] = shape->localMinimum[lane];
        maximum[lane] = shape->localMaximum[lane];
    }
    minimum[3] = 0.0f;
    maximum[3] = 0.0f;
}

void carControlsSetGearRequest(CarControls* c, int32_t value) { c->gearRequest = value; }
void carControlsSetShiftTarget(CarControls* c, int32_t value) { c->shiftTarget = value; }
void carControlsSetNitroRequest(CarControls* c, uint32_t value) { c->nitroRequest = value; }
void carControlsSetSteeringAngle(CarControls* c, uint16_t value) { c->steeringAngle = value; }
void carControlsSetThrottle(CarControls* c, float value) { c->throttle = value; }
void carControlsSetBrake(CarControls* c, float value) { c->brake = value; }
void carControlsSetHandbrake(CarControls* c, float value) { c->handbrake = value; }
int32_t carControlsGetGearRequest(const CarControls* c) { return c->gearRequest; }
int32_t carControlsGetShiftTarget(const CarControls* c) { return c->shiftTarget; }
uint32_t carControlsGetNitroRequest(const CarControls* c) { return c->nitroRequest; }
uint16_t carControlsGetSteeringAngle(const CarControls* c) { return c->steeringAngle; }
float carControlsGetThrottle(const CarControls* c) { return c->throttle; }
float carControlsGetBrake(const CarControls* c) { return c->brake; }
float carControlsGetHandbrake(const CarControls* c) { return c->handbrake; }

float physicsMaximumPairedAverage(const float values[4])
{
    const float first = (values[0] + values[3]) * 0.5f;
    const float second = (values[1] + values[2]) * 0.5f;
    return first < second ? second : first;
}

float physicsQuarterOfFourFlags(const uint8_t values[4])
{
    float count = 0.0f;
    for (unsigned i = 0; i < 4; ++i)
        if (values[i]) count += 0.25f;
    return count;
}

CarFrameResult carFrameUpdateBlink(CarFrame* frame)
{
    if (!frame) return CAR_FRAME_INVALID_BINDING;
    if (frame->movement->bindings.player) return CAR_FRAME_OK;
    float elapsed = (float)(int32_t)((uint32_t)frame->environment.clock->worldTimer -
                                     (uint32_t)frame->state.blinkStart) * 0.0002500000118743628f;
    int32_t active;
    if (elapsed < frame->state.blinkInterval) active = 1;
    else {
        elapsed = elapsed - frame->state.blinkInterval;
        if (elapsed < frame->state.blinkInterval) active = !frame->state.blinkPhase;
        else active = elapsed - frame->state.blinkOffset < frame->state.blinkDuration;
    }
    frame->state.blinkVisible = (active && frame->state.blinkVisualEnabled) || frame->state.blinkForced;
    if (!frame->environment.driver) return CAR_FRAME_INVALID_BINDING;
    const int32_t hornActive = carDriverHornActive(frame->environment.driver);
    if ((active && frame->state.blinkControlEnabled && !hornActive) ||
        (!(active && frame->state.blinkControlEnabled) && hornActive)) {
        if (carDriverSetHorn(frame->environment.driver, active && frame->state.blinkControlEnabled) != CAR_DRIVER_OK)
            return CAR_FRAME_EVENT_CAPACITY;
    }
    return CAR_FRAME_OK;
}

static void carFrameAddHighlight(CarFrame* frame, CarFrameDrivingStat kind, float value)
{
    replayManagerAddHighlight(frame->environment.replayManager, value);
    if (frame->environment.presentation) {
        CarFramePresentationEvent* event = carFramePresentationAppend(frame->environment.presentation,
            CAR_FRAME_DRIVING_STAT, frame->movement, kind);
        if (event) event->value[0] = value;
        else frame->result = CAR_FRAME_EVENT_CAPACITY;
    }
}

static int32_t carFrameCountNearbyCars(const World* world, int32_t first, int32_t end, const float point[4])
{
    int32_t count = 0;
    for (int32_t index = first; index < end; ++index) {
        const float* other = world->carTable[index]->position;
        const float x = other[0] - point[0];
        const float y = other[1] - point[1];
        const float z = other[2] - point[2];
        if (sqrtf((x * x + y * y) + z * z) < 40.0f) ++count;
    }
    return count;
}

CarFrameResult carFramePostStep(CarFrame* frame, float elapsed)
{
    if (!frame) return CAR_FRAME_INVALID_BINDING;
    frame->result = CAR_FRAME_OK;
    CarMovement* movement = frame->movement;
    VehicleState* state = movement->bindings.state;
    if (0.0f < elapsed) {
        const float reciprocal = finiteRatio(1.0f, elapsed);
        for (int lane = 0; lane < 4; ++lane) {
            const float linear = state->velocity[lane] - frame->state.previousVelocity[lane];
            const float angular = state->angularVelocity[lane] - frame->state.previousAngularVelocity[lane];
            state->linearAcceleration[lane] = linear * reciprocal;
            state->angularAcceleration[lane] = angular * reciprocal;
        }
    }
    if (!(state->speed < 0.0f) && !(1000.0f < state->speed))
        state->speedDistance = state->speedDistance + state->speed * elapsed;
    const float x = frame->state.previousPosition[0] - state->position[0];
    const float y = frame->state.previousPosition[1] - state->position[1];
    const float z = frame->state.previousPosition[2] - state->position[2];
    state->displacementDistance = state->displacementDistance + sqrtf((x * x + y * y) + z * z);
    memcpy(frame->state.previousPosition, state->position, sizeof(float) * 4);
    if (!movement->bindings.route) return frame->result = CAR_FRAME_INVALID_BINDING;
    trackRouteUpdateLocation(movement->bindings.route, &movement->routeLocation, state->position);
    const int32_t loop = frame->environment.clock->worldLoopCounter;
    TerrainSurface* terrain = movement->bindings.environment->contact.terrain;
    if ((((uint32_t)loop + 7u) & 31u) == 0 && movement->mode != 0 &&
        !terrainSurfaceHasGround(terrain, state->position)) {
        const float adjacent[2] = {state->position[0] + 1.0f, state->position[1]};
        if (!terrainSurfaceHasGround(terrain, adjacent)) movement->bindings.ownerMotion->recoveryRequested = 1;
    }
    float routeDistance = 0.0f;
    if (movement->routeLocation.node) {
        const float dx = movement->routeLocation.point[0] - movement->routeLocation.node->point[0];
        const float dy = movement->routeLocation.point[1] - movement->routeLocation.node->point[1];
        routeDistance = sqrtf(dx * dx + dy * dy);
    }
    if (5000.0f < routeDistance) {
        const CarCollisionState* collision = movement->bindings.ownerMotion->collision;
        if (!collision || !collision->policy) return frame->result = CAR_FRAME_INVALID_BINDING;
        if (!collision->policy->collisionDisabled) movement->bindings.ownerMotion->recoveryRequested = 1;
    } else if (300.0f < routeDistance && loop % 60 == 0 && movement->mode != 0) {
        trackRouteLocationAtPoint(movement->bindings.route, &movement->routeLocation, state->position);
    }
    frame->result = carFrameUpdateBlink(frame);
    if (frame->result != CAR_FRAME_OK || !movement->bindings.player) return frame->result;
    CarFramePlayerState* player = frame->environment.player;
    if (!player || &player->movement != movement->bindings.player || player->car != movement ||
        !frame->environment.replayManager) return frame->result = CAR_FRAME_MISSING_PLAYER_OWNER;
    int32_t mode = player->movement.steeringFeedbackBusy;
    const int32_t contacts = state->activeContactCount;
    if ((mode == 6 || mode == 7) && contacts > 0) carFramePlayerEndState(player, 1);
    if ((((uint32_t)loop + 3u) & 7u) != 0) return frame->result;
    const float speed = state->speed;
    if (40.0f < speed && contacts < 4 && fabsf(vehicleStateMovingHeadingDifference(state)) < 0.09812500327825546f)
        carFrameAddHighlight(frame, CAR_FRAME_STAT_IN_AIR, 2.0f);
    mode = player->movement.steeringFeedbackBusy;
    if (!player->registry->airbornePredictionDisabled && mode != 6 && mode != 7 && contacts == 0 &&
        !player->wrongWay && 30.0f < speed) {
        float duration = 3.0f;
        float clearance;
        (void)carFramePredictGroundClearance(frame, &duration, &clearance);
        const float now = (float)frame->environment.clock->worldTimer * 0.0002500000118743628f;
        frame->state.airborneDeadline = duration < 0.9f ? now + 5.0f : duration * 0.5f + now;
        if (1.5f < clearance && 0.5f < duration && carFramePlayerPrepareAirborne(player)) {
            if (!carFramePlayerStartState(player, 6, 0, 1, 1)) carFramePlayerClearPreview(player);
        }
    }
    mode = player->movement.steeringFeedbackBusy;
    if (mode == 6) {
        if (state->velocity[2] < 0.0f && player->airborneCameraHeight < state->position[2])
            player->airborneCameraHeight = state->position[2];
        const float now = (float)frame->environment.clock->worldTimer * 0.0002500000118743628f;
        if (frame->state.airborneDeadline < now) {
            carFramePlayerEndState(player, 0);
            (void)carFramePlayerStartState(player, 7, 0, 0, 1);
        }
    }
    if (speed < 5.0f && !player->drivingLocked) carFrameAddHighlight(frame, CAR_FRAME_STAT_GOING_SLOW, -10.0f);
    if (40.0f < speed) {
        const int32_t cars = carFrameCountNearbyCars(frame->environment.world, frame->environment.world->firstGroupEnd,
                                         frame->environment.world->secondGroupEnd, state->position);
        if (cars > 0) carFrameAddHighlight(frame, CAR_FRAME_STAT_CLOSE_CARS, (float)cars * 0.5f);
        const int32_t cops = carFrameCountNearbyCars(frame->environment.world, frame->environment.world->secondGroupEnd,
                                         frame->environment.world->thirdGroupEnd, state->position);
        if (cops > 0) carFrameAddHighlight(frame, CAR_FRAME_STAT_CLOSE_COPS, (float)cops * 0.05f);
    }
    carFrameAddHighlight(frame, CAR_FRAME_STAT_SPEED, speed * 0.006666666828095913f);
    if (30.0f < speed) {
        if (0.39250001311302185f < fabsf(vehicleStateMovingHeadingDifference(state)))
            carFrameAddHighlight(frame, CAR_FRAME_STAT_SLIDING_SIDEWAYS, 1.0f);
        const CollisionImpact* impact = movement->collisionSystem && movement->collisionParticipant
            ? collisionShapeContactAt(movement->collisionSystem, movement->collisionParticipant->shape, 0) : NULL;
        if (impact) {
            const int32_t playerContact = collisionImpactInvolvesPlayer(impact);
            const float age = (float)(int32_t)((uint32_t)frame->environment.clock->worldTimer -
                                               (uint32_t)impact->createdTicks) * 0.0002500000118743628f;
            if (playerContact && age < 0.5f) {
                if (collisionImpactIsCarPair(impact) && 40.0f < speed)
                    carFrameAddHighlight(frame, CAR_FRAME_STAT_CAR_COLLISION, 3.0f);
                else if (collisionImpactInvolvesSmokeable(impact))
                    carFrameAddHighlight(frame, CAR_FRAME_STAT_SMOKEABLE_COLLISION, 2.5f);
            }
        }
    }
    if (player->result) frame->result = CAR_FRAME_PLAYER_TRANSITION_FAILURE;
    return frame->result;
}




static float unitClamp(float value)
{
    if (value < 0.0f) value = 0.0f;
    if (1.0f < value) value = 1.0f;
    return value;
}

static float rangeStrength(float value, const TerrainMaterialEffect* range)
{
    const float numerator = value - range->start;
    const float denominator = range->full - range->start;
    return unitClamp(finiteRatio(numerator, denominator));
}

static CarFrameResult effect(CarFrame* frame, int32_t type, int32_t key,
                              float strength, const float* velocity)
{
    const CarEffectResult result=carTriggerEffect(frame->movement,type,strength,key,velocity);
    if(result!=CAR_EFFECT_OK)return frame->result=result==CAR_EFFECT_INVALID_BINDING ?
        CAR_FRAME_INVALID_BINDING:CAR_FRAME_PARTICLE_FAILURE;
    // This record is diagnostic. CPU emission and RNG advance synchronously
    // even when the presentation observer has no remaining storage.
    CarFramePresentation* presentation=frame->environment.presentation;
    if(!presentation || !presentation->events || presentation->count>=presentation->capacity)
        return CAR_FRAME_OK;
    CarFramePresentationEvent* request = carFramePresentationAppend(frame->environment.presentation,
        CAR_FRAME_SURFACE_EFFECT, frame->movement, type);
    if (!request) return CAR_FRAME_OK;
    request->argument = key;
    request->value[0] = strength;
    if (velocity) {
        memcpy(request->position, velocity, sizeof(request->position));
        request->hasPosition = 1;
    }
    return CAR_FRAME_OK;
}

void carFrameExtendSignalWindow(CarFrame* frame, int32_t start, int32_t end)
{
    CarFrameState* state = &frame->state;
    if (state->signalWindowLocked) return;
    const int32_t now = frame->environment.clock->worldTimeFrames;
    if (state->signalWindowStart < now) {
        if (!(now < state->signalWindowEnd)) state->signalWindowStart = start;
    } else if (start < state->signalWindowStart) {
        state->signalWindowStart = start;
    }
    if (state->signalWindowEnd < end) state->signalWindowEnd = end;
    const int32_t distance = (int32_t)((uint32_t)state->signalWindowEnd -
        (uint32_t)state->signalWindowStart + 81u);
    const int32_t intervals = distance / 82;
    state->signalWindowEnd = (int32_t)((uint32_t)state->signalWindowStart +
        (uint32_t)intervals * 82u);
}

CarFrameResult carFrameUpdateSurfaceEffects(CarFrame* frame, float nearestPlayerDistance)
{
    if (!frame || !frame->movement || !frame->environment.clock)
        return CAR_FRAME_INVALID_BINDING;
    CarMovement* movement = frame->movement;
    VehicleState* vehicle = movement->bindings.state;
    const float speedMph = vehicle->speed * 2.236999988555908f;
    int32_t wetMask = 0;
    int32_t deepWater = 0;
    int32_t contacts = 0;
    frame->result = CAR_FRAME_OK;
    for (int32_t wheel = 0; wheel < 4; ++wheel) {
        if (!vehicle->wheelContact[wheel]) continue;
        ++contacts;
        const int32_t material = vehicle->material[wheel];
        if (material < 0 || material >= 33) return frame->result = CAR_FRAME_INVALID_BINDING;
        const TerrainMaterial* response = &terrainDefaultMaterials(NULL)[material];
        const int32_t key = (wheel + 5) * 65536 + material;
        if (material == 4 || material == 5 || material == 20) {
            wetMask |= 1 << wheel;
            deepWater |= material == 5;
        }
        const float slipX = vehicle->wheelSlip[wheel][0];
        const float slipY = vehicle->wheelSlip[wheel][1];
        const float slip = sqrtf(slipX * slipX + slipY * slipY);
        if (nearestPlayerDistance < 150.0f && response->effects[2].start < slip) {
            uint32_t count = (uint32_t)frame->state.wheelSlipFrames[wheel] + 1u;
            if (count > 255u) count = 255u;
            frame->state.wheelSlipFrames[wheel] = (uint8_t)count;
            if (response->effects[2].name[0]) {
                float strength = finiteRatio(slip - response->effects[2].start,
                                               response->effects[2].full - response->effects[2].start);
                if (count < 2u) strength = 0.0f;
                else if (count < 20u) strength = strength * 0.25f;
                else if (count < 60u) strength = strength * 0.5f;
                else if (count < 120u) strength = strength * 0.75f;
                strength = unitClamp(strength);
                if (0.0f < strength && effect(frame, 3, key, strength, NULL) != CAR_FRAME_OK)
                    return frame->result;
            }
        } else {
            frame->state.wheelSlipFrames[wheel] = 0;
        }
        if (nearestPlayerDistance < 40.0f && wheel >= 2 &&
            response->effects[3].start < slipX && response->effects[3].name[0]) {
            const float strength = rangeStrength(slipX, &response->effects[3]);
            if (0.0f < strength) {
                const float local[4] = {slipX, slipY, 0.0f, 0.0f};
                float position[4];
                vectorMatrixTransformPoint(position, vehicle->wheelVisualTransform[wheel], local);
                for (int lane = 0; lane < 4; ++lane)
                    position[lane] = position[lane] + vehicle->velocity[lane];
                if (effect(frame, 4, key, strength, position) != CAR_FRAME_OK) return frame->result;
            }
        }
    }
    if (contacts >= 3) {
        int32_t material = 0;
        (void)terrainSurfaceElevation(movement->bindings.environment->contact.terrain,
                                      vehicle->position, &material, NULL, NULL);
        if (material < 0 || material >= 33) return frame->result = CAR_FRAME_INVALID_BINDING;
        const TerrainMaterial* response = &terrainDefaultMaterials(NULL)[material];
        float first = response->effects[0].name[0] ? rangeStrength(speedMph, &response->effects[0]) : 0.0f;
        float second = response->effects[1].name[0] ? rangeStrength(speedMph, &response->effects[1]) : 0.0f;
        const float excess = (first + second) - 1.0f;
        if (0.0f < first + second) {
            if (0.0f < excess) {
                const float correction = excess + excess;
                if (response->effects[0].start < response->effects[1].start) {
                    first = first - correction;
                    if (first < 0.0f) first = 0.0f;
                } else {
                    second = second - correction;
                    if (second < 0.0f) second = 0.0f;
                }
            }
            if (0.0f < first && effect(frame, 1, material, first, NULL) != CAR_FRAME_OK)
                return frame->result;
            if (0.0f < second && effect(frame, 2, material, second, NULL) != CAR_FRAME_OK)
                return frame->result;
        }
        float impulse = 0.0f;
        for (int wheel = 0; wheel < 4; ++wheel) impulse = impulse + vehicle->suspensionImpulse[wheel];
        const float strength = rangeStrength(impulse, &response->effects[4]);
        if (0.0f < strength && effect(frame, 5, material, strength, NULL) != CAR_FRAME_OK)
            return frame->result;
    }
    if (nearestPlayerDistance < 60.0f && wetMask > 0 && 10.0f < vehicle->speed) {
        const int32_t now = frame->environment.clock->worldTimeFrames;
        carFrameExtendSignalWindow(frame, (int32_t)((uint32_t)now + 60u),
                                   (int32_t)((uint32_t)now + 180u));
        if (frame->environment.sound)
            carSoundSetSurfaceLevel(frame->environment.sound, (wetMask & 6) != 0, deepWater ? 3 : 1);
        if (movement->bindings.player &&
            (((uint32_t)frame->environment.clock->worldLoopCounter + 3u) & 3u) == 0 &&
            30.0f < vehicle->speed) {
            if (deepWater) {
                if (!frame->state.looseSurfaceHighlight) {
                    replayManagerAddHighlight(frame->environment.replayManager, 10.0f);
                    frame->state.looseSurfaceHighlight = 1;
                }
            } else {
                replayManagerAddHighlight(frame->environment.replayManager, 1.25f);
            }
        }
    }
    if (!deepWater) frame->state.looseSurfaceHighlight = 0;
    if (movement->bindings.damage) {
        const float total = bodyDamageVisibleTotal(&movement->bindings.damage->body);
        if (0.8999999761581421f < total) {
            if (effect(frame, 7, 0, 1.0f, NULL) != CAR_FRAME_OK) return frame->result;
        } else if (carDamageItemLevel(movement->bindings.damage->levels, 14) == 3 &&
                   0.6499999761581421f < total) {
            if (effect(frame, 6, 0, 1.0f, NULL) != CAR_FRAME_OK) return frame->result;
        }
    }
    int32_t nitro = 0;
    if (movement->bindings.player && movement->mode == 1) {
        if (!movement->physics || !movement->physics->engine)
            return frame->result = CAR_FRAME_INVALID_BINDING;
        nitro = engineGetNitroEnabled(movement->physics->engine) != 0;
    }
    if ((frame->state.specialEffectRequested || nitro) &&
        effect(frame, 10, 0, 1.0f, NULL) != CAR_FRAME_OK) return frame->result;
    if (frame->state.disabledWheelCount) {
        for (int wheel = 0; wheel < 4; ++wheel) {
            if (!vehicle->wheelContact[wheel] ||
                !((movement->bindings.ownerMotion->disabledWheelMask >> wheel) & 1u)) continue;
            const float strength = unitClamp(((vehicle->speed * 2.236999988555908f) -
                4.470272541046143f) * 0.04473999887704849f);
            if (effect(frame, 9, (wheel + 5) * 65536, strength, NULL) != CAR_FRAME_OK)
                return frame->result;
        }
    }
    return frame->result;
}



static CarFrameResult forceRequest(CarFrame* frame, CarFrameForceFeedbackOperation operation,
                                    int32_t type, int32_t port, const float* values, int32_t count)
{
    CarFramePresentationEvent* event = carFramePresentationAppend(frame->environment.presentation,
        CAR_FRAME_FORCE_FEEDBACK, frame->movement, operation);
    if (!event) return frame->result = CAR_FRAME_EVENT_CAPACITY;
    event->argument = type;
    event->port = port;
    memcpy(event->forceParameters, values, (size_t)count * sizeof(*values));
    event->parameterCount = count;
    return CAR_FRAME_OK;
}

static CarFrameResult pair(CarFrame* frame, int32_t type, int32_t port,
                            float first, float second, float third, float fourth)
{
    const float values[6] = {first, second, third, fourth, 1.0f, 1.0f};
    return forceRequest(frame, CAR_FRAME_FORCE_PAIR_PARAMETERS, type, port, values, 6);
}

static CarFrameResult periodic(CarFrame* frame, int32_t type, int32_t port,
                                float strength, float direction, float frequency)
{
    const float values[9] = {strength, direction, frequency, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    return forceRequest(frame, CAR_FRAME_FORCE_PERIODIC_PARAMETERS, type, port, values, 9);
}

CarFrameResult carFrameSetupForceFeedback(CarFrame* frame)
{
    if (!frame) return CAR_FRAME_INVALID_BINDING;
    if (!frame->movement->bindings.player) return CAR_FRAME_OK;
    return carFrameUpdateForceFeedback(frame, frame->movement->bindings.player->seat);
}

CarFrameResult carFrameUpdateForceFeedback(CarFrame* frame, int32_t port)
{
    if (!frame || !frame->movement || !frame->environment.clock)
        return CAR_FRAME_INVALID_BINDING;
    CarMovement* movement = frame->movement;
    const CarMovementPlayer* player = movement->bindings.player;
    frame->result = CAR_FRAME_OK;
    int32_t active = 0;
    if (player) {
        const CarFramePlayerState* owner = frame->environment.player;
        if (!movement->bindings.applicationState || !owner || &owner->movement != player ||
            !owner->registry || !owner->registry->raceKind)
            return frame->result = CAR_FRAME_INVALID_BINDING;
        const int32_t kind = *owner->registry->raceKind;
        active = *movement->bindings.applicationState != 3 && kind != 4 && kind != 7 &&
                 kind != 5 && kind != 9 && kind != 6 && player->steeringFeedbackEnabled;
    }
    if (!active) {
        if (pair(frame, 0, port, 0.0f, 0.0f, 1.5f, 1.5f) != CAR_FRAME_OK ||
            pair(frame, 1, port, 0.0f, 0.0f, 0.0f, 0.0f) != CAR_FRAME_OK ||
            periodic(frame, 4, port, 0.0f, 0.0f, 0.0f) != CAR_FRAME_OK ||
            periodic(frame, 3, port, 0.0f, 0.0f, 0.0f) != CAR_FRAME_OK) return frame->result;
        const float gain = 1.0f;
        return forceRequest(frame, CAR_FRAME_FORCE_GAIN, 0, port, &gain, 1);
    }
    if (!movement->bindings.controls) return frame->result = CAR_FRAME_INVALID_BINDING;
    const VehicleState* vehicle = movement->bindings.state;
    float angle = (float)movement->bindings.controls->steeringAngle * 0.0054931640625f;
    angle = angle < 180.0f ? -angle : 360.0f - angle;
    const float steeringForce = angle * 0.02500000037252903f;
    const float x = vehicle->velocity[0] * vehicle->velocity[0];
    const float y = vehicle->velocity[1] * vehicle->velocity[1];
    const float z = vehicle->velocity[2] * vehicle->velocity[2];
    float speed = sqrtf((x + y) + z);
    if (speed < 5.0f) speed = 0.0f;
    speed = speed - 5.0f;
    if (speed < 0.0f) speed = 0.0f;
    if (30.0f < speed) speed = 30.0f;
    speed = speed * 0.03333333507180214f;
    const float damping = 0.05000000074505806f - speed * 0.05000000074505806f;
    float first = speed;
    float second = speed;
    const float slip = physicsMaximumPairedAverage(vehicle->wheelSlipRatio);
    if (slip < 0.05000000074505806f) { first = 0.0f; second = 0.0f; }
    if (0.9200000166893005f < slip && 0.8999999761581421f < speed) {
        float scale = (slip - 0.9200000166893005f) * 12.50000286102295f;
        if (1.0f < scale) scale = 1.0f;
        const float reduction = scale * 0.4000000059604645f;
        first = first - reduction;
        second = second - reduction;
    }
    float impulse = 0.0f;
    for (int wheel = 0; wheel < 4; ++wheel) impulse = impulse + vehicle->suspensionImpulse[wheel];
    if (0.0f < impulse) {
        const float strength = impulse * 0.25f;
        if (forceRequest(frame, CAR_FRAME_FORCE_IMPACT, 0, port, &strength, 1) != CAR_FRAME_OK)
            return frame->result;
        first = 1.0f;
        second = 1.0f;
        const float seconds = (float)frame->environment.clock->worldTimer * 0.0002500000118743628f;
        frame->state.feedbackDeadline = convertFloatToInt32(((seconds + 1.0f) * 4000.0f) + 0.5f);
    }
    if (pair(frame, 0, port, 0.0f, damping, first * speed, second * speed) != CAR_FRAME_OK ||
        pair(frame, 1, port, 0.0f, speed, 1.0f - speed, 1.0f - speed) != CAR_FRAME_OK)
        return frame->result;
    const float gain = physicsQuarterOfFourFlags((const uint8_t*)vehicle->wheelContact);
    if (forceRequest(frame, CAR_FRAME_FORCE_GAIN, 0, port, &gain, 1) != CAR_FRAME_OK)
        return frame->result;
    const int32_t state = player->steeringFeedbackBusy;
    if (state && state != 6 && state != 7) {
        if (periodic(frame, 4, port, 0.0f, 0.0f, 0.0f) != CAR_FRAME_OK)
            return frame->result;
        return periodic(frame, 3, port, 0.0f, 0.0f, 0.0f);
    }
    float force = physicsMaximumPairedAverage(vehicle->wheelSlipRatio) - 0.8999999761581421f;
    if (force < 0.0f) force = 0.0f;
    force = force * 50.0f;
    if (1.0f < force) force = 1.0f;
    if (periodic(frame, 4, port, speed * force, -steeringForce, 100.0f) != CAR_FRAME_OK)
        return frame->result;
    int32_t wet = 0;
    float amplitude = 0.0f;
    float frequency = 0.0f;
    for (int wheel = 0; wheel < 4; ++wheel) {
        const int32_t material = vehicle->material[wheel];
        if (material < 0 || material >= 33) return frame->result = CAR_FRAME_INVALID_BINDING;
        if (material == 4 || material == 5 || material == 20) ++wet;
        frequency = frequency + terrainDefaultMaterials(NULL)[material].parameter34;
        amplitude = amplitude + terrainDefaultMaterials(NULL)[material].parameter30;
    }
    const float strength = speed * ((amplitude * 0.25f) * 0.5f);
    if (periodic(frame, 3, port, strength, 90.0f, frequency * 0.25f) != CAR_FRAME_OK)
        return frame->result;
    if (wet >= 2) {
        if (pair(frame, 0, port, 0.0f, 0.0f, 1.5f, 1.5f) != CAR_FRAME_OK)
            return frame->result;
        return pair(frame, 1, port, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    return frame->result;
}

int32_t carSetPursuitResponse(Car* car, float damageResponseScale, float physicsBoost)
{
    if (!car) return 0;
    car->pursuitDamageResponseScale=damageResponseScale;
    car->physicsBoost=physicsBoost;
    return 1;
}


static void advanceOwner(void* owner, float elapsed)
{ (void)carMovementCapture((CarMovement*)owner, elapsed); }

static void detachMover(CarMovement* movement)
{
    if (movement->physics) {
        physicsCarDispose(movement->physics);
        free(movement->physics);
        movement->physics = NULL;
    }
    if (movement->mellow) {
        mellowMoverDispose(movement->mellow);
        (void)physicsObjectsReleaseMellowMover(movement->bindings.pools, movement->mellow);
        movement->mellow = NULL;
    }
    if (movement->inactive) {
        (void)physicsObjectsReleaseNullMover(movement->bindings.pools, movement->inactive);
        movement->inactive = NULL;
    }
    free(movement->traffic); movement->traffic = NULL;
    free(movement->delivery); movement->delivery = NULL;
    free(movement->cop); movement->cop = NULL;
    if (movement->belt) {
        beltMoverDispose(movement->belt);free(movement->belt);movement->belt = NULL;
    }
}

CarMovementResult carMovementSetMode(CarMovement* movement, int32_t mode)
{
    if (mode < 0 || mode > 6)
        return movement->result = CAR_MOVEMENT_UNSUPPORTED_MODE;
    if (mode == 5 && movement->info.modelCode != 4 &&
        (movement->info.modelCode < 31 || movement->info.modelCode > 34))
        return movement->result = CAR_MOVEMENT_INVALID_BINDING;
    if (mode == 6) {
        const BeltMoverBindings* belt = movement->bindings.belt;
        if (!belt || !belt->command || !belt->vehicle || !belt->vehicle->performance || !belt->targetSpeed ||
            !belt->targetDuration || !belt->replanDelayTicks || !belt->world)
            return movement->result = CAR_MOVEMENT_INVALID_BINDING;
    }
    detachMover(movement);
    movement->mode = mode;
    PhysicsPoolStatus status = PHYSICS_POOL_OK;
    const CarMovementBindings* binding = &movement->bindings;
    if (mode == 0) {
        movement->inactive = physicsObjectsAcquireNullMover(binding->pools, &status);
        if (movement->inactive) nullMoverInitialize(movement->inactive, binding->state);
    } else if (mode == 1) {
        movement->physics = (PhysicsCar*)malloc(sizeof(*movement->physics));
        if (movement->physics) {
            status = physicsCarInitialize(movement->physics, &movement->info, binding->controls, binding->owner,
                                           binding->ownerMotion, binding->state, binding->environment, binding->pools);
            if (status != PHYSICS_POOL_OK) { free(movement->physics); movement->physics = NULL; }
        }
    } else if (mode == 2) {
        movement->traffic = (TrafficCar*)malloc(sizeof(*movement->traffic));
        if (movement->traffic) trafficCarInitialize(movement->traffic,movement);
    } else if (mode == 3) {
        movement->delivery = (DeliveryMover*)malloc(sizeof(*movement->delivery));
        if (movement->delivery) deliveryMoverInitialize(movement->delivery,movement);
    } else if (mode == 5) {
        movement->cop = (CopMover*)malloc(sizeof(*movement->cop));
        if (movement->cop && !copMoverInitialize(movement->cop,movement)) {
            free(movement->cop); movement->cop = NULL;
            return movement->result = CAR_MOVEMENT_INVALID_BINDING;
        }
    } else if (mode == 6) {
        movement->belt = (BeltMover*)malloc(sizeof(*movement->belt));
        if (movement->belt && !beltMoverInitialize(movement->belt,movement)) {
            free(movement->belt);movement->belt = NULL;
        }
    } else {
        movement->mellow = physicsObjectsAcquireMellowMover(binding->pools, &status);
        if (movement->mellow) {
            status = mellowMoverInitialize(movement->mellow, &movement->info, binding->controls, binding->owner,
                                            binding->ownerMotion, binding->state, binding->environment, binding->pools);
            if (status != PHYSICS_POOL_OK) {
                (void)physicsObjectsReleaseMellowMover(binding->pools, movement->mellow);
                movement->mellow = NULL;
            }
        }
    }
    CollisionShape* shape = binding->ownerMotion->collision->shape;
    shape->bodyMode = mode == 1 || mode == 4;
    shape->body = carMovementCollisionBody(movement);
    shape->prepareOwner = mode == 1 && movement->physics ? (void*)&movement->physics->components :
                          mode == 4 && movement->mellow ? (void*)&movement->mellow->components : NULL;
    shape->vehicle.position = binding->state->position;
    shape->vehicle.velocity = binding->state->velocity;
    shape->vehicle.orientation = binding->state->orientation;
    binding->owner->movement = movement;
    binding->owner->move = advanceOwner;
    if ((mode == 0 && !movement->inactive) || (mode == 1 && !movement->physics) ||
        (mode == 2 && !movement->traffic) || (mode == 3 && !movement->delivery) ||
        (mode == 4 && !movement->mellow) || (mode == 5 && !movement->cop) || (mode == 6 && !movement->belt))
        return movement->result = CAR_MOVEMENT_ALLOCATION_FAILURE;
    if (mode == 1) movement->physics->gripScale = movement->frictionBoost;
    return movement->result = CAR_MOVEMENT_OK;
}

CarMovementResult carMovementInitialize(CarMovement* movement, const CarMovementBindings* binding, int32_t mode)
{
    if (!movement || !binding || !binding->info || !binding->controls || !binding->owner ||
        !binding->ownerMotion || !binding->ownerMotion->collision || !binding->ownerMotion->collision->shape ||
        !binding->ownerMotion->collision->shape->geometry || !binding->state || !binding->environment || !binding->pools)
        return CAR_MOVEMENT_INVALID_BINDING;
    if (binding->driver && binding->driver->profile != 0 && binding->driver->profile != 1 && binding->driver->profile != 4)
        return CAR_MOVEMENT_UNSUPPORTED_DRIVER;
    if (binding->driver && binding->driver->profile != 0 && !binding->recoveryPoints)
        return CAR_MOVEMENT_MISSING_RECOVERY_DATA;
    memset(movement, 0, sizeof(*movement));
    movement->info = *binding->info;
    movement->bindings = *binding;
    movement->bindings.info = &movement->info;
    binding->ownerMotion->owner = binding->owner;
    binding->ownerMotion->damage = binding->damage;
    binding->state->position = binding->owner->position;
    binding->state->centerOfMass = movement->info.centerOfMass;
    binding->state->wheelInfo = movement->info.wheels;
    memcpy(binding->state->wheelSideOffset, movement->info.wheelSideOffset, sizeof(binding->state->wheelSideOffset));
    movement->frictionBoost = 1.0f;
    movement->publishedGear = binding->state->gear;
    trackRouteLocationInitialize(&movement->routeLocation);
    if (binding->route) trackRouteUpdateLocation(binding->route, &movement->routeLocation, binding->owner->position);
    if (binding->driver && binding->driver->profile != 0) {
        binding->driver->recoveryPose = NULL;
        carMovementRefreshRecoveryPose(movement);
    }
    movement->initialized = 1;
    return carMovementSetMode(movement, mode);
}

void carMovementDispose(CarMovement* movement)
{
    if (!movement || !movement->initialized) return;
    detachMover(movement);
    CollisionShape* shape = movement->bindings.ownerMotion->collision->shape;
    shape->body = NULL; shape->prepareOwner = NULL; shape->bodyMode = 0;
    movement->bindings.owner->movement = NULL;
    movement->bindings.owner->move = NULL;
    movement->initialized = 0;
}

int32_t carMovementUpdateSound(CarMovement* movement)
{
    if (!movement || !movement->frame || movement->frame->movement != movement)
        return 0;
    CarSoundState* sound = movement->frame->environment.sound;
    if (!sound) return 1; // Source13513C has the same genuine absent-sound gate.
    return sound->Update(1.0f, 0, 1.0f) == WORLD_AUDIO_OK;
}



float nullMoverMove(NullMover* mover, float elapsed)
{
    vehicleStateUpdateWheelPoses(mover->state);
    return elapsed;
}

#include "indep/inc/CarInfo.hpp"
#include "indep/inc/AnimatedDriver.hpp"

void carAssignAnimatedDriver(Car* car,const hp2::CarRenderBindings* bindings)
{
    // Complete PAL133488; the source suppresses the driver only for actual
    // CarInfo body category2. The other branch creates its real visual owner.
    hp2::Require(car && car->driverInfo && bindings && bindings->cars,
        "AssignAnimatedDriver has no canonical Car/type binding");
    const auto model=car->driverInfo->appearance.modelCode;
    const auto& info=bindings->cars->Car(static_cast<std::size_t>(model));
    if (info.bodyCategory==2)return;
    hp2::Require(!car->visual,"Car already owns an AnimatedDriver");
    auto visual=bindings->visual;
    visual.movement=static_cast<CarMovement*>(car->movement);
    visual.vehicle=&bindings->cars->VisualInfo(static_cast<std::size_t>(model));
    visual.character=car->driverInfo->character;
    car->visual=carDriverVisualCreate(&visual,bindings->resources,bindings->characters);
}

/* The original Car embeds state, terrain and route coordinates. Native
 * pointers make those same objects larger; one private allocation owns them.
 * It is not another simulation state or a per-car factory. */
struct CarOwnedState {
    explicit CarOwnedState(Car& car):owner(car) {}
    Car& owner;
    VehicleState state{};
    CarMotionState motion{};
    TerrainSurface terrain{};
    CarMovement movement{};
    CarControls controls{};
    PhysicsCarEnvironment physics{};
    CarFrame frame{};
    CarDriver driver{};
    CarDriverEnvironment driverEnvironment{};
    RealDriverBindings realBindings{};
    RealDriverConfiguration realConfiguration{};
    DriverPhysicsVehicle driverPhysics{};
    hp2::CarRenderBindings render{};
    std::unique_ptr<CarDamage> damage;
    std::unique_ptr<hp2::CarCollisionBody> collision;
    CollisionParticipant participant{};
    std::unique_ptr<CarSoundState> sound;
    WorldEventDispatcher* events{};
    bool terrainInitialized{};
    bool handlerRegistered{};

    ~CarOwnedState()
    {
        // Full PAL130EE0 ownership order. Only this graph owns these objects.
        // Controller disposal detaches actual input callbacks before movers.
        carDriverDispose(&driver);
        if (movement.initialized && motion.collision)carMovementDispose(&movement);
        else detachMover(&movement); // rollback before collision construction
        collision.reset();motion.collision=nullptr;
        damage.reset();movement.bindings.damage=nullptr;motion.damage=nullptr;
        carAppearanceDispose(&owner);
        sound.reset();frame.environment.sound=nullptr;
        if (handlerRegistered)worldEventsUnregisterCarHandler(events);
        // TrackPathCoordinate's final208748 destructor only applies the
        // deleting ABI flag. Its authored TrackRoute is borrowed, not freed.
        if (terrainInitialized)terrainSurfaceDispose(&terrain);
        owner.movement=nullptr;owner.move=nullptr;
        owner.interactionContext=nullptr;owner.interaction=nullptr;
    }
};

Car::Car(RaceDriverInfo& selection,const float* initialPosition,uint16_t heading,
    float routeDistanceHint,const hp2::CarConstructionBindings& binding):Car()
{
    static std::uint64_t nextPresentationIdentity = 0;
    presentationId = ++nextPresentationIdentity;
    // Complete PAL130A48..130EE0. In the release F12 is an additional route
    // distance hint; beta's shorter constructor has no such argument.
    hp2::Require(binding.definition && binding.topology && binding.movement &&
        binding.physics && binding.physics->clock && binding.physics->tuning &&
        binding.frame && binding.frame->world && binding.frame->clock && binding.frame->settings &&
        binding.driver && binding.render && binding.render->cars && binding.collision && binding.events,
        "Car construction is missing its actual World/Track/physics/render owners");
    hp2::Require(binding.movement->pools && binding.movement->route &&
        binding.frame->clock==binding.physics->clock && binding.collision->world==binding.frame->world &&
        binding.frame->spatial==binding.events && binding.render->applicationState==binding.movement->applicationState,
        "Car construction supplied different simulation/settings owners");
    hp2::Require(selection.appearance.modelCode>=0 && selection.appearance.modelCode<64 &&
        &binding.render->cars->Car(static_cast<size_t>(selection.appearance.modelCode))==binding.definition,
        "Car constructor selection is not its canonical retained CarInfo");
    hp2::Require(std::isfinite(routeDistanceHint) && (!initialPosition ||
        (std::isfinite(initialPosition[0]) && std::isfinite(initialPosition[1]) && std::isfinite(initialPosition[2]))),
        "Car constructor position/hint is not finite");
    auto owned=std::make_unique<CarOwnedState>(*this);
    auto& data=*owned;auto& ownedMovement=data.movement;auto& state=data.state;auto& motion=data.motion;
    auto& frame=data.frame;
    data.events=binding.events;data.render=*binding.render;
    data.physics=*binding.physics;
    data.physics.settings=binding.frame->settings;
    data.physics.contact.terrain=&data.terrain;
    ownedMovement.info=binding.definition->PhysicsInfo();
    ownedMovement.bindings=*binding.movement;
    auto& live=ownedMovement.bindings;
    live.info=&ownedMovement.info;live.owner=this;live.state=&state;live.ownerMotion=&motion;
    live.controls=&data.controls;live.environment=&data.physics;
    live.driver=nullptr;live.player=nullptr;live.damage=nullptr;live.belt=nullptr;
    this->movement=&ownedMovement;this->move=advanceOwner;
    frame.movement=&ownedMovement;frame.environment=*binding.frame;
    frame.environment.player=nullptr;frame.environment.driver=&data.driver;
    frame.environment.sound=nullptr;frame.environment.carReplay=nullptr;
    frame.environment.routeVehicleClass=ownedMovement.info.routeVehicleClass;
    ownedMovement.frame=&frame;interactionContext=&frame;interaction=carFrameInteractionFactor;

    hp2::InitializeCarState(state,position,data.terrain,*binding.definition);
    motion.wheelExclusionMask=0;motion.wheelEffectsFlags=UINT32_MAX;motion.wheelInclusionMask=0;
    terrainSurfaceInitialize(&data.terrain,binding.topology);data.terrainInitialized=true;
    trackRouteLocationInitialize(&ownedMovement.routeLocation);
    float point[3]{};
    if (initialPosition) {
        std::memcpy(point,initialPosition,sizeof(point));
        std::memcpy(frame.state.previousPosition,initialPosition,sizeof(point));
    }
    // Never call carMovementInitialize here: it would overwrite this exact
    // coordinate/hint and reinitialize already constructed CarState.
    if (routeDistanceHint<0.0f)trackRouteUpdateLocation(live.route,&ownedMovement.routeLocation,point);
    else trackRouteRelocate(live.route,&ownedMovement.routeLocation,point,routeDistanceHint);
    std::memcpy(position,point,sizeof(point));
    state.heading=heading;
    motion.disabledWheelMask=0;frame.state.disabledWheelCount=0;
    ownedMovement.frictionBoost=1.0f;interactionFactor=0.0f;
    pursuitDamageResponseScale=0.0f;physicsBoost=0.0f;
    // A native unproduced bearing is explicit until RenderCars calls
    // DamageAbove; no centred marker is fabricated before that source pass.
    trackerBearing=std::numeric_limits<float>::quiet_NaN();
    driverInfo=&selection;tableIndex=0;driverNumber=selection.driverNumber;
    raceEntry=nullptr;visual=nullptr;

    // The actual enabled global gates creation. Missing resources on its
    // enabled branch are failures; they never silently select disabled audio.
    const auto* policy=binding.sound && binding.sound->globals ?
        &binding.sound->globals->policy:carSoundCurrentPolicy();
    if (policy->audioSystemEnabled) {
        hp2::Require(binding.sound && binding.sound->clock==frame.environment.clock &&
            binding.sound->cars==binding.render->cars,
            "Enabled Car sound has no same-owner resource/audio binding");
        auto audio=*binding.sound;audio.frame=&frame.state;
        WorldAudioResult result=WORLD_AUDIO_INVALID_BINDING;
        if (audio.audio && audio.audio->library)audio.audio->library->ClearSourceFailure();
        if (driverNumber==0 || driverNumber==1)data.sound.reset(carSoundCreatePlayer(&ownedMovement,&audio,&result));
        else if (ownedMovement.info.bodyCategory==0 || ownedMovement.info.bodyCategory==2)
            data.sound.reset(carSoundCreateAi(&ownedMovement,&audio,&result));
        else if (ownedMovement.info.bodyCategory==1)data.sound.reset(carSoundCreateAiCop(&ownedMovement,&audio,&result));
        if (!data.sound || result!=WORLD_AUDIO_OK) {
            std::string failure="Car sound constructor failed on its actual enabled branch; model="+
                std::to_string(selection.appearance.modelCode)+"; driver="+std::to_string(driverNumber)+
                "; category="+std::to_string(ownedMovement.info.bodyCategory)+
                "; audio result="+std::to_string(static_cast<int>(result));
            if (audio.audio && audio.audio->library) {
                const auto detail=audio.audio->library->FailureDescription();
                if (!detail.empty())failure+="; "+detail;
            }
            throw std::runtime_error(failure);
        }
        frame.environment.sound=data.sound.get();
    }
    data.driverPhysics={&ownedMovement,&binding.definition->performance,
        binding.driver->physics && binding.driver->physics->tuning ? binding.driver->physics->tuning:&driverPhysicsDefaultTuning,
        ownedMovement.info.steeringRangeDegrees,ownedMovement.info.routeVehicleClass};
    data.driverEnvironment=*binding.driver;
    data.driverEnvironment.movement=&ownedMovement;data.driverEnvironment.physics=&data.driverPhysics;
    data.driverEnvironment.clock=frame.environment.clock;data.driverEnvironment.sound=data.sound.get();
    if(binding.driver->real) {
        data.realBindings=*binding.driver->real;
        data.realConfiguration={binding.definition->steeringRangeDegrees,
            binding.definition->driveTrainInfo.forwardGearCount,binding.definition->bodyCategory};
        data.realBindings.configuration=&data.realConfiguration;
        data.realBindings.frame=&frame;data.realBindings.sound=data.sound.get();
        data.realBindings.player=nullptr;
        data.driverEnvironment.real=&data.realBindings;
    }
    data.driver.environment=&data.driverEnvironment;
    hp2::Require(carDriverChange(&data.driver,this,CAR_DRIVER_NULL)==CAR_DRIVER_OK,
        "Car could not construct its actual NullController/NullDriver");
    PhysicsPoolStatus poolResult=PHYSICS_POOL_OK;
    ownedMovement.inactive=physicsObjectsAcquireNullMover(live.pools,&poolResult);
    hp2::Require(ownedMovement.inactive && poolResult==PHYSICS_POOL_OK,"Car NullMover pool is exhausted");
    nullMoverInitialize(ownedMovement.inactive,&state);
    if (ownedMovement.info.bodyCategory!=2) {
        // PAL1B54D4/1B54E4 bind BodyDamage and attachment markers to the
        // same CarTypeInfo+20 geometry name used by CarRenderInfo.
        data.damage=std::make_unique<CarDamage>(this,binding.definition->baseName.c_str());
        data.damage->body.settings=frame.environment.settings;
        live.damage=data.damage.get();motion.damage=live.damage;
    }
    motion.inputCategory=0;motion.owner=this;ownedMovement.mode=0;
    data.collision=std::make_unique<hp2::CarCollisionBody>(*binding.collision,*this,state,data.terrain,
        *binding.definition,binding.collisionResources,binding.collisionResourceCount,*frame.environment.clock);
    motion.collision=&data.collision->State();
    // Final880/884/888 are unused per-Car ESP debugger handles, not simulation
    // allocations. The optional PS2 host-debug channel25C5A8/25B900 is retired.
    carMovementUpdateTerrainPose(&ownedMovement);
    state.gear=2;ownedMovement.publishedGear=1;ownedMovement.displaySpeed=0.0f;
    appearance=nullptr;
    carInitRenderInfo(this,&data.render);
    if (data.damage)carDamageInitSectorPositions(data.damage.get(),data.render.markers);
    motion.actionEnabled=1;driverVisible=1;motion.recoveryRequested=0;driveCountdown=0;
    frame.state.lastGroundPoint[0]=position[0];frame.state.lastGroundPoint[1]=position[1];
    // Callback130A28's only target133918 is an exact empty body. Preserve
    // add/remove reference lifetime without introducing an empty runtime stub.
    worldEventsRegisterCarHandler(data.events);data.handlerRegistered=true;
    frame.state.signalWindowStart=frame.state.signalWindowEnd=frame.state.signalWindowLocked=0;
    frame.state.signalEnabled=frame.state.signalMode=0;
    frame.state.blinkVisible=frame.state.blinkForced=0;
    frame.state.blinkVisualEnabled=frame.state.blinkControlEnabled=frame.state.blinkPhase=0;
    frame.state.blinkStart=frame.environment.clock->worldTimer;
    frame.state.blinkInterval=frame.state.blinkOffset=frame.state.blinkDuration=-1.0f;
    carMotionClearWindow(&motion);
    const float ticks=static_cast<float>(frame.environment.clock->worldTimer);
    const float seconds=ticks*0.0002500000118743628f;
    frame.state.feedbackDeadline=convertFloatToInt32((seconds*4000.0f)+0.5f);
    motion.transitionCounter=1;frame.state.airborneDeadline=0.0f;motion.transitionFlag=0;
    motion.windowDurationSeconds=2.0f;
    std::memset(frame.state.wheelSlipFrames,0,sizeof(frame.state.wheelSlipFrames));
    frame.state.previousSurfaceMask=0;frame.state.specialEffectRequested=0;frame.state.looseSurfaceHighlight=0;
    ownedMovement.result=CAR_MOVEMENT_OK;ownedMovement.initialized=1;frame.result=CAR_FRAME_OK;
    data.participant.shape=&data.collision->Shape();data.participant.movement=&ownedMovement;
    data.participant.terrain=&data.terrain; // Same original Car cache borrowed by physics and collision.
    hp2::Require(collisionSystemBind(binding.collision,&data.participant)==COLLISION_SYSTEM_OK,
        "Car could not register the same movement with its collision owner");
    ownedState=owned.release();
}

Car::~Car()
{
    // Default borrowed facades and World's list sentinel own no full graph.
    delete ownedState;
}

CarMovement* Car::Movement() noexcept { return static_cast<CarMovement*>(movement); }
CarDriver* Car::Driver() noexcept { return ownedState ? &ownedState->driver:nullptr; }
CarFrame* Car::Frame() noexcept { return ownedState ? &ownedState->frame:nullptr; }

void Car::GetRenderingWheelAndBrakeMatrices(float wheels[4][16],float brakes[4][16],
    int32_t viewNumber)
{
    // Complete PAL135FB8..1360DC; corresponding RenderCars direct call is
    // retained in chatgpt_r12_scene57/individual_paths57.json. The old native
    // field name wheelExclusionMask denotes the original view override mask.
    auto* mover=Movement();
    hp2::Require(mover && mover->bindings.ownerMotion && mover->bindings.state &&
        wheels && brakes && viewNumber>=0 && viewNumber<32,
        "Rendering wheel matrices require the actual Car physics owner");
    if ((mover->bindings.ownerMotion->wheelExclusionMask>>viewNumber)&1u) {
        hp2::Require(ownedState && ownedState->render.cars && driverInfo,
            "Override wheel matrices have no authored CarInfo");
        const auto& definition=ownedState->render.cars->Car(static_cast<std::size_t>(driverInfo->appearance.modelCode));
        for (std::size_t wheel=0;wheel<4;++wheel) {
            eIdentity(wheels[wheel]);eIdentity(brakes[wheel]);
            for (std::size_t axis=0;axis<3;++axis)
                wheels[wheel][12+axis]=brakes[wheel][12+axis]=definition.wheels[wheel].position[axis];
        }
    } else {
        std::memcpy(wheels,mover->bindings.state->wheelVisualTransform,4*16*sizeof(float));
        std::memcpy(brakes,mover->bindings.state->wheelMountTransform,4*16*sizeof(float));
    }
}
void Car::BindWorldServices(ReplayManager* replay,PursuitManager* pursuit,Chopper* chopper) noexcept
{
    if(!ownedState)return;
    auto& state=*ownedState;
    state.frame.environment.replayManager=replay;
    if(state.driverEnvironment.real) {
        state.realBindings.replay=replay;state.realBindings.pursuitManager=pursuit;state.realBindings.chopper=chopper;
    }
    if(state.driver.kind==CAR_DRIVER_REAL && state.driver.owner.real) {
        auto& input=state.driver.owner.real->input;
        input.replay=replay;input.pursuitManager=pursuit;input.chopper=chopper;
    }
}

// PAL133898 IsCarInFront: compare the car position with the view direction.
#include "indep/inc/Pursuit.hpp"
void Car::DamageAbove(const float eye[3],const float forward[3],
    const hp2::CarDamageDisplayBindings& display)
{
    // Full final PAL135280. The scalar is produced even if the subsequent
    // damage-cap branch is suppressed by NIS, race policy or flow state.
    hp2::Require(eye && forward,"Car bearing has no actual camera vectors");
    const float delta[3]={eye[0]-position[0],eye[1]-position[1],eye[2]-position[2]};
    const float length=std::sqrt((delta[0]*delta[0]+delta[1]*delta[1])+delta[2]*delta[2]);
    const float reciprocal=1.0f/length;
    const float direction[3]={delta[0]*reciprocal,delta[1]*reciprocal,delta[2]*reciprocal};
    const float dot=(forward[0]*direction[0]+forward[1]*direction[1])+forward[2]*direction[2];
    const float crossZ=forward[0]*direction[1]-forward[1]*direction[0];
    trackerBearing=(dot+1.0f)*0.5f;
    if (crossZ>=0.0f)trackerBearing=-trackerBearing;
    for (const auto* player:display.players.playersByNumber)
        if (player && carFramePlayerReplayActive(player))return;
    if (!display.parameters.specialPursuit || display.sessionState!=3 || !display.pursuit)return;
    const auto* carMovement=static_cast<const CarMovement*>(this->movement);
    hp2::Require(carMovement && carMovement->frame && carMovement->frame->environment.driver,
        "Car damage display has no actual movement/controller owner");
    const auto& frame=*carMovement->frame;
    if (frame.environment.driver->kind!=CAR_DRIVER_AI &&
        (!frame.environment.player || carMovement->info.bodyCategory==1))return;
    const float capDelta[3]={position[0]-eye[0],position[1]-eye[1],position[2]-eye[2]};
    float distance=std::sqrt((capDelta[0]*capDelta[0]+capDelta[1]*capDelta[1])+capDelta[2]*capDelta[2])*
        0.004999999888241291f;
    if (distance<0.0f)distance=0.0f;
    if (distance>1.0f)distance=1.0f;
    PursuitCollisionGroup* pursuit=nullptr;
    hp2::Require(pursuitManagerFindPursuitForCar(display.pursuit,this,&pursuit)==WORLD_SERVICES_OK &&
        display.pursuit->collision && tableIndex<WORLD_MAX_CARS,
        "Car damage display has no canonical pursuit/damage owner");
    DamageCap(pursuit!=nullptr,display.pursuit->collision->damageLevels[tableIndex],distance,display);
}

void Car::DamageCap(bool pursued,float damage,float distanceScale,
    const hp2::CarDamageDisplayBindings& display)
{
    // Complete PAL135490..135AA0. Final literals are initialized by the
    // source car.cpp static initializer136744..136830; no beta assets.
    const auto* descriptor=display.resources.ResolveTexture(hashString("WHITE16X16_NOALPHA"),false);
    const auto* texture=hp2::textureInfoForDescriptor(descriptor);
    float scale=distanceScale+distanceScale+0.05000000074505806f;
    if (display.sessionState==4 || display.sessionState==7)scale=0.10000000149011612f;
    const auto* carMovement=static_cast<const CarMovement*>(this->movement);
    hp2::Require(carMovement && carMovement->bindings.state,"Damage cap has no actual Car orientation owner");
    float carMatrix[16];std::memcpy(carMatrix,carMovement->bindings.state->orientation,sizeof(carMatrix));
    std::memcpy(carMatrix+12,position,3*sizeof(float));carMatrix[15]=1.0f;
    constexpr float capMatrix[16]={.5f,0,0,0,0,.5f,0,0,0,0,.5f,0,0,0,1,1};
    float localToWorld[16];eMulMatrix(localToWorld,capMatrix,carMatrix);
    constexpr std::array<float,4> anchor{0,0,0,0};
    constexpr std::array<std::array<float,4>,4> corners{{{3,0,3,0},{0,-3,3,0},{-3,0,3,0},{0,3,3,0}}};
    hp2::FlailerPoly polygon;
    constexpr std::array<std::array<float,3>,4> uv{{{0,0,1},{0,1,1},{1,1,1},{1,0,1}}};
    for (std::size_t i=0;i<4;++i)polygon.vertices[i].textureCoordinate=uv[i];
    polygon.rasterState={0x01000100u,0x00000080u,0,0};
    const auto put=[&](std::size_t i,const std::array<float,4>& value) {
        for (std::size_t axis=0;axis<4;++axis)polygon.vertices[i].position[axis]=value[axis]*scale;
    };
    const auto paint=[&](std::uint32_t color) {for (auto& vertex:polygon.vertices)vertex.color=color;};
    const auto emit=[&]() {display.frame.RenderPolygon3D(polygon,texture,localToWorld,display.view);};
    const std::uint32_t baseColor=pursued?0xFFCC0000u:0xFF00AA00u;
    paint(baseColor);
    if (damage==0.0f) {
        for (std::size_t i=0;i<4;++i) {
            put(0,anchor);put(1,corners[i]);put(2,corners[(i+1)%4]);put(3,anchor);emit();
        }
    } else {
        std::array<std::array<float,4>,4> direction{};
        for (std::size_t i=0;i<4;++i)for (std::size_t axis=0;axis<4;++axis)
            direction[i][axis]=corners[i][axis]-anchor[axis];
        const float length=std::sqrt((direction[0][0]*direction[0][0]+direction[0][1]*direction[0][1])+
            direction[0][2]*direction[0][2]);
        const float damagedLength=length*damage;
        const float reciprocal=1.0f/length;
        for (auto& value:direction)for (float& axis:value)axis*=reciprocal;
        for (std::size_t i=0;i<4;++i) {
            const auto nextCorner=(i+1)%4;
            put(0,anchor);put(1,anchor);
            std::array<float,4> current{},following{};
            for (std::size_t axis=0;axis<4;++axis) {
                current[axis]=anchor[axis]+direction[i][axis]*damagedLength;
                following[axis]=anchor[axis]+direction[nextCorner][axis]*damagedLength;
            }
            put(2,following);put(3,current);
            paint(pursued?0xFF995500u:0xFF005599u);emit();
            polygon.vertices[0].position=polygon.vertices[3].position;
            polygon.vertices[1].position=polygon.vertices[2].position;
            put(2,corners[nextCorner]);put(3,corners[i]);paint(baseColor);emit();
        }
    }
    for (std::size_t i=0;i<4;++i)put(i,corners[i]);
    emit();
}

int32_t driverCarInHeadingCone(const VehicleState* observer,const VehicleState* target,
                               uint16_t maximumDifference)
{
    const float direction[2]={target->position[0]-observer->position[0],
                              target->position[1]-observer->position[1]};
    VectorPolar2 polar;
    vectorPolarFromCartesian2(&polar,direction);
    const uint16_t wrapped=(uint16_t)(polar.angle-observer->heading);
    const int32_t difference=wrapped<32768u?(int32_t)wrapped:(int32_t)wrapped-65536;
    const int32_t magnitude=difference<0?-difference:difference;
    return magnitude<(int32_t)maximumDifference;
}

// Full PAL130958..1309F0, CarDriver::DoSnapshot. The controls and command
// belong to the active native input owner; the dispatch facade owns no copy.
void carDriverInputSnapshot(CarMovement* movement,DriverCommand* command,ReplaySnapshot* snapshot)
{
    CarControls* controls=movement->bindings.controls;
    replaySnapshotBeginSection(snapshot,nullptr,"CarDriver");
    replaySnapshotMoveData(snapshot,&controls->steeringAngle,2,2,0);
    replaySnapshotMoveFloat(snapshot,&controls->throttle);
    replaySnapshotMoveFloat(snapshot,&controls->brake);
    replaySnapshotMoveFloat(snapshot,&controls->handbrake);
    replaySnapshotMoveInteger(snapshot,&controls->gearRequest,1);
    driverCommandSnapshot(command,movement->bindings.recoveryPoints,snapshot);
    replaySnapshotEndSection(snapshot);
}

#include "indep/inc/TrafficCar.hpp"
#include "indep/inc/CarController.hpp"
#include <cstdio>

// Complete final PAL130FF0..131410. Concrete mode dispatch replaces the
// corresponding source vtables; it has no separate state or success fallback.
void carDoSnapshot(Car* car,ReplaySnapshot* snapshot)
{
    hp2::Require(car&&car->ownedState,"Car snapshot requires its actual constructed owner");
    auto& owned=*car->ownedState;
    auto& movement=owned.movement;
    auto& motion=owned.motion;
    auto& frame=owned.frame.state;
    char sectionName[16];
    std::snprintf(sectionName,sizeof(sectionName),"Car%d",static_cast<int>(static_cast<int16_t>(car->tableIndex)));
    replaySnapshotBeginSection(snapshot,nullptr,sectionName);
    replaySnapshotMoveData(snapshot,&frame.signalMode,1,-1,0);
    replaySnapshotMoveUnsigned(snapshot,&frame.retainedSnapshotFlag,1);
    replaySnapshotMoveData(snapshot,&frame.blinkStart,4,-1,0);
    replaySnapshotMoveFloat(snapshot,&frame.blinkInterval);
    replaySnapshotMoveFloat(snapshot,&frame.blinkOffset);
    replaySnapshotMoveFloat(snapshot,&frame.blinkDuration);
    replaySnapshotMoveData(snapshot,&frame.blinkVisible,1,4,0); // source834
    replaySnapshotMoveData(snapshot,&frame.blinkForced,1,4,0);  // source830
    replaySnapshotMoveData(snapshot,&frame.blinkVisualEnabled,1,4,0);
    replaySnapshotMoveData(snapshot,&frame.blinkControlEnabled,1,4,0);
    replaySnapshotMoveData(snapshot,&frame.blinkPhase,1,4,0);   // source81C
    replaySnapshotMoveData(snapshot,&frame.signalEnabled,1,4,0);
    replaySnapshotMoveInteger(snapshot,&frame.signalWindowStart,4);
    replaySnapshotMoveInteger(snapshot,&frame.signalWindowEnd,4);
    replaySnapshotMoveData(snapshot,&frame.signalWindowLocked,1,4,0);
    replaySnapshotMoveData(snapshot,&motion.disabledWheelMask,1,1,1);
    replaySnapshotMoveData(snapshot,&frame.disabledWheelCount,1,1,1);
    replaySnapshotMoveData(snapshot,&frame.disabledWheelStartTicks,4,-1,0);
    if(owned.damage)carDamageSnapshot(owned.damage.get(),snapshot);
    // Keep the native tag valid while destroying the old concrete owner.
    // PAL stores selection independently of its old object's live vtable.
    int32_t controllerKind=static_cast<int32_t>(owned.driver.kind),moverKind=movement.mode;
    replaySnapshotMoveData(snapshot,&controllerKind,1,4,0);
    replaySnapshotMoveData(snapshot,&moverKind,1,4,0);
    const int8_t oldSkin=car->driverInfo->appearance.skinNumber;
    replaySnapshotMoveData(snapshot,&car->driverInfo->appearance.skinNumber,1,1,1);
    if(snapshot->restoring) {
        hp2::Require(controllerKind>=CAR_DRIVER_NULL&&controllerKind<=CAR_DRIVER_COP&&moverKind>=0&&moverKind<=6,
            "Car snapshot contains an invalid controller or mover mode");
        hp2::Require(carDriverChange(&owned.driver,car,CAR_DRIVER_NULL)==CAR_DRIVER_OK,
            "Car snapshot could not release the old controller");
        hp2::Require(carMovementSetMode(&movement,0)==CAR_MOVEMENT_OK,
            "Car snapshot could not release the old mover");
        hp2::Require(carDriverChange(&owned.driver,car,static_cast<CarDriverKind>(controllerKind))==CAR_DRIVER_OK,
            "Car snapshot could not construct its recorded controller");
        hp2::Require(carMovementSetMode(&movement,moverKind)==CAR_MOVEMENT_OK,
            "Car snapshot could not construct its recorded mover");
        if(oldSkin!=car->driverInfo->appearance.skinNumber)
            hp2::Require(carSwitchSkin(car,car->driverInfo->appearance.skinNumber)==CAR_APPEARANCE_OK,
                "Car snapshot skin is absent from the actual model catalogue");
    }
    switch(owned.driver.kind) {
    case CAR_DRIVER_NULL:carControllerSnapshot(&movement,snapshot);break;
    case CAR_DRIVER_REAL:realControllerSnapshot(owned.driver.owner.real,snapshot);break;
    case CAR_DRIVER_AI:aiDriverSnapshot(owned.driver.owner.ai,snapshot);break;
    case CAR_DRIVER_TRAFFIC:trafficDriverSnapshot(owned.driver.owner.traffic,snapshot);break;
    case CAR_DRIVER_COP:copDriverSnapshot(owned.driver.owner.cop,snapshot);break;
    }
    switch(movement.mode) {
    case 0:case 3:break; // Actual empty inherited CarMover/DeliveryMover snapshots.
    case 1:physicsCarSnapshot(movement.physics,snapshot);break;
    case 2:trafficCarSnapshot(movement.traffic,snapshot);break;
    case 4:mellowMoverSnapshot(movement.mellow,snapshot);break;
    case 5:copMoverSnapshot(movement.cop,snapshot);break;
    case 6:beltMoverSnapshot(movement.belt,snapshot);break;
    default:hp2::Require(false,"Car snapshot has an invalid live mover mode");
    }
    vehicleStateSnapshot(&owned.state,snapshot);
    collisionCarSnapshot(movement.collisionSystem,motion.collision,snapshot);
    if(car->visual)carDriverVisualSnapshot(car->visual,snapshot);
    replaySnapshotMoveFloat(snapshot,&movement.displaySpeed);
    replaySnapshotMoveData(snapshot,&movement.publishedGear,1,-1,0);
    replaySnapshotMoveFloat(snapshot,&movement.frictionBoost);
    replaySnapshotMoveData(snapshot,&motion.actionEnabled,1,1,1);
    replaySnapshotMoveData(snapshot,&car->driverVisible,1,4,0);
    replaySnapshotMoveFloat(snapshot,&frame.airborneDeadline);
    trackRouteLocationSnapshot(movement.bindings.route,&movement.routeLocation,snapshot);
    uint8_t packed=static_cast<uint8_t>(static_cast<uint8_t>(motion.recoveryRequested)+
        (static_cast<unsigned>(static_cast<uint8_t>(car->driveCountdown))<<4));
    replaySnapshotMoveData(snapshot,&packed,1,1,0);
    if(snapshot->restoring) {motion.recoveryRequested=packed&15u;car->driveCountdown=packed>>4;}
    replaySnapshotMoveData(snapshot,&motion.transitionFlag,1,4,0);
    replaySnapshotMoveData(snapshot,&motion.transitionCounter,1,4,0);
    replaySnapshotMoveData(snapshot,&motion.windowStartTicks,4,-1,0);
    replaySnapshotMoveFloat(snapshot,&motion.windowDurationSeconds);
    replaySnapshotMoveData(snapshot,frame.lastGroundPoint,8,-1,0);
    replaySnapshotMoveData(snapshot,frame.wheelSlipFrames,4,4,0);
    if(snapshot->recording || snapshot->restoring)car->interactionFactor=0.0f;
    replaySnapshotMoveData(snapshot,&frame.specialEffectRequested,1,4,0);
    replaySnapshotMoveFloat(snapshot,&car->pursuitDamageResponseScale);
    replaySnapshotMoveFloat(snapshot,&car->physicsBoost);
    replaySnapshotEndSection(snapshot);
}


// PAL132A78 ChangeGear. Car+4B4 is the actual AnimatedDriver, not an audio
// emitter. Reverse state belongs to that same visual's action3 byte.
void carMovementSetPublishedGear(CarMovement* movement,int32_t gear)
{
    const int32_t previous=movement->publishedGear;
    if(gear==previous)return;
    auto* visual=movement->bindings.owner ? movement->bindings.owner->visual : nullptr;
    if(previous!=1 && visual) {
        if(previous==0)carDriverVisualEndAction3(visual);
        carDriverVisualBeginGearChange(visual);
        if(gear==0)carDriverVisualBeginAction3(visual);
        // A busy/blocked animation is a normal source outcome. Only an
        // actual invalid owner/definition result fails the native operation.
        if(carDriverVisualResult(visual)!=CAR_DRIVER_VISUAL_OK)
            movement->result=CAR_MOVEMENT_PRESENTATION_FAILURE;
    }
    movement->publishedGear=gear;
}

// PAL132D40 ResetCore, with the actual replay owner borrowed from CarFrame.
CarMovementResult carMovementPlace(CarMovement* movement, const float position[3], uint16_t heading, float speed)
{
    const int32_t oldMode = movement->mode;
    if (!movement->bindings.route) return movement->result = CAR_MOVEMENT_MISSING_ROUTE;
    auto* driver=static_cast<CarDriver*>(movement->bindings.owner->controller);
    hp2::Require(driver && driver->environment && driver->environment->movement==movement,
        "Car placement has no actual controller/input owner");
    if (carMovementSetMode(movement, 0) != CAR_MOVEMENT_OK) return movement->result;
    VehicleState* state = movement->bindings.state;
    vehicleStateReset(state);
    trackRouteUpdateLocation(movement->bindings.route, &movement->routeLocation, position);
    state->heading = heading; state->steeringAngle = 0;
    memcpy(state->position, position, 3 * sizeof(float));
    state->position[3] = 1.0f;
    state->forwardSpeed = speed;
    terrainSurfaceReset(movement->bindings.environment->contact.terrain);
    carMovementUpdateTerrainPose(movement);
    float velocity[4] = {0,0,0,0};
    vectorSetLength3(velocity, state->orientation, speed + 0.10000000149011612f);
    memcpy(state->velocity, velocity, sizeof(velocity));
    const float x = velocity[0] * velocity[0], y = velocity[1] * velocity[1], z = velocity[2] * velocity[2];
    state->speed = sqrtf((x + y) + z);
    state->velocityFrameValid = 0;
    for (int wheel = 0; wheel < 4; ++wheel)
        state->wheelAngularVelocity[wheel] = finiteRatio(state->forwardSpeed, movement->info.wheels[wheel].reactionScale);
    movement->bindings.controls->steeringAngle = 0;
    if(carDriverClearSteeringMemory(driver)!=CAR_DRIVER_OK)
        return movement->result=CAR_MOVEMENT_UNSUPPORTED_DRIVER;
    vehicleStateUpdateWheelPoses(state);
    if(carDriverSelectResetGear(driver)!=CAR_DRIVER_OK)
        return movement->result=CAR_MOVEMENT_UNSUPPORTED_DRIVER;
    if (carMovementSetMode(movement, oldMode) != CAR_MOVEMENT_OK) return movement->result;
    if (movement->bindings.player) {
        // PAL132E88 invokes ReplayManager::InvalidateInterestingEvents
        // (13E2F0), a -99 intensity contribution to the current snapshot.
        // Preserve its synchronous position before ReplayManager::EndFrame.
        if (!movement->frame || !movement->frame->environment.replayManager)
            return movement->result = CAR_MOVEMENT_INVALID_BINDING;
        replayManagerAddHighlight(movement->frame->environment.replayManager,-99.0f);
    }
    return movement->result;
}

// PAL132EB8..132F20 / Beta132F48..132FB0 Car::ResetToPosition.
CarMovementResult carMovementPlaceAndNotify(CarMovement* movement, CarDriver* driver,
    const float position[3], uint16_t heading, float speed)
{
    if (carMovementPlace(movement, position, heading, speed) != CAR_MOVEMENT_OK)
        return movement->result;
    trackRouteUpdateLocation(movement->bindings.route, &movement->routeLocation,
        movement->bindings.state->position);
    if (carDriverOnCarPlaced(driver) != CAR_DRIVER_OK)
        return movement->result = CAR_MOVEMENT_UNSUPPORTED_DRIVER;
    carMotionClearWindow(movement->bindings.ownerMotion);
    return movement->result;
}

// PAL132F20..133070 Car::Reset. After ResetCore, the same controller must
// receive BOTH authored road owners, then its class-specific JustReset hook.
// Writing only movement.bindings.driver->recoveryPose leaves AI/Traffic's
// DrivingLocation group stale and corrupts subsequent DriveTarget references.
void carMovementRecover(CarMovement* movement,const CarRecoveryGroup* group,const CarRecoveryPose* pose,
                        float speed,float directionScale,int32_t setWindow,int32_t holdFrames)
{
    if(!group || !pose)return;
    if(!movement->bindings.route) {movement->result=CAR_MOVEMENT_MISSING_ROUTE;return;}
    trackRouteRelocate(movement->bindings.route,&movement->routeLocation,pose->position,pose->routeDistance);
    const float first=pose->direction[0]*directionScale;
    const float second=pose->direction[1]*directionScale;
    const uint16_t heading=vectorAtan2Angle(first,second);
    if(carMovementPlace(movement,pose->position,heading,speed)!=CAR_MOVEMENT_OK)return;
    trackRouteUpdateLocation(movement->bindings.route,&movement->routeLocation,movement->bindings.state->position);
    auto* driver=static_cast<CarDriver*>(movement->bindings.owner->controller);
    hp2::Require(driver && driver->environment && driver->environment->movement==movement,
        "Car recovery has no actual controller owner");
    if(carDriverResetOnRoad(driver,group,pose)!=CAR_DRIVER_OK)return;
    if(setWindow)carMotionSetWindow(movement->bindings.ownerMotion,2.0f,
        movement->bindings.environment->clock->worldTimer);
    // PAL 0x133034-0x133040: Reset(..., holdFrames=1) stores 2 to
    // Car+0x85c, the same field decremented by Car::Drive at 0x131aa8.
    // The countdown belongs to the actual Car, not to a transient mover copy.
    if(holdFrames)movement->bindings.owner->driveCountdown=2;
}

// PAL134E38 StartForceImpact. Calls its original platform consumer now.
void carMovementSteeringImpact(CarMovement* movement, const float localImpulse[4], float magnitude)
{
    const CarMovementPlayer* player = movement->bindings.player;
    const CarMovementRace* race = movement->bindings.race;
    if (!player || !(race && race->mode != 4 && race->mode != 7 && race->mode != 5 && race->mode != 8 && race->mode != 9) || race->mode == 6 ||
        !player->steeringFeedbackEnabled || player->steeringFeedbackBusy) return;
    if (!movement->bindings.applicationState) {
        movement->result = CAR_MOVEMENT_INVALID_BINDING;
        return;
    }
    if (*movement->bindings.applicationState == 3) return;
    const float* velocity = movement->bindings.state->velocity;
    const float xx = velocity[0] * velocity[0];
    const float yy = velocity[1] * velocity[1];
    const float zz = velocity[2] * velocity[2];
    const float speed = sqrtf((xx + yy) + zz);
    float speedFactor = speed - 5.0f;
    if (speedFactor < 0.0f) speedFactor = 0.0f;
    else {
        if (30.0f < speedFactor) speedFactor = 30.0f;
        speedFactor = speedFactor * 0.03333333507180214f;
    }
    const float limited = minimumFinite(magnitude, 10.0f);
    const float scaled = limited * 0.10000000149011612f;
    const float strength = scaled * speedFactor;
    const float lx = localImpulse[0] * localImpulse[0];
    const float ly = localImpulse[1] * localImpulse[1];
    const float lz = localImpulse[2] * localImpulse[2];
    const float reciprocal = finiteRatio(1.0f, sqrtf((lx + ly) + lz));
    const auto* feedback=movement->bindings.feedback;
    if(!feedback || !feedback->input) {
        movement->result=CAR_MOVEMENT_INVALID_BINDING;
        return;
    }
    const int32_t duration=convertFloatToInt32(strength*100.0f)+100;
    // The released endpoint consumes normalized Y only; localImpulse's
    // other lanes remain mathematical inputs, not another pending request.
    const auto result=joystickPortCollisionForce(feedback->input,
        player->controllerPort,duration,strength,localImpulse[1]*reciprocal);
    if(result!=GAME_INPUT_FORCE_NO_DEVICE)
        movement->result=result==GAME_INPUT_FORCE_INVALID_REQUEST ?
            CAR_MOVEMENT_INVALID_BINDING : CAR_MOVEMENT_PRESENTATION_FAILURE;
}

/* Coordinator and NIS views. */
int32_t carViewMoverOpposesRoad(const CarMovement* movement,const CarDriver* driver)
{
    const CarRecoveryPose* pose=carDriverCurrentPose(driver,0);
    if (!pose) return 0;
    const float* orientation=movement->bindings.state->orientation;
    const float y=orientation[1]*pose->direction[1];
    const float x=orientation[0]*pose->direction[0];
    return !(0.0f<x+y);
}


CarDriveResult carDrive(Car* car, float elapsed)
{
    if (car == 0 || car->controls == 0 || car->updateControls == 0)
        return CAR_DRIVE_MISSING_CONTROLS;

    car->updateControls(car->controls, elapsed);
    if (car->driveCountdown != 0)
        --car->driveCountdown;
    return CAR_DRIVE_OK;
}

CarThinkResult carThink(Car* car, float elapsed)
{
    if (car == 0 || car->interaction == 0 ||
        car->controller == 0 || car->controllerThink == 0)
        return CAR_THINK_MISSING_COMPONENT;

    car->interactionFactor = car->interaction(car->interactionContext, car);
    car->controllerThink(car->controller, elapsed);
    return CAR_THINK_OK;
}


static float elapsedSeconds(int32_t nowTicks, int32_t startTicks)
{
    uint32_t differenceBits = (uint32_t)nowTicks - (uint32_t)startTicks;
    int32_t difference;
    memcpy(&difference, &differenceBits, sizeof(difference));
    return (float)difference * 0.0002500000118743628f;
}

void carMotionRequestRecovery(CarMotionState *state, const CarRecoveryPolicy *policy)
{
    if (policy->registryGuard && policy->positiveEntryGate > 0) {
        hp2::Require(state->owner && state->owner->driverInfo,"Car recovery has no live driver selection");
    }
    if (policy->registryGuard && policy->positiveEntryGate > 0 && state->owner->driverInfo->driverClass == 2) {
        struct PursuitCollisionGroup* group = NULL;
        /* The original void method requires its global manager and actual
         * Car to exist on this branch. A missing native binding is an
         * invariant failure, never a successful no-group substitution. */
        if (!carRecoveryFindForMotion(policy->pursuit, state, &group)) abort();
        if (group) return;
    }
    state->recoveryRequested = 1;
}

void carMotionBeginTransition(CarMotionState *state, int32_t nowTicks)
{
    carMotionSetWindow(state, 2.0f, nowTicks);
    state->transitionCounter = 0;
    state->transitionFlag = 1;
}

void carMotionSetWindow(CarMotionState *state, float seconds, int32_t nowTicks)
{
    state->windowDurationSeconds = seconds;
    state->windowStartTicks = nowTicks;
}

void carMotionClearWindow(CarMotionState *state)
{
    state->windowStartTicks = 0;
}

bool carMotionWindowActive(const CarMotionState *state, int32_t nowTicks)
{
    if (!state->windowStartTicks || state->windowStartTicks == INT32_MAX) return false;
    return elapsedSeconds(nowTicks, state->windowStartTicks) < state->windowDurationSeconds;
}

bool carMotionWheelSelected(const CarMotionState *state, uint32_t wheel, int32_t nowTicks)
{
    const uint32_t bit = UINT32_C(1) << (wheel & 31U);
    if (state->wheelExclusionMask & bit) return false;
    return carMotionWindowActive(state, nowTicks) || (state->wheelInclusionMask & bit) != 0;
}





bool carMotionCanControl(const CarMotionState *state, int32_t nowTicks)
{
    if (carMotionWindowActive(state, nowTicks)) return false;
    return carCollisionAllowsControl(state->collision);
}

int32_t carMotionTransitionFlag(const CarMotionState *state)
{
    return state->transitionFlag;
}

bool carMotionActionAllowed(const CarMotionState *state)
{
    return !carMotionTransitionFlag(state) && state->actionEnabled != 0;
}

bool carMotionClassActionAllowed(const CarMotionState *state)
{
    if (!carMotionActionAllowed(state)) return false;
    hp2::Require(state->owner && state->owner->driverInfo,"Car visual-effects gate has no live driver selection");
    return state->owner->driverInfo->driverClass != 3;
}

float carMotionPostCollisionYaw(const CarMotionState *state, int32_t nowTicks)
{
    static const CurveKey keys[] = {{0.0f, 0.0f}, {0.4f, 1.0f}, {1.0f, 1.0f}, {2.0f, 0.0f}};
    static const KeyedCurve curve = {keys, 4};
    const int32_t collisionTicks = state->collision->lastCollisionTimeTicks;
    if (!collisionTicks || collisionTicks == INT32_MAX) return 0.0f;
    const float elapsed = elapsedSeconds(nowTicks, collisionTicks);
    return elapsed < 4.0f ? keyedCurveEvaluate(&curve, elapsed) : 0.0f;
}

bool carMotionHighSpeedSideways(const VehicleState *state)
{
    if (state->activeContactCount < 2) return false;
    if (!(0.12999999523162842f < fabsf(vehicleStateMovingHeadingDifference(state)))) return false;
    return 12.0f < state->forwardSpeed;
}

void carMotionRecoverBelowGround(CarMotionState *state, const VehicleState *vehicle,
                                 TerrainSurface *terrain, const CarRecoveryPolicy *policy)
{
    if (!state->collision->shape->contactCount || vehicle->activeContactCount != 4) return;
    bool found;
    const float height = terrainSurfaceElevation(terrain, vehicle->position, NULL, NULL, &found);
    if (found && vehicle->position[2] + 0.25f < height) carMotionRequestRecovery(state, policy);
}


RigidBody* carMovementCollisionBody(CarMovement* movement)
{
    if (movement->mode == 1 && movement->physics) return movement->physics->body;
    if (movement->mode == 4 && movement->mellow) return movement->mellow->body;
    return NULL;
}

VehicleState* carMovementState(CarMovement* movement)
{ return movement->bindings.state; }

CarMotionState* carMovementOwnerMotion(CarMovement* movement)
{ return movement->bindings.ownerMotion; }

struct CarDamage* carMovementDamage(CarMovement* movement)
{ return movement->bindings.damage; }

void carMovementBindCollision(CarMovement* movement, CollisionSystem* system, CollisionParticipant* participant)
{
    movement->collisionSystem = system;
    movement->collisionParticipant = participant;
}







void carMovementPrepareCollision(CarMovement* movement)
{
    if (movement->mode == 1 && movement->physics) physicsCarPrepare(movement->physics);
    else if (movement->mode == 4 && movement->mellow) mellowMoverPrepare(movement->mellow);
}



void carMovementNotifyCollision(CarMovement* movement, CarMovement* other)
{
    CarDriver* controller = (CarDriver*)movement->bindings.owner->controller;
    if (controller == NULL) {
        carMovementDefaultCollisionNotify(movement);
        return;
    }
    DriverAIBase* driver = NULL;
    switch (controller->kind) {
    case CAR_DRIVER_NULL:
    case CAR_DRIVER_REAL:
    case CAR_DRIVER_COP:
        carMovementDefaultCollisionNotify(movement);
        return;
    case CAR_DRIVER_AI:
        if (controller->owner.ai) driver = &controller->owner.ai->base;
        break;
    case CAR_DRIVER_TRAFFIC:
        if (controller->owner.traffic) driver = &controller->owner.traffic->base;
        break;
    default:
        break;
    }
    if (driver == NULL || driver->physics.movement != movement) {
        controller->result = CAR_DRIVER_INVALID_BINDING;
        movement->result = CAR_MOVEMENT_INVALID_BINDING;
        return;
    }
    driverAIOnCollision(driver, other);
    controller->result = driver->result;
}





void carMovementAlignTerrainPose(const PhysicsCarInfo* info, TerrainSurface* terrain,
                                 float orientation[16], float position[4], uint16_t heading)
{
    static const float offsets[5][2] = {{0,0},{1,0},{-1,0},{0,1},{0,-1}};
    float elevations[5];
    for (int sample = 0; sample < 5; ++sample) {
        const float point[4] = {bScalarAdd(offsets[sample][0], position[0]), bScalarAdd(offsets[sample][1], position[1]), position[2], 0};
        elevations[sample] = terrainSurfaceElevation(terrain, point, NULL, NULL, NULL);
    }
    const float positiveX = bScalarAdd(elevations[1], -elevations[0]), negativeX = bScalarAdd(elevations[2], -elevations[0]);
    const float positiveY = bScalarAdd(elevations[3], -elevations[0]), negativeY = bScalarAdd(elevations[4], -elevations[0]);
    const float slopeX = bScalarMultiply(bScalarAdd(positiveX, -negativeX), 0.5f);
    const float slopeY = bScalarMultiply(bScalarAdd(positiveY, -negativeY), 0.5f);
    const int32_t xAngle = convertFloatToInt32(bScalarMultiply(slopeX, 10435.66796875f));
    const uint16_t yRotation = (uint16_t)(0u - (uint32_t)xAngle);
    const uint16_t xRotation = (uint16_t)convertFloatToInt32(bScalarMultiply(slopeY, 10435.66796875f));
    const float radiusHeight = bScalarAdd(elevations[0], info->wheels[3].reactionScale);
    const float mountHeight = bScalarAdd(radiusHeight, -info->wheels[3].mountPosition[2]);
    position[2] = bScalarAdd(mountHeight, info->centerOfMass[2]);
    float frame[16];
    rotationMatrixIdentity(frame);
    motionMatrixRotateYaw(frame, frame, heading);
    motionMatrixRotateX(frame, frame, xRotation);
    motionMatrixRotateY(orientation, frame, yRotation);
}

void carMovementUpdateTerrainPose(CarMovement* movement)
{
    VehicleState* state = movement->bindings.state;
    carMovementAlignTerrainPose(&movement->info, movement->bindings.environment->contact.terrain,
                                 state->orientation, state->position, state->heading);
    for (int axis = 0; axis < 4; ++axis)
        state->centerPosition[axis] = state->position[axis] - movement->info.centerOfMass[axis];
}



static const CarRaceMetric* findMetric(const CarMovementRace* race, const Car* car)
{
    for (size_t index = 0; index < race->metricCount; ++index)
        if (race->metrics[index].car == car) return &race->metrics[index];
    return NULL;
}

void carMovementUpdateRacePerformance(CarMovement* movement)
{
    const CarMovementRace* race = movement->bindings.race;
    const CarMovementPlayer* player = movement->bindings.player;
    if (!player || !race || !race->enabled || race->mode != 2) return;
    const CarRaceMetric* own = findMetric(race, movement->bindings.owner);
    const CarRaceMetric* other = findMetric(race, race->references[player->seat == 0 ? 0 : 1]);
    if (!own || !other || !race->performance) {
        movement->result = CAR_MOVEMENT_MISSING_RACE_METRIC;
        return;
    }
    if (!movement->bindings.route) {
        movement->result = CAR_MOVEMENT_MISSING_ROUTE;
        return;
    }
    const float first = own->position * movement->bindings.route->length;
    const float second = other->position * movement->bindings.route->length;
    movement->bindings.state->performanceScale = keyedCurveEvaluate(race->performance, first - second);
}

static int32_t tickDifference(int32_t now, int32_t start)
{
    const uint32_t bits = (uint32_t)now - (uint32_t)start;
    int32_t result; memcpy(&result, &bits, sizeof(result));
    return result;
}

float carMovementCapture(CarMovement* movement, float elapsed)
{
    if (movement->result != CAR_MOVEMENT_OK) return 0.0f;
    carMovementUpdateRacePerformance(movement);
    if (movement->result != CAR_MOVEMENT_OK) return 0.0f;
    float advanced;
    if (movement->mode == 1 && movement->physics) advanced = physicsCarMove(movement->physics, elapsed);
    else if (movement->mode == 2 && movement->traffic) advanced = trafficCarMove(movement->traffic,elapsed);
    else if (movement->mode == 3 && movement->delivery) advanced = deliveryMoverMove(movement->delivery,elapsed);
    else if (movement->mode == 5 && movement->cop) {
        const CarMovementRace* race = movement->bindings.race;
        if (!movement->frame || !race || !race->difficulty || *race->difficulty < 0 || *race->difficulty > 2) {
            movement->result = CAR_MOVEMENT_INVALID_BINDING; return 0.0f;
        }
        advanced = copMoverMove(movement->cop,elapsed);
    }
    else if (movement->mode == 6 && movement->belt) {
        if (movement->belt->path.count < 2 && movement->bindings.belt->command->count < 2) {
            movement->result = CAR_MOVEMENT_INVALID_BINDING;return 0.0f;
        }
        advanced = beltMoverMove(movement->belt,elapsed);
    }
    else if (movement->mode == 4 && movement->mellow) advanced = mellowMoverMove(movement->mellow, elapsed);
    else if (movement->mode == 0 && movement->inactive) advanced = nullMoverMove(movement->inactive, elapsed);
    else { movement->result = CAR_MOVEMENT_INVALID_BINDING; return 0.0f; }
    VehicleState* state = movement->bindings.state;
    CarMotionState* ownerMotion = movement->bindings.ownerMotion;
    carMovementSetPublishedGear(movement, state->gear);
    carMotionRecoverBelowGround(ownerMotion, state, movement->bindings.environment->contact.terrain,
                                 &movement->bindings.environment->recovery);
    if (ownerMotion->recoveryRequested) {
        const float first = state->orientation[0] * movement->routeLocation.direction[0];
        const float second = state->orientation[1] * movement->routeLocation.direction[1];
        const float dot = first + second;
        const float along = 0.0f < dot ? dot : 0.0f;
        const float forward = 0.0f < state->forwardSpeed ? state->forwardSpeed : 0.0f;
        const float speed = minimumFinite(15.0f, forward * along);
        const CarMovementDriver* driver = movement->bindings.driver;
        if (driver && driver->profile != 0) {
            const CarRecoveryGroup* group = NULL;
            const CarRecoveryPointRegistry* points = movement->bindings.recoveryPoints;
            if (points && driver->recoveryPose) {
                for (size_t index = 0; index < points->groupCount; ++index)
                    if (points->groups[index].id == driver->recoveryPose->groupId) {
                        group = &points->groups[index];
                        break;
                    }
            }
            carMovementRecover(movement, group, driver->recoveryPose, speed, 1.0f, 1, 0);
        }
        ownerMotion->recoveryRequested = 0;
    } else {
        const int32_t now = movement->bindings.environment->clock->worldTimer;
        if (carMotionWindowActive(ownerMotion, now)) {
            if (!movement->collisionParticipant || !ownerMotion->collision ||
                movement->collisionParticipant->carState!=ownerMotion->collision) {
                movement->result = CAR_MOVEMENT_INVALID_BINDING;
                return advanced;
            }
            const int32_t collision = ownerMotion->collision->suppressedCollisionTicks;
            if (collision && collision != INT32_MAX) {
                const float seconds = (float)tickDifference(now, ownerMotion->windowStartTicks) * 0.0002500000118743628f;
                if (ownerMotion->windowDurationSeconds - 0.5f < seconds) {
                    const float collisionSeconds = (float)tickDifference(now, collision) * 0.0002500000118743628f;
                    if (0.0f <= collisionSeconds && collisionSeconds < 0.5f) {
                        int32_t extension; physicsSecondsToTicks(&extension, 0.5f);
                        const uint32_t bits = (uint32_t)ownerMotion->windowStartTicks + (uint32_t)extension;
                        memcpy(&ownerMotion->windowStartTicks, &bits, sizeof(bits));
                    }
                }
            }
        }
    }
    return advanced;
}


const CarDriveDisplayTuning carDriveDisplayDefaultTuning = {
    {25.37999916f, 40.8899994f, 56.4700012f, 72.05999756f, 80.0f, 89.51000214f},
    {{1000,8500},{6500,8500},{6500,8500},{5500,9000},{6500,7200},{6600,7000},{0,0}},
    -2.0f, 1.0f, 50.0f, 0.0f, 100.0f, 0.00999999977648f
};

void carMovementUpdatePowertrainDisplay(CarMovement* movement, const CarDriveDisplayTuning* tuning)
{
    VehicleState* state = movement->bindings.state;
    float speed = tuning->speedOverride > 0.0f ? tuning->speedOverride : state->speed;
    if (tuning->speedThresholds[5] <= speed)
        speed = tuning->speedThresholds[5] - tuning->maximumSpeedMargin;
    speed = physicsClampDelta(movement->displaySpeed, speed, tuning->speedStep);
    movement->displaySpeed = speed;

    int32_t gear = state->gear;
    float rpm = 0.0f;
    if (speed < tuning->speedThresholds[0]) {
        gear = tuning->reverseThreshold <= speed ? 2 : 0;
        float magnitude = fabsf(speed);
        if (tuning->speedThresholds[0] < magnitude) magnitude = tuning->speedThresholds[0];
        const float fraction = magnitude / tuning->speedThresholds[0];
        rpm = tuning->rpmBands[0][0] + fraction * (tuning->rpmBands[0][1] - tuning->rpmBands[0][0]);
    } else {
        for (int32_t index = 1; index < 6; ++index) {
            if (speed < tuning->speedThresholds[index]) {
                gear = index + 2;
                if (gear >= 7) gear = 6;
                const float fraction = (speed - tuning->speedThresholds[index - 1]) /
                    (tuning->speedThresholds[index] - tuning->speedThresholds[index - 1]);
                const float* band = tuning->rpmBands[index + 1];
                rpm = band[0] + fraction * (band[1] - band[0]);
                break;
            }
        }
    }
    if (state->gear == gear) {
        const float previous = tuning->rpmOverride == 100.0f ? state->engineRpm : tuning->rpmOverride;
        rpm = physicsClampDelta(previous, rpm, tuning->rpmStep);
    }
    state->gear = gear;
    state->engineRpm = tuning->rpmOverride == 100.0f ? rpm : tuning->rpmOverride;
}


static const CurveKey speedDistanceKeys[] = {
    {0.0f, 0.0f}, {1.0f, 0.0f}, {5.0f, 10.0f}, {150.0f, 60.0f}
};
static const float distanceWeights[] = {
    1.0f, 0.9f, 0.75f, 0.55f, 0.3f, 0.0f
};

const CarInteractionTuning carInteractionDefaultTuning = {
    {speedDistanceKeys, 4},
    {distanceWeights, 6, 0.0f, 1.0f, 5.0f},
    0.95f, 19.999996185302734375f, 0.9f
};

/* A saturated intermediate still participates in the following operation.
 * In particular coincident positions must not turn a zero curve value into
 * an infinity-times-zero NaN. */
static float boundedProduct(float first, float second)
{
    const float product = first * second;
    return isinf(product) ? copysignf(FLT_MAX, product) : product;
}

float carInteractionPair(const CarMovement* car, const CarMovement* other,
                         const CarInteractionTuning* tuning)
{
    const VehicleState* otherState = other->bindings.state;
    if (otherState->speed < tuning->speedDistance.keys[1].coordinate)
        return 0.0f;

    float difference[3];
    for (int lane = 0; lane < 3; ++lane)
        difference[lane] = otherState->position[lane] - car->bindings.state->position[lane];
    const float distanceSquared = (difference[0] * difference[0] +
                                   difference[1] * difference[1]) +
                                  difference[2] * difference[2];
    const float distance = sqrtf(distanceSquared);
    if (tuning->speedDistance.keys[tuning->speedDistance.count - 1].value < distance)
        return 0.0f;

    const float* velocity = otherState->velocity;
    const float velocitySquared = (velocity[0] * velocity[0] + velocity[1] * velocity[1]) +
                                  velocity[2] * velocity[2];
    const float inverseSpeed = finiteRatio(1.0f, sqrtf(velocitySquared));
    float direction[3];
    for (int lane = 0; lane < 3; ++lane)
        direction[lane] = velocity[lane] * inverseSpeed;
    const float projection = (difference[0] * direction[0] + difference[1] * direction[1]) +
                             difference[2] * direction[2];
    const float alignment = boundedProduct(finiteRatio(projection, distance) -
                                            tuning->alignmentOffset,
                                            tuning->alignmentScale);
    if (alignment < 0.0f)
        return 0.0f;

    const float speedDistance = keyedCurveEvaluate(&tuning->speedDistance, otherState->speed);
    const float effectiveDistance = boundedProduct(alignment, speedDistance);
    return sampledCurveEvaluate(&tuning->distanceWeight,
                                 finiteRatio(distance, effectiveDistance));
}

CarInteractionResult carInteractionFactor(const CarInteractionRegistry* registry,
                                           const CarMovement* car, float* factor)
{
    if (!registry || !registry->world || !registry->frames || !registry->tuning ||
        !car || !factor || registry->world->totalCars < 0 ||
        (size_t)registry->world->totalCars > registry->frameCount)
        return CAR_INTERACTION_INVALID_BINDING;

    float sum = 0.0f;
    for (int32_t index = 0; index < registry->world->totalCars; ++index) {
        CarMovement* other = carFrameMovement(registry->frames[index]);
        if (!other || other->bindings.owner != registry->world->carTable[index])
            return CAR_INTERACTION_INVALID_BINDING;
        if (other != car)
            sum = sum + carInteractionPair(car, other, registry->tuning);
        if (registry->tuning->accumulationLimit <= sum)
            break;
    }
    *factor = minimumFinite(sum, registry->tuning->accumulationLimit);
    return CAR_INTERACTION_OK;
}

float driverObservedAcceleration(const DriverPhysicsVehicle* vehicle)
{
    const VehicleState* state = vehicle->movement->bindings.state;
    const float* acceleration = state->linearAcceleration;
    if (state->speed < vehicle->tuning->accelerationSpeedThreshold) {
        const float xx = acceleration[0] * acceleration[0];
        const float yy = acceleration[1] * acceleration[1];
        const float zz = acceleration[2] * acceleration[2];
        return sqrtf((xx + yy) + zz);
    }
    const float* velocity = state->velocity;
    const float xx = velocity[0] * velocity[0];
    const float yy = velocity[1] * velocity[1];
    const float zz = velocity[2] * velocity[2];
    const float inverseLength = finiteRatio(1.0f, sqrtf((xx + yy) + zz));
    const float x = (velocity[0] * inverseLength) * acceleration[0];
    const float y = (velocity[1] * inverseLength) * acceleration[1];
    const float z = (velocity[2] * inverseLength) * acceleration[2];
    return (x + y) + z;
}


CarFrame* carFrameCreate(CarMovement* movement, const CarFrameEnvironment* environment,
                         CarFrameResult* result)
{
    if (result) *result = CAR_FRAME_INVALID_BINDING;
    if (!movement || !movement->bindings.owner || !movement->bindings.state ||
        !movement->bindings.state->position || !environment || !environment->world ||
        !environment->clock || !movement->bindings.environment ||
        !movement->bindings.environment->contact.terrain)
        return NULL;
    CarFrame* frame = (CarFrame*)calloc(1, sizeof(*frame));
    if (!frame) {
        if (result) *result = CAR_FRAME_ALLOCATION_FAILURE;
        return NULL;
    }
    frame->movement = movement;
    frame->environment = *environment;
    movement->bindings.owner->driverVisible = 1;
    memcpy(frame->state.previousPosition, movement->bindings.state->position, sizeof(float) * 4);
    movement->bindings.owner->interactionContext = frame;
    movement->bindings.owner->interaction = carFrameInteractionFactor;
    movement->frame = frame;
    frame->result = CAR_FRAME_OK;
    if (result) *result = CAR_FRAME_OK;
    return frame;
}

void carFrameDispose(CarFrame* frame)
{
    if (frame && frame->movement && frame->movement->bindings.owner) {
        Car* owner = frame->movement->bindings.owner;
        if (owner->interactionContext == frame && owner->interaction == carFrameInteractionFactor) {
            owner->interactionContext = NULL;
            owner->interaction = NULL;
        }
        if (frame->movement->frame == frame) frame->movement->frame = NULL;
    }
    free(frame);
}
CarMovement* carFrameMovement(CarFrame* frame) { return frame ? frame->movement : NULL; }
CarFrameState* carFrameState(CarFrame* frame) { return frame ? &frame->state : NULL; }
void carFrameSetSpecialEffect(CarFrame* frame, int32_t requested)
{
    frame->state.specialEffectRequested = requested;
}
CarFrameResult carFrameGetResult(const CarFrame* frame)
{ return frame ? frame->result : CAR_FRAME_INVALID_BINDING; }

CarFrameResult carFramePreStep(CarFrame* frame, float elapsed)
{
    if (!frame) return CAR_FRAME_INVALID_BINDING;
    VehicleState* state = frame->movement->bindings.state;
    if (0.0f < elapsed) {
        memcpy(frame->state.previousPosition, state->position, sizeof(float) * 4);
        memcpy(frame->state.previousVelocity, state->velocity, sizeof(float) * 4);
        memcpy(frame->state.previousAngularVelocity, state->angularVelocity, sizeof(float) * 4);
    }
    // PAL131D10..131D50: VU dot followed by scalar SQRT/MUL/ADD.
    const float speedSquared = bScalarAdd(bScalarAdd(
        bScalarMultiply(state->velocity[0], state->velocity[0]),
        bScalarMultiply(state->velocity[1], state->velocity[1])),
        bScalarMultiply(state->velocity[2], state->velocity[2]));
    const float radius = bScalarAdd(bScalarMultiply(bScalarSquareRoot(speedSquared), 0.05f), 3.0f);
    TerrainSurface* terrain = frame->movement->bindings.environment->contact.terrain;
    terrainSurfaceUpdate(terrain, state->position, radius, state->velocity);
    if (terrainSurfaceHasGround(terrain, state->position) && state->activeContactCount >= 2) {
        frame->state.lastGroundPoint[0] = state->position[0];
        frame->state.lastGroundPoint[1] = state->position[1];
    }
    carFrameResetPeakMaterials(state);
    frame->result = CAR_FRAME_OK;
    return frame->result;
}

float carFrameInteractionFactor(void* context, Car* car)
{
    CarFrame* frame = (CarFrame*)context;
    if (!frame || frame->movement->bindings.owner != car) {
        if (frame) frame->result = CAR_FRAME_INVALID_BINDING;
        return 0.0f;
    }
    CarInteractionRegistry registry = {
        frame->environment.world, frame->environment.frames, frame->environment.frameCount,
        frame->environment.interactionTuning ? frame->environment.interactionTuning :
                                             &carInteractionDefaultTuning
    };
    float factor = 0.0f;
    frame->result = carInteractionFactor(&registry, frame->movement, &factor) == CAR_INTERACTION_OK ?
                    CAR_FRAME_OK : CAR_FRAME_INVALID_BINDING;
    return factor;
}

float carFramePredictGroundClearance(CarFrame* frame, float* duration, float* maximumClearance)
{
    CarMovement* movement = frame->movement;
    VehicleState* state = movement->bindings.state;
    const PhysicsCarInfo* info = movement->bindings.info;
    TerrainSurface* terrain = movement->bindings.environment->contact.terrain;
    // AverageAir PAL135B30 preserves scalar and VU operation boundaries.
    const float downforce = bScalarAdd(info->aerodynamics->frontDownforce, info->aerodynamics->rearDownforce);
    float downwardAcceleration = bScalarMultiply(state->speed, bScalarMultiply(downforce, 0.3f));
    downwardAcceleration = bScalarAdd(bScalarDivide(downwardAcceleration, info->body->mass), 9.8f);
    float worldFrame[16];
    memcpy(worldFrame, state->orientation, sizeof(worldFrame));
    memcpy(worldFrame + 12, state->position, sizeof(float) * 4);
    worldFrame[15] = 1.0f;
    float average[4] = {0, 0, 0, 0};
    for (int wheel = 0; wheel < 4; ++wheel) {
        // PAL135C10..135C40 samples the bottom of the wheel using the
        // authored radius (+12C), not the mount's local Z (+128).
        float point[4] = {0, 0, -info->wheels[wheel].reactionScale, 0};
        vectorMatrixTransformPoint(point, state->wheelVisualTransform[wheel], point);
        vectorMatrixTransformPoint(point, worldFrame, point);
        for (int lane = 0; lane < 3; ++lane)
            average[lane] = bScalarAdd(bScalarMultiply(point[lane], 0.25f), average[lane]);
    }
    *maximumClearance = 0.0f;
    const float initialGround = terrainSurfaceElevation(terrain, average, NULL, NULL, NULL);
    float sum = maximumFinite(0.0f, bScalarAdd(average[2], -initialGround));
    const int32_t samples = convertFloatToInt32(bScalarMultiply(bScalarMultiply(state->speed, *duration), 0.3333333432674408f));
    const float sampleDivisor = (float)samples;
    int32_t index = 1;
    for (; index < samples; ++index) {
        const float time = bScalarDivide(bScalarMultiply(*duration, (float)index), sampleDivisor);
        float point[4];
        for (int lane = 0; lane < 3; ++lane) {
            const float acceleration = lane == 2 ? -downwardAcceleration : 0.0f;
            const float velocity = bScalarAdd(bScalarMultiply(acceleration, time), state->velocity[lane]);
            point[lane] = bScalarAdd(bScalarMultiply(velocity, time), average[lane]);
        }
        point[3] = 0.0f;
        const float ground = terrainSurfaceElevation(terrain, point, NULL, NULL, NULL);
        const float clearance = bScalarAdd(point[2], -ground);
        if (*maximumClearance < clearance) *maximumClearance = clearance;
        if (!(0.0f < clearance)) {
            *duration = time;
            ++index;
            break;
        }
        sum = bScalarAdd(sum, clearance);
    }
    return index == 1 ? sum : bScalarAdd(bScalarDivide(sum, (float)index), -1.0f);
}


static CarFrameResult carFrameQueueSurfaceFeedback(CarFrame* frame, int32_t code, float value)
{
    CarFramePresentationEvent* event = carFramePresentationAppend(frame->environment.presentation,
        CAR_FRAME_CONTROLLER_EFFECT, frame->movement, code);
    if (!event) return frame->result = CAR_FRAME_EVENT_CAPACITY;
    event->port = frame->movement->bindings.player->controllerPort;
    event->value[0] = value;
    return CAR_FRAME_OK;
}

CarFrameResult carFrameUpdateAccelerationFeedback(CarFrame* frame)
{
    if (!frame) return CAR_FRAME_INVALID_BINDING;
    if (!frame->movement->bindings.player) return CAR_FRAME_OK;
    CarFramePresentationEvent* event = carFramePresentationAppend(frame->environment.presentation,
        CAR_FRAME_ACCELERATION_FEEDBACK, frame->movement, 0);
    if (!event) return frame->result = CAR_FRAME_EVENT_CAPACITY;
    event->port = frame->movement->bindings.player->seat;
    memcpy(event->value, frame->movement->bindings.state->linearAcceleration, sizeof(event->value));
    return CAR_FRAME_OK;
}

CarFrameResult carFrameUpdateSurfaceFeedback(CarFrame* frame)
{
    static const CurveKey displacementKeys[2] = {{1.388888955116272f, 0.0f}, {13.88888931274414f, 1.0f}};
    static const KeyedCurve displacementCurve = {displacementKeys, 2};
    if (!frame) return CAR_FRAME_INVALID_BINDING;
    if (!frame->movement->bindings.player) return CAR_FRAME_OK;
    if (frame->environment.carReplay && frame->environment.carReplay->restoreMode) return CAR_FRAME_OK;
    VehicleState* state = frame->movement->bindings.state;
    float load = 0.0f;
    for (int wheel = 0; wheel < 4; ++wheel) load = bScalarAdd(load, state->wheelNormalLoad[wheel]);
    if (carFrameQueueSurfaceFeedback(frame, 0, bScalarDivide(bScalarMultiply(load, 0.25f), frame->movement->bindings.info->body->mass)) != CAR_FRAME_OK)
        return frame->result;
    float displacement = 0.0f;
    for (int wheel = 0; wheel < 4; ++wheel) displacement = bScalarAdd(displacement, state->wheelSurfaceOffset[wheel]);
    const float speedScale = keyedCurveEvaluate(&displacementCurve, state->speed);
    if (carFrameQueueSurfaceFeedback(frame, 1, bScalarMultiply(displacement, speedScale)) != CAR_FRAME_OK) return frame->result;
    float impulse = 0.0f;
    for (int wheel = 0; wheel < 4; ++wheel) impulse = bScalarAdd(impulse, state->suspensionImpulse[wheel]);
    if (carFrameQueueSurfaceFeedback(frame, 2, bScalarMultiply(impulse, 0.25f)) != CAR_FRAME_OK) return frame->result;
    if (frame->state.disabledWheelCount > 0) {
        CarFramePlayerState* player = frame->environment.player;
        if (!player || !player->replay) return frame->result = CAR_FRAME_MISSING_PLAYER_OWNER;
        if (!carFramePlayerReplayActive(player)) {
            int32_t contact = 0;
            for (int wheel = 0; wheel < 4; ++wheel)
                if (state->wheelContact[wheel] &&
                    ((frame->movement->bindings.ownerMotion->disabledWheelMask >> wheel) & 1u)) { contact = 1; break; }
            const int32_t ticks = frame->state.disabledWheelStartTicks;
            if (contact && ticks && ticks != INT_MAX) {
                const int32_t age = (int32_t)((uint32_t)frame->environment.clock->worldTimer - (uint32_t)ticks);
                const float strength = bScalarAdd(1.0f, -bScalarMultiply(
                    bScalarMultiply((float)age, 0.0002500000118743628f), 0.1428571492433548f));
                if (0.0f < strength) {
                    const float x = bScalarMultiply(state->velocity[0], state->velocity[0]);
                    const float y = bScalarMultiply(state->velocity[1], state->velocity[1]);
                    const float z = bScalarMultiply(state->velocity[2], state->velocity[2]);
                    const float speed = bScalarDivide(bScalarSquareRoot(bScalarAdd(bScalarAdd(x, y), z)), (float)frame->state.disabledWheelCount);
                    if (carFrameQueueSurfaceFeedback(frame, 3, bScalarMultiply(speed, strength)) != CAR_FRAME_OK) return frame->result;
                }
            }
        }
    }
    uint8_t mask = 0;
    for (int wheel = 0; wheel < 4; ++wheel)
        if (state->peakMaterial[wheel] == 30 || state->peakMaterial[wheel] == 31)
            mask = (uint8_t)(mask | (uint8_t)(1u << wheel));
    const uint8_t combined = (uint8_t)(mask | frame->state.previousSurfaceMask);
    if (combined != frame->state.previousSurfaceMask) {
        if (13.0f < state->speed) {
            if ((combined & 3u) != (frame->state.previousSurfaceMask & 3u) &&
                carFrameQueueSurfaceFeedback(frame, 5, 1.0f) != CAR_FRAME_OK) return frame->result;
        } else if (5.0f < state->speed && carFrameQueueSurfaceFeedback(frame, 4, 1.0f) != CAR_FRAME_OK) return frame->result;
    }
    frame->state.previousSurfaceMask = mask;
    return CAR_FRAME_OK;
}

CarFrameResult carFrameDisableWheels(CarFrame* frame, uint8_t mask)
{
    if (!frame || !frame->movement || !frame->movement->bindings.ownerMotion ||
        !frame->environment.clock || !frame->environment.presentation)
        return CAR_FRAME_INVALID_BINDING;
    CarMotionState* motion = frame->movement->bindings.ownerMotion;
    CarFramePresentation* presentation = frame->environment.presentation;
    // The original successful ordering is unchanged: publish the wheel events
    // before the tire audio. Capacity failure must not publish only some of the
    // wheel events while leaving the entire car punctured. The audio-disabled
    // native owner has no CarSoundState and needs no tire sound refresh.
    const uint8_t requested = mask;
    size_t eventCount = 0;
    for (int wheel = 0; wheel < 4; ++wheel)
        if ((requested >> wheel) & 1u) ++eventCount;
    if (presentation->count > presentation->capacity ||
        (eventCount && (!presentation->events ||
            eventCount > presentation->capacity - presentation->count))) {
        presentation->failed = 1;
        return frame->result = CAR_FRAME_EVENT_CAPACITY;
    }
    const uint8_t oldMask = motion->disabledWheelMask;
    const int8_t oldCount = frame->state.disabledWheelCount;
    const int32_t oldTicks = frame->state.disabledWheelStartTicks;
    const size_t oldPresentationCount = presentation->count;
    motion->disabledWheelMask = static_cast<uint8_t>(oldMask | requested);
    frame->state.disabledWheelCount = 0;
    frame->state.disabledWheelStartTicks = frame->environment.clock->worldTimer;
    for (int wheel = 0; wheel < 4; ++wheel) {
        if (!((motion->disabledWheelMask >> wheel) & 1u)) continue;
        ++frame->state.disabledWheelCount;
        if ((requested >> wheel) & 1u) {
            CarFramePresentationEvent* event = carFramePresentationAppend(presentation,
                CAR_FRAME_SURFACE_EFFECT, frame->movement, 8);
            if (!event) {
                motion->disabledWheelMask = oldMask;
                frame->state.disabledWheelCount = oldCount;
                frame->state.disabledWheelStartTicks = oldTicks;
                presentation->count = oldPresentationCount;
                return frame->result = CAR_FRAME_EVENT_CAPACITY;
            }
            event->argument = (wheel + 5) * 65536;
            event->value[0] = 1.0f;
        }
    }
    if (frame->environment.sound && !carSoundRefreshEffects(frame->environment.sound)) {
        motion->disabledWheelMask = oldMask;
        frame->state.disabledWheelCount = oldCount;
        frame->state.disabledWheelStartTicks = oldTicks;
        presentation->count = oldPresentationCount;
        return frame->result = CAR_FRAME_EVENT_CAPACITY;
    }
    return frame->result = CAR_FRAME_OK;
}

void carFrameClearDisabledWheels(CarFrame* frame)
{
    frame->state.disabledWheelStartTicks = 0;
    frame->movement->bindings.ownerMotion->disabledWheelMask = 0;
    frame->state.disabledWheelCount = 0;
}

void carFrameBeginBlink(CarFrame* frame, int32_t visual, int32_t control, int32_t phase,
                         float interval, float offset, float duration)
{
    const float elapsed = bScalarMultiply((float)(int32_t)((uint32_t)frame->environment.clock->worldTimer -
                                           (uint32_t)frame->state.blinkStart), 0.0002500000118743628f);
    if (bScalarAdd(bScalarAdd(frame->state.blinkInterval, frame->state.blinkOffset), frame->state.blinkDuration) < elapsed) {
        frame->state.blinkPhase = phase;
        frame->state.blinkStart = frame->environment.clock->worldTimer;
        frame->state.blinkInterval = interval;
        frame->state.blinkOffset = offset;
        frame->state.blinkDuration = duration;
        frame->state.blinkVisualEnabled = visual;
        frame->state.blinkControlEnabled = control;
    }
}

float physicsMaximum(float a, float b) { return maximumFinite(a, b); }

float physicsMinimum(float a, float b) { return minimumFinite(a, b); }

float physicsClamp(float value, float lower, float upper)
{
    return physicsMinimum(physicsMaximum(value, lower), upper);
}

float physicsClampDelta(float center, float value, float delta)
{
    const float ceiling = center + delta;
    if (ceiling < value) return ceiling;
    const float floor = center - delta;
    if (value < floor) return floor;
    return value;
}

void physicsSecondsToTicks(int32_t* ticks, float seconds)
{
    *ticks = convertFloatToInt32(seconds * 4000.0f + 0.5f);
}
