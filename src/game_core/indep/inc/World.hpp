#ifndef NFSHP2_WORLD_HPP
#define NFSHP2_WORLD_HPP

#include <stdint.h>
#ifdef __cplusplus
#include <cstdint>
#endif
#include <stddef.h>

/* Native public views of the services owned/borrowed by World. Concrete
 * implementations retain their individually proved source owners. */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum WorldServicesResult {
    WORLD_SERVICES_OK,
    WORLD_SERVICES_INVALID_BINDING,
    WORLD_SERVICES_CAPACITY,
    WORLD_SERVICES_OWNER_FAILURE
} WorldServicesResult;

typedef struct Chopper Chopper;
typedef struct TrafficScheduler TrafficScheduler;
typedef struct PursuitManager PursuitManager;
typedef struct RaceFrame RaceFrame;
typedef struct WorldEventDispatcher WorldEventDispatcher;
typedef struct WorldParticleSystem WorldParticleSystem;
typedef struct TrackGraph TrackGraph;
typedef struct PursuitLandmarkCatalog PursuitLandmarkCatalog;
struct RaceParameters;
struct World;
struct GameTime;
struct CarRecoveryPointRegistry;
struct CarFramePlayerRegistry;

typedef struct PursuitLandmark {
    int32_t vertexCount;
    float vertices[6][2];
    uint32_t tag;
    int32_t uncloakRoads;
} PursuitLandmark;

/* The34530 catalog is a distinct track resource from scenario landmarks.
 * The source world wrapper can run with no CopManager, while its track
 * graph/landmark and world owners remain live. */
typedef struct WorldPursuitServices {
    PursuitManager* manager;
    struct World* world;
    TrackGraph* trackGraph;
    const PursuitLandmarkCatalog* landmarks;
} WorldPursuitServices;

PursuitLandmarkCatalog* pursuitLandmarksCreate(const uint8_t* track, size_t size);
void pursuitLandmarksDispose(PursuitLandmarkCatalog* catalog);
size_t pursuitLandmarkCount(const PursuitLandmarkCatalog* catalog);
const PursuitLandmark* pursuitLandmarkAt(const PursuitLandmarkCatalog* catalog, size_t index);
const PursuitLandmark* pursuitLandmarkFind(const PursuitLandmarkCatalog* catalog, const float point[2]);
int32_t pursuitPointInPolygon(const float point[2], const float vertices[][2], int32_t count);

/* The graph owns adjacency/annotations and borrows the one road registry.
 * Uncloak mutates its actual connection flags, which road queries also read. */
TrackGraph* trackGraphCreate(const struct CarRecoveryPointRegistry* road);
void trackGraphDispose(TrackGraph* graph);
void trackGraphUncloakAllEdges(TrackGraph* graph);
int32_t trackGraphHasCloakedEdges(const TrackGraph* graph);
int32_t trackGraphFindVertex(const TrackGraph* graph, int32_t groupId, int32_t poseIndex);
int32_t trackGraphFindEdge(const TrackGraph* graph, int32_t groupId, int32_t poseIndex);
size_t trackGraphVertexCount(const TrackGraph* graph);
size_t trackGraphEdgeCount(const TrackGraph* graph);
/* Read-only diagnostics of the same live objects, not alternate owners. */
int32_t trackGraphInspectVertex(const TrackGraph* graph, size_t index,
    int32_t values[8], float* routeDistance);
int32_t trackGraphInspectEdge(const TrackGraph* graph, size_t index,
    int32_t values[8], const int32_t** roadBlocks, size_t* roadBlockCount,
    const int32_t** speedTraps, size_t* speedTrapCount);

typedef struct TrafficSchedulerBindings {
    struct World* world;
    const struct GameTime* clock;
    const struct RaceParameters* parameters;
    RaceFrame* race;
    struct CarFramePlayerRegistry* players;
    const struct CarRecoveryPointRegistry* road;
    const int32_t* alternatePursuitSkin;
} TrafficSchedulerBindings;

void trafficSchedulerDispose(TrafficScheduler* scheduler);

WorldServicesResult trafficSchedulerUpdate(TrafficScheduler* scheduler,float elapsed);
WorldServicesResult pursuitManagerPreThink(WorldPursuitServices* services);
WorldServicesResult chopperAdvance(Chopper* chopper,float elapsed);
WorldServicesResult worldEventsDispatch(WorldEventDispatcher* dispatcher);
WorldServicesResult worldParticlesAdvance(WorldParticleSystem* particles,float elapsed);
WorldServicesResult raceFrameUpdate(RaceFrame* race);

