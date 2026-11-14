#ifndef NFSHP2_NIS_HPP
#define NFSHP2_NIS_HPP

#include <stdint.h>
#include <exception>
struct CarLiveReplayContext;
struct CarLiveReplayRecords;
/* Live player playback is a distinct owner from the integer replay log and
 * snapshot manager. These fields are the currently consumed live state;
 * clip playback and its resource lifecycle stay with this owner. */
typedef struct CarFrameLiveReplay {
    int32_t active;
    int32_t expiresWorldFrame;
    struct CarLiveReplayContext* cameraContext;
    struct CarLiveReplayRecords* records;
    int32_t infoIndex;
} CarFrameLiveReplay;
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/CameraCoordinator.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CarNisResult {
    CAR_NIS_OK,
    CAR_NIS_INVALID_BINDING,
    CAR_NIS_INVALID_RECORD,
    CAR_NIS_RESOURCE_FAILURE,
    CAR_NIS_MOVEMENT_FAILURE
} CarNisResult;

typedef struct CarNisInfo {
    char name[32];
    int32_t kind;
    int32_t selectionGroup;
    int32_t authoredIndex;
    int32_t frameCount;
    int32_t takeCount;
    uint32_t firstAnimationKey;
    uint32_t secondAnimationKey;
    uint32_t actorModelKeys[4];
    const uint8_t* authoredRecord;
    int32_t firstBodyTypeCount;
    int32_t secondBodyTypeCount;
    uint32_t bodyKeys[2][8];
} CarNisInfo;

typedef struct CarNisOverlay CarNisOverlay;
struct CarViewMoverRegistry;

CarNisResult carLiveReplayLoad(CarFrameLiveReplay* replay, int32_t index);
CarNisResult carLiveReplayResourceLoaded(CarFrameLiveReplay* replay);
CarNisResult carLiveReplayReleaseRecordBuffers(CarFrameLiveReplay* replay);
CarNisResult carLiveReplayEnsureResourcesLoaded(CarFrameLiveReplay* replay);
CarNisResult carLiveReplaySetupCars(CarFrameLiveReplay* replay);
CarNisResult carLiveReplayStartAnimating(CarFrameLiveReplay* replay);
CarNisResult carLiveReplayStart(CarFrameLiveReplay* replay, int32_t index,
    int32_t take, struct CarMovement* first, struct CarMovement* second);
int32_t carLiveReplayFrameNumber(const CarFrameLiveReplay* replay);
CarNisResult carLiveReplaySetupForRendering(CarFrameLiveReplay* replay);
int32_t carLiveReplayCurrentInfoIndex(const CarFrameLiveReplay* replay);
const CarNisInfo* carLiveReplayCurrentInfo(const CarFrameLiveReplay* replay);
CarNisResult carLiveReplayRestoreCars(CarFrameLiveReplay* replay);
CarNisResult carLiveReplayStopAnimating(CarFrameLiveReplay* replay);
CarNisResult carLiveReplayStop(CarFrameLiveReplay* replay);
struct ReplaySnapshot;
struct World;
CarNisResult carLiveReplayDoSnapshot(CarFrameLiveReplay* replay,
    struct ReplaySnapshot* snapshot,const struct World* world);
void carNisOverlayFinish(CarNisOverlay* overlay);
CarNisResult carNisOverlayStart(CarNisOverlay* overlay);
CarNisResult carNisOverlayStartTicket(CarNisOverlay* overlay);
CarNisResult carNisOverlayStartArrest(CarNisOverlay* overlay);
CarNisResult carNisOverlayStartBusted(CarNisOverlay* overlay);
CarNisResult carNisOverlayUpdateDateAndTime(CarNisOverlay* overlay);
CarNisResult carNisOverlayUpdate(CarNisOverlay* overlay, int32_t blink);
struct RaceParameters;
CarNisResult carNisOverlaySetVisible(CarNisOverlay* overlay,int32_t visible,
    const struct RaceParameters* race,const int32_t* sessionState);

