#include "track_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "indep/inc/Scenery.hpp"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/os.hpp>

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace hp2 {

using namespace godot;

namespace {

bool can_batch(const ArrayMesh& mesh) {
    for (int surface = 0; surface < mesh.get_surface_count(); ++surface) {
        const Ref<BaseMaterial3D> material = mesh.surface_get_material(surface);
        if (material.is_null() || material->get_transparency() != BaseMaterial3D::TRANSPARENCY_DISABLED ||
            material->get_depth_draw_mode() == BaseMaterial3D::DEPTH_DRAW_DISABLED)
            return false;
    }
    return true;
}

void attach_placement(Node3D& root, const Ref<ArrayMesh>& mesh, const Transform3D& transform) {
    auto* node = memnew(MeshInstance3D);
    root.add_child(node);
    node->set_mesh(mesh);
    node->set_transform(transform);
}

struct PlacementBatch {
    Ref<ArrayMesh> mesh;
    std::vector<Transform3D> transforms;
};

void attach_batch(Node3D& root, const PlacementBatch& batch) {
    if (batch.transforms.size() == 1) {
        attach_placement(root, batch.mesh, batch.transforms.front());
        return;
    }
    Ref<MultiMesh> instances;
    instances.instantiate();
    instances->set_transform_format(MultiMesh::TRANSFORM_3D);
    instances->set_mesh(batch.mesh);
    instances->set_instance_count(static_cast<int>(batch.transforms.size()));
    const AABB meshBounds = batch.mesh->get_aabb();
    AABB bounds = batch.transforms.front().xform(meshBounds);
    for (std::size_t i = 0; i < batch.transforms.size(); ++i) {
        const auto& transform = batch.transforms[i];
        instances->set_instance_transform(static_cast<int>(i), transform);
        bounds.merge_with(transform.xform(meshBounds));
    }
    instances->set_custom_aabb(bounds);
    auto* node = memnew(MultiMeshInstance3D);
    root.add_child(node);
    node->set_multimesh(instances);
}

struct PlacementMesh {
    Ref<ArrayMesh> mesh;
    bool batchable{};
};

}

std::size_t attach_track(Node3D& root, const Scenery& scenery, const SkyResources* sky) {
    MeshConverter converter;
    const bool batchScenery = OS::get_singleton()->get_environment("HP2_MVP_SCENERY_BATCHING") != "0";
    std::unordered_map<const EditorMarkerModel*, PlacementMesh> meshes;
    std::size_t placements{};
    for (std::size_t index = 0; index < scenery.loadedSectionCount; ++index) {
        const auto* section = scenery.loadedSections[index];
        if (!section) throw std::runtime_error("Loaded section table contains an empty entry");
        std::unordered_map<const EditorMarkerModel*, std::size_t> batchIndices;
        std::vector<PlacementBatch> batches;
        for (std::size_t item = 0; item < section->instanceCount; ++item) {
            const auto& instance = section->instances[item];
            // Single-player primary view, forward route, as in StuffScenery11321C.
            if (instance.flags & 0x19u) continue;
            if (instance.modelIndex < 0 || std::size_t(instance.modelIndex) >= section->modelCount)
                throw std::runtime_error("Invalid scenery model index in section " + std::to_string(section->number));
            const auto& model = section->models[instance.modelIndex];
            const EditorMarkerModel* level = nullptr;
            for (const auto& candidate : model.levels) {
                if (candidate.solid) {
                    level = &candidate;
                    break;
                }
            }
            if (!level) continue; // Original selection rejects placements without an authored LOD.
            auto& entry = meshes[level];
            const auto& mesh = entry.mesh;
            if (mesh.is_null()) {
                entry.mesh = converter.convert(*level);
                entry.batchable = batchScenery && can_batch(*mesh.ptr());
            }
            if (!mesh->get_surface_count()) continue; // All strips can be non-drawing.
            Transform3D transform;
            if (!(instance.flags & 0x200u)) {
                float frame[16]{};
                for (int lane = 0; lane < 9; ++lane)
                    frame[(lane / 3) * 4 + lane % 3] = instance.rotation[lane] * (1.0f / 16384.0f);
                std::copy_n(instance.position, 3, frame + 12);
                frame[15] = 1;
                transform = race_transform(frame);
            }
            // A MultiMesh has one renderer instance and one culling bound.
            // Keep section boundaries and preserve per-instance winding for mirrored placements.
            if (entry.batchable && transform.basis.determinant() > 0) {
                const auto [batch, inserted] = batchIndices.try_emplace(level, batches.size());
                if (inserted) batches.push_back({mesh, {}});
                batches[batch->second].transforms.push_back(transform);
            } else {
                attach_placement(root, mesh, transform);
            }
            ++placements;
        }
        for (const auto& batch : batches) attach_batch(root, batch);
    }
    if (sky && sky->Model(0).solid) {
        const auto mesh = converter.convert(sky->Model(0), false);
        if (!mesh->get_surface_count()) throw std::runtime_error("Authored sky has no drawable surfaces");
        auto* node = memnew(MeshInstance3D);
        root.add_child(node);
        node->set_mesh(mesh);
        node->set_transform(race_transform(sky->Transform(0)));
    }
    if (!placements) throw std::runtime_error("No drawable track placements");
    return placements;
}

}
