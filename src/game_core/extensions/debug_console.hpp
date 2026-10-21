#pragma once
// Port-side debug tooling. No original-game counterpart or ABI is asserted.
#include "debug_ui.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
namespace csys { class System; }
namespace port::extensions {
struct Snapshot {
    std::string screen;
    double fps = 0, milliseconds = 0;
    int quads = 0, glyphs = 0, textures = 0, missing = 0, clipped = 0;
};
class DebugConsole {
public:
    static constexpr std::size_t log_capacity = 2048;
    explicit DebugConsole(std::string settings_path = {});
    ~DebugConsole();
    DebugConsole(const DebugConsole&) = delete;
    DebugConsole& operator=(const DebugConsole&) = delete;
    bool input(const Input& event);
    bool visible() const;
    bool info() const;
    void set_info(bool value);
    void log(std::string text, bool error = false); // thread-safe; collects while hidden
    void snapshot(Snapshot value);
    Snapshot snapshot() const;
    void execute(const std::string& command);
    csys::System& commands(); // main-thread only, as are input/frame/execute
    std::size_t log_count();
    const render::Bitmap& font() const;
    void set_ui_scale(float scale);
    float ui_scale()const;
    std::vector<Batch> frame(float width, float height, float delta, const std::function<void()>& draw = {});
    void enable_console(bool enabled);
    void enable_viewer_input(bool enabled);
    bool capture_mouse()const;
    bool capture_keyboard()const;
    void clipboard(const char* (*get)(void*), void (*set)(void*, const char*), void* user);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