struct CarReplayAnimationController;
struct CarLiveReplayContext {
    CarViewMoverRegistry* registry;
    CarMovement* first;
    CarMovement* second;
    struct CarReplayAnimationController* firstDriver;
    struct CarReplayAnimationController* secondDriver;
    uint32_t activeViews;
    int32_t groupIndex,takeIndex,startedWorldTicks;
    float firstActorOffset[4],secondActorOffset[4];
    int32_t savedFirstMode,savedSecondMode;
    int32_t animating;
    EditorMarkerModel actorModels[4];
    float groundHeight;
};

struct CarRecoveryPose;

CarViewMoverResult carLiveReplayCameraUnbind(CarFrameLiveReplay* replay);
CarViewMoverResult carLiveReplayCameraSetSelection(CarFrameLiveReplay* replay,int32_t group,
    int32_t take,int32_t startedWorldTicks,const float firstActorOffset[4],const float secondActorOffset[4]);
CarViewMover* carLiveReplayCreateView(CarFrameLiveReplay* replay,int32_t viewNumber,CarViewMoverResult* result);
void carLiveReplayTransformBounds(float minimum[4],float maximum[4],const float frame[16]);
void carLiveReplayPlaceFirst(struct CarLiveReplayContext* context,const struct CarRecoveryPose* road,
    float position[4],struct CarMovement* movement,float along,float across,uint16_t heading);
void carLiveReplayPlaceSecond(struct CarLiveReplayContext* context,float position[4],
    struct CarMovement* movement,const float firstPosition[4],uint16_t firstHeading,
    uint16_t secondHeading,float distance);
int32_t carLiveReplayPlacementBlocked(struct CarLiveReplayContext* context,
    struct CarMovement* first,struct CarMovement* second);
CarViewMoverResult carLiveReplayRegisterView(CarFrameLiveReplay* replay,CarViewMover* mover);
CarViewMoverResult carLiveReplayUnregisterView(CarFrameLiveReplay* replay,CarViewMover* mover);
CarViewMoverResult carLiveReplaySetViewMasks(struct CarLiveReplayContext* context,int32_t viewNumber,
    struct CarMovement* first,struct CarMovement* second,int32_t enabled);


#ifdef __cplusplus
}

#include <cstddef>
#include <cstdint>
#include <vector>
#include "indep/inc/Timer.hpp"
#include "indep/inc/Point.hpp"

