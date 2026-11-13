#include "indep/inc/EventTree.hpp"
#include "indep/inc/EasterEggs.hpp"
#include "Fred.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bWare.hpp"
#include "Language.hpp"
#include "Timer.hpp"
#include "bware/inc/bMath.hpp"
#include "bware/inc/bFile.hpp"
#include <cstring>
#include <utility>
#include <type_traits>

namespace hp2 {
EventTrackEncodingRates eventTrackEncodingRates{};
std::array<std::uint8_t,8> eventTournamentPoints{{10,6,4,3,2,1,0,0}};
std::array<EventProgressKind,7> eventStatusDispatch{{EventProgressKind::Rank,
    EventProgressKind::Time,EventProgressKind::Rank,EventProgressKind::Knockout,
    EventProgressKind::Tournament,EventProgressKind::Time,EventProgressKind::Time}};
std::array<EventProgressKind,7> eventRewardDispatch{{EventProgressKind::Rank,
    EventProgressKind::Time,EventProgressKind::Rank,EventProgressKind::Knockout,
    EventProgressKind::Tournament,EventProgressKind::Time,EventProgressKind::Time}};
std::int32_t eventStatusForceComplete{};

namespace {
template<class T> EventStateReference BindProgressCell(std::unique_ptr<EventStateSnapshot>& cell)
{
    // Final executable-load initialization of both source profile banks is
    // zero. Rebinding a resource retains the existing canonical state.
    if (!cell)cell=std::make_unique<EventStateSnapshot>(T{});
    auto* value=std::get_if<T>(cell.get());
    Require(value,"Event progress slot changed type while borrowed by a live profile");
    return value;
}

bool HasProgressReference(const EventStateReference& reference)
{
    return std::visit([](auto value) {
        if constexpr (std::is_same_v<decltype(value),std::monostate>)return false;
        else return value!=nullptr;
    },reference);
}
}

EventStateReference EventTreeProgress::Bind(std::size_t bank,std::size_t offset,std::int32_t kind)
{
    Require(bank<banks_.size() && offset<256,"Event profile state exceeds its actual bank");
    auto& cell=banks_[bank][offset];
    switch (kind) {
    case 0:case 2:return BindProgressCell<EventRankState>(cell);
    case 1:case 5:case 6:return BindProgressCell<EventTimeState>(cell);
    case 3:return BindProgressCell<EventKoState>(cell);
    case 4:return BindProgressCell<EventTournamentState>(cell);
    default:return std::monostate{};
    }
}

void EventTree::CalcStatePointers(EventTreeProgress& progress,std::size_t bank,std::int32_t alignment)
{
    Require(bank<2 && alignment>0,"Event profile state alignment or bank is invalid");
    stateBytes_=0;
    const auto unit=static_cast<std::size_t>(alignment);
    // The source's first pass binds sizes >=alignment, rounding after each
    // allocation. The second pass appends the smaller states without padding.
    for (unsigned pass=0;pass<2;++pass) {
        for (auto& event:events_) {
            const auto size=event.GetStateSize();
            if ((size>=unit)!=(pass==0))continue;
            if (event.number==32 && HasProgressReference(progress.sharedBonus_)) {
                event.state=progress.sharedBonus_;
                continue;
            }
            Require(stateBytes_<=256 && size<=256-stateBytes_,
                "Authored event progress exceeds its physical profile bank");
            event.state=progress.Bind(bank,stateBytes_,event.kind);
            stateBytes_+=size;
            if (pass==0)stateBytes_=(stateBytes_+unit-1)/unit*unit;
            if (event.number==32)progress.sharedBonus_=event.state;
        }
    }
}

void EventKoState::SetSelectedCar(std::int32_t model,std::int32_t skin,std::int32_t dashboard)
{
    selectedVariant=static_cast<std::int8_t>(dashboard);
    selectedModel=static_cast<std::int16_t>(model);
    selectedSkin=static_cast<std::int8_t>(skin);
}

void EventTournamentState::SetSelectedCar(std::int32_t model,std::int32_t skin,std::int32_t dashboard)
{
    selectedVariant=static_cast<std::int8_t>(dashboard);
    selectedModel=static_cast<std::int16_t>(model);
    selectedSkin=static_cast<std::int8_t>(skin);
}

void EventInfo::SetSelectedCar(std::int32_t model,std::int32_t skin,std::int32_t dashboard)
{
    if (kind==3) {
        auto* progress=GetKoState();Require(progress,"Knockout selection has no live saved state");
        progress->SetSelectedCar(model,skin,dashboard);
    } else if (kind==4) {
        auto* progress=GetTournamentState();Require(progress,"Tournament selection has no live saved state");
        progress->SetSelectedCar(model,skin,dashboard);
    }
}

std::int32_t EventKoState::GetStatus(const EventGoal& goal) const
{
    if (!GetRaceIndex())return 1;
    const auto rank=ranks&15u;
    if (!rank)return 2;
    return goal.value>=rank ? 4 : 3;
}

std::int32_t EventTournamentState::GetStatus(const EventGoal& goal,std::int32_t races,
    std::int32_t opponents) const
{
    const auto index=GetRaceIndex(opponents);
    if (!index)return 1;
    if (index<races)return 2;
    return goal.value>=static_cast<std::uint32_t>(RacerRank(0)) ? 4 : 3;
}

namespace {
template<class T> const T* ReadState(const EventStateReference& state)
{
    const auto* value=std::get_if<T*>(&state);return value ? *value : nullptr;
}
// A read-only typed view replaces the source's void* argument. It owns no
// progress and performs the same per-kind calls for live and prior states.
template<class State> std::int32_t ReadEventStatus(const RaceEvent& race,const State& state,const EventGoal& goal)
{
    if (race.kind==0 || race.kind==2) {
        const auto* value=ReadState<EventRankState>(state);Require(value,"Rank status has no actual saved state");
        return value->GetStatus(goal);
    }
    if (race.kind==1 || race.kind==5 || race.kind==6) {
        const auto* value=ReadState<EventTimeState>(state);Require(value,"Time status has no actual saved state");
        return value->GetStatus(goal);
    }
    if (race.kind==3) {
        const auto* value=ReadState<EventKoState>(state);Require(value,"Knockout status has no actual saved state");
        return value->GetStatus(goal);
    }
    if (race.kind==4) {
        const auto* value=ReadState<EventTournamentState>(state);Require(value,"Tournament status has no actual saved state");
        return value->GetStatus(goal,static_cast<std::int32_t>(race.tracks.size()),
            static_cast<std::int32_t>(race.opponents.size()));
    }
    return 1;
}
}

std::int32_t RaceEvent::GetStatus(const EventStateReference& progress) const
{
    return ReadEventStatus(*this,progress,objective);
}

std::int32_t RaceEvent::GetStatus(const EventGoal& goal) const
{
    return ReadEventStatus(*this,state,goal);
}

std::int32_t RaceEvent::GetStatus() const
{
    return GetStatus(state);
}

std::int32_t RaceEvent::GetOldStatus() const
{
    if (kind==0 || kind==2) {
        const EventRankState rank{static_cast<std::uint8_t>(priorState.ranks)};
        return rank.GetStatus(objective);
    }
    if (kind==1 || kind==5 || kind==6) {
        EventTimeState time;
        std::memcpy(&time.ticks,&priorState.ranks,sizeof(time.ticks));
        return time.GetStatus(objective);
    }
    if (kind==3)return priorState.GetStatus(objective);
    if (kind==4) {
        EventTournamentState tournament;
        for (std::size_t index=0;index<8;++index) {
            const auto word=index<4 ? priorState.ranks : priorState.lastFinishPositions;
            tournament.points[index]=static_cast<std::uint8_t>(word>>((index%4)*8));
        }
        return tournament.GetStatus(objective,static_cast<std::int32_t>(tracks.size()),
            static_cast<std::int32_t>(opponents.size()));
    }
    return 1;
}

void RaceEvent::ResetState()
{
    if (!ReadState<EventRankState>(state) && !ReadState<EventTimeState>(state) &&
        !ReadState<EventKoState>(state) && !ReadState<EventTournamentState>(state))return;
    if (kind==0 || kind==2) {
        auto* const* value=std::get_if<EventRankState*>(&state);
        Require(value && *value,"Rank reset has no actual saved state");(*value)->rank=0;
    } else if (kind==1 || kind==5 || kind==6) {
        auto* const* value=std::get_if<EventTimeState*>(&state);
        Require(value && *value,"Time reset has no actual saved state");(*value)->ticks=0;
    } else if (kind==3) {
        auto* const* value=std::get_if<EventKoState*>(&state);
        Require(value && *value,"Knockout reset has no actual saved state");
        (*value)->lastFinishPositions=0;(*value)->ranks=0;
    } else if (kind==4) {
        auto* const* value=std::get_if<EventTournamentState*>(&state);
        Require(value && *value,"Tournament reset has no actual saved state");(*value)->points.fill(0);
    }
}

bool RaceEvent::IsRewardUnlocked(std::int32_t index) const
{
    return IsRewardUnlocked(index,state);
}
bool RaceEvent::IsRewardUnlocked(std::int32_t index,const EventStateReference& progress) const
{
    // PAL200B30 accepts live or alternate progress through the same complete procedure.
    if(index<0 || static_cast<std::size_t>(index)>=rewards.size() || GetStatus(progress)<3)return false;
    const auto threshold=rewards[static_cast<std::size_t>(index)].parameters;
    if(kind==0 || kind==2) {
        const auto* rank=ReadState<EventRankState>(progress);return rank && static_cast<std::int32_t>(threshold)>=rank->rank;
    }
    if(kind==1 || kind==5 || kind==6) {
        const auto* time=ReadState<EventTimeState>(progress);
        return time && static_cast<std::int32_t>(threshold)>=time->ticks;
    }
    if(kind==3) {
        const auto* knockout=ReadState<EventKoState>(progress);
        return knockout && static_cast<std::int32_t>(threshold)>=knockout->GetRank(0);
    }
    if(kind==4) {
        const auto* tournament=ReadState<EventTournamentState>(progress);
        return tournament && threshold>=static_cast<std::uint32_t>(tournament->RacerRank(0));
    }
    return false;
}
bool RaceEvent::AnyRewardUnlocked() const
{
    for(std::size_t i=0;i<rewards.size();++i)if(IsRewardUnlocked(static_cast<std::int32_t>(i)))return true;
    return false;
}
bool RaceEvent::IsWinner() const
{
    // Complete final direct dependency200C38, a descriptive native API;
    // no independently licensed beta function name is claimed for it.
    if(GetStatus()<3)return false;
    if(kind==0 || kind==2) {const auto* value=ReadState<EventRankState>(state);return value && value->rank==1;}
    if(kind==1 || kind==5 || kind==6) {
        const auto* value=ReadState<EventTimeState>(state);if(!value)return false;
        for(const auto& reward:rewards)if(static_cast<std::int32_t>(reward.parameters)<value->ticks)return false;
        return true;
    }
    if(kind==3) {const auto* value=ReadState<EventKoState>(state);return value && value->GetRank(0)==1;}
    if(kind==4) {const auto* value=ReadState<EventTournamentState>(state);return value && value->RacerRank(0)==1;}
    return false;
}

std::int32_t EventKoState::GetRank(std::int32_t racer) const
{
    if (static_cast<std::uint32_t>(racer)>=8u)return 0;
    return static_cast<std::int32_t>((ranks>>(static_cast<unsigned>(racer)*4))&15u);
}

std::int32_t RaceEvent::RacerIndexFromDriverNumber(std::int32_t driverNumber) const
{
    if (driverNumber<100)return driverNumber;
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(driverNumber)+
        static_cast<std::uint32_t>(playerCount)-100u);
}

