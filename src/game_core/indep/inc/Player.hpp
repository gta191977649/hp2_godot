#ifndef NFSHP2_PLAYER_HPP
#define NFSHP2_PLAYER_HPP

#include "indep/inc/Car.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include "indep/inc/RaceParameters.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/CarDriver.hpp"
/* Native ordered presentation transport borrowed by Player/Car. */
#ifdef __cplusplus
#include <exception>
extern "C" {
#endif
struct CarMovement;
struct CarRoadPreview;
typedef enum CarFramePresentationKind {
    CAR_FRAME_DRIVING_STAT,
    CAR_FRAME_CAMERA_PUSH,
    CAR_FRAME_CAMERA_POP,
    CAR_FRAME_CAMERA_REPLACE,
    CAR_FRAME_CAMERA_RESET,
    CAR_FRAME_INPUT_MODE,
    CAR_FRAME_UI_BEGIN_CINEMATIC,
    CAR_FRAME_UI_END_CINEMATIC,
    CAR_FRAME_UI_CLEAR_TRANSIENT,
    CAR_FRAME_CINEMATIC_AUDIO_START,
    CAR_FRAME_CINEMATIC_AUDIO_STOP,
    CAR_FRAME_ACCELERATION_FEEDBACK,
    CAR_FRAME_CONTROLLER_EFFECT,
    CAR_FRAME_SURFACE_EFFECT,
    CAR_FRAME_FORCE_FEEDBACK
} CarFramePresentationKind;

typedef enum CarFrameDrivingStat {
    CAR_FRAME_STAT_IN_AIR,
    CAR_FRAME_STAT_GOING_SLOW,
    CAR_FRAME_STAT_CLOSE_CARS,
    CAR_FRAME_STAT_CLOSE_COPS,
    CAR_FRAME_STAT_SPEED,
    CAR_FRAME_STAT_SLIDING_SIDEWAYS,
    CAR_FRAME_STAT_CAR_COLLISION,
    CAR_FRAME_STAT_SMOKEABLE_COLLISION
} CarFrameDrivingStat;

typedef enum CarFrameForceFeedbackOperation {
    CAR_FRAME_FORCE_PAIR_PARAMETERS,
    CAR_FRAME_FORCE_PERIODIC_PARAMETERS,
    CAR_FRAME_FORCE_IMPACT,
    CAR_FRAME_FORCE_GAIN
} CarFrameForceFeedbackOperation;

typedef struct CarFramePresentationEvent {
    CarFramePresentationKind kind;
    int32_t code;
    int32_t port;
    int32_t argument;
    float value[4];
    struct CarMovement* car;
    CarRoadPreview preview;
    int32_t hasPreview;
    float position[4];
    int32_t hasPosition;
    float forceParameters[9];
    int32_t parameterCount;
} CarFramePresentationEvent;

typedef struct CarFramePresentation {
    CarFramePresentationEvent* events;
    size_t capacity;
    size_t count;
    int32_t failed;
} CarFramePresentation;

/* Presentation consumers receive ordered typed requests. They do not feed
 * invented gameplay results into the simulation. Storage remains borrowed. */
CarFramePresentationEvent* carFramePresentationAppend(CarFramePresentation* presentation,
    CarFramePresentationKind kind, struct CarMovement* car, int32_t code);
int32_t carFramePresentationCopyPreview(CarFramePresentationEvent* event, const CarRoadPreview* preview);
void carFramePresentationReset(CarFramePresentation* presentation);
#ifdef __cplusplus
}
#endif

