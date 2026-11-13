#include "bware/inc/bMath.hpp"
#include "indep/inc/spikestrip.hpp"
#include "bware/inc/bWare.hpp"

#include <math.h>
#include <string.h>
#include "indep/inc/Replay.hpp"
#include "bware/inc/bWare.hpp"
#include <new>

struct PursuitSpikePoolBlock {
    PursuitSpikePoolBlock* next;
    PursuitSpikeStrip strips[PURSUIT_SPIKE_INITIAL_COUNT/4+1];
};

namespace {
void initializeFreeStrips(PursuitSpikeRegistry* registry)
{
    registry->freeStrips=nullptr;
    registry->totalCapacity=registry->capacity;
    for(size_t index=registry->capacity;index>0;--index) {
        auto* strip=&registry->strips[index-1];
        strip->next=registry->freeStrips;
        registry->freeStrips=strip;
    }
}

void releaseSpikeSlot(PursuitSpikeRegistry* registry,PursuitSpikeStrip* strip)
{
    memset(strip,0,sizeof(*strip));
    strip->next=registry->freeStrips;
    registry->freeStrips=strip;
}
}

void pursuitSpikeStripBeginRetraction(PursuitSpikeStrip* strip)
{
    strip->retracting = 1;
    strip->retractSeconds = 2.0f;
}

void pursuitSpikeStripCenter(const PursuitSpikeStrip* strip, float output[4])
{
    int32_t index, lane;
    output[0] = output[1] = output[2] = output[3] = 0.0f;
    for (index = 0; index < strip->segmentCount; ++index)
        for (lane = 0; lane < 3; ++lane) output[lane] += strip->segments[index].frame[12 + lane];
    if (strip->segmentCount > 1) {
        const float reciprocal = 1.0f / (float)strip->segmentCount;
        for (lane = 0; lane < 3; ++lane) output[lane] *= reciprocal;
    }
}

void pursuitSpikeStripSet(PursuitSpikeStrip* strip, TerrainTopology* terrain,
                          const float first[2], const float second[2],
                          int32_t ownerSelector, float lifetime)
{
    float begin[4], end[4], direction[3], side[3], up[3];
    float squares[3], length, reciprocal;
    int32_t index;
    strip->ownerSelector = ownerSelector;
    strip->first[0] = first[0];
    strip->first[1] = first[1];
    strip->second[0] = second[0];
    strip->second[1] = second[1];
    strip->lifetime = lifetime;
    pursuitGroundPoint(terrain, begin, first);
    pursuitGroundPoint(terrain, end, second);
    // PAL24154C..2415FC separates the length vector (begin-end) from
    // the placement direction (end-begin). Preserve the original finite
    // scalar/VU operation boundaries; host sqrt/div/mul drift in the basis
    // and then accumulate again at every segment's translation.
    for (index = 0; index < 3; ++index) {
        const float difference = bScalarAdd(begin[index], -end[index]);
        direction[index] = bScalarAdd(end[index], -begin[index]);
        squares[index] = bScalarMultiply(difference, difference);
    }
    length = bScalarSquareRoot(bScalarAdd(bScalarAdd(squares[0], squares[1]), squares[2]));
    reciprocal = bScalarDivide(1.0f, length);
    for (index = 0; index < 3; ++index) direction[index] = bScalarMultiply(direction[index], reciprocal);
    strip->segmentCount = convertFloatToInt32(bScalarMultiply(length, 1.5384615659713745f));
    if (strip->segmentCount > PURSUIT_SPIKE_SEGMENTS) strip->segmentCount = PURSUIT_SPIKE_SEGMENTS;
    for (index = 0; index < 3; ++index)
        begin[index] = bScalarAdd(begin[index], bScalarMultiply(direction[index], 0.6499999761581421f));
    side[0] = bScalarAdd(bScalarMultiply(0.0f, direction[2]), -bScalarMultiply(direction[1], 1.0f));
    side[1] = bScalarAdd(bScalarMultiply(1.0f, direction[0]), -bScalarMultiply(direction[2], 0.0f));
    side[2] = bScalarAdd(bScalarMultiply(0.0f, direction[1]), -bScalarMultiply(direction[0], 0.0f));
    squares[0] = bScalarMultiply(side[0], side[0]);
    squares[1] = bScalarMultiply(side[1], side[1]);
    squares[2] = bScalarMultiply(side[2], side[2]);
    reciprocal = bScalarDivide(1.0f, bScalarSquareRoot(bScalarAdd(bScalarAdd(squares[0], squares[1]), squares[2])));
    for (index = 0; index < 3; ++index) side[index] = bScalarMultiply(side[index], reciprocal);
    // PAL241660/241664 OPMULA/OPMSUB keep the second product's operand
    // order. Reversing it is not identical for the source finite multiplier.
    up[0] = bScalarAdd(bScalarMultiply(direction[1], side[2]), -bScalarMultiply(side[1], direction[2]));
    up[1] = bScalarAdd(bScalarMultiply(direction[2], side[0]), -bScalarMultiply(side[2], direction[0]));
    up[2] = bScalarAdd(bScalarMultiply(direction[0], side[1]), -bScalarMultiply(side[0], direction[1]));
    for (index = 0; index < strip->segmentCount; ++index) {
        float* frame = strip->segments[index].frame;
        int32_t lane;
        for (lane = 0; lane < 3; ++lane) {
            frame[lane] = direction[lane];
            frame[4 + lane] = side[lane];
            frame[8 + lane] = up[lane];
            frame[12 + lane] = begin[lane];
            begin[lane] = bScalarAdd(begin[lane], bScalarMultiply(direction[lane], 0.6499999761581421f));
        }
        frame[3] = frame[7] = frame[11] = 0.0f;
        frame[15] = 1.0f;
        strip->segments[index].visible = 1;
    }
}

