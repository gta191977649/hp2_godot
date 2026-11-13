#include "indep/inc/VoronoiClip.hpp"
#include "indep/inc/Contact.hpp"
#include "indep/inc/Polyhedra.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/VoronoiClip.hpp"

#include <math.h>
#include <stddef.h>
#include <string.h>

CollisionWitness* collisionWitnessInitialize(CollisionWitness* witness, CollisionGeometry* first, CollisionGeometry* second)
{
    for(int lane=0;lane<3;++lane) witness->firstPoint[lane]=witness->secondPoint[lane]=0.0f;
    witness->first=first->vertices;
    witness->second=second->vertices;
    witness->firstGeometry=first;
    witness->secondGeometry=second;
    witness->side=0;
    witness->distance=0.0f;
    witness->firstDisplay=witness->secondDisplay=NULL;
    return witness;
}

float collisionPointDistance(const float first[4], const float second[4])
{
    float delta[4];
    for (int lane = 0; lane < 3; ++lane) delta[lane] = first[lane] - second[lane];
    return sqrtf(collisionGeometryDot3(delta, delta));
}

void collisionTransformDirection(float output[4], const float frame[16], const float value[4])
{
    float result[4];
    for (int row = 0; row < 4; ++row) {
        float sum = frame[row] * value[0];
        const float second = frame[4 + row] * value[1];
        sum = sum + second;
        const float third = frame[8 + row] * value[2];
        result[row] = sum + third;
    }
    memcpy(output, result, sizeof(result));
}

void collisionTransformPoint(float output[4], const float frame[16], const float value[4])
{
    vectorMatrixTransformPoint(output, frame, value);
}

void collisionTransformVertex(CollisionTransformedVertex* output, const float frame[16],
                               const CollisionFeature* vertex)
{
    const float point[4] = {vertex->vector[0], vertex->vector[1], vertex->vector[2], 0.0f};
    output->feature = vertex;
    vectorMatrixTransformPoint(output->point, frame, point);
}

void collisionTransformEdge(CollisionTransformedEdge* output, const float frame[16],
                             const CollisionFeature* edge)
{
    output->feature = edge;
    const CollisionFeature* first = edge->adjacent[1].feature;
    const CollisionFeature* second = edge->adjacent[0].feature;
    const float firstPoint[4] = {first->vector[0], first->vector[1], first->vector[2], 0.0f};
    const float secondPoint[4] = {second->vector[0], second->vector[1], second->vector[2], 0.0f};
    vectorMatrixTransformPoint(output->first, frame, firstPoint);
    vectorMatrixTransformPoint(output->second, frame, secondPoint);
    memcpy(output->direction, output->first, sizeof(output->direction));
    for (int lane = 0; lane < 3; ++lane) output->direction[lane] = output->first[lane] - output->second[lane];
}

CollisionClip* collisionClipInitialize(CollisionClip* clip)
{
    clip->lower = 0.0f;
    clip->upper = 1.0f;
    clip->lowerFeature = clip->upperFeature = NULL;
    clip->boundary = 0;
    return clip;
}

int32_t collisionClipPlanes(CollisionClip* clip, const CollisionTransformedEdge* edge,
                            const CollisionPlane* planes, int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        const CollisionPlane* plane = &planes[index];
        const float first = collisionGeometryDot3(plane->normal, edge->second) - plane->offset;
        const float second = collisionGeometryDot3(plane->normal, edge->first) - plane->offset;
        if (first < 0.0f) {
            if (second < 0.0f) {
                clip->lowerFeature = clip->upperFeature = plane->feature;
                return 0;
            }
            const float denominator = first - second;
            const float fraction = finiteRatio(first, denominator);
            if (clip->lower < fraction) {
                clip->lower = fraction;
                clip->lowerFeature = plane->feature;
                if (clip->upper < fraction) return 0;
            }
        } else if (second < 0.0f) {
            const float denominator = first - second;
            const float fraction = finiteRatio(first, denominator);
            if (fraction < clip->upper) {
                clip->upper = fraction;
                clip->upperFeature = plane->feature;
                if (fraction < clip->lower) return 0;
            }
        }
    }
    return 1;
}

void collisionVertexSetPlanes(CollisionFeature* vertex)
{
    for (int index = 0; index < vertex->adjacentCount; ++index) {
        CollisionPlane* plane = &vertex->adjacent[index];
        CollisionFeature* edge = plane->feature;
        for (int lane = 0; lane < 3; ++lane) {
            float direction = (float)edge->encodedDirection[lane] * 0.0001220703125f;
            if (edge->adjacent[1].feature != vertex) direction = 0.0f - direction;
            plane->normal[lane] = direction;
        }
        plane->offset = collisionGeometryDot3(vertex->vector, plane->normal);
    }
}

CollisionFeature* collisionNextEdgeInFace(const CollisionFeature* vertex,
                                         const CollisionFeature* previousEdge,
                                         const CollisionFeature* face)
{
    for (int index = 0; index < vertex->adjacentCount; ++index) {
        CollisionFeature* edge = vertex->adjacent[index].feature;
        if (edge != previousEdge)
            for (int other = 0; other < face->adjacentCount; ++other)
                if (edge == face->adjacent[other].feature) return edge;
    }
    return NULL;
}

