#ifndef NFSHP2_CAR_HPP
#define NFSHP2_CAR_HPP

#include <stdint.h>
#ifdef __cplusplus
#include <cstdint>
#endif
#include "indep/inc/RaceParameters.hpp"

#ifdef __cplusplus
namespace hp2 {
struct CarConstructionBindings; struct CarDamageDisplayBindings;
extern "C++" { struct ApplicationFrontendFlow; struct FrontendPlayerManager; }
}
struct GameInputRouter;
struct GameSession;
struct CarFramePlayerRegistry;
// Host call binding for the original process-global rumble/wheel services.
// All five entries borrow the live owners. No event queue, copied option,
// animation state or hardware-completion state is retained here.
struct CarMovementFeedbackBindings {
    GameInputRouter* input{};
    const hp2::ApplicationFrontendFlow* flow{};
    const hp2::FrontendPlayerManager* frontend{};
    const CarFramePlayerRegistry* players{};
    const GameSession* session{};
};
extern "C" {
#endif

float physicsMaximumPairedAverage(const float values[4]);
float physicsQuarterOfFourFlags(const uint8_t values[4]);

// Native game objects use host pointers. The PS2's packed disk and object
// layouts are decoded at the boundaries; they are not aliased onto host data.


// Vehicle motion methods belong to the game, not the graphics backend.
// The active movement component is responsible for the original simulation.
typedef void (*CarMoveMethod)(void* movement, float elapsed);
typedef void (*CarControlsMethod)(void* controls, float elapsed);
struct Car;
struct RaceCarEntry;
struct RaceDriverInfo;
struct CarAppearance;
struct CarOwnedState;
struct CarMovement;
struct CarDriver;
struct CarFrame;
struct ReplayManager;
struct PursuitManager;
struct Chopper;
struct CarFramePlayerRegistry;
struct CarCinematicScreen;
typedef float (*CarInteractionMethod)(void* context, struct Car* car);
typedef void (*CarControllerMethod)(void* controller, float elapsed);

typedef struct Car {
    struct Car* next;
    struct Car* previous;
    uint16_t tableIndex;
    int16_t driverNumber;
    float position[4];
    // Native equivalents of the original movement-component dispatch and
    // countdown. No approximate acceleration or collision is implemented.
    void* movement;
    CarMoveMethod move;
    uint32_t driveCountdown;
    void* interactionContext;
    CarInteractionMethod interaction;
    void* controller;
    CarControllerMethod controllerThink;
    float interactionFactor;
    void* controls;
    CarControlsMethod updateControls;
    struct RaceCarEntry* raceEntry;
    struct CarDriverVisual* visual;
    struct RaceDriverInfo* driverInfo;
    struct CarAppearance* appearance;
    /* Car constructor enables the AnimatedDriver render pass. Pursuit
     * completion disables it independently of the visual animation owner. */
    int32_t driverVisible;
    /* Independent final-Car cells45C/460. The first retains the pursuit
     * damage response; the second feeds PhysicsCar::SetBoost. */
    float pursuitDamageResponseScale;
    float physicsBoost;
    /* PAL+890: DamageAbove writes this signed camera bearing before its
     * NIS/damage-display gates. Player::DrawTracker reads the same cell. */
    float trackerBearing;
    /* Shared C/C++ ABI. A borrowed zero-initialized facade/sentinel owns none. */
    struct CarOwnedState* ownedState
#ifdef __cplusplus
        = nullptr
#endif
        ;
#ifdef __cplusplus
    std::uint64_t presentationId{};
    Car() noexcept = default;
    Car(RaceDriverInfo& selection,const float* initialPosition,uint16_t heading,
        float routeDistanceHint,const hp2::CarConstructionBindings& bindings);
    ~Car();
    Car(const Car&) = delete;
    Car& operator=(const Car&) = delete;
    struct CarMovement* Movement() noexcept;
    struct CarDriver* Driver() noexcept;
    struct CarFrame* Frame() noexcept;
    void GetRenderingWheelAndBrakeMatrices(float wheels[4][16],float brakes[4][16],
        int32_t viewNumber);
    void DamageAbove(const float eye[3],const float forward[3],
        const hp2::CarDamageDisplayBindings& display);
    void DamageCap(bool pursued,float damage,float distanceScale,
        const hp2::CarDamageDisplayBindings& display);
    // Publish the same late-created World services to retained and future
    // concrete controller bindings; no gameplay state is copied here.
    void BindWorldServices(struct ReplayManager* replay,struct PursuitManager* pursuit,
        struct Chopper* chopper) noexcept;
#endif
} Car;

typedef enum CarAppearanceResult {
    CAR_APPEARANCE_OK,
    CAR_APPEARANCE_INVALID_BINDING,
    CAR_APPEARANCE_INVALID_SELECTION
} CarAppearanceResult;

CarAppearanceResult carSwitchSkin(Car* car, int32_t skinNumber);
struct ReplaySnapshot;
void carDoSnapshot(Car* car,struct ReplaySnapshot* snapshot);

typedef enum CarEffectResult {
    CAR_EFFECT_OK,
    CAR_EFFECT_INVALID_BINDING,
    CAR_EFFECT_PARTICLE_FAILURE
} CarEffectResult;

struct CarMovement;
CarEffectResult carTriggerEffect(struct CarMovement* movement,int32_t effect,
    float strength,int32_t key,const float* velocity);

int32_t carCalculateDebugChecksum(const Car* car);
/* Checked native binding of the actual pursuit stores;0 extra ELF bodies. */
int32_t carSetPursuitResponse(Car* car, float damageResponseScale, float physicsBoost);

/* Drive updates the actual control owner before the collision participant
 * pass advances movement. These are separate original dispatch operations. */
typedef enum CarDriveResult {
    CAR_DRIVE_OK = 0,
    CAR_DRIVE_MISSING_CONTROLS = 1
} CarDriveResult;
CarDriveResult carDrive(Car* car, float elapsed);

typedef enum CarThinkResult {
    CAR_THINK_OK = 0,
    CAR_THINK_MISSING_COMPONENT = 1
} CarThinkResult;
CarThinkResult carThink(Car* car, float elapsed);

struct CarMovement;
struct CarDriver;
int32_t carViewMoverOpposesRoad(const struct CarMovement* movement,const struct CarDriver* driver);

#ifdef __cplusplus
}
#endif