#ifdef __cplusplus
}
namespace hp2 { class CarInfoResource; }
TrafficScheduler* trafficSchedulerCreate(const TrafficSchedulerBindings* bindings,
    const hp2::CarInfoResource* carInfo);
#endif

#include "indep/inc/Timer.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/Replay.hpp"

#ifdef __cplusplus
namespace hp2 { struct WorldConstructionBindings; }
extern "C" {
#endif

struct WorldOwnedState;

typedef void (*WorldTimeoutCallback)(void* context);

typedef struct WorldTiming {
    float pausedSeconds;
    float replayWaitSeconds;
    float timeScale;
    float scaleDuration;
    WorldTimeoutCallback callback;
    void* callbackContext;
} WorldTiming;

typedef struct WorldStepIO {
    void* context;
    int (*readQuantized)(void* context, int bits, int channel, int32_t* value);
    void (*writeQuantized)(void* context, int32_t value, int bits, int channel);
    uint32_t (*getTicker)(void* context);
    float (*tickerMilliseconds)(void* context, uint32_t from, uint32_t to);
} WorldStepIO;

typedef struct WorldStepOptions {
    int recording;
    int replaying;
    int disableReplayWaitA;
    int disableReplayWaitB;
    int tweakerPause;
} WorldStepOptions;

typedef enum WorldStepResult {
    WORLD_STEP_OK = 0,
    WORLD_STEP_MISSING_REPLAY_SOURCE = 1,
    WORLD_STEP_MISSING_REPLAY_CLOCK = 2,
    WORLD_STEP_MISSING_RECORD_SINK = 3
} WorldStepResult;

void worldTimingInitialize(WorldTiming* world);
void worldSetTimeScale(WorldTiming* world, float scale, float duration,
                       WorldTimeoutCallback callback, void* context);
void worldResetTimeScale(WorldTiming* world);
void worldSetPaused(WorldTiming* world, float seconds,
                    WorldTimeoutCallback callback, void* context);
/* elapsed is the same GameTime.realTimeElapsed read at PAL GP-4C8C. */
void worldUpdatePause(WorldTiming* world, float elapsed);
/* Same mutable source global33028C, shared with Player input. */
extern int32_t worldInputPauseGate;
int worldIsPaused(const WorldTiming* world);
WorldStepResult worldGetTimestep(WorldTiming* world, const GameTime* time,
                                 const WorldStepOptions* options,
                                 const WorldStepIO* io, float* elapsed);
/* Same recorded input step and time-scale transaction, with pacing owned
 * by the native application scheduler. recordedElapsed receives the
 * unscaled quantized step for both live and restored updates; no wait runs. */
WorldStepResult worldGetNativeTimestep(WorldTiming* world, const GameTime* time,
                                       const WorldStepOptions* options,
                                       const WorldStepIO* io, float* elapsed,
                                       float* recordedElapsed);

// A Windows-native world owns a list of cars and the simulation's ordered
// table. Only verified members are present; other game objects will join this
// class as their original behavior is recovered.
#define WORLD_MAX_CARS 14

typedef struct World {
    Car carList;
    Car* carTable[WORLD_MAX_CARS];
    float carSnapshots[WORLD_MAX_CARS][4];
    int32_t totalCars;
    int32_t firstGroupEnd;
    int32_t secondGroupEnd;
    int32_t thirdGroupEnd;
    float playerPositions[2][4];
    float playerTrackDistances[2];
    WorldTiming timing;
    uint32_t frameCounter;
    uint8_t playerSlotFlags[2][40];
    float playerSlotPosition[2][40][2];
    int32_t lastPlayerSlot;
    uint32_t simulationRandomState;
    /* World+8 is the ordered AI-car membership view. Cars remain owned by
     * the main car list; these pointers never contain another vehicle state. */
    Car* aiCars[8];
    uint32_t aiCarCount;
    struct WorldOwnedState* ownedState
#ifdef __cplusplus
        = nullptr
#endif
        ;
#ifdef __cplusplus
    std::uint64_t presentationGeneration{};
    World() noexcept = default;
    explicit World(const hp2::WorldConstructionBindings& bindings);
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;
#endif
} World;

typedef enum WorldSimulationResult {
    WORLD_SIMULATION_OK,
    WORLD_SIMULATION_INVALID_BINDING,
    WORLD_SIMULATION_INVALID_CAR_TABLE,
    WORLD_SIMULATION_REPLAY_FAILURE,
    WORLD_SIMULATION_DRIVER_FAILURE,
    WORLD_SIMULATION_MOVEMENT_FAILURE,
    WORLD_SIMULATION_FRAME_FAILURE,
    WORLD_SIMULATION_COLLISION_FAILURE,
    WORLD_SIMULATION_SERVICE_FAILURE,
    WORLD_SIMULATION_PLAYER_FAILURE,
    WORLD_SIMULATION_NONFINITE_STATE
} WorldSimulationResult;

/* First rejected live owner/field. Strings name the observed state; this
 * diagnostic never repairs state, refreshes a cache or identifies a writer. */
typedef struct WorldStateFailure {
    int32_t car;          /* World.carTable index, or -1 for the clock/world. */
    const char* field;    /* Static field name, or null when validation passed. */
    int32_t element;      /* Array lane, or -1 for a scalar/binding. */
} WorldStateFailure;

/* This is the live world binding, not another copy of its car/driver state.
 * The parallel frame and driver views follow World.carTable exactly. GameTime
 * advances in the outer loop; this timestep consumes the published clock. */
typedef struct WorldSimulation {
    World* world;
    const GameTime* clock;
    struct CollisionSystem* collisions;
    struct CarFrame* const* frames;
    struct CarDriver* const* drivers;
    size_t frameCount;
    struct CarFramePlayerRegistry* players;
    struct WorldReplay* replay;
    struct TrafficScheduler* traffic;
    struct PursuitManager* pursuit;
    struct Chopper* chopper;
    struct WorldEventDispatcher* events;
    struct WorldParticleSystem* particles;
    struct RaceFrame* race;
    WorldSimulationResult result;
    int32_t failedCar;
    struct WorldPursuitServices* pursuitServices;
    struct TrackRouteEditor* editor; /* Borrowed live Drive-camera/input producer. */
    WorldStateFailure stateFailure;
} WorldSimulation;

WorldSimulationResult worldDoTimestep(WorldSimulation* simulation, float elapsed);
CollisionSystemResult worldServicesAdvanceParticipants(struct CollisionSystem* system, float elapsed);


typedef enum WorldCarTableResult {
    WORLD_CARS_OK = 0,
    WORLD_CARS_TOO_MANY = 1
} WorldCarTableResult;

void worldInitialize(World* world);
uint32_t worldRandomBounded(World* world, uint32_t bound);
float worldRandomFloat(World* world,float maximum);
uint32_t worldAdvanceFrame(World* world);
void worldRecordPlayerSlot(World* world, int32_t index,
                           uint8_t flagA, uint8_t flagB,
                           const float positionA[2], const float positionB[2]);
void worldAddCar(World* world, Car* car);
void worldRemoveCar(Car* car);
void worldRegisterAiCar(World* world, Car* car);
void worldUnregisterAiCar(World* world, Car* car);
WorldCarTableResult worldBuildCarTable(World* world);
Car* worldGetCarByDriverNumber(const World* world, int32_t driverNumber);
Car* worldGetCarByIndex(const World* world, int32_t index);
void worldSnapshotCarPointer(const World* world, ReplaySnapshot* snapshot, Car** value);
void worldDoSnapshot(World* world,ReplaySnapshot* snapshot);
/* Required native replay invariant, independent of reference hash policy.
 * Checks the live clock and Car pose/velocity owners without speed clamps. */
int worldValidateReplayState(const World* world,const GameTime* clock);
int worldInspectReplayState(const World* world,const GameTime* clock,WorldStateFailure* failure);
/* The two World callback words from PAL14D1BC/14D1CC. Native logical
 * references and an authored Player address have explicit, distinct formats. */
void worldSnapshotTimingCallback(WorldTiming* timing,
    struct CarFramePlayerRegistry* players,ReplaySnapshot* snapshot);
/* Source snapshot children operate on the same live World-owned objects. */
struct TrafficScheduler;
struct TrackGraph;
void trafficSchedulerDoSnapshot(struct TrafficScheduler* scheduler, ReplaySnapshot* snapshot);
void trackGraphDoSnapshot(struct TrackGraph* graph, ReplaySnapshot* snapshot);
float worldGetDistanceToClosestPlayer2D(const World* world, const float position[2]);
float worldGetDistanceToClosestPlayer3D(const World* world, const float position[3]);
float worldGetClosestSignedTrackDelta(const World* world, float reference, float trackLength);
uint16_t worldCalculateDebugChecksum(World* world, uint32_t seed,
                                     const void* optionalContribution);
int worldCarIsBefore(const Car* first, const Car* second);
void worldSetCurrent(World* world);
Car* worldFindCurrentCarByDriverNumber(int32_t driverNumber);


typedef enum WorldCarOrderResult {
    WORLD_CAR_ORDER_OK = 0,
    WORLD_CAR_ORDER_INVALID_TABLE,
    WORLD_CAR_ORDER_MISSING_COMPONENT
} WorldCarOrderResult;

// The four car-thinking passes that occur in the original world timestep.
// This function is deliberately not the whole world timestep: traffic,
// collision and movement processing must execute in their original places.
WorldCarOrderResult worldThinkCars(World* world, float elapsed);
// Original all-car drive pass. The complete world timestep performs other
// systems between thinking and driving; this method does not skip them.
WorldCarOrderResult worldDriveCars(World* world, float elapsed);

/* Stable intrusive spatial list; independent of the grouped simulation table. */
void worldSortCarList(World* world);

#ifdef __cplusplus
}

