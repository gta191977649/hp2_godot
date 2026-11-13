#include "indep/inc/eMath.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/AcidFX.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "Chopper.hpp"
#include "indep/inc/Table.hpp"
#include "indep/inc/QuickSpline.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/Race.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Texture.hpp"
#include "indep/inc/FlailerChunks.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bWare.hpp"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <array>
#include <algorithm>
#include <memory>
#include <new>
#include <vector>

struct PreviewComet {
    Spline* spline = nullptr;
    const PreviewCometParameters* parameters = nullptr;
    const PreviewCometServices* services = nullptr;
    std::vector<ChopperEffectInstance> emitters;
    const EditorMarkerTexture* texture = nullptr;
    bool emittersEnabled = false;
};

namespace {

/* PAL167208..167744 initializes four D0-byte records. These exact values
 * come from executing that initializer and its actual memset, not the zero
 * BSS image. Keep the authored nonmonotonic final curve in records1..3. */
const CurveKey previewWidthKeys[] = {
    {0.0f,0.20000000298023224f}, {0.05000000074505806f,0.699999988079071f},
    {0.10000000149011612f,1.0f}, {0.5f,1.0f}, {1.0f,0.5f}
};
const CurveKey previewStartKeys[] = {
    {0.0f,0.05000000074505806f}, {0.800000011920929f,0.949999988079071f},
    {1.0f,0.949999988079071f}
};
const CurveKey previewEndKeys[] = {
    {0.0f,-0.03999999910593033f}, {0.5f,0.4699999988079071f},
    {0.949999988079071f,0.8999999761581421f}, {1.0f,0.949999988079071f}
};
const CurveKey drivingLineEndKeys[] = {
    {0.0f,-0.03999999910593033f}, {1.0f,0.4000000059604645f},
    {0.949999988079071f,0.8999999761581421f}, {1.0f,0.949999988079071f}
};
const PreviewCometParameters previewParameters[] = {
    {"ZONE_PREVIEW",32,-15.0f,0x954C4637u,1.5f,0.0f,0.0f,0.10000000149011612f,
     {previewWidthKeys,5},{previewStartKeys,3},{previewEndKeys,4},
     {240.0f,200.0f,20.0f,255.0f},{180.0f,40.0f,0.0f,0.0f}},
    {"ZONE_PREVIEW",64,-15.0f,0x1FB22C32u,1.5f,0.0f,-1.25f,0.10000000149011612f,
     {previewWidthKeys,5},{previewStartKeys,3},{drivingLineEndKeys,4},
     {50.0f,160.0f,0.0f,255.0f},{20.0f,40.0f,0.0f,0.0f}},
    {"ZONE_PREVIEW",64,-15.0f,0x1FB22C32u,1.5f,0.0f,-1.25f,0.10000000149011612f,
     {previewWidthKeys,5},{previewStartKeys,3},{drivingLineEndKeys,4},
     {130.0f,130.0f,0.0f,255.0f},{80.0f,80.0f,0.0f,0.0f}},
    {"ZONE_PREVIEW",64,-15.0f,0x1FB22C32u,1.5f,0.0f,-1.25f,0.10000000149011612f,
     {previewWidthKeys,5},{previewStartKeys,3},{drivingLineEndKeys,4},
     {60.0f,60.0f,180.0f,255.0f},{0.0f,0.0f,80.0f,0.0f}}
};

const EditorMarkerTexture* previewTexture(EditorMarkerResources* assets,uint32_t key)
{
    for (auto* catalogue=assets;catalogue;catalogue=catalogue->nextCatalogue) {
        for (size_t index=0;index<catalogue->textureCount;++index) {
            auto& texture=catalogue->textures[index];
            if (texture.key==key) {
                texture.used=1;
                return &texture;
            }
        }
    }
    return nullptr;
}

void previewCross(float result[4],const float first[4],const float second[4])
{
    const float x=first[1]*second[2]-first[2]*second[1];
    const float y=first[2]*second[0]-first[0]*second[2];
    const float z=first[0]*second[1]-first[1]*second[0];
    result[0]=x;result[1]=y;result[2]=z;result[3]=0.0f;
}

void previewNormalize(float value[4])
{
    // VRSQRT followed by multiplication; zero/degenerate source lanes have
    // no meaningful direction. The existing finite VU convention is used.
    const float x=value[0]*value[0],y=value[1]*value[1],z=value[2]*value[2];
    const float reciprocal=finiteRatio(1.0f,sqrtf((x+y)+z));
    for (int axis=0;axis<3;++axis)value[axis]*=reciprocal;
    value[3]=0.0f;
}

float previewMaximumParameter(const Spline& spline)
{
    if (spline.boundary==SPLINE_CLOSED)return static_cast<float>(spline.count);
    if (spline.boundary==SPLINE_CLAMP || spline.boundary==SPLINE_EXTRAPOLATE)
        return static_cast<float>(spline.count-1);
    return 0.0f;
}

}

const PreviewCometParameters* previewCometParameters(int32_t variant)
{
    if (variant==-1)return &previewParameters[0];
    return &previewParameters[1+std::min(2,std::max(0,variant))];
}

PreviewComet* previewCometInitialize(Spline* spline,
    const PreviewCometParameters* parameters,const PreviewCometServices* services)
{
    if (!spline || !parameters || !services || !services->particles ||
        !services->assets || (services->emitterGroupCount && !services->emitterGroups) ||
        parameters->segments<1 || !parameters->emitterGroup)return nullptr;
    try {
        auto comet=std::make_unique<PreviewComet>();
        comet->spline=spline;
        comet->parameters=parameters;
        comet->services=services;
        const uint32_t groupKey=hashString(parameters->emitterGroup);
        for (size_t group=0;group<services->emitterGroupCount;++group) {
            const auto& source=services->emitterGroups[group];
            if (source.key!=groupKey)continue;
            if (source.count && !source.members)return nullptr;
            const size_t first=comet->emitters.size();
            comet->emitters.resize(first+source.count);
            for (size_t index=0;index<source.count;++index) {
                if (!source.members[index])return nullptr;
                auto& emitter=comet->emitters[first+index];
                emitter.resource=source.members[index];
                emitter.particleDefinition=emitter.resource->particleDefinition;
                memcpy(emitter.frame,emitter.resource->frame,sizeof(emitter.frame));
                emitter.emissionState=1;
                emitter.occupied=1;
            }
        }
        if (previewCometEnableEmitters(comet.get())!=WORLD_SERVICES_OK)return nullptr;
        comet->texture=previewTexture(services->assets,parameters->textureKey);
        if (!comet->texture) {
            previewCometDisableEmitters(comet.get());
            return nullptr;
        }
        return comet.release();
    } catch (const std::bad_alloc&) {
        return nullptr;
    }
}

WorldServicesResult previewCometEnableEmitters(PreviewComet* comet)
{
    if (!comet || !comet->services || !comet->services->particles)
        return WORLD_SERVICES_INVALID_BINDING;
    if (comet->emittersEnabled)return WORLD_SERVICES_OK;
    for (auto& emitter:comet->emitters) {
        const auto result=worldParticlesAddEmitter(comet->services->particles,&emitter);
        if (result!=WORLD_SERVICES_OK) {
            for (auto& registered:comet->emitters) {
                if (registered.particleSystem)
                    worldParticlesRemoveEmitter(registered.particleSystem,&registered);
            }
            return result;
        }
    }
    comet->emittersEnabled=true;
    return WORLD_SERVICES_OK;
}

void previewCometDisableEmitters(PreviewComet* comet)
{
    if (!comet || !comet->emittersEnabled)return;
    for (auto& emitter:comet->emitters) {
        if (emitter.particleSystem)
            worldParticlesRemoveEmitter(emitter.particleSystem,&emitter);
    }
    comet->emittersEnabled=false;
}

void previewCometDispose(PreviewComet* comet)
{
    if (!comet)return;
    previewCometDisableEmitters(comet);
    // Live particles retain their canonical group and definition, not a
    // dangling emitter from this destroyed PreviewComet allocation.
    if (comet->services && comet->services->particles) {
        for (auto& emitter:comet->emitters) {
            for (auto* group=comet->services->particles->firstGroup;group;group=group->next)
                if (group->emitter==&emitter)group->emitter=nullptr;
        }
    }
    delete comet;
}

