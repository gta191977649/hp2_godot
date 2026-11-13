#pragma once

#include "Ecstasy.hpp"
#include "indep/inc/CarDriver.hpp"
#include "indep/inc/Timer.hpp"
#include "indep/inc/Table.hpp"
#include "indep/inc/Track.hpp"
#include "indep/inc/TrackRoute.hpp"


#include "indep/inc/RaceParameters.hpp"
#include "indep/inc/QuickSpline.hpp"
#include "indep/inc/Point.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct CarMovement;
struct World;
struct CarCameraShake;

/* Shared native view result and opaque coordinator handles. Keep the
 * session declaration independent of the coordinator's World storage. */
typedef struct CarViewMover CarViewMover;
typedef struct CarViewMoverRegistry CarViewMoverRegistry;
typedef enum CarViewMoverResult {
    CAR_VIEW_MOVER_OK,
    CAR_VIEW_MOVER_INVALID_BINDING,
    CAR_VIEW_MOVER_ALLOCATION_FAILURE,
    CAR_VIEW_MOVER_MISSING_RESOURCE,
    CAR_VIEW_MOVER_CAMERA_FAILURE
} CarViewMoverResult;

/* Authored camera records. The halfword parameters retain their encoded
 * values until the presentation backend consumes the selected view. */
typedef struct CarCameraView {
    int32_t kind;
    float offset[3];
    uint16_t parameters[8];
} CarCameraView;

typedef struct CarCameraCatalogue {
    int32_t viewCount[2];
    CarCameraView views[2][4];
} CarCameraCatalogue;

/* The caller supplies the live presentation layout. Source-valid indices
 * are nonnegative; a missing view returns null. Layout three uses the
 * authored split-screen catalogue, all others use the normal catalogue. */
const CarCameraView* carCameraViewAt(const CarCameraCatalogue* catalogue,
                                     int32_t presentationLayout, int32_t index);

typedef float CarCameraPoint[4];

typedef struct CarCameraPathPlayback {
    float eyeAdvanceRate;
    float targetAdvanceRate;
    float authoredParameter;
    uint32_t eyeUsesRouteFraction;
    uint32_t targetUsesRouteFraction;
    float routeStart;
    float routeEnd;
} CarCameraPathPlayback;

typedef struct CarCameraPathProjection {
    uint32_t usesWidth;
    float routeStart;
    float routeEnd;
    const CarCameraPoint* lensKeys; /* Exactly 32 retained samples. */
} CarCameraPathProjection;

typedef struct CarCameraPathLegacy {
    float authoredCoefficients[8];
    float start[2];
    float end[2];
    uint32_t interpolate;
    float eyeProjectionGate;
    uint32_t closestApproachGate;
    uint32_t placementSelector;
} CarCameraPathLegacy;

/* This is a semantic view. Scalar values are copied into a camera at its
 * construction; every point span still borrows the same Player/resource
 * owner. That owner and its arrays must outlive all cameras using the view.
 * No console object layout or second copy of the point storage is exposed. */
typedef struct CarCameraPathItems {
    uint32_t format;
    const char* name;
    CarCameraPathPlayback playback;
    CarCameraPathProjection projection;
    float nearClip;
    float farClip;
    uint32_t directionGate;
    CarCameraPathLegacy legacy;
    const CarCameraPoint* eyeKeys;
    const CarCameraPoint* rollKeys;
    int32_t eyeCount;
    const CarCameraPoint* targetKeys;
    int32_t targetCount;
} CarCameraPathItems;

/* The Player factory's offset track is a separate retained value. Its
 * metadata words are preserved without assigning unknown gameplay meaning.
 * The source producer for cinematic cases 15/16 clears this record and sets
 * count to two; other real camera producers may supply nonzero keys/rate. */
typedef struct CarCameraOffsetTrack {
    char name[32];
    uint32_t flags;
    float advanceRate;
    int32_t count;
    uint32_t properties;
    CarCameraPoint keys[32];
} CarCameraOffsetTrack;

typedef struct CarCameraShakeChannel {
    float direction[4];
    const KeyedCurve* envelope;
    float phase;
    float duration;
    float frequency;
} CarCameraShakeChannel;

/* Both screen channels borrow the same envelope owned by this service.
 * Sampling changes the CPU view matrix; it is not a renderer request. */
typedef struct CarCameraShake {
    CarCameraShakeChannel channels[2];
    CurveKey envelopeKeys[5];
    KeyedCurve envelope;
    float rotationScale[3];
    float testDirection[4];
    float testDuration;
    int32_t testRequested;
} CarCameraShake;

/* Envelopes have at least one key with increasing finite coordinates.
 * Duration is positive and phase*frequency fits a signed 32-bit integer. */
int32_t carCameraEnvelopeSegment(const KeyedCurve* curve, float coordinate);
int32_t carCameraEnvelopeOutside(const KeyedCurve* curve, float coordinate);
float carCameraEnvelopeSlope(const KeyedCurve* curve, int32_t segment);
float carCameraEnvelopeEvaluate(const KeyedCurve* curve, float coordinate);
void carCameraShakeInitialize(CarCameraShake* shake);
void carCameraShakeReset(CarCameraShake* shake);
void carCameraShakeStart(CarCameraShakeChannel* channel, const KeyedCurve* envelope,
    const float direction[4], float duration, float frequency);