std::int32_t RaceEvent::DriverNumberFromRacerIndex(std::int32_t racer) const
{
    if (racer<playerCount)return racer;
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(racer)+100u-
        static_cast<std::uint32_t>(playerCount));
}

bool RaceEvent::RacerEliminated(std::int32_t racer,const RaceParameters& parameters) const
{
    if (parameters.raceKind!=4)return false;
    const auto* typed=std::get_if<EventKoState*>(&state);
    Require(typed && *typed,"Knockout elimination has no live profile state");
    return (*typed)->GetRank(racer)!=0;
}

bool RaceEvent::DriverEliminated(std::int32_t driverNumber,const RaceParameters& parameters) const
{
    return RacerEliminated(RacerIndexFromDriverNumber(driverNumber),parameters);
}

std::int32_t RaceEvent::GetRaceIndex() const
{
    if (kind==3) {
        const auto* typed=std::get_if<EventKoState*>(&state);
        return typed && *typed ? (*typed)->GetRaceIndex() : 0;
    }
    if (kind==4) {
        const auto* typed=std::get_if<EventTournamentState*>(&state);
        return typed && *typed ? (*typed)->GetRaceIndex(static_cast<std::int32_t>(opponents.size())) : 0;
    }
    return 0;
}

void EventTrack::SetRace(RaceParameters& race,const TrackInfoResources& tracks) const
{
    race.selectedScenario=static_cast<std::int32_t>(configuration&255u);
    race.totalLaps=static_cast<std::int32_t>((configuration>>14)&15u);
    race.authoredTrackOptionFirst=static_cast<std::int32_t>((configuration>>10)&3u);
    race.authoredTrackOptionSecond=static_cast<std::int32_t>((configuration>>12)&3u);
    race.reverse=static_cast<std::int32_t>((configuration>>9)&1u);
    const float density=static_cast<float>((configuration>>18)&127u);
    race.trafficDensity=density*eventTrackEncodingRates.traffic;
    const float opposing=static_cast<float>(configuration>>25);
    race.opposingTrafficProbability=opposing*eventTrackEncodingRates.opposingTraffic;
    if (const auto* selected=tracks.Find(static_cast<std::int32_t>(configuration&255u)))
        race.pointToPoint=selected->routeKind;
}

namespace {
template<class T> T* State(const EventStateReference& reference)
{
    const auto* typed=std::get_if<T*>(&reference);
    return typed ? *typed : nullptr;
}

template<class T> const T* Pool(const std::vector<T>& pool,std::int16_t index)
{
    Require(index>=0 && static_cast<std::size_t>(index)<pool.size(),
        "Event contains an invalid authored pool reference");
    return &pool[static_cast<std::size_t>(index)];
}

std::int16_t SignedShort(const std::uint8_t* bytes)
{
    const auto raw=ReadU16(bytes);std::int16_t result;
    std::memcpy(&result,&raw,sizeof(result));return result;
}

std::int32_t SignedWord(const std::uint8_t* bytes)
{
    const auto raw=ReadU32(bytes);std::int32_t result;
    std::memcpy(&result,&raw,sizeof(result));return result;
}

std::string Name(const std::uint8_t* bytes,std::size_t capacity)
{
    const auto* end=static_cast<const std::uint8_t*>(std::memchr(bytes,0,capacity));
    Require(end,"Event name has no authored terminator");
    return {reinterpret_cast<const char*>(bytes),static_cast<std::size_t>(end-bytes)};
}

void Indices(std::vector<std::int16_t>& output,const std::uint8_t* bytes,
    std::int8_t count,std::size_t capacity)
{
    Require(count>=0 && static_cast<std::size_t>(count)<=capacity,
        "Event authored selection count exceeds its field");
    for (std::int32_t index=0;index<count;++index)output.push_back(SignedShort(bytes+index*2));
}
}

