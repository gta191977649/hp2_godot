#include "bware/inc/bChunk.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/FlailerDB.hpp"
#include "indep/inc/NIS.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/CarLoader.hpp"
#include "indep/inc/Fred.hpp"
#include "indep/inc/GameFlow.hpp"
#include "indep/inc/DemoDisc.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/SmokeableInfo.hpp"
#include "indep/inc/CarDamage.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/RaceParameters.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/Race.hpp"
#include "indep/inc/Scenery.hpp"
#include "indep/inc/CarSounds.hpp"
#include "indep/inc/WeatherMan.hpp"
#include "indep/inc/eLight.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/HeliSheet.hpp"
#include "indep/inc/EventManager.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bFile.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <iterator>
#include <list>
#include <utility>
#include <vector>

namespace hp2 {

namespace {
const char* resourceKindName(ResourceFileKind kind) noexcept
{
    switch (kind) {
    case ResourceFileKind::Nis:return "NIS";
    case ResourceFileKind::Car:return "car";
    case ResourceFileKind::Track:return "track";
    case ResourceFileKind::Global:return "global";
    case ResourceFileKind::Frontend:return "frontend";
    case ResourceFileKind::TrackCommon:return "track-common";
    case ResourceFileKind::TrackAuxiliary:return "track-auxiliary";
    }
    return "invalid";
}

void logResourceEvent(const char* event,const std::string& filename,ResourceFileKind kind,
    std::int32_t type,std::uint32_t flags,std::int32_t request,std::uint64_t bytes,
    std::size_t pending,double milliseconds=0,const char* error="") noexcept
{
    char detail[1792]{};
    char pendingText[32]{};
    if (pending==std::numeric_limits<std::size_t>::max())std::snprintf(pendingText,sizeof(pendingText),"unknown");
    else std::snprintf(pendingText,sizeof(pendingText),"%zu",pending);
    std::snprintf(detail,sizeof(detail),
        "file=\"%.650s\" kind=%s type=%d flags=0x%X request=%d bytes=%llu pending=%s elapsed_ms=%.3f error=\"%.700s\"",
        filename.c_str(),resourceKindName(kind),type,static_cast<unsigned>(flags),request,
        static_cast<unsigned long long>(bytes),pendingText,milliseconds,error);
    RecordSessionEvent(event,detail);
}

// UI bundles share their physical ResourceFile lifetime. FRONTB additionally
// owns the authored event trees and profile bindings; INGAME and track UI
// bundles publish only their actual Flailer packages, models and textures.
struct LoadedFrontendResources final {
    ResourceRegistry& registry;
    FrontendPlayerManager& profiles;
    FlailerRenderManager& renderer;
    std::unique_ptr<EventTreeResources> events;
    std::vector<FlailerScreenResource*> screens;
    std::uint64_t flailerBundle{};
    bool profilesBound{};
    LoadedFrontendResources(const std::vector<std::uint8_t>& bytes,ResourceRegistry& shared,
        const ResourceFrontendBindings& bindings,ResourceFileKind kind)
        :registry(shared),profiles(bindings.profiles),renderer(bindings.renderer),
         events(kind==ResourceFileKind::Frontend ?
             std::make_unique<EventTreeResources>(bytes,bindings.demo.ActivationWord()!=0) : nullptr)
    {
        Require(&registry.Flailer().Models()==&registry.Models(),
            "Frontend resources require the same actual model root");
        if (events)Require(!profiles.boundEventResources,
            "Frontend profiles still borrow another event-tree resource");
        try {
            const auto previousScreenCount=registry.Flailer().ScreenCount();
            flailerBundle=registry.Flailer().AppendBundle(bytes);
            for (auto i=previousScreenCount;i<registry.Flailer().ScreenCount();++i)
                screens.push_back(&registry.Flailer().ScreenAt(i));
            if (events) {profiles.BindEventProgress(*events);profilesBound=true;}
        } catch (...) {Release();throw;}
    }
    void Release()
    {
        Require(!flailerBundle || registry.Flailer().CanRemoveBundle(flailerBundle),
            "Frontend UI strips or models still retain their actual resource");
        // This CPU renderer caches pointers to the same decoded screens.
        // Retire only this file's entries after actual submission references
        // have drained; never manufacture a GPU completion to release them.
        for (auto* screen:screens)renderer.SetScreenVisibility(screen,1);
        screens.clear();
        if (profilesBound) {
            Require(profiles.boundEventResources==events.get(),"Frontend profile owner changed before resource retirement");
            profiles.boundEventResources=nullptr;profilesBound=false;
        }
        if (flailerBundle) {
            Require(registry.Flailer().RemoveBundle(flailerBundle),"Frontend resource unpublication failed");
            flailerBundle=0;
        }
    }
    ~LoadedFrontendResources() {try {Release();}catch (...) {std::terminate();}}
};

void validateFrontendChunks(const std::vector<std::uint8_t>& bytes)
{
    for (const auto& chunk:ReadChunks(bytes))switch (chunk.id) {
    case 0:case 0x80034000u:case 0xb0300001u:case 0xb0300100u:
    case 0x80010200u:case 0x80034a00u:case 0x15010u:break;
    default:throw std::runtime_error("Frontend ResourceFile chunk has no actual CPU provider");
    }
}

// GLOBALB is a single resource lifetime. Every published pointer below is
// borrowed from these actual decoded owners, never another GLOBALB load.
struct LoadedGlobalResources final {
    ResourceRegistry& registry;
    AcidFXRuntime& runtime;
    CarInfoResource cars;
    TrackScenarioData tracks;
    CarDriverAppearanceResources characters;
    DriverBehaviorData behavior;
    NisResources nis;
    ChopperEffectData effects;
    std::uint64_t flailerBundle{};
    bool rootParticles{},runtimeBound{};

