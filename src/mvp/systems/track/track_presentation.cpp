#include "track_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/Smokeable.hpp"
#include "bridge/presentation_snapshot.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/os.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <numeric>
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
    std::uint32_t layers{};
};

GeometryInstance3D& attach_batch(Node3D& root, const PlacementBatch& batch) {
    if (batch.transforms.size() == 1) {
        auto& node = attach_placement(root, batch.mesh, batch.transforms.front());
        node.set_layer_mask(batch.layers);
        return node;
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
    node->set_layer_mask(batch.layers);
    return *node;
}

struct PlacementMesh {
    Ref<ArrayMesh> mesh;
    Ref<ArrayMesh> reflection;
    bool batchable{};
};

Transform3D placement_transform(const SceneryInstance& instance) {
    if (instance.flags & 0x200u) return {};
    float frame[16]{};
    for (int lane = 0; lane < 9; ++lane)
        frame[(lane / 3) * 4 + lane % 3] = instance.rotation[lane] * (1.0f / 16384.0f);
    std::copy_n(instance.position, 3, frame + 12);
    frame[15] = 1.0f;
    return race_transform(frame);
}

AABB placement_bounds(const ArrayMesh& mesh, bool billboard) {
    const auto bounds = mesh.has_meta("hp2_animation_bounds") ?
        AABB(mesh.get_meta("hp2_animation_bounds")) : mesh.get_aabb();
    if (!billboard) return bounds;
    const auto end = bounds.get_end();
    const float x = std::max(std::abs(bounds.position.x), std::abs(end.x));
    const float z = std::max(std::abs(bounds.position.z), std::abs(end.z));
    const float radius = std::sqrt(x * x + z * z);
    return {{-radius, bounds.position.y, -radius}, {2 * radius, bounds.size.y, 2 * radius}};
}

}

struct TrackSceneryLod::Storage {
    struct Batch {
        Ref<ArrayMesh> mesh;
        MultiMeshInstance3D* node{};
        Ref<MultiMesh> instances;
        MeshInstance3D* single{};
        AABB meshBounds;
        std::vector<Transform3D> pending, visible;
        int capacity{};

        bool commit(Node3D& root, std::uint32_t layers) {
            if (pending == visible) {
                pending.clear();
                return false;
            }
            if (pending.empty()) {
                if (single) single->hide();
                if (node) node->hide();
            } else if (capacity == 1) {
                if (!single) {
                    single = &attach_placement(root, mesh, pending.front());
                    single->set_layer_mask(layers);
                    single->set_custom_aabb(meshBounds);
                } else {
                    single->set_transform(pending.front());
                }
                single->show();
            } else {
                if (instances.is_null()) {
                    instances.instantiate();
                    instances->set_transform_format(MultiMesh::TRANSFORM_3D);
                    instances->set_mesh(mesh);
                    instances->set_instance_count(capacity);
                    node = memnew(MultiMeshInstance3D);
                    node->set_layer_mask(layers);
                    node->set_multimesh(instances);
                    root.add_child(node);
                }
                AABB bounds = pending.front().xform(meshBounds);
                for (std::size_t i = 0; i < pending.size(); ++i) {
                    const auto& transform = pending[i];
                    if (i >= visible.size() || transform != visible[i])
                        instances->set_instance_transform(static_cast<int>(i), transform);
                    bounds.merge_with(transform.xform(meshBounds));
                }
                instances->set_custom_aabb(bounds);
                instances->set_visible_instance_count(static_cast<int>(pending.size()));
                node->show();
            }
            visible.swap(pending);
            pending.clear();
            return true;
        }
    };
    struct Placement {
        std::array<float, 3> position;
        Transform3D transform;
        std::array<int, 3> batches;
        std::array<int16_t, 3> minimumPixels;
        float radius;
        float boundsRadius;
        std::size_t defaultLod{};
        AABB bounds;
    };
    struct Section {
        struct TreeNode {
            AABB bounds;
            std::array<int16_t, 5> children;
            int count{};
        };
        std::vector<Placement> placements;
        std::vector<Batch> batches;
        std::vector<int> placementIndices;
        std::vector<TreeNode> tree;
        std::vector<GeometryInstance3D*> staticNodes;
        std::size_t staticPlacements{};
        bool active{}, wasActive{};
    };
    struct Zone {
        std::vector<int16_t> sections;
        float polygon[16][2];
        int32_t pointCount;
    };

