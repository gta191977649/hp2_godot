#pragma once
// TASK-0112 E-0112-H03. Value-only native GPU handoff for recovered HUD leaves;
// no Godot types, no original object layout and no source mutation during draw.
#include "game_core/port/driving/memory.hpp"
#include <array>
#include <vector>
namespace port::race {
struct HudTachometerQuad {
 // Per vertex: original screen X,Y,Z,U,V. Original source ePoly order.
 std::array<std::array<float,5>,4> points;
 std::array<std::uint32_t,4> colours;
};
std::vector<HudTachometerQuad> snapshot_source_tachometer(const driving::Memory&,std::uint32_t bitmap);
void prepare_source_hud_geometry(driving::Memory&); //Original bitmap cache lookup before immutable publication.
HudTachometerQuad snapshot_source_bitmap_geometry(const driving::Memory&,std::uint32_t bitmap,float alpha_scale=1); //001426E0.
float source_hud_tracking_angle(const driving::Memory&,std::uint32_t car,std::uint32_t camera); //001352FC..001353AC pure producer slice.
void prepare_source_hud_tracking(driving::Memory&,std::uint32_t world,std::uint32_t camera); //Explicit native render-input preparation.
void update_source_hud_mirror(driving::Memory&,std::uint32_t player); //00158AB0, call once before native publication.
struct HudMirrorView {
 std::uint32_t camera=0;
 std::array<int,2> texture_size{{256,64}};
 std::array<int,2> raster_size{};
 std::array<std::uint32_t,2> overlay_hashes{{0x0000dbd4,0xcf29d0df}};
 std::array<float,4> display_rect{{160,20,192,64}};
 std::array<float,2> aspect_ratio{{1.5f,0}};
 std::array<float,4> overlay_rect{};
 // 001042E8/0010EA18: raster focal lengths, including target anisotropy.
 std::array<float,2> focal_pixels{};
 float projection_aspect=0; // fy/fx in normalized clip coordinates.
 float vertical_fov_degrees=0;
};
HudMirrorView source_hud_mirror_view(const driving::Memory&,std::uint32_t player); //00106320/0010F1A8 source descriptors.
std::uint32_t source_hud_mirror_camera(const driving::Memory&,std::uint32_t player); //Read-only actual Camera owner.
}
