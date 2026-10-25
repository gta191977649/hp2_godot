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
int main(){
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
