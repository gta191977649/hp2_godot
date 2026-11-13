#include "indep/inc/Replay.hpp"
#include "indep/inc/Table.hpp"
#include "bware/inc/bMath.hpp"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void carCameraParameterMapInitialize(CarCameraParameterMap* mapping,
                                      float firstParameter, float firstValue,
                                      float lastParameter, float lastValue)
{
    const float valueRange = lastValue - firstValue;
    const float parameterRange = lastParameter - firstParameter;
    const float scale = finiteRatio(valueRange, parameterRange);
    const float origin = scale * firstParameter;
    mapping->scale = scale;
    mapping->offset = firstValue - origin;
}

float carCameraParameterMapEvaluate(const CarCameraParameterMap* mapping, float parameter)
{
    const float scaled = mapping->scale * parameter;
    return scaled + mapping->offset;
}

void driverFilterUpdate(DriverFilter* filter,float value,float elapsed)
{
    const float previous = filter->current;
    filter->current = value;
    const float change = value-previous;
    filter->previous = previous;
    const float midpoint = previous+change*0.5f;
    driverHistoryPush(&filter->integral,elapsed*midpoint);
    driverHistoryPush(&filter->derivative,finiteRatio(change,elapsed));
}

void driverFilterReset(DriverFilter* filter,float previous)
{
    filter->current = 0.0f;filter->previous = previous;
    driverHistoryReset(&filter->integral,0.0f);driverHistoryReset(&filter->derivative,0.0f);
}

void driverHistoryPush(DriverHistory* history, float sample)
{
    if (history->count < history->capacity) ++history->count;
    const float added = bScalarAdd(history->sum, sample);
    history->sum = bScalarAdd(added, -(history->samples[history->next]));
    history->samples[history->next] = sample;
    history->value = bScalarDivide(history->sum, (float)history->count);
    history->next = (uint8_t)(history->next + 1u);
    if (history->capacity <= history->next) history->next = 0;
}

void driverHistoryReset(DriverHistory* history, float sample)
{
    for (uint32_t index = 0; index < history->capacity; ++index) history->samples[index] = sample;
    history->count = 0;
    history->sum = sample * (float)history->capacity;
}

void driverHistoryFill(DriverHistory* history, float sample)
{
    for (uint32_t index = 0; index < history->capacity; ++index) history->samples[index] = sample;
    history->value = sample;
    history->count = history->capacity;
    history->sum = sample * (float)history->capacity;
}

float driverHistoryLatest(const DriverHistory* history)
{
    if (history->count == 0) return 0.0f;
    const uint8_t index = history->next == 0 ? (uint8_t)(history->capacity - 1u) : (uint8_t)(history->next - 1u);
    return history->samples[index];
}

SampledCurve* sampledCurveSetSamples(SampledCurve* curve, const float* samples,
                                    int32_t sampleCount)
{
    curve->samples = samples;
    curve->sampleCount = sampleCount;
    return curve;
}

float sampledCurveEvaluate(const SampledCurve* curve, float input)
{
    if (input < curve->minimum)
        input = curve->minimum;
    else if (curve->maximum < input)
        input = curve->maximum;

    const float offset = bScalarAdd(input, -curve->minimum);
    const float coordinate = bScalarMultiply(offset, curve->coordinateScale);
    // Integer conversion truncates, selecting the lower surrounding sample.
    const int32_t index = (int32_t)coordinate;
    if (index == curve->sampleCount - 1)
        return curve->samples[index];

    const float first = curve->samples[index];
    const float second = curve->samples[index + 1];
    const float fraction = bScalarAdd(coordinate, -(float)index);
    const float change = bScalarAdd(second, -first);
    const float interpolatedChange = bScalarMultiply(change, fraction);
    return bScalarAdd(first, interpolatedChange);
}

float keyedCurveEvaluate(const KeyedCurve* curve, float coordinate)
{
    const CurveKey* keys = curve->keys;
    if (curve->count < 2) return keys[0].value;
    if (coordinate <= keys[0].coordinate) return keys[0].value;
    if (keys[curve->count - 1].coordinate <= coordinate)
        return keys[curve->count - 1].value;
    for (int32_t i = 0; i < curve->count - 1; ++i) {
        if (keys[i].coordinate <= coordinate && coordinate < keys[i + 1].coordinate) {
            const float width = bScalarAdd(keys[i + 1].coordinate, -(keys[i].coordinate));
            const float change = bScalarAdd(keys[i + 1].value, -(keys[i].value));
            if (fabsf(width) <= 0.000001f)
                return bScalarAdd(keys[i].value, bScalarMultiply(change, 0.5f));
            const float fraction = bScalarDivide((bScalarAdd(coordinate, -(keys[i].coordinate))), width);
            return bScalarAdd(keys[i].value, bScalarMultiply(fraction, change));
        }
    }
    return keys[0].value;
}

float keyedCurveInverse(const KeyedCurve* curve, float value)
{
    const CurveKey* keys = curve->keys;
    // The original fallback intentionally returns the first key's VALUE.
    if (curve->count < 2) return keys[0].value;
    if (value <= keys[0].value) return keys[0].coordinate;
    if (keys[curve->count - 1].value <= value)
        return keys[curve->count - 1].coordinate;
    for (int32_t i = 0; i < curve->count - 1; ++i) {
        if (keys[i].value <= value && value < keys[i + 1].value) {
            const float width = keys[i + 1].value - keys[i].value;
            const float change = keys[i + 1].coordinate - keys[i].coordinate;
            if (fabsf(width) <= 0.000001f)
                return keys[i].coordinate + change * 0.5f;
            const float fraction = (value - keys[i].value) / width;
            return keys[i].coordinate + fraction * change;
        }
    }
    return keys[0].value;
}

