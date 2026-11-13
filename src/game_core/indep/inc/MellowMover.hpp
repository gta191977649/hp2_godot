#ifndef NFSHP2_MELLOWMOVER_HPP
#define NFSHP2_MELLOWMOVER_HPP

// Native declarations use the PC runtime layout.

#include "indep/inc/Engine.hpp"
#include "indep/inc/Suspension.hpp"
#include "indep/inc/Track.hpp"
#include "CarState.hpp"
#include "indep/inc/Wheel.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MellovatorInfo {
    const EngineInfo* engine;
    const DriveTrainInfo* driveTrain;
    const WheelTorqueInfo* wheels;
    const SuspensionInfo* suspension;
    const float* centerOfMass;
    float preloadDownforceScale;
} MellovatorInfo;

typedef struct MellovatorState {
    float wheelAngularVelocity[4];
} MellovatorState;

/* This component integrates wheel rotation and applies its original simplified
 * tyre/suspension model to an actual RigidBody. Terrain queries use the same
 * decoded topology as the full PhysicsCar suspension. */
typedef struct Mellovator {
    MellovatorState motion;
    MellovatorInfo info;
    RigidBody* body;
    TerrainSurface* terrain;
    float travel[4];
    float travelSpeed[4];
    float suspensionForce[4];
    int32_t touchingGround[4];
    float tireForce[4][2];
    float slip[4][2];
    int32_t slipping[4];
    int32_t rotationLocked[4];
    float compressionExcess[4];
    float groundNormal[4][4];
    float engineRpm;
    int32_t currentGear;
    float throttle;
    float brake;
    uint16_t steeringAngle;
    int32_t desiredGear;
    float boost;
    float frictionBoost;
    float preload[4];
    SampledCurve torqueCurve;
    SampledCurve brakingCurve;
    float upshiftRpm[9];
    float downshiftRpm[9];
} Mellovator;
struct ReplaySnapshot;
void mellovatorSnapshot(Mellovator* component, struct ReplaySnapshot* snapshot);

Mellovator* mellovatorInitialize(Mellovator* component, const MellovatorInfo* info,
                                RigidBody* body, TerrainSurface* terrain);
void mellovatorCalibrate(Mellovator* component, float downforce, int useCenterOfMass);
void mellovatorSetWheelMotion(Mellovator* component, const float angularVelocity[4]);
static inline void mellovatorRestoreState(Mellovator* component, const MellovatorState* state)
{ component->motion = *state; }
static inline void mellovatorSaveState(const Mellovator* component, MellovatorState* state)
{ *state = component->motion; }
static inline MellovatorState* mellovatorState(Mellovator* component)
{ return &component->motion; }
static inline int32_t mellovatorStateSize(const Mellovator* component)
{ (void)component; return 4; }
float* mellovatorComputeDerivatives(const Mellovator* component, float derivatives[4]);
void mellovatorGroundHeightsAndPositions(Mellovator* component, float positions[4][4], float heights[4]);
void mellovatorComputeSuspensionForces(Mellovator* component, float elapsed);
void mellovatorPrepareStep(Mellovator* component, float elapsed);
void mellovatorResolveForces(Mellovator* component);
float mellovatorResolveBottomOutImpulses(Mellovator* component, VehicleState* state,
                                        float impulseScale);

#ifdef __cplusplus
}
#endif

#include "indep/inc/PhysicsCar.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MellowMover {
    PhysicsCarInfo info;
    CarControls* controls;
    Car* owner;
    CarMotionState* ownerMotion;
    VehicleState* state;
    PhysicsCarEnvironment* environment;
    PhysicsPools* pools;
    RigidBody* body;
    Mellovator* suspension;
    Aerodynamics* aerodynamics;
    Wheel* wheels[4];
    WheelVehicleState wheelView;
    ComponentSystem components;
    ComponentBinding bindings[2];
    float derivatives[33];
    WheelTrail* wheelTrails[4];
    WheelTrailStore localTrailStore;
    WheelTrail localTrails[16];
} MellowMover;

PhysicsPoolStatus mellowMoverInitialize(MellowMover* mover, const PhysicsCarInfo* info,
                                        CarControls* controls, Car* owner,
                                        CarMotionState* ownerMotion, VehicleState* state,
                                        PhysicsCarEnvironment* environment, PhysicsPools* pools);
void mellowMoverDispose(MellowMover* mover);
void mellowMoverSnapshot(MellowMover* mover,struct ReplaySnapshot* snapshot);
void mellowMoverCalibrate(MellowMover* mover);
void mellowMoverLoadState(MellowMover* mover);
void mellowMoverPublishState(MellowMover* mover, float elapsed);
void mellowMoverPrepare(MellowMover* mover);
void mellowMoverCollectForces(MellowMover* mover);
void mellowMoverSetBoost(MellowMover* mover, float boost);
void mellowMoverStabilizeAirborne(MellowMover* mover);
float mellowMoverMove(MellowMover* mover, float elapsed);

#ifdef __cplusplus
}
#endif
#endif