EventInfo* EventTree::GetEventNumber(std::int32_t number)
{
    for (auto& event:events_)if (event.number==number)return &event;
    return nullptr;
}

const std::int32_t* EventInfo::GetCar(std::int32_t index) const
{
    if (index<0 || static_cast<std::size_t>(index)>=carIndices.size())return nullptr;
    Require(tree,"Event car selection has no retained tree");
    return Pool(tree->cars_,carIndices[static_cast<std::size_t>(index)]);
}

const EventGoal* EventInfo::GetGoal(std::int32_t index) const
{
    if (index<0 || static_cast<std::size_t>(index)>=goalIndices.size())return nullptr;
    Require(tree,"Event goal selection has no retained tree");
    return Pool(tree->goals_,goalIndices[static_cast<std::size_t>(index)]);
}

const EventTrack* EventInfo::GetTrack(std::int32_t index) const
{
    if (index<0 || static_cast<std::size_t>(index)>=trackIndices.size())return nullptr;
    Require(tree,"Event track selection has no retained tree");
    return Pool(tree->tracks_,trackIndices[static_cast<std::size_t>(index)]);
}

const EventReward* EventInfo::GetReward(std::int32_t index) const
{
    if (index<0 || static_cast<std::size_t>(index)>=rewardIndices.size())return nullptr;
    Require(tree,"Event reward selection has no retained tree");
    return Pool(tree->rewards_,rewardIndices[static_cast<std::size_t>(index)]);
}

const std::int32_t* EventInfo::GetOpponent(std::int32_t index) const
{
    if (index<0 || static_cast<std::size_t>(index)>=opponentIndices.size())return nullptr;
    Require(tree,"Event opponent selection has no retained tree");
    return Pool(tree->opponents_,opponentIndices[static_cast<std::size_t>(index)]);
}

const EventGoal* EventInfo::GetObjective() const
{
    Require(tree,"Event objective has no retained tree");
    return Pool(tree->goals_,objectiveIndex);
}

EventRankState* EventInfo::GetRankState() const
{
    return kind==0 || kind==2 ? State<EventRankState>(state) : nullptr;
}

EventTournamentState* EventInfo::GetTournamentState() const
{
    return kind==4 ? State<EventTournamentState>(state) : nullptr;
}

EventKoState* EventInfo::GetKoState() const
{
    return kind==3 ? State<EventKoState>(state) : nullptr;
}

EventTimeState* EventInfo::GetTimeState() const
{
    return kind==1 || kind==5 || kind==6 ? State<EventTimeState>(state) : nullptr;
}

std::int32_t EventInfo::GetRaceCount() const
{
    if (kind==3)return static_cast<std::int32_t>(opponentIndices.size());
    if (kind==4)return static_cast<std::int32_t>(trackIndices.size());
    return 1;
}

// Complete final PAL2012D8..201350; absent progress is the original zero case.
std::int32_t EventInfo::GetRaceIndex() const
{
    if (const auto* progress=GetKoState())return progress->GetRaceIndex();
    if (const auto* progress=GetTournamentState())
        return progress->GetRaceIndex(static_cast<std::int32_t>(opponentIndices.size()));
    return 0;
}

// PAL200E78..200F98. The returned native string replaces the original shared
// scratch text; it does not own or duplicate the profile's finish state.
std::string EventInfo::GetFinishString(const LanguageManager& language) const
{
    if (GetStatus()<3)return {};
    std::int32_t rank{};
    if (kind==0 || kind==2) {
        const auto* progress=GetRankState();Require(progress,"Event finish has no rank owner");
        rank=progress->rank;
    } else if (kind==3) {
        const auto* progress=GetKoState();Require(progress,"Event finish has no knockout owner");
        rank=progress->GetRank(0);
    } else if (kind==4) {
        const auto* progress=GetTournamentState();Require(progress,"Event finish has no tournament owner");
        rank=progress->RacerRank(0);
    } else {
        auto* progress=GetTimeState();Require(progress,"Event finish has no time owner");
        std::array<char,GAME_TIMER_TEXT_CAPACITY> text{};
        gameTimerPrintToString(&progress->ticks,text.data(),0,&language.currentLanguage);
        return text.data();
    }
    return language.GetString(61000+rank);
}

bool EventInfo::IsMultiTrack() const
{
    return static_cast<std::uint32_t>(static_cast<std::uint8_t>(kind)-3u)<2u;
}

std::int32_t EventRankState::GetStatus(const EventGoal& goal) const
{
    if (!rank)return 1;
    return goal.value>=rank ? 4 : 3;
}

std::int32_t EventTimeState::GetStatus(const EventGoal& goal) const
{
    if (!ticks || ticks==INT32_MAX)return 1;
    std::int32_t target;std::memcpy(&target,&goal.value,sizeof(target));
    return target>=ticks ? 4 : 3;
}

std::int32_t EventKoState::GetRaceIndex() const
{
    std::int32_t index=0;
    for (std::uint32_t shift=0;shift<32;shift+=4)if (ranks&(15u<<shift))++index;
    return index;
}

std::uint32_t EventKoState::GetRandomSeed() const
{
    return randomSeed;
}

void EventKoState::NewRandomSeed()
{
    randomSeed=vectorDefaultRandomBounded(UINT32_MAX);
}

std::int32_t EventTournamentState::GetRaceIndex(std::int32_t opponents) const
{
    Require(opponents>=0 && static_cast<std::size_t>(opponents)<eventTournamentPoints.size(),
        "Tournament opponent count cannot index its finish-points owner");
    std::int32_t perRace=0,total=0;
    for (std::int32_t index=0;index<=opponents;++index)
        perRace+=eventTournamentPoints[static_cast<std::size_t>(index)];
    for (auto score:points)total+=score;
    Require(perRace!=0,"Tournament finish-points total is zero");
    return total/perRace;
}

std::uint32_t EventTournamentState::GetRandomSeed() const
{
    return randomSeed;
}

void EventTournamentState::NewRandomSeed()
{
    randomSeed=vectorDefaultRandomBounded(UINT32_MAX);
}

std::size_t EventInfo::GetStateSize() const
{
    switch (kind) {
    case 1:case 5:case 6:return 4;
    case 3:case 4:return 16;
    default:return 1;
    }
}