void carCameraShakeAdvanceChannel(CarCameraShakeChannel* channel, float elapsed);
void carCameraShakeSample(const CarCameraShakeChannel* channel, float output[4]);
void carCameraShakeAdvance(CarCameraShake* shake, float elapsed);
void carCameraShakeSampleScreen(const CarCameraShake* shake, int32_t channel, float output[4]);
void carCameraShakeApply(const CarCameraShake* shake, const struct World* world,
    int32_t viewNumber, float matrix[16]);


typedef struct CarAttachedCamera CarAttachedCamera;
typedef struct CarAttachedCameraSettings { float translationBias[3]; } CarAttachedCameraSettings;

typedef struct CarAttachedCameraContext {
    CarCinematicServices* services;
    const struct CarMovement* movement;
    const struct World* world;
    const struct CarCameraShake* shake;
    const CarAttachedCameraSettings* settings;
    int32_t viewNumber;
    // Native mirror terrain query borrows the same track topology as the
    // other car cameras; the PAL owner fields above remain unchanged.
    TerrainTopology* terrain;
} CarAttachedCameraContext;

void carAttachedCameraSettingsInitialize(CarAttachedCameraSettings* settings);
/* PC-only mirror eye clearance; never used by authored main-view cameras.
 * The function does not mutate car state, physics or the ground surface. */
float carAttachedCameraNativeEyeHeight(float eyeHeight,float groundHeight,bool found);
CarAttachedCamera* carAttachedCameraCreate(const CarAttachedCameraContext* context,
    CarCinematicResult* result);
void carAttachedCameraDispose(CarAttachedCamera* camera);
CarCinematicResult carAttachedCameraAdvance(CarAttachedCamera* camera);
CarCinematicBase* carAttachedCameraBase(CarAttachedCamera* camera);


struct CarAttachedCamera {
    CarCinematicBase base;
    CarAttachedCameraContext context;
    TerrainSurface nativeMirrorTerrain;
    bool nativeMirrorTerrainActive;
};

struct CarReplayPresentationState;
struct CarViewRecordedTake;
typedef struct CarRecordedCamera CarRecordedCamera;
typedef struct CarRecordedCameraSettings {
    uint16_t rotation[3];
    float fieldOfViewOverride;
} CarRecordedCameraSettings;
typedef struct CarRecordedCameraContext {
    CarCinematicServices* services;
    struct CarMovement* movement;
    const struct CarReplayPresentationState* presentation;
    const struct CarViewRecordedTake* take;
    const CarRecordedCameraSettings* settings;
    int32_t viewNumber;
    int32_t startWorldTicks;
    float timeOffset;
} CarRecordedCameraContext;

void carRecordedCameraSettingsInitialize(CarRecordedCameraSettings* settings);
CarRecordedCamera* carRecordedCameraCreate(const CarRecordedCameraContext* context,CarCinematicResult* result);
void carRecordedCameraDispose(CarRecordedCamera* camera);
CarCinematicResult carRecordedCameraAdvance(CarRecordedCamera* camera);
CarCinematicBase* carRecordedCameraBase(CarRecordedCamera* camera);
const struct CarViewRecordedTake* carRecordedCameraTake(const CarRecordedCamera* camera);

struct CarRecordedCamera {
    CarCinematicBase base;
    CarRecordedCameraContext context;
    int32_t startedWorldTicks;
};

extern const CarCinematicMethods carCinematicChaseMethods;
extern const CarCinematicMethods carCinematicFixedMethods;
extern const CarCinematicMethods carCinematicDriveMethods;
extern const CarCinematicMethods carCinematicOrbitMethods;
extern const CarCinematicMethods carCinematicZoneMethods;
extern const CarCinematicMethods carCinematicCrashMethods;
extern const CarCinematicMethods carCinematicAirborneMethods;
extern const CarCinematicMethods carCinematicVelocityMethods;
extern const CarCinematicMethods carCinematicAttachedMethods;
extern const CarCinematicMethods carCinematicBumperMethods;
extern const CarCinematicMethods carCinematicPathMethods;
extern const CarCinematicMethods carCinematicRecordedMethods;


CarCinematicResult carCinematicRenderMovers(const CarCinematicScreen* screen,NativeEcstasyFrame* frame);
struct Car* carCinematicGetCarToFollow(const CarCinematicBase* camera);

void carCinematicBindOwner(CarCinematicBase* camera,void* owner,
    const CarCinematicMethods* methods);
CarCinematicResult carCinematicDestroyOwned(CarCinematicBase* camera);
CarCinematicResult carCinematicAdvanceOwned(CarCinematicBase* camera);
int32_t carCinematicCurrentViewMode(const CarCinematicServices* services);

void carCinematicLookAt(float output[16],const float eye[4],const float target[4],const float up[4]);
void carCinematicViewInitialize(CarCinematicServices* services,CarCinematicView* view);
void carCinematicViewPublish(CarCinematicServices* services,CarCinematicView* view,
                              const float matrix[16],float elapsed);
void carCinematicConstrainTransition(TerrainSurface* surface,float previousEye[4],float eye[4],
                                      float previousTarget[4],float target[4]);
CarCinematicResult carCinematicBaseInitialize(CarCinematicBase* camera,CarCinematicServices* services,
                                              int32_t viewNumber,int32_t kind);
void carCinematicBaseActivate(CarCinematicBase* camera);
void carCinematicBaseDeactivate(CarCinematicBase* camera);
void carCinematicBaseDispose(CarCinematicBase* camera);


/* Camera declarations use borrowed services and decoded resources. */
typedef struct CarMovement CarMovement;
typedef struct Car Car;
typedef struct RaceFrame RaceFrame;
typedef struct PursuitManager PursuitManager;
typedef struct PursuitCollisionEntry PursuitCollisionEntry;