#include <stdbool.h>
#include <stddef.h>
#include "indep/inc/CollisionBody.hpp"
#include "CarState.hpp"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct TerrainSurface TerrainSurface;
struct CarDamage;
struct Car;
struct PursuitManager;
struct PursuitCollisionGroup;

typedef struct CarMotionState {
    CarCollisionState *collision;
    /* Predicates read this owner's current driverInfo. Snapshot/setup can
     * change the selection independently of the model's body category. */
    const struct Car* owner;
    int32_t inputCategory;
    const struct CarDamage* damage;
    int32_t windowStartTicks;
    float windowDurationSeconds;
    int32_t transitionFlag;
    int32_t transitionCounter;
    int32_t recoveryRequested;
    uint32_t wheelExclusionMask;
    uint32_t wheelEffectsFlags;
    uint32_t wheelInclusionMask;
    uint8_t disabledWheelMask;
    int8_t actionEnabled;
} CarMotionState;

/* These gates preserve the original race policy without assigning it a new
 * game-mode meaning. Initialize them from the current race configuration. */
typedef struct CarRecoveryPolicy CarRecoveryPolicy;

/* Host binding adapter only: identify the same actual Car by its borrowed
 * motion owner, then call the sole original group-query implementation.
 * Returns1 on a valid lookup (including no matching pursuit),0 on a missing
 * native owner. Canonical Manager/Car queries are in PursuitManager.h. */
int32_t carRecoveryFindForMotion(const struct PursuitManager *manager,
    const CarMotionState *motion, struct PursuitCollisionGroup **group);
