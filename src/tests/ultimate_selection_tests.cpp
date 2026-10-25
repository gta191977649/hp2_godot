#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/port/not_implemented.hpp"
#include "platform/ps2_asset_root.hpp"
#include "game_core/port/assets/fe_language.hpp"
#include "port/assets/event_name_catalog.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
namespace {
void require(bool result,const char* message){if(!result)throw std::runtime_error(message);}
std::vector<std::uint8_t> read(const std::string& path){
    std::ifstream stream(path,std::ios::binary);
    if(!stream)throw std::runtime_error("Original asset could not be opened");
    return {std::istreambuf_iterator<char>(stream),{}};
}
}
int main(){
    using namespace hp2::debug;
    std::array<CarTypeInfo,64> cars{};
    for(int i=0;i<64;++i){
        auto& c=cars[i];c.Type=i;c.NumAvailableSkinNumbers=3;c.AvailableSkinNumbers={2,4,6};
        c.DefaultSkinNumber=4;std::snprintf(c.CarTypeName.data(),32,"FIXTURE%d",i);
        c.DashGeometryFilename[0]='D';c.CarClass=3;
    }
    cars[3].UsageType=CAR_USAGE_TYPE_COP;
    require(cars[3].GetNumAvailableSkins()==1 && cars[3].GetAvailableSkin(9,0)==1
        && cars[3].GetAvailableSkin(0,1)==2,"police skin must follow original language branch");
    require(cars[0].GetAvailableSkin(4,0)==4,"normal skin modulo differs");
    FrontendDatabase fred;fred.GameType=9;
    auto& player=fred.FrontendPlayers[0];
    require(&player.GetCarStatus(7)==&player.aCarStatus[7],"original status getter changed car identity");
    require(!hp2::guard([&]{player.GetCarStatus(64);}).empty(),"invalid status identity was silently accepted");
    player.SetCarAvailability(eCAR_UNLOCKED_FULLY_VISIBLE,0,-1,true,cars,0);
    player.FlushAvailableCars();
    require(player.aCarStatus[0].nSkins==3 && player.aCarStatus[0].bDashAvailable
        && player.aCarStatus[0].nStatus==eCAR_LOCKED_INVISIBLE,"flush must preserve skin/dashboard fields");
    EventTree tree;tree.pCars={std::uint32_t(1u<<16)|0x40000000u,std::uint32_t(2u<<16)|0x06000000u};
    EventInfo event;event.nCars=2;event.aCars[1]=1;
    fred.MakeEventCarsAvailable(tree,event,cars,0);
    for(auto& p:fred.FrontendPlayers){
        require(p.aCarStatus[0].nStatus==eCAR_LOCKED_INVISIBLE,"event restriction leaked old available car");
        require(p.aCarStatus[1].nStatus==eCAR_UNLOCKED_FULLY_VISIBLE && p.aCarStatus[1].nSkins==3
            && p.aCarStatus[1].bDashAvailable,"all-skins event entry not applied to both players");
        require(p.aCarStatus[2].nSkins==1 && p.aCarStatus[2].aSkins[0]==6
            && !p.aCarStatus[2].bDashAvailable,"explicit-skin event entry differs");
    }
    player.AssignCarOfType(9,1,99,1,cars);
    require(player.GetSelectedRideInfo(9).SkinNumber==2 && player.GetSelectedRideInfo(9).HasDash==1,
        "ride selection must substitute first allowed skin and available dashboard");
    player.AssignCarOfType(9,1,4,0,cars);
    const port::selection::ListServices services{cars,{},[](int){return std::string("same fixture name");},false};
    VisibleCarSet list(player,9,false,services);
    require(list.TotalNumCars==2 && list.NumberUnlocked==2,"event visible count differs");
    {
        CarSelector ui{list};SelectCarCameraMover camera;ui.pSelectCarCameraMover=&camera;
        require(ui.FadeCarOverlayAlpha==130,"original selector starts behind loading overlay");
        std::vector<std::string> calls;
        std::vector<std::uint32_t> help;
        const auto set_help=[&](std::uint32_t address){help.push_back(address);};
        for(int game:{0,7,8,9})for(int players:{1,2,3}){
            fred.GameType=game;fred.NumPlayerCars=players;ui.DesiredSelectorMode=0;help.clear();
            ui.AdjustHelpMessage(fred,set_help);
            const std::uint32_t expected=game==8?0x311cb0:game==9?0x311cf8:
                game==7?(players==1?0x311c28:0x311c40):(players==1?0:0x311bf8);
            if(players==3 && game!=8 && game!=9)require(help.empty(),"unsupported player count must preserve help override");
            else require(help==std::vector<std::uint32_t>{expected},"choose help table differs");
            ui.DesiredSelectorMode=1;help.clear();ui.AdjustHelpMessage(fred,set_help);
            require(help==std::vector<std::uint32_t>{game==8?0x311e18u:game==9?0x311e28u:0x311c58u},"orbit help table differs");
        }
        fred.GameType=9;fred.NumPlayerCars=1;ui.DesiredSelectorMode=0;

        port::selection::OrbitServices orbit{cars};orbit.real_time_frames=123;
        orbit.camera.sound_enabled=true;
        orbit.camera.audio=[&](const auto& a){calls.emplace_back(a.sample);};
        orbit.adjust_help=[&]{calls.emplace_back("help");};
        orbit.car_description=[](int type){return type+100;};
        orbit.speech_event=[&](int event,const auto& args){require(event==0x92 && args==std::vector<int>({0,101}),"orbit speech original event/arguments differ");calls.emplace_back("speech");};
        orbit.speech_fade=[&](float v){require(v==1,"choose speech fade differs");calls.emplace_back("fade");};
        orbit.help_active=true;ui.JoyHandler(0,33,1,orbit);require(calls.empty(),"help must block orbit entry");
        orbit.help_active=false;ui.FadeCarOverlayAlpha=1;ui.JoyHandler(0,33,1,orbit);require(calls.empty(),"loading overlay must block orbit entry");
        ui.FadeCarOverlayAlpha=0;ui.JoyHandler(0,33,1,orbit);
        require(calls==std::vector<std::string>({"speech","help","MenuCarDisplayActive","MenuCarDisplayOn"})
            && ui.DesiredSelectorMode==1 && ui.OrbitActiveTime==123 && !ui.ManualOrbitEngaged,"orbit entry ordering/state differs");
        calls.clear();ui.JoyHandler(0,33,1,orbit);
        require(calls==std::vector<std::string>({"help","MenuCarDisplayOff","MenuCarDisplayActive","fade"}) && ui.DesiredSelectorMode==0,"choose entry order differs");
        ui.JoyHandler(9,35,1,orbit);require(camera.DesiredHOrbitSpeed==-9000 && ui.ManualOrbitEngaged,"digital orbit left differs");
        ui.JoyHandler(9,36,0,orbit);require(camera.DesiredHOrbitSpeed==0,"digital release must zero speed regardless other held input");
        ui.JoyHandler(0,41,255,orbit);require(camera.DesiredHOrbitSpeed==8929.6875f,"analog horizontal original scale differs");
        ui.JoyHandler(0,42,0,orbit);require(camera.DesiredVOrbitSpeed==10000,"analog vertical scale differs");
        const auto radius=camera.DesiredRadiusSpeed;ui.JoyHandler(0,43,255,orbit);require(camera.DesiredRadiusSpeed==radius,"unhandled original zoom event changed radius");
        ui.SelectorState=SS_READY;ui.JoyHandler(0,35,1,orbit);require(camera.DesiredHOrbitSpeed==8929.6875f,"ready selector accepted orbit input");
        ui.SelectorState=SS_SELECT;orbit.version_e3=true;ui.JoyHandler(0,34,1,orbit);require(!ui.AllCarsUnlocked,"E3 unlock must be ignored");
        orbit.version_e3=false;ui.JoyHandler(0,34,0,orbit);require(!ui.AllCarsUnlocked,"unlock release changed state");
        ui.JoyHandler(0,34,1,orbit);require(ui.AllCarsUnlocked,"original unlock press not applied");
    }
    auto* selected=list.pCurrentVisibleCar;
    require(selected->eCarType==1 && selected->SkinIndex==1,"per-game car/skin selection was not restored");
    require(list.VisibleCarList[0]->eCarType==1 && list.VisibleCarList[1]->eCarType==2,"equal sort ties lost insertion order");
    require(list.GetPrevCar(selected)==selected,"first car must not wrap");
    auto* last=list.GetNextCar(selected);require(last->eCarType==2 && list.GetNextCar(last)==last,"last car must not wrap");
    selected->SetCurrentSkinNumber(99);require(selected->SkinIndex==1,"missing skin reset existing skin index");
    selected->SetCurrentSkinNumber(6);require(selected->NextSkin()==0 && selected->PrevSkin()==2,"skin boundary wrapping differs");
    // A sort that reorders ownership must retain the selected node's identity.
    const port::selection::ListServices reverse_names{cars,{},[](int id){return id==1?std::string("Z"):std::string("A");},false};
    list.SortVisibleCarList(reverse_names);
    require(list.VisibleCarList[0]->eCarType==2 && list.pCurrentVisibleCar==selected && selected->CarIndex==1,
        "sort invalidated current identity or did not assign final indices");
    CarSelector selector{list};SelectCarScreen screen;screen.NumCarSelectors=1;screen.CarSelectors[0]=&selector;
    port::selection::Popup popup;int popups=0,launches=0;
    const port::selection::SelectorServices selector_services{
        [&](const auto& p){popup=p;return ++popups;},
        [&]{if(!screen.AreAllSelectorsReady())return false;screen.SetSelectedCars(fred,cars);++launches;return true;}};
    player.mPlayerSettings.TransmissionType=2;player.mPlayerSettings.DrivingStyle=1;
    require(selector.TransmissionType==TT_AUTO && selector.HandlingMode==HM_CLASSIC,"selector incorrectly inherited profile settings");
    selector.ScreenCallback(1,0,0,selector_services);require(popups==0,"unrelated menu item opened selector popup");
    selected->CarUnlocked=false;selector.ScreenCallback(1,0,0x52e039f3,selector_services);
    require(popups==0,"locked car was confirmed");selected->CarUnlocked=true;
    selector.ScreenCallback(1,0,0x52e039f3,selector_services);
    require(selector.SelectorState==SS_TRANSMISSION && popup.n_buttons==3 && popup.button_params[2]==3
        && popup.screen_hash==0x41c2fdc3 && popup.message_id==49000,"transmission popup differs");
    selector.ScreenCallback(21,3,0,selector_services);
    require(launches==0 && selector.TransmissionType==TT_SPORT && selector.SelectorState==SS_HANDLING
        && popup.message_id==0xbf90 && popup.n_buttons==2,"single-player skipped required handling choice");
    selector.ScreenCallback(21,0,0,selector_services);
    require(selector.SelectorState==SS_TRANSMISSION && popup.start_button==2,"popup cancellation discarded selected transmission");
    selector.ScreenCallback(21,3,0,selector_services);selector.ScreenCallback(21,5,0,selector_services);
    require(launches==1 && selector.SelectorState==SS_HANDLING && player.mPlayerSettings.TransmissionType==2
        && player.mPlayerSettings.DrivingStyle==1 && player.GetSelectedRideInfo(9).Type==1,
        "readiness-before-advance or selected-car commit differs");
    const auto before=selector.SelectorState;selector.ScreenCallback(21,6,0,selector_services);
    require(selector.SelectorState==before && launches==1,"programmatic dismiss changed selector state");
    selector.IsTwoPlayerScreen=1;
    require(!screen.AreAllSelectorsReady(),"two-player screen must wait for SS_READY");
    selector.AdvanceState(selector_services);
    require(screen.AreAllSelectorsReady() && popup.n_buttons==0 && popup.button_params[0]==7,"two-player waiting popup differs");
    require(!hp2::guard([&]{selector.ScreenCallback(18,0,0,selector_services);}).empty(),"untranslated selector Tick must fail explicitly");
    CarSelector reentered{list};
    require(reentered.SelectorState==SS_SELECT && reentered.TransmissionType==TT_AUTO
        && reentered.HandlingMode==HM_CLASSIC,"reentry did not restore original selector defaults");
    reentered.DesiredSelectorMode=1;const auto saved_popups=popups;
    reentered.ScreenCallback(1,0,0x52e039f3,selector_services);
    require(popups==saved_popups,"orbit confirm incorrectly advanced selection");
    // Sorting unlocked, cop, performance and class fields is exercised independently of names.
    player.SetCarAvailability(eCAR_LOCKED_CLOAKED,3,-1,false,cars,0);
    const port::selection::ListServices locked_services{cars,[](int,bool){return false;},services.full_name,false};
    VisibleCarSet locked(player,9,false,locked_services);
    require(locked.TotalNumCars==3 && locked.NumberUnlocked==2 && locked.VisibleCarList.back()->eCarType==3,
        "locked car visibility/order or police unlock branch differs");
    require(!hp2::guard([&]{VisibleCarSet missing(player,9,false,services);}).empty(),"missing original unlock service was guessed");
    auto keyed_cars=cars;FrontendPlayerInfo keyed_player;
    for(int i=0;i<5;++i){keyed_cars[i].HighPerformance=0;keyed_cars[i].CarClass=5;
        keyed_cars[i].UsageType=CAR_USAGE_TYPE_RACING;
        keyed_player.SetCarAvailability(i==4?eCAR_LOCKED_FULLY_VISIBLE:eCAR_UNLOCKED_FULLY_VISIBLE,
            i,-1,false,keyed_cars,0);}
    keyed_cars[0].CarClass=1;keyed_cars[2].HighPerformance=1;keyed_cars[3].UsageType=CAR_USAGE_TYPE_COP;
    const port::selection::ListServices keyed_services{keyed_cars,[](int,bool){return false;},services.full_name,false};
    VisibleCarSet keyed(keyed_player,9,false,keyed_services);
    const std::array<int,5> key_order{1,0,2,3,4};
    for(std::size_t i=0;i<key_order.size();++i)require(keyed.VisibleCarList[i]->eCarType==key_order[i],
        "unlock, COP, performance or descending class sort priority changed");
    const auto root=hp2::platform::resolve_zzdata_root(".");
    if(!root){std::puts("PASS Debug selection state; SKIP original assets");return 0;}
    const auto actual_cars=port::selection::decode_car_info(read(*root+"/GLOBAL/GLOBALB.BUN"));
    hp2::port::assets::FeLanguage english(read(*root+"/LANGUAGES/FE_ENGLISH.BIN"));
    const auto translated=[&](int id){std::string value;
        if(!english.lookup(id,value))throw std::runtime_error("Original car language ID unavailable in Retail assets: "+std::to_string(id));
        return value;};
    for(int type=0;type<64;++type){
        const auto& info=FindCarLanguageInfo(type);
        require(info.LanguageSpecFullName==static_cast<int>(port::assets::EventNameCatalog::car_name_id(type)),
            "reviewed Debug/Retail full-name ID correspondence changed");
        require(!actual_cars[type].GetFullName(translated).empty(),"full-name translation binding missing");
        // Debug001953B0 uses localized acceleration/BHP but computes displayed
        // top speed numerically; not every unused LanguageSpecTopSpeed ID exists.
        if(actual_cars[type].UsageType!=CAR_USAGE_TYPE_COP)
            require(!translated(info.LanguageSpec0_60).empty() && !translated(info.LanguageSpecBHP).empty(),
                "original displayed specification text unavailable");
    }
    require(FindCarLanguageInfo(0).LanguageSpecFullName==41100 && FindCarLanguageInfo(0).LanguageSpecShortName==41300,
        "full and short original names were conflated");
    const auto actual_trees=port::race::parse_debug_event_tables(read(*root+"/FRONTEND/FRONTB.BUN"));
    int events=0;
    for(const auto& t:actual_trees)for(const auto& e:t.pEvents){
        FrontendDatabase f;f.MakeEventCarsAvailable(t,e,actual_cars,0);
        std::array<bool,64> expected{};
        for(int i=0;i<e.nCars;++i){
            const auto packed=*e.GetCar(t,i);const auto type=(packed>>16)&255;expected.at(type)=true;
            const auto& s=f.FrontendPlayers[0].aCarStatus.at(type);
            require(s.nStatus==eCAR_UNLOCKED_FULLY_VISIBLE && s.nSkins>0 && s.nSkins<=10,"original event has unavailable or invalid listed car");
        }
        for(int i=0;i<64;++i)require((f.FrontendPlayers[0].aCarStatus[i].nStatus==eCAR_UNLOCKED_FULLY_VISIBLE)==expected[i],
            "real event restriction does not match referenced original car identities");
        const port::selection::ListServices original_names{actual_cars,{},
            [&](int type){return actual_cars[type].GetFullName(translated);},false};
        VisibleCarSet original_list(f.FrontendPlayers[0],9,e.nType==6,original_names);
        require(original_list.pCurrentVisibleCar && original_list.TotalNumCars>0
            && original_list.NumberUnlocked==original_list.TotalNumCars,"event did not produce a valid original visible selection list");
        ++events;
    }
    require(events==64,"not all original event restrictions were covered");
    std::puts("PASS Debug availability, visible list, skin, popup/readiness/commit and64 original asset event restrictions");
}

