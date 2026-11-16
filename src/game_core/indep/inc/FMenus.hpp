#pragma once

#include "FlailerChunks.hpp"
#include "FEAnim.hpp"
#include "indep/inc/CarDriver.hpp"
#include "Texture.hpp"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct FlailerFrontendInput FlailerFrontendInput;
GameInputResult flailerFrontendDispatchInput(FlailerFrontendInput* input,int32_t port,int32_t event,uint8_t value);
#ifdef __cplusplus
}
namespace hp2 { struct FlailerScreenList; }
struct FlailerFrontendInput {
    GameInputRouter* router{};
    hp2::FlailerScreenList* screens{};
};
#endif

#include "indep/inc/Point.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>

class DemoDiscManager;
struct ReplayIntegerLog;
struct CarSoundBindings;
namespace hp2 { class MoviePlayer; }

namespace hp2 {

enum class ScreenMessage : std::int32_t;
struct ScreenCallbackData;
using ScreenCallback=void (*)(ScreenMessage,ScreenCallbackData*);
struct FlailerUiState;
struct FlailerMenu;
class FlailerMenuStorage;
// Native declaration of the individually proved helpmenu.cpp owner.
class FlailerHelpMenu final {
public:
    explicit FlailerHelpMenu(FlailerUiState& ui);
    bool IsActive() const;
    void Summon(const std::int32_t* items);
    void Dismiss();
    void Tick();
    void SplitHelp(std::int32_t languageId,char prefix[128],char description[128],std::uint32_t capacity) const;
    const tCubic1D& Slide() const noexcept { return slide_; }
private:
    void PlaySelect();
    FlailerUiState* ui_{};
    std::uint32_t screenKey_{0x087cd45cu};
    tCubic1D slide_{};
    std::int32_t wasVisible_{};
    std::uint32_t summonCount_{};
    FlailerObjectChunk* fadeObject_{};
    std::array<float,4> initialAlpha_{};
};
class FlailerHighlightAnimation;
class FlailerAlphaAnimation;
class FlailerGlobalBackgroundAnimation;
struct FlailerScreenConfig;
struct FrontendPlayerManager;
struct FrontendRaceSetupBindings;
struct ApplicationFrontendFlow;
class LanguageManager;

struct FlailerMenuItem {
    FlailerMenuItem* next{};
    FlailerMenuItem* previous{};
    std::uint32_t objectKey{};
    FlailerObjectChunk* object{};
    std::uint32_t flags{1};
    std::uint32_t ordinal{};
    std::uint32_t savedColor{};
    std::int32_t joystickPort{-1};
    std::uint32_t screenKey{};
    FlailerUiState* ui{};

    FlailerMenuItem()=default;
    FlailerMenuItem(FlailerMenu* menu,std::uint32_t key,std::uint32_t number);
    ~FlailerMenuItem();
    void ResetSavedParameters();
    void Activate(ScreenCallback handler);
    void UnActivate(ScreenCallback handler);
    void Highlight(ScreenCallback handler,FlailerBitmapChunk* bar);
    void UnHighlight(ScreenCallback handler,FlailerBitmapChunk* bar);
    void SetMenuItemState(std::uint32_t bits);
    void ClearMenuItemState(std::uint32_t bits);
    void JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value,ScreenCallback handler);
};

struct FlailerMenu {
    FlailerMenuItem root{};
    FlailerMenuItem* current{};
    ScreenCallback callback{};
    FlailerUiState* ui{};
    std::uint32_t screenKey{};
    std::int32_t joystickPort{-1};
    FlailerBitmapChunk* highlightBar{};
    std::int32_t GetCurrentItemNumber() const;
    std::uint32_t GetActiveMenuItem() const;
    FlailerMenuItem* FindItem(std::uint32_t objectKey);
    void SetCurrentItemNumber(std::int32_t number);
    void Activate();
    void UnActivate();
    void ResetCurrentMenuItem();
    void SetActiveMenuItem(std::uint32_t key);
    void MoveToNextItem();
    void MoveToPrevItem();
    void JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value);
    void Tick();
};

struct FlailerScreen {
    FlailerScreen* next{};
    FlailerScreen* previous{};
    FlailerMenu* menu;
    std::int32_t currentItemNumber;
    std::int32_t joystickPort;
    std::int32_t screenId;
    ScreenCallback callback;
    FlailerUiState* ui{};
    std::shared_ptr<FlailerMenuStorage> ownedMenu{};

    FlailerScreen(std::int32_t id,ScreenCallback handler,std::int32_t port);
    void SetActiveMenuItem(std::uint32_t key);
    void Activate();
    void UnActivate();
    void JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value);
    void Tick();
    void RenderModels(bool advanceState=true);
};

