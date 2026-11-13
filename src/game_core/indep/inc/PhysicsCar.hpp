#ifndef NFSHP2_PHYSICS_CAR_HPP
#define NFSHP2_PHYSICS_CAR_HPP

#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Sound3d.hpp"
#include "indep/inc/CarDamage.hpp"
#include <stddef.h>
#include <stdint.h>
typedef struct CarMotionState CarMotionState;
struct PursuitManager;
typedef struct CarRecoveryPolicy {
    int32_t registryGuard;
    int32_t positiveEntryGate;
    const struct PursuitManager *pursuit;
} CarRecoveryPolicy;
#include "CarState.hpp"
#include "indep/inc/ComponentPhysics.hpp"
#include "indep/inc/RigidBody.hpp"
#include "indep/inc/Suspension.hpp"
#include "indep/inc/Aerodynamics.hpp"
#include "indep/inc/Engine.hpp"
#include "indep/inc/Wheel.hpp"
#include "indep/inc/Wheel.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Skids.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct YawTuning {
    float lowSpeedYawBoost;
    float highSpeedYawBoost;
    float lowSpeedSpeed;
    float highSpeedSpeed;
    float maxYawBonus;
} YawTuning;

void physicsInitializeYawTuning(YawTuning* tuning);
float physicsYawFrictionBoost(const YawTuning* tuning, int scaleDelta,
                              float* magnitude, float yaw, float handbrake,
                              float speed, float baseCoefficient,
                              float coefficientDelta, float coefficientScale);

// Component ownership follows the game's original physics object families.
// Host object sizes can be wider than the original serialized objects.
typedef enum PhysicsObjectType {
    PHYSICS_OBJECT_RIGID_BODY,
    PHYSICS_OBJECT_STEERING,
    PHYSICS_OBJECT_WHEEL,
    PHYSICS_OBJECT_ENGINE,
    PHYSICS_OBJECT_DRIVE_TRAIN,
    PHYSICS_OBJECT_AERODYNAMICS,
    PHYSICS_OBJECT_SUSPENSION,
    PHYSICS_OBJECT_MELLOVATOR,
    PHYSICS_OBJECT_MELLOW_MOVER,
    PHYSICS_OBJECT_NULL_MOVER,
    PHYSICS_OBJECT_TYPE_COUNT
} PhysicsObjectType;

typedef enum PhysicsPoolStatus {
    PHYSICS_POOL_OK,
    PHYSICS_POOL_INVALID_ARGUMENT,
    PHYSICS_POOL_OVERFLOW,
    PHYSICS_POOL_OUT_OF_MEMORY,
    PHYSICS_POOL_EXHAUSTED,
    PHYSICS_POOL_FOREIGN_POINTER,
    PHYSICS_POOL_DOUBLE_RELEASE,
    PHYSICS_POOL_STILL_IN_USE
} PhysicsPoolStatus;

typedef struct PhysicsObjectInfo {
    const char* name;
    size_t originalBytes;
    size_t allocationAlignment;
    uint32_t instancesPerVehicle;
    uint32_t permanentExtraInstances;
} PhysicsObjectInfo;

typedef struct PhysicsPoolUsage {
    uint32_t capacity;
    uint32_t allocated;
    uint32_t peakAllocated;
    size_t nativeObjectBytes;
    size_t slotStride;
} PhysicsPoolUsage;

typedef struct PhysicsPools PhysicsPools;

const PhysicsObjectInfo* physicsObjectInfo(PhysicsObjectType type);

// The source engine allocates its ten object families together. nativeBytes
// contains the actual host sizeof(T) per family; 0 uses the original 32-bit
// size until that family's restored native type is available.
PhysicsPools* physicsPoolsCreate(uint32_t vehicleCapacity,
                                 const size_t nativeBytes[PHYSICS_OBJECT_TYPE_COUNT],
                                 PhysicsPoolStatus* status);

// Explicit release refuses to destroy storage containing live objects.
PhysicsPoolStatus physicsPoolsClose(PhysicsPools* pools);
void physicsPoolsDestroy(PhysicsPools* pools);

void* physicsPoolAcquire(PhysicsPools* pools, PhysicsObjectType type,
                         PhysicsPoolStatus* status);
PhysicsPoolStatus physicsPoolRelease(PhysicsPools* pools, PhysicsObjectType type,
                                      void* object);
