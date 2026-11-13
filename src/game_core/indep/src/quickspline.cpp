static const float catmullRomBasis[48] = {
    0, 1, 0, 0,  -0.5f, 0, 0.5f, 0,  1, -2.5f, 2, -0.5f,  -0.5f, 1.5f, -1.5f, 0.5f,
    -0.5f, 0, 0.5f, 0,  2, -5, 4, -1,  -1.5f, 4.5f, -4.5f, 1.5f,  0, 0, 0, 0,
    2, -5, 4, -1,  -3, 9, -9, 3,  0, 0, 0, 0,  0, 0, 0, 0
};
#include "indep/inc/QuickSpline.hpp"
#include "bware/inc/bMath.hpp"
#include <float.h>
#include <math.h>
static inline float geometryQueryDot3(const float first[4], const float second[4])
{
    return bScalarAdd(bScalarAdd(bScalarMultiply(first[0],second[0]),
                                bScalarMultiply(first[1],second[1])),
                      bScalarMultiply(first[2],second[2]));
}

static inline float geometryQueryPointDistance(const float first[4], const float second[4])
{
    const float difference[4] = {bScalarAdd(first[0],-second[0]),bScalarAdd(first[1],-second[1]),
                                  bScalarAdd(first[2],-second[2]),0.0f};
    return bScalarSquareRoot(geometryQueryDot3(difference,difference));
}



#include <math.h>
#include <stdlib.h>
#include <string.h>
static inline void* splineAllocateStorage(size_t count, size_t size)
{
    void* storage = calloc(count == 0 ? 1 : count, size);
    if (storage == NULL) abort();
    return storage;
}



static inline float splinePointDistance(const float first[4], const float second[4])
{
    const float x = first[0] - second[0];
    const float y = first[1] - second[1];
    const float z = first[2] - second[2];
    return sqrtf((x * x + y * y) + z * z);
}


#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

int32_t quickSplineFindLocalClosestPoint(const QuickSpline* spline,
                                        const float point[4], int32_t initialIndex)
{
    int32_t closest = initialIndex;
    float distance = geometryQueryPointDistance(point, spline->points[initialIndex]);
    for (int32_t i = initialIndex + 1; i < spline->count; ++i) {
        const float candidate = geometryQueryPointDistance(point, spline->points[i]);
        if (distance <= candidate) break;
        closest = i;
        distance = candidate;
    }
    for (int32_t i = initialIndex - 1; i >= 0; --i) {
        const float candidate = geometryQueryPointDistance(point, spline->points[i]);
        if (distance <= candidate) break;
        closest = i;
        distance = candidate;
    }
    return closest;
}

int32_t quickSplineFindClosestPoint(const QuickSpline* spline, const float point[4])
{
    int32_t closest = -1;
    float distance = 0.0f;
    for (int32_t i = 0; i < spline->count; ++i) {
        const float candidate = geometryQueryPointDistance(point, spline->points[i]);
        if (closest < 0 || candidate < distance) {
            distance = candidate;
            closest = i;
        }
    }
    return closest;
}

void quickSplinePreparePoints(QuickSpline* spline)
{
    if (spline->count == 0) return;
    float (*points)[4] = spline->points;
    const int32_t count = spline->count;
    if (spline->boundary == QUICK_SPLINE_CLOSED) {
        memcpy(points[-1], points[count - 1], sizeof(points[0]));
        memcpy(points[count], points[0], sizeof(points[0]));
        memcpy(points[count + 1], points[1], sizeof(points[0]));
        if (spline->period != 0.0f) {
            points[-1][3] = bScalarAdd(points[count - 1][3], -(spline->period));
            points[count][3] = bScalarAdd(spline->period, points[0][3]);
            points[count + 1][3] = bScalarAdd(spline->period, points[1][3]);
        }
    } else if (spline->boundary == QUICK_SPLINE_CLAMP) {
        memcpy(points[-1], points[0], sizeof(points[0]));
        memcpy(points[count], points[count - 1], sizeof(points[0]));
        if (spline->period != 0.0f) {
            points[-1][3] = -points[1][3];
            points[count][3] = bScalarAdd((bScalarAdd(spline->period, spline->period)), -(points[count - 1][3]));
        }
    } else if (spline->boundary == QUICK_SPLINE_EXTRAPOLATE) {
        for (int lane = 0; lane < 4; ++lane) {
            const float beginning = bScalarAdd(points[1][lane], -(points[0][lane]));
            points[-1][lane] = bScalarAdd(points[0][lane], bScalarMultiply(beginning, -1.0f));
            const float ending = bScalarAdd(points[count - 1][lane], -(points[count - 2][lane]));
            points[count][lane] = bScalarAdd(points[count - 1][lane], ending);
        }
        if (spline->period != 0.0f) {
            points[-1][3] = -points[1][3];
            points[count][3] = bScalarAdd((bScalarAdd(spline->period, spline->period)), -(points[count - 1][3]));
        }
    }
}

