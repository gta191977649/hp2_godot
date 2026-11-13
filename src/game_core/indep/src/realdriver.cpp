#include "indep/inc/Pursuit.hpp"
#include "Chopper.hpp"
#include "indep/inc/Player.hpp"
#include "indep/inc/Car.hpp"
#include "bware/inc/bMath.hpp"
#include "RealDriver.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "indep/inc/Replay.hpp"
#include "indep/inc/Flags.hpp"

#include <limits.h>
#include <math.h>
#include <string.h>

// PAL13AEB8..13B080, original RealDriver::DoSnapshot. The byte inputs and
// filtered values belong to this existing live driver. Input registration
// itself is intentionally absent from the source snapshot.
void realDriverDoSnapshot(RealDriver* driver,ReplaySnapshot* snapshot)
{
    if (!snapshot)return;
    if (!driver || !driver->movement || !driver->movement->bindings.controls) {
        snapshot->result=REPLAY_SNAPSHOT_INVALID_BINDING;
        return;
    }
    replaySnapshotBeginSection(snapshot,driver,"RealDriver");
    replaySnapshotMoveData(snapshot,&driver->port,1,4,0);
    replaySnapshotMoveInteger(snapshot,&driver->forceBrake,1);
    replaySnapshotMoveInteger(snapshot,&driver->forceReverse,1);
    replaySnapshotMoveData(snapshot,&driver->countersteering,1,4,0);
    replaySnapshotMoveInteger(snapshot,&driver->gearInputMode,1);
    replaySnapshotMoveFloat(snapshot,&driver->handbrakeTime);
    for (auto& value:driver->steeringBytes)
        replaySnapshotMoveData(snapshot,&value,1,1,0);
    replaySnapshotMoveFloat(snapshot,&driver->steeringInput);
    for (auto& value:driver->throttleBytes)
        replaySnapshotMoveData(snapshot,&value,1,1,0);
    replaySnapshotMoveFloat(snapshot,&driver->throttleInput);
    for (auto& value:driver->brakeBytes)
        replaySnapshotMoveData(snapshot,&value,1,1,0);
    replaySnapshotMoveFloat(snapshot,&driver->brakeInput);
    for (auto& value:driver->alternateThrottleBytes)
        replaySnapshotMoveData(snapshot,&value,1,1,0);
    replaySnapshotMoveFloat(snapshot,&driver->alternateThrottle);
    for (auto& value:driver->handbrakeBytes)
        replaySnapshotMoveData(snapshot,&value,1,1,0);
    replaySnapshotMoveFloat(snapshot,&driver->handbrakeInput);
    replaySnapshotMoveInteger(snapshot,&driver->nitroStartedTicks,4);
    replaySnapshotMoveData(snapshot,&driver->reverseRequested,1,4,0);
    carDriverInputSnapshot(driver->movement,&driver->command,snapshot);
    replaySnapshotEndSection(snapshot);
}

void realDriverSetGearInputMode(RealDriver* driver,int32_t mode)
{
    if(mode==1){
        driver->gearInputMode=1;
        driver->movement->bindings.controls->gearRequest=driver->movement->bindings.state->gear;
    }else{
        driver->movement->bindings.controls->gearRequest=-1;
        driver->gearInputMode=mode==0?0:2;
    }
}

void realDriverSetHandbrakeTime(RealDriver* driver,float seconds)
{
    driver->handbrakeTime=seconds;
}

void realDriverClearInput(RealDriver* driver)
{
    driver->handbrakeInput=0.0f;
    driver->steeringBytes[0]=128;
    driver->steeringBytes[1]=128;
    for(int index=0;index<3;++index){
        driver->throttleBytes[index]=0;
        driver->brakeBytes[index]=0;
    }
    for(int index=0;index<2;++index){
        driver->alternateThrottleBytes[index]=0;
        driver->handbrakeBytes[index]=0;
    }
    driver->steeringInput=0.0f;
    driver->throttleInput=0.0f;
    driver->brakeInput=0.0f;
    driver->alternateThrottle=0.0f;
}

