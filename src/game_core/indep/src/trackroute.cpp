#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/CameraMover.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/TrackPath.hpp"

#include "indep/inc/Replay.hpp"
#include "bware/inc/bWare.hpp"

#include "bware/inc/bChunk.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {
const CarRecoveryGroup* snapshotGroupById(const CarRecoveryPointRegistry* registry,int32_t id)
{
    if(id<0)return nullptr;
    for(size_t i=0;i<registry->groupCount;++i)
        if(registry->groups[i].id==id)return &registry->groups[i];
    hp2::Require(false,"Replay route id is absent from the actual loaded road owner");
    return nullptr;
}
int32_t snapshotPoseIndex(const CarRecoveryGroup* group,const CarRecoveryPose* pose)
{
    if(!group || !pose)return -1;
    for(int32_t i=0;i<group->poseCount;++i)if(&group->poses[i]==pose)return i;
    hp2::Require(false,"Replay road pose is not owned by its authored route");
    return -1;
}
}

// PAL1C2B30..1C2BC4. The serialized route id is signed one-byte data,
// regardless of host pointer size or the resource owner's array order.
void carRecoverySnapshotGroup(const CarRecoveryPointRegistry* registry,
    ReplaySnapshot* snapshot,const CarRecoveryGroup** group)
{
    int32_t id=-1;
    if(!snapshot->restoring && *group)id=(*group)->id;
    replaySnapshotMoveInteger(snapshot,&id,1);
    if(snapshot->restoring)*group=snapshotGroupById(registry,id);
}

// PAL1C2BC8..1C2C9C. This overload stores an index in a known actual route.
void carRecoverySnapshotPoseInGroup(const CarRecoveryGroup* group,
    ReplaySnapshot* snapshot,const CarRecoveryPose** pose)
{
    // The enclosing snapshot has already imported its route. During restore
    // *pose still belongs to the previous live route until MoveInteger below
    // imports the saved index (PAL1C2C20). PAL's temporary pointer difference
    // is overwritten by ReadData; validating that obsolete pair on a native
    // host incorrectly rejects legitimate transitions between authored roads.
    // Capture/checksum modes still validate the live pair, and restore still
    // checks the incoming index against the newly imported retained owner.
    int32_t index=snapshot->restoring?-1:snapshotPoseIndex(group,*pose);
    replaySnapshotMoveInteger(snapshot,&index,2);
    if((snapshot->recording || snapshot->restoring) && *pose && index==-1)*pose=nullptr;
    if(snapshot->restoring) {
        hp2::Require(index<0 || (group && index<group->poseCount),
            "Replay road pose index exceeds the retained route");
        *pose=index<0?nullptr:&group->poses[index];
    }
}

// PAL1C2CA0..1C2D98. The global form packs authored route and pose indices.
void carRecoverySnapshotPose(const CarRecoveryPointRegistry* registry,
    ReplaySnapshot* snapshot,const CarRecoveryPose** pose)
{
    int32_t encoded=-1;
    if(!snapshot->restoring && *pose) {
        const auto* group=snapshotGroupById(registry,(*pose)->groupId);
        const auto index=snapshotPoseIndex(group,*pose);
        hp2::Require(group && index>=0 && index<65536,"Replay road reference is invalid");
        const uint32_t bits=(static_cast<uint32_t>(static_cast<uint16_t>(group->id))<<16)|
            static_cast<uint32_t>(index);
        memcpy(&encoded,&bits,sizeof(encoded));
    }
    replaySnapshotMoveInteger(snapshot,&encoded,4);
    if(snapshot->restoring) {
        if(encoded==-1){*pose=nullptr;return;}
        const int32_t groupId=static_cast<int16_t>(static_cast<uint32_t>(encoded)>>16);
        const auto* group=snapshotGroupById(registry,groupId);
        const int32_t index=static_cast<uint16_t>(encoded);
        hp2::Require(group && index<group->poseCount,"Replay packed road pose is out of bounds");
        *pose=&group->poses[index];
    }
}

int32_t carRecoveryGroupContainsRouteDistance(const CarRecoveryGroup* group, float distance)
{
    const int32_t afterMinimum = group->minimumRouteDistance <= distance;
    const int32_t beforeMaximum = distance <= group->maximumRouteDistance;
    if (group->minimumRouteDistance < group->maximumRouteDistance)
        return afterMinimum && beforeMaximum;
    return afterMinimum || beforeMaximum;
}

