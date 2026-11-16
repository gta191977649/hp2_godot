#pragma once

#include "indep/inc/RaceEffects.hpp"
#include <memory>

namespace godot { class Camera3D; class Node3D; }

namespace hp2 {
class RaceEffectsPresentation {
public:
    explicit RaceEffectsPresentation(godot::Node3D& scene);
    ~RaceEffectsPresentation();
    void prepare_track_coronas(const std::vector<RaceLightEffect>& lights, bool ntsc, bool rear);
    void prepare_particles(const std::vector<RaceParticleAsset>& assets);
    void update_water_sky(godot::Camera3D& camera);
    void update(const RaceEffectsFrame& frame, godot::Camera3D& camera, godot::Camera3D* rear,
        std::uint64_t generation, bool ntsc, int worldFrames, bool particlesVisible, bool sunGlareVisible);
    void update_preview_effects(const RaceEffectsFrame& frame, godot::Camera3D& camera,int view);
    void update_preview_particles(const RaceEffectsFrame& frame, float sourceFocal = 0.0f);
    void reset();
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
