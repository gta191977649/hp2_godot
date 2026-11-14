#include "indep/inc/Replay.hpp"
#include "../inc/AnimatedDriver.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/AnimatedDriver.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Car.hpp"
#include "CameraMover.hpp"
#include "indep/inc/Timer.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"

#include <string.h>
#include "indep/inc/CarDriver.hpp"
#include "bware/inc/bWare.hpp"
#include <memory>
#include <cstring>
#include <algorithm>
#include <cmath>

// Released renderer controls330DC0/330DC4; they belong to this visual owner.
int32_t carDriverVisualForcedDetail=-1;
int32_t carDriverVisualForceAnimated=0;

namespace hp2 {
static void buildDriverQuaternionTable(CarDriverVisual& visual,CarReplayAnimationController& animation,
    float rotations[CAR_DRIVER_VISUAL_BONE_COUNT][4],float translation[3],bool interpolateSteering)
{
    // PAL1AA2F0..1AA6C0. Steering and gestures share the existing playback
    // queues/catalogue; rendering must not introduce another animation clock.
    Require(visual.bindings.clock && animation.catalogue,"Driver pose lacks its live clock or catalogue");
    if(visual.lookBack)carDriverVisualRefreshLookBack(&visual);
    if(visual.action3)carDriverVisualRefreshAction3(&visual);
    const auto time=visual.bindings.clock->worldTime;
    const CarDriverVisualPlayback* playing=nullptr;
    for(const auto& candidate:visual.playback)
        if(time>=candidate.startedTicks && time<candidate.endingTicks) {playing=&candidate;break;}
    visual.steering.duration=0.10000000149011612f;
    tCubic1DUpdate(&visual.steering,visual.bindings.clock->worldTimeElapsed,0.0f,0.0f);
    const auto steeringFrame=carDriverVisualSteeringFrame(&visual);
    const auto* steering=carReplayAnimationFind(animation.catalogue,0x2bb7f510u);
    Require(steering && steering->boneCount==CAR_DRIVER_VISUAL_BONE_COUNT && steering->framePositions &&
        steering->frameCount>0,"Driver steering clip lacks its authored 42-bone pose");
    const float phase=interpolateSteering ?
        std::clamp(visual.steering.value,0.0f,static_cast<float>(steering->frameCount-1)) :
        static_cast<float>(steeringFrame);
    const auto sampleFrame=static_cast<int32_t>(phase);
    Require(
        carDriverAnimationReadAllRotations(steering,rotations,CAR_DRIVER_VISUAL_BONE_COUNT,
            static_cast<float>(sampleFrame)),"Driver steering clip sample is outside its authored poses");
    // GetTranslation1A78E0 resets an out-of-range rounded sample to frame0.
    const auto translationFrame=sampleFrame>=0 && sampleFrame<steering->frameCount ? sampleFrame:0;
    std::memcpy(translation,steering->framePositions+static_cast<std::size_t>(translationFrame)*4,3*sizeof(float));
    const float blend=phase-static_cast<float>(sampleFrame);
    if(blend>0.0f) {
        float next[CAR_DRIVER_VISUAL_BONE_COUNT][4];
        Require(carDriverAnimationReadAllRotations(steering,next,CAR_DRIVER_VISUAL_BONE_COUNT,
            static_cast<float>(sampleFrame+1)),"Driver steering clip lacks its next authored pose");
        for(std::size_t bone=0;bone<CAR_DRIVER_VISUAL_BONE_COUNT;++bone)
            vectorQuaternionSlerp(rotations[bone],rotations[bone],next[bone],blend);
        const auto* nextTranslation=steering->framePositions+static_cast<std::size_t>(sampleFrame+1)*4;
        for(std::size_t axis=0;axis<3;++axis)
            translation[axis]+=(nextTranslation[axis]-translation[axis])*blend;
    }
    if(!playing) {
        if(visual.bindings.clock->worldLoopCounter%120==0)(void)carDriverVisualStartAnimation(&visual,2);
        return;
    }
    Require(playing->definition,"Driver playing slot lost its authored definition");
    const auto& definition=*playing->definition;
    std::int32_t sample{};
    Require(carDriverVisualPlaybackFrame(playing,time,&sample)!=0,"Driver gesture has no source frame");
    float weight=1.0f;
    if(definition.property) {
        const auto elapsed=sample-definition.firstFrame;
        const auto remaining=definition.firstFrame+definition.frameCount-sample;
        if(elapsed<5)weight=0.20000000298023224f*static_cast<float>(elapsed+1);
        else if(remaining<5)weight=0.20000000298023224f*static_cast<float>(remaining);
    }
    const auto* clip=carReplayAnimationFind(animation.catalogue,
        definition.kind==11 ? visual.shiftAnimationKey:definition.key);
    if(definition.kind==11) {
        if(!clip)return;
        // Signed-byte table330DE0: the three gear-change arm bones only.
        float other[3][4];
        Require(carDriverAnimationReadRotationRange(clip,other,3,30,3,static_cast<float>(sample))!=0,
            "Driver shift pose exceeds its authored animation");
        for(std::size_t index=0;index<3;++index)
            vectorQuaternionSlerp(rotations[30+index],rotations[30+index],other[index],weight);
        return;
    }
    if(weight==0.0f)return;
    if(definition.rebaseRotations) {
        Require(clip,"Additive driver gesture has no preprocessed clip");
        float other[CAR_DRIVER_VISUAL_BONE_COUNT][4];
        Require(carDriverAnimationReadRotationRange(clip,other,CAR_DRIVER_VISUAL_BONE_COUNT,
            0,CAR_DRIVER_VISUAL_BONE_COUNT,static_cast<float>(sample))!=0,
            "Additive driver gesture exceeds its authored animation");
        for(std::int32_t bone=0;bone<CAR_DRIVER_VISUAL_BONE_COUNT;++bone) {
            vectorQuaternionMultiply(rotations[bone],rotations[bone],other[bone]);
        }
    } else if(clip && weight==1.0f) {
        Require(carDriverAnimationReadAllRotations(clip,rotations,CAR_DRIVER_VISUAL_BONE_COUNT,
            static_cast<float>(sample))!=0,"Driver gesture exceeds its authored quaternion table");
    } else if(clip) {
        float other[CAR_DRIVER_VISUAL_BONE_COUNT][4];
        Require(carDriverAnimationReadRotationRange(clip,other,CAR_DRIVER_VISUAL_BONE_COUNT,
            0,CAR_DRIVER_VISUAL_BONE_COUNT,static_cast<float>(sample))!=0,
            "Blended driver gesture exceeds its authored animation");
        for(std::int32_t bone=0;bone<CAR_DRIVER_VISUAL_BONE_COUNT;++bone) {
            vectorQuaternionSlerp(rotations[bone],rotations[bone],other[bone],weight);
        }
    }
}

void BuildDriverQuaternionTable(CarDriverVisual& visual,CarReplayAnimationController& animation,
    float rotations[CAR_DRIVER_VISUAL_BONE_COUNT][4],float translation[3])
{
    buildDriverQuaternionTable(visual,animation,rotations,translation,false);
}

void SampleDriverPresentationPose(CarDriverVisual& visual,CarReplayAnimationController& animation,
    float rotations[CAR_DRIVER_VISUAL_BONE_COUNT][4],float translation[3])
{
    buildDriverQuaternionTable(visual,animation,rotations,translation,true);
}

std::size_t RenderDriverVisual(CarDriverVisual& visual,const float localToWorld[16],
    const SceneLightContext* lightContext,const CarCinematicScreen& view,EcstasyFrame& frame,
    std::uint32_t flags,std::int32_t detail)
{
    // PAL1A9A30..1A9B30 retains the actual animated/static branch per slot.
    if(carDriverVisualForcedDetail>=0)detail=carDriverVisualForcedDetail;
    if(carDriverVisualForceAnimated && detail>=2)detail=1;
    Require(detail>=0 && detail<3,"Driver detail exceeds its three authored model slots");
    if(auto* animation=visual.animations[detail]) {
        float rotations[CAR_DRIVER_VISUAL_BONE_COUNT][4],translation[3];
        BuildDriverQuaternionTable(visual,*animation,rotations,translation);
        return RenderDriverAnimationPose(visual.poseCache,*animation,localToWorld,translation,rotations,
            CAR_DRIVER_VISUAL_BONE_COUNT,lightContext,view,frame,flags);
    }
    if(auto* model=visual.staticModels[detail])
        return frame.RenderModel(*model,localToWorld,view,flags,1.0f,
            visual.bindings.clock->worldTimeFrames,lightContext);
    return 0;
}
}

