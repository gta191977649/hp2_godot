#include "indep/inc/ComponentPhysics.hpp"

#include <limits.h>
#include <stdint.h>
#include "bware/inc/bMath.hpp"



void componentStateScale(float* output, const float* input, int32_t count, float scale)
{
    for (int32_t i = 0; i < count; ++i) output[i] = bScalarMultiply(input[i], scale);
}
void componentStateAdd(float* output, const float* first, const float* second, int32_t count)
{
    for (int32_t i = 0; i < count; ++i) output[i] = bScalarAdd(first[i], second[i]);
}
void componentStateMultiplyAdd(float* output, const float* first, const float* second,
                               int32_t count, float scale)
{
    for (int32_t i = 0; i < count; ++i) {
        const float change = bScalarMultiply(second[i], scale);
        output[i] = bScalarAdd(first[i], change);
    }
}

static void saveBody(void* object, float* output)
{
    const RigidBodyState* state = &((RigidBody*)object)->motion;
    for (int i = 0; i < 4; ++i) output[i] = state->position[i];
    for (int i = 0; i < 16; ++i) output[4 + i] = state->orientation[i];
    for (int i = 0; i < 4; ++i) output[20 + i] = state->linearMomentum[i];
    for (int i = 0; i < 4; ++i) output[24 + i] = state->angularMomentum[i];
    output[28] = state->damping;
}

static void restoreBody(void* object, const float* input)
{
    RigidBodyState* state = &((RigidBody*)object)->motion;
    for (int i = 0; i < 3; ++i) state->position[i] = input[i];
    for (int i = 0; i < 16; ++i) state->orientation[i] = input[4 + i];
    for (int i = 0; i < 3; ++i) state->linearMomentum[i] = input[20 + i];
    for (int i = 0; i < 3; ++i) state->angularMomentum[i] = input[24 + i];
    state->position[3] = 1.0f;
    state->linearMomentum[3] = state->angularMomentum[3] = 0.0f;
    state->damping = input[28];
}

static int32_t bodySize(void* object) { return rigidBodyStateSize((RigidBody*)object); }
static void bodyDerivatives(void* object, const void* context, float* output)
{
    RigidBodyState state;
    (void)context;
    rigidBodyComputeDerivatives((RigidBody*)object, &state);
    for (int i = 0; i < 4; ++i) output[i] = state.position[i];
    for (int i = 0; i < 16; ++i) output[4 + i] = state.orientation[i];
    for (int i = 0; i < 4; ++i) output[20 + i] = state.linearMomentum[i];
    for (int i = 0; i < 4; ++i) output[24 + i] = state.angularMomentum[i];
    output[28] = state.damping;
}
static void prepareBody(void* object, const void* context)
{ (void)context; rigidBodyPrepareStep((RigidBody*)object); }
static void applyBodyDelta(void* object, const float* derivatives, float elapsed)
{
    RigidBodyState* state = &((RigidBody*)object)->motion;
    for (int i = 0; i < 3; ++i) {
        const float change = bScalarMultiply(elapsed, derivatives[i]);
        state->position[i] = bScalarAdd(state->position[i], change);
    }
    for (int i = 0; i < 16; ++i) {
        const float change = bScalarMultiply(elapsed, derivatives[4 + i]);
        state->orientation[i] = bScalarAdd(state->orientation[i], change);
    }
    for (int i = 0; i < 3; ++i) {
        const float change = bScalarMultiply(elapsed, derivatives[20 + i]);
        state->linearMomentum[i] = bScalarAdd(state->linearMomentum[i], change);
    }
    for (int i = 0; i < 3; ++i) {
        const float change = bScalarMultiply(elapsed, derivatives[24 + i]);
        state->angularMomentum[i] = bScalarAdd(state->angularMomentum[i], change);
    }
    // Homogeneous slots describe representation, not integrated state.
    state->position[3] = 1.0f;
    state->linearMomentum[3] = state->angularMomentum[3] = 0.0f;
    const float dampingChange = bScalarMultiply(elapsed, derivatives[28]);
    state->damping = bScalarAdd(state->damping, dampingChange);
}

