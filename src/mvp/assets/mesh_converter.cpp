#include "mesh_converter.hpp"

#include "assets/shader_resource.hpp"

#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Texture.hpp"
#include "indep/inc/eLight.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace hp2 {

using namespace godot;

namespace {

Ref<Image> texture_image(const TextureInfo& source, bool& cutout) {
    std::uint32_t width{}, height{};
    auto rgba = DecodeTextureMipRgba(source, 0, 0, source.pixels.size, width, height);
    const bool alphaTest = !source.descriptor->authoredRecord[0x4f] && !source.drawFlag4Enabled;
    for (std::size_t alpha = 3; alpha < rgba.size(); alpha += 4) {
        cutout |= alphaTest && rgba[alpha] <= 48;
        rgba[alpha] = std::min(255, 2 * int(rgba[alpha]));
    }
    PackedByteArray bytes;
    bytes.resize(rgba.size());
    std::memcpy(bytes.ptrw(), rgba.data(), rgba.size());
    auto image = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, bytes);
    if (image.is_null() || image->generate_mipmaps() != OK)
        throw std::runtime_error("Godot could not build texture " + std::to_string(source.descriptor->key));
    return image;
}

Ref<ImageTexture> animation_data(int width, int height, const std::vector<float>& data) {
    PackedByteArray bytes;
    bytes.resize(data.size() * sizeof(float));
    std::memcpy(bytes.ptrw(), data.data(), bytes.size());
    return ImageTexture::create_from_image(Image::create_from_data(width, height, false, Image::FORMAT_RGBAF, bytes));
}

float record_float(const std::uint8_t* record) {
    float value{};
    std::memcpy(&value, record, sizeof(value));
    return value;
}

bool is_light_shaft(std::uint32_t key) {
    // GRADIENTLIGHT, LIGHTRAYGRAD_3 and LIGHTRAYGRAD_4 in the retail texture packs.
    return key == 0x885adba5u || key == 0xba0e31b3u || key == 0xba0e31b4u;
}

}

Vector3 race_vector(const float* source) {
    return {source[0], source[2], -source[1]};
}

Transform3D race_transform(const float* source) {
    return {Basis(race_vector(source), race_vector(source + 8), -race_vector(source + 4)),
        race_vector(source + 12)};
}

Ref<Texture2D> upload_paint_texture(const EditorMarkerTexture* descriptor) {
    const auto* source = textureInfoForDescriptor(descriptor);
    if (!source || !source->pixels.Data()) return {};
    std::uint32_t width{}, height{};
    const auto rgba = DecodeTextureMipRgba(*source, 0, 0, source->pixels.size, width, height);
    PackedByteArray bytes;
    bytes.resize(rgba.size());
    std::memcpy(bytes.ptrw(), rgba.data(), rgba.size());
    const auto image = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, bytes);
    image->generate_mipmaps();
    auto texture = ImageTexture::create_from_image(image);
    texture->set_name(String::num_uint64(descriptor->key));
    return texture;
}

void apply_paint_lighting(MeshInstance3D& mesh, const SceneLightContext& context) {
    if (!mesh.get_mesh()->has_meta("hp2_car_paint")) return;
    constexpr std::array colors{"hp2_paint_light_r", "hp2_paint_light_g", "hp2_paint_light_b", "hp2_paint_light_a"};
    constexpr std::array lights{"hp2_paint_direction_0", "hp2_paint_direction_1", "hp2_paint_direction_2"};
    constexpr std::array environment{"hp2_paint_env_x", "hp2_paint_env_y", "hp2_paint_env_z"};
    for (std::size_t axis = 0; axis < colors.size(); ++axis)
        mesh.set_instance_shader_parameter(colors[axis], Vector4(context.colorMatrix[axis],
            context.colorMatrix[4 + axis], context.colorMatrix[8 + axis], context.colorMatrix[12 + axis]));
    for (std::size_t axis = 0; axis < lights.size(); ++axis) {
        mesh.set_instance_shader_parameter(lights[axis], Vector3(context.directionMatrix[axis],
            context.directionMatrix[4 + axis], context.directionMatrix[8 + axis]));
        mesh.set_instance_shader_parameter(environment[axis], Vector3(context.environmentMatrix[axis],
            context.environmentMatrix[4 + axis], context.environmentMatrix[8 + axis]));
    }
    mesh.set_instance_shader_parameter("hp2_paint_eye", Vector3(context.localEye[0], context.localEye[1], context.localEye[2]));
    mesh.set_instance_shader_parameter("hp2_paint_bound", true);
}

