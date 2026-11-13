#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

struct Scenery;
struct CollisionObjectRegistry;
namespace godot { class Node3D; }

namespace hp2 {

class SkyResources;
struct WorldObjectEntityState;

class WorldObjectPresentation {
public:
    explicit WorldObjectPresentation(godot::Node3D& scene);
    ~WorldObjectPresentation();
    void update(const std::vector<WorldObjectEntityState>& objects, int worldFrames);

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

struct TrackPlacementVisibility {
    const std::uint16_t* flags{};
    godot::Node3D* node{};
};
inline constexpr std::uint32_t trackSkyLayer = 1u << 3;
std::size_t attach_track(godot::Node3D& root, const Scenery& scenery, const SkyResources* sky, bool reverse, const CollisionObjectRegistry& objects,
    std::vector<TrackPlacementVisibility>& movablePlacements);

}