void carDriverVisualSetupSteeringFrame(CarDriverVisual* visual)
{
    // PAL1A9460. Native storage embeds the original scalar curve instead of
    // allocating its48-byte console object. No secondary steering state.
    auto& curve=visual->steering;
    curve.desiredDerivative=0;
    curve.threshold=1.0f;
    curve.continuous=1;
    curve.duration=0.10000000149011612f;
    curve.value=30.0f;
    curve.derivative=curve.desiredDerivative;
    curve.mode=0;
    curve.desiredValue=30.0f;
}

void carDriverVisualInitializeArtwork(CarDriverVisual* visual)
{
    // PAL1A97E8: all three actual model slots, including skeletal and static
    // branches, share the original two replacement cells and detail indices.
    hp2::Require(visual && visual->resources && visual->appearances,
        "AnimatedDriver has no live resource owners");
    auto& resources=visual->resources->Models();
    for (std::size_t index=0;index<3;++index) {
        if (visual->animations[index])editorMarkerModelDispose(&visual->animatedModels[index]);
        if (visual->staticModels[index])editorMarkerModelDispose(visual->staticModels[index]);
    }
    const auto* character=visual->bindings.character;
    if (!character)return;
    const auto* appearance=carDriverAppearanceFind(visual->appearances,character->appearanceName);
    if (!appearance)return;
    for (std::size_t index=0;index<2;++index) {
        hp2::Require(character->variants[index]>=0 && character->variants[index]<16,
            "Driver character texture variant is outside authored table");
        auto& replacement=visual->replacements[index];
        replacement.originalKey=appearance->materialKeys[index];
        replacement.replacementKey=appearance->textureKeys[index][character->variants[index]];
    }
    int32_t detail=0;
    for (std::size_t index=0;index<3;++index) {
        const auto key=appearance->modelKeys[index];
        if (!key)continue;
        EditorMarkerModel probe{};
        editorMarkerModelInitialize(&probe,&resources,key);
        try {
            if (probe.solid) {
                const auto* tree=editorMarkerModelAnimationTree(&probe);
                if (tree && tree->nodeCount>0) {
                    auto& model=visual->animatedModels[index];
                    if (!visual->animations[index]) {
                        auto controller=std::make_unique<CarReplayAnimationController>();
                        hp2::Require(carReplayAnimationInitialize(&visual->resources->Animations(),controller.get(),&model)!=0,
                            "Driver animation cannot join its actual instance list");
                        visual->animations[index]=controller.release();
                    }
                    editorMarkerModelInitialize(&model,&resources,key);
                    editorMarkerModelAttachReplacementTextures(&model,visual->replacements,2,detail++);
                } else {
                    if (!visual->staticModels[index])visual->staticModels[index]=new EditorMarkerModel{};
                    editorMarkerModelInitialize(visual->staticModels[index],&resources,key);
                    editorMarkerModelAttachReplacementTextures(visual->staticModels[index],visual->replacements,2,detail++);
                }
            }
        } catch (...) {editorMarkerModelDispose(&probe);throw;}
        editorMarkerModelDispose(&probe);
    }
    visual->shiftAnimationKey=hashString("DRIVER_SHIFT01");
}

