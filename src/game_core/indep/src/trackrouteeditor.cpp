#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/eMath.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bWare.hpp"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// The source mode3 editor remains separate from unported authoring modes.
static float lengthThree(const float value[4])
{
    const float x=value[0]*value[0],y=value[1]*value[1],z=value[2]*value[2];
    return sqrtf((x+y)+z);
}

static void normalizeThree(float value[4])
{
    const float inverse=finiteRatio(1.0f,lengthThree(value));
    for(int axis=0;axis<3;++axis)value[axis]*=inverse;
    value[3]=0.0f;
}

static const CarRecoveryGroup* findGroup(const TrackRouteEditor* editor,int32_t id)
{
    if(id<0||id>=100)return NULL;
    const CarRecoveryPointRegistry* registry=editor->context.registry;
    for(size_t index=0;index<registry->groupCount;++index)
        if(registry->groups[index].id==id)return &registry->groups[index];
    return NULL;
}

static CarRecoveryMutableGroup* mutableGroup(TrackRouteEditor* editor,const CarRecoveryGroup* group)
{
    for(size_t index=0;index<editor->context.mutableGroupCount;++index){
        CarRecoveryMutableGroup* candidate=&editor->context.mutableGroups[index];
        if(candidate->group==group&&candidate->poses==group->poses)return candidate;
    }
    return NULL;
}

void trackRouteEditorSettingsInitialize(TrackRouteEditorSettings* settings)
{
    settings->selectedGroup=-1;
    settings->activeCameraDraws=0;
    settings->selectionRevision=0;
}

TrackRouteEditor* trackRouteEditorCreate(const TrackRouteEditorContext* context,
    TrackRouteEditorResult* result)
{
    if(result)*result=TRACK_ROUTE_EDITOR_INVALID_BINDING;
    if(!context||!context->registry||!context->terrain||!context->clock||
       !context->input||!context->settings||!context->markers||
       !context->mutableGroups||context->mutableGroupCount!=context->registry->groupCount)
        return NULL;
    for(size_t index=0;index<context->mutableGroupCount;++index){
        const CarRecoveryMutableGroup* group=&context->mutableGroups[index];
        if(group->group!=&context->registry->groups[index]||group->poses!=group->group->poses||
           group->group->poseCount<1||group->group->id<0||group->group->id>=100)return NULL;
    }
    TrackRouteEditor* editor=static_cast<TrackRouteEditor*>(calloc(1,sizeof(*editor)));
    if(!editor){if(result)*result=TRACK_ROUTE_EDITOR_ALLOCATION_FAILURE;return NULL;}
    editor->context=*context;
    static const char* const modelNames[4]={
        "MARKER_BOUNDARY","MARKER_HEART","MARKER_ROUTEPATHEDGE","MARKER_ROUTEPATHEDGE"};
    static const char* const firstNames[3]={"MARKER_LTBLUE","MARKER_ORANGE","MARKER_BROWN"};
    static const char* const lastNames[3]={"MARKER_DARKTEAL","MARKER_RED","MARKER_GOLD"};
    for(int index=0;index<3;++index){
        const char* original[4]={"MARKER_BLUE","MARKER_RED","MARKER_PINKBLUE","MARKER_PINKBLUE"};
        const char* replacement[4]={firstNames[index],firstNames[index],firstNames[index],lastNames[index]};
        for(int family=0;family<4;++family){
            EditorMarkerReplacement* table=&editor->replacements[family][index];
            table->originalKey=hashString(original[family]);
            table->replacementKey=hashString(replacement[family]);
            editorMarkerModelInitialize(&editor->markers[family][index],context->markers,
                hashString(modelNames[family]));
            editorMarkerModelSetReplacement(&editor->markers[family][index],table);
        }
    }
    if(result)*result=TRACK_ROUTE_EDITOR_OK;
    return editor;
}