void collisionVertexMove(CollisionFeature* vertex, const float delta[4])
{
    for (int lane = 0; lane < 3; ++lane) vertex->vector[lane] = vertex->vector[lane] + delta[lane];
}

void collisionEdgeSetPlanes(CollisionFeature* edge)
{
    float direction[3];
    for (int lane = 0; lane < 3; ++lane) direction[lane] = (float)edge->encodedDirection[lane] * 0.0001220703125f;
    CollisionPlane* entries = edge->adjacent;
    for (int lane = 0; lane < 3; ++lane) {
        entries[0].normal[lane] = direction[lane];
        entries[1].normal[lane] = 0.0f - direction[lane];
    }
    entries[0].offset = collisionGeometryDot3(direction, entries[0].feature->vector);
    entries[1].offset = collisionGeometryDot3(entries[1].normal, entries[1].feature->vector);
    for (int side = 2; side < 4; ++side) {
        CollisionFeature* face = entries[side].feature;
        for (int index = 0; index < face->adjacentCount; ++index) {
            const CollisionPlane* facePlane = &face->adjacent[index];
            if (facePlane->feature == edge) {
                for (int lane = 0; lane < 3; ++lane) entries[side].normal[lane] = 0.0f - facePlane->normal[lane];
                entries[side].offset = collisionGeometryDot3(entries[side].normal, entries[1].feature->vector);
                break;
            }
        }
    }
}

void collisionFaceSetPlanes(CollisionFeature* face)
{
    for (int index = 0; index < face->adjacentCount; ++index) {
        CollisionPlane* plane = &face->adjacent[index];
        CollisionFeature* edge = plane->feature;
        CollisionFeature* endpoint = edge->adjacent[0].feature;
        float direction[3];
        for (int lane = 0; lane < 3; ++lane) direction[lane] = (float)edge->encodedDirection[lane] * 0.0001220703125f;
        for (int lane = 0; lane < 3; ++lane) {
            const int next = (lane + 1) % 3, last = (lane + 2) % 3;
            const float positive = face->vector[next] * direction[last];
            const float negative = direction[next] * face->vector[last];
            plane->normal[lane] = positive - negative;
        }
        plane->offset = collisionGeometryDot3(plane->normal, endpoint->vector);
        const CollisionFeature* nextEdge = collisionNextEdgeInFace(endpoint, edge, face);
        const CollisionFeature* other = nextEdge->adjacent[0].feature;
        if (other == endpoint) other = nextEdge->adjacent[1].feature;
        const float distance = collisionGeometryDot3(plane->normal, other->vector) - plane->offset;
        if (distance < 0.0f) {
            for (int lane = 0; lane < 3; ++lane) plane->normal[lane] = 0.0f - plane->normal[lane];
            plane->offset = collisionGeometryDot3(plane->normal, endpoint->vector);
        }
    }
}

void collisionFaceMove(CollisionFeature* face, const float delta[4])
{
    face->offset = face->offset + collisionGeometryDot3(delta, face->vector);
}

CollisionGeometry* collisionGeometryInitialize(CollisionGeometry* geometry)
{
    geometry->ownsStorage = 1;
    geometry->display = NULL;
    geometry->vertexCount = geometry->edgeCount = geometry->faceCount = geometry->planeCount = 0;
    geometry->vertexCapacity = geometry->edgeCapacity = geometry->faceCapacity = geometry->planeCapacity = 0;
    geometry->vertices = geometry->edges = geometry->faces = NULL;
    geometry->planes = NULL;
    return geometry;
}

void collisionGeometryBind(CollisionGeometry* geometry, CollisionFeature* vertices, int16_t vertexCapacity,
                           CollisionFeature* edges, int16_t edgeCapacity,
                           CollisionFeature* faces, int16_t faceCapacity,
                           CollisionPlane* planes, int16_t planeCapacity)
{
    geometry->vertices = vertices; geometry->edges = edges; geometry->faces = faces; geometry->planes = planes;
    geometry->vertexCapacity = vertexCapacity; geometry->edgeCapacity = edgeCapacity;
    geometry->faceCapacity = faceCapacity; geometry->planeCapacity = planeCapacity;
    geometry->ownsStorage = 0;
    geometry->vertexCount = geometry->edgeCount = geometry->faceCount = geometry->planeCount = 0;
}

CollisionPlane* collisionGeometryNewPlanes(CollisionGeometry* geometry, int32_t count)
{
    const int32_t oldCount = geometry->planeCount;
    if (geometry->planeCapacity < oldCount + count) return NULL;
    CollisionPlane* result = geometry->planes + oldCount;
    geometry->planeCount = (int16_t)(oldCount + count);
    for (int32_t index = 0; index < count; ++index) {
        result[index].normal[0] = result[index].normal[1] = 0.0f;
        result[index].normal[2] = 1.0f;
        result[index].offset = 0.0f; result[index].feature = NULL;
    }
    return result;
}

CollisionFeature* collisionGeometryNewVertex(CollisionGeometry* geometry, const float point[3],
                                            int8_t adjacentCount, CollisionPlane* adjacent)
{
    if (geometry->vertexCount >= geometry->vertexCapacity) return NULL;
    CollisionFeature* vertex = &geometry->vertices[geometry->vertexCount++];
    vertex->kind = 0; vertex->adjacentCount = adjacentCount; vertex->adjacent = adjacent;
    memcpy(vertex->vector, point, sizeof(vertex->vector));
    return vertex;
}