static int32_t engineSize(void* object) { return engineAngularVelocityStateSize((Engine*)object); }
static void saveEngine(void* object, float* output) { engineSaveAngularVelocity((Engine*)object, output); }
static void restoreEngine(void* object, const float* input) { engineRestoreAngularVelocity((Engine*)object, input); }
static void engineDerivatives(void* object, const void* context, float* output)
{ (void)context; (void)engineComputeDerivatives((Engine*)object, output); }
static void prepareEngine(void* object, const void* context)
{ (void)context; engineConstrainAngularVelocity((Engine*)object); }
static void applyEngineDelta(void* object, const float* derivatives, float elapsed)
{
    Engine* engine = (Engine*)object;
    const float change = bScalarMultiply(elapsed, derivatives[0]);
    engine->angularVelocity = bScalarAdd(engine->angularVelocity, change);
}

static int32_t driveTrainSize(void* object)
{ return driveTrainClutchStateSize((DriveTrain*)object); }
static void saveDriveTrain(void* object, float* output)
{ driveTrainSaveClutch((DriveTrain*)object, output); }
static void restoreDriveTrain(void* object, const float* input)
{ driveTrainRestoreClutch((DriveTrain*)object, input); }
static void driveTrainDerivatives(void* object, const void* context, float* output)
{ (void)context; (void)driveTrainComputeDerivatives((DriveTrain*)object, output); }
static void prepareDriveTrain(void* object, const void* context)
{ (void)context; driveTrainPrepareStep((DriveTrain*)object); }
static void applyDriveTrainDelta(void* object, const float* derivatives, float elapsed)
{
    DriveTrain* driveTrain = (DriveTrain*)object;
    const float change = bScalarMultiply(elapsed, derivatives[0]);
    driveTrain->clutch = bScalarAdd(driveTrain->clutch, change);
}

static int32_t wheelSize(void* object) { return wheelStateSize((Wheel*)object); }
static void saveWheel(void* object, float* output)
{
    const Wheel* wheel = (Wheel*)object;
    output[0] = wheel->motion.angle;
    output[1] = wheel->motion.angularVelocity;
}
static void restoreWheel(void* object, const float* input)
{
    const WheelState state = {input[0], input[1]};
    wheelRestoreState((Wheel*)object, &state);
}
static void wheelDerivatives(void* object, const void* context, float* output)
{
    WheelState derivatives;
    (void)wheelComputeDerivatives((Wheel*)object, (const WheelStepContext*)context, &derivatives);
    output[0] = derivatives.angle;
    output[1] = derivatives.angularVelocity;
}
static void prepareWheel(void* object, const void* context)
{ (void)context; wheelPrepareStep((Wheel*)object); }
static void applyWheelDelta(void* object, const float* derivatives, float elapsed)
{
    Wheel* wheel = (Wheel*)object;
    const float angleChange = bScalarMultiply(elapsed, derivatives[0]);
    wheel->motion.angle = bScalarAdd(wheel->motion.angle, angleChange);
    const float speedChange = bScalarMultiply(elapsed, derivatives[1]);
    wheel->motion.angularVelocity = bScalarAdd(wheel->motion.angularVelocity, speedChange);
}

static const ComponentOperations bodyOperations = {
    bodySize, saveBody, restoreBody, bodyDerivatives, prepareBody, applyBodyDelta
};
static const ComponentOperations engineOperations = {
    engineSize, saveEngine, restoreEngine, engineDerivatives, prepareEngine, applyEngineDelta
};
static const ComponentOperations driveTrainOperations = {
    driveTrainSize, saveDriveTrain, restoreDriveTrain, driveTrainDerivatives,
    prepareDriveTrain, applyDriveTrainDelta
};
static const ComponentOperations wheelOperations = {
    wheelSize, saveWheel, restoreWheel, wheelDerivatives, prepareWheel, applyWheelDelta
};

