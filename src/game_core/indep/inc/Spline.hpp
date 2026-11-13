#ifndef NFSHP2_INDEP_SPLINE_HPP
#define NFSHP2_INDEP_SPLINE_HPP

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SplineBoundary {
    SPLINE_CLAMP = 0,
    SPLINE_CLOSED = 1,
    SPLINE_EXTRAPOLATE = 2
} SplineBoundary;

typedef struct CurveSnapshot {
    void* context;
    void (*begin)(void* context, const char* name);
    void (*moveBytes)(void* context, void* data, size_t size);
    void (*end)(void* context);
    int restoring;
} CurveSnapshot;

typedef struct Spline {
    int32_t capacity;
    int32_t count;
    SplineBoundary boundary;
    float (*points)[4];
    float (*ownedPoints)[4];
    int32_t lengthDirty;
    int32_t lookupCapacity;
    int32_t lookupStepIndex;
    float length;
    float snapshotValue;
    int32_t lookupCount;
    float* parameterAtDistance;
} Spline;

// Borrowed points remain caller-owned until growth creates owned storage.
// Zero-capacity splines grow on their first append. Finite, nondegenerate
// control polygons are required by the length lookup. Closed curves require three points, others two.
// Destroy releases the object itself only when releaseObject is odd; that
// mode requires an object allocated by the native C allocator. Snapshot restore
// requires backing storage of the recorded capacity; pointers are not saved.
Spline* splineInitialize(Spline* spline, SplineBoundary boundary,
                         int32_t capacity, int32_t lookupCapacity);
Spline* splineInitializeBorrowed(Spline* spline, SplineBoundary boundary,
                                 int32_t count, float (*points)[4],
                                 int32_t lookupCapacity);
void splineDestroy(Spline* spline, int releaseObject);
void splineSnapshot(Spline* spline, CurveSnapshot* snapshot);
void splineSetPoint(Spline* spline, int32_t index, const float point[4]);
void splineAppendPoint(Spline* spline, const float point[4]);
void splineRemoveFirstPoint(Spline* spline);
const float* splinePointAt(const Spline* spline, float parameter, float temporary[4]);
int32_t splineBoundarySupportCount(const Spline* spline);
float* splineBlendPoints(const Spline* spline, float output[4], float parameter,
                         float previous, float current, float next, float following);
float* splineEvaluate(const Spline* spline, float output[4], float parameter);
float* splineEvaluateTangent(const Spline* spline, float output[4], float parameter);
float* splineEvaluateCurvature(const Spline* spline, float output[4], float parameter);
void splineBuildLengthLookup(Spline* spline);
float splineControlPolygonLength(Spline* spline);
float splineLength(Spline* spline);
float splineParameterAtDistance(Spline* spline, float distance);

float splineTangentProjection(const Spline* spline, const float point[4], float parameter);
float splineDirectionCosine(const Spline* spline, const float point[4], float parameter);
float splineFindDirectionParameter(const Spline* spline, const float point[4],
                                   float cosine, float tolerance);
float splineFindClosestParameter(const Spline* spline, const float point[4], float tolerance);

#ifdef __cplusplus
}
#endif
#endif
