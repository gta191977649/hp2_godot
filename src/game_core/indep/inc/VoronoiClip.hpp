#ifndef NFSHP2_INDEP_VORONOICLIP_HPP
#define NFSHP2_INDEP_VORONOICLIP_HPP

#include "indep/inc/Polyhedra.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include <math.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CollisionWitness {
    int32_t side;
    CollisionFeature* first;
    CollisionFeature* second;
    CollisionGeometry* firstGeometry;
    CollisionGeometry* secondGeometry;
    float firstPoint[4], secondPoint[4];
    float distance;
    void* firstDisplay;
    void* secondDisplay;
} CollisionWitness;

struct ReplaySnapshot;
void collisionWitnessSnapshot(CollisionWitness* witness, struct ReplaySnapshot* snapshot);

CollisionWitness* collisionWitnessInitialize(CollisionWitness* witness, CollisionGeometry* first, CollisionGeometry* second);
int32_t collisionWitnessRemember(const CollisionWitness* witness, CollisionFeature** firstHistory,
                                 CollisionFeature** secondHistory, int32_t index);
int32_t collisionWitnessVertices(CollisionWitness* witness, const float featureFrame[16],
                                 const float otherFrame[16], CollisionTransformedVertex* feature,
                                 CollisionTransformedVertex* other);

/* side identifies the vertex owner (1 first, 2 second). Frames convert each
 * owner's feature to the opposite owner's coordinates. Both feature graphs
 * must satisfy the CollisionGeometry topology contract. Witness point XYZ is
 * meaningful; W is padding and is not part of the witness result. Returns 0
 * after selecting a different feature, 1 for separation, or 2 at a boundary. */
int32_t collisionWitnessVertexEdge(CollisionWitness* witness, const float featureFrame[16],
                                   const float otherFrame[16], CollisionTransformedVertex* vertex,
                                   CollisionTransformedEdge* edge);

int32_t collisionWitnessVertexFace(CollisionWitness* witness, const float featureFrame[16],
                                   CollisionTransformedVertex* vertex);
/* feature is an edge. Its first two planes bound endpoints and its remaining
 * two planes bound neighboring faces. featureRole is 1 or 2 in the ordering
 * selected by witness.side, rather than an absolute geometry index. */
int32_t collisionWitnessEdgeRegions(CollisionWitness* witness, const CollisionTransformedEdge* segment,
                                    const CollisionFeature* feature, int32_t featureRole);
int32_t collisionWitnessEdges(CollisionWitness* witness, const float featureFrame[16],
                              const float otherFrame[16], CollisionTransformedEdge* first,
                              CollisionTransformedEdge* second);
int32_t collisionWitnessEdgeFace(CollisionWitness* witness, const float featureFrame[16],
                                 const float otherFrame[16], CollisionTransformedEdge* edge);

/* Persistent features belong to the supplied geometries. Frames describe
 * orientation and positions describe translation separately. randomState is
 * the caller's shared simulation sequence, consumed only on witness cycles.
 * Diagnostic drawing is outside this production query. */
int32_t collisionWitnessQuery(CollisionWitness* witness, const float firstFrame[16],
                              const float firstPosition[4], const float secondFrame[16],
                              const float secondPosition[4], uint32_t* randomState);

static inline float collisionGeometryDot3(const float a[3], const float b[3])
{
    const float x = a[0] * b[0], y = a[1] * b[1], z = a[2] * b[2];
    return (x + y) + z;
}

static inline void collisionGeometrySubtract4(float out[4], const float a[4], const float b[4])
{
    for (int lane = 0; lane < 4; ++lane) out[lane] = a[lane] - b[lane];
}

static inline void collisionGeometryPointAlong(float out[4], const CollisionTransformedEdge* edge, float parameter)
{
    for (int lane = 0; lane < 4; ++lane) {
        const float scaled = edge->direction[lane] * parameter;
        out[lane] = scaled + edge->second[lane];
    }
}

#ifdef __cplusplus
}
#endif

#endif