CarDriverVisual* carDriverVisualCreate(const CarDriverVisualBindings* bindings,
    NativeResourceRegistry* resources,const CarDriverAppearanceCatalogue* appearances)
{
    // Complete PAL1A95C0 owner construction. Allocation failure propagates;
    // this function never substitutes an appearance-only success object.
    hp2::Require(bindings && bindings->clock && bindings->definitions && resources && appearances,
        "AnimatedDriver construction lacks its actual owners");
    auto visual=std::make_unique<CarDriverVisual>();
    visual->bindings=*bindings;visual->resources=resources;visual->appearances=appearances;
    for (auto& playback:visual->playback)
        carDriverVisualPlaybackAssign(&playback,bindings->definitions,0,0,bindings->clock->worldTime);
    try {
        carDriverVisualInitializeArtwork(visual.get());
        carDriverVisualSetupSteeringFrame(visual.get());
    } catch (...) {carDriverVisualDestroy(visual.release());throw;}
    return visual.release();
}

void carDriverVisualDestroy(CarDriverVisual* visual)
{
    // PAL1A96B8. The original allocation flags become ordinary native owner
    // destruction; every live controller and model is unlinked first.
    if (!visual)return;
    for (std::size_t index=0;index<3;++index) {
        if (visual->animations[index]) {
            editorMarkerModelDispose(&visual->animatedModels[index]);
            (void)carReplayAnimationUnregisterController(visual->animations[index]);
            delete visual->animations[index];
        }
        if (visual->staticModels[index]) {
            editorMarkerModelDispose(visual->staticModels[index]);
            delete visual->staticModels[index];
        }
    }
    delete visual;
}

