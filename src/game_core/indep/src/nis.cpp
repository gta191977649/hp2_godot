#include <exception>
#include "indep/inc/NIS.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bChunk.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/CollisionBody.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bFile.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/AnimatedDriver.hpp"

#include <cstring>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <stdexcept>
#include <utility>
#include <cmath>
#include <algorithm>
#include <array>

struct CarLiveReplayRecords {
    const hp2::NisResources* resources{};
    CarReplayAnimationRegistry* animations{};
    CarViewCameraResources* cameras{};
    EditorMarkerResources* models{};
    hp2::ResourceLoader* loader{};
    int32_t loadedIndex{-1};
    int32_t pendingIndex{-1};
    int32_t queuedIndex{-1};
    hp2::ResourceFile* animationFile{};
    hp2::ResourceFile* cameraFile{};
    std::exception_ptr failure;
};

static CarNisResult nisWaitForResourceLoad(const CarFrameLiveReplay* replay)
{
    if (!replay || !replay->records || !replay->records->loader) return CAR_NIS_INVALID_BINDING;
    try {
        while(replay->records->pendingIndex>=0) {
            if(replay->records->failure || replay->records->loader->ShuttingDown())
                return CAR_NIS_RESOURCE_FAILURE;
            replay->records->loader->ServiceResourceLoading();
        }
        if(replay->records->failure)return CAR_NIS_RESOURCE_FAILURE;
        return CAR_NIS_OK;
    } catch(...) {
        if(!replay->records->failure)replay->records->failure=std::current_exception();
        return CAR_NIS_RESOURCE_FAILURE;
    }
}

static CollisionShape* nisCarCollision(const CarMovement* movement)
{
    return movement && movement->bindings.ownerMotion &&
        movement->bindings.ownerMotion->collision ?
        movement->bindings.ownerMotion->collision->shape : nullptr;
}

CarNisResult carLiveReplaySetupCars(CarFrameLiveReplay* replay)
{
    if (!replay || !replay->cameraContext) return CAR_NIS_INVALID_BINDING;
    CarLiveReplayContext* context = replay->cameraContext;
    CarMovement* cars[2] = {context->first, context->second};
    int32_t* savedModes[2] = {&context->savedFirstMode, &context->savedSecondMode};
    for (int32_t actor = 0; actor < 2; ++actor) {
        CarMovement* car = cars[actor];
        if (!car) continue;
        CollisionShape* collision = nisCarCollision(car);
        if (!car->bindings.state || !collision) return CAR_NIS_INVALID_BINDING;
        if (car->mode != 0) *savedModes[actor] = car->mode;
        vehicleStateResetForSound(car->bindings.state);
        if (carMovementSetMode(car, 0) != CAR_MOVEMENT_OK)
            return CAR_NIS_MOVEMENT_FAILURE;
        collisionShapeLock(collision);
    }
    return CAR_NIS_OK;
}

CarNisResult carLiveReplayStartAnimating(CarFrameLiveReplay* replay)
{
    if (!replay || !replay->cameraContext || !replay->records)
        return CAR_NIS_INVALID_BINDING;
    CarLiveReplayContext* context = replay->cameraContext;
    CarLiveReplayRecords* records = replay->records;
    const CarNisInfo* info = replay->infoIndex < 0 ? nullptr :
        records->resources->Info(static_cast<size_t>(replay->infoIndex));
    if (!info || !records->models || !records->animations || !context->registry ||
        !context->first || !context->second || !context->first->bindings.owner ||
        !context->second->bindings.owner || !context->first->bindings.environment ||
        !context->first->bindings.environment->contact.terrain)
        return CAR_NIS_INVALID_BINDING;
    const PhysicsCarInfo* carInfo = context->first->bindings.info;
    if (info->selectionGroup && (!carInfo || !carInfo->driverAnimationOffset))
        return CAR_NIS_INVALID_BINDING;
    CarReplayPresentationState* presentation = carViewMoverPresentation(
        context->registry, context->first->bindings.owner);
    if (!presentation) return CAR_NIS_INVALID_BINDING;

    context->animating = 1;
    for (int32_t actor = 0; actor < 2; ++actor) {
        editorMarkerModelInitialize(&context->actorModels[actor], records->models,
            info->actorModelKeys[actor]);
        editorMarkerModelInitialize(&context->actorModels[actor + 2], records->models,
            info->actorModelKeys[actor + 2]);
    }
    const float height = terrainSurfaceElevation(
        context->first->bindings.environment->contact.terrain,
        presentation->centerPosition, nullptr, nullptr, nullptr);
    const float verticalOffset = -(presentation->centerPosition[2] - height);
    context->groundHeight = height;

    CarDriverVisual* firstVisual = context->first->bindings.owner->visual;
    if (firstVisual) {
        carDriverVisualSetReplaySuppressed(firstVisual, 1);
        context->firstDriver = carDriverVisualAnimation(firstVisual);
        (void)carReplayAnimationFind(&records->animations->root, info->firstAnimationKey);
    } else {
        context->firstDriver = nullptr;
    }
    if (info->selectionGroup) {
        /* Literal PAL GP-7178/GP-717C, separately verified against beta.
         * These are animation offsets; the physical center of mass is110. */
        context->firstActorOffset[0] = carInfo->driverAnimationOffset[0] - 0.1589999943971634f;
        context->firstActorOffset[1] = carInfo->driverAnimationOffset[1] - 0.03099999949336052f;
        context->firstActorOffset[2] = carInfo->driverAnimationOffset[2] - 0.03099999949336052f;
        context->firstActorOffset[3] = 0.0f;
    } else {
        context->firstActorOffset[0] = 0.0f;
        context->firstActorOffset[1] = 0.0f;
        context->firstActorOffset[2] = verticalOffset;
        context->firstActorOffset[3] = 0.0f;
    }
    CarDriverVisual* secondVisual = context->second->bindings.owner->visual;
    if (secondVisual) {
        carDriverVisualSetReplaySuppressed(secondVisual, 1);
        context->secondDriver = carDriverVisualAnimation(secondVisual);
        (void)carReplayAnimationFind(&records->animations->root, info->secondAnimationKey);
    } else {
        context->secondDriver = nullptr;
    }
    context->secondActorOffset[0] = 0.0f;
    context->secondActorOffset[1] = 0.0f;
    context->secondActorOffset[2] = verticalOffset;
    context->secondActorOffset[3] = 0.0f;
    if (info->selectionGroup)
        context->secondActorOffset[0] += context->firstActorOffset[0];
    return CAR_NIS_OK;
}

CarNisResult carLiveReplayStart(CarFrameLiveReplay* replay, int32_t index,
    int32_t take, CarMovement* first, CarMovement* second)
{
    if (!replay || !replay->records || !replay->cameraContext ||
        !replay->cameraContext->registry || !first || !second)
        return CAR_NIS_INVALID_BINDING;
    CarLiveReplayContext* context = replay->cameraContext;
    const CarCinematicServices* services = context->registry->environment.services;
    const CarNisInfo* info = index < 0 ? nullptr :
        replay->records->resources->Info(static_cast<size_t>(index));
    if (!services || !services->clock) return CAR_NIS_INVALID_BINDING;
    if (!info || take < 0 || take >= info->takeCount) return CAR_NIS_INVALID_RECORD;

    context->startedWorldTicks = services->clock->worldTimeFrames;
    context->takeIndex = take;
    replay->active = 1;
    replay->infoIndex = index;
    context->first = first;
    context->second = second;
    const uint32_t expiry = static_cast<uint32_t>(context->startedWorldTicks) +
        static_cast<uint32_t>(info->frameCount) * 2u;
    std::memcpy(&replay->expiresWorldFrame, &expiry, sizeof(expiry));
    CarNisResult result = carLiveReplayEnsureResourcesLoaded(replay);
    if (result != CAR_NIS_OK) return result;
    result = carLiveReplaySetupCars(replay);
    if (result != CAR_NIS_OK) return result;
    return carLiveReplayStartAnimating(replay);
}