CollisionFeature* collisionGeometryNewEdge(CollisionGeometry* geometry, const float direction[3],
                                          int8_t adjacentCount, CollisionPlane* adjacent)
{
    if (geometry->edgeCount >= geometry->edgeCapacity) return NULL;
    CollisionFeature* edge = &geometry->edges[geometry->edgeCount++];
    edge->kind = 1; edge->adjacentCount = adjacentCount; edge->adjacent = adjacent;
    for (int lane = 0; lane < 3; ++lane) {
        const float scaled = direction[lane] * 8192.0f;
        edge->encodedDirection[lane] = (int16_t)(uint16_t)(uint32_t)(int32_t)scaled;
    }
    return edge;
}

CollisionFeature* collisionGeometryNewFace(CollisionGeometry* geometry, const float normal[3],
                                          int8_t adjacentCount, CollisionPlane* adjacent, float offset)
{
    if (geometry->faceCount >= geometry->faceCapacity) return NULL;
    CollisionFeature* face = &geometry->faces[geometry->faceCount++];
    face->kind = 2; face->adjacentCount = adjacentCount; face->adjacent = adjacent; face->offset = offset;
    memcpy(face->vector, normal, sizeof(face->vector));
    return face;
}

void collisionGeometrySetPlanes(CollisionGeometry* geometry)
{
    for (int index = 0; index < geometry->vertexCount; ++index) collisionVertexSetPlanes(&geometry->vertices[index]);
    for (int index = 0; index < geometry->faceCount; ++index) collisionFaceSetPlanes(&geometry->faces[index]);
    for (int index = 0; index < geometry->edgeCount; ++index) collisionEdgeSetPlanes(&geometry->edges[index]);
}

uint32_t collisionGeometryFeatureCode(const CollisionGeometry* geometry, const CollisionFeature* feature)
{
    if (!feature) return UINT32_MAX;
    if (feature->kind == 0) return (uint32_t)(feature - geometry->vertices) << 4;
    if (feature->kind == 1) return ((uint32_t)(feature - geometry->edges) << 4) | 1u;
    if (feature->kind == 2) return ((uint32_t)(feature - geometry->faces) << 4) | 2u;
    return UINT32_MAX;
}

CollisionFeature* collisionGeometryFeature(const CollisionGeometry* geometry, uint32_t code)
{
    if (code == UINT32_MAX) return NULL;
    const uint32_t index = code >> 4;
    switch (code & 15u) {
    case 0: return &geometry->vertices[index];
    case 1: return &geometry->edges[index];
    case 2: return &geometry->faces[index];
    default: return NULL;
    }
}

void collisionGeometryEncodeLinks(const CollisionGeometry* geometry, CollisionGeometryLinks* links)
{
    for(int index=0;index<geometry->vertexCount;++index) links->vertexPlanes[index]=(uint32_t)(geometry->vertices[index].adjacent-geometry->planes);
    for(int index=0;index<geometry->edgeCount;++index) links->edgePlanes[index]=(uint32_t)(geometry->edges[index].adjacent-geometry->planes);
    for(int index=0;index<geometry->faceCount;++index) links->facePlanes[index]=(uint32_t)(geometry->faces[index].adjacent-geometry->planes);
    for(int index=0;index<geometry->planeCount;++index) links->planeFeatures[index]=collisionGeometryFeatureCode(geometry,geometry->planes[index].feature);
}

void collisionGeometryDecodeLinks(CollisionGeometry* geometry, const CollisionGeometryLinks* links)
{
    for(int index=0;index<geometry->vertexCount;++index) geometry->vertices[index].adjacent=geometry->planes+links->vertexPlanes[index];
    for(int index=0;index<geometry->edgeCount;++index) geometry->edges[index].adjacent=geometry->planes+links->edgePlanes[index];
    for(int index=0;index<geometry->faceCount;++index) geometry->faces[index].adjacent=geometry->planes+links->facePlanes[index];
    for(int index=0;index<geometry->planeCount;++index) geometry->planes[index].feature=collisionGeometryFeature(geometry,links->planeFeatures[index]);
}

void collisionGeometryBindIndexed(CollisionGeometry* geometry, CollisionFeature* vertices, int16_t vertexCount,
                                  CollisionFeature* edges, int16_t edgeCount,
                                  CollisionFeature* faces, int16_t faceCount,
                                  CollisionPlane* planes, int16_t planeCount,
                                  const CollisionGeometryLinks* links)
{
    collisionGeometryBind(geometry,vertices,vertexCount,edges,edgeCount,faces,faceCount,planes,planeCount);
    geometry->vertexCount=vertexCount;geometry->edgeCount=edgeCount;
    geometry->faceCount=faceCount;geometry->planeCount=planeCount;
    collisionGeometryDecodeLinks(geometry,links);
}

