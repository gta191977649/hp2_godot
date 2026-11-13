#include "indep/inc/Integrable.hpp"

static int32_t engineComponentSize(void* object)
{
    return engineAngularVelocityStateSize((Engine*)object);
}

static float* engineComponentState(void* object)
{
    return engineAngularVelocityState((Engine*)object);
}

static void engineComponentDerivatives(void* object, float* output)
{
    (void)engineComputeDerivatives((Engine*)object, output);
}

PhysicsIntegrateResult physicsIntegrateEngine(Engine* engine, float elapsed)
{
    const PhysicsComponent component = {
        engine, engineComponentSize, engineComponentState,
        engineComponentDerivatives
    };
    float scratch;
    return physicsComponentIntegrate(component, elapsed, &scratch, 1);
}

PhysicsIntegrateResult physicsIntegrateWheel(Wheel* wheel,
                                              const WheelStepContext* context)
{
    if (wheel == NULL || context == NULL)
        return PHYSICS_INTEGRATE_INVALID_COMPONENT;
    WheelState derivatives;
    (void)wheelComputeDerivatives(wheel, context, &derivatives);
    // The native state has named members. Updating each member avoids
    // treating distinct struct members as an array through pointer arithmetic.
    const float angleChange = derivatives.angle * context->elapsed;
    wheel->motion.angle = wheel->motion.angle + angleChange;
    const float velocityChange = derivatives.angularVelocity * context->elapsed;
    wheel->motion.angularVelocity = wheel->motion.angularVelocity + velocityChange;
    return PHYSICS_INTEGRATE_OK;
}

PhysicsIntegrateResult physicsComponentIntegrate(PhysicsComponent component,
                                                  float elapsed,
                                                  float* scratch,
                                                  size_t scratchCount)
{
    if (component.object == NULL || component.stateSize == NULL ||
        component.state == NULL || component.derivatives == NULL)
        return PHYSICS_INTEGRATE_INVALID_COMPONENT;

    const int32_t count = component.stateSize(component.object);
    if (count <= 0) {
        // The original still calls both virtual operations before deciding
        // whether to execute the accumulation loop. Their implementations
        // must accept a zero-state component.
        if (scratch == NULL || scratchCount == 0)
            return PHYSICS_INTEGRATE_INSUFFICIENT_STORAGE;
        component.derivatives(component.object, scratch);
        (void)component.state(component.object);
        return PHYSICS_INTEGRATE_OK;
    }
    if (scratch == NULL || (size_t)count > scratchCount)
        return PHYSICS_INTEGRATE_INSUFFICIENT_STORAGE;

    component.derivatives(component.object, scratch);
    float* values = component.state(component.object);
    if (values == NULL) return PHYSICS_INTEGRATE_INVALID_COMPONENT;
    for (int32_t i = 0; i < count; ++i) {
        const float change = scratch[i] * elapsed;
        values[i] = values[i] + change;
    }
    return PHYSICS_INTEGRATE_OK;
}
