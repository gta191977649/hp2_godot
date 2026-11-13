#include "indep/inc/Spline.hpp"
#include <float.h>
#include <limits.h>
#include <cmath>
#include <stdexcept>
#include <stdint.h>
#include "bware/inc/bMath.hpp"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static inline void* splineAllocateStorage(size_t count, size_t size)
{
    void* storage = calloc(count == 0 ? 1 : count, size);
    if (storage == NULL) abort();
    return storage;
}

static inline float splineMaximumParameter(const Spline* spline)
{
    if (spline->boundary == SPLINE_CLOSED) return (float)spline->count;
    if (spline->boundary == SPLINE_CLAMP || spline->boundary == SPLINE_EXTRAPOLATE)
        return (float)(spline->count - 1);
    return 0.0f;
}

static inline float splinePointDistance(const float first[4], const float second[4])
{
    const float x = first[0] - second[0];
    const float y = first[1] - second[1];
    const float z = first[2] - second[2];
    return sqrtf((x * x + y * y) + z * z);
}


#include <math.h>
#include <stdlib.h>
#include <string.h>

Spline* splineInitializeBorrowed(Spline* spline, SplineBoundary boundary,
                                 int32_t count, float (*points)[4],
                                 int32_t lookupCapacity)
{
    spline->capacity = count;
    spline->count = count;
    spline->boundary = boundary;
    spline->points = points;
    spline->ownedPoints = NULL;
    spline->lookupCapacity = lookupCapacity;
    spline->length = 0.0f;
    spline->lengthDirty = 1;
    spline->parameterAtDistance = NULL;
    return spline;
}

void splineSetPoint(Spline* spline, int32_t index, const float point[4])
{
    // This source operation does not invalidate the length cache.
    memcpy(spline->points[index], point, sizeof(spline->points[index]));
}

void splineRemoveFirstPoint(Spline* spline)
{
    // There is no first point to remove from an empty spline. The previous
    // decrement underflowed count and corrupted subsequent camera updates.
    if (!spline || spline->count <= 0) return;
    spline->lengthDirty = 1;
    --spline->count;
    for (int32_t i = 0; i < spline->count; ++i)
        memcpy(spline->points[i], spline->points[i + 1], sizeof(spline->points[i]));
}

int32_t splineBoundarySupportCount(const Spline* spline)
{
    if (spline->boundary == SPLINE_CLOSED) return 3;
    if (spline->boundary == SPLINE_CLAMP || spline->boundary == SPLINE_EXTRAPOLATE) return 2;
    return 0;
}

float splineControlPolygonLength(Spline* spline)
{
    if (spline->lengthDirty) {
        spline->length = 0.0f;
        for (int32_t i = 1; i < spline->count; ++i)
            spline->length += splinePointDistance(spline->points[i - 1], spline->points[i]);
        spline->lengthDirty = 0;
    }
    return spline->length;
}

static inline float geometryQueryDot3(const float first[4], const float second[4])
{
    return (first[0] * second[0] + first[1] * second[1]) + first[2] * second[2];
}

static inline float geometryQueryPointDistance(const float first[4], const float second[4])
{
    const float difference[4] = {first[0] - second[0], first[1] - second[1],
                                  first[2] - second[2], 0.0f};
    return sqrtf(geometryQueryDot3(difference, difference));
}

static inline float geometryQueryMaximumParameter(const Spline* spline)
{
    if (spline->boundary == SPLINE_CLOSED) return (float)spline->count;
    if (spline->boundary == SPLINE_CLAMP || spline->boundary == SPLINE_EXTRAPOLATE)
        return (float)(spline->count - 1);
    return 0.0f;
}

static const float distanceUnits[7] = {16.0f, 64.0f, 256.0f, 1024.0f,
                                     4096.0f, 16384.0f, 65536.0f};
static const float distanceSteps[7] = {0.000244140625f, 0.0009765625f, 0.00390625f,
                                     0.015625f, 0.0625f, 0.25f, 1.0f};
static const float distanceScales[7] = {0.0625f, 0.015625f, 0.00390625f,
                                      0.0009765625f, 0.000244140625f,
                                      0.00006103515625f, 0.0000152587890625f};

Spline* splineInitialize(Spline* spline, SplineBoundary boundary,
                         int32_t capacity, int32_t lookupCapacity)
{
    spline->capacity = capacity;
    spline->count = 0;
    spline->boundary = boundary;
    spline->lookupCapacity = lookupCapacity;
    spline->length = 0.0f;
    spline->lengthDirty = 1;
    spline->parameterAtDistance = NULL;
    spline->ownedPoints = static_cast<float(*)[4]>(splineAllocateStorage((size_t)capacity, sizeof(*spline->points)));
    spline->points = spline->ownedPoints;
    return spline;
}

