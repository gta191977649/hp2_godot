// Camera matrix operations. PAL156BE0/156CD0/156D68.
// Native typed storage owns the camera matrix and its direction vectors.
#include "CameraMover.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/CameraMover.hpp"
#include "bware/inc/bMath.hpp"
#include <cstring>
#include <cmath>

void carCinematicViewInitialize(CarCinematicServices* services,CarCinematicView* view)
{
    // Camera() source frame: focus10, near0.5, far10000, FOV0x36FB.
    // Native zeroing defines formerly unspecified padding and vector W.
    *view={};view->elapsed=1.0f;view->resetHistory=1;
    view->current.focusDistance=10.0f;
    view->current.nearClip=0.5f;view->current.farClip=10000.0f;
    view->current.fieldOfView=0x36fb;
    const float matrix[16]={1,0,0,0, 0,-1,0,0, 0,0,-1,100, 0,0,1200,1};
    // The authored Camera matrix is column-major in the native e-API;
    // preserve all sixteen PAL stack lanes, including matrix[11]=100.
    carCinematicViewPublish(services,view,matrix,1.0f);
    carCinematicViewPublish(services,view,matrix,1.0f);
}

void carCinematicViewPublish(CarCinematicServices* services,CarCinematicView* view,
                              const float matrix[16],float elapsed)
{
    if(services->suppressPublication)return;
    view->previous=view->current;
    view->elapsed=elapsed;
    if(services->override.enabled&&!services->suppressOverride){
        services->overrideWasUsed=1;
        memcpy(view->current.matrix,services->override.matrix,sizeof(view->current.matrix));
        for(int axis=0;axis<4;++axis)view->current.matrix[12+axis]*=0.009999999776482582f;
        view->current.matrix[15]=1.0f;
    }else{
        if(services->overrideWasUsed)services->overrideWasUsed=0;
        memcpy(view->current.matrix,matrix,sizeof(view->current.matrix));
    }
    float transpose[16],translated[4],translation[4];
    vectorMatrixTranspose(transpose,view->current.matrix);
    transpose[3]=transpose[7]=transpose[11]=0.0f;
    for(int axis=0;axis<3;++axis)translation[axis]=view->current.matrix[12+axis];
    translation[3]=0.0f;
    vectorMatrixTransform(translated,transpose,translation);
    for(int axis=0;axis<3;++axis){
        view->current.eye[axis]=0.0f-translated[axis];view->current.forward[axis]=transpose[8+axis];
    }
    view->current.eye[3]=view->current.forward[3]=0.0f;
    carCinematicNormalizeThree(view->current.forward);
    for(int axis=0;axis<3;++axis){
        const float distance=view->current.forward[axis]*view->current.focusDistance;
        view->current.target[axis]=distance+view->current.eye[axis];
    }
    view->current.target[3]=0.0f;
    if(view->resetHistory){view->resetHistory=0;view->previous=view->current;}
    const float inverse=finiteRatio(1.0f,elapsed);
    for(int axis=0;axis<3;++axis){
        view->eyeVelocity[axis]=(view->current.eye[axis]-view->previous.eye[axis])*inverse;
        view->forwardVelocity[axis]=(view->current.forward[axis]-view->previous.forward[axis])*inverse;
        view->targetVelocity[axis]=(view->current.target[axis]-view->previous.target[axis])*inverse;
    }
    view->eyeVelocity[3]=view->forwardVelocity[3]=view->targetVelocity[3]=0.0f;
    view->focusVelocity=(view->current.focusDistance-view->previous.focusDistance)*inverse;
    view->minimumVisibilityVelocity=(view->current.minimumVisibility-view->previous.minimumVisibility)*inverse;
    view->maximumVisibilityVelocity=(view->current.maximumVisibility-view->previous.maximumVisibility)*inverse;
    view->nearClipVelocity=(view->current.nearClip-view->previous.nearClip)*inverse;
    view->farClipVelocity=(view->current.farClip-view->previous.farClip)*inverse;
    const uint16_t delta=(uint16_t)(view->current.fieldOfView-view->previous.fieldOfView);
    view->fieldOfViewVelocity=(uint16_t)convertFloatToInt32(inverse*(float)delta);
}
