#ifndef NFSHP2_RACE_COORDINATOR_HPP
#define NFSHP2_RACE_COORDINATOR_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/CameraMover.hpp"
#include "Flags.hpp"

#ifdef __cplusplus
#include <exception>
extern "C" {
#endif

/* Native declarations of the individually proved RaceCoordinator owner.
 * GameSession retains its existing public name and is the sole state owner.
 * No original packed PS2 object layout is imported. */
typedef struct RaceAction {
    int32_t kind;
    /* PAL RCMessage14 transports a ReplaySnapshot* in the same source word.
     * Native messages preserve that pointer at the host width; ordinary
     * integer messages retain their checked signed32 values. */
    intptr_t argument;
} RaceAction;

/* Storage is borrowed from the race owner. Queue insertion preserves the
 * original deferred action order; enqueuing does not execute a transition. */
typedef struct RaceActionQueue {
    RaceAction* entries;
    size_t capacity;
    size_t readIndex;
    size_t count;
    int32_t overflow;
} RaceActionQueue;

void raceActionsInitialize(RaceActionQueue* queue, RaceAction* storage, size_t capacity);
int32_t raceActionsEnqueue(RaceActionQueue* queue, int32_t kind, intptr_t argument);
int32_t raceActionsTake(RaceActionQueue* queue, RaceAction* action);


struct CarFramePlayerRegistry;
typedef struct GameSessionCameras GameSessionCameras;

/* The session owns its coordinator. Its environment borrows the same player
 * registry and camera coordinators that the World frame already updates. */
typedef struct GameSessionCameraContext {
    CarViewMoverRegistry* coordinators;
    const struct CarFramePlayerRegistry* players;
} GameSessionCameraContext;

GameSessionCameras* gameSessionCamerasCreate(const GameSessionCameraContext* context,
    CarViewMoverResult* result);
void gameSessionCamerasDispose(GameSessionCameras* cameras);
CarViewMoverResult gameSessionCameraActivate(GameSessionCameras* cameras);
void gameSessionCameraDeactivate(GameSessionCameras* cameras);
CarViewMover* gameSessionCurrentCamera(const GameSessionCameras* cameras);


/* The application session, race configuration and live race are different
 * owners. Replay and input producers append to this session's action queue. */
/* A borrowed view of the same session-owned selection record. Its seat is
 * assigned by that owner's constructor and read by screen radial blur. */
typedef struct GameSessionViewSelection {
    int32_t seat;
} GameSessionViewSelection;

struct RaceCoordinatorBindings;
struct RaceCoordinatorOwnedState;
typedef enum RaceCoordinatorResult {
    RC_RESULT_OK,
    RC_RESULT_INVALID_BINDING,
    RC_RESULT_QUEUE_CAPACITY,
    RC_RESULT_UNIMPLEMENTED_STATE_DEPENDENCY,
    RC_RESULT_CAMERA_FAILURE,
    RC_RESULT_RACE_FAILURE,
    RC_RESULT_RESOURCE_FAILURE
} RaceCoordinatorResult;

typedef struct GameSession {
    int32_t state;
    RaceActionQueue actions;
    GameSessionViewSelection* viewSelection;
    struct GameSessionCameras* cameras;
    /* PAL owner+14/+18. The full Flyby/PreRace callers prove these writes;
     * the complete state dispatcher remains open, rather than simulated. */
    int32_t pausingJoystickPort;
    int32_t stateBeforePause;
    /* Actual source+0C reentrancy flag. SendMessage and PostMessage share
     * exactly the one actions queue above. */
    int32_t processingMessages;
    struct RaceCoordinatorBindings* bindings;
    CarViewMover* flybyCamera;   /* source+20, owned */
    CarViewMover* preRaceCamera; /* source+24, owned */
    /* Native checked boundary result: never a replacement RC state. */
    RaceCoordinatorResult result;
    /* One native allocation owns the real Replay/Race/World lifetimes.
     * Players stay in the existing canonical creation-order registry. */
    struct RaceCoordinatorOwnedState* ownedState
#ifdef __cplusplus
        = nullptr
#endif
        ;
} GameSession;


/* Actual method-pointer table at PAL2F0BF0, read by ExecuteQueuedMessages.
 * These values describe the existing state; they do not supply missing
 * state callbacks or claim that application startup is implemented. */
typedef enum RaceCoordinatorState {
    RC_STATE_NONE=0, RC_STATE_CAMERA_FLYBY=1, RC_STATE_PRE_RACE=2,
    RC_STATE_RACE_IN_PROGRESS=3, RC_STATE_INSTANT_REPLAY=4,
    RC_STATE_AUTO_REPLAY=5, RC_STATE_PAUSE=6, RC_STATE_POST_RACE=7,
    RC_STATE_AI_DEMO=8, RC_STATE_REPLAY_DEMO=9
} RaceCoordinatorState;

/* Complete original header methods1832A8/174E00. The bounded native FIFO
 * reports capacity failure; neither request changes state synchronously. */
static inline int32_t raceCoordinatorRequestPause(GameSession* session,int32_t argument)
{
    return raceActionsEnqueue(&session->actions,5,argument);
}

static inline int32_t raceCoordinatorRequestUnPause(GameSession* session,int32_t reason)
{
    return raceActionsEnqueue(&session->actions,6,reason);
}

void raceCoordinatorJoystickEvent(GameSession* session,int32_t event);
int32_t raceCoordinatorExecuteQueuedMessages(GameSession* session);
int32_t raceCoordinatorSendMessage(GameSession* session,int32_t kind,intptr_t argument);
int32_t raceCoordinatorChangeState(GameSession* session,int32_t state);
int32_t raceCoordinatorCheckForDemoDiscTimeout(GameSession* session);

#ifdef __cplusplus
}

