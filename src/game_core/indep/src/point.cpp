#include "indep/inc/Point.hpp"
#include "bware/inc/bMath.hpp"
static inline void tCubicSetValue(tCubic1D* axes, const float* value, int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        axes[index].value = value[index];
        if (value[index] != axes[index].desiredValue) axes[index].mode = 2;
    }
}

static inline void tCubicSetDerivative(tCubic1D* axes, const float* value, int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        axes[index].derivative = value[index];
        if (value[index] != axes[index].desiredDerivative) axes[index].mode = 2;
    }
}

static inline void tCubicSetDesiredValue(tCubic1D* axes, const float* value, int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        axes[index].desiredValue = value[index];
        if (value[index] != axes[index].value) axes[index].mode = 2;
    }
}

#include "indep/inc/Point.hpp"

#include <math.h>
#include <string.h>

void tCubic1DMakeCoeffs(tCubic1D* curve)
{
    const float difference = bScalarAdd(curve->desiredValue, -(curve->value));
    const float twiceDerivative = bScalarAdd(curve->derivative, curve->derivative);
    const float threeDifference = bScalarMultiply(difference, 3.0f);
    const float derivativeSum = bScalarAdd(curve->derivative, curve->desiredDerivative);
    const float twiceDifference = bScalarAdd(difference, difference);
    curve->constant = curve->value;
    curve->linear = curve->derivative;
    curve->cubic = bScalarAdd(derivativeSum, -(twiceDifference));
    curve->quadratic = bScalarAdd((bScalarAdd(threeDifference, -(curve->desiredDerivative))), -(twiceDerivative));
}

float tCubic1DGetVal(const tCubic1D* curve, float parameter)
{
    const float cubic = bScalarMultiply(curve->cubic, parameter);
    const float quadratic = bScalarMultiply((bScalarAdd(cubic, curve->quadratic)), parameter);
    const float linear = bScalarMultiply((bScalarAdd(quadratic, curve->linear)), parameter);
    return bScalarAdd(linear, curve->constant);
}

float tCubic1DGetdVal(const tCubic1D* curve, float parameter)
{
    const float threeParameter = bScalarMultiply(parameter, 3.0f);
    const float twiceQuadratic = bScalarAdd(curve->quadratic, curve->quadratic);
    const float first = bScalarMultiply(curve->cubic, threeParameter);
    const float second = bScalarMultiply((bScalarAdd(first, twiceQuadratic)), parameter);
    return bScalarAdd(second, curve->linear);
}

float tCubic1DGetddVal(const tCubic1D* curve, float parameter)
{
    const float sixParameter = bScalarMultiply(parameter, 6.0f);
    const float twiceQuadratic = bScalarAdd(curve->quadratic, curve->quadratic);
    return bScalarAdd(bScalarMultiply(curve->cubic, sixParameter), twiceQuadratic);
}

float tCubic1DVelocityAtTime(const tCubic1D* curve, float time)
{
    const float inverseDuration = bScalarFiniteRatio(1.0f, curve->duration);
    const float value = tCubic1DGetdVal(curve, bScalarMultiply(time, inverseDuration));
    return bScalarMultiply(value, inverseDuration);
}

float tCubic1DAccelerationAtTime(const tCubic1D* curve, float time)
{
    const float inverseDuration = bScalarFiniteRatio(1.0f, curve->duration);
    const float value = tCubic1DGetddVal(curve, bScalarMultiply(time, inverseDuration));
    const float inverseSquared = bScalarMultiply(inverseDuration, inverseDuration);
    return bScalarMultiply(value, inverseSquared);
}

void tCubic1DLimitEndVelocity(tCubic1D* curve, float maximum)
{
    const float velocity = tCubic1DVelocityAtTime(curve, curve->duration);
    const float magnitude = fabsf(velocity);
    if (maximum < magnitude) {
        const float sign = bScalarFiniteRatio(magnitude, velocity);
        const float bounded = bScalarMultiply(sign, maximum);
        curve->desiredDerivative = bScalarMultiply(bounded, curve->duration);
    }
}

void tCubic2DSetVal(tCubic2D* curve, const float value[2])
{ tCubicSetValue(curve->axes, value, 2); }
void tCubic2DSetdVal(tCubic2D* curve, const float value[2])
{ tCubicSetDerivative(curve->axes, value, 2); }
void tCubic2DSetValDesired(tCubic2D* curve, const float value[2])
{ tCubicSetDesiredValue(curve->axes, value, 2); }
void tCubic2DGetVal(const tCubic2D* curve, float value[2])
{
    for (int axis = 0; axis < 2; ++axis) value[axis] = curve->axes[axis].value;
}
void tCubic2DGetdVal(const tCubic2D* curve, float value[2])
{
    for (int axis = 0; axis < 2; ++axis) value[axis] = curve->axes[axis].derivative;
}
void tCubic2DGetddVal(const tCubic2D* curve, float value[2])
{
    for (int axis = 0; axis < 2; ++axis) {
        const tCubic1D* scalar = &curve->axes[axis];
        value[axis] = tCubic1DGetddVal(scalar, finiteRatio(scalar->parameter, scalar->duration));
    }
}

