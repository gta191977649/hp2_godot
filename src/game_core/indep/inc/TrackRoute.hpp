#ifndef NFSHP2_INDEP_INC_TRACKROUTE_HPP
#define NFSHP2_INDEP_INC_TRACKROUTE_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Ecstasy.hpp"

struct ReplaySnapshot;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CarRecoveryPose {
    float position[3];
    float direction[2];
    float segmentLength;
    float halfWidths[2];
    float routeDistance;
    uint32_t flags;
    uint8_t groupId;
    float gradeAcross;
    float gradeAlong;
    int16_t laneOffsetsQuantized[21];
    uint8_t laneSpeeds[21];
} CarRecoveryPose;

typedef struct CarRecoveryGroup {
    int16_t id;
    int32_t category;
    uint32_t flags;
    float minimum[2];
    float maximum[2];
    const CarRecoveryPose* poses;
    int32_t poseCount;
    uint32_t laneFlags[21];
    float minimumRouteDistance;
    float maximumRouteDistance;
    uint32_t laneProperties[21][11];
} CarRecoveryGroup;

typedef struct CarRecoveryConnection {
    uint8_t groupId;
    uint8_t flags;
    uint16_t poseIndex;
    float routeDistance;
    uint32_t properties;
} CarRecoveryConnection;

/* Group order is the authored order within each category. The arrays and
 * route remain borrowed from the track resource owner. */
typedef struct CarRecoveryPointRegistry {
    const CarRecoveryGroup* groups;
    size_t groupCount;
    const TrackRoute* route;
    int32_t reverse;
    CarRecoveryConnection* connections;
    size_t connectionCount;
} CarRecoveryPointRegistry;

typedef struct CarRecoverySelection {
    const CarRecoveryGroup* group;
    const CarRecoveryPose* pose;
} CarRecoverySelection;

/* Requires a populated route and at least one authored pose. The first pose
 * establishes traversal continuity; later poses determine the route range. */
void carRecoveryUpdateRouteRange(CarRecoveryGroup* group, const TrackRoute* route);
int32_t carRecoveryGroupContainsRouteDistance(const CarRecoveryGroup* group, float distance);

float carRecoveryDistance(const TrackRoute* route, const CarRecoveryPose* pose,
                           const float point[2], float routeDistance);
const CarRecoveryPose* carRecoveryFindInGroup(const CarRecoveryPointRegistry* registry,
                                             const CarRecoveryGroup* group,
                                             const float point[2], float maximumDistance,
                                             float routeDistance, uint32_t flags);
int32_t carRecoverySelectPose(CarRecoverySelection* selection, const CarRecoveryPointRegistry* registry,
                        const float point[2], int32_t categoryMode, uint32_t flags,
                        float routeDistance);


/* Writable views refer to the same resource-owned groups/points exposed by
 * the query registry. They do not contain a second geometry or flag copy. */
typedef struct CarRecoveryMutableGroup {
    CarRecoveryGroup* group;
    CarRecoveryPose* poses;
} CarRecoveryMutableGroup;

void carRecoverySwapPosePrefix(CarRecoveryPose* first, CarRecoveryPose* second);
void carRecoveryReversePoseDistance(CarRecoveryPose* pose, float routeLength);
void carRecoveryReverseGroup(CarRecoveryMutableGroup* owner, float routeLength);
void carRecoveryReverseGroupDistances(CarRecoveryMutableGroup* owner, float routeLength);
void carRecoveryReverseGroups(CarRecoveryMutableGroup* groups, size_t count, float routeLength);
void carRecoveryReverseConnections(CarRecoveryPointRegistry* registry);
int32_t carRecoveryPreprocess(CarRecoveryPointRegistry* registry,
    CarRecoveryMutableGroup* groups, size_t count, int32_t forward);


const CarRecoveryGroup* carRecoveryConnectionGroup(const CarRecoveryPointRegistry* registry,
                                                   const CarRecoveryConnection* connection);
const CarRecoveryPose* carRecoveryConnectionPose(const CarRecoveryPointRegistry* registry,
                                                 const CarRecoveryConnection* connection);
const CarRecoveryConnection* carRecoveryPointConnection(const CarRecoveryPointRegistry* registry,
                                                         const CarRecoveryPose* pose);