struct FlailerScreenNode {
    FlailerScreenNode* next{};
    FlailerScreenNode* previous{};
    FlailerScreen screenRoot{-1,nullptr,-1};
    FlailerScreen* screen{};
    FlailerScreenConfig* configuration{};
    FlailerScreen* FindFlailerScreen(std::int32_t id);
    bool IsPortFiltered(std::int32_t port) const;
    void Activate();
    void UnActivate();
    void JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value);
    void SetCallback(ScreenCallback handler);
    void SetFilterPort(std::int32_t port);
    void SetActiveMenuItem(std::uint32_t key);
    void SetVisibility(std::int32_t visibility);
    void Tick();
    void RenderModels(bool advanceState=true);
    std::uint32_t GetActiveMenuItem() const;
};

struct ScreenStack {
    static constexpr std::size_t Capacity=12;
    static constexpr std::size_t NameCapacity=32;
    std::array<std::array<char,NameCapacity>,Capacity> names{};
    std::uint32_t count{};
    char* Pop();
    void Push(const char* name);
};

struct FlailerPopupConfig {
    std::int32_t buttonCount{};
    std::uint32_t flags{};
    FlailerScreenNode* sourceNode{};
    std::uint32_t screenKey{};
    const char* message{};
    std::array<std::uint32_t,3> buttonLanguageIds{};
    std::array<std::int32_t,3> buttonResults{};
    std::int32_t cancelResult{};
    std::int32_t dismissResult{};
    std::int32_t initialButton{};
    std::int32_t joystickPort{};
    // Copied client parameters are retained for the screen callback. The
    // popup framework neither interprets nor dereferences these words.
    std::array<std::uint32_t,2> clientParameters{};
    std::uint32_t footerLanguageId{};
};
struct FlailerPopup {
    std::int32_t activeState{};
    FlailerPopupConfig ownedConfiguration{};
    FlailerPopupConfig* configuration{&ownedConfiguration};
    std::array<std::unique_ptr<FlailerMenuItem>,3> buttons;
    ScreenCallback callback{};
    std::int32_t selectedIndex{INT32_MIN};
    float phase{};
    FlailerUiState* ui{};
    std::unique_ptr<FlailerHighlightAnimation> highlight;
    FlailerPopup();
    explicit FlailerPopup(FlailerUiState& state);
    ~FlailerPopup();
    FlailerMenuItem* GetCurrentMenuItem();
    void HighlightCurrentMenuItem();
    void UnHighlightCurrentMenuItem();
    void Summon(const FlailerPopupConfig& parameters);
    void Dismiss(std::int32_t port,std::int32_t result);
    bool JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value);
    void Tick();
};

// tNewSongMonitor owns the actual optional alpha animation. Reset deliberately
// leaves the current track, change timer and counter untouched (PAL16BDE8).
class FlailerNewSongMonitor final {
public:
    FlailerNewSongMonitor();
    ~FlailerNewSongMonitor();
    void Reset();
    void Update(FlailerUiState& ui,const CarSoundBindings& sound);
    std::int32_t currentJukeboxIndex{-1};
    std::int32_t songChangedTimer{INT32_MAX};
    std::uint32_t screenKey{};
    std::int32_t numberOfChanges{};
    std::int32_t songChangeStarted{};
    std::unique_ptr<FlailerAlphaAnimation> alphaAnimation;
};
extern std::int32_t newSongDisplayEnabled;
extern std::int32_t newSongTitleWidth;
class FlailerLoadingScreenAnimation;

struct FlailerScreenList {
    // Port-side identity for events queued against the presented screen.
    std::uint64_t pointerEpoch{};
    ScreenStack stack{};
    const char* mainScreen{};
    std::array<FlailerPopup*,2> popups{};
    FlailerScreenNode root{};
    FlailerScreenNode* active{};
    FlailerUiState* ui{};
    FlailerHelpMenu* help{};
    FlailerGlobalBackgroundAnimation* background{};
    ::FlailerFrontendInput* input{};
    const std::int32_t* overrideHelpItems{};
    // Source ScreenList+1CC..1E0 retain the six actual movie texture views.
    // Their storage belongs to TexturePack; this owner never copies pixels.
    std::array<TextureByteView,6> movieTextures{};
    std::string movieFilename;
    FlailerNewSongMonitor newSongMonitor;
    const CarSoundBindings* soundBindings{};
    FlailerLoadingScreenAnimation* loadingAnimation{};

