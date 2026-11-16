#pragma once

#include "FMenus.hpp"
#include <array>
#include <cstdint>
#include <optional>

struct CarChaseCamera;
struct CarFramePlayerState;
struct CarViewMover;
struct GameInputRouter;
struct GameTime;
struct ReplaySnapshot;
class ReplayManagerOwner;

namespace hp2 {
class DemoModeReplay;
struct ReplayPointerState;

// The original ReplayScreen.hpp/ReplayScreen.cpp owner. Services borrow the
// actual session, snapshots, Flailer objects and world cameras. The selected
// player is the existing GameSessionViewSelection cell, not a second seat.
class ReplayScreen final {
public:
    ReplayScreen(RaceScreenServices& services,std::int32_t seat);
    ~ReplayScreen() noexcept;
    ReplayScreen(const ReplayScreen&)=delete;
    ReplayScreen& operator=(const ReplayScreen&)=delete;

    // Checked host retirement executes the source destructor before reset.
    // The noexcept destructor still detaches every callback/camera if Close
    // reports a physical audio error during an exceptional parent teardown.
    void Close();
    void Tick();
    void Update();
    void ChangeCameraMover();
    void CalculateTimelineMarkers(bool visible);
    bool ShouldWeBeWatchingNis() const;
    void StopWatchingNis();
    bool HasCameraMover() const noexcept;
    void AddJoyHandlers(std::int32_t port);
    void RemoveJoyHandlers(std::int32_t port) noexcept;
    void JoyHandler(std::int32_t port,std::int32_t event,std::uint8_t value);
    static void JoyHandlerBridge(std::int32_t port,std::int32_t event,std::uint8_t value,ReplayScreen* owner);
    std::int32_t GetTimelinePositionX(float seconds) const;
    void SetFlailerBitmapPosition(FlailerBitmapChunk* bitmap,float seconds) const;
    void Pause(std::int32_t paused);
    // Port-side timeline input and committed UI state.
    ReplayPointerState PointerState() const;
    void Seek(float position,bool preview);
    void CancelSeek();
    bool Seeking() const noexcept { return seek_.has_value(); }

private:
    struct TimelineMarker {
        float start{},end{};
        FlailerBitmapChunk* bitmap{};
        ReplaySnapshot* snapshot{};
    };
    FlailerUiState& Ui() const;
    CarFramePlayerState& Player() const;
    GameInputRouter& Input() const;
    ReplayManagerOwner& Replay() const;
    DemoModeReplay& Demo() const;
    const GameTime& Clock() const;
    void RetireCameras();
    void MuteReplay(bool paused);
    void PlayCue(const char* name);
    void Post(std::int32_t message,std::intptr_t argument);
    RaceScreenServices& services_;
    float startSeconds_{},endSeconds_{};
    std::int32_t timelineLeft_{10},timelineWidth_{100};
    std::int32_t hidden_{},cameraSelection_{},lastJoyLoop_{},stoppedAtTicks_{};
    std::array<TimelineMarker,16> markers_{};
    std::array<TimelineMarker,16> segments_{};
    std::size_t markerCount_{},segmentCount_{};
    FlailerBitmapChunk* cursor_{};
    std::array<FlailerObjectChunk*,3> indicators_{};
    CarChaseCamera* chase_{};
    CarViewMover* demoCamera_{};
    CarViewMover* nisCamera_{};
    std::array<std::uint32_t,2> registered_{};
    bool screenVisible_{},closed_{};
    struct SeekState {
        std::int32_t ticks;
        float position;
        bool preview;
    };
    struct PlaybackMode {
        std::int32_t paused, slowMotion, lastTarget;
    };
    std::optional<SeekState> seek_;
    std::optional<PlaybackMode> scrubPlayback_;
};
} // namespace hp2