void realDriverSteeringInput(RealDriver* driver,int32_t event,uint8_t value)
{
    if(event==0x6f)driver->steeringBytes[0]=value;
    else if(event==0x70)driver->steeringBytes[1]=value;
    const uint8_t selected=driver->steeringBytes[0]==128?driver->steeringBytes[1]:driver->steeringBytes[0];
    driver->steeringInput=selected==255?-1.0f:(128.0f-(float)selected)*0.0078125f;
}

void realDriverSelectDefaultGear(RealDriver* driver)
{
    driver->movement->bindings.controls->gearRequest=driver->gearInputMode==1?2:-1;
}


void realDriverHornInput(RealDriver* driver,uint8_t pressed)
{
    if(pressed){
        realDriverStartHorn(driver);
        /* The removed call records a road-editor marker only. */
        if(driver->configuration->bodyCategory==1){
            // PAL13C1A4/13C1A8 reads the canonical RaceSettings+274. The
            // Car frame already borrows that owner, including after controller
            // replacement; no independently nullable pursuit flag exists.
            if(driver->frame->environment.settings->specialPursuit){
                if(!driver->pursuitManager){driver->result=CAR_DRIVER_MISSING_PURSUIT;return;}
                if(pursuitManagerSignalHorn(driver->pursuitManager)!=WORLD_SERVICES_OK)
                    driver->result=CAR_DRIVER_MOVEMENT_FAILURE;
            }else{
                CarFrameState* state=carFrameState(driver->frame);
                if(state->signalMode){state->signalMode=0;state->signalEnabled=0;}
                else{state->signalEnabled=1;state->signalMode=2;}
            }
        }else carFrameState(driver->frame)->blinkVisible=1;
    }else{
        realDriverStopHorn(driver);
        if(driver->configuration->bodyCategory!=1)carFrameState(driver->frame)->blinkVisible=0;
    }
}

void realDriverDispatchInput(RealDriver* driver,int32_t device,int32_t event,uint8_t value)
{
    switch(event){
    case 0x6f:case 0x70:realDriverSteeringInput(driver,event,value);break;
    case 0x71:case 0x72:case 0x73:realDriverThrottleInput(driver,event,value);break;
    case 0x74:case 0x75:case 0x76:realDriverBrakeInput(driver,event,value);break;
    case 0x77:case 0x78:realDriverAlternateThrottleInput(driver,event,value);break;
    case 0x79:case 0x7a:realDriverHandbrakeInput(driver,event,value);break;
    case 0x7b:realDriverShiftUpInput(driver,device,event,value);break;
    case 0x7c:realDriverShiftDownInput(driver,device,event,value);break;
    case 0x7d:realDriverRecoverInput(driver,value);break;
    case 0x7e:realDriverHornInput(driver,value);break;
    case 0x7f:realDriverNitroInput(driver,value);break;
    default:break;
    }
}

void realDriverShiftUpInput(RealDriver* driver,int32_t device,int32_t event,uint8_t pressed)
{
    if(!pressed)return;
    CarControls* controls=driver->movement->bindings.controls;
    if(driver->gearInputMode==1){
        if(controls->gearRequest!=0&&controls->gearRequest<driver->configuration->forwardGearCount+1)
            ++controls->gearRequest;
    }else if(driver->gearInputMode==2){
        if(driver->movement->mode==4)return;
        if(controls->gearRequest==-2){
            controls->shiftTarget=(int32_t)((uint32_t)controls->shiftTarget+1u);
        }else{
            controls->gearRequest=-2;
            const int32_t gear=driver->movement->bindings.state->gear;
            controls->shiftTarget=gear<driver->configuration->forwardGearCount+1?gear+1:gear;
        }
    }else if(driver->player){
        carFramePlayerRouteInput(driver->player,device,event,pressed);
    }
}