void quickSplineGetControlPoints(const QuickSpline* spline, float output[16], int32_t first)
{
    const int32_t count = spline->count;
    for (int32_t i = 0; i < 4; ++i) {
        const int32_t index = first + i;
        float* point = output + i * 4;
        if (index < -1) {
            const float beyond = (float)(-index);
            for (int lane = 0; lane < 4; ++lane) {
                const float change = bScalarAdd(spline->points[0][lane], -(spline->points[1][lane]));
                point[lane] = bScalarAdd(spline->points[0][lane], bScalarMultiply(change, beyond));
            }
        } else if (count < index) {
            const float beyond = (float)(index - count + 1);
            for (int lane = 0; lane < 4; ++lane) {
                const float change = bScalarAdd(spline->points[count - 1][lane], -(spline->points[count - 2][lane]));
                point[lane] = bScalarAdd(spline->points[count - 1][lane], bScalarMultiply(change, beyond));
            }
        } else {
            memcpy(point, spline->points[index], 4 * sizeof(float));
        }
    }
}


float* quickSplineBlendPoints(const QuickSpline* spline, float output[4],
                              float fraction, int32_t derivativeOrder,
                              const float controlPoints[16])
{
    const float square = bScalarMultiply(fraction, fraction);
    const float powers[4] = {1.0f, fraction, square, bScalarMultiply(fraction, square)};
    float weights[4];
    vectorMatrixTransform(weights, spline->basis + derivativeOrder * 16, powers);
    vectorMatrixTransform(output, controlPoints, weights);
    return output;
}

QuickSpline* quickSplineInitialize(QuickSpline* spline, QuickSplineBoundary boundary,
                                   int32_t basisType, uint16_t capacity)
{
    spline->capacity = capacity;
    spline->ownsStorage = 1;
    spline->storage = static_cast<float (*)[4]>(splineAllocateStorage((size_t)capacity + 3, sizeof(*spline->storage)));
    quickSplineConfigure(spline, boundary, basisType);
    return spline;
}

QuickSpline* quickSplineInitializeBorrowed(QuickSpline* spline, QuickSplineBoundary boundary,
                                           int32_t basisType, float (*storage)[4],
                                           int32_t storageCount)
{
    spline->ownsStorage = 0;
    spline->capacity = (uint16_t)(storageCount - 3);
    spline->storage = storage;
    quickSplineConfigure(spline, boundary, basisType);
    return spline;
}

void quickSplineDestroy(QuickSpline* spline, int releaseObject)
{
    if (spline->ownsStorage) free(spline->storage);
    if (releaseObject & 1) free(spline);
}

void quickSplineConfigure(QuickSpline* spline, QuickSplineBoundary boundary, int32_t basisType)
{
    spline->boundary = boundary;
    spline->basisType = basisType;
    spline->basis = basisType == 0 ? catmullRomBasis : NULL;
    spline->paddingCount = boundary == QUICK_SPLINE_CLOSED ? 3 : 2;
    spline->points = spline->storage + 1;
    spline->period = 0.0f;
    quickSplineSetPointCount(spline, 0);
}

void quickSplineSnapshot(QuickSpline* spline, CurveSnapshot* snapshot, int32_t components)
{
    snapshot->begin(snapshot->context, "QuickSpline");
    snapshot->moveBytes(snapshot->context, &spline->count, sizeof(spline->count));
    if (snapshot->restoring) quickSplineSetPointCount(spline, spline->count);
    for (int32_t i = 0; i < spline->count; ++i)
        snapshot->moveBytes(snapshot->context, spline->points[i], (size_t)components * sizeof(float));
    snapshot->end(snapshot->context);
}

void quickSplineSetPointCount(QuickSpline* spline, uint16_t count)
{
    spline->count = count;
    spline->pointsDirty = 1;
    if (count == 0) {
        spline->minimumParameter = 0.0f;
        spline->maximumParameter = 0.0f;
    } else if (spline->boundary == QUICK_SPLINE_CLOSED) {
        spline->minimumParameter = 0.0f;
        spline->maximumParameter = (float)count;
    } else if (spline->boundary == QUICK_SPLINE_CLAMP) {
        spline->minimumParameter = 0.0f;
        spline->maximumParameter = (float)(count - 1);
    } else if (spline->boundary == QUICK_SPLINE_EXTRAPOLATE) {
        spline->minimumParameter = -9999999.0f;
        spline->maximumParameter = 9999999.0f;
    }
}

