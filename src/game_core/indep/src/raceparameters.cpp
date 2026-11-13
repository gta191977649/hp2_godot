#include "indep/inc/RaceParameters.hpp"
#include "indep/inc/Fred.hpp"
#include "CarInfo.hpp"
#include "bware/inc/bWare.hpp"
#include "indep/inc/RaceCoordinator.hpp"
#include "bware/inc/bMath.hpp"
#include "indep/inc/Replay.hpp"

#include <cstring>
#include <cstdio>
#include "bware/inc/bWare.hpp"
using hp2::Require;

GlobalDisplayStartupOptions globalDisplayStartupOptions{1,1,1,1};

void globalDisplaySettingsResetToDefaults(GlobalDisplaySettings* settings,
    const ApplicationLaunchOptions* options,const int32_t* language)
{
    Require(settings && options && language,"Display reset has no canonical launch/language owners");
    settings->authoredOption0=options->skipFrontend ? globalDisplayStartupOptions.option0 : 1;
    settings->languageDependentOption=options->skipFrontend ?
        globalDisplayStartupOptions.languageDependentOption : static_cast<uint8_t>(*language!=0);
    settings->singleViewTrackMapMode=0;settings->splitViewTrackMapMode=7;
    settings->pursuitTrackMapMode=8;settings->leaderboardMode=2;
    settings->authoredOption6=options->skipFrontend ? globalDisplayStartupOptions.option6 : 1;
    settings->authoredOption7=options->skipFrontend ? globalDisplayStartupOptions.option7 : 1;
}

void raceParametersInitializeDefaults(RaceParameters* parameters,
    const ApplicationLaunchOptions* options,const int32_t* language)
{
    Require(parameters && parameters->drivers && parameters->runtimeSettings && options && language,
        "Race reset has no retained driver/settings/application bindings");
    auto* drivers=parameters->drivers;auto* runtime=parameters->runtimeSettings;
    // Native pointers replace the source's global driver array and embedded
    // bank. Preserve those bindings while clearing their actual values.
    *parameters={};*runtime={};
    std::memset(drivers,0,sizeof(RaceDriverInfo)*RACE_DRIVER_LIMIT);
    parameters->drivers=drivers;parameters->runtimeSettings=runtime;
    parameters->selectedScenario=11;parameters->authoredTrackOptionSecond=3;
    parameters->opposingTrafficProbability=0.5f;
    parameters->playerStartingPositions[0]=7;parameters->playerStartingPositions[1]=8;
    parameters->pulloverLimit=2;parameters->relativePaceEnabled=1;
    parameters->playerPaceContribution[1]=1.0f;parameters->trafficDensity=1.0f;
    parameters->totalLaps=2;parameters->mode=1;parameters->opponentCount=7;
    parameters->authoredRaceOption254=1;
    parameters->pulloversEnabled=1;parameters->roadBlocksEnabled=1;
    parameters->authoredPursuitOption268=1;parameters->authoredPursuitOption26C=1;
    parameters->chopperEnabled=1;parameters->resetPursuitTimerAfterPullover=1;
    parameters->playerPaceContribution[0]=1.0f;
    playerSettingsResetToDefaults(&runtime->players[0],options);
    playerSettingsResetToDefaults(&runtime->players[1],options);
    globalDisplaySettingsResetToDefaults(&runtime->display,options,language);
    parameters->eventRandomSeed=vectorDefaultRandomBounded(UINT32_MAX);
}

void playerSettingsSnapshot(PlayerSettings* settings,ReplaySnapshot* snapshot)
{
    Require(settings && snapshot,"Player-settings snapshot has no actual owners");
    replaySnapshotBeginSection(snapshot,nullptr,"cPlayerSettings");
    replaySnapshotMoveData(snapshot,&settings->initialVehicleMode,1,1,1);
    replaySnapshotMoveData(snapshot,&settings->gearInputMode,1,1,1);
    replaySnapshotMoveData(snapshot,&settings->authoredTimeDisplayOption,1,1,1);
    replaySnapshotEndSection(snapshot);
}

