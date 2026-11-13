#include "indep/inc/CarState.hpp"
#include "indep/inc/CarInfo.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Track.hpp"
#include "bware/inc/bMath.hpp"
#include "CarState.hpp"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void carFrameResetPeakMaterials(VehicleState* state)
{
    for (int wheel = 3; wheel >= 0; --wheel)
        state->peakMaterial[wheel] = 1;
}

void vehicleStateResetForSound(VehicleState* state)
{
    state->engineRpm = 1000.0f;
    for (int32_t wheel = 0; wheel < 4; ++wheel) {
        state->wheelAngularVelocity[wheel] = 0.0f;
        state->wheelAngle[wheel] = 0.0f;
        state->wheelNormalLoad[wheel] = 0.0f;
        state->suspensionImpulse[wheel] = 0.0f;
        state->wheelContact[wheel] = 1;
        state->wheelSlip[wheel][1] = 0.0f;
        state->wheelSlip[wheel][0] = 0.0f;
        state->material[wheel] = 1;
        state->wheelSlipRatio[wheel] = 0.0f;
        state->peakMaterial[wheel] = 1;
        state->suspensionTravel[wheel] = 0.0f;
    }
    state->rightSlip = 0.0f;
    state->activeContactCount = 4;
    state->speed = 0.0f;
    state->averageSlipRatio = 0.0f;
    state->leftSlip = 0.0f;
}

void vehicleStateSetWheelContact(VehicleState* state, int32_t wheel, int8_t contact)
{
    const int8_t previous = state->wheelContact[wheel];
    state->wheelContact[wheel] = contact;
    if (!previous) {
        if (contact) ++state->activeContactCount;
    } else if (!contact) {
        --state->activeContactCount;
    }
}

void vehicleStateSetWheelMaterial(VehicleState* state, int32_t wheel, int32_t material)
{
    state->material[wheel] = material;
    if (state->peakMaterial[wheel] < material)
        state->peakMaterial[wheel] = material;
}

void vehicleStateReset(VehicleState* state)
{
    if (state->position) state->position[3] = 1.0f;
    state->centerPosition[3] = 1.0f;
    for (int axis = 0; axis < 4; ++axis) {
        state->velocity[axis] = 0.0f;
        state->linearAcceleration[axis] = 0.0f;
        state->angularVelocity[axis] = 0.0f;
        state->angularAcceleration[axis] = 0.0f;
    }
    rotationMatrixIdentity(state->orientation);
    state->heading = state->steeringAngle = 0;
    for (int wheel = 0; wheel < 4; ++wheel) {
        state->wheelAngularVelocity[wheel] = 0.0f;
        state->wheelAngle[wheel] = 0.0f;
        state->wheelNormalLoad[wheel] = 0.0f;
        state->wheelSurfaceOffset[wheel] = 0.0f;
        state->suspensionImpulse[wheel] = 0.0f;
        state->suspensionTravel[wheel] = 0.0f;
        state->wheelContact[wheel] = 1;
        state->material[wheel] = state->peakMaterial[wheel] = 1;
        state->wheelSlip[wheel][0] = state->wheelSlip[wheel][1] = 0.0f;
        state->wheelSlipRatio[wheel] = 0.0f;
    }
    state->activeContactCount = 4;
    vehicleStateUpdateWheelPoses(state);
    state->averageSlipRatio = state->leftSlip = state->rightSlip = 0.0f;
    state->forwardSpeed = state->speed = 0.0f;
    state->effectiveThrottle = 0.0f;
    state->engineRpm = 1000.0f;
    state->engineSoundRpm = -1.0f;
    state->clutchEngaged = state->gear = 1;
    state->performanceScale = 1.0f;
    state->tractionAssistPhase = 0;
    state->tractionAssistTime = 0.0f;
    state->tractionAssistScale = 1.0f;
    memset(state->velocityFrame, 0, sizeof(state->velocityFrame));
    state->velocityFrameValid = 0;
}

