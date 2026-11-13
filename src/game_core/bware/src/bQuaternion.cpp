#include "bware/inc/bMath.hpp"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

float* vectorQuaternionConjugate(float output[4], const float input[4])
{
    const float x = -input[0], y = -input[1], z = -input[2], w = input[3];
    output[0] = x; output[1] = y; output[2] = z; output[3] = w; return output;
}
float vectorQuaternionLength(const float value[4])
{
    const float xx = value[0] * value[0], yy = value[1] * value[1];
    const float zz = value[2] * value[2], ww = value[3] * value[3];
    return sqrtf(((xx + yy) + zz) + ww);
}
float* vectorQuaternionNormalize(float output[4], const float input[4])
{
    const float length = vectorQuaternionLength(input);
    const float scale = finiteRatio(1.0f, length);
    float result[4]; for (int i = 0; i < 4; ++i) result[i] = input[i] * scale;
    memcpy(output, result, sizeof(result)); return output;
}
float* vectorQuaternionMultiply(float output[4], const float a[4], const float b[4])
{
    const float xx = a[0] * b[0], yy = a[1] * b[1], zz = a[2] * b[2];
    const float dot = (xx + yy) + zz;
    const float crossXFirst = a[1] * b[2], crossXSecond = b[1] * a[2];
    const float crossYFirst = a[2] * b[0], crossYSecond = b[2] * a[0];
    const float crossZFirst = a[0] * b[1], crossZSecond = b[0] * a[1];
    const float crossX = crossXFirst - crossXSecond;
    const float crossY = crossYFirst - crossYSecond;
    const float crossZ = crossZFirst - crossZSecond;
    const float bx = b[0] * a[3], by = b[1] * a[3], bz = b[2] * a[3];
    const float x = a[0] * b[3], y = a[1] * b[3], z = a[2] * b[3];
    const float w = a[3] * b[3];
    output[0] = x + (bx + crossX); output[1] = y + (by + crossY); output[2] = z + (bz + crossZ);
    output[3] = w - dot; return output;
}
void vectorQuaternionToMatrix(const float q[4], float matrix[16])
{
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float twiceX = x + x, twiceY = y + y, twiceZ = z + z;
    const float xx = x * twiceX, yy = y * twiceY, zz = z * twiceZ;
    const float wz = w * twiceZ, xz = x * twiceZ, wx = w * twiceX;
    const float yz = y * twiceZ, wy = w * twiceY, xy = x * twiceY;
    matrix[3] = matrix[7] = matrix[11] = matrix[12] = matrix[13] = matrix[14] = 0.0f; matrix[15] = 1.0f;
    matrix[8] = xz - wy; matrix[4] = xy + wz; matrix[9] = yz + wx;
    matrix[0] = 1.0f - (yy + zz); matrix[5] = 1.0f - (xx + zz); matrix[10] = 1.0f - (xx + yy);
    matrix[1] = xy - wz; matrix[2] = xz + wy; matrix[6] = yz - wx;
}
void vectorMatrixFromQuaternion(float matrix[16], const float q[4])
{
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float twiceX = x + x, twiceY = y + y, twiceW = w + w, twiceZ = z + z;
    const float xx = twiceX * x, yy = twiceY * y, wy = twiceW * y;
    const float wx = twiceW * x, xy = twiceX * y, yz = twiceY * z;
    const float xz = twiceX * z, wz = twiceW * z, zz = twiceZ * z;
    const float withoutX = 1.0f - xx, withoutY = 1.0f - yy;
    matrix[3] = matrix[7] = matrix[11] = matrix[12] = matrix[13] = matrix[14] = 0.0f; matrix[15] = 1.0f;
    matrix[4] = xy + wz; matrix[8] = xz - wy; matrix[9] = yz + wx;
    matrix[10] = withoutX - yy; matrix[0] = withoutY - zz; matrix[5] = withoutX - zz;
    matrix[1] = xy - wz; matrix[2] = xz + wy; matrix[6] = yz - wx;
}

float* vectorQuaternionFromMatrix(float output[4], const float matrix[16])
{ vectorMathQuaternionFromMatrix(output, matrix); return output; }

float* vectorQuaternionSlerp(float output[4], const float a[4], const float b[4], float fraction)
{
    const float xx = a[0] * b[0], yy = a[1] * b[1], zz = a[2] * b[2], ww = a[3] * b[3];
    const float dot = ((xx + yy) + zz) + ww;
    float firstWeight = 1.0f - fraction, secondWeight = fraction;
    if (0.05000000074505806f < 1.0f - fabsf(dot)) {
        const uint16_t angle = (uint16_t)(16384u - vectorArcSineAngle(fabsf(dot)));
        const float denominator = rotationSineAngle(angle);
        const float angleValue = (float)angle;
        const float secondCoordinate = fraction * angleValue;
        const float firstCoordinate = firstWeight * angleValue;
        firstWeight = finiteRatio(rotationSineAngle((uint16_t)(int32_t)firstCoordinate), denominator);
        secondWeight = finiteRatio(rotationSineAngle((uint16_t)(int32_t)secondCoordinate), denominator);
    }
    if (dot < 0.0f) firstWeight = -firstWeight;
    float result[4];
    for (int i = 0; i < 4; ++i) {
        const float firstTerm = a[i] * firstWeight, secondTerm = b[i] * secondWeight;
        result[i] = secondTerm + firstTerm;
    }
    memcpy(output, result, sizeof(result)); return output;
}

void vectorQuaternionFromMatrixCopy(float output[4], const float matrix[16])
{ float copy[16]; memcpy(copy, matrix, sizeof(copy)); vectorMathQuaternionFromMatrix(output, copy); }
