#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "RaceParameters.hpp"

struct RaceFrame;
struct RaceCarEntry;
namespace hp2 { class GameResources; class CarInfoResource; enum class CarInfoAvailability; }
namespace hp2 {
class TrackInfoResources;
class LanguageManager;
struct FrontendPlayerManager;
struct EventRaceBindings;
struct FrontendRaceSetupBindings;
class EventTreeResources;

// Native semantic targets for the two distinct writable compiler dispatch
// tables. These are live final-PAL data owners, not PS2 code addresses.
enum class EventProgressKind { Rank,Time,Knockout,Tournament };
extern std::array<EventProgressKind,7> eventStatusDispatch;
extern std::array<EventProgressKind,7> eventRewardDispatch;
// Final3317A0, initially zero. No application-options or profile mirror.
extern std::int32_t eventStatusForceComplete;

struct EventDescriptionToken {
    std::int32_t kind{};
    std::string name;
};
// The final eight-token table is a live owner, not immutable ELF storage.
extern std::array<EventDescriptionToken,8> eventDescriptionTokens;

struct EventTrackEncodingRates {
    float traffic{0.009999999776482582f};
    float opposingTraffic{0.009999999776482582f};
};
// One native projection of the two PAL GP scalar cells32EC50/32EC54.
// They remain live values, rather than treating writable ELF storage as
// immutable merely because its initial percentages happen to match.
extern EventTrackEncodingRates eventTrackEncodingRates;

struct EventTrack {
    // One authored34A02 packed settings word, separate from live settings.
    std::uint32_t configuration{};
    void SetRace(RaceParameters& race,const TrackInfoResources& tracks) const;
};

struct EventGoal {
    std::uint16_t kind{};
    std::uint16_t eventNumber{};
    std::uint32_t value{};
};
struct RaceEvent;
struct EventInfo;
// RaceEvent+1B0 is the original four-entry tUnlockMap. It retains the
// prerequisite goal and destination event number before the race; screens
// query the actual current event's resulting state, never a second copy of
// profile progress in DataEventSelection.
class tUnlockMap final {
public:
    tUnlockMap();
    void Reset();
    void AddGoal(const EventGoal& goal,std::int32_t destination);
    std::int32_t GetEventsUnlocked(std::int32_t* output,std::size_t capacity,
        const RaceEvent& event) const;
    std::int32_t GetNumEventsUnlocked(const RaceEvent& event) const;
    void BuildForEvent(const EventInfo& event);
private:
    struct Entry {EventGoal goal{};std::int32_t destination{};};
    std::array<Entry,4> entries_{};
    std::int32_t count_{};
};
struct EventReward {
    std::uint32_t selection{};
    std::uint32_t parameters{};
};
struct EventRankState {
    std::uint8_t rank{};
    std::int32_t GetStatus(const EventGoal& goal) const;
};
struct EventTimeState {
    std::int32_t ticks{};
    std::int32_t GetStatus(const EventGoal& goal) const;
};
struct EventKoState {
    std::uint32_t ranks{};
    // Eight packed finish positions retained for the next starting grid.
    // Source2019FC writes this word from the prior race's eight results.
    std::uint32_t lastFinishPositions{};
    std::int16_t selectedModel{};
    std::int8_t selectedSkin{};
    std::int8_t selectedVariant{};
    std::uint32_t randomSeed{};
    std::int32_t GetRaceIndex() const;
    std::int32_t GetRank(std::int32_t racer) const;
    std::int32_t GetStatus(const EventGoal& goal) const;
    void SetSelectedCar(std::int32_t model,std::int32_t skin,std::int32_t dashboard);
    std::uint32_t GetRandomSeed() const;
    void NewRandomSeed();
};
struct EventTournamentState {
    std::array<std::uint8_t,8> points{};
    std::int16_t selectedModel{};
    std::int8_t selectedSkin{};
    std::int8_t selectedVariant{};
    std::uint32_t randomSeed{};
    std::int32_t RacerRank(std::int32_t racer) const;
    std::int32_t GetRaceIndex(std::int32_t opponents) const;
    std::int32_t GetStatus(const EventGoal& goal,std::int32_t races,std::int32_t opponents) const;
    void SetSelectedCar(std::int32_t model,std::int32_t skin,std::int32_t dashboard);
    std::uint32_t GetRandomSeed() const;
    void NewRandomSeed();
};
extern std::array<std::uint8_t,8> eventTournamentPoints;

// A borrowed reference to the actual profile's one state owner. The resource
// loader does not invent profile progress, reset it, or duplicate it.
using EventStateReference=std::variant<std::monostate,EventRankState*,EventTimeState*,
    EventKoState*,EventTournamentState*>;
using EventStateSnapshot=std::variant<std::monostate,EventRankState,EventTimeState,
    EventKoState,EventTournamentState>;

// The two256-byte source profile banks are represented as canonical typed
// cells keyed by their save-stream offsets. Cells have stable addresses;
// resources and the selected RaceEvent borrow them. There is no second byte
// buffer mirroring live progress and no per-resource copy of these values.
class EventTreeProgress final {
public:
    EventTreeProgress()=default;
    EventTreeProgress(const EventTreeProgress&)=delete;
    EventTreeProgress& operator=(const EventTreeProgress&)=delete;
    EventTreeProgress(EventTreeProgress&&)=delete;
    EventTreeProgress& operator=(EventTreeProgress&&)=delete;
private:
    friend class EventTree;
    friend struct FrontendProfileStream;
    friend struct FrontendPlayerManager;
    EventStateReference Bind(std::size_t bank,std::size_t offset,std::int32_t kind);
    std::array<std::array<std::unique_ptr<EventStateSnapshot>,256>,2> banks_{};
    EventStateReference sharedBonus_{};
};

class EventTree;
struct EventInfo {
    EventTree* tree{};
    EventStateReference state{};
    std::string name;
    std::int8_t number{};
    std::int8_t kind{};
    std::int8_t category{};
    std::int8_t opponentVehicleClass{};
    std::int32_t pursuitDifficulty{};
    std::int8_t roadBlocksEnabled{};
    std::int8_t authoredPursuitOption26C{};
    std::int8_t authoredPursuitOption268{};
    std::int8_t pulloversEnabled{};
    std::int8_t chopperEnabled{};
    std::int8_t initialRoadBlockRequests{};
    std::int8_t initialChopperRequests{};
    float playerPaceContribution{};
    // Distinct authored EventInfo+0xB0 short blurb, before +0xB4 details.
    std::int32_t summaryLanguageId{};
    std::int32_t descriptionLanguageId{};
    std::int32_t authoredRaceOptionB8{};
    std::int16_t objectiveIndex{};
    std::vector<std::int16_t> carIndices;
    std::vector<std::int16_t> trackIndices;
    std::vector<std::int16_t> goalIndices;
    std::vector<std::int16_t> rewardIndices;
    std::vector<std::int16_t> opponentIndices;

