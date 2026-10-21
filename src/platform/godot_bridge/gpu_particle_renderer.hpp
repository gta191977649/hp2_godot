#pragma once
#include "track_particle_renderer.hpp"
#include "game_core/port/track/particles.hpp"
namespace port::godot_bridge {
// TASK0085 host adapter; original emission remains in ParticleSimulation.
class GpuParticleRenderer {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 using TextureLookup=TrackParticleRenderer::TextureLookup;
 using MipLookup=TrackParticleRenderer::MipLookup;
 GpuParticleRenderer(godot::Node3D&,TextureLookup,MipLookup={});
 ~GpuParticleRenderer();
 void clear();
 void commit_frame();
 void abort_frame();
 void set_material_callbacks(std::function<void(const godot::Ref<godot::ShaderMaterial>&)>,std::function<void(const godot::Ref<godot::ShaderMaterial>&,const godot::StringName&,const godot::Variant&)>);
 ParticleFrame update(std::shared_ptr<const port::track::Scene>,const std::vector<port::track::ParticleGpuCommand>&,const port::track::FrameView&,godot::Ref<godot::Texture2D>,bool=true,unsigned=1,godot::Camera3D* =nullptr,bool=true);
 void set_alpha_ranks(const std::vector<std::size_t>&,std::size_t,float);
 godot::Dictionary stats()const;
};
}