WorldServicesResult previewCometRender(PreviewComet* comet,float parameter,PreviewCometMesh* mesh)
{
    if (!comet || !comet->spline || !comet->parameters || !mesh)
        return WORLD_SERVICES_INVALID_BINDING;
    mesh->count=0;
    mesh->texture=comet->texture;
    const auto& parameters=*comet->parameters;
    const size_t required=static_cast<size_t>(parameters.segments/2);
    if (!mesh->quads || mesh->capacity<required)
        return WORLD_SERVICES_CAPACITY;
    if (!comet->spline->points || comet->spline->count<2)
        return WORLD_SERVICES_INVALID_BINDING;

    const float begin=keyedCurveEvaluate(&parameters.startCurve,parameter);
    const float end=keyedCurveEvaluate(&parameters.endCurve,parameter);
    const float step=finiteRatio(end-begin,static_cast<float>(parameters.segments));
    const float phase=parameter*parameters.uvRate;
    float floor=static_cast<float>(convertFloatToInt32(phase));
    if (phase<floor)floor-=1.0f;
    const float uvPhase=phase-floor;
    const float maximum=previewMaximumParameter(*comet->spline);
    PreviewCometVertex ring[4]{};
    ring[0].uv[0]=ring[3].uv[0]=0.0f;
    ring[1].uv[0]=ring[2].uv[0]=1.0f;
    int32_t positive=0,negative=1;
    // PAL prepares i+1 in161CB4/161CD4, then increments it again in161F2C
    // before the161F38 store. Preserve actual samples0,2,4,..., not an
    // inferred evenly indexed loop with twice the source draw calls.
    for (int32_t sample=0;sample<=parameters.segments;sample+=2) {
        const float index=static_cast<float>(sample);
        const float selected=begin+step*index;
        const float splineParameter=maximum*selected;
        float point[4],tangent[4],curvature[4];
        splineEvaluate(comet->spline,point,splineParameter);
        splineEvaluateTangent(comet->spline,tangent,splineParameter);
        splineEvaluateCurvature(comet->spline,curvature,splineParameter);
        point[2]+=parameters.pathHeight;
        previewNormalize(tangent);
        const float vertical[4]={0.0f,0.0f,1.0f,0.0f};
        float side[4],up[4];
        previewCross(side,vertical,tangent);
        previewNormalize(side);
        const float x=curvature[0]*side[0],y=curvature[1]*side[1],z=curvature[2]*side[2];
        const float bend=parameters.curvatureScale*((x+y)+z);
        for (int axis=0;axis<3;++axis)up[axis]=side[axis]*bend+vertical[axis];
        up[3]=0.0f;
        previewCross(side,up,tangent);
        previewNormalize(side);
        previewCross(up,tangent,side);
        previewNormalize(up);

        // Source+58 is the second start key's coordinate (0.8), not its
        // value0.95 at+5C. Emitter shutdown and alpha use the same cell.
        const float cutoff=parameters.startCurve.keys[1].coordinate;
        if (sample==0) {
            for (auto& emitter:comet->emitters) {
                if (emitter.emissionState!=1)continue;
                if (parameter<cutoff) {
                    for (int axis=0;axis<3;++axis) {
                        emitter.frame[axis]=side[axis];
                        emitter.frame[4+axis]=up[axis];
                        emitter.frame[8+axis]=tangent[axis];
                        emitter.frame[12+axis]=point[axis];
                    }
                    emitter.frame[3]=emitter.frame[7]=emitter.frame[11]=0.0f;
                    emitter.frame[15]=1.0f;
                } else {
                    emitter.emissionState=0;
                }
            }
        }
        const float fraction=finiteRatio(index,static_cast<float>(parameters.segments));
        const float inverse=1.0f-fraction;
        const float halfWidth=parameters.width*(keyedCurveEvaluate(&parameters.widthCurve,fraction)*0.5f);
        for (int axis=0;axis<3;++axis) {
            const float offset=side[axis]*halfWidth;
            ring[positive].position[axis]=point[axis]+offset;
            ring[negative].position[axis]=point[axis]-offset;
        }
        ring[positive].position[2]+=parameters.height;
        ring[negative].position[2]+=parameters.height;
        ring[positive].uv[1]=ring[negative].uv[1]=uvPhase+fraction;
        float fade=1.0f;
        if (0.0f<=parameter-cutoff)
            fade=1.0f-finiteRatio(parameter-cutoff,1.0f-cutoff);
        for (int channel=0;channel<4;++channel) {
            float value=inverse*parameters.firstColor[channel]+fraction*parameters.lastColor[channel];
            if (channel==3)value*=fade;
            const auto color=static_cast<uint8_t>(convertFloatToInt32(value));
            ring[positive].color[channel]=ring[negative].color[channel]=color;
        }
        if (sample>0) {
            auto& quad=mesh->quads[mesh->count++];
            memcpy(quad.vertices,ring,sizeof(ring));
        }
        positive^=3;
        negative^=3;
    }
    return WORLD_SERVICES_OK;
}

WorldServicesResult previewCometRenderFrame(PreviewComet* comet,float parameter,
    const CarCinematicScreen* screen,NativeEcstasyFrame* frame)
{
    if (!screen || !frame)return WORLD_SERVICES_INVALID_BINDING;
    // PAL161820..161F98 uses its real ePoly constructor and one identity
    // transform; each generated segment reaches Render3D at161F18.
    std::array<PreviewCometQuad,32> quads{};
    PreviewCometMesh mesh{quads.data(),quads.size(),0,nullptr};
    const auto result=previewCometRender(comet,parameter,&mesh);
    if (result!=WORLD_SERVICES_OK)return result;
    const auto* texture=hp2::textureInfoForDescriptor(mesh.texture);
    if (!texture)return WORLD_SERVICES_INVALID_BINDING;
    float identity[16];eIdentity(identity);
    for (size_t index=0;index<mesh.count;++index) {
        hp2::FlailerPoly polygon{};
        polygon.rasterState={{0x01000100u,0x00000080u,0,0}};
        for (size_t vertex=0;vertex<4;++vertex) {
            const auto& from=mesh.quads[index].vertices[vertex];
            auto& to=polygon.vertices[vertex];
            to.position={{from.position[0],from.position[1],from.position[2],0}};
            to.textureCoordinate={{from.uv[0],from.uv[1],1}};
            std::memcpy(&to.color,from.color,4);
        }
        frame->RenderPolygon3D(polygon,texture,identity,*screen);
    }
    return WORLD_SERVICES_OK;
}

void carCameraClusterMean(const CarCameraCluster* cluster,float output[4])
{
    float sum[3]={0.0f,0.0f,0.0f};
    int32_t count=0;
    for(const CarCameraClusterPoint* point=cluster->first;point;point=point->next){
        for(int axis=0;axis<3;++axis)sum[axis]=sum[axis]+point->position[axis];
        ++count;
    }
    const float divisor=(float)(count>1?count:1);
    const float reciprocal=finiteRatio(1.0f,divisor);
    for(int axis=0;axis<3;++axis)output[axis]=sum[axis]*reciprocal;
    /* The source stack accumulator initializes XYZ only. W is padding. */
    output[3]=0.0f;
}

float carCameraApproachScalar(float value, float target, float maximumChange)
{
    const float difference = target - value;
    if (fabsf(difference) <= maximumChange) maximumChange = fabsf(difference);
    if (target < value) value -= maximumChange;
    else if (value < target) value += maximumChange;
    return value;
}

void carCameraLimitPointHeight(const Spline* ground, Spline* camera,
                                int32_t index, float maximumHeight)
{
    if (index < 0 || index >= camera->count) return;
    const float groundHeight = ground->points[index][2];
    if (maximumHeight < camera->points[index][2] - groundHeight) {
        float point[4];
        memcpy(point, camera->points[index], sizeof(point));
        point[2] = groundHeight + maximumHeight;
        splineSetPoint(camera, index, point);
    }
}

int32_t carCameraIsHeightMinimum(const float* heights, int32_t count, int32_t index)
{
    if (index == 0) return heights[0] < heights[1];
    if (index == count - 1) return heights[index] < heights[index - 1];
    const float height = heights[index];
    const int32_t lowerPrevious = height < heights[index - 1];
    const int32_t lowerNext = height < heights[index + 1];
    return height <= heights[index - 1] && height <= heights[index + 1] &&
           (lowerPrevious || lowerNext);
}

void carCameraLimitMaximumSlope(Spline* curve, float gradient)
{
    for (int32_t index = 1; index < curve->count; ++index) {
        const float x = curve->points[index][0] - curve->points[index - 1][0];
        const float y = curve->points[index][1] - curve->points[index - 1][1];
        const float xx = x * x, yy = y * y;
        const float change = sqrtf(xx + yy) * gradient;
        const float maximum = curve->points[index - 1][2] + change;
        if (maximum < curve->points[index][2]) curve->points[index][2] = maximum;
    }
    for (int32_t index = curve->count - 2; index >= 0; --index) {
        const float x = curve->points[index][0] - curve->points[index + 1][0];
        const float y = curve->points[index][1] - curve->points[index + 1][1];
        const float xx = x * x, yy = y * y;
        const float change = sqrtf(xx + yy) * gradient;
        const float maximum = curve->points[index + 1][2] + change;
        if (maximum < curve->points[index][2]) curve->points[index][2] = maximum;
    }
}

void carRoadPreviewStepPosition(const CarRoadPreviewStep* step, float output[4])
{
    driverRoadPointPosition(step->pose, output, 0.0f, step->across);
}


// PAL160F4C..160F90 and16357C..1635C4 both retain sin(FOV)/cos(FOV).
// The authored Drive views use exactly0x4000 (90 degrees). In native IEEE
// arithmetic the source sine polynomial yields -0 for cos(90), which would
// select the opposite atan quadrant and a negative projection near plane.
// Retain the positive one-sided tangent at that single authored boundary.
// All ordinary source angles, zoom arithmetic and the common trig stay intact.
static float zoneInitialFovTangent(uint16_t fieldOfView)
{
    const float sine=rotationSineAngle(fieldOfView);
    const float cosine=vectorCosineAngle(fieldOfView);
    if(fieldOfView==0x4000 && cosine==0.0f)return FLT_MAX;
    return finiteRatio(sine,cosine);
}