void raceParametersSnapshot(RaceParameters* parameters,ReplaySnapshot* snapshot,
    const CarDriverAppearanceCatalogue* characters)
{
    Require(parameters && snapshot && parameters->drivers && parameters->runtimeSettings && characters,
        "Race snapshot has no retained driver/settings/character bindings");
    auto& race=*parameters;auto& runtime=*race.runtimeSettings;
    replaySnapshotBeginSection(snapshot,nullptr,"RaceParameters");
    replaySnapshotMoveInteger(snapshot,&race.selectedScenario,1);
    replaySnapshotMoveData(snapshot,&race.authoredTrackOptionFirst,1,-1,0);
    replaySnapshotMoveData(snapshot,&race.authoredTrackOptionSecond,1,-1,0);
    replaySnapshotMoveData(snapshot,&race.reverse,1,-1,0);
    replaySnapshotMoveFloat(snapshot,&race.trafficDensity);
    replaySnapshotMoveFloat(snapshot,&race.opposingTrafficProbability);
    replaySnapshotMoveData(snapshot,&race.raceKind,1,-1,0);
    replaySnapshotMoveData(snapshot,&race.timeLimitTicks,4,-1,0);
    replaySnapshotMoveInteger(snapshot,&race.pointToPoint,1);
    replaySnapshotMoveInteger(snapshot,&race.totalLaps,1);
    replaySnapshotMoveInteger(snapshot,&race.mode,1);
    replaySnapshotMoveInteger(snapshot,&race.opponentCount,1);
    replaySnapshotMoveData(snapshot,&race.opponentVehicleClass,1,-1,0);
    replaySnapshotMoveData(snapshot,&race.playerStartingPositions[0],1,1,1);
    replaySnapshotMoveData(snapshot,&race.playerStartingPositions[1],1,1,1);
    replaySnapshotMoveData(snapshot,&race.authoredRaceOption40,1,4,0);
    replaySnapshotMoveData(snapshot,&runtime.display.authoredOption6,1,1,1);
    replaySnapshotMoveData(snapshot,&runtime.display.authoredOption7,1,1,1);
    replaySnapshotMoveData(snapshot,&race.authoredRaceOption4C,1,4,0);
    // PAL deliberately transfers this same float's first byte twice.
    replaySnapshotMoveData(snapshot,&race.opposingTrafficProbability,1,-1,0);
    replaySnapshotMoveData(snapshot,&race.eventSelected,1,4,0);
    replaySnapshotMoveData(snapshot,&race.authoredRaceOption254,1,-1,0);
    replaySnapshotMoveInteger(snapshot,&race.policeDriverCount,1);
    replaySnapshotMoveInteger(snapshot,&race.pulloversEnabled,1);
    replaySnapshotMoveInteger(snapshot,&race.roadBlocksEnabled,1);
    replaySnapshotMoveInteger(snapshot,&race.authoredPursuitOption268,1);
    replaySnapshotMoveInteger(snapshot,&race.authoredPursuitOption26C,1);
    replaySnapshotMoveInteger(snapshot,&race.chopperEnabled,1);
    replaySnapshotMoveInteger(snapshot,&race.specialPursuit,1);
    replaySnapshotMoveInteger(snapshot,&race.resetPursuitTimerAfterPullover,1);
    replaySnapshotMoveInteger(snapshot,&race.pulloverLimit,1);
    replaySnapshotMoveInteger(snapshot,&race.initialRoadBlockRequests,1);
    replaySnapshotMoveInteger(snapshot,&race.initialChopperRequests,1);
    replaySnapshotMoveData(snapshot,&race.pursuitDifficulty,1,-1,0);
    playerSettingsSnapshot(&runtime.players[0],snapshot);
    playerSettingsSnapshot(&runtime.players[1],snapshot);
    replaySnapshotMoveData(snapshot,&race.opponentDifficulty,1,-1,0);
    replaySnapshotMoveInteger(snapshot,&race.relativePaceEnabled,1);
    replaySnapshotMoveFloat(snapshot,&race.playerPaceContribution[0]);
    replaySnapshotMoveFloat(snapshot,&race.playerPaceContribution[1]);
    replaySnapshotMoveInteger(snapshot,&race.driverCount,4);
    Require(race.driverCount>=0 && race.driverCount<=RACE_DRIVER_LIMIT,
        "Replay driver count exceeds its actual retained array");
    for (int32_t index=0;index<race.driverCount;++index)
        hp2::SnapshotDriverInfo(race.drivers[index],*snapshot,*characters);
    replaySnapshotEndSection(snapshot);
}

