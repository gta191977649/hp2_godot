#include "bware/inc/bMath.hpp"
#include "indep/inc/Track.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Skids.hpp"
#include "indep/inc/Clans.hpp"
#include "indep/inc/Car.hpp"
#include <cassert>

#include <math.h>
#include <string.h>

void wheelTrailPackPoint(WheelTrailPoint* point, const float position[3], const float width[3])
{
    for (int axis = 0; axis < 3; ++axis) {
        const uint8_t encoded = (uint8_t)(uint32_t)convertFloatToInt32(width[axis] * 64.0f);
        point->position[axis] = position[axis];
        memcpy(point->width + axis, &encoded, sizeof(encoded));
    }
}

void wheelTrailUnpackPoint(const WheelTrailPoint* point, float position[3], float width[3])
{
    for (int axis = 0; axis < 3; ++axis) {
        position[axis] = point->position[axis];
        if (width != NULL) width[axis] = (float)point->width[axis] * 0.015625f;
    }
}

void wheelTrailEndpoints(const WheelTrailPoint* point, float first[3], float second[3])
{
    for (int axis = 0; axis < 3; ++axis) {
        const float width = (float)point->width[axis] * 0.015625f;
        first[axis] = point->position[axis] + width;
        second[axis] = point->position[axis] - width;
    }
}

// Individually proved complete SkidSet/SkidMaker implementations.
static float dotThree(const float a[3], const float b[3])
{
    const float x = a[0] * b[0], y = a[1] * b[1], z = a[2] * b[2];
    return (x + y) + z;
}

static void changed(WheelTrailStore* store, WheelTrail* trail, WheelTrailChange change)
{
    trail->revision = ++store->revision;
    trail->change = change;
}

void wheelTrailStoreInitialize(WheelTrailStore* store, WheelTrail* storage, size_t capacity)
{
    store->storage = storage;
    store->capacity = capacity;
    store->revision = 0;
    store->clans = nullptr;
    memset(storage, 0, capacity * sizeof(*storage));
}

void wheelTrailStop(WheelTrailStore* store, WheelTrail* trail)
{
    if (trail->attachment != NULL) {
        *trail->attachment = NULL;
        trail->attachment = NULL;
        changed(store, trail, WHEEL_TRAIL_STOPPED);
    }
}

void wheelTrailDelete(WheelTrailStore* store, WheelTrail* trail)
{
    // DeleteThisSkid PAL1AB228 releases the same global and Clan membership,
    // and SkidSet dtor clears the wheel's attachment before freeing its slot.
    wheelTrailStop(store,trail);
    if(trail->clan)wheelTrailClanDetach(store,trail);
    memset(trail,0,sizeof(*trail));
}

void wheelTrailDetach(WheelTrailStore* store, WheelTrail** attachment)
{
    if (*attachment != NULL) wheelTrailStop(store, *attachment);
}

int wheelTrailAppend(WheelTrailStore* store, WheelTrail* trail, const float position[4],
                     const float width[3], float intensity)
{
    float direction[3] = {0.0f, 0.0f, 0.0f};
    float distance = 0.0f;
    const size_t count = trail->pointCount;
    if (count > 0) {
        for (int axis = 0; axis < 3; ++axis)
            direction[axis] = position[axis] - trail->points[count - 1].position[axis];
        distance = sqrtf(dotThree(direction, direction));
        const float scale = finiteRatio(1.0f, distance);
        for (int axis = 0; axis < 3; ++axis) direction[axis] = direction[axis] * scale;
    }
    int overwrite = 0;
    if (count >= 2) {
        const float turning = 1.0f - dotThree(direction, trail->previousDirection);
        const float total = trail->distance + distance;
        if (0.20000000298023224f < turning) {
            wheelTrailStop(store, trail);
            return 0;
        }
        if (total < 0.25f || turning < 0.0020000000949949026f) overwrite = 1;
        if (3.0f < total) overwrite = 0;
    }
    if (!overwrite && count == 8) return 1;
    WheelTrailPoint* point;
    if (overwrite) {
        point = trail->points + count - 1;
        trail->distance = trail->distance + distance;
    } else {
        point = trail->points + trail->pointCount++;
        if (trail->pointCount >= 2)
            memcpy(trail->previousDirection, direction, sizeof(direction));
        trail->distance = distance;
    }
    wheelTrailPackPoint(point, position, width);
    point->opacity = (uint8_t)(uint32_t)convertFloatToInt32(intensity * 255.0f);
    for (int axis = 0; axis < 3; ++axis) {
        const float low = position[axis] - distance, high = position[axis] + distance;
        if (low < trail->minimum[axis]) trail->minimum[axis] = low;
        if (trail->maximum[axis] < high) trail->maximum[axis] = high;
        trail->center[axis] = (trail->minimum[axis] + trail->maximum[axis]) * 0.5f;
    }
    if(trail->clan)wheelTrailClanExpand(store,trail);
    changed(store, trail, WHEEL_TRAIL_UPDATED);
    return 0;
}