float* collisionRelativeTransform(float output[16], const float firstFrame[16],
                                  const float firstPosition[4], const float secondFrame[16],
                                  const float secondPosition[4])
{
    float relative[16], transposed[16];
    memcpy(relative, secondFrame, sizeof(relative));
    for (int lane = 0; lane < 4; ++lane) {
        const float translated = relative[12 + lane] + secondPosition[lane];
        relative[12 + lane] = translated - firstPosition[lane];
    }
    relative[15] = 1.0f;
    vectorMatrixTranspose(transposed, firstFrame);
    vectorMatrixMultiply(output, transposed, relative);
    return output;
}

static float dot3(const float first[3],const float second[3])
{
    const float x=first[0]*second[0],y=first[1]*second[1],z=first[2]*second[2];
    return (x+y)+z;
}

static void replaceFeature(CollisionWitness* witness,int32_t side,CollisionFeature* feature)
{
    if(side==1)witness->first=feature;else witness->second=feature;
}

int32_t collisionWitnessVertexEdge(CollisionWitness* witness,const float featureFrame[16],
                                   const float otherFrame[16],CollisionTransformedVertex* vertex,
                                   CollisionTransformedEdge* edge)
{
    CollisionFeature* first=witness->side==1?witness->first:witness->second;
    CollisionFeature* second=witness->side==2?witness->first:witness->second;
    collisionTransformVertex(vertex,featureFrame,first);
    for(int index=0;index<4;++index) {
        const CollisionPlane* plane=&second->adjacent[index];
        if(dot3(plane->normal,vertex->point)-plane->offset<0.0f) {
            replaceFeature(witness,3-witness->side,plane->feature);
            return 0;
        }
    }
    collisionTransformEdge(edge,otherFrame,second);
    CollisionClip clip;collisionClipInitialize(&clip);
    const float point[4]={first->vector[0],first->vector[1],first->vector[2],0.0f};
    CollisionFeature* changed=collisionClipVertex(&clip,edge,point,first->adjacent,first->adjacentCount);
    if(changed) {
        replaceFeature(witness,witness->side,changed);
        return 0;
    }
    float* firstPoint=witness->side==1?witness->firstPoint:witness->secondPoint;
    float* secondPoint=witness->side==2?witness->firstPoint:witness->secondPoint;
    memcpy(firstPoint,point,sizeof(point));
    if(clip.boundary) {
        witness->distance=0.0f;
        memcpy(secondPoint,vertex->point,4*sizeof(float));
        return 2;
    }
    const CollisionFeature* endpoint=second->adjacent[0].feature;
    float direction[3],delta[3];
    for(int lane=0;lane<3;++lane) {
        direction[lane]=(float)second->encodedDirection[lane]*0.0001220703125f;
        delta[lane]=vertex->point[lane]-endpoint->vector[lane];
    }
    const float fraction=dot3(delta,direction);
    for(int lane=0;lane<3;++lane) {
        const float scaled=direction[lane]*fraction;
        secondPoint[lane]=endpoint->vector[lane]+scaled;
        delta[lane]=vertex->point[lane]-secondPoint[lane];
    }
    secondPoint[3]=0.0f;
    witness->distance=sqrtf(dot3(delta,delta));
    return 1;
}

static float planeDistance(const CollisionFeature* face, const float point[4])
{
    return dot3(face->vector, point) - face->offset;
}

static void projectPlane(float output[4], const float point[4], const CollisionFeature* face, float distance)
{
    const float scale = -distance;
    for (int lane = 0; lane < 3; ++lane) {
        const float component = face->vector[lane] * scale;
        output[lane] = point[lane] + component;
    }
    output[3] = 0.0f;
}

int32_t collisionWitnessVertexFace(CollisionWitness* witness, const float featureFrame[16],
                                   CollisionTransformedVertex* transformed)
{
    CollisionFeature* vertex = witness->side == 1 ? witness->first : witness->second;
    CollisionFeature* face = witness->side == 2 ? witness->first : witness->second;
    collisionTransformVertex(transformed, featureFrame, vertex);
    float minimum = 0.0f;
    int32_t violated = -1;
    for (int index = 0; index < face->adjacentCount; ++index) {
        const CollisionPlane* plane = &face->adjacent[index];
        const float distance = dot3(plane->normal, transformed->point) - plane->offset;
        if (distance < minimum) { minimum = distance; violated = index; }
    }
    if (violated != -1) {
        replaceFeature(witness, 3 - witness->side, face->adjacent[violated].feature);
        return 0;
    }
    const float distance = planeDistance(face, transformed->point);
    float* vertexPoint = witness->side == 1 ? witness->firstPoint : witness->secondPoint;
    float* facePoint = witness->side == 2 ? witness->firstPoint : witness->secondPoint;
    if (fabsf(distance) < 0.0001f) {
        witness->distance = 0.0f;
        memcpy(vertexPoint, vertex->vector, 3 * sizeof(float));
        memcpy(facePoint, transformed->point, 4 * sizeof(float));
        return 2;
    }
    for (int index = 0; index < vertex->adjacentCount; ++index) {
        CollisionFeature* edge = vertex->adjacent[index].feature;
        CollisionFeature* neighbor = edge->adjacent[0].feature;
        if (neighbor == vertex) neighbor = edge->adjacent[1].feature;
        CollisionTransformedVertex other;
        collisionTransformVertex(&other, featureFrame, neighbor);
        const float neighborDistance = planeDistance(face, other.point);
        if ((distance < 0.0f && distance < neighborDistance) ||
            (0.0f < distance && neighborDistance < distance)) {
            replaceFeature(witness, witness->side, edge);
            return 0;
        }
    }
    memcpy(vertexPoint, vertex->vector, 3 * sizeof(float));
    if (0.0f < distance) {
        witness->distance = distance;
        projectPlane(facePoint, transformed->point, face, distance);
        return 1;
    }
    const CollisionGeometry* geometry = witness->side == 2 ? witness->firstGeometry : witness->secondGeometry;
    float maximum = distance;
    CollisionFeature* selected = face;
    for (int index = 0; index < geometry->faceCount; ++index) {
        CollisionFeature* candidate = &geometry->faces[index];
        const float candidateDistance = planeDistance(candidate, transformed->point);
        if (maximum < candidateDistance) {
            maximum = candidateDistance;
            selected = candidate;
            replaceFeature(witness, 3 - witness->side, selected);
        }
    }
    witness->distance = maximum;
    projectPlane(facePoint, transformed->point, selected, maximum);
    return maximum < 0.0f ? 2 : 1;
}

