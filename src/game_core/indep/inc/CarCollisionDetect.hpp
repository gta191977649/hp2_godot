#ifndef NFSHP2_INDEP_INC_CARCOLLISIONDETECT_HPP
#define NFSHP2_INDEP_INC_CARCOLLISIONDETECT_HPP
#include "indep/inc/DriveTarget.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/World.hpp"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct DriverCollisionBox {
    float center[4];
    float axes[3][4];
    float halfExtents[3];
} DriverCollisionBox;

/* These operations use finite authored geometry and ordered time intervals.
 * The original no-contact time is 99999 seconds. */
int32_t driverCollisionIntersectIntervals(float* minimum,float* maximum,
    float firstMinimum,float firstMaximum,float secondMinimum,float secondMaximum);
void driverCollisionAxisInterval(float* minimum,float* maximum,
    float relativeVelocity,float displacement,float radius);
int32_t driverCollisionSeparatedOnAxis(const DriverCollisionBox* first,
    const DriverCollisionBox* second,const float axis[4],const float displacement[4]);
int32_t driverCollisionBoxesOverlap2D(const DriverCollisionBox* first,
    const DriverCollisionBox* second,const float displacement[2]);
int32_t driverCollisionBoxesOverlap3D(const DriverCollisionBox* first,
    const DriverCollisionBox* second,const float displacement[4]);
float driverCollisionBoxTime2D(const DriverCollisionBox* first,
    const DriverCollisionBox* second,const float displacement[2],const float relativeVelocity[2]);
void driverCollisionBuildPathBoxes(const DriverCommand* command,DriverCollisionBox* boxes,
    const DriverPhysicsVehicle* vehicle,float scale);
float driverCollisionVelocityTime(const DriverPhysicsVehicle* first,
    const DriverPhysicsVehicle* second,float scale);
int32_t driverCollisionNeighborEligible(const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception);
int32_t driverCollisionLowSpeedClose(const DriverAINeighbor* neighbor,
    const DriverAIPerception* perception);
int32_t driverCollisionPredictPair(const DriverPhysicsVehicle* first,const DriverCommand* firstCommand,
    const DriverPhysicsVehicle* second,const DriverCommand* secondCommand,float horizon,float* time);
CarMovement* driverCollisionPredictNeighbors(const DriverAIBase* driver,
    const DriverCommand* command,const DriverAIPerception* perception,const DriverAIRoadState* road,
    float horizon,float* time,World* world);

#ifdef __cplusplus
}
#endif

#endif
