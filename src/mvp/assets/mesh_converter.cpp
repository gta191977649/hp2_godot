#include "mesh_converter.hpp"

#include "assets/shader_resource.hpp"
#include "assets/race_diagnostics.hpp"

#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Texture.hpp"
#include "indep/inc/eLight.hpp"
#include "port/not_implemented.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/skin.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
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

Ref<Image> authored_texture_image(const TextureInfo& source, int& minAlpha, int& maxAlpha) {
    const auto* wire = source.descriptor->authoredRecord;
    std::uint32_t width{}, height{};
    const auto levels = wire[0x6e];
    if (levels > 6) throw std::runtime_error("Invalid authored mip count");
    std::uint32_t baseSize{};
    std::memcpy(&baseSize, wire + 0x44, sizeof(baseSize));
    PackedByteArray bytes;
    std::size_t offset = 0;
    for (unsigned level = 0; level <= levels; ++level) {
        std::uint32_t w{}, h{};
        const auto size = levels ? baseSize >> (level * 2) : source.pixels.size;
        auto rgba = DecodeTextureMipRgba(source, level, offset, size, w, h);
        if (!level) { width = w; height = h; }
        for (std::size_t i = 3; i < rgba.size(); i += 4) {
            minAlpha = std::min(minAlpha, int(rgba[i]));
            maxAlpha = std::max(maxAlpha, int(rgba[i]));
        }
        const auto end = bytes.size();
        bytes.resize(end + rgba.size());
        std::memcpy(bytes.ptrw() + end, rgba.data(), rgba.size());
        offset += size;
    }
    // Godot expects storage down to 1x1; the shader clamps sampling to authored TEX1.MXL.
    if (levels) {
        std::int64_t fullSize = 0;
        auto w = width, h = height;
        while (true) {
            fullSize += std::int64_t(w) * h * 4;
            if (w == 1 && h == 1) break;
            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
        }
        bytes.resize(fullSize);
    }
    return Image::create_from_data(width, height, levels != 0, Image::FORMAT_RGBA8, bytes);
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

bool is_road_water(std::uint32_t key) {
    // W_RDWATER and RD_PUDDLE families, including their separate additive masks.
    return key == 0x8bbf7d4eu || key == 0x79940f30u || key == 0x0cca6e99u ||
        key == 0x5ef002d2u || key == 0x3cf05d44u || key == 0x6833b84fu || key == 0xcfff069du;
}

bool track_material(const std::uint8_t* wire, std::uint32_t flags) {
    return (flags & 0x40u) || wire[0x75] || wire[0x76] == 0x44 ||
        wire[0x76] == 0x48 || wire[0x76] == 0x42;
}

std::vector<const EditorMarkerTexture*> model_bindings(const EditorMarkerModel& model,
        const EditorMarkerSolidGeometry& geometry) {
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
    return bindings;
}

}

Vector3 race_vector(const float* source) {
    return {source[0], source[2], -source[1]};
}

Transform3D race_transform(const float* source) {
    return {Basis(race_vector(source), race_vector(source + 8), -race_vector(source + 4)),
        race_vector(source + 12)};
}

Skeleton3D* attach_pose_palette(MeshInstance3D& mesh, const String& name, std::size_t boneCount) {
    // The source palette already includes the inverse binds.
    auto* skeleton = memnew(Skeleton3D);
    skeleton->set_name(name);
    mesh.add_child(skeleton);
    Ref<Skin> skin;
    skin.instantiate();
    for (std::size_t bone = 0; bone < boneCount; ++bone) {
        skeleton->add_bone(String("Bone") + String::num_uint64(bone));
        skin->add_bind(static_cast<int>(bone), Transform3D());
    }
    mesh.set_skin(skin);
    mesh.set_skeleton_path(NodePath(name));
    return skeleton;
}

Vector4i paint_texture_sampling(const EditorMarkerTexture& descriptor) {
    const auto* wire = descriptor.authoredRecord;
    return {wire[0x4c] & 3, wire[0x6f], wire[0x70], wire[0x6e]};
}