// PAL1B2C70..1B2C8C: authored NIS frames are half the same world-frame
// counter used by Start/expiry, including the source signed division.
int32_t carLiveReplayFrameNumber(const CarFrameLiveReplay* replay)
{
    hp2::Require(replay && replay->cameraContext && replay->cameraContext->registry &&
        replay->cameraContext->registry->environment.services &&
        replay->cameraContext->registry->environment.services->clock,
        "NIS frame has no canonical camera/world clock owner");
    const auto& context=*replay->cameraContext;
    const uint32_t bits=static_cast<uint32_t>(context.registry->environment.services->clock->worldTimeFrames)-
        static_cast<uint32_t>(context.startedWorldTicks);
    int32_t elapsed;std::memcpy(&elapsed,&bits,sizeof(elapsed));
    return elapsed/2;
}

// PAL1B13F0..1B1510. Called by the actual visible NIS renderer each frame;
// StartAnimating alone cannot keep the source actor phase/ground current.
CarNisResult carLiveReplaySetupForRendering(CarFrameLiveReplay* replay)
{
    if (!replay || !replay->cameraContext)return CAR_NIS_INVALID_BINDING;
    auto& context=*replay->cameraContext;
    const auto* info=carLiveReplayCurrentInfo(replay);
    if (!info || !context.first || !context.first->bindings.owner ||
        !context.first->bindings.environment || !context.first->bindings.environment->contact.terrain ||
        !context.registry)return CAR_NIS_INVALID_BINDING;
    const auto* presentation=carViewMoverPresentation(context.registry,context.first->bindings.owner);
    if (!presentation)return CAR_NIS_INVALID_BINDING;
    context.groundHeight=terrainSurfaceElevation(context.first->bindings.environment->contact.terrain,
        presentation->centerPosition,nullptr,nullptr,nullptr);
    if (info->selectionGroup) {
        const auto* carInfo=context.first->bindings.info;
        if (!carInfo || !carInfo->driverAnimationOffset)return CAR_NIS_INVALID_BINDING;
        context.firstActorOffset[0]=carInfo->driverAnimationOffset[0]-0.1589999943971634f;
        context.firstActorOffset[1]=carInfo->driverAnimationOffset[1]-0.03099999949336052f;
        context.firstActorOffset[2]=carInfo->driverAnimationOffset[2]-0.03099999949336052f;
        context.firstActorOffset[3]=0.0f;
    }
    const auto sample=std::min(carLiveReplayFrameNumber(replay),info->frameCount-1);
    if (context.firstDriver)carReplayAnimationSelect(context.firstDriver,info->firstAnimationKey,
        static_cast<float>(sample));
    if (context.secondDriver)carReplayAnimationSelect(context.secondDriver,info->secondAnimationKey,
        static_cast<float>(sample));
    // Original PlaySoundEvents1B0FD8 is an empty function.
    return CAR_NIS_OK;
}

int32_t carLiveReplayCurrentInfoIndex(const CarFrameLiveReplay* replay)
{
    if (!replay || !replay->records) return -1;
    if (replay->active) return replay->infoIndex;
    if (replay->records->queuedIndex != -1) return replay->records->queuedIndex;
    if (replay->records->pendingIndex != -1) return replay->records->pendingIndex;
    return replay->records->loadedIndex;
}

const CarNisInfo* carLiveReplayCurrentInfo(const CarFrameLiveReplay* replay)
{
    if (!replay || !replay->records || replay->infoIndex < 0) return nullptr;
    return replay->records->resources->Info(static_cast<size_t>(replay->infoIndex));
}

CarNisResult carLiveReplayRestoreCars(CarFrameLiveReplay* replay)
{
    if (!replay || !replay->cameraContext) return CAR_NIS_INVALID_BINDING;
    CarLiveReplayContext* context = replay->cameraContext;
    if (context->first) {
        CollisionShape* collision = nisCarCollision(context->first);
        if (!collision) return CAR_NIS_INVALID_BINDING;
        if (carMovementSetMode(context->first, context->savedFirstMode) != CAR_MOVEMENT_OK)
            return CAR_NIS_MOVEMENT_FAILURE;
        collisionShapeUnlock(collision);
    }
    if (context->second) {
        CollisionShape* collision = nisCarCollision(context->second);
        if (!collision) return CAR_NIS_INVALID_BINDING;
        if (carMovementSetMode(context->second, context->savedSecondMode) != CAR_MOVEMENT_OK)
            return CAR_NIS_MOVEMENT_FAILURE;
        collisionShapeUnlock(collision);
    }
    return CAR_NIS_OK;
}

CarNisResult carLiveReplayStopAnimating(CarFrameLiveReplay* replay)
{
    if (!replay || !replay->cameraContext) return CAR_NIS_INVALID_BINDING;
    CarLiveReplayContext* context = replay->cameraContext;
    if (!context->animating) return CAR_NIS_OK;
    if (!context->first || !context->second || !context->first->bindings.owner ||
        !context->second->bindings.owner) return CAR_NIS_INVALID_BINDING;
    context->animating = 0;
    CarDriverVisual* first = context->first->bindings.owner->visual;
    if (first) carDriverVisualSetReplaySuppressed(first, 0);
    context->firstDriver = nullptr;
    CarDriverVisual* second = context->second->bindings.owner->visual;
    if (second) carDriverVisualSetReplaySuppressed(second, 0);
    context->secondDriver = nullptr;
    /* Actual order is E0,110,F8,128: two paired model arrays, not a single
     * four-element source object or copied visual controller. */
    for (int32_t actor = 0; actor < 2; ++actor) {
        editorMarkerModelDispose(&context->actorModels[actor]);
        editorMarkerModelDispose(&context->actorModels[actor + 2]);
    }
    return CAR_NIS_OK;
}

CarNisResult carLiveReplayStop(CarFrameLiveReplay* replay)
{
    if (!replay) return CAR_NIS_INVALID_BINDING;
    if (!replay->active) return CAR_NIS_OK;
    CarNisResult result = carLiveReplayStopAnimating(replay);
    if (result != CAR_NIS_OK) return result;
    result = carLiveReplayRestoreCars(replay);
    if (result != CAR_NIS_OK) return result;
    CarLiveReplayContext* context = replay->cameraContext;
    context->second = nullptr;
    context->takeIndex = -1;
    replay->active = 0;
    context->startedWorldTicks = 0;
    replay->expiresWorldFrame = 0;
    replay->infoIndex = -1;
    context->first = nullptr;
    return CAR_NIS_OK;
}

CarNisResult carLiveReplayEnsureResourcesLoaded(CarFrameLiveReplay* replay)
{
    if (!replay || !replay->records) return CAR_NIS_INVALID_BINDING;
    if (replay->records->loadedIndex == replay->infoIndex) return CAR_NIS_OK;
    const CarNisResult result = carLiveReplayLoad(replay, replay->infoIndex);
    if (result != CAR_NIS_OK) return result;
    return nisWaitForResourceLoad(replay);
}

CarNisResult carLiveReplayReleaseRecordBuffers(CarFrameLiveReplay* replay)
{
    if(!replay || !replay->records || !replay->records->loader)return CAR_NIS_INVALID_BINDING;
    CarLiveReplayRecords* records = replay->records;
    records->pendingIndex=records->queuedIndex=-1;
    bool failed=false;
    const auto release=[&](hp2::ResourceFile*& file) {
        if(!file)return;
        try {records->loader->CancelResourceFile(file);file=nullptr;}
        catch(...) {
            failed=true;if(!records->failure)records->failure=std::current_exception();
        }
    };
    // An incomplete pair must not run NisLoaded. Retire both members even
    // when one release fails, leaving any remainder with its real loader.
    release(records->animationFile);release(records->cameraFile);
    if(!records->animationFile && !records->cameraFile)records->loadedIndex=-1;
    return failed ? CAR_NIS_RESOURCE_FAILURE:CAR_NIS_OK;
}