    void SetMainScreen(const char* name);
    FlailerScreenNode* FindScreen(std::int32_t screenId);
    FlailerScreenNode* FindScreen(const char* name);
    void PopScreens(std::int32_t number);
    void PopUntil(const char* name);
    const char* GetCurrentScreenNameFromStack() const;
    void SetHelpOverride(const std::int32_t* items);
    bool MakeHelpGo(const std::int32_t* items,std::int32_t state);
    void ChangeToScreen(const char* name,bool push);
    bool MoveToPreviousScreen(std::int32_t count,bool consultCallback,std::int32_t port);
    void AddNormalMenuJoyHandlers();
    void RemoveNormalMenuJoyHandlers();
    void JoyHandler(std::int32_t port,std::int32_t event,std::uint8_t value);
    void RenderModels(bool advanceState=true);
    void Tick();
    std::int32_t SummonPopup(const FlailerPopupConfig& parameters);
    void DismissPopup(std::int32_t index);
    bool IsAnyPopupActive() const;
    std::int32_t GetPopupActiveState(std::int32_t index) const;
    FlailerPopupConfig* GetPopupConfig(std::int32_t index) const;
    void StartMovie(const char* filename);
    void UpdateMovie();
    void StopMovie();
    const char* GetDefaultMovieFilename() const noexcept;
    void StartDefaultMovie();
};
// Borrow of the process menu owner read by PAL16D420. Null is the original
// absent-before-startup/after-shutdown case; no second screen list is stored.
void MenuSystemTick(FlailerScreenList* menuSystem);

std::int32_t DriveMovie(FlailerUiState& ui,FlailerBitmapChunk& bitmap,ReplayIntegerLog& log);

// Native arrangement of frontend.cpp's one demo track-list/cache owner.
// Pointers borrow the application objects; selected race/profile data stay
// with those objects. Construct once for the live frontend.
struct FrontendDemoMode {
    FlailerScreenList* screens{};
    FrontendPlayerManager* frontend{};
    const FrontendRaceSetupBindings* raceSetup{};
    ::DemoDiscManager* demoDisc{};
    GameResources* files{};
    std::array<std::int32_t,32> tracks{};
    std::int32_t trackCount{};
    bool trackListBuilt{};
    std::int32_t trackSequence{};
    void BuildDemoTrackList();
    void StartDemoMode();
    void ConsiderStartDemoMode(std::int32_t idleStartedTicks);
    // Port-side entry for a frontend action using the same launch policy.
    void RequestDemoMode(std::int32_t requestedTrack=-1);
};

struct MainMenuScreenServices;
struct FrontendCreditsContent;
struct FrontendCreditsText;
// Port-side registration for one host-owned frontend page and its menu entry.
struct FrontendMenuExtension {
    const char* label{};
    const char* description{};
    FlailerScreenConfig* screen{};
    void (*bind)(MainMenuScreenServices&,void*){};
    void (*service)(void*){};
    void* context{};
    FlailerScreenConfig* submenu{};
    FlailerScreenConfig* secondarySubmenu{};
};

struct MainMenuScreenServices {
    FlailerUiState* ui{};
    FlailerScreenList* screens{};
    FrontendPlayerManager* frontend{};
    const FrontendRaceSetupBindings* raceSetup{};
    const ::CarSoundBindings* sounds{};
    FrontendDemoMode* demoMode{};
    const FrontendMenuExtension* extension{};
    const FrontendCreditsContent* credits{};
    const FrontendMenuExtension* optionsExtension{};
    // Port-side Display toggle. A negative action reads, zero disables, one enables.
    bool (*subtitleSetting)(void*,int){};
    void* subtitleContext{};
    const FrontendMenuExtension* controlsExtension{};
};
// Actual source GP-C0FC main-menu screen lifetime. Bindings borrow existing
// services; activation owns one concrete NewMainMenu, never a second FRED.
void BindMainMenuScreen(MainMenuScreenServices& services);
void UnbindMainMenuScreen();
void MainMenuScreenCallback(ScreenMessage message,ScreenCallbackData* data);
extern FlailerScreenConfig fNewMainMenuScreen;
extern std::int32_t mainMenuClearEasterEggButtons;

// frontend.cpp's Title/Mumbo owners borrow the same services as MainMenu.
// No copy of the application movie, language, profiles or race setup lives here.
struct FrontendStartupServices {
    MainMenuScreenServices* menus{};
    ReplayIntegerLog* movieLog{};
    TexturePack* legalTextures{};
};
struct TitleMovieDescription {
    const char* filename;
    std::int32_t alternateBitmap;
};
const TitleMovieDescription& GetTitleMovieDescription(std::int32_t index,
    const LanguageManager& language,const ::DemoDiscManager& demo);
std::int32_t GetNumTitleMovies(const ::DemoDiscManager& demo);

class TitleScreen final {
public:
    TitleScreen(FrontendStartupServices& services,std::uint32_t screen);
    ~TitleScreen();
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    void HandleTick();
    // Actual final owner's state, not a diagnostic mirror.
    FlailerBitmapChunk* bitmap{};
    std::uint32_t screenKey{};
    std::int32_t movieIndex{};
    std::int32_t phase{};
    std::int32_t startedTicks{};
    std::int32_t inputVisibleTicks{};
private:
    bool CanAcceptInput() const;
    void OpenCurrentMovie();
    FrontendStartupServices& services_;
    std::unique_ptr<FlailerHighlightAnimation> highlight_;
};

