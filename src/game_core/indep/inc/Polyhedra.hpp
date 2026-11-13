#ifndef NFSHP2_INDEP_POLYHEDRA_HPP
#define NFSHP2_INDEP_POLYHEDRA_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Native storage for the three feature kinds read by the final PAL collision
 * routines. These are not binary layouts. Adjacency arrays, their referenced
 * features and the geometry storage must outlive queries. A manifold edge has
 * its two endpoint vertices first, then its two neighboring faces. A face has
 * three or four edges; a vertex contains its incident edges. Counts/capacities
 * fit positive int16_t, adjacency counts fit positive int8_t, coordinates and
 * arithmetic intermediates are finite. Inputs/outputs are distinct except
 * where a mutator explicitly changes its own feature. */
typedef struct CollisionFeature CollisionFeature;
typedef struct CollisionPlane {
    float normal[3];
    float offset;
    CollisionFeature* feature;
} CollisionPlane;

struct CollisionFeature {
    int8_t kind;                 /* 0 vertex, 1 edge, 2 face */
    int8_t adjacentCount;
    int16_t encodedDirection[3]; /* edge direction, in units of 1/8192 */
    float vector[3];             /* vertex position or face normal */
    float offset;                /* face plane constant */
    CollisionPlane* adjacent;
};

typedef struct CollisionGeometry {
    CollisionFeature* vertices;
    CollisionFeature* edges;
    CollisionFeature* faces;
    CollisionPlane* planes;
    int16_t vertexCount, edgeCount, faceCount, planeCount;
    int16_t vertexCapacity, edgeCapacity, faceCapacity, planeCapacity;
    int32_t ownsStorage;
    void* display;
} CollisionGeometry;

/* Serialized links are indices, kept in separate typed storage on the host. */
typedef struct CollisionGeometryLinks {
    uint32_t* vertexPlanes;
    uint32_t* edgePlanes;
    uint32_t* facePlanes;
    uint32_t* planeFeatures;
} CollisionGeometryLinks;

typedef struct CollisionTransformedVertex {
    const CollisionFeature* feature;
    float point[4];
} CollisionTransformedVertex;

typedef struct CollisionTransformedEdge {
    const CollisionFeature* feature;
    float first[4], second[4], direction[4];
} CollisionTransformedEdge;

typedef struct CollisionClip {
    float lower, upper;
    CollisionFeature* lowerFeature;
    CollisionFeature* upperFeature;
    int32_t boundary;
} CollisionClip;

float collisionPointDistance(const float first[4], const float second[4]);
float* collisionRelativeTransform(float output[16], const float firstFrame[16],
                                  const float firstPosition[4], const float secondFrame[16],
                                  const float secondPosition[4]);
void collisionTransformDirectionTo(float output[4], const float frame[16], const float value[4]);
void collisionTransformDirection(float output[4], const float frame[16], const float value[4]);
void collisionTransformPoint(float output[4], const float frame[16], const float value[4]);
void collisionTransformVertex(CollisionTransformedVertex* output, const float frame[16],
                               const CollisionFeature* vertex);
void collisionTransformEdge(CollisionTransformedEdge* output, const float frame[16],
                             const CollisionFeature* edge);
CollisionClip* collisionClipInitialize(CollisionClip* clip);
int32_t collisionClipPlanes(CollisionClip* clip, const CollisionTransformedEdge* edge,
                            const CollisionPlane* planes, int32_t count);
CollisionFeature* collisionClipVertex(CollisionClip* clip, const CollisionTransformedEdge* edge,
                                      const float* point, const CollisionPlane* planes, int32_t count);
CollisionFeature* collisionClipFace(CollisionClip* clip, const CollisionTransformedEdge* edge,
                                    const float* plane, const CollisionPlane* planes, int32_t count);
void collisionVertexSetPlanes(CollisionFeature* vertex);
CollisionFeature* collisionNextEdgeInFace(const CollisionFeature* vertex,
                                         const CollisionFeature* previousEdge,
                                         const CollisionFeature* face);
void collisionVertexMove(CollisionFeature* vertex, const float delta[4]);
void collisionEdgeSetPlanes(CollisionFeature* edge);
void collisionFaceSetPlanes(CollisionFeature* face);
void collisionFaceMove(CollisionFeature* face, const float delta[4]);
CollisionGeometry* collisionGeometryInitialize(CollisionGeometry* geometry);
void collisionGeometryBind(CollisionGeometry* geometry, CollisionFeature* vertices, int16_t vertexCapacity,
                           CollisionFeature* edges, int16_t edgeCapacity,
                           CollisionFeature* faces, int16_t faceCapacity,
                           CollisionPlane* planes, int16_t planeCapacity);
