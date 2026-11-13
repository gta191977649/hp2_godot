#pragma once
#include "indep/inc/Replay.hpp"
#include "indep/inc/RaceParameters.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct GameTime;
struct ApplicationLaunchOptions;
struct CarDriverAppearanceCatalogue;
struct WorldSimulation;
struct GameInputRouter;
struct bSound;
class DemoDiscManager;
namespace hp2 { class GameResources; class NativeApplicationHost; }

namespace hp2 {
// The actual six filename-pointer values remain mutable configuration. The
// archive supplies the five named JUL23 recordings; index0 may be absent.
extern std::array<std::string,6> demoModeReplayFilenames;

class DemoModeReplay final {
public:
    DemoModeReplay();
    DemoModeReplay(const DemoModeReplay&)=delete;
    DemoModeReplay& operator=(const DemoModeReplay&)=delete;
    ~DemoModeReplay();
    const std::string& GetBaseFilename(std::int32_t index) const;
    std::string GenerateFilename(std::int32_t index) const;
    void LoadResourceFile(GameResources& resources,const GameTime& clock);
    void UnloadResourceFile();
    void RestoreRaceParametersSnapshot(RaceParameters& parameters,
        const ApplicationLaunchOptions& options,const std::int32_t& language,
        const CarDriverAppearanceCatalogue& characters);
    bool Loader(std::uint32_t id,std::uint8_t* payload,std::size_t bytes,const GameTime& clock);
    bool Unloader(std::uint32_t id,const std::uint8_t* payload);
    ReplaySnapshot* FirstSnapshot() noexcept;
    bool HasLoadedResource() const noexcept { return loaded_; }
    bool IsLoadPending() const noexcept { return loadPending_; }
    bool IsReplayDataBorrowed() const noexcept { return replayBorrower_!=nullptr; }
    std::size_t RecordedFrameCount() const noexcept {return streamIndex_.FrameCount();}
    bool ConsiderLaunching(GameResources& resources,const GameTime& clock,
        const DemoDiscManager& demoDisc);
    void Launch();
    void BeginDemoMode(GameSession& coordinator);
    bool ServiceDemoMode(GameSession& coordinator,const GameTime& clock);
    void AdvanceSnapshot(bool forward,GameSession& coordinator);
    // Native FIFO adapter: the Demo clock cannot expire an enqueued snapshot
    // before coordinator message14 has restored its source loop count.
    void SnapshotRestoreCompleted(const ReplaySnapshot& snapshot);
    bool IsSnapshotRestorePending() const noexcept { return snapshotRestorePending_; }
    void EndDemoMode() noexcept;
    bool CanTestDemoMode() const;
    void BeginDemoModeTest(GameSession& coordinator);
    void BeginRecordingSnapshot() noexcept;
    void EndRecordingSnapshot(bool discard,const GameTime& clock);
    bool IsRecordingTest() const noexcept { return recordingTest_!=0; }
    bool IsRecordingSnapshot() const noexcept { return recordingSnapshot_!=nullptr; }
    bool IsTestingSnapshot() const noexcept { return testSnapshot_!=nullptr; }
    void SetRecordingTest(bool value) noexcept { recordingTest_=value?1:0; }
    void ServiceRecording(bool snapshotTime,ReplayManagerOwner& replay,const GameTime& clock);
    // Native file boundary for the original Save13F7E8 chunk order. Only
    // explicitly supplied destinations are writable; original disc resources
    // and current profile files are never overwritten as a side effect.
    void SaveRecordingToFile(const std::filesystem::path& path,const RaceParameters& parameters) const;
    void RequestSaveRecording(const std::filesystem::path& path);
    void RequestEraseRecording();
    void BindSessionHost(NativeApplicationHost* host);
    // An optional PAL file-specific relocation. It is never inferred from
    // a small integer or a current host address. No known Player callback
    // token occurs in the five shipped recordings; any encountered non-null
    // address must still resolve explicitly rather than being guessed.
    void SetSourcePlayerTokens(const std::array<std::uint32_t,2>& tokens);
    std::size_t SourceSegmentCount() const;
    void RecordSessionEvent(const char* event,const ReplaySnapshot* snapshot=nullptr,
        const char* detail=nullptr) const noexcept;
    std::int32_t active{};
    std::int32_t selectedRecording{};
    // Source+24, independent of the selected recording at+0C.
    std::int32_t forcedRecording{-1};
private:
    struct ResourceFile;
    struct SessionDiagnostics;
    friend class ::ReplayManagerOwner;
    void ValidateSnapshotReader(const ReplaySnapshot& snapshot,const RaceParameters& parameters,
        const CarDriverAppearanceCatalogue& characters) const;
    std::vector<std::unique_ptr<ResourceFile>> resources_;
    std::vector<std::unique_ptr<ReplaySnapshotStorage>> snapshots_;
    std::unique_ptr<BitStream> repeat_;
    ReplayStreamIndex streamIndex_;
    ReplayManagerOwner* replayBorrower_{};
    std::uint8_t* metadata_{};
    std::size_t metadataBytes_{}; // Bound for source-backed v8 metadata before native serialization.
    bool loadPending_{};
    bool loaded_{};
    // Source +10/+14/+18/+1C/+20/+28/+2C are distinct from the input file
    // BitStream flags. Snapshot pointers borrow ReplayManager's real list.
    std::int32_t recordingRequested_{};
    std::int32_t recordingTest_{};
    ReplaySnapshot* recordingSnapshot_{};
    std::int32_t currentSnapshotIndex_{};
    ReplaySnapshot* testSnapshot_{};
    bool snapshotRestorePending_{}; // Native queue-order guard, not PS2 snapshot data.
    std::int32_t saveRequested_{};
    std::filesystem::path saveDestination_;
    std::int32_t eraseRequested_{};
    std::array<std::uint32_t,2> sourcePlayerTokens_{};
    std::unique_ptr<SessionDiagnostics> diagnostics_;
};
} // namespace hp2