struct WorldSimulation;
struct ApplicationLaunchOptions;
struct TrafficScheduler;
struct TrafficSchedulerBindings;
struct WorldObjectRegistry;
struct ChopperMunitionEffects;
struct WorldAudioEffects;
struct WorldAudioRequestQueue;
struct WorldAudioMessages;
struct WorldSpeechManager;
struct PursuitGroupBindings;
struct PursuitManager;
struct PursuitCollisionEntry;
struct CarCameraShake;
struct Scenery;
struct AerialNavigation;
struct Chopper;
namespace hp2 {
class ResourceRegistry;
class CollisionObjectData;
class ClanResources;
class SkidResources;
/* Loaded track/application objects outlive this World. All mutable Car,
 * collision, particle, speech and input state stays in those exact owners. */
struct WorldConstructionBindings {
    GameTime* clock{};
    const RaceParameters* parameters{};
    const ApplicationLaunchOptions* launch{};
    WorldSimulation* simulation{};
    const CarConstructionBindings* cars{};
    const TrafficSchedulerBindings* traffic{};
    CarCameraShake* cameraShake{};
    Scenery* scenery{};
    WorldObjectRegistry* objects{};
    ResourceRegistry* resources{};
    CollisionObjectData* sceneryObjects{};
    CollisionObjectData* inGameObjects{};
    ChopperMunitionEffects* effects{};
    WorldAudioEffects* audio{};
    WorldAudioRequestQueue* audioRequests{};
    WorldAudioMessages* messages{};
    WorldSpeechManager* speech{};
    const int32_t* impactPresentationMode{};
    int32_t* impactPresentationRequested{};
    const PursuitGroupBindings* pursuit{};
    const uint8_t* trackBytes{};
    size_t trackSize{};
    AerialNavigation* navigation{};
    /* Exact loaded TrackInfo+148; independent of scenario metadata. */
};
const CarConstructionBindings& WorldCarBindings(const World& world);
Car* WorldCreateCar(World& world, RaceDriverInfo& driver, const float* position,
    uint16_t heading, float routeDistance);
Car* WorldSpawnTraffic(World& world, RaceDriverInfo& driver);
PursuitCollisionEntry* WorldCreateCop(World& world, RaceDriverInfo* driver, bool playerCop);
Chopper* WorldCreateChopper(World& world);
void WorldDestroyChopper(World& world);
PursuitManager* WorldPursuitManager(World& world);
ClanResources& WorldClans(World& world);
SkidResources& WorldSkids(World& world);
WorldObjectRegistry& WorldObjects(World& world);
/* Matching the canonical table publishes borrowed views for the native
 * timestep. It neither allocates vehicles nor copies their state. */
void WorldSynchronizeCarViews(World& world);
void WorldPrepareCarCleanup(World& world);
Car* TrafficPlaceNewCar(TrafficScheduler* scheduler,RaceDriverInfo& driver);
}
#endif

#endif
