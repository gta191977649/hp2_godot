
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x00152070; E-0042-C021; E-0042-S001; exact Debug TU ..\indep\src\track.cpp
namespace port::driving {
float PlaneContact::GetElevation(Memory& m,std::uint32_t,std::uint32_t point,
    std::uint32_t material,std::uint32_t plane,std::uint32_t found) {
    const auto p=m.vector(point);
    for(float v:p)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite synthetic plane query");
    if(material)m.word(material,1);
    if(plane)m.vector(plane,{{0,0,1,height}});
    if(found)m.word(found,1);
    return height;
}
}

#include "game_core/indep/src/track.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00152070; E-0042-C021; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00152070.md.
void TopologyCoordinate::GetElevation() {
    hp2::not_implemented("0x00152070", "TopologyCoordinate::GetElevation (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00152B10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152b10.md.
void TopologyTree::Sub_00152B10() {
    hp2::not_implemented("0x00152B10", "TopologyTree::Sub_00152B10 (signature and full body unresolved)");
}

// ELF 0x00152C10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152c10.md.
void TopologyTree::Loader() {
    hp2::not_implemented("0x00152C10", "TopologyTree::Loader (signature and full body unresolved)");
}

// ELF 0x00152D30; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152d30.md.
void TopologyTree::Unloader() {
    hp2::not_implemented("0x00152D30", "TopologyTree::Unloader (signature and full body unresolved)");
}

// ELF 0x00152DB0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152db0.md.
void TopologyTree::GetSquareCorner() {
    hp2::not_implemented("0x00152DB0", "TopologyTree::GetSquareCorner (signature and full body unresolved)");
}

// ELF 0x00152E08; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152e08.md.
void TopologyTree::GetSquare() {
    hp2::not_implemented("0x00152E08", "TopologyTree::GetSquare (signature and full body unresolved)");
}

// ELF 0x00152F38; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152f38.md.
void TopologyTree::GetIntersectingPolygons() {
    hp2::not_implemented("0x00152F38", "TopologyTree::GetIntersectingPolygons (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001517D8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001517d8.md.
void TrackPolygon::PointInShaft() {
    hp2::not_implemented("0x001517D8", "TrackPolygon::PointInShaft (signature and full body unresolved)");
}
} // namespace hp2::recovered