    const std::int32_t* GetCar(std::int32_t index) const;
    const EventTrack* GetTrack(std::int32_t index) const;
    const EventGoal* GetGoal(std::int32_t index) const;
    const EventReward* GetReward(std::int32_t index) const;
    const std::int32_t* GetOpponent(std::int32_t index) const;
    const EventGoal* GetObjective() const;
    EventRankState* GetRankState() const;
    EventTimeState* GetTimeState() const;
    EventKoState* GetKoState() const;
    EventTournamentState* GetTournamentState() const;
    std::int32_t GetRaceCount() const;
    std::int32_t GetRaceIndex() const;
    std::string GetFinishString(const LanguageManager& language) const;
    bool IsMultiTrack() const;
    std::size_t GetStateSize() const;
    std::int32_t GetStatus(const EventGoal* goal=nullptr) const;
    bool IsRewardUnlocked(std::int32_t index) const;
    bool WillSuperBonusEventBeUnlockedIfThisEventIsWon() const;
    void SetSelectedCar(std::int32_t model,std::int32_t skin,std::int32_t dashboard);
};

struct EventResult {
    std::int32_t resultCode{};
    std::int32_t elapsedTicks{};
    std::int32_t interactionExclusion{};
    void Reset();
};

// This is the selected current-race snapshot, separate from profile progress.
// The source intentionally retains both the live progress pointer and its
// value before mutating a per-event seed. Pools are copied as typed values.
struct RaceEvent {
    RaceEvent()=default;
    RaceEvent(const RaceEvent&)=delete;
    RaceEvent& operator=(const RaceEvent&)=delete;
    RaceEvent(RaceEvent&&)=delete;
    RaceEvent& operator=(RaceEvent&&)=delete;
    EventTree* tree{};
    EventStateReference state{};
    // The one persistent saved-progress owner. Rank/time publication writes
    // only its one/four-byte prefix; the selection/seed tail survives. The
    // challenge knockout intentionally borrows this same owner as live state.
    // Tournament points are packed into the two actual result words.
    EventKoState priorState{};
    // Source RaceEvent+18 is the separate start-of-current-race snapshot;
    // +08 above retains the prior event state for reward/rollback decisions.
    EventKoState raceStartState{};
    std::string name;
    std::string rewardName;
    std::int16_t number{};
    std::int16_t kind{};
    std::int16_t category{};
    std::int16_t playerCount{1};
    std::int32_t willUnlockBonus{};
    EventGoal objective{};
    std::vector<std::int32_t> cars;
    std::vector<EventTrack> tracks;
    std::vector<EventReward> rewards;
    std::vector<std::int32_t> opponents;
    std::array<EventResult,8> results{};
    tUnlockMap unlocks;
    void ResetResults();
    bool CaptureStartState();
    bool RecordResult(const ::RaceFrame& race,const ::RaceCarEntry& entry,std::int32_t finishPosition);
    void RaceIsOver(const FrontendPlayerManager& frontend,const RaceParameters& parameters);
    bool CanContinue(const FrontendPlayerManager& frontend,const RaceParameters& parameters) const;
    bool ResultShouldBeDiscarded(const FrontendPlayerManager& frontend) const;
    bool DidBetter(const FrontendPlayerManager& frontend) const;
    bool IsRewardUnlocked(std::int32_t index) const;
    bool AnyRewardUnlocked() const;
    bool IsRewardUnlocked(std::int32_t index,const EventStateReference& progress) const;
    bool IsWinner() const;
    void MaybeDiscardResult(const FrontendPlayerManager& frontend);
    void RestartRace(const FrontendPlayerManager& frontend,const RaceParameters& parameters);
    void RestartEvent(FrontendPlayerManager& frontend,const FrontendRaceSetupBindings& bindings);
    bool SetupNextRace(FrontendPlayerManager& frontend,const FrontendRaceSetupBindings& bindings);
    std::int32_t RacerIndexFromDriverNumber(std::int32_t driverNumber) const;
    std::int32_t DriverNumberFromRacerIndex(std::int32_t racer) const;
    bool RacerEliminated(std::int32_t racer,const RaceParameters& parameters) const;
    bool DriverEliminated(std::int32_t driverNumber,const RaceParameters& parameters) const;
    std::int32_t GetRaceIndex() const;
    EventKoState* GetKoState() const;
    EventTournamentState* GetTournamentState() const;
    std::int32_t GetRaceCount() const;
    std::int32_t GetStatus(const EventStateReference& progress) const;
    std::int32_t GetStatus(const EventGoal& goal) const;
    std::int32_t GetStatus() const;
    std::int32_t GetOldStatus() const;
    void ResetState();
};

struct EventRaceBindings {
    FrontendPlayerManager* frontend{};
    RaceEvent* currentEvent{};
    const CarInfoResource* cars{};
    const TrackInfoResources* tracks{};
    const LanguageManager* language{};
};

// The typed pools below are decoded only from final chunk34A00. All returned
// pointers borrow this retained owner. Changing a selected event does not
// allocate another car/track catalogue or copy live profile progress.
class EventTree final {
public:
    EventTree()=default;
    EventTree(const EventTree&)=delete;
    EventTree& operator=(const EventTree&)=delete;
    EventTree(EventTree&&)=delete;
    EventTree& operator=(EventTree&&)=delete;
    const std::string& Name() const noexcept { return name_; }
    std::int32_t TreeType() const noexcept { return type_; }
    const EventInfo* CurrentEvent() const noexcept { return currentEvent_; }
    EventInfo* GetEventNumber(std::int32_t number);
    void CalcStatePointers(EventTreeProgress& progress,std::size_t bank,std::int32_t alignment);
    void SetRace(RaceParameters& race,EventInfo& event,const EventRaceBindings& bindings);
    bool IsCarUnlocked(std::int32_t model) const;
    bool CarIsAReward(std::int32_t model) const;
    const EventInfo* FindEventCarIsARewardFor(std::int32_t model) const;
    bool IsTrackUnlocked(std::int32_t track,std::int32_t direction) const;
    bool TrackIsAReward(std::int32_t track,std::int32_t direction) const;
    const EventInfo* FindEventTrackIsARewardFor(std::int32_t track,std::int32_t direction) const;
    const std::vector<EventInfo>& Events() const noexcept { return events_; }
    std::size_t StateBytes() const noexcept { return stateBytes_; }
private:
    friend struct EventInfo;
    friend class EventTreeResources;
    friend void FormatEventDescriptionToken(const EventTree&,const EventInfo&,
        const EventDescriptionToken&,char*,const LanguageManager&);
    std::string name_;
    std::int32_t type_{};
    EventInfo* currentEvent_{};
    std::vector<std::int32_t> cars_;
    std::vector<EventTrack> tracks_;
    std::vector<EventGoal> goals_;
    std::vector<EventReward> rewards_;
    std::vector<std::int32_t> opponents_;
    std::vector<EventInfo> events_;
    std::size_t stateBytes_{};
    EventTreeResources* resources_{};
};

const EventDescriptionToken* GetEventDescriptionToken(const char* text);
// output has at least64 bytes. Unknown token kinds leave it unchanged.
void FormatEventDescriptionToken(const EventTree& tree,const EventInfo& event,
    const EventDescriptionToken& token,char* output,const LanguageManager& language);
// output owns limit+1 bytes for limit>=0; zero/negative limits still write
// the empty string. Input and output use the original sequential copy order.
void FormatEventDescription(const EventTree& tree,const EventInfo& event,
    const char* input,char* output,std::int32_t limit,const LanguageManager& language);

class EventTreeResources final {
public:
    explicit EventTreeResources(GameResources& resources);
    EventTreeResources(GameResources& resources,CarInfoAvailability availability);
    explicit EventTreeResources(std::vector<std::uint8_t> authored,bool demoDisc=false);
    EventTreeResources(const EventTreeResources&)=delete;
    EventTreeResources& operator=(const EventTreeResources&)=delete;
    EventTreeResources(EventTreeResources&&)=delete;
    EventTreeResources& operator=(EventTreeResources&&)=delete;
    EventTree* Find(const char* name) const;
    std::size_t TreeCount() const noexcept { return trees_.size(); }
    EventTree& TreeAt(std::size_t index) const;
    EventTree* PrimaryTree(std::size_t index) const;
private:
    std::vector<std::uint8_t> authored_;
    std::vector<std::unique_ptr<EventTree>> trees_;
    std::array<EventTree*,2> primaryTrees_{};
};

bool IsCarAnEventReward(const EventTreeResources& resources,std::int32_t model);
bool EventTreeIsCarUnlocked(const EventTreeResources& resources,std::int32_t model);
const EventInfo* FindEventThatCarIsARewardFor(const EventTreeResources& resources,std::int32_t model);
const EventInfo* FindEventThatTrackIsARewardFor(const EventTreeResources& resources,
    std::int32_t track,std::int32_t direction);
bool IsTrackAnEventReward(const EventTreeResources& resources,std::int32_t track,std::int32_t direction);
bool EventTreeIsTrackUnlocked(const EventTreeResources& resources,std::int32_t track,std::int32_t direction);

} // namespace hp2