void tCubic3DSetVal(tCubic3D* curve, const float value[3])
{ tCubicSetValue(curve->axes, value, 3); }
void tCubic3DSetdVal(tCubic3D* curve, const float value[3])
{ tCubicSetDerivative(curve->axes, value, 3); }
void tCubic3DSetValDesired(tCubic3D* curve, const float value[3])
{ tCubicSetDesiredValue(curve->axes, value, 3); }
void tCubic3DSetdValDesired(tCubic3D* curve, const float value[3])
{
    for (int axis = 0; axis < 3; ++axis) curve->axes[axis].desiredDerivative = value[axis];
}
void tCubic3DSetDuration(tCubic3D* curve, const float value[3])
{
    for (int axis = 0; axis < 3; ++axis) curve->axes[axis].duration = value[axis];
}
void tCubic3DSetThreshold(tCubic3D* curve, const float value[3])
{
    for (int axis = 0; axis < 3; ++axis) curve->axes[axis].threshold = value[axis];
}
void tCubic3DGetVal(const tCubic3D* curve, float value[3])
{
    for (int axis = 0; axis < 3; ++axis) value[axis] = curve->axes[axis].value;
}
void tCubic3DGetdVal(const tCubic3D* curve, float value[3])
{
    for (int axis = 0; axis < 3; ++axis) value[axis] = curve->axes[axis].derivative;
}
void tCubic3DGetddVal(const tCubic3D* curve, float value[3])
{
    for (int axis = 0; axis < 3; ++axis) {
        const tCubic1D* scalar = &curve->axes[axis];
        value[axis] = tCubic1DGetddVal(scalar, finiteRatio(scalar->parameter, scalar->duration));
    }
}
void tCubic3DGetValDesired(const tCubic3D* curve, float value[3])
{
    for (int axis = 0; axis < 3; ++axis) value[axis] = curve->axes[axis].desiredValue;
}
void tCubic3DGetdValDesired(const tCubic3D* curve, float value[3])
{
    for (int axis = 0; axis < 3; ++axis) value[axis] = curve->axes[axis].desiredDerivative;
}

// Remaining accepted PAL cubic operations, co-located with cPoint.
void tCubic1DInitialize(tCubic1D* curve)
{
    memset(curve, 0, sizeof(*curve));
    curve->duration = 1.0f;
    curve->threshold = 1.0f;
    curve->continuous = 1;
}

void tCubic1DLimitAcceleration(tCubic1D* curve, float maximum)
{
    float first = tCubic1DAccelerationAtTime(curve, 0.0f);
    const float firstMagnitude = fabsf(first);
    float last = tCubic1DAccelerationAtTime(curve, curve->duration);
    const float lastMagnitude = fabsf(last);
    int32_t changed = 0;
    if (maximum < firstMagnitude) {
        first = bScalarMultiply(bScalarFiniteRatio(firstMagnitude, first), maximum);
        changed = 1;
    }
    if (maximum < lastMagnitude) {
        last = bScalarMultiply(bScalarFiniteRatio(lastMagnitude, last), maximum);
        changed = 1;
    }
    if (changed) {
        const float durationSquared = bScalarMultiply(curve->duration, curve->duration);
        first = bScalarMultiply(first, durationSquared);
        last = bScalarMultiply(last, durationSquared);
        curve->quadratic = bScalarMultiply(first, 0.5f);
        curve->cubic = bScalarMultiply((bScalarAdd(last, -(first))), 0.166666671633720398f);
    }
}

void tCubic1DUpdate(tCubic1D* curve, float elapsed,
                    float maximumVelocity, float maximumAcceleration)
{
    if (curve->mode == 2) {
        curve->parameter = 0.0f;
        if (!curve->continuous) curve->mode = 1;
        if (0.0f < maximumVelocity) tCubic1DLimitEndVelocity(curve, maximumVelocity);
        tCubic1DMakeCoeffs(curve);
        /* The original gate uses maximumAcceleration, while its clamp call
         * receives maximumVelocity. Keep that original argument order. */
        if (0.0f < maximumAcceleration) tCubic1DLimitAcceleration(curve, maximumVelocity);
    } else if (curve->mode != 1) {
        return;
    }
    const float step = bScalarFiniteRatio(elapsed, curve->duration);
    curve->parameter = bScalarAdd(curve->parameter, step);
    if (curve->threshold <= curve->parameter) {
        if (1.0f <= curve->threshold) {
            curve->value = curve->desiredValue;
            curve->derivative = curve->desiredDerivative;
        }
        curve->mode = 0;
    } else {
        curve->value = tCubic1DGetVal(curve, curve->parameter);
        curve->derivative = tCubic1DGetdVal(curve, curve->parameter);
    }
}
void tCubic2DSetdValDesired(tCubic2D* curve, const float value[2])
{
    for (int axis = 0; axis < 2; ++axis) curve->axes[axis].desiredDerivative = value[axis];
}
void tCubic2DSetDuration(tCubic2D* curve, const float value[2])
{
    for (int axis = 0; axis < 2; ++axis) curve->axes[axis].duration = value[axis];
}
void tCubic2DSetThreshold(tCubic2D* curve, const float value[2])
{
    for (int axis = 0; axis < 2; ++axis) curve->axes[axis].threshold = value[axis];
}
void tCubic2DGetValDesired(const tCubic2D* curve, float value[2])
{
    for (int axis = 0; axis < 2; ++axis) value[axis] = curve->axes[axis].desiredValue;
}
void tCubic2DGetdValDesired(const tCubic2D* curve, float value[2])
{
    for (int axis = 0; axis < 2; ++axis) value[axis] = curve->axes[axis].desiredDerivative;
}
void tCubic2DUpdate(tCubic2D* curve, float elapsed)
{
    for (int axis = 0; axis < 2; ++axis) tCubic1DUpdate(&curve->axes[axis], elapsed, 0.0f, 0.0f);
}
void tCubic3DUpdate(tCubic3D* curve, float elapsed)
{
    for (int axis = 0; axis < 3; ++axis) tCubic1DUpdate(&curve->axes[axis], elapsed, 0.0f, 0.0f);
}
