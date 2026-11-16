#include "track_viewer.hpp"

#include "assets/shader_resource.hpp"
#include "track_viewer_scene.hpp"

#include "assets/mesh_converter.hpp"
#include "bware/inc/bFile.hpp"
#include "bware/inc/bChunk.hpp"
#include "indep/inc/FMenus.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/Language.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/Smokeable.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/Timer.hpp"
#include "systems/camera/race_post_presentation.hpp"
#include "systems/effects/race_effect_options.hpp"
#include "systems/effects/race_effects_presentation.hpp"
#include "systems/track/track_presentation.hpp"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/compositor.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/geometry3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hp2 {
using namespace godot;
namespace {
enum Category { Placement, RouteNode, Emitter, CategoryCount };
constexpr std::array<const char*, CategoryCount> categories{"Scenery", "Route nodes", "Emitters"};
constexpr std::array<const char*, 7> movementActions{"hp2_steer_left", "hp2_steer_right",
    "hp2_accelerate", "hp2_brake", "hp2_r2", "hp2_l2", "hp2_l1"};
constexpr float radians = 3.14159265358979323846f / 180.0f;

struct Entity {
    Category category;
    int section;
    Vector3 position;
    AABB bounds;
    std::string label;
    std::string details;
    std::string search;
};

struct Section {
    int number;
    int count{};
    AABB bounds;
};

struct HierarchyModel {
    std::string name;
    int bundle{};
    int section{-1};
    std::size_t modelIndex{};
    std::array<std::uint32_t, 3> keys{};
    std::array<bool, 3> resolved{};
    std::vector<int> instances;
};

struct HierarchyInstance {
    const SceneryInstance* source{};
    int model{-1};
    std::size_t index{};
    TrackInspectionPlacement draw;
    AABB bounds;
};

struct HierarchySection {
    std::uint32_t number;
    std::vector<int> models;
};

std::string model_name(const EditorMarkerSolid* solid, std::uint32_t key) {
    if (solid && solid->name[0]) {
        const auto end = std::find(std::begin(solid->name), std::end(solid->name), '\0');
        return {solid->name, end};
    }
    char label[24];
    std::snprintf(label, sizeof(label), "Model 0x%08X", key);
    return label;
}

Vector3 source_vector(Vector3 vector) {
    return {vector.x, -vector.z, vector.y};
}

AABB marker(Vector3 position, float radius = 1.5f) {
    const Vector3 half(radius, radius, radius);
    return {position - half, half * 2};
}

void box_lines(PackedVector3Array& points, const AABB& box) {
    const auto start = points.size();
    points.resize(start + 24);
    auto* output = points.ptrw() + start;
    for (int corner = 0; corner < 8; ++corner)
        for (const int axis : {1, 2, 4})
            if (!(corner & axis)) {
                *output++ = box.get_endpoint(corner);
                *output++ = box.get_endpoint(corner | axis);
            }
}

void set_lines(MeshInstance3D& instance, const PackedVector3Array& vertices) {
    if (vertices.is_empty()) {
        instance.set_mesh({});
        return;
    }
    Ref<ArrayMesh> mesh;
    mesh.instantiate();
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = vertices;
    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);
    instance.set_mesh(mesh);
}

MeshInstance3D* make_lines(Node3D& scene, Color color) {
    auto* node = memnew(MeshInstance3D);
    scene.add_child(node);
    Ref<StandardMaterial3D> material;
    material.instantiate();
    material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    material->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
    material->set_albedo(color);
    node->set_material_override(material);
    node->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    node->set_layer_mask(1u << 9);
    return node;
}

constexpr ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
}

struct TrackViewer::Storage {
    explicit Storage(Node3D& owner) : host(owner) {
        container = memnew(SubViewportContainer);
        container->set_name("TrackViewerViewport");
        container->set_stretch(true);
        container->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        container->hide();
        host.add_child(container);
        output = memnew(SubViewport);
        view = output;
        Ref<World3D> world;
        world.instantiate();
        view->set_world_3d(world);
        container->add_child(view);
        view->set_update_mode(SubViewport::UPDATE_DISABLED);
    }

    Node3D& host;
    SubViewportContainer* container{};
    SubViewport* output{};
    SubViewport* view{};
    Camera3D* camera{};
    Node3D* trackScene{};
    Node3D* lines{};
    MeshInstance3D* sectionLines{};
    std::array<MeshInstance3D*, CategoryCount> entityLines{};
    MeshInstance3D* selectionLines{};
    MeshInstance3D* modelHighlight{};
    std::unique_ptr<TrackViewerScene> source;
    std::unique_ptr<WorldObjectPresentation> worldObjects;
    std::unique_ptr<TrackSceneryLod> sceneryVisibility;
    std::unique_ptr<RaceEffectsPresentation> effects;
    RaceEffectsFrame effectsFrame;
    Ref<RacePostPresentation> post;
    Ref<ShaderMaterial> wireframeMaterial;
    std::size_t placementCount{};
    Rect2 viewportRect;
    AABB routeBounds;
    struct Track { int id; std::string caption; };
    std::vector<Track> tracks;
    std::vector<Section> sections;
    std::array<std::string, 2> bundleNames;
    std::array<std::vector<int>, 2> bundleModels;
    std::vector<HierarchySection> hierarchySections;
    std::vector<HierarchyModel> hierarchyModels;
    std::vector<HierarchyInstance> hierarchyInstances;
    std::unordered_map<const ArrayMesh*, PackedVector3Array> pickFaces;
    int selectedModel{-1}, selectedInstance{-1};
    bool revealInstance{};
    bool hierarchyAudit{};
    std::vector<Entity> entities;
    std::vector<std::size_t> filtered;
    std::array<bool, CategoryCount> categoryVisible{true, true, true};
    std::array<bool, movementActions.size()> movement{};
    ImGuiTextFilter entityFilter;
    Request request;
    std::string failure;
    int selectedTrack{};
    int preferredTrack{};
    int language{};
    int selectedSection{-1};
    int selectedEntity{-1};
    bool active{}, confirm{}, captured{}, debugView{}, loading{};
    bool wireframeView{};
    bool sectionsVisible{}, entitiesVisible{};
    bool filterDirty{true};
    bool poseDirty{};
    float yaw{}, pitch{}, speed{120}, fov{60};
    float inspectorWidth{260.0f};
    bool inspectorVisible{true};
    Vector3 start, position;
    float startYaw{}, startPitch{};
    struct CameraFlight {
        Vector3 from, to;
        float fromYaw, fromPitch, yawDelta, toPitch;
        float elapsed{};
    };
    std::optional<CameraFlight> flight;

