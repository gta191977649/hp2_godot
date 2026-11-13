#include "indep/inc/PsychoDriver.hpp"
#include "CarSounds.hpp"
#include "indep/inc/Collision.hpp"
#include "indep/inc/AICarController.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Table.hpp"
#include "bware/inc/bMath.hpp"

#include <string.h>

void aiControlsSetCommand(AIControls* controls,const DriverCommand* command)
{
    if (!command || command->count<0 || command->count>DRIVER_COMMAND_CAPACITY) return;
    driverCommandCopy(&controls->command,command);driverCommandResample(&controls->command);
}

void aiControlsSetTargetPoint(AIControls* controls,const float point[2]) {controls->targetPoint[0]=point[0];controls->targetPoint[1]=point[1];}
const float* aiControlsTargetPoint(const AIControls* controls) {return controls->targetPoint;}
void aiControlsSetLookAheadScale(AIControls* controls,float scale) {controls->lookAheadScale=scale;}
float aiControlsTargetSpeed(const AIControls* controls) {return controls->targetSpeed;}
float aiControlsTargetDuration(const AIControls* controls) {return controls->targetDuration;}
void aiControlsFullThrottle(AIControls* controls) {CarControls* output=controls->bindings.movement->bindings.controls;output->throttle=1;output->brake=0;output->handbrake=0;}
void aiControlsNeutralGear(AIControls* controls) {controls->bindings.movement->bindings.controls->gearRequest=-1;}

#include "indep/inc/Replay.hpp"
// Full final PAL139980..139A64. In particular, the final speed tolerance,
// handbrake timer and reverse hysteresis differ from the earlier beta body.
void aiControlsSnapshot(AIControls* controls,ReplaySnapshot* snapshot)
{
    replaySnapshotBeginSection(snapshot,nullptr,"PsychoDriver");
    replaySnapshotMoveFloat(snapshot,&controls->lookAheadScale);
    replaySnapshotMoveFloat(snapshot,&controls->targetSpeed);
    replaySnapshotMoveFloat(snapshot,&controls->targetDuration);
    replaySnapshotMoveFloat(snapshot,&controls->speedTolerance);
    replaySnapshotMoveData(snapshot,controls->targetPoint,8,-1,0);
    replaySnapshotMoveData(snapshot,&controls->enabled,1,4,0);
    replaySnapshotMoveFloat(snapshot,&controls->forcedHandbrakeTime);
    replaySnapshotMoveData(snapshot,&controls->reverseHysteresis,1,4,0);
    driverFilterSnapshot(&controls->headingError,snapshot);
    driverFilterSnapshot(&controls->velocityHeadingError,snapshot);
    driverFilterSnapshot(&controls->speedError,snapshot);
    carDriverInputSnapshot(controls->bindings.movement,&controls->command,snapshot);
    replaySnapshotEndSection(snapshot);
}


static const float proportionalSamples[10]={.275f,.204f,.148f,.115f,.09f,.074f,.057f,.049f,.043f,.04f};
static const float derivativeSamples[10]={.156f,.123f,.107f,.09f,.082f,.076f,.071f,.067f,.064f,.062f};
static const float integralSamples[10]={.21f,.244f,.267f,.29f,.305f,.321f,.328f,.336f,.341f,.344f};
static const float lookAheadSamples[10]={15,18,23.3f,28,34.8f,40.3f,45.8f,49.4f,53.1f,55.9f};
static const float steeringLimitSamples[10]={45,30,20,12,8,6,4.5f,4.2f,4.1f,4};
static const float steeringRateSamples[10]={4,2,1,.5f,.27f,.14f,.1f,.1f,.1f,.1f};
static const SampledCurve proportionalCurve={proportionalSamples,10,0,160,.05625f};
static const SampledCurve derivativeCurve={derivativeSamples,10,0,160,.05625f};
static const SampledCurve integralCurve={integralSamples,10,0,160,.05625f};
static const SampledCurve lookAheadCurve={lookAheadSamples,10,0,160,.05625f};
static const SampledCurve steeringLimitCurve={steeringLimitSamples,10,0,160,.05625f};
static const SampledCurve steeringRateCurve={steeringRateSamples,10,0,160,.05625f};
static const float handlingProportional[6]={.9f,1,.8f,1,.5f,0};
static const float handlingIntegral[6]={.2f,0,.1f,0,0,0};
static const float handlingDerivative[6]={.3f,.08f,.18f,0,.5f,0};
static const float handlingLookAhead[6]={.55f,.4f,.35f,.3f,1,0};