void splineDestroy(Spline* spline, int releaseObject)
{
    free(spline->ownedPoints);
    free(spline->parameterAtDistance);
    if (releaseObject & 1) free(spline);
}

void splineSnapshot(Spline* spline, CurveSnapshot* snapshot)
{
    snapshot->begin(snapshot->context, "Spline");
    snapshot->moveBytes(snapshot->context, &spline->capacity, sizeof(spline->capacity));
    snapshot->moveBytes(snapshot->context, &spline->count, sizeof(spline->count));
    snapshot->moveBytes(snapshot->context, &spline->boundary, sizeof(spline->boundary));
    for (int32_t i = 0; i < spline->count; ++i)
        snapshot->moveBytes(snapshot->context, spline->points[i], 3 * sizeof(float));
    snapshot->moveBytes(snapshot->context, &spline->lengthDirty, sizeof(spline->lengthDirty));
    snapshot->moveBytes(snapshot->context, &spline->lookupCapacity, sizeof(spline->lookupCapacity));
    snapshot->moveBytes(snapshot->context, &spline->lookupStepIndex, sizeof(spline->lookupStepIndex));
    snapshot->moveBytes(snapshot->context, &spline->length, sizeof(spline->length));
    snapshot->moveBytes(snapshot->context, &spline->snapshotValue, sizeof(spline->snapshotValue));
    snapshot->moveBytes(snapshot->context, &spline->lookupCount, sizeof(spline->lookupCount));
    if (spline->parameterAtDistance != NULL)
        snapshot->moveBytes(snapshot->context, spline->parameterAtDistance,
                            (size_t)spline->lookupCount * sizeof(float));
    snapshot->end(snapshot->context);
}

void splineAppendPoint(Spline* spline, const float point[4])
{
    spline->lengthDirty = 1;
    if (spline->count == spline->capacity) {
        // A native zero-capacity spline still needs one real point slot.
        // Multiplying zero by two left capacity at zero and corrupted the
        // next append; normal positive capacities keep the original doubling.
        if (spline->capacity > INT32_MAX / 2) abort();
        spline->capacity = spline->capacity ? spline->capacity * 2 : 1;
        float (*replacement)[4] = static_cast<float(*)[4]>(splineAllocateStorage((size_t)spline->capacity, sizeof(*replacement)));
        if (spline->count > 0)
            memcpy(replacement, spline->points, (size_t)spline->count * sizeof(*replacement));
        free(spline->ownedPoints);
        spline->ownedPoints = replacement;
        spline->points = replacement;
    }
    const int32_t index = spline->count++;
    memcpy(spline->points[index], point, sizeof(spline->points[index]));
}

const float* splinePointAt(const Spline* spline, float parameter, float temporary[4])
{
    const float last = splineMaximumParameter(spline);
    if (spline->boundary == SPLINE_CLOSED) {
        parameter = vectorWrapFloat(parameter, last);
    } else if (spline->boundary == SPLINE_CLAMP) {
        if (parameter < 0.0f) parameter = 0.0f;
        else if (last < parameter) parameter = last;
    } else if (spline->boundary == SPLINE_EXTRAPOLATE) {
        if (parameter < 0.0f) {
            for (int lane = 0; lane < 4; ++lane) {
                const float change = spline->points[1][lane] - spline->points[0][lane];
                temporary[lane] = spline->points[0][lane] + change * parameter;
            }
            return temporary;
        }
        if (last < parameter) {
            const int32_t index = spline->count - 1;
            const float beyond = parameter - last;
            for (int lane = 0; lane < 4; ++lane) {
                const float change = spline->points[index][lane] - spline->points[index - 1][lane];
                temporary[lane] = spline->points[index][lane] + change * beyond;
            }
            return temporary;
        }
    }
    return spline->points[(int32_t)parameter];
}