    void capture(bool enabled) {
        if (captured == enabled) return;
        captured = enabled;
        if (enabled) flight.reset();
        movement.fill(false);
        Input::get_singleton()->set_mouse_mode(enabled ? Input::MOUSE_MODE_CAPTURED : Input::MOUSE_MODE_VISIBLE);
    }

    void focus(const AABB& bounds) {
        flight.reset();
        const auto center = bounds.get_center();
        const float distance = std::max(9.0f,
            static_cast<float>(bounds.size.length()) * 0.65f / std::tan(fov * radians * 0.5f));
        const Vector3 direction = Vector3(1, 0.65f, 1).normalized();
        position = center + direction * distance;
        yaw = std::atan2(direction.x, direction.z);
        pitch = std::asin(-direction.y);
        poseDirty = true;
    }

    void fly_to(const AABB& bounds) {
        const auto center = bounds.get_center();
        const float size = std::max(1.0f, static_cast<float>(bounds.size.length()));
        const auto viewportSize = view->get_visible_rect().size;
        const float aspect = viewportSize.x / std::max(1.0f, static_cast<float>(viewportSize.y));
        const float halfFovTan = std::tan(fov * radians * 0.5f) * std::min(1.0f, aspect);
        const float distance = std::max(3.0f, size * 0.65f / halfFovTan);
        Vector3 direction = position - center;
        direction.y = 0;
        if (direction.is_zero_approx()) direction = Vector3(1, 0, 1);
        direction.normalize();
        direction.y = 0.70f;
        direction.normalize();
        const float targetYaw = std::atan2(direction.x, direction.z);
        flight = CameraFlight{position, center + direction * distance, yaw, pitch,
            std::remainder(targetYaw - yaw, 2.0f * 3.14159265358979323846f), std::asin(-direction.y)};
    }

    void update_camera() {
        if (!camera) return;
        camera->set_position(position);
        camera->set_rotation(Vector3(pitch, yaw, 0));
        camera->set_fov(fov);
        poseDirty = false;
    }

    void update_mode() {
        if (modelHighlight) modelHighlight->set_visible(!debugView && modelHighlight->get_mesh().is_valid());
        view->set_debug_draw(wireframeView ? Viewport::DEBUG_DRAW_WIREFRAME : Viewport::DEBUG_DRAW_DISABLED);
        if (trackScene) {
            trackScene->set_visible(!debugView);
            if (wireframeView && wireframeMaterial.is_null()) {
                Ref<Shader> shader;
                shader = load_shader("res://shader/track_viewer.gdshader");
                wireframeMaterial.instantiate();
                wireframeMaterial->set_shader(shader);
            }
            const Ref<Material> material = wireframeView ? Ref<Material>(wireframeMaterial) : Ref<Material>();
            for (int i = 0; i < trackScene->get_child_count(); ++i) {
                auto* geometry = Object::cast_to<GeometryInstance3D>(trackScene->get_child(i));
                if (geometry && geometry->get_layer_mask() != trackSkyLayer &&
                    geometry->get_material_override() != material)
                    geometry->set_material_override(material);
            }
        }
        if (camera) {
            camera->set_cull_mask_value(4, !wireframeView);
            camera->get_environment()->set_glow_enabled(wireframeView);
        }
        if (sectionLines) sectionLines->set_visible(debugView || sectionsVisible);
        for (std::size_t category = 0; category < entityLines.size(); ++category)
            if (entityLines[category])
                entityLines[category]->set_visible((debugView || entitiesVisible) && categoryVisible[category]);
    }

    void update_selection() {
        if (!selectionLines) return;
        PackedVector3Array points;
        if (selectedEntity >= 0) box_lines(points, entities[selectedEntity].bounds.grow(0.2f));
        else if (selectedSection >= 0) box_lines(points, sections[selectedSection].bounds);
        set_lines(*selectionLines, points);
    }

    void rebuild_filter() {
        filtered.clear();
        std::array<PackedVector3Array, CategoryCount> points;
        for (std::size_t index = 0; index < entities.size(); ++index) {
            const auto& entity = entities[index];
            if (!categoryVisible[entity.category] ||
                (selectedSection >= 0 && entity.section != sections[selectedSection].number)) continue;
            if (!entityFilter.PassFilter(entity.search.c_str())) continue;
            filtered.push_back(index);
            box_lines(points[entity.category], marker(entity.position, entity.category == Placement ? 0.9f : 2.0f));
        }
        for (std::size_t i = 0; i < points.size(); ++i)
            if (entityLines[i]) set_lines(*entityLines[i], points[i]);
        filterDirty = false;
    }

    void index(const TrackViewerScene& scene) {
        const auto& scenery = scene.scenery();
        for (std::size_t slot = 0; slot < scenery.loadedSectionCount; ++slot) {
            const auto& source = *scenery.loadedSections[slot];
            auto& section = sections.emplace_back();
            section.number = source.number;
            for (std::size_t index = 0; index < source.instanceCount; ++index) {
                const auto& instance = source.instances[index];
                auto& entity = entities.emplace_back();
                entity.category = Placement;
                entity.section = section.number;
                entity.position = race_vector(instance.position);
                std::uint32_t key{};
                if (instance.modelIndex >= 0 && static_cast<std::size_t>(instance.modelIndex) < source.modelCount)
                    key = source.models[instance.modelIndex].levels[0].key;
                entity.label = "Scenery " + std::to_string(section.number) + ":" + std::to_string(index);
                char attributes[120];
                std::snprintf(attributes, sizeof(attributes), "Model 0x%08X | model index %d | flags 0x%04X",
                    key, instance.modelIndex, static_cast<unsigned>(instance.flags));
                entity.details = attributes;
                const float lo[]{float(instance.minimum[0]), float(instance.minimum[1]), float(instance.minimum[2])};
                const float hi[]{float(instance.maximum[0]), float(instance.maximum[1]), float(instance.maximum[2])};
                const auto from = race_vector(lo);
                const auto to = race_vector(hi);
                entity.bounds = AABB(from, to - from).abs();
                if (entity.bounds.size.length_squared() < 0.01f) entity.bounds = marker(entity.position);
                section.bounds = section.count++ ? section.bounds.merge(entity.bounds) : entity.bounds;
            }
        }
        TerrainSurface query{};
        const bool hasTerrain = terrainSurfaceInitialize(&query, &scene.terrain());
        struct TerrainGuard {
            TerrainSurface* surface;
            ~TerrainGuard() { if (surface) terrainSurfaceDispose(surface); }
        } guard{hasTerrain ? &query : nullptr};
        const auto* route = &scene.route();
        if (route) {
            for (int index = 0; index < route->nodeCount; ++index) {
                const auto& node = route->nodes[index];
                float point[4]{node.point[0], node.point[1]};
                bool found{};
                if (hasTerrain) point[2] = terrainSurfaceElevation(&query, point, nullptr, nullptr, &found);
                const auto position = race_vector(point);
                routeBounds = index ? routeBounds.expand(position) : AABB(position, Vector3());
                entities.push_back({RouteNode, -1, position,
                    marker(position), "Route node " + std::to_string(index),
                    "Route section " + std::to_string(node.section) + " | distance " + std::to_string(node.distance) +
                        (found ? "" : " | elevation unavailable")});
            }
        }
        {
            for (auto* emitter = scene.particles().firstEmitter; emitter; emitter = emitter->autoNext) {
                if (!emitter->resource || emitter->lifetimeOwner) continue;
                const auto& source = *emitter->resource;
                const auto position = race_vector(emitter->frame + 12);
                char attributes[96];
                std::snprintf(attributes, sizeof(attributes), "Resource 0x%08X | definition 0x%08X",
                    source.key, source.definitionKey);
                entities.push_back({Emitter, -1,
                    position, marker(position, 3.0f),
                    std::string("Emitter ") + (source.name ? source.name : "") + " #" +
                        std::to_string(emitter->generation), attributes});
            }
        }
        for (auto& entity : entities) entity.search = entity.label + " " + entity.details;
        std::sort(sections.begin(), sections.end(), [](const auto& left, const auto& right) {
            return left.number < right.number;
        });
        PackedVector3Array bounds;
        for (const auto& section : sections)
            // Source zone bounds exclude the shared scenery sections.
            if (section.count && section.number < 90) box_lines(bounds, section.bounds);
        set_lines(*sectionLines, bounds);
        filterDirty = true;
    }

