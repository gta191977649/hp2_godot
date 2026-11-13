#include "indep/inc/GameFlow.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/Race.hpp"
#include "Texture.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/FMenus.hpp"
#include "indep/inc/DemoDisc.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/Language.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/CarLoader.hpp"
#include "indep/inc/DemoModeReplay.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/RealDriver.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/WorldObject.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/SmokeableInfo.hpp"
#include "indep/inc/EventManager.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/NIS.hpp"
#include "indep/inc/EasterEggs.hpp"
#include "bsound/include/bSound.h"

#include "indep/inc/SpeechNoise.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/src/speech/mgr.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/HeliSheet.hpp"
#include "indep/inc/PhysicsCar.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/src/speech/mgr.hpp"
#include <array>
#include <vector>
#include <string>
#include "bware/inc/bWare.hpp"
#include <cstring>

namespace hp2 {

namespace {
struct PhaseCloseErrors {
    std::exception_ptr first;
    void Remember(std::exception_ptr error) noexcept {if(!first)first=error;}
    template<class Operation> void Run(Operation&& operation) noexcept {
        try {operation();}catch(...) {Remember(std::current_exception());}
    }
};

enum class FlowOperation {
    None,BeginNone,BeginLoadingFrontend,EndLoadingFrontend,LoadingFrontend,
    BeginUnloadingFrontend,BeginLoadingTrackCommon,BeginLoadingTrackSpecific,
    EndLoadingTrackSpecific,LoadingTrackSpecific,BeginUnloadingTrackSpecific,
    BeginUnloadingTrackCommon,BeginExitDemo,ExitDemo,EndExitDemo
};
struct FlowRow {const char* name;FlowOperation begin,end,tick;};
using Op=FlowOperation;
// Exact final table relationships, translated into readable native operations.
// Source PCs below serve diagnostics only and never dispatch behavior.
constexpr FlowRow flowRows[]={
    {"GAMEFLOW_STATE_NONE",Op::BeginNone,Op::None,Op::None},
    {"GAMEFLOW_STATE_LOADING_FRONTEND",Op::BeginLoadingFrontend,Op::EndLoadingFrontend,Op::LoadingFrontend},
    {"GAMEFLOW_STATE_UNLOADING_FRONTEND",Op::BeginUnloadingFrontend,Op::None,Op::None},
    {"GAMEFLOW_STATE_IN_FRONTEND",Op::None,Op::None,Op::None},
    {"GAMEFLOW_STATE_LOADING_TRACK_COMMON",Op::BeginLoadingTrackCommon,Op::None,Op::None},
    {"GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC",Op::BeginLoadingTrackSpecific,Op::EndLoadingTrackSpecific,Op::LoadingTrackSpecific},
    {"GAMEFLOW_STATE_RACING",Op::None,Op::None,Op::None},
    {"GAMEFLOW_STATE_UNLOADING_TRACK_SPECIFIC",Op::BeginUnloadingTrackSpecific,Op::None,Op::None},
    {"GAMEFLOW_STATE_UNLOADING_TRACK_COMMON",Op::BeginUnloadingTrackCommon,Op::None,Op::None},
    {"GAMEFLOW_STATE_EXIT_DEMO_DISC",Op::BeginExitDemo,Op::EndExitDemo,Op::ExitDemo}
};
std::uint32_t SourceDependency(FlowOperation operation)
{
    switch (operation) {
    case Op::BeginNone:return 0x168650;
    case Op::BeginLoadingFrontend:return 0x168698;
    case Op::EndLoadingFrontend:return 0x1687a8;
    case Op::LoadingFrontend:return 0x168718;
    case Op::BeginUnloadingFrontend:return 0x168830;
    case Op::BeginLoadingTrackCommon:return 0x168db8;
    case Op::BeginLoadingTrackSpecific:return 0x1690a0;
    case Op::EndLoadingTrackSpecific:return 0x169168;
    case Op::LoadingTrackSpecific:return 0x169160;
    case Op::BeginUnloadingTrackSpecific:return 0x169288;
    case Op::BeginUnloadingTrackCommon:return 0x168ea0;
    case Op::BeginExitDemo:return 0x169490;
    case Op::ExitDemo:return 0x1694a0;
    case Op::EndExitDemo:return 0x169528;
    default:return 0;
    }
}
std::int32_t WrappingDifference(std::int32_t left,std::int32_t right)
{
    const auto bits=static_cast<std::uint32_t>(left)-static_cast<std::uint32_t>(right);
    std::int32_t result;std::memcpy(&result,&bits,4);return result;
}
bool UiBound(const GameFlowBindings& bindings)
{
    return bindings.clock && bindings.demo && bindings.ui &&
        bindings.ui->resources && bindings.ui->render;
}
bool FrontendBound(const GameFlowBindings& bindings)
{
    if (!UiBound(bindings) || !bindings.frontend)return false;
    const auto* menu=bindings.frontend->startup.menus;
    return menu && menu->ui==bindings.ui && menu->screens && menu->frontend &&
        menu->raceSetup && menu->raceSetup->parameters && menu->sounds &&
        bindings.ui->language && bindings.ui->sound && menu->demoMode &&
        menu->demoMode->demoDisc==bindings.demo;
}
bool RenderingPending(const GameFlowBindings& bindings)
{
    return bindings.ui->resources->StripPool().HasPendingSubmissions() ||
        (bindings.renderFrame->State()!=EcstasyFrame::Phase::Complete &&
            !bindings.renderFrame->Commands().empty());
}
}

GameFlowFrontendResources::GameFlowFrontendResources(ResourceLoader& loader,
    FrontendStartupServices& menus,const ApplicationLaunchOptions& options)
    :files(loader),startup(menus),launchOptions(options)
{
    Require(menus.menus && menus.menus->frontend && menus.menus->demoMode &&
        menus.menus->demoMode->demoDisc && menus.menus->ui && menus.menus->ui->render,
        "Frontend resource lifetime has no actual application services");
    bindings=std::make_unique<ResourceFrontendBindings>(ResourceFrontendBindings{
        *menus.menus->frontend,*menus.menus->demoMode->demoDisc,*menus.menus->ui->render});
}
std::exception_ptr GameFlowFrontendResources::Close() noexcept
{
    if(closed_)return closeFailure_;
    PhaseCloseErrors errors;
    errors.Remember(closeFailure_);
    errors.Run([]{CloseSelectCarScreen();});
    errors.Run([&] {
        Require(!carEffects || carEffects->UsedPositions()==0,
            "Frontend close precedes selected-car appearance retirement");
        carEffects.reset();
    });
    // ResourceFiles still borrow these records until loader shutdown.
    closed_=!carEffects;
    closeFailure_=errors.first;
    return closeFailure_;
}
GameFlowFrontendResources::~GameFlowFrontendResources() noexcept {(void)Close();}

void BeginGameFlowLoadingFrontEnd(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(FrontendBound(bindings),"Frontend loading has no canonical resource/UI owners");
    auto& owner=*bindings.frontend;
    auto& menu=*owner.startup.menus;
    Require(!owner.files.FindResourceFile(1) && !owner.carEffects,
        "Previous frontend resources must retire before loading again");
    flow.loadingFrontendStartedLoop=bindings.clock->realLoopCounter;
    raceParametersInitializeDefaults(menu.raceSetup->parameters,&owner.launchOptions,
        &menu.ui->language->currentLanguage);
    // bCountFreeMemory(16) is a PS2 heap-leak diagnostic. Native allocation
    // lifetimes are owned here; no console free-byte value is fabricated.
    ResourceFileBindings fileBindings;
    fileBindings.kind=ResourceFileKind::Frontend;
    fileBindings.frontend=owner.bindings.get();
    auto* first=owner.files.LoadResourceFile("FRONTEND\\FRONTA.BUN",1,0,fileBindings);
    try {
        fileBindings.flow=&flow;fileBindings.gameFlow=&bindings;
        auto* second=owner.files.LoadResourceFile("FRONTEND\\FRONTB.LZC",1,9,fileBindings,
            ResourceFileCompletion::FrontendLoaded);
        second->ChangeFilenameForHotChunking("FRONTEND\\FRONTB.BUN");
    } catch (...) {owner.files.UnloadResourceFile(first);throw;}
}

void GameFlowLoadingFrontEndPart1(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(FrontendBound(bindings),"Frontend completion lost its application owners");
    auto& menu=*bindings.frontend->startup.menus;
    menu.ui->language->LoadLocalizedSpeech();
    Require(soundLoadRegionsFrontend(*menu.sounds,*menu.frontend,*menu.raceSetup->parameters,&flow)==WORLD_AUDIO_OK,
        "Frontend sound regions could not start their actual load");
}

void GameFlowLoadingFrontEndPart2(ApplicationFrontendFlow& flow)
{
    ChangeGameFlow(flow,3,0);
}

void EndGameFlowLoadingFrontEnd(ApplicationFrontendFlow&,const GameFlowBindings& bindings)
{
    Require(FrontendBound(bindings),"Frontend activation has no actual application owners");
    auto& owner=*bindings.frontend;
    auto& menu=*owner.startup.menus;
    Require(owner.files.IsResourceLoadingComplete() && !owner.carEffects,
        "Frontend activation precedes resource completion or repeats an active lifetime");
    // PAL32F780 is eDisableFixUpTables, not InGameFlag330540.
    owner.files.Registry().Models().fixupsBlocked=0;
    owner.files.Registry().ResourceRefresh();
    owner.carEffects=std::make_unique<CarEffectsResources>();
    InitFrontendSounds(*menu.ui->sound,*menu.sounds);
    // InitSelectCarScreen's PAL tail calls this actual method. The released
    // InitMoviePlayer237688 is JR RA/NOP and adds no native stub.
    CarSelector::InitSpotlights(owner.files.Registry());
    if (bindings.demo->ActivationWord()) {
        menu.ui->render->SetScreenVisibility(*menu.ui->resources,0x8254cc20u,1);
    } else {
        menu.ui->render->SetScreenVisibility(*menu.ui->resources,0xd0131dd1u,1);
        menu.ui->resources->SetObjectVisibility(0xd0131dd1u,0x807d4c5cu,0);
    }
    LaunchFrontendMenus(owner.startup);
}

void BeginGameFlowUnloadingFrontEnd(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(FrontendBound(bindings),"Frontend teardown lost its actual owners");
    auto& owner=*bindings.frontend;
    auto& menu=*owner.startup.menus;
    Require(bindings.renderFrame,"Frontend retirement has no canonical render frame");
    Require(!RenderingPending(bindings),
        "Frontend teardown awaits actual render completion");
    Require(menu.raceSetup->demoReplay,
        "Frontend teardown needs its existing car/demo resource owners");
    UnlaunchFrontendMenus(owner.startup);
    // CloseMoviePlayer237690 is an original JR RA/NOP, folded here.
    CloseSelectCarScreen();
    carLoader.UnloadEverything();
    UnInitFrontendSounds(*menu.ui->sound);
    if (owner.carEffects) {
        Require(owner.carEffects->UsedPositions()==0,
            "Frontend car effects still have a live selected car");
        owner.carEffects.reset();
    }
    Require(soundUnloadRegions(*menu.sounds,owner.collisionAudio)==WORLD_AUDIO_OK,
        "Frontend sound-region retirement failed");
    while (auto* file=owner.files.FindResourceFile(1))owner.files.UnloadResourceFile(file);
    menu.ui->language->UnloadLocalizedSpeech();
    // A recorded attract Demo has already read RaceParameters but has not
    // yet constructed ReplayManagerOwner. Preserve the resource bytes until
    // the race finishes: each snapshot and bRepeat directly borrows them.
    if (!menu.raceSetup->demoReplay->active)
        menu.raceSetup->demoReplay->UnloadResourceFile();
    menu.ui->resources->DeleteAllStrips();
    // SlotPoolManager.Cleanup and bCountFreeMemory are PS2 allocator/heap
    // diagnostics. The native owning pools above perform real retirement.
    while (owner.files.HasPendingFileReads())owner.files.ServiceResourceLoading();
    ChangeGameFlow(flow,4,0);
}

namespace {
bool TrackBound(const GameFlowBindings& bindings)
{
    if (!FrontendBound(bindings) || !bindings.track || !bindings.track->fileBindings)return false;
    const auto& track=*bindings.track;
    const auto& files=*track.fileBindings;
    const auto* menu=bindings.frontend->startup.menus;
    return &track.files==&bindings.frontend->files && files.parameters==menu->raceSetup->parameters &&
        files.sound==menu->sounds && files.cameras && files.particles &&
        &track.hudTextures.CatalogueRoot()==&track.files.Registry().Models();
}
ResourceFileBindings TrackFileBindings(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings,
    ResourceFileKind kind)
{
    Require(TrackBound(bindings),"Track flow has no canonical resource, selection or audio owners");
    auto file=*bindings.track->fileBindings;
    file.kind=kind;file.frontend=bindings.frontend->bindings.get();
    file.flow=&flow;file.gameFlow=&bindings;
    return file;
}
}

GameFlowTrackResources::GameFlowTrackResources(ResourceLoader& loader,const CarInfoResource& catalogue,
    TexturePack& textures,const ResourceFileBindings& bindings)
    :files(loader),cars(catalogue),hudTextures(textures),fileBindings(std::make_unique<ResourceFileBindings>(bindings))
{
    Require(bindings.parameters && bindings.sound && bindings.cameras && bindings.particles &&
        catalogue.CarsLoaded() && &textures.CatalogueRoot()==&files.Registry().Models(),
        "Track resources require the actual application resource bindings");
}
// This is the native arrangement of the source phase's actual owners.
// ResourceFile owns its decoded track; RaceCoordinator owns Race/World;
// these records only join their real services and retain native pool memory.
struct GameFlowTrackResources::Storage {
    explicit Storage(const GameFlowTrackProcessBindings& b):process(b) {
        simulation.clock=b.clock;
        carDriverVisualAnimationDefinitionsInitialize(&animations);
        carCameraShakeInitialize(&shake);
        carChaseCameraSettingsInitialize(&chaseSettings);
        carViewMoverSettingsInitialize(&viewSettings);
        carOrbitCameraSettingsInitialize(&orbitSettings);
        carVelocityCameraSettingsInitialize(&velocitySettings);
        carAirborneCameraSettingsInitialize(&airborneSettings);
        carRecordedCameraSettingsInitialize(&recordedSettings);
        carFramePlayerAirborneSettingsInitialize(&airbornePathSettings);
        carFixedCameraCommandsInitialize(&fixedCommands);
        carAttachedCameraSettingsInitialize(&attachedSettings);
        trackRouteEditorSettingsInitialize(&editorSettings);
    }
    GameFlowTrackProcessBindings process;
    CarDriverVisualAnimationDefinitions animations{};
    CollisionSpace collisionSpace{};
    // PAL InitCollisionBody:256 witnesses; InitContact:48 contacts;
    // InitCollision:32 impacts. Host sizeof replaces PS2 object sizes.
    std::array<CollisionPair,256> pairs{};
    std::array<CollisionQueuedContact,48> contacts{};
    std::array<CollisionImpact,32> impacts{};
    // PAL InitCollisionBody1D5D30..1D5D44 creates the160-entry
    // TrackPolygonCollisionBody pool alongside the witness pool above.
    std::array<CollisionTerrainShape,160> terrainShapes{};
    CollisionSystem collisions{};
    PhysicsPools* physicsPools{};
    std::unique_ptr<SkyResources> sky;
    std::unique_ptr<VisibleZoneResources> visibleZones;
    bool sunLoaded{},collisionInitialized{},eventsInitialized{};
    std::vector<CarRecoveryMutableGroup> mutableRoadGroups;
    TrackRouteEditorSettings editorSettings{};
    std::unique_ptr<TrackRouteEditor,decltype(&trackRouteEditorDispose)> editor{nullptr,trackRouteEditorDispose};
    // PAL InitWorldObjects initially reserves128 objects. Standalone
    // bOMalloc allocations grow without moving these borrowed slots.
    std::array<WorldObject,128> worldObjectSlots{};
    WorldObjectRegistry objects{};
    std::array<WorldEvent,256> eventSlots{};
    WorldEventDispatcher events{};
    CarMovementFeedbackBindings movementFeedback{};
    std::array<CarFramePresentationEvent,256> presentationSlots{};
    CarFramePresentation presentation{presentationSlots.data(),presentationSlots.size(),0,0};
    TrackScenarioCatalog catalog{};
    CarMovementRace movementRace{};
    CarMovementBindings movement{};
    PhysicsCarEnvironment physics{};
    CarFrameEnvironment frame{};
    CarFramePlayerRegistry players{};
    CarFramePlayerServices playerServices{};
    CarFramePlayerInputBindings playerInputBindings{};
    PreviewCometServices previewComet{};
    WorldSimulation simulation{};
    WorldReplay worldReplay{};
    RaceFrameServices raceServices{};
    RaceCommentary commentary{};
    DriverAIEnvironment ai{};
    RealDriverBindings real{};
    CarDriverEnvironment driver{};
    CarRenderBindings render{};
    EditorMarkerTexture* sharedEffectTexture{};
    CarConstructionBindings carBindings{};
    TrafficSchedulerBindings traffic{};
    CarCameraShake shake{};
    WorldAudioEffects audioEffects{};
    PursuitGroupBindings pursuitGroups{};
    WorldConstructionBindings worldBindings{};
    CarPlayerCameraServices cameraInput{};
    CarChaseCameraSettings chaseSettings{};
    CarViewMoverSettings viewSettings{};
    CarOrbitCameraSettings orbitSettings{};
    CarVelocityCameraSettings velocitySettings{};
    CarAirborneCameraSettings airborneSettings{};
    CarRecordedCameraSettings recordedSettings{};
    CarFramePlayerAirborneSettings airbornePathSettings{};
    CarFixedCameraCommands fixedCommands{};
    CarAttachedCameraSettings attachedSettings{};
    ScreenRadialBlur radialBlur{};
    CarViewMoverRegistry views{};
    CarFramePlayerCameraContext playerCameras{};
    NisManagerRegistry nis;
    PlayerConstructionBindings playerBindings{};
    RaceScreenServices screenServices{};
    std::unique_ptr<StartRaceScreens> startScreens;
    RaceCoordinatorBindings coordinatorBindings{};
    RaceCoordinatorInitBindings coordinatorInit{};
    GameSessionViewSelection viewSelection{};
    GameSessionCameraContext sessionCameras{};
    std::unique_ptr<RaceCoordinatorOwner> coordinator;

