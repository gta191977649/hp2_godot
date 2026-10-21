#pragma once
#include "game_core/port/track/assets.hpp"
#include "game_core/port/track/shadow.hpp"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
namespace port::godot_bridge {
struct ShadowFrame {std::size_t strips=0,triangles=0;std::vector<std::string> gaps;};
class TrackShadowRenderer {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit TrackShadowRenderer(godot::Node3D&);~TrackShadowRenderer();void clear();
 ShadowFrame update(const godot::Dictionary& visual,const port::track::Scene&,port::driving::Memory&,const port::track::Topology&,bool enabled=true,unsigned host_layer=1);
};
}