float vehicleStateVelocityHeadingDifference(const VehicleState* state)
{
    const uint16_t velocityHeading = vectorAtan2Angle(state->velocity[0], state->velocity[1]);
    const uint16_t difference = (uint16_t)(state->heading - velocityHeading);
    float radians = (float)difference * 0.0000958738019107841f;
    if (3.141590118408203f < radians)
        radians = radians - 6.283180236816406f;
    return radians;
}

float vehicleStateMovingHeadingDifference(const VehicleState* state)
{
    return state->forwardSpeed < 5.0f ? 0.0f : vehicleStateVelocityHeadingDifference(state);
}

const float* vehicleStateVelocityFrame(VehicleState* state)
{
    if (!state->velocityFrameValid) {
        float direction[4];
        for (int i = 0; i < 4; ++i)
            direction[i] = state->forwardSpeed < 0.0f ? 0.0f - state->velocity[i] : state->velocity[i];
        const float x = direction[0] * direction[0];
        const float y = direction[1] * direction[1];
        const float z = direction[2] * direction[2];
        const float xy = x + y;
        const float squared = xy + z;
        float basis[16];
        rotationMatrixIdentity(basis);
        if (1.0f < squared && (direction[0] != 0.0f || direction[1] != 0.0f)) {
            // The source's XYZ speed gate selects a horizontal travel frame.
            // Keep its ordinary float path. At the representation limits,
            // rescale before normalization: the cross-axis pass squares the
            // length again and needs headroom for intermediate rounding.
            float horizontalX = direction[0], horizontalY = direction[1];
            if (!isfinite(xy) || xy < FLT_MIN || FLT_MAX * 0.5f < xy) {
                const double length = hypot((double)horizontalX, (double)horizontalY);
                horizontalX = (float)((double)horizontalX / length);
                horizontalY = (float)((double)horizontalY / length);
            }
            basis[0] = horizontalY;
            basis[1] = -horizontalX;
            basis[4] = horizontalX;
            basis[5] = horizontalY;
        } else {
            // Pure vertical motion has no horizontal travel direction. Reuse
            // the existing heading basis instead of normalizing a zero axis.
            const float cosine = vectorCosineAngle(state->heading);
            const float sine = rotationSineAngle(state->heading);
            basis[0] = sine;
            basis[1] = -cosine;
            basis[4] = cosine;
            basis[5] = sine;
        }
        motionMatrixOrthonormalize(state->velocityFrame, basis);
        state->velocityFrameValid = 1;
    }
    return state->velocityFrame;
}

void vehicleStateUpdateWheelPose(VehicleState* state, int32_t wheel)
{
    float visual[16], mount[16];
    rotationMatrixIdentity(visual);
    rotationMatrixIdentity(mount);
    const float angularCoordinate = state->wheelAngle[wheel] * 10430.3779296875f;
    const uint16_t angle = (uint16_t)convertFloatToInt32(angularCoordinate);
    motionMatrixRotateY(visual, visual, angle);
    if (wheel < 2) {
        motionMatrixRotateYaw(visual, visual, state->steeringAngle);
        motionMatrixRotateYaw(mount, mount, state->steeringAngle);
    }
    const WheelTorqueInfo* info = &state->wheelInfo[wheel];
    for (int i = 0; i < 3; ++i) {
        visual[12 + i] = info->mountPosition[i];
        mount[12 + i] = info->mountPosition[i];
    }
    visual[13] = visual[13] + state->wheelSideOffset[wheel];
    mount[13] = mount[13] + state->wheelSideOffset[wheel];
    visual[14] = visual[14] + state->suspensionTravel[wheel];
    mount[14] = mount[14] + state->suspensionTravel[wheel];
    memcpy(state->wheelVisualTransform[wheel], visual, sizeof(visual));
    memcpy(state->wheelMountTransform[wheel], mount, sizeof(mount));
}

void vehicleStateUpdateWheelPoses(VehicleState* state)
{
    for (int32_t wheel = 0; wheel < 4; ++wheel)
        vehicleStateUpdateWheelPose(state, wheel);
}

void vehicleStateUpdateCenterPosition(VehicleState* state)
{
    float center[4];
    vectorMatrixTransformPoint(center, state->orientation, state->centerOfMass);
    for (int i = 0; i < 3; ++i)
        state->centerPosition[i] = state->position[i] - center[i];
    state->centerPosition[3] = 1.0f;
}