void carMotionRequestRecovery(CarMotionState *state, const CarRecoveryPolicy *policy);
void carMotionBeginTransition(CarMotionState *state, int32_t nowTicks);
void carMotionSetWindow(CarMotionState *state, float seconds, int32_t nowTicks);
void carMotionClearWindow(CarMotionState *state);
bool carMotionWindowActive(const CarMotionState *state, int32_t nowTicks);
bool carMotionWheelSelected(const CarMotionState *state, uint32_t wheel, int32_t nowTicks);
bool collisionShapeAllowsContact(const CollisionShape *shape);
bool carCollisionAllowsControl(const CarCollisionState *collision);
bool carMotionCanControl(const CarMotionState *state, int32_t nowTicks);
int32_t carMotionTransitionFlag(const CarMotionState *state);
bool carMotionActionAllowed(const CarMotionState *state);
bool carMotionClassActionAllowed(const CarMotionState *state);
float carMotionPostCollisionYaw(const CarMotionState *state, int32_t nowTicks);
bool carMotionHighSpeedSideways(const VehicleState *state);
void carMotionRecoverBelowGround(CarMotionState *state, const VehicleState *vehicle,
                                 TerrainSurface *terrain, const CarRecoveryPolicy *policy);
#ifdef __cplusplus
}
#endif
#include "indep/inc/RigidBody.hpp"
#include "indep/inc/PhysicsCar.hpp"
#include "indep/inc/MellowMover.hpp"
#include "indep/inc/CarMover.hpp"
#include "indep/inc/TrafficCar.hpp"
#include "indep/inc/DeliveryMover.hpp"
typedef struct CopMover CopMover;
#include "indep/inc/TrackPath.hpp"
#include "indep/inc/Table.hpp"
#include "indep/inc/TrackRoute.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CarMovement CarMovement;
struct CarDamage;
struct CollisionSystem;
struct CollisionParticipant;
struct PursuitCollisionContext;
struct CarFrame;
struct BeltMover;
struct BeltMoverBindings;

typedef enum CarMovementResult {
    CAR_MOVEMENT_OK,
    CAR_MOVEMENT_INVALID_BINDING,
    CAR_MOVEMENT_UNSUPPORTED_MODE,
    CAR_MOVEMENT_UNSUPPORTED_DRIVER,
    CAR_MOVEMENT_ALLOCATION_FAILURE,
    CAR_MOVEMENT_PRESENTATION_FAILURE,
    CAR_MOVEMENT_MISSING_ROUTE,
    CAR_MOVEMENT_MISSING_RACE_METRIC,
    CAR_MOVEMENT_MISSING_RECOVERY_DATA,
    CAR_MOVEMENT_PURSUIT_FAILURE
} CarMovementResult;

/* These are live route-owner references, never generated replacement poses.
 * Driver profiles 0, 1 and 4 are the basic and route/player controller
 * families used by the recovered Physics/Mellow domain. */
typedef struct CarMovementDriver {
    int32_t profile;
    const CarRecoveryPose* recoveryPose;
    float recoveryDelay;
    float previousSpeed;
} CarMovementDriver;

typedef struct CarMovementPlayer {
    int32_t seat;
    int32_t controllerPort;
    int32_t steeringFeedbackEnabled;
    int32_t steeringFeedbackBusy;
} CarMovementPlayer;

typedef struct CarRaceMetric {
    const Car* car;
    float position;
} CarRaceMetric;

typedef struct CarMovementRace {
    int32_t enabled;
    int32_t mode;
    const CarRaceMetric* metrics;
    size_t metricCount;
    const Car* references[2];
    const KeyedCurve* performance;
    const int32_t* difficulty;
} CarMovementRace;

