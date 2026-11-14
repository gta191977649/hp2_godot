#pragma once

#include "indep/inc/Flags.hpp"
#include "indep/inc/RaceEffects.hpp"
#include <array>
#include <memory>

struct TerrainTopology;
struct TrackRoute;
namespace hp2 { class ResourceFile; }

namespace hp2 {
class TrackViewerScene {
public:
    explicit TrackViewerScene(const GameResources& files);
    ~TrackViewerScene();
    void load(int track);
    bool service_load();
    void advance(double delta);
    std::vector<RaceParticleAsset> particle_assets();
    void take_effects(RaceEffectsFrame& frame);
    RacePostState post();
    const Scenery& scenery() const;
    std::array<const ResourceFile*, 2> track_bundles() const;
    const SkyResources& sky() const;
    const TrackRoute& route() const;
    TerrainTopology& terrain() const;
    const WorldParticleSystem& particles() const;
    void set_eye(const float* position);
    const GameTime& clock() const;
    bool ntsc() const;
    float fraction() const;
    std::uint64_t generation() const;

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
