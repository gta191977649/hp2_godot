#include <exception>
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/Car.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/eMath.hpp"

#include <chrono>
#include <cmath>
#include <cstring>

namespace {

float particleRandomUnit(uint32_t& state)
{
    state ^= state << 17;
    state ^= state >> 13;
    state ^= state << 5;
    return static_cast<float>(state & 0x7fffffffu) * 4.656612873077393e-10f;
}

float cycleDuration(ChopperEffectInstance& emitter, const WorldParticleCycle& cycle)
{
    if (cycle.duration == 0.0f) return 0.0f;
    const float variation = cycle.duration * cycle.variation;
    const float minimum = cycle.duration - variation;
    const float randomPart = particleRandomUnit(emitter.randomState) * variation;
    return minimum + randomPart;
}

void appendEmitter(WorldParticleSystem& system, ChopperEffectInstance& emitter, bool atFront)
{
    emitter.particleSystem = &system;
    emitter.autoPrevious = atFront ? nullptr : system.lastEmitter;
    emitter.autoNext = atFront ? system.firstEmitter : nullptr;
    if (emitter.autoPrevious) emitter.autoPrevious->autoNext = &emitter;
    else system.firstEmitter = &emitter;
    if (emitter.autoNext) emitter.autoNext->autoPrevious = &emitter;
    else system.lastEmitter = &emitter;
    ++system.emitterCount;
}

void removeParticle(WorldParticleSystem& system, WorldParticleGroup& group, WorldParticle& particle)
{
    if (particle.previous) particle.previous->next = particle.next;
    else group.first = particle.next;
    if (particle.next) particle.next->previous = particle.previous;
    else group.last = particle.previous;
    particle.occupied = 0;
    particle.next = particle.previous = nullptr;
    --system.particleCount;
}

int16_t wrapHalf(int32_t value)
{
    const uint16_t bits = static_cast<uint16_t>(value);
    int16_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

int8_t wrapByte(int32_t value)
{
    const uint8_t bits = static_cast<uint8_t>(value);
    int8_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

float dragAcceleration(float velocity, float drag)
{
    const float square = velocity * velocity;
    const float signedDrag = (velocity < 0.0f ? 1.0f : -1.0f) * drag;
    return signedDrag * square;
}

void advanceParticle(WorldParticle& particle, const WorldParticleDefinition& definition,
                     const CarMovement* car, float elapsed, int32_t ticks)
{
    const int32_t previousTicks = particle.remainingTicks;
    const int32_t remainingTicks = previousTicks - ticks;
    if (definition.animationRate != 0) {
        const float periodValue = 1024.0f * (1.0f / static_cast<float>(definition.animationRate));
        const int32_t period = convertFloatToInt32(periodValue);
        const uint32_t cells = (1u << static_cast<uint8_t>(definition.atlasWidthPower)) <<
            static_cast<uint8_t>(definition.atlasHeightPower);
        const int32_t frames = previousTicks / period - remainingTicks / period;
        particle.atlasFrame = static_cast<uint8_t>((static_cast<uint32_t>(particle.atlasFrame) +
            static_cast<uint32_t>(frames)) & (cells - 1u));
    }
    const float magnitude = static_cast<float>(particle.speed) * 0.0078125f;
    const float directionScale = magnitude * 0.007874015718698502f;
    const float oldX = static_cast<float>(particle.direction[0]) * directionScale;
    const float oldY = static_cast<float>(particle.direction[1]) * directionScale;
    const float oldZ = static_cast<float>(particle.direction[2]) * directionScale;
    const float deltaX = dragAcceleration(oldX, definition.drag) * elapsed;
    const float deltaY = dragAcceleration(oldY, definition.drag) * elapsed;
    const float deltaZ = (-definition.gravity + dragAcceleration(oldZ, definition.drag)) * elapsed;
    const float x = oldX + deltaX;
    const float y = oldY + deltaY;
    const float z = oldZ + deltaZ;
    particle.position[0] = particle.position[0] + x * elapsed;
    particle.position[1] = particle.position[1] + y * elapsed;
    particle.position[2] = particle.position[2] + z * elapsed;
    if (definition.followsCar && car && definition.carMotionScale > 0.0f) {
        const float influence = static_cast<float>(particle.carMotionScale) * 0.003921568859368563f;
        const VehicleState& state = *car->bindings.state;
        const float followX = elapsed * state.velocity[0];
        const float followY = elapsed * state.velocity[1];
        const float followZ = elapsed * state.velocity[2];
        particle.position[0] = particle.position[0] + followX * influence;
        particle.position[1] = particle.position[1] + followY * influence;
        particle.position[2] = particle.position[2] + followZ * influence;
    }
    const float speed = std::sqrt((x * x + y * y) + z * z);
    const float reciprocal = finiteRatio(1.0f, speed);
    particle.speed = static_cast<uint16_t>(convertFloatToInt32(speed * 128.0f));
    particle.remainingTicks = wrapHalf(remainingTicks);
    const float normalX = x * reciprocal;
    const float normalY = y * reciprocal;
    const float normalZ = z * reciprocal;
    particle.direction[0] = wrapByte(convertFloatToInt32(normalX * 127.0f));
    particle.direction[1] = wrapByte(convertFloatToInt32(normalY * 127.0f));
    particle.direction[2] = wrapByte(convertFloatToInt32(normalZ * 127.0f));
}

} // namespace

namespace {

uint16_t particleAngle(float degrees)
{
    const int32_t scaled = convertFloatToInt32(degrees * 65536.0f);
    return static_cast<uint16_t>(scaled / 360);
}

WorldParticleGroup* acquireParticleGroup(WorldParticleSystem& system,
    ChopperEffectInstance& emitter, CarMovement* car)
{
    WorldParticleGroup* group = emitter.particleGroup;
    if (group && group->occupied && group->generation == emitter.particleGroupGeneration &&
        group->emitter == &emitter && group->car == car) return group;
    for (size_t index = 0; index < system.groupCapacity; ++index) {
        group = &system.groupStorage[index];
        if (group->occupied) continue;
        *group = {};
        group->definition = emitter.particleDefinition;
        group->emitter = &emitter;
        group->car = car;
        group->generation = system.nextGeneration++;
        group->occupied = 1;
        group->previous = system.lastGroup;
        if (system.lastGroup) system.lastGroup->next = group;
        else system.firstGroup = group;
        system.lastGroup = group;
        emitter.particleGroup = group;
        emitter.particleGroupGeneration = group->generation;
        ++system.groupCount;
        return group;
    }
    return nullptr;
}

WorldParticle* acquireParticle(WorldParticleSystem& system, WorldParticleGroup& group)
{
    for (size_t index = 0; index < system.particleCapacity; ++index) {
        WorldParticle* particle = &system.particleStorage[index];
        if (particle->occupied) continue;
        *particle = {};
        particle->occupied = 1;
        particle->previous = group.last;
        if (group.last) group.last->next = particle;
        else group.first = particle;
        group.last = particle;
        ++system.particleCount;
        return particle;
    }
    ++system.droppedParticles;
    return nullptr;
}

void particleDirection(float output[4], ChopperEffectInstance& emitter,
                       const WorldParticleDefinition& definition)
{
    float identity[16], rotation[16];
    rotationMatrixIdentity(identity);
    const float halfSpread = definition.directionSpread * 0.5f;
    if (definition.conicalEmission) {
        const float spread = particleRandomUnit(emitter.randomState) * definition.directionSpread;
        const uint16_t cone = particleAngle(spread - halfSpread);
        const float yawDegrees = particleRandomUnit(emitter.randomState) * 360.0f - 180.0f;
        const uint16_t yaw = particleAngle(yawDegrees);
        const float initial[4] = {vectorCosineAngle(yaw), rotationSineAngle(yaw), 0, 0};
        motionMatrixRotateY(rotation, identity, cone);
        vectorMatrixTransform(output, rotation, initial);
        float emitterRotation[16];
        std::memcpy(emitterRotation, emitter.frame, sizeof(emitterRotation));
        emitterRotation[12] = emitterRotation[13] = emitterRotation[14] = 0.0f;
        emitterRotation[15] = 1.0f;
        vectorMatrixTransform(output, emitterRotation, output);
    } else {
        const float first = particleRandomUnit(emitter.randomState) * definition.directionSpread;
        const uint16_t x = particleAngle(first - halfSpread);
        const float second = particleRandomUnit(emitter.randomState) * definition.directionSpread;
        const uint16_t y = particleAngle(second - halfSpread);
        const float third = particleRandomUnit(emitter.randomState) * definition.directionSpread;
        const uint16_t yaw = particleAngle(third - halfSpread);
        motionMatrixRotateX(rotation, identity, x);
        motionMatrixRotateY(rotation, rotation, y);
        motionMatrixRotateYaw(rotation, rotation, yaw);
        const float initial[4] = {emitter.frame[8], emitter.frame[9], emitter.frame[10], 0};
        vectorMatrixTransform(output, rotation, initial);
    }
}

} // namespace

WorldServicesResult worldParticlesTriggerEmitter(WorldParticleSystem* system,
    ChopperEffectInstance* emitter, float elapsed, CarMovement* car)
{
    if (!system || !system->randomState || !emitter) return WORLD_SERVICES_INVALID_BINDING;
    const WorldParticleDefinition* definition = emitter->particleDefinition;
    if (!definition) return WORLD_SERVICES_OK;
    bool interested = false;
    for (const auto& point : system->interestPoints) {
        const float x = emitter->frame[12] - point[0];
        const float y = emitter->frame[13] - point[1];
        const float z = emitter->frame[14] - point[2];
        if (std::sqrt((x * x + y * y) + z * z) < definition->interestDistance) {
            interested = true;
            break;
        }
    }
    if (emitter->randomState == 0)
        emitter->randomState = static_cast<uint32_t>(convertFloatToInt32(
            particleRandomUnit(*system->randomState) * 2147483648.0f));
    if (!interested || (emitter->emissionState != 1 && !(elapsed < 0.0f)))
        return WORLD_SERVICES_OK;
    float duration = elapsed;
    if (duration >= 0.0f) {
        if (emitter->cycleRemaining != 0.0f && emitter->cycleRemaining < duration)
            duration = emitter->cycleRemaining;
    } else duration = -duration;
    float interval = emitter->emissionRemainder;
    if (!(interval < duration)) {
        emitter->emissionRemainder = elapsed >= 0.0f ? interval - duration : 0.0f;
        return WORLD_SERVICES_OK;
    }
    WorldParticleGroup* group = acquireParticleGroup(*system, *emitter, car);
    if (!group) {
        emitter->emissionRemainder = elapsed >= 0.0f ? interval - duration : 0.0f;
        return WORLD_SERVICES_OK;
    }
    const float rateVariation = definition->emissionRateVariation * definition->emissionRate;
    const float minimumRate = definition->emissionRate - rateVariation;
    const float lifeVariation = definition->lifetimeVariation * definition->lifetime;
    const float minimumLife = definition->lifetime - lifeVariation;
    const float speedVariation = definition->speedVariation * definition->speed;
    const float minimumSpeed = definition->speed - speedVariation;
    const float motionVariation = definition->carMotionVariation * definition->carMotionScale;
    const float minimumMotion = definition->carMotionScale - motionVariation;
    const float rotationRange = definition->directionAngle * 182.04444885253906f;
    const float minimumRotation = -rotationRange * 0.5f;
    float atlasCells = 1.0f;
    if (definition->randomAtlasFrame) {
        const uint32_t count = (1u << static_cast<uint8_t>(definition->atlasWidthPower)) <<
            static_cast<uint8_t>(definition->atlasHeightPower);
        atlasCells = static_cast<float>(count);
    }
    do {
        duration = duration - interval;
        float direction[4];
        particleDirection(direction, *emitter, *definition);
        float local[4], position[4];
        for (size_t axis = 0; axis < 3; ++axis) {
            const float randomOffset = particleRandomUnit(emitter->randomState) * definition->spawnRange[axis];
            const float centered = randomOffset - definition->spawnRange[axis] * 0.5f;
            local[axis] = definition->spawnOffset[axis] + centered;
        }
        local[3] = 1.0f;
        vectorMatrixTransform(position, emitter->frame, local);
        const float randomFrame = particleRandomUnit(emitter->randomState) * atlasCells;
        const uint8_t atlasFrame = static_cast<uint8_t>(convertFloatToInt32(randomFrame));
        const float randomLife = particleRandomUnit(emitter->randomState) * lifeVariation;
        float life = minimumLife + randomLife;
        const float randomRotation = particleRandomUnit(emitter->randomState) * rotationRange;
        const float rotationValue = minimumRotation + randomRotation;
        const float randomSpeed = particleRandomUnit(emitter->randomState) * speedVariation;
        float speed = minimumSpeed + randomSpeed;
        float motion = 0.0f;
        if (minimumMotion != 0.0f) {
            const float randomMotion = particleRandomUnit(emitter->randomState) * motionVariation;
            motion = minimumMotion + randomMotion;
            motion = minimumFinite(1.0f, maximumFinite(0.0f, motion));
            if (!definition->followsCar) {
                float velocity[3];
                for (size_t axis = 0; axis < 3; ++axis) {
                    const float own = speed * direction[axis];
                    const float inherited = motion * emitter->carVelocity[axis];
                    velocity[axis] = own + inherited;
                }
                speed = std::sqrt((velocity[0] * velocity[0] + velocity[1] * velocity[1]) + velocity[2] * velocity[2]);
                const float reciprocal = finiteRatio(1.0f, speed);
                for (size_t axis = 0; axis < 3; ++axis) direction[axis] = velocity[axis] * reciprocal;
            }
        }
        uint16_t rotation = static_cast<uint16_t>(convertFloatToInt32(rotationValue)) & 0xfffeu;
        if (definition->emissionMode) rotation |= static_cast<uint16_t>(emitter->randomState & 1u);
        if (life > 60.0f) life = 60.0f;
        if (!emitter->suppressParticles) {
            if (WorldParticle* particle = acquireParticle(*system, *group)) {
                for (size_t axis = 0; axis < 3; ++axis) {
                    particle->position[axis] = position[axis];
                    particle->direction[axis] = wrapByte(convertFloatToInt32(direction[axis] * 127.0f));
                }
                particle->remainingTicks = wrapHalf(convertFloatToInt32(life * 1024.0f));
                particle->rotation = rotation;
                particle->speed = static_cast<uint16_t>(convertFloatToInt32(minimumFinite(speed * 128.0f, 65535.0f)));
                particle->carMotionScale = static_cast<uint8_t>(convertFloatToInt32(motion * 255.0f));
                particle->atlasFrame = atlasFrame;
            }
        }
        const float randomRate = particleRandomUnit(emitter->randomState) * rateVariation;
        interval = finiteRatio(1.0f, minimumRate + randomRate);
        if (interval > 240.0f) interval = 240.0f;
        if (interval < 0.0f) interval = 0.0f;
    } while (interval <= duration);
    if (!group->first) worldParticlesDisposeGroup(system, group);
    emitter->emissionRemainder = elapsed >= 0.0f ? interval - duration : 0.0f;
    return system->result = WORLD_SERVICES_OK;
}

void worldParticlesInitialize(WorldParticleSystem* system,
    WorldParticle* particles, size_t particleCapacity,
    WorldParticleGroup* groups, size_t groupCapacity,
    uint32_t* randomState, const CarCinematicServices* cameras)
{
    *system = {};
    system->particleStorage = particles;
    system->particleCapacity = particleCapacity;
    system->groupStorage = groups;
    system->groupCapacity = groupCapacity;
    system->randomState = randomState;
    system->cameras = cameras;
    system->nextGeneration = 1;
    if ((!particles && particleCapacity) || (!groups && groupCapacity) || !randomState || !cameras) {
        system->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    for (size_t index = 0; index < particleCapacity; ++index) particles[index] = {};
    for (size_t index = 0; index < groupCapacity; ++index) groups[index] = {};
}

WorldServicesResult worldParticlesAddEmitter(WorldParticleSystem* system, ChopperEffectInstance* emitter)
{
    if (!system || !system->randomState || !emitter || emitter->particleSystem)
        return WORLD_SERVICES_INVALID_BINDING;
    if (emitter->randomState == 0)
        emitter->randomState = static_cast<uint32_t>(convertFloatToInt32(
            particleRandomUnit(*system->randomState) * 2147483648.0f));
    if (!emitter->particleDefinition || emitter->emissionState == 2) {
        emitter->emissionState = 2;
        appendEmitter(*system, *emitter, false);
    } else if (static_cast<uint32_t>(emitter->emissionState) < 2u) {
        const WorldParticleDefinition& definition = *emitter->particleDefinition;
        emitter->cycleRemaining = cycleDuration(*emitter,
            emitter->emissionState ? definition.on : definition.off);
        emitter->emissionRemainder = 0.0f;
        appendEmitter(*system, *emitter, true);
    } else {
        if (emitter->emissionState != 3) emitter->emissionState = 2;
        appendEmitter(*system, *emitter, false);
    }
    return system->result = WORLD_SERVICES_OK;
}

void worldParticlesRemoveEmitter(WorldParticleSystem* system, ChopperEffectInstance* emitter)
{
    if (!system || !emitter || emitter->particleSystem != system) return;
    if (emitter->autoPrevious) emitter->autoPrevious->autoNext = emitter->autoNext;
    else system->firstEmitter = emitter->autoNext;
    if (emitter->autoNext) emitter->autoNext->autoPrevious = emitter->autoPrevious;
    else system->lastEmitter = emitter->autoPrevious;
    emitter->autoNext = emitter->autoPrevious = nullptr;
    emitter->particleSystem = nullptr;
    --system->emitterCount;
}

void worldParticlesUpdateInterestPoints(WorldParticleSystem* system)
{
    const int32_t views[3] = {1, 2, 0};
    for (size_t index = 0; index < 3; ++index) {
        const CarCinematicView* view = system->cameras->screens[views[index]].view;
        if (view) std::memcpy(system->interestPoints[index], view->current.eye, sizeof(float) * 4);
    }
}

void worldParticlesDisposeGroup(WorldParticleSystem* system, WorldParticleGroup* group)
{
    if (!system || !group || !group->occupied) return;
    // PAL12A258 clears the two borrowed definition/Car cells before the
    // AcidParticle list destructor12A2C0 returns every live particle to its
    // original pool. The Windows pools use occupied slots, not PS2 addresses.
    group->definition = nullptr;
    group->car = nullptr;
    while (group->first) removeParticle(*system, *group, *group->first);
    if (group->previous) group->previous->next = group->next;
    else system->firstGroup = group->next;
    if (group->next) group->next->previous = group->previous;
    else system->lastGroup = group->previous;
    if (group->emitter && group->emitter->particleGroup == group &&
        group->emitter->particleGroupGeneration == group->generation) {
        group->emitter->particleGroup = nullptr;
        group->emitter->particleGroupGeneration = 0;
    }
    group->emitter = nullptr;
    group->occupied = 0;
    group->next = group->previous = nullptr;
    --system->groupCount;
}

void worldParticlesKillAllActiveEffects(WorldParticleSystem* system)
{
    // The source list is the active-group list at AcidEffectSystem+0.
    // Auto emitters form a different list and survive this operation.
    while (system->firstGroup)
        worldParticlesDisposeGroup(system, system->firstGroup);
}

WorldServicesResult worldParticlesAdvance(WorldParticleSystem* system, float elapsed)
{
    if (!system || !system->randomState || !system->cameras)
        return WORLD_SERVICES_INVALID_BINDING;
    const auto started = std::chrono::steady_clock::now();
    const int32_t ticks = convertFloatToInt32(elapsed * 1024.0f);
    worldParticlesUpdateInterestPoints(system);
    ChopperEffectInstance* emitter = system->firstEmitter;
    while (emitter) {
        ChopperEffectInstance* next = emitter->autoNext;
        const int32_t state = emitter->emissionState;
        if (state == 1) {
            const auto result = worldParticlesTriggerEmitter(system, emitter, elapsed, nullptr);
            if (result != WORLD_SERVICES_OK) return system->result = result;
        }
        if (emitter->cycleRemaining != 0.0f && static_cast<uint32_t>(state) < 2u) {
            const float remaining = emitter->cycleRemaining - elapsed;
            if (remaining <= 0.0f) {
                emitter->emissionState = state == 0 ? 1 : 0;
                const WorldParticleDefinition& definition = *emitter->particleDefinition;
                const WorldParticleCycle& cycle = state == 0 ? definition.on : definition.off;
                emitter->cycleRemaining = cycle.duration == 0.0f ? 0.0f : cycleDuration(*emitter, cycle) + remaining;
                emitter->emissionRemainder = 0.0f;
            } else emitter->cycleRemaining = remaining;
        }
        // A completed transient emitter may remain linked between active
        // world effects until its Chopper owner next sweeps. Ending the list
        // walk here silently prevents later still-active rocket trails and
        // explosion members from emitting this frame. Retired emitters have
        // no update, but must not hide unrelated live source instances.
        emitter = next;
    }
    // PAL1288E8..1289C8: the live global definition list is edited only
    // while CurrentGameFlowState is3. World and Frame effects borrow it.
    for (auto* definition = system->definitions && system->gameFlowState &&
            *system->gameFlowState==3 ? *system->definitions : nullptr;
            definition; definition=definition->nextDefinition) {
        float knots[4], sizeRotation[16]{};
        for (size_t lane = 0; lane < 4; ++lane) {
            knots[lane] = definition->controlKnots[lane] * 0.3333333432674408f;
            sizeRotation[lane * 4] = definition->controlSizes[lane];
            sizeRotation[lane * 4 + 1] = definition->controlRotations[lane] * 182.04444885253906f;
        }
        if (!worldParticleHermiteBasis(definition->colorCurve, definition->controlColors, knots) ||
            !worldParticleHermiteBasis(definition->sizeRotationCurve, sizeRotation, knots))
            return system->result = WORLD_SERVICES_OWNER_FAILURE;
    }
    WorldParticleGroup* group = system->firstGroup;
    while (group) {
        WorldParticleGroup* next = group->next;
        WorldParticle* particle = group->first;
        while (particle) {
            WorldParticle* following = particle->next;
            if (static_cast<int32_t>(particle->remainingTicks) - ticks <= 0)
                removeParticle(*system, *group, *particle);
            else advanceParticle(*particle, *group->definition, group->car, elapsed, ticks);
            particle = following;
        }
        if (!group->first) worldParticlesDisposeGroup(system, group);
        group = next;
    }
    system->stepMilliseconds = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    return system->result = WORLD_SERVICES_OK;
}

void worldParticleHermiteEvaluate(float output[4], const float coefficients[16], float parameter)
{
    const float square = parameter * parameter;
    const float cube = square * parameter;
    float result[4];
    for (size_t lane = 0; lane < 4; ++lane) {
        const float first = coefficients[lane] * cube;
        const float second = coefficients[4 + lane] * square;
        const float third = coefficients[8 + lane] * parameter;
        result[lane] = ((first + second) + third) + coefficients[12 + lane];
    }
    std::memcpy(output, result, sizeof(result));
}

int32_t worldParticleHermiteBasis(float output[16], const float values[16], const float knots[4])
{
    float powers[16];
    for (size_t row = 0; row < 4; ++row) {
        const float square = knots[row] * knots[row];
        powers[row * 4] = square * knots[row];
        powers[row * 4 + 1] = square;
        powers[row * 4 + 2] = knots[row];
        powers[row * 4 + 3] = 1.0f;
    }
    float inverse[16];
    afxMatrixInverse(inverse,powers);
    const float constraints[16] = {0,0,0,1, 1,1,1,1, 0,0,1,0, 3,2,1,0};
    const float hermite[16] = {2,-2,1,1, -3,3,-2,-1, 0,0,1,0, 1,0,0,0};
    float constrained[16], basis[16];
    eMulMatrix(constrained,constraints,inverse);
    eMulMatrix(basis,hermite,constrained);
    eMulMatrix(output,basis,values);
    // Source returns no status. This native API reports completed evaluation;
    // there is no invented singular-determinant rejection in the PAL caller.
    return 1;
}

#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"

#include <algorithm>

namespace hp2 {
namespace {
std::string text(const std::uint8_t* data, std::size_t capacity)
{
    const auto* end = std::find(data, data + capacity, std::uint8_t(0));
    Require(end != data + capacity, "Chopper effect resource name is unterminated");
    return {reinterpret_cast<const char*>(data), static_cast<std::size_t>(end - data)};
}

float real(const std::uint8_t* data)
{
    const auto bits = ReadU32(data);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    Require(std::isfinite(value), "Chopper effect frame is non-finite");
    return value;
}

} // namespace

ChopperEffectData::ChopperEffectData(const std::vector<std::uint8_t>& global,
    const ChopperEffectData* sharedDefinitions)
{
    std::vector<Chunk> definitionChunks, instanceChunks;
    const auto visit = [&](const auto& self, std::size_t begin, std::size_t size, unsigned depth) -> void {
        Require(depth <= 32, "Chopper effect nesting exceeds 32 levels");
        for (const auto& chunk : ReadChunks(global, begin, size)) {
            if (chunk.id == 0x35021u) definitionChunks.push_back(chunk);
            else if (chunk.id == 0x35020u) instanceChunks.push_back(chunk);
            else if (chunk.IsContainer()) self(self, chunk.payloadOffset, chunk.size, depth + 1);
        }
    };
    visit(visit, 0, global.size(), 0);
    for (const auto& chunk : definitionChunks) {
        const auto begin = (chunk.payloadOffset + 15u) & ~std::size_t(15u);
        Require(begin <= chunk.endOffset && chunk.endOffset - begin >= 16,
            "Chopper effect definition header is truncated");
        Require(ReadU32(global.data() + begin + 8) == 4, "Unsupported Chopper effect definition version");
        const auto count = ReadU32(global.data() + begin + 12);
        Require(count <= (chunk.endOffset - begin - 16) / 544 &&
            static_cast<std::size_t>(count) * 544 == chunk.endOffset - begin - 16,
            "Chopper effect definition array is incomplete");
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto* bytes = global.data() + begin + 16 + static_cast<std::size_t>(index) * 544;
            WorldParticleDefinition definition{};
            definition.key = ReadU32(bytes + 8);
            definition.textureKey = ReadU32(bytes + 12);
            definition.groupKey = ReadU32(bytes + 16);
            const uint32_t carEffectPosition = ReadU32(bytes + 0x58);
            std::memcpy(&definition.carEffectPosition, &carEffectPosition, sizeof(carEffectPosition));
            const auto signedByte = [](std::uint8_t value) {
                std::int8_t result;
                std::memcpy(&result, &value, sizeof(result));
                return result;
            };
            definition.conicalEmission = signedByte(bytes[0x18]);
            definition.atlasWidthPower = signedByte(bytes[0x19]);
            definition.atlasHeightPower = signedByte(bytes[0x1a]);
            definition.animationRate = signedByte(bytes[0x1b]);
            definition.randomAtlasFrame = signedByte(bytes[0x1c]);
            definition.emissionMode = signedByte(bytes[0x1d]);
            definition.followsCar = signedByte(bytes[0x1e]);
            Require(definition.atlasWidthPower >= 0 && definition.atlasHeightPower >= 0 &&
                definition.atlasWidthPower + definition.atlasHeightPower <= 30,
                "Effect atlas dimensions exceed the CPU frame selector");
            definition.on = {real(bytes + 0x20), real(bytes + 0x24)};
            definition.off = {real(bytes + 0x28), real(bytes + 0x2c)};
            definition.emissionRate = real(bytes + 0x30);
            definition.emissionRateVariation = real(bytes + 0x34);
            definition.lifetime = real(bytes + 0x38);
            definition.lifetimeVariation = real(bytes + 0x3c);
            definition.speed = real(bytes + 0x40);
            definition.speedVariation = real(bytes + 0x44);
            definition.directionAngle = real(bytes + 0x48);
            definition.directionSpread = real(bytes + 0x4c);
            definition.carMotionScale = real(bytes + 0x50);
            definition.carMotionVariation = real(bytes + 0x54);
            for (int lane = 0; lane < 3; ++lane) {
                definition.spawnOffset[lane] = real(bytes + 0x60 + lane * 4);
                definition.spawnRange[lane] = real(bytes + 0x70 + lane * 4);
            }
            definition.interestDistance = real(bytes + 0x80);
            definition.gravity = real(bytes + 0x84);
            definition.drag = real(bytes + 0x88);
            definition.maximumPixelSize = real(bytes + 0x8c);
            for (int lane=0;lane<4;++lane) {
                definition.controlKnots[lane]=real(bytes+0x90+lane*4);
                definition.controlSizes[lane]=real(bytes+0xe0+lane*4);
                definition.controlRotations[lane]=real(bytes+0xf0+lane*4);
            }
            for (int lane = 0; lane < 16; ++lane) {
                definition.controlColors[lane]=real(bytes+0xa0+lane*4);
                definition.colorCurve[lane] = real(bytes + 0x100 + lane * 4);
                definition.sizeRotationCurve[lane] = real(bytes + 0x140 + lane * 4);
            }
            const auto name = text(bytes + 384, 64);
            const auto render = text(bytes + 448, 48);
            const auto group = text(bytes + 496, 48);
            Require(hashString(name.c_str()) == definition.key && hashString(render.c_str()) == definition.textureKey &&
                hashString(group.c_str()) == definition.groupKey, "Chopper effect definition name/key mismatch");
            auto inserted=particleDefinitions_.emplace(definition.key, definition);
            Require(inserted.second, "Repeated Chopper effect definition");
            auto* current=&inserted.first->second;
            if(lastDefinition_)lastDefinition_->nextDefinition=current;else firstDefinition_=current;
            lastDefinition_=current;
        }
    }
    std::size_t instanceCount = 0;
    for (const auto& chunk : instanceChunks) {
        const auto begin = (chunk.payloadOffset + 15u) & ~std::size_t(15u);
        Require(begin <= chunk.endOffset && chunk.endOffset - begin >= 16,
            "Chopper effect instance header is truncated");
        Require(ReadU32(global.data() + begin + 8) == 5, "Unsupported Chopper effect instance version");
        const auto count = ReadU32(global.data() + begin + 12);
        Require(count <= (chunk.endOffset - begin - 16) / 192 &&
            static_cast<std::size_t>(count) * 192 == chunk.endOffset - begin - 16,
            "Chopper effect instance array is incomplete");
        instanceCount += count;
    }
    templates_.reserve(instanceCount);
    authoredEmitters_.reserve(instanceCount);
    for (const auto& chunk : instanceChunks) {
        const auto begin = (chunk.payloadOffset + 15u) & ~std::size_t(15u);
        const auto count = ReadU32(global.data() + begin + 12);
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto* bytes = global.data() + begin + 16 + static_cast<std::size_t>(index) * 192;
            const auto definitionKey=ReadU32(bytes+12);
            const auto* definition=Definition(definitionKey);
            if (!definition && sharedDefinitions)definition=sharedDefinitions->Definition(definitionKey);
            Require(definition!=nullptr,"Chopper effect instance references an unknown definition");
            names_.push_back(std::make_unique<std::string>(text(bytes + 128, 64)));
            ChopperEffectTemplate instance{};
            instance.name = names_.back()->c_str();
            instance.key = hashString(instance.name);
            instance.definitionKey = definition->key;
            instance.particleDefinition = definition;
            instance.renderKey = definition->textureKey;
            instance.groupKey = definition->groupKey;
            for (int lane = 0; lane < 16; ++lane) instance.frame[lane] = real(bytes + 48 + lane * 4);
            templates_.push_back(instance);
            members_[instance.groupKey].push_back(&templates_.back());
            ChopperEffectInstance emitter{};
            emitter.resource=&templates_.back();emitter.particleDefinition=definition;
            std::memcpy(emitter.frame,instance.frame,sizeof(emitter.frame));
            const auto state=ReadU32(bytes+0x10);std::memcpy(&emitter.emissionState,&state,4);
            emitter.randomState=ReadU32(bytes+0x24);
            emitter.cycleRemaining=real(bytes+0x14);emitter.emissionRemainder=real(bytes+0x18);
            const auto suppressed=ReadU32(bytes+0x28);std::memcpy(&emitter.suppressParticles,&suppressed,4);
            for (std::size_t lane=0;lane<3;++lane)emitter.carVelocity[lane]=real(bytes+0x70+lane*4);
            emitter.occupied=1;
            authoredEmitters_.push_back(emitter);
        }
    }
    for (const auto& group : members_)
        groups_.push_back({group.first, group.second.data(), group.second.size()});
}

const ChopperEffectGroup* ChopperEffectData::Find(std::uint32_t key) const noexcept
{
    for (auto* catalogue=this;catalogue;catalogue=catalogue->nextCatalogue_)
        for (const auto& group:catalogue->groups_)if (group.key==key)return &group;
    return nullptr;
}

const WorldParticleDefinition* ChopperEffectData::Definition(std::uint32_t key) const noexcept
{
    for (auto* catalogue=this;catalogue;catalogue=catalogue->nextCatalogue_) {
        const auto found=catalogue->particleDefinitions_.find(key);
        if (found!=catalogue->particleDefinitions_.end())return &found->second;
    }
    return nullptr;
}

WorldParticleDefinition* ChopperEffectData::EditableDefinition(std::uint32_t key) noexcept
{
    for(auto* catalogue=this;catalogue;catalogue=catalogue->nextCatalogue_) {
        const auto found=catalogue->particleDefinitions_.find(key);
        if(found!=catalogue->particleDefinitions_.end())return &found->second;
    }
    return nullptr;
}

ChopperEffectData::~ChopperEffectData()
{
    if (UnbindParticles()!=WORLD_SERVICES_OK)std::terminate();
    Unregister();
}

WorldServicesResult ChopperEffectData::BindParticles(WorldParticleSystem& system)
{
    if (particleSystem_ || !system.randomState || !system.cameras)return WORLD_SERVICES_INVALID_BINDING;
    particleSystem_=&system;
    for (auto& emitter:authoredEmitters_) {
        if (system.nextGeneration==UINT64_MAX)return WORLD_SERVICES_CAPACITY;
        emitter.generation=system.nextGeneration++;
        const auto result=worldParticlesAddEmitter(&system,&emitter);
        if (result!=WORLD_SERVICES_OK)return result;
    }
    return WORLD_SERVICES_OK;
}

WorldServicesResult ChopperEffectData::UnbindParticles() noexcept
{
    if (!particleSystem_)return WORLD_SERVICES_OK;
    auto& system=*particleSystem_;
    // The source world teardown retires live effects before their definition
    // chunk is released. Refuse to free a definition still used by a group.
    for (auto* group=system.firstGroup;group;group=group->next)
        for (const auto& definition:particleDefinitions_)
            if (group->definition==&definition.second)return WORLD_SERVICES_INVALID_BINDING;
    for (auto* emitter=system.firstEmitter;emitter;emitter=emitter->autoNext)
        if (emitter->resource)
            for (const auto& resource:templates_)if (emitter->resource==&resource) {
                bool authored=false;
                for (const auto& instance:authoredEmitters_)if (emitter==&instance) {authored=true;break;}
                if (!authored)return WORLD_SERVICES_INVALID_BINDING;
            }
    for (auto& emitter:authoredEmitters_) {
        if (emitter.particleGroup && emitter.particleGroup->emitter==&emitter &&
                emitter.particleGroup->generation==emitter.particleGroupGeneration)
            emitter.particleGroup->emitter=nullptr;
        emitter.particleGroup=nullptr;emitter.particleGroupGeneration=0;
        if (emitter.particleSystem)worldParticlesRemoveEmitter(&system,&emitter);
    }
    particleSystem_=nullptr;return WORLD_SERVICES_OK;
}

bool ChopperEffectData::Register(ChopperEffectData& bank) noexcept
{
    if (catalogueRoot_ || nextCatalogue_ || &bank==this)return false;
    auto* root=bank.catalogueRoot_?bank.catalogueRoot_:&bank;
    if (root==this)return false;
    auto* tail=root;
    while (tail->nextCatalogue_) {
        if (tail->nextCatalogue_==this)return false;
        tail=tail->nextCatalogue_;
    }
    tail->nextCatalogue_=this;catalogueRoot_=root;
    if(firstDefinition_) {
        auto** definition=&root->firstDefinition_;
        while(*definition)definition=&(*definition)->nextDefinition;
        *definition=firstDefinition_;
    }
    return true;
}

void ChopperEffectData::Unregister() noexcept
{
    if (!catalogueRoot_)return;
    if(firstDefinition_) {
        auto** definition=&catalogueRoot_->firstDefinition_;
        while(*definition && *definition!=firstDefinition_)definition=&(*definition)->nextDefinition;
        if(*definition)*definition=lastDefinition_->nextDefinition;
        lastDefinition_->nextDefinition=nullptr;
    }
    auto** link=&catalogueRoot_->nextCatalogue_;
    while (*link && *link!=this)link=&(*link)->nextCatalogue_;
    if (*link)*link=nextCatalogue_;
    nextCatalogue_=nullptr;catalogueRoot_=nullptr;
}

void ChopperEffectData::UpdateTextures(ResourceRegistry& resources)
{
    for (auto* catalogue=this;catalogue;catalogue=catalogue->nextCatalogue_)
      for (const auto& entry:catalogue->particleDefinitions_) {
        const auto& definition=entry.second;
        auto& texture=catalogue->textures_[&definition];
        if (!resources.IsTextureAlive(texture) || texture->key!=definition.textureKey)
            texture=resources.ResolveTexture(definition.textureKey,false);
    }
}

const EditorMarkerTexture* ChopperEffectData::Texture(const ChopperEffectTemplate& resource) const noexcept
{
    return resource.particleDefinition ? Texture(*resource.particleDefinition):nullptr;
}

const EditorMarkerTexture* ChopperEffectData::Texture(const WorldParticleDefinition& definition) const noexcept
{
    const auto* root=catalogueRoot_?catalogueRoot_:this;
    for (auto* catalogue=root;catalogue;catalogue=catalogue->nextCatalogue_) {
        const auto found=catalogue->textures_.find(&definition);
        if (found!=catalogue->textures_.end())return found->second;
    }
    return nullptr;
}

} // namespace hp2


#include <string.h>

void chopperEffectsInitialize(ChopperMunitionEffects* effects,
    ChopperEffectInstance* instances, size_t capacity,
    ChopperEffectRequest* requests, size_t requestCapacity,
    const ChopperEffectGroup* groups, size_t groupCount)
{
    memset(effects, 0, sizeof(*effects));
    effects->instances = instances;
    effects->capacity = capacity;
    effects->requests = requests;
    effects->requestCapacity = requestCapacity;
    effects->groups = groups;
    effects->groupCount = groupCount;
    effects->nextGeneration = 1;
    if ((!instances && capacity) || (!requests && requestCapacity) || (!groups && groupCount)) {
        effects->result = WORLD_SERVICES_INVALID_BINDING;
        return;
    }
    if (capacity) memset(instances, 0, capacity * sizeof(*instances));
}

WorldServicesResult chopperEffectsBindParticles(ChopperMunitionEffects* effects,
    WorldParticleSystem* particles)
{
    if (!effects || !particles || !particles->randomState || !particles->cameras)
        return WORLD_SERVICES_INVALID_BINDING;
    if (effects->particles == particles) return WORLD_SERVICES_OK;
    for (size_t index = 0; index < effects->capacity; ++index)
        if (effects->instances[index].occupied) return WORLD_SERVICES_INVALID_BINDING;
    effects->particles = particles;
    return effects->result = WORLD_SERVICES_OK;
}

static bool canRecordRequests(const ChopperMunitionEffects* effects, size_t count)
{
    if (effects->particles) return true;
    return effects->requestCount <= effects->requestCapacity &&
        count <= effects->requestCapacity - effects->requestCount &&
        count <= UINT64_MAX - effects->nextSequence;
}

static ChopperEffectRequest* appendRequest(ChopperMunitionEffects* effects, ChopperEffectInstance* instance,
    ChopperEffectRequestKind kind)
{
    if (!effects->requests || effects->requestCount >= effects->requestCapacity ||
        effects->nextSequence == UINT64_MAX) {
        ++effects->droppedRequests;
        return nullptr;
    }
    ChopperEffectRequest* request = &effects->requests[
        (effects->readIndex + effects->requestCount) % effects->requestCapacity];
    request->kind = kind;
    request->resource = instance->resource;
    request->instance = instance;
    request->generation = instance->generation;
    request->sequence = effects->nextSequence++;
    memcpy(request->frame, instance->frame, sizeof(request->frame));
    request->elapsed = 0.0f;
    ++effects->requestCount;
    return request;
}

WorldServicesResult chopperEffectsSpawnGroup(ChopperMunitionEffects* effects,
    uint32_t groupKey, const float frame[16])
{
    if (!effects || !frame) return WORLD_SERVICES_INVALID_BINDING;
    const ChopperEffectGroup* group = NULL;
    for (size_t index = 0; index < effects->groupCount; ++index)
        if (effects->groups[index].key == groupKey) { group = &effects->groups[index]; break; }
    if (!group || (group->count && !group->members))
        return effects->result = WORLD_SERVICES_INVALID_BINDING;
    // Never reserve half of an authored explosion group: each member must
    // belong to the same successfully published detonation.
    for (size_t index = 0; index < group->count; ++index) {
        if (!group->members[index] || group->members[index]->groupKey != groupKey)
            return effects->result = WORLD_SERVICES_INVALID_BINDING;
    }
    size_t freeSlots = 0;
    for (size_t index = 0; index < effects->capacity; ++index)
        freeSlots += !effects->instances[index].occupied;
    // An exhausted native emitter pool may contain completed auto-emitters
    // that have not yet reached the Chopper owner's normal sweep. Reclaim
    // only after the real particle cycle has marked them completed. The
    // request-only test backend uses its own explicit completion markers.
    if (group->count > freeSlots) {
        size_t requestCompleted = 0;
        for (ChopperEffectInstance* instance = effects->first; instance; instance = instance->next)
            requestCompleted += instance->emissionState != 1;
        if (requestCompleted && requestCompleted <= SIZE_MAX - group->count &&
            canRecordRequests(effects, group->count + requestCompleted)) {
            chopperEffectsSweep(effects);
            if (effects->result != WORLD_SERVICES_OK)return effects->result;
            freeSlots = 0;
            for (size_t index = 0; index < effects->capacity; ++index)
                freeSlots += !effects->instances[index].occupied;
        }
    }
    if (group->count > freeSlots || !canRecordRequests(effects, group->count) ||
        group->count > UINT64_MAX - effects->nextGeneration)
        return effects->result = WORLD_SERVICES_CAPACITY;
    size_t slot = 0;
    for (size_t index = 0; index < group->count; ++index) {
        while (effects->instances[slot].occupied) ++slot;
        ChopperEffectInstance* instance = &effects->instances[slot++];
        memset(instance, 0, sizeof(*instance));
        instance->resource = group->members[index];
        instance->particleDefinition = instance->resource->particleDefinition;
        memcpy(instance->frame, frame, sizeof(instance->frame));
        instance->generation = effects->nextGeneration++;
        instance->emissionState = 1;
        instance->occupied = 1;
        instance->next = effects->first;
        instance->previous = NULL;
        if (effects->first) effects->first->previous = instance;
        else effects->last = instance;
        effects->first = instance;
        if (effects->particles) {
            const auto result = worldParticlesAddEmitter(effects->particles, instance);
            if (result != WORLD_SERVICES_OK) return effects->result = result;
        }
        appendRequest(effects, instance, CHOPPER_EFFECT_START);
    }
    return effects->result = WORLD_SERVICES_OK;
}

static void detachParticleEmitter(ChopperMunitionEffects* effects, ChopperEffectInstance* instance)
{
    WorldParticleSystem* particles = effects->particles;
    if (!particles) return;
    if (instance->particleSystem == particles) worldParticlesRemoveEmitter(particles, instance);
    for (WorldParticleGroup* group = particles->firstGroup; group; group = group->next)
        if (group->emitter == instance) group->emitter = nullptr;
    instance->particleGroup = nullptr;
    instance->particleGroupGeneration = 0;
}

static void releaseInstance(ChopperMunitionEffects* effects, ChopperEffectInstance* instance)
{
    detachParticleEmitter(effects, instance);
    appendRequest(effects, instance, CHOPPER_EFFECT_RELEASE);
    if (instance->previous) instance->previous->next = instance->next;
    else effects->first = instance->next;
    if (instance->next) instance->next->previous = instance->previous;
    else effects->last = instance->previous;
    instance->occupied = 0;
    instance->next = instance->previous = NULL;
}

void chopperEffectsSweep(ChopperMunitionEffects* effects)
{
    if (!effects) return;
    ChopperEffectInstance* instance = effects->first;
    while (instance) {
        ChopperEffectInstance* next = instance->next;
        if (instance->emissionState != 1) {
            if (!canRecordRequests(effects, 1)) {
                effects->result = WORLD_SERVICES_CAPACITY;
                return;
            }
            releaseInstance(effects, instance);
        }
        instance = next;
    }
    effects->result = WORLD_SERVICES_OK;
}

void chopperEffectsDispose(ChopperMunitionEffects* effects)
{
    if (!effects) return;
    for (ChopperEffectInstance* instance = effects->first; instance; instance = instance->next)
        instance->emissionState = 0;
    chopperEffectsSweep(effects);
}

// Dispose the real MunitionEffectsHandler subset in the shared native pool.
// Other World effects retain their own instances and published particles.
void chopperEffectsReleaseOwner(ChopperMunitionEffects* effects,const void* owner)
{
    if (!effects || !owner)return;
    ChopperEffectInstance* instance=effects->first;
    while (instance) {
        ChopperEffectInstance* next=instance->next;
        if (instance->lifetimeOwner==owner) {
            if (!canRecordRequests(effects,1)) {
                effects->result=WORLD_SERVICES_CAPACITY;
                return;
            }
            releaseInstance(effects,instance);
        }
        instance=next;
    }
}

int32_t chopperEffectsTake(ChopperMunitionEffects* effects, ChopperEffectRequest* request)
{
    if (!effects || !request || !effects->requestCount) return 0;
    *request = effects->requests[effects->readIndex];
    effects->readIndex = (effects->readIndex + 1) % effects->requestCapacity;
    --effects->requestCount;
    return 1;
}

void chopperEffectCompleted(ChopperMunitionEffects* effects,
    ChopperEffectInstance* instance, uint64_t generation)
{
    if (!effects || !instance) return;
    if (effects->particles) return;
    for (ChopperEffectInstance* current = effects->first; current; current = current->next) {
        if (current == instance && current->occupied && current->generation == generation) {
            current->emissionState = 0;
            return;
        }
    }
}

WorldServicesResult chopperEffectsCreatePersistentGroup(ChopperMunitionEffects* effects,
    uint32_t groupKey, ChopperEffectInstance** first, ChopperEffectInstance** last)
{
    if (!effects || !first || !last || *first || *last) return WORLD_SERVICES_INVALID_BINDING;
    const ChopperEffectGroup* group = NULL;
    for (size_t index = 0; index < effects->groupCount; ++index)
        if (effects->groups[index].key == groupKey) { group = &effects->groups[index]; break; }
    if (!group || (group->count && !group->members))
        return effects->result = WORLD_SERVICES_INVALID_BINDING;
    size_t available = 0;
    for (size_t index = 0; index < effects->capacity; ++index)
        available += !effects->instances[index].occupied;
    if (group->count > available || group->count > UINT64_MAX - effects->nextGeneration)
        return effects->result = WORLD_SERVICES_CAPACITY;
    for (size_t index = 0; index < group->count; ++index)
        if (!group->members[index] || group->members[index]->groupKey != groupKey)
            return effects->result = WORLD_SERVICES_INVALID_BINDING;
    size_t slot = 0;
    for (size_t index = 0; index < group->count; ++index) {
        while (effects->instances[slot].occupied) ++slot;
        ChopperEffectInstance* instance = &effects->instances[slot++];
        memset(instance, 0, sizeof(*instance));
        instance->resource = group->members[index];
        instance->particleDefinition = instance->resource->particleDefinition;
        memcpy(instance->frame, instance->resource->frame, sizeof(instance->frame));
        instance->generation = effects->nextGeneration++;
        instance->occupied = 1;
        instance->previous = *last;
        if (*last) (*last)->next = instance;
        else *first = instance;
        *last = instance;
    }
    return effects->result = WORLD_SERVICES_OK;
}

void chopperEffectsReleasePersistentGroup(ChopperMunitionEffects* effects,
    ChopperEffectInstance** first, ChopperEffectInstance** last)
{
    if (!effects || !first || !last) return;
    ChopperEffectInstance* instance = *first;
    while (instance) {
        if (!canRecordRequests(effects, 1)) {
            effects->result = WORLD_SERVICES_CAPACITY;
            return;
        }
        ChopperEffectInstance* next = instance->next;
        detachParticleEmitter(effects, instance);
        appendRequest(effects, instance, CHOPPER_EFFECT_RELEASE);
        *first = next;
        if (next) next->previous = NULL;
        else *last = NULL;
        instance->occupied = 0;
        instance->next = instance->previous = NULL;
        instance = next;
    }
    effects->result = WORLD_SERVICES_OK;
}

WorldServicesResult chopperEffectsEmit(ChopperMunitionEffects* effects,
    ChopperEffectInstance* instance, const float frame[16], float elapsed)
{
    if (!effects || !instance || !frame) return WORLD_SERVICES_INVALID_BINDING;
    size_t slot = 0;
    while (slot < effects->capacity && &effects->instances[slot] != instance) ++slot;
    if (slot == effects->capacity || !instance->occupied || !instance->resource)
        return effects->result = WORLD_SERVICES_INVALID_BINDING;
    if (!canRecordRequests(effects, 1))
        return effects->result = WORLD_SERVICES_CAPACITY;
    memcpy(instance->frame, frame, sizeof(instance->frame));
    if (effects->particles) {
        const auto result = worldParticlesTriggerEmitter(effects->particles, instance, elapsed, nullptr);
        if (result != WORLD_SERVICES_OK) return effects->result = result;
    }
    if (ChopperEffectRequest* request = appendRequest(effects, instance, CHOPPER_EFFECT_EMIT))
        request->elapsed = elapsed;
    return effects->result = WORLD_SERVICES_OK;
}

#include <array>

namespace hp2 {

struct AcidFXRuntime::Storage {
    std::array<WorldParticle,1000> particles{};
    std::array<WorldParticleGroup,400> groups{};
    std::array<ChopperEffectInstance,600> emitters{};
    std::uint32_t randomState=0x23985727u;
    WorldParticleSystem system{};
    WorldParticleSystem frameSystem{};
    ChopperMunitionEffects handler{};
    ChopperEffectData* catalogue=nullptr;
};

AcidFXRuntime::AcidFXRuntime(const CarCinematicServices& cameras)
    :storage_(std::make_unique<Storage>())
{
    auto& state=*storage_;
    worldParticlesInitialize(&state.system,state.particles.data(),state.particles.size(),
        state.groups.data(),state.groups.size(),&state.randomState,&cameras);
    // Both original AcidEffectSystem instances share the global afxInit
    // pools (PAL126D70); only their emitter/group lists and interest points
    // are separate. No effect has been published during either initializer.
    worldParticlesInitialize(&state.frameSystem,state.particles.data(),state.particles.size(),
        state.groups.data(),state.groups.size(),&state.randomState,&cameras);
    chopperEffectsInitialize(&state.handler,state.emitters.data(),state.emitters.size(),
        nullptr,0,nullptr,0);
    Require(chopperEffectsBindParticles(&state.handler,&state.system)==WORLD_SERVICES_OK,
        "FX pools must borrow their one initialized particle owner");
}

AcidFXRuntime::~AcidFXRuntime()
{
    auto& state=*storage_;
    chopperEffectsDispose(&state.handler);
    worldParticlesKillAllActiveEffects(&state.frameSystem);
    worldParticlesKillAllActiveEffects(&state.system);
    // ResourceRegistry and actors must release their borrowed emitters first.
    if (state.catalogue || state.system.firstEmitter || state.frameSystem.firstEmitter)std::terminate();
}

WorldParticleSystem& AcidFXRuntime::Particles() noexcept
{
    return storage_->system;
}

WorldParticleSystem& AcidFXRuntime::FrameParticles() noexcept
{
    return storage_->frameSystem;
}

void AcidFXRuntime::BindGameFlow(const std::int32_t& state) noexcept
{
    storage_->system.gameFlowState=&state;
    storage_->frameSystem.gameFlowState=&state;
}

ChopperEffectInstance* AcidFXRuntime::CreateFrameEmitter(std::uint32_t key,const void* owner)
{
    auto& state=*storage_;
    Require(state.catalogue && owner,"Frame emitter has no actual catalogue/lifetime owner");
    const auto* definition=state.catalogue->Definition(key);
    Require(definition,"Frame emitter definition is missing from the loaded catalogue");
    for(auto& emitter:state.emitters)if(!emitter.occupied) {
        Require(state.handler.nextGeneration!=UINT64_MAX,"FX emitter generation exhausted");
        emitter={};emitter.occupied=1;emitter.lifetimeOwner=owner;
        emitter.generation=state.handler.nextGeneration++;
        emitter.particleDefinition=definition;emitter.emissionState=1;
        if(worldParticlesAddEmitter(&state.frameSystem,&emitter)!=WORLD_SERVICES_OK) {
            emitter={};throw std::runtime_error("Frame emitter could not enter its actual effect system");
        }
        return &emitter;
    }
    return nullptr; // Original fixed afx emitter pool may be exhausted.
}

void AcidFXRuntime::ReleaseFrameEmitter(ChopperEffectInstance& emitter,const void* owner)
{
    auto& state=*storage_;
    Require(emitter.occupied && emitter.lifetimeOwner==owner && emitter.particleSystem==&state.frameSystem,
        "Frame emitter release has a mismatched lifetime owner");
    worldParticlesRemoveEmitter(&state.frameSystem,&emitter);
    if(emitter.particleGroup && emitter.particleGroup->generation==emitter.particleGroupGeneration &&
            emitter.particleGroup->emitter==&emitter)emitter.particleGroup->emitter=nullptr;
    emitter={};
}

void AcidFXRuntime::BindCatalogue(ChopperEffectData& catalogue)
{
    auto& state=*storage_;
    Require(!state.catalogue,"The FX pool already has a global catalog");
    Require(catalogue.Particles()==&state.system,
        "The global catalog must publish its authored emitters to the same FX owner");
    state.handler.groups=catalogue.Groups();
    state.handler.groupCount=catalogue.GroupCount();
    state.catalogue=&catalogue;
    state.system.definitions=state.frameSystem.definitions=catalogue.DefinitionList();
}

void AcidFXRuntime::ReleaseCatalogue(ChopperEffectData& catalogue)
{
    auto& state=*storage_;
    Require(state.catalogue==&catalogue,"The global FX catalog is not bound here");
    for (const auto& emitter:state.emitters)
        Require(!emitter.occupied,"An actor still owns a dynamic FX emitter");
    Require(!state.frameSystem.firstEmitter,
        "A camera still owns an emitter from the retiring global FX catalog");
    // PAL afxClose126E08 retires FrameEffects before the shared pools and
    // their authored definitions. Camera destruction removes auto emitters;
    // their already emitted particles retain definitions until this close.
    worldParticlesKillAllActiveEffects(&state.frameSystem);
    Require(catalogue.UnbindParticles()==WORLD_SERVICES_OK,
        "Live FX groups still reference the global catalog");
    state.handler.groups=nullptr;
    state.handler.groupCount=0;
    state.catalogue=nullptr;
    state.system.definitions=state.frameSystem.definitions=nullptr;
}

ChopperMunitionEffects& AcidFXRuntime::Emitters()
{
    Require(storage_->catalogue!=nullptr,"Dynamic FX needs the loaded global catalog");
    return storage_->handler;
}

} // namespace hp2


// Complete original matrix primitives. Matrix lane names are row/column;
// reload suffixes preserve the original overlapping-input store order.
float afxMatrixDeterminant(const float input[16])
{
    const float m00_0=input[0];
    const float m33_0=input[15];
    const float m30_0=input[3];
    const float m03_0=input[12];
    const float m12_0=input[9];
    const float m21_0=input[6];
    const float m20_0=input[2];
    const float m31_0=input[7];
    const float m22_0=input[10];
    const float m11_0=input[5];
    const float m10_0=input[1];
    const float m32_0=input[11];
    const float m13_0=input[13];
    const float m02_0=input[8];
    const float m01_0=input[4];
    const float m23_0=input[14];
    return (((((((((((((((((((((((((m30_0 * m03_0) * (m12_0 * m21_0)) - ((m20_0 * m03_0) * (m12_0 * m31_0))) - ((m30_0 * m03_0) * (m22_0 * m11_0))) + ((m10_0 * m03_0) * (m22_0 * m31_0))) + ((m20_0 * m03_0) * (m32_0 * m11_0))) - ((m10_0 * m03_0) * (m32_0 * m21_0))) - ((m30_0 * m13_0) * (m02_0 * m21_0))) + ((m20_0 * m13_0) * (m02_0 * m31_0))) + ((m30_0 * m13_0) * (m22_0 * m01_0))) - ((m00_0 * m13_0) * (m22_0 * m31_0))) - ((m20_0 * m13_0) * (m32_0 * m01_0))) + ((m00_0 * m13_0) * (m32_0 * m21_0))) + ((m30_0 * m23_0) * (m02_0 * m11_0))) - ((m10_0 * m23_0) * (m02_0 * m31_0))) - ((m30_0 * m23_0) * (m12_0 * m01_0))) + ((m00_0 * m23_0) * (m12_0 * m31_0))) + ((m10_0 * m23_0) * (m32_0 * m01_0))) - ((m00_0 * m23_0) * (m32_0 * m11_0))) - ((m20_0 * m33_0) * (m02_0 * m11_0))) + ((m10_0 * m33_0) * (m02_0 * m21_0))) + ((m20_0 * m33_0) * (m12_0 * m01_0))) - ((m00_0 * m33_0) * (m12_0 * m21_0))) - ((m10_0 * m33_0) * (m22_0 * m01_0))) + ((m00_0 * m33_0) * (m22_0 * m11_0)));
}

void afxMatrixInverse(float output[16],const float input[16])
{
    const float determinant=afxMatrixDeterminant(input);
    const float m22_0=input[10];
    const float m21_0=input[6];
    const float m32_0=input[11];
    const float m31_0=input[7];
    const float m13_0=input[13];
    const float m12_0=input[9];
    const float m23_0=input[14];
    const float m11_0=input[5];
    const float m33_0=input[15];
    const float reciprocal=finiteRatio(1.0f,determinant);
    output[0]=(reciprocal * (((((((m21_0 * m32_0) * m13_0) - ((m31_0 * m22_0) * m13_0)) + ((m31_0 * m12_0) * m23_0)) - ((m11_0 * m32_0) * m23_0)) - ((m21_0 * m12_0) * m33_0)) + ((m11_0 * m22_0) * m33_0)));
    const float m22_1=input[10];
    const float m30_0=input[3];
    const float m20_0=input[2];
    const float m32_1=input[11];
    const float m13_1=input[13];
    const float m12_1=input[9];
    const float m23_1=input[14];
    const float m10_0=input[1];
    const float m33_1=input[15];
    output[1]=(reciprocal * (((((((m30_0 * m22_1) * m13_1) - ((m20_0 * m32_1) * m13_1)) - ((m30_0 * m12_1) * m23_1)) + ((m10_0 * m32_1) * m23_1)) + ((m20_0 * m12_1) * m33_1)) - ((m10_0 * m22_1) * m33_1)));
    const float m21_1=input[6];
    const float m20_1=input[2];
    const float m31_1=input[7];
    const float m30_1=input[3];
    const float m13_2=input[13];
    const float m11_1=input[5];
    const float m23_2=input[14];
    const float m10_1=input[1];
    const float m33_2=input[15];
    output[2]=(reciprocal * (((((((m20_1 * m31_1) * m13_2) - ((m30_1 * m21_1) * m13_2)) + ((m30_1 * m11_1) * m23_2)) - ((m10_1 * m31_1) * m23_2)) - ((m20_1 * m11_1) * m33_2)) + ((m10_1 * m21_1) * m33_2)));
    const float m21_2=input[6];
    const float m30_2=input[3];
    const float m20_2=input[2];
    const float m31_2=input[7];
    const float m12_2=input[9];
    const float m11_2=input[5];
    const float m22_2=input[10];
    const float m10_2=input[1];
    const float m32_2=input[11];
    output[3]=(reciprocal * (((((((m30_2 * m21_2) * m12_2) - ((m20_2 * m31_2) * m12_2)) - ((m30_2 * m11_2) * m22_2)) + ((m10_2 * m31_2) * m22_2)) + ((m20_2 * m11_2) * m32_2)) - ((m10_2 * m21_2) * m32_2)));
    const float m22_3=input[10];
    const float m31_3=input[7];
    const float m21_3=input[6];
    const float m32_3=input[11];
    const float m03_0=input[12];
    const float m02_0=input[8];
    const float m23_3=input[14];
    const float m01_0=input[4];
    const float m33_3=input[15];
    output[4]=(reciprocal * (((((((m31_3 * m22_3) * m03_0) - ((m21_3 * m32_3) * m03_0)) - ((m31_3 * m02_0) * m23_3)) + ((m01_0 * m32_3) * m23_3)) + ((m21_3 * m02_0) * m33_3)) - ((m01_0 * m22_3) * m33_3)));
    const float m22_4=input[10];
    const float m20_3=input[2];
    const float m32_4=input[11];
    const float m30_3=input[3];
    const float m03_1=input[12];
    const float m02_1=input[8];
    const float m23_4=input[14];
    const float m00_0=input[0];
    const float m33_4=input[15];
    output[5]=(reciprocal * (((((((m20_3 * m32_4) * m03_1) - ((m30_3 * m22_4) * m03_1)) + ((m30_3 * m02_1) * m23_4)) - ((m00_0 * m32_4) * m23_4)) - ((m20_3 * m02_1) * m33_4)) + ((m00_0 * m22_4) * m33_4)));
    const float m21_4=input[6];
    const float m30_4=input[3];
    const float m20_4=input[2];
    const float m31_4=input[7];
    const float m03_2=input[12];
    const float m01_1=input[4];
    const float m23_5=input[14];
    const float m00_1=input[0];
    const float m33_5=input[15];
    output[6]=(reciprocal * (((((((m30_4 * m21_4) * m03_2) - ((m20_4 * m31_4) * m03_2)) - ((m30_4 * m01_1) * m23_5)) + ((m00_1 * m31_4) * m23_5)) + ((m20_4 * m01_1) * m33_5)) - ((m00_1 * m21_4) * m33_5)));
    const float m21_5=input[6];
    const float m20_5=input[2];
    const float m31_5=input[7];
    const float m30_5=input[3];
    const float m02_2=input[8];
    const float m01_2=input[4];
    const float m22_5=input[10];
    const float m00_2=input[0];
    const float m32_5=input[11];
    output[7]=(reciprocal * (((((((m20_5 * m31_5) * m02_2) - ((m30_5 * m21_5) * m02_2)) + ((m30_5 * m01_2) * m22_5)) - ((m00_2 * m31_5) * m22_5)) - ((m20_5 * m01_2) * m32_5)) + ((m00_2 * m21_5) * m32_5)));
    const float m12_3=input[9];
    const float m11_3=input[5];
    const float m32_6=input[11];
    const float m31_6=input[7];
    const float m03_3=input[12];
    const float m02_3=input[8];
    const float m13_3=input[13];
    const float m01_3=input[4];
    const float m33_6=input[15];
    output[8]=(reciprocal * (((((((m11_3 * m32_6) * m03_3) - ((m31_6 * m12_3) * m03_3)) + ((m31_6 * m02_3) * m13_3)) - ((m01_3 * m32_6) * m13_3)) - ((m11_3 * m02_3) * m33_6)) + ((m01_3 * m12_3) * m33_6)));
    const float m12_4=input[9];
    const float m30_6=input[3];
    const float m10_3=input[1];
    const float m32_7=input[11];
    const float m03_4=input[12];
    const float m02_4=input[8];
    const float m13_4=input[13];
    const float m00_3=input[0];
    const float m33_7=input[15];
    output[9]=(reciprocal * (((((((m30_6 * m12_4) * m03_4) - ((m10_3 * m32_7) * m03_4)) - ((m30_6 * m02_4) * m13_4)) + ((m00_3 * m32_7) * m13_4)) + ((m10_3 * m02_4) * m33_7)) - ((m00_3 * m12_4) * m33_7)));
    const float m11_4=input[5];
    const float m10_4=input[1];
    const float m31_7=input[7];
    const float m30_7=input[3];
    const float m03_5=input[12];
    const float m01_4=input[4];
    const float m13_5=input[13];
    const float m00_4=input[0];
    const float m33_8=input[15];
    output[10]=(reciprocal * (((((((m10_4 * m31_7) * m03_5) - ((m30_7 * m11_4) * m03_5)) + ((m30_7 * m01_4) * m13_5)) - ((m00_4 * m31_7) * m13_5)) - ((m10_4 * m01_4) * m33_8)) + ((m00_4 * m11_4) * m33_8)));
    const float m11_5=input[5];
    const float m30_8=input[3];
    const float m10_5=input[1];
    const float m31_8=input[7];
    const float m02_5=input[8];
    const float m01_5=input[4];
    const float m12_5=input[9];
    const float m00_5=input[0];
    const float m32_8=input[11];
    output[11]=(reciprocal * (((((((m30_8 * m11_5) * m02_5) - ((m10_5 * m31_8) * m02_5)) - ((m30_8 * m01_5) * m12_5)) + ((m00_5 * m31_8) * m12_5)) + ((m10_5 * m01_5) * m32_8)) - ((m00_5 * m11_5) * m32_8)));
    const float m12_6=input[9];
    const float m21_6=input[6];
    const float m11_6=input[5];
    const float m22_6=input[10];
    const float m03_6=input[12];
    const float m02_6=input[8];
    const float m13_6=input[13];
    const float m01_6=input[4];
    const float m23_6=input[14];
    output[12]=(reciprocal * (((((((m21_6 * m12_6) * m03_6) - ((m11_6 * m22_6) * m03_6)) - ((m21_6 * m02_6) * m13_6)) + ((m01_6 * m22_6) * m13_6)) + ((m11_6 * m02_6) * m23_6)) - ((m01_6 * m12_6) * m23_6)));
    const float m12_7=input[9];
    const float m10_6=input[1];
    const float m22_7=input[10];
    const float m20_6=input[2];
    const float m03_7=input[12];
    const float m02_7=input[8];
    const float m13_7=input[13];
    const float m00_6=input[0];
    const float m23_7=input[14];
    output[13]=(reciprocal * (((((((m10_6 * m22_7) * m03_7) - ((m20_6 * m12_7) * m03_7)) + ((m20_6 * m02_7) * m13_7)) - ((m00_6 * m22_7) * m13_7)) - ((m10_6 * m02_7) * m23_7)) + ((m00_6 * m12_7) * m23_7)));
    const float m11_7=input[5];
    const float m20_7=input[2];
    const float m10_7=input[1];
    const float m21_7=input[6];
    const float m03_8=input[12];
    const float m01_7=input[4];
    const float m13_8=input[13];
    const float m00_7=input[0];
    const float m23_8=input[14];
    output[14]=(reciprocal * (((((((m20_7 * m11_7) * m03_8) - ((m10_7 * m21_7) * m03_8)) - ((m20_7 * m01_7) * m13_8)) + ((m00_7 * m21_7) * m13_8)) + ((m10_7 * m01_7) * m23_8)) - ((m00_7 * m11_7) * m23_8)));
    const float m11_8=input[5];
    const float m10_8=input[1];
    const float m21_8=input[6];
    const float m20_8=input[2];
    const float m02_8=input[8];
    const float m01_8=input[4];
    const float m12_8=input[9];
    const float m00_8=input[0];
    const float m22_8=input[10];
    output[15]=(reciprocal * (((((((m10_8 * m21_8) * m02_8) - ((m20_8 * m11_8) * m02_8)) + ((m20_8 * m01_8) * m12_8)) - ((m00_8 * m21_8) * m12_8)) - ((m10_8 * m01_8) * m22_8)) + ((m00_8 * m11_8) * m22_8)));
}

// Final AcidEffectSystem::RenderEffects128DA0..12982C, original AcidFXE.cpp.
// The original live lists, curves, atlas, size and source group order remain
// canonical. The PC renderer receives projected floating-point vertices;
// no console screen-address or float-as-depth-word transport remains.
#include "indep/inc/Texture.hpp"

namespace hp2 {
namespace {
bool particleCarVisible(const WorldParticleGroup& group,const CarCinematicScreen& view)
{
    if(!group.car)return true;
    const auto& bindings=group.car->bindings;
    Require(bindings.owner && bindings.ownerMotion,"Particle group lost its live Car/motion owner");
    if(!((bindings.ownerMotion->wheelEffectsFlags>>view.id)&1u))return false;
    // Exact final26-entry switch2AD3B0. The hidden-car and rear-view
    // exceptions are source car-effect positions, not texture heuristics.
    switch(group.definition->carEffectPosition) {
    case 10:return view.id!=3 && view.view->hiddenCar!=bindings.owner;
    case 9:case 11:case 26:return view.id==3 || view.view->hiddenCar!=bindings.owner;
    case 1:case 5:case 6:return view.view->hiddenCar!=bindings.owner || !view.view->visibilityFlags;
    default:return true;
    }
}
}

WorldParticleRenderSummary RenderWorldParticles(const WorldParticleSystem& system,
    const ChopperEffectData& catalogue,const CarCinematicScreen& view,EcstasyFrame& frame)
{
    WorldParticleRenderSummary result;
    if(!system.firstGroup)return result;
    Require(view.id>=0 && view.id<32 && view.view && view.target && view.matricesCalculated,
        "Particle rendering lacks the current calculated source view");
    // RenderEffects128EB8 obtains its focal input from CalculateH104448;
    // the released display is512 wide. Main PC views scale that focal input
    // to their actual height. The authored pixel cap must use the same scale,
    // otherwise a growing nebula becomes a fixed512-pixel patch at4K.
    const bool nativeRaster=system.cameras && system.cameras->projection.nativeAspectCorrection &&
        (view.id==1 || view.id==2);
    float particlePixelScale=1.0f;
    if(nativeRaster) {
        Require(std::isfinite(view.pixelScale) && view.pixelScale>0.0f &&
            std::isfinite(view.sourcePixelScale) && view.sourcePixelScale>0.0f,
            "Particle view has no calculated source/native focal scales");
        particlePixelScale=view.pixelScale/view.sourcePixelScale;
    }
    const auto depthRange=ecstasyNativeDepthRange(view.matrices.clipToScreen,view.target->format);
    const auto* projection=view.matrices.clipToScreen;
    Require(projection[0]!=0.0f && projection[5]!=0.0f,
        "Particle projection has no native pixel scale");
    // Reuse one producer buffer across groups. AttachParticleGroup copies
    // directly into the frame's contiguous native storage before it clears.
    std::vector<EcstasyParticleQuad> quads;
    for(const auto* group=system.firstGroup;group;group=group->next) {
        ++result.groups;
        Require(group->occupied && group->definition,"Particle list contains a retired definition/group");
        if(!particleCarVisible(*group,view)) {++result.hiddenCarGroups;continue;}
        const auto& definition=*group->definition;
        const auto* descriptor=catalogue.Texture(definition);
        if(!descriptor) {++result.missingTextures;continue;}
        if(system.renderSelectedOnly && system.selectedEmitter && group->emitter!=system.selectedEmitter)continue;
        const auto* texture=textureInfoForDescriptor(descriptor);
        Require(texture && texture->pixels.Data(),"Particle definition points to an unretained texture");
        const auto widthPower=static_cast<std::uint8_t>(definition.atlasWidthPower);
        const auto heightPower=static_cast<std::uint8_t>(definition.atlasHeightPower);
        Require(definition.atlasWidthPower>=0 && definition.atlasHeightPower>=0 &&
            widthPower<=30 && heightPower<=30,"Particle atlas dimensions exceed their authored domain");
        const auto width=std::uint32_t(1)<<widthPower,height=std::uint32_t(1)<<heightPower;
        const float du=1.0f/static_cast<float>(width),dv=1.0f/static_cast<float>(height);
        const float inverseLifetime=1.0f/definition.lifetime;
        const float sourceMaximum=definition.maximumPixelSize==0.0f ? 512.0f:definition.maximumPixelSize;
        const float maximum=sourceMaximum*particlePixelScale;
        quads.clear();
        float depthSum=0.0f;
        for(const auto* particle=group->first;particle;particle=particle->next) {
            ++result.particles;
            if(!particle->occupied)throw std::runtime_error("Particle list contains a retired particle");
            const float age=1.0f-(static_cast<float>(particle->remainingTicks)*0.0009765625f)*inverseLifetime;
            const float point[4]={particle->position[0],particle->position[1],particle->position[2],1.0f};
            if(eGetClipFlags(point,view.matrices.worldToClip)) {++result.centerClipped;continue;}
            float camera[4],clip[4],size[4];
            eMulVector4(camera,view.matrices.worldToView,point);
            eMulVector4(clip,view.matrices.worldToClip,point);
            worldParticleHermiteEvaluate(size,definition.sizeRotationCurve,age);
            float pixels=size[0]>0.0f ? (size[0]*view.pixelScale)/camera[2]:0.0f;
            if(maximum<pixels)pixels=maximum;
            if(!(pixels>1.0f)) {++result.sizeClipped;continue;}
            if(!(camera[2]<definition.interestDistance)) {++result.distanceClipped;continue;}
            const auto delta=static_cast<std::uint16_t>(convertFloatToInt32(size[1]));
            const auto rotation=static_cast<std::uint16_t>((particle->rotation&1u) ?
                particle->rotation-delta:particle->rotation+delta);
            EcstasyParticleQuad quad;
            Require(std::isfinite(clip[3]) && clip[3]>0.0f,"Particle clip divisor is invalid");
            const float z=clip[2]/clip[3];
            const float divisor=projection[11]*z+projection[15];
            Require(std::isfinite(divisor) && divisor!=0.0f,"Particle screen divisor is invalid");
            const float scaleX=projection[0]/divisor,scaleY=projection[5]/divisor;
            // Pixel-centre snapping is an authored visual convention. Work
            // around the native viewport centre, without the GS2048 bias,
            // unsigned12.4 wrapping, or integer bits carried in float Z.
            const float centerX=std::floor((clip[0]/clip[3])*scaleX);
            const float centerY=std::floor((clip[1]/clip[3])*scaleY);
            quad.depth=depthRange[0]+(depthRange[1]-depthRange[0])*(z+1.0f)*0.5f;
            if(rotation==0) {
                const float halfX=std::trunc(pixels*8.0f)*(1.0f/16.0f);
                const float halfY=std::trunc(pixels*7.0f)*(1.0f/16.0f);
                for(std::size_t i=0;i<4;++i) {
                    quad.clipXY[i][0]=(centerX+((i&1u) ? halfX:-halfX))/scaleX;
                    quad.clipXY[i][1]=(centerY+((i&2u) ? halfY:-halfY))/scaleY;
                }
            } else {
                float sine,cosine;motionAngleComponents(rotation,&sine,&cosine);
                for(std::size_t i=0;i<4;++i) {
                    const float px=(i&1u) ? pixels:-pixels,py=(i&2u) ? pixels:-pixels;
                    // PAL1293DC..12946C: rotate, apply the authored X8/Y7
                    // proportions, then retain its sixteenth-pixel grid.
                    const float rotatedX=(cosine*px)+((-sine)*py);
                    const float rotatedY=(sine*px)+(cosine*py);
                    quad.clipXY[i][0]=(std::floor(rotatedX*8.0f+centerX*16.0f)*(1.0f/16.0f))/scaleX;
                    quad.clipXY[i][1]=(std::floor(rotatedY*7.0f+centerY*16.0f)*(1.0f/16.0f))/scaleY;
                }
            }
            float color[4];worldParticleHermiteEvaluate(color,definition.colorCurve,age);
            for(std::size_t lane=0;lane<4;++lane) {
                if(!std::isfinite(color[lane]))throw std::runtime_error("Particle curve produced a nonfinite color");
                const auto component=static_cast<std::uint32_t>(convertFloatToInt32(std::clamp(color[lane],0.0f,255.0f)));
                quad.color|=component<<(lane*8);
            }
            const float u=static_cast<float>(particle->atlasFrame&(width-1u))*du;
            const float v=static_cast<float>((particle->atlasFrame>>widthPower)&(height-1u))*dv;
            quad.uv={{{u,v},{u+du,v},{u,v+dv},{u+du,v+dv}}};
            depthSum=depthSum+static_cast<float>(convertFloatToInt32(camera[2]));
            quads.push_back(quad);++result.drawnParticles;
        }
        if(!quads.empty()) {
            frame.AttachParticleGroup(quads,*descriptor,view,depthSum/static_cast<float>(quads.size()));
            ++result.drawnGroups;
        }
    }
    // eAttachDirectData submits each group in source list order. Its single
    // average-depth alpha key is sorted with scenery and cars by the later
    // AttachAlphaPolys flush, without reordering this packet's own quads.
    return result;
}
} // namespace hp2
