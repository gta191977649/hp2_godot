#pragma once
// E0056H01: port-side representation and distance policy, NOT original PS2 LOD.
#include "runtime.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
namespace port::track {
struct StaticLodLevel {
    unsigned slot=0;
    std::uint32_t model_hash=0;
    std::size_t solid=0;
    std::int16_t threshold=0;
    bool facade=false;
    bool available=true;
};
struct StaticLodInstance {
    std::size_t source=0;
    int section=0;
    std::array<float,3> anchor{};
    float radius=0;
    std::vector<StaticLodLevel> levels;
};
struct StaticLodScene {
    std::vector<StaticLodInstance> instances;
    std::map<int,std::vector<std::size_t>> sections;
    std::size_t levels=0;
    std::vector<std::string> gaps;
};
struct LodDistanceRange {unsigned slot=0;float begin=0,end=0;};
inline StaticLodScene describe_static_lods(const Scene& scene) {
    StaticLodScene result;result.instances.resize(scene.instances.size());
    ResourceIndex index(scene);std::vector<bool> owned(scene.instances.size());
    for(const auto& section:scene.sections)for(std::size_t i=0;i<section.instance_count;++i){
        const auto source=section.instance_begin+i;
        if(source>=owned.size()||owned[source])throw std::runtime_error("Invalid static LOD section ownership");
        owned[source]=true;const auto& instance=scene.instances[source];
        auto& item=result.instances[source];item.source=source;item.section=section.number;
        item.anchor=instance.position;item.radius=instance.radius+4;
        if(!std::isfinite(item.radius)||item.radius<=0)throw std::runtime_error("Invalid static LOD radius");
        // The sky is submitted separately by the shared environment renderer.
        if(std::find(scene.environment.sky_instances.begin(),scene.environment.sky_instances.end(),source)!=scene.environment.sky_instances.end())continue;
        for(unsigned slot=0;slot<3;++slot){
            const auto hash=instance.lod_hashes[slot];if(!hash)continue;
            const auto model=index.solids.find(hash);
            const bool available=model!=index.solids.end();
            if(!available)result.gaps.push_back("Static LOD model resource missing: hash="+std::to_string(hash)+" instance="+std::to_string(source)+" slot="+std::to_string(slot));
            // Retain the missing slot's interval. Original missing Solid draws
            // nothing; a lower model must not silently fill its interval.
            item.levels.push_back({slot,hash,available?model->second:std::size_t(-1),instance.far_clip_sizes[slot],instance.facades[slot],available});
            ++result.levels;
        }
    }
    if(std::find(owned.begin(),owned.end(),false)!=owned.end())throw std::runtime_error("Unowned static LOD instance");
    // Preserve the original static leaf order, without its per-frame frustum
    // or projected-size tests. Unreferenced records are never submitted.
    for(const auto& section:scene.sections){
        auto& order=result.sections[section.number];if(section.nodes.empty())continue;
        std::vector<std::size_t> stack{0};std::size_t visits=0;
        while(!stack.empty()){
            const auto index=stack.back();stack.pop_back();
            if(index>=section.nodes.size()||++visits>section.nodes.size())throw std::runtime_error("Invalid static HLOD scenery tree");
            for(auto code:section.nodes[index].children){
                if(code<0)stack.push_back(std::size_t(-int(code)));
                else {if(std::size_t(code)>=section.instance_count)throw std::runtime_error("HLOD tree instance exceeds section");
                    order.push_back(section.instance_begin+std::size_t(code));}
            }
            if(stack.size()>64)throw std::runtime_error("Static HLOD tree stack exceeds original capacity");
        }
    }
    return result;
}
inline std::vector<LodDistanceRange> lod_distance_ranges(const StaticLodInstance& instance,float focal_h) {
    if(!std::isfinite(focal_h)||focal_h<=0||!std::isfinite(instance.radius)||instance.radius<=0)
        throw std::invalid_argument("Invalid host HLOD projection");
    std::vector<LodDistanceRange> result;float previous=0;
    for(const auto& level:instance.levels){
        const auto threshold=std::max(2,int(level.threshold));
        if(float(threshold)>focal_h)continue;
        const double end=double(instance.radius)*(1.0+double(focal_h)/threshold);
        if(!std::isfinite(end)||end>std::numeric_limits<float>::max())throw std::invalid_argument("Host HLOD distance overflow");
        const float upper=float(end);
        if(upper<=previous)continue; // Earlier slots own overlapping source intervals.
        const float begin=result.empty()?0:std::nextafter(previous,std::numeric_limits<float>::infinity());
        if(begin<=upper)result.push_back({level.slot,begin,upper});
        previous=upper;
    }
    return result;
}
}
