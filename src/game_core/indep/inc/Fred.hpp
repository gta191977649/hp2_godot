#ifndef NFSHP2_FRED_HPP
#define NFSHP2_FRED_HPP


#include "Track.hpp"

#ifdef __cplusplus
namespace hp2 { struct FrontendPlayerManager; }
typedef hp2::FrontendPlayerManager RaceFrontendManager;
extern "C" {
#else
typedef struct RaceFrontendManager RaceFrontendManager;
#endif

typedef struct RaceProfileBestTime {
    int32_t ticks;
    int16_t playerNameIndex;
    int16_t modelCode;
} RaceProfileBestTime;

/* Borrowed storage belongs to the loaded user profile. This view neither
 * invents initial best times nor replaces profile persistence. */
typedef struct RaceProfileTimes {
    const TrackScenarioCatalog* catalog;
    RaceProfileBestTime* bestTimes;
    size_t bestTimeCount;
    const RaceFrontendManager* frontend;
} RaceProfileTimes;

// Final PAL1878B0 checksum owner. The Windows profile stream below is a
// versioned semantic projection; it does not claim PS2 raw-layout compatibility.
#define GAME_SAVE_SIZE 0x114e0u
#define GAME_SAVE_CHECKSUM_OFFSET (GAME_SAVE_SIZE-4u)
uint32_t saveGameCalculateChecksum(const uint8_t* data);
int saveGameIsValid(const uint8_t* data,size_t bytes);
int saveGameFinalize(uint8_t* data,size_t bytes);

int32_t raceProfileCourseIndex(const RaceProfileTimes* profile, int32_t scenario);
int32_t raceProfileTimeIndex(const RaceProfileTimes* profile, int32_t scenario,
                             int32_t vehicleClass, int32_t reverse);
RaceProfileBestTime* raceProfileFindBestTime(const RaceProfileTimes* profile, int32_t scenario,
                                            int32_t vehicleClass, int32_t reverse);
/* Returns -1 for a missing canonical frontend binding, otherwise the actual
 * FRED UseTheRaceEvent classifier (0 or1). This adapter adds no ELF body. */
int32_t raceProfileUsesResults(const RaceProfileTimes* profile);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include "CarInfo.hpp"
#include "CarDriver.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "Track.hpp"
#include "indep/inc/EventTree.hpp"
#include "CameraMover.hpp"
#include "indep/inc/Point.hpp"
#include <array>
#include <exception>
#include <cstdint>
#include <string>

struct RaceProfileBestTime;
struct CarFramePlayerRegistry;
struct RaceFrame;
struct RaceCarEntry;
struct PursuitManager;
struct CarSoundBindings;

namespace hp2 {
class LanguageManager;
class EventTree;
class EventTreeResources;
struct FrontendRaceSetupBindings;
struct FlailerSoundState;
class HighScoreTable;
class DemoModeReplay;
struct ApplicationFrontendFlow;
using FrontendRaceRuntimeSettings=::RaceRuntimeSettings;

struct FrontendCarAvailability {
    std::int32_t status{};
    std::array<std::uint8_t,10> skinNumbers{};
    std::int32_t skinCount{};
    std::int32_t allowDashboardTextures{};
};

struct FrontendRideInfo {
    CarAppearanceSelection appearance{};
};

// The campaign screen's live selection is separate from the current race
// event number. Tree0 is Championship and tree1 is Hot Pursuit.
struct DataEventSelection {
    std::int32_t treeType{};
    std::int32_t selectedHotPursuitEvent{1};
    std::int32_t selectedChampionshipEvent{1};
    // Actual DataEventSelectScreen+0C; Cops mode entry resets the existing
    // campaign view, independently of the two persisted selected events.
    std::int32_t lastViewState{};
    std::int32_t GetSelectedEvent(std::int32_t type) const;
    void SetSelectedEvent(std::int32_t ignoredType,std::int32_t event);
};

/* ONE music-settings portion of the existing FRED audio-options member.
 * PAL producers1EEF60/1EF240 own these signed row lanes/count/mask. This
 * native arrangement is not the beta/console DataAudioOptionsScreen layout;
 * live mixer values and saved SoundVolumeSettings remain their own owners. */
struct FrontendMusicSettings {
    struct Row {
        std::int8_t group{};
        std::int8_t track{};
        std::int8_t status{};
        std::int8_t genreBit{};
    };
    std::array<Row,32> rows{};
    std::int32_t count{};
    std::uint8_t enabledGenres{};
    // Actual saved audio options; live mixer categories remain bSound owned.
    std::array<float,7> savedVolumes{};
    std::int32_t savedAudioMode{};
    // The original DataAudioOptionsScreen navigation cells A0/A4/A8/AE.
    // These are separate from the live mixer and its seven category values.
    std::array<std::int32_t,3> menuState{};
    std::uint8_t menuMode{};
};

// Profile availability and selected rides are mutable frontend data. They
// borrow the canonical car catalogue and are separate from live race setup.
struct FrontendPlayerInfo {
    const CarInfoResource* cars{};
    const LanguageManager* language{};
    std::array<FrontendCarAvailability,64> availability{};
    std::array<FrontendRideInfo,11> selectedRides{};
    PlayerSettings settings{};
    // Final FrontendPlayerInfo+6CC, accumulated profile points.
    std::int32_t totalPoints{};

    void SetCarAvailability(std::int32_t status,std::int32_t model,std::int32_t skin,bool allowDashboard);
    void FlushAvailableCars();
    std::int32_t GetCarStatus(std::int32_t model) const;
    std::int32_t GetSelectedCarClass(std::int32_t gameType) const;
    void BuildRideInfo(FrontendRideInfo& destination,std::int32_t model,std::uint32_t skin,bool allowDashboard) const;
    void AssignCarOfType(std::int32_t gameType,std::int32_t model,std::uint32_t skin,bool allowDashboard);
    void SetDefaultCarType(std::int32_t model,std::uint32_t skin,bool allowDashboard);
    const FrontendRideInfo* GetSelectedRideInfo(std::int32_t gameType) const;
};

struct DataSelectTrackScreen {
    std::int32_t baseTrackNumber{};
    std::int32_t lapCount{};
    std::int32_t direction{};
    std::int32_t timeOfDay{};
    void InitWithDefaults(const TrackInfoResources& tracks);
    std::int32_t CalculateTrackNumberToLoad(const TrackInfoResources& tracks) const;
};

// Final transient race statistics survive retirement of World and Players.
// These are the original FRED+38C result owner, not a screen-side mirror.
struct TransientStatsElement {
    std::int32_t position{},driverNumber{},completionReason{},totalTicks{},bestLapTicks{};
    std::int32_t racePoints{},profilePoints{},bonusPoints{},tickets{},transmission{-1};
    float maximumSpeed{},averageSpeed{},pursuitSeconds{};
    std::array<std::int32_t,11> lapTicks{};
    std::int32_t laps{};
    void Construct(const ::RaceCarEntry& entry,const ::RaceFrame& race,
        const FrontendPlayerManager& frontend,const RaceParameters& parameters);
};
struct TransientStats {
    std::int32_t count{},pointsReward{-1},taskReward{-1};
    std::array<TransientStatsElement,8> rows{};
    void Construct(const ::RaceFrame& race,const FrontendPlayerManager& frontend,
        const RaceParameters& parameters);
    const TransientStatsElement* GetDriver(std::int32_t driver) const;
};

// Native PC presentation preferences are saved by the same profile owner.
// These fields adapt a new platform and contribute no original ELF bodies.
struct NativeDisplaySettings {
    std::int32_t width{1280},height{720};
    bool fullscreen{},vsync{true};
};
// The original RaceSetup profile lanes; damage remains in the existing
// savedChallengeOptions bank, one canonical cell for each game type3..7.
// Arrays retain source field order, not console addresses or packed layout.
struct FrontendChallengeSetup {
    std::int32_t playerCount{1},gameType{3},raceOption254{1},opponentSkill{1},copSkill{};
};
struct FrontendRaceSetupOptions {
    std::array<std::int32_t,6> single{{2,2,0,1,3,0}};
    std::array<std::int32_t,5> knockout{{7,0,1,3,0}};
    std::array<std::int32_t,5> lapKnockout{{4,0,1,3,0}};
    std::array<std::int32_t,2> freeRun{{3,0}};
    std::array<std::int32_t,4> beTheCop{{2,0,1,0}};
};
struct FrontendPlayerManager {
    // Final HandleLanguageChange21E210 writes FRED+174 (2FC1C4).
    // This is the live profile cell, independent of manager currentLanguage.
    NativeDisplaySettings nativeDisplay{};
    std::int32_t currentLanguage{};
    // Actual saved FRED+04, tested by EndGameScreens.ShouldAutoSave.
    std::int32_t autoSave{};
    std::int32_t gameType{};
    std::int32_t playerCount{};
    std::array<FrontendPlayerInfo*,2> players{};
    std::int32_t selectedEventNumber{};
    HighScoreTable* highScores{};
    DataEventSelection eventSelection{};
    DataSelectTrackScreen trackSelection{};
    EventTreeProgress eventProgress;
    // Actual FRED+144/+148 calibration values. DisplayOptionsChangeManager
    // snapshots these two cells; Windows presentation remains a borrower.
    std::array<std::int32_t,2> displayCalibration{};
    GlobalDisplaySettings displaySettings{};
    std::int32_t knockoutTrackPolicy{};
    std::int32_t pendingState{};
    const EventTreeResources* boundEventResources{};
    // Same profile cells consumed by the record display: source FRED+1C and
    // FRED+100B8,128 retained names of12 bytes. These are not UI copies.
    std::int32_t recordDisplaySuppressed{};
    std::array<std::array<char,12>,128> recordPlayerNames{};
    FrontendMusicSettings musicSettings{};
    // Final FRED+20 and FRED+106B8/+106E0. These are the profile's actual
    // cops-on selection and earned-reward banks, not derived UI caches.
    std::int32_t copsEnabled{};
    std::array<std::int32_t,10> taskRewardsEarned{};
    std::array<std::int32_t,18> pointsRewardsEarned{};
    // Final FRED+10728, written by CheckForAbortion before finishing the
    // remaining racers. This belongs to the profile, not a results screen.
    std::int32_t raceAborted{};
    TransientStats transientStats{};
    // Source challenge option +40 is saved independently for five modes.
    std::array<std::int32_t,5> savedChallengeOptions{{1,1,1,1,1}};
    FrontendChallengeSetup challengeSetup{};
    FrontendRaceSetupOptions challengeModeSetup{};
    // Original FRED+178/+17C. Independent of per-player inputProfile.
    std::array<std::int32_t,2> joystickConfigurations{{0,0}};
    void SetupRaceParametersAfterRaceSetup(const FrontendRaceSetupBindings& bindings);
    void InitWithDefaults(const FrontendRaceSetupBindings& bindings);
    std::array<std::uint8_t,GAME_SAVE_SIZE> SerializeNativeProfile() const;
    bool DeserializeNativeProfile(const void* data,std::size_t bytes,std::string& error);
    // PC persistence stores named UTF-8 JSON. Legacy NFRED57 images are
    // read-only migration input; these adapters add no accepted ELF bodies.
    std::string SerializeNativeProfileText() const;
    bool ValidateNativeProfileText(const void* data,std::size_t bytes,std::string& error) const;
    bool DeserializeNativeProfileText(const void* data,std::size_t bytes,std::string& error);
    bool ConvertLegacyProfileToText(const void* data,std::size_t bytes,std::string& text,std::string& error) const;
    void CaptureAudioOptions(const ::CarSoundBindings& sound,bool rebuildMusicList);
    void ApplyAudioOptions(const ::CarSoundBindings& sound,const RaceParameters& parameters,
        FlailerSoundState& uiSound) const;
    std::int32_t InsertPlayerName(const char* name);
    void GetOptionsSetFromRace(const ::CarFramePlayerRegistry& registry,
        RaceParameters& parameters,const ::CarSoundBindings& sound);
    std::int32_t FigureOutBonusPoints(const ::RaceFrame& race,const ::RaceCarEntry& entry,
        const RaceParameters& parameters) const;
    void AwardPointsToPlayer(const ::RaceFrame& race,const ::RaceCarEntry& entry,
        const RaceParameters& parameters);
    void TheRaceIsOver(::RaceFrame& race,const FrontendRaceSetupBindings& setup,
        const ::CarSoundBindings& sound);
    void TheRaceWasRestarted(const FrontendRaceSetupBindings& setup);
    void TheRaceIsLeaving(const FrontendRaceSetupBindings& setup);
    void TheRaceWasQuit(::RaceFrame& race,const FrontendRaceSetupBindings& setup,
        const ::CarSoundBindings& sound);
    void CheckForAbortion(const CarFramePlayerRegistry& players);
    bool UseTheRaceEvent() const;
    bool IsChallengeGame() const;
    bool IsEventGame() const;
    EventTree* GetEventTree(const EventTreeResources& trees,const std::int32_t& demoDisc) const;
    void SetupCarsForRace(const FrontendRaceSetupBindings& bindings);
    void CalculateStartingPositions(const FrontendRaceSetupBindings& bindings);
    RaceProfileBestTime* GetBestTotalTimesTable(std::int32_t scenario,std::int32_t vehicleClass,
        std::int32_t reverse,std::int32_t laps) const;
    void SetRecordTimesToBeat(std::int32_t seat,const FrontendRaceSetupBindings& bindings);
    void MaybeSetSelectedCarForMultiTrackEvent(const FrontendRaceSetupBindings& bindings);
    void BindEventProgress(const EventTreeResources& trees);
    void SetupFakeRaceEventForChallengeKnockout(const FrontendRaceSetupBindings& bindings);
    void PickRandomTracksForChallengeKnockout(const FrontendRaceSetupBindings& bindings);
    void StartRace(const FrontendRaceSetupBindings& bindings);
    void StartDemoMode(std::int32_t track,const FrontendRaceSetupBindings& bindings);
    void StartDemoDiscGame(const FrontendRaceSetupBindings& bindings);
    void MakeEventCarsAvailable(const EventTree& tree,const EventInfo& event);
    void MakeChallengeCarsAvailable(const CarInfoResource& cars,
        const EventTreeResources& events,const std::int32_t& demoDisc);
    const char* RecordPlayerName(std::int32_t index) const;
};

bool AreThereAnyCarsOfType(const RaceParameters& race,std::int32_t model);
bool AreThereAnyCarsOfColour(const RaceParameters& race,const CarInfoResource& cars,
    std::int32_t model,std::int32_t skin);
std::int32_t PickRandomSkinOfUniqueColour(const RaceParameters& race,const CarInfoResource& cars,
    const LanguageManager& language,std::int32_t model);

} // namespace hp2

#include "RaceParameters.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <cstddef>

static_assert(sizeof(RaceProfileBestTime)==8 && offsetof(RaceProfileBestTime,ticks)==0 &&
    offsetof(RaceProfileBestTime,playerNameIndex)==4 && offsetof(RaceProfileBestTime,modelCode)==6,
    "Canonical timer/name/model record must retain the original eight logical bytes");

namespace hp2 {
class TrackInfoResources;
struct FrontendPlayerManager;

// One retained high-score owner. Total-race and lap records are separate
// source banks; the existing RaceProfileTimes view borrows this lap bank.
// TrackInfo is borrowed from the same mutable GLOBALB resource as the UI.
class HighScoreTable final {
public:
    static constexpr std::size_t TotalRecordCount=7200;
    static constexpr std::size_t LapRecordCount=800;
    HighScoreTable(TrackInfoResources& tracks,const FrontendPlayerManager& frontend);
    HighScoreTable(const HighScoreTable&)=delete;
    HighScoreTable& operator=(const HighScoreTable&)=delete;
    void InitStatsTables();
    std::int32_t MaybeRecordHighScore(FrontendPlayerManager& frontend,const ::RaceFrame& race,
        ::RaceCarEntry& entry,const RaceParameters& parameters);
    std::int32_t GetUniqueLapIndex(std::int32_t laps) const;
    std::int32_t GetTotalTimesIndex(std::int32_t scenario,std::int32_t vehicleClass,
        std::int32_t reverse,std::int32_t laps) const;
    RaceProfileTimes& LapView() noexcept { return lapView_; }
    const RaceProfileTimes& LapView() const noexcept { return lapView_; }
    std::array<RaceProfileBestTime,TotalRecordCount>& TotalRecords() noexcept { return total_; }
    const std::array<RaceProfileBestTime,TotalRecordCount>& TotalRecords() const noexcept { return total_; }
    std::array<RaceProfileBestTime,LapRecordCount>& LapRecords() noexcept { return lap_; }
private:
    TrackInfoResources& tracks_;
    std::array<RaceProfileBestTime,TotalRecordCount> total_{};
    std::array<RaceProfileBestTime,LapRecordCount> lap_{};
    RaceProfileTimes lapView_{};
};

// PAL source copies these nine live bytes before looking up the lap count.
// Their initial authored choices are not treated as an immutable ELF object.
extern std::array<std::int8_t,9> highScoreLapCounts;

using FrontendRaceRuntimeSettings=::RaceRuntimeSettings;
} // namespace hp2


#include "RaceParameters.hpp"
#include "CarInfo.hpp"
#include "CarDriver.hpp"
#include "indep/inc/Ecstasy.hpp"
#include "Track.hpp"
#include "indep/inc/CarDriver.hpp"
#include <cstdint>
#include <string>

struct DriverBehavior;
struct ApplicationLaunchOptions;
namespace hp2 { class CarInfoResource; }
namespace hp2 {
class LanguageManager;
class TrackInfoResources;
struct RaceEvent;
class EventTreeResources;
using FrontendRaceRuntimeSettings=::RaceRuntimeSettings;

// These pointers borrow the actual application/resource/setup owners. There
// is no second player selection, race configuration, model or behavior bank.
struct FrontendRaceSetupBindings {
    RaceParameters* parameters{};
    const CarInfoResource* cars{};
    const LanguageManager* language{};
    const TrackInfoResources* tracks{};
    const CarDriverAppearanceCatalogue* characters{};
    const DriverBehavior* behavior{};
    const ApplicationLaunchOptions* applicationOptions{};
    RaceEvent* currentEvent{};
    const EventTreeResources* events{};
    // Borrowed actual application content-policy cell (PAL GP-5AB8).
    const std::int32_t* demoDisc{};
    // Must be the same record as parameters->runtimeSettings at StartRace.
    FrontendRaceRuntimeSettings* runtimeSettings{};
    DemoModeReplay* demoReplay{};
    ApplicationFrontendFlow* flow{};
};

std::int32_t PickCharacterNumberThatIsntRacing(std::int32_t* numbers,
    std::int32_t count,std::int32_t preferredAppearanceGroup,
    const CarDriverAppearanceCatalogue& characters);
std::int32_t PickCopCharacterNumber(const RaceParameters& parameters,
    const TrackInfoResources& tracks,const CarDriverAppearanceCatalogue& characters);

RaceDriverInfo* InitializeDriverInfo(RaceDriverInfo& driver,std::int32_t number,
    std::int32_t driverClass,const CarAppearanceSelection& ride,
    std::int32_t characterIndex,std::int32_t behaviorProfile,std::int32_t startingPosition,
    const CarDriverAppearanceCatalogue& characters);
void AddDriverInfo(RaceParameters& parameters,const RaceDriverInfo& driver,
    const CarInfoResource& cars);
bool AreThereTooManyUniqueCarGeometries(const RaceParameters& parameters,
    const CarAppearanceSelection& candidate,const CarInfoResource& cars);

struct FrontendRandomlyPickableTrack {
    const TrackScenarioDefinition* info{};
    std::int32_t direction{};
};
void RandomizeList(std::vector<FrontendRandomlyPickableTrack>& tracks);
bool IsTrackUnlocked(const EventTreeResources& resources,std::int32_t track,std::int32_t direction);
void CheckForTaskAndPointRewards(FrontendPlayerManager& frontend,const ::RaceFrame& race,
    const RaceParameters& parameters);
std::int32_t GetCarForPointsReward(std::int32_t index);
std::int32_t GetCarForTaskReward(std::int32_t taskId);
std::int32_t GetLanguageIDForTaskReward(std::int32_t taskId);
std::int32_t GetGameTypeForTaskReward(std::int32_t taskId);
std::int32_t GetNumPointsRewardsEarned(const FrontendPlayerManager& frontend);
std::int32_t GetNumCarsUnlocked(const FrontendPlayerManager& frontend,
    const CarInfoResource& cars,const EventTreeResources* events,std::int32_t usage);
std::int32_t GetNumCarsAvailable(const CarInfoResource& cars,std::int32_t usage);
std::int32_t GetNumCopCarsUnlocked(const FrontendPlayerManager& frontend,
    const CarInfoResource& cars,const EventTreeResources* events);
std::int32_t GetNumCopCarsAvailable(const CarInfoResource& cars);
std::string FormatRewardPoints(std::int32_t points,const LanguageManager& language);
bool IsCarUnlocked(const FrontendPlayerManager& frontend,const EventTreeResources* events,
    std::int32_t model,std::int32_t gameType);
// One mutable final GP-5A48 cell, initially7; not a race/profile duplicate.
extern std::int32_t frontendMaximumOpponents;

} // namespace hp2


namespace hp2 {
class TexturePack;
class DriverBehaviorData final {
public:
    explicit DriverBehaviorData(const std::vector<std::uint8_t>& globalBundle);
    DriverBehaviorData(const DriverBehaviorData&)=delete;
    DriverBehaviorData& operator=(const DriverBehaviorData&)=delete;
    DriverBehaviorData(DriverBehaviorData&&)=delete;
    DriverBehaviorData& operator=(DriverBehaviorData&&)=delete;
    const DriverBehavior& Settings() const noexcept {return settings_;}
private:
    DriverBehavior settings_{};
};
}


// Native frontend selection declarations share the actual FRED owner header.
// This arrangement does not assert a beta object layout or original lexical
// placement of every type; method definitions retain their proven CPP files.
namespace hp2 {
struct FlailerScreenList;
class FlailerResources;
struct FlailerUiState;
class FlailerHighlightAnimation;
class FlailerAnimationGroup;
class FlailerAlphaAnimation;
struct FlailerTextChunk;

// The catalogue is decoded from final GLOBALB chunk 34201. Record numbers
// include authored holes. This owner is separate from the loaded track world.
using TrackInfo=TrackScenarioDefinition;

class TrackInfoResources final {
public:
    explicit TrackInfoResources(std::vector<std::uint8_t> authored);
    explicit TrackInfoResources(TrackScenarioData& owner) noexcept;
    const TrackInfo* Find(std::int32_t recordNumber) const noexcept;
    const std::vector<TrackInfo>& Records() const noexcept { return data_->Records(); }
    std::vector<TrackInfo>& MutableRecords() noexcept { return data_->MutableRecords(); }
    const TrackScenarioCatalog& Catalog() const noexcept { return data_->Catalog(); }
    const std::vector<std::uint8_t>& Authored() const noexcept { return data_->Authored(); }
private:
    std::unique_ptr<TrackScenarioData> owned_;
    TrackScenarioData* data_{};
};

struct TimeOfDayTrack {
    TimeOfDayTrack* next{};
    TimeOfDayTrack* previous{};
    const TrackInfo* info{};
    float thumbnailOffset{};
    TimeOfDayTrack(float offset,const TrackInfo* track);
};

struct SelectableTrack {
    SelectableTrack* next{};
    SelectableTrack* previous{};
    std::int32_t direction{};
    float horizontalThumbnail{};
    TimeOfDayTrack* currentTime{};
    TimeOfDayTrack timeRoot{0,nullptr};
    const TrackInfo* baseInfo{};
    EditorMarkerModel model{};
    std::int32_t availability{};
    std::uint32_t thumbnailKey{};
    float timeMenuHeight{};
    std::vector<std::unique_ptr<TimeOfDayTrack>> ownedTimes;

    SelectableTrack()=default;
    SelectableTrack(const TrackInfoResources& tracks,EditorMarkerResources& models,
        const TrackInfo& base,std::int32_t reverse,std::int32_t status,
        std::uint32_t thumbnail,float offset);
    ~SelectableTrack();
    SelectableTrack(const SelectableTrack&)=delete;
    SelectableTrack& operator=(const SelectableTrack&)=delete;
    TimeOfDayTrack* GetTimeOfDay(std::int32_t value);
    bool SetTimeOfDay(std::int32_t value);
    bool ScrollTimeOfDay(std::int32_t direction);
    void DrawTimeOfDay(FlailerResources& resources,std::uint32_t screen,float offset);
    std::uint32_t PickTrackTextureForSlideshow(::hp2::TexturePack& pack) const;
};

struct SelectableTrackManager {
    SelectableTrack* first{};
    SelectableTrack* last{};
    SelectableTrack* current{};
    float thumbnailSpacing{};
    // Same selectable nodes as the linked list. Only their lifetime is held
    // here; current selection and availability are never copied into a cache.
    std::vector<std::unique_ptr<SelectableTrack>> ownedTracks;

    std::int32_t GetCurrentBaseTrackNumber() const;
    std::int32_t GetCurrentTimeOfDay() const;
    std::int32_t GetCurrentDirection() const;
    SelectableTrack* GetNextTrack() const;
    SelectableTrack* GetPrevTrack() const;
    float GetDesiredHorizThumbnailOffset() const;
    bool AtLeftEdge() const;
    bool AtRightEdge() const;
    SelectableTrack* FindTrack(const TrackInfoResources& resources,std::int32_t number,std::int32_t direction) const;
    void SetCurrent(const TrackInfoResources& resources,std::int32_t number,std::int32_t direction,std::int32_t timeOfDay);
    bool ScrollSelectedTrack(std::int32_t direction);
    bool ScrollTimeOfDay(std::int32_t direction);
    void DrawThumbnails(FlailerResources& resources,std::uint32_t screen,float horizontal,float vertical);
    SelectableTrack& AddTrack(const TrackInfoResources& tracks,EditorMarkerResources& models,
        const TrackInfo& base,std::int32_t direction,std::int32_t status,
        std::uint32_t thumbnail,float offset);
    std::int32_t GetTrackStatus(const FrontendPlayerManager& frontend,const TrackInfoResources& tracks,
        const EventTreeResources& events,std::int32_t track,std::int32_t direction) const;
    void BuildList(const FrontendPlayerManager& frontend,const TrackInfoResources& tracks,
        const EventTreeResources& events,FlailerResources& resources,std::uint32_t screen,float spacing);
};

struct TrackTimeOfDayMenu {
    FlailerUiState* ui{};
    FlailerScreenList* screens{};
    std::uint32_t screenKey{};
    SelectableTrack* current{};
    TrackTimeOfDayMenu(FlailerUiState& state,FlailerScreenList* list,std::uint32_t screen);
    std::int32_t SetSelectableTrack(SelectableTrack* track);
};

class TrackRecordDisplayer final {
public:
    TrackRecordDisplayer(FlailerUiState& ui,FrontendPlayerManager& frontend,std::uint32_t screen);
    void SetCurrentTrack(SelectableTrack* track);
    void Update();
    // Native factoring of source19F320, not an additional claimed original.
    void ShowCurrentRecord();
    struct Record {std::int32_t laps;RaceProfileBestTime* time;};
    FlailerUiState* ui{};
    FrontendPlayerManager* frontend{};
    std::uint32_t screenKey{};
    SelectableTrack* previousTrack{};
    std::int32_t startedTicks{};
    std::vector<Record> records;
    std::size_t currentRecord{};
    bool advanceArmed{true};
};

class SelectTrackSlideshow final {
public:
    SelectTrackSlideshow(FlailerUiState& ui,const GameResources& resources,std::uint32_t screen);
    ~SelectTrackSlideshow();
    std::exception_ptr Shutdown() noexcept;
    void SetCurrentTrack(SelectableTrack* track);
    void TextureLoadedCallback();
    void MaybeTriggerLoad();
    void Update();
    FlailerUiState* ui{};
    std::uint32_t screenKey{};
    std::unique_ptr<::hp2::TexturePack> texturePack;
    SelectableTrack* currentTrack{};
    std::int32_t startedTicks{};
    std::int32_t transitionTicks{};
    std::uint32_t pendingTextureKey{};
    std::uint32_t loadCompleted{};
    std::int32_t transitionState{};
    std::unique_ptr<FlailerAlphaAnimation> currentAlpha;
    std::unique_ptr<FlailerAlphaAnimation> nextAlpha;
private:
    std::exception_ptr shutdownFailure_;
    bool shutdownStarted_{};
};

struct TrackPresentationSettings {
    float lockedSlideDuration{0.5f};
    float lockedDelay{0.4000000059604645f};
    float pulseSpeed{9.0f};
    float modelScale{1.0f};
    float modelAngle{};
    float modelAngleStep{0.009999999776482582f};
    // The final mutable string/table cells330B68..330BA0 and2B54D0/F0.
    // They are CPU selection data, not a claim that writable data is immutable.
    std::array<std::string,2> directionPrefixes{{"","R_"}};
    std::array<std::string,3> pictureVariants{{"A","B","C"}};
    std::array<std::string,4> pictureTimeSuffixes{{"","_OVERCAST","_SUNSET","_NIGHT"}};
    // Final static producer1A09E8 fills these six cells. DoTick publishes
    // them into the same still camera; they are not per-screen snapshots.
    std::array<float,4> cameraEye{{25.1200008392334f,-2.563999891281128f,16.899999618530273f,0.0f}};
    std::array<float,4> cameraTarget{{2.559999942779541f,-0.5120000243186951f,0.5120000243186951f,0.0f}};
    std::array<std::uint32_t,3> difficultyLanguage{{0xd6dcu,0xd6ddu,0xd6deu}};
    std::array<std::uint32_t,2> timeMenuObjects{{0xedf2e4eeu,0x20d73d01u}};
    std::string reverseLogoSuffix{"R"};
    std::string frenchLogoSuffix{"_FR"};
    // Static config points to these eight final32A578 cells. Help::Summon
    // reads eight entries; a non-null all-zero list differs from null.
    std::array<std::int32_t,8> helpItems{};
};
// One mutable native projection of final330BB8/BC and330BF4..330C00.
extern TrackPresentationSettings trackPresentationSettings;

class TrackLockedDisplayer final {
public:
    TrackLockedDisplayer(FlailerUiState& ui,const EventTreeResources& events,std::uint32_t screen);
    ~TrackLockedDisplayer();
    void SetCurrentTrack(SelectableTrack* track);
    void Update();
    FlailerUiState* ui{};
    const EventTreeResources* events{};
    std::uint32_t screenKey{};
    SelectableTrack* currentTrack{};
    float anchorX{};
    std::int32_t initialAlpha{};
    std::unique_ptr<FlailerAnimationGroup> animation;
    std::array<char,256> message{};
    std::int32_t startedTicks{};
};

void FormatUnlockBlurbForCar(char* output,std::size_t capacity,std::int32_t model,
    std::int32_t gameType,std::int32_t secondPlayer,const EventTreeResources& events,
    const LanguageManager& language);
void GetTrackUnlockInfo(char* output,std::size_t capacity,std::int32_t track,std::int32_t direction,
    const EventTreeResources& events,const LanguageManager& language);

// Native retained storage for the same selectable tracks and TOD rings.
// Availability is supplied as an explicit record-number list by the live
// frontend/save owner; resource presence alone never unlocks a track.
class TrackSelectionStorage final {
public:
    TrackSelectionStorage(const TrackInfoResources& resources,
        const std::vector<std::int32_t>& visibleBaseTracks,float thumbnailSpacing);
    ~TrackSelectionStorage();
    void BindModels(EditorMarkerResources& models);
    SelectableTrackManager& Manager() noexcept { return manager_; }
private:
    SelectableTrackManager manager_{};
};

struct VisibleCar {
    VisibleCar* next{};
    VisibleCar* previous{};
    const CarDefinition* definition{};
    // Borrowed permitted choices. The frontend/save policy owns this view;
    // no skin count is copied out of the canonical choices owner.
    const FrontendCarAvailability* availability{};
    std::int32_t skinIndex{};
    bool selectionFlag{};
    std::uint32_t thumbnailKey{};
    std::int32_t selectionOrder{};
    std::int32_t NextSkin();
    std::int32_t PrevSkin();
};

struct VisibleCarSet {
    VisibleCar root{};
    VisibleCar* current{};
    VisibleCar* GetNextCar(VisibleCar* selected) const;
    VisibleCar* GetPrevCar(VisibleCar* selected) const;
};

struct MainMenuScreenServices;
struct GameFlowFrontendResources;
struct FrontendFrame;
struct ScreenCallbackData;
enum class ScreenMessage : std::int32_t;
struct FlailerScreenConfig;
struct SelectCarScreenServices {
    MainMenuScreenServices* menus{};
    ::CarCinematicServices* cameras{};
    ResourceRegistry* resources{};
    const CarTextureNamingPolicy* naming{};
    const PositionMarkerRegistry* markers{};
    GameFlowFrontendResources* frontendResources{};
    FrontendFrame* frame{};
    AcidFXRuntime* effects{};
};

struct CarSelector {
    CarSelector(SelectCarScreenServices& services,FrontendPlayerInfo& profile,
        std::int32_t port,bool split,std::int32_t view,std::uint32_t screen);
    ~CarSelector();
    CarSelector(const CarSelector&)=delete;
    CarSelector& operator=(const CarSelector&)=delete;
    static void InitSpotlights(ResourceRegistry& resources);
    static void CloseSpotlights();
    void Tick();
    void HandleMessage(ScreenMessage message,ScreenCallbackData& data);
    void JoyHandler(std::int32_t port,std::int32_t event,std::uint8_t value);
    static void JoyHandlerBridge(std::int32_t port,std::int32_t event,std::uint8_t value,void* owner);
    void EnterOrbitMode();
    void EnterChooseMode();
    bool CanEnterOrbitMode(const VisibleCar* car) const;
    void AdvanceState();
    void DevanceState();
    void SummonPopup();
    void DismissPopup();
    void SetupSwatches();
    void UpdateSwatches();
    void SetupThumbnails();
    void UpdateThumbnails();
    void UpdateInfoBarAtTop();
    void SetLogo();
    void UpdateFadeGroups();
    void UpdateNebula();
    void ShowLoadedCar();
    void SetupCarOverrideModel();
    void SetCameraParameters(std::int32_t model);
    void AdjustHelpMessage();
    VisibleCarSet visibleCars{};
    bool splitScreen{};
    std::int32_t selectionState{};
    std::int32_t firstChoice{};
    std::int32_t secondChoice{};
    std::int32_t popupIndex{-1};
    FlailerScreenList* screens{};
private:
    friend struct SelectCarScreen;
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

struct SelectCarScreen {
    explicit SelectCarScreen(SelectCarScreenServices& services);
    ~SelectCarScreen();
    SelectCarScreen(const SelectCarScreen&)=delete;
    SelectCarScreen& operator=(const SelectCarScreen&)=delete;
    FrontendPlayerManager* frontend{};
    std::int32_t selectorCount{};
    std::array<CarSelector*,2> selectors{};
    void SetSelectedCars();
    void DismissAllPopups();
    bool AreAllSelectorsReady() const;
    void ActivateCarSelector(ScreenCallbackData& data);
    void DeactivateCarSelector(ScreenCallbackData& data);
    void HandleMessage(ScreenMessage message,ScreenCallbackData& data);
    bool MaybeContinueToNextScreen();
    static void Callback(ScreenMessage message,ScreenCallbackData* data);
private:
    SelectCarScreenServices* services_{};
    std::array<std::unique_ptr<CarSelector>,2> ownedSelectors_;
};
// Source CampaignTrees.obj/RewardView.obj owners. Service pointers borrow
// the same application objects as Select Car; no new catalogue/profile exists.
class tTrackModelViewer final {
public:
    tTrackModelViewer(SelectCarScreenServices& services,const EventReward& reward);
    ~tTrackModelViewer();
    tTrackModelViewer(const tTrackModelViewer&)=delete;
    tTrackModelViewer& operator=(const tTrackModelViewer&)=delete;
    void RenderTrack();
    void TickTrack(FrontendStillCamera& camera);
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
class tCarModelViewer final {
public:
    tCarModelViewer(SelectCarScreenServices& services,const EventReward& reward);
    ~tCarModelViewer();
    tCarModelViewer(const tCarModelViewer&)=delete;
    tCarModelViewer& operator=(const tCarModelViewer&)=delete;
    void BeginLoading();
    void Unload();
    void Tick(FrontendStillCamera& camera);
    void MakeVisible();
    void MakeInvisible();
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
void BindCampaignScreen(SelectCarScreenServices& services);
void UnbindCampaignScreen();
FlailerScreenConfig& CampaignScreenConfiguration();
std::uint32_t FigureOutTrackNameImageHash(std::int32_t track,std::int32_t direction,
    const LanguageManager& language);
// PC-only Select Track presentation precision. Source PAL still owns raw
// binary32 length/conversion semantics; this controls visible decimal text.
std::string FormatTrackDistanceForDisplay(float miles,bool metric,const char* route);

void BindSelectCarScreen(SelectCarScreenServices& services);
void UnbindSelectCarScreen();
bool IsCarSelectScreenActive() noexcept;
std::array<FlailerScreenConfig,2>& SelectCarScreenConfigurations();

enum class ScreenMessage : std::int32_t;
struct ScreenCallbackData;
struct FlailerScreenConfig;
class FrontendStillCamera;
struct FlailerStrip;
struct FlailerRenderManager;

// The frontend owns one frame, consumed before its screen is destroyed.
// Text requests borrow the actual clipped cache retained by the same Flailer
// manager. Model culling/lighting is still a separate CPU dependency.
struct FrontendModelRequest {
    const EditorMarkerModel* model{};
    std::array<float,16> transform{};
    std::int32_t viewNumber{-1}; // borrowed source view, never the Flailer view0 by inference
};
struct FrontendEventEntry {
    int number{}, status{};
    std::string title;
    std::array<float,2> position{};
};
struct FrontendFrame {
    std::vector<FrontendModelRequest> models;
    std::vector<FrontendEventEntry> events;
    std::string eventDescription;
    int selectedEvent{};
    bool eventDetails{};
    // Explicit outstanding CPU renderer obligations, not a ready flag.
    bool modelDrawCpuClosureRequired{};
    FlailerRenderManager* textRenderer{};
    void Clear() noexcept;
};

struct SelectTrackScreenServices {
    FlailerUiState* ui{};
    FlailerScreenList* screens{};
    FrontendPlayerManager* frontend{};
    const TrackInfoResources* tracks{};
    const EventTreeResources* events{};
    const GameResources* resources{};
    const FrontendRaceSetupBindings* raceSetup{};
    ::CarCinematicServices* cameras{};
    FrontendFrame* frame{};
};

struct SelectTrackScreen {
    SelectTrackScreen();
    SelectTrackScreen(SelectTrackScreenServices& services,std::uint32_t screen,ScreenCallbackData& callback);
    ~SelectTrackScreen();
    std::exception_ptr Shutdown() noexcept;
    SelectTrackScreen(const SelectTrackScreen&)=delete;
    SelectTrackScreen& operator=(const SelectTrackScreen&)=delete;
    DataSelectTrackScreen data{};
    SelectableTrackManager* tracks{};
    FrontendPlayerManager* frontend{};
    FlailerUiState* ui{};
    FlailerScreenList* screens{};
    FlailerHighlightAnimation* highlight{};
    std::uint32_t screenKey{};
    std::int32_t selectionState{};
    void ResyncData();
    bool PassJoystickTest(std::int32_t port) const;
    bool CanSelectLaps(const TrackInfo& track) const;
    void StartLapSelector();
    void StopLapSelector();
    void StartRace(const FrontendRaceSetupBindings& bindings);
    std::int32_t ResyncScreen();
    void DoTick();
    void DoRender();
    void ScreenCallback(ScreenMessage message,ScreenCallbackData& callback);
    void HandleLapSelection(ScreenMessage message,ScreenCallbackData& callback);
    FrontendStillCamera* Camera() noexcept;
    // Native inspection of the actual embedded curves, not copied values.
    const tCubic1D* HorizontalMotion() const noexcept;
    const tCubic1D* VerticalMotion() const noexcept;
    float PulsePhase() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

// One source GP-4D30 screen owner. Binding borrows the live frontend services;
// activation/deactivation allocate and delete the concrete screen itself.
void BindTrackSelectScreen(SelectTrackScreenServices& services);
void UnbindTrackSelectScreen();
SelectTrackScreen* CurrentTrackSelectScreen() noexcept;
FlailerScreenConfig& TrackSelectScreenConfiguration();
void TrackSelectScreenCallback(ScreenMessage message,ScreenCallbackData* callback);

bool IsTrackUnavailable(const FrontendPlayerManager& frontend,
    const TrackInfoResources& tracks,std::int32_t track,std::int32_t direction);

} // namespace hp2

#endif
#endif
