#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/port/assets/fe_language.hpp"
#include "game_core/port/not_implemented.hpp"
#include "platform/ps2_asset_root.hpp"
#include "port/vehicle/catalog.hpp"
#include "port/runtime/frame_builder.hpp"
#include "port/vehicle/model.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(float a,float b,const char* message){require(std::abs(a-b)<.00001f,message);}
std::uint32_t hash(const std::string& text){return hp2::frontend::bStringHash(text.c_str());}
void add(hp2::frontend::FeScreen& screen,std::uint32_t id){hp2::frontend::FeWidget w;w.name_hash=id;w.type=2;screen.widgets.push_back(w);}
hp2::frontend::FeWidget& widget(hp2::frontend::FeScreen& screen,std::uint32_t id){
    auto* w=hp2::frontend::FindWidget_00143D50(screen,id,0);require(w!=nullptr,"fixture widget absent");return *w;
}
}
int main(){
    {
        hp2::frontend::FeScreen alpha;
        add(alpha,1);add(alpha,2);add(alpha,3);
        alpha.widgets[0].class_hash=10;alpha.widgets[0].a=128;
        alpha.widgets[0].field_0x5c={0x40123456u,0xffabcdefu,0x00010203u};
        alpha.widgets[1].type=1;alpha.widgets[1].a=80;alpha.widgets[1].class_hash=11;
        alpha.widgets[2].a=64;alpha.widgets[2].class_hash=12;
        {
            hp2::debug::feAlphaAnimation fade(alpha,10),orbit(alpha,11),snap(alpha,12);
            fade.SetAlphaScale(.5f);
            require(alpha.widgets[0].a==192 && alpha.widgets[0].field_0x5c[0]==0x20123456u
                && alpha.widgets[0].field_0x5c[1]==0x00abcdefu,"cached signed per-corner alpha conversion differs");
            fade.SetAlphaScale(.5f);require(alpha.widgets[0].a==192,"alpha animation compounded live alpha");
            fade.AddObject(999);require(fade.Fading.size()==1,"missing alpha object must be original no-op");
            hp2::debug::FrontendPlayerInfo player;std::array<hp2::debug::CarTypeInfo,64> cars{};
            port::selection::ListServices services{cars,{},[](int){return std::string("TEST");},false};
            player.aCarStatus[0].nStatus=hp2::debug::eCAR_UNLOCKED_FULLY_VISIBLE; player.aCarStatus[0].nSkins=1;
            hp2::debug::VisibleCarSet list(player,9,false,services);hp2::debug::CarSelector ui{list};
            ui.SelectorMode=1;ui.UpdateFadeGroups(fade,orbit,snap,cars);
            require(alpha.widgets[0].a==0 && alpha.widgets[1].a==80 && alpha.widgets[2].a==0,"orbit fade groups differ");
            ui.DesiredSelectorMode=0;ui.SelectorMode=.5f;ui.UpdateFadeGroups(fade,orbit,snap,cars);
            require(alpha.widgets[2].a==0 && alpha.widgets[1].a==40,"snap fade must retain prior alpha mid-transition");
            ui.SelectorMode=.25f;ui.UpdateFadeGroups(fade,orbit,snap,cars);
            require(alpha.widgets[2].a==64,"snap group did not restore below original threshold");
            cars[0].UsageType=hp2::debug::CAR_USAGE_TYPE_COP;ui.UpdateFadeGroups(fade,orbit,snap,cars);
            require(alpha.widgets[1].a==0,"cop orbit group must remain hidden");
        }
        require(alpha.widgets[0].a==128 && alpha.widgets[0].field_0x5c[1]==0xffabcdefu
            && alpha.widgets[1].a==80 && alpha.widgets[2].a==64,"animation destruction failed to restore authored alpha");
        {
            hp2::debug::feAlphaAnimation duplicate(alpha,10);
            duplicate.SetAlphaScale(.5f);duplicate.AddObject(1);
            require(duplicate.Fading.size()==2,"original duplicate alpha capture was deduplicated");
        }
        require(alpha.widgets[0].a==192,"duplicate restoration must use original head-first destruction order");
    }
    using namespace hp2::debug;
    tCubic1D path;path.duration=.5f;path.ValDesired=39;path.state=2;
    cPoint::SplineSeek(path,.125f,0,0);near(path.Val,6.09375f,"original spline quarter value differs");
    near(path.dVal,43.875f,"original spline quarter velocity differs");
    auto retarget=path;retarget.ValDesired=78;retarget.state=2;cPoint::SplineSeek(retarget,.125f,0,0);
    near(retarget.Val,23.4990234375f,"retarget discarded original nonzero velocity");
    near(retarget.dVal,89.12109375f,"retarget derivative differs");
    cPoint::SplineSeek(path,.125f,0,0);near(path.Val,19.5f,"original spline half value differs");
    cPoint::SplineSeek(path,.25f,0,0);require(path.Val==39 && path.dVal==0 && path.state==0,"original spline arrival differs");
    auto early=path;early.Val=7;early.ValDesired=39;early.arrivalThreshold=.25f;early.state=2;
    cPoint::SplineSeek(early,.125f,0,0);require(early.Val==7 && early.state==0,"early arrival invented a final value");
    auto rebuilt=retarget;rebuilt.flags=1;rebuilt.state=2;cPoint::SplineSeek(rebuilt,.01f,0,0);
    require(rebuilt.state==2,"nonzero flag must preserve rebuild state2");
    // Original DDClamp activation passes DClamp=0 to ClampSecondDerivative.
    auto clamp=retarget;clamp.state=2;cPoint::SplineSeek(clamp,.01f,0,1);
    require(clamp.Coeff[0]==0 && clamp.Coeff[1]==0,"SplineSeek used DDClamp as the second clamp magnitude");
    std::array<CarTypeInfo,64> cars{};FrontendPlayerInfo player;
    for(int i=0;i<64;++i){std::snprintf(cars[i].CarTypeName.data(),32,"TEST%d",i);cars[i].CarClass=3;
        cars[i].NumAvailableSkinNumbers=3;cars[i].AvailableSkinNumbers={2,4,6};cars[i].DefaultSkinNumber=4;}
    player.SetCarAvailability(eCAR_UNLOCKED_FULLY_VISIBLE,0,-1,false,cars,0);
    player.SetCarAvailability(eCAR_LOCKED_CLOAKED,1,-1,false,cars,0);
    const port::selection::ListServices list_services{cars,[](int,bool){return false;},[](int){return std::string("fixture");},false};
    VisibleCarSet list(player,9,false,list_services);CarSelector selector{list};
    hp2::frontend::FeScreen screen;
    for(int i=0;i<3;++i)add(screen,hash("SWATCH_"+std::to_string(i)));
    for(auto id:{0xedf2e4eeu,0x20d73d01u,0x7fe46db3u,0x2561a343u,hash("THUMBNAIL_TEST0"),hash("THUMBNAIL_TEST1"),hash("THUMBNAIL_CLOAK_0")})add(screen,id);
    const port::selection::WidgetContext context{screen,cars,[](const auto&){return hp2::frontend::WidgetMetrics{12,8};},
        [](int type,int skin){require(type==0 || type==1,"swatch car identity changed");return std::uint32_t(100+skin);},{},{}};
    auto ctx=context;selector.SetupSwatches(ctx);selector.UpdateSwatches(ctx);
    {
        port::selection::RenderServices render;port::selection::OrbitServices orbit{cars};
        std::vector<int> sounds;int locked_calls=0,info_calls=0,ticks=0,platform_calls=0,locked_render_calls=0;
        port::selection::ScreenServices services{{},ctx,render,orbit};
        services.set_locked_car=[&](auto* car,int unlocked,int second){require(car && !unlocked && !second,"locked display callback arguments differ");++locked_calls;};
        services.update_info=[&]{++info_calls;};services.sound=[&](int sound){sounds.push_back(sound);};
        services.tick=[&]{++ticks;};services.render_platform=[&]{++platform_calls;};
        services.render_locked=[&](int value){require(value==0,"locked render override differs");++locked_render_calls;};
        ScreenCallbackData data{9,7,0};
        selector.FadeCarOverlayAlpha=0; // Explicit fixture: original loading has completed.
        selector.ScreenCallback(MENUITEM_PAD_LEFT,data,services);
        require(locked_calls==1 && sounds.empty() && !info_calls,"bounded selection must still update locked displayer before returning");
        selector.ScreenCallback(MENUITEM_PAD_DOWN,data,services);
        require(data.Param==1 && list.pCurrentVisibleCar->SkinIndex==2 && sounds==std::vector<int>{14},"skin callback state/sound differs");
        selector.ScreenCallback(MENUITEM_PAD_RIGHT,data,services);
        require(locked_calls==2 && info_calls==1 && selector.ThumbnailHorizCubic.ValDesired==326
            && selector.ThumbnailHorizCubic.state==2 && sounds.back()==12,"car change original ordering/retarget differs");
        selector.ScreenCallback(MENUITEM_PAD_UP,data,services);
        require(sounds.size()==2 && data.Param==1,"locked car color input must be consumed without changing skin/sound");
        selector.SelectorMode=.5f;selector.ScreenCallback(MENUITEM_PAD_LEFT,data,services);
        require(info_calls==1 && locked_calls==2,"transition must suppress car selection");
        selector.DesiredSelectorMode=1;data.Noise=7;selector.ScreenCallback(MENUITEM_SELECT,data,services);
        require(data.Noise==0,"orbit confirmation must suppress frontend sound");
        selector.ScreenCallback(SCREEN_TICK,data,services);selector.ScreenCallback(SCREEN_RENDER,data,services);
        require(ticks==1 && platform_calls==1 && locked_render_calls==1,"screen tick/render dependency dispatch differs");
        selector.ScreenCallback(SCREEN_ACTIVATE,data,services);
        services.state.maybe_continue=[](){return true;};
        selector.PopupID=12;data.Param=7;data.Noise=4;
        selector.ScreenCallback(POPUP_DISMISS,data,services);
        require(data.Noise==18 && selector.PopupID==-1,"continued popup must publish original selection sound");
        selector.SelectorMode=0;selector.DesiredSelectorMode=0;list.pCurrentVisibleCar=list.VisibleCarList[0].get();
        list.pCurrentVisibleCar->SkinIndex=1;selector.SetupSwatches(ctx);selector.UpdateSwatches(ctx);
    }

    require(selector.NumSwatches==3 && widget(screen,hash("SWATCH_1")).texture_hash==104,"swatch used skin ordinal instead of actual ID");
    require(widget(screen,hash("SWATCH_1")).x==587 && widget(screen,hash("SWATCH_1")).y==143
        && widget(screen,0x7fe46db3).y==139 && widget(screen,0x7fe46db3).x==583,"authored swatch/marker coordinates differ");
    selector.SetupThumbnails(ctx);selector.UpdateThumbnails(ctx,0);
    require(widget(screen,hash("THUMBNAIL_TEST0")).x==377 && widget(screen,0x2561a343).x==416
        && widget(screen,hash("THUMBNAIL_CLOAK_0")).x==428 && !widget(screen,hash("THUMBNAIL_TEST1")).visible,
        "original locked divider or cloak thumbnail placement differs");
    list.pCurrentVisibleCar=list.VisibleCarList[1].get();selector.SetupSwatches(ctx);selector.UpdateSwatches(ctx);
    require(!widget(screen,0x7fe46db3).visible && !widget(screen,hash("SWATCH_0")).visible,"cloaked car leaked color selection");
    selector.ScreenHash=0x18dc8e53;selector.AllCarsUnlocked=1;selector.UpdateSwatches(ctx);
    require(widget(screen,hash("SWATCH_0")).x==606 && widget(screen,hash("SWATCH_0")).y==321,"second-player swatch coordinates differ");
    selector.IsTwoPlayerScreen=1;selector.SetupThumbnails(ctx);
    require(selector.ThumbnailHorizCubic.Val==344,"locked target must subtract divider even when global unlock override is active");
    auto cop=cars[0];cop.UsageType=CAR_USAGE_TYPE_COP;std::snprintf(cop.CarTypeName.data(),32,"LAMBOCOP");
    require(selector.CalculateCarLogoTexture(cop)==hash("CARLOGO_MURCIELAGO")
        && selector.CalculateNFSLogoTexture(cop)==hash("LOGO_SHERIFF"),"original two-player cop logo aliases differ");
    auto hp=cars[0];hp.HighPerformance=1;std::snprintf(hp.CarTypeName.data(),32,"TESTHP");
    require(selector.CalculateCarLogoTexture(hp)==hash("CARLOGO_TEST") && selector.CalculateNFSLogoTexture(hp)==hash("LOGO_NFS"),"two-player high-performance logo handling differs");
    const auto root=hp2::platform::resolve_zzdata_root(".");if(!root){std::puts("PASS selection widget/spline state; SKIP original assets");return 0;}
    port::assets::DirectorySource source(*root);const auto global=source.read("GLOBAL/GLOBALB.BUN"),front=source.read("FRONTEND/FRONTB.BUN");
    const auto original_cars=port::selection::decode_car_info(global);
    const auto trees=port::race::parse_debug_event_tables(front);
    auto screens=hp2::frontend::parse_screens(front);
    const auto authored=std::find_if(screens.begin(),screens.end(),[](const auto& s){return s.name_hash_0x08==0xb5074ab4;});
    require(authored!=screens.end(),"original one-player selection document missing");
    port::vehicle::Catalog skins(source);skins.load();port::assets::Catalog textures(source);
    const auto cache=std::make_shared<port::vehicle::ModelCache>();
    port::vehicle::Model platform(cache),cover(cache);
    platform.load_frontend_solid(source,hash("PLATFORM"));cover.load_frontend_solid(source,hash("CARCOVER"));
    require(platform.is_frontend_solid() && cover.is_frontend_solid() && platform.car_id()==-1 && cover.car_id()==-1,
        "frontend named-solid adapter fabricated a vehicle identity");
    require(platform.parts().size()==1 && cover.parts().size()==1 && !platform.textures().empty() && !cover.textures().empty(),
        "original selector props were not loaded with their real geometry/textures");
    const auto reads=cache->reads;port::vehicle::Model reused(cache);reused.load_frontend_solid(source,hash("PLATFORM"));
    require(cache->reads==reads && reused.geometry().solids.front().hash==hash("PLATFORM"),"frontend prop reload bypassed native geometry cache");
    auto fonts=hp2::frontend::parse_fonts(front);auto more=hp2::frontend::parse_fonts(global);fonts.insert(fonts.end(),more.begin(),more.end());
    port::runtime::WidgetSizes sizes(textures,fonts);
    hp2::port::assets::FeLanguage english(source.read("LANGUAGES/FE_ENGLISH.BIN"));
    const auto translated=[&](int id){std::string text;require(english.lookup(id,text),"original numeric name translation missing");return text;};
    for(const auto& event:trees[1].pEvents){
        FrontendDatabase fred;fred.MakeEventCarsAvailable(trees[1],event,original_cars,0);
        const port::selection::ListServices names{original_cars,{},[&](int type){return original_cars[type].GetFullName(translated);},false};
        VisibleCarSet available(fred.FrontendPlayers[0],9,event.nType==6,names);CarSelector ui{available};
        auto document=*authored;
        port::selection::WidgetContext bindings{document,original_cars,sizes.measure(),
            [&](int type,int skin){return skins.skin(type,skin).Field_0x30;},translated,
            [&](const auto& key){std::string text;return english.lookup_hash(hash(key),text)?text:key;}};
        ui.SetupThumbnails(bindings);
        for(const auto& node:available.VisibleCarList){
            available.pCurrentVisibleCar=node.get();ui.SetupSwatches(bindings);ui.UpdateSwatches(bindings);
            ui.SetLogo(bindings);ui.UpdateThumbnails(bindings,0);
            require(widget(document,0x39e91754).text==original_cars[node->eCarType].GetFullName(translated),"real selected car name binding differs");
            for(int i=0;i<ui.NumSwatches;++i)require(textures.texture(ui.SwatchTextureHashes[i])!=nullptr,"original swatch texture missing");
            require(textures.texture(widget(document,0xb270d4a5).texture_hash)!=nullptr,"original selected car logo texture missing");
        }
    }
    std::puts("PASS selection spline/retarget, swatches, cloak/divider, two-player logos and32 original event document bindings");
}