namespace hp2 {
class GameResources;
class ResourceLoader;
extern "C++" {
class FlailerResources;
struct FlailerRenderManager;
struct FlailerObjectChunk;
struct FlailerTextChunk;
class LanguageManager;
}

/* The GLOBALB37010 owner is shared by every NIS manager. File decoding uses
 * the same immutable GameResources instance throughout the world lifetime. */
class NisResources final {
public:
    explicit NisResources(const GameResources& resources);
    NisResources(const GameResources& resources,const std::vector<std::uint8_t>& globalBundle);
    NisResources(const NisResources&) = delete;
    NisResources& operator=(const NisResources&) = delete;
    const CarNisInfo* Info(std::size_t index) const noexcept;
    const CarNisInfo* InfoByKind(std::size_t ordinal, int32_t kind) const noexcept;
    std::size_t CountKind(int32_t kind) const noexcept;
    bool SupportsFirstBodyType(std::size_t index, uint32_t key) const noexcept;
    bool SupportsSecondBodyType(std::size_t index, uint32_t key) const noexcept;
    bool IsSuitable(std::size_t index, uint32_t firstKey, uint32_t secondKey,
        int32_t excludedIndex) const noexcept;
    std::size_t Count() const noexcept;
    const GameResources& Files() const noexcept;
private:
    const GameResources& resources_;
    std::vector<std::uint8_t> global_;
    std::vector<CarNisInfo> infos_;
};

CarNisResult BindNisResources(CarFrameLiveReplay& replay,
    const NisResources& resources, ResourceLoader& loader,
    CarViewMoverRegistry& views, int32_t group);
CarNisResult UnbindNisResources(CarFrameLiveReplay& replay);
const NisResources* NisResourceCatalogue(const CarFrameLiveReplay& replay) noexcept;

class NisManager;
/* PAL intrusive list ownership. This registry outlives each actual manager;
 * it does not own a second replay/player list. */
struct NisManagerRegistry { NisManager* first{}; };
// Synchronous port presentation boundary. Models and poses are borrowed
// for the callback only. The donor retains the clip and its timeline.
using NisPoseSink = void (*)(void*, std::size_t, const EditorMarkerModel&,
    const float*, const std::vector<std::array<float,16>>*);
void VisitNisPoses(NisManagerRegistry& registry, int viewId, NisPoseSink sink, void* user,
    float worldFrameFraction);
// Original per-frame NIS consumers borrow the same live replay managers,
// animation/model catalogues and render view; they own no second timeline.
std::size_t RenderNisPlayback(CarFrameLiveReplay& playback,
    const CarCinematicScreen& view,EcstasyFrame& frame);
std::size_t DrawNisObjects(NisManagerRegistry& registry,
    const CarCinematicScreen& view,EcstasyFrame& frame);
extern float nisHandOffsets[2][3];
struct NisManagerBindings {
    NisManagerRegistry* registry{};
    const NisResources* resources{};
    ResourceLoader* loader{};
    CarViewMoverRegistry* views{};
};
class NisManager final {
public:
    NisManager(const char* name,int32_t group,const NisManagerBindings& bindings);
    ~NisManager();
    NisManager(const NisManager&)=delete;
    NisManager& operator=(const NisManager&)=delete;
    CarFrameLiveReplay& Playback() noexcept { return playback_; }
    const char* Name() const noexcept { return name_; }
    NisManager* Next() const noexcept { return next_; }
    CarNisResult Close();
    std::exception_ptr Shutdown() noexcept;
private:
    char name_[32]{};
    CarFrameLiveReplay playback_{};
    NisManagerRegistry* registry_{};
    NisManager* previous_{};
    NisManager* next_{};
    std::exception_ptr shutdownFailure_;
};
}

/* The player owns this small CPU overlay state; resources and the render
 * manager are the existing shared Flailer owners, not a second screen list. */
struct CarNisOverlayTuning {
    float startScale;
    float startAlpha;
    float endAlpha;
    float durationSeconds;
};

/* The four actual mutable GP cells330820..33082C have one native owner. */
extern CarNisOverlayTuning carNisOverlayTuning;

struct CarNisOverlay {
    CarNisOverlay();
    ~CarNisOverlay();
    CarNisOverlay(const CarNisOverlay&) = delete;
    CarNisOverlay& operator=(const CarNisOverlay&) = delete;
    hp2::FlailerResources* resources;
    hp2::FlailerRenderManager* manager;
    uint32_t screenKey;
    hp2::FlailerObjectChunk* object;
    float savedScaleX;
    float savedScaleY;
    float savedTopLeftY;
    int32_t mode;
    int32_t lastDateFrames;
    /* The source owns two48-byte allocations. Native inline ownership keeps
     * exactly these two curves for the whole overlay lifetime. */
    tCubic1D scale;
    tCubic1D alpha;
    hp2::FlailerTextChunk* secondText;
    float startTopLeftX;
    float startTopLeftY;
    float startScaleX;
    float startScaleY;
    const GameTime* clock;
    hp2::LanguageManager* language;
    const CarNisOverlayTuning* tuning;
};

namespace hp2 {
/* This checked host binding preserves PAL1829A8's screen/transform producer.
 * It borrows the same Flailer, Language and GameTime owners as the Player. */
CarNisResult BindNisOverlay(CarNisOverlay& overlay,
    FlailerResources& resources, FlailerRenderManager& manager,
    LanguageManager& language, const GameTime& clock,
    const CarNisOverlayTuning& tuning, uint32_t screenKey);
}
#endif

#endif
