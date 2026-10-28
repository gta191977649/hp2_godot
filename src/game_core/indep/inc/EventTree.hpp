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
struct EventGoal { std::uint16_t nType=0,nEvent=0;std::uint32_t nData=0; };
// Reward union fields are intentionally opaque here, pending their consumers.
struct EventReward { std::array<std::uint8_t,8> Field_0x00{}; };
struct EventInfo {
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
enum eEventStatus {eEVENT_LOCKED=0,eEVENT_UNTRIED=1,eEVENT_INPROGRESS=2,eEVENT_FAILED=3,eEVENT_PASSED=4,eEVENT_MAX_STATUS=5};
struct EventResult {
    int nRank=0;Timer tTime;int nReason=0;
    void Reset();
};
struct EventTimeState {
    static void Reset(::port::driving::init::ByteView);
    static eEventStatus GetStatus(::port::driving::init::ByteView,const EventGoal&);
};
// E-0098-RP04: explicit host storage and required persistent state binding; not PS2 ABI.
struct RaceEvent {
    const EventTree* pTree=nullptr;
    ::port::driving::init::ByteView pState{nullptr,0};
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
