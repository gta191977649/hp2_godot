#pragma once
// Host-only MultiMesh resources; caller owns eligibility, ordering and exact transforms.
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <memory>
#include <tuple>
#include <vector>
namespace port::godot_bridge {
class TrackInstancePool {
public:
    using Key=std::tuple<int,unsigned,std::size_t,std::size_t,std::size_t>; // section, LOD, first instance, solid, surface
    struct ViewCounters {std::size_t allocations=0,uploads=0,upload_bytes=0;};
    explicit TrackInstancePool(godot::Node3D& owner);
    ~TrackInstancePool();
    void begin_view(unsigned view);
    void end_view(unsigned view);
    // Key is stable within this view. Transforms are relative to owner, in host axes.
    // Source mesh/custom attributes and immutable shared material are retained exactly.
    godot::MultiMeshInstance3D* submit(unsigned view,const Key& key,
        const godot::Ref<godot::Mesh>& mesh,const godot::Ref<godot::ShaderMaterial>& material,
        const std::vector<godot::Transform3D>& transforms,unsigned layer_mask);
    void clear();
    std::size_t entries()const;
    std::size_t allocations()const;
    std::size_t transform_uploads()const;
    std::size_t capacity()const;
    ViewCounters view_counters(unsigned view)const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
