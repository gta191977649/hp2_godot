#ifndef NFSHP2_REAL_DRIVER_HPP
#define NFSHP2_REAL_DRIVER_HPP

#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Car.hpp"
#include "CarSounds.hpp"
#include "indep/inc/Table.hpp"
#include "indep/inc/DriveTarget.hpp"
#include "indep/inc/CarDriver.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct CarFramePlayerState;
struct Chopper;
struct ReplayManager;
struct CarFrame;
struct PursuitManager;
typedef struct RealDriver RealDriver;

typedef struct RealDriverConfiguration {
    float steeringRangeDegrees;
    int32_t forwardGearCount;
    int32_t bodyCategory;
} RealDriverConfiguration;

typedef struct RealDriverTuning {
    SampledCurve wheelSteeringRange;
    SampledCurve padSteeringRange;
    SampledCurve steeringRate;
    SampledCurve steeringResponse;
    SampledCurve pedalResponse;
    SampledCurve countersteerScale;
    SampledCurve linearResponse;
    float countersteerWeight;
} RealDriverTuning;

typedef struct RealDriverBindings {
    const RealDriverConfiguration* configuration;
    RealDriverTuning* tuning;
    struct CarFramePlayerState* player;
    CarSoundState* sound;
    struct Chopper* chopper;
    struct ReplayManager* replay;
    struct CarFrame* frame;
    GameInputRouter* inputRouter;
    struct PursuitManager* pursuitManager;
} RealDriverBindings;

struct RealDriver {
    CarMovement* movement;
    const RealDriverConfiguration* configuration;
    RealDriverTuning* tuning;
    struct CarFramePlayerState* player;
    CarSoundState* sound;
    struct Chopper* chopper;
    struct ReplayManager* replay;
    struct CarFrame* frame;
    GameInputRouter* inputRouter;
    int32_t port;
    int32_t inputEnabled;
    int32_t forceBrake;
    int32_t forceReverse;
    float handbrakeTime;
    int32_t countersteering;
    int32_t gearInputMode;
    uint8_t steeringBytes[2];
    float steeringInput;
    uint8_t throttleBytes[3];
    float throttleInput;
    uint8_t brakeBytes[3];
    float brakeInput;
    uint8_t alternateThrottleBytes[2];
    float alternateThrottle;
    uint8_t handbrakeBytes[2];
    float handbrakeInput;
    int32_t nitroStartedTicks;
    int32_t reverseRequested;
    CarDriverResult result;
    DriverCommand command;
    struct PursuitManager* pursuitManager;
};



void realDriverTuningInitialize(RealDriverTuning* tuning);
CarDriverResult realDriverInitialize(RealDriver* driver,CarMovement* movement,const RealDriverBindings* bindings);
void realDriverDispose(RealDriver* driver);
void realDriverClearInput(RealDriver* driver);
void realDriverSteeringInput(RealDriver* driver, int32_t event, uint8_t value);
void realDriverThrottleInput(RealDriver* driver, int32_t event, uint8_t value);
void realDriverAlternateThrottleInput(RealDriver* driver, int32_t event, uint8_t value);
void realDriverBrakeInput(RealDriver* driver, int32_t event, uint8_t value);
void realDriverHandbrakeInput(RealDriver* driver, int32_t event, uint8_t value);
void realDriverSelectDefaultGear(RealDriver* driver);
void realDriverSetHandbrakeTime(RealDriver* driver,float seconds);
void realDriverUnlock(RealDriver* driver);
void realDriverSetGearInputMode(RealDriver* driver,int32_t mode);
void realDriverDispatchInput(RealDriver* driver,int32_t device,int32_t event,uint8_t value);
void realDriverHornInput(RealDriver* driver,uint8_t pressed);
void realDriverUpdatePedals(RealDriver* driver, float elapsed);
void realDriverUpdateSteering(RealDriver* driver, float elapsed);
void realDriverReadInput(RealDriver* driver, float elapsed);
void realDriverDoSnapshot(RealDriver* driver,struct ReplaySnapshot* snapshot);
CarDriverResult realDriverActivateInput(RealDriver* driver);
CarDriverResult realDriverDeactivateInput(RealDriver* driver);
CarDriverResult realDriverStartHorn(RealDriver* driver);
CarDriverResult realDriverStopHorn(RealDriver* driver);
void realDriverShiftUpInput(RealDriver* driver, int32_t device, int32_t event, uint8_t pressed);
void realDriverShiftDownInput(RealDriver* driver, int32_t device, int32_t event, uint8_t pressed);
void realDriverRecoverInput(RealDriver* driver, uint8_t pressed);
void realDriverNitroInput(RealDriver* driver, uint8_t pressed);
int32_t realDriverNitroPreviouslyActive(const RealDriver* driver);
int32_t realDriverNitroActive(const RealDriver* driver);
int32_t realDriverNitroReady(const RealDriver* driver);

#ifdef __cplusplus
}
#endif
#endif