    std::exception_ptr RetireGameplayNoThrow(GameFlowTrackResources& phase,bool terminal) noexcept {
        PhaseCloseErrors errors;
        // Concrete screens borrow Players/Race, including after a failed
        // normal ScreenDeactivate that never reached their callbacks.
        if(terminal && startScreens)errors.Remember(startScreens->Shutdown());
        if(coordinator)errors.Remember(coordinator->Close());
        coordinator.reset();
        movementFeedback.session=nullptr;
        // Terminal cancellation, including an interrupted frame. Storage is
        // reused by later track loads; it must not retain retired Car borrows.
        // This does not acknowledge delivery of any unfinished side effect.
        errors.Run([&]{carFramePresentationReset(&presentation);});
        // These process services borrow the one coordinator's session. They
        // remain alive in the frontend after that session has been destroyed.
        phase.fileBindings->sound->session=nullptr;
        if(worldBindings.speech)worldBindings.speech->session=nullptr;
        players.sessionState=nullptr;playerServices.session=nullptr;playerServices.raceActions=nullptr;
        players.displaySettings=nullptr;players.enableVisibleDrivingLine=nullptr;playerServices.previewComet=nullptr;previewComet={};
        raceServices.session=nullptr;worldReplay.raceActions=nullptr;
        if(startScreens) {
            if(!terminal)errors.Run([]{UnbindStartRaceScreens();});
            startScreens.reset();
        }
        // CloseAnimatedDriver is an exact empty source body: the animation
        // definitions continue to belong to the still-loaded common phase.
        errors.Run([&]{worldObjectsClose(&objects);});
        simulation.editor=nullptr;editor.reset();mutableRoadGroups.clear();
        visibleZones.reset();
        if(sunLoaded) {
            errors.Run([&]{phase.files.Registry().Sun().UnloadTrack();});sunLoaded=false;
        }
        sky.reset();
        if(physicsPools) {
            const auto status=physicsObjectsClosePools(physicsPools);
            if(status==PHYSICS_POOL_OK)physicsPools=nullptr;
            else {
                errors.Run([]{throw std::runtime_error("Track physics retirement still has live physical components");});
                // World and every Car have retired. Preserve the failed
                // bookkeeping check while releasing terminal native storage.
                if(terminal) {physicsPoolsDestroy(physicsPools);physicsPools=nullptr;}
            }
        }
        if(collisionInitialized) {errors.Run([&]{collisionSystemDispose(&collisions);});collisionInitialized=false;}
        // A registered sound owner must retain this borrow until the existing
        // soundUnloadRegions/Unregister. Disabled audio acquired only the timer
        // borrow above and has no SDK registration to retire it later.
        if(!process.impactAudio->registered && process.impactAudio->collisions==&collisions) {
            process.impactAudio->collisions=nullptr;
            if(collisions.contactAudio==process.impactAudio)collisions.contactAudio=nullptr;
        }
        if(eventsInitialized) {errors.Run([&]{worldEventsDispose(&events);});eventsInitialized=false;}
        errors.Run([&]{UnbindPalRadialRenderEgg(&radialBlur);});
        if(process.cameras->radialBlur==&radialBlur)process.cameras->radialBlur=nullptr;
        process.cameras->session=nullptr;
        cameraInput.world=nullptr;
        return errors.first;
    }
    void RetireGameplay(GameFlowTrackResources& phase) {
        if(coordinator) {coordinator->Retire();coordinator.reset();}
        if(auto failure=RetireGameplayNoThrow(phase,false))std::rethrow_exception(failure);
    }
};

std::exception_ptr GameFlowTrackResources::Close() noexcept
{
    if(closed_)return closeFailure_;
    closed_=true;
    if(storage_)closeFailure_=storage_->RetireGameplayNoThrow(*this,true);
    return closeFailure_;
}
GameFlowTrackResources::~GameFlowTrackResources() noexcept {(void)Close();}

void GameFlowTrackResources::BindProcessResources(const GameFlowTrackProcessBindings& b)
{
    Require(!closed_ && !storage_ && b.resources && &b.resources->Loader()==&files && b.globalFile && b.globalFile->Loaded() &&
        b.clock && b.launch && b.input && b.cameras && b.cameras->clock==b.clock && b.carTextureNaming &&
        b.positionMarkers && b.realDriverTuning && b.oneShotAudio && b.impactAudio &&
        b.skipStartSequence && b.alternateSkin &&
        b.replayAvailableBytes && b.impactPresentationMode && b.impactPresentationRequested,
        "Track lifetime requires the complete canonical process owners");
    const auto& global=b.globalFile->Resources();
    Require(global.cars==&cars && global.track && global.characters && global.driverBehavior && global.nis &&
        fileBindings->cameras==&b.resources->CameraResources() &&
        fileBindings->particles==&b.resources->FX().Particles() &&
        fileBindings->sound && fileBindings->sound->clock==b.clock,
        "Track lifecycle received a different global catalogue, camera, clock or sound owner");
    storage_=std::make_unique<Storage>(b);
    collisionSystem=&storage_->collisions;collisionAudio=b.impactAudio;
    animationDefinitions=&storage_->animations;
}
GameSession* GameFlowTrackResources::Session() noexcept
{return storage_ && storage_->coordinator ? &storage_->coordinator->Session() : nullptr;}
WorldSimulation* GameFlowTrackResources::Simulation() noexcept
{return storage_ ? &storage_->simulation : nullptr;}
CarFramePlayerRegistry* GameFlowTrackResources::Players() noexcept
{return storage_ ? &storage_->players : nullptr;}
CarViewMoverRegistry* GameFlowTrackResources::ViewMovers() noexcept
{return storage_ ? &storage_->views : nullptr;}
NisManagerRegistry* GameFlowTrackResources::NisManagers() noexcept
{return storage_ ? &storage_->nis : nullptr;}
const SkyResources* GameFlowTrackResources::Sky() const noexcept
{return storage_ ? storage_->sky.get() : nullptr;}
const EditorMarkerModel* GameFlowTrackResources::BoundaryModel() const noexcept
{return storage_ && storage_->visibleZones ? &storage_->visibleZones->BoundaryModel() : nullptr;}

void GameFlowTrackResources::ServicePresentation(const ApplicationFrontendFlow& flow,
    const FrontendPlayerManager& frontend)
{
    if(!storage_)return;
    auto& own=*storage_;auto& pending=own.presentation;
    Require(!pending.failed && pending.count<=pending.capacity,
        "Frame presentation overflowed before its original consumers ran");
    for(std::size_t index=0;index<pending.count;++index) {
        const auto& event=pending.events[index];
        Require(event.car && event.car->frame,"Frame presentation lost its actual Car owner");
        switch(event.kind) {
        case CAR_FRAME_DRIVING_STAT:
            // carFrameAddHighlight already committed the real replay value.
            Require(event.code>=CAR_FRAME_STAT_IN_AIR && event.code<=CAR_FRAME_STAT_SMOKEABLE_COLLISION,
                "Unknown completed driving-stat observation");
            break;
        case CAR_FRAME_ACCELERATION_FEEDBACK:
            carCameraMaybeShake(own.shake,own.players,event.port,event.value);
            break;
        case CAR_FRAME_CONTROLLER_EFFECT:
            joystickPortRumbleSurface(*own.process.input,event.port,event.code,event.value[0],
                flow,Session(),frontend,own.players);
            break;
        case CAR_FRAME_FORCE_FEEDBACK: {
            const auto result=joystickDispatchForceFeedback(own.process.input,&event);
            Require(result==GAME_INPUT_FORCE_NO_DEVICE,
                result==GAME_INPUT_FORCE_DEVICE_UNSUPPORTED ?
                    "Registered steering wheel has no native force-feedback endpoint" :
                    "Invalid original steering-force request");
            break;
        }
        case CAR_FRAME_SURFACE_EFFECT:
            // effect() emits its actual particles synchronously, then records
            // an observation. The distinct disabled-wheel producer only
            // queues effect8, so it still needs its original consumer here.
            Require(event.code>=0 && event.code<=10,"Unknown surface effect observation");
            if(event.code==8)
                Require(carTriggerEffect(event.car,event.code,event.value[0],event.argument,
                    event.hasPosition?event.position:nullptr)==CAR_EFFECT_OK,
                    "Disabled-wheel particle effect failed on its actual owner");
            break;
        case CAR_FRAME_UI_CLEAR_TRANSIENT: {
            auto* player=event.car->frame->environment.player;
            Require(player && player->car==event.car,"Transient presentation lost its actual Player");
            if(event.code==0)carFramePlayerClearHudAnimations(player);
            else if(event.code==8)carFramePlayerClearRecoveryAirborneTime(player);
            else throw std::runtime_error("Unknown original transient-state clear");
            break;
        }
        default:throw std::runtime_error("Frame presentation has no implemented original consumer");
        }
    }
    // Reclaim only after every ordered side effect completed. Unknown or
    // failed requests remain an explicit failure rather than a silent drop.
    carFramePresentationReset(&pending);
}

// PAL169168. Native resource decoding has already prepared TopologyTree,
// TrackPaths and TrackRoutes in their sole ResourceFile owners. This step
// creates the actual runtime pools/handles and joins them in source order.
// AI demo invokes the PAL DisableRenderEggs path after race players are
// initialized. Normal races with unlocked rendering modifiers still require
// their original concrete Vulkan Stomper effect descriptors.
void EndGameFlowLoadingTrackSpecific(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings) && bindings.track->storage_,"Track activation has no process lifetime binding");
    auto& phase=*bindings.track;auto& own=*phase.storage_;auto& process=own.process;
    Require(!own.coordinator && !own.physicsPools && phase.commonFile && phase.trackFile &&
        phase.files.IsResourceLoadingComplete(),"Track activation precedes actual resource completion");
    Require(process.clock==bindings.clock && process.launch==&bindings.frontend->launchOptions,
        "Track activation has another clock or launch state");
    auto& menus=*bindings.frontend->startup.menus;
    auto& parameters=*menus.raceSetup->parameters;
    if(!parameters.demoReplay && !parameters.fastSpeedTrapSpawns)
        Require(easterEggs.records && easterEggs.recordCount>8,
            "Track activation has no canonical unlock records");
    const auto& track=phase.trackFile->Resources();const auto& common=phase.commonFile->Resources();
    const auto& global=process.globalFile->Resources();
    Require(track.track && track.terrain && track.route && track.recovery && track.scenery && track.navigation &&
        track.events && track.collisionObjects && common.collisionObjects && common.animations &&
        menus.frontend->highScores && menus.raceSetup->currentEvent,
        "Track activation is missing an actual decoded runtime or profile dependency");
    auto& resources=phase.files.Registry();auto& models=resources.Models();
    auto& sound=*phase.fileBindings->sound;auto& particles=process.resources->FX().Particles();
    auto& effects=process.resources->FX().Emitters();
    auto* speech=worldSpeechGetInstance();
    Require(sound.audio && sound.globals && sound.manager && sound.spatial.cameras==process.cameras &&
        speech && speech->fsm && speech->messages && speech->noise,
        "Race requires the actual sound and Language/SPCH owners");
    // LoadedTrackResources binds34200 before asynchronous sound regions are
    // published. Refresh its bank borrow now, before the original Car/tire
    // constructors perform their live bSound::Region lookup (PAL212BC0).
    Require(carSoundBindTrackBank(sound,*track.track)==WORLD_AUDIO_OK,
        "Track activation could not bind the current published sound bank");
    auto* first=parameters.fastSpeedTrapSpawns ? nullptr :
        raceParametersGetDriverInfoByDriverNumber(&parameters,0);
    if(parameters.fastSpeedTrapSpawns) {
        Require(parameters.drivers && parameters.driverCount>0 && parameters.driverCount<=RACE_DRIVER_LIMIT,
            "AI Demo activation has no valid driver bank");
        // Native bootstrap borrows must describe the Car that InitPlayers
        // watches. PAL14A330..14A3D4 creates racers in start-grid order;
        // 14D338 preserves that order within class2, and149CFC..149D1C
        // selects firstGroupEnd+seat. AI Demo has no human driver0.
        first=nullptr;
        for(int32_t position=1;position<=8 && !first;++position) {
            for(int32_t index=0;index<parameters.driverCount;++index) {
                auto& selected=parameters.drivers[index];
                if(selected.eliminated>0 || selected.startingPosition!=position)continue;
                if(selected.driverClass==2)first=&selected;
                // The first matching non-racer also consumes this grid slot.
                break;
            }
        }
    }
    Require(first && first->appearance.modelCode>=0 && first->appearance.modelCode<64,
        "Track activation has no selected presentation car");
    Require(phase.cars.IsAvailable(static_cast<std::size_t>(first->appearance.modelCode)),
        "Track presentation car is absent from the actual catalogue");
    const auto& firstType=phase.cars.Car(static_cast<std::size_t>(first->appearance.modelCode));
    const CarCameraCatalogue* defaultCatalogue=&firstType.cameraCatalogue;
    if(!defaultCatalogue->viewCount[0] || !defaultCatalogue->viewCount[1]) {
        // A playable traffic car can be the first presentation vehicle while
        // having no authored camera rows. The existing chase-camera fallback
        // must borrow a nonempty catalogue from this SAME resource owner.
        // Vehicle geometry, dynamics and target clearance stay with the car.
        for(const auto& candidate:phase.cars.Cars()) {
            if(candidate.cameraCatalogue.viewCount[0] && candidate.cameraCatalogue.viewCount[1]) {
                defaultCatalogue=&candidate.cameraCatalogue;break;
            }
        }
        Require(defaultCatalogue->viewCount[0] && defaultCatalogue->viewCount[1],
            "Track activation has no authored default camera catalogue");
    }
    models.fixupsBlocked=0;resources.ResourceRefresh();flow.inGameFlag=1;
    try {
        collisionSpaceInitialize(&own.collisionSpace);
        collisionSystemInitialize(&own.collisions,&own.collisionSpace,own.pairs.data(),own.pairs.size(),
            own.contacts.data(),own.contacts.size(),own.impacts.data(),own.impacts.size(),&collisionSolverDefaultTuning);
        own.collisionInitialized=true;
        own.collisions.terrainShapes=own.terrainShapes.data();
        own.collisions.terrainCapacity=own.terrainShapes.size();
        Require(!process.impactAudio->collisions || process.impactAudio->collisions==&own.collisions,
            "Collision audio borrows another track's collision timer");
        own.collisions.contactAudio=process.impactAudio;
        // The source contact-sample selector reads the global collision timer
        // even when audio loading was disabled. Bind that same native owner;
        // registration and sample-table readiness still belong to sound-load.
        process.impactAudio->collisions=&own.collisions;
        // InitTree's actual1024-entry native polygon cache and the decoded
        // route initialization already belong to loaded track.terrain/route.
        PhysicsPoolStatus poolStatus;
        own.physicsPools=physicsObjectsCreatePools(WORLD_MAX_CARS,&poolStatus);
        Require(own.physicsPools && poolStatus==PHYSICS_POOL_OK,"Track physics pool allocation failed");
        own.sky=std::make_unique<SkyResources>(models,track.scenery->Native());
        // rain_Load12A148 and InitDebugTopology203CE0 are exact JR RA/NOP.
        own.sunLoaded=true;resources.Sun().LoadTrack(resources);
        own.visibleZones=std::make_unique<VisibleZoneResources>(models);
        Require(track.recovery->Native().reverse==(parameters.reverse!=0),
            "Loaded road preprocessing uses another direction");
        own.mutableRoadGroups=track.recovery->MutableGroups();
        TrackRouteEditorContext editorContext{&track.recovery->Native(),own.mutableRoadGroups.data(),
            own.mutableRoadGroups.size(),&track.terrain->Topology(),process.input,process.clock,&own.editorSettings,&models};
        TrackRouteEditorResult editorResult;
        own.editor.reset(trackRouteEditorCreate(&editorContext,&editorResult));
        Require(own.editor && editorResult==TRACK_ROUTE_EDITOR_OK,"Actual track route editor construction failed");
        own.simulation.editor=own.editor.get();
        worldObjectsInitialize(&own.objects,own.worldObjectSlots.data(),own.worldObjectSlots.size(),&models);
        Require(carDriverVisualPrepareAnimations(&own.animations,common.animations)!=0,
            "Actual common driver-animation preprocessing failed");
        // AIPlan allocations belong to their actual native Driver owners;
        // their destructors retire them before the track memory is released.
        worldEventsInitialize(&own.events,own.eventSlots.data(),own.eventSlots.size(),
            track.events->Containers(),track.events->ContainerCount());own.eventsInitialized=true;
        own.catalog={global.track->Catalog().scenarios,global.track->Catalog().scenarioCount,
            track.track->Catalog().landmarks,track.track->Catalog().landmarkCount};
        own.movementRace.difficulty=&parameters.opponentDifficulty;
        own.movement.pools=own.physicsPools;own.movement.route=&track.route->Native();
        own.movement.recoveryPoints=&track.recovery->Native();own.movement.applicationState=&flow.currentState;
        own.movement.race=&own.movementRace;
        own.movementFeedback={process.input,&flow,menus.frontend,&own.players,nullptr};
        own.movement.feedback=&own.movementFeedback;
        own.physics.clock=process.clock;own.physics.tuning=&physicsCarDefaultTuning;
        own.physics.contact.tuning=&wheelContactDefaultTuning;
        own.physics.contact.materials=terrainDefaultMaterials(&own.physics.contact.materialCount);
        own.frame.clock=process.clock;own.frame.settings=&parameters;own.frame.spatial=&own.events;
        own.frame.presentation=&own.presentation;
        own.players.clock=process.clock;own.players.services=&own.playerServices;
        own.players.raceKind=&parameters.raceKind;own.players.pursuitEnabled=&parameters.specialPursuit;
        own.players.soundPolicy=&sound.globals->policy;
        // TriggerVisibleDrivingLine180B78 reads the same live RaceSettings+4C
        // even when disabled. Missing the borrow is an owner failure, not false.
        own.players.enableVisibleDrivingLine=&parameters.authoredRaceOption4C;
        own.simulation.players=&own.players;own.simulation.replay=&own.worldReplay;
        own.worldReplay.log=bindings.frontend->startup.movieLog;own.worldReplay.clock=process.clock;
        Require(own.worldReplay.log && own.worldReplay.log->clock==process.clock,
            "Track replay requires the canonical application Joylog clock");
        own.raceServices.settings=&parameters;own.raceServices.players=&own.players;
        own.raceServices.profile=&menus.frontend->highScores->LapView();
        own.raceServices.currentEvent=menus.raceSetup->currentEvent;own.raceServices.commentary=&own.commentary;
        own.commentary.settings=&parameters;own.commentary.players=&own.players;
        own.commentary.profile=own.raceServices.profile;own.commentary.audio=speech->messages;
        own.ai.simulation=&own.simulation;own.ai.tuning=&driverAIDefaultTuning;
        own.ai.behavior=&global.driverBehavior->Settings();own.ai.scenario=global.track->Find(parameters.selectedScenario);
        own.ai.specialPursuit=&parameters.specialPursuit;own.ai.policy=&driverAIDefaultPolicy;
        own.real.tuning=process.realDriverTuning;own.real.inputRouter=process.input;
        own.driver.real=&own.real;own.driver.ai=&own.ai;
        own.render.cars=&phase.cars;own.render.resources=&resources;own.render.naming=process.carTextureNaming;
        own.render.scenario=own.ai.scenario;
        own.render.markers=process.positionMarkers;own.render.particles=&particles;
        own.render.effectGroups=effects.groups;own.render.effectGroupCount=effects.groupCount;
        own.render.applicationState=&flow.currentState;own.render.characters=&global.characters->Catalogue();
        own.render.sharedEffectTexture=&own.sharedEffectTexture;own.render.visual.clock=process.clock;
        own.render.visual.cinematics=process.cameras;own.render.visual.definitions=&own.animations;
        own.carBindings.definition=&firstType;own.carBindings.topology=&track.terrain->Topology();
        own.carBindings.movement=&own.movement;own.carBindings.physics=&own.physics;own.carBindings.frame=&own.frame;
        own.carBindings.driver=&own.driver;own.carBindings.render=&own.render;own.carBindings.collision=&own.collisions;
        own.carBindings.events=&own.events;own.carBindings.sound=&sound;
        own.traffic={nullptr,process.clock,&parameters,nullptr,&own.players,&track.recovery->Native(),process.alternateSkin};
        own.audioEffects={sound.audio,&worldAudioRuntime.randomState,&sound.globals->policy.audioSystemEnabled};
        own.pursuitGroups={&own.collisions,&track.terrain->Topology(),common.collisionObjects,&phase.cars,
            menus.ui->language,&track.route->Native()};
        own.worldBindings.clock=process.clock;own.worldBindings.parameters=&parameters;
        own.worldBindings.launch=process.launch;own.worldBindings.simulation=&own.simulation;own.worldBindings.cars=&own.carBindings;
        own.worldBindings.traffic=&own.traffic;own.worldBindings.cameraShake=&own.shake;
        own.worldBindings.scenery=&track.scenery->Native();own.worldBindings.objects=&own.objects;own.worldBindings.resources=&resources;
        own.worldBindings.sceneryObjects=track.collisionObjects;own.worldBindings.inGameObjects=common.collisionObjects;
        own.worldBindings.effects=&effects;own.worldBindings.audio=&own.audioEffects;own.worldBindings.audioRequests=sound.audio;
        own.worldBindings.messages=speech->messages;own.worldBindings.speech=speech;own.worldBindings.pursuit=&own.pursuitGroups;
        own.worldBindings.trackBytes=track.track->Authored().data();own.worldBindings.trackSize=track.track->Authored().size();
        own.worldBindings.navigation=&track.navigation->Native();own.worldBindings.impactPresentationMode=process.impactPresentationMode;
        own.worldBindings.impactPresentationRequested=process.impactPresentationRequested;
        own.radialBlur.cameras=process.cameras;
        own.radialBlur.texture=textureInfoForDescriptor(resources.SpecialTexture(5));
        process.cameras->radialBlur=&own.radialBlur;
        // Native target for the verified PAL 0x31D410 Stomper descriptor.
        BindPalRadialRenderEgg(&own.radialBlur);
        own.cameraInput.input=process.input;own.cameraInput.players=&own.players;
        own.cameraInput.allowAllViewKinds=&process.launch->debugCamerasEnabled;
        own.cameraInput.defaultCatalogue=defaultCatalogue;own.cameraInput.presentationLayout=&process.cameras->presentationLayout;
        own.playerServices.input=process.input;own.playerServices.cameras=&own.cameraInput;
        own.playerInputBindings.cameraCyclingEnabled=&process.launch->debugCamerasEnabled;
        own.playerServices.inputBindings=&own.playerInputBindings;
        own.playerServices.radialBlur=&own.radialBlur;own.playerServices.audio=sound.audio;own.playerServices.audioManager=sound.manager;
        // The Player finish producers and Finish cancellation borrow the same
        // localized SED stream already used by the process speech manager.
        own.playerServices.streamedAudio=speech->streamedAudio;
        own.previewComet={&process.resources->FX().FrameParticles(),effects.groups,effects.groupCount,&models};
        own.playerServices.previewComet=&own.previewComet;
        own.views.environment.services=process.cameras;own.views.environment.terrain=&track.terrain->Topology();
        own.views.environment.simulation=&own.simulation;own.views.environment.resources=&process.resources->CameraResources();
        own.views.environment.race=&parameters;own.views.environment.defaultCatalogue=defaultCatalogue;
        own.views.environment.shake=&own.shake;own.views.environment.chaseSettings=&own.chaseSettings;
        own.views.environment.airborneSettings=&own.airborneSettings;
        own.views.environment.recordedSettings=&own.recordedSettings;
        own.views.environment.settings=&own.viewSettings;
        own.playerCameras.services=process.cameras;own.playerCameras.input=&own.cameraInput;
        own.playerCameras.terrain=&track.terrain->Topology();own.playerCameras.simulation=&own.simulation;
        own.playerCameras.editor=own.editor.get();own.playerCameras.viewMovers=&own.views;own.playerCameras.race=&parameters;
        own.playerCameras.defaultCatalogue=defaultCatalogue;own.playerCameras.shake=&own.shake;
        own.playerCameras.chaseSettings=&own.chaseSettings;own.playerCameras.orbitSettings=&own.orbitSettings;
        own.playerCameras.velocitySettings=&own.velocitySettings;own.playerCameras.airborneSettings=&own.airborneSettings;
        own.playerCameras.airbornePathSettings=&own.airbornePathSettings;own.playerCameras.fixedCommands=&own.fixedCommands;
        own.playerCameras.attachedSettings=&own.attachedSettings;own.playerCameras.viewCategory=&first->appearance.dashboardTextures;
        own.playerBindings.registry=&own.players;own.playerBindings.parameters=&parameters;own.playerBindings.input=process.input;
        own.playerBindings.flailer=menus.ui->resources;own.playerBindings.renderManager=menus.ui->render;
        own.playerBindings.language=menus.ui->language;own.playerBindings.clock=process.clock;
        own.playerBindings.cars=&phase.cars;
        own.playerBindings.track=track.track->CurrentInfo();
        own.playerBindings.playerHeadingTrackMap=menus.ui->externalPresentation;
        own.playerBindings.nis={&own.nis,global.nis,&phase.files,&own.views};
        own.screenServices.frontend=&menus;own.screenServices.players=&own.players;
        own.startScreens=std::make_unique<StartRaceScreens>(own.screenServices);BindStartRaceScreens(*own.startScreens);
        own.coordinatorBindings.cameraRegistry=&own.views;own.coordinatorBindings.startScreens=own.startScreens.get();
        own.coordinatorBindings.resources=&phase.files;own.coordinatorBindings.specialPursuit=&parameters.specialPursuit;
        own.coordinatorBindings.demoDisc=bindings.demo;own.coordinatorBindings.clock=process.clock;
        own.coordinatorBindings.replayAvailableBytes=process.replayAvailableBytes;
        own.coordinatorInit={&parameters,&own.playerBindings,&own.worldBindings,&own.raceServices,&own.coordinatorBindings,
            &own.playerCameras,&own.catalog,&track.route->Native(),process.skipStartSequence,
            menus.raceSetup->demoReplay,process.impactAudio,process.oneShotAudio,&sound,&particles};
        own.sessionCameras={&own.views,&own.players};
        sound.spatial.players=&own.players;
        // PAL 16924C activates the selected Stomper mode before PAL 16926C
        // creates RaceCoordinator. AI Demo/alternate race modes skip it.
        // Graphical unlocks without proven Vulkan descriptor targets retain
        // their original menu status, but cannot abort a valid race. Only
        // bound render modes execute native Stomper state transactions.
        if(!parameters.demoReplay && !parameters.fastSpeedTrapSpawns) {
            // Demo invokes PAL DisableRenderEggs. A later normal race must
            // restore the same global owner before selecting its unlock.
            EnableRenderEggs();
            ActivateRenderEggs(easterEggs.records,easterEggs.recordCount);
        }
        // One original source restore and one live native World. Authored
        // input/checksum bytes remain unchanged; no disposable races run
        // during loading and no replay generated by this port replaces them.
        auto& demo=*menus.raceSetup->demoReplay;
        if(parameters.demoReplay && demo.HasLoadedResource())demo.Launch();
        own.coordinator=std::make_unique<RaceCoordinatorOwner>(own.coordinatorInit,own.viewSelection,own.sessionCameras);
        auto& session=own.coordinator->Session();own.movementFeedback.session=&session;
        own.commentary.race=own.coordinatorBindings.race;own.commentary.session=&session;
        own.cameraInput.world=own.coordinatorBindings.world;own.players.sessionState=&session.state;process.cameras->session=&session;
    } catch(...) {
        (void)own.RetireGameplayNoThrow(phase,true);flow.inGameFlag=0;throw;
    }
}