class MumboJumbo final {
public:
    MumboJumbo(FrontendStartupServices& services,std::uint32_t screen);
    bool ShouldSeeLegalScreen() const;
    bool ShouldSeeAntiPiracy() const;
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    std::int32_t unused{};
    std::int32_t startedTicks{};
    FlailerBitmapChunk* bitmap{};
    std::int32_t phase{};
    std::uint32_t screenKey{};
private:
    FrontendStartupServices& services_;
};
void BindFrontendStartup(FrontendStartupServices& services);
void UnbindFrontendStartup();
void TitleScreenCallback(ScreenMessage message,ScreenCallbackData* data);
void MumboJumboCallback(ScreenMessage message,ScreenCallbackData* data);
void CallbackFinishedLegalScreen();
void CallbackFinishedAntiPiracyScreen();
extern FlailerScreenConfig fTitleScreen;
extern FlailerScreenConfig fMumboJumboScreen;
extern std::int32_t frontendLaunchType;
bool ShouldSeeAutoloadScreen(const ::DemoDiscManager& demo);
bool ShouldSeeLanguageSelectScreen(const FlailerUiState& ui,const ::DemoDiscManager& demo);
bool ShouldSeeLegalMumboJumbo();
bool ShouldSeeTitleSequence();
void LaunchFrontendMenus(FrontendStartupServices& services);
void UnlaunchFrontendMenus(FrontendStartupServices& services);

// Same mainmenu.cpp services, separate actual PAL Cops On/Off owner cells.
// CopsMenuState arranges each instance's authored animations/selection once;
// it neither copies the FRED/race state nor substitutes a screen callback.
struct CopsMenuState;
class CopsOnModeSelect final {
public:
    CopsOnModeSelect(MainMenuScreenServices& services,std::uint32_t screen);
    ~CopsOnModeSelect();
    void StartChangeToScreen(const char* destination);
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    void OnTick();
    void SetupHighlight(std::uint32_t group,std::int32_t languageId);
    void OnHighlight(ScreenMessage message,ScreenCallbackData& data);
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
    static void JoyHandlerBridge(std::int32_t port,std::int32_t event,
        std::uint8_t value,CopsOnModeSelect* owner);
    void JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value);
private:
    std::unique_ptr<CopsMenuState> state_;
};
class CopsOffModeSelect final {
public:
    CopsOffModeSelect(MainMenuScreenServices& services,std::uint32_t screen);
    ~CopsOffModeSelect();
    void StartChangeToScreen(const char* destination);
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    void OnTick();
    void SetupHighlight(std::uint32_t group,std::int32_t languageId);
    void OnHighlight(ScreenMessage message,ScreenCallbackData& data);
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
    static void JoyHandlerBridge(std::int32_t port,std::int32_t event,
        std::uint8_t value,CopsOffModeSelect* owner);
    void JoyHandler(std::int32_t port,std::int32_t event,std::int32_t value);
private:
    std::unique_ptr<CopsMenuState> state_;
};
struct MainMenuTrackDescription {
    std::int32_t trackNumber{};
    std::int32_t direction{};
};
MainMenuTrackDescription PickTrackNumberForExpressGame(const MainMenuScreenServices& services);
void StartQuickRace(MainMenuScreenServices& services,std::int32_t manualTransmission, int requestedTrack=-1);
extern FlailerScreenConfig fCopsOnModeSelectScreen;
extern FlailerScreenConfig fCopsOffModeSelecttScreen;

