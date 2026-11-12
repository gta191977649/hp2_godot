#pragma once
// Port-owned immutable transport for original ePoly map output. UV.q remains
// projective until the GPU consumer; this is not an original C++ ABI layout.
#include <array>
#include <cstdint>
namespace port::race {
struct HudVertex {
 std::array<float,3> position{};
 std::array<float,3> uvq{{0,0,1}};
 std::uint32_t colour=0x80808080;
};
struct HudPolygon {
 std::array<HudVertex,4> vertices;
 std::uint32_t texture_hash=0;
 int layer=0;
 bool flailer=false;
};
}