CarNisResult carLiveReplayLoad(CarFrameLiveReplay* replay, int32_t index)
{
    if (!replay || !replay->records || !replay->cameraContext)
        return CAR_NIS_INVALID_BINDING;
    CarLiveReplayRecords* records = replay->records;
    if(records->loader->ShuttingDown())return CAR_NIS_RESOURCE_FAILURE;
    if(records->failure) {
        // The failed attempt has no future producer. Do not report OK for
        // its old pending index or enqueue a successor behind it.
        (void)carLiveReplayReleaseRecordBuffers(replay);
        return CAR_NIS_RESOURCE_FAILURE;
    }
    if (records->loadedIndex == index || records->pendingIndex == index)
        return CAR_NIS_OK;
    if (records->pendingIndex >= 0) {
        records->queuedIndex = index;
        return CAR_NIS_OK;
    }
    if(records->loadedIndex>=0 || records->animationFile || records->cameraFile) {
        const CarNisResult released = carLiveReplayReleaseRecordBuffers(replay);
        if (released != CAR_NIS_OK) return released;
    }
    const CarNisInfo* info = index < 0 ? nullptr : records->resources->Info((size_t)index);
    if (!info) return CAR_NIS_INVALID_RECORD;
    records->failure=nullptr;
    try {
        const std::string prefix = std::string("NIS/") + info->name;
        records->pendingIndex = index;
        hp2::ResourceFileBindings bindings{records->cameras,replay,replay->cameraContext->groupIndex};
        records->animationFile=records->loader->LoadResourceFile((prefix+".bin").c_str(),4,0,bindings);
        records->cameraFile=records->loader->LoadResourceFile((prefix+".cam").c_str(),4,0,bindings,
            hp2::ResourceFileCompletion::NisLoaded);
        return CAR_NIS_OK;
    } catch (...) {
        records->failure=std::current_exception();
        (void)carLiveReplayReleaseRecordBuffers(replay);
        return CAR_NIS_RESOURCE_FAILURE;
    }
}

CarNisResult carLiveReplayResourceLoaded(CarFrameLiveReplay* replay)
{
    if (!replay || !replay->records || !replay->cameraContext)
        return CAR_NIS_INVALID_BINDING;
    auto* records=replay->records;
    if(records->pendingIndex<0 || records->loader->ShuttingDown())return CAR_NIS_RESOURCE_FAILURE;
    if (!carViewCameraResourcesCommitReplayGroup(records->cameras,replay->cameraContext->groupIndex))
        return CAR_NIS_RESOURCE_FAILURE;
    records->loadedIndex=records->pendingIndex;
    records->pendingIndex=-1;
    if (records->queuedIndex>=0) {
        const int32_t queued=records->queuedIndex;
        records->queuedIndex=-1;
        return carLiveReplayLoad(replay,queued);
    }
    return CAR_NIS_OK;
}

namespace hp2 {

NisResources::NisResources(const GameResources& resources)
    : NisResources(resources,resources.Load("GLOBAL/GLOBALB.LZC"))
{
}

NisResources::NisResources(const GameResources& resources,const std::vector<std::uint8_t>& globalBundle)
    : resources_(resources), global_(globalBundle)
{
    bool found = false;
    for (const auto& chunk : ReadChunks(global_)) {
        if (chunk.id != 0x37010u) continue;
        Require(!found, "More than one NIS info catalogue");
        found = true;
        Require(chunk.size % 160u == 0, "Partial NIS info record");
        for (std::size_t offset = chunk.payloadOffset; offset < chunk.endOffset; offset += 160u) {
            const auto* bytes = global_.data() + offset;
            CarNisInfo info{};
            std::memcpy(info.name, bytes, sizeof(info.name));
            const auto* end = static_cast<const char*>(std::memchr(info.name, 0, sizeof(info.name)));
            Require(end && end != info.name, "Invalid NIS resource name");
            Require(std::string(info.name).find_first_of("/\\:") == std::string::npos,
                "NIS resource name contains a path component");
            const auto signedWord = [&](std::size_t field) {
                const uint32_t raw = ReadU32(bytes + field);
                int32_t value;
                std::memcpy(&value, &raw, sizeof(value));
                return value;
            };
            info.kind = signedWord(32);
            info.selectionGroup = signedWord(36);
            info.authoredIndex = signedWord(40);
            info.frameCount = signedWord(44);
            info.takeCount = signedWord(48);
            info.firstAnimationKey = ReadU32(bytes + 52);
            info.secondAnimationKey = ReadU32(bytes + 56);
            for (size_t model = 0; model < 4; ++model)
                info.actorModelKeys[model] = ReadU32(bytes + 80u + model * 4u);
            info.authoredRecord = bytes;
            info.firstBodyTypeCount = signedWord(64);
            info.secondBodyTypeCount = signedWord(68);
            Require(info.firstBodyTypeCount >= 0 && info.firstBodyTypeCount <= 8 &&
                info.secondBodyTypeCount >= 0 && info.secondBodyTypeCount <= 8,
                "NIS body-key count exceeds its actual record extent");
            for (size_t body = 0; body < 8; ++body) {
                info.bodyKeys[0][body] = ReadU32(bytes + 96u + body * 4u);
                info.bodyKeys[1][body] = ReadU32(bytes + 128u + body * 4u);
            }
            Require(info.frameCount > 0 && info.takeCount > 0 && info.takeCount <= 8,
                "NIS info has invalid frame/take counts");
            infos_.push_back(info);
        }
    }
    Require(found, "GLOBALB has no NIS info catalogue");
}

const CarNisInfo* NisResources::Info(std::size_t index) const noexcept
{
    return index < infos_.size() ? &infos_[index] : nullptr;
}

const CarNisInfo* NisResources::InfoByKind(std::size_t ordinal, int32_t kind) const noexcept
{
    for (const CarNisInfo& info : infos_) {
        if (info.kind != kind) continue;
        if (ordinal == 0) return &info;
        --ordinal;
    }
    return nullptr;
}

std::size_t NisResources::CountKind(int32_t kind) const noexcept
{
    size_t count = 0;
    for (const CarNisInfo& info : infos_)
        if (info.kind == kind) ++count;
    return count;
}

bool NisResources::SupportsFirstBodyType(std::size_t index, uint32_t key) const noexcept
{
    const CarNisInfo* info = Info(index);
    if (!info) return false;
    for (int32_t body = 0; body < info->firstBodyTypeCount; ++body)
        if (info->bodyKeys[0][body] == key) return true;
    return false;
}

bool NisResources::SupportsSecondBodyType(std::size_t index, uint32_t key) const noexcept
{
    const CarNisInfo* info = Info(index);
    if (!info) return false;
    for (int32_t body = 0; body < info->secondBodyTypeCount; ++body)
        if (info->bodyKeys[1][body] == key) return true;
    return false;
}

bool NisResources::IsSuitable(std::size_t index, uint32_t firstKey, uint32_t secondKey,
    int32_t excludedIndex) const noexcept
{
    if (index == static_cast<size_t>(excludedIndex)) return false;
    if (!SupportsSecondBodyType(index, secondKey)) return false;
    return SupportsFirstBodyType(index, firstKey);
}

std::size_t NisResources::Count() const noexcept
{
    return infos_.size();
}

const GameResources& NisResources::Files() const noexcept
{
    return resources_;
}

const NisResources* NisResourceCatalogue(const CarFrameLiveReplay& replay) noexcept
{
    return replay.records ? replay.records->resources : nullptr;
}

CarNisResult BindNisResources(CarFrameLiveReplay& replay,
    const NisResources& resources, ResourceLoader& loader,
    CarViewMoverRegistry& views, int32_t group)
{
    if (replay.records || group < 0 || group > 1 || replay.active)
        return CAR_NIS_INVALID_BINDING;
    if (!views.environment.resources || &loader.Files()!=&resources.Files()) return CAR_NIS_INVALID_BINDING;
    try {
        auto records = std::make_unique<CarLiveReplayRecords>();
        records->resources = &resources;
        records->animations = &loader.Registry().Animations();
        records->cameras = views.environment.resources;
        records->models = &loader.Registry().Models();
        records->loader = &loader;
        if (!replay.cameraContext) {
            replay.cameraContext = static_cast<CarLiveReplayContext*>(
                std::calloc(1, sizeof(CarLiveReplayContext)));
            if (!replay.cameraContext) return CAR_NIS_RESOURCE_FAILURE;
            replay.cameraContext->registry = &views;
        } else if (replay.cameraContext->registry != &views) {
            return CAR_NIS_INVALID_BINDING;
        }
        replay.cameraContext->groupIndex = group;
        replay.infoIndex = -1;
        replay.records = records.release();
        return CAR_NIS_OK;
    } catch (...) {
        return CAR_NIS_RESOURCE_FAILURE;
    }
}

CarNisResult UnbindNisResources(CarFrameLiveReplay& replay)
{
    if (!replay.records) return CAR_NIS_OK;
    if (replay.cameraContext && replay.cameraContext->activeViews)
        return CAR_NIS_INVALID_BINDING;
    CarNisResult result = carLiveReplayStop(&replay);
    if (result != CAR_NIS_OK) return result;
    result = carLiveReplayReleaseRecordBuffers(&replay);
    if (result != CAR_NIS_OK) return result;
    delete replay.records;
    replay.records = nullptr;
    return CAR_NIS_OK;
}

}


