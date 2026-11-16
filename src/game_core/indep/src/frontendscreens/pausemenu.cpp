#include "PauseMenu.hpp"
#include "subtitle_menu_row.hpp"
#include "RaceCoordinator.hpp"
#include "Replay.hpp"
#include "Language.hpp"
#include "bsound/include/bSound.h"
#include "DemoDisc.hpp"
#include "CarController.hpp"
#include "CarDamage.hpp"
#include "TrackedController.hpp"
#include "indep/inc/CarDriver.hpp"
#include "RealDriver.hpp"
#include "Race.hpp"
#include <cstdio>
#include "Fred.hpp"
#include "Player.hpp"
#include "RaceParameters.hpp"
#include "GameFlow.hpp"
#include "Flags.hpp"
#include "bware/inc/bWare.hpp"
#include <cmath>
#include <limits>

namespace hp2 {
namespace {
constexpr std::uint32_t pauseSubtitleRow=0xaee77886u; // Port-side MENUITEM_13.
constexpr std::uint32_t hudHotkeyRow=0xaee77885u;
constexpr std::uint32_t hudHotkeyValue=0x4aff0df3u;
constexpr std::uint32_t lookbackRow=0xaee77884u;
constexpr std::pair<std::uint32_t,std::uint32_t> pauseDisplayRows[]{
    {0x52e039f3u,0x5b472b16u}, {0x52e039f4u,0x2e931f1bu}, {0x52e039f5u,0x55cb09f2u},
    {0x52e039f6u,0x71a78ef4u}, {0x52e039f7u,0x8b788a68u}, {0x52e039f8u,0xe8434eb4u},
    {0x52e039f9u,0x5d6c141bu}, {0x52e039fau,0x12b1ebd9u}, {0x52e039fbu,0x5d2a9432u},
    {0xaee77883u,0xffc3dcd2u}, {lookbackRow,0x2e018745u}, {hudHotkeyRow,hudHotkeyValue},
    {pauseSubtitleRow,SubtitleMenuRow::ValueKey}
};

FlailerUiState& PauseUi(RaceScreenServices& services)
{
    Require(services.frontend && services.frontend->ui && services.frontend->ui->resources,
        "Pause display has no actual frontend resource owner");
    return *services.frontend->ui;
}
RaceParameters& PauseParameters(const RaceScreenServices& services)
{
    Require(services.frontend && services.frontend->raceSetup && services.frontend->raceSetup->parameters,
        "Pause display has no actual RaceParameters owner");
    return *services.frontend->raceSetup->parameters;
}
GlobalDisplaySettings& DisplaySettings(const RaceScreenServices& services)
{
    auto& parameters=PauseParameters(services);
    Require(parameters.runtimeSettings,"Pause display has no actual runtime settings bank");
    return parameters.runtimeSettings->display;
}
void SetDisplayControl(RaceScreenServices& services,std::uint32_t screen,
    std::uint32_t item,std::uint32_t value,bool enabled)
{
    // Full boolean branch of existing ScreenList enable-item16D198. The
    // menu nodes and rendered resources are the same retained owners.
    auto& resources=*PauseUi(services).resources;
    if (services.frontend->screens) {
        auto* node=services.frontend->screens->FindScreen(static_cast<std::int32_t>(screen));
        if (node && node->screen && node->screen->menu) {
            if (auto* entry=node->screen->menu->FindItem(item)) {
                if (enabled) {entry->SetMenuItemState(1);resources.SetObjectVisibility(screen,item,1);}
                else entry->ClearMenuItemState(1);
            }
        }
    }
    resources.SetObjectVisibility(screen,value,enabled);
}
bool IsTimedNonPursuit(const RaceParameters& parameters)
{
    const bool finiteTime=parameters.timeLimitTicks!=0 &&
        parameters.timeLimitTicks!=std::numeric_limits<std::int32_t>::max();
    return !parameters.specialPursuit && finiteTime &&
        (parameters.raceKind==2 || (parameters.pointToPoint && parameters.raceKind!=2));
}
}
PauseScreenBase::PauseScreenBase(RaceScreenServices& services,std::uint32_t screen)
    : services_(services),screen_(screen),
      highlight_(PauseUi(services),screen,0xAB671C9Au,PauseUi(services).animationSettings.highlightColor),
      valueHighlight_(PauseUi(services),screen,0x1042EAC3u,-1),
      arrows_(PauseUi(services),screen,0xD1CDA594u,0x9345E4A7u) {}
PauseScreenBase::~PauseScreenBase()=default;
void PauseScreenBase::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    // PAL17119C forwards the event before the arrows handler. Highlight4
    // selects the authored text and restarts its pulse; only Tick18 advances it.
    highlight_.HandleEvents(message,data);arrows_.HandleEvent(message,data);
}
CarFramePlayerState& PauseDisplayOpts::Player() const
{
    Require(player_,"Pause display port has no actual live Player");return *player_;
}
// Port-side scroll window over the authored row bands.
struct PauseDisplayOpts::SubtitleOptions {
    struct ClipState {
        FlailerTextChunk* text{};
        std::uint32_t clipKey{},format{};
    };
    static constexpr std::uint32_t clipKey=0x83ab872du;
    static constexpr std::uint32_t firstRow=pauseDisplayRows[0].first;

    FlailerResources& resources;
    SubtitleMenuRow option;
    FlailerBitmapChunk clip;
    FlailerScrollingMenuList scroll;
    std::array<ClipState,2*std::size(pauseDisplayRows)> originalClips{};
    FlailerObjectChunk& first;
    float originalFirstY{},pitch{};
    std::uint32_t selectedValue{};
    bool scrolled{};

    SubtitleOptions(MainMenuScreenServices& services,std::uint32_t screen)
        :resources(*services.ui->resources),
         option(services,screen,pauseSubtitleRow,hudHotkeyRow,hudHotkeyValue,
             resources.FindObject(screen,hudHotkeyRow,1)->topLeftY-
             resources.FindObject(screen,lookbackRow,1)->topLeftY),
         scroll(*services.ui,screen,firstRow,hudHotkeyRow,0,0,clipKey),
         first(*resources.FindObject(screen,firstRow,1)),originalFirstY(first.topLeftY),
         pitch(resources.FindObject(screen,pauseSubtitleRow,1)->topLeftY-
             resources.FindObject(screen,hudHotkeyRow,1)->topLeftY)
    {
        // Borrow an authored bitmap's dimensions for an invisible clip object.
        const auto* arrow=static_cast<FlailerBitmapChunk*>(resources.FindObject(screen,0xd1cda594u,2));
        Require(arrow && pitch>0,"Pause Display scrolling needs the authored arrow and row pitch");
        clip=*arrow;
        clip.key=clipKey;clip.visible=0;clip.strip=nullptr;
        clip.topLeftX=0;clip.topLeftY=originalFirstY-pitch*0.2f;
        const float height=resources.FindObject(screen,hudHotkeyRow,1)->topLeftY-originalFirstY+pitch;
        clip.scaleX*=640.0f/resources.ScaledBitmapWidth(clip);
        clip.scaleY*=height/resources.ScaledBitmapHeight(clip);
        auto& root=resources.FindScreen(screen)->objects;
        std::size_t index=0;
        for(const auto& [label,value]:pauseDisplayRows) {
            for(const auto key:{label,value}) {
                auto* text=static_cast<FlailerTextChunk*>(resources.FindObject(screen,key,1));
                Require(text,"Pause Display scrolling needs the authored labels and values");
                originalClips[index++]={text,text->clipObjectKey,text->formatOptions};
                scroll.AddObject(key);
            }
        }
        clip.previous=root.previous;clip.next=&root;
        clip.previous->next=&clip;root.previous=&clip;
    }

    ~SubtitleOptions()
    {
        clip.previous->next=clip.next;clip.next->previous=clip.previous;
        for(const auto& original:originalClips) {
            original.text->clipObjectKey=original.clipKey;
            original.text->formatOptions=original.format;
            original.text->updateKind=5;
        }
    }

    void ScrollTo(std::uint32_t selected)
    {
        if(selected!=pauseSubtitleRow && selected!=firstRow)return;
        const bool showLast=selected==pauseSubtitleRow;
        if(scrolled==showLast)return;
        scrolled=showLast;
        scroll.SetCurrentObject(selected);
    }

    bool Moving() const
    {
        const float target=originalFirstY-(scrolled?pitch:0.0f);
        return std::abs(first.topLeftY-target)>1.0f;
    }
};