void playerSettingsResetToDefaults(PlayerSettings* settings,const ApplicationLaunchOptions* options)
{
    settings->authoredOption=1;
    settings->inputProfile=0;settings->initialVehicleMode=0;settings->gearInputMode=0;
    const auto view=options->skipFrontend ? options->startupCameraView : std::uint8_t{1};
    settings->alternateCamera=0;settings->lookback=1;
    settings->splitTimeMode=0;settings->authoredTimeDisplayOption=0;
    settings->cameraSpring=0;settings->cameraView=static_cast<std::int8_t>(view);settings->hudFlags=0x355;
    // Source strcpy writes seven bytes, retaining any existing name tail.
    std::memcpy(settings->playerName,"PLAYER",7);
    settings->scoring=1;settings->hudHotkey=1;
    settings->bestTotalTimeTicks=0;settings->bestLapTimeTicks=0;
}

namespace hp2 {
bool AreThereTooManyUniqueCarGeometries(const RaceParameters& parameters,
    const CarAppearanceSelection& candidate,const CarInfoResource& cars)
{
    std::int32_t unique=0;
    if (parameters.driverCount<0)return false;
    // The source deliberately visits the still-existing slot at driverCount
    // to inspect its eliminated byte before selecting the candidate argument.
    Require(parameters.drivers && parameters.driverCount<RACE_DRIVER_LIMIT,
        "Geometry selection requires the actual trailing driver slot");
    for (std::int32_t index=0;index<=parameters.driverCount;++index) {
        const auto& slot=parameters.drivers[index];
        if (slot.eliminated>0)continue;
        const auto& ride=index<parameters.driverCount ? slot.appearance : candidate;
        const auto& car=cars.Car(static_cast<std::size_t>(ride.modelCode));
        if (car.bodyCategory==2)continue;
        if (ride.reserved) { ++unique;continue; }
        std::int32_t previous=0;
        for (;previous<index;++previous) {
            const auto& other=parameters.drivers[previous];
            if (other.eliminated>0 || other.appearance.reserved)continue;
            const auto& definition=cars.Car(static_cast<std::size_t>(other.appearance.modelCode));
            if (std::strcmp(car.geometryCompressedPath.c_str(),definition.geometryCompressedPath.c_str())==0)break;
        }
        if (previous==index)++unique;
    }
    return unique>=6;
}

void AddDriverInfo(RaceParameters& parameters,const RaceDriverInfo& driver,
    const CarInfoResource& cars)
{
    if (parameters.driverCount>=RACE_DRIVER_LIMIT)return;
    Require(parameters.driverCount>=0 && parameters.drivers,
        "Driver append has no canonical race-driver storage");
    // The original lookup has no writes and its returned skin is unused.
    (void)cars.FindSkin(driver.appearance.modelCode,driver.appearance.skinNumber,true);
    const auto index=parameters.driverCount;
    parameters.drivers[index]=driver;
    parameters.driverCount=index+1;
}
} // namespace hp2

namespace {
void AddCounter(int32_t& counter, uint32_t amount)
{
    const uint32_t bits = static_cast<uint32_t>(counter) + amount;
    std::memcpy(&counter, &bits, sizeof(counter));
}
}

RaceDriverInfo* raceParametersGetDriverInfo(RaceParameters* parameters, int32_t index)
{
    if (index < 0 || index >= RACE_DRIVER_LIMIT) return nullptr;
    return &parameters->drivers[index];
}