int32_t collisionWitnessEdgeRegions(CollisionWitness* witness, const CollisionTransformedEdge* segment,
                                    const CollisionFeature* feature, int32_t featureRole)
{
    CollisionClip clip;
    collisionClipInitialize(&clip);
    CollisionFeature* changed = collisionClipVertex(&clip, segment, NULL, feature->adjacent, 2);
    if (changed) {
        replaceFeature(witness, featureRole == witness->side ? 1 : 2, changed);
        return 0;
    }
    if (clip.boundary) { witness->distance = 0.0f; return 2; }
    changed = collisionClipFace(&clip, segment, NULL, feature->adjacent + 2, 2);
    if (changed) {
        replaceFeature(witness, featureRole == witness->side ? 1 : 2, changed);
        return 0;
    }
    if (clip.boundary) { witness->distance = 0.0f; return 2; }
    return 1;
}

static void edgeDirection(float output[4], const CollisionFeature* edge)
{
    for (int lane = 0; lane < 3; ++lane)
        output[lane] = (float)edge->encodedDirection[lane] * 0.0001220703125f;
    output[3] = 0.0f;
}

static float closestEdgePoints(float firstPoint[4], float secondPoint[4],
                               const CollisionFeature* firstFeature, const CollisionFeature* secondFeature,
                               const float featureFrame[16], const float otherFrame[16],
                               const CollisionTransformedEdge* transformedOther)
{
    const float* start = firstFeature->adjacent[0].feature->vector;
    const float* end = firstFeature->adjacent[1].feature->vector;
    float direction[4], otherDirection[4], transformedDirection[4], delta[4], perpendicular[4];
    edgeDirection(otherDirection, secondFeature);
    collisionTransformDirection(transformedDirection, otherFrame, otherDirection);
    edgeDirection(direction, firstFeature);
    const float cosine = dot3(transformedDirection, direction);
    const float squaredCosine = cosine * cosine;
    const float denominator = 1.0f - squaredCosine;
    for (int lane = 0; lane < 3; ++lane) {
        delta[lane] = transformedOther->second[lane] - start[lane];
        const float projected = transformedDirection[lane] * cosine;
        perpendicular[lane] = direction[lane] - projected;
    }
    const float numerator = dot3(delta, perpendicular);
    if (denominator < 0.000001f) {
        memcpy(firstPoint, 0.0f < numerator ? end : start, 3 * sizeof(float));
        firstPoint[3] = 0.0f;
    } else {
        float parameter = finiteRatio(numerator, denominator);
        if (parameter < 0.0f) parameter = 0.0f;
        else {
            const float start4[4] = {start[0], start[1], start[2], 0.0f};
            const float end4[4] = {end[0], end[1], end[2], 0.0f};
            const float length = collisionPointDistance(end4, start4);
            if (length < parameter) parameter = length;
        }
        for (int lane = 0; lane < 3; ++lane) {
            const float offset = direction[lane] * parameter;
            firstPoint[lane] = start[lane] + offset;
        }
        firstPoint[3] = 0.0f;
    }
    float transformedPoint[4];
    collisionTransformPoint(transformedPoint, featureFrame, firstPoint);
    const float* otherStart = secondFeature->adjacent[0].feature->vector;
    for (int lane = 0; lane < 3; ++lane) delta[lane] = transformedPoint[lane] - otherStart[lane];
    const float parameter = dot3(delta, otherDirection);
    for (int lane = 0; lane < 3; ++lane) {
        const float offset = otherDirection[lane] * parameter;
        secondPoint[lane] = otherStart[lane] + offset;
    }
    secondPoint[3] = 0.0f;
    return collisionPointDistance(secondPoint, transformedPoint);
}

