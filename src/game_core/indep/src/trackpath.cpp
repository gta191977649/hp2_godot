#include "indep/inc/QuickSpline.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/TrackPath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Replay.hpp"
#include "bware/inc/bWare.hpp"

#include "bware/inc/bChunk.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {
// Live track changes and Replay can invalidate cached node borrows. Native
// pointer subtraction/comparison across different allocations is undefined;
// validate an index before dereferencing a node, preserving all valid PAL
// node selection and spline math unchanged.
static int32_t trackRouteOwnedNodeIndex(const TrackRoute* route,const TrackRouteNode* node)
{
    if (!route || !node || !route->nodes || route->nodeCount<=0) return -1;
    const auto first=reinterpret_cast<std::uintptr_t>(route->nodes);
    const auto current=reinterpret_cast<std::uintptr_t>(node);
    if (current<first) return -1;
    const auto offset=current-first;
    if (offset % sizeof(TrackRouteNode) != 0 ||
        offset/sizeof(TrackRouteNode)>=static_cast<std::uintptr_t>(route->nodeCount)) return -1;
    return static_cast<int32_t>(offset/sizeof(TrackRouteNode));
}

static inline float trackRouteDistance2(const float first[2], const float second[2])
{
    const float x = first[0] - second[0];
    const float y = first[1] - second[1];
    const float squaredX = x * x, squaredY = y * y;
    return sqrtf(squaredX + squaredY);
}


// PAL207BD0..207C68, native index adaptation of the same retained node owner.
void trackPathSnapshotNode(const TrackRoute* route,const TrackRouteNode** node,ReplaySnapshot* snapshot)
{
    int16_t index=-1;
    if(*node) {
        int32_t found=-1;
        for(int32_t i=0;i<route->nodeCount;++i)if(&route->nodes[i]==*node){found=i;break;}
        hp2::Require(found>=0 && found<=32767,"Replay track-path node is not a representable authored index");
        index=static_cast<int16_t>(found);
    }
    replaySnapshotMoveData(snapshot,&index,2,2,1);
    hp2::Require(index<0 || index<route->nodeCount,"Replay track-path node is out of bounds");
    *node=index<0?nullptr:&route->nodes[index];
}
}

// PAL208768..2087F8. The route coordinate keeps its actual authored node;
// the stream never embeds a wider host pointer or another topology owner.
void trackRouteLocationSnapshot(const TrackRoute* route,TrackRouteLocation* location,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"TrackPathCoordinate");
    replaySnapshotMoveData(snapshot,location->point,8,-1,0);
    trackPathSnapshotNode(route,&location->node,snapshot);
    replaySnapshotMoveFloat(snapshot,&location->parameter);
    replaySnapshotMoveFloat(snapshot,&location->progress);
    replaySnapshotMoveFloat(snapshot,&location->distance);
    replaySnapshotMoveData(snapshot,location->direction,8,-1,0);
    replaySnapshotEndSection(snapshot);
}

float trackWrappedDelta(float period, float position, float reference)
{
    const float delta = position - reference;
    const float halfPeriod = period * 0.5f;
    if (delta < -halfPeriod)
        return delta + period;
    if (halfPeriod < delta)
        return delta - period;
    return delta;
}

float trackWrappedDelta100(float position, float reference)
{
    const float delta = position - reference;
    if (delta < -50.0f)
        return delta + 100.0f;
    if (50.0f < delta)
        return delta - 100.0f;
    return delta;
}

float trackWrappedDeltaUnit(float position, float reference)
{
    const float delta = position - reference;
    if (delta < -0.5f)
        return delta + 1.0f;
    if (0.5f < delta)
        return delta - 1.0f;
    return delta;
}

