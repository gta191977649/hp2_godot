#include "bware/inc/bMath.hpp"
#include "indep/inc/Track.hpp"


#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

void terrainBoundsExpandRadius3(float minimum[4], float maximum[4],
                                const float point[4], float radius)
{
    for (size_t i = 0; i < 3; ++i) {
        float low = point[i] - radius, high = point[i] + radius;
        if (low < minimum[i]) minimum[i] = low;
        if (maximum[i] < high) maximum[i] = high;
    }
    if (point[3] < minimum[3]) minimum[3] = point[3];
    if (maximum[3] < point[3]) maximum[3] = point[3];
}

float* vectorNormalize2(float output[2], const float input[2])
{
    const float x = input[0], y = input[1];
    const float xx = x * x, yy = y * y;
    const float length = sqrtf(xx + yy);
    if (length == 0.0f) { output[0] = 1.0f; output[1] = 0.0f; }
    else { output[1] = finiteRatio(y, length); output[0] = finiteRatio(x, length); }
    return output;
}
float* vectorSetLength2(float output[2], const float input[2], float length)
{
    const float x = input[0], y = input[1];
    const float xx = x * x, yy = y * y;
    const float divisor = finiteRatio(sqrtf(xx + yy), length);
    if (divisor == 0.0f) { output[0] = 1.0f; output[1] = 0.0f; }
    else { output[1] = finiteRatio(y, divisor); output[0] = finiteRatio(x, divisor); }
    return output;
}
float* vectorSetLength3(float output[3], const float input[4], float length)
{
    const float x = input[0], y = input[1], z = input[2];
    const float xx = bScalarMultiply(x, x), yy = bScalarMultiply(y, y), zz = bScalarMultiply(z, z);
    const float sum = bScalarAdd(bScalarAdd(xx, yy), zz);
    const float divisor = bScalarFiniteRatio(bScalarSquareRoot(sum), length);
    if (divisor == 0.0f) { output[0] = length; output[1] = 0.0f; output[2] = 0.0f; }
    else {
        const float scale = bScalarFiniteRatio(1.0f, divisor);
        output[2] = bScalarMultiply(z, scale); output[0] = bScalarMultiply(x, scale); output[1] = bScalarMultiply(y, scale);
    }
    return output;
}
float* vectorMultiplyAdd2(float output[2], const float first[2], const float second[2], float scale)
{
    const float y = bScalarMultiply(second[1], scale), x = bScalarMultiply(second[0], scale);
    const float outY = bScalarAdd(first[1], y), outX = bScalarAdd(first[0], x);
    output[1] = outY; output[0] = outX;
    return output;
}
int vectorNear2(const float first[2], const float second[2], float tolerance)
{
    if (tolerance < fabsf(first[0] - second[0])) return 0;
    return !(tolerance < fabsf(first[1] - second[1]));
}
int vectorNear3(const float first[3], const float second[3], float tolerance)
{
    if (tolerance < fabsf(first[0] - second[0])) return 0;
    if (tolerance < fabsf(first[1] - second[1])) return 0;
    return !(tolerance < fabsf(first[2] - second[2]));
}
int32_t vectorFixedLength2(const int32_t value[2]) { return vectorMathFixedLength(value, 2); }
int32_t vectorFixedLength3(const int32_t value[3]) { return vectorMathFixedLength(value, 3); }
int32_t vectorFixedDistance2(const int32_t a[2], const int32_t b[2]) { return vectorMathFixedDistance(a, b, 2); }
int32_t vectorFixedDistance3(const int32_t a[3], const int32_t b[3]) { return vectorMathFixedDistance(a, b, 3); }
int32_t* vectorFixedScale2(int32_t output[2], const int32_t input[2], int32_t scale)
{
    const int32_t x = vectorMathFixedProduct(input[0], scale), y = vectorMathFixedProduct(input[1], scale);
    output[1] = y; output[0] = x; return output;
}
int32_t* vectorFixedScale3(int32_t output[3], const int32_t input[3], int32_t scale)
{
    const int32_t x = vectorMathFixedProduct(input[0], scale), y = vectorMathFixedProduct(input[1], scale), z = vectorMathFixedProduct(input[2], scale);
    output[2] = z; output[0] = x; output[1] = y; return output;
}
int32_t* vectorFixedMultiplyAdd2(int32_t output[2], const int32_t a[2], const int32_t b[2], int32_t scale)
{
    const int32_t x = vectorMathAddWord(a[0], vectorMathFixedProduct(b[0], scale));
    const int32_t y = vectorMathAddWord(a[1], vectorMathFixedProduct(b[1], scale));
    output[1] = y; output[0] = x; return output;
}
int32_t* vectorFixedMultiplyAdd3(int32_t output[3], const int32_t a[3], const int32_t b[3], int32_t scale)
{
    const int32_t x = vectorMathAddWord(a[0], vectorMathFixedProduct(b[0], scale));
    const int32_t y = vectorMathAddWord(a[1], vectorMathFixedProduct(b[1], scale));
    const int32_t z = vectorMathAddWord(a[2], vectorMathFixedProduct(b[2], scale));
    output[0] = x; output[1] = y; output[2] = z; return output;
}
int32_t vectorFixedDot2(const int32_t a[2], const int32_t b[2])
{ return vectorMathAddWord(vectorMathFixedProduct(a[0], b[0]), vectorMathFixedProduct(a[1], b[1])); }
int32_t vectorFixedDot3(const int32_t a[3], const int32_t b[3])
{ return vectorMathAddWord(vectorMathAddWord(vectorMathFixedProduct(a[0], b[0]), vectorMathFixedProduct(a[1], b[1])), vectorMathFixedProduct(a[2], b[2])); }
int32_t* vectorFixedCross3(int32_t output[3], const int32_t a[3], const int32_t b[3])
{
    const int32_t x = vectorMathSubtractWord(vectorMathFixedProduct(a[1], b[2]), vectorMathFixedProduct(a[2], b[1]));
    const int32_t y = vectorMathSubtractWord(vectorMathFixedProduct(a[2], b[0]), vectorMathFixedProduct(a[0], b[2]));
    const int32_t z = vectorMathSubtractWord(vectorMathFixedProduct(a[0], b[1]), vectorMathFixedProduct(a[1], b[0]));
    output[1] = y; output[2] = z; output[0] = x; return output;
}
void vectorBoundsFromPoint2(float minimum[2], float maximum[2], const float point[2])
{ const float x = point[0], y = point[1]; minimum[1] = y; minimum[0] = x; maximum[1] = y; maximum[0] = x; }
void vectorBoundsExpandPoint2(float minimum[2], float maximum[2], const float point[2])
{
    const float x = point[0], y = point[1];
    const float lowX = vectorMathLower(minimum[0], x), lowY = vectorMathLower(minimum[1], y);
    const float highX = vectorMathUpper(maximum[0], x), highY = vectorMathUpper(maximum[1], y);
    minimum[1] = lowY; minimum[0] = lowX; maximum[1] = highY; maximum[0] = highX;
}
void vectorBoundsExpandRadius2(float minimum[2], float maximum[2], const float point[2], float radius)
{
    const float lowX = bScalarAdd(point[0], -radius), lowY = bScalarAdd(point[1], -radius);
    const float highX = bScalarAdd(point[0], radius), highY = bScalarAdd(point[1], radius);
    minimum[0] = vectorMathLower(minimum[0], lowX); minimum[1] = vectorMathLower(minimum[1], lowY);
    maximum[0] = vectorMathUpper(maximum[0], highX); maximum[1] = vectorMathUpper(maximum[1], highY);
}
void vectorBoundsExpandBounds2(float minimum[2], float maximum[2], const float otherMinimum[2], const float otherMaximum[2])
{
    const float lowX = vectorMathLower(minimum[0], otherMinimum[0]), lowY = vectorMathLower(minimum[1], otherMinimum[1]);
    const float highX = vectorMathUpper(maximum[0], otherMaximum[0]), highY = vectorMathUpper(maximum[1], otherMaximum[1]);
    minimum[1] = lowY; minimum[0] = lowX; maximum[1] = highY; maximum[0] = highX;
}
int vectorBoundsContainsPoint2(const float minimum[2], const float maximum[2], const float point[2], float tolerance)
{
    for (int i = 0; i < 2; ++i) if (point[i] < minimum[i] - tolerance || maximum[i] + tolerance < point[i]) return 0;
    return 1;
}
int vectorBoundsContainsBounds2(const float minimum[2], const float maximum[2], const float low[2], const float high[2])
{ for (int i = 0; i < 2; ++i) if (!(minimum[i] <= low[i]) || !(high[i] <= maximum[i])) return 0; return 1; }
int vectorBoundsOverlaps2(const float minimum[2], const float maximum[2], const float low[2], const float high[2])
{ for (int i = 0; i < 2; ++i) if (!(low[i] < maximum[i]) || !(minimum[i] < high[i])) return 0; return 1; }
void vectorBoundsFromPoint3(float minimum[4], float maximum[4], const float point[4])
{ float copy[4]; memcpy(copy, point, sizeof(copy)); memcpy(minimum, copy, sizeof(copy)); memcpy(maximum, copy, sizeof(copy)); }
void vectorBoundsExpandPoint3(float minimum[4], float maximum[4], const float point[4])
{
    float low[4], high[4];
    for (int i = 0; i < 4; ++i) { low[i] = vectorMathLower(minimum[i], point[i]); high[i] = vectorMathUpper(maximum[i], point[i]); }
    memcpy(minimum, low, sizeof(low)); memcpy(maximum, high, sizeof(high));
}
void vectorBoundsExpandBounds3(float minimum[4], float maximum[4], const float otherMinimum[4], const float otherMaximum[4])
{
    float low[4], high[4];
    for (int i = 0; i < 4; ++i) { low[i] = vectorMathLower(minimum[i], otherMinimum[i]); high[i] = vectorMathUpper(maximum[i], otherMaximum[i]); }
    memcpy(minimum, low, sizeof(low)); memcpy(maximum, high, sizeof(high));
}
int vectorBoundsContainsPoint3(const float minimum[3], const float maximum[3], const float point[3], float tolerance)
{ for (int i = 0; i < 3; ++i) if (point[i] < minimum[i] - tolerance || maximum[i] + tolerance < point[i]) return 0; return 1; }
int vectorBoundsContainsBounds3(const float minimum[3], const float maximum[3], const float low[3], const float high[3])
{ for (int i = 0; i < 3; ++i) if (!(minimum[i] <= low[i]) || !(high[i] <= maximum[i])) return 0; return 1; }
int vectorBoundsOverlaps3(const float minimum[3], const float maximum[3], const float low[3], const float high[3])
{ for (int i = 0; i < 3; ++i) if (!(low[i] < maximum[i]) || !(minimum[i] < high[i])) return 0; return 1; }

int32_t* vectorFixedNormalize2(int32_t output[2], const int32_t input[2])
{ vectorMathFixedNormalize(output, input, 2); return output; }

int32_t* vectorFixedNormalize3(int32_t output[3], const int32_t input[3])
{ vectorMathFixedNormalize(output, input, 3); return output; }

void vectorBoundsInitialize2(float minimum[2], float maximum[2])
{ minimum[0] = minimum[1] = FLT_MAX; maximum[0] = maximum[1] = -FLT_MAX; }

void vectorBoundsInitialize3(float minimum[4], float maximum[4])
{ for (int i = 0; i < 3; ++i) { minimum[i] = FLT_MAX; maximum[i] = -FLT_MAX; } minimum[3] = 0.0f; maximum[3] = 1.0f; }
