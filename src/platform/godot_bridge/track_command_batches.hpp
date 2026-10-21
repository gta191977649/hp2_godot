#pragma once
// Host-only conservative batching plan. Native commands and their order remain intact.
#include "game_core/port/track/runtime.hpp"
#include <tuple>
#include <limits>
#include <cstring>
#include <cmath>
namespace port::godot_bridge {
// Host experiment: Copy blending remains opaque after the original alpha test.
// Cutout eligibility is NOT reused for occluders (which cannot model holes).
inline bool eligible_track_opaque_queue(const port::track::DrawCommand& c){
    return !c.effect.alpha_pass&&!c.effect.environment_pass&&!c.effect.rewrite_alpha&&
        !c.effect.second_pass_320&&!c.effect.matrix_pack&&!c.effect.texture.additive&&
        c.effect.texture.blend==port::vehicle::BlendMode::Copy&&c.effect.test.write_depth&&
        (c.effect.test.alpha_test==1||c.effect.test.alpha_test==6)&&c.effect.test.depth_test==2;
}
struct TrackBatchSubrange {
    std::size_t command=0,first_vertex=0,vertex_count=0,first_index=0,index_count=0;
};
struct TrackCommandBatch {std::vector<TrackBatchSubrange> subranges;std::size_t vertices=0,indices=0;bool baked_world=false; /* Matrix rows travel per vertex; local attributes remain unchanged. */};
inline bool same_track_batch_state(const port::track::Scene& scene,
    const port::track::DrawCommand& a,const port::track::DrawCommand& b){
    // Two-pass environment draws must remain baseA/envA/baseB/envB.
    if(a.effect.environment_pass||b.effect.environment_pass)return false;
    // Additional native alpha producers may sort between different depth keys.
    if(a.effect.alpha_pass&&std::memcmp(&a.depth,&b.depth,sizeof(float))!=0)return false;
    const auto& sa=scene.geometry.solids.at(a.solid).surfaces.at(a.surface);
    const auto& sb=scene.geometry.solids.at(b.solid).surfaces.at(b.surface);
    if(sa.texture_hash!=sb.texture_hash||sa.material_hash!=sb.material_hash)return false;
    if(std::memcmp(a.matrix.data(),b.matrix.data(),sizeof(a.matrix))!=0||a.identity!=b.identity||a.has_light_context!=b.has_light_context||std::memcmp(a.light_context.data(),b.light_context.data(),sizeof(a.light_context))!=0)return false;
    const auto effect=[](const auto&v){return std::tie(v.flags,v.matrix_pack,v.rendering_order,v.alpha_pass,v.rewrite_alpha,v.second_pass_320,v.environment_pass);};
    const auto texture=[](const auto&v){return std::tie(v.blend,v.deferred,v.texture_alpha,v.alpha_register,v.tex1_register,v.magnification_linear,v.minification_linear,v.repeat_u,v.repeat_v,v.maximum_mip_level,v.requires_lod_projection,v.additive);};
    const auto test=[](const auto&v){return std::tie(v.test_register,v.write_depth,v.alpha_test,v.alpha_reference,v.depth_test);};
    return effect(a.effect)==effect(b.effect)&&texture(a.effect.texture)==texture(b.effect.texture)&&test(a.effect.test)==test(b.effect.test);
}
inline std::vector<TrackCommandBatch> contiguous_track_batches(const port::track::Scene& scene,
    const port::track::Frame& frame){
    std::vector<TrackCommandBatch> result;result.reserve(frame.commands.size());
    for(std::size_t i=0;i<frame.commands.size();++i){
        const auto& command=frame.commands[i];
        const auto& surface=scene.geometry.solids.at(command.solid).surfaces.at(command.surface);
        constexpr auto maximum=std::size_t(std::numeric_limits<std::int32_t>::max());
        if(surface.vertices.size()>maximum)throw std::length_error("Track mesh exceeds host signed index capacity");
        const bool capacity=!result.empty()&&result.back().vertices<=maximum&&surface.vertices.size()<=maximum-result.back().vertices;
        if(!capacity||!same_track_batch_state(scene,frame.commands[i-1],command))result.emplace_back();
        auto& batch=result.back();batch.subranges.push_back({i,batch.vertices,surface.vertices.size(),batch.indices,surface.indices.size()});batch.vertices+=surface.vertices.size();batch.indices+=surface.indices.size();
    }
    return result;
}
}