void driverPerceptionLocalPoint(VehicleState* state,float output[2],const float point[2])
{
    float transpose[16],local[4];
    vectorMatrixTranspose(transpose,vehicleStateVelocityFrame(state));
    const float delta[4]={point[0]-state->position[0],
                          point[1]-state->position[1],0.0f,0.0f};
    vectorMatrixTransformPoint(local,transpose,delta);
    output[0]=local[0];
    output[1]=local[1];
}

// PAL1C0BA8: the three-dimensional overload uses the same cached travel
// frame as the two-dimensional AI query. All source XYZ lanes are retained.
void vehicleStateWorldToCar3(VehicleState* state,float output[4],const float point[4])
{
    float transpose[16],delta[4];
    vectorMatrixTranspose(transpose,vehicleStateVelocityFrame(state));
    for (int lane=0;lane<4;++lane)delta[lane]=point[lane]-state->position[lane];
    eMulVector3(output,transpose,delta);
}

// PAL1C0C18: rotate a displacement; translation is a separate operation in
// CarToWorld. Local temporaries preserve the source's aliased-input behavior.
void vehicleStateCarToWorldUnadjusted(VehicleState* state,float output[2],const float point[2])
{
    const float* transform=vehicleStateVelocityFrame(state);
    float local[4]={point[0],point[1],0.0f,0.0f};
    eMulVector3(local,transform,local);
    output[0]=local[0];output[1]=local[1];
}

// PAL1C0C80: the PS2 return-by-value ABI is a native output argument here.
void vehicleStateCarToWorld(VehicleState* state,float output[2],const float point[2])
{
    float rotated[2];
    vehicleStateCarToWorldUnadjusted(state,rotated,point);
    output[0]=rotated[0]+state->position[0];
    output[1]=rotated[1]+state->position[1];
}

namespace hp2 {
// PAL1C01A8..1C026C. The native car owns position and the retained resource
// owns all coefficients; this state borrows them, including the same terrain
// coordinate subsequently consumed by the actual collision and mover paths.
void InitializeCarState(VehicleState& state,float position[4],
    TerrainSurface& terrain,const CarDefinition& definition)
{
    state.position=position;
    state.centerOfMass=definition.centerOfMass.data();
    state.wheelInfo=definition.wheelInfo.data();
    state.terrain=&terrain;
    for (int wheel=0;wheel<4;++wheel)
        state.wheelSideOffset[wheel]=definition.wheels[wheel].sideValue;
    state.speedDistance=0.0f;
    state.displacementDistance=0.0f;
    for (int axis=0;axis<3;++axis) {
        state.position[axis]=0.0f;
        state.centerPosition[axis]=0.0f;
    }
    state.pursuitMode=0;
    vehicleStateReset(&state);
}
} // namespace hp2

