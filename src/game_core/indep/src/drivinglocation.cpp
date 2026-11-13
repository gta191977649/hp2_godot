#include "indep/inc/AICarController.hpp"
int32_t driverAIRoadEmpty(const DriverAIRoadState* road)
{
    return road->group==NULL;
}
#include "indep/inc/DrivingLocation.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/Replay.hpp"
// Full PAL239CF0..239D8C, DrivingLocation::DoSnapshot.
void driverAIRoadSnapshot(DriverAIRoadState* road,const CarRecoveryPointRegistry* registry,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"DrivingLocation");
    carRecoverySnapshotGroup(registry,snapshot,&road->group);
    carRecoverySnapshotPoseInGroup(road->group,snapshot,&road->recovery.recoveryPose);
    replaySnapshotMoveInteger(snapshot,&road->lane,1);
    replaySnapshotMoveData(snapshot,&road->selectedTicks,4,-1,0);
    replaySnapshotEndSection(snapshot);
}

void driverAIRoadAssign(DriverAIRoadState* road,const CarRecoveryGroup* group,const CarRecoveryPose* pose,
    int32_t lane,int32_t lockSelection,int32_t nowTicks)
{
    road->group=group;
    road->recovery.recoveryPose=pose;
    driverAIRoadSetLane(road,lane,lockSelection,nowTicks);
}
void driverAIRoadSetLane(DriverAIRoadState* road,int32_t lane,int32_t lockSelection,int32_t nowTicks)
{
    road->lane=lane;
    driverAIRoadMarkSelected(road,lockSelection,nowTicks);
}
void driverAIRoadMarkSelected(DriverAIRoadState* road,int32_t selected,int32_t nowTicks)
{
    road->selectedTicks=selected?nowTicks:0;
}
float driverAIRoadSelectionCooldown(const DriverAIRoadState* road,int32_t nowTicks)
{
    const int32_t elapsed=(int32_t)((uint32_t)nowTicks-(uint32_t)road->selectedTicks);
    const float seconds=(float)elapsed*0.0002500000118743628f;
    const float remaining=3.0f-seconds;
    return 0.0f<=remaining?remaining:0.0f;
}
int32_t driverAIRoadSelectionLocked(const DriverAIRoadState* road,int32_t nowTicks)
{
    return 0.0f<driverAIRoadSelectionCooldown(road,nowTicks);
}
const DriverAITuning driverAIDefaultTuning={1,10000.5f,26.821636199951172f,29.503801345825195f,0,2.0f,20.0f,10.0f,4,8000.5f,12000.5f};