class FlailerAnimatedBlurb;
class FlailerSelectionSet;
struct OptionsMenuState {
    MainMenuScreenServices& services;
    std::uint32_t screen;
    FlailerSlideInAnimation slide;
    FlailerHighlightAnimation highlight;
    FlailerHighlightAnimation selectionChange;
    std::unique_ptr<FlailerSelectionSet> selection;
    std::unique_ptr<FlailerExitScreenAnimation> exit;
    OptionsMenuState(MainMenuScreenServices& bindings,std::uint32_t key);
};
// The original Options base owns its two highlight animations, slide,
// selection set and optional exit. Native virtual dispatch calls the real
// derived implementation; the source empty base HandleEvent is not emitted.
class OptionsScreenBase {
public:
    OptionsScreenBase(MainMenuScreenServices& services,std::uint32_t screen);
    virtual ~OptionsScreenBase();
    virtual void HandleEvent(ScreenMessage message,ScreenCallbackData& data)=0;
    virtual void HandleHighlight(ScreenMessage message,ScreenCallbackData& data,std::uint32_t group);
    void FirstCrackAtMessage(ScreenMessage message,ScreenCallbackData& data);
    static void BaseCallback(ScreenMessage message,ScreenCallbackData* data);
    // Port-side reuse of the original Options rail placement for new rows.
    static void PositionHighlightBars(FlailerUiState& ui,std::uint32_t screen,
        std::uint32_t group,float rowOffset=0.0f);
protected:
    void SetMenuHighlight(ScreenCallbackData& data,std::uint32_t group,float rowOffset=0.0f);
    void PositionHighlightBars(std::uint32_t group,float rowOffset=0.0f);
    static MainMenuScreenServices& BoundServices();
    static void Install(std::unique_ptr<OptionsScreenBase> instance);
    std::unique_ptr<OptionsMenuState> state_;
};
class OptionsScreen final : public OptionsScreenBase {
    class PortRow;
    std::unique_ptr<PortRow> portRow_;
public:
    OptionsScreen(MainMenuScreenServices& services,std::uint32_t screen);
    ~OptionsScreen() override;
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data) override;
    void HandleHighlight(ScreenMessage message,ScreenCallbackData& data,std::uint32_t group) override;
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
private:
    std::unique_ptr<FlailerAnimatedBlurb> blurb_;
    FlailerTextChunk* portControlsLabel_{};
    std::string originalControlsText_;
    std::uint32_t originalControlsLanguage_{};
    std::int32_t rumbleTimer_{};
};
// Original joystickconfiguration.cpp screen owners and typed analog bridge.
class JoystickCalibrationScreen;
void JoystickCalibrationInput(JoystickCalibrationScreen* owner,std::int32_t port,std::int32_t event,std::uint8_t value);
void BindJoystickScreens(MainMenuScreenServices& services);
void UnbindJoystickScreens();
std::array<FlailerScreenConfig,5>& JoystickScreenConfigurations();
// Original FrontendEffects.cpp overloads; shared reset flag or borrowed float phase.
void AnimateCursor(FlailerUiState& ui,FlailerBitmapChunk* first,FlailerBitmapChunk* second,std::int32_t reset);
void AnimateCursor(FlailerUiState& ui,FlailerBitmapChunk* first,FlailerBitmapChunk* second,float* phase);

class DisplayOptionsChangeManager final {
public:
    DisplayOptionsChangeManager();
    ~DisplayOptionsChangeManager();
    void Start(const FrontendPlayerManager& frontend);
    bool Different(const FrontendPlayerManager& frontend) const;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
extern DisplayOptionsChangeManager gDisplayOptionsChangeManager;
extern FlailerScreenConfig fOptions;
std::array<FlailerScreenConfig,3>& AudioDisplayScreenConfigurations();
std::array<FlailerScreenConfig,2>& CustomizeHUDScreenConfigurations();
std::array<FlailerScreenConfig,2>& DisplayCalibrationScreenConfigurations();
void InitializeDisplayCalibration(FrontendPlayerManager& frontend,const GameTime& clock);
void ApplyDisplayCalibration(const FrontendPlayerManager& frontend,const GameTime& clock);
// Individually mapped original helpers in RaceSetup/FrontendEffects.
void ArrowAligner(FlailerUiState&,std::uint32_t screen,std::uint32_t left,
    std::uint32_t right,std::uint32_t item);
void AnimateCursor(FlailerUiState&,FlailerBitmapChunk*,FlailerBitmapChunk*,std::int32_t reset);
void BindOptionsScreen(MainMenuScreenServices& services);
void UnbindOptionsScreen();

struct FlailerSlider {
    float displayScale{1.0f};
    FlailerUiState* ui{};
    std::uint32_t screenKey{};
    std::uint32_t menuItemKey{};
    std::uint32_t boundsObjectKey{};
    std::uint32_t thumbObjectKey{};
    std::int32_t channel{};
    float* setting{};
    float minimumDisplay{};
    float maximumDisplay{};
    tCubic1D position{};
    FlailerSlider()=default;
    FlailerSlider(FlailerUiState& state,std::uint32_t screen,std::uint32_t menuItem,
        std::uint32_t bounds,std::uint32_t thumb,std::int32_t audioChannel,
        float* value,float minimum,float maximum);
    float ValueToDisplay(float value) const;
    float DisplayToValue(float value) const;
    void StepValue(float step,bool updateMixer);
    void Animate();
};

// SelectionSet borrows the actual object nodes in the loaded screen. The
// list changes those objects' visibility; it has no second selection/color
// copy and owns no resource objects.
class FlailerSelectionSet final {
public:
    FlailerSelectionSet();
    void AddItem(FlailerObjectChunk* object);
    void AddItems(FlailerResources& resources,std::uint32_t screen,
        const std::uint32_t* keys,std::int32_t count);
    void SetCurrentItem(std::uint32_t key);
    std::array<FlailerObjectChunk*,32> items{};
    std::int32_t count{};
    FlailerObjectChunk* current{};
    std::int32_t mode{1};
    std::int32_t showOtherItems{};
};

// Original Options tSlideOnScreen owns this one interpolator and borrows
// the same screen/group/anchor nodes used by menu input and Flailer render.
class FlailerSlideOnScreen final {
public:
    FlailerSlideOnScreen(FlailerUiState& state,std::uint32_t screen,
        std::uint32_t anchor,std::uint32_t group,std::uint32_t visibilityScreen);
    ~FlailerSlideOnScreen();
    FlailerSlideOnScreen(const FlailerSlideOnScreen&)=delete;
    FlailerSlideOnScreen& operator=(const FlailerSlideOnScreen&)=delete;
    void Tick();
    void Appear();
    void Disappear();
    tCubic1D position{};
private:
    FlailerUiState& ui_;
    std::uint32_t screen_,anchor_,group_,visibilityScreen_;
    float savedX_{},savedY_{};
};

class FlailerLoadingScreenAnimation final {
public:
    FlailerLoadingScreenAnimation(FlailerUiState& ui,const ApplicationFrontendFlow& flow,
        const FrontendPlayerManager& frontend,const ::DemoDiscManager& demo,std::uint32_t screen);
    ~FlailerLoadingScreenAnimation();
    FlailerLoadingScreenAnimation(const FlailerLoadingScreenAnimation&)=delete;
    FlailerLoadingScreenAnimation& operator=(const FlailerLoadingScreenAnimation&)=delete;
    void Tick();
    std::uint32_t ScreenKey() const noexcept {return screenKey_;}
private:
    FlailerUiState& ui_;
    const ApplicationFrontendFlow& flow_;
    const FrontendPlayerManager& frontend_;
    std::uint32_t screenKey_;
    std::unique_ptr<FlailerHighlightAnimation> highlight_;
    FlailerTextChunk* gearNotice_{};
};

} // namespace hp2

