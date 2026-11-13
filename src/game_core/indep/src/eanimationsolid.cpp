#include "bware/inc/bMath.hpp"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"

#include <cmath>
#include <cstring>

int32_t editorMarkerRigidAnimationMatrixAtFrame(const EditorMarkerRigidAnimation* animation,
    float output[16],int32_t frame)
{
    if(!animation || !output || !animation->matrices || animation->frameCount<=0 || frame<0)return 0;
    const int32_t sample=frame%animation->frameCount;
    std::memcpy(output,animation->matrices+static_cast<size_t>(sample)*16,16*sizeof(float));
    return 1;
}

int32_t editorMarkerRigidAnimationMatrixAtTime(const EditorMarkerRigidAnimation* animation,
    float output[16],int32_t time)
{
    if(!animation || animation->frameDuration<=0 || time<0)return 0;
    return editorMarkerRigidAnimationMatrixAtFrame(animation,output,time/animation->frameDuration);
}

const CarReplayAnimationClip* carReplayAnimationFind(
    const CarReplayAnimationCatalogue* catalogue, uint32_t key)
{
    for (const CarReplayAnimationCatalogue* resource = catalogue;
            resource; resource = resource->next) {
        for (size_t index = 0; index < resource->count; ++index) {
            if (resource->clips[index].key == key)
                return resource->clips + index;
        }
    }
    return NULL;
}

int32_t carReplayAnimationAppendCatalogue(CarReplayAnimationRegistry* registry,
    CarReplayAnimationCatalogue* catalogue)
{
    if (!registry || !catalogue || catalogue == &registry->root ||
        catalogue->next || (catalogue->count && !catalogue->clips)) return 0;
    CarReplayAnimationCatalogue* tail = &registry->root;
    while (tail->next) {
        if (tail->next == catalogue) return 0;
        tail = tail->next;
    }
    /* LoaderAnimations appends at the tail. Duplicate keys are retained;
     * Find returns the first loaded match, even after a later duplicate load. */
    tail->next = catalogue;
    registry->reconnectRequired = 1;
    return 1;
}

int32_t carReplayAnimationRemoveCatalogue(CarReplayAnimationRegistry* registry,
    CarReplayAnimationCatalogue* catalogue)
{
    if (!registry || !catalogue || catalogue == &registry->root) return 0;
    CarReplayAnimationCatalogue* previous = &registry->root;
    while (previous->next && previous->next != catalogue)
        previous = previous->next;
    if (!previous->next) return 0;
    previous->next = catalogue->next;
    catalogue->next = NULL;
    registry->reconnectRequired = 1;
    return 1;
}

int32_t carDriverAnimationReadRotation(const CarReplayAnimationClip* clip,
    float output[4], int32_t bone, float frame)
{
    const int32_t sample = convertFloatToInt32(frame);
    if (!clip || !output || !clip->boneRotations || bone < 0 ||
        bone >= clip->boneCount || sample < 0 || sample >= clip->frameCount) {
        return 0;
    }

    const size_t index = ((size_t)bone * clip->frameCount + (size_t)sample) * 4;
    const float scale = 0.000030518509447574615f;
    for (int32_t lane = 0; lane < 4; ++lane) {
        output[lane] = (float)clip->boneRotations[index + (size_t)lane] * scale;
    }
    return 1;
}

int32_t carDriverAnimationWriteRotation(const CarReplayAnimationClip* clip,
    const float input[4], int32_t bone, float frame)
{
    if (!clip || !input || !clip->boneRotations || !clip->frameCount ||
        bone < 0 || bone >= clip->boneCount) {
        return 0;
    }

    int32_t sample = convertFloatToInt32(frame);
    if (sample < 0 || sample >= clip->frameCount) sample = 0;
    const size_t index = ((size_t)bone * clip->frameCount + (size_t)sample) * 4;
    for (int32_t lane = 0; lane < 4; ++lane) {
        const float scaled = input[lane] * 32767.0f;
        const uint16_t bits = (uint16_t)(uint32_t)convertFloatToInt32(scaled);
        const int32_t value = bits <= INT16_MAX ? (int32_t)bits : (int32_t)bits - 65536;
        clip->boneRotations[index + (size_t)lane] = (int16_t)value;
    }
    return 1;
}