struct CarMovement;
struct WorldSimulation;
struct CarCameraShake;

/* This is the live chase owner. The catalogue, selected index and facing
 * weight are read here by transitions; they are not copied into a cinematic
 * event or maintained as a second set of Player flags. */
typedef struct CarChaseCamera CarChaseCamera;

typedef struct CarChaseCameraSettings {
    int32_t useAttachedCar;
    int32_t selectedDriverNumber;
    int32_t overrideView;
    float overrideOffset[3];
    uint16_t overridePitch[2];
    float rollPositiveScale;
    float rollNegativeScale;
    float positiveRollThreshold;
    float negativeRollThreshold;
    float movementReduction;
    float distanceResponseScale;
    float maximumResponseSpeed;
    float negativeDistanceScale;
    float maximumRollResponse;
    float minimumRollResponse;
    float dynamicRollScale;
    float recenterFastBoundary;
    float responseAcceleration;
    float responseDecay;
    float responseTimeout;
    float terrainPitchNoise;
    float responseScale;
    int8_t ignoredTerrainMaterials[33];
} CarChaseCameraSettings;

typedef struct CarChaseCameraVehicle {
    struct CarMovement* movement;
    const CarCameraCatalogue* catalogue;
    const float* targetClearance;
} CarChaseCameraVehicle;

/* The vehicle table borrows actual movement and immutable resource owners.
 * It and their catalogue/clearance storage outlive the camera. Every camera
 * in one world shares the same mutable settings and shake service. */
typedef struct CarChaseCameraContext {
    CarCinematicServices* services;
    TerrainTopology* terrain;
    struct CarFramePlayerState* player;
    struct CarMovement* movement;
    const CarChaseCameraVehicle* vehicles;
    size_t vehicleCount;
    const CarCameraCatalogue* defaultCatalogue;
    const struct WorldSimulation* simulation;
    struct CarCameraShake* shake;
    CarChaseCameraSettings* settings;
    int32_t viewNumber;
    int32_t viewIndex;
    int32_t dynamicResponse;
    int32_t initiallyFacingBack;
    float facingRate;
    int32_t extendedVisibility;
} CarChaseCameraContext;

void carChaseCameraSettingsInitialize(CarChaseCameraSettings* settings);
CarChaseCamera* carChaseCameraCreate(const CarChaseCameraContext* context, CarCinematicResult* result);
void carChaseCameraDispose(CarChaseCamera* camera);
void carChaseCameraReset(CarChaseCamera* camera);
void carChaseCameraSetViewIndex(CarChaseCamera* camera, int32_t index);
void carChaseCameraSetFacingTarget(CarChaseCamera* camera, int32_t facingBack);
CarCinematicResult carChaseCameraAdvance(CarChaseCamera* camera);
CarCinematicBase* carChaseCameraBase(CarChaseCamera* camera);

int32_t carChaseCameraHasRearFacing(const CarChaseCamera* camera);
int32_t carChaseCameraUsesForwardSide(const CarChaseCamera* camera);
float carChaseCameraTerrainClearance(const CarChaseCamera* camera);

struct CarChaseCamera {
    CarCinematicBase base;
    struct CarMovement* movement;
    const CarCameraCatalogue* catalogue;
    int32_t viewIndex;
    float facingTarget;
    float facingWeight;
    float facingRate;
    uint16_t targetYaw;
    float targetDistance;
    CarChaseCameraContext context;
    uint16_t yawOffset;
    uint16_t previousHeading;
    uint16_t smoothedYaw;
    uint16_t previousPitch;
    uint16_t smoothedPitch;
    float previousForwardSpeed;
    float smoothingWeight;
    TerrainSurface surface;
    float rollHistory[20];
    float rollResponse;
    float desiredRollResponse;
    float rollIdleTime;
    float substepRemainder;
    float responseScale;
};

float carChaseCameraAngularRate(uint16_t difference);
int32_t carChaseCameraPitchStep(uint16_t difference);
uint16_t carChaseCameraTargetQuadrant(const struct CarMovement* first,
    const struct CarMovement* second, const float orientation[16]);
PursuitCollisionEntry* carCameraClosestPursuer(const PursuitManager* manager,
    const struct CarMovement* target);
struct CarMovement* carChaseCameraRelatedVehicle(const PursuitManager* manager,
    const struct CarMovement* movement, int32_t* isPursuer);
const CarChaseCameraVehicle* carChaseCameraVehicleForCar(const CarChaseCamera* camera,
    const Car* car);

typedef struct CarBumperCamera CarBumperCamera;
typedef struct CarBumperCameraContext {
    CarCinematicServices* services;
    TerrainTopology* terrain;
    CarMovement* movement;
    struct CarFramePlayerState* player;
    const RaceParameters* race;
    const RaceFrame* raceFrame;
    const CarCameraPathItems* items;
    const int8_t* ignoredTerrainMaterials;
    int32_t viewNumber;
} CarBumperCameraContext;

CarBumperCamera* carBumperCameraCreate(const CarBumperCameraContext* context,CarCinematicResult* result);
void carBumperCameraDispose(CarBumperCamera* camera);
const float* carBumperCameraFocusPosition(const CarBumperCamera* camera);
int32_t carBumperCameraFinished(const CarBumperCamera* camera);
CarCinematicResult carBumperCameraAdvance(CarBumperCamera* camera);
CarCinematicBase* carBumperCameraBase(CarBumperCamera* camera);
void carBumperCameraConstrain(CarBumperCamera* camera,float matrix[16]);

