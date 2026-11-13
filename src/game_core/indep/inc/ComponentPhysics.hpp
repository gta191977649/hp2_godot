#ifndef NFSHP2_INDEP_INC_COMPONENTPHYSICS_HPP
#define NFSHP2_INDEP_INC_COMPONENTPHYSICS_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/Engine.hpp"
#include "indep/inc/Wheel.hpp"
#include "indep/inc/RigidBody.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// Native component operations retain the original state/derivative/preparation
// contract. applyDelta is distinct from restoring a saved state: concrete
// restoration can have additional effects that integration must not invoke.
typedef struct ComponentOperations {
    int32_t (*stateSize)(void* object);
    void (*saveState)(void* object, float* output);
    void (*restoreState)(void* object, const float* input);
    void (*derivatives)(void* object, const void* context, float* output);
    void (*prepare)(void* object, const void* context);
    void (*applyDelta)(void* object, const float* derivatives, float elapsed);
} ComponentOperations;

typedef struct ComponentBinding {
    void* object;
    const void* context;
    const ComponentOperations* operations;
} ComponentBinding;

typedef struct ComponentSystem {
    ComponentBinding* components;
    size_t capacity;
    size_t count;
    int32_t stateCount;
    void* stageOwner;
    void (*prepareStage)(void* owner);
} ComponentSystem;

typedef enum ComponentSystemResult {
    COMPONENT_SYSTEM_OK,
    COMPONENT_SYSTEM_INVALID_ARGUMENT,
    COMPONENT_SYSTEM_FULL,
    COMPONENT_SYSTEM_DUPLICATE,
    COMPONENT_SYSTEM_INSUFFICIENT_STORAGE
} ComponentSystemResult;

// The system borrows both its component bindings and component objects.
// Native ownership is managed by the physics pools and their owning car.
void componentSystemInitialize(ComponentSystem* system, ComponentBinding* storage, size_t capacity);
ComponentSystemResult componentSystemAdd(ComponentSystem* system, ComponentBinding component);
int32_t componentSystemStateSize(const ComponentSystem* system);
ComponentSystemResult componentSystemDerivatives(const ComponentSystem* system, float* output, size_t count);
ComponentSystemResult componentSystemSaveState(const ComponentSystem* system, float* output, size_t count);
ComponentSystemResult componentSystemRestoreState(ComponentSystem* system, const float* input, size_t count);
ComponentSystemResult componentSystemPrepare(ComponentSystem* system);
ComponentSystemResult componentSystemIntegrateEuler(ComponentSystem* system, float elapsed,
                                                   float* scratch, size_t count);
// Runge-Kutta uses seven state-sized buffers, supplied by the owning simulation.
ComponentSystemResult componentSystemIntegrateRungeKutta(ComponentSystem* system, float elapsed,
                                                        float* scratch, size_t count);
void componentStateScale(float* output, const float* input, int32_t count, float scale);
void componentStateAdd(float* output, const float* first, const float* second, int32_t count);
void componentStateMultiplyAdd(float* output, const float* first, const float* second,
                               int32_t count, float scale);

ComponentBinding componentBindEngine(Engine* engine);
ComponentBinding componentBindDriveTrain(DriveTrain* driveTrain);
ComponentBinding componentBindWheel(Wheel* wheel, const WheelStepContext* context);
ComponentBinding componentBindRigidBody(RigidBody* body);

#ifdef __cplusplus
}
#endif

#endif
