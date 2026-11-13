#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bWare.hpp"

#include <math.h>
#include <string.h>
#include <cstring>
#include <cmath>

void carReplayAnimationRegistryInitialize(CarReplayAnimationRegistry* registry)
{
    if (registry) memset(registry, 0, sizeof(*registry));
}

int32_t carReplayAnimationInitialize(CarReplayAnimationRegistry* registry,
    CarReplayAnimationController* controller, const EditorMarkerModel* model)
{
    if (!registry || !controller) return 0;
    /* PAL1A67F8 appends to the actual instance list. The model reference is
     * the native borrowed projection of its initialized eModel subobject. */
    memset(controller, 0, sizeof(*controller));
    controller->catalogue = &registry->root;
    controller->model = model;
    controller->blend = 1.0f;
    controller->registry = registry;
    controller->previous = registry->last;
    if (registry->last) registry->last->next = controller;
    else registry->first = controller;
    registry->last = controller;
    return 1;
}

int32_t carReplayAnimationUnregisterController(CarReplayAnimationController* controller)
{
    if (!controller || !controller->registry) return 0;
    CarReplayAnimationRegistry* registry = controller->registry;
    if (controller->previous) controller->previous->next = controller->next;
    else registry->first = controller->next;
    if (controller->next) controller->next->previous = controller->previous;
    else registry->last = controller->previous;
    controller->registry = NULL;
    controller->next = NULL;
    controller->previous = NULL;
    return 1;
}

void carReplayAnimationReconnect(CarReplayAnimationController* controller)
{
    if (controller->primaryKey)
        controller->primary = carReplayAnimationFind(controller->catalogue,
            controller->primaryKey);
    if (controller->secondaryKey)
        controller->secondary = carReplayAnimationFind(controller->catalogue,
            controller->secondaryKey);
}

void carReplayAnimationReconnectAll(CarReplayAnimationRegistry* registry)
{
    for (CarReplayAnimationController* controller = registry->first;
            controller; controller = controller->next) {
        carReplayAnimationReconnect(controller);
    }
}

void carReplayAnimationResourceEndFixup(CarReplayAnimationRegistry* registry)
{
    /* Actual resource-end phase103FC0..103FD4. Unload first removes the
     * catalogue, then this phase rebinds every live cache before storage dies. */
    if (registry && registry->reconnectRequired) {
        carReplayAnimationReconnectAll(registry);
        registry->reconnectRequired = 0;
    }
}

void carReplayAnimationBounds(const CarReplayAnimationController* controller,float minimum[4],float maximum[4])
{
    if (!controller->primary) return;
    vectorBoundsFromPoint3(minimum,maximum,controller->primary->minimum);
    vectorBoundsExpandPoint3(minimum,maximum,controller->primary->maximum);
    if (controller->secondary) {
        vectorBoundsExpandPoint3(minimum,maximum,controller->secondary->minimum);
        vectorBoundsExpandPoint3(minimum,maximum,controller->secondary->maximum);
    }
    float modelMinimum[3]={0},modelMaximum[3]={0};
    if (controller->model) editorMarkerModelBounds(controller->model,modelMinimum,modelMaximum);
    float extent=fmaxf(fabsf(modelMinimum[0]),fabsf(modelMinimum[1]));
    extent=fmaxf(extent,fabsf(modelMinimum[2]));
    extent=fmaxf(extent,fabsf(modelMaximum[0]));
    extent=fmaxf(extent,fabsf(modelMaximum[1]));
    extent=fmaxf(extent,fabsf(modelMaximum[2]));
    for (int axis=0;axis<3;++axis) { minimum[axis]-=extent;maximum[axis]+=extent; }
}