DriverHistory* driverHistoryInitialize(DriverHistory* history, uint8_t capacity)
{
    if (!history || capacity == 0) return NULL;
    float* samples = (float*)calloc(capacity, sizeof(*samples));
    if (!samples) return NULL;
    history->capacity = capacity;
    history->next = 0;
    history->count = 0;
    history->sum = 0.0f;
    history->value = 0.0f;
    history->samples = samples;
    return history;
}

void driverHistoryDispose(DriverHistory* history)
{
    free(history->samples);
    history->samples = NULL;
}

// Complete PAL18FE38. The serialized capacity must fit the same owner's
// existing allocation; source restores never allocate a replacement history.
void driverHistorySnapshot(DriverHistory* history,ReplaySnapshot* snapshot)
{
    const uint8_t capacity=history->capacity;
    replaySnapshotBeginSection(snapshot,nullptr,"Average");
    replaySnapshotMoveData(snapshot,&history->capacity,1,1,0);
    replaySnapshotMoveData(snapshot,&history->next,1,1,0);
    replaySnapshotMoveData(snapshot,&history->count,1,1,0);
    replaySnapshotMoveFloat(snapshot,&history->sum);
    if (history->capacity>capacity || !history->samples) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
        replaySnapshotEndSection(snapshot);
        return;
    }
    for (uint32_t index=0;index<history->capacity;++index)
        replaySnapshotMoveFloat(snapshot,history->samples+index);
    if (snapshot->recording || snapshot->restoring)
        history->value=history->count ? history->sum/static_cast<float>(history->count) : 0.0f;
    replaySnapshotEndSection(snapshot);
}

// Full PAL18FF98..190008, PidError::DoSnapshot.
void driverFilterSnapshot(DriverFilter* filter,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"PidError");
    driverHistorySnapshot(&filter->integral,snapshot);
    driverHistorySnapshot(&filter->derivative,snapshot);
    replaySnapshotMoveFloat(snapshot,&filter->current);
    replaySnapshotMoveFloat(snapshot,&filter->previous);
    replaySnapshotEndSection(snapshot);
}

// PAL Table methods and the existing native PidError storage adapter.
static void sampledCurveUpdateScale(SampledCurve* curve)
{
    const float intervals = (float)(curve->sampleCount - 1);
    const float width = bScalarAdd(curve->maximum, -curve->minimum);
    curve->coordinateScale = bScalarDivide(intervals, width);
}

void sampledCurveSetRange(SampledCurve* curve, float minimum, float maximum)
{
    curve->minimum = minimum;
    curve->maximum = maximum;
    sampledCurveUpdateScale(curve);
}

SampledCurve* sampledCurveInitialize(SampledCurve* curve, const float* samples,
                                    int32_t sampleCount,
                                    float minimum, float maximum)
{
    curve->samples = samples;
    curve->sampleCount = sampleCount;
    sampledCurveSetRange(curve, minimum, maximum);
    return curve;
}

float sampledCurveInverse(const SampledCurve* curve, float value)
{
    const float tolerance = 0.000001f;
    const float first = curve->samples[0];
    const float firstDifference = value - first;
    if (fabsf(firstDifference) < tolerance)
        return first;

    int below = first < value;
    for (int32_t index = 1; index < curve->sampleCount; ++index) {
        const float current = curve->samples[index];
        const float difference = value - current;
        if (fabsf(difference) < tolerance) {
            const float offset = (float)index / curve->coordinateScale;
            return curve->minimum + offset;
        }

        const int currentBelow = current < value;
        if (currentBelow != below) {
            const float previous = curve->samples[index - 1];
            const float change = current - previous;
            const float fromPrevious = value - previous;
            const float fraction = fromPrevious / change;
            const float coordinate = (float)(index - 1) + fraction;
            const float offset = coordinate / curve->coordinateScale;
            return curve->minimum + offset;
        }
        below = currentBelow;
    }

    const float offset = (float)(curve->sampleCount - 1) / curve->coordinateScale;
    return curve->minimum + offset;
}

int32_t driverFilterInitialize(DriverFilter* filter,uint8_t integralCapacity,uint8_t derivativeCapacity)
{
    memset(filter,0,sizeof(*filter));
    if (!driverHistoryInitialize(&filter->integral,integralCapacity)) return 0;
    if (!driverHistoryInitialize(&filter->derivative,derivativeCapacity)) {
        driverHistoryDispose(&filter->integral);return 0;
    }
    return 1;
}

void driverFilterDispose(DriverFilter* filter)
{
    driverHistoryDispose(&filter->integral);driverHistoryDispose(&filter->derivative);
}

/* Camera path support. */
float carCameraParameterMapInverse(const CarCameraParameterMap* mapping, float value)
{
    if (fabsf(mapping->scale) <= 0.0010000000474974513f) return mapping->offset;
    return finiteRatio(value - mapping->offset, mapping->scale);
}
