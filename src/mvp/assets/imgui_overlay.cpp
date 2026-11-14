#include "imgui_overlay.hpp"
#include "trackviewer/track_viewer.hpp"
#include "systems/effects/race_effect_options.hpp"

#include <imgui_console/imgui_console.h>
#include <imgui_internal.h>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef ERROR

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

namespace hp2 {
using namespace godot;

namespace {
constexpr std::array<Key, ImGuiKey_COUNT> keys{
    KEY_TAB, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_PAGEUP, KEY_PAGEDOWN,
    KEY_HOME, KEY_END, KEY_INSERT, KEY_DELETE, KEY_BACKSPACE, KEY_SPACE, KEY_ENTER,
    KEY_ESCAPE, KEY_KP_ENTER, KEY_A, KEY_C, KEY_V, KEY_X, KEY_Y, KEY_Z
};

class Console : public ImGuiConsole {
public:
    Console() : ImGuiConsole("HP2 Debug Console") {}
    bool focus{};

    void reveal_error() {
        m_TextFilter.Clear();
        m_ScrollToBottom = true;
    }

private:
    void InputBar() override {
        if (focus) {
            ImGui::SetKeyboardFocusHere();
            focus = false;
        }
        ImGuiConsole::InputBar();
    }
};
}

struct ImGuiOverlay::Storage {
    explicit Storage(Node& owner) {
        auto* layer = memnew(CanvasLayer);
        owner.add_child(layer);
        layer->set_layer(100);
        canvas = memnew(Node2D);
        layer->add_child(canvas);
        canvas->set_texture_filter(CanvasItem::TEXTURE_FILTER_LINEAR);
        context = ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        for (std::size_t i = 0; i < keys.size(); ++i) io.KeyMap[i] = static_cast<int>(i);
        console = std::make_unique<Console>();
        console->System().UnregisterCommand("run");
        console->System().RegisterCommand("effect", "Open the race effect controls", [this] {
            effectsOpen = true;
            focusEffects = true;
        });
        io.ClipboardUserData = this;
        io.GetClipboardTextFn = [](void* data) {
            auto& state = *static_cast<Storage*>(data);
            state.clipboard = DisplayServer::get_singleton()->clipboard_get().utf8();
            return state.clipboard.get_data();
        };
        io.SetClipboardTextFn = [](void*, const char* text) {
            DisplayServer::get_singleton()->clipboard_set(String::utf8(text));
        };

        baseStyle = ImGui::GetStyle();
    }

    void update_dpi() {
        const auto id = canvas->get_window()->get_window_id();
        auto* display = DisplayServer::get_singleton();
        const auto hwnd = reinterpret_cast<HWND>(
            display->window_get_native_handle(DisplayServer::WINDOW_HANDLE, id));
        using GetWindowDpi = UINT (WINAPI *)(HWND);
        static const auto getWindowDpi = reinterpret_cast<GetWindowDpi>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
        const auto nativeDpi = hwnd && getWindowDpi ? getWindowDpi(hwnd) : 0;
        const auto dpi = nativeDpi ? nativeDpi :
            std::max(96, display->screen_get_dpi(display->window_get_current_screen(id)));
        const float scale = static_cast<float>(dpi) / 96.0f;
        if (scale == dpiScale) return;
        dpiScale = scale;
        ImGui::GetStyle() = baseStyle;
        ImGui::GetStyle().ScaleAllSizes(scale);
        auto& io = ImGui::GetIO();
        io.FontDefault = nullptr;
        io.Fonts->Clear();
        ImFontConfig config;
        config.SizePixels = std::round(13.0f * scale);
        io.Fonts->AddFontDefault(&config);
        unsigned char* bytes{};
        int width{}, height{};
        io.Fonts->GetTexDataAsRGBA32(&bytes, &width, &height);
        PackedByteArray pixels;
        pixels.resize(static_cast<int64_t>(width) * height * 4);
        std::memcpy(pixels.ptrw(), bytes, pixels.size());
        const auto image = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, pixels);
        if (atlas.is_valid()) atlas->set_image(image);
        else atlas = ImageTexture::create_from_image(image);
        io.Fonts->SetTexID(atlas.ptr());
        std::fprintf(stderr, "HP2_MVP_TRACE stage=debug_overlay_dpi dpi=%u scale=%.2f\n",
            static_cast<unsigned>(dpi), scale);
    }