typedef struct CarMovementBindings {
    const PhysicsCarInfo* info;
    CarControls* controls;
    Car* owner;
    CarMotionState* ownerMotion;
    VehicleState* state;
    PhysicsCarEnvironment* environment;
    PhysicsPools* pools;
    TrackRoute* route;
    const CarRecoveryPointRegistry* recoveryPoints;
    CarMovementDriver* driver;
    CarMovementPlayer* player;
    const CarMovementRace* race;
    const int32_t* applicationState;
    const struct CarMovementFeedbackBindings* feedback;
    struct CarDamage* damage;
    struct PursuitCollisionContext* pursuit;
    const struct BeltMoverBindings* belt;
} CarMovementBindings;

/* Owns only the active mover lifecycle. Configurations, state, route,
 * collision owner, damage, driver and presentation services remain borrowed. */
struct CarMovement {
    PhysicsCarInfo info;
    CarMovementBindings bindings;
    PhysicsCar* physics;
    MellowMover* mellow;
    NullMover* inactive;
    struct CollisionSystem* collisionSystem;
    struct CollisionParticipant* collisionParticipant;
    TrackRouteLocation routeLocation;
    int32_t mode;
    int32_t initialized;
    int32_t publishedGear;
    float frictionBoost;
    float displaySpeed;
    CarMovementResult result;
    struct CarFrame* frame;
    TrafficCar* traffic;
    DeliveryMover* delivery;
    CopMover* cop;
    struct BeltMover* belt;
};

CarMovementResult carMovementInitialize(CarMovement* movement, const CarMovementBindings* bindings, int32_t mode);
void carMovementDispose(CarMovement* movement);
CarMovementResult carMovementSetMode(CarMovement* movement, int32_t mode);
void carMovementSetGripOverride(CarMovement* movement, float value);
void carMovementSetPublishedGear(CarMovement* movement, int32_t gear);
void carMovementUpdateRacePerformance(CarMovement* movement);
void carMovementRefreshRecoveryPose(CarMovement* movement);
void carMovementAlignTerrainPose(const PhysicsCarInfo* info, TerrainSurface* terrain,
                                 float orientation[16], float position[4], uint16_t heading);
void carMovementUpdateTerrainPose(CarMovement* movement);
CarMovementResult carMovementPlace(CarMovement* movement, const float position[3], uint16_t heading, float speed);
CarMovementResult carMovementPlaceAndNotify(CarMovement* movement, struct CarDriver* driver,
    const float position[3], uint16_t heading, float speed);
void carMovementRecover(CarMovement* movement, const CarRecoveryGroup* group, const CarRecoveryPose* pose,
                        float speed, float directionScale, int32_t setWindow, int32_t holdFrames);
void carMovementDefaultCollisionNotify(CarMovement* movement);
void carMovementBindCollision(CarMovement* movement, struct CollisionSystem* system,
                               struct CollisionParticipant* participant);

RigidBody* carMovementCollisionBody(CarMovement* movement);
void carMovementPrepareCollision(CarMovement* movement);
int32_t carMovementUpdateSound(CarMovement* movement);
VehicleState* carMovementState(CarMovement* movement);
CarMotionState* carMovementOwnerMotion(CarMovement* movement);
struct CarDamage* carMovementDamage(CarMovement* movement);
float carMovementCapture(CarMovement* movement, float elapsed);
void carMovementNotifyCollision(CarMovement* movement, CarMovement* other);
void carMovementRecordCollisionImpact(CarMovement* movement, const float impulse[4],
                                      const float lever[4], float impulseMagnitude);
void carMovementRecordCollisionDamage(CarMovement* movement, const float lever[4], float impulseMagnitude);
void carMovementRecordCollisionFeedback(CarMovement* movement, const float impulse[4], float impulseMagnitude);
void carMovementSteeringImpact(CarMovement* movement, const float localImpulse[4], float impulseMagnitude);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace hp2 { struct CarRenderBindings; }
void carAssignAnimatedDriver(Car* car,const hp2::CarRenderBindings* bindings);
#endif