float* splineBlendPoints(const Spline* spline, float output[4], float parameter,
                         float previous, float current, float next, float following)
{
    float temporary[4], blended[4];
    const float* point = splinePointAt(spline, parameter - 1.0f, temporary);
    for (int lane = 0; lane < 4; ++lane) blended[lane] = point[lane] * previous;
    point = splinePointAt(spline, parameter, temporary);
    for (int lane = 0; lane < 4; ++lane) blended[lane] = point[lane] * current + blended[lane];
    point = splinePointAt(spline, parameter + 1.0f, temporary);
    for (int lane = 0; lane < 4; ++lane) blended[lane] = point[lane] * next + blended[lane];
    point = splinePointAt(spline, parameter + 2.0f, temporary);
    for (int lane = 0; lane < 4; ++lane) output[lane] = point[lane] * following + blended[lane];
    return output;
}

float* splineEvaluate(const Spline* spline, float output[4], float parameter)
{
    const float base = floorf(parameter);
    const float fraction = parameter - base;
    const float square = fraction * fraction;
    const float cube = square * fraction;
    const float previous = ((square + square) - cube - fraction) * 0.5f;
    const float current = ((cube * 3.0f - square * 5.0f) + 2.0f) * 0.5f;
    const float next = ((square * 4.0f - cube * 3.0f) + fraction) * 0.5f;
    const float following = (cube - square) * 0.5f;
    return splineBlendPoints(spline, output, base, previous, current, next, following);
}

float* splineEvaluateTangent(const Spline* spline, float output[4], float parameter)
{
    const float base = floorf(parameter);
    const float fraction = parameter - base;
    const float square = fraction * fraction;
    const float previous = ((square * -3.0f + fraction * 4.0f) - 1.0f) * 0.5f;
    const float current = (square * 9.0f - fraction * 10.0f) * 0.5f;
    const float next = ((square * -9.0f + fraction * 8.0f) + 1.0f) * 0.5f;
    const float following = (square * 3.0f - (fraction + fraction)) * 0.5f;
    return splineBlendPoints(spline, output, base, previous, current, next, following);
}

float* splineEvaluateCurvature(const Spline* spline, float output[4], float parameter)
{
    const float base = floorf(parameter);
    const float fraction = parameter - base;
    const float previous = (fraction * -6.0f + 4.0f) * 0.5f;
    const float current = (fraction * 18.0f - 10.0f) * 0.5f;
    const float next = (fraction * -18.0f + 8.0f) * 0.5f;
    const float following = (fraction * 6.0f - 2.0f) * 0.5f;
    return splineBlendPoints(spline, output, base, previous, current, next, following);
}

