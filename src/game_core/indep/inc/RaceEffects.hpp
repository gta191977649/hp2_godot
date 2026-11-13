#pragma once

#include "indep/inc/AcidFX.hpp"
#include "indep/inc/Clans.hpp"
#include "indep/inc/eLight.hpp"

namespace hp2 {

struct RaceParticleEvent {
    ParticleEffectEvent event;
    const EditorMarkerTexture* texture{};
    std::uint64_t world{};
    std::uint32_t step{};
    bool frameEffects{};
};

struct RaceLightEffect {
    const EditorMarkerTexture* texture{};
    std::array<float, 3> position{}, normal{};
    std::array<float, 4> minimumColor{}, maximumColor{};
    float minimumSize{}, maximumSize{}, exponent{}, bias{}, minimumScale{}, maximumScale{}, intensity{};
    bool twoSided{};
    int view{1};
};

struct RaceSunEffect {
    SunFlareParameters parameters;
    const EditorMarkerTexture* texture{};
    std::array<float, 3> position{};
    float sizeScale{1}, opacityScale{1};
    bool sampleSky{};
    bool depthTestDisc{};
    float margin{};
    std::array<float,2> positionOverride{};
};

struct RaceEffectsFrame {
    std::vector<RaceParticleEvent> particles;
    std::vector<RaceLightEffect> lights;
    std::vector<RaceSunEffect> sun;
    std::vector<SkidRenderQuad> skids;
    std::vector<SkidRenderQuad> rearSkids;
    struct ParticleVisibility {
        std::size_t slot;
        std::uint64_t generation;
        bool frameEffects, main, rear;
    };
    std::vector<ParticleVisibility> visibility;
};

void CollectLightEffect(const LightFlare& flare, const float transform[16], float intensity,
    ResourceRegistry& resources, std::vector<RaceLightEffect>& output);
void CollectCarLightEffects(Car& car, const float transform[16], std::int32_t worldFrames,
    std::vector<RaceLightEffect>& output);
bool ParticleEffectVisible(const WorldParticleGroup& group,const CarCinematicScreen& view);

}