    ~Storage() {
        auto* rendering = RenderingServer::get_singleton();
        for (const auto& batch : batches) rendering->free_rid(batch);
        ImGui::SetCurrentContext(context);
        console.reset();
        ImGui::DestroyContext(context);
    }

    ImGuiContext* context{};
    std::unique_ptr<Console> console;
    Node2D* canvas{};
    Ref<ImageTexture> atlas;
    CharString clipboard;
    ImGuiStyle baseStyle;
    float dpiScale{};
    std::vector<RID> batches;
    std::array<float, 240> frameTimes{};
    std::size_t nextSample{}, sampleCount{};
    std::chrono::steady_clock::time_point previousFrame{};
    bool open{};
    bool effectsOpen{};
    bool focusEffects{};
    RaceEffectOptions effectOptions;
    Rect2 effectsRect;
    TrackViewer* viewer{};

    void draw(const ImDrawData& data) {
        auto* rendering = RenderingServer::get_singleton();
        std::size_t used = 0;
        for (int i = 0; i < data.CmdListsCount; ++i) {
            const auto& list = *data.CmdLists[i];
            PackedVector2Array points, uv;
            PackedColorArray colors;
            for (const auto& vertex : list.VtxBuffer) {
                points.push_back({vertex.pos.x, vertex.pos.y});
                uv.push_back({vertex.uv.x, vertex.uv.y});
                const auto lane = [&](int shift) { return ((vertex.col >> shift) & 255u) / 255.0f; };
                colors.push_back({lane(IM_COL32_R_SHIFT), lane(IM_COL32_G_SHIFT),
                    lane(IM_COL32_B_SHIFT), lane(IM_COL32_A_SHIFT)});
            }
            for (const auto& command : list.CmdBuffer) {
                if (command.UserCallback || !command.ElemCount || command.TextureId != atlas.ptr()) continue;
                const auto& clip = command.ClipRect;
                if (clip.z <= clip.x || clip.w <= clip.y) continue;
                if (used == batches.size()) {
                    batches.push_back(rendering->canvas_item_create());
                    rendering->canvas_item_set_parent(batches.back(), canvas->get_canvas_item());
                    rendering->canvas_item_set_draw_index(batches.back(), static_cast<int>(used));
                }
                const auto item = batches[used++];
                PackedInt32Array indices;
                for (unsigned j = 0; j < command.ElemCount; ++j)
                    indices.push_back(list.IdxBuffer[command.IdxOffset + j] + command.VtxOffset);
                rendering->canvas_item_clear(item);
                rendering->canvas_item_set_custom_rect(item, true,
                    Rect2(clip.x, clip.y, clip.z - clip.x, clip.w - clip.y));
                rendering->canvas_item_set_clip(item, true);
                rendering->canvas_item_add_triangle_array(item, indices, points, colors, uv, {}, {}, atlas->get_rid());
            }
        }
        while (batches.size() > used) {
            rendering->free_rid(batches.back());
            batches.pop_back();
        }
    }
};

ImGuiOverlay::ImGuiOverlay(Node& owner) : storage_(std::make_unique<Storage>(owner)) {
    log("debug", "ImGui ready. The grave/tilde key toggles the console and returns keyboard control to the game.");
    std::fprintf(stderr, "HP2_MVP_TRACE stage=debug_overlay console=hidden graph=visible\n");
}

ImGuiOverlay::~ImGuiOverlay() = default;

bool ImGuiOverlay::captures_game_input() const noexcept {
    return storage_->open || (storage_->viewer && storage_->viewer->modal());
}

void ImGuiOverlay::attach_track_viewer(TrackViewer& viewer, std::function<void()> open) {
    auto& s = *storage_;
    s.viewer = &viewer;
    s.console->System().RegisterCommand("trackviewer", "Browse all tracks and inspect entities",
        [this, open = std::move(open)] {
            storage_->open = false;
            open();
        });
}

const RaceEffectOptions& ImGuiOverlay::effect_options() const noexcept { return storage_->effectOptions; }

