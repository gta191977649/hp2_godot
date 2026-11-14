#include "mesh_converter.hpp"

#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Texture.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
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

constexpr auto animatedSurface = R"(
global uniform vec4 hp2_animation_clock; // interpolated ticks, texture seconds, real ticks, committed ticks
uniform ivec2 repeat_axes = ivec2(1);
#ifdef SHAFT
uniform sampler2D albedo_texture : source_color, filter_linear, repeat_enable;
uniform sampler2D albedo_minified : source_color, filter_nearest, repeat_enable;
#else
uniform sampler2D albedo_texture : source_color, filter_linear_mipmap, repeat_enable;
#endif
#ifdef FLIPBOOK
uniform sampler2DArray frame_textures : source_color, filter_linear_mipmap, repeat_enable;
uniform int texture_count;
uniform float texture_rate;
uniform bool real_clock;
#endif
#ifdef SCROLL
uniform vec2 scroll_rate;
uniform float scroll_interval;
uniform bool stepped_scroll;
#endif
#ifdef MORPH
uniform sampler2D vertex_frames : filter_nearest, repeat_disable;
uniform int frame_stride;
uniform int morph_count;
uniform float morph_rate;
vec4 sample_vertex(int frame, int vertex) {
    int i = frame * frame_stride + vertex;
    return texelFetch(vertex_frames, ivec2(i % 1024, i / 1024), 0);
}
#endif
#ifdef RIGID
uniform sampler2D rigid_frames : filter_nearest, repeat_disable;
uniform int rigid_count;
uniform float rigid_duration;
vec4 sample_rigid(int channel, int frame) {
    return texelFetch(rigid_frames, ivec2(channel, frame), 0);
}
#endif
#if defined(MORPH) || defined(RIGID)
uniform int animation_start;
instance uniform int hp2_animation_start = -1;
#endif

void vertex() {
#if defined(MORPH) || defined(RIGID)
    int start = hp2_animation_start >= 0 ? hp2_animation_start : animation_start;
#endif
#ifdef MORPH
    float phase = (hp2_animation_clock.x - float(start & 16383)) * morph_rate / 60.0;
    int frame = int(floor(phase));
    // Donor selects an unsigned remainder when the random start is ahead.
    int current = int(uint(frame % morph_count) % uint(morph_count));
    int next = int(uint((frame + 1) % morph_count) % uint(morph_count));
    int vertex = int(CUSTOM0.x);
    VERTEX = mix(sample_vertex(current, vertex).xyz, sample_vertex(next, vertex).xyz, fract(phase));
#endif
#ifdef RIGID
    if (start != 0) {
        float tick = float((int(hp2_animation_clock.x) - start) & 16383) + fract(hp2_animation_clock.x);
        float phase = tick / rigid_duration;
        bool loop = (start & 16384) != 0;
        if (loop || phase < float(rigid_count)) {
            int current = int(phase) % rigid_count;
            int next = loop ? (current + 1) % rigid_count : min(current + 1, rigid_count - 1);
            float weight = fract(phase);
            vec4 a = sample_rigid(0, current);
            vec4 b = sample_rigid(0, next);
            vec4 q = normalize(mix(a, dot(a, b) < 0.0 ? -b : b, weight));
            vec3 point = VERTEX * mix(sample_rigid(2, current).xyz, sample_rigid(2, next).xyz, weight);
            VERTEX = point + 2.0 * cross(q.xyz, cross(q.xyz, point) + q.w * point)
                + mix(sample_rigid(1, current).xyz, sample_rigid(1, next).xyz, weight);
        }
    }
#endif
#ifdef SCROLL
    float time = hp2_animation_clock.y;
    if (stepped_scroll) time = trunc(time / scroll_interval);
    vec2 offset = time * scroll_rate;
    UV += offset - trunc(offset);
#endif
}