namespace hp2 {
// PAL1A6E9C compares this signed bone index after the parent multiply;
// 3110D0 supplies one independent multiplier for each basis column.
std::int32_t nBoneScale=-1;
float vBoneScale[4]{};

struct DriverPoseCache::Storage {
    std::array<std::uint8_t,CAR_DRIVER_VISUAL_BONE_COUNT*96> source{};
    std::array<std::int16_t,CAR_DRIVER_VISUAL_BONE_COUNT> parents{};
    std::array<std::uint8_t,CAR_DRIVER_VISUAL_BONE_COUNT> order{};
    std::array<std::array<float,4>,CAR_DRIVER_VISUAL_BONE_COUNT> rotations{};
    std::array<std::array<float,16>,CAR_DRIVER_VISUAL_BONE_COUNT> local{},world{};
    std::vector<std::array<float,16>> matrices;
    std::array<float,3> scale{};
    std::int32_t scaleBone{-1};
    std::size_t count{};
    bool skeletonValid{},poseValid{};
};

DriverPoseCache::DriverPoseCache() noexcept=default;
DriverPoseCache::~DriverPoseCache()=default;
DriverPoseCache::DriverPoseCache(DriverPoseCache&&) noexcept=default;
DriverPoseCache& DriverPoseCache::operator=(DriverPoseCache&&) noexcept=default;

const std::vector<std::array<float,16>>& DriverPoseCache::Prepare(
    const EditorMarkerAnimationTree& tree,const float rotations[][4],std::size_t rotationCount)
{
    Require(tree.nodeCount>0 && tree.nodeCount<=CAR_DRIVER_VISUAL_BONE_COUNT && rotations &&
        static_cast<std::size_t>(tree.nodeCount)<=rotationCount && tree.authoredNodes &&
        static_cast<std::size_t>(tree.nodeCount)<=tree.authoredBytes/96,
        "Animation pose lacks its authored skeleton or quaternion table");
    if(!storage_)storage_=std::make_unique<Storage>();
    auto& cache=*storage_;const auto count=static_cast<std::size_t>(tree.nodeCount);
    if(!cache.skeletonValid || cache.count!=count ||
        std::memcmp(cache.source.data(),tree.authoredNodes,count*96)) {
        std::array<std::uint8_t,CAR_DRIVER_VISUAL_BONE_COUNT> visited{},order{};
        std::array<std::int16_t,CAR_DRIVER_VISUAL_BONE_COUNT> parents{};
        std::size_t ordered=0;
        const auto visit=[&](const auto& self,std::size_t index,std::int16_t parent)->void {
            Require(index<count && !visited[index],"Animation hierarchy has an invalid or repeated bone");
            visited[index]=1;order[ordered++]=static_cast<std::uint8_t>(index);parents[index]=parent;
            std::int8_t child;std::memcpy(&child,tree.authoredNodes+index*96+0x4e,1);
            while(child) {
                Require(child>0 && static_cast<std::size_t>(child)<count,
                    "Animation child index exceeds its authored tree");
                self(self,static_cast<std::size_t>(child),static_cast<std::int16_t>(index));
                std::memcpy(&child,tree.authoredNodes+static_cast<std::size_t>(child)*96+0x4f,1);
            }
        };
        visit(visit,0,-1);
        Require(ordered==count,"Animation tree contains a bone outside its root hierarchy");
        // Publish the plan only after the complete current hierarchy passes.
        // No borrowed resource pointer survives this call.
        cache.order=order;cache.parents=parents;cache.count=count;
        std::memcpy(cache.source.data(),tree.authoredNodes,count*96);
        cache.skeletonValid=true;cache.poseValid=false;++work_.skeletonPlans;
    }
    std::array<float,3> scale;std::memcpy(scale.data(),vBoneScale,sizeof(scale));
    const auto scaled=nBoneScale;
    bool finite=true;
    for(std::size_t index=0;index<count;++index)
        for(std::size_t lane=0;lane<4;++lane)finite&=std::isfinite(rotations[index][lane]);
    if(scaled>=0 && static_cast<std::size_t>(scaled)<count)
        for(const auto value:scale)finite&=std::isfinite(value);
    if(!finite)cache.poseValid=false;
    const bool scaleChanged=cache.scaleBone!=scaled || std::memcmp(cache.scale.data(),scale.data(),sizeof(scale));
    std::array<std::uint8_t,CAR_DRIVER_VISUAL_BONE_COUNT> dirty{};
    float identity[16];eIdentity(identity);
    cache.matrices.resize(count);
    for(std::size_t ordinal=0;ordinal<count;++ordinal) {
        const auto index=cache.order[ordinal];const auto parent=cache.parents[index];
        const bool localChanged=!cache.poseValid ||
            std::memcmp(cache.rotations[index].data(),rotations[index],4*sizeof(float));
        if(localChanged) {
            vectorQuaternionToMatrix(rotations[index],cache.local[index].data());
            for(std::size_t axis=0;axis<3;++axis)
                std::memcpy(&cache.local[index][12+axis],cache.source.data()+index*96+0x40+axis*4,sizeof(float));
            std::memcpy(cache.rotations[index].data(),rotations[index],4*sizeof(float));
            ++work_.quaternionMatrices;
        }
        dirty[index]=localChanged || (parent>=0 && dirty[static_cast<std::size_t>(parent)]) ||
            (scaleChanged && (index==cache.scaleBone || index==scaled));
        if(!dirty[index])continue;
        auto& matrix=cache.world[index];matrix=cache.local[index];
        eMulMatrix(matrix.data(),matrix.data(),parent>=0 ? cache.world[static_cast<std::size_t>(parent)].data():identity);
        if(scaled>=0 && index==scaled)
            for(std::size_t column=0;column<3;++column)
                for(std::size_t row=0;row<4;++row)matrix[column*4+row]*=scale[column];
        ++work_.parentMatrices;
    }
    for(std::size_t index=0;index<count;++index)if(dirty[index]) {
        float inverseBind[16];std::memcpy(inverseBind,cache.source.data()+index*96,sizeof(inverseBind));
        auto& matrix=cache.matrices[index];matrix=cache.world[index];
        // Preserve the original aliasing and multiplication order exactly.
        eMulMatrix(matrix.data(),inverseBind,matrix.data());++work_.inverseBindMatrices;
        for(const auto value:matrix)finite&=std::isfinite(value);
    }
    cache.scaleBone=scaled;cache.scale=scale;cache.poseValid=finite;
    return cache.matrices;
}

static std::size_t renderPosedAnimation(CarReplayAnimationController& animation,
    const float localToWorld[16],const float rootTranslation[3],
    const SceneLightContext* lightContext,const CarCinematicScreen& view,EcstasyFrame& frame,
    std::uint32_t flags,const std::vector<std::array<float,16>>& matrices)
{
    float transform[16],rootOffset[4];std::memcpy(transform,localToWorld,sizeof(transform));
    float rotation[16];std::memcpy(rotation,localToWorld,sizeof(rotation));
    rotation[12]=rotation[13]=rotation[14]=0.0f;
    eMulVector3(rootOffset,rotation,rootTranslation);
    for(std::size_t axis=0;axis<3;++axis)transform[12+axis]+=rootOffset[axis];
    transform[15]=1.0f;
    return frame.RenderModel(*const_cast<EditorMarkerModel*>(animation.model),transform,
        view,flags,1.0f,0,lightContext,&matrices);
}

std::size_t RenderDriverAnimationPose(DriverPoseCache& cache,CarReplayAnimationController& animation,
    const float localToWorld[16],const float rootTranslation[3],const float rotations[][4],
    std::size_t rotationCount,const SceneLightContext* lightContext,
    const CarCinematicScreen& view,EcstasyFrame& frame,std::uint32_t flags)
{
    if(!animation.model || !animation.model->solid)return 0;
    const auto* tree=editorMarkerModelAnimationTree(animation.model);
    if(!tree || tree->nodeCount<=0)return 0;
    Require(localToWorld && rootTranslation,"Animation pose lacks its authored skeleton or quaternion table");
    const auto& matrices=cache.Prepare(*tree,rotations,rotationCount);
    return renderPosedAnimation(animation,localToWorld,rootTranslation,lightContext,view,frame,flags,matrices);
}
std::size_t RenderAnimationPose(CarReplayAnimationController& animation,
    const float localToWorld[16],const float rootTranslation[3],
    const float rotations[][4],std::size_t rotationCount,const SceneLightContext* lightContext,
    const CarCinematicScreen& view,EcstasyFrame& frame,std::uint32_t flags,
    std::vector<std::array<float,16>>* posedMatrices)
{
    // PAL1A6B80 uses this model's resource-owned tree, then sends all the
    // posed matrices to the same eModel::RenderStrips owner. The native
    // frame retains the resulting vertices until physical GPU completion.
    if(!animation.model || !animation.model->solid)return 0;
    const auto* tree=editorMarkerModelAnimationTree(animation.model);
    if(!tree || tree->nodeCount<=0)return 0;
    const auto count=static_cast<std::size_t>(tree->nodeCount);
    Require(localToWorld && rootTranslation && rotations && count<=rotationCount &&
        tree->authoredNodes && count<=tree->authoredBytes/96,
        "Animation pose lacks its authored skeleton or quaternion table");
    // Use the caller-owned native pose directly when the NIS also needs it
    // for hand props. The previous path allocated an internal bone array and
    // then copied every matrix to an additional caller allocation per actor.
    std::vector<std::array<float,16>> localMatrices;
    auto& matrices=posedMatrices ? *posedMatrices : localMatrices;
    matrices.resize(count);
    // Authored bone links are signed 8-bit indices. Common animation trees
    // fit in stack storage. Retain the original dynamic path for larger
    // unusual trees so that their validation and failure semantics remain.
    std::array<std::uint8_t,128> inlineVisited{};
    std::vector<std::uint8_t> largeVisited;
    if(count>inlineVisited.size())largeVisited.resize(count);
    std::uint8_t* visited=count>inlineVisited.size() ? largeVisited.data() : inlineVisited.data();
    // PoseMatrixRecursive1A6E00: local quaternion/translation, parent,
    // then each first-child/next-sibling link. Node0 is the authored root.
    const auto pose=[&](const auto& self,std::size_t index,const float parent[16])->void {
        Require(index<count && !visited[index],"Animation hierarchy has an invalid or repeated bone");
        visited[index]=true;
        const auto* node=tree->authoredNodes+index*96;
        auto& matrix=matrices[index];
        vectorQuaternionToMatrix(rotations[index],matrix.data());
        for(std::size_t axis=0;axis<3;++axis)
            std::memcpy(&matrix[12+axis],node+0x40+axis*4,sizeof(float));
        eMulMatrix(matrix.data(),matrix.data(),parent);
        if(nBoneScale>=0 && index==static_cast<std::size_t>(nBoneScale))
            for(std::size_t column=0;column<3;++column)
                for(std::size_t row=0;row<4;++row)
                    matrix[column*4+row]*=vBoneScale[column];
        std::int8_t child;std::memcpy(&child,node+0x4e,1);
        while(child) {
            Require(child>0 && static_cast<std::size_t>(child)<count,
                "Animation child index exceeds its authored tree");
            self(self,static_cast<std::size_t>(child),matrix.data());
            std::memcpy(&child,tree->authoredNodes+static_cast<std::size_t>(child)*96+0x4f,1);
        }
    };
    float identity[16];eIdentity(identity);pose(pose,0,identity);
    for(std::size_t index=0;index<count;++index) {
        Require(visited[index],"Animation tree contains a bone outside its root hierarchy");
        float inverseBind[16];std::memcpy(inverseBind,tree->authoredNodes+index*96,sizeof(inverseBind));
        eMulMatrix(matrices[index].data(),inverseBind,matrices[index].data());
    }
    // The caller's NIS hand-prop palette and the driver-owned local cache
    // both reach the same synchronous model consumer. Only frame-owned
    // skinned strips survive this call; no palette pointer reaches the GPU.
    return renderPosedAnimation(animation,localToWorld,rootTranslation,lightContext,view,frame,flags,matrices);
}
}


// Original eAnimation::SetAnimation, PAL1A6968..1A69BC.

void carReplayAnimationSelect(CarReplayAnimationController* controller,uint32_t key,float phase)
{
    if (controller->primaryKey!=key) {
        controller->primaryKey=key;controller->primary=carReplayAnimationFind(controller->catalogue,key);
    }
    controller->phase=phase;controller->blend=1.0f;
}
