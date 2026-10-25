#include "port/runtime/frontend_runtime.hpp"
#include "game_core/port/input/joyevents.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cstdio>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace {
void require(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
const hp2::frontend::FeWidget& widget(const port::runtime::FrontEndRuntime& runtime,std::uint32_t hash) {
    const auto* screen=runtime.screen(runtime.menu_state().screen);
    for(const auto& w:screen->widgets)if(w.name_hash==hash)return w;
    throw std::runtime_error("missing original Jukebox widget "+std::to_string(hash)+" screen "+std::to_string(runtime.menu_state().screen));
}
struct Env {
    std::string key,prior;bool existed;
    Env(std::string k,const std::string& value):key(std::move(k)) {
        const auto* old=std::getenv(key.c_str());existed=old!=nullptr;if(old)prior=old;
#ifdef _WIN32
        _putenv_s(key.c_str(),value.c_str());
#else
        setenv(key.c_str(),value.c_str(),1);
#endif
    }
    ~Env(){
#ifdef _WIN32
        _putenv_s(key.c_str(),existed?prior.c_str():"");
#else
        if(existed)setenv(key.c_str(),prior.c_str(),1);else unsetenv(key.c_str());
#endif
    }
};
}
int main() {
    if(!hp2::platform::resolve_zzdata_root(".")){std::puts("SKIP original assets not configured");return 0;}
    port::runtime::FrontEndRuntime runtime;
    const auto loaded=runtime.load(".","FRONTEND/FRONTB.BUN");
    require(loaded.ok,loaded.error);
    // E-0092-J08: polling is not001AEAB0. Intro, a skipped second intro's
    // static Title, and unsolicited device completion must remain silent.
    runtime.menu_open(22);
    runtime.set_startup_presentation(true,false,true);
    for(int i=0;i<3;++i)require(!runtime.frontend_music(false).has_track,"intro poll started BGM");
    runtime.set_startup_presentation(false,false,true);
    require(!runtime.frontend_music(false).has_track,"skipped intro Title poll started BGM");
    require(!runtime.frontend_music(true).has_track,"idle completion started BGM");
    int jukebox=-1;
    for(int i=0;i<runtime.screen_count();++i)if(runtime.screen(i)->name_hash_0x08==0x95b2b627)jukebox=i;
    require(jukebox>=0,"registered Jukebox document missing");
    auto menu=runtime.menu_open(jukebox);
    require(menu.ok && menu.not_implemented.empty(),"original Jukebox construction failed: "+menu.not_implemented);
    require(widget(runtime,0x64a819c7).text!="Band Name" && !widget(runtime,0x64a819c7).text.empty(),
        "first Jukebox row still has authored placeholder");
    require(widget(runtime,0x64a819c7).string_id_0x60==0,"dynamic song row retained authored string ID");
    require(widget(runtime,0xd1cda594).visible==1,"category focus arrow missing");
    const auto press=[&](int event){auto state=runtime.menu_joy_event(0,event,true);require(state.not_implemented.empty(),state.not_implemented);return state;};
    auto music=runtime.frontend_music(false);
    require(music.ok && !music.has_track,"direct Jukebox entry must not invent a BGM start call");
    music=runtime.frontend_music(false,true);
    require(music.ok && music.has_track && music.index==5,"explicit initial preferred frontend song differs");
    press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    require(widget(runtime,0xd1cda594).visible==0 && widget(runtime,0x0b825845).visible==1,
        "song focus arrows do not replace category arrows");
    auto tick=runtime.menu_tick(0.5);require(tick.not_implemented.empty(),tick.not_implemented);
    music=runtime.frontend_music(false);require(!music.has_track,"preview fired at exact0.5 boundary");
    tick=runtime.menu_tick(0.001);require(tick.not_implemented.empty(),tick.not_implemented);
    music=runtime.frontend_music(false);
    require(music.ok && music.has_track && music.stop && music.index==0 && !music.pcm.samples.empty(),
        "selected original preview was not decoded/replaced");
    music=runtime.frontend_music(true);
    require(music.ok && !music.has_track,"preview auto-mode0 must not naturally advance");
    // Authored first-song mode2 -> mode3; mode3 must be frontend-eligible.
    press(hp2::input::JOY_EVENT_SCROLL_RIGHT);
    require(hp2::port::audio::Sub_001AF050(3,true),"original frontend matrix excludes mode3");
    press(hp2::input::JOY_EVENT_SCROLL_UP);
    music=runtime.frontend_music(false);
    require(music.ok && music.stop && !music.has_track,"category return did not stop preview");
    for(int i=0;i<3;++i)require(!runtime.frontend_music(false).has_track,"poll resurrected stopped preview");
    require(!runtime.frontend_music(true).has_track,"stale completion resurrected stopped preview");
    press(hp2::input::JOY_EVENT_MENU_BACK);
    music=runtime.frontend_music(false);
    require(music.ok && music.has_track && music.index==5,"Back must resume preferred song5, not highlighted song0");
    require(runtime.profile().read_u8(0x94+0x1e)==3,"Back did not commit per-song mode into original profile block");
    music=runtime.frontend_music(true);
    require(music.ok && music.has_track && music.index==7,"resumed automatic music must advance naturally");
    runtime.menu_open(28);
    for(int i=0;i<8;++i)press(hp2::input::JOY_EVENT_SCROLL_DOWN);
    auto routed=press(hp2::input::JOY_EVENT_MENU_SELECT);
    require(routed.exiting,"Audio Jukebox entry must use original exit animation");
    tick=runtime.menu_tick(1.0);require(tick.not_implemented.empty(),tick.not_implemented);
    require(runtime.menu_state().screen==jukebox,"Audio entry did not reach registered Jukebox document");
    require(widget(runtime,0x89fb635f).string_id_0x60==0x1addb,"Jukebox reentry did not preserve changed mode3");
    press(hp2::input::JOY_EVENT_MENU_L1);
    require(widget(runtime,0x89fb635f).string_id_0x60==0x1add9,"disabled genre must display original mode0 text");
    require(widget(runtime,0x64a819c7).r==0x18,"disabled genre row colour differs");
    press(hp2::input::JOY_EVENT_MENU_BACK);
    require(runtime.menu_state().screen==28,"Jukebox Back must return through real Audio history");
    // Isolated host-save fixture: neither input nor output points at the user's save.
    const auto folder=std::filesystem::absolute("local/task0091/autosave-fixture");
    std::filesystem::create_directories(folder);
    auto profile=runtime.profile();profile.write_u32(4,1);
    const auto payload=hp2::memorycard::Store_00245738(profile);
    const auto input=folder/"input.db";
    {std::ofstream out(input,std::ios::binary);out.write(reinterpret_cast<const char*>(payload.data()),payload.size());require(bool(out),"save fixture write failed");}
    Env input_env("HP2_PS2_PROFILE_PATH",input.string());
    Env output_env("HP2_PORT_SAVE_ROOT",(folder/"output").string());
    port::runtime::FrontEndRuntime saving;
    require(saving.load(".","FRONTEND/FRONTB.BUN").ok,"isolated saved-profile load failed");
    saving.menu_open(28);
    for(int i=0;i<8;++i)saving.menu_joy_event(0,hp2::input::JOY_EVENT_SCROLL_DOWN,true);
    saving.menu_joy_event(0,hp2::input::JOY_EVENT_MENU_SELECT,true);saving.menu_tick(1);
    saving.menu_joy_event(0,hp2::input::JOY_EVENT_MENU_L1,true);
    menu=saving.menu_joy_event(0,hp2::input::JOY_EVENT_MENU_BACK,true);
    require(menu.not_implemented.empty() && menu.active_class=="Autosave","changed Jukebox must route through Autosave history");
    require(saving.menu_joy_event(0,hp2::input::JOY_EVENT_MENU_BACK,true).active_class=="Autosave",
        "Autosave flags9 must consume Back");
    saving.menu_tick(1.0);saving.menu_tick(0.001);saving.menu_tick(0);
    const auto output=hp2::platform::profile_store_path(".",hp2::memorycard::kRegionNtsc_001A3390);
    require(std::filesystem::exists(output),"Autosave did not use host store boundary");
    saving.menu_tick(2.0);saving.menu_tick(0.001);saving.menu_tick(2.0);saving.menu_tick(0.001);
    require(saving.menu_state().active_class=="Audio Options","Autosave completion must pop back to Audio Options");
    std::ifstream unchanged(input,std::ios::binary);
    const std::vector<std::uint8_t> input_after{std::istreambuf_iterator<char>(unchanged),std::istreambuf_iterator<char>()};
    require(input_after==payload,"Autosave modified the read-only input profile");
    std::puts("PASS original Jukebox widget binding, mode/genre interaction, preview, stop, Back and automatic completion");
}
