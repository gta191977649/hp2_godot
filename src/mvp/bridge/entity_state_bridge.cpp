#include "entity_state_bridge.hpp"

#include "indep/inc/CarState.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/eMath.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace hp2 {

namespace {
template<class Range>
bool finite(const Range& values) {
    return std::all_of(values.begin(), values.end(), [](float value) { return std::isfinite(value); });
}
}

EntityFrame EntityStateBridge::stage(const RaceStateView& race) const {
    EntityFrame frame;
    const auto* simulation = race.simulation;
    const auto* world = simulation ? simulation->world : nullptr;
    if (!world || !race.camera)
        throw std::runtime_error("Committed race is missing World or primary camera");

    const auto& camera = *race.camera;
    std::copy_n(camera.eye, 3, frame.camera.eye.begin());
    std::copy_n(camera.forward, 3, frame.camera.forward.begin());
    frame.camera.up = {camera.matrix[1], camera.matrix[5], camera.matrix[9]};
    // Ecstasy primary view preserves authored vertical framing on square PC pixels.
    const float sourceRatio = race.ntsc ? 448.0f / (512.0f * 1.0714286566f) : 1.0f / 1.285f;
    frame.camera.verticalFov = 2.0f * std::atan(
        std::tan(camera.fieldOfView * (3.141592654f / 65536.0f)) * sourceRatio) * 57.2957795f;
    frame.camera.nearClip = camera.nearClip;
    frame.camera.farClip = camera.farClip;
    if (!finite(frame.camera.eye) || !finite(frame.camera.forward) || !finite(frame.camera.up) ||
        !(frame.camera.verticalFov > 0 && frame.camera.verticalFov < 180 &&
          camera.nearClip > 0 && camera.farClip > camera.nearClip && std::isfinite(camera.farClip)))
        throw std::runtime_error("Primary camera has an invalid committed projection");

    frame.step = world->frameCounter;
    frame.generation = world->presentationGeneration;
    frame.track = race.track;
    const auto* player = simulation->players ? simulation->players->playersByNumber[0] : nullptr;
    if (!frame.generation || !player || world->totalCars < 1 || world->totalCars > WORLD_MAX_CARS)
        throw std::runtime_error("Committed World has an invalid car table or no player");
    frame.vehicles.reserve(world->totalCars);
    std::unordered_set<std::uint64_t> handles;
    for (int slot = 0; slot < world->totalCars; ++slot) {
        auto* car = world->carTable[slot];
        auto* movement = car ? car->Movement() : nullptr;
        if (!movement || !movement->bindings.state || !movement->bindings.ownerMotion || !car->driverInfo)
            throw std::runtime_error("Committed World car has incomplete bindings at slot " + std::to_string(slot));
        const auto& state = *movement->bindings.state;
        const auto& appearance = car->driverInfo->appearance;
        auto& entity = frame.vehicles.emplace_back();
        entity.handle = car->presentationId;
        if (!entity.handle || !handles.insert(entity.handle).second)
            throw std::runtime_error("Committed World car has an invalid or duplicate handle");
        entity.model = appearance.modelCode;
        entity.skin = appearance.skinNumber;
        entity.player = player->car == movement;
        const auto& motion = *movement->bindings.ownerMotion;
        entity.visible = car != race.hiddenCar && motion.actionEnabled &&
            !carMotionTransitionFlag(&motion) && (motion.wheelEffectsFlags & 2u);
        eMulMatrix(entity.transform.data(), CarScaleMatrix, state.orientation);
        std::copy_n(state.centerPosition, 3, entity.transform.begin() + 12);
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
    }
    if (std::count_if(frame.vehicles.begin(), frame.vehicles.end(),
            [](const VehicleEntityState& vehicle) { return vehicle.player; }) != 1)
        throw std::runtime_error("Committed race must have exactly one player");
    return frame;
}

void EntityStateBridge::publish(EntityFrame frame) noexcept {
    current_ = std::move(frame);
}

}
