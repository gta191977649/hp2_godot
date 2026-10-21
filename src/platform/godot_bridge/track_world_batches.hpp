#pragma once
// Port-only matrix-attribute batches. Native command order and local lighting remain authoritative.
#include "track_command_batches.hpp"
namespace port::godot_bridge {
inline bool eligible_track_world_batch(const port::track::DrawCommand& c){
    if(c.identity||c.effect.flags&0x1c0||c.effect.alpha_pass||c.effect.environment_pass||
       c.effect.rewrite_alpha||c.effect.second_pass_320||c.effect.matrix_pack||
       c.effect.texture.additive||c.effect.texture.blend!=port::vehicle::BlendMode::Copy)return false;
    for(auto v:c.matrix)if(!std::isfinite(v))return false;
    return c.matrix[3]==0&&c.matrix[7]==0&&c.matrix[11]==0&&c.matrix[15]==1;
}
struct TrackSectionOwner {int section=0;unsigned count=0;};
inline std::vector<TrackSectionOwner> track_section_owners(const port::track::Scene& scene){
    std::size_t size=scene.instances.size();
    for(const auto& s:scene.sections){if(s.instance_count>std::numeric_limits<std::size_t>::max()-s.instance_begin)throw std::length_error("Section ownership range overflow");size=std::max(size,s.instance_begin+s.instance_count);}
    std::vector<TrackSectionOwner> result(size);
    for(const auto& s:scene.sections)for(std::size_t i=0;i<s.instance_count;++i){auto& o=result[s.instance_begin+i];o.section=s.number;++o.count;}
    return result;
}
inline std::vector<TrackCommandBatch> contiguous_track_world_batches(const port::track::Scene& scene,const port::track::Frame& frame,const std::vector<TrackSectionOwner>* ownership=nullptr){
    auto result=contiguous_track_batches(scene,frame);
    std::vector<TrackCommandBatch> merged;merged.reserve(result.size());
    // Bounded chunks avoid a whole-track mesh and excessive upload on visibility changes.
    for(auto& batch:result){
        bool append=false;
        if(!merged.empty()){
            const auto& a=frame.commands[merged.back().subranges.back().command];
            const auto& b=frame.commands[batch.subranges.front().command];
            auto normalized=b;normalized.matrix=a.matrix;
            append=eligible_track_world_batch(a)&&eligible_track_world_batch(b)&&
                a.lod==b.lod&&same_track_batch_state(scene,a,normalized)&&
                merged.back().vertices+batch.vertices<=65536;
            // Keep original section ownership as a conservative spatial boundary.
            if(append){
                unsigned owners_a=0,owners_b=0;int section_a=0,section_b=0;
                if(ownership){
                    if(a.instance<ownership->size()){const auto& o=(*ownership)[a.instance];owners_a=o.count;section_a=o.section;}
                    if(b.instance<ownership->size()){const auto& o=(*ownership)[b.instance];owners_b=o.count;section_b=o.section;}
                }else for(const auto& section:scene.sections){
                    if(a.instance>=section.instance_begin&&a.instance-section.instance_begin<section.instance_count){++owners_a;section_a=section.number;}
                    if(b.instance>=section.instance_begin&&b.instance-section.instance_begin<section.instance_count){++owners_b;section_b=section.number;}
                }
                append=owners_a==1&&owners_b==1&&section_a==section_b;
            }
        }
        if(!append){merged.push_back(std::move(batch));continue;}
        auto& target=merged.back();
        for(auto range:batch.subranges){range.first_vertex+=target.vertices;range.first_index+=target.indices;target.subranges.push_back(range);}
        target.vertices+=batch.vertices;target.indices+=batch.indices;target.baked_world=true;
    }
    return merged;
}
}