std::int32_t EventInfo::GetStatus(const EventGoal* goal) const
{
    if (eventStatusForceComplete)return 4;
    Require(tree,"Event status has no actual retained tree");
    bool secret=false;
    if (number==31 && (tree->type_==0 || tree->type_==1)) {
        const std::size_t code=tree->type_==0 ? 6u : 7u;
        Require(easterEggs.records && code<easterEggs.recordCount,
            "Event status has no canonical EasterEggs table");
        secret=easterEggs.records[code].unlocked!=0;
    }
    if (!goal) {
        if (number==32) {
            Require(tree->resources_,"Bonus event has no retained primary-tree registry");
            auto* championship=tree->resources_->PrimaryTree(0);
            auto* hotPursuit=tree->resources_->PrimaryTree(1);
            Require(championship && hotPursuit,"Bonus event has no two actual primary trees");
            for (std::int32_t id=1;id<=31;++id) {
                const auto* first=championship->GetEventNumber(id);
                const auto* second=hotPursuit->GetEventNumber(id);
                if (first && first->GetStatus()!=4)return secret ? 1 : 0;
                if (second && second->GetStatus()!=4)return secret ? 1 : 0;
            }
        } else if (!goalIndices.empty()) {
            bool satisfied=false;
            // Source evaluates every goal before testing the accumulated OR.
            for (std::size_t index=0;index<goalIndices.size();++index) {
                const auto* prerequisite=GetGoal(static_cast<std::int32_t>(index));
                Require(prerequisite,"Event prerequisite has no authored goal");
                const auto* previous=tree->GetEventNumber(prerequisite->eventNumber);
                if (previous && previous->GetStatus(prerequisite)==4)satisfied=true;
            }
            if (!satisfied)return secret ? 1 : 0;
        }
        goal=GetObjective();
    }
    Require(goal,"Event status has no authored objective");
    std::int32_t result=1;
    if (kind>=0 && static_cast<std::size_t>(kind)<eventStatusDispatch.size()) {
        switch (eventStatusDispatch[static_cast<std::size_t>(kind)]) {
        case EventProgressKind::Rank: {
            auto* progress=State<EventRankState>(state);
            Require(progress,"Rank event has no actual progress cell");
            result=progress->GetStatus(*goal);break;
        }
        case EventProgressKind::Time: {
            auto* progress=State<EventTimeState>(state);
            Require(progress,"Time event has no actual progress cell");
            result=progress->GetStatus(*goal);break;
        }
        case EventProgressKind::Knockout: {
            auto* progress=State<EventKoState>(state);
            Require(progress,"Knockout event has no actual progress cell");
            result=progress->GetStatus(*goal);break;
        }
        case EventProgressKind::Tournament: {
            auto* progress=State<EventTournamentState>(state);
            Require(progress,"Tournament event has no actual progress cell");
            result=progress->GetStatus(*goal,static_cast<std::int32_t>(trackIndices.size()),
                static_cast<std::int32_t>(opponentIndices.size()));break;
        }
        }
    }
    return secret && result==0 ? 1 : result;
}

bool EventInfo::IsRewardUnlocked(std::int32_t index) const
{
    const auto* reward=GetReward(index);
    if (!reward || GetStatus()<3)return false;
    if (kind<0 || static_cast<std::size_t>(kind)>=eventRewardDispatch.size())return false;
    std::int32_t threshold;
    std::memcpy(&threshold,&reward->parameters,sizeof(threshold));
    switch (eventRewardDispatch[static_cast<std::size_t>(kind)]) {
    case EventProgressKind::Rank: {
        auto* progress=GetRankState();Require(progress,"Rank reward has no actual progress cell");
        return threshold>=progress->rank;
    }
    case EventProgressKind::Time: {
        auto* progress=GetTimeState();Require(progress,"Time reward has no actual progress cell");
        return threshold>=progress->ticks;
    }
    case EventProgressKind::Knockout: {
        auto* progress=GetKoState();Require(progress,"Knockout reward has no actual progress cell");
        return threshold>=progress->GetRank(0);
    }
    case EventProgressKind::Tournament: {
        auto* progress=GetTournamentState();Require(progress,"Tournament reward has no actual progress cell");
        return reward->parameters>=static_cast<std::uint32_t>(progress->RacerRank(0));
    }
    }
    return false;
}

bool EventInfo::WillSuperBonusEventBeUnlockedIfThisEventIsWon() const
{
    Require(tree && tree->resources_,"Bonus predicate has no actual primary-tree owner");
    auto* championship=tree->resources_->PrimaryTree(0);
    auto* hotPursuit=tree->resources_->PrimaryTree(1);
    Require(championship && hotPursuit,"Bonus predicate has no two published event trees");
    for (std::int32_t id=1;id<=31;++id) {
        const auto* first=championship->GetEventNumber(id);
        const auto* second=hotPursuit->GetEventNumber(id);
        if (first && first->GetStatus()!=4 && (number!=id || tree!=championship))return false;
        if (second && second->GetStatus()!=4 && (number!=id || tree!=hotPursuit))return false;
    }
    const auto* bonus=championship->GetEventNumber(32);
    Require(bonus,"Bonus predicate has no authored championship event32");
    if (bonus->GetStatus()>0)return false;
    return GetStatus()!=4;
}

bool EventTree::IsCarUnlocked(std::int32_t model) const
{
    for (const auto& event:events_) {
        for (std::size_t index=0;index<event.rewardIndices.size();++index) {
            const auto* reward=event.GetReward(static_cast<std::int32_t>(index));
            Require(reward,"Car reward has no authored pool entry");
            if (!(reward->selection&255u) &&
                static_cast<std::int32_t>((reward->selection>>8)&255u)==model)
                return event.IsRewardUnlocked(static_cast<std::int32_t>(index));
        }
    }
    return false;
}

bool EventTree::CarIsAReward(std::int32_t model) const
{
    for (const auto& event:events_) {
        for (std::size_t index=0;index<event.rewardIndices.size();++index) {
            const auto* reward=event.GetReward(static_cast<std::int32_t>(index));
            Require(reward,"Car reward has no authored pool entry");
            if (!(reward->selection&255u) &&
                static_cast<std::int32_t>((reward->selection>>8)&255u)==model)return true;
        }
    }
    return false;
}

const EventInfo* EventTree::FindEventCarIsARewardFor(std::int32_t model) const
{
    for (const auto& event:events_) {
        for (std::size_t index=0;index<event.rewardIndices.size();++index) {
            const auto* reward=event.GetReward(static_cast<std::int32_t>(index));
            Require(reward,"Car reward has no authored pool entry");
            if (!(reward->selection&255u) &&
                static_cast<std::int32_t>((reward->selection>>8)&255u)==model)return &event;
        }
    }
    return nullptr;
}

bool EventTree::IsTrackUnlocked(std::int32_t track,std::int32_t direction) const
{
    for (const auto& event:events_) {
        for (std::size_t index=0;index<event.rewardIndices.size();++index) {
            const auto* reward=event.GetReward(static_cast<std::int32_t>(index));
            Require(reward,"Track reward has no authored pool entry");
            if ((reward->selection&255u)==1u &&
                static_cast<std::int32_t>((reward->selection>>8)&255u)==track &&
                static_cast<std::int32_t>((reward->selection>>16)&255u)==direction)
                return event.IsRewardUnlocked(static_cast<std::int32_t>(index));
        }
    }
    return true;
}

bool EventTree::TrackIsAReward(std::int32_t track,std::int32_t direction) const
{
    for (const auto& event:events_) {
        for (std::size_t index=0;index<event.rewardIndices.size();++index) {
            const auto* reward=event.GetReward(static_cast<std::int32_t>(index));
            Require(reward,"Track reward has no authored pool entry");
            if ((reward->selection&255u)==1u &&
                static_cast<std::int32_t>((reward->selection>>8)&255u)==track &&
                static_cast<std::int32_t>((reward->selection>>16)&255u)==direction)return true;
        }
    }
    return false;
}

const EventInfo* EventTree::FindEventTrackIsARewardFor(std::int32_t track,std::int32_t direction) const
{
    for (const auto& event:events_) {
        for (std::size_t index=0;index<event.rewardIndices.size();++index) {
            const auto* reward=event.GetReward(static_cast<std::int32_t>(index));
            Require(reward,"Track reward has no authored pool entry");
            if ((reward->selection&255u)==1u &&
                static_cast<std::int32_t>((reward->selection>>8)&255u)==track &&
                static_cast<std::int32_t>((reward->selection>>16)&255u)==direction)return &event;
        }
    }
    return nullptr;
}

bool IsCarAnEventReward(const EventTreeResources& resources,std::int32_t model)
{
    for (std::size_t index=0;index<resources.TreeCount();++index)
        if (resources.TreeAt(index).CarIsAReward(model))return true;
    return false;
}

bool EventTreeIsCarUnlocked(const EventTreeResources& resources,std::int32_t model)
{
    for (std::size_t index=0;index<resources.TreeCount();++index)
        if (resources.TreeAt(index).IsCarUnlocked(model))return true;
    return false;
}

