#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/texture2d_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <cstdint>
#include <map>
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
    godot::Ref<godot::ArrayMesh> convert(const EditorMarkerModel& model, bool depthWrite,
        std::size_t boneCount, bool canTriggerRigidAnimation);

private:
    struct TextureSequence {
        godot::Ref<godot::Texture2DArray> texture;
        int count{}, rate{};
        bool realClock{};
        bool cutout{};
    };
    godot::Ref<godot::StandardMaterial3D> material(const EditorMarkerTexture& descriptor);
    const TextureSequence& texture_sequence(const EditorMarkerTexture& descriptor);
    std::unordered_map<const EditorMarkerTexture*, godot::Ref<godot::StandardMaterial3D>> materials_;
    std::unordered_map<const EditorMarkerTexture*, TextureSequence> textureSequences_;
    std::map<godot::String, godot::Ref<godot::Shader>> animatedShaders_;
    std::map<std::pair<const EditorMarkerTexture*, bool>, godot::Ref<godot::Material>> animatedTextures_;
};

}
