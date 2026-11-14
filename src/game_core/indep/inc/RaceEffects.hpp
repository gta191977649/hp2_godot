#pragma once

#include "indep/inc/AcidFX.hpp"
#include "indep/inc/Clans.hpp"
#include "indep/inc/eLight.hpp"
#include "indep/inc/FlailerChunks.hpp"

namespace hp2 {

struct RaceParticleEvent {
    ParticleEffectEvent event;
    const EditorMarkerTexture* texture{};
    std::uint64_t world{};
    std::uint32_t step{};
    bool frameEffects{};
    std::uint64_t replayEpoch{};
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

struct RaceParticleAsset {
    const WorldParticleDefinition* definition{};
    const EditorMarkerTexture* texture{};
};

// Port-side exhaust presentation inputs, sampled from the actual engine
// and authored exhaust marker list. The host owns its cosmetic emission.
struct RaceExhaustOutlet {
    std::uint64_t car{};
    std::size_t outlet{};
    std::array<float, 3> position{}, direction{}, velocity{};
    float throttle{};
    bool active{};
};

struct RaceEffectsFrame {
    // Port-side staging of authored world strips. Texture descriptors are
    // borrowed until the host uploads the frame; vertices are world-space.
    struct ShadowStrip {
        const EditorMarkerTexture* texture;
        std::size_t first, count;
        int view;
    };
    std::vector<ShadowStrip> shadows;
    std::vector<FlailerPolyVertex> shadowVertices;
    std::vector<RaceExhaustOutlet> exhaust;
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
// Port-side callback for the existing projected car shadow geometry.
void CollectCarShadowEffects(Car& car, const float transform[16], const CarCinematicScreen& view,
    int layout, LightContextBatch& lights, RaceEffectsFrame& output);
bool ParticleEffectVisible(const WorldParticleGroup& group,const CarCinematicScreen& view);

}