namespace {
const char* FirstInvalidArray(const float* values,size_t count,const char* field,
    int32_t* element,int32_t offset=0)
{
    for(size_t index=0;index<count;++index)if(!isfinite(values[index])) {
        if(element)*element=offset+(int32_t)index;
        return field;
    }
    return nullptr;
}
const char* FirstInvalidInput(const VehicleState* state,int32_t* element)
{
    if(!state)return "state";
    if(!state->position)return "position";
    struct NamedArray {const float* values;size_t count;const char* field;};
    const NamedArray arrays[]={
        {state->position,4,"position"},{state->wheelSideOffset,4,"wheelSideOffset"},
        {state->centerPosition,4,"centerPosition"},{state->velocity,4,"velocity"},
        {state->orientation,16,"orientation"},{state->angularVelocity,4,"angularVelocity"},
        {state->linearAcceleration,4,"linearAcceleration"},
        {state->angularAcceleration,4,"angularAcceleration"}};
    for(const auto& array:arrays)
        if(const char* field=FirstInvalidArray(array.values,array.count,array.field,element))return field;
    if(state->centerOfMass)
        if(const char* field=FirstInvalidArray(state->centerOfMass,3,"centerOfMass",element))return field;
    for(int wheel=0;wheel<4;++wheel) {
        if(state->wheelInfo)
            if(const char* field=FirstInvalidArray(state->wheelInfo[wheel].mountPosition,3,
                "wheelInfo.mountPosition",element,wheel*3))return field;
        const NamedArray wheelArrays[]={
            {state->wheelAngularVelocity+wheel,1,"wheelAngularVelocity"},
            {state->wheelAngle+wheel,1,"wheelAngle"},
            {state->wheelNormalLoad+wheel,1,"wheelNormalLoad"},
            {state->wheelSurfaceOffset+wheel,1,"wheelSurfaceOffset"},
            {state->suspensionImpulse+wheel,1,"suspensionImpulse"},
            {state->suspensionTravel+wheel,1,"suspensionTravel"}};
        for(const auto& array:wheelArrays)
            if(const char* field=FirstInvalidArray(array.values,array.count,array.field,element,wheel))return field;
        if(const char* field=FirstInvalidArray(state->wheelSlip[wheel],2,"wheelSlip",element,wheel*2))return field;
        if(const char* field=FirstInvalidArray(state->wheelSlipRatio+wheel,1,"wheelSlipRatio",element,wheel))return field;
    }
    struct NamedScalar {float value;const char* field;};
    const NamedScalar scalars[]={
        {state->averageSlipRatio,"averageSlipRatio"},{state->leftSlip,"leftSlip"},
        {state->rightSlip,"rightSlip"},{state->forwardSpeed,"forwardSpeed"},
        {state->speed,"speed"},{state->effectiveThrottle,"effectiveThrottle"},
        {state->engineRpm,"engineRpm"},{state->performanceScale,"performanceScale"},
        {state->tractionAssistTime,"tractionAssistTime"},{state->tractionAssistScale,"tractionAssistScale"},
        {state->tractionAssistDwell,"tractionAssistDwell"},{state->speedDistance,"speedDistance"},
        {state->displacementDistance,"displacementDistance"},{state->engineSoundRpm,"engineSoundRpm"}};
    for(const auto& scalar:scalars)if(!isfinite(scalar.value))return scalar.field;
    return nullptr;
}
bool VehicleStateInputsFinite(const VehicleState* state)
{
    return FirstInvalidInput(state,nullptr)==nullptr;
}
}
const char* vehicleStateFirstInvalidField(const VehicleState* state,int32_t* element)
{
    if(element)*element=-1;
    if(const char* field=FirstInvalidInput(state,element))return field;
    // A mover invalidates this lazy cache after publishing new velocity.
    // Its old bytes are not live state until the actual producer rebuilds it.
    if(state->velocityFrameValid)
        if(const char* field=FirstInvalidArray(state->velocityFrame,16,"velocityFrame",element))return field;
    for(int wheel=0;wheel<4;++wheel) {
        if(const char* field=FirstInvalidArray(state->wheelVisualTransform[wheel],16,
            "wheelVisualTransform",element,wheel*16))return field;
        if(const char* field=FirstInvalidArray(state->wheelMountTransform[wheel],16,
            "wheelMountTransform",element,wheel*16))return field;
    }
    return nullptr;
}
int32_t vehicleStateIsFinite(const VehicleState* state)
{
    return vehicleStateFirstInvalidField(state,nullptr)==nullptr;
}

