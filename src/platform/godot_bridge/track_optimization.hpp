#pragma once
// Host submission policy only; never an original-game behavior switch.
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <stdexcept>
namespace port::godot_bridge {
struct TrackOptimizationOptions {
    bool resource_reuse=false,particle_mesh_reuse=false,instancing=false,occlusion=false,opaque_queue=false,world_batches=false;
};
inline godot::Dictionary optimization_options_dictionary(const TrackOptimizationOptions& o){
    godot::Dictionary d;d["resource_reuse"]=o.resource_reuse;d["particle_mesh_reuse"]=o.particle_mesh_reuse;d["instancing"]=o.instancing;d["occlusion"]=o.occlusion;d["opaque_queue"]=o.opaque_queue;d["world_batches"]=o.world_batches;return d;
}
inline TrackOptimizationOptions optimization_options_from_dictionary(const godot::Dictionary& d,TrackOptimizationOptions o={}){
    const auto keys=d.keys();for(int i=0;i<keys.size();++i){const godot::String key=keys[i];if(d[key].get_type()!=godot::Variant::BOOL)throw std::invalid_argument("Optimization options must be Boolean");const bool value=d[key];
    if(key=="resource_reuse")o.resource_reuse=value;else if(key=="particle_mesh_reuse")o.particle_mesh_reuse=value;else if(key=="instancing")o.instancing=value;else if(key=="occlusion")o.occlusion=value;else if(key=="opaque_queue")o.opaque_queue=value;else if(key=="world_batches")o.world_batches=value;else throw std::invalid_argument("Unknown host optimization option");}return o;
}
inline TrackOptimizationOptions startup_track_optimizations(){
    TrackOptimizationOptions o;
    // Project default is promoted only after the TASK0055 acceptance gate.
    const godot::String configured=godot::ProjectSettings::get_singleton()->get_setting("hp2/rendering/optimization_profile","baseline");
    if(configured=="optimized")o={true,true,false,false,false,true};else if(configured!="baseline")throw std::invalid_argument("Unknown default host optimization profile");
    const auto arguments=godot::OS::get_singleton()->get_cmdline_user_args();
    for(int i=0;i<arguments.size();++i){const auto& arg=arguments[i];if(arg.begins_with("--track-optimization-profile=")){
      const auto profile=arg.trim_prefix("--track-optimization-profile=");if(profile=="optimized")o={true,true,false,false,false,true};else if(profile=="baseline")o={};else throw std::invalid_argument("Unknown track optimization profile");}}
    for(int i=0;i<arguments.size();++i){const auto& arg=arguments[i];if(!arg.begins_with("--track-opt-"))continue;const auto parts=arg.trim_prefix("--track-opt-").split("=");if(parts.size()!=2||(parts[1]!="true"&&parts[1]!="false"))throw std::invalid_argument("Expected --track-opt-<name>=true|false");godot::Dictionary d;d[parts[0]]=parts[1]=="true";o=optimization_options_from_dictionary(d,o);}return o;
}
}
