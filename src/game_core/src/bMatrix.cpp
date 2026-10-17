
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x0025AC68; E-0041-M01; E-0042-S001; exact Debug TU src\bMatrix.cpp
namespace port::driving::init {
std::array<float,16> bIdentity(){
    return {{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}};
}
}

// Retail 0x0025AC98; E-0041-M01; E-0042-S001; exact Debug TU src\bMatrix.cpp
namespace port::driving::init {
std::array<float,16> bInvertMatrix(const std::array<float,16>& a){
    // Arithmetic order follows 0025ACC0..0025AE00; do not replace with a library inverse.
    const float c12=a[4]*a[10]-a[6]*a[8];
    const float c3=a[5]*a[10]-a[6]*a[9];
    const float c19=a[4]*a[9]-a[5]*a[8];
    const float reciprocal=1.0f/((a[0]*c3-a[1]*c12)+a[2]*c19);
    const float negative=-reciprocal;
    std::array<float,16> b;
    b[15]=1; b[3]=0; b[7]=0; b[11]=0;
    b[0]=reciprocal*c3;
    b[2]=reciprocal*(a[1]*a[6]-a[2]*a[5]);
    b[4]=negative*c12;
    b[6]=negative*(a[0]*a[6]-a[2]*a[4]);
    b[1]=negative*(a[1]*a[10]-a[2]*a[9]);
    b[5]=reciprocal*(a[0]*a[10]-a[2]*a[8]);
    b[10]=reciprocal*(a[0]*a[5]-a[1]*a[4]);
    b[8]=reciprocal*c19;
    b[9]=negative*(a[0]*a[9]-a[1]*a[8]);
    b[14]=-((a[12]*b[2]+a[13]*b[6])+a[14]*b[10]);
    b[12]=-((a[12]*b[0]+a[13]*b[4])+a[14]*b[8]);
    b[13]=-((a[12]*b[1]+a[13]*b[5])+a[14]*b[9]);
    return b;
}
}

// Retail 0x0025AF20; E-0042-U194; E-0042-S001; exact Debug TU src\bMatrix.cpp
namespace port::driving {
void bTransposeMatrix(SolverMatrix& out, const SolverMatrix& in) {
    using namespace detail_group_00138f38;
    const SolverMatrix copy = in;
    for (unsigned r = 0; r < 4; ++r)
        for (unsigned c = 0; c < 4; ++c) out[r * 4 + c] = copy[c * 4 + r];
}
}

// Retail 0x0025AED0; E-0042-U193; E-0042-S001; exact Debug TU src\bMatrix.cpp
namespace port::driving {
void bMulMatrix(SolverVector& out, const SolverMatrix& matrix,
                 const SolverVector& in, const SolverMathContext& context) {
    using namespace detail_group_00138f38;
    SolverVector v = context.Field_00320A60;
    for (unsigned i = 0; i < 3; ++i) v[i] = v[i] + in[i];
    SolverVector result{};
    for (unsigned i = 0; i < 4; ++i) {
        float acc = matrix[i] * v[0];
        const float y = matrix[4 + i] * v[1];
        acc = acc + y;
        const float z = matrix[8 + i] * v[2];
        acc = acc + z;
        const float w = matrix[12 + i] * v[3];
        result[i] = acc + w;
    }
    out = result;
}
}

// Retail 0x0025AE18; E-0041-M01; E-0042-S001; exact Debug TU src\bMatrix.cpp
namespace port::driving::native {
Matrix bMulMatrix(const Matrix& a,const Matrix& b) {
    using namespace detail_group_0019d4d0;
    Matrix result;
    for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r) {
        float v=a[r]*b[c*4];const float y=a[4+r]*b[c*4+1];v=v+y;
        const float z=a[8+r]*b[c*4+2];v=v+z;const float w=a[12+r]*b[c*4+3];result[c*4+r]=v+w;
    } return result;
}
}
