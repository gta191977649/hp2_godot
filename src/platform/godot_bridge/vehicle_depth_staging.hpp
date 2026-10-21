#pragma once
// E0055V01 host-only BASE writer contract. Capture exact setter arguments, then
// let the consumer read at its existing poll point. No source/depth setters here.
// Registry and publishers are main-thread only; no Godot objects cross workers.
#include "parameter_staging_registry.hpp"
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>
namespace port::godot_bridge::vehicle_depth_staging {
using Registry=ParameterStagingRegistry<godot::StringName,godot::Variant>;
using Owner=Registry::Owner;
inline Registry& registry(){static Registry value;return value;}
inline Owner create(const godot::Ref<godot::ShaderMaterial>& material){
    if(material.is_null()||material->get_shader().is_null())return {};
    const std::array<godot::StringName,8> names{{"source_camera","light_35c","light_35d","light_35e","light_35f","light_360","light_361","light_362"}};
    return registry().create(material->get_instance_id(),material->get_shader()->get_instance_id(),names);
}
inline void publish(const Owner& owner,std::size_t index,const godot::Variant& value){registry().publish(owner,index,value);}
inline bool lookup(std::uint64_t material_id,std::uint64_t shader_id,const godot::StringName& name,godot::Variant& value){return registry().lookup(material_id,shader_id,name,value);}
inline bool read_snapshot(std::uint64_t material_id,std::uint64_t shader_id,Registry::Snapshot& value){return registry().read_snapshot(material_id,shader_id,value);}
inline void invalidate(std::uint64_t material_id){registry().invalidate(material_id);}
inline Registry::Statistics stats(){return registry().stats();}
}