void trackRouteEditorDispose(TrackRouteEditor* editor)
{
    if(!editor)return;
    for(int index=0;index<3;++index)
        for(int family=0;family<4;++family)editorMarkerModelDispose(&editor->markers[family][index]);
    /* Member destruction visits event records in reverse declaration order. */
    for(int index=17;index>=0;--index){
        TrackRouteEditorInput* input=&editor->inputs[index];
        if(input->event){gameInputUnregister(editor->context.input,input->device,input->event);input->event=0;}
    }
    free(editor);
}

void trackRouteEditorFindNear(TrackRouteEditor* editor,const float point[4])
{
    float radius=40.0f;
    for(int pass=0;pass<2;++pass){
        int32_t selected=editor->context.settings->selectedGroup;
        for(int attempt=0;attempt<99;++attempt){
            selected=(selected+1)%100;
            const CarRecoveryGroup* group=findGroup(editor,selected);
            if(!group)continue;
            const CarRecoveryPose* pose=carRecoveryFindInGroup(editor->context.registry,
                group,point,radius,0.0f,0);
            const int32_t ownedIndex=carRecoveryOwnedPoseIndex(group,pose);
            if(ownedIndex>=0){
                editor->selectedPose=ownedIndex;
                editor->context.settings->selectedGroup=selected;
                ++editor->context.settings->selectionRevision;
                return;
            }
        }
        radius=999999.0f;
    }
}

void trackRouteEditorUpdateRoadPose(TrackRouteEditor* editor,int32_t groupId,int32_t poseIndex)
{
    const CarRecoveryGroup* group=findGroup(editor,groupId);
    if(!group || !group->poses || group->poseCount<=0){editor->validPose=0;return;}
    const int32_t index=poseIndex%group->poseCount;
    if(index<0){editor->validPose=0;return;}
    const CarRecoveryPose* pose=&group->poses[index];
    float anchor[4],matrix[16],relative[4];
    driverRoadPointPosition(pose,anchor,0.0f,0.0f);
    TerrainSurface surface;
    terrainSurfaceInitialize(&surface,editor->context.terrain);
    anchor[2]=terrainSurfaceElevation(&surface,anchor,NULL,NULL,NULL);
    const uint16_t heading=vectorAtan2Angle(pose->direction[0],pose->direction[1]);
    const uint16_t difference=(uint16_t)(heading-editor->roadHeading);
    rotationMatrixIdentity(matrix);
    motionMatrixRotateYaw(matrix,matrix,difference);
    for(int axis=0;axis<4;++axis)relative[axis]=editor->desiredEye[axis]-editor->roadAnchor[axis];
    vectorMatrixTransform(relative,matrix,relative);
    for(int axis=0;axis<4;++axis)editor->desiredEye[axis]=relative[axis]+anchor[axis];
    vectorMatrixTransform(editor->desiredDirection,matrix,editor->desiredDirection);
    memcpy(editor->roadAnchor,anchor,sizeof(anchor));
    editor->roadHeading=heading;
    editor->validPose=1;
    terrainSurfaceDispose(&surface);
}

void trackRouteEditorResetCamera(TrackRouteEditor* editor)
{
    const float eye[4]={-12.0f,0.0f,4.0f,0.0f};
    const float direction[4]={0.9599999785423279f,0.0f,-0.23000000417232513f,0.0f};
    memcpy(editor->desiredEye,eye,sizeof(eye));
    memcpy(editor->desiredDirection,direction,sizeof(direction));
    normalizeThree(editor->desiredDirection);
    memset(editor->roadAnchor,0,sizeof(editor->roadAnchor));
    editor->roadHeading=0;
    trackRouteEditorUpdateRoadPose(editor,editor->context.settings->selectedGroup,editor->selectedPose);
}

