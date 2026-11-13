#ifndef NFSHP2_BWARE_INC_BMATH_HPP
#define NFSHP2_BWARE_INC_BMATH_HPP

#include <math.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Native PC floating-point operations. Historical bScalar* symbol names are
// retained for binary/source compatibility, not as a PS2 FPU/VU emulator.
// Render and physics use hardware float arithmetic; no bit-level FPU model.
float bScalarAdd(float left, float right);
float bScalarMultiply(float left, float right);
float bScalarDivide(float numerator, float denominator);
float bScalarFiniteRatio(float numerator, float denominator);
float bScalarSquareRoot(float value);

// Component ratios retain the PC domain guard for an actual zero divisor
// and overflowing quotient. Nonzero divisors and representable subnormal
// results use native IEEE arithmetic; no console denormal flushing remains.
static inline float finiteRatio(float numerator, float denominator)
{
    if (denominator == 0.0f) {
        const int negative = (signbit(numerator) != 0) != (signbit(denominator) != 0);
        return negative ? -FLT_MAX : FLT_MAX;
    }
    const float quotient = numerator / denominator;
    if (isinf(quotient))
        return copysignf(FLT_MAX, quotient);
    return quotient;
}

static inline int32_t convertFloatToInt32(float value)
{
    if (isnan(value)) return 0;
    if (value >= 2147483648.0f) return INT32_MAX;
    if (value < -2147483648.0f) return INT32_MIN;
    return (int32_t)value;
}

static inline float maximumFinite(float first, float second)
{
    return first < second ? second : first;
}

static inline float minimumFinite(float first, float second)
{
    return second < first ? second : first;
}

/* Arrays are native storage, not console pointers. Matrices contain four
 * consecutive columns. Quaternion order is x,y,z,w. Four-lane operations
 * require four accessible floats, including the documented W lane.
 * Inputs must be finite, normal-range values; inverse matrices must have a
 * nonzero determinant. Floating coordinates converted to integers must fit
 * int32_t; wrap periods and random bounds must be nonzero. Fixed atan input
 * must exclude INT32_MIN, whose absolute value is not positive int32_t.
 * Bounds minimum and maximum arrays must occupy distinct storage.
 * Output may alias an input unless stated otherwise. */
float* vectorNormalize2(float output[2], const float input[2]);
float* vectorSetLength2(float output[2], const float input[2], float length);
float* vectorSetLength3(float output[3], const float input[4], float length);
float* vectorMultiplyAdd2(float output[2], const float first[2], const float second[2], float scale);
int vectorNear2(const float first[2], const float second[2], float tolerance);
int vectorNear3(const float first[3], const float second[3], float tolerance);
uint32_t vectorIntegerSquareRoot(uint32_t value);
int32_t vectorFixedReciprocal(int32_t value);
int32_t vectorFixedDivide(int32_t numerator, int32_t denominator);
int32_t vectorFixedLength2(const int32_t value[2]);
int32_t vectorFixedLength3(const int32_t value[3]);
int32_t vectorFixedDistance2(const int32_t first[2], const int32_t second[2]);
int32_t vectorFixedDistance3(const int32_t first[3], const int32_t second[3]);
int32_t* vectorFixedScale2(int32_t output[2], const int32_t input[2], int32_t scale);
int32_t* vectorFixedScale3(int32_t output[3], const int32_t input[3], int32_t scale);
int32_t* vectorFixedMultiplyAdd2(int32_t output[2], const int32_t first[2], const int32_t second[2], int32_t scale);
int32_t* vectorFixedMultiplyAdd3(int32_t output[3], const int32_t first[3], const int32_t second[3], int32_t scale);
int32_t vectorFixedDot2(const int32_t first[2], const int32_t second[2]);
int32_t vectorFixedDot3(const int32_t first[3], const int32_t second[3]);
int32_t* vectorFixedCross3(int32_t output[3], const int32_t first[3], const int32_t second[3]);
int32_t* vectorFixedNormalize2(int32_t output[2], const int32_t input[2]);
int32_t* vectorFixedNormalize3(int32_t output[3], const int32_t input[3]);