#include <memory>

namespace hp2 { class ResourceLoader; struct PlayerConstructionBindings; struct WorldConstructionBindings; }
namespace hp2 { struct FlailerUiState; class StartRaceScreens; class PauseMenu; class DemoModeReplay; }
class DemoDiscManager;
class ReplayManagerOwner;
struct RaceFrame;
struct World;
struct GameTime;
struct WorldAudioRequestQueue;
struct TrackScenarioCatalog;
struct TrackRoute;
struct TerrainTopology;
struct RaceParameters;
struct RaceFrameServices;
struct CarFramePlayerCameraContext;
struct WorldImpactAudio;
struct Sound3dSampleRegistry;
struct CarSoundBindings;
struct WorldParticleSystem;
/* One borrowed environment for actual existing owners; no function-pointer
 * state table, replacement World, artificial Player, or test callbacks. */
struct RaceCoordinatorBindings {
    RaceFrame* race{};
    World* world{};
    CarViewMoverRegistry* cameraRegistry{};
    hp2::StartRaceScreens* startScreens{};
    hp2::ResourceLoader* resources{};
    const int32_t* specialPursuit{};
    DemoDiscManager* demoDisc{};
    const GameTime* clock{};
    /* The original live ReplayManager owner and the actual host allocation
     * budget sample consumed by MarkTheBeginningOfTime. These pointers do
     * not introduce a replay state/clock or a fixed successful budget. */
    ReplayManagerOwner* replayOwner{};
    const size_t* replayAvailableBytes{};
    hp2::PauseMenu* pauseMenu{};
};
/* Host ownership binding/retirement, zero additional original bodies.
 * This does not claim the still-open full RC constructor/InitForRace. */
int32_t raceCoordinatorBindDispatch(GameSession& session,RaceCoordinatorBindings& bindings);
void raceCoordinatorReleaseScreenCameras(GameSession& session);
// Source RC+28/+2C borrowers used by ReplayScreen.Tick and MainLoop.
bool raceCoordinatorHasEndGameScreens(const GameSession& session) noexcept;
void raceCoordinatorUpdateReplayScreen(GameSession& session);
/* Full original methods149CB8/149D78 once supplied with the same live
 * Player owners. InitForRace's outer creation graph remains independent. */