CarReplayAnimationController* carDriverVisualAnimation(CarDriverVisual* visual)
{ return visual?visual->animations[0]:NULL; }


void carDriverVisualSetReplaySuppressed(CarDriverVisual* visual,int32_t suppressed)
{ visual->replaySuppressed=suppressed; }


static const CarViewMoverEnvironment* nisViewEnvironment(const CarLiveReplayContext* context)
{
    return &context->registry->environment;
}

static CarReplayPresentationState* nisViewPresentation(CarLiveReplayContext* context,
    const CarMovement* movement)
{
    return carViewMoverPresentation(context->registry, movement->bindings.owner);
}

static const CarDriver* nisViewDriver(const CarLiveReplayContext* context,
    const CarMovement* movement)
{
    const WorldSimulation* simulation = nisViewEnvironment(context)->simulation;
    for (int32_t index = 0; index < simulation->world->totalCars; ++index)
        if (simulation->world->carTable[index] == movement->bindings.owner &&
            static_cast<size_t>(index) < simulation->frameCount)
            return simulation->drivers[index];
    return nullptr;
}



CarViewMoverResult carLiveReplaySetViewMasks(struct CarLiveReplayContext* context,int32_t viewNumber,
    CarMovement* first,CarMovement* second,int32_t enabled)
{
    CarViewMoverRegistry* registry=context->registry;
    const WorldSimulation* simulation=registry->environment.simulation;
    const uint32_t mask=1u<<viewNumber;
    CarReplayPresentationState* selected[2]={first?nisViewPresentation(context,first):NULL,
        second?nisViewPresentation(context,second):NULL};
    if ((first&&!selected[0]) || (second&&!selected[1])) return CAR_VIEW_MOVER_INVALID_BINDING;
    for (int selectedIndex=0;selectedIndex<2;++selectedIndex) if (selected[selectedIndex]) {
        if (enabled) selected[selectedIndex]->motion->wheelExclusionMask|=mask;
        else selected[selectedIndex]->motion->wheelExclusionMask&=~mask;
    }
    for (int32_t index=0;index<simulation->world->totalCars;++index) {
        const Car* car=simulation->world->carTable[index];
        if ((first&&car==first->bindings.owner) || (second&&car==second->bindings.owner)) continue;
        CarReplayPresentationState* presentation=carViewMoverPresentation(registry,car);
        if (!presentation) return CAR_VIEW_MOVER_INVALID_BINDING;
        if (!enabled) presentation->motion->wheelEffectsFlags|=mask;else presentation->motion->wheelEffectsFlags&=~mask;
    }
    const CarFramePlayerRegistry* players=simulation->players;
    int32_t excluded=enabled;
    if (players->count>=2)
        for (int32_t index=0;index<players->count;++index)
            if (carFramePlayerCarViewMover(players->players[index])) excluded=0;
    const uint32_t otherMask=1u<<(viewNumber==2?1:2);
    for (int selectedIndex=0;selectedIndex<2;++selectedIndex) if (selected[selectedIndex]) {
        if (excluded) selected[selectedIndex]->motion->wheelInclusionMask|=otherMask;
        else selected[selectedIndex]->motion->wheelInclusionMask&=~otherMask;
        if (enabled) selected[selectedIndex]->motion->wheelInclusionMask&=~mask;
    }
    return CAR_VIEW_MOVER_OK;
}


CarViewMoverResult carLiveReplayUnregisterView(CarFrameLiveReplay* replay,CarViewMover* mover)
{
    if (!replay || !replay->cameraContext || !mover || mover->kind!=CAR_VIEW_MOVER_REPLAY ||
        mover->registry!=replay->cameraContext->registry) return CAR_VIEW_MOVER_INVALID_BINDING;
    struct CarLiveReplayContext* context=replay->cameraContext;
    CarViewMoverResult result=carLiveReplaySetViewMasks(context,mover->viewNumber,mover->state.replay.first,mover->state.replay.second,0);
    if (result!=CAR_VIEW_MOVER_OK) return result;
    context->activeViews&=~(1u<<mover->viewNumber);
    CarCinematicScreen* screen=&nisViewEnvironment(context)->services->screens[mover->viewNumber];
    // PAL1B24F4/1B24F8 retire the independent composition and colour requests.
    screen->replayComposition=0;screen->blackAndWhite=0;
    return CAR_VIEW_MOVER_OK;
}


