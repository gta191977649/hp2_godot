#include "game_core/indep/src/frontendscreens/options.hpp"
#include "game_core/port/not_implemented.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

int main(){try {
    using namespace port::frontend;
    const auto check=[](bool v){if(!v)throw std::runtime_error("Back behavior mismatch");};
    BackContext c;c.settings_changed=false;c.keyboard_active=false;c.language=0;c.language_instance_0x08=1;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> calls;std::vector<std::string> history;
    BackServices services{[&](auto address,auto arg){calls.emplace_back(address,arg);},[&](const char* name){history.emplace_back(name);}};
    for(auto handler:{0x001F0278u,0x001F3EC0u,0x00240060u,0x0024DBD8u,0x001F19E8u,0x001F21B0u,0x002174A0u,0x00217AF8u,0x00247830u}) {
        auto event=query_back(handler,c,services);check(event.permission==1&&event.sound==6);
    }
    check(calls.empty()&&history.empty());
    c.exit_active=true;
    for(auto handler:{0x001F0278u,0x001F3EC0u,0x00240060u,0x001F1220u,0x002174A0u,0x00217AF8u}) {
        auto event=query_back(handler,c,services);check(event.permission==0&&event.sound==0);
    }
    check(calls.empty());c.exit_active=false;
    c.settings_changed=true;c.autosave=1;
    query_back(0x001F0278,c,services);check(calls.size()==1&&calls[0]==std::make_pair(0x001EFB80u,0x11u));check(history==std::vector<std::string>{"Autosave"});
    calls.clear();history.clear();c.display_field_0x64={1,2,3,4,5,6,7,8};
    query_back(0x001F3EC0,c,services);check(c.dat_002FC19C==c.display_field_0x64&&history.size()==1&&calls.empty());
    history.clear();query_back(0x00240060,c,services);check(history.size()==1);
    c.settings_changed=false;c.keyboard_active=true;calls.clear();history.clear();
    auto event=query_back(0x0024DBD8,c,services);check(event.permission==0&&event.sound==29&&!*c.keyboard_active);
    check(calls==std::vector<std::pair<std::uint32_t,std::uint32_t>>{{0x0024D088,0x11},{0x0016D448,0}});
    calls.clear();c.selected_hash=0x52E039F3;
    event=query_back(0x001F1DD8,c,services);check(event.permission==0&&event.sound==6);
    check(calls==std::vector<std::pair<std::uint32_t,std::uint32_t>>{{0x0021E0D0,0},{0x00170320,1}});
    calls.clear();c.language_instance_0x08=0;c.selected_hash=0x52E039F4;
    query_back(0x001F1DD8,c,services);check(calls.size()==3&&calls[1]==std::make_pair(0x00183830u,0x002FC050u)&&*c.language==1);
    calls.clear();c.language=-1;event=query_back(0x001F1DD8,c,services);check(event.permission==0&&calls.empty());
    c.language=0;c.settings_changed=false;query_back(0x001F1220,c,services);
    check(calls==std::vector<std::pair<std::uint32_t,std::uint32_t>>{{0x001F0D10,0x11},{0x001AF050,5},{0x0027D640,1}});
    const auto throws=[&](auto action){bool threw=false;try{action();}catch(const hp2::NotImplemented&){threw=true;}check(threw);};
    c.settings_changed.reset();throws([&]{query_back(0x001F0278,c,services);});
    c.loadsave_state_0x54=1;throws([&]{query_back(0x00247830,c,services);});
    throws([&]{query_back(0xffffffff,c,services);});
    c.settings_changed=true;throws([&]{query_back(0x001F0278,c,{});});
    std::cout<<"FRONTEND_BACK_OK\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
