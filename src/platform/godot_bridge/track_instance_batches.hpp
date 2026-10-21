#pragma once
// TASK0055 host-only instancing. Native packets are neither reordered nor rewritten.
#include "track_command_batches.hpp"
#include <cmath>
namespace port::godot_bridge {
struct TrackInstanceBoundary {
    unsigned view=1;
    int section=0;
    std::size_t lod_epoch=0;
    bool known=true;
    bool operator==(const TrackInstanceBoundary& b)const{return view==b.view&&section==b.section&&lod_epoch==b.lod_epoch;}
};
struct TrackInstanceSpan {std::size_t first=0,count=0;bool eligible=false;};
struct TrackInstancePlan {
    std::vector<TrackInstanceSpan> spans;
    std::size_t commands=0,eligible_commands=0,instanced_commands=0,groups=0,saved_draws=0;
};
inline bool valid_track_instance_matrix(const std::array<float,16>& m){
    for(float v:m)if(!std::isfinite(v))return false;
    if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)return false;
    const double determinant=double(m[0])*(double(m[5])*m[10]-double(m[9])*m[6])-double(m[4])*(double(m[1])*m[10]-double(m[9])*m[2])+double(m[8])*(double(m[1])*m[6]-double(m[5])*m[2]);
    // Mirror transforms change winding and require a separate proven cull policy.
    return std::isfinite(determinant)&&determinant>0;
}
inline bool eligible_track_instance(const port::track::Scene& scene,const port::track::DrawCommand& c){
    const auto& surface=scene.geometry.solids.at(c.solid).surfaces.at(c.surface);
    return !surface.vertices.empty()&&!surface.indices.empty()&&!(c.effect.flags&0x1c0)&&!c.has_light_context&&
        !c.effect.alpha_pass&&!c.effect.environment_pass&&!c.effect.rewrite_alpha&&!c.effect.second_pass_320&&
        !c.effect.matrix_pack&&!c.effect.texture.additive&&c.effect.texture.blend==port::vehicle::BlendMode::Copy&&valid_track_instance_matrix(c.matrix);
}
inline bool same_track_instance_state(const port::track::Scene& scene,const port::track::DrawCommand& a,const port::track::DrawCommand& b){
    if(a.solid!=b.solid||a.surface!=b.surface||a.lod!=b.lod||a.visibility!=b.visibility)return false;
    auto normalized=b;normalized.matrix=a.matrix;
    // Matrix is the only differing draw input supported by this host experiment.
    return same_track_batch_state(scene,a,normalized);
}
inline TrackInstancePlan contiguous_track_instance_batches(const port::track::Scene& scene,const port::track::Frame& frame,const std::vector<TrackInstanceBoundary>& boundaries){
    if(boundaries.size()!=frame.commands.size())throw std::invalid_argument("Track instance boundaries must identify every command");
    TrackInstancePlan plan;plan.commands=frame.commands.size();
    for(std::size_t i=0;i<frame.commands.size();++i){
        const bool eligible=boundaries[i].known&&eligible_track_instance(scene,frame.commands[i]);if(eligible)++plan.eligible_commands;
        const bool append=eligible&&!plan.spans.empty()&&plan.spans.back().eligible&&boundaries[i]==boundaries[i-1]&&same_track_instance_state(scene,frame.commands[i-1],frame.commands[i]);
        if(append)++plan.spans.back().count;else plan.spans.push_back({i,1,eligible});
    }
    for(const auto& span:plan.spans)if(span.eligible&&span.count>1){++plan.groups;plan.instanced_commands+=span.count;plan.saved_draws+=span.count-1;}
    return plan;
}
// Section ownership is derived without changing original section submission order.
inline std::vector<TrackInstanceBoundary> track_instance_boundaries(const port::track::Scene& scene,const port::track::Frame& frame,unsigned view){
    std::vector<TrackInstanceBoundary> result;result.reserve(frame.commands.size());
    for(const auto& command:frame.commands){int section=0;unsigned owners=0;for(const auto& s:scene.sections)if(command.instance>=s.instance_begin&&command.instance-s.instance_begin<s.instance_count){section=s.number;++owners;}result.push_back({view,section,command.lod,owners==1});}return result;
}
struct TrackInstanceSubmission {TrackCommandBatch batch;std::size_t instance_count=1;};
// The fallback portions retain the existing contiguous mesh batching algorithm.
// An instanced span owns only its first source surface; transforms carry repetition.
inline std::vector<TrackInstanceSubmission> track_instance_submissions(const port::track::Scene& scene,const port::track::Frame& frame,const TrackInstancePlan& plan){
    std::vector<TrackInstanceSubmission> result;std::size_t next=0;bool barrier=true;
    constexpr auto maximum=std::size_t(std::numeric_limits<std::int32_t>::max());
    for(const auto& span:plan.spans){
        if(span.first!=next||span.count==0||span.count>frame.commands.size()-next)throw std::invalid_argument("Instance spans must partition the original command stream");
        const bool instanced=span.eligible&&span.count>1;
        const auto append=[&](std::size_t i,bool force){const auto& c=frame.commands[i];const auto& surface=scene.geometry.solids.at(c.solid).surfaces.at(c.surface);if(surface.vertices.size()>maximum)throw std::length_error("Track mesh exceeds host signed index capacity");const bool capacity=!result.empty()&&result.back().batch.vertices<=maximum&&surface.vertices.size()<=maximum-result.back().batch.vertices;if(force||!capacity||!same_track_batch_state(scene,frame.commands[i-1],c))result.emplace_back();auto& batch=result.back().batch;batch.subranges.push_back({i,batch.vertices,surface.vertices.size(),batch.indices,surface.indices.size()});batch.vertices+=surface.vertices.size();batch.indices+=surface.indices.size();};
        if(instanced){append(span.first,true);result.back().instance_count=span.count;barrier=true;}
        else{for(std::size_t i=span.first;i<span.first+span.count;++i){append(i,barrier);barrier=false;}}
        next+=span.count;
    }
    if(next!=frame.commands.size())throw std::invalid_argument("Instance spans do not cover the original command stream");
    return result;
}

}

