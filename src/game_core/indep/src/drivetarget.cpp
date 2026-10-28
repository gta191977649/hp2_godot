// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/drivetarget.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0021BDC8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021bdc8.md.
void DriveTarget::GetLastIndexToCheckForDrivability() {
    hp2::not_implemented("0x0021BDC8", "DriveTarget::GetLastIndexToCheckForDrivability (signature and full body unresolved)");
}

// ELF 0x0021DAC8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021dac8.md.
void DriveTarget::RecalculateLength() {
    hp2::not_implemented("0x0021DAC8", "DriveTarget::RecalculateLength (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include <cmath>
namespace hp2::debug {
// Debug00216528, E-0099-W02. Original zero-fill is followed by Clear, yielding last-index -1.
DriveTarget::DriveTarget(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){
    for(unsigned i=0;i<0x210;++i)memory_.byte(address_+i,0);
    Clear(0,0);
}
// Debug002167C0, E-0099-W02. Empty/zero/negative-distance branches; positive target interpolation remains explicit.
void DriveTarget::Clear(std::uint32_t car,float distance){
    (void)car;
    const int points=memory_.signed_byte(address_);
    if(points!=0){
        const bool nonzero=memory_.scalar(0x325f40)<std::abs(distance);
        if(nonzero && 0.f<=distance && points>0)
            hp2::not_implemented("0x002167C0","original positive-distance target shift/interpolation not translated");
        memory_.byte(address_,0);
    }
    memory_.scalar(address_+0x200,0);memory_.word(address_+0x1fc,0);
    memory_.byte(address_+0x204,255);memory_.word(address_+0x1f8,0);
}
}
