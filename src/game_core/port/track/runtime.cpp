#include "runtime.hpp"
#include "static_lods.hpp"
#include "game_core/indep/inc/Scenery.hpp"
#include "game_core/indep/src/scenery.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include <cmath>
#include <numeric>
#include <stdexcept>
namespace port::track {
ResourceIndex::ResourceIndex(const Scene& scene){
    solids.reserve(scene.geometry.solids.size());textures.reserve(scene.textures.size());sections.reserve(scene.sections.size());
    for(std::size_t i=0;i<scene.geometry.solids.size();++i)solids.emplace(scene.geometry.solids[i].hash,i);
    for(std::size_t i=0;i<scene.textures.size();++i)textures.emplace(scene.textures[i].header.tex_hash,i);
    for(std::size_t i=0;i<scene.sections.size();++i)sections.emplace(scene.sections[i].number,i);
}
void Runtime::load(std::shared_ptr<const Scene> scene){if(!scene)throw std::invalid_argument("Null track runtime scene");std::vector<std::size_t> order(scene->zones.size());std::iota(order.begin(),order.end(),0);auto index=std::make_unique<ResourceIndex>(*scene);scene_=std::move(scene);index_=std::move(index);zone_order_=std::move(order);texture_chain_order_.clear();}
void Runtime::load(assets::Source& source,const Entry& entry){auto scene=std::make_shared<const Scene>(load_scene(source,entry));load(std::move(scene));}
void Runtime::unload(){scene_.reset();index_.reset();zone_order_.clear();texture_chain_order_.clear();}
Frame Runtime::prepare_frame(const FrameView& view,port::driving::Memory& math){
    if(!scene_)throw std::logic_error("Track runtime has no loaded resources");
    for(auto v:view.position)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite track camera position");for(auto v:view.direction)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite track camera direction");for(auto v:view.clip_matrix)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite track clip matrix");if(!std::isfinite(view.focal_h)||view.focal_h<=0)throw std::invalid_argument("Invalid track horizontal focal length");
    if(!std::isfinite(view.near_z)||!std::isfinite(view.far_z)||view.near_z<=0||view.far_z<=view.near_z||!std::isfinite(view.visual_timestep)||view.visual_timestep<0)throw std::invalid_argument("Invalid original track visual view");
    auto order=zone_order_;auto texture_order=texture_chain_order_;auto result=hp2::recovered::eDisplayFrame(*scene_,view,order,math,&texture_order,index_.get());
    result.weather=query_weather(scene_->environment,view.position);
    postprocess::View post_view;post_view.id=int(view.view_id);post_view.near_z=view.near_z;post_view.far_z=view.far_z;
    post_view.camera_focal_distance=view.camera_focal_distance;post_view.camera_depth_of_field=view.camera_depth_of_field;
    post_view.weather={result.weather.fog_colour,result.weather.fog_start,result.weather.fog};
    result.postprocess=postprocess::prepare(post_view,view.post_controls,512,448);
    if(view.environment_capture_valid)result.environment_views=environment_cameras(view.position,view.environment_position,view.frame_counter,view.view_mode);
    zone_order_=std::move(order);texture_chain_order_=std::move(texture_order);return result;
}
std::string Runtime::diagnostics()const{return scene_?"Track "+std::to_string(scene_->entry.number)+": "+std::to_string(scene_->sections.size())+" scenery sections, "+std::to_string(scene_->zones.size())+" visible zones":"Track runtime unloaded";}
// E0056H01: user-authorized host frame policy; original DrawAScenery is not called.
Frame Runtime::prepare_hlod_frame(const FrameView& view,port::driving::Memory& math,
    const StaticLodScene& resources,std::size_t warm_instance) {
    if(!scene_)throw std::logic_error("HLOD runtime has no loaded resources");
    for(float v:view.position)if(!std::isfinite(v))throw std::invalid_argument("Invalid HLOD camera position");
    for(float v:view.direction)if(!std::isfinite(v))throw std::invalid_argument("Invalid HLOD camera direction");
    if(!std::isfinite(view.focal_h)||view.focal_h<=0)throw std::invalid_argument("Invalid HLOD focal value");
    const auto& scene=*scene_;Frame frame;frame.resource_index=index_.get();frame.host_lod_focal_h=view.focal_h;
    auto zone_order=zone_order_;
    hp2::recovered::GrandSceneryCullInfo::WhatSectionsShouldWeDraw(scene,view,zone_order,frame);
    if(warm_instance==std::size_t(-1))zone_order_=std::move(zone_order);
    const auto emit=[&](std::size_t i){
        const auto& object=scene.instances.at(i);const auto& item=resources.instances.at(i);
        ++frame.considered_instances;if(object.flags&view.exclude_flags&0xff)return;
        // Configure all original levels; engine distance/viewport culling selects draws.
        for(const auto& level:item.levels){
            if(!level.available)continue;
            auto matrix=object.transform;const bool identity=object.flags&0x200;
            if(!identity){if(level.facade)matrix=hp2::recovered::CreateFacadeMatrix(math,object.position,view.position);
                if(object.flags&view.exclude_flags&0x100)matrix[14]+=2;}
            hp2::recovered::GrandSceneryCullInfo::StuffScenery(scene,i,level.solid,level.slot,view,1,matrix,identity,0,frame);
        }
    };
    if(warm_instance!=std::size_t(-1))emit(warm_instance);
    else for(int section:frame.sections){const auto found=resources.sections.find(section);if(found!=resources.sections.end())for(auto i:found->second)emit(i);}
    // E0056H01: same queue assembly as the original eDisplayFrame slice,
    // applied to all resource levels. No distance-based host selection.
    std::vector<std::uint32_t> warm_chains;
    auto* texture_chain_order=warm_instance==std::size_t(-1)?&texture_chain_order_:&warm_chains;
    // Sort references to immutable frame commands, not their large matrix/light payloads.
    std::vector<std::size_t> opaque,alpha;
    std::vector<port::track::AlphaKey> keys;
    opaque.reserve(frame.commands.size());alpha.reserve(frame.commands.size());keys.reserve(frame.commands.size());
    std::vector<std::uint32_t> temporary_chains;
    auto& chains=texture_chain_order?*texture_chain_order:temporary_chains;
    std::unordered_map<std::uint32_t,bool> seen_chains;seen_chains.reserve(chains.size()+64);for(auto hash:chains)seen_chains.emplace(hash,true);
    for(std::size_t index=0;index<frame.commands.size();++index){
        const auto& command=frame.commands[index];
        const auto& surface=scene.geometry.solids.at(command.solid).surfaces.at(command.surface);
        if(seen_chains.emplace(surface.texture_hash,true).second)chains.push_back(surface.texture_hash);
        if(command.effect.alpha_pass){keys.push_back({command.effect.rendering_order,command.depth});alpha.push_back(index);}
        else opaque.push_back(index);
    }
    // Retail00109358: non-deferred texture chains are bucketed by +29 too.
    // First-use texture chains and shared image-parent sibling grouping follow
    // original static-resident order; paging uploads remain a host boundary.
    const auto hash=[&](const auto& c){return scene.geometry.solids[c.solid].surfaces[c.surface].texture_hash;};
    std::vector<std::uint32_t> grouped;
    std::map<std::pair<int,std::uint32_t>,std::uint32_t> first_parent;
    // Preserve first-use and image-parent sibling reversal; index lookups only
    // replace repeated linear searches, never the original logical ordering.
    std::unordered_map<std::uint32_t,const hp2::assets::TpkTextureHeader*> headers;
    headers.reserve(scene.textures.size());for(const auto& t:scene.textures)headers.emplace(t.header.tex_hash,&t.header);
    for(int bucket=0;bucket<8;++bucket)for(auto chain:chains){
        const auto found=headers.find(chain);if(found==headers.end())continue;const auto& t=*found->second;
        if(t.RenderingOrder!=bucket||t.Field_0x4d!=0)continue;
        const auto parent=t.ImageParentHash?t.ImageParentHash:chain;
        const auto key=std::make_pair(bucket,parent);const auto prior=first_parent.find(key);
        if(prior==first_parent.end()){first_parent.emplace(key,chain);grouped.push_back(chain);}
        else grouped.insert(std::find(grouped.begin(),grouped.end(),prior->second)+1,chain);
    }
    std::unordered_map<std::uint32_t,std::size_t> ranks;ranks.reserve(grouped.size());for(std::size_t i=0;i<grouped.size();++i)ranks.emplace(grouped[i],i);
    std::stable_sort(opaque.begin(),opaque.end(),[&](auto ai,auto bi){
        const auto& a=frame.commands[ai];const auto& b=frame.commands[bi];
        if(a.effect.rendering_order!=b.effect.rendering_order)return a.effect.rendering_order<b.effect.rendering_order;
        const auto aa=ranks.find(hash(a)),bb=ranks.find(hash(b));
        return (aa==ranks.end()?grouped.size():aa->second)<(bb==ranks.end()?grouped.size():bb->second);
    });
    for(auto index:port::track::alpha_order_001055B0(keys))opaque.push_back(alpha[index]);
    std::vector<port::track::DrawCommand> ordered;ordered.reserve(opaque.size());
    for(auto index:opaque)ordered.push_back(std::move(frame.commands[index]));
    frame.commands=std::move(ordered);
    frame.weather=query_weather(scene.environment,view.position);
    postprocess::View post;post.id=int(view.view_id);post.near_z=view.near_z;post.far_z=view.far_z;
    post.camera_focal_distance=view.camera_focal_distance;post.camera_depth_of_field=view.camera_depth_of_field;
    post.weather={frame.weather.fog_colour,frame.weather.fog_start,frame.weather.fog};
    frame.postprocess=postprocess::prepare(post,view.post_controls,512,448);
    if(view.environment_capture_valid)frame.environment_views=environment_cameras(view.position,view.environment_position,view.frame_counter,view.view_mode);
    frame.resource_index=nullptr;return frame;
}
}
