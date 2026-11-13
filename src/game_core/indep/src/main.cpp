#include <exception>
#include "Flags.hpp"
#include "port/not_implemented.hpp"
#include "bware/inc/bWare.hpp"
#include "DemoDisc.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bFile.hpp"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/eEnvMap.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/FMenus.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/World.hpp"
#include "bware/inc/bWare.hpp"
#include <fstream>
#include <stdexcept>
#include <string>
#include "indep/inc/Fred.hpp"
#include "FMenus.hpp"
#include "indep/inc/Language.hpp"
#include "indep/inc/DemoModeReplay.hpp"
#include "indep/inc/CarLoader.hpp"
#include "indep/inc/RealDriver.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrafficDriver.hpp"
#include "indep/inc/AIPlan.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Smokeable.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/Sound3dSample.hpp"
#include "indep/inc/SpeechNoise.hpp"
#include "indep/inc/WeatherMan.hpp"
#include "indep/inc/EasterEggs.hpp"

#include "indep/inc/eMath.hpp"
#include "indep/src/MoviePlayer/MV3Codec.h"
#include "indep/src/speech/mgr.hpp"
#include "bsound/include/bSound.h"
#include "bsound/include/bJukebox.h"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/CarDriver.hpp"
#include <array>
#include <cstring>
#include <cstdio>
#include <thread>

// Native terminal lifetime API in the proven weatherman.cpp definition path.
// WeatherMan.hpp retains its inherited scope; no new header identity is claimed.
extern "C" WeatherManResult weatherManUnbindClock(const GameTime* clock);
namespace hp2 { void InitDisplaySystem(const GameResources& files,GameTime& clock); }

ApplicationBootState applicationBootState={-1};
ApplicationFrameTiming applicationFrameTiming{};
float applicationPreviousCpuFrameRate=0.0f;
float applicationPreviousGpuFrameRate=0.0f;
bool applicationGpuTimingAvailable=false;

// PAL12EE30..12EE94; same process vector RNG. vectorRandomSeed performs
// its actual initial bound1 draw; assigning a seed directly would differ.
void applicationSeedRandomNumber(ReplayIntegerLog& log)
{
    if (!log.clock || (log.replaying && !log.stream))
        throw std::invalid_argument("Application seed has no canonical clock/Joylog binding");
    uint32_t seed;
    if (log.replaying)seed=replayIntegerLogRead(&log,32,0);
    else {
        seed=hp2NativeTickerSource()+UINT32_C(0x12345678);
        replayIntegerLogWrite(&log,seed,32,0);
    }
    (void)vectorRandomSeed(seed,&vectorDefaultRandomState);
}

// PAL12EDA0..12EE2C. The PC file adapter accepts readable config.ini and
// read-only legacy config.bin. Its one selected word and Joylog order are
// unchanged; malformed input fails before the original cell is published.
void applicationLoadConfig(const std::filesystem::path& filename,ReplayIntegerLog& log)
{
    if (!log.clock || (log.replaying && !log.stream))
        throw std::invalid_argument("Application config has no canonical clock/Joylog binding");
    if (log.replaying) {
        applicationBootState.trackSelectedInConfigFile=static_cast<int32_t>(replayIntegerLogRead(&log,32,0));
        return;
    }
    const auto selected=hp2::loadNativeTrackConfiguration(filename);
    applicationBootState.trackSelectedInConfigFile=selected;
    replayIntegerLogWrite(&log,static_cast<uint32_t>(selected),32,0);
}

// Complete PAL12F190..12F374. The total checksum word is consumed without a
// comparison in the release. Every sixteenth call additionally compares the
// five components; bBreak is adapted to a descriptive native failure.
void applicationVerifyJoylogChecksum(ApplicationChecksumState& state,ReplayIntegerLog& log,
    WorldSimulation* simulation,const hp2::FlailerScreenList* screens,
    const hp2::ApplicationFrontendFlow& flow)
{
    hp2::Require(log.clock,"Application checksum has no canonical game clock");
    uint16_t world=0,screen=0;
    if (simulation) {
        hp2::Require(simulation->world && simulation->replay && simulation->clock==log.clock,
            "Application checksum world has no canonical clock/replay bindings");
        world=worldCalculateDebugChecksum(simulation->world,static_cast<uint32_t>(log.clock->worldTimer),
            simulation->replay->checksumContribution);
    }
    if (screens && screens->active) {
        hp2::Require(screens->active->screen,"Active source screen node has no first screen");
        screen=static_cast<uint16_t>(screens->active->screen->screenId);
    }
    const uint16_t random=static_cast<uint16_t>(vectorDefaultRandomState);
    const uint16_t loop=static_cast<uint16_t>(log.clock->realLoopCounter);
    const uint16_t flowState=static_cast<uint16_t>(flow.currentState);
    const uint16_t total=static_cast<uint16_t>(world+loop+random+screen+flowState);
    if (log.replaying)(void)replayIntegerLogRead(&log,16,3);
    else replayIntegerLogWrite(&log,total,16,3);
    if ((state.calls++&15u)!=0)return;
    const uint16_t components[]={world,screen,random,flowState,loop};
    // Source re-reads the live replaying flag here: consuming the total may
    // have closed the stream at its end. Do not retain a stale mode snapshot.
    if (log.replaying) {
        unsigned differences=0;
        for (const auto value:components)
            differences+=value!=replayIntegerLogRead(&log,16,3);
        if (differences)throw std::runtime_error("Original application Joylog checksum mismatch");
    } else {
        for (const auto value:components)replayIntegerLogWrite(&log,value,16,3);
    }
}

namespace hp2 {

struct ApplicationResourceRuntime::Storage {
    FileIoRuntime io;
    QueuedFileService queued;
    struct AnimationRoot {
        CarReplayAnimationRegistry registry{};
        AnimationRoot() {carReplayAnimationRegistryInitialize(&registry);}
    } animations;
    EditorMarkerResources models{};
    std::unique_ptr<CarViewCameraResources,decltype(&carViewCameraResourcesDispose)> cameras;
    AcidFXRuntime fx;
    ChopperEffectData effectsRoot;
    FlailerResources flailer;
    ResourceRegistry registry;
    ResourceGlobalBindings global;
    ResourceLoader loader;
    const DemoDiscManager* demo=nullptr;

    Storage(const GameResources& files,const CarCinematicServices& views,ReplayIntegerLog& log,CarInfoAvailability availability)
        :queued(files,io,log),cameras(carViewCameraResourcesCreate(),carViewCameraResourcesDispose),fx(views),
         effectsRoot(std::vector<std::uint8_t>{}),flailer(models),
         registry(models,animations.registry,effectsRoot,flailer),global{fx,availability},loader(files,registry,queued)
    {
        if (!cameras)throw std::bad_alloc();
        Require(availability==CarInfoAvailability::FullGame || availability==CarInfoAvailability::DemoDisc,
            "Application resources require an actual content-policy owner");
    }
};

ApplicationResourceRuntime::ApplicationResourceRuntime(const GameResources& files,
    const CarCinematicServices& cameras,ReplayIntegerLog& log,CarInfoAvailability availability)
    :storage_(std::make_unique<Storage>(files,cameras,log,availability)) {}

ApplicationResourceRuntime::ApplicationResourceRuntime(const GameResources& files,
    const CarCinematicServices& cameras,ReplayIntegerLog& log,const DemoDiscManager& demo)
    :ApplicationResourceRuntime(files,cameras,log,demo.Availability())
{
    storage_->demo=&demo;
}

ApplicationResourceRuntime::~ApplicationResourceRuntime()=default;
ResourceRegistry& ApplicationResourceRuntime::Registry() noexcept {return storage_->registry;}
ResourceLoader& ApplicationResourceRuntime::Loader() noexcept {return storage_->loader;}
QueuedFileService& ApplicationResourceRuntime::QueuedFiles() noexcept {return storage_->queued;}
FileIoRuntime& ApplicationResourceRuntime::FileIo() noexcept {return storage_->io;}
AcidFXRuntime& ApplicationResourceRuntime::FX() noexcept {return storage_->fx;}
CarViewCameraResources& ApplicationResourceRuntime::CameraResources() noexcept {return *storage_->cameras;}
ResourceFileBindings ApplicationResourceRuntime::GlobalFileBindings() const
{
    if (storage_->demo)storage_->global.availability=storage_->demo->Availability();
    ResourceFileBindings bindings;
    bindings.kind=ResourceFileKind::Global;bindings.global=&storage_->global;
    return bindings;
}

} // namespace hp2