void realDriverShiftDownInput(RealDriver* driver,int32_t device,int32_t event,uint8_t pressed)
{
    if(!pressed)return;
    CarControls* controls=driver->movement->bindings.controls;
    if(driver->gearInputMode==1){
        if(controls->gearRequest!=0&&controls->gearRequest>=2)--controls->gearRequest;
    }else if(driver->gearInputMode==2){
        if(driver->movement->mode==4)return;
        if(controls->gearRequest==-2){
            controls->shiftTarget=(int32_t)((uint32_t)controls->shiftTarget-1u);
        }else{
            controls->gearRequest=-2;
            const int32_t gear=driver->movement->bindings.state->gear;
            controls->shiftTarget=gear<3?gear:gear-1;
        }
    }else if(driver->player){
        carFramePlayerRouteInput(driver->player,device,event,pressed);
    }
}

void realDriverRecoverInput(RealDriver* driver,uint8_t pressed)
{
    if(driver->forceBrake||!pressed||!driver->player||
       !carFramePlayerCrashEligible(driver->player))return;
    realDriverRadialBlur(driver,REAL_DRIVER_RADIAL_BLUR_STOP,0.0f);
    CarMovement* movement=driver->movement;
    const VehicleState* vehicle=movement->bindings.state;
    const float first=vehicle->orientation[0]*movement->routeLocation.direction[0];
    const float second=vehicle->orientation[1]*movement->routeLocation.direction[1];
    float speed=vehicle->forwardSpeed<0.0f?0.0f:vehicle->forwardSpeed;
    const float dot=first+second;
    speed=speed*(dot<0.0f?0.0f:dot);
    if(15.0f<speed)speed=15.0f;
    float direction=1.0f;
    if(driver->pursuitManager&&driver->configuration->bodyCategory==1){
        if(!driver->pursuitManager->collision){driver->result=CAR_DRIVER_MISSING_PURSUIT;return;}
        direction=driverPursuitRecoveryDirection(driver->pursuitManager->collision);
    }
    const CarDriver* controller=(const CarDriver*)movement->bindings.owner->controller;
    const CarRecoveryGroup* group=carDriverCurrentGroup(controller,1);
    const CarRecoveryPose* pose=carDriverCurrentPose(controller,1);
    carMovementRecover(movement,group,pose,speed,direction,1,1);
    carFramePlayerOnRecovered(driver->player);
    if(movement->result!=CAR_MOVEMENT_OK)driver->result=CAR_DRIVER_MOVEMENT_FAILURE;
}

void realDriverNitroInput(RealDriver* driver,uint8_t pressed)
{
    // PAL13C25C/13C260 reads the same live RaceSettings+274 as the horn.
    // The explicit PC option admits human racers and playable traffic to
    // this same request, cooldown and replay guard. Their actual physics,
    // shared Nitro cues and FX consume it; police keep the pursuit rule.
    if(!pressed)return;
    const bool pursuitNitro=driver->configuration->bodyCategory==1&&
        driver->frame->environment.settings->specialPursuit;
    const bool racerNitro=hp2::applicationNativeGameplay.racerNitro&&
        (driver->configuration->bodyCategory==0||driver->configuration->bodyCategory==2)&&driver->player;
    if((pursuitNitro||racerNitro)&&realDriverNitroReady(driver)){
        driver->movement->bindings.controls->nitroRequest=1;
        driver->nitroStartedTicks=driver->movement->bindings.environment->clock->worldTimer;
    }
}

void realDriverThrottleInput(RealDriver* driver,int32_t event,uint8_t value)
{
    if(event==0x71)driver->throttleBytes[0]=value;
    else if(event==0x72){
        // PAL13B9A8 queries this driver's current joystick type. The same
        // source port cell is updated by hotplug and restored by replay.
        const uint32_t amplified=joystickHasStiffThrottle(driver->inputRouter,driver->port)?(uint32_t)value*4u:value;
        driver->throttleBytes[1]=(uint8_t)(amplified<256u?amplified:255u);
    }else if(event==0x73){
        float amplified=(float)value*1.0700000524520874f;
        if(255.0f<amplified)amplified=255.0f;
        driver->throttleBytes[2]=(uint8_t)convertFloatToInt32(amplified);
    }
    const uint8_t secondary=driver->throttleBytes[1]<driver->throttleBytes[2]?
        driver->throttleBytes[2]:driver->throttleBytes[1];
    const uint8_t selected=driver->throttleBytes[0]?driver->throttleBytes[0]:secondary;
    driver->throttleInput=(float)selected*0.003921568859368563f;
}