int32_t raceCoordinatorInitPlayers(GameSession& session,int32_t aiDemo);
int32_t raceCoordinatorCleanupPlayers(GameSession& session);
/* Full PAL InitCars14A2C8/CleanupCars14A5F0. The supplied World owns the
 * actual factory/services graph; the catalog and parameters are borrowed
 * from the already loaded track and frontend selection. */
void raceCoordinatorInitCars(World& world,const RaceParameters& parameters,
    const TrackScenarioCatalog& catalog);
void raceCoordinatorCleanupCars(World& world);
/* These are borrowed live resources/configurations, not alternate game
 * owners. InitForRace constructs its own Replay, Race and World in source
 * order and publishes their exact pointers to the existing service views. */
struct RaceCoordinatorInitBindings {
    RaceParameters* parameters{};
    const hp2::PlayerConstructionBindings* players{};
    const hp2::WorldConstructionBindings* world{};
    RaceFrameServices* raceServices{};
    RaceCoordinatorBindings* dispatch{};
    const CarFramePlayerCameraContext* playerCameras{};
    const TrackScenarioCatalog* catalog{};
    const TrackRoute* route{};
    const int32_t* skipStartSequence{};
    hp2::DemoModeReplay* demoReplay{};
    WorldImpactAudio* collisionAudio{};
    Sound3dSampleRegistry* oneShotAudio{};
    CarSoundBindings* sounds{};
    WorldParticleSystem* particles{}; // Same application WorldEffects owner, never a copied pool.
};
/* Ordinary final startup graph. The AI-demo RenderEggs teardown dependency
 * is reported explicitly until its actual renderer owner is available. */
void raceCoordinatorInitForRace(GameSession& session,const RaceCoordinatorInitBindings& bindings);
/* Native exception-safe retirement of the actual objects constructed above.
 * This is not acceptance of the still-open full post-race/result lifecycle. */
void raceCoordinatorDisposeOwnedRace(GameSession& session);
/* Native lifetime of the individually proved source constructor149738 and
 * destructor149858. The sixteen message entries are the original source
 * pool capacity; Session() is the only coordinator state owner. */
class RaceCoordinatorOwner final {
public:
    RaceCoordinatorOwner(const RaceCoordinatorInitBindings& bindings,
        GameSessionViewSelection& selection,const GameSessionCameraContext& cameras);
    ~RaceCoordinatorOwner() noexcept;
    // Checked ordinary gameplay retirement. A failed operation leaves the
    // owner available for terminal Close while its providers remain alive.
    void Retire();
    std::exception_ptr Close() noexcept;
    RaceCoordinatorOwner(const RaceCoordinatorOwner&)=delete;
    RaceCoordinatorOwner& operator=(const RaceCoordinatorOwner&)=delete;
    GameSession& Session() noexcept;
    const GameSession& Session() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
int32_t raceCoordinatorTheRaceHasFinished(GameSession& session);
void raceCoordinatorDoSnapshot(GameSession& session,struct ReplaySnapshot& snapshot);
/* Borrowed live UI/race/audio owners. No countdown clock or voice mirror. */
void raceCoordinatorAnimateCountdownElement(hp2::FlailerUiState& ui,
    WorldAudioRequestQueue& audio,float start,float end,float progress,
    uint32_t screen,uint32_t element);
void raceCoordinatorDisplayCountdown(hp2::FlailerUiState& ui,
    WorldAudioRequestQueue& audio,const RaceFrame& race,int32_t specialPursuit,int32_t number);
int32_t raceCoordinatorCalculateRacerPositions(const TrackScenarioCatalog& catalog,
    const TrackRoute& route,TerrainTopology& terrain,int32_t reverse,
    int32_t count,float (*positions)[4]);
#endif
#endif
