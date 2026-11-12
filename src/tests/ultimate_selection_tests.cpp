#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/port/frontend/popup.hpp"
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
    {
        // Explicit port fixtures, not an original-frame or ELF comparison.
        hp2::frontend::FeScreen text;std::vector<hp2::frontend::FeWidget> lines;
        for(int i=0;i<3;++i){hp2::frontend::FeWidget w;w.type=1;w.class_hash=9;w.text_capacity=8;
            w.y=10.f+i*12;w.visible=1;text.widgets.push_back(w);}
        const hp2::frontend::WidgetMeasure measure=[](const auto&){return hp2::frontend::WidgetMetrics{20,7};};
        require(hp2::frontend::Sub_00144BA0(text,9,"short",measure)==29,
            "popup bottom must use next authored line before hiding it");
        require(text.widgets[0].text=="short" && !text.widgets[1].visible && !text.widgets[2].visible,
            "popup unused line visibility differs");
        require(hp2::frontend::Sub_00144BA0(text,9,"a@b@c",measure)==41 && text.widgets[2].text=="c",
            "popup last line bottom differs");
        require(hp2::frontend::Sub_00144BA0(text,9,"a@b@c@d",measure)==0,
            "popup exhausted-line return differs");
        port::driving::Memory memory;memory.map(0x327628,24);memory.map(0x328e40,8);
        memory.word(0x327628,128);memory.word(0x32762c,64);memory.word(0x327630,0);
        memory.word(0x327634,24);memory.word(0x327638,24);memory.word(0x32763c,24);
        memory.scalar(0x328e40,1);memory.scalar(0x328e44,2);
        hp2::frontend::FeScreen document;document.name_hash_0x08=123;
        for(int i=0;i<3;++i){hp2::frontend::FeWidget w;w.type=1;w.name_hash=0xe0105663u+i;
            w.r=10+i;w.g=20;w.b=30;w.a=57;document.widgets.push_back(w);}
        port::frontend::PopupView view(document,memory,measure,[](int){return std::string("translated");});
        auto svc=view.services([](auto,auto&){},[](int){});
        for(int i=0;i<3;++i)svc.reset_item(i,123,0);
        svc.highlight(1);auto& w=view.screen.widgets[1];
        require(w.r==128 && w.g==64 && w.b==0 && w.a==57 && w.dirty_0x2b==1,
            "popup item highlight changed original alpha or colour");
        svc.unhighlight(1);require(w.r==11 && w.g==20 && w.b==30 && w.a==57,
            "popup item did not restore saved RGB");
        svc.set_highlight_animation(1);
        hp2::frontend::FeWidget unrelated;unrelated.name_hash=0xab671c9a;
        require(!view.ghost_target(unrelated),"popup ghost hijacked another document's identical hash");
        port::frontend::PopupView second(document,memory,measure,[](int){return std::string("translated");});
        const auto first_targets=view.button_targets(&view.screen,0,3);
        const auto second_targets=second.button_targets(&second.screen,1,3);
        require(first_targets.at(0xe0105663u)=="popup:0:0" && second_targets.at(0xe0105663u)=="popup:1:0",
            "two live popup slots aliased their UI action identities");
        require(view.button_targets(&second.screen,0,3).empty() && second.button_targets(&view.screen,1,3).empty(),
            "popup targets leaked into another document with identical hashes");
    }
    {
        port::frontend::PopupMenu popup;popup.active=true;popup.config.n_buttons=3;
        popup.config.button_params={1,2,3};popup.index=0;
        std::vector<std::string> calls;
        port::frontend::PopupServices svc;
        {
            port::frontend::PopupMenu summoned;port::frontend::PopupServices init;
            std::vector<std::string> order;port::selection::Popup config;
            config.n_buttons=3;config.start_button=2;config.message_id=49000;
            config.button_ids={10,20,30};config.flags=7;config.title_id=40;
            init.show_view=[&]{require(summoned.active && summoned.config.message_id==49000,"show before config copy");order.emplace_back("show");};
            init.callback=[&](auto msg,auto& data){require(msg==hp2::debug::POPUP_ACTIVE && data.Port==-1 && data.Noise==0,"summon callback payload differs");data.Noise=3;order.emplace_back("callback");};
            init.sound=[&](int n){require(n==3,"summon ignored callback noise");order.emplace_back("sound");};
            init.reset_item=[&](int i,auto hash,int port){require(hash==config.screen_hash && port==config.port,"item screen/port differs");order.push_back("reset"+std::to_string(i));};
            init.highlight=[&](int i){require(i==2,"configured starting index lost");order.emplace_back("highlight");};
            init.render_message=[&](int id){require(id==49000,"message ID differs");order.emplace_back("message");return 35;};
            init.set_top_y=[&](auto hash,int y){require(y==45,"button baseline differs");order.push_back("y"+std::to_string(hash-0xe0105663u));};
            init.clear_text=[&](auto hash){order.emplace_back(hash==0x85eda094u?"clear-title":"clear-button");};
            init.set_language=[&](auto hash,int id){require(id==(hash==0x85eda094u?40:config.button_ids.at(hash-0xe0105663u)),"button/title language differs");order.emplace_back("language");};
            init.align_arrows=[&]{order.emplace_back("arrows");};
            init.set_highlight_animation=[&](int i){require(i==2,"animation target differs");order.emplace_back("animation");};
            summoned.Sub_0016AAD8(config,init);
            require(order==std::vector<std::string>({"show","callback","sound","reset0","reset1","reset2","highlight","message","y0","y1","y2","clear-button","clear-button","clear-button","clear-title","language","language","language","language","arrows","animation"}),"Summon operation order differs");
            require(!hp2::guard([&]{summoned.Sub_0016AAD8(config,{});}).empty(),"Summon silently omitted view");
        }
        svc.dismiss_view=[&]{require(popup.active,"popup hidden after deactivation");calls.emplace_back("hide");};
        svc.unhighlight=[&](int i){calls.push_back("off"+std::to_string(i));};
        svc.highlight=[&](int i){calls.push_back("on"+std::to_string(i));};
        svc.item_hash=[](int i){return std::uint32_t(100+i);};
        svc.set_highlight_animation=[&](int i){calls.push_back("anim"+std::to_string(i));};
        svc.align_arrows=[&]{calls.emplace_back("arrows");};
        svc.sound=[&](int i){calls.push_back("sound"+std::to_string(i));};
        svc.callback=[&](auto msg,auto& data){
            if(msg==hp2::debug::POPUP_DISMISS){
                require(!popup.active && data.Param==3 && data.Noise==5 && data.Port==1,
                    "popup dismissal parameter/activity/noise/port differs");
                popup.active=true;popup.config.message_id=99;data.Noise=18;calls.emplace_back("next-popup");
            }else{
                require(msg==hp2::debug::POPUP_NEXT && data.ItemHash==102 && data.Param==0 && data.Noise==1,
                    "popup navigation callback differs");calls.emplace_back("move");
            }
        };
        popup.Sub_0016AF28(1,5,0,svc);require(calls.empty(),"release changed popup");
        popup.Sub_0016AF28(1,3,1,svc);require(calls.empty(),"horizontal popup accepted digital up");
        popup.Sub_0016AF28(1,5,1,svc);
        require(popup.index==2 && calls==std::vector<std::string>({"off0","on2","move","sound1","anim2","arrows"}),
            "popup wrap/highlight/callback/animation order differs");
        calls.clear();popup.Sub_0016AF28(1,9,1,svc);
        require(popup.active && popup.config.message_id==99 && calls==std::vector<std::string>({"hide","next-popup","sound18"}),
            "reentrant dismissal erased next popup or callback noise");
        require(!hp2::guard([&]{popup.Sub_0016AE08(1,3,{});}).empty(),"missing popup view silently dismissed");
        int dismissed=-1;svc.callback=[&](auto msg,auto& data){
            if(msg==hp2::debug::POPUP_DISMISS)dismissed=data.Param;
        };
        popup.config.flags=8;popup.Sub_0016AF28(1,9,1,svc);
        require(dismissed==-1,"noncancellable flags8 popup accepted confirmation");
        popup.config.flags=10;popup.config.cancel_param=42;popup.Sub_0016AF28(1,9,1,svc);
        require(dismissed==42,"flags8 confirmation did not take cancel branch");
        popup.active=true;popup.config.flags=0;popup.index=1;popup.config.n_buttons=3;
        popup.Sub_0016AF28(1,5,1,svc);require(popup.index==1,"vertical popup accepted digital horizontal");
        popup.Sub_0016AF28(1,4,1,svc);require(popup.index==2,"vertical down did not advance");
        popup.Sub_0016AF28(1,4,1,svc);require(popup.index==0,"vertical down failed to wrap");
        popup.config.flags=1;popup.Sub_0016AF28(1,18,1,svc);require(popup.index==2,"analog left mapping differs");
        popup.Sub_0016AF28(1,19,1,svc);require(popup.index==0,"analog right mapping differs");
        popup.config.n_buttons=0;popup.config.button_params[0]=7;
        popup.active=true;popup.Sub_0016AF28(1,7,1,svc);require(dismissed==7,"zero-button Start parameter differs");
        port::frontend::PopupList slots;port::selection::Popup config;
        const auto summon=[](auto& menu,const auto& config){menu.config=config;menu.active=true;};
        require(slots.Sub_0016C028(config,summon)==0 && slots.Sub_0016C028(config,summon)==1,
            "popup owner did not allocate first inactive slot");
        require(slots.Sub_0016C028(config,{})==-1,"full popup owner invoked missing summon or exceeded two slots");
        slots.Sub_0016C080(-1,{});slots.Sub_0016C080(2,{});
        bool hidden=false;port::frontend::PopupServices owner_svc;
        owner_svc.dismiss_view=[&]{hidden=true;};owner_svc.sound=[](int){};
        owner_svc.callback=[&](auto msg,auto& data){
            require(msg==hp2::debug::POPUP_DISMISS && hidden && data.Param==6 && !slots.menus[0].active,
                "programmatic popup dismissal config/order differs");
            require(slots.Sub_0016C028(config,summon)==0,"reentrant popup did not reuse released first slot");
        };
        slots.Sub_0016C080(0,owner_svc);require(slots.menus[0].active,"owner erased callback replacement");
        slots.menus[1].active=false;slots.Sub_0016C080(1,{});
        require(!hp2::guard([&]{slots.Sub_0016C028(config,{});}).empty(),"inactive slot silently skipped missing Summon");
    }
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
        std::array<int,2> preloaded{-1,-1};std::vector<std::string> requests;
        port::selection::PreloadServices loader;
        loader.unload=[&](int type){requests.push_back("unload"+std::to_string(type));};
        loader.load=[&](int type){requests.push_back("load"+std::to_string(type));};
        loader.begin_loading=[&]{requests.push_back("begin");};
        loader.set_frontend_roundrobin=[&](int frontend,int roundrobin){
            require(frontend==1 && roundrobin==1,"preload original loader flags differ");requests.push_back("flags");};
        fred.NumPlayerCars=1;PreloadCurrentSelectedCars(fred,preloaded,services,loader);
        require(preloaded==std::array<int,2>{1,-1} && requests==std::vector<std::string>{"flags","load1","begin"},"initial preload request differs");
        requests.clear();PreloadCurrentSelectedCars(fred,preloaded,services,loader);
        require(requests.empty(),"duplicate original preload restarted identical request");
        fred.FrontendPlayers[0].AssignCarOfType(9,2,6,0,cars);fred.NumPlayerCars=2;
        PreloadCurrentSelectedCars(fred,preloaded,services,loader);
        require(requests==std::vector<std::string>{"unload1","flags","load2","begin","flags","load1","begin"}
            && preloaded==std::array<int,2>{2,1},"changed/two-player preload request order differs");
        requests.clear();fred.NumPlayerCars=0;UnloadCurrentSelectedCars(preloaded,loader);
        require(requests==std::vector<std::string>{"unload2","unload1"} && preloaded==std::array<int,2>{-1,-1},"preload cleanup depended on current player count");
        requests.clear();UnloadCurrentSelectedCars(preloaded,loader);require(requests.empty(),"preload cleanup repeated unloaded requests");
        fred.NumPlayerCars=1;fred.FrontendPlayers[0].AssignCarOfType(9,1,4,0,cars);
    }
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
    std::vector<std::string> continuation_calls;
    port::selection::ContinuationServices continuation;
    continuation.dismiss_all_popups=[&]{
        require(player.GetSelectedRideInfo(fred.GameType).Type==selected->eCarType,
            "continuation dismissed before committing selected ride");
        continuation_calls.emplace_back("dismiss");
    };
    continuation.start_race=[&]{continuation_calls.emplace_back("race");};
    continuation.change_screen=[&](const char* name,bool push){
        require(std::string(name)=="Select Track" && push,"continuation track/history differs");
        continuation_calls.emplace_back("track");
    };
    require(!SelectCarScreen::MaybeContinueToNextScreen(nullptr,fred,cars,{}),"absent selection continued");
    require(!SelectCarScreen::MaybeContinueToNextScreen(&screen,fred,cars,{}),"unready selection continued");
    selector.SelectorState=SS_HANDLING;
    for(int mode:{8,9,0}){
        fred.GameType=mode;continuation_calls.clear();
        require(SelectCarScreen::MaybeContinueToNextScreen(&screen,fred,cars,continuation)==1,
            "ready selection failed to continue");
        require(continuation_calls==std::vector<std::string>({"dismiss",mode==0?"track":"race"}),
            "continuation branch/order differs");
    }
    fred.GameType=9;
    continuation.dismiss_all_popups=[&]{continuation_calls.emplace_back("dismiss");fred.GameType=0;};
    continuation_calls.clear();
    SelectCarScreen::MaybeContinueToNextScreen(&screen,fred,cars,continuation);
    require(continuation_calls==std::vector<std::string>({"dismiss","track"}),"GameType read before dismissal");
    fred.GameType=9;
    require(!hp2::guard([&]{SelectCarScreen::MaybeContinueToNextScreen(&screen,fred,cars,{});}).empty(),
        "missing continuation service silently succeeded");
    selector.SelectorState=SS_SELECT;
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

