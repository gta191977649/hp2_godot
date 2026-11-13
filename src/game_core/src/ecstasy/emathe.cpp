#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include "CameraMover.hpp"
#include "indep/inc/eMath.hpp"

#include <float.h>
#include <math.h>
#include <string.h>

void motionMatrixTranslate(float output[16], const float input[16], const float translation[3])
{
    float transform[16];
    motionMatrixTranslation(transform, translation);
    vectorMatrixMultiply(output, transform, input);
}

void motionMatrixTranslation(float output[16], const float translation[3])
{
    rotationMatrixIdentity(output);
    output[12] = translation[0];
    output[13] = translation[1];
    output[14] = translation[2];
}

static void crossThree(float output[3], const float first[3], const float second[3])
{
    // Original OPMULA(first, second), OPMSUB(second, first).
    const float x1 = bScalarMultiply(first[1], second[2]), x2 = bScalarMultiply(second[1], first[2]);
    const float y1 = bScalarMultiply(first[2], second[0]), y2 = bScalarMultiply(second[2], first[0]);
    const float z1 = bScalarMultiply(first[0], second[1]), z2 = bScalarMultiply(second[0], first[1]);
    output[0] = bScalarAdd(x1, -x2);
    output[1] = bScalarAdd(y1, -y2);
    output[2] = bScalarAdd(z1, -z2);
}

static void normalizeThree(float value[3])
{
    // PAL11BD98..11BDC8 (repeated at 11BDE4): MUL.xyz, ADDy/ADDz,
    // RSQRT, ADDq from zero, then MULx with the source vector first.
    const float x = bScalarMultiply(value[0], value[0]);
    const float y = bScalarMultiply(value[1], value[1]);
    const float z = bScalarMultiply(value[2], value[2]);
    const float squared = bScalarAdd(bScalarAdd(x, y), z);
    const float reciprocal = bScalarAdd(0.0f, bScalarDivide(1.0f, bScalarSquareRoot(fabsf(squared))));
    for (int i = 0; i < 3; ++i) value[i] = bScalarMultiply(value[i], reciprocal);
}

float* motionMatrixOrthonormalize(float output[16], const float input[16])
{
    float first[3] = {input[0], input[1], input[2]};
    float second[3] = {input[4], input[5], input[6]};
    float third[3];
    normalizeThree(first);
    crossThree(third, first, second);
    normalizeThree(third);
    crossThree(second, third, first);
    for (int i = 0; i < 3; ++i) {
        output[i] = first[i];
        output[4 + i] = second[i];
        output[8 + i] = third[i];
    }
    output[3] = output[7] = output[11] = 0.0f;
    output[12] = output[13] = output[14] = 0.0f;
    output[15] = 1.0f;
    return output;
}

float* motionMatrixAngularDerivative(float output[16], const float angularVelocity[3],
                                     const float orientation[16])
{
    float matrix[16];
    memcpy(matrix, orientation, sizeof(matrix));
    const float x = angularVelocity[0], y = angularVelocity[1], z = angularVelocity[2];
    for (int column = 0; column < 3; ++column) {
        const int index = column * 4;
        const float xy = -z * matrix[index + 1];
        const float xz = y * matrix[index + 2];
        const float yx = z * matrix[index];
        const float yz = -x * matrix[index + 2];
        const float zx = -y * matrix[index];
        const float zy = x * matrix[index + 1];
        output[index] = xy + xz;
        output[index + 1] = yx + yz;
        output[index + 2] = zx + zy;
    }
    return output;
}

float* motionMatrixTransposeRotation(float output[16], const float input[16])
{
    vectorMatrixTranspose(output, input);
    output[3] = output[7] = output[11] = output[15] = 0.0f;
    output[12] = output[13] = output[14] = 0.0f;
    return output;
}

float* eTransposeMatrix(float output[16], const float input[16])
{
    float matrix[16];
    memcpy(matrix, input, sizeof(matrix));
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            output[column * 4 + row] = matrix[row * 4 + column];
    return output;
}

void carCinematicLookAt(float output[16],const float eye[4],const float target[4],const float up[4])
{
    float forward[4],negativeUp[4],right[4],vertical[4],basis[16],translation[16];
    for(int axis=0;axis<3;++axis){forward[axis]=target[axis]-eye[axis];negativeUp[axis]=-up[axis];}
    forward[3]=negativeUp[3]=0.0f;
    carCinematicNormalizeThree(forward);
    carCinematicCrossThree(right,negativeUp,forward);
    carCinematicCrossThree(vertical,forward,right);
    carCinematicNormalizeThree(right);carCinematicNormalizeThree(vertical);
    for(int axis=0;axis<3;++axis){
        basis[axis*4]=right[axis];basis[axis*4+1]=vertical[axis];basis[axis*4+2]=forward[axis];
        basis[axis*4+3]=0.0f;
    }
    basis[12]=basis[13]=basis[14]=0.0f;basis[15]=1.0f;
    rotationMatrixIdentity(translation);
    for(int axis=0;axis<3;++axis)translation[12+axis]=-eye[axis];
    vectorMatrixMultiply(output,basis,translation);
}