static void copyPosePrefix(CarRecoveryPose* destination, const CarRecoveryPose* source)
{
    memcpy(destination->position, source->position, sizeof(destination->position));
    memcpy(destination->direction, source->direction, sizeof(destination->direction));
    destination->segmentLength = source->segmentLength;
    memcpy(destination->halfWidths, source->halfWidths, sizeof(destination->halfWidths));
    destination->gradeAcross = source->gradeAcross;
    destination->gradeAlong = source->gradeAlong;
    destination->routeDistance = source->routeDistance;
    destination->flags = source->flags;
}

void carRecoverySwapPosePrefix(CarRecoveryPose* first, CarRecoveryPose* second)
{
    if (first == second) return;
    const CarRecoveryPose temporary = *first;
    copyPosePrefix(first, second);
    copyPosePrefix(second, &temporary);
}

// Native owner validation before reproducing the PAL neighbor calculation.
// Subtracting a stale pose pointer from another route's array is undefined C++
// behavior and previously admitted a negative index into the reverse path.
// Use integer addresses only to establish membership; the valid-route index
// and neighbor arithmetic below are otherwise unchanged.
int32_t carRecoveryOwnedPoseIndex(const CarRecoveryGroup* group,
                                       const CarRecoveryPose* pose)
{
    if (!group || !pose || !group->poses || group->poseCount <= 0) return -1;
    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(group->poses);
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pose);
    if (address < base) return -1;
    const std::uintptr_t distance = address - base;
    const std::uintptr_t stride = sizeof(CarRecoveryPose);
    if (distance % stride != 0 || distance / stride >= static_cast<std::uintptr_t>(group->poseCount))
        return -1;
    return static_cast<int32_t>(distance / stride);
}

// PAL uses an 8-bit link index in AI connection choices. Verify that a
// retained pointer is a member of this exact table before computing its
// source index; cross-array subtraction is undefined in native C++.
int32_t carRecoveryOwnedConnectionIndex(const CarRecoveryPointRegistry* registry,
                                        const CarRecoveryConnection* connection)
{
    if (!registry || !connection || !registry->connections ||
        !registry->connectionCount || registry->connectionCount > 255u) return -1;
    const uintptr_t base = reinterpret_cast<uintptr_t>(registry->connections);
    const uintptr_t address = reinterpret_cast<uintptr_t>(connection);
    if (address < base) return -1;
    const uintptr_t distance = address - base;
    if (distance % sizeof(CarRecoveryConnection)) return -1;
    const uintptr_t index = distance / sizeof(CarRecoveryConnection);
    return index < registry->connectionCount ? static_cast<int32_t>(index) : -1;
}

const CarRecoveryPose* carRecoveryAdjacentPose(const CarRecoveryGroup* group,
                                              const CarRecoveryPose* pose, int32_t forward)
{
    const int32_t index = carRecoveryOwnedPoseIndex(group, pose);
    if (index < 0) return NULL;
    if (forward) return index + 1 < group->poseCount ? &group->poses[index + 1] : NULL;
    return index ? &group->poses[index - 1] : NULL;
}

const CarRecoveryPose* carRecoveryFirstFollowingPose(const CarRecoveryGroup* group,const float point[2],
                                                     const CarRecoveryPose* first)
{
    if (!point) return NULL;
    const int32_t firstIndex = carRecoveryOwnedPoseIndex(group, first);
    if (firstIndex < 0) return NULL;
    int32_t index=firstIndex;
    const CarRecoveryPose* pose=first;
    for (;;) {
        ++index;
        const float x=point[0]-pose->position[0],y=point[1]-pose->position[1];
        const float firstProjection=x*pose->direction[0],secondProjection=y*pose->direction[1];
        const float projection=-(firstProjection+secondProjection);
        if (!(projection<0.0f) || !(index<group->poseCount)) break;
        ++pose;
    }
    return index<group->poseCount ? pose : NULL;
}

void carRoadPreviewPointXY(const CarRecoveryPose* pose, float output[2], float along, float across)
{
    const float side[2] = {pose->direction[1], -pose->direction[0]};
    vectorMultiplyAdd2(output, pose->position, side, across);
    vectorMultiplyAdd2(output, output, pose->direction, along);
}

