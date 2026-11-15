#include "entity_state_bridge.hpp"

#include "indep/inc/CarState.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/WorldObject.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace hp2 {

namespace {
template<class Range>
bool finite(const Range& values) {
    return std::all_of(values.begin(), values.end(), [](float value) { return std::isfinite(value); });
}
RaceCameraState capture_camera(const CarCinematicViewFrame& camera, float sourceRatio) {
    RaceCameraState result;
    std::copy_n(camera.eye, 3, result.eye.begin());
    std::copy_n(camera.forward, 3, result.forward.begin());
    result.up = {camera.matrix[1], camera.matrix[5], camera.matrix[9]};
    result.verticalFov = 2.0f * std::atan(
        std::tan(camera.fieldOfView * (3.141592654f / 65536.0f)) * sourceRatio) * 57.2957795f;
    result.nearClip = camera.nearClip;
    result.farClip = camera.farClip;
    if (!finite(result.eye) || !finite(result.forward) || !finite(result.up) ||
        !(result.verticalFov > 0 && result.verticalFov < 180 &&
          camera.nearClip > 0 && camera.farClip > camera.nearClip && std::isfinite(camera.farClip)))
        throw std::runtime_error("Race camera has an invalid committed projection");
    return result;
}

}

RaceCameraState source_main_camera(const CarCinematicViewFrame& camera, bool ntsc) {
    const float sourceRatio = ntsc ? 448.0f / (512.0f * 1.0714286566f) : 1.0f / 1.285f;
    return capture_camera(camera, sourceRatio);
}

