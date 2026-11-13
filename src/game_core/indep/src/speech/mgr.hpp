#ifndef NFSHP2_WORLD_SPEECH_RAISE_H
#define NFSHP2_WORLD_SPEECH_RAISE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct GameSession;
struct WorldStreamAudio;
struct WorldAudioMessages;
struct SpeechNoise;
struct WorldSpeechFsm;
struct CarFramePlayerState;

// Shared original race/lap speech producers. These references borrow the one
// live race, parameter, Player, profile and SED owners; no copied PS2 layout.
struct RaceFrame;
struct RaceParameters;
struct RaceProfileTimes;
struct CarFramePlayerRegistry;
struct Car;
typedef struct RaceCommentary {
    const struct RaceFrame* race;
    const struct RaceParameters* settings;
    const struct GameSession* session;
    const struct RaceProfileTimes* profile;
    const struct CarFramePlayerRegistry* players;
    struct WorldAudioMessages* audio;
} RaceCommentary;
int32_t raceCommentarySuppressed(const RaceCommentary* commentary,const struct Car* car);
void raceCommentarySelectSingle(const RaceCommentary* commentary,uint32_t words[4],
    const struct Car* car,int32_t lap);
void raceCommentarySelectSplit(const RaceCommentary* commentary,uint32_t words[4],const struct Car* car);
int32_t raceCommentaryAnnounceRecord(RaceCommentary* commentary,const struct Car* car);
void raceCommentaryAnnounceLap(RaceCommentary* commentary,const struct Car* car,int32_t lap);

/* Native semantic fields are established by the final PAL raise producers.
 * There is no copied console runtime block or unexamined padding. A producer
 * updates only its own fields; other values retain the previous context. */
typedef struct WorldSpeechContext {
    uint32_t fromId;
    uint32_t toId;
    int32_t location;
    int32_t arrestType;
    uint32_t otherCopId;
    int32_t variant;
    int32_t approximateVehicle;
    int32_t exactVehicle;
    int32_t vehicleColor;
    int32_t vehicleSpeed;
    int32_t racingVariant;
    int32_t direction;
    int32_t copLocation;
    int32_t rotorChatterVariant;
} WorldSpeechContext;

typedef struct WorldSpeechEvent {
    uint32_t type;
    uint32_t contextIndex;
} WorldSpeechEvent;

enum { WORLD_SPEECH_EVENT_LIMIT = 2, WORLD_SPEECH_CONTEXT_LIMIT = 4 };

typedef struct WorldSpeechQueue {
    WorldSpeechEvent events[WORLD_SPEECH_EVENT_LIMIT];
    WorldSpeechContext contexts[WORLD_SPEECH_CONTEXT_LIMIT];
    uint32_t contextWriteIndex;
    uint32_t readIndex;
    uint32_t writeIndex;
    uint32_t count;
} WorldSpeechQueue;

typedef enum WorldSpeechResult {
    WORLD_SPEECH_OK,
    WORLD_SPEECH_SUPPRESSED,
    WORLD_SPEECH_DROPPED,
    WORLD_SPEECH_INVALID_BINDING,
    WORLD_SPEECH_AUDIO_FAILURE,
    WORLD_SPEECH_ALLOCATION_FAILURE
} WorldSpeechResult;

/* The three source lookup arrays have distinct identities and are populated
 * together by ShuffleCopIds. Their initial static values are all zero. */
typedef struct WorldSpeechIdentityMap {
    uint32_t copIds[4];
    uint32_t fromCopIds[4];
    uint32_t toIds[4];
} WorldSpeechIdentityMap;

/* The one game speech owner. Its speaking value is computed by the real
 * PCM/noise/FSM update; backend acknowledgement or queue occupancy alone
 * must never write it. CopManager borrows this owner and its same queue. */
typedef struct WorldSpeechManager {
    WorldSpeechQueue queue;
    uint32_t currentContext;
    int32_t speaking;
    struct WorldSpeechFsm* fsm;
    const struct GameSession* session;
    struct WorldStreamAudio* streamedAudio;
    struct SpeechNoise* noise;
    struct WorldAudioMessages* messages;
    WorldSpeechResult result;
    WorldSpeechIdentityMap* identities;
} WorldSpeechManager;

