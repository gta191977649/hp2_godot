// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace port::frontend {
// Host disk adapter for the recovered successful Autosave path. This is not a
// PS2 memory-card device/status model. Timing/popup states retain Retail values.
struct AutosaveState {
    int phase=1;
    double elapsed=0;
    std::uint32_t text_id=0x24de5;
    std::string error;
    int logical_slot=0; // Port private-device slot, formatted as1 in original messages.
};
}
namespace hp2::frontend {
// ELF00246138/00246B80/00246B10/00247C68; E-0092-J07. The host supplies an
// available private save destination and performs I/O at the00245B00 seam.
bool Sub_00246138(::port::frontend::AutosaveState&,double delta,
    const std::function<std::string()>& store);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class MemcardLoader {
public:
    // ELF 0x00246BD8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00246bd8.md.
    // Original Debug reference: MemcardLoader::MemcardLoader(int, MemcardLoaderModes); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00246BD8();
};
} // namespace hp2::recovered