void driverRoadPointPosition(const CarRecoveryPose* pose, float output[4],
                              float along, float across)
{
    const float origin[3] = {pose->position[0], pose->position[1], pose->position[2]};
    const float lateral[3] = {pose->direction[1], -pose->direction[0], pose->gradeAcross};
    const float forward[3] = {pose->direction[0], pose->direction[1], pose->gradeAlong};
    for (int axis = 0; axis < 3; ++axis) {
        const float lateralOffset = lateral[axis] * across;
        const float shiftedOrigin = lateralOffset + origin[axis];
        const float forwardOffset = forward[axis] * along;
        output[axis] = forwardOffset + shiftedOrigin;
    }
    output[3] = 0.0f;
}


void trackRouteEditorSetWidths(CarRecoveryPose* pose,float first,float second)
{
    if(64.0f<first)first=64.0f;
    if(first<1.0f)first=1.0f;
    if(64.0f<second)second=64.0f;
    if(second<1.0f)second=1.0f;
    pose->halfWidths[1]=second;
    pose->halfWidths[0]=first;
}

// Road algorithms and resource ownership.
// Individual TrackRoute/AIUtil PDR+MAP/PAL evidence: road_paths77.json.
// Existing PAL algorithms are retained; only native C++ casts are adapted.
void carRecoveryUpdateRouteRange(CarRecoveryGroup* group, const TrackRoute* route)
{
    float minimum[2] = {route->length, route->length};
    float maximum[2] = {0.0f, 0.0f};
    const float halfLength = route->length * 0.5f;
    int32_t wrapped = 0;
    int32_t region = 0;
    TrackRouteLocation location;
    trackRouteLocationInitialize(&location);
    trackRouteUpdateLocation(route, &location, group->poses[0].position);
    float previous = location.distance;
    for (int32_t index = 1; index < group->poseCount; ++index) {
        trackRouteUpdateLocation(route, &location, group->poses[index].position);
        const float distance = location.distance;
        if (halfLength < fabsf(distance - previous)) {
            wrapped = 1;
            region ^= 1;
        }
        previous = distance;
        if (maximum[region] < distance) maximum[region] = distance;
        if (distance < minimum[region]) minimum[region] = distance;
    }
    if (wrapped) {
        group->minimumRouteDistance = maximumFinite(minimum[0], minimum[1]);
        group->maximumRouteDistance = minimumFinite(maximum[0], maximum[1]);
    } else {
        group->minimumRouteDistance = minimum[0];
        group->maximumRouteDistance = maximum[0];
    }
}

static float pointDistance(const CarRecoveryPose* pose, const float point[2])
{
    const float x = pose->position[0] - point[0];
    const float y = pose->position[1] - point[1];
    const float xx = x * x;
    const float yy = y * y;
    return sqrtf(xx + yy);
}

float carRecoveryDistance(const TrackRoute* route, const CarRecoveryPose* pose,
                           const float point[2], float routeDistance)
{
    const float distance = pointDistance(pose, point);
    const float along = fabsf(trackWrappedDelta(route->length, pose->routeDistance, routeDistance)) - 24.0f;
    return maximumFinite(along, distance);
}

const CarRecoveryPose* carRecoveryFindInGroup(const CarRecoveryPointRegistry* registry,
                                             const CarRecoveryGroup* group,
                                             const float point[2], float maximumDistance,
                                             float routeDistance, uint32_t flags)
{
    if (point[0] < group->minimum[0] - maximumDistance ||
        group->maximum[0] + maximumDistance < point[0] ||
        point[1] < group->minimum[1] - maximumDistance ||
        group->maximum[1] + maximumDistance < point[1]) return NULL;
    const CarRecoveryPose* selected = NULL;
    for (int32_t index = 0; index < group->poseCount; ++index) {
        const CarRecoveryPose* pose = &group->poses[index];
        const float distance = flags & 32u ? carRecoveryDistance(registry->route, pose, point, routeDistance) :
                                           pointDistance(pose, point);
        const float x = point[0] - pose->position[0];
        const float y = point[1] - pose->position[1];
        const float first = x * pose->direction[0];
        const float second = y * pose->direction[1];
        const float projection = -(first + second);
        if ((!(flags & 1u) || 0.0f < projection) && distance < maximumDistance &&
            (!(flags & 2u) || (pose->flags & (1u << 15))) &&
            (!(flags & 4u) || !(pose->flags & (1u << 14)))) {
            selected = pose;
            maximumDistance = distance;
        }
        const int32_t skip = convertFloatToInt32(distance * 0.0416666679084301f) - 2;
        if (0 < skip) index += skip;
    }
    return selected;
}


