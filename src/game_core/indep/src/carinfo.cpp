#include "game_core/port/driving/projection_support.hpp"
#include "game_core/indep/inc/Table.hpp"
namespace port::driving {
void CalcIndexMultiplier(Memory& m,std::uint32_t s) {hp2::recovered::Table(m,s).CalcIndexMultiplier();}
void SetMinMax(Memory& m,std::uint32_t s,float lower,float upper) {hp2::recovered::Table(m,s).SetMinMax(lower,upper);}
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
void Table::SetMinMax(float lower,float upper) {
    m_.scalar(address_+8,lower);m_.scalar(address_+0xc,upper);CalcIndexMultiplier();
}
// Debug00187318, full listing E-0044-E01.
void Table::CalcIndexMultiplier() {
    const auto count=port::driving::detail_group_0018f808::signed_bits(m_.word(address_+4)-1);
    m_.scalar(address_+0x10,static_cast<float>(count)/(m_.scalar(address_+0xc)-m_.scalar(address_+8)));
}
}