WorldSpeechContext* worldSpeechNextContext(WorldSpeechManager* manager);
WorldSpeechResult worldSpeechPushEvent(WorldSpeechManager* manager, uint32_t event);
WorldSpeechResult worldSpeechInitialize(WorldSpeechManager* manager);
WorldSpeechResult worldSpeechAllocateVoices(WorldSpeechManager* manager);
WorldSpeechResult worldSpeechUpdate(WorldSpeechManager* manager);
WorldSpeechResult worldSpeechFlush(WorldSpeechManager* manager);
/* Native owned-FSM storage disposal, not a claimed PAL destructor. All
 * audio/stream/noise/identity bindings remain borrowed and outlive the owner. */
void worldSpeechDispose(WorldSpeechManager* manager);
extern int32_t worldSpeechDebugPhraseRequested;

/* Storage initialization and deque access are native ownership helpers,
 * not aliases for the original full manager/FSM constructor or update. */
void worldSpeechQueueInitialize(WorldSpeechQueue* queue);
int32_t worldSpeechTakeEvent(WorldSpeechManager* manager, WorldSpeechEvent* event);

#ifdef __cplusplus
}
#endif

#include "indep/src/speech/SEDWedge.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void worldSpeechShuffleCopIds(WorldSpeechIdentityMap* identities);
int32_t worldSpeechGetArrestType(const WorldSpeechContext* context);
int32_t worldSpeechGetExactVehicle(const WorldSpeechContext* context);
int32_t worldSpeechGetApproximateVehicle(const WorldSpeechContext* context);
int32_t worldSpeechGetVehicleColor(const WorldSpeechContext* context);
int32_t worldSpeechGetRaceType(const WorldSpeechContext* context);
int32_t worldSpeechConvertSpeed(float speed);
WorldSpeechManager* worldSpeechGetInstance(void);
WorldSpeechResult worldSpeechFade(WorldSpeechManager* manager,float duration);
WorldSpeechResult worldSpeechMute(WorldSpeechManager* manager,int32_t muted);
// PC Pause Menu: retain the current spoken phrase and its sample cursor.
WorldSpeechResult worldSpeechPausePlayback(WorldSpeechManager* manager,int32_t paused);
WorldAudioMessagesResult worldAudioPlayPullover(WorldAudioMessages* messages,
    const struct GameSession* session);
WorldAudioMessagesResult worldAudioPlayBeACopIntro(WorldAudioMessages* messages);
WorldAudioMessagesResult worldAudioRaceFinishedSpeech(struct CarFramePlayerState* player);
WorldAudioMessagesResult worldAudioKnockoutRaceOverSpeech(struct CarFramePlayerState* player,
    int32_t won);
int32_t worldSpeechConvertCarTypeToDescription(int32_t carType);
int32_t worldSpeechConvertCarTypeToUnlockSpeech(int32_t carType);
int32_t worldSpeechConvertTrackNumberToDescription(int32_t trackNumber);
int32_t worldSpeechConvertTrackNumberToUnlockSpeech(int32_t trackNumber);
int32_t worldSpeechConvertColourName(const char* name);

