#pragma once
// TASK-0097 E-0097-D01/D02/D03. Original Debug header marker:
// ../indep/inc/EventTree.hpp. Debug NFSHP2.ELF SHA815b60c7...332f1b ONLY.
// hp2::debug separates this behavior authority from existing Retail projections.
// Standard containers/explicit service arguments are host bindings, NOT PS2 ABI.
#include "game_core/port/driving/byte_view.hpp"
#include "game_core/indep/src/timer.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>
namespace port::race {struct EventLaunchServices;}

namespace hp2::debug {
struct EventTree;
struct RaceEvent;
enum eEventStatus {eEVENT_LOCKED=0,eEVENT_UNTRIED=1,eEVENT_INPROGRESS=2,eEVENT_FAILED=3,eEVENT_PASSED=4,eEVENT_MAX_STATUS=5};
struct EventGoal {
    // Debug001FB4A8, E-0099-W82; original eventtree.cpp definition.
    static bool Passed(const ::port::driving::Memory&,std::uint32_t goal,std::uint32_t tree);
    std::uint16_t nType=0,nEvent=0;std::uint32_t nData=0;
};
// Reward union fields are intentionally opaque here, pending their consumers.
struct EventReward { std::array<std::uint8_t,8> Field_0x00{}; };
struct EventInfo {
    static void GetSelectedCar(const ::port::driving::Memory&,std::uint32_t,int&,int&,int&); //001FC618, E-0101-PL01.
    static void SetSelectedCar(::port::driving::Memory&,std::uint32_t,int,int,int); //001FC5E8, W86.
    static int GetStateSize(const ::port::driving::Memory&,std::uint32_t); //001FC028, W72 raw overload.
    static int GetRaceCount(const ::port::driving::Memory&,std::uint32_t); //001FC4B0.
    static std::uint32_t GetCar(const ::port::driving::Memory&,std::uint32_t,int);
    static std::uint32_t GetGoal(const ::port::driving::Memory&,std::uint32_t,int);
    static std::uint32_t GetTrack(const ::port::driving::Memory&,std::uint32_t,int);
    static std::uint32_t GetReward(const ::port::driving::Memory&,std::uint32_t,int);
    static std::uint32_t GetOpponent(const ::port::driving::Memory&,std::uint32_t,int);
    static std::uint32_t GetObjective(const ::port::driving::Memory&,std::uint32_t);
    // Debug001FC2A8/001FC1E8, E-0099-W82; raw source globals/state, not decoded profile.
    static eEventStatus GetStatus(const ::port::driving::Memory&,std::uint32_t,std::uint32_t goal=0);
    static bool WillSuperBonusEventBeUnlockedIfThisEventIsWon(const ::port::driving::Memory&,std::uint32_t);
    std::string sName;
    std::array<char,32> port_name_bytes{}; // Serialized name carrier for exact32-byte copy.
    std::int8_t nNumber=0,nType=0,nClass=0,nOpponentSelect=0,nCops=0;
    std::int32_t eCopStrength=0;
    std::int8_t bRoadBlocks=0,bSpikeStrips=0,bBarricadeSmokeables=0,bTickets=0,bHelicopter=0;
    std::int8_t nNumBeACopRoadBlocks=0,nNumBeACopChoppers=0;
    std::int8_t nCars=0,nTracks=0,nGoals=0,nRewards=0,nOpponents=0,bZoneEnabled=0;
    std::int16_t nObjective=0;
    std::array<std::int16_t,20> aCars{};
    std::array<std::int16_t,8> aTracks{},aGoals{},aRewards{};
    std::array<std::int16_t,7> aOpponents{};
    float fOpponentBoostScale=0;
    std::int32_t mTitleString=0,mDescriptionString=0,mDamage=0;
    // Debug001FBBC8/001FBC08/001FBC48/001FBC88/001FBCC8/001FBD08,
    // E-0097-D02. Original caller-index bounds return null. Invalid asset table
    // indices are outside the recovered valid memory domain and throw explicitly.
    const std::uint32_t* GetCar(const EventTree&,int) const;
    const EventGoal* GetGoal(const EventTree&,int) const;
    const std::uint32_t* GetTrack(const EventTree&,int) const;
    const EventReward* GetReward(const EventTree&,int) const;
    const std::uint32_t* GetOpponent(const EventTree&,int) const;
    const EventGoal* GetObjective(const EventTree&) const;
    // Debug001FC028 E-0097-D02. Race-KO is12 bytes, NOT Retail16.
    int GetStateSize() const;
    bool WillSuperBonusEventBeUnlockedIfThisEventIsWon() const;
};
struct EventTree {
    // W72 original logical-memory overloads; decoded members below are never
    // reinterpreted as the original 0x120-byte layout or profile state.
    static void Sub_001F8688(::port::driving::Memory&,std::uint32_t,std::uint32_t,std::optional<std::uint32_t> center_w);
    static void Sub_001F87A8(::port::driving::Memory&,std::uint32_t,int);
    static bool SetName(::port::driving::Memory&,std::uint32_t,std::uint32_t);
    static void SetEvents(::port::driving::Memory&,std::uint32_t,std::uint32_t,int);
    static void CalcStatePointers(::port::driving::Memory&,std::uint32_t,std::uint32_t,int,int);
    static std::uint32_t GetEventNumber(const ::port::driving::Memory&,std::uint32_t,int);
    static void SetRace(::port::driving::Memory&,std::uint32_t tree,std::uint32_t race,std::uint32_t info); //001FA378, W72.
    std::string sName;
    int nType=-1;
    std::vector<std::uint32_t> pCars,pTracks,pOpponents;
    std::vector<EventGoal> pGoals;
    std::vector<EventReward> pRewards;
    std::vector<EventInfo> pEvents;
    const EventInfo* pCurrentEvent=nullptr;
    void SetRace(::port::driving::init::ByteView,const EventInfo&,RaceEvent&,
        ::port::driving::init::ByteView event_state,const ::port::race::EventLaunchServices&);
};
struct EventRankState {
 static void Reset(::port::driving::Memory&,std::uint32_t); //001FB4F8, W78.
 static eEventStatus GetStatus(const ::port::driving::Memory&,std::uint32_t,std::uint32_t); //001FB510.
};
struct EventKoState {
 static void SetRank(::port::driving::Memory&,std::uint32_t,int,int); //001FB7E8,E-0110-F01.
 static void SetSelectedCar(::port::driving::Memory&,std::uint32_t,int,int,int); //001FB958,W86.
 static void Reset(::port::driving::Memory&,std::uint32_t); //001FB5A0.
 static eEventStatus GetStatus(const ::port::driving::Memory&,std::uint32_t,std::uint32_t); //001FB8E8.
 static int GetRank(const ::port::driving::Memory&,std::uint32_t,int); //001FB710.
 static int GetRaceIndex(const ::port::driving::Memory&,std::uint32_t); //001FB698.
 static int GetRandomSeed(const ::port::driving::Memory&,std::uint32_t); //001FB7B0.
 static void NewRandomSeed(::port::driving::Memory&,std::uint32_t); //001FB7B8.
};
struct EventTournamentState {
 static void SetSelectedCar(::port::driving::Memory&,std::uint32_t,int,int,int); //001FBB98,W86.
 static void Reset(::port::driving::Memory&,std::uint32_t); //001FB988.
 static unsigned RacerRank(const ::port::driving::Memory&,std::uint32_t,int); //001FBA60.
 static unsigned UserRank(const ::port::driving::Memory&,std::uint32_t); //001FBAB8.
 static eEventStatus GetStatus(const ::port::driving::Memory&,std::uint32_t,std::uint32_t,int,int); //001FBB20.
 static int GetRaceIndex(const ::port::driving::Memory&,std::uint32_t,int); //001FB9F0.
 static int GetRandomSeed(const ::port::driving::Memory&,std::uint32_t); //001FBAE8.
 static void NewRandomSeed(::port::driving::Memory&,std::uint32_t); //001FBAF0.
};
struct EventResult {
    static void Reset(::port::driving::Memory&,std::uint32_t); //001FC648.
    int nRank=0;Timer tTime;int nReason=0;
    void Reset();
};
struct EventTimeState {
    static void Reset(::port::driving::Memory&,std::uint32_t); //001FB540, W78 raw source overload.
    static eEventStatus GetStatus(const ::port::driving::Memory&,std::uint32_t,std::uint32_t); //001FB560.
    static void Reset(::port::driving::init::ByteView);
    static eEventStatus GetStatus(::port::driving::init::ByteView,const EventGoal&);
};
// E-0098-RP04: explicit host storage and required persistent state binding; not PS2 ABI.
struct RaceEvent {
    // E-0110-F01. Original raw postrace/progress/retry consumers, not a save adapter.
    static bool IsMultiTrack(const ::port::driving::Memory&,std::uint32_t); //001FC5D8.
    static bool RaceWasAborted(const ::port::driving::Memory&,std::uint32_t); //001FC658.
    static int DriverNumberFromRacerIndex(const ::port::driving::Memory&,std::uint32_t,int); //001FC6D8.
    static bool DriverInTheRace(::port::driving::Memory&,std::uint32_t,int); //001FC7D0.
    static bool RacerInTheRace(::port::driving::Memory&,std::uint32_t,int); //001FC7A0.
    static int GetLastPlace(::port::driving::Memory&,std::uint32_t); //001FC870.
    static int GetFirstPlace(::port::driving::Memory&,std::uint32_t); //001FC900.
    static void PostResults(::port::driving::Memory&,std::uint32_t); //001FCA30.
    static void RestartRace(::port::driving::Memory&,std::uint32_t); //001FCBD0.
    static void RestartEvent(::port::driving::Memory&,std::uint32_t); //001FCC48,E-0101-RA02.
    static bool SetupNextRace(::port::driving::Memory&,std::uint32_t); //001FD1C0,E-0101-RA02.
    static int GetRaceCount(const ::port::driving::Memory&,std::uint32_t); //001FD190.
    static bool IsThereAnotherRace(::port::driving::Memory&,std::uint32_t); //001FD490.
    // E-0099-W78 original raw result-state lifecycle; no decoded profile cast.
    static std::uint32_t GetRankState(const ::port::driving::Memory&,std::uint32_t); //001FC990.
    static std::uint32_t GetTournamentState(const ::port::driving::Memory&,std::uint32_t); //001FC9B8.
    static std::uint32_t GetKoState(const ::port::driving::Memory&,std::uint32_t); //001FC9D8.
    static std::uint32_t GetTimeState(const ::port::driving::Memory&,std::uint32_t); //001FC9F8.
    static eEventStatus GetStatus(const ::port::driving::Memory&,std::uint32_t,std::uint32_t state); //001FD598 void* overload.
    static eEventStatus GetStatus(const ::port::driving::Memory&,std::uint32_t); //001FD6C8.
    static eEventStatus GetOldStatus(const ::port::driving::Memory&,std::uint32_t); //001FD6E8.
    static bool ResultShouldBeDiscarded(const ::port::driving::Memory&,std::uint32_t); //001FCCE8.
    static void MaybeDiscardResult(::port::driving::Memory&,std::uint32_t); //001FD0B8.
    static int GetRaceIndex(const ::port::driving::Memory&,std::uint32_t); //001FD118.
    static void ResetState(::port::driving::Memory&,std::uint32_t); //001FD708.
    // W56 raw original-memory bindings; decoded pState below is not reinterpreted.
    static int GetStateSize(const ::port::driving::Memory&,std::uint32_t address); //001FC078
    static void SavePreRaceState(::port::driving::Memory&,std::uint32_t address); //001FCB90
    static void ResetResults(::port::driving::Memory&,std::uint32_t); //001FC678.
    static int RacerIndexFromDriverNumber(const ::port::driving::Memory&,std::uint32_t,int); //001FC6B8,W70.
    static bool RacerEliminated(const ::port::driving::Memory&,std::uint32_t,int); //001FC808.
    static bool DriverEliminated(const ::port::driving::Memory&,std::uint32_t,int); //001FC840.
    static void RecordResults(::port::driving::Memory&,std::uint32_t event,std::uint32_t race,std::uint32_t racing_car,int rank); //001FC6F8.
    const EventTree* pTree=nullptr;
    ::port::driving::init::ByteView pState{nullptr,0};
    // Port binding of the original mutable points table00328830 for the decoded
    // GetRaceIndex overload. Raw original-memory overload reads it directly.
    std::function<std::uint8_t(unsigned)> port_points_at_00328830;
    std::array<std::uint8_t,16> aOldState{},aPrevState{};
    std::array<char,32> sName{},sRewardName{};
    std::int16_t nNumber=0,nType=0,nClass=0,nCars=0,nTracks=0,nRewards=0,nOpponents=0,nInitialPlayers=0;
    int bWillSuperBonusEventBeUnlockedIfThisEventIsWon=0;
    EventGoal Objective;
    std::array<std::uint32_t,20> aCars{};
    std::array<std::uint32_t,8> aTracks{};
    std::array<EventReward,8> aRewards{};
    std::array<std::uint32_t,7> aOpponents{};
    std::array<EventResult,8> aResults{};
    void ResetResults();
    void ResetState();
    int GetRaceIndex() const;
};
class EventTrack {
public:
    static void SetRace(::port::driving::Memory&,std::uint32_t,std::uint32_t); //001FA178 raw original GetTrackInfo.
    // Debug001FA178 E-0097-D03. packed EventTrack -> Debug RaceParameters0x310.
    // GetTrackInfo service returns original Point2Point field or null for a missing
    // original track record, which must leave Point2Point unchanged.
    static void SetRace(std::uint32_t,::port::driving::init::ByteView,
        const std::function<std::optional<int>(int)>& GetTrackInfo);
};
}
namespace port::race {
// Port asset adapter for Debug001FB138/001FB458. Decodes the original common
// chunk schema; does not bind Retail progress bytes to Debug pState pointers.
std::vector<hp2::debug::EventTree> parse_debug_event_tables(const std::vector<std::uint8_t>&);
}