// PAL169288. Rendering must have released its actual submitted resources
// before the coordinator, model and texture borrowers can be retired.
void BeginGameFlowUnloadingTrackSpecific(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings) && bindings.track->storage_,"Track retirement has no actual lifetime owner");
    auto& phase=*bindings.track;auto& own=*phase.storage_;
    auto& menus=*bindings.frontend->startup.menus;
    Require(bindings.renderFrame,"Track retirement has no canonical render frame");
    Require(!RenderingPending(bindings),
        "Track retirement precedes real rendering completion");
    phase.files.Registry().Models().fixupsBlocked=1;flow.inGameFlag=0;
    if(!own.process.launch->skipFrontend)
        bindings.ui->render->SetScreenVisibility(*bindings.ui->resources,0xd0131dd1u,0);
    if(own.coordinator)own.coordinator->Retire();
    own.coordinator.reset();
    // Restore all patched native rendering words before their owning track
    // textures and GPU resources are retired. Persistent unlocks remain in
    // the single EasterEgg record table for the next race.
    ResetRenderEggs();
    phase.hudTextures.UnloadAllTexturesFromFile();
    own.RetireGameplay(phase);
    // The native TerrainData, route and smokeable resources are owned by
    // the actual ResourceFile. Its checked destructor frees those buffers
    // after all live gameplay/renderer borrowers above have disappeared.
    while(auto* file=phase.files.FindResourceFile(3))phase.files.UnloadResourceFile(file);
    phase.trackFile=nullptr;
    Require(soundUnloadRegions(*phase.fileBindings->sound,phase.collisionAudio)==WORLD_AUDIO_OK,
        "Track sound-region retirement failed");
    menus.raceSetup->demoReplay->UnloadResourceFile();
    menus.ui->language->UnloadLocalizedSpeech();
    while(phase.files.HasPendingFileReads())phase.files.ServiceResourceLoading();
    // SlotPoolManager and bCountFreeMemory are console allocator accounting;
    // the native owners above perform checked real release, without a
    // fabricated PS2 free-byte number or diagnostic background-color draw.
    const auto next=flow.argument;
    own.process.launch->skipFrontend=0;
    ChangeGameFlow(flow,next ? next : 8,0);
    const auto& parameters=*menus.raceSetup->parameters;
    if(!parameters.fastSpeedTrapSpawns && !parameters.demoReplay)
        easterEggs.ResetNonPersistentUnlocks();
}

