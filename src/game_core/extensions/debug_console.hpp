#pragma once
// Port-side debug tooling. No original-game counterpart or ABI is asserted.
#include "port/render/frame.hpp"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
namespace csys { class System; }
namespace port::extensions {
enum class Key { Tab, Left, Right, Up, Down, PageUp, PageDown, Home, End,
    Insert, Delete, Backspace, Space, Enter, Escape, KeyPadEnter, A, C, V, X, Y, Z, Count };
struct Input {
    enum class Kind { Key, Text, Mouse, Button, Wheel, FocusLost, Toggle } kind;
    int code = 0;
    bool pressed = false, repeat = false, ctrl = false, shift = false, alt = false, super = false;
    float x = 0, y = 0;
    unsigned unicode = 0;
};
struct Snapshot {
    std::string screen;
    double fps = 0, milliseconds = 0;
    int quads = 0, glyphs = 0, textures = 0, missing = 0, clipped = 0;
};
struct Vertex { render::Vec2 position, uv; render::Color color; };
struct Batch { render::Rect clip; std::vector<Vertex> vertices; std::vector<int> indices; };
// Applies display origin/scale, clamps the scissor and honors both command offsets.
Batch convert_batch(const std::vector<Vertex>& vertices, const std::vector<unsigned>& indices,
    std::size_t index_offset, std::size_t count, std::size_t vertex_offset,
    render::Rect clip, render::Vec2 origin, render::Vec2 scale, render::Vec2 viewport);
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
    std::vector<Batch> frame(float width, float height, float delta);
    void clipboard(const char* (*get)(void*), void (*set)(void*, const char*), void* user);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
