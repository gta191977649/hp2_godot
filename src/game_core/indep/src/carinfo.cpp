
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

#include "game_core/indep/src/carinfo.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001884B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001884b8.md.
void CarTypeInfo::GetNumAvailableSkins() {
    hp2::not_implemented("0x001884B8", "CarTypeInfo::GetNumAvailableSkins (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/Table.hpp"

namespace hp2::recovered {
// ELF 0x00189120; E-0042-C057; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00189120.md.
void Table::SetMinMax() {
    hp2::not_implemented("0x00189120", "Table::SetMinMax (signature and full body unresolved)");
}

// ELF 0x00189168; E-0042-C058; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00189168.md.
void Table::CalcIndexMultiplier() {
    hp2::not_implemented("0x00189168", "Table::CalcIndexMultiplier (signature and full body unresolved)");
}
} // namespace hp2::recovered
