#ifndef NFSHP2_TRAFFICCAR_HPP
#define NFSHP2_TRAFFICCAR_HPP

// Native declarations use the PC runtime layout.

#include "indep/inc/Table.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct CarMovement;
typedef struct TrafficCar {
    struct CarMovement* movement;
    float position[2];
    float headingRadians;
    float speed;
    float savedAcceleration;
    float savedSteering;
    SampledCurve acceleration;
    SampledCurve braking;
} TrafficCar;

void trafficCarInitialize(TrafficCar* car, struct CarMovement* movement);
float trafficCarMove(TrafficCar* car, float elapsed);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct ReplaySnapshot;
void trafficCarSnapshot(TrafficCar* car,struct ReplaySnapshot* snapshot);
float trafficCarCalculateStopDistance(float sourceUnused,float speed);
#ifdef __cplusplus
}
#endif

#endif