const EventInfo* FindEventThatCarIsARewardFor(const EventTreeResources& resources,std::int32_t model)
{
    for (std::size_t index=0;index<resources.TreeCount();++index) {
        const auto* event=resources.TreeAt(index).FindEventCarIsARewardFor(model);
        if (event)return event;
    }
    return nullptr;
}

const EventInfo* FindEventThatTrackIsARewardFor(const EventTreeResources& resources,
    std::int32_t track,std::int32_t direction)
{
    for (std::size_t index=0;index<resources.TreeCount();++index) {
        const auto* event=resources.TreeAt(index).FindEventTrackIsARewardFor(track,direction);
        if (event)return event;
    }
    return nullptr;
}

bool IsTrackAnEventReward(const EventTreeResources& resources,std::int32_t track,std::int32_t direction)
{
    for (std::size_t index=0;index<resources.TreeCount();++index)
        if (resources.TreeAt(index).TrackIsAReward(track,direction))return true;
    return false;
}

bool EventTreeIsTrackUnlocked(const EventTreeResources& resources,std::int32_t track,std::int32_t direction)
{
    // The source stops at the FIRST tree declaring this track reward.
    for (std::size_t index=0;index<resources.TreeCount();++index) {
        const auto& tree=resources.TreeAt(index);
        if (tree.TrackIsAReward(track,direction))return tree.IsTrackUnlocked(track,direction);
    }
    return true;
}

void EventResult::Reset()
{
    interactionExclusion=0;
    resultCode=0;
    elapsedTicks=0;
}

void RaceEvent::ResetResults()
{
    for (auto& result:results)result.Reset();
}

void EventTree::SetRace(RaceParameters& race,EventInfo& event,const EventRaceBindings& bindings)
{
    Require(bindings.frontend && bindings.currentEvent && bindings.cars && bindings.tracks && bindings.language,
        "Selected event has no canonical frontend/race/resource bindings");
    auto& current=*bindings.currentEvent;
    const auto& language=*bindings.language;
    currentEvent_=&event;
    bindings.frontend->selectedEventNumber=event.number;
    current.name=event.name;
    if (const auto* rank=event.GetRankState())
        current.priorState.ranks=(current.priorState.ranks&0xffffff00u)|rank->rank;
    else if (const auto* time=event.GetTimeState())
        std::memcpy(&current.priorState.ranks,&time->ticks,sizeof(time->ticks));
    else if (const auto* knockout=event.GetKoState())current.priorState=*knockout;
    else if (const auto* tournament=event.GetTournamentState()) {
        current.priorState.ranks=0;current.priorState.lastFinishPositions=0;
        for (std::size_t index=0;index<8;++index) {
            auto& word=index<4 ? current.priorState.ranks : current.priorState.lastFinishPositions;
            word|=static_cast<std::uint32_t>(tournament->points[index])<<((index%4)*8);
        }
        current.priorState.selectedModel=tournament->selectedModel;
        current.priorState.selectedSkin=tournament->selectedSkin;
        current.priorState.selectedVariant=tournament->selectedVariant;
        current.priorState.randomSeed=tournament->randomSeed;
    }
    else Require(false,"Selected event has no matching profile progress owner");
    current.tree=event.tree;current.state=event.state;
    current.number=event.number;current.kind=event.kind;current.category=event.category;
    current.playerCount=1;current.objective=*event.GetObjective();
    current.unlocks.BuildForEvent(event);
    std::int32_t objectiveTicks;
    std::memcpy(&objectiveTicks,&current.objective.value,sizeof(objectiveTicks));
    current.cars.resize(event.carIndices.size());
    for (std::size_t index=0;index<current.cars.size();++index)
        current.cars[index]=*event.GetCar(static_cast<std::int32_t>(index));
    current.tracks.resize(event.trackIndices.size());
    for (std::size_t index=0;index<current.tracks.size();++index)
        current.tracks[index]=*event.GetTrack(static_cast<std::int32_t>(index));
    current.rewards.resize(event.rewardIndices.size());
    for (std::size_t index=0;index<current.rewards.size();++index)
        current.rewards[index]=*event.GetReward(static_cast<std::int32_t>(index));
    current.opponents.resize(event.opponentIndices.size());
    for (std::size_t index=0;index<current.opponents.size();++index)
        current.opponents[index]=*event.GetOpponent(static_cast<std::int32_t>(index));
    const auto byId=[&language](std::int32_t id) {
        const auto* text=language.GetString(id);return text ? text : "";
    };
    const auto byName=[&language](const char* text) {
        const auto key=hashString(text);std::int32_t signedKey;
        std::memcpy(&signedKey,&key,sizeof(key));
        const auto* found=language.GetString(signedKey,true);return found ? found : text;
    };
    const char* rewardName;
    if (current.rewards.empty())rewardName=byName("No reward");
    else {
        const auto selection=current.rewards.front().selection;
        const auto type=selection&255u;
        const auto number=(selection>>8)&255u;
        if (!type)rewardName=byName(bindings.cars->Car(number).GetFullName(language));
        else if (type==1) {
            const auto direction=(selection>>16)&255u;
            const auto* track=bindings.tracks->Find(static_cast<std::int32_t>(number));
            Require(track,"Reward refers to an unpublished authored track");
            rewardName=byId(direction<2 ? track->nameLanguageIds[direction] : 0);
        } else rewardName=byName("Unhandled type");
    }
    current.rewardName.assign(rewardName,std::strlen(rewardName)<32 ? std::strlen(rewardName) : 32);
    FormatEventDescription(*this,event,byId(event.descriptionLanguageId),race.eventDescription,512,language);
    race.eventSelected=1;
    current.ResetResults();
    race.pulloverLimit=2;race.timeLimitTicks=0;race.specialPursuit=0;
    switch (event.kind) {
    case 1:race.raceKind=2;race.timeLimitTicks=objectiveTicks;break;
    case 2:race.raceKind=3;break;
    case 3: {
        auto* progress=event.GetKoState();Require(progress,"Knockout has no actual progress owner");
        race.raceKind=4;
        if (!progress->GetRaceIndex())progress->NewRandomSeed();
        race.eventRandomSeed=progress->GetRandomSeed();break;
    }
    case 4: {
        auto* progress=event.GetTournamentState();Require(progress,"Tournament has no actual progress owner");
        race.raceKind=5;
        if (!progress->GetRaceIndex(static_cast<std::int32_t>(event.opponentIndices.size())))progress->NewRandomSeed();
        race.eventRandomSeed=progress->GetRandomSeed();break;
    }
    case 5:race.pointToPoint=1;race.raceKind=1;race.timeLimitTicks=objectiveTicks;break;
    case 6:
        race.pulloverLimit=0;race.specialPursuit=1;race.resetPursuitTimerAfterPullover=0;
        race.raceKind=1;race.timeLimitTicks=objectiveTicks;break;
    default:race.raceKind=1;break;
    }
    race.mode=1;race.opponentCount=static_cast<std::int32_t>(event.opponentIndices.size());
    race.policeDriverCount=type_==1 ? 6 : 0;
    race.opponentVehicleClass=event.opponentVehicleClass;race.pursuitDifficulty=event.pursuitDifficulty;
    race.pulloversEnabled=event.pulloversEnabled;race.roadBlocksEnabled=event.roadBlocksEnabled;
    race.authoredPursuitOption268=event.authoredPursuitOption268;
    race.authoredPursuitOption26C=event.authoredPursuitOption26C;race.chopperEnabled=event.chopperEnabled;
    race.initialRoadBlockRequests=event.initialRoadBlockRequests;race.initialChopperRequests=event.initialChopperRequests;
    race.authoredRaceOption4C=0;race.authoredRaceOption40=event.authoredRaceOptionB8!=0;
    const auto firstStartingPosition=static_cast<std::uint8_t>(race.opponentCount+1);
    std::memcpy(&race.playerStartingPositions[0],&firstStartingPosition,1);
    race.playerPaceContribution[0]=event.playerPaceContribution;
    race.playerPaceContribution[1]=event.playerPaceContribution;
}

