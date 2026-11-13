#include "Chopper.hpp"
#include "indep/inc/TrackRoute.hpp"
#include "indep/inc/Car.hpp"
#include "indep/inc/CarDriver.hpp"
#include <climits>
#include "indep/inc/Race.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/World.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/eMath.hpp"
#include "indep/inc/AICarController.hpp"
#include "indep/inc/CameraMover.hpp"
#include "indep/inc/Pursuit.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include "indep/inc/CameraCoordinator.hpp"
#include "bware/inc/bWare.hpp"
#include <cstdlib>
#include "indep/inc/Car.hpp"
#include "bware/inc/bMath.hpp"


const float* carOrbitCameraTargetPosition(const CarOrbitCamera* camera)
{
    if(!camera)return NULL;
    if(camera->input.selectedCar){
        const CarMovement* movement=carOrbitCameraSelectedMovement(camera);
        return movement&&movement->bindings.state?movement->bindings.state->centerPosition:NULL;
    }
    return camera->base.view?camera->base.view->current.target:NULL;
}

const float* carAirborneCameraFocusPosition(const CarAirborneCamera* camera)
{
    return camera->movement ? camera->movement->bindings.state->centerPosition : camera->base.view->current.target;
}

void carAirborneCameraWeightedDirection(const CarAirborneCamera* camera, float output[4],
    const float displacement[4], const float velocity[4])
{
    for (int axis = 0; axis < 4; ++axis) {
        const float damping = velocity[axis] * camera->velocityWeight;
        const float attraction = displacement[axis] * camera->directionWeight;
        output[axis] = damping + attraction;
    }
}

int32_t carPathCameraFinished(const CarPathCamera* camera)
{
    const int eyeFinished = camera->items.playback.eyeAdvanceRate == 0.0f ||
        camera->eyeParameter >= (float)(camera->items.eyeCount - 1);
    const int targetFinished = camera->items.playback.targetAdvanceRate == 0.0f ||
        camera->targetParameter >= (float)(camera->items.targetCount - 1);
    return eyeFinished && targetFinished;
}

int32_t carChaseCameraPitchStep(uint16_t difference)
{
    const int32_t signedDifference = difference > 32768 ? (int32_t)difference - 65536 : difference;
    if (-7 <= signedDifference && signedDifference <= 7) return -signedDifference;
    return -signedDifference / 8;
}

void carChaseCameraSetViewIndex(CarChaseCamera* camera, int32_t index) { camera->viewIndex = index; }

int32_t carBumperCameraFinished(const CarBumperCamera* camera)
{
    const int eyeFinished=camera->items.playback.eyeAdvanceRate==0.0f ||
        camera->eyeParameter>=(float)(camera->items.eyeCount-1);
    const int targetFinished=camera->items.playback.targetAdvanceRate==0.0f ||
        camera->targetParameter>=(float)(camera->items.targetCount-1);
    return eyeFinished&&targetFinished;
}


uint16_t carChaseCameraTargetQuadrant(const CarMovement* first,
    const CarMovement* second, const float orientation[16])
{
    const float* a = first->bindings.state->position;
    const float* b = second->bindings.state->position;
    float delta[3];
    for (int axis = 0; axis < 3; ++axis) delta[axis] = b[axis] - a[axis];
    const float xx = delta[0] * delta[0], yy = delta[1] * delta[1], zz = delta[2] * delta[2];
    const float inverse = finiteRatio(1.0f, sqrtf((xx + yy) + zz));
    for (int axis = 0; axis < 3; ++axis) delta[axis] *= inverse;
    const float forward = (orientation[0] * delta[0] + orientation[1] * delta[1]) + orientation[2] * delta[2];
    const float side = (orientation[4] * delta[0] + orientation[5] * delta[1]) + orientation[6] * delta[2];
    if (0.707099974155426f < forward) return 0;
    if (forward < -0.707099974155426f) return 32768;
    return side < 0.0f ? 16384 : 49152;
}

CarMovement* carChaseCameraRelatedVehicle(const PursuitManager* manager,
    const CarMovement* movement, int32_t* isPursuer)
{
    *isPursuer = 0;
    if (!manager) return NULL;
    PursuitCollisionEntry* entry = pursuitCollisionFind(manager->collision, movement);
    if (entry) {
        if (!entry->group) return NULL;
        *isPursuer = 1;
        return entry->group->target;
    }
    entry = carCameraClosestPursuer(manager, movement);
    return entry ? entry->movement : NULL;
}

#include "CameraMover.hpp"
#include "bware/inc/bWare.hpp"
#include "eMath.hpp"
#include "CarSounds.hpp"
#include "bsound/include/bSound.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>

namespace hp2 {

namespace {
// PAL330450 (GP-54A0): shared cadence, not one counter per camera.
std::uint32_t selectCarQuaternionUpdates{};

// PAL15AAB0 and15AB58. These operations have different expression order
// from bQuaternion's general helpers; preserve the camera's actual bodies.
void MyQuaternionMultiply(float output[4],const float first[4],const float second[4])
{
    const float x=((first[1]*second[2]-first[2]*second[1])+first[3]*second[0])+first[0]*second[3];
    const float y=((first[2]*second[0]-first[0]*second[2])+first[3]*second[1])+first[1]*second[3];
    const float z=((first[0]*second[1]-first[1]*second[0])+first[3]*second[2])+first[2]*second[3];
    const float w=((first[3]*second[3]-first[0]*second[0])-first[1]*second[1])-first[2]*second[2];
    output[0]=x;output[1]=y;output[2]=z;output[3]=w;
}
void NormalizeQuaternion(float value[4])
{
    const float length=std::sqrt(((value[0]*value[0]+value[1]*value[1])+value[2]*value[2])+value[3]*value[3]);
    for(int axis=0;axis<4;++axis)value[axis]/=length;
}
}

const CarCinematicMethods SelectCarCameraMover::methods_={
    &SelectCarCameraMover::DisposeOwned,&SelectCarCameraMover::AdvanceOwned,nullptr,nullptr};

SelectCarCameraMover::SelectCarCameraMover(CarCinematicServices& services,std::int32_t viewNumber,
    WorldAudioRequestQueue* audio)
{
    Require(carCinematicBaseInitialize(&base_,&services,viewNumber,11)==CAR_CINEMATIC_OK,
        "Select-car camera has no live source view");
    carCinematicBindOwner(&base_,this,&methods_);
    try {
        if(carSoundGlobals.policy.audioSystemEnabled) {
            // PAL15A754 reads the same process bSound used by the frontend.
            // Borrow that actual application owner; a separate function-static
            // bSound has neither this queue nor its completed FrontEnd bank.
            audio_=audio;
            Require(audio_ && audio_->library && audio_->library->Driver()==audio_,
                "Select-car camera has no actual application sound owner");
            auto* region=audio_->library->Region("FrontEnd");
            Require(region,"Select-car camera lacks the loaded FrontEnd sound region");
            const char* names[]={"MenuCarDisplayOn","MenuCarDisplayOff","MenuCarDisplayActive","MenuCarOrbitBump"};
            for(std::size_t index=0;index<samples_.size();++index) {
                Require(worldAudioCopyVoice(audio_,region->Sample(names[index]),&samples_[index])==WORLD_AUDIO_OK,
                    "Select-car camera could not create its original private sound sample");
                Require(worldAudioSetVoiceCategory(audio_,samples_[index],3)==WORLD_AUDIO_OK,
                    "Select-car camera sound category could not be submitted");
            }
        }
    } catch(...) {
        for(auto sample:samples_)if(sample.instance && worldAudioRetireVoice(audio_,sample)!=WORLD_AUDIO_OK)std::terminate();
        carCinematicBaseDispose(&base_);throw;
    }
}
SelectCarCameraMover::~SelectCarCameraMover()
{
    for(auto sample:samples_)if(sample.instance && worldAudioRetireVoice(audio_,sample)!=WORLD_AUDIO_OK)std::terminate();
    carCinematicBaseDispose(&base_);
}
void SelectCarCameraMover::DisposeOwned(void* camera) {delete static_cast<SelectCarCameraMover*>(camera);}
CarCinematicResult SelectCarCameraMover::AdvanceOwned(void* camera)
{
    static_cast<SelectCarCameraMover*>(camera)->Update();return CAR_CINEMATIC_OK;
}
void SelectCarCameraMover::Play(std::size_t sample,float volume)
{
    Require(audio_ && samples_.at(sample).instance,"Enabled select-car sound lost its original voice");
    Require(worldAudioPlayVoice(audio_,samples_[sample],volume,0.0f,0.0f)==WORLD_AUDIO_OK,
        "Select-car sample playback could not be submitted");
}
void SelectCarCameraMover::Bump(float speed,float scale)
{
    const float volume=std::fabs(speed)*scale;
    if(volume>0.10700000077486038f && carSoundGlobals.policy.audioSystemEnabled)Play(3,volume);
}
void SelectCarCameraMover::SetMode(std::int32_t mode)
{
    mode_=mode;
    if(!carSoundGlobals.policy.audioSystemEnabled)return;
    if(mode==0) {
        Play(1,1.0f);
        Require(worldAudioStopInstance(audio_,samples_[2].instance)==WORLD_AUDIO_OK,
            "Select-car orbit sound could not be stopped");
    } else if(mode==1) {Play(2,0.0f);Play(0,1.0f);}
}
void SelectCarCameraMover::SetRadiusStats(float radius) {radiusStats_=radius;}
void SelectCarCameraMover::SetOffsetStats(const float* offset) {std::copy_n(offset,3,offsetStats_.begin());}
void SelectCarCameraMover::SetOffsetOrbit(const float* offset) {std::copy_n(offset,3,offsetOrbit_.begin());}
void SelectCarCameraMover::SetFOVStats(std::uint16_t value) {fieldOfViewStats_=value;}
void SelectCarCameraMover::SetVertAngleOrbit(std::uint16_t angle) {verticalOrbit_=static_cast<std::uint16_t>(0u-angle);}
void SelectCarCameraMover::SetOrientationStats(std::uint16_t horizontal,std::uint16_t vertical)
{
    horizontalStats_=horizontal;verticalStats_=static_cast<std::uint16_t>(0u-vertical);
    float matrix[16];rotationMatrixIdentity(matrix);motionMatrixRotateX(matrix,matrix,0);
    motionMatrixRotateY(matrix,matrix,32768);motionMatrixRotateYaw(matrix,matrix,horizontal);
    vectorQuaternionFromMatrixCopy(orientationStats_.data(),matrix);
}
void SelectCarCameraMover::Update()
{
    const float elapsed=base_.services->clock->realTimeElapsed;
    const auto approach=[&](float& current,float desired,float increase,float decrease) {
        if(current<desired) {current+=elapsed*increase;if(desired<current)current=desired;}
        else if(desired<current) {current-=elapsed*decrease;if(current<desired)current=desired;}
    };
    approach(horizontalSpeed_,desiredHorizontalSpeed,45000.0f,45000.0f);
    approach(verticalSpeed_,desiredVerticalSpeed,23700.0f,23700.0f);
    approach(radiusSpeed_,desiredRadiusSpeed,17.020000457763672f,17.020000457763672f);
    float matrix[16];rotationMatrixIdentity(matrix);
    const auto angle=static_cast<std::uint16_t>(convertFloatToInt32(modeParam_*32768.0f));
    float fraction=1.0f-(vectorCosineAngle(angle)+1.0f)*0.5f;
    if(fraction>1.0f)fraction=1.0f;
    if(fraction<0.0f)fraction=0.0f;
    if(modeParam_==0.0f) {
        float orientation[16];rotationMatrixIdentity(orientation);
        verticalOrbit_=50000;radiusOrbit_=13.199999809265137f;
        motionMatrixRotateX(orientation,orientation,0);motionMatrixRotateY(orientation,orientation,32768);
        motionMatrixRotateYaw(orientation,orientation,horizontalStats_);
        vectorQuaternionFromMatrixCopy(orientationOrbit_.data(),orientation);
    }
    if(mode_==1) {
        float rotation[16],delta[4];rotationMatrixIdentity(rotation);
        motionMatrixRotateYaw(rotation,rotation,static_cast<std::uint16_t>(convertFloatToInt32(horizontalSpeed_*elapsed)));
        vectorQuaternionFromMatrixCopy(delta,rotation);
        MyQuaternionMultiply(orientationOrbit_.data(),delta,orientationOrbit_.data());
        std::int32_t update;std::memcpy(&update,&selectCarQuaternionUpdates,sizeof(update));
        ++selectCarQuaternionUpdates;
        if(update%60==0)NormalizeQuaternion(orientationOrbit_.data());
    }
    float interpolated[4],orientation[16];
    vectorQuaternionInterpolate(interpolated,orientationStats_.data(),orientationOrbit_.data(),fraction);
    vectorMatrixFromQuaternion(orientation,interpolated);
    float offset[3];for(int axis=0;axis<3;++axis)offset[axis]=offsetStats_[axis]+(offsetOrbit_[axis]-offsetStats_[axis])*fraction;
    motionMatrixTranslate(matrix,matrix,offset);eMulMatrix(matrix,matrix,orientation);
    if(mode_==1) {
        verticalOrbit_=static_cast<std::uint16_t>(verticalOrbit_+convertFloatToInt32(verticalSpeed_*elapsed));
        if(verticalOrbit_>60331) {
            verticalOrbit_=60331;verticalSpeed_=-verticalSpeed_*0.16500000655651093f;
            Bump(verticalSpeed_,0.0006000000284984708f);
        }
        if(verticalOrbit_<50000) {
            verticalOrbit_=50000;verticalSpeed_=-verticalSpeed_*0.16500000655651093f;
            Bump(verticalSpeed_,0.0006000000284984708f);
        }
    }
    const float vertical=static_cast<float>(verticalStats_)+fraction*static_cast<float>(static_cast<int>(verticalOrbit_)-verticalStats_);
    motionMatrixRotateX(matrix,matrix,static_cast<std::uint16_t>(convertFloatToInt32(vertical)));
    if(mode_==1) {
        radiusOrbit_+=radiusSpeed_*elapsed;
        if(radiusOrbit_>15.5f) {
            radiusOrbit_=15.5f;radiusSpeed_*= -0.16500000655651093f;Bump(radiusSpeed_,0.800000011920929f);
        }
        if(radiusOrbit_<10.949999809265137f) {
            radiusOrbit_=10.949999809265137f;radiusSpeed_*= -0.16500000655651093f;Bump(radiusSpeed_,0.800000011920929f);
        }
        const float volume=std::max(std::max(std::fabs(horizontalSpeed_)*0.00009999999747378752f,
            std::fabs(verticalSpeed_)*0.00015999999595806003f),std::fabs(radiusSpeed_)*0.14800000190734863f);
        if(carSoundGlobals.policy.audioSystemEnabled) {
            Require(worldAudioSetVoiceVolume(audio_,samples_[2],volume)==WORLD_AUDIO_OK,"Select-car orbit volume submission failed");
            const float pitch=(((radiusOrbit_-10.949999809265137f)*0.21978020668029785f)*-1.553999900817871f+1.784999966621399f)-1.0f;
            Require(worldAudioSetVoicePitchOffset(audio_,samples_[2],pitch)==WORLD_AUDIO_OK,"Select-car orbit pitch submission failed");
        }
    }
    const float radius=radiusStats_+fraction*(radiusOrbit_-radiusStats_);
    const float translation[3]={0.0f,0.0f,radius};motionMatrixTranslate(matrix,matrix,translation);
    if(mode_==0) {modeParam_-=0.029999999329447746f;if(modeParam_<0.0f)modeParam_=0.0f;}
    else if(mode_==1) {modeParam_+=0.029999999329447746f;if(modeParam_>1.0f)modeParam_=1.0f;}
    if(!base_.services->suppressPublication) {
        base_.view->current.fieldOfView=4500;base_.view->current.focusDistance=radius;
    }
    carCinematicViewPublish(base_.services,base_.view,matrix,elapsed);
}

const CarCinematicMethods FrontendStillCamera::methods_={
    &FrontendStillCamera::DisposeOwned,&FrontendStillCamera::AdvanceOwned,nullptr,nullptr};

FrontendStillCamera::FrontendStillCamera(CarCinematicServices& services,std::int32_t viewNumber)
{
    Require(carCinematicBaseInitialize(&base_,&services,viewNumber,12)==CAR_CINEMATIC_OK,
        "Frontend still camera has no supported live view binding");
    carCinematicBindOwner(&base_,this,&methods_);
}

FrontendStillCamera::~FrontendStillCamera()
{
    // Native RAII replaces only the known derived/base deleting transport.
    // The actual base removes this same camera from its live screen stack.
    carCinematicBaseDispose(&base_);
}

void FrontendStillCamera::DisposeOwned(void* camera)
{
    delete static_cast<FrontendStillCamera*>(camera);
}

CarCinematicResult FrontendStillCamera::AdvanceOwned(void* camera)
{
    static_cast<FrontendStillCamera*>(camera)->Update();return CAR_CINEMATIC_OK;
}

void FrontendStillCamera::Update()
{
    float matrix[16];
    carCinematicLookAt(matrix,eye.data(),target.data(),up.data());
    if (!base_.services->suppressPublication) base_.view->current.fieldOfView=fieldOfView;
    carCinematicViewPublish(base_.services,base_.view,matrix,base_.services->clock->realTimeElapsed);
}

} // namespace hp2

