#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <cstdint>
#include <unordered_map>

struct EditorMarkerModel;
struct EditorMarkerTexture;

namespace hp2 {

godot::Vector3 race_vector(const float* source);
godot::Transform3D race_transform(const float* source);

// Temporary decode cache. The scene retains Godot Refs only.
class MeshConverter {
public:
    godot::Ref<godot::ArrayMesh> convert(const EditorMarkerModel& model, bool depthWrite = true,
        std::size_t boneCount = 0);

private:
    godot::Ref<godot::StandardMaterial3D> material(const EditorMarkerTexture& descriptor);
    std::unordered_map<const EditorMarkerTexture*, godot::Ref<godot::StandardMaterial3D>> materials_;
};

}
