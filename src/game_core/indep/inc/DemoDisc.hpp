#pragma once

#include <cstdint>

struct GameTime;
struct ApplicationLaunchOptions;
namespace hp2 { enum class CarInfoAvailability; }

// Native settings for the release's six demo tweakables. Sony launcher
// arguments and packed console records are not Windows configuration data.
struct DemoDiscConfiguration {
    bool useTweakables=false;
    std::uint16_t language=0;
    std::uint16_t aspect=0;
    std::uint16_t playMode=0;
    std::uint16_t inactiveTimeoutSeconds=60;
    std::uint16_t totalTimeoutSeconds=600;
    std::uint16_t masterVolumeScale=10;
};

struct DemoDiscParameters {
    std::uint16_t language;
    std::uint16_t aspect;
    std::uint16_t playMode;
    std::uint16_t inactiveTimeoutSeconds;
    std::uint16_t totalTimeoutSeconds;
    std::uint16_t mediaType;
    std::uint16_t masterVolumeScale;
    std::uint16_t directorySector;
};

// One application-lifetime content policy. Native member layout is not a
// binary overlay of the original 48-byte console object.
class DemoDiscManager final {
public:
    DemoDiscManager();
    void Init(const GameTime& clock,ApplicationLaunchOptions& launch,
        const DemoDiscConfiguration& configuration);
    void ForceActive(const GameTime& clock,ApplicationLaunchOptions& launch,
        const DemoDiscConfiguration& configuration);

    std::int32_t ActivationWord() const noexcept { return activation_; }
    bool IsAttractMode() const noexcept;
    hp2::CarInfoAvailability Availability() const noexcept;
    const DemoDiscParameters& Parameters() const noexcept { return parameters_; }
    // ReplayScreen event108 writes the original manager +1E/+20 cells.
    // This accessor updates the existing policy, never a replay-only copy.
    void SetReplayRecordingTimeouts() noexcept {
        parameters_.inactiveTimeoutSeconds=32767;parameters_.totalTimeoutSeconds=32767;
    }
    std::int32_t InitialRealTime() const noexcept { return initialRealTime_; }
    std::int32_t LastRealTime() const noexcept { return lastRealTime_; }
    // GenerateJoyEvents writes this same source+0C cell after real scanner
    // activity, only when the sampled real time is newer.
    void NoteInputTime(std::int32_t ticks) noexcept { if (lastRealTime_<ticks)lastRealTime_=ticks; }

    void SetEndReason(std::int32_t reason) noexcept;
    std::int32_t EndReason() const noexcept { return endReason_; }
    // Windows consumes this request in its real application shutdown path;
    // returning to the Sony browser or stopping forever is not required.
    void EndDemo() noexcept;
    bool ExitRequested() const noexcept { return exitRequested_; }

private:
    DemoDiscParameters parameters_;
    std::int32_t activation_;
    std::int32_t initialRealTime_;
    std::int32_t lastRealTime_;
    std::int32_t endReason_;
    bool exitRequested_;
};

extern DemoDiscManager TheDemoDiscManager;
