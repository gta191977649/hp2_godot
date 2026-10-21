#include "debug_ui.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <stdexcept>
#include <cmath>
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

namespace {
// Port-side viewport containment. Also repairs persisted positions after resize.
void contain_windows(ImGuiContext* context,bool drawn){
    const auto limit=ImGui::GetIO().DisplaySize;
    for(auto* window:context->Windows){
        if(window->Flags&ImGuiWindowFlags_ChildWindow)continue;
        const ImVec2 size{std::min(window->SizeFull.x,limit.x),std::min(window->SizeFull.y,limit.y)};
        if((!drawn&&!(window->Flags&ImGuiWindowFlags_AlwaysAutoResize))||window->Size.x>limit.x||window->Size.y>limit.y){
            ImGui::SetWindowSize(window,size,ImGuiCond_Always);
            if(drawn)window->Size={std::min(window->Size.x,limit.x),std::min(window->Size.y,limit.y)};
        }
        const ImVec2 pos{std::clamp(window->Pos.x,0.f,std::max(0.f,limit.x-window->Size.x)),std::clamp(window->Pos.y,0.f,std::max(0.f,limit.y-window->Size.y))};
        const ImVec2 delta{pos.x-window->Pos.x,pos.y-window->Pos.y};
        if(delta.x==0&&delta.y==0)continue;
        if(drawn)for(auto* child:context->Windows)if(child==window||child->RootWindow==window){
            for(auto& vertex:child->DrawList->VtxBuffer){vertex.pos.x+=delta.x;vertex.pos.y+=delta.y;}
            for(auto& command:child->DrawList->CmdBuffer){command.ClipRect.x=std::max(0.f,command.ClipRect.x+delta.x);command.ClipRect.z=std::min(limit.x,command.ClipRect.z+delta.x);command.ClipRect.y=std::max(0.f,command.ClipRect.y+delta.y);command.ClipRect.w=std::min(limit.y,command.ClipRect.w+delta.y);}
        }
        if(drawn)for(auto* child:context->Windows)if(child!=window&&child->RootWindow==window){
            ImGui::SetWindowPos(child,{child->Pos.x+delta.x,child->Pos.y+delta.y},ImGuiCond_Always);child->OuterRectClipped.Translate(delta);
        }
        ImGui::SetWindowPos(window,pos,ImGuiCond_Always);
        window->OuterRectClipped=ImRect(pos,{std::min(limit.x,pos.x+window->Size.x),std::min(limit.y,pos.y+window->Size.y)});
    }
}
}
struct DebugUiSession::Impl { ImGuiContext* context=nullptr;render::Bitmap atlas;std::string settings;std::vector<ImRect> input_windows;float scale=1; };
DebugUiSession::DebugUiSession(const std::string& settings):impl_(std::make_unique<Impl>()) {
    impl_->context=ImGui::CreateContext();current();impl_->settings=settings;
    auto& io=ImGui::GetIO();io.IniFilename=settings.empty()?nullptr:impl_->settings.c_str();
    io.BackendFlags|=ImGuiBackendFlags_RendererHasVtxOffset;
    for(int i=0;i<ImGuiKey_COUNT;++i)io.KeyMap[i]=i;
    unsigned char* bytes=nullptr;int w=0,h=0;io.Fonts->GetTexDataAsRGBA32(&bytes,&w,&h);
    impl_->atlas.width=w;impl_->atlas.height=h;impl_->atlas.rgba.assign(bytes,bytes+w*h*4);
    io.Fonts->SetTexID(reinterpret_cast<void*>(1));
}
DebugUiSession::~DebugUiSession(){current();ImGui::GetIO().IniFilename=nullptr;ImGui::DestroyContext(impl_->context);}
void DebugUiSession::current(){ImGui::SetCurrentContext(impl_->context);}
void DebugUiSession::save_settings(){current();if(!impl_->settings.empty())ImGui::SaveIniSettingsToDisk(impl_->settings.c_str());}
const render::Bitmap& DebugUiSession::font()const{return impl_->atlas;}
float DebugUiSession::scale()const{return impl_->scale;}
void DebugUiSession::set_scale(float scale){
    if(!std::isfinite(scale)||scale<.5f||scale>8.f)throw std::invalid_argument("UI scale must be finite and within0.5..8");
    if(std::abs(scale-impl_->scale)<.001f)return;
    current();auto& io=ImGui::GetIO();const float ratio=impl_->scale/scale;
    // Mouse state is logical too, including a held drag across a monitor move.
    const auto rescale=[&](ImVec2& point){if(std::isfinite(point.x)&&std::isfinite(point.y)&&ImGui::IsMousePosValid(&point)){point.x*=ratio;point.y*=ratio;}};
    rescale(io.MousePos);rescale(io.MousePosPrev);
    for(auto& point:io.MouseClickedPos)rescale(point);
    impl_->input_windows.clear();impl_->scale=scale;
    io.Fonts->Clear();ImFontConfig config;config.SizePixels=13.f*scale;
    io.Fonts->AddFontDefault(&config);io.FontGlobalScale=1.f/scale;
    unsigned char* pixels=nullptr;int width=0,height=0;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    impl_->atlas.width=width;impl_->atlas.height=height;impl_->atlas.rgba.assign(pixels,pixels+width*height*4);++impl_->atlas.revision;
    io.Fonts->SetTexID(reinterpret_cast<void*>(1));
    ImGui::SetCurrentFont(io.Fonts->Fonts[0]);
}
void DebugUiSession::begin(float width,float height,float delta){
    current();auto& io=ImGui::GetIO();io.DisplaySize={std::max(1.f,width/impl_->scale),std::max(1.f,height/impl_->scale)};
    io.DisplayFramebufferScale={impl_->scale,impl_->scale};
    ImGui::GetStyle().WindowMinSize={std::min(32.f,io.DisplaySize.x),std::min(32.f,io.DisplaySize.y)};
    io.DeltaTime=std::max(.00001f,delta);contain_windows(impl_->context,false);ImGui::NewFrame();
}
std::vector<Batch> DebugUiSession::finish(float width,float height){
    current();contain_windows(impl_->context,true);ImGui::Render();std::vector<Batch> output;
    impl_->input_windows.clear();
    for(auto* window:impl_->context->Windows)
        if(window->Active && !window->Hidden && !(window->Flags&ImGuiWindowFlags_NoInputs))
            impl_->input_windows.push_back(window->OuterRectClipped);
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
bool DebugUiSession::capture_mouse()const{
    ImGui::SetCurrentContext(impl_->context);const auto& io=ImGui::GetIO();
    // Event delivery precedes NewFrame: hit-test the last visible panel at the
    // current cursor so the first click cannot start a camera drag underneath it.
    for(const auto& rectangle:impl_->input_windows)if(rectangle.Contains(io.MousePos))return true;
    return ImGui::IsAnyMouseDown()&&io.WantCaptureMouse;
}
bool DebugUiSession::capture_keyboard()const{ImGui::SetCurrentContext(impl_->context);return ImGui::GetIO().WantCaptureKeyboard;}
void DebugUiSession::reset_input(){
    current();auto& io=ImGui::GetIO();std::fill(std::begin(io.KeysDown),std::end(io.KeysDown),false);
    std::fill(std::begin(io.MouseDown),std::end(io.MouseDown),false);io.KeyCtrl=io.KeyShift=io.KeyAlt=io.KeySuper=false;
}
bool DebugUiSession::input(const Input& e){
    current();auto& io=ImGui::GetIO();
    if(e.kind==Input::Kind::FocusLost){reset_input();return false;}
    io.KeyCtrl=e.ctrl;io.KeyShift=e.shift;io.KeyAlt=e.alt;io.KeySuper=e.super;
    switch(e.kind){
    case Input::Kind::Key:if(e.code>=0&&e.code<static_cast<int>(Key::Count))io.KeysDown[e.code]=e.pressed;return capture_keyboard();
    case Input::Kind::Text:if(e.unicode)io.AddInputCharacter(e.unicode);return capture_keyboard();
    case Input::Kind::Mouse:io.MousePos={e.x/impl_->scale,e.y/impl_->scale};return capture_mouse();
    case Input::Kind::Button:if(e.code>=0&&e.code<5)io.MouseDown[e.code]=e.pressed;return capture_mouse();
    case Input::Kind::Wheel:io.MouseWheelH+=e.x;io.MouseWheel+=e.y;return capture_mouse();
    default:return false;
    }
}
void DebugUiSession::clipboard(const char* (*get)(void*),void (*set)(void*,const char*),void* user){
    current();auto& io=ImGui::GetIO();io.GetClipboardTextFn=get;io.SetClipboardTextFn=set;io.ClipboardUserData=user;
}
}
