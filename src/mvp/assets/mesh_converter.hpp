#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/texture2d_array.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector4i.hpp>

#include <cstdint>
#include <map>
#include <tuple>
#include <unordered_map>

struct EditorMarkerModel;
struct EditorMarkerTexture;
namespace godot { class MeshInstance3D; class Skeleton3D; }

namespace hp2 {

struct SceneLightContext;
struct CarPaintAssets {
    godot::Ref<godot::Texture2D> specular, reflection;
    bool frontend{};
    godot::Vector4i specularSampling, reflectionSampling;
};

godot::Ref<godot::Texture2D> upload_paint_texture(const EditorMarkerTexture* descriptor, bool authoredMips = false);
godot::Vector4i paint_texture_sampling(const EditorMarkerTexture& descriptor);
void apply_paint_lighting(godot::MeshInstance3D& mesh, const SceneLightContext& context);

godot::Vector3 race_vector(const float* source);
godot::Transform3D race_transform(const float* source);
godot::Skeleton3D* attach_pose_palette(godot::MeshInstance3D& mesh, const godot::String& name,
    std::size_t boneCount);

// Temporary decode cache. The scene retains Godot Refs only.
class MeshConverter {
public:
    enum class ScenerySurface { Full, OpaqueOnly, Reflection, ReflectionBillboard, Sky };
    godot::Ref<godot::ArrayMesh> convert_wheel_blur(const EditorMarkerModel& model);
    godot::Ref<godot::ArrayMesh> convert_frontend_platform(const EditorMarkerModel& model);
    static void audit_track_model(const EditorMarkerModel& model);
    godot::Ref<godot::ArrayMesh> convert(const EditorMarkerModel& model, bool depthWrite = true,
        std::size_t boneCount = 0);
    godot::Ref<godot::ArrayMesh> convert(const EditorMarkerModel& model, bool depthWrite,
        std::size_t boneCount, bool canTriggerRigidAnimation, ScenerySurface scenery = ScenerySurface::Full);
    explicit MeshConverter(const CarPaintAssets* paint = nullptr, bool paintWheel = false, bool track = false)
        : paint_(paint), paintWheel_(paintWheel), track_(track) {}

private:
    bool wheelBlur_{};
    bool frontendPlatform_{};
    struct TextureSequence {
        godot::Ref<godot::Texture2DArray> texture;
        godot::Ref<godot::Texture2DArray> raw;
        int count{}, rate{};
        bool realClock{};
        bool cutout{};
    };
    struct TrackTexture {
        godot::Ref<godot::Texture2D> texture;
        int minAlpha{255}, maxAlpha{};
    };
    const TrackTexture& track_texture(const EditorMarkerTexture& descriptor);
    godot::Ref<godot::StandardMaterial3D> material(const EditorMarkerTexture& descriptor);
    const TextureSequence& texture_sequence(const EditorMarkerTexture& descriptor);
    std::unordered_map<const EditorMarkerTexture*, godot::Ref<godot::StandardMaterial3D>> materials_;
    std::unordered_map<const EditorMarkerTexture*, TextureSequence> textureSequences_;
    std::map<godot::String, godot::Ref<godot::Shader>> animatedShaders_;
    std::map<std::tuple<const EditorMarkerTexture*, bool, bool, bool>, godot::Ref<godot::Material>> animatedTextures_;
    std::unordered_map<const EditorMarkerTexture*, godot::Ref<godot::Texture2D>> paintTextures_;
    std::unordered_map<const EditorMarkerTexture*, TrackTexture> trackTextures_;
    const CarPaintAssets* paint_;
    bool paintWheel_;
    bool track_;
};

}