int32_t collisionWitnessEdges(CollisionWitness* witness, const float featureFrame[16],
                              const float otherFrame[16], CollisionTransformedEdge* first,
                              CollisionTransformedEdge* second)
{
    CollisionFeature* firstFeature = witness->side == 1 ? witness->first : witness->second;
    CollisionFeature* secondFeature = witness->side == 2 ? witness->first : witness->second;
    collisionTransformEdge(first, featureFrame, firstFeature);
    int32_t result = collisionWitnessEdgeRegions(witness, first, secondFeature, 2);
    if (result != 1) return result;
    collisionTransformEdge(second, otherFrame, secondFeature);
    result = collisionWitnessEdgeRegions(witness, second, firstFeature, 1);
    if (result != 1) return result;
    float* firstPoint = witness->side == 1 ? witness->firstPoint : witness->secondPoint;
    float* secondPoint = witness->side == 2 ? witness->firstPoint : witness->secondPoint;
    witness->distance = closestEdgePoints(firstPoint, secondPoint, firstFeature, secondFeature,
                                         featureFrame, otherFrame, second);
    return result;
}

static void segmentPoint(float output[4], const CollisionTransformedEdge* segment, float parameter)
{
    for (int lane = 0; lane < 4; ++lane) {
        const float delta = segment->direction[lane] * parameter;
        output[lane] = delta + segment->second[lane];
    }
}

int32_t collisionWitnessEdgeFace(CollisionWitness* witness, const float featureFrame[16],
                                 const float otherFrame[16], CollisionTransformedEdge* edge)
{
    CollisionFeature* edgeFeature = witness->side == 1 ? witness->first : witness->second;
    CollisionFeature* face = witness->side == 2 ? witness->first : witness->second;
    collisionTransformEdge(edge, featureFrame, edgeFeature);
    CollisionClip clip;
    collisionClipInitialize(&clip);
    if (collisionClipPlanes(&clip, edge, face->adjacent, face->adjacentCount)) {
        float lower[4], upper[4];
        segmentPoint(lower, edge, clip.lower);
        segmentPoint(upper, edge, clip.upper);
        const float upperDistance = planeDistance(face, upper);
        const float lowerDistance = planeDistance(face, lower);
        const float product = lowerDistance * upperDistance;
        if (product <= 0.0f) { witness->distance = 0.0f; return 2; }
        const float derivative = dot3(face->vector, edge->direction);
        const float signedDerivative = lowerDistance * derivative;
        if (0.0f < signedDerivative) {
            if (clip.lowerFeature) replaceFeature(witness, 3 - witness->side, clip.lowerFeature);
            else replaceFeature(witness, witness->side, edgeFeature->adjacent[0].feature);
        } else {
            if (clip.upperFeature) replaceFeature(witness, 3 - witness->side, clip.upperFeature);
            else replaceFeature(witness, witness->side, edgeFeature->adjacent[1].feature);
        }
        return 0;
    }

    CollisionFeature* selected = NULL;
    CollisionFeature* changed = NULL;
    float minimum = 0.0f;
    for (int index = 0; index < face->adjacentCount; ++index) {
        CollisionFeature* candidate = face->adjacent[index].feature;
        CollisionTransformedEdge transformed;
        collisionTransformEdge(&transformed, otherFrame, candidate);
        float firstPoint[4], secondPoint[4];
        const float distance = closestEdgePoints(firstPoint, secondPoint, edgeFeature, candidate,
                                                featureFrame, otherFrame, &transformed);
        CollisionClip candidateClip;
        collisionClipInitialize(&candidateClip);
        CollisionFeature* candidateChange = collisionClipVertex(&candidateClip, edge, NULL, candidate->adjacent, 2);
        if (candidateClip.boundary) return 2;
        if (index == 0 || distance < minimum) {
            minimum = distance;
            selected = candidate;
            changed = candidateChange;
        }
    }
    replaceFeature(witness, 3 - witness->side, changed ? changed : selected);
    return 0;
}

int32_t collisionWitnessQuery(CollisionWitness* witness, const float firstFrame[16],
                              const float firstPosition[4], const float secondFrame[16],
                              const float secondPosition[4], uint32_t* randomState)
{
    float firstToSecond[16], secondToFirst[16];
    collisionRelativeTransform(firstToSecond, secondFrame, secondPosition, firstFrame, firstPosition);
    collisionRelativeTransform(secondToFirst, firstFrame, firstPosition, secondFrame, secondPosition);
    CollisionTransformedVertex firstVertex{}, secondVertex{};
    CollisionTransformedEdge firstEdge{}, secondEdge{};
    CollisionFeature* firstHistory[80];
    CollisionFeature* secondHistory[80];
    int32_t iteration = 0, restarts = 0, result = 0;
    do {
        const int firstKind = witness->first->kind, secondKind = witness->second->kind;
        if (firstKind == 0 && secondKind == 0) {
            witness->side = 1;
            result = collisionWitnessVertices(witness, firstToSecond, secondToFirst, &firstVertex, &secondVertex);
        } else if (firstKind == 0 && secondKind == 1) {
            witness->side = 1;
            result = collisionWitnessVertexEdge(witness, firstToSecond, secondToFirst, &firstVertex, &secondEdge);
        } else if (firstKind == 1 && secondKind == 0) {
            witness->side = 2;
            result = collisionWitnessVertexEdge(witness, secondToFirst, firstToSecond, &secondVertex, &firstEdge);
        } else if (firstKind == 0 && secondKind == 2) {
            witness->side = 1;
            result = collisionWitnessVertexFace(witness, firstToSecond, &firstVertex);
        } else if (firstKind == 2 && secondKind == 0) {
            witness->side = 2;
            result = collisionWitnessVertexFace(witness, secondToFirst, &secondVertex);
        } else if (firstKind == 1 && secondKind == 1) {
            witness->side = 1;
            result = collisionWitnessEdges(witness, firstToSecond, secondToFirst, &firstEdge, &secondEdge);
        } else if (firstKind == 1 && secondKind == 2) {
            witness->side = 1;
            result = collisionWitnessEdgeFace(witness, firstToSecond, secondToFirst, &firstEdge);
        } else if (firstKind == 2 && secondKind == 1) {
            witness->side = 2;
            result = collisionWitnessEdgeFace(witness, secondToFirst, firstToSecond, &secondEdge);
        }
        if (collisionWitnessRemember(witness, firstHistory, secondHistory, iteration)) {
            ++restarts;
            iteration = 0;
            witness->first = &witness->firstGeometry->vertices[vectorRandomBounded((uint32_t)witness->firstGeometry->vertexCount, randomState)];
            witness->second = &witness->secondGeometry->vertices[vectorRandomBounded((uint32_t)witness->secondGeometry->vertexCount, randomState)];
        } else ++iteration;
    } while (result == 0 && restarts < 3 && iteration < 80);
    return result;
}

