#include "game_core/port/assets/fe_language.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    try {
        auto check=[](bool b){if(!b)throw std::runtime_error("scroll/localisation mismatch");};
        std::vector<std::uint8_t> bytes(132,0);
        auto word=[&](std::size_t p,std::uint32_t x){for(int i=0;i<4;++i)bytes[p+i]=std::uint8_t(x>>(i*8));};
        word(0,0x39000);word(4,124);word(0x54,1);word(0x58,1);word(0x5c,8);
        word(108,0x111);word(112,0);word(116,0x2720);word(120,0);
        bytes[124]='H';bytes[125]='i';
        hp2::port::assets::FeLanguage language(bytes);std::string text;
        check(language.lookup(0x2720,text)&&text=="Hi");check(!language.lookup(0x2721,text));
        check(language.lookup_hash(0x111,text)&&text=="Hi");
        check(!language.lookup_hash(0x112,text));
        word(120,8);bool bad=false;try{hp2::port::assets::FeLanguage invalid(bytes);}catch(const std::runtime_error&){bad=true;}check(bad);
        using namespace hp2::frontend;
        FeScreen screen;FeWidget scroll;scroll.type=1;scroll.name_hash=0x75980cf0;
        scroll.x=10;scroll.y=400;scroll.a=128;scroll.text="This text will scroll";
        FeWidget bar;bar.name_hash=0x70a404ec;bar.a=128;screen.widgets={scroll,bar};
        tAnimatedBlurb state;state.Construct_002491B0(screen);check(screen.widgets[0].x==650);
        state.Bind_00249518("First");state.Update_00249348(0.01f);
        check(screen.widgets[0].text=="First"&&screen.widgets[0].x==650&&screen.widgets[1].a==0);
        state.Update_00249348(0.125f);check(screen.widgets[0].x==330);
        state.Update_00249348(0.125f);check(screen.widgets[0].x==10);
        state.Update_00249348(0);state.Bind_00249518("Second");state.Update_00249348(0.3f);
        check(screen.widgets[0].a==64&&screen.widgets[1].a==64&&screen.widgets[0].text=="First");
        state.Update_00249348(0.21f);check(screen.widgets[0].text=="Second"&&screen.widgets[0].x==650);
        check(screen.widgets[0].y==400&&screen.widgets[0].string_id_0x60==0);
        std::uint32_t rng=0x12345678;
        check(bRandom(10000,rng)==9896); // unsigned DIVU old state
        check(port::frontend::hint_binding("Audio Options",0x52e039fbu).string_id==0x24bec);
        check(port::frontend::hint_binding("Audio Options",0xaee77883u).string_id==0x24bed);
        check(port::frontend::hint_binding("Joystick PreConfig",0x52e039f8u).string_id==0x24c4d);
        check(port::frontend::hint_binding("NameEntry",0x52e039f6u).string_id==0x24d13);
        check(!port::frontend::hint_binding("Username Options",0x52e039f3u).attachment_known);
        check(port::frontend::hint_binding("Language Options",0).attachment_known);
        check(!port::frontend::hint_binding("Credits",0).attached);
        rng=0xffffffff;check(bRandom(10000,rng)==7295);
        FeWidget fading;fading.type=2;fading.name_hash=bStringHash("FADING_BP_OBJECT");
        FeWidget master;master.type=2;master.name_hash=bStringHash("MASTER_BP_OBJECT");
        screen.widgets={fading,master};tGlobalBackgroundAnimation background;
        background.Construct_002495F0();auto random=[](std::int32_t){return 0;};
        background.Bind_00249A40(&screen,random,[](const FeWidget&){return WidgetMetrics{20,40};});
        check(screen.widgets[0].x==150&&screen.widgets[0].y==160&&screen.widgets[0].a==0);
        check(screen.widgets[0].texture_hash==bStringHash("GRID_BLUEPRINT1"));
        background.Update_00249AE8(0.5f,random);check(screen.widgets[0].a==64);
        background.Update_00249AE8(24.5f,random);check(screen.widgets[0].x==220);
        background.Update_00249AE8(24.5f,random);check(screen.widgets[0].a==64);
        background.Update_00249AE8(0.6f,random);check(screen.widgets[0].x==150&&screen.widgets[0].a==0);
        std::cout<<"FRONTEND_SCROLL_OK\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