CarViewMoverResult carLiveReplayRegisterView(CarFrameLiveReplay* replay,CarViewMover* mover)
{
    if (!replay || !replay->cameraContext || !mover || mover->kind!=CAR_VIEW_MOVER_REPLAY ||
        mover->registry!=replay->cameraContext->registry) return CAR_VIEW_MOVER_INVALID_BINDING;
    struct CarLiveReplayContext* context=replay->cameraContext;
    CarMovement* first=mover->state.replay.first;CarMovement* second=mover->state.replay.second;
    const CarDriver* driver=nisViewDriver(context,first);
    if (!first || !second || !driver || !first->bindings.info || !second->bindings.info ||
        !first->bindings.state->centerOfMass || !second->bindings.state->centerOfMass ||
        !first->bindings.recoveryPoints)
        return CAR_VIEW_MOVER_INVALID_BINDING;
    const CarNisInfo* info=carLiveReplayCurrentInfo(replay);
    if (!info) return CAR_VIEW_MOVER_MISSING_RESOURCE;
    if (context->firstDriver) carReplayAnimationSelect(context->firstDriver,info->firstAnimationKey,0);
    if (context->secondDriver) carReplayAnimationSelect(context->secondDriver,info->secondAnimationKey,0);
    CarRoadPreview preview{};
    const CarRecoveryGroup* group=carDriverCurrentGroup(driver,1);
    const CarRecoveryPose* road=carDriverCurrentPose(driver,1);
    (void)carRoadPreviewInitialize(&preview,first->bindings.recoveryPoints,group,road,
        first->bindings.state->position,first->routeLocation.direction,200.0f,250.0f,
        first->info.routeVehicleClass,0);
    float minimum[4]={-4,-4,0,0},maximum[4]={4,4,2,0};
    CarReplayAnimationController* animations[2]={context->firstDriver,context->secondDriver};
    for (int index=0;index<2;++index) if (animations[index]) {
        float low[4]={0},high[4]={0};carReplayAnimationBounds(animations[index],low,high);
        vectorBoundsExpandPoint3(minimum,maximum,low);vectorBoundsExpandPoint3(minimum,maximum,high);
    }
    const CarViewRecordedTake* take=mover->child.kind==CAR_VIEW_CHILD_RECORDED?
        carRecordedCameraTake(mover->child.owner.recorded):NULL;
    if (take) for (size_t index=0;index<take->poseCount;++index) {
        const float point[4]={take->poses[index].position[0],take->poses[index].position[1],take->poses[index].position[2],0};
        terrainBoundsExpandRadius3(minimum,maximum,point,1.0f);
    }
    if (preview.steps && preview.count>0) {
        const float requiredWidth=maximum[1]-minimum[1],requiredLength=maximum[0]-minimum[0];
        float bestWidth=0;int32_t best=0,start=0;
        for (;start<preview.count;++start) {
            float length=0;
            for (int32_t index=start;index<preview.count;++index) {
                const CarRecoveryPose* point=preview.steps[index].pose;
                const float width=point->halfWidths[0]+point->halfWidths[1];
                if (bestWidth<width) { bestWidth=requiredWidth;best=start; }
                if (width<requiredWidth) break;
                length+=point->segmentLength;
                if (8.0f<=length) break;
            }
            if (requiredLength<=length) break;
        }
        road=preview.steps[start==preview.count?best:start].pose;
    }
    carRoadPreviewDispose(&preview);
    if (!road) return CAR_VIEW_MOVER_MISSING_RESOURCE;
    const VehicleState* state=first->bindings.state;
    const float forward=road->direction[0]*state->orientation[0]+road->direction[1]*state->orientation[1];
    const int back=forward<0.0f;
    const float dx=state->position[0]-road->position[0],dy=state->position[1]-road->position[1];
    const int side=dx*road->direction[1]-dy*road->direction[0]<0.0f;
    uint16_t heading=vectorAtan2Angle(road->direction[0],road->direction[1]);
    if (back) heading=(uint16_t)(heading-32768);
    const float positiveMargin=fmaxf(4.0f,maximum[1]);
    const float negativeMargin=fmaxf(2.5f,-minimum[1]);
    const float firstMargin=back!=side?positiveMargin:negativeMargin;
    const float secondMargin=back==side?positiveMargin:negativeMargin;
    const float firstAlong=back?-minimum[0]:maximum[0];
    const float firstAcross=side?firstMargin-road->halfWidths[0]:road->halfWidths[1]-firstMargin;
    int32_t columns=convertFloatToInt32((road->halfWidths[0]+road->halfWidths[1]-firstMargin)-secondMargin);
    if (columns<1) columns=1;
    int32_t blocked=1;
    for (int row=0;row<5 && blocked;++row) {
        const float along=firstAlong+(float)row*(back?-1.0f:1.0f);
        for (int32_t column=0;column<columns && blocked;++column) {
            const float across=firstAcross+(float)column*(side?1.0f:-1.0f);
            float firstPoint[4],secondPoint[4],constrained[4];
            carLiveReplayPlaceFirst(context,road,firstPoint,first,along,across,heading);
            firstPoint[2]+=0.5f;
            TerrainSurface surface;terrainSurfaceInitialize(&surface,nisViewEnvironment(context)->terrain);
            /* Exact source radius and initial search distance are retained
             * below; both are independent mutable-free numeric constants. */
            terrainSurfaceUpdate(&surface,firstPoint,13.600000381469727f,NULL);
            float reduction=0;int32_t turns=0,sign=1,angleStep=1000;
            uint16_t candidateHeading=heading;
            for (;;) {
                const float distance=7.599999904632568f-reduction;
                carLiveReplayPlaceSecond(context,secondPoint,second,firstPoint,heading,candidateHeading,distance+3.0f);
                memcpy(constrained,secondPoint,sizeof(constrained));
                blocked=terrainSurfaceConstrainSegment(&surface,1,true,firstPoint,constrained,0.5f,1.0f);
                if (!blocked) carLiveReplayPlaceSecond(context,secondPoint,second,firstPoint,heading,candidateHeading,distance);
                if (distance<6.0f) {
                    reduction=0;blocked=1;
                    candidateHeading=(uint16_t)(candidateHeading+sign*angleStep);
                    sign=-sign;angleStep+=1000;++turns;
                }
                if (turns>=66) {
                    carLiveReplayPlaceSecond(context,secondPoint,second,firstPoint,heading,heading,6.0f);break;
                }
                if (!blocked) break;
                reduction+=1.0f;
            }
            blocked=carLiveReplayPlacementBlocked(context,first,second);
            terrainSurfaceDispose(&surface);
        }
    }
    CarViewMoverResult result=carLiveReplaySetViewMasks(context,mover->viewNumber,first,second,1);
    if (result!=CAR_VIEW_MOVER_OK) return result;
    context->activeViews|=1u<<mover->viewNumber;
    CarCinematicScreen* screen=&nisViewEnvironment(context)->services->screens[mover->viewNumber];
    // PAL1B2440/1B2444 publish eView+6 and +74. The latter is consumed
    // by AttachBlackAndWhite; it is not a replay layer or a new random draw.
    screen->replayComposition=1;screen->blackAndWhite=1;
    return CAR_VIEW_MOVER_OK;
}