std::int32_t EventTournamentState::RacerRank(std::int32_t racer) const
{
    Require(racer>=0 && static_cast<std::size_t>(racer)<points.size(),
        "Tournament racer is outside the actual eight positions");
    const auto value=points[static_cast<std::size_t>(racer)];std::int32_t rank=1;
    for (std::int32_t other=0;other<8;++other) {
        if (other==racer)continue;
        const auto score=points[static_cast<std::size_t>(other)];
        if (value<score || (value==score && other<racer))++rank;
    }
    return rank;
}

EventTreeResources::EventTreeResources(GameResources& resources)
    : EventTreeResources(resources.Load("FRONTEND/FRONTB.LZC"))
{
}

EventTreeResources::EventTreeResources(GameResources& resources,CarInfoAvailability availability)
    : EventTreeResources(resources.Load("FRONTEND/FRONTB.LZC"),availability==CarInfoAvailability::DemoDisc)
{
}

EventTreeResources::EventTreeResources(std::vector<std::uint8_t> authored,bool demoDisc)
    : authored_(std::move(authored))
{
    const auto visit=[&](const auto& self,std::size_t begin,std::size_t size,unsigned depth)->void {
        Require(depth<=32,"Event tree resource nesting exceeds32levels");
        for (const auto& chunk:ReadChunks(authored_,begin,size)) {
            if (chunk.id==0x80034a00u) {
                auto tree=std::make_unique<EventTree>();
                tree->resources_=this;
                bool named=false,events=false;
                for (const auto& child:ReadChunks(authored_,chunk.payloadOffset,chunk.size)) {
                    const auto* bytes=authored_.data()+child.payloadOffset;
                    if (child.id==1) {
                        Require(!named,"Event tree repeats its name");named=true;
                        tree->name_=Name(bytes,child.size);
                        if (!stringCompareInsensitive(tree->name_.c_str(),demoDisc ? "Hot Pursuit E3" : "Hot Pursuit"))
                            tree->type_=1;
                        else Require(!stringCompareInsensitive(tree->name_.c_str(),demoDisc ? "Championship E3" : "Championship"),
                            "Event tree name does not belong to the bound content policy");
                    } else if (child.id==0x34a01u || child.id==0x34a06u) {
                        Require(child.size%4==0,"Event integer pool is truncated");
                        auto& pool=child.id==0x34a01u ? tree->cars_ : tree->opponents_;
                        Require(pool.empty(),"Event tree repeats an integer pool");
                        for (std::size_t i=0;i<child.size;i+=4)pool.push_back(SignedWord(bytes+i));
                    } else if (child.id==0x34a02u) {
                        Require(tree->tracks_.empty() && child.size%4==0,"Event track pool is repeated or truncated");
                        for (std::size_t i=0;i<child.size;i+=4)tree->tracks_.push_back({ReadU32(bytes+i)});
                    } else if (child.id==0x34a03u) {
                        Require(tree->goals_.empty() && child.size%8==0,"Event goal pool is repeated or truncated");
                        for (std::size_t i=0;i<child.size;i+=8)
                            tree->goals_.push_back({ReadU16(bytes+i),ReadU16(bytes+i+2),ReadU32(bytes+i+4)});
                    } else if (child.id==0x34a04u) {
                        Require(tree->rewards_.empty() && child.size%8==0,"Event reward pool is repeated or truncated");
                        for (std::size_t i=0;i<child.size;i+=8)
                            tree->rewards_.push_back({ReadU32(bytes+i),ReadU32(bytes+i+4)});
                    } else if (child.id==0x34a05u) {
                        Require(!events && child.size%188==0,"Event records are repeated or truncated");events=true;
                        tree->events_.resize(child.size/188);
                        for (std::size_t index=0;index<tree->events_.size();++index) {
                            const auto* record=bytes+index*188;auto& event=tree->events_[index];
                            event.tree=tree.get();event.name=Name(record+8,32);
                            std::memcpy(&event.number,record+0x28,1);std::memcpy(&event.kind,record+0x29,1);
                            std::memcpy(&event.category,record+0x2a,1);
                            std::memcpy(&event.opponentVehicleClass,record+0x2b,1);
                            event.pursuitDifficulty=SignedWord(record+0x30);
                            std::memcpy(&event.roadBlocksEnabled,record+0x34,1);
                            std::memcpy(&event.authoredPursuitOption26C,record+0x35,1);
                            std::memcpy(&event.authoredPursuitOption268,record+0x36,1);
                            std::memcpy(&event.pulloversEnabled,record+0x37,1);
                            std::memcpy(&event.chopperEnabled,record+0x38,1);
                            std::memcpy(&event.initialRoadBlockRequests,record+0x39,1);
                            std::memcpy(&event.initialChopperRequests,record+0x3a,1);
                            const auto pace=ReadU32(record+0xac);
                            std::memcpy(&event.playerPaceContribution,&pace,sizeof(pace));
                            event.summaryLanguageId=SignedWord(record+0xb0);
                            event.descriptionLanguageId=SignedWord(record+0xb4);
                            event.authoredRaceOptionB8=SignedWord(record+0xb8);
                            event.objectiveIndex=SignedShort(record+0x42);
                            Indices(event.carIndices,record+0x44,static_cast<std::int8_t>(record[0x3b]),20);
                            Indices(event.trackIndices,record+0x6c,static_cast<std::int8_t>(record[0x3c]),8);
                            Indices(event.goalIndices,record+0x7c,static_cast<std::int8_t>(record[0x3d]),8);
                            Indices(event.rewardIndices,record+0x8c,static_cast<std::int8_t>(record[0x3e]),8);
                            Indices(event.opponentIndices,record+0x9c,static_cast<std::int8_t>(record[0x3f]),8);
                        }
                    }
                }
                Require(named && events,"Event tree is missing its name or records");
                for (const auto& event:tree->events_) {
                    for (auto index:event.carIndices)(void)Pool(tree->cars_,index);
                    for (auto index:event.trackIndices)(void)Pool(tree->tracks_,index);
                    for (auto index:event.goalIndices)(void)Pool(tree->goals_,index);
                    for (auto index:event.rewardIndices)(void)Pool(tree->rewards_,index);
                    for (auto index:event.opponentIndices)(void)Pool(tree->opponents_,index);
                }
                Require(tree->type_==0 || tree->type_==1,"Event tree has no primary publication slot");
                primaryTrees_[static_cast<std::size_t>(tree->type_)]=tree.get();
                trees_.push_back(std::move(tree));
            } else if (chunk.IsContainer())self(self,chunk.payloadOffset,chunk.size,depth+1);
        }
    };
    visit(visit,0,authored_.size(),0);
    Require(!trees_.empty(),"Frontend has no authored event trees");
}

EventTree* EventTreeResources::Find(const char* name) const
{
    if (!name)return nullptr;
    for (const auto& tree:trees_)if (!stringCompareInsensitive(tree->Name().c_str(),name))return tree.get();
    return nullptr;
}

EventTree& EventTreeResources::TreeAt(std::size_t index) const
{
    Require(index<trees_.size(),"Event tree index exceeds its retained resource");
    return *trees_[index];
}

EventTree* EventTreeResources::PrimaryTree(std::size_t index) const
{
    Require(index<primaryTrees_.size(),"Event primary-tree index exceeds its two source slots");
    return primaryTrees_[index];
}

} // namespace hp2