void ImGuiOverlay::show_console() {
    storage_->open = true;
    storage_->console->focus = true;
    storage_->console->reveal_error();
}

void ImGuiOverlay::log(const char* event, const char* detail, bool error) {
    auto& system = storage_->console->System();
    if (error) system.Log(csys::ERROR) << detail << csys::endl;
    else system.Log() << event << ": " << detail << csys::endl;
    auto& items = system.Items();
    constexpr std::size_t capacity = 1024;
    if (items.size() > capacity) items.erase(items.begin(), items.end() - capacity);
}

bool ImGuiOverlay::input(const Ref<InputEvent>& event) {
    auto& s = *storage_;
    ImGui::SetCurrentContext(s.context);
    auto& io = ImGui::GetIO();
    if (const Ref<InputEventKey> key = event; key.is_valid()) {
        if (event->is_action("hp2_debug_console")) {
            if (event->is_action_pressed("hp2_debug_console")) {
                s.open = !s.open;
                s.console->focus = s.open;
                std::fill(std::begin(io.KeysDown), std::end(io.KeysDown), false);
                std::fill(std::begin(io.MouseDown), std::end(io.MouseDown), false);
            }
            return true;
        }
        if (!s.open && !(s.viewer && s.viewer->modal()) &&
            !(s.viewer && s.viewer->active() && !s.viewer->captured() && io.WantTextInput)) return false;
        const auto found = std::find(keys.begin(), keys.end(), key->get_keycode());
        if (found != keys.end()) io.KeysDown[found - keys.begin()] = key->is_pressed();
        io.KeyCtrl = key->is_ctrl_pressed();
        io.KeyShift = key->is_shift_pressed();
        io.KeyAlt = key->is_alt_pressed();
        io.KeySuper = key->is_meta_pressed();
        if (key->is_pressed() && key->get_unicode() >= 32 && !io.KeyCtrl && !io.KeySuper)
            io.AddInputCharacter(key->get_unicode());
        return true;
    }
    if (s.viewer && s.viewer->captured() && !s.open) return false;
    if (!s.open && !s.effectsOpen && !(s.viewer && (s.viewer->active() || s.viewer->modal()))) return false;
    if (const Ref<InputEventMouse> mouse = event; mouse.is_valid()) {
        const auto position = mouse->get_position();
        io.MousePos = {position.x, position.y};
        if (const Ref<InputEventMouseButton> button = event; button.is_valid()) {
            const auto index = button->get_button_index();
            if (index >= MOUSE_BUTTON_LEFT && index <= MOUSE_BUTTON_MIDDLE)
                io.MouseDown[index - MOUSE_BUTTON_LEFT] = button->is_pressed();
            if (button->is_pressed()) {
                if (index == MOUSE_BUTTON_WHEEL_UP) io.MouseWheel += button->get_factor();
                if (index == MOUSE_BUTTON_WHEEL_DOWN) io.MouseWheel -= button->get_factor();
            }
        }
        const auto* console = s.open ? ImGui::FindWindowByName("HP2 Debug Console") : nullptr;
        const bool overConsole = console && console->Rect().Contains(io.MousePos);
        return overConsole || (s.effectsOpen && s.effectsRect.has_point(position)) ||
            (s.viewer && s.viewer->modal()) ||
            ((s.effectsOpen || (s.viewer && s.viewer->active())) && io.WantCaptureMouse);
    }
    return false;
}

