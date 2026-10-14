#include "debug_console.hpp"
#include <imgui_console.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <deque>
#include <mutex>
#include <sstream>
#include <stdexcept>
namespace port::extensions {
Batch convert_batch(const std::vector<Vertex>& vertices, const std::vector<unsigned>& indices,
    std::size_t offset, std::size_t count, std::size_t base, render::Rect clip,
    render::Vec2 origin, render::Vec2 scale, render::Vec2 viewport) {
    Batch result;
    const float left = std::max(0.f, (clip.x-origin.x)*scale.x);
    const float top = std::max(0.f, (clip.y-origin.y)*scale.y);
    const float right = std::min(viewport.x, (clip.x+clip.width-origin.x)*scale.x);
    const float bottom = std::min(viewport.y, (clip.y+clip.height-origin.y)*scale.y);
    if (right <= left || bottom <= top) return result;
    if (offset > indices.size() || count > indices.size()-offset || count % 3)
        throw std::out_of_range("Invalid console triangle index range");
    result.clip = {left,top,right-left,bottom-top};
    // Expand only referenced vertices; this also removes any 16-bit index limitation.
    for (std::size_t i=0;i<count;++i) {
        const auto index = static_cast<std::size_t>(indices[offset+i]);
        if (base > vertices.size() || index >= vertices.size()-base)
            throw std::out_of_range("Invalid console vertex offset");
        auto vertex = vertices[base+index];
        vertex.position = {(vertex.position.x-origin.x)*scale.x,(vertex.position.y-origin.y)*scale.y};
        result.indices.push_back(static_cast<int>(result.vertices.size()));
        result.vertices.push_back(vertex);
    }
    return result;
}
struct DebugConsole::Impl {
    struct Console : ImGuiConsole {
        Console(): ImGuiConsole("HP2 Debug Console") {}
        bool focus = false;
        void InputBar() override;
    };
    ImGuiContext* context = nullptr;
    std::unique_ptr<Console> console;
    render::Bitmap atlas;
    std::string settings;
    bool open = false, details = false, toggle_down = false;
    Snapshot state;
    std::mutex mutex;
    std::deque<std::pair<std::string,bool>> pending;
    ~Impl() {
        if (!context) return;
        current();
        // Settings handlers contain pointers to Console; save before destroying it.
        if (!settings.empty()) ImGui::SaveIniSettingsToDisk(settings.c_str());
        ImGui::GetIO().IniFilename=nullptr;
        console.reset();
        ImGui::DestroyContext(context);
    }
    void current() { ImGui::SetCurrentContext(context); }
    void trim() {
        auto& items = console->System().Items();
        if (items.size()>log_capacity) items.erase(items.begin(),items.end()-log_capacity);
    }
    void drain() {
        std::deque<std::pair<std::string,bool>> incoming;
        { std::lock_guard<std::mutex> lock(mutex); incoming.swap(pending); }
        for (auto& entry:incoming)
            console->System().Log(entry.second?csys::ERROR:csys::LOG) << entry.first << csys::endl;
        trim();
    }
};
void DebugConsole::Impl::Console::InputBar() {
    if (focus) { ImGui::SetKeyboardFocusHere(); focus=false; }
    ImGuiConsole::InputBar();
}
DebugConsole::DebugConsole(std::string settings):impl_(std::make_unique<Impl>()) {
    auto& p=*impl_;
    p.context=ImGui::CreateContext(); p.current(); p.settings=std::move(settings);
    auto& io=ImGui::GetIO(); io.IniFilename=p.settings.empty()?nullptr:p.settings.c_str();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    for (int i=0;i<ImGuiKey_COUNT;++i) io.KeyMap[i]=i;
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
    system.RegisterCommand("info","Set detailed information panel on/off",[this](const csys::String& value) {
        if (value.m_String=="on") impl_->details=true;
        else if (value.m_String=="off") impl_->details=false;
        else impl_->console->System().Log(csys::ERROR) << "Usage: info on/off" << csys::endl;
    },csys::Arg<csys::String>("on/off"));
    unsigned char* bytes=nullptr; int w=0,h=0;
    io.Fonts->GetTexDataAsRGBA32(&bytes,&w,&h);
    p.atlas.width=w;p.atlas.height=h;p.atlas.rgba.assign(bytes,bytes+w*h*4);
    io.Fonts->SetTexID(reinterpret_cast<void*>(1));
}
DebugConsole::~DebugConsole() = default;
bool DebugConsole::visible() const {return impl_->open;}
bool DebugConsole::info() const {return impl_->details;}
void DebugConsole::set_info(bool value) {impl_->details=value;}
void DebugConsole::log(std::string text,bool error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->pending.emplace_back(std::move(text),error);
    while(impl_->pending.size()>log_capacity) impl_->pending.pop_front();
}
void DebugConsole::snapshot(Snapshot value) {impl_->state=std::move(value);}
Snapshot DebugConsole::snapshot() const {return impl_->state;}
csys::System& DebugConsole::commands() {impl_->current();return impl_->console->System();}
void DebugConsole::execute(const std::string& command) {
    impl_->current();impl_->drain();commands().RunCommand(command);impl_->trim();
}
std::size_t DebugConsole::log_count() {impl_->drain();return commands().Items().size();}
const render::Bitmap& DebugConsole::font() const {return impl_->atlas;}
void DebugConsole::clipboard(const char* (*get)(void*),void (*set)(void*,const char*),void* user) {
    impl_->current();auto& io=ImGui::GetIO();io.GetClipboardTextFn=get;io.SetClipboardTextFn=set;io.ClipboardUserData=user;
}
bool DebugConsole::input(const Input& e) {
    auto& p=*impl_;p.current();auto& io=ImGui::GetIO();
    if(e.kind==Input::Kind::FocusLost) {
        std::fill(std::begin(io.KeysDown),std::end(io.KeysDown),false);
        std::fill(std::begin(io.MouseDown),std::end(io.MouseDown),false);
        io.KeyCtrl=io.KeyShift=io.KeyAlt=io.KeySuper=false;p.toggle_down=false;return false;
    }
    if(e.kind==Input::Kind::Toggle) {
        if(e.pressed&&!e.repeat&&!p.toggle_down) {
            p.open=!p.open;p.console->focus=p.open;
            std::fill(std::begin(io.KeysDown),std::end(io.KeysDown),false);
            std::fill(std::begin(io.MouseDown),std::end(io.MouseDown),false);
        }
        p.toggle_down=e.pressed;return true;
    }
    if(!p.open) return false;
    io.KeyCtrl=e.ctrl;io.KeyShift=e.shift;io.KeyAlt=e.alt;io.KeySuper=e.super;
    switch(e.kind) {
    case Input::Kind::Key: if(e.code>=0&&e.code<static_cast<int>(Key::Count))io.KeysDown[e.code]=e.pressed;break;
    case Input::Kind::Text: if(e.unicode)io.AddInputCharacter(e.unicode);break;
    case Input::Kind::Mouse:io.MousePos={e.x,e.y};break;
    case Input::Kind::Button:if(e.code>=0&&e.code<5)io.MouseDown[e.code]=e.pressed;break;
    case Input::Kind::Wheel:io.MouseWheelH+=e.x;io.MouseWheel+=e.y;break;
    default:break;
    }
    return true;
}
std::vector<Batch> DebugConsole::frame(float width,float height,float delta) {
    auto& p=*impl_;p.current();p.drain();auto& io=ImGui::GetIO();
    io.DisplaySize={std::max(1.f,width),std::max(1.f,height)};io.DeltaTime=std::max(0.00001f,delta);
    ImGui::NewFrame();
    if(p.open) {
        ImGui::SetNextWindowSize({std::min(760.f,width-16),std::min(440.f,height-16)},ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos({8,36},ImGuiCond_FirstUseEver);
        p.console->Draw();
    }
    p.trim();ImGui::Render();std::vector<Batch> output;
    auto* data=ImGui::GetDrawData();
    for(int list=0;list<data->CmdListsCount;++list) {
        auto* draw=data->CmdLists[list];std::vector<Vertex> vertices;
        for(const auto& v:draw->VtxBuffer) {
            const auto c=v.col;
            vertices.push_back({{v.pos.x,v.pos.y},{v.uv.x,v.uv.y},
                {((c>>IM_COL32_R_SHIFT)&255)/255.f,((c>>IM_COL32_G_SHIFT)&255)/255.f,
                 ((c>>IM_COL32_B_SHIFT)&255)/255.f,((c>>IM_COL32_A_SHIFT)&255)/255.f}});
        }
        std::vector<unsigned> indices(draw->IdxBuffer.begin(),draw->IdxBuffer.end());
        for(const auto& cmd:draw->CmdBuffer) {
            if(cmd.UserCallback) {
                if(cmd.UserCallback!=ImDrawCallback_ResetRenderState)cmd.UserCallback(draw,&cmd);
                continue;
            }
            if(cmd.TextureId!=reinterpret_cast<void*>(1))throw std::runtime_error("Unsupported console texture");
            const auto& c=cmd.ClipRect;
            auto batch=convert_batch(vertices,indices,cmd.IdxOffset,cmd.ElemCount,cmd.VtxOffset,
                {c.x,c.y,c.z-c.x,c.w-c.y},{data->DisplayPos.x,data->DisplayPos.y},
                {data->FramebufferScale.x,data->FramebufferScale.y},{width,height});
            if(!batch.indices.empty())output.push_back(std::move(batch));
        }
    }
    return output;
}
}
