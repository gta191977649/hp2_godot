// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/ComponentPhysics.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): PhysicsSystem::IntegrateEuler() [0x002B5328]
// Diagnostic text (not a callable signature): PhysicsSystem::IntegrateEuler [0x002B5348]
// Diagnostic text (not a callable signature): PhysicsSystem::GetDerivative() [0x002B5380]
namespace hp2::recovered {
class PhysicsSystem {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x0019C808; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0019c808.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ComputeAuxiliaries();
    // ELF 0x0019C8F8; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0019c8f8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ResolveForces();

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x0019C170; E-0042-C089; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019c170.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0019C170();
    // ELF 0x0019C238; E-0042-C090; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019c238.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void AddComponent();
    // ELF 0x0019C290; E-0042-C091; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019c290.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IntegrateEuler();
    // ELF 0x0019C670; E-0042-C092; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019c670.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetDerivative();

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x0019C190; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019c190.md.
    // Original Debug reference: PhysicsSystem::~PhysicsSystem(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0019C190();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring class with adopted retail correspondence; evidence: E-0042-S001.
// Placement: TASK-0042 E-0042-S001; exact Debug ComponentPhysics.hpp definition marker for PhysicsVehicle destructor emitted in physicscar.cpp; no native inheritance or ABI assertion.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class PhysicsVehicle {
public:
    // ELF 0x00139678; E-0042-U030; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00139678.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00139678();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class PhysicsComponent {
public:
    // ELF 0x0019C910; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019c910.md.
    // Original Debug reference: PhysicsComponent::ResolveForces(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ResolveForces();
};
} // namespace hp2::recovered