void ImGuiOverlay::frame(double delta) {
    auto& s = *storage_;
    ImGui::SetCurrentContext(s.context);
    const auto size = s.canvas->get_viewport_rect().size;
    if (size.x <= 24 || size.y <= 24) return;
    const float previousScale = s.dpiScale;
    s.update_dpi();
    const auto scale = s.dpiScale;
    auto& io = ImGui::GetIO();
    io.DisplaySize = {size.x, size.y};
    if (!s.canvas->get_window()->has_focus() || (s.viewer && s.viewer->captured() && !s.open)) {
        std::fill(std::begin(io.KeysDown), std::end(io.KeysDown), false);
        std::fill(std::begin(io.MouseDown), std::end(io.MouseDown), false);
        io.KeyCtrl = io.KeyShift = io.KeyAlt = io.KeySuper = false;
        if (s.viewer && s.viewer->captured() && !s.open) io.MousePos = {-FLT_MAX, -FLT_MAX};
    }
    io.DeltaTime = static_cast<float>(std::max(delta, 0.000001));
    const auto now = std::chrono::steady_clock::now();
    const float ms = s.sampleCount
        ? std::chrono::duration<float, std::milli>(now - s.previousFrame).count()
        : io.DeltaTime * 1000.0f;
    s.previousFrame = now;
    s.frameTimes[s.nextSample] = ms;
    s.nextSample = (s.nextSample + 1) % s.frameTimes.size();
    s.sampleCount = std::min(s.sampleCount + 1, s.frameTimes.size());
    const float peak = *std::max_element(s.frameTimes.begin(), s.frameTimes.end());

    ImGui::NewFrame();
    const float margin = 12 * scale;
    const float graphWidth = 270 * scale;
    const bool previousRoadshine = s.effectOptions.roadshine;
    if (s.viewer) s.viewer->draw(size.x, size.y, scale, io.Framerate, s.open, s.effectOptions);
    if (s.open) {
        const auto condition = previousScale != scale ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
        ImGui::SetNextWindowPos({margin, margin}, condition);
        const float width = std::max(240 * scale, size.x - graphWidth - 3 * margin);
        ImGui::SetNextWindowSize({std::min(520 * scale, width),
            std::min(270 * scale, std::max(1.0f, size.y - 2 * margin))}, condition);
        s.console->Draw();
    }
    if (s.effectsOpen) {
        if (s.focusEffects) {
            ImGui::SetNextWindowFocus();
            s.focusEffects = false;
        }
        ImGui::SetNextWindowPos({size.x * 0.5f, margin}, ImGuiCond_FirstUseEver, {0.5f, 0});
        if (ImGui::Begin("effect", &s.effectsOpen, ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::Checkbox("Fog", &s.effectOptions.fog);
            ImGui::Checkbox("Depth of field", &s.effectOptions.depthOfField);
            ImGui::Checkbox("Black and white", &s.effectOptions.monochrome);
            ImGui::Checkbox("Color filter", &s.effectOptions.colorFilter);
            ImGui::Checkbox("Motion blur", &s.effectOptions.motionBlur);
            ImGui::Checkbox("Track visibility zones", &s.effectOptions.sceneryVisibility);
            ImGui::Separator();
            ImGui::Checkbox("Race particles (fire, smoke, sandstorms)", &s.effectOptions.particles);
            ImGui::Checkbox("Sun and glare", &s.effectOptions.sunGlare);
            ImGui::Checkbox("Roadshine", &s.effectOptions.roadshine);
            ImGui::TextUnformatted("Checked effects follow race triggers.");
        }
        const auto position = ImGui::GetWindowPos();
        const auto extent = ImGui::GetWindowSize();
        s.effectsRect = Rect2(position.x, position.y, extent.x, extent.y);
        ImGui::End();
    }
    if (previousRoadshine != s.effectOptions.roadshine)
        RenderingServer::get_singleton()->global_shader_parameter_set("hp2_roadshine", s.effectOptions.roadshine);
    if (!s.viewer || !s.viewer->active()) {
        ImGui::SetNextWindowPos({size.x - margin, margin}, ImGuiCond_Always, {1, 0});
        ImGui::SetNextWindowBgAlpha(0.8f);
        if (ImGui::Begin("HP2 Frame Time", nullptr, ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::Text("%.1f FPS  %.2f ms  Peak %.2f ms", 1000.0f / std::max(ms, 0.001f), ms, peak);
            const auto offset = s.sampleCount == s.frameTimes.size() ? s.nextSample : 0;
            ImGui::PlotLines("##frame_ms", s.frameTimes.data(), static_cast<int>(s.sampleCount),
                static_cast<int>(offset), nullptr, 0.0f, std::max(16.67f, peak), {250 * scale, 72 * scale});
            ImGui::TextUnformatted("` / ~ toggles console");
        }
        ImGui::End();
    }
    ImGui::Render();
    s.draw(*ImGui::GetDrawData());
}

}
