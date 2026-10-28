#include "port/runtime/frontend_runtime.hpp" // Exercise entry header after hp2::port declarations.
#include "game_core/port/frontend/event_entry.hpp"
#include "game_core/indep/inc/EventTree.hpp"
#include "game_core/indep/src/eventtree.hpp"
#include "game_core/port/not_implemented.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
namespace {void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}}

namespace {
// Host callbacks exercise orchestration only, never a production page/race claim.
void check_event_entry(){
    using namespace hp2::debug;
    std::array<std::uint8_t,0x310> bytes{};
    port::driving::init::ByteView race(bytes);FrontendDatabase fred;
    std::array<CarTypeInfo,64> cars{};EventTree tree;tree.nType=1;
    tree.pCars={0x06020000};EventInfo event;event.nNumber=7;event.nType=5;event.nCars=1;
    tTreeDetails details{&tree,&event};int mode=-5,status_calls=0;
    eEventStatus status=eEVENT_UNTRIED;std::vector<std::string> calls;
    port::selection::EventEntryServices services{fred,race,mode,cars};
    services.get_status=[&](const EventInfo& selected){
        require(&selected==&event,"entry status used a different event");++status_calls;return status;};
    services.set_race=[&](EventTree& selected_tree,const EventInfo& selected){
        require(&selected_tree==&tree && &selected==&event,"entry lost tree/event identity");
        require(mode==1 && race.word(0x254)==1 && fred.nCurrentEvent==7,
            "entry must set both modes/current event before SetRace");
        calls.push_back("race");};
    services.build_unlock_list=[&]{require(calls==std::vector<std::string>{"race"},"unlock list must follow SetRace");calls.push_back("unlocks");};
    services.change_screen=[&](const char* name,bool push){
        require(std::string(name)=="Select Car" && push,"entry must change to Select Car with history");
        require(fred.FrontendPlayers[0].aCarStatus[2].nStatus==eCAR_UNLOCKED_FULLY_VISIBLE
            && fred.FrontendPlayers[1].aCarStatus[2].aSkins[0]==6,"entry must apply both players' car restrictions before transition");
        calls.push_back("select");};
    services.push_screen=[&](const char* name){require(std::string(name)=="View Event Tree","continuation history differs");calls.push_back("push");};
    services.continue_event=[&]{calls.push_back("continue");};
    ScreenCallbackData data;data.Noise=99;data.Param=123;data.Port=1;
    details.HandleMessages(SCREEN_START_BUTTON,data,services);
    require(status_calls==0 && calls.empty() && data.Noise==99 && mode==-5,
        "START must not alias SELECT in Debug tree details");
    for(int msg=-2;msg<=24;++msg){
        if(msg==MENUITEM_SELECT)continue;data.Noise=99;
        details.HandleMessages(static_cast<ScreenMessage>(msg),data,services);
        require(data.Noise==((msg>=6 && msg<10)?0:99),"details non-select sound mutation differs");
    }
    require(status_calls==0 && calls.empty(),"non-select message attempted event entry");
    for(int st=-1;st<=0;++st){status=static_cast<eEventStatus>(st);data.Noise=99;
        details.HandleMessages(MENUITEM_SELECT,data,services);
        require(data.Noise==0x17 && calls.empty() && mode==-5,"locked event mutated entry state");}
    for(int tree_type:{0,1})for(int event_type:{0,1,2,3,4,5,6})for(int port:{0,1}){
        tree.nType=tree_type;event.nType=event_type;data.Port=port;
        mode=-5;race.word(0x254,0);fred.GameType=-8;calls.clear();status=eEVENT_UNTRIED;
        const int before=status_calls;details.HandleMessages(MENUITEM_SELECT,data,services);
        require(data.Noise==0x16 && data.Param==123 && fred.GameType==(tree_type==0?8:9)
            && calls==std::vector<std::string>{"race","unlocks","select"},"ordinary event entry route/order differs");
        require(status_calls-before==((event_type==3 || event_type==4)?2:1),"entry must query multitrack status again");
    }
    for(int event_type:{3,4})for(int port:{0,1}){
        event.nType=event_type;data.Port=port;status=eEVENT_INPROGRESS;
        fred.GameType=123;calls.clear();details.HandleMessages(MENUITEM_SELECT,data,services);
        const std::vector<std::string> expected=port==0?std::vector<std::string>{"race","unlocks","push","continue"}
            :std::vector<std::string>{"race","unlocks"};
        require(calls==expected && fred.GameType==123,"in-progress multitrack must bypass car availability/mode change");
    }
    // Status is a live read; it can change after the first selection gate.
    int queries=0;services.get_status=[&](const EventInfo&){return ++queries==1?eEVENT_UNTRIED:eEVENT_INPROGRESS;};
    data.Port=0;calls.clear();details.HandleMessages(MENUITEM_SELECT,data,services);
    require(queries==2 && calls.back()=="continue","multitrack status was incorrectly cached");
    event.nType=5;services.set_race={};const int old_mode=mode;
    require(!hp2::guard([&]{details.StartEvent(data,services);}).empty() && mode==old_mode,
        "missing entry services must report a gap");
    std::puts("PASS Debug tree details entry ordering, locked/select/START sounds, both ports and multitrack continuation (host fixtures)");
}
}

