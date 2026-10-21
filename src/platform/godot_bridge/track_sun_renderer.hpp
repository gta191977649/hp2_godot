#pragma once
#include "game_core/port/track/assets.hpp"
#include "game_core/port/track/effects.hpp"
#include "game_core/port/driving/memory.hpp"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <functional>
#include <memory>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
namespace port::godot_bridge {
struct SunFrame {std::size_t quads=0;unsigned uncovered=0;bool readback_pending=false,uncovered_available=true;std::vector<std::string> gaps;std::vector<port::track::AlphaKey> alpha_keys;};
class TrackSunRenderer {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using TextureLookup=std::function<godot::Ref<godot::Texture2D>(std::uint32_t)>;
 using MipLookup=std::function<godot::Ref<godot::Texture2D>(std::uint32_t,unsigned)>;
 TrackSunRenderer(godot::Node3D&,TextureLookup,MipLookup={});~TrackSunRenderer();void clear();
 SunFrame update(godot::Camera3D*,const port::track::Scene&,port::driving::Memory&,godot::Ref<godot::Texture2D> raw_depth,bool enabled=true,unsigned host_layer=1);
 // Call once from RenderingServer.frame_post_draw. The originalSun algorithm
 // consumes the previous completed32x32 transfer before scheduling the next.
 void complete_readback();
 godot::Array scheduled_viewports()const;
 // Port-only readback diagnostics; optional comparison uses the reference GPU copy.
 godot::Dictionary stats()const;
 void set_alpha_ranks(const std::vector<std::size_t>& ranks,std::size_t total,float host_origin_depth);
};
}
