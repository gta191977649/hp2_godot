#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void add(hp2::frontend::FeScreen& screen,std::uint32_t hash){hp2::frontend::FeWidget w;w.name_hash=hash;w.type=2;screen.widgets.push_back(w);}
}
int main(){
    using namespace hp2::debug;
    const auto path=port::driving::debug_image_path(".");
    if(path.empty() || !std::filesystem::exists(path)){std::puts("SKIP selection lifecycle: configured original Debug ELF required");return 0;}
    auto memory=port::driving::read_debug_image(path);
    port::selection::initialize_camera_offsets(memory);
    std::array<CarTypeInfo,64> cars{};FrontendPlayerInfo player;
    for(int i=0;i<3;++i){auto& c=cars[i];std::snprintf(c.CarTypeName.data(),32,"FIXTURE%d",i);
        c.NumAvailableSkinNumbers=1;c.AvailableSkinNumbers[0]=2;c.DefaultSkinNumber=2;
        for(auto& wheel:c.Wheels){wheel.Radius=.3f;wheel.Position[2]=-.1f;}
        player.SetCarAvailability(eCAR_UNLOCKED_FULLY_VISIBLE,i,-1,false,cars,0);
    }
    port::selection::ListServices list_svc{cars,{},[](int i){return std::to_string(i);},false};
    VisibleCarSet list(player,9,false,list_svc);CarSelector selector{list};
    auto* a=list.VisibleCarList[0].get();auto* b=list.VisibleCarList[1].get();auto* c=list.VisibleCarList[2].get();
    list.pCurrentVisibleCar=a;
    FrontEndRenderingCar main,reflection;SelectCarCameraMover camera;
    selector.pFrontEndRenderingCar=&main;selector.pFrontEndRenderingCarReflection=&reflection;selector.pSelectCarCameraMover=&camera;
    hp2::frontend::FeScreen screen;
    for(auto hash:{0x75c8b565u,0xabb370a6u,0x84f544f3u})add(screen,hash);
    feAlphaAnimation fade(screen),orbit_fade(screen),snap(screen);
    port::selection::WidgetContext widgets{screen,cars,[](const auto&){return hp2::frontend::WidgetMetrics{12,8};},
        [](int,int){return 123u;},{},{}};
    std::vector<std::string> events;bool loaded=false;int reinitializations=0;float lighting=-1;int silhouette=-1;
    port::selection::RenderServices rendering;
    rendering.get_skin_info=[&](int type,int skin,int flags){require(skin==2 && flags==0,"loaded skin ID/flags differ");events.push_back("skin"+std::to_string(type));};
    rendering.reinit=[&](auto&,const auto& ride){require(ride.SkinNumber==2,"reinit lost original skin ID");++reinitializations;};
    port::selection::OrbitServices orbit{cars};orbit.adjust_help=[&]{events.push_back("help");};
    port::selection::TickServices svc{memory,widgets,rendering,orbit,fade,orbit_fade,snap};
    svc.elapsed=1.f/60;
    svc.is_loaded=[&](int type){events.push_back("poll"+std::to_string(type));return loaded;};
    svc.unload=[&](int type){events.push_back("unload"+std::to_string(type));};
    svc.load=[&](int type){events.push_back("load"+std::to_string(type));};
    svc.begin_loading=[&]{events.push_back("begin");};
    svc.swatch_rgb=[&](std::uint32_t){events.push_back("swatch");return std::optional<std::array<std::uint8_t,3>>{{0,0,0}};};
    svc.set_lighting=[&](int view,float intensity,int sil){require(view==1,"selector light view differs");lighting=intensity;silhouette=sil;events.push_back("light");};
    svc.update_locked=[&]{events.push_back("locked");};svc.update_nebula=[&]{events.push_back("nebula");};
    selector.SetupSwatches(widgets);selector.SetupThumbnails(widgets);selector.LoadingCar=a;
    selector.Tick(svc);
    require(selector.LoadingCar==a && !selector.LoadedCar && selector.FadeCarOverlayAlpha==130 && !reinitializations,"incomplete async load became visible");
    require(events==std::vector<std::string>{"poll0","swatch","light","locked","nebula"},"Tick dependency order differs");
    require(camera.OffsetStats[0]==memory.scalar(0x325608) && camera.OffsetStats[2]==memory.scalar(0x32560c)
        && camera.OffsetOrbit[2]==memory.scalar(0x325618),"camera offsets omitted original dynamic initialization");
    loaded=true;events.clear();selector.Tick(svc);
    require(selector.LoadedCar==a && !selector.LoadingCar && selector.FadeCarOverlayAlpha==127 && reinitializations==4,
        "initial completion must run ShowLoadedCar twice at full overlay");
    require(lighting==1 && silhouette==0,"black swatch original fourth-power light differs");
    list.pCurrentVisibleCar=b;selector.FadeCarOverlayAlpha=123;selector.ThumbnailHorizCubic.state=0;events.clear();
    selector.Tick(svc);require(selector.LoadedCar==a && !selector.LoadingCar && selector.FadeCarOverlayAlpha==130,"fade equality started load one frame early");
    events.clear();loaded=false;selector.Tick(svc);
    require(!selector.LoadedCar && selector.LoadingCar==b && events[0]=="unload0" && events[1]=="load1" && events[2]=="begin",
        "old unload/new request/begin order differs");
    list.pCurrentVisibleCar=c;loaded=true;events.clear();const auto old_reinits=reinitializations;selector.Tick(svc);
    require(!selector.LoadedCar && selector.LoadingCar==c && reinitializations==old_reinits
        && events[0]=="poll1" && events[1]=="unload1" && events[2]=="load2" && events[3]=="begin",
        "superseded completion rendered stale car or skipped unload");
    selector.Tick(svc);require(selector.LoadedCar==c && !selector.LoadingCar,"latest original load did not complete");
    selector.FadeCarOverlayAlpha=0;list.pCurrentVisibleCar=b;selector.ThumbnailHorizCubic.state=2;
    selector.ThumbnailHorizCubic.ValDesired=100;selector.ThumbnailHorizCubic.duration=1;
    for(int i=0;i<20;++i)selector.Tick(svc);
    require(selector.LoadedCar==c && !selector.LoadingCar,"thumbnail animation did not gate old car unload");
    selector.ThumbnailHorizCubic.state=0;selector.Tick(svc);require(selector.LoadingCar==b,"settled thumbnail failed to allow switch");
    selector.Tick(svc);selector.SelectorMode=1;selector.DesiredSelectorMode=1;selector.ManualOrbitEngaged=1;
    selector.OrbitActiveTime=100;orbit.real_time_frames=100+memory.signed_word(0x327b20);const auto rotation=selector.CarRotationZ;
    selector.Tick(svc);require(selector.DesiredSelectorMode==1 && selector.CarRotationZ==rotation,"orbit timeout boundary/manual rotation differs");
    ++orbit.real_time_frames;events.clear();selector.Tick(svc);
    require(selector.DesiredSelectorMode==0 && selector.SelectorMode==1-memory.scalar(0x3255d8) && events[0]=="help",
        "strict orbit timeout/help/transition order differs");
    selector.SelectorMode=0;selector.DesiredSelectorMode=.001f;selector.ManualOrbitEngaged=0;selector.CarRotationZ=65535;
    selector.Tick(svc);require(selector.SelectorMode==.001f && selector.CarRotationZ==std::uint16_t(65535+memory.half(0x327ad2)),"selector step clamp/rotation wrap differs");
    selector.LoadedCar=b;b->CarUnlocked=false;b->pCarStatus->nStatus=eCAR_LOCKED_PARTLY_VISIBLE;
    selector.Tick(svc);require(silhouette==1,"loaded partly-visible car lost silhouette");
    b->pCarStatus->nStatus=eCAR_LOCKED_CLOAKED;selector.Tick(svc);require(lighting==0 && main.OverrideModel==&selector.CloakedCarModel,"cloaked car leaked swatch light/model");
    svc.all_cars_available=true;selector.Tick(svc);require(silhouette==0,"global availability did not suppress silhouette");
    auto missing=svc;missing.update_nebula={};require(!hp2::guard([&]{selector.Tick(missing);}).empty(),"missing original effects silently no-op");
    std::puts("PASS Debug selector load/fade/stale completion/scroll gate/orbit/light order; original runtime comparison remains open");
}