// PAL168DB8. Common A must finish before the compressed common B is queued.
void BeginGameFlowLoadingTrackCommon(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings) && bindings.track->animationDefinitions,
        "Common track loading has no actual animated-driver definitions");
    auto& phase=*bindings.track;
    Require(!phase.files.FindResourceFile(2),"Previous common track files are still live");
    phase.files.Registry().Models().fixupsBlocked=1;
    if (!bindings.frontend->launchOptions.skipFrontend) {
        const auto screen=bindings.demo->ActivationWord()?0xf46de7f7u:0xd0131dd1u;
        bindings.ui->render->SetScreenVisibility(*bindings.ui->resources,screen,0);
    }
    auto file=TrackFileBindings(flow,bindings,ResourceFileKind::TrackCommon);
    phase.files.LoadResourceFile("GLOBAL\\INGAMEA.BUN",2,0,file);
    // Beta3275D8 -> finalGP-5380, the existing preprocessing gate.
    phase.animationDefinitions->rotationPreprocessPending=1;
    while (!phase.files.IsResourceLoadingComplete())phase.files.ServiceResourceLoading();
    phase.commonFile=phase.files.LoadResourceFile("GLOBAL\\INGAMEB.LZC",2,9,file,
        ResourceFileCompletion::TrackCommonLoaded);
    phase.commonFile->ChangeFilenameForHotChunking("GLOBAL\\INGAMEB.BUN");
}

