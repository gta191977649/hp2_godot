#pragma once

#include "assets/mesh_converter.hpp"

#include <godot_cpp/classes/skeleton3d.hpp>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace hp2 {

// Render between the last two visible samples using the runtime's step fraction.
// Seed both endpoints on spawn or seek so a retired pose never enters a new scene.
struct PoseInterpolation {
    struct Sample {
        godot::Transform3D root;
        std::vector<godot::Transform3D> bones;
    };
    Sample previous, current;
    std::uint32_t step{};
    bool valid{};

    void capture(std::uint32_t nextStep, const godot::Transform3D& root,
        const std::vector<std::array<float, 16>>* palette) {
        // Catch-up can skip simulation ticks between rendered frames. Keep the
        // last sampled endpoint so the first frame after catch-up can blend.
        const auto elapsed = nextStep - step;
        const bool continuous = valid && elapsed > 0 && elapsed <= 8;
        if (continuous) std::swap(previous, current);
        current.root = root;
        current.bones.resize(palette ? palette->size() : 0);
        for (std::size_t i = 0; i < current.bones.size(); ++i)
            current.bones[i] = race_transform((*palette)[i].data());
        if (!continuous) previous = current;
        step = nextStep;
        valid = true;
    }

    void apply(godot::Node3D& node, godot::Skeleton3D* skeleton, float fraction) const {
        const auto root = previous.root.interpolate_with(current.root, fraction);
        if (node.get_transform() != root) node.set_transform(root);
        if (!skeleton) return;
        for (std::size_t i = 0; i < current.bones.size(); ++i) {
            const auto pose = previous.bones[i].interpolate_with(current.bones[i], fraction);
            if (skeleton->get_bone_pose(i) != pose) skeleton->set_bone_pose(i, pose);
        }
    }
};

}