void driverRoadReversePose(CarRecoveryPose* pose, float routeLength)
{
    float end[4];
    driverRoadPointPosition(pose, end, pose->segmentLength, 0.0f);
    for (int axis = 0; axis < 3; ++axis) pose->position[axis] = end[axis];
    pose->direction[0] = -pose->direction[0];
    pose->direction[1] = -pose->direction[1];
    pose->gradeAlong = -pose->gradeAlong;
    pose->gradeAcross = -pose->gradeAcross;
    const float firstWidth = pose->halfWidths[0];
    pose->halfWidths[0] = pose->halfWidths[1];
    pose->halfWidths[1] = firstWidth;
    pose->routeDistance = routeLength - pose->routeDistance;
    for (int lane = 0; lane < 10; ++lane) {
        const int16_t offset = pose->laneOffsetsQuantized[lane];
        const uint8_t speed = pose->laneSpeeds[lane];
        pose->laneOffsetsQuantized[lane] = pose->laneOffsetsQuantized[lane + 10];
        pose->laneSpeeds[lane] = pose->laneSpeeds[lane + 10];
        pose->laneOffsetsQuantized[lane + 10] = offset;
        pose->laneSpeeds[lane + 10] = speed;
    }
}


CarDriverResult driverRoadPrepareConnections(CarRecoveryPointRegistry* registry)
{
    if (!registry || !registry->route) return CAR_DRIVER_INVALID_BINDING;
    TrackRouteLocation location;
    trackRouteLocationInitialize(&location);
    for (size_t index = 0; index < registry->connectionCount; ++index) {
        CarRecoveryConnection* connection = &registry->connections[index];
        const CarRecoveryPose* first = carRecoveryConnectionPose(registry, connection);
        if (!first) return CAR_DRIVER_MISSING_ROAD;
        const CarRecoveryConnection* following = carRecoveryPointConnection(registry, first);
        if (!following) return CAR_DRIVER_MISSING_ROAD;
        const CarRecoveryPose* destination = carRecoveryConnectionPose(registry, following);
        if (!destination) return CAR_DRIVER_MISSING_ROAD;
        trackRouteUpdateLocation(registry->route, &location, destination->position);
        connection->routeDistance = location.distance;
    }
    return CAR_DRIVER_OK;
}

uint32_t carRecoveryConnectionAtDistance(const CarRecoveryPointRegistry* registry, float distance)
{
    // An empty/retired table contains no authored zero-th connection.
    // Callers report MISSING_ROAD before ever using this neutral cursor.
    if (!registry || !registry->connections || !registry->connectionCount) return 0;
    uint32_t index = 0;
    while (registry->connections[index].routeDistance < distance) {
        ++index;
        if (index == registry->connectionCount) return 0;
    }
    return index;
}

const CarRecoveryGroup* carRecoveryConnectionGroup(const CarRecoveryPointRegistry* registry,
                                                   const CarRecoveryConnection* connection)
{
    if (!registry || !connection || (!registry->groups && registry->groupCount)) return NULL;
    for (size_t index=0;index<registry->groupCount;++index)
        if (registry->groups[index].id==connection->groupId) return &registry->groups[index];
    return NULL;
}

const CarRecoveryPose* carRecoveryConnectionPose(const CarRecoveryPointRegistry* registry,
                                                 const CarRecoveryConnection* connection)
{
    const CarRecoveryGroup* group=carRecoveryConnectionGroup(registry,connection);
    if (!group || connection->poseIndex>=group->poseCount) return NULL;
    return &group->poses[connection->poseIndex];
}

