#ifndef NFSHP2_CARCONTROLLER_HPP
#define NFSHP2_CARCONTROLLER_HPP
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct Car;
/* Final-only debug cell PAL330A58. Actual Pause replay event6 writes this
 * same AI-consumer flag; it is independent of every Car.pursuitMode. */
extern int32_t carControllerSuperEasyAI;
// Native base owner. The canonical Car and its physical state remain borrowed.
// Snapshot dispatch follows the actual input owner attached to this same Car.
typedef struct CarController { struct Car* car; } CarController;
void carControllerInitialize(CarController* controller,struct Car* car);
void carControllerForceBoost(CarController* controller,float performanceScale);
void carControllerReleaseBoost(CarController* controller);
struct CarMovement;
struct ReplaySnapshot;
void carControllerSnapshot(struct CarMovement* movement,struct ReplaySnapshot* snapshot);
#ifdef __cplusplus
}
#endif

#include "indep/inc/Car.hpp"
#include "indep/inc/DriveTarget.hpp"
/* The inactive controller still owns its original empty command. It is a
 * real input to prediction and snapshots, even though its update is empty. */
typedef struct NullController {
    CarController base;
    CarMovement* movement;
    CarMovementDriver recovery;
    DriverCommand command;
} NullController;
#endif