#pragma once


#include "FlailerDB.hpp"
#include "bsound/include/bSample.h"
#include "indep/inc/Timer.hpp"
#include "FEAnim.hpp"
#include "CarSounds.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace hp2 {

class LanguageManager;

enum class ScreenMessage : std::int32_t {
    Select=1,ItemActivate=2,ItemDeactivate=3,Highlight=4,Unhighlight=5,
    Left=6,Right=7,Previous=8,Next=9,ItemEvent=10,
    ScreenActivate=11,ScreenDeactivate=12,ScreenBack=13,ScreenEvent8=14,
    ScreenEvent13=15,ScreenEvent15=16,LeaveScreen=17,Tick=18,RenderModels=19,
    PopupActivate=20,PopupDismiss=21,PopupPrevious=22,PopupNext=23,Input=24
};

// The source callback payload is four words and is message-specific. Keep
// the third word neutral: navigation uses it as a handled flag, activation
// as a one-based item selection, and item events carry the input event.
struct ScreenCallbackData {
    std::uint32_t key{};
    std::int32_t joystickPort{};
    std::int32_t value{};
    std::int32_t sound{};
};

struct FlailerSoundClock {
    std::uint32_t ticks{};
    float ticksPerSecond{};
};

struct FlailerSoundState {
    static constexpr std::size_t EventCount=31;
    struct Event { std::int32_t bankKind{};const char* name{};WorldAudioVoiceRef voice{}; };
    std::array<Event,EventCount> events{};
    WorldAudioRequestQueue* audio{};
    const std::int32_t* enabled{};
    const FlailerSoundClock* clock{};
    WorldAudioVoiceRef lastVoice{};
    std::uint32_t lastTicks{};
    WorldAudioResult result{WORLD_AUDIO_OK};
    const WorldAudioManagerState* manager{};
    SoundVolumeSettings* volumes{};
    FlailerSoundState();
    WorldAudioResult BindBank(std::int32_t kind,const WorldAudioBank* bank);
    WorldAudioResult ReleaseBank(std::int32_t kind);
    void Play(std::int32_t event);
};
void InitGlobalMenuSound(FlailerSoundState& sounds,const CarSoundBindings& bindings);
void InitFrontendSounds(FlailerSoundState& sounds,const CarSoundBindings& bindings);
void UnInitFrontendSounds(FlailerSoundState& sounds);

