#pragma once

#include "indep/inc/CarDriver.hpp"
#include "Car.hpp"
#include "indep/inc/Table.hpp"
#include "indep/inc/AICarController.hpp"
#include "RealDriver.hpp"
#include "indep/inc/QuickSpline.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* RealController and CopDriver share this authored-road state. Movement
 * recovery borrows the embedded view; there is one current pose and timer. */
typedef struct DriverRoute {
    CarMovement* movement;
    CarMovementDriver recovery;
    float acceleration;
    CarDriverResult result;
} DriverRoute;

CarDriverResult driverRouteInitialize(DriverRoute* driver, CarMovement* movement,
                                      CarDriverKind kind);
void driverRouteAdvance(DriverRoute* driver, float elapsed);
void driverRouteSnapshot(DriverRoute* driver,struct ReplaySnapshot* snapshot);
void driverRouteSetPose(DriverRoute* driver, const CarRecoveryPose* pose);
const CarRecoveryGroup* driverRouteCurrentGroup(const DriverRoute* driver);
float driverRouteLateralExcess(const DriverRoute* driver);

struct RealController {
    DriverRoute base;
    RealDriver input;
    DriverHistory lateralVelocity;
    float previousLateral;
    int32_t historyCount;
    CarRecoverySelection history[4];
    QuickSpline path;
    float pathStorage[7][4];
    DriverPhysicsVehicle physics;
};

CarDriverResult realControllerInitialize(RealController* controller,CarMovement* movement,
                                          const DriverPhysicsVehicle* physics,const RealDriverBindings* input);
void realControllerDispose(RealController* controller);
void realControllerThink(RealController* controller,float elapsed);
void realControllerSnapshot(RealController* controller,struct ReplaySnapshot* snapshot);
int32_t realControllerMeasureLateralMotion(RealController* controller,float* distance,float* velocity,
                                           const CarRecoverySelection* road,float elapsed);
void realControllerBuildRoadCommand(RealController* controller,DriverCommand* command,
                                     const CarRecoverySelection* road,float lateralVelocity,float lateralDistance);

#ifdef __cplusplus
}
#endif