namespace hp2 {
// Original writable hand offsets334250/334260; static initializer1B3C70
// clears XYZ in both. These are shared source controls, not actor positions.
float nisHandOffsets[2][3]{};

static void nisAnimationTranslation(const CarReplayAnimationController& animation,float output[3])
{
    const auto* clip=animation.primary;
    Require(clip && clip->framePositions && clip->frameCount,
        "NIS actor has no retained authored animation translation");
    // GetTranslation1A78E0 converts the source sample and resets an invalid
    // sample to0. SetupForRendering supplies an exact integer phase here.
    auto sample=convertFloatToInt32(animation.phase);
    if(sample<0 || sample>=clip->frameCount)sample=0;
    std::memcpy(output,clip->framePositions+static_cast<std::size_t>(sample)*4,3*sizeof(float));
}

template<class Submit>
static void nisRenderHandProp(CarLiveReplayContext& context,
    CarReplayAnimationController& animation,const float actor[16],const float translation[3],
    const std::vector<std::array<float,16>>& posed,std::size_t hand,bool first,
    Submit& submit)
{
    // PAL1B30A8..1B32FC: source posed-matrix25/38 is converted back from
    // the same model's inverse bind, then put in the actual actor frame.
    const std::size_t bone=hand==1 ? 38u:25u;
    if(posed.empty())return;
    const auto* tree=editorMarkerModelAnimationTree(animation.model);
    Require(tree && tree->authoredNodes && bone<posed.size() &&
        bone<static_cast<std::size_t>(tree->nodeCount),"NIS hand prop lost its source posed bone");
    float world[16],rotation[16],root[4],inverseBind[16],bind[16],handMatrix[16];
    std::memcpy(world,actor,sizeof(world));std::memcpy(rotation,actor,sizeof(rotation));
    rotation[12]=rotation[13]=rotation[14]=0.0f;
    eMulVector3(root,rotation,translation);
    for(std::size_t axis=0;axis<3;++axis)world[12+axis]+=root[axis];
    world[15]=1.0f;
    std::memcpy(inverseBind,tree->authoredNodes+bone*96,sizeof(inverseBind));
    carCameraInvertRigidTransform(bind,inverseBind);
    motionMatrixTranslate(bind,bind,nisHandOffsets[hand]);
    eMulMatrix(handMatrix,bind,posed[bone].data());
    eMulMatrix(world,handMatrix,world);
    auto& model=context.actorModels[(first?0u:2u)+hand];
    if(model.solid)submit((first?0u:3u)+hand+1u,model,world,nullptr);
}

template<class Submit>
static void visitNisPlayback(CarFrameLiveReplay& playback,int viewId,Submit&& submit,
    float worldFrameFraction=0.0f,bool interpolate=false)
{
    // PAL1B2C90..1B30A8: the actual NIS actor pointers and active-view mask
    // gate rendering. Both actors are authored relative to the first car;
    // the separately placed second car is not a second animation origin.
    Require(playback.cameraContext,"NIS rendering lost its retained manager context");
    auto& context=*playback.cameraContext;
    if(!context.firstDriver && !context.secondDriver)return;
    Require(viewId>=0 && viewId<32,"NIS render view is outside the source bit mask");
    if(!(context.activeViews&(1u<<viewId)))return;
    if(!context.first)return;
    Require(carLiveReplaySetupForRendering(&playback)==CAR_NIS_OK,
        "NIS setup lost its actual animation, car or terrain owner");
    const auto* info=carLiveReplayCurrentInfo(&playback);
    auto& movement=*context.first;
    Require(info && movement.bindings.state && movement.bindings.ownerMotion,
        "NIS rendering has no first-car physical pose");
    const bool overridePose=(movement.bindings.ownerMotion->wheelExclusionMask&(1u<<viewId))!=0;
    const auto* presentation=carViewMoverPresentation(context.registry,movement.bindings.owner);
    Require(presentation && presentation->motion==movement.bindings.ownerMotion,
        "NIS rendering override differs from the Max camera's Car owner");
    float source[16],ground[16];
    std::memcpy(source,overridePose?presentation->orientation:movement.bindings.state->orientation,sizeof(source));
    std::memcpy(source+12,overridePose?presentation->centerPosition:movement.bindings.state->centerPosition,3*sizeof(float));
    source[15]=1.0f;std::memcpy(ground,source,sizeof(ground));
    const float up[4]={0,0,1,0};
    std::memcpy(ground+8,up,sizeof(up));
    carCinematicCrossThree(ground+4,up,ground);
    carCinematicCrossThree(ground,ground+4,up);
    // Both NIS actors are rendered sequentially. Reuse the transient native
    // quaternion and bone arrays between them rather than reallocating two
    // arrays for each actor on every rendered frame.
    std::vector<std::array<float,4>> quaternions;
    std::vector<std::array<float,4>> nextQuaternions;
    DriverPoseCache cache;
    const auto drawActor=[&](CarReplayAnimationController* animation,const float offset[4],bool first) {
        if(!animation || !animation->primary || !animation->model || !animation->model->solid)return;
        float actor[16],displacement[4],translation[3];
        std::memcpy(actor,first && info->selectionGroup?source:ground,sizeof(actor));
        eMulVector4(displacement,source,offset);
        for(std::size_t axis=0;axis<4;++axis)actor[12+axis]+=displacement[axis];
        nisAnimationTranslation(*animation,translation);
        const auto* clip=animation->primary;
        const auto sample=static_cast<int32_t>(animation->phase);
        const auto next=std::min({sample+1,info->frameCount-1,static_cast<int32_t>(clip->frameCount)-1});
        const auto worldFrame=static_cast<uint32_t>(context.registry->environment.services->clock->worldTimeFrames);
        // The authored clip advances once per two world frames. Sample only
        // the interval ahead of the committed phase without advancing playback.
        const float blend=interpolate && next>sample ?
            (static_cast<float>((worldFrame-static_cast<uint32_t>(context.startedWorldTicks))&1u)+worldFrameFraction)*0.5f : 0.0f;
        if(blend>0.0f)
            for(std::size_t axis=0;axis<3;++axis)
                translation[axis]+=(clip->framePositions[static_cast<std::size_t>(next)*4+axis]-translation[axis])*blend;
        if(!first || !info->selectionGroup) {
            float position[4];eMulVector3(position,actor,translation);
            const auto height=terrainSurfaceElevation(movement.bindings.environment->contact.terrain,
                position,nullptr,nullptr,nullptr);
            actor[14]+=(height-context.groundHeight)+0.05000000074505806f;
        }
        const auto count=static_cast<std::size_t>(animation->primary->boneCount);
        const auto* tree=editorMarkerModelAnimationTree(animation->model);
        if(!tree || static_cast<std::size_t>(tree->nodeCount)!=count)return;
        quaternions.resize(count);
        auto* rotations=reinterpret_cast<float (*)[4]>(quaternions.data());
        Require(carDriverAnimationReadAllRotations(animation->primary,rotations,count,animation->phase)!=0,
            "NIS actor phase is outside its retained authored animation");
        if(blend>0.0f) {
            nextQuaternions.resize(count);
            auto* nextRotations=reinterpret_cast<float (*)[4]>(nextQuaternions.data());
            Require(carDriverAnimationReadAllRotations(clip,nextRotations,count,static_cast<float>(next))!=0,
                "NIS actor interpolation lacks its next authored pose");
            for(std::size_t bone=0;bone<count;++bone)
                vectorQuaternionSlerp(rotations[bone],rotations[bone],nextRotations[bone],blend);
        }
        const auto& posed=cache.Prepare(*tree,rotations,count);
        float world[16],rotation[16],root[4];
        std::memcpy(world,actor,sizeof(world));std::memcpy(rotation,actor,sizeof(rotation));
        rotation[12]=rotation[13]=rotation[14]=0.0f;
        eMulVector3(root,rotation,translation);
        for(std::size_t axis=0;axis<3;++axis)world[12+axis]+=root[axis];
        world[15]=1.0f;
        submit(first?0u:3u,*animation->model,world,&posed);
        nisRenderHandProp(context,*animation,actor,translation,posed,0,first,submit);
        nisRenderHandProp(context,*animation,actor,translation,posed,1,first,submit);
    };
    drawActor(context.firstDriver,context.firstActorOffset,true);
    drawActor(context.secondDriver,context.secondActorOffset,false);
}

std::size_t RenderNisPlayback(CarFrameLiveReplay& playback,
    const CarCinematicScreen& view,EcstasyFrame& frame)
{
    std::size_t draws=0;
    visitNisPlayback(playback,view.id,[&](std::size_t,const EditorMarkerModel& model,
        const float* transform,const std::vector<std::array<float,16>>* pose) {
        draws+=frame.RenderModel(const_cast<EditorMarkerModel&>(model),transform,view,0,1.0f,0,nullptr,pose);
    });
    return draws;
}

void VisitNisPoses(NisManagerRegistry& registry,int viewId,NisPoseSink sink,void* user,float worldFrameFraction)
{
    std::size_t base=0;
    for(auto* manager=registry.first;manager;manager=manager->Next(),base+=6)
        visitNisPlayback(manager->Playback(),viewId,[&](std::size_t slot,const EditorMarkerModel& model,
            const float* transform,const std::vector<std::array<float,16>>* pose) {
            sink(user,base+slot,model,transform,pose);
        },std::clamp(worldFrameFraction,0.0f,1.0f),true);
}

std::size_t DrawNisObjects(NisManagerRegistry& registry,
    const CarCinematicScreen& view,EcstasyFrame& frame)
{
    // PAL1B3300 follows the one intrusive manager list in creation order.
    std::size_t draws=0;
    for(auto* manager=registry.first;manager;manager=manager->Next())
        draws+=RenderNisPlayback(manager->Playback(),view,frame);
    return draws;
}

// PAL1B0FE0..1B1110. Sound-event initialization1B0FD0 is an original
// empty body and is folded. The same live replay owns every actor/resource.
NisManager::NisManager(const char* name,int32_t group,const NisManagerBindings& b)
{
    if (!name || std::strlen(name)>=sizeof(name_) || !b.registry || !b.resources ||
        !b.loader || !b.views || group<0 || group>1)
        throw std::invalid_argument("NisManager requires its actual shared owners");
    std::memcpy(name_,name,std::strlen(name)+1);
    playback_.infoIndex=-1;
    const auto result=BindNisResources(playback_,*b.resources,*b.loader,*b.views,group);
    if (result!=CAR_NIS_OK) {
        carLiveReplayCameraUnbind(&playback_);
        throw std::runtime_error("NisManager resource-owner construction failed");
    }
    registry_=b.registry;
    // PAL1B1008 inserts before the sentinel: registration is FIFO. The
    // native null-terminated representation preserves that same ordering.
    if (!registry_->first) registry_->first=this;
    else {
        previous_=registry_->first;
        while (previous_->next_) previous_=previous_->next_;
        previous_->next_=this;
    }
}

// PAL1B1110..1B11F4 plus its real list-base tail1B3CB0. Errors leave
// ownership intact so the caller cannot report a completed destruction.
CarNisResult NisManager::Close()
{
    if (!registry_) return CAR_NIS_OK;
    auto result=carLiveReplayStopAnimating(&playback_);
    if (result!=CAR_NIS_OK) return result;
    result=UnbindNisResources(playback_);
    if (result!=CAR_NIS_OK) return result;
    if (carLiveReplayCameraUnbind(&playback_)!=CAR_VIEW_MOVER_OK)
        return CAR_NIS_INVALID_BINDING;
    if (previous_) previous_->next_=next_;
    else registry_->first=next_;
    if (next_) next_->previous_=previous_;
    previous_=next_=nullptr;
    registry_=nullptr;
    return CAR_NIS_OK;
}
NisManager::~NisManager()
{
    (void)Shutdown();
}

std::exception_ptr NisManager::Shutdown() noexcept
{
    const auto remember=[&]() noexcept {
        if(!shutdownFailure_)shutdownFailure_=std::current_exception();
    };
    const auto attempt=[&](auto&& operation) noexcept {
        try {operation();}catch(...) {remember();}
    };
    auto* context=playback_.cameraContext;
    if(context) {
        if(context->registry)for(auto* mover=context->registry->first;mover;) {
            auto* next=mover->next;
            if(mover->kind==CAR_VIEW_MOVER_REPLAY && mover->state.replay.group==context->groupIndex) {
                attempt([&] {Require(carLiveReplayUnregisterView(&playback_,mover)==CAR_VIEW_MOVER_OK,
                    "NIS terminal view-mask retirement failed");});
                if(context->registry->environment.services && mover->viewNumber>=0 && mover->viewNumber<10) {
                    auto& screen=context->registry->environment.services->screens[mover->viewNumber];
                    screen.replayComposition=0;screen.blackAndWhite=0;
                }
                carViewMoverDispose(mover);
            }
            mover=next;
        }
        context->activeViews=0;
        CarMovement* cars[]{context->first,context->second};
        const int32_t modes[]{context->savedFirstMode,context->savedSecondMode};
        for(std::size_t actor=0;actor<2;++actor)if(cars[actor]) {
            if(cars[actor]->bindings.owner && cars[actor]->bindings.owner->visual)
                carDriverVisualSetReplaySuppressed(cars[actor]->bindings.owner->visual,0);
            if(playback_.active)attempt([&] {
                Require(carMovementSetMode(cars[actor],modes[actor])==CAR_MOVEMENT_OK,
                    "NIS terminal car-mode restoration failed");
            });
            if(auto* collision=nisCarCollision(cars[actor]))collisionShapeUnlock(collision);
        }
        context->firstDriver=context->secondDriver=nullptr;
        for(auto& model:context->actorModels)editorMarkerModelDispose(&model);
        context->first=context->second=nullptr;context->animating=0;
    }
    playback_.active=0;playback_.infoIndex=-1;playback_.expiresWorldFrame=0;
    if(playback_.records) {
        if(!shutdownFailure_)shutdownFailure_=playback_.records->failure;
        attempt([&] {Require(carLiveReplayReleaseRecordBuffers(&playback_)==CAR_NIS_OK,
            "NIS terminal resource retirement failed");});
        // CancelResourceFile revokes completion before any fallible provider
        // release. The loader owns remaining files, never a dead replay.
        delete playback_.records;playback_.records=nullptr;
    }
    if(context) {std::free(context);playback_.cameraContext=nullptr;}
    if(registry_) {
        if(previous_)previous_->next_=next_;else registry_->first=next_;
        if(next_)next_->previous_=previous_;
        previous_=next_=nullptr;registry_=nullptr;
    }
    return shutdownFailure_;
}
}