PauseDisplayOpts::PauseDisplayOpts(RaceScreenServices& services,std::uint32_t screen,std::int32_t port)
    : PauseScreenBase(services,screen)
{
    Require(services.players,"Pause display has no actual Player registry");
    for (std::size_t index=0;index<2;++index) {
        auto* player=services.players->playersByNumber[index];
        if (player && player->movement.controllerPort==port) {
            player_=player;
            PauseUi(services).resources->SetLanguageID(screen,0x18B2DBC8u,index?0x334B6u:0x334B5u);
            break;
        }
    }
    (void)Player();
    DisplayRearview();DisplayCameraSpring();DisplayLookback();DisplayRacePosition();
    DisplaySplitTime();DisplayLapInfo();DisplayGauges();DisplayTrackmap();DisplayLeaderboard();
    DisplayScoring();DisplayHUD();DisplayHudHotKey();DoGrayout();AdjustLapInfo();
    if(services.frontend->subtitleSetting)
        subtitles_=std::make_unique<SubtitleOptions>(*services.frontend,screen);
}
PauseDisplayOpts::~PauseDisplayOpts() {player_=nullptr;}
bool PauseDisplayOpts::ShouldSeeRearViewMirror() const
{
    return services_.players->count==1 && Player().settings.inputProfile==0;
}
bool PauseDisplayOpts::ShouldSeeScoring() const
{
    return !raceParametersIsFreeRun(&PauseParameters(services_)) && Player().settings.inputProfile==0;
}
bool PauseDisplayOpts::ShouldSeeLeaderboard() const
{
    const auto& parameters=PauseParameters(services_);
    if (IsTimedNonPursuit(parameters) || raceParametersIsFreeRun(&parameters) || parameters.specialPursuit)return false;
    return Player().settings.inputProfile==0;
}
bool PauseDisplayOpts::ShouldSeePosition() const
{
    const auto& parameters=PauseParameters(services_);
    if (IsTimedNonPursuit(parameters) || parameters.specialPursuit || raceParametersIsFreeRun(&parameters))return false;
    return Player().settings.inputProfile==0;
}
bool PauseDisplayOpts::ShouldSeeSplitTime() const
{
    return !PauseParameters(services_).specialPursuit && Player().settings.inputProfile==0;
}
bool PauseDisplayOpts::ShouldSeeTrackMap() const {return Player().settings.inputProfile==0;}
bool PauseDisplayOpts::ShouldSeeLapInfo() const
{
    if (Player().settings.inputProfile!=0)return false;
    return !PauseParameters(services_).specialPursuit || Player().movement.seat!=0;
}
bool PauseDisplayOpts::ShouldSeeGauges() const {return Player().settings.inputProfile==0;}
void PauseDisplayOpts::AdjustLapInfo()
{
    // Both released branches select this exact ID; no invented cop label.
    PauseUi(services_).resources->SetLanguageID(screen_,0x52E039F7u,0x222F0);
}
void PauseDisplayOpts::DoGrayout()
{
    SetDisplayControl(services_,screen_,0x52E039F4u,0x2E931F1Bu,ShouldSeeRearViewMirror());
    SetDisplayControl(services_,screen_,0x52E039F5u,0x55CB09F2u,ShouldSeeTrackMap());
    SetDisplayControl(services_,screen_,0x52E039F6u,0x71A78EF4u,ShouldSeeLeaderboard());
    SetDisplayControl(services_,screen_,0x52E039F7u,0x8B788A68u,ShouldSeeLapInfo());
    SetDisplayControl(services_,screen_,0x52E039F8u,0xE8434EB4u,ShouldSeePosition());
    SetDisplayControl(services_,screen_,0x52E039F9u,0x5D6C141Bu,ShouldSeeGauges());
    SetDisplayControl(services_,screen_,0x52E039FAu,0x12B1EBD9u,ShouldSeeSplitTime());
    SetDisplayControl(services_,screen_,0x52E039FBu,0x5D2A9432u,ShouldSeeScoring());
}
void PauseDisplayOpts::DisplayTrackmap()
{
    const auto& display=DisplaySettings(services_);
    const auto mode=PauseParameters(services_).specialPursuit?display.pursuitTrackMapMode:
        services_.players->count==1?display.singleViewTrackMapMode:display.splitViewTrackMapMode;
    constexpr std::uint32_t ids[]{0x22345,0x22346,0x22347,0x223A8,0x22348,
        0x22349,0x22353,0x22348,0x223A9,0x223AA};
    PauseUi(services_).resources->SetLanguageID(screen_,0x55CB09F2u,mode>=0 && mode<10?ids[mode]:0);
}
bool PauseDisplayOpts::CycleTrackMap(std::int32_t direction)
{
    const auto* flow=PauseUi(services_).flow;
    Require(flow,"Track-map cycle has no actual application-flow owner");
    const bool changed=globalDisplaySettingsCycleTrackMapMode(&DisplaySettings(services_),direction,
        &PauseParameters(services_),&flow->currentState,&services_.players->count)!=0;
    DisplayTrackmap();return changed;
}
bool PauseDisplayOpts::ToggleHudHotKey()
{
    playerSettingsToggleHudHotkey(&Player().settings);DisplayHudHotKey();return true;
}
void PauseDisplayOpts::DisplayHudHotKey()
{
    PauseUi(services_).resources->SetLanguageID(screen_,0x4AFF0DF3u,Player().settings.hudHotkey?0x22352:0x22353);
}
bool PauseDisplayOpts::CycleHUD(std::int32_t direction)
{
    carFramePlayerCycleInputProfile(&Player(),direction);DisplayHUD();DoGrayout();return true;
}
void PauseDisplayOpts::DisplayHUD()
{
    constexpr std::uint32_t ids[]{0x33454,0x33456,0x33455,0x33469};
    const auto mode=Player().settings.inputProfile;
    PauseUi(services_).resources->SetLanguageID(screen_,0x5B472B16u,mode>=0 && mode<4?ids[mode]:0);
}
void PauseDisplayOpts::DisplayGauges()
{
    PauseUi(services_).resources->SetLanguageID(screen_,0x5D6C141Bu,
        Player().settings.hudFlags&PLAYER_HUD_GAUGES?0x22352:0x22353);
}
bool PauseDisplayOpts::ToggleGauges()
{
    const auto old=Player().settings.hudFlags&PLAYER_HUD_GAUGES;
    playerSettingsToggleGauges(&Player().settings);const auto now=Player().settings.hudFlags&PLAYER_HUD_GAUGES;
    DisplayGauges();return old!=now;
}
void PauseDisplayOpts::DisplayLapInfo()
{
    PauseUi(services_).resources->SetLanguageID(screen_,0x8B788A68u,
        Player().settings.hudFlags&PLAYER_HUD_LAP_INFO?0x22352:0x22353);
}
bool PauseDisplayOpts::ToggleLapInfo()
{
    const auto old=Player().settings.hudFlags&PLAYER_HUD_LAP_INFO;
    playerSettingsToggleLapInfo(&Player().settings);const auto now=Player().settings.hudFlags&PLAYER_HUD_LAP_INFO;
    DisplayLapInfo();return old!=now;
}
void PauseDisplayOpts::DisplaySplitTime()
{
    constexpr std::uint32_t ids[]{0x2234C,0x2234D,0x2234E,0x2234F,0x22353};
    const auto mode=Player().settings.splitTimeMode;
    PauseUi(services_).resources->SetLanguageID(screen_,0x12B1EBD9u,mode>=0 && mode<5?ids[mode]:0);
}
bool PauseDisplayOpts::CycleSplitTime(std::int32_t direction)
{
    const auto old=Player().settings.splitTimeMode;playerSettingsCycleSplitTime(&Player().settings,direction);
    const auto now=Player().settings.splitTimeMode;DisplaySplitTime();return old!=now;
}
void PauseDisplayOpts::DisplayRacePosition()
{
    PauseUi(services_).resources->SetLanguageID(screen_,0xE8434EB4u,
        Player().settings.hudFlags&PLAYER_HUD_RACE_POSITION?0x22352:0x22353);
}
bool PauseDisplayOpts::ToggleRacePosition()
{
    const auto old=Player().settings.hudFlags&PLAYER_HUD_RACE_POSITION;
    playerSettingsToggleRacePosition(&Player().settings);const auto now=Player().settings.hudFlags&PLAYER_HUD_RACE_POSITION;
    DisplayRacePosition();return old!=now;
}
void PauseDisplayOpts::DisplayLookback()
{
    const auto mode=Player().settings.lookback;
    PauseUi(services_).resources->SetLanguageID(screen_,0x2E018745u,mode==0?0x22356:mode==1?0x22351:0);
}
bool PauseDisplayOpts::ToggleLookback()
{
    playerSettingsToggleLookback(&Player().settings);
    carFramePlayerSetLookbackSpeed(&Player(),300.8999938964844f);DisplayLookback();return true;
}
void PauseDisplayOpts::DisplayCameraSpring()
{
    PauseUi(services_).resources->SetLanguageID(screen_,0xFFC3DCD2u,Player().settings.cameraSpring==1?0x22353:0x22352);
}
bool PauseDisplayOpts::ToggleCameraSpring()
{
    playerSettingsToggleCameraSpring(&Player().settings);
    carFramePlayerSetCameraSpring(&Player(),Player().settings.cameraSpring);DisplayCameraSpring();return true;
}
void PauseDisplayOpts::DisplayRearview()
{
    PauseUi(services_).resources->SetLanguageID(screen_,0x2E931F1Bu,DisplaySettings(services_).authoredOption0?0x22352:0x22353);
}
bool PauseDisplayOpts::ToggleRearView()
{
    auto& settings=DisplaySettings(services_);settings.authoredOption0=settings.authoredOption0==0;
    DisplayRearview();return true;
}
bool PauseDisplayOpts::CycleLeaderboard(std::int32_t direction)
{
    const bool changed=globalDisplaySettingsCycleLeaderboardMode(&DisplaySettings(services_),direction)!=0;
    DisplayLeaderboard();return changed;
}
void PauseDisplayOpts::DisplayLeaderboard()
{
    constexpr std::uint32_t ids[]{0x22353,0x18E71,0x22352};
    const auto mode=DisplaySettings(services_).leaderboardMode;
    Require(mode>=0 && mode<3,"Leaderboard mode lies outside the original display table");
    PauseUi(services_).resources->SetLanguageID(screen_,0x71A78EF4u,ids[mode]);
}
bool PauseDisplayOpts::CycleScoring(std::int32_t)
{
    playerSettingsToggleScoring(&Player().settings);DisplayScoring();return true;
}
void PauseDisplayOpts::DisplayScoring()
{
    PauseUi(services_).resources->SetLanguageID(screen_,0x5D2A9432u,Player().settings.scoring?0x22352:0x22353);
}
namespace {
PauseMenu* boundPause{};
std::unique_ptr<PauseDisplayOpts> currentPauseDisplay;
std::unique_ptr<PauseOptions> currentPauseOptions;
class PauseControlPages;
std::unique_ptr<PauseControlPages> currentPauseControls;
std::unique_ptr<PauseAudioOpts> currentPauseAudio;
std::unique_ptr<PauseRaceObjective> currentPauseObjective;
std::unique_ptr<PauseDebugMenu> currentPauseDebug;
std::int32_t pauseMenuCarToCarCollisionsEnabled=1; // PAL3306A4, retained debug-only cell.
std::int32_t displayOptionsPort{}; // Actual PAL3306F8; independent of Pause.optionsPort.
std::int32_t pauseObjectiveButtonChar=0x89; // Actual PAL3306B4, retained while unplugged.
constexpr const char* pauseNames[]{"Pause Menu","Pause Options","Pause audio","Pause display",
    "Debug Menu","Race Objective","Race Objective 2PBAC","Pause BAC controls"};
GameInputRouter& PauseInput(RaceScreenServices& services)
{
    Require(services.frontend && services.frontend->screens && services.frontend->screens->input &&
        services.frontend->screens->input->router,"Pause has no actual input owner");
    return *services.frontend->screens->input->router;
}
FlailerScreenList& PauseScreens(RaceScreenServices& services)
{
    Require(services.frontend && services.frontend->screens,"Pause has no actual ScreenList");
    return *services.frontend->screens;
}
void SetPauseItem(RaceScreenServices& services,std::uint32_t screen,std::uint32_t item,std::int32_t state)
{
    // Complete direct16D198 closure with the same retained screen/menu/item.
    auto& screens=PauseScreens(services);
    if (!screen && screens.active && screens.active->screen)
        screen=static_cast<std::uint32_t>(screens.active->screen->screenId);
    auto* node=screens.FindScreen(static_cast<std::int32_t>(screen));
    if (!node || !node->screen || !node->screen->menu)return;
    auto* entry=node->screen->menu->FindItem(item);
    if (!entry)return;
    if (state==0 || state==2)entry->ClearMenuItemState(1);
    else entry->SetMenuItemState(1);
    if (state==2)PauseUi(services).resources->SetObjectVisibility(screen,item,0);
    if (state==1)PauseUi(services).resources->SetObjectVisibility(screen,item,1);
}
void PauseVisibility(RaceScreenServices& services,std::uint32_t screen,std::int32_t hidden)
{
    auto& ui=PauseUi(services);Require(ui.render,"Pause has no actual render owner");
    ui.render->SetScreenVisibility(*ui.resources,screen,hidden);
}
void RequestUnpause(RaceScreenServices& services,std::int32_t reason)
{
    Require(services.coordinator,"Pause has no actual RaceCoordinator");
    Require(raceCoordinatorRequestUnPause(services.coordinator,reason)!=0,
        "RaceCoordinator queue rejected the original deferred unpause request");
}
std::int32_t ChooseWhoSelectedMenuItem(RaceScreenServices& services,const ScreenCallbackData& data)
{
    Require(services.players,"Pause selection has no actual player registry");
    if (services.players->count==2)return data.joystickPort;
    Require(services.players->players[0],"Pause selection has no first live Player");
    return services.players->players[0]->movement.controllerPort;
}
}
FlailerScreenConfig fPauseMenuScreen{"Pause Menu",PauseMenuCallback,
    {{static_cast<std::int32_t>(0x605D2BF1u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};
FlailerScreenConfig fPauseDisplayScreen{"Pause display",PauseDisplayCallback,
    {{static_cast<std::int32_t>(0xCC36FAB2u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};
void BindPauseMenu(PauseMenu& owner)
{
    Require(!boundPause || boundPause==&owner,"Another actual PauseMenu owner is already bound");boundPause=&owner;
}
void UnbindPauseMenu()
{
    Require(!currentPauseDisplay && !currentPauseAudio && !currentPauseObjective && !currentPauseOptions && !currentPauseControls && !currentPauseDebug,
        "Pause must deactivate its live submenus before unbinding");boundPause=nullptr;
}
PauseMenu::PauseMenu(RaceScreenServices& services):services_(services),
    highlight_(PauseUi(services),0x605D2BF1u,0xAB671C9Au,PauseUi(services).animationSettings.highlightColor) {}
PauseMenu::~PauseMenu() {(void)Shutdown();}
void PauseMenu::EnterPause(std::int32_t port)
{
    Require(services_.players,"Pause has no actual player registry");
    player_=nullptr;pausingPort_=port;
    for (std::size_t index=0;index<2;++index) {
        auto* player=services_.players->playersByNumber[index];
        if (player && player->movement.controllerPort==port && !player_)player_=player;
    }
    auto& input=PauseInput(services_);
    for (auto* player:services_.players->playersByNumber)
        if (player)joystickPortStopForce(&input,player->movement.controllerPort);
    for (const auto* name:pauseNames)
        if (auto* node=PauseScreens(services_).FindScreen(name))node->SetFilterPort(port);
}
void PauseMenu::ExitPause()
{
    state_=0;pausingPort_=-1;player_=nullptr;
}
void PauseMenu::Activate(std::uint32_t screen)
{
    Require(services_.replay && services_.players,"Pause activation has no actual replay/player owners");
    state_=1;screen_=screen;
    SetPauseItem(services_,screen,0x52E039F6u,
        services_.replay->instantReplaySnapshotCount && !services_.replay->desynchronized?1:0);
    const auto& parameters=PauseParameters(services_);
    SetPauseItem(services_,screen,0x52E039F7u,parameters.eventSelected || parameters.specialPursuit?1:2);
    SetPauseItem(services_,screen,0x52E039F8u,parameters.specialPursuit?1:2);
    SetPauseItem(services_,screen,0x52E039F9u,carFramePlayerAllDrivingLocked(services_.players)?0:1);
    // Final16FDB0 returns0 in retail. Explicit --debug is the requested
    // native testing entry into the still-authored source debug screen.
    SetPauseItem(services_,screen,0x52E039FAu,applicationDebugToolsEnabled?1:2);
    std::uint32_t resume,restart,quit;
    if (raceParametersIsFreeRun(&parameters)) {resume=0x30D48;restart=0x30D4E;quit=0x30D4C;}
    else if (parameters.eventGame) {resume=0x30D4A;restart=0x30D45;quit=0x30D4B;}
    else if (parameters.specialPursuit) {resume=0x30D52;restart=0x30D51;quit=0x30D50;}
    else {resume=0x30D46;restart=0x30D45;quit=0x30D4F;}
    auto& resources=*PauseUi(services_).resources;
    resources.SetLanguageID(screen,0x52E039F9u,resume);
    resources.SetLanguageID(screen,0x52E039F3u,quit);
    resources.SetLanguageID(screen,0x52E039F4u,restart);
}
void PauseMenu::DeActivate() {screen_=0;state_=1;}
bool PauseMenu::AllPlayerJoysticksPluggedIn() const
{
    Require(services_.players,"Pause has no actual player registry");
    const auto& players=*services_.players;
    Require(players.count>=0 && players.count<=2,"Pause player count exceeds the real registry");
    auto& input=PauseInput(services_);
    bool connected=true;
    for (std::int32_t index=0;index<players.count;++index) {
        Require(players.players[index],"Pause player registry contains a missing live Player");
        connected=static_cast<bool>(joystickIsPluggedIn(&input,players.players[index]->movement.controllerPort)) && connected;
    }
    return connected;
}
bool PauseMenu::MaybeResume(std::int32_t port)
{
    if (!AllPlayerJoysticksPluggedIn())return false;
    Require(player_,"Pause resume has no actual Player that paused");
    if (player_->movement.controllerPort!=port)return false;
    RequestUnpause(services_,1);
    Require(joystickResetTypeHandlers(&PauseInput(services_))==GAME_INPUT_OK,
        "Pause resume could not renew the actual controller type handlers");
    return true;
}
void PauseMenu::MaybeRestart()
{
    if (!AllPlayerJoysticksPluggedIn())return;
    const auto& parameters=PauseParameters(services_);
    std::uint32_t message=0x38271,footer=0x38279;
    if (raceParametersIsFreeRun(&parameters)) {message=0x3827F;footer=0x38289;}
    else if (!parameters.eventGame && parameters.specialPursuit) {message=0x38281;footer=0x38285;}
    auto& ui=PauseUi(services_);Require(ui.language,"Pause restart popup has no Language");
    auto& screens=PauseScreens(services_);FlailerPopupConfig config;
    config.buttonCount=2;config.flags=3;config.sourceNode=screens.active;config.screenKey=0x037F3133;
    config.message=ui.language->GetString(message);config.buttonLanguageIds={{0x38273,0x38274,0}};
    config.buttonResults={{2,0,0}};config.initialButton=1;config.joystickPort=pausingPort_;config.footerLanguageId=footer;
    Require(screens.SummonPopup(config)>=0,"Pause restart popup could not obtain its actual popup owner");
    state_=2;PauseVisibility(services_,screen_,1);
}
void PauseMenu::MaybeQuit()
{
    const auto& parameters=PauseParameters(services_);
    std::uint32_t message=0x38272,footer=0x3827B;
    if (raceParametersIsFreeRun(&parameters)) {message=0x3827D;footer=0x38288;}
    else if (parameters.eventGame) {message=0x3827E;footer=0x38284;}
    else if (parameters.specialPursuit) {message=0x38282;footer=0x38283;}
    auto& ui=PauseUi(services_);Require(ui.language,"Pause quit popup has no Language");
    auto& screens=PauseScreens(services_);FlailerPopupConfig config;
    config.buttonCount=2;config.flags=0x13;config.sourceNode=screens.active;config.screenKey=0x037F3133;
    config.message=ui.language->GetString(message);config.buttonLanguageIds={{0x38273,0x38274,0}};
    config.buttonResults={{3,0,0}};config.initialButton=1;config.joystickPort=pausingPort_;config.footerLanguageId=footer;
    Require(screens.SummonPopup(config)>=0,"Pause quit popup could not obtain its actual popup owner");
    state_=3;PauseVisibility(services_,screen_,1);
}
void PauseMenu::MenuCallback(ScreenMessage message,ScreenCallbackData& data)
{
    // PAL170CB0 uses the same event contract as the pause submenus. Calling
    // Animate alone never supplies the selected key and hides the echo.
    highlight_.HandleEvents(message,data);
    if (message==ScreenMessage::Select && state_==1) {
        switch (data.key) {
        case 0x52E039F3u:(void)MaybeResume(data.joystickPort);break;
        case 0x52E039F4u:MaybeRestart();break;
        case 0x52E039F5u:optionsPort_=ChooseWhoSelectedMenuItem(services_,data);
            PauseScreens(services_).ChangeToScreen(pauseNames[1],false);break;
        case 0x52E039F6u:replayPort_=ChooseWhoSelectedMenuItem(services_,data);RequestUnpause(services_,4);break;
        case 0x52E039F7u: {
            const auto& parameters=PauseParameters(services_);
            PauseScreens(services_).ChangeToScreen(pauseNames[parameters.mode==2 && parameters.specialPursuit?6:5],false);break;
        }
        case 0x52E039F8u:PauseScreens(services_).ChangeToScreen(pauseNames[7],false);break;
        case 0x52E039F9u:MaybeQuit();break;
        case 0x52E039FAu:PauseScreens(services_).ChangeToScreen(pauseNames[4],false);break;
        default:break;
        }
    } else if (message==ScreenMessage::LeaveScreen) {data.sound=0;data.value=0;}
    else if (message==ScreenMessage::ScreenBack) {
        if (MaybeResume(data.joystickPort))data.sound=7;
        data.value=1;
    } else if (message==ScreenMessage::PopupDismiss) {
        if (state_==2) {
            if (data.value) {
                auto& parameters=PauseParameters(services_);
                Require(parameters.runtimeSettings && services_.players,"Pause restart has no actual runtime player-settings bank");
                const auto& registry=*services_.players;
                Require(registry.count>=0 && registry.count<=2,"Pause restart player count exceeds its actual bank");
                for (std::int32_t index=0;index<registry.count;++index) {
                    const auto* player=registry.players[index];
                    Require(player && player->movement.seat>=0 && player->movement.seat<2,
                        "Pause restart has no actual seat-indexed Player");
                    parameters.runtimeSettings->players[player->movement.seat]=player->settings;
                }
                RequestUnpause(services_,2);
            } else {PauseVisibility(services_,screen_,0);state_=1;}
        } else if (state_==3) {
            if (data.value)RequestUnpause(services_,3);
            else {PauseVisibility(services_,screen_,0);PauseVisibility(services_,0x037F3133,1);state_=1;}
        }
    }
    // Tick18's actual CheckJoysticks1709D8 is JR RA/NOP; no stub is added.
}
void PauseMenuCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundPause && data,"Pause callback has no actual singleton/payload");
    if (message==ScreenMessage::ScreenActivate) {
        if (boundPause->GetState()==0) {
            Require(boundPause->Services().coordinator,"Pause activation has no actual RaceCoordinator");
            boundPause->EnterPause(boundPause->Services().coordinator->pausingJoystickPort);
        }
        boundPause->Activate(data->key);data->value=0;
    } else if (message==ScreenMessage::ScreenDeactivate)boundPause->DeActivate();
    else boundPause->MenuCallback(message,*data);
}

void PauseDisplayOpts::MenuCallback(ScreenMessage message,ScreenCallbackData& data)
{
    if(subtitles_ && message==ScreenMessage::Highlight)subtitles_->ScrollTo(data.key);
    if(subtitles_ && message==ScreenMessage::Tick) {
        const bool wasMoving=subtitles_->Moving();
        subtitles_->scroll.HandleEvent(message,data);
        // The separate echo and arrows return once the selected row is inside the window.
        if(subtitles_->Moving())return;
        if(wasMoving) {
            arrows_.SetCurrentItem(subtitles_->selectedValue);
            highlight_.ForceStart();valueHighlight_.ForceStart();
        }
    }
    PauseScreenBase::HandleEvent(message,data);
    if (message==ScreenMessage::ScreenBack) {
        Require(boundPause,"Display resume has no actual PauseMenu");
        if (boundPause->MaybeResume(data.joystickPort))data.sound=7;
    } else if (message==ScreenMessage::LeaveScreen) {
        PauseScreens(services_).ChangeToScreen(pauseNames[1],false);data.value=0;
    } else if (message==ScreenMessage::Select) {
        data.sound=0;
        if(subtitles_ && data.key==pauseSubtitleRow) {
            subtitles_->option.Toggle();
            valueHighlight_.ForceStart();
            data.sound=2;
        }
    } else if (message==ScreenMessage::Tick)valueHighlight_.Animate();
    else if (message==ScreenMessage::Left || message==ScreenMessage::Right) {
        const auto direction=message==ScreenMessage::Left?-1:1;
        bool changed=false;
        switch (data.key) {
        case 0x52E039F3u:changed=CycleHUD(direction);break;
        case 0x52E039F4u:changed=ToggleRearView();break;
        case 0x52E039F5u:changed=CycleTrackMap(direction);break;
        case 0x52E039F6u:changed=CycleLeaderboard(direction);break;
        case 0x52E039F7u:changed=ToggleLapInfo();break;
        case 0x52E039F8u:changed=ToggleRacePosition();break;
        case 0x52E039F9u:changed=ToggleGauges();break;
        case 0x52E039FAu:changed=CycleSplitTime(direction);break;
        case 0x52E039FBu:changed=CycleScoring(direction);break;
        case lookbackRow:changed=ToggleLookback();break;
        case 0xAEE77883u:changed=ToggleCameraSpring();break;
        case hudHotkeyRow:changed=ToggleHudHotKey();break;
        case pauseSubtitleRow:
            if(subtitles_) {subtitles_->option.Toggle();changed=true;}
            break;
        default:break;
        }
        data.sound=changed?(direction<0?1:2):0;
        if (changed)valueHighlight_.ForceStart();
    } else if (message==ScreenMessage::Highlight) {
        std::uint32_t value=0;
        for(const auto& [row,field]:pauseDisplayRows)
            if(row==data.key) {value=field;break;}
        if(subtitles_)subtitles_->selectedValue=value;
        arrows_.SetCurrentItem(subtitles_ && subtitles_->Moving()?0:value);
        valueHighlight_.SetCurrentMenuItem(value);
    }
}
void PauseDisplayCallback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundPause && data,"Pause display callback has no actual owner/payload");
    // The surrounding menu may need retirement after this view's failed
    // ScreenActivate; its item notifications arrive before ScreenDeactivate.
    if(!currentPauseDisplay && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if (message==ScreenMessage::ScreenActivate) {
        Require(!currentPauseDisplay,"Pause display is already active");
        currentPauseDisplay=std::make_unique<PauseDisplayOpts>(boundPause->Services(),data->key,displayOptionsPort);
    } else if (message==ScreenMessage::ScreenDeactivate)currentPauseDisplay.reset();
    else {Require(currentPauseDisplay!=nullptr,"Pause display has no live screen");currentPauseDisplay->MenuCallback(message,*data);}
}
namespace {
bSound& PauseSound(RaceScreenServices& services)
{
    auto& ui=PauseUi(services);
    Require(ui.sound && ui.sound->audio && ui.sound->audio->library &&
        ui.sound->manager==&ui.sound->audio->library->Mixer(),"Pause audio has no actual sound-library/mixer owner");
    return *ui.sound->audio->library;
}
}
PauseAudioOpts::PauseAudioOpts(RaceScreenServices& services,std::uint32_t screen)
    :PauseScreenBase(services,screen)
{
    auto& sound=PauseSound(services);
    for (std::size_t channel=0;channel<editingVolumes_.size();++channel)
        editingVolumes_[channel]=sound.GetCategoryVolume(static_cast<std::uint32_t>(channel));
    constexpr std::array<std::int32_t,5> channels{{4,0,2,1,6}};
    constexpr std::array<std::uint32_t,5> thumbs{{0xC866642Bu,0xC866686Cu,0xC8666CADu,0xC86670EEu,0xC866752Fu}};
    for (std::size_t index=0;index<channels.size();++index) {
        const auto channel=channels[index];
        sliders_.push_back(std::make_unique<FlailerSlider>(PauseUi(services),screen,
            0x52E039F3u+static_cast<std::uint32_t>(index),0xA29F7A16u+static_cast<std::uint32_t>(index),
            thumbs[index],channel,&editingVolumes_[static_cast<std::size_t>(channel)],0.0f,1.0f));
    }
    RedrawAudioMode(worldAudioGetMode());
}
std::exception_ptr PauseAudioOpts::Close() noexcept
{
    if(closed_)return closeFailure_;
    closed_=true;
    try {
        auto& sound=PauseSound(services_);
        constexpr std::array<std::uint32_t,7> channels{{5,4,0,6,2,3,1}};
        for (const auto channel:channels)
            Require(sound.SetCategoryVolume(channel,editingVolumes_[channel])==WORLD_AUDIO_OK,
                "Pause audio commit exhausted actual backend command capacity");
        SaveVolumesForSpeech(*PauseUi(services_).sound);
    }catch(...) {closeFailure_=std::current_exception();}
    sliders_.clear();
    return closeFailure_;
}
void PauseAudioOpts::Cancel() noexcept {closed_=true;sliders_.clear();}
PauseAudioOpts::~PauseAudioOpts() {(void)Close();}
FlailerSlider* PauseAudioOpts::FindSlider(std::uint32_t item)
{
    for (const auto& slider:sliders_)if (slider->menuItemKey==item)return slider.get();
    return nullptr;
}
bool PauseAudioOpts::ChangeAudioMode(ScreenMessage direction)
{
    const auto step=direction==ScreenMessage::Left?-1:1;
    auto mode=(worldAudioGetMode()+step)%3;
    if (mode<0)mode=2;
    Require(worldAudioSetMode(PauseUi(services_).sound->audio,mode)==WORLD_AUDIO_OK,
        "Pause audio mode command has no writable backend capacity");
    RedrawAudioMode(mode);return true;
}
void PauseAudioOpts::RedrawAudioMode(std::int32_t mode)
{
    const std::uint32_t language=mode==0?0x1ADB9u:mode==1?0x1ADB8u:mode==2?0x1ADBBu:0;
    PauseUi(services_).resources->SetLanguageID(screen_,0x2932DFCAu,language);
}
void PauseAudioOpts::MenuCallback(ScreenMessage message,ScreenCallbackData& data)
{
    PauseScreenBase::HandleEvent(message,data);
    if (message==ScreenMessage::ScreenBack) {
        Require(boundPause,"Audio resume has no actual PauseMenu");
        if (boundPause->MaybeResume(data.joystickPort))data.sound=7;
    } else if (message==ScreenMessage::LeaveScreen) {
        PauseScreens(services_).ChangeToScreen(pauseNames[1],false);data.value=0;
    } else if (message==ScreenMessage::Select)data.sound=0;
    else if (message==ScreenMessage::Left || message==ScreenMessage::Right) {
        if (data.key==0x52E039F8u) {
            if (ChangeAudioMode(message)) {data.sound=message==ScreenMessage::Left?1:2;valueHighlight_.ForceStart();}
        } else if (auto* slider=FindSlider(data.key))
            slider->StepValue(message==ScreenMessage::Left?-0.05000000074505806f:0.05000000074505806f,false);
    } else if (message==ScreenMessage::Tick) {
        for (const auto& slider:sliders_)slider->Animate();
        valueHighlight_.Animate();
    } else if (message==ScreenMessage::Highlight) {
        if (data.key==0x52E039F8u) {
            valueHighlight_.SetCurrentMenuItem(0x2932DFCAu);arrows_.SetCurrentItem(0x2932DFCAu);
        } else arrows_.SetCurrentItem(0);
    }
}
void PauseAudioOpts::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundPause && data,"Pause audio callback has no actual owner/payload");
    if(!currentPauseAudio && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if (message==ScreenMessage::ScreenActivate) {
        Require(!currentPauseAudio,"Pause audio is already active");
        currentPauseAudio=std::make_unique<PauseAudioOpts>(boundPause->Services(),data->key);
    } else if (message==ScreenMessage::ScreenDeactivate) {
        data->sound=6;
        const auto failure=currentPauseAudio?currentPauseAudio->Close():std::exception_ptr{};
        currentPauseAudio.reset();
        if(failure)std::rethrow_exception(failure);
    }
    else {Require(currentPauseAudio!=nullptr,"Pause audio has no live screen");currentPauseAudio->MenuCallback(message,*data);}
}
FlailerScreenConfig fPauseAudioScreen{"Pause audio",PauseAudioOpts::Callback,
    {{static_cast<std::int32_t>(0x6B321E4Eu),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};
PauseRaceObjective::PauseRaceObjective(RaceScreenServices& services,std::uint32_t screen)
    :services_(services),screen_(screen),onePlayer_(screen!=0xB5ECD7A0u)
{
    auto& ui=PauseUi(services);const auto& parameters=PauseParameters(services);
    if (parameters.eventGame) {
        Require(ui.language && services.frontend->raceSetup->currentEvent,
            "Pause objective has no actual RaceEvent/Language");
        const auto title=FormatLocalizedText(ui.language->GetString(0xF619),
            {static_cast<std::int32_t>(services.frontend->raceSetup->currentEvent->number)});
        ui.resources->SetLocalizedText(screen,0x6A77AF98u,title.c_str());
    } else ui.resources->SetLanguageID(screen,0x6A77AF98u,parameters.specialPursuit?0xECD10:0xF618);
    ui.resources->SetObjectVisibility(screen,0xAF6F1CDCu,1);
    ui.resources->SetObjectVisibility(screen,0x0019DE3Eu,1);
}
PauseRaceObjective::~PauseRaceObjective()=default;
void PauseRaceObjective::HandleEvent(ScreenMessage message,ScreenCallbackData& data)
{
    if (message==ScreenMessage::ScreenBack) {
        Require(boundPause,"Objective resume has no actual PauseMenu");
        if (boundPause->MaybeResume(data.joystickPort)) {data.value=1;data.sound=7;}
    } else if (message==ScreenMessage::Select || message==ScreenMessage::LeaveScreen) {
        PauseScreens(services_).ChangeToScreen(pauseNames[0],false);data.value=0;
    } else if (message==ScreenMessage::RenderModels) {
        auto& ui=PauseUi(services_);Require(ui.render,"Pause objective has no actual renderer");
        if (onePlayer_) {
            auto& input=PauseInput(services_);
            if (joystickIsPluggedIn(&input,0))pauseObjectiveButtonChar=joystickGetButtonChar(&input,0,126);
            const auto* format=FigureOutRaceObjective(services_,0);
            Require(format,"Pause objective has no source text");
            const auto text=FormatLocalizedButtonText(format,pauseObjectiveButtonChar,512);
            ui.render->RenderTextIntoRectangle(*ui.resources,screen_,0xAEFE36D8u,0xCCCF9A75u,text.c_str(),1.0f);
        } else {
            const auto* first=FigureOutRaceObjective(services_,0);const auto* second=FigureOutRaceObjective(services_,1);
            Require(first && second,"Pause objective lacks actual two-player source text");
            ui.render->RenderTextIntoRectangle(*ui.resources,screen_,0x8EC51608u,0x232320F5u,first,1.0f);
            ui.render->RenderTextIntoRectangle(*ui.resources,screen_,0x8EC51609u,0x232320F6u,second,1.0f);
        }
    }
}
void PauseRaceObjective::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundPause && data,"Pause objective callback has no actual owner/payload");
    if(!currentPauseObjective && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if (message==ScreenMessage::ScreenActivate) {
        Require(!currentPauseObjective,"Pause objective is already active");
        currentPauseObjective=std::make_unique<PauseRaceObjective>(boundPause->Services(),data->key);
    } else if (message==ScreenMessage::ScreenDeactivate)currentPauseObjective.reset();
    else {Require(currentPauseObjective!=nullptr,"Pause objective has no live screen");currentPauseObjective->HandleEvent(message,*data);}
}
FlailerScreenConfig fPauseRaceObjectiveScreen{"Race Objective",PauseRaceObjective::Callback,
    {{static_cast<std::int32_t>(0x887B4D37u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};
FlailerScreenConfig fPauseRaceObjective2PScreen{"Race Objective 2PBAC",PauseRaceObjective::Callback,
    {{static_cast<std::int32_t>(0xB5ECD7A0u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};

int HandlePauseMenuReplaySpecialEvents(RaceScreenServices& services,const ReplaySpecialEventInfo& event)
{
    auto playerAt=[&](int32_t index)->CarFramePlayerState& {
        Require(services.players && index>=0 && index<2 && services.players->playersByNumber[index],
            "Pause replay event has no actual numbered Player");
        return *services.players->playersByNumber[index];
    };
    switch (event.kind) {
    case 0:
        Require(services.race,"Pause replay event has no actual Race");
        services.race->pendingFinishPlayerIndex=event.argument2;
        services.race->pendingFinishPosition=event.argument1;
        return 1;
    case 1: {
        auto& player=playerAt(event.argument1);
        Require(player.car && player.car->bindings.state && player.car->bindings.owner,
            "Driving-style replay event has no canonical Car state");
        player.car->bindings.state->pursuitMode=event.argument2;
        carFramePlayerSetDrivingStyle(&player,event.argument2);
        auto& parameters=PauseParameters(services);parameters.authoredRaceOption254=event.argument2;
        SetAIDriverModesFromPlayers(services.players->world,services.race,parameters,*services.players);
        return 1;
    }
    case 2: {
        auto& parameters=PauseParameters(services);parameters.authoredRaceOption40=event.argument1!=0;
        Require(services.players && services.players->world,"Damage replay event has no actual World");
        const auto& world=*services.players->world;
        Require(world.totalCars>=0 && world.totalCars<=WORLD_MAX_CARS,"Damage replay event has an invalid car table");
        for (int32_t index=0;index<world.totalCars;++index) {
            const auto* car=world.carTable[index];Require(car && car->movement,"Damage replay event has no actual Car");
            const auto* movement=static_cast<const CarMovement*>(car->movement);
            Require(movement->bindings.owner==car,"Damage replay event has mismatched Car movement");
            if (movement->bindings.damage)carDamageRethink(movement->bindings.damage);
        }
        return 1;
    }
    case 3: {
        auto& player=playerAt(event.argument1);
        Require(player.car && player.car->bindings.owner,"Transmission replay event has no actual Car");
        auto* driver=static_cast<CarDriver*>(player.car->bindings.owner->controller);
        Require(driver && driver->kind==CAR_DRIVER_REAL && driver->owner.real &&
            driver->owner.real->input.movement==player.car,
            "Transmission replay event has no matching actual RealController");
        player.settings.gearInputMode=static_cast<int8_t>(event.argument2);
        realDriverSetGearInputMode(&driver->owner.real->input,event.argument2);
        return 1;
    }
    case 4:DisplaySettings(services).authoredOption6=static_cast<uint8_t>(event.argument1);return 1;
    case 5:DisplaySettings(services).authoredOption7=static_cast<uint8_t>(event.argument1);return 1;
    case 6:carControllerSuperEasyAI=event.argument1;return 1;
    default:return 0;
    }
}
namespace {
constexpr std::array<std::uint32_t,7> pauseOptionLabels{{0x5DEB55A3u,0xB6791614u,0xA2450F0Fu,
    0x6DB88809u,0x1F1B628Bu,0x16E95F2Bu,0xCB3BDD8Fu}};
void AddPauseReplayEvent(RaceScreenServices& services,int32_t kind,int32_t first,int32_t second)
{
    Require(services.replay,"Pause options have no actual ReplayManager");
    (void)replayManagerAddSpecialEvent(*services.replay,services,kind,first,second);
}
}
PauseDebugMenu::PauseDebugMenu(RaceScreenServices& services,std::uint32_t screen)
    :services_(services),screen_(screen)
{
    auto& resources=*PauseUi(services_).resources;
    if(services_.race && services_.race->state==4)
        resources.Printf(screen_,0x52E039F3u,"Finish in %dth position",finishPosition_);
    else resources.Printf(screen_,0x52E039F3u,"Please wait for start race!");
    resources.Printf(screen_,0x52E039F4u,"Speedline mode");
    resources.Printf(screen_,0x52E039F5u,"Car-to-car collisions %s",pauseMenuCarToCarCollisionsEnabled?"On":"Off");
    resources.Printf(screen_,0x52E039F6u,"Screen Print %s",applicationScreenPrintf?"On":"Off");
    resources.Printf(screen_,0x52E039F7u,"Environment: %s","<default>");
    resources.Printf(screen_,0x52E039F9u,"Rearview Mirror %s",DisplaySettings(services_).authoredOption0?"On":"Off");
    resources.Printf(screen_,0x52E039FAu,"ENV MAPPING %s",environmentMappingEnabled?"ON":"OFF");
    resources.Printf(screen_,0x52E039FBu,"Show debug Info %s",displayPlayerDebugInfo?"ON":"OFF");
    resources.Printf(screen_,0xAEE77884u,"Super easy AI %s",carControllerSuperEasyAI?"On":"Off");
    resources.Printf(screen_,0x52E039F8u,"removed");
    // Both Beta and PAL retain inactive speedline/removed branches and an
    // empty SetStormType; the collision flag has only menu readers/writes.
    // ScreenPrint retains a renderer, but its queue has no active producer;
    // DisplayCullingStats and BeltMover::DebugScreenPrint are empty in both.
    // These are source limitations, not missing PS2 renderer replacements.
    // Keep the authored rows visible, but do not advertise inactive tools.
    for(const auto key:{0x52E039F4u,0x52E039F5u,0x52E039F6u,0x52E039F7u,0x52E039F8u})
        SetPauseItem(services_,screen_,key,0);
    if (!applicationDebugRaceOverridesEnabled) {
        SetPauseItem(services_,screen_,0x52E039F3u,2);
        SetPauseItem(services_,screen_,0xAEE77884u,2);
    }
}
std::exception_ptr PauseDebugMenu::Close() noexcept
{
    if(closed_)return closeFailure_;
    closed_=true;
    if(finishPosition_)try {
        Require(boundPause && boundPause->GetPlayerThatPaused(),"Debug finish has no actual pausing Player");
        AddPauseReplayEvent(services_,0,finishPosition_,boundPause->GetPlayerThatPaused()->movement.seat);
    }catch(...) {closeFailure_=std::current_exception();}
    return closeFailure_;
}
void PauseDebugMenu::Cancel() noexcept {closed_=true;finishPosition_=0;}
PauseDebugMenu::~PauseDebugMenu() {(void)Close();}
void PauseDebugMenu::MenuCallback(ScreenMessage message,ScreenCallbackData& data)
{
    if(message==ScreenMessage::Left || message==ScreenMessage::Right) {
        PauseUi(services_).sound->Play(1);OnMenuItemScroll(message,data);
    } else if(message==ScreenMessage::LeaveScreen) {
        PauseScreens(services_).ChangeToScreen(pauseNames[0],false);data.value=0;
    } else if(message==ScreenMessage::Select && data.key==0xAEE77883u) {
        // Native orderly process-exit equivalent of the retained source
        // debug exit request; cleanup still belongs to ApplicationRuntime.
        applicationDebugExitRequested=true;
    }
}
void PauseDebugMenu::OnMenuItemScroll(ScreenMessage message,ScreenCallbackData& data)
{
    auto& resources=*PauseUi(services_).resources;
    switch(data.key) {
    case 0x52E039F3u:
        if(applicationDebugRaceOverridesEnabled && services_.race && services_.race->state==4) {
            finishPosition_+=message==ScreenMessage::Left?-1:1;
            finishPosition_=std::max(0,std::min(finishPosition_,static_cast<std::int32_t>(services_.race->entryCount)));
            resources.Printf(screen_,data.key,"Finish in %dth position",finishPosition_);
        }
        break;
    case 0x52E039F9u: {
        auto& enabled=DisplaySettings(services_).authoredOption0;enabled=enabled==0;
        resources.Printf(screen_,data.key,"Rearview Mirror %s",enabled?"ON":"OFF");break;
    }
    case 0x52E039FAu:
        environmentMappingEnabled=environmentMappingEnabled==0;
        resources.Printf(screen_,data.key,"ENV MAPPING %s",environmentMappingEnabled?"ON":"OFF");break;
    case 0x52E039FBu:
        displayPlayerDebugInfo=displayPlayerDebugInfo==0;
        resources.Printf(screen_,data.key,"Show debug Info %s",displayPlayerDebugInfo?"ON":"OFF");break;
    case 0xAEE77884u:
        if (applicationDebugRaceOverridesEnabled) (void)ToggleSuperEasyAI();
        break;
    default:break;
    }
}
bool PauseDebugMenu::ToggleSuperEasyAI()
{
    carControllerSuperEasyAI=carControllerSuperEasyAI==0;
    AddPauseReplayEvent(services_,6,carControllerSuperEasyAI,0);
    PauseUi(services_).resources->Printf(screen_,0xAEE77884u,"Super easy AI %s",carControllerSuperEasyAI?"On":"Off");
    return true;
}
void PauseDebugMenu::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundPause && data,"Pause debug callback has no actual owner/payload");
    if(!currentPauseDebug && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if(message==ScreenMessage::ScreenActivate) {
        Require(applicationDebugToolsEnabled && !currentPauseDebug,"Pause debug is not enabled or is already active");
        currentPauseDebug=std::make_unique<PauseDebugMenu>(boundPause->Services(),data->key);
    } else if(message==ScreenMessage::ScreenDeactivate) {
        const auto failure=currentPauseDebug?currentPauseDebug->Close():std::exception_ptr{};
        currentPauseDebug.reset();
        if(failure)std::rethrow_exception(failure);
    }
    else {Require(currentPauseDebug!=nullptr,"Pause debug has no active screen");currentPauseDebug->MenuCallback(message,*data);}
}
// PAL174C7C..174CB8 registers this exact key and174590 callback.
FlailerScreenConfig fPauseDebugMenu{"Debug Menu",PauseDebugMenu::Callback,
    {{static_cast<std::int32_t>(0xD4742CD7u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};

CarFramePlayerState& PauseOptions::Player() const
{
    Require(player_,"Pause options port has no actual live Player");return *player_;
}
PauseOptions::PauseOptions(RaceScreenServices& services,std::uint32_t screen,std::int32_t port)
    :PauseScreenBase(services,screen),port_(port)
{
    auto& ui=PauseUi(services);
    for (std::size_t index=0;index<labels_.size();++index) {
        auto* object=ui.resources->FindObject(screen,pauseOptionLabels[index],1);
        Require(object && object->kind==1,"Pause options value object is absent from its authored final screen");
        labels_[index]=static_cast<FlailerTextChunk*>(object);
    }
    Require(services.players,"Pause options have no actual Player registry");
    for (std::size_t index=0;index<2;++index) {
        auto* player=services.players->playersByNumber[index];
        if (player && player->movement.controllerPort==port) {
            player_=player;ui.resources->SetLanguageID(screen,0x18B2DBC8u,index?0x334B6u:0x334B5u);break;
        }
    }
    (void)Player();
    DisplayVibration();DisplayDamage();DisplayDrivingStyle();DisplayTransmission();
    DisplayJumpCam();DisplayZoneCam();DisplayAltCam();UpdateCameraOptionsVisibility();
    Require(services.frontend->demoMode && services.frontend->demoMode->demoDisc,
        "Pause options have no actual DemoDiscManager");
    if (services.frontend->demoMode->demoDisc->ActivationWord()) {
        SetPauseItem(services,screen,0x52E039F6u,0);ui.resources->SetObjectVisibility(screen,pauseOptionLabels[1],0);
    } else ui.resources->SetObjectVisibility(screen,pauseOptionLabels[1],1);
}
PauseOptions::~PauseOptions()=default;
void PauseOptions::UpdateCameraOptionsVisibility()
{
    Require(services_.players,"Pause camera options have no actual Player registry");
    const bool single=services_.players->count==1;
    SetPauseItem(services_,screen_,0x52E039F9u,single?1:2);
    PauseUi(services_).resources->SetObjectVisibility(screen_,0x1F1B628Bu,single);
    SetPauseItem(services_,screen_,0x52E039FAu,single?1:2);
    PauseUi(services_).resources->SetObjectVisibility(screen_,0x16E95F2Bu,single);
}
bool PauseOptions::SetVibration(ScreenMessage)
{
    auto& setting=Player().settings.authoredOption;
    if (setting)setting=0;
    else {
        setting=1;auto& ui=PauseUi(services_);
        Require(ui.flow && services_.frontend->frontend && services_.players,"Vibration feedback has no actual flow/profile/player owners");
        joystickPortRumbleImpact(PauseInput(services_),port_,8.0f,*ui.flow,services_.coordinator,
            *services_.frontend->frontend,*services_.players);
    }
    DisplayVibration();return true;
}
bool PauseOptions::SetDamage(ScreenMessage)
{
    AddPauseReplayEvent(services_,2,PauseParameters(services_).authoredRaceOption40^1,0);DisplayDamage();return true;
}
bool PauseOptions::SetDrivingStyle(ScreenMessage)
{
    AddPauseReplayEvent(services_,1,Player().movement.seat,Player().settings.initialVehicleMode==0);
    DisplayDrivingStyle();UpdateCameraOptionsVisibility();return true;
}
bool PauseOptions::SetTransmission(ScreenMessage direction)
{
    int32_t mode=Player().settings.gearInputMode;
    if (direction==ScreenMessage::Left)--mode;else if (direction==ScreenMessage::Right)++mode;
    mode=((mode%3)+3)%3;
    AddPauseReplayEvent(services_,3,Player().movement.seat,mode);DisplayTransmission();return true;
}
bool PauseOptions::SetJumpCam(ScreenMessage)
{
    AddPauseReplayEvent(services_,4,DisplaySettings(services_).authoredOption6==0,0);DisplayJumpCam();return true;
}
bool PauseOptions::SetZoneCam(ScreenMessage)
{
    AddPauseReplayEvent(services_,5,DisplaySettings(services_).authoredOption7==0,0);DisplayZoneCam();return true;
}
bool PauseOptions::SetAltCam(ScreenMessage)
{
    playerSettingsCycleAlternateCamera(&Player().settings);DisplayAltCam();return true;
}
void PauseOptions::DisplayVibration() {labels_[0]->SetLanguageID(Player().settings.authoredOption?0x3345D:0x3345E);}
void PauseOptions::DisplayDamage() {labels_[1]->SetLanguageID(PauseParameters(services_).authoredRaceOption40?0x33459:0x3345A);}
void PauseOptions::DisplayDrivingStyle() {labels_[2]->SetLanguageID(Player().settings.initialVehicleMode?0xBFA4:0xBF9A);}
void PauseOptions::DisplayTransmission()
{
    constexpr std::uint32_t ids[]{0xBF7C,0xBF86,0xBF72};const auto mode=Player().settings.gearInputMode;
    if (mode>=0 && mode<3)labels_[3]->SetLanguageID(ids[mode]);
}
void PauseOptions::DisplayJumpCam() {labels_[4]->SetLanguageID(DisplaySettings(services_).authoredOption6?0x22352:0x22353);}
void PauseOptions::DisplayZoneCam() {labels_[5]->SetLanguageID(DisplaySettings(services_).authoredOption7?0x22352:0x22353);}
void PauseOptions::DisplayAltCam()
{
    // The released display reads settings+0C (lookback), although its setter
    // cycles the independent alternateCamera byte. Preserve both operations.
    labels_[6]->SetLanguageID(Player().settings.lookback?0x1D4C6:0x22356);
}
void PauseOptions::MenuCallback(ScreenMessage message,ScreenCallbackData& data)
{
    PauseScreenBase::HandleEvent(message,data);
    if (message==ScreenMessage::ScreenBack) {
        Require(boundPause,"Options resume has no actual PauseMenu");
        if (boundPause->MaybeResume(data.joystickPort))data.sound=7;
    } else if (message==ScreenMessage::LeaveScreen) {
        PauseScreens(services_).ChangeToScreen(pauseNames[0],false);data.value=0;
    } else if (message==ScreenMessage::Select) {
        if (data.key==0x52E039F3u)PauseScreens(services_).ChangeToScreen(pauseNames[2],false);
        else if (data.key==0x52E039F4u) {displayOptionsPort=data.joystickPort;PauseScreens(services_).ChangeToScreen(pauseNames[3],false);}
        else data.sound=0;
    } else if (message==ScreenMessage::Tick)valueHighlight_.Animate();
    else if (message==ScreenMessage::Left || message==ScreenMessage::Right) {
        bool changed=false;
        switch (data.key) {
        case 0x52E039F5u:changed=SetVibration(message);break;
        case 0x52E039F6u:changed=SetDamage(message);break;
        case 0x52E039F7u:changed=SetDrivingStyle(message);break;
        case 0x52E039F8u:changed=SetTransmission(message);break;
        case 0x52E039F9u:changed=SetJumpCam(message);break;
        case 0x52E039FAu:changed=SetZoneCam(message);break;
        case 0x52E039FBu:changed=SetAltCam(message);break;
        default:break;
        }
        if (changed) {data.sound=message==ScreenMessage::Left?1:2;valueHighlight_.ForceStart();}
    } else if (message==ScreenMessage::Highlight) {
        const auto index=data.key-0x52E039F5u;
        const auto label=index<pauseOptionLabels.size()?pauseOptionLabels[index]:0;
        arrows_.SetCurrentItem(label);valueHighlight_.SetCurrentMenuItem(label);
    }
}
void PauseOptions::Callback(ScreenMessage message,ScreenCallbackData* data)
{
    Require(boundPause && data,"Pause options callback has no actual owner/payload");
    if(!currentPauseOptions && (message==ScreenMessage::Unhighlight ||
        message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
    if (message==ScreenMessage::ScreenActivate) {
        Require(!currentPauseOptions,"Pause options are already active");
        currentPauseOptions=std::make_unique<PauseOptions>(boundPause->Services(),data->key,boundPause->GetOptionsPort());
    } else if (message==ScreenMessage::ScreenDeactivate)currentPauseOptions.reset();
    else {Require(currentPauseOptions!=nullptr,"Pause options have no live screen");currentPauseOptions->MenuCallback(message,*data);}
}
FlailerScreenConfig fPauseOptionsScreen{"Pause Options",PauseOptions::Callback,
    {{static_cast<std::int32_t>(0x2B5A03A8u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};

namespace {
// PAL171C50..172068: control-page callbacks registered by the static
// pause configuration.
class PauseControlPages final {
public:
    PauseControlPages(RaceScreenServices& services,std::uint32_t screen):services_(services),screen_(screen)
    {PauseUi(services).resources->SetLanguageID(screen,0x6A77AF98u,0x30D55);}
    const char* Instruction(std::int32_t page) const
    {
        auto& ui=PauseUi(services_);Require(ui.language,"Pause control text has no actual Language");
        auto& input=PauseInput(services_);
        if (!joystickIsPluggedIn(&input,0))return ui.language->GetString(0xDBBAB);
        if (page!=1 && page!=2)return nullptr;
        if (joystickIsTypeWheel(&input,0)) {
            if (joystickGetType(&input,0)==0x16)return ui.language->GetString(page==1?0x30D7C:0x30D7D);
            return ui.language->GetString(page==1?0x30D76:0x30D77);
        }
        Require(services_.players && services_.players->playersByNumber[0],"Pause control text has no actual live Player");
        const auto mode=services_.players->playersByNumber[0]->settings.gearInputMode;
        const bool digital=joystickGetType(&input,0)==0x41;
        if (mode==0)return ui.language->GetString(page==1?(digital?0x30D7A:0x30D74):(digital?0x30D7B:0x30D75));
        if (mode==1 || mode==2)return ui.language->GetString(page==1?(digital?0x30D78:0x30D72):(digital?0x30D79:0x30D73));
        return nullptr;
    }
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data)
    {
        if (message==ScreenMessage::Select) {
            ++page_;if (page_>=3) {PauseScreens(services_).ChangeToScreen(pauseNames[0],false);data.value=0;}
        } else if (message==ScreenMessage::LeaveScreen) {
            --page_;if (page_<=0)PauseScreens(services_).ChangeToScreen(pauseNames[0],false);data.value=0;
        } else if (message==ScreenMessage::RenderModels) {
            auto& input=PauseInput(services_);auto& ui=PauseUi(services_);
            if (joystickIsPluggedIn(&input,0))buttonChar_=joystickGetButtonChar(&input,0,126);
            const auto* format=Instruction(page_);Require(format && ui.render,"Pause control page has no source text/render owner");
            const auto text=FormatLocalizedButtonText(format,buttonChar_,512);
            ui.render->RenderTextIntoRectangle(*ui.resources,screen_,0xAEFE36D8u,0xCCCF9A75u,text.c_str(),1.0f);
            ui.resources->SetObjectVisibility(screen_,0xAF6F1CDCu,1);ui.resources->SetObjectVisibility(screen_,0x0019DE3Eu,1);
        }
    }
    static void Callback(ScreenMessage message,ScreenCallbackData* data)
    {
        Require(boundPause && data,"Pause control pages have no actual owner/payload");
        if(!currentPauseControls && (message==ScreenMessage::Unhighlight ||
            message==ScreenMessage::ItemDeactivate || message==ScreenMessage::LeaveScreen))return;
        if (message==ScreenMessage::ScreenActivate) {
            Require(!currentPauseControls,"Pause control pages are already active");
            currentPauseControls=std::make_unique<PauseControlPages>(boundPause->Services(),data->key);
        } else if (message==ScreenMessage::ScreenDeactivate)currentPauseControls.reset();
        else {Require(currentPauseControls!=nullptr,"Pause control pages have no live screen");currentPauseControls->HandleEvent(message,*data);}
    }
private:
    RaceScreenServices& services_;
    std::uint32_t screen_;
    std::int32_t page_{1};
    static std::int32_t buttonChar_;
};
std::int32_t PauseControlPages::buttonChar_=0x89; // Final3306BC, retained across actual lifetimes.
}
std::exception_ptr PauseMenu::Shutdown() noexcept
{
    // Terminal release is independent of a failed ScreenDeactivate. Do not
    // publish new mixer commands or debug finish events after admission stops.
    if(boundPause==this) {
        if(currentPauseAudio)currentPauseAudio->Cancel();
        if(currentPauseDebug)currentPauseDebug->Cancel();
        currentPauseDisplay.reset();currentPauseOptions.reset();
        currentPauseControls.reset();currentPauseAudio.reset();
        currentPauseObjective.reset();currentPauseDebug.reset();
        boundPause=nullptr;
    }
    state_=0;pausingPort_=-1;optionsPort_=-1;replayPort_=-1;player_=nullptr;
    return {};
}
FlailerScreenConfig fPauseControlsScreen{"Pause BAC controls",PauseControlPages::Callback,
    {{static_cast<std::int32_t>(0x887B4D37u),0,0,0}},{{-1,-1,-1,-1}},1,nullptr};

}
