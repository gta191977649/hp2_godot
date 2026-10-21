#pragma once
#include "game_core/port/track/effects.hpp"
#include <string>
namespace port::godot_bridge {
// Native local position (host mesh axes y,z,x), raw native CUSTOM0 normal.
// Bind base_texture, mip_1..MXL, specular_texture when Fusion40 !=None.
// source_camera is inverse-rotation*(native camera-object translation).
// Bind material_350..353 even for unlit strip40; light_35c..362 are per-command.
// base_lit/flag_0x4 are specialized from effective flags, not caller overrides.
// depth_near/depth_far define the source depth projection; bind actual view values.
std::string track_shader_source(const hp2::assets::TpkTextureHeader&,
    const port::track::EffectState&,port::track::Fusion40);
// Separate original packet318 additive pass. Packet31C requires destination alpha.
std::string track_environment_shader_source(const port::track::EffectState&);
}