static void zoneFreezeCrossThree(float output[4], const float first[4], const float second[4])
{
    output[0] = first[1] * second[2] - second[1] * first[2];
    output[1] = first[2] * second[0] - second[2] * first[0];
    output[2] = first[0] * second[1] - second[0] * first[1];
    output[3] = 0.0f;
}
static float zoneFreezeMaximumParameter(const Spline* curve)
{
    if (curve->boundary == SPLINE_CLOSED) return (float)curve->count;
    if (curve->boundary == SPLINE_CLAMP || curve->boundary == SPLINE_EXTRAPOLATE)
        return (float)(curve->count - 1);
    return 0.0f;
}
static void zoneFreezeScaleVector(float output[4], const float input[4], float factor)
{
    for (int axis = 0; axis < 4; ++axis) output[axis] = input[axis] * factor;
}
static void zoneFreezeInitializeBlend(tCubic3D* curve, const float begin[4], const float beginVelocity[4],
    const float end[4], const float endVelocity[4], float duration)
{
    for (int axis = 0; axis < 3; ++axis) {
        tCubic1DInitialize(&curve->axes[axis]);
        curve->axes[axis].duration = duration;
        curve->axes[axis].continuous = 0;
    }
    tCubic3DSetVal(curve, begin);
    tCubic3DSetdVal(curve, beginVelocity);
    tCubic3DSetValDesired(curve, end);
    tCubic3DSetdValDesired(curve, endVelocity);
}
CarZoneFreezeCamera* carZoneFreezeCameraCreate(const CarZoneFreezeCameraContext* context,
    CarCinematicResult* result)
{
    if (result) *result = CAR_CINEMATIC_INVALID_BINDING;
    if (!context || !context->services || !context->services->clock || !context->services->radialBlur ||
        !context->terrain || !context->player || !context->movement || !context->movement->bindings.state ||
        context->viewNumber < 0 || context->viewNumber > 3 ||
        !context->services->screens[context->viewNumber].view) return NULL;
    if ((context->previous.kind == CAR_CAMERA_REFERENCE_BASE && !context->previous.owner.base) ||
        (context->previous.kind == CAR_CAMERA_REFERENCE_CHASE && !context->previous.owner.chase) ||
        (context->previous.kind != CAR_CAMERA_REFERENCE_BASE && context->previous.kind != CAR_CAMERA_REFERENCE_CHASE))
        return NULL;
    const VehicleState* state = context->movement->bindings.state;
    if (!state->position) return NULL;
    CarZoneFreezeCamera* camera = (CarZoneFreezeCamera*)calloc(1, sizeof(*camera));
    if (!camera) { if (result) *result = CAR_CINEMATIC_ALLOCATION_FAILURE; return NULL; }
    if (carCinematicBaseInitialize(&camera->base, context->services, context->viewNumber, 12) != CAR_CINEMATIC_OK) {
        free(camera); return NULL;
    }
    carCinematicBindOwner(&camera->base,camera,&carCinematicZoneMethods);
    camera->base.player = context->player;
    splineInitialize(&camera->eye, SPLINE_EXTRAPOLATE, 100, 16384);
    splineInitialize(&camera->target, SPLINE_EXTRAPOLATE, 100, 16384);
    terrainSurfaceInitialize(&camera->surface, context->terrain);
    float frame[16];
    rotationMatrixIdentity(frame);
    if (state->speed > 0.009999999776482582f) vectorNormalize2(frame, state->velocity);
    else {
        float direction[2];
        if (fabsf(1.0f - state->orientation[2]) <= 0.0010000000474974513f) {
            direction[0] = -state->orientation[8];
            direction[1] = -state->orientation[9];
        } else {
            direction[0] = state->orientation[0];
            direction[1] = state->orientation[1];
        }
        vectorNormalize2(frame, direction);
    }
    zoneFreezeCrossThree(frame + 4, frame + 8, frame);
    const float worldElapsed = context->services->clock->worldTimeElapsed;
    CarCinematicView* view = camera->base.view;
    float predictedEye[4], predictedTarget[4];
    for (int axis = 0; axis < 4; ++axis) {
        frame[12 + axis] = state->velocity[axis] * worldElapsed + state->position[axis];
        predictedEye[axis] = view->eyeVelocity[axis] * worldElapsed + view->current.eye[axis];
        predictedTarget[axis] = view->targetVelocity[axis] * worldElapsed + view->current.target[axis];
    }
    float inverse[16], localEye[4], relativeVelocity[4], localVelocity[4], side[4];
    carCameraInvertRigidTransform(inverse, frame);
    vectorMatrixTransformPoint(localEye, inverse, predictedEye);
    float direction = -1.0f;
    if (context->previous.kind == CAR_CAMERA_REFERENCE_BASE ||
        carChaseCameraUsesForwardSide(context->previous.owner.chase)) direction = 1.0f;
    for (int axis = 0; axis < 4; ++axis) {
        localEye[axis] *= direction;
        relativeVelocity[axis] = view->eyeVelocity[axis] - state->velocity[axis];
    }
    relativeVelocity[3] = 0.0f;
    vectorMatrixTransform(localVelocity, inverse, relativeVelocity);
    zoneFreezeCrossThree(side, frame + 8, localEye);
    const float x = localVelocity[0] * side[0];
    const float y = localVelocity[1] * side[1];
    const float z = localVelocity[2] * side[2];
    const float turn = (0.0f <= (x + y) + z ? 1.0f : -1.0f) * direction;
    const uint16_t initialAngle = vectorAtan2Angle(localEye[0], localEye[1]);
    const float unitAngle = (float)initialAngle * 0.0000152587890625f;
    const float startAngle = unitAngle + turn * 0.15000000596046448f;
    for (int index = 0; index < 32; ++index) {
        float angle = (float)index * 0.032258063554763794f;
        angle *= turn;
        angle *= 0.6700000166893005f;
        angle += startAngle;
        const uint16_t encoded = (uint16_t)convertFloatToInt32(angle * 65536.0f);
        const float sine = rotationSineAngle(encoded);
        const float cosine = vectorCosineAngle(encoded);
        const float localTarget[4] = {cosine * 0.10000000149011612f,
            sine * 0.10000000149011612f, 1.2000000476837158f, 1.0f};
        const float localPoint[4] = {cosine * 8.0f, sine * 8.0f, 2.0f, 1.0f};
        float target[4], eye[4];
        vectorMatrixTransformPoint(target, frame, localTarget);
        splineAppendPoint(&camera->target, target);
        vectorMatrixTransformPoint(eye, frame, localPoint);
        terrainSurfaceUpdate(&camera->surface, target, 8.699999809265137f, NULL);
        carCameraConstrainTerrain(&camera->surface, target, eye, 1, 1);
        carCameraLiftNearGround(&camera->surface, eye, 1.0f);
        splineAppendPoint(&camera->eye, eye);
    }
    float endpoint[4], tangent[4], velocity[4];
    splineEvaluate(&camera->eye, endpoint, 0.0f);
    splineEvaluateTangent(&camera->eye, tangent, 0.0f);
    zoneFreezeScaleVector(tangent, tangent, (zoneFreezeMaximumParameter(&camera->eye) * 0.5f) * 0.4000000059604645f);
    for (int axis = 0; axis < 4; ++axis) velocity[axis] = (view->eyeVelocity[axis] - state->velocity[axis]) * 0.4000000059604645f;
    zoneFreezeInitializeBlend(&camera->entryEye, predictedEye, velocity, endpoint, tangent, 0.4000000059604645f);
    splineEvaluate(&camera->target, endpoint, 0.0f);
    splineEvaluateTangent(&camera->target, tangent, 0.0f);
    zoneFreezeScaleVector(tangent, tangent, (zoneFreezeMaximumParameter(&camera->target) * 0.5f) * 0.4000000059604645f);
    for (int axis = 0; axis < 4; ++axis) velocity[axis] = (view->targetVelocity[axis] - state->velocity[axis]) * 0.4000000059604645f;
    zoneFreezeInitializeBlend(&camera->entryTarget, predictedTarget, velocity, endpoint, tangent, 0.4000000059604645f);
    splineEvaluate(&camera->eye, endpoint, zoneFreezeMaximumParameter(&camera->eye));
    splineEvaluateTangent(&camera->eye, tangent, zoneFreezeMaximumParameter(&camera->eye));
    zoneFreezeScaleVector(tangent, tangent, (zoneFreezeMaximumParameter(&camera->eye) * 0.5f) * 0.4000000059604645f);
    zoneFreezeScaleVector(velocity, view->eyeVelocity, 0.6000000238418579f);
    zoneFreezeInitializeBlend(&camera->exitEye, endpoint, tangent, predictedEye, velocity, 0.6000000238418579f);
    splineEvaluate(&camera->target, endpoint, zoneFreezeMaximumParameter(&camera->target));
    splineEvaluateTangent(&camera->target, tangent, zoneFreezeMaximumParameter(&camera->target));
    zoneFreezeScaleVector(tangent, tangent, (zoneFreezeMaximumParameter(&camera->target) * 0.5f) * 0.4000000059604645f);
    zoneFreezeScaleVector(velocity, view->targetVelocity, 0.6000000238418579f);
    zoneFreezeInitializeBlend(&camera->exitTarget, endpoint, tangent, predictedTarget, velocity, 0.6000000238418579f);
    tCubic1DInitialize(&camera->zoom);
    camera->zoom.value = camera->zoom.desiredValue = 1.0f;
    camera->zoom.duration = 0.4000000059604645f;
    camera->zoom.continuous = 0;
    camera->zoom.mode = 2;
    camera->initialFovTangent = zoneInitialFovTangent(view->current.fieldOfView);
    if (!context->services->suppressPublication) {
        view->hiddenCar = NULL;
        view->visibilityFlags = 0;
    }
    if (result) *result = CAR_CINEMATIC_OK;
    return camera;
}
void carZoneFreezeCameraDispose(CarZoneFreezeCamera* camera)
{
    if (!camera) return;
    screenRadialBlurStop(camera->base.services->radialBlur, camera->base.player->movement.seat);
    terrainSurfaceDispose(&camera->surface);
    splineDestroy(&camera->target, 0);
    splineDestroy(&camera->eye, 0);
    carCinematicBaseDispose(&camera->base);
    free(camera);
}
const CarCinematicView* carZoneFreezeCameraView(const CarZoneFreezeCamera* camera) { return camera->base.view; }
static void zoneFreezeUpdateBlend(tCubic3D* eyeCurve, tCubic3D* targetCurve, float elapsed, float eye[4], float target[4])
{
    tCubic3DUpdate(eyeCurve, elapsed);
    tCubic3DUpdate(targetCurve, elapsed);
    tCubic3DGetVal(eyeCurve, eye);
    tCubic3DGetVal(targetCurve, target);
    eye[3] = target[3] = 0.0f;
}
CarCinematicResult carZoneFreezeCameraAdvance(CarZoneFreezeCamera* camera)
{
    if (!camera || !camera->base.services || !camera->base.services->clock || !camera->base.player)
        return CAR_CINEMATIC_INVALID_BINDING;
    CarCinematicServices* services = camera->base.services;
    if (services->session && services->session->state == 6) return CAR_CINEMATIC_OK;
    const float previousTime = camera->elapsed;
    const float elapsed = services->clock->realTimeElapsed;
    camera->elapsed += elapsed;
    float eye[4], target[4], volume = 0.0f, intensity = 0.0f;
    const int32_t seat = camera->base.player->movement.seat;
    if (camera->elapsed < 0.4000000059604645f) {
        zoneFreezeUpdateBlend(&camera->entryEye, &camera->entryTarget, elapsed, eye, target);
        carCameraConstrainTerrain(&camera->surface, target, eye, 0, 0);
        volume = camera->elapsed * 2.5f;
        screenRadialBlurSetAmounts(services->radialBlur, seat, 0.0f, volume * 0.8399999737739563f);
    } else if (camera->elapsed > 2.4000000953674316f) {
        if (previousTime <= 2.4000000953674316f) {
            camera->zoom.desiredValue = 1.0f;
            camera->zoom.duration = carZoneFreezeCameraDuration(camera) - camera->elapsed;
            if (camera->zoom.value != 1.0f) camera->zoom.mode = 2;
        }
        zoneFreezeUpdateBlend(&camera->exitEye, &camera->exitTarget, elapsed, eye, target);
        carCameraConstrainTerrain(&camera->surface, target, eye, 0, 0);
        volume = (carZoneFreezeCameraDuration(camera) - camera->elapsed) * 1.6666666269302368f;
        screenRadialBlurSetAmounts(services->radialBlur, seat, 0.0f, volume * 0.8399999737739563f);
        intensity = 1.0f - volume;
    } else {
        const float fraction = (camera->elapsed - 0.4000000059604645f) * 0.5f;
        splineEvaluate(&camera->eye, eye, zoneFreezeMaximumParameter(&camera->eye) * fraction);
        splineEvaluate(&camera->target, target, zoneFreezeMaximumParameter(&camera->target) * fraction);
        volume = 1.0f;
    }
    tCubic1DUpdate(&camera->zoom, elapsed, 0.0f, 0.0f);
    for (int axis = 0; axis < 4; ++axis) eye[axis] = target[axis] + (eye[axis] - target[axis]) * camera->zoom.value;
    const uint16_t fov = vectorAtan2Angle(camera->zoom.value, camera->initialFovTangent);
    if (!services->suppressPublication) camera->base.view->current.fieldOfView = fov;
    const float up[4] = {0, 0, 1, 0};
    float matrix[16];
    carCinematicLookAt(matrix, eye, target, up);
    carCinematicViewPublish(services, camera->base.view, matrix, elapsed);
    const float normalized = camera->elapsed / carZoneFreezeCameraDuration(camera);
    const uint16_t code = (uint16_t)convertFloatToInt32(normalized * 65536.0f);
    const float angle = (normalized * 360.0f) * 0.01745329424738884f + 1.5707964897155762f;
    const float pitch = rotationSineRadians(angle) * 0.5f + 0.5f;
    carFramePlayerUpdateCinematicAudio(camera->base.player, code, intensity + intensity, volume, pitch);
    return camera->base.player->result ? CAR_CINEMATIC_BACKEND_CAPACITY : CAR_CINEMATIC_OK;
}