struct CarBumperCamera {
    CarCinematicBase base;
    CarBumperCameraContext context;
    CarCameraPathItems items;
    Spline eye,roll,target,lens;
    float eyeParameter,targetParameter;
    TerrainSurface surface;
};

typedef struct CarAirborneCamera CarAirborneCamera;

/* The live direction override is shared by cameras, rather than copied into
 * each Player. An all-zero override selects the authored coefficients. */
typedef struct CarAirborneCameraSettings { float directionOverride[4]; } CarAirborneCameraSettings;

typedef struct CarAirborneCameraContext {
    CarCinematicServices* services;
    struct CarFramePlayerState* player;
    const CarMovement* movement;
    const RaceParameters* race;
    const CarCameraPathItems* items;
    const CarCameraOffsetTrack* offset;
    const CarAirborneCameraSettings* settings;
    int32_t viewNumber;
    int32_t extendedVisibility;
} CarAirborneCameraContext;

void carAirborneCameraSettingsInitialize(CarAirborneCameraSettings* settings);
CarAirborneCamera* carAirborneCameraCreate(const CarAirborneCameraContext* context,
    CarCinematicResult* result);
void carAirborneCameraDispose(CarAirborneCamera* camera);
CarCinematicResult carAirborneCameraAdvance(CarAirborneCamera* camera);
const float* carAirborneCameraFocusPosition(const CarAirborneCamera* camera);
void carAirborneCameraWeightedDirection(const CarAirborneCamera* camera, float output[4],
    const float displacement[4], const float velocity[4]);
CarCinematicBase* carAirborneCameraBase(CarAirborneCamera* camera);

struct CarAirborneCamera {
    CarCinematicBase base;
    const CarMovement* movement;
    const RaceParameters* race;
    CarCameraPathItems items;
    CarCameraOffsetTrack offset;
    Spline lens, eye, roll, offsets;
    float eyeParameter, offsetParameter;
    float anchor[4], directionVelocity[4], directionDisplacement[4];
    float directionWeight, velocityWeight;
    float firstEye[4], lastEye[4];
    int32_t passedClosestPoint;
    float closestDistance;
    tCubic1D focusNoise;
    int32_t extendedVisibility;
};

typedef struct CarPathCamera CarPathCamera;
typedef struct CarPathCameraContext {
    CarCinematicServices* services;
    struct CarFramePlayerState* player;
    const CarMovement* movement; /* Optional: an unattached path plays in time. */
    const RaceParameters* race;
    const CarCameraPathItems* items;
    int32_t viewNumber;
} CarPathCameraContext;

CarPathCamera* carPathCameraCreate(const CarPathCameraContext* context, CarCinematicResult* result);
void carPathCameraDispose(CarPathCamera* camera);
CarCinematicResult carPathCameraAdvance(CarPathCamera* camera);
const float* carPathCameraFocusPosition(const CarPathCamera* camera);
int32_t carPathCameraFinished(const CarPathCamera* camera);
CarCinematicBase* carPathCameraBase(CarPathCamera* camera);

struct CarPathCamera {
    CarCinematicBase base;
    const CarMovement* movement;
    const RaceParameters* race;
    CarCameraPathItems items;
    Spline eye, roll, target, lens;
    float eyeParameter, targetParameter;
};

void carCameraUp(float output[4], const float eye[4], const float target[4], uint16_t roll);

void carCameraTerrainOffset(const CarChaseCamera* camera, TerrainSurface* surface,
    const float first[4], const float second[4], float offset[4]);
void carCameraLookAtWithSlope(float output[16], const float eye[4],
    const float target[4], const float slope[2]);

/* Shared numerical body of the concrete camera clearance operation. This
 * carries no camera identity or virtual dispatch substitute. */
void carCameraTerrainOffsetWithClearance(TerrainSurface* surface,
    const int8_t ignoredMaterials[33],float clearance,
    const float first[4],const float second[4],float offset[4]);


/* Concrete cameras borrow input and world services without copying gameplay state. */
typedef struct TrackRouteEditor TrackRouteEditor;

typedef struct CarPlayerFixedCamera CarPlayerFixedCamera;
typedef struct CarPlayerOrbitCamera CarPlayerOrbitCamera;
struct CarFramePlayerRegistry;

typedef enum CarPlayerCameraResult {
    CAR_PLAYER_CAMERA_OK,
    CAR_PLAYER_CAMERA_INVALID_BINDING,
    CAR_PLAYER_CAMERA_INPUT_CAPACITY,
    CAR_PLAYER_CAMERA_MOVEMENT_FAILURE
} CarPlayerCameraResult;

/* All free-camera instances use the same live view vectors and modifiers.
 * Presentation reads these owners directly; there is no gameplay snapshot. */
typedef struct CarFixedCameraSharedState {
    float position[4];
    float target[4];
    int32_t modifiers[2];
} CarFixedCameraSharedState;

typedef struct CarPlayerCameraServices {
    GameInputRouter* input;
    World* world;
    struct CarFramePlayerRegistry* players;
    CarFixedCameraSharedState fixed;
    const CarCameraCatalogue* defaultCatalogue;
    const int32_t* presentationLayout;
    const int32_t* allowAllViewKinds;
} CarPlayerCameraServices;

struct CarPlayerFixedCamera {
    CarPlayerCameraServices* services;
    int32_t device;
    int32_t inputEnabled;
    float movementRate[3];
    float analogForwardRate;
    int16_t headingRate;
    int16_t pitchRate;
    CarPlayerCameraResult result;
};

