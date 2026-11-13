#ifndef NFSHP2_TRAFFICDRIVER_HPP
#define NFSHP2_TRAFFICDRIVER_HPP

// Native declarations use the PC runtime layout.

#include "indep/inc/CarDriver.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/PsychoDriver.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct TrafficDriver {
    DriverAIBase base;
    AIControls input;
    int32_t roadBranch;
    CarDriverResult result;
};

CarDriverResult trafficDriverInitialize(TrafficDriver* driver,CarMovement* movement,
    const DriverPhysicsVehicle* physics,DriverAIEnvironment* environment,
    const GameTime* clock,struct CarSoundState* sound);
void trafficDriverDispose(TrafficDriver* driver);
void trafficDriverResetAfterPlacement(TrafficDriver* driver);
void trafficDriverApplyAction(TrafficDriver* driver);
void trafficDriverActionTarget(TrafficDriver* driver,float* speed,float* duration,float* tolerance);

const CarRecoveryGroup* trafficDriverCurrentGroup(TrafficDriver* driver, int32_t refreshIfMissing);
const CarRecoveryPose* trafficDriverCurrentPose(TrafficDriver* driver, int32_t refreshIfMissing);
CarDriverResult trafficDriverGetResult(const TrafficDriver* driver);
void trafficDriverThink(TrafficDriver* driver, float elapsed);
void trafficDriverSnapshot(TrafficDriver* driver,struct ReplaySnapshot* snapshot);
void trafficDriverUpdateControls(TrafficDriver* driver, float elapsed);
int32_t trafficDriverHornActive(const TrafficDriver* driver);
CarDriverResult trafficDriverSetHorn(TrafficDriver* driver, int32_t enabled);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif
void trafficDriverGetRouteNumbersToUse(const TrafficDriver* driver,int32_t* first,int32_t* last);
void trafficDriverSetDirection(TrafficDriver* driver,int32_t roadBranch);
#ifdef __cplusplus
}
#endif
#endif
