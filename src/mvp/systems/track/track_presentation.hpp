#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/transform3d.hpp>

struct Scenery;
struct CollisionObjectRegistry;
struct EditorMarkerModel;
namespace godot { class Camera3D; class Node3D; }

namespace hp2 {

class SkyResources;
struct WorldObjectEntityState;
struct TrackPlacementVisibility;
class WorldObjectPresentation;
// Optional scene-inspection data from the same mesh and transform submitted for drawing.
struct TrackInspectionPlacement {
    std::uint32_t section;
    std::size_t instance;
    godot::Ref<godot::ArrayMesh> mesh;
    godot::Transform3D transform;
};
class TrackSceneryLod {
public:
    enum class View { Reflection, Race, Viewer };
    struct Statistics {
        std::size_t sections{}, treeNodes{}, candidates{}, placements{}, batchUpdates{};
        bool fallback{};
    };
    explicit TrackSceneryLod(godot::Node3D& root, View view = View::Reflection, bool treeCulling = true);
    ~TrackSceneryLod();
    void update(const float origin[3], float focal, float frustumReach, bool reset);
    void update_camera(const godot::Camera3D& camera, const float origin[3], float focal,
        bool zonesEnabled, bool reset);
    const Statistics& statistics() const;

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
    friend std::size_t attach_track(godot::Node3D&, const Scenery&, const SkyResources*, bool,
        const CollisionObjectRegistry&, std::vector<TrackPlacementVisibility>&,
        WorldObjectPresentation&, TrackSceneryLod*, bool, TrackSceneryLod*, std::vector<TrackInspectionPlacement>*);
};

class WorldObjectPresentation {
public:
    explicit WorldObjectPresentation(godot::Node3D& scene);
    ~WorldObjectPresentation();
    void prepare_animation(const EditorMarkerModel& model);
    void update(const std::vector<WorldObjectEntityState>& objects);

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

struct TrackPlacementVisibility {
    const std::uint16_t* flags{};
    godot::Node3D* node{};
};
inline constexpr std::uint32_t trackSkyLayer = 1u << 3;
inline constexpr std::uint32_t trackReflectionLayer = 1u << 4;
inline constexpr std::uint32_t trackMirrorSceneryLayer = 1u << 5;
inline constexpr std::uint32_t trackPrimarySceneryLayer = 1u << 6;
std::size_t attach_track(godot::Node3D& root, const Scenery& scenery, const SkyResources* sky, bool reverse, const CollisionObjectRegistry& objects,
    std::vector<TrackPlacementVisibility>& movablePlacements, WorldObjectPresentation& worldObjects,
    TrackSceneryLod* reflection = nullptr, bool sharedWorldReflection = false, TrackSceneryLod* primary = nullptr,
    std::vector<TrackInspectionPlacement>* inspection = nullptr);

}
