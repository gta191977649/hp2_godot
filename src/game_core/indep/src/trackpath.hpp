// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/track/spline.hpp"
#include "game_core/port/track/route_data.hpp"

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

namespace hp2::debug {
// Decoded Debug state, not an original ABI; E-0099-W08.
struct TrackPathCoordinate {
    static void DoSnapshot(::port::driving::Memory&,std::uint32_t,std::uint32_t snapshot,const ::port::track::RoutePaths&); //00203650, W74.
    // E-0099-W26. Explicit shared-Memory overloads retain the original pointer
    // representation; the older decoded fields below remain port projections.
    static void Sub_002035A0(::port::driving::Memory&,std::uint32_t);
    static void Sub_00203630(::port::driving::Memory&,std::uint32_t,int in_chrg); // Original empty non-owning destructor.
    static void UpdatePosition(::port::driving::Memory&,std::uint32_t,const ::port::track::RoutePaths&,const std::array<float,2>&);
    static void ResetPosition(::port::driving::Memory&,std::uint32_t,const ::port::track::RoutePaths&,const std::array<float,2>&,float distance); //00203720,W122.
    // Debug00203708 ->00203238, E-0099-W34. Distance/lateral overload.
    // The original path-search branch can consume an uninitialized stack
    // direction; that branch throws rather than inventing those bits.
    static void UpdatePosition(::port::driving::Memory&,std::uint32_t,const ::port::track::RoutePaths&,float distance,float lateral,int path_number=0);
    std::array<float,2> Position{{0,0}},PathDirection{{1,0}};
    int pTrackPoint=-1; // Port index replacing the original pointer; -1 is null.
    float SplineParam=0,RatioAroundTrack=0,DistanceAroundTrack=0;
    std::array<std::uint32_t,2> pCaffeineObj{};
    // Debug002035A0/002035D8; E-0099-W08.
    TrackPathCoordinate()=default;
    TrackPathCoordinate(const port::track::RoutePaths&,const std::array<float,2>&,float tolerance);
    // Debug002036E0 ->00202F30; E-0099-W08.
    void UpdatePosition(const port::track::RoutePaths&,const std::array<float,2>&,float tolerance);
};
class TrackPathManager {
public:
    static std::uint32_t FindClosestPoint(::port::driving::Memory&,const ::port::track::RoutePaths&,const std::array<float,2>&,float distance); //00202BE0,W122.
    static void ResetPosition(::port::driving::Memory&,std::uint32_t,const ::port::track::RoutePaths&,const std::array<float,2>&,float distance); //00202ED0,W122.
    // Debug00203480, E-0099-W17. Signed shortest distance with original ties.
    static float CompareDistances(float track_length,float,float);
    // Debug002034D0; E-0099-W34. Original percentage wrap and +/-50 ties.
    static float ComparePercentDistances(float,float);
    // Debug00202B00/00202D58; E-0099-W08. Returns a port point index.
    static int FindClosestPoint(const port::track::RoutePaths&,const std::array<float,2>&);
    static int FindClosestAdjacentPointOnSamePath(const port::track::RoutePaths&,
        const std::array<float,2>&,int point);
    // Debug00202780 -> QuickSpline00237940,16 subdivisions; E-0099-W08.
    static void Init(port::track::RoutePaths&);
};
}