typedef struct DriverAnimationSource {
    const char* name;
    int32_t firstFrame;
    int32_t frameCount;
    int32_t holdFrame;
    int32_t priority;
    int32_t property;
    int32_t rebaseRotations;
} DriverAnimationSource;

/* The14 authored definitions are the exact final-PAL table at3111F8. Mutable
 * keys, counts and preprocessing state belong to the supplied resource owner. */
static const DriverAnimationSource animationSources[CAR_DRIVER_VISUAL_ANIMATION_COUNT] = {
    {"", 0, 0, 0, 0, 0, 0},
    {"DRIVER_ANIM01", 15, 30, 0, 0, 0, 0},
    {"DRIVER_ANIM01", 231, 30, 0, 0, 0, 1},
    {"DRIVER_ANIM01", 61, 30, 75, 1, 1, 0},
    {"DRIVER_ANIM01", 111, 20, 121, 1, 1, 1},
    {"DRIVER_ANIM01", 91, 20, 101, 1, 1, 1},
    {"DRIVER_ANIM01", 211, 20, 0, 2, 1, 0},
    {"DRIVER_ANIM01", 131, 20, 0, 3, 1, 0},
    {"DRIVER_ANIM01", 151, 20, 0, 3, 1, 0},
    {"DRIVER_ANIM01", 171, 20, 0, 3, 1, 0},
    {"DRIVER_ANIM01", 191, 20, 0, 3, 1, 0},
    {"DRIVER_SHIFT01", 1, 30, 0, 1, 1, 0},
    {"DRIVER_PRERACEG02", 0, 0, 0, 0, 0, 0},
    {"DRIVER_PRERACEG14", 0, 0, 0, 0, 0, 0}
};

