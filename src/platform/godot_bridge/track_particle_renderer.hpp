#pragma once
#include "game_core/port/track/runtime.hpp"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <functional>
#include <memory>
namespace godot {class ShaderMaterial;}
namespace port::godot_bridge {
class Ps2DrivingRuntime;
struct ParticleFrame {std::size_t active=0,visible=0,groups=0,unsupported_quads=0,mesh_creations=0,surface_creations=0,mesh_updates=0,capacity_quads=0;std::vector<std::string> gaps;std::vector<port::track::AlphaKey> alpha_keys;};
class TrackParticleRenderer {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using TextureLookup=std::function<godot::Ref<godot::Texture2D>(std::uint32_t)>;
 using MipLookup=std::function<godot::Ref<godot::Texture2D>(std::uint32_t,unsigned)>;
 TrackParticleRenderer(godot::Node3D&,TextureLookup,MipLookup={},std::uint32_t original_global_seed=0x23985727);~TrackParticleRenderer();void clear();
 void commit_frame();
 void abort_frame();
 std::uint32_t global_seed()const;
 void set_material_callbacks(std::function<void(const godot::Ref<godot::ShaderMaterial>&)>,std::function<void(const godot::Ref<godot::ShaderMaterial>&,const godot::StringName&,const godot::Variant&)>);
 void set_mesh_reuse(bool enabled);
 void set_driving_source(Ps2DrivingRuntime*);
 void set_mesh_change_callback(std::function<void(godot::MeshInstance3D*)> callback);
 godot::Dictionary stats()const;
 static godot::Dictionary test_mesh_reuse();
 void set_alpha_ranks(const std::vector<std::size_t>& ranks,std::size_t total,float host_origin_depth);
 // Call once for the original main-view visual timestep; capture faces do not step WorldEffects.
 ParticleFrame update(std::shared_ptr<const port::track::Scene>,port::driving::Memory&,const port::track::FrameView&,float visual_timestep,godot::Ref<godot::Texture2D> raw_depth,bool enabled=true,unsigned host_layer=1,godot::Camera3D* source_camera=nullptr,bool require_native_depth=true);
};
}