struct CarPlayerOrbitCamera {
    CarPlayerCameraServices* services;
    int32_t device;
    int32_t inputEnabled;
    int32_t fixedTargetMode;
    Car* selectedCar;
    const float* fixedTarget;
    int32_t angularRate[2];
    float distanceFactor;
    int32_t followChopper;
    CarPlayerCameraResult result;
};

CarPlayerCameraResult carPlayerFixedCameraActivateInput(CarPlayerFixedCamera* camera);
void carPlayerFixedCameraDeactivateInput(CarPlayerFixedCamera* camera);
CarPlayerCameraResult carPlayerOrbitCameraActivateInput(CarPlayerOrbitCamera* camera);
void carPlayerOrbitCameraDeactivateInput(CarPlayerOrbitCamera* camera);

void carPlayerFixedCameraDispatchInput(CarPlayerFixedCamera* camera,
    int32_t device, int32_t event, uint8_t value);
void carPlayerOrbitCameraDispatchInput(CarPlayerOrbitCamera* camera,
    int32_t device, int32_t event, uint8_t value);

typedef struct CarFixedCamera CarFixedCamera;

typedef struct CarCameraMarkerPositions {
    const float (*positions)[4];
    size_t count;
    int32_t enabled;
} CarCameraMarkerPositions;

/* The original tool transport receives the latest fixed-point camera pose.
 * Windows tooling consumes this request; it does not own the camera view. */
typedef struct CarFixedCameraCapture {
    int32_t enabled;
    int32_t position[3];
    int32_t target[3];
    int32_t pending;
    int32_t transmissionRequested;
} CarFixedCameraCapture;

/* Shared commands are consumed in place by every free-camera instance.
 * Positions and input modifiers remain in CarPlayerCameraServices.fixed. */
typedef struct CarFixedCameraCommands {
    int32_t markerRequest;
    float targetRequest[4];
    float firstModifierMultiplier;
    float secondModifierMultiplier;
    const CarCameraMarkerPositions* markers;
    CarFixedCameraCapture* capture;
} CarFixedCameraCommands;

typedef struct CarFixedCameraContext {
    CarCinematicServices* cameras;
    CarPlayerCameraServices* players;
    TerrainTopology* terrain;
    CarFixedCameraCommands* commands;
    int32_t viewNumber;
    int32_t device;
} CarFixedCameraContext;

void carFixedCameraCommandsInitialize(CarFixedCameraCommands* commands);
CarFixedCamera* carFixedCameraCreate(const CarFixedCameraContext* context,
    const float position[4],const float target[4],CarCinematicResult* result);
void carFixedCameraDispose(CarFixedCamera* camera);
CarCinematicResult carFixedCameraAdvance(CarFixedCamera* camera);
CarCinematicBase* carFixedCameraBase(CarFixedCamera* camera);
CarPlayerFixedCamera* carFixedCameraInput(CarFixedCamera* camera);

struct CarFixedCamera {
    CarCinematicBase base;
    CarPlayerFixedCamera input;
    TerrainTopology* terrain;
    CarFixedCameraCommands* commands;
    int32_t lastCaptureFrame;
};

struct WorldSimulation;
typedef struct CarOrbitCamera CarOrbitCamera;

typedef struct CarOrbitCameraSettings {
    int32_t overridePose;
    float overrideDistance;
    uint16_t overrideHeading;
    uint16_t overridePitch;
    float targetHeight;
    float neutralDistance;
    float carTranslationBias[4];
    uint16_t carInitialFieldOfView;
    uint16_t fixedInitialFieldOfView;
    uint16_t fieldOfView;
} CarOrbitCameraSettings;

/* The table borrows the actual movement owners for the World car list.
 * It supplies live center-of-mass positions and orientations after selection
 * changes; no per-camera copy of the selected car's motion is maintained. */
typedef struct CarOrbitCameraContext {
    CarCinematicServices* cameras;
    CarPlayerCameraServices* players;
    const CarMovement* const* vehicles;
    size_t vehicleCount;
    const struct WorldSimulation* simulation;
    CarOrbitCameraSettings* settings;
    int32_t viewNumber;
    int32_t device;
} CarOrbitCameraContext;

void carOrbitCameraSettingsInitialize(CarOrbitCameraSettings* settings);
CarOrbitCamera* carOrbitCameraCreateForCar(const CarOrbitCameraContext* context,
    Car* car,CarCinematicResult* result);
CarOrbitCamera* carOrbitCameraCreateAtTarget(const CarOrbitCameraContext* context,
    const float* target,CarCinematicResult* result);
void carOrbitCameraDispose(CarOrbitCamera* camera);
CarCinematicResult carOrbitCameraAdvance(CarOrbitCamera* camera);
const float* carOrbitCameraTargetPosition(const CarOrbitCamera* camera);
CarCinematicBase* carOrbitCameraBase(CarOrbitCamera* camera);
CarPlayerOrbitCamera* carOrbitCameraInput(CarOrbitCamera* camera);

struct CarOrbitCamera {
    CarCinematicBase base;
    CarPlayerOrbitCamera input;
    CarOrbitCameraContext context;
    float distance;
    float requestedDistance;
    float minimumDistance;
    float maximumDistance;
    uint16_t heading;
    uint16_t pitch;
    uint16_t centeredPitch;
    int32_t headingRate;
    int32_t pitchRate;
    int32_t acceleration;
    int32_t recenterPitch;
};

const CarMovement* carOrbitCameraSelectedMovement(const CarOrbitCamera* camera);

typedef struct CarVelocityCamera CarVelocityCamera;