#ifdef __cplusplus
extern "C" {
#endif
struct World;
typedef struct CarInteractionTuning CarInteractionTuning;
typedef struct CarFrame CarFrame;
typedef struct CarFramePlayerState CarFramePlayerState;
typedef struct CarFramePresentation CarFramePresentation;
typedef struct WorldEventDispatcher CarSpatialIndex;
struct CarDriver;
struct ReplayManager;
struct CarSoundState;
struct FernShakerSystem;
struct RaceParameters;
typedef struct CarFrameCarReplay {
    int16_t restoreMode;
} CarFrameCarReplay;

typedef struct CarFrameState {
    float previousPosition[4];
    float previousVelocity[4];
    float previousAngularVelocity[4];
    float lastGroundPoint[2];
    float airborneDeadline;
    int32_t blinkStart;
    float blinkInterval;
    float blinkOffset;
    float blinkDuration;
    int32_t blinkPhase;
    int32_t blinkVisualEnabled;
    int32_t blinkControlEnabled;
    int32_t blinkForced;
    int32_t blinkVisible;
    int32_t signalMode;
    int32_t signalEnabled;
    int8_t disabledWheelCount;
    int32_t disabledWheelStartTicks;
    uint8_t previousSurfaceMask;
    uint8_t wheelSlipFrames[4];
    int32_t signalWindowStart;
    int32_t signalWindowEnd;
    int32_t signalWindowLocked;
    int32_t looseSurfaceHighlight;
    int32_t specialEffectRequested;
    int32_t feedbackDeadline;
    /* Final Car+80C: initialized by130E28, retained by130FF0 snapshot. */
    uint32_t retainedSnapshotFlag;
} CarFrameState;

typedef enum CarFrameResult {
    CAR_FRAME_OK,
    CAR_FRAME_INVALID_BINDING,
    CAR_FRAME_ALLOCATION_FAILURE,
    CAR_FRAME_EVENT_CAPACITY,
    CAR_FRAME_MISSING_PLAYER_OWNER,
    CAR_FRAME_MISSING_SPATIAL_OWNER,
    CAR_FRAME_MOVEMENT_FAILURE,
    CAR_FRAME_PLAYER_TRANSITION_FAILURE,
    CAR_FRAME_PARTICLE_FAILURE
} CarFrameResult;

/* All owners are borrowed. The frame table follows the live World car table;
 * no independent list or copied controller state replaces that owner. */
typedef struct CarFrameEnvironment {
    struct World* world;
    const GameTime* clock;
    CarFrame* const* frames;
    size_t frameCount;
    const CarInteractionTuning* interactionTuning;
    CarFramePlayerState* player;
    CarFramePresentation* presentation;
    CarSpatialIndex* spatial;
    int32_t routeVehicleClass;
    struct CarDriver* driver;
    struct ReplayManager* replayManager;
    struct CarSoundState* sound;
    const CarFrameCarReplay* carReplay;
    struct FernShakerSystem* fernShakers;
    const struct RaceParameters* settings;
} CarFrameEnvironment;

CarFrame* carFrameCreate(CarMovement* movement, const CarFrameEnvironment* environment,
                         CarFrameResult* result);
void carFrameDispose(CarFrame* frame);
CarMovement* carFrameMovement(CarFrame* frame);
CarFrameResult carFrameGetResult(const CarFrame* frame);
CarFrameState* carFrameState(CarFrame* frame);
void carFrameResetPeakMaterials(VehicleState* state);
float carFramePredictGroundClearance(CarFrame* frame, float* duration, float* maximumClearance);

CarFrameResult carFrameSetupForceFeedback(CarFrame* frame);
CarFrameResult carFrameUpdateForceFeedback(CarFrame* frame, int32_t port);
CarFrameResult carFramePreStep(CarFrame* frame, float elapsed);
CarFrameResult carFramePostStep(CarFrame* frame, float elapsed);
CarFrameResult carFrameUpdateBlink(CarFrame* frame);
void carFrameBeginBlink(CarFrame* frame, int32_t visual, int32_t control, int32_t phase,
                         float interval, float offset, float duration);
CarFrameResult carFrameDisableWheels(CarFrame* frame, uint8_t mask);
void carFrameClearDisabledWheels(CarFrame* frame);
CarFrameResult carFrameInteractionSetup(CarFrame* frame);
CarFrameResult carFrameUpdateAccelerationFeedback(CarFrame* frame);
CarFrameResult carFrameUpdateSurfaceFeedback(CarFrame* frame);
CarFrameResult carFrameUpdateSurfaceEffects(CarFrame* frame, float nearestPlayerDistance);
void carFrameExtendSignalWindow(CarFrame* frame, int32_t start, int32_t end);
void carFrameSetSpecialEffect(CarFrame* frame, int32_t requested);

/* Concrete adapter for the existing Car think interface. It invokes the
 * recovered nearby-car calculation and records a checked frame result. */
float carFrameInteractionFactor(void* context, Car* car);

struct CarFrame {
    CarMovement* movement;
    CarFrameEnvironment environment;
    CarFrameState state;
    CarFrameResult result;
};

#ifdef __cplusplus
}
struct CarDriverEnvironment;
struct CarSoundBindings;
struct WorldEventDispatcher;
namespace hp2 {
struct CarDefinition;
struct CarRenderBindings;
class CollisionResource;
class ResourceRegistry;
class EcstasyFrame;
/* All arguments borrow the live application, Player and resource owners.
 * No flag, damage level or camera pose is retained in this call binding. */
struct CarDamageDisplayBindings {
    ResourceRegistry& resources;
    const RaceParameters& parameters;
    const int32_t& sessionState;
    const ::CarFramePlayerRegistry& players;
    const ::PursuitManager* pursuit;
    const ::CarCinematicScreen& view;
    EcstasyFrame& frame;
};
/* Borrowed construction inputs. Templates supply configuration/services;
 * their Car/state/controls owners are never adopted. World/Track/resources,
 * settings and pools must outlive the constructed Car. */
struct CarConstructionBindings {
    const CarDefinition* definition{};
    TerrainTopology* topology{};
    const CarMovementBindings* movement{};
    const PhysicsCarEnvironment* physics{};
    const CarFrameEnvironment* frame{};
    const CarDriverEnvironment* driver{};
    const CarRenderBindings* render{};
    CollisionSystem* collision{};
    CollisionResource* const* collisionResources{};
    size_t collisionResourceCount{};
    const CarSoundBindings* sound{};
    WorldEventDispatcher* events{};
};
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef struct World World;
typedef struct CarDriveDisplayTuning {
    float speedThresholds[6];
    float rpmBands[7][2];
    float reverseThreshold;
    float speedStep;
    float rpmStep;
    float speedOverride;
    float rpmOverride;
    float maximumSpeedMargin;
} CarDriveDisplayTuning;

extern const CarDriveDisplayTuning carDriveDisplayDefaultTuning;
void carMovementUpdatePowertrainDisplay(CarMovement* movement, const CarDriveDisplayTuning* tuning);

struct CarFrame;

typedef struct CarInteractionTuning {
    KeyedCurve speedDistance;
    SampledCurve distanceWeight;
    float alignmentOffset;
    float alignmentScale;
    float accumulationLimit;
} CarInteractionTuning;

typedef struct CarInteractionRegistry {
    const World* world;
    struct CarFrame* const* frames;
    size_t frameCount;
    const CarInteractionTuning* tuning;
} CarInteractionRegistry;

typedef enum CarInteractionResult {
    CAR_INTERACTION_OK,
    CAR_INTERACTION_INVALID_BINDING
} CarInteractionResult;

extern const CarInteractionTuning carInteractionDefaultTuning;

float carInteractionPair(const CarMovement* car, const CarMovement* other,
                         const CarInteractionTuning* tuning);
CarInteractionResult carInteractionFactor(const CarInteractionRegistry* registry,
                                           const CarMovement* car, float* factor);
#ifdef __cplusplus
}
#endif
#endif
