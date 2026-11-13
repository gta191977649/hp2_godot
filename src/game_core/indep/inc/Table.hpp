#ifndef NFSHP2_INDEP_INC_TABLE_HPP
#define NFSHP2_INDEP_INC_TABLE_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CurveKey {
    float coordinate;
    float value;
} CurveKey;

typedef struct KeyedCurve {
    const CurveKey* keys;
    int32_t count;
} KeyedCurve;

// The source requires at least one key. Increasing coordinates/values make
// the corresponding forward/inverse search useful; inputs must be finite.
float keyedCurveEvaluate(const KeyedCurve* curve, float coordinate);
float keyedCurveInverse(const KeyedCurve* curve, float value);

typedef struct CarCameraParameterMap {
    float scale;
    float offset;
} CarCameraParameterMap;

void carCameraParameterMapInitialize(CarCameraParameterMap* mapping,
                                      float firstParameter, float firstValue,
                                      float lastParameter, float lastValue);
float carCameraParameterMapEvaluate(const CarCameraParameterMap* mapping, float parameter);
float carCameraParameterMapInverse(const CarCameraParameterMap* mapping, float value);

#ifdef __cplusplus
}
#endif



#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DriverHistory {
    uint8_t capacity;
    uint8_t next;
    uint8_t count;
    float sum;
    float value;
    float* samples;
} DriverHistory;

/* Capacity is positive. The native owner embeds the record and owns only
 * the allocated sample array, which dispose releases. */
DriverHistory* driverHistoryInitialize(DriverHistory* history, uint8_t capacity);
void driverHistoryDispose(DriverHistory* history);
void driverHistoryPush(DriverHistory* history, float sample);
/* Reset retains the current output and next index. Fill sets the output and
 * marks every sample present. These are distinct original operations. */
void driverHistoryReset(DriverHistory* history, float sample);
void driverHistoryFill(DriverHistory* history, float sample);
float driverHistoryLatest(const DriverHistory* history);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct ReplaySnapshot;
void driverHistorySnapshot(DriverHistory* history,struct ReplaySnapshot* snapshot);
struct DriverFilter;
void driverFilterSnapshot(struct DriverFilter* filter,struct ReplaySnapshot* snapshot);
#ifdef __cplusplus
}
#endif


// Original Table/PidError native declarations; existing host layouts retained.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Samples are borrowed and remain owned by the vehicle configuration.
typedef struct SampledCurve {
    const float* samples;
    int32_t sampleCount;
    float minimum;
    float maximum;
    float coordinateScale;
} SampledCurve;

// Callers supply a non-null curve and at least two finite samples. The
// finite input range must increase and produce a finite, positive scale.
// Evaluation inputs and all intermediate arithmetic must remain finite;
// the scaled input must be representable as a valid sample index. These
// operations preserve the configured behavior rather than validate data.
SampledCurve* sampledCurveInitialize(SampledCurve* curve, const float* samples,
                                    int32_t sampleCount,
                                    float minimum, float maximum);
void sampledCurveSetRange(SampledCurve* curve, float minimum, float maximum);

// This updates only the sample reference and count. Call SetRange afterwards
// when a different count requires a new coordinate scale.
SampledCurve* sampledCurveSetSamples(SampledCurve* curve, const float* samples,
                                    int32_t sampleCount);

// Clamp to the input range, then interpolate the surrounding sample pair.
float sampledCurveEvaluate(const SampledCurve* curve, float input);

// Search in sample order for the first value match or crossing. A missing
// crossing returns the final input coordinate. The initial tolerance match
// deliberately returns the first sample VALUE, even for a nonzero minimum.
// Samples need not be monotonic; inverse does not choose the closest crossing.
float sampledCurveInverse(const SampledCurve* curve, float value);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef struct DriverFilter {
    DriverHistory integral;
    DriverHistory derivative;
    float current;
    float previous;
} DriverFilter;

int32_t driverFilterInitialize(DriverFilter* filter,uint8_t integralCapacity,uint8_t derivativeCapacity);
void driverFilterDispose(DriverFilter* filter);
void driverFilterUpdate(DriverFilter* filter,float value,float elapsed);
void driverFilterReset(DriverFilter* filter,float previous);

#ifdef __cplusplus
}
#endif

#endif
