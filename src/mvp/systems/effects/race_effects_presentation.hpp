#pragma once

#include "indep/inc/RaceEffects.hpp"
#include <memory>

namespace godot { class Camera3D; class Node3D; }

namespace hp2 {
class RaceEffectsPresentation {
public:
    explicit RaceEffectsPresentation(godot::Node3D& scene);
    ~RaceEffectsPresentation();
    void prepare_particles(const std::vector<RaceParticleAsset>& assets);
    void update(const RaceEffectsFrame& frame, godot::Camera3D& camera, godot::Camera3D* rear,
        std::uint64_t generation, bool ntsc, int worldFrames);
    void update_preview_lights(const std::vector<RaceLightEffect>& lights, godot::Camera3D& camera,int view);
    void update_preview_particles(const RaceEffectsFrame& frame);
    void reset();
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