// Original CameraMover virtual dispatch adapted to native typed owners.

void carCinematicBindOwner(CarCinematicBase* camera,void* owner,
    const CarCinematicMethods* methods)
{
    camera->ownedObject=owner;
    camera->methods=methods;
}

CarCinematicResult carCinematicDestroyOwned(CarCinematicBase* camera)
{
    if(!camera)return CAR_CINEMATIC_OK;
    if(!camera->ownedObject||!camera->methods||!camera->methods->dispose)
        return CAR_CINEMATIC_INVALID_BINDING;
    void* owner=camera->ownedObject;
    const CarCinematicMethods* methods=camera->methods;
    camera->ownedObject=NULL;
    camera->methods=NULL;
    methods->dispose(owner);
    return CAR_CINEMATIC_OK;
}

CarCinematicResult carCinematicAdvanceOwned(CarCinematicBase* camera)
{
    if(!camera)return CAR_CINEMATIC_OK;
    if(!camera->ownedObject||!camera->methods||!camera->methods->advance)
        return CAR_CINEMATIC_INVALID_BINDING;
    return camera->methods->advance(camera->ownedObject);
}

static void disposeChase(void* owner){carChaseCameraDispose(static_cast<CarChaseCamera*>(owner));}
static CarCinematicResult advanceChase(void* owner){return carChaseCameraAdvance(static_cast<CarChaseCamera*>(owner));}
static void disposeFixed(void* owner){carFixedCameraDispose(static_cast<CarFixedCamera*>(owner));}
static CarCinematicResult advanceFixed(void* owner){return carFixedCameraAdvance(static_cast<CarFixedCamera*>(owner));}
static void disposeDrive(void* owner){carDriveCameraDispose(static_cast<CarDriveCamera*>(owner));}
static CarCinematicResult advanceDrive(void* owner){return carDriveCameraAdvance(static_cast<CarDriveCamera*>(owner));}
static void disposeOrbit(void* owner){carOrbitCameraDispose(static_cast<CarOrbitCamera*>(owner));}
static CarCinematicResult advanceOrbit(void* owner){return carOrbitCameraAdvance(static_cast<CarOrbitCamera*>(owner));}
static void disposeZone(void* owner){carZoneFreezeCameraDispose(static_cast<CarZoneFreezeCamera*>(owner));}
static CarCinematicResult advanceZone(void* owner){return carZoneFreezeCameraAdvance(static_cast<CarZoneFreezeCamera*>(owner));}
static void disposeCrash(void* owner){carCrashCameraDispose(static_cast<CarCrashCamera*>(owner));}
static CarCinematicResult advanceCrash(void* owner){return carCrashCameraAdvance(static_cast<CarCrashCamera*>(owner));}
static void disposeAirborne(void* owner){carAirborneCameraDispose(static_cast<CarAirborneCamera*>(owner));}
static CarCinematicResult advanceAirborne(void* owner){return carAirborneCameraAdvance(static_cast<CarAirborneCamera*>(owner));}
static void disposeVelocity(void* owner){carVelocityCameraDispose(static_cast<CarVelocityCamera*>(owner));}
static CarCinematicResult advanceVelocity(void* owner){return carVelocityCameraAdvance(static_cast<CarVelocityCamera*>(owner));}
static void disposeAttached(void* owner){carAttachedCameraDispose(static_cast<CarAttachedCamera*>(owner));}
static CarCinematicResult advanceAttached(void* owner){return carAttachedCameraAdvance(static_cast<CarAttachedCamera*>(owner));}
static void disposeBumper(void* owner){carBumperCameraDispose(static_cast<CarBumperCamera*>(owner));}
static CarCinematicResult advanceBumper(void* owner){return carBumperCameraAdvance(static_cast<CarBumperCamera*>(owner));}
static void disposePath(void* owner){carPathCameraDispose(static_cast<CarPathCamera*>(owner));}
static CarCinematicResult advancePath(void* owner){return carPathCameraAdvance(static_cast<CarPathCamera*>(owner));}
static void disposeRecorded(void* owner){carRecordedCameraDispose(static_cast<CarRecordedCamera*>(owner));}
static CarCinematicResult advanceRecorded(void* owner){return carRecordedCameraAdvance(static_cast<CarRecordedCamera*>(owner));}


static Car* followedMovement(const CarMovement* movement)
{
    return movement ? movement->bindings.owner : nullptr;
}
static Car* getCarChase(const void* owner)
{return followedMovement(static_cast<const CarChaseCamera*>(owner)->movement);}
static Car* getCarOrbit(const void* owner)
{return static_cast<const CarOrbitCamera*>(owner)->input.selectedCar;}
static Car* getCarAirborne(const void* owner)
{return followedMovement(static_cast<const CarAirborneCamera*>(owner)->movement);}
static Car* getCarVelocity(const void* owner)
{return followedMovement(static_cast<const CarVelocityCamera*>(owner)->context.movement);}
static Car* getCarAttached(const void* owner)
{return followedMovement(static_cast<const CarAttachedCamera*>(owner)->context.movement);}
static Car* getCarBumper(const void* owner)
{return followedMovement(static_cast<const CarBumperCamera*>(owner)->context.movement);}
static Car* getCarPath(const void* owner)
{return followedMovement(static_cast<const CarPathCamera*>(owner)->movement);}
static Car* getCarRecorded(const void* owner)
{return followedMovement(static_cast<const CarRecordedCamera*>(owner)->context.movement);}
static CarCinematicResult renderCrash(void* owner,const CarCinematicScreen* screen,NativeEcstasyFrame* frame)
{return carCrashCameraRender(static_cast<CarCrashCamera*>(owner),screen,frame);}

const CarCinematicMethods carCinematicChaseMethods={disposeChase,advanceChase,nullptr,getCarChase};
const CarCinematicMethods carCinematicFixedMethods={disposeFixed,advanceFixed,nullptr,nullptr};
const CarCinematicMethods carCinematicDriveMethods={disposeDrive,advanceDrive,nullptr,nullptr};
const CarCinematicMethods carCinematicOrbitMethods={disposeOrbit,advanceOrbit,nullptr,getCarOrbit};
const CarCinematicMethods carCinematicZoneMethods={disposeZone,advanceZone,nullptr,nullptr};
const CarCinematicMethods carCinematicCrashMethods={disposeCrash,advanceCrash,renderCrash,nullptr};
const CarCinematicMethods carCinematicAirborneMethods={disposeAirborne,advanceAirborne,nullptr,getCarAirborne};
const CarCinematicMethods carCinematicVelocityMethods={disposeVelocity,advanceVelocity,nullptr,getCarVelocity};
const CarCinematicMethods carCinematicAttachedMethods={disposeAttached,advanceAttached,nullptr,getCarAttached};
const CarCinematicMethods carCinematicBumperMethods={disposeBumper,advanceBumper,nullptr,getCarBumper};
const CarCinematicMethods carCinematicPathMethods={disposePath,advancePath,nullptr,getCarPath};
const CarCinematicMethods carCinematicRecordedMethods={disposeRecorded,advanceRecorded,nullptr,getCarRecorded};
void carAttachedCameraSettingsInitialize(CarAttachedCameraSettings* settings)
{
    /* The final camera initializer writes these three cells at334180. */
    for(int axis=0;axis<3;++axis)settings->translationBias[axis]=0.0f;
}

float carAttachedCameraNativeEyeHeight(float eyeHeight,float groundHeight,bool found)
{
    // PC presentation safeguard for the rear-view overlay (view3) only.
    // Some camera orientations put the inverted eye under local ground;
    // the physical near plane then sees the underside of the track mesh.
    // Ignore invalid terrain and ground far above the car (bridges/tunnels).
    constexpr float minimumEyeClearance=1.25f;
    constexpr float maximumAcceptedGroundAboveEye=3.0f;
    if(!found || !std::isfinite(eyeHeight) || !std::isfinite(groundHeight) ||
        groundHeight-eyeHeight>maximumAcceptedGroundAboveEye)return eyeHeight;
    return std::max(eyeHeight,groundHeight+minimumEyeClearance);
}

CarAttachedCamera* carAttachedCameraCreate(const CarAttachedCameraContext* context,
    CarCinematicResult* result)
{
    if(result)*result=CAR_CINEMATIC_INVALID_BINDING;
    if(!context||!context->services||!context->services->clock||!context->movement||
       !context->movement->bindings.state||!context->world||!context->shake||!context->settings)
        return NULL;
    CarAttachedCamera* camera=static_cast<CarAttachedCamera*>(calloc(1,sizeof(CarAttachedCamera)));
    if(!camera){if(result)*result=CAR_CINEMATIC_ALLOCATION_FAILURE;return NULL;}
    const CarCinematicResult status=carCinematicBaseInitialize(&camera->base,
        context->services,context->viewNumber,6);
    if(status!=CAR_CINEMATIC_OK){free(camera);if(result)*result=status;return NULL;}
    carCinematicBindOwner(&camera->base,camera,&carCinematicAttachedMethods);
    camera->context=*context;
    if(context->viewNumber==3 && context->terrain) {
        terrainSurfaceInitialize(&camera->nativeMirrorTerrain,context->terrain);
        camera->nativeMirrorTerrainActive=true;
    }
    if(!context->services->suppressPublication){
        camera->base.view->hiddenCar=context->movement->bindings.owner;
        camera->base.view->visibilityFlags=0;
    }
    if(result)*result=CAR_CINEMATIC_OK;
    return camera;
}

void carAttachedCameraDispose(CarAttachedCamera* camera)
{
    if(!camera)return;
    if(camera->nativeMirrorTerrainActive)
        terrainSurfaceDispose(&camera->nativeMirrorTerrain);
    carCinematicBaseDispose(&camera->base);
    free(camera);
}

CarCinematicResult carAttachedCameraAdvance(CarAttachedCamera* camera)
{
    if(!camera||!camera->base.view)return CAR_CINEMATIC_INVALID_BINDING;
    const VehicleState* state=camera->context.movement->bindings.state;
    float translation[16],rotation[16],matrix[16];
    rotationMatrixIdentity(translation);
    for(int axis=0;axis<3;++axis)translation[12+axis]=-state->centerPosition[axis];
    eTransposeMatrix(rotation,state->orientation);
    motionMatrixRotateX(rotation,rotation,16384);
    motionMatrixRotateY(rotation,rotation,16384);
    motionMatrixRotateYaw(rotation,rotation,0);
    vectorMatrixMultiply(matrix,rotation,translation);
    for(int axis=0;axis<3;++axis)
        matrix[12+axis]=matrix[12+axis]+camera->context.settings->translationBias[axis];
    if(!camera->base.services->suppressPublication)
        camera->base.view->current.fieldOfView=20000;
    camera->base.view->current.farClip=192.0f;
    camera->base.view->current.nearClip=0.75f;
    carCameraShakeApply(camera->context.shake,camera->context.world,camera->base.viewNumber,matrix);
    if(camera->nativeMirrorTerrainActive) {
        // The released attached camera places its eye at vehicle centre.
        // Read ground through the existing track collision topology, and
        // adjust *only* the final view matrix, after authored shake. This
        // cannot teleport the car or change pursuit/AI collision.
        float inverse[16];carCameraInvertRigidTransform(inverse,matrix);
        bool found=false;
        const float ground=terrainSurfaceElevation(&camera->nativeMirrorTerrain,
            inverse+12,nullptr,nullptr,&found);
        const float safeEye=carAttachedCameraNativeEyeHeight(inverse[14],ground,found);
        if(safeEye!=inverse[14]) {
            inverse[14]=safeEye;
            carCameraInvertRigidTransform(matrix,inverse);
        }
    }
    carCinematicViewPublish(camera->base.services,camera->base.view,matrix,
        camera->base.services->clock->realTimeElapsed);
    return CAR_CINEMATIC_OK;
}

CarCinematicBase* carAttachedCameraBase(CarAttachedCamera* camera)
{
    return camera?&camera->base:NULL;
}

CarCinematicResult carCinematicRenderMovers(const CarCinematicScreen* screen,NativeEcstasyFrame* frame)
{
    if (!screen || !frame)return CAR_CINEMATIC_INVALID_BINDING;
    // PAL15EEE0 reads source intrusive head at eView+3C. The native list
    // adapter stores that newest owner in last (not its oldest first node).
    auto* camera=screen->last;
    if (!camera)return CAR_CINEMATIC_OK;
    if (!camera->ownedObject || !camera->methods)return CAR_CINEMATIC_INVALID_BINDING;
    // PAL CameraMover::Render is empty; only the concrete override submits.
    return camera->methods->render ? camera->methods->render(camera->ownedObject,screen,frame) : CAR_CINEMATIC_OK;
}

Car* carCinematicGetCarToFollow(const CarCinematicBase* camera)
{
    if (!camera)return nullptr;
    hp2::Require(camera->ownedObject && camera->methods,
        "Camera GetCarToFollow has no actual virtual owner");
    // PAL base GetCarToFollow15FCF8 returns null. Concrete overrides below
    // borrow the same Car stored by their source constructors, never Player.
    return camera->methods->getCar ? camera->methods->getCar(camera->ownedObject) : nullptr;
}

// Original MaxCameraMover PAL15A340..15A698; retained typed asset borrows
// replace only PS2 pointers. Euler controls below come from released data.
void carRecordedCameraSettingsInitialize(CarRecordedCameraSettings* settings)
{
    if (!settings) return;
    // Released .data330440..33044B: X=0, Y=0x8000, Z=0x8000,
    // FOV=32768. MaxCamera::Update reads these exact source controls at
    //15A5C4/15A5D4/15A5E4. Zeroing Y/Z reverses authored Busted cameras.
    memset(settings,0,sizeof(*settings));
    settings->rotation[1]=0x8000;settings->rotation[2]=0x8000;
    settings->fieldOfViewOverride=32768.0f;
}

CarRecordedCamera* carRecordedCameraCreate(const CarRecordedCameraContext* context,CarCinematicResult* result)
{
    if (result) *result=CAR_CINEMATIC_INVALID_BINDING;
    if (!context || !context->services || !context->services->clock || !context->services->session ||
        !context->movement || !context->movement->bindings.state || !context->presentation ||
        context->presentation->motion!=context->movement->bindings.ownerMotion || !context->presentation->motion ||
        !context->take || !context->take->poses || !context->take->poseCount ||
        context->take->poseCount>INT32_MAX || !context->settings ||
        context->viewNumber<0 || context->viewNumber>3 || !context->services->screens[context->viewNumber].view) return NULL;
    CarRecordedCamera* camera=(CarRecordedCamera*)calloc(1,sizeof(*camera));
    if (!camera) { if (result) *result=CAR_CINEMATIC_ALLOCATION_FAILURE;return NULL; }
    if (carCinematicBaseInitialize(&camera->base,context->services,context->viewNumber,10)!=CAR_CINEMATIC_OK) {
        free(camera);return NULL;
    }
    carCinematicBindOwner(&camera->base,camera,&carCinematicRecordedMethods);
    camera->context=*context;
    camera->startedWorldTicks=context->startWorldTicks?context->startWorldTicks:context->services->clock->worldTimeFrames;
    if (!context->services->suppressPublication) {
        camera->base.view->hiddenCar=NULL;camera->base.view->visibilityFlags=0;
    }
    camera->base.view->current.nearClip=0.15000000596046448f;
    if (result) *result=CAR_CINEMATIC_OK;
    return camera;
}

void carRecordedCameraDispose(CarRecordedCamera* camera)
{
    if (!camera) return;
    carCinematicBaseDispose(&camera->base);free(camera);
}