Ref<Texture2D> upload_paint_texture(const EditorMarkerTexture* descriptor, bool authoredMips) {
    const auto* source = textureInfoForDescriptor(descriptor);
    if (!source || !source->pixels.Data()) return {};
    if (authoredMips) {
        int minAlpha = 255, maxAlpha = 0;
        auto texture = ImageTexture::create_from_image(authored_texture_image(*source, minAlpha, maxAlpha));
        texture->set_name(String::num_uint64(descriptor->key));
        return texture;
    }
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
        result->set_render_priority(source->requiresDepthSort ? std::min<int>(wire[0x29],7):-1);
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
    if (track_) {
        TypedArray<Image> raw;
        for (const auto& frame : frames) {
            int lo = 255, hi = 0;
            raw.push_back(authored_texture_image(frame, lo, hi));
        }
        sequence.raw.instantiate();
        if (sequence.raw->create_from_images(raw) != OK)
            throw std::runtime_error("Godot rejected original track texture frames");
    }
    return sequence;
}

const MeshConverter::TrackTexture& MeshConverter::track_texture(const EditorMarkerTexture& descriptor) {
    auto [it, inserted] = trackTextures_.try_emplace(&descriptor);
    auto& result = it->second;
    if (inserted) {
        const auto* source = textureInfoForDescriptor(&descriptor);
        if (!source || !source->pixels.Data()) throw std::runtime_error("Missing original track texture");
        const auto image = authored_texture_image(*source, result.minAlpha, result.maxAlpha);
        result.texture = ImageTexture::create_from_image(image);
        if (result.texture.is_null()) throw std::runtime_error("Godot rejected original track texture");
        result.texture->set_name(String::num_uint64(descriptor.key));
    }
    return result;
}

void MeshConverter::audit_track_model(const EditorMarkerModel& model) {
    if (!model.solid || !model.solid->geometry) return;
    const auto& geometry = *model.solid->geometry;
    const auto bindings = model_bindings(model, geometry);
    std::map<std::tuple<std::uint32_t, unsigned, int, int, int, int, int, int, int, bool>, int> states;
    for (const auto& strip : geometry.strips) {
        if ((strip.flags & 0x400u) || strip.nativeTriangleOrder.empty()) continue;
        const auto slot = std::size_t(strip.textureIndex);
        if (slot >= bindings.size() || !bindings[slot] || !textureInfoForDescriptor(bindings[slot])) {
            ++states[{slot < geometry.textures.size() ? geometry.textures[slot].key : 0, strip.flags,
                -1, -1, -1, -1, -1, -1, -1, false}];
            continue;
        }
        const auto* descriptor = bindings[slot];
        const auto* wire = descriptor->authoredRecord;
        ++states[{descriptor->key, strip.flags, wire[0x76], wire[0x75], wire[0x4d], wire[0x50], wire[0x4f],
            wire[0x74], wire[0x6e], descriptor != geometry.textures[slot].texture}];
    }
    for (const auto& [state, count] : states) {
        const auto& [key, flags, blend, bias, sort, depth, test, gloss, mip, replaced] = state;
        std::fprintf(stderr, "TRACK_MATERIAL model=%08x solid=%.24s lod=%d texture=%08x flags=%04x blend=%d bias=%d sort=%d depth=%d test=%d gloss=%d mip=%d replaced=%d strips=%d\n",
            model.key, model.solid->name, model.detailLevel, key, flags, blend, bias, sort, depth, test, gloss, mip, int(replaced), count);
    }
}

Ref<ArrayMesh> MeshConverter::convert(const EditorMarkerModel& model, bool depthWrite, std::size_t boneCount) {
    return convert(model, depthWrite, boneCount, false);
}

Ref<ArrayMesh> MeshConverter::convert_wheel_blur(const EditorMarkerModel& model) {
    MeshConverter converter;
    converter.wheelBlur_ = true;
    return converter.convert(model, false);
}

Ref<ArrayMesh> MeshConverter::convert_frontend_platform(const EditorMarkerModel& model) {
    MeshConverter converter;
    converter.frontendPlatform_ = true;
    return converter.convert(model, false);
}