static int32_t signedBits(uint32_t value)
{
    int32_t result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static int32_t now(const CarDriverVisual* visual)
{
    return visual->bindings.clock->worldTime;
}

void carDriverVisualAnimationDefinitionsInitialize(
    CarDriverVisualAnimationDefinitions* definitions)
{
    memset(definitions, 0, sizeof(*definitions));
    for (int32_t index = 0; index < CAR_DRIVER_VISUAL_ANIMATION_COUNT; ++index) {
        const DriverAnimationSource* source = &animationSources[index];
        CarDriverVisualAnimationDefinition* definition = &definitions->animations[index];
        definition->kind = index;
        definition->firstFrame = source->firstFrame;
        definition->frameCount = source->frameCount;
        definition->holdFrame = source->holdFrame;
        definition->priority = source->priority;
        definition->property = source->property;
        definition->rebaseRotations = source->rebaseRotations;
    }
    /* The actual startup store168E38 enables the one-shot pass before the
     * INGAMEB catalogue is prepared. It is not inferred from ELF zero data. */
    definitions->rotationPreprocessPending = 1;
}

int32_t carDriverVisualPrepareAnimations(CarDriverVisualAnimationDefinitions* definitions,
    const CarReplayAnimationCatalogue* catalogue)
{
    if (!definitions || !catalogue) return 0;
    for (int32_t index = 1; index < CAR_DRIVER_VISUAL_ANIMATION_COUNT; ++index) {
        CarDriverVisualAnimationDefinition* definition = &definitions->animations[index];
        definition->key = hashString(animationSources[index].name);
        definition->clip = carReplayAnimationFind(catalogue, definition->key);
        const CarReplayAnimationClip* clip = definition->clip;
        if (!clip) {
            definition->frameCount = 0;
            continue;
        }
        if (!definition->firstFrame) definition->frameCount = clip->frameCount;
        if (!definitions->rotationPreprocessPending || !definition->rebaseRotations) continue;
        if (!clip->boneRotations || clip->boneCount < CAR_DRIVER_VISUAL_BONE_COUNT ||
            clip->frameCount <= 30 || definition->firstFrame < 0 || definition->frameCount < 0 ||
            definition->firstFrame > clip->frameCount ||
            definition->frameCount > clip->frameCount - definition->firstFrame) {
            return 0;
        }
        for (int32_t frame = definition->firstFrame;
             frame < definition->firstFrame + definition->frameCount; ++frame) {
            for (int32_t bone = 0; bone < CAR_DRIVER_VISUAL_BONE_COUNT; ++bone) {
                float reference[4], current[4], inverse[4], relative[4];
                if (!carDriverAnimationReadRotation(clip, reference, bone, 30.0f) ||
                    !carDriverAnimationReadRotation(clip, current, bone, (float)frame)) {
                    return 0;
                }
                vectorQuaternionConjugate(inverse, reference);
                vectorQuaternionMultiply(relative, inverse, current);
                vectorQuaternionNormalize(relative, relative);
                if (!carDriverAnimationWriteRotation(clip, relative, bone, (float)frame)) return 0;
            }
        }
    }
    definitions->rotationPreprocessPending = 0;
    return 1;
}

void carDriverVisualPlaybackAssign(CarDriverVisualPlayback* playback,
    const CarDriverVisualAnimationDefinitions* definitions, int32_t kind,
    int32_t delay, int32_t now)
{
    if ((uint32_t)kind >= CAR_DRIVER_VISUAL_ANIMATION_COUNT) return;
    const CarDriverVisualAnimationDefinition* definition = definitions->animations + kind;
    playback->definition = definition;
    playback->startedTicks = signedBits((uint32_t)now + (uint32_t)delay);
    playback->endingTicks = signedBits((uint32_t)playback->startedTicks +
                                     (uint32_t)definition->frameCount * 2u);
    playback->holdFrame = definition->holdFrame;
    if (playback->holdFrame) {
        playback->endingTicks = signedBits((uint32_t)playback->startedTicks + 100000u);
    }
}

int32_t carDriverVisualPlaybackFrame(const CarDriverVisualPlayback* playback,
    int32_t now, int32_t* frame)
{
    const CarDriverVisualAnimationDefinition* definition = playback->definition;
    if (!definition || !frame) return 0;
    const int32_t elapsed = signedBits((uint32_t)now - (uint32_t)playback->startedTicks);
    int32_t value = signedBits((uint32_t)definition->firstFrame + (uint32_t)(elapsed / 2));
    if (playback->holdFrame == -1) {
        if (definition->frameCount <= 0) return 0;
        while (value >= definition->frameCount) {
            value = signedBits((uint32_t)value - (uint32_t)definition->frameCount);
        }
    } else if (playback->holdFrame && playback->holdFrame < value) {
        value = playback->holdFrame;
    }
    *frame = value;
    return 1;
}

int32_t carDriverVisualPlaybackFinish(CarDriverVisualPlayback* playback, int32_t now)
{
    int32_t frame;
    if (!carDriverVisualPlaybackFrame(playback, now, &frame)) return 0;
    const CarDriverVisualAnimationDefinition* definition = playback->definition;
    const uint32_t remaining = (uint32_t)definition->firstFrame +
                               (uint32_t)definition->frameCount - (uint32_t)frame;
    playback->endingTicks = signedBits((uint32_t)now + remaining * 2u);
    playback->holdFrame = 0;
    playback->startedTicks = signedBits((uint32_t)playback->endingTicks -
                                      (uint32_t)definition->frameCount * 2u);
    return 1;
}

void carDriverVisualPlaybackCancel(CarDriverVisualPlayback* playback,
    const CarDriverVisualAnimationDefinitions* definitions, int32_t now)
{
    carDriverVisualPlaybackAssign(playback, definitions, 0, 0, now);
}

int32_t carDriverVisualPlaybackRelease(CarDriverVisualPlayback* playback,
    const CarDriverVisualAnimationDefinitions* definitions, int32_t now)
{
    const int active = now < playback->endingTicks && now >= playback->startedTicks;
    if (!active) {
        carDriverVisualPlaybackCancel(playback, definitions, now);
        return 1;
    }
    const int32_t remaining = signedBits((uint32_t)playback->endingTicks - (uint32_t)now);
    if (remaining > 0 && playback->holdFrame) return carDriverVisualPlaybackFinish(playback, now);
    return 1;
}

CarDriverVisualPlayback* carDriverVisualFindPlaying(CarDriverVisual* visual, int32_t kind)
{
    for (int index = 0; index < 2; ++index) {
        CarDriverVisualPlayback* playback = visual->playback + index;
        if (playback->definition->kind == kind &&
            signedBits((uint32_t)playback->endingTicks - (uint32_t)now(visual)) > 0) {
            return playback;
        }
    }
    return NULL;
}

// Complete PAL1A9770..1A97E4. Restore first cancels active playback through
// the actual AnimatedDriver owner; the two retained signed bytes then move.
void carDriverVisualSnapshot(CarDriverVisual* visual,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"AnimatedDriver");
    if(snapshot->restoring)carDriverVisualClearAnimations(visual);
    replaySnapshotMoveData(snapshot,&visual->lookBack,1,1,1);
    replaySnapshotMoveData(snapshot,&visual->action3,1,1,1);
    replaySnapshotEndSection(snapshot);
}