// PAL1B11F8..1B1314. Restoring stops animation before replacing references;
// it deliberately leaves the NIS resource owner and its active state intact
// until the same source stream has been read in full.
CarNisResult carLiveReplayDoSnapshot(CarFrameLiveReplay* replay,
    ReplaySnapshot* snapshot,const World* world)
{
    if (!replay || !replay->cameraContext || !snapshot || !world)
        return CAR_NIS_INVALID_BINDING;
    auto& context=*replay->cameraContext;
    replaySnapshotBeginSection(snapshot,replay,"NisManager");
    if (snapshot->restoring) {
        const auto stopped=carLiveReplayStopAnimating(replay);
        if (stopped!=CAR_NIS_OK) {
            replaySnapshotEndSection(snapshot);
            return stopped;
        }
    }
    replaySnapshotMoveInteger(snapshot,&context.startedWorldTicks,4);
    replaySnapshotMoveInteger(snapshot,&replay->expiresWorldFrame,4);
    replaySnapshotMoveInteger(snapshot,&replay->infoIndex,4);
    Car* first=context.first?context.first->bindings.owner:nullptr;
    Car* second=context.second?context.second->bindings.owner:nullptr;
    worldSnapshotCarPointer(world,snapshot,&first);
    worldSnapshotCarPointer(world,snapshot,&second);
    if (snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
        context.first=first?first->Movement():nullptr;
        context.second=second?second->Movement():nullptr;
        if ((first && !context.first) || (second && !context.second)) {
            snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
            replaySnapshotEndSection(snapshot);
            return CAR_NIS_INVALID_BINDING;
        }
    }
    replaySnapshotMoveInteger(snapshot,&replay->active,1);
    replaySnapshotMoveInteger(snapshot,&context.takeIndex,1);
    replaySnapshotMoveData(snapshot,&context.savedFirstMode,1,-1,0);
    replaySnapshotMoveData(snapshot,&context.savedSecondMode,1,-1,0);
    CarNisResult result=CAR_NIS_OK;
    if (snapshot->result!=REPLAY_SNAPSHOT_OK)result=CAR_NIS_INVALID_BINDING;
    else if (snapshot->restoring && replay->active) {
        result=carLiveReplayEnsureResourcesLoaded(replay);
        if (result==CAR_NIS_OK)result=carLiveReplayStartAnimating(replay);
    }
    replaySnapshotEndSection(snapshot);
    return result;
}

/* Coordinator and NIS views. */
CarViewMoverResult carLiveReplayCameraUnbind(CarFrameLiveReplay* replay)
{
    if (!replay) return CAR_VIEW_MOVER_INVALID_BINDING;
    if (!replay->cameraContext) return CAR_VIEW_MOVER_OK;
    if (replay->cameraContext->activeViews) return CAR_VIEW_MOVER_INVALID_BINDING;
    free(replay->cameraContext);replay->cameraContext=NULL;return CAR_VIEW_MOVER_OK;
}

