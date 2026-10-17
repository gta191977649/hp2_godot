#pragma once
// Port-owned spline storage; no original memory layout or allocation is implied.
#include <array>
#include <vector>
#include <cstdint>
namespace port::track {
struct Spline {
    std::vector<std::array<float,2>> points;
    bool looping=false;
};
struct PathRow { int begin=0,end=0; Spline spline; };
struct PathPoint {std::array<float,2> position{};int path=0;std::array<int,3> neighbours{};};
struct Paths {std::vector<PathRow> paths;std::vector<PathPoint> points;};
}