// PAL168AC0. The ordinary and reserved cars are deliberately two passes;
// their duplicate numbers and skin references depend on this load order.
void GameFlowLoadingTrackCommonPart1(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings),"Common car selection lost its actual race parameters");
    auto& parameters=*bindings.frontend->startup.menus->raceSetup->parameters;
    carLoader.singleSkinBatch=false;carLoader.frontendTextures=0;
    for (int32_t i=0;i<parameters.driverCount;++i) {
        auto* driver=raceParametersGetDriverInfo(&parameters,i);
        Require(driver!=nullptr,"Common car selection has an invalid DriverInfo index");
        if (driver->appearance.reserved)continue;
        auto& ride=driver->appearance;
        const auto& car=bindings.track->cars.Car(static_cast<std::size_t>(ride.modelCode));
        const auto selectedSkin=ride.skinNumber;
        if (car.bodyCategory==2)ride.skinNumber=-1;
        const auto duplicate=carLoader.Load(ride);
        ride.variant=car.bodyCategory==2?0:static_cast<std::uint8_t>(duplicate);
        ride.skinNumber=selectedSkin;
    }
    for (int32_t i=0;i<parameters.driverCount;++i) {
        auto* driver=raceParametersGetDriverInfo(&parameters,i);
        Require(driver!=nullptr,"Reserved car selection has an invalid DriverInfo index");
        if (driver->appearance.reserved)
            driver->appearance.variant=static_cast<std::uint8_t>(carLoader.Load(driver->appearance));
    }
    carLoader.BeginLoading(flow,bindings);
}

