#ifndef NFSHP2_CARMOVER_HPP
#define NFSHP2_CARMOVER_HPP

// Native declarations use the PC runtime layout.
#include "CarState.hpp"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NullMover { VehicleState* state; } NullMover;
static inline void nullMoverInitialize(NullMover* mover, VehicleState* state)
{ mover->state = state; }
float nullMoverMove(NullMover* mover, float elapsed);
#ifdef __cplusplus
}
#endif

#include "indep/inc/Table.hpp"
#include "indep/inc/QuickSpline.hpp"
#include "indep/inc/Point.hpp"
#include "indep/inc/Skids.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct CarMovement;
struct DriverCommand;
struct DriverPhysicsVehicle;
struct World;

/* These references address the active AI input and controller storage.
 * They remain live through the mover lifetime, including driver rebinding. */
typedef struct BeltMoverBindings {
    struct DriverCommand* command;
    const struct DriverPhysicsVehicle* vehicle;
    const float* targetSpeed;
    const float* targetDuration;
    int32_t* replanDelayTicks;
    struct World* world;
} BeltMoverBindings;

typedef struct BeltMover {
    struct CarMovement* movement;
    float elapsed;
    float parameterScale;
    float startParameter;
    float velocityParameter;
    float accelerationParameter;
    QuickSpline path;
    tCubic1D lateralOffset;
    tCubic2D velocityFilter;
    tCubic2D lateralFilter;
    DriverHistory lateralHistory;
    int32_t sliding;
    Wheel* wheels[4];
    WheelVehicleState wheelVehicle;
    WheelTrail* trails[4];
    WheelTrailStore localTrailStore;
    WheelTrail localTrails[16];
} BeltMover;

int32_t beltMoverInitialize(BeltMover* mover, struct CarMovement* movement);
void beltMoverDispose(BeltMover* mover);
float beltMoverRelativePosition(BeltMover* mover,const float position[4],float parameter,
                                 float difference[4],float tangent[4]);
float beltMoverSlope(BeltMover* mover);
float beltMoverAvailableAcceleration(BeltMover* mover);
void beltMoverRecalculate(BeltMover* mover);
void beltMoverActivate(BeltMover* mover);
float beltMoverLateralGrip(BeltMover* mover,float acceleration);
float beltMoverPredictRoadError(BeltMover* mover,float duration);
float beltMoverMove(BeltMover* mover,float elapsed);
float* beltMoverPosition(BeltMover* mover,float output[4],float elapsed);
float* beltMoverVelocity(BeltMover* mover,float output[4],float elapsed);
float* beltMoverAcceleration(BeltMover* mover,float output[4],float elapsed);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct ReplaySnapshot;
void beltMoverSnapshot(BeltMover* mover,struct ReplaySnapshot* snapshot);
#ifdef __cplusplus
}
#endif

#endif