static float clamp(float value,float low,float high)
{
    return minimumFinite(maximumFinite(value,low),high);
}

CarDriverResult aiControlsInitialize(AIControls* controls,const AIControlsBindings* bindings)
{
    if (!controls || !bindings || !bindings->movement || !bindings->vehicle || !bindings->clock ||
        !bindings->movement->bindings.state || !bindings->movement->bindings.controls)
        return CAR_DRIVER_INVALID_BINDING;
    memset(controls,0,sizeof(*controls));controls->bindings=*bindings;
    driverCommandInitialize(&controls->command);
    CarControls* output=bindings->movement->bindings.controls;
    output->steeringAngle=0;output->throttle=0;output->brake=0;output->handbrake=0;
    output->gearRequest=1;output->nitroRequest=0;
    controls->lookAheadScale=1;controls->enabled=1;controls->reverseHysteresis=1;
    if (!driverFilterInitialize(&controls->headingError,15,5) ||
        !driverFilterInitialize(&controls->velocityHeadingError,15,5) ||
        !driverFilterInitialize(&controls->speedError,6,3)) {
        aiControlsDispose(controls);controls->result=CAR_DRIVER_ALLOCATION_FAILURE;
        return controls->result;
    }
    aiControlsClearAngleFilters(controls);aiControlsClearSpeedFilter(controls);
    return controls->result=CAR_DRIVER_OK;
}

void aiControlsDispose(AIControls* controls)
{
    driverFilterDispose(&controls->headingError);driverFilterDispose(&controls->velocityHeadingError);
    driverFilterDispose(&controls->speedError);aiControlsStopHorn(controls);
}

float aiControlsWrappedAngleRadians(uint16_t angle)
{
    float radians=(float)angle*0.000095873801910784f;
    if (radians>3.141590118408203f) radians-=6.283180236816406f;
    else if (radians < -3.141590118408203f) radians+=6.283180236816406f;
    return radians;
}

uint16_t aiControlsMeasureAngles(const AIControls* controls,float* headingError,float* velocityError)
{
    const VehicleState* state=controls->bindings.movement->bindings.state;
    const uint16_t target=vectorAtan2Angle(controls->targetPoint[0]-state->position[0],
                                         controls->targetPoint[1]-state->position[1]);
    *headingError=aiControlsWrappedAngleRadians((uint16_t)(state->heading-target));
    *velocityError=state->forwardSpeed>=1 ? aiControlsWrappedAngleRadians((uint16_t)(
        vectorAtan2Angle(state->velocity[0],state->velocity[1])-target)) : *headingError;
    return target;
}

void aiControlsClearAngleFilters(AIControls* controls)
{
    driverFilterReset(&controls->headingError,0);driverFilterReset(&controls->velocityHeadingError,0);
}

void aiControlsResetAngleFilters(AIControls* controls)
{
    float heading,velocity;aiControlsMeasureAngles(controls,&heading,&velocity);
    driverFilterReset(&controls->headingError,heading);driverFilterReset(&controls->velocityHeadingError,velocity);
}

void aiControlsClearSpeedFilter(AIControls* controls)
{
    driverFilterReset(&controls->speedError,0);
}

void aiControlsResetSpeedFilter(AIControls* controls)
{
    driverFilterReset(&controls->speedError,fabsf(controls->targetSpeed)-controls->bindings.movement->bindings.state->speed);
}

int32_t aiControlsHandlingMode(const AIControls* controls)
{
    const CarMovement* movement=controls->bindings.movement;
    switch (movement->mode) {
    case 1:return 1;
    case 4:return movement->bindings.ownerMotion->inputCategory==3 ? 4:0;
    case 5:return 2;
    case 6:return 0;
    default:return 3;
    }
}