void realDriverAlternateThrottleInput(RealDriver* driver,int32_t event,uint8_t value)
{
    if(event==0x77)driver->alternateThrottleBytes[0]=value;
    else if(event==0x78){
        const uint32_t amplified=joystickHasStiffThrottle(driver->inputRouter,driver->port)?(uint32_t)value*4u:value;
        driver->alternateThrottleBytes[1]=(uint8_t)(amplified<256u?amplified:255u);
    }
    const uint8_t selected=driver->alternateThrottleBytes[0]?
        driver->alternateThrottleBytes[0]:driver->alternateThrottleBytes[1];
    driver->alternateThrottle=(float)selected*0.003921568859368563f;
}

void realDriverBrakeInput(RealDriver* driver,int32_t event,uint8_t value)
{
    if(0x74<=event&&event<=0x76)driver->brakeBytes[event-0x74]=value;
    const uint8_t secondary=driver->brakeBytes[1]<driver->brakeBytes[2]?
        driver->brakeBytes[2]:driver->brakeBytes[1];
    const uint8_t selected=driver->brakeBytes[0]?driver->brakeBytes[0]:secondary;
    driver->brakeInput=(float)selected*0.003921568859368563f;
}

void realDriverHandbrakeInput(RealDriver* driver,int32_t event,uint8_t value)
{
    if(event==0x79)driver->handbrakeBytes[0]=value;
    else if(event==0x7a)driver->handbrakeBytes[1]=(uint8_t)((uint32_t)value*2u);
    const uint8_t selected=driver->handbrakeBytes[0]?driver->handbrakeBytes[0]:driver->handbrakeBytes[1];
    driver->handbrakeInput=(float)selected*0.003921568859368563f;
}


void realDriverRadialBlur(RealDriver* driver,RealDriverRadialBlurOperation operation,float amount)
{
    const CarFramePlayerServices* services=driver->player&&driver->player->registry?
        driver->player->registry->services:NULL;
    if(!services||!services->radialBlur){driver->result=CAR_DRIVER_INVALID_BINDING;return;}
    ScreenRadialBlur* radialBlur=services->radialBlur;
    const int32_t seat=driver->player->movement.seat;
    switch(operation){
    case REAL_DRIVER_RADIAL_BLUR_START:screenRadialBlurStart(radialBlur,seat);break;
    case REAL_DRIVER_RADIAL_BLUR_AMOUNTS:screenRadialBlurSetAmounts(radialBlur,seat,amount,0.8f);break;
    case REAL_DRIVER_RADIAL_BLUR_STOP:screenRadialBlurStop(radialBlur,seat);break;
    }
}


CarDriverResult realDriverInitialize(RealDriver* driver,CarMovement* movement,const RealDriverBindings* bindings)
{
    if(!driver||!movement||!movement->bindings.controls||!bindings||
       !bindings->configuration||!bindings->inputRouter||!bindings->tuning)
        return CAR_DRIVER_INVALID_BINDING;
    memset(driver,0,sizeof(*driver));
    driver->movement=movement;
    driver->configuration=bindings->configuration;
    driver->tuning=bindings->tuning;
    driver->player=bindings->player;
    driver->sound=bindings->sound;
    driver->chopper=bindings->chopper;
    driver->replay=bindings->replay;
    driver->frame=bindings->frame;
    driver->inputRouter=bindings->inputRouter;
    driver->pursuitManager=bindings->pursuitManager;
    driver->port=-1;
    driver->forceBrake=1;
    driverCommandInitialize(&driver->command);
    CarControls* controls=movement->bindings.controls;
    controls->steeringAngle=0;
    controls->throttle=0.0f;
    controls->brake=0.0f;
    controls->handbrake=0.0f;
    controls->gearRequest=-1;
    controls->nitroRequest=0;
    realDriverClearInput(driver);
    return CAR_DRIVER_OK;
}