TrackRouteEditorResult trackRouteEditorActivate(TrackRouteEditor* editor,const float eye[4])
{
    if(!editor||!eye)return TRACK_ROUTE_EDITOR_INVALID_BINDING;
    if(editor->mode==3)return TRACK_ROUTE_EDITOR_OK;
    const TrackRouteEditorResult status=trackRouteEditorActivateInput(editor,1);
    if(status!=TRACK_ROUTE_EDITOR_OK)return status;
    editor->mode=3;
    ++editor->context.settings->activeCameraDraws;
    if(editor->context.settings->selectedGroup<0)trackRouteEditorFindNear(editor,eye);
    trackRouteEditorResetCamera(editor);
    memcpy(editor->currentEye,editor->desiredEye,sizeof(editor->currentEye));
    memcpy(editor->currentDirection,editor->desiredDirection,sizeof(editor->currentDirection));
    memset(editor->eyeVelocity,0,sizeof(editor->eyeVelocity));
    memset(editor->directionVelocity,0,sizeof(editor->directionVelocity));
    return TRACK_ROUTE_EDITOR_OK;
}

void trackRouteEditorDeactivate(TrackRouteEditor* editor)
{
    if(!editor||editor->mode!=3)return;
    editor->context.settings->selectedGroup=-1;
    --editor->context.settings->activeCameraDraws;
    editor->mode=0;
    trackRouteEditorDeactivateInput(editor);
}

static int32_t consumePress(TrackRouteEditorInput* input)
{
    if(!input->pending)return 0;
    input->pending=0;
    return 1;
}

