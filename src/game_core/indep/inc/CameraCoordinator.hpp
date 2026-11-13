#ifndef NFSHP2_CAR_VIEW_CAMERA_RESOURCES_H
#define NFSHP2_CAR_VIEW_CAMERA_RESOURCES_H

#include "indep/inc/CameraMover.hpp"
struct CarMotionState;

/* Recorded pose is distinct from live VehicleState. The three masks are
 * Car+510/514/518: motion and replay borrow the same CarMotionState words. */
typedef struct CarReplayPresentationState {
    float orientation[16];
    float centerPosition[4];
    struct CarMotionState* motion;
} CarReplayPresentationState;

struct CarLiveReplayContext;
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CarMovement;
struct CarFramePlayerState;
struct CarFrameLiveReplay;
typedef struct CarViewMoverEnvironment CarViewMoverEnvironment;

/* Each coordinator owns a concrete CPU camera. The registry and the actual
 * camera/resource/world owners outlive its coordinators. A coordinator may
 * legitimately have no camera when an authored lookup returns no record. */
CarViewMoverRegistry* carViewMoverRegistryCreate(const CarViewMoverEnvironment* environment,
    CarViewMoverResult* result);
void carViewMoverRegistryDispose(CarViewMoverRegistry* registry);
CarViewMoverResult carViewMoversAdvance(CarViewMoverRegistry* registry,float elapsed);
void carViewMoversInvalidate(CarViewMoverRegistry* registry);

CarViewMover* carViewMoverCreateForCar(CarViewMoverRegistry* registry,int32_t viewNumber,
    struct CarMovement* car,CarViewMoverResult* result);
CarViewMover* carViewMoverCreateForCountdown(CarViewMoverRegistry* registry,int32_t viewNumber,
    struct CarFramePlayerState* player,CarViewMoverResult* result);
CarViewMover* carViewMoverCreateForReplay(CarViewMoverRegistry* registry,int32_t viewNumber,
    int32_t groupIndex,struct CarMovement* firstCar,struct CarMovement* secondCar,
    int32_t takeIndex,float timeOffset,int32_t startWorldTicks,CarViewMoverResult* result);
void carViewMoverDispose(CarViewMover* mover);
void carViewMoverActivate(CarViewMover* mover);
void carViewMoverDeactivate(CarViewMover* mover);
CarCameraReference carViewMoverCurrentCamera(const CarViewMover* mover);
CarCinematicBase* carViewMoverCurrentBase(CarViewMover* mover);
CarViewMoverResult carViewMoverAdvanceCamera(CarViewMover* mover);

/* Replay removal updates the same live replay/presentation owners before
 * releasing the concrete coordinator; it is not a generic destroy hook. */
CarViewMoverResult carViewMoverReplayDeactivate(struct CarFrameLiveReplay* replay,
    CarViewMover* mover);
CarViewMoverResult carViewMoverReplayActivate(struct CarFrameLiveReplay* replay,
    CarViewMover* mover);
CarViewMoverResult carViewMoverReplayRemove(struct CarFrameLiveReplay* replay,
    CarViewMover* mover);

typedef struct CarViewCameraResources CarViewCameraResources;
struct CarViewMoverRegistry;
typedef struct CarViewRegionVertex { float x,y; } CarViewRegionVertex;
typedef struct CarViewRegionTriangle {
    uint32_t vertices[3];
    uint32_t properties;
} CarViewRegionTriangle;
typedef struct CarViewAuthoredCamera {
    CarCameraPathItems items;
    const CarViewRegionVertex* vertices;
    size_t vertexCount;
    const CarViewRegionTriangle* triangles;
    size_t triangleCount;
} CarViewAuthoredCamera;
typedef struct CarViewRecordedPose {
    float position[3];
    float quaternion[4];
    uint16_t fieldOfView;
} CarViewRecordedPose;
typedef struct CarViewRecordedTake {
    const char* name;
    const CarViewRecordedPose* poses;
    size_t poseCount;
} CarViewRecordedTake;

/* Decoded records retain their own immutable stream storage. No borrowed
 * bundle byte pointer survives loading. A raw lookup borrows the current
 * publication; a camera that survives replacement holds its explicit lease. */
CarViewCameraResources* carViewCameraResourcesCreate(void);
void carViewCameraResourcesDispose(CarViewCameraResources* resources);
int carViewCameraResourcesBindMovers(CarViewCameraResources* resources,struct CarViewMoverRegistry* registry);
int carViewCameraResourcesLoadBundle(CarViewCameraResources* resources,
    const uint8_t* bytes,size_t size);
/* Native retirement of the same published non-replay camera groups.
 * Bits0..3 select groups; bit4 selects offsets. Existing leases stay valid. */
int carViewCameraResourcesUnloadTrackGroups(CarViewCameraResources* resources,uint32_t groupMask);
int carViewCameraResourcesLoadReplayFile(CarViewCameraResources* resources,
    const void* assetOwner,const char* immutableFileIdentity,const uint8_t* bytes,size_t size);