void realDriverDispose(RealDriver* driver)
{
    if(driver->player)realDriverRadialBlur(driver,REAL_DRIVER_RADIAL_BLUR_STOP,0.0f);
    realDriverDeactivateInput(driver);
}

void realDriverUnlock(RealDriver* driver)
{
    driver->forceBrake=0;
    realDriverSelectDefaultGear(driver);
}

CarDriverResult realDriverStopHorn(RealDriver* driver)
{
    if(driver->configuration->bodyCategory!=1&&driver->sound&&
       !carSoundStopHorn(driver->sound))driver->result=CAR_DRIVER_CAPACITY;
    return driver->result;
}

CarDriverResult realDriverActivateInput(RealDriver* driver)
{
    if(driver->inputEnabled)return driver->result;
    GameInputRouter* router=driver->inputRouter;
    if(!router||!router->registrations)return driver->result=CAR_DRIVER_MISSING_INPUT;
    if(router->capacity-router->count<17||UINT64_MAX-router->nextOrder<17)
        return driver->result=CAR_DRIVER_CAPACITY;
    GameInputTarget target;
    target.kind=GAME_INPUT_REAL_DRIVER;target.owner.real=driver;
    for(int32_t event=0x6f;event<=0x7f;++event)
        if(gameInputRegister(router,driver->port,event,target)!=GAME_INPUT_OK)
            return driver->result=CAR_DRIVER_CAPACITY;
    driver->inputEnabled=1;
    return driver->result;
}

CarDriverResult realDriverDeactivateInput(RealDriver* driver)
{
    if(!driver->inputEnabled)return driver->result;
    GameInputRouter* router=driver->inputRouter;
    if(!router)return driver->result=CAR_DRIVER_MISSING_INPUT;
    for(int32_t event=0x6f;event<=0x7f;++event)gameInputUnregister(router,driver->port,event);
    realDriverStopHorn(driver);
    driver->inputEnabled=0;
    return driver->result;
}


CarDriverResult realDriverStartHorn(RealDriver* driver)
{
    if(driver->configuration->bodyCategory!=1&&driver->sound&&
       !carSoundStartHorn(driver->sound))driver->result=CAR_DRIVER_CAPACITY;
    chopperNotifyHorn(driver->chopper);
    return driver->result;
}


static const float wheelSteeringRange[] = {45.0f,15.0f,11.0f,9.0f,8.0f,7.0f,6.0f,6.0f,6.0f,6.0f};
static const float padSteeringRange[] = {45.0f,13.0f,8.0f,5.5f,4.2f,3.6f,3.1f,2.9f,2.9f,2.9f};
static const float steeringRate[] = {1.0f,1.0f,0.92f,0.78f,0.64f,0.54f,0.48f,0.42f,0.38f,0.34f};
static const float steeringResponse[] = {0.0f,0.1f,0.24f,0.53f,0.79f,1.0f};
static const float pedalResponse[] = {1.0f,0.4f,0.2f,0.1f,0.0f,-0.1f,-0.2f,-0.4f,-1.0f};
static const float countersteerScale[] = {0.8f,0.9f,0.92f,0.95f,1.0f};
static const float linearResponse[] = {0.0f,0.2f,0.4f,0.6f,0.8f,1.0f};