typedef struct CarVelocityCameraSettings {
    float targetGain[3];
    float targetDirectionScale[3];
    float targetThreshold[3];
    float targetDuration[3];
    float targetOffset[3];
    float eyeGain[3];
    float eyeDirectionScale[3];
    float eyeThreshold[3];
    float eyeDuration[3];
    float eyeOffset[3];
} CarVelocityCameraSettings;

typedef struct CarVelocityCameraContext {
    CarCinematicServices* services;
    const CarMovement* movement;
    const CarVelocityCameraSettings* settings;
    struct CarFramePlayerState* player;
    int32_t viewNumber;
} CarVelocityCameraContext;

void carVelocityCameraSettingsInitialize(CarVelocityCameraSettings* settings);
CarVelocityCamera* carVelocityCameraCreate(const CarVelocityCameraContext* context,
    CarCinematicResult* result);
void carVelocityCameraDispose(CarVelocityCamera* camera);
void carVelocityCameraUpdateOffsets(CarVelocityCamera* camera);
void carVelocityCameraRefreshTargets(CarVelocityCamera* camera);
CarCinematicResult carVelocityCameraAdvance(CarVelocityCamera* camera);
CarCinematicBase* carVelocityCameraBase(CarVelocityCamera* camera);

struct CarVelocityCamera {
    CarCinematicBase base;
    CarVelocityCameraContext context;
    float eyeOffset[4];
    float targetOffset[4];
    float previousVelocity[4];
    tCubic3D eye;
    tCubic3D target;
};

typedef struct CarDriveCamera CarDriveCamera;

CarDriveCamera* carDriveCameraCreate(CarCinematicServices* services,int32_t viewNumber,
    TrackRouteEditor* editor,CarCinematicResult* result);
void carDriveCameraDispose(CarDriveCamera* camera);
CarCinematicResult carDriveCameraAdvance(CarDriveCamera* camera);
CarCinematicBase* carDriveCameraBase(CarDriveCamera* camera);


typedef enum CarCameraReferenceKind {
    CAR_CAMERA_REFERENCE_BASE,
    CAR_CAMERA_REFERENCE_CHASE
} CarCameraReferenceKind;

typedef struct CarCameraReference {
    CarCameraReferenceKind kind;
    union {
        const CarCinematicBase* base;
        const CarChaseCamera* chase;
    } owner;
} CarCameraReference;


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <array>
#include <cstdint>
#include "bsound/include/bSample.h"

namespace hp2 {

class SelectCarCameraMover final {
public:
    SelectCarCameraMover(CarCinematicServices& services,std::int32_t viewNumber,
        WorldAudioRequestQueue* audio);
    ~SelectCarCameraMover();
    SelectCarCameraMover(const SelectCarCameraMover&)=delete;
    SelectCarCameraMover& operator=(const SelectCarCameraMover&)=delete;
    void Update();
    void SetMode(std::int32_t mode);
    void SetRadiusStats(float radius);
    void SetOffsetStats(const float* offset);
    void SetOffsetOrbit(const float* offset);
    void SetFOVStats(std::uint16_t fieldOfView);
    void SetOrientationStats(std::uint16_t horizontal,std::uint16_t vertical);
    void SetVertAngleOrbit(std::uint16_t angle);
    float GetModeParam() const noexcept {return modeParam_;}
    CarCinematicBase& Base() noexcept {return base_;}
    float desiredHorizontalSpeed{300.0f},desiredVerticalSpeed{300.0f};
    float desiredRadiusSpeed{};
private:
    static void DisposeOwned(void* camera);
    static CarCinematicResult AdvanceOwned(void* camera);
    static const CarCinematicMethods methods_;
    void Play(std::size_t sample,float volume);
    void Bump(float speed,float scale);
    CarCinematicBase base_{};
    std::int32_t mode_{};
    float modeParam_{};
    const Car* followedCar_{};
    std::uint16_t horizontalStats_{},horizontalOrbit_{},verticalStats_{},verticalOrbit_{};
    std::uint16_t fieldOfViewStats_{10000},fieldOfViewOrbit_{10000};
    float radiusStats_{5.0f},radiusOrbit_{13.199999809265137f};
    float horizontalSpeed_{300.0f},verticalSpeed_{300.0f},radiusSpeed_{};
    std::array<float,4> offsetStats_{},offsetOrbit_{},orientationStats_{},orientationOrbit_{};
    WorldAudioRequestQueue* audio_{};
    std::array<WorldAudioVoiceRef,4> samples_{};
};

// The track selection screen owns this concrete CPU camera. Its base uses
// the same live camera stack/view and publication gates as gameplay cameras.
class FrontendStillCamera final {
public:
    FrontendStillCamera(CarCinematicServices& services,std::int32_t viewNumber);
    ~FrontendStillCamera();
    FrontendStillCamera(const FrontendStillCamera&)=delete;
    FrontendStillCamera& operator=(const FrontendStillCamera&)=delete;
    void Update();
    CarCinematicBase& Base() noexcept { return base_; }
    std::array<float,4> eye{{1.0f,1.0f,1.0f,0.0f}};
    std::array<float,4> target{};
    std::array<float,4> up{{0.0f,0.0f,1.0f,0.0f}};
    std::uint16_t fieldOfView{6000};
private:
    static void DisposeOwned(void* camera);
    static CarCinematicResult AdvanceOwned(void* camera);
    static const CarCinematicMethods methods_;
    CarCinematicBase base_{};
};

} // namespace hp2

#endif // __cplusplus


