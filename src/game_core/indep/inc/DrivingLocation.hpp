#ifndef NFSHP2_INDEP_INC_DRIVINGLOCATION_HPP
#define NFSHP2_INDEP_INC_DRIVINGLOCATION_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/CarDriver.hpp"
typedef struct DriverAIRoadState DriverAIRoadState;

#ifdef __cplusplus
extern "C" {
#endif

void driverAIRoadInitialize(DriverAIRoadState* road,CarDriverKind kind);
struct CarRecoveryPointRegistry;
struct ReplaySnapshot;
void driverAIRoadSnapshot(DriverAIRoadState* road,const struct CarRecoveryPointRegistry* registry,struct ReplaySnapshot* snapshot);

#ifdef __cplusplus
}
#endif
#include "indep/inc/AICarController.hpp"
#ifdef __cplusplus
extern "C" {
#endif
void driverAIRoadAssign(DriverAIRoadState* road,const CarRecoveryGroup* group,const CarRecoveryPose* pose,
    int32_t lane,int32_t lockSelection,int32_t nowTicks);
void driverAIRoadSetLane(DriverAIRoadState* road,int32_t lane,int32_t lockSelection,int32_t nowTicks);
void driverAIRoadMarkSelected(DriverAIRoadState* road,int32_t selected,int32_t nowTicks);
float driverAIRoadSelectionCooldown(const DriverAIRoadState* road,int32_t nowTicks);
int32_t driverAIRoadSelectionLocked(const DriverAIRoadState* road,int32_t nowTicks);
int32_t driverAIRoadEmpty(const DriverAIRoadState* road);
#ifdef __cplusplus
}
#endif
#endif