float carZoneFreezeCameraDuration(const CarZoneFreezeCamera* camera)
{
    (void)camera;
    return 3.0f;
}

// ZonePreviewCameraMover: original PAL methods; no parallel crash-camera owner.
static float zonePreviewMaximumParameter(const Spline* curve)
{
    if (curve->boundary == SPLINE_CLOSED) return (float)curve->count;
    if (curve->boundary == SPLINE_CLAMP || curve->boundary == SPLINE_EXTRAPOLATE)
        return (float)(curve->count - 1);
    return 0.0f;
}
static void zonePreviewCurvePoint(const Spline* curve, const CarCameraParameterMap* mapping,
    float normalized, float point[4])
{
    const float parameter = zonePreviewMaximumParameter(curve) * carCameraParameterMapEvaluate(mapping, normalized);
    splineEvaluate(curve, point, parameter);
}
static void zonePreviewCurveTangent(const Spline* curve, const CarCameraParameterMap* mapping,
    float normalized, float travelTime, float duration, int32_t reverse, float tangent[4])
{
    const float maximum = zonePreviewMaximumParameter(curve);
    const float parameter = maximum * carCameraParameterMapEvaluate(mapping, normalized);
    splineEvaluateTangent(curve, tangent, parameter);
    const float last = carCameraParameterMapEvaluate(mapping, 1.0f);
    const float first = carCameraParameterMapEvaluate(mapping, 0.0f);
    const float signedMaximum = reverse ? -maximum : maximum;
    const float scaledRange = signedMaximum * (last - first);
    const float speed = finiteRatio(scaledRange, travelTime) * duration;
    for (int axis = 0; axis < 4; ++axis) tangent[axis] *= speed;
}
static void zonePreviewInitializeBlend(tCubic3D* curve, const float begin[4], const float beginVelocity[4],
    const float end[4], const float endVelocity[4], float duration)
{
    for (int axis = 0; axis < 3; ++axis) {
        tCubic1DInitialize(&curve->axes[axis]);
        curve->axes[axis].duration = duration;
        curve->axes[axis].continuous = 0;
    }
    tCubic3DSetVal(curve, begin);
    tCubic3DSetdVal(curve, beginVelocity);
    tCubic3DSetValDesired(curve, end);
    tCubic3DSetdValDesired(curve, endVelocity);
}
static void zonePreviewScaledVector(float output[4], const float input[4], float scale)
{
    for (int axis = 0; axis < 4; ++axis) output[axis] = input[axis] * scale;
}
int32_t carCrashCameraPrepare(CarRoadPreview* preview, const CarMovement* movement)
{
    if (!preview || !movement || !movement->bindings.state || !movement->bindings.recoveryPoints)
        return 0;
    const CarRecoveryPointRegistry* registry = movement->bindings.recoveryPoints;
    const CarRecoveryPose* pose = movement->bindings.driver ? movement->bindings.driver->recoveryPose : NULL;
    const CarRecoveryGroup* group = NULL;
    if (pose)
        for (size_t index = 0; index < registry->groupCount; ++index)
            if (registry->groups[index].id == pose->groupId) { group = &registry->groups[index]; break; }
    if (!carRoadPreviewInitialize(preview, registry, group, pose,
        movement->bindings.state->position, movement->routeLocation.direction,
        40.0f, 320.0f, movement->info.routeVehicleClass, 0)) return 0;
    if (preview->steps && preview->count >= 4) return 1;
    carRoadPreviewDispose(preview);
    return 0;
}
CarCrashCamera* carCrashCameraCreate(const CarCrashCameraContext* context,
    CarRoadPreview* preview, CarCinematicResult* result)
{
    if (result) *result = CAR_CINEMATIC_INVALID_BINDING;
    if (!context || !context->services || !context->services->clock || !context->simulation ||
        !context->terrain || !context->player || !context->player->registry ||
        !context->player->registry->services || !context->player->registry->services->previewComet ||
        !context->services->radialBlur || !preview || !preview->steps || preview->count < 4 ||
        !(preview->length > 0.0f) || context->viewNumber < 0 || context->viewNumber > 3 ||
        !context->services->screens[context->viewNumber].view) return NULL;
    CarCrashCamera* camera = (CarCrashCamera*)calloc(1, sizeof(*camera));
    if (!camera) { if (result) *result = CAR_CINEMATIC_ALLOCATION_FAILURE; return NULL; }
    if (carCinematicBaseInitialize(&camera->base, context->services, context->viewNumber, 12) != CAR_CINEMATIC_OK) {
        free(camera); return NULL;
    }
    carCinematicBindOwner(&camera->base,camera,&carCinematicCrashMethods);
    camera->base.player = context->player;
    Spline* curves[4] = {&camera->ground, &camera->avoidance, &camera->eye, &camera->target};
    for (int index = 0; index < 4; ++index)
        if (!splineInitialize(curves[index], SPLINE_EXTRAPOLATE, 100, 16384)) {
            carCrashCameraDispose(camera);
            if (result) *result = CAR_CINEMATIC_ALLOCATION_FAILURE;
            return NULL;
        }
    // PAL162094..1620B0 constructs the actual ZonePreviewComet on ground,
    // before the route is populated. Its authored definitions live in GLOBALB.
    camera->comet=previewCometInitialize(&camera->ground,previewCometParameters(-1),
        context->player->registry->services->previewComet);
    if (!camera->comet) {
        carCrashCameraDispose(camera);
        if (result) *result=CAR_CINEMATIC_ALLOCATION_FAILURE;
        return nullptr;
    }
    terrainSurfaceInitialize(&camera->surface, context->terrain);
    camera->forwardDuration = preview->length * 0.009999999776482582f;
    camera->reverseDuration = preview->length * 0.0033333334140479565f;
    const float entryFraction = 0.25f / (camera->forwardDuration + 0.25f);
    const float endpointFraction = 20.0f / preview->length;
    carCameraParameterMapInitialize(&camera->selectionMap, 0.0f, entryFraction, 1.0f, 1.0f - endpointFraction);
    carCameraParameterMapInitialize(&camera->fullMap, 0.0f, entryFraction + endpointFraction, 1.0f, 1.0f);
    if (!carCameraBuildTerrainCurves(preview, &camera->surface, &camera->ground,
            &camera->eye, &camera->target, &camera->selectionMap) ||
        carCameraBuildAvoidancePath(context->simulation, &camera->surface, &camera->eye,
            &camera->ground, &camera->avoidance, &camera->selectionMap, &camera->fullMap,
            camera->markers, 4) < 0) {
        carCrashCameraDispose(camera);
        if (result) *result = CAR_CINEMATIC_ALLOCATION_FAILURE;
        return NULL;
    }
    CarCinematicView* view = camera->base.view;
    memcpy(camera->previousEye, view->current.eye, sizeof(camera->previousEye));
    memcpy(camera->previousTarget, view->current.target, sizeof(camera->previousTarget));
    float predictedEye[4], predictedTarget[4];
    for (int axis = 0; axis < 4; ++axis) {
        const float eyeStep = view->eyeVelocity[axis] * context->services->clock->worldTimeElapsed;
        const float targetStep = view->targetVelocity[axis] * context->services->clock->worldTimeElapsed;
        predictedEye[axis] = eyeStep + view->current.eye[axis];
        predictedTarget[axis] = targetStep + view->current.target[axis];
    }
    float first[4], firstVelocity[4], last[4], lastVelocity[4];
    zonePreviewCurvePoint(&camera->eye, &camera->selectionMap, 0.0f, last);
    zonePreviewCurveTangent(&camera->eye, &camera->selectionMap, 0.0f, camera->forwardDuration, 0.25f, 0, lastVelocity);
    zonePreviewScaledVector(firstVelocity, view->eyeVelocity, 0.25f);
    zonePreviewInitializeBlend(&camera->entryEye, predictedEye, firstVelocity, last, lastVelocity, 0.25f);
    zonePreviewCurvePoint(&camera->avoidance, &camera->fullMap, 0.0f, last);
    zonePreviewCurveTangent(&camera->avoidance, &camera->fullMap, 0.0f, camera->forwardDuration, 0.25f, 0, lastVelocity);
    zonePreviewScaledVector(firstVelocity, view->targetVelocity, 0.25f);
    zonePreviewInitializeBlend(&camera->entryTarget, predictedTarget, firstVelocity, last, lastVelocity, 0.25f);
    zonePreviewCurvePoint(&camera->eye, &camera->selectionMap, 1.0f, first);
    zonePreviewCurveTangent(&camera->eye, &camera->selectionMap, 1.0f, camera->forwardDuration, 0.4000000059604645f, 0, firstVelocity);
    zonePreviewCurvePoint(&camera->target, &camera->selectionMap, 1.0f, last);
    zonePreviewCurveTangent(&camera->target, &camera->selectionMap, 1.0f, camera->reverseDuration, 0.4000000059604645f, 1, lastVelocity);
    zonePreviewInitializeBlend(&camera->bridgeEye, first, firstVelocity, last, lastVelocity, 0.4000000059604645f);
    zonePreviewCurvePoint(&camera->avoidance, &camera->fullMap, 1.0f, first);
    zonePreviewCurveTangent(&camera->avoidance, &camera->fullMap, 1.0f, camera->forwardDuration, 0.4000000059604645f, 0, firstVelocity);
    zonePreviewCurvePoint(&camera->ground, &camera->fullMap, 1.0f, last);
    zonePreviewCurveTangent(&camera->ground, &camera->fullMap, 1.0f, camera->reverseDuration, 0.4000000059604645f, 1, lastVelocity);
    zonePreviewInitializeBlend(&camera->bridgeTarget, first, firstVelocity, last, lastVelocity, 0.4000000059604645f);
    zonePreviewCurvePoint(&camera->target, &camera->selectionMap, 0.0f, first);
    zonePreviewCurveTangent(&camera->target, &camera->selectionMap, 0.0f, camera->reverseDuration, 0.6000000238418579f, 1, firstVelocity);
    zonePreviewScaledVector(lastVelocity, view->eyeVelocity, 0.6000000238418579f);
    zonePreviewInitializeBlend(&camera->exitEye, first, firstVelocity, predictedEye, lastVelocity, 0.6000000238418579f);
    zonePreviewCurvePoint(&camera->ground, &camera->fullMap, 0.0f, first);
    zonePreviewCurveTangent(&camera->ground, &camera->fullMap, 0.0f, camera->reverseDuration, 0.6000000238418579f, 1, firstVelocity);
    zonePreviewScaledVector(lastVelocity, view->targetVelocity, 0.6000000238418579f);
    zonePreviewInitializeBlend(&camera->exitTarget, first, firstVelocity, predictedTarget, lastVelocity, 0.6000000238418579f);
    tCubic1DInitialize(&camera->zoom);
    camera->zoom.value = camera->zoom.desiredValue = 1.0f;
    camera->zoom.duration = 0.25f;
    camera->zoom.continuous = 0;
    camera->zoom.mode = 2;
    camera->initialFovTangent = zoneInitialFovTangent(view->current.fieldOfView);
    if (!context->services->suppressPublication) {
        view->hiddenCar = NULL;
        view->visibilityFlags = 0;
    }
    if (result) *result = CAR_CINEMATIC_OK;
    return camera;
}
void carCrashCameraDispose(CarCrashCamera* camera)
{
    if (!camera) return;
    if (camera->base.player)
        screenRadialBlurStop(camera->base.services->radialBlur, camera->base.player->movement.seat);
    terrainSurfaceDispose(&camera->surface);
    previewCometDispose(camera->comet);
    camera->comet=nullptr;
    splineDestroy(&camera->target, 0);
    splineDestroy(&camera->eye, 0);
    splineDestroy(&camera->avoidance, 0);
    splineDestroy(&camera->ground, 0);
    carCinematicBaseDispose(&camera->base);
    free(camera);
}
float carCrashCameraDuration(const CarCrashCamera* camera)
{
    const float maximum = zonePreviewMaximumParameter(&camera->ground);
    const float first = maximum * carCameraParameterMapEvaluate(&camera->fullMap, 0.0f);
    const float last = maximum * carCameraParameterMapEvaluate(&camera->fullMap, 1.0f);
    float pauses = 0.0f;
    for (int index = 0; index < 4; ++index)
        if (first < camera->markers[index] && camera->markers[index] < last)
            pauses += 0.800000011920929f;
    return ((((pauses + 0.25f) + camera->forwardDuration) + 0.4000000059604645f) +
        camera->reverseDuration) + 0.6000000238418579f;
}
void carCrashCameraActivate(CarCrashCamera* camera)
{
    carCinematicBaseActivate(&camera->base);
    hp2::Require(previewCometEnableEmitters(camera->comet)==WORLD_SERVICES_OK,
        "ZonePreview camera cannot activate its actual comet emitters");
}
void carCrashCameraDeactivate(CarCrashCamera* camera)
{
    previewCometDisableEmitters(camera->comet);
    carCinematicBaseDeactivate(&camera->base);
}
const CarCinematicView* carCrashCameraView(const CarCrashCamera* camera) { return camera->base.view; }