// PAL168928. Duplicate the actual eSolids after all cars have loaded. The
// native geometry copies are owned by the one canonical model catalogue.
void GameFlowLoadingTrackCommonPart2(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings),"Common geometry completion lost its actual owners");
    auto& parameters=*bindings.frontend->startup.menus->raceSetup->parameters;
    auto& models=bindings.track->files.Registry().Models();
    for (int32_t i=0;i<parameters.driverCount;++i) {
        const auto* driver=raceParametersGetDriverInfo(&parameters,i);
        Require(driver!=nullptr,"Geometry duplication has an invalid DriverInfo index");
        const int duplicate=static_cast<std::int8_t>(driver->appearance.variant);
        if (duplicate<=0)continue;
        const auto& base=bindings.track->cars.Car(static_cast<std::size_t>(driver->appearance.modelCode)).baseName;
        if (driver->appearance.reserved) {
            for (const char suffix:{'A','B'}) {
                const auto original=base+"_"+suffix;
                const auto renamed=original+std::to_string(duplicate);
                ecstasyDuplicateSolid(&models,original.c_str(),renamed.c_str());
            }
        }
        const auto original=base+"_SCUFFS";
        const auto renamed=original+std::to_string(duplicate);
        ecstasyDuplicateSolid(&models,original.c_str(),renamed.c_str());
    }
    ChangeGameFlow(flow,5,0);
}