TrackRouteEditorResult trackRouteEditorTickCamera(TrackRouteEditor* editor)
{
    if(!editor)return TRACK_ROUTE_EDITOR_INVALID_BINDING;
    const float elapsed=editor->context.clock->worldTimeElapsed;
    if(consumePress(&editor->inputs[11]))trackRouteEditorFindNear(editor,editor->roadAnchor);
    const CarRecoveryGroup* group=findGroup(editor,editor->context.settings->selectedGroup);
    if(!group)return TRACK_ROUTE_EDITOR_OK;
    CarRecoveryMutableGroup* writable=mutableGroup(editor,group);
    if(!writable)return TRACK_ROUTE_EDITOR_INVALID_BINDING;
    if(consumePress(&editor->inputs[14]))trackRouteEditorResetCamera(editor);
    const int32_t previous=editor->selectedPose;
    if(trackRouteEditorInputRepeat(&editor->inputs[12],elapsed,0.4000000059604645f,0.05000000074505806f))++editor->selectedPose;
    if(trackRouteEditorInputRepeat(&editor->inputs[13],elapsed,0.4000000059604645f,0.05000000074505806f))--editor->selectedPose;
    if(editor->selectedPose>=group->poseCount)editor->selectedPose=0;
    else if(editor->selectedPose<0)editor->selectedPose=group->poseCount-1;
    CarRecoveryPose* pose=&writable->poses[editor->selectedPose];
    const uint8_t firstWidth=editor->inputs[15].raw,secondWidth=editor->inputs[16].raw;
    if(firstWidth||secondWidth){
        float first=pose->halfWidths[0],second=pose->halfWidths[1];
        if(previous!=editor->selectedPose){
            if(secondWidth)first=writable->poses[previous].halfWidths[0];
            if(firstWidth)second=writable->poses[previous].halfWidths[1];
        }
        const float step=elapsed*10.0f;
        if(editor->inputs[9].raw){if(secondWidth)first+=step;else second-=step;}
        else if(editor->inputs[10].raw){if(secondWidth)first-=step;else second+=step;}
        trackRouteEditorSetWidths(pose,first,second);
    }else if(editor->inputs[17].raw){
        float across=0.0f,along=0.0f;
        if(editor->inputs[9].raw)across=-(elapsed*5.0f);
        if(editor->inputs[10].raw)across+=elapsed*5.0f;
        if(editor->inputs[7].raw)along=elapsed*5.0f;
        if(editor->inputs[8].raw)along-=elapsed*5.0f;
        if(across!=0.0f||along!=0.0f){
            float original[4],moved[4];
            driverRoadPointPosition(pose,original,0.0f,0.0f);
            driverRoadPointPosition(pose,moved,along,across);
            if(trackRouteEditorMovePoint(writable,editor->selectedPose,moved))
                trackRouteEditorMovePoint(writable,editor->selectedPose,original);
        }
    }
    float forward[4]={editor->desiredDirection[0],editor->desiredDirection[1],0,0};
    float side[4]={editor->desiredDirection[1],-editor->desiredDirection[0],0,0};
    const float up[4]={0,0,1,0};
    normalizeThree(forward);normalizeThree(side);
    float speed=elapsed*35.0f;
    if(editor->inputs[4].raw)speed*=4.0f;
    float nextEye[4];memcpy(nextEye,editor->desiredEye,sizeof(nextEye));
    const uint8_t longitudinal=editor->inputs[2].raw,lateral=editor->inputs[3].raw;
    const float translations[6]={longitudinal>192?-speed:0.0f,longitudinal<64?speed:0.0f,
        lateral>192?speed:0.0f,lateral<64?-speed:0.0f,
        editor->inputs[5].raw?speed*0.5f:0.0f,editor->inputs[6].raw?(-speed)*0.5f:0.0f};
    const float* axes[6]={forward,forward,side,side,up,up};
    for(int step=0;step<6;++step){
        if(translations[step]==0.0f)continue;
        for(int axis=0;axis<4;++axis){const float change=axes[step][axis]*translations[step];nextEye[axis]=change+nextEye[axis];}
    }
    float displacement[4];
    for(int axis=0;axis<4;++axis)displacement[axis]=nextEye[axis]-editor->desiredEye[axis];
    const float distance=lengthThree(displacement);
    if(0.0f<distance){
        TerrainSurface surface;terrainSurfaceInitialize(&surface,editor->context.terrain);
        const float nextHeight=terrainSurfaceElevation(&surface,nextEye,NULL,NULL,NULL);
        const float previousHeight=terrainSurfaceElevation(&surface,editor->desiredEye,NULL,NULL,NULL);
        const float change=nextHeight-previousHeight;
        if(fabsf(change/distance)<0.20000000298023224f)nextEye[2]+=change;
        memcpy(editor->desiredEye,nextEye,sizeof(nextEye));
        terrainSurfaceDispose(&surface);
    }
    const float degrees=elapsed*90.0f;
    const float angular=degrees*65536.0f;
    uint16_t step=(uint16_t)(convertFloatToInt32(angular)/360);
    if(editor->inputs[4].raw)step=(uint16_t)(step*4u);
    uint16_t yaw=0,pitch=step;
    if(editor->inputs[0].raw>192)yaw=(uint16_t)(0u-step);
    if(editor->inputs[0].raw<64)yaw=step;
    if(editor->inputs[1].raw<193)pitch=0;
    if(editor->inputs[1].raw<64)pitch=(uint16_t)(0u-step);
    if(yaw){
        float matrix[16];rotationMatrixIdentity(matrix);motionMatrixRotateYaw(matrix,matrix,yaw);
        vectorMatrixTransform(editor->desiredDirection,matrix,editor->desiredDirection);
    }
    if(pitch){editor->desiredDirection[2]+=rotationSineAngle(pitch);normalizeThree(editor->desiredDirection);}
    trackRouteEditorUpdateRoadPose(editor,editor->context.settings->selectedGroup,editor->selectedPose);
    float eyeError[4],directionError[4];
    for(int axis=0;axis<4;++axis){eyeError[axis]=editor->desiredEye[axis]-editor->currentEye[axis];directionError[axis]=editor->desiredDirection[axis]-editor->currentDirection[axis];}
    if(lengthThree(eyeError)>200.0f||lengthThree(directionError)>1.0f){
        memcpy(editor->currentEye,editor->desiredEye,sizeof(editor->currentEye));
        memcpy(editor->currentDirection,editor->desiredDirection,sizeof(editor->currentDirection));
        memset(editor->eyeVelocity,0,sizeof(editor->eyeVelocity));
        memset(editor->directionVelocity,0,sizeof(editor->directionVelocity));
    }else{
        const float eyeSpring=elapsed*50.0f,eyeDamping=elapsed*-12.0f;
        const float directionSpring=elapsed*40.0f,directionDamping=elapsed*-10.0f;
        for(int axis=0;axis<4;++axis){
            const float pull=eyeError[axis]*eyeSpring,drag=editor->eyeVelocity[axis]*eyeDamping;
            editor->eyeVelocity[axis]+=drag+pull;
            const float travel=editor->eyeVelocity[axis]*elapsed;
            editor->currentEye[axis]=travel+editor->currentEye[axis];
            const float turn=directionError[axis]*directionSpring,angularDrag=editor->directionVelocity[axis]*directionDamping;
            editor->directionVelocity[axis]+=angularDrag+turn;
            const float rotation=editor->directionVelocity[axis]*elapsed;
            editor->currentDirection[axis]=rotation+editor->currentDirection[axis];
        }
    }
    return TRACK_ROUTE_EDITOR_OK;
}

