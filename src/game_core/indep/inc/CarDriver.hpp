#ifndef NFSHP2_INDEP_INC_CARDRIVER_HPP
#define NFSHP2_INDEP_INC_CARDRIVER_HPP

#include <stddef.h>
#include <stdint.h>
#include "indep/inc/Table.hpp"

/* Native logical input values. This is host-owned state, not a console
 * protocol packet or an asserted original C++ object layout. The profile
 * selects authored action mappings; it does not identify a PS2 device. */
#ifdef __cplusplus
extern "C" {
#endif

enum { JOYSTICK_PORT_COUNT = 2, JOYSTICK_SAMPLES = 32,
    JOYSTICK_PROFILE_DISCONNECTED = -1, JOYSTICK_PROFILE_GAMEPAD = 0x79 };

typedef struct JoystickControls {
    uint16_t held;                 /* One logical bit means pressed. */
    uint8_t sticks[4];             /* Right X/Y, left X/Y; centre 128. */
    uint8_t pressures[12];         /* Authored pressure-channel order. */
    int32_t mappingProfile;        /* Authored scanner profile, or -1. */
} JoystickControls;

typedef struct JoystickSample {
    JoystickControls controls;
    int32_t controllerType;
} JoystickSample;

typedef int32_t (*JoystickReader)(int32_t port, JoystickControls* controls);

typedef struct JoystickSystem {
    JoystickSample samples[JOYSTICK_PORT_COUNT][JOYSTICK_SAMPLES];
    int32_t top;
    int32_t bottom;
    int32_t preparedTop;
    int32_t initialized;
    int32_t readInVblank;
    int32_t portState[JOYSTICK_PORT_COUNT];
    JoystickReader read;
} JoystickSystem;

void joystickInitialize(JoystickSystem* joystick, JoystickReader reader);
void joystickShutdown(JoystickSystem* joystick);
void joystickSetReader(JoystickSystem* joystick, JoystickReader reader);
void joystickSetVblankReading(JoystickSystem* joystick, int enabled);
void joystickReadVblank(JoystickSystem* joystick);
void joystickPrepareFrame(JoystickSystem* joystick);
int32_t joystickHasFrame(const JoystickSystem* joystick);
void joystickAdvanceFrame(JoystickSystem* joystick);
const JoystickControls* joystickCurrentData(const JoystickSystem* joystick, int32_t port,
                                    int32_t* controllerType, const JoystickControls** previousData);
/* Translate an authored scanner field into logical native control values.
 * Only the original table field widths (1/8/16 bits) are used. */
uint32_t joystickReadMappedControl(const JoystickControls* controls,uint8_t field,uint32_t invert,uint32_t mask);
int32_t joystickPadType(const JoystickSample* sample);
int32_t joystickSupportsPressureMode(uint8_t type);

#ifdef __cplusplus
}
#endif

typedef enum CarDriverKind {
    CAR_DRIVER_NULL,
    CAR_DRIVER_REAL,
    CAR_DRIVER_AI,
    CAR_DRIVER_TRAFFIC,
    CAR_DRIVER_COP
} CarDriverKind;

typedef struct CarControls {
    uint16_t steeringAngle;
    uint16_t padding;
    float throttle;
    float brake;
    float handbrake;
    int32_t gearRequest;
    int32_t shiftTarget;
    uint32_t nitroRequest;
} CarControls;

typedef struct Car Car;
typedef struct World World;

typedef struct DriverBehaviorProfile {
    float connectionCumulativeChances[3];
    float targetAcceptanceChance;
    float roadSpeedScale;
    float playerPerformanceLimit;
    uint8_t speedSamplePointRange;
} DriverBehaviorProfile;

typedef struct DriverBehavior {
    DriverBehaviorProfile profiles[10];
    CurveKey positionResponse[7];
    CurveKey speedResponse[7];
    float rankMinimumResponse[15];
    float rankMaximumResponse[15];
    float paceAboveOneGains[8][2];
    float paceBelowOneGains[8][2];
    float speedOffsetIntervalSeconds;
    float performanceIntervalSeconds;
    float maximumPaceDistance;
    float roadSpeedScaleNegativeTwo;
    float roadSpeedScaleNegativeOne;
    /* Authored34B00+2C4. Group index is (policeDriverCount<1), then the
     * live opponent difficulty0..2. These are already cumulative values. */
    float opponentProfileCumulativeChances[2][3][10];
    /* Final34B00+288: actual StartRace difficulty/class lookup. */
    float playerPaceContributionByDifficulty[3][5];
} DriverBehavior;



