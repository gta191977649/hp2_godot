#pragma once
#include <string_view>
namespace port::godot_bridge {
// Host-only transport of original top-left screen coordinates. Godot flips its
// GPU projection Y for render targets; explicit POSITION bypasses that matrix.
// Follow its sign so the custom packets agree with ordinary world geometry.
inline constexpr std::string_view screen_position_shader=R"glsl(
vec4 hp2_host_screen_position(vec2 top_left_uv,float projection_y){
    vec2 ndc=vec2(top_left_uv.x*2.0-1.0,1.0-top_left_uv.y*2.0);
    ndc.y*=sign(projection_y);
    return vec4(ndc,0.0,1.0);
}
)glsl";
}
