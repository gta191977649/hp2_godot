#ifndef NFSHP2_INDEP_QUICKSPLINE_HPP
#define NFSHP2_INDEP_QUICKSPLINE_HPP

#include "indep/inc/Spline.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum QuickSplineBoundary {
    QUICK_SPLINE_CLOSED = 0,
    QUICK_SPLINE_CLAMP = 1,
    QUICK_SPLINE_EXTRAPOLATE = 2
} QuickSplineBoundary;

typedef struct QuickSpline {
    QuickSplineBoundary boundary;
    int32_t basisType;
    float maximumParameter;
    float minimumParameter;
    float period;
    int pointsDirty;
    int ownsStorage;
    uint8_t paddingCount;
    uint16_t capacity;
    uint16_t count;
    float (*storage)[4];
    float (*points)[4];
    const float* basis;
} QuickSpline;

// Each storage block reserves one point before and two after the real points.
// The recovered basisType is zero (Catmull-Rom). Other values leave basis null
// until the owner provides its three 4x4 basis matrices explicitly.
QuickSpline* quickSplineInitialize(QuickSpline* spline, QuickSplineBoundary boundary,
                                   int32_t basisType, uint16_t capacity);
QuickSpline* quickSplineInitializeBorrowed(QuickSpline* spline, QuickSplineBoundary boundary,
                                           int32_t basisType, float (*storage)[4],
                                           int32_t storageCount);
void quickSplineDestroy(QuickSpline* spline, int releaseObject);
void quickSplineConfigure(QuickSpline* spline, QuickSplineBoundary boundary, int32_t basisType);
void quickSplineSnapshot(QuickSpline* spline, CurveSnapshot* snapshot, int32_t components);
void quickSplineSetPointCount(QuickSpline* spline, uint16_t count);
void quickSplinePreparePoints(QuickSpline* spline);
float* quickSplineBlendPoints(const QuickSpline* spline, float output[4],
                              float fraction, int32_t derivativeOrder,
                              const float controlPoints[16]);
float quickSplineBoundParameter(const QuickSpline* spline, float parameter);
void quickSplineGetControlPoints(const QuickSpline* spline, float output[16], int32_t first);
float* quickSplineEvaluate(QuickSpline* spline, float output[4],
                           float parameter, int32_t derivativeOrder);
void quickSplineCalculateLength(QuickSpline* spline, int32_t subdivisions);

int32_t quickSplineFindLocalClosestPoint(const QuickSpline* spline,
                                        const float point[4], int32_t initialIndex);
int32_t quickSplineFindClosestPoint(const QuickSpline* spline, const float point[4]);
float quickSplineFindClosestParameter(QuickSpline* spline, const float point[4],
                                      float initialParameter, float tolerance);

#ifdef __cplusplus
}
#endif
#endif