int32_t collisionWitnessVertices(CollisionWitness* witness, const float featureFrame[16],
                                 const float otherFrame[16], CollisionTransformedVertex* feature,
                                 CollisionTransformedVertex* other)
{
    CollisionFeature* first=witness->side==1 ? witness->first : witness->second;
    CollisionFeature* second=witness->side==2 ? witness->first : witness->second;
    collisionTransformVertex(feature,featureFrame,first);
    for(int index=0;index<second->adjacentCount;++index) {
        const CollisionPlane* plane=&second->adjacent[index];
        if(dot3(plane->normal,feature->point)-plane->offset<0.0f) {
            if(witness->side==2) witness->first=plane->feature;
            else witness->second=plane->feature;
            return 0;
        }
    }
    collisionTransformVertex(other,otherFrame,second);
    for(int index=0;index<first->adjacentCount;++index) {
        const CollisionPlane* plane=&first->adjacent[index];
        if(dot3(plane->normal,other->point)-plane->offset<0.0f) {
            if(witness->side==1) witness->first=plane->feature;
            else witness->second=plane->feature;
            return 0;
        }
    }
    float difference[3];
    for(int lane=0;lane<3;++lane) difference[lane]=first->vector[lane]-other->point[lane];
    witness->distance=sqrtf(dot3(difference,difference));
    float* firstPoint=witness->side==1 ? witness->firstPoint : witness->secondPoint;
    float* secondPoint=witness->side==2 ? witness->firstPoint : witness->secondPoint;
    for(int lane=0;lane<3;++lane) { firstPoint[lane]=first->vector[lane];secondPoint[lane]=second->vector[lane]; }
    return 0.00001f<witness->distance ? 1 : 2;
}

void collisionTransformDirectionTo(float output[4], const float frame[16], const float value[4])
{
    float matrix[16];
    memcpy(matrix, frame, sizeof(matrix));
    matrix[12] = matrix[13] = matrix[14] = 0.0f;
    matrix[15] = 1.0f;
    vectorMatrixTransformPoint(output, matrix, value);
}

CollisionFeature* collisionClipVertex(CollisionClip* clip, const CollisionTransformedEdge* edge,
                                      const float* point, const CollisionPlane* planes, int32_t count)
{
    collisionClipPlanes(clip, edge, planes, count);
    if (clip->lowerFeature && clip->lowerFeature == clip->upperFeature) return clip->lowerFeature;
    CollisionFeature* limits[2] = {clip->lowerFeature, clip->upperFeature};
    const float parameters[2] = {clip->lower, clip->upper};
    for (int side = 0; side < 2; ++side) {
        CollisionFeature* feature = limits[side];
        if (feature) {
            float projected[4], reference[4], delta[4];
            collisionGeometryPointAlong(projected, edge, parameters[side]);
            if (point) memcpy(reference, point, sizeof(reference));
            else { for (int lane = 0; lane < 3; ++lane) reference[lane] = feature->vector[lane]; reference[3] = 0.0f; }
            collisionGeometrySubtract4(delta, projected, reference);
            if (collisionGeometryDot3(delta, delta) < 0.00001f) clip->boundary = 1;
            else {
                const float derivative = collisionGeometryDot3(edge->direction, delta);
                if ((side == 0 && 0.0f < derivative) || (side == 1 && derivative < 0.0f)) return feature;
            }
        }
    }
    return NULL;
}