namespace hp2 {
EventKoState* RaceEvent::GetKoState() const
{
    if (kind!=3)return nullptr;
    const auto* owner=std::get_if<EventKoState*>(&state);
    return owner?*owner:nullptr;
}
EventTournamentState* RaceEvent::GetTournamentState() const
{
    if (kind!=4)return nullptr;
    const auto* owner=std::get_if<EventTournamentState*>(&state);
    return owner?*owner:nullptr;
}
std::int32_t RaceEvent::GetRaceCount() const
{
    if (kind==3)return static_cast<std::int32_t>(opponents.size());
    if (kind==4)return static_cast<std::int32_t>(tracks.size());
    return 1;
}
}

#include "Race.hpp"
#include "Pursuit.hpp"
#include "indep/inc/PhysicsCar.hpp"

namespace hp2 {
namespace {
bool CaptureEventProgress(const EventStateReference& state,EventKoState& snapshot)
{
    if(const auto* rank=ReadState<EventRankState>(state))
        snapshot.ranks=(snapshot.ranks&0xffffff00u)|rank->rank;
    else if(const auto* time=ReadState<EventTimeState>(state))
        std::memcpy(&snapshot.ranks,&time->ticks,sizeof(time->ticks));
    else if(const auto* knockout=ReadState<EventKoState>(state))snapshot=*knockout;
    else if(const auto* tournament=ReadState<EventTournamentState>(state)) {
        snapshot.ranks=0;snapshot.lastFinishPositions=0;
        for(std::size_t i=0;i<8;++i) {
            auto& word=i<4?snapshot.ranks:snapshot.lastFinishPositions;
            word|=static_cast<std::uint32_t>(tournament->points[i])<<((i%4)*8);
        }
        snapshot.selectedModel=tournament->selectedModel;snapshot.selectedSkin=tournament->selectedSkin;
        snapshot.selectedVariant=tournament->selectedVariant;snapshot.randomSeed=tournament->randomSeed;
    } else return false;
    return true;
}
}

// PAL201A80 copies the current progress into the RaceEvent-owned start state.
bool RaceEvent::CaptureStartState()
{
    return CaptureEventProgress(state,raceStartState);
}

bool RaceEvent::RecordResult(const RaceFrame& race,const RaceCarEntry& entry,std::int32_t position)
{
    if(!entry.car || !race.services || !race.services->settings)return false;
    const auto index=RacerIndexFromDriverNumber(entry.car->driverNumber);
    if(index<0 || static_cast<std::size_t>(index)>=results.size())return false;
    const auto& parameters=*race.services->settings;
    auto ticks=parameters.raceKind==2?entry.bestSegmentTicks[race.finishCheckpoint]:entry.finishTicks;
    if(parameters.specialPursuit) {
        if(!race.services->pursuit)return false;
        physicsSecondsToTicks(&ticks,race.services->pursuit->remainingSeconds);
    }
    results[static_cast<std::size_t>(index)]={position,ticks,entry.completionReason};
    return true;
}

// Complete PAL2018F8. Publication uses the same profile variant selected by
// EventTree::SetRace. An aborted race never advances that persisted state.
void RaceEvent::RaceIsOver(const FrontendPlayerManager& frontend,const RaceParameters& parameters)
{
    if(frontend.raceAborted)return;
    if(kind==0 || kind==2) {
        if(auto* const* cell=std::get_if<EventRankState*>(&state);cell && *cell)
            (*cell)->rank=static_cast<std::uint8_t>(results[0].resultCode);
    } else if(kind==1 || kind==5 || kind==6) {
        if(auto* const* cell=std::get_if<EventTimeState*>(&state);cell && *cell)
            (*cell)->ticks=results[0].elapsedTicks;
    } else if(kind==3) {
        auto* const* cell=std::get_if<EventKoState*>(&state);
        if(!cell || !*cell)return;
        auto& progress=**cell;
        std::int32_t last=0,lastPosition=0,first=0,firstPosition=8;
        for(std::int32_t index=0;index<8;++index) {
            const auto driver=DriverNumberFromRacerIndex(index);
            const auto* info=raceParametersGetDriverInfoByDriverNumber(
                const_cast<RaceParameters*>(&parameters),driver);
            if(!info || info->eliminated!=0)continue;
            const auto position=results[static_cast<std::size_t>(index)].resultCode;
            if(lastPosition<position){lastPosition=position;last=index;}
            if(position<firstPosition){firstPosition=position;first=index;}
        }
        const auto assignRank=[&](std::int32_t index) {
            const auto shift=static_cast<unsigned>(index)*4;
            const auto rank=static_cast<std::uint32_t>(results[static_cast<std::size_t>(index)].resultCode)&15u;
            progress.ranks=(progress.ranks&~(15u<<shift))|(rank<<shift);
        };
        assignRank(last);
        if(parameters.mode+parameters.opponentCount<3)assignRank(first);
        for(std::size_t index=0;index<8;++index) {
            const auto shift=static_cast<unsigned>(index)*4;
            const auto rank=static_cast<std::uint32_t>(results[index].resultCode)&15u;
            progress.lastFinishPositions=(progress.lastFinishPositions&~(15u<<shift))|(rank<<shift);
        }
    } else if(kind==4) {
        auto* const* cell=std::get_if<EventTournamentState*>(&state);
        if(!cell || !*cell)return;
        for(std::size_t index=0;index<8;++index) {
            const auto position=results[index].resultCode;
            if(position==0)continue;
            Require(position>0 && position<=8,"Tournament finish rank exceeds the authored points table");
            (*cell)->points[index]=static_cast<std::uint8_t>((*cell)->points[index]+
                eventTournamentPoints[static_cast<std::size_t>(position-1)]);
        }
    }
}
}