struct FlailerUiState {
    FlailerResources* resources{};
    FlailerSoundState* sound{};
    FlailerRenderManager* render{};
    LanguageManager* language{};
    const GameTime* clock{};
    // Borrow the one application movie and flow owners. Their lifetime must
    // outlast this UI; no decoder, movie clock or flow mirror lives here.
    MoviePlayer* movie{};
    const ApplicationFrontendFlow* flow{};
    std::int32_t ignoreJoystick{};
    std::int32_t wrapNavigation{1};
    // Retail PAL331530 starts enabled; only the campaign bonus state disables it.
    std::int32_t helpEnabled{1};
    std::array<std::int32_t,2> lastInputLoop{};
    std::int32_t twoPlayerScreenMode{};
    FlailerAnimationSettings animationSettings{};
    std::array<std::uint32_t,3> highlightedRgb{{128,64,0}};
    std::array<std::uint32_t,3> inactiveRgb{{24,24,24}};
    // Native render-call context, scoped by FlailerScreen::RenderModels.
    // Additional PC presentation frames rebuild geometry without advancing
    // source UI state or submitting another frontend sound event.
    bool advancePresentation{true};
    bool externalPresentation{};
    void SetIgnoreJoystick(std::int32_t ignored);
    void SetTwoPlayerScreenMode(std::int32_t enabled);
};

struct FlailerScreenConfig {
    const char* name{};
    ScreenCallback callback{};
    std::array<std::int32_t,4> screenIds{};
    std::array<std::int32_t,4> joystickPorts{{-1,-1,-1,-1}};
    std::int32_t screenCount{};
    const std::int32_t* helpItems{};
};

// The RAII owners below create the same menu/item rings used by the original
// methods; they do not mirror current selection or object color.
class FlailerMenuStorage final {
public:
    FlailerMenuStorage(FlailerUiState& ui,FlailerScreen& screen,std::int32_t selected,
        const char* itemFormat="MENUITEM_%d");
    ~FlailerMenuStorage();
    FlailerMenu& Menu() noexcept { return menu_; }
private:
    FlailerMenu menu_{};
    std::vector<std::unique_ptr<FlailerMenuItem>> items_;
};

class FlailerScreenStorage final {
public:
    FlailerScreenStorage(FlailerUiState& ui,FlailerScreenConfig& configuration);
    ~FlailerScreenStorage();
    FlailerScreenNode& Node() noexcept { return node_; }
private:
    FlailerScreenNode node_{};
    std::vector<std::unique_ptr<FlailerScreen>> screens_;
};

} // namespace hp2

// Native declarations arranged with the proven frontend owner. Bodies retain
// individual StartRaceScreens/EndGameScreens CPP placement; no absent beta
// StartRaceScreens.hpp is invented by this arrangement.
struct RaceFrame;
struct GameSession;
struct CarFramePlayerRegistry;
struct TrackScenarioDefinition;
struct ReplayManager;
namespace hp2 {
class TrackInfoResources;
struct RaceScreenServices {
    MainMenuScreenServices* frontend{};
    ::RaceFrame* race{};
    ::CarFramePlayerRegistry* players{};
    ::GameSession* coordinator{};
    const ::TrackScenarioDefinition* currentTrack{};
    ::ReplayManager* replay{};
};
extern std::array<std::array<float,4>,2> endGamePlayerColors;
extern std::array<float,4> endGameOpponentColor;
extern std::array<float,4> startRaceEliminatedColor;
std::uint32_t SprintfAndHash(const char* format,std::int32_t value);
std::int32_t GetTheRightTrackLanguageID(const ::TrackScenarioDefinition* track,std::int32_t direction);
std::int32_t GetTheRightTrackLanguageID(const TrackInfoResources& tracks,
    std::int32_t track,std::int32_t direction);
void SetRaceTrackCaption(RaceScreenServices& services,std::uint32_t screen,
    std::uint32_t trackLabel,std::uint32_t roundLabel,std::int32_t increment);
class StartRaceScreens final {
public:
    explicit StartRaceScreens(RaceScreenServices& bindings):services(bindings) {}
    ~StartRaceScreens() noexcept;
    std::exception_ptr Shutdown() noexcept;
    std::int32_t NeedToShowRaceObjective() const;
    std::int32_t NeedToShowRaceObjective2P() const;
    std::int32_t NeedToShowRaceStats() const;
    void AddScreen(const char* name);
    void Enable();
    void Disable();
    RaceScreenServices& services;
    std::array<const char*,4> screens{};
    std::int32_t screenCount{};
    std::int32_t current{};
private:
    std::exception_ptr closeFailure_;
};
struct PreRaceStat {
    std::int32_t position{};
    const char* name{};
    std::int32_t model{};
    std::int32_t status{};
    std::int32_t points{};
    std::array<float,4> color{};
};
class PreRaceStats final {
public:
    PreRaceStats(StartRaceScreens& manager,std::uint32_t screen);
    ~PreRaceStats();
    std::int32_t BuildStatsDefault();
    void BuildStatsTournament();
    void BuildStatsRaceKnockout();
    PreRaceStat* GetStatsForRacePosition(std::int32_t position);
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
    std::array<PreRaceStat,8> stats{};
private:
    StartRaceScreens& manager_;
    std::uint32_t screen_;
};
class PreRaceBlurb final {
public:
    PreRaceBlurb(StartRaceScreens& manager,std::uint32_t screen);
    ~PreRaceBlurb() noexcept;
    std::exception_ptr Close() noexcept;
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
private:
    StartRaceScreens& manager_;
    std::uint32_t screen_;
    bool closed_{};
    std::exception_ptr closeFailure_;
};
class PreRace2PBeACop final {
public:
    PreRace2PBeACop(StartRaceScreens& manager,std::uint32_t screen);
    ~PreRace2PBeACop();
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
private:
    StartRaceScreens& manager_;
    std::uint32_t screen_;
};
const char* FigureOutRaceObjective(RaceScreenServices& services,std::int32_t player);
extern FlailerScreenConfig fPreRaceBlurbScreen;
extern FlailerScreenConfig fPreRace2PBeACopScreen;
void EnableStartRaceScreens(StartRaceScreens& owner);
void DisableStartRaceScreens(StartRaceScreens& owner);
extern FlailerScreenConfig fPreRaceStatsScreen;
extern FlailerScreenConfig fPreRaceLetterboxScreen;
void PreRaceLetterboxCallback(ScreenMessage message,ScreenCallbackData* data);
void BindStartRaceScreens(StartRaceScreens& owner);
void UnbindStartRaceScreens();
bool ReadyToSkipStartRaceScreens(const StartRaceScreens& owner);
}