const CarRecoveryPose* carRecoveryAdjacentPose(const CarRecoveryGroup* group,
    const CarRecoveryPose* pose,int32_t forward);
/* Native validation of route ownership; -1 for a stale/foreign point. */
int32_t carRecoveryOwnedPoseIndex(const CarRecoveryGroup* group,const CarRecoveryPose* pose);
/* -1 when a cached branch belongs to another or retired route table. */
int32_t carRecoveryOwnedConnectionIndex(const CarRecoveryPointRegistry* registry,
                                        const CarRecoveryConnection* connection);
const CarRecoveryPose* carRecoveryFirstFollowingPose(const CarRecoveryGroup* group,const float point[2],
                                                     const CarRecoveryPose* first);
int32_t carRecoveryConnectionsAvailable(const CarRecoveryPointRegistry* registry);
/* Connections are sorted by route distance and the registry is nonempty.
 * A position beyond the last connection wraps to the first one. */
uint32_t carRecoveryConnectionAtDistance(const CarRecoveryPointRegistry* registry, float distance);
int32_t carRecoveryFindFollowingBranches(const CarRecoveryPointRegistry* registry,
                                         CarRecoverySelection* output,int32_t capacity,
                                         const CarRecoveryGroup* group,const CarRecoveryPose* pose,
                                         float distance,int32_t forward);


/* The original point calculation defines XYZ. The unused homogeneous lane
 * is initialized to zero in the native result. Output has four floats. */
void driverRoadPointPosition(const CarRecoveryPose* pose, float output[4],
                              float along, float across);
void driverRoadReversePose(CarRecoveryPose* pose, float routeLength);
int32_t driverRoadSelectionValid(const CarRecoverySelection* selection);
CarDriverResult driverRoadNextPose(const CarRecoveryPointRegistry* registry, CarRecoverySelection* selection);
CarDriverResult driverRoadAdvanceSelection(const CarRecoveryPointRegistry* registry, CarRecoverySelection* selection);
CarDriverResult driverRoadPrepareConnections(CarRecoveryPointRegistry* registry);



float driverLaneOutsideRoad(const CarRecoveryPose* pose,float across);
void carRecoverySnapshotGroup(const struct CarRecoveryPointRegistry* registry,
    struct ReplaySnapshot* snapshot,const struct CarRecoveryGroup** group);
void carRecoverySnapshotPoseInGroup(const struct CarRecoveryGroup* group,
    struct ReplaySnapshot* snapshot,const CarRecoveryPose** pose);
void carRecoverySnapshotPose(const struct CarRecoveryPointRegistry* registry,
    struct ReplaySnapshot* snapshot,const CarRecoveryPose** pose);
void carRecoverySelectionSnapshot(const struct CarRecoveryPointRegistry* registry,
    struct CarRecoverySelection* selection,struct ReplaySnapshot* snapshot);


// Native public bindings for the individually proved TrackRoute/editor owners.
// Host views preserve the existing resource/router lifetime; no Beta ABI layout
// or unimplemented authoring mode is implied by these declarations.
typedef struct TerrainTopology TerrainTopology;
typedef struct TrackRouteEditor TrackRouteEditor;
typedef struct TrackRouteEditorInput TrackRouteEditorInput;
struct EditorMarkerResources;

typedef enum TrackRouteEditorResult {
    TRACK_ROUTE_EDITOR_OK,
    TRACK_ROUTE_EDITOR_INVALID_BINDING,
    TRACK_ROUTE_EDITOR_ALLOCATION_FAILURE,
    TRACK_ROUTE_EDITOR_INPUT_CAPACITY
} TrackRouteEditorResult;

/* One track owns these mutable commands and one editor. Every Drive camera
 * borrows that same editor; activation is deliberately not reference counted. */
typedef struct TrackRouteEditorSettings {
    int32_t selectedGroup;
    int32_t activeCameraDraws;
    uint64_t selectionRevision;
} TrackRouteEditorSettings;

typedef struct TrackRouteEditorContext {
    CarRecoveryPointRegistry* registry;
    CarRecoveryMutableGroup* mutableGroups;
    size_t mutableGroupCount;
    TerrainTopology* terrain;
    GameInputRouter* input;
    const GameTime* clock;
    TrackRouteEditorSettings* settings;
    struct EditorMarkerResources* markers;
} TrackRouteEditorContext;

