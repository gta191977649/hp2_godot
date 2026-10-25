#include "port/runtime/frontend_runtime.hpp"
#include "game_core/port/input/joyevents.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>

namespace {
void require(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
bool near(float a,float b){return std::abs(a-b)<0.00001f;}
float value(const std::array<std::uint8_t,0xb0>& data,std::size_t index) {
    std::uint32_t bits=0;for(unsigned b=0;b<4;++b)bits|=std::uint32_t(data[index*4+b])<<(b*8);
    float out;std::memcpy(&out,&bits,4);return out;
}
}
int main() {
    port::frontend::ScalarPath path;path.target=1;path.phase=2;
    hp2::frontend::Sub_001DAEC8(path,0.125f);
    require(near(path.current,0.5f) && near(path.velocity,1.5f),"original cubic position/derivative differ");
    path.target=0;path.phase=2;
    hp2::frontend::Sub_001DAEC8(path,0);
    require(near(path.current,0.5f) && near(path.velocity,1.5f),"retarget must preserve current derivative");
    hp2::frontend::Sub_001DAEC8(path,0.125f);
    require(near(path.current,0.4375f),"midflight retarget used a zero-velocity substitute");
    hp2::frontend::Sub_001DAEC8(path,0.125f);
    require(path.phase==0 && path.current==0 && path.velocity==0,"path endpoint must publish target and derivative");
    std::array<std::uint8_t,0xb0> snapshot{};
    constexpr std::uint32_t defaults[]{0x3ee66666,0x3f59999a,0x3ec28f5c,0x3f0ccccd,0x3ea8f5c3,0x3e9eb852,0x3f400000};
    for(std::size_t i=0;i<7;++i)for(unsigned b=0;b<4;++b)snapshot[i*4+b]=static_cast<std::uint8_t>(defaults[i]>>(b*8));
    snapshot[0xac]=1;
    port::frontend::AudioOptionsState state;
    hp2::frontend::Sub_001EF3C8(state,snapshot);
    require(near(state.sliders[0].current,0.7f),"default amplitude must normalize to0.7");
    hp2::frontend::Sub_0016DD38(state,0,0.05f);
    require(near(value(state.snapshot,0),0.5f),"slider must add raw0.05 before normalization");
    for(int i=0;i<30;++i)hp2::frontend::Sub_0016DD38(state,0,0.05f);
    require(near(value(state.snapshot,0),0.45f/0.7f),"slider upper bound must be category-default/0.7, not1");
    for(int i=0;i<30;++i)hp2::frontend::Sub_0016DD38(state,0,-0.05f);
    require(value(state.snapshot,0)==0,"slider lower bound differs");
    require(!hp2::frontend::Sub_0016DD38(state,0,-0.05f),"clamped slider must not emit changed-value feedback");
    hp2::frontend::Sub_001EF3C8(state,snapshot);
    hp2::frontend::FeScreen document;
    hp2::frontend::FeWidget track;track.type=2;track.name_hash=0xa29f7a16;track.x=10;
    hp2::frontend::FeWidget marker;marker.type=2;marker.name_hash=0xc866642b;marker.y=73;marker.scale_x=2;
    document.widgets={track,marker};
    hp2::frontend::Sub_0016DE30(state,document,0,[](const auto&){return hp2::frontend::WidgetMetrics{100,10};},[](auto){return std::string{};});
    require(document.widgets[1].x==80 && document.widgets[1].y==73 && document.widgets[1].scale_x==2,
        "slider must move marker X while retaining authored Y and scale");
    if(!hp2::platform::resolve_zzdata_root(".")){std::puts("PASS scalar/audio projection; SKIP original assets");return 0;}
    port::runtime::FrontEndRuntime runtime;
    const auto loaded=runtime.load(".","FRONTEND/FRONTB.BUN");require(loaded.ok,loaded.error);
    auto menu=runtime.menu_open(28);require(menu.not_implemented.empty(),menu.not_implemented);
    const auto press=[&](int event){auto result=runtime.menu_joy_event(0,event,true);require(result.not_implemented.empty(),result.not_implemented);return result;};
    press(hp2::input::JOY_EVENT_SCROLL_RIGHT);
    const auto music=runtime.frontend_music(false);require(music.ok && near(music.gain,0.5f),"music slider is not applied live");
    const auto audio=runtime.drain_menu_audio();
    bool feedback=false;for(const auto& request:audio)if(request.event=="MenuAdjustVolume")feedback=true;
    require(feedback,"changed slider did not queue original volume feedback event");
    for(int i=0;i<7;++i)press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    press(hp2::input::JOY_EVENT_SCROLL_RIGHT);
    const auto* screen=runtime.screen(28);
    const auto* output=hp2::frontend::FindWidget_00143D50(*const_cast<hp2::frontend::FeScreen*>(screen),0x2932dfca,1);
    require(output && output->string_id_0x60==0x1adbb,"output mode2 string differs");
    press(hp2::input::JOY_EVENT_SCROLL_RIGHT);
    require(output->string_id_0x60==0x1adb9,"output mode must wrap2 to0");
    press(hp2::input::JOY_EVENT_SCROLL_DOWN);press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    press(hp2::input::JOY_EVENT_MENU_SELECT);
    require(near(runtime.frontend_music(false).gain,0.45f),"restore volume defaults did not affect live music");
    press(hp2::input::JOY_EVENT_SCROLL_DOWN);press(hp2::input::JOY_EVENT_MENU_SELECT);
    press(hp2::input::JOY_EVENT_MENU_BACK);
    require(runtime.profile().read_u32(0x94+0xa4)==0 && runtime.profile().read_u8(0x94+0xaf)==0xff,
        "restore music defaults must reset preferred index0 and genre maskFF");
    require(runtime.profile().read_u8(0x94+0xac)==0,"Audio Back did not commit output mode");
    std::puts("PASS original Audio sliders, cubic retargeting, marker geometry, output cycle, feedback and defaults/Back");
}
