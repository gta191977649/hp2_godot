#ifndef NFSHP2_INDEP_INC_RIGIDBODY_HPP
#define NFSHP2_INDEP_INC_RIGIDBODY_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/Table.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RigidBody RigidBody;
typedef struct RigidBodyState RigidBodyState;
struct ReplaySnapshot;
void rigidBodySnapshot(RigidBody* body, struct ReplaySnapshot* snapshot);

void rigidBodySetAngularMomentum(RigidBody* body, const float momentum[4]);
void rigidBodySetOrientation(RigidBody* body, const float orientation[16]);
void rigidBodySetPosition(RigidBody* body, const float position[4]);
void rigidBodySetDampingEnabled(RigidBody* body, int32_t enabled);
void rigidBodySetPursuitSpeedEnabled(RigidBody* body, int32_t enabled);
float rigidBodyInverseMass(const RigidBody* body);
float* rigidBodyForce(RigidBody* body);
float rigidBodyYawInertia(const RigidBody* body);
float* rigidBodyCenterOfMass(RigidBody* body);
float* rigidBodyAngularMomentum(RigidBody* body);
const float* rigidBodyAngularVelocity(const RigidBody* body);
float* rigidBodyLinearMomentum(RigidBody* body);
const float* rigidBodyLinearVelocity(const RigidBody* body);
void rigidBodyRestoreState(RigidBody* body, const RigidBodyState* state);
void rigidBodySaveState(const RigidBody* body, RigidBodyState* state);

typedef struct RigidBodyInfo {
    float mass;
    float inertia[16];
} RigidBodyInfo;

// Physical points use XYZ with W=1; physical directions and their rates
// use XYZ with W=0. Neither homogeneous slot is an integrated degree of freedom.
typedef struct RigidBodyState {
    float position[4];
    float orientation[16];
    float linearMomentum[4];
    float angularMomentum[4];
    float damping;
} RigidBodyState;

// The tuning contains the game's original damping curves and speed-limit
// coefficients. It outlives every body that refers to it.
typedef struct RigidBodyTuning {
    SampledCurve angularDamping;
    SampledCurve dampingTarget;
    float gravity;
    float pursuitSpeedScale;
    float uphillLimit;
    float uphillSpeedScale;
    float downhillLimit;
    float downhillSpeedScale;
    float slopeBrakeScale;
    float minimumSlopeBrake;
    float dampingLinearScale;
    float dampingAngularScale;
    float dampingRise;
    float dampingFall;
    float angularDampingScale;
    float linearDampingScale;
} RigidBodyTuning;

extern const RigidBodyTuning rigidBodyDefaultTuning;

typedef struct RigidBody {
    RigidBodyState motion;
    const RigidBodyInfo* info;
    const RigidBodyTuning* tuning;
    float linearVelocity[4];
    float angularVelocity[4];
    float worldTransform[16];
    float force[4];
    float torque[4];
    int32_t dampingEnabled;
    int32_t pursuitSpeedEnabled;
    float mass;
    float inverseMass;
    float inverseInertia[16];
    float centerOfMass[4];
    float yawTorqueLimit;
    float forwardSpeedLimit;
    float speedLimitScale;
} RigidBody;

RigidBody* rigidBodyInitialize(RigidBody* body, const RigidBodyInfo* info,
                               const RigidBodyTuning* tuning);
void rigidBodyRefreshMass(RigidBody* body);
void rigidBodySetCenterOfMass(RigidBody* body, const float center[4]);
void rigidBodySetMotion(RigidBody* body, const float position[4],
                        const float orientation[16], const float linearVelocity[4],
                        const float angularVelocity[4]);
void rigidBodyApplyForce(RigidBody* body, const float force[4], const float lever[4]);
void rigidBodyApplyLocalForce(RigidBody* body, const float force[4], const float lever[4]);
void rigidBodyAddTorque(RigidBody* body, const float torque[4]);
void rigidBodyApplyImpulse(RigidBody* body, const float impulse[4], const float lever[4]);
void rigidBodyApplyScaledAngularImpulse(RigidBody* body, const float impulse[4],
                                        const float lever[4], float angularScale);
void rigidBodyPredictImpulseVelocity(const RigidBody* body, float linear[4],
                                     float angular[4], const float impulse[4],
                                     const float lever[4]);
float rigidBodyKineticEnergy(const RigidBody* body);
float rigidBodyImpulseEnergy(const RigidBody* body, const float impulse[4], const float lever[4]);
void rigidBodyOrientationDerivatives(float output[16], const float angularVelocity[4],
                                     const float orientation[16]);
RigidBodyState* rigidBodyComputeDerivatives(RigidBody* body, RigidBodyState* derivatives);
void rigidBodyPrepareStep(RigidBody* body);
// Reconcile only angular velocity after the caller orthonormalizes orientation.
// Preserves collected forces, torques, momenta and all other prepared state.
void rigidBodyRefreshAngularVelocity(RigidBody* body);
float rigidBodyHorizontalAccelerationG(const RigidBody* body);

void rigidBodySetLinearMomentum(RigidBody* body, const float momentum[4]);

void rigidBodySetSpeedLimitScale(RigidBody* body, float scale);

void rigidBodySetForwardSpeedLimit(RigidBody* body, float limit);
void rigidBodySetYawTorqueLimit(RigidBody* body, float limit);
const float* rigidBodyInverseInertia(const RigidBody* body);

float rigidBodyMass(const RigidBody* body);
const float* rigidBodyWorldTransform(const RigidBody* body);

float* rigidBodyOrientation(RigidBody* body);

float* rigidBodyPosition(RigidBody* body);

RigidBodyState* rigidBodyState(RigidBody* body);
// Three unused console alignment slots are excluded from the native state.
int32_t rigidBodyStateSize(const RigidBody* body);

#ifdef __cplusplus
}
#endif

#endif
