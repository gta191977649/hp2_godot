#include "game_core/indep/src/fmenus.hpp"
#include "game_core/indep/src/frontendscreens/options.hpp"
#include <iostream>
#include <stdexcept>
int main(){try{
    using namespace hp2::frontend;
    auto check=[](bool b){if(!b)throw std::runtime_error("viewport mismatch");};
    FeScreen screen;
    for(int i=1;i<=9;++i){FeWidget w;w.type=1;w.name_hash=0x52e039f2u+i;
        w.y=70+(i-1)*37;screen.widgets.push_back(w);}
    FeWidget ten;ten.type=1;ten.name_hash=0xaee77883;ten.y=403;screen.widgets.push_back(ten);
    FeWidget eleven=ten;eleven.name_hash=0xaee77884;eleven.y=440;screen.widgets.push_back(eleven);
    FeWidget clip;clip.type=2;clip.name_hash=0xc67cfb4a;clip.x=20;clip.y=73;
    screen.widgets.push_back(clip);
    FeWidget above;above.name_hash=0xedf2e4ee;screen.widgets.push_back(above);
    FeWidget below;below.name_hash=0x20d73d01;screen.widgets.push_back(below);
    FeWidget unrelated;unrelated.y=999;screen.widgets.push_back(unrelated);
    const auto authored=screen;
    tScrollingMenuList state;state.Sub_001EF3C8(screen);
    check(screen.widgets[0].clip_hash_0x4c==0xc67cfb4a&&(screen.widgets[0].text_align&0x10));
    auto rect=GetWidgetClip_00145F30(screen,screen.widgets[0],[](const FeWidget&){return WidgetMetrics{690,260};});
    check(rect.valid&&rect.x0==24&&rect.x1==710&&rect.y0==73&&rect.y1==333);
    state.Handle_0024A480(4,0x52e039f3);check(state.Sub_0024A368()==0x6e34e350);
    state.Handle_0024A480(9,0);state.Handle_0024A480(4,0x52e039fa);
    state.Update_0024A1A8(0.25f);check(screen.widgets[7].y==70+7*37-18.5f);
    state.Update_0024A1A8(0.25f);check(screen.widgets[7].y==70+6*37);
    check(screen.widgets[14].y==999&&screen.widgets[11].y==73);
    check(screen.widgets[12].visible==1&&screen.widgets[13].visible==1);
    // Rapid retarget must preserve position and normalized velocity, not restart at rest.
    auto retarget_screen=authored;
    tScrollingMenuList retarget;retarget.Sub_001EF3C8(retarget_screen);
    retarget.Handle_0024A480(4,0x52e039fa);retarget.Update_0024A1A8(0.25f);
    check(retarget_screen.widgets[0].y==51.5f);
    retarget.Handle_0024A480(4,0x52e039fb);retarget.Update_0024A1A8(0);
    check(retarget_screen.widgets[0].y==51.5f);
    retarget.Update_0024A1A8(0.25f);check(retarget_screen.widgets[0].y==16.8125f);
    retarget.Update_0024A1A8(0.25f);check(retarget_screen.widgets[0].y==-4);
    tLeftRightArrowScroll edge_selector;edge_selector.Construct_0024A690(retarget_screen,0xd1cda594,0x9345e4a7);
    MenuEvent_0016A458 edge;edge.field_0x00=0x52e039f3;
    Sub_001EFB80(8,edge,retarget,edge_selector,retarget_screen,{});check(edge.result_0x08==1);
    edge={};edge.field_0x00=0xaee77884;
    Sub_001EFB80(9,edge,retarget,edge_selector,retarget_screen,{});check(edge.result_0x08==1);
    FeWidget ghost;CopyWidgetTransform_001410A0(ghost,screen.widgets[0]);
    check(ghost.clip_hash_0x4c==0xc67cfb4a);
    ghost.text_align=0;check(!GetWidgetClip_00145F30(screen,ghost,{}).valid);
    FeWidget mono;mono.type=1;mono.name_hash=0x2932dfca;mono.y=329.75f;
    FeWidget left;left.type=2;left.name_hash=0xd1cda594;left.x=410;left.visible=1;
    FeWidget right=left;right.name_hash=0x9345e4a7;right.x=609;
    FeScreen selector_screen;selector_screen.widgets={mono,left,right};
    tLeftRightArrowScroll selector;selector.Construct_0024A690(selector_screen,0xd1cda594,0x9345e4a7);
    check(selector_screen.widgets[1].visible==0&&selector_screen.widgets[2].visible==0);
    selector.Bind_0024A860(0x2932dfca);
    auto measure=[](const FeWidget&w){return WidgetMetrics{10,w.type==1?28.0f:16.0f};};
    selector.Update_0024A730(0,measure);
    check(selector_screen.widgets[1].y==335&&selector_screen.widgets[2].y==335);
    check(selector_screen.widgets[1].x==410&&selector_screen.widgets[2].x==609);
    selector_screen.widgets[0].y=182.25f;selector.Update_0024A730(0,measure);
    check(selector_screen.widgets[1].y==188);
    selector.Bind_0024A860(0);check(selector_screen.widgets[1].visible==0);
    FeScreen display;
    for(int i=1;i<=9;++i){FeWidget w;w.type=1;w.name_hash=0x52e039f2u+i;
        w.y=70+(i-1)*37;display.widgets.push_back(w);}
    for(auto hash:{0x55cb09f2u,0x2e931f1bu,0x03d90172u,0x5610f2f4u,0x8fe4086bu,0x422313cbu}){
        FeWidget w;w.name_hash=hash;w.type=1;w.y=70;display.widgets.push_back(w);}
    display.widgets.push_back(left);display.widgets.push_back(right);
    tScrollingMenuList display_viewport;display_viewport.Sub_001F33B8(display);
    check(display_viewport.field_0x34==9);
    for(const auto&w:display.widgets)check(w.clip_hash_0x4c==0xc67cfb4a);
    display_viewport.Handle_0024A480(4,0x52e039fb);
    display_viewport.Update_0024A1A8(0.5f);
    check(display.widgets[8].y==292&&display.widgets[11].y==-4);
    std::cout<<"FRONTEND_VIEWPORT_OK\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