int32_t pursuitSpikeStripAdvance(PursuitSpikeStrip* strip, float elapsed)
{
    if (strip->retracting) {
        int32_t index;
        strip->retractSeconds -= elapsed;
        for (index = 0; index < strip->segmentCount; ++index)
            strip->segments[index].visible = strip->segments[index].visible == 0;
        return strip->retractSeconds <= 0.0f;
    }
    if (strip->lifetime > 0.0f) {
        strip->lifetime -= elapsed;
        if (strip->lifetime <= 0.0f) pursuitSpikeStripBeginRetraction(strip);
    }
    return 0;
}

void pursuitSpikeRegistryInitialize(PursuitSpikeRegistry* registry,
                                    PursuitSpikeStrip* storage, size_t capacity,
                                    TerrainTopology* terrain,
                                    WorldObjectRegistry* objects)
{
    memset(registry, 0, sizeof(*registry));
    registry->strips = storage;
    registry->capacity = capacity;
    registry->terrain = terrain;
    registry->objects = objects;
    if (storage) memset(storage, 0, capacity * sizeof(*storage));
    if(storage || !capacity)initializeFreeStrips(registry);
}

PursuitSpikeStrip* pursuitSpikeCreate(PursuitSpikeRegistry* registry,
                                      const float first[2], const float second[2],
                                      int32_t ownerSelector, float lifetime)
{
    if (!registry || !registry->terrain || !registry->objects || !first || !second) return NULL;
    if (!isfinite(first[0]) || !isfinite(first[1]) ||
        !isfinite(second[0]) || !isfinite(second[1]) || !isfinite(lifetime)) return NULL;
    if (first[0] == second[0] && first[1] == second[1]) return NULL;
    PursuitSpikeStrip* strip=pursuitSpikeAllocate(registry);
    if(strip)pursuitSpikeStripSet(strip,registry->terrain,first,second,ownerSelector,lifetime);
    return strip;
}