void vectorBoundsInitialize2(float minimum[2], float maximum[2]);
void vectorBoundsFromPoint2(float minimum[2], float maximum[2], const float point[2]);
void vectorBoundsExpandPoint2(float minimum[2], float maximum[2], const float point[2]);
void vectorBoundsExpandRadius2(float minimum[2], float maximum[2], const float point[2], float radius);
void vectorBoundsExpandBounds2(float minimum[2], float maximum[2], const float otherMinimum[2], const float otherMaximum[2]);
int vectorBoundsContainsPoint2(const float minimum[2], const float maximum[2], const float point[2], float tolerance);
int vectorBoundsContainsBounds2(const float minimum[2], const float maximum[2], const float otherMinimum[2], const float otherMaximum[2]);
int vectorBoundsOverlaps2(const float minimum[2], const float maximum[2], const float otherMinimum[2], const float otherMaximum[2]);
void vectorBoundsInitialize3(float minimum[4], float maximum[4]);
void vectorBoundsFromPoint3(float minimum[4], float maximum[4], const float point[4]);
void vectorBoundsExpandPoint3(float minimum[4], float maximum[4], const float point[4]);
void vectorBoundsExpandBounds3(float minimum[4], float maximum[4], const float otherMinimum[4], const float otherMaximum[4]);
int vectorBoundsContainsPoint3(const float minimum[3], const float maximum[3], const float point[3], float tolerance);
int vectorBoundsContainsBounds3(const float minimum[3], const float maximum[3], const float otherMinimum[3], const float otherMaximum[3]);
int vectorBoundsOverlaps3(const float minimum[3], const float maximum[3], const float otherMinimum[3], const float otherMaximum[3]);

/* This direct conversion requires distinct input/output storage. */
float* vectorQuaternionFromMatrix(float output[4], const float matrix[16]);
void vectorQuaternionToMatrix(const float quaternion[4], float matrix[16]);
float* vectorQuaternionSlerp(float output[4], const float first[4], const float second[4], float fraction);
/* This interpolation retains the selected hemisphere in the first input.
 * All three arrays must occupy distinct storage. */
void vectorQuaternionInterpolate(float output[4], float first[4], float second[4], float fraction);
float* vectorQuaternionConjugate(float output[4], const float input[4]);
float vectorQuaternionLength(const float quaternion[4]);
float* vectorQuaternionNormalize(float output[4], const float input[4]);
float* vectorQuaternionMultiply(float output[4], const float first[4], const float second[4]);
void vectorMatrixFromQuaternion(float matrix[16], const float quaternion[4]);
void vectorQuaternionFromMatrixCopy(float output[4], const float matrix[16]);
void vectorMatrixInverseAffine(float output[16], const float matrix[16]);
void vectorMatrixMultiply(float output[16], const float left[16], const float right[16]);
void vectorMatrixTransform(float output[4], const float matrix[16], const float value[4]);
void vectorMatrixTransformPoint(float output[4], const float matrix[16], const float value[4]);
float* vectorMatrixTranspose(float output[16], const float matrix[16]);

uint32_t vectorRandomBounded(uint32_t bound, uint32_t* state);
float vectorRandomFloat(uint32_t* state, float maximum);
uint32_t vectorRandomSeed(uint32_t seed, uint32_t* state);
extern uint32_t vectorDefaultRandomState;
uint32_t vectorDefaultRandomBounded(uint32_t bound);
float vectorDefaultRandomFloat(float maximum);
float vectorWrapFloat(float value, float period);
int32_t vectorWrapInteger(int32_t value, int32_t period);
float vectorCosineAngle(uint16_t angle);
void vectorSineCosineAngle(float* sine, float* cosine, uint16_t angle);
uint16_t vectorArcSineAngle(float value);
uint16_t vectorAtan2Angle(float x, float y);
int32_t vectorFixedSineAngle(uint16_t angle);
void vectorFixedSineCosineAngle(int32_t* sine, int32_t* cosine, uint16_t angle);
uint16_t vectorFixedAtanAngle(int32_t value);
uint16_t vectorFixedAtan2Angle(int32_t x, int32_t y);
typedef struct VectorPolar2 { float radius; uint16_t angle; } VectorPolar2;
VectorPolar2* vectorPolarFromCartesian2(VectorPolar2* output, const float input[2]);

// One turn occupies all 16 bits. The game's polynomial and angular
// quantization are preserved instead of substituting a host sine function.
float rotationSineAngle(uint16_t angle);
float rotationSineRadians(float radians);
void rotationMatrixIdentity(float matrix[16]);

/* Keep multiplication and accumulation as separate float operations. */

static inline int32_t vectorMathSignedWord(uint32_t value)
{
    return value <= INT32_MAX ? (int32_t)value : -1 - (int32_t)(UINT32_MAX - value);
}

static inline int32_t vectorMathAddWord(int32_t a, int32_t b)
{ return vectorMathSignedWord((uint32_t)a + (uint32_t)b); }

static inline int32_t vectorMathSubtractWord(int32_t a, int32_t b)
{ return vectorMathSignedWord((uint32_t)a - (uint32_t)b); }

static inline int32_t vectorMathShiftRight(int32_t value, unsigned amount)
{
    amount &= 31;
    if (!amount) return value;
    return value >= 0 ? value >> amount : -1 - (int32_t)((uint32_t)(-1 - value) >> amount);
}

static inline int32_t vectorMathFixedProduct(int32_t a, int32_t b)
{
    const int64_t product = (int64_t)a * b;
    const int64_t shifted = product >= 0 ? product / 65536 : -1 - ((-1 - product) / 65536);
    return vectorMathSignedWord((uint32_t)shifted);
}

