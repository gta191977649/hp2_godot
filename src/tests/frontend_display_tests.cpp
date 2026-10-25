#include "port/runtime/frontend_runtime.hpp"
#include "game_core/port/input/joyevents.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {void require(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}}
int main() {
    require(hp2::frontend::bSin(0.00001f)==0 && std::abs(hp2::frontend::bSin(1.57079637f)-1.0f)<0.00001f,
        "frontend sine must retain original angle quantization and cardinal polynomial");
    port::frontend::DisplayOptionsState display;
    display.snapshot={1,0,0,7,8,2,1,1};
    hp2::frontend::MenuEvent_0016A458 event;event.field_0x00=0x52e039f3;
    hp2::frontend::Sub_001F3A28(display,6,event,{});
    require(display.snapshot[2]==6 && display.snapshot[3]==6 && display.snapshot[4]==6,"map-style left wrap and derived secondary fields differ");
    hp2::frontend::Sub_001F3A28(display,7,event,{});
    require(display.snapshot[2]==0 && display.snapshot[3]==7 && display.snapshot[4]==8,"map-style right wrap differs");
    display.snapshot[2]=99;hp2::frontend::Sub_001F3A28(display,6,event,{});
    require(display.snapshot[2]==0,"unknown map value must choose first table entry independently of direction");
    port::frontend::DisplayOffsetState offset;int sound;
    for(int i=0;i<200;++i)hp2::frontend::Sub_001F4B20(offset,6,sound);
    require(offset.x==-650 && sound==0,"calibration X lower clamp differs");
    for(int i=0;i<300;++i)hp2::frontend::Sub_001F4B20(offset,7,sound);
    require(offset.x==550 && sound==1,"calibration Right must not invent an upper clamp");
    for(int i=0;i<60;++i)hp2::frontend::Sub_001F4B20(offset,8,sound);
    require(offset.y==-50 && sound==0,"calibration Y lower clamp differs");
    hp2::frontend::Sub_001F4B20(offset,14,sound);
    require(offset.x==0 && offset.y==0 && sound==5,"calibration must restore cached entry offsets");
    port::frontend::HudOptionsState hud;event.field_0x00=0x52e039f8;
    hp2::frontend::Sub_001F46D8(hud,6,event);
    require(hud.snapshot[6]==4 && (hud.snapshot[0x10]&4)==0,"HUD split-time mode0 left must wrap to Off and clear mask4");
    hp2::frontend::Sub_001F46D8(hud,7,event);
    require(hud.snapshot[6]==0 && (hud.snapshot[0x10]&4)!=0,"HUD split-time Off right must wrap and enable mask4");
    if(!hp2::platform::resolve_zzdata_root(".")){std::puts("PASS Display/HUD/calibration projection; SKIP original assets");return 0;}
    port::runtime::FrontEndRuntime runtime;const auto loaded=runtime.load(".","FRONTEND/FRONTB.BUN");require(loaded.ok,loaded.error);
    runtime.load_fonts({"FRONTEND/FRONTB.BUN","GLOBAL/GLOBALB.BUN","GLOBAL/INGAMEB.BUN"});
    const auto press=[&](int id){const auto state=runtime.menu_joy_event(0,id,true);require(state.not_implemented.empty(),state.not_implemented);return state;};
    const auto tick=[&]{const auto state=runtime.menu_tick(1);require(state.not_implemented.empty(),state.not_implemented);};
    runtime.menu_open(24);press(hp2::input::JOY_EVENT_SCROLL_DOWN);press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    auto state=press(hp2::input::JOY_EVENT_MENU_SELECT);require(state.exiting,"Display must use original Options exit animation");tick();
    require(runtime.menu_state().active_class=="Display Options","Display preparation/entry failed");
    press(hp2::input::JOY_EVENT_SCROLL_RIGHT);
    const auto* map=hp2::frontend::FindWidget_00143D50(*const_cast<hp2::frontend::FeScreen*>(runtime.screen(26)),0x55cb09f2,1);
    require(map && map->string_id_0x60==0x22346,"Display must show map style3D Standard after Right");
    for(int i=0;i<6;++i)press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    press(hp2::input::JOY_EVENT_MENU_SELECT);tick();
    require(runtime.menu_state().active_class=="Customize P1 HUD","P1 HUD route failed");
    press(hp2::input::JOY_EVENT_SCROLL_DOWN);press(hp2::input::JOY_EVENT_SCROLL_DOWN);press(hp2::input::JOY_EVENT_SCROLL_RIGHT);
    press(hp2::input::JOY_EVENT_MENU_BACK);
    require(runtime.menu_state().active_class=="Display Options" && (runtime.profile().read_u32(0x10d8c)&0x40)!=0,
        "HUD Back/destruction must commit full P1 snapshot before returning");
    for(int i=0;i<7;++i)press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    press(hp2::input::JOY_EVENT_MENU_SELECT);tick();
    require(runtime.menu_state().active_class=="Customize P2 HUD","shared HUD document lost P2 class identity");
    press(hp2::input::JOY_EVENT_SCROLL_RIGHT);press(hp2::input::JOY_EVENT_MENU_BACK);
    require(runtime.profile().read_u32(0x11458)==1 && runtime.profile().read_u32(0x10d88)==0,"P2 HUD change leaked into P1 record");
    for(int i=0;i<8;++i)press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    press(hp2::input::JOY_EVENT_MENU_SELECT);tick();
    require(runtime.menu_state().active_class=="Calibrate display offset","calibration route failed");
    press(hp2::input::JOY_EVENT_SCROLL_RIGHT);press(hp2::input::JOY_EVENT_SCROLL_UP);
    require(runtime.profile().read_i32(0x144)==4 && runtime.profile().read_i32(0x148)==-1,"calibration raw profile offsets differ");
    port::render::Frame frame;runtime.build_frame(*runtime.catalog(),frame);
    require(frame.display_offset.x==1 && frame.display_offset.y==-1,"host display translation did not consume calibration request");
    press(hp2::input::JOY_EVENT_SELECT);
    require(runtime.profile().read_i32(0x144)==0 && runtime.profile().read_i32(0x148)==0,"calibration reset did not restore cached offsets");
    press(hp2::input::JOY_EVENT_MENU_BACK);press(hp2::input::JOY_EVENT_MENU_BACK);
    require(runtime.menu_state().active_class=="Options" && runtime.profile().read_u8(0x14e)==1,"Display Back/history or map commit failed");
    std::puts("PASS original Display entry, map style, P1/P2 HUD destruction commit and calibration routes/bounds");
}