RaceDriverInfo* raceParametersGetDriverInfoByDriverNumber(RaceParameters* parameters, int32_t driverNumber)
{
    for (int32_t index = 0; index < parameters->driverCount; ++index) {
        RaceDriverInfo* driver = raceParametersGetDriverInfo(parameters, index);
        if (driver->driverNumber == driverNumber) return driver;
    }
    return nullptr;
}

RaceDriverInfo* raceParametersEliminateDriver(RaceParameters* parameters, int32_t driverNumber, int32_t reason)
{
    RaceDriverInfo* driver = raceParametersGetDriverInfoByDriverNumber(parameters, driverNumber);
    if (driver && driver->eliminated <= 0) {
        driver->eliminated = static_cast<int8_t>(reason);
        if (driver->driverClass == 1) AddCounter(parameters->mode, UINT32_MAX);
        else if (driver->driverClass == 2) AddCounter(parameters->opponentCount, UINT32_MAX);
    }
    return driver;
}

void raceParametersClearEliminatedDrivers(RaceParameters* parameters)
{
    for (int32_t index = 0; index < parameters->driverCount; ++index) {
        RaceDriverInfo& driver = parameters->drivers[index];
        if (driver.eliminated <= 0) continue;
        if (driver.driverClass == 1) AddCounter(parameters->mode, 1);
        else if (driver.driverClass == 2) AddCounter(parameters->opponentCount, 1);
        driver.eliminated = 0;
    }
}

void raceParametersResetStartingPositions(RaceParameters* parameters)
{
    // PAL1EDD18: restore the two actual human slots, then reset every
    // retained driver. AI starting positions are recalculated by FRED.
    parameters->playerStartingPositions[0]=static_cast<int8_t>(parameters->opponentCount+1);
    parameters->playerStartingPositions[1]=static_cast<int8_t>(parameters->opponentCount+2);
    for(int32_t i=0;i<parameters->driverCount;++i) {
        auto* driver=raceParametersGetDriverInfo(parameters,i);
        const auto number=driver->driverNumber;
        driver->startingPosition=number>=0 && number<2?parameters->playerStartingPositions[number]:0;
    }
}

int32_t raceParametersGetNumEliminated(const RaceParameters* parameters)
{
    int32_t count = 0;
    for (int32_t index = 0; index < parameters->driverCount; ++index) {
        if (parameters->drivers[index].eliminated > 0) ++count;
    }
    return count;
}

int32_t raceParametersIsFreeRun(const RaceParameters* parameters)
{
    return parameters->raceKind == 1 && parameters->totalLaps == 0 &&
           parameters->specialPursuit == 0;
}

void playerSettingsToggleScoring(PlayerSettings* settings)
{
    settings->scoring = settings->scoring == 0;
}

int32_t globalDisplaySettingsCycleLeaderboardMode(GlobalDisplaySettings* settings, int32_t direction)
{
    (void)direction;
    const int8_t previous = settings->leaderboardMode;
    settings->leaderboardMode = previous == 2 ? 0 : 2;
    return settings->leaderboardMode != previous;
}

void playerSettingsToggleGauges(PlayerSettings* settings)
{
    settings->hudFlags ^= PLAYER_HUD_GAUGES;
}

void playerSettingsToggleHudHotkey(PlayerSettings* settings)
{
    settings->hudHotkey = settings->hudHotkey == 0;
}

void playerSettingsToggleLapInfo(PlayerSettings* settings)
{
    settings->hudFlags ^= PLAYER_HUD_LAP_INFO;
}

void playerSettingsCycleSplitTime(PlayerSettings* settings, int32_t direction)
{
    const uint32_t sum = static_cast<uint32_t>(settings->splitTimeMode) +
                         static_cast<uint32_t>(direction);
    int32_t next;
    std::memcpy(&next, &sum, sizeof(next));
    if (next < 0) next = 4;
    if (next >= 5) next = 0;
    settings->splitTimeMode = static_cast<int8_t>(next);
    if (next == 4) settings->hudFlags &= ~static_cast<uint32_t>(PLAYER_HUD_SPLIT_TIME);
    else settings->hudFlags |= PLAYER_HUD_SPLIT_TIME;
}

