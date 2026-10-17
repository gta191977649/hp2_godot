// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): RealController::Think() [0x002BE128]
// Diagnostic text (not a callable signature): RealController::CalculateLateralPos [0x002BE140]
namespace hp2::recovered {
class RealController {
public:
    // ELF 0x0023A3D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a3d8.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0023A3D8();
    // ELF 0x0023A4D8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a4d8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0023A4D8();
    // ELF 0x0023A560; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a560.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: RealController::DoSnapshot(ReplaySnapshot *); E-0041-V01; retail ABI unconfirmed.
    void DoSnapshot();
    // ELF 0x0023AA50; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023aa50.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: RealController::Think(float); E-0041-V01; retail ABI unconfirmed.
    void Think();
};
} // namespace hp2::recovered