size_t carViewCameraResourcesReplayCacheCount(const CarViewCameraResources* resources);
int carViewCameraResourcesCommitReplayGroup(CarViewCameraResources* resources,int32_t group);
#ifdef __cplusplus
// Revoke this file's publication without committing any pending take.
// Immutable cached records still retain previously borrowed camera views.
void carViewCameraResourcesRetireReplayFile(CarViewCameraResources* resources,
    const void* assetOwner,const char* immutableFileIdentity,int32_t group) noexcept;
#endif
uint32_t carViewCameraResourcesRevision(const CarViewCameraResources* resources);
size_t carViewCameraResourcesRegionCount(const CarViewCameraResources* resources);
const CarViewAuthoredCamera* carViewCameraResourcesRegionAt(
    const CarViewCameraResources* resources,size_t index);
const CarViewAuthoredCamera* carViewCameraResourcesFind(
    const CarViewCameraResources* resources,const char* name);
const CarCameraOffsetTrack* carViewCameraResourcesChooseOffset(
    const CarViewCameraResources* resources,uint32_t selector);
size_t carViewCameraResourcesTakeCount(const CarViewCameraResources* resources,int32_t group);
const CarViewRecordedTake* carViewCameraResourcesTakeAt(
    const CarViewCameraResources* resources,int32_t group,size_t index);

int32_t carViewPointInPolygon(const float point[2],const CarViewRegionVertex* vertices,int32_t count);
int32_t carViewRegionContains(const CarViewAuthoredCamera* camera,const CarMovement* movement);




#ifdef __cplusplus
}
#include <memory>
namespace hp2 {
// Retain the actual decoded record behind a currently published semantic
// view. An empty result means that this catalogue does not publish the view.
// Also permits native tools to keep a view across a load/unload transaction.
std::shared_ptr<const void> RetainCameraResource(const CarViewCameraResources& resources,
    const CarViewAuthoredCamera* camera);
std::shared_ptr<const void> RetainCameraResource(const CarViewCameraResources& resources,
    const CarViewRecordedTake* take);
std::shared_ptr<const void> RetainCameraResource(const CarViewCameraResources& resources,
    const CarCameraOffsetTrack* offset);
}
#endif

/* The sole coordinator types now live in their proved original header.
 * CarViewMover keeps its existing native name; it owns the same child and
 * remains in the same World registry. */

#include "indep/inc/CameraMover.hpp"
#include "indep/inc/World.hpp"

struct WorldSimulation;
struct CarRecordedCamera;
struct CarRecordedCameraSettings;
typedef struct CarViewMoverSettings {
    uint16_t projectionAngle;
    /* Separate source GP-5A44 loop control, authored initial zero. */
    int32_t loopPreRaceCameras;
} CarViewMoverSettings;

struct CarViewMoverEnvironment {
    CarCinematicServices* services;
    TerrainTopology* terrain;
    const struct WorldSimulation* simulation;
    CarViewCameraResources* resources;
    const RaceParameters* race;
    const CarChaseCameraVehicle* vehicles;
    size_t vehicleCount;
    const CarCameraCatalogue* defaultCatalogue;
    struct CarCameraShake* shake;
    CarChaseCameraSettings* chaseSettings;
    const CarAirborneCameraSettings* airborneSettings;
    const struct CarRecordedCameraSettings* recordedSettings;
    const CarViewMoverSettings* settings;
};

#ifdef __cplusplus
extern "C" {
#endif

/* The settings cell is the shared mutable projection-angle owner. Its
 * default follows the original display initialization producer. */
void carViewMoverSettingsInitialize(CarViewMoverSettings* settings);
CarReplayPresentationState* carViewMoverPresentation(CarViewMoverRegistry* registry,const Car* car);

#ifdef __cplusplus
}
#endif


typedef enum CarViewMoverKind { CAR_VIEW_MOVER_CAR,CAR_VIEW_MOVER_COUNTDOWN,CAR_VIEW_MOVER_REPLAY,
    CAR_VIEW_MOVER_FLYBY,CAR_VIEW_MOVER_PRE_RACE } CarViewMoverKind;
typedef enum CarViewChildKind { CAR_VIEW_CHILD_NONE,CAR_VIEW_CHILD_CHASE,CAR_VIEW_CHILD_BUMPER,
    CAR_VIEW_CHILD_PATH,CAR_VIEW_CHILD_AIRBORNE,CAR_VIEW_CHILD_RECORDED } CarViewChildKind;
