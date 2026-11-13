#ifndef NFSHP2_CAR_STATE_HPP
#define NFSHP2_CAR_STATE_HPP

#include <stdint.h>
#include "indep/inc/Wheel.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct TerrainSurface;
struct ReplaySnapshot;

typedef struct VehicleState {
    /* Physical boundaries and snapshot restore publish positions/centers with
     * W=1, velocities/accelerations with W=0. Source vector snapshots are XYZ.
     * Path movers may have other finite padding; physics imports reconstruct it. */
    float* position;
    const float* centerOfMass;
    const WheelTorqueInfo* wheelInfo;
    float wheelSideOffset[4];
    float centerPosition[4];
    float velocity[4];
    float orientation[16];
    float angularVelocity[4];
    float wheelVisualTransform[4][16];
    float wheelMountTransform[4][16];
    float wheelAngularVelocity[4];
    float wheelAngle[4];
    float wheelNormalLoad[4];
    float wheelSurfaceOffset[4];
    float suspensionImpulse[4];
    float suspensionTravel[4];
    int8_t wheelContact[4];
    int32_t material[4];
    int32_t peakMaterial[4];
    float wheelSlip[4][2];
    float wheelSlipRatio[4];
    uint16_t heading;
    uint16_t steeringAngle;
    float averageSlipRatio;
    float leftSlip;
    float rightSlip;
    int32_t activeContactCount;
    float forwardSpeed;
    float speed;
    int32_t gear;
    float effectiveThrottle;
    float engineRpm;
    int32_t clutchEngaged;
    int32_t pursuitMode;
    float performanceScale;
    int32_t tractionAssistPhase;
    float tractionAssistTime;
    float tractionAssistScale;
    float tractionAssistDwell;
    float velocityFrame[16];
    int32_t velocityFrameValid;
    float linearAcceleration[4];
    float angularAcceleration[4];
    float speedDistance;
    float displacementDistance;
    /* Source CarState+394 / Car+3D4, distinct from Car+4A0 displaySpeed. */
    float engineSoundRpm;
    /* Borrowed final TopologyCoordinate projection, no second terrain owner. */
    struct TerrainSurface* terrain;
} VehicleState;

struct TerrainSurface;

void vehicleStateReset(VehicleState* state);
void vehicleStateSnapshot(VehicleState* state,struct ReplaySnapshot* snapshot);
/* Native replay/state invariant: finite live numeric owners and derived poses.
 * No range clamps, PS2 float emulation or speed thresholds are implied. */
int32_t vehicleStateIsFinite(const VehicleState* state);
/* Null means valid. Field names are static literals; element is a flattened
 * array index, or -1 for a scalar/missing owner. No allocation or mutation.
 * Invalid lazy velocity-frame bytes are excluded until the cache is live. */
const char* vehicleStateFirstInvalidField(const VehicleState* state,int32_t* element);
void driverPerceptionLocalPoint(VehicleState* state,float output[2],const float point[2]);
void vehicleStateWorldToCar3(VehicleState* state,float output[4],const float point[4]);
void vehicleStateCarToWorldUnadjusted(VehicleState* state,float output[2],const float point[2]);
void vehicleStateCarToWorld(VehicleState* state,float output[2],const float point[2]);
void vehicleStateSetWheelContact(VehicleState* state, int32_t wheel, int8_t contact);
void vehicleStateSetWheelMaterial(VehicleState* state, int32_t wheel, int32_t material);
float vehicleStateVelocityHeadingDifference(const VehicleState* state);
float vehicleStateMovingHeadingDifference(const VehicleState* state);
const float* vehicleStateVelocityFrame(VehicleState* state);
void vehicleStateUpdateWheelPose(VehicleState* state, int32_t wheel);
void vehicleStateUpdateWheelPoses(VehicleState* state);
void vehicleStateUpdateCenterPosition(VehicleState* state);
void vehicleStateResetForSound(VehicleState* state);
int32_t vehicleStateGroundedWheelCount(const VehicleState* state);
void vehicleStateInitializeWheelContact(VehicleState* state, Wheel* wheel,
                                        struct TerrainSurface* terrain,
                                        float angle, float angularVelocity,
                                        float travel, uint16_t steeringAngle);

#ifdef __cplusplus
}
namespace hp2 {
struct CarDefinition;
void InitializeCarState(VehicleState& state,float position[4],
    TerrainSurface& terrain,const CarDefinition& definition);
}
#endif
#endif