CarCinematicResult carCrashCameraRender(CarCrashCamera* camera,
    const CarCinematicScreen* screen,NativeEcstasyFrame* frame)
{
    if (!camera || !camera->comet || !screen || !frame)
        return CAR_CINEMATIC_INVALID_BINDING;
    // PAL163790..1637C8: draw only through the forward phase, including
    // equality. The denominator is the released route-dependent duration,
    // not the beta's fixed scale and not scaled/paused World elapsed time.
    const float end=camera->forwardDuration+0.25f;
    if (end<camera->elapsed)return CAR_CINEMATIC_OK;
    const auto result=previewCometRenderFrame(camera->comet,camera->elapsed/end,screen,frame);
    return result==WORLD_SERVICES_OK ? CAR_CINEMATIC_OK : CAR_CINEMATIC_BACKEND_CAPACITY;
}
static void zonePreviewUpdateBlend(tCubic3D* eyeCurve, tCubic3D* targetCurve, float elapsed, float eye[4], float target[4])
{
    tCubic3DUpdate(eyeCurve, elapsed);
    tCubic3DUpdate(targetCurve, elapsed);
    tCubic3DGetVal(eyeCurve, eye);
    tCubic3DGetVal(targetCurve, target);
    eye[3] = target[3] = 0.0f;
}
CarCinematicResult carCrashCameraAdvance(CarCrashCamera* camera)
{
    if (!camera || !camera->base.services || !camera->base.services->clock || !camera->base.player ||
        !camera->base.services->radialBlur)
        return CAR_CINEMATIC_INVALID_BINDING;
    CarCinematicServices* services = camera->base.services;
    if (services->session && services->session->state == 6) return CAR_CINEMATIC_OK;
    const float elapsed = services->clock->realTimeElapsed;
    const float previousTime = camera->elapsed;
    if (camera->pauseCountdown > 0.0f) camera->pauseCountdown -= elapsed;
    else camera->elapsed += elapsed;
    const int32_t seat = camera->base.player->movement.seat;
    const float forwardEnd = camera->forwardDuration + 0.25f;
    const float bridgeEnd = forwardEnd + 0.4000000059604645f;
    float eye[4], target[4], phaseBlend = 0.6000000238418579f;
    if (camera->elapsed < 0.25f) {
        zonePreviewUpdateBlend(&camera->entryEye, &camera->entryTarget, elapsed, eye, target);
        carCinematicConstrainTransition(&camera->surface, camera->previousEye, eye, camera->previousTarget, target);
        screenRadialBlurSetAmounts(services->radialBlur, seat, 0.0f,
            (camera->elapsed * 4.0f) * 0.800000011920929f);
    } else if (camera->elapsed < forwardEnd) {
        const float fraction = (camera->elapsed - 0.25f) / camera->forwardDuration;
        const float previousFraction = (previousTime - 0.25f) / camera->forwardDuration;
        phaseBlend = fraction * 0.4000000059604645f + phaseBlend;
        const float groundMaximum = zonePreviewMaximumParameter(&camera->ground);
        const float currentGround = groundMaximum * carCameraParameterMapEvaluate(&camera->fullMap, fraction);
        const float previousGround = groundMaximum * carCameraParameterMapEvaluate(&camera->fullMap, previousFraction);
        zonePreviewCurvePoint(&camera->eye, &camera->selectionMap, fraction, eye);
        splineEvaluate(&camera->avoidance, target, currentGround);
        int32_t pause = 0;
        for (int index = 0; index < 4; ++index)
            if (previousGround < camera->markers[index] && camera->markers[index] <= currentGround) {
                camera->pauseCountdown = 0.800000011920929f;
                phaseBlend = 0.0f;
                pause = 1;
                break;
            }
        if (pause) screenRadialBlurStop(services->radialBlur, seat);
        else {
            screenRadialBlurStart(services->radialBlur, seat);
            const float first = fraction * 0.699999988079071f;
            const float last = (1.0f - fraction) * 0.800000011920929f;
            screenRadialBlurSetAmounts(services->radialBlur, seat, first,
                fraction * 0.8999999761581421f + last);
        }
    } else if (camera->elapsed < bridgeEnd) {
        if (previousTime < forwardEnd) {
            camera->zoom.duration = 0.4000000059604645f;
            camera->zoom.desiredValue = 1.0f;
            if (camera->zoom.value != 1.0f) camera->zoom.mode = 2;
        }
        zonePreviewUpdateBlend(&camera->bridgeEye, &camera->bridgeTarget, elapsed, eye, target);
        phaseBlend = 1.0f - ((camera->elapsed - 0.25f) - camera->forwardDuration) * 2.5f;
        const float remaining = ((camera->forwardDuration + 0.25f) + 0.4000000059604645f) - camera->elapsed;
        const float fraction = remaining * 2.5f;
        screenRadialBlurSetAmounts(services->radialBlur, seat,
            fraction * 0.699999988079071f, fraction * 0.8999999761581421f);
    } else if (camera->elapsed < bridgeEnd + camera->reverseDuration) {
        if (previousTime < bridgeEnd) screenRadialBlurStop(services->radialBlur, seat);
        const float fraction = 1.0f - (((camera->elapsed - 0.25f) - camera->forwardDuration) -
            0.4000000059604645f) / camera->reverseDuration;
        zonePreviewCurvePoint(&camera->target, &camera->selectionMap, fraction, eye);
        zonePreviewCurvePoint(&camera->ground, &camera->fullMap, fraction, target);
        phaseBlend = 0.0f;
    } else {
        zonePreviewUpdateBlend(&camera->exitEye, &camera->exitTarget, elapsed, eye, target);
        carCinematicConstrainTransition(&camera->surface, camera->previousEye, eye, camera->previousTarget, target);
        phaseBlend = 0.0f;
    }
    tCubic1DUpdate(&camera->zoom, elapsed, 0.0f, 0.0f);
    for (int axis = 0; axis < 4; ++axis) {
        const float difference = eye[axis] - target[axis];
        eye[axis] = target[axis] + difference * camera->zoom.value;
    }
    const uint16_t fieldOfView = vectorAtan2Angle(camera->zoom.value, camera->initialFovTangent);
    if (!services->suppressPublication) camera->base.view->current.fieldOfView = fieldOfView;
    const float up[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    float matrix[16];
    carCinematicLookAt(matrix, eye, target, up);
    carCinematicViewPublish(services, camera->base.view, matrix, elapsed);
    carFramePlayerUpdateCinematicAudio(camera->base.player, 0, 1.0f - phaseBlend,
        phaseBlend, phaseBlend * 0.5f - 0.5f);
    return camera->base.player->result ? CAR_CINEMATIC_BACKEND_CAPACITY : CAR_CINEMATIC_OK;
}

/* EnsureTopology, PAL15FEA0: original ZoneCameras owner. */
void carCinematicConstrainTransition(TerrainSurface* surface,float previousEye[4],float eye[4],
                                      float previousTarget[4],float target[4])
{
    float* previous[2]={previousTarget,previousEye};
    float* current[2]={target,eye};
    for(int endpoint=0;endpoint<2;++endpoint){
        float half[4],middle[4];
        for(int axis=0;axis<4;++axis)half[axis]=(previous[endpoint][axis]-current[endpoint][axis])*0.5f;
        float radius=carCinematicLengthThree(half);
        if(15.0f<radius){
            radius=15.0f;
            for(int axis=0;axis<4;++axis)half[axis]*=15.0f;
        }
        for(int axis=0;axis<4;++axis)middle[axis]=half[axis]+current[endpoint][axis];
        terrainSurfaceUpdate(surface,middle,radius,NULL);
        carCameraConstrainTerrain(surface,previous[endpoint],current[endpoint],0,0);
    }
}


/* Camera path support. */
void carCameraFlattenEndHeight(const Spline* ground, Spline* camera, int32_t index)
{
    const int32_t first = index < camera->count / 2 ? 0 : index - 2;
    const int32_t last = index < camera->count / 2 ? index + 2 : camera->count - 1;
    float minimumHeight = 1000.0f;
    for (int32_t sample = first; sample <= last; ++sample) {
        float groundPoint[4], cameraPoint[4];
        splineEvaluate(ground, groundPoint, (float)sample);
        splineEvaluate(camera, cameraPoint, (float)sample);
        const float height = cameraPoint[2] - groundPoint[2];
        if (height < minimumHeight) minimumHeight = height;
    }
    for (int32_t sample = first; sample <= last; ++sample)
        carCameraLimitPointHeight(ground, camera, sample, minimumHeight);
}

int32_t carCameraResampleHorizontal(Spline* curve, float multiplier,
                                    const float* specialParameters, int32_t specialCount)
{
    const float scaledCount = multiplier * (float)curve->count;
    float roundedCount = (float)convertFloatToInt32(scaledCount);
    if (roundedCount < scaledCount) roundedCount += 1.0f;
    const int32_t sampleCount = convertFloatToInt32(roundedCount);
    float (*samples)[4] = static_cast<float(*)[4]>(malloc((size_t)sampleCount * sizeof(*samples)));
    if (!samples) return 0;
    const float lastSample = (float)(sampleCount - 1);
    const float lastParameter = zonePreviewMaximumParameter(curve);
    CurveKey keys[6];
    keys[0].coordinate = 0.0f;
    keys[0].value = 0.0f;
    for (int32_t index = 0; index < specialCount; ++index) {
        const float parameter = specialParameters[index];
        const float scaled = lastSample * parameter;
        const float coordinate = finiteRatio(scaled, lastParameter);
        const float truncated = (float)convertFloatToInt32(coordinate);
        float floor = truncated;
        if (coordinate < floor) floor -= 1.0f;
        float rounded = truncated;
        if (coordinate - floor < 0.5f) {
            if (coordinate < rounded) rounded -= 1.0f;
        } else if (rounded < coordinate) {
            rounded += 1.0f;
        }
        keys[index + 1].coordinate = rounded;
        keys[index + 1].value = parameter;
    }
    keys[specialCount + 1].coordinate = lastSample;
    keys[specialCount + 1].value = lastParameter;
    const KeyedCurve mapping = {keys, specialCount + 2};
    for (int32_t index = 0; index < sampleCount; ++index) {
        const float parameter = keyedCurveEvaluate(&mapping, (float)index);
        splineEvaluate(curve, samples[index], parameter);
    }
    Spline temporary;
    splineInitializeBorrowed(&temporary, SPLINE_EXTRAPOLATE, sampleCount, samples, 0);
    const int32_t originalCount = curve->count;
    for (int32_t index = 0; index < originalCount; ++index) {
        const float scaled = (float)index * zonePreviewMaximumParameter(curve);
        const float parameter = finiteRatio(scaled, (float)(originalCount - 1));
        const float sample = keyedCurveInverse(&mapping, parameter);
        float point[4], original[4];
        splineEvaluate(&temporary, point, sample);
        splineEvaluate(curve, original, parameter);
        point[2] = original[2];
        splineSetPoint(curve, index, point);
    }
    splineDestroy(&temporary, 2);
    free(samples);
    return 1;
}

int32_t carCameraConstrainTerrain(TerrainSurface* surface, float first[4], float second[4],
                                   int32_t bothDirections, int32_t twoSided)
{
    int32_t result = terrainSurfaceConstrainSegment(surface, 0, twoSided == 1,
        first, second, 0.699999988079071f, 1.0f);
    result |= static_cast<int32_t>(terrainSurfaceConstrainSegment(surface, 1, twoSided == 1,
        first, second, 0.699999988079071f, 1.0f));
    if (bothDirections == 1) {
        result |= static_cast<int32_t>(terrainSurfaceConstrainSegment(surface, 0, twoSided == 1,
            second, first, 0.699999988079071f, 1.0f));
        result |= static_cast<int32_t>(terrainSurfaceConstrainSegment(surface, 1, twoSided == 1,
            second, first, 0.699999988079071f, 1.0f));
    }
    return result;
}

int32_t carCameraPathIntersectsTerrain(Spline* path, TerrainTopology* topology)
{
    const float step = finiteRatio(zonePreviewMaximumParameter(path), splineControlPolygonLength(path)) * 10.0f;
    float points[2][4];
    float parameter = 0.0f;
    int32_t sample = 0;
    TerrainSurface surface;
    terrainSurfaceInitialize(&surface, topology);
    while (parameter <= zonePreviewMaximumParameter(path)) {
        const int32_t current = sample & 1;
        const int32_t previous = current ^ 1;
        splineEvaluate(path, points[current], parameter);
        if (sample > 0) {
            float half[4], middle[4];
            for (int axis = 0; axis < 4; ++axis) {
                const float difference = points[current][axis] - points[previous][axis];
                half[axis] = difference * 0.5f;
                middle[axis] = half[axis] + points[previous][axis];
            }
            const float xx = half[0] * half[0];
            const float yy = half[1] * half[1];
            const float zz = half[2] * half[2];
            const float radius = sqrtf((xx + yy) + zz) + 0.5f;
            terrainSurfaceUpdate(&surface, middle, radius, NULL);
            if (terrainSurfaceConstrainSegment(&surface, 1, true,
                points[previous], points[current], 0.5f, 1.0f)) {
                terrainSurfaceDispose(&surface);
                return 1;
            }
        }
        parameter += step;
        ++sample;
    }
    terrainSurfaceDispose(&surface);
    return 0;
}

int32_t carCameraBuildTerrainCurves(CarRoadPreview* preview, TerrainSurface* surface,
                                    Spline* ground, Spline* eye, Spline* target,
                                    const CarCameraParameterMap* selection)
{
    if (!preview->steps) return 1;
    float along = 0.0f;
    float lastDistance = 0.0f;
    float nextDistance = 0.0f;
    int32_t index = 0;
    while (index < preview->count - 1 && nextDistance < preview->length) {
        float point[4];
        driverRoadPointPosition(preview->steps[index].pose, point, along, preview->initialAcross);
        terrainSurfaceUpdate(surface, point, 12.0f, NULL);
        point[2] = terrainSurfaceElevation(surface, point, NULL, NULL, NULL) + 1.5f;
        if (ground) splineAppendPoint(ground, point);
        if (eye) {
            float eyePoint[4] = {point[0], point[1], point[2] + 0.5f, 0.0f};
            splineAppendPoint(eye, eyePoint);
        }
        if (target) {
            float targetPoint[4] = {point[0], point[1], point[2] + 10.0f, 0.0f};
            splineAppendPoint(target, targetPoint);
        }
        lastDistance = nextDistance;
        nextDistance = lastDistance + 10.0f;
        along += 10.0f;
        while (nextDistance < preview->length && index < preview->count - 1 &&
               preview->steps[index].length < along) {
            along -= preview->steps[index].length;
            ++index;
            preview->initialAcross += preview->steps[index].acrossChange;
        }
        preview->initialAcross = carCameraApproachScalar(preview->initialAcross,
            preview->steps[index].across, 4.0f);
    }
    preview->length = lastDistance;
    if (ground && eye && target) {
        float eyeHeight[100], targetHeight[100];
        for (int32_t sample = 0; sample < 100; ++sample) {
            const float scaled = zonePreviewMaximumParameter(ground) * (float)sample;
            const float parameter = scaled * 0.010101010091602802f;
            float point[4], destination[4];
            splineEvaluate(ground, point, parameter);
            terrainSurfaceUpdate(surface, point, 10.699999809265137f, NULL);
            point[2] = terrainSurfaceElevation(surface, point, NULL, NULL, NULL) + 1.5f;
            splineEvaluate(eye, destination, parameter);
            destination[2] = point[2] + 0.5f;
            carCameraConstrainTerrain(surface, point, destination, 1, 0);
            eyeHeight[sample] = destination[2] - point[2];
            splineEvaluate(target, destination, parameter);
            destination[2] = point[2] + 10.0f;
            carCameraConstrainTerrain(surface, point, destination, 1, 0);
            targetHeight[sample] = destination[2] - point[2];
        }
        for (int32_t sample = 0; sample < 100; ++sample) {
            const float scaled = zonePreviewMaximumParameter(ground) * (float)sample;
            const int32_t center = convertFloatToInt32(scaled * 0.010101010091602802f);
            for (int32_t point = center - 1; point < center + 3; ++point)
                carCameraLimitPointHeight(ground, eye, point, eyeHeight[sample]);
        }
        for (int32_t sample = 0; sample < 100; ++sample) {
            const float scaled = zonePreviewMaximumParameter(ground) * (float)sample;
            const int32_t center = convertFloatToInt32(scaled * 0.010101010091602802f);
            for (int32_t point = center - 1; point < center + 3; ++point)
                carCameraLimitPointHeight(ground, target, point, targetHeight[sample]);
        }
    }
    if (selection && ground && eye) {
        const float first = zonePreviewMaximumParameter(eye) * carCameraParameterMapEvaluate(selection, 0.0f);
        const float last = zonePreviewMaximumParameter(eye) * carCameraParameterMapEvaluate(selection, 1.0f);
        const int32_t firstIndex = convertFloatToInt32(first);
        const int32_t lastIndex = convertFloatToInt32(last);
        carCameraFlattenEndHeight(ground, eye, firstIndex);
        carCameraFlattenEndHeight(ground, eye, lastIndex);
        if (target) {
            carCameraFlattenEndHeight(ground, target, firstIndex);
            carCameraFlattenEndHeight(ground, target, lastIndex);
        }
    }
    if (eye) carCameraLimitMaximumSlope(eye, 0.4000000059604645f);
    if (target) carCameraLimitMaximumSlope(target, 0.4000000059604645f);
    if (ground && !carCameraResampleHorizontal(ground, 0.33329999446868896f, NULL, 0)) return 0;
    if (eye && !carCameraResampleHorizontal(eye, 0.33329999446868896f, NULL, 0)) return 0;
    if (target && !carCameraResampleHorizontal(target, 0.33329999446868896f, NULL, 0)) return 0;
    return 1;
}

static float zoneAvoidDistanceThree(const float first[4],const float second[4])
{
    const float x=first[0]-second[0],y=first[1]-second[1],z=first[2]-second[2];
    const float xx=x*x,yy=y*y,zz=z*z;
    return sqrtf((xx+yy)+zz);
}

int32_t carCameraClusterTryAppend(CarCameraCluster* cluster,const float point[4],int32_t append)
{
    for(const CarCameraClusterPoint* member=cluster->first;member;member=member->next)
        if(20.0f<zoneAvoidDistanceThree(point,member->position))return 0;
    if(append){
        CarCameraClusterPoint* member=static_cast<CarCameraClusterPoint*>(malloc(sizeof(*member)));
        if(!member)return -1;
        member->next=NULL;
        memcpy(member->position,point,sizeof(member->position));
        if(cluster->last)cluster->last->next=member;
        else cluster->first=member;
        cluster->last=member;
    }
    return 1;
}

int32_t carCameraClusterAddPoint(CarCameraClusters* clusters,const float point[4],int32_t append)
{
    for(CarCameraCluster* cluster=clusters->first;cluster;cluster=cluster->next){
        const int32_t result=carCameraClusterTryAppend(cluster,point,append);
        if(result)return result;
    }
    CarCameraCluster* cluster=static_cast<CarCameraCluster*>(calloc(1,sizeof(*cluster)));
    if(!cluster)return -1;
    if(carCameraClusterTryAppend(cluster,point,1)<0){free(cluster);return -1;}
    if(clusters->last)clusters->last->next=cluster;
    else clusters->first=cluster;
    clusters->last=cluster;
    return 1;
}

void carCameraClusterDispose(CarCameraCluster* cluster)
{
    CarCameraClusterPoint* point=cluster->first;
    while(point){CarCameraClusterPoint* next=point->next;free(point);point=next;}
    cluster->first=cluster->last=NULL;
}

void carCameraClustersDispose(CarCameraClusters* clusters)
{
    CarCameraCluster* cluster=clusters->first;
    while(cluster){
        CarCameraCluster* next=cluster->next;
        carCameraClusterDispose(cluster);free(cluster);cluster=next;
    }
    clusters->first=clusters->last=NULL;
}

int32_t carCameraAvoidanceExcluded(const CarCameraAvoidanceExclusions* exclusions,const PursuitCollisionEntry* entry)
{
    for(int32_t index=0;index<exclusions->count;++index)
        if(exclusions->entries[index]==entry)return 1;
    return 0;
}

void carCameraAvoidanceExclude(CarCameraAvoidanceExclusions* exclusions,const PursuitCollisionEntry* entry)
{
    if(carCameraAvoidanceExcluded(exclusions,entry)||exclusions->count>=6)return;
    exclusions->entries[exclusions->count++]=entry;
}

static const float* zoneAvoidEntryCenter(const PursuitCollisionEntry* entry)
{
    return entry->movement->bindings.state->centerPosition;
}

static CarMovement* zoneAvoidMovementForCar(const WorldSimulation* simulation,const Car* car)
{
    const size_t index=car->tableIndex;
    if(index>=simulation->frameCount||simulation->world->carTable[index]!=car||!simulation->frames[index])return NULL;
    return carFrameMovement(simulation->frames[index]);
}

static int32_t zoneAvoidCollectPursuit(CarCameraClusters* clusters,const WorldSimulation* simulation)
{
    const PursuitManager* manager=simulation->pursuit;
    CarCameraAvoidanceExclusions exclusions={{0},0};
    if(manager->spikes){
        for(const PursuitSpikeStrip* strip=manager->spikes->first;strip;strip=strip->next){
            float center[4];pursuitSpikeStripCenter(strip,center);
            if(carCameraClusterAddPoint(clusters,center,1)<0)return 0;
            const int32_t count=pursuitManagerRoadBlockCount(manager);
            for(int32_t index=0;index<count;++index){
                const PursuitRoadBlock* block=pursuitManagerRoadBlockAt(manager,index);
                if(!block||block->siteIndex!=strip->ownerSelector)continue;
                for(int member=0;member<2;++member)
                    if(block->entries[member])carCameraAvoidanceExclude(&exclusions,block->entries[member]);
            }
        }
    }
    const PursuitCollisionContext* context=manager->collision;
    for(size_t index=0;index<context->entryCount;++index){
        const PursuitCollisionEntry* entry=context->entries[index];
        if(!entry)continue;
        if(entry->state==4){
            if(carCameraAvoidanceExcluded(&exclusions,entry))continue;
            float center[4];memcpy(center,zoneAvoidEntryCenter(entry),sizeof(center));
            const int32_t blockCount=pursuitManagerRoadBlockCount(manager);
            for(int32_t blockIndex=0;blockIndex<blockCount;++blockIndex){
                const PursuitRoadBlock* block=pursuitManagerRoadBlockAt(manager,blockIndex);
                if(!block||(block->entries[0]!=entry&&block->entries[1]!=entry))continue;
                if(block->entries[0]&&block->entries[1]){
                    const float* first=zoneAvoidEntryCenter(block->entries[0]);
                    const float* second=zoneAvoidEntryCenter(block->entries[1]);
                    if(zoneAvoidDistanceThree(first,second)<10.0f)
                        for(int axis=0;axis<4;++axis){const float sum=first[axis]+second[axis];center[axis]=sum*0.5f;}
                }
                for(int member=0;member<2;++member)
                    if(block->entries[member])carCameraAvoidanceExclude(&exclusions,block->entries[member]);
                break;
            }
            if(carCameraClusterAddPoint(clusters,center,1)<0)return 0;
        }else if(entry->state==3||entry->state==5||entry->state==7||entry->state==8){
            if(!carCameraAvoidanceExcluded(&exclusions,entry)&&carCameraClusterAddPoint(clusters,zoneAvoidEntryCenter(entry),1)<0)return 0;
        }
    }
    const Chopper* chopper=simulation->chopper;
    if(chopper&&(chopper->phase==0||chopper->phase==1||chopper->phase==2||chopper->phase==4))
        if(carCameraClusterAddPoint(clusters,chopper->position,1)<0)return 0;
    return 1;
}

static int32_t zoneAvoidCollectRace(CarCameraClusters* clusters,const WorldSimulation* simulation)
{
    const World* world=simulation->world;
    for(int32_t index=world->firstGroupEnd;index<world->secondGroupEnd;++index){
        const Car* car=world->carTable[index];
        if(!car->raceEntry)return 0;
        if(car->raceEntry->position==1){
            if(carCameraClusterAddPoint(clusters,car->position,0)<0)return 0;
            break;
        }
    }
    for(int32_t index=world->firstGroupEnd;index<world->secondGroupEnd;++index){
        const Car* car=world->carTable[index];
        CarMovement* movement=zoneAvoidMovementForCar(simulation,car);
        if(!car->raceEntry||!movement)return 0;
        if(car->raceEntry->position==2||movement->bindings.state->gear==0)
            if(carCameraClusterAddPoint(clusters,car->position,0)<0)return 0;
    }
    if(!simulation->collisions)return 0;
    for(const CollisionImpact* impact=simulation->collisions->firstImpact;impact;impact=impact->next)
        if(collisionImpactIsCarPair(impact)&&carCameraClusterAddPoint(clusters,impact->point,0)<0)return 0;
    return 1;
}

typedef struct AvoidanceCandidate { float parameter;float point[4]; } AvoidanceCandidate;

static float zoneAvoidCompareCandidates(const void* first,const void* second)
{
    return ((const AvoidanceCandidate*)first)->parameter-((const AvoidanceCandidate*)second)->parameter;
}

int32_t carCameraBuildAvoidancePath(const WorldSimulation* simulation,
    TerrainSurface* surface,const Spline* search,const Spline* original,Spline* output,
    const CarCameraParameterMap* inverseMap,const CarCameraParameterMap* selectionMap,
    float* markers,int32_t maximumMarkers)
{
    if(maximumMarkers<0||maximumMarkers>4||original->count<2||output->count!=0)return -1;
    CarCameraClusters clusters{};
    if(!(simulation->pursuit?zoneAvoidCollectPursuit(&clusters,simulation):zoneAvoidCollectRace(&clusters,simulation))){
        carCameraClustersDispose(&clusters);return -1;
    }
    const int32_t eligibleCount=original->count-1;
    uint8_t* used=static_cast<uint8_t*>(calloc((size_t)eligibleCount,1));
    if(!used){carCameraClustersDispose(&clusters);return -1;}
    AvoidanceCandidate candidates[4];
    int32_t accepted=0;
    const float maximum=zonePreviewMaximumParameter(original);
    for(const CarCameraCluster* cluster=clusters.first;cluster&&accepted<maximumMarkers;cluster=cluster->next){
        float center[4];carCameraClusterMean(cluster,center);
        const float parameter=splineFindDirectionParameter(search,center,-0.6000000238418579f,0.009999999776482582f);
        const float normalized=finiteRatio(parameter,zonePreviewMaximumParameter(search));
        const float mapped=carCameraParameterMapInverse(inverseMap,normalized);
        const float selected=maximum*carCameraParameterMapEvaluate(selectionMap,mapped);
        int32_t index=convertFloatToInt32(selected);
        if(selected<(float)index)--index;
        if(index<0||index>=eligibleCount||used[index]){
            --index;
            if(index<0||index>=eligibleCount||used[index])continue;
        }
        const float key=(float)index;
        if(key<=maximum*carCameraParameterMapEvaluate(selectionMap,0.0f)||
           maximum*carCameraParameterMapEvaluate(selectionMap,1.0f)<=key)continue;
        float pathPoint[4];splineEvaluate(search,pathPoint,parameter);
        if(!simulation->pursuit&&carCameraTerrainOccludes(surface,pathPoint,center))continue;
        if(!(zoneAvoidDistanceThree(center,pathPoint)<60.0f))continue;
        used[index]=1;
        candidates[accepted].parameter=key;
        memcpy(candidates[accepted].point,center,sizeof(center));
        ++accepted;
    }
    free(used);
    if(!arrayHeapSort(candidates,accepted,sizeof(candidates[0]),zoneAvoidCompareCandidates)){
        carCameraClustersDispose(&clusters);return -1;
    }
    for(int32_t index=0;index<accepted;++index)markers[index]=candidates[index].parameter;
    int32_t next=0;
    for(int32_t index=0;index<original->count;++index){
        const float* point=next<accepted&&markers[next]==(float)index?candidates[next++].point:original->points[index];
        const int32_t previous=output->count;
        splineAppendPoint(output,point);
        if(output->count!=previous+1){carCameraClustersDispose(&clusters);return -1;}
    }
    const int32_t sampled=carCameraResampleHorizontal(output,0.33329999446868896f,markers,accepted);
    carCameraClustersDispose(&clusters);
    return sampled?accepted:-1;
}

void carCameraLiftNearGround(TerrainSurface* surface, float point[4], float clearance)
{
    terrainSurfaceUpdate(surface, point, 10.0f, NULL);
    const float ground = terrainSurfaceElevation(surface, point, NULL, NULL, NULL);
    if (ground - clearance < point[2] && point[2] < ground + clearance)
        point[2] = ground + clearance;
}


// Original PreviewRoute methods. Retained PAL branch/order behavior.

float carRoadPreviewChooseLane(const CarRoadPreview* preview, CarRoadPreviewStep* step,
                               const CarRecoveryGroup* group, const CarRecoveryPose* pose,
                               const float point[2], int32_t vehicleClass)
{
    float bestDistance = 1000000.0f;
    float bestAcross = 0.0f;
    int32_t bestLane = -1;
    for (int32_t candidate = 0; candidate < 3; ++candidate) {
        int32_t lane = vehicleClass * 2 + candidate;
        float across;
        const int enabled = candidate < 2 && (group->laneFlags[lane] & 3u) != 0;
        if (enabled) {
            across = (float)((int32_t)pose->laneOffsetsQuantized[lane] * 256) * 0.0000152587890625f;
        } else if (candidate < 2) {
            continue;
        } else {
            const float projection = pose->direction[0] * preview->direction[0] +
                                      pose->direction[1] * preview->direction[1];
            if (projection < 0.0f) break;
            lane = -1;
            across = 0.0f;
        }
        float projected[2];
        carRoadPreviewPointXY(pose, projected, 0.0f, across);
        const float x = point[0] - projected[0];
        const float y = point[1] - projected[1];
        const float distance = sqrtf(x * x + y * y);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestAcross = across;
            bestLane = lane;
            if (enabled) break;
        }
    }
    step->lane = bestLane;
    step->group = group;
    step->pose = pose;
    step->across = bestAcross;
    return bestDistance;
}