static WheelTrail* createTrail(WheelTrailStore* store, WheelTrail** attachment,
    const void* owner, uint32_t flags, const float position[4], const float width[3],
    int32_t materialIndex, float intensity)
{
    // Source global SkidList eviction uses its least recently rendered tail.
    // The monotonic order also preserves insertion order before first render.
    WheelTrail* trail = store->storage;
    for (size_t i = 0; i < store->capacity; ++i) {
        WheelTrail* candidate = store->storage + i;
        if (candidate->creation == 0) { trail = candidate; break; }
        if (candidate->creation < trail->creation) trail = candidate;
    }
    wheelTrailDelete(store,trail);
    trail->attachment = attachment;
    trail->owner = owner;
    trail->variant = flags & 2u ? (int32_t)((flags >> 2) & 1u) : 0;
    trail->materialIndex = materialIndex;
    for (int axis = 0; axis < 3; ++axis) {
        trail->minimum[axis] = trail->maximum[axis] = position[axis];
        trail->center[axis] = position[axis];
        trail->origin[axis] = position[axis];
    }
    if(store->clans)wheelTrailClanAttach(store,trail,position);
    trail->creation = ++store->revision;
    wheelTrailAppend(store, trail, position, width, intensity);
    changed(store, trail, WHEEL_TRAIL_STARTED);
    return trail;
}

void wheelTrailUpdate(WheelTrailStore* store, WheelTrail** attachment, const void* owner,
    const float ownerCenter[3], uint32_t flags, const float position[4],
    const float width[3], int32_t materialIndex, float intensity)
{
    if (owner != NULL) {
        float difference[3];
        for (int axis = 0; axis < 3; ++axis) difference[axis] = ownerCenter[axis] - position[axis];
        if (4.0f < sqrtf(dotThree(difference, difference))) return;
    }
    WheelTrail* trail = *attachment;
    if (trail == NULL) {
        *attachment = createTrail(store, attachment, owner, flags, position, width, materialIndex, intensity);
        return;
    }
    const int32_t variant = flags & 2u ? (int32_t)((flags >> 2) & 1u) : 0;
    if (trail->variant == variant && trail->materialIndex == materialIndex &&
        wheelTrailAppend(store, trail, position, width, intensity) == 0) return;
    float lastPosition[4] = {0.0f, 0.0f, 0.0f, 0.0f}, lastWidth[3];
    const WheelTrailPoint* last = trail->points + trail->pointCount - 1;
    const float lastIntensity = (float)last->opacity * 0.003921568859368563f;
    wheelTrailUnpackPoint(last, lastPosition, lastWidth);
    wheelTrailStop(store, trail);
    trail = createTrail(store, attachment, owner, flags, lastPosition, lastWidth, materialIndex, lastIntensity);
    wheelTrailAppend(store, trail, position, width, intensity);
    *attachment = trail;
}


#include "indep/inc/TerrainType.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include <array>

namespace hp2 {
struct SkidResources::Storage {
    std::array<WheelTrail,128> trails{};
    WheelTrailStore store{};
    std::array<const EditorMarkerTexture*,33> textures{};
};

SkidResources::SkidResources(ResourceRegistry& resources)
    : storage_(std::make_unique<Storage>())
{
    // Final PAL1AB118 allocates128 SkidSets then reads the authored33x0x90
    //TerrainTypeInfo table. The same loaded GetTextureInfo(a1=1,a2=0)
    //lookup is used, including its explicit original default-texture rule.
    std::size_t count=0;
    const TerrainMaterial* materials=terrainDefaultMaterials(&count);
    Require(materials && count==storage_->textures.size(),"Skids requires the final33-material table");
    wheelTrailStoreInitialize(&storage_->store,storage_->trails.data(),storage_->trails.size());
    for(std::size_t index=0;index<count;++index) {
        const char* sample=materials[index].skidSampleName;
        storage_->textures[index]=sample ? resources.ResolveTexture(hashString(sample),true) : nullptr;
    }
}

SkidResources::~SkidResources()
{
    // The bound trail references must never survive the same pool release.
    // This is native lifetime adaptation of CloseSkids, not extra PAL bodies.
    assert(!storage_->store.clans && "Clans must close before Skids");
    for(auto& trail:storage_->trails)wheelTrailDelete(&storage_->store,&trail);
    storage_->textures.fill(nullptr);
}

WheelTrailStore& SkidResources::Trails() noexcept { return storage_->store; }
const EditorMarkerTexture* SkidResources::MaterialTexture(std::size_t material) const
{
    Require(material<storage_->textures.size(),"Skid material index is outside the authored table");
    return storage_->textures[material];
}
}