const TrackRouteNode* trackRouteFindNearestNode(const TrackRoute* route, const float point[2])
{
    const TrackRouteNode* selected = NULL;
    float minimum = 0.0f;
    for (int section = 0; section < route->sectionCount; ++section) {
        const TrackRouteSection* range = &route->sections[section];
        for (int node = range->firstNode; node < range->endNode; ++node) {
            const TrackRouteNode* candidate = &route->nodes[node];
            const float distance = trackRouteDistance2(candidate->point, point);
            if (!selected || distance < minimum) { minimum = distance; selected = candidate; }
        }
    }
    return selected;
}

const TrackRouteNode* trackRouteFindLocalNode(const TrackRoute* route, const float point[2], const TrackRouteNode* node)
{
    const int32_t index=trackRouteOwnedNodeIndex(route,node);
    if (index<0 || !point || !route->sections || node->section<0 ||
        node->section>=route->sectionCount) return NULL;
    const TrackRouteSection* section = &route->sections[node->section];
    if (section->firstNode<0 || section->endNode>route->nodeCount ||
        index<section->firstNode || index>=section->endNode) return NULL;
    const TrackRouteNode* candidates[3] = {node, NULL, NULL};
    int count = 1;
    if (index != section->firstNode) candidates[count++] = node - 1;
    else if (section->closed) candidates[count++] = &route->nodes[section->endNode - 1];
    if (index != section->endNode - 1) candidates[count++] = node + 1;
    else if (section->closed) candidates[count++] = &route->nodes[section->firstNode];
    const TrackRouteNode* selected = NULL;
    float minimum = 0.0f;
    for (int candidate = 0; candidate < count; ++candidate) {
        const float distance = trackRouteDistance2(point, candidates[candidate]->point);
        if (!selected || distance < minimum) { minimum = distance; selected = candidates[candidate]; }
    }
    return selected;
}

TrackRouteLocation* trackRouteLocationInitialize(TrackRouteLocation* location)
{
    memset(location, 0, sizeof(*location));
    location->direction[0] = 1.0f;
    return location;
}

// PAL208350, the original distance/across overload. Keep the final branch
// at20851C exactly: it repeats only when the decremented node count is zero.
// The other moved-node path reaches208528 with an unwritten stack direction.
// Native C++ rejects that undefined source domain instead of inventing a
// tangent or making an uninitialized-memory value part of the game state.
int32_t trackRoutePositionAtDistance(const TrackRoute* route,TrackRouteLocation* location,
    float distance,float across,int32_t sectionIndex)
{
    if (!route || !location || !route->sections || route->sectionCount<=0 ||
        route->nodeCount<=0 || !route->nodes || !(route->length>0.0f) ||
        !std::isfinite(route->length) || !std::isfinite(distance) || !std::isfinite(across)) return 0;
    const TrackRouteNode* node=location->node;
    if(node && (!std::isfinite(location->parameter) || !std::isfinite(location->point[0]) ||
        !std::isfinite(location->point[1]))) return 0;
    if (!node) {
        if (sectionIndex<0 || sectionIndex>=route->sectionCount) return 0;
        const int32_t index=route->sections[sectionIndex].firstNode;
        if (index>=0) {
            if (index>=route->nodeCount) return 0;
            node=route->nodes+index;
        }
        if (!node) return 0;
    }
    int32_t remaining=route->nodeCount;
    float direction[2];
    for (;;) {
        const auto index=trackRouteOwnedNodeIndex(route,node);
        if (index<0 || node->section<0 || node->section>=route->sectionCount) return 0;
        const auto& section=route->sections[node->section];
        if (section.firstNode<0 || section.endNode>route->nodeCount ||
            section.firstNode>=section.endNode || index<section.firstNode || index>=section.endNode ||
            !section.curve || !section.curve->storage) return 0;
        const auto* next=index<section.endNode-1 ? node+1 : nullptr;
        const auto* previous=index>section.firstNode ? node-1 : nullptr;
        bool moved=false;
        if (distance<node->distance && previous) {node=previous;moved=true;}
        else if (node->distance<distance) {
            if (!next) return 0; // PAL would dereference a null next node.
            if (next->distance<distance) {node=next;moved=true;}
        }
        if (moved) {
            --remaining;
            if (remaining!=0) return 0; // source scratch direction is unwritten
            continue;
        }
        if (next) {
            direction[0]=next->point[0]-node->point[0];
            direction[1]=next->point[1]-node->point[1];
        } else {
            if (!previous) return 0;
            direction[0]=node->point[0]-previous->point[0];
            direction[1]=node->point[1]-previous->point[1];
        }
        const float squared=direction[0]*direction[0]+direction[1]*direction[1];
        if (!(squared>0.0f) || !std::isfinite(squared)) return 0;
        vectorNormalize2(direction,direction);
        break;
    }
    float point[2];
    vectorMultiplyAdd2(point,node->point,direction,distance);
    const float lateral[2]={direction[1],-direction[0]};
    vectorMultiplyAdd2(point,point,lateral,across);
    if (!std::isfinite(point[0]) || !std::isfinite(point[1])) return 0;
    TrackRouteLocation staged=*location;
    trackRouteUpdateLocation(route,&staged,point);
    if (!staged.node || !std::isfinite(staged.parameter) || !std::isfinite(staged.progress) ||
        !std::isfinite(staged.distance) || !std::isfinite(staged.direction[0]) ||
        !std::isfinite(staged.direction[1]) ||
        (staged.direction[0]==0.0f && staged.direction[1]==0.0f)) return 0;
    *location=staged;
    return 1;
}

