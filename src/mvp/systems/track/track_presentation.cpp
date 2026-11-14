#include "track_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/Smokeable.hpp"
#include "bridge/presentation_snapshot.hpp"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/os.hpp>

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hp2 {

using namespace godot;

namespace {

bool can_batch(const ArrayMesh& mesh) {
    return mesh.get_meta("hp2_batchable", false);
}

MeshInstance3D& attach_placement(Node3D& root, const Ref<ArrayMesh>& mesh, const Transform3D& transform) {
    auto* node = memnew(MeshInstance3D);
    root.add_child(node);
    node->set_mesh(mesh);
    node->set_transform(transform);
    return *node;
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
    const AABB meshBounds = batch.mesh->has_meta("hp2_animation_bounds") ?
        AABB(batch.mesh->get_meta("hp2_animation_bounds")) : batch.mesh->get_aabb();
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

struct WorldObjectPresentation::Storage {
    struct Instance {
        MeshInstance3D* node{};
        const EditorMarkerSolid* solid{};
        std::uint16_t animationStart{};
        std::uint64_t lastSeen{};
    };
    struct MeshAsset {
        const EditorMarkerSolid* solid{};
        const EditorMarkerReplacement* replacement{};
        int detailLevel{};
        bool operator==(const MeshAsset& other) const noexcept {
            return solid == other.solid && replacement == other.replacement && detailLevel == other.detailLevel;
        }
    };
    struct MeshAssetHash {
        std::size_t operator()(const MeshAsset& key) const noexcept {
            return std::hash<const void*>{}(key.solid) ^
                (std::hash<const void*>{}(key.replacement) << 1) ^ std::hash<int>{}(key.detailLevel);
        }
    };

    explicit Storage(Node3D& scene) : root(*memnew(Node3D)) {
        root.set_name("WorldObjects");
        scene.add_child(&root);
    }

    const Ref<ArrayMesh>& mesh_for(const EditorMarkerModel& model) {
        const MeshAsset key{model.solid, model.replacement, model.detailLevel};
        auto [mesh, inserted] = meshes.try_emplace(key);
        if (inserted) mesh->second = converter.convert(model, true, 0, true);
        return mesh->second;
    }

    Node3D& root;
    MeshConverter converter;
    std::unordered_map<MeshAsset, Ref<ArrayMesh>, MeshAssetHash> meshes;
    std::unordered_map<std::uint64_t, Instance> instances;
    std::uint64_t serial{};
};

WorldObjectPresentation::WorldObjectPresentation(Node3D& scene)
    : storage_(std::make_unique<Storage>(scene)) {}

WorldObjectPresentation::~WorldObjectPresentation() = default;

void WorldObjectPresentation::prepare_animation(const EditorMarkerModel& model) {
    storage_->mesh_for(model);
}

void WorldObjectPresentation::update(const std::vector<WorldObjectEntityState>& objects) {
    auto& storage = *storage_;
    ++storage.serial;
    for (const auto& object : objects) {
        auto found = storage.instances.find(object.handle);
        if (!object.visible || !object.model->solid) {
            if (found != storage.instances.end()) {
                found->second.lastSeen = storage.serial;
                found->second.node->hide();
            }
            continue;
        }

        const auto& model = *object.model;
        if (found == storage.instances.end()) {
            auto* node = memnew(MeshInstance3D);
            const auto* name = model.solid->name;
            const auto length = std::find(name, name + sizeof(model.solid->name), '\0') - name;
            node->set_name(String::utf8(name, length).validate_node_name() + "_" + String::num_uint64(object.handle));
            storage.root.add_child(node);
            found = storage.instances.emplace(object.handle, Storage::Instance{node}).first;
        }
        auto& instance = found->second;
        if (instance.solid != model.solid) {
            const auto& mesh = storage.mesh_for(model);
            instance.node->set_mesh(mesh);
            instance.solid = model.solid;
            if (mesh->get_meta("hp2_rigid", false) || mesh->get_meta("hp2_morph", false))
                instance.node->set_instance_shader_parameter("hp2_animation_start", model.rigidAnimationStart);
            instance.animationStart = model.rigidAnimationStart;
        }
        if (instance.animationStart != model.rigidAnimationStart) {
            instance.node->set_instance_shader_parameter("hp2_animation_start", model.rigidAnimationStart);
            instance.animationStart = model.rigidAnimationStart;
        }
        instance.node->set_transform(race_transform(object.transform.data()));
        instance.node->show();
        instance.lastSeen = storage.serial;
    }
    for (auto it = storage.instances.begin(); it != storage.instances.end();) {
        if (it->second.lastSeen == storage.serial) {
            ++it;
        } else {
            storage.root.remove_child(it->second.node);
            it->second.node->queue_free();
            it = storage.instances.erase(it);
        }
    }
}

std::size_t attach_track(Node3D& root, const Scenery& scenery, const SkyResources* sky, bool reverse,
    const CollisionObjectRegistry& objects, std::vector<TrackPlacementVisibility>& movablePlacements,
    WorldObjectPresentation& worldObjects) {
    MeshConverter converter;
    const bool batchScenery = OS::get_singleton()->get_environment("HP2_MVP_SCENERY_BATCHING") != "0";
    std::unordered_map<const EditorMarkerModel*, PlacementMesh> meshes;
    // PAL10F928 single-player scenery filter. The donor loader already
    // applies the authored reverse placement transforms.
    const auto directionMask = 0x1u | (reverse ? 4u : 8u);
    std::unordered_set<const SceneryInstance*> movable;
    for (std::size_t i = 0; i < objects.placementCount; ++i)
        if (const auto* placement = objects.placements[i].scenery) movable.insert(placement);
    std::size_t placements{};
    for (std::size_t index = 0; index < scenery.loadedSectionCount; ++index) {
        const auto* section = scenery.loadedSections[index];
        if (!section) throw std::runtime_error("Loaded section table contains an empty entry");
        std::unordered_map<const EditorMarkerModel*, std::size_t> batchIndices;
        std::vector<PlacementBatch> batches;
        for (std::size_t item = 0; item < section->instanceCount; ++item) {
            const auto& instance = section->instances[item];
            if (instance.flags & directionMask) continue;
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
            // Smokeable spawners and rigid animated ferns transfer the
            // original scenery slot to a WorldObject while active.
            const auto* rigid = editorMarkerModelRigidAnimation(&model.levels[0]);
            const bool moves = movable.count(&instance) != 0 || (rigid && rigid->playbackType != 0);
            if (!moves && (instance.flags & 0x10u)) continue;
            auto& entry = meshes[level];
            const auto& mesh = entry.mesh;
            if (mesh.is_null()) {
                entry.mesh = converter.convert(*level);
                entry.batchable = batchScenery && can_batch(*mesh.ptr());
            }
            if (rigid && rigid->playbackType != 0)
                worldObjects.prepare_animation(model.levels[0]);
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
            if (moves) {
                auto& node = attach_placement(root, mesh, transform);
                node.set_visible((instance.flags & 0x10u) == 0);
                movablePlacements.push_back({&instance.flags, &node});
            } else if (entry.batchable && transform.basis.determinant() > 0) {
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
        node->set_name("TrackSky");
        // The sun pass also samples this sky through its dedicated camera.
        node->set_layer_mask(trackSkyLayer);
        node->set_mesh(mesh);
        for (int i=0;i<mesh->get_surface_count();++i) {
            auto material=mesh->surface_get_material(i);
            material->set_render_priority(Material::RENDER_PRIORITY_MIN);
        }
        node->set_transform(race_transform(sky->Transform(0)));
    }
    if (!placements) throw std::runtime_error("No drawable track placements");
    return placements;
}

}
