#pragma once
// Port-owned decoded route/path resources. No original object ABI is asserted.
// TASK-0099 E-0099-W08; Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/track/spline.hpp"
#include <array>
#include <vector>
namespace port::track {
struct RoutePathDistance {
    float begin=0,end=0,ratio=0,length=0;
    std::vector<float> cumulative;
};
struct RoutePaths {
    Paths geometry;
    std::vector<RoutePathDistance> distances;
    float track_length=0;
    bool reverse_direction=false;
    // E-0099-W26. Actual copied original20-byte TrackPathPoint records in the
    // shared Memory; decoded indices are never written into original pointers.
    std::uint32_t point_table=0;
};
struct RouteData {
    std::array<std::uint32_t,100> routes{};
    std::uint32_t signposts=0;
    std::size_t signpost_count=0;
    RoutePaths paths;
    // E-0099-W14. Retail Signpost records are12bytes (loader001C2F18);
    // Debug records are8bytes (loader001BFFC8). Preserve every original byte,
    // including Retail+8 whose behavior is not translated into Debug.
    std::vector<std::array<std::uint8_t,12>> retail_signposts;
    // Current Debug sign index -> immutable Retail record index after sorting.
    std::vector<std::uint32_t> retail_signpost_order;
    bool initialized=false;
};
// Original loader schemas: Debug001BFFC8/001BF9B0/002029C0, E-0099-W08.
// Input is an already decompressed original track bundle. Routes/signposts are
// copied into shared logical Memory; original route lists/globals are published.
// reverse_direction changes path distance orientation only. Original route
// reversal is a separate caller operation and is not performed by this adapter.
// Explicit version adapter: input is Retail7335... assets; shared gameplay
// representation is Debug815b... . Matching route header/point fields are
// reviewed separately from the differing signpost stride. No whole-version
// equivalence or interpretation of Retail Signpost+8 is asserted.
RouteData load_retail_route_data(port::driving::Memory&,const std::vector<std::uint8_t>&,
                          bool reverse_direction=false);
// Port integration of actual Debug002035D8/00202F30 for graph AddVertex002478E0.
float route_point_distance(const RouteData&,const port::driving::Memory&,
                           int route,int point);
}