const EntityFrame& EntityStateBridge::stage(const RaceStateView& race) {
    auto& frame = pending_;
    frame.vehicles.clear();
    const auto* simulation = race.simulation;
    const auto* world = simulation ? simulation->world : nullptr;
    if (!world || !race.camera)
        throw std::runtime_error("Committed race is missing World or primary camera");

    frame.camera = source_main_camera(*race.camera, race.ntsc);
    frame.rearCamera.reset();
    if (race.rearCamera)
        frame.rearCamera = capture_camera(*race.rearCamera, 64.0f / (512.0f * 1.08f));

    frame.step = world->frameCounter;
    frame.generation = world->presentationGeneration;
    frame.replayEpoch = world->presentationReplayEpoch;
    frame.track = race.track;
    frame.reverse = race.reverse;
    const auto* player = simulation->players ? simulation->players->playersByNumber[0] : nullptr;
    if (!frame.generation || !player || world->totalCars < 1 || world->totalCars > WORLD_MAX_CARS)
        throw std::runtime_error("Committed World has an invalid car table or no player");
    frame.vehicles.reserve(world->totalCars);
    for (int slot = 0; slot < world->totalCars; ++slot) {
        auto* car = world->carTable[slot];
        auto* movement = car ? car->Movement() : nullptr;
        if (!movement || !movement->bindings.state || !movement->bindings.ownerMotion || !car->driverInfo)
            throw std::runtime_error("Committed World car has incomplete bindings at slot " + std::to_string(slot));
        const auto& state = *movement->bindings.state;
        const auto& appearance = car->driverInfo->appearance;
        const auto handle = car->presentationId;
        if (!handle || std::any_of(frame.vehicles.begin(), frame.vehicles.end(),
            [handle](const VehicleEntityState& vehicle) { return vehicle.handle == handle; }))
            throw std::runtime_error("Committed World car has an invalid or duplicate handle");
        auto& entity = frame.vehicles.emplace_back();
        entity.handle = handle;
        entity.model = appearance.modelCode;
        entity.skin = appearance.skinNumber;
        entity.player = player->car == movement;
        const auto& motion = *movement->bindings.ownerMotion;
        const bool visible = motion.actionEnabled &&
            !carMotionTransitionFlag(&motion) && (motion.wheelEffectsFlags & 2u);
        entity.visible = visible && car != race.hiddenCar;
        entity.exteriorVisible = visible;
        entity.rearVisible = visible && car != race.rearHiddenCar;
        const bool replayPose = (motion.wheelExclusionMask & 2u) != 0;
        const auto* presentation = replayPose && race.viewMovers ?
            carViewMoverPresentation(race.viewMovers, car) : nullptr;
        if (replayPose && (!presentation || presentation->motion != &motion))
            throw std::runtime_error("NIS vehicle has no committed replay pose");
        eMulMatrix(entity.transform.data(), CarScaleMatrix, presentation ? presentation->orientation : state.orientation);
        std::copy_n(presentation ? presentation->centerPosition : state.centerPosition,
            3, entity.transform.begin() + 12);
        entity.transform[15] = 1;
        entity.speed = state.speed;
        if (!finite(entity.transform) || !std::isfinite(entity.speed))
            throw std::runtime_error("Nonfinite vehicle state for handle " + std::to_string(entity.handle));

        float wheels[4][16], brakes[4][16];
        car->GetRenderingWheelAndBrakeMatrices(wheels, brakes, 1);
        float flipped[16];
        motionMatrixRotateYaw(flipped, eMathIdentityMatrix, 0x8000u);
        for (int wheel = 0; wheel < 4; ++wheel) {
            auto& pose = entity.wheels[wheel];
            if (wheel == 0 || wheel == 3) eMulMatrix(pose.data(), flipped, wheels[wheel]);
            else std::copy_n(wheels[wheel], 16, pose.begin());
            if (!finite(pose))
                throw std::runtime_error("Nonfinite wheel pose for handle " + std::to_string(entity.handle));
        }
        // Publish the physical blur pose once. The active Godot view and the
        // depth buffer determine which rims are visible, including in orbit.
        for (std::size_t wheel = 0; wheel < entity.wheelBlur.size(); ++wheel) {
            auto& blur = entity.wheelBlur[wheel];
            blur.alpha = std::clamp(std::fabs(state.wheelAngularVelocity[wheel]) * 0.02f, 0.0f, 1.0f) * 90.0f;
            if (wheel == 0 || wheel == 3) eMulMatrix(blur.transform.data(), flipped, brakes[wheel]);
            else std::copy_n(brakes[wheel], 16, blur.transform.begin());
            if (!finite(blur.transform) || !std::isfinite(blur.alpha))
                throw std::runtime_error("Invalid committed wheel blur state");
        }
    }
    if (std::count_if(frame.vehicles.begin(), frame.vehicles.end(),
            [](const VehicleEntityState& vehicle) { return vehicle.player; }) != 1)
        throw std::runtime_error("Committed race must have exactly one player");
    // PAL1CDB20 traverses the committed live list. Keep hidden pool entries
    // in the snapshot so blinking and parked objects retain their instances.
    const auto& objects = WorldObjects(*simulation->world);
    frame.worldObjects.clear();
    frame.worldObjects.reserve(objects.count);
    for (auto* object = objects.first; object; object = object->next) {
        if (frame.worldObjects.size() == objects.count || !object->occupied || !object->presentationId)
            throw std::runtime_error("Invalid WorldObject presentation registry");
        auto& entity = frame.worldObjects.emplace_back();
        entity.handle = object->presentationId;
        entity.model = &object->model;
        entity.visible = object->borrowedEnabled ? *object->borrowedEnabled : object->hasFrame;
        if (entity.visible) {
            const auto* transform = object->borrowedFrame ? object->borrowedFrame : object->frame;
            std::copy_n(transform, 16, entity.transform.begin());
            if (!finite(entity.transform)) throw std::runtime_error("Nonfinite WorldObject transform");
        }
    }
    if (frame.worldObjects.size() != objects.count)
        throw std::runtime_error("WorldObject list does not match its committed count");
    return frame;
}

void EntityStateBridge::publish() noexcept {
    // The draw loop already uploaded or reused each borrowed model.
    pending_.worldObjects.clear();
    std::swap(current_, pending_);
}

}
