#include "mesh_converter.hpp"

#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Texture.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace hp2 {

using namespace godot;

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
    std::uint32_t width{}, height{};
    auto rgba = DecodeTextureMipRgba(*source, 0, 0, source->pixels.size, width, height);
    const auto* wire = descriptor.authoredRecord;
    const bool alphaTest = !wire[0x4f] && !source->drawFlag4Enabled;
    bool cutout = false;
    for (std::size_t alpha = 3; alpha < rgba.size(); alpha += 4) {
        cutout |= alphaTest && rgba[alpha] <= 48;
        rgba[alpha] = std::min(255, 2 * int(rgba[alpha]));
    }
    PackedByteArray bytes;
    bytes.resize(rgba.size());
    std::memcpy(bytes.ptrw(), rgba.data(), rgba.size());
    auto image = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, bytes);
    if (image.is_null() || image->generate_mipmaps() != OK)
        throw std::runtime_error("Godot could not build texture " + std::to_string(descriptor.key));
    const auto texture = ImageTexture::create_from_image(image);
    if (texture.is_null()) throw std::runtime_error("Godot could not allocate an ImageTexture");

    Ref<StandardMaterial3D> result;
    result.instantiate();
    result->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, texture);
    result->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS);
    result->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    result->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
    if (cutout) {
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
    materials_.emplace(&descriptor, result);
    return result;
}

Ref<ArrayMesh> MeshConverter::convert(const EditorMarkerModel& model, bool depthWrite) {
    if (!model.solid || !model.solid->geometry)
        throw std::runtime_error("Solid has no geometry for model " + std::to_string(model.key));
    struct Surface {
        Ref<StandardMaterial3D> material;
        PackedVector3Array vertices;
        PackedVector2Array uv;
        PackedInt32Array indices;
    };
    std::vector<Surface> surfaces;
    std::unordered_map<const EditorMarkerTexture*, std::size_t> surfaceIndices;
    const auto& geometry = *model.solid->geometry;
    for (const auto& strip : geometry.strips) {
        if ((strip.flags & 0x400u) || strip.nativeTriangleOrder.empty()) continue;
        if (strip.nativeTopologyCount != strip.positions.size() ||
            strip.nativeTopologyMask != strip.drawDisableMask ||
            strip.textureCoordinates.size() != strip.positions.size() ||
            strip.nativeTriangleOrder.size() % 3 ||
            strip.textureIndex < 0 || std::size_t(strip.textureIndex) >= geometry.textures.size())
            throw std::runtime_error("Invalid strip topology or texture slot for model " + std::to_string(model.key));
        const auto& binding = geometry.textures[strip.textureIndex];
        const auto* descriptor = binding.texture;
        for (int slot = 0; slot < model.replacementCount; ++slot) {
            const auto& replacement = model.replacement[slot];
            if (replacement.originalKey == binding.key && replacement.texture) {
                descriptor = replacement.texture;
                break;
            }
        }
        if (!descriptor) throw std::runtime_error("Missing texture binding for model " + std::to_string(model.key));
        const auto [entry, inserted] = surfaceIndices.emplace(descriptor, surfaces.size());
        if (inserted) {
            auto& surface = surfaces.emplace_back();
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
            surface.uv.push_back({strip.textureCoordinates[i][0], strip.textureCoordinates[i][1]});
        }
        for (const auto index : strip.nativeTriangleOrder) {
            if (index >= strip.positions.size())
                throw std::runtime_error("Triangle index out of range for model " + std::to_string(model.key));
            surface.indices.push_back(start + index);
        }
    }

    Ref<ArrayMesh> mesh;
    mesh.instantiate();
    for (const auto& surface : surfaces) {
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = surface.vertices;
        arrays[Mesh::ARRAY_TEX_UV] = surface.uv;
        arrays[Mesh::ARRAY_INDEX] = surface.indices;
        const auto index = mesh->get_surface_count();
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
        if (mesh->get_surface_count() != index + 1)
            throw std::runtime_error("Godot rejected a surface for model " + std::to_string(model.key));
        mesh->surface_set_material(index, surface.material);
    }
    return mesh;
}

}