static inline uint32_t vectorMathAbsoluteWord(int32_t value)
{ return value >= 0 ? (uint32_t)value : 0u - (uint32_t)value; }

static inline float vectorMathLower(float a, float b)
{
    if (a == 0.0f && b == 0.0f)
        return copysignf(0.0f, signbit(a) || signbit(b) ? -1.0f : 1.0f);
    return b < a ? b : a;
}

static inline float vectorMathUpper(float a, float b)
{
    if (a == 0.0f && b == 0.0f)
        return copysignf(0.0f, signbit(a) && signbit(b) ? -1.0f : 1.0f);
    return a < b ? b : a;
}

static inline int32_t vectorMathFixedLength(const int32_t* input, int dimensions)
{
    uint32_t magnitude = 0;
    for (int i = 0; i < dimensions; ++i) magnitude += vectorMathAbsoluteWord(input[i]);
    unsigned shift = 0;
    while (vectorMathShiftRight(vectorMathSignedWord(magnitude), shift) > 65535) shift += 2;
    uint32_t squared = 0;
    for (int i = 0; i < dimensions; ++i) {
        const int32_t value = vectorMathShiftRight(input[i], shift);
        squared += (uint32_t)((int64_t)value * value);
    }
    return vectorMathSignedWord(vectorIntegerSquareRoot(squared) << (shift & 31));
}

static inline int32_t vectorMathFixedDistance(const int32_t* first, const int32_t* second, int dimensions)
{
    int32_t difference[3];
    uint32_t magnitude = 0;
    for (int i = 0; i < dimensions; ++i) {
        difference[i] = vectorMathSubtractWord(first[i], second[i]);
        magnitude += vectorMathAbsoluteWord(difference[i]);
    }
    const unsigned shift = vectorMathSignedWord(magnitude) > 0x00ffffff ? 16u : 8u;
    uint32_t squared = 0;
    for (int i = 0; i < dimensions; ++i) {
        const int32_t value = vectorMathShiftRight(difference[i], shift);
        squared += (uint32_t)((int64_t)value * value);
    }
    return vectorMathSignedWord(vectorIntegerSquareRoot(squared) << shift);
}

static inline void vectorMathFixedNormalize(int32_t* output, const int32_t* input, int dimensions)
{
    uint32_t magnitude = 0;
    for (int i = 0; i < dimensions; ++i) magnitude += vectorMathAbsoluteWord(input[i]);
    unsigned shift = 0;
    while (vectorMathShiftRight(vectorMathSignedWord(magnitude), shift) > 65535) shift += 2;
    int32_t reduced[3]; uint32_t squared = 0;
    for (int i = 0; i < dimensions; ++i) {
        reduced[i] = vectorMathShiftRight(input[i], shift);
        squared += (uint32_t)((int64_t)reduced[i] * reduced[i]);
    }
    const int32_t length = (int32_t)vectorIntegerSquareRoot(squared);
    if (!length) {
        output[0] = 65536;
        for (int i = 1; i < dimensions; ++i) output[i] = 0;
    } else {
        const int32_t scale = vectorFixedReciprocal(length);
        for (int i = 0; i < dimensions; ++i) output[i] = vectorMathFixedProduct(reduced[i], scale);
    }
}

static inline void vectorMathQuaternionFromMatrix(float output[4], const float matrix[16])
{
    const float trace = (matrix[0] + matrix[5]) + matrix[10];
    if (trace > 0.0f) {
        const float root = sqrtf(trace + 1.0f), scale = finiteRatio(0.5f, root);
        output[3] = root * 0.5f;
        output[0] = (matrix[6] - matrix[9]) * scale;
        output[1] = (matrix[8] - matrix[2]) * scale;
        output[2] = (matrix[1] - matrix[4]) * scale;
    } else {
        int first = matrix[0] < matrix[5] ? 1 : 0;
        if (matrix[first * 5] < matrix[10]) first = 2;
        const int second = (first + 1) % 3, third = (second + 1) % 3;
        const float otherSum = matrix[second * 5] + matrix[third * 5];
        const float difference = matrix[first * 5] - otherSum;
        const float root = sqrtf(difference + 1.0f);
        const float scale = root == 0.0f ? root : finiteRatio(0.5f, root);
        float result[4];
        result[first] = root * 0.5f;
        result[3] = (matrix[second * 4 + third] - matrix[third * 4 + second]) * scale;
        result[second] = (matrix[first * 4 + second] + matrix[second * 4 + first]) * scale;
        result[third] = (matrix[first * 4 + third] + matrix[third * 4 + first]) * scale;
        memcpy(output, result, sizeof(result));
    }
}

#ifdef __cplusplus
}
#endif

#endif
