// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class bVoice {
public:
    // ELF 0x0027C5C0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0027c5c0.md.
    // Original Debug reference: bVoice::Release(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Release();
    // ELF 0x0027C5E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0027c5e8.md.
    // Original Debug reference: bVoice::Set(unsigned short, bSample *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Set();
    // ELF 0x0027C6F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0027c6f8.md.
    // Original Debug reference: bVoice::bVoice(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0027C6F8();
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
// Port view storage of original8-byte bVoice; no host ABI claim. E-0099-W40.
class bVoice:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Release(); //00274550
 void Set(std::uint16_t,std::uint32_t); //00274578
 std::uint32_t GetSample()const; //002745B8
 std::uint16_t GetId()const; //002745F0
};
class bVoiceMgr {
public:
 static void Initialize(::port::driving::Memory&); //00272C90
 static std::uint32_t GetVoice(::port::driving::Memory&,std::uint16_t,std::uint32_t); //00272D10
 static void ReleaseVoice(::port::driving::Memory&,std::uint32_t); //00272F28
};
}
