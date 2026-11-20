#ifndef NFSHP2_FLAGS_HPP
#define NFSHP2_FLAGS_HPP
#include <stdint.h>

/* One native representation of the actual Config.cpp launch cells. The
 * prefix is retained for existing AI/pursuit/frontend borrowers. These are
 * distinct source globals from the live RaceParameters configuration. */
typedef struct ApplicationLaunchOptions {
    int32_t skipFrontend;
    float timeUntilCopsAppear;
    int32_t forcedOpponentModel;
    uint8_t startupCameraView;
    int32_t startupPlayerCars;
    int32_t startupAICars;
    int32_t startupLaps;
    int32_t startupRaceType;
    int32_t startupMaxCops;
    int32_t startupTickets;
    int32_t startupRoadBlocks;
    int32_t startupHelicopter;
    int32_t startupBeACop;
    float startupTrafficDensity;
    float startupOncomingTraffic;
    int32_t startupDifficulty;
    int32_t startupJumpCamera;
    int32_t startupZoneCamera;
    int32_t startupPlayerModel[2];
    int32_t startupPlayerSkin[2];
    int32_t allCarsAvailable;
    int32_t tunePreRaceSequence;
    int32_t demoTrackDirection;
    float beginDemoSeconds;
    float demoDurationSeconds;
    int32_t debugCamerasEnabled;
    int32_t engineSoundEnabled;
    int32_t musicEnabled;
    int32_t skipMovies;
    int32_t joyEventLoggingEnabled;
    // Config.obj bVersionE3, PAL32FE38; distinct from demo launcher state.
    int32_t bVersionE3;
} ApplicationLaunchOptions;

#ifdef __cplusplus
class DemoDiscManager;
struct SpeechCaptionState;
extern "C" {
#endif
extern ApplicationLaunchOptions applicationLaunchOptions;
// Fatal source errors share the native crash reporter, never a live log.
void hp2NativeRecordFailure(const char* reason);

/* The original Main.cpp cell, separate from launch defaults and the current
 * race's selectedScenario. LoadConfig is its actual file/replay producer. */
typedef struct ApplicationBootState {
    int32_t trackSelectedInConfigFile;
} ApplicationBootState;
extern ApplicationBootState applicationBootState;
#ifdef __cplusplus
}

#include "Joylog.hpp"
#include <cstdint>
#include <chrono>
#include <array>
#include <filesystem>
#include <memory>
#include <exception>
#include <optional>
#include <string>
#include <vector>
#include "CarDriver.hpp"
#include "NativeRenderSettings.hpp"
JoylogConfigItems applicationJoylogConfigBindings();
// Explicit PC test switches requested for inspecting the original game.
// These are process options, not saved FRED progress or Joylog config cells.
extern bool applicationDebugToolsEnabled;
// Port-side gate for race overrides when the host selects an isolated profile.
extern bool applicationDebugRaceOverridesEnabled;
// Independent native UI preference; never enables the source debug menu.
extern bool applicationShowFrameRate;
extern bool applicationUnlockAllForTesting;
extern bool applicationDebugExitRequested;
extern int32_t applicationScreenPrintf; // Config DoScreenPrintf, PAL32FED8.
extern float applicationPreviousCpuFrameRate;
extern float applicationPreviousGpuFrameRate;
extern bool applicationGpuTimingAvailable;
// Native diagnostic observations, never simulation or source-clock inputs.
// FPS counts accepted presentation requests over complete application time;
// CPU/GPU work durations are separate quantities, not reciprocal frame rates.
struct ApplicationFrameTiming {
    double framesPerSecond{},frameMilliseconds{},cpuMilliseconds{},gpuMilliseconds{};
    std::uint64_t presentedFrames{},cpuSamples{},gpuSamples{};
    bool frameRateAvailable{},cpuTimeAvailable{},gpuTimeAvailable{};
};
extern ApplicationFrameTiming applicationFrameTiming;
void applicationConfigureTesting(bool debugTools,bool unlockAll,bool allowRaceOverrides);
void applicationSeedRandomNumber(ReplayIntegerLog& log);
void applicationLoadConfig(const std::filesystem::path& filename,ReplayIntegerLog& log);

