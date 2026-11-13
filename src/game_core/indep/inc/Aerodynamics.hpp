#ifndef NFSHP2_INDEP_INC_AERODYNAMICS_HPP
#define NFSHP2_INDEP_INC_AERODYNAMICS_HPP

#include "indep/inc/RigidBody.hpp"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AerodynamicsInfo {
    float drag;
    float frontDownforce;
    float rearDownforce;
} AerodynamicsInfo;

typedef struct AerodynamicsTuning {
    float boostedDrag;
    float dragLeverZ;
} AerodynamicsTuning;
extern const AerodynamicsTuning aerodynamicsDefaultTuning;

typedef struct Aerodynamics {
    const AerodynamicsInfo* info;
    RigidBody* body;
    const AerodynamicsTuning* tuning;
    float speedScale;
    float dragScale;
    float dragReduction;
    float downforceMode;
} Aerodynamics;

void aerodynamicsInitialize(Aerodynamics* aerodynamic, const AerodynamicsInfo* info,
                            RigidBody* body, const AerodynamicsTuning* tuning);
struct ReplaySnapshot;
void aerodynamicsResetTransientState(Aerodynamics* aerodynamic, struct ReplaySnapshot* snapshot);
void aerodynamicsApplyForces(Aerodynamics* aerodynamic, float* totalDownforce);
float aerodynamicsEstimateDownforce(const Aerodynamics* aerodynamic, float speed);

#ifdef __cplusplus
}
#endif

#endif