int32_t carDriverAnimationReadRotationRange(const CarReplayAnimationClip* clip,
    float output[][4],size_t outputCount,int32_t firstBone,size_t count,float frame)
{
    if(!clip || firstBone<0 || count>outputCount ||
        static_cast<size_t>(firstBone)>clip->boneCount ||
        count>clip->boneCount-static_cast<size_t>(firstBone))return 0;
    if(!count)return 1;
    const int32_t sample=convertFloatToInt32(frame);
    if(!output || !clip->boneRotations || sample<0 || sample>=clip->frameCount)return 0;
    const float scale=0.000030518509447574615f;
    for(size_t bone=0;bone<count;++bone) {
        const size_t index=((static_cast<size_t>(firstBone)+bone)*clip->frameCount+
            static_cast<size_t>(sample))*4;
        for(size_t lane=0;lane<4;++lane)
            output[bone][lane]=static_cast<float>(clip->boneRotations[index+lane])*scale;
    }
    return 1;
}

int32_t carDriverAnimationReadAllRotations(const CarReplayAnimationClip* clip,
    float output[][4],size_t outputCount,float frame)
{
    if(!clip || outputCount<clip->boneCount)return 0;
    return carDriverAnimationReadRotationRange(clip,output,outputCount,0,clip->boneCount,frame);
}

namespace hp2 {

CarReplayAnimationResources::CarReplayAnimationResources(
    const std::vector<std::uint8_t>& bundle, CarReplayAnimationRegistry* registry,EditorMarkerResources* modelRoot,
    bool ownModelCatalogue)
    : authored_(bundle)
{
    // Common track bundles contain both Flailer and animation chunks. Their
    // Flailer bundle already owns all eSolid/texture chunks, so animations
    // borrow that catalogue instead of publishing the same models twice.
    Require(ownModelCatalogue || modelRoot,"Borrowed animation models have no canonical catalogue");
    if (ownModelCatalogue)models_=std::make_unique<TrackRouteEditorResources>(bundle,modelRoot);
    modelView_=models_ ? &models_->Native() : modelRoot;
    const auto visit = [&](const auto& self, std::size_t begin,
            std::size_t size, unsigned depth) -> void {
        Require(depth <= 32, "Animation resource nesting exceeds 32 levels");
        for (const auto& chunk : ReadChunks(authored_, begin, size)) {
            if (chunk.id == 0x80034008u) {
                const auto children = ReadChunks(authored_, chunk.payloadOffset, chunk.size);
                Require(children.size() >= 3 && children[0].id == 0x34009u &&
                    children[1].id == 0x34010u && children[2].id == 0x34014u,
                    "Animation resource header or streams are missing");
                const auto& header = children[0];
                const std::size_t aligned = (header.payloadOffset + 15) & ~std::size_t(15);
                Require(aligned <= header.endOffset && header.endOffset - aligned >= 96,
                    "Animation header is truncated");
                const auto* bytes = authored_.data() + aligned;
                CarReplayAnimationClip clip{};
                clip.key = ReadU32(bytes + 64);
                clip.frameCount = ReadU16(bytes + 68);
                clip.boneCount = ReadU16(bytes + 70);
                const std::size_t rotationLanes =
                    std::size_t(clip.frameCount) * clip.boneCount * 4;
                const std::size_t positionLanes = std::size_t(clip.frameCount) * 4;
                Require(rotationLanes <= children[1].size / sizeof(std::int16_t) &&
                    positionLanes <= children[2].size / sizeof(float),
                    "Animation rotation or frame-position stream is truncated");
                rotations_.emplace_back(rotationLanes);
                positions_.emplace_back(positionLanes);
                auto& rotations = rotations_.back();
                auto& positions = positions_.back();
                for (std::size_t lane = 0; lane < rotationLanes; ++lane) {
                    const auto raw = ReadU16(authored_.data() +
                        children[1].payloadOffset + lane * 2);
                    std::memcpy(&rotations[lane], &raw, sizeof(raw));
                }
                for (std::size_t lane = 0; lane < positionLanes; ++lane) {
                    const auto raw = ReadU32(authored_.data() +
                        children[2].payloadOffset + lane * 4);
                    std::memcpy(&positions[lane], &raw, sizeof(raw));
                }
                clip.boneRotations = rotations.data();
                clip.framePositions = positions.data();
                clip.authoredRecord = authored_.data() + chunk.payloadOffset;
                clip.authoredRecordSize = chunk.size;
                for (int axis = 0; axis < 4; ++axis) {
                    const auto first = ReadU32(bytes + 32 + axis * 4);
                    const auto last = ReadU32(bytes + 48 + axis * 4);
                    std::memcpy(clip.minimum + axis, &first, sizeof(first));
                    std::memcpy(clip.maximum + axis, &last, sizeof(last));
                    Require(std::isfinite(clip.minimum[axis]) &&
                        std::isfinite(clip.maximum[axis]),
                        "Animation bounds contain a non-finite scalar");
                }
                clips_.push_back(clip);
            } else if (chunk.IsContainer()) {
                self(self, chunk.payloadOffset, chunk.size, depth + 1);
            }
        }
    };
    visit(visit, 0, authored_.size(), 0);
    catalogue_.clips = clips_.data();
    catalogue_.count = clips_.size();
    if (registry) Require(Register(*registry), "Animation catalogue is already registered");
}

CarReplayAnimationResources::~CarReplayAnimationResources()
{
    Unregister();
}

bool CarReplayAnimationResources::Register(CarReplayAnimationRegistry& registry) noexcept
{
    if (registry_ || !carReplayAnimationAppendCatalogue(&registry, &catalogue_))
        return false;
    registry_ = &registry;
    return true;
}

void CarReplayAnimationResources::Unregister() noexcept
{
    if (!registry_) return;
    CarReplayAnimationRegistry* registry = registry_;
    carReplayAnimationRemoveCatalogue(registry, &catalogue_);
    registry_ = nullptr;
    carReplayAnimationResourceEndFixup(registry);
}

}