    Storage(Node3D& root, View view, bool treeCulling)
        : root(root), view(view), treeCulling(treeCulling) {}

    bool primary() const { return view == View::Race; }
    bool gate_static() const { return view == View::Viewer || (primary() && treeCulling); }

    void prepare(const Scenery& scenery) {
        sections.resize(scenery.sectionCount);
        for (const auto* source = scenery.firstVisibleZone; source; source = source->next) {
            auto& zone = zones.emplace_back();
            zone.sections.assign(source->adjacentZones, source->adjacentZones + source->adjacentCount);
            std::memcpy(zone.polygon, source->polygon, sizeof(zone.polygon));
            zone.pointCount = source->polygonCount;
        }
        zoneOrder.resize(zones.size());
        std::iota(zoneOrder.begin(), zoneOrder.end(), std::size_t{});
        for (std::size_t i = 0; i < scenery.loadedSectionCount; ++i) {
            const auto* section = scenery.loadedSections[i];
            if (section && (view != View::Reflection || section->number < 100)) defaultSections.push_back(section->number);
        }
    }

    void prepare_tree(const ScenerySection& source) {
        auto& section = sections[source.number];
        if (section.placements.empty() || !source.cullNodes || !source.record34104Count) return;
        section.tree.resize(source.record34104Count);
        std::vector<bool> visited(section.tree.size()), covered(section.placements.size());
        const auto copy = [&](const auto& self, std::size_t index, unsigned depth) -> bool {
            if (index >= section.tree.size() || visited[index] || depth >= 64) return false;
            visited[index] = true;
            const auto& input = source.cullNodes[index];
            if (input.count < 0 || input.count > 5) return false;
            auto& node = section.tree[index];
            const auto a = race_vector(input.minimum), b = race_vector(input.maximum);
            node.bounds = AABB(a.min(b), (a - b).abs());
            node.count = input.count;
            std::copy_n(input.children, node.count, node.children.begin());
            for (int i = 0; i < node.count; ++i) {
                const int child = node.children[i];
                if (child < 0) {
                    if (!self(self, -child, depth + 1)) return false;
                    node.bounds.merge_with(section.tree[-child].bounds);
                } else {
                    if (std::size_t(child) >= section.placementIndices.size()) return false;
                    const int placement = section.placementIndices[child];
                    if (placement < 0) continue;
                    if (covered[placement]) return false;
                    covered[placement] = true;
                    node.bounds.merge_with(section.placements[placement].bounds);
                }
            }
            return true;
        };
        // An incomplete tree cannot safely remove any of this section's placements.
        if (!copy(copy, 0, 0) || std::find(covered.begin(), covered.end(), false) != covered.end())
            section.tree.clear();
    }

    static int classify(const AABB& bounds, const std::array<Plane, 6>& planes) {
        const auto half = bounds.size * 0.5f;
        const auto center = bounds.position + half;
        int result = 2;
        for (const auto& plane : planes) {
            const float distance = plane.distance_to(center);
            const float radius = plane.normal.abs().dot(half);
            if (distance > radius + 0.01f) return 0;
            if (distance >= -radius - 0.01f) result = 1;
        }
        return result;
    }