    LoadedGlobalResources(const GameResources& files,const std::vector<std::uint8_t>& bytes,
        ResourceRegistry& shared,const ResourceGlobalBindings& bindings)
        :registry(shared),runtime(bindings.effects),cars(bytes,bindings.availability),tracks(bytes),
         characters(bytes),behavior(bytes),nis(files,bytes),effects(bytes)
    {
        Require(!registry.Effects().Particles() && registry.Effects().GroupCount()==0,
            "The global FX root must be empty before GLOBALB publication");
        Require(&registry.Flailer().Models()==&registry.Models(),
            "Global Flailer and resource services must share their one model root");
        try {
            flailerBundle=registry.Flailer().AppendBundle(bytes);
            Require(effects.Register(registry.Effects()),"GLOBALB FX catalogue is already registered");
            Require(registry.Effects().BindParticles(runtime.Particles())==WORLD_SERVICES_OK,
                "Global FX root did not bind the canonical particle owner");
            rootParticles=true;
            Require(effects.BindParticles(runtime.Particles())==WORLD_SERVICES_OK,
                "GLOBALB authored emitters did not bind the canonical particle owner");
            runtime.BindCatalogue(effects);runtimeBound=true;
        } catch (...) {Release();throw;}
    }
    void Release()
    {
        Require(!flailerBundle || registry.Flailer().CanRemoveBundle(flailerBundle),
            "Global UI strips or models are still borrowed by live consumers");
        if (runtimeBound) {runtime.ReleaseCatalogue(effects);runtimeBound=false;}
        else Require(effects.UnbindParticles()==WORLD_SERVICES_OK,
            "Global particle definitions are still borrowed by live effects");
        effects.Unregister();
        if (rootParticles) {
            Require(registry.Effects().UnbindParticles()==WORLD_SERVICES_OK,
                "The global FX root is still borrowed");
            rootParticles=false;
        }
        if (flailerBundle) {
            Require(registry.Flailer().RemoveBundle(flailerBundle),"Global UI resource retirement failed");
            flailerBundle=0;
        }
    }
    ~LoadedGlobalResources()
    {
        try {Release();}catch (...) {std::terminate();}
    }
};

void validateGlobalChunks(const std::vector<std::uint8_t>& bytes)
{
    // GLOBALA is genuinely a zero-byte final disc file. A nonempty GLOBALB
    // must have all its real CPU providers before it can become loaded.
    std::uint32_t tables=0;
    for (const auto& chunk:ReadChunks(bytes))switch (chunk.id) {
    case 0:case 0x15010u:case 0x3401eu:case 0xb0300001u:case 0xb0300100u:
    case 0x80010200u:break;
    case 0x34600u:tables|=1u;break;
    case 0x34601u:tables|=2u;break;
    case 0x37010u:tables|=4u;break;
    case 0x34301u:tables|=8u;break;
    case 0x34300u:tables|=16u;break;
    case 0x34201u:tables|=32u;break;
    case 0x35021u:tables|=64u;break;
    case 0x35020u:tables|=128u;break;
    case 0x34b00u:tables|=256u;break;
    default:throw std::runtime_error("Global ResourceFile chunk has no actual CPU provider");
    }
    Require(bytes.empty() || tables==511u,"Nonempty Global file is missing an actual GLOBALB catalogue");
}

// One loaded track supplies all of its CPU services. Weather, sound and
// particles borrow these owners; none loads another copy of TRACKB.
struct LoadedTrackResources final {
    TrackScenarioData track;
    TerrainData terrain;
    TrackRouteData route;
    CarRecoveryData recovery;
    LightSceneResources lights;
    TrackRouteEditorResources models;
    CollisionObjectData objects;
    SceneryResources scenery;
    AerialNavigationData navigation;
    WorldEventsData events;
    ChopperEffectData effects;
    CarSoundBindings* sound;
    SunResources& sun;
    const GameTime* sceneClock{};
    bool weatherActive{},soundActive{},sceneClockBound{};