void BeginGameFlowUnloadingTrackCommon(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings),"Common track retirement lost its actual owners");
    auto& phase=*bindings.track;
    while (auto* file=phase.files.FindResourceFile(2))phase.files.UnloadResourceFile(file);
    phase.commonFile=nullptr;
    ecstasyDeleteAllDuplicatedSolids(&phase.files.Registry().Models());
    carLoader.UnloadEverything();
    // The console free-heap colour diagnostic has no native allocation side
    // effect. Actual resources and loaded cars have been retired above.
    ChangeGameFlow(flow,1,0);
}

void BeginGameFlowLoadingTrackSpecific(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings),"Specific track loading lost its actual owners");
    auto& phase=*bindings.track;
    Require(!phase.files.FindResourceFile(3),"Previous specific track files are still live");
    phase.files.Registry().Models().fixupsBlocked=1;
    const auto number=std::to_string(phase.fileBindings->parameters->selectedScenario);
    const auto a="TRACKS\\TRACKA"+number+".BUN";
    const auto b="TRACKS\\TRACKB"+number+".LZC";
    const auto hot="TRACKS\\TRACKB"+number+".BUN";
    auto file=TrackFileBindings(flow,bindings,ResourceFileKind::TrackAuxiliary);
    phase.files.LoadResourceFile(a.c_str(),3,0,file);
    file.kind=ResourceFileKind::Track;
    phase.trackFile=phase.files.LoadResourceFile(b.c_str(),3,9,file,ResourceFileCompletion::TrackSpecificLoaded);
    phase.trackFile->ChangeFilenameForHotChunking(hot.c_str());
}

void GameFlowLoadingTrackSpecificPart1(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings),"Specific track callback lost its actual application owners");
    auto& menu=*bindings.frontend->startup.menus;
    menu.ui->language->LoadLocalizedSpeech();
    Require(menu.raceSetup->demoReplay,"Track callback has no actual demo replay owner");
    // Frontend handed the same loaded recording to the race. A second load
    // would either reject an active owner or append a different snapshot to
    // the replay list. An attract Demo with no saved file may still be a live
    // AI-only race and must not invent a replay stream.
    if (menu.raceSetup->demoReplay->active && !menu.raceSetup->demoReplay->HasLoadedResource())
        menu.raceSetup->demoReplay->LoadResourceFile(const_cast<GameResources&>(bindings.track->files.Files()),*bindings.clock);
    Require(soundLoadRegionsInGame(*menu.sounds,*menu.frontend,*menu.raceSetup->parameters,
        bindings.track->collisionAudio,bindings.track->collisionSystem,flow,bindings)==WORLD_AUDIO_OK,
        "Track sound-region loading failed");
}

void GameFlowLoadingTrackSpecificPart2(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{LoadHudTextures(flow,bindings);}

// PAL168C30 includes the released extra generic-gauge request. The native
// completion marker remains in the actual HUD TexturePack read queue.
void LoadHudTextures(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings),"HUD textures lost the current track flow");
    auto& parameters=*bindings.frontend->startup.menus->raceSetup->parameters;
    Require(parameters.runtimeSettings,"HUD textures have no canonical display settings");
    auto& pack=bindings.track->hudTextures;
    const auto units=parameters.runtimeSettings->display.languageDependentOption;
    if (parameters.mode==1) {
        auto* driver=raceParametersGetDriverInfoByDriverNumber(&parameters,0);
        if (!driver || driver->eliminated>0)driver=raceParametersGetDriverInfoByDriverNumber(&parameters,1);
        if (driver && driver->eliminated<=0) {
            const auto& car=bindings.track->cars.Car(static_cast<std::size_t>(driver->appearance.modelCode));
            // Playable traffic uses the existing digital speed/gear HUD.
            // Its resource records contain no model-specific gauge textures.
            if(car.bodyCategory!=2) {
                const auto gauges=ChooseLoadableTextures(car,units);
                pack.LoadTexturesFromFile(&gauges.speedometer,1);
                pack.LoadTexturesFromFile(&gauges.units,1);
                pack.LoadTexturesFromFile(&gauges.generic,1);
                if (gauges.tachometer)pack.LoadTexturesFromFile(&gauges.tachometer,1);
            }
        }
    }
    const auto key=hashString(units==1?"3RDPERSONGAUGE_KMH":"3RDPERSONGAUGE_MPH");
    pack.LoadTexturesFromFile(&key,1,flow,bindings);
}

void GameFlowLoadingTrackSpecificPart3(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings),"Pre-race resource callback lost its track owner");
    if (!bindings.frontend->launchOptions.skipFrontend) {
        auto file=TrackFileBindings(flow,bindings,ResourceFileKind::TrackAuxiliary);
        bindings.track->files.LoadResourceFile("GLOBAL\\PRERACEANIM.LZC",3,8,file,ResourceFileCompletion::TrackPreRaceLoaded);
    } else GameFlowLoadingTrackSpecificPart4(flow,bindings);
}

void GameFlowLoadingTrackSpecificPart4(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    auto file=TrackFileBindings(flow,bindings,ResourceFileKind::TrackAuxiliary);
    auto& phase=*bindings.track;
    const auto number=std::to_string(phase.fileBindings->parameters->selectedScenario);
    const auto track="TRACKS\\TEX"+number+"TRACK.BIN";
    const auto location="TRACKS\\TEX"+number+"LOCATION.BIN";
    phase.files.LoadResourceFile(track.c_str(),3,1,file);
    phase.files.LoadResourceFile(location.c_str(),3,1,file,ResourceFileCompletion::TrackLocationTexturesLoaded);
}

void GameFlowLoadingTrackSpecificPart5(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    auto file=TrackFileBindings(flow,bindings,ResourceFileKind::TrackAuxiliary);
    bindings.track->files.LoadResourceFile("GLOBAL\\DRIVTEX.BIN",3,0,file,ResourceFileCompletion::TrackDriverTexturesLoaded);
}

void GameFlowLoadingTrackSpecificPart6(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(TrackBound(bindings),"Track ready callback lost its actual screen owners");
    const auto screen=bindings.demo->ActivationWord()?0xf46de7f7u:0xd0131dd1u;
    bindings.ui->render->SetScreenVisibility(*bindings.ui->resources,screen,1);
    ChangeGameFlow(flow,6,0);
}

const char* GetGameFlowName(std::int32_t state)
{
    Require(static_cast<std::uint32_t>(state)<10u,"GameFlow name index is outside the released table");
    return flowRows[state].name;
}

void BeginGameFlowExitDemoDisc(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(bindings.clock,"Exit demo state has no actual application clock");
    flow.exitDemoStartedTicks=bindings.clock->realTimer;
}

void GameFlowExitDemoDisc(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(UiBound(bindings) && bindings.dynamicTextures,"Exit demo has no actual texture/UI owners");
    if (!bindings.dynamicTextures->HasPendingReads()) {
        bindings.ui->render->SetScreenVisibility(*bindings.ui->resources,0xd0131dd1u,1);
        bindings.ui->render->SetScreenVisibility(*bindings.ui->resources,0x991ba7e2u,0);
    }
    const auto ticks=WrappingDifference(bindings.clock->realTimer,flow.exitDemoStartedTicks);
    if (15.0f<static_cast<float>(ticks)*0.0002500000118743628f)ChangeGameFlow(flow,0,0);
}