const CarRecoveryConnection* carRecoveryPointConnection(const CarRecoveryPointRegistry* registry,
                                                         const CarRecoveryPose* pose)
{
    if (!registry || !pose || !registry->connections) return NULL;
    const size_t index=pose->flags&255u;
    return index<registry->connectionCount ? &registry->connections[index] : NULL;
}

int32_t carRecoveryConnectionsAvailable(const CarRecoveryPointRegistry* registry)
{
    return registry->connections!=NULL;
}


void carRecoveryReversePoseDistance(CarRecoveryPose* pose, float routeLength)
{
    pose->routeDistance = routeLength - pose->routeDistance;
}

void carRecoveryReverseGroup(CarRecoveryMutableGroup* owner, float routeLength)
{
    CarRecoveryGroup* group = owner->group;
    for (int32_t index = 0; index < group->poseCount / 2; ++index)
        carRecoverySwapPosePrefix(&owner->poses[index], &owner->poses[group->poseCount - index - 1]);
    for (int32_t index = 0; index < group->poseCount; ++index)
        driverRoadReversePose(&owner->poses[index], routeLength);
    for (int32_t lane = 0; lane < 10; ++lane) {
        const uint32_t flags = group->laneFlags[lane];
        group->laneFlags[lane] = group->laneFlags[lane + 10];
        group->laneFlags[lane + 10] = flags;
        for (int32_t property = 0; property < 11; ++property) {
            const uint32_t value = group->laneProperties[lane][property];
            group->laneProperties[lane][property] = group->laneProperties[lane + 10][property];
            group->laneProperties[lane + 10][property] = value;
        }
    }
    group->flags ^= 2u;
}

void carRecoveryReverseGroupDistances(CarRecoveryMutableGroup* owner, float routeLength)
{
    for (int32_t index = 0; index < owner->group->poseCount; ++index)
        carRecoveryReversePoseDistance(&owner->poses[index], routeLength);
}

static CarRecoveryMutableGroup* mutableGroup(CarRecoveryMutableGroup* groups, size_t count, int32_t id)
{
    for (size_t index = 0; index < count; ++index)
        if (groups[index].group->id == id) return &groups[index];
    return NULL;
}

void carRecoveryReverseGroups(CarRecoveryMutableGroup* groups, size_t count, float routeLength)
{
    for (int32_t id = 0; id < 70; ++id) {
        CarRecoveryMutableGroup* owner = mutableGroup(groups, count, id);
        if (!owner) continue;
        if (id >= 50) carRecoveryReverseGroupDistances(owner, routeLength);
        else carRecoveryReverseGroup(owner, routeLength);
    }
}

void carRecoveryReverseConnections(CarRecoveryPointRegistry* registry)
{
    for (size_t index = 0; index < registry->connectionCount; ++index) {
        CarRecoveryConnection* connection = &registry->connections[index];
        if (connection->groupId >= 50) continue;
        const CarRecoveryGroup* group = carRecoveryConnectionGroup(registry, connection);
        connection->poseIndex = (uint16_t)(group->poseCount - connection->poseIndex - 1);
        connection->flags ^= 8u;
    }
}

typedef struct OrderedConnection {
    const CarRecoveryConnection* connection;
    uint32_t previousIndex;
} OrderedConnection;

static float connectionDistanceOrder(const void* first, const void* second)
{
    const OrderedConnection* a = static_cast<const OrderedConnection*>(first);
    const OrderedConnection* b = static_cast<const OrderedConnection*>(second);
    return a->connection->routeDistance - b->connection->routeDistance;
}