void realDriverTuningInitialize(RealDriverTuning* tuning)
{
    sampledCurveInitialize(&tuning->wheelSteeringRange,wheelSteeringRange,10,0.0f,160.0f);
    sampledCurveInitialize(&tuning->padSteeringRange,padSteeringRange,10,0.0f,160.0f);
    sampledCurveInitialize(&tuning->steeringRate,steeringRate,10,0.0f,160.0f);
    sampledCurveInitialize(&tuning->steeringResponse,steeringResponse,6,0.0f,1.0f);
    sampledCurveInitialize(&tuning->pedalResponse,pedalResponse,9,0.0f,256.0f);
    sampledCurveInitialize(&tuning->countersteerScale,countersteerScale,5,0.0f,1.0f);
    sampledCurveInitialize(&tuning->linearResponse,linearResponse,6,0.0f,1.0f);
    tuning->countersteerWeight=0.95f;
}









void realDriverUpdatePedals(RealDriver* driver,float elapsed)
{
    CarControls* controls=driver->movement->bindings.controls;
    VehicleState* vehicle=driver->movement->bindings.state;
    const float remaining=driver->handbrakeTime-elapsed;
    driver->handbrakeTime=remaining;
    float forward=0.0f;
    if(0.0f<remaining&&8.940545082092285f<fabsf(vehicle->forwardSpeed)){
        controls->handbrake=1.0f;
    }else{
        driver->handbrakeTime=0.0f;
        controls->handbrake=driver->handbrakeInput;
        forward=driver->throttleInput;
    }
    const float reverse=driver->brakeInput;
    if(0.6000000238418579f<reverse)driver->reverseRequested=1;
    else if(-2.0f<vehicle->forwardSpeed)driver->reverseRequested=0;
    float brake;
    float throttle;
    float reverseThrottle;
    if(0.5f<vehicle->forwardSpeed){
        brake=reverse;throttle=forward;reverseThrottle=0.0f;
    }else if(vehicle->forwardSpeed<-30.0f||
             (driver->reverseRequested&&vehicle->forwardSpeed<-2.0f)){
        brake=forward;throttle=0.0f;reverseThrottle=reverse;
    }else{
        brake=0.0f;throttle=forward;reverseThrottle=reverse;
    }
    reverseThrottle=reverseThrottle+reverseThrottle;
    if(1.0f<reverseThrottle)reverseThrottle=1.0f;
    controls->brake=brake;
    if(0.5f<controls->handbrake&&
       (((uint32_t)driver->movement->bindings.environment->clock->worldLoopCounter+1u)&7u)==0u&&
       10.0f<vehicle->speed&&driver->replay)
        replayManagerAddHighlight(driver->replay,1.0f);
    if(throttle+0.20000000298023224f<reverseThrottle){
        controls->throttle=reverseThrottle;
        controls->gearRequest=0;
    }else{
        controls->throttle=throttle;
        if(controls->gearRequest==0)realDriverSelectDefaultGear(driver);
    }
    vehicle->effectiveThrottle=controls->throttle;
}