int main(){
    check_event_entry();
    hp2::debug::EventTree tree;tree.pCars={0x12345678};tree.pTracks={0x87654321};
    tree.pGoals={{2,17,1234}};tree.pRewards.resize(1);tree.pOpponents={0xabcdef};
    hp2::debug::EventInfo event;event.nCars=event.nTracks=event.nGoals=event.nRewards=event.nOpponents=1;
    require(event.GetCar(tree,-1)==nullptr && event.GetCar(tree,1)==nullptr,"Debug car getter must return null at caller bounds");
    require(*event.GetCar(tree,0)==0x12345678 && *event.GetTrack(tree,0)==0x87654321,"indexed car/track lookup differs");
    require(event.GetGoal(tree,0)->nEvent==17 && event.GetObjective(tree)->nData==1234,"objective must use nObjective, independent of requirement array");
    event.nType=3;require(event.GetStateSize()==12,"Debug race-KO state must be12, not Retail16");
    require(hp2::frontend::EventSaveSize_00200DE8(3)==16,"Retail state-size behavior changed during Debug recovery");
    event.aCars[0]=-1;require(!hp2::guard([&]{event.GetCar(tree,0);}).empty(),"negative populated table index must not silently become a valid car");
    std::array<std::uint8_t,0x310> image{};port::driving::init::ByteView race(image);race.word(0x28,9);race.word(0x30,0xfeed);
    // Maximum bitfields exercise independent shifts, including ignored alternatebit8.
    std::uint32_t packed=11u|(1u<<8)|(1u<<9)|(2u<<10)|(3u<<12)|(8u<<14)|(75u<<18)|(25u<<25);
    hp2::debug::EventTrack::SetRace(packed,race,[](int id)->std::optional<int>{require(id==11,"track lookup used array ordinal instead of original number");return 1;});
    require(race.word(0)==11 && race.word(4)==2 && race.word(8)==3 && race.word(12)==1 && race.word(0x2c)==8,"packed track settings differ");
    require(std::abs(race.scalar(16)-0.75f)<0.000001f && std::abs(race.scalar(20)-0.25f)<0.000001f,"traffic percentages use wrong original units");
    require(race.word(0x28)==1 && race.word(0x30)==0xfeed,"track mapping did not preserve unrelated race parameters");
    race.word(0x28,7);hp2::debug::EventTrack::SetRace(0xff,race,[](int)->std::optional<int>{return std::nullopt;});
    require(race.word(0x28)==7,"missing original TrackInfo must preserve existing Point2Point field");
    auto malformed=event;malformed.nCars=0;require(malformed.GetCar(tree,0)==nullptr,"empty count bound differs");
    const auto root=hp2::platform::resolve_zzdata_root(".");
    if(!root){std::puts("PASS Debug event getters/track mapping; SKIP original assets");return 0;}
    std::ifstream stream(*root+"/FRONTEND/FRONTB.BUN",std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(stream),{}};
    const auto trees=port::race::parse_debug_event_tables(bytes);require(trees.size()==2,"original two event tables missing");
    const auto retail=hp2::frontend::parse_event_trees(bytes);
    require(trees[1].sName=="Hot Pursuit" && trees[1].nType==1 && trees[1].pEvents.size()==32,"full Hot Pursuit32-node inventory missing");
    std::array<int,7> counts{};
    for(std::size_t ti=0;ti<trees.size();++ti){const auto& t=trees[ti];
        for(std::size_t i=0;i<t.pEvents.size();++i){const auto& e=t.pEvents[i];
            require(e.nNumber==static_cast<int>(i+1),"original event numbering differs");
            require(e.GetObjective(t)!=nullptr,"event objective not bound");
            for(int n=0;n<e.nCars;++n)require(e.GetCar(t,n)!=nullptr,"event car restriction reference invalid");
            for(int n=0;n<e.nTracks;++n)require(e.GetTrack(t,n)!=nullptr,"event track reference invalid");
            for(int n=0;n<e.nGoals;++n)require(e.GetGoal(t,n)!=nullptr,"event prerequisite reference invalid");
            for(int n=0;n<e.nRewards;++n)require(e.GetReward(t,n)!=nullptr,"event reward reference invalid");
            for(int n=0;n<e.nOpponents;++n)require(e.GetOpponent(t,n)!=nullptr,"event opponent reference invalid");
            require(e.nCars==retail[ti].records[i].nCars && e.nTracks==retail[ti].records[i].nTracks,"common asset field correspondence failed");
            if(ti==1){require(e.nType>=0 && e.nType<7,"unexpected Hot Pursuit event type");++counts[e.nType];}
        }
    }
    require(counts==std::array<int,7>{8,2,4,5,4,4,5},"Hot Pursuit event-type coverage changed");
    require(trees[1].pEvents[0].bBarricadeSmokeables==18,"noncanonical original integer flags must not be silently bool-normalized");
    std::puts("PASS Debug event semantics, six table references, track mapping,64 asset records and Retail/Debug race-KO incompatibility");
}