void carDriverVisualClearAnimations(CarDriverVisual* visual)
{
    for (int index = 0; index < 2; ++index) {
        carDriverVisualPlaybackAssign(visual->playback + index,
            visual->bindings.definitions, 0, 0, now(visual));
    }
}

void carDriverVisualReleaseAnimations(CarDriverVisual* visual)
{
    for (int index = 0; index < 2; ++index) {
        if (!carDriverVisualPlaybackRelease(visual->playback + index,
                visual->bindings.definitions, now(visual))) {
            visual->result = CAR_DRIVER_VISUAL_INVALID_ANIMATION;
        }
    }
}

void carDriverVisualReleaseAnimation(CarDriverVisual* visual, int32_t kind)
{
    for (int index = 0; index < 2; ++index) {
        CarDriverVisualPlayback* playback = visual->playback + index;
        if (playback->definition->kind == kind &&
            !carDriverVisualPlaybackRelease(playback, visual->bindings.definitions, now(visual))) {
            visual->result = CAR_DRIVER_VISUAL_INVALID_ANIMATION;
        }
    }
}

int32_t carDriverVisualMaximumRemaining(const CarDriverVisual* visual)
{
    int32_t maximum = 0;
    for (int index = 0; index < 2; ++index) {
        const int32_t remaining = signedBits((uint32_t)visual->playback[index].endingTicks -
                                            (uint32_t)now(visual));
        if (maximum < remaining) maximum = remaining;
    }
    return maximum;
}

typedef struct AnimationBlock {
    int32_t kind;
    int32_t model;
} AnimationBlock;

static const AnimationBlock modelBlocks[21] = {
    {8, 23}, {12, 23}, {8, 24}, {12, 24}, {8, 47}, {8, 48}, {8, 2},
    {8, 3}, {8, 21}, {11, 13}, {11, 14}, {11, 15}, {11, 16},
    {3, 13}, {3, 14}, {3, 15}, {3, 16}, {11, 7}, {11, 8}, {11, 9}, {11, 10}
};

static const AnimationBlock copBlocks[8] = {
    {8, 32}, {7, 32}, {8, 4}, {7, 4}, {10, 4}, {8, 33}, {7, 33}, {10, 33}
};

static const float steeringWiggle[62] = {
    0.0f, 0.100000001f, 0.200000003f, 0.300000012f,
    0.5f, 0.699999988f, 0.899999976f, 1.10000002f,
    1.20000005f, 1.29999995f, 1.35000002f, 1.37f,
    1.29999995f, 1.25999999f, 1.10000002f, 1.0f,
    0.800000012f, 0.699999988f, 0.699999988f, 0.75f,
    0.800000012f, 0.899999976f, 0.800000012f, 0.600000024f,
    0.5f, 0.300000012f, 0.100000001f, -0.100000001f,
    -0.200000003f, -0.300000012f, -0.400000006f, -0.600000024f,
    -0.699999988f, -0.75f, -0.800000012f, -0.850000024f,
    -0.839999974f, -0.699999988f, -0.600000024f, -0.600000024f,
    -0.5f, -0.400000006f, -0.200000003f, -0.100000001f,
    0.100000001f, -0.100000001f, -0.200000003f, -0.300000012f,
    -0.400000006f, -0.600000024f, -0.699999988f, -0.75f,
    -0.800000012f, -0.850000024f, -0.839999974f, -0.699999988f,
    -0.600000024f, -0.600000024f, -0.5f, -0.400000006f,
    -0.200000003f, -0.100000001f
};