int32_t carRecoveryPreprocess(CarRecoveryPointRegistry* registry,
    CarRecoveryMutableGroup* groups, size_t count, int32_t forward)
{
    // Low eight pose-flag bits encode a connection index; 255 is the
    // no-connection sentinel. Reject invalid asset ownership *before* the
    // reverse pass mutates any source-owned route or the 256-byte remap table.
    if (!registry || !registry->route || (count && !groups) ||
        (registry->groupCount && !registry->groups) ||
        (registry->connectionCount && !registry->connections) ||
        registry->connectionCount > 255) return 0;
    for (size_t i=0; i<registry->connectionCount; ++i)
        if (!carRecoveryConnectionPose(registry, &registry->connections[i])) return 0;
    // A remap index absent from the connection table would read an
    // uninitialized stack byte after sort and silently redirect traffic.
    for (size_t g=0; g<registry->groupCount; ++g) {
        const CarRecoveryGroup& group=registry->groups[g];
        if (group.poseCount<0 || (group.poseCount && !group.poses)) return 0;
        for (int32_t i=0; i<group.poseCount; ++i) {
            const uint32_t previous=group.poses[i].flags & 255u;
            if (previous != 255u && previous >= registry->connectionCount) return 0;
        }
    }
    for (size_t g=0; g<count; ++g)
        if (!groups[g].group || (groups[g].group->poseCount && !groups[g].poses)) return 0;
    if (!forward) {
        carRecoveryReverseGroups(groups, count, registry->route->length);
        carRecoveryReverseConnections(registry);
    }
    for (size_t index = 0; index < registry->connectionCount; ++index) {
        const CarRecoveryConnection* connection = &registry->connections[index];
        if ((connection->flags & 7u) == 5u) {
            CarRecoveryMutableGroup* owner = mutableGroup(groups, count, connection->groupId);
            if (!owner) return 0;
            owner->group->flags |= 4u;
        }
    }
    for (int32_t id = 0; id < 100; ++id) {
        CarRecoveryMutableGroup* owner = mutableGroup(groups, count, id);
        if (owner) carRecoveryUpdateRouteRange(owner->group, registry->route);
    }
    if (driverRoadPrepareConnections(registry) != CAR_DRIVER_OK) return 0;
    const size_t connectionCount = registry->connectionCount;
    OrderedConnection* ordered = static_cast<OrderedConnection*>(malloc((connectionCount ? connectionCount : 1) * sizeof(*ordered)));
    CarRecoveryConnection* sorted = static_cast<CarRecoveryConnection*>(malloc((connectionCount ? connectionCount : 1) * sizeof(*sorted)));
    if (!ordered || !sorted) { free(ordered); free(sorted); return 0; }
    for (size_t index = 0; index < connectionCount; ++index) {
        ordered[index].connection = &registry->connections[index];
        ordered[index].previousIndex = (uint32_t)index;
    }
    if (!arrayHeapSort(ordered, (int32_t)connectionCount, sizeof(*ordered), connectionDistanceOrder)) {
        free(ordered); free(sorted); return 0;
    }
    uint8_t newIndices[256];
    for (size_t index = 0; index < connectionCount; ++index)
        newIndices[ordered[index].previousIndex] = (uint8_t)index;
    for (int32_t id = 0; id < 100; ++id) {
        CarRecoveryMutableGroup* owner = mutableGroup(groups, count, id);
        if (!owner) continue;
        for (int32_t index = 0; index < owner->group->poseCount; ++index) {
            CarRecoveryPose* pose = &owner->poses[index];
            const uint32_t previous = pose->flags & 255u;
            if (previous != 255u) pose->flags = (pose->flags & ~255u) | newIndices[previous];
        }
    }
    for (size_t index = 0; index < connectionCount; ++index)
        sorted[index] = *ordered[index].connection;
    if (connectionCount) memcpy(registry->connections, sorted, connectionCount * sizeof(*sorted));
    free(ordered);
    free(sorted);
    return 1;
}