void splineBuildLengthLookup(Spline* spline)
{
    // This native lookup is built only for authored, nondegenerate curves.
    // A borrowed evaluation-only spline may have lookupCapacity==0; the
    // previous doubling loop stayed at zero forever and froze the host.
    if (!spline || !spline->points || spline->count <
            (spline->boundary == SPLINE_CLOSED ? 3 : 2) ||
        spline->lookupCapacity < 1 ||
        (spline->boundary != SPLINE_CLOSED && spline->boundary != SPLINE_CLAMP &&
         spline->boundary != SPLINE_EXTRAPOLATE))
        throw std::invalid_argument("Spline has no valid length-lookup geometry/capacity");
    const float maximum = splineMaximumParameter(spline);
    float previous[4], current[4];
    splineEvaluate(spline, previous, 0.0f);
    float estimate = 0.0f;
    for (float parameter = 0.1f; parameter <= maximum; parameter += 0.1f) {
        splineEvaluate(spline, current, parameter);
        estimate += splinePointDistance(previous, current);
        memcpy(previous, current, sizeof(previous));
    }
    estimate *= 1.4f;
    const float scaledLength = estimate * 65536.0f;
    if (!(estimate > 0.0f) || !std::isfinite(scaledLength))
        throw std::invalid_argument("Spline length estimate is degenerate or non-finite");
    int32_t stepIndex = 0;
    for (;;) {
        for (stepIndex = 0; stepIndex < 7; ++stepIndex)
            if (scaledLength <= (float)spline->lookupCapacity * distanceUnits[stepIndex]) break;
        if (stepIndex != 7) break;
        if (spline->lookupCapacity > INT32_MAX / 2)
            throw std::length_error("Spline length lookup capacity cannot grow");
        spline->lookupCapacity *= 2;
    }
    spline->lookupStepIndex = stepIndex;
    free(spline->parameterAtDistance);
    spline->parameterAtDistance = static_cast<float*>(splineAllocateStorage((size_t)spline->lookupCapacity, sizeof(float)));
    spline->parameterAtDistance[0] = 0.0f;
    // Keep a trailing endpoint slot. The original estimated capacity is
    // sufficient for normal authored curves; this grows only when a curve's
    // local segment length exceeds that coarse estimate. Existing samples
    // and their floating-point arithmetic are left unchanged.
    const auto reserveEntry = [&](int32_t entry) {
        if (entry < spline->lookupCapacity) return;
        int32_t next = spline->lookupCapacity;
        while (entry >= next) {
            if (next > INT32_MAX / 2)
                throw std::length_error("Spline lookup exceeds its signed index range");
            next *= 2;
        }
        float* expanded = static_cast<float*>(splineAllocateStorage(static_cast<size_t>(next), sizeof(float)));
        memcpy(expanded,spline->parameterAtDistance,
            static_cast<size_t>(spline->lookupCapacity) * sizeof(float));
        free(spline->parameterAtDistance);
        spline->parameterAtDistance = expanded;
        spline->lookupCapacity = next;
    };
    const float interval = distanceUnits[stepIndex] * 0.0000152587890625f;
    const float parameterStep = 0.001953125f;
    float nextDistance = interval;
    float total = 0.0f;
    float priorTotal = 0.0f;
    int32_t nextEntry = 1;
    splineEvaluate(spline, previous, 0.0f);
    for (int32_t segment = 0; (float)segment < floorf(maximum); ++segment) {
        for (float parameter = (float)segment; parameter <= (float)(segment + 1); parameter += parameterStep) {
            splineEvaluate(spline, current, parameter);
            total += splinePointDistance(previous, current);
            while (nextDistance < total) {
                reserveEntry(nextEntry + 1);
                const float fraction = (nextDistance - priorTotal) / interval;
                const float offset = fraction * parameterStep;
                spline->parameterAtDistance[nextEntry++] = (parameter - parameterStep) + offset;
                nextDistance += interval;
            }
            memcpy(previous, current, sizeof(previous));
            priorTotal = total;
        }
    }
    reserveEntry(nextEntry);
    const float lastParameter = spline->parameterAtDistance[nextEntry - 1];
    const float tail = (maximum - lastParameter) * interval;
    const float remainder = total - (nextDistance - interval);
    if (!(remainder > 0.0f) || !std::isfinite(remainder))
        throw std::invalid_argument("Spline distance lookup has no finite terminal segment");
    spline->parameterAtDistance[nextEntry] = lastParameter + tail / remainder;
    spline->lookupCount = nextEntry + 1;
    spline->length = total;
    spline->lengthDirty = 0;
}

float splineLength(Spline* spline)
{
    if (spline->lengthDirty) splineBuildLengthLookup(spline);
    return spline->length;
}

float splineParameterAtDistance(Spline* spline, float distance)
{
    if (!spline) throw std::invalid_argument("Spline distance query has no owner");
    if (spline->lengthDirty) splineBuildLengthLookup(spline);
    if (!spline->parameterAtDistance || spline->lookupStepIndex < 0 || spline->lookupStepIndex >= 7 ||
        spline->lookupCount < 2 || !std::isfinite(distance))
        throw std::invalid_argument("Spline distance query has no finite, active lookup");
    const float divided = distance / distanceSteps[spline->lookupStepIndex];
    // Avoid undefined float-to-int conversion and an out-of-bounds sample.
    // For the source's valid domain the original interpolation is unchanged.
    if (!std::isfinite(divided) || divided < 0.0f ||
        divided >= static_cast<float>(spline->lookupCount - 1))
        throw std::out_of_range("Spline distance query exceeds the authored lookup");
    const float scaled = distance * distanceScales[spline->lookupStepIndex];
    const int32_t lower = (int32_t)divided;
    const float fraction = scaled - (float)(int32_t)scaled;
    const float first = spline->parameterAtDistance[lower];
    const float change = spline->parameterAtDistance[lower + 1] - first;
    return first + fraction * change;
}

float splineTangentProjection(const Spline* spline, const float point[4], float parameter)
{
    float position[4], tangent[4], difference[4];
    splineEvaluate(spline, position, parameter);
    splineEvaluateTangent(spline, tangent, parameter);
    for (int lane = 0; lane < 4; ++lane) difference[lane] = position[lane] - point[lane];
    return geometryQueryDot3(difference, tangent);
}

