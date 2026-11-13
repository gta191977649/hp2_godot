#ifndef NFSHP2_DRIVETARGET_HPP
#define NFSHP2_DRIVETARGET_HPP

#include "indep/inc/TrackRoute.hpp"
#include "CarState.hpp"

#ifdef __cplusplus
extern "C" {
#endif

enum { DRIVER_COMMAND_CAPACITY=20 };
typedef struct DriverPhysicsVehicle DriverPhysicsVehicle;

typedef struct DriverCommandPoint {
    float position[4];
    float targetSpeed;
    CarRecoverySelection road;
    int32_t laneIndex;
} DriverCommandPoint;

/* Path points own their values and borrow authored road references. Caches
 * belong to this command and use the actual simulation clock. */
typedef struct DriverCommand {
    int32_t count;
    DriverCommandPoint points[DRIVER_COMMAND_CAPACITY];
    int32_t closestPoint;
    int32_t closestTicks;
    int32_t adjustmentTicks;
    float length;
    int32_t lastSpeedPoint;
    uint32_t roadFlags;
} DriverCommand;
struct ReplaySnapshot;
void driverCommandSnapshot(DriverCommand* command,const CarRecoveryPointRegistry* registry,
    struct ReplaySnapshot* snapshot);

DriverCommand* driverCommandInitialize(DriverCommand* command);
DriverCommand* driverCommandCopy(DriverCommand* output,const DriverCommand* input);
uint16_t driverCommandHeading(const DriverCommand* command,int32_t first);
int32_t driverCommandLastSegment(const DriverCommand* command);
int32_t driverCommandPointAhead(const float point[2],const VehicleState* vehicle,float threshold);
void driverCommandAppend(DriverCommand* command,const float point[4],
                          const CarRecoverySelection* road,int32_t laneIndex,
                          int32_t rejectBacktracking,const VehicleState* vehicle);
void driverCommandSetPoint(DriverCommand* command,int32_t index,const float point[4],
                            const CarRecoverySelection* road,int32_t laneIndex,float targetSpeed);
void driverCommandInsertFront(DriverCommand* command,int32_t count);
void driverCommandTrimFront(DriverCommand* command,int32_t count,int32_t updateLength);
void driverCommandRecalculateLength(DriverCommand* command);
int32_t driverCommandRoadAt(const DriverCommand* command,int32_t index,CarRecoverySelection* road);
int32_t driverCommandClosestPoint(DriverCommand* command,const VehicleState* vehicle,int32_t nowTicks);
int32_t driverCommandPointAtDistance(DriverCommand* command,const VehicleState* vehicle,
                                     int32_t nowTicks,float distance,float output[4]);
float driverCommandSegmentTime(float distance,float firstSpeed,float secondSpeed);
void driverCommandAdvance(DriverCommand* command,const VehicleState* vehicle,
                           int32_t nowTicks,float distance);
void driverCommandBuildStraight(DriverCommand* command,const DriverPhysicsVehicle* vehicle);
void driverCommandBuildVelocityPath(DriverCommand* command,const DriverPhysicsVehicle* vehicle);
void driverCommandBlendPredictedPath(DriverCommand* command,const DriverPhysicsVehicle* vehicle);
float driverCommandLateralAccelerationLimit(const DriverCommand* command,const DriverPhysicsVehicle* vehicle,float speed);
int32_t driverCommandCheckLateralAcceleration(const DriverCommand* command,const DriverPhysicsVehicle* vehicle,
    const float initialVelocity[2],const float finalVelocity[2],float* finalSpeed,float distance,float initialSpeed);
int32_t driverCommandCheckStartingTurn(const DriverCommand* command,const DriverPhysicsVehicle* vehicle,
    const float point[2],float* targetSpeed,float lookaheadSeconds);
int32_t driverCommandCheckTurn(const DriverCommand* command,const DriverPhysicsVehicle* vehicle,
    const float first[2],const float current[2],const float next[2],float* targetSpeed,float distance,float initialSpeed);
float driverCommandPlanSpeed(DriverCommand* command,const DriverPhysicsVehicle* vehicle);
/* Finite native arc resampling. Repeated points carry zero arc distance;
 * an entirely stationary command stays stationary. Non-finite/overflowing
 * positions fail before publishing a partially resampled command. */
void driverCommandResample(DriverCommand* command);

float driverCommandDistanceThree(const float first[4],const float second[4]);
float driverCommandDotThree(const float first[4],const float second[4]);
float driverCommandDistanceTwo(const float first[2],const float second[2]);

#ifdef __cplusplus
}
#endif
#endif