namespace hp2 {
namespace {
// The released process owns one input service. This C-ABI trampoline keeps
// a lifetime borrow only; no second controller packet or game state exists.
NativeApplicationHost* applicationHost{};
int32_t ReadApplicationInput(int32_t port,JoystickControls* destination)
{
    Require(applicationHost,"Physical input outside application lifetime");
    Require(destination,"Physical input has no native destination");
    return applicationHost->ReadInput(port,*destination);
}
void RequireAudio(WorldAudioResult result,const char* operation,const bSound* sound=nullptr)
{
    if(result==WORLD_AUDIO_OK)return;
    auto message=std::string(operation)+" failed, audio result="+std::to_string(static_cast<int>(result));
    if(sound) {
        const auto cause=sound->FailureDescription();
        if(!cause.empty())message+="; "+cause;
    }
    throw std::runtime_error(message);
}
}

struct ApplicationRuntime::Storage {
    NativeDurationStatistics sourceUpdateTiming,presentationTiming;
    double deferredPresentationCpuMilliseconds{};
    NativeFrameTiming frameTiming;
    NativeSimulationClock simulationClock;
    bool pendingClockAdvance{},pendingLegacyRealClock{},sourceJoylogTiming{};
    std::uint32_t scheduledStepQuanta{60};
    float replayStepSeconds{1.0f/60.0f};
    std::uint64_t presentationTimingEpoch{};
    bool frameTimingStarted{};
    std::chrono::steady_clock::time_point renderTimingWindowStart{std::chrono::steady_clock::now()};
    std::uint64_t renderTimedFrames{},gpuTimedFrames{},lastCompletedGpuFrame{};
    double renderUpdateMs{},renderGatherMs{},renderFenceMs{},renderSubmitMs{},renderPresentMs{},renderGpuMs{};
    GameResources& files;NativeApplicationHost& host;
    std::filesystem::path profileDirectory,legacyProfileDirectory;
    GameTime clock{};ReplayIntegerLog joylog{};ApplicationChecksumState checksum{};
    DemoDiscManager& demo{TheDemoDiscManager};
    CarCinematicServices cameras{};
    std::array<CarCinematicView,4> views{};
    std::array<EcstasyRenderTarget,15> targets{};
    std::unique_ptr<EnvironmentMap> environmentMap;
    std::unique_ptr<ApplicationResourceRuntime> resources;
    ResourceFile* global{};
    std::unique_ptr<TrackInfoResources> tracks;
    LanguageManager language;
    FrontendPlayerManager profilesOwner;
    // Immutable launch preference wins over an older asynchronously loaded
    // profile's PC display extension. FRED remains the live display owner.
    std::optional<NativeDisplaySettings> launchDisplay;
    std::array<FrontendPlayerInfo,2> profiles{};
    std::unique_ptr<HighScoreTable> highScores;
    RaceParameters parameters{};RaceRuntimeSettings runtime{};
    std::array<RaceDriverInfo,RACE_DRIVER_LIMIT> drivers{};
    RaceEvent event;
    DemoModeReplay demoReplay;
    ApplicationFrontendFlow flow;
    FrontendRaceSetupBindings setup{};
    std::unique_ptr<TexturePack> dynamicTextures,hudTextures,carTextures;
    CarTextureNamingPolicy carNaming;PositionMarkerRegistry positionMarkers{};
    GameInputRouter input{};std::array<GameInputRegistration,256> registrations{};
    JoystickSystem samples{};RealDriverTuning driverTuning{};
    WorldAudioRequestQueue audioQueue{};std::array<WorldAudioRequest,4096> audioRequests{};
    bSound sound;CarSoundBindings soundBindings{};
    SoundVolumeSettings volumes;
    FlailerSoundClock soundClock{0,4000.0f};FlailerSoundState uiSound;
    WorldAudioMessages speechMessages{};WorldStreamAudio speechStream{};
    Sound3dSampleRegistry positionalSamples;WorldImpactAudio impactAudio{};WorldAudioEffects impactEffects{};
    FlailerUiState ui;FlailerRenderManager renderer;
    FlailerScreenList screens;
    std::unique_ptr<FlailerHelpMenu> help;
    std::unique_ptr<FlailerGlobalBackgroundAnimation> background;
    std::array<std::unique_ptr<FlailerPopup>,2> popups;
    std::unique_ptr<FlailerLoadingScreenAnimation> loadingAnimation;
    std::vector<std::unique_ptr<FlailerScreenStorage>> screenStorage;
    FlailerFrontendInput frontendInput{};FrontendDemoMode demoMode;
    MainMenuScreenServices menus{};FrontendStartupServices startup{};
    SelectCarScreenServices carSelection{};SelectTrackScreenServices trackSelection{};FrontendFrame frontendFrame;
    std::unique_ptr<MoviePlayer> movie;
    std::unique_ptr<GameFlowFrontendResources> frontendPhase;
    std::unique_ptr<GameFlowTrackResources> trackPhase;
    std::unique_ptr<MemcardScreens> memcards;
    GameFlowBindings flowBindings{};
    std::unique_ptr<EcstasyFrame> frame{std::make_unique<EcstasyFrame>()};
    int32_t skipStartSequence{},alternateSkin{};
    int requestedTrack{-1};
    bool (*supportsFrontendTrack)(int){};
    bool showFrontend{};
    bool stateOnly() const noexcept {return requestedTrack>=0;}
    bool quickRaceRequested{};
    // PAL1BE910 requests the same bStartGlare consumed by StuffSunGlare.
    // The application's Sun owner retains its original textures and clock.
    int32_t impactPresentationMode{1};
    size_t replayAvailableBytes{};
    bool initialized{},inputBound{},audioBound{},audioOpened{},speechBound{},menusBound{},mainMenuBound{},languageInitialized{};
    bool speechNoiseBound{},soundScapeBound{},jukeboxBound{};
    bool shutdownStarted{},shutdownComplete{};
    std::exception_ptr shutdownFailure;
    void RecordCleanupFailure(const char* operation,std::exception_ptr failure) noexcept
    {
        if(!failure)return;
        if(!shutdownFailure)shutdownFailure=failure;
        char detail[768]{};
        try {std::rethrow_exception(failure);}
        catch(const std::exception& error) {
            std::snprintf(detail,sizeof(detail),"%s: %s",operation,error.what());
        } catch(...) {
            std::snprintf(detail,sizeof(detail),"%s: non-standard exception",operation);
        }
        host.RecordSessionEvent("shutdown.failure",detail);
    }
    template<class F> void Retire(const char* operation,F&& action) noexcept
    {
        try {action();}catch(...) {RecordCleanupFailure(operation,std::current_exception());}
    }
    template<class F> void RetireAudio(const char* operation,F&& action) noexcept
    {
        Retire(operation,[&] {
            auto result=action();
            for(unsigned retry=0;result==WORLD_AUDIO_CAPACITY && retry<4;++retry) {
                RequireAudio(sound.CancelDeviceRequestsAfterClose(),"Terminal audio cancellation",&sound);
                result=action();
            }
            RequireAudio(result,operation,&sound);
        });
    }
    template<class F> void RetireSpeech(const char* operation,F&& action) noexcept
    {
        Retire(operation,[&] {
            auto result=action();
            for(unsigned retry=0;(result==WORLD_STREAM_AUDIO_BUSY || result==WORLD_STREAM_AUDIO_CAPACITY) && retry<4;++retry) {
                RequireAudio(sound.CancelDeviceRequestsAfterClose(),"Terminal speech cancellation",&sound);
                result=action();
            }
            Require(result==WORLD_STREAM_AUDIO_OK,operation);
        });
    }
    std::array<int32_t,7> journalState{{INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN}};
    uint32_t journalSummaryTicks{};
    void RecordLifecycle(WorldSimulation* simulation,GameSession* session)
    {
        const auto* first=parameters.drivers && parameters.driverCount>0?parameters.drivers:nullptr;
        const auto* active=screens.active?screens.active->screen:nullptr;
        const std::array<int32_t,7> state{{active?active->screenId:-1,session?session->state:-1,
            parameters.selectedScenario,parameters.mode,parameters.driverCount,
            first?first->appearance.modelCode:-1,first?first->appearance.skinNumber:-1}};
        char detail[640]{};
        if(state!=journalState) {
            journalState=state;
            std::snprintf(detail,sizeof(detail),"screen=%08x race_state=%d scenario=%d mode=%d drivers=%d player_model=%d skin=%d demo=%d",
                static_cast<unsigned>(state[0]),state[1],state[2],state[3],state[4],state[5],state[6],demoReplay.active);
            host.RecordSessionEvent("application.state",detail);
        }
        const auto ticks=static_cast<uint32_t>(clock.realTimer);
        if(!simulation || !simulation->world || ticks-journalSummaryTicks<20000u)return;
        journalSummaryTicks=ticks;
        const auto& world=*simulation->world;const auto* car=world.totalCars>0?world.carTable[0]:nullptr;
        const auto* movement=car?static_cast<const CarMovement*>(car->movement):nullptr;
        const auto* vehicle=movement?movement->bindings.state:nullptr;
        const auto* replay=simulation->replay?simulation->replay->manager:nullptr;
        std::snprintf(detail,sizeof(detail),"world_loop=%d timer=%d scenario=%d cars=%d player_model=%d mover=%d "
            "position=(%.3f,%.3f,%.3f) speed=%.3f restoring=%d recording=%d exhausted=%d desynchronized=%d "
            "source_sync_policy=%d source_samples=%u source_mismatches=%u first_source_mismatch_loop=%d "
            "application_fps=%.2f frame_ms=%.3f cpu_ms=%.3f gpu_ms=%.3f gpu_available=%d",
            clock.worldLoopCounter,clock.worldTimer,parameters.selectedScenario,world.totalCars,state[5],movement?movement->mode:-1,
            static_cast<double>(car?car->position[0]:0.0f),static_cast<double>(car?car->position[1]:0.0f),
            static_cast<double>(car?car->position[2]:0.0f),static_cast<double>(vehicle?vehicle->speed:0.0f),
            replay?replay->restoring:0,replay?replay->recording:0,replay?replay->exhausted:0,replay?replay->desynchronized:0,
            replay?static_cast<int>(replay->synchronizationPolicy):0,
            static_cast<unsigned>(replay?replay->sourceReferenceSamples:0),
            static_cast<unsigned>(replay?replay->sourceReferenceMismatchCount:0),
            replay?replay->firstSourceReferenceMismatchLoop:-1,
            applicationFrameTiming.framesPerSecond,applicationFrameTiming.frameMilliseconds,
            applicationFrameTiming.cpuMilliseconds,applicationFrameTiming.gpuMilliseconds,
            applicationFrameTiming.gpuTimeAvailable?1:0);
        if(demoReplay.active)demoReplay.RecordSessionEvent("demo.status",nullptr,detail);
        else host.RecordSessionEvent("gameplay.status",detail);
        // A bounded observation of the actual owners makes a live AI failure
        // distinguishable from authored input playback and device starvation.
        // It never refreshes a road, services a voice or changes a controller.
        const auto* speech=worldSpeechGetInstance();
        const auto* voice=speechStream.voice.instance;
        std::snprintf(detail,sizeof(detail),"world_loop=%d enabled=%d speech_enabled=%d queued=%u retained=%u "
            "speech_state=%d events=%u voice_playing=%d voice_paused=%d voice_muted=%d voice_volume=%.5f category6=%.5f",
            clock.worldLoopCounter,carSoundGlobals.policy.audioSystemEnabled,soundDriverState.speechEnabled,
            static_cast<unsigned>(audioQueue.count),static_cast<unsigned>(audioQueue.retainedCount),speechStream.state,
            static_cast<unsigned>(speech?speech->queue.count:0),voice?voice->playing:0,voice?voice->paused:0,
            voice?voice->muted:0,static_cast<double>(voice?voice->volume:0.0f),
            static_cast<double>(sound.GetCategoryVolume(6)));
        host.RecordSessionEvent("audio.status",detail);
        if(session && (session->state==RC_STATE_AI_DEMO || demoReplay.active)) {
            const auto* observed=car;
            const auto* players=trackPhase->Players();
            if(players && players->count && players->players[0] && players->players[0]->car)
                observed=players->players[0]->car->bindings.owner;
            const auto* driver=observed?static_cast<const CarDriver*>(observed->controller):nullptr;
            const auto* motion=observed?static_cast<const CarMovement*>(observed->movement):nullptr;
            const auto* ai=driver && driver->kind==CAR_DRIVER_AI?driver->owner.ai:nullptr;
            const auto* road=ai?ai->base.currentRoad:nullptr;
            const auto* action=ai?ai->base.action:nullptr;
            std::snprintf(detail,sizeof(detail),"world_loop=%d path=%s recording=%d driver=%d kind=%d "
                "mover=%d action=%d action_status=%d road_group=%d lane=%d path_points=%d target_speed=%.3f progress=%.6f",
                clock.worldLoopCounter,demoReplay.active?"authored_input":"live_ai",demoReplay.selectedRecording,
                observed?observed->driverNumber:-1,driver?static_cast<int>(driver->kind):-1,motion?motion->mode:-1,
                action?action->kind:-1,action?static_cast<int>(action->status):-1,
                road && road->group?road->group->id:-1,road?road->lane:-1,ai?ai->input.command.count:0,
                static_cast<double>(ai?ai->input.targetSpeed:0.0f),
                static_cast<double>(motion?motion->routeLocation.progress:0.0f));
            host.RecordSessionEvent("demo.driver",detail);
        }
    }
    void AudioResult(WorldAudioResult result,const char* operation) const
    {
        if(result!=WORLD_AUDIO_OK) {
            char detail[256]{};std::snprintf(detail,sizeof(detail),"operation=%s result=%d",operation,static_cast<int>(result));
            host.RecordSessionEvent("audio.failure",detail);
        }
        RequireAudio(result,operation,&sound);
    }
    void RefreshNativeDisplayGeometry()
    {
        const auto width=host.Width(),height=host.Height();
        Require(width>0 && height>0,"Native render extent is unavailable");
        cameras.projection.nativeSourceVideoMode=gameTimeGetVideoMode(&clock);
        if(targets[0].width==width && targets[0].height==height && cameras.projection.horizontalResolution==width)return;
        for(std::size_t index=0;index<8;++index) {targets[index].width=width;targets[index].height=height;}
        cameras.projection.horizontalResolution=width;
        // The source view switch sets player/split/rear subrectangles. The
        // six cubemap faces and mapped environment image keep authored sizes.
        ecstasyViewsApplyMode(&cameras);
    }
    Storage(GameResources& source,NativeApplicationHost& physical,const std::filesystem::path& directory,
        const std::filesystem::path& legacyDirectory)
        :files(source),host(physical),profileDirectory(directory),legacyProfileDirectory(legacyDirectory)
    {
        gameTimeInitialize(&clock);gameTimeUseNativeTiming(&clock);joylog.clock=&clock;cameras.clock=&clock;
        parameters.drivers=drivers.data();parameters.runtimeSettings=&runtime;
        profilesOwner.players={{&profiles[0],&profiles[1]}};
    }
    void DrainFiles()
    {
        auto& loader=resources->Loader();
        for(;;) {
            bool pending=!loader.IsResourceLoadingComplete() || loader.HasPendingFileReads();
            for(const auto* pack:texturePackRegistry.packs)pending=pending || pack->HasPendingReads();
            if(!pending)break;
            // The real queued reads remain owned and serviced here. Windows
            // messages must also be dispatched during this startup wait so
            // moving/closing the already shown HWND is not deferred until
            // every resource and texture transfer finishes.
            if(!host.PollEvents())throw std::runtime_error("Application was closed during resource loading");
            texturePackRegistry.ServiceReads();loader.ServiceResourceLoading();
            if(audioOpened)AudioResult(host.PumpAudio(sound),"Physical audio during resource load");
            std::this_thread::yield();
        }
        // No pending read remains; preserve the original retained-failure
        // checks without entering another wait that starves the message pump.
        for(auto* pack:texturePackRegistry.packs)pack->WaitForReads();
    }
    void RegisterScreen(FlailerScreenConfig& configuration)
    {
        for(const auto& stored:screenStorage)
            Require(std::strcmp(stored->Node().configuration->name,configuration.name)!=0,"Duplicate source screen registration");
        auto owner=std::make_unique<FlailerScreenStorage>(ui,configuration);
        auto& node=owner->Node();node.previous=screens.root.previous;node.next=&screens.root;
        screens.root.previous->next=&node;screens.root.previous=&node;screenStorage.push_back(std::move(owner));
    }
    void BindMenus()
    {

        screens.ui=&ui;screens.root.previous=screens.root.next=&screens.root;
        help=std::make_unique<FlailerHelpMenu>(ui);screens.help=help.get();
        background=std::make_unique<FlailerGlobalBackgroundAnimation>(ui);screens.background=background.get();
        for(size_t i=0;i<popups.size();++i) {
            popups[i]=std::make_unique<FlailerPopup>(ui);screens.popups[i]=popups[i].get();
        }
        frontendInput={&input,&screens};screens.input=&frontendInput;screens.soundBindings=&soundBindings;
        demoMode.screens=&screens;demoMode.frontend=&profilesOwner;demoMode.raceSetup=&setup;
        demoMode.demoDisc=&demo;demoMode.files=&files;
        menus={&ui,&screens,&profilesOwner,&setup,&soundBindings,&demoMode};
        startup={&menus,&joylog,dynamicTextures.get()};
        menusBound=true;
        BindFrontendStartup(startup);
        BindCreditsScreens(startup);
        const auto nativeDirectory=profileDirectory.u8string();
        const auto legacyDirectory=legacyProfileDirectory.u8string();
        memcards=std::make_unique<MemcardScreens>(profilesOwner,nativeDirectory.empty()?nullptr:nativeDirectory.c_str(),
            legacyDirectory.empty()?nullptr:legacyDirectory.c_str());
        BindMemcardScreens(*memcards,menus);
        trackSelection.ui=&ui;trackSelection.screens=&screens;trackSelection.frontend=&profilesOwner;
        trackSelection.tracks=tracks.get();trackSelection.resources=&files;trackSelection.raceSetup=&setup;
        trackSelection.cameras=&cameras;trackSelection.frame=&frontendFrame;BindTrackSelectScreen(trackSelection);
        carSelection={&menus,&cameras,&resources->Registry(),&carNaming,&positionMarkers,nullptr,&frontendFrame,&resources->FX()};
        resources->FX().BindGameFlow(flow.currentState);
        BindSelectCarScreen(carSelection);
        BindCampaignScreen(carSelection);
        FlailerScreenConfig* configurations[]={&fNewMainMenuScreen,&fCopsOnModeSelectScreen,
            &fCopsOffModeSelecttScreen,&fOptions,&fNameEntry,&fTitleScreen,&fMumboJumboScreen,&TrackSelectScreenConfiguration(),
            &fPreRaceBlurbScreen,&fPreRace2PBeACopScreen,&fPreRaceStatsScreen,&fPreRaceLetterboxScreen,
            &fPauseMenuScreen,&fPauseDisplayScreen,&fPauseOptionsScreen,&fPauseControlsScreen,&fPauseAudioScreen,
            &fPauseRaceObjectiveScreen,&fPauseRaceObjective2PScreen,&fPauseDebugMenu,&CampaignScreenConfiguration()};
        for(auto* configuration:configurations)RegisterScreen(*configuration);
        for(auto& configuration:EndGameScreenConfigurations())RegisterScreen(configuration);
        for(auto& configuration:MemcardScreenConfigurations())RegisterScreen(configuration);
        for(auto& configuration:LanguageScreenConfigurations())RegisterScreen(configuration);
        for(auto& configuration:CreditsScreenConfigurations())RegisterScreen(configuration);
        for(auto& configuration:AudioDisplayScreenConfigurations())RegisterScreen(configuration);
        for(auto& configuration:CustomizeHUDScreenConfigurations())RegisterScreen(configuration);
        for(auto& configuration:DisplayCalibrationScreenConfigurations())RegisterScreen(configuration);
        for(auto& configuration:RaceSetupScreenConfigurations())RegisterScreen(configuration);
        for(auto& configuration:JoystickScreenConfigurations())RegisterScreen(configuration);
        for(auto& configuration:SelectCarScreenConfigurations())RegisterScreen(configuration);
        screens.mainScreen=fNewMainMenuScreen.name;
        loadingAnimation=std::make_unique<FlailerLoadingScreenAnimation>(ui,flow,profilesOwner,demo,0xd0131dd1u);
        screens.loadingAnimation=loadingAnimation.get();menusBound=true;
    }
    WorldSimulation* Simulation() noexcept
    {
        auto* simulation=trackPhase?trackPhase->Simulation():nullptr;
        return simulation && simulation->world?simulation:nullptr;
    }
    GameSession* Session() noexcept {return trackPhase?trackPhase->Session():nullptr;}
    void ServiceFlow()
    {
        // FRONTB ResourceFile owns its bank; each field here is a borrow.
        setup.events=profilesOwner.boundEventResources;trackSelection.events=profilesOwner.boundEventResources;
        // Screen registration precedes FRONTB loading. Bind the main-menu
        // callbacks only after that real event owner is published, before
        // ServiceGameFlow can activate the loaded frontend. The binding
        // keeps process-lifetime services; subsequent banks refresh the
        // borrowed pointers above when the frontend is loaded again.
        if(setup.events && !mainMenuBound) {
            BindMainMenuScreen(menus);mainMenuBound=true;
        }
        if(showFrontend && flow.currentState==3 && flow.requestedState==2 &&
            (!supportsFrontendTrack(parameters.selectedScenario) || profilesOwner.playerCount!=1))
            throw NotImplemented("PAL185528", ("Godot race handoff for track "+
                std::to_string(parameters.selectedScenario)+", players="+std::to_string(profilesOwner.playerCount)).c_str());
        const auto result=ServiceGameFlow(flow,flowBindings);
        if(result.status!=GameFlowServiceStatus::Complete &&
           result.status!=GameFlowServiceStatus::PendingRenderCompletion)
            throw std::runtime_error("Original GameFlow failed at "+std::to_string(result.sourceFunction)+
                ", state="+std::to_string(result.state));
        setup.events=profilesOwner.boundEventResources;trackSelection.events=profilesOwner.boundEventResources;
        if(stateOnly() && !showFrontend && !quickRaceRequested && mainMenuBound &&
            flow.currentState==3 && flow.requestedState==3) {
            StartQuickRace(menus,0,requestedTrack);
            quickRaceRequested=true;
        }
    }
    void GenerateInput(ReplayManager* replay)
    {
        Require(joystickGenerateEvents(input,samples,joylog,replay,clock,demo)==GAME_INPUT_OK,
            "Source joystick scanner/typed callback dispatch failed");
    }
};

ApplicationRuntime::ApplicationRuntime(GameResources& files,NativeApplicationHost& host,
    const std::filesystem::path& profileDirectory,const std::filesystem::path& legacyProfileDirectory)
    :storage_(std::make_unique<Storage>(files,host,profileDirectory,legacyProfileDirectory))
{
    Require(!applicationHost,"Only one source application process may own input");applicationHost=&host;
}
ApplicationRuntime::~ApplicationRuntime()
{
    (void)ShutdownNoThrow();
    if(applicationHost==&storage_->host)applicationHost=nullptr;
}
EcstasyFrame& ApplicationRuntime::Frame() noexcept {return *storage_->frame;}
bSound& ApplicationRuntime::Audio() noexcept {return storage_->sound;}
const GameTime& ApplicationRuntime::Clock() const noexcept {return storage_->clock;}
const FrontendPlayerManager& ApplicationRuntime::Frontend() const {return storage_->profilesOwner;}
const ApplicationFrontendFlow& ApplicationRuntime::Flow() const noexcept {return storage_->flow;}
void ApplicationRuntime::SetDisplayOptions(int32_t width,int32_t height,bool fullscreen,bool vsync)
{
    auto& s=*storage_;Require(s.initialized,"Display settings precede profile initialization");
    Require(width>0 && height>0,"Display extent must be positive");
    s.host.ApplyDisplaySettings(width,height,fullscreen,vsync);
    s.profilesOwner.nativeDisplay={width,height,fullscreen,vsync};
    s.launchDisplay=s.profilesOwner.nativeDisplay;s.RefreshNativeDisplayGeometry();
}

void ApplicationRuntime::Initialize(const std::filesystem::path& configFile)
{
    auto& s=*storage_;
    Require(!s.shutdownStarted && !s.initialized && !s.resources,"Application initialization cannot replace live or retired owners");
    Require(s.host.Width()>0 && s.host.Height()>0,"Application has no actual display extent");
    // InitializeEverything12EE98. Native allocation/thread/window setup is
    // supplied by host. All game services retain their actual dependencies.
    s.demo.Init(s.clock,applicationLaunchOptions,DemoDiscConfiguration{});
    s.demoReplay.BindSessionHost(&s.host);
    applicationSeedRandomNumber(s.joylog);
    applicationLoadConfig(configFile.empty()?s.profileDirectory/"config.ini":configFile,s.joylog);
    if(s.stateOnly() && !s.showFrontend) {
        applicationLaunchOptions.skipMovies=1;
        s.skipStartSequence=1;
    }
    eMathInit();
    InitStomper();
    InitDisplaySystem(s.files,s.clock);
    for(auto& view:s.cameras.screens)ecstasyViewInitialize(&view);
    for(auto& view:s.views)carCinematicViewInitialize(&s.cameras,&view);
    for(size_t i=0;i<s.targets.size();++i) {
        auto& target=s.targets[i];target.id=static_cast<int32_t>(i);target.enabled=1;
        target.width=s.host.Width();target.height=s.host.Height();
        target.horizontalScale=target.verticalScale=target.fieldOfViewScale=1.0f;
        if(i>=8) {
            // PAL SetScreenBuffers: six 64-square faces and the mapped
            // 256-square environment image retain their authored extents.
            target.width=target.height=i==14?256:64;target.format=0x32;
            target.opaqueOnly=i==14?0:1;
        }
    }
    ecstasyViewsBind(&s.cameras,s.views.data(),s.targets.data());
    if(s.requestedTrack<0)s.environmentMap=std::make_unique<EnvironmentMap>(s.cameras,s.targets.data(),s.targets.size());
    ecstasyViewProjectionInitialize(&s.cameras.projection,s.host.Width());
    s.cameras.projection.nativeAspectCorrection=1;
    s.cameras.projection.nativeSourceVideoMode=gameTimeGetVideoMode(&s.clock);
    s.resources=std::make_unique<ApplicationResourceRuntime>(s.files,s.cameras,s.joylog,s.demo);
    auto& loader=s.resources->Loader();auto& registry=s.resources->Registry();
    gameInputRouterInitialize(&s.input,s.registrations.data(),s.registrations.size());
    joystickInitialize(&s.samples,&ReadApplicationInput);s.inputBound=true;
    Require(joystickInitializeTypeMonitoring(&s.input)==GAME_INPUT_OK,"Joystick monitor registration failed");
    easterEggs.input=&s.input;realDriverTuningInitialize(&s.driverTuning);positionMarkersInitialize(&s.positionMarkers);
    // Final LoadGlobalChunks1682D0 paths and flags. Missing files remain
    // physical failures; no generated bundle or alternate file is injected.
    loader.LoadResourceFile("GLOBAL/GLOBALA.BUN",0,6,s.resources->GlobalFileBindings());
    s.global=loader.LoadResourceFile("GLOBAL/GLOBALB.LZC",0,9,s.resources->GlobalFileBindings());
    s.global->ChangeFilenameForHotChunking("GLOBAL/GLOBALB.BUN");s.DrainFiles();
    const auto& global=s.global->Resources();
    Require(global.cars && global.track && global.characters && global.driverBehavior,
        "GLOBALB did not publish its original catalogues");
    s.tracks=std::make_unique<TrackInfoResources>(*global.track);
    s.language.Init(s.files);s.languageInitialized=true;
    s.dynamicTextures=std::make_unique<TexturePack>(texturePackRegistry,s.files,registry.Models(),"DynTextures","GLOBAL/DYNTEX.BIN");
    Require(s.dynamicTextures->LoadTextureTableFromFile("GLOBAL/DYNTEX.BIN"),"Missing final resource GLOBAL/DYNTEX.BIN");
    s.hudTextures=std::make_unique<TexturePack>(texturePackRegistry,s.files,registry.Models(),"HudTextures","GLOBAL/HUDTEX.BIN");
    Require(s.hudTextures->LoadTextureTableFromFile("GLOBAL/HUDTEX.BIN"),"Missing final resource GLOBAL/HUDTEX.BIN");
    s.carTextures=std::make_unique<TexturePack>(texturePackRegistry,s.files,registry.Models(),"CarTextures","CARS/TEXTURES.BIN");
    Require(s.carTextures->LoadTextureTableFromFile("CARS/TEXTURES.BIN"),"Missing final resource CARS/TEXTURES.BIN");
    worldAudioInitialize(&s.audioQueue,nullptr,0,s.audioRequests.data(),s.audioRequests.size());
    s.AudioResult(s.sound.Bind(s.audioQueue,s.files,carSoundGlobals.policy.audioSystemEnabled),"bSound binding");s.audioBound=true;
    s.soundBindings.audio=&s.audioQueue;s.soundBindings.globals=&carSoundGlobals;s.soundBindings.engineGlobals=&carEngineSoundGlobals;
    s.soundBindings.cars=global.cars;s.soundBindings.clock=&s.clock;s.soundBindings.files=&s.files;
    s.soundBindings.manager=&s.sound.Mixer();s.soundBindings.volumes=&s.volumes;
    s.soundBindings.spatial={nullptr,&s.cameras,&s.clock,&carSoundGlobals.speedOfSound,&carSoundGlobals.multiplayerAzimuth};
    s.soundBindings.physicalProgressOwner=&s;
    s.soundBindings.servicePhysicalAudio=[](void* context)->WorldAudioResult {
        auto& owner=*static_cast<Storage*>(context);
        if(!owner.audioOpened)return WORLD_AUDIO_INVALID_BINDING;
        if(!owner.host.PollEvents())return WORLD_AUDIO_BACKEND_FAILURE;
        return owner.host.PumpAudio(owner.sound);
    };
    s.AudioResult(s.sound.BindSoundClient(&s.soundBindings),"Sound source callback binding");
    s.uiSound.audio=&s.audioQueue;s.uiSound.enabled=&carSoundGlobals.policy.audioSystemEnabled;
    s.uiSound.clock=&s.soundClock;s.uiSound.manager=&s.sound.Mixer();s.uiSound.volumes=&s.volumes;
    worldAudioMessagesInitialize(&s.speechMessages);
    Require(worldStreamAudioBind(s.speechStream,s.speechMessages,s.sound,s.files,s.clock,carSoundGlobals,soundDriverState)==WORLD_STREAM_AUDIO_OK,
        "Original SED/speech binding failed");s.speechBound=true;
    auto* speech=worldSpeechGetInstance();Require(speech && speech->noise,"Original speech owner allocation failed");
    speech->messages=&s.speechMessages;speech->streamedAudio=&s.speechStream;
    s.AudioResult(speech->noise->Bind(&s.audioQueue,&worldAudioRuntime),"Speech noise binding");s.speechNoiseBound=true;
    if(carSoundGlobals.policy.audioSystemEnabled) {
        s.AudioResult(s.sound.Initialize(),"Sound driver initialization");
        s.AudioResult(s.host.OpenAudio(s.sound),"Physical Windows audio device");s.audioOpened=true;
        s.AudioResult(s.sound.SetPath("sound\\"),"Source audio bank path");
        s.AudioResult(soundLoadRegion(s.soundBindings,"common"),"Source common sound bank");
        while(!soundDriverState.commonRegionLoaded) {
            s.AudioResult(s.host.PumpAudio(s.sound),"Physical common bank read");
            s.AudioResult(s.sound.Update(),"Common bank source completion");
            if(!s.host.PollEvents())throw std::runtime_error("Application was closed during sound initialization");
            std::this_thread::yield();
        }
        s.AudioResult(soundJukeboxPlayer.Bind(s.sound,s.files),"Original jukebox binding");s.jukeboxBound=true;
        if(applicationLaunchOptions.musicEnabled)
            s.AudioResult(soundJukeboxPlayer.LoadCartridge(s.demo.ActivationWord()?
                "SOUND/JukeboxMDemoDisk.JBC":"SOUND/JukeboxM.JBC",false),"Original music cartridge");
        SetVolumeDefaults(s.uiSound,s.demo);
        s.AudioResult(s.host.PumpAudio(s.sound),"Physical mixer initialization");
    }
    s.language.Bind({s.profilesOwner,s.flow,*s.dynamicTextures,loader,registry.Flailer(),s.speechStream,
        &s,[](void* context) {
            auto& owner=*static_cast<Storage*>(context);
            Require(owner.audioOpened,"Localized speech retirement requires its physical audio device");
            owner.AudioResult(owner.host.PumpAudio(owner.sound),"Physical localized speech retirement");
            // Pump consumes the retained STOP itself. Do not run another
            // source Update/region completion while inside a flow teardown.
            std::this_thread::yield();
        }});
    // Final LoadLanguageInfo100808 tail100818 calls SetLanguage(0).
    s.language.SetLanguage(0);
    // PAL LoadGlobalChunks1683DC..16847C loads the initial dynamic image
    // after language selection, then services its real queued pixel reads.
    // English language0 itself does not request this texture.
    const uint32_t initialTexture=s.demo.ActivationWord() ? hashString("FIRSTLEGAL"):0xab0e817du;
    if(s.demo.ActivationWord() || !s.dynamicTextures->GetLoadedTexture(initialTexture))
        s.dynamicTextures->LoadTexturesFromFile(&initialTexture,1);
    s.DrainFiles();
    s.highScores=std::make_unique<HighScoreTable>(*s.tracks,s.profilesOwner);s.profilesOwner.highScores=s.highScores.get();
    for(auto& profile:s.profiles){profile.cars=global.cars;profile.language=&s.language;}
    s.setup.parameters=&s.parameters;s.setup.cars=global.cars;s.setup.language=&s.language;
    s.setup.tracks=s.tracks.get();s.setup.characters=&global.characters->Catalogue();
    s.setup.behavior=&global.driverBehavior->Settings();s.setup.applicationOptions=&applicationLaunchOptions;
    s.setup.demoDisc=&applicationLaunchOptions.bVersionE3;
    s.setup.currentEvent=&s.event;s.setup.runtimeSettings=&s.runtime;s.setup.demoReplay=&s.demoReplay;s.setup.flow=&s.flow;
    s.profilesOwner.InitWithDefaults(s.setup);
    InitializeDisplayCalibration(s.profilesOwner,s.clock);
    if(carSoundGlobals.policy.audioSystemEnabled)s.profilesOwner.CaptureAudioOptions(s.soundBindings,true);
    s.ui.resources=&registry.Flailer();s.ui.sound=&s.uiSound;s.ui.render=&s.renderer;
    s.ui.language=&s.language;s.ui.clock=&s.clock;s.ui.flow=&s.flow;
    // The native backend uses the original CPU RGBA branch. The decoder
    // and Flailer consumer must borrow the same source flag, not separate
    // zero/one defaults that publish pixels but suppress their bitmap.
    s.renderer.bitmapComposite.enabled=0;
    s.movie=std::make_unique<MoviePlayer>(s.files,s.resources->FileIo(),registry,s.clock,
        carSoundGlobals.policy.audioSystemEnabled,s.renderer.bitmapComposite.enabled,&s.audioQueue);s.ui.movie=s.movie.get();
    s.renderer.bitmapComposite.movie=s.movie.get();
    if(s.stateOnly()) {
        s.movie->UseExternalPlayback();
        s.ui.externalPresentation=true;
    }
    // PAL InitFlailerMenuSystem1696C0 calls InitGlobalMenuSound16DED8
    // before the screen-list constructor; Common is now fully published.
    InitGlobalMenuSound(s.uiSound,s.soundBindings);
    s.BindMenus();
    if(s.showFrontend)s.menus.quickRaceTrack=s.requestedTrack;
    s.frontendPhase=std::make_unique<GameFlowFrontendResources>(loader,s.startup,applicationLaunchOptions);
    ResourceFileBindings trackBinding;trackBinding.parameters=&s.parameters;trackBinding.sound=&s.soundBindings;
    trackBinding.cameras=&s.resources->CameraResources();trackBinding.particles=&s.resources->FX().Particles();
    s.trackPhase=std::make_unique<GameFlowTrackResources>(loader,*global.cars,*s.hudTextures,trackBinding);
    s.impactEffects={&s.audioQueue,&worldAudioRuntime.randomState,&carSoundGlobals.policy.audioSystemEnabled};
    s.AudioResult(worldImpactAudioBind(&s.impactAudio,&s.impactEffects),"World impact audio binding");
    s.replayAvailableBytes=s.host.AvailableMemory();
    GameFlowTrackProcessBindings process{s.resources.get(),s.global,&s.clock,&applicationLaunchOptions,&s.input,
        &s.cameras,&s.carNaming,&s.positionMarkers,&s.driverTuning,&s.positionalSamples,&s.impactAudio,
        &s.skipStartSequence,&s.alternateSkin,&s.replayAvailableBytes,
        &s.impactPresentationMode,&s.resources->Registry().Sun().Glare().start};
    s.trackPhase->BindProcessResources(process);s.soundBindings.spatial.players=s.trackPhase->Players();
    s.AudioResult(s.positionalSamples.Bind(&s.audioQueue,s.soundBindings.spatial,&carSoundGlobals.policy.audioSystemEnabled),
        "Shared positional audio binding");
    s.AudioResult(soundScapeBindServices(s.soundBindings),"Process soundscape binding");s.soundScapeBound=true;
    s.flowBindings={&s.clock,&s.demo,s.dynamicTextures.get(),&s.ui,s.frontendPhase.get(),s.trackPhase.get(),s.frame.get()};
    carLoader.Bind(loader,*global.cars,*s.carTextures,s.language,s.carNaming,s.positionMarkers,nullptr);
    Require(!applicationLaunchOptions.skipFrontend,"StartSkipFEGame requires its original FRED producer");
    ChangeGameFlow(s.flow,1,0);s.initialized=true;
}

void ApplicationRuntime::InitializeRace(int track)
{
    Require(track>=0 && storage_->requestedTrack<0 && !storage_->initialized,
        "Quick Race must be requested before initialization");
    storage_->requestedTrack=track;
    Initialize();
}

void ApplicationRuntime::InitializeFrontend(int quickRaceTrack, bool (*supportsTrack)(int))
{
    Require(quickRaceTrack>=0 && supportsTrack && supportsTrack(quickRaceTrack) &&
        !storage_->stateOnly() && !storage_->initialized,
        "Frontend must be requested before initialization");
    storage_->requestedTrack=quickRaceTrack;
    storage_->supportsFrontendTrack=supportsTrack;
    storage_->showFrontend=true;
    Initialize();
}

FrontendStateView ApplicationRuntime::FrontendState() const noexcept
{
    auto& s=*storage_;
    if(!s.stateOnly() || !s.initialized)return {};
    const auto* active=s.screens.active;
    return {&s.renderer,s.movie.get(),&s.frontendFrame,&s.cameras,
        active && active->configuration ? active->configuration->name:nullptr,
        active ? active->GetActiveMenuItem():0};
}

int ApplicationRuntime::SelectedTrack() const noexcept {return storage_->parameters.selectedScenario;}

RaceStateView ApplicationRuntime::RaceState() const noexcept
{
    auto& s=*storage_;
    if(s.requestedTrack<0 || !s.flow.inGameFlag || !s.trackPhase || !s.trackPhase->trackFile)return {};
    const auto* simulation=s.Simulation();
    const auto* session=s.Session();
    const auto* view=s.cameras.screens[1].view;
    auto* scenery=s.trackPhase->trackFile->Resources().scenery;
    return {simulation,scenery?&scenery->Native():nullptr,view?&view->current:nullptr,
        view?view->hiddenCar:nullptr,s.trackPhase->Sky(),s.clock.worldTimeFrames,
        s.parameters.selectedScenario,session?session->state:0,gameTimeGetVideoMode(&s.clock)==HP2_VIDEO_NTSC};
}

bool ApplicationRuntime::Tick()
{
    Require(storage_->requestedTrack<0,"Native presentation is disabled for this race");
    return TickInternal();
}

bool ApplicationRuntime::AdvanceRace()
{
    Require(storage_->requestedTrack>=0,"No explicit Quick Race was requested");
    return TickInternal();
}

bool ApplicationRuntime::TickInternal()
{
    auto& s=*storage_;Require(s.initialized,"Application frame precedes initialization");
    const auto tickStarted=s.host.FrameTime();
    if(!s.frameTimingStarted) {
        s.frameTiming.Reset(tickStarted);s.simulationClock.Reset(tickStarted);
        applicationFrameTiming={};s.frameTimingStarted=true;
        s.presentationTimingEpoch=s.host.PresentationTimingEpoch();s.renderTimingWindowStart=tickStarted;
    }
    if(!s.host.PollEvents() || s.demo.ExitRequested() || applicationDebugExitRequested)return false;
    if(s.presentationTimingEpoch!=s.host.PresentationTimingEpoch()) {
        s.presentationTimingEpoch=s.host.PresentationTimingEpoch();
        const auto resumed=s.host.FrameTime();
        s.frameTiming.Reset(resumed);s.simulationClock.Reset(resumed);applicationFrameTiming={};
    }
    auto observed=s.host.FrameTime();
    if(!s.simulationClock.UpdateDue(observed) &&
        (s.requestedTrack>=0 || !s.host.PresentationDue(observed))) {
        s.host.WaitUntil(std::min(s.simulationClock.Deadline(),s.host.PresentationDeadline()));
        return true;
    }
    const auto sourceBefore=s.sourceUpdateTiming.totalNanoseconds;
    ScopedNativeDuration sourceMeasure(s.sourceUpdateTiming);
    bool advancedPresentation=false;
    std::uint32_t updates=0;
    for(;;) {
        advancedPresentation=s.simulationClock.UpdateDue(observed);
        if(advancedPresentation) {
            if(s.stateOnly()) {
                s.renderer.CompleteSubmissions();
                s.frontendFrame.Clear();
            }
            // The previous logical frame stayed unchanged while additional
            // presentations borrowed it. Retire its post-draw mutations once.
            if(s.pendingClockAdvance) {
                Require(worldParticlesAdvance(&s.resources->FX().FrameParticles(),s.clock.realTimeElapsed)==WORLD_SERVICES_OK,
                    "FrameEffects lost its logical update owner");
                texturePackRegistry.UpdateAnimations(s.resources->Registry().Models(),s.clock.worldTimeFrames,s.clock.realTimeFrames);
                if(s.pendingLegacyRealClock)gameTimeAdvanceRealTime(&s.clock);
                else gameTimeAdvanceNativeRealTime(&s.clock);
                gameTimeAdvanceNativeWorldTime(&s.clock);
                s.pendingClockAdvance=false;
            }
            if(s.memcards)s.memcards->Service();
            if(s.launchDisplay)s.profilesOwner.nativeDisplay=*s.launchDisplay;
            const auto& display=s.profilesOwner.nativeDisplay;
            s.host.ApplyDisplaySettings(display.width,display.height,display.fullscreen,display.vsync);
            s.RefreshNativeDisplayGeometry();
            if(s.language.currentLanguage!=s.profilesOwner.currentLanguage)s.language.SetLanguage(s.profilesOwner.currentLanguage);
            const auto* previousSimulation=s.Simulation();
            const bool previouslyInGame=s.flow.inGameFlag!=0;
            s.ServiceFlow();
            if(auto* session=s.Session()) {
                Require(raceCoordinatorExecuteQueuedMessages(session)!=0,"RaceCoordinator queued-message dispatch failed");
                (void)raceCoordinatorCheckForDemoDiscTimeout(session);
                Require(session->result==RC_RESULT_OK,"RaceCoordinator demo timeout failed");
                Require(raceCoordinatorSendMessage(session,3,0)!=0,"RaceCoordinator frame message failed");
            }
            s.ServiceFlow();
            auto* simulation=s.Simulation();auto* session=s.Session();
            if(simulation!=previousSimulation || previouslyInGame!=(s.flow.inGameFlag!=0)) {
                // Loading/retirement is an explicit lifecycle boundary. Its
                // elapsed service time must not become catch-up movement in
                // a newly created race or skip the first visible countdown.
                observed=s.host.FrameTime();s.simulationClock.Reset(observed);
                s.replayStepSeconds=1.0f/60.0f;
            }
            s.RecordLifecycle(simulation,session);
            // PAL12F40C gates the application checksum on the live InGameFlag.
            // World owns the in-race checksum; do not duplicate it or consume a
            // second Joylog channel3 record during the same gameplay frame.
            if(!s.flow.inGameFlag)
                applicationVerifyJoylogChecksum(s.checksum,s.joylog,simulation,&s.screens,s.flow);
            auto* players=s.trackPhase->Players();carFramePlayerDetermineAllInputMaps(players);
            auto* replay=session && session->bindings && session->bindings->replayOwner?&session->bindings->replayOwner->State():nullptr;
            if(replay)replay->owner->DoFancyJoylogDebugging(s.clock,s.joylog);
            const bool restoringFrame=replay && replay->owner->WillRestoreFrame(s.clock);
            const auto elapsed=restoringFrame ? s.replayStepSeconds:1.0f/60.0f;
            const bool sourceJoylog=s.joylog.stream || s.joylog.replaying || s.joylog.recording;
            if(sourceJoylog) {
                // The original Joylog has no native clock-version marker.
                // Keep both of its clock domains on the recorded contract.
                s.clock.nativeTimingEnabled=0;s.clock.worldTimerPolicy=GAME_TIME_TIMER_LEGACY;
            } else if(s.sourceJoylogTiming) {
                const auto worldPolicy=s.clock.worldTimerPolicy;
                gameTimeUseNativeTiming(&s.clock);
                if(restoringFrame)s.clock.worldTimerPolicy=worldPolicy;
            }
            s.sourceJoylogTiming=sourceJoylog;s.pendingLegacyRealClock=sourceJoylog;
            Require((s.pendingLegacyRealClock ? gameTimePrepareRealTimestep(&s.clock,elapsed,&s.joylog):
                gameTimePrepareNativeRealTimestep(&s.clock,elapsed,&s.joylog))!=0,"Native real timestep preparation failed");
            s.scheduledStepQuanta=s.clock.realTimeQuantized>0 ? static_cast<std::uint32_t>(s.clock.realTimeQuantized):60u;
            if(!restoringFrame)s.replayStepSeconds=1.0f/60.0f;
            if(IsGameFlowPaused(session)) {gameTimeSetWorldElapsed(&s.clock,0.0f);s.GenerateInput(replay);}
            else if(!session) {gameTimeSetWorldElapsed(&s.clock,s.clock.realTimeElapsed);s.GenerateInput(nullptr);}
            else {
                Require(simulation && replay && session->bindings && session->bindings->startScreens,
                    "Live coordinator has no canonical World/Replay/screen graph");
                worldUpdatePause(&simulation->world->timing,s.clock.realTimeElapsed);
                if(worldIsPaused(&simulation->world->timing)) {gameTimeSetWorldElapsed(&s.clock,0.0f);s.GenerateInput(replay);}
                else {
                    replay->owner->BeginFrame(s.clock,session->bindings->startScreens->services);
                    if(replay->replayPausedThisFrame) {gameTimeSetWorldElapsed(&s.clock,0.0f);s.GenerateInput(replay);}
                    else {
                        WorldStepOptions options{replay->recording,replay->restoring,0,0,0};
                        WorldStepIO io{replay,
                            [](void* context,int bits,int channel,int32_t* value) {
                                auto& source=*static_cast<ReplayManager*>(context);
                                *value=static_cast<int32_t>(replayManagerRead(&source,static_cast<uint32_t>(bits),static_cast<uint32_t>(channel)));
                                return source.stream && source.stream->result==BIT_STREAM_OK?1:0;},
                            [](void* context,int32_t value,int bits,int channel) {
                                replayManagerWrite(static_cast<ReplayManager*>(context),static_cast<uint32_t>(value),
                                    static_cast<uint32_t>(bits),static_cast<uint32_t>(channel));},
                            [](void*){return hp2NativeTickerSource();},
                            [](void*,uint32_t from,uint32_t to){return tickerMilliseconds(from,to);}};
                        float worldElapsed{},recordedElapsed{};
                        Require(worldGetNativeTimestep(&simulation->world->timing,&s.clock,&options,&io,&worldElapsed,&recordedElapsed)==WORLD_STEP_OK,
                            "Source World timestep/replay stream failed");
                        if(options.replaying && recordedElapsed>0.0f) {
                            s.replayStepSeconds=recordedElapsed;
                            // This is the file's unscaled timestep. Changing the
                            // presentation rate cannot consume its frames faster.
                            s.scheduledStepQuanta=static_cast<std::uint32_t>(gameTimeQuantizeElapsed(recordedElapsed));
                        }
                        gameTimeSetWorldElapsed(&s.clock,worldElapsed);s.GenerateInput(replay);
                        const auto stepResult=worldDoTimestep(simulation,worldElapsed);
                        if(stepResult!=WORLD_SIMULATION_OK) {
                            // Report actual owner results without replacing a failure
                            // with a successful step or attributing a global collision
                            // failure to the last Car visited by the source loops.
                            auto detail=std::string("Original World timestep failed (world=")+
                                std::to_string(static_cast<int>(stepResult));
                            if(const auto* field=simulation->stateFailure.field) {
                                // Read the same failed owner once, before lengthy service
                                // counters. Do not recompute derived state on an error path.
                                const auto index=simulation->stateFailure.car;
                                detail+=", state_phase="+std::string(stepResult==WORLD_SIMULATION_REPLAY_FAILURE?
                                    "before_replay":"after_step");
                                detail+=", state_field="+std::string(field);
                                detail+=", state_element="+std::to_string(simulation->stateFailure.element);
                                detail+=", state_car="+std::to_string(index);
                                detail+=", loop="+std::to_string(s.clock.worldLoopCounter);
                                if(index>=0 && index<simulation->world->totalCars)
                                    if(const auto* car=simulation->world->carTable[index]) {
                                        detail+=", state_driver_number="+std::to_string(car->driverNumber);
                                        if(static_cast<std::size_t>(index)<simulation->frameCount && simulation->drivers)
                                            if(const auto* driver=simulation->drivers[index])
                                                detail+=", state_driver_kind="+std::to_string(static_cast<int>(driver->kind));
                                    }
                            }
                            if(simulation->collisions) {
                                detail+=", collision="+std::to_string(static_cast<int>(simulation->collisions->result));
                                if(const auto* objects=simulation->collisions->objects) {
                                    detail+=", objects_occupied="+std::to_string(objects->occupiedObjects);
                                    detail+=", objects_capacity="+std::to_string(objects->totalObjectCapacity);
                                    detail+=", objects_simulating="+std::to_string(objects->simulatingCount);
                                }
                                if(const auto* impact=simulation->collisions->contactAudio) {
                                    detail+=", impact_audio_registered="+std::to_string(impact->registered);
                                    detail+=", impact_audio_enabled="+std::to_string(
                                        impact->audio && impact->audio->enabled?*impact->audio->enabled:-1);
                                }
                            }
                            if(stepResult==WORLD_SIMULATION_DRIVER_FAILURE || stepResult==WORLD_SIMULATION_MOVEMENT_FAILURE ||
                                stepResult==WORLD_SIMULATION_FRAME_FAILURE) {
                                const auto index=simulation->failedCar;
                                detail+=", car="+std::to_string(index);
                                if(index>=0 && static_cast<std::size_t>(index)<simulation->frameCount && simulation->drivers)
                                    if(const auto* driver=simulation->drivers[index]) {
                                        detail+=", driver_kind="+std::to_string(static_cast<int>(driver->kind));
                                        detail+=", driver_result="+std::to_string(static_cast<int>(driver->result));
                                        const DriverAIBase* base=nullptr;
                                        const AIControls* input=nullptr;
                                        int ownerResult=-1;
                                        if(driver->kind==CAR_DRIVER_AI && driver->owner.ai) {
                                            base=&driver->owner.ai->base;input=&driver->owner.ai->input;
                                            ownerResult=driver->owner.ai->result;
                                        } else if(driver->kind==CAR_DRIVER_TRAFFIC && driver->owner.traffic) {
                                            base=&driver->owner.traffic->base;input=&driver->owner.traffic->input;
                                            ownerResult=driver->owner.traffic->result;
                                        }
                                        if(base) {
                                            // Observe the failing owners directly. Do not refresh a
                                            // route, invoke a controller, or mutate the recorded fault.
                                            detail+=", driver_owner_result="+std::to_string(ownerResult);
                                            detail+=", driver_base_result="+std::to_string(static_cast<int>(base->result));
                                            detail+=", driver_input_result="+std::to_string(input?static_cast<int>(input->result):-1);
                                            detail+=", driver_action_kind="+std::to_string(base->action?base->action->kind:-1);
                                            if(base->currentRoad) {
                                                detail+=", driver_road_group="+std::to_string(base->currentRoad->group?base->currentRoad->group->id:-1);
                                                detail+=", driver_pose_group="+std::to_string(base->currentRoad->recovery.recoveryPose?
                                                    base->currentRoad->recovery.recoveryPose->groupId:-1);
                                            }
                                        }
                                    }
                                if(index>=0 && static_cast<std::size_t>(index)<simulation->frameCount && simulation->frames) {
                                    const auto* failed=simulation->frames[index];
                                    if(failed) {
                                        detail+=", frame="+std::to_string(static_cast<int>(failed->result));
                                        if(failed->movement)detail+=", movement="+
                                            std::to_string(static_cast<int>(failed->movement->result));
                                        if(const auto* player=failed->environment.player) {
                                            detail+=", player_result="+std::to_string(player->result);
                                            detail+=", player_cinematic_state="+std::to_string(player->movement.steeringFeedbackBusy);
                                            if(const auto* registry=player->registry) {
                                                detail+=", driving_line_gate="+(registry->enableVisibleDrivingLine ?
                                                    std::to_string(*registry->enableVisibleDrivingLine):std::string("unbound"));
                                                if(registry->sessionState)detail+=", session_state="+
                                                    std::to_string(*registry->sessionState);
                                            }
                                        }
                                    }
                                }
                            }
                            detail+=")";
                            if(s.demoReplay.active)s.demoReplay.RecordSessionEvent("demo.world_error",nullptr,detail.c_str());
                            throw std::runtime_error(detail);
                        }
                    }
                    replayManagerEndFrame(replay,&s.clock);
                }
            }
            s.soundClock.ticks=static_cast<uint32_t>(s.clock.realTimer);
            MenuSystemTick(&s.screens);
            s.trackPhase->ServicePresentation(s.flow,s.profilesOwner);
            // DamageAbove normally publishes this operand during RenderCars.
            // The Godot host has no Ecstasy pass to service the HUD dependency.
            const auto* view=s.cameras.screens[1].view;
            if(s.stateOnly() && simulation && view && view->current.fieldOfView) {
                auto& world=*simulation->world;
                for(int32_t i=0;i<world.totalCars;++i) {
                    auto* car=world.carTable[i];
                    if(car)
                        car->PublishTrackerBearing(view->current.eye,view->current.forward);
                }
            }
            carFramePlayerUpdateNonGameStateAll(players);
            // PAL12F58C..12F5A8 updates the existing RC+2C ReplayScreen after the
            // Player HUD and before camera movers. Tick is a separate RC message.
            if(session)raceCoordinatorUpdateReplayScreen(*session);
            if(simulation)Require(carViewMoversAdvance(s.trackPhase->ViewMovers(),s.clock.realTimeElapsed)==CAR_VIEW_MOVER_OK,
                "Source camera coordinator update failed");
            // UpdateCameraMovers15EF28 advances the top concrete CameraMover of ten
            // views. C++ dispatch targets the actual owner, never a replacement pose.
            for(auto& screen:s.cameras.screens)if(screen.last)
                Require(carCinematicAdvanceOwned(screen.last)==CAR_CINEMATIC_OK,"Active camera mover update failed");
            s.AudioResult(soundUpdate(s.soundBindings,s.trackPhase->Simulation(),s.speechStream,s.profilesOwner,s.parameters,s.flow,&s.joylog),"Original sound frame");
            if(s.audioOpened)s.AudioResult(s.host.PumpAudio(s.sound),"Physical audio frame");
            joystickUpdatePortRumblers(s.input,s.clock);
            for(int32_t port=0;port<2;++port)if(s.input.pendingMotorDirty[port]) {
                s.host.SetRumble(port,s.input.pendingMotorBytes[port][0],s.input.pendingMotorBytes[port][1]);s.input.pendingMotorDirty[port]=0;
            }
            if(simulation)carCameraShakeAdvance(s.trackPhase->ViewMovers()->environment.shake,s.clock.realTimeElapsed);
        }
        if(s.requestedTrack>=0) {
            // Publish authored 2D layout and preview operands without building an Ecstasy frame.
            s.renderer.BeginFrame(*s.ui.resources,&s.language,
                static_cast<float>(s.clock.realTimer)*0.0002500000118743628f);
            s.screens.RenderModels(true);
            if(!s.flow.inGameFlag)CollectFrontendCarModels(s.frontendFrame);
            s.frontendFrame.modelDrawCpuClosureRequired=false;
            auto* players=s.trackPhase->Players();
            Require(players && players->count>=0 && players->count<=2,
                "Flailer track map player registry is invalid");
            for(int32_t index=0;index<players->count;++index)
                carFramePlayerRenderTrackMap(players->players[index]);
            s.pendingClockAdvance=true;
            s.simulationClock.CompleteStep(s.scheduledStepQuanta);
            texturePackRegistry.ServiceReads();
            s.resources->Loader().ServiceResourceLoading();
            s.replayAvailableBytes=s.host.AvailableMemory();
            if(++updates<8u && s.simulationClock.UpdateDue(observed))continue;
            return !s.demo.ExitRequested();
        }
        Require(s.frame->State()==EcstasyFrame::Phase::Complete || s.frame->Commands().empty(),"Previous CPU frame capture is still live");
        if(s.frame->State()==EcstasyFrame::Phase::Complete)s.frame->BeginNextFrame();
        else Require(s.frame->State()==EcstasyFrame::Phase::Building && s.frame->Commands().empty(),"Previous CPU frame capture is still live");
        s.flowBindings.renderFrame=s.frame.get();
        EcstasySceneBindings scene{s.cameras,s.ui,s.trackPhase.get(),&s.flow,&s.parameters,&s.flow.currentState,&s.clock,&s.frontendFrame,&s.screens};
        scene.environmentMap=s.environmentMap.get();scene.effects=&s.resources->FX();
        scene.advancePresentation=advancedPresentation;scene.deferPresentationUpdates=true;
        EcstasyDisplayFrame(scene,*s.frame);s.frame->Seal();
        if(advancedPresentation) {
            s.pendingClockAdvance=true;
            s.simulationClock.CompleteStep(s.scheduledStepQuanta);
            ++updates;
            // Catch up complete logical updates before one presentation. Bound
            // work per host call, but retain all outstanding time for later calls.
            if(updates<8u && s.simulationClock.UpdateDue(observed)) {
                s.frame->CancelBeforeSubmit();
                texturePackRegistry.ServiceReads();s.resources->Loader().ServiceResourceLoading();
                s.replayAvailableBytes=s.host.AvailableMemory();
                continue;
            }
        }
        break;
    }
    sourceMeasure.Finish();
    const auto capturedUpdateMs=static_cast<double>(s.sourceUpdateTiming.totalNanoseconds-sourceBefore)*1.0e-6;
    if(!s.host.PresentationDue(s.host.FrameTime())) {
        // A logical frame still executes the source capture/post-draw contract,
        // but a capped PC display does not submit it to the GPU. Retirement is
        // the same cancel path used for intermediate catch-up updates above.
        s.frame->CancelBeforeSubmit();
        s.deferredPresentationCpuMilliseconds+=capturedUpdateMs;
        if(advancedPresentation) {
            texturePackRegistry.ServiceReads();s.resources->Loader().ServiceResourceLoading();
            s.replayAvailableBytes=s.host.AvailableMemory();
        }
        s.host.WaitUntil(std::min(s.simulationClock.Deadline(),s.host.PresentationDeadline()));
        return !s.demo.ExitRequested();
    }
    {ScopedNativeDuration measure(s.presentationTiming);s.host.SubmitFrame(*s.frame);}
    // Native timings have separate clock domains. CPU work includes capture,
    // Gather and recording/submission; waiting and presentation are reported
    // separately. GPU time is from the preceding completed Vulkan submission,
    // never the wall time spent by the CPU inside SubmitFrame.
    EcstasyVulkanFrameStatistics renderTiming{};
    const bool hasRenderTiming=s.host.ReadRenderStatistics(renderTiming);
    const auto updateMs=capturedUpdateMs+s.deferredPresentationCpuMilliseconds;
    s.deferredPresentationCpuMilliseconds=0.0;
    applicationGpuTimingAvailable=hasRenderTiming && renderTiming.gpuTimeAvailable;
    if(s.clock.realTimeFrames>0 && (s.clock.realTimeFrames&8)!=((s.clock.realTimeFrames-1)&8)) {
        const auto rate=[](double milliseconds) {return static_cast<float>(1000.0/std::max(milliseconds,0.001));};
        applicationPreviousCpuFrameRate=rate(updateMs+renderTiming.gatherMilliseconds+renderTiming.recordSubmitMilliseconds);
        applicationPreviousGpuFrameRate=applicationGpuTimingAvailable?rate(renderTiming.gpuMilliseconds):0.0f;
    }
    if(hasRenderTiming) {
        ++s.renderTimedFrames;s.renderUpdateMs+=updateMs;
        s.renderGatherMs+=renderTiming.gatherMilliseconds;s.renderFenceMs+=renderTiming.fenceWaitMilliseconds;
        s.renderSubmitMs+=renderTiming.recordSubmitMilliseconds;s.renderPresentMs+=renderTiming.presentMilliseconds;
        if(renderTiming.gpuTimeAvailable && renderTiming.completedGpuFrame!=s.lastCompletedGpuFrame) {
            ++s.gpuTimedFrames;s.renderGpuMs+=renderTiming.gpuMilliseconds;
            s.lastCompletedGpuFrame=renderTiming.completedGpuFrame;
        }
        const auto timingObserved=s.host.FrameTime();
        if(timingObserved-s.renderTimingWindowStart>=std::chrono::seconds(5)) {
            const auto count=static_cast<double>(s.renderTimedFrames);
            char detail[512]{};
            std::snprintf(detail,sizeof(detail),
                "frames=%llu cpu_update_ms=%.3f cpu_gather_ms=%.3f cpu_submit_ms=%.3f fence_wait_ms=%.3f present_ms=%.3f gpu_ms=%.3f gpu_samples=%llu gpu_completed_frame=%llu hud_fps=%.2f hud_frame_ms=%.3f",
                static_cast<unsigned long long>(s.renderTimedFrames),s.renderUpdateMs/count,s.renderGatherMs/count,
                s.renderSubmitMs/count,s.renderFenceMs/count,s.renderPresentMs/count,
                s.gpuTimedFrames?s.renderGpuMs/static_cast<double>(s.gpuTimedFrames):0.0,
                static_cast<unsigned long long>(s.gpuTimedFrames),static_cast<unsigned long long>(s.lastCompletedGpuFrame),
                applicationFrameTiming.framesPerSecond,applicationFrameTiming.frameMilliseconds);
            s.host.RecordSessionEvent("render.timing_window",detail);
            s.renderTimingWindowStart=timingObserved;s.renderTimedFrames=0;s.gpuTimedFrames=0;
            s.renderUpdateMs=0.0;s.renderGatherMs=0.0;s.renderFenceMs=0.0;
            s.renderSubmitMs=0.0;s.renderPresentMs=0.0;s.renderGpuMs=0.0;
        }
    }

    Require(s.frame->State()==EcstasyFrame::Phase::Complete,"Physical presenter did not retire the CPU frame capture");
    if(advancedPresentation) {
        texturePackRegistry.ServiceReads();s.resources->Loader().ServiceResourceLoading();
        s.replayAvailableBytes=s.host.AvailableMemory();
    }
    // No PAL/NTSC wait: presentation follows the physical backend/VSync.
    // Observe its complete wall interval, including resource and device waits.
    const auto completed=s.host.FrameTime();
    if(s.presentationTimingEpoch!=s.host.PresentationTimingEpoch()) {
        // Reset only on a physical window-resume epoch, never an active stall.
        s.presentationTimingEpoch=s.host.PresentationTimingEpoch();
        s.frameTiming.Reset(completed);s.simulationClock.Reset(completed);applicationFrameTiming={};
    } else if(s.frameTiming.Observe(completed,hasRenderTiming?&renderTiming:nullptr,updateMs)) {
        applicationFrameTiming=s.frameTiming.Read();
    }
    return !s.demo.ExitRequested();
}

std::exception_ptr ApplicationRuntime::ShutdownNoThrow() noexcept
{
    auto& s=*storage_;
    if(s.shutdownComplete || s.shutdownStarted)return s.shutdownFailure;
    s.shutdownStarted=true;
    s.host.RecordSessionEvent("shutdown.begin","terminal owner retirement");
    if(s.resources) {
        s.resources->Registry().Models().fixupsBlocked=1;
        s.resources->Loader().BeginShutdown();
        carLoader.BeginShutdown(s.resources->Loader());
        for(auto* pack:texturePackRegistry.packs)
            if(&pack->CatalogueRoot()==&s.resources->Registry().Models())pack->BeginShutdown();
    }
    // Close/join the actual consumer while all borrowed buffers still live.
    // Cancellation rejects retained commands; it never manufactures an ACK.
    if(s.audioOpened)s.Retire("Physical audio shutdown",[&] {
        s.AudioResult(s.host.CloseAudio(s.sound),"Physical audio shutdown");s.audioOpened=false;
    });
    if(s.audioBound)s.RetireAudio("Audio request cancellation",[&] {
        return s.sound.CancelDeviceRequestsAfterClose();
    });
    s.Retire("CPU frame cancellation",[&] {
        Require(s.frame->State()!=EcstasyFrame::Phase::Submitted,
            "Shutdown precedes retirement of the submitted CPU frame capture");
        if(s.frame->State()!=EcstasyFrame::Phase::Complete)s.frame->CancelBeforeSubmit();
    });
    s.Retire("Pending frontend frame",[&]{s.frontendFrame.Clear();});
    s.Retire("Pending UI submissions",[&]{s.renderer.CompleteSubmissions();});
    s.Retire("Active screen",[&]{if(s.screens.active)s.screens.ChangeToScreen(nullptr,false);});
    s.screens.active=nullptr;
    s.Retire("Input callbacks",[&] {
        while(s.input.count) {
            const auto registration=s.input.registrations[s.input.count-1];
            Require(gameInputUnregister(&s.input,registration.device,registration.event)!=0,
                "Terminal input callback has no registered owner");
        }
    });
    if(s.menusBound) {
        s.Retire("Campaign screen",[]{UnbindCampaignScreen();});
        s.Retire("Memcard screens",[]{UnbindMemcardScreens();});
        s.Retire("Car selection",[]{UnbindSelectCarScreen();});
        s.Retire("Track selection",[]{UnbindTrackSelectScreen();});
        s.Retire("Credits",[]{UnbindCreditsScreens();});
        s.Retire("Frontend startup",[]{UnbindFrontendStartup();});
        s.Retire("Main menu",[]{UnbindMainMenuScreen();});
        s.menusBound=false;s.mainMenuBound=false;
    } else if(s.mainMenuBound) {
        s.Retire("Main menu",[]{UnbindMainMenuScreen();});s.mainMenuBound=false;
    }
    // The authorized profile write joins in the owner destructor. Do not
    // dispatch its queued UI continuation during terminal cleanup.
    s.memcards.reset();
    if(s.movie)s.RecordCleanupFailure("Movie",s.movie->Shutdown());
    // Gameplay borrows these screens/backgrounds. Retire it before deleting
    // their owners, but retain phase fileBindings until all reads are joined.
    if(s.trackPhase)s.RecordCleanupFailure("Track phase",s.trackPhase->Close());
    if(s.frontendPhase)s.RecordCleanupFailure("Frontend phase",s.frontendPhase->Close());
    s.loadingAnimation.reset();s.screens.loadingAnimation=nullptr;
    s.background.reset();s.help.reset();s.screenStorage.clear();
    for(auto& popup:s.popups)popup.reset();
    s.screens.background=nullptr;s.screens.help=nullptr;s.screens.popups.fill(nullptr);
    s.screens.movieTextures={};s.screens.movieFilename.clear();s.screens.overrideHelpItems=nullptr;
    s.screens.root.next=s.screens.root.previous=&s.screens.root;
    if(s.resources) {
        s.Retire("Flailer strips",[&]{s.resources->Registry().Flailer().DeleteAllStrips();});
        s.RecordCleanupFailure("Resource loader",s.resources->Loader().Shutdown());
    }
    s.trackPhase.reset();s.frontendPhase.reset();s.global=nullptr;
    if(s.languageInitialized) {
        if(s.speechBound && s.language.textEnabled)
            s.RetireSpeech("Localized speech cleanup",[&]{return soundUnloadLocalizedSpeech(s.speechStream);});
        s.Retire("Language",[&]{s.language.Close();s.languageInitialized=false;});
    }
    if(s.speechBound) {
        auto* speech=worldSpeechGetInstance();
        speech->messages=nullptr;speech->streamedAudio=nullptr;speech->session=nullptr;
        s.RetireSpeech("Speech process shutdown",[&]{return worldStreamAudioDestroy(&s.speechStream);});
        s.speechBound=false;
    }
    if(s.audioBound) {
        if(s.soundScapeBound)s.RetireAudio("Soundscape",[]{return soundScapeDispose();});
        if(s.jukeboxBound)s.RetireAudio("Music",[&] {
            Require(soundJukeboxPlayer.audio==&s.audioQueue,"Music shutdown lost its actual audio owner");
            return soundJukeboxPlayer.Close();
        });
        s.RetireAudio("Positional sound",[&]{return s.positionalSamples.Close();});
        s.RetireAudio("Sound process",[&]{return s.sound.ShutdownAfterOwnersRetired();});
        s.Retire("Audio completion callbacks",[&] {
            Require((!soundDriverState.completionFlow || soundDriverState.completionFlow==&s.flow) &&
                (!soundDriverState.completionBindings || soundDriverState.completionBindings==&s.flowBindings),
                "Sound shutdown encountered another application's completion owner");
            soundDriverState.completion=SoundRegionCompletion::None;
            soundDriverState.completionFlow=nullptr;soundDriverState.completionBindings=nullptr;
            soundDriverState.commonRegionLoaded=0;soundDriverState.frontendRegionCache=nullptr;
            for(auto& region:soundDriverState.regions)region={};
        });
        s.audioBound=false;
    }
    if(s.soundScapeBound)s.Retire("Weather clock",[&] {
        Require(weatherManUnbindClock(&s.clock)==WEATHER_MAN_OK,
            "Weather clock retirement has a different owner or live regions");s.soundScapeBound=false;
    });
    if(s.jukeboxBound)s.Retire("Music callback",[&] {
        s.AudioResult(soundJukeboxPlayer.UnbindAfterAudioShutdown(s.sound,s.audioQueue),"Music callback shutdown");
        s.jukeboxBound=false;
    });
    if(s.speechNoiseBound)s.Retire("Speech noise callback",[&] {
        auto* speech=worldSpeechGetInstance();Require(speech && speech->noise,"Speech noise owner disappeared");
        s.AudioResult(speech->noise->UnbindAfterAudioShutdown(s.audioQueue),"Speech noise callback shutdown");
        s.speechNoiseBound=false;
    });
    s.renderer.bitmapComposite.movie=nullptr;s.ui.movie=nullptr;
    s.movie.reset();s.environmentMap.reset();
    for(auto* pack:{s.carTextures.get(),s.hudTextures.get(),s.dynamicTextures.get()})
        if(pack)s.RecordCleanupFailure("Texture pack",pack->Shutdown());
    s.carTextures.reset();s.hudTextures.reset();s.dynamicTextures.reset();
    s.highScores.reset();s.tracks.reset();s.resources.reset();
    if(s.inputBound) {
        s.Retire("Input type monitoring",[&]{joystickCloseTypeMonitoring(&s.input);});
        s.Retire("Input samples",[&]{joystickShutdown(&s.samples);});s.inputBound=false;
    }
    if(easterEggs.input==&s.input)easterEggs.input=nullptr;
    s.Retire("Stomper",[]{CloseStomper();});
    s.initialized=false;s.shutdownComplete=true;
    s.host.RecordSessionEvent("shutdown.end",s.shutdownFailure?"completed with cleanup failure":"complete");
    return s.shutdownFailure;
}

void ApplicationRuntime::Shutdown()
{
    if(auto failure=ShutdownNoThrow())std::rethrow_exception(failure);
}
}