WorldSpeechResult worldSpeechRaiseTacticDivert(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseRotorRequest(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseRoadblockRequest(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseRoadBlockFailureAvoid(WorldSpeechManager* manager, uint32_t toId, int32_t variant, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseCrashAmbulance(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseSpikeStripSet(WorldSpeechManager* manager, uint32_t fromId, int32_t copLocation, int32_t variant);
WorldSpeechResult worldSpeechRaiseDispatchHelicopterNA(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaisePursuitUpdateSubjectLocation(WorldSpeechManager* manager, uint32_t fromId, int32_t location, int32_t variant);
WorldSpeechResult worldSpeechRaiseTacticCutoff(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseBeginPursuitIdRepeated(WorldSpeechManager* manager, uint32_t fromId, int32_t variant, int32_t exactVehicle, int32_t vehicleColor);
WorldSpeechResult worldSpeechRaiseBeginPursuitSpeed(WorldSpeechManager* manager, uint32_t fromId, float speed);
WorldSpeechResult worldSpeechRaiseTacticHit(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId);
WorldSpeechResult worldSpeechRaiseRoadBlockSet(WorldSpeechManager* manager, uint32_t fromId, int32_t variant, int32_t copLocation);
WorldSpeechResult worldSpeechRaiseSuperInitiate(WorldSpeechManager* manager, uint32_t toId, int32_t location, int32_t exactVehicle, int32_t vehicleColor, float speed);
WorldSpeechResult worldSpeechRaiseBeginPursuitGeneral(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseRotorTraffic(WorldSpeechManager* manager);
WorldSpeechResult worldSpeechRaiseBeginPursuitRacing(WorldSpeechManager* manager, uint32_t fromId, int32_t location);
WorldSpeechResult worldSpeechRaiseSuperRequest(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseAirborne(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseRadioArrest(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseBeginPursuitReckless(WorldSpeechManager* manager, uint32_t fromId, int32_t location);
WorldSpeechResult worldSpeechRaiseSuspectCrashTraffic(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseSuspectLost(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseCurrentSpeed(WorldSpeechManager* manager, uint32_t fromId, float speed);
WorldSpeechResult worldSpeechRaiseChatterGeneral(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseSpikeStripSetLeft(WorldSpeechManager* manager, uint32_t fromId, int32_t copLocation, int32_t variant);
WorldSpeechResult worldSpeechRaiseRacerCaught(WorldSpeechManager* manager, uint32_t fromId, uint32_t otherCopId, int32_t arrestType);
WorldSpeechResult worldSpeechRaiseSuspectCrashTow(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseBeginPursuitErratic(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseBackupRequestLocation(WorldSpeechManager* manager, uint32_t fromId, int32_t location);
WorldSpeechResult worldSpeechRaisePursuitUpdateSpeed(WorldSpeechManager* manager, uint32_t fromId, int32_t variant, int32_t vehicleSpeed);
WorldSpeechResult worldSpeechRaiseDispatchBeOnLookout(WorldSpeechManager* manager, int32_t location);
WorldSpeechResult worldSpeechRaiseBeginPursuitCopHitRun(WorldSpeechManager* manager, uint32_t fromId, int32_t location);
WorldSpeechResult worldSpeechRaiseRequestAnotherRoadblock(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseDirection(WorldSpeechManager* manager, uint32_t fromId, int32_t direction);
WorldSpeechResult worldSpeechRaiseCrashTow(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId);
WorldSpeechResult worldSpeechRaiseRotorChatter(WorldSpeechManager* manager, int32_t rotorChatterVariant);
WorldSpeechResult worldSpeechRaiseBeginPursuitNoId(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseBeginPursuitAgain(WorldSpeechManager* manager, uint32_t fromId, int32_t exactVehicle, int32_t vehicleColor);
WorldSpeechResult worldSpeechRaiseRoadBlockSetting(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseCopBackOff(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseRotorLocation(WorldSpeechManager* manager, int32_t location);
WorldSpeechResult worldSpeechRaiseSpikeStripFailure(WorldSpeechManager* manager, uint32_t toId, int32_t variant, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseCarStopped(WorldSpeechManager* manager, uint32_t fromId, int32_t exactVehicle);
WorldSpeechResult worldSpeechRaiseFlatTire(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId);
WorldSpeechResult worldSpeechRaiseSpikeStripSetRight(WorldSpeechManager* manager, uint32_t fromId, int32_t copLocation, int32_t variant);
WorldSpeechResult worldSpeechRaiseBeginPursuitIdGeneral(WorldSpeechManager* manager, uint32_t fromId, int32_t approximateVehicle, int32_t vehicleColor);
WorldSpeechResult worldSpeechRaiseBeginPursuitIdSpecific(WorldSpeechManager* manager, uint32_t fromId, int32_t variant, int32_t exactVehicle, int32_t vehicleColor);
WorldSpeechResult worldSpeechRaiseBeginPursuitHitRunTraffic(WorldSpeechManager* manager, uint32_t fromId, int32_t location);
WorldSpeechResult worldSpeechRaiseDispatchRoadBlockNA(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseSuspectCrashAmbulance(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseRotorConfirm(WorldSpeechManager* manager);
WorldSpeechResult worldSpeechRaiseCrash(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId);
WorldSpeechResult worldSpeechRaiseSpikeStripRequest(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseUpdateReckless(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseCopPursuitAvoidance(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseSuperBackOff(WorldSpeechManager* manager);
WorldSpeechResult worldSpeechRaiseBackupRequest(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseSpikeStripTireBlow(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseChatterTraffic(WorldSpeechManager* manager, uint32_t fromId);
WorldSpeechResult worldSpeechRaiseRotorSpikeFailed(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaiseSuspectCrash(WorldSpeechManager* manager, uint32_t fromId, int32_t variant);
WorldSpeechResult worldSpeechRaisePursuitCopHit(WorldSpeechManager* manager, uint32_t fromId, int32_t location);
WorldSpeechResult worldSpeechRaiseSuspectFound(WorldSpeechManager* manager,uint32_t fromId,int32_t variant,int32_t location);
WorldSpeechResult worldSpeechRaiseBackupConfirm(WorldSpeechManager* manager, uint32_t fromId, uint32_t toId);
WorldSpeechResult worldSpeechRaisePursuitUpdateVehicleIdSpecific(WorldSpeechManager* manager, uint32_t fromId, int32_t exactVehicle, int32_t vehicleColor);
WorldSpeechResult worldSpeechRaiseRoadBlockFailureThrough(WorldSpeechManager* manager, uint32_t toId, int32_t variant, uint32_t fromId);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// The seven PAL context accessors share the sole identity-map owner.
struct WorldSpeechIdView {
    const WorldSpeechIdentityMap& map;
    uint32_t CopId(const WorldSpeechContext& context) const;
    uint32_t TargetCopId(const WorldSpeechContext& context) const;
    uint32_t FromCopId(const WorldSpeechContext& context) const;
    uint32_t TargetFromCopId(const WorldSpeechContext& context) const;
    uint32_t ToId(const WorldSpeechContext& context) const;
    uint32_t TargetToId(const WorldSpeechContext& context) const;
    uint32_t Supervisor(const WorldSpeechContext& context) const;
};
struct RaceParameters;
namespace hp2 {class CarInfoResource;}
namespace hp2 {
struct RaceEvent;
struct EventReward;
struct TransientStatsElement;
struct FrontendPlayerManager;
WorldAudioMessagesResult worldAudioPostGameLapRecord(WorldAudioMessages* messages,
    const RaceParameters& parameters,const RaceEvent* event,int32_t player,int32_t record,
    const TransientStatsElement& result);
WorldAudioMessagesResult worldAudioPostGameTimeRecord(WorldAudioMessages* messages,
    const RaceParameters& parameters,const RaceEvent* event,int32_t player,int32_t record,
    const TransientStatsElement& result);
WorldAudioMessagesResult worldAudioTournamentRaceSpeech(WorldAudioMessages* messages,bool finished,int32_t place);
WorldAudioMessagesResult worldAudioUnlockCarSpeech(WorldAudioMessages* messages,const CarInfoResource& cars,int32_t model);
WorldAudioMessagesResult worldAudioUnlockTrackSpeech(WorldAudioMessages* messages,int32_t track,int32_t direction);
WorldAudioMessagesResult worldAudioRewardScreenSpeech(WorldAudioMessages* messages,
    const CarInfoResource& cars,bool rewardEarned,const EventReward* reward);
WorldAudioMessagesResult worldAudioRaceEventSpeech(WorldAudioMessages* messages,CarFramePlayerState* player,
    const TransientStatsElement* result,const RaceEvent& event,const FrontendPlayerManager& frontend);
}
#endif
#endif