#include "indep/inc/World.hpp"

/* Native declarations shared by the original zonecameras.cpp owners.
 * Resource spans and service pointers retain their existing live owners. */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ChopperEffectGroup ChopperEffectGroup;

typedef struct CarRoadPreviewStep {
    const CarRecoveryGroup* group;
    const CarRecoveryPose* pose;
    float length;
    float across;
    int32_t lane;
    float acrossChange;
} CarRoadPreviewStep;

typedef struct CarRoadPreview {
    CarRoadPreviewStep* steps;
    int32_t count;
    float direction[2];
    float length;
    float initialAcross;
} CarRoadPreview;

void carRoadPreviewPointXY(const CarRecoveryPose* pose, float output[2], float along, float across);
void carRoadPreviewStepPosition(const CarRoadPreviewStep* step, float output[4]);
float carRoadPreviewChooseLane(const CarRoadPreview* preview, CarRoadPreviewStep* step,
                               const CarRecoveryGroup* group, const CarRecoveryPose* pose,
                               const float point[2], int32_t vehicleClass);
int32_t carRoadPreviewFindStart(const CarRoadPreview* preview, CarRoadPreviewStep* step,
                                const CarRecoveryPointRegistry* registry, const float point[2],
                                float maximumDistance, int32_t vehicleClass,
                                const CarRecoveryPose* previous, int32_t requireForwardDistance);
float carRoadPreviewAdvance(CarRoadPreview* preview, const CarRecoveryPointRegistry* registry,
                            int32_t index, float maximumDistance, int32_t vehicleClass,
                            int32_t requireForwardDistance);
int32_t carRoadPreviewInitialize(CarRoadPreview* preview, const CarRecoveryPointRegistry* registry,
                                 const CarRecoveryGroup* group, const CarRecoveryPose* pose,
                                 const float position[4], const float direction[2],
                                 float maximumFirstDistance, float length, int32_t vehicleClass,
                                 int32_t requireForwardDistance);
void carRoadPreviewDispose(CarRoadPreview* preview);


// Original ZoneFreezeCameraMover PAL160298..1614C4; shared typed camera owner.
typedef struct CarZoneFreezeCamera CarZoneFreezeCamera;

typedef struct CarZoneFreezeCameraContext {
    CarCinematicServices* services;
    TerrainTopology* terrain;
    struct CarFramePlayerState* player;
    const CarMovement* movement;
    CarCameraReference previous;
    int32_t viewNumber;
} CarZoneFreezeCameraContext;

CarZoneFreezeCamera* carZoneFreezeCameraCreate(const CarZoneFreezeCameraContext* context,
    CarCinematicResult* result);
void carZoneFreezeCameraDispose(CarZoneFreezeCamera* camera);
float carZoneFreezeCameraDuration(const CarZoneFreezeCamera* camera);
CarCinematicResult carZoneFreezeCameraAdvance(CarZoneFreezeCamera* camera);
const CarCinematicView* carZoneFreezeCameraView(const CarZoneFreezeCamera* camera);


struct CarZoneFreezeCamera {
    CarCinematicBase base;
    float elapsed;
    float initialFovTangent;
    tCubic3D entryEye;
    tCubic3D exitEye;
    tCubic3D entryTarget;
    tCubic3D exitTarget;
    tCubic1D zoom;
    Spline eye;
    Spline target;
    TerrainSurface surface;
};


typedef struct PreviewComet PreviewComet;

typedef struct PreviewCometParameters {
    const char* emitterGroup;
    int32_t segments;
    float uvRate;
    uint32_t textureKey;
    float width;
    float height;
    float pathHeight;
    float curvatureScale;
    KeyedCurve widthCurve;
    KeyedCurve startCurve;
    KeyedCurve endCurve;
    float firstColor[4];
    float lastColor[4];
} PreviewCometParameters;

/* The same GLOBALB emitter definitions, live AcidEffectSystem and loaded
 * texture catalogues used by the rest of the engine. */
typedef struct PreviewCometServices {
    WorldParticleSystem* particles;
    const ChopperEffectGroup* emitterGroups;
    size_t emitterGroupCount;
    EditorMarkerResources* assets;
} PreviewCometServices;

typedef struct PreviewCometVertex {
    float position[3];
    float uv[2];
    uint8_t color[4];
} PreviewCometVertex;

typedef struct PreviewCometQuad {
    PreviewCometVertex vertices[4];
} PreviewCometQuad;

/* A renderer-owned output span replaces the console's transient ePoly/DMA
 * allocation. It contains real world-space textured geometry, in source
 * submission order, and borrows the actual loaded texture. */
typedef struct PreviewCometMesh {
    PreviewCometQuad* quads;
    size_t capacity;
    size_t count;
    const EditorMarkerTexture* texture;
} PreviewCometMesh;

const PreviewCometParameters* previewCometParameters(int32_t variant);
PreviewComet* previewCometInitialize(Spline* spline,
    const PreviewCometParameters* parameters,const PreviewCometServices* services);
void previewCometDispose(PreviewComet* comet);
WorldServicesResult previewCometEnableEmitters(PreviewComet* comet);
void previewCometDisableEmitters(PreviewComet* comet);
WorldServicesResult previewCometRender(PreviewComet* comet,float parameter,PreviewCometMesh* mesh);
WorldServicesResult previewCometRenderFrame(PreviewComet* comet,float parameter,
    const CarCinematicScreen* screen,NativeEcstasyFrame* frame);

/* ZonePreviewCameraMover PAL161F98..16417C. Native names retain the
 * existing caller ABI; the original owner now includes its visible comet. */
