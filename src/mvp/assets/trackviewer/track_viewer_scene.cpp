#include "track_viewer_scene.hpp"

#include "bware/inc/bChunk.hpp"
#include "bware/inc/bFile.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/Texture.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/WeatherMan.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/eMath.hpp"

#include <algorithm>
#include <stdexcept>

namespace hp2 {
namespace {
constexpr double stepSeconds = 1.0 / 60.0;
std::uint64_t nextGeneration{};
}

struct TrackViewerScene::Storage {
    explicit Storage(const GameResources& source) : files(source) {
        eMathInit();
        gameTimeInitialize(&clock);
        gameTimeUseNativeTiming(&clock);
        gameTimeSetVideoMode(&clock, files.TryOpenPhysical("PAL.TAG") ? HP2_VIDEO_PAL : HP2_VIDEO_NTSC);
        log.clock = &clock;
        cameras.clock = &clock;
        for (int index : {0, 1, 2}) cameras.screens[index].view = &primary;
        ecstasyViewProjectionInitialize(&cameras.projection, 512);
        resources = std::make_unique<ApplicationResourceRuntime>(files, cameras, log, CarInfoAvailability::FullGame);
    }

    ~Storage() {
        if (!resources) return;
        auto& particles = resources->FX().Particles();
        particles.publishEffect = nullptr;
        particles.presentationContext = nullptr;
        worldParticlesKillAllActiveEffects(&particles);
        sky.reset();
        resources->Registry().Sun().UnloadTrack();
        (void)resources->Loader().Shutdown();
    }

    const GameResources& files;
    GameTime clock{};
    ReplayIntegerLog log{};
    CarCinematicServices cameras{};
    CarCinematicView primary{};
    std::unique_ptr<ApplicationResourceRuntime> resources;
    ResourceFile* trackA{};
    ResourceFile* trackFile{};
    std::unique_ptr<SkyResources> sky;
    std::vector<RaceParticleEvent> events;
    std::vector<RaceLightEffect> lights;
    std::uint64_t generation{++nextGeneration};
    double remainder{};

