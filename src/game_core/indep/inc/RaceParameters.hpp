#ifndef NFSHP2_RACE_PARAMETERS_HPP
#define NFSHP2_RACE_PARAMETERS_HPP

#include <stdint.h>

struct CarDriverCharacter;

#ifdef __cplusplus
extern "C" {
#endif

enum { RACE_DRIVER_LIMIT = 14 };

typedef enum PlayerHudFlag {
    PLAYER_HUD_SPLIT_TIME = 0x004,
    PLAYER_HUD_RACE_POSITION = 0x010,
    PLAYER_HUD_GAUGES = 0x040,
    PLAYER_HUD_LAP_INFO = 0x100
} PlayerHudFlag;

/* Mutable profile settings shared by the options screens and the HUD.
 * This is a native representation, not a serialized save-game structure. */
typedef struct PlayerSettings {
    int8_t alternateCamera;
    int8_t splitTimeMode;
    int32_t cameraSpring;
    int32_t lookback;
    uint32_t hudFlags;
    int32_t hudHotkey;
    int32_t scoring;
    /* Remaining logical values of the same cPlayerSettings owner. Their
     * native positions do not reproduce the console's packed record. */
    int8_t inputProfile;
    uint8_t authoredOption;
    int8_t initialVehicleMode;
    int8_t gearInputMode;
    int8_t cameraView;
    uint8_t authoredTimeDisplayOption;
    char playerName[12];
    int32_t bestTotalTimeTicks;
    int32_t bestLapTimeTicks;
} PlayerSettings;

typedef struct GlobalDisplaySettings {
    int8_t leaderboardMode;
    uint8_t authoredOption0;
    uint8_t languageDependentOption;
    int8_t singleViewTrackMapMode;
    int8_t splitViewTrackMapMode;
    int8_t pursuitTrackMapMode;
    uint8_t authoredOption6;
    uint8_t authoredOption7;
} GlobalDisplaySettings;

/* Four independent launch/tweaker cells read by the display reset. Their
 * native owner contains the actual mutable values, each initially one. */
typedef struct GlobalDisplayStartupOptions {
    uint8_t option0;
    uint8_t languageDependentOption;
    uint8_t option6;
    uint8_t option7;
} GlobalDisplayStartupOptions;

/* The two runtime settings values and display settings are the actual
 * RaceParameters bank. Profiles copy values into it at race launch; live
 * players then take their intentional independent settings copy. */
typedef struct RaceRuntimeSettings {
    PlayerSettings players[2];
    GlobalDisplaySettings display;
} RaceRuntimeSettings;

/* Driver selection belongs to race setup, separately from live RaceCarEntry
 * progress. Other selection metadata can remain with its resource owner. */
typedef struct CarAppearanceSelection {
    int32_t modelCode;
    int8_t skinNumber;
    int8_t dashboardTextures;
    uint8_t variant;
    uint8_t reserved;
} CarAppearanceSelection;

typedef struct RaceDriverInfo {
    int8_t eliminated;
    int8_t driverNumber;
    int32_t driverClass;
    CarAppearanceSelection appearance;
    int8_t characterIndex;
    int8_t behaviorProfile;
    int8_t startingPosition;
    const struct CarDriverCharacter* character;
} RaceDriverInfo;

typedef struct RaceParameters {
    /* Shared selected-race configuration. This prefix was previously exposed
     * as RaceSettings; all consumers now borrow this same live object. */
    int32_t selectedScenario;
    int32_t reverse;
    int32_t raceKind;
    int32_t pointToPoint;
    int32_t totalLaps;
    int32_t mode;
    int32_t opponentCount;
    int32_t specialPursuit;
    int32_t timeLimitTicks;
    int32_t relativePaceEnabled;
    float playerPaceContribution[2];
    RaceDriverInfo* drivers;
    int32_t driverCount;
    float trafficDensity;
    float opposingTrafficProbability;
    /* Source+18 and+278. These are independent live race settings, not
     * application SkipFE options and not copies of session state. */
    int32_t fastSpeedTrapSpawns;
    int32_t resetPursuitTimerAfterPullover;
    int32_t pulloverLimit;
    /* Independent packed EventTrack selectors, source +04/+08. They are
     * decoded as two-bit authored values; no weather/name alias is assumed. */
    int32_t authoredTrackOptionFirst;
    int32_t authoredTrackOptionSecond;
    /* Authored pursuit configuration. The manager copies its two initial
     * allowances and clamped difficulty once, then owns their live state. */
    int32_t pulloversEnabled;
    int32_t roadBlocksEnabled;
    int32_t authoredPursuitOption268;
    int32_t authoredPursuitOption26C;
    int32_t chopperEnabled;
    int32_t initialRoadBlockRequests;
    int32_t initialChopperRequests;
    int32_t pursuitDifficulty;
    /* Shared EventTree::SetRace outputs. Native storage is independent of
     * the original packed offsets; the text belongs to this same owner. */
    int32_t eventSelected;
    int32_t policeDriverCount;
    int32_t opponentVehicleClass;
    int32_t authoredRaceOption4C;
    int32_t authoredRaceOption40;
    /* Independent source +44/+48 words set by FRED::StartDemoDiscGame.
     * They are not the separate display option bytes at +2F2/+2F3. */
    int32_t authoredRaceOption44;
    int32_t authoredRaceOption48;
    int8_t playerStartingPositions[2];
    uint32_t eventRandomSeed;
    char eventDescription[513];
    /* Source +2F4, independent of pursuitDifficulty(+288). */
    int32_t opponentDifficulty;
    /* Native stable storage for source embedded +28C/+2BC/+2EC. The same
     * instance must be supplied by FrontendRaceSetupBindings. */
    RaceRuntimeSettings* runtimeSettings;
    int32_t demoReplay;
    int32_t authoredRaceOption254;
    int32_t eventGame;
    int32_t unlockedRewardCarCount;
} RaceParameters;

RaceDriverInfo* raceParametersGetDriverInfo(RaceParameters* parameters, int32_t index);
RaceDriverInfo* raceParametersGetDriverInfoByDriverNumber(RaceParameters* parameters, int32_t driverNumber);
RaceDriverInfo* raceParametersEliminateDriver(RaceParameters* parameters, int32_t driverNumber, int32_t reason);
void raceParametersClearEliminatedDrivers(RaceParameters* parameters);
void raceParametersResetStartingPositions(RaceParameters* parameters);
int32_t raceParametersGetNumEliminated(const RaceParameters* parameters);
int32_t raceParametersIsFreeRun(const RaceParameters* parameters);

void playerSettingsToggleScoring(PlayerSettings* settings);
struct ApplicationLaunchOptions;
void playerSettingsResetToDefaults(PlayerSettings* settings,const struct ApplicationLaunchOptions* options);
struct ReplaySnapshot;
struct CarDriverAppearanceCatalogue;
extern GlobalDisplayStartupOptions globalDisplayStartupOptions;
void globalDisplaySettingsResetToDefaults(GlobalDisplaySettings* settings,
    const struct ApplicationLaunchOptions* options,const int32_t* language);
void raceParametersInitializeDefaults(RaceParameters* parameters,
    const struct ApplicationLaunchOptions* options,const int32_t* language);
void playerSettingsSnapshot(PlayerSettings* settings,struct ReplaySnapshot* snapshot);
void raceParametersSnapshot(RaceParameters* parameters,struct ReplaySnapshot* snapshot,
    const struct CarDriverAppearanceCatalogue* characters);
int32_t globalDisplaySettingsCycleLeaderboardMode(GlobalDisplaySettings* settings, int32_t direction);
/* The context values are borrowed live source owners, not copied options. */
int32_t globalDisplaySettingsCycleTrackMapMode(GlobalDisplaySettings* settings,int32_t direction,
    const RaceParameters* race,const int32_t* applicationState,const int32_t* playerCount);
extern int32_t singlePlayerTrackMapModes[6];
extern int32_t splitPlayerTrackMapModes[2];
extern int32_t pursuitTrackMapModes[2];
void playerSettingsToggleGauges(PlayerSettings* settings);
void playerSettingsToggleHudHotkey(PlayerSettings* settings);
void playerSettingsToggleLapInfo(PlayerSettings* settings);
void playerSettingsCycleSplitTime(PlayerSettings* settings, int32_t direction);
void playerSettingsToggleRacePosition(PlayerSettings* settings);
void playerSettingsToggleLookback(PlayerSettings* settings);
void playerSettingsCycleAlternateCamera(PlayerSettings* settings);
void playerSettingsToggleCameraSpring(PlayerSettings* settings);

#ifdef __cplusplus
}
namespace hp2 {
struct CarDefinition;
struct LoadableGaugeTextures {
    uint32_t speedometer{};
    uint32_t tachometer{};
    uint32_t units{};
    uint32_t generic{};
};
// Complete final PAL1EE3C0; the catalogue record and global display option
// are supplied from their existing owners instead of console globals.
LoadableGaugeTextures ChooseLoadableTextures(const CarDefinition& car,int32_t units);
}
#endif

#endif