int32_t carRoadPreviewFindStart(const CarRoadPreview* preview, CarRoadPreviewStep* step,
                                const CarRecoveryPointRegistry* registry, const float point[2],
                                float maximumDistance, int32_t vehicleClass,
                                const CarRecoveryPose* previous, int32_t requireForwardDistance)
{
    float closest = 1000000.0f;
    CarRoadPreviewStep selected = {};
    selected.lane = -1;
    const CarRecoveryPose* lastCandidate = NULL;
    for (size_t index = 0; index < registry->groupCount; ++index) {
        const CarRecoveryGroup* group = &registry->groups[index];
        if (group->category != 0) continue;
        const CarRecoveryPose* candidate = carRecoveryFindInGroup(registry, group, point,
                                                                  maximumDistance, 0.0f, 1u);
        lastCandidate = candidate;
        if (!candidate || candidate == previous) continue;
        CarRoadPreviewStep trial = {};
        const float distance = carRoadPreviewChooseLane(preview, &trial, group, candidate,
                                                        point, vehicleClass);
        if (distance < closest) {
            selected = trial;
            closest = distance;
        }
    }
    *step = selected;
    if (!lastCandidate || !selected.pose) return 0;
    if (!requireForwardDistance) return 1;
    return previous && !(selected.pose->routeDistance < previous->routeDistance);
}

