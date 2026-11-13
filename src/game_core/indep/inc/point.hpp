#ifndef NFSHP2_INDEP_INC_POINT_HPP
#define NFSHP2_INDEP_INC_POINT_HPP

// Native scalar/vector cubic ABI used by the proved cPoint::SplineSeek
// owner. PAL callback/PDR evidence licenses the original point.hpp path;
// this is not a recovered beta class layout or a complete cPoint port.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tCubic1D {
    float value;
    float derivative;
    float desiredValue;
    float desiredDerivative;
    float cubic;
    float quadratic;
    float linear;
    float constant;
    float parameter;
    float duration;
    float threshold;
    int16_t mode;
    int16_t continuous;
} tCubic1D;

typedef struct tCubic2D { tCubic1D axes[2]; } tCubic2D;
typedef struct tCubic3D { tCubic1D axes[3]; } tCubic3D;

/* Explicit native initialization for the inline scalar curves owned by a
 * mover or camera. Duration is positive; curve arguments are finite. */
void tCubic1DInitialize(tCubic1D* curve);
void tCubic1DMakeCoeffs(tCubic1D* curve);
float tCubic1DGetVal(const tCubic1D* curve, float parameter);
float tCubic1DGetdVal(const tCubic1D* curve, float parameter);
float tCubic1DGetddVal(const tCubic1D* curve, float parameter);
float tCubic1DVelocityAtTime(const tCubic1D* curve, float time);
float tCubic1DAccelerationAtTime(const tCubic1D* curve, float time);
void tCubic1DLimitEndVelocity(tCubic1D* curve, float maximum);
void tCubic1DLimitAcceleration(tCubic1D* curve, float maximum);
void tCubic1DUpdate(tCubic1D* curve, float elapsed,
                    float maximumVelocity, float maximumAcceleration);

void tCubic2DSetVal(tCubic2D* curve, const float value[2]);
void tCubic2DSetdVal(tCubic2D* curve, const float value[2]);
void tCubic2DSetValDesired(tCubic2D* curve, const float value[2]);
void tCubic2DSetdValDesired(tCubic2D* curve, const float value[2]);
void tCubic2DSetDuration(tCubic2D* curve, const float value[2]);
void tCubic2DSetThreshold(tCubic2D* curve, const float value[2]);
void tCubic2DGetVal(const tCubic2D* curve, float value[2]);
void tCubic2DGetdVal(const tCubic2D* curve, float value[2]);
void tCubic2DGetddVal(const tCubic2D* curve, float value[2]);
void tCubic2DGetValDesired(const tCubic2D* curve, float value[2]);
void tCubic2DGetdValDesired(const tCubic2D* curve, float value[2]);
void tCubic2DUpdate(tCubic2D* curve, float elapsed);

void tCubic3DSetVal(tCubic3D* curve, const float value[3]);
void tCubic3DSetdVal(tCubic3D* curve, const float value[3]);
void tCubic3DSetValDesired(tCubic3D* curve, const float value[3]);
void tCubic3DSetdValDesired(tCubic3D* curve, const float value[3]);
void tCubic3DSetDuration(tCubic3D* curve, const float value[3]);
void tCubic3DSetThreshold(tCubic3D* curve, const float value[3]);
void tCubic3DGetVal(const tCubic3D* curve, float value[3]);
void tCubic3DGetdVal(const tCubic3D* curve, float value[3]);
void tCubic3DGetddVal(const tCubic3D* curve, float value[3]);
void tCubic3DGetValDesired(const tCubic3D* curve, float value[3]);
void tCubic3DGetdValDesired(const tCubic3D* curve, float value[3]);
void tCubic3DUpdate(tCubic3D* curve, float elapsed);

#ifdef __cplusplus
}
#endif

#endif
