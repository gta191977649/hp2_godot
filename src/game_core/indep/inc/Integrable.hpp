#ifndef NFSHP2_INDEP_INTEGRABLE_HPP
#define NFSHP2_INDEP_INTEGRABLE_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/Engine.hpp"
#include "indep/inc/Wheel.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// Native representation of the original integrable component interface.
// Concrete Engine, Wheel, Drivetrain and Suspension components supply these
// operations as their original implementations are recovered.
typedef struct PhysicsComponent {
    void* object;
    int32_t (*stateSize)(void* object);
    float* (*state)(void* object);
    void (*derivatives)(void* object, float* output);
} PhysicsComponent;

// The original game's integration evaluates derivatives first, obtains the
// state array afterwards, then adds derivative[i] * elapsed to each value.
// Caller owns scratch storage, avoiding per-tick allocation or a fixed limit.
// This method does not substitute any vehicle simulation component.
typedef enum PhysicsIntegrateResult {
    PHYSICS_INTEGRATE_OK = 0,
    PHYSICS_INTEGRATE_INVALID_COMPONENT,
    PHYSICS_INTEGRATE_INSUFFICIENT_STORAGE
} PhysicsIntegrateResult;
PhysicsIntegrateResult physicsComponentIntegrate(PhysicsComponent component,
                                                  float elapsed,
                                                  float* scratch,
                                                  size_t scratchCount);

PhysicsIntegrateResult physicsIntegrateEngine(Engine* engine, float elapsed);
PhysicsIntegrateResult physicsIntegrateWheel(Wheel* wheel,
                                              const WheelStepContext* context);

#ifdef __cplusplus
}
#endif

#endif