void aiControlsUpdateTargetPoint(AIControls* controls)
{
    const VehicleState* state=controls->bindings.movement->bindings.state;
    const int32_t handling=aiControlsHandlingMode(controls);
    float distance;
    if (handling==3 || handling==4)
        distance=maximumFinite(15,state->forwardSpeed*handlingLookAhead[handling]);
    else {
        distance=sampledCurveEvaluate(&lookAheadCurve,state->forwardSpeed);
        if (state->pursuitMode==1) distance*=1.2f;
    }
    float point[4];
    if (controls->command.count<2) {
        if (state->forwardSpeed<5) driverPointAlongHeading(point,state,distance);
        else driverPointAlongVelocity(point,state,distance);
    } else driverCommandPointAtDistance(&controls->command,state,controls->bindings.clock->worldTimer,distance,point);
    controls->targetPoint[0]=point[0];controls->targetPoint[1]=point[1];
}

void aiControlsComputePedals(const AIControls* controls,float* throttle,float* brake)
{
    const VehicleState* state=controls->bindings.movement->bindings.state;
    if (fabsf(fabsf(controls->targetSpeed)-state->speed)<=controls->speedTolerance) {
        *throttle=0;*brake=0;return;
    }
    const DriverFilter* filter=&controls->speedError;
    const float derivative=clamp(filter->derivative.value,-2,2);
    const float request=(filter->current*.1f+derivative*.01f)+filter->integral.sum*.02f;
    const CarControls* output=controls->bindings.movement->bindings.controls;
    const float pedal=clamp((output->throttle-output->brake)+clamp(request,-.25f,.25f),-.94f,1);
    if (pedal<0) {*throttle=0;*brake=-pedal;}
    else {*throttle=pedal;*brake=0;}
}