#include "indep/inc/Table.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "bsound/include/bSample.h"
#include "RaceParameters.hpp"
#ifdef __cplusplus
extern "C" {
#endif

/* Native borrowed services of the individually proved Player owner. */
struct PursuitManager;
struct CarPlayerCameraServices;
struct RaceFrame;
struct ReplayManager;
struct WorldStreamAudio;
struct ScreenRadialBlur;
struct GameSession;
struct CarFramePlayerInputBindings;
struct PreviewCometServices;

/* These are references to the live owners used by the world, drivers and
 * player. The service binding contains no copies of their mutable state. */
typedef struct CarFramePlayerServices {
    struct PursuitManager* pursuit;
    RaceActionQueue* raceActions;
    WorldAudioRequestQueue* audio;
    GameInputRouter* input;
    struct CarPlayerCameraServices* cameras;
    struct RaceFrame* race;
    struct ReplayManager* replay;
    struct WorldStreamAudio* streamedAudio;
    struct ScreenRadialBlur* radialBlur;
    const WorldAudioManagerState* audioManager;
    struct GameSession* session;
    const struct CarFramePlayerInputBindings* inputBindings;
    const struct PreviewCometServices* previewComet;
} CarFramePlayerServices;

typedef struct CarFramePlayerState CarFramePlayerState;
typedef struct CarFramePlayerRegistry CarFramePlayerRegistry;
typedef struct CarFrameLiveReplay CarFrameLiveReplay;
typedef struct CarFramePresentation CarFramePresentation;
typedef struct CarFramePlayerCameras CarFramePlayerCameras;
struct CarPlayerScoreState;
struct CarPlayerVisibleDrivingLine;
struct CarPlayerNisSelection;
struct CarPlayerOwnedState;
extern int32_t displayPlayerDebugInfo; // PAL330814, source default0.
struct CarSoundPolicy;
struct RaceProfileBestTime;
typedef struct CarNisOverlay CarNisOverlay;
/* PAL1831D0 initializes the one mutable split-player colour at30BB20. */
extern float playerSecondLapColor[4];
void carFramePlayerHudStartWrongWay(CarFramePlayerState* player);
void carFramePlayerHudDoAnimations(CarFramePlayerState* player);
void carFramePlayerRenderTrackMap(CarFramePlayerState* player);
void carFramePlayerSetTrackMapMode(CarFramePlayerState* player,int32_t mode);
uint32_t carFramePlayerDetermineHudFeatures(CarFramePlayerState* player);
uint32_t carFramePlayerGetGaugeMode(const CarFramePlayerState* player);
int32_t carFramePlayerShouldShowBustedText(const CarFramePlayerState* player);
int32_t carFramePlayerCurrentBestLapRecord(const CarFramePlayerState* player,int32_t previous);
float carFramePlayerDisplaySpeed(const CarFramePlayerState* player,float metresPerSecond);
void carFramePlayerDetermineAllInputMaps(CarFramePlayerRegistry* registry);
void carFramePlayerUpdateNonGameState(CarFramePlayerState* player);
void carFramePlayerUpdateNonGameStateAll(CarFramePlayerRegistry* registry);
void carFramePlayerUpdateHud(CarFramePlayerState* player);
void carFramePlayerUpdateScoring(CarFramePlayerState* player);
void carFramePlayerRenderHud(CarFramePlayerState* player);
void carFramePlayerDrawTracker(CarFramePlayerState* player,const Car* car,int32_t slot);
void carFramePlayerFillRaceOverMessages(CarFramePlayerState* player);
void carFramePlayerEndPause(CarFramePlayerState* player);
int32_t carFramePlayerCalcBonusPoints(const CarFramePlayerState* player,int32_t finishPosition);
void carFramePlayerDoSnapshot(CarFramePlayerState* player,struct ReplaySnapshot* snapshot);
void carFramePlayerCinematicTimeout(void* context);
typedef enum CarPlayerNisCompletion {
    CAR_PLAYER_NIS_COMPLETION_NONE,
    CAR_PLAYER_NIS_COMPLETION_PURSUIT_RESET
} CarPlayerNisCompletion;

typedef struct CarFramePlayerRegistry {
    /* Source pPlayersByIndex: creation order, traversed up to count. */
    CarFramePlayerState* players[2];
    int32_t count;
    World* world;
    const GameTime* clock;
    const int32_t* raceKind;
    const int32_t* sessionState;
    const int32_t* alternateRaceGate;
    const GlobalDisplaySettings* displaySettings;
    int32_t airbornePredictionDisabled;
    /* Actual GP-5A10 is shared with every vehicle sound, never copied. */
    const struct CarSoundPolicy* soundPolicy;
    int32_t trackId;
    const int32_t* pursuitEnabled;
    CarFramePlayerServices* services;
    /* Source pPlayersByNumber: the same players indexed by movement.seat.
     * World positions use this view even when creation order differs. */
    CarFramePlayerState* playersByNumber[2];
    /* Actual RaceSettings+4C / PAL31A8E4, not a copied default. */
    const int32_t* enableVisibleDrivingLine;
} CarFramePlayerRegistry;

/* The movement view is embedded in this owner, so recovery, collision pad
 * feedback and frame transitions read the same cinematic state. The legacy
 * view member steeringFeedbackBusy is the cinematic mode, not a second flag. */
struct CarFramePlayerState {
    CarMovementPlayer movement;
    CarMovement* car;
    CarFramePlayerRegistry* registry;
    CarFrameLiveReplay* replay;
    CarFramePresentation* presentation;
    CarDriver* driver;
    int32_t wrongWay;
    int32_t previousCinematicState;
    /* PAL+60: eight constructor-zero bytes retained by DoSnapshot. The
     * release contains no direct Player consumer beyond its snapshot. */
    uint8_t cinematicStateFlags[8];
    int32_t stateStartedTicks[8];
    int32_t inputMode;
    /* PAL+50: TimeFinishedRacing, written from GameTime.worldTimeFrames. */
    int32_t drivingLocked;
    int32_t transitionAudio;
    float transitionAudioStrength;
    float airborneCameraHeight;
    CarRoadPreview airbornePreview;
    int32_t result;
    DriverHistory speedHistory;
    float maximumSpeedRatio;
    int32_t speedRatioCountdownTicks;
    float cinematicCountdown;
    int32_t crashTicks;
    int32_t recoveryProtectionTicks;
    float previousSpeedMph;
    int32_t speedSampleTicks;
    /* PAL+BC/+C0: legacy snapshot cells. C0 is explicitly zeroed by the
     * final ctor; BC has no ctor write or direct Player reader/writer.
     * Value-initialization makes the latter defined on the native host;
     * restoration preserves its four actual bytes. No gameplay is inferred. */
    uint32_t snapshotRetainedWord;
    int32_t snapshotRetainedFlag;
    int32_t lookBack;
    /* PAL+54: FinishAnimationLength in world frames, set360 at finish. */
    int32_t finishCounter;
    WorldAudioInstance* cinematicVoice;
    CarFramePlayerCameras* cameras;
    uint64_t cinematicVoiceGeneration;
    CarPlayerNisCompletion nisCompletion;
    int32_t nisCompletionIndex;
    CarNisOverlay* nisOverlay;
    struct CarPlayerScoreState* scoreState;
    struct CarPlayerVisibleDrivingLine* visibleDrivingLine;
    struct CarPlayerNisSelection* nisSelection;
    /* Source ctor copies the complete live runtime bank record by value.
     * Camera, input and HUD read this one embedded settings owner. */
    PlayerSettings settings;
    /* PAL Player+98, ctor17531C zero; request inputs store realTimer. */
    int32_t inputActionRealTimer;
    /* PAL+0C, independent of the canonical controller port in movement. */
    int32_t joystickType;
    /* Owned constructor state; null for a borrowed legacy facade. */
    struct CarPlayerOwnedState* ownedState;
    /* PAL+9F8/+9FC are borrowed insertion results in the same current
     * profile table, written by HighScoreTable.MaybeRecordHighScore. */
    struct RaceProfileBestTime* recordLapTime;
    struct RaceProfileBestTime* recordTotalTime;
    /* PAL+A84 is the source once-only RaceFinishedSpeech gate. */
    int32_t undamagedRace; // PAL+9EC: ctor1; qualifying collision in UpdateScoring clears it.
    int32_t raceFinishedSpeechSent;
    /* PAL+A88 is independent of A84: once-only event completion wording. */
    int32_t eventFinishedSpeechSent;

};

typedef enum CarPlayerLifecycleResult {
    CAR_PLAYER_LIFECYCLE_OK=0,
    CAR_PLAYER_LIFECYCLE_INVALID_OWNER=1,
    CAR_PLAYER_LIFECYCLE_NIS_FAILURE=2,
    CAR_PLAYER_LIFECYCLE_CAMERA_FAILURE=3,
    CAR_PLAYER_LIFECYCLE_HUD_NOT_PORTED=4
} CarPlayerLifecycleResult;
int32_t carFramePlayerAttachToCar(CarFramePlayerState* player,Car* car);
int32_t carFramePlayerDetachFromCar(CarFramePlayerState* player);
int32_t carFramePlayerHideHUD(CarFramePlayerRegistry* registry);
int32_t carFramePlayerShowHUD(CarFramePlayerRegistry* registry);
int32_t carFramePlayerSetHudFeatures(CarFramePlayerState* player,uint32_t mask);
void carFramePlayerClearHudAnimations(CarFramePlayerState* player);
void carFramePlayerClearRecoveryAirborneTime(CarFramePlayerState* player);
void carFramePlayerResetFlailerMovement(CarFramePlayerState* player);
uint32_t carFramePlayerHudFeatures(const CarFramePlayerState* player);
uint32_t carFramePlayerHudBlockedFeatures(const CarFramePlayerState* player);

/* Native binding of the actual ctor copy block, not a complete Player ctor. */
int32_t carFramePlayerBindSettings(CarFramePlayerState* player,const PlayerSettings* settings);

int32_t carFramePlayerWrongWay(const CarFramePlayerState* player, int32_t wasWrongWay);
void carFramePlayerClearPreview(CarFramePlayerState* player);
int32_t carFramePlayerPrepareAirborne(CarFramePlayerState* player);
int32_t carFramePlayerStartState(CarFramePlayerState* player, int32_t state,
                                 int32_t forced, int32_t playAudio, int32_t transitionAudio);
void carFramePlayerEndState(CarFramePlayerState* player, int32_t stopAudio);
void carFramePlayerReconcileInput(CarFramePlayerState* player);
void carFramePlayerSetDrivingStyle(CarFramePlayerState* player,int32_t style);
void carFramePlayerSetInputMode(CarFramePlayerState* player,int32_t mode);
void carFramePlayerServiceReplay(CarFramePlayerState* player);
void carFramePlayerStartScore(CarFramePlayerState* player, int32_t score);
int32_t carFramePlayerTriggerVisibleDrivingLine(CarFramePlayerState* player,uint32_t variant,float duration);
void carFramePlayerAdvance(CarFramePlayerState* player);
void carFramePlayerAdvanceAll(CarFramePlayerRegistry* registry);
int32_t carFramePlayerCrashEligible(const CarFramePlayerState* player);
void carFramePlayerUpdateCrashState(CarFramePlayerState* player);
int32_t carFramePlayerReplayActive(const CarFramePlayerState* player);
void carFramePlayerOnRecovered(CarFramePlayerState* player);
void carFramePlayerRouteInput(CarFramePlayerState* player, int32_t device, int32_t event, uint8_t pressed);
void carFramePlayerDispatchInput(CarFramePlayerState* player, int32_t device, int32_t event, uint8_t pressed);
void carFramePlayerCycleInputProfile(CarFramePlayerState* player, int32_t delta);
void carFramePlayerSetCameraSpring(CarFramePlayerState* player,int32_t enabled);
void carFramePlayerSetLookbackSpeed(CarFramePlayerState* player,float speed);
int32_t carFramePlayerAllDrivingLocked(const CarFramePlayerRegistry* registry);
void carFramePlayerMenuInput(CarFramePlayerState* player, int32_t device, int32_t event, uint8_t pressed);
void carFramePlayerRaceInput(CarFramePlayerState* player, int32_t device, int32_t event, uint8_t pressed);
void carFramePlayerCycleCameraInput(CarFramePlayerState* player, int32_t device, int32_t event, uint8_t pressed);
void carFramePlayerFinish(CarFramePlayerState* player);
void carFramePlayerUpdateCinematicAudio(CarFramePlayerState* player, uint16_t controlCode,
    float strength, float volume, float pitchOffset);
void carFramePlayerStopStateAudio(CarFramePlayerState* player);
int32_t carFramePlayerBindSoundPolicy(CarFramePlayerRegistry* registry);
int32_t carFramePlayerSoundEnabled(const CarFramePlayerRegistry* registry);

/* Native SDK ownership adapters for the source Player+8C private sample.
 * These are not extra original methods or another sample/voice pool. */
WorldAudioResult playerCinematicVoiceStart(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager, int32_t enabled, int32_t playAudio,
    WorldAudioVoiceRef* voice, const char* cueName, float gain, float pitchScale);
WorldAudioResult playerCinematicVoiceStop(WorldAudioRequestQueue* queue,
    int32_t enabled, WorldAudioVoiceRef* voice);
WorldAudioResult playerCinematicVoiceRejectPreview(WorldAudioRequestQueue* queue,
    const WorldAudioManagerState* manager, int32_t enabled);


/* Player camera and input declarations borrow the live service owners. */

typedef struct CarFramePlayerAirborneSettings {
    float lensStart;
    float lensStep;
} CarFramePlayerAirborneSettings;

/* One Player owns these arrays for both phases. Cameras borrow the arrays;
 * the return phase changes this same path instead of replacing its storage. */
typedef struct CarFramePlayerAirbornePath {
    CarCameraPathItems items;
    CarCameraPoint lens[32];
    CarCameraPoint eye[3];
    CarCameraPoint roll[3];
    CarCameraOffsetTrack offset;
    int32_t prepared;
} CarFramePlayerAirbornePath;

void carFramePlayerAirborneSettingsInitialize(CarFramePlayerAirborneSettings* settings);
void carFramePlayerAirbornePathInitialize(CarFramePlayerAirbornePath* path);
CarCinematicResult carFramePlayerAirbornePathEnter(CarFramePlayerAirbornePath* path,
    const CarMovement* movement, const CarRoadPreview* preview, TerrainTopology* terrain,
    const RaceParameters* race, const CarFramePlayerAirborneSettings* settings);
CarCinematicResult carFramePlayerAirbornePathExit(CarFramePlayerAirbornePath* path,
    const CarMovement* movement, TerrainTopology* terrain, const RaceParameters* race,
    const CarFramePlayerAirborneSettings* settings);

/* Borrow the actual settings word. The final game reads/writes it through
 * the configuration stream and reads it again before camera-mode cycling. */
typedef struct CarFramePlayerInputBindings {
    const int32_t* cameraCyclingEnabled;
} CarFramePlayerInputBindings;

struct TrackRouteEditor;
typedef struct CarFramePlayerCameras CarFramePlayerCameras;

/* These pointers borrow the World/track owners. No camera owns another copy
 * of the car list, mutable settings, input router or presentation state. */
typedef struct CarFramePlayerCameraContext {
    CarCinematicServices* services;
    CarPlayerCameraServices* input;
    TerrainTopology* terrain;
    struct WorldSimulation* simulation;
    struct TrackRouteEditor* editor;
    CarViewMoverRegistry* viewMovers;
    const RaceParameters* race;
    const CarChaseCameraVehicle* chaseVehicles;
    size_t vehicleCount;
    const CarMovement* const* movements;
    const CarCameraCatalogue* defaultCatalogue;
    struct CarCameraShake* shake;
    CarChaseCameraSettings* chaseSettings;
    CarOrbitCameraSettings* orbitSettings;
    const CarVelocityCameraSettings* velocitySettings;
    const CarAirborneCameraSettings* airborneSettings;
    const CarFramePlayerAirborneSettings* airbornePathSettings;
    CarFixedCameraCommands* fixedCommands;
    const CarAttachedCameraSettings* attachedSettings;
    const int8_t* viewCategory;
} CarFramePlayerCameraContext;

CarCinematicResult carFramePlayerCamerasInitialize(struct CarFramePlayerState* player,
    const CarFramePlayerCameraContext* context,int32_t viewNumber,
    int32_t overlayViewNumber,int32_t dynamicResponse);
void carFramePlayerCamerasDispose(struct CarFramePlayerState* player);
CarCinematicResult carFramePlayerChangeCamera(struct CarFramePlayerState* player,
    int32_t requestedKind,int32_t remember);
CarCinematicResult carFramePlayerPopCamera(struct CarFramePlayerState* player);
CarCinematicResult carFramePlayerClearCameras(struct CarFramePlayerState* player);
CarCinematicResult carFramePlayerActivateCameras(struct CarFramePlayerState* player);
CarCinematicResult carFramePlayerDeactivateCameras(struct CarFramePlayerState* player);
CarCinematicResult carFramePlayerAdvanceCamera(struct CarFramePlayerState* player);
CarCinematicResult carFramePlayerCamerasSetInputEnabled(struct CarFramePlayerState* player,
    int32_t enabled);
void carFramePlayerResetCurrentCamera(struct CarFramePlayerState* player);
int32_t carFramePlayerCameraKind(const struct CarFramePlayerState* player);
int32_t carFramePlayerSelectCameraView(struct CarFramePlayerState* player,int32_t requested);
CarCinematicBase* carFramePlayerCurrentCamera(struct CarFramePlayerState* player);
CarCinematicView* carFramePlayerCurrentCameraView(struct CarFramePlayerState* player);
CarViewMover* carFramePlayerCarViewMover(const struct CarFramePlayerState* player);
float carFramePlayerCurrentCameraDuration(const struct CarFramePlayerState* player);
int32_t carFramePlayerPrepareCrashCamera(struct CarFramePlayerState* player);
int32_t carFramePlayerValidateCurrentCrashCamera(struct CarFramePlayerState* player);
CarCinematicResult carFramePlayerSetAttachedCamera(struct CarFramePlayerState* player,int32_t enabled);
CarCinematicResult carFramePlayerSetScriptedCamera(struct CarFramePlayerState* player,
    CarMovement* movement);
CarCinematicResult carFramePlayerSetCountdownCamera(struct CarFramePlayerState* player);

typedef enum CarPlayerCameraFamily {
    CAR_PLAYER_CAMERA_NONE,
    CAR_PLAYER_CAMERA_CHASE,
    CAR_PLAYER_CAMERA_FIXED,
    CAR_PLAYER_CAMERA_DRIVE,
    CAR_PLAYER_CAMERA_ORBIT,
    CAR_PLAYER_CAMERA_ZONE,
    CAR_PLAYER_CAMERA_CRASH,
    CAR_PLAYER_CAMERA_AIRBORNE,
    CAR_PLAYER_CAMERA_VELOCITY
} CarPlayerCameraFamily;

/* The family replaces concrete C++ virtual dispatch. It differs from the
 * source kind: both Zone and Crash actually publish kind12. */
typedef struct CarPlayerOwnedCamera {
    CarPlayerCameraFamily family;
    CarCinematicBase* base;
    union {
        CarChaseCamera* chase;
        CarFixedCamera* fixed;
        CarDriveCamera* drive;
        CarOrbitCamera* orbit;
        CarZoneFreezeCamera* zone;
        CarCrashCamera* crash;
        CarAirborneCamera* airborne;
        CarVelocityCamera* velocity;
    } owner;
} CarPlayerOwnedCamera;

struct CarFramePlayerCameras {
    const CarFramePlayerCameraContext* context;
    CarPlayerOwnedCamera current;
    CarPlayerOwnedCamera suspended[2];
    CarViewMover* scripted;
    CarViewMover* countdown;
    CarViewMover* replay;
    CarAttachedCamera* overlay;
    CarFramePlayerAirbornePath airbornePath;
    CarRoadPreview crashPreview;
    int32_t viewNumber;
    int32_t overlayViewNumber;
    int32_t dynamicResponse;
};

/* Concrete Player owners provide these dependencies. No camera identifier
 * array or substitute input-mode table is kept by the state transition. */
void carFramePlayerReconcileInput(CarFramePlayerState* player);
int32_t carFramePlayerPrepareCrashCamera(CarFramePlayerState* player);
int32_t carFramePlayerValidateCurrentCrashCamera(CarFramePlayerState* player);

int32_t carFramePlayerClearTransientPresentation(CarFramePlayerState* player);

struct CarFramePlayerState;

float carFramePlayerAverageSpeedRatio(const struct CarFramePlayerState* player);
int32_t carFramePlayerExceededSpeedRatio(const struct CarFramePlayerState* player, float limit);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef struct CarPlayerScoreAnimation {
    float position[2];
    float positionIn[2];
    float positionOut[2];
    float scale[2];
    float scaleIn[2];
    float color[4];
    float colorChange[4];
} CarPlayerScoreAnimation;

int32_t carFramePlayerScoreStringValue(const char* text);
void carFramePlayerAddToScore(CarFramePlayerState* player, const char* text);
void carFramePlayerStartScoreAnimation(CarFramePlayerState* player,
    const CarPlayerScoreAnimation* animation);
int32_t carFramePlayerReadTotalScore(const CarFramePlayerState* player, int32_t* total);
void carFramePlayerDisposeScore(CarFramePlayerState* player);
void carFramePlayerDisposeVisibleDrivingLine(CarFramePlayerState* player);
struct CarPlayerVisibleDrivingLine* carPlayerVisibleDrivingLineCreate(
    CarFramePlayerState* player,int32_t variant,float duration);
const Spline* carFramePlayerVisibleDrivingLineSpline(const CarFramePlayerState* player);
int32_t carFramePlayerRenderVisibleDrivingLine(CarFramePlayerState* player,
    PreviewCometMesh* mesh);
int32_t carFramePlayerRenderPlayers(CarFramePlayerRegistry* registry,
    const CarCinematicScreen* screen,NativeEcstasyFrame* frame);
int32_t carFramePlayerServiceVisibleDrivingLine(CarFramePlayerState* player);
int32_t carFramePlayerPredetermineNis(CarFramePlayerState* player);
int32_t carFramePlayerInitializeNisSelection(CarFramePlayerState* player);
int32_t carFramePlayerCreateRandomNisArray(CarFramePlayerState* player, int32_t kind);
int32_t carFramePlayerFindNextNisIndex(CarFramePlayerState* player,
    const CarMovement* first, const CarMovement* second, int32_t kind);
int32_t carFramePlayerLoadNextNis(CarFramePlayerState* player,
    const CarMovement* first, const CarMovement* second, int32_t pulloverCount);
int32_t carFramePlayerPlayNextNis(CarFramePlayerState* player,
    CarMovement* first, CarMovement* second, CarPlayerNisCompletion completion,
    int32_t completionIndex);
/* Sole native request cell for PAL32FCE4. Its renderer consumer, not NIS,
 * clears it; no unrelated replay-active state is represented here. */
extern int32_t carGlareResetRequested;
void carFramePlayerDisposeNisSelection(CarFramePlayerState* player);

#ifdef __cplusplus
}
#include "NIS.hpp"

