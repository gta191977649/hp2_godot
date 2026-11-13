#pragma once

#include "indep/inc/Smokeable.hpp"
#include "indep/inc/Polyhedra.hpp"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace hp2 {

class CollisionObjectData final {
public:
    CollisionObjectData(const std::vector<std::uint8_t>& track, TerrainTopology& topology);
    CollisionObjectData(const std::vector<std::uint8_t>& track, TerrainTopology* topology);
    ~CollisionObjectData();
    CollisionObjectData(const CollisionObjectData&) = delete;
    CollisionObjectData& operator=(const CollisionObjectData&) = delete;
    CollisionObjectData(CollisionObjectData&&) = delete;
    CollisionObjectData& operator=(CollisionObjectData&&) = delete;
    const CollisionBreakableDefinition* Find(std::uint32_t key) const noexcept;
    RigidBodyInfo* MutableRigidBodyInfo(std::uint32_t key) noexcept;
    CollisionPlacedObject* Placements() noexcept { return placements_.data(); }
    std::size_t PlacementCount() const noexcept { return placements_.size(); }
    std::size_t DefinitionCount() const noexcept { return definitions_.size(); }
    const CollisionBreakableDefinition& DefinitionAt(std::size_t index) const;
    // The car constructor borrows the hulls already owned by this loaded
    // ResourceFile. Exposing the same catalogue avoids decoding another copy.
    const std::vector<std::unique_ptr<CollisionResource>>& CollisionResources() const noexcept { return geometry_; }

private:
    struct Definition {
        std::string name;
        std::uint32_t key = 0;
        RigidBodyInfo body{};
        CollisionBreakableDefinition native{};
        std::array<std::uint32_t,4> fragmentKeys{};
        std::array<const CollisionBreakableDefinition*,4> fragments{};
        std::array<std::uint32_t,1> activationGroups{}, movingGroups{};
    };
    std::vector<std::unique_ptr<CollisionResource>> geometry_;
    std::vector<std::unique_ptr<Definition>> definitions_;
    std::vector<std::uint32_t> artKeys_;
    std::vector<CollisionPlacedObject> placements_;
};

} // namespace hp2