// PAL208820 uses the same global route owner and literal section0. The
// native signature borrows that owner explicitly; there is no second route.
int32_t trackCoordinatePositionAtDistance(const TrackRoute* route,TrackRouteLocation* location,
    float distance,float across)
{
    return trackRoutePositionAtDistance(route,location,distance,across,0);
}

// PAL2078E8..208898: TrackPath curve construction. Direction publication
// precedes publication of this route to road preprocessing.
int32_t trackRouteBuildCurves(TrackRoute* route)
{
    route->length = route->sections[0].nominalLength;
    for (int index = 0; index < route->sectionCount; ++index) {
        TrackRouteSection* section = &route->sections[index];
        if (section->curve) continue;
        QuickSpline* curve = (QuickSpline*)malloc(sizeof(*curve));
        if (!curve) return 0;
        const QuickSplineBoundary boundary = section->closed ? QUICK_SPLINE_CLOSED : QUICK_SPLINE_EXTRAPOLATE;
        quickSplineInitialize(curve, boundary, 0, (uint16_t)section->nodeCount);
        if (!curve->storage) { free(curve); return 0; }
        section->curve = curve;
        quickSplineSetPointCount(curve, (uint16_t)section->nodeCount);
        for (int node = 0; node < section->nodeCount; ++node) {
            const TrackRouteNode* source = &route->nodes[section->firstNode + node];
            curve->points[node][0] = source->point[0];
            curve->points[node][1] = source->point[1];
            curve->points[node][2] = curve->points[node][3] = 0.0f;
            curve->pointsDirty = 1;
        }
        quickSplineCalculateLength(curve, 16);
    }
    return 1;
}

void trackRouteDispose(TrackRoute* route)
{
    for (int index = 0; index < route->sectionCount; ++index) {
        TrackRouteSection* section = &route->sections[index];
        if (section->curve) {
            quickSplineDestroy(section->curve, 3);
            section->curve = NULL;
        }
    }
}

float trackRouteDistanceDifference(const TrackRoute* route, float first, float second)
{
    return trackWrappedDelta(route->length, first, second);
}

const TrackRouteNode* trackRouteFindNodeNearDistance(const TrackRoute* route, const float point[2], float distance)
{
    if (route->reverse) distance = route->length - distance;
    const TrackRouteNode* selected = NULL;
    float minimum = 0.0f;
    for (int section = 0; section < route->sectionCount; ++section) {
        const TrackRouteSection* range = &route->sections[section];
        for (int node = range->firstNode; node < range->endNode; ++node) {
            const TrackRouteNode* candidate = &route->nodes[node];
            const float planarDistance = trackRouteDistance2(candidate->point, point);
            const float difference = trackRouteDistanceDifference(route, candidate->distance, distance);
            const float penalty = fabsf(difference * 0.25f);
            const float score = planarDistance + penalty;
            if (!selected || score < minimum) { minimum = score; selected = candidate; }
        }
    }
    return selected;
}