float quickSplineBoundParameter(const QuickSpline* spline, float parameter)
{
    if (spline->boundary == QUICK_SPLINE_CLOSED)
        return vectorWrapFloat(parameter, spline->maximumParameter);
    if (spline->boundary == QUICK_SPLINE_CLAMP) {
        const float last = bScalarAdd(spline->maximumParameter, -(0.0000001f));
        if (parameter < spline->minimumParameter) parameter = spline->minimumParameter;
        if (last < parameter) parameter = last;
    }
    return parameter;
}

float* quickSplineEvaluate(QuickSpline* spline, float output[4],
                           float parameter, int32_t derivativeOrder)
{
    if (spline->pointsDirty) {
        quickSplinePreparePoints(spline);
        spline->pointsDirty = 0;
    }
    if (spline->count == 0) {
        memset(output, 0, 4 * sizeof(float));
        return output;
    }
    parameter = quickSplineBoundParameter(spline, parameter);
    const float base = floorf(parameter);
    const float fraction = bScalarAdd(parameter, -(base));
    const int32_t index = (int32_t)base;
    if (spline->boundary == QUICK_SPLINE_EXTRAPOLATE &&
        (index < 0 || index >= (int32_t)spline->count - 1)) {
        float controlPoints[16];
        quickSplineGetControlPoints(spline, controlPoints, index - 1);
        return quickSplineBlendPoints(spline, output, fraction, derivativeOrder, controlPoints);
    }
    return quickSplineBlendPoints(spline, output, fraction, derivativeOrder,
                                  &spline->points[index - 1][0]);
}

void quickSplineCalculateLength(QuickSpline* spline, int32_t subdivisions)
{
    float total = 0.0f;
    for (int32_t i = 0; i < spline->count; ++i) {
        spline->points[i][3] = total;
        spline->pointsDirty = 1;
        const float base = (float)i;
        if (base < spline->maximumParameter) {
            float previous[4], current[4];
            quickSplineEvaluate(spline, previous, base, 0);
            for (int32_t part = 1; part <= subdivisions; ++part) {
                const float fraction = (float)part / (float)subdivisions;
                quickSplineEvaluate(spline, current, base + fraction, 0);
                total += splinePointDistance(current, previous);
                memcpy(previous, current, sizeof(previous));
            }
        }
    }
    spline->period = total;
    spline->pointsDirty = 1;
}

float quickSplineFindClosestParameter(QuickSpline* spline, const float point[4],
                                      float initialParameter, float tolerance)
{
    if (spline->pointsDirty) {
        quickSplinePreparePoints(spline);
        spline->pointsDirty = 0;
    }
    if (spline->count == 0) return 0.0f;
    float parameter = initialParameter;
    float maximumStep = 1.0f;
    float previousProjection = 0.0f;
    float previousStep = 0.0f;
    for (int32_t iteration = 0; iteration < 998; ++iteration) {
        float position[4], tangent[4], difference[4];
        quickSplineEvaluate(spline, position, parameter, 0);
        quickSplineEvaluate(spline, tangent, parameter, 1);
        const float tangentLength = bScalarSquareRoot(geometryQueryDot3(tangent,tangent));
        if (tangentLength == 0.0f) return parameter;
        for (int lane = 0; lane < 4; ++lane) difference[lane] = bScalarAdd(point[lane],-position[lane]);
        const float projection = bScalarDivide(geometryQueryDot3(difference,tangent),tangentLength);
        if (fabsf(projection) < tolerance) return parameter;
        float divisor = tangentLength;
        if (previousStep != 0.0f) {
            divisor = bScalarDivide(bScalarAdd(previousProjection,-projection),previousStep);
            if (divisor == 0.0f) return parameter;
        }
        float step = bScalarDivide(projection,divisor);
        if (step < -maximumStep) step = -maximumStep;
        if (maximumStep < step) step = maximumStep;
        parameter = quickSplineBoundParameter(spline,bScalarAdd(parameter,step));
        if (bScalarMultiply(step,previousStep) < 0.0f) maximumStep = bScalarMultiply(maximumStep,0.5f);
        previousStep = step;
        previousProjection = projection;
    }
    return parameter;
}