    static void publish(void* context, const ParticleEffectEvent& event) {
        auto& scene = *static_cast<Storage*>(context);
        const auto* texture = event.operation == ParticleEffectOperation::Spawn ?
            scene.resources->Registry().Effects().Texture(*event.definition) : nullptr;
        scene.events.push_back({event, texture, scene.generation,
            static_cast<std::uint32_t>(scene.clock.worldTimeFrames), false, 0});
    }
};

TrackViewerScene::TrackViewerScene(const GameResources& files) : storage_(std::make_unique<Storage>(files)) {}
TrackViewerScene::~TrackViewerScene() = default;

void TrackViewerScene::load(int track) {
    auto& s = *storage_;
    auto& loader = s.resources->Loader();
    const auto global = s.resources->GlobalFileBindings();
    loader.LoadResourceFile("GLOBAL/GLOBALA.BUN", 0, 6, global);
    auto* globals = loader.LoadResourceFile("GLOBAL/GLOBALB.LZC", 0, 9, global);
    globals->ChangeFilenameForHotChunking("GLOBAL/GLOBALB.BUN");
    ResourceFileBindings binding;
    binding.sceneClock = &s.clock;
    binding.kind = ResourceFileKind::TrackAuxiliary;
    // Shared scene materials and FX textures are bundled with the in-game art.
    loader.LoadResourceFile("GLOBAL/INGAMEA.BUN", 2, 0, binding);
    auto* common = loader.LoadResourceFile("GLOBAL/INGAMEB.LZC", 2, 9, binding);
    common->ChangeFilenameForHotChunking("GLOBAL/INGAMEB.BUN");
    const auto id = std::to_string(track);
    s.trackA = loader.LoadResourceFile(("TRACKS/TRACKA" + id + ".BUN").c_str(), 3, 0, binding);
    binding.kind = ResourceFileKind::Track;
    binding.particles = &s.resources->FX().Particles();
    s.trackFile = loader.LoadResourceFile(("TRACKS/TRACKB" + id + ".LZC").c_str(), 3, 9, binding);
    s.trackFile->ChangeFilenameForHotChunking(("TRACKS/TRACKB" + id + ".BUN").c_str());
    binding.kind = ResourceFileKind::TrackAuxiliary;
    loader.LoadResourceFile(("TRACKS/TEX" + id + "TRACK.BIN").c_str(), 3, 1, binding);
    loader.LoadResourceFile(("TRACKS/TEX" + id + "LOCATION.BIN").c_str(), 3, 1, binding);
}

bool TrackViewerScene::service_load() {
    auto& s = *storage_;
    if (s.sky) return true;
    auto& loader = s.resources->Loader();
    loader.ServiceResourceLoading();
    if (!loader.IsResourceLoadingComplete()) return false;
    const auto& decoded = s.trackFile->Resources();
    auto& registry = s.resources->Registry();
    s.sky = std::make_unique<SkyResources>(registry.Models(), decoded.scenery->Native());
    registry.Sun().LoadTrack(registry);
    for (const auto* pack : registry.Lights().FlarePacks())
        for (const auto& flare : pack->flares)
            CollectLightEffect(flare, eMathIdentityMatrix, 1.0f, registry, s.lights);
    auto& particles = s.resources->FX().Particles();
    particles.presentationContext = &s;
    particles.publishEffect = &Storage::publish;
    return true;
}

void TrackViewerScene::advance(double delta) {
    auto& s = *storage_;
    s.remainder += std::clamp(delta, 0.0, 0.05);
    while (s.remainder >= stepSeconds) {
        s.remainder -= stepSeconds;
        if (!gameTimePrepareNativeRealTimestep(&s.clock, static_cast<float>(stepSeconds), nullptr))
            throw std::runtime_error("Scene clock rejected its logical step");
        gameTimeSetWorldElapsed(&s.clock, s.clock.realTimeElapsed);
        if (worldParticlesAdvance(&s.resources->FX().Particles(), s.clock.worldTimeElapsed) != WORLD_SERVICES_OK)
            throw std::runtime_error("Scene particle scheduler failed");
        texturePackRegistry.UpdateAnimations(s.resources->Registry().Models(), s.clock.worldTimeFrames, s.clock.realTimeFrames);
        gameTimeAdvanceNativeRealTime(&s.clock);
        gameTimeAdvanceNativeWorldTime(&s.clock);
    }
}

std::vector<RaceParticleAsset> TrackViewerScene::particle_assets() {
    const auto& catalogue = storage_->resources->Registry().Effects();
    std::vector<RaceParticleAsset> assets;
    for (auto* definition = *catalogue.DefinitionList(); definition; definition = definition->nextDefinition)
        if (const auto* texture = catalogue.Texture(*definition)) assets.push_back({definition, texture});
    return assets;
}

void TrackViewerScene::take_effects(RaceEffectsFrame& frame) {
    auto& s = *storage_;
    auto& registry = s.resources->Registry();
    frame.particles.clear();
    frame.particles.swap(s.events);
    frame.visibility.clear();
    const auto& system = particles();
    for (auto* group = system.firstGroup; group; group = group->next)
        frame.visibility.push_back({static_cast<std::size_t>(group - system.groupStorage),
            group->generation, false, true, false});
    if (frame.lights.empty()) frame.lights = s.lights;
    frame.sun.clear();
    auto& sun = registry.Sun();
    float margin{};
    for (const auto& row : sun.AuthoredFlares())
        if (row.kind == 0) margin = std::max(margin, row.size);
    for (const auto& row : sun.AuthoredFlares()) {
        if (row.size <= 0 || row.scale == 0) continue;
        if (const auto* texture = sun.TrackTexture(static_cast<std::size_t>(row.kind)))
            frame.sun.push_back({row, texture, sun.Vector(), 1.0f, std::abs(row.scale), row.scale > 0,
                &row == sun.AuthoredFlares().data(), margin, {}});
    }
}

RacePostState TrackViewerScene::post() {
    RacePostState result;
    WeatherQuery weather{};
    if (weatherManQuery(&weather, storage_->primary.current.eye) != WEATHER_MAN_OK)
        throw std::runtime_error("Scene camera lost its authored weather");
    result.fogEnabled = fogSettings.enabled;
    result.fog = {float(weather.fogColor & 255u), float((weather.fogColor >> 8) & 255u),
        float((weather.fogColor >> 16) & 255u), weather.fogStrength};
    result.fogRange = {weather.fogDistance, storage_->cameras.projection.depthRanges[1][1]};
    result.fogMode = fogSettings.modes[0];
    result.fogPower = fogSettings.power;
    return result;
}

const Scenery& TrackViewerScene::scenery() const { return storage_->trackFile->Resources().scenery->Native(); }
std::array<const ResourceFile*, 2> TrackViewerScene::track_bundles() const {
    return {storage_->trackA, storage_->trackFile};
}
const SkyResources& TrackViewerScene::sky() const { return *storage_->sky; }
const TrackRoute& TrackViewerScene::route() const { return storage_->trackFile->Resources().route->Native(); }
TerrainTopology& TrackViewerScene::terrain() const { return storage_->trackFile->Resources().terrain->Topology(); }
const WorldParticleSystem& TrackViewerScene::particles() const { return storage_->resources->FX().Particles(); }
void TrackViewerScene::set_eye(const float* position) { std::copy_n(position, 3, storage_->primary.current.eye); }
const GameTime& TrackViewerScene::clock() const { return storage_->clock; }
bool TrackViewerScene::ntsc() const { return gameTimeGetVideoMode(&storage_->clock) == HP2_VIDEO_NTSC; }
float TrackViewerScene::fraction() const { return static_cast<float>(storage_->remainder / stepSeconds); }
std::uint64_t TrackViewerScene::generation() const { return storage_->generation; }
}