    void index_hierarchy(const std::vector<TrackInspectionPlacement>& drawings) {
        const auto bundles = source->track_bundles();
        std::unordered_map<const EditorMarkerSolid*, int> owners;
        std::unordered_set<const EditorMarkerSolid*> used;
        for (int file = 0; file < 2; ++file) {
            bundleNames[file] = bundles[file]->Filename();
            const auto* catalogue = bundles[file]->Resources().models;
            if (!catalogue) continue;
            for (std::size_t i = 0; i < catalogue->solidCount; ++i)
                owners.emplace(&catalogue->solids[i], file);
        }
        std::map<std::pair<std::uint32_t, std::size_t>, const TrackInspectionPlacement*> placed;
        for (const auto& draw : drawings) placed.emplace(std::make_pair(draw.section, draw.instance), &draw);
        const auto& scenery = source->scenery();
        for (std::size_t i = 0; i < scenery.sectionCount; ++i) {
            const auto* section = scenery.sections[i];
            if (!section) continue;
            auto& branch = hierarchySections.emplace_back();
            branch.number = section->number;
            for (std::size_t j = 0; j < section->modelCount; ++j) {
                auto& item = hierarchyModels.emplace_back();
                item.bundle = 1;
                item.section = section->number;
                item.modelIndex = j;
                const auto& sourceModel = section->models[j];
                const EditorMarkerSolid* solid = nullptr;
                for (std::size_t lod = 0; lod < item.keys.size(); ++lod) {
                    const auto& level = sourceModel.levels[lod];
                    item.keys[lod] = level.key;
                    item.resolved[lod] = level.solid != nullptr;
                    if (level.solid) {
                        used.insert(level.solid);
                        if (!solid) solid = level.solid;
                    }
                }
                if (const auto owner = owners.find(solid); owner != owners.end()) item.bundle = owner->second;
                item.name = model_name(solid, item.keys[0]);
                branch.models.push_back(static_cast<int>(hierarchyModels.size() - 1));
            }
            for (std::size_t j = 0; j < section->instanceCount; ++j) {
                const auto& instance = section->instances[j];
                if (instance.modelIndex < 0 || std::size_t(instance.modelIndex) >= branch.models.size()) continue;
                auto& item = hierarchyInstances.emplace_back();
                item.source = &instance;
                item.index = j;
                item.model = branch.models[instance.modelIndex];
                const auto found = placed.find({section->number, j});
                if (found != placed.end()) {
                    item.draw = *found->second;
                    item.bounds = item.draw.transform.xform(item.draw.mesh->get_aabb()).grow(0.002f);
                }
                hierarchyModels[item.model].instances.push_back(static_cast<int>(hierarchyInstances.size() - 1));
            }
        }
        for (int file = 0; file < 2; ++file) {
            const auto* catalogue = bundles[file]->Resources().models;
            if (!catalogue) continue;
            for (std::size_t j = 0; j < catalogue->solidCount; ++j) {
                const auto& solid = catalogue->solids[j];
                if (used.count(&solid)) continue;
                auto& item = hierarchyModels.emplace_back();
                item.bundle = file;
                item.modelIndex = j;
                item.keys[0] = solid.key;
                item.resolved[0] = true;
                item.name = model_name(&solid, solid.key);
                bundleModels[file].push_back(static_cast<int>(hierarchyModels.size() - 1));
            }
        }
    }

    void select_model(int model, int instance = -1) {
        selectedModel = model;
        selectedInstance = instance;
        revealInstance = false;
        modelHighlight->remove_meta("section");
        modelHighlight->remove_meta("instance");
        modelHighlight->hide();
        modelHighlight->set_mesh({});
        if (instance < 0) return;
        const auto& draw = hierarchyInstances[instance].draw;
        if (draw.mesh.is_null()) return;
        modelHighlight->set_mesh(draw.mesh);
        modelHighlight->set_transform(draw.transform);
        modelHighlight->set_visible(!debugView);
        modelHighlight->set_meta("section", draw.section);
        modelHighlight->set_meta("instance", static_cast<int64_t>(draw.instance));
    }

    void pick(const Vector2& pointer) {
        if (!modelHighlight || debugView) return;
        const auto pixel = (pointer - viewportRect.position) / viewportRect.size * view->get_visible_rect().size;
        const auto origin = camera->project_ray_origin(pixel);
        const auto direction = camera->project_ray_normal(pixel);
        float nearest = camera->get_far();
        int picked = -1;
        auto* geometry = Geometry3D::get_singleton();
        for (std::size_t i = 0; i < hierarchyInstances.size(); ++i) {
            const auto& item = hierarchyInstances[i];
            if (item.draw.mesh.is_null() || !item.bounds.intersects_ray(origin, direction)) continue;
            const auto& transform = item.draw.transform;
            if (std::abs(transform.basis.determinant()) < 1e-8f) continue;
            const auto inverse = transform.affine_inverse();
            const auto localOrigin = inverse.xform(origin);
            const auto localRay = inverse.basis.xform(direction).normalized();
            auto [found, inserted] = pickFaces.try_emplace(item.draw.mesh.ptr());
            if (inserted) found->second = item.draw.mesh->get_faces();
            const auto& faces = found->second;
            for (int64_t j = 0; j + 2 < faces.size(); j += 3) {
                const auto hit = geometry->ray_intersects_triangle(localOrigin, localRay,
                    faces[j], faces[j + 1], faces[j + 2]);
                if (hit.get_type() == Variant::NIL) continue;
                const auto distance = (transform.xform(Vector3(hit)) - origin).dot(direction);
                if (distance < camera->get_near() || distance >= nearest) continue;
                nearest = distance;
                picked = static_cast<int>(i);
            }
        }
        if (picked == selectedInstance) picked = -1;
        flight.reset();
        select_model(picked < 0 ? -1 : hierarchyInstances[picked].model, picked);
        revealInstance = picked >= 0;
    }