void vectorQuaternionInterpolate(float output[4], float first[4], float second[4], float fraction)
{
    float dot = ((first[3]*second[3] + first[0]*second[0]) + first[1]*second[1]) + first[2]*second[2];
    if (dot < 0.0f) {
        for (int axis = 0; axis < 4; ++axis) first[axis] = -first[axis];
        dot = -dot;
    }
    float firstWeight, secondWeight;
    if (0.05000000074505806f < dot + 1.0f) {
        if (1.0f-dot < 0.05000000074505806f) {
            firstWeight = 1.0f-fraction;
            secondWeight = fraction;
        } else {
            const uint16_t angle = (uint16_t)(16384u - vectorArcSineAngle(dot));
            const float angleValue = (float)angle;
            const float denominator = rotationSineAngle(angle);
            const float firstCoordinate = angleValue * (1.0f-fraction);
            const float secondCoordinate = angleValue * fraction;
            firstWeight = finiteRatio(rotationSineAngle((uint16_t)convertFloatToInt32(firstCoordinate)), denominator);
            secondWeight = finiteRatio(rotationSineAngle((uint16_t)convertFloatToInt32(secondCoordinate)), denominator);
        }
    } else {
        second[0] = -first[1]; second[1] = first[0];
        second[2] = -first[3]; second[3] = first[2];
        const float firstCoordinate = ((0.5f-fraction)*3.141592741012573f)*10430.3779296875f;
        const float secondCoordinate = (fraction*3.141592741012573f)*10430.3779296875f;
        firstWeight = rotationSineAngle((uint16_t)convertFloatToInt32(firstCoordinate));
        secondWeight = rotationSineAngle((uint16_t)convertFloatToInt32(secondCoordinate));
    }
    for (int axis = 0; axis < 4; ++axis)
        output[axis] = firstWeight*first[axis] + secondWeight*second[axis];
}
