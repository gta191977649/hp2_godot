#include "port/runtime/frontend_runtime.hpp"
#include "game_core/port/input/joyevents.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>
namespace {void require(bool b,const std::string& s){if(!b)throw std::runtime_error(s);}}
int main(){
    port::frontend::NameEntryState state;
    std::memcpy(state.names[0].data(),"ABCDEFG",8);
    hp2::frontend::Sub_0024C708(state,0);
    hp2::frontend::MenuEvent_0016A458 e;e.pad_index_0x04=1;
    hp2::frontend::Sub_0024D088(state,1,e);
    require(state.buffer[0]=='A' && state.cursor==0 && e.sound_id_0x0C==0,"wrong pad edited keyboard");
    e.pad_index_0x04=0;
    hp2::frontend::Sub_0024D088(state,6,e);
    require(state.column==13,"keyboard column left wrap differs");
    hp2::frontend::Sub_0024D088(state,8,e);
    require(state.row==2,"keyboard row up wrap differs");
    state.column=12;state.cursor=2;
    hp2::frontend::Sub_0024D088(state,1,e);
    require(std::string(state.buffer.data())=="ABDEFG " && state.cursor==1 && e.sound_id_0x0C==26,"delete must shift current position then move left");
    state.row=0;state.column=0;
    hp2::frontend::Sub_0024D088(state,10,e);hp2::frontend::Sub_0024D088(state,1,e);
    require(state.buffer[1]=='A' && state.cursor==2,"case toggle/overwrite differ");
    state.cursor=6;hp2::frontend::Sub_0024D088(state,16,e);
    require(state.cursor==6,"name cursor must clamp at6");
    e.pad_index_0x04=1;hp2::frontend::Sub_0024D088(state,17,e);
    require(!state.active && std::string(state.names[0].data())=="AADEFG " && e.sound_id_0x0C==29,"Back must commit, including original ungated Back");
    state.row=2;state.column=13;hp2::frontend::Sub_0024C708(state,0);
    require(state.row==2 && state.column==13 && !state.uppercase && state.cursor==0,"reopen must preserve grid and reset case/cursor");
    e.pad_index_0x04=0;require(!hp2::frontend::Sub_0024D088(state,1,e),"Done key must close editor");
    e.field_0x00=0x52e039f6;hp2::frontend::Sub_0024D660(state,7,e);
    require(state.flags[1]==0,"P1 pad toggled P2 gender");e.pad_index_0x04=1;hp2::frontend::Sub_0024D660(state,7,e);
    require(state.flags[1]==1 && e.sound_id_0x0C==2,"P2 gender gate differs");
    if(!hp2::platform::resolve_zzdata_root(".")){std::puts("PASS keyboard; SKIP original assets");return 0;}
    port::runtime::FrontEndRuntime runtime;auto loaded=runtime.load(".","FRONTEND/FRONTB.BUN");require(loaded.ok,loaded.error);
    runtime.load_fonts({"FRONTEND/FRONTB.BUN","GLOBAL/GLOBALB.BUN","GLOBAL/INGAMEB.BUN"});
    auto press=[&](int id,int pad=0){auto r=runtime.menu_joy_event(pad,id,true);require(r.not_implemented.empty(),r.not_implemented);return r;};
    auto tick=[&](double d){auto r=runtime.menu_tick(d);require(r.not_implemented.empty(),r.not_implemented);};
    auto opened=runtime.menu_open(24);require(opened.not_implemented.empty(),opened.not_implemented);
    for(int i=0;i<4;++i)press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    press(hp2::input::JOY_EVENT_MENU_SELECT);tick(1);
    require(runtime.menu_state().active_class=="NameEntry","Options NameEntry route failed");
    const int document=runtime.menu_state().screen;
    const auto widget=[&](std::uint32_t hash){return hp2::frontend::FindWidget_00143D50(*const_cast<hp2::frontend::FeScreen*>(runtime.screen(document)),hash,0);};
    require(widget(0xc01086b5) && !widget(0xc01086b5)->visible,"caret must start hidden");
    press(hp2::input::JOY_EVENT_MENU_SELECT,1);require(!widget(0xc01086b5)->visible,"wrong pad opened P1 keyboard");
    press(hp2::input::JOY_EVENT_MENU_SELECT);for(int i=0;i<72;++i)tick(1.0/120);
    require(widget(0xc01086b5)->visible && !widget(0x71450a81)->visible,"keyboard visibility binding differs");
    require(widget(hp2::frontend::bStringHash("CHAR_00"))->text=="a" &&
        widget(hp2::frontend::bStringHash("CHAR_0D"))->text=="n", "authored keyboard rows were not populated");
    const auto initial_current=runtime.menu_state().current;
    press(hp2::input::JOY_EVENT_SCROLL_DOWN);press(hp2::input::JOY_EVENT_MENU_SELECT);
    require(runtime.menu_state().current==initial_current,"keyboard directional input leaked to outer list");
    press(hp2::input::JOY_EVENT_MENU_BACK);
    require(runtime.menu_state().active_class=="NameEntry" && !widget(0xc01086b5)->visible && runtime.profile().read_u8(0x10d90)=='o',"keyboard Back must commit first, without leaving screen");
    press(hp2::input::JOY_EVENT_SCROLL_DOWN);press(hp2::input::JOY_EVENT_SCROLL_RIGHT);
    require(runtime.profile().read_u8(0x10d83)==1,"gender toggle not committed");
    press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    press(hp2::input::JOY_EVENT_MENU_SELECT,0);require(!widget(0xc01086b5)->visible,"P1 pad opened P2 keyboard");
    press(hp2::input::JOY_EVENT_MENU_SELECT,1);tick(0.6);
    press(hp2::input::JOY_EVENT_SCROLL_RIGHT,1);press(hp2::input::JOY_EVENT_MENU_SELECT,1);
    press(hp2::input::JOY_EVENT_MENU_BACK,1);
    require(runtime.profile().read_u8(0x11460)=='p' && runtime.profile().read_u8(0x10d90)=='o',"P2 name commit leaked across player records");
    press(hp2::input::JOY_EVENT_MENU_BACK);require(runtime.menu_state().active_class=="Options","outer NameEntry Back did not pop history");
    std::puts("PASS NameEntry keyboard, pad gates, delete/case, authored binding and profile commit");
}