void trackRouteEditorInputDispatch(TrackRouteEditorInput* input,uint8_t value)
{
    input->raw=value;
    if(value){input->pending=1;input->repeatElapsed=0.0f;input->nextRepeat=0.0f;}
}

TrackRouteEditorResult trackRouteEditorActivateInput(TrackRouteEditor* editor,int32_t device)
{
    if(editor->inputRegistered)return TRACK_ROUTE_EDITOR_OK;
    GameInputRouter* router=editor->context.input;
    if(!router->registrations||router->count>router->capacity||
       router->capacity-router->count<18||router->nextOrder>UINT64_MAX-18)
        return TRACK_ROUTE_EDITOR_INPUT_CAPACITY;
    static const int order[18]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,16,15,17};
    for(int index=0;index<18;++index){
        const int selected=order[index];
        TrackRouteEditorInput* input=&editor->inputs[selected];
        input->event=0x4a+selected;
        input->device=device;
        GameInputTarget target;
        target.kind=GAME_INPUT_TRACK_ROUTE_EDITOR;
        target.owner.editorInput=input;
        gameInputRegister(router,device,input->event,target);
        input->raw=0;input->pending=0;
        input->repeatElapsed=0.0f;input->nextRepeat=0.0f;
    }
    editor->inputRegistered=1;
    return TRACK_ROUTE_EDITOR_OK;
}

void trackRouteEditorDeactivateInput(TrackRouteEditor* editor)
{
    if(!editor->inputRegistered)return;
    for(int index=0;index<18;++index){
        TrackRouteEditorInput* input=&editor->inputs[index];
        if(input->event){gameInputUnregister(editor->context.input,input->device,input->event);input->event=0;}
    }
    editor->inputRegistered=0;
}

int32_t trackRouteEditorInputRepeat(TrackRouteEditorInput* input,float elapsed,
    float initialDelay,float repeatDelay)
{
    if(input->raw==0&&input->pending==0)return 0;
    const float accumulated=input->repeatElapsed+elapsed;
    input->pending=0;
    input->repeatElapsed=accumulated;
    if(!(input->nextRepeat<=accumulated))return 0;
    if(input->nextRepeat==0.0f)input->nextRepeat=initialDelay;
    else input->nextRepeat=accumulated+repeatDelay;
    return 1;
}

int32_t trackRouteEditorGetCameraPosition(const TrackRouteEditor* editor,
    float eye[4],float direction[4])
{
    if(editor->mode!=3||!editor->validPose)return 0;
    for(int axis=0;axis<4;++axis)eye[axis]=editor->currentEye[axis];
    for(int axis=0;axis<4;++axis)direction[axis]=editor->currentDirection[axis];
    return 1;
}
