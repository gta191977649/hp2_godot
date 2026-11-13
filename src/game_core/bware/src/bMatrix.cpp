#include "bware/inc/bMath.hpp"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#if defined(_M_X64) || defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define NFSHP2_NATIVE_SSE2 1
#else
#define NFSHP2_NATIVE_SSE2 0
#endif

void rotationMatrixIdentity(float matrix[16])
{
    for (int i = 0; i < 16; ++i)
        matrix[i] = 0.0f;
    matrix[0] = 1.0f;
    matrix[5] = 1.0f;
    matrix[10] = 1.0f;
    matrix[15] = 1.0f;
}

void vectorMatrixMultiply(float output[16], const float left[16], const float right[16])
{
    float result[16];
    for (int column = 0; column < 4; ++column) vectorMatrixTransform(result + column * 4, left, right + column * 4);
    memcpy(output, result, sizeof(result));
}
void vectorMatrixInverseAffine(float output[16], const float matrix[16])
{
    const float a = matrix[0], b = matrix[1], c = matrix[2];
    const float d = matrix[4], e = matrix[5], f = matrix[6];
    const float g = matrix[8], h = matrix[9], i = matrix[10];
    const float x = matrix[12], y = matrix[13], z = matrix[14];
    // PAL25ACC0..25ADF4: keep the original scalar product order and
    // rounding boundary of every cofactor, determinant and translation sum.
    const float cofactorD = bScalarAdd(bScalarMultiply(d, i), -bScalarMultiply(f, g));
    const float cofactorA = bScalarAdd(bScalarMultiply(e, i), -bScalarMultiply(f, h));
    const float cofactorG = bScalarAdd(bScalarMultiply(d, h), -bScalarMultiply(e, g));
    const float firstTerm = bScalarMultiply(a, cofactorA);
    const float secondTerm = bScalarMultiply(b, cofactorD);
    const float thirdTerm = bScalarMultiply(c, cofactorG);
    const float determinant = bScalarAdd(bScalarAdd(firstTerm, -secondTerm), thirdTerm);
    const float inverse = bScalarDivide(1.0f, determinant), negativeInverse = -inverse;
    float result[16];
    result[0] = bScalarMultiply(inverse, cofactorA);
    result[2] = bScalarMultiply(inverse, bScalarAdd(bScalarMultiply(b, f), -bScalarMultiply(c, e)));
    result[4] = bScalarMultiply(negativeInverse, cofactorD);
    result[6] = bScalarMultiply(negativeInverse, bScalarAdd(bScalarMultiply(a, f), -bScalarMultiply(c, d)));
    result[1] = bScalarMultiply(negativeInverse, bScalarAdd(bScalarMultiply(b, i), -bScalarMultiply(c, h)));
    result[5] = bScalarMultiply(inverse, bScalarAdd(bScalarMultiply(a, i), -bScalarMultiply(c, g)));
    result[10] = bScalarMultiply(inverse, bScalarAdd(bScalarMultiply(a, e), -bScalarMultiply(b, d)));
    result[8] = bScalarMultiply(inverse, cofactorG);
    result[9] = bScalarMultiply(negativeInverse, bScalarAdd(bScalarMultiply(a, h), -bScalarMultiply(b, g)));
    result[3] = result[7] = result[11] = 0.0f; result[15] = 1.0f;
    result[14] = -bScalarAdd(bScalarAdd(bScalarMultiply(x, result[2]), bScalarMultiply(y, result[6])), bScalarMultiply(z, result[10]));
    result[12] = -bScalarAdd(bScalarAdd(bScalarMultiply(x, result[0]), bScalarMultiply(y, result[4])), bScalarMultiply(z, result[8]));
    result[13] = -bScalarAdd(bScalarAdd(bScalarMultiply(x, result[1]), bScalarMultiply(y, result[5])), bScalarMultiply(z, result[9]));
    memcpy(output, result, sizeof(result));
}

void vectorMatrixTransform(float output[4], const float matrix[16], const float input[4])
{
    // Column-major matrix, column vector. Snapshot input before storing output:
    // engine callers are allowed to alias their input and result. Native SSE2
    // executes the four independent float32 lanes without console arithmetic.
    const float x = input[0], y = input[1], z = input[2], w = input[3];
#if NFSHP2_NATIVE_SSE2
    __m128 result = _mm_mul_ps(_mm_loadu_ps(matrix), _mm_set1_ps(x));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_loadu_ps(matrix + 4), _mm_set1_ps(y)));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_loadu_ps(matrix + 8), _mm_set1_ps(z)));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_loadu_ps(matrix + 12), _mm_set1_ps(w)));
    _mm_storeu_ps(output, result);
#else
    float result[4];
    for (int row = 0; row < 4; ++row)
        result[row] = ((matrix[row]*x + matrix[row+4]*y) + matrix[row+8]*z) + matrix[row+12]*w;
    memcpy(output, result, sizeof(result));
#endif
}

float* vectorMatrixTranspose(float output[16], const float matrix[16])
{
    float result[16];
    for (int column = 0; column < 4; ++column) for (int row = 0; row < 4; ++row) result[column * 4 + row] = matrix[row * 4 + column];
    memcpy(output, result, sizeof(result)); return output;
}

void vectorMatrixTransformPoint(float output[4], const float matrix[16], const float input[4])
{
    const float point[4] = {input[0], input[1], input[2], 1.0f};
    vectorMatrixTransform(output, matrix, point);
}