struct WorldSimulation;
namespace hp2 {struct FlailerScreenList;struct ApplicationFrontendFlow;}
// Main.cpp's separate checksum cadence cell (released GP-5AC0). Clock,
// RNG, world and frontend state remain borrowed from their actual owners.
struct ApplicationChecksumState {uint32_t calls{};};
void applicationVerifyJoylogChecksum(ApplicationChecksumState& state,ReplayIntegerLog& log,
    WorldSimulation* simulation,const hp2::FlailerScreenList* screens,
    const hp2::ApplicationFrontendFlow& flow);

struct CarCinematicServices;
struct EditorMarkerTexture;
struct CarViewCameraResources;
struct WorldSimulation;
struct Scenery;
struct CarCinematicViewFrame;
struct Car;
struct CarViewMoverRegistry;
namespace hp2 {
class GameResources;
class ResourceRegistry;
class ResourceLoader;
class QueuedFileService;
class FileIoRuntime;
class AcidFXRuntime;
enum class CarInfoAvailability;
struct ResourceFileBindings;

// Native lifetime arrangement for the source application's shared resource
// services. Cameras and physical I/O are required actual application owners.
// Construction initializes empty list/pool owners, not loaded game content;
// LoadGlobalChunks/World/CarFactory remain real separate game procedures.
class ApplicationResourceRuntime final {
public:
    ApplicationResourceRuntime(const GameResources& files,const CarCinematicServices& cameras,ReplayIntegerLog& log,
        CarInfoAvailability availability);
    ApplicationResourceRuntime(const GameResources& files,const CarCinematicServices& cameras,ReplayIntegerLog& log,
        const DemoDiscManager& demo);
    ~ApplicationResourceRuntime();
    ApplicationResourceRuntime(const ApplicationResourceRuntime&)=delete;
    ApplicationResourceRuntime& operator=(const ApplicationResourceRuntime&)=delete;
    ResourceRegistry& Registry() noexcept;
    ResourceLoader& Loader() noexcept;
    QueuedFileService& QueuedFiles() noexcept;
    FileIoRuntime& FileIo() noexcept;
    AcidFXRuntime& FX() noexcept;
    CarViewCameraResources& CameraResources() noexcept;
    ResourceFileBindings GlobalFileBindings() const;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}

#include "bsound/include/bSample.h"
struct bSound;
struct GameTime;
namespace hp2 {
class EcstasyFrame;
struct EcstasyVulkanFrameStatistics;
extern "C++" {struct FrontendPlayerManager;struct ApplicationFrontendFlow;}
std::filesystem::path nativeExecutablePath();
// Launcher preferences are a PC adapter. This file is separate from the
// original config.bin replay cells and from earned FRED progress.
enum class NativeTextureFilter {Authored,Bilinear,Trilinear,Anisotropic};
enum class NativeAntialiasing {Off,Fxaa,Msaa2,Msaa4,Msaa8};
struct NativeGameplayOptions {
    bool racerNitro{},trafficVehicles{},disableCatchUp{},cockpitView{};
};
extern NativeGameplayOptions applicationNativeGameplay;
struct NativePcSettings {
    std::int32_t width{1280},height{720};
    bool fullscreen{},vsync{true};
    std::string monitorDevice;
    std::int32_t frameLimit{}; // Zero leaves native presentation uncapped.
    NativeTextureFilter textureFilter{NativeTextureFilter::Trilinear};
    NativeAntialiasing antialiasing{NativeAntialiasing::Off};
    std::int32_t anisotropy{16};
    NativeRenderSettings render{};
    NativeGameplayOptions gameplay{};
    NativeInputSettings input{};
    bool debugTools{},unlockAll{},skipMovies{};
    bool showFps{true};
};
void validateNativePcSettings(const NativePcSettings& settings);
NativePcSettings loadNativePcSettings(const std::filesystem::path& file);
void saveNativePcSettings(const std::filesystem::path& file,const NativePcSettings& settings);
// Readable PC transport for the original four-byte track-selection input.
// Replay serialization continues to use the source word and ordering.
std::int32_t loadNativeTrackConfiguration(const std::filesystem::path& file);
void saveNativeTrackConfiguration(const std::filesystem::path& file,std::int32_t track);

// Native content/service clock for legacy movie/input counter consumers.
// Its50/59.94 conversion never paces the PC simulation or presentation.
// QPC observations and physical GPU/display completions remain distinct.
class NativeRefreshClock final {
public:
    std::uint64_t Observe(std::uint64_t monotonicQpc,std::uint64_t qpcFrequency,
        std::int32_t videoMode);
    std::uint64_t QpcUntilNextRefresh() const;
    static bool Advanced(std::uint32_t before,std::uint32_t current) noexcept;
private:
    std::uint64_t lastQpc_{},frequency_{},remainder_{};
    bool initialized_{},ntsc_{};
};
// Complete-frame observations use a monotonic host clock and a half-second
// window. Only distinct accepted presents enter the numerator; offscreen
// submissions and failed/out-of-date presents cannot manufacture display FPS.
// Frame ms is elapsed/present count from the same window, not CPU+GPU time.
class NativeFrameTiming final {
public:
    using Clock=std::chrono::steady_clock;
    void Reset(Clock::time_point now) noexcept;
    bool Observe(Clock::time_point now,const EcstasyVulkanFrameStatistics* renderer,
        double updateMilliseconds) noexcept;
    const ApplicationFrameTiming& Read() const noexcept {return published_;}
private:
    Clock::time_point windowStart_{},lastObservation_{};
    std::uint64_t presentedFrames_{},cpuSamples_{},gpuSamples_{},lastSubmission_{},lastGpuSubmission_{};
    double cpuMilliseconds_{},gpuMilliseconds_{};
    bool initialized_{},hasPresentationTarget_{};
    ApplicationFrameTiming published_{};
};
// Native logical-update deadlines are independent of presentation and of
// the content/service VBlank counters. A60-quanta step is exactly1/60s.
// Fractional nanoseconds carry across steps; a delayed host retains its debt.
class NativeSimulationClock final {
public:
    using Clock=std::chrono::steady_clock;
    void Reset(Clock::time_point now) noexcept;
    bool UpdateDue(Clock::time_point now) const noexcept;
    void CompleteStep(std::uint32_t elapsedQuanta=60);
    Clock::time_point Deadline() const noexcept {return deadline_;}
private:
    Clock::time_point deadline_{};
    std::uint32_t remainder_{};
    bool initialized_{};
};
// Physical platform capabilities only. All game state and scheduling stay
// in Main.cpp's one application owner. The device owns the uploaded frame
// data and protects its in-flight resources with Vulkan fences.
class NativeApplicationHost {
public:
    virtual ~NativeApplicationHost()=default;
    virtual bool PollEvents()=0;
    // One monotonic physical clock drives deadlines and measured FPS. Tests
    // may supply controlled observations without publishing GPU completions.
    virtual NativeFrameTiming::Clock::time_point FrameTime() const noexcept {
        return NativeFrameTiming::Clock::now();
    }
    // Changes only when a physical window resumes from minimization. Long
    // active frames remain measured; they are never discarded by a dt cap.
    virtual std::uint64_t PresentationTimingEpoch() const noexcept {return 0;}
    virtual bool PresentationDue(NativeFrameTiming::Clock::time_point) const noexcept {return true;}
    virtual NativeFrameTiming::Clock::time_point PresentationDeadline() const noexcept {
        return NativeFrameTiming::Clock::time_point::max();
    }
    virtual void WaitUntil(NativeFrameTiming::Clock::time_point) {}
    // Optional physical session journal. Simulation-only hosts have no log
    // device; this callback never changes game state or replay decisions.
    virtual void RecordSessionEvent(const char*,const char*) noexcept {}