void realDriverUpdateSteering(RealDriver* driver,float elapsed)
{
    CarControls* controls=driver->movement->bindings.controls;
    const VehicleState* vehicle=driver->movement->bindings.state;
    RealDriverTuning* tuning=driver->tuning;
    float current=(float)controls->steeringAngle*0.0054931640625f;
    if(180.0f<=current)current=current-360.0f;
    const float heading=vehicleStateMovingHeadingDifference(vehicle)*57.2957763671875f;
    const float absoluteHeading=fabsf(heading);
    const float xx=vehicle->velocity[0]*vehicle->velocity[0];
    const float yy=vehicle->velocity[1]*vehicle->velocity[1];
    const float zz=vehicle->velocity[2]*vehicle->velocity[2];
    const float velocity=sqrtf((xx+yy)+zz);
    float range=sampledCurveEvaluate(joystickIsTypeWheel(driver->inputRouter,driver->port)?
        &tuning->wheelSteeringRange:&tuning->padSteeringRange,vehicle->forwardSpeed);
    if(driver->countersteering){
        if(absoluteHeading<5.0f||velocity<13.410818099975586f)driver->countersteering=0;
    }else if(7.0f<absoluteHeading&&13.410818099975586f<=velocity){
        driver->countersteering=1;
    }
    if(heading*driver->steeringInput<0.0f&&
       (driver->countersteering||range<absoluteHeading)){
        tuning->countersteerWeight=joystickIsTypeWheel(driver->inputRouter,driver->port)?1.0f:
            sampledCurveEvaluate(&tuning->countersteerScale,fabsf(driver->steeringInput));
        range=absoluteHeading*tuning->countersteerWeight;
        const float limit=tuning->countersteerWeight*45.0f;
        if(limit<range)range=limit;
    }
    current=finiteRatio(current,range);
    float normalized=sampledCurveInverse(&tuning->steeringResponse,fabsf(current));
    if(current<0.0f)normalized=-normalized;
    float change=driver->steeringInput-normalized;
    float rate=9.0f*sampledCurveEvaluate(&tuning->steeringRate,vehicle->forwardSpeed);
    if((0.0f<normalized&&driver->steeringInput<normalized)||
       (normalized<0.0f&&normalized<driver->steeringInput))rate=rate*2.0f;
    rate=rate*elapsed;
    if(rate<0.17299999296665192f)rate=0.17299999296665192f;
    if(rate<change)change=rate;
    else if(change<-rate)change=-rate;
    normalized=normalized+change;
    float angle=sampledCurveEvaluate(&tuning->steeringResponse,fabsf(normalized))*range;
    if(normalized<0.0f)angle=-angle;
    angle=angle*driver->configuration->steeringRangeDegrees;
    const int32_t turns=convertFloatToInt32(angle*65536.0f)/360;
    controls->steeringAngle=(uint16_t)(uint32_t)turns;
}


static int32_t nitroTicksValid(int32_t ticks)
{
    return ticks!=0&&ticks!=INT_MAX;
}

static float nitroElapsed(const RealDriver* driver)
{
    const int32_t now=driver->movement->bindings.environment->clock->worldTimer;
    const int32_t elapsed=(int32_t)((uint32_t)now-(uint32_t)driver->nitroStartedTicks);
    return (float)elapsed*0.0002500000118743628f;
}

int32_t realDriverNitroPreviouslyActive(const RealDriver* driver)
{
    const float elapsed=nitroElapsed(driver)-driver->movement->bindings.environment->clock->worldTimeElapsed;
    return nitroTicksValid(driver->nitroStartedTicks)&&0.0f<=elapsed&&elapsed<4.0f;
}

int32_t realDriverNitroActive(const RealDriver* driver)
{
    return nitroTicksValid(driver->nitroStartedTicks)&&nitroElapsed(driver)<4.0f;
}

int32_t realDriverNitroReady(const RealDriver* driver)
{
    if(driver->player&&carFramePlayerReplayActive(driver->player))return 0;
    return !nitroTicksValid(driver->nitroStartedTicks)||4.0f+0.0f<=nitroElapsed(driver);
}

void realDriverReadInput(RealDriver* driver,float elapsed)
{
    /* Track recording belongs to the original editor; the empty update hook
     * has no native operation. Device decoding remains a host boundary. */
    if(!joystickIsPluggedIn(driver->inputRouter,driver->port))realDriverClearInput(driver);
    realDriverUpdatePedals(driver,elapsed);
    realDriverUpdateSteering(driver,elapsed);
    if(driver->forceReverse){driver->throttleInput=0.0f;driver->brakeInput=1.0f;}
    if(driver->forceBrake)driver->movement->bindings.controls->brake=1.0f;
    if(!driver->player)return;
    const int32_t active=realDriverNitroActive(driver);
    const int32_t previous=realDriverNitroPreviouslyActive(driver);
    if(active&&!previous){
        realDriverRadialBlur(driver,REAL_DRIVER_RADIAL_BLUR_START,0.0f);
        realDriverRadialBlur(driver,REAL_DRIVER_RADIAL_BLUR_AMOUNTS,0.0f);
    }else if(!active&&previous){
        realDriverRadialBlur(driver,REAL_DRIVER_RADIAL_BLUR_STOP,0.0f);
    }
}