struct CarViewResourceLease;
typedef struct CarViewChild {
    CarViewChildKind kind;
    union {
        CarChaseCamera* chase;
        CarBumperCamera* bumper;
        CarPathCamera* path;
        CarAirborneCamera* airborne;
        struct CarRecordedCamera* recorded;
    } owner;
    // Native ownership of this child's borrowed immutable resource spans.
    // Released only after the original concrete camera has been disposed.
    struct CarViewResourceLease* resourceLease;
} CarViewChild;
struct CarViewMover {
    CarViewMover* next;
    CarViewMover* previous;
    CarViewMoverRegistry* registry;
    CarViewChild child;
    CarViewMoverKind kind;
    int32_t viewNumber;
    int32_t updateSuspended;
    int32_t dirty;
    union {
        struct {
            CarMovement* car;
            int32_t rearFacing,viewIndex,lastSelectionFrame,regionIndex;
            uint32_t resourceRevision;
        } car;
        struct { struct CarFramePlayerState* player;int32_t stage; } countdown;
        struct { int32_t group;CarMovement* first;CarMovement* second; } replay;
        struct { int32_t activeIndex,nextIndex;uint32_t resourceRevision; } flyby;
        struct {
            int32_t activeIndex,numberToShow,shownCount,cameraIndex,carCount;
            CarMovement* cars[32];
            int32_t carIndex,finished;
            uint32_t resourceRevision;
        } preRace;
    } state;
};
typedef struct CarViewPresentationOwner {
    const Car* car;
    CarReplayPresentationState state;
} CarViewPresentationOwner;
struct CarViewMoverRegistry {
    CarViewMover* first;
    CarViewMover* last;
    CarViewMoverEnvironment environment;
    CarViewPresentationOwner presentations[WORLD_MAX_CARS];
    size_t presentationCount;
};

#ifdef __cplusplus
extern "C" {
#endif
CarCinematicBase* carViewChildBase(const CarViewChild* child);
CarViewMover* carViewMoverAllocate(CarViewMoverRegistry* registry,int32_t viewNumber,CarViewMoverKind kind);
void carViewChildDispose(CarViewChild* child);
CarViewMoverResult carViewChildAdvance(CarViewChild* child);
CarViewMoverResult carViewMoverUpdateCar(CarViewMover* mover);
CarViewMoverResult carViewMoverUpdateCountdown(CarViewMover* mover);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
CarViewMover* carViewMoverCreateForFlyby(CarViewMoverRegistry* registry,int32_t viewNumber,
    CarViewMoverResult* result);
CarViewMover* carViewMoverCreateForPreRace(CarViewMoverRegistry* registry,int32_t viewNumber,
    int32_t specialPursuit,CarViewMoverResult* result);
CarViewMoverResult carViewMoverUpdateFlyby(CarViewMover* mover);
CarViewMoverResult carViewMoverUpdatePreRace(CarViewMover* mover);
int32_t carViewMoverFlybyFinished(const CarViewMover* mover);
/* A direct observation of the finished field written by the complete update,
 * not a second camera timer or an additional accepted source method. */
int32_t carViewMoverPreRaceFinished(const CarViewMover* mover);
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus


#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hp2 {

using CameraVector4 = std::array<float, 4>;

struct CameraRegionVertex {
    float x = 0.0f;
    float y = 0.0f;
};

struct CameraRegionTriangle {
    std::array<std::uint32_t, 3> vertices{};
    std::uint32_t properties = 0;
};

struct CameraProjectionSettings {
    std::uint32_t usesWidth = 0;
    float routeStart = 0.0f;
    float routeEnd = 0.0f;
    std::array<CameraVector4, 32> samples{};
};

struct CameraKeyPlayback {
    float eyeAdvanceRate = 0.0f;
    float targetAdvanceRate = 0.0f;
    // Authored only in format 2; its consumer has not been identified.
    float authoredParameter = 0.0f;
    std::uint32_t eyeUsesRouteFraction = 0;
    std::uint32_t targetUsesRouteFraction = 0;
    float routeStart = 0.0f;
    float routeEnd = 0.0f;
};

struct CameraLegacyFraming {
    // The remaining authored controls are preserved without calling them a matrix.
    std::array<float, 8> authoredCoefficients{};
    std::array<float, 2> start{};
    std::array<float, 2> end{};
    std::uint32_t interpolate = 0;
    // The source tests this float for zero; retain its complete authored value.
    float eyeProjectionGate = 0.0f;
    std::uint32_t closestApproachGate = 0;
    std::uint32_t placementSelector = 0;
};

// Native semantic data, not the source process object layout. All stream
// storage is owned, immutable through Get(), and retained by every copy.
class CameraDataItems final {
public:
    struct Data {
        std::uint32_t format = 0;
        std::string name;
        CameraProjectionSettings projection;
        CameraKeyPlayback playback;
        float nearClip = 0.0f;
        float farClip = 0.0f;
        std::uint32_t directionGate = 0;
        CameraLegacyFraming legacy;
        std::vector<CameraVector4> eyeKeys;
        std::vector<CameraVector4> rollKeys;
        std::vector<CameraVector4> targetKeys;
        std::vector<CameraRegionTriangle> regionTriangles;
        std::vector<CameraRegionVertex> regionVertices;
        std::size_t encodedBytes = 0;
    };

    CameraDataItems(const std::uint8_t* bytes, std::size_t size);
    explicit CameraDataItems(const std::vector<std::uint8_t>& bytes)
        : CameraDataItems(bytes.data(), bytes.size()) {}
    CameraDataItems(const CameraDataItems& other) noexcept;
    CameraDataItems& operator=(const CameraDataItems&) noexcept = default;

    const Data& Get() const noexcept { return *data_; }

private:
    std::shared_ptr<const Data> data_;
};

} // namespace hp2

#endif
#endif
