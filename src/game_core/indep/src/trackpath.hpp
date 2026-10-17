// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/track/spline.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class TrackPathManager {
public:
    // Debug00202B00/00202D58/00202F30; E-0048-D02. First-position XY heading
    // projection only; path distance, ratio, reverse direction and persistence excluded.
    static std::array<float,2> UpdatePosition(const port::track::Paths&,const std::array<float,2>&,float tolerance);
    // ELF 0x00207C68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00207c68.md.
    // Original Debug reference: TrackPathManager::FindClosestPoint(bVector2 const *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FindClosestPoint();
    // ELF 0x00207ED0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00207ed0.md.
    // Original Debug reference: TrackPathManager::FindClosestAdjacentPointOnSamePath(bVector2 const *, TrackPathPoint *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FindClosestAdjacentPointOnSamePath();
    // ELF 0x00208598; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00208598.md.
    // Original Debug reference: TrackPathManager::CompareDistances(float, float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void CompareDistances();
    // ELF 0x002085E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002085e8.md.
    // Original Debug reference: TrackPathManager::ComparePercentDistances(float, float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ComparePercentDistances();
};
} // namespace hp2::recovered
