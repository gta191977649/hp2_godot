#pragma once
// TASK-0044 canonical Debug free-function host signatures. Companion header is port-side;
// original implementation TUs are src/bMath.cpp, src/bMatrix.cpp and src/ecstasy/emathe.cpp.
#include "game_core/port/driving/projections.hpp"
namespace hp2::recovered {
using port::driving::Memory;
using port::driving::SolverMatrix;
using port::driving::SolverVector;
using port::driving::SolverMathContext;
using port::driving::native::Matrix;
std::uint32_t bRandom(Memory& m,std::uint32_t state,std::uint32_t divisor);
std::uint32_t bRandom(Memory& m,int range);
void bSetRandomSeed(Memory&,std::uint32_t value,std::uint32_t state);
float bRandom(Memory& m,std::uint32_t state,float value);
float bRandom(Memory& m,float value);
float bSin(Memory&m,std::uint16_t a);
float bSin(Memory&m,float f);
float bCos(Memory&m,std::uint16_t angle);
std::uint16_t bASin(Memory&m,float f);
std::int16_t bATan(Memory&m,float x,float y);
std::int32_t bDiv(std::int32_t numerator,std::int32_t denominator);
std::uint16_t bFixATan(Memory&m,std::int32_t value);
std::int16_t bFixATan(Memory&m,std::int32_t x,std::int32_t y);
std::array<float,16> bIdentity();
std::array<float,16> bInvertMatrix(const std::array<float,16>& a);
void bTransposeMatrix(SolverMatrix& out, const SolverMatrix& in);
void bMulMatrix(SolverVector& out, const SolverMatrix& matrix,
                 const SolverVector& in, const SolverMathContext& context);
Matrix bMulMatrix(const Matrix& a,const Matrix& b);
void eOrthonormalizeMatrix(Memory& m,std::uint32_t a);
void eMulMatrix(SolverMatrix&out,const SolverMatrix&a,const SolverMatrix&b);

float GetRoadNoiseGridValue(Memory& m,std::int32_t x,std::int32_t y);
float GetRoadNoise(Memory& m,std::uint32_t point,std::int32_t material);
void Sub_0019B358(Memory& m);
void Sub_00139510(Memory& m);
float eSin(Memory&m,float f12);
void Sub_001A2FC0(Memory&m);
void Sub_00254350(Memory&m);
void eRotateZ(Memory&,SolverMatrix& destination,const SolverMatrix& input,std::uint16_t angle);
void eRotateY(Memory&,SolverMatrix& destination,const SolverMatrix& input,std::uint16_t angle);
void eRotateX(Memory&,SolverMatrix& destination,const SolverMatrix& input,std::uint16_t angle);
void eTranslate(SolverMatrix& destination,const SolverMatrix& input,const std::array<float,3>&);
}

namespace hp2::recovered {
// Debug00251D28; E-0046-S03; original bWare/src/bMath.cpp. Host vector binding.
std::array<float,3> bNormalize(const std::array<float,3>&,float length);
}
