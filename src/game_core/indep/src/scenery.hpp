// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/src/bMath.hpp"
namespace port::track {struct Scene;struct Zone;}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
// TASK-0051 host-value overloads; original free functions in scenery.cpp.
bool IsPointInPoly(const std::array<float,3>&,const port::track::Zone&);
const port::track::Zone* FindVisibleZone(const port::track::Scene&,
    const std::array<float,3>&,std::vector<std::size_t>& original_list_order);
// Debug0014F9F8; E-0048-A05. Original free function; explicit host math-image
// argument replaces original process-global lookup tables. No Godot dependency.
port::driving::SolverMatrix CreateFacadeMatrix(port::driving::Memory& original_math,
    const std::array<float,3>& object_position,const std::array<float,3>& camera_position);
class VisibleZone {
public:
    // ELF 0x0014F340; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014f340.md.
    // Original Debug reference: VisibleZone::FindIndexTo(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FindIndexTo();
};
} // namespace hp2::recovered