ComponentBinding componentBindEngine(Engine* engine)
{
    const ComponentBinding binding = {engine, NULL, &engineOperations};
    return binding;
}
ComponentBinding componentBindDriveTrain(DriveTrain* driveTrain)
{
    const ComponentBinding binding = {driveTrain, NULL, &driveTrainOperations};
    return binding;
}
ComponentBinding componentBindWheel(Wheel* wheel, const WheelStepContext* context)
{
    const ComponentBinding binding = {wheel, context, &wheelOperations};
    return binding;
}
ComponentBinding componentBindRigidBody(RigidBody* body)
{
    const ComponentBinding binding = {body, NULL, &bodyOperations};
    return binding;
}

void componentSystemInitialize(ComponentSystem* system, ComponentBinding* storage, size_t capacity)
{
    system->components = storage;
    system->capacity = capacity;
    system->count = 0;
    system->stateCount = 0;
    system->stageOwner = NULL;
    system->prepareStage = NULL;
}

static int validSystem(const ComponentSystem* system)
{
    return system != NULL && system->count <= system->capacity &&
           system->stateCount >= 0 && (system->capacity == 0 || system->components != NULL);
}

ComponentSystemResult componentSystemAdd(ComponentSystem* system, ComponentBinding component)
{
    const ComponentOperations* operations = component.operations;
    if (!validSystem(system) || component.object == NULL || operations == NULL ||
        operations->stateSize == NULL || operations->saveState == NULL ||
        operations->restoreState == NULL || operations->derivatives == NULL ||
        operations->prepare == NULL || operations->applyDelta == NULL ||
        (operations == &wheelOperations && component.context == NULL))
        return COMPONENT_SYSTEM_INVALID_ARGUMENT;
    const int32_t size = operations->stateSize(component.object);
    if (size < 0 || size > INT32_MAX - system->stateCount)
        return COMPONENT_SYSTEM_INVALID_ARGUMENT;
    for (size_t i = 0; i < system->count; ++i)
        if (system->components[i].object == component.object) return COMPONENT_SYSTEM_DUPLICATE;
    if (system->count == system->capacity) return COMPONENT_SYSTEM_FULL;
    system->components[system->count++] = component;
    system->stateCount += size;
    return COMPONENT_SYSTEM_OK;
}

static int validStorage(const ComponentSystem* system, const float* storage, size_t count)
{
    return validSystem(system) && (size_t)system->stateCount <= count &&
           (system->stateCount == 0 || storage != NULL);
}

ComponentSystemResult componentSystemDerivatives(const ComponentSystem* system, float* output, size_t count)
{
    if (!validStorage(system, output, count)) return COMPONENT_SYSTEM_INSUFFICIENT_STORAGE;
    size_t offset = 0;
    for (size_t i = 0; i < system->count; ++i) {
        ComponentBinding component = system->components[i];
        const int32_t size = component.operations->stateSize(component.object);
        component.operations->derivatives(component.object, component.context, output + offset);
        offset += (size_t)size;
    }
    return COMPONENT_SYSTEM_OK;
}

ComponentSystemResult componentSystemSaveState(const ComponentSystem* system, float* output, size_t count)
{
    if (!validStorage(system, output, count)) return COMPONENT_SYSTEM_INSUFFICIENT_STORAGE;
    size_t offset = 0;
    for (size_t i = 0; i < system->count; ++i) {
        ComponentBinding component = system->components[i];
        component.operations->saveState(component.object, output + offset);
        offset += (size_t)component.operations->stateSize(component.object);
    }
    return COMPONENT_SYSTEM_OK;
}

ComponentSystemResult componentSystemRestoreState(ComponentSystem* system, const float* input, size_t count)
{
    if (!validStorage(system, input, count)) return COMPONENT_SYSTEM_INSUFFICIENT_STORAGE;
    size_t offset = 0;
    for (size_t i = 0; i < system->count; ++i) {
        ComponentBinding component = system->components[i];
        component.operations->restoreState(component.object, input + offset);
        offset += (size_t)component.operations->stateSize(component.object);
    }
    return COMPONENT_SYSTEM_OK;
}

