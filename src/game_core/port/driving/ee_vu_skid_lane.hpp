#pragma once
// Port CPU-instruction carrier, NOT recovered game logic. TASK-0099 W71.
// Sony VU User's Manual6.0 (April2002) pp26..28, original PDF SHA256
// a858ef55cd2638accdfca612d5359eb6e48a48c7fd6792b18080d866e610fde5.
// This handles only exact extreme products at001A81DC's computed W lane.
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
namespace port::driving {
struct VuSkidExtremeProduct {
 std::uint32_t bits;
 bool overflow,sticky_overflow;
};
inline VuSkidExtremeProduct vu_skid_extreme_product_001a81dc(float input,double multiplier,bool prior_sticky=false){
 const double maximum=std::ldexp(2.-std::ldexp(1.,-23),128);
 if(!std::isfinite(input)||!std::isfinite(multiplier))hp2::not_implemented("0x001A81DC","Unsupported original VU skid product operands");
 // Binary32 operands (or the known24-bit VU MAX Q) multiply exactly in double.
 const double product=double(input)*multiplier;
 if(!std::isfinite(product)||std::fabs(product)<maximum){
  std::uint32_t input_bits;std::uint64_t multiplier_bits,product_bits;
  std::memcpy(&input_bits,&input,4);std::memcpy(&multiplier_bits,&multiplier,8);std::memcpy(&product_bits,&product,8);
  hp2::not_implemented("0x001A81DC",("Non-extreme extended VU skid W product remains unresolved; inputWord="+std::to_string(input_bits)+" multiplierDoubleWord="+std::to_string(multiplier_bits)+" productDoubleWord="+std::to_string(product_bits)).c_str());
 }
 const bool overflow=std::fabs(product)>maximum;
 return {std::signbit(product)?0xffffffffu:0x7fffffffu,overflow,prior_sticky||overflow};
}
}