struct CarPlayerNisSelection {
    int32_t cursors[2] = {-1, -1};
    int32_t infoIndices[2][32]{};
    int32_t takeIndices[2][32]{};
};

/* The individual beta definitions of these three tiny methods are in this
 * header (Player.cpp and CopMgr.cpp instantiate the same inline form). */
inline CarNisResult carNisOverlayStartTicket(CarNisOverlay* overlay)
{
    if (!overlay) return CAR_NIS_INVALID_BINDING;
    overlay->mode = 1;
    return carNisOverlayStart(overlay);
}

inline CarNisResult carNisOverlayStartArrest(CarNisOverlay* overlay)
{
    if (!overlay) return CAR_NIS_INVALID_BINDING;
    overlay->mode = 2;
    return carNisOverlayStart(overlay);
}

inline CarNisResult carNisOverlayStartBusted(CarNisOverlay* overlay)
{
    if (!overlay) return CAR_NIS_INVALID_BINDING;
    overlay->mode = 3;
    return carNisOverlayStart(overlay);
}

namespace hp2 {
class FlailerResources;
class LanguageManager;
}
struct CarFramePlayerCameraContext;
struct TrackResourceInfo;
namespace hp2 {
/* All services exist before World/Car creation. Later camera binding borrows
 * the real race owners without rebuilding Player or its NisManager. */
struct PlayerConstructionBindings {
    CarFramePlayerRegistry* registry{};
    const RaceParameters* parameters{};
    GameInputRouter* input{};
    FlailerResources* flailer{};
    FlailerRenderManager* renderManager{};
    LanguageManager* language{};
    const GameTime* clock{};
    NisManagerBindings nis;
    const class CarInfoResource* cars{};
    const TrackResourceInfo* track{};
};
CarFramePlayerState* CreatePlayer(int32_t port,int32_t seat,const PlayerConstructionBindings& bindings);
bool DestroyPlayer(CarFramePlayerState* player);
// Terminal adapters retire partial native owners while their providers live.
std::exception_ptr DetachPlayerForShutdown(CarFramePlayerState* player) noexcept;
std::exception_ptr DestroyPlayerForShutdown(CarFramePlayerState* player) noexcept;
bool BindPlayerForRace(CarFramePlayerState& player,const CarFramePlayerCameraContext& cameras);
/* A binding borrows one actual authored HUD screen and the same language
 * manager used by the rest of the frontend. HUD mask changes rebind objects
 * in the same Player owner and never reset its accumulated score. */
struct PlayerScoreBindings {
    FlailerResources* resources;
    LanguageManager* language;
    uint32_t screenKey;
    bool enabled;
};
bool BindPlayerScore(CarFramePlayerState& player, const PlayerScoreBindings& bindings);
// Port-side update keeps player scoring and cameras with a separate HUD owner.
void AdvancePlayersWithoutFlailerHud(CarFramePlayerRegistry* registry);
}
#endif

#endif