CarViewMoverResult carLiveReplayCameraSetSelection(CarFrameLiveReplay* replay,int32_t group,
    int32_t take,int32_t startedWorldTicks,const float firstActorOffset[4],const float secondActorOffset[4])
{
    if (!replay || !replay->cameraContext || group<0 || group>1 || take<0 ||
        !firstActorOffset || !secondActorOffset) return CAR_VIEW_MOVER_INVALID_BINDING;
    struct CarLiveReplayContext* context=replay->cameraContext;
    context->groupIndex=group;context->takeIndex=take;context->startedWorldTicks=startedWorldTicks;
    memcpy(context->firstActorOffset,firstActorOffset,sizeof(context->firstActorOffset));
    memcpy(context->secondActorOffset,secondActorOffset,sizeof(context->secondActorOffset));
    return CAR_VIEW_MOVER_OK;
}

CarViewMover* carLiveReplayCreateView(CarFrameLiveReplay* replay,int32_t viewNumber,CarViewMoverResult* result)
{
    if (result) *result=CAR_VIEW_MOVER_INVALID_BINDING;
    if (!replay || !replay->cameraContext) return NULL;
    struct CarLiveReplayContext* context=replay->cameraContext;
    CarViewMover* mover=carViewMoverCreateForReplay(context->registry,viewNumber,context->groupIndex,
        context->first,context->second,context->takeIndex,context->firstActorOffset[0],
        context->startedWorldTicks,result);
    if (!mover) return NULL;
    const CarViewMoverResult registration=carLiveReplayRegisterView(replay,mover);
    if (registration!=CAR_VIEW_MOVER_OK) {
        carViewMoverDispose(mover);if (result) *result=registration;return NULL;
    }
    if (result) *result=CAR_VIEW_MOVER_OK;
    return mover;
}

void carLiveReplayTransformBounds(float minimum[4],float maximum[4],const float frame[16])
{
    float corners[8][4];
    for (int index=0;index<8;++index) {
        const float point[4]={index&4?maximum[0]:minimum[0],index&2?maximum[1]:minimum[1],
            index&1?maximum[2]:minimum[2],0};
        vectorMatrixTransformPoint(corners[index],frame,point);
    }
    vectorBoundsFromPoint3(minimum,maximum,corners[0]);
    for (int index=1;index<8;++index) vectorBoundsExpandPoint3(minimum,maximum,corners[index]);
}

void carLiveReplayPlaceFirst(struct CarLiveReplayContext* context,const CarRecoveryPose* road,
    float position[4],CarMovement* movement,float along,float across,uint16_t heading)
{
    CarReplayPresentationState* presentation=nisViewPresentation(context,movement);
    float frame[16],centerOffset[4];
    driverRoadPointPosition(road,position,along,across);
    carMovementAlignTerrainPose(movement->bindings.info,movement->bindings.environment->contact.terrain,
        frame,position,heading);
    vectorMatrixTransformPoint(centerOffset,frame,movement->bindings.state->centerOfMass);
    for (int axis=0;axis<4;++axis) position[axis]-=centerOffset[axis];
    memcpy(presentation->centerPosition,position,sizeof(presentation->centerPosition));
    memcpy(presentation->orientation,frame,sizeof(presentation->orientation));
}

void carLiveReplayPlaceSecond(struct CarLiveReplayContext* context,float position[4],CarMovement* movement,
    const float firstPosition[4],uint16_t firstHeading,uint16_t secondHeading,float distance)
{
    position[0]=firstPosition[0]-distance*vectorCosineAngle(secondHeading);
    position[1]=firstPosition[1]-distance*rotationSineAngle(secondHeading);
    position[2]=firstPosition[2]+3.0f;position[3]=0;
    TerrainSurface surface;terrainSurfaceInitialize(&surface,nisViewEnvironment(context)->terrain);
    terrainSurfaceUpdate(&surface,position,4.0f,NULL);
    position[2]=terrainSurfaceElevation(&surface,position,NULL,NULL,NULL)+0.5f;
    float frame[16];carMovementAlignTerrainPose(movement->bindings.info,&surface,frame,position,firstHeading);
    const float* center=movement->bindings.state->centerOfMass;
    for (int axis=0;axis<4;++axis) position[axis]-=center[axis];
    CarReplayPresentationState* presentation=nisViewPresentation(context,movement);
    memcpy(presentation->centerPosition,position,sizeof(presentation->centerPosition));
    memcpy(presentation->orientation,frame,sizeof(presentation->orientation));
    terrainSurfaceDispose(&surface);
}

static int32_t nisCameraBoundsBlocked(struct CarLiveReplayContext* context,const float frame[16],
    float minimum[4],float maximum[4])
{
    carLiveReplayTransformBounds(minimum,maximum,frame);
    return collisionSpaceOverlaps(nisViewEnvironment(context)->simulation->collisions->space,minimum,maximum)!=0;
}

int32_t carLiveReplayPlacementBlocked(struct CarLiveReplayContext* context,CarMovement* first,CarMovement* second)
{
    float frame[16],minimum[4],maximum[4];
    const CarReplayPresentationState* pose=nisViewPresentation(context,first);
    memcpy(frame,pose->orientation,sizeof(frame));
    memcpy(frame+12,pose->centerPosition,3*sizeof(float));frame[15]=1;
    DriverPhysicsVehicle physics{};physics.movement=first;
    driverCollisionBounds(&physics,minimum,maximum);
    int32_t blocked=nisCameraBoundsBlocked(context,frame,minimum,maximum);
    if (context->firstDriver) {
        memset(minimum,0,sizeof(minimum));memset(maximum,0,sizeof(maximum));
        carReplayAnimationBounds(context->firstDriver,minimum,maximum);
        blocked|=nisCameraBoundsBlocked(context,frame,minimum,maximum);
    }
    if (context->secondDriver) {
        memset(minimum,0,sizeof(minimum));memset(maximum,0,sizeof(maximum));
        carReplayAnimationBounds(context->secondDriver,minimum,maximum);
        blocked|=nisCameraBoundsBlocked(context,frame,minimum,maximum);
    }
    pose=nisViewPresentation(context,second);
    memcpy(frame,pose->orientation,sizeof(frame));
    memcpy(frame+12,pose->centerPosition,3*sizeof(float));frame[15]=1;
    physics.movement=second;driverCollisionBounds(&physics,minimum,maximum);
    blocked|=nisCameraBoundsBlocked(context,frame,minimum,maximum);
    return blocked;
}

CarViewMoverResult carViewMoverReplayDeactivate(CarFrameLiveReplay* replay,CarViewMover* mover)
{
    if (!mover) return CAR_VIEW_MOVER_INVALID_BINDING;
    if (mover->updateSuspended) return CAR_VIEW_MOVER_OK;
    CarViewMoverResult result=carLiveReplayUnregisterView(replay,mover);
    if (result==CAR_VIEW_MOVER_OK) carViewMoverDeactivate(mover);
    return result;
}

CarViewMoverResult carViewMoverReplayActivate(CarFrameLiveReplay* replay,CarViewMover* mover)
{
    if (!mover) return CAR_VIEW_MOVER_INVALID_BINDING;
    if (!mover->updateSuspended) return CAR_VIEW_MOVER_OK;
    CarViewMoverResult result=carLiveReplayRegisterView(replay,mover);
    if (result==CAR_VIEW_MOVER_OK) carViewMoverActivate(mover);
    return result;
}

CarViewMoverResult carViewMoverReplayRemove(CarFrameLiveReplay* replay,CarViewMover* mover)
{
    if (!mover) return CAR_VIEW_MOVER_OK;
    CarViewMoverResult result=carViewMoverReplayDeactivate(replay,mover);
    if (result==CAR_VIEW_MOVER_OK) carViewMoverDispose(mover);
    return result;
}