int32_t carDriverVisualAnimationBlocked(int32_t kind, int32_t modelCode,
    int8_t driverAnimationCode, const CarCinematicServices* cinematics,
    const CarDriverCharacter* character)
{
    for (int index = 0; index < 21; ++index) {
        if (modelBlocks[index].kind == kind && modelBlocks[index].model == modelCode) return 1;
    }
    if (carCinematicCurrentViewMode(cinematics) != 1 && !driverAnimationCode &&
        (kind == 11 || kind == 2)) {
        return 1;
    }
    if (character && (uint8_t)character->appearanceName[0] == 'C' &&
        (uint8_t)character->appearanceName[1] == 'o') {
        for (int index = 0; index < 8; ++index) {
            if (copBlocks[index].kind == kind && copBlocks[index].model == modelCode) return 1;
        }
    }
    return 0;
}

static int blocked(CarDriverVisual* visual, int32_t kind)
{
    if (!visual->bindings.vehicle || !visual->bindings.cinematics) {
        visual->result = CAR_DRIVER_VISUAL_INVALID_BINDING;
        return 1;
    }
    return carDriverVisualAnimationBlocked(kind, visual->bindings.vehicle->modelCode,
        visual->bindings.vehicle->driverAnimationCode, visual->bindings.cinematics,
        visual->bindings.character);
}

int32_t carDriverVisualStartAnimation(CarDriverVisual* visual, int32_t kind)
{
    if ((uint32_t)kind >= CAR_DRIVER_VISUAL_ANIMATION_COUNT) {
        visual->result = CAR_DRIVER_VISUAL_INVALID_ANIMATION;
        return 0;
    }
    if (visual->bindings.movement && blocked(visual, kind)) return 0;
    int32_t available = -1;
    int32_t queuedDelay = 0;
    for (int index = 0; index < 2; ++index) {
        const CarDriverVisualPlayback* playback = visual->playback + index;
        const int32_t remaining = signedBits((uint32_t)playback->endingTicks - (uint32_t)now(visual));
        if (remaining <= 0) available = index;
        else if (remaining >= 10001) return 0;
        else if (queuedDelay < remaining) queuedDelay = remaining;
    }
    if (available < 0) return 0;
    carDriverVisualPlaybackAssign(visual->playback + available,
        visual->bindings.definitions, kind, queuedDelay, now(visual));
    return 1;
}

void carDriverVisualRequestAnimation(CarDriverVisual* visual, int32_t kind, int32_t priority)
{
    for (int index = 0; index < 2; ++index) {
        const CarDriverVisualPlayback* playback = visual->playback + index;
        if (now(visual) >= playback->endingTicks || now(visual) < playback->startedTicks) continue;
        const int32_t existing = playback->definition->priority;
        if (!existing || existing >= priority) return;
    }
    carDriverVisualClearAnimations(visual);
    if (!visual->bindings.movement) {
        visual->result = CAR_DRIVER_VISUAL_INVALID_BINDING;
        return;
    }
    const int disallowed = blocked(visual, kind);
    if (visual->result != CAR_DRIVER_VISUAL_OK) return;
    (void)carDriverVisualStartAnimation(visual, disallowed ? 6 : kind);
}

void carDriverVisualBeginGearChange(CarDriverVisual* visual)
{
    (void)carDriverVisualStartAnimation(visual, 11);
}

// PAL1AA1E8..1AA2EC. A rejected repeat or unavailable/blocked clip consumes
// its actual RNG draw. The source caps attempts at1001 and counts a permitted
// request even when the actual two-slot playback queue cannot accept it.
void carDriverVisualQueueRandomAnimations(CarDriverVisual* visual,int32_t first,
    int32_t last,int32_t count)
{
    if (!visual || !visual->bindings.definitions || !visual->bindings.clock) {
        if (visual) visual->result=CAR_DRIVER_VISUAL_INVALID_BINDING;
        return;
    }
    const int64_t width=static_cast<int64_t>(last)-first+1;
    if (width<=0 || count<=0)return;
    if (first<0 || last>=CAR_DRIVER_VISUAL_ANIMATION_COUNT) {
        visual->result=CAR_DRIVER_VISUAL_INVALID_ANIMATION;
        return;
    }
    int32_t previous=-1;
    int32_t requested=0;
    for (int32_t attempt=1;attempt<=1001 && requested<count;++attempt) {
        const int32_t selected=first+static_cast<int32_t>(
            vectorDefaultRandomBounded(static_cast<uint32_t>(width)));
        if (selected==previous)continue;
        // Source311208 is definition+16: the authored/prepared frame count,
        // not the separate animation pointer at definition+4.
        if (!visual->bindings.definitions->animations[selected].frameCount)continue;
        if (visual->bindings.movement && blocked(visual,selected))continue;
        (void)carDriverVisualStartAnimation(visual,selected);
        previous=selected;
        ++requested;
    }
}