void trackRouteUpdateLocation(const TrackRoute* route, TrackRouteLocation* location, const float point[2])
{
    const TrackRouteNode* node = location->node;
    if (trackRouteOwnedNodeIndex(route,node)<0 ||
        128.0f < trackRouteDistance2(point, location->point))
        node = trackRouteFindNearestNode(route, point);
    if (!node) {
        location->point[0] = point[0]; location->point[1] = point[1];
        location->progress = location->distance = 0.0f;
        location->direction[0] = 1.0f; location->direction[1] = 0.0f;
        return;
    }
    node = trackRouteFindLocalNode(route, point, node);
    if (!node) {
        // Even the nearest candidate can be invalid when a resource is
        // retiring; do not interpret its bytes as a section or spline.
        location->node=NULL;
        location->point[0]=point[0];location->point[1]=point[1];
        location->progress=location->distance=0.0f;
        location->direction[0]=1.0f;location->direction[1]=0.0f;
        return;
    }
    float minimum = trackRouteDistance2(point, node->point);
    for (int neighbor = 0; neighbor < 3; ++neighbor) {
        const int index = node->neighbors[neighbor];
        if (index >= 0 && index < route->nodeCount) {
            const TrackRouteNode* candidate = &route->nodes[index];
            if (candidate->section<0 || candidate->section>=route->sectionCount) continue;
            const float distance = trackRouteDistance2(point, candidate->point);
            if (distance < minimum) { node = candidate; minimum = distance; }
        }
    }
    TrackRouteSection* section = &route->sections[node->section];
    float parameter = location->parameter;
    if (node != location->node) {
        location->node = node;
        parameter = (float)(trackRouteOwnedNodeIndex(route,node) - section->firstNode);
    }
    const float point4[4] = {point[0], point[1], 0.0f, 0.0f};
    parameter = quickSplineFindClosestParameter(section->curve, point4, parameter, 0.05000000074505806f);
    float evaluated[4], tangent[4];
    quickSplineEvaluate(section->curve, evaluated, parameter, 0);
    quickSplineEvaluate(section->curve, tangent, parameter, 1);
    quickSplineEvaluate(section->curve, evaluated, parameter, 0);
    float direction[2];
    vectorNormalize2(direction, tangent);
    const float alongSection = evaluated[3] * section->distanceScale;
    float distance = section->startDistance + alongSection;
    if (route->reverse) {
        direction[0] = -direction[0]; direction[1] = -direction[1];
        distance = route->length - distance;
    }
    location->parameter = parameter;
    location->point[0] = point[0]; location->point[1] = point[1];
    location->direction[0] = direction[0]; location->direction[1] = direction[1];
    location->distance = distance;
    location->progress = finiteRatio(distance, route->length);
}

TrackRouteLocation* trackRouteLocationAtPoint(const TrackRoute* route, TrackRouteLocation* location, const float point[2])
{
    trackRouteLocationInitialize(location);
    trackRouteUpdateLocation(route, location, point);
    return location;
}

void trackRouteRelocate(const TrackRoute* route, TrackRouteLocation* location, const float point[2], float distance)
{
    location->node = NULL;
    const TrackRouteNode* node = trackRouteFindNodeNearDistance(route, point, distance);
    if (node) trackRouteUpdateLocation(route, location, node->point);
    trackRouteUpdateLocation(route, location, point);
}

