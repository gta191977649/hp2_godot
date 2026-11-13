#ifndef NFSHP2_TRACK_PATH_HPP
#define NFSHP2_TRACK_PATH_HPP

#include "indep/inc/QuickSpline.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Native route topology decoded from the authored route section/node chunks.
 * These records are not console layouts. Nodes use XY; route elevation and
 * collision elevation belong to TerrainSurface. All referenced indices must
 * be in range, sections contain at least two nodes, and splines are finite. */
typedef struct TrackRouteNode {
    float point[2];
    int8_t section;
    uint8_t flags;
    int16_t neighbors[3];
    float distance;
} TrackRouteNode;

typedef struct TrackRouteSection {
    uint8_t flags;
    int8_t closed;
    int16_t firstNode;
    int16_t endNode;
    int16_t nodeCount;
    float startDistance;
    float nominalLength;
    float distanceScale;
    float endpoints[4];
    QuickSpline* curve;
} TrackRouteSection;

typedef struct TrackRoute {
    float length;
    int32_t reverse;
    int32_t sectionCount;
    TrackRouteSection* sections;
    int32_t nodeCount;
    TrackRouteNode* nodes;
} TrackRoute;

typedef struct TrackRouteLocation {
    float point[2];
    const TrackRouteNode* node;
    float parameter;
    float progress;
    float distance;
    float direction[2];
} TrackRouteLocation;
struct ReplaySnapshot;
void trackRouteLocationSnapshot(const TrackRoute* route,TrackRouteLocation* location,
    struct ReplaySnapshot* snapshot);

// Signed shortest difference on a periodic coordinate. The caller supplies
// the period used by the game state. A result can still be outside the half
// interval when the two inputs are more than one whole turn apart.
float trackWrappedDelta(float period, float position, float reference);

// Fixed-period variants used by the original simulation.
float trackWrappedDelta100(float position, float reference);
float trackWrappedDeltaUnit(float position, float reference);


/* Created curves are owned by the route. An existing non-null section curve
 * is retained unchanged and must use the same native allocation contract if
 * trackRouteDispose is called. Topology arrays themselves remain borrowed. */
int32_t trackRouteBuildCurves(TrackRoute* route);
void trackRouteDispose(TrackRoute* route);
const TrackRouteNode* trackRouteFindNearestNode(const TrackRoute* route, const float point[2]);
const TrackRouteNode* trackRouteFindNodeNearDistance(const TrackRoute* route, const float point[2], float distance);
const TrackRouteNode* trackRouteFindLocalNode(const TrackRoute* route, const float point[2], const TrackRouteNode* node);
void trackRouteUpdateLocation(const TrackRoute* route, TrackRouteLocation* location, const float point[2]);
/* Convenience accessor to the shared trackWrappedDelta implementation. */
float trackRouteDistanceDifference(const TrackRoute* route, float first, float second);
TrackRouteLocation* trackRouteLocationInitialize(TrackRouteLocation* location);
TrackRouteLocation* trackRouteLocationAtPoint(const TrackRoute* route, TrackRouteLocation* location, const float point[2]);
void trackRouteRelocate(const TrackRoute* route, TrackRouteLocation* location, const float point[2], float distance);

/* Full PAL208350/208820 distance overloads. The zero result is a native
 * invalid-input/undefined-console-scratch failure, never a successful position.
 * Failure leaves the caller's coordinate unchanged; a finite positive route,
 * live spline and nonzero segment tangent are required for publication. */
int32_t trackRoutePositionAtDistance(const TrackRoute* route,TrackRouteLocation* location,
    float distance,float across,int32_t sectionIndex);
int32_t trackCoordinatePositionAtDistance(const TrackRoute* route,TrackRouteLocation* location,
    float distance,float across);

#ifdef __cplusplus
}

#include <cstdint>
#include <vector>

namespace hp2 {

class TrackRouteData final {
public:
    explicit TrackRouteData(const std::vector<std::uint8_t>& track, bool reverse = false);
    ~TrackRouteData();
    TrackRouteData(const TrackRouteData&) = delete;
    TrackRouteData& operator=(const TrackRouteData&) = delete;
    TrackRouteData(TrackRouteData&&) = delete;
    TrackRouteData& operator=(TrackRouteData&&) = delete;
    TrackRoute& Native() noexcept { return route_; }
    const TrackRoute& Native() const noexcept { return route_; }

private:
    TrackRoute route_{};
    std::vector<TrackRouteSection> sections_;
    std::vector<TrackRouteNode> nodes_;
};

} // namespace hp2

#endif
#endif