void trackRouteEditorSettingsInitialize(TrackRouteEditorSettings* settings);
TrackRouteEditor* trackRouteEditorCreate(const TrackRouteEditorContext* context,
    TrackRouteEditorResult* result);
void trackRouteEditorDispose(TrackRouteEditor* editor);
TrackRouteEditorResult trackRouteEditorActivate(TrackRouteEditor* editor,const float eye[4]);
void trackRouteEditorDeactivate(TrackRouteEditor* editor);
TrackRouteEditorResult trackRouteEditorTickCamera(TrackRouteEditor* editor);
void trackRouteEditorResetCamera(TrackRouteEditor* editor);
void trackRouteEditorFindNear(TrackRouteEditor* editor,const float point[4]);
void trackRouteEditorUpdateRoadPose(TrackRouteEditor* editor,int32_t groupId,int32_t poseIndex);
int32_t trackRouteEditorGetCameraPosition(const TrackRouteEditor* editor,
    float eye[4],float direction[4]);

/* Input storage belongs to the editor. The router borrows concrete records,
 * preserving raw values, pending presses, elapsed repeat time and order. */
void trackRouteEditorInputDispatch(TrackRouteEditorInput* input,uint8_t value);
int32_t trackRouteEditorInputRepeat(TrackRouteEditorInput* input,float elapsed,
    float initialDelay,float repeatDelay);
TrackRouteEditorResult trackRouteEditorActivateInput(TrackRouteEditor* editor,int32_t device);
void trackRouteEditorDeactivateInput(TrackRouteEditor* editor);

/* These editing operations mutate the same resource-owned point array that
 * every recovery, road-planner and camera reader already borrows. */
void trackRouteEditorSetWidths(CarRecoveryPose* pose,float first,float second);
int32_t trackRouteEditorUpdateSegment(CarRecoveryMutableGroup* group,int32_t index);
int32_t trackRouteEditorMovePoint(CarRecoveryMutableGroup* group,int32_t index,
    const float point[4]);


struct TrackRouteEditorInput {
    int32_t event;
    int32_t device;
    uint8_t raw;
    uint8_t pending;
    float repeatElapsed;
    float nextRepeat;
};

struct TrackRouteEditor {
    TrackRouteEditorContext context;
    int32_t mode;
    int32_t selectedPose;
    int32_t inputRegistered;
    int32_t validPose;
    TrackRouteEditorInput inputs[18];
    float desiredEye[4];
    float desiredDirection[4];
    float currentEye[4];
    float eyeVelocity[4];
    float currentDirection[4];
    float directionVelocity[4];
    float roadAnchor[4];
    uint16_t roadHeading;
    EditorMarkerModel markers[4][3];
    EditorMarkerReplacement replacements[4][3];
};



#ifdef __cplusplus
}

#include <cstdint>
#include <vector>

namespace hp2 {

class CarRecoveryData final {
public:
    CarRecoveryData(const std::vector<std::uint8_t>& track, const TrackRoute* route, bool reverse);
    CarRecoveryData(const CarRecoveryData&) = delete;
    CarRecoveryData& operator=(const CarRecoveryData&) = delete;
    CarRecoveryData(CarRecoveryData&&) = delete;
    CarRecoveryData& operator=(CarRecoveryData&&) = delete;
    CarRecoveryPointRegistry& Native() noexcept { return registry_; }
    const CarRecoveryPointRegistry& Native() const noexcept { return registry_; }
    const CarRecoveryGroup* Group(std::uint8_t id) const noexcept;
    std::vector<CarRecoveryMutableGroup> MutableGroups();

private:
    CarRecoveryPointRegistry registry_{};
    std::vector<CarRecoveryGroup> groups_;
    std::vector<std::vector<CarRecoveryPose>> poses_;
    std::vector<CarRecoveryConnection> connections_;
};

} // namespace hp2

#endif


// Native declaration of the individually proved trackroute.cpp HeapSort.
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The original ordering function returns a floating difference. Storage
 * and element strides must describe a valid writable array. */
typedef float (*ArraySortCompare)(const void* first, const void* second);
int32_t arrayHeapSort(void* array, int32_t count, size_t stride, ArraySortCompare compare);

#ifdef __cplusplus
}
#endif

#endif