ComponentSystemResult componentSystemPrepare(ComponentSystem* system)
{
    if (!validSystem(system)) return COMPONENT_SYSTEM_INVALID_ARGUMENT;
    for (size_t i = 0; i < system->count; ++i) {
        ComponentBinding component = system->components[i];
        component.operations->prepare(component.object, component.context);
    }
    return COMPONENT_SYSTEM_OK;
}

static ComponentSystemResult prepareAfterIntegration(ComponentSystem* system)
{
    if (system->prepareStage != NULL) {
        system->prepareStage(system->stageOwner);
        return COMPONENT_SYSTEM_OK;
    }
    return componentSystemPrepare(system);
}

ComponentSystemResult componentSystemIntegrateEuler(ComponentSystem* system, float elapsed,
                                                   float* scratch, size_t count)
{
    if (!validStorage(system, scratch, count)) return COMPONENT_SYSTEM_INSUFFICIENT_STORAGE;
    ComponentSystemResult result = componentSystemDerivatives(system, scratch, count);
    if (result != COMPONENT_SYSTEM_OK) return result;
    size_t offset = 0;
    for (size_t i = 0; i < system->count; ++i) {
        ComponentBinding component = system->components[i];
        component.operations->applyDelta(component.object, scratch + offset, elapsed);
        offset += (size_t)component.operations->stateSize(component.object);
    }
    return prepareAfterIntegration(system);
}

ComponentSystemResult componentSystemIntegrateRungeKutta(ComponentSystem* system, float elapsed,
                                                        float* scratch, size_t count)
{
    if (!validSystem(system)) return COMPONENT_SYSTEM_INVALID_ARGUMENT;
    const size_t size = (size_t)system->stateCount;
    if (size > SIZE_MAX / 7 || size * 7 > count || (size && scratch == NULL))
        return COMPONENT_SYSTEM_INSUFFICIENT_STORAGE;
    if (!size) return prepareAfterIntegration(system);
    float* initial = scratch;
    float* derivatives = scratch + size;
    float* temporary = scratch + 2 * size;
    float* first = scratch + 3 * size;
    float* second = scratch + 4 * size;
    float* third = scratch + 5 * size;
    float* fourth = scratch + 6 * size;
    const int32_t states = system->stateCount;
    componentSystemSaveState(system, initial, size);
    componentSystemDerivatives(system, derivatives, size);
    const float halfElapsed = elapsed * 0.5f;
    componentStateScale(first, derivatives, states, halfElapsed);
    componentStateAdd(temporary, initial, first, states);
    componentSystemRestoreState(system, temporary, size);
    prepareAfterIntegration(system);
    componentSystemDerivatives(system, derivatives, size);
    componentStateScale(second, derivatives, states, halfElapsed);
    componentStateAdd(temporary, initial, second, states);
    componentSystemRestoreState(system, temporary, size);
    prepareAfterIntegration(system);
    componentSystemDerivatives(system, derivatives, size);
    componentStateScale(third, derivatives, states, elapsed);
    componentStateAdd(temporary, initial, third, states);
    componentSystemRestoreState(system, temporary, size);
    prepareAfterIntegration(system);
    componentSystemDerivatives(system, derivatives, size);
    componentStateScale(fourth, derivatives, states, elapsed);
    componentStateMultiplyAdd(temporary, initial, first, states, 0.3333333432674408f);
    componentStateMultiplyAdd(temporary, temporary, second, states, 0.6666666865348816f);
    componentStateMultiplyAdd(temporary, temporary, third, states, 0.3333333432674408f);
    componentStateMultiplyAdd(temporary, temporary, fourth, states, 0.1666666716337204f);
    componentSystemRestoreState(system, temporary, size);
    return prepareAfterIntegration(system);
}

int32_t componentSystemStateSize(const ComponentSystem* system)
{ return system->stateCount; }