    // PAL1691FC..169210 publishes TrackPath reverse==1 before InitTrackRoutes.
    // The path and preprocessed road must consume that same direction.
    LoadedTrackResources(const std::vector<std::uint8_t>& bytes,ResourceRegistry& registry,
        bool reverse,CarSoundBindings* audio,const GameTime* clock,WorldParticleSystem& particles)
        :track(bytes),terrain(track.Authored()),route(track.Authored(),reverse),
         recovery(track.Authored(),&route.Native(),reverse),
         lights(track.Authored(),registry.Lights()),
         models(track.Authored(),&registry.Models()),objects(track.Authored(),terrain.Topology()),
         scenery(track.Authored(),registry.Models(),reverse),
         navigation(track.Authored()),events(track.Authored()),
         effects(track.Authored(),&registry.Effects()),sound(audio),sun(registry.Sun()),
         sceneClock(audio ? nullptr : clock)
    {
        Require(track.CurrentInfo()!=nullptr,"Loaded track has no actual34200 owner");
        Require(registry.Effects().Particles()==&particles,
            "GLOBALB authored emitters must already borrow this same CPU particle owner");
        Require(effects.Register(registry.Effects()),"Track FX catalogue is already registered");
        try {
            for (const auto& chunk:ReadChunks(track.Authored()))sun.LoadChunk(track.Authored(),chunk);
            Require(effects.BindParticles(particles)==WORLD_SERVICES_OK,"Actual static FX emitter publication failed");
            if (sound)Require(carSoundBindTrackBank(*sound,track)==WORLD_AUDIO_OK,"Track sound-bank binding failed");
            Require(weatherManBindClock(clock)==WEATHER_MAN_OK,"Track weather has no actual game clock");
            sceneClockBound=sceneClock!=nullptr;
            Require(weatherManLoadResources(track)==WEATHER_MAN_OK,"Actual weather resource publication failed");
            weatherActive=true;
            if (sound) {
                Require(soundScapeLoadResources(*sound,track)==WORLD_AUDIO_OK,"Actual SoundScape resource publication failed");
                soundActive=true;
            }
        } catch (...) {Release();throw;}
    }
    void Release()
    {
        if (soundActive) {
            Require(soundScapeUnloadResources(track)==WORLD_AUDIO_OK,"Actual SoundScape resource retirement failed");
            soundActive=false;
        }
        if (weatherActive) {
            Require(weatherManUnloadResources(track)==WEATHER_MAN_OK,"Actual weather resource retirement failed");
            weatherActive=false;
        }
        if (sceneClockBound) {
            Require(weatherManUnbindClock(sceneClock)==WEATHER_MAN_OK,"Scene weather clock retirement failed");
            sceneClockBound=false;
        }
        if (sound && sound->trackInfo==track.CurrentInfo()) {sound->trackInfo=nullptr;sound->trackBank=nullptr;}
        Require(effects.UnbindParticles()==WORLD_SERVICES_OK,"Track FX definitions are still borrowed by live world effects");
        effects.Unregister();
        sun.Unload();
    }
    ~LoadedTrackResources()
    {
        try {Release();}catch (...) {std::terminate();}
    }
    void ReleaseForShutdown(std::exception_ptr& failure)
    {
        if(soundActive) {
            const auto error=soundScapeShutdownResources(track);
            if(!failure)failure=error;
            soundActive=false;
        }
        // The application has already retired actual World/camera/frame
        // borrowers. Keep these guards; never fake their retention counts.
        Release();
    }
};

// Common/auxiliary track bundles carry the same real model/texture, HUD,
// animation, collision-definition and camera providers. They do not create
// another TrackScenarioData or pretend that TRACKA is a complete TRACKB.
bool validateTrackVisualChunks(const std::vector<std::uint8_t>& bytes)
{
    bool animations=false;
    for (const auto& chunk:ReadChunks(bytes))switch (chunk.id) {
    case 0x80034008u:animations=true;break;
    case 0:case 0x80034020u:case 0x34026u:
    case 0x80034000u:case 0xb0300001u:case 0xb0300100u:
    case 0x80010200u:case 0x15010u:case 0x80034410u:case 0x80034415u:
    case 0x34492u:break;
    default:throw std::runtime_error("Track visual ResourceFile chunk has no actual provider");
    }
    return animations;
}

void validateTrackChunks(const std::vector<std::uint8_t>& bytes)
{
    for (const auto& chunk:ReadChunks(bytes))switch (chunk.id) {
    case 0:case 0x80036000u:case 0x35020u:case 0x35021u:case 0x34250u:
    case 0x80034020u:case 0x34026u:case 0x34027u:case 0x80034000u:case 0x80034100u:
    case 0x34146u:case 0x80034147u:case 0x80034800u:case 0x34121u:case 0x34122u:
    case 0x34130u:case 0x34131u:case 0x34132u:case 0x34133u:case 0x34200u:
    case 0x80034405u:case 0x80034425u:case 0x80034410u:case 0x80034415u:case 0x34492u:
    case 0x80034500u:case 0x80038000u:case 0x34202u:
    case 0x80034015u:case 0x80034019u:case 0x34106u:break;
    default:throw std::runtime_error("Track ResourceFile chunk has no actual CPU provider");
    }
}
}

void ResourceRegistry::PostLoadFixup()
{
    ResourceRefresh();
    UpdateLightTextures();
    Effects().UpdateTextures(*this);
    Flailer().DeleteAllStrips();
}

struct ResourceFile::Storage {
    std::string filename;
    std::int32_t type{};
    std::uint32_t flags{};
    PhysicalResource physical;
    std::vector<std::uint8_t> data;
    std::int32_t transfersPending{};
    std::int32_t queuedId{};
    std::exception_ptr failure;
    bool loaded{};
    bool cameraData{};
    std::string cameraIdentity;
    ResourceFileBindings bindings{};
    ResourceFileCompletion completion{ResourceFileCompletion::None};
    std::unique_ptr<CarReplayAnimationResources> animationData;
    std::unique_ptr<TrackRouteEditorResources> modelData;
    std::unique_ptr<CollisionObjectData> collisionObjects;
    std::unique_ptr<CarDamageData> damage;
    std::unique_ptr<PositionMarkerResources> positionMarkers;
    std::unique_ptr<LoadedTrackResources> trackData;
    std::unique_ptr<LoadedGlobalResources> globalData;
    std::unique_ptr<LoadedFrontendResources> frontendData;
    std::uint32_t trackCameraGroups{};
    ResourceFileResources resources{};
};

struct ResourceLoader::Storage {
    const GameResources& files;
    ResourceRegistry& registry;
    std::list<std::unique_ptr<ResourceFile>> ordered;
    QueuedFileService& queued;
    std::size_t pendingFiles{};
    bool shuttingDown{},shutdownComplete{};
    std::exception_ptr shutdownFailure;
    explicit Storage(const GameResources& input,ResourceRegistry& shared,QueuedFileService& queue)
        : files(input),registry(shared),queued(queue) {}
};

ResourceFile::ResourceFile(std::unique_ptr<Storage> storage):storage_(std::move(storage)) {}
ResourceFile::~ResourceFile()=default;
const std::string& ResourceFile::Filename() const noexcept { return storage_->filename; }
std::int32_t ResourceFile::Type() const noexcept { return storage_->type; }
std::uint32_t ResourceFile::Flags() const noexcept { return storage_->flags; }
bool ResourceFile::Loaded() const noexcept { return storage_->loaded; }
std::int32_t ResourceFile::TransfersPending() const noexcept { return storage_->transfersPending; }
void ResourceFile::QueuedReadDone(bool failed)
{
    auto& file=*storage_;
    Require(file.queuedId && file.transfersPending==1,"Resource transfer has no actual queued owner");
    logResourceEvent(failed ? "resource.read.failed" : "resource.read.complete",file.filename,
        file.bindings.kind,file.type,file.flags,file.queuedId,file.data.size(),std::numeric_limits<std::size_t>::max());
    file.queuedId=0;--file.transfersPending;
    if (failed)file.failure=std::make_exception_ptr(std::runtime_error("ResourceFile physical transfer failed"));
}
const ResourceFileResources& ResourceFile::Resources() const
{
    Require(storage_->loaded,"Resource owners are not published before file completion");
    return storage_->resources;
}

std::int32_t ResourceFile::GetSize(std::uint32_t kind,std::int32_t* chunks) const
{
    std::uint32_t bytes=0,count=0;
    for (const auto& chunk:ReadChunks(storage_->data)) if (!kind || chunk.id==kind) {
        bytes+=chunk.size;++count;
    }
    std::int32_t result;
    std::memcpy(&result,&bytes,sizeof(result));
    if (chunks)std::memcpy(chunks,&count,sizeof(count));
    return result;
}

void ResourceFile::ChangeFilenameForHotChunking(const char* filename)
{
    Require(filename!=nullptr,"Hot-chunk filename is missing");
    // PAL199720 -> 199A60 writes the lookup name (79 bytes at receiver+0x10).
    // The queued request already owns the physical input path separately:
    // renaming a compressed bundle must not redirect its pending file read.
    storage_->filename.assign(filename,std::min(std::strlen(filename),std::size_t(79)));
}

ResourceLoader::ResourceLoader(const GameResources& files,ResourceRegistry& registry,QueuedFileService& queued)
    :storage_(std::make_unique<Storage>(files,registry,queued))
{
    Require(&files==&queued.Files(),"Resource loader and queue must borrow the same physical file owner");
    registry.BindQueuedFiles(queued);
}

ResourceLoader::~ResourceLoader()
{
    if(const auto failure=Shutdown()) {
        try {std::rethrow_exception(failure);}
        catch(const std::exception& error) {RecordSessionEvent("resource.shutdown.failed",error.what());}
        catch(...) {RecordSessionEvent("resource.shutdown.failed","non-standard exception");}
    }
}

const GameResources& ResourceLoader::Files() const noexcept { return storage_->files; }
ResourceRegistry& ResourceLoader::Registry() noexcept { return storage_->registry; }

ResourceFile* ResourceLoader::LoadResourceFile(const char* filename,std::int32_t type,
    std::uint32_t flags,const ResourceFileBindings& bindings,ResourceFileCompletion completion)
{
    Require(!storage_->shuttingDown,"Resource loader is shutting down");
    Require(filename && *filename,"ResourceFile requires a filename");
    Require(completion==ResourceFileCompletion::None ||
        (completion==ResourceFileCompletion::NisLoaded && bindings.nis) ||
        (completion==ResourceFileCompletion::CarLoaded && bindings.carLoader && bindings.kind==ResourceFileKind::Car) ||
        (completion==ResourceFileCompletion::FrontendLoaded && bindings.frontend && bindings.flow &&
            bindings.gameFlow && bindings.kind==ResourceFileKind::Frontend) ||
        (completion==ResourceFileCompletion::TrackCommonLoaded && bindings.kind==ResourceFileKind::TrackCommon && bindings.flow && bindings.gameFlow) ||
        (completion==ResourceFileCompletion::TrackSpecificLoaded && bindings.kind==ResourceFileKind::Track && bindings.flow && bindings.gameFlow) ||
        ((completion==ResourceFileCompletion::TrackPreRaceLoaded || completion==ResourceFileCompletion::TrackLocationTexturesLoaded ||
            completion==ResourceFileCompletion::TrackDriverTexturesLoaded) && bindings.kind==ResourceFileKind::TrackAuxiliary && bindings.flow && bindings.gameFlow),
        "Resource completion has no actual native owner");
    if (bindings.kind==ResourceFileKind::Nis)
        Require(bindings.cameras && bindings.nis && bindings.cameraGroup>=0 && bindings.cameraGroup<2,
            "NIS chunk registration requires its live camera/replay owners");
    else if (bindings.kind==ResourceFileKind::Car)
        Require(bindings.positionMarkers && (completion==ResourceFileCompletion::None || completion==ResourceFileCompletion::CarLoaded),
            "Car resources require the actual marker registry and valid completion owner");
    else if (bindings.kind==ResourceFileKind::Global)
        Require(bindings.global && completion==ResourceFileCompletion::None &&
            (bindings.global->availability==CarInfoAvailability::FullGame ||
             bindings.global->availability==CarInfoAvailability::DemoDisc),
            "Global resources require the actual FX runtime and explicit content policy");
    else if (bindings.kind==ResourceFileKind::Frontend)
        Require(bindings.frontend && (completion==ResourceFileCompletion::None ||
            completion==ResourceFileCompletion::FrontendLoaded),
            "Frontend resources require actual profile/demo owners");
    else if (bindings.kind==ResourceFileKind::TrackCommon || bindings.kind==ResourceFileKind::TrackAuxiliary)
        Require((bindings.frontend && bindings.cameras) ||
            (bindings.kind==ResourceFileKind::TrackAuxiliary && bindings.sceneClock && completion==ResourceFileCompletion::None),
            "Track visual resources require actual application or scene owners");
    else Require(bindings.kind==ResourceFileKind::Track && bindings.particles &&
            ((bindings.parameters && bindings.sound && bindings.cameras) ||
             (bindings.sceneClock && !bindings.sound && completion==ResourceFileCompletion::None)) &&
            (completion==ResourceFileCompletion::None || completion==ResourceFileCompletion::TrackSpecificLoaded),
        "Track resources require the actual race parameters and audio bindings");
    if (bindings.kind==ResourceFileKind::Track)
        for (const auto& current:storage_->ordered)
            Require(current->storage_->bindings.kind!=ResourceFileKind::Track,
                "The current whole track must retire before another Track ResourceFile is queued");
    auto data=std::make_unique<ResourceFile::Storage>();
    data->filename=filename;data->type=type;data->flags=flags;data->bindings=bindings;data->completion=completion;
    logResourceEvent("resource.request",data->filename,bindings.kind,type,flags,0,0,storage_->pendingFiles);
    // Opening and reading are the Windows physical-file boundary. The game
    // transfer/list/publication/completion algorithm remains in this unit.
    data->physical=storage_->files.OpenPhysical(data->filename);
    Require(data->physical.range.Size()<=std::uint64_t(INT32_MAX),"ResourceFile exceeds signed byte extent");
    data->data.resize(static_cast<std::size_t>(data->physical.range.Size()));
    data->transfersPending=data->data.empty()?0:1;
    auto owner=std::unique_ptr<ResourceFile>(new ResourceFile(std::move(data)));
    ResourceFile* file=owner.get();
    storage_->ordered.push_back(std::move(owner));
    try {
        if (file->storage_->transfersPending) {
            file->storage_->queuedId=storage_->queued.Add(file->storage_->data.data(),filename,0,
                static_cast<std::int32_t>(file->storage_->data.size()),
                {QueuedFileCompletionKind::ResourceFileTransfer,file,nullptr});
            // PAL1993D0..1993E4 sets the resource read to priority5/32MiB.
            // Preserve that request policy without changing smaller texture reads.
            storage_->queued.ChangeParams(file->storage_->queuedId,5,0x02000000);
        }
    } catch (...) {
        if(file->storage_->queuedId)storage_->queued.CancelAndWait(file->storage_->queuedId);
        storage_->ordered.pop_back();throw;
    }
    ++storage_->pendingFiles;
    const auto& queued=*file->storage_;
    logResourceEvent("resource.queued",queued.filename,bindings.kind,type,flags,queued.queuedId,
        queued.data.size(),storage_->pendingFiles);

    return file;
}

std::size_t ResourceLoader::ServiceResourceLoading()
{
    auto& service=*storage_;
    if(service.shuttingDown)return service.pendingFiles;
    service.queued.Service();
    if (!service.pendingFiles)return 0;
    for (auto& owner:service.ordered) {
        auto& file=*owner->storage_;
        if (file.loaded)continue;
        if (file.failure)std::rethrow_exception(file.failure);
        if (file.transfersPending)return service.pendingFiles;
        const auto began=std::chrono::steady_clock::now();
        std::size_t decodedBytes=file.data.size();
        logResourceEvent("resource.decode.begin",file.filename,file.bindings.kind,file.type,file.flags,
            0,decodedBytes,service.pendingFiles);
        try {
        if ((file.flags&8u) && file.data.size()>=4 &&
            std::memcmp(file.data.data(),"COMP",4)==0)
            file.data=DecodeComp(file.data);
        decodedBytes=file.data.size();
        bool animations=false,cameras=false;
        const bool trackVisual=file.bindings.kind==ResourceFileKind::TrackCommon || file.bindings.kind==ResourceFileKind::TrackAuxiliary;
        // Validate every top-level provider once before publication. Complete
        // bundle kinds do not need a second discarded ReadChunks vector.
        if (trackVisual)animations=validateTrackVisualChunks(file.data);
        else if (file.bindings.kind==ResourceFileKind::Track)validateTrackChunks(file.data);
        else if (file.bindings.kind==ResourceFileKind::Global)validateGlobalChunks(file.data);
        else if (file.bindings.kind==ResourceFileKind::Frontend)validateFrontendChunks(file.data);
        else for (const auto& chunk:ReadChunks(file.data)) {
            if (file.bindings.kind==ResourceFileKind::Car) {
                switch (chunk.id) {
                case 0:case 0x34013u:case 0x80034020u:case 0x34026u:case 0x34030u:case 0x80034000u:break;
                default:throw std::runtime_error("Car ResourceFile chunk has no actual CPU loader");
                }
            } else {
                if (chunk.id==0x80034008u)animations=true;
                else if (chunk.id==0x80034420u)cameras=true;
                else Require(chunk.id==0,"ResourceFile chunk has no bound native loader");
            }
        }
        Require(!(animations && cameras),"NIS animation/camera publication has mixed file ownership");
        if (file.bindings.kind==ResourceFileKind::Global && !file.data.empty()) {
            for (const auto& other:service.ordered)
                Require(!other->storage_->globalData,"One GLOBALB owner must serve all live consumers");
            auto global=std::make_unique<LoadedGlobalResources>(service.files,file.data,service.registry,
                *file.bindings.global);
            file.resources.models=&service.registry.Models();file.resources.cars=&global->cars;
            file.resources.track=&global->tracks;file.resources.characters=&global->characters;
            file.resources.driverBehavior=&global->behavior;file.resources.nis=&global->nis;
            file.resources.effects=&global->effects;file.globalData=std::move(global);
        }
        if (trackVisual && !file.bindings.frontend && !file.data.empty()) {
            file.modelData=std::make_unique<TrackRouteEditorResources>(file.data,&service.registry.Models());
            file.resources.models=&file.modelData->Native();
        }
        if ((file.bindings.kind==ResourceFileKind::Frontend || (trackVisual && file.bindings.frontend)) && !file.data.empty()) {
            auto frontend=std::make_unique<LoadedFrontendResources>(file.data,service.registry,
                *file.bindings.frontend,file.bindings.kind);
            file.resources.models=&service.registry.Models();file.resources.frontendEvents=frontend->events.get();
            file.frontendData=std::move(frontend);
        }
        if (animations) {
            file.animationData=std::make_unique<CarReplayAnimationResources>(file.data,
                &service.registry.Animations(),&service.registry.Models(),!trackVisual);
            file.resources.models=&file.animationData->Models();
            file.resources.animations=&file.animationData->Animations();
        }
        if (trackVisual && file.bindings.cameras && !file.data.empty()) {
            file.collisionObjects=std::make_unique<CollisionObjectData>(file.data,file.bindings.terrain);
            file.resources.collisionObjects=file.collisionObjects.get();
            Require(carViewCameraResourcesLoadBundle(file.bindings.cameras,file.data.data(),file.data.size()),
                "Track common camera resources failed actual decoding");
            for (const auto& chunk:ReadChunks(file.data)) {
                if (chunk.id==0x80034410u)file.trackCameraGroups|=4u;
                if (chunk.id==0x80034415u)file.trackCameraGroups|=8u;
                if (chunk.id==0x34492u)file.trackCameraGroups|=16u;
            }
            file.resources.cameras=file.bindings.cameras;
        }
        if (file.bindings.kind==ResourceFileKind::Car) {
            // CollisionObjectData owns the hulls as well as the34026
            // definitions. Do not build a second CollisionResource array.
            auto models=std::make_unique<TrackRouteEditorResources>(file.data,&service.registry.Models());
            auto objects=std::make_unique<CollisionObjectData>(file.data,file.bindings.terrain);
            auto damage=std::make_unique<CarDamageData>(file.data);
            auto markers=std::make_unique<PositionMarkerResources>(file.data,*file.bindings.positionMarkers);
            file.modelData=std::move(models);file.collisionObjects=std::move(objects);
            file.damage=std::move(damage);file.positionMarkers=std::move(markers);
            file.resources.models=&file.modelData->Native();
            file.resources.collisionObjects=file.collisionObjects.get();file.resources.damage=file.damage.get();
        }
        if (file.bindings.kind==ResourceFileKind::Track) {
            const auto* parameters=file.bindings.parameters;
            auto* sound=file.bindings.sound;
            Require(!sound || !sound->race || (sound->race->services && sound->race->services->settings==parameters),
                "Track and audio must borrow the same canonical race parameters");
            auto track=std::make_unique<LoadedTrackResources>(file.data,service.registry,
                parameters && parameters->reverse==1,sound,sound ? sound->clock : file.bindings.sceneClock,
                *file.bindings.particles);
            if (file.bindings.cameras) {
                Require(carViewCameraResourcesLoadBundle(file.bindings.cameras,file.data.data(),file.data.size()),
                "Track camera resources failed actual decoding");
                for (const auto& chunk:ReadChunks(file.data)) {
                    if (chunk.id==0x80034405u)file.trackCameraGroups|=1u;
                    if (chunk.id==0x80034425u)file.trackCameraGroups|=2u;
                    if (chunk.id==0x80034410u)file.trackCameraGroups|=4u;
                    if (chunk.id==0x80034415u)file.trackCameraGroups|=8u;
                    if (chunk.id==0x34492u)file.trackCameraGroups|=16u;
                }
            }
            file.resources.models=&track->models.Native();file.resources.collisionObjects=&track->objects;
            file.resources.track=&track->track;file.resources.terrain=&track->terrain;file.resources.route=&track->route;
            file.resources.recovery=&track->recovery;file.resources.scenery=&track->scenery;
            file.resources.navigation=&track->navigation;file.resources.events=&track->events;
            file.resources.effects=&track->effects;
            file.resources.cameras=file.bindings.cameras;file.trackData=std::move(track);
        }
        if (cameras) {
            Require(file.filename.size()>4,"NIS camera filename is too short");
            file.cameraIdentity=file.filename.substr(0,file.filename.size()-4);
            Require(carViewCameraResourcesLoadReplayFile(file.bindings.cameras,&service.files,
                file.cameraIdentity.c_str(),file.data.data(),file.data.size())!=0,"Recorded camera file publication failed");
            file.cameraData=true;
        }
        if (file.flags&4u)std::vector<std::uint8_t>().swap(file.data);
        // This is an actual CPU dependency, not a renderer-complete callback.
        // It must refresh the same model/animation/texture/Flailer owners.
        service.registry.PostLoadFixup();
        } catch (...) {
            file.failure=std::current_exception();
            const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();
            try {std::rethrow_exception(file.failure);}
            catch (const std::exception& error) {
                logResourceEvent("resource.decode.failed",file.filename,file.bindings.kind,file.type,file.flags,
                    0,decodedBytes,service.pendingFiles,elapsed,error.what());
            } catch (...) {
                logResourceEvent("resource.decode.failed",file.filename,file.bindings.kind,file.type,file.flags,
                    0,decodedBytes,service.pendingFiles,elapsed,"non-standard exception");
            }
            throw;
        }
        file.loaded=true;
        Require(service.pendingFiles!=0,"ResourceFile count underflow");
        --service.pendingFiles;
        const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();
        logResourceEvent("resource.published",file.filename,file.bindings.kind,file.type,file.flags,
            0,decodedBytes,service.pendingFiles,elapsed);

        const auto completion=file.completion;
        auto* nis=file.bindings.nis;
        auto* carCompletion=file.bindings.carLoader;
        auto* flow=file.bindings.flow;
        const auto* gameFlow=file.bindings.gameFlow;
        // The real NIS callback may unload this file and queue another pair.
        // No reference/iterator into the old file is used after that call.
        if (completion==ResourceFileCompletion::NisLoaded)
            Require(carLiveReplayResourceLoaded(nis)==CAR_NIS_OK,"NIS loaded callback failed");
        else if (completion==ResourceFileCompletion::CarLoaded)carCompletion->LoadingDoneCallback();
        else if (completion==ResourceFileCompletion::FrontendLoaded)
            GameFlowLoadingFrontEndPart1(*flow,*gameFlow);
        else if (completion==ResourceFileCompletion::TrackCommonLoaded)
            GameFlowLoadingTrackCommonPart1(*flow,*gameFlow);
        else if (completion==ResourceFileCompletion::TrackSpecificLoaded)
            GameFlowLoadingTrackSpecificPart1(*flow,*gameFlow);
        else if (completion==ResourceFileCompletion::TrackPreRaceLoaded)
            GameFlowLoadingTrackSpecificPart4(*flow,*gameFlow);
        else if (completion==ResourceFileCompletion::TrackLocationTexturesLoaded)
            GameFlowLoadingTrackSpecificPart5(*flow,*gameFlow);
        else if (completion==ResourceFileCompletion::TrackDriverTexturesLoaded)
            GameFlowLoadingTrackSpecificPart6(*flow,*gameFlow);
        return service.pendingFiles;
    }
    Require(service.pendingFiles==0,"Outstanding ResourceFile count has no live owner");
    return 0;
}

bool ResourceLoader::IsResourceLoadingComplete() const noexcept { return storage_->pendingFiles==0; }
bool ResourceLoader::HasPendingFileReads() const noexcept { return !storage_->queued.Empty(); }
bool ResourceLoader::ShuttingDown() const noexcept { return storage_->shuttingDown; }

void ResourceLoader::BeginShutdown() noexcept
{
    auto& service=*storage_;service.shuttingDown=true;
    for(auto& owner:service.ordered) {
        auto& file=*owner->storage_;file.completion=ResourceFileCompletion::None;
        file.bindings.nis=nullptr;file.bindings.carLoader=nullptr;
        file.bindings.flow=nullptr;file.bindings.gameFlow=nullptr;
    }
    carLoader.BeginShutdown(*this);
    service.queued.BeginShutdown();
}

std::exception_ptr ResourceLoader::Shutdown() noexcept
{
    auto& service=*storage_;
    if(service.shutdownComplete)return service.shutdownFailure;
    BeginShutdown();
    const auto remember=[&](std::exception_ptr error) noexcept {
        if(!service.shutdownFailure)service.shutdownFailure=error;
    };
    remember(carLoader.Shutdown(*this));
    // Child providers must disappear before GLOBALB. Continue independent
    // retirements after an error, while every provider binding is still live.
    for(int globals=0;globals!=2;++globals) {
        for(auto it=service.ordered.end();it!=service.ordered.begin();) {
            --it;auto* file=it->get();
            if((file->storage_->bindings.kind==ResourceFileKind::Global)!=(globals!=0))continue;
            if(file->storage_->failure)remember(file->storage_->failure);
            try {
                const auto next=std::next(it);
                RetireResourceFile(file,true);it=next;
            } catch(...) {remember(std::current_exception());}
        }
    }
    remember(service.queued.Shutdown());
    service.shutdownComplete=service.ordered.empty() && service.queued.Empty();
    return service.shutdownFailure;
}

void ResourceLoader::DetachFlowCompletions(const ApplicationFrontendFlow& flow) noexcept
{
    for (const auto& owner:storage_->ordered) {
        auto& file=*owner->storage_;
        if (file.bindings.flow!=&flow)continue;
        file.completion=ResourceFileCompletion::None;
        file.bindings.flow=nullptr;file.bindings.gameFlow=nullptr;
    }
}

void ResourceLoader::UnloadResourceFile(ResourceFile* file)
{
    RetireResourceFile(file,storage_->shuttingDown);
}

void ResourceLoader::CancelResourceFile(ResourceFile* file)
{
    RetireResourceFile(file,storage_->shuttingDown);
}

void ResourceLoader::RetireResourceFile(ResourceFile* file,bool terminal)
{
    if (!file)return;
    auto& service=*storage_;
    const auto found=std::find_if(service.ordered.begin(),service.ordered.end(),
        [file](const auto& owner){return owner.get()==file;});
    Require(found!=service.ordered.end(),"ResourceFile does not belong to this service");

    auto& data=*file->storage_;
    data.completion=ResourceFileCompletion::None;
    data.bindings.nis=nullptr;data.bindings.carLoader=nullptr;
    data.bindings.flow=nullptr;data.bindings.gameFlow=nullptr;
    // Pending unload cancels publication instead of decoding a dying file
    // or calling its completion. Join the real write before freeing bytes.
    if(data.queuedId) {
        service.queued.CancelAndWait(data.queuedId);data.queuedId=0;data.transfersPending=0;
    }
    Require(!data.queuedId && !data.transfersPending,"ResourceFile cannot retire an outstanding physical write");
    logResourceEvent("resource.unload.begin",data.filename,data.bindings.kind,data.type,data.flags,
        0,data.physical.range.Size(),service.pendingFiles);
    data.animationData.reset();
    if (data.frontendData) {data.frontendData->Release();data.frontendData.reset();}
    if (data.globalData) {
        for (const auto& other:service.ordered)
            Require(other.get()==file || other->storage_->bindings.kind==ResourceFileKind::Global,
                "Dependent Car/Track/NIS resources must retire before GLOBALB");
        data.globalData->Release();data.globalData.reset();
    }
    data.animationData.reset();
    data.positionMarkers.reset();data.damage.reset();data.collisionObjects.reset();data.modelData.reset();
    if(data.trackData) {
        if(terminal)data.trackData->ReleaseForShutdown(service.shutdownFailure);
        else data.trackData->Release();
    }
    if (data.trackCameraGroups)
        Require(carViewCameraResourcesUnloadTrackGroups(data.bindings.cameras,data.trackCameraGroups),
            "Track camera group retirement failed");
    data.trackCameraGroups=0;data.trackData.reset();
    data.resources={};
    if(data.cameraData) {
        // Commit would publish still-pending takes during failed-load
        // cleanup. Revoke only this physical file's pending/published owner.
        carViewCameraResourcesRetireReplayFile(data.bindings.cameras,&service.files,
            data.cameraIdentity.c_str(),data.bindings.cameraGroup);
        data.cameraData=false;data.cameraIdentity.clear();
    }
    std::vector<std::uint8_t>().swap(data.data);
    if(!terminal)service.registry.PostLoadFixup();
    if (!data.loaded) {
        Require(service.pendingFiles!=0,"Failed ResourceFile count underflow");
        --service.pendingFiles;
    }

    logResourceEvent("resource.unloaded",data.filename,data.bindings.kind,data.type,data.flags,
        0,data.physical.range.Size(),service.pendingFiles);
    service.ordered.erase(found);
}

ResourceFile* ResourceLoader::FindResourceFile(const char* filename) const noexcept
{
    if (!filename)return nullptr;
    for (auto it=storage_->ordered.rbegin();it!=storage_->ordered.rend();++it)
        if ((*it)->Filename()==filename)return it->get();
    return nullptr;
}

ResourceFile* ResourceLoader::FindResourceFile(std::int32_t type) const noexcept
{
    for (auto it=storage_->ordered.rbegin();it!=storage_->ordered.rend();++it)
        if ((*it)->Type()==type)return it->get();
    return nullptr;
}

std::int32_t ResourceLoader::GetResourceFileSize(std::int32_t type,std::uint32_t kind,std::int32_t* chunks) const
{
    std::uint32_t bytes=0,count=0;
    for (auto it=storage_->ordered.rbegin();it!=storage_->ordered.rend();++it)if ((*it)->Type()==type) {
        std::int32_t number=0;
        bytes+=static_cast<std::uint32_t>((*it)->GetSize(kind,&number));
        count+=static_cast<std::uint32_t>(number);
    }
    std::int32_t result;
    std::memcpy(&result,&bytes,sizeof(result));if (chunks)std::memcpy(chunks,&count,sizeof(count));
    return result;
}
}
