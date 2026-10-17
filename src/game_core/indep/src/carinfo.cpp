
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x00189168; E-0042-C058; E-0042-S001; exact Debug TU ..\indep\src\carinfo.cpp
namespace port::driving {
void CalcIndexMultiplier(Memory& m,std::uint32_t s) {
    using namespace detail_group_0018f808;
    const float numerator=static_cast<float>(signed_bits(m.word(s+4)-1));
    const float denominator=m.scalar(s+0xc)-m.scalar(s+8);
    m.scalar(s+0x10,numerator/denominator);
}
}

// Retail 0x00189120; E-0042-C057; E-0042-S001; exact Debug TU ..\indep\src\carinfo.cpp
namespace port::driving {
void SetMinMax(Memory& m,std::uint32_t s,float f12,float f13) {
    using namespace detail_group_0018f808;
    m.scalar(s+8,f12);m.scalar(s+0xc,f13);CalcIndexMultiplier(m,s);
}
}