// Native resource storage for the proved PAL207B28 TrackPath loader.
// Source sections/nodes retain their authored order; wider host pointers
// and owned spline arrays replace the relocated32-bit resource references.
namespace hp2 {
namespace {
float real(const std::uint8_t* bytes)
{
    const auto bits = ReadU32(bytes);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    Require(std::isfinite(value), "Route contains a non-finite coordinate or parameter");
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

TrackRouteData::TrackRouteData(const std::vector<std::uint8_t>& track, bool reverse)
{
    Chunk sectionChunk{}, nodeChunk{};
    bool found = false;
    for (const auto& root : ReadChunks(track)) {
        if (root.id != 0x80034147u) continue;
        Require(!found, "Track repeats its route container");
        found = true;
        for (const auto& chunk : ReadChunks(track, root.payloadOffset, root.size)) {
            if (chunk.id == 0x34148u) {
                Require(sectionChunk.id == 0, "Route repeats its section array");
                sectionChunk = chunk;
            } else if (chunk.id == 0x34149u) {
                Require(nodeChunk.id == 0, "Route repeats its node array");
                nodeChunk = chunk;
            }
        }
    }
    Require(found && sectionChunk.id && nodeChunk.id, "Track is missing its route arrays");
    Require(sectionChunk.size && sectionChunk.size % 40 == 0,
            "Route section array contains a partial record");
    Require(nodeChunk.size && nodeChunk.size % 20 == 0,
            "Route node array contains a partial record");
    const std::size_t sectionCount = sectionChunk.size / 40;
    const std::size_t nodeCount = nodeChunk.size / 20;
    Require(sectionCount <= 128 && nodeCount <= std::numeric_limits<std::int16_t>::max(),
            "Route exceeds its original index range");
    sections_.resize(sectionCount);
    nodes_.resize(nodeCount);
    for (std::size_t index = 0; index < sectionCount; ++index) {
        const auto* encoded = track.data() + sectionChunk.payloadOffset + index * 40;
        auto& section = sections_[index];
        section.flags = encoded[0];
        std::memcpy(&section.closed, encoded + 1, 1);
        section.firstNode = half(encoded + 2);
        section.endNode = half(encoded + 4);
        section.nodeCount = half(encoded + 6);
        section.startDistance = real(encoded + 8);
        section.nominalLength = real(encoded + 12);
        section.distanceScale = real(encoded + 16);
        for (int lane = 0; lane < 4; ++lane) section.endpoints[lane] = real(encoded + 20 + lane * 4);
        Require(section.firstNode >= 0 && section.endNode >= section.firstNode &&
                static_cast<std::size_t>(section.endNode) <= nodeCount &&
                section.nodeCount >= 2 &&
                static_cast<std::size_t>(section.firstNode) + section.nodeCount <= nodeCount,
                "Route section references nodes outside its array");
    }
    for (std::size_t index = 0; index < nodeCount; ++index) {
        const auto* encoded = track.data() + nodeChunk.payloadOffset + index * 20;
        auto& node = nodes_[index];
        node.point[0] = real(encoded);
        node.point[1] = real(encoded + 4);
        std::memcpy(&node.section, encoded + 8, 1);
        node.flags = encoded[9];
        Require(node.section >= 0 && static_cast<std::size_t>(node.section) < sectionCount,
                "Route node references an unknown section");
        for (int link = 0; link < 3; ++link) {
            node.neighbors[link] = half(encoded + 10 + link * 2);
            Require(node.neighbors[link] == -1 || (node.neighbors[link] >= 0 &&
                    static_cast<std::size_t>(node.neighbors[link]) < nodeCount),
                    "Route node references an unknown neighbor");
        }
        node.distance = real(encoded + 16);
    }
    route_.reverse = reverse ? 1 : 0;
    route_.sectionCount = static_cast<std::int32_t>(sectionCount);
    route_.sections = sections_.data();
    route_.nodeCount = static_cast<std::int32_t>(nodeCount);
    route_.nodes = nodes_.data();
    if (!trackRouteBuildCurves(&route_)) {
        trackRouteDispose(&route_);
        throw std::bad_alloc();
    }
}

TrackRouteData::~TrackRouteData()
{
    trackRouteDispose(&route_);
}

} // namespace hp2