Ref<StandardMaterial3D> MeshConverter::material(const EditorMarkerTexture& descriptor) {
    if (const auto found = materials_.find(&descriptor); found != materials_.end())
        return found->second;
    const auto* source = textureInfoForDescriptor(&descriptor);
    if (!source) throw std::runtime_error("Missing decoded texture " + std::to_string(descriptor.key));
    if (!source->pixels.Data())
        throw std::runtime_error("Unloaded texture " + std::to_string(descriptor.key));
    const auto* wire = descriptor.authoredRecord;
    bool cutout = false;
    const auto image = texture_image(*source, cutout);
    const auto texture = ImageTexture::create_from_image(image);
    if (texture.is_null()) throw std::runtime_error("Godot could not allocate an ImageTexture");
    texture->set_name(String::num_uint64(descriptor.key));

    Ref<StandardMaterial3D> result;
    result.instantiate();
    result->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, texture);
    result->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS);
    result->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    result->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    result->set_flag(BaseMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
    result->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
    if (wire[0x76] == 0x48 || wire[0x76] == 0x42) {
        result->set_blend_mode(wire[0x76] == 0x48 ? BaseMaterial3D::BLEND_MODE_ADD :
            BaseMaterial3D::BLEND_MODE_SUB);
        result->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
    } else if (cutout) {
        // CreateTextureContext108CC0 and UpdateGifTags105AC0: TEST GREATER 48.
        // Prefer the authored mask when a material also requests alpha sorting.
        result->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
        result->set_alpha_scissor_threshold(97.0f / 255.0f);
    } else if (wire[0x76] == 0x44 || source->requiresDepthSort) {
        // 0x44 is (source - destination) * source alpha + destination.
        // Ordering metadata alone misses unsorted, non-depth-writing overlays.
        result->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
    }
    if (!wire[0x50]) result->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_DISABLED);
    else if (result->get_transparency() == BaseMaterial3D::TRANSPARENCY_ALPHA)
        result->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_ALWAYS);
    if (!(wire[0x4c] & 3u)) result->set_flag(BaseMaterial3D::FLAG_USE_TEXTURE_REPEAT, false);
    if (result->get_transparency() == BaseMaterial3D::TRANSPARENCY_ALPHA)
        result->set_render_priority(std::min<int>(wire[0x29], 7));
    materials_.emplace(&descriptor, result);
    return result;
}

const MeshConverter::TextureSequence& MeshConverter::texture_sequence(const EditorMarkerTexture& descriptor) {
    auto [it, inserted] = textureSequences_.try_emplace(&descriptor);
    auto& sequence = it->second;
    if (!inserted) return sequence;
    std::vector<TextureInfo> frames;
    for (const auto* pack : texturePackRegistry.animationPacks) {
        if (pack->ReadFrames(descriptor, frames, sequence.rate, sequence.realClock)) break;
    }
    if (frames.size() < 2) return sequence;
    TypedArray<Image> images;
    for (const auto& frame : frames) images.push_back(texture_image(frame, sequence.cutout));
    sequence.texture.instantiate();
    if (sequence.texture->create_from_images(images) != OK)
        throw std::runtime_error("Godot rejected texture sequence " + std::to_string(descriptor.key));
    sequence.count = static_cast<int>(frames.size());
    return sequence;
}

Ref<ArrayMesh> MeshConverter::convert(const EditorMarkerModel& model, bool depthWrite, std::size_t boneCount) {
    return convert(model, depthWrite, boneCount, false);
}