#ifdef __cplusplus
namespace hp2 { class EasterEggs; class CopsOnModeSelect; class CopsOffModeSelect; struct CarSelector; class JoystickCalibrationScreen; class ReplayScreen; }
typedef hp2::EasterEggs GameEasterEggs;
typedef hp2::CopsOnModeSelect GameCopsOnModeSelect;
typedef hp2::CopsOffModeSelect GameCopsOffModeSelect;
typedef hp2::CarSelector GameCarSelector;
typedef hp2::JoystickCalibrationScreen GameJoystickCalibration;
typedef hp2::ReplayScreen GameReplayScreen;
extern "C" {
#else
typedef struct GameEasterEggs GameEasterEggs;
typedef struct GameCopsOnModeSelect GameCopsOnModeSelect;
typedef struct GameCopsOffModeSelect GameCopsOffModeSelect;
typedef struct GameCarSelector GameCarSelector;
typedef struct GameJoystickCalibration GameJoystickCalibration;
typedef struct GameReplayScreen GameReplayScreen;
#endif

typedef struct RealController RealController;
typedef struct AIDriver AIDriver;
typedef struct TrafficDriver TrafficDriver;
typedef struct CopDriver CopDriver;
struct CarMovementDriver;
struct CarRecoveryGroup;
struct CarRecoveryPose;
struct CarDriverEnvironment;
struct DriverCommand;
struct NullController;



typedef enum CarDriverResult {
    CAR_DRIVER_OK,
    CAR_DRIVER_INVALID_BINDING,
    CAR_DRIVER_MISSING_ROAD,
    CAR_DRIVER_MISSING_INPUT,
    CAR_DRIVER_MISSING_PURSUIT,
    CAR_DRIVER_ALLOCATION_FAILURE,
    CAR_DRIVER_CAPACITY,
    CAR_DRIVER_MOVEMENT_FAILURE
} CarDriverResult;

/* The concrete owner retains its state and lifetime. The recovery view is
 * borrowed from that same owner, including its current authored road pose. */
typedef struct CarDriver {
    CarDriverKind kind;
    union {
        RealController* real;
        AIDriver* ai;
        TrafficDriver* traffic;
        CopDriver* cop;
    } owner;
    struct CarMovementDriver* movementView;
    CarDriverResult result;
    const struct CarDriverEnvironment* environment;
    int32_t ownsOwner;
    struct NullController* nullOwner;
} CarDriver;

/* These host dispatch/binding operations are native integration helpers,
 * not additional functions attributed to the original executable. */
CarDriverResult carDriverThink(CarDriver* driver, float elapsed);
CarDriverResult carDriverAttach(CarDriver* driver, Car* car);
CarDriverResult carDriverSetInputEnabled(CarDriver* driver, int32_t enabled, int32_t port);
int32_t carDriverHornActive(const CarDriver* driver);
CarDriverResult carDriverSetHorn(CarDriver* driver, int32_t enabled);
CarDriverResult carDriverOnCarPlaced(CarDriver* driver);
CarDriverResult carDriverClearSteeringMemory(CarDriver* driver);
CarDriverResult carDriverSelectResetGear(CarDriver* driver);
CarDriverResult carDriverResetOnRoad(CarDriver* driver,const struct CarRecoveryGroup* group,
                                    const struct CarRecoveryPose* pose);
const struct CarRecoveryGroup* carDriverCurrentGroup(const CarDriver* driver, int32_t direction);
const struct CarRecoveryPose* carDriverCurrentPose(const CarDriver* driver, int32_t direction);
float carDriverRaceSpeedMetric(const CarDriver* driver);
float carDriverLateralExcess(const CarDriver* driver);
const struct DriverCommand* carDriverCommand(const CarDriver* driver);
struct DriverCommand* carDriverMutableCommand(CarDriver* driver);


struct CarMovement;
struct ReplaySnapshot;
void carDriverInputSnapshot(struct CarMovement* movement,struct DriverCommand* command,struct ReplaySnapshot* snapshot);
struct DriverPhysicsVehicle;
struct DriverAIEnvironment;
struct RealDriverBindings;
struct GameTime;
struct CarSoundState;
/* The per-car factory borrows the world's existing services and immutable
 * vehicle configuration. The dispatch record keeps its address when a race
 * finish or replay transition replaces its concrete controller. */
typedef struct CarDriverEnvironment {
    struct CarMovement* movement;
    const struct DriverPhysicsVehicle* physics;
    struct DriverAIEnvironment* ai;
    const struct RealDriverBindings* real;
    struct PursuitCollisionContext* pursuit;
    const struct GameTime* clock;
    struct CarSoundState* sound;
} CarDriverEnvironment;

CarDriverResult carDriverChange(CarDriver* driver,Car* car,CarDriverKind kind);
void carDriverDispose(CarDriver* driver);


struct RealDriver;
typedef enum RealDriverRadialBlurOperation {
    REAL_DRIVER_RADIAL_BLUR_START,
    REAL_DRIVER_RADIAL_BLUR_AMOUNTS,
    REAL_DRIVER_RADIAL_BLUR_STOP
} RealDriverRadialBlurOperation;

/* Concrete adapter to the existing player screen-effect owner;0 ELF bodies. */
void realDriverRadialBlur(struct RealDriver* driver, RealDriverRadialBlurOperation operation, float amount);

void carControlsSetNitroRequest(CarControls* control, uint32_t value);
void carControlsSetHandbrake(CarControls* control, float handbrake);
void carControlsSetSteeringAngle(CarControls* control, uint16_t angle);
void carControlsSetBrake(CarControls* control, float brake);
void carControlsSetThrottle(CarControls* control, float throttle);
void carControlsSetShiftTarget(CarControls* control, int32_t value);
void carControlsSetGearRequest(CarControls* control, int32_t value);
uint32_t carControlsGetNitroRequest(const CarControls* control);
int32_t carControlsGetShiftTarget(const CarControls* control);
int32_t carControlsGetGearRequest(const CarControls* control);
float carControlsGetHandbrake(const CarControls* control);
float carControlsGetBrake(const CarControls* control);
float carControlsGetThrottle(const CarControls* control);
uint16_t carControlsGetSteeringAngle(const CarControls* control);

/* Borrowed native input targets and ordered host registration adapter.
 * No original PS2 callback/layout representation is retained. */
struct RealDriver;
struct CarFramePlayerState;
struct CarPlayerFixedCamera;
struct CarPlayerOrbitCamera;
struct TrackRouteEditorInput;
struct FlailerFrontendInput;

typedef enum GameInputOwnerKind {
    GAME_INPUT_REAL_DRIVER,
    GAME_INPUT_PLAYER,
    GAME_INPUT_PLAYER_ROUTE,
    GAME_INPUT_PLAYER_MENU,
    GAME_INPUT_PLAYER_RACE,
    GAME_INPUT_PLAYER_CAMERA,
    GAME_INPUT_FIXED_CAMERA,
    GAME_INPUT_ORBIT_CAMERA,
    GAME_INPUT_TRACK_ROUTE_EDITOR,
    GAME_INPUT_FRONTEND,
    GAME_INPUT_EASTER_EGGS,
    GAME_INPUT_COPS_ON_MODE_SELECT,
    GAME_INPUT_COPS_OFF_MODE_SELECT,
    GAME_INPUT_CAR_SELECTOR,
    GAME_INPUT_RACE_COORDINATOR,
    GAME_INPUT_TYPE_MONITOR,
    GAME_INPUT_JOYSTICK_CALIBRATION,
    GAME_INPUT_REPLAY_SCREEN
} GameInputOwnerKind;

typedef struct GameInputTarget {
    GameInputOwnerKind kind;
    union {
        struct RealDriver* real;
        struct CarFramePlayerState* player;
        struct CarPlayerFixedCamera* fixedCamera;
        struct CarPlayerOrbitCamera* orbitCamera;
        struct TrackRouteEditorInput* editorInput;
        struct FlailerFrontendInput* frontend;
        GameEasterEggs* easterEggs;
        GameCopsOnModeSelect* copsOnModeSelect;
        GameCopsOffModeSelect* copsOffModeSelect;
        GameCarSelector* carSelector;
        struct GameSession* raceCoordinator;
        struct GameInputRouter* typeMonitor;
        GameJoystickCalibration* joystickCalibration;
        GameReplayScreen* replayScreen;
    } owner;
} GameInputTarget;

typedef struct GameInputRegistration {
    int32_t device;
    int32_t event;
    GameInputTarget target;
    uint64_t order;
    /* EventNode's actual scanner cache/state. The callback target above is
     * still the single borrowed gameplay owner. */
    int32_t scannerConfiguration;
    uint32_t scannerValue;
    int32_t scannerInitial;
    int32_t scannerPhase;
} GameInputRegistration;

typedef enum GameInputResult {
    GAME_INPUT_OK,
    GAME_INPUT_INVALID_BINDING,
    GAME_INPUT_CAPACITY,
    GAME_INPUT_OWNER_FAILURE
} GameInputResult;

/* Pending output words belong to the input owner. These are requested values,
 * not an acknowledgement from a Windows device. Source11D3D8 writes six motor
 * bytes and a dirty word; source12C240 clears the two selected wheel words. */
typedef struct GameInputRumbler {
    const KeyedCurve* envelope;
    float phase;
    float amplitude;
    int32_t value;
    int32_t previousValue;
} GameInputRumbler;
typedef struct GameInputForceWheel {
    int32_t port;
    uint32_t forceWord6C;
    uint32_t forceWord70;
} GameInputForceWheel;

typedef struct GameInputCalibration {
    CurveKey keys[4];
    uint8_t hysteresis;
} GameInputCalibration;

typedef struct GameInputPortScanner {
    int32_t currentType;
    int32_t pendingType;
    int32_t stableSamples;
    GameInputCalibration calibration[4];
} GameInputPortScanner;

typedef struct GameInputQueuedEvent {
    uint16_t packedEvent;
    uint8_t port;
    uint8_t registeredEvent;
    GameInputTarget target;
    uint8_t active;
} GameInputQueuedEvent;

/* Native opt-in debug focus is distinct from both physical device packets.
 * It is transient UI state, never part of a REC or a car snapshot. */
typedef struct NativeDebugInputFocus {
    int32_t active;
    int32_t startPending;
    int32_t chordLatched;
    int32_t waitNeutral;
    int32_t initialized;
    int32_t originalCameraKind;
    int32_t predictedCameraKind;
    uint32_t resetEpoch;
    struct World* context;
    JoystickControls previousNormal;
    JoystickControls previousTools;
} NativeDebugInputFocus;

/* Storage and concrete input owners are borrowed. Registration order and
 * first-match removal are retained at the host input-device boundary. */
typedef struct GameInputRouter {
    GameInputRegistration* registrations;
    size_t capacity;
    size_t count;
    uint64_t nextOrder;
    GameInputResult result;
    int32_t dispatching;
    int32_t dispatchDevice;
    int32_t dispatchEvent;
    int32_t dispatchCancelled;
    /* Source persistent monitor AA60 is distinct from the sampled device
     * packets. TypeChanged runs before storing the newly observed type. */
    uint8_t monitoredTypes[2];
    int32_t userConfigurations[2];
    uint8_t pendingMotorBytes[2][6];
    int32_t pendingMotorDirty[2];
    GameInputForceWheel forceWheels[2];
    GameInputRumbler rumblers[2][2];
    struct World* world;
    /* Original Joystick+BC/C0/C4 and QueuedJoystickEvents are distinct from
     * CurrentJoystickTypes. Source type monitoring consumes queued events. */
    GameInputPortScanner scanners[2];
    GameInputQueuedEvent queuedEvents[256];
    uint32_t queuedEventCount;
    int32_t generatingEvents;
    NativeDebugInputFocus nativeDebug;
} GameInputRouter;

// UI thread hooks. A reset is issued on focus loss or actual selected-device
// replacement. The caption borrows static text; nullptr means no active mode.
void joystickNotifyNativeInputReset(int32_t port);
const char* joystickNativeDebugCaption();

void gameInputRouterInitialize(GameInputRouter* router,GameInputRegistration* storage,size_t capacity);
GameInputResult gameInputRegister(GameInputRouter* router,int32_t device,int32_t event,GameInputTarget target);
int32_t gameInputUnregister(GameInputRouter* router,int32_t device,int32_t event);
GameInputResult gameInputDispatch(GameInputRouter* router,int32_t device,int32_t event,uint8_t value);
GameInputResult joystickInitializeTypeMonitoring(GameInputRouter* router);
void joystickCloseTypeMonitoring(GameInputRouter* router);
struct ReplaySnapshot;
void joystickDoSnapshot(GameInputRouter* router, struct ReplaySnapshot* snapshot);
GameInputResult joystickResetTypeHandlers(GameInputRouter* router);
GameInputResult joystickTypeChanged(GameInputRouter* router,int32_t port,uint8_t type);
int32_t joystickIsPluggedIn(const GameInputRouter* router,int32_t port);
uint8_t joystickGetType(const GameInputRouter* router,int32_t port);
void joystickSetUserConfiguration(GameInputRouter* router,int32_t port,int32_t configuration);
int32_t joystickGetUserConfiguration(const GameInputRouter* router,int32_t port);
int32_t joystickDetermineAvailableConfigurations(int32_t* output,int32_t capacity,int32_t type);
int32_t joystickChooseConfiguration(int32_t type,int32_t requested);
uint32_t joystickGetButtonChar(const GameInputRouter* router,int32_t port,int32_t event);
const char* joystickGetButtonName(const GameInputRouter* router,int32_t port,int32_t event);
void joystickGetAndShiftButtonChar(const GameInputRouter* router,int32_t port,int32_t event,
    uint32_t* button,uint32_t* shift);
int32_t joystickCanBeCalibrated(uint8_t type);
int32_t joystickHasStiffThrottle(const GameInputRouter* router,int32_t port);
int32_t joystickIsTypeWheel(const GameInputRouter* router,int32_t port);
void joystickSetRumble(GameInputRouter* router,int32_t port,uint8_t smallMotor,uint8_t largeMotor);
int32_t joystickGetWheelId(const GameInputRouter* router,int32_t port);
void joystickPortStopForce(GameInputRouter* router,int32_t port);

/* Native wheel dispatch follows the original missing-wheel return. A handled
 * missing device is distinct from an actual hardware acknowledgement. */
typedef enum GameInputForceDispatchResult {
    GAME_INPUT_FORCE_INVALID_REQUEST,
    GAME_INPUT_FORCE_NO_DEVICE,
    GAME_INPUT_FORCE_DEVICE_UNSUPPORTED
} GameInputForceDispatchResult;
struct CarFramePresentationEvent;
GameInputForceDispatchResult joystickDispatchForceFeedback(const GameInputRouter* router,
    const struct CarFramePresentationEvent* request);
/* PAL12C728 PortPlayCrash: absence of a registered wheel returns before
 * touching force state. This result never acknowledges physical playback. */
GameInputForceDispatchResult joystickPortCollisionForce(const GameInputRouter* router,
    int32_t port,int32_t duration,float strength,float direction);

void joystickSetCalibration(GameInputRouter* router,int32_t port,int32_t axis,uint8_t minimum,uint8_t maximum);
void joystickGetCalibration(const GameInputRouter* router,int32_t port,int32_t axis,uint8_t* minimum,uint8_t* maximum);
uint8_t joystickGetDeadzone(const GameInputRouter* router,int32_t port,int32_t axis);



#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
struct JoystickSystem;
struct ReplayIntegerLog;
struct ReplayManager;
struct GameTime;
class DemoDiscManager;
// Full released GenerateJoyEvents: physical samples -> source scanners ->
// Joylog channel1 -> Replay channel0 -> retained typed callback queue.
GameInputResult joystickGenerateEvents(GameInputRouter& input,JoystickSystem& samples,
    ReplayIntegerLog& log,ReplayManager* replay,const GameTime& clock,DemoDiscManager& demo);
#endif


// Character resource declarations use this observed CarDriver header as a
// native arrangement only. Bodies remain in proven characterinfo.cpp; no
// original inline placement or beta object layout is asserted.
#ifdef __cplusplus
extern "C" {
#endif

typedef struct CarDriverAppearance {
    char name[24];
    uint32_t modelKeys[3];
    uint32_t materialKeys[2];
    int32_t variantCounts[2];
    uint32_t textureKeys[2][16];
} CarDriverAppearance;

typedef struct CarDriverCharacter {
    int32_t id;
    char name[32];
    char appearanceName[24];
    int32_t variants[2];
} CarDriverCharacter;

typedef struct CarDriverAppearanceCatalogue {
    const CarDriverAppearance* appearances;
    size_t appearanceCount;
    const CarDriverCharacter* characters;
    size_t characterCount;
} CarDriverAppearanceCatalogue;

/* Decoded arrays borrow their storage from the retained resource owner.
 * Authored payload size retains the source chunk-count division separately
 * from the platform's typed record sizes. */
typedef struct CarDriverAppearanceChunk {
    uint32_t kind;
    uint32_t authoredPayloadSize;
    union {
        const CarDriverAppearance* appearances;
        const CarDriverCharacter* characters;
    } records;
    size_t availableRecords;
} CarDriverAppearanceChunk;

int32_t carDriverAppearanceLoad(CarDriverAppearanceCatalogue* catalogue,const CarDriverAppearanceChunk* chunk);
int32_t carDriverAppearanceUnload(CarDriverAppearanceCatalogue* catalogue,uint32_t kind);

const CarDriverAppearance* carDriverAppearanceFind(const CarDriverAppearanceCatalogue* catalogue,const char* name);
const CarDriverCharacter* carDriverCharacterFind(const CarDriverAppearanceCatalogue* catalogue,const char* name);
const CarDriverCharacter* carDriverCharacterFindKey(const CarDriverAppearanceCatalogue* catalogue,uint32_t key);
int32_t carDriverCharacterCount(const CarDriverAppearanceCatalogue* catalogue);
const CarDriverCharacter* carDriverCharacterAt(const CarDriverAppearanceCatalogue* catalogue,int32_t index);

#ifdef __cplusplus
}
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

#include <array>
#include <cstdint>

namespace hp2 {
// Logical action ordering retained from the authored input maps.
enum class Ps2Button : std::uint8_t {
    Select, L3, R3, Start, Up, Right, Down, Left,
    L2, R2, L1, R1, Triangle, Circle, Cross, Square
};

struct ControllerState {
    std::uint16_t held = 0;               // 1 bit means pressed
    std::array<std::uint8_t, 4> sticks = {128,128,128,128}; // RX, RY, LX, LY
    std::array<std::uint8_t, 12> pressures{};
    bool connected = false;
};

JoystickControls NativeScannerControls(const ControllerState& state) noexcept;
void SetButton(ControllerState& state, Ps2Button button, bool pressed,
               std::uint8_t pressure = 255) noexcept;
std::uint8_t NormalizeAxis(std::int16_t signedValue) noexcept;

// PC device preferences feed typed logical samples to the authored action
// scanners. They never replace a Player input map, replay command or AI owner.
// The first16 keys use Ps2Button order; the last4 are analog steering and
// menu aliases. Values are Win32 virtual keys, zero means unbound.
enum class NativeDeviceMode : std::uint8_t {
    Automatic,Keyboard,Gamepad,KeyboardAndGamepad,Disabled
};
enum class NativeBindingKind : std::uint8_t {
    None,Button,AxisPositive,AxisNegative,Hat,AxisFullPositive,AxisFullNegative
};
struct NativeInputBinding {
    NativeBindingKind kind{NativeBindingKind::None};
    std::int32_t index{};
    std::int32_t direction{}; // SDL hat bit: Up1,Right2,Down4,Left8.
};
struct NativeGamepadProfile {
    // Logical buttons retain the original game's 16-action ordering.
    // Physical indices use SDL2's standard layout, or the selected raw HID.
    std::array<NativeInputBinding,16> buttons{{
        {NativeBindingKind::Button,4,0},{NativeBindingKind::Button,7,0},
        {NativeBindingKind::Button,8,0},{NativeBindingKind::Button,6,0},
        {NativeBindingKind::Button,11,0},{NativeBindingKind::Button,14,0},
        {NativeBindingKind::Button,12,0},{NativeBindingKind::Button,13,0},
        {NativeBindingKind::AxisPositive,4,0},{NativeBindingKind::AxisPositive,5,0},
        {NativeBindingKind::Button,9,0},{NativeBindingKind::Button,10,0},
        {NativeBindingKind::Button,3,0},{NativeBindingKind::Button,1,0},
        {NativeBindingKind::Button,0,0},{NativeBindingKind::Button,2,0}}};
    std::array<std::int32_t,4> axes{{2,3,0,1}}; // Right X/Y, left X/Y; -1 disables.
    std::array<bool,4> invert{};
    std::array<std::int32_t,4> deadzone{{-1,-1,-1,-1}}; // -1 inherits the legacy setting.
    std::array<std::int32_t,4> saturation{{32767,32767,32767,32767}};
    std::int32_t triggerThreshold{-1}; // -1 inherits; otherwise 0..229 in pressure units.
    std::int32_t vibrationScale{100}; // Percent of game-requested motor strength.
};
struct NativeInputSettings {
    std::array<std::uint16_t,20> keys{{
        0x09,0,'N',0x1b,0x26,0x27,0x28,0x25,
        'Q','E',0xa0,0x20,'C','H','W','S','A','D',0x0d,0x08}};
    std::array<std::int32_t,2> controller{{0,1}}; // Legacy index only when no stable key is saved.
    std::int32_t leftDeadzone{7849},rightDeadzone{8689},triggerThreshold{};
    bool vibration{true},invertSteering{},invertLookY{},swapSticks{};
    std::array<std::uint16_t,20> player2Keys{{
        0x6d,0,'O',0x6b,0x68,0x66,0x62,0x64,
        0x67,0x69,0x6f,0x60,0x65,0x6e,'I','K','J','L',0x0d,0x08}};
    std::array<NativeDeviceMode,2> deviceMode{{NativeDeviceMode::Automatic,NativeDeviceMode::Automatic}};
    std::array<std::string,2> deviceKey{}; // Persistent GUID+serial/path identity; empty means automatic.
    std::array<NativeGamepadProfile,2> profiles{};
};
struct NativePadSample {
    std::uint16_t buttons{}; // XInput button layout, not the DS2 layout.
    std::int16_t leftX{},leftY{},rightX{},rightY{};
    std::uint8_t leftTrigger{},rightTrigger{};
    bool connected{};
    std::array<std::int16_t,16> rawAxes{};
    std::array<std::uint8_t,64> rawButtons{};
    std::array<std::uint8_t,4> hats{};
    std::int32_t axisCount{},buttonCount{},hatCount{};
    bool rawValid{},standardized{};
    std::int32_t instanceId{-1}; // Live SDL connection identity; never persisted as selection.
};
struct NativeInputDeviceInfo {
    std::string id,name,guid;
    std::int32_t index{-1},instanceId{-1},axes{},buttons{},hats{};
    bool standardized{};
    std::string pathKey; // Unambiguous physical-path alias, also valid when serial alone is unique.
};
bool NativeKeyboardEnabled(const NativeInputSettings& settings,std::int32_t port) noexcept;
bool NativeGamepadEnabled(const NativeInputSettings& settings,std::int32_t port) noexcept;
const std::array<std::uint16_t,20>& NativeKeyboardBindings(const NativeInputSettings& settings,std::int32_t port);
bool DetectNativeBinding(const NativePadSample& current,const NativePadSample& baseline,
    NativeInputBinding& binding,bool axisOnly=false) noexcept;
std::string NativeBindingName(const NativeInputBinding& binding,bool standardized=true);
const char* NativeInputActionName(std::size_t action) noexcept;
std::int32_t ResolveNativeDevice(const std::vector<NativeInputDeviceInfo>& devices,
    const std::string& key,std::int32_t fallbackIndex) noexcept;
std::int32_t ResolveNativePlayerDevice(const NativeInputSettings& settings,
    const std::vector<NativeInputDeviceInfo>& devices,std::int32_t port) noexcept;
// Stable SDL2 GameController snapshots are converted to native logical
// controller values. Controller buttons 0..14 and axes 0..5 are SDL2's public
// GameController enums, not console hardware registers.
struct Sdl2GamepadSnapshot {
    std::array<std::uint8_t,15> buttons{};
    std::array<std::int16_t,6> axes{};
    bool connected{};
};
NativePadSample ConvertSdl2Gamepad(const Sdl2GamepadSnapshot& snapshot) noexcept;
void ValidateNativeInputSettings(const NativeInputSettings& settings);
// Fold old global axis choices into both profiles without changing response.
void MigrateNativeInputPreferences(NativeInputSettings& settings);
// Explicit PC device assignment, selected by the native configurator. The
// keyboard remains a real independent player-one device; connection of the
// selected gamepad is still determined by the host read, never by this choice.
void AssignNativeKeyboardAndGamepad(NativeInputSettings& settings,std::int32_t gamepad);
void AssignNativePlayerOneGamepad(NativeInputSettings& settings,std::int32_t gamepad);
enum class NativeCheatTextResult { Unhandled,Pending,Complete,Cancelled };
// PC keyboard transport only. Holding Alt while typing HP2Cnn separates text
// capture from ordinary mapped pad keys; the original EasterEgg gates remain.
class NativeCheatTextInput final {
public:
    NativeCheatTextResult Character(std::uint32_t character,bool alt,bool control,
        bool focused,bool enabled,std::array<char,6>& command) noexcept;
    bool FilterKeyboard(std::array<bool,256>& keys,bool alt,bool control,
        bool focused,bool enabled) noexcept;
    void Cancel() noexcept;
    bool Capturing() const noexcept {return capturing_;}
private:
    std::array<char,6> text_{};
    std::uint8_t length_{};
    bool capturing_{};
};
ControllerState NativeInputState(const NativeInputSettings& settings,std::int32_t port,
    const NativePadSample& pad,const std::array<bool,256>& keys,bool focused);
}

namespace hp2 {
class CarDriverAppearanceResources final {
public:
    explicit CarDriverAppearanceResources(const std::vector<std::uint8_t>& bundle);
    CarDriverAppearanceResources(const CarDriverAppearanceResources&)=delete;
    CarDriverAppearanceResources& operator=(const CarDriverAppearanceResources&)=delete;
    const CarDriverAppearanceCatalogue& Catalogue() const noexcept { return catalogue_; }
private:
    std::vector<CarDriverAppearance> appearances_;
    std::vector<CarDriverCharacter> characters_;
    CarDriverAppearanceCatalogue catalogue_{};
};
}

#endif

#endif

#ifdef __cplusplus
struct GameTime;
struct GameSession;
struct CarFramePlayerRegistry;
struct RaceFrame;
struct RaceParameters;
namespace hp2 {struct ApplicationFrontendFlow;struct FrontendPlayerManager;}
void joystickRumblerStart(GameInputRumbler& rumbler,const KeyedCurve& envelope,float amplitude);
bool joystickRumblerUpdate(GameInputRumbler& rumbler,float elapsed);
void joystickUpdatePortRumblers(GameInputRouter& input,const GameTime& clock);
void joystickPortRumble(GameInputRouter& input,int32_t port,float magnitude,
    const float* coefficients,const KeyedCurve& envelope,bool special,
    const hp2::ApplicationFrontendFlow& flow,const GameSession* coordinator,
    const hp2::FrontendPlayerManager& frontend,const CarFramePlayerRegistry& players);
void joystickPortRumbleImpact(GameInputRouter& input,int32_t port,float magnitude,
    const hp2::ApplicationFrontendFlow& flow,const GameSession* coordinator,
    const hp2::FrontendPlayerManager& frontend,const CarFramePlayerRegistry& players);
void joystickPortRumbleSurface(GameInputRouter& input,int32_t port,int32_t code,float magnitude,
    const hp2::ApplicationFrontendFlow& flow,const GameSession* coordinator,
    const hp2::FrontendPlayerManager& frontend,const CarFramePlayerRegistry& players);
struct CarCameraShake;
void carCameraMaybeShake(CarCameraShake& shake,const CarFramePlayerRegistry& players,
    int32_t port,const float acceleration[4]);
void SetAIDriverModesFromPlayers(World* world,const RaceFrame* race,
    const RaceParameters& parameters,const CarFramePlayerRegistry& players);
#endif
