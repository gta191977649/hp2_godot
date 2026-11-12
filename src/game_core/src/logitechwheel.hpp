// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class WheelDevice {
public:
    // ELF 0x0012B3B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0012b3b8.md.
    // Original Debug reference: WheelDevice::WheelDevice(int); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0012B3B8();
    // ELF 0x0012BC68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0012bc68.md.
    // Original Debug reference: WheelDevice::RecalculateDeviceNumber(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void RecalculateDeviceNumber();
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
void Sub_0012B510(::port::driving::Memory&,std::uint32_t,int); //WheelDevice constructor.
void Sub_0012CA50(::port::driving::Memory&); //Original two-device CRT initializer.
// E-0099-W66: bReset is never read by either recovered machine-code body.
// The port projection omits that dead operand, including unwritten caller values.
void PortSetForceConditionData(::port::driving::Memory&,int,int,float,float,float,float,float,float); //0012C3E0.
void PortSetForcePeriodicData(::port::driving::Memory&,int,int,float,float,float,float,float,float,float,float,float); //0012C540.
void PortPlayShake(::port::driving::Memory&,int,float); //0012C918.
void PortStopForce(::port::driving::Memory&,int); //0012C398,E-0112-P02.
void PortSetOverallForceGain(::port::driving::Memory&,int,float); //0012C980.
int GetWheelId(::port::driving::Memory&,int); //0012C2B8.
void PortPlayCrash(::port::driving::Memory&,int,int,float,float); //0012C880.
}
