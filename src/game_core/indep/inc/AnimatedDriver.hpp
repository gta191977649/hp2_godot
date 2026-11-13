#ifndef NFSHP2_ANIMATED_DRIVER_HPP
#define NFSHP2_ANIMATED_DRIVER_HPP
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Point.hpp"
#include <stdint.h>

#ifdef __cplusplus
#include <array>
#include <memory>
#include <vector>
namespace hp2 {
// Native local-pose scratch owned by one visual. Object/world/view matrices
// are never part of this cache; moving a car cannot invalidate its palette.
class DriverPoseCache final {
public:
    struct Work {std::size_t skeletonPlans{},quaternionMatrices{},parentMatrices{},inverseBindMatrices{};};
    DriverPoseCache() noexcept;
    ~DriverPoseCache();
    DriverPoseCache(DriverPoseCache&&) noexcept;
    DriverPoseCache& operator=(DriverPoseCache&&) noexcept;
    DriverPoseCache(const DriverPoseCache&)=delete;
    DriverPoseCache& operator=(const DriverPoseCache&)=delete;
    const std::vector<std::array<float,16>>& Prepare(const EditorMarkerAnimationTree& tree,
        const float rotations[][4],std::size_t rotationCount);
    const Work& Statistics() const noexcept {return work_;}
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
    Work work_;
};
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CarDriverVisual CarDriverVisual;
struct CarReplayAnimationController;
struct CarMovement;
struct CarDriverCharacter;
struct GameTime;
struct CarCinematicServices;
struct PursuitManager;
struct CarDriverVisualAnimationDefinitions;

/* An immutable boundary view of the same authored vehicle record. The
 * gameplay state is always read through movement, never copied here. */
typedef struct CarDriverVisualVehicleInfo {
    int32_t modelCode;
    int8_t driverAnimationCode;
} CarDriverVisualVehicleInfo;

typedef struct CarDriverVisualBindings {
    struct CarMovement* movement;
    const CarDriverVisualVehicleInfo* vehicle;
    const struct CarDriverCharacter* character;
    const struct GameTime* clock;
    const struct CarCinematicServices* cinematics;
    const struct PursuitManager* pursuit;
    struct CarDriverVisualAnimationDefinitions* definitions;
} CarDriverVisualBindings;

typedef enum CarDriverVisualResult {
    CAR_DRIVER_VISUAL_OK,
    CAR_DRIVER_VISUAL_INVALID_BINDING,
    CAR_DRIVER_VISUAL_INVALID_ANIMATION,
    CAR_DRIVER_VISUAL_ALLOCATION_FAILURE,
    CAR_DRIVER_VISUAL_MISSING_MODEL
} CarDriverVisualResult;

/* The appearance owner is separate from gameplay control. Replay borrows
 * the same first animation controller used by this visual owner. */
void carDriverVisualBeginLookBack(CarDriverVisual* visual);
void carDriverVisualEndLookBack(CarDriverVisual* visual);
void carDriverVisualSetLookBack(CarDriverVisual* visual,int32_t enabled);
void carDriverVisualBeginGearChange(CarDriverVisual* visual);
int32_t carDriverVisualSteeringFrame(CarDriverVisual* visual);
struct CarReplayAnimationController* carDriverVisualAnimation(CarDriverVisual* visual);
void carDriverVisualSetReplaySuppressed(CarDriverVisual* visual,int32_t suppressed);
int32_t carDriverVisualStartAnimation(CarDriverVisual* visual,int32_t kind);
void carDriverVisualRequestAnimation(CarDriverVisual* visual,int32_t kind,int32_t priority);
void carDriverVisualReleaseAnimation(CarDriverVisual* visual,int32_t kind);
void carDriverVisualClearAnimations(CarDriverVisual* visual);
void carDriverVisualReleaseAnimations(CarDriverVisual* visual);
int32_t carDriverVisualMaximumRemaining(const CarDriverVisual* visual);
void carDriverVisualRefreshLookBack(CarDriverVisual* visual);
void carDriverVisualBeginAction3(CarDriverVisual* visual);
void carDriverVisualEndAction3(CarDriverVisual* visual);
void carDriverVisualRefreshAction3(CarDriverVisual* visual);
CarDriverVisualResult carDriverVisualResult(const CarDriverVisual* visual);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

enum { CAR_DRIVER_VISUAL_ANIMATION_COUNT=14, CAR_DRIVER_VISUAL_BONE_COUNT=42 };

typedef struct CarDriverVisualAnimationDefinition {
    int32_t kind;
    uint32_t key;
    const CarReplayAnimationClip* clip;
    int32_t firstFrame;
    int32_t frameCount;
    int32_t holdFrame;
    int32_t priority;
    int32_t property;
    int32_t rebaseRotations;
} CarDriverVisualAnimationDefinition;

/* This resource-owned table is shared by all visual instances. Its rotation
 * preprocessing mutates the catalogue's one retained stream only once. */
typedef struct CarDriverVisualAnimationDefinitions {
    CarDriverVisualAnimationDefinition animations[CAR_DRIVER_VISUAL_ANIMATION_COUNT];
    int32_t rotationPreprocessPending;
} CarDriverVisualAnimationDefinitions;

void carDriverVisualAnimationDefinitionsInitialize(CarDriverVisualAnimationDefinitions* definitions);
int32_t carDriverVisualPrepareAnimations(CarDriverVisualAnimationDefinitions* definitions,
    const CarReplayAnimationCatalogue* catalogue);
int32_t carDriverAnimationReadRotation(const CarReplayAnimationClip* clip,float output[4],
    int32_t bone,float frame);
int32_t carDriverAnimationWriteRotation(const CarReplayAnimationClip* clip,const float input[4],
    int32_t bone,float frame);
int32_t carDriverAnimationReadAllRotations(const CarReplayAnimationClip* clip,
    float output[][4],size_t outputCount,float frame);
// Native batching of the same codec, including the three shift-arm bones.
int32_t carDriverAnimationReadRotationRange(const CarReplayAnimationClip* clip,
    float output[][4],size_t outputCount,int32_t firstBone,size_t count,float frame);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CarDriverVisualPlayback {
    const CarDriverVisualAnimationDefinition* definition;
    int32_t startedTicks;
    int32_t endingTicks;
    int32_t holdFrame;
} CarDriverVisualPlayback;

void carDriverVisualPlaybackAssign(CarDriverVisualPlayback* playback,
    const CarDriverVisualAnimationDefinitions* definitions,int32_t kind,int32_t delay,int32_t now);
int32_t carDriverVisualPlaybackFrame(const CarDriverVisualPlayback* playback,int32_t now,int32_t* frame);
int32_t carDriverVisualPlaybackFinish(CarDriverVisualPlayback* playback,int32_t now);
int32_t carDriverVisualPlaybackRelease(CarDriverVisualPlayback* playback,
    const CarDriverVisualAnimationDefinitions* definitions,int32_t now);

#ifdef __cplusplus
}
#endif
#include "indep/inc/CarDriver.hpp"
struct CarCinematicServices;
#ifdef __cplusplus
extern "C" {
#endif
int32_t carDriverVisualAnimationBlocked(int32_t kind,int32_t modelCode,
    int8_t driverAnimationCode,const struct CarCinematicServices* cinematics,
    const CarDriverCharacter* character);
#ifdef __cplusplus
}
#endif