CollisionPlane* collisionGeometryNewPlanes(CollisionGeometry* geometry, int32_t count);
CollisionFeature* collisionGeometryNewVertex(CollisionGeometry* geometry, const float point[3],
                                            int8_t adjacentCount, CollisionPlane* adjacent);
CollisionFeature* collisionGeometryNewEdge(CollisionGeometry* geometry, const float direction[3],
                                          int8_t adjacentCount, CollisionPlane* adjacent);
CollisionFeature* collisionGeometryNewFace(CollisionGeometry* geometry, const float normal[3],
                                          int8_t adjacentCount, CollisionPlane* adjacent, float offset);
void collisionFeaturePoints(const CollisionFeature* feature, float output[4][3], const float frame[16]);
void collisionGeometrySetPlanes(CollisionGeometry* geometry);
uint32_t collisionGeometryFeatureCode(const CollisionGeometry* geometry, const CollisionFeature* feature);
CollisionFeature* collisionGeometryFeature(const CollisionGeometry* geometry, uint32_t code);
void collisionGeometryEncodeLinks(const CollisionGeometry* geometry, CollisionGeometryLinks* links);
void collisionGeometryDecodeLinks(CollisionGeometry* geometry, const CollisionGeometryLinks* links);
void collisionGeometryBindIndexed(CollisionGeometry* geometry, CollisionFeature* vertices, int16_t vertexCount,
                                  CollisionFeature* edges, int16_t edgeCount,
                                  CollisionFeature* faces, int16_t faceCount,
                                  CollisionPlane* planes, int16_t planeCount,
                                  const CollisionGeometryLinks* links);
float collisionGeometryMaximumViolation(const CollisionGeometry* geometry);
void collisionGeometryRecenter(CollisionGeometry* geometry, const float center[4], int32_t* recentered);

/* The original primitive retains its exact feature and adjacency order.
 * This owner must outlive the geometry and every feature witness into it. */
typedef struct CollisionBox {
    CollisionGeometry geometry;
    CollisionFeature vertices[8];
    CollisionFeature edges[12];
    CollisionFeature faces[6];
    CollisionPlane planes[96];
} CollisionBox;

CollisionBox* collisionBoxInitialize(CollisionBox* box, const float maximum[3], const float minimum[3]);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hp2 {
class GameResources;
struct CarDefinition;

class CollisionResource final {
public:
    CollisionResource() = default;
    CollisionResource(const CollisionResource&) = delete;
    CollisionResource& operator=(const CollisionResource&) = delete;
    CollisionResource(CollisionResource&&) = delete;
    CollisionResource& operator=(CollisionResource&&) = delete;
    CollisionGeometry& Geometry() noexcept { return geometry_; }
    const CollisionGeometry& Geometry() const noexcept { return geometry_; }
    const std::string& Name() const noexcept { return name_; }
    std::uint32_t Key() const noexcept { return key_; }
    bool UsesDefaultCarHull() const noexcept { return box_ != nullptr; }
    void Recenter(const float center[4]);

private:
    friend std::vector<std::unique_ptr<CollisionResource>> ReadCollisionResources(const std::vector<std::uint8_t>&, bool);
    friend std::unique_ptr<CollisionResource> LoadCarCollisionResource(const GameResources&, const CarDefinition&);
    friend struct CollisionResourceReader;
    CollisionGeometry geometry_{};
    std::vector<CollisionFeature> vertices_, edges_, faces_;
    std::vector<CollisionPlane> planes_;
    std::string name_;
    std::uint32_t key_ = 0;
    std::int32_t recentered_ = 0;
    std::unique_ptr<CollisionBox> box_;
};

// Reads the original chunked resource. Every reference/index/type/count is
// validated before pointers are linked. The caller may allow absent geometry;
// malformed records remain an error. The car loader preserves the original
// default primitive only when the requested authored hull does not exist.
std::vector<std::unique_ptr<CollisionResource>> ReadCollisionResources(const std::vector<std::uint8_t>& data, bool allowEmpty = false);
std::unique_ptr<CollisionResource> LoadCarCollisionResource(const GameResources& resources,
                                                         const CarDefinition& car);
CollisionResource* FindPipelinePolyhedron(CollisionResource* const* resources,std::size_t count,
    std::uint32_t key,const float* center);
} // namespace hp2

#endif

#endif