    void update(const float origin[3], float focal, float frustumReach, bool reset,
        const Camera3D* camera = nullptr, bool zonesEnabled = true) {
        const std::array<float, 3> eye{origin[0], origin[1], origin[2]};
        const bool useTree = camera && primary() && treeCulling && zonesEnabled;
        const auto pose = useTree ? camera->get_camera_transform() : Transform3D{};
        const auto projection = useTree ? camera->get_camera_projection() : Projection{};
        if (reset) std::iota(zoneOrder.begin(), zoneOrder.end(), std::size_t{});
        stats.batchUpdates = 0;
        if (initialized && !reset && eye == previousEye && focal == previousFocal &&
            frustumReach == previousReach && zonesEnabled == previousZones &&
            pose == previousPose && projection == previousProjection) return;
        initialized = true;
        previousEye = eye;
        previousFocal = focal;
        previousReach = frustumReach;
        previousZones = zonesEnabled;
        previousPose = pose;
        previousProjection = projection;

        // PAL150950..150B0C. Mirror the source zone order in presentation storage.
        const auto found = zonesEnabled ? std::find_if(zoneOrder.begin(), zoneOrder.end(), [&](std::size_t i) {
            return sceneryPointInVisibleZone(origin, zones[i].polygon, zones[i].pointCount);
        }) : zoneOrder.end();
        const auto layers = primary() ? trackPrimarySceneryLayer : 1u;
        const bool fallback = view != View::Reflection && found == zoneOrder.end();
        if (fallback && usingFallback && !reset) return;
        usingFallback = fallback;
        stats = {};
        stats.fallback = fallback;
        for (const auto number : activeSections) sections[number].active = false;
        nextSections.clear();
        const auto activate = [&](int number) {
            if (number < 0 || std::size_t(number) >= sections.size()) return;
            auto& section = sections[number];
            if (section.active || (section.placements.empty() && section.staticNodes.empty())) return;
            section.active = true;
            nextSections.push_back(number);
        };
        if (found == zoneOrder.end()) {
            for (const auto number : defaultSections) activate(number);
        } else {
            for (int number = 90; number < 100; ++number) activate(number);
            for (const auto number : zones[*found].sections) activate(number);
            std::rotate(zoneOrder.begin(), found, found + 1);
        }
        for (const auto number : activeSections) {
            auto& section = sections[number];
            if (section.active) continue;
            section.wasActive = false;
            for (auto& batch : section.batches) stats.batchUpdates += batch.commit(root, layers);
            for (auto* node : section.staticNodes) node->set_layer_mask_value(1, false);
        }
        stats.sections = nextSections.size();
        std::array<Plane, 6> planes;
        bool hasFrustum = false;
        if (useTree && !fallback) {
            const auto frustum = camera->get_frustum();
            hasFrustum = frustum.size() == int(planes.size());
            for (int i = 0; hasFrustum && i < frustum.size(); ++i) planes[i] = frustum[i];
        }
        for (const auto number : nextSections) {
            auto& section = sections[number];
            if (!section.wasActive)
                for (auto* node : section.staticNodes) node->set_layer_mask_value(1, true);
            section.wasActive = true;
            stats.placements += section.staticPlacements;
            const auto select = [&](const Placement& placement) {
                ++stats.candidates;
                if (fallback) {
                    const int batch = placement.batches[placement.defaultLod];
                    if (batch >= 0) {
                        section.batches[batch].pending.push_back(placement.transform);
                        ++stats.placements;
                    }
                    return;
                }
                const float dx = placement.position[0] - origin[0];
                const float dy = placement.position[1] - origin[1];
                const float dz = placement.position[2] - origin[2];
                const float squaredDistance = dx * dx + dy * dy + dz * dz;
                const float reach = frustumReach + placement.boundsRadius;
                if (squaredDistance > reach * reach) return;
                const float distance = std::sqrt(squaredDistance) - placement.radius;
                // PAL150E04 and PAL14FFC4: source projected size and authored LOD thresholds.
                const auto pixels = static_cast<int>(placement.radius < distance ?
                    placement.radius * focal / distance : focal);
                if (pixels < 2) return;
                for (std::size_t lod = 0; lod < placement.batches.size(); ++lod) {
                    const auto batch = placement.batches[lod];
                    if (batch == missingLod || pixels < placement.minimumPixels[lod]) continue;
                    if (batch >= 0) {
                        section.batches[batch].pending.push_back(placement.transform);
                        ++stats.placements;
                    }
                    break;
                }
            };
            if (hasFrustum && !section.tree.empty()) {
                candidates.clear();
                const auto visit = [&](const auto& self, int index, int clip) -> void {
                    ++stats.treeNodes;
                    const auto& node = section.tree[index];
                    if (clip == 1) clip = classify(node.bounds, planes);
                    if (!clip) return;
                    for (int i = 0; i < node.count; ++i) {
                        const int child = node.children[i];
                        if (child < 0) self(self, -child, clip);
                        else {
                            const int placement = section.placementIndices[child];
                            if (placement >= 0 && (clip == 2 || classify(section.placements[placement].bounds, planes)))
                                candidates.push_back(placement);
                        }
                    }
                };
                visit(visit, 0, 1);
                std::sort(candidates.begin(), candidates.end());
                for (const auto placement : candidates) select(section.placements[placement]);
            } else {
                for (const auto& placement : section.placements) select(placement);
            }
            for (auto& batch : section.batches) stats.batchUpdates += batch.commit(root, layers);
        }
        activeSections.swap(nextSections);
    }