CollisionFeature* collisionClipFace(CollisionClip* clip, const CollisionTransformedEdge* edge,
                                    const float* plane, const CollisionPlane* planes, int32_t count)
{
    const int32_t accepted = collisionClipPlanes(clip, edge, planes, count);
    CollisionFeature* first = clip->lowerFeature;
    CollisionFeature* second = clip->upperFeature;
    if (first && first == second) return first;
    int32_t oppositeKinds = 0;
    if (!accepted) {
        if (first->kind == 2 && second->kind == 1) return first;
        if (second->kind == 2 && first->kind == 1) return second;
        oppositeKinds = 1;
    }
    CollisionFeature* features[2] = {first, second};
    const float parameters[2] = {clip->lower, clip->upper};
    for (int side = 0; side < 2; ++side) {
        CollisionFeature* feature = features[side];
        if (feature && feature->kind == 2) {
            float projected[4];
            collisionGeometryPointAlong(projected, edge, parameters[side]);
            const float* normal = plane ? plane : feature->vector;
            const float offset = plane ? plane[3] : feature->offset;
            const float distance = collisionGeometryDot3(normal, projected) - offset;
            if (fabsf(distance) < 0.0001f) clip->boundary = 1;
            else {
                const float derivative = collisionGeometryDot3(normal, edge->direction);
                const float signedDerivative = distance * derivative;
                if ((side == 0 && 0.0f < signedDerivative) || (side == 1 && signedDerivative < 0.0f)) return feature;
                if (oppositeKinds) return side == 0 ? second : first;
            }
        }
    }
    return NULL;
}

void collisionFeaturePoints(const CollisionFeature* feature, float output[4][3], const float frame[16])
{
    const CollisionFeature* vertices[8];
    int32_t count = 0;
    if (feature->kind == 0) vertices[count++] = feature;
    else if (feature->kind == 1) {
        vertices[count++] = feature->adjacent[0].feature;
        vertices[count++] = feature->adjacent[1].feature;
    } else if (feature->kind == 2) {
        for (int index = 0; index < feature->adjacentCount; ++index) {
            const CollisionFeature* edge = feature->adjacent[index].feature;
            for (int end = 0; end < 2; ++end) {
                const CollisionFeature* vertex = edge->adjacent[end].feature;
                int found = 0;
                while (found < count && vertices[found] != vertex) ++found;
                if (found == count) vertices[count++] = vertex;
            }
        }
        count = feature->adjacentCount;
    }
    for (int index = 0; index < count; ++index) {
        const CollisionFeature* vertex = vertices[index];
        const float point[4] = {vertex->vector[0], vertex->vector[1], vertex->vector[2], 0.0f};
        float transformed[4];
        vectorMatrixTransformPoint(transformed, frame, point);
        for (int lane = 0; lane < 3; ++lane) output[index][lane] = transformed[lane] - frame[12 + lane];
    }
    if (count > 0) for (int index = count; index < 4; ++index) memcpy(output[index], output[index - 1], sizeof(output[index]));
}

#include "indep/inc/Replay.hpp"

// PAL1D0948..1D0A30. Feature codes are authored geometry indices; host
// pointers are reconstructed only against the same two actual polyhedra.
void collisionWitnessSnapshot(CollisionWitness* witness,ReplaySnapshot* snapshot)
{
    if(!snapshot)return;
    if(!witness || !witness->firstGeometry || !witness->secondGeometry) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;return;
    }
    replaySnapshotBeginSection(snapshot,witness,"VCWitness");
    uint32_t first=collisionGeometryFeatureCode(witness->firstGeometry,witness->first);
    uint32_t second=collisionGeometryFeatureCode(witness->secondGeometry,witness->second);
    replaySnapshotMoveUnsigned(snapshot,&first,4);replaySnapshotMoveUnsigned(snapshot,&second,4);
    if(snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
        const auto valid=[](const CollisionGeometry& geometry,uint32_t code) {
            if(code==UINT32_MAX)return true;
            const auto index=code>>4;
            switch(code&15u) {
            case 0:return index<static_cast<uint32_t>(geometry.vertexCount);
            case 1:return index<static_cast<uint32_t>(geometry.edgeCount);
            case 2:return index<static_cast<uint32_t>(geometry.faceCount);
            default:return false;
            }
        };
        if(!valid(*witness->firstGeometry,first) || !valid(*witness->secondGeometry,second))
            snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;
        else {
            witness->first=collisionGeometryFeature(witness->firstGeometry,first);
            witness->second=collisionGeometryFeature(witness->secondGeometry,second);
        }
    }
    replaySnapshotMoveInteger(snapshot,&witness->side,4);
    replaySnapshotMoveData(snapshot,witness->firstPoint,12,-1,0);
    replaySnapshotMoveData(snapshot,witness->secondPoint,12,-1,0);
    replaySnapshotMoveFloat(snapshot,&witness->distance);
    replaySnapshotEndSection(snapshot);
}

int32_t collisionWitnessRemember(const CollisionWitness* witness, CollisionFeature** firstHistory,
                                 CollisionFeature** secondHistory, int32_t index)
{
    firstHistory[index]=witness->first;
    secondHistory[index]=witness->second;
    --index;
    for(int remaining=10;index>=0 && remaining>0;--index,--remaining)
        if(firstHistory[index]==witness->first && secondHistory[index]==witness->second) return 1;
    return 0;
}

float collisionGeometryMaximumViolation(const CollisionGeometry* geometry)
{
    float maximum=0.0f;
    for(int face=0;face<geometry->faceCount;++face)
        for(int vertex=0;vertex<geometry->vertexCount;++vertex) {
            const float distance=collisionGeometryDot3(geometry->faces[face].vector,geometry->vertices[vertex].vector)-geometry->faces[face].offset;
            if(maximum<distance) maximum=distance;
        }
    return maximum;
}