Ref<ArrayMesh> MeshConverter::convert(const EditorMarkerModel& model, bool depthWrite, std::size_t boneCount,
    bool canTriggerRigidAnimation, bool opaqueOnly) {
    if (!model.solid || !model.solid->geometry)
        throw std::runtime_error("Solid has no geometry for model " + std::to_string(model.key));
    struct Surface {
        Ref<StandardMaterial3D> material;
        const EditorMarkerTexture* descriptor{};
        PackedVector3Array vertices;
        PackedVector2Array uv;
        PackedColorArray colors;
        PackedInt32Array indices;
        PackedInt32Array bones;
        PackedFloat32Array weights;
        PackedFloat32Array animationIndices;
        PackedFloat32Array paintVertices;
        const EditorMarkerLightMaterial* lightMaterial{};
        std::uint32_t paintFlags{};
    };
    std::vector<Surface> surfaces;
    std::map<std::tuple<const EditorMarkerTexture*, const EditorMarkerLightMaterial*, std::uint32_t>, std::size_t>
        surfaceIndices;
    const auto* morph = boneCount ? nullptr : model.solid->vertexAnimation;
    if (morph && morph->count < 2) morph = nullptr;
    const auto* rigid = boneCount ? nullptr : model.solid->rigidAnimation;
    if (rigid && ((!canTriggerRigidAnimation && model.rigidAnimationStart == 0) ||
            rigid->frameCount <= 0 || rigid->frameDuration <= 0))
        rigid = nullptr;
    const auto& geometry = *(morph ? morph->frames[0] : model.solid)->geometry;
    int stride = 0;
    Ref<ImageTexture> vertexFrames, rigidFrames;
    Vector3 minimum, maximum;
    bool hasBounds = false;
    const auto extend_bounds = [&](const Vector3& point) {
        minimum = hasBounds ? minimum.min(point) : point;
        maximum = hasBounds ? maximum.max(point) : point;
        hasBounds = true;
    };
    if (morph) {
        for (const auto& strip : geometry.strips) stride += strip.positions.size();
        const int texels = stride * morph->count;
        const int rows = std::max(1, (texels + 1023) / 1024);
        std::vector<float> data(rows * 1024 * 4);
        int offset = 0;
        for (unsigned frame = 0; frame < morph->count; ++frame) {
            const auto& strips = morph->frames[frame]->geometry->strips;
            if (strips.size() != geometry.strips.size())
                throw std::runtime_error("Vertex animation changes its strip count: " + std::to_string(model.key));
            for (std::size_t i = 0; i < strips.size(); ++i) {
                const auto& strip = strips[i];
                const auto& base = geometry.strips[i];
                if (strip.positions.size() != base.positions.size() ||
                    strip.nativeTriangleOrder != base.nativeTriangleOrder ||
                    strip.textureIndex != base.textureIndex)
                    throw std::runtime_error("Vertex animation changes its topology: " + std::to_string(model.key));
                for (const auto& position : strip.positions) {
                    const auto point = race_vector(position.data());
                    extend_bounds(point);
                    for (int axis = 0; axis < 3; ++axis) data[offset++] = point[axis];
                    ++offset;
                }
            }
        }
        vertexFrames = animation_data(1024, rows, data);
    }
    if (rigid) {
        float radius = 0;
        for (const auto& strip : geometry.strips)
            for (const auto& point : strip.positions)
                radius = std::max(radius, static_cast<float>(race_vector(point.data()).length()));
        if (hasBounds) radius = std::max(radius, static_cast<float>(minimum.abs().max(maximum.abs()).length()));
        std::vector<float> data;
        data.reserve(rigid->frameCount * 12);
        for (int i = 0; i < rigid->frameCount; ++i) {
            const auto transform = race_transform(rigid->matrices + i * 16);
            const auto rotation = transform.basis.get_rotation_quaternion();
            const auto scale = transform.basis.get_scale();
            const auto extent = Vector3(1, 1, 1) *
                (radius * std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)}));
            extend_bounds(transform.origin - extent);
            extend_bounds(transform.origin + extent);
            data.insert(data.end(), {rotation.x, rotation.y, rotation.z, rotation.w,
                transform.origin.x, transform.origin.y, transform.origin.z, 0,
                scale.x, scale.y, scale.z, 0});
        }
        rigidFrames = animation_data(3, rigid->frameCount, data);
    }
    // Replacement slots are resolved by the donor for each model detail.
    // Read them without writing the shared solid table.
    std::vector<const EditorMarkerTexture*> bindings;
    bindings.reserve(geometry.textures.size());
    for (const auto& binding : geometry.textures) bindings.push_back(binding.texture);
    if (model.replacementCount > 0 && (!model.replacement || model.detailLevel < 0 || model.detailLevel >= 16))
        throw std::runtime_error("Invalid model texture replacement table");
    for (int index = 0; index < model.replacementCount; ++index) {
        const auto& replacement = model.replacement[index];
        const auto slot = replacement.textureIndices[model.detailLevel];
        if (slot >= 0 && static_cast<std::size_t>(slot) < bindings.size())
            bindings[slot] = replacement.texture;
    }
    int vertexOffset = 0;
    for (const auto& strip : geometry.strips) {
        const int stripOffset = vertexOffset;
        vertexOffset += strip.positions.size();
        if ((strip.flags & 0x400u) || strip.nativeTriangleOrder.empty()) continue;
        if (strip.nativeTopologyCount != strip.positions.size() ||
            strip.nativeTopologyMask != strip.drawDisableMask ||
            strip.textureCoordinates.size() != strip.positions.size() ||
            strip.colors.size() != strip.positions.size() ||
            strip.nativeTriangleOrder.size() % 3 ||
            strip.textureIndex < 0 || std::size_t(strip.textureIndex) >= geometry.textures.size())
            throw std::runtime_error("Invalid strip channels, topology or texture slot for model " + std::to_string(model.key));
        const auto* descriptor = bindings[strip.textureIndex];
        if (!descriptor) throw std::runtime_error("Missing texture binding for model " + std::to_string(model.key));
        const auto* texture = textureInfoForDescriptor(descriptor);
        if (opaqueOnly && texture && texture->requiresDepthSort) continue;
        const EditorMarkerLightMaterial* lightMaterial = nullptr;
        std::uint32_t paintFlags = 0;
        if (paint_ && texture && !texture->requiresDepthSort && !morph && !rigid &&
            (strip.flags & 0x1c0u) && strip.normals.size() == strip.positions.size()) {
            if (strip.lightMaterialIndex >= 0 && std::size_t(strip.lightMaterialIndex) < geometry.lightMaterials.size())
                lightMaterial = geometry.lightMaterials[strip.lightMaterialIndex];
            if (!lightMaterial && model.resources) {
                const auto* root = model.resources->catalogueRoot ? model.resources->catalogueRoot : model.resources;
                lightMaterial = root->defaultLightMaterial;
            }
            if (lightMaterial && !paint_->frontend && lightMaterial->inGameVariant)
                lightMaterial = lightMaterial->inGameVariant.get();
            if (lightMaterial) {
                paintFlags = strip.flags | (texture->drawFlag4Enabled ? 4u : 0u);
                if (!environmentMappingEnabled || paint_->reflection.is_null()) paintFlags &= ~0x100u;
            }
        }
        const auto key = std::make_tuple(descriptor, lightMaterial, paintFlags);
        const auto [entry, inserted] = surfaceIndices.emplace(key, surfaces.size());
        if (inserted) {
            auto& surface = surfaces.emplace_back();
            surface.descriptor = descriptor;
            surface.lightMaterial = lightMaterial;
            surface.paintFlags = paintFlags;
            surface.material = material(*descriptor);
            if (!depthWrite) {
                surface.material = surface.material->duplicate();
                surface.material->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_DISABLED);
            }
        }
        auto& surface = surfaces[entry->second];
        const auto start = surface.vertices.size();
        for (std::size_t i = 0; i < strip.positions.size(); ++i) {
            surface.vertices.push_back(race_vector(strip.positions[i].data()));
            if (morph) surface.animationIndices.push_back(stripOffset + i);
            if (boneCount) {
                std::array<std::uint32_t, 3> encoded{};
                std::memcpy(encoded.data(), strip.positions[i].data(), sizeof(encoded));
                // PAL10D778 stores the two bone indices and blend in XYZ's low bytes.
                const auto weight = encoded[2] & 255u;
                const auto first = encoded[weight == 1 ? 1 : 0] & 255u;
                const auto second = weight > 1 ? encoded[1] & 255u : first;
                if (first >= boneCount || second >= boneCount)
                    throw std::runtime_error("NIS vertex refers to a bone outside its palette");
                const float blend = weight > 1 ? weight / 256.0f : 0.0f;
                for (const auto bone : {first, second, first, first}) surface.bones.push_back(bone);
                for (const auto value : {1.0f - blend, blend, 0.0f, 0.0f}) surface.weights.push_back(value);
            }
            surface.uv.push_back({strip.textureCoordinates[i][0], strip.textureCoordinates[i][1]});
            const auto rgb555 = strip.colors[i];
            if (lightMaterial) {
                for (const float component : strip.normals[i]) surface.paintVertices.push_back(component);
                surface.paintVertices.push_back(rgb555);
            }
            // GS modulation uses 128 as unity. Texture alpha supplies opacity.
            surface.colors.push_back({
                std::min(rgb555 & 31u, 16u) / 16.0f,
                std::min((rgb555 >> 5) & 31u, 16u) / 16.0f,
                std::min((rgb555 >> 10) & 31u, 16u) / 16.0f,
                1.0f});
        }
        for (const auto index : strip.nativeTriangleOrder) {
            if (index >= strip.positions.size())
                throw std::runtime_error("Triangle index out of range for model " + std::to_string(model.key));
            surface.indices.push_back(start + index);
        }
    }

    Ref<ArrayMesh> mesh;
    mesh.instantiate();
    bool batchable = true;
    bool hasScroll = false, hasTextureSequence = false, hasPuddle = false;
    bool hasSortedSurfaces = false;
    int shaftSurfaces = 0;
    for (const auto& surface : surfaces) {
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = surface.vertices;
        arrays[Mesh::ARRAY_TEX_UV] = surface.uv;
        arrays[Mesh::ARRAY_COLOR] = surface.colors;
        arrays[Mesh::ARRAY_INDEX] = surface.indices;
        if (morph) arrays[Mesh::ARRAY_CUSTOM0] = surface.animationIndices;
        if (surface.lightMaterial) arrays[Mesh::ARRAY_CUSTOM0] = surface.paintVertices;
        if (boneCount) {
            arrays[Mesh::ARRAY_BONES] = surface.bones;
            arrays[Mesh::ARRAY_WEIGHTS] = surface.weights;
        }
        const auto index = mesh->get_surface_count();
        const auto format = morph ? Mesh::ARRAY_CUSTOM_R_FLOAT : Mesh::ARRAY_CUSTOM_RGBA_FLOAT;
        const auto flags = morph || surface.lightMaterial ? std::uint64_t(format) << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT : 0;
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays, {}, {}, flags);
        if (mesh->get_surface_count() != index + 1)
            throw std::runtime_error("Godot rejected a surface for model " + std::to_string(model.key));
        const auto* wire = surface.descriptor->authoredRecord;
        hasSortedSurfaces |= textureInfoForDescriptor(surface.descriptor)->requiresDepthSort;
        if (surface.lightMaterial) {
            auto& base = paintTextures_[surface.descriptor];
            if (base.is_null()) base = upload_paint_texture(surface.descriptor);
            if (base.is_null() || ((surface.paintFlags & 0x40u) && paint_->specular.is_null()))
                throw std::runtime_error("Car paint is missing an authored texture");
            Ref<ShaderMaterial> material;
            material.instantiate();
            material->set_shader(load_shader("res://shader/car_paint.gdshader"));
            material->set_shader_parameter("base_image", base);
            material->set_shader_parameter("specular_image", paint_->specular);
            material->set_shader_parameter("reflection_image", paint_->reflection);
            material->set_shader_parameter("paint_flags", int(surface.paintFlags));
            material->set_shader_parameter("frontend", paint_->frontend);
            material->set_shader_parameter("wheel", paintWheel_);
            material->set_shader_parameter("alpha_test", !wire[0x4f] &&
                !textureInfoForDescriptor(surface.descriptor)->drawFlag4Enabled);
            material->set_shader_parameter("repeat_axes", Vector2i(wire[0x4c] & 1u, (wire[0x4c] >> 1) & 1u));
            PackedVector4Array coefficients;
            const auto* values = surface.lightMaterial->NativeCoefficients();
            for (int i = 0; i < 16; i += 4)
                coefficients.push_back({values[i], values[i + 1], values[i + 2], values[i + 3]});
            material->set_shader_parameter("coefficients", coefficients);
            const auto& blend = nativeRenderPolicy.reflectionBlend;
            const bool overrideBlend = (surface.paintFlags & 4u) && !paint_->frontend;
            material->set_shader_parameter("reflection_selectors", overrideBlend ?
                Vector4i(blend[0], blend[1], blend[2], blend[3]) : Vector4i(0, 2, 0, 1));
            material->set_shader_parameter("reflection_fixed", float(overrideBlend ? blend[4] : paint_->frontend ? 64 : 16));
            mesh->surface_set_material(index, material);
            mesh->set_meta("hp2_car_paint", true);
            batchable = false;
            continue;
        }
        const auto textureKeyValue = surface.descriptor->key;
        // TEX1 has MXL=0, MMAG=1 and MMIN=3 on the original shaft textures.
        const bool shaft = is_light_shaft(textureKeyValue) && !wire[0x6e] && wire[0x6f] == 1 && wire[0x70] == 3;
        if (shaft) ++shaftSurfaces;
        hasPuddle |= textureKeyValue == 0x8bbf7d4eu || textureKeyValue == 0x79940f30u ||
            textureKeyValue == 0x0cca6e99u || textureKeyValue == 0x5ef002d2u ||
            textureKeyValue == 0x3cf05d44u || textureKeyValue == 0x6833b84fu || textureKeyValue == 0xcfff069du;
        const bool scroll = wire[0x79] != 0;
        const auto& sequence = texture_sequence(*surface.descriptor);
        const auto transparency = sequence.cutout ? BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR :
            surface.material->get_transparency();
        const auto depth = surface.material->get_depth_draw_mode();
        const auto blend = surface.material->get_blend_mode();
        const bool mixedWrap = (wire[0x4c] & 3u) == 1u || (wire[0x4c] & 3u) == 2u;
        batchable &= transparency == BaseMaterial3D::TRANSPARENCY_DISABLED &&
            depth != BaseMaterial3D::DEPTH_DRAW_DISABLED;
        hasScroll |= scroll;
        hasTextureSequence |= sequence.count != 0;
        if (!scroll && !sequence.count && !morph && !rigid && !mixedWrap && !shaft) {
            mesh->surface_set_material(index, surface.material);
            continue;
        }
        const auto textureKey = std::make_pair(surface.descriptor, depthWrite);
        if (!morph && !rigid) {
            const auto cached = animatedTextures_.find(textureKey);
            if (cached != animatedTextures_.end()) {
                mesh->surface_set_material(index, cached->second);
                continue;
            }
        }
        const char* blendName = blend == BaseMaterial3D::BLEND_MODE_ADD ? "add" :
            blend == BaseMaterial3D::BLEND_MODE_SUB ? "sub" : "mix";
        const char* coverage = transparency == BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR ? "cutout" :
            transparency == BaseMaterial3D::TRANSPARENCY_ALPHA ? "transparent" : "opaque";
        const char* depthName = depth == BaseMaterial3D::DEPTH_DRAW_DISABLED ? "read" : "write";
        const String shaderPath = String("res://shader/mesh_surface_") + coverage + "_" + blendName + "_" + depthName + ".gdshader";
        auto& shader = animatedShaders_[shaderPath];
        if (shader.is_null()) shader = load_shader(shaderPath);
        Ref<ShaderMaterial> material;
        material.instantiate();
        material->set_shader(shader);
        material->set_render_priority(surface.material->get_render_priority());
        material->set_shader_parameter("albedo_texture", surface.material->get_texture(BaseMaterial3D::TEXTURE_ALBEDO));
        material->set_shader_parameter("use_scroll", scroll);
        material->set_shader_parameter("use_shaft", shaft);
        material->set_shader_parameter("use_morph", morph != nullptr);
        material->set_shader_parameter("use_rigid", rigid != nullptr);
        if (shaft) {
            const auto albedo = surface.material->get_texture(BaseMaterial3D::TEXTURE_ALBEDO);
            material->set_shader_parameter("albedo_minified", albedo);
            material->set_shader_parameter("albedo_magnified", albedo);
        }
        material->set_shader_parameter("repeat_axes", Vector2i(wire[0x4c] & 1u, (wire[0x4c] >> 1) & 1u));
        if (morph || rigid) material->set_shader_parameter("animation_start", model.rigidAnimationStart);
        if (morph) {
            material->set_shader_parameter("vertex_frames", vertexFrames);
            material->set_shader_parameter("frame_stride", stride);
            material->set_shader_parameter("morph_count", morph->count);
            material->set_shader_parameter("morph_rate", static_cast<float>(morph->framesPerSecond));
        }
        if (rigid) {
            material->set_shader_parameter("rigid_frames", rigidFrames);
            material->set_shader_parameter("rigid_count", rigid->frameCount);
            material->set_shader_parameter("rigid_duration", static_cast<float>(rigid->frameDuration));
        }
        if (scroll) {
            material->set_shader_parameter("scroll_rate", Vector2(record_float(wire + 0x7c), record_float(wire + 0x80)));
            material->set_shader_parameter("scroll_interval", record_float(wire + 0x84));
            material->set_shader_parameter("stepped_scroll", wire[0x79] == 2);
        }
        if (sequence.count) {
            material->set_shader_parameter("frame_textures", sequence.texture);
            material->set_shader_parameter("texture_count", sequence.count);
            material->set_shader_parameter("texture_rate", static_cast<float>(sequence.rate));
            material->set_shader_parameter("real_clock", sequence.realClock);
        }
        mesh->surface_set_material(index, material);
        if (!morph && !rigid) animatedTextures_.emplace(textureKey, material);
    }
    mesh->set_meta("hp2_batchable", batchable);
    mesh->set_meta("hp2_sorted_surfaces", hasSortedSurfaces);
    mesh->set_meta("hp2_morph", morph != nullptr);
    mesh->set_meta("hp2_rigid", rigid != nullptr);
    mesh->set_meta("hp2_uv_scroll", hasScroll);
    mesh->set_meta("hp2_texture_sequence", hasTextureSequence);
    if (hasPuddle) mesh->set_meta("hp2_road_water", true);
    if (shaftSurfaces && shaftSurfaces == mesh->get_surface_count())
        mesh->set_meta("hp2_light_shaft", true);
    if (morph || rigid || hasScroll || hasTextureSequence)
        mesh->set_name(String::utf8(model.solid->name, std::find(model.solid->name,
            model.solid->name + sizeof(model.solid->name), '\0') - model.solid->name));
    if (hasBounds) {
        const AABB bounds(minimum, maximum - minimum);
        mesh->set_custom_aabb(bounds);
        mesh->set_meta("hp2_animation_bounds", bounds);
    }
    return mesh;
}

}