Ref<ArrayMesh> MeshConverter::convert(const EditorMarkerModel& model, bool depthWrite, std::size_t boneCount,
    bool canTriggerRigidAnimation, ScenerySurface scenery) {
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
        PackedFloat32Array mipBias;
        const EditorMarkerLightMaterial* lightMaterial{};
        std::uint32_t paintFlags{};
        const EditorMarkerLightMaterial* roadMaterial{};
        bool gsTrack{};
    };
    std::vector<Surface> surfaces;
    std::map<std::tuple<const EditorMarkerTexture*, const EditorMarkerLightMaterial*, std::uint32_t,
        const EditorMarkerLightMaterial*, bool>, std::size_t>
        surfaceIndices;
    const bool billboard = scenery == ScenerySurface::ReflectionBillboard;
    const bool reflection = billboard || scenery == ScenerySurface::Reflection;
    const bool sky = scenery == ScenerySurface::Sky;
    const auto* morph = boneCount ? nullptr : model.solid->vertexAnimation;
    if (morph && morph->count < 2) morph = nullptr;
    const auto* rigid = boneCount ? nullptr : model.solid->rigidAnimation;
    if (rigid && ((!canTriggerRigidAnimation && model.rigidAnimationStart == 0) ||
            rigid->frameCount <= 0 || rigid->frameDuration <= 0))
        rigid = nullptr;
    const auto& geometry = *(morph ? morph->frames[0] : model.solid)->geometry;
    const auto* catalogue = model.resources && model.resources->catalogueRoot ?
        model.resources->catalogueRoot : model.resources;
    const auto authoredMaterial = [&](const auto& strip) -> const EditorMarkerLightMaterial* {
        if (strip.lightMaterialIndex >= 0 && std::size_t(strip.lightMaterialIndex) < geometry.lightMaterials.size())
            if (const auto* material = geometry.lightMaterials[strip.lightMaterialIndex]) return material;
        return catalogue ? catalogue->defaultLightMaterial : nullptr;
    };
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
    const auto bindings = model_bindings(model, geometry);
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
        const auto* wire = descriptor->authoredRecord;
        const bool prelitCopy = !sky && wire[0x76] == 0x0a && wire[0x4e] == 0 && !(strip.flags & 0x180u);
        const bool gsTrack = track_ && (track_material(wire, strip.flags) || prelitCopy);
        const bool transparent = wire[0x76] == 0x44 || wire[0x76] == 0x48 || wire[0x76] == 0x42;
        if (scenery != ScenerySurface::Full && !sky && texture && (texture->requiresDepthSort ||
            (reflection && (gsTrack ? transparent : material(*descriptor)->get_transparency() == BaseMaterial3D::TRANSPARENCY_ALPHA) &&
                !texture_sequence(*descriptor).cutout))) continue;
        const EditorMarkerLightMaterial* roadMaterial = nullptr;
        if (gsTrack && (strip.flags & 0x40u)) {
            roadMaterial = authoredMaterial(strip);
            if (!roadMaterial || strip.normals.size() != strip.positions.size())
                throw std::runtime_error("Track strip 0x40 is missing its original lighting inputs");
            if (roadMaterial->inGameVariant) roadMaterial = roadMaterial->inGameVariant.get();
        }
        const EditorMarkerLightMaterial* lightMaterial = nullptr;
        std::uint32_t paintFlags = 0;
        const bool previewGlass = paint_ && paint_->frontend && wire[0x76] == 0x44 && !wire[0x50];
        if (paint_ && texture && (!texture->requiresDepthSort || previewGlass) && !morph && !rigid &&
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
        const auto key = std::make_tuple(descriptor, lightMaterial, paintFlags, roadMaterial, gsTrack);
        const auto [entry, inserted] = surfaceIndices.emplace(key, surfaces.size());
        if (inserted) {
            auto& surface = surfaces.emplace_back();
            surface.descriptor = descriptor;
            surface.lightMaterial = lightMaterial;
            surface.paintFlags = paintFlags;
            surface.roadMaterial = roadMaterial;
            surface.gsTrack = gsTrack;
            if (!gsTrack) surface.material = material(*descriptor);
            if (!gsTrack && !depthWrite) {
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
                    throw std::runtime_error("Skinned vertex refers to a bone outside its palette");
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
            if (roadMaterial) {
                for (const float component : strip.normals[i]) surface.paintVertices.push_back(component);
            }
            if (gsTrack || frontendPlatform_ || (lightMaterial && paint_->frontend)) {
                // SetMipMapK (0011C328) sends the low byte of CalcAutoK per strip.
                surface.mipBias.push_back(static_cast<std::uint8_t>(strip.autoK) / 16.f);
            }
            if (gsTrack || sky || frontendPlatform_) {
                surface.colors.push_back({float((rgb555 & 31u) * 8) / 255.f,
                    float(((rgb555 >> 5) & 31u) * 8) / 255.f, float(((rgb555 >> 10) & 31u) * 8) / 255.f,
                    (rgb555 & 0x8000u) ? 128.f / 255.f : 0.f});
                continue;
            }
            if (wheelBlur_) {
                // Preserve all five authored bits through Godot's UNORM vertex color.
                surface.colors.push_back({(rgb555 & 31u) / 31.0f, ((rgb555 >> 5) & 31u) / 31.0f,
                    ((rgb555 >> 10) & 31u) / 31.0f, 1.0f});
                continue;
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
    std::map<const Surface*, const Surface*> wetMasks;
    std::set<const Surface*> mergedMasks;
    if (depthWrite && !morph && !rigid && !boneCount && !sky && !reflection) {
        for (const auto& base : surfaces) {
            const auto key = base.descriptor->key;
            if (!base.gsTrack || base.roadMaterial || (key != 0x8bbf7d4eu && key != 0x79940f30u))
                continue;
            const auto* wire = base.descriptor->authoredRecord;
            if (wire[0x29] != 2 || wire[0x4e] || !wire[0x4f] || !wire[0x50] ||
                wire[0x75] || wire[0x76] != 0x44 || wire[0x79] ||
                textureInfoForDescriptor(base.descriptor)->requiresDepthSort ||
                texture_sequence(*base.descriptor).count || !track_texture(*base.descriptor).minAlpha)
                continue;
            const auto fullVertexAlpha = [](const PackedColorArray& colors) {
                return std::all_of(colors.ptr(), colors.ptr() + colors.size(),
                    [](const Color& color) { return color.a == 128.f / 255.f; });
            };
            if (!fullVertexAlpha(base.colors)) continue;
            for (const auto& mask : surfaces) {
                if (!mask.gsTrack || !mask.roadMaterial || mask.descriptor->key != 0x0cca6e99u ||
                    mergedMasks.count(&mask))
                    continue;
                const auto* state = mask.descriptor->authoredRecord;
                if (state[0x29] != 3 || state[0x4e] || !state[0x4f] || state[0x50] ||
                    state[0x6e] || state[0x6f] != 1 || state[0x70] != 3 || state[0x71] ||
                    state[0x75] || state[0x76] != 0x48 || state[0x79] ||
                    textureInfoForDescriptor(mask.descriptor)->requiresDepthSort ||
                    texture_sequence(*mask.descriptor).count)
                    continue;
                // The source-over strip writes depth and the next bucket's additive
                // strip tests equal depth. Share a fragment only for identical triangles.
                if (base.vertices != mask.vertices || base.uv != mask.uv || base.indices != mask.indices)
                    continue;
                wetMasks.emplace(&base, &mask);
                mergedMasks.insert(&mask);
                break;
            }
        }
    }
    bool batchable = true;
    bool hasScroll = false, hasTextureSequence = false, hasPuddle = false;
    bool hasSortedSurfaces = false;
    bool needsReflectionFilter = false;
    int shaftSurfaces = 0;
    for (const auto& surface : surfaces) {
        if (mergedMasks.count(&surface)) continue;
        const auto paired = wetMasks.find(&surface);
        const auto* wetMask = paired != wetMasks.end() ? paired->second : nullptr;
        const auto textureKeyValue = surface.descriptor->key;
        const auto* wire = surface.descriptor->authoredRecord;
        // The original shaft textures use TEX1.MXL=0, MMAG=1 and MMIN=3.
        const bool shaft = is_light_shaft(textureKeyValue) && !wire[0x6e] && wire[0x6f] == 1 && wire[0x70] == 3;
        if (shaft && profile_grid_effect_disabled("no-shafts")) continue;
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = surface.vertices;
        arrays[Mesh::ARRAY_TEX_UV] = surface.uv;
        arrays[Mesh::ARRAY_COLOR] = surface.colors;
        arrays[Mesh::ARRAY_INDEX] = surface.indices;
        if (morph) arrays[Mesh::ARRAY_CUSTOM0] = surface.animationIndices;
        if (surface.lightMaterial) arrays[Mesh::ARRAY_CUSTOM0] = surface.paintVertices;
        if (surface.roadMaterial || wetMask)
            arrays[Mesh::ARRAY_CUSTOM1] = wetMask ? wetMask->paintVertices : surface.paintVertices;
        if (!surface.mipBias.is_empty()) arrays[Mesh::ARRAY_CUSTOM2] = surface.mipBias;
        if (wetMask) {
            PackedFloat32Array tint;
            for (int vertex = 0; vertex < wetMask->colors.size(); ++vertex) {
                const auto& color = wetMask->colors[vertex];
                for (const float channel : {color.r, color.g, color.b, wetMask->mipBias[vertex]})
                    tint.push_back(channel);
            }
            arrays[Mesh::ARRAY_CUSTOM3] = tint;
        }
        if (boneCount) {
            arrays[Mesh::ARRAY_BONES] = surface.bones;
            arrays[Mesh::ARRAY_WEIGHTS] = surface.weights;
        }
        const auto index = mesh->get_surface_count();
        const auto format = morph ? Mesh::ARRAY_CUSTOM_R_FLOAT : Mesh::ARRAY_CUSTOM_RGBA_FLOAT;
        auto flags = morph || surface.lightMaterial ? std::uint64_t(format) << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT : 0;
        if (surface.roadMaterial || wetMask)
            flags |= std::uint64_t(Mesh::ARRAY_CUSTOM_RGB_FLOAT) << Mesh::ARRAY_FORMAT_CUSTOM1_SHIFT;
        if (!surface.mipBias.is_empty()) flags |= std::uint64_t(Mesh::ARRAY_CUSTOM_R_FLOAT) << Mesh::ARRAY_FORMAT_CUSTOM2_SHIFT;
        if (wetMask) flags |= std::uint64_t(Mesh::ARRAY_CUSTOM_RGBA_FLOAT) << Mesh::ARRAY_FORMAT_CUSTOM3_SHIFT;
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays, {}, {}, flags);
        if (mesh->get_surface_count() != index + 1)
            throw std::runtime_error("Godot rejected a surface for model " + std::to_string(model.key));
        hasSortedSurfaces |= textureInfoForDescriptor(surface.descriptor)->requiresDepthSort;
        if (frontendPlatform_) {
            auto& base = paintTextures_[surface.descriptor];
            if (base.is_null()) base = upload_paint_texture(surface.descriptor, true);
            Ref<ShaderMaterial> material;
            material.instantiate();
            material->set_shader(load_shader("res://shader/frontend_platform.gdshader"));
            material->set_render_priority(wire[0x29]);
            material->set_shader_parameter("base_image", base);
            material->set_shader_parameter("base_sampling", paint_texture_sampling(*surface.descriptor));
            material->set_shader_parameter("base_lod_scale", float(1 << wire[0x71]));
            mesh->surface_set_material(index, material);
            mesh->set_meta("hp2_frontend_platform", true);
            batchable = false;
            continue;
        }
        if (wheelBlur_) {
            auto& texture = paintTextures_[surface.descriptor];
            if (texture.is_null()) texture = upload_paint_texture(surface.descriptor);
            if (texture.is_null()) throw std::runtime_error("Missing authored wheel blur texture");
            Ref<ShaderMaterial> overlay;
            overlay.instantiate();
            overlay->set_shader(load_shader("res://shader/wheel_blur.gdshader"));
            overlay->set_shader_parameter("wheel_blur_texture", texture);
            overlay->set_shader_parameter("repeat_axes", Vector2i(wire[0x4c] & 1u, (wire[0x4c] >> 1) & 1u));
            overlay->set_shader_parameter("alpha_test", !wire[0x4f] &&
                !textureInfoForDescriptor(surface.descriptor)->drawFlag4Enabled);
            overlay->set_render_priority(surface.material->get_render_priority());
            mesh->surface_set_material(index, overlay);
            batchable = false;
            continue;
        }
        if (surface.lightMaterial) {
            auto& base = paintTextures_[surface.descriptor];
            if (base.is_null()) base = upload_paint_texture(surface.descriptor, paint_->frontend);
            if (base.is_null() || ((surface.paintFlags & 0x40u) && paint_->specular.is_null()))
                throw std::runtime_error("Car paint is missing an authored texture");
            Ref<ShaderMaterial> material;
            material.instantiate();
            const bool glass = paint_->frontend && textureInfoForDescriptor(surface.descriptor)->requiresDepthSort;
            material->set_shader(load_shader(glass ? "res://shader/car_paint_glass.gdshader" :
                "res://shader/car_paint.gdshader"));
            if (glass) material->set_render_priority(std::min<int>(wire[0x29], 7));
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
            if (paint_->frontend) {
                // BuildDmaData 00116828 compiles the authored exponent pairs
                // into the parameters used by the VU vertex power curve.
                const auto intercept = [](float slope, float amplitude) {
                    if (amplitude == 0.0f) return 0.0f;
                    std::int32_t bits{};
                    std::memcpy(&bits, &amplitude, sizeof(bits));
                    return float(bits) - slope * 1065353216.0f;
                };
                material->set_shader_parameter("frontend_power", Vector4(
                    values[1] == 0.0f ? 0.0f : values[0], intercept(values[0], values[1]),
                    values[13] == 0.0f ? 0.0f : values[12], intercept(values[12], values[13])));
                material->set_shader_parameter("base_sampling", paint_texture_sampling(*surface.descriptor));
                material->set_shader_parameter("specular_sampling", paint_->specularSampling);
                material->set_shader_parameter("reflection_sampling", paint_->reflectionSampling);
                material->set_shader_parameter("base_lod_scale", float(1 << wire[0x71]));
                material->set_shader_parameter("base_tfx", int(wire[0x4e]));
            }
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
        if (shaft) ++shaftSurfaces;
        const bool puddle = is_road_water(textureKeyValue);
        hasPuddle |= puddle;
        const bool scroll = wire[0x79] != 0;
        const auto& sequence = texture_sequence(*surface.descriptor);
        if (surface.gsTrack) {
            const auto* source = textureInfoForDescriptor(surface.descriptor);
            const auto& raw = track_texture(*surface.descriptor);
            const bool alphaTest = !wire[0x4f] && !source->drawFlag4Enabled;
            const auto blend = wire[0x76];
            const bool transparent = blend == 0x44 || blend == 0x48 || blend == 0x42 || source->requiresDepthSort;
            const bool write = depthWrite && wire[0x50];
            needsReflectionFilter |= transparent;
            batchable &= !transparent && write;
            hasScroll |= scroll;
            hasTextureSequence |= sequence.count != 0;
            const bool cacheable = !morph && !rigid && !surface.roadMaterial && !wetMask;
            const auto cacheKey = std::make_tuple(surface.descriptor, depthWrite, billboard, true);
            if (cacheable) {
                const auto cached = animatedTextures_.find(cacheKey);
                if (cached != animatedTextures_.end()) {
                    mesh->surface_set_material(index, cached->second);
                    continue;
                }
            }
            const char* mode = !transparent ? (!wire[0x75] && !morph && !rigid ? "copy_static" : "copy") :
                blend == 0x48 ? "add" : blend == 0x42 ? "sub" : "mix";
            const bool waterBlend = puddle && !source->requiresDepthSort && wire[0x4e] == 0 &&
                ((blend == 0x44 && write && !wire[0x75]) || (blend == 0x48 && !write));
            const String path = String(waterBlend ? "res://shader/road_water_" : "res://shader/track_material_") +
                mode + (write ? "_write" : "_read") +
                (wire[0x75] ? "_biased.gdshader" : ".gdshader");
            auto& shader = animatedShaders_[path];
            if (shader.is_null()) shader = load_shader(path);
            Ref<ShaderMaterial> draw;
            draw.instantiate();
            draw->set_shader(shader);
            if (waterBlend && blend == 0x44) draw->set_meta("hp2_wet_road", true);
            // Authored projected shadows in bucket 4 follow the wet asphalt and
            // its bucket-3 mask. Drawing them first loses their road coverage.
            const bool projectedShadow = blend == 0x44 && !write && wire[0x75] == 1 && wire[0x29] == 4;
            draw->set_render_priority(source->requiresDepthSort || waterBlend || projectedShadow ?
                std::min<int>(wire[0x29], 7) : -1);
            draw->set_shader_parameter("gs_texture_linear", raw.texture);
            draw->set_shader_parameter("gs_texture_nearest", raw.texture);
            draw->set_shader_parameter("gs_test", alphaTest);
            draw->set_shader_parameter("gs_copy", blend == 0x0a);
            // The authored shaft family uses priority 5 and this base-level TEX1 state.
            draw->set_shader_parameter("gs_dark_blend", blend == 0x44 && wire[0x29] == 5 && !wire[0x50] &&
                !wire[0x6e] && wire[0x6f] == 1 && wire[0x70] == 3);
            draw->set_shader_parameter("gs_bias", int(wire[0x75]));
            draw->set_shader_parameter("gs_filter", Vector4i(wire[0x6e], wire[0x6f], wire[0x70], wire[0x71]));
            draw->set_shader_parameter("repeat_axes", Vector2i(wire[0x4c] & 1u, (wire[0x4c] >> 1) & 1u));
            draw->set_shader_parameter("use_scroll", scroll);
            draw->set_shader_parameter("use_morph", morph != nullptr);
            draw->set_shader_parameter("use_rigid", rigid != nullptr);
            draw->set_shader_parameter("face_camera", billboard);
            if (morph || rigid) draw->set_shader_parameter("animation_start", model.rigidAnimationStart);
            if (morph) {
                draw->set_shader_parameter("vertex_frames", vertexFrames);
                draw->set_shader_parameter("frame_stride", stride);
                draw->set_shader_parameter("morph_count", morph->count);
                draw->set_shader_parameter("morph_rate", float(morph->framesPerSecond));
            }
            if (rigid) {
                draw->set_shader_parameter("rigid_frames", rigidFrames);
                draw->set_shader_parameter("rigid_count", rigid->frameCount);
                draw->set_shader_parameter("rigid_duration", float(rigid->frameDuration));
            }
            if (scroll) {
                draw->set_shader_parameter("scroll_rate", Vector2(record_float(wire + 0x7c), record_float(wire + 0x80)));
                draw->set_shader_parameter("scroll_interval", record_float(wire + 0x84));
                draw->set_shader_parameter("stepped_scroll", wire[0x79] == 2);
            }
            if (sequence.count) {
                draw->set_shader_parameter("gs_frames_linear", sequence.raw);
                draw->set_shader_parameter("gs_frames_nearest", sequence.raw);
                draw->set_shader_parameter("texture_count", sequence.count);
                draw->set_shader_parameter("texture_rate", float(sequence.rate));
                draw->set_shader_parameter("real_clock", sequence.realClock);
            }
            const auto* roadMaterial = wetMask ? wetMask->roadMaterial : surface.roadMaterial;
            if (roadMaterial) {
                auto* registry = catalogue ? catalogue->resourceRegistry : nullptr;
                const auto* falloff = registry ? registry->SpecialTexture(6) : nullptr;
                if (!falloff || (!wetMask && blend != 0x0a && blend != 0x48) || sequence.count)
                    throw NotImplemented("0010AC50", "Unsupported track strip 0x40 state or missing SPECULARFALLOFF");
                const auto& specular = track_texture(*falloff);
                // TEST coverage agrees on the two kicks if the alpha test is always
                // passing, or both textures have the same constant alpha at all mips.
                if (alphaTest && (raw.minAlpha != raw.maxAlpha || specular.minAlpha != specular.maxAlpha ||
                    raw.minAlpha != specular.minAlpha))
                    throw NotImplemented("0010AC50", "Track strip 0x40 needs destination alpha feedback");
                const auto* c = roadMaterial->NativeCoefficients();
                draw->set_shader_parameter("gs_specular", specular.texture);
                draw->set_shader_parameter("gs_road", !wetMask);
                draw->set_shader_parameter("gs_road_alpha", Vector4(c[12], c[13], c[14], c[15]));
                draw->set_shader_parameter("gs_road_min_dot", c[2]);
                const auto& sun = registry->Lights().sunlightDirection;
                draw->set_shader_parameter("gs_sun", race_vector(sun.data()) * 10000.f);
                if (wetMask) {
                    const auto& mask = track_texture(*wetMask->descriptor);
                    const auto repeat = wetMask->descriptor->authoredRecord[0x4c];
                    draw->set_shader_parameter("gs_has_wet_mask", true);
                    draw->set_shader_parameter("gs_wet_mask_linear", mask.texture);
                    draw->set_shader_parameter("gs_wet_mask_nearest", mask.texture);
                    draw->set_shader_parameter("gs_wet_mask_repeat", Vector2i(repeat & 1u, (repeat >> 1) & 1u));
                }
            }
            draw->set_meta("hp2_texture_key", std::int64_t(textureKeyValue));
            mesh->surface_set_material(index, draw);
            if (cacheable) animatedTextures_.emplace(cacheKey, draw);
            continue;
        }
        const auto transparency = sequence.cutout ? BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR :
            surface.material->get_transparency();
        needsReflectionFilter |= transparency == BaseMaterial3D::TRANSPARENCY_ALPHA;
        const auto depth = surface.material->get_depth_draw_mode();
        const auto blend = surface.material->get_blend_mode();
        const bool mixedWrap = (wire[0x4c] & 3u) == 1u || (wire[0x4c] & 3u) == 2u;
        batchable &= transparency == BaseMaterial3D::TRANSPARENCY_DISABLED &&
            depth != BaseMaterial3D::DEPTH_DRAW_DISABLED;
        hasScroll |= scroll;
        hasTextureSequence |= sequence.count != 0;
        if (!scroll && !sequence.count && !morph && !rigid && !mixedWrap && !shaft && !billboard && !sky) {
            mesh->surface_set_material(index, surface.material);
            continue;
        }
        const auto textureKey = std::make_tuple(surface.descriptor, depthWrite, billboard, false);
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
        const String shaderPath = sky
            ? String("res://shader/track_sky_") + blendName + ".gdshader"
            : String("res://shader/mesh_surface_") + coverage + "_" + blendName + "_" + depthName + ".gdshader";
        auto& shader = animatedShaders_[shaderPath];
        if (shader.is_null()) shader = load_shader(shaderPath);
        Ref<ShaderMaterial> material;
        material.instantiate();
        material->set_shader(shader);
        material->set_render_priority(sky
            ? Material::RENDER_PRIORITY_MIN + (transparency == BaseMaterial3D::TRANSPARENCY_ALPHA ? 1 : 0)
            : surface.material->get_render_priority());
        material->set_shader_parameter("albedo_texture", sky ? track_texture(*surface.descriptor).texture :
            surface.material->get_texture(BaseMaterial3D::TEXTURE_ALBEDO));
        material->set_shader_parameter("use_scroll", scroll);
        material->set_shader_parameter("use_shaft", shaft);
        if (sky) {
            material->set_shader_parameter("sky_blend_alpha", transparency == BaseMaterial3D::TRANSPARENCY_ALPHA);
            material->set_shader_parameter("sky_cutout", transparency == BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
        }
        material->set_shader_parameter("use_morph", morph != nullptr);
        material->set_shader_parameter("use_rigid", rigid != nullptr);
        if (billboard) material->set_shader_parameter("face_camera", true);
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
            material->set_shader_parameter("frame_textures", sky ? sequence.raw : sequence.texture);
            material->set_shader_parameter("texture_count", sequence.count);
            material->set_shader_parameter("texture_rate", static_cast<float>(sequence.rate));
            material->set_shader_parameter("real_clock", sequence.realClock);
        }
        mesh->surface_set_material(index, material);
        if (!morph && !rigid) animatedTextures_.emplace(textureKey, material);
    }
    mesh->set_meta("hp2_batchable", batchable);
    mesh->set_meta("hp2_sorted_surfaces", hasSortedSurfaces);
    mesh->set_meta("hp2_reflection_surface_filter", hasSortedSurfaces || needsReflectionFilter);
    mesh->set_meta("hp2_morph", morph != nullptr);
    mesh->set_meta("hp2_rigid", rigid != nullptr);
    mesh->set_meta("hp2_uv_scroll", hasScroll);
    mesh->set_meta("hp2_texture_sequence", hasTextureSequence);
    if (hasPuddle) mesh->set_meta("hp2_road_water", true);
    if (shaftSurfaces && shaftSurfaces == mesh->get_surface_count())
        mesh->set_meta("hp2_light_shaft", true);
    if (track_ || morph || rigid || hasScroll || hasTextureSequence)
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