float carRoadPreviewAdvance(CarRoadPreview* preview, const CarRecoveryPointRegistry* registry,
                            int32_t index, float maximumDistance, int32_t vehicleClass,
                            int32_t requireForwardDistance)
{
    CarRoadPreviewStep* previous = &preview->steps[index - 1];
    CarRoadPreviewStep* current = &preview->steps[index];
    float first[4], second[4];
    carRoadPreviewStepPosition(previous, first);
    const ptrdiff_t nextIndex = previous->pose - previous->group->poses + 1;
    if (previous->group->poseCount < nextIndex + 1) {
        if (!carRoadPreviewFindStart(preview, current, registry, first, maximumDistance,
                                     vehicleClass, previous->pose, requireForwardDistance))
            return 0.0f;
        const float x = previous->pose->position[0] - current->pose->position[0];
        const float y = previous->pose->position[1] - current->pose->position[1];
        current->acrossChange = x * current->pose->direction[1] - y * current->pose->direction[0];
    } else {
        current->group = previous->group;
        current->pose = &previous->group->poses[nextIndex];
        current->lane = previous->lane;
        current->across = current->lane == -1 ? 0.0f :
            (float)((int32_t)current->pose->laneOffsetsQuantized[current->lane] * 256) * 0.0000152587890625f;
        current->acrossChange = 0.0f;
    }
    carRoadPreviewStepPosition(current, second);
    const float x = second[0] - first[0];
    const float y = second[1] - first[1];
    const float z = second[2] - first[2];
    const float length = sqrtf((x * x + y * y) + z * z);
    previous->length = length;
    return length;
}

