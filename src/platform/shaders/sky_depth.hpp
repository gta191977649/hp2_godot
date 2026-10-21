#pragma once
#include <string_view>
namespace port::godot_bridge {
// Host clip convention adapter. Original sky depth is at the far background;
// Godot Compatibility uses CLIP_SPACE_FAR=-1, Forward+ uses0.
inline constexpr std::string_view sky_far_depth_vertex=R"glsl(
    POSITION=PROJECTION_MATRIX*MODELVIEW_MATRIX*vec4(VERTEX,1.0);
    POSITION.z=CLIP_SPACE_FAR*POSITION.w;
)glsl";
}