void aiControlsUpdate(AIControls* controls,float elapsed)
{
    CarMovement* movement=controls->bindings.movement;
    VehicleState* state=movement->bindings.state;
    CarControls* output=movement->bindings.controls;
    float currentSteering=(float)output->steeringAngle*.0054931640625f;
    if (currentSteering>180) currentSteering-=360;
    const float interval=maximumFinite(elapsed,.0001f);
    const float signedSpeed=state->forwardSpeed<0 ? -state->speed:state->speed;
    aiControlsUpdateTargetPoint(controls);
    const int32_t frontContact=state->wheelContact[0] || state->wheelContact[1];
    const int32_t rearContact=state->wheelContact[2] || state->wheelContact[3];
    const int32_t handling=aiControlsHandlingMode(controls);
    const int32_t limitedHandling=handling==3 || handling==4;
    float headingError,velocityError;aiControlsMeasureAngles(controls,&headingError,&velocityError);
    int32_t overshoot=0;
    float steering;
    if (frontContact) {
        driverFilterUpdate(&controls->headingError,headingError,elapsed);
        driverFilterUpdate(&controls->velocityHeadingError,velocityError,elapsed);
        const CollisionShape* shape=movement->bindings.ownerMotion->collision->shape;
        for (int32_t index=0;index<shape->contactCount;++index) {
            const CollisionImpact* impact=collisionShapeContactAt(movement->collisionSystem,shape,index);
            if (collisionImpactIsCarPair(impact)) aiControlsResetAngleFilters(controls);
        }
        overshoot=headingError*velocityError<0 ||
            (fabsf(velocityError-headingError)>.0009587380336597562f && fabsf(headingError)>2*fabsf(velocityError));
        const DriverFilter* filter=overshoot || state->forwardSpeed<5 ? &controls->headingError:&controls->velocityHeadingError;
        const float derivative=clamp(filter->derivative.value,-.5f,.5f);
        float p,d,i;
        if (limitedHandling) {p=handlingProportional[handling];d=handlingDerivative[handling];i=handlingIntegral[handling];}
        else {
            p=sampledCurveEvaluate(&proportionalCurve,signedSpeed);
            d=sampledCurveEvaluate(&derivativeCurve,signedSpeed);
            i=sampledCurveEvaluate(&integralCurve,signedSpeed);
            if (state->pursuitMode==1) d*=1.2f;else i*=.5f;
        }
        steering=(-filter->current*p-derivative*d)-filter->integral.sum*i;
    } else steering=-vehicleStateVelocityHeadingDifference(state);
    steering=clamp(steering,-.7853982448577881f,.7853982448577881f);
    float speedError=fabsf(controls->targetSpeed)-state->speed;
    if (fabsf(controls->targetDuration)>.001f) speedError=(speedError/controls->targetDuration)*.3f;
    driverFilterUpdate(&controls->speedError,speedError,elapsed);
    float throttle=0,brake=0;
    int32_t gear;
    if (!rearContact) {gear=1;aiControlsClearSpeedFilter(controls);}
    else if (fabsf(controls->targetSpeed)<2) {gear=-1;brake=1;}
    else if (controls->targetSpeed>0) {
        if (signedSpeed < -7) controls->reverseHysteresis=0;
        else if (signedSpeed > -2) controls->reverseHysteresis=1;
        if (!controls->reverseHysteresis) {gear=-1;brake=.5f;steering=-steering;}
        else {gear=-1;aiControlsComputePedals(controls,&throttle,&brake);}
    } else {
        if (signedSpeed>7) controls->reverseHysteresis=0;
        else if (signedSpeed<2) controls->reverseHysteresis=1;
        if (!controls->reverseHysteresis) {gear=-1;brake=1;}
        else {gear=0;steering=-steering;aiControlsComputePedals(controls,&throttle,&brake);}
    }
    float angleDegrees;
    if (frontContact) {
        const float limit=limitedHandling ? clamp(sampledCurveEvaluate(&steeringLimitCurve,signedSpeed),0,45):45;
        const float previous=currentSteering/limit;
        float requested=steering/(limit*.01745329424738884f);
        if (!overshoot && limitedHandling) requested=clamp(requested,-1,1);
        float change=requested-previous;
        if (limitedHandling || !overshoot) {
            float rate=sampledCurveEvaluate(&steeringRateCurve,signedSpeed)*.5f;
            if ((previous>0 && requested<previous) || (previous<0 && previous<requested)) rate+=rate;
            change=clamp(change,-rate*interval,rate*interval);
        }
        angleDegrees=(previous+change)*limit;
        if (limitedHandling) angleDegrees*=movement->info.steeringRangeDegrees;
    } else angleDegrees=steering*57.2957763671875f;
    controls->targetDuration-=elapsed;if (controls->targetDuration<0) controls->targetDuration=0;
    output->throttle=controls->enabled ? throttle:0;output->brake=controls->enabled ? brake:0;
    controls->forcedHandbrakeTime-=elapsed;
    if (controls->forcedHandbrakeTime>0 && fabsf(signedSpeed)>8.940545082092285f) {
        output->throttle=0;output->brake=0;output->handbrake=1;
    } else output->handbrake=0;
    output->gearRequest=gear;
    output->steeringAngle=(uint16_t)(convertFloatToInt32(angleDegrees*65536)/360);
}

void aiControlsSetTarget(AIControls* controls,int32_t enabled,float speed,float duration,float tolerance)
{
    controls->enabled=enabled;controls->targetSpeed=speed;controls->targetDuration=duration;controls->speedTolerance=tolerance;
}
void aiControlsSetHandbrakeTime(AIControls* controls,float seconds) {controls->forcedHandbrakeTime=seconds;}
void aiControlsStartHorn(AIControls* controls) {if (controls->bindings.sound && !carSoundStartHorn(controls->bindings.sound)) controls->result=CAR_DRIVER_CAPACITY;}
void aiControlsStopHorn(AIControls* controls) {if (controls->bindings.sound && !carSoundStopHorn(controls->bindings.sound)) controls->result=CAR_DRIVER_CAPACITY;}
int32_t aiControlsHornActive(const AIControls* controls) {return controls->bindings.sound ? carSoundHornActive(controls->bindings.sound):0;}