int32_t carRoadPreviewInitialize(CarRoadPreview* preview, const CarRecoveryPointRegistry* registry,
                                 const CarRecoveryGroup* group, const CarRecoveryPose* pose,
                                 const float position[4], const float direction[2],
                                 float maximumFirstDistance, float length, int32_t vehicleClass,
                                 int32_t requireForwardDistance)
{
    if (!preview || !registry || !position || !direction || vehicleClass < 0 || vehicleClass > 9)
        return 0;
    preview->steps = NULL;
    preview->count = convertFloatToInt32(length * 0.25f);
    preview->length = length;
    preview->direction[0] = direction[0];
    preview->direction[1] = direction[1];
    if (preview->count <= 0) return 0;
    preview->steps = (CarRoadPreviewStep*)calloc((size_t)preview->count, sizeof(*preview->steps));
    if (!preview->steps) { preview->count = 0; return 0; }
    int32_t found;
    if (group && pose)
        found = carRoadPreviewChooseLane(preview, &preview->steps[0], group, pose,
                                          position, vehicleClass) <= maximumFirstDistance;
    else
        found = carRoadPreviewFindStart(preview, &preview->steps[0], registry, position,
                                         maximumFirstDistance, vehicleClass, NULL, requireForwardDistance);
    if (!found) { carRoadPreviewDispose(preview); return 0; }
    const CarRecoveryPose* first = preview->steps[0].pose;
    const float x = position[0] - first->position[0];
    const float y = position[1] - first->position[1];
    preview->initialAcross = x * first->direction[1] - y * first->direction[0];
    float accumulated = 0.0f;
    int32_t index = 1;
    for (; index < preview->count; ++index) {
        accumulated = accumulated + carRoadPreviewAdvance(preview, registry, index,
                                                           maximumFirstDistance, vehicleClass,
                                                           requireForwardDistance);
        if (preview->length < accumulated || !preview->steps[index].pose) break;
    }
    preview->length = accumulated;
    preview->count = index;
    return 1;
}

void carRoadPreviewDispose(CarRoadPreview* preview)
{
    free(preview->steps);
    preview->steps = NULL;
    preview->count = 0;
}


/* Retained PAL1655C0 minimum-slope helper. It shares the complete Spline
 * protocol with the maximum helper above. Its individual Beta identity
 * remains unresolved; moving this native dependency does not accept one. */

void carCameraRaiseMinimumSlope(Spline* curve, float gradient)
{
    for (int32_t index = 1; index < curve->count; ++index) {
        const float x = curve->points[index][0] - curve->points[index - 1][0];
        const float y = curve->points[index][1] - curve->points[index - 1][1];
        const float xx = x * x, yy = y * y;
        const float change = sqrtf(xx + yy) * gradient;
        const float minimum = curve->points[index - 1][2] + change;
        if (curve->points[index][2] < minimum) curve->points[index][2] = minimum;
    }
    for (int32_t index = curve->count - 2; index >= 0; --index) {
        const float x = curve->points[index][0] - curve->points[index + 1][0];
        const float y = curve->points[index][1] - curve->points[index + 1][1];
        const float xx = x * x, yy = y * y;
        const float change = sqrtf(xx + yy) * gradient;
        const float minimum = curve->points[index + 1][2] + change;
        if (curve->points[index][2] < minimum) curve->points[index][2] = minimum;
    }
}
