#include "game_core/indep/src/raceparameters.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/race/launch.hpp"
#include "port/runtime/race_launch_catalog.hpp"
#include "platform/ps2_asset_root.hpp"
#include "game_core/indep/inc/World.hpp"
#include <fstream>
#include <array>
#include <cstdio>
#include <stdexcept>
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void baseline(port::driving::Memory& memory){
    const auto address=memory.allocate(0x310);hp2::debug::RaceParameters race(memory,address);
    const auto before=memory.word(0x329060);race.InitWithDefaults({});
    require(memory.word(address)==11 && memory.word(address+8)==3 && memory.word(address+0x254)==1,"original baseline track/time/mode differs");
    require(memory.scalar(address+0x10)==1 && memory.scalar(address+0x14)==.5f && memory.word(address+0x34)==7,"original default traffic/grid differs");
    require(memory.word(address+0x304)==0x312288 && memory.word(address+0x308)==0,"original global DriverInfo table binding differs");
    require(memory.word(address+0x30c)==before%0xffffffffu && memory.word(0x329060)!=before,"default color seed must consume original shared RNG");
    for(unsigned i=0;i<0x150;++i)require(memory.byte(0x312288+i)==0,"original driver table must be cleared");
    require(memory.byte(address+0x28c+4)==1 && memory.word(address+0x28c+0x10)==0x355
        && memory.byte(address+0x28c+20)=='P' && memory.word(address+0x2bc+0x2c)==1,"both original player defaults missing");
    require(memory.byte(address+0x2ec+3)==7 && memory.byte(address+0x2ec+4)==8 && memory.byte(address+0x2ec+6)==1
        && memory.byte(address+0x44)==0,"display JumpCams must not silently enable race JumpCams before copying");
}
std::uint32_t next_seed(std::uint32_t old){const auto v=old^0x1d872b41u;const auto x=v^(v>>5);return x^v^(x<<27);}
void participants(port::driving::Memory& memory,std::uint32_t address){
    using namespace hp2::debug;
    RaceParameters race(memory,address);race.InitWithDefaults({});
    memory.word(address+0x20,1);memory.word(address+0x30,1);memory.word(address+0x34,0);
    memory.byte(address+0x3c,1);memory.word(address+0x25c,6);memory.word(address,13);
    memory.word(address+0x30c,0x12345678);
    std::array<CarTypeInfo,64> cars{};
    for(int i=0;i<64;++i)cars[i].Type=i;
    cars[0].CarClass=5;
    cars[1].UsageType=CAR_USAGE_TYPE_TRAFFIC;cars[1].GeometryFilename[0]='T';cars[1].CarClass=2;
    cars[1].NumAvailableSkinNumbers=1;cars[1].AvailableSkinNumbers[0]=4;cars[1].LocationProb[0]=1;
    cars[2].UsageType=CAR_USAGE_TYPE_COP;cars[2].GeometryFilename[0]='C';cars[2].CarClass=2;
    cars[3]=cars[2];cars[3].Type=3; // Same class: original first encountered cop wins.
    FrontendDatabase fred;fred.GameType=9;fred.FrontendPlayers[0].RideInfoByGameMode[9].Init(0,6,0,0);
    EventInfo event;event.nNumber=1;event.nType=5;
    port::race::ParticipantServices services{cars};
    services.character_address=[](int n){return n<0?0u:0x500000u+unsigned(n)*0x44;};
    services.character_is_male=[](int){return true;};
    services.track_cop_hash=[](int track)->std::optional<std::uint32_t>{require(track==13,"cop character lookup used supplied argument, not race track");return 123;};
    services.character_number_from_hash=[](std::uint32_t hash)->std::optional<int>{require(hash==123,"cop name hash mapping changed");return 9;};
    std::vector<int> skins;services.skin_info=[&](int,int skin,int fallback){require(fallback==1,"driver skin fallback differs");skins.push_back(skin);};
    fred.SetupCarsForRace(memory,address,event,services);
    require(memory.word(address+0x308)==13 && memory.word(address+0x34)==0 && memory.word(address+0x25c)==6,"first event must retain player, six traffic and six police, without AI racers");
    auto expected=0x12345678u;for(int i=0;i<1003;++i)expected=next_seed(expected);
    require(memory.word(0x329060)==expected,"traffic retries must preserve warm-up and overwritten-skin RNG draws");
    require(skins.size()==13 && skins[0]==6 && skins[1]==-1 && skins[7]==1,"original signed traffic skin sentinel or cop fixed skin differs");
    const auto player=race.GetDriverInfo(0);require(memory.signed_byte(player+3)==-2 && memory.byte(player+4)==1
        && memory.byte(player+19)==1 && fred.FrontendPlayers[0].RideInfoByGameMode[9].CanBeVertexDamaged==1,"player setup failed to preserve start/skill or mutate selected ride damage flag");
    for(int i=0;i<6;++i){const auto t=race.GetDriverInfo(i+1),cop=race.GetDriverInfo(i+7);
        require(memory.byte(t+1)==50+i && memory.word(t+8)==3 && memory.word(t+12)==1 && memory.byte(t+17)==255
            && memory.word(t+20)==0,"traffic DriverInfo differs");
        require(memory.byte(cop+1)==10+i && memory.word(cop+8)==4 && memory.word(cop+12)==2 && memory.byte(cop+2)==9
            && memory.byte(cop+4)==0 && memory.byte(cop+17)==1,"cop DriverInfo or first-match class tie differs");}
    // Callback probes assert original live-read order, independent of immutable production providers.
    fred.FrontendPlayers[0].RideInfoByGameMode[8].Init(0,7,0,0);
    services.character_is_male=[&](int){fred.GameType=8;return true;};
    cars[4]=cars[1];cars[4].Type=4;cars[4].LocationProb[0]=0;cars[4].LocationProb[1]=1;
    services.skin_info=[&](int,int skin,int){if(skin==-1 && memory.word(address+0x308)==1)memory.word(address,23);};
    services.track_cop_hash=[](int track)->std::optional<std::uint32_t>{require(track==23,"cop lookup cached old track");return 123;};
    fred.SetupCarsForRace(memory,address,event,services);
    require(memory.byte(race.GetDriverInfo(0)+17)==7,"selected ride must be resolved after original character selection");
    require(memory.word(race.GetDriverInfo(1)+12)==1 && memory.word(race.GetDriverInfo(2)+12)==4,
        "location must be resolved for every generation call, not cached across traffic attempts");
    fred.GameType=9;
    const auto before=memory.word(address+0x308);event.nNumber=2;
    require(!hp2::guard([&]{fred.SetupCarsForRace(memory,address,event,services);}).empty()
        && memory.word(address+0x308)==before,"unknown event branch must fail before mutation");
}
}
int main(){
    using namespace hp2::debug;
    port::driving::Memory memory;memory.map(0x312288,0x150);memory.map(0x328690,12);memory.map(0x329060,4);
    memory.map(0x3253fc,8);memory.word(0x3253fc,0x3a83126f);memory.word(0x325400,0x3a83126f);
    const char name[]="PLAYER";for(unsigned i=0;i<sizeof(name);++i)memory.byte(0x328690+i,name[i]);memory.word(0x329060,0x12345678);
    baseline(memory);
    const auto address=memory.allocate(0x310);hp2::debug::RaceParameters race(memory,address);
    port::race::DefaultSettings defaults;defaults.skip_frontend=true;defaults.pov_index=5;defaults.rear_view=0;
    defaults.speedo_mode=1;defaults.jump_camera=0;defaults.zone_camera=0;race.InitWithDefaults(defaults);
    require(memory.byte(address+0x28c+4)==5 && memory.byte(address+0x2ec)==0 && memory.byte(address+0x2ec+1)==1
        && memory.byte(address+0x2ec+6)==0,"SkipFE settings projection differs");
    defaults.skip_frontend=false;defaults.english_language=false;race.InitWithDefaults(defaults);
    require(memory.byte(address+0x2ec+1)==1,"non-English units default differs");
    memory.byte(address+0x28c+31,77);hp2::debug::cPlayerSettings::ResetToDefaults(memory,address+0x28c,defaults);
    require(memory.byte(address+0x28c+31)==77,"player reset must preserve trailing name bytes beyond NUL");
    const auto source=memory.allocate(24);for(unsigned i=0;i<24;++i)memory.byte(source+i,std::uint8_t(31+i));
    memory.word(source+12,41);memory.byte(source+17,6);memory.byte(source,0);memory.byte(source+1,0);memory.word(source+8,1);
    int calls=0;const port::race::SkinLookup lookup=[&](int type,int skin,int fallback){require(type==41 && skin==6 && fallback==1,"AddDriverInfo skin lookup arguments differ");++calls;};
    race.AddDriverInfo(source,lookup);require(calls==1 && memory.word(address+0x308)==1,"driver append count differs");
    for(unsigned i=0;i<24;++i)require(memory.byte(race.GetDriverInfo(0)+i)==memory.byte(source+i),"full driver record/padding/character pointer copy differs");
    memory.byte(source+1,1);memory.word(source+8,2);race.AddDriverInfo(source,lookup);
    memory.byte(source+1,50);memory.word(source+8,3);race.AddDriverInfo(source,lookup);
    memory.byte(source+1,10);memory.word(source+8,4);race.AddDriverInfo(source,lookup);
    memory.byte(source+1,255);memory.word(source+8,4);race.AddDriverInfo(source,lookup);
    require(race.GetDriverInfoByDriverNumber(-1)==race.GetDriverInfo(4) && race.GetDriverInfoByDriverNumber(9)==0,"driver-number search must use signed byte/first match");
    memory.word(address+0x30,2);memory.word(address+0x34,3);
    race.EliminateDriver(0,2);race.EliminateDriver(0,3);race.EliminateDriver(1,1);race.EliminateDriver(10,1);
    require(memory.word(address+0x30)==1 && memory.word(address+0x34)==2 && race.GetNumEliminated()==3,"elimination must decrement only player/AI once for positive ranks");
    race.ClearEliminatedDrivers();require(memory.word(address+0x30)==2 && memory.word(address+0x34)==3 && race.GetNumEliminated()==0,"clear eliminations must restore counts, including non-racer rank reset");
    race.EliminateDriver(50,255);require(race.GetNumEliminated()==0,"signed negative elimination rank must not count positive");race.ClearEliminatedDrivers();
    require(memory.byte(race.GetDriverInfo(2))==255,"clear must preserve original negative rank");
    memory.word(address+0x34,3);race.ResetStartingPositions();
    require(memory.byte(race.GetDriverInfo(0)+4)==4 && memory.byte(race.GetDriverInfo(1)+4)==5
        && memory.byte(race.GetDriverInfo(4)+4)==0 && memory.byte(race.GetDriverInfo(3)+4)==0,"starting positions must use signed driver number, not driver type");
    const auto count=memory.word(address+0x308);race.SetDriverInfo(source,13);
    require(memory.word(address+0x308)==count && calls==5,"SetDriverInfo must not append or look up skin");
    memory.word(address+0x308,14);race.AddDriverInfo(source,{});race.SetDriverInfo(0,14);
    require(memory.word(address+0x308)==14,"full DriverInfo table must be original no-op");
    require(!hp2::guard([&]{race.SetDriverInfo(source,-1);}).empty(),"negative host write must fail explicitly");
    memory.word(address+0x20,1);memory.word(address+0x2c,0);memory.word(address+0x274,0);require(race.IsFreeRun(),"zero-lap single race free run differs");
    memory.word(address+0x274,1);require(!race.IsFreeRun(),"BeACop must override free run");
    participants(memory,address);
    const auto image=port::driving::debug_image_path(".");if(!image.empty()){
        auto original=port::driving::read_debug_image(image);baseline(original);
        const auto root=hp2::platform::resolve_zzdata_root(".");
        if(root){
            const auto read=[](const std::string& path){std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Configured original launch asset unavailable");
                return std::vector<std::uint8_t>{std::istreambuf_iterator<char>(in),{}};};
            port::race::LaunchCatalog catalog(read(*root+"/GLOBAL/GLOBALB.BUN"),original);
            for(int i=0;i<64;++i)require(catalog.cars()[i].Type==i,"reviewed original car identity correspondence changed");
            const auto* track=catalog.track(13);require(track && track->number==13 && track->point_to_point==1 && !catalog.track(54),"reviewed Retail track adapter/index/cleared row differs");
            const auto character=catalog.character_number_from_hash(track->cop_name_hash);
            require(character && catalog.character_address(*character) && !catalog.character_address(-1),"original track cop character binding missing");
            const std::uint32_t parameters=0x311f78; // Original TheRaceParameters in this SHA-bound image.
            RaceParameters real(original,parameters);real.InitWithDefaults({});
            auto trees=port::race::parse_debug_event_tables(read(*root+"/FRONTEND/FRONTB.BUN"));const auto& first=trees.at(1).pEvents.at(0);
            FrontendDatabase fred;fred.GameType=9;fred.MakeEventCarsAvailable(trees[1],first,catalog.cars(),0);
            const int selected=(*first.GetCar(trees[1],0)>>16)&255;fred.FrontendPlayers[0].AssignCarOfType(9,selected,0,1,catalog.cars());
            std::array<std::uint8_t,4> saved_time{};port::driving::init::ByteView saved(saved_time);saved.word(0,123456);
            RaceEvent active;active.aResults[7].nRank=8;active.aPrevState.fill(77);
            std::vector<int> text_calls;
            port::race::EventLaunchServices bindings{fred,
                [&](const std::string& key){require(key=="fixture car","original reward name translation changed");text_calls.push_back(1);return key;},
                [&](int id){require(id==first.mDescriptionString,"wrong original description ID");text_calls.push_back(2);return std::string("fixture template");},
                [&](const auto& tree,const auto& event,const std::string& text){require(&tree==&trees[1] && &event==&first && text=="fixture template","original description call bindings changed");text_calls.push_back(3);return std::string("fixture objective");}};
            bindings.car_full_name=[&](int car){require(car==41,"first reward car identity differs");text_calls.push_back(4);return std::string("fixture car");};
            const auto original_seed=original.word(parameters+0x30c);
            trees[1].SetRace(original.view(parameters,0x310),first,active,saved,bindings);
            require(text_calls==std::vector<int>({4,1,2,3}) && fred.nCurrentEvent==1 && trees[1].pCurrentEvent==&first,"event binding/localization order differs");
            require(saved.word(0)==123456 && active.aOldState[0]==64 && active.aOldState[1]==226 && active.aOldState[2]==1
                && active.aResults[7].nRank==0 && active.aPrevState[0]==77,"event must capture prior profile state before startup reset; results reset must not overwrite previous-state snapshot");
            require(original.word(parameters+0x20)==1 && original.word(parameters+0x24)==1200000 && original.word(parameters+0x28)==1
                && original.word(parameters+0x25c)==6 && original.word(parameters+0x268)==18,"real delivery objective/police settings differ");
            require(original.word(parameters+0x2f8)==1 && original.word(parameters+0x30c)==original_seed && original.word(parameters+0x44)==0
                && original.word(parameters+0x278)==1,"event setup must retain prior/default boost/RNG/camera/arrest settings");
            require(!first.WillSuperBonusEventBeUnlockedIfThisEventIsWon() && active.GetRaceIndex()==0,"first delivery must not query special-event prediction or multitrack state");
            active.ResetState();require(saved.word(0)==0 && active.aOldState[0]==64,"startup reset must preserve event-entry old snapshot");
            auto providers=catalog.services(0);port::race::StartServices start{providers};
            start.track_point_to_point=[&](int n)->std::optional<int>{const auto* t=catalog.track(n);return t?std::optional<int>(t->point_to_point):std::nullopt;};
            std::vector<int> startup_calls;int flow_requests=0;
            start.copy_profile_settings=[&](auto& storage,std::uint32_t target){
                require(target==parameters && storage.word(parameters+0x308)==13,"profile settings must follow participant construction");startup_calls.push_back(1);
                // Explicit profile service fixture. Production private-profile/record adaptation remains required.
                storage.byte(target+0x28c+2,fred.FrontendPlayers[0].mPlayerSettings.DrivingStyle);
                storage.byte(target+0x28c+3,fred.FrontendPlayers[0].mPlayerSettings.TransmissionType);
            };
            start.set_record_times=[&](int player){require(player==0 && startup_calls.back()==1,"record lookup must follow profile settings for actual players only");startup_calls.push_back(2);
                original.word(parameters+0x28c+0x20,1234);original.word(parameters+0x28c+0x24,5678);};
            start.change_game_flow=[&](auto flow,auto continuation){
                require(flow==(start.current_flow==port::race::Flow::in_frontend?port::race::Flow::unloading_frontend:port::race::Flow::loading_track_common)
                    && continuation==port::race::Flow::none && original.word(parameters+0x258)==1,"original StartRace GameFlow target/continuation differs");
                require(startup_calls.back()==2 && fred.bRaceWasAborted==1,"GameFlow must follow records and precede aborted flag clear");
                startup_calls.push_back(3);++flow_requests;
            };
            fred.bRaceWasAborted=1;fred.StartRace(original,parameters,active,start);
            require(startup_calls==std::vector<int>({1,2,3}) && flow_requests==1 && fred.bRaceWasAborted==0,"first-event native startup handoff order differs");
            const auto probe=original.allocate(16);start.current_flow=port::race::Flow::none;
            for(int retry=0;retry<30;++retry){startup_calls.clear();fred.bRaceWasAborted=1;fred.StartRace(original,parameters,active,start);
                require(original.word(parameters+0x308)==13 && active.aOldState[0]==64,"startup reentry duplicated participants or overwrote prior progress snapshot");}
            // Existing logical allocator retains a16-byte unmapped guard after each allocation.
            require(original.allocate(16)==probe+32 && flow_requests==31,"temporary native startup driver storage grew across reentry");
            {
                // Shared-storage integration fixture: accessed Car headers, NOT real Car/World construction.
                const auto world_address=original.allocate(0x1730);World world(original,world_address);
                original.word(world_address+8,world_address+8);original.word(world_address+12,world_address+8);
                auto previous=world_address;
                for(int i=0;i<13;++i){const auto car=original.allocate(0x18),driver=real.GetDriverInfo(i);
                    original.word(previous,car);original.word(car+4,previous);previous=car;
                    original.word(car+0x14,driver);original.half(car+0xa,static_cast<std::uint16_t>(original.signed_byte(driver+1)));
                }
                original.word(previous,world_address);original.word(world_address+4,previous);world.BuildCarTable();
                require(original.word(world_address+0x1c)==13 && original.word(world_address+0x20)==1
                    && original.word(world_address+0x24)==1 && original.word(world_address+0x28)==7,
                    "real first-event driver records must partition into one racer, six police and six traffic");
                require(world.GetCarByNumber(0) && world.GetCarByNumber(15) && world.GetCarByNumber(55)
                    && world.GetCarByNumber(100)==0,"World lookup lost original participant identities");
            }
            require(original.word(parameters+0x308)==13 && original.word(real.GetDriverInfo(0)+12)==41,"real first event participants missing or selected car replaced");
            for(int i=0;i<6;++i){
                const auto traffic_type=original.word(real.GetDriverInfo(1+i)+12);
                const auto cop_type=original.word(real.GetDriverInfo(7+i)+12);
                require(traffic_type<64 && catalog.cars()[traffic_type].UsageType==CAR_USAGE_TYPE_TRAFFIC
                    && cop_type<64 && catalog.cars()[cop_type].UsageType==CAR_USAGE_TYPE_COP,"real participant car identities do not match original usage categories");
            }
            for(int i=0;i<6;++i)require(original.byte(real.GetDriverInfo(7+i)+2)==*character,"actual track cop character must reach every police driver");
            require(catalog.skin_info(41,-1,1)!=0 && catalog.skin_info(41,-1,0)==0,"original GetSkinInfo first-row fallback differs");
            saved.word(0,0);require(EventTimeState::GetStatus(saved,active.Objective)==eEVENT_UNTRIED,"zero event time must be untried");
            saved.word(0,0x7fffffff);require(EventTimeState::GetStatus(saved,active.Objective)==eEVENT_UNTRIED,"max event time sentinel must be untried");
            saved.word(0,active.Objective.nData);require(EventTimeState::GetStatus(saved,active.Objective)==eEVENT_PASSED,"time goal equality must pass");
            saved.word(0,active.Objective.nData+1);require(EventTimeState::GetStatus(saved,active.Objective)==eEVENT_FAILED,"time beyond goal must fail");
            saved.word(0,0x80000000);require(EventTimeState::GetStatus(saved,active.Objective)==eEVENT_PASSED,"original event time comparison must be signed");
            std::puts("PASS original first event car/track/character inputs and thirteen participant records");
        }
        std::puts("PASS original SHA-bound Debug race baseline and driver operations");
    }else std::puts("PASS synthetic race baseline/drivers; SKIP unconfigured original Debug image");
}