namespace {
void prepareSkidQuads(hp2::SkidResources& skids,const WheelTrail& trail,std::uint8_t fade,
    std::vector<hp2::SkidRenderQuad>& output)
{
    // Complete SkidSet::Render CPU work PAL1AACC0..1AAE7C, stopping at the
    // explicit ePoly::Render3D backend boundary rather than simulating it.
    const auto* texture=skids.MaterialTexture(static_cast<std::size_t>(trail.materialIndex));
    if(!texture)return;
    for(std::size_t index=1;index<trail.pointCount;++index) {
        const auto& previous=trail.points[index-1];const auto& next=trail.points[index];
        hp2::SkidRenderQuad quad{};quad.texture=texture;
        quad.rasterState={0x01000100u,0x01000080u,0u,0u};
        quad.vertices[0].textureCoordinate={0.0f,0.0f,1.0f};
        quad.vertices[1].textureCoordinate={1.0f,0.0f,1.0f};
        quad.vertices[2].textureCoordinate={1.0f,1.0f,1.0f};
        quad.vertices[3].textureCoordinate={0.0f,1.0f,1.0f};
        wheelTrailEndpoints(&previous,quad.vertices[0].position.data(),quad.vertices[3].position.data());
        wheelTrailEndpoints(&next,quad.vertices[1].position.data(),quad.vertices[2].position.data());
        const auto color=[fade](std::uint8_t opacity) {
            const int alpha=std::max(static_cast<int>(opacity)-static_cast<int>(fade),0)>>1;
            return (static_cast<std::uint32_t>(alpha)<<24)|0x00808080u;
        };
        quad.vertices[0].color=quad.vertices[3].color=color(previous.opacity);
        quad.vertices[1].color=quad.vertices[2].color=color(next.opacity);
        for(auto& vertex:quad.vertices)vertex.position[2]+=0.05000000074505806f;
        output.push_back(quad);
    }
}
}
void wheelTrailPrepareRender(hp2::SkidResources& skids,WheelTrail* first,
    const float matrix[16],const CarCinematicScreen& screen,std::vector<hp2::SkidRenderQuad>& output)
{
    hp2::Require(matrix && screen.view,"RenderSkids requires its original live view");
    if(screen.replayComposition)return;
    for(auto* trail=first;trail;trail=trail->clanNext) {
        // PAL1AB2B8 calls the boolean IsBoxVisible wrapper10CE80, whose
        // SLTU at10CE94 normalizes classifier10CCB0's 0/1/2 result before
        // AND with the Car's per-view bit. Keeping raw class2 here hides
        // fully visible car trails while admitting partly clipped ones.
        auto visible=static_cast<std::int32_t>(
            ecstasyClassifyBounds(trail->minimum,trail->maximum,matrix)!=0);
        if(trail->owner) {
            const auto* car=static_cast<const Car*>(trail->owner);
            const auto* movement=static_cast<const CarMovement*>(car->movement);
            hp2::Require(movement && movement->bindings.ownerMotion,
                "Skid owner has no canonical Car motion flags");
            const auto flags=static_cast<std::uint32_t>(movement->bindings.ownerMotion->wheelEffectsFlags);
            visible&=static_cast<std::int32_t>((flags>>(static_cast<unsigned>(screen.id)&31u))&1u);
        }
        if(!visible)continue;
        const auto& camera=screen.view->current;
        // PAL1AB2EC reads SkidSet+100, the bounds center updated by every
        // append. The immutable first-point origin is not the fade anchor.
        const float x=trail->center[0]-camera.eye[0],y=trail->center[1]-camera.eye[1],z=trail->center[2]-camera.eye[2];
        const float distance=sqrtf((x*x+y*y)+z*z);
        const auto pixels=ecstasyViewPixelSize(&screen,1.0f,distance);
        if(pixels<=4)continue;
        std::uint8_t fade=0;
        if(pixels<10)fade=static_cast<std::uint8_t>(static_cast<std::uint32_t>(
            convertFloatToInt32(256.0f-(static_cast<float>(pixels)-4.0f)*42.66666793823242f)));
        prepareSkidQuads(skids,*trail,fade,output);
        // Same source global LRU operation even when material has no texture.
        trail->creation=++skids.Trails().revision;
    }
}