void playerSettingsToggleRacePosition(PlayerSettings* settings)
{
    settings->hudFlags ^= PLAYER_HUD_RACE_POSITION;
}

void playerSettingsToggleLookback(PlayerSettings* settings)
{
    if (settings->lookback == 0) settings->lookback = 1;
    else if (settings->lookback == 1) settings->lookback = 0;
}

void playerSettingsCycleAlternateCamera(PlayerSettings* settings)
{
    const int32_t next = static_cast<int32_t>(settings->alternateCamera) + 1;
    settings->alternateCamera = static_cast<int8_t>(next % 2);
}

void playerSettingsToggleCameraSpring(PlayerSettings* settings)
{
    settings->cameraSpring = settings->cameraSpring == 0;
}

/* Exact mutable final tables3140A0/331090/331098. Their final-only direct
 * selection helpers are a fully decoded closure of CycleTrackMap1EE148;
 * no beta name/path or accepted function is claimed for those helpers. */
int32_t singlePlayerTrackMapModes[6]={0,1,2,3,4,6};
int32_t splitPlayerTrackMapModes[2]={7,6};
int32_t pursuitTrackMapModes[2]={8,6};
namespace {
int32_t CycleAuthoredTrackMapMode(int32_t value,int32_t direction,const int32_t* modes,int32_t count)
{
    int32_t index=-1;
    for (int32_t candidate=0;candidate<count;++candidate)
        if (modes[candidate]==value) {index=candidate;break;}
    if (index==-1)return modes[0];
    index=static_cast<int32_t>(static_cast<uint32_t>(index)+static_cast<uint32_t>(direction));
    if (index>=count)index=0;
    else if (index<0)index=count-1;
    return modes[index];
}
}
int32_t globalDisplaySettingsCycleTrackMapMode(GlobalDisplaySettings* settings,int32_t direction,
    const RaceParameters* race,const int32_t* applicationState,const int32_t* playerCount)
{
    int8_t* value;
    const int32_t* modes;
    int32_t count;
    if (race->specialPursuit && *applicationState!=3) {
        value=&settings->pursuitTrackMapMode;modes=pursuitTrackMapModes;count=2;
    } else if (*applicationState==3 || *playerCount==1) {
        value=&settings->singleViewTrackMapMode;modes=singlePlayerTrackMapModes;count=6;
    } else {value=&settings->splitViewTrackMapMode;modes=splitPlayerTrackMapModes;count=2;}
    const int8_t previous=*value;
    *value=static_cast<int8_t>(CycleAuthoredTrackMapMode(previous,direction,modes,count));
    return *value!=previous;
}

hp2::LoadableGaugeTextures hp2::ChooseLoadableTextures(const CarDefinition& car,int32_t units)
{
    Require(units==0 || units==1,"Gauge units are outside the original MPH/KMH domain");
    LoadableGaugeTextures result;
    char name[128];
    std::snprintf(name,sizeof(name),"3RDPERSON_%d",convertFloatToInt32(car.gauges.genericMaximum));
    result.generic=hashString(name);
    std::string base=car.name;
    if (base=="LAMBOCOP") base="MURCIELAGO";
    else {
        std::size_t suffix=0;
        if (base.size()>=3 && base.compare(base.size()-3,3,"COP")==0) suffix=3;
        if (base.size()>=2 && base.compare(base.size()-2,2,"HP")==0) suffix=2;
        base.resize(base.size()-suffix);
    }
    std::snprintf(name,sizeof(name),units==1 ? "%s_KMH" : "%s_MPH",base.c_str());
    result.speedometer=hashString(name);
    if (car.gauges.combined) {
        result.tachometer=0;
        std::snprintf(name,sizeof(name),"OVERLAY_%s",base.c_str());
    } else {
        std::snprintf(name,sizeof(name),"%s_TACH",base.c_str());
        result.tachometer=hashString(name);
        std::snprintf(name,sizeof(name),"OVERLAY_GENERIC");
    }
    result.units=hashString(name);
    return result;
}