typedef struct CarCrashCamera CarCrashCamera;

typedef struct CarCrashCameraContext {
    CarCinematicServices* services;
    struct WorldSimulation* simulation;
    TerrainTopology* terrain;
    struct CarFramePlayerState* player;
    int32_t viewNumber;
} CarCrashCameraContext;

int32_t carCrashCameraPrepare(CarRoadPreview* preview,const CarMovement* movement);
CarCrashCamera* carCrashCameraCreate(const CarCrashCameraContext* context,
                                      CarRoadPreview* preview,CarCinematicResult* result);
void carCrashCameraDispose(CarCrashCamera* camera);
float carCrashCameraDuration(const CarCrashCamera* camera);
void carCrashCameraActivate(CarCrashCamera* camera);
void carCrashCameraDeactivate(CarCrashCamera* camera);
CarCinematicResult carCrashCameraAdvance(CarCrashCamera* camera);
const CarCinematicView* carCrashCameraView(const CarCrashCamera* camera);


struct CarCrashCamera {
    CarCinematicBase base;
    tCubic3D entryEye;
    tCubic3D entryTarget;
    tCubic3D bridgeEye;
    tCubic3D bridgeTarget;
    tCubic3D exitEye;
    tCubic3D exitTarget;
    tCubic1D zoom;
    float elapsed;
    float initialFovTangent;
    float forwardDuration;
    float reverseDuration;
    float pauseCountdown;
    float markers[4];
    Spline ground;
    Spline avoidance;
    Spline eye;
    Spline target;
    CarCameraParameterMap selectionMap;
    CarCameraParameterMap fullMap;
    TerrainSurface surface;
    float previousEye[4];
    float previousTarget[4];
    PreviewComet* comet;
};

CarCinematicResult carCrashCameraRender(CarCrashCamera* camera,
    const CarCinematicScreen* screen,NativeEcstasyFrame* frame);

struct WorldSimulation;
struct PursuitCollisionEntry;


float carCameraApproachScalar(float value, float target, float maximumChange);
void carCameraLimitPointHeight(const Spline* ground, Spline* camera,
                                int32_t index, float maximumHeight);
void carCameraFlattenEndHeight(const Spline* ground, Spline* camera, int32_t index);
int32_t carCameraIsHeightMinimum(const float* heights, int32_t count, int32_t index);
/* Samples must remain finite, count >= 2 and multiplier must produce at
 * least two temporary samples. Special parameters are an increasing list
 * of at most four entries, matching the source stack's authored contract. */
int32_t carCameraResampleHorizontal(Spline* curve, float multiplier,
                                    const float* specialParameters, int32_t specialCount);

void carCameraLimitMaximumSlope(Spline* curve, float gradient);
/* Retained PAL1655C0 helper; individual Beta identity remains unresolved. */
void carCameraRaiseMinimumSlope(Spline* curve, float gradient);
int32_t carCameraConstrainTerrain(TerrainSurface* surface, float first[4], float second[4],
                                   int32_t bothDirections, int32_t twoSided);
int32_t carCameraPathIntersectsTerrain(Spline* path, TerrainTopology* topology);
int32_t carCameraBuildTerrainCurves(CarRoadPreview* preview, TerrainSurface* surface,
                                    Spline* ground, Spline* eye, Spline* target,
                                    const CarCameraParameterMap* selection);

typedef struct CarCameraClusterPoint {
    struct CarCameraClusterPoint* next;
    float position[4];
} CarCameraClusterPoint;

typedef struct CarCameraCluster {
    struct CarCameraCluster* next;
    CarCameraClusterPoint* first;
    CarCameraClusterPoint* last;
} CarCameraCluster;

typedef struct CarCameraClusters {
    CarCameraCluster* first;
    CarCameraCluster* last;
} CarCameraClusters;

typedef struct CarCameraAvoidanceExclusions {
    const struct PursuitCollisionEntry* entries[6];
    int32_t count;
} CarCameraAvoidanceExclusions;

void carCameraClusterMean(const CarCameraCluster* cluster,float output[4]);
/* Return one for acceptance, zero for geometric rejection and minus one
 * for native allocation failure. A query-only miss still creates a cluster. */
int32_t carCameraClusterTryAppend(CarCameraCluster* cluster,const float point[4],int32_t append);
int32_t carCameraClusterAddPoint(CarCameraClusters* clusters,const float point[4],int32_t append);
void carCameraClusterDispose(CarCameraCluster* cluster);
void carCameraClustersDispose(CarCameraClusters* clusters);
void carCameraAvoidanceExclude(CarCameraAvoidanceExclusions* exclusions,const struct PursuitCollisionEntry* entry);
int32_t carCameraAvoidanceExcluded(const CarCameraAvoidanceExclusions* exclusions,const struct PursuitCollisionEntry* entry);


/* Curves and the terrain candidate cache are real initialized
 * owners. Output starts empty, marker capacity is at least maximumMarkers,
 * and maximumMarkers is between zero and four. Return minus one for native
 * allocation/binding failure, otherwise the number of authored path changes. */
int32_t carCameraBuildAvoidancePath(const struct WorldSimulation* simulation,
    TerrainSurface* surface,const Spline* search,const Spline* original,Spline* output,
    const CarCameraParameterMap* inverseMap,const CarCameraParameterMap* selectionMap,
    float* markers,int32_t maximumMarkers);


void carCameraLiftNearGround(TerrainSurface* surface, float point[4], float clearance);


#ifdef __cplusplus
}
#endif