struct CarDriverVisual {
    CarDriverVisualBindings bindings;
    CarReplayAnimationController* animations[3];
    EditorMarkerModel* staticModels[3];
    EditorMarkerReplacement replacements[2];
    tCubic1D steering;
    uint32_t shiftAnimationKey;
    int32_t steeringStartedTicks;
    int32_t replaySuppressed;
    int8_t lookBack;
    int8_t action3;
    int32_t lookBackAnimation;
    CarDriverVisualPlayback playback[2];
    CarDriverVisualResult result;
    /* The three animated model handles are the original eAnimation model
     * subobjects. Controllers borrow them; staticModels remain separate. */
    EditorMarkerModel animatedModels[3];
    NativeResourceRegistry* resources;
    const struct CarDriverAppearanceCatalogue* appearances;
#ifdef __cplusplus
    hp2::DriverPoseCache poseCache;
#endif
};


#ifdef __cplusplus
extern "C" {
#endif

struct CarDriverAppearanceCatalogue;
struct ReplaySnapshot;
void carDriverVisualSnapshot(CarDriverVisual* visual,struct ReplaySnapshot* snapshot);
void carDriverVisualInitializeState(CarDriverVisual* visual,const CarDriverVisualBindings* bindings);
CarDriverVisualPlayback* carDriverVisualFindPlaying(CarDriverVisual* visual,int32_t kind);
void carDriverVisualSetupSteeringFrame(CarDriverVisual* visual);
void carDriverVisualInitializeArtwork(CarDriverVisual* visual);
CarDriverVisual* carDriverVisualCreate(const CarDriverVisualBindings* bindings,
    NativeResourceRegistry* resources,const struct CarDriverAppearanceCatalogue* appearances);
void carDriverVisualDestroy(CarDriverVisual* visual);
void carDriverVisualQueueRandomAnimations(CarDriverVisual* visual,int32_t first,
    int32_t last,int32_t count);
extern int32_t carDriverVisualForcedDetail;
extern int32_t carDriverVisualForceAnimated;

/* Header definition individually anchored by the complete PreRace camera
 * Update. The original request is exactly the inclusive range12..13, two
 * selections, through the same live animation catalogue and RNG. */
static inline void carDriverVisualBeginPreRaceAnimation(CarDriverVisual* visual)
{
    carDriverVisualQueueRandomAnimations(visual,12,13,2);
}

/* PAL1AA6C0 is originally a header definition. Its native C interface keeps
 * the declaration here and the out-of-line body in the proven compilation
 * unit, indep/src/animateddriver.cpp. */
void carDriverVisualPlaybackCancel(CarDriverVisualPlayback* playback,
    const CarDriverVisualAnimationDefinitions* definitions, int32_t now);

#ifdef __cplusplus
}
namespace hp2 {
void BuildDriverQuaternionTable(CarDriverVisual& visual,CarReplayAnimationController& animation,
    float rotations[CAR_DRIVER_VISUAL_BONE_COUNT][4],float translation[3]);
std::size_t RenderDriverVisual(CarDriverVisual& visual,const float localToWorld[16],
    const SceneLightContext* lightContext,const CarCinematicScreen& view,EcstasyFrame& frame,
    std::uint32_t flags,std::int32_t detail);
std::size_t RenderDriverAnimationPose(DriverPoseCache& cache,CarReplayAnimationController& animation,
    const float localToWorld[16],const float rootTranslation[3],const float rotations[][4],
    std::size_t rotationCount,const SceneLightContext* lightContext,
    const CarCinematicScreen& view,EcstasyFrame& frame,std::uint32_t flags);
}
#endif

#endif
