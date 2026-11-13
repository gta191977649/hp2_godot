#ifndef NFSHP2_GHOST_CAR_HPP
#define NFSHP2_GHOST_CAR_HPP
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

// GhostCar.hpp is an observed original header tied to the three separately
// proved GhostCarData methods. These are native semantic declarations only.
enum { RACE_GHOST_SAMPLE_LIMIT = 1024 };

typedef struct RaceGhostSample {
    float frame[16];
    int32_t ticks;
} RaceGhostSample;

typedef struct RaceGhostClip {
    RaceGhostSample* samples;
    size_t capacity;
    int32_t count;
    int32_t cursor;
    int32_t lastCaptureTicks;
    int32_t finishTicks;
    int32_t lapCount;
} RaceGhostClip;

/* Rendering borrows the live playback pose and its authored model key. */
typedef struct RaceGhostModel {
    float frame[16];
    uint32_t modelKey;
    int32_t visible;
} RaceGhostModel;

/* Native storage binding is not an additional original function. Capture
 * requires room for the original 1024-sample limit. Loaded clips have
 * monotonically ordered ticks and a cursor in [0,count). */
int32_t raceGhostBind(RaceGhostClip* clip, RaceGhostSample* samples, size_t capacity);
int32_t raceGhostSample(RaceGhostClip* clip, float output[16], const int32_t* ticks);
void raceGhostCapture(RaceGhostClip* clip, const float frame[16], const int32_t* ticks);


/* A recording has one allocation and an explicit owner. Playback can borrow
 * its clip, but cannot transfer or release this allocation implicitly. */
typedef struct RaceGhostStorage {
    RaceGhostClip clip;
    RaceGhostSample samples[RACE_GHOST_SAMPLE_LIMIT];
    uint16_t identifiers[2];
    char label[128];
} RaceGhostStorage;

/* Full original GhostCarData constructor1B4368: both identifiers,
 * lap count, cleared timestamp array, counters and actual host label. Version3 and
 * reserved serialized bytes remain explicit in the existing file codec. */
void raceGhostDataInitialize(RaceGhostStorage* storage,int32_t firstIdentifier,
    int32_t secondIdentifier,int32_t lapCount);
RaceGhostStorage* raceGhostCreateRecording(int32_t lapCount);
void raceGhostDisposeRecording(RaceGhostStorage* recording);


#ifdef __cplusplus
}
#endif
#endif