namespace hp2 {
namespace {
void RestoreEventProgress(const EventKoState& snapshot,EventStateReference& state)
{
    if(auto* const* rank=std::get_if<EventRankState*>(&state);rank && *rank)
        (*rank)->rank=static_cast<std::uint8_t>(snapshot.ranks);
    else if(auto* const* time=std::get_if<EventTimeState*>(&state);time && *time)
        std::memcpy(&(*time)->ticks,&snapshot.ranks,sizeof(snapshot.ranks));
    else if(auto* const* knockout=std::get_if<EventKoState*>(&state);knockout && *knockout)
        **knockout=snapshot;
    else if(auto* const* tournament=std::get_if<EventTournamentState*>(&state);tournament && *tournament) {
        for(std::size_t index=0;index<8;++index) {
            const auto word=index<4?snapshot.ranks:snapshot.lastFinishPositions;
            (*tournament)->points[index]=static_cast<std::uint8_t>(word>>((index%4)*8));
        }
        (*tournament)->selectedModel=snapshot.selectedModel;
        (*tournament)->selectedSkin=snapshot.selectedSkin;
        (*tournament)->selectedVariant=snapshot.selectedVariant;
        (*tournament)->randomSeed=snapshot.randomSeed;
    } else throw std::runtime_error("RaceEvent restore has no canonical progress cell");
}
EventTournamentState SavedTournament(const EventKoState& snapshot)
{
    EventTournamentState result;
    EventStateReference reference=&result;
    RestoreEventProgress(snapshot,reference);
    return result;
}
bool ValidEventTime(std::int32_t ticks)
{
    return ticks!=0 && ticks!=INT32_MAX;
}
}

// PAL202420, IsThereAnotherRace. The final driver elimination test is read
// from the same RaceParameters table already used to create this race.
bool RaceEvent::CanContinue(const FrontendPlayerManager& frontend,const RaceParameters& parameters) const
{
    if(kind==3) {
        std::int32_t activePlayers=0;
        for(std::int32_t localPlayerNumber=0;localPlayerNumber<frontend.playerCount;++localPlayerNumber) {
            const auto* driver=raceParametersGetDriverInfoByDriverNumber(const_cast<RaceParameters*>(&parameters),localPlayerNumber);
            if(driver && driver->eliminated==0)++activePlayers;
        }
        std::int32_t last=0,position=0;
        for(std::int32_t racer=0;racer<8;++racer) {
            const auto* driver=raceParametersGetDriverInfoByDriverNumber(const_cast<RaceParameters*>(&parameters),DriverNumberFromRacerIndex(racer));
            if(!driver || driver->eliminated!=0)continue;
            if(position<results[static_cast<std::size_t>(racer)].resultCode) {
                last=racer;position=results[static_cast<std::size_t>(racer)].resultCode;
            }
        }
        if(static_cast<std::uint32_t>(DriverNumberFromRacerIndex(last))<=1u)--activePlayers;
        return activePlayers>0 && parameters.mode+parameters.opponentCount>=3;
    }
    return kind==4 && GetRaceIndex()+1<=GetRaceCount();
}

// Full PAL201BD8/201DB8. The comparison direction differs for knockout
// ranks and a tournament in progress; neither can use one generic min().
bool RaceEvent::ResultShouldBeDiscarded(const FrontendPlayerManager& frontend) const
{
    const auto previous=GetOldStatus(),current=GetStatus();
    if(current<previous)return previous!=3;
    if(current!=previous)return false;
    if(kind==0 || kind==2) {
        const auto* value=ReadState<EventRankState>(state);if(!value)return false;
        const auto old=static_cast<std::uint8_t>(priorState.ranks);
        return old!=0 && (value->rank==0 || old<value->rank);
    }
    if(kind==1 || kind==5 || kind==6) {
        const auto* value=ReadState<EventTimeState>(state);if(!value)return false;
        std::int32_t old;std::memcpy(&old,&priorState.ranks,sizeof(old));
        return ValidEventTime(old) && (!ValidEventTime(value->ticks) || old<value->ticks);
    }
    if(kind==3) {
        const auto* value=GetKoState();if(!frontend.IsEventGame() || !value)return false;
        const auto old=priorState.GetRank(0),rank=value->GetRank(0);
        return old==0 ? rank!=0 : rank!=0 && old<rank;
    }
    if(kind==4) {
        const auto* value=GetTournamentState();
        return value && previous>=2 && SavedTournament(priorState).RacerRank(0)<value->RacerRank(0);
    }
    return false;
}
bool RaceEvent::DidBetter(const FrontendPlayerManager& frontend) const
{
    const auto previous=GetOldStatus(),current=GetStatus();
    if(previous!=current)return previous<current;
    if(kind==0 || kind==2) {
        const auto* value=ReadState<EventRankState>(state);if(!value)return false;
        const auto old=static_cast<std::uint8_t>(priorState.ranks);
        return value->rank!=0 && (old==0 || value->rank<old);
    }
    if(kind==1 || kind==5 || kind==6) {
        const auto* value=ReadState<EventTimeState>(state);if(!value)return false;
        std::int32_t old;std::memcpy(&old,&priorState.ranks,sizeof(old));
        return ValidEventTime(value->ticks) && (!ValidEventTime(old) || value->ticks<old);
    }
    if(kind==3) {
        const auto* value=GetKoState();if(!frontend.IsEventGame() || !value)return false;
        const auto old=priorState.GetRank(0),rank=value->GetRank(0);
        if(rank==0 && old!=0)return true;
        return old!=0 && rank!=0 && rank<old;
    }
    if(kind==4) {
        const auto* value=GetTournamentState();
        if(!value || current<3)return false;
        return previous<2 || value->RacerRank(0)<SavedTournament(priorState).RacerRank(0);
    }
    return false;
}
void RaceEvent::MaybeDiscardResult(const FrontendPlayerManager& frontend)
{
    if(ResultShouldBeDiscarded(frontend))RestoreEventProgress(priorState,state);
}
void RaceEvent::RestartRace(const FrontendPlayerManager& frontend,const RaceParameters& parameters)
{
    if((kind!=3 && kind!=4) || !CanContinue(frontend,parameters))
        Require(CaptureEventProgress(state,priorState),"RestartRace has no canonical saved progress");
    RestoreEventProgress(raceStartState,state);
}
void RaceEvent::RestartEvent(FrontendPlayerManager& frontend,const FrontendRaceSetupBindings& bindings)
{
    Require(bindings.parameters && bindings.tracks && bindings.currentEvent==this,
        "RestartEvent has no canonical selected event/race/track owners");
    auto& parameters=*bindings.parameters;
    if(!CanContinue(frontend,parameters) && !ResultShouldBeDiscarded(frontend))
        Require(CaptureEventProgress(state,priorState),"RestartEvent has no canonical saved progress");
    ResetState();ResetResults();
    raceParametersClearEliminatedDrivers(&parameters);
    raceParametersResetStartingPositions(&parameters);
    frontend.CalculateStartingPositions(bindings);
    Require(!tracks.empty(),"RestartEvent has no authored first race track");
    tracks.front().SetRace(parameters,*bindings.tracks);
}
}

namespace hp2 {
bool RaceEvent::SetupNextRace(FrontendPlayerManager& frontend,const FrontendRaceSetupBindings& bindings)
{
    Require(bindings.parameters && bindings.tracks && bindings.runtimeSettings && frontend.players[0],
        "Next event race has no canonical profile/settings/track owners");
    auto& parameters=*bindings.parameters;
    if(!CanContinue(frontend,parameters))return false;
    bindings.runtimeSettings->players[0]=frontend.players[0]->settings;
    if(kind==3) {
        std::int32_t last=0,position=0;
        for(std::int32_t racer=0;racer<8;++racer) {
            const auto* driver=raceParametersGetDriverInfoByDriverNumber(&parameters,DriverNumberFromRacerIndex(racer));
            if(!driver || driver->eliminated!=0)continue;
            if(position<results[static_cast<std::size_t>(racer)].resultCode) {
                last=racer;position=results[static_cast<std::size_t>(racer)].resultCode;
            }
        }
        (void)raceParametersEliminateDriver(&parameters,DriverNumberFromRacerIndex(last),
            results[static_cast<std::size_t>(last)].resultCode);
    }
    if(kind==3 || kind==4) {
        if(GetRaceIndex()!=0) {
            if(kind==3) {
                parameters.playerStartingPositions[0]=static_cast<std::int8_t>(results[0].resultCode);
                parameters.playerStartingPositions[1]=static_cast<std::int8_t>(results[1].resultCode);
                for(std::int32_t racer=0;racer<8;++racer) {
                    auto* driver=raceParametersGetDriverInfoByDriverNumber(&parameters,DriverNumberFromRacerIndex(racer));
                    if(driver && driver->eliminated==0)
                        driver->startingPosition=static_cast<std::int8_t>(results[static_cast<std::size_t>(racer)].resultCode);
                }
            } else {
                const auto* progress=GetTournamentState();Require(progress,"Tournament continuation has no live points");
                parameters.playerStartingPositions[0]=static_cast<std::int8_t>(progress->RacerRank(0));
                for(std::int32_t racer=0;racer<8;++racer) {
                    auto* driver=raceParametersGetDriverInfoByDriverNumber(&parameters,DriverNumberFromRacerIndex(racer));
                    if(driver && driver->eliminated==0)driver->startingPosition=static_cast<std::int8_t>(progress->RacerRank(racer));
                }
            }
        }
        Require(!tracks.empty(),"Next event race has no authored tracks");
        const auto index=GetRaceIndex()%static_cast<std::int32_t>(tracks.size());
        Require(index>=0,"Next event race index is negative");
        tracks[static_cast<std::size_t>(index)].SetRace(parameters,*bindings.tracks);
    }
    return true;
}
}