// PAL241308 constructs20 embedded elements with2419F8. That constructor
// calls WorldObject1CD760(SPIKEBELT_00,nullptr,false); every element belongs
// to the SAME global WorldObjectList and eModel registry, initially hidden.
// Init writes the actual embedded frame/+0C enable cells; native borrows
// retain those cells rather than copying geometry into another owner.
PursuitSpikeStrip* pursuitSpikeAllocate(PursuitSpikeRegistry* registry)
{
    const uint32_t key = hashString("SPIKEBELT_00");
    if(!registry || !registry->terrain || !registry->objects || (!registry->strips && registry->capacity))return NULL;
    if(!registry->freeStrips) {
        // AddSpikeStrip2276D0 and CopManager snapshot223B70 both call
        // bOMalloc without IsFull. The source initial10 pool expands by3.
        auto* block=new(std::nothrow) PursuitSpikePoolBlock{};
        if(!block)return nullptr;
        block->next=registry->expanded;registry->expanded=block;
        constexpr size_t count=PURSUIT_SPIKE_INITIAL_COUNT/4+1;
        for(size_t index=count;index>0;--index) {
            auto* strip=&block->strips[index-1];
            strip->next=registry->freeStrips;registry->freeStrips=strip;
        }
        registry->totalCapacity+=count;
    }
    auto* strip=registry->freeStrips;registry->freeStrips=strip->next;
    memset(strip,0,sizeof(*strip));strip->modelKey=key;strip->registry=registry;
    try {
        for(auto& segment:strip->segments) {
            worldObjectRegister(registry->objects,&segment.renderElement,key,nullptr,0);
            segment.renderElement.borrowedFrame=segment.frame;
            segment.renderElement.borrowedEnabled=&segment.visible;
        }
    } catch(...) {
        for(auto& segment:strip->segments)
            if(segment.renderElement.occupied)worldObjectDispose(registry->objects,&segment.renderElement);
        releaseSpikeSlot(registry,strip);
        throw;
    }
    strip->occupied=1;strip->next=registry->first;
    if(registry->first)registry->first->previous=strip;
    else registry->last=strip;
    registry->first=strip;
    return strip;
}

// PAL241898..241960. Restore runs Init with the restored endpoints BEFORE
// restoring blinking state. No fake endpoints are used to allocate a strip.
void pursuitSpikeStripDoSnapshot(PursuitSpikeStrip* strip,ReplaySnapshot* snapshot)
{
    hp2::Require(strip && snapshot && strip->registry && strip->registry->terrain,
        "Spike strip snapshot has no canonical pool/terrain owner");
    replaySnapshotBeginSection(snapshot,nullptr,"tSpikeStrip");
    replaySnapshotMoveData(snapshot,strip->first,8,-1,0);
    replaySnapshotMoveData(snapshot,strip->second,8,-1,0);
    replaySnapshotMoveInteger(snapshot,&strip->ownerSelector,4);
    replaySnapshotMoveFloat(snapshot,&strip->lifetime);
    if(snapshot->restoring)
        pursuitSpikeStripSet(strip,strip->registry->terrain,strip->first,strip->second,
            strip->ownerSelector,strip->lifetime);
    replaySnapshotMoveData(snapshot,&strip->retracting,1,4,0);
    replaySnapshotMoveFloat(snapshot,&strip->retractSeconds);
    replaySnapshotEndSection(snapshot);
}

void pursuitSpikeDestroy(PursuitSpikeStrip* strip)
{
    PursuitSpikeRegistry* registry;
    if (!strip || !strip->occupied) return;
    registry = strip->registry;
    if (registry) {
        // PAL241430 visits all20 embedded destructors in reverse order;
        //241A48 retires the same base eModel and unlinks its WorldObject.
        for(int32_t index=PURSUIT_SPIKE_SEGMENTS-1;index>=0;--index)
            worldObjectDispose(registry->objects,&strip->segments[index].renderElement);
        if (strip->previous) strip->previous->next = strip->next;
        else registry->first = strip->next;
        if (strip->next) strip->next->previous = strip->previous;
        else registry->last = strip->previous;
    }
    if(registry)releaseSpikeSlot(registry,strip);
    else memset(strip,0,sizeof(*strip));
}

void pursuitSpikeRegistryDispose(PursuitSpikeRegistry* registry)
{
    if(!registry)return;
    while(registry->first)pursuitSpikeDestroy(registry->first);
    while(registry->expanded) {
        auto* block=registry->expanded;registry->expanded=block->next;
        delete block;
    }
    if(registry->strips || !registry->capacity)initializeFreeStrips(registry);
}
