// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trackpath.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrackPathCoordinate::UpdatePosition() [0x002BB238]
#include "game_core/indep/inc/TrackPath.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x002086B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002086b8.md.
void TrackPathCoordinate::Sub_002086B8() {
    hp2::not_implemented("0x002086B8", "TrackPathCoordinate::Sub_002086B8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/src/quickspline.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace hp2::recovered {
// Debug00202B00/00202D58/00202F30; E-0048-D02; bounded first-position heading.
std::array<float,2> TrackPathManager::UpdatePosition(const port::track::Paths& data,const std::array<float,2>& position,float tolerance){
    auto distance=[&](int index){const auto& p=data.points.at(index).position;float x=p[0]-position[0],y=p[1]-position[1];return std::sqrt(x*x+y*y);};
    int selected=-1;float best=0;
    for(const auto& path:data.paths)for(int i=path.begin;i<path.end;++i){float d=distance(i);if(selected<0||d<best){selected=i;best=d;}}
    if(selected<0)throw std::runtime_error("Original track path has no starting direction");
    const auto& initial=data.points.at(selected);const auto& row=data.paths.at(initial.path);
    std::vector<int> adjacent{selected};
    if(selected==row.begin){if(row.spline.looping)adjacent.push_back(row.end-1);}else adjacent.push_back(selected-1);
    if(selected==row.end-1){if(row.spline.looping)adjacent.push_back(row.begin);}else adjacent.push_back(selected+1);
    int nearest=-1;for(int i:adjacent){float d=distance(i);if(nearest<0||d<best){nearest=i;best=d;}}
    selected=nearest;best=distance(selected);
    // The original scans exactly three neighbour numbers after same-path selection.
    for(unsigned n=0;n<3;++n){const int i=data.points.at(selected).neighbours[n];if(i>=0){float d=distance(i);if(d<best){selected=i;best=d;}}}
    const auto& point=data.points.at(selected);const auto& path=data.paths.at(point.path);
    const float param=QuickSpline::FindClosestParam(path.spline,position,float(selected-path.begin),tolerance);
    auto direction=QuickSpline::GetPoint(path.spline,param,1);
    const float length=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]);
    if(!std::isfinite(length)||length==0)throw std::runtime_error("Original starting path derivative is degenerate");
    direction[0]/=length;direction[1]/=length;return direction;
}
}

namespace hp2::recovered {
// ELF 0x00207C68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00207c68.md.
void TrackPathManager::FindClosestPoint() {
    hp2::not_implemented("0x00207C68", "TrackPathManager::FindClosestPoint (signature and full body unresolved)");
}

// ELF 0x00207ED0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00207ed0.md.
void TrackPathManager::FindClosestAdjacentPointOnSamePath() {
    hp2::not_implemented("0x00207ED0", "TrackPathManager::FindClosestAdjacentPointOnSamePath (signature and full body unresolved)");
}

// ELF 0x00208598; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00208598.md.
void TrackPathManager::CompareDistances() {
    hp2::not_implemented("0x00208598", "TrackPathManager::CompareDistances (signature and full body unresolved)");
}

// ELF 0x002085E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002085e8.md.
void TrackPathManager::ComparePercentDistances() {
    hp2::not_implemented("0x002085E8", "TrackPathManager::ComparePercentDistances (signature and full body unresolved)");
}
} // namespace hp2::recovered