    void activate_model(int index, bool locate) {
        int nearestInstance = -1;
        float nearest = std::numeric_limits<float>::infinity();
        for (const int instance : hierarchyModels[index].instances) {
            const auto& item = hierarchyInstances[instance];
            if (item.draw.mesh.is_null()) continue;
            const float distance = position.distance_squared_to(item.bounds.get_center());
            if (distance < nearest) {
                nearest = distance;
                nearestInstance = instance;
            }
        }
        select_model(index, nearestInstance);
        if (locate && nearestInstance >= 0) {
            revealInstance = true;
            fly_to(hierarchyInstances[nearestInstance].bounds);
        }
    }

    void draw_model(int index) {
        auto& model = hierarchyModels[index];
        ImGui::PushID(index);
        if (revealInstance && selectedModel == index) ImGui::SetNextItemOpen(true);
        auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (selectedModel == index) flags |= ImGuiTreeNodeFlags_Selected;
        if (model.instances.empty()) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        const bool opened = ImGui::TreeNodeEx("model", flags, "%s", model.name.c_str());
        if (ImGui::IsItemClicked(0)) {
            const bool doubleClicked = ImGui::IsMouseDoubleClicked(0);
            if (doubleClicked || !ImGui::IsItemToggledOpen()) activate_model(index, doubleClicked);
        }
        if (opened && !model.instances.empty()) {
            for (const int id : model.instances) {
                const auto& item = hierarchyInstances[id];
                const std::string row = "Instance " + std::to_string(item.index) +
                    (item.draw.mesh.is_null() ? " (hidden)" : "");
                ImGui::PushID(id);
                if (ImGui::Selectable(row.c_str(), selectedInstance == id, ImGuiSelectableFlags_AllowDoubleClick)) {
                    select_model(index, id);
                    if (ImGui::IsMouseDoubleClicked(0) && item.draw.mesh.is_valid()) fly_to(item.bounds);
                }
                if (revealInstance && selectedInstance == id) {
                    ImGui::SetScrollHereY();
                    revealInstance = false;
                }
                if (hierarchyAudit && selectedInstance == id) {
                    const auto min = ImGui::GetItemRectMin();
                    const auto max = ImGui::GetItemRectMax();
                    const auto origin = ImGui::GetWindowPos();
                    if (ImGui::IsItemVisible() && min.y >= origin.y)
                        modelHighlight->set_meta("hierarchy_row", Vector2(min.x + 20, (min.y + max.y) * 0.5f));
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    void draw_hierarchy() {
        if (hierarchyAudit && modelHighlight) modelHighlight->remove_meta("hierarchy_row");
        const float height = ImGui::GetContentRegionAvail().y;
        if (ImGui::BeginChild("HierarchyTree", ImVec2(0, std::max(1.0f, height * 0.58f)), true,
                ImGuiWindowFlags_HorizontalScrollbar)) {
            for (int file = 0; file < 2; ++file) {
                if (bundleNames[file].empty()) continue;
                const bool revealBundle = revealInstance && selectedModel >= 0 && hierarchyModels[selectedModel].bundle == file;
                if (revealBundle) ImGui::SetNextItemOpen(true);
                if (!ImGui::TreeNodeEx(bundleNames[file].c_str(), ImGuiTreeNodeFlags_DefaultOpen)) continue;
                for (const auto& section : hierarchySections) {
                    const auto belongs = [&](int index) { return hierarchyModels[index].bundle == file; };
                    if (file == 0 && std::none_of(section.models.begin(), section.models.end(), belongs)) continue;
                    if (file == 1 && !section.models.empty() &&
                        std::none_of(section.models.begin(), section.models.end(), belongs)) continue;
                    if (revealBundle && hierarchyModels[selectedModel].section == static_cast<int>(section.number))
                        ImGui::SetNextItemOpen(true);
                    const bool sectionSelected = selectedSection >= 0 &&
                        sections[selectedSection].number == static_cast<int>(section.number);
                    const auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
                        (sectionSelected ? ImGuiTreeNodeFlags_Selected : 0);
                    const bool opened = ImGui::TreeNodeEx(
                        reinterpret_cast<const void*>(static_cast<uintptr_t>(section.number) + 1),
                        flags, "Section %u", section.number);
                    if (ImGui::IsItemClicked(0) && !ImGui::IsItemToggledOpen()) {
                        const auto found = std::find_if(sections.begin(), sections.end(), [&](const auto& entry) {
                            return entry.number == static_cast<int>(section.number);
                        });
                        select_model(-1);
                        selectedSection = found == sections.end() || sectionSelected ? -1 :
                            static_cast<int>(found - sections.begin());
                        selectedEntity = -1;
                        filterDirty = true;
                        update_selection();
                    }
                    if (opened) {
                        for (const int index : section.models) if (belongs(index)) draw_model(index);
                        ImGui::TreePop();
                    }
                }
                if (!bundleModels[file].empty() && ImGui::TreeNode("Model definitions")) {
                    for (const int index : bundleModels[file]) draw_model(index);
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
        if (ImGui::BeginChild("HierarchyProperties", ImVec2(0, 0), true)) {
            if (selectedModel < 0) ImGui::TextUnformatted("Select a model to inspect it.");
            else {
                const auto& model = hierarchyModels[selectedModel];
                ImGui::TextWrapped("%s", model.name.c_str());
                ImGui::TextWrapped("%s", bundleNames[model.bundle].c_str());
                if (model.section >= 0) ImGui::Text("Section %d", model.section);
                ImGui::Text("Model index %zu", model.modelIndex);
                for (std::size_t lod = 0; lod < model.keys.size(); ++lod)
                    if (model.keys[lod]) ImGui::Text("LOD %zu   0x%08X%s", lod, model.keys[lod],
                        model.resolved[lod] ? "" : " (unresolved)");
                if (selectedInstance >= 0) {
                    const auto& item = hierarchyInstances[selectedInstance];
                    const auto& entry = *item.source;
                    ImGui::Text("Instance %zu   Flags 0x%04X", item.index, static_cast<unsigned>(entry.flags));
                    ImGui::Text("HP2 XYZ  %.3f, %.3f, %.3f", entry.position[0], entry.position[1], entry.position[2]);
                    ImGui::TextUnformatted("HP2 rotation matrix");
                    for (int row = 0; row < 3; ++row) ImGui::Text("%.4f   %.4f   %.4f",
                        entry.rotation[row * 3] / 16384.0f, entry.rotation[row * 3 + 1] / 16384.0f,
                        entry.rotation[row * 3 + 2] / 16384.0f);
                    ImGui::Text("Bounds min  %d, %d, %d", entry.minimum[0], entry.minimum[1], entry.minimum[2]);
                    ImGui::Text("Bounds max  %d, %d, %d", entry.maximum[0], entry.maximum[1], entry.maximum[2]);
                    if (item.draw.mesh.is_null()) ImGui::TextUnformatted("Hidden in the current view");
                } else ImGui::TextUnformatted("No visible placement");
            }
        }
        ImGui::EndChild();
    }

    void draw_entities(float dpi) {
        if (entityFilter.Draw("Search##entities", -80.0f)) filterDirty = true;
        for (std::size_t i = 0; i < categories.size(); ++i)
            if (ImGui::Checkbox(categories[i], &categoryVisible[i])) {
                filterDirty = true;
                update_mode();
            }
        if (selectedSection >= 0) {
            ImGui::Text("Section %d", sections[selectedSection].number);
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear section")) {
                selectedSection = selectedEntity = -1;
                filterDirty = true;
                update_selection();
            }
        }
        if (filterDirty) rebuild_filter();
        ImGui::Text("%zu entities", filtered.size());
        const float detailsHeight = selectedEntity >= 0 ? 105 * dpi : 0;
        if (ImGui::BeginChild("EntityList", ImVec2(0, -detailsHeight), true)) {
            ImGuiListClipper clipper(static_cast<int>(filtered.size()), ImGui::GetTextLineHeightWithSpacing());
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                    const auto index = static_cast<int>(filtered[row]);
                    const auto& entity = entities[index];
                    ImGui::PushID(index);
                    if (ImGui::Selectable(entity.label.c_str(), selectedEntity == index,
                            ImGuiSelectableFlags_AllowDoubleClick)) {
                        selectedEntity = index;
                        update_selection();
                        if (ImGui::IsMouseDoubleClicked(0)) focus(entity.bounds);
                    }
                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();
        if (selectedEntity >= 0) {
            const auto& entity = entities[selectedEntity];
            const auto point = source_vector(entity.position);
            ImGui::TextWrapped("%s", entity.label.c_str());
            ImGui::Text("XYZ %.2f, %.2f, %.2f", point.x, point.y, point.z);
            ImGui::TextWrapped("%s", entity.details.c_str());
            if (ImGui::Button("Focus selected")) focus(entity.bounds);
        }
    }

    void attach();
};

TrackViewer::TrackViewer(Node3D& host) : storage_(std::make_unique<Storage>(host)) {}
TrackViewer::~TrackViewer() {
    if (storage_->post.is_valid()) storage_->post->stop();
    storage_->capture(false);
}

void TrackViewer::open(bool inRace, int currentTrack, int language) {
    auto& s = *storage_;
    if (s.active || s.confirm) return;
    if (s.language != language) s.tracks.clear();
    s.preferredTrack = currentTrack;
    s.language = language;
    if (inRace) s.confirm = true;
    else s.request = {Action::Open, currentTrack};
}

TrackViewer::Request TrackViewer::take_request() {
    const auto request = storage_->request;
    storage_->request = {};
    return request;
}

void TrackViewer::begin_load(int track, const GameResources& files) {
    auto& s = *storage_;
    s.capture(false);
    s.loading = true;
    s.failure.clear();
    s.selectedTrack = track;
    if (s.tracks.empty()) {
        const TrackInfoResources catalogue(files.Load("GLOBAL/GLOBALB.LZC"));
        constexpr std::array<const char*, 8> languages{"ENGLISH", "FRENCH", "GERMAN", "ITALIAN",
            "SPANISH", "SWEDISH", "LONGEST", "ID"};
        const auto locale = static_cast<std::size_t>(std::clamp(s.language, 0, 7));
        auto languageFile = std::string("LANGUAGES/FE_") + languages[locale] + ".BIN";
        if (!files.TryOpenPhysical(languageFile)) languageFile = "LANGUAGES/FE_ENGLISH.BIN";
        LanguageManager language;
        if (!language.Loader(files.Load(languageFile), &files))
            throw std::runtime_error("Could not decode the original track captions");
        for (std::size_t index = 0; index < catalogue.Records().size(); ++index) {
            const int id = static_cast<int>(index + 1);
            if (const auto* track = catalogue.Find(id)) {
                const auto* name = language.GetString(GetTheRightTrackLanguageID(track, 0));
                s.tracks.push_back({id, std::to_string(id) + "  " + (name && *name ? name : track->name)});
            }
        }
        if (s.tracks.empty()) throw std::runtime_error("The original track catalogue is empty");
    }
    if (!s.selectedTrack) s.selectedTrack = s.tracks.front().id;
    if (std::none_of(s.tracks.begin(), s.tracks.end(), [&](const auto& item) { return item.id == s.selectedTrack; }))
        throw std::runtime_error("Track is absent from the original catalogue");
    s.source = std::make_unique<TrackViewerScene>(files);
    s.source->load(s.selectedTrack);
}

void TrackViewer::show() {
    auto& s = *storage_;
    s.active = true;
    s.container->show();
    s.output->set_update_mode(SubViewport::UPDATE_ALWAYS);
}

void TrackViewer::error(const char* message) {
    storage_->failure = message;
    storage_->loading = false;
}

void TrackViewer::close() {
    auto& s = *storage_;
    s.capture(false);
    s.active = false;
    s.confirm = false;
    s.container->hide();
    s.output->set_update_mode(SubViewport::UPDATE_DISABLED);
}

bool TrackViewer::active() const { return storage_->active; }
bool TrackViewer::modal() const { return storage_->confirm; }
bool TrackViewer::captured() const { return storage_->captured; }
int TrackViewer::track() const { return storage_->selectedTrack; }
std::uint64_t TrackViewer::generation() const { return storage_->source && storage_->camera ? storage_->source->generation() : 0; }
std::uint32_t TrackViewer::step() const { return storage_->source && storage_->camera ? storage_->source->clock().worldTimeFrames : 0; }
std::size_t TrackViewer::placement_count() const { return storage_->placementCount; }
std::size_t TrackViewer::scenery_section_count() const {
    return storage_->sceneryVisibility ? storage_->sceneryVisibility->statistics().sections : 0;
}
int TrackViewer::entity_count() const { return static_cast<int>(storage_->entities.size()); }
int TrackViewer::catalogue_size() const { return static_cast<int>(storage_->tracks.size()); }

bool TrackViewer::input(const Ref<InputEvent>& event) {
    auto& s = *storage_;
    if (!s.active) return false;
    if (const Ref<InputEventMouseMotion> motion = event; motion.is_valid() && s.captured) {
        const auto relative = motion->get_screen_relative();
        s.yaw = std::remainder(s.yaw - relative.x * 0.0025f, 2.0f * 3.14159265358979323846f);
        s.pitch = std::clamp(s.pitch - relative.y * 0.0025f, -1.55f, 1.55f);
        s.poseDirty = true;
    }
    if (const Ref<InputEventMouseButton> button = event; button.is_valid()) {
        if (!button->is_pressed() && button->get_button_index() == MOUSE_BUTTON_RIGHT) s.capture(false);
        if (button->is_pressed() && s.camera && (s.captured || s.viewportRect.has_point(button->get_position()))) {
            const auto which = button->get_button_index();
            if (which == MOUSE_BUTTON_RIGHT) s.capture(true);
            if (which == MOUSE_BUTTON_LEFT && !s.captured) s.pick(button->get_position());
            if (which == MOUSE_BUTTON_WHEEL_UP) s.speed = std::min(4000.0f, s.speed * 1.25f);
            if (which == MOUSE_BUTTON_WHEEL_DOWN) s.speed = std::max(5.0f, s.speed / 1.25f);
        }
    }
    if (s.captured) {
        if (event->is_action_pressed("hp2_pause")) s.capture(false);
        if (Ref<InputEventKey>(event).is_valid())
            for (std::size_t index = 0; index < movementActions.size(); ++index)
                if (event->is_action(movementActions[index]) && !event->is_echo())
                    s.movement[index] = event->is_pressed();
    }
    return true;
}

void TrackViewer::advance(double delta, bool blocked) {
    auto& s = *storage_;
    if (!s.active) return;
    if (blocked || !s.host.get_window()->has_focus()) {
        s.capture(false);
        s.flight.reset();
    }
    if (s.flight && s.camera) {
        auto& flight = *s.flight;
        flight.elapsed += static_cast<float>(delta);
        const float t = std::clamp(flight.elapsed / 0.5f, 0.0f, 1.0f);
        s.position = flight.from.lerp(flight.to, t);
        s.yaw = flight.fromYaw + flight.yawDelta * t;
        s.pitch = flight.fromPitch + (flight.toPitch - flight.fromPitch) * t;
        s.poseDirty = true;
        if (t == 1.0f) s.flight.reset();
    }
    if (s.captured && s.camera) {
        const auto& move = s.movement;
        const auto basis = Basis::from_euler(Vector3(s.pitch, s.yaw, 0));
        const auto planar = basis.xform(Vector3(move[1] - move[0], 0, move[3] - move[2]));
        const auto direction = (planar + Vector3(0, move[4] - move[5], 0)).limit_length();
        const float boost = move[6] ? 4.0f : 1.0f;
        if (!direction.is_zero_approx()) {
            s.position += direction * (s.speed * boost * std::clamp(static_cast<float>(delta), 0.0f, 0.05f));
            s.poseDirty = true;
        }
    }
    if (s.poseDirty) s.update_camera();
}

void TrackViewer::draw(float width, float height, float dpi, float fps, bool blocked, RaceEffectOptions& effects) {
    auto& s = *storage_;
    if (s.confirm) ImGui::OpenPopup("Leave current race?");
    if (ImGui::BeginPopupModal("Leave current race?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Open Track Viewer and leave the current race?");
        if (ImGui::Button("Open Track Viewer")) {
            s.confirm = false;
            s.request = {Action::Open, s.preferredTrack};
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            s.confirm = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (!s.active) return;
    const float splitter = 6.0f * dpi;
    const float minSidebar = std::min(185.0f * dpi, width * 0.46f);
    const float maxSidebar = std::max(minSidebar, width * 0.70f);
    const float sidebar = s.inspectorVisible ? std::clamp(s.inspectorWidth * dpi, minSidebar, maxSidebar) : 0.0f;
    char positionText[96] = "Position --";
    char lookAtText[96] = "Look At --";
    if (s.camera) {
        const auto pose = s.camera->get_global_transform();
        const auto position = source_vector(pose.origin);
        // The free camera has no fixed target. Use a point one unit down its view axis.
        const auto target = source_vector(pose.origin - pose.basis.get_column(2).normalized());
        std::snprintf(positionText, sizeof(positionText), "Position (%.3f, %.3f, %.3f)",
            position.x, position.y, position.z);
        std::snprintf(lookAtText, sizeof(lookAtText), "Look At (%.3f, %.3f, %.3f)",
            target.x, target.y, target.z);
    }
    const auto& style = ImGui::GetStyle();
    const float controlsWidth = ImGui::CalcTextSize("TrackMain Menu000 FPSInspector").x +
        10 * style.FramePadding.x + 5 * style.ItemSpacing.x + 2 * style.WindowPadding.x;
    const float coordinatesWidth = ImGui::CalcTextSize(
        "Position (-0000.000, -0000.000, -0000.000)  Look At (-0000.000, -0000.000, -0000.000)").x +
        2 * style.ItemSpacing.x;
    const bool inlineCoordinates = width >= controlsWidth + 140 * dpi + coordinatesWidth;
    const bool splitCoordinates = ImGui::CalcTextSize(positionText).x +
        ImGui::CalcTextSize(lookAtText).x + style.ItemSpacing.x + 2 * style.WindowPadding.x > width;
    const float top = ImGui::GetFrameHeightWithSpacing() + 2 * style.WindowPadding.y +
        (inlineCoordinates ? 0 : ImGui::GetTextLineHeightWithSpacing() * (splitCoordinates ? 2 : 1));
    const float viewLeft = s.inspectorVisible ? sidebar + splitter : 0.0f;
    const Rect2 layout(viewLeft, top, std::max(1.0f, width - viewLeft), std::max(1.0f, height - top));
    if (layout != s.viewportRect) {
        s.viewportRect = layout;
        s.container->set_position(layout.position);
        s.container->set_size(layout.size);
    }
    if (blocked) s.capture(false);
    const auto inputFlags = s.captured ? ImGuiWindowFlags_NoInputs : 0;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(width, top));
    if (ImGui::Begin("Track Viewer Toolbar", nullptr, panelFlags | inputFlags | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Track");
        ImGui::SameLine();
        const float available = width - controlsWidth - (inlineCoordinates ? coordinatesWidth : 0);
        ImGui::SetNextItemWidth(std::clamp(available, 40.0f, 280 * dpi));
        const auto selected = std::find_if(s.tracks.begin(), s.tracks.end(), [&](const auto& track) {
            return track.id == s.selectedTrack;
        });
        const char* caption = selected != s.tracks.end() ? selected->caption.c_str() : "Loading catalogue";
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.13f, 0.17f, 1.0f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, std::max(64.0f, height - top - 12 * dpi)));
        if (ImGui::BeginCombo("##Track", caption, ImGuiComboFlags_HeightLarge)) {
            for (const auto& track : s.tracks) {
                const bool current = track.id == s.selectedTrack;
                if (ImGui::Selectable(track.caption.c_str(), current) && (!current || !s.failure.empty()))
                    s.request = {Action::Load, track.id};
                if (current) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button("Main Menu")) s.request = {Action::Close, 0};
        ImGui::SameLine();
        ImGui::Text("%.0f FPS", fps);
        ImGui::SameLine();
        if (ImGui::Button("Inspector")) s.inspectorVisible = !s.inspectorVisible;
        if (s.hierarchyAudit && s.modelHighlight) {
            const auto min = ImGui::GetItemRectMin();
            const auto max = ImGui::GetItemRectMax();
            s.modelHighlight->set_meta("inspector_button", Vector2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f));
        }
        if (inlineCoordinates) ImGui::SameLine(0, 2 * style.ItemSpacing.x);
        ImGui::TextUnformatted(positionText);
        if (!splitCoordinates) ImGui::SameLine();
        ImGui::TextUnformatted(lookAtText);
    }
    ImGui::End();
    if (!s.inspectorVisible || sidebar <= 0) return;
    ImGui::SetNextWindowPos(ImVec2(sidebar, top));
    ImGui::SetNextWindowSize(ImVec2(splitter, layout.size.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Track Viewer Splitter", nullptr, panelFlags | inputFlags |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::InvisibleButton("Resize inspector", ImVec2(splitter, layout.size.y));
        const bool dragging = ImGui::IsItemActive();
        if (ImGui::IsItemHovered() || dragging) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (dragging)
            s.inspectorWidth = std::clamp(ImGui::GetIO().MousePos.x, minSidebar, maxSidebar) / dpi;
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::SetNextWindowPos(ImVec2(0, top));
    ImGui::SetNextWindowSize(ImVec2(sidebar, s.viewportRect.size.y));
    if (ImGui::Begin("Track Viewer Inspector", nullptr, panelFlags | inputFlags)) {
        if (!s.failure.empty()) ImGui::TextWrapped("Track %d: %s", s.selectedTrack, s.failure.c_str());
        else if (s.loading) ImGui::TextUnformatted("Loading track...");
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3 * dpi, style.FramePadding.y));
        if (ImGui::BeginTabBar("Inspector", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
            const auto hierarchyFlags = s.revealInstance ? ImGuiTabItemFlags_SetSelected : 0;
            if (ImGui::BeginTabItem("Hierarchy", nullptr, hierarchyFlags)) {
                s.draw_hierarchy();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Entities")) { s.draw_entities(dpi); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Camera")) {
                if (ImGui::SliderFloat("FOV", &s.fov, 10, 100, "%.0f deg")) s.poseDirty = true;
                ImGui::SliderFloat("Speed", &s.speed, 5, 4000, "%.0f m/s");
                if (ImGui::Button("Reset camera")) {
                    s.flight.reset();
                    s.position = s.start;
                    s.yaw = s.startYaw;
                    s.pitch = s.startPitch;
                    s.fov = 60;
                    s.poseDirty = true;
                }
                if (ImGui::Button("Frame track") && s.camera) s.focus(s.routeBounds);
                ImGui::Separator();
                if (s.camera && ImGui::Button("Copy coordinates"))
                    DisplayServer::get_singleton()->clipboard_set(String(positionText) + "\n" + lookAtText);
                ImGui::TextWrapped("%s", positionText);
                ImGui::TextWrapped("%s", lookAtText);
                ImGui::TextWrapped("Hold right mouse to fly. WASD to move, Q/E for height, Shift to boost. Left-click to select.");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Render")) {
                if (ImGui::Checkbox("Entity view", &s.debugView)) {
                    if (s.debugView) s.wireframeView = false;
                    s.update_mode();
                }
                if (ImGui::Checkbox("Wireframe view", &s.wireframeView)) {
                    if (s.wireframeView) s.debugView = false;
                    s.update_mode();
                }
                if (ImGui::Checkbox("Section bounds", &s.sectionsVisible)) s.update_mode();
                if (ImGui::Checkbox("Entity markers", &s.entitiesVisible)) s.update_mode();
                ImGui::Checkbox("Track visibility zones", &effects.sceneryVisibility);
                ImGui::Checkbox("Scene particles", &effects.particles);
                ImGui::Checkbox("Sun and glare", &effects.sunGlare);
                ImGui::Checkbox("Fog", &effects.fog);
                ImGui::Checkbox("Depth of field", &effects.depthOfField);
                ImGui::Checkbox("Black and white", &effects.monochrome);
                ImGui::Checkbox("Color filter", &effects.colorFilter);
                ImGui::Checkbox("Motion blur", &effects.motionBlur);
                ImGui::Checkbox("Roadshine", &effects.roadshine);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::PopStyleVar();
    }
    ImGui::End();
    if (s.filterDirty && (s.debugView || s.entitiesVisible)) s.rebuild_filter();
}

void TrackViewer::Storage::attach() {
    auto& s = *this;
    if (RenderingServer::get_singleton()->get_rendering_device())
        s.view = RacePostPresentation::create_scene(*s.output);
    s.trackScene = memnew(Node3D);
    s.view->add_child(s.trackScene);
    s.trackScene->set_name("RaceScene");
    s.worldObjects = std::make_unique<WorldObjectPresentation>(*s.trackScene);
    s.sceneryVisibility = std::make_unique<TrackSceneryLod>(*s.trackScene, TrackSceneryLod::View::Viewer);
    std::vector<TrackPlacementVisibility> movable;
    // A scene inspection never promotes authored placements to physics bodies.
    const CollisionObjectRegistry staticPlacements{};
    std::vector<TrackInspectionPlacement> inspection;
    s.placementCount = attach_track(*s.trackScene, s.source->scenery(), &s.source->sky(), false,
        staticPlacements, movable, *s.worldObjects, nullptr, false, s.sceneryVisibility.get(), &inspection);
    s.index_hierarchy(inspection);
    s.effects = std::make_unique<RaceEffectsPresentation>(*s.trackScene);
    s.effects->prepare_track_coronas(s.source->track_coronas(), s.source->ntsc(), false);
    s.effects->prepare_particles(s.source->particle_assets());
    s.camera = memnew(Camera3D);
    s.view->add_child(s.camera);
    s.camera->make_current();
    s.camera->set_cull_mask_value(3, false);
    s.camera->set_cull_mask(s.camera->get_cull_mask() & ~trackReflectionLayer);
    s.camera->set_cull_mask_value(10, true);
    if (s.view != s.output) {
        s.post.instantiate();
        s.post->attach(*s.view, *s.output, *s.camera);
    }
    s.lines = memnew(Node3D);
    s.view->add_child(s.lines);
    s.lines->set_name("TrackViewerEntities");
    s.sectionLines = make_lines(*s.lines, Color(0.7f, 0.83f, 0.95f));
    s.entityLines = {make_lines(*s.lines, Color(1, 0.63f, 0.12f)),
        make_lines(*s.lines, Color(0.23f, 0.83f, 1)), make_lines(*s.lines, Color(1, 0.33f, 0.27f))};
    s.selectionLines = make_lines(*s.lines, Color(0.35f, 1, 0.35f));
    s.modelHighlight = memnew(MeshInstance3D);
    s.modelHighlight->set_name("TrackViewerSelection");
    s.modelHighlight->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    Ref<ShaderMaterial> highlight;
    highlight.instantiate();
    highlight->set_shader(load_shader("res://shader/track_viewer_selection.gdshader"));
    s.modelHighlight->set_material_override(highlight);
    s.hierarchyAudit = OS::get_singleton()->get_environment("HP2_MVP_VIEWER_HIERARCHY") == "1";
    if (s.hierarchyAudit) {
        PackedStringArray names;
        for (const auto& name : s.bundleNames) names.push_back(name.c_str());
        s.modelHighlight->set_meta("track_bundles", names);
    }
    s.modelHighlight->hide();
    s.lines->add_child(s.modelHighlight);
    s.index(*s.source);
    const auto& route = s.source->route();
    if (route.nodeCount < 2) throw std::runtime_error("Track has no camera start route");
    const auto& first = route.nodes[0];
    const auto& next = route.nodes[1];
    const auto forward = Vector3(next.point[0] - first.point[0], 0, first.point[1] - next.point[1]).normalized();
    TerrainSurface terrain{};
    float point[4]{first.point[0], first.point[1]};
    bool found{};
    if (terrainSurfaceInitialize(&terrain, &s.source->terrain())) {
        point[2] = terrainSurfaceElevation(&terrain, point, nullptr, nullptr, &found);
        terrainSurfaceDispose(&terrain);
    }
    s.start = race_vector(point) + Vector3(0, 10, 0) - forward * 16;
    s.startYaw = std::atan2(-forward.x, -forward.z);
    s.startPitch = -0.16f;
    s.position = s.start;
    s.yaw = s.startYaw;
    s.pitch = s.startPitch;
    s.camera->set_keep_aspect_mode(Camera3D::KEEP_HEIGHT);
    s.camera->set_perspective(s.fov, 0.1f, 30000.0f);
    Ref<Environment> background;
    background.instantiate();
    background->set_background(Environment::BG_COLOR);
    background->set_bg_color(Color(0, 0, 0));
    background->set_glow_blend_mode(Environment::GLOW_BLEND_MODE_ADDITIVE);
    background->set_glow_intensity(0.35f);
    background->set_glow_strength(0.8f);
    s.camera->set_environment(background);
    s.update_mode();
    s.update_camera();
    s.loading = false;
}

void TrackViewer::detach() {
    auto& s = *storage_;
    s.capture(false);
    s.flight.reset();
    if (s.lines) {
        s.view->remove_child(s.lines);
        s.lines->queue_free();
    }
    s.effects.reset();
    s.worldObjects.reset();
    s.sceneryVisibility.reset();
    s.effectsFrame = {};
    if (s.post.is_valid()) s.post->detach();
    s.post.unref();
    for (auto* node : {static_cast<Node3D*>(s.camera), s.trackScene}) {
        if (!node) continue;
        s.view->remove_child(node);
        node->queue_free();
    }
    if (s.view != s.output) {
        s.output->remove_child(s.view);
        s.view->queue_free();
        s.view = s.output;
    }
    s.pickFaces.clear();
    s.hierarchyInstances.clear();
    s.hierarchyModels.clear();
    s.hierarchySections.clear();
    s.bundleModels = {};
    s.bundleNames = {};
    s.selectedModel = s.selectedInstance = -1;
    s.revealInstance = false;
    s.source.reset();
    s.placementCount = 0;
    s.camera = nullptr;
    s.trackScene = s.lines = nullptr;
    s.sectionLines = s.selectionLines = nullptr;
    s.modelHighlight = nullptr;
    s.entityLines.fill(nullptr);
    s.entities.clear();
    s.sections.clear();
    s.filtered.clear();
    s.selectedEntity = s.selectedSection = -1;
    s.filterDirty = true;
}

void TrackViewer::update_scene(double delta, const RaceEffectOptions& options) {
    auto& s = *storage_;
    if (!s.source || !s.source->service_load()) return;
    const bool loaded = !s.camera;
    if (loaded) s.attach();
    const auto eye = source_vector(s.camera->get_global_position());
    const float position[]{eye.x, eye.y, eye.z};
    s.sceneryVisibility->update_camera(*s.camera, position, 0.0f, options.sceneryVisibility, loaded);
    s.source->set_eye(position);
    s.source->advance(delta);
    const auto& clock = s.source->clock();
    const auto offset = clock.worldTimeElapsed * s.source->fraction();
    RenderingServer::get_singleton()->global_shader_parameter_set("hp2_animation_clock",
        Vector4(clock.worldTimeFrames + offset * 60.0f, clock.worldTimer * 0.0002500000118743628f + offset,
            clock.realTimeFrames, clock.worldTimeFrames));
    s.source->take_effects(s.effectsFrame);
    const bool showEffects = !s.debugView && !s.wireframeView;
    if (s.post.is_valid())
        s.post->update(s.source->post(), loaded, showEffects ? options : RaceEffectOptions{false, false, false, false, false, false, false});
    s.effects->update(s.effectsFrame, *s.camera, nullptr, s.source->generation(), s.source->ntsc(),
        clock.worldTimeFrames, showEffects && options.particles, showEffects && options.sunGlare);
}

void TrackViewer::return_to_main_menu(MainMenuScreenServices& menus, const void*) {
    auto& screens = *menus.screens;
    const auto* current = screens.active ? screens.active->configuration->name : "";
    if (std::string_view(current) == "BootAutoload" || std::string_view(current) == "Language Options Pre FE") {
        // Keep boot profile transfer and language selection in the original flow.
        auto& stack = screens.stack;
        for (std::size_t i = 0; i < stack.count; ++i) {
            if (std::string_view(stack.names[i].data()) == "Title Screen") {
                std::snprintf(stack.names[i].data(), stack.names[i].size(), "%s", "New Main Menu");
                return;
            }
        }
    }
    screens.StopMovie();
    screens.stack.count = 0;
    screens.ChangeToScreen("New Main Menu", false);
}
}
