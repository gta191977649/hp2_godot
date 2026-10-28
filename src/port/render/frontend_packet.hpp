#pragma once
// Port-owned transport for a single physical frontend colour/depth target.
// Original submission/flush ordering is assembled by the native page owner.
#include "frame.hpp"
#include <map>
namespace port::render {
struct FrontendVertex {std::array<float,4> clip{};Vec2 uv;std::array<float,4> colour{};};
struct FrontendDraw {
    std::size_t first=0,count=0;
    TextureId texture=0;
    // 0 copy,1 source-over,2 additive; explicit host pipeline selections.
    int blend=1;
    bool depth_write=true,raw_gs=false;
    std::uint8_t repeat=0; // Original U/V repeat bits.
    float alpha_threshold=-1;
    int stage=0;
    float sort_depth=0;
    std::int8_t rendering_order=0;
};
struct FrontendPacket {
    int width=512,height=448;
    std::vector<FrontendVertex> vertices;
    std::vector<FrontendDraw> draws;
    std::map<TextureId,std::shared_ptr<const Bitmap>> textures;
};
}