// PAL1C0270..1C05D4. Replay stores this one owner, never a second physics
// snapshot structure. Byte widths and transfer order follow the final PAL
// body, including its extra X-mantissa quantization absent from beta.
void vehicleStateSnapshot(VehicleState* state,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"CarState");
    replaySnapshotMoveData(snapshot,state->centerPosition,12,-1,0);
    if (snapshot->recording || snapshot->restoring) {
        uint32_t bits;
        memcpy(&bits,&state->position[0],sizeof(bits));
        bits&=0xfffffff0u;
        memcpy(&state->position[0],&bits,sizeof(bits));
    }
    replaySnapshotMoveData(snapshot,state->position,12,-1,0);
    replaySnapshotMoveData(snapshot,state->velocity,12,-1,0);
    replaySnapshotMoveData(snapshot,state->linearAcceleration,12,-1,0);
    replaySnapshotMoveMatrix(snapshot,state->orientation,64);
    replaySnapshotMoveData(snapshot,state->angularVelocity,12,-1,0);
    replaySnapshotMoveData(snapshot,state->angularAcceleration,12,-1,0);
    for (int wheel=0;wheel<4;++wheel) {
        replaySnapshotMoveFloat(snapshot,&state->wheelAngularVelocity[wheel]);
        replaySnapshotMoveFloat(snapshot,&state->wheelAngle[wheel]);
        replaySnapshotMoveFloat(snapshot,&state->wheelNormalLoad[wheel]);
        replaySnapshotMoveFloat(snapshot,&state->suspensionImpulse[wheel]);
        replaySnapshotMoveData(snapshot,&state->wheelContact[wheel],1,1,1);
        replaySnapshotMoveData(snapshot,&state->material[wheel],1,-1,0);
        replaySnapshotMoveData(snapshot,state->wheelSlip[wheel],8,-1,0);
        replaySnapshotMoveFloat(snapshot,&state->wheelSlipRatio[wheel]);
        replaySnapshotMoveFloat(snapshot,&state->suspensionTravel[wheel]);
        if (snapshot->recording || snapshot->restoring)state->peakMaterial[wheel]=1;
    }
    replaySnapshotMoveData(snapshot,&state->heading,2,2,0);
    replaySnapshotMoveData(snapshot,&state->steeringAngle,2,2,0);
    replaySnapshotMoveFloat(snapshot,&state->speedDistance);
    if (snapshot->recording || snapshot->restoring) {
        state->averageSlipRatio=0.0f;
        state->leftSlip=0.0f;
        state->rightSlip=0.0f;
    }
    replaySnapshotMoveFloat(snapshot,&state->forwardSpeed);
    replaySnapshotMoveFloat(snapshot,&state->speed);
    replaySnapshotMoveData(snapshot,&state->gear,1,-1,0);
    replaySnapshotMoveFloat(snapshot,&state->effectiveThrottle);
    replaySnapshotMoveFloat(snapshot,&state->engineRpm);
    replaySnapshotMoveInteger(snapshot,&state->clutchEngaged,1);
    replaySnapshotMoveInteger(snapshot,&state->pursuitMode,1);
    replaySnapshotMoveFloat(snapshot,&state->performanceScale);
    replaySnapshotMoveInteger(snapshot,&state->tractionAssistPhase,4);
    replaySnapshotMoveFloat(snapshot,&state->tractionAssistTime);
    replaySnapshotMoveFloat(snapshot,&state->tractionAssistScale);
    if(snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK) {
        // XYZ is serialized; rebuild native point/direction slots before
        // checking the new state. Live-state finite validation stays strict.
        state->position[3]=state->centerPosition[3]=1.0f;
        state->velocity[3]=state->linearAcceleration[3]=0.0f;
        state->angularVelocity[3]=state->angularAcceleration[3]=0.0f;
    }
    // Reject malformed restored inputs before they reach the wheel-pose
    // trigonometry/translation producers. Old derived caches are intentionally
    // excluded here: the healthy snapshot is about to replace them.
    if(snapshot->restoring && snapshot->result==REPLAY_SNAPSHOT_OK &&
        !VehicleStateInputsFinite(state))snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;
    if ((snapshot->recording || snapshot->restoring) && snapshot->result==REPLAY_SNAPSHOT_OK) {
        if(!state->wheelInfo) {
            snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
            replaySnapshotEndSection(snapshot);return;
        }
        rotationMatrixIdentity(state->velocityFrame);
        state->velocityFrameValid=0;
        state->activeContactCount=0;
        for (int wheel=0;wheel<4;++wheel)
            state->activeContactCount+=state->wheelContact[wheel];
        vehicleStateUpdateWheelPoses(state);
        if(snapshot->restoring && !vehicleStateIsFinite(state))
            snapshot->result=REPLAY_SNAPSHOT_INVALID_TRANSFER;
    }
    replaySnapshotEndSection(snapshot);
}