    static constexpr int missingLod = -2;
    Node3D& root;
    View view;
    bool treeCulling;
    bool initialized{}, usingFallback{}, previousZones{};
    Statistics stats;
    std::vector<Section> sections;
    std::vector<Zone> zones;
    std::vector<std::size_t> zoneOrder;
    std::vector<int> defaultSections, activeSections, nextSections;
    std::array<float, 3> previousEye{};
    std::vector<int> candidates;
    Transform3D previousPose;
    Projection previousProjection;
    float previousFocal{}, previousReach{};
};

TrackSceneryLod::TrackSceneryLod(Node3D& root, View view, bool treeCulling)
    : storage_(std::make_unique<Storage>(root, view, treeCulling)) {}
TrackSceneryLod::~TrackSceneryLod() = default;

void TrackSceneryLod::update(const float origin[3], float focal, float frustumReach, bool reset) {
    storage_->update(origin, focal, frustumReach, reset);
}

void TrackSceneryLod::update_camera(const Camera3D& camera, const float origin[3], float focal,
    bool zonesEnabled, bool reset) {
    storage_->update(origin, focal, camera.get_far(), reset, &camera, zonesEnabled);
}

const TrackSceneryLod::Statistics& TrackSceneryLod::statistics() const { return storage_->stats; }

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
    MeshConverter converter{nullptr, false, true};
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
    WorldObjectPresentation& worldObjects, TrackSceneryLod* reflection, bool sharedWorldReflection, TrackSceneryLod* primary,
    std::vector<TrackInspectionPlacement>* inspection) {
    MeshConverter converter{nullptr, false, true};
    const bool batchScenery = OS::get_singleton()->get_environment("HP2_MVP_SCENERY_BATCHING") != "0";
    const bool auditMaterials = OS::get_singleton()->get_environment("HP2_MVP_TRACK_MATERIAL_AUDIT") == "1" &&
        !OS::get_singleton()->get_environment("HP2_PROFILE_DIR").is_empty();
    std::unordered_set<const EditorMarkerModel*> auditedModels;
    std::unordered_map<const EditorMarkerModel*, PlacementMesh> meshes;
    // PAL10F928 single-player scenery filter. The donor loader already
    // applies the authored reverse placement transforms.
    const auto directionMask = 0x1u | (reverse ? 4u : 8u);
    const auto reflectionMask = directionMask | 0x12u;
    if (reflection) reflection->storage_->prepare(scenery);
    if (primary) primary->storage_->prepare(scenery);
    using Surface = MeshConverter::ScenerySurface;
    std::unordered_map<const EditorMarkerModel*, Ref<ArrayMesh>> facades;
    const auto reflection_mesh = [&](const EditorMarkerModel& model, bool billboard) -> const Ref<ArrayMesh>& {
        if (billboard) {
            auto& mesh = facades[&model];
            if (mesh.is_null()) mesh = converter.convert(model, true, 0, false, Surface::ReflectionBillboard);
            return mesh;
        }
        auto& entry = meshes[&model];
        if (entry.reflection.is_null()) {
            if (entry.mesh.is_valid() && !bool(entry.mesh->get_meta("hp2_reflection_surface_filter", false)))
                entry.reflection = entry.mesh;
            else
                entry.reflection = converter.convert(model, true, 0, false, Surface::Reflection);
        }
        return entry.reflection;
    };
    std::unordered_set<const SceneryInstance*> movable;
    for (std::size_t i = 0; i < objects.placementCount; ++i)
        if (const auto* placement = objects.placements[i].scenery) movable.insert(placement);
    std::size_t placements{};
    for (std::size_t index = 0; index < scenery.loadedSectionCount; ++index) {
        const auto* section = scenery.loadedSections[index];
        if (!section) throw std::runtime_error("Loaded section table contains an empty entry");
        if (auditMaterials)
            for (std::size_t i = 0; i < section->modelCount; ++i)
                for (const auto& model : section->models[i].levels)
                    if (model.solid && auditedModels.insert(&model).second)
                        MeshConverter::audit_track_model(model);
        std::map<std::pair<const EditorMarkerModel*, std::uint32_t>, std::size_t> batchIndices;
        std::vector<PlacementBatch> batches;
        std::map<std::tuple<const EditorMarkerModel*, bool, bool>, int> reflectionBatches;
        decltype(reflectionBatches) primaryBatches;
        auto* primarySection = primary ? &primary->storage_->sections.at(section->number) : nullptr;
        if (primarySection && primary->storage_->primary() && primary->storage_->treeCulling && section->record34104Count)
            primarySection->placementIndices.assign(section->instanceCount, -1);
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
            const bool reflect = !moves && !(instance.flags & reflectionMask);
            const bool splitReflection = sharedWorldReflection && reflect && ((instance.flags & 0x100u) ||
                bool(mesh->get_meta("hp2_sorted_surfaces", false)));
            const bool primaryLod = primary && primary->storage_->primary() && !moves && mesh->get_surface_count() &&
                !bool(mesh->get_meta("hp2_reflection_surface_filter", false));
            const bool gated = primary && primary->storage_->gate_static() && !moves && !primaryLod &&
                section->record34104Count && mesh->get_surface_count();
            if (gated) ++primarySection->staticPlacements;
            const auto layers = (primaryLod ? trackMirrorSceneryLayer : 1u) |
                (gated && primary->storage_->primary() ? trackMirrorSceneryLayer : 0u) |
                (sharedWorldReflection && reflect && !splitReflection ? trackReflectionLayer : 0u);
            if (rigid && rigid->playbackType != 0)
                worldObjects.prepare_animation(model.levels[0]);
            const auto transform = placement_transform(instance);
            if (inspection && !(instance.flags & 0x10u) && mesh->get_surface_count())
                inspection->push_back({section->number, item, mesh, transform});
            const auto addLods = [&](TrackSceneryLod& lods, auto& indices, bool isReflection) {
                auto& lodSection = lods.storage_->sections.at(section->number);
                if (!lodSection.placementIndices.empty())
                    lodSection.placementIndices[item] = static_cast<int>(lodSection.placements.size());
                auto& placement = lodSection.placements.emplace_back();
                std::copy_n(instance.position, 3, placement.position.begin());
                std::copy_n(model.minimumPixels, 3, placement.minimumPixels.begin());
                placement.radius = model.radius + 4.0f;
                // Include authored bounds for large track cells as well as the model radius.
                float squaredExtent = 0;
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    const float extent = std::max(std::abs(float(instance.minimum[axis]) - instance.position[axis]),
                        std::abs(float(instance.maximum[axis]) - instance.position[axis]));
                    squaredExtent += extent * extent;
                }
                placement.boundsRadius = 2.0f + std::max(placement.radius, std::sqrt(squaredExtent));
                const bool identity = (instance.flags & 0x200u) != 0;
                placement.transform = transform;
                if (isReflection && !identity && (instance.flags & 0x100u)) placement.transform.origin.y += 2.0f;
                placement.defaultLod = static_cast<std::size_t>(level - model.levels);
                placement.batches.fill(TrackSceneryLod::Storage::missingLod);
                if (!lodSection.placementIndices.empty())
                    placement.bounds = transform.xform(placement_bounds(*mesh.ptr(), false));
                for (std::size_t lod = 0; lod < placement.batches.size(); ++lod) {
                    const auto& candidate = model.levels[lod];
                    if (!candidate.solid) continue;
                    const bool billboard = !identity && model.facade[lod];
                    auto& candidateEntry = meshes[&candidate];
                    auto& candidateMesh = candidateEntry.mesh;
                    if (!isReflection && candidateMesh.is_null()) {
                        candidateMesh = converter.convert(candidate);
                        candidateEntry.batchable = batchScenery && can_batch(*candidateMesh.ptr());
                    }
                    // Preserve a model's original transparent surfaces on the legacy path.
                    const bool safeLod = isReflection || !bool(candidateMesh->get_meta("hp2_reflection_surface_filter", false));
                    if (!safeLod) continue;
                    const auto& opaque = isReflection ? reflection_mesh(candidate, billboard) : candidateMesh;
                    placement.batches[lod] = -1;
                    if (!opaque->get_surface_count()) continue;
                    const auto [key, inserted] = indices.try_emplace(
                        std::make_tuple(&candidate, isReflection && billboard, transform.basis.determinant() < 0),
                        static_cast<int>(lodSection.batches.size()));
                    if (inserted) {
                        auto& batch = lodSection.batches.emplace_back();
                        batch.mesh = opaque;
                        batch.meshBounds = placement_bounds(*opaque.ptr(), isReflection && billboard);
                    }
                    placement.batches[lod] = key->second;
                    auto& batch = lodSection.batches[key->second];
                    ++batch.capacity;
                    if (!lodSection.placementIndices.empty())
                        placement.bounds.merge_with(transform.xform(batch.meshBounds));
                }
            };
            if (reflection && reflect && section->record34104Count) addLods(*reflection, reflectionBatches, true);
            if (primaryLod) addLods(*primary, primaryBatches, false);
            if (!mesh->get_surface_count()) continue; // All strips can be non-drawing.
            // A MultiMesh has one renderer instance and one culling bound.
            // Keep section boundaries and preserve per-instance winding for mirrored placements.
            if (moves) {
                auto& node = attach_placement(root, mesh, transform);
                node.set_layer_mask(1);
                node.set_visible((instance.flags & 0x10u) == 0);
                movablePlacements.push_back({&instance.flags, &node});
            } else if (gated && primary->storage_->primary()) {
                auto& node = attach_placement(root, mesh, transform);
                node.set_layer_mask(layers & ~1u);
                primarySection->staticNodes.push_back(&node);
            } else if (meshes.at(level).batchable && transform.basis.determinant() > 0) {
                const auto batchLayers = gated ? layers & ~1u : layers;
                const auto [batch, inserted] = batchIndices.try_emplace(std::make_pair(level, batchLayers), batches.size());
                if (inserted) batches.push_back({mesh, {}, batchLayers});
                batches[batch->second].transforms.push_back(transform);
            } else {
                auto& node = attach_placement(root, mesh, transform);
                node.set_layer_mask(gated ? layers & ~1u : layers);
                if (gated) primarySection->staticNodes.push_back(&node);
            }
            if (splitReflection) {
                if (entry.reflection.is_null()) entry.reflection = converter.convert(*level, true, 0, false, Surface::OpaqueOnly);
                if (entry.reflection->get_surface_count()) {
                    auto reflected = transform;
                    if (instance.flags & 0x100u) reflected.origin.y += 2.0f;
                    attach_placement(root, entry.reflection, reflected).set_layer_mask(trackReflectionLayer);
                }
            }
            ++placements;
        }
        if (primarySection && !primarySection->placementIndices.empty()) primary->storage_->prepare_tree(*section);
        for (const auto& batch : batches) {
            auto& node = attach_batch(root, batch);
            if (!batch.layers && primarySection) primarySection->staticNodes.push_back(&node);
        }
    }
    if (sky && sky->Model(0).solid) {
        MeshConverter skyConverter;
        const auto mesh = skyConverter.convert(sky->Model(0), false, 0, false, MeshConverter::ScenerySurface::Sky);
        if (!mesh->get_surface_count()) throw std::runtime_error("Authored sky has no drawable surfaces");
        auto* node = memnew(MeshInstance3D);
        root.add_child(node);
        node->set_name("TrackSky");
        // The sun pass also samples this sky through its dedicated camera.
        node->set_layer_mask(trackSkyLayer);
        node->set_mesh(mesh);
        // The sky shader bypasses world far clipping. Keep Godot's CPU
        // frustum test conservative over the original sky projection range.
        constexpr float skyCullMargin = 50000.0f;
        node->set_extra_cull_margin(skyCullMargin);
        node->set_transform(race_transform(sky->Transform(0)));
        if (reflection) {
            auto& reflectedSky = attach_placement(reflection->storage_->root, mesh, node->get_transform());
            reflectedSky.set_layer_mask(trackSkyLayer);
            reflectedSky.set_extra_cull_margin(skyCullMargin);
        }
    }
    if (!placements) throw std::runtime_error("No drawable track placements");
    return placements;
}

}