CarCinematicResult carRecordedCameraAdvance(CarRecordedCamera* camera)
{
    if (!camera) return CAR_CINEMATIC_INVALID_BINDING;
    const CarRecordedCameraContext* context=&camera->context;
    CarCinematicServices* services=context->services;
    if (services->session->state==6) return CAR_CINEMATIC_OK;
    const VehicleState* live=context->movement->bindings.state;
    const int override=(context->presentation->motion->wheelExclusionMask&(1u<<camera->base.viewNumber))!=0;
    const float* center=override?context->presentation->centerPosition:live->centerPosition;
    const float* frame=override?context->presentation->orientation:live->orientation;
    const float negative[3]={-center[0],-center[1],-center[2]};
    float translation[16],orientation[16],worldToLocal[16];
    motionMatrixTranslation(translation,negative);eTransposeMatrix(orientation,frame);
    vectorMatrixMultiply(worldToLocal,orientation,translation);
    const int32_t elapsed=(int32_t)((uint32_t)services->clock->worldTime-(uint32_t)camera->startedWorldTicks);
    int32_t index=elapsed/2;
    if (index<0) return CAR_CINEMATIC_INVALID_BINDING;
    if ((size_t)index>=context->take->poseCount) {
        camera->startedWorldTicks=services->clock->worldTime;index=0;
    }
    const CarViewRecordedPose* pose=context->take->poses+index;
    const float position[3]={context->timeOffset-pose->position[0],-pose->position[1],-pose->position[2]};
    float recorded[16],rotation[16],matrix[16];
    motionMatrixTranslation(recorded,position);
    vectorMatrixFromQuaternion(rotation,pose->quaternion);
    eTransposeMatrix(rotation,rotation);
    motionMatrixRotateX(rotation,rotation,context->settings->rotation[0]);
    motionMatrixRotateY(rotation,rotation,context->settings->rotation[1]);
    motionMatrixRotateYaw(rotation,rotation,context->settings->rotation[2]);
    vectorMatrixMultiply(recorded,rotation,recorded);
    vectorMatrixMultiply(matrix,recorded,worldToLocal);
    const uint16_t fov=context->settings->fieldOfViewOverride==32768.0f?pose->fieldOfView:
        (uint16_t)convertFloatToInt32(context->settings->fieldOfViewOverride);
    if (!services->suppressPublication) camera->base.view->current.fieldOfView=fov;
    carCinematicViewPublish(services,camera->base.view,matrix,services->clock->realTimeElapsed);
    return CAR_CINEMATIC_OK;
}

CarCinematicBase* carRecordedCameraBase(CarRecordedCamera* camera) { return camera?&camera->base:NULL; }
const CarViewRecordedTake* carRecordedCameraTake(const CarRecordedCamera* camera) { return camera?camera->context.take:NULL; }

/* Camera movement and view management. */
static void updateLayout(CarCinematicServices* services)
{
    const int first=services->screens[1].last!=NULL;
    const int second=services->screens[2].last!=NULL;
    const int third=services->screens[3].last!=NULL;
    services->presentationLayout=first&&second?3:first&&third?2:first?1:0;
}

CarCinematicResult carCinematicBaseInitialize(CarCinematicBase* camera,CarCinematicServices* services,
                                              int32_t viewNumber,int32_t kind)
{
    if(!camera||!services||viewNumber< -1||viewNumber>3)return CAR_CINEMATIC_INVALID_BINDING;
    memset(camera,0,sizeof(*camera));
    camera->services=services;camera->viewNumber=viewNumber;camera->kind=kind;
    if(viewNumber==-1)return CAR_CINEMATIC_OK;
    camera->screen=&services->screens[viewNumber];camera->view=camera->screen->view;
    if(!camera->view)return CAR_CINEMATIC_INVALID_BINDING;
    camera->view->current.farClip=10000.0f;
    camera->savedHiddenCar=camera->view->hiddenCar;
    camera->savedVisibilityFlags=camera->view->visibilityFlags;
    carCinematicBaseActivate(camera);
    return CAR_CINEMATIC_OK;
}

void carCinematicBaseActivate(CarCinematicBase* camera)
{
    if(camera->active)return;
    camera->active=1;
    if(!camera->services->suppressPublication){
        camera->view->hiddenCar=camera->savedHiddenCar;
        camera->view->visibilityFlags=camera->savedVisibilityFlags;
    }
    camera->previous=camera->screen->last;camera->next=NULL;
    if(camera->previous)camera->previous->next=camera;else camera->screen->first=camera;
    camera->screen->last=camera;
    updateLayout(camera->services);
    camera->view->current.nearClip=0.5f;
}

void carCinematicBaseDeactivate(CarCinematicBase* camera)
{
    if(!camera->active)return;
    camera->active=0;
    camera->savedHiddenCar=camera->view->hiddenCar;
    camera->savedVisibilityFlags=camera->view->visibilityFlags;
    if(camera->previous)camera->previous->next=camera->next;else camera->screen->first=camera->next;
    if(camera->next)camera->next->previous=camera->previous;else camera->screen->last=camera->previous;
    camera->previous=camera->next=NULL;
    updateLayout(camera->services);
}

void carCinematicBaseDispose(CarCinematicBase* camera)
{
    carCinematicBaseDeactivate(camera);
}

