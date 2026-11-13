#ifndef NFSHP2_INDEP_INC_EMATH_HPP
#define NFSHP2_INDEP_INC_EMATH_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

float motionSineRadians(float radians);
void motionMatrixYaw(float output[16], uint16_t angle);
void motionMatrixRotateX(float output[16], const float input[16], uint16_t angle);
void motionMatrixRotateY(float output[16], const float input[16], uint16_t angle);
void motionMatrixRotateYaw(float output[16], const float input[16], uint16_t angle);
void motionMatrixTranslate(float output[16], const float input[16], const float translation[3]);
void motionMatrixTranslation(float output[16], const float translation[3]);
float* motionMatrixOrthonormalize(float output[16], const float input[16]);
float* motionMatrixTransposeRotation(float output[16], const float input[16]);
float* eTransposeMatrix(float output[16], const float input[16]);
void motionMatrixAxisAngle(float output[16], const float axis[3], uint16_t angle);
float* motionMatrixAngularDerivative(float output[16], const float angularVelocity[3],
                                     const float orientation[16]);

/* Shared native rotation support; no additional ELF body. */
void motionAngleComponents(uint16_t angle, float* sine, float* cosine);

extern float eMathIdentityMatrix[16];
extern float eMathZeroMatrix[16];
void eMathInit(void);
void eIdentity(float output[16]);
void eZeroMatrix(float output[16]);
/* The original e-API multiplies second * first. Both inputs may alias output. */
void eMulMatrix(float output[16],const float first[16],const float second[16]);
void eMulVector4(float output[4],const float matrix[16],const float input[4]);
extern float eUnitVector4[4];
/* ActualVector3 API writes four output lanes; input W is ignored. */
void eMulVector3(float output[4],const float matrix[16],const float input[3]);
/* Original direction normalization uses XY length; its VU W result was
 * unwritten. Native directions define W=0. Inputs must be finite. */
void eNormalizedDirection(float output[4],const float first[4],const float second[4]);
/* Position input W is ignored. Perspective output retains clip W. */
void eRotTransPers(float output[4],const float matrix[16],const float input[4]);
uint32_t eGetClipFlags(const float input[4],const float matrix[16]);

/* Existing private native suboperations of qualified eCreateLookAtMatrix. */
float carCinematicLengthThree(const float value[4]);
void carCinematicNormalizeThree(float value[4]);
void carCinematicCrossThree(float output[4], const float first[4], const float second[4]);

void carCameraInvertRigidTransform(float output[16], const float input[16]);

#ifdef __cplusplus
}
#endif

#endif
