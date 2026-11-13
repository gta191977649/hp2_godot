#include "indep/inc/GhostCar.hpp"
#include "bware/inc/bWare.hpp"
#include <stdlib.h>
#include <string.h>

// Full ghostcar.cpp roster: constructor, GetSample, AddSample. The last two
// are existing accepted bodies moved without changing their arithmetic.

static int32_t tickDifference(int32_t first, int32_t second)
{
    const uint32_t bits = (uint32_t)first - (uint32_t)second;
    int32_t result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

int32_t raceGhostBind(RaceGhostClip* clip, RaceGhostSample* samples, size_t capacity)
{
    if (!clip || !samples || capacity < RACE_GHOST_SAMPLE_LIMIT) return 0;
    memset(clip, 0, sizeof(*clip));
    clip->samples = samples;
    clip->capacity = capacity;
    return 1;
}

int32_t raceGhostSample(RaceGhostClip* clip, float output[16], const int32_t* ticks)
{
    // Native file/API boundary: an empty recording is a valid stored clip,
    // but it has no pose to sample. Check the owned extent before the first
    // authored array access; the interpolation below remains unchanged.
    if (!clip || !output || !ticks || !clip->samples || clip->count <= 0 ||
        clip->count > RACE_GHOST_SAMPLE_LIMIT ||
        static_cast<size_t>(clip->count) > clip->capacity ||
        clip->cursor < 0 || clip->cursor >= clip->count) return 0;
    int32_t cursor = clip->cursor;
    const RaceGhostSample *first, *second;
    float fraction = 0.0f, inverse = 1.0f, duration;
    while (clip->samples[cursor].ticks < *ticks) {
        if (cursor == clip->count - 1) return 0;
        clip->cursor = ++cursor;
    }
    while (*ticks < clip->samples[cursor].ticks) {
        if (cursor == 0) return 0;
        clip->cursor = --cursor;
    }
    first = &clip->samples[cursor];
    second = cursor + 1 < clip->count ? first + 1 : first;
    duration = (float)tickDifference(second->ticks, first->ticks) * 0.00025000001187436283f;
    if (0.0f < duration) {
        fraction = ((float)tickDifference(*ticks, first->ticks) * 0.00025000001187436283f) / duration;
        inverse = 1.0f - fraction;
    }
    for (int32_t lane = 0; lane < 16; ++lane)
        output[lane] = first->frame[lane] * inverse + second->frame[lane] * fraction;
    return 1;
}

void raceGhostCapture(RaceGhostClip* clip, const float frame[16], const int32_t* ticks)
{
    // Capture is bound to the complete authored sample allocation. Invalid
    // host bindings must not turn the original append into an out-of-bounds
    // write; a full valid clip retains the original timestamp update below.
    if (!clip || !frame || !ticks || !clip->samples || clip->count < 0 ||
        clip->count > RACE_GHOST_SAMPLE_LIMIT ||
        clip->capacity < RACE_GHOST_SAMPLE_LIMIT) return;
    const float elapsed = (float)tickDifference(*ticks, clip->lastCaptureTicks) * 0.00025000001187436283f;
    if (0.5f < elapsed) {
        clip->lastCaptureTicks = *ticks;
        if (clip->count < RACE_GHOST_SAMPLE_LIMIT) {
            RaceGhostSample* sample = &clip->samples[clip->count++];
            memcpy(sample->frame, frame, sizeof(sample->frame));
            sample->ticks = *ticks;
        }
    }
}

void raceGhostDataInitialize(RaceGhostStorage* storage,int32_t firstIdentifier,
    int32_t secondIdentifier,int32_t lapCount)
{
    storage->clip.samples=storage->samples;
    storage->clip.capacity=RACE_GHOST_SAMPLE_LIMIT;
    storage->clip.count=0;
    storage->clip.cursor=0;
    storage->clip.lastCaptureTicks=0;
    storage->clip.finishTicks=0;
    storage->clip.lapCount=lapCount;
    storage->identifiers[0]=(uint16_t)firstIdentifier;
    storage->identifiers[1]=(uint16_t)secondIdentifier;
    for(size_t index=0;index<RACE_GHOST_SAMPLE_LIMIT;++index)
        storage->samples[index].ticks=0;
    bGetComputerName(storage->label,(int32_t)sizeof(storage->label));
}

// Native ownership adapters: the live Race and file transport borrow this
// allocation, rather than allocating a duplicate clip or source image.
RaceGhostStorage* raceGhostCreateRecording(int32_t lapCount)
{
    RaceGhostStorage* storage=static_cast<RaceGhostStorage*>(malloc(sizeof(*storage)));
    if(storage) raceGhostDataInitialize(storage,0,0,lapCount);
    return storage;
}

void raceGhostDisposeRecording(RaceGhostStorage* recording)
{
    free(recording);
}
