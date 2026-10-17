// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/Car.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): Car::SetMovementMode() [0x002AE518]
// Diagnostic text (not a callable signature): Car::Move() [0x002AE5A0]
// Diagnostic text (not a callable signature): Car::TriggerCarTerrainEffects() [0x002AE690]
namespace hp2::recovered {
class Car {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00133070; E-0042-U013; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00133070.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void PleaseResetMeLater();
    // ELF 0x00133130; E-0042-U014; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00133130.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsBlinking();
    // ELF 0x00135E18; E-0042-C008; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00135e18.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ShouldVisualEffectsBeGenerated();
    // ELF 0x00135EB8; E-0042-C009; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00135eb8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetPostCollisionYawControlBoost();
};
} // namespace hp2::recovered
