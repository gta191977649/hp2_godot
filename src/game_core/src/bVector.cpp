#include "game_core/src/bMath.hpp"
#include <cmath>
#include "game_core/port/driving/ee_scalar.hpp"
namespace hp2::recovered {
// Debug00251C70; TASK-0099 E-0099-W37. Exact zero-vector fallback.
std::array<float,2> bNormalize(const std::array<float,2>& v){const float length=std::sqrt(v[0]*v[0]+v[1]*v[1]);if(length==0)return {{1,0}};return {{v[0]/length,v[1]/length}};}
// Debug00251CC8; E-0099-W37. Division by requested length precedes zero test.
std::array<float,2> bNormalize(const std::array<float,2>& v,float length){const float scale=std::sqrt(v[0]*v[0]+v[1]*v[1])/length;if(scale==0)return {{1,0}};return {{v[0]/scale,v[1]/scale}};}
}

namespace hp2::recovered {
// Debug00251D28; E-0046-S03. Preserve sqrt/length then reciprocal grouping.
std::array<float,3> bNormalize(const std::array<float,3>&v,float length){
    float sum=v[0]*v[0];sum=sum+v[1]*v[1];sum=sum+v[2]*v[2];
    // E-0110-M01. Original251D4C/251D74 are scalar DIV.S, with finite
    // saturation/underflow semantics rather than IEEE NaN/Inf. Ordinary
    // arithmetic retains the established native projection and source order.
    const auto divide=[](float numerator,float denominator,const char* pc){
      if(!std::isfinite(numerator)||!std::isfinite(denominator))hp2::not_implemented(pc,"bNormalize DIV.S finite operand domain exceeded");
      if(denominator==0){if(!::port::driving::pc_sqrt_carrier_enabled())hp2::not_implemented(pc,"bNormalize DIV.S zero operand requires explicit PC numerical carrier scope");::port::diagnostics::warn(pc,"EE DIV.S zero denominator saturates; PC maximum7F7FFFFF differs from EE extended maximum7FFFFFFF","using the explicitly enabled finite PC carrier; exact encoding/flags remain unverified");return ::port::driving::ee_div_s_zero(numerator,denominator,pc);}
      const float result=numerator/denominator;
      if(std::isfinite(result)&&std::fpclassify(result)!=FP_SUBNORMAL)return result;
      if(!::port::driving::pc_sqrt_carrier_enabled())hp2::not_implemented(pc,"bNormalize DIV.S exceptional native result requires explicit PC numerical carrier scope");
      ::port::diagnostics::warn(pc,"EE DIV.S overflow saturates and exponent underflow flushes to signed zero","using the scoped finite PC carrier; exact EE low-bit rounding/flags remain unverified");
      return std::isfinite(result)?std::copysign(0.f,result):std::copysign(std::numeric_limits<float>::max(),result);
    };
    float divisor=divide(std::sqrt(sum),length,"0x00251D4C");
    if(divisor==0)return {{length,0,0}};
    divisor=divide(1.0f,divisor,"0x00251D74");return {{v[0]*divisor,v[1]*divisor,v[2]*divisor}};
}
}