void carCameraInvertRigidTransform(float output[16], const float input[16])
{
    float rotation[16], translation[16];
    rotationMatrixIdentity(rotation);
    rotationMatrixIdentity(translation);
    for (int column = 0; column < 3; ++column)
        for (int row = 0; row < 3; ++row)
            rotation[column * 4 + row] = input[row * 4 + column];
    for (int axis = 0; axis < 3; ++axis) translation[12 + axis] = -input[12 + axis];
    vectorMatrixMultiply(output, rotation, translation);
}

float carCinematicLengthThree(const float value[4])
{
    const float x=value[0]*value[0],y=value[1]*value[1],z=value[2]*value[2];
    return sqrtf((x+y)+z);
}

void carCinematicNormalizeThree(float value[4])
{
    const float scale=finiteRatio(1.0f,carCinematicLengthThree(value));
    for(int axis=0;axis<3;++axis)value[axis]*=scale;
    value[3]=0.0f;
}

void carCinematicCrossThree(float output[4],const float first[4],const float second[4])
{
    const float x=first[1]*second[2],y=first[2]*second[0],z=first[0]*second[1];
    const float otherX=second[1]*first[2],otherY=second[2]*first[0],otherZ=second[0]*first[1];
    output[0]=x-otherX;output[1]=y-otherY;output[2]=z-otherZ;output[3]=0.0f;
}

int32_t carViewPointInPolygon(const float point[2],const CarViewRegionVertex* vertices,int32_t count)
{
    int32_t previous=count-1,inside=0;
    for (int32_t index=0;index<count;++index) {
        const CarViewRegionVertex* current=vertices+index;
        const CarViewRegionVertex* last=vertices+previous;
        if ((current->y<=point[1] && point[1]<last->y) ||
            (last->y<=point[1] && point[1]<current->y)) {
            const float delta=(last->x-current->x)*(point[1]-current->y);
            const float x=finiteRatio(delta,last->y-current->y)+current->x;
            if (point[0]<x) inside=!inside;
        }
        previous=index;
    }
    return inside;
}

void motionMatrixAxisAngle(float output[16], const float axis[3], uint16_t angle)
{
    const float cosine = vectorCosineAngle(angle);
    const float sine = rotationSineAngle(angle);
    const float x = axis[0], y = axis[1], z = axis[2];
    const float remainder = 1.0f - cosine;
    const float sineZ = sine * z, sineY = sine * y;
    const float scaledX = remainder * x;
    const float scaledZ = remainder * z;
    const float scaledY = remainder * y;
    const float xz = scaledX * z, zz = scaledZ * z;
    const float yz = scaledY * z, yy = scaledY * y;
    const float sineX = x * sine;
    const float xy = scaledX * y, xx = scaledX * x;
    output[0] = xx + cosine;
    output[1] = xy - sineZ;
    output[2] = xz + sineY;
    output[3] = 0.0f;
    output[4] = xy + sineZ;
    output[5] = yy + cosine;
    output[6] = yz - sineX;
    output[7] = 0.0f;
    output[8] = xz - sineY;
    output[9] = yz + sineX;
    output[10] = zz + cosine;
    output[11] = 0.0f;
    output[12] = output[13] = output[14] = 0.0f;
    output[15] = 1.0f;
}

float motionSineRadians(float radians)
{
    const float period = 6.2831854820251465f;
    while (period < radians) radians = bScalarAdd(radians, -period);
    while (radians < 0.0f) radians = bScalarAdd(radians, period);
    float sign = 1.0f;
    if (3.1415927410125732f <= radians) {
        radians = bScalarAdd(radians, -3.1415927410125732f);
        sign = -1.0f;
    }
    if (1.5707963705062866f <= radians)
        radians = bScalarAdd(3.1415927410125732f, -radians);
    const float squared = bScalarMultiply(radians, radians);
    const float cubed = bScalarMultiply(radians, squared);
    const float fifth = bScalarMultiply(cubed, squared);
    const float seventh = bScalarMultiply(fifth, squared);
    const float ninth = bScalarMultiply(seventh, squared);
    const float cubicTerm = bScalarMultiply(cubed, -0.16666656732559204f);
    const float fifthTerm = bScalarMultiply(fifth, 0.008333025500178337f);
    const float seventhTerm = bScalarMultiply(seventh, -0.00019807413627859205f);
    const float ninthTerm = bScalarMultiply(ninth, 0.000002601886990305502f);
    float result = bScalarAdd(radians, cubicTerm);
    result = bScalarAdd(result, fifthTerm);
    result = bScalarAdd(result, seventhTerm);
    result = bScalarAdd(result, ninthTerm);
    return bScalarMultiply(result, sign);
}

