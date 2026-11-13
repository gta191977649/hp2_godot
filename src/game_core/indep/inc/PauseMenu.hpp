#pragma once
#include "FMenus.hpp"
#include <exception>

struct CarFramePlayerState;
namespace hp2 {
// Existing display methods retain their actual live Player/settings owners.
class PauseScreenBase {
public:
    PauseScreenBase(RaceScreenServices& services,std::uint32_t screen);
    virtual ~PauseScreenBase();
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
protected:
    RaceScreenServices& services_;
    std::uint32_t screen_;
    FlailerHighlightAnimation highlight_;
    FlailerHighlightAnimation valueHighlight_;
    FlailerLeftRightArrowScroll arrows_;
};
class PauseDisplayOpts final : public PauseScreenBase {
public:
    PauseDisplayOpts(RaceScreenServices& services,std::uint32_t screen,std::int32_t port);
    ~PauseDisplayOpts() override;
    void MenuCallback(ScreenMessage message,ScreenCallbackData& data);
    void DoGrayout();
    bool ShouldSeeRearViewMirror() const;
    bool ShouldSeeScoring() const;
    bool ShouldSeeLeaderboard() const;
    bool ShouldSeePosition() const;
    bool ShouldSeeSplitTime() const;
    bool ShouldSeeTrackMap() const;
    bool ShouldSeeLapInfo() const;
    bool ShouldSeeGauges() const;
    void AdjustLapInfo();
    void DisplayTrackmap();
    bool CycleTrackMap(std::int32_t direction);
    bool ToggleHudHotKey();
    void DisplayHudHotKey();
    bool CycleHUD(std::int32_t direction);
    void DisplayHUD();
    void DisplayGauges();
    bool ToggleGauges();
    void DisplayLapInfo();
    bool ToggleLapInfo();
    void DisplaySplitTime();
    bool CycleSplitTime(std::int32_t direction);
    void DisplayRacePosition();
    bool ToggleRacePosition();
    void DisplayLookback();
    bool ToggleLookback();
    void DisplayCameraSpring();
    bool ToggleCameraSpring();
    void DisplayRearview();
    bool ToggleRearView();
    bool CycleLeaderboard(std::int32_t direction);
    void DisplayLeaderboard();
    bool CycleScoring(std::int32_t direction);
    void DisplayScoring();
    ::CarFramePlayerState& Player() const;
private:
    ::CarFramePlayerState* player_{};
};

class PauseOptions final : public PauseScreenBase {
public:
    PauseOptions(RaceScreenServices& services,std::uint32_t screen,std::int32_t port);
    ~PauseOptions() override;
    bool SetVibration(ScreenMessage direction);
    bool SetDamage(ScreenMessage direction);
    bool SetDrivingStyle(ScreenMessage direction);
    bool SetTransmission(ScreenMessage direction);
    bool SetJumpCam(ScreenMessage direction);
    bool SetZoneCam(ScreenMessage direction);
    bool SetAltCam(ScreenMessage direction);
    void DisplayVibration();
    void DisplayDamage();
    void DisplayDrivingStyle();
    void DisplayTransmission();
    void DisplayJumpCam();
    void DisplayZoneCam();
    void DisplayAltCam();
    void MenuCallback(ScreenMessage message,ScreenCallbackData& data);
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
private:
    void UpdateCameraOptionsVisibility(); // Complete final172260 dependency; no invented MAP method claim.
    ::CarFramePlayerState& Player() const;
    ::CarFramePlayerState* player_{};
    std::int32_t port_;
    std::array<FlailerTextChunk*,7> labels_{};
};
class PauseAudioOpts final : public PauseScreenBase {
public:
    PauseAudioOpts(RaceScreenServices& services,std::uint32_t screen);
    ~PauseAudioOpts() override;
    std::exception_ptr Close() noexcept;
    void Cancel() noexcept;
    FlailerSlider* FindSlider(std::uint32_t item);
    bool ChangeAudioMode(ScreenMessage direction);
    void RedrawAudioMode(std::int32_t mode);
    void MenuCallback(ScreenMessage message,ScreenCallbackData& data);
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
private:
    // The source screen's seven editing cells are committed on destruction.
    // They are not a second persistent mixer owner.
    std::array<float,7> editingVolumes_{};
    std::vector<std::unique_ptr<FlailerSlider>> sliders_;
    bool closed_{};
    std::exception_ptr closeFailure_;
};
class PauseRaceObjective final {
public:
    PauseRaceObjective(RaceScreenServices& services,std::uint32_t screen);
    ~PauseRaceObjective();
    void HandleEvent(ScreenMessage message,ScreenCallbackData& data);
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
private:
    RaceScreenServices& services_;
    std::uint32_t screen_;
    std::int32_t onePlayer_;
};

// Individual PAL173E30/174058/1740C0/174198/174518/174590 bodies;
// the PC process switch exposes the retained authored debug screen.
class PauseDebugMenu final {
public:
    PauseDebugMenu(RaceScreenServices& services,std::uint32_t screen);
    ~PauseDebugMenu();
    std::exception_ptr Close() noexcept;
    void Cancel() noexcept;
    void MenuCallback(ScreenMessage message,ScreenCallbackData& data);
    void OnMenuItemScroll(ScreenMessage message,ScreenCallbackData& data);
    bool ToggleSuperEasyAI();
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
private:
    RaceScreenServices& services_;
    std::int32_t finishPosition_{};
    std::uint32_t screen_{};
    bool closed_{};
    std::exception_ptr closeFailure_;
};

class PauseMenu final {
public:
    explicit PauseMenu(RaceScreenServices& services);
    ~PauseMenu();
    std::exception_ptr Shutdown() noexcept;
    void EnterPause(std::int32_t port);
    void ExitPause();
    void Activate(std::uint32_t screen);
    void DeActivate();
    bool AllPlayerJoysticksPluggedIn() const;
    bool MaybeResume(std::int32_t port);
    void MaybeRestart();
    void MaybeQuit();
    void MenuCallback(ScreenMessage message,ScreenCallbackData& data);
    std::int32_t GetState() const noexcept {return state_;}
    std::int32_t GetPortThatPaused() const noexcept {return pausingPort_;}
    std::int32_t GetOptionsPort() const noexcept {return optionsPort_;}
    std::int32_t GetPortThatHitReplay() const noexcept {return replayPort_;}
    ::CarFramePlayerState* GetPlayerThatPaused() const noexcept {return player_;}
    RaceScreenServices& Services() noexcept {return services_;}
private:
    RaceScreenServices& services_;
    std::int32_t state_{};
    std::uint32_t screen_{};
    std::int32_t pausingPort_{-1};
    std::int32_t optionsPort_{-1};
    std::int32_t replayPort_{-1};
    ::CarFramePlayerState* player_{};
    FlailerHighlightAnimation highlight_;
};
void BindPauseMenu(PauseMenu& owner);
void UnbindPauseMenu();
void PauseMenuCallback(ScreenMessage message,ScreenCallbackData* data);
void PauseDisplayCallback(ScreenMessage message,ScreenCallbackData* data);
extern FlailerScreenConfig fPauseMenuScreen;
extern FlailerScreenConfig fPauseDisplayScreen;
extern FlailerScreenConfig fPauseOptionsScreen;
extern FlailerScreenConfig fPauseControlsScreen;
extern FlailerScreenConfig fPauseAudioScreen;
extern FlailerScreenConfig fPauseRaceObjectiveScreen;
extern FlailerScreenConfig fPauseRaceObjective2PScreen;
extern FlailerScreenConfig fPauseDebugMenu;
}

struct ReplaySpecialEventInfo;
namespace hp2 {
int HandlePauseMenuReplaySpecialEvents(RaceScreenServices& services,const ::ReplaySpecialEventInfo& event);
}
