// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/track/spline.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class QuickSpline {
public:
    // Debug 00237268/00237448/002372E8/00237340, E-0048-D02.
    // Bounded XY OVERHAUSER projection for original track starting direction.
    // Storage/return ABI are port-owned; no original object layout is asserted.
    static std::array<float,2> GetPoint(const port::track::Spline&,float param,int deriv);
    // Debug 00237748, E-0048-D02. Original iterative closest-param arithmetic.
    static float FindClosestParam(const port::track::Spline&,const std::array<float,2>&,float initial,float tolerance);
    // Debug 002372E8 and 00237268, E-0048-D02; host storage projections.
    static float ClampParam(const port::track::Spline&,float);
    static std::array<float,2> EvaluateSpline(const std::array<std::array<float,2>,4>&,float,int);
    // ELF 0x0023D368; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023d368.md.
    // Original Debug reference: QuickSpline::FixupEndpoints(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FixupEndpoints();
    // ELF 0x0023D600; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023d600.md.
    // Original Debug reference: QuickSpline::GenerateExtrapolatedControlPoints(bVector4 *, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GenerateExtrapolatedControlPoints();
    // ELF 0x0023D868; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023d868.md.
    // Original Debug reference: QuickSpline::FindClosestControlPoint(bVector3 const &, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FindClosestControlPoint();
};
} // namespace hp2::recovered