    // Legacy optional host hook. The native PC application does not call it;
    // logical updates use NativeSimulationClock and output uses the backend.
    virtual bool WaitForSourceRefresh(std::uint32_t frameStart);
    virtual int32_t ReadInput(int32_t port,JoystickControls& destination)=0;
    virtual PortInputEdges TakePortInputEdges() {return {};}
    // Port-side lifecycle hooks. The view is borrowed for this call only.
    // Called before live input and after a successful World update, including catch-up.
    virtual void BeforeWorldStep(const WorldSimulation&) {}
    virtual void AfterWorldStep(const WorldSimulation&) {}
    virtual void SetRumble(int32_t port,uint8_t small,uint8_t large)=0;
    virtual void SubmitFrame(EcstasyFrame& frame)=0;
    // Physical renderer observations only. CPU-only hosts expose no GPU
    // duration; the application must not substitute submit wall time.
    virtual bool ReadRenderStatistics(EcstasyVulkanFrameStatistics&) const noexcept {return false;}
    virtual WorldAudioResult OpenAudio(bSound& sound)=0;
    virtual WorldAudioResult PumpAudio(bSound& sound)=0;
    virtual WorldAudioResult CloseAudio(bSound& sound)=0;
    virtual int Width() const=0;
    virtual int Height() const=0;
    virtual std::size_t AvailableMemory() const=0;
    virtual void ApplyDisplaySettings(int32_t width,int32_t height,bool fullscreen,bool vsync)=0;
};
// Native process lifetime for the individually matched Main.cpp startup
// and frame graph. Construction creates no race or authored resource. A
// missing final file or service remains an exception, never a fallback.
class SkyResources;
class MoviePlayer;
class FlailerResources;
class LanguageManager;
struct FlailerRenderManager;
struct FrontendFrame;
struct NisManagerRegistry;
struct RaceEffectsFrame;
struct RaceParticleAsset;
struct MainMenuScreenServices;
struct FrontendMenuExtension;
// Port-side authored content. Its strings outlive the runtime.
struct FrontendCreditsLine {
    const char* text;
    float scale = 1.0f;
};
struct FrontendCreditsContent {
    const char* label;
    const FrontendCreditsLine* lines;
    std::size_t count;
};
// Port-side fallback for credit lines the authored atlas cannot render.
struct FrontendCreditsText {
    const char* text;
    std::uint32_t color;
    float centerX, top, height;
    float clipLeft, clipTop, clipWidth, clipHeight;
};
enum class FrontendPointerAction { Focus, Confirm, Back, Left, Right, Previous, Next, Item,
    CaretLeft, CaretRight, Help, ReplayPause, ReplayPrevious, ReplayNext, ReplaySlow, ReplayCamera, ReplayHud,
    ReplayPlayer, ReplaySeekPreview, ReplaySeek, ReplaySeekCancel };
struct FrontendPointerTarget {
    std::uint32_t screen{}, object{}, event{};
    FrontendPointerAction action{FrontendPointerAction::Confirm};
    float position{}; // Normalized position for a replay timeline request.
};
// Port-side view of the active source replay controls.
struct ReplayPointerState {
    std::uint32_t screen{};
    bool canSelectPlayer{}, canSeek{}, seeking{};
    float seekPosition{};
};
// Port-side physical keyboard input for the active original name editor.
enum class FrontendNameEditAction { Insert, Left, Right, Backspace, Delete, Commit, Cancel };
struct FrontendStateView {
    // Borrowed CPU layout. Consume synchronously before the next runtime advance.
    const FlailerRenderManager* layout{};
    MoviePlayer* movie{};
    const FrontendFrame* previews{};
    const ::CarCinematicServices* cameras{};
    const char* screen{};
    std::uint32_t selectedItem{};
    std::uint64_t pointerEpoch{};
    std::vector<FrontendPointerTarget> pointerTargets;
    bool pointerModal{}, pointerReplay{}, pointerHelp{}, pointerHelpAvailable{};
    int pointerPopupButtons{};
    const RaceEffectsFrame* effects{};
    std::uint64_t nameEditSession{};
    bool nameEditInput{};
    bool creatingProfile{};
    std::vector<FrontendCreditsText> creditsText;
    ReplayPointerState replayPointer;
};
// Port-side operands from the original primary view and render policy.
struct RacePostState {
    std::array<float, 2> fogRange{};
    std::array<float, 4> fog{}; // RGB bytes and strength
    std::array<float, 4> focus{}; // center, range, min alpha, max alpha
    const EditorMarkerTexture* radialMask{};
    bool radialActive{};
    const EditorMarkerTexture* filterTexture{};
    std::array<std::uint8_t, 2> radialAmounts{};
    std::array<std::uint8_t, 4> filterColor{};
    std::uint8_t filterAlpha{};
    int fogMode{}, focusTransition{}, focusPasses{}, focusJitter{};
    float fogPower{};
    bool fogEnabled{}, focusEnabled{}, monochrome{};
};
struct RaceStateView {
    const ::WorldSimulation* simulation{};
    const ::Scenery* scenery{};
    const ::CarCinematicViewFrame* camera{};
    const ::Car* hiddenCar{};
    const SkyResources* sky{};
    int worldFrames{};
    int track{-1};
    int sessionState{};
    bool ntsc{};
    NisManagerRegistry* nis{};
    bool blackAndWhite{};
    ::CarViewMoverRegistry* viewMovers{};
    const ::CarCinematicViewFrame* rearCamera{};
    const ::Car* rearHiddenCar{};
    std::array<const EditorMarkerTexture*, 2> rearTextures{};
    bool reverse{};
    bool beTheCop{};
    // Port-side borrowed render operands, consumed with the committed view.
    const float* reflectionOrigin{};
    ResourceRegistry* renderResources{};
};
class ApplicationRuntime final {
public:
    ApplicationRuntime(GameResources& files,NativeApplicationHost& host,
        const std::filesystem::path& profileDirectory);
    ~ApplicationRuntime();
    ApplicationRuntime(const ApplicationRuntime&)=delete;
    ApplicationRuntime& operator=(const ApplicationRuntime&)=delete;
    void Initialize(const std::filesystem::path& configFile={});
    bool Tick();
    // The host can request one race once the authored frontend banks are ready.
    void InitializeRace(void (*start)(MainMenuScreenServices&, const void*), const void* context);
    // Port-side frontend lifecycle hooks. The extension outlives the runtime.
    void InitializeFrontend(void (*ready)(MainMenuScreenServices&, const void*) = nullptr,
        const void* context = nullptr, const FrontendMenuExtension* extension = nullptr,
        const FrontendCreditsContent* credits = nullptr, const FrontendMenuExtension* optionsExtension = nullptr,
        bool requireNamedProfile = false, const FrontendMenuExtension* controlsExtension = nullptr);
    FrontendStateView FrontendState() const;
    void BindSubtitleSetting(bool (*setting)(void*,int),void* context);
    const ::SpeechCaptionState& SpeechCaption() const noexcept;
    void QueueFrontendPointer(std::uint64_t epoch, FrontendPointerTarget target);
    void QueueFrontendNameEdit(std::uint64_t epoch, std::uint64_t session, FrontendNameEditAction action,
        std::uint32_t character = 0);
    bool AdvanceRace();
    int SelectedTrack() const noexcept;
    RaceStateView RaceState() const noexcept;
    RacePostState RacePost() const;
    // Port-side borrowed original resource catalogue for frontend previews.
    ResourceRegistry& PresentationResources() const noexcept;
    void TakeRaceEffects(RaceEffectsFrame& frame);
    std::vector<RaceParticleAsset> RaceParticleAssets() const;
    void SetDisplayOptions(int32_t width,int32_t height,bool fullscreen,bool vsync);
    void Shutdown();
    EcstasyFrame& Frame() noexcept;
    bSound& Audio() noexcept;
    const GameTime& Clock() const noexcept;
    // Port-side fraction of the pending logical interval for visual interpolation.
    float PresentationStepFraction() const noexcept;
    const FrontendPlayerManager& Frontend() const;
    const ApplicationFrontendFlow& Flow() const noexcept;
private:
    bool TickInternal();
    std::exception_ptr ShutdownNoThrow() noexcept;
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
#endif
#endif