PhysicsPoolStatus physicsPoolGetUsage(const PhysicsPools* pools,
                                       PhysicsObjectType type,
                                       PhysicsPoolUsage* out);


typedef struct Mellovator Mellovator;
typedef struct MellowMover MellowMover;
typedef struct NullMover NullMover;

// All ten source families are allocated together. Recovered owners reserve
// their actual native sizeof, including wider host pointers.
PhysicsPools* physicsObjectsCreatePools(uint32_t vehicleCapacity, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsClosePools(PhysicsPools* pools);

// These acquire raw storage, like the original per-class allocation operators.
// Initialize a component before use. Its configuration and linked components
// remain owned by the caller. Remove borrowed system references before release.
RigidBody* physicsObjectsAcquireRigidBody(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseRigidBody(PhysicsPools* pools, RigidBody* object);
Steering* physicsObjectsAcquireSteering(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseSteering(PhysicsPools* pools, Steering* object);
Wheel* physicsObjectsAcquireWheel(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseWheel(PhysicsPools* pools, Wheel* object);
Engine* physicsObjectsAcquireEngine(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseEngine(PhysicsPools* pools, Engine* object);
DriveTrain* physicsObjectsAcquireDriveTrain(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseDriveTrain(PhysicsPools* pools, DriveTrain* object);
Aerodynamics* physicsObjectsAcquireAerodynamics(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseAerodynamics(PhysicsPools* pools, Aerodynamics* object);
Suspension* physicsObjectsAcquireSuspension(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseSuspension(PhysicsPools* pools, Suspension* object);
Mellovator* physicsObjectsAcquireMellovator(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseMellovator(PhysicsPools* pools, Mellovator* object);
MellowMover* physicsObjectsAcquireMellowMover(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseMellowMover(PhysicsPools* pools, MellowMover* object);
NullMover* physicsObjectsAcquireNullMover(PhysicsPools* pools, PhysicsPoolStatus* status);
PhysicsPoolStatus physicsObjectsReleaseNullMover(PhysicsPools* pools, NullMover* object);

RigidBody* physicsObjectsCreateRigidBody(PhysicsPools* pools, const RigidBodyInfo* info,
                                        const RigidBodyTuning* tuning, PhysicsPoolStatus* status);
Engine* physicsObjectsCreateEngine(PhysicsPools* pools, const EngineInfo* info,
                                   const EngineVehicleState* vehicle, const EngineTuning* tuning,
                                   PhysicsPoolStatus* status);
DriveTrain* physicsObjectsCreateDriveTrain(PhysicsPools* pools, const DriveTrainInfo* info,
                                          Engine* engine, Wheel* const wheels[4],
                                          DriveTrainVehicleState* vehicle,
                                          const DriveTrainTuning* tuning, PhysicsPoolStatus* status);
Steering* physicsObjectsCreateSteering(PhysicsPools* pools, const SteeringGeometry* geometry,
                                      PhysicsPoolStatus* status);
Aerodynamics* physicsObjectsCreateAerodynamics(PhysicsPools* pools, const AerodynamicsInfo* info,
                                               RigidBody* body, const AerodynamicsTuning* tuning,
                                               PhysicsPoolStatus* status);

typedef struct Car Car;
typedef struct CarControls CarControls;

typedef struct PhysicsCarInfo {
    const RigidBodyInfo* body;
    const float* centerOfMass;
    const SteeringGeometry* steering;
    const EngineInfo* engine;
    const DriveTrainInfo* driveTrain;
    const AerodynamicsInfo* aerodynamics;
    const WheelTorqueInfo* wheels;
    const WheelTireInfo* tires;
    const WheelBrakeInfo* brakes;
    const SuspensionInfo* suspension;
    float wheelSideOffset[4];
    float forwardSpeedLimit;
    float ordinaryYawLimit;
    float pursuitYawLimit;
    float preloadDownforceScale;
    float yawBaseCoefficient;
    float yawCollisionCoefficient;
    int32_t pursuitConfiguration;
    int32_t modelCode;
    int32_t routeVehicleClass;
    float steeringRangeDegrees;
    int32_t bodyCategory;
    /* Borrows the canonical authored CarInfo F0 vector, separately from
     * the physical center of mass at110. */
    const float* driverAnimationOffset;
} PhysicsCarInfo;

typedef struct PhysicsCarTuning {
    float nonPursuitGripScale;
    float maximumStep;
    float tractionEntrySlip;
    float tractionFullSlip;
    float tractionReleaseSlip;
    float tractionMaximumSpeed;
    float tractionMinimumBrakeScale;
    float tractionHoldSeconds;
    float tractionDwellSeconds;
    float tractionEntryHeadingError;
    float tractionReleaseHeadingError;
    float slipDisplayScale;
    int32_t useCenterOfMassForLoad;
    int32_t useDownforceForTireLoad;
} PhysicsCarTuning;

extern const PhysicsCarTuning physicsCarDefaultTuning;

struct RaceParameters;
/* One live World owns the last integration step. Rewinding one participant
 * changes the step consumed by subsequent collision preparation, including
 * preparation of a different Car. This is native owner state, not a timer. */
typedef struct PhysicsStepContext {
    float lastSubstepElapsed;
} PhysicsStepContext;

/* Stable contact resources have no independent elapsed value. A force pass
 * receives a transient WheelContactContext built from the World step owner. */
typedef struct PhysicsCarContactBindings {
    TerrainSurface* terrain;
    const TerrainMaterial* materials;
    size_t materialCount;
    const WheelContactTuning* tuning;
} PhysicsCarContactBindings;

typedef struct PhysicsCarEnvironment {
    const struct RaceParameters* settings;
    const GameTime* clock;
    PhysicsCarContactBindings contact;
    PhysicsStepContext* stepContext;
    const PhysicsCarTuning* tuning;
    CarRecoveryPolicy recovery;
    /* Borrows pCurrentRace->state (PAL Move gp-5700, source Race+0).
     * World and every copied Car environment retain this same live cell. */
    const int32_t* raceStage;
    int32_t requestUpwardImpulse;
    WheelTrailStore* wheelTrails;
} PhysicsCarEnvironment;

WheelContactContext physicsCarContactContext(const PhysicsCarEnvironment* environment);

typedef struct PhysicsCar {
    PhysicsCarInfo info;
    CarControls* controls;
    Car* owner;
    CarMotionState* ownerMotion;
    VehicleState* state;
    PhysicsCarEnvironment* environment;
    PhysicsPools* pools;
    RigidBody* body;
    Steering* steering;
    Wheel* wheels[4];
    Suspension* suspension[4];
    Engine* engine;
    DriveTrain* driveTrain;
    Aerodynamics* aerodynamics;
    EngineVehicleState engineView;
    DriveTrainVehicleState driveTrainView;
    WheelVehicleState wheelView;
    WheelStepContext wheelSteps[4];
    ComponentSystem components;
    ComponentBinding bindings[11];
    float derivatives[47];
    int32_t momentumHoldTicks;
    int32_t momentumHoldStart;
    int32_t momentumHold;
    int32_t momentumHoldArmed;
    int32_t momentumHoldStarts;
    int32_t uprightRecoveryStart;
    float gripScale;
    float brakeRecovery;
    WheelTrail* wheelTrails[4];
    WheelTrailStore localTrailStore;
    WheelTrail localTrails[16];
} PhysicsCar;
struct ReplaySnapshot;
void physicsCarSnapshot(PhysicsCar* car,struct ReplaySnapshot* snapshot);

PhysicsPoolStatus physicsCarInitialize(PhysicsCar* car, const PhysicsCarInfo* info,
                                       CarControls* controls, Car* owner,
                                       CarMotionState* ownerMotion, VehicleState* state,
                                       PhysicsCarEnvironment* environment, PhysicsPools* pools);
void physicsCarDispose(PhysicsCar* car);
void physicsCarPrepareAxleLoads(PhysicsCar* car);
void physicsCarLoadState(PhysicsCar* car);
void physicsCarPrepare(PhysicsCar* car);
int32_t physicsCarRestState(const PhysicsCar* car);
void physicsCarResolveSuspensionImpulses(PhysicsCar* car);
void physicsCarCollectForces(PhysicsCar* car);
void physicsCarCollectWheelForces(PhysicsCar* car);
void physicsCarPublishState(PhysicsCar* car);
void physicsCarSetGripScale(PhysicsCar* car, float scale);
void physicsCarDampAirborneRotation(PhysicsCar* car);
float physicsCarMove(PhysicsCar* car, float elapsed);

#ifdef __cplusplus
}
#endif
#endif