void carDriverVisualRefreshAction3(CarDriverVisual* visual)
{
    if (!carDriverVisualFindPlaying(visual, 3)) (void)carDriverVisualStartAnimation(visual, 3);
}

void carDriverVisualBeginAction3(CarDriverVisual* visual)
{
    if (!visual->action3) {
        visual->action3 = 1;
        carDriverVisualRefreshAction3(visual);
    }
}

void carDriverVisualEndAction3(CarDriverVisual* visual)
{
    if (visual->action3) {
        visual->action3 = 0;
        carDriverVisualReleaseAnimation(visual, 3);
    }
}

void carDriverVisualRefreshLookBack(CarDriverVisual* visual)
{
    int32_t selected = visual->lookBackAnimation;
    CarMovement* movement = visual->bindings.movement;
    if (movement) {
        int32_t isPursuer;
        CarMovement* related = carChaseCameraRelatedVehicle(visual->bindings.pursuit, movement, &isPursuer);
        if (related) {
            const uint16_t quadrant = carChaseCameraTargetQuadrant(movement, related,
                movement->bindings.state->orientation);
            if (quadrant == 0) selected = 0;
            else if (quadrant == 32768 || quadrant == 16384) selected = 5;
            else if (quadrant == 49152) selected = 4;
        } else {
            const uint32_t angle = movement->bindings.controls->steeringAngle;
            if (1820u < angle && angle < 32768u) selected = 4;
            else if (32768u < angle && angle < 63715u) selected = 5;
        }
    }
    if (selected != 4) carDriverVisualReleaseAnimation(visual, 4);
    if (selected != 5) carDriverVisualReleaseAnimation(visual, 5);
    if (selected && !carDriverVisualFindPlaying(visual, selected)) {
        (void)carDriverVisualStartAnimation(visual, selected);
    }
    visual->lookBackAnimation = selected;
}

void carDriverVisualBeginLookBack(CarDriverVisual* visual)
{
    if (!visual->lookBack) {
        visual->lookBack = 1;
        visual->lookBackAnimation = 5;
        carDriverVisualRefreshLookBack(visual);
    }
}

void carDriverVisualEndLookBack(CarDriverVisual* visual)
{
    if (visual->lookBack) {
        visual->lookBack = 0;
        carDriverVisualReleaseAnimation(visual, 5);
        carDriverVisualReleaseAnimation(visual, 4);
    }
}

void carDriverVisualSetLookBack(CarDriverVisual* visual, int32_t enabled)
{
    if (enabled) carDriverVisualBeginLookBack(visual);
    else carDriverVisualEndLookBack(visual);
}

int32_t carDriverVisualSteeringFrame(CarDriverVisual* visual)
{
    float fraction = 0.0f;
    if (visual->bindings.movement && visual->bindings.movement->bindings.controls) {
        const uint16_t angle = visual->bindings.movement->bindings.controls->steeringAngle;
        fraction = (float)angle * 0.0000152587890625f;
    }

    float desired;
    if (fraction < 0.5f) desired = fraction * 150.0f + 30.0f;
    else desired = 30.0f - (1.0f - fraction) * 150.0f;

    const uint32_t elapsed = (uint32_t)now(visual) - (uint32_t)visual->steeringStartedTicks;
    desired = desired + steeringWiggle[elapsed % 62u];
    if (45.0f < desired) desired = 45.0f;
    if (desired < 15.0f) desired = 15.0f;
    visual->steering.desiredValue = desired;
    if (desired != visual->steering.value) visual->steering.mode = 2;
    return convertFloatToInt32(visual->steering.value);
}

void carDriverVisualInitializeState(CarDriverVisual* visual,const CarDriverVisualBindings* bindings)
{
    *visual=CarDriverVisual{};visual->bindings=*bindings;
    if(!bindings->clock||!bindings->definitions){visual->result=CAR_DRIVER_VISUAL_INVALID_BINDING;return;}
    for(int index=0;index<2;++index)
        carDriverVisualPlaybackAssign(visual->playback+index,bindings->definitions,0,0,now(visual));
    tCubic1DInitialize(&visual->steering);
    visual->steering.value=visual->steering.desiredValue=30.0f;
    visual->steering.duration=0.10000000149011612f;
}

CarDriverVisualResult carDriverVisualResult(const CarDriverVisual* visual)
{ return visual->result; }
