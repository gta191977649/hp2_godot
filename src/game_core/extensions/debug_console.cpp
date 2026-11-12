#include "debug_console.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include <imgui_console.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <mutex>
#include <sstream>
#include <stdexcept>
namespace port::extensions {
struct DebugConsole::Impl {
    struct Console : ImGuiConsole {
        Console(): ImGuiConsole("HP2 Debug Console") {}
        bool focus = false;
        void InputBar() override;
    };
    std::unique_ptr<DebugUiSession> ui;
    std::unique_ptr<Console> console;
    std::string settings;
    bool open = false, details = false, toggle_down = false, enabled = true, viewer_input = false;
    Snapshot state;
    // Port-side wall frame intervals, independent of the smoothed status sample.
    std::array<float,240> frame_ms{};
    int frame_count = 0, frame_next = 0;
    std::mutex mutex;
    std::deque<std::pair<std::string,csys::ItemType>> pending;
    ~Impl() {
        if (!ui) return;
        current();
        // Settings handlers contain pointers to Console; save before destroying it.
        ui->save_settings();
        ImGui::GetIO().IniFilename=nullptr;
        console.reset();

    }
    void current() { ui->current(); }
    void trim() {
        auto& items = console->System().Items();
        if (items.size()>log_capacity) items.erase(items.begin(),items.end()-log_capacity);
    }
    void drain() {
        std::deque<std::pair<std::string,csys::ItemType>> incoming;
        { std::lock_guard<std::mutex> lock(mutex); incoming.swap(pending); }
        for (auto& entry:incoming)
            console->System().Log(entry.second) << entry.first << csys::endl;
        trim();
    }
};
void DebugConsole::Impl::Console::InputBar() {
    if (focus) { ImGui::SetKeyboardFocusHere(); focus=false; }
    ImGuiConsole::InputBar();
}
DebugConsole::DebugConsole(std::string settings):impl_(std::make_unique<Impl>()) {
    auto& p=*impl_;
    p.ui=std::make_unique<DebugUiSession>(settings);p.current();p.settings=std::move(settings);
    p.console=std::make_unique<Impl::Console>();
    auto& system=p.console->System();
    system.UnregisterCommand("run");
    system.UnregisterCommand("clear");
    system.RegisterCommand("clear","Clear retained and queued logs",[this] {
        auto& p=*impl_; std::lock_guard<std::mutex> lock(p.mutex);
        p.pending.clear(); p.console->System().Items().clear();
    });
    system.RegisterCommand("status","Display host status",[this] {
        const auto& s=impl_->state; std::ostringstream out;
        out << "screen=" << s.screen << " FPS=" << s.fps << " ms=" << s.milliseconds
            << " quads=" << s.quads << " glyphs=" << s.glyphs << " textures=" << s.textures
            << " missing=" << s.missing << " clipped=" << s.clipped;
        impl_->console->System().Log() << out.str() << csys::endl;
    });
    system.RegisterCommand("warnings","List retained missing-feature warnings and exact occurrence counts",[this] {
        for(const auto& warning:port::diagnostics::warnings_snapshot())
            impl_->console->System().Log(csys::WARNING)
                << port::diagnostics::detail::warning_message(warning) << csys::endl;
        impl_->trim();
    });
    system.RegisterCommand("info","Set detailed information panel on/off",[this](const csys::String& value) {
        if (value.m_String=="on") impl_->details=true;
        else if (value.m_String=="off") impl_->details=false;
        else impl_->console->System().Log(csys::ERROR) << "Usage: info on/off" << csys::endl;
    },csys::Arg<csys::String>("on/off"));

}
DebugConsole::~DebugConsole() = default;
bool DebugConsole::visible() const {return impl_->open;}
bool DebugConsole::info() const {return impl_->details;}
void DebugConsole::set_info(bool value) {impl_->details=value;}
void DebugConsole::log(std::string text,bool error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->pending.emplace_back(std::move(text),error?csys::ERROR:csys::LOG);
    while(impl_->pending.size()>log_capacity) impl_->pending.pop_front();
}
void DebugConsole::warning(std::string text) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->pending.emplace_back(std::move(text),csys::WARNING);
    while(impl_->pending.size()>log_capacity) impl_->pending.pop_front();
}
void DebugConsole::snapshot(Snapshot value) {impl_->state=std::move(value);}
Snapshot DebugConsole::snapshot() const {return impl_->state;}
csys::System& DebugConsole::commands() {impl_->current();return impl_->console->System();}
void DebugConsole::execute(const std::string& command) {
    impl_->current();impl_->drain();commands().RunCommand(command);impl_->trim();
}
std::size_t DebugConsole::log_count() {impl_->drain();return commands().Items().size();}
const render::Bitmap& DebugConsole::font() const {return impl_->ui->font();}
void DebugConsole::clipboard(const char* (*get)(void*),void (*set)(void*,const char*),void* user) {
    impl_->ui->clipboard(get,set,user);
}
void DebugConsole::enable_console(bool enabled){impl_->enabled=enabled;if(!enabled)impl_->open=false;}
void DebugConsole::enable_viewer_input(bool enabled){impl_->viewer_input=enabled;}
bool DebugConsole::capture_mouse()const{return impl_->open||impl_->ui->capture_mouse();}
bool DebugConsole::capture_keyboard()const{return impl_->open||impl_->ui->capture_keyboard();}
bool DebugConsole::input(const Input& e){
    auto& p=*impl_;p.current();
    if(e.kind==Input::Kind::FocusLost){p.ui->reset_input();p.toggle_down=false;return false;}
    if(e.kind==Input::Kind::Toggle){
        if(!p.enabled)return false;
        if(e.pressed&&!e.repeat&&!p.toggle_down){p.open=!p.open;p.console->focus=p.open;p.ui->reset_input();}
        p.toggle_down=e.pressed;return true;
    }
    if(!p.open&&!p.viewer_input)return false;
    const bool captured=p.ui->input(e);return p.open||captured;
}
std::vector<Batch> DebugConsole::frame(float width,float height,float delta,const std::function<void()>& draw){
    auto& p=*impl_;p.current();p.drain();p.ui->begin(width,height,delta);
    if(std::isfinite(delta)&&delta>0&&std::isfinite(delta*1000.f)){
        p.frame_ms[p.frame_next]=delta*1000.f;
        p.frame_next=(p.frame_next+1)%static_cast<int>(p.frame_ms.size());
        p.frame_count=std::min(p.frame_count+1,static_cast<int>(p.frame_ms.size()));
    }
    if(draw)draw();
    if(p.details){
        ImGui::SetNextWindowSizeConstraints({16,16},{std::max(16.f,width/p.ui->scale()-16),std::max(16.f,height/p.ui->scale()-16)});
        ImGui::SetNextWindowSize({300,155},ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos({8,36},ImGuiCond_FirstUseEver);
        if(ImGui::Begin("HP2 Debug Info",&p.details)){
            const float latest=p.frame_count?p.frame_ms[(p.frame_next+239)%240]:0.f;
            const float peak=p.frame_count?*std::max_element(p.frame_ms.begin(),p.frame_ms.begin()+p.frame_count):0.f;
            ImGui::Text("%.0f FPS  |  %.2f ms",latest>0?1000.f/latest:0.f,latest);
            ImGui::TextDisabled("Frame ms  |  peak %.2f  |  last %d frames",peak,p.frame_count);
            ImGui::PlotLines("##frame_curve",p.frame_ms.data(),p.frame_count,
                p.frame_count==240?p.frame_next:0,nullptr,0.f,std::max(33.33f,peak*1.1f),{ImGui::GetContentRegionAvail().x,70});
        }ImGui::End();
    }
    if(p.open){
        ImGui::SetNextWindowSize({std::max(16.f,std::min(760.f,width/p.ui->scale()-16)),std::max(16.f,std::min(440.f,height/p.ui->scale()-44))},ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos({8,36},ImGuiCond_FirstUseEver);p.console->Draw();
    }
    p.trim();return p.ui->finish(width,height);
}
void DebugConsole::set_ui_scale(float scale){impl_->ui->set_scale(scale);}
float DebugConsole::ui_scale()const{return impl_->ui->scale();}
}
