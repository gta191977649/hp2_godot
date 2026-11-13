#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <exception>

struct GameSession;
struct GameTime;
class DemoDiscManager;
struct WorldImpactAudio;
struct CollisionSystem;
struct CarDriverVisualAnimationDefinitions;
struct ApplicationLaunchOptions;
struct CarCinematicServices;
struct CarViewMoverRegistry;
struct CarFramePlayerRegistry;
struct WorldSimulation;
struct GameInputRouter;
struct RealDriverTuning;
struct Sound3dSampleRegistry;
struct PositionMarkerRegistry;
struct EditorMarkerModel;
namespace hp2 {
class TexturePack;
class ResourceLoader;
class CarEffectsResources;
class CarInfoResource;
struct ResourceFileBindings;
class ResourceFile;
struct ResourceFrontendBindings;
class ApplicationResourceRuntime;
struct CarTextureNamingPolicy;
class SkyResources;
class EcstasyFrame;
struct NisManagerRegistry;
}

namespace hp2 {
struct FlailerUiState;
struct FrontendStartupServices;
struct FrontendPlayerManager;

// These are the three live application-flow globals, independently of the
// GameSession owner and race parameters. This is native ownership, not a
// beta/console class layout. The future flow dispatcher consumes this same
// requested state and argument and updates currentState after shutdown.
struct ApplicationFrontendFlow {
    std::int32_t currentState{};
    std::int32_t requestedState{};
    std::int32_t argument{};
    // Independent InGameFlag, beta3275A8 -> PAL330540, authored initial0.
    // The complete startup producer remains an explicit dependency.
    std::int32_t inGameFlag{};
    // The independent released GP-539C texture-language cell starts at0.
    bool languageTexturesFrench{};
    // Released application globals consumed by the corresponding handlers.
    std::int32_t loadingFrontendStartedLoop{};
    std::int32_t exitDemoStartedTicks{};
};

void ChangeGameFlow(ApplicationFrontendFlow& flow,std::int32_t state,std::int32_t argument);

// The application flow and the pointed-to session have separate states.
bool IsGameFlowPaused(const GameSession* session) noexcept;
void GameFlowNotifyLanguageChange(ApplicationFrontendFlow& flow,TexturePack& dynamicTextures,
    ResourceLoader& loader,std::int32_t current,std::int32_t previous);

// Actual frontend-phase ownership. Resource files still belong to the
// application's ResourceLoader; profiles, language and audio are borrowed.
// The CarEffects pool is retired before World creates the racing pool.
struct GameFlowFrontendResources final {
    GameFlowFrontendResources(ResourceLoader& files,FrontendStartupServices& startup,
        const ApplicationLaunchOptions& launchOptions);
    ~GameFlowFrontendResources() noexcept;
    std::exception_ptr Close() noexcept;
    GameFlowFrontendResources(const GameFlowFrontendResources&)=delete;
    GameFlowFrontendResources& operator=(const GameFlowFrontendResources&)=delete;
    ResourceLoader& files;
    FrontendStartupServices& startup;
    const ApplicationLaunchOptions& launchOptions;
    std::unique_ptr<ResourceFrontendBindings> bindings;
    std::unique_ptr<CarEffectsResources> carEffects;
    // Absent before World has registered any impact voices; borrow the same
    // world owner when retiring a phase that has registered those voices.
    WorldImpactAudio* collisionAudio{};
private:
    bool closed_{};
    std::exception_ptr closeFailure_;
};

struct GameFlowBindings {
    const GameTime* clock{};
    DemoDiscManager* demo{};
    TexturePack* dynamicTextures{};
    FlailerUiState* ui{};
    GameFlowFrontendResources* frontend{};
    struct GameFlowTrackResources* track{};
    // Borrow the canonical frame: its texture/geometry references must retire
    // after a real fence, before a resource phase can be destroyed.
    const EcstasyFrame* renderFrame{};
};

// Process owners are borrowed. Track resources, collisions, camera movers,
// Players and RaceCoordinator are constructed by the track lifetime below.
// Each setting pointer names an existing mutable source/host owner; no
// copy of race selection, profile, input or sound state is made here.
struct GameFlowTrackProcessBindings {
    ApplicationResourceRuntime* resources{};
    ResourceFile* globalFile{};
    GameTime* clock{};
    ApplicationLaunchOptions* launch{};
    GameInputRouter* input{};
    CarCinematicServices* cameras{};
    CarTextureNamingPolicy* carTextureNaming{};
    PositionMarkerRegistry* positionMarkers{};
    RealDriverTuning* realDriverTuning{};
    Sound3dSampleRegistry* oneShotAudio{};
    WorldImpactAudio* impactAudio{};
    const std::int32_t* skipStartSequence{};
    const std::int32_t* alternateSkin{};
    const std::size_t* replayAvailableBytes{};
    const std::int32_t* impactPresentationMode{};
    std::int32_t* impactPresentationRequested{};
};

// The track phase borrows the same loader, catalogue, audio and HUD texture
// pack used by the application. ResourceFile remains the only owner of each
// physical bundle; the coordinator remains the only owner of Race/World.
struct GameFlowTrackResources final {
    GameFlowTrackResources(ResourceLoader& files,const CarInfoResource& cars,
        TexturePack& hudTextures,const ResourceFileBindings& trackBindings);
    ~GameFlowTrackResources() noexcept;
    // Retire consumers without releasing the bindings borrowed by files.
    // The application destroys this object after ResourceLoader::Shutdown.
    std::exception_ptr Close() noexcept;
    GameFlowTrackResources(const GameFlowTrackResources&)=delete;
    GameFlowTrackResources& operator=(const GameFlowTrackResources&)=delete;
    ResourceLoader& files;
    const CarInfoResource& cars;
    TexturePack& hudTextures;
    std::unique_ptr<ResourceFileBindings> fileBindings;
    ResourceFile* commonFile{};
    ResourceFile* trackFile{};
    WorldImpactAudio* collisionAudio{};
    CollisionSystem* collisionSystem{};
    CarDriverVisualAnimationDefinitions* animationDefinitions{};
    void BindProcessResources(const GameFlowTrackProcessBindings& bindings);
    GameSession* Session() noexcept;
    WorldSimulation* Simulation() noexcept;
    CarFramePlayerRegistry* Players() noexcept;
    CarViewMoverRegistry* ViewMovers() noexcept;
    NisManagerRegistry* NisManagers() noexcept;
    void ServicePresentation(const ApplicationFrontendFlow& flow,const FrontendPlayerManager& frontend);
    const SkyResources* Sky() const noexcept;
    const EditorMarkerModel* BoundaryModel() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
    bool closed_{};
    std::exception_ptr closeFailure_;
    friend void EndGameFlowLoadingTrackSpecific(ApplicationFrontendFlow&,const GameFlowBindings&);
    friend void BeginGameFlowUnloadingTrackSpecific(ApplicationFrontendFlow&,const GameFlowBindings&);
};
enum class GameFlowServiceStatus { Complete,InvalidState,InvalidBinding,MissingSourceDependency,PendingRenderCompletion };
struct GameFlowServiceResult {
    GameFlowServiceStatus status{GameFlowServiceStatus::Complete};
    std::uint32_t sourceFunction{};
    std::int32_t state{};
};
const char* GetGameFlowName(std::int32_t state);
void GameFlowLoadingFrontEnd(const ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void BeginGameFlowLoadingFrontEnd(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingFrontEndPart1(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingFrontEndPart2(ApplicationFrontendFlow& flow);
void EndGameFlowLoadingFrontEnd(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void BeginGameFlowUnloadingFrontEnd(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void BeginGameFlowLoadingTrackCommon(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingTrackCommonPart1(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingTrackCommonPart2(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void BeginGameFlowUnloadingTrackCommon(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void BeginGameFlowLoadingTrackSpecific(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void EndGameFlowLoadingTrackSpecific(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void BeginGameFlowUnloadingTrackSpecific(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingTrackSpecificPart1(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingTrackSpecificPart2(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingTrackSpecificPart3(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingTrackSpecificPart4(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingTrackSpecificPart5(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowLoadingTrackSpecificPart6(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void LoadHudTextures(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void BeginGameFlowExitDemoDisc(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void GameFlowExitDemoDisc(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);
void EndGameFlowExitDemoDisc(const GameFlowBindings& bindings);
// The fixed released table is used, never injected callbacks. The native
// result reports an unimplemented original begin/end before committing that
// transition. It is not a loading-complete or race-ready signal.
GameFlowServiceResult ServiceGameFlow(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings);

} // namespace hp2
