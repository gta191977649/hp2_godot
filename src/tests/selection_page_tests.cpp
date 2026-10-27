#include "port/runtime/selection_page.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/port/not_implemented.hpp"
#include "port/assets/catalog.hpp"
#include "platform/ps2_asset_root.hpp"
#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Lease final:port::selection::PageLease {
    std::vector<std::string>& events;std::string name;int& live;std::function<void()> release;
    Lease(std::vector<std::string>& e,std::string n,int& count):events(e),name(std::move(n)),live(count){++live;events.push_back("+"+name);}
    ~Lease(){if(release)release();--live;events.push_back("-"+name);}
};
}
int main(){
    const auto path=port::driving::debug_image_path(".");const auto root=hp2::platform::resolve_zzdata_root(".");
    if(path.empty() || !std::filesystem::exists(path) || !root){std::puts("SKIP selection page requires configured Debug ELF/original assets");return 0;}
    auto memory=port::driving::read_debug_image(path);port::selection::initialize_camera_offsets(memory);
    port::assets::DirectorySource source(*root);
    auto resources=port::track::decode_particles(source.read("GLOBAL/GLOBALB.BUN"));resources.emitters.clear();
    const auto definition=std::find_if(resources.definitions.begin(),resources.definitions.end(),[](const auto& d){return d.name=="XE_SelectCarNebula";});
    require(definition!=resources.definitions.end(),"original selection nebula absent");
    port::track::ParticleEmitter outside;outside.name=definition->name;outside.name_hash=outside.data_hash=definition->name_hash;outside.state=1;
    resources.emitters.push_back(outside);
    port::track::SharedParticleState shared{42,50,2};port::track::ParticleSimulation effects(resources,shared);
    std::array<hp2::debug::CarTypeInfo,64> cars{};hp2::debug::FrontendDatabase fred;fred.GameType=9;
    for(int i=0;i<3;++i){auto& car=cars[i];std::snprintf(car.CarTypeName.data(),32,"FIXTURE%d",i);
        car.pLanguageInfo=&hp2::debug::FindCarLanguageInfo(0);car.NumAvailableSkinNumbers=1;
        car.AvailableSkinNumbers[0]=2;car.DefaultSkinNumber=2;
        for(auto& wheel:car.Wheels)wheel.Radius=.3f;
        fred.FrontendPlayers[0].SetCarAvailability(hp2::debug::eCAR_UNLOCKED_FULLY_VISIBLE,i,-1,false,cars,0);
    }
    port::selection::ListServices names{cars,{},[](int i){return std::to_string(i);},false};
    hp2::debug::VisibleCarSet original(fred.FrontendPlayers[0],9,false,names);
    hp2::frontend::FeScreen screen;screen.name_hash_0x08=0xb5074ab4;
    for(auto hash:{0xa8af730bu,0xbea3c79cu,0x75c8b565u,0x736163fcu}){
        hp2::frontend::FeWidget w;w.name_hash=hash;w.type=hash==0x736163fc?1:2;w.text_capacity=80;
        if(hash==0xa8af730b){w.class_hash=0x6a096871;w.x=80.5f;w.y=15.75f;}screen.widgets.push_back(w);
    }
    port::selection::WidgetContext widgets{screen,cars,[](const auto&){return hp2::frontend::WidgetMetrics{12,8};},
        [](int,int){return 123u;},[](int){return std::string("translated");},[](const auto& s){return s;}};
    port::selection::PageServices services{memory,widgets,fred,effects,shared,{},port::selection::OrbitServices{cars}};
    std::map<std::pair<int,int>,port::selection::PageJoy> handlers;
    services.points=0;services.spotlight_colours=0; // Explicit host fixture, never inferred production globals.
    std::vector<std::string> events;int live=0,reinit=0,preloads=0;bool loaded=false;int car_create=0;
    const auto lease=[&](std::string name)->port::selection::PageHandle{return std::make_unique<Lease>(events,std::move(name),live);};
    services.model=[&](auto hash){return lease("model"+std::to_string(hash));};
    services.car=[&](auto& car,int view){require(view==1 && car.BodyMatrix[0]==1 && car.BodyMatrix[15]==1,"render car constructor state differs");return lease("car"+std::to_string(car_create++));};
    services.camera=[&](auto&,int view){require(view==1,"camera view differs");return lease("camera");};
    services.joy=[&](int port,int event,const port::selection::PageJoy& handler)->port::selection::PageHandle{
        require(port>=0 && port<2 && event>=33 && event<=43,"Debug joy registration differs");
        const auto key=std::make_pair(port,event);require(!handlers.count(key),"duplicate input registration");handlers.emplace(key,handler);
        auto token=std::make_unique<Lease>(events,"joy"+std::to_string(port)+":"+std::to_string(event),live);
        token->release=[&,key]{handlers.erase(key);};return token;
    };
    services.load=[&](int type){loaded=false;return lease("load"+std::to_string(type));};services.is_loaded=[&](int){return loaded;};
    services.begin_loading=[&]{events.push_back("begin");};
    services.preload_selected=[&]{++preloads;events.push_back("preload");};
    services.dismiss_popup=[&](int id){events.push_back("dismiss"+std::to_string(id));};
    services.sound=[](int){};services.stop_sound=[&](int id){require(id==12,"stop sound differs");events.push_back("stop");};
    services.help_override=[&](auto id){require(id==0x311cf8,"initial help differs");};
    services.lighting=[](int,float,int){};services.swatch_rgb=[](auto){return std::optional<std::array<std::uint8_t,3>>{{0,0,0}};};
    services.render_platform=[]{};services.rendering.get_skin_info=[](int,int,int){};
    services.rendering.reinit=[&](auto&,const auto&){++reinit;};
    services.locked.format_unlock_blurb=[](auto& out,int,int,int){out[0]=0;};services.locked.render_text=[](auto,auto,auto,auto,float){};
    {
        port::selection::SelectionPage page(original,services,0,0,1.f/60);
        auto& ui=page.selector();
        require(&ui.VisibleCars!=&original && ui.VisibleCars.pCurrentVisibleCar!=original.pCurrentVisibleCar,"page retained temporary car nodes");
        require(ui.LoadingCar && !ui.LoadedCar && !reinit && effects.particle_count()==0,"page constructor invented load completion or stepped effects before display");
        require(effects.auto_emitter_order().size()==21 && ui.NumNebulaEmitters==20,"page did not preserve unrelated FrameEffects registration");
        require(ui.NebulaColourParam==0 && page.reflected_car().Reflection==1,"unlocked nebula or reflected car defaults differ");
        require(handlers.size()==22,"page input registrations have no owned callback");
        handlers.at({1,34})(1,34,1);require(ui.AllCarsUnlocked==1,"registered second-port input failed native dispatch");
        page.after_display();require(effects.particle_count()==21 && shared.particles==71,"after-display shared frontend step differs");
        loaded=true;page.tick(1,66,1.f/60);require(ui.LoadedCar && !ui.LoadingCar && reinit==4,"page initial completion lost original double reinit");
        auto* next=ui.VisibleCars.GetNextCar(ui.VisibleCars.pCurrentVisibleCar);ui.VisibleCars.pCurrentVisibleCar=next;
        ui.FadeCarOverlayAlpha=130;ui.ThumbnailHorizCubic.state=0;page.tick(2,132,1.f/60);
        require(ui.LoadingCar==next && !ui.LoadedCar,"page failed real switch ownership");
        const auto before=events.size();page.close();
        require(page.closed() && !live && handlers.empty() && effects.auto_emitter_order().size()==1 && effects.resources().emitters.size()==1
            && !effects.particle_count() && shared.particles==50 && shared.groups==2,"page exit leaked resource/registration/shared pool ownership");
        require(preloads==2 && events[before]=="dismiss-1","page exit omitted dismissal or duplicate original preload");
        const auto first=std::find(events.begin()+before,events.end(),"preload");
        require(first!=events.end() && first+1!=events.end() && *(first+1)=="preload" && *(first+2)=="-load1","preload/unload order differs");
        require(screen.widgets[0].x==80 && screen.widgets[0].y==15,"page close failed authored locked group restore");
        const auto closed_events=events.size();page.close();require(events.size()==closed_events,"port close repeated original teardown");
        bool rejected=false;try{page.tick(3,198,1.f/60);}catch(const std::logic_error&){rejected=true;}require(rejected,"closed page accepted tick");
    }
    // Reentry must not grow stable emitter storage or retain registration leases.
    for(int i=0;i<30;++i){port::selection::SelectionPage page(original,services,0,0,1.f/60);page.close();
        require(!live && effects.resources().emitters.size()==1 && effects.auto_emitter_order().size()==1,"reentry grew emitter storage");}
    // Failed construction rolls back completed registrations/loads and cancels
    // host work without executing normal successful-exit preload/audio actions.
    const auto old_preloads=preloads;auto failed=services;
    failed.swatch_rgb=[](auto)->std::optional<std::array<std::uint8_t,3>>{hp2::not_implemented("0x001947D0","injected missing palette dependency");};
    require(!hp2::guard([&]{port::selection::SelectionPage page(original,failed,0,0,1.f/60);}).empty(),"injected construction failure did not propagate");
    require(!live && handlers.empty() && effects.auto_emitter_order().size()==1 && effects.resources().emitters.size()==1
        && shared.particles==50 && preloads==old_preloads,"failed construction leaked leases/emitter storage or ran success exit");
    {
        auto failed_close=services;
        failed_close.preload_selected=[]{hp2::not_implemented("0x00192580","injected unavailable preload backend");};
        port::selection::SelectionPage page(original,failed_close,0,0,1.f/60);
        require(!hp2::guard([&]{page.close();}).empty(),"unavailable preload close falsely succeeded");
        bool rejected=false;try{page.tick(0,0,1.f/60);}catch(const std::logic_error&){rejected=true;}
        require(rejected && !page.closed(),"failed teardown allowed native Tick or reported successful close");
    }
    require(!live && handlers.empty() && effects.auto_emitter_order().size()==1,"failed close disposal leaked ownership");
    std::puts("PASS native selection page delayed load/switch, shared after-display effects, ordered exit,30 reentries and construction rollback (host service fixtures)");
}