// Individually proven PauseMenu.hpp owns these declarations since51.
#include "PauseMenu.hpp"

// Individually proved original header; contains the single manager definition.
#include "EndGameScreens.hpp"

// Original MemcardScreens.cpp owns the serialized transfer image and deferred
// completion. The one live profile remains FrontendPlayerManager.
#include <functional>
#include <string>
namespace hp2 {
class MemcardScreens final {
public:
    enum class LoadResult {Loaded,Absent,Failed};
    MemcardScreens(FrontendPlayerManager& frontend,const char* utf8Directory=nullptr,bool requireNamedProfile=false);
    ~MemcardScreens();
    MemcardScreens(const MemcardScreens&)=delete;
    MemcardScreens& operator=(const MemcardScreens&)=delete;
    LoadResult LoadProfile(const char* filename=nullptr);
    bool SaveProfile();
    bool RequestAutosave(std::function<void()> completed);
    void Service();
    bool Busy() const noexcept;
    const std::string& LastError() const noexcept;
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    // Port-side creation continuation using the existing NameEntry page.
    bool BootNameEntryActive() const noexcept;
    bool CreateBootProfile(const char* name);
    void FinishBootNameEntry(bool saved);
private:
    void ApplyProfileOptions();
    friend void BindMemcardScreens(MemcardScreens&,MainMenuScreenServices&);
    friend void UnbindMemcardScreens();
    friend bool RequestProfileAutosave(std::function<void()>,std::function<bool()>);
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
void BindMemcardScreens(MemcardScreens& owner,MainMenuScreenServices& services);
void UnbindMemcardScreens();
MemcardScreens* GetBootNameEntryOwner();
bool RequestProfileAutosave(std::function<void()> completed,std::function<bool()> ownerAlive={});
std::array<FlailerScreenConfig,4>& MemcardScreenConfigurations();
void MemcardScreenCallback(ScreenMessage message,ScreenCallbackData* data);
}

namespace hp2 {
std::array<FlailerScreenConfig,2>& LanguageScreenConfigurations();
void LanguageOptionsScreenCallback(ScreenMessage message,ScreenCallbackData* data);
}

namespace hp2 {
void BindRaceSetupScreens(MainMenuScreenServices&);
void UnbindRaceSetupScreens();
std::array<FlailerScreenConfig,7>& RaceSetupScreenConfigurations();

}

namespace hp2 {
void BindNameEntryScreen(MainMenuScreenServices&);
void UnbindNameEntryScreen();
void NameEntryScreenCallback(ScreenMessage,ScreenCallbackData*);
enum class FrontendNameEditAction;
std::uint64_t ActiveNameEditSession();
void ApplyNameEditCommand(FrontendNameEditAction action, std::uint32_t character);
extern FlailerScreenConfig fNameEntry;
void BindCreditsScreens(FrontendStartupServices&);
void UnbindCreditsScreens();
std::array<FlailerScreenConfig,7>& CreditsScreenConfigurations();
FlailerScreenConfig& PortCreditsScreenConfiguration();
void GetPortCreditsText(std::vector<FrontendCreditsText>& text);
}
