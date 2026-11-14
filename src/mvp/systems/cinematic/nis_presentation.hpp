#pragma once

#include <memory>
#include <cstdint>

namespace godot { class Node3D; class Node; }

namespace hp2 {

struct NisManagerRegistry;

class NisPresentation {
public:
    explicit NisPresentation(godot::Node& host);
    ~NisPresentation();
    void update(godot::Node3D& scene, NisManagerRegistry& registry, bool monochrome,
        std::uint32_t step, std::uint64_t replayEpoch, float worldFrameFraction);
    void clear();
    std::size_t part_count() const;

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

}