void carCameraUp(float output[4], const float eye[4], const float target[4], uint16_t roll)
{
    float forward[4], rotation[16];
    for (int axis = 0; axis < 4; ++axis) forward[axis] = target[axis] - eye[axis];
    vectorSetLength3(forward, forward, 1.0f);
    motionMatrixAxisAngle(rotation, forward, roll);
    const float up[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    vectorMatrixTransformPoint(output, rotation, up);
}

void carCameraTerrainOffset(const CarChaseCamera* camera, TerrainSurface* surface,
    const float first[4], const float second[4], float offset[4])
{
    carCameraTerrainOffsetWithClearance(surface,
        camera->context.settings->ignoredTerrainMaterials,
        carChaseCameraTerrainClearance(camera),first,second,offset);
}

void carCameraTerrainOffsetWithClearance(TerrainSurface* surface,
    const int8_t ignoredMaterials[33],float cameraClearance,
    const float first[4],const float second[4],float offset[4])
{
    float point[4], correction[4];
    memcpy(point, second, sizeof(point));
    terrainSurfaceConstrainSegment(surface, 1, true, first, point, 0.009999999776482582f, 1.0f);
    for (int axis = 0; axis < 4; ++axis) correction[axis] = point[axis] - second[axis];
    int32_t iteration = 0;
    for (;;) {
        for (int axis = 0; axis < 4; ++axis) point[axis] = second[axis] + correction[axis];
        float sum[4] = {0,0,0,0};
        int32_t contacts = 0;
        for (size_t index = 0; index < surface->polygonCount; ++index) {
            const TerrainPolygon* polygon = surface->polygons[index];
            if (ignoredMaterials[polygon->materialIndex]) continue;
            float distance;
            if (!terrainPolygonContains(polygon, point, &distance)) continue;
            const float clearance = polygon->flags & 2u ?
                cameraClearance : 0.699999988079071f;
            if (0.0f <= distance && distance < clearance) {
                const float penetration = clearance - distance;
                for (int axis = 0; axis < 4; ++axis)
                    sum[axis] = polygon->plane[axis] * penetration + sum[axis];
                ++contacts;
            }
        }
        if (!contacts) break;
        const float inverse = finiteRatio(1.0f, (float)contacts);
        for (int axis = 0; axis < 4; ++axis) correction[axis] = sum[axis] * inverse + correction[axis];
        if (++iteration >= 3) break;
    }
    memcpy(offset, correction, sizeof(correction));
    offset[3] = 0.0f;
}

void carCameraLookAtWithSlope(float output[16], const float eye[4],
    const float target[4], const float slope[2])
{
    const float up[4] = {0,0,1,0};
    float matrix[16], inverse[16], local[4], adjusted[4];
    carCinematicLookAt(matrix, eye, target, up);
    carCameraInvertRigidTransform(inverse, matrix);
    vectorMatrixTransformPoint(local, matrix, target);
    const float y = local[2] * slope[1];
    const float x = local[2] * slope[0];
    local[1] -= y;
    local[0] -= x;
    vectorMatrixTransformPoint(adjusted, inverse, local);
    carCinematicLookAt(output, eye, adjusted, up);
}

void carChaseCameraSettingsInitialize(CarChaseCameraSettings* settings)

{

    static const int8_t ignoredMaterials[33] = {

        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1

    };

    memset(settings, 0, sizeof(*settings));

    settings->useAttachedCar = 1;

    settings->selectedDriverNumber = -1;

    settings->overrideOffset[0] = 7.300000190734863f;

    settings->overrideOffset[1] = 2.299999952316284f;

    settings->overridePitch[1] = 2000;

    settings->rollPositiveScale = 0.6000000238418579f;

    settings->rollNegativeScale = 0.04500000178813934f;

    settings->positiveRollThreshold = 0.699999988079071f;

    settings->negativeRollThreshold = -0.30000001192092896f;

    settings->movementReduction = 6.0f;

    settings->distanceResponseScale = 1.0f;

    settings->maximumResponseSpeed = 500.0f;

    settings->negativeDistanceScale = 0.20000000298023224f;

    settings->maximumRollResponse = 6.0f;

    settings->minimumRollResponse = -1.0f;

    settings->dynamicRollScale = 0.4000000059604645f;

    settings->recenterFastBoundary = 3.0f;

    settings->responseAcceleration = 0.029999999329447746f;

    settings->responseDecay = 0.012000000104308128f;

    settings->responseTimeout = 2.0f;

    settings->terrainPitchNoise = 250.0f;

    settings->responseScale = 1.0f;

    memcpy(settings->ignoredTerrainMaterials, ignoredMaterials, sizeof(ignoredMaterials));

}

void carChaseCameraReset(CarChaseCamera* camera)

{

    const VehicleState* state = camera->movement->bindings.state;

    camera->yawOffset = 0;

    camera->facingTarget = 0.0f;

    camera->facingWeight = 0.0f;

    camera->targetYaw = 0;

    camera->targetDistance = 0.0f;

    camera->previousHeading = state->heading;

    camera->smoothedYaw = 0;

    camera->previousPitch = vectorArcSineAngle(state->orientation[2]);

    camera->smoothedPitch = 0;

    memset(camera->rollHistory, 0, sizeof(camera->rollHistory));

    camera->rollResponse = 0.0f;

    camera->desiredRollResponse = 0.0f;

    camera->rollIdleTime = 0.0f;

    camera->substepRemainder = 0.0f;

    camera->responseScale = 1.0f;

    camera->previousForwardSpeed = state->forwardSpeed;

    camera->smoothingWeight = 1.0f;

}

CarChaseCamera* carChaseCameraCreate(const CarChaseCameraContext* context, CarCinematicResult* result)

{

    if (result) *result = CAR_CINEMATIC_INVALID_BINDING;

    if (!context || !context->services || !context->services->clock || !context->terrain ||

        !context->movement || !context->movement->bindings.state || !context->vehicles ||

        !context->settings || !context->simulation || !context->shake ||

        context->viewNumber < 0 || context->viewNumber > 3 ||

        !context->services->screens[context->viewNumber].view) return NULL;

    const CarCameraCatalogue* catalogue = NULL;

    for (size_t index = 0; index < context->vehicleCount; ++index)

        if (context->vehicles[index].movement == context->movement) catalogue = context->vehicles[index].catalogue;

    if (!catalogue) return NULL;

    if (!catalogue->viewCount[0] || !catalogue->viewCount[1]) catalogue = context->defaultCatalogue;

    if (!catalogue || !catalogue->viewCount[0] || !catalogue->viewCount[1]) return NULL;

    CarChaseCamera* camera = (CarChaseCamera*)calloc(1, sizeof(*camera));

    if (!camera) { if (result) *result = CAR_CINEMATIC_ALLOCATION_FAILURE; return NULL; }

    if (carCinematicBaseInitialize(&camera->base, context->services, context->viewNumber, 1) != CAR_CINEMATIC_OK) {

        free(camera); return NULL;

    }
    carCinematicBindOwner(&camera->base,camera,&carCinematicChaseMethods);

    camera->context = *context;

    camera->movement = context->movement;

    camera->catalogue = catalogue;

    camera->base.player = context->player;

    carChaseCameraReset(camera);

    terrainSurfaceInitialize(&camera->surface, context->terrain);

    camera->facingRate = context->facingRate;

    camera->facingTarget = context->initiallyFacingBack ? 1.0f : 0.0f;

    if (!context->services->suppressPublication) {

        camera->base.view->hiddenCar = NULL;

        camera->base.view->visibilityFlags = 0;

    }

    carChaseCameraSetViewIndex(camera, context->viewIndex);

    if (result) *result = CAR_CINEMATIC_OK;

    return camera;

}

void carChaseCameraDispose(CarChaseCamera* camera)

{

    if (!camera) return;

    if (!camera->base.services->suppressPublication) {

        camera->base.view->hiddenCar = NULL;

        camera->base.view->current.minimumVisibility = 0.0f;

        camera->base.view->current.maximumVisibility = 0.0f;

    }

    terrainSurfaceDispose(&camera->surface);

    carCinematicBaseDispose(&camera->base);

    free(camera);

}

void carChaseCameraSetFacingTarget(CarChaseCamera* camera, int32_t facingBack)

{

    camera->facingTarget = facingBack ? 1.0f : 0.0f;

}

CarCinematicBase* carChaseCameraBase(CarChaseCamera* camera) { return camera ? &camera->base : NULL; }

const CarChaseCameraVehicle* carChaseCameraVehicleForCar(const CarChaseCamera* camera,

    const Car* car)

{

    for (size_t index = 0; index < camera->context.vehicleCount; ++index) {

        const CarChaseCameraVehicle* vehicle = &camera->context.vehicles[index];

        if (vehicle->movement && vehicle->movement->bindings.owner == car) return vehicle;

    }

    return NULL;

}

float carChaseCameraAngularRate(uint16_t difference)
{
    const int32_t signedDifference = difference >= 32768 ? (int32_t)difference - 65536 : difference;
    const int32_t magnitude = signedDifference < 0 ? -signedDifference : signedDifference;
    const float coordinate = (float)magnitude * 0.0002610000083222985f;
    const uint16_t angle = vectorAtan2Angle(1.0f, coordinate);
    const float rate = (float)angle * -1.9088000059127808f;
    return signedDifference < 0 ? -rate : rate;
}

int32_t carChaseCameraHasRearFacing(const CarChaseCamera* camera)
{
    return camera->base.player && camera->base.player->settings.lookback == 0 && camera->facingWeight > 0.0f;
}

int32_t carChaseCameraUsesForwardSide(const CarChaseCamera* camera)
{
    const CarCameraView* view = carCameraViewAt(camera->catalogue,
        camera->base.services->presentationLayout, camera->viewIndex);
    if (!view || view->kind == 2) return 1;
    return camera->base.player && camera->base.player->settings.lookback == 0 && camera->facingWeight > 0.0f;
}

float carChaseCameraTerrainClearance(const CarChaseCamera* camera)
{
    float weight = 0.0f;
    if (camera->base.player && camera->base.player->settings.lookback == 0) weight = camera->facingWeight;
    const float forward = (1.0f - weight) * 0.699999988079071f;
    return forward + weight * 3.0f;
}

static float chaseLengthThree(const float value[4])
{
    const float x = value[0] * value[0], y = value[1] * value[1], z = value[2] * value[2];
    return sqrtf((x + y) + z);
}

static float chaseApproach(float value, float desired, float change)
{
    if (value < desired) { value += change; if (desired < value) value = desired; }
    else if (desired < value) { value -= change; if (value < desired) value = desired; }
    return value;
}

static void chaseChooseRollResponse(CarChaseCamera* camera, const VehicleState* state,
    float input, int32_t direction)
{
    const CarChaseCameraSettings* settings = camera->context.settings;
    float sum = 0.0f;
    int32_t accepted = 0;
    if (direction > 0 && !(0.0f < camera->rollResponse)) {
        accepted = 1; sum = input;
        const float threshold = settings->positiveRollThreshold * settings->responseScale;
        for (int index = 1; index <= 9; ++index) {
            if (camera->rollHistory[index] < threshold) accepted = 0;
            sum += camera->rollHistory[index];
        }
        if (accepted)
            for (int index = 10; index < 20; ++index)
                if (threshold < camera->rollHistory[index]) accepted = 0;
    } else if (direction < 0 && !(camera->rollResponse < 0.0f)) {
        accepted = -1; sum = input;
        const float threshold = settings->negativeRollThreshold * settings->responseScale;
        for (int index = 1; index <= 9; ++index) {
            if (threshold < camera->rollHistory[index]) accepted = 0;
            sum += camera->rollHistory[index];
        }
        if (accepted)
            for (int index = 10; index < 20; ++index)
                if (camera->rollHistory[index] < threshold) accepted = 0;
    } else if (!direction) {
        const float high = settings->positiveRollThreshold * settings->responseScale;
        const float low = settings->negativeRollThreshold * settings->responseScale;
        for (int index = 1; index <= 9; ++index)
            if (camera->rollHistory[index] < high && low < camera->rollHistory[index])
                camera->desiredRollResponse = 0.0f;
    }
    if (!accepted) return;
    camera->desiredRollResponse = sum * 0.10000000149011612f;
    float headingDifference = vehicleStateMovingHeadingDifference(state);
    if (headingDifference < 0.0f) headingDifference = -headingDifference;
    float weight = 1.0f - headingDifference * settings->movementReduction;
    if (weight < 0.0f) weight = 0.0f;
    camera->desiredRollResponse *= weight;
    const float maximum = settings->maximumRollResponse * settings->responseScale;
    const float minimum = settings->minimumRollResponse * settings->responseScale;
    if (maximum < camera->desiredRollResponse) camera->desiredRollResponse = maximum;
    if (camera->desiredRollResponse < minimum) camera->desiredRollResponse = minimum;
    camera->rollIdleTime = 0.0f;
    if (camera->context.dynamicResponse && 5.0f < state->forwardSpeed) {
        if (state->clutchEngaged == 1) camera->desiredRollResponse *= 0.75f;
        camera->responseScale = settings->dynamicRollScale;
        camera->desiredRollResponse *= settings->dynamicRollScale;
    } else camera->responseScale = 1.0f;
}

static float chaseUpdateRollResponse(CarChaseCamera* camera, const VehicleState* state,
    float roll, float elapsed)
{
    CarChaseCameraSettings* settings = camera->context.settings;
    int32_t direction = 0;
    if ((roll * settings->rollNegativeScale) * settings->responseScale <
        settings->negativeRollThreshold * settings->responseScale) {
        direction = -1;
        roll *= settings->rollNegativeScale * settings->responseScale;
    } else if (settings->positiveRollThreshold * settings->responseScale <
        (roll * settings->rollPositiveScale) * settings->responseScale) {
        direction = 1;
        roll *= settings->rollPositiveScale * settings->responseScale;
    } else roll = 0.0f;
    const float steps = elapsed * 60.0f + camera->substepRemainder;
    const int32_t wholeSteps = convertFloatToInt32(steps);
    camera->substepRemainder = steps - (float)wholeSteps;
    for (int32_t step = 0; step < wholeSteps; ++step) {
        for (int index = 19; index > 0; --index) camera->rollHistory[index] = camera->rollHistory[index - 1];
        camera->rollHistory[0] = roll;
        chaseChooseRollResponse(camera, state, roll, direction);
    }
    const float delta = elapsed * 60.0f;
    const float desired = camera->desiredRollResponse;
    float value = camera->rollResponse;
    if (desired == 0.0f) {
        const float change = (settings->responseDecay * delta) * camera->responseScale;
        if (settings->recenterFastBoundary < value) value -= change + change;
        else if (0.0f < value) value = change < value ? value - change : 0.0f;
        else if (value < 0.0f) value = value < -change ? value + change : 0.0f;
    } else if (0.0f < desired) {
        const float difference = value - desired;
        if (0.0f < difference) {
            camera->rollIdleTime += elapsed;
            if (settings->responseTimeout < camera->rollIdleTime || !direction)
                camera->desiredRollResponse = 0.0f;
        } else {
            float change = (settings->responseAcceleration * camera->responseScale) * delta;
            if (-0.4000000059604645f < difference) change *= 0.30000001192092896f;
            else if (-1.0f < difference) change *= 0.6000000238418579f;
            value += change;
        }
    } else {
        if (state->forwardSpeed * 2.236999988555908f < 5.0f) camera->desiredRollResponse = 0.0f;
        else if (value < desired) {
            camera->rollIdleTime += elapsed;
            if (settings->responseTimeout < camera->rollIdleTime || !direction)
                camera->desiredRollResponse = 0.0f;
        } else {
            float change = -settings->responseAcceleration * camera->responseScale;
            change *= delta;
            value += change;
        }
    }
    camera->rollResponse = value;
    float speedFactor = settings->maximumResponseSpeed - state->forwardSpeed * 2.236999988555908f;
    if (speedFactor < 0.0f) speedFactor = 0.0f;
    speedFactor = finiteRatio(speedFactor, settings->maximumResponseSpeed);
    return camera->context.dynamicResponse ? 0.0f :
        (camera->rollResponse * settings->distanceResponseScale) * speedFactor;
}

static void chaseUpdateTargetFacing(CarChaseCamera* camera, const CarChaseCameraVehicle* vehicle,
    const CarMovement* target, uint16_t heading, float viewBackDistance, float rate, float elapsed)
{
    const VehicleState* state = vehicle->movement->bindings.state;
    const float* point = target->bindings.state->position;
    const float x = point[0] - state->position[0], y = point[1] - state->position[1];
    const float distance = sqrtf(x * x + y * y);
    int32_t angle = (uint16_t)(heading - vectorAtan2Angle(x, y));
    const float factor = camera->base.services->presentationLayout == 3 ? 0.0f : 0.4000000059604645f;
    float clearance;
    if ((uint32_t)(angle - 8192) > 49151u) {
        angle = 0;
        clearance = *vehicle->targetClearance - viewBackDistance + factor * 0.75f;
    } else clearance = *vehicle->targetClearance - viewBackDistance + factor;
    int32_t current = camera->targetYaw;
    if (current < angle - 32768) angle -= 65536;
    if (angle + 32768 < current) angle += 65536;
    if (current < angle) {
        const float speed = current + 10000 < angle || (50.0f < distance && (uint16_t)angle != 0) ? 60000.0f : 20000.0f;
        current += convertFloatToInt32(rate * (elapsed * speed));
        if (angle < current) current = angle;
    } else if (angle < current) {
        const float speed = angle < current - 10000 || (50.0f < distance && (uint16_t)angle != 0) ? 60000.0f : 20000.0f;
        current -= convertFloatToInt32(rate * (elapsed * speed));
        if (current < angle) current = angle;
    }
    camera->targetYaw = (uint16_t)current;
    camera->targetDistance = chaseApproach(camera->targetDistance, clearance, rate * elapsed);
    camera->yawOffset = (uint16_t)camera->targetYaw;
}

CarCinematicResult carChaseCameraAdvance(CarChaseCamera* camera)
{
    if (!camera || !camera->context.simulation || !camera->context.simulation->world)
        return CAR_CINEMATIC_INVALID_BINDING;
    const WorldSimulation* simulation = camera->context.simulation;
    CarChaseCameraSettings* settings = camera->context.settings;
    const CarChaseCameraVehicle* vehicle = NULL;
    if (settings->useAttachedCar) {
        vehicle = carChaseCameraVehicleForCar(camera, camera->movement->bindings.owner);
    } else {
        Car* selected = worldGetCarByDriverNumber(simulation->world, settings->selectedDriverNumber);
        if (!selected && settings->selectedDriverNumber >= 0) {
            do {
                --settings->selectedDriverNumber;
                /* The debugger's selection-change packet is retired. */
                selected = worldGetCarByDriverNumber(simulation->world, settings->selectedDriverNumber);
            } while (!selected && settings->selectedDriverNumber >= 0);
        }
        vehicle = carChaseCameraVehicleForCar(camera, selected ? selected : camera->movement->bindings.owner);
    }
    if (!vehicle || !vehicle->targetClearance || !vehicle->movement->bindings.state ||
        !vehicle->movement->bindings.state->position) return CAR_CINEMATIC_INVALID_BINDING;
    CarMovement* movement = vehicle->movement;
    VehicleState* state = movement->bindings.state;
    CarCinematicServices* services = camera->base.services;
    const float elapsed = services->clock->worldTimeElapsed;
    const int32_t playerFacing = camera->base.player && camera->base.player->settings.lookback == 0;
    float facingRate = camera->facingRate;
    CarMovement* related = NULL;
    if (simulation->pursuit && camera->facingTarget == 1.0f && !playerFacing) {
        int32_t isPursuer;
        related = carChaseCameraRelatedVehicle(simulation->pursuit, movement, &isPursuer);
        if (isPursuer) facingRate = 1.3333333730697632f;
    } else camera->targetYaw = 0, camera->targetDistance = 0.0f;
    camera->facingWeight = chaseApproach(camera->facingWeight, camera->facingTarget, facingRate * elapsed);
    const CarCameraView* view = carCameraViewAt(camera->catalogue, services->presentationLayout, camera->viewIndex);
    if (!view) {
        carChaseCameraSetViewIndex(camera, 0);
        view = carCameraViewAt(camera->catalogue, services->presentationLayout, camera->viewIndex);
    }
    if (!view) return CAR_CINEMATIC_INVALID_BINDING;
    settings->responseScale = 1.0f;
    float transpose[16], localAngular[4];
    eTransposeMatrix(transpose, state->orientation);
    vectorMatrixTransformPoint(localAngular, transpose, state->angularVelocity);
    const float response = chaseUpdateRollResponse(camera, state, localAngular[0], elapsed);
    const uint16_t facingAngle = (uint16_t)convertFloatToInt32(camera->facingWeight * 32768.0f);
    const float facing = 1.0f - (vectorCosineAngle(facingAngle) + 1.0f) * 0.5f;
    float offsetX, offsetY, offsetZ;
    uint16_t pitch[2];
    if (settings->overrideView) {
        offsetX = settings->overrideOffset[0];
        if (view->kind == 2) offsetX += response;
        offsetY = settings->overrideOffset[1]; offsetZ = settings->overrideOffset[2];
        pitch[0] = settings->overridePitch[0]; pitch[1] = settings->overridePitch[1];
    } else {
        offsetX = view->offset[0];
        if (view->kind == 2) offsetX += response * (1.0f - facing);
        offsetY = view->offset[1]; offsetZ = view->offset[2];
        pitch[0] = view->parameters[0]; pitch[1] = view->parameters[1];
    }
    if (view->kind == 2 && response < 0.0f) offsetY += response * settings->negativeDistanceScale;
    float initial[16], orientation[16], local[16], output[16];
    camera->yawOffset = (uint16_t)convertFloatToInt32(facing * 32768.0f);
    rotationMatrixIdentity(initial);
    for (int axis = 0; axis < 3; ++axis) initial[12 + axis] = -state->centerPosition[axis];
    memcpy(orientation, state->orientation, sizeof(orientation));
    uint16_t heading = 0;
    if (view->kind != 2) {
        eTransposeMatrix(local, orientation);
        if (related) camera->yawOffset = carChaseCameraTargetQuadrant(movement, related, orientation);
        if (!playerFacing) motionMatrixRotateYaw(local, local, camera->yawOffset);
        vectorMatrixMultiply(orientation, local, initial);
    } else {
        float velocity[2] = {state->velocity[0], state->velocity[1]};
        const float dot = velocity[0] * orientation[0] + velocity[1] * orientation[1];
        if (dot < 0.0f) {
            const float scale = dot * -2.0f;
            velocity[1] += orientation[1] * scale;
            velocity[0] += orientation[0] * scale;
        }
        float weight = (sqrtf(velocity[0] * velocity[0] + velocity[1] * velocity[1]) -
            8.319999694824219f) * 0.2127659171819687f;
        if (weight < 0.0f) weight = 0.0f;
        if (1.0f < weight) weight = 1.0f;
        vectorSetLength2(velocity, velocity, weight);
        const float remainder = 1.0f - weight;
        const float x = orientation[0] * remainder, y = orientation[1] * remainder;
        velocity[1] = y + velocity[1]; velocity[0] = x + velocity[0];
        heading = vectorAtan2Angle(velocity[0], velocity[1]);
        motionMatrixRotateYaw(orientation, initial, (uint16_t)-heading);
        if (related && !playerFacing) chaseUpdateTargetFacing(camera, vehicle, related, heading, offsetY, facingRate, elapsed);
    }
    rotationMatrixIdentity(local);
    const int32_t oriented = view->kind == 2 || (playerFacing && camera->facingWeight > 0.0f);
    if (view->kind == 2) {
        const float contactDirection = state->activeContactCount < 3 ? -1.0f : 1.0f;
        camera->smoothingWeight += contactDirection * 0.24315999448299408f;
        if (1.0f < camera->smoothingWeight) camera->smoothingWeight = 1.0f;
        else if (camera->smoothingWeight < 0.0f) camera->smoothingWeight = 0.0f;
        const uint16_t difference = (uint16_t)(heading - camera->previousHeading + camera->smoothedYaw);
        const float rate = carChaseCameraAngularRate(difference);
        const int32_t step = convertFloatToInt32((camera->smoothingWeight * rate) * elapsed);
        camera->smoothedYaw = (uint16_t)(difference + step);
        motionMatrixRotateYaw(local, local, camera->smoothedYaw);
        camera->previousHeading = heading;
        const uint16_t currentPitch = vectorArcSineAngle(state->orientation[2]);
        int32_t wantedPitch = currentPitch;
        if (!camera->yawOffset) {
            if (camera->smoothingWeight == 0.0f) wantedPitch = currentPitch <= 32767 ?
                currentPitch - currentPitch / 9 : currentPitch + (65535 - currentPitch) / 3;
            else wantedPitch = currentPitch <= 32767 ? currentPitch + currentPitch / 9 :
                currentPitch - (65535 - currentPitch) / 3;
        }
        const uint16_t pitchDifference = (uint16_t)(camera->smoothedPitch - wantedPitch);
        int32_t pitchStep = 0;
        if (contactDirection != 0.0f)
            pitchStep = convertFloatToInt32(((float)carChaseCameraPitchStep(pitchDifference) * elapsed) * 25.0f);
        const uint16_t smoothPitch = (uint16_t)(camera->smoothedPitch + pitchStep);
        size_t materialCount;
        const TerrainMaterial* materials = terrainDefaultMaterials(&materialCount);
        if (materialCount <= 2) return CAR_CINEMATIC_INVALID_BINDING;
        const float noise = terrainMaterialDisplacement(&materials[2], state->position);
        const uint16_t noisyPitch = (uint16_t)convertFloatToInt32((float)smoothPitch + settings->terrainPitchNoise * noise);
        motionMatrixRotateY(local, local, noisyPitch);
        if (!playerFacing) motionMatrixRotateYaw(local, local, camera->yawOffset);
        camera->previousPitch = currentPitch;
        camera->smoothedPitch = smoothPitch;
    } else {
        camera->smoothedYaw = 0; camera->previousHeading = state->heading;
        camera->previousPitch = vectorArcSineAngle(state->orientation[2]); camera->smoothedPitch = 0;
    }
    float adjustedX = offsetX;
    if (playerFacing && camera->facingWeight > 0.0f) {
        const float remaining = 1.0f - camera->facingWeight;
        const float weight = camera->facingWeight;
        const int32_t split = services->presentationLayout == 3;
        offsetY = split ? remaining * offsetY - weight : remaining * offsetY + weight * -3.0f;
        adjustedX = remaining * offsetX + weight * (split ? 16.0f : 18.0f);
        float wanted = (float)pitch[0] * remaining;
        wanted += weight * (split ? 3540.0f : 5200.0f);
        motionMatrixRotateY(local, local, (uint16_t)-convertFloatToInt32(wanted));
    }
    float slope[2] = {0,0}, correctedEye[4] = {0,0,0,0}, correction[4] = {0,0,0,0};
    float verticalBlend = 0.0f;
    if (oriented) {
        if (related && !playerFacing) { offsetY += camera->targetDistance; adjustedX = 2.240000009536743f; }
        const float translation[3] = {adjustedX, offsetZ, -offsetY};
        float translated[16], inverse[16], middle[4], carLocal[4];
        motionMatrixTranslate(translated, local, translation);
        vectorMatrixMultiply(translated, translated, orientation);
        carCameraInvertRigidTransform(inverse, translated);
        float eye[4]; memcpy(eye, inverse + 12, sizeof(eye));
        float difference[4];
        for (int axis = 0; axis < 4; ++axis) {
            middle[axis] = (eye[axis] + state->position[axis]) * 0.5f;
            difference[axis] = state->position[axis] - eye[axis];
        }
        const float radius = chaseLengthThree(difference) * 0.5f + 1.0f;
        terrainSurfaceUpdate(&camera->surface, middle, radius, state->velocity);
        vectorMatrixTransformPoint(carLocal, translated, state->position);
        float inverseX = finiteRatio(1.0f, carLocal[0]);
        if (inverseX < -1000.0f) inverseX = -1000.0f;
        if (1000.0f < inverseX) inverseX = 1000.0f;
        const float negativeInverse = -inverseX;
        slope[1] = negativeInverse * carLocal[2]; slope[0] = negativeInverse * carLocal[1];
        carCameraTerrainOffset(camera, &camera->surface, state->position, eye, correction);
        for (int axis = 0; axis < 4; ++axis) correctedEye[axis] = eye[axis] + correction[axis];
        if (!related) {
            for (int axis = 0; axis < 4; ++axis) difference[axis] = state->position[axis] - correctedEye[axis];
            const float distance = chaseLengthThree(difference);
            DriverPhysicsVehicle physics{}; physics.movement = movement;
            float minimum[4], maximum[4]; driverCollisionBounds(&physics, minimum, maximum);
            const float minimumX = fabsf(minimum[0]);
            const float limit = maximumFinite(maximum[0], minimumX) + 0.699999988079071f;
            if (distance < limit) {
                const float xx = difference[0] * difference[0], yy = difference[1] * difference[1];
                const float height = sqrtf(fabsf(limit * limit - (xx + yy)));
                const float gap = correctedEye[2] - state->position[2];
                const float lift = height - gap;
                verticalBlend = finiteRatio(gap, height);
                correctedEye[2] += lift; correction[2] += lift;
            }
        }
        float localCorrection[4]; vectorMatrixTransform(localCorrection, translated, correction);
        offsetY += localCorrection[2]; offsetZ -= localCorrection[1]; adjustedX -= localCorrection[0];
    }
    const float translation[3] = {adjustedX, offsetZ, -offsetY};
    motionMatrixTranslate(local, local, translation);
    const float pitchChange = verticalBlend * (float)((int32_t)pitch[1] - pitch[0]);
    const uint16_t pitchAngle = (uint16_t)-(pitch[0] + convertFloatToInt32(pitchChange));
    motionMatrixRotateY(local, local, pitchAngle);
    if (oriented) carCameraLookAtWithSlope(output, correctedEye, state->position, slope);
    else {
        float inverse[16]; vectorMatrixMultiply(output, local, orientation);
        carCameraInvertRigidTransform(inverse, output);
        const float ground = terrainSurfaceElevation(&camera->surface, inverse + 12, NULL, NULL, NULL) + 0.699999988079071f;
        if (inverse[14] < ground) {
            inverse[14] = ground; carCameraInvertRigidTransform(output, inverse);
        }
        motionMatrixRotateX(output, output, 16384);
        motionMatrixRotateY(output, output, 49152);
    }
    CarCinematicView* liveView = camera->base.view;
    if (!services->suppressPublication) {
        liveView->visibilityFlags = 0;
        if (!oriented) {
            liveView->hiddenCar = movement->bindings.owner;
            if (view->kind == 1 && camera->facingTarget != 1.0f) liveView->visibilityFlags = 1;
        } else liveView->hiddenCar = NULL;
        if (view->kind == 0 || view->kind == 1) liveView->current.fieldOfView = 16384;
        else if (view->kind == 2) liveView->current.fieldOfView = 13020;
    }
    liveView->current.nearClip = view->kind == 1 ? 0.4000000059604645f : 0.5f;
    carCameraShakeApply(camera->context.shake, simulation->world, camera->base.viewNumber, output);
    float focus[4];
    for (int axis = 0; axis < 4; ++axis) focus[axis] = liveView->current.eye[axis] - state->position[axis];
    const float distance = chaseLengthThree(focus);
    if (!services->suppressPublication) liveView->current.focusDistance = distance;
    if (!camera->context.extendedVisibility && !services->suppressPublication) {
        liveView->current.minimumVisibility = 15.0f; liveView->current.maximumVisibility = 65.0f;
    }
    carCinematicViewPublish(services, liveView, output, services->clock->realTimeElapsed);
    if (playerFacing) camera->yawOffset = 0;
    return CAR_CINEMATIC_OK;
}

static float bumperDistanceThree(const float first[4],const float second[4])
{
    const float x=first[0]-second[0],y=first[1]-second[1],z=first[2]-second[2];
    return sqrtf((x*x+y*y)+z*z);
}
static float bumperAdvanceParameter(float current,float rate,uint32_t useRoute,
    int32_t count,float fraction,float elapsed)
{
    float result=useRoute?(float)(count-1)*fraction:current+(rate*elapsed)*25.0f;
    if (result>(float)(count-1)) result=(float)(count-1);
    return result;
}

CarBumperCamera* carBumperCameraCreate(const CarBumperCameraContext* context,CarCinematicResult* result)
{
    if (result) *result=CAR_CINEMATIC_INVALID_BINDING;
    if (!context || !context->services || !context->services->clock || !context->services->session ||
        !context->movement || !context->movement->bindings.state || !context->race || !context->raceFrame ||
        !context->terrain || !context->ignoredTerrainMaterials || !context->items ||
        !context->items->eyeKeys || !context->items->rollKeys || !context->items->targetKeys ||
        !context->items->projection.lensKeys || context->items->eyeCount<1 || context->items->targetCount<1 ||
        context->viewNumber<0 || context->viewNumber>3 || !context->services->screens[context->viewNumber].view) return NULL;
    CarBumperCamera* camera=(CarBumperCamera*)calloc(1,sizeof(*camera));
    if (!camera) { if (result) *result=CAR_CINEMATIC_ALLOCATION_FAILURE;return NULL; }
    if (carCinematicBaseInitialize(&camera->base,context->services,context->viewNumber,5)!=CAR_CINEMATIC_OK) {
        free(camera);return NULL;
    }
    carCinematicBindOwner(&camera->base,camera,&carCinematicBumperMethods);
    camera->context=*context;camera->items=*context->items;camera->base.player=context->player;
    splineInitializeBorrowed(&camera->eye,SPLINE_CLAMP,camera->items.eyeCount,(float(*)[4])camera->items.eyeKeys,16384);
    splineInitializeBorrowed(&camera->roll,SPLINE_CLAMP,camera->items.eyeCount,(float(*)[4])camera->items.rollKeys,16384);
    splineInitializeBorrowed(&camera->target,SPLINE_CLAMP,camera->items.targetCount,(float(*)[4])camera->items.targetKeys,16384);
    splineInitializeBorrowed(&camera->lens,SPLINE_CLAMP,32,(float(*)[4])camera->items.projection.lensKeys,16384);
    terrainSurfaceInitialize(&camera->surface,context->terrain);
    if (!context->services->suppressPublication) {
        camera->base.view->hiddenCar=NULL;camera->base.view->visibilityFlags=0;
        camera->base.view->current.fieldOfView=13020;
    }
    camera->base.view->current.nearClip=camera->items.nearClip;
    camera->base.view->current.farClip=camera->items.farClip;
    if (result) *result=CAR_CINEMATIC_OK;
    return camera;
}

void carBumperCameraDispose(CarBumperCamera* camera)
{
    if (!camera) return;
    if (!camera->base.services->suppressPublication) {
        camera->base.view->current.minimumVisibility=0.0f;
        camera->base.view->current.maximumVisibility=0.0f;
    }
    terrainSurfaceDispose(&camera->surface);
    splineDestroy(&camera->lens,0);splineDestroy(&camera->target,0);
    splineDestroy(&camera->roll,0);splineDestroy(&camera->eye,0);
    carCinematicBaseDispose(&camera->base);free(camera);
}

const float* carBumperCameraFocusPosition(const CarBumperCamera* camera)
{
    return camera->context.movement?camera->context.movement->bindings.state->centerPosition:
        camera->base.view->current.target;
}

void carBumperCameraConstrain(CarBumperCamera* camera,float matrix[16])
{
    const VehicleState* vehicle=camera->context.movement->bindings.state;
    float inverse[16],eye[4],local[4],offset[4],difference[4];
    carCameraInvertRigidTransform(inverse,matrix);
    memcpy(eye,inverse+12,sizeof(eye));
    TerrainSurface surface;terrainSurfaceInitialize(&surface,camera->context.terrain);
    float midpoint[4];
    for (int axis=0;axis<4;++axis)
        midpoint[axis]=vehicle->centerPosition[axis]*0.5f+eye[axis]*0.5f;
    const float radius=bumperDistanceThree(vehicle->centerPosition,eye)*0.5f+0.5f;
    terrainSurfaceUpdate(&surface,midpoint,radius,NULL);
    vectorMatrixTransformPoint(local,matrix,vehicle->centerPosition);
    const float slope[2]={finiteRatio(local[0],local[2]),finiteRatio(local[1],local[2])};
    carCameraTerrainOffsetWithClearance(&surface,camera->context.ignoredTerrainMaterials,
        0.699999988079071f,vehicle->centerPosition,eye,offset);
    for (int axis=0;axis<4;++axis) {
        eye[axis]+=offset[axis];difference[axis]=eye[axis]-vehicle->centerPosition[axis];
    }
    const float length=sqrtf((difference[0]*difference[0]+difference[1]*difference[1])+difference[2]*difference[2]);
    DriverPhysicsVehicle physics{};physics.movement=camera->context.movement;
    float minimum[4],maximum[4];driverCollisionBounds(&physics,minimum,maximum);
    const float limit=maximumFinite(maximum[0],fabsf(minimum[0]))+0.699999988079071f;
    if (length<limit) {
        const float xy=difference[0]*difference[0]+difference[1]*difference[1];
        eye[2]=vehicle->centerPosition[2]+sqrtf(fabsf(limit*limit-xy));
    }
    carCameraLookAtWithSlope(matrix,eye,vehicle->centerPosition,slope);
    terrainSurfaceDispose(&surface);
}

CarCinematicResult carBumperCameraAdvance(CarBumperCamera* camera)
{
    if (!camera) return CAR_CINEMATIC_INVALID_BINDING;
    CarCinematicServices* services=camera->base.services;
    if (services->session->state==6) return CAR_CINEMATIC_OK;
    const VehicleState* vehicle=camera->context.movement->bindings.state;
    const float percent=camera->context.movement->routeLocation.progress*100.0f;
    const float coordinate=(camera->context.race->reverse?100.0f-percent:percent)*0.009999999776482582f;
    float fraction=finiteRatio(coordinate-camera->items.playback.routeStart,
        camera->items.playback.routeEnd-camera->items.playback.routeStart);
    if (fraction<0.0f) fraction=0.0f;
    camera->eyeParameter=bumperAdvanceParameter(camera->eyeParameter,camera->items.playback.eyeAdvanceRate,
        camera->items.playback.eyeUsesRouteFraction,camera->items.eyeCount,fraction,services->clock->worldTimeElapsed);
    camera->targetParameter=bumperAdvanceParameter(camera->targetParameter,camera->items.playback.targetAdvanceRate,
        camera->items.playback.targetUsesRouteFraction,camera->items.targetCount,fraction,services->clock->worldTimeElapsed);
    float negative[3]={-vehicle->centerPosition[0],-vehicle->centerPosition[1],-vehicle->centerPosition[2]};
    float translation[16],orientation[16],worldToLocal[16];
    motionMatrixTranslation(translation,negative);eTransposeMatrix(orientation,vehicle->orientation);
    vectorMatrixMultiply(worldToLocal,orientation,translation);
    float eye[4],target[4],roll[4],up[4],local[16],matrix[16];
    splineEvaluate(&camera->eye,eye,camera->eyeParameter);
    float noisePoint[4]={vehicle->position[0],vehicle->position[1],0,0};
    const TerrainMaterial* materials=terrainDefaultMaterials(NULL);
    const float first=terrainMaterialDisplacement(materials+2,noisePoint)*0.8769999742507935f;
    noisePoint[1]-=1.0f;
    const float second=terrainMaterialDisplacement(materials+2,noisePoint)*0.8769999742507935f;
    noisePoint[1]+=2.0f;
    const float third=terrainMaterialDisplacement(materials+2,noisePoint)*0.8769999742507935f;
    eye[0]+=first;eye[1]+=second;eye[2]+=third;
    const float minimumZ=terrainSurfaceElevation(&camera->surface,eye,NULL,NULL,NULL)+0.699999988079071f;
    if (eye[2]<minimumZ) eye[2]=minimumZ;
    splineEvaluate(&camera->target,target,camera->targetParameter);
    splineEvaluate(&camera->roll,roll,camera->eyeParameter);
    carCameraUp(up,eye,target,(uint16_t)convertFloatToInt32(roll[0]));
    carCinematicLookAt(local,eye,target,up);
    vectorMatrixMultiply(matrix,local,worldToLocal);
    fraction=finiteRatio(coordinate-camera->items.projection.routeStart,
        camera->items.projection.routeEnd-camera->items.projection.routeStart);
    if (fraction<0.0f) fraction=0.0f;
    if (fraction>1.0f) fraction=1.0f;
    float lens[4];splineEvaluate(&camera->lens,lens,fraction*31.0f);
    uint16_t fieldOfView;
    if (camera->items.projection.usesWidth) {
        float distance=bumperDistanceThree(eye,vehicle->position);if (distance<1.0f) distance=1.0f;
        fieldOfView=(uint16_t)(vectorAtan2Angle(distance,lens[0]*0.25f)*2u);
    } else fieldOfView=(uint16_t)convertFloatToInt32(lens[0]*182.04444885253906f);
    if (!services->suppressPublication) {
        const float focus=bumperDistanceThree(eye,target);
        camera->base.view->current.fieldOfView=fieldOfView;
        camera->base.view->current.focusDistance=focus;
        camera->base.view->current.minimumVisibility=maximumFinite(2.0f,focus);
        camera->base.view->current.maximumVisibility=7.5f;
    }
    if (camera->context.raceFrame->state==4) carBumperCameraConstrain(camera,matrix);
    carCinematicViewPublish(services,camera->base.view,matrix,services->clock->realTimeElapsed);
    return CAR_CINEMATIC_OK;
}

CarCinematicBase* carBumperCameraBase(CarBumperCamera* camera) { return camera?&camera->base:NULL; }

static float airborneLengthThree(const float value[4])

{

    const float x = value[0] * value[0];

    const float y = value[1] * value[1];

    const float z = value[2] * value[2];

    return sqrtf((x + y) + z);

}

static float airborneRouteCoordinate(const CarAirborneCamera* camera)

{

    const float percent = camera->movement->routeLocation.progress * 100.0f;

    const float directed = camera->race->reverse ? 100.0f - percent : percent;

    return directed * 0.009999999776482582f;

}

static float airborneBoundFraction(float value)

{

    if (value < 0.0f) value = 0.0f;

    if (value > 1.0f) value = 1.0f;

    return value;

}

static float airborneFramingValue(const CarAirborneCamera* camera, int axis, float fraction,

    float distance, uint16_t halfFov)

{

    float value = camera->items.legacy.start[axis];

    if (camera->items.legacy.interpolate) {

        const float change = camera->items.legacy.end[axis] - value;

        value += fraction * change;

    }

    const float tangent = finiteRatio(rotationSineAngle(halfFov), vectorCosineAngle(halfFov));

    return (value * distance) * tangent;

}

void carAirborneCameraSettingsInitialize(CarAirborneCameraSettings* settings)

{

    if (settings) memset(settings, 0, sizeof(*settings));

}

CarAirborneCamera* carAirborneCameraCreate(const CarAirborneCameraContext* context,

    CarCinematicResult* result)

{

    if (result) *result = CAR_CINEMATIC_INVALID_BINDING;

    if (!context || !context->services || !context->services->clock || !context->services->session ||

        !context->movement || !context->movement->bindings.state || !context->race ||

        !context->settings || !context->items || !context->offset ||

        !context->items->projection.lensKeys || !context->items->eyeKeys || !context->items->rollKeys ||

        context->items->eyeCount < 1 || context->offset->count < 1 || context->offset->count > 32 ||

        context->viewNumber < 0 || context->viewNumber > 3 ||

        !context->services->screens[context->viewNumber].view) return NULL;

    CarAirborneCamera* camera = (CarAirborneCamera*)calloc(1, sizeof(*camera));

    if (!camera) { if (result) *result = CAR_CINEMATIC_ALLOCATION_FAILURE; return NULL; }

    if (carCinematicBaseInitialize(&camera->base, context->services, context->viewNumber, 7) != CAR_CINEMATIC_OK) {

        free(camera); return NULL;

    }
    carCinematicBindOwner(&camera->base,camera,&carCinematicAirborneMethods);

    camera->base.player = context->player;

    camera->movement = context->movement;

    camera->race = context->race;

    camera->items = *context->items;

    camera->offset = *context->offset;

    /* Spline evaluation borrows these points and never mutates them. The

     * established Spline API also supports mutable callers, hence the cast. */

    splineInitializeBorrowed(&camera->lens, SPLINE_CLAMP, 32,

        (float (*)[4])camera->items.projection.lensKeys, 16384);

    splineInitializeBorrowed(&camera->eye, SPLINE_CLAMP, camera->items.eyeCount,

        (float (*)[4])camera->items.eyeKeys, 16384);

    splineInitializeBorrowed(&camera->roll, SPLINE_CLAMP, camera->items.eyeCount,

        (float (*)[4])camera->items.rollKeys, 16384);

    splineInitializeBorrowed(&camera->offsets, SPLINE_CLAMP, camera->offset.count,

        camera->offset.keys, 16384);

    tCubic1DInitialize(&camera->focusNoise);

    camera->focusNoise.duration = 0.41999998688697815f;

    if (!camera->base.services->suppressPublication) {

        camera->base.view->hiddenCar = NULL;

        camera->base.view->visibilityFlags = 0;

    }

    memcpy(camera->anchor, camera->movement->bindings.state->position, sizeof(camera->anchor));

    const float* override = context->settings->directionOverride;

    if (override[0] != 0.0f || override[1] != 0.0f || override[2] != 0.0f)

        memcpy(camera->directionVelocity, override, sizeof(camera->directionVelocity));

    else {

        camera->directionVelocity[0] = camera->items.legacy.authoredCoefficients[0];

        camera->directionVelocity[1] = camera->items.legacy.authoredCoefficients[1];

        camera->directionVelocity[2] = camera->items.legacy.authoredCoefficients[1];

    }

    camera->directionWeight = camera->items.legacy.authoredCoefficients[4];

    camera->velocityWeight = camera->items.legacy.authoredCoefficients[3];

    memcpy(camera->firstEye, camera->items.eyeKeys[0], sizeof(camera->firstEye));

    memcpy(camera->lastEye, camera->items.eyeKeys[camera->items.eyeCount - 1], sizeof(camera->lastEye));

    camera->closestDistance = 1000000.0f;

    camera->base.view->current.nearClip = camera->items.nearClip;

    camera->base.view->current.farClip = camera->items.farClip;

    int32_t initialNoise = (int32_t)vectorDefaultRandomBounded(19) + 26;

    if (vectorDefaultRandomBounded(2)) initialNoise = -initialNoise;

    camera->focusNoise.value = (float)initialNoise;

    camera->focusNoise.mode = 2;

    camera->extendedVisibility = context->extendedVisibility;

    if (!camera->extendedVisibility && !camera->base.services->suppressPublication) {

        camera->base.view->current.minimumVisibility = 0.0f;

        camera->base.view->current.maximumVisibility = 0.0f;

    }

    if (result) *result = CAR_CINEMATIC_OK;

    return camera;

}

void carAirborneCameraDispose(CarAirborneCamera* camera)

{

    if (!camera) return;

    if (!camera->base.services->suppressPublication) {

        camera->base.view->current.minimumVisibility = 0.0f;

        camera->base.view->current.maximumVisibility = 0.0f;

    }

    camera->base.view->resetHistory = 1;

    splineDestroy(&camera->offsets, 0);

    splineDestroy(&camera->roll, 0);

    splineDestroy(&camera->eye, 0);

    splineDestroy(&camera->lens, 0);

    carCinematicBaseDispose(&camera->base);

    free(camera);

}

CarCinematicBase* carAirborneCameraBase(CarAirborneCamera* camera) { return camera ? &camera->base : NULL; }

CarCinematicResult carAirborneCameraAdvance(CarAirborneCamera* camera)

{

    if (!camera) return CAR_CINEMATIC_INVALID_BINDING;

    CarCinematicServices* services = camera->base.services;

    if (services->session->state == 6) return CAR_CINEMATIC_OK;

    const VehicleState* state = camera->movement->bindings.state;

    const float elapsed = services->clock->worldTimeElapsed;

    const float coordinate = airborneRouteCoordinate(camera);

    const float fraction = airborneBoundFraction(finiteRatio(coordinate - camera->items.playback.routeStart,

        camera->items.playback.routeEnd - camera->items.playback.routeStart));

    float eye[4], up[4] = {0.0f, 0.0f, 1.0f, 0.0f};

    if (camera->items.legacy.eyeProjectionGate == 0.0f) {

        if (camera->items.playback.eyeUsesRouteFraction)

            camera->eyeParameter = (float)(camera->items.eyeCount - 1) * fraction;

        else {

            const float change = (camera->items.playback.eyeAdvanceRate * elapsed) * 25.0f;

            camera->eyeParameter += change;

        }

        if (camera->eyeParameter > (float)(camera->items.eyeCount - 1))

            camera->eyeParameter = (float)(camera->items.eyeCount - 1);

        splineEvaluate(&camera->eye, eye, camera->eyeParameter);

    } else {

        if (!camera->passedClosestPoint) {

            float line[4], relative[4], unitLine[4];

            for (int axis = 0; axis < 4; ++axis) {

                line[axis] = camera->lastEye[axis] - camera->firstEye[axis];

                relative[axis] = state->position[axis] - camera->firstEye[axis];

            }

            const float numerator = (line[0] * relative[0] + line[1] * relative[1]) + line[2] * relative[2];

            const float denominator = (line[0] * line[0] + line[1] * line[1]) + line[2] * line[2];

            const float projected = airborneBoundFraction(finiteRatio(numerator, denominator));

            vectorSetLength3(unitLine, line, camera->items.legacy.eyeProjectionGate);

            for (int axis = 0; axis < 4; ++axis) eye[axis] = (camera->firstEye[axis] + line[axis] * projected) + unitLine[axis];

        } else memcpy(eye, camera->base.view->current.eye, sizeof(eye));

        if (camera->items.legacy.closestApproachGate) {

            float delta[4];

            for (int axis = 0; axis < 4; ++axis) delta[axis] = state->position[axis] - eye[axis];

            const float distance = airborneLengthThree(delta);

            if (camera->closestDistance < distance) camera->passedClosestPoint = 1;

            camera->closestDistance = distance;

        }

    }

    const float lensFraction = airborneBoundFraction(finiteRatio(coordinate - camera->items.projection.routeStart,

        camera->items.projection.routeEnd - camera->items.projection.routeStart));

    float lens[4], relative[4];

    splineEvaluate(&camera->lens, lens, lensFraction * 31.0f);

    for (int axis = 0; axis < 4; ++axis) relative[axis] = state->position[axis] - eye[axis];

    float distance = airborneLengthThree(relative);

    if (distance < 1.0f) distance = 1.0f;

    uint16_t fieldOfView;

    if (camera->items.projection.usesWidth)

        fieldOfView = (uint16_t)(vectorAtan2Angle(distance, lens[0] * 0.25f) * 2u);

    else fieldOfView = (uint16_t)convertFloatToInt32(lens[0] * 182.04444885253906f);

    if (!services->suppressPublication) camera->base.view->current.fieldOfView = fieldOfView;

    camera->offsetParameter += camera->offset.advanceRate * elapsed;

    if (camera->offsetParameter > (float)(camera->offset.count - 1))

        camera->offsetParameter = (float)(camera->offset.count - 1);

    float offset[4];

    splineEvaluate(&camera->offsets, offset, camera->offsetParameter);

    memcpy(camera->anchor, state->position, sizeof(camera->anchor));

    const uint16_t halfFov = fieldOfView >> 1;

    const float vertical = airborneFramingValue(camera, 0, fraction, distance, halfFov);

    camera->anchor[2] += vertical - offset[1];

    const float horizontal = airborneFramingValue(camera, 1, fraction, distance, halfFov);

    const float reciprocalDistance = finiteRatio(1.0f, distance);

    for (int axis = 0; axis < 4; ++axis) relative[axis] *= reciprocalDistance;

    const float side[4] = {-relative[1], relative[0], 0.0f, 0.0f};

    const float sideScale = horizontal + offset[0];

    camera->anchor[0] += side[0] * sideScale;

    camera->anchor[1] += side[1] * sideScale;

    const float authoredOffset[4] = {camera->items.legacy.authoredCoefficients[5],

        camera->items.legacy.authoredCoefficients[6], camera->items.legacy.authoredCoefficients[7], 1.0f};

    float transformed[4], direction[4], weighted[4];

    vectorMatrixTransformPoint(transformed, state->orientation, authoredOffset);

    for (int axis = 0; axis < 4; ++axis) {

        camera->anchor[axis] += transformed[axis];

        direction[axis] = camera->anchor[axis] - eye[axis];

    }

    const float reciprocalLength = finiteRatio(1.0f, airborneLengthThree(direction));

    for (int axis = 0; axis < 4; ++axis) direction[axis] *= reciprocalLength;

    carAirborneCameraWeightedDirection(camera, weighted, camera->directionDisplacement, camera->directionVelocity);

    for (int axis = 0; axis < 4; ++axis) {

        camera->directionVelocity[axis] -= weighted[axis];

        camera->directionDisplacement[axis] += camera->directionVelocity[axis];

        direction[axis] += camera->directionDisplacement[axis];

        camera->anchor[axis] = eye[axis] + direction[axis];

    }

    float roll[4], matrix[16];

    splineEvaluate(&camera->roll, roll, camera->eyeParameter);

    carCameraUp(up, eye, camera->anchor, (uint16_t)convertFloatToInt32(roll[0]));

    carCinematicLookAt(matrix, eye, camera->anchor, up);

    for (int axis = 0; axis < 4; ++axis) relative[axis] = state->position[axis] - eye[axis];

    const float focus = airborneLengthThree(relative);

    if (!services->suppressPublication) camera->base.view->current.focusDistance = focus;

    camera->base.view->current.nearClip = 0.5f;

    camera->focusNoise.desiredDerivative = camera->focusNoise.value * 6.269999980926514f;

    tCubic1DUpdate(&camera->focusNoise, elapsed, 0.0f, 0.0f);

    float visible = focus + camera->focusNoise.value;

    if (visible < 2.0f) visible = 2.0f;

    if (camera->extendedVisibility && !services->suppressPublication) {

        camera->base.view->current.minimumVisibility = visible + camera->focusNoise.value;

        camera->base.view->current.maximumVisibility = 7.5f;

    }

    carCinematicViewPublish(services, camera->base.view, matrix, services->clock->realTimeElapsed);

    return CAR_CINEMATIC_OK;

}

static float pathRouteCoordinate(const CarPathCamera* camera)

{

    const float percent = camera->movement->routeLocation.progress * 100.0f;

    return (camera->race->reverse ? 100.0f - percent : percent) * 0.009999999776482582f;

}

static float pathLengthBetween(const float first[4], const float second[4])

{

    const float x = first[0] - second[0], y = first[1] - second[1], z = first[2] - second[2];

    return sqrtf((x * x + y * y) + z * z);

}

static float pathAdvanceParameter(float current, float rate, uint32_t useRoute,

    int attached, int32_t count, float fraction, float elapsed)

{

    float parameter;

    if (useRoute && attached) parameter = (float)(count - 1) * fraction;

    else parameter = current + (rate * elapsed) * 25.0f;

    if (parameter > (float)(count - 1)) parameter = (float)(count - 1);

    return parameter;

}

CarPathCamera* carPathCameraCreate(const CarPathCameraContext* context, CarCinematicResult* result)

{

    if (result) *result = CAR_CINEMATIC_INVALID_BINDING;

    if (!context || !context->services || !context->services->clock || !context->items ||

        !context->items->eyeKeys || !context->items->rollKeys || !context->items->targetKeys ||

        !context->items->projection.lensKeys || context->items->eyeCount < 1 || context->items->targetCount < 1 ||

        (context->movement && (!context->movement->bindings.state || !context->race)) ||

        context->viewNumber < 0 || context->viewNumber > 3 || !context->services->screens[context->viewNumber].view) return NULL;

    CarPathCamera* camera = (CarPathCamera*)calloc(1, sizeof(*camera));

    if (!camera) { if (result) *result = CAR_CINEMATIC_ALLOCATION_FAILURE; return NULL; }

    if (carCinematicBaseInitialize(&camera->base, context->services, context->viewNumber, 9) != CAR_CINEMATIC_OK) {

        free(camera); return NULL;

    }
    carCinematicBindOwner(&camera->base,camera,&carCinematicPathMethods);

    camera->base.player = context->player;

    camera->movement = context->movement;

    camera->race = context->race;

    camera->items = *context->items;

    splineInitializeBorrowed(&camera->eye, SPLINE_CLAMP, camera->items.eyeCount, (float (*)[4])camera->items.eyeKeys, 16384);

    splineInitializeBorrowed(&camera->roll, SPLINE_CLAMP, camera->items.eyeCount, (float (*)[4])camera->items.rollKeys, 16384);

    splineInitializeBorrowed(&camera->target, SPLINE_CLAMP, camera->items.targetCount, (float (*)[4])camera->items.targetKeys, 16384);

    splineInitializeBorrowed(&camera->lens, SPLINE_CLAMP, 32, (float (*)[4])camera->items.projection.lensKeys, 16384);

    camera->base.view->current.nearClip = camera->items.nearClip;

    camera->base.view->current.farClip = camera->items.farClip;

    if (result) *result = CAR_CINEMATIC_OK;

    return camera;

}

void carPathCameraDispose(CarPathCamera* camera)

{

    if (!camera) return;

    if (!camera->base.services->suppressPublication) {

        camera->base.view->current.minimumVisibility = 0.0f;

        camera->base.view->current.maximumVisibility = 0.0f;

    }

    camera->base.view->resetHistory = 1;

    splineDestroy(&camera->lens, 0);

    splineDestroy(&camera->target, 0);

    splineDestroy(&camera->roll, 0);

    splineDestroy(&camera->eye, 0);

    carCinematicBaseDispose(&camera->base);

    free(camera);

}

const float* carPathCameraFocusPosition(const CarPathCamera* camera)

{

    return camera->movement ? camera->movement->bindings.state->centerPosition : camera->base.view->current.target;

}

CarCinematicBase* carPathCameraBase(CarPathCamera* camera) { return camera ? &camera->base : NULL; }

CarCinematicResult carPathCameraAdvance(CarPathCamera* camera)

{

    if (!camera) return CAR_CINEMATIC_INVALID_BINDING;

    CarCinematicServices* services = camera->base.services;

    float fraction = 0.0f;

    if (camera->movement) {

        fraction = finiteRatio(pathRouteCoordinate(camera) - camera->items.playback.routeStart,

            camera->items.playback.routeEnd - camera->items.playback.routeStart);

        if (fraction < 0.0f) fraction = 0.0f;

    }

    camera->targetParameter = pathAdvanceParameter(camera->targetParameter, camera->items.playback.targetAdvanceRate,

        camera->items.playback.targetUsesRouteFraction, camera->movement != NULL, camera->items.targetCount,

        fraction, services->clock->worldTimeElapsed);

    camera->eyeParameter = pathAdvanceParameter(camera->eyeParameter, camera->items.playback.eyeAdvanceRate,

        camera->items.playback.eyeUsesRouteFraction, camera->movement != NULL, camera->items.eyeCount,

        fraction, services->clock->worldTimeElapsed);

    float eye[4], target[4], roll[4], up[4], matrix[16];

    splineEvaluate(&camera->eye, eye, camera->eyeParameter);

    splineEvaluate(&camera->target, target, camera->targetParameter);

    splineEvaluate(&camera->roll, roll, camera->eyeParameter);

    carCameraUp(up, eye, target, (uint16_t)convertFloatToInt32(roll[0]));

    carCinematicLookAt(matrix, eye, target, up);

    uint16_t fieldOfView = 13020;

    if (camera->movement) {

        fraction = finiteRatio(pathRouteCoordinate(camera) - camera->items.projection.routeStart,

            camera->items.projection.routeEnd - camera->items.projection.routeStart);

        if (fraction < 0.0f) fraction = 0.0f;

        if (fraction > 1.0f) fraction = 1.0f;

        float lens[4];

        splineEvaluate(&camera->lens, lens, fraction * 31.0f);

        if (camera->items.projection.usesWidth) {

            float distance = pathLengthBetween(camera->movement->bindings.state->position, eye);

            if (distance < 1.0f) distance = 1.0f;

            fieldOfView = (uint16_t)(vectorAtan2Angle(distance, lens[0] * 0.25f) * 2u);

        } else fieldOfView = (uint16_t)convertFloatToInt32(lens[0] * 182.04444885253906f);

    }

    const float focus = pathLengthBetween(eye, target);

    if (!services->suppressPublication) {

        camera->base.view->current.fieldOfView = fieldOfView;

        camera->base.view->current.focusDistance = focus;

        camera->base.view->current.minimumVisibility = maximumFinite(2.0f, focus);

        camera->base.view->current.maximumVisibility = 30.0f;

    }

    carCinematicViewPublish(services, camera->base.view, matrix, services->clock->realTimeElapsed);

    return CAR_CINEMATIC_OK;

}

/* Concrete camera movement. */
void carFixedCameraCommandsInitialize(CarFixedCameraCommands* commands)
{
    memset(commands,0,sizeof(*commands));
    commands->markerRequest=-1;
    commands->firstModifierMultiplier=3.059999942779541f;
    commands->secondModifierMultiplier=7.159999847412109f;
}

CarFixedCamera* carFixedCameraCreate(const CarFixedCameraContext* context,
    const float position[4],const float target[4],CarCinematicResult* result)
{
    if(result)*result=CAR_CINEMATIC_INVALID_BINDING;
    if(!context||!context->cameras||!context->cameras->clock||!context->players||
       !context->players->input||!context->terrain||!context->commands||!position||!target)
        return NULL;
    const GameInputRouter* input=context->players->input;
    if(input->count>input->capacity||input->capacity-input->count<15||
       input->nextOrder>UINT64_MAX-15){
        if(result)*result=CAR_CINEMATIC_BACKEND_CAPACITY;
        return NULL;
    }
    CarFixedCamera* camera=static_cast<CarFixedCamera*>(calloc(1,sizeof(*camera)));
    if(!camera){if(result)*result=CAR_CINEMATIC_ALLOCATION_FAILURE;return NULL;}
    CarCinematicResult status=carCinematicBaseInitialize(&camera->base,context->cameras,
        context->viewNumber,2);
    if(status!=CAR_CINEMATIC_OK){free(camera);if(result)*result=status;return NULL;}
    carCinematicBindOwner(&camera->base,camera,&carCinematicFixedMethods);
    camera->input.services=context->players;
    camera->input.device=context->device;
    camera->terrain=context->terrain;
    camera->commands=context->commands;
    camera->lastCaptureFrame=INT_MIN;
    context->players->fixed.modifiers[0]=context->players->fixed.modifiers[1]=0;
    memcpy(context->players->fixed.position,position,sizeof(context->players->fixed.position));
    memcpy(context->players->fixed.target,target,sizeof(context->players->fixed.target));
    if(carPlayerFixedCameraActivateInput(&camera->input)!=CAR_PLAYER_CAMERA_OK){
        carCinematicBaseDispose(&camera->base);free(camera);
        if(result)*result=CAR_CINEMATIC_BACKEND_CAPACITY;
        return NULL;
    }
    if(result)*result=CAR_CINEMATIC_OK;
    return camera;
}

void carFixedCameraDispose(CarFixedCamera* camera)
{
    if(!camera)return;
    carPlayerFixedCameraDeactivateInput(&camera->input);
    carCinematicBaseDispose(&camera->base);
    free(camera);
}

static void fixedOffsetFromTarget(float offset[4],const CarFixedCameraSharedState* state)
{
    for(int axis=0;axis<4;++axis)offset[axis]=state->position[axis]-state->target[axis];
    vectorSetLength3(offset,offset,10.0f);
}

static void fixedConsumeCommands(CarFixedCamera* camera)
{
    CarFixedCameraCommands* commands=camera->commands;
    CarFixedCameraSharedState* state=&camera->input.services->fixed;
    if(commands->markerRequest>=0){
        const CarCameraMarkerPositions* markers=commands->markers;
        if(markers&&markers->enabled&&markers->positions&&
           (size_t)commands->markerRequest<markers->count){
            float offset[4];
            fixedOffsetFromTarget(offset,state);
            const float* point=markers->positions[commands->markerRequest];
            for(int axis=0;axis<4;++axis){
                state->position[axis]=point[axis]+offset[axis];
                state->target[axis]=point[axis];
            }
        }
        commands->markerRequest=-1;
    }
    if(commands->targetRequest[1]!=0.0f){
        TerrainSurface surface;
        terrainSurfaceInitialize(&surface,camera->terrain);
        commands->targetRequest[2]=terrainSurfaceElevation(&surface,commands->targetRequest,
            NULL,NULL,NULL)+3.0f;
        float offset[4];
        fixedOffsetFromTarget(offset,state);
        for(int axis=0;axis<4;++axis){
            state->position[axis]=commands->targetRequest[axis]+offset[axis];
            state->target[axis]=commands->targetRequest[axis];
        }
        commands->targetRequest[0]=commands->targetRequest[1]=commands->targetRequest[2]=0.0f;
        terrainSurfaceDispose(&surface);
    }
}

static float fixedMovementScale(const CarFixedCamera* camera,float rate)
{
    const int32_t* modifiers=camera->input.services->fixed.modifiers;
    if(modifiers[1])rate=rate*camera->commands->secondModifierMultiplier;
    else if(modifiers[0])rate=rate*camera->commands->firstModifierMultiplier;
    return rate*camera->base.services->clock->worldTimeElapsed;
}

static void fixedTranslate(CarFixedCameraSharedState* state,const float offset[4])
{
    for(int axis=0;axis<4;++axis){
        state->position[axis]=state->position[axis]+offset[axis];
        state->target[axis]=state->target[axis]+offset[axis];
    }
}

CarCinematicResult carFixedCameraAdvance(CarFixedCamera* camera)
{
    if(!camera||!camera->base.view)return CAR_CINEMATIC_INVALID_BINDING;
    fixedConsumeCommands(camera);
    CarPlayerFixedCamera* input=&camera->input;
    CarFixedCameraSharedState* state=&input->services->fixed;
    const GameTime* clock=camera->base.services->clock;
    CarFixedCameraCapture* capture=camera->commands->capture;
    int32_t transmit=0;
    if(capture&&capture->enabled&&!camera->base.services->suppressOverride){
        const int32_t frames=(int32_t)((uint32_t)clock->realTime-(uint32_t)camera->lastCaptureFrame);
        const int32_t negative=(int32_t)(0u-(uint32_t)frames);
        const int32_t magnitude=frames<negative?negative:frames;
        if(16<magnitude){camera->lastCaptureFrame=clock->realTime;transmit=1;}
    }
    const int32_t moving=input->headingRate||input->pitchRate||input->movementRate[0]!=0.0f||
        input->movementRate[1]!=0.0f||input->movementRate[2]!=0.0f||input->analogForwardRate!=0.0f;
    float direction[4];
    for(int axis=0;axis<4;++axis)direction[axis]=state->target[axis]-state->position[axis];
    uint16_t heading=vectorFixedAtan2Angle(convertFloatToInt32(direction[0]*65536.0f),
        convertFloatToInt32(direction[1]*65536.0f));
    if(input->headingRate||input->pitchRate||input->movementRate[0]!=0.0f){
        const float headingChange=(float)input->headingRate*clock->worldTimeElapsed;
        heading=(uint16_t)(heading+(uint32_t)convertFloatToInt32(headingChange));
        const float x=direction[0]*direction[0],y=direction[1]*direction[1];
        uint16_t pitch=vectorFixedAtan2Angle(convertFloatToInt32(sqrtf(x+y)*65536.0f),
            convertFloatToInt32(direction[2]*65536.0f));
        const float pitchChange=(float)input->pitchRate*clock->worldTimeElapsed;
        pitch=(uint16_t)(pitch+(uint32_t)convertFloatToInt32(pitchChange));
        if((uint16_t)(pitch-16375u)<=16392u)pitch=16374u;
        if((uint16_t)(pitch-32768u)<=16393u)pitch=49162u;
        state->position[2]=state->position[2]+fixedMovementScale(camera,input->movementRate[0]);
        const float cosine=vectorCosineAngle(pitch);
        const float xRadius=vectorCosineAngle(heading)*20.0f;
        state->target[0]=(cosine*xRadius)+state->position[0];
        const float yRadius=rotationSineAngle(heading)*20.0f;
        state->target[1]=(cosine*yRadius)+state->position[1];
        state->target[2]=(rotationSineAngle(pitch)*20.0f)+state->position[2];
    }
    if(input->movementRate[1]!=0.0f){
        const float distance=fixedMovementScale(camera,input->movementRate[1]);
        const float offset[4]={vectorCosineAngle(heading)*distance,
            rotationSineAngle(heading)*distance,0.0f,0.0f};
        fixedTranslate(state,offset);
    }
    if(input->analogForwardRate!=0.0f){
        float offset[4];
        for(int axis=0;axis<4;++axis)offset[axis]=state->target[axis]-state->position[axis];
        vectorSetLength3(offset,offset,fixedMovementScale(camera,input->analogForwardRate));
        fixedTranslate(state,offset);
    }
    if(input->movementRate[2]!=0.0f){
        const float distance=fixedMovementScale(camera,input->movementRate[2]);
        const uint16_t perpendicular=(uint16_t)(heading+16384u);
        const float offset[4]={vectorCosineAngle(perpendicular)*distance,
            rotationSineAngle(perpendicular)*distance,0.0f,0.0f};
        fixedTranslate(state,offset);
    }
    float up[4],matrix[16];
    carCameraUp(up,state->position,state->target,0);
    carCinematicLookAt(matrix,state->position,state->target,up);
    if(!camera->base.services->suppressPublication)camera->base.view->current.fieldOfView=13020;
    if(capture&&capture->enabled&&!camera->base.services->suppressOverride&&moving){
        for(int axis=0;axis<3;++axis){
            capture->position[axis]=convertFloatToInt32(state->position[axis]*65536.0f);
            capture->target[axis]=convertFloatToInt32(state->target[axis]*65536.0f);
        }
        capture->pending=1;
        if(transmit){capture->transmissionRequested=1;capture->pending=0;}
    }
    carCinematicViewPublish(camera->base.services,camera->base.view,matrix,clock->realTimeElapsed);
    return CAR_CINEMATIC_OK;
}

CarCinematicBase* carFixedCameraBase(CarFixedCamera* camera)
{
    return camera?&camera->base:NULL;
}

CarPlayerFixedCamera* carFixedCameraInput(CarFixedCamera* camera)
{
    return camera?&camera->input:NULL;
}

CarPlayerCameraResult carPlayerFixedCameraActivateInput(CarPlayerFixedCamera* camera)
{
    if (camera->inputEnabled) return camera->result;
    if (!camera->services || !camera->services->input)
        return camera->result = CAR_PLAYER_CAMERA_INVALID_BINDING;
    GameInputTarget target{};target.kind=GAME_INPUT_FIXED_CAMERA;target.owner.fixedCamera=camera;
    for (int32_t event = 0x37; event <= 0x45; ++event)
        if (gameInputRegister(camera->services->input, camera->device, event, target) != GAME_INPUT_OK)
            return camera->result = CAR_PLAYER_CAMERA_INPUT_CAPACITY;
    camera->inputEnabled = 1;
    return camera->result = CAR_PLAYER_CAMERA_OK;
}

void carPlayerFixedCameraDeactivateInput(CarPlayerFixedCamera* camera)
{
    if (!camera->inputEnabled) return;
    for (int32_t event = 0x37; event <= 0x45; ++event)
        gameInputUnregister(camera->services->input, camera->device, event);
    camera->inputEnabled = 0;
}

void carPlayerFixedCameraDispatchInput(CarPlayerFixedCamera* camera,
    int32_t device, int32_t event, uint8_t value)
{
    (void)device;
    switch (event) {
    case 0x37: camera->headingRate = value ? 11000 : 0; break;
    case 0x38: camera->headingRate = value ? -11000 : 0; break;
    case 0x39: camera->pitchRate = value ? -11000 : 0; break;
    case 0x3A: camera->pitchRate = value ? 11000 : 0; break;
    case 0x3B: camera->movementRate[0] = value ? -7.5f : 0.0f; break;
    case 0x3C: camera->movementRate[0] = value ? 7.5f : 0.0f; break;
    case 0x3D:
        camera->analogForwardRate = 0.0f;
        camera->movementRate[1] = value ? 30.0f : 0.0f;
        break;
    case 0x3E:
        camera->analogForwardRate = 0.0f;
        camera->movementRate[1] = value ? -30.0f : 0.0f;
        break;
    case 0x3F: camera->movementRate[2] = value ? 30.0f : 0.0f; break;
    case 0x40: camera->movementRate[2] = value ? -30.0f : 0.0f; break;
    case 0x41: camera->services->fixed.modifiers[0] = value != 0; break;
    case 0x42: camera->services->fixed.modifiers[1] = value != 0; break;
    case 0x43:
    case 0x44:
        /* The original selection request is read only by the retired
         * diagnostic car-list formatter, then cleared. */
        break;
    case 0x45: {
        CarFramePlayerState* player = camera->services->players->players[0];
        if (!player) break;
        const float* first = camera->services->fixed.position;
        const float* target = camera->services->fixed.target;
        const float x = target[0] - first[0];
        const float y = target[1] - first[1];
        const float z = target[2] - first[2];
        const float xx = x * x, yy = y * y, zz = z * z;
        const float reciprocal = finiteRatio(1.0f, sqrtf((xx + yy) + zz));
        const float normalized[3] = {x * reciprocal, y * reciprocal, z * reciprocal};
        float position[3];
        for (int axis = 0; axis < 3; ++axis) {
            const float offset = normalized[axis] * 10.0f;
            position[axis] = offset + first[axis];
        }
        if (carMovementPlaceAndNotify(player->car, player->driver, position, 0, 0.0f) != CAR_MOVEMENT_OK)
            camera->result = CAR_PLAYER_CAMERA_MOVEMENT_FAILURE;
        break;
    }
    case 0x46: {
        const float centered = (float)value - 128.0f;
        camera->headingRate = (int16_t)convertFloatToInt32(centered * -110.0f);
        break;
    }
    case 0x47: {
        const float centered = (float)value - 128.0f;
        camera->pitchRate = (int16_t)convertFloatToInt32(centered * 110.0f);
        break;
    }
    case 0x48: {
        const float centered = (float)value - 128.0f;
        camera->movementRate[2] = centered * -0.20000000298023224f;
        break;
    }
    case 0x49: {
        const float centered = (float)value - 128.0f;
        camera->analogForwardRate = centered * -0.20000000298023224f;
        break;
    }
    default: break;
    }
}

void carOrbitCameraSettingsInitialize(CarOrbitCameraSettings* settings)

{

    memset(settings,0,sizeof(*settings));

    settings->overrideDistance=5.0f;

    settings->overrideHeading=26000;

    settings->overridePitch=62000;

    settings->neutralDistance=0.30000001192092896f;

    settings->carInitialFieldOfView=13020;

    settings->fixedInitialFieldOfView=6000;

    settings->fieldOfView=8000;

}

const CarMovement* carOrbitCameraSelectedMovement(const CarOrbitCamera* camera)

{

    const Car* car=camera->input.selectedCar;

    for(size_t index=0;index<camera->context.vehicleCount;++index){

        const CarMovement* movement=camera->context.vehicles[index];

        if(movement&&movement->bindings.owner==car)return movement;

    }

    return NULL;

}

static CarOrbitCamera* orbitCreate(const CarOrbitCameraContext* context,Car* car,

    const float* target,int32_t fixedTarget,CarCinematicResult* result)

{

    if(result)*result=CAR_CINEMATIC_INVALID_BINDING;

    if(!context||!context->cameras||!context->cameras->clock||!context->players||

       !context->players->input||!context->settings||

       (context->vehicleCount&&!context->vehicles))return NULL;

    const GameInputRouter* input=context->players->input;

    if(input->count>input->capacity||input->capacity-input->count<9||input->nextOrder>UINT64_MAX-9){

        if(result)*result=CAR_CINEMATIC_BACKEND_CAPACITY;

        return NULL;

    }

    CarOrbitCamera* camera=static_cast<CarOrbitCamera*>(calloc(1,sizeof(*camera)));

    if(!camera){if(result)*result=CAR_CINEMATIC_ALLOCATION_FAILURE;return NULL;}

    const CarCinematicResult status=carCinematicBaseInitialize(&camera->base,context->cameras,

        context->viewNumber,4);

    if(status!=CAR_CINEMATIC_OK){free(camera);if(result)*result=status;return NULL;}
    carCinematicBindOwner(&camera->base,camera,&carCinematicOrbitMethods);

    camera->context=*context;

    camera->input.services=context->players;

    camera->input.device=context->device;

    camera->input.fixedTargetMode=fixedTarget;

    camera->input.selectedCar=car;

    camera->input.fixedTarget=target;

    camera->input.distanceFactor=1.0f;

    camera->distance=camera->requestedDistance=5.0f;

    camera->minimumDistance=fixedTarget?4.300000190734863f:1.0f;

    camera->maximumDistance=1000.0f;

    camera->heading=26000;

    camera->pitch=camera->centeredPitch=62000;

    camera->acceleration=fixedTarget?10:24;

    if(!context->cameras->suppressPublication&&camera->base.view){

        camera->base.view->hiddenCar=NULL;

        camera->base.view->visibilityFlags=0;

        camera->base.view->current.fieldOfView=fixedTarget?context->settings->fixedInitialFieldOfView:

            context->settings->carInitialFieldOfView;

    }

    if(carPlayerOrbitCameraActivateInput(&camera->input)!=CAR_PLAYER_CAMERA_OK){

        carCinematicBaseDispose(&camera->base);free(camera);

        if(result)*result=CAR_CINEMATIC_BACKEND_CAPACITY;

        return NULL;

    }

    if(result)*result=CAR_CINEMATIC_OK;

    return camera;

}

CarOrbitCamera* carOrbitCameraCreateForCar(const CarOrbitCameraContext* context,

    Car* car,CarCinematicResult* result)

{

    return orbitCreate(context,car,NULL,0,result);

}

CarOrbitCamera* carOrbitCameraCreateAtTarget(const CarOrbitCameraContext* context,

    const float* target,CarCinematicResult* result)

{

    return orbitCreate(context,NULL,target,1,result);

}

void carOrbitCameraDispose(CarOrbitCamera* camera)

{

    if(!camera)return;

    carPlayerOrbitCameraDeactivateInput(&camera->input);

    if(!camera->base.services->suppressPublication&&camera->base.view){

        camera->base.view->current.minimumVisibility=0.0f;

        camera->base.view->current.maximumVisibility=0.0f;

    }

    carCinematicBaseDispose(&camera->base);

    free(camera);

}

static int32_t orbitApproachRate(int32_t value,int32_t target,int32_t step)

{

    const int32_t difference=value<target?(int32_t)((uint32_t)target-(uint32_t)value):

        (int32_t)((uint32_t)value-(uint32_t)target);

    const int32_t next=value<target?(int32_t)((uint32_t)value+(uint32_t)step):

        (int32_t)((uint32_t)value-(uint32_t)step);

    return step<difference?next:target;

}

static void orbitTargetTransform(float matrix[16],const float position[4],

    const float orientation[16],float height)

{

    float translation[16],transpose[16];

    rotationMatrixIdentity(translation);

    translation[12]=-position[0];

    translation[13]=-position[1];

    translation[14]=(-position[2])-height;

    eTransposeMatrix(transpose,orientation);

    vectorMatrixMultiply(matrix,transpose,translation);

}

CarCinematicResult carOrbitCameraAdvance(CarOrbitCamera* camera)

{

    if(!camera||!camera->base.view)return CAR_CINEMATIC_INVALID_BINDING;

    const CarCinematicServices* services=camera->base.services;

    if(services->session&&services->session->state==6)return CAR_CINEMATIC_OK;

    const CarOrbitCameraSettings* settings=camera->context.settings;

    float matrix[16];

    const Chopper* chopper=camera->context.simulation?camera->context.simulation->chopper:NULL;

    if(camera->input.followChopper&&chopper&&((uint32_t)chopper->phase<3||chopper->phase==4)){

        float orientation[16];

        memcpy(orientation,chopper->frame,sizeof(orientation));

        orientation[12]=orientation[13]=orientation[14]=0.0f;

        orbitTargetTransform(matrix,chopper->position,orientation,settings->targetHeight);

    }else if(camera->input.selectedCar){

        const CarMovement* movement=carOrbitCameraSelectedMovement(camera);

        if(!movement||!movement->bindings.state)return CAR_CINEMATIC_INVALID_BINDING;

        const VehicleState* state=movement->bindings.state;

        orbitTargetTransform(matrix,state->centerPosition,state->orientation,settings->targetHeight);

        for(int axis=0;axis<4;++axis)matrix[12+axis]=matrix[12+axis]+settings->carTranslationBias[axis];

    }else{

        rotationMatrixIdentity(matrix);

        matrix[14]=-settings->neutralDistance;

    }

    if(camera->input.distanceFactor!=1.0f)

        camera->requestedDistance=camera->requestedDistance*camera->input.distanceFactor;

    camera->distance=camera->requestedDistance;

    if(camera->distance<camera->minimumDistance)camera->distance=camera->minimumDistance;

    if(camera->maximumDistance<camera->distance)camera->distance=camera->maximumDistance;

    if(camera->requestedDistance<camera->minimumDistance)camera->requestedDistance=camera->minimumDistance;

    if(camera->maximumDistance<camera->requestedDistance)camera->requestedDistance=camera->maximumDistance;

    camera->headingRate=orbitApproachRate(camera->headingRate,camera->input.angularRate[0],camera->acceleration);

    camera->pitchRate=orbitApproachRate(camera->pitchRate,camera->input.angularRate[1],camera->acceleration);

    camera->pitch=(uint16_t)(camera->pitch+(uint32_t)camera->pitchRate);

    if(camera->recenterPitch){

        if(camera->pitch<camera->centeredPitch){

            const int32_t difference=(int32_t)camera->centeredPitch-camera->pitch;

            camera->pitch=difference<81?camera->centeredPitch:(uint16_t)(camera->pitch+80u);

        }else{

            const int32_t difference=(int32_t)camera->pitch-camera->centeredPitch;

            camera->pitch=difference<81?camera->centeredPitch:(uint16_t)(camera->pitch-80u);

        }

    }

    camera->heading=(uint16_t)(camera->heading+(uint32_t)camera->headingRate);

    const uint16_t heading=settings->overridePose?settings->overrideHeading:camera->heading;

    const uint16_t pitch=settings->overridePose?settings->overridePitch:camera->pitch;

    const float distance=settings->overridePose?settings->overrideDistance:camera->distance;

    motionMatrixRotateYaw(matrix,matrix,heading);

    motionMatrixRotateY(matrix,matrix,pitch);

    const float offset[3]={distance,0.0f,0.0f};

    motionMatrixTranslate(matrix,matrix,offset);

    motionMatrixRotateX(matrix,matrix,16384);

    motionMatrixRotateY(matrix,matrix,49152);

    if(!services->suppressPublication){

        camera->base.view->current.fieldOfView=settings->fieldOfView;

        camera->base.view->current.focusDistance=camera->distance;

        camera->base.view->current.minimumVisibility=camera->distance;

        camera->base.view->current.maximumVisibility=8.0f;

    }

    carCinematicViewPublish(camera->base.services,camera->base.view,matrix,services->clock->realTimeElapsed);

    return CAR_CINEMATIC_OK;

}

CarCinematicBase* carOrbitCameraBase(CarOrbitCamera* camera)

{

    return camera?&camera->base:NULL;

}

CarPlayerOrbitCamera* carOrbitCameraInput(CarOrbitCamera* camera)

{

    return camera?&camera->input:NULL;

}

CarPlayerCameraResult carPlayerOrbitCameraActivateInput(CarPlayerOrbitCamera* camera)
{
    if (camera->inputEnabled) return camera->result;
    if (!camera->services || !camera->services->input)
        return camera->result = CAR_PLAYER_CAMERA_INVALID_BINDING;
    GameInputTarget target{};target.kind=GAME_INPUT_ORBIT_CAMERA;target.owner.orbitCamera=camera;
    for (int32_t event = 0x2E; event <= 0x36; ++event)
        if (gameInputRegister(camera->services->input, camera->device, event, target) != GAME_INPUT_OK)
            return camera->result = CAR_PLAYER_CAMERA_INPUT_CAPACITY;
    camera->inputEnabled = 1;
    return camera->result = CAR_PLAYER_CAMERA_OK;
}

void carPlayerOrbitCameraDeactivateInput(CarPlayerOrbitCamera* camera)
{
    if (!camera->inputEnabled) return;
    for (int32_t event = 0x2E; event <= 0x36; ++event)
        gameInputUnregister(camera->services->input, camera->device, event);
    camera->inputEnabled = 0;
}

void carPlayerOrbitCameraDispatchInput(CarPlayerOrbitCamera* camera,
    int32_t device, int32_t event, uint8_t value)
{
    (void)device;
    const int32_t rate = camera->fixedTargetMode ? 300 : 500;
    switch (event) {
    case 0x2E: camera->angularRate[0] = value ? -rate : 0; break;
    case 0x2F: camera->angularRate[0] = value ? rate : 0; break;
    case 0x30: camera->angularRate[1] = value ? -rate : 0; break;
    case 0x31: camera->angularRate[1] = value ? rate : 0; break;
    case 0x32: camera->distanceFactor = value ? 0.9900000095367432f : 1.0f; break;
    case 0x33: camera->distanceFactor = value ? 1.0099999904632568f : 1.0f; break;
    case 0x34:
        if (!camera->fixedTargetMode && value) {
            Car* next = camera->selectedCar->next;
            if (next == &camera->services->world->carList) next = next->next;
            camera->selectedCar = next;
        }
        break;
    case 0x35:
        if (!camera->fixedTargetMode && value) {
            Car* previous = camera->selectedCar->previous;
            if (previous == &camera->services->world->carList) previous = previous->previous;
            camera->selectedCar = previous;
        }
        break;
    case 0x36:
        if (value) camera->followChopper = !camera->followChopper;
        break;
    default: break;
    }
}

void carVelocityCameraSettingsInitialize(CarVelocityCameraSettings* settings)
{
    const CarVelocityCameraSettings defaults = {
        {2.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f},
        {0.10000000149011612f, 0.10000000149011612f, 0.10000000149011612f},
        {1.0f, 1.0f, 2.0f}, {6.0f, 0.0f, 0.5f},
        {-6.0f, -0.800000011920929f, -1.0f}, {1.0f, 1.7999999523162842f, 1.0f},
        {0.10000000149011612f, 0.10000000149011612f, 0.10000000149011612f},
        {1.0f, 1.0f, 2.0f}, {-6.0f, 0.0f, 3.0f}
    };
    *settings = defaults;
}

static void velocityConfigureCurves(CarVelocityCamera* camera)
{
    const CarVelocityCameraSettings* settings = camera->context.settings;
    tCubic3DSetThreshold(&camera->eye, settings->eyeThreshold);
    tCubic3DSetThreshold(&camera->target, settings->targetThreshold);
    tCubic3DSetDuration(&camera->eye, settings->eyeDuration);
    tCubic3DSetDuration(&camera->target, settings->targetDuration);
}

CarVelocityCamera* carVelocityCameraCreate(const CarVelocityCameraContext* context,
    CarCinematicResult* result)
{
    if (result) *result = CAR_CINEMATIC_INVALID_BINDING;
    if (!context || !context->services || !context->services->clock ||
        !context->movement || !context->movement->bindings.state ||
        !context->movement->bindings.state->position || !context->settings)
        return NULL;

    CarVelocityCamera* camera=static_cast<CarVelocityCamera*>(calloc(1,sizeof(*camera)));
    if (!camera) {
        if (result) *result = CAR_CINEMATIC_ALLOCATION_FAILURE;
        return NULL;
    }
    const CarCinematicResult status = carCinematicBaseInitialize(&camera->base,
        context->services, context->viewNumber, 17);
    if (status != CAR_CINEMATIC_OK) {
        free(camera);
        if (result) *result = status;
        return NULL;
    }
    carCinematicBindOwner(&camera->base,camera,&carCinematicVelocityMethods);
    camera->context = *context;
    camera->base.player = context->player;
    memcpy(camera->previousVelocity, context->movement->bindings.state->velocity,
        sizeof(camera->previousVelocity));
    for (int32_t axis = 0; axis < 3; ++axis) {
        tCubic1DInitialize(&camera->eye.axes[axis]);
        tCubic1DInitialize(&camera->target.axes[axis]);
    }
    velocityConfigureCurves(camera);
    carVelocityCameraUpdateOffsets(camera);
    carVelocityCameraRefreshTargets(camera);
    for (int32_t axis = 0; axis < 3; ++axis) {
        tCubic1D* curves[2] = {&camera->eye.axes[axis], &camera->target.axes[axis]};
        for (int32_t index = 0; index < 2; ++index) {
            tCubic1D* curve = curves[index];
            curve->value = curve->desiredValue;
            curve->derivative = curve->desiredDerivative;
            curve->mode = 0;
        }
    }
    if (result) *result = CAR_CINEMATIC_OK;
    return camera;
}

void carVelocityCameraDispose(CarVelocityCamera* camera)
{
    if (!camera) return;
    carCinematicBaseDispose(&camera->base);
    free(camera);
}

void carVelocityCameraRefreshTargets(CarVelocityCamera* camera)
{
    tCubic3DSetValDesired(&camera->eye, camera->eyeOffset);
    tCubic3DSetValDesired(&camera->target, camera->targetOffset);
}

static float velocityResponseOffset(float value, float directionScale, float gain, float offset)
{
    const uint16_t angle = vectorAtan2Angle(directionScale, value);
    const int32_t signedAngle = angle <= 32767 ? (int32_t)angle : (int32_t)angle - 65536;
    const float fraction = (float)signedAngle * 0.00006103515625f;
    const float displacement = gain * fraction;
    return offset + displacement;
}

void carVelocityCameraUpdateOffsets(CarVelocityCamera* camera)
{
    const VehicleState* state = camera->context.movement->bindings.state;
    const CarVelocityCameraSettings* settings = camera->context.settings;
    float transform[16], inverse[16], change[4], local[4];
    memcpy(transform, state->orientation, sizeof(transform));
    memcpy(transform + 12, state->position, 3 * sizeof(float));
    transform[15] = 1.0f;
    eTransposeMatrix(inverse, transform);
    for (int32_t axis = 0; axis < 4; ++axis)
        change[axis] = state->velocity[axis] - camera->previousVelocity[axis];
    vectorMatrixTransformPoint(local, inverse, change);
    for (int32_t axis = 0; axis < 3; ++axis) {
        camera->eyeOffset[axis] = velocityResponseOffset(local[axis], settings->eyeDirectionScale[axis],
            settings->eyeGain[axis], settings->eyeOffset[axis]);
        camera->targetOffset[axis] = velocityResponseOffset(local[axis], settings->targetDirectionScale[axis],
            settings->targetGain[axis], settings->targetOffset[axis]);
    }
}

CarCinematicResult carVelocityCameraAdvance(CarVelocityCamera* camera)
{
    if (!camera || !camera->base.view) return CAR_CINEMATIC_INVALID_BINDING;
    CarCinematicServices* services = camera->base.services;
    if (services->session && services->session->state == 6) return CAR_CINEMATIC_OK;
    velocityConfigureCurves(camera);
    carVelocityCameraUpdateOffsets(camera);
    carVelocityCameraRefreshTargets(camera);
    const VehicleState* state = camera->context.movement->bindings.state;
    memcpy(camera->previousVelocity, state->velocity, sizeof(camera->previousVelocity));
    const float elapsed = services->clock->worldTimeElapsed;
    for (int32_t axis = 0; axis < 3; ++axis)
        tCubic1DUpdate(&camera->eye.axes[axis], elapsed, 0.0f, 0.0f);
    for (int32_t axis = 0; axis < 3; ++axis)
        tCubic1DUpdate(&camera->target.axes[axis], elapsed, 0.0f, 0.0f);

    float eye[4] = {0}, target[4] = {0}, transform[16];
    for (int32_t axis = 0; axis < 3; ++axis) {
        eye[axis] = camera->eye.axes[axis].value;
        target[axis] = camera->target.axes[axis].value;
    }
    memcpy(transform, state->orientation, sizeof(transform));
    memcpy(transform + 12, state->position, 3 * sizeof(float));
    transform[15] = 1.0f;
    vectorMatrixTransformPoint(eye, transform, eye);
    vectorMatrixTransformPoint(target, transform, target);
    const float up[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    carCinematicLookAt(transform, eye, target, up);
    carCinematicViewPublish(services, camera->base.view, transform, services->clock->realTimeElapsed);
    return CAR_CINEMATIC_OK;
}

CarCinematicBase* carVelocityCameraBase(CarVelocityCamera* camera)
{
    return camera ? &camera->base : NULL;
}

struct CarDriveCamera {
    CarCinematicBase base;
    TrackRouteEditor* editor;
};

CarDriveCamera* carDriveCameraCreate(CarCinematicServices* services,int32_t viewNumber,
    TrackRouteEditor* editor,CarCinematicResult* result)
{
    if(result)*result=CAR_CINEMATIC_INVALID_BINDING;
    if(!services||!services->clock||!editor)return NULL;
    CarDriveCamera* camera=static_cast<CarDriveCamera*>(calloc(1,sizeof(*camera)));
    if(!camera){if(result)*result=CAR_CINEMATIC_ALLOCATION_FAILURE;return NULL;}
    CarCinematicResult status=carCinematicBaseInitialize(&camera->base,services,viewNumber,3);
    if(status!=CAR_CINEMATIC_OK){free(camera);if(result)*result=status;return NULL;}
    carCinematicBindOwner(&camera->base,camera,&carCinematicDriveMethods);
    camera->editor=editor;
    const TrackRouteEditorResult activated=trackRouteEditorActivate(editor,camera->base.view->current.eye);
    if(activated!=TRACK_ROUTE_EDITOR_OK){
        carCinematicBaseDispose(&camera->base);free(camera);
        if(result)*result=CAR_CINEMATIC_BACKEND_CAPACITY;
        return NULL;
    }
    if(result)*result=CAR_CINEMATIC_OK;
    return camera;
}

void carDriveCameraDispose(CarDriveCamera* camera)
{
    if(!camera)return;
    trackRouteEditorDeactivate(camera->editor);
    carCinematicBaseDispose(&camera->base);
    free(camera);
}

CarCinematicResult carDriveCameraAdvance(CarDriveCamera* camera)
{
    if(!camera)return CAR_CINEMATIC_INVALID_BINDING;
    float eye[4],direction[4];
    if(!trackRouteEditorGetCameraPosition(camera->editor,eye,direction))return CAR_CINEMATIC_OK;
    float target[4],matrix[16];
    for(int axis=0;axis<4;++axis)target[axis]=eye[axis]+direction[axis];
    const float up[4]={0,0,1,0};
    carCinematicLookAt(matrix,eye,target,up);
    carCinematicViewPublish(camera->base.services,camera->base.view,matrix,
        camera->base.services->clock->realTimeElapsed);
    return CAR_CINEMATIC_OK;
}

CarCinematicBase* carDriveCameraBase(CarDriveCamera* camera)
{
    return camera?&camera->base:NULL;
}

/* Camera path support. */
const CarCameraView* carCameraViewAt(const CarCameraCatalogue* catalogue,
                                     int32_t presentationLayout, int32_t index)
{
    const int32_t layout = presentationLayout == 3;
    if (index < catalogue->viewCount[layout]) return &catalogue->views[layout][index];
    return NULL;
}
