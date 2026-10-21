#pragma once
// Port-only exact planner cache. Never cache or replace original frame preparation.
#include "track_world_batches.hpp"
#include <map>
namespace port::godot_bridge {
inline bool same_submission_input(const port::track::DrawCommand& a,const port::track::DrawCommand& b){
    const auto effect=[](const auto& c){const auto& e=c.effect;const auto& t=e.texture;const auto& z=e.test;
        return std::tie(e.flags,e.matrix_pack,e.rendering_order,e.alpha_pass,e.rewrite_alpha,e.second_pass_320,e.environment_pass,
            t.blend,t.deferred,t.texture_alpha,t.alpha_register,t.tex1_register,t.magnification_linear,t.minification_linear,t.repeat_u,t.repeat_v,t.maximum_mip_level,t.requires_lod_projection,t.additive,
            z.test_register,z.write_depth,z.alpha_test,z.alpha_reference,z.depth_test);};
    return std::tie(a.instance,a.solid,a.surface,a.lod,a.visibility,a.identity,a.has_light_context,a.original_order)==std::tie(b.instance,b.solid,b.surface,b.lod,b.visibility,b.identity,b.has_light_context,b.original_order)&&
        std::memcmp(a.matrix.data(),b.matrix.data(),sizeof(a.matrix))==0&&std::memcmp(a.light_context.data(),b.light_context.data(),sizeof(a.light_context))==0&&
        std::memcmp(&a.depth,&b.depth,sizeof(a.depth))==0&&effect(a)==effect(b);
}
// Only fields unobserved by BOTH contiguous planners are normalized. Alpha depth
// remains bit-exact because it defines an original ordering barrier.
inline bool same_submission_plan_input(port::track::DrawCommand a,port::track::DrawCommand b){
    a.visibility=b.visibility=0;a.original_order=b.original_order=0;
    if(!a.effect.alpha_pass&&!b.effect.alpha_pass)a.depth=b.depth=0;
    return same_submission_input(a,b);
}
class TrackSubmissionCache {
    const port::track::Scene* scene_=nullptr;
    std::vector<TrackSectionOwner> owners_;
    std::vector<port::track::DrawCommand> inputs_;
    std::vector<TrackCommandBatch> plan_;
    bool valid_=false,world_=false,dynamic_=false;
    struct SurfaceDescriptor {std::size_t vertices,indices;std::uint32_t texture,material;};
    std::map<std::pair<std::size_t,std::size_t>,SurfaceDescriptor> surfaces_;
    static constexpr std::size_t surface_capacity_=4096;
    SurfaceDescriptor surface(const port::track::Scene& scene,const port::track::DrawCommand& c){
        const auto key=std::make_pair(std::size_t(c.solid),std::size_t(c.surface));
        const auto found=surfaces_.find(key);
        if(found!=surfaces_.end()){++surface_hits;return found->second;}
        const auto& s=scene.geometry.solids.at(c.solid).surfaces.at(c.surface);
        const SurfaceDescriptor result{s.vertices.size(),s.indices.size(),s.texture_hash,s.material_hash};
        if(surfaces_.size()==surface_capacity_){surfaces_.clear();++surface_evictions;}
        surfaces_.emplace(key,result);++surface_builds;return result;
    }
    bool same_state(const port::track::Scene& scene,const port::track::DrawCommand& a,const port::track::DrawCommand& b){
        const auto sa=surface(scene,a),sb=surface(scene,b);
        if(sa.texture!=sb.texture||sa.material!=sb.material||a.effect.environment_pass||b.effect.environment_pass)return false;
        // Compare only the state fields used by same_track_batch_state; geometry,
        // instance selection, visibility and LOD do not define its first-stage key.
        auto x=a,y=b;x.instance=y.instance=0;x.solid=y.solid=0;x.surface=y.surface=0;
        x.lod=y.lod=0;x.visibility=y.visibility=0;x.original_order=y.original_order=0;
        if(!a.effect.alpha_pass)x.depth=y.depth=0;
        return same_submission_input(x,y);
    }
    std::vector<TrackCommandBatch> dynamic_plan(const port::track::Scene& scene,const port::track::Frame& frame,bool world){
        std::vector<TrackCommandBatch> result;result.reserve(frame.commands.size());
        constexpr auto maximum=std::size_t(std::numeric_limits<std::int32_t>::max());
        for(std::size_t i=0;i<frame.commands.size();++i){
            const auto counts=surface(scene,frame.commands[i]);
            if(counts.vertices>maximum)throw std::length_error("Track mesh exceeds host signed index capacity");
            const bool capacity=!result.empty()&&result.back().vertices<=maximum&&counts.vertices<=maximum-result.back().vertices;
            if(!capacity||!same_state(scene,frame.commands[i-1],frame.commands[i]))result.emplace_back();
            auto& batch=result.back();batch.subranges.push_back({i,batch.vertices,counts.vertices,batch.indices,counts.indices});
            batch.vertices+=counts.vertices;batch.indices+=counts.indices;
        }
        if(!world)return result;
        std::vector<TrackCommandBatch> merged;merged.reserve(result.size());
        for(auto& batch:result){
            bool append=false;
            if(!merged.empty()){
                const auto& a=frame.commands[merged.back().subranges.back().command];
                const auto& b=frame.commands[batch.subranges.front().command];
                auto normalized=b;normalized.matrix=a.matrix;
                append=eligible_track_world_batch(a)&&eligible_track_world_batch(b)&&a.lod==b.lod&&
                    same_state(scene,a,normalized)&&merged.back().vertices+batch.vertices<=65536;
                if(append){
                    append=a.instance<owners_.size()&&b.instance<owners_.size()&&
                        owners_[a.instance].count==1&&owners_[b.instance].count==1&&
                        owners_[a.instance].section==owners_[b.instance].section;
                }
            }
            if(!append){merged.push_back(std::move(batch));continue;}
            auto& target=merged.back();
            for(auto range:batch.subranges){range.first_vertex+=target.vertices;range.first_index+=target.indices;target.subranges.push_back(range);}
            target.vertices+=batch.vertices;target.indices+=batch.indices;target.baked_world=true;
        }
        return merged;
    }
public:
    std::uint64_t hits=0,builds=0,owner_builds=0,compares=0;
    std::uint64_t dynamic_hits=0,dynamic_builds=0,size_misses=0,state_misses=0,mode_misses=0,disabled_builds=0,scene_builds=0,cold_builds=0;
    const char* last_build_reason="none";
    std::uint64_t surface_hits=0,surface_builds=0,surface_evictions=0;
    std::size_t cached_surfaces()const{return surfaces_.size();}
    void clear(){scene_=nullptr;owners_.clear();inputs_.clear();plan_.clear();valid_=false;dynamic_=false;surfaces_.clear();hits=builds=owner_builds=compares=0;dynamic_hits=dynamic_builds=size_misses=state_misses=mode_misses=disabled_builds=0;surface_hits=surface_builds=surface_evictions=scene_builds=cold_builds=0;last_build_reason="none";}
    const std::vector<TrackCommandBatch>& acquire(const port::track::Scene& scene,const port::track::Frame& frame,bool world,bool enabled,bool dynamic_plan_enabled=false){
        const bool changed_scene=scene_!=&scene;
        if(changed_scene){clear();scene_=&scene;++scene_builds;}
        if(!enabled){last_build_reason="disabled";++disabled_builds;valid_=false;plan_=world?contiguous_track_world_batches(scene,frame):contiguous_track_batches(scene,frame);++builds;return plan_;}
        if(!valid_){++cold_builds;last_build_reason=changed_scene?"scene":"cold";}
        else if(world_!=world||dynamic_!=dynamic_plan_enabled){++mode_misses;last_build_reason="mode";}
        else if(inputs_.size()!=frame.commands.size()){++size_misses;last_build_reason="selection_size";}
        bool same=valid_&&world_==world&&dynamic_==dynamic_plan_enabled&&inputs_.size()==frame.commands.size();
        if(same)for(std::size_t i=0;i<inputs_.size();++i){++compares;if(!(dynamic_plan_enabled?same_submission_plan_input(inputs_[i],frame.commands[i]):same_submission_input(inputs_[i],frame.commands[i]))){same=false;++state_misses;last_build_reason="selection_or_state";break;}}
        if(same){++hits;if(dynamic_plan_enabled)++dynamic_hits;return plan_;}
        if(world&&owners_.empty()){owners_=track_section_owners(scene);++owner_builds;}
        plan_=dynamic_plan_enabled?dynamic_plan(scene,frame,world):(world?contiguous_track_world_batches(scene,frame,&owners_):contiguous_track_batches(scene,frame));
        if(dynamic_plan_enabled)++dynamic_builds;
        inputs_=frame.commands;world_=world;dynamic_=dynamic_plan_enabled;valid_=true;++builds;return plan_;
    }
};
}