void EndGameFlowExitDemoDisc(const GameFlowBindings& bindings)
{
    Require(bindings.demo,"Exit demo has no actual DemoDisc owner");
    bindings.demo->EndDemo();
}

void GameFlowLoadingFrontEnd(const ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    Require(bindings.clock && bindings.demo,"Frontend loading has no actual application/demo clock");
    if (!bindings.demo->ActivationWord())return;
    const auto loops=WrappingDifference(bindings.clock->realLoopCounter,flow.loadingFrontendStartedLoop);
    if (loops<181 || loops>=541)return;
    Require(UiBound(bindings),"Frontend loading has no actual Flailer owners");
    auto& ui=*bindings.ui;
    if (loops<361) {
        ui.render->SetScreenVisibility(*ui.resources,0x8139ec90u,1);
        ui.render->SetScreenVisibility(*ui.resources,0xa7227ae8u,0);
    } else {
        ui.render->SetScreenVisibility(*ui.resources,0xa7227ae8u,1);
        ui.render->SetScreenVisibility(*ui.resources,0x8254cc20u,0);
    }
}

GameFlowServiceResult ServiceGameFlow(ApplicationFrontendFlow& flow,const GameFlowBindings& bindings)
{
    do {
        if (static_cast<std::uint32_t>(flow.currentState)>=10u ||
            static_cast<std::uint32_t>(flow.requestedState)>=10u)
            return {GameFlowServiceStatus::InvalidState,0,flow.requestedState};
        if (flow.currentState!=flow.requestedState) {
            const auto ending=flowRows[flow.currentState].end;
            if (ending!=Op::None && ending!=Op::EndExitDemo && ending!=Op::EndLoadingFrontend &&
                ending!=Op::EndLoadingTrackSpecific)
                return {GameFlowServiceStatus::MissingSourceDependency,SourceDependency(ending),flow.currentState};
            const auto beginning=flowRows[flow.requestedState].begin;
            if (beginning!=Op::None && beginning!=Op::BeginNone && beginning!=Op::BeginExitDemo &&
                beginning!=Op::BeginLoadingFrontend && beginning!=Op::BeginUnloadingFrontend &&
                beginning!=Op::BeginLoadingTrackCommon && beginning!=Op::BeginLoadingTrackSpecific &&
                beginning!=Op::BeginUnloadingTrackCommon && beginning!=Op::BeginUnloadingTrackSpecific)
                return {GameFlowServiceStatus::MissingSourceDependency,SourceDependency(beginning),flow.requestedState};
            if ((ending==Op::EndExitDemo && !bindings.demo) ||
                (beginning==Op::BeginExitDemo && (!UiBound(bindings) || !bindings.dynamicTextures)) ||
                ((beginning==Op::BeginLoadingFrontend || beginning==Op::BeginUnloadingFrontend ||
                    ending==Op::EndLoadingFrontend) && !FrontendBound(bindings)) ||
                ((beginning==Op::BeginLoadingTrackCommon || beginning==Op::BeginLoadingTrackSpecific ||
                    beginning==Op::BeginUnloadingTrackCommon || beginning==Op::BeginUnloadingTrackSpecific ||
                    ending==Op::EndLoadingTrackSpecific) && !TrackBound(bindings)))
                return {GameFlowServiceStatus::InvalidBinding,
                    SourceDependency(beginning!=Op::None ? beginning : ending),flow.requestedState};
            // PAL eWaitUntilRenderingDone blocks here. The one real frame
            // retains both Flailer strips and 3D texture/geometry borrowers.
            if (beginning==Op::BeginUnloadingFrontend || beginning==Op::BeginUnloadingTrackSpecific) {
                if(!bindings.renderFrame)
                    return {GameFlowServiceStatus::InvalidBinding,0x111dd8,flow.currentState};
                if(RenderingPending(bindings))
                    return {GameFlowServiceStatus::PendingRenderCompletion,0x111dd8,flow.currentState};
            }
            if(ending==Op::EndLoadingTrackSpecific || beginning==Op::BeginUnloadingTrackSpecific) {
                if(!bindings.track->Players())
                    return {GameFlowServiceStatus::InvalidBinding,SourceDependency(ending==Op::EndLoadingTrackSpecific ? ending : beginning),flow.currentState};
                if(ending==Op::EndLoadingTrackSpecific) {
                    const auto& parameters=*bindings.frontend->startup.menus->raceSetup->parameters;
                    if(!parameters.demoReplay && !parameters.fastSpeedTrapSpawns &&
                        (!easterEggs.records || easterEggs.recordCount<=8))
                        return {GameFlowServiceStatus::InvalidBinding,0x24bf18,flow.currentState};
                    // PS2 graphics-egg addresses cannot be dereferenced in
                    // the Vulkan renderer. Eligibility for a race is not
                    // dependent on an optional visual effect being bound.
                }
            }
            if (ending==Op::EndExitDemo)EndGameFlowExitDemoDisc(bindings);
            else if (ending==Op::EndLoadingFrontend)EndGameFlowLoadingFrontEnd(flow,bindings);
            else if (ending==Op::EndLoadingTrackSpecific)EndGameFlowLoadingTrackSpecific(flow,bindings);
            // Reload the requested state after End; this ordering is source
            // behavior, even though the currently closed End only exits.
            flow.currentState=flow.requestedState;
            if (flowRows[flow.currentState].begin==Op::BeginExitDemo)BeginGameFlowExitDemoDisc(flow,bindings);
            else if (flowRows[flow.currentState].begin==Op::BeginLoadingFrontend)BeginGameFlowLoadingFrontEnd(flow,bindings);
            else if (flowRows[flow.currentState].begin==Op::BeginUnloadingFrontend)BeginGameFlowUnloadingFrontEnd(flow,bindings);
            else if (flowRows[flow.currentState].begin==Op::BeginLoadingTrackCommon)BeginGameFlowLoadingTrackCommon(flow,bindings);
            else if (flowRows[flow.currentState].begin==Op::BeginLoadingTrackSpecific)BeginGameFlowLoadingTrackSpecific(flow,bindings);
            else if (flowRows[flow.currentState].begin==Op::BeginUnloadingTrackCommon)BeginGameFlowUnloadingTrackCommon(flow,bindings);
            else if (flowRows[flow.currentState].begin==Op::BeginUnloadingTrackSpecific)BeginGameFlowUnloadingTrackSpecific(flow,bindings);
            // BeginGameFlowNone168650 is exactly JR RA/NOP, omitted.
        }
        switch (flowRows[flow.currentState].tick) {
        case Op::LoadingFrontend:
            if (!bindings.clock || !bindings.demo)
                return {GameFlowServiceStatus::InvalidBinding,0x168718,flow.currentState};
            GameFlowLoadingFrontEnd(flow,bindings);break;
        case Op::ExitDemo:
            if (!UiBound(bindings) || !bindings.dynamicTextures)
                return {GameFlowServiceStatus::InvalidBinding,0x1694a0,flow.currentState};
            GameFlowExitDemoDisc(flow,bindings);break;
        default:break; // The other actual ticks are NULL or JR RA/NOP169160.
        }
    } while (flow.currentState!=flow.requestedState);
    return {GameFlowServiceStatus::Complete,0,flow.currentState};
}

void ChangeGameFlow(ApplicationFrontendFlow& flow,std::int32_t state,std::int32_t argument)
{
    flow.requestedState=state;
    flow.argument=argument;
}

bool IsGameFlowPaused(const GameSession* session) noexcept
{
    return session && session->state == 6;
}

void GameFlowNotifyLanguageChange(ApplicationFrontendFlow& flow,TexturePack& dynamicTextures,
    ResourceLoader& loader,std::int32_t current,std::int32_t previous)
{
    (void)previous;
    if ((current==1)==flow.languageTexturesFrench)return;
    const std::uint32_t loading=0xab0e817du;
    if (dynamicTextures.GetLoadedTexture(loading))dynamicTextures.UnloadTexturesFromFile(&loading,1);
    auto& registry=dynamicTextures.Registry();
    SwapTextureName(registry,loading,hashString("LOADING_FR"));
    dynamicTextures.LoadTexturesFromFile(&loading,1);
    while (dynamicTextures.HasPendingReads())loader.ServiceResourceLoading();
    SwapTextureName(registry,hashString("NFS_LOGO_ALPHA"),hashString("NFS_LOGO_ALPHA_FR"));
    flow.languageTexturesFrench=!flow.languageTexturesFrench;
}

} // namespace hp2