float splineDirectionCosine(const Spline* spline, const float point[4], float parameter)
{
    float position[4], tangent[4], difference[4];
    splineEvaluate(spline, position, parameter);
    splineEvaluateTangent(spline, tangent, parameter);
    for (int lane = 0; lane < 4; ++lane) difference[lane] = position[lane] - point[lane];
    const float firstLength = geometryQueryDot3(difference, difference);
    const float secondLength = geometryQueryDot3(tangent, tangent);
    // ZonePreview's real avoidance search (PAL1660D4 ->18A238) queries
    // direction before its later distance/occlusion gates. The query may
    // coincide with an eye point or meet a stationary tangent. Neither has
    // a preferred direction: its cosine contribution is zero. Preserve the
    // established float operations for ordinary finite nonzero norms;
    // wider intermediates handle tiny nonzero vectors without flushing them
    // or manufacturing a console reciprocal-square-root saturation value.
    if(!std::isnormal(firstLength) || !std::isnormal(secondLength)) {
        double delta[3],direction[3],deltaSquared=0.0,directionSquared=0.0;
        for(int axis=0;axis<3;++axis) {
            delta[axis]=static_cast<double>(position[axis])-static_cast<double>(point[axis]);
            direction[axis]=static_cast<double>(tangent[axis]);
            deltaSquared+=delta[axis]*delta[axis];directionSquared+=direction[axis]*direction[axis];
        }
        if(!std::isfinite(deltaSquared) || !std::isfinite(directionSquared))
            throw std::invalid_argument("Spline direction query requires finite geometry");
        if(deltaSquared==0.0 || directionSquared==0.0)return 0.0f;
        const double deltaLength=std::sqrt(deltaSquared),directionLength=std::sqrt(directionSquared);
        double cosine=0.0;
        for(int axis=0;axis<3;++axis)cosine+=(delta[axis]/deltaLength)*(direction[axis]/directionLength);
        return static_cast<float>(cosine);
    }
    const float firstScale = 1.0f / sqrtf(firstLength);
    const float secondScale = 1.0f / sqrtf(secondLength);
    for (int lane = 0; lane < 4; ++lane) {
        difference[lane] *= firstScale;
        tangent[lane] *= secondScale;
    }
    return geometryQueryDot3(difference, tangent);
}

float splineFindDirectionParameter(const Spline* spline, const float point[4],
                                   float cosine, float tolerance)
{
    float previousError = splineDirectionCosine(spline, point, 0.0f) - cosine;
    float lastError = 0.0f;
    float closestDistance = 0.0f;
    int32_t selected = 0;
    for (int32_t i = 1; i < spline->count; ++i) {
        lastError = splineDirectionCosine(spline, point, (float)i) - cosine;
        if (previousError * lastError < 0.0f) {
            float middle[4];
            for (int lane = 0; lane < 4; ++lane)
                middle[lane] = spline->points[i - 1][lane] * 0.5f + spline->points[i][lane] * 0.5f;
            const float distance = geometryQueryPointDistance(point, middle);
            if (selected == 0 || distance < closestDistance) {
                closestDistance = distance;
                selected = i;
            }
        }
        previousError = lastError;
    }
    if (selected == 0) return previousError < 0.0f ? geometryQueryMaximumParameter(spline) + 1.0f : -1.0f;
    float lower = (float)(selected - 1);
    float upper = (float)selected;
    float parameter = lower;
    float error = splineDirectionCosine(spline, point, parameter) - cosine;
    int32_t iteration = 0;
    while (tolerance < fabsf(error) && iteration < 10) {
        parameter = (lower + upper) * 0.5f;
        error = splineDirectionCosine(spline, point, parameter) - cosine;
        if (error * lastError < 0.0f) lower = parameter;
        else { upper = parameter; lastError = error; }
        ++iteration;
    }
    return parameter;
}

float splineFindClosestParameter(const Spline* spline, const float point[4], float tolerance)
{
    float lowerError = splineTangentProjection(spline, point, 0.0f);
    float upperError = 0.0f;
    int32_t selected;
    for (selected = 1; selected < spline->count; ++selected) {
        upperError = splineTangentProjection(spline, point, (float)selected);
        if (lowerError * upperError < 0.0f) break;
        lowerError = upperError;
    }
    if (selected >= spline->count) return lowerError < 0.0f ? geometryQueryMaximumParameter(spline) : 0.0f;
    float lower = (float)(selected - 1);
    float upper = (float)selected;
    float parameter = lower;
    float error = lowerError;
    int32_t iteration = 0;
    while (tolerance < fabsf(error) && iteration < 10) {
        parameter = (lower + upper) * 0.5f;
        error = splineTangentProjection(spline, point, parameter);
        if (error * upperError < 0.0f) lower = parameter;
        else { upper = parameter; upperError = error; }
        ++iteration;
    }
    return parameter;
}
