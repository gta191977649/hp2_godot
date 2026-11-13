#ifndef NFSHP2_INDEP_INC_RACE_HPP
#define NFSHP2_INDEP_INC_RACE_HPP

#include "RaceParameters.hpp"
#include <stdint.h>
#include "indep/inc/Timer.hpp"
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/GhostCar.hpp"
#include "indep/inc/World.hpp"
#include <stddef.h>

#ifdef __cplusplus
namespace hp2 {struct RaceEvent;}
typedef hp2::RaceEvent CanonicalRaceEvent;
extern "C" {
#else
typedef struct CanonicalRaceEvent CanonicalRaceEvent;
#endif


typedef enum RaceGhostFileResult {
    RACE_GHOST_FILE_OK,
    RACE_GHOST_FILE_INVALID,
    RACE_GHOST_FILE_IO_ERROR
} RaceGhostFileResult;

/* The original race owner supplies its personal-ghost filename. It is not
 * an ISO resource or a memory-card command. UTF-8 paths reach the native
 * filesystem only here. */
typedef struct RaceGhostFile {
    const char* filename;
    RaceGhostFileResult result;
} RaceGhostFile;

RaceGhostFileResult raceGhostFileSave(RaceGhostFile* file, const RaceGhostStorage* recording);
RaceGhostFileResult raceGhostFileLoad(RaceGhostFile* file, RaceGhostStorage* playback);


typedef struct RaceCarEntry {
    struct Car* car;
    int16_t completionReason;
    int8_t position;
    int8_t previousPosition;
    int8_t finishPosition;
    int16_t currentCheckpoint;
    int16_t furthestCheckpoint;
    int8_t lap;
    int8_t furthestLap;
    uint8_t advancedLap;
    uint8_t advancedCheckpoint;
    int16_t updateCounter;
    float progress;
    float progressRate;
    float previousPercent;
    float displayProgress;
    int32_t checkpointTicks[5];
    int32_t segmentTicks[5];
    int32_t previousSegmentTicks[5];
    int32_t bestSegmentTicks[5];
    int32_t previousBestSegmentTicks[5];
    int32_t lapTicks[11];
    int32_t finishTicks;
    float maximumSpeed;
    float averageSpeed;
} RaceCarEntry;


/* Live race progress is separate from race configuration and the session
 * action queue. Other systems borrow these entries and configuration fields. */
typedef struct RaceFrame {
    int32_t state;
    int32_t startTicks;
    int32_t elapsedTicks;
    int32_t bestSegmentTicks;
    RaceCarEntry** entries;
    size_t entryCount;
    size_t entryCapacity;
    int32_t finishedCount;
    int32_t targetLapCount;
    int16_t checkpointCount;
    int8_t previousCheckpoint;
    int8_t finishCheckpoint;
    float checkpointPositions[5];
    const TrackRoute* route;
    const GameTime* clock;
    const int32_t* raceKind;
    const int32_t* replayRestoring;
    RaceGhostClip* playback;
    RaceGhostModel* ghostModel;
    struct Car* recordingCar;
    RaceGhostClip* recording;
    float startPosition[2];
    float finishPosition[2];
    int32_t startedWorldFrame;
    int32_t countdownWorldFrame;
    struct RaceFrameServices* services;
    int32_t pendingFinishPosition;
    int32_t pendingFinishPlayerIndex;
    /* Actual source Race+20 is initialized to zero and transferred as an
     * unsigned one-byte snapshot value. No unproved gameplay interpretation
     * or beta variable name is attached to this independent source cell. */
    uint32_t snapshotFlags;
    /* PAL Race+6B0, one owned table created by Init147960. */
    struct RaceTimeAroundTrackTable* timeTable;
} RaceFrame;

/* One embedded owner of the eight RacingCar records and sorted pointer view.
 * Placement51 preserves the accepted native allocation and ownership. */
enum { RACE_PARTICIPANT_LIMIT = 8 };
typedef struct RaceFrameOwner {
    RaceFrame race;
    RaceCarEntry participants[RACE_PARTICIPANT_LIMIT];
    RaceCarEntry* ordered[RACE_PARTICIPANT_LIMIT];
} RaceFrameOwner;
struct TrackScenarioCatalog;
WorldServicesResult raceFrameOwnerInitialize(RaceFrameOwner* owner,struct RaceFrameServices* services,
    const GameTime* clock,const TrackRoute* route,const struct TrackScenarioCatalog* course);
WorldServicesResult raceFrameOwnerAddCar(RaceFrameOwner* owner,struct Car* car);
WorldServicesResult raceFrameOwnerPopulate(RaceFrameOwner* owner);
void raceFrameOwnerDispose(RaceFrameOwner* owner);
void raceCarEntryBeginRace(RaceCarEntry* entry,const RaceFrame* race,int32_t pointToPoint);
void raceCarEntryEstimateFinish(RaceCarEntry* entry,const RaceFrame* race,int32_t laps,const int32_t* startTicks);
int32_t raceFrameCountdownStep(const RaceFrame* race,int32_t specialPursuit);
float raceFrameCountdownProgress(const RaceFrame* race,int32_t specialPursuit);
RaceCarEntry* raceFrameFindFinishPosition(const RaceFrame* race,int32_t position);
int32_t raceFrameNextFinishPosition(const RaceFrame* race);
/* Complete released query1490F8. The source consults creation-order Players
 * through the sole live registry. AreAllPlayersFinishedRacing1490B0 already
 * exists as carFramePlayerAllDrivingLocked and is not implemented twice. */
struct CarFramePlayerState* raceFrameMostRecentlyFinishedPlayer(const RaceFrame* race);
/* Final PAL146788, the six authored finish-reason language IDs. */
int32_t raceFinishReasonLanguageId(int32_t reason);

struct World;

struct RaceProfileTimes;
struct CarFramePlayerRegistry;
struct ReplayManager;
struct PursuitManager;
struct RaceCommentary;
struct RaceGhostStorage;
struct RaceGhostFile;
struct GameSession;

/* Each reference borrows the live owner used by the rest of the world.
 * Recording clips are created by raceGhostCreateRecording. Persistence
 * borrows a native filename; saving never changes the loaded playback. */
typedef struct RaceFrameServices {
    struct World* world;
    const RaceParameters* settings;
    CanonicalRaceEvent* currentEvent;
    const struct RaceProfileTimes* profile;
    struct CarFramePlayerRegistry* players;
    struct ReplayManager* replay;
    struct PursuitManager* pursuit;
    struct RaceCommentary* commentary;
    struct RaceGhostStorage* recordingStorage;
    struct RaceGhostFile* ghostFile;
    WorldServicesResult result;
    struct GameSession* session;
} RaceFrameServices;

WorldServicesResult raceFrameSetState(RaceFrame* race, int32_t state);
WorldServicesResult raceFrameCommitGhost(RaceFrame* race, const RaceCarEntry* entry);
WorldServicesResult raceFrameFinishParticipant(RaceFrame* race, size_t index,
    int32_t finishPosition, int32_t transition, int32_t completionReason);


struct CarFramePlayerRegistry;
struct ReplayManager;
void raceFrameHighlightPositionChanges(const RaceFrame* race,const struct CarFramePlayerRegistry* players,struct ReplayManager* replay);

struct CarFramePlayerState* raceCarEntryPlayer(const RaceCarEntry* entry,const RaceFrame* race);
int32_t raceCarEntryBestLapTicks(const RaceCarEntry* entry,const RaceFrame* race);

typedef struct RaceCheckpointCrossing {
    int32_t forward;
    int32_t backward;
    int32_t correctionTicks;
} RaceCheckpointCrossing;

int32_t raceCarEntryCheckCheckpoint(const RaceCarEntry* entry, const RaceFrame* race,
                                   RaceCheckpointCrossing* crossing,
                                   float currentPercent, float checkpointPercent);
float raceFrameCheckpointFraction(const RaceFrame* race, float currentPercent);
void raceFrameSortEntries(RaceFrame* race);
/* Complete final finish graph. Both consume the same live participants and
 * event-results owner used by the running race; neither builds UI copies. */
WorldServicesResult raceFrameSimulateFinish(RaceFrame* race);
WorldServicesResult raceFrameNotifyRaceOver(RaceFrame* race);
void raceCarEntryInitialize(RaceCarEntry* entry, struct Car* car, const RaceFrame* race);
void raceCarEntryUpdate(RaceCarEntry* entry, const RaceFrame* race);
void raceFrameAdvanceGhost(RaceFrame* race);
RaceCarEntry* raceCarEntryResetTiming(RaceCarEntry* entry);

/* One source TimeAroundTrackTable owns256 rate/time pairs. These are CPU
 * values derived from the same live authored road poses, not a second track. */
typedef struct RaceTimeAroundTrackBin {
    float progressPerSecond;
    float secondsAtStart;
} RaceTimeAroundTrackBin;
typedef struct RaceTimeAroundTrackTable {
    float lapSeconds;
    RaceTimeAroundTrackBin bins[256];
} RaceTimeAroundTrackTable;
struct CarRecoveryPointRegistry;
struct RaceFrameOwner;
WorldServicesResult raceTimeAroundTrackBuild(RaceTimeAroundTrackTable* table,const RaceFrame* race,
    const struct CarRecoveryPointRegistry* road,int32_t vehicleClass);
const RaceTimeAroundTrackBin* raceTimeAroundTrackEntry(const RaceTimeAroundTrackTable* table,
    float progress,float* seconds);
WorldServicesResult raceTimeAroundTrackEvaluate(const RaceTimeAroundTrackTable* table,
    float progress,float* seconds);
WorldServicesResult raceFrameOwnerInitStage(struct RaceFrameOwner* owner,int32_t stage,
    const struct CarRecoveryPointRegistry* road);
void raceFrameDisposeTimeTable(RaceFrame* race);
struct ReplaySnapshot;
void raceCarEntryDoSnapshot(RaceCarEntry* entry,const RaceFrame* race,struct ReplaySnapshot* snapshot);
void raceFrameOwnerDoSnapshot(RaceFrameOwner* owner,struct ReplaySnapshot* snapshot);
WorldServicesResult raceFrameCalculateSplitTime(const RaceFrame* race,int32_t* ticks,
    const RaceCarEntry* first,const RaceCarEntry* second);

// Original Race course/query boundaries; individually proved in paths73.json.
int32_t raceFrameAddCheckpoint(RaceFrame* race,const struct TrackScenarioCatalog* catalog,
    const char* name,float position[2],float fallbackPercent);
void raceFrameBuildCheckpoints(RaceFrame* race,const struct TrackScenarioCatalog* catalog,
    int32_t reverse,int32_t pointToPoint);
typedef struct RaceConnectionTiming {
    float baseSeconds;
    float driverStepSeconds;
} RaceConnectionTiming;
extern const RaceConnectionTiming raceConnectionDefaultTiming;
int32_t raceFrameIsDriving(const RaceFrame* race,int32_t specialPursuit);
RaceCarEntry* raceFrameEntryForCar(const struct Car* car);
int32_t raceFrameConnectionWindowActive(const RaceFrame* race,const struct Car* car,
    const RaceConnectionTiming* timing);

#ifdef __cplusplus
}
#endif

#endif