namespace hp2 {
namespace {
float real(const std::uint8_t* bytes)
{
    const auto bits = ReadU32(bytes);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    Require(std::isfinite(value), "Recovery data contains a non-finite value");
    return value;
}

std::int32_t integer(const std::uint8_t* bytes)
{
    const auto bits = ReadU32(bytes);
    std::int32_t value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::int16_t half(const std::uint8_t* bytes)
{
    const auto bits = ReadU16(bytes);
    std::int16_t value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
} // namespace

CarRecoveryData::CarRecoveryData(const std::vector<std::uint8_t>& track, const TrackRoute* route, bool reverse)
{
    Chunk encoded{};
    for (const auto& chunk : ReadChunks(track)) {
        if (chunk.id != 0x34121u) continue;
        Require(encoded.id == 0, "Track repeats its recovery groups");
        encoded = chunk;
    }
    Require(encoded.id != 0, "Track is missing its authored recovery groups");
    std::vector<std::size_t> offsets;
    std::size_t offset = 0;
    while (offset < encoded.size) {
        Require(encoded.size - offset >= 1064, "Recovery group has a partial header");
        const auto count = integer(track.data() + encoded.payloadOffset + offset + 16);
        Require(count > 0 && count <= 1024, "Recovery group has an invalid point count");
        const std::size_t size = 1064 + static_cast<std::size_t>(count) * 112;
        Require(size <= encoded.size - offset, "Recovery group has a partial point array");
        offsets.push_back(offset);
        offset += size;
    }
    Require(!offsets.empty() && offsets.size() <= 100, "Track has an invalid recovery group count");
    groups_.resize(offsets.size());
    poses_.resize(offsets.size());
    bool ids[100]{};
    for (std::size_t index = 0; index < offsets.size(); ++index) {
        const auto* bytes = track.data() + encoded.payloadOffset + offsets[index];
        auto& group = groups_[index];
        group.id = half(bytes + 10);
        group.category = integer(bytes + 12);
        group.poseCount = integer(bytes + 16);
        group.flags = ReadU32(bytes + 24);
        for (int lane = 0; lane < 21; ++lane) {
            group.laneFlags[lane] = ReadU32(bytes + 40 + lane * 48);
            for (int property = 0; property < 11; ++property)
                group.laneProperties[lane][property] = ReadU32(bytes + 44 + lane * 48 + property * 4);
        }
        Require(group.id >= 0 && group.id < 100 && !ids[group.id], "Recovery group repeats an invalid ID");
        Require(group.category == 0 || group.category == 1, "Recovery group has an unsupported category");
        ids[group.id] = true;
        for (int axis = 0; axis < 2; ++axis) {
            group.minimum[axis] = real(bytes + 1048 + axis * 4);
            group.maximum[axis] = real(bytes + 1056 + axis * 4);
        }
        auto& points = poses_[index];
        points.resize(static_cast<std::size_t>(group.poseCount));
        for (std::size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex) {
            const auto* record = bytes + 1064 + pointIndex * 112;
            auto& point = points[pointIndex];
            for (int axis = 0; axis < 3; ++axis) {
                point.position[axis] = real(record + axis * 4);
            }
            for (int axis = 0; axis < 2; ++axis)
                point.direction[axis] = real(record + 12 + axis * 4);
            point.segmentLength = real(record + 20);
            point.halfWidths[0] = real(record + 24);
            point.halfWidths[1] = real(record + 28);
            point.routeDistance = real(record + 40);
            point.flags = ReadU32(record + 44);
            point.groupId = record[111];
            point.gradeAcross = real(record + 32);
            point.gradeAlong = real(record + 36);
            for (int lane = 0; lane < 21; ++lane) {
                point.laneOffsetsQuantized[lane] = half(record + 48 + lane * 2);
                point.laneSpeeds[lane] = record[90 + lane];
            }
            Require(point.groupId == group.id, "Recovery point references another group");
        }
        group.poses = points.data();
    }
    registry_.groups = groups_.data();
    registry_.groupCount = groups_.size();
    registry_.route = route;
    registry_.reverse = reverse;
    Require(route && route->length > 0.0f && route->sectionCount > 0 && route->nodeCount > 0,
            "Recovery groups cannot be bound to an empty route");
    Chunk connections{};
    for (const auto& chunk : ReadChunks(track)) {
        if (chunk.id != 0x34122u) continue;
        Require(connections.id == 0, "Track repeats its road connections");
        connections = chunk;
    }
    if (connections.id != 0) {
        Require(connections.size % 12 == 0, "Road connections have a partial record");
        connections_.resize(connections.size / 12);
        Require(connections_.size() <= 255, "Road connections exceed the authored point index range");
        for (std::size_t index = 0; index < connections_.size(); ++index) {
            const auto* record = track.data() + connections.payloadOffset + index * 12;
            auto& connection = connections_[index];
            connection.groupId = record[0];
            connection.flags = record[1];
            connection.poseIndex = ReadU16(record + 2);
            /* The load-time route pass replaces this field for every link.
             * Serialized scratch bits can be non-finite and are not data. */
            connection.routeDistance = 0.0f;
            connection.properties = ReadU32(record + 8);
            const auto* group = Group(connection.groupId);
            Require(group && connection.poseIndex < group->poseCount,
                    "Road connection references an invalid authored point");
        }
    }
    registry_.connections = connections_.data();
    registry_.connectionCount = connections_.size();
    std::vector<CarRecoveryMutableGroup> mutableGroups;
    for (std::size_t index = 0; index < groups_.size(); ++index) {
        mutableGroups.push_back({&groups_[index], poses_[index].data()});
        for (const auto& point : poses_[index])
            Require((point.flags & 255u) == 255u || (point.flags & 255u) < connections_.size(),
                    "Recovery point references an invalid road connection");
    }
    Require(carRecoveryPreprocess(&registry_, mutableGroups.data(), mutableGroups.size(), !reverse) != 0,
            "Road groups and connections cannot be prepared on the authored route");
}

const CarRecoveryGroup* CarRecoveryData::Group(std::uint8_t id) const noexcept
{
    for (const auto& group : groups_) if (group.id == id) return &group;
    return nullptr;
}

std::vector<CarRecoveryMutableGroup> CarRecoveryData::MutableGroups()
{
    std::vector<CarRecoveryMutableGroup> views;
    views.reserve(groups_.size());
    for (std::size_t index = 0; index < groups_.size(); ++index)
        views.push_back({&groups_[index], poses_[index].data()});
    return views;
}

} // namespace hp2

int32_t arrayHeapSort(void* array, int32_t count, size_t stride, ArraySortCompare compare)
{
    if (count < 2) return 1;
    unsigned char* elements = static_cast<unsigned char*>(array);
    unsigned char* temporary = static_cast<unsigned char*>(malloc(stride));
    if (!temporary) return 0;
    int32_t remaining = count;
    int32_t first = count / 2 + 1;
    for (;;) {
        if (first < 2) {
            memcpy(temporary, elements + (size_t)(remaining - 1) * stride, stride);
            --remaining;
            memcpy(elements + (size_t)remaining * stride, elements, stride);
            if (remaining == 1) {
                memcpy(elements, temporary, stride);
                break;
            }
        } else {
            --first;
            memcpy(temporary, elements + (size_t)(first - 1) * stride, stride);
        }
        int32_t destination = first - 1;
        int32_t child = first * 2;
        while (child <= remaining) {
            if (child < remaining && compare(elements + (size_t)(child - 1) * stride,
                                              elements + (size_t)child * stride) < 0.0f)
                ++child;
            if (compare(temporary, elements + (size_t)(child - 1) * stride) < 0.0f) {
                memcpy(elements + (size_t)destination * stride,
                       elements + (size_t)(child - 1) * stride, stride);
                destination = child - 1;
                child *= 2;
            } else {
                child = remaining + 1;
            }
        }
        memcpy(elements + (size_t)destination * stride, temporary, stride);
    }
    free(temporary);
    return 1;
}


// Individual TrackRoute methods: PAL1C1990 and PAL1C1A98.
int32_t trackRouteEditorUpdateSegment(CarRecoveryMutableGroup* group,int32_t index)
{
    if(index>=group->group->poseCount-1)return 0;
    CarRecoveryPose* point=&group->poses[index];
    CarRecoveryPose* next=&group->poses[index+1];
    const float dx=next->position[0]-point->position[0];
    const float dy=next->position[1]-point->position[1];
    point->direction[0]=dx;point->direction[1]=dy;
    const float yy=dy*dy,xx=point->direction[0]*point->direction[0];
    point->segmentLength=sqrtf(xx+yy);
    const float dz=next->position[2]-point->position[2];
    const float grade=finiteRatio(dz,point->segmentLength);
    point->gradeAlong=grade;next->gradeAlong=grade;
    vectorNormalize2(point->direction,point->direction);
    return point->segmentLength>24.0f||point->segmentLength<4.0f;
}

int32_t trackRouteEditorMovePoint(CarRecoveryMutableGroup* group,int32_t index,const float point[4])
{
    if(index<0||index>=group->group->poseCount)return 1;
    CarRecoveryPose* pose=&group->poses[index];
    pose->position[0]=point[0];pose->position[1]=point[1];
    /* The original editor stores the supplied X in the elevation cell too. */
    pose->position[2]=point[0];
    int32_t result=trackRouteEditorUpdateSegment(group,index);
    if(index>0)result+=trackRouteEditorUpdateSegment(group,index-1);
    return result;
}