void motionMatrixRotateX(float output[16], const float input[16], uint16_t angle)
{
    float rotation[16], sine, cosine;
    motionAngleComponents(angle, &sine, &cosine);
    rotationMatrixIdentity(rotation);
    rotation[5] = cosine;
    rotation[6] = sine;
    rotation[9] = -sine;
    rotation[10] = cosine;
    vectorMatrixMultiply(output, rotation, input);
}

void motionMatrixRotateY(float output[16], const float input[16], uint16_t angle)
{
    float rotation[16], sine, cosine;
    motionAngleComponents(angle, &sine, &cosine);
    rotationMatrixIdentity(rotation);
    rotation[0] = cosine;
    rotation[2] = -sine;
    rotation[8] = sine;
    rotation[10] = cosine;
    vectorMatrixMultiply(output, rotation, input);
}

void motionMatrixRotateYaw(float output[16], const float input[16], uint16_t angle)
{
    float rotation[16];
    motionMatrixYaw(rotation, angle);
    vectorMatrixMultiply(output, rotation, input);
}

void motionAngleComponents(uint16_t angle, float* sine, float* cosine)
{
    const float radians = bScalarMultiply((float)angle, 0.0000958738019107841f);
    *cosine = motionSineRadians(bScalarAdd(radians, 1.5707964897155762f));
    *sine = motionSineRadians(radians);
}

float eMathIdentityMatrix[16];
float eMathZeroMatrix[16];

void eIdentity(float output[16])
{
    rotationMatrixIdentity(output);
}

void eZeroMatrix(float output[16])
{
    for (int i=0;i<16;++i)output[i]=0.0f;
}

void eMathInit(void)
{
    eIdentity(eMathIdentityMatrix);
    eZeroMatrix(eMathZeroMatrix);
}

void eMulMatrix(float output[16],const float first[16],const float second[16])
{
    vectorMatrixMultiply(output,second,first);
}

void eMulVector4(float output[4],const float matrix[16],const float input[4])
{
    vectorMatrixTransform(output,matrix,input);
}

void eNormalizedDirection(float output[4],const float first[4],const float second[4])
{
    const float x=second[0]-first[0],y=second[1]-first[1],z=second[2]-first[2];
    const float xSquared=x*x,ySquared=y*y;
    const float length=sqrtf(fabsf(xSquared+ySquared));
    const float scale=finiteRatio(1.0f,length);
    output[0]=x*scale;output[1]=y*scale;output[2]=z*scale;output[3]=0.0f;
}

static void transformPosition(float output[4],const float matrix[16],const float input[4])
{
    const float point[4]={input[0],input[1],input[2],1.0f};
    eMulVector4(output,matrix,point);
}

void eRotTransPers(float output[4],const float matrix[16],const float input[4])
{
    float clip[4];transformPosition(clip,matrix,input);
    const float scale=finiteRatio(1.0f,clip[3]);
    output[0]=clip[0]*scale;output[1]=clip[1]*scale;output[2]=clip[2]*scale;output[3]=clip[3];
}

uint32_t eGetClipFlags(const float input[4],const float matrix[16])
{
    float clip[4];transformPosition(clip,matrix,input);
    const float limit=fabsf(clip[3]);
    uint32_t flags=0;
    for (int axis=0;axis<3;++axis) {
        // Native homogeneous clipping preserves IEEE float values. The
        // source VU subnormal-flush behavior is not a PC arithmetic mode.
        const bool positive=clip[axis]>limit;
        const bool negative=clip[axis]<-limit;
        if (positive)flags|=1u<<(axis*2);
        if (negative)flags|=2u<<(axis*2);
    }
    return flags;
}


// The same writable source eUnitVector4 input is initialized by the original
// eMath static producer. Future writers borrow this object, not another unit.
float eUnitVector4[4]={0.0f,0.0f,0.0f,1.0f};

void eMulVector3(float output[4],const float matrix[16],const float input[3])
{
    const float point[4]={eUnitVector4[0]+input[0],eUnitVector4[1]+input[1],
        eUnitVector4[2]+input[2],eUnitVector4[3]};
    eMulVector4(output,matrix,point);
}

void motionMatrixYaw(float output[16], uint16_t angle)
{
    float sine, cosine;
    motionAngleComponents(angle, &sine, &cosine);
    rotationMatrixIdentity(output);
    output[0] = cosine;
    output[1] = sine;
    output[4] = -sine;
    output[5] = cosine;
}