void fragment() {
    vec2 margin = 0.5 / vec2(textureSize(albedo_texture, 0));
    vec2 uv = mix(clamp(UV, margin, 1.0-margin), UV, notEqual(repeat_axes, ivec2(0)));
#ifdef FLIPBOOK
    float clock = real_clock ? hp2_animation_clock.z : hp2_animation_clock.w;
    float frame = mod(trunc(texture_rate * (clock / 60.0)), float(texture_count));
    vec4 color = texture(frame_textures, vec3(uv, frame));
#else
#ifdef SHAFT
    vec2 texels = UV * vec2(textureSize(albedo_texture, 0));
    float footprint = max(length(dFdx(texels)), length(dFdy(texels)));
    vec4 color = footprint > 1.0 ? texture(albedo_minified, uv) : texture(albedo_texture, uv);
#else
    vec4 color = texture(albedo_texture, uv);
#endif
#endif
    vec3 tint = COLOR.rgb;
    if (!OUTPUT_IS_SRGB) {
        tint = mix(tint / 12.92, pow((tint + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), tint));
    }
    ALBEDO = color.rgb * tint;
#ifdef CUTOUT
    ALPHA = color.a;
    ALPHA_SCISSOR_THRESHOLD = 97.0 / 255.0;
#endif
#ifdef TRANSPARENT
    ALPHA = color.a;
#endif
}
)";

}

Vector3 race_vector(const float* source) {
    return {source[0], source[2], -source[1]};
}

Transform3D race_transform(const float* source) {
    return {Basis(race_vector(source), race_vector(source + 8), -race_vector(source + 4)),
        race_vector(source + 12)};
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
    bool canTriggerRigidAnimation) {
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
    };
    std::vector<Surface> surfaces;
    std::unordered_map<const EditorMarkerTexture*, std::size_t> surfaceIndices;
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
        const auto [entry, inserted] = surfaceIndices.emplace(descriptor, surfaces.size());
        if (inserted) {
            auto& surface = surfaces.emplace_back();
            surface.descriptor = descriptor;
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
    int shaftSurfaces = 0;
    for (const auto& surface : surfaces) {
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = surface.vertices;
        arrays[Mesh::ARRAY_TEX_UV] = surface.uv;
        arrays[Mesh::ARRAY_COLOR] = surface.colors;
        arrays[Mesh::ARRAY_INDEX] = surface.indices;
        if (morph) arrays[Mesh::ARRAY_CUSTOM0] = surface.animationIndices;
        if (boneCount) {
            arrays[Mesh::ARRAY_BONES] = surface.bones;
            arrays[Mesh::ARRAY_WEIGHTS] = surface.weights;
        }
        const auto index = mesh->get_surface_count();
        const auto flags = morph ? std::uint64_t(Mesh::ARRAY_CUSTOM_R_FLOAT) << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT : 0;
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays, {}, {}, flags);
        if (mesh->get_surface_count() != index + 1)
            throw std::runtime_error("Godot rejected a surface for model " + std::to_string(model.key));
        const auto* wire = surface.descriptor->authoredRecord;
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
        String source = "shader_type spatial;\nrender_mode unshaded, cull_disabled";
        if (blend == BaseMaterial3D::BLEND_MODE_ADD) source += ", blend_add";
        if (blend == BaseMaterial3D::BLEND_MODE_SUB) source += ", blend_sub";
        if (depth == BaseMaterial3D::DEPTH_DRAW_DISABLED) source += ", depth_draw_never";
        else if (transparency == BaseMaterial3D::TRANSPARENCY_ALPHA) source += ", depth_draw_always";
        source += ";\n";
        if (transparency == BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR) source += "#define CUTOUT\n";
        if (transparency == BaseMaterial3D::TRANSPARENCY_ALPHA) source += "#define TRANSPARENT\n";
        if (morph) source += "#define MORPH\n";
        if (rigid) source += "#define RIGID\n";
        if (scroll) source += "#define SCROLL\n";
        if (shaft) source += "#define SHAFT\n";
        if (sequence.count) source += "#define FLIPBOOK\n";
        source += animatedSurface;
        auto& shader = animatedShaders_[source];
        if (shader.is_null()) {
            shader.instantiate();
            shader->set_code(source);
        }
        Ref<ShaderMaterial> material;
        material.instantiate();
        material->set_shader(shader);
        material->set_render_priority(surface.material->get_render_priority());
        material->set_shader_parameter("albedo_texture", surface.material->get_texture(BaseMaterial3D::TEXTURE_ALBEDO));
        if (shaft) material->set_shader_parameter("albedo_minified",
            surface.material->get_texture(BaseMaterial3D::TEXTURE_ALBEDO));
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
