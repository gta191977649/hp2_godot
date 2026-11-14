#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

struct Scenery;
struct CollisionObjectRegistry;
struct EditorMarkerModel;
namespace godot { class Node3D; }

namespace hp2 {

class SkyResources;
struct WorldObjectEntityState;
struct TrackPlacementVisibility;
class WorldObjectPresentation;
class TrackReflection {
public:
    explicit TrackReflection(godot::Node3D& root);
    ~TrackReflection();
    void update(const float origin[3], float focal, float frustumReach, bool reset);

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
    friend std::size_t attach_track(godot::Node3D&, const Scenery&, const SkyResources*, bool,
        const CollisionObjectRegistry&, std::vector<TrackPlacementVisibility>&,
        WorldObjectPresentation&, TrackReflection*, bool);
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
std::size_t attach_track(godot::Node3D& root, const Scenery& scenery, const SkyResources* sky, bool reverse, const CollisionObjectRegistry& objects,
    std::vector<TrackPlacementVisibility>& movablePlacements, WorldObjectPresentation& worldObjects,
    TrackReflection* reflection = nullptr, bool sharedWorldReflection = false);

}
