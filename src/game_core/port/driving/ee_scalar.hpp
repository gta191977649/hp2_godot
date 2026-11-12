#pragma once
// Port-side EE instruction projection, not an original game helper or parameter.
// TASK-0038 E-0038-E12; legacy bounded PC numerical carrier for finite operands
// and a signed-zero denominator. It returns host finite maximum, not an exact
// EE result encoding. Sony v6 user manual p163 specifies EE Fmax7FFFFFFF;
// that extended finite encoding is not native IEEE binary32. E-0099-W54 records
// this counterevidence; general EE division/VU bit equivalence is unresolved.
// See reverse/driving/task0038-ee-zero-divide.md for instruction sites and ISA reference.
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
namespace port::driving {
// TASK-0110 PC numerical model. Source SQRT.S semantics are defined by Sony:
// negative normal operands use the positive magnitude, with I/SI set. Strict
// diagnostics retain the research stop for unestablished EE low-bit rounding.
// Playable World explicitly opts into the existing positive-normal PC sqrt
// carrier for negative normals too; this is not bit-exact EE verification.
inline bool& pc_sqrt_carrier_enabled(){static thread_local bool enabled=false;return enabled;}
class PcSqrtCarrierScope {
 bool previous_;
public:
 explicit PcSqrtCarrierScope(bool enabled):previous_(pc_sqrt_carrier_enabled()){pc_sqrt_carrier_enabled()=enabled;}
 ~PcSqrtCarrierScope(){pc_sqrt_carrier_enabled()=previous_;}
 PcSqrtCarrierScope(const PcSqrtCarrierScope&)=delete;
 PcSqrtCarrierScope& operator=(const PcSqrtCarrierScope&)=delete;
};
inline float ee_div_s_zero(float numerator,float denominator,const char* instruction_address) {
    if(!std::isfinite(numerator) || !std::isfinite(denominator) || denominator!=0.0f)
        hp2::not_implemented(instruction_address,"Unsupported operand outside port-side zero-denominator EE DIV.S projection");
    std::uint32_t a,b;std::memcpy(&a,&numerator,4);std::memcpy(&b,&denominator,4);
    const std::uint32_t bits=((a^b)&0x80000000u)|0x7f7fffffu;
    float value;std::memcpy(&value,&bits,4);return value;
}
// Port-only instruction carrier for Debug00128510: VRSQRT with numerator1
// and zero sum-of-squares. E-0098-P03; exception flags/general VU remain outside.
// Shares the legacy host-maximum numerical carrier above. Existing bounded
// consumer tests do not establish an EE/VU result encoding or hardware flags.
inline float ee_vrsqrt_one_zero(float squared,const char* instruction_address){
    return ee_div_s_zero(1.f,squared,instruction_address);
}
// TASK-0110 E-0110-D01. Sony MUL.S p372 saturates exponent overflow and
// sets O/SO, without a floating-point trap. This scoped PC carrier uses the
// existing IEEE finite maximum rather than EE extended MAX7FFFFFFF; it is
// not bit-exact EE arithmetic or a global FCR31 model. Double precision holds
// the exact product of two finite binary32 significands for native range
// detection. Crossing the IEEE limit alone does not prove EE exponent overflow.
inline float ee_mul_s_finite(float left,float right,const char* instruction_address){
 if(!std::isfinite(left)||!std::isfinite(right))hp2::not_implemented(instruction_address,"EE MUL.S carrier requires finite native operands");
 const double product=static_cast<double>(left)*static_cast<double>(right);
 if(std::fabs(product)>static_cast<double>(std::numeric_limits<float>::max())){
  if(!pc_sqrt_carrier_enabled())hp2::not_implemented(instruction_address,"Sony defines MUL.S overflow saturation; exact EE extended-MAX arithmetic remains outside strict native projection");
  ::port::diagnostics::warn(instruction_address,"MUL.S product exceeds finite PC range; PC maximum7F7FFFFF differs from EE extended finite maximum7FFFFFFF","using the explicitly enabled finite PC saturation carrier; exact EE encoding, low-bit rounding and FCR31 flags remain unverified");
  return std::copysign(std::numeric_limits<float>::max(),static_cast<float>(std::signbit(left)!=std::signbit(right)?-1:1));
 }
 return left*right;
}
// E-0110-C01. Scoped native-range carriers for the original VU ADD/SUB
// instruction slices. EE/VU extended finite encodings remain outside this
// host float carrier; ordinary native results preserve their original order.
inline float ee_vadd_sub_finite(float left,float right,bool subtract,const char* instruction_address){
 if(!std::isfinite(left)||!std::isfinite(right))hp2::not_implemented(instruction_address,"VU ADD/SUB carrier requires finite native operands");
 const double result=subtract?double(left)-double(right):double(left)+double(right);
 if(std::fabs(result)>double(std::numeric_limits<float>::max())){
  if(!pc_sqrt_carrier_enabled())hp2::not_implemented(instruction_address,"VU ADD/SUB exceeds strict native range; extended finite arithmetic is unresolved");
  ::port::diagnostics::warn(instruction_address,"ADD/SUB result exceeds finite PC range; native maximum7F7FFFFF differs from EE/VU extended finite maximum7FFFFFFF","using the explicitly enabled signed finite PC projection; exact extended encoding, rounding and flags remain unverified");
  return std::copysign(std::numeric_limits<float>::max(),std::signbit(result)?-1.f:1.f);
 }
 return subtract?left-right:left+right;
}
inline float ee_vadd_finite(float left,float right,const char* instruction_address){return ee_vadd_sub_finite(left,right,false,instruction_address);}
inline float ee_vsub_finite(float left,float right,const char* instruction_address){return ee_vadd_sub_finite(left,right,true,instruction_address);}
// R01: scalar ADD.S has the same bounded PC range projection, without
// asserting the VU and scalar instruction flags or low-bit rounding coincide.
inline float ee_add_s_finite(float left,float right,const char* instruction_address){return ee_vadd_sub_finite(left,right,false,instruction_address);}
inline float ee_sub_s_finite(float left,float right,const char* instruction_address){return ee_vadd_sub_finite(left,right,true,instruction_address);}
// E-0110-RM01. Audited VU accumulator carrier (Sony VU v6 p42).
// A true multiplication exponent overflow dominates ACC, including opposite
// signed MAX. Keep that tag across subsequent ordinary products. The native
// maximum remains a lossy PC projection of VU extended range, not bit parity.
struct EeVuAccumulatorCarrier { float value; bool multiplication_overflow; };
inline EeVuAccumulatorCarrier ee_vmula_finite(float left,float right,const char* pc){
 if(!std::isfinite(left)||!std::isfinite(right))hp2::not_implemented(pc,"VU accumulator carrier requires finite native operands");
 const double product=double(left)*double(right),native_max=double(std::numeric_limits<float>::max());
 const double vu_max=std::ldexp(2.0-std::ldexp(1.0,-23),128);
 if(std::fabs(product)>native_max){
  if(!pc_sqrt_carrier_enabled())hp2::not_implemented(pc,"VU product exceeds strict native range");
  const bool overflow=std::fabs(product)>vu_max;
  ::port::diagnostics::warn(pc,overflow?"VU multiplication exceeds original extended exponent range; Sony defines signed MAX and multiplication-overflow dominance":"VU multiplication exceeds native range while remaining extended finite","using the enabled finite PC maximum projection; VU extended encoding, low bits and flags remain unverified");
  return {std::copysign(std::numeric_limits<float>::max(),std::signbit(product)?-1.f:1.f),overflow};
 }
 return {left*right,false};
}
// E-0100-MP01: standalone VMUL destination, with no persistent ACC tag.
// Uses the same explicit bounded PC product projection as the VU accumulator.
inline float ee_vmul_finite(float left,float right,const char* pc){return ee_vmula_finite(left,right,pc).value;}
inline EeVuAccumulatorCarrier ee_vmadda_finite(EeVuAccumulatorCarrier accumulator,float left,float right,const char* pc){
 if(!std::isfinite(accumulator.value))hp2::not_implemented(pc,"VU accumulator carrier rejects nonfinite ACC");
 const auto product=ee_vmula_finite(left,right,pc);
 if(product.multiplication_overflow)return product;
 if(accumulator.multiplication_overflow)return accumulator;
 return {ee_vadd_finite(accumulator.value,product.value,pc),false};
}
// Sony VU v6 p43: MSUB reverses the overflowing product's sign.
inline EeVuAccumulatorCarrier ee_vmsuba_finite(EeVuAccumulatorCarrier accumulator,float left,float right,const char* pc){
 if(!std::isfinite(accumulator.value))hp2::not_implemented(pc,"VU accumulator carrier rejects nonfinite ACC");
 const auto product=ee_vmula_finite(left,right,pc);
 if(product.multiplication_overflow)return {-product.value,true};
 if(accumulator.multiplication_overflow)return accumulator;
 return {ee_vsub_finite(accumulator.value,product.value,pc),false};
}
// E-0110-WP01. Sony instruction manual v6 p356 defines CVT.W.S truncation
// and signed word saturation above biased exponent9D. Unlike the native
// floating maximum projection, these finite-input integer result bits are
// directly representable. General extended operand encodings/FCR31 remain out
// of scope. Call only audited sites; the generic cvt_w_s guard stays strict.
inline std::int32_t ee_cvt_w_s_finite(float operand,const char* instruction_address){
 if(!std::isfinite(operand))hp2::not_implemented(instruction_address,"EE CVT.W.S audited finite operand domain exceeded");
 if(static_cast<double>(operand)>=2147483648.0||static_cast<double>(operand)<-2147483648.0){
  ::port::diagnostics::warn(instruction_address,"source CVT.W.S finite out-of-range conversion clamps to the signed 32-bit word limit","using the Sony-defined integer result at this audited instruction; general extended operands and FCR31 effects remain outside this carrier");
  return std::signbit(operand)?std::numeric_limits<std::int32_t>::min():std::numeric_limits<std::int32_t>::max();
 }
 return static_cast<std::int32_t>(operand);
}
// TASK-0099 E-0099-W54. User-authorized Sony original CPU-only supplemental
// evidence: April2002 instruction manualv6 p376, core user manualv6 pp158-159,
// 164-165; reverse/task0099/ee-sqrt.md. Not a complete FCR31/EE emulator.
struct EeSqrtSResult {
 float value;
 bool invalid; // I cause, replaced on this operation
 bool divide_by_zero; // D cause is cleared by SQRT.S
 bool sticky_invalid; // SI is accumulated, never cleared by this instruction
};
inline EeSqrtSResult ee_sqrt_s(float operand,bool prior_sticky_invalid,const char* instruction_address){
 std::uint32_t bits;std::memcpy(&bits,&operand,4);const auto magnitude_bits=bits&0x7fffffffu,exponent=magnitude_bits&0x7f800000u;
 if((exponent==0&&magnitude_bits!=0)||exponent==0x7f800000u)hp2::not_implemented(instruction_address,"EE SQRT.S operand encoding outside scoped normal/signed-zero projection");
 const bool invalid=operand<0;const float magnitude=invalid?-operand:operand;const float result=std::sqrt(magnitude);
 // Sony specifies truncating FPU arithmetic whose low bit can differ even
 // from IEEE rounding-to-zero. Exact roots need no unproven rounding rule.
 if(invalid&&static_cast<double>(result)*static_cast<double>(result)!=static_cast<double>(magnitude)){
  if(!pc_sqrt_carrier_enabled())hp2::not_implemented(instruction_address,"Sony defines negative SQRT.S magnitude/sign/flags; nonexact EE root rounding remains unresolved");
  ::port::diagnostics::warn(instruction_address,"EE SQRT.S negative-normal magnitude/flags are defined; exact low-bit rounding is not established","using the existing positive-normal PC sqrt numerical carrier for the magnitude; strict diagnostic mode retains the precision guard");
 }
 return {result,invalid,false,prior_sticky_invalid||invalid};
}

}
